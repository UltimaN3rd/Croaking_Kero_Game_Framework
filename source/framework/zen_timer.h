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

// You MUST call zen_Init() to use Zen Timer on Windows, otherwise the tick rate will be set at 1 and you'll get garbage.

#pragma push_macro ("SPLIT_TIMER_MAX")
#undef SPLIT_TIMER_MAX
#pragma push_macro ("SPLIT_TIMER_NAME_LENGTH")
#undef SPLIT_TIMER_NAME_LENGTH

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>









#ifdef WIN32








#include <windows.h>

typedef struct {
	LARGE_INTEGER start, end;
} zen_timer_t;

#define SPLIT_TIMER_MAX 64
#define SPLIT_TIMER_NAME_LENGTH 64

typedef struct {
	zen_timer_t timer;
	int current;
	int64_t splits[SPLIT_TIMER_MAX];
	char names[SPLIT_TIMER_MAX][64];
} zen_split_timer_t;

static struct {
	int64_t ticks_per_microsecond;
} zen_internal = {
	.ticks_per_microsecond = 1
};

static inline void zen_Init () {
	LARGE_INTEGER ticks_per_second;
	QueryPerformanceFrequency(&ticks_per_second);
	zen_internal.ticks_per_microsecond = ticks_per_second.QuadPart / 1000000;
}

static inline  zen_timer_t zen_Start () {
	zen_timer_t timer;
	QueryPerformanceCounter(&timer.start);
	return timer;
}

// Returns time in microseconds since the timer was Start()ed. Doesn't actually "end" the timer - just checks the current time and compares to the start time. So you could call this function multiple times with the same timer to continue getting the time since the timer started.
static inline int64_t  zen_End (zen_timer_t *timer) {
	QueryPerformanceCounter(&timer->end);
	return (timer->end.QuadPart - timer->start.QuadPart) / zen_internal.ticks_per_microsecond;
}

static inline void zen_Split (zen_split_timer_t *timer, const char *name) {
	if (timer->current >= SPLIT_TIMER_MAX) return;
	timer->splits[timer->current] = zen_End (&timer->timer);
	strncpy (timer->names[timer->current], name, SPLIT_TIMER_NAME_LENGTH);
	timer->timer.start = timer->timer.end;
	++timer->current;
}

static inline void zen_SplitReset (zen_split_timer_t *timer) {
	timer->current = 0;
	timer->timer = zen_Start ();
}

static inline void zen_SplitPrintAll (zen_split_timer_t *timer) {
	for (int i = 0; i < timer->current; ++i) {
		LOG ("%10lldus - %s", timer->splits[i], timer->names[i]);
	}
}











#elif defined __linux__











#include <time.h>

typedef struct {
	int64_t start, end;
} zen_timer_t;

#define SPLIT_TIMER_MAX 64
#define SPLIT_TIMER_NAME_LENGTH 64

typedef struct {
	zen_timer_t timer;
	int64_t splits[SPLIT_TIMER_MAX];
	char names[SPLIT_TIMER_MAX][64];
	int current;
} zen_split_timer_t;

// Stub function. Required on Windows, not necessary on Linux.
static inline void zen_Init () {}

static inline  zen_timer_t zen_Start () {
	zen_timer_t timer = {};
	struct timespec time;
	clock_gettime (CLOCK_MONOTONIC, &time);
	timer.start = (time.tv_sec * 1000000000   +   time.tv_nsec) / 1000;
	return timer;
}

// Returns time in microseconds since the timer was Start()ed. Doesn't actually "end" the timer - just checks the current time and compares to the start time. So you could call this function multiple times with the same timer to continue getting the time since the timer started.
static inline int64_t  zen_End (zen_timer_t *timer) {
	struct timespec time;
	clock_gettime (CLOCK_MONOTONIC, &time);
	timer->end = (time.tv_sec * 1000000000   +   time.tv_nsec) / 1000;
	return timer->end - timer->start;
}

static inline void zen_Split (zen_split_timer_t *timer, const char *name) {
	if (timer->current >= SPLIT_TIMER_MAX) return;
	timer->splits[timer->current] = zen_End (&timer->timer);
	strncpy (timer->names[timer->current], name, SPLIT_TIMER_NAME_LENGTH);
	timer->timer.start = timer->timer.end;
	++timer->current;
}

static inline void zen_SplitReset (zen_split_timer_t *timer) {
	timer->current = 0;
	timer->timer = zen_Start ();
}

static inline void zen_SplitPrintAll (zen_split_timer_t *timer) {
	for (int i = 0; i < timer->current; ++i) {
		LOG ("%10ldus - %s", timer->splits[i], timer->names[i]);
	}
}










#elif defined __APPLE__











#include <time.h>

typedef struct {
	int64_t start, end;
} zen_timer_t;

#define SPLIT_TIMER_MAX 64
#define SPLIT_TIMER_NAME_LENGTH 64

typedef struct {
	zen_timer_t timer;
	int64_t splits[SPLIT_TIMER_MAX];
	char names[SPLIT_TIMER_MAX][64];
	int current;
} zen_split_timer_t;

// Stub function. Required on Windows, not necessary on Linux.
static inline void zen_Init () {}

static inline  zen_timer_t zen_Start () {
	zen_timer_t timer = {};
	struct timespec time;
	clock_gettime (CLOCK_MONOTONIC, &time);
	timer.start = (time.tv_sec * 1000000000   +   time.tv_nsec) / 1000;
	return timer;
}

// Returns time in microseconds since the timer was Start()ed. Doesn't actually "end" the timer - just checks the current time and compares to the start time. So you could call this function multiple times with the same timer to continue getting the time since the timer started.
static inline int64_t  zen_End (zen_timer_t *timer) {
	struct timespec time;
	clock_gettime (CLOCK_MONOTONIC, &time);
	timer->end = (time.tv_sec * 1000000000   +   time.tv_nsec) / 1000;
	return timer->end - timer->start;
}

static inline void zen_Split (zen_split_timer_t *timer, const char *name) {
	if (timer->current >= SPLIT_TIMER_MAX) return;
	timer->splits[timer->current] = zen_End (&timer->timer);
	strncpy (timer->names[timer->current], name, SPLIT_TIMER_NAME_LENGTH);
	timer->timer.start = timer->timer.end;
	++timer->current;
}

static inline void zen_SplitReset (zen_split_timer_t *timer) {
	timer->current = 0;
	timer->timer = zen_Start ();
}

static inline void zen_SplitPrintAll (zen_split_timer_t *timer) {
	for (int i = 0; i < timer->current; ++i) {
		LOG ("%10"PRId64"us - %s", timer->splits[i], timer->names[i]);
	}
}










#endif

#pragma pop_macro ("SPLIT_TIMER_MAX")
#pragma pop_macro ("SPLIT_TIMER_NAME_LENGTH")