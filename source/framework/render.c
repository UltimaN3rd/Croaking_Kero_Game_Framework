// Copyright [2025] [Nicholas Walton]
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "discrete_random.h"
#include "render.h"

#include "zen_timer.h"
#include "sprite.h"

#include <pthread.h>

void DrawCircle(sprite_t *destination, int center_x, int center_y, float radius, uint8_t color);
void DrawCircleFilled(sprite_t *destination, int center_x, int center_y, float radius, uint8_t color);

render_state_t *render_state_being_edited = NULL;
extern pthread_mutex_t update_render_swap_state_mutex;
extern render_data_t render_data;
extern bool quit;

void font_Write_Length (const font_t *font, sprite_t *destination, int left, int top, char *text, const size_t length);
void font_Write (const font_t *font, sprite_t *destination, int left, int top, char *text);

void Render_Cursor (const sprite_t *sprite, int x, int y, int offsetx, int offsety) {
	render_state_being_edited->cursor = (typeof (render_state_being_edited->cursor)) {
		.sprite = sprite,
		.x = x - offsetx,
		.y = y - offsety,
	};
}

void Render_Camera (int x, int y) {
	render_state_being_edited->camera.x = x;
	render_state_being_edited->camera.y = y;
}

static uint64_t random_state;
void Render_ScreenShake (int x, int y) {
	if (x) render_state_being_edited->camera.x += DiscreteRandom_Range(&random_state, -x, x);
	if (y) render_state_being_edited->camera.y += DiscreteRandom_Range(&random_state, -y, y);
}

void Render_Sprite_ (Render_Sprite_arguments arguments) {
	auto count = render_state_being_edited->element_count++;
	if (count >= RENDER_MAX_ELEMENTS) return;
	arguments.rotation -= (int)arguments.rotation;
	render_state_being_edited->elements[count] = (typeof((render_state_t){}.elements[0])){
		.type = render_element_sprite,
		.depth = arguments.depth,
		.flags = arguments.flags,
		.sprite = {
			.position = {.x = arguments.x, .y = arguments.y},
			.flags = arguments.sprite_flags,
			.sprite = arguments.sprite,
			.rotation = arguments.rotation,
			.originx = arguments.sprite_flags.center_horizontally ? arguments.sprite->w/2 : (arguments.sprite_flags.flip_horizontally ? arguments.sprite->w-1 - arguments.originx : arguments.originx),
			.originy = arguments.sprite_flags.center_vertically ? arguments.sprite->h/2 : (arguments.sprite_flags.flip_vertically ? arguments.sprite->h-1 - arguments.originy : arguments.originy),
		}
	};
}

void Render_SpriteSilhouette_ (uint8_t color, Render_Sprite_arguments arguments) {
	auto count = render_state_being_edited->element_count++;
	if (count >= RENDER_MAX_ELEMENTS) return;
	arguments.rotation -= (int)arguments.rotation;
	render_state_being_edited->elements[count] = (typeof((render_state_t){}.elements[0])){
		.type = render_element_sprite_silhouette,
		.depth = arguments.depth,
		.flags = arguments.flags,
		.sprite_silhouette = {
			.color = color,
			.sprite = {
				.position = {.x = arguments.x, .y = arguments.y},
				.flags = arguments.sprite_flags,
				.sprite = arguments.sprite,
				.rotation = arguments.rotation,
				.originx = arguments.sprite_flags.center_horizontally ? arguments.sprite->w/2 : (arguments.sprite_flags.flip_horizontally ? arguments.sprite->w-1 - arguments.originx : arguments.originx),
				.originy = arguments.sprite_flags.center_vertically ? arguments.sprite->h/2 : (arguments.sprite_flags.flip_vertically ? arguments.sprite->h-1 - arguments.originy : arguments.originy),
			}
		}
	};
}

