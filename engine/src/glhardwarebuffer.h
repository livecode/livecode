/* Copyright (C) 2020 LiveCode Ltd.

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

typedef struct __MCGLHardwareBuffer *MCGLHardwareBufferRef;

bool MCGLHardwareBufferIsSupported(void);

bool MCGLHardwareBufferCreate(uindex_t p_width, uindex_t p_height, MCGLHardwareBufferRef &r_buffer);
void MCGLHardwareBufferDestroy(MCGLHardwareBufferRef p_buffer);
bool MCGLHardwareBufferGetStride(MCGLHardwareBufferRef p_buffer, uindex_t &r_stride);

bool MCGLHardwareBufferLock(MCGLHardwareBufferRef p_buffer, void *&r_ptr);
void MCGLHardwareBufferUnlock(MCGLHardwareBufferRef p_buffer);

bool MCGLHardwareBufferBindToGLTexture(MCGLHardwareBufferRef p_buffer, GLuint p_texture);
