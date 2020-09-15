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

#ifndef __GLCONTEXT_H__
#define __GLCONTEXT_H__

#include <graphics.h>

struct __MCGLContext;
typedef __MCGLContext* MCGLContextRef;

struct MCGLColorVertex
{
	GLshort position[2];
	// color in RGBA byte order
	uint32_t color;
};

struct MCGLTextureVertex
{
	GLshort position[2];
	GLubyte texture_position[2];
};

enum MCGLProgramType
{
	kMCGLProgramTypeNone,
	kMCGLProgramTypeColor,
	kMCGLProgramTypeTexture,
};

extern bool MCGLCheckError(const char *p_prefix);

extern bool MCGLContextCreate(MCGLContextRef &r_context);
extern void MCGLContextDestroy(MCGLContextRef p_context);
extern bool MCGLContextInit(MCGLContextRef p_context);
extern void MCGLContextReset(MCGLContextRef p_context);

extern bool MCGLContextSetProjectionTransform(MCGLContextRef p_context, const MCGAffineTransform &p_transform);
extern bool MCGLContextConcatProjectionTransform(MCGLContextRef p_context, const MCGAffineTransform &p_transform);
extern bool MCGLContextSetWorldTransform(MCGLContextRef p_context, const MCGAffineTransform &p_transform);
extern bool MCGLContextConcatWorldTransform(MCGLContextRef p_context, const MCGAffineTransform &p_transform);
extern bool MCGLContextSetTextureTransform(MCGLContextRef p_context, const MCGAffineTransform &p_transform);
extern bool MCGLContextConcatTextureTransform(MCGLContextRef p_context, const MCGAffineTransform &p_transform);

extern bool MCGLContextSelectProgram(MCGLContextRef p_context, MCGLProgramType p_program);


extern void MCPlatformEnableOpenGLMode(void);
extern void MCPlatformDisableOpenGLMode(void);
extern MCGLContextRef MCPlatformGetOpenGLContext(void);


#endif // __GLCONTEXT_H__
