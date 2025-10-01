#pragma once

#include "framework.h"

typedef struct update_data_s { // update_data_t
	mouse_t *source_mouse;
	#define KEY_NORMAL 0
	#define KEY_PRESSED 0b1
	#define KEY_HELD 0b10
	#define KEY_RELEASED 0b100
	#define KEY_REPEATED 0b1000
	struct {
		uint8_t keyboard_state[256];
		uint8_t mouse_state[MOUSE_BUTTON_COUNT];
		mouse_t mouse;
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
	update_state_e new_state;
	#define KEYBOARD_EVENT_MAX 256
	struct {
		uint32_t count;
		struct {
			os_key_e key;
			bool pressed;
		} events [KEYBOARD_EVENT_MAX];
	} keyboard_events;
	#define MOUSE_EVENT_MAX 256
	struct {
		uint32_t count;
		struct {
			enum { mouse_event_button, mouse_event_movement } type;
			union {
				struct {
					mouse_button_e button;
					bool pressed;
				} button;
				struct {
					int16_t x, y;
				} movement;
			};
		} events[MOUSE_EVENT_MAX];
	} mouse_events;
	char game_save_filename[560];
	char config_filename[560];
	bool new_game;
	char debug_frame_time_string[64];
	struct {
		#ifndef FLOATY_TEXT_MAX
		#define FLOATY_TEXT_MAX 8
		#endif
		unsigned int count;
		struct {
			int x;
			int32split_t y;
			int vy;
			int time;
			char string[32];
		} text[FLOATY_TEXT_MAX];
	} floaty_text;
	struct {
		bool *show_simtime, *show_rendertime, *show_framerate;
	} debug;
} update_data_t;

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

void Update_ChangeState (update_state_e new_state);

#define FloatyTextPrintf(x, y, vy, time, str, ...) do {\
	char s[32];\
	snprintf (s, 32, str, __VA_ARGS__);\
	FloatyTextCreate (x, y, vy, time, s);\
} while (0)
void FloatyTextCreate (int x, int y, int vy, int time, const char *const str);
void FloatyTextDelete (unsigned int i);
