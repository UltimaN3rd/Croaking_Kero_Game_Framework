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

#include "framework.h"

extern bool quit;
extern update_data_t update_data;
extern render_data_t render_data;

static uint64_t random_state;

void FloatyTextUpdate ();
void FloatyTextDraw ();

#ifndef PARTICLES_BOUNDARY_LEFT
#define PARTICLES_BOUNDARY_LEFT 0
#endif
#ifndef PARTICLES_BOUNDARY_RIGHT
#define PARTICLES_BOUNDARY_RIGHT (RESOLUTION_WIDTH-1)
#endif
#ifndef PARTICLES_BOUNDARY_BOTTOM
#define PARTICLES_BOUNDARY_BOTTOM -20
#endif
#ifndef PARTICLES_BOUNDARY_TOP
#define PARTICLES_BOUNDARY_TOP (RESOLUTION_HEIGHT + 100)
#endif

static void TypeThisFrame (os_key_e key) {
	char c = update_data.frame.keyboard_state[os_KEY_SHIFT] & (KEY_PRESSED | KEY_HELD) ? os_key_shifted(key) : key;

	if (c < ' ' || c > '~') return;
	assert (update_data.frame.typing.count < sizeof (update_data.frame.typing.chars)-1); if (update_data.frame.typing.count >= sizeof (update_data.frame.typing.chars)-1) { LOG ("Exceeded max typing chars in 1 frame"); return;}

	update_data.frame.typing.chars[update_data.frame.typing.count++] = c;
	update_data.frame.typing.chars[update_data.frame.typing.count] = 0; // Always append NULL terminator
}

static update_state_e current_state = 0;
static void Update_ObjectClearTopUnusedMemory ();
static void Update_ObjectDelete (uint16_t index);
static void Update_ObjectSort (uint16_t starting_index);
static bool object_created_or_destroyed_this_frame = false;
static struct {
	update_state_e new_state;
	bool happened;
	char data[UPDATE_CHANGE_STATE_DATA_SIZE_MAX];
	void (*FuncRunAfterStateChange) ();
} state_change;

static typeof(update_data.frame) unedited_frameinput = {}; // Save input state in case game modifies it

typedef struct __attribute__((__packed__)) {
	uint32_t id;
	uint32_t memory_offset;
	Update_Object_Func_t UpdateAndRender;
	int8_t layer; // Objects are ordered by layer - higher layer means processed first. Objects in higher layers may occlude input from objects in lower layers.
	bool survive_state_change : 1;
} update_object_t;

// Placed at the base address of every object's memory
typedef struct {
	bool used : 1;
	uint32_t size : 31;
} update_object_header_t;