void Render_Shape_ (Render_Shape_arguments arguments) {
	auto count = render_state_being_edited->element_count++;
	if (count >= RENDER_MAX_ELEMENTS) return;
	render_state_being_edited->elements[count] = (typeof((render_state_t){}.elements[0])){
		.type = render_element_shape,
		.depth = arguments.depth,
		.flags = arguments.flags,
		.shape = arguments.shape
	};
}

void Render_Text_ (Render_Text_arguments arguments) {
	auto count = render_state_being_edited->element_count++;
	if (count >= RENDER_MAX_ELEMENTS) return;
	render_state_being_edited->elements[count] = (typeof((render_state_t){}.elements[0])){
		.type = render_element_text,
		.depth = arguments.depth,
		.flags = arguments.flags,
		.text = {.x = arguments.x, .y = arguments.y, .length = arguments.length}
	};
	strncpy (render_state_being_edited->elements[count].text.string, arguments.string, sizeof(render_state_being_edited->elements[count].text.string)-1);
	render_state_being_edited->elements[count].text.string[sizeof(render_state_being_edited->elements[count].text.string)-1] = 0;
	if (arguments.length == 0) render_state_being_edited->elements[count].text.length = strlen (render_state_being_edited->elements[count].text.string);
	if (arguments.center_horizontally_on_screen) {
		auto w = font_StringDimensions(&resources_font, render_state_being_edited->elements[count].text.string).w;
		render_state_being_edited->elements[count].text.x = (RESOLUTION_WIDTH - w) / 2;
	}
	if (arguments.center_vertically_on_screen) {
		auto h = font_StringDimensions(&resources_font, render_state_being_edited->elements[count].text.string).h;
		render_state_being_edited->elements[count].text.y = (RESOLUTION_HEIGHT - h) / 2;
	}
}

void Render_Background_ (Render_Background_arguments background) {
	render_state_being_edited->background = background;
}

void Render_Particle (int x, int y, uint8_t pixel, bool ignore_camera) {
	if (render_state_being_edited->particles.count >= PARTICLES_MAX) return;
	if (!ignore_camera) {
		x -= render_state_being_edited->camera.x;
		y -= render_state_being_edited->camera.y;
	}
	if (x < 0 || x > RESOLUTION_WIDTH-1 || y < 0 || y > RESOLUTION_HEIGHT-1) return;
	int i = render_state_being_edited->particles.count++;
	render_state_being_edited->particles.position[i].x = x;
	render_state_being_edited->particles.position[i].y = y;
	render_state_being_edited->particles.pixel[i] = pixel;
}

render_state_t *Render_GetCurrentEditableState () {
	return render_state_being_edited;
}

void Render_SelectStateToEdit () {
	// Select oldest non-busy render state to replace
	static uint64_t state_count = 0;
	int found = -1;
	uint64_t lowest_state_count = -1;

	pthread_mutex_lock (&update_render_swap_state_mutex);
	for(int i = 0; i < 3; ++i) {
		if(!render_data.render_states[i].busy && render_data.render_states[i].state_count <= lowest_state_count) {
			found = i;
			lowest_state_count = render_data.render_states[i].state_count;
		}
	}
	assert (found != -1);
	render_state_being_edited = &render_data.render_states[found];
	render_state_being_edited->busy = true;

	pthread_mutex_unlock (&update_render_swap_state_mutex);

	*render_state_being_edited = (typeof(*render_state_being_edited)){
		.state_count = ++state_count,
		.busy = true,
	};
}

void Render_FinishEditingState () {
	render_state_being_edited->busy = false;
	render_state_being_edited = NULL;
}

void Render_ShowRenderTime (bool show) { render_state_being_edited->debug.show_rendertime = show; }
void Render_ShowFPS (bool show) { render_state_being_edited->debug.show_framerate = show; }

