#pragma once

// uint64_t my_random_state = DiscreteRandom_SeedFromTime ();
// uint32_t a_random_number = DiscreteRandom_Next (&my_random_state);
// Make a local uint64_t. This is the specific state of your random sequence. Assign it whatever value you want and call call DiscreteRandom_Seed (&your_var), or just use SeedFromTime(). Then to get each random value, do: uint32_t num = DiscreteRandom_Next (&your_var);

// Code copied from https://en.wikipedia.org/wiki/Permuted_congruential_generator#Example_code

#include <stdint.h>

#define DISCRETE_RANDOM_MAX 0xffffffff
#define DISCRETE_RANDOM_MULTIPLIER 6364136223846793005u
#define DISCRETE_RANDOM_INCREMENT 1442695040888963407u

static inline uint32_t DiscreteRandom_InternalRotate (uint32_t x, unsigned r) {
	return x >> r | x << (-r & 31);
}

static inline uint32_t DiscreteRandom_Next (uint64_t *state) {
	uint64_t x = *state;
	unsigned int count = (unsigned int)(x >> 59);		// 59 = 64 - 5

	*state = x * DISCRETE_RANDOM_MULTIPLIER + DISCRETE_RANDOM_INCREMENT;
	x ^= x >> 18;								// 18 = (64 - 27)/2
	return DiscreteRandom_InternalRotate ((uint32_t)(x >> 27), count);	// 27 = 32 - 5
}

static inline void DiscreteRandom_Seed (uint64_t *seed) {
	*seed += DISCRETE_RANDOM_INCREMENT;
	DiscreteRandom_Next (seed);
}

static inline int DiscreteRandom_Range (uint64_t *state, int min, int max) {
	if (min > max) {
		int temp = min;
		min = max;
		max = temp;
	}
	return DiscreteRandom_Next (state)%(max - min + 1) + min;
}

#define DiscreteRandom_Percentage(state) DiscreteRandom_Range (state, 0, 100)

static inline int DiscreteRandom_Bool (uint64_t *state) {
	return DiscreteRandom_Next (state) % 2;
}

static inline float DiscreteRandom_Rangef (uint64_t *state, float min, float max) {
	return (max - min) * ((float)DiscreteRandom_Next(state) / (float)DISCRETE_RANDOM_MAX) + min;
}

#include <time.h>

static inline uint64_t DiscreteRandom_SeedFromTime () {
	uint64_t t = (uint64_t) time (NULL);
	DiscreteRandom_Seed (&t);
	return t;
}