#pragma once

#include <stdint.h>
#include <string.h>
#include "resources.h"

void sprite_Blit(const sprite_t *source, sprite_t *destination, int x, int y);

void sprite_BlitRotated90(const sprite_t *source, sprite_t *destination, int x, int y);
void sprite_BlitRotated180(const sprite_t *source, sprite_t *destination, int x, int y);
void sprite_BlitRotated270(const sprite_t *source, sprite_t *destination, int x, int y);

void sprite_BlitFlippedHorizontally(const sprite_t *source, sprite_t *destination, int x, int y);
void sprite_BlitFlippedVertically(const sprite_t *source, sprite_t *destination, int x, int y);


void sprite_SampleRotated(const sprite_t *source, sprite_t *destination, int x, int y, float angle, float originx, float originy);
void sprite_SampleRotatedFlipped(const sprite_t *source, sprite_t *destination, int x, int y, float angle, float originx, float originy, bool flipx, bool flipy);


void sprite_BlitColor(const sprite_t *source, sprite_t *destination, int x, int y, uint8_t color);

void sprite_BlitRotated90Color(const sprite_t *source, sprite_t *destination, int x, int y, uint8_t color);
void sprite_BlitRotated180Color(const sprite_t *source, sprite_t *destination, int x, int y, uint8_t color);
void sprite_BlitRotated270Color(const sprite_t *source, sprite_t *destination, int x, int y, uint8_t color);

void sprite_BlitFlippedHorizontallyColor(const sprite_t *source, sprite_t *destination, int x, int y, uint8_t color);
void sprite_BlitFlippedVerticallyColor(const sprite_t *source, sprite_t *destination, int x, int y, uint8_t color);


static inline void sprite_SetPixelsToZero (sprite_t *sprite) {
	memset (sprite->p, 0, sprite->w * sprite->h);
}

typedef struct {
	const sprite_t *sprite;
	int spritex, spritey, samplerx, samplery;
	float rotation, originx, originy;
	bool flipx, flipy;
} sprite_SamplePixel_arguments;
#define sprite_SamplePixel(...) sprite_SamplePixel_ ((sprite_SamplePixel_arguments){__VA_ARGS__})
uint8_t sprite_SamplePixel_ (sprite_SamplePixel_arguments arguments);