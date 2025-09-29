#pragma once

#include <stdio.h>
#include <stdint.h>

static_assert (sizeof (float) == 4); // Make sure float is 32 bits
typedef enum {cereal_bool, cereal_u64, cereal_u32, cereal_u16, cereal_u8, cereal_f32, cereal_array} cereal_type_e;
// cereal_array shall be serialized as follows:
// KEY [COUNT, a, b, c, ..., z]
// where KEY is the cereal_t.key string, COUNT is the number of elements in the array between 0 and UINT64_MAX, a, b, c, are elements of the array of type array_type
typedef struct {
	char key[32];
	cereal_type_e type;
	void *var;
	cereal_type_e array_type;
	cereal_type_e array_counter_type;
	void *array_counter;
} cereal_t;

extern uint64_t cereal_dummy;

bool cereal_WriteToFile (const cereal_t cereal[], const int cereal_count, FILE *file);
bool cereal_ReadFromFile (const cereal_t cereal[], const int cereal_count, FILE *file);