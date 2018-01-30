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

#ifndef __JSOBJECT_H__
#define __JSOBJECT_H__

////////////////////////////////////////////////////////////////////////////////

#include <foundation.h>
#include <foundation-auto.h>

typedef uintptr_t MCJSObjectID;
typedef void (*MCJSObjectReleaseCallback)(MCJSObjectID p_id);

typedef MCValueRef MCJSObjectRef;
typedef MCAutoValueRefBase<MCJSObjectRef> MCAutoJSObjectRef;

extern "C" MC_DLLEXPORT
MCTypeInfoRef kMCJSObjectTypeInfo;

extern "C" MC_DLLEXPORT
bool MCJSObjectCreate(MCJSObjectID p_id, MCJSObjectReleaseCallback p_release, MCJSObjectRef &r_object);

extern "C" MC_DLLEXPORT
MCJSObjectID MCJSObjectGetID(MCJSObjectRef p_object);

////////////////////////////////////////////////////////////////////////////////

bool MCJSCreateJSObjectTypeInfo();

////////////////////////////////////////////////////////////////////////////////

#endif
