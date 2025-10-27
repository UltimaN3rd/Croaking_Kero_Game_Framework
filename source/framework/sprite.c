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

#include "sprite.h"
#include "framework_types.h"

#include <assert.h>
#include "turns_math.h"

void sprite_Blit(const sprite_t *source, sprite_t *destination, int x, int y) {
	int left, right, bottom, top;
	left    = MAX(0, x);
	right   = MIN(destination->w-1, x+source->w-1);
	bottom  = MAX(0, y);
	top     = MIN(destination->h-1, y+source->h-1);
	for(int y2 = bottom; y2 <= top; ++y2) {
		for(int x2 = left; x2 <= right; ++x2) {
			uint8_t source_pixel = source->p[x2-x + (y2-y)*source->w];
			if (source_pixel != 0)
				destination->p[x2 + y2*destination->w] = source_pixel;
		}
	}
}

void sprite_BlitRotated90(const sprite_t *source, sprite_t *destination, int x, int y) {
	int left, right, bottom, top;
	left    = MAX(0, x);
	right   = MIN(destination->w-1, x+source->h-1);
	bottom  = MAX(0, y);
	top     = MIN(destination->h-1, y+source->w-1);
	for(int y2 = bottom; y2 <= top; ++y2) {
		for(int x2 = left; x2 <= right; ++x2) {
			uint8_t source_pixel = source->p[y2-y + (x2-x)*source->w];
			if (source_pixel != 0)
				destination->p[x2 + y2*destination->w] = source_pixel;
		}
	}
}

void sprite_BlitRotated180(const sprite_t *source, sprite_t *destination, int x, int y) {
	int left, right, bottom, top, w, h;
	left    = MAX(0, x);
	right   = MIN(destination->w-1, x+source->w-1);
	bottom  = MAX(0, y);
	top     = MIN(destination->h-1, y+source->h-1);
	w = source->w-1;
	h = source->h-1;
	for(int y2 = bottom; y2 <= top; ++y2) {
		for(int x2 = left; x2 <= right; ++x2) {
			uint8_t source_pixel = source->p[w - (x2-x) + (h-(y2-y))*source->w];
			if (source_pixel != 0)
				destination->p[x2 + y2*destination->w] = source_pixel;
		}
	}
}

void sprite_BlitRotated270(const sprite_t *source, sprite_t *destination, int x, int y) {
	int left, right, bottom, top, w, h;
	left    = MAX(0, x);
	right   = MIN(destination->w-1, x+source->h-1);
	bottom  = MAX(0, y);
	top     = MIN(destination->h-1, y+source->w-1);
	w = source->w-1;
	h = source->h-1;
	for(int y2 = bottom; y2 <= top; ++y2) {
		for(int x2 = left; x2 <= right; ++x2) {
			uint8_t source_pixel = source->p[h-(y2-y) + (w-(x2-x))*source->w];
			if (source_pixel != 0)
				destination->p[x2 + y2*destination->w] = source_pixel;
		}
	}
}

void sprite_BlitFlippedHorizontally(const sprite_t *source, sprite_t *destination, int x, int y) {
	int left, right, bottom, top, w;
	left    = MAX(0, x);
	right   = MIN(destination->w-1, x+source->w-1);
	bottom  = MAX(0, y);
	top     = MIN(destination->h-1, y+source->h-1);
	w = source->w-1;
	for(int y2 = bottom; y2 <= top; ++y2) {
		for(int x2 = left; x2 <= right; ++x2) {
			uint8_t source_pixel = source->p[w - (x2-x) + (y2-y)*source->w];
			if (source_pixel != 0)
				destination->p[x2 + y2*destination->w] = source_pixel;
		}
	}
}

void sprite_BlitFlippedVertically(const sprite_t *source, sprite_t *destination, int x, int y) {
	int left, right, bottom, top, h;
	left    = MAX(0, x);
	right   = MIN(destination->w-1, x+source->w-1);
	bottom  = MAX(0, y);
	top     = MIN(destination->h-1, y+source->h-1);
	h = source->h-1;
	for(int y2 = bottom; y2 <= top; ++y2) {
		for(int x2 = left; x2 <= right; ++x2) {
			uint8_t source_pixel = source->p[x2-x + (h-(y2-y))*source->w];
			if (source_pixel != 0)
				destination->p[x2 + y2*destination->w] = source_pixel;
		}
	}
}

