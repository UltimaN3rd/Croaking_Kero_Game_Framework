#include "sprite.h"

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

void sprite_RotateByShear(const sprite_t *source, sprite_t *destination, int destinationx, int destinationy, int originx, int originy, float angle);

void sprite_SampleRotated(const sprite_t *source, sprite_t *destination, int x, int y, float angle, float originx, float originy) {
	static bool shear = true;
	if (shear) {
		sprite_RotateByShear (source, destination, x, y, originx, originy, angle);
		return;
	}
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

	// for (int i = left; i <= right; ++i) {
	// 	destination->p[i + bottom*destination->w].u32 = -1;
	// 	destination->p[i + top*destination->w].u32 = -1;
	// }
	// for (int i = bottom; i <= top; ++i) {
	// 	destination->p[left + i*destination->w].u32 = -1;
	// 	destination->p[right + i*destination->w].u32 = -1;
	// }

	int tx, ty;
	int spritex, spritey;
	int sourcex, sourcey;
	uint8_t pixel;
// 	float alpha;
// 	uint8_t red, green, blue;
// 	uint32_t destination_pixel;
	uint32_t destination_pixel_index;
// 	float anti_alpha;

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
			// Below is alpha blending. Removed for now to only use binary alpha
			/*else {
			 *				alpha = ((pixel & 0xff000000) >> 24) / 255.f;
			 *				blue =  (pixel  & 0x000000ff)        * alpha;
			 *				green = ((pixel & 0x0000ff00) >> 8)  * alpha;
			 *				red =   ((pixel & 0x00ff0000) >> 16) * alpha;
			 *				anti_alpha = 1.f - alpha;
			 *				destination_pixel = destination->p[destination_pixel_index];
			 *				destination_pixel = (((destination_pixel & 0x00ff0000) >> 16) * anti_alpha + red) +
			 *				(((destination_pixel & 0x0000ff00) >>  8) * anti_alpha + green) +
			 *				(  destination_pixel & 0x000000ff)        * anti_alpha + blue;
		}*/
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

	// for (int i = left; i <= right; ++i) {
	// 	destination->p[i + bottom*destination->w].u32 = -1;
	// 	destination->p[i + top*destination->w].u32 = -1;
	// }
	// for (int i = bottom; i <= top; ++i) {
	// 	destination->p[left + i*destination->w].u32 = -1;
	// 	destination->p[right + i*destination->w].u32 = -1;
	// }

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

	// for (int i = left; i <= right; ++i) {
	// 	destination->p[i + bottom*destination->w].u32 = -1;
	// 	destination->p[i + top*destination->w].u32 = -1;
	// }
	// for (int i = bottom; i <= top; ++i) {
	// 	destination->p[left + i*destination->w].u32 = -1;
	// 	destination->p[right + i*destination->w].u32 = -1;
	// }

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

void sprite_RotateByShear(const sprite_t *source, sprite_t *destination, int destinationx, int destinationy, int originx, int originy, float angle) {
	angle = angle - (int)angle;
	float absangle = fabs (angle);

	if(absangle < 0.000002f) { // Rotation is so small that errors will emerge. Just do non-rotated blit and early out. Value found through experimentation. May need adjustment
		sprite_Blit(source, destination, destinationx - originx, destinationy - originy);
		return;
	}

	const uint8_t *source_pixel = source->p;
	int pixel_direction = 1;

	uint16_t w, h, dw, dh;
	w = source->w;
	h = source->h;
	dw = destination->w;
	dh = destination->h;

	if(absangle > .25f) {
		source_pixel += w * h - 1;
		pixel_direction = -1;
		if(angle > .25f) angle -= .5f;
		if(angle < -.25f) angle += .5f;
		originx = w-1 - originx;
		originy = h-1 - originy;
	}

	float skewx = -tan_turns (angle/2);
	float skewy = sin_turns (angle);

	int rowx;
	int newx, newy;

	int skewed_originx = originx + originy * skewx;
	int skewed_originy = originy + skewed_originx * skewy;
	skewed_originx += skewed_originy * skewx;

	// Copy sprite from source to destination, triple skewing each pixel
	for(int sy = 0; sy < h; ++sy) {
		rowx = sy * skewx;
		for(int sx = 0; sx < w; ++sx, source_pixel += pixel_direction) {
			uint8_t p = *source_pixel;
			if(p == 0) continue;
			newx = sx + rowx;
			newy = sy + newx*skewy;
			newx += newy * skewx;
			newx += -skewed_originx + destinationx;
			newy += -skewed_originy + destinationy;
			if (newx < 0 || newx > dw-1 ||
				newy < 0 || newy > dh-1) continue;
			destination->p[newx + newy*dw] = p;
		}
	}
}

// void SpriteRotateByShear(sprite_t source, sprite_t destination, int destinationx, int destinationy, int originx, int originy, float r) {
// 	r = fmod(r + PI, TWOPI);
// 	if(r < 0) r += TWOPI;
// 	r -= PI;

// 	if(r < 0.000002f && r > -0.000001f) { // Rotation is so small that errors will emerge. Just do non-rotated blit and early out. Values found through experimentation. May need adjustment
// 		SpriteBlitAlpha(source, destination, destinationx - originx, destinationy - originy);
// 		return;
// 	}

// 	pixel_t *source_pixel = source.p;
// 	int pixel_direction = 1;

// 	if(r > HALFPI || r < -HALFPI) {
// 		source_pixel += source.w * source.h - 1;
// 		pixel_direction = -1;
// 		if(r > HALFPI) r -= PI;
// 		if(r < -HALFPI) r += PI;
// 		originx = source.w-1 - originx;
// 		originy = source.h-1 - originy;
// 	}

// 	float skewx = -tanf(r/2);
// 	float skewy = sinf(r);

// 	int rowx;
// 	int newx, newy;

// 	int skewed_originx = originx + originy * skewx;
// 	int skewed_originy = originy + skewed_originx * skewy;
// 	skewed_originx += skewed_originy * skewx;

// 	// Copy sprite from source to destination, triple skewing each pixel
// 	for(int sy = 0; sy < source.h; ++sy) {
// 		rowx = sy * skewx;
// 		for(int sx = 0; sx < source.w; ++sx, source_pixel += pixel_direction) {
// 			if(source_pixel->a != 255) continue;
// 			newx = sx + rowx;
// 			newy = sy + newx*skewy;
// 			newx += newy * skewx;
// 			newx += -skewed_originx + destinationx;
// 			newy += -skewed_originy + destinationy;
// 			if (newx < 0 || newx > destination.w-1 ||
// 				newy < 0 || newy > destination.h-1) continue;
// 			destination.p[newx + newy*destination.w] = *source_pixel;
// 		}
// 	}
// }