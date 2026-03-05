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
# include "windows/OpenGL2_1.h"

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

#include <Shlobj.h>
#include <assert.h>
#include "NtSetTimerResolution.h"

LRESULT CALLBACK os_Internal_WindowProcessMessage(HWND window_handle, UINT message, WPARAM wParam, LPARAM lParam);

static const char *os_HResultToStr (HRESULT result) {
    static char win32_error_buffer[512];
    DWORD len;  // Number of chars returned.
    len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, result, 0, win32_error_buffer, 512, NULL);
    if (len == 0) {
        HINSTANCE hInst = LoadLibraryA("Ntdsbmsg.dll");
        if (hInst == NULL)
            snprintf (win32_error_buffer, sizeof (win32_error_buffer), "Cannot convert error to string: Failed to load Ntdsbmsg.dll");
        else {
            len = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, hInst, result, 0, win32_error_buffer, 512, NULL);
            if (len == 0) snprintf (win32_error_buffer, sizeof (win32_error_buffer), "HRESULT error message not found");
            FreeLibrary(hInst);
        }
    }
    return win32_error_buffer;
}


static RECT windowed_size = {};
static RECT windowed_pos = {};

bool os_Init (const char *window_title) {
	{ ULONG time; NtSetTimerResolution (1000, TRUE, &time); }
	SetErrorMode (SEM_FAILCRITICALERRORS);

	{
		PWSTR str = NULL;
		auto result = SHGetKnownFolderPath (&FOLDERID_SavedGames, 0, NULL, &str);
		char buf[512];
		BOOL used_default_char = FALSE;
		auto conversion_result = WideCharToMultiByte (CP_ACP, 0, str, -1, buf, sizeof (buf), NULL, &used_default_char);
		CoTaskMemFree (str);
		if (result == S_OK && conversion_result != 0)
			snprintf (os_public.directories.savegame, sizeof (os_public.directories.savegame), "%s", buf);
		else {
			const char *sav = getenv ("HOME");
			snprintf (os_public.directories.savegame, sizeof (os_public.directories.savegame), "%s/Saved Games", sav);
		}
	}
	{
		PWSTR str = NULL;
		auto result = SHGetKnownFolderPath (&FOLDERID_LocalAppData, 0, NULL, &str);
		char buf[512];
		BOOL used_default_char = FALSE;
		auto conversion_result = WideCharToMultiByte (CP_ACP, 0, str, -1, buf, sizeof (buf), NULL, &used_default_char);
		CoTaskMemFree (str);
		if (result == S_OK && conversion_result != 0)
			snprintf (os_public.directories.config, sizeof (os_public.directories.savegame), "%s", buf);
		else {
			const char *sav = getenv ("HOME");
			snprintf (os_public.directories.config, sizeof (os_public.directories.savegame), "%s/Appdata/Local", sav);
		}
	}

	log_Init ();

	{ auto result = SetProcessDPIAware(); assert (result); if (result == 0) { LOG ("SetProcessDPIAware() failed"); } }
	// SetThreadDpiAwarenessContext (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	{
		LARGE_INTEGER t;
		{ auto result = QueryPerformanceFrequency(&t); assert (result); if (result == 0) { LOG ("QueryPerformanceFrequency() failed [%s]", os_HResultToStr(GetLastError ())); return false; } }
		os_private.win32.ticks_per_microsecond = t.QuadPart / 1000000;
	}

	{
		const wchar_t window_class_name[] = L"Window Class";
		WNDCLASS window_class = { 0 };
		window_class.lpfnWndProc = os_Internal_WindowProcessMessage;
		window_class.lpszClassName = window_class_name;
		window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // This line might not be necessary. Prevents old problems of DC being usurped when resources are low
		window_class.hCursor = LoadCursor (NULL, IDC_ARROW);
		{ auto result = RegisterClass(&window_class); assert (result); if (result == 0) { LOG ("RegisterClass failed [%s]", os_HResultToStr(GetLastError ())); return false; } }

		os_private.win32.bitmap_info.bmiHeader.biSize = sizeof (os_private.win32.bitmap_info.bmiHeader);
		os_private.win32.bitmap_info.bmiHeader.biPlanes = 1;
		os_private.win32.bitmap_info.bmiHeader.biBitCount = 32;
		os_private.win32.bitmap_info.bmiHeader.biCompression = BI_RGB;
		{ os_private.win32.bitmap_device_context = CreateCompatibleDC (0); assert (os_private.win32.bitmap_device_context); if (os_private.win32.bitmap_device_context == NULL) { LOG ("CreateCompatibleDC failed"); return false; } }

		wchar_t title[1024];
		mbstowcs (title, window_title, 1023);

		os_public.window.is_fullscreen = false;
		{ os_private.win32.window_handle = CreateWindow (window_class_name, title, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL); assert (os_private.win32.window_handle); if(os_private.win32.window_handle == NULL) { LOG ("CreateWindow() failed [%s]", os_HResultToStr(GetLastError())); return false; } }
	}

	int pixel_format;
	{ os_private.win32.window_context = GetDC (os_private.win32.window_handle); assert (os_private.win32.window_context); if (os_private.win32.window_context == NULL) { LOG ("GetDC() failed"); return false; } }
	PIXELFORMATDESCRIPTOR format_descriptor = {
		sizeof (PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, 32,
		0,0,0,0,0,0,0,0,0,0,0,0,0,
		32, // DEPTH BUFFER
		0,0,0,0,0,0,0
	};
	{ pixel_format = ChoosePixelFormat (os_private.win32.window_context, &format_descriptor); assert (pixel_format); if (pixel_format == 0) { LOG ("ChoosePixelFormat() failed [%s]", os_HResultToStr(GetLastError())); return false; } }
	{ auto result = SetPixelFormat (os_private.win32.window_context, pixel_format, &format_descriptor); assert (result); if (result == FALSE) { LOG ("SetPixelFormat failed [%s]", os_HResultToStr(GetLastError())); return false; } }

	os_private.win32.gl_context = OpenGL2_1_Init_Win32 (os_private.win32.window_context); assert (os_private.win32.gl_context); if (os_private.win32.gl_context == NULL) { LOG ("Failed to initialize OpenGL"); return false; }

	// os_SetBackgroundColor (0x40, 0x3a, 0x4d);
	os_SetBackgroundColor (0,0,0);

#ifdef OSINTERFACE_COLOR_INDEX_MODE
	if (!os_CreateGLColorMap ()) { LOG ("Failed to create OpenGL color map shader"); return false;}
#endif
	
#ifdef OSINTERFACE_FRAME_BUFFER_SCALED
	os_WindowFrameBufferCalculateScale ();
#else
	#ifndef OSINTERFACE_NATIVE_GL_RENDERING
		os_private.frame_buffer.width = os_public.window.width;
		os_private.frame_buffer.height = os_public.window.height;
		os_private.frame_buffer.pixels = malloc (os_private.frame_buffer.width * os_private.frame_buffer.height * 4); assert (os_private.frame_buffer.pixels); if (os_private.frame_buffer.pixels == NULL) { LOG ("mallocing os_private.frame_buffer.pixels failed"); return false; } }
	#endif
