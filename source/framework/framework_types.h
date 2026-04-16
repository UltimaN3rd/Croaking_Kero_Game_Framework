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
#include "c23defs.h"
#include "utilities.h"

#define MAX(__a__, __b__) ({auto __a1__ = (__a__); auto __b1__ = (__b__); __a1__ > __b1__ ? __a1__ : __b1__;})
#define MIN(__a__, __b__) ({auto __a1__ = (__a__); auto __b1__ = (__b__); __a1__ < __b1__ ? __a1__ : __b1__;})
#define SIGN(__a__) ((__a__) < 0 ? -1 : 1)

#define repeat_(count, counter) for (int repeat_var_##counter = (count); repeat_var_##counter > 0; repeat_var_##counter--)
#define repeat(count) repeat_(count, __COUNTER__)

typedef enum : u8 { MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE, MOUSE_X1, MOUSE_X2 } mouse_button_e;
#define MOUSE_BUTTON_COUNT (MOUSE_X2+1)

typedef struct {
	void (*Initialize) (const void *const data);
	void (*Update) ();
} update_state_functions_t;

typedef union {
	i32 i32;
	struct {
		u16 low;
		i16 high;
	};
} int32split_t;

typedef struct {
	int32split_t x, y;
} vec2i32split_t;

static inline f32 absf(f32 a) {
	return (a < 0) ? -a : a;
}

// a and b MUST be in the range -0.5 to 0.5
static inline f32 AngleDifference(f32 from, f32 to) {
    assert (from >= -0.5f && from <= 0.5f);
    assert (to >= -0.5f && to <= 0.5f);
	f32 d = to - from;
	if(d > 0.5f) d = 1.f - d;
	return d;
}

typedef struct {
	u16 w, h;
	u8 *p;
} sprite_t;

#define BITMAP_FONT_FIRST_VISIBLE_CHAR 33
#define BITMAP_FONT_LAST_VISIBLE_CHAR 126
#define BITMAP_FONT_NUM_VISIBLE_CHARS (BITMAP_FONT_LAST_VISIBLE_CHAR - BITMAP_FONT_FIRST_VISIBLE_CHAR + 1)

typedef struct {
    i8 line_height, baseline;
    u8 space_width;
    const sprite_t *bitmaps[BITMAP_FONT_NUM_VISIBLE_CHARS];
    i8 descent[BITMAP_FONT_NUM_VISIBLE_CHARS];
} font_t;

typedef enum { sound_waveform_none, sound_waveform_sine, sound_waveform_triangle, sound_waveform_saw, sound_waveform_pulse, sound_waveform_noise, sound_waveform_silence, sound_waveform_preparing } sound_waveform_e;

typedef struct {
    f32 peak;
    u16 attack;
    u16 decay;
    f32 sustain;
    u16 release;
} ADSR_t;

typedef struct {
    u16 frequency_range;
    u16 vibrations_per_hundred_seconds;
} vibrato_t;

typedef struct sound_t {
    u32 duration;
    const struct sound_t *next;
    u16 frequency;
    ADSR_t ADSR;
	vibrato_t vibrato;
	i16 sweep; // 0 means frequency stays the same. Otherwise, this is an offset to which frequency will lerp through the duration of the sound
	i8 square_duty_cycle; // 0 produces a contant signal (no sound). 1 to 127 represent ~1%-50% duty cycle. The top 50% is left out since it's equivalent (usually) to the bottom 50% in reverse. Only has an effect when waveform == sound_waveform_pulse
	i8 square_duty_cycle_sweep; // Delta from starting duty cycle across full duration of sound
    sound_waveform_e waveform;
} sound_t;

typedef struct {
    u8 count;
    const sound_t *sounds [[counted_by(count)]] [];
} sound_group_t;

typedef struct {
    u8 count;
    const sound_t *sounds [[counted_by(count)]] [];
} sound_music_t;

typedef struct {
    struct {
        i16 x, y;
    } offset;
    const sprite_t *sprite;
} cursor_t;