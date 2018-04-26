/* Copyright (C) 2018 LiveCode Ltd.
 
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

/*
 * Implementation of MCJSObjectRef custom value type for containing
 * reference to JavaScript objects.
 */

#include <jsobject.h>

////////////////////////////////////////////////////////////////////////////////

struct __MCJSObjectImpl
{
	MCJSObjectID obj_id;
	MCJSObjectReleaseCallback release;
};

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF
MCTypeInfoRef kMCJSObjectTypeInfo = nil;

////////////////////////////////////////////////////////////////////////////////

static void __MCJSObjectDestroy(MCValueRef p_value)
{
    __MCJSObjectImpl *self;
    self = (__MCJSObjectImpl *)MCValueGetExtraBytesPtr(p_value);
    
    // release the underlying object reference
    if (self->release)
		self->release(self->obj_id);
    
    self->~__MCJSObjectImpl();
}

static bool __MCJSObjectCopy(MCValueRef p_value, bool p_release, MCValueRef& r_copy)
{
    if (p_release)
        r_copy = p_value;
    else
        r_copy = MCValueRetain(p_value);
    
    return true;
}

static bool __MCJSObjectEqual(MCValueRef p_left, MCValueRef p_right)
{
	__MCJSObjectImpl *t_left, *t_right;
	t_left = (__MCJSObjectImpl *)MCValueGetExtraBytesPtr(p_left);
	t_right = (__MCJSObjectImpl *)MCValueGetExtraBytesPtr(p_right);
	
	return t_left->obj_id == t_right->obj_id;
}

static hash_t __MCJSObjectHash(MCValueRef p_value)
{
	__MCJSObjectImpl *self;
	self = (__MCJSObjectImpl *)MCValueGetExtraBytesPtr(p_value);

	return MCHashUInteger(self->obj_id);
}

static bool __MCJSObjectDescribe(MCValueRef p_value, MCStringRef& r_description)
{
	__MCJSObjectImpl *self;
	self = (__MCJSObjectImpl *)MCValueGetExtraBytesPtr(p_value);
    
    return MCStringFormat(r_description,
                          "<javascript object %d>",
                          self->obj_id);
}

static MCValueCustomCallbacks kMCJSObjectCustomValueCallbacks =
{
    false,
    __MCJSObjectDestroy,
    __MCJSObjectCopy,
    __MCJSObjectEqual,
    __MCJSObjectHash,
    __MCJSObjectDescribe,
    nil,
    nil,
};

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF
bool MCJSObjectCreate(MCJSObjectID p_obj_id, MCJSObjectReleaseCallback p_release, MCJSObjectRef& r_js_object)
{
    MCJSObjectRef t_js_object;
    if (!MCValueCreateCustom(kMCJSObjectTypeInfo, sizeof(__MCJSObjectImpl), t_js_object))
        return false;
    
    __MCJSObjectImpl *t_js_object_imp;
    t_js_object_imp = (__MCJSObjectImpl *)MCValueGetExtraBytesPtr(t_js_object);
    t_js_object_imp->obj_id = p_obj_id;
    t_js_object_imp->release = p_release;
    
    r_js_object = t_js_object;
    
    return true;
}

extern "C" MC_DLLEXPORT_DEF
MCJSObjectID MCJSObjectGetID(MCJSObjectRef p_value)
{
	__MCJSObjectImpl *self;
	self = (__MCJSObjectImpl *)MCValueGetExtraBytesPtr(p_value);
	
	return self->obj_id;
}

////////////////////////////////////////////////////////////////////////////////

bool MCJSCreateJSObjectTypeInfo(void)
{
	if (kMCJSObjectTypeInfo != nil)
		return true;
		
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.emscripten.JSObject"), kMCNullTypeInfo, &kMCJSObjectCustomValueCallbacks, kMCJSObjectTypeInfo))
		return false;
	
    return true;
}

////////////////////////////////////////////////////////////////////////////////
