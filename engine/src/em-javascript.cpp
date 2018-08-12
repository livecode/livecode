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
 * Interface to JavaScript:
 *     Evaluating JS scripts.
 *     Referencing JS objects, properties & methods.
 *     Value type conversion.
 */

#include <em-javascript.h>

////////////////////////////////////////////////////////////////////////////////

// Implemented in JavaScript
extern "C" bool MCEmscriptenSystemEvaluateJavaScriptWithArguments(MCStringRef p_script, MCProperListRef p_args, MCValueRef *r_result);
extern "C" void MCEmscriptenUtilReleaseObject(uintptr_t pObjectID);
extern "C" MCJSObjectRef MCEmscriptenSystemWrapHandler(MCHandlerRef pHandler);

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF
bool MCEmscriptenJSEvaluateScriptWithArguments(MCStringRef p_script, MCProperListRef p_args, MCValueRef &r_result)
{
	bool t_success = true;
	MCAutoValueRef t_result;
	t_success = MCEmscriptenSystemEvaluateJavaScriptWithArguments(p_script, p_args, &(&t_result));
	
	if (!t_success)
		return MCErrorThrowGeneric((MCStringRef)*t_result);
	
	r_result = t_result.Take();
	return true;
}

extern "C" MC_DLLEXPORT_DEF
bool MCEmscriptenJSEvaluateScript(MCStringRef p_script, MCValueRef &r_result)
{
	return MCEmscriptenJSEvaluateScriptWithArguments(p_script, kMCEmptyProperList, r_result);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF
bool MCEmscriptenJSWrapHandler(MCHandlerRef p_handler, MCJSObjectRef &r_wrapper)
{
	bool t_success = true;
	MCJSObjectRef t_wrapper;
	t_wrapper = MCEmscriptenSystemWrapHandler(p_handler);
	if (t_wrapper == nil)
		return MCErrorThrowGeneric(MCSTR("Failed to wrap handler"));
	
	r_wrapper = t_wrapper;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

// Callable from JavaScript
extern "C" MC_DLLEXPORT_DEF
MCJSObjectRef MCEmscriptenJSObjectFromID(MCJSObjectID p_id)
{
	MCJSObjectRef t_obj;
	if (!MCJSObjectCreate(p_id, MCEmscriptenUtilReleaseObject, t_obj))
		return nil;
	
	return t_obj;
}

extern "C" MC_DLLEXPORT_DEF
MCJSObjectID MCEmscriptenJSObjectGetID(MCJSObjectRef p_obj)
{
	return MCJSObjectGetID(p_obj);
}

extern "C" MC_DLLEXPORT_DEF
bool MCEmscriptenIsJSObject(MCValueRef p_value)
{
	return MCValueGetTypeInfo(p_value) == kMCJSObjectTypeInfo;
}

////////////////////////////////////////////////////////////////////////////////

bool MCEmscriptenJSInitialize(void)
{
    return true;
}

void MCEmscriptenJSFinalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////
