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
#include "c23defs.h"

typedef struct {
	uint32_t count;
	counted_by(count) int16_t samples[];
} sound_sample_t;

extern const sound_sample_t sound_sample_square, sound_sample_sine, sound_sample_triangle, sound_sample_saw, sound_sample_noise, sound_sample_silence, sound_sample_preparing;