#pragma once

#include <stdint.h>

typedef struct {
	uint16_t w, h;
	uint8_t p[];
} sprite_t;

#include "sound.h"
#include "samples.h"

#define BITMAP_FONT_FIRST_VISIBLE_CHAR 33
#define BITMAP_FONT_LAST_VISIBLE_CHAR 126
#define BITMAP_FONT_NUM_VISIBLE_CHARS (BITMAP_FONT_LAST_VISIBLE_CHAR - BITMAP_FONT_FIRST_VISIBLE_CHAR + 1)

typedef struct {
    int8_t line_height, baseline;
    uint8_t space_width;
    const sprite_t *bitmaps[BITMAP_FONT_NUM_VISIBLE_CHARS];
    int8_t descent[BITMAP_FONT_NUM_VISIBLE_CHARS];
} font_t;

extern const uint8_t palette[256][3];
extern const sprite_t resources_menu_cursor;
extern const sprite_t resources_menu_arrow7;
extern const sprite_t resources_menu_folder_open;
extern const sprite_t resources_menu_back;
extern const sprite_t resources_gameplay_pipe_body;
extern const sprite_t resources_gameplay_coin;
extern const sprite_t resources_gameplay_pipe_top;
extern const sprite_t resources_gameplay_heli;
extern const font_t resources_font;
extern const sound_music_t resources_music_ost1;
