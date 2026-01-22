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

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679
#endif
#define PI M_PI
#define TWOPI (M_PI + M_PI)
#define HALFPI (M_PI / 2.0)
#define QUARTERPI (M_PI / 4.0)
#define ROOT2 1.4142135623730950488016887242096980785696718753769480731766797379907324784621070388503875343276415727
#include <string.h>
#include <assert.h>

#include "turns_math.h"
#include "DEFER.h"

#ifndef NDEBUG
#define UNREACHABLE(...) do { LOG (__VA_ARGS__); abort (); } while (false)
#else
#define UNREACHABLE(...) unreachable();
#endif

#define SWAP(a, b) do {auto t = (a); (a) = (b); (b) = t;} while(0)

typedef struct {
	float x, y;
} vec2f_t;

static inline float vec2f_Distance (vec2f_t a, vec2f_t b) {
	a.x -= b.x;
	a.y -= b.y;
	a.x *= a.x;
	a.y *= a.y;
	return sqrt (a.x + a.y);
}

static inline vec2f_t vec2f_FromTo (vec2f_t a, vec2f_t b) {
	b.x -= a.x;
	b.y -= a.y;
	return b;
}

static inline vec2f_t vec2f_LerpInaccurate (vec2f_t from, vec2f_t to, float t) {
    from.x += (to.x - from.x) * t;
    from.y += (to.y - from.y) * t;
    return from;
}

static inline float vec2f_Magnitude (vec2f_t v) {
    return sqrtf (v.x * v.x + v.y * v.y);
}

static inline float vec2f_AngleOfAsTurns (vec2f_t v) {
    return XYToAngleTurns (v.x, v.y);
}

typedef union {
    struct { int32_t x, y; };
    struct { int32_t w, h; };
    struct { int32_t width, height; };
    int32_t array[2];
} vec2i_t;

typedef struct { int16_t x, y; } vec2i16_t;
typedef struct { int8_t x, y; } vec2i8_t;
typedef struct { uint8_t x, y; } vec2u8_t;

static inline float vec2i16_Distance (vec2i16_t a, vec2i16_t b) {
    int16_t dx = b.x - a.x;
    int16_t dy = b.y - a.y;
    return sqrtf (dx * dx + dy * dy);
}

// Checks the number of characters of the smaller of the two strings
bool StringsAreTheSame (const char *a, const char *b);

// If one string is longer, only compares the overlapping characters. "Hello" == "hello world!"
bool StringsAreTheSameCaseInsensitive (const char *a, const char *b);

bool StringEndsWithCaseInsensitive (const char *a, const char *b);

static inline char *StringSkipWhiteSpace (char *c) {
	register char a = *c;
	while (a > 0 && (a < 33 || a > 126)) {
		a = *(++c);
	}
	return c;
}

static inline char *StringSkipNonWhiteSpace (char *c) {
	register char a = *c;
	while (a > 0 && (a > 32 && a < 127)) {
		a = *(++c);
	}
	return c;
}
static inline char *StringSkipWord (char *c) {
    return StringSkipWhiteSpace (StringSkipNonWhiteSpace (StringSkipWhiteSpace (c)));
}

static inline char *StringFindNext (char *string, char find) {
    for (;;) {
        if (*string == find) return string;
        if (*string == '\0') return NULL;
        ++string;
    }
}

static inline char *StringFindNextOfThese (const char *string, const char *find_list) {
    int list_length = strlen (find_list);
    if (list_length == 0) return NULL;
    for (;;) {
        if (*string == '\0') return NULL;
        for (int i = 0; i < list_length; ++i) {
            if (*string == find_list[i]) return (char*)string;
        }
        ++string;
    }
}

static inline char *StringSkipLine (char *c) {
    char a = *c;
    while (a != '\n' && a != '\0') a = *(++c);
    if (a != '\0') ++c;
    return c;
}

static inline bool StringCopyDirectory (char *source, char *destination, int max_chars) {
    int source_length = strlen (source);
    char *s = source + source_length - 1;
    while (*s != '/' && *s != '\\') --s, --source_length;
    if (source_length > max_chars) {
        LOG ("Source directory of %s too long. %d > %d", source, source_length, max_chars);
        return false;
    }
    strncpy (destination, source, source_length);
    return true;
}