#endif

	#ifdef OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE
	if (os_LogGLErrors ()) { LOG ("OpenGL had errors");}
	{ auto result = wglMakeCurrent (os_private.win32.window_context, NULL); assert (result); if (result == FALSE) { LOG ("wglMakeCurrent(NULL) to release the context for another thread failed [%s]", os_HResultToStr(GetLastError())); return false; } }
	#endif

	GetClientRect (os_private.win32.window_handle, &windowed_size);
	GetWindowRect (os_private.win32.window_handle, &windowed_pos);

	LOG ("os_Init completed successfully");

	return true;
}

bool os_GLMakeCurrent () {
	{ auto result = wglMakeCurrent (os_private.win32.window_context, os_private.win32.gl_context); assert (result); if (result == FALSE) { LOG ("wglMakeCurrent failed [%s]", os_HResultToStr(GetLastError())); return false; } }
	return true;
}

void os_SetBackgroundColor (uint8_t r, uint8_t g, uint8_t b) {
	{ os_private.win32.background_brush = CreateSolidBrush (RGB (r, g, b)); assert (os_private.win32.background_brush); if (os_private.win32.background_brush == NULL) { LOG ("CreateSolidBrush() failed"); return; } }
	os_private.background_color.r = r;
	os_private.background_color.g = g;
	os_private.background_color.b = b;
}

