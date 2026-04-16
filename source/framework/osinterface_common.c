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

#ifdef __APPLE__
# define GL_SILENCE_DEPRECATION
# include <OpenGL/glu.h>
#elifdef __linux__
# include "linux/OpenGL2_1.h"
#elifdef WIN32
# include "windows/OpenGL2_1.h"
#else
# error "Unsupported platform"
#endif
#include "osinterface.h"
#include "log.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef OSINTERFACE_COLOR_INDEX_MODE
extern const u8 palette[256][3];
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
bool os_CreateGLColorMap ();
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
}
#endif

#ifndef OSINTERFACE_NATIVE_GL_RENDERING
void os_ClearScreen () {
	memset (os_private.frame_buffer.pixels, 0, os_private.frame_buffer.width * os_private.frame_buffer.height);
}
#endif

#ifdef OSINTERFACE_COLOR_INDEX_MODE

bool os_CreateGLColorMap () {
	if (os_LogGLErrors ()) { LOG ("OpenGL error"); return false; }
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
		return false;
	}
	if (os_LogGLErrors ()) { LOG ("OpenGL error"); return false; }
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
		return false;
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
		return false;
	}
	glDeleteShader (vertex);
	glDeleteShader (fragment);
	glUseProgram (program);
	if (os_LogGLErrors ()) { LOG ("OpenGL error"); return false; }
	f32 f32_palette[256][3];
	for (int i = 0; i < 256; ++i) {
		f32_palette[i][0] = palette[i][0] / 255.f;
		f32_palette[i][1] = palette[i][1] / 255.f;
		f32_palette[i][2] = palette[i][2] / 255.f;
	}
	glUniform3fv (glGetUniformLocation(program, "palette"), 256, (const GLfloat*)f32_palette);

	os_private.gl.locations.vertex.scale = glGetUniformLocation(program, "uni_scale");
	os_private.gl.locations.fragment.texture = glGetUniformLocation(program, "sam_texture");

	if (os_LogGLErrors ()) { LOG ("OpenGL error"); return false; }
	glActiveTexture (GL_TEXTURE0);
	glGenTextures (1, &os_private.gl.texture);
	glBindTexture (GL_TEXTURE_2D, os_private.gl.texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glUniform1i (os_private.gl.locations.fragment.texture, 0);
	if (os_LogGLErrors ()) { LOG ("OpenGL error"); return false; }

	glValidateProgram (program);
	glGetProgramiv (program, GL_VALIDATE_STATUS, &success);
	if (!success) {
		char buf[512];
		glGetProgramInfoLog (program, 512, NULL, buf);
		LOG ("OpenGL shader validation error [%s]", buf);
		assert (false);
		return false;
	}
	if (os_LogGLErrors ()) { LOG ("OpenGL error"); return false; }
	return true;
}
#endif