// Iterates over a given number of characters, and if each one is an upper case letter it will be changed to lower case.
static inline void StringDecapitalize (char *c, int number_of_letters_to_decapitalize) {
    while (number_of_letters_to_decapitalize-- && *c)
        if (*c >= 'A' && *c <= 'Z')
            *c += 'a' - 'A';
}

// Iterates over a given number of characters, and if each one is a lower case letter it will be changed to upper case.
static inline void StringCapitalize (char *c, int number_of_letters_to_capitalize) {
    while (number_of_letters_to_capitalize-- && *c)
        if (*c >= 'a' && *c <= 'z')
            *c -= 'a' - 'A';
}

// Returns NULL if there's no file extension. Otherwire returns the first character after the last period in the string.
static inline const char *StringGetFileExtension (const char *c) {
    const char *start = c;
    while (*++c != '\0'); // Skip to end of string
    const char *end = c;
    while (*--c != '.') // Go back to last '.' in string
        if (c == start)
            return NULL;
    ++c; // One letter after first period
    if (c == end)
        return NULL;
    return c;
}

// Finds the first non-matching character and returns -1 if a[N] < b[N], 1 if a[N] > b[N]. 0 if the strings are equal. Also returns -1 if a matches b but is shorter and vice versa.
static inline int StringCompareCaseInsensitive (const char *a, const char *b) {
    char ca = *a, cb = *b;
    const int capitalize = 'A' - 'a';
    while (ca && cb) {
        if (ca >= 'a' && ca <= 'z') ca += capitalize;
        if (cb >= 'a' && cb <= 'z') cb += capitalize;
        if (ca != cb) return (ca < cb) ? -1 : 1;
        ca = *++a;
        cb = *++b;
    }
    // No mis-matched characters were found
    if (ca) return 1; // b is shorter than a
    if (cb) return -1; // a is shorter than b
    return 0; // The strings are equal
}

#define repeat_(count, counter) for (int repeat_var_##counter = (count); repeat_var_##counter > 0; repeat_var_##counter--)
#define repeat(count) repeat_(count, __COUNTER__)

#define ROUNDF(flt) (((flt) > 0) ? ((flt)+.5f) : ((flt)-.5f))

void BresenhamLine (int x1, int y1, int x2, int y2, void (*PixelFunction) (int x, int y));

#if WIN32
#include <io.h>
static inline bool FileExists (const char *const filename) { return _access(filename, 0) == 0; }
#else
#include <unistd.h>
static inline bool FileExists (const char *const filename) { return access (filename, F_OK) == 0; }
#endif

static inline char *ReadEntireFileAllocateBuffer (const char *filename) {
	FILE *file = fopen (filename, "rb");
	if (!file) { LOG ("Failed to read file [%s]", filename); return NULL; }
	DEFER (fclose (file));
	long start_pos = ftell (file);
	fseek (file, 0, SEEK_END);
	long end_pos = ftell (file);
	fseek (file, start_pos, SEEK_SET);
	long size = end_pos - start_pos;
	char *buf = malloc (size);
	assert (buf); if (buf == NULL) { LOG ("[%s] Failed to allocate %ld bytes", filename, size); return NULL; }
    bool success = false;
	DEFER (if (!success) free (buf););
	auto result = fread (buf, 1, size, file);
    assert (result == size);
    if (result != size) { LOG ("Reading file [%s] only read %lu/%ld bytes", filename, result, size); return NULL; }
    success = true;
    return buf;
}

#define IS_POINTER(__var_or_type__) (__builtin_types_compatible_p(typeof (__var_or_type__), void*) || __builtin_types_compatible_p(typeof (__var_or_type__), void const*) || __builtin_types_compatible_p(typeof (__var_or_type__), void volatile*) || __builtin_types_compatible_p(typeof (__var_or_type__), void const volatile*))

#define foreach(__element_name__, __array__) for (auto __element_name__ = &__array__[0]; __element_name__ < 1[&__array__]; ++__element_name__)

static inline uint32_t RandFast () {
    static uint32_t r = 1;
	r ^= r << 13;
	r ^= r >> 17;
	r ^= r << 5;
	return r;
}

static inline float RandFastf () { return (float)RandFast() / UINT32_MAX; }

static inline int32_t RandFast_Range (int32_t from, int32_t to) {
    if (from > to) SWAP (from, to);
    return (int32_t)(RandFastf() * (to - from) + from);
}
