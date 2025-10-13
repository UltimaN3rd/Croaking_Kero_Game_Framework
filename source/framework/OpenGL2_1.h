#pragma once

#if WIN32

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <GL/wglext.h>

typedef BOOL (*wglSwapIntervalEXT_t) (int interval); extern wglSwapIntervalEXT_t wglSwapIntervalEXT;
typedef HGLRC (*wglCreateContextAttribsARB_t) (HDC hDC, HGLRC hshareContext, const int *attribList); extern wglCreateContextAttribsARB_t wglCreateContextAttribsARB;
typedef void (*glActiveTexture_t) (GLenum texture); extern glActiveTexture_t glActiveTexture;

HGLRC OpenGL2_1_Init_Win32 (HDC window_context);

#elif __linux__

#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/glu.h>

typedef struct {
    bool success;
    XVisualInfo *visual_info;
    GLXContext context;
} OpenGL2_1_Init_Linux_return_t;
OpenGL2_1_Init_Linux_return_t OpenGL2_1_Init_Linux (Display *display, int screen);

typedef void (*glXSwapIntervalEXT_t)(Display *dpy, GLXDrawable drawable, int interval); extern glXSwapIntervalEXT_t glXSwapIntervalEXT;

// Returns 0 on failure. -1 on Tear success. 1 on fallback swap success.
int OpenGL2_1_EnableSwapTear_FallbackSwap_Linux (Display *display, Window window);

#elif __APPLE__

#else

#error "Unsupported platform"

#endif

typedef void (*glUniform2f_t) (GLint location, GLfloat v0, GLfloat v1); extern glUniform2f_t glUniform2f;
typedef GLuint (*glCreateShader_t) (GLenum shaderType); extern glCreateShader_t glCreateShader;
typedef void (*glShaderSource_t) (GLuint shader, GLsizei count, const GLchar **string, const GLint *length); extern glShaderSource_t glShaderSource;
typedef void (*glCompileShader_t) (GLuint shader); extern glCompileShader_t glCompileShader;
typedef void (*glGetShaderiv_t) (GLuint shader, GLenum pname, GLint *params); extern glGetShaderiv_t glGetShaderiv;
typedef void (*glGetShaderInfoLog_t) (GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog); extern glGetShaderInfoLog_t glGetShaderInfoLog;
typedef GLuint (*glCreateProgram_t) (); extern glCreateProgram_t glCreateProgram;
typedef void (*glAttachShader_t) (GLuint program, GLuint shader); extern glAttachShader_t glAttachShader;
typedef void (*glLinkProgram_t) (GLuint program); extern glLinkProgram_t glLinkProgram;
typedef void (*glGetProgramiv_t) (GLuint program, GLenum pname, GLint *params); extern glGetProgramiv_t glGetProgramiv;
typedef void (*glGetProgramInfoLog_t) (GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog); extern glGetProgramInfoLog_t glGetProgramInfoLog;
typedef void (*glDeleteShader_t) (GLuint shader); extern glDeleteShader_t glDeleteShader;
typedef void (*glUseProgram_t) (GLuint program); extern glUseProgram_t glUseProgram;
typedef void (*glUniform3fv_t) (GLint location, GLsizei count, const GLfloat *value); extern glUniform3fv_t glUniform3fv;
typedef GLint (*glGetUniformLocation_t) (GLuint program, const GLchar *name); extern glGetUniformLocation_t glGetUniformLocation;
typedef void (*glUniform1i_t) (GLint location, GLint v0); extern glUniform1i_t glUniform1i;
typedef void (*glValidateProgram_t) (GLuint program); extern glValidateProgram_t glValidateProgram;