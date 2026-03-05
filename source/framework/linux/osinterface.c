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

// Built together with ../osinterface_common.c

#include "osinterface.h"
#include "log.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef OSINTERFACE_COLOR_INDEX_MODE
extern const uint8_t palette[256][3];
#endif

#pragma push_macro ("Min")
#undef Min
#define Min(a, b) ((a) < (b) ? (a) : (b))
#pragma push_macro ("Max")
#undef Max
#define Max(a, b) ((a) > (b) ? (a) : (b))

#ifdef OSINTERFACE_COLOR_INDEX_MODE
bool os_CreateGLColorMap ();
#endif

#include <pwd.h>
#include "OpenGL2_1.h"

bool os_Init (const char *window_title) {
	{
		const char *sav = getenv ("XDG_DATA_HOME");
		if (sav) snprintf (os_public.directories.savegame, sizeof (os_public.directories.savegame), "%s", sav);
		const char *conf = getenv ("XDG_CONFIG_HOME");
		if (conf) snprintf (os_public.directories.config, sizeof (os_public.directories.config), "%s", conf);

		if (sav == NULL || conf == NULL) {
			const char *home = getenv ("HOME");
			if (home == NULL)
				home = getpwuid(getuid())->pw_dir; // getpwuid can apparently fail, but this seems basically impossible
			if (sav == NULL) snprintf (os_public.directories.savegame, sizeof (os_public.directories.savegame), "%s/.local/share", home);
			if (conf == NULL) snprintf (os_public.directories.config, sizeof (os_public.directories.config), "%s/.config", home);
		}
	}

	log_Init ();

	os_public.window.is_fullscreen = false;

	snprintf (os_public.window.title, sizeof (os_public.window.title)-1, "%s", window_title);

	// Create window
    {
        os_private.x11.display = XOpenDisplay (0);
        os_private.x11.root_window = DefaultRootWindow (os_private.x11.display);
        os_private.x11.screen = DefaultScreen (os_private.x11.display);

        // XMatchVisualInfo(os_private.x11.display, os_private.x11.screen, 24, TrueColor, &os_private.x11.visual_info);

		auto gl = OpenGL2_1_Init_Linux (os_private.x11.display, os_private.x11.screen);
		assert (gl.success); if (!gl.success) { LOG ("Failed to initialize OpenGL"); return false; }
		os_private.x11.gl_visual_info = gl.visual_info;
		os_private.x11.gl_context = gl.context;

        XSetWindowAttributes window_attributes;
        // window_attributes.background_pixel = 0xff808080;
        window_attributes.background_pixel = 0xff000000;
        window_attributes.colormap = XCreateColormap(os_private.x11.display, os_private.x11.root_window, os_private.x11.gl_visual_info->visual, AllocNone);
		window_attributes.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | FocusChangeMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
        os_private.x11.window = XCreateWindow (os_private.x11.display, os_private.x11.root_window, 0, 0, 1280, 720, 0, os_private.x11.gl_visual_info->depth, InputOutput, os_private.x11.gl_visual_info->visual, CWColormap | CWBackPixel | CWEventMask, &window_attributes);
		os_public.window.is_fullscreen = false;
        XMapWindow (os_private.x11.display, os_private.x11.window);
		os_GLMakeCurrent ();

		OpenGL2_1_EnableSwapTear_FallbackSwap_Linux(os_private.x11.display, os_private.x11.window);
		
		glViewport (0, 0, os_public.window.w, os_public.window.h);
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();
		glOrtho (0, os_public.window.w-1, 0, os_public.window.h-1, -1, 1);

		XChangeProperty (os_private.x11.display, os_private.x11.window, XA_WM_NAME, XA_STRING, 8, 0, (const char unsigned*)window_title, strlen (window_title));

		os_private.x11.WM_DELETE_WINDOW = XInternAtom(os_private.x11.display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(os_private.x11.display, os_private.x11.window, &os_private.x11.WM_DELETE_WINDOW, 1);

		// Set up drawing
		{
			struct {
				Window root_window;
				int x, y;
				unsigned int border_width, depth;
			} dummy;
			unsigned int width, height;
			XGetGeometry (os_private.x11.display, os_private.x11.window, &dummy.root_window, &dummy.x, &dummy.y, &width, &height, &dummy.border_width, &dummy.depth);
			os_public.window.width = width;
			os_public.window.height = height;
		}
#ifdef OSINTERFACE_FRAME_BUFFER_SCALED
		os_WindowFrameBufferCalculateScale ();
#else
	#ifndef OSINTERFACE_NATIVE_GL_RENDERING
		os_private.frame_buffer.width = os_public.window.width;
		os_private.frame_buffer.height = os_public.window.height;
		os_private.frame_buffer.pixels = malloc (os_private.frame_buffer.width * os_private.frame_buffer.height * sizeof (*os_private.frame_buffer.pixels));
	#endif
#endif

		XkbSetDetectableAutoRepeat(os_private.x11.display, True, 0); // No key repeat when holding key down

		{
			struct {
				Window root_window, child_window;
				int x, y;
				unsigned int mask;
			} dummy;
			XQueryPointer(os_private.x11.display, os_private.x11.window, &dummy.root_window, &dummy.child_window, &dummy.x, &dummy.y, &os_public.mouse.p.x, &os_public.mouse.p.y, &dummy.mask);
			os_public.mouse.p.y = os_public.window.h - 1 - os_public.mouse.p.y;
		}

        XFlush(os_private.x11.display);
    }

	// os_private.background_color.r = 0x40;
	// os_private.background_color.g = 0x3a;
	// os_private.background_color.b = 0x4d;
	os_SetBackgroundColor (0,0,0);

#ifdef OSINTERFACE_COLOR_INDEX_MODE
	if (!os_CreateGLColorMap ()) { LOG ("Failed to create OpenGL color map shader"); return false;}
#endif

	#ifdef OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE
	glXMakeCurrent (os_private.x11.display, None, NULL);
	#endif

	return true;
}

void os_SetBackgroundColor (uint8_t r, uint8_t g, uint8_t b) {
	// os_private.background_color.u32 = ((uint32_t)r << 16) + ((uint32_t)g << 8) + b;
	os_private.background_color.r = r;
	os_private.background_color.g = g;
	os_private.background_color.b = b;
	os_private.background_color.a = 255;
	XSetWindowBackground (os_private.x11.display, os_private.x11.window, os_private.background_color.u32);
}

void os_SendQuitEvent () {
	XEvent e;
	e.type = DestroyNotify;
	XSendEvent (os_private.x11.display, os_private.x11.window, True, NoEventMask, &e);
}

os_event_t os_NextEvent () {
	XEvent e;
	os_event_t event = {.type = os_EVENT_INTERNAL};
	bool key_is_down = true;
	while (event.type == os_EVENT_INTERNAL) {
		if (XPending(os_private.x11.display)) {
			__label__ mouse_button_skip;
			XNextEvent(os_private.x11.display, &e);
			switch (e.type) {
				case DestroyNotify: {
					event.type = os_EVENT_QUIT;
					event.exit_code = 0;
				} break;

				case ClientMessage: {
					XClientMessageEvent* ev = (XClientMessageEvent*)&e;
					if((Atom)ev->data.l[0] == os_private.x11.WM_DELETE_WINDOW) {
						event.type = os_EVENT_QUIT;
					}
				} break;

				case FocusOut: {
					memset(os_public.keyboard, false, sizeof(os_public.keyboard));
					os_public.mouse.buttons = 0;
				} break;

				case KeyRelease:
					event.type = os_EVENT_KEY_RELEASE;
					key_is_down = false;
				case KeyPress: { // key_is_down is initialized to true before the switch statement
					int symbol = XLookupKeysym(&e.xkey, 0);
					if (symbol == XK_Shift_R) symbol = XK_Shift_L;
					os_key_e key;
					switch (symbol) {
						case 'a': key = 'a'; break;
						case 'b': key = 'b'; break;
						case 'c': key = 'c'; break;
						case 'd': key = 'd'; break;
						case 'e': key = 'e'; break;
						case 'f': key = 'f'; break;
						case 'g': key = 'g'; break;
						case 'h': key = 'h'; break;
						case 'i': key = 'i'; break;
						case 'j': key = 'j'; break;
						case 'k': key = 'k'; break;
						case 'l': key = 'l'; break;
						case 'm': key = 'm'; break;
						case 'n': key = 'n'; break;
						case 'o': key = 'o'; break;
						case 'p': key = 'p'; break;
						case 'q': key = 'q'; break;
						case 'r': key = 'r'; break;
						case 's': key = 's'; break;
						case 't': key = 't'; break;
						case 'u': key = 'u'; break;
						case 'v': key = 'v'; break;
						case 'w': key = 'w'; break;
						case 'x': key = 'x'; break;
						case 'y': key = 'y'; break;
						case 'z': key = 'z'; break;
						case '0': key = '0'; break;
						case '1': key = '1'; break;
						case '2': key = '2'; break;
						case '3': key = '3'; break;
						case '4': key = '4'; break;
						case '5': key = '5'; break;
						case '6': key = '6'; break;
						case '7': key = '7'; break;
						case '8': key = '8'; break;
						case '9': key = '9'; break;
						case XK_Return: key = os_KEY_ENTER; break;
						case XK_space: key = ' '; break;
						case XK_Escape: key = os_KEY_ESCAPE; break;
						case XK_Left: key = os_KEY_LEFT; break;
						case XK_Right: key = os_KEY_RIGHT; break;
						case XK_Down: key = os_KEY_DOWN; break;
						case XK_Up: key = os_KEY_UP; break;
						case XK_Alt_R: key = os_KEY_RALT; break;
						case XK_Alt_L: key = os_KEY_LALT; break;
						case XK_Control_L: case XK_Control_R: key = os_KEY_CTRL; break;
						case XK_Home: key = os_KEY_HOME; break;
						case XK_End: key = os_KEY_END; break;
						case XK_Page_Up: key = os_KEY_PAGEUP; break;
						case XK_Page_Down: key = os_KEY_PAGEDOWN; break;
						case XK_F1: key = os_KEY_F1; break;
						case XK_F2: key = os_KEY_F2; break;
						case XK_F3: key = os_KEY_F3; break;
						case XK_F4: key = os_KEY_F4; break;
						case XK_F5: key = os_KEY_F5; break;
						case XK_F6: key = os_KEY_F6; break;
						case XK_F7: key = os_KEY_F7; break;
						case XK_F8: key = os_KEY_F8; break;
						case XK_F9: key = os_KEY_F9; break;
						case XK_F10: key = os_KEY_F10; break;
						case XK_F11: key = os_KEY_F11; break;
						case XK_F12: key = os_KEY_F12; break;
						case XK_BackSpace: key = os_KEY_BACKSPACE; break;
						case XK_Delete: key = os_KEY_DELETE; break;
						case XK_Shift_L: key = os_KEY_SHIFT; break;
						case XK_Tab: key = '	'; break;
						case XK_apostrophe: key = '\''; break;
						case XK_comma: key = ','; break;
						case XK_minus: key = '-'; break;
						case XK_period: key = '.'; break;
						case XK_slash: key = '/'; break;
						case XK_semicolon: key = ';'; break;
						case XK_equal: key = '='; break;
						case XK_bracketleft: key = '['; break;
						case XK_bracketright: key = ']'; break;
						case XK_backslash: key = '\\'; break;
						case XK_grave: key = '`'; break;
						default: key = os_KEY_INVALID; break;
					}
					if (os_public.keyboard[key] != key_is_down) { // Prevent key repeats from sending events. Should only happen with key release anyway.
						os_public.keyboard[key] = key_is_down;
						event.key = key;
						if(key_is_down) {
							event.type = os_EVENT_KEY_PRESS;
						}
					}
				} break;

				case ConfigureNotify: {
					XConfigureEvent* ev = (XConfigureEvent*)&e;
					if (ev->width != os_public.window.w || ev->height != os_public.window.h) { // Window resized
						event.type = os_EVENT_WINDOW_RESIZE;
						os_public.window.w = event.width = ev->width;
						os_public.window.h = event.height = ev->height;

						struct {
							Window root_window, child_window;
							int x, y;
							unsigned int mask;
						} dummy;
						XQueryPointer(os_private.x11.display, os_private.x11.window, &dummy.root_window, &dummy.child_window, &dummy.x, &dummy.y, &os_public.mouse.p.x, &os_public.mouse.p.y, &dummy.mask);
						os_public.mouse.p.y = os_public.window.h - 1 - os_public.mouse.p.y;


	#ifndef OSINTERFACE_NATIVE_GL_RENDERING
						glViewport (0, 0, os_public.window.w, os_public.window.h);
						glMatrixMode (GL_PROJECTION);
						glLoadIdentity ();
						glOrtho (0, os_public.window.w-1, 0, os_public.window.h-1, -1, 1);

		#ifdef OSINTERFACE_FRAME_BUFFER_SCALED
							os_WindowFrameBufferCalculateScale ();
		#else
			#ifdef OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE
							while (os_private.new_frame_buffer.is_valid); // Wait for different thread to switch to most recently created frame buffer
							os_private.new_frame_buffer.width  = os_public.window.width;
							os_private.new_frame_buffer.height = os_public.window.height;
							os_private.new_frame_buffer.pixels = malloc (os_private.new_frame_buffer.width * os_private.new_frame_buffer.height * sizeof (os_private.new_frame_buffer.pixels[0]));
							assert (os_private.frame_buffer.pixels);
							os_private.new_frame_buffer.is_valid = true;
			#else // OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE
							os_private.frame_buffer.w = os_public.window.width;
							os_private.frame_buffer.h = os_public.window.height;
							os_private.frame_buffer.pixels = realloc (os_private.frame_buffer.pixels, os_private.frame_buffer.width * os_private.frame_buffer.height * sizeof (os_private.frame_buffer.pixels[0]));
							assert (os_private.frame_buffer.pixels);
			#endif
							glMatrixMode (GL_MODELVIEW);
							glLoadIdentity ();
							glPixelZoom (1, 1);
							glRasterPos2i (0, 0);
		#endif
	#endif
					}
				} break;

				case MotionNotify: {
					XMotionEvent *ev = (XMotionEvent*)&e;
					event.type = os_EVENT_MOUSE_MOVE;
					event.previous_position.x = os_public.mouse.p.x;
					event.previous_position.y = os_public.mouse.p.y;
					os_public.mouse.p.x = event.new_position.x = ev->x;
					os_public.mouse.p.y = event.new_position.y = os_public.window.h - 1 - ev->y;
				} break;

				case ButtonRelease:
					event.type = os_EVENT_MOUSE_BUTTON_RELEASE;
					key_is_down = false;
				case ButtonPress: { // key_is_down is initialized to true before the switch statement
					if (key_is_down)
						event.type = os_EVENT_MOUSE_BUTTON_PRESS;
					XButtonEvent *ev = (XButtonEvent*)&e;
					event.button.p.x = ev->x;
					event.button.p.y = os_public.window.h - 1 - ev->y;
					switch (ev->button) {
						case Button1: event.button.button = os_MOUSE_LEFT; break;
						case Button2: event.button.button = os_MOUSE_MIDDLE; break;
						case Button3: event.button.button = os_MOUSE_RIGHT; break;
						case Button4: {
							event.type = os_EVENT_MOUSE_SCROLL;
							event.scrolled_up = true;
							goto mouse_button_skip;
						} break;
						case Button5: {
							event.type = os_EVENT_MOUSE_SCROLL;
							event.scrolled_up = false;
							goto mouse_button_skip;
						} break;
					}
					if (key_is_down) os_public.mouse.buttons |= event.button.button;
					else os_public.mouse.buttons &= ~event.button.button;
					mouse_button_skip:
				} break;
			}
		}
		else event.type = os_EVENT_NULL;
	}
    return event;
}

void os_WindowSize (int width, int height) {
	XMoveResizeWindow (os_private.x11.display, os_private.x11.window, 0, 0, width, height);
}

void os_Maximize (bool maximize) {
	os_public.window.is_fullscreen = false;
	
	if (maximize) {
		XEvent e = {};
		e.type = ClientMessage;
		e.xclient.window = os_private.x11.window;
		e.xclient.message_type = XInternAtom(os_private.x11.display, "_NET_WM_STATE", False);
		e.xclient.format = 32;
		e.xclient.data.l[0] = 1;
		e.xclient.data.l[1] = XInternAtom(os_private.x11.display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
		e.xclient.data.l[2] = XInternAtom(os_private.x11.display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

		XSendEvent(os_private.x11.display, os_private.x11.root_window, False, SubstructureNotifyMask, &e);
	}
	else os_WindowSize (os_public.window.w, os_public.window.h);
}

#ifdef OSINTERFACE_NATIVE_GL_RENDERING
void os_DrawScreen () { glXSwapBuffers (os_private.x11.display, os_private.x11.window); }
#endif // OSINTERFACE_NATIVE_GL_RENDERING

void os_WaitForScreenRefresh () {
	glFinish ();
} // Ensure that last frame has been presented

void os_Fullscreen (bool fullscreen) {
	if (fullscreen) {
		os_public.window.is_fullscreen = true;

		Atom wm_state = XInternAtom (os_private.x11.display, "_NET_WM_STATE", False);
		Atom fullscreen = XInternAtom (os_private.x11.display, "_NET_WM_STATE_FULLSCREEN", False);

		XEvent e = {};
		e.type = ClientMessage;
		e.xclient.window = os_private.x11.window;
		e.xclient.message_type = wm_state;
		e.xclient.format = 32;
		e.xclient.data.l[0] = 1;
		e.xclient.data.l[1] = fullscreen;
		e.xclient.data.l[2] = 0;

		XSendEvent (os_private.x11.display, os_private.x11.root_window, False, SubstructureRedirectMask | SubstructureNotifyMask, &e);
	} else {
		os_public.window.is_fullscreen = false;

		Atom wm_state = XInternAtom (os_private.x11.display, "_NET_WM_STATE", False);
		Atom fullscreen = XInternAtom (os_private.x11.display, "_NET_WM_STATE_FULLSCREEN", False);

		XEvent e = {};
		e.type = ClientMessage;
		e.xclient.window = os_private.x11.window;
		e.xclient.message_type = wm_state;
		e.xclient.format = 32;
		e.xclient.data.l[0] = 0;
		e.xclient.data.l[1] = fullscreen;
		e.xclient.data.l[2] = 0;

		XSendEvent (os_private.x11.display, os_private.x11.root_window, False, SubstructureRedirectMask | SubstructureNotifyMask, &e);
	}
}

int64_t os_uTime () {
	timespec_t t;
	clock_gettime (CLOCK_MONOTONIC, &t);
	int64_t ticks = t.tv_sec * 1000000000 + t.tv_nsec;
	return ticks / 1000;
}

void os_uSleepEfficient (int64_t microseconds) {
	// if (microseconds > 0) usleep (microseconds);
	if (microseconds <= 0) return;
	timespec_t t;
	if (clock_gettime (CLOCK_MONOTONIC, &t) == -1) {
		t.tv_sec = 0;
		t.tv_nsec = microseconds * 1000;
		if (microseconds > 1000000) {
			t.tv_sec = microseconds / 1000000;
			t.tv_nsec = microseconds * 1000 - t.tv_sec * 1000000000;
		}
		while (clock_nanosleep (CLOCK_MONOTONIC, 0, &t, &t) == EINTR); // If sleep is interrupted, it will repeat with the remaining time until complete.
		return;
	}

	t.tv_nsec += microseconds * 1000;
	if (t.tv_nsec > 999999999) {
		t.tv_nsec -= 1000000000;
		++t.tv_sec;
	}

	while (clock_nanosleep (CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL) == EINTR); // If sleep is interrupted, it will repeat with the same target time until complete.
}

void os_uSleepPrecise (int64_t microseconds) {
	os_uSleepEfficient (microseconds);
}

// XFixes version of cursor show/hide. Not using because if ShowCursor is called while the cursor is already visible, the program crashes...
// #include <X11/extensions/Xfixes.h>
// static bool cursor_hidden = false;
// void os_ShowCursor () {
	// if (!cursor_hidden) return;
	// cursor_hidden = false;
	// XFixesShowCursor (os_private.x11.display, os_private.x11.window);
// }

// void os_HideCursor () {
	// if (cursor_hidden) return;
	// cursor_hidden = true;
	// XFixesHideCursor (os_private.x11.display, os_private.x11.window);
// }

void os_ShowCursor () {
	XUndefineCursor (os_private.x11.display, os_private.x11.window);
}

void os_HideCursor () {
	Cursor invisible_cursor;
	Pixmap pixmap_dummy;
	pixmap_dummy = XCreateBitmapFromData(os_private.x11.display, os_private.x11.window, "", 1, 1);
	invisible_cursor = XCreatePixmapCursor(os_private.x11.display, pixmap_dummy, pixmap_dummy, &(XColor){}, &(XColor){}, 0, 0);
	XFreePixmap(os_private.x11.display, pixmap_dummy);
	XDefineCursor (os_private.x11.display, os_private.x11.window, invisible_cursor);
	XFreeCursor(os_private.x11.display, invisible_cursor);
}

// Returns the refresh rate of the display on which the program window is.
// Returns 0 if the refresh rate cannot be retrieved.
int os_GetScreenRefreshRate () {
	XRRScreenConfiguration *configuration = XRRGetScreenInfo (os_private.x11.display, os_private.x11.window);
	if (configuration) {
		return XRRConfigCurrentRate (configuration);
	}
	LOG ("Unable to retrieve screen refresh rate");
	return 0;
}

// UNTESTED FUNCTION PROBABLY DOESN'T WORK YET
// Attempts to set specified screen refresh rate.
// Returns the rate after the attempted change, or 0 if the rate cannot be retrieved.
// If the maximum screen refresh rate is lower than the requested rate, the maximum rate will be set instead.
int os_SetScreenRefreshRate (int rate) {
	if (os_GetScreenRefreshRate () == rate) return rate;

	int num_rates;
	short *rates;
	rates = XRRRates (os_private.x11.display, os_private.x11.screen, sizeof (short), &num_rates);
	short highest_rate = 0;
	for (int i = 0; i < num_rates; ++i) if (rates[i] > highest_rate) highest_rate = rates[i];

	XRRScreenConfiguration *configuration = XRRGetScreenInfo (os_private.x11.display, os_private.x11.window);
	Rotation *current_rotation = 0;
	XRRConfigRotations(configuration, current_rotation);
	XRRSetScreenConfigAndRate (os_private.x11.display, configuration, os_private.x11.screen, 0, XRRConfigCurrentConfiguration (configuration, current_rotation), (short)rate, 0);

	if (os_GetScreenRefreshRate () == rate) return rate;

	LOG ("Unable to set specified refresh rate [%d]", rate);

	return 0;
}

void os_MessageBox_ (os_MessageBox_arguments arguments) {
	if (arguments.message == NULL) arguments.message = "";
	if (arguments.title == NULL) arguments.title = "";
	char string[1024];
	snprintf (string, 1023, "zenity --info --title=\"%s\" --text \"%s\"", arguments.title, arguments.message);
	system (string);
}

bool os_GLMakeCurrent () {
	auto result = glXMakeCurrent (os_private.x11.display, os_private.x11.window, os_private.x11.gl_context); assert (result); if (result == false) { LOG ("glXMarkCurrent() failed [%s]", gluErrorString(glGetError())); return false; }
	return true;
}

os_char1024_t os_OpenFileDialog (const char *title) {
	if (!title) title = "Open File";
	os_char1024_t buf;
	snprintf (buf.str, sizeof(buf.str), "zenity --file-selection --title=\"%s\" --file-filter=\"All Files | *\" --filename=\".\"", title);
	FILE *result = popen (buf.str, "r");
	if (result == NULL) return (os_char1024_t){""};
	auto len = fread (buf.str, 1, sizeof (buf.str), result);
	pclose (result);
	if (len <= 0) return (os_char1024_t){""};
	buf.str[len-1] = 0;
	return buf;
}
os_char1024_t os_SaveFileDialog (const char *title, const char *save_button_text, const char *filename_label, const char *default_filename) {
	if (!title) title = "Save File";
	os_char1024_t buf;
	snprintf (buf.str, sizeof(buf.str), "zenity --file-selection --title=\"%s\" --file-filter=\"All Files | *\" --filename=\".\" --save --confirm-overwrite", title);
	FILE *result = popen (buf.str, "r");
	if (result == NULL) return (os_char1024_t){""};
	auto len = fread (buf.str, 1, sizeof (buf.str), result);
	pclose (result);
	if (len <= 0) return (os_char1024_t){""};
	buf.str[len-1] = 0;
	return buf;
}

void os_OpenURL (const char *url) { os_OpenFileBrowser (url); }

void os_OpenFileBrowser (const char *directory) {
	char command[1040];
	assert (strlen (directory) < 1024);
	snprintf (command, sizeof (command), "xdg-open \"%s\"", directory);
	system (command);
}

void os_Cleanup () {}

#ifndef OSINTERFACE_NATIVE_GL_RENDERING

void os_DrawScreen () {
	if (os_LogGLErrors ()) LOG ("Had GL errors");
	glFinish ();
	if (os_LogGLErrors ()) LOG ("Had GL errors");
	glClearColor (os_private.background_color.r / 255.f, os_private.background_color.g / 255.f, os_private.background_color.b / 255.f, 1);
	if (os_LogGLErrors ()) LOG ("Had GL errors");
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	#ifdef OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE
		if (os_private.new_frame_buffer.is_valid) {
			os_private.frame_buffer.w = os_private.new_frame_buffer.w;
			os_private.frame_buffer.h = os_private.new_frame_buffer.h;
			free (os_private.frame_buffer.pixels);
			os_private.frame_buffer.pixels = os_private.new_frame_buffer.pixels;
			os_private.new_frame_buffer.is_valid = false;
		}
	#endif

#ifdef OSINTERFACE_COLOR_INDEX_MODE
	glUniform2f (os_private.gl.locations.vertex.scale, (float)os_private.frame_buffer.width * os_private.frame_buffer.scale / os_public.window.width, (float)os_private.frame_buffer.height * os_private.frame_buffer.scale / os_public.window.height);
	// if (os_LogGLErrors ()) LOG ("Had GL errors");
	// glActiveTexture(GL_TEXTURE0);
	// glBindTexture (GL_TEXTURE_2D, os_private.gl.texture);
	if (os_LogGLErrors ()) LOG ("Had GL errors");
	// BUG: When I ALT+F4, this line sometimes segfaults
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, os_private.frame_buffer.width, os_private.frame_buffer.height, 0, GL_RED, GL_UNSIGNED_BYTE, os_private.frame_buffer.pixels);
	if (os_LogGLErrors ()) LOG ("Had GL errors");
	glBegin (GL_TRIANGLE_STRIP);
	glTexCoord2f (0,0); glVertex2f(-1,-1);
	glTexCoord2f (0,1); glVertex2f(-1, 1);
	glTexCoord2f (1,0); glVertex2f( 1,-1);
	glTexCoord2f (1,1); glVertex2f( 1, 1);
	glEnd ();
	if (os_LogGLErrors ()) LOG ("Had GL errors");
#else
	glDrawPixels (os_private.frame_buffer.width, os_private.frame_buffer.height, GL_BGRA, GL_UNSIGNED_BYTE, os_private.frame_buffer.pixels);
#endif

	glXSwapBuffers (os_private.x11.display, os_private.x11.window);
	if (os_LogGLErrors ()) LOG ("Had GL errors");
}
#endif

bool os_LogGLErrors () {
	{
		auto context = glXGetCurrentContext ();
		assert (context); if (context == NULL) { LOG ("The thread which called os_LogGLErrors() does not have an OpenGL context"); return true; }
	}
	bool had_error = false;
	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR) {
		had_error = true;
		LOG ("OpenGL error [%s]", gluErrorString(error));
	}
	assert (!had_error);
	return had_error;
}
