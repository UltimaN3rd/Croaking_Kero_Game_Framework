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

os_public_t os_public = {};
os_private_t os_private = {};

#ifdef OSINTERFACE_COLOR_INDEX_MODE
void os_CreateGLColorMap ();
#endif

#ifdef OSINTERFACE_FRAME_BUFFER_SCALED
void os_SetWindowFrameBuffer (frame_buffer_pixel_t *pixels, int width, int height) {
	os_private.frame_buffer.has_been_set = true;
	os_private.frame_buffer.pixels = pixels;
	os_private.frame_buffer.width  = width;
	os_private.frame_buffer.height = height;
	os_WindowFrameBufferCalculateScale ();
}

os_intxy_t os_WindowPositionToScaledFrameBufferPosition (int windowx, int windowy) {
	return (os_intxy_t){
		(windowx - os_private.frame_buffer.left) / os_private.frame_buffer.scale,
		(windowy - os_private.frame_buffer.bottom) / os_private.frame_buffer.scale
	};
}

os_intxy_t os_ScaledFrameBufferPositionToWindowPosition (int framex, int framey) {
	return (os_intxy_t){
		framex * os_private.frame_buffer.scale + os_private.frame_buffer.left + os_private.frame_buffer.scale/2,
		framey * os_private.frame_buffer.scale + os_private.frame_buffer.bottom + os_private.frame_buffer.scale/2
	};
}
#endif




#ifdef WIN32

#include <Shlobj.h>
#include <assert.h>
#include "NtSetTimerResolution.h"
#include <GL/glew.h>
#include <GL/wglew.h>

LRESULT CALLBACK os_Internal_WindowProcessMessage(HWND window_handle, UINT message, WPARAM wParam, LPARAM lParam);

const char *os_HResultToStr (HRESULT result) {
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

	{
		auto gltemp = wglCreateContext (os_private.win32.window_context); assert (gltemp); if (gltemp == FALSE) { LOG ("wglCreateContext failed [%s]", os_HResultToStr(GetLastError())); return false; }
		{ auto result = wglMakeCurrent (os_private.win32.window_context, gltemp); assert (result); if (result == FALSE) { LOG ("wglMakeCurrent failed [%s]", os_HResultToStr(GetLastError())); return false; } }
		{ auto result = glewInit (); assert (result == GLEW_OK); if (result != GLEW_OK) { LOG ("glewInit failed [%s]", glewGetErrorString (result)); return false; } }
		{
			auto result = wglSwapIntervalEXT (-1); assert (result); if (result == FALSE) {
				LOG ("wglSwapIntervalEXT (-1) failed. Could not enable adaptive sync [%s]", os_HResultToStr(GetLastError()));
				{ auto result = wglSwapIntervalEXT (1); assert (result); if (result == FALSE) { LOG ("wglSwapIntervalEXT (1) failed. Could not enable VSync [%s]", os_HResultToStr(GetLastError())); } }
			}
			if (os_LogGLErrors ()) { LOG ("OpenGL had errors");}
		}
		{ os_private.win32.gl_context = wglCreateContextAttribsARB (os_private.win32.window_context, 0, (const int[]){WGL_CONTEXT_MAJOR_VERSION_ARB, 2, WGL_CONTEXT_MINOR_VERSION_ARB, 1, 0}); assert (os_private.win32.window_context); if (os_private.win32.gl_context == NULL) { LOG ("wglCreateContextAttribsARB failed [%s]", os_HResultToStr(GetLastError())); return false; } }
		{ auto result = wglMakeCurrent (os_private.win32.window_context, os_private.win32.gl_context); assert (result); if (result == FALSE) { LOG ("wglMakeCurrent failed [%s]", os_HResultToStr(GetLastError())); return false; } }
		if (os_LogGLErrors ()) { LOG ("OpenGL had errors");}
		LOG ("OpenGL version: [%s]", (const char*)glGetString (GL_VERSION));
		wglDeleteContext (gltemp);
	}

	// os_SetBackgroundColor (0x40, 0x3a, 0x4d);
	os_SetBackgroundColor (0,0,0);

#ifdef OSINTERFACE_COLOR_INDEX_MODE
	os_CreateGLColorMap ();
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
		
		RECT desktop_rect;
		HWND desktop_handle = GetDesktopWindow();
		if (desktop_handle) GetWindowRect(desktop_handle, &desktop_rect);
		else { desktop_rect.left = 0; desktop_rect.top = 0; desktop_rect.right = 800; desktop_rect.bottom = 600; }
		SetWindowLongPtr (os_private.win32.window_handle, GWL_STYLE, (WS_POPUP | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW);
		SetWindowPos (os_private.win32.window_handle, NULL, desktop_rect.left, desktop_rect.top, desktop_rect.right - desktop_rect.left, desktop_rect.bottom - desktop_rect.top, SWP_NOOWNERZORDER);
	} else {
		os_public.window.is_fullscreen = false;

		SetWindowLongPtr (os_private.win32.window_handle, GWL_STYLE, ~WS_POPUP & (WS_OVERLAPPEDWINDOW | WS_VISIBLE));
		ShowWindow (os_private.win32.window_handle, SW_SHOW);
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
	SetCursor (LoadCursor (NULL, IDC_ARROW));
	ShowCursor (true);
}

void os_HideCursor () {
	ShowCursor (false);
}

os_event_t os_NextEvent () {
    MSG message = {};
    os_private.win32.event.type = os_EVENT_INTERNAL;
	while (os_private.win32.event.type == os_EVENT_INTERNAL) {
		if (PeekMessage (&message, NULL, 0, 0, PM_REMOVE)) {
			// WM_QUIT is a weird special case. It can't be passed into the MessageProcessing function by DispatchMessage. If using GetMessage it will only return 0 when it finally gets WM_QUIT, whereas with PeekMessage we must check it here.
			if (message.message == WM_QUIT) {
				os_private.win32.event.type = os_EVENT_QUIT;
				os_private.win32.event.exit_code = message.wParam;
			}
			else {
				DispatchMessage (&message); // Fills out event structure by passing on message to Internal_WindowProcessMessage
			}
		}
		else os_private.win32.event.type = os_EVENT_NULL;
	}
    return os_private.win32.event;
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
            os_private.win32.event.type = os_EVENT_INTERNAL;

			// glClearColor (os_private.background_color.r / 255.f, os_private.background_color.g / 255.f, os_private.background_color.b / 255.f, 1);
			// glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// glDrawPixels (os_private.frame_buffer.width, os_private.frame_buffer.height, GL_BGRA, GL_UNSIGNED_BYTE, os_private.frame_buffer.pixels);

			// SwapBuffers (os_private.win32.window_context);

			ValidateRect (os_private.win32.window_handle, NULL);
		} break;
        
		case WM_SIZE: {
            os_private.win32.event.type = os_EVENT_WINDOW_RESIZE;

			os_private.win32.event.width  = os_public.window.w = LOWORD(lParam);
			os_private.win32.event.height = os_public.window.h = HIWORD(lParam);

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
					os_private.win32.event.key = key;
					if(key_is_down) {
						os_private.win32.event.type = os_EVENT_KEY_PRESS;
					}
					else {
						os_private.win32.event.type = os_EVENT_KEY_RELEASE;
					}
				}
			}
		} break;

		case WM_MOUSEMOVE: {
			os_private.win32.event.type = os_EVENT_MOUSE_MOVE;
			os_private.win32.event.previous_position.x = os_public.mouse.p.x;
			os_private.win32.event.previous_position.y = os_public.mouse.p.y;
			os_public.mouse.p.x = os_private.win32.event.new_position.x = LOWORD(lParam);
			os_public.mouse.p.y = os_private.win32.event.new_position.y = os_public.window.h - 1 - HIWORD(lParam);
		} break;

		case WM_LBUTTONDOWN: {
			os_public.mouse.buttons |=  os_MOUSE_LEFT;
			os_private.win32.event.type = os_EVENT_MOUSE_BUTTON_PRESS;
			os_private.win32.event.button.button = os_MOUSE_LEFT;
			os_private.win32.event.button.p.x = LOWORD(lParam);
			os_private.win32.event.button.p.y = os_public.window.h - 1 - HIWORD(lParam);
		} break;
		case WM_LBUTTONUP: {
			os_public.mouse.buttons &= ~os_MOUSE_LEFT;
			os_private.win32.event.type = os_EVENT_MOUSE_BUTTON_RELEASE;
			os_private.win32.event.button.button = os_MOUSE_LEFT;
			os_private.win32.event.button.p.x = LOWORD(lParam);
			os_private.win32.event.button.p.y = os_public.window.h - 1 - HIWORD(lParam);
		} break;

		case WM_RBUTTONDOWN: {
			os_public.mouse.buttons |=  os_MOUSE_RIGHT;
			os_private.win32.event.type = os_EVENT_MOUSE_BUTTON_PRESS;
			os_private.win32.event.button.button = os_MOUSE_RIGHT;
			os_private.win32.event.button.p.x = LOWORD(lParam);
			os_private.win32.event.button.p.y = os_public.window.h - 1 - HIWORD(lParam);
		} break;
		case WM_RBUTTONUP: {
			os_public.mouse.buttons &= ~os_MOUSE_RIGHT;
			os_private.win32.event.type = os_EVENT_MOUSE_BUTTON_RELEASE;
			os_private.win32.event.button.button = os_MOUSE_RIGHT;
			os_private.win32.event.button.p.x = LOWORD(lParam);
			os_private.win32.event.button.p.y = os_public.window.h - 1 - HIWORD(lParam);
		} break;

		case WM_MBUTTONDOWN: {
			os_public.mouse.buttons |=  os_MOUSE_MIDDLE;
			os_private.win32.event.type = os_EVENT_MOUSE_BUTTON_PRESS;
			os_private.win32.event.button.button = os_MOUSE_MIDDLE;
			os_private.win32.event.button.p.x = LOWORD(lParam);
			os_private.win32.event.button.p.y = os_public.window.h - 1 - HIWORD(lParam);
		} break;
		case WM_MBUTTONUP: {
			os_public.mouse.buttons &= ~os_MOUSE_MIDDLE;
			os_private.win32.event.type = os_EVENT_MOUSE_BUTTON_RELEASE;
			os_private.win32.event.button.button = os_MOUSE_MIDDLE;
			os_private.win32.event.button.p.x = LOWORD(lParam);
			os_private.win32.event.button.p.y = os_public.window.h - 1 - HIWORD(lParam);
		} break;

		case WM_XBUTTONDOWN: {
			if(GET_XBUTTON_WPARAM(wParam) == XBUTTON1) {
					 os_public.mouse.buttons |= os_MOUSE_X1;
			} else { os_public.mouse.buttons |= os_MOUSE_X2; }
			os_private.win32.event.type = os_EVENT_MOUSE_BUTTON_PRESS;
			os_private.win32.event.button.button = os_MOUSE_X1;
			os_private.win32.event.button.p.x = LOWORD(lParam);
			os_private.win32.event.button.p.y = os_public.window.h - 1 - HIWORD(lParam);
		} break;
		case WM_XBUTTONUP: {
			if(GET_XBUTTON_WPARAM(wParam) == XBUTTON1) {
					 os_public.mouse.buttons &= ~os_MOUSE_X1;
			} else { os_public.mouse.buttons &= ~os_MOUSE_X2; }
			os_private.win32.event.type = os_EVENT_MOUSE_BUTTON_RELEASE;
			os_private.win32.event.button.button = os_MOUSE_X1;
			os_private.win32.event.button.p.x = LOWORD(lParam);
			os_private.win32.event.button.p.y = os_public.window.h - 1 - HIWORD(lParam);
		} break;

		case WM_MOUSEWHEEL: {
			os_private.win32.event.type = os_EVENT_MOUSE_SCROLL;
			os_private.win32.event.scrolled_up = !(wParam & 0b10000000000000000000000000000000);
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
	os_char1024_t str;
	OPENFILENAMEA name = {
		.lStructSize = sizeof(OPENFILENAME),
		.lpstrFile = str.str,
		.nMaxFile = sizeof(str.str),
	};
	if (!GetOpenFileNameA (&name)) {
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

void os_OpenFileBrowser (const char *directory) {
	char command[1040];
	assert (strlen (directory) < 1024);
	snprintf (command, sizeof (command), "explorer \"%s\"", directory);
	char *c = command;
	while (*c) { if (*c == '/') *c = '\\'; ++c; }
	system (command);
}








#elif defined __linux__




#include <pwd.h>
#include <GL/glew.h>





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
		int attributes [] = { GLX_RGBA, GLX_DOUBLEBUFFER,
			GLX_RED_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_DEPTH_SIZE, 24,
			GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
			GLX_CONTEXT_MINOR_VERSION_ARB, 1,
			0 };
		os_private.x11.gl_visual_info = glXChooseVisual (os_private.x11.display, os_private.x11.screen, attributes); assert (os_private.x11.gl_visual_info); if (os_private.x11.gl_visual_info == NULL) { LOG ("GLX failed to select suitable visual. Is there an undefined attribute in the list?"); return false; };

		os_private.x11.gl_context = glXCreateContext (os_private.x11.display, os_private.x11.gl_visual_info, 0, true); assert (os_private.x11.gl_context); if (os_private.x11.gl_context == NULL) { LOG ("GLX failed to create context"); return false; }

        XSetWindowAttributes window_attributes;
        // window_attributes.background_pixel = 0xff808080;
        window_attributes.background_pixel = 0xff000000;
        window_attributes.colormap = XCreateColormap(os_private.x11.display, os_private.x11.root_window, os_private.x11.gl_visual_info->visual, AllocNone);
		window_attributes.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | FocusChangeMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
        os_private.x11.window = XCreateWindow (os_private.x11.display, os_private.x11.root_window, 0, 0, 1280, 720, 0, os_private.x11.gl_visual_info->depth, InputOutput, os_private.x11.gl_visual_info->visual, CWColormap | CWBackPixel | CWEventMask, &window_attributes);
		os_public.window.is_fullscreen = false;
        XMapWindow (os_private.x11.display, os_private.x11.window);
		os_GLMakeCurrent ();
		
		{ auto result = glewInit (); assert (result == GLEW_OK); if (result != GLEW_OK) { LOG ("glewInit failed [%s]", glewGetErrorString (result)); return false; } }

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

	const char *glx_extensions = glXQueryExtensionsString (os_private.x11.display,0);
	bool swap_tear_extension_present = false;
	const char *e = glx_extensions;

	do {
		e = strstr (e, "GLX_EXT_swap_control_tear");
		if (e == NULL) break;
		char next = *(e + strlen("GLX_EXT_swap_control_tear"));
		if (next == ' ' || next == '\0') {
			glXSwapIntervalEXT (os_private.x11.display, os_private.x11.window, -1);
			swap_tear_extension_present = true;
			break;
		}
		e += strlen ("GLX_EXT_swap_control_tear");
	} while (true);

	if (!swap_tear_extension_present) {
		LOG ("OpenGL extension 'GLX_EXT_swap_control_tear' not found.");
		bool swap_extension_present = false;
		e = glx_extensions;
		do {
			e = strstr (e, "GLX_EXT_swap_control");
			if (e == NULL) break;
			char next = *(e + strlen("GLX_EXT_swap_control"));
			if (next == ' ' || next == '\0') {
				glXSwapIntervalEXT (os_private.x11.display, os_private.x11.window, 1);
				swap_extension_present = true;
				break;
			}
			e += strlen("GLX_EXT_swap_control");
		} while (true);

		if (!swap_extension_present) {
			LOG ("OpenGL extension 'GLX_EXT_swap_control' not found.");
		}
	}

	// os_private.background_color.r = 0x40;
	// os_private.background_color.g = 0x3a;
	// os_private.background_color.b = 0x4d;
	os_SetBackgroundColor (0,0,0);

#ifdef OSINTERFACE_COLOR_INDEX_MODE
	os_CreateGLColorMap ();
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

void os_OpenFileBrowser (const char *directory) {
	char command[1040];
	assert (strlen (directory) < 1024);
	snprintf (command, sizeof (command), "xdg-open \"%s\"", directory);
	system (command);
}

void os_Cleanup () {}






#elif defined __APPLE__









#define GL_SILENCE_DEPRECATION
#include <GL/glew.h>
#include <CoreGraphics/CoreGraphics.h>
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <objc/objc.h>
#import <Foundation/Foundation.h>
#import <OpenGL/gl.h>
#import <stdint.h>
#import <mach/mach_time.h>

// Keycodes taken from HIToolbox/Events.h
enum {
  kVK_ANSI_A                    = 0x00,
  kVK_ANSI_S                    = 0x01,
  kVK_ANSI_D                    = 0x02,
  kVK_ANSI_F                    = 0x03,
  kVK_ANSI_H                    = 0x04,
  kVK_ANSI_G                    = 0x05,
  kVK_ANSI_Z                    = 0x06,
  kVK_ANSI_X                    = 0x07,
  kVK_ANSI_C                    = 0x08,
  kVK_ANSI_V                    = 0x09,
  kVK_ANSI_B                    = 0x0B,
  kVK_ANSI_Q                    = 0x0C,
  kVK_ANSI_W                    = 0x0D,
  kVK_ANSI_E                    = 0x0E,
  kVK_ANSI_R                    = 0x0F,
  kVK_ANSI_Y                    = 0x10,
  kVK_ANSI_T                    = 0x11,
  kVK_ANSI_1                    = 0x12,
  kVK_ANSI_2                    = 0x13,
  kVK_ANSI_3                    = 0x14,
  kVK_ANSI_4                    = 0x15,
  kVK_ANSI_6                    = 0x16,
  kVK_ANSI_5                    = 0x17,
  kVK_ANSI_Equal                = 0x18,
  kVK_ANSI_9                    = 0x19,
  kVK_ANSI_7                    = 0x1A,
  kVK_ANSI_Minus                = 0x1B,
  kVK_ANSI_8                    = 0x1C,
  kVK_ANSI_0                    = 0x1D,
  kVK_ANSI_RightBracket         = 0x1E,
  kVK_ANSI_O                    = 0x1F,
  kVK_ANSI_U                    = 0x20,
  kVK_ANSI_LeftBracket          = 0x21,
  kVK_ANSI_I                    = 0x22,
  kVK_ANSI_P                    = 0x23,
  kVK_ANSI_L                    = 0x25,
  kVK_ANSI_J                    = 0x26,
  kVK_ANSI_Quote                = 0x27,
  kVK_ANSI_K                    = 0x28,
  kVK_ANSI_Semicolon            = 0x29,
  kVK_ANSI_Backslash            = 0x2A,
  kVK_ANSI_Comma                = 0x2B,
  kVK_ANSI_Slash                = 0x2C,
  kVK_ANSI_N                    = 0x2D,
  kVK_ANSI_M                    = 0x2E,
  kVK_ANSI_Period               = 0x2F,
  kVK_ANSI_Grave                = 0x32,
  kVK_ANSI_KeypadDecimal        = 0x41,
  kVK_ANSI_KeypadMultiply       = 0x43,
  kVK_ANSI_KeypadPlus           = 0x45,
  kVK_ANSI_KeypadClear          = 0x47,
  kVK_ANSI_KeypadDivide         = 0x4B,
  kVK_ANSI_KeypadEnter          = 0x4C,
  kVK_ANSI_KeypadMinus          = 0x4E,
  kVK_ANSI_KeypadEquals         = 0x51,
  kVK_ANSI_Keypad0              = 0x52,
  kVK_ANSI_Keypad1              = 0x53,
  kVK_ANSI_Keypad2              = 0x54,
  kVK_ANSI_Keypad3              = 0x55,
  kVK_ANSI_Keypad4              = 0x56,
  kVK_ANSI_Keypad5              = 0x57,
  kVK_ANSI_Keypad6              = 0x58,
  kVK_ANSI_Keypad7              = 0x59,
  kVK_ANSI_Keypad8              = 0x5B,
  kVK_ANSI_Keypad9              = 0x5C
};

/* keycodes for keys that are independent of keyboard layout*/
enum {
  kVK_Return                    = 0x24,
  kVK_Tab                       = 0x30,
  kVK_Space                     = 0x31,
  kVK_Delete                    = 0x33,
  kVK_Escape                    = 0x35,
  kVK_Command                   = 0x37,
  kVK_Shift                     = 0x38,
  kVK_CapsLock                  = 0x39,
  kVK_Option                    = 0x3A,
  kVK_Control                   = 0x3B,
  kVK_RightShift                = 0x3C,
  kVK_RightOption               = 0x3D,
  kVK_RightControl              = 0x3E,
  kVK_Function                  = 0x3F,
  kVK_F17                       = 0x40,
  kVK_VolumeUp                  = 0x48,
  kVK_VolumeDown                = 0x49,
  kVK_Mute                      = 0x4A,
  kVK_F18                       = 0x4F,
  kVK_F19                       = 0x50,
  kVK_F20                       = 0x5A,
  kVK_F5                        = 0x60,
  kVK_F6                        = 0x61,
  kVK_F7                        = 0x62,
  kVK_F3                        = 0x63,
  kVK_F8                        = 0x64,
  kVK_F9                        = 0x65,
  kVK_F11                       = 0x67,
  kVK_F13                       = 0x69,
  kVK_F16                       = 0x6A,
  kVK_F14                       = 0x6B,
  kVK_F10                       = 0x6D,
  kVK_F12                       = 0x6F,
  kVK_F15                       = 0x71,
  kVK_Help                      = 0x72,
  kVK_Home                      = 0x73,
  kVK_PageUp                    = 0x74,
  kVK_ForwardDelete             = 0x75,
  kVK_F4                        = 0x76,
  kVK_End                       = 0x77,
  kVK_F2                        = 0x78,
  kVK_PageDown                  = 0x79,
  kVK_F1                        = 0x7A,
  kVK_LeftArrow                 = 0x7B,
  kVK_RightArrow                = 0x7C,
  kVK_DownArrow                 = 0x7D,
  kVK_UpArrow                   = 0x7E
};

/* ISO keyboards only*/
enum {
  kVK_ISO_Section               = 0x0A
};

/* JIS keyboards only*/
enum {
  kVK_JIS_Yen                   = 0x5D,
  kVK_JIS_Underscore            = 0x5E,
  kVK_JIS_KeypadComma           = 0x5F,
  kVK_JIS_Eisu                  = 0x66,
  kVK_JIS_Kana                  = 0x68
};

enum {
	OSXUserEvent_WindowClose, OSXUserEvent_WindowResize, OSXUserEvent_LostFocus, OSXUserEvent_EnterFullscreen, OSXUserEvent_ExitFullscreen,
};

@interface AppDelegate : NSObject<NSApplicationDelegate>
-(NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender;
@end
@implementation AppDelegate
-(NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
	NSEvent *event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:(NSPoint){0,0} modifierFlags:0 timestamp:[[NSProcessInfo processInfo] systemUptime] windowNumber:[os_private.osx.window windowNumber] context:nil subtype:OSXUserEvent_WindowClose data1:0 data2:0];
	assert (event);
	[NSApp postEvent:event atStart:true];
	return NSTerminateCancel;
}
@end

static bool live_resizing = false;

#define TRUESIZE [[os_private.osx.window contentView] convertRectToBacking:[[os_private.osx.window contentView] bounds]].size
// #define TRUESIZE [os_private.osx.window convertRectToScreen:[[os_private.osx.window contentView] bounds]].size

@interface WindowDelegate : NSObject<NSWindowDelegate>
-(void)windowWillClose:(NSNotification*)notification;
-(void)windowDidResignKey:(NSNotification*)notification;
-(void)windowDidResize:(NSNotification *)notification;
-(void)windowWillStartLiveResize:(NSNotification *)notification;
-(void)windowDidEndLiveResize:(NSNotification *)notification;
@end
@implementation WindowDelegate
-(void)windowWillClose:(NSNotification *)notification {
	NSEvent *event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:(NSPoint){0,0} modifierFlags:0 timestamp:[[NSProcessInfo processInfo] systemUptime] windowNumber:[os_private.osx.window windowNumber] context:nil subtype:OSXUserEvent_WindowClose data1:0 data2:0];
	assert (event);
	[NSApp postEvent:event atStart:true];
}
-(void)windowDidResignKey:(NSNotification*)notification {
	NSEvent *event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:(NSPoint){0,0} modifierFlags:0 timestamp:[[NSProcessInfo processInfo] systemUptime] windowNumber:[os_private.osx.window windowNumber] context:nil subtype:OSXUserEvent_LostFocus data1:0 data2:0];
	assert (event);
	[NSApp postEvent:event atStart:false];
}
-(void)windowDidResize:(NSNotification *)notification {
	if (live_resizing) return;
	NSSize size = TRUESIZE;
	NSEvent *event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:(NSPoint){0,0} modifierFlags:0 timestamp:[[NSProcessInfo processInfo] systemUptime] windowNumber:[os_private.osx.window windowNumber] context:nil subtype:OSXUserEvent_WindowResize data1:size.width data2:size.height];
	assert (event);
	[NSApp postEvent:event atStart:false];
}
-(void)windowWillStartLiveResize:(NSNotification *)notification {
	live_resizing = true;
}
-(void)windowDidEndLiveResize:(NSNotification *)notification {
	live_resizing = false;
	NSSize size = TRUESIZE;
	NSEvent *event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:(NSPoint){0,0} modifierFlags:0 timestamp:[[NSProcessInfo processInfo] systemUptime] windowNumber:[os_private.osx.window windowNumber] context:nil subtype:OSXUserEvent_WindowResize data1:size.width data2:size.height];
	assert (event);
	[NSApp postEvent:event atStart:false];
}
@end

static float ticks_to_nanosec = 1;

bool os_Init (const char *window_title) {
	assert (window_title);
	assert ([NSThread isMainThread]);

	// Platform directories
	NSString *path = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES)[0];
	const char *cpath = [path cStringUsingEncoding:NSUTF8StringEncoding];
    snprintf (os_public.directories.savegame, sizeof (os_public.directories.savegame), "%s", cpath);
    snprintf (os_public.directories.config, sizeof (os_public.directories.config), "%s/Preferences", cpath);

	log_Init ();

	mach_timebase_info_data_t timebase;
	mach_timebase_info (&timebase);
	ticks_to_nanosec = (double)timebase.numer / timebase.denom;

	os_public.window.is_fullscreen = false;
	snprintf (os_public.window.title, sizeof (os_public.window.title)-1, "%s", window_title);

	os_private.osx.application = [NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
	AppDelegate *appDelegate = [AppDelegate new];
	[NSApp setDelegate:appDelegate];
	[NSApp finishLaunching];
	id menuBar = [NSMenu new];
	id menuItemApp = [NSMenuItem new];
	[menuBar addItem:menuItemApp];
	[NSApp setMainMenu:menuBar];
	id appMenu = [NSMenu new];
	[appMenu addItem:[[NSMenuItem alloc] initWithTitle:[@"Quit " stringByAppendingString:[[NSProcessInfo processInfo] processName]] action:@selector(terminate:) keyEquivalent:@"q"]];
	[menuItemApp setSubmenu:appMenu];
	os_private.osx.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,800,600) styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
	[os_private.osx.window setReleasedWhenClosed:NO];
	[os_private.osx.window setDelegate:[WindowDelegate new]];
	[os_private.osx.window setTitle:[NSString stringWithCString:os_public.window.title encoding:NSASCIIStringEncoding]];
	[os_private.osx.window setFrameAutosaveName:[os_private.osx.window title]]; // Autosave and load window position and size
	NSOpenGLPixelFormatAttribute glAttributes[] = {
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFANoRecovery,
		NSOpenGLPFASampleBuffers, 1,
		NSOpenGLPFASamples, 4,
		NSOpenGLPFADepthSize, 32,
		// NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
		0,
	};
	NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:glAttributes];
	os_private.osx.gl_context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
	[os_private.osx.gl_context setView:[os_private.osx.window contentView]];
	[os_private.osx.gl_context makeCurrentContext];
	glewExperimental = GL_TRUE;
	{ auto result = glewInit (); assert (result == GLEW_OK); if (result != GLEW_OK) { LOG ("glewInit failed [%s]", glewGetErrorString (result)); return false; } }
	[os_private.osx.gl_context setValues:(GLint[]){1} forParameter:NSOpenGLContextParameterSwapInterval];
	[os_private.osx.window makeKeyAndOrderFront:os_private.osx.window];
	[os_private.osx.window setAcceptsMouseMovedEvents:YES];
	os_SetBackgroundColor (0,0,0);
	[NSApp activate];
	// NSSize size = [[os_private.osx.window contentView] frame].size;
	NSSize size = TRUESIZE;
	NSEvent *event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:(NSPoint){0,0} modifierFlags:0 timestamp:[[NSProcessInfo processInfo] systemUptime] windowNumber:[os_private.osx.window windowNumber] context:nil subtype:OSXUserEvent_WindowResize data1:size.width data2:size.height];
	assert (event);
	[NSApp postEvent:event atStart:false];
#ifdef OSINTERFACE_FRAME_BUFFER_SCALED
	os_WindowFrameBufferCalculateScale ();
#endif

	GLint swap_interval = 1;
	[os_private.osx.gl_context setValues:&swap_interval forParameter:NSOpenGLContextParameterSwapInterval];
	char msg[16];
	snprintf (msg, sizeof (msg), "swap: %d", swap_interval);

#ifdef OSINTERFACE_COLOR_INDEX_MODE
	os_CreateGLColorMap ();
#endif
	[os_private.osx.gl_context update];

	pthread_mutex_init(&os_private.osx.opengl_mutex, NULL);

	return true;
}

void os_SetBackgroundColor (uint8_t r, uint8_t g, uint8_t b) {
	os_private.background_color.r = r;
	os_private.background_color.g = g;
	os_private.background_color.b = b;
	os_private.background_color.a = 255;
	[os_private.osx.window setBackgroundColor:[NSColor colorWithRed:r/255.f green:g/255.f blue:b/255.f alpha:1]];
}

void os_SendQuitEvent () {
	[[NSApp sharedApplication] terminate:nil];
}

os_event_t os_NextEvent () {
	assert ([NSThread isMainThread]);
	os_event_t event = {.type = os_EVENT_INTERNAL};
	bool key_is_down = true;
	bool pass_event_along = true;
	while (event.type == os_EVENT_INTERNAL) {
		NSEvent *e = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
		if (!e) {
			event = (os_event_t) {.type = os_EVENT_NULL};
			break;
		}
		switch ([e type]) {
			case NSEventTypeLeftMouseDragged:
			case NSEventTypeRightMouseDragged:
			case NSEventTypeOtherMouseDragged:
			case NSEventTypeMouseMoved: {
				NSPoint ep = [[os_private.osx.window contentView] convertPointToBacking:[e locationInWindow]];
				os_vec2i p = {ep.x, ep.y};
				// if (p.x < 0) p.x = 0;
				// if (p.x > os_public.window.w-1) p.x = os_public.window.w-1;
				// if (p.y < 0) p.y = 0;
				// if (p.y > os_public.window.h-1) p.y = os_public.window.h-1;
				if (p.x < 0 || p.x > os_public.window.w-1 || p.y < 0 || p.y > os_public.window.h-1) break;
				event = (os_event_t){
					.type = os_EVENT_MOUSE_MOVE,
					.previous_position = os_public.mouse.p,
					.new_position = p,
				};
				os_public.mouse.p = p;
			} break;

			case NSEventTypeLeftMouseUp:
			case NSEventTypeRightMouseUp:
			case NSEventTypeOtherMouseUp:
				event.type = os_EVENT_MOUSE_BUTTON_RELEASE;
				key_is_down = false;
			case NSEventTypeLeftMouseDown:
			case NSEventTypeRightMouseDown:
			case NSEventTypeOtherMouseDown: {
				NSPoint ep = [[os_private.osx.window contentView] convertPointToBacking:[e locationInWindow]];
				os_vec2i p = {ep.x, ep.y};
				event.button.p = p;
				if (key_is_down) {
					if (p.x < 0 || p.x > os_public.window.w-1 || p.y < 0 || p.y > os_public.window.h-1) break; // Don't process clicks outside window active area. However, do process releases.
					event.type = os_EVENT_MOUSE_BUTTON_PRESS;
				}
				unsigned int index = [e buttonNumber];
				if (index > 4) {
					event.type = os_EVENT_NULL;
					break;
				}
				switch (index) {
					case 0: event.button.button = os_MOUSE_LEFT; break;
					case 1: event.button.button = os_MOUSE_RIGHT; break;
					case 2: event.button.button = os_MOUSE_MIDDLE; break;
					case 3: event.button.button = os_MOUSE_X1; break;
					case 4: event.button.button = os_MOUSE_X2; break;
				}
				if (key_is_down) os_public.mouse.buttons |= event.button.button;
				else os_public.mouse.buttons &= ~event.button.button;
			} break;

			case NSEventTypeScrollWheel: {
				auto d = [e scrollingDeltaY];
				if (d == 0) break;
				event.type = os_EVENT_MOUSE_SCROLL;
				if (d > 0) event.scrolled_up = true;
				else event.scrolled_up = false;
			} break;

			case NSEventTypeKeyUp:
				event.type = os_EVENT_KEY_RELEASE;
				key_is_down = false;
			case NSEventTypeKeyDown: {
				pass_event_along = false;
				if ([e isARepeat]) {
					event.type = os_EVENT_NULL;
					break;
				}
				os_key_e key;
				switch([e keyCode]) {
					case kVK_ANSI_A: key = 'a'; break;
					case kVK_ANSI_B: key = 'b'; break;
					case kVK_ANSI_C: key = 'c'; break;
					case kVK_ANSI_D: key = 'd'; break;
					case kVK_ANSI_E: key = 'e'; break;
					case kVK_ANSI_F: key = 'f'; break;
					case kVK_ANSI_G: key = 'g'; break;
					case kVK_ANSI_H: key = 'h'; break;
					case kVK_ANSI_I: key = 'i'; break;
					case kVK_ANSI_J: key = 'j'; break;
					case kVK_ANSI_K: key = 'k'; break;
					case kVK_ANSI_L: key = 'l'; break;
					case kVK_ANSI_M: key = 'm'; break;
					case kVK_ANSI_N: key = 'n'; break;
					case kVK_ANSI_O: key = 'o'; break;
					case kVK_ANSI_P: key = 'p'; break;
					case kVK_ANSI_Q: key = 'q'; break;
					case kVK_ANSI_R: key = 'r'; break;
					case kVK_ANSI_S: key = 's'; break;
					case kVK_ANSI_T: key = 't'; break;
					case kVK_ANSI_U: key = 'u'; break;
					case kVK_ANSI_V: key = 'v'; break;
					case kVK_ANSI_W: key = 'w'; break;
					case kVK_ANSI_X: key = 'x'; break;
					case kVK_ANSI_Y: key = 'y'; break;
					case kVK_ANSI_Z: key = 'z'; break;
					case kVK_ANSI_0: key = '0'; break;
					case kVK_ANSI_1: key = '1'; break;
					case kVK_ANSI_2: key = '2'; break;
					case kVK_ANSI_3: key = '3'; break;
					case kVK_ANSI_4: key = '4'; break;
					case kVK_ANSI_5: key = '5'; break;
					case kVK_ANSI_6: key = '6'; break;
					case kVK_ANSI_7: key = '7'; break;
					case kVK_ANSI_8: key = '8'; break;
					case kVK_ANSI_9: key = '9'; break;
					case kVK_Return: key = os_KEY_ENTER; break;
					case kVK_Space: key = ' '; break;
					case kVK_Escape: key = os_KEY_ESCAPE; break;
					case kVK_LeftArrow: key = os_KEY_LEFT; break;
					case kVK_RightArrow: key = os_KEY_RIGHT; break;
					case kVK_DownArrow: key = os_KEY_DOWN; break;
					case kVK_UpArrow: key = os_KEY_UP; break;
					case kVK_Option: key = os_KEY_LALT; break;
					case kVK_RightOption: key = os_KEY_RALT; break;
					case kVK_Control: case kVK_RightControl: key = os_KEY_CTRL; abort ();  break;
					case kVK_F1: key = os_KEY_F1; break;
					case kVK_F2: key = os_KEY_F2; break;
					case kVK_F3: key = os_KEY_F3; break;
					case kVK_F4: key = os_KEY_F4; break;
					case kVK_F5: key = os_KEY_F5; break;
					case kVK_F6: key = os_KEY_F6; break;
					case kVK_F7: key = os_KEY_F7; break;
					case kVK_F8: key = os_KEY_F8; break;
					case kVK_F9: key = os_KEY_F9; break;
					case kVK_F10: key = os_KEY_F10; break;
					case kVK_F11: key = os_KEY_F11; break;
					case kVK_F12: key = os_KEY_F12; break;
					case kVK_Delete: key = os_KEY_BACKSPACE; break;
					case kVK_ForwardDelete: key = os_KEY_DELETE; break;
					case kVK_Shift: case kVK_RightShift: key = os_KEY_SHIFT; break;
					case kVK_Tab: key = '	'; break;
					case kVK_ANSI_Quote: key = '\''; break;
					case kVK_ANSI_Comma: key = ','; break;
					case kVK_ANSI_Minus: key = '-'; break;
					case kVK_ANSI_Period: key = '.'; break;
					case kVK_ANSI_Slash: key = '/'; break;
					case kVK_ANSI_Semicolon: key = ';'; break;
					case kVK_ANSI_Equal: key = '='; break;
					case kVK_ANSI_LeftBracket: key = '['; break;
					case kVK_ANSI_RightBracket: key = ']'; break;
					case kVK_ANSI_Backslash: key = '\\'; break;
					case kVK_ANSI_Grave: key = '`'; break;
					default: key = os_KEY_INVALID; break;
				}
				os_public.keyboard[key] = key_is_down;
				event.key = key;
				if(key_is_down) {
					event.type = os_EVENT_KEY_PRESS;
				}
			} break;

			case NSEventTypeFlagsChanged: {
				typedef union {
					struct {
						uint8_t alpha_shift:1;
						uint8_t shift:1;
						uint8_t control:1;
						uint8_t alternate:1;
						uint8_t command:1;
						uint8_t numeric_pad:1;
						uint8_t help:1;
						uint8_t function:1;
					};
					uint8_t mask;
				} osx_event_modifiers_t;
				static osx_event_modifiers_t mods_prev = {};
				osx_event_modifiers_t mods = {.mask = ([e modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask) >> 16};
				if (mods.alpha_shift ^ mods_prev.alpha_shift) {
					event.key = os_KEY_SHIFT;
					if (mods.alpha_shift) {
						event.type = os_EVENT_KEY_PRESS;
						os_public.keyboard[os_KEY_SHIFT] = true;
					}
					else {
						event.type = os_EVENT_KEY_RELEASE;
						os_public.keyboard[os_KEY_SHIFT] = false;
					}
				}
				if (mods.shift ^ mods_prev.shift) {
					event.key = os_KEY_SHIFT;
					if (mods.shift) {
						event.type = os_EVENT_KEY_PRESS;
						os_public.keyboard[os_KEY_SHIFT] = true;
					}
					else {
						event.type = os_EVENT_KEY_RELEASE;
						os_public.keyboard[os_KEY_SHIFT] = false;
					}
				}
				if (mods.command ^ mods_prev.command) {
					event.key = os_KEY_CTRL;
					if (mods.command) {
						event.type = os_EVENT_KEY_PRESS;
						os_public.keyboard[os_KEY_CTRL] = true;
					}
					else {
						event.type = os_EVENT_KEY_RELEASE;
						os_public.keyboard[os_KEY_CTRL] = false;
					}
				}
				mods_prev = mods;
			} break;

			case NSEventTypeApplicationDefined: {
				switch ([e subtype]) {
					case OSXUserEvent_WindowResize: {
						int w = [e data1];
						int h = [e data2];
						event = (os_event_t) {.type = os_EVENT_WINDOW_RESIZE, .width = w, .height = h};
						os_public.window.w = event.width = w;
						os_public.window.h = event.height = h;

#ifndef OSINTERFACE_NATIVE_GL_RENDERING
						pthread_mutex_lock(&os_private.osx.opengl_mutex);
						[os_private.osx.gl_context update];
						// [os_private.osx.gl_context makeCurrentContext];
						glViewport (0, 0, os_public.window.w, os_public.window.h);
						// glMatrixMode (GL_PROJECTION);
						// glLoadIdentity ();
						// glOrtho (0, os_public.window.w-1, 0, os_public.window.h-1, -1, 1);

	#ifdef OSINTERFACE_FRAME_BUFFER_SCALED
						os_WindowFrameBufferCalculateScale ();
	#else
		#ifdef OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE
						while (os_private.new_frame_buffer.is_valid); // Wait for different thread to switch to most recently created frame buffer
						os_private.new_frame_buffer.width  = os_public.window.width;
						os_private.new_frame_buffer.height = os_public.window.height;
						os_private.new_frame_buffer.pixels = malloc (os_private.new_frame_buffer.width * os_private.new_frame_buffer.height * sizeof (os_private.new_frame_buffer.pixels[0]));
						assert (os_private.new_frame_buffer.pixels);
						os_private.new_frame_buffer.is_valid = true;
		#else // OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE
						os_private.frame_buffer.w = os_public.window.width;
						os_private.frame_buffer.h = os_public.window.height;
						os_private.frame_buffer.pixels = realloc (os_private.frame_buffer.pixels, os_private.frame_buffer.width * os_private.frame_buffer.height * sizeof (os_private.frame_buffer.pixels[0]));
						assert (os_private.new_frame_buffer.pixels);
		#endif
						glMatrixMode (GL_MODELVIEW);
						glLoadIdentity ();
						glPixelZoom (1, 1);
						glRasterPos2i (0, 0);
	#endif
						pthread_mutex_unlock(&os_private.osx.opengl_mutex);
#endif
					} break;

					case OSXUserEvent_WindowClose: {
						event.type = os_EVENT_QUIT;
						event.exit_code = 0;
					} break;

					case OSXUserEvent_LostFocus: {
						memset(os_public.keyboard, false, sizeof(os_public.keyboard));
						os_public.mouse.buttons = 0;
					} break;

					case OSXUserEvent_EnterFullscreen: os_Fullscreen (true); break;
					case OSXUserEvent_ExitFullscreen: os_Fullscreen (false); break;

					default: break;
				}
			} break;

			case NSEventTypeMouseEntered: {
				if (os_public.mouse.hidden)
					[NSCursor hide];
			} break;

			case NSEventTypeMouseExited: {
				if (os_public.mouse.hidden)
					[NSCursor unhide];
			} break;

			default: break;
		}
		if (pass_event_along)
	        [NSApp sendEvent:e];
        [NSApp updateWindows];
	}
	return event;
}

void os_WindowSize (int width, int height) {
	// [os_private.osx.window setFrameSize:(NSSize){width, height}];
	[os_private.osx.window setContentSize:(NSSize){width, height}];
	// [os_private.osx.gl_context update];
}

void os_Maximize (bool maximize) {
	os_public.window.is_fullscreen = false;
	
	if (maximize) {
		[os_private.osx.window setFrame:[[os_private.osx.window screen] visibleFrame] display:YES];
	}
	else {
		os_WindowSize (os_public.window.w, os_public.window.h);
	}
	// [os_private.osx.gl_context update];
}

#ifdef OSINTERFACE_NATIVE_GL_RENDERING
void os_DrawScreen () { [os_private.osx.gl_context flushBuffer]; }
#endif // OSINTERFACE_NATIVE_GL_RENDERING

void os_WaitForScreenRefresh () {
	glFinish ();
} // Ensure that last frame has been presented

void os_Fullscreen (bool fullscreen) {
	if (![NSThread isMainThread]) {
		NSEvent *event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:(NSPoint){0,0} modifierFlags:0 timestamp:[[NSProcessInfo processInfo] systemUptime] windowNumber:[os_private.osx.window windowNumber] context:nil subtype:(fullscreen?OSXUserEvent_EnterFullscreen:OSXUserEvent_ExitFullscreen) data1:0 data2:0];
		assert (event);
		[NSApp postEvent:event atStart:false];
		os_public.window.is_fullscreen = fullscreen; // Set this here even though the main thread will actually go into or out of fullscreen later, so that a thread can call os_Fulscreen() then check the value immediately and get the correct value
		return;
	}
	if (fullscreen) {
		if (!([NSApp currentSystemPresentationOptions] & NSApplicationPresentationFullScreen))
			[os_private.osx.window toggleFullScreen:nil];
		// if (![os_private.osx.window isInFullScreenMode])
		// 	[os_private.osx.window enterFullScreenMode:[os_private.osx.window screen] withOptions:nil];
	} else {
		if (([NSApp currentSystemPresentationOptions] & NSApplicationPresentationFullScreen))
			[os_private.osx.window toggleFullScreen:nil];
		// if ([os_private.osx.window isInFullScreenMode])
		// 	[os_private.osx.window exitFullScreenModeWithOptions:nil];
	}
}

int64_t os_uTime () {
	uint64_t t = mach_absolute_time();
	return t * ticks_to_nanosec / 1000;
}

void os_uSleepEfficient (int64_t microseconds) {
	// if (microseconds > 0) usleep (microseconds);
	if (microseconds <= 0) return;
	os_uSleepPrecise (microseconds);
}

void os_uSleepPrecise (int64_t microseconds) {
	mach_wait_until(mach_absolute_time() + microseconds*1000.0/ticks_to_nanosec);
}

void os_ShowCursor () {
	os_public.mouse.hidden = false;
	[NSCursor unhide];
}

// void os_HideCursor () { [NSCursor hide]; }
void os_HideCursor () {
	os_public.mouse.hidden = true;
	[NSCursor hide];
}

// Returns the refresh rate of the display on which the program window is.
// Returns 0 if the refresh rate cannot be retrieved.
int os_GetScreenRefreshRate () {
	CGDisplayModeRef mode = CGDisplayCopyDisplayMode([[[[NSScreen mainScreen] deviceDescription] objectForKey:@"NSScreenNumber"] intValue]);
	int rate = CGDisplayModeGetRefreshRate(mode);
	CGDisplayModeRelease (mode);
	return rate;
}

void os_MessageBox_ (os_MessageBox_arguments arguments) {
	if (arguments.message == NULL) arguments.message = "";
	if (arguments.title == NULL) arguments.title = "";
	NSAlert *alert = [NSAlert new];
	[alert setMessageText:[NSString stringWithCString:arguments.message encoding:NSASCIIStringEncoding]];
	[alert setInformativeText:[NSString stringWithCString:arguments.title encoding:NSASCIIStringEncoding]];
	[alert addButtonWithTitle:@"Okay"];
	[alert runModal];
	// [alert beginSheetModalForWindow:os_private.osx.window completionHandler:nil];
	[alert release];
}

bool os_GLMakeCurrent () {
	pthread_mutex_lock(&os_private.osx.opengl_mutex);
	// [os_private.osx.gl_context update];
	[os_private.osx.gl_context makeCurrentContext];
	pthread_mutex_unlock(&os_private.osx.opengl_mutex);
	return true;
}

os_char1024_t os_OpenFileDialog (const char *title) {
	NSOpenPanel *win = [NSOpenPanel openPanel];
	win.title = [NSString stringWithCString:title encoding:NSASCIIStringEncoding];
	[win setCanChooseFiles:YES];
	[win setCanChooseDirectories:YES];
	[win setAllowsOtherFileTypes:YES];
	[win setAllowsMultipleSelection:NO];
	if ([win runModal] == NSModalResponseOK) {
		os_char1024_t str;
		if (snprintf (str.str, 1024, "%s", [[[[win URLs][0] path] stringByResolvingSymlinksInPath] cStringUsingEncoding:NSUTF8StringEncoding]) < 1024) { // filename was too long
			[win release];
			return str;
		}
	};
	[win release];
	return (os_char1024_t){};
}

os_char1024_t os_SaveFileDialog (const char *title, const char *save_button_text, const char *filename_label, const char *default_filename) {
	NSSavePanel *win = [NSSavePanel savePanel];
	win.title = [NSString stringWithCString:title encoding:NSASCIIStringEncoding];
	win.prompt = [NSString stringWithCString:save_button_text encoding:NSASCIIStringEncoding];
	win.nameFieldLabel = [NSString stringWithCString:filename_label encoding:NSASCIIStringEncoding];
	win.nameFieldStringValue = [NSString stringWithCString:default_filename encoding:NSASCIIStringEncoding];
	win.showsTagField = NO;
	if ([win runModal] == NSModalResponseOK) {
		os_char1024_t str;
		if (snprintf (str.str, 1024, "%s", [[[[win URL] path] stringByResolvingSymlinksInPath] cStringUsingEncoding:NSUTF8StringEncoding]) < 1024) { // filename was too long
			[win release];
			return str;
		}
	};
	[win release];
	return (os_char1024_t){};
}

void os_OpenFileBrowser (const char *directory) {
	char command[1040];
	assert (strlen (directory) < 1024);
	snprintf (command, sizeof (command), "open \"%s\"", directory);
	system (command);
}

void os_Cleanup () {}


#endif // WIN32, __linux__, __APPLE__

#ifdef OSINTERFACE_FRAME_BUFFER_SCALED
void os_WindowFrameBufferCalculateScale () {
	if (!os_private.frame_buffer.has_been_set) return;

	int scalex, scaley;
	scalex = os_public.window.w / os_private.frame_buffer.width;
	scaley = os_public.window.h / os_private.frame_buffer.height;
	os_private.frame_buffer.scale  = Min (scalex, scaley);
	os_private.frame_buffer.left   = (os_public.window.w - os_private.frame_buffer.width  * os_private.frame_buffer.scale) / 2;
	os_private.frame_buffer.bottom = (os_public.window.h - os_private.frame_buffer.height * os_private.frame_buffer.scale) / 2;

	glViewport (0, 0, os_public.window.w, os_public.window.h);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	// glOrtho (0, os_public.window.w-1, 0, os_public.window.h-1, -1, 1);
	
	// glMatrixMode (GL_MODELVIEW);
	// glLoadIdentity ();
	// glPixelZoom (os_private.frame_buffer.scale, os_private.frame_buffer.scale);
	// glRasterPos2i (os_private.frame_buffer.left, os_private.frame_buffer.bottom);
}
#endif

#ifndef OSINTERFACE_NATIVE_GL_RENDERING
void os_ClearScreen () {
	memset (os_private.frame_buffer.pixels, 0, os_private.frame_buffer.width * os_private.frame_buffer.height);
}
#endif

#ifdef OSINTERFACE_COLOR_INDEX_MODE
void os_CreateGLColorMap () {
	if (os_LogGLErrors ()) LOG ("OpenGL error");
	auto vertex = glCreateShader (GL_VERTEX_SHADER);
	glShaderSource (vertex, 1,&(const char *){
R"(#version 120
uniform vec2 uni_scale;

void main()
{
	gl_Position = vec4(gl_Vertex.xy * uni_scale, 0,1);
	gl_TexCoord[0] = gl_MultiTexCoord0;
})"}, NULL);
	glCompileShader (vertex);
	int success = 0;
	glGetShaderiv (vertex, GL_COMPILE_STATUS, &success);
	if (!success) {
		char buf[512];
		glGetShaderInfoLog (vertex, 512, NULL, buf);
		LOG ("OpenGL vertex shader compilation error [%s]", buf);
		assert (false);
		return;
	}
	if (os_LogGLErrors ()) LOG ("OpenGL error");
	auto fragment = glCreateShader (GL_FRAGMENT_SHADER);
	glShaderSource (fragment, 1,&(const char *){
R"(#version 120
uniform sampler2D sam_texture;
uniform vec3 palette[256];

void main()
{
	gl_FragColor = vec4(  palette[  int(texture2D(sam_texture, gl_TexCoord[0].st).r * 255)  ]    ,1);
})"}, NULL);
	glCompileShader (fragment);
	success = 0;
	glGetShaderiv (fragment, GL_COMPILE_STATUS, &success);
	if (!success) {
		char buf[512];
		glGetShaderInfoLog (fragment, 512, NULL, buf);
		LOG ("OpenGL fragment shader compilation error [%s]", buf);
		assert (false);
		return;
	}
	auto program = glCreateProgram ();
	glAttachShader (program, vertex);
	glAttachShader (program, fragment);
	glLinkProgram (program);
	glGetProgramiv (program, GL_LINK_STATUS, &success);
	if (!success) {
		char buf[512];
		glGetProgramInfoLog (program, 512, NULL, buf);
		LOG ("OpenGL shader linking error [%s]", buf);
		return;
	}
	glDeleteShader (vertex);
	glDeleteShader (fragment);
	glUseProgram (program);
	if (os_LogGLErrors ()) LOG ("OpenGL error");
	float float_palette[256][3];
	for (int i = 0; i < 256; ++i) {
		float_palette[i][0] = palette[i][0] / 255.f;
		float_palette[i][1] = palette[i][1] / 255.f;
		float_palette[i][2] = palette[i][2] / 255.f;
	}
	glUniform3fv (glGetUniformLocation(program, "palette"), 256, (const GLfloat*)float_palette);

	os_private.gl.locations.vertex.scale = glGetUniformLocation(program, "uni_scale");
	os_private.gl.locations.fragment.texture = glGetUniformLocation(program, "sam_texture");

	if (os_LogGLErrors ()) LOG ("OpenGL error");
	glActiveTexture (GL_TEXTURE0);
	glGenTextures (1, &os_private.gl.texture);
	glBindTexture (GL_TEXTURE_2D, os_private.gl.texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glUniform1i (os_private.gl.locations.fragment.texture, 0);
	if (os_LogGLErrors ()) LOG ("OpenGL error");

	// os_private.gl.vao = 0;
	// glGenVertexArrays(1, &os_private.gl.vao);
	// assert (os_private.gl.vao); if (os_private.gl.vao == 0) { LOG ("OpenGL failed to generate VAO"); }
	// glBindVertexArray(os_private.gl.vao);
	// glGenBuffers (1, &os_private.gl.vbo);
	// glBindBuffer (GL_ARRAY_BUFFER, os_private.gl.vbo);
	// const GLfloat vertices[] = {
	// 	-1, -1, 0, 0,
	// 	-1, 1, 0, 1,
	// 	1, -1, 1, 0,
	// 	1, 1, 1, 1,
	// };

	// glEnableClientState (GL_VERTEX_ARRAY);
	// glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	// glVertexPointer (2, GL_FLOAT, sizeof(float)*4, vertices);
	// glTexCoordPointer (2, GL_FLOAT, sizeof(float)*4, &vertices[2]);

	// glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
	// glVertexAttribPointer (os_private.gl.locations.vertex.position, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (float), 0);
	// glEnableVertexAttribArray(os_private.gl.locations.vertex.position);
	// glVertexAttribPointer (os_private.gl.locations.vertex.tex_coord, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (float), (void*)(sizeof(float)*2));
	// glEnableVertexAttribArray(os_private.gl.locations.vertex.tex_coord);
	// glBindBuffer (GL_ARRAY_BUFFER, 0);

	glValidateProgram (program);
	glGetProgramiv (program, GL_VALIDATE_STATUS, &success);
	if (!success) {
		char buf[512];
		glGetProgramInfoLog (program, 512, NULL, buf);
		LOG ("OpenGL shader validation error [%s]", buf);
		assert (false);
		return;
	}
	if (os_LogGLErrors ()) LOG ("OpenGL error");
}
#endif

#ifndef OSINTERFACE_NATIVE_GL_RENDERING

void os_DrawScreen () {
	#if __APPLE__
		pthread_mutex_lock(&os_private.osx.opengl_mutex);
	#endif

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
	if (os_LogGLErrors ()) LOG ("Had GL errors");
	// glActiveTexture(GL_TEXTURE0);
	// glBindTexture (GL_TEXTURE_2D, os_private.gl.texture);
	if (os_LogGLErrors ()) LOG ("Had GL errors");
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, os_private.frame_buffer.width, os_private.frame_buffer.height, 0, GL_RED, GL_UNSIGNED_BYTE, os_private.frame_buffer.pixels);
	if (os_LogGLErrors ()) LOG ("Had GL errors");
	// glBindVertexArray (os_private.gl.vao);
	// if (os_LogGLErrors ()) LOG ("Had GL errors");
	// glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
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

	#if WIN32
		SwapBuffers (os_private.win32.window_context);
		ValidateRect (os_private.win32.window_handle, NULL);
	#elif __linux__
		glXSwapBuffers (os_private.x11.display, os_private.x11.window);
	#elif __APPLE__
		[os_private.osx.gl_context flushBuffer];
		pthread_mutex_unlock(&os_private.osx.opengl_mutex);
	#endif
	if (os_LogGLErrors ()) LOG ("Had GL errors");
}
#endif

bool os_LogGLErrors () {
	{
		#if WIN32
		auto context = wglGetCurrentContext ();
		#elif __linux__
		auto context = glXGetCurrentContext ();
		#elif __APPLE__
		auto context = [NSOpenGLContext currentContext];
		#endif
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