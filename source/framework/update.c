#include "framework.h"

extern bool quit;
extern mouse_t mouse;
extern update_data_t update_data;
extern render_data_t render_data;

static_assert(true); // Solves clang getting confused about pragma push/pop. Related to preamble optimization

static uint64_t random_state;

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

		if (current_state != update_data.new_state) {
			folder_ReturnToBaseDirectory ();
			// os_ShowCursor ();
			// sound_MusicPause ();

			memset (update_data.frame.keyboard_state, 0, sizeof (update_data.frame.keyboard_state));
			memset (update_data.frame.mouse_state, 0, sizeof (update_data.frame.mouse_state));
			update_data.frame.mouse = (mouse_t){.x = update_data.source_mouse->x, .y = update_data.source_mouse->y};

			// update_state_e previous_state = current_state;
			current_state = update_data.new_state;
			state_functions[current_state].Initialize ();
		}

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
				++event;
			}
		}

		for (int i = 0; i < MOUSE_BUTTON_COUNT; ++i) {
			auto pk = &update_data.frame.mouse_state[i];
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
						update_data.frame.mouse_state[event->button.button] |= (event->button.pressed ? KEY_PRESSED : KEY_RELEASED);
					} break;
					case mouse_event_movement: {
						update_data.frame.mouse.x = event->movement.x;
						update_data.frame.mouse.y = event->movement.y;
					} break;
				}
				++event;
			}
		}
	
		static int64_t max_recorded_frame_time = 0;

		{	// Create a render state based on current game state
			// update_data.render_state = UpdateSelectRenderState (update_data);
			asm volatile("" ::: "memory");
			Render_SelectStateToEdit ();
			asm volatile("" ::: "memory");

			state_functions[current_state].Update ();

			char temp[64];
			if (current_state == update_state_gameplay) {
				switch (replay.mode) {
					case replay_mode_playback:
						sprintf (temp, "Frame %d", replay.frame);
						// Render_Text (.x = 0, .y = RESOLUTION_HEIGHT-11, .string = temp, .flags = {.ignore_camera = true});
						break;
					case replay_mode_ended:
						sprintf (temp, "Replay finished on frame %d\nF3 to continue", replay.frame);
						// Render_Text (.x = 0, .y = RESOLUTION_HEIGHT-11, .string = temp, .flags = {.ignore_camera = true});
						break;
					default: break;
				}
			}

			// if (submenu_vars.debug.show_simtime) {
			// 	if (current_state == update_state_gameplay) {
			// 		sprintf (temp, "S%4"PRId64"us", max_recorded_frame_time);
			// 		Render_Text (.x = 0, .y = RESOLUTION_HEIGHT-resources_font.line_height*2, .string = temp, .flags = {.ignore_camera = true});
			// 	}
			// }
			// if (submenu_vars.debug.show_rendertime) Render_ShowRenderTime (true);
			// if (submenu_vars.debug.show_framerate) Render_ShowFPS (true);

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
	x -= arguments.originx;
	y -= arguments.originy;
    auto w = sprite->w;
    auto h = sprite->h;
	switch (flip) {
		case CPFSFLIP_NONE: {
			for (int sy = 0; sy < h; ++sy) {
				for (int sx = 0; sx < w; ++sx) {
					uint8_t p = sprite->p[sx + sy * w];
					if (p != 0)
						ParticleAddAngleVelocity (arguments.color ? arguments.color : p, x + sx, y + sy, direction + DiscreteRandom_Rangef (&random_state, -0.01, 0.01), velocity + DiscreteRandom_Range (&random_state, -16384, 16384));
				}
			}
		} break;
		case CPFSFLIP_X: {
			x += w-1;
			for (int sy = 0; sy < h; ++sy) {
				for (int sx = 0; sx < w; ++sx) {
					uint8_t p = sprite->p[sx + sy * w];
					if (p != 0)
						ParticleAddAngleVelocity (arguments.color ? arguments.color : p, x - sx, y + sy, direction + DiscreteRandom_Rangef (&random_state, -0.01, 0.01), velocity + DiscreteRandom_Range (&random_state, -16384, 16384));
				}
			}
		} break;
		case CPFSFLIP_Y: {
			y += h-1;
			for (int sy = 0; sy < h; ++sy) {
				for (int sx = 0; sx < w; ++sx) {
					uint8_t p = sprite->p[sx + sy * w];
					if (p != 0)
						ParticleAddAngleVelocity (arguments.color ? arguments.color : p, x + sx, y - sy, direction + DiscreteRandom_Rangef (&random_state, -0.01, 0.01), velocity + DiscreteRandom_Range (&random_state, -16384, 16384));
				}
			}
		} break;
		case CPFSFLIP_BOTH: {
			x += w-1;
			y += h-1;
			for (int sy = 0; sy < h; ++sy) {
				for (int sx = 0; sx < w; ++sx) {
					uint8_t p = sprite->p[sx + sy * w];
					if (p != 0)
						ParticleAddAngleVelocity (arguments.color ? arguments.color : p, x - sx, y - sy, direction + DiscreteRandom_Rangef (&random_state, -0.01, 0.01), velocity + DiscreteRandom_Range (&random_state, -16384, 16384));
				}
			}
		} break;
	}
}