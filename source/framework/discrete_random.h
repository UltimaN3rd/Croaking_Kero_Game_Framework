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

// u64 my_random_state = DiscreteRandom_SeedFromTime ();
// u32 a_random_number = DiscreteRandom_Next (&my_random_state);
// Make a local u64. This is the specific state of your random sequence. Assign it whatever value you want and call call DiscreteRandom_Seed (&your_var), or just use SeedFromTime(). Then to get each random value, do: u32 num = DiscreteRandom_Next (&your_var);

// Code based on https://en.wikipedia.org/wiki/Permuted_congruential_generator#Example_code
// Edits have been made, though the core algorithm is unchanged.
// Original license: https://en.wikipedia.org/wiki/Wikipedia:Text_of_the_Creative_Commons_Attribution-ShareAlike_4.0_International_License

#include <stdint.h>

#define DISCRETE_RANDOM_MAX 0xffffffff
#define DISCRETE_RANDOM_MULTIPLIER 6364136223846793005u
#define DISCRETE_RANDOM_INCREMENT 1442695040888963407u

static inline u32 DiscreteRandom_InternalRotate (u32 x, unsigned r) {
	return x >> r | x << (-r & 31);
}

static inline u32 DiscreteRandom_Next (u64 *state) {
	u64 x = *state;
	unsigned int count = (unsigned int)(x >> 59);		// 59 = 64 - 5

	*state = x * DISCRETE_RANDOM_MULTIPLIER + DISCRETE_RANDOM_INCREMENT;
	x ^= x >> 18;								// 18 = (64 - 27)/2
	return DiscreteRandom_InternalRotate ((u32)(x >> 27), count);	// 27 = 32 - 5
}

static inline void DiscreteRandom_Seed (u64 *seed) {
	*seed += DISCRETE_RANDOM_INCREMENT;
	DiscreteRandom_Next (seed);
}

static inline int DiscreteRandom_Range (u64 *state, int min, int max) {
	if (min > max) {
		int temp = min;
		min = max;
		max = temp;
	}
	return DiscreteRandom_Next (state)%(max - min + 1) + min;
}

#define DiscreteRandom_Percentage(state) DiscreteRandom_Range (state, 0, 100)

static inline int DiscreteRandom_Bool (u64 *state) {
	return DiscreteRandom_Next (state) % 2;
}

static inline f32 DiscreteRandom_Rangef (u64 *state, f32 min, f32 max) {
	return (max - min) * ((f32)DiscreteRandom_Next(state) / (f32)DISCRETE_RANDOM_MAX) + min;
}

#include <time.h>

static inline u64 DiscreteRandom_SeedFromTime () {
	u64 t = (u64) time (NULL);
	DiscreteRandom_Seed (&t);
	return t;
}