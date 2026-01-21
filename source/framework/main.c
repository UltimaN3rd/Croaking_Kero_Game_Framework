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
#include <stdatomic.h>
#include <pthread.h>

void *Update(void*);

void game_ToggleFullscreen ();

update_data_t update_data = {};
render_data_t render_data = {};
pthread_t thread_render = 0, thread_update = 0, thread_sound = 0;
bool quit = false;
pthread_mutex_t update_render_swap_state_mutex = (pthread_mutex_t){};

sprite_t render_data_frame_0 = {.p = {[RESOLUTION_WIDTH*RESOLUTION_HEIGHT-1] = 0}}, render_data_frame_1 = {.p = {[RESOLUTION_WIDTH*RESOLUTION_HEIGHT-1] = 0}};

#ifndef NDEBUG
typedef struct __attribute((__packed__)) {
	struct __attribute((__packed__)) {
		char magic[2];
		uint32_t file_size;
		uint32_t filler;
		uint32_t image_data_address;
	} file;
	struct __attribute((__packed__)) {
		uint32_t header_size;
		int32_t width;
		int32_t height;
		uint16_t color_planes;
		uint16_t bits_per_pixel;
		uint32_t compression;
		uint32_t compressed_size;
		uint32_t pixels_per_m_horizontal;
		uint32_t pixels_per_m_vertical;
		uint32_t colors_used;
		uint32_t important_colors;
	} info;
} bmp_header_t;
static_assert (sizeof (bmp_header_t) == 54);

static struct __attribute((__packed__)) {
	bmp_header_t header;
	char palette[4*256];
} bmp_static_data = {
	.header = {
		.file = {
			.magic = "BM",
			.file_size = sizeof (bmp_static_data) + RESOLUTION_WIDTH * RESOLUTION_HEIGHT,
			.image_data_address = sizeof (bmp_static_data),
		},
		.info = {
			.header_size = sizeof (bmp_static_data.header.info),
			.width = RESOLUTION_WIDTH,
			.height = RESOLUTION_HEIGHT,
			.color_planes = 1,
			.bits_per_pixel = 8,
			.compression = 0,
			.compressed_size = 0,
			.pixels_per_m_horizontal = 300,
			.pixels_per_m_vertical = 300,
			.colors_used = 256,
			.important_colors = 256,
		},
	},
};
static_assert (sizeof (bmp_static_data) == 54+256*4);

void OutputScreenshot ();
#endif

