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

#include <stdint.h>
#include <stdbool.h>
#include <float.h>
#include <inttypes.h>
#include <assert.h>

#define MAX(__a__, __b__) ({auto __a1__ = (__a__); auto __b1__ = (__b__); __a1__ > __b1__ ? __a1__ : __b1__;})
#define MIN(__a__, __b__) ({auto __a1__ = (__a__); auto __b1__ = (__b__); __a1__ < __b1__ ? __a1__ : __b1__;})
#define SIGN(__a__) ((__a__) < 0 ? -1 : 1)

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

typedef struct {
	uint16_t w, h;
	uint8_t p[];
} sprite_t;

#define BITMAP_FONT_FIRST_VISIBLE_CHAR 33
#define BITMAP_FONT_LAST_VISIBLE_CHAR 126
#define BITMAP_FONT_NUM_VISIBLE_CHARS (BITMAP_FONT_LAST_VISIBLE_CHAR - BITMAP_FONT_FIRST_VISIBLE_CHAR + 1)

typedef struct {
    int8_t line_height, baseline;
    uint8_t space_width;
    const sprite_t *bitmaps[BITMAP_FONT_NUM_VISIBLE_CHARS];
    int8_t descent[BITMAP_FONT_NUM_VISIBLE_CHARS];
} font_t;

#include "samples.h"

typedef struct {
    float peak;
    uint16_t attack;
    uint16_t decay;
    float sustain;
    uint16_t release;
} ADSR_t;

typedef struct sound_t {
    uint32_t t;
    uint32_t duration;
    const struct sound_t *next;
    float d;
    int16_t frequency_begin, frequency_end;
    uint8_t ADSR_state;
    ADSR_t ADSR;
    const sound_sample_t *sample;
} sound_t;

typedef struct {
    uint8_t count;
    counted_by(count) const sound_t *sounds[];
} sound_group_t;

typedef struct {
    uint8_t count;
    counted_by(count) const sound_t *sounds[];
} sound_music_t;