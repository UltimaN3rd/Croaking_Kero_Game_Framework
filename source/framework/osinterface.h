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

// For a frame buffer allocated by this library, automatically resized to the window size, you needn't define anything.
// For a frame buffer allocated by you, scaled to fit the window, you must:
// #define OSINTERFACE_FRAME_BUFFER_SCALED

// static_assert(true); // Solves clang getting confused about pragma push/pop. Related to preamble optimization

#include <stdint.h>
#include <stdbool.h>

#ifdef OSINTERFACE_COLOR_INDEX_MODE
typedef uint8_t frame_buffer_pixel_t;
#else
typedef uint32_t frame_buffer_pixel_t;
#endif

typedef enum {
	os_EVENT_NULL, os_EVENT_INTERNAL, os_EVENT_QUIT, os_EVENT_WINDOW_RESIZE, os_EVENT_KEY_PRESS, os_EVENT_KEY_RELEASE, os_EVENT_MOUSE_BUTTON_PRESS, os_EVENT_MOUSE_BUTTON_RELEASE, os_EVENT_MOUSE_MOVE, os_EVENT_MOUSE_SCROLL,
} os_event_e;

typedef enum { os_MOUSE_LEFT = 0b1, os_MOUSE_MIDDLE = 0b10, os_MOUSE_RIGHT = 0b100, os_MOUSE_X1 = 0b1000, os_MOUSE_X2 = 0b10000 } os_mouse_button_e;

typedef enum {
	os_KEY_INVALID, os_KEY_ENTER, os_KEY_ESCAPE, os_KEY_LEFT, os_KEY_RIGHT, os_KEY_UP, os_KEY_DOWN, os_KEY_LALT, os_KEY_RALT, os_KEY_TAB = '	', os_KEY_CTRL, os_KEY_BACKSPACE, os_KEY_DELETE, os_KEY_SHIFT, os_KEY_HOME, os_KEY_END, os_KEY_PAGEUP, os_KEY_PAGEDOWN,
	os_KEY_F1, os_KEY_F2, os_KEY_F3, os_KEY_F4, os_KEY_F5, os_KEY_F6, os_KEY_F7, os_KEY_F8, os_KEY_F9, os_KEY_F10, os_KEY_F11, os_KEY_F12,
	os_KEY_FIRST_WRITABLE = ' ', os_KEY_SPACE = ' ', os_KEY_APOSTROPHE = '\'', os_KEY_COMMA = ',', os_KEY_MINUS = '-', os_KEY_PERIOD = '.', os_KEY_SLASH = '/',  os_KEY_SEMICOLON = ';', os_KEY_EQUALS = '=', os_KEY_BRACKETLEFT = '[', os_KEY_BACKSLASH = '\\', os_KEY_BRACKETRIGHT = ']', os_KEY_GRAVE = '`',
	os_KEY_0 = '0', os_KEY_1, os_KEY_2, os_KEY_3, os_KEY_4, os_KEY_5, os_KEY_6, os_KEY_7, os_KEY_8, os_KEY_9,
	os_KEY_A = 'a', os_KEY_B, os_KEY_C, os_KEY_D, os_KEY_E, os_KEY_F, os_KEY_G, os_KEY_H, os_KEY_I, os_KEY_J, os_KEY_K, os_KEY_L, os_KEY_M, os_KEY_N, os_KEY_O, os_KEY_P, os_KEY_Q,	os_KEY_R, os_KEY_S, os_KEY_T, os_KEY_U, os_KEY_V, os_KEY_W, os_KEY_X, os_KEY_Y, os_KEY_Z, os_KEY_LAST_WRITABLE = 'z',
} os_key_e;

static inline char os_key_shifted (os_key_e key) {
	if (key < os_KEY_FIRST_WRITABLE || key > os_KEY_LAST_WRITABLE) return key;
	switch (key) {
		case ' ': return ' ';
		case '\'': return '"';
		case ',': return '<';
		case '-': return '_';
		case '.': return '>';
		case '/': return '?';
		case ';': return ':';
		case '=': return '+';
		case '[': return '{';
		case '\\': return '|';
		case ']': return '}';
		case '`': return '~';
		case '0': return ')';
		case '1': return '!';
		case '2': return '@';
		case '3': return '#';
		case '4': return '$';
		case '5': return '%';
		case '6': return '^';
		case '7': return '&';
		case '8': return '*';
		case '9': return '(';
		default: break;
	}
	if (key >= 'a' && key <= 'z') return key + 'A' - 'a';
	__builtin_unreachable();
}