void *Update(void*) {
	LOG ("Update thread started");
	while (!render_data.thread_initialized) os_uSleepEfficient (1000);

	int64_t time_last, time_now;
	int64_t us_per_frame;
	us_per_frame = 1000000 / 120; // Update rate = 120Hz
	time_last = os_uTime ();

	zen_timer_t frame_timer;

	update_state_e current_state = -1;
	update_data.new_state = 0;

	// uint32_t keyboard_event_count = 0;
	// uint32_t mouse_event_count = 0;

	struct {
		FILE *file;
		int frame;
		enum { replay_mode_recording, replay_mode_playback, replay_mode_ended } mode;
		typeof (update_data.keyboard_events) keyboard_events;
		typeof (update_data.mouse_events) mouse_events;
	} replay = {.mode = replay_mode_ended};
	assert (KEYBOARD_EVENT_MAX == 256);
	assert (MOUSE_EVENT_MAX == 256);

	// These are for basic input while using the replay system. The game gets input from the replay, but I can control the replay using these
	typeof (os_public.keyboard) keyboard = {}, keyboard_previous = {};
	typeof (os_public.mouse) mouse = {}, mouse_previous = {};
	#define KEYBOARD_REPEAT_INITIAL_DELAY 30
	#define KEYBOARD_REPEAT_DELAY 10
	uint8_t keyboard_repeat_time[256] = {};

	LOG ("Update thread entering main loop");

	while(!quit) { 
		memcpy (keyboard_previous, keyboard, sizeof (keyboard));
		memcpy (&mouse_previous, &mouse, sizeof (mouse));
		memcpy (keyboard, os_public.keyboard, sizeof (keyboard));
		memcpy (&mouse, &os_public.mouse, sizeof (mouse));
		update_data.frame.typing = (typeof(update_data.frame.typing)){};
update_data.frame.mouse.scroll = 0;

		frame_timer = zen_Start ();

		for (int i = 0; i < 256; ++i) {
			auto pk = &update_data.frame.keyboard_state[i];
			*pk &= ~KEY_REPEATED; // Always clear repeated flag at start of frame
			if (*pk & KEY_RELEASED) {
				*pk = KEY_NORMAL;
				keyboard_repeat_time[i] = 0;
			}
			else if (*pk & KEY_PRESSED) {
				*pk = KEY_HELD;
				keyboard_repeat_time[i] = 1;
			}
			else if (*pk & KEY_HELD) {
				++keyboard_repeat_time[i];
				if (keyboard_repeat_time[i] > KEYBOARD_REPEAT_INITIAL_DELAY) {
					*pk |= KEY_REPEATED;
					keyboard_repeat_time[i] = KEYBOARD_REPEAT_INITIAL_DELAY - KEYBOARD_REPEAT_DELAY;
					TypeThisFrame(*pk);
				}
			}
		}

		int keyboard_events_this_frame;
		typeof (update_data.keyboard_events) keyboard_events_cache;
		keyboard_events_cache = update_data.keyboard_events;
		int keyboard_events_this_frame_unrestricted = keyboard_events_cache.count - replay.keyboard_events.count;
		keyboard_events_this_frame = MIN (KEYBOARD_EVENT_MAX, keyboard_events_this_frame_unrestricted);
		if (keyboard_events_this_frame_unrestricted > KEYBOARD_EVENT_MAX)
			LOG ("Looks like we dropped %d keyboard events!", keyboard_events_this_frame_unrestricted - KEYBOARD_EVENT_MAX);
		if (keyboard_events_this_frame != 0) {
			int index = replay.keyboard_events.count % KEYBOARD_EVENT_MAX;
			int ending_index = (index + keyboard_events_this_frame-1) % KEYBOARD_EVENT_MAX;
			int events1 = keyboard_events_this_frame;
			int events2 = 0;
			if (ending_index < index) {
				events1 = KEYBOARD_EVENT_MAX - index;
				events2 = keyboard_events_this_frame - events1;
			}
			memcpy (&replay.keyboard_events.events[0], &keyboard_events_cache.events[index], events1 * sizeof (replay.keyboard_events.events[0]));
			memcpy (&replay.keyboard_events.events[events1], &keyboard_events_cache.events[0], events2 * sizeof (replay.keyboard_events.events[0]));
			replay.keyboard_events.count = keyboard_events_cache.count;
		}

		{
			auto event = replay.keyboard_events.events;
			repeat (keyboard_events_this_frame) {
				update_data.frame.keyboard_state[event->key] |= (event->pressed ? KEY_PRESSED : KEY_RELEASED);
				if (event->pressed == KEY_PRESSED) {
					TypeThisFrame (event->key);
				}
				++event;
			}
		}

		for (int i = 0; i < MOUSE_BUTTON_COUNT; ++i) {
			auto pk = &update_data.frame.mouse.buttons[i];
			auto k = *pk;
			if (k & KEY_RELEASED) *pk = KEY_NORMAL;
			else if (k & KEY_PRESSED) *pk = KEY_HELD;
		}

		int mouse_events_this_frame;
		typeof (update_data.mouse_events) mouse_events_cache;
		mouse_events_cache = update_data.mouse_events;
		int mouse_events_this_frame_unrestricted = mouse_events_cache.count - replay.mouse_events.count;
		mouse_events_this_frame = MIN (MOUSE_EVENT_MAX, mouse_events_this_frame_unrestricted);
		if (mouse_events_this_frame_unrestricted > MOUSE_EVENT_MAX)
			LOG ("Looks like we dropped %d mouse events!", mouse_events_this_frame_unrestricted - (MOUSE_EVENT_MAX));
		if (mouse_events_this_frame != 0) {
			int index = replay.mouse_events.count % MOUSE_EVENT_MAX;
			int ending_index = (index + mouse_events_this_frame-1) % MOUSE_EVENT_MAX;
			int events1 = mouse_events_this_frame;
			int events2 = 0;
			if (ending_index < index) {
				events1 = MOUSE_EVENT_MAX - index;
				events2 = mouse_events_this_frame - events1;
			}
			memcpy (&replay.mouse_events.events[0], &mouse_events_cache.events[index], events1 * sizeof (replay.mouse_events.events[0]));
			memcpy (&replay.mouse_events.events[events1], &mouse_events_cache.events[0], events2 * sizeof (replay.mouse_events.events[0]));
			replay.mouse_events.count = mouse_events_cache.count;
		}

		{
			auto event = replay.mouse_events.events;
			repeat (mouse_events_this_frame) {
				switch (event->type) {
					case mouse_event_button: {
						update_data.frame.mouse.buttons[event->button.button] |= (event->button.pressed ? KEY_PRESSED : KEY_RELEASED);
					} break;
					case mouse_event_movement: {
						update_data.frame.mouse.x = event->movement.x;
						update_data.frame.mouse.y = event->movement.y;
					} break;
					case mouse_event_scroll: {
						update_data.frame.mouse.scroll = event->scroll.up ? 1 : -1;
					} break;
				}
				++event;
			}
		}
	
		static int64_t max_recorded_frame_time = 0;

		unedited_frameinput = update_data.frame; // Save input state in case game modifies it
		object_created_or_destroyed_this_frame = false;

		{	// Create a render state based on current game state
			// update_data.render_state = UpdateSelectRenderState (update_data);
			asm volatile("" ::: "memory");
			Render_SelectStateToEdit ();
			asm volatile("" ::: "memory");

			for (int16_t i = update_data.objects.count-1; i >= 0; --i) {
				const auto obj = &((update_object_t*)update_data.objects.mem)[i];
				const auto objdata = update_data.objects.mem + obj->memory_offset;
				if (!obj->UpdateAndRender (objdata))
					Update_ObjectDelete (i);
			}

			state_functions[current_state].Update ();

			ParticlesUpdate(PARTICLES_BOUNDARY_LEFT, PARTICLES_BOUNDARY_RIGHT, PARTICLES_BOUNDARY_BOTTOM, PARTICLES_BOUNDARY_TOP);
			for (int i = 0; i < update_data.gameplay.particles.count; ++i) {
				Render_Particle (update_data.gameplay.particles.position[i].x.high, update_data.gameplay.particles.position[i].y.high, update_data.gameplay.particles.pixel[i], false);
			}
			FloatyTextUpdate();
			FloatyTextDraw();


			if (update_data.debug.show_simtime && *update_data.debug.show_simtime) {
				char temp[64];
					sprintf (temp, "S%4"PRId64"us", max_recorded_frame_time);
					Render_Text (.x = 0, .y = RESOLUTION_HEIGHT-framework_font.line_height*2, .string = temp, .flags = {.ignore_camera = true});
			}
			if (update_data.debug.show_rendertime && *update_data.debug.show_rendertime) Render_ShowRenderTime (true);
			if (update_data.debug.show_framerate && *update_data.debug.show_framerate) Render_ShowFPS (true);

			asm volatile("" ::: "memory");
			Render_FinishEditingState ();
			asm volatile("" ::: "memory");
		}

		int64_t frame_time = zen_End (&frame_timer);

		// Sleep until next frame
		time_now = os_uTime ();
		int64_t sleep_time;
		sleep_time = time_last + us_per_frame - time_now;

		time_last += us_per_frame;
		if(time_now - time_last > us_per_frame) time_last = time_now;
		// LOG("Update sleep: %lldus", sleep_time);
		os_uSleepPrecise (sleep_time);

		static int frames_before_reset = 60;
		if (--frames_before_reset == 0) {
			frames_before_reset = 120;
			max_recorded_frame_time = 0;
		}
		if (frame_time > max_recorded_frame_time) {
			frames_before_reset = 120;
			max_recorded_frame_time = frame_time;
		}
	}

	LOG ("Update thread exiting normally");
	
	return NULL;
} // Update ()

#define PARTICLE_GRAVITY -2048

void ParticleAdd_ (uint8_t pixel, int x, int y, int32_t vx, int32_t vy, ParticleAdd_arguments arguments) {
	auto udata = &update_data.gameplay;

	int i = udata->particles.count++;
	if (udata->particles.count >= PARTICLES_MAX) {
		udata->particles.count = PARTICLES_MAX;
		i = 0;
	}
	udata->particles.pixel[i] = pixel;
	udata->particles.position[i].x.high = x;
	udata->particles.position[i].y.high = y;
	udata->particles.position[i].x.low = 0;
	udata->particles.position[i].y.low = 0;
	udata->particles.velocity[i].x = vx;
	udata->particles.velocity[i].y = vy;
	udata->particles.gravity[i] = arguments.gravity;
	udata->particles.time[i] = arguments.time;
}

void ParticlesUpdate (int left, int right, int bottom, int top) {
	auto udata = &update_data.gameplay;

	for (int i = 0; i < udata->particles.count; ++i) {
		if (udata->particles.gravity[i])
			udata->particles.velocity[i].y += PARTICLE_GRAVITY;
		if (udata->particles.time[i]) {
			if (--udata->particles.time[i] == 0) {
				ParticleDelete (i--);
				continue;
			}
		}
		udata->particles.position[i].x.i32 += udata->particles.velocity[i].x;
		udata->particles.position[i].y.i32 += udata->particles.velocity[i].y;
		int x = udata->particles.position[i].x.high;
		int y = udata->particles.position[i].y.high;
		if (x < left || x > right || y < bottom || y > top) {
			ParticleDelete (i--);
		}
	}
}

void ParticleDelete (int index) {
	auto udata = &update_data.gameplay;

	if (udata->particles.count == 0) return;
	--udata->particles.count;
	for (int i = index; i < udata->particles.count; ++i) {
		udata->particles.position[i] = udata->particles.position[i+1];
		udata->particles.velocity[i] = udata->particles.velocity[i+1];
		udata->particles.pixel[i] = udata->particles.pixel[i+1];
		udata->particles.gravity[i] = udata->particles.gravity[i+1];
		udata->particles.time[i] = udata->particles.time[i+1];
	}
}

void CreateParticlesFromSprite_ (const sprite_t *sprite, int x, int y, float direction, int32_t velocity, CreateParticlesFromSprite_arguments arguments) {
	enum {CPFSFLIP_NONE, CPFSFLIP_Y, CPFSFLIP_X, CPFSFLIP_BOTH} flip = (arguments.flipx ? 2 : 0) | (arguments.flipy ? 1 : 0);
    auto w = sprite->w;
    auto h = sprite->h;
	float c = cos_turns (arguments.rotation);
	float s = sin_turns (arguments.rotation);
	switch (flip) {
		case CPFSFLIP_NONE: {
			for (int sy = 0; sy < h; ++sy) {
				for (int sx = 0; sx < w; ++sx) {
					uint8_t p = sprite->p[sx + sy * w];
					int tx = sx - arguments.originx;
					int ty = sy - arguments.originy;
					int rx = c*tx - s*ty;
					int ry = s*tx + c*ty;
					if (p != 0)
						ParticleAddAngleVelocity (arguments.color ? arguments.color : p, x + rx, y + ry, direction + DiscreteRandom_Rangef (&random_state, -0.01, 0.01), velocity + DiscreteRandom_Range (&random_state, -16384, 16384));
				}
			}
		} break;
		case CPFSFLIP_X: {
			// x += w-1;
			for (int sy = 0; sy < h; ++sy) {
				for (int sx = 0; sx < w; ++sx) {
					uint8_t p = sprite->p[sx + sy * w];
					int tx = sx - arguments.originx;
					int ty = sy - arguments.originy;
					int rx = c*tx - s*ty;
					int ry = s*tx + c*ty;
					if (p != 0)
						ParticleAddAngleVelocity (arguments.color ? arguments.color : p, x - rx, y + ry, direction + DiscreteRandom_Rangef (&random_state, -0.01, 0.01), velocity + DiscreteRandom_Range (&random_state, -16384, 16384));
				}
			}
		} break;
		case CPFSFLIP_Y: {
			// y += h-1;
			for (int sy = 0; sy < h; ++sy) {
				for (int sx = 0; sx < w; ++sx) {
					uint8_t p = sprite->p[sx + sy * w];
					int tx = sx - arguments.originx;
					int ty = sy - arguments.originy;
					int rx = c*tx - s*ty;
					int ry = s*tx + c*ty;
					if (p != 0)
						ParticleAddAngleVelocity (arguments.color ? arguments.color : p, x + rx, y - ry, direction + DiscreteRandom_Rangef (&random_state, -0.01, 0.01), velocity + DiscreteRandom_Range (&random_state, -16384, 16384));
				}
			}
		} break;
		case CPFSFLIP_BOTH: {
			// x += w-1;
			// y += h-1;
			for (int sy = 0; sy < h; ++sy) {
				for (int sx = 0; sx < w; ++sx) {
					uint8_t p = sprite->p[sx + sy * w];
					int tx = sx - arguments.originx;
					int ty = sy - arguments.originy;
					int rx = c*tx - s*ty;
					int ry = s*tx + c*ty;
					if (p != 0)
						ParticleAddAngleVelocity (arguments.color ? arguments.color : p, x - rx, y - ry, direction + DiscreteRandom_Rangef (&random_state, -0.01, 0.01), velocity + DiscreteRandom_Range (&random_state, -16384, 16384));
				}
			}
		} break;
	}
}

void Update_ChangeState (update_state_e new_state) {
	assert (new_state >= 0 && new_state < update_state_count); if (new_state < 0 || new_state >= update_state_count) { LOG ("Update new state %d out of bounds", new_state); new_state = 0; }
	update_data.new_state = new_state;
}

void FloatyTextCreate (int x, int y, int vy, int time, const char *const str) {
	assert (update_data.floaty_text.count <= FLOATY_TEXT_MAX);
	if (update_data.floaty_text.count == FLOATY_TEXT_MAX) FloatyTextDelete (0);
	assert (update_data.floaty_text.count < FLOATY_TEXT_MAX);

	unsigned int i = update_data.floaty_text.count++;
	update_data.floaty_text.text[i].x = x;
	update_data.floaty_text.text[i].y = (int32split_t){.high = y};
	update_data.floaty_text.text[i].vy = vy;
	update_data.floaty_text.text[i].time = time;
	snprintf (update_data.floaty_text.text[i].string, sizeof(update_data.floaty_text.text[i].string), str);
}

// Sort starting from index, continuing up. If 0 or 1, sort all objects.
static void Update_ObjectSort (uint16_t starting_index) {
	if (update_data.objects.count == 0) return;
	assert (starting_index < update_data.objects.count);
	auto index = starting_index;
	if (index == 0) index = 1;
	while (index < update_data.objects.count) {
		auto obj = &((update_object_t*)update_data.objects.mem)[index];
		auto objbelow = &((update_object_t*)update_data.objects.mem)[index-1];
		for (auto i = index; i > 0; --i, --obj, --objbelow) {
			if (objbelow->layer >= obj->layer) break;
			SWAP (*obj, *objbelow);
		}
		++index;
	}

	object_created_or_destroyed_this_frame = true;
}

uint32_t Update_ObjectCreate_ (const void *const data, const size_t data_size, const Update_Object_Func_t UpdateAndRenderFunc, const int8_t layer, const bool survive_state_change) {
	uint16_t object_size = sizeof (update_object_header_t) + data_size;
	// object_size += object_size % 8; // Align by 8 bytes
	const auto new_bottom = update_data.objects.bottom_used + sizeof (update_object_t);
	const auto new_top = update_data.objects.top_used + object_size;
	if (new_bottom + new_top >= UPDATE_OBJECT_MEMORY_SIZE) return 0; // Out of memory
	const auto id = update_data.objects.nextid++;
	auto obj = &((update_object_t*)update_data.objects.mem)[update_data.objects.count++];
	*obj = (update_object_t) {
		.id = id,
		.memory_offset = UPDATE_OBJECT_MEMORY_SIZE - new_top + sizeof(update_object_header_t),
		.UpdateAndRender = UpdateAndRenderFunc,
		.layer = layer,
		.survive_state_change = survive_state_change,
	};
	memcpy (&update_data.objects.mem[UPDATE_OBJECT_MEMORY_SIZE - new_top], &(update_object_header_t){.used = 1, .size = object_size}, sizeof (update_object_header_t));
	memcpy (&update_data.objects.mem[obj->memory_offset], data, data_size);
	update_data.objects.bottom_used = new_bottom;
	update_data.objects.top_used = new_top;

	object_created_or_destroyed_this_frame = true;

	return id;
}

void FloatyTextDraw () {
	for (int i = 0; i < update_data.floaty_text.count; ++i) {
		auto t = update_data.floaty_text.text[i];
		Render_Text (.x = t.x, .y = t.y.high, .string = t.string);
	}
}