void os_Fullscreen (bool fullscreen) {
	if (fullscreen) {
		os_public.window.is_fullscreen = true;
		
		GetClientRect (os_private.win32.window_handle, &windowed_size);
		GetWindowRect (os_private.win32.window_handle, &windowed_pos);

		RECT fullscreen_rect;

		const auto monitor = MonitorFromWindow (os_private.win32.window_handle, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO info = {.cbSize = sizeof (MONITORINFO)};
		{
			auto result = GetMonitorInfo (monitor, &info); assert (result);
			if (!result) {
				LOG ("Failed to get monitor info.");
				HWND desktop_handle = GetDesktopWindow();
				if (desktop_handle) GetWindowRect(desktop_handle, &fullscreen_rect);
				else { fullscreen_rect.left = 0; fullscreen_rect.top = 0; fullscreen_rect.right = 800; fullscreen_rect.bottom = 600; }
			}
			else {
				fullscreen_rect = info.rcMonitor;
			}
		}
		
		SetWindowLongPtr (os_private.win32.window_handle, GWL_STYLE, (WS_POPUP | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW);
		SetWindowPos (os_private.win32.window_handle, NULL, fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right - fullscreen_rect.left, fullscreen_rect.bottom - fullscreen_rect.top, SWP_NOOWNERZORDER);

		RECT client_rect;
		GetClientRect (os_private.win32.window_handle, &client_rect);
		PostMessageA (os_private.win32.window_handle, WM_SIZE, SIZE_RESTORED, (client_rect.bottom << 16) + client_rect.right);
	} else {
		os_public.window.is_fullscreen = false;

		SetWindowLongPtr (os_private.win32.window_handle, GWL_STYLE, ~WS_POPUP & (WS_OVERLAPPEDWINDOW | WS_VISIBLE));
		SetWindowPos (os_private.win32.window_handle, NULL, windowed_pos.left, windowed_pos.top, windowed_size.right - windowed_size.left, windowed_size.bottom - windowed_size.top, SWP_NOOWNERZORDER);
		ShowWindow (os_private.win32.window_handle, SW_SHOW);
		
		RECT client_rect;
		GetClientRect (os_private.win32.window_handle, &client_rect);
		PostMessageA (os_private.win32.window_handle, WM_SIZE, SIZE_RESTORED, (client_rect.bottom << 16) + client_rect.right);
	}
}

void os_Maximize (bool maximize) {
	os_public.window.is_fullscreen = false;

	long args = ~WS_POPUP & (WS_OVERLAPPEDWINDOW);
	if (maximize) args |= WS_MAXIMIZE;

	SetWindowLongPtr (os_private.win32.window_handle, GWL_STYLE, args);
	ShowWindow (os_private.win32.window_handle, SW_SHOW);
}

void os_WindowSize (int width, int height) {
	os_Maximize (false);
	RECT rect;
	GetWindowRect (os_private.win32.window_handle, &rect);
	rect.right = rect.left + width;
	rect.bottom = rect.top + height;
	AdjustWindowRect (&rect, WS_OVERLAPPEDWINDOW, FALSE);
	if (rect.left < 0) {
		rect.right -= rect.left;
		rect.left = 0;
	}
	if (rect.top < 0) {
		rect.bottom -= rect.top;
		rect.top = 0;
	}
	SetWindowPos (os_private.win32.window_handle, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW);
}

void os_ShowCursor () {
	os_public.mouse.hidden = false;
	SetCursor (LoadCursor (NULL, IDC_ARROW));
}

void os_HideCursor () {
	os_public.mouse.hidden = true;
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(os_private.win32.window_handle, &p);

	RECT rect;
	GetClientRect(os_private.win32.window_handle, &rect);
	if (PtInRect(&rect, p)) SetCursor(NULL);
}

#define OS_INTERNAL_EVENTS_SIZE 256
static struct {
	uint32_t pushed, popped;
	os_event_t event[OS_INTERNAL_EVENTS_SIZE];
} events = {};

static inline void PushEvent (os_event_t event) { events.event[events.pushed++ % OS_INTERNAL_EVENTS_SIZE] = event; }

// Handle resize differently because otherwise we end up with a zillion resize events being pushed. Wait until all other events are processed, then, if there were any number of resizes, handle just the last one.
static struct {
	bool happened;
	int16_t w, h;
} internal_resize_event = {};
os_event_t os_NextEvent () {
	MSG message = {};
	internal_resize_event.happened = false;
	do {
		assert (events.pushed - events.popped < OS_INTERNAL_EVENTS_SIZE);
		if (events.pushed > events.popped)
			return events.event[events.popped++ % OS_INTERNAL_EVENTS_SIZE];
		if (PeekMessage (&message, NULL, 0, 0, PM_REMOVE)) {
			if (message.message == WM_QUIT) // WM_QUIT is a weird special case. It can't be passed into the MessageProcessing function by DispatchMessage. If using GetMessage it will only return 0 when it finally gets WM_QUIT, whereas with PeekMessage we must check it here.
				return (os_event_t){ .type = os_EVENT_QUIT, .exit_code = message.wParam };
			else DispatchMessage (&message); // Fills out event structure by passing on message to Internal_WindowProcessMessage
		}
	} while (events.pushed > events.popped);
	if (internal_resize_event.happened)
		return (os_event_t) { .type = os_EVENT_WINDOW_RESIZE, .width = os_public.window.w = internal_resize_event.w, .height = internal_resize_event.h };
    return (os_event_t){ .type = os_EVENT_NULL };
}

// void os_WindowPositionToFrameBufferPosition (int windowx, int windowy, int *resultx, int *resulty) {
// 	*resultx = (windowx - os_private.frame_buffer.left)   / os_private.frame_buffer.scale;
// 	*resulty = (windowy - os_private.frame_buffer.bottom) / os_private.frame_buffer.scale;
// }

void os_SendQuitEvent () {
	PostQuitMessage (0);
}

LRESULT CALLBACK os_Internal_WindowProcessMessage(HWND window_handle, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool has_focus = true;
    switch (message) {
		case WM_DESTROY: {
			PostQuitMessage (0);
		} break;
        
		case WM_PAINT: {
			ValidateRect (os_private.win32.window_handle, NULL);
		} break;
        
		case WM_SIZE: {
			internal_resize_event.happened = true;
			os_public.window.w = internal_resize_event.w = LOWORD(lParam);
			os_public.window.h = internal_resize_event.h = HIWORD(lParam);

			glViewport (0, 0, os_public.window.w, os_public.window.h);
			glMatrixMode (GL_PROJECTION);
			glLoadIdentity ();
			glOrtho (0, os_public.window.w-1, 0, os_public.window.h-1, -1, 1);

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
				os_private.new_frame_buffer.is_valid = true;
		#else // OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE
				os_private.frame_buffer.w = os_public.window.width;
				os_private.frame_buffer.h = os_public.window.height;
				os_private.frame_buffer.pixels = realloc (os_private.frame_buffer.pixels, os_private.frame_buffer.width * os_private.frame_buffer.height * sizeof (os_private.frame_buffer.pixels[0]));
		#endif
	#endif
#endif
		} break;

		case WM_KILLFOCUS: {
			has_focus = false;
			memset(os_public.keyboard, 0, sizeof(os_public.keyboard));
			os_public.mouse.buttons = 0;
			// Should I send a "released all inputs" event back?
		} break;

		case WM_SETFOCUS: has_focus = true; break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP: {
			if(has_focus) {
				bool key_is_down, key_was_down;
				key_is_down  = ((lParam & (1 << 31)) == 0);
				key_was_down = ((lParam & (1 << 30)) != 0);
				if(key_is_down != key_was_down) {
					if (wParam == VK_MENU) { // The ALT key is handled differently. It's a syskey, and both L and R trigger the same message so they're differentiated with lParam bit 24
						if (lParam & (1 << 24)) {
							wParam = VK_RMENU;
						}
						else {
							wParam = VK_LMENU;
						}
					}
					os_key_e key;
					switch (wParam) {
						case 'A': key = 'a'; break;
						case 'B': key = 'b'; break;
						case 'C': key = 'c'; break;
						case 'D': key = 'd'; break;
						case 'E': key = 'e'; break;
						case 'F': key = 'f'; break;
						case 'G': key = 'g'; break;
						case 'H': key = 'h'; break;
						case 'I': key = 'i'; break;
						case 'J': key = 'j'; break;
						case 'K': key = 'k'; break;
						case 'L': key = 'l'; break;
						case 'M': key = 'm'; break;
						case 'N': key = 'n'; break;
						case 'O': key = 'o'; break;
						case 'P': key = 'p'; break;
						case 'Q': key = 'q'; break;
						case 'R': key = 'r'; break;
						case 'S': key = 's'; break;
						case 'T': key = 't'; break;
						case 'U': key = 'u'; break;
						case 'V': key = 'v'; break;
						case 'W': key = 'w'; break;
						case 'X': key = 'x'; break;
						case 'Y': key = 'y'; break;
						case 'Z': key = 'z'; break;
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
						case VK_RETURN: key = os_KEY_ENTER; break;
						case VK_SPACE: key = ' '; break;
						case VK_ESCAPE: key = os_KEY_ESCAPE; break;
						case VK_LEFT: key = os_KEY_LEFT; break;
						case VK_RIGHT: key = os_KEY_RIGHT; break;
						case VK_DOWN: key = os_KEY_DOWN; break;
						case VK_UP: key = os_KEY_UP; break;
						case VK_RMENU: key = os_KEY_RALT; break;
						case VK_LMENU: key = os_KEY_LALT; break;
						case VK_HOME: key = os_KEY_HOME; break;
						case VK_END: key = os_KEY_END; break;
						case VK_PRIOR: key = os_KEY_PAGEUP; break;
						case VK_NEXT: key = os_KEY_PAGEDOWN; break;
						case VK_F1: key = os_KEY_F1; break;
						case VK_F2: key = os_KEY_F2; break;
						case VK_F3: key = os_KEY_F3; break;
						case VK_F4: key = os_KEY_F4; break;
						case VK_F5: key = os_KEY_F5; break;
						case VK_F6: key = os_KEY_F6; break;
						case VK_F7: key = os_KEY_F7; break;
						case VK_F8: key = os_KEY_F8; break;
						case VK_F9: key = os_KEY_F9; break;
						case VK_F10: key = os_KEY_F10; break;
						case VK_F11: key = os_KEY_F11; break;
						case VK_F12: key = os_KEY_F12; break;
						case VK_BACK: key = os_KEY_BACKSPACE; break;
						case VK_DELETE: key = os_KEY_DELETE; break;
						case VK_SHIFT: key = os_KEY_SHIFT; break;
						case VK_TAB: key = '	'; break;
						case VK_OEM_7: key = '\''; break;
						case VK_OEM_COMMA: key = ','; break;
						case VK_OEM_MINUS: key = '-'; break;
						case VK_OEM_PERIOD: key = '.'; break;
						case VK_OEM_2: key = '/'; break;
						case VK_OEM_1: key = ';'; break;
						case VK_OEM_PLUS: key = '='; break;
						case VK_OEM_4: key = '['; break;
						case VK_OEM_6: key = ']'; break;
						case VK_OEM_5: key = '\\'; break;
						case VK_OEM_3: key = '`'; break;
						default: key = os_KEY_INVALID; break;
					}
					os_public.keyboard[key] = key_is_down;
					PushEvent ((os_event_t){.type = key_is_down ? os_EVENT_KEY_PRESS : os_EVENT_KEY_RELEASE, .key = key});
				}
			}
		} break;

		case WM_MOUSEMOVE: {
			PushEvent ((os_event_t){.type = os_EVENT_MOUSE_MOVE, .previous_position = os_public.mouse.p, .new_position = { .x = LOWORD(lParam), .y = os_public.window.h - 1 - HIWORD(lParam)}});
			os_public.mouse.p.x = LOWORD(lParam);
			os_public.mouse.p.y = os_public.window.h - 1 - HIWORD(lParam);
		} break;

		case WM_LBUTTONDOWN: {
			os_public.mouse.buttons |=  os_MOUSE_LEFT;
			PushEvent ((os_event_t){.type = os_EVENT_MOUSE_BUTTON_PRESS, .button = {.button = os_MOUSE_LEFT, .p = {.x = LOWORD(lParam), .y = os_public.window.h - 1 - HIWORD(lParam)}}});
		} break;
		case WM_LBUTTONUP: {
			os_public.mouse.buttons &= ~os_MOUSE_LEFT;
			PushEvent ((os_event_t){.type = os_EVENT_MOUSE_BUTTON_RELEASE, .button = {.button = os_MOUSE_LEFT, .p = {.x = LOWORD(lParam), .y = os_public.window.h - 1 - HIWORD(lParam)}}});
		} break;

		case WM_RBUTTONDOWN: {
			os_public.mouse.buttons |=  os_MOUSE_RIGHT;
			PushEvent ((os_event_t){.type = os_EVENT_MOUSE_BUTTON_PRESS, .button = {.button = os_MOUSE_RIGHT, .p = {.x = LOWORD(lParam), .y = os_public.window.h - 1 - HIWORD(lParam)}}});
		} break;
		case WM_RBUTTONUP: {
			os_public.mouse.buttons &= ~os_MOUSE_RIGHT;
			PushEvent ((os_event_t){.type = os_EVENT_MOUSE_BUTTON_RELEASE, .button = {.button = os_MOUSE_RIGHT, .p = {.x = LOWORD(lParam), .y = os_public.window.h - 1 - HIWORD(lParam)}}});
		} break;

		case WM_MBUTTONDOWN: {
			os_public.mouse.buttons |=  os_MOUSE_MIDDLE;
			PushEvent ((os_event_t){.type = os_EVENT_MOUSE_BUTTON_PRESS, .button = {.button = os_MOUSE_MIDDLE, .p = {.x = LOWORD(lParam), .y = os_public.window.h - 1 - HIWORD(lParam)}}});
		} break;
		case WM_MBUTTONUP: {
			os_public.mouse.buttons &= ~os_MOUSE_MIDDLE;
			PushEvent ((os_event_t){.type = os_EVENT_MOUSE_BUTTON_RELEASE, .button = {.button = os_MOUSE_MIDDLE, .p = {.x = LOWORD(lParam), .y = os_public.window.h - 1 - HIWORD(lParam)}}});
		} break;

		case WM_XBUTTONDOWN: {
			if(GET_XBUTTON_WPARAM(wParam) == XBUTTON1) {
					 os_public.mouse.buttons |= os_MOUSE_X1;
			} else { os_public.mouse.buttons |= os_MOUSE_X2; }
			PushEvent ((os_event_t){.type = os_EVENT_MOUSE_BUTTON_PRESS, .button = {.button = os_MOUSE_X1, .p = {.x = LOWORD(lParam), .y = os_public.window.h - 1 - HIWORD(lParam)}}});
		} break;
		case WM_XBUTTONUP: {
			if(GET_XBUTTON_WPARAM(wParam) == XBUTTON1) {
					 os_public.mouse.buttons &= ~os_MOUSE_X1;
			} else { os_public.mouse.buttons &= ~os_MOUSE_X2; }
			PushEvent ((os_event_t){.type = os_EVENT_MOUSE_BUTTON_RELEASE, .button = {.button = os_MOUSE_X1, .p = {.x = LOWORD(lParam), .y = os_public.window.h - 1 - HIWORD(lParam)}}});
		} break;

		case WM_MOUSEWHEEL: {
			PushEvent ((os_event_t){.type = os_EVENT_MOUSE_SCROLL, .scrolled_up = !(wParam & 0b10000000000000000000000000000000)});
		} break;

		case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT && os_public.mouse.hidden)
                SetCursor(NULL);
            else
                return DefWindowProc(window_handle, message, wParam, lParam);
        } break;

		default: return DefWindowProc(window_handle, message, wParam, lParam);
    }

	return 0;
}

