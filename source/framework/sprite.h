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
#include <string.h>

#include "framework_types.h"


// --------------------------------------------------------------------------------
// Base functions (copy-paste this whole section and edit to create new variants)
void sprite_Blit(const sprite_t *source, sprite_t *destination, int x, int y);

void sprite_BlitRotated90(const sprite_t *source, sprite_t *destination, int x, int y, int originx, int originy);
void sprite_BlitRotated180(const sprite_t *source, sprite_t *destination, int x, int y, int originx, int originy);
void sprite_BlitRotated270(const sprite_t *source, sprite_t *destination, int x, int y, int originx, int originy);

void sprite_BlitFlippedHorizontally(const sprite_t *source, sprite_t *destination, int x, int y);
void sprite_BlitFlippedVertically(const sprite_t *source, sprite_t *destination, int x, int y);

void sprite_SampleRotated(const sprite_t *source, sprite_t *destination, int x, int y, f32 angle, f32 originx, f32 originy);
void sprite_SampleRotatedFlipped(const sprite_t *source, sprite_t *destination, int x, int y, f32 angle, f32 originx, f32 originy, bool flipx, bool flipy);

// --------------------------------------------------------------------------------
// Silhouette (in given color)
void sprite_BlitColor(const sprite_t *source, sprite_t *destination, int x, int y, u8 color);

void sprite_BlitRotated90Color(const sprite_t *source, sprite_t *destination, int x, int y, u8 color, int originx, int originy);
void sprite_BlitRotated180Color(const sprite_t *source, sprite_t *destination, int x, int y, u8 color, int originx, int originy);
void sprite_BlitRotated270Color(const sprite_t *source, sprite_t *destination, int x, int y, u8 color, int originx, int originy);

void sprite_BlitFlippedHorizontallyColor(const sprite_t *source, sprite_t *destination, int x, int y, u8 color);
void sprite_BlitFlippedVerticallyColor(const sprite_t *source, sprite_t *destination, int x, int y, u8 color);

void sprite_SampleRotatedColor(const sprite_t *source, sprite_t *destination, int x, int y, f32 angle, f32 originx, f32 originy, u8 color);
void sprite_SampleRotatedFlippedColor(const sprite_t *source, sprite_t *destination, int x, int y, f32 angle, f32 originx, f32 originy, bool flipx, bool flipy, u8 color);

// --------------------------------------------------------------------------------
// Single color swap
void sprite_BlitSubstituteColor(const sprite_t *source, sprite_t *destination, int x, int y, u8 color_from, u8 color_to);

// --------------------------------------------------------------------------------
// Palette swap
void sprite_BlitColorSwap(const sprite_t *source, sprite_t *destination, int x, int y, const u8 color_swap_palette[256]);

void sprite_BlitColorSwapRotated90(const sprite_t *source, sprite_t *destination, int x, int y, int originx, int originy, const u8 color_swap_palette[256]);
void sprite_BlitColorSwapRotated180(const sprite_t *source, sprite_t *destination, int x, int y, int originx, int originy, const u8 color_swap_palette[256]);
void sprite_BlitColorSwapRotated270(const sprite_t *source, sprite_t *destination, int x, int y, int originx, int originy, const u8 color_swap_palette[256]);

void sprite_BlitColorSwapFlippedHorizontally(const sprite_t *source, sprite_t *destination, int x, int y, const u8 color_swap_palette[256]);
void sprite_BlitColorSwapFlippedVertically(const sprite_t *source, sprite_t *destination, int x, int y, const u8 color_swap_palette[256]);

void sprite_SampleColorSwapRotated(const sprite_t *source, sprite_t *destination, int x, int y, f32 angle, f32 originx, f32 originy, const u8 color_swap_palette[256]);
void sprite_SampleColorSwapRotatedFlipped(const sprite_t *source, sprite_t *destination, int x, int y, f32 angle, f32 originx, f32 originy, bool flipx, bool flipy, const u8 color_swap_palette[256]);

// --------------------------------------------------------------------------------

static inline void sprite_SetPixelsToZero (sprite_t *sprite) {
	memset (sprite->p, 0, sprite->w * sprite->h);
}

typedef struct {
	const sprite_t *sprite;
	int spritex, spritey, samplerx, samplery;
	f32 rotation, originx, originy;
	bool flipx, flipy;
} sprite_SamplePixel_arguments;
#define sprite_SamplePixel(...) sprite_SamplePixel_ ((sprite_SamplePixel_arguments){__VA_ARGS__})
u8 sprite_SamplePixel_ (sprite_SamplePixel_arguments arguments);