void sprite_BlitColor(const sprite_t *source, sprite_t *destination, int x, int y, uint8_t color) {
	int left, right, bottom, top;
	left    = MAX(0, x);
	right   = MIN(destination->w-1, x+source->w-1);
	bottom  = MAX(0, y);
	top     = MIN(destination->h-1, y+source->h-1);
	for(int y2 = bottom; y2 <= top; ++y2) {
		for(int x2 = left; x2 <= right; ++x2) {
			uint8_t source_pixel = source->p[x2-x + (y2-y)*source->w];
			if (source_pixel != 0)
				destination->p[x2 + y2*destination->w] = color;
		}
	}
}

void sprite_BlitRotated90Color(const sprite_t *source, sprite_t *destination, int x, int y, uint8_t color) {
	int left, right, bottom, top;
	left    = MAX(0, x);
	right   = MIN(destination->w-1, x+source->h-1);
	bottom  = MAX(0, y);
	top     = MIN(destination->h-1, y+source->w-1);
	for(int y2 = bottom; y2 <= top; ++y2) {
		for(int x2 = left; x2 <= right; ++x2) {
			uint8_t source_pixel = source->p[y2-y + (x2-x)*source->w];
			if (source_pixel != 0)
				destination->p[x2 + y2*destination->w] = color;
		}
	}
}

void sprite_BlitRotated180Color(const sprite_t *source, sprite_t *destination, int x, int y, uint8_t color) {
	int left, right, bottom, top, w, h;
	left    = MAX(0, x);
	right   = MIN(destination->w-1, x+source->w-1);
	bottom  = MAX(0, y);
	top     = MIN(destination->h-1, y+source->h-1);
	w = source->w-1;
	h = source->h-1;
	for(int y2 = bottom; y2 <= top; ++y2) {
		for(int x2 = left; x2 <= right; ++x2) {
			uint8_t source_pixel = source->p[w - (x2-x) + (h-(y2-y))*source->w];
			if (source_pixel != 0)
				destination->p[x2 + y2*destination->w] = color;
		}
	}
}

void sprite_BlitRotated270Color(const sprite_t *source, sprite_t *destination, int x, int y, uint8_t color) {
	int left, right, bottom, top, w, h;
	left    = MAX(0, x);
	right   = MIN(destination->w-1, x+source->h-1);
	bottom  = MAX(0, y);
	top     = MIN(destination->h-1, y+source->w-1);
	w = source->w-1;
	h = source->h-1;
	for(int y2 = bottom; y2 <= top; ++y2) {
		for(int x2 = left; x2 <= right; ++x2) {
			uint8_t source_pixel = source->p[h-(y2-y) + (w-(x2-x))*source->w];
			if (source_pixel != 0)
				destination->p[x2 + y2*destination->w] = color;
		}
	}
}

void sprite_BlitFlippedHorizontallyColor(const sprite_t *source, sprite_t *destination, int x, int y, uint8_t color) {
	int left, right, bottom, top, w;
	left    = MAX(0, x);
	right   = MIN(destination->w-1, x+source->w-1);
	bottom  = MAX(0, y);
	top     = MIN(destination->h-1, y+source->h-1);
	w = source->w-1;
	for(int y2 = bottom; y2 <= top; ++y2) {
		for(int x2 = left; x2 <= right; ++x2) {
			uint8_t source_pixel = source->p[w - (x2-x) + (y2-y)*source->w];
			if (source_pixel != 0)
				destination->p[x2 + y2*destination->w] = color;
		}
	}
}

void sprite_BlitFlippedVerticallyColor(const sprite_t *source, sprite_t *destination, int x, int y, uint8_t color) {
	int left, right, bottom, top, h;
	left    = MAX(0, x);
	right   = MIN(destination->w-1, x+source->w-1);
	bottom  = MAX(0, y);
	top     = MIN(destination->h-1, y+source->h-1);
	h = source->h-1;
	for(int y2 = bottom; y2 <= top; ++y2) {
		for(int x2 = left; x2 <= right; ++x2) {
			uint8_t source_pixel = source->p[x2-x + (h-(y2-y))*source->w];
			if (source_pixel != 0)
				destination->p[x2 + y2*destination->w] = color;
		}
	}
}

