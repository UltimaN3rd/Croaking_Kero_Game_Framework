#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <float.h>
#include <inttypes.h>
#include <assert.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SIGN(a) ((a) < 0 ? -1 : 1)

#define repeat_(count, counter) for (int repeat_var_##counter = (count); repeat_var_##counter > 0; repeat_var_##counter--)
#define repeat(count) repeat_(count, __COUNTER__)

typedef enum { MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE, MOUSE_X1, MOUSE_X2 } mouse_button_e;
#define MOUSE_BUTTON_COUNT (MOUSE_X2+1)

typedef struct {
	void (*Initialize) ();
	void (*Update) ();
} update_state_functions_t;

typedef struct {
	int x, y;
	uint8_t buttons;
} mouse_t;

typedef union {
	int32_t i32;
	struct {
		uint16_t low;
		int16_t high;
	};
} int32split_t;

typedef struct {
	int32split_t x, y;
} vec2i32split_t;

static inline float absf(float a) {
	return (a < 0) ? -a : a;
}

// a and b MUST be in the range -0.5 to 0.5
static inline float AngleDifference(float from, float to) {
    assert (from >= -0.5f && from <= 0.5f);
    assert (to >= -0.5f && to <= 0.5f);
	float d = to - from;
	if(d > 0.5f) d = 1.f - d;
	return d;
}

#include "c23defs.h"
#include "osinterface.h"
#include "utilities.h"

static inline mouse_button_e ConvertMouseButton (os_mouse_button_e button) {
	switch (button) {
		case os_MOUSE_LEFT: return MOUSE_LEFT;
		case os_MOUSE_RIGHT: return MOUSE_RIGHT;
		case os_MOUSE_MIDDLE: return MOUSE_MIDDLE;
		case os_MOUSE_X1: return MOUSE_X1;
		case os_MOUSE_X2: return MOUSE_X2;
		default: UNREACHABLE ("%s", "Invalid mouse button! Check the os_mouse_button_e enum for valid values!!! >:(");
	}
	unreachable();
}

extern os_public_t os_public;
extern os_private_t os_private;

#include "log.h"
#include "cereal.h"
#include "DEFER.h"
#include "discrete_random.h"
#include "explorer.h"
#include "folders.h"
#include "resources.h"
#include "game.h"
#include "render.h"
#include "menu.h"
#include "samples.h"
#include "sprite.h"
#include "sound.h"
#include "update.h"
#include "zen_timer.h"

#ifndef OSINTERFACE_FRAME_BUFFER_SCALED
#error "You must define OSINTERFACE_FRAME_BUFFER_SCALED"
#endif
#ifndef OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE
#error "You must define OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE"
#endif
#ifndef OSINTERFACE_COLOR_INDEX_MODE
#error "You must define OSINTERFACE_COLOR_INDEX_MODE"
#endif