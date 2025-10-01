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

#include <stdio.h>
#include <stdint.h>

static_assert (sizeof (float) == 4); // Make sure float is 32 bits
typedef enum {cereal_bool, cereal_u64, cereal_u32, cereal_u16, cereal_u8, cereal_f32, cereal_array, cereal_string} cereal_type_e;
// cereal_array shall be serialized as follows:
// KEY [COUNT, a, b, c, ..., z]
// where KEY is the cereal_t.key string, COUNT is the number of elements in the array between 0 and UINT64_MAX, a, b, c, are elements of the array of type array_type
typedef struct {
	char key[32];
	cereal_type_e type;
	void *var;
	union {
		struct {
			cereal_type_e type;
			cereal_type_e counter_type;
			void *counter;
		} array;
		struct {
			uint16_t capacity;
		} string;
	} u;
} cereal_t;

extern uint64_t cereal_dummy;

bool cereal_WriteToFile (const cereal_t cereal[], const int cereal_count, FILE *file);
bool cereal_ReadFromFile (const cereal_t cereal[], const int cereal_count, FILE *file);