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

// Work around an SDK bug in 64-bit builds
#define HAVE_INTTYPES_H 1

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
#include "system.h"

////////////////////////////////////////////////////////////////////////////////

extern jobject MCJavaPrivateDoNativeListenerCallback(jlong p_handler, jstring p_method_name, jobjectArray p_args);

extern "C" JNIEXPORT jobject JNICALL Java_com_runrev_android_LCBInvocationHandler_doNativeListenerCallback(JNIEnv *env, jobject object, jlong handler, jstring p_method, jobjectArray p_args) __attribute__((visibility("default")));

JNIEXPORT jobject JNICALL Java_com_runrev_android_LCBInvocationHandler_doNativeListenerCallback(JNIEnv *env, jobject object, jlong p_handler, jstring p_method_name, jobjectArray p_args)
{
    jobject t_result =
            MCJavaPrivateDoNativeListenerCallback(p_handler, p_method_name, p_args);

    // At the moment we have no way of dealing with any errors thrown in
    // the course of handling or attempting to handle the native listener
    // callback, so
    MCAutoErrorRef t_error;
    if (MCErrorCatch(&t_error))
    {
        MCAutoStringRef t_string;
        /* UNCHECKED */ MCStringFormat(&t_string, "%@", *t_error);
        
        MCsystem->Debug(*t_string);
    }
    
    return t_result;
}

////////////////////////////////////////////////////////////////////////////////