void os_WaitForScreenRefresh () {
	glFinish ();
	UpdateWindow (os_private.win32.window_handle); // Ensure that last frame has been presented
}

#ifdef OSINTERFACE_NATIVE_GL_RENDERING
void os_DrawScreen () { SwapBuffers (os_private.win32.window_context); }
#endif // OSINTERFACE_NATIVE_GL_RENDERING

int64_t os_uTime () {
	LARGE_INTEGER ticks;
	QueryPerformanceCounter(&ticks);
	return ticks.QuadPart / os_private.win32.ticks_per_microsecond;
}

void os_uSleepEfficient (int64_t microseconds) {
	if (microseconds > 0) Sleep (microseconds / 1000);
}

void os_uSleepPrecise (int64_t microseconds) {
	if (microseconds < 0) return;
	int64_t end, milliseconds;
	end = os_uTime () + microseconds;

	// Sleep an amount of milliseconds - inaccurate so we sleep 2 milliseconds less than we actually want
	milliseconds = microseconds / 1000 - 2;
	if (milliseconds < 0) goto tinysleep;
	Sleep (milliseconds);
tinysleep:
	while (os_uTime () < end) Sleep (0);
}

// Returns the refresh rate of the display on which the program window is.
// Returns 0 if the refresh rate cannot be retrieved.
int os_GetScreenRefreshRate () {
	uint16_t screen_refresh;
	screen_refresh = GetDeviceCaps(CreateCompatibleDC(NULL), VREFRESH);
	if (screen_refresh == 1) return 0; // 0 or 1 indicate the "default" refresh rate.
	return screen_refresh;
}

