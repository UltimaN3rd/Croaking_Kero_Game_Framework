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
#include "utilities.h"
#include "sprite.h"
#include "framework_types.h"

typedef struct __attribute__((__packed__)) {
	vec2i16_t position;
    int16_t originx, originy;
	const sprite_t *sprite;
	float rotation;
	struct __attribute__((__packed__)) {
	    bool flip_horizontally : 1;
	    bool flip_vertically : 1;
        bool center_horizontally : 1;
        bool center_vertically : 1;
		unsigned int rotation_by_quarters : 2;
    } flags;
} render_state_sprite_t;

typedef struct __attribute__((__packed__)) {
	enum : uint8_t { render_shape_rectangle, render_shape_circle, render_shape_line, render_shape_dot, render_shape_triangle, render_shape_ellipse } type;
	union __attribute__((__packed__)) {
		struct __attribute__((__packed__)) {
			int16_t x, y, w, h;
			struct {
				bool center_horizontally:1;
				bool center_vertically:1;
			} flags;
			uint8_t color_edge, color_fill;
		} rectangle;
		struct __attribute__((__packed__)) {
			int16_t x, y, r;
			uint8_t color_edge, color_fill;
		} circle;
		struct __attribute__((__packed__)) {
			int16_t x, y, rx, ry;
			uint8_t color_edge, color_fill;
		} ellipse;
		struct __attribute__((__packed__)) {
			int16_t x0, y0, x1, y1;
			uint8_t color;
		} line;
		struct __attribute__((__packed__)) {
			int16_t x, y;
			uint8_t color;
		} dot;
		struct __attribute__((__packed__)) {
			int16_t x0, y0, x1, y1, x2, y2;
			struct __attribute__((__packed__)) {
				bool center_horizontally:1;
				bool center_vertically:1;
			} flags;
			uint8_t color_edge, color_fill;
		} triangle;
	};
} render_shape_t;

// Packed SOA to save mem may be better than AOS because pos/pixel are always accessed together
typedef struct {
	vec2i16_t position;
	uint8_t pixel;
} __attribute__((__packed__)) render_state_particle_t;

typedef struct render_state_s {
	volatile bool busy;
	uint64_t state_count;
	struct __attribute__((__packed__)) {
		struct {
			enum : uint8_t {render_element_sprite, render_element_shape, render_element_text, render_element_sprite_silhouette, render_element_darkness_rectangle} type : 3;
			bool ignore_camera : 1;
		};
		int8_t depth;
		union {
			render_state_sprite_t sprite;
			render_shape_t shape;
			struct __attribute__((__packed__)) {
				char *string;
				int16_t x, y, length;
			} text;
			struct __attribute__((__packed__)) {
				render_state_sprite_t sprite;
				uint8_t color;
			} sprite_silhouette;
			struct __attribute__((__packed__)) {
				int16_t l, b, r, t;
				uint8_t levels : 3; // Maximum value of 7
			} darkness_rectangle;
		};
		#define RENDER_MAX_ELEMENTS 4096
	} elements[RENDER_MAX_ELEMENTS];
	int32_t element_count;
	#define RENDER_STATE_MEM_AMOUNT UINT16_MAX
	struct {
		uint16_t position;
		char bytes[RENDER_STATE_MEM_AMOUNT];
	} mem;
	#define PARTICLES_MAX 4096
	struct {
		int16_t count;
		render_state_particle_t array[PARTICLES_MAX];
	} particles;
	struct {
		int x, y;
	} camera;
	// #define RENDER_TEXT_MAX_BYTES 4096
	// char text[RENDER_TEXT_MAX_BYTES]; // A series of strings formatted like so: {uint16_t length, uint16_t x, uint16_t y, "null-terminated string" [if not aligned to a word, skip a byte]} repeat until length = 0
	struct {
		enum { background_type_none, background_type_blank, background_type_stripes, background_type_checkers, background_type_sprite } type;
		union {
			struct {
				uint8_t color;
			} blank;
			struct {
				uint8_t color;
				int width;
				float angle;
			} stripes;
			struct {
				uint8_t color;
				int width, height;
				int x, y;
			} checkers;
			sprite_t *sprite;
		};
	} background;
	struct {
		const sprite_t *sprite;
		int x, y, offsetx, offsety;
	} cursor;
	struct {
		bool show_rendertime, show_framerate;
	} debug;
} render_state_t;

