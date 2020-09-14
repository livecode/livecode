/* Copyright (C) 2019 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "prefix.h"

#if defined(_IOS_MOBILE)
#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>
#elif defined(TARGET_SUBPLATFORM_ANDROID)
#define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#else
#error GLContext not supported on this platform
#endif

#include "glcontext.h"
#include "packed.h"

#ifdef DEBUG_GL
#define MCGLDebugCheckError(p_prefix) MCGLCheckError(p_prefix)
#else
#define MCGLDebugCheckError(p_prefix) (true)
#endif

////////////////////////////////////////////////////////////////////////////////

struct MCGLProgramConfig
{
	MCGLProgramType program;

	MCGAffineTransform projection_transform;
	MCGAffineTransform world_transform;
	MCGAffineTransform texture_transform;
};

struct color_program_t
{
	// Program
	GLuint program;

	// Uniform
	GLuint world_transform;

	// Vertex array object
	GLuint vao;
};

struct texture_program_t
{
	// Program
	GLuint program;

	// Uniformas
	GLuint world_transform;
	GLuint texture_transform;

	// Certex array object
	GLuint vao;
};

struct __MCGLContext
{
	// The version of OpenGL in use.
	GLint opengl_major_version;
	GLint opengl_minor_version;
	
	// Context initialization
	bool is_initialized;

	// Programs
	color_program_t color_program;
	texture_program_t texture_program;
	texture_program_t external_texture_program;

	// buffer
	GLuint buffer;

	// current context state
	MCGLProgramConfig config;
};

////////////////////////////////////////////////////////////////////////////////

static bool MCGLUniformAffineTransform(GLuint p_uniform, const MCGAffineTransform &p_transform)
{
	MCLog("MCGLUniformAffineTransform(%d, {%f, %f, %f, %f, %f, %f})", p_uniform, p_transform.a, p_transform.b, p_transform.c, p_transform.d, p_transform.tx, p_transform.ty);
	GLfloat t_matrix[] = {
		p_transform.a,
		p_transform.b,
		p_transform.c,
		p_transform.d,
		p_transform.tx,
		p_transform.ty,
	};
	glUniformMatrix3x2fv(p_uniform, 1, false, t_matrix);
	return MCGLDebugCheckError("uniform transform: ");
}

bool MCGLCheckError(const char *p_prefix)
{
	if (p_prefix == nil)
		p_prefix = "";

	GLenum t_error;
	t_error = glGetError();

	if (t_error == GL_NO_ERROR)
		return true;

	while (t_error != GL_NO_ERROR)
	{
		MCLog("%sGL error: %d", p_prefix, t_error);
		t_error = glGetError();
	}

	return false;
}

static bool MCGLCompileShader(GLuint p_shader_type, const char *p_source, GLuint &r_shader)
{
	bool t_success;
	t_success = true;

	GLuint t_shader;
	t_shader = 0;

	if (t_success)
	{
		t_shader = glCreateShader(p_shader_type);
		t_success = t_shader != 0;
	}

	if (t_success)
	{
		glShaderSource(t_shader, 1, &p_source, nil);
		glCompileShader(t_shader);

		/* DEBUG */
		GLint t_status;
		glGetShaderiv(t_shader, GL_COMPILE_STATUS, &t_status);
		t_success = t_status == GL_TRUE;
		if (!t_success)
		{
			char t_buffer[512];
			glGetShaderInfoLog(t_shader, 512, nil, t_buffer);
			MCLog("shader compile failed: %s\n%s", t_buffer, p_source);
		}
	}

	if (t_success)
		r_shader = t_shader;
	else
		glDeleteShader(t_shader);

	return t_success;
}

struct MCGLShaderSourceInfo
{
	GLuint type;
	const char *source;
};

static bool MCGLCompileProgram(const MCGLShaderSourceInfo *p_shaders, uindex_t p_shader_count, GLuint &r_program)
{
	bool t_success;
	t_success = true;

	GLuint t_program;
	t_program = 0;

	if (t_success)
	{
		t_program = glCreateProgram();
		t_success = t_program != 0;
	}

	for (uindex_t i = 0; t_success && (i < p_shader_count); i++)
	{
		GLuint t_shader;
		t_success = MCGLCompileShader(p_shaders[i].type, p_shaders[i].source, t_shader);
		if (t_success)
		{
			glAttachShader(t_program, t_shader);
			glDeleteShader(t_shader);
		}
	}

	if (t_success)
	{
		glLinkProgram(t_program);

		GLint t_status;
		glGetProgramiv(t_program, GL_LINK_STATUS, &t_status);
		t_success = t_status == GL_TRUE;
		if (!t_success)
		{
			char t_buffer[512];
			glGetProgramInfoLog(t_program, 512, nil, t_buffer);
			MCLog("program link failed: %s", t_buffer);
		}
	}

	if (t_success)
		r_program = t_program;
	else
		glDeleteProgram(t_program);

	return t_success;
}

