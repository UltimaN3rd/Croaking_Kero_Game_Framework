#pragma once

#include <stdint.h>
#include "c23defs.h"

typedef struct {
	uint32_t count;
	counted_by(count) int16_t samples[];
} sound_sample_t;

extern const sound_sample_t sound_sample_square, sound_sample_sine, sound_sample_triangle, sound_sample_saw, sound_sample_noise, sound_sample_silence, sound_sample_preparing;