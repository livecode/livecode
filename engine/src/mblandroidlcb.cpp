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
#include "system.h"

////////////////////////////////////////////////////////////////////////////////

extern void MCJavaPrivateDoNativeListenerCallback(jlong p_handler, jstring p_method_name, jobjectArray p_args);

#ifdef TARGET_SUBPLATFORM_ANDROID
struct remote_call_t
{
    jlong p_handler;
    jstring p_method_name;
    jobjectArray p_args;
};

static void remote_call_func(void *p_context)
{
    auto ctxt = static_cast<remote_call_t *>(p_context);
    MCJavaPrivateDoNativeListenerCallback(ctxt->p_handler, ctxt->p_method_name, ctxt->p_args);
}
#endif

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_LCBInvocationHandler_doNativeListenerCallback(JNIEnv *env, jobject object, jlong handler, jstring p_method, jobjectArray p_args) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_LCBInvocationHandler_doNativeListenerCallback(JNIEnv *env, jobject object, jlong p_handler, jstring p_method_name, jobjectArray p_args)
{
#ifdef TARGET_SUBPLATFORM_ANDROID
    extern bool MCAndroidIsOnEngineThread(void);
    if (!MCAndroidIsOnEngineThread())
    {
        typedef void (*co_yield_callback_t)(void *);
        extern void co_yield_to_engine_and_call(co_yield_callback_t callback, void *context);
        remote_call_t t_context = {p_handler, p_method_name, p_args};
        co_yield_to_engine_and_call(remote_call_func, &t_context);
    }
    else
    {
#endif
        MCJavaPrivateDoNativeListenerCallback(p_handler, p_method_name, p_args);
#ifdef TARGET_SUBPLATFORM_ANDROID
    }
#endif
    
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
    
}

////////////////////////////////////////////////////////////////////////////////
