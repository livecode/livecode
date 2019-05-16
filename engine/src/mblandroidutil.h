/* Copyright (C) 2003-2015 LiveCode Ltd.

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

#ifndef __MC_MOBILE_ANDROID_UTIL__
#define __MC_MOBILE_ANDROID_UTIL__

#include <jni.h>

// This method causes a current wait on the engine thread to be broken.
// In particular, the given MCScreenDC::wait() invocation will be exited if
// 'anyevent' was specified.
//
// ONLY CALL THIS FROM THE ANDROID UI THREAD
//
void MCAndroidBreakWait();

// This method executes a (non-static) Java method of the Engine instance
// to be invoked. The Remote form does a context switch to the UI thread.
// The non-Remote form may or may not do a context-switch :o)
//
// ONLY CALL THESE FROM THE ENGINE THREAD
//
// Here the 'signature' is a sequence of chars as follows:
//   'v' : void return value
//   's' : const char * in native encoding -> String object type
//   'S' : const MCString * in native encoding -> String object type
//   'U' : const MCString * in unicode encoding -> String object type
//   'd' : MCDataRef containing binary data -> byte[] object type
//   'i' : int32_t -> int type
//   'j' : int64_t -> int type
//   'b' : bool -> boolean type
//   'l' : JNE object reference -> java.util.List object type
//   'm' : JNE object reference -> java.util.Map object type
//   'o' : JNE object reference -> java.lang.Object object type
//   'f' : float -> float type
//   'r' : double -> double type
//
// The signature must be at least one char long - the return type being
// the first char.
//
// If the return-type char is not void, the 'return_value' must be a pointer
// to the appropriate type.
//
// If the return-type is a char *, it must be freed by the caller using
// MCCStringFree.
//
void MCAndroidEngineCall(const char *method, const char *signature, void *return_value, ...);
void MCAndroidEngineRemoteCall(const char *method, const char *signature, void *return_value, ...);
void MCAndroidObjectCall(jobject p_object, const char *p_method, const char *p_signature, void *p_return_value, ...);
void MCAndroidObjectRemoteCall(jobject p_object, const char *p_method, const char *p_signature, void *p_return_value, ...);

bool MCAndroidGetBuildInfo(MCStringRef t_key, MCStringRef &r_value);
extern "C" MC_DLLEXPORT
bool MCAndroidCheckRuntimePermission(MCStringRef p_permission);
extern "C" MC_DLLEXPORT
bool MCAndroidCheckPermissionExists(MCStringRef p_permission);
extern "C" MC_DLLEXPORT
bool MCAndroidHasPermission(MCStringRef p_permission);

typedef struct _android_device_configuration
{
	bool have_orientation_map;
	int orientation_map[4];
} MCAndroidDeviceConfiguration;

// this method reads the device configuration file from a standard location
// within the apk package, containing device specific information not
// discernable from the standard APIs.  If an entry matches the current
// device fingerprint, that entry is parsed and stored in the global
// 'MCandroiddeviceconfiguration'
// currently this contains the screen rotation -> orientation mapping
// for devices with non-standard orientations
bool MCAndroidLoadDeviceConfiguration();

void MCAndroidInitEngine();

#endif