static const char *s_color_vertex_shader_source =
R"glsl(#version 300 es

uniform mat3x2 pTransform;

in vec2 pPosition;
in vec4 pColor;

out vec4 pFragColor;

void main()
{
	pFragColor = pColor;
	gl_Position = vec4(pTransform * vec3(pPosition, 1.0), 0.0, 1.0);
}
)glsl";

static const char *s_color_fragment_shader_source =
R"glsl(#version 300 es

precision mediump float;
in vec4 pFragColor;

out vec4 outColor;

void main()
{
	outColor = pFragColor;
}
)glsl";

static MCGLShaderSourceInfo s_color_shaders[] = {
	{GL_VERTEX_SHADER, s_color_vertex_shader_source},
	{GL_FRAGMENT_SHADER, s_color_fragment_shader_source},
};

static const char *s_texture_vertex_shader_source =
R"glsl(#version 300 es

uniform mat3x2 pTransform;

in vec2 pPosition;
in vec2 pTextureCoord;

out vec2 pVertexTextureCoord;
void main()
{
	gl_Position = vec4(pTransform * vec3(pPosition, 1.0), 0.0, 1.0);
	pVertexTextureCoord = pTextureCoord;
}
)glsl";

static const char *s_texture_fragment_shader_source =
R"glsl(#version 300 es

precision mediump float;
uniform mat3x2 pTextureTransform;

in vec2 pVertexTextureCoord;

out vec4 outColor;

uniform sampler2D tex;

void main()
{
	outColor = texture(tex, pTextureTransform * vec3(pVertexTextureCoord, 1.0));
}
)glsl";

static MCGLShaderSourceInfo s_texture_shaders[] = {
	{GL_VERTEX_SHADER, s_texture_vertex_shader_source},
	{GL_FRAGMENT_SHADER, s_texture_fragment_shader_source},
};

static const char *s_external_texture_fragment_shader_source =
R"glsl(#version 300 es

#extension GL_OES_EGL_image_external_essl3 : require

precision mediump float;
uniform mat3x2 pTextureTransform;

in vec2 pVertexTextureCoord;

out vec4 outColor;

uniform samplerExternalOES tex;

void main()
{
	outColor = texture(tex, pTextureTransform * vec3(pVertexTextureCoord, 1.0));
}
)glsl";

static MCGLShaderSourceInfo s_external_texture_shaders[] = {
	{GL_VERTEX_SHADER, s_texture_vertex_shader_source},
	{GL_FRAGMENT_SHADER, s_external_texture_fragment_shader_source},
};

static void MCGLContextFreeColorProgram(color_program_t &x_program)
{
	glDeleteProgram(x_program.program);
	glDeleteVertexArrays(1, &x_program.vao);
	MCMemoryClear(x_program);
}

static bool MCGLContextCreateColorProgram(GLuint p_buffer, color_program_t &r_program)
{
	bool t_success;
	t_success = true;

	color_program_t t_program;

	/* Compile shader program for drawing solid color */
	t_program.program = 0;
	if (t_success)
	{
		t_success = MCGLCompileProgram(s_color_shaders, 2, t_program.program);
		/* UNCHECKED */ MCGLDebugCheckError("compile color program: ");
	}

	/* Fetch uniform values */
	t_program.world_transform = 0;
	if (t_success)
	{
		t_program.world_transform  = glGetUniformLocation(t_program.program, "pTransform");
		/* UNCHECKED */ MCGLDebugCheckError("get color program uniform locations: ");
	}

	/* Create vertex arrays to manage attributes */
	t_program.vao = 0;
	if (t_success)
	{
		glGenVertexArrays(1, &t_program.vao);
		/* UNCHECKED */ MCGLDebugCheckError("create VAO: ");
	}

	/* Set up vertex attributes */
	if (t_success)
	{
		GLint t_position;
		GLint t_color;
		t_position = glGetAttribLocation(t_program.program, "pPosition");
		t_color = glGetAttribLocation(t_program.program, "pColor");
		t_success = t_position != -1 && t_color != -1;
		if (t_success)
		{
			glBindVertexArray(t_program.vao);
			glBindBuffer(GL_ARRAY_BUFFER, p_buffer);
			glEnableVertexAttribArray(t_position);
			glVertexAttribPointer(t_position, 2, GL_SHORT, GL_FALSE, sizeof(MCGLColorVertex), 0);
			glEnableVertexAttribArray(t_color);
			glVertexAttribPointer(t_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(MCGLColorVertex), (void*)(2 * sizeof(GLshort)));
		}
		/* UNCHECKED */ MCGLDebugCheckError("set up color VAO: ");
	}

	if (t_success)
	{
		r_program = t_program;

		MCLog("color program: %d, uniform: %d, vao: %d", t_program.program, t_program.world_transform, t_program.vao);
	}
	else
	{
		MCGLContextFreeColorProgram(t_program);
		/* UNCHECKED */ MCGLDebugCheckError("color program cleanup: ");
	}

	return t_success;
}