typedef struct {
	int x, y;
} os_vec2i;

typedef struct {
	float x, y;
} os_vec2f;

typedef struct {
    os_event_e type;
    union {
        struct { // WINDOW_RESIZE
            int width, height;
        };
		os_key_e key; // KEY_PRESS + RELEASE
		struct {
			os_mouse_button_e button; // MOUSE_BUTTON_PRESS + _RELEASE
			union { os_vec2i position, p; };
		} button;
		struct { // MOUSE_MOVE
			os_vec2i previous_position, new_position;
		};
		bool scrolled_up; // MOUSE_SCROLL
		int exit_code; // QUIT
    };
} os_event_t;

typedef struct {
	bool keyboard[128];
	struct {
		union { os_vec2i p, position; };
		uint8_t buttons;
		bool hidden;
	} mouse;
	struct {
		union { int width, w; };
		union { int height, h; };
		bool is_fullscreen;
		char title[128];
	} window;
	#define os_PLATFORM_DIRECTORY_MAX_LENGTH 512
	struct {
    	char savegame[os_PLATFORM_DIRECTORY_MAX_LENGTH], config[os_PLATFORM_DIRECTORY_MAX_LENGTH];
	} directories;
} os_public_t;

#ifdef OSINTERFACE_FRAME_BUFFER_SCALED
void os_WindowFrameBufferCalculateScale ();
#endif






#ifdef WIN32






#define UNICODE
#define _UNICODE

#include <windows.h>
#include <uxtheme.h>
#include <mmsystem.h>
#include <process.h>
#include <string.h>
#include <wingdi.h> // Only needed for HGLRC definition

typedef struct {
	union {
		struct { uint8_t r, g, b, a; };
		uint32_t u32;
	} background_color;
	struct {
		HWND window_handle;
		BITMAPINFO bitmap_info;
		HBITMAP bitmap;
		HDC bitmap_device_context;
		HBRUSH background_brush;
		int64_t ticks_per_microsecond;
		HGLRC gl_context;
		HDC window_context;
	} win32;
	struct {
		union { unsigned int width, w; };
		union { unsigned int height, h; };
		frame_buffer_pixel_t *pixels;
		int scale;
		int left, bottom;
		bool has_been_set;
	} frame_buffer;
	#ifdef OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE
	struct {
		volatile bool is_valid;
		union { unsigned int width, w; };
		union { unsigned int height, h; };
		frame_buffer_pixel_t *pixels;
	} new_frame_buffer;
	#endif
	#ifdef OSINTERFACE_COLOR_INDEX_MODE
	struct {
		uint32_t texture;
		struct {
			struct { int scale; } vertex;
			struct { int palette, texture; } fragment;
		} locations;
	} gl;
	#endif
} os_private_t;








#elif defined __linux__








#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <errno.h>
#include <time.h>
typedef struct timespec timespec_t;
#include <unistd.h> // usleep
// #include <X11/extensions/Xfixes.h> // XFixesHideCursor - Doesn't work under XWayland, so no longer necessary
#include <X11/extensions/Xrandr.h> // Refresh rate
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>

typedef struct {
	union {
		struct { uint8_t r, g, b, a; };
		uint32_t u32;
	} background_color;
	struct {
		Display* display;
		int root_window;
		int screen;
		Window window;
		Atom WM_DELETE_WINDOW;
		XVisualInfo *gl_visual_info;
		GLXContext gl_context;
	} x11;
#ifndef OSINTERFACE_NATIVE_GL_RENDERING
	struct {
		union { unsigned int width, w; };
		union { unsigned int height, h; };
		frame_buffer_pixel_t *pixels;
		int scale;
		int left, bottom;
		bool has_been_set;
	} frame_buffer;
	struct {
		volatile bool is_valid;
		union { unsigned int width, w; };
		union { unsigned int height, h; };
		frame_buffer_pixel_t *pixels;
	} new_frame_buffer;
	#ifdef OSINTERFACE_COLOR_INDEX_MODE
	struct {
		uint32_t texture;
		struct {
			struct { int scale; } vertex;
			struct { int palette, texture; } fragment;
		} locations;
	} gl;
	#endif
#endif
} os_private_t;





