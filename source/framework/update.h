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

#pragma once

#include "framework.h"

typedef enum {update_input_event_keyboard, update_input_event_mouse_button, update_input_event_mouse_movement, update_input_event_mouse_scroll} update_input_event_type_e;
typedef struct __attribute__((__packed__)) {
	update_input_event_type_e tag : 8; // Specified 8 bits just so no bits are assumed
	union {
		struct __attribute__((__packed__)) {
			os_key_e key : 7;
			bool pressed : 1;
		} keyboard;
		struct __attribute__((__packed__)) {
			mouse_button_e button : 7;
			bool pressed : 1;
		} mouse_button;
		struct __attribute__((__packed__)) {
			// Cast the next 2 events are int16_t x, y. This is because other event types are only 2 bytes, but this event type would swell the size of all events to 5 bytes
		} movement;
		struct __attribute__((__packed__)) {
			bool up : 1;
		} scroll;
	} _;
} update_input_event_t;

// For multi-events like scroll, how many extra event slots can they take up)
#define UPDATE_INPUT_EVENT_MAX_EXTRA_EVENTS 2

typedef struct update_data_s { // update_data_t
	#define KEY_NORMAL 0
	#define KEY_PRESSED 0b1
	#define KEY_HELD 0b10
	#define KEY_RELEASED 0b100
	#define KEY_REPEATED 0b1000
	struct {
		#define UPDATE_KEYBOARD_KEY_COUNT 128
		uint8_t keyboard[UPDATE_KEYBOARD_KEY_COUNT];
		struct {
			uint8_t count;
			char chars[33];
		} typing;
		struct {
			int x, y;
			int8_t scroll;
			uint8_t buttons[MOUSE_BUTTON_COUNT];
		} mouse;
	} frame;
	char text_to_print[64];
	struct {
		struct {
			vec2i32split_t position[PARTICLES_MAX];
			uint8_t pixel[PARTICLES_MAX];
			vec2i_t velocity[PARTICLES_MAX];
			int count;
			bool gravity[PARTICLES_MAX];
			int time[PARTICLES_MAX];
		} particles;
	} gameplay;
	#define UPDATE_EVENTS_MAX 256
	struct {
		uint32_t count;
		update_input_event_t _[UPDATE_EVENTS_MAX];
	} events;
	char debug_frame_time_string[64];
	struct {
		bool *show_simtime, *show_rendertime, *show_framerate;
	} debug;
	#define UPDATE_OBJECT_MEMORY_SIZE 65535
	// Fixed memory buffer. Bottom contains array of object descriptors. Each object has a fixed size component in the bottom region of the memory, and a pointer to memory in the top region.
	struct {
		uint32_t bottom_used, top_used, count, nextid;
		char mem[UPDATE_OBJECT_MEMORY_SIZE];
	} objects;
} update_data_t;

void Update_ClearInputAll ();
void Update_ClearInputMouse ();
void Update_ClearInputMouseButtons ();
void Update_ClearInputKeyboard ();

typedef struct {
	bool gravity;
	int time;
} ParticleAdd_arguments;
#define ParticleAdd(pixel, x, y, vx, vy, ...) ParticleAdd_ (pixel, x, y, vx, vy, (ParticleAdd_arguments){.gravity = true, .time = -1, __VA_ARGS__})
void ParticleAdd_ (uint8_t pixel, int x, int y, int32_t vx, int32_t vy, ParticleAdd_arguments arguments);
#define ParticleAddAngleVelocity(pixel, x, y, angle, velocity, ...) ParticleAdd (pixel, x, y, cos_turns (angle) * velocity, sin_turns (angle) * velocity, __VA_ARGS__)
void ParticlesUpdate (int left, int right, int bottom, int top);
void ParticleDelete (int index);
typedef struct {
	float rotation;
	bool flipx, flipy;
	int originx, originy;
	uint8_t color;
} CreateParticlesFromSprite_arguments;
#define CreateParticlesFromSprite(sprite, x, y, direction, velocity, ...) CreateParticlesFromSprite_ (sprite, x, y, direction, velocity, (CreateParticlesFromSprite_arguments){__VA_ARGS__})
void CreateParticlesFromSprite_ (const sprite_t *sprite, int x, int y, float direction, int32_t velocity, CreateParticlesFromSprite_arguments arguments);

#define UPDATE_CHANGE_STATE_DATA_SIZE_MAX 1024

// Careful calling this! The second argument must either be missing, or some data to be instantly copied to a 1KB buffer.
// If you're passing an address to some other data, that address must remain valid until the end of the frame when it will be dereferenced!
#define Update_ChangeState(new_state__, ...) \
	do { \
		__VA_OPT__(static_assert (sizeof (__VA_ARGS__) <= UPDATE_CHANGE_STATE_DATA_SIZE_MAX);) \
		Update_ChangeStatePrepare_ (new_state__, 0 __VA_OPT__(+ &__VA_ARGS__), 0 __VA_OPT__(+ sizeof (__VA_ARGS__))); \
		Update_ChangeStateNow_ (); \
	} while (0)

// If you use this, you must later call Update_ChangeStateNow_(); to actually do the state change
#define Update_ChangeStatePrepare(new_state__, ...) \
	do { \
		__VA_OPT__(static_assert (sizeof (__VA_ARGS__) <= UPDATE_CHANGE_STATE_DATA_SIZE_MAX);) \
		Update_ChangeStatePrepare_ (new_state__, 0 __VA_OPT__(+ &__VA_ARGS__), 0 __VA_OPT__(+ sizeof (__VA_ARGS__))); \
	} while (0)
void Update_ChangeStatePrepare_ (update_state_e new_state, const void *const data_to_copy_max_1kb, const size_t data_size);
void Update_ChangeStateNow_ ();
void Update_RunAfterStateChange (void (*func)());

typedef bool (*Update_Object_Func_t) (const void *self);

uint32_t Update_ObjectCreate_ (const void *const data, const size_t data_size, const Update_Object_Func_t UpdateAndRenderFunc, const int8_t layer, const bool survive_state_change);

// 4th argument is your object, which can be initialized as (object_t){a, b, c}
#define Update_ObjectCreate(update_and_render_func__, layer__, survive_state_change__, ...) \
	({ \
		static_assert (__VA_OPT__(true), "The object must be the final argument"); \
		static_assert (_Generic (typeof(update_and_render_func__(NULL)), bool: true, default: false), "Return value of update & render function must be bool"); \
		static_assert (!IS_POINTER (typeof(__VA_ARGS__)), "Do not pass a pointer - pass the actual object data by value. You may need to wrap it in a struct."); \
		static_assert (layer__ >= -128 && layer__ <= 127, "Layer must be between -128 and 127"); \
		Update_ObjectCreate_ (&(__VA_ARGS__), sizeof (typeof(__VA_ARGS__)), (Update_Object_Func_t)(update_and_render_func__), layer__, survive_state_change__); \
	})

typeof((update_data_t){}.frame) Update_GetUneditedFrameInputState ();
