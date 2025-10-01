#pragma once

#include <stdint.h>
#include "utilities.h"
#include "sprite.h"
#include "framework_types.h"

typedef struct {
	vec2i_t position;
	const sprite_t *sprite;
	float rotation;
    int originx, originy;
	struct {
	    bool flip_horizontally : 1;
	    bool flip_vertically : 1;
        bool center_horizontally : 1;
        bool center_vertically : 1;
		unsigned int rotation_by_quarters : 2;
    } flags;
} render_state_sprite_t;

typedef struct {
	enum { render_shape_rectangle, render_shape_circle, render_shape_line, render_shape_dot } type;
	union {
		struct {
			int x, y, w, h;
			struct {
				bool filled:1;
				bool center_horizontally:1;
				bool center_vertically:1;
			} flags;
			uint8_t color_edge, color_fill;
		} rectangle;
		struct {
			int x, y, r;
			bool filled;
			uint8_t color_edge, color_fill;
		} circle;
		struct {
			int x0, y0, x1, y1;
			uint8_t color;
		} line;
		struct {
			int x, y;
			uint8_t color;
		} dot;
	};
} render_shape_t;

typedef struct render_state_s {
	volatile bool busy;
	uint64_t state_count;
	struct {
		enum {render_element_sprite, render_element_shape, render_element_text, render_element_sprite_silhouette} type;
		int depth;
		struct {
			bool ignore_camera : 1;
		} flags;
		union {
			render_state_sprite_t sprite;
			render_shape_t shape;
			struct {
				int16_t x, y, length;
				char string[64];
			} text;
			struct {
				uint8_t color;
				render_state_sprite_t sprite;
			} sprite_silhouette;
		};
		#define RENDER_MAX_ELEMENTS 4096
	} elements[RENDER_MAX_ELEMENTS];
	int32_t element_count;
	#define PARTICLES_MAX 4096
	struct {
		uint8_t pixel[PARTICLES_MAX];
		vec2i_t position[PARTICLES_MAX];
		int count;
	} particles;
	mouse_t mouse;
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
	int depth;
    int x, y;
	const sprite_t *sprite;
    float rotation;
    int originx, originy;
	typeof ((render_state_t){}.elements[0].flags) flags;
	typeof ((render_state_sprite_t){}.flags) sprite_flags;
} Render_Sprite_arguments;
#define Render_Sprite(...) Render_Sprite_((Render_Sprite_arguments){__VA_ARGS__})
void Render_Sprite_ (Render_Sprite_arguments arguments);

#define Render_SpriteSilhouette(__color__, ...) Render_SpriteSilhouette_ (__color__, (Render_Sprite_arguments){RENDER_SPRITE_ARGUMENTS_DEFAULT, __VA_ARGS__})
void Render_SpriteSilhouette_ (uint8_t color, Render_Sprite_arguments arguments);

typedef struct {
    int depth;
	typeof ((render_state_t){}.elements[0].flags) flags;
    render_shape_t shape;
} Render_Shape_arguments;
#define Render_Shape(...) Render_Shape_((Render_Shape_arguments){__VA_ARGS__})
void Render_Shape_ (Render_Shape_arguments arguments);

typedef struct {
    int depth;
	const typeof ((render_state_t){}.elements[0].flags) flags;
    int x, y, length;
    const char *string;
	bool center_horizontally_on_screen;
	bool center_vertically_on_screen;
} Render_Text_arguments;
#define Render_Text(...) Render_Text_((Render_Text_arguments){.y = 20, __VA_ARGS__})
void Render_Text_ (Render_Text_arguments arguments);

typedef typeof((render_state_t){}.background) Render_Background_arguments;
#define Render_Background(...) Render_Background_ ((Render_Background_arguments){__VA_ARGS__})
void Render_Background_ (Render_Background_arguments background);

void Render_Particle (int x, int y, uint8_t pixel, bool ignore_camera);

render_state_t *Render_GetCurrentEditableState ();

void Render_Cursor (const sprite_t *sprite, int x, int y, int offsetx, int offsety);

typedef union {
    struct {int width, height;};
    struct {int w, h;};
} font_StringDimensions_return_t;
font_StringDimensions_return_t font_StringDimensions (const font_t *font, char *text);

// Must be called after any call to Render_Camera in order to take effect
void Render_ScreenShake (int x, int y);

void Render_ShowRenderTime (bool show);
void Render_ShowFPS (bool show);