void sprite_SampleRotated(const sprite_t *source, sprite_t *destination, int x, int y, float angle, float originx, float originy) {
	float sin_angle = sin_turns(angle);
	float cos_angle = cos_turns(angle);
	float sin_bangle = sin_turns(angle + 0.25f);
	float cos_bangle = cos_turns(angle + 0.25f);

	float l = -originx;
	float r = source->w-originx;
	float b = -originy;
	float t = source->h-originy;

	int corners[] = {
		(int)(l*cos_angle - b*sin_angle + x),
		(int)(l*sin_angle + b*cos_angle + y),

		(int)(l*cos_angle - t*sin_angle + x),
		(int)(l*sin_angle + t*cos_angle + y),

		(int)(r*cos_angle - b*sin_angle + x),
		(int)(r*sin_angle + b*cos_angle + y),

		(int)(r*cos_angle - t*sin_angle + x),
		(int)(r*sin_angle + t*cos_angle + y),
	};

	int left =   MIN(destination->w-1, MAX(0, MIN(corners[0], MIN(corners[2], MIN(corners[4], corners[6])))  ));
	int right =  MIN(destination->w-1, MAX(0, MAX(corners[0], MAX(corners[2], MAX(corners[4], corners[6])))+1));
	int bottom = MIN(destination->h-1, MAX(0, MIN(corners[1], MIN(corners[3], MIN(corners[5], corners[7])))  ));
	int top =    MIN(destination->h-1, MAX(0, MAX(corners[1], MAX(corners[3], MAX(corners[5], corners[7])))+1));

	int tx, ty;
	int spritex, spritey;
	int sourcex, sourcey;
	uint8_t pixel;
	uint32_t destination_pixel_index;

	for(ty = bottom; ty <= top; ty++){
		for(tx = left; tx <= right; tx++){
			spritex = tx-x;
			spritey = ty-y;
			sourcex = (int)(spritex*sin_bangle - spritey*cos_bangle + originx + 1) - 1;
			sourcey = (int)(spritex*cos_bangle + spritey*sin_bangle + originy + 1) - 1;
			if (sourcex < 0 || sourcex > source->w-1 || sourcey < 0 || sourcey > source->h-1
			 || (pixel = source->p[sourcex + sourcey * source->w])  == 0) continue;
			destination_pixel_index = tx + ty * destination->w;
			destination->p[destination_pixel_index] = pixel;
		}
	}
}

void sprite_SampleRotatedColor(const sprite_t *source, sprite_t *destination, int x, int y, float angle, float originx, float originy, uint8_t color) {
	float sin_angle = sin_turns(angle);
	float cos_angle = cos_turns(angle);
	float sin_bangle = sin_turns(angle + 0.25f);
	float cos_bangle = cos_turns(angle + 0.25f);

	float l = -originx;
	float r = source->w-originx;
	float b = -originy;
	float t = source->h-originy;

	int corners[] = {
		(int)(l*cos_angle - b*sin_angle + x),
		(int)(l*sin_angle + b*cos_angle + y),

		(int)(l*cos_angle - t*sin_angle + x),
		(int)(l*sin_angle + t*cos_angle + y),

		(int)(r*cos_angle - b*sin_angle + x),
		(int)(r*sin_angle + b*cos_angle + y),

		(int)(r*cos_angle - t*sin_angle + x),
		(int)(r*sin_angle + t*cos_angle + y),
	};

	int left =   MIN(destination->w-1, MAX(0, MIN(corners[0], MIN(corners[2], MIN(corners[4], corners[6])))  ));
	int right =  MIN(destination->w-1, MAX(0, MAX(corners[0], MAX(corners[2], MAX(corners[4], corners[6])))+1));
	int bottom = MIN(destination->h-1, MAX(0, MIN(corners[1], MIN(corners[3], MIN(corners[5], corners[7])))  ));
	int top =    MIN(destination->h-1, MAX(0, MAX(corners[1], MAX(corners[3], MAX(corners[5], corners[7])))+1));

	int tx, ty;
	int spritex, spritey;
	int sourcex, sourcey;
	uint8_t pixel;
	uint32_t destination_pixel_index;

	for(ty = bottom; ty <= top; ty++){
		for(tx = left; tx <= right; tx++){
			spritex = tx-x;
			spritey = ty-y;
			sourcex = (int)(spritex*sin_bangle - spritey*cos_bangle + originx + 1) - 1;
			sourcey = (int)(spritex*cos_bangle + spritey*sin_bangle + originy + 1) - 1;
			if (sourcex < 0 || sourcex > source->w-1 || sourcey < 0 || sourcey > source->h-1
			 || (pixel = source->p[sourcex + sourcey * source->w]) == 0) continue;
			destination_pixel_index = tx + ty * destination->w;
			destination->p[destination_pixel_index] = color;
		}
	}
}

