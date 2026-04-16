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

typedef struct [[gnu::packed]] {
	v2i16 position;
    i16 originx, originy;
	const sprite_t *sprite;
	f32 rotation;
	struct [[gnu::packed]] {
	    bool flip_horizontally : 1;
	    bool flip_vertically : 1;
        bool center_horizontally : 1;
        bool center_vertically : 1;
		unsigned int rotation_by_quarters : 2;
    } flags;
	const u8 (*color_swap_palette)[256];
} render_state_sprite_t;

typedef struct {
	i16 x, y;
	f32 u, v;
} textured_poly_vertex_t;

typedef struct [[gnu::packed]] {
	enum : u8 { render_shape_rectangle, render_shape_circle, render_shape_line, render_shape_dot, render_shape_triangle, render_shape_ellipse } type;
	union [[gnu::packed]] {
		struct [[gnu::packed]] {
			i16 x, y, w, h;
			struct {
				bool center_horizontally:1;
				bool center_vertically:1;
			} flags;
			u8 color_edge, color_fill;
		} rectangle;
		struct [[gnu::packed]] {
			i16 x, y, r;
			u8 color_edge, color_fill;
		} circle;
		struct [[gnu::packed]] {
			i16 x, y, rx, ry;
			u8 color_edge, color_fill;
		} ellipse;
		struct [[gnu::packed]] {
			i16 x0, y0, x1, y1;
			u8 color;
		} line;
		struct [[gnu::packed]] {
			i16 x, y;
			u8 color;
		} dot;
		struct [[gnu::packed]] {
			i16 x0, y0, x1, y1, x2, y2;
			struct [[gnu::packed]] {
				bool center_horizontally:1;
				bool center_vertically:1;
			} flags;
			u8 color_edge, color_fill;
		} triangle;
	};
} render_shape_t;

// Packed SOA to save mem may be better than AOS because pos/pixel are always accessed together
typedef struct [[gnu::packed]] {
	v2i16 position;
	u8 pixel;
} render_state_particle_t;

typedef struct [[gnu::packed]] {
	struct {
		enum : u8 {render_element_sprite, render_element_shape, render_element_text, render_element_sprite_silhouette, render_element_darkness_rectangle, render_element_textured_poly} type : 3;
		bool ignore_camera : 1;
	};
	i8 depth;
	union {
		render_state_sprite_t sprite;
		render_shape_t shape;
		struct [[gnu::packed]] {
			char *string;
			i16 x, y, length;
		} text;
		struct [[gnu::packed]] {
			render_state_sprite_t sprite;
			u8 color;
		} sprite_silhouette;
		struct [[gnu::packed]] {
			i16 l, b, r, t;
			u8 levels : 3; // Maximum value of 7
		} darkness_rectangle;
		struct [[gnu::packed]] {
			textured_poly_vertex_t *vertices;
			const sprite_t *texture;
			i16 x, y;
			u8 vertex_count;
		} textured_poly;
	};
	#define RENDER_MAX_ELEMENTS 4096
} render_state_element_t;

