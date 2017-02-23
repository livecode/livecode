/* Copyright (C) 2003-2016 LiveCode Ltd.

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

#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-private.h"
#include "foundation-java-private.h"

static bool s_java_initialised = false;

MC_DLLEXPORT_DEF
bool MCJavaInitialize()
{
	if (s_java_initialised)
		return true;
	
    s_java_initialised = initialise_jvm();
    return s_java_initialised;
}

MC_DLLEXPORT_DEF
void MCJavaFinalize()
{
    finalise_jvm();
}

MC_DLLEXPORT_DEF MCTypeInfoRef kMCJavaObjectTypeInfo;

MC_DLLEXPORT_DEF MCTypeInfoRef MCJavaGetObjectTypeInfo() { return kMCJavaObjectTypeInfo; }

struct __MCJavaObjectImpl
{
    void *object;
};

__MCJavaObjectImpl *MCJavaObjectGet(MCJavaObjectRef p_obj)
{
    return (__MCJavaObjectImpl*)MCValueGetExtraBytesPtr(p_obj);
}

static inline __MCJavaObjectImpl MCJavaObjectImplMake(void* p_obj)
{
    __MCJavaObjectImpl t_obj;
    t_obj.object = p_obj;
    return t_obj;
}

static void __MCJavaObjectDestroy(MCValueRef p_value)
{
    // no-op
}

static bool __MCJavaObjectCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
    if (p_release)
        r_copy = p_value;
    else
        r_copy = MCValueRetain(p_value);
    return true;
}

static bool __MCJavaObjectEqual(MCValueRef p_left, MCValueRef p_right)
{
    if (p_left == p_right)
        return true;
    
    return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_right), sizeof(__MCJavaObjectImpl)) == 0;
}

static hash_t __MCJavaObjectHash(MCValueRef p_value)
{
    return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCJavaObjectImpl));
}

bool __MCJavaObjectDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
    if (!s_java_initialised)
        return false;

    return MCJavaPrivateObjectDescribe(p_value, r_desc);
}

static MCValueCustomCallbacks kMCJavaObjectCustomValueCallbacks =
{
    true,
    __MCJavaObjectDestroy,
    __MCJavaObjectCopy,
    __MCJavaObjectEqual,
    __MCJavaObjectHash,
    __MCJavaObjectDescribe,
    
    nil,
    nil,
};

MC_DLLEXPORT_DEF bool MCJavaObjectCreate(void *p_object, MCJavaObjectRef &r_object)
{
    bool t_success;
    t_success = true;
    
    MCJavaObjectRef t_obj;
    t_obj = nil;
    
    t_success = MCValueCreateCustom(kMCJavaObjectTypeInfo, sizeof(__MCJavaObjectImpl), t_obj);
    
    if (t_success)
    {
        *MCJavaObjectGet(t_obj) = MCJavaObjectImplMake(p_object);
        r_object = t_obj;
    }

    return t_success;
}

bool MCJavaCreateJavaObjectTypeInfo()
{
    return MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.java.JavaObject"), kMCNullTypeInfo, &kMCJavaObjectCustomValueCallbacks, kMCJavaObjectTypeInfo);
}

MC_DLLEXPORT_DEF void *MCJavaObjectGetObject(const MCJavaObjectRef p_obj)
{
    __MCJavaObjectImpl *t_impl;
    t_impl = MCJavaObjectGet(p_obj);
    return t_impl -> object;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCJavaCheckSignature(MCTypeInfoRef p_signature, MCStringRef p_args, MCStringRef p_return, int p_call_type)
{
    return MCJavaPrivateCheckSignature(p_signature, p_args, p_return, p_call_type);
}

MC_DLLEXPORT_DEF
bool MCJavaCallJNIMethod(MCNameRef p_class_name, void *p_method_id, int p_call_type, MCTypeInfoRef p_signature, void *r_return, void **p_args, uindex_t p_arg_count)
{
    if (!s_java_initialised)
        return false;
    
    return MCJavaPrivateCallJNIMethod(p_class_name, p_method_id, p_call_type,  p_signature, r_return, p_args, p_arg_count);
}

MC_DLLEXPORT_DEF bool MCJavaConvertJStringToStringRef(MCJavaObjectRef p_object, MCStringRef &r_string)
{
    if (!s_java_initialised)
        return false;
   
    return MCJavaPrivateConvertJStringToStringRef(p_object, r_string);
}

MC_DLLEXPORT_DEF bool MCJavaConvertStringRefToJString(MCStringRef p_string, MCJavaObjectRef &r_object)
{
    if (!s_java_initialised)
        return false;
    
    return MCJavaPrivateConvertStringRefToJString(p_string, r_object);
}

MC_DLLEXPORT_DEF bool MCJavaCallConstructor(MCNameRef p_class_name, MCListRef p_args, MCJavaObjectRef& r_object)
{
    if (!s_java_initialised)
        return false;

    return MCJavaPrivateCallConstructor(p_class_name, p_args, r_object);
}

MC_DLLEXPORT_DEF void *MCJavaGetMethodId(MCNameRef p_class_name, MCStringRef p_method_name, MCStringRef p_signature)
{
    if (!s_java_initialised)
        return nil;

    return MCJavaPrivateGetMethodId(p_class_name, p_method_name, p_signature);
}