void sprite_SampleRotatedFlipX (const sprite_t *source, sprite_t *destination, int x, int y, float angle, float originx, float originy) {
	float sin_angle = sin_turns(angle);
	float cos_angle = cos_turns(angle);
	float sin_bangle = sin_turns(angle + 0.25f);
	float cos_bangle = cos_turns(angle + 0.25f);

	float l = -originx;
	float r = source->w-originx;
	float b = -originy;
	float t = source->h-originy;

	int corners[] = {
		(int)(l*cos_angle - b*sin_angle + x),
		(int)(l*sin_angle + b*cos_angle + y),

		(int)(l*cos_angle - t*sin_angle + x),
		(int)(l*sin_angle + t*cos_angle + y),

		(int)(r*cos_angle - b*sin_angle + x),
		(int)(r*sin_angle + b*cos_angle + y),

		(int)(r*cos_angle - t*sin_angle + x),
		(int)(r*sin_angle + t*cos_angle + y),
	};

	int left =   MIN(destination->w-1, MAX(0, MIN(corners[0], MIN(corners[2], MIN(corners[4], corners[6])))  ));
	int right =  MIN(destination->w-1, MAX(0, MAX(corners[0], MAX(corners[2], MAX(corners[4], corners[6])))+1));
	int bottom = MIN(destination->h-1, MAX(0, MIN(corners[1], MIN(corners[3], MIN(corners[5], corners[7])))  ));
	int top =    MIN(destination->h-1, MAX(0, MAX(corners[1], MAX(corners[3], MAX(corners[5], corners[7])))+1));

	int tx, ty;
	int spritex, spritey;
	int sourcex, sourcey;
	uint8_t pixel;
	uint32_t destination_pixel_index;

	for(ty = bottom; ty <= top; ty++){
		for(tx = left; tx <= right; tx++){
			spritex = tx-x;
			spritey = ty-y;
			sourcex = source->w-1 - ((int)(spritex*sin_bangle - spritey*cos_bangle + originx + 1) - 1);
			sourcey =               (int)(spritex*cos_bangle + spritey*sin_bangle + originy + 1) - 1;
			if (sourcex < 0 || sourcex > source->w-1 || sourcey < 0 || sourcey > source->h-1
			 || (pixel = source->p[sourcex + sourcey * source->w])  == 0) continue;
			destination_pixel_index = tx + ty * destination->w;
			destination->p[destination_pixel_index] = pixel;
		}
	}
}

void sprite_SampleRotatedFlipXColor (const sprite_t *source, sprite_t *destination, int x, int y, float angle, float originx, float originy, uint8_t color) {
	float sin_angle = sin_turns(angle);
	float cos_angle = cos_turns(angle);
	float sin_bangle = sin_turns(angle + 0.25f);
	float cos_bangle = cos_turns(angle + 0.25f);

	float l = -originx;
	float r = source->w-originx;
	float b = -originy;
	float t = source->h-originy;

	int corners[] = {
		(int)(l*cos_angle - b*sin_angle + x),
		(int)(l*sin_angle + b*cos_angle + y),

		(int)(l*cos_angle - t*sin_angle + x),
		(int)(l*sin_angle + t*cos_angle + y),

		(int)(r*cos_angle - b*sin_angle + x),
		(int)(r*sin_angle + b*cos_angle + y),

		(int)(r*cos_angle - t*sin_angle + x),
		(int)(r*sin_angle + t*cos_angle + y),
	};

	int left =   MIN(destination->w-1, MAX(0, MIN(corners[0], MIN(corners[2], MIN(corners[4], corners[6])))  ));
	int right =  MIN(destination->w-1, MAX(0, MAX(corners[0], MAX(corners[2], MAX(corners[4], corners[6])))+1));
	int bottom = MIN(destination->h-1, MAX(0, MIN(corners[1], MIN(corners[3], MIN(corners[5], corners[7])))  ));
	int top =    MIN(destination->h-1, MAX(0, MAX(corners[1], MAX(corners[3], MAX(corners[5], corners[7])))+1));

	int tx, ty;
	int spritex, spritey;
	int sourcex, sourcey;
	uint8_t pixel;
	uint32_t destination_pixel_index;

	for(ty = bottom; ty <= top; ty++){
		for(tx = left; tx <= right; tx++){
			spritex = tx-x;
			spritey = ty-y;
			sourcex = source->w-1 - ((int)(spritex*sin_bangle - spritey*cos_bangle + originx + 1) - 1);
			sourcey =               (int)(spritex*cos_bangle + spritey*sin_bangle + originy + 1) - 1;
			if (sourcex < 0 || sourcex > source->w-1 || sourcey < 0 || sourcey > source->h-1
			 || (pixel = source->p[sourcex + sourcey * source->w])  == 0) continue;
			destination_pixel_index = tx + ty * destination->w;
			destination->p[destination_pixel_index] = color;
		}
	}
}