void *Render (void*) {
	LOG ("Render thread started");
	render_data.thread_initialized = true;
	
	os_GLMakeCurrent ();
	if (os_LogGLErrors ()) LOG ("OpenGL error");
	// os_CreateGLColorMap ();
	os_WindowFrameBufferCalculateScale ();

	int screen_refresh;
	screen_refresh = os_GetScreenRefreshRate ();
	LOG("Reported screen refresh rate: %d", screen_refresh);
	if(screen_refresh < 59) screen_refresh = 60;

	int64_t time_last, useconds_per_frame;
	time_last = os_uTime ();
	useconds_per_frame = 1000000 / screen_refresh;
	// LOG ("Microseconds per frame: %"PRId64"", useconds_per_frame);

	sprite_t *frame;
	bool frame_select = 0;
	frame = (render_data.frame[frame_select]);

	int64_t sleep_time;

	// zen_timer_t timer = zen_Start ();

	static uint64_t frame_index = 0;

	int i = 0;
	do {
		int j = i++ % 3;
		if(!render_data.render_states[j].busy && render_data.render_states[j].state_count > 0) break;
	} while (true);

	LOG ("Render loop beginning");

	while (!quit) {

		if (render_data.pause_thread) {
			render_data.pause_thread = false;
			while (!render_data.resume_thread) os_uSleepEfficient(10000);
		}

		// LOG ("R");
		// Wait until screen has been drawn. Usually instant.
		os_WaitForScreenRefresh ();
		static bool show_fps = false, reset_fps = true;
		int fps_this_frame = 0;
		if (show_fps){
			static int64_t last_time = 0;
			auto time = os_uTime ();
			int64_t delta = time - last_time;
			last_time = time;
			#define FPS_SAMPLES 30
			static int64_t total_time = 0;
			static int8_t sample_count = 0;
			if (reset_fps) {
				reset_fps = false;
				total_time = 0;
				sample_count = 0;
			}
			if (sample_count < FPS_SAMPLES) {
				++sample_count;
				total_time += delta;
			}
			else {
				total_time *= (float)(FPS_SAMPLES - 1) / (FPS_SAMPLES);
				total_time += delta;
			}
			static_assert (FPS_SAMPLES > 1);
			int64_t average_time = total_time / sample_count;
			fps_this_frame = 1000000.f/average_time + 0.5f;
		}
		os_SetWindowFrameBuffer (frame->p, frame->w, frame->h);

		// Select most recently updated render state for this render
		render_state_t *render_state;
		{
			int found = -1;
			uint64_t highest_state_count = 0;

			pthread_mutex_lock (&update_render_swap_state_mutex);
			for(int i = 0; i < 3; ++i) {
				if(!render_data.render_states[i].busy && render_data.render_states[i].state_count > highest_state_count) {
					found = i;
					highest_state_count = render_data.render_states[i].state_count;
				}
			}
			assert (found != -1);
			render_state = &render_data.render_states[found];
			render_state->busy = true;

			pthread_mutex_unlock (&update_render_swap_state_mutex);
		}

		if (!show_fps && render_state->debug.show_framerate) reset_fps = true;
		show_fps = render_state->debug.show_framerate;

		assert (render_state->state_count >= frame_index);
		frame_index = render_state->state_count;

		if (render_state->element_count > RENDER_MAX_ELEMENTS) {
			LOG ("RENDER WARNING: Render element count maximum exceeded (%d > %d)", render_state->element_count, RENDER_MAX_ELEMENTS);
			render_state->element_count = RENDER_MAX_ELEMENTS;
		}

		zen_timer_t frame_timer = zen_Start();

		// Draw background
		switch (render_state->background.type) {
			case background_type_none: break;

			case background_type_blank: {
				uint8_t *p = frame->p;
				int c = frame->w * frame->h;
				uint8_t b = render_state->background.blank.color;
				repeat (c) *p++ = b;
			} break;

			case background_type_stripes: {
				uint8_t colors[2] = {render_state->background.stripes.color, render_state->background.stripes.color/2};
				bool color_index = 0;
				uint8_t color = colors[color_index];
				uint8_t *p = frame->p;
				float angle = render_state->background.stripes.angle;
				angle -= (int)angle;
				angle = fabs (angle);
				if (angle == 0 || angle == 0.5f) {// Flat horizontal line
					int width = frame->w;
					// int height = frame->h;
					int h = 0;
					int w = render_state->background.stripes.width;
					// int y = 0;
					for (int y = 0; y < frame->h; ++y) {
						repeat (width) *p++ = color;
						if (++h == w) {
							color_index = !color_index;
							color = colors[color_index];
							h = 0;
						}
					}
				}
				else if (angle == 0.25f || angle == 0.75f) { // flat vertical line
					int width = frame->w;
					// int height = frame->h;
					int stripe_width = render_state->background.stripes.width;
					// int y = 0;
					for (int y = 0; y < frame->h; ++y) {
						int x = 0;
						int w = stripe_width;
						while (x < width) {
							if (x + w > width) w = width - x;
							repeat (w) {
								*p++ = color;
								++x;
							}
							color_index = !color_index;
							color = colors[color_index];
						}
					}
				}
				else {
					// This is fricked
					float slope = tanf (render_state->background.stripes.angle * TWOPI);
					float x_per_y = 1.f / slope;
					// float slopex = 0;
					int width = frame->w;
					// int height = frame->h;
					int stripe_width = render_state->background.stripes.width;
					// int y = 0;
					for (int y = 0; y < frame->h; ++y) {
						int x = y * x_per_y;
						int xpy = ROUNDF(x_per_y);
						if (xpy == 0) xpy = 1;
						color_index = (x / xpy) % 2;
						int w = stripe_width - (x % stripe_width);
						x = 0;
						while (x < width) {
							if (x + w > width) w = width - x;
							repeat (w) {
								*p++ = color;
								++x;
							}
							color_index = !color_index;
							color = colors[color_index];
							w = stripe_width;
						}
					}
				}
			} break;

			case background_type_checkers: {
				int height = frame->h;
				int width = frame->w;
				uint8_t *p = frame->p;
				int checker_width = render_state->background.checkers.width;
				int checker_height = render_state->background.checkers.height;
				assert (checker_height);
				uint8_t colors[2] = {render_state->background.checkers.color, render_state->background.checkers.color/2};
				bool color_index = 0;
				uint8_t color = colors[color_index];
				int y = 0;
				int offsetx = render_state->background.checkers.x;
				color_index = (offsetx / checker_width) % 2;
				offsetx %= checker_width;
				int offsety = render_state->background.checkers.y;
				color_index = (color_index + (offsety / checker_height)) % 2;
				offsety %= checker_height;
				int h = checker_height - offsety;
				while (y < height) {
					if (y + h > height) h = height - y;
					repeat (h) {
						int x = 0;
						int w = checker_width - offsetx;
						while (x < width) {
							color = colors[color_index];
							if (x + w > width) w = width - x;
							repeat (w) {
								*p++ = color;
								++x;
							}
							color_index = !color_index;
							color = colors[color_index];
							w = checker_width;
							if (x + w > width) w = width - x;
							repeat (w) {
								*p++ = color;
								++x;
							}
							color_index = !color_index;
							w = checker_width;
						}
						++y;
					}
					color_index = !color_index;
					h = checker_height;
				}
			} break;

			case background_type_sprite: {
				sprite_Blit (render_state->background.sprite, frame, 0, 0);
			} break;
		}

		auto count = render_state->element_count;

		for (int r = 1; r < count; ++r) {
			auto elementr = &render_state->elements[r];
			for (int l = r-1; l >= 0 && render_state->elements[l].depth > elementr->depth; --l) {
				SWAP (render_state->elements[l], *elementr);
				--elementr;
			}
		}

		auto camera = render_state->camera;
		auto element = render_state->elements;
		repeat (count) {
			switch (element->type) {
				case render_element_sprite: {
					auto s = element->sprite;
					if (!element->flags.ignore_camera) {
						s.position.x -= camera.x;
						s.position.y -= camera.y;
					}
					enum {flip_none = 0b00, flip_hori = 0b01, flip_vert = 0b10, flip_both = 0b11} flip = (s.flags.flip_vertically << 1) | s.flags.flip_horizontally;
					if ( flip != flip_none && s.flags.rotation_by_quarters != 0 ) {
						LOG ("Sprites cannot be both flipped and rotated!");
						assert (false);
						s.flags.rotation_by_quarters = 0;
						// s.rotation = 0;
					}
					if (s.rotation != 0) {
						sprite_SampleRotatedFlipped(s.sprite, frame, s.position.x, s.position.y, s.rotation, s.originx, s.originy, s.flags.flip_horizontally, s.flags.flip_vertically);
					}
					else {
						s.position.x -= s.originx;
						s.position.y -= s.originy;
						switch (flip) {
							case flip_none: {
								switch (s.flags.rotation_by_quarters) {
									case 0: sprite_Blit (s.sprite, frame, s.position.x, s.position.y); break;
									case 1: sprite_BlitRotated90 (s.sprite, frame, s.position.x, s.position.y); break;
									case 2: sprite_BlitRotated180 (s.sprite, frame, s.position.x, s.position.y); break;
									case 3: sprite_BlitRotated270 (s.sprite, frame, s.position.x, s.position.y); break;
								}
							} break;
							case flip_both: sprite_BlitRotated180 (s.sprite, frame, s.position.x, s.position.y); break;
							case flip_hori: sprite_BlitFlippedHorizontally (s.sprite, frame, s.position.x, s.position.y); break;
							case flip_vert: sprite_BlitFlippedVertically (s.sprite, frame, s.position.x, s.position.y); break;
						}
					}
				} break;

				case render_element_sprite_silhouette: {
					auto color = element->sprite_silhouette.color;
					auto s = element->sprite_silhouette.sprite;
					if (!element->flags.ignore_camera) {
						s.position.x -= camera.x;
						s.position.y -= camera.y;
					}
					enum {flip_none = 0b00, flip_hori = 0b01, flip_vert = 0b10, flip_both = 0b11} flip = (s.flags.flip_vertically << 1) | s.flags.flip_horizontally;
					if ( flip != flip_none && s.flags.rotation_by_quarters != 0 ) {
						LOG ("Sprites cannot be both flipped and rotated!");
						assert (false);
						s.flags.rotation_by_quarters = 0;
						// s.rotation = 0;
					}
					if (s.rotation != 0) {
						sprite_SampleRotatedFlipped(s.sprite, frame, s.position.x, s.position.y, s.rotation, s.originx, s.originy, s.flags.flip_horizontally, s.flags.flip_vertically);
					}
					else {
						s.position.x -= s.originx;
						s.position.y -= s.originy;
						switch (flip) {
							case flip_none: {
								switch (s.flags.rotation_by_quarters) {
									case 0: sprite_BlitColor (s.sprite, frame, s.position.x, s.position.y, color); break;
									case 1: sprite_BlitRotated90Color (s.sprite, frame, s.position.x, s.position.y, color); break;
									case 2: sprite_BlitRotated180Color (s.sprite, frame, s.position.x, s.position.y, color); break;
									case 3: sprite_BlitRotated270Color (s.sprite, frame, s.position.x, s.position.y, color); break;
								}
							} break;
							case flip_both: sprite_BlitRotated180Color (s.sprite, frame, s.position.x, s.position.y, color); break;
							case flip_hori: sprite_BlitFlippedHorizontallyColor (s.sprite, frame, s.position.x, s.position.y, color); break;
							case flip_vert: sprite_BlitFlippedVerticallyColor (s.sprite, frame, s.position.x, s.position.y, color); break;
						}
					}
				} break;

				case render_element_shape: {
					auto s = element->shape;
					switch (s.type) {
						case render_shape_rectangle: {
							auto r = s.rectangle;
							if (!element->flags.ignore_camera) {
								r.x -= camera.x;
								r.y -= camera.y;
							}
							int rb = r.y;
							int rt = rb + r.h-1;
							int rl = r.x;
							int rr = rl + r.w-1;

							if (rb > rt) SWAP (rb, rt);
							if (rl > rr) SWAP (rl, rr);

							if (r.flags.center_horizontally) {
								int half_width = r.w/2;
								rl -= half_width;
								rr -= half_width;
							}

							if (r.flags.center_vertically) {
								int half_height = r.h/2;
								rb -= half_height;
								rt -= half_height;
							}

							int bottom = MAX (rb, 0);
							int top = MIN (rt, frame->h-1);
							int left = MAX (rl, 0);
							int right = MIN (rr, frame->w-1);
							
							int ibottom = (bottom == rb) ? bottom+1 : bottom;
							int itop = (top == rt) ? top-1 : top;
							int ileft = (left == rl) ? left+1 : left;
							int iright = (right == rr) ? right-1 : right;

							if (top < bottom || right < left) break;

							if (bottom == rb && r.color_edge != 0)
								for (int x = left; x <= right; ++x)
									frame->p[x + bottom * frame->w] = r.color_edge;

							for (int y = ibottom; y <= itop; ++y) {
								if (left == rl && r.color_edge != 0)
									frame->p[left + y * frame->w] = r.color_edge;
								if (r.flags.filled && r.color_fill != 0) {
									for (int x = ileft; x <= iright; ++x) {
										frame->p[x + y * frame->w] = r.color_fill;
									}
								}
								if (right == rr && r.color_edge != 0)
									frame->p[right + y * frame->w] = r.color_edge;
							}

							if (top == rt && r.color_edge != 0)
								for (int x = left; x <= right; ++x)
									frame->p[x + top * frame->w] = r.color_edge;
						} break;

						case render_shape_circle: {
							auto c = s.circle;
							if (!element->flags.ignore_camera) {
								c.x -= camera.x;
								c.y -= camera.y;
							}
							if (c.filled && c.color_fill != 0) DrawCircleFilled (frame, c.x, c.y, c.r, c.color_fill);
							if ((!c.filled || c.color_edge != c.color_fill) && c.color_edge != 0) DrawCircle (frame, c.x, c.y, c.r, c.color_edge);
						} break;

						case render_shape_line: {
							auto l = s.line;
							if (!element->flags.ignore_camera) {
								l.x0 -= camera.x;
								l.x1 -= camera.x;
								l.y0 -= camera.y;
								l.y1 -= camera.y;
							}

							if (l.color == 0) break;

							if (l.x1 < l.x0) {
								SWAP(l.x0, l.x1);
								SWAP(l.y0, l.y1);
							}

							int bottom = l.y0;
							int top = l.y1;
							if (bottom > top) SWAP (bottom, top);

							if (l.x0 > frame->w-1 || l.x1 < 0 || bottom > frame->h-1 || top < 0) break; // line is completely outside the screen
		
							if (l.x0 < 0) {
								float width = l.x1 - l.x0;
								int height = l.y1 - l.y0;
								float distance = -l.x0 / width;
								l.y0 += height * distance;
								l.x0 = 0;
							}

							if (l.x1 > frame->w-1) {
								float width = l.x1 - l.x0;
								int height = l.y1 - l.y0;
								float distance = (frame->w-1 - l.x1) / width;
								l.y1 -= height * distance;
								l.x1 = frame->w-1;
							}

							if (l.y0 > l.y1) { SWAP (l.x0, l.x1); SWAP (l.y0, l.y1); }

							if (l.y0 < 0) {
								float width = l.x1 - l.x0;
								float height = l.y1 - l.y0;
								float distance = (float)-l.y0 / height;
								l.x0 += width * distance;
								l.y0 = 0;
							}

							if (l.y1 > frame->h-1) {
								float width = l.x1 - l.x0;
								float height = l.y1 - l.y0;
								float distance = (float)(frame->h-1 - l.y1) / height;
								l.x1 += width * distance;
								l.y1 = frame->h-1;
							}

							int dx = l.x1 - l.x0;
							int dy = l.y1 - l.y0;

							int ax = abs (dx) * 2;
							int sx = SIGN (dx);
							int ay = abs (dy) * 2;
							int sy = SIGN (dy);

							int x = l.x0;
							int y = l.y0;
							
							int d;
							if (ax > ay) {
								d = ay - (ax / 2);
								for (;;) {
									frame->p[x + y * frame->w] = l.color;
									if (x == l.x1) break;
									if (d >= 0) {
										y += sy;
										d -= ax;
									}
									x += sx;
									d += ay;
								}
							}
							else {
								d = ax - ay / 2;
								for (;;) {
									frame->p[x + y * frame->w] = l.color;
									if (y == l.y1) break;
									if (d >= 0) {
										x += sx;
										d -= ay;
									}
									y += sy;
									d += ax;
								}
							}
						} break;

						case render_shape_dot: {
							auto d = s.dot;
							if (!element->flags.ignore_camera) {
								d.x -= camera.x;
								d.y -= camera.y;
							}
							if (d.x < 0 || d.x > frame->w-1 || d.y < 0 || d.y > frame->h-1) break;
							frame->p[d.x + d.y * frame->w] = d.color;
						} break;
					}
				} break;

				case render_element_text: {
					auto text = element->text;
					if (!element->flags.ignore_camera) {
						text.x -= camera.x;
						text.y -= camera.y;
					}
					font_Write_Length (&resources_font, frame, text.x, text.y, text.string, text.length);
				} break;
			}
			++element;
		}

		// ************************************
		// Pixel particles
		// ************************************
		for (int i = 0; i < render_state->particles.count; ++i) {
			int x = render_state->particles.position[i].x;
			int y = render_state->particles.position[i].y;
			assert (x >= 0 && x < frame->w && y >= 0 && y < frame->h);
			// if (x < 0 || x >= frame->w || y < 0 || y >= frame->h) continue;
			frame->p[x + y * frame->w] = render_state->particles.pixel[i];
		}

		auto frame_time = zen_End (&frame_timer);

		if (render_state->debug.show_rendertime) {
			char str[32];
			snprintf (str, sizeof(str), "R%4"PRId64"us", frame_time);
			font_Write (&resources_font, frame, 1, frame->h-2-resources_font.line_height, str);
		}

		if (render_state->debug.show_framerate) {
			char str[32];
			snprintf (str,sizeof (str), "FPS%d", fps_this_frame);
			font_Write (&resources_font, frame, 1, frame->h-2, str);
		}

		if (render_state->cursor.sprite != NULL)
			sprite_Blit (render_state->cursor.sprite, frame, render_state->cursor.x, render_state->cursor.y);

		/**********************************************
		 * Present frame and wait for next screen refresh
		 **********************************************/
		// Update window
		os_DrawScreen ();
		
		asm volatile("" ::: "memory");
		render_state->busy = false;
		asm volatile("" ::: "memory");

		// Swap frame, clear new frame and perform stateless rendering
		frame_select = !frame_select;
		frame = (render_data.frame[frame_select]);

		// Sleep until next frame
		int64_t time_now = os_uTime ();
		sleep_time = (time_last + useconds_per_frame) - time_now;
		time_last += useconds_per_frame;
		if(time_now - time_last > useconds_per_frame) time_last = time_now;
		// LOG("Render sleep: %ldus", sleep_time);
		else os_uSleepPrecise (sleep_time);

	}

	LOG ("Render thread exiting normally");

	return NULL;
}