static void MCGLContextFreeTextureProgram(texture_program_t &x_program)
{
	glDeleteProgram(x_program.program);
	glDeleteVertexArrays(1, &x_program.vao);
	MCMemoryClear(x_program);
}

static bool MCGLContextCreateTextureProgram(GLuint p_buffer, bool p_external_texture, texture_program_t &r_program)
{
	bool t_success;
	t_success = true;

	/* Select the shader set to compile into the program based on 
	 * whether or not external textures will be used */
	MCGLShaderSourceInfo *t_shaders;
	if (p_external_texture)
		t_shaders = s_external_texture_shaders;
	else
		t_shaders = s_texture_shaders;

	/* Compile shader programs for drawing solid color & texture */
	texture_program_t t_program;
	t_program.program = 0;
	if (t_success)
		t_success = MCGLCompileProgram(t_shaders, 2, t_program.program);
	/* UNCHECKED */ MCGLDebugCheckError("compile texture program: ");

	/* Fetch uniform values */
	t_program.world_transform = 0;
	t_program.texture_transform = 0;
	if (t_success)
	{
		t_program.world_transform = glGetUniformLocation(t_program.program, "pTransform");
		t_program.texture_transform = glGetUniformLocation(t_program.program, "pTextureTransform");
		/* UNCHECKED */ MCGLDebugCheckError("get uniform locations: ");
	}

	/* Create vertex arrays to manage attributes */
	t_program.vao = 0;
	if (t_success)
	{
		glGenVertexArrays(1, &t_program.vao);
		/* UNCHECKED */ MCGLDebugCheckError("create VAOs: ");
	}

	/* Set up vertex attributes */
	if (t_success)
	{
		GLint t_position;
		GLint t_texture_coord;
		t_position = glGetAttribLocation(t_program.program, "pPosition");
		t_texture_coord = glGetAttribLocation(t_program.program, "pTextureCoord");
		t_success = t_position != -1 && t_texture_coord != -1;
		if (t_success)
		{
			glBindVertexArray(t_program.vao);
			glBindBuffer(GL_ARRAY_BUFFER, p_buffer);
			glEnableVertexAttribArray(t_position);
			glVertexAttribPointer(t_position, 2, GL_SHORT, GL_FALSE, sizeof(MCGLTextureVertex), nil);
			glEnableVertexAttribArray(t_texture_coord);
			glVertexAttribPointer(t_texture_coord, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(MCGLTextureVertex), (void*)(2 * sizeof(GLshort)));
		}
		/* UNCHECKED */ MCGLDebugCheckError("set up texture VAO: ");
	}

	if (t_success)
	{
		r_program = t_program;

		MCLog("texture program: %d, uniforms: [%d, %d], vao: %d", t_program.program, t_program.world_transform, t_program.texture_transform, t_program.vao);
	}
	else
	{
		MCGLContextFreeTextureProgram(t_program);
		/* UNCHECKED */ MCGLDebugCheckError("texture program cleanup: ");
	}

	return t_success;
}