void os_Cleanup () {
	{ ULONG time; NtSetTimerResolution (0, FALSE, &time); }
}

void os_MessageBox_ (os_MessageBox_arguments arguments) {
	// char title[256];
	// GetWindowTextA (os_private.win32.window_handle, title, 255);
	MessageBoxA (NULL, arguments.message, arguments.title, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_TOPMOST);
}

os_char1024_t os_OpenFileDialog (const char *title) {
	os_char1024_t str = {};
	OPENFILENAMEA name = {
		.lStructSize = sizeof(OPENFILENAMEA),
		.hwndOwner = os_private.win32.window_handle,
		.lpstrFile = str.str,
		.nMaxFile = sizeof(str.str),
	};
	if (!GetOpenFileNameA (&name)) {
		LOG ("Failed to open file dialog");
		return (os_char1024_t){};
	}
	return str;
}

os_char1024_t os_SaveFileDialog (const char *title, const char *save_button_text, const char *filename_label, const char *default_filename) {
	os_char1024_t str;
	OPENFILENAMEA name = {
		.lStructSize = sizeof(OPENFILENAME),
		.lpstrFile = str.str,
		.nMaxFile = sizeof(str.str),
	};
	if (!GetSaveFileNameA (&name)) {
		return (os_char1024_t){};
	}
	return str;
}

void os_OpenURL (const char *url) { os_OpenFileBrowser (url); }

void os_OpenFileBrowser (const char *directory) {
	char command[1040];
	assert (strlen (directory) < 1024);
	snprintf (command, sizeof (command), "explorer \"%s\"", directory);
	char *c = command;
	while (*c) { if (*c == '/') *c = '\\'; ++c; }
	system (command);
}

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

	SwapBuffers (os_private.win32.window_context);
	ValidateRect (os_private.win32.window_handle, NULL);
	
	if (os_LogGLErrors ()) LOG ("Had GL errors");
}
#endif

bool os_LogGLErrors () {
	{
		auto context = wglGetCurrentContext ();
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