#elif defined __APPLE__

#include <objc/objc.h>
#include <pthread.h>

typedef struct {
	union {
		struct { uint8_t r, g, b, a; };
		uint32_t u32;
	} background_color;
	struct {
		id application, window, gl_context;
		pthread_mutex_t opengl_mutex;
	} osx;
#ifndef OSINTERFACE_NATIVE_GL_RENDERING
	struct {
		union { unsigned int width, w; };
		union { unsigned int height, h; };
		frame_buffer_pixel_t *pixels;
		int scale;
		int left, bottom;
		bool has_been_set;
	} frame_buffer;
	struct {
		volatile bool is_valid;
		union { unsigned int width, w; };
		union { unsigned int height, h; };
		frame_buffer_pixel_t *pixels;
	} new_frame_buffer;
	#ifdef OSINTERFACE_COLOR_INDEX_MODE
	struct {
		uint32_t texture;
		struct {
			struct { int scale; } vertex;
			struct { int palette, texture; } fragment;
		} locations;
	} gl;
	#endif
#endif
} os_private_t;




#endif





bool os_Init (const char *window_title);
void os_SetBackgroundColor (uint8_t r, uint8_t g, uint8_t b);
void os_Fullscreen (bool fullscreen);
void os_Maximize (bool maximize);
void os_WindowSize (int width, int height);
void os_ShowCursor ();
void os_HideCursor ();
os_event_t os_NextEvent ();
void os_SendQuitEvent ();
void os_WaitForScreenRefresh ();
void os_DrawScreen ();
int64_t os_uTime ();
void os_uSleepEfficient (int64_t microseconds);
void os_uSleepPrecise (int64_t microseconds);
void os_MessageBox (char *message);
bool os_GLMakeCurrent ();
typedef struct {char str[1024];} os_char1024_t; 
os_char1024_t os_OpenFileDialog (const char *title);
os_char1024_t os_SaveFileDialog (const char *title, const char *save_button_text, const char *filename_label, const char *default_filename);

#ifdef OSINTERFACE_FRAME_BUFFER_SCALED
typedef struct {int x, y;} os_intxy_t;
os_intxy_t os_WindowPositionToScaledFrameBufferPosition (int windowx, int windowy);
os_intxy_t os_ScaledFrameBufferPositionToWindowPosition (int framex, int framey);
void os_SetWindowFrameBuffer (frame_buffer_pixel_t *pixels, int width, int height);
void os_WindowFrameBufferCalculateScale ();
#endif

// UNTESTED FUNCTION PROBABLY DOESN'T WORK YET
// Attempts to set specified screen refresh rate.
// Returns the rate after the attempted change, or 0 if the rate cannot be retrived.
// If the maximum screen refresh rate is lower than the requested rate, the maximum rate will be set instead.
int os_SetScreenRefreshRate (int rate);

// Returns the refresh rate of the display containing the program window.
// Returns 0 if the refresh rate cannot be retrieved.
int os_GetScreenRefreshRate ();
void os_Cleanup ();
void os_ClearScreen ();

extern os_public_t os_public;
extern os_private_t os_private;

typedef struct { const char *title, *message; } os_MessageBox_arguments;
void os_MessageBox_ (os_MessageBox_arguments arguments);
#define os_MessageBox(...) os_MessageBox_ ((os_MessageBox_arguments){.title = os_public.window.title, __VA_ARGS__})

os_char1024_t os_OpenFileDialog (const char *title);
os_char1024_t os_SaveFileDialog (const char *title, const char *save_button_text, const char *filename_label, const char *default_filename);

void os_OpenFileBrowser (const char *directory);
bool os_LogGLErrors ();