bool MCGLContextInit(MCGLContextRef self)
{
	MCLog("MCGLContextInit(<%p>)");
	if (self == nil)
		return false;

	if (self->is_initialized)
		return true;

	/* UNCHECKED */ MCGLCheckError("pre-init: ");
	bool t_success;
	t_success = true;

	MCLog("gl Version String: %s", glGetString(GL_VERSION));
	
	// Get the OpenGL version.
	glGetIntegerv(GL_MAJOR_VERSION, &self->opengl_major_version);
	glGetIntegerv(GL_MINOR_VERSION, &self->opengl_minor_version);
	/* UNCHECKED */ MCGLDebugCheckError("get version: ");
	

	/* Create input buffers */
	GLuint t_buffer;
	t_buffer = 0;
	if (t_success)
	{
		glGenBuffers(1, &t_buffer);
		/* UNCHECKED */ MCGLDebugCheckError("create buffers: ");
	}

	/* Create programs */
	color_program_t t_color_program;
	MCMemoryClear(t_color_program);
	if (t_success)
		t_success = MCGLContextCreateColorProgram(t_buffer, t_color_program);
	texture_program_t t_texture_program;
	MCMemoryClear(t_texture_program);
	if (t_success)
		t_success = MCGLContextCreateTextureProgram(t_buffer, false, t_texture_program);

	/* Create optional external texture program */
	texture_program_t t_external_texture_program;
	MCMemoryClear(t_external_texture_program);
	if (t_success)
	{
		/* UNCHECKED */ MCGLContextCreateTextureProgram(t_buffer, true, t_external_texture_program);
	}

	if (t_success)
	{
		self->color_program = t_color_program;
		self->texture_program = t_texture_program;
		self->external_texture_program = t_external_texture_program;

		self->buffer = t_buffer;

		self->is_initialized = true;
	}
	else
	{
		MCGLContextFreeColorProgram(t_color_program);
		MCGLContextFreeTextureProgram(t_texture_program);
		MCGLContextFreeTextureProgram(t_external_texture_program);
		glDeleteBuffers(1, &t_buffer);
		/* UNCHECKED */ MCGLDebugCheckError("init cleanup: ");
	}

	return t_success && MCGLCheckError("Init: ");
}

static void MCGLContextFinalize(MCGLContextRef p_context)
{
	MCGLContextSelectProgram(p_context, kMCGLProgramTypeNone);

	MCGLContextFreeColorProgram(p_context->color_program);
	MCGLContextFreeTextureProgram(p_context->texture_program);
	MCGLContextFreeTextureProgram(p_context->external_texture_program);

	glDeleteBuffers(1, &p_context->buffer);
}

bool MCGLContextCreate(MCGLContextRef &r_context)
{
	MCGLContextRef t_context;
	if (!MCMemoryNew(t_context))
		return false;

	MCLog("created GL context <%p>", t_context);
	r_context = t_context;
	return true;
}

void MCGLContextDestroy(MCGLContextRef p_context)
{
	if (p_context == nil)
		return;

	MCLog("destroying GL context <%p>", p_context);
	MCGLContextReset(p_context);
	MCMemoryDelete(p_context);
}

void MCGLContextReset(MCGLContextRef p_context)
{
	if (p_context == nil)
		return;
	if (!p_context->is_initialized)
		return;
	MCGLContextFinalize(p_context);
	MCMemoryClear(p_context, sizeof(__MCGLContext));
}

bool MCGLContextGetProgramSupported(MCGLContextRef p_context, MCGLProgramType p_program)
{
	if (p_context == nil)
		return false;

	switch (p_program)
	{
		case kMCGLProgramTypeNone:
			return true;

		case kMCGLProgramTypeColor:
			return p_context->color_program.program != 0;

		case kMCGLProgramTypeTexture:
			return p_context->texture_program.program != 0;

		case kMCGLProgramTypeExternalTexture:
			return p_context->external_texture_program.program != 0;
	}
}

bool MCGLContextSelectProgram(MCGLContextRef p_context, MCGLProgramType p_program)
{
	if (p_context == nil)
		return false;

	if (!MCGLContextGetProgramSupported(p_context, p_program))
		return false;

	p_context->config.program = p_program;

	switch (p_program)
	{
		case kMCGLProgramTypeNone:
			glUseProgram(0);
			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			break;

		case kMCGLProgramTypeColor:
			// select color program and related vao and buffer
			glUseProgram(p_context->color_program.program);
			glBindVertexArray(p_context->color_program.vao);
			glBindBuffer(GL_ARRAY_BUFFER, p_context->buffer);
			break;

		case kMCGLProgramTypeTexture:
			// select texture program and related vao and buffer
			glUseProgram(p_context->texture_program.program);
			glBindVertexArray(p_context->texture_program.vao);
			glBindBuffer(GL_ARRAY_BUFFER, p_context->buffer);
			break;

		case kMCGLProgramTypeExternalTexture:
			// select external texture program and related vao and buffer
			glUseProgram(p_context->external_texture_program.program);
			glBindVertexArray(p_context->external_texture_program.vao);
			glBindBuffer(GL_ARRAY_BUFFER, p_context->buffer);
			break;
	}

	return MCGLDebugCheckError("switch program: ");
}

static inline bool MCGLContextUpdateTransform(MCGLContextRef p_context)
{
	MCGAffineTransform t_transform;
	t_transform = MCGAffineTransformConcat(p_context->config.projection_transform, p_context->config.world_transform);

	switch (p_context->config.program)
	{
		case kMCGLProgramTypeNone:
			return false;

		case kMCGLProgramTypeColor:
			return MCGLUniformAffineTransform(p_context->color_program.world_transform, t_transform);

		case kMCGLProgramTypeTexture:
			return MCGLUniformAffineTransform(p_context->texture_program.world_transform, t_transform);

		case kMCGLProgramTypeExternalTexture:
			return MCGLUniformAffineTransform(p_context->external_texture_program.world_transform, t_transform);
	}
}

bool MCGLContextSetProjectionTransform(MCGLContextRef p_context, const MCGAffineTransform &p_transform)
{
	MCLog("MCGLContextSetProjectionTransform(<%p>, {%f, %f, %f, %f, %f, %f})", p_context, p_transform.a, p_transform.b, p_transform.c, p_transform.d, p_transform.tx, p_transform.ty);
	if (p_context == nil)
		return false;

	p_context->config.projection_transform = p_transform;

	return MCGLContextUpdateTransform(p_context);
}

bool MCGLContextConcatProjectionTransform(MCGLContextRef p_context, const MCGAffineTransform &p_transform)
{
	MCLog("MCGLContextConcatProjectionTransform({%f, %f, %f, %f, %f, %f})", p_transform.a, p_transform.b, p_transform.c, p_transform.d, p_transform.tx, p_transform.ty);
	if (p_context == nil)
		return false;

	p_context->config.projection_transform = MCGAffineTransformConcat(p_context->config.projection_transform, p_transform);

	return MCGLContextUpdateTransform(p_context);
}

bool MCGLContextSetWorldTransform(MCGLContextRef p_context, const MCGAffineTransform &p_transform)
{
	MCLog("MCGLContextSetWorldTransform({%f, %f, %f, %f, %f, %f})", p_transform.a, p_transform.b, p_transform.c, p_transform.d, p_transform.tx, p_transform.ty);
	if (p_context == nil)
		return false;

	p_context->config.world_transform = p_transform;

	return MCGLContextUpdateTransform(p_context);
}

bool MCGLContextConcatWorldTransform(MCGLContextRef p_context, const MCGAffineTransform &p_transform)
{
	MCLog("MCGLContextConcatWorldTransform({%f, %f, %f, %f, %f, %f})", p_transform.a, p_transform.b, p_transform.c, p_transform.d, p_transform.tx, p_transform.ty);
	if (p_context == nil)
		return false;

	p_context->config.world_transform = MCGAffineTransformConcat(p_context->config.world_transform, p_transform);
	
	return MCGLContextUpdateTransform(p_context);
}

bool MCGLContextSetTextureTransform(MCGLContextRef p_context, const MCGAffineTransform &p_transform)
{
	MCLog("MCGLContextSetTextureTransform({%f, %f, %f, %f, %f, %f})", p_transform.a, p_transform.b, p_transform.c, p_transform.d, p_transform.tx, p_transform.ty);
	if (p_context == nil)
		return false;

	p_context->config.texture_transform = p_transform;

	switch (p_context->config.program)
	{
		case kMCGLProgramTypeTexture:
			return MCGLUniformAffineTransform(p_context->texture_program.texture_transform, p_context->config.texture_transform);

		case kMCGLProgramTypeExternalTexture:
			return MCGLUniformAffineTransform(p_context->external_texture_program.texture_transform, p_context->config.texture_transform);

		default:
			return false;
	}
}

bool MCGLContextConcatTextureTransform(MCGLContextRef p_context, const MCGAffineTransform &p_transform)
{
	MCLog("MCGLContextConcatTextureTransform({%f, %f, %f, %f, %f, %f})", p_transform.a, p_transform.b, p_transform.c, p_transform.d, p_transform.tx, p_transform.ty);
	if (p_context == nil)
		return false;

	return MCGLContextSetTextureTransform(p_context, MCGAffineTransformConcat(p_context->config.texture_transform, p_transform));
}