void DrawCircle(sprite_t *destination, int center_x, int center_y, float radius, uint8_t color) {
	float d;
	int x, y;
	d = 3.f - (2.f*radius);
	x = 0;
	y = radius;
	#pragma push_macro ("PXY")
	#undef PXY
	#define PXY(a, b) do { auto xx = (a); auto yy = (b); if (xx >= 0 && xx < destination->w && yy >= 0 && yy < destination->h) destination->p[xx + yy*destination->w] = color; } while (0)
	while(x <= y) {
		PXY ((center_x+x), (center_y+y));
		PXY ((center_x-x), (center_y+y));
		PXY ((center_x+x), (center_y-y));
		PXY ((center_x-x), (center_y-y));
		PXY ((center_x+y), (center_y+x));
		PXY ((center_x-y), (center_y+x));
		PXY ((center_x+y), (center_y-x));
		PXY ((center_x-y), (center_y-x));
		if(d < 0) { d += 4*x     +  6; ++x; }
		else      { d += 4*(x-y) + 10; ++x; --y; }
	}
	#pragma pop_macro ("PXY")
}

void DrawCircleFilled(sprite_t *destination, int center_x, int center_y, float radius, uint8_t color) {
	float d;
	int x, y;
	d = 3.f - (2.f*radius);
	x = 0;
	y = radius;
	while(x <= y) {
		for(int i = MAX(0, (center_x-x)); i <= MIN(destination->w-1, (center_x+x)); ++i) {
			if (center_y+y >= 0 && center_y+y < destination->h)
				destination->p[i + (center_y+y)*destination->w] = color;
			if (center_y-y >= 0 && center_y-y < destination->h)
				destination->p[i + (center_y-y)*destination->w] = color;
		}
		for(int i = MAX(0, (center_x-y)); i <= MIN(destination->w-1, (center_x+y)); ++i) {
			if (center_y+x >= 0 && center_y+x < destination->h)
				destination->p[i + (center_y+x)*destination->w] = color;
			if (center_y-x >= 0 && center_y-x < destination->h)
				destination->p[i + (center_y-x)*destination->w] = color;
		}
		if(d < 0) { d += 4*x     +  6; ++x; }
		else      { d += 4*(x-y) + 10; ++x; --y; }
	}
}

void font_Write_Length (const font_t *font, sprite_t *destination, int left, int top, char *text, const size_t length) {
    char c = *(text++);
    int i;
    int x = left, y = top - font->line_height; // Current x and y, updated as we draw each character
    while (c != '\0') {
        switch (c) {
            case ' ': x += font->space_width; break;

            case '\n': {
                x = left;
                y -= font->line_height;
            } break;

            default: {
                i = c - BITMAP_FONT_FIRST_VISIBLE_CHAR;
                if (i >= 0 && i < BITMAP_FONT_NUM_VISIBLE_CHARS) {
                    __label__ skip_drawing_letter;

                    int yy = y - font->descent[i];
                    int right = x + font->bitmaps[i]->w-1;
                    int top = yy + font->bitmaps[i]->h-1;

                    if (right < 0 || top < 0 || x > destination->w-1 || yy > destination->h-1) goto skip_drawing_letter;

					sprite_Blit (font->bitmaps[i], destination, x, yy);

                    skip_drawing_letter:
                    x += font->bitmaps[i]->w;
                } // If character wasn't in the range, then we ignore it.
            } break;
        }
        c = *(text++);
    }
}

void font_Write (const font_t *font, sprite_t *destination, int left, int top, char *text) {
	font_Write_Length(font, destination, left, top, text, strlen (text));
}

font_StringDimensions_return_t font_StringDimensions (const font_t *font, char *text) {
    char c = *(text++);
    int i;
    int x = 0, y = font->line_height; // Current x and y, updated as we draw each character
    bool string_is_visible = false;
    int width = 0;
    int line_descent = 0;
    while (c != '\0') {
        switch (c) {
            case ' ': x += font->space_width; break;

            case '\n': {
                x = 0;
                y += font->line_height;
                line_descent = 0;
            } break;

            default: {
                i = c - BITMAP_FONT_FIRST_VISIBLE_CHAR;
                if (i >= 0 && i < BITMAP_FONT_NUM_VISIBLE_CHARS) {
                    string_is_visible = true;
                    if (font->descent[i] > line_descent) line_descent = font->descent[i];
                    x += font->bitmaps[i]->w;
                    if (x > width) width = x;
                } // If character wasn't in the range, then we ignore it.
            } break;
        }
        c = *(text++);
    }

    y += line_descent;

    if (!string_is_visible) {
        return (font_StringDimensions_return_t){.width = 0, .height = 0};
    }
    return (font_StringDimensions_return_t){.width = width, .height = y};
}