int main (int argc, char **argv) {
	update_data.keyboard_events.count = 0;

	render_data.frame[0] = &render_data_frame_0;
	render_data.frame[1] = &render_data_frame_1;
	render_data.frame[0]->w = render_data.frame[1]->w = RESOLUTION_WIDTH;
	render_data.frame[0]->h = render_data.frame[1]->h = RESOLUTION_HEIGHT;

	if (!os_Init (GAME_TITLE)) { LOG ("os_Init failed."); abort (); }
	os_SetBackgroundColor (0x40, 0x3a, 0x4d);
	log_Time ();
	LOG (" Program started");
	os_SetWindowFrameBuffer (render_data.frame[0]->p, render_data.frame[0]->w, render_data.frame[0]->h);
	// os_WindowSize (1920, 1080);
	os_HideCursor ();
	zen_Init();
	folder_SetCurrentFolderAsBaseDirectory ();

	if (pthread_mutex_init (&update_render_swap_state_mutex, NULL) != 0) { LOG ("Failed to initialize update_render_swap_state_mutex"); abort (); }
	if (pthread_create (&thread_sound, NULL, Sound, NULL)) { LOG ("Failed to create sound thread."); abort (); }
	if (pthread_create (&thread_update, NULL, Update, NULL)) { LOG ("Failed to create update thread."); abort (); }
	if (pthread_create (&thread_render, NULL, Render, NULL)) { LOG ("Failed to create render thread."); abort (); }

	#ifndef NDEBUG
	for (int i = 0; i < 256; ++i) {
		bmp_static_data.palette[i*4] = palette[i][2];
		bmp_static_data.palette[i*4+1] = palette[i][1];
		bmp_static_data.palette[i*4+2] = palette[i][0];
		bmp_static_data.palette[i*4+3] = 0;
	}
	#endif

	LOG ("Main thread entering main event loop");

	os_event_t event;
	while (!quit) {
		event = os_NextEvent ();
		switch (event.type) {
			case os_EVENT_NULL: os_uSleepEfficient(2000); break;

			case os_EVENT_QUIT: {
				quit = true;
				exit (event.exit_code);
			} break;

			case os_EVENT_KEY_PRESS: {
				__label__ skip_sending_key_event;
				switch (event.key) {
					case os_KEY_ENTER: {
						if (os_public.keyboard[os_KEY_LALT] || os_public.keyboard[os_KEY_RALT]) {
							game_ToggleFullscreen ();
							goto skip_sending_key_event;
						}
					} break;
					
					case os_KEY_F4: {
						if (os_public.keyboard[os_KEY_LALT] || os_public.keyboard[os_KEY_RALT]) {
							quit = true;
							os_SendQuitEvent ();
							goto skip_sending_key_event;
						}
					} break;

					#ifndef NDEBUG
					case os_KEY_P: {
						OutputScreenshot();
					} break;
					#endif

					default: break;
				}

				int index = update_data.keyboard_events.count % KEYBOARD_EVENT_MAX;
				update_data.keyboard_events.events[index].key = event.key;
				update_data.keyboard_events.events[index].pressed = true;
				++update_data.keyboard_events.count;

skip_sending_key_event:
			} break;

			case os_EVENT_KEY_RELEASE: {
				int index = update_data.keyboard_events.count % KEYBOARD_EVENT_MAX;
				update_data.keyboard_events.events[index].key = event.key;
				update_data.keyboard_events.events[index].pressed = false;
				++update_data.keyboard_events.count;
			} break;

			case os_EVENT_MOUSE_MOVE: {
				auto ret = os_WindowPositionToScaledFrameBufferPosition (event.new_position.x, event.new_position.y);

				int i = update_data.mouse_events.count % MOUSE_EVENT_MAX;
				update_data.mouse_events.events[i].type = mouse_event_movement;
				update_data.mouse_events.events[i].movement.x = ret.x;
				update_data.mouse_events.events[i].movement.y = ret.y;
				++update_data.mouse_events.count;
			} break;

			case os_EVENT_MOUSE_BUTTON_PRESS: {
				int i = update_data.mouse_events.count % MOUSE_EVENT_MAX;
				update_data.mouse_events.events[i].type = mouse_event_button;
				update_data.mouse_events.events[i].button.button = ConvertMouseButton (event.button.button);
				update_data.mouse_events.events[i].button.pressed = true;
				++update_data.mouse_events.count;
			} break;

			case os_EVENT_MOUSE_BUTTON_RELEASE: {
				int i = update_data.mouse_events.count % MOUSE_EVENT_MAX;
				update_data.mouse_events.events[i].type = mouse_event_button;
				update_data.mouse_events.events[i].button.button = ConvertMouseButton (event.button.button);
				update_data.mouse_events.events[i].button.pressed = false;
				++update_data.mouse_events.count;
			} break;

			case os_EVENT_MOUSE_SCROLL: {
				int i = update_data.mouse_events.count % MOUSE_EVENT_MAX;
				update_data.mouse_events.events[i] = (typeof(update_data.mouse_events.events[i])){
					.type = mouse_event_scroll,
					.scroll.up = event.scrolled_up,
				};
				++update_data.mouse_events.count;
			} break;

			default: break;
		}
	}
	sound_extern_data.quit = true;
	LOG ("Main thread event loop exited. Performing OS cleanup and awaiting other threads' completion before exiting");

	os_Cleanup ();
	
	// If we exit the main thread before closing the child threads, we'll access main's variables from inside those threads after we've released it. So better to wait for them to quit properly before exiting.
	pthread_join (thread_sound, 0);
	pthread_join (thread_update, 0);
	pthread_join (thread_render, 0);

	LOG ("Closing gracefully");

	return 0;
}

#ifndef NDEBUG
void OutputScreenshot () {
	auto t = time(0);
	auto lt = localtime (&t);
	char buf[128];
	// Create new filename based on time, then add number if file already exists
	for (int i = 0; true; ++i) {
		snprintf (buf, sizeof(buf), "%s/" GAME_TITLE "/%04d-%02d-%02d-%02d-%02d-%02d-%02d.bmp",
		#if WIN32
		"C:/Users/Nick/Pictures",
		#elif __linux__
		"/home/nick/Pictures",
		#elif __APPLE__
		"/Users/nick/Pictures", // This won't work since the app is sandboxed.
		#endif
		lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec, i);
		FILE *test = fopen (buf, "rb");
		if (!test) {
			LOG ("Failed to open file [%s]", buf);
			break;
		}
		fclose (test);
	}

	FILE *phil = fopen (buf, "wb");
	if (!phil) return;

	render_data.resume_thread = false;
	render_data.pause_thread = true;
	while (render_data.pause_thread) os_uSleepEfficient(1000);

	fwrite (&bmp_static_data, sizeof (bmp_static_data), 1, phil);
	fwrite (render_data.frame[0]->p, RESOLUTION_WIDTH*RESOLUTION_HEIGHT, 1, phil);
	fclose (phil);
	render_data.resume_thread = true;
}
#endif