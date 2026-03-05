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
#define GL_SILENCE_DEPRECATION
#include <OpenGL/glu.h>
#import <Cocoa/Cocoa.h>

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

#define GL_SILENCE_DEPRECATION
#include <OpenGL/glu.h>
#import <Cocoa/Cocoa.h>
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

enum { OSXUserEvent_WindowClose, OSXUserEvent_WindowResize, OSXUserEvent_LostFocus, OSXUserEvent_EnterFullscreen, OSXUserEvent_ExitFullscreen, OSXUserEvent_MouseMoved, };

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

@interface WindowDelegate : NSObject<NSWindowDelegate> {
	BOOL cursor_hidden;
	BOOL mouse_in_bounds;
}
-(void)windowWillClose:(NSNotification*)notification;
-(void)windowDidResignKey:(NSNotification*)notification;
-(void)windowDidResize:(NSNotification *)notification;
-(void)windowWillStartLiveResize:(NSNotification *)notification;
-(void)windowDidEndLiveResize:(NSNotification *)notification;
-(void)My_ShowCursor;
-(void)My_HideCursor;
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

    if (cursor_hidden) {
        [NSCursor unhide];
        cursor_hidden = NO;
    }
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
- (void)mouseEntered:(NSEvent *)event {
	mouse_in_bounds = YES;
    if (os_public.mouse.hidden && !cursor_hidden) {
        [NSCursor hide];
        cursor_hidden = YES;
    }
}
- (void)mouseExited:(NSEvent *)event {
	mouse_in_bounds = NO;
    if (cursor_hidden) {
        [NSCursor unhide];
        cursor_hidden = NO;
    }
}
-(void)My_ShowCursor {
    if (cursor_hidden) {
        [NSCursor unhide];
        cursor_hidden = NO;
    }
}
-(void)My_HideCursor {
    if (!cursor_hidden && mouse_in_bounds) {
        [NSCursor hide];
        cursor_hidden = YES;
    }
}
- (void)mouseMoved:(NSEvent *)event {
    NSPoint p = [[os_private.osx.window contentView] convertPointToBacking:[event locationInWindow]];
	NSEvent *e = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:(NSPoint){0,0} modifierFlags:0 timestamp:[[NSProcessInfo processInfo] systemUptime] windowNumber:[os_private.osx.window windowNumber] context:nil subtype:OSXUserEvent_MouseMoved data1:(int)p.x data2:(int)p.y];
	assert (e);
	[NSApp postEvent:e atStart:false];
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

    NSView *content_view = [os_private.osx.window contentView];
    NSTrackingArea *tracking_area = [[NSTrackingArea alloc] initWithRect:content_view.bounds options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect owner:[os_private.osx.window delegate] userInfo:nil];
    [content_view addTrackingArea:tracking_area];

	[os_private.osx.window setTitle:@(os_public.window.title)];
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
	if (!os_CreateGLColorMap ()) { LOG ("Failed to create OpenGL color map shader"); return false;}
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
					case kVK_Control: case kVK_RightControl: key = os_KEY_CTRL; break;
					case kVK_Home: key = os_KEY_HOME; break;
					case kVK_End: key = os_KEY_END; break;
					case kVK_PageUp: key = os_KEY_PAGEUP; break;
					case kVK_PageDown: key = os_KEY_PAGEDOWN; break;
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

					case OSXUserEvent_MouseMoved: {
						const os_vec2i p = {[e data1], [e data2]};
						event = (os_event_t){
							.type = os_EVENT_MOUSE_MOVE,
							.previous_position = os_public.mouse.p,
							.new_position = p,
						};
						os_public.mouse.p = p;
					} break;

					default: break;
				}
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
	[[os_private.osx.window delegate] My_ShowCursor];
}

void os_HideCursor () {
	os_public.mouse.hidden = true;
	[[os_private.osx.window delegate] My_HideCursor];
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
	[alert setMessageText:@(arguments.message)];
	[alert setInformativeText:@(arguments.title)];
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
	win.title = @(title);
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
	win.title = @(title);
	win.prompt = @(save_button_text);
	win.nameFieldLabel = @(filename_label);
	win.nameFieldStringValue = @(default_filename);
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

void os_OpenURL (const char *url) { os_OpenFileBrowser (url); }

void os_OpenFileBrowser (const char *directory) {
	char command[1040];
	assert (strlen (directory) < 1024);
	snprintf (command, sizeof (command), "open \"%s\"", directory);
	system (command);
}

void os_Cleanup () {}

#ifndef OSINTERFACE_NATIVE_GL_RENDERING

void os_DrawScreen () {
	pthread_mutex_lock(&os_private.osx.opengl_mutex);

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

	[os_private.osx.gl_context flushBuffer];
	pthread_mutex_unlock(&os_private.osx.opengl_mutex);

	if (os_LogGLErrors ()) LOG ("Had GL errors");
}
#endif

bool os_LogGLErrors () {
	{
		auto context = [NSOpenGLContext currentContext];
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
