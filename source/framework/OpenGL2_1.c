#include "OpenGL2_1.h"
#include "log.h"
#include <assert.h>

static bool LogGLErrors () {
	{
		#if WIN32
		auto context = wglGetCurrentContext ();
		#elif __linux__
		auto context = glXGetCurrentContext ();
		#elif __APPLE__
		auto context = [NSOpenGLContext currentContext];
		#endif
		assert (context); if (context == NULL) { LOG ("The thread which called LogGLErrors() does not have an OpenGL context"); return true; }
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

glUniform2f_t glUniform2f;
glCreateShader_t glCreateShader;
glShaderSource_t glShaderSource;
glCompileShader_t glCompileShader;
glGetShaderiv_t glGetShaderiv;
glGetShaderInfoLog_t glGetShaderInfoLog;
glCreateProgram_t glCreateProgram;
glAttachShader_t glAttachShader;
glLinkProgram_t glLinkProgram;
glGetProgramiv_t glGetProgramiv;
glGetProgramInfoLog_t glGetProgramInfoLog;
glDeleteShader_t glDeleteShader;
glUseProgram_t glUseProgram;
glUniform3fv_t glUniform3fv;
glGetUniformLocation_t glGetUniformLocation;
glUniform1i_t glUniform1i;
glValidateProgram_t glValidateProgram;

#if WIN32

wglSwapIntervalEXT_t wglSwapIntervalEXT;
wglCreateContextAttribsARB_t wglCreateContextAttribsARB;
glActiveTexture_t glActiveTexture;

#define GLFUNC(__funcname__) do { __funcname__ = (__funcname__##_t)wglGetProcAddress ((LPCSTR)#__funcname__); assert (__funcname__); if (__funcname__ == NULL) { LOG ("WGL failed to find function ["#__funcname__"]"); return NULL; } } while (0)
#define GLFUNC_LOCAL(__funcname__) __funcname__##_t __funcname__ = NULL; GLFUNC (__funcname__)

static const char *HResultToStr (HRESULT result) {
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

HGLRC OpenGL2_1_Init_Win32 (HDC window_context) {
    auto gltemp = wglCreateContext (window_context); assert (gltemp); if (gltemp == FALSE) { LOG ("wglCreateContext failed [%s]", HResultToStr(GetLastError())); return NULL; }
    { auto result = wglMakeCurrent (window_context, gltemp); assert (result); if (result == FALSE) { LOG ("wglMakeCurrent failed [%s]", HResultToStr(GetLastError())); return NULL; } }
    GLFUNC (wglCreateContextAttribsARB);
    auto gl_context = wglCreateContextAttribsARB (window_context, 0, (const int[]){WGL_CONTEXT_MAJOR_VERSION_ARB, 2, WGL_CONTEXT_MINOR_VERSION_ARB, 1, 0}); assert (window_context); if (gl_context == NULL) { LOG ("wglCreateContextAttribsARB failed [%s]", HResultToStr(GetLastError())); return NULL; }
    { auto result = wglMakeCurrent (window_context, gl_context); assert (result); if (result == FALSE) { LOG ("wglMakeCurrent failed [%s]", HResultToStr(GetLastError())); return NULL; } }
    if (LogGLErrors ()) { LOG ("OpenGL had errors");}
    LOG ("OpenGL version: [%s]", (const char*)glGetString (GL_VERSION));
    wglDeleteContext (gltemp);
    
    GLFUNC (wglSwapIntervalEXT);
    GLFUNC (glUniform2f);
    GLFUNC (glActiveTexture);
	GLFUNC (glCreateShader);
	GLFUNC (glShaderSource);
	GLFUNC (glCompileShader);
	GLFUNC (glGetShaderiv);
	GLFUNC (glGetShaderInfoLog);
	GLFUNC (glCreateProgram);
	GLFUNC (glAttachShader);
	GLFUNC (glLinkProgram);
	GLFUNC (glGetProgramiv);
	GLFUNC (glGetProgramInfoLog);
	GLFUNC (glDeleteShader);
	GLFUNC (glUseProgram);
	GLFUNC (glUniform3fv);
	GLFUNC (glGetUniformLocation);
	GLFUNC (glUniform1i);
	GLFUNC (glValidateProgram);
    {
        auto result = wglSwapIntervalEXT (-1); assert (result); if (result == FALSE) {
            LOG ("wglSwapIntervalEXT (-1) failed. Could not enable adaptive sync [%s]", HResultToStr(GetLastError()));
            { auto result = wglSwapIntervalEXT (1); assert (result); if (result == FALSE) { LOG ("wglSwapIntervalEXT (1) failed. Could not enable VSync [%s]", HResultToStr(GetLastError())); } }
        }
        if (LogGLErrors ()) { LOG ("OpenGL had errors");}
    }
    return gl_context;
}













#elif __linux__

#include <string.h>

glXSwapIntervalEXT_t glXSwapIntervalEXT;

#define GLFUNC(__funcname__) do { __funcname__ = (__funcname__##_t)glXGetProcAddress ((const GLubyte*)#__funcname__); assert (__funcname__); if (__funcname__ == NULL) { LOG ("GLX failed to find function ["#__funcname__"]"); return ret; } } while (0)

OpenGL2_1_Init_Linux_return_t OpenGL2_1_Init_Linux (Display *display, int screen) {
	OpenGL2_1_Init_Linux_return_t ret = {};
	int attributes [] = { GLX_RGBA, GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_DEPTH_SIZE, 24,
		GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
		GLX_CONTEXT_MINOR_VERSION_ARB, 1,
		0 };

	ret.visual_info = glXChooseVisual (display, screen, attributes); assert (ret.visual_info); if (ret.visual_info == NULL) { LOG ("GLX failed to select suitable visual. Is there an undefined attribute in the list?"); return ret; };

	ret.context = glXCreateContext (display, ret.visual_info, 0, true); assert (ret.context); if (ret.context == NULL) { LOG ("GLX failed to create context"); return ret; }
    
    GLFUNC (glUniform2f);
	GLFUNC (glCreateShader);
	GLFUNC (glShaderSource);
	GLFUNC (glCompileShader);
	GLFUNC (glGetShaderiv);
	GLFUNC (glGetShaderInfoLog);
	GLFUNC (glCreateProgram);
	GLFUNC (glAttachShader);
	GLFUNC (glLinkProgram);
	GLFUNC (glGetProgramiv);
	GLFUNC (glGetProgramInfoLog);
	GLFUNC (glDeleteShader);
	GLFUNC (glUseProgram);
	GLFUNC (glUniform3fv);
	GLFUNC (glGetUniformLocation);
	GLFUNC (glUniform1i);
	GLFUNC (glValidateProgram);
	GLFUNC (glXSwapIntervalEXT);

	ret.success = true;
	return ret;
}

int OpenGL2_1_EnableSwapTear_FallbackSwap_Linux (Display *display, Window window) {
	const char *glx_extensions = glXQueryExtensionsString (display,0);
	const char *e = glx_extensions;

	do {
		e = strstr (e, "GLX_EXT_swap_control_tear");
		if (e == NULL) break;
		char next = *(e + strlen("GLX_EXT_swap_control_tear"));
		if (next == ' ' || next == '\0') {
			glXSwapIntervalEXT (display, window, -1);
			LOG ("GLX_EXT_swap_control_tear enabled");
			return -1;
		}
		e += strlen ("GLX_EXT_swap_control_tear");
	} while (true);

	LOG ("OpenGL extension 'GLX_EXT_swap_control_tear' not found.");
	e = glx_extensions;
	do {
		e = strstr (e, "GLX_EXT_swap_control");
		if (e == NULL) break;
		char next = *(e + strlen("GLX_EXT_swap_control"));
		if (next == ' ' || next == '\0') {
			glXSwapIntervalEXT (display, window, 1);
			LOG ("GLX_EXT_swap_control enabled");
			return 1;
		}
		e += strlen("GLX_EXT_swap_control");
	} while (true);

	LOG ("OpenGL extension 'GLX_EXT_swap_control' not found.");
	return 0;
}















#elif __APPLE__

#else










#error "Unsupported platform"

#endif