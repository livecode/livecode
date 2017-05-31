/* Copyright (C) 2017 LiveCode Ltd.
 
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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "globals.h"
#include "object.h"
#include "mbldc.h"

#include "mblandroid.h"
#include "mblandroidutil.h"

#include "mblsyntax.h"

////////////////////////////////////////////////////////////////////////////////

extern void MCJavaPrivateDoNativeListenerCallback(jlong p_handler, jstring p_method_name, jobjectArray p_args);

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_LCBInvocationHandler_doNativeListenerCallback(JNIEnv *env, jobject object, jlong handler, jstring p_method, jobjectArray p_args) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_LCBInvocationHandler_doNativeListenerCallback(JNIEnv *env, jobject object, jlong p_handler, jstring p_method_name, jobjectArray p_args)
{
    MCJavaPrivateDoNativeListenerCallback(p_handler, p_method_name, p_args);
}

////////////////////////////////////////////////////////////////////////////////