typedef struct render_state_s {
	volatile bool busy;
	u64 state_count;
	render_state_element_t elements[RENDER_MAX_ELEMENTS];
	i32 element_count;
	#define RENDER_STATE_MEM_AMOUNT UINT16_MAX
	struct {
		u16 position;
		char bytes[RENDER_STATE_MEM_AMOUNT];
	} mem;
	#define PARTICLES_MAX 4096
	struct {
		i16 count;
		render_state_particle_t array[PARTICLES_MAX];
	} particles;
	struct {
		int x, y;
	} camera;
	struct {
		enum { background_type_none, background_type_blank, background_type_stripes, background_type_checkers, background_type_sprite } type;
		union {
			struct {
				u8 color;
			} blank;
			struct {
				u8 color;
				int width;
				f32 angle;
			} stripes;
			struct {
				u8 color;
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
	volatile bool pause_thread, resume_thread; // The render thread will pause at the start of an iteration if pause_thread is set. To use:
											   // resume_thread = false;
											   // pause_thread = true;
											   // Do stuff
											   // resume_thread = true;
} render_data_t;

void *Render(void*);

void Render_SelectStateToEdit ();
void Render_FinishEditingState ();

void Render_Camera (int x, int y);

typedef struct {
	i8 depth;
    int x, y;
	const sprite_t *sprite;
    f32 rotation;
    int originx, originy;
	bool ignore_camera;
	typeof ((render_state_sprite_t){}.flags) sprite_flags;
	const u8 (*const color_swap_palette)[256];
} Render_Sprite_arguments;
#define Render_Sprite(...) Render_Sprite_((Render_Sprite_arguments){__VA_ARGS__})
void Render_Sprite_ (Render_Sprite_arguments arguments);

#define Render_SpriteSilhouette(__color__, ...) Render_SpriteSilhouette_ (__color__, (Render_Sprite_arguments){__VA_ARGS__})
void Render_SpriteSilhouette_ (u8 color, Render_Sprite_arguments arguments);

typedef struct {
    i8 depth;
	bool ignore_camera;
    render_shape_t shape;
} Render_Shape_arguments;
#define Render_Shape(...) Render_Shape_((Render_Shape_arguments){__VA_ARGS__})
void Render_Shape_ (Render_Shape_arguments arguments);

typedef struct [[gnu::packed]] {
	enum {render_text_payload_sprite} tag;
	union {
		struct [[gnu::packed]] {
			const sprite_t *_;
			i16 x, y;
		} sprite;
	} _;
} render_text_payload_t;

typedef struct {
    i8 depth;
    int x, y, length;
    const char *string;
	struct [[gnu::packed]] {
		i16 count;
		const render_text_payload_t *_;
	} payload;
	bool ignore_camera;
	bool center_horizontally_on_screen;
	bool center_vertically_on_screen;
	u8 translucent_background_darkness : 4; // Max value 7
} Render_Text_arguments;
#define Render_Text(...) Render_Text_((Render_Text_arguments){.y = 20, __VA_ARGS__})
void Render_Text_ (Render_Text_arguments arguments);

typedef typeof((render_state_t){}.background) Render_Background_arguments;
#define Render_Background(...) Render_Background_ ((Render_Background_arguments){__VA_ARGS__})
void Render_Background_ (Render_Background_arguments background);

void Render_Particle (int x, int y, u8 pixel, bool ignore_camera);

render_state_t *Render_GetCurrentEditableState ();

void Render_Cursor (const cursor_t *cursor, int x, int y);
void Render_CursorAtRawMousePos (const cursor_t *cursor);

typedef struct {
	i16 w, h;
} font_StringDimensions_return_t;
font_StringDimensions_return_t font_StringDimensions (const font_t *font, const char *text, const render_text_payload_t *payload_ptr);

// Must be called after any call to Render_Camera in order to take effect
void Render_ScreenShake (int x, int y);

void Render_ShowRenderTime (bool show);
void Render_ShowFPS (bool show);

typedef struct {
	i16 l, b, r, t;
	i8 depth;
	u8 levels; // Capped at 7
} Render_DarkenRectangle_arguments_t;
#define Render_DarkenRectangle(...) Render_DarkenRectangle_ ((Render_DarkenRectangle_arguments_t){.l = 0, .b = 0, .r = RESOLUTION_WIDTH-1, .t = RESOLUTION_HEIGHT-1, .levels = 1, .depth = 0, __VA_ARGS__})
void Render_DarkenRectangle_ (Render_DarkenRectangle_arguments_t args);

void Render_Screenshot (sprite_t *destination);

typedef struct {
	i8 depth;
	const sprite_t *texture;
	i16 x, y;
	u8 vertex_count;
	textured_poly_vertex_t *vertices;
} Render_TexturedPoly_arguments_t;
// Call convention: Render_TexturedPoly(.texture = &whatever, .vertex_count = n, .vertices = (textured_poly_vertex_t[]){{your}, {x, y, u, v}, {verts}, {here}}, .otherarguments)
// Vertices MUST be in counter-clockwise order!!!
#define Render_TexturedPoly(...) Render_TexturedPoly_ ((Render_TexturedPoly_arguments_t){__VA_ARGS__})
void Render_TexturedPoly_ (Render_TexturedPoly_arguments_t args);

i16 Render_TextGetPayloadCountFromString (const char *text);