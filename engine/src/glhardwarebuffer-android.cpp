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

/* TODO - Don't attempt to build on 32bit armv7 - needs ndk update */
#if !defined(__arm__)
#define MCANDROIDUSEHARDWAREBUFFER
#endif

#if defined(MCANDROIDUSEHARDWAREBUFFER)

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <EGL/egl.h>

// required for glEGLImageTargetTexture2DOES and GL_TEXTURE_EXTERNAL_OES definition
#include <GLES2/gl2ext.h>

#include <android/hardware_buffer.h>
#include <EGL/eglext.h>

#include <foundation.h>

#include "glcontext.h"
#include "glhardwarebuffer.h"

#include <dlfcn.h>

struct __MCGLHardwareBuffer
{
	AHardwareBuffer* handle;
	EGLImageKHR egl_image;
	uindex_t stride;
	void *lock;
};

typedef int (*AHardwareBuffer_allocate_func)(const AHardwareBuffer_Desc* desc, AHardwareBuffer** outBuffer);
typedef void (*AHardwareBuffer_release_func)(AHardwareBuffer* buffer);
typedef void (*AHardwareBuffer_describe_func)(const AHardwareBuffer* buffer, AHardwareBuffer_Desc* outDesc);
typedef int (*AHardwareBuffer_lock_func)(AHardwareBuffer* buffer, uint64_t usage, int32_t fence, const ARect* rect, void** outVirtualAddress);
typedef int (*AHardwareBuffer_unlock_func)(AHardwareBuffer* buffer, int32_t* fence);

typedef EGLClientBuffer (*eglGetNativeClientBufferANDROID_func)(const struct AHardwareBuffer *buffer);

static AHardwareBuffer_allocate_func s_AHardwareBuffer_allocate = nil;
static AHardwareBuffer_release_func s_AHardwareBuffer_release = nil;
static AHardwareBuffer_describe_func s_AHardwareBuffer_describe = nil;
static AHardwareBuffer_lock_func s_AHardwareBuffer_lock = nil;
static AHardwareBuffer_unlock_func s_AHardwareBuffer_unlock = nil;
static eglGetNativeClientBufferANDROID_func s_eglGetNativeClientBufferANDROID = nil;

static bool MCAndroidHardwareBufferInitialize()
{
	static bool s_initialized = false;
	if (!s_initialized)
	{
		s_initialized = true;
		s_AHardwareBuffer_allocate = (AHardwareBuffer_allocate_func)dlsym(NULL, "AHardwareBuffer_allocate");
		s_AHardwareBuffer_release = (AHardwareBuffer_release_func)dlsym(NULL, "AHardwareBuffer_release");
		s_AHardwareBuffer_describe = (AHardwareBuffer_describe_func)dlsym(NULL, "AHardwareBuffer_describe");
		s_AHardwareBuffer_lock = (AHardwareBuffer_lock_func)dlsym(NULL, "AHardwareBuffer_lock");
		s_AHardwareBuffer_unlock = (AHardwareBuffer_unlock_func)dlsym(NULL, "AHardwareBuffer_unlock");

		s_eglGetNativeClientBufferANDROID = (eglGetNativeClientBufferANDROID_func)dlsym(NULL, "eglGetNativeClientBufferANDROID");

		MCLog("MCGLHardwareBuffer - load AHardwareBuffer functions: %p %p %p %p %p", s_AHardwareBuffer_allocate, s_AHardwareBuffer_release, s_AHardwareBuffer_describe, s_AHardwareBuffer_lock, s_AHardwareBuffer_unlock);
		MCLog("MCGLHardwareBuffer - load egl functions: %p", s_eglGetNativeClientBufferANDROID);
	}

	return s_AHardwareBuffer_allocate != nil &&
			s_AHardwareBuffer_release != nil &&
			s_AHardwareBuffer_describe != nil &&
			s_AHardwareBuffer_lock != nil &&
			s_AHardwareBuffer_unlock != nil &&
			s_eglGetNativeClientBufferANDROID != nil;
}

static bool MCAndroidHardwareBufferAllocate(uindex_t p_width, uindex_t p_height, AHardwareBuffer* &r_buffer, uint32_t &r_stride)
{
	AHardwareBuffer_Desc t_desc;
	t_desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
	t_desc.height = p_width;
	t_desc.width = p_height;
	t_desc.layers = 1;
	t_desc.rfu0 = 0;
	t_desc.rfu1 = 0;
	t_desc.stride = 0;
	t_desc.usage = 
		AHARDWAREBUFFER_USAGE_CPU_READ_NEVER |
		AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN |
		AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;

	AHardwareBuffer *t_hw_buffer = nil;
	if (0 != s_AHardwareBuffer_allocate(&t_desc, &t_hw_buffer))
		return false;

	s_AHardwareBuffer_describe(t_hw_buffer, &t_desc);

	r_buffer = t_hw_buffer;
	r_stride = t_desc.stride * sizeof(uint32_t);

	return true;
}

static bool MCAndroidHardwareBufferBindToEGLImage(MCGLHardwareBufferRef p_buffer)
{
	if (p_buffer == nil)
		return false;

	if (p_buffer->egl_image != EGL_NO_IMAGE_KHR)
		return true;

	bool t_success = true;

	EGLClientBuffer t_client_buffer = nil;
	if (t_success)
	{
		t_client_buffer = s_eglGetNativeClientBufferANDROID(p_buffer->handle);
		t_success = t_client_buffer != nil;
	}

	EGLDisplay t_display = EGL_NO_DISPLAY;
	if (t_success)
	{
		t_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		t_success = t_display != EGL_NO_DISPLAY;
	}

	EGLImageKHR t_image = EGL_NO_IMAGE_KHR;
	if (t_success)
	{
		EGLint t_image_attribs[] = {
			EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
			EGL_NONE,
		};

		t_image = eglCreateImageKHR(t_display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, t_client_buffer, t_image_attribs);
		t_success = t_image != EGL_NO_IMAGE_KHR;
	}

	if (t_success)
		p_buffer->egl_image = t_image;

	return t_success;
}

static void MCAndroidHardwareBufferUnbindEGLImage(MCGLHardwareBufferRef p_buffer)
{
	if (p_buffer == nil)
		return;

	if (p_buffer->egl_image == EGL_NO_IMAGE_KHR)
		return;

	EGLDisplay t_display;
	t_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglDestroyImageKHR(t_display, p_buffer->egl_image);
	p_buffer->egl_image = EGL_NO_IMAGE_KHR;
}

bool MCGLHardwareBufferIsSupported(void)
{
	return MCAndroidHardwareBufferInitialize();
}

bool MCGLHardwareBufferCreate(uindex_t p_width, uindex_t p_height, MCGLHardwareBufferRef &r_buffer)
{
	MCLog("MCGLHardwareBufferCreate");
	if (!MCAndroidHardwareBufferInitialize())
		return false;

	bool t_success = true;

	MCGLHardwareBufferRef t_buffer = nil;
	if (t_success)
		t_success = MCMemoryNew(t_buffer);

	if (t_success)
		t_success = MCAndroidHardwareBufferAllocate(p_width, p_height, t_buffer->handle, t_buffer->stride);

	if (t_success)
		t_success = MCAndroidHardwareBufferBindToEGLImage(t_buffer);

	if (t_success)
	{
		MCLog(" - created <%p>", t_buffer);
		r_buffer = t_buffer;
		return true;
	}

	if (t_buffer != nil)
		MCGLHardwareBufferDestroy(t_buffer);

	return false;
}

void MCGLHardwareBufferDestroy(MCGLHardwareBufferRef p_buffer)
{
	MCLog("MCGLHardwareBufferDestroy <%p>", p_buffer);
	if (p_buffer == nil)
		return;

	MCAndroidHardwareBufferUnbindEGLImage(p_buffer);

	if (p_buffer->handle != nil)
		s_AHardwareBuffer_release(p_buffer->handle);

	MCMemoryDelete(p_buffer);
}

bool MCGLHardwareBufferBindToGLTexture(MCGLHardwareBufferRef p_buffer, GLuint p_texture)
{
	MCLog("MCGLHardwareBufferBindToGLTexture");
	if (!MCAndroidHardwareBufferBindToEGLImage(p_buffer))
		return false;

	glBindTexture(GL_TEXTURE_EXTERNAL_OES, p_texture);

	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, p_buffer->egl_image);

	return MCGLCheckError("Binding hardware buffer to GL texture");
}

bool MCGLHardwareBufferGetStride(MCGLHardwareBufferRef p_buffer, uindex_t &r_stride)
{
	MCLog("MCGLHardwareBufferGetStride");
	if (p_buffer == nil)
		return false;

	r_stride = p_buffer->stride;
	return true;
}

bool MCGLHardwareBufferLock(MCGLHardwareBufferRef p_buffer, void *&r_ptr)
{
	MCLog("MCGLHardwareBufferLock <%p>", p_buffer);
	if (p_buffer == nil)
		return false;

	if (p_buffer->lock == nil)
	{
		if (0 != s_AHardwareBuffer_lock(p_buffer->handle, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, nil, &p_buffer->lock))
			return false;
	}

	r_ptr = p_buffer->lock;
	return true;
}

void MCGLHardwareBufferUnlock(MCGLHardwareBufferRef p_buffer)
{
	MCLog("MCGLHardwareBufferUnlock <%p>", p_buffer);
	if (p_buffer == nil || p_buffer->lock == nil)
		return;

	int t_error;
	t_error = s_AHardwareBuffer_unlock(p_buffer->handle, nil);
	if (t_error != 0)
		MCLog("error unlocking hardwarebuffer: %d", t_error);
	p_buffer->lock = nil;
}

#else // defined(MCANDROIDUSEHARDWAREBUFFER)

#include <foundation.h>
#include <GLES3/gl3.h>
#include "glhardwarebuffer.h"

bool MCGLHardwareBufferIsSupported(void)
{
	return false;
}

bool MCGLHardwareBufferCreate(uindex_t p_width, uindex_t p_height, MCGLHardwareBufferRef &r_buffer)
{
	return false;
}

void MCGLHardwareBufferDestroy(MCGLHardwareBufferRef p_buffer)
{
}

bool MCGLHardwareBufferBindToGLTexture(MCGLHardwareBufferRef p_buffer, GLuint p_texture)
{
	return false;
}

bool MCGLHardwareBufferGetStride(MCGLHardwareBufferRef p_buffer, uindex_t &r_stride)
{
	return false;
}

bool MCGLHardwareBufferLock(MCGLHardwareBufferRef p_buffer, void *&r_ptr)
{
	return false;
}

void MCGLHardwareBufferUnlock(MCGLHardwareBufferRef p_buffer)
{
}

#endif // defined(MCANDROIDUSEHARDWAREBUFFER)