void sprite_SampleRotatedFlipY (const sprite_t *source, sprite_t *destination, int x, int y, float angle, float originx, float originy) {
	float sin_angle = sin_turns(angle);
	float cos_angle = cos_turns(angle);

	float l = -originx;
	float r = source->w-originx;
	float b = -originy;
	float t = source->h-originy;

	int corners[] = {
		(int)(l*cos_angle - b*sin_angle + x),
		(int)(l*sin_angle + b*cos_angle + y),

		(int)(l*cos_angle - t*sin_angle + x),
		(int)(l*sin_angle + t*cos_angle + y),

		(int)(r*cos_angle - b*sin_angle + x),
		(int)(r*sin_angle + b*cos_angle + y),

		(int)(r*cos_angle - t*sin_angle + x),
		(int)(r*sin_angle + t*cos_angle + y),
	};

	int left =   MIN(destination->w-1, MAX(0, MIN(corners[0], MIN(corners[2], MIN(corners[4], corners[6])))  ));
	int right =  MIN(destination->w-1, MAX(0, MAX(corners[0], MAX(corners[2], MAX(corners[4], corners[6])))+1));
	int bottom = MIN(destination->h-1, MAX(0, MIN(corners[1], MIN(corners[3], MIN(corners[5], corners[7])))  ));
	int top =    MIN(destination->h-1, MAX(0, MAX(corners[1], MAX(corners[3], MAX(corners[5], corners[7])))+1));

	int tx, ty;
	int spritex, spritey;
	int sourcex, sourcey;
	uint8_t pixel;
	uint32_t destination_pixel_index;
	float sin_bangle = sin_turns(angle + 0.25f);
	float cos_bangle = cos_turns(angle + 0.25f);

	for(ty = bottom; ty <= top; ty++){
		for(tx = left; tx <= right; tx++){
			spritex = tx-x;
			spritey = ty-y;
			sourcex =               (int)(spritex*sin_bangle - spritey*cos_bangle + originx + 1) - 1;
			sourcey = source->h-1 - ((int)(spritex*cos_bangle + spritey*sin_bangle + originy + 1) - 1);
			if (sourcex < 0 || sourcex > source->w-1 || sourcey < 0 || sourcey > source->h-1
			 || (pixel = source->p[sourcex + sourcey * source->w])  == 0) continue;
			destination_pixel_index = tx + ty * destination->w;
			destination->p[destination_pixel_index] = pixel;
		}
	}
}

void sprite_SampleRotatedFlipYColor (const sprite_t *source, sprite_t *destination, int x, int y, float angle, float originx, float originy, uint8_t color) {
	float sin_angle = sin_turns(angle);
	float cos_angle = cos_turns(angle);

	float l = -originx;
	float r = source->w-originx;
	float b = -originy;
	float t = source->h-originy;

	int corners[] = {
		(int)(l*cos_angle - b*sin_angle + x),
		(int)(l*sin_angle + b*cos_angle + y),

		(int)(l*cos_angle - t*sin_angle + x),
		(int)(l*sin_angle + t*cos_angle + y),

		(int)(r*cos_angle - b*sin_angle + x),
		(int)(r*sin_angle + b*cos_angle + y),

		(int)(r*cos_angle - t*sin_angle + x),
		(int)(r*sin_angle + t*cos_angle + y),
	};

	int left =   MIN(destination->w-1, MAX(0, MIN(corners[0], MIN(corners[2], MIN(corners[4], corners[6])))  ));
	int right =  MIN(destination->w-1, MAX(0, MAX(corners[0], MAX(corners[2], MAX(corners[4], corners[6])))+1));
	int bottom = MIN(destination->h-1, MAX(0, MIN(corners[1], MIN(corners[3], MIN(corners[5], corners[7])))  ));
	int top =    MIN(destination->h-1, MAX(0, MAX(corners[1], MAX(corners[3], MAX(corners[5], corners[7])))+1));

	int tx, ty;
	int spritex, spritey;
	int sourcex, sourcey;
	uint8_t pixel;
	uint32_t destination_pixel_index;
	float sin_bangle = sin_turns(angle + 0.25f);
	float cos_bangle = cos_turns(angle + 0.25f);

	for(ty = bottom; ty <= top; ty++){
		for(tx = left; tx <= right; tx++){
			spritex = tx-x;
			spritey = ty-y;
			sourcex =               (int)(spritex*sin_bangle - spritey*cos_bangle + originx + 1) - 1;
			sourcey = source->h-1 - ((int)(spritex*cos_bangle + spritey*sin_bangle + originy + 1) - 1);
			if (sourcex < 0 || sourcex > source->w-1 || sourcey < 0 || sourcey > source->h-1
			 || (pixel = source->p[sourcex + sourcey * source->w])  == 0) continue;
			destination_pixel_index = tx + ty * destination->w;
			destination->p[destination_pixel_index] = color;
		}
	}
}

typedef enum {sprite_flip_none, sprite_flip_y, sprite_flip_x, sprite_flip_both} sprite_flip_e;

void sprite_SampleRotatedFlipped(const sprite_t *source, sprite_t *destination, int x, int y, float angle, float originx, float originy, bool flipx, bool flipy) {
	sprite_flip_e flip_state = (flipx ? 2 : 0) | (flipy ? 1 : 0);
	switch (flip_state) {
		case 0b00: sprite_SampleRotated (source, destination, x, y, angle, originx, originy); break;
		case 0b11: sprite_SampleRotated (source, destination, x, y, angle + 0.5f, originx, originy); break;
		case 0b01: sprite_SampleRotatedFlipY (source, destination, x, y, angle, originx, originy); break;
		case 0b10: sprite_SampleRotatedFlipX (source, destination, x, y, angle, originx, originy); break;
	}
	return;
}

void sprite_SampleRotatedFlippedColor(const sprite_t *source, sprite_t *destination, int x, int y, float angle, float originx, float originy, bool flipx, bool flipy, uint8_t color) {
	sprite_flip_e flip_state = (flipx ? 2 : 0) | (flipy ? 1 : 0);
	switch (flip_state) {
		case 0b00: sprite_SampleRotatedColor (source, destination, x, y, angle, originx, originy, color); break;
		case 0b11: sprite_SampleRotatedColor (source, destination, x, y, angle + 0.5f, originx, originy, color); break;
		case 0b01: sprite_SampleRotatedFlipYColor (source, destination, x, y, angle, originx, originy, color); break;
		case 0b10: sprite_SampleRotatedFlipXColor (source, destination, x, y, angle, originx, originy, color); break;
	}
	return;
}

uint8_t sprite_SamplePixel_ (sprite_SamplePixel_arguments arguments) {
	sprite_flip_e flip = (arguments.flipx ? 2 : 0) | (arguments.flipy ? 1 : 0);
	int sx = 0, sy = 0;
	int sax = arguments.samplerx, say = arguments.samplery;
	int ox = arguments.originx, oy = arguments.originy;
	int spw = arguments.sprite->w, sph = arguments.sprite->h;
	float saa = -arguments.rotation;
	if (arguments.rotation == 0) {
		int spx = arguments.spritex - ox, spy = arguments.spritey - oy;
		switch (flip) {
			case sprite_flip_none: {
				sx = sax - spx;
				sy = say - spy;
			} break;
			case sprite_flip_both: {
				sx = (spw-1) - (sax - spx);
				sy = (sph-1) - (say - spy);
			} break;
			case sprite_flip_x: {
				sx = (spw-1) - (sax - spx);
				sy =            say - spy;
			} break;
			case sprite_flip_y: {
				sx =            sax - spx;
				sy = (sph-1) - (say - spy);
			} break;
		}
	}
	else { // rotation != 0
		int spx = arguments.spritex, spy = arguments.spritey;
		float sina = sin_turns (saa);
		float cosa = cos_turns (saa);
		int x = sax - spx;
		int y = say - spy;
		sx = x*cosa - y*sina + ox;
		sy = x*sina + y*cosa + oy;
		switch (flip) {
			case sprite_flip_none: break;
			case sprite_flip_both: {
				sx = (spw-1) - sx;
				sy = (sph-1) - sy;
			} break;
			case sprite_flip_y: sy = (sph-1) - sy; break;
			case sprite_flip_x: sx = (spw-1) - sx; break;
		}
	}
	if (sx < 0 || sx > spw-1 || sy < 0 || sy > sph-1) return 0;
	return arguments.sprite->p[sx + sy * spw];
}