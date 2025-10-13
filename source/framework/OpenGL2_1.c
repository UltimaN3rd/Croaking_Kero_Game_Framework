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

#if WIN32

wglSwapIntervalEXT_t wglSwapIntervalEXT;
wglCreateContextAttribsARB_t wglCreateContextAttribsARB;
glUniform2f_t glUniform2f;
glActiveTexture_t glActiveTexture;
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















#elif __APPLE__

#else










#error "Unsupported platform"

#endif