#include "framework.h"

typedef struct render_data_s {
	bool thread_initialized;
	render_state_t render_states[3];
	sprite_t *frame[2];
	sprite_t *level;
	volatile bool pause_thread, resume_thread; // The render thread will pause at the start of an iteration. To use these:
} render_data_t;

void *Render(void*);

void Render_SelectStateToEdit ();
void Render_FinishEditingState ();

void Render_Camera (int x, int y);

typedef struct {
	int8_t depth;
    int x, y;
	const sprite_t *sprite;
    float rotation;
    int originx, originy;
	bool ignore_camera;
	typeof ((render_state_sprite_t){}.flags) sprite_flags;
} Render_Sprite_arguments;
#define Render_Sprite(...) Render_Sprite_((Render_Sprite_arguments){__VA_ARGS__})
void Render_Sprite_ (Render_Sprite_arguments arguments);

#define Render_SpriteSilhouette(__color__, ...) Render_SpriteSilhouette_ (__color__, (Render_Sprite_arguments){__VA_ARGS__})
void Render_SpriteSilhouette_ (uint8_t color, Render_Sprite_arguments arguments);

typedef struct {
    int8_t depth;
	bool ignore_camera;
    render_shape_t shape;
} Render_Shape_arguments;
#define Render_Shape(...) Render_Shape_((Render_Shape_arguments){__VA_ARGS__})
void Render_Shape_ (Render_Shape_arguments arguments);

typedef struct {
    int8_t depth;
    int x, y, length;
    const char *string;
	bool ignore_camera;
	bool center_horizontally_on_screen;
	bool center_vertically_on_screen;
	uint8_t translucent_background_darkness : 4; // Max value 7
} Render_Text_arguments;
#define Render_Text(...) Render_Text_((Render_Text_arguments){.y = 20, __VA_ARGS__})
void Render_Text_ (Render_Text_arguments arguments);

typedef typeof((render_state_t){}.background) Render_Background_arguments;
#define Render_Background(...) Render_Background_ ((Render_Background_arguments){__VA_ARGS__})
void Render_Background_ (Render_Background_arguments background);

void Render_Particle (int x, int y, uint8_t pixel, bool ignore_camera);

render_state_t *Render_GetCurrentEditableState ();

void Render_Cursor (const cursor_t *cursor, int x, int y);
void Render_CursorAtRawMousePos (const cursor_t *cursor);

typedef struct {
	int16_t w, h;
} font_StringDimensions_return_t;
font_StringDimensions_return_t font_StringDimensions (const font_t *font, const char *text);

// Must be called after any call to Render_Camera in order to take effect
void Render_ScreenShake (int x, int y);

void Render_ShowRenderTime (bool show);
void Render_ShowFPS (bool show);

typedef struct {
	int16_t l, b, r, t;
	int8_t depth;
	uint8_t levels; // Capped at 7
} Render_DarkenRectangle_arguments_t;
#define Render_DarkenRectangle(...) Render_DarkenRectangle_ ((Render_DarkenRectangle_arguments_t){.l = 0, .b = 0, .r = RESOLUTION_WIDTH-1, .t = RESOLUTION_HEIGHT-1, .levels = 1, .depth = 0, __VA_ARGS__})
void Render_DarkenRectangle_ (Render_DarkenRectangle_arguments_t args);

void Render_Screenshot (sprite_t *destination);