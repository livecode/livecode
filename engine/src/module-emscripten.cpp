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

#include <jsobject.h>
#include <em-javascript.h>
#include <foundation.h>

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF
MCTypeInfoRef MCJSObjectTypeInfo()
{
	return kMCJSObjectTypeInfo;
}

////////////////////////////////////////////////////////////////////////////////

#if defined(__EMSCRIPTEN__)

extern "C" MC_DLLEXPORT_DEF
bool MCEmscriptenEvaluateJavaScriptWithArguments(MCStringRef p_script, MCProperListRef p_args, MCValueRef &r_result)
{
	return MCEmscriptenJSEvaluateScriptWithArguments(p_script, p_args, r_result);
}

extern "C" MC_DLLEXPORT_DEF
bool MCEmscriptenWrapJSEventHandler(MCHandlerRef p_handler, MCJSObjectRef &r_wrapper)
{
	return MCEmscriptenJSWrapHandler(p_handler, r_wrapper);
}

#else // !defined(__EMSCRIPTEN__)

extern "C" MC_DLLEXPORT_DEF
bool MCEmscriptenEvaluateJavaScriptWithArguments(MCStringRef p_script, MCProperListRef p_args, MCStringRef &r_result)
{
	return false;
}

extern "C" MC_DLLEXPORT_DEF
bool MCEmscriptenWrapJSEventHandler(MCHandlerRef p_handler, MCJSObjectRef &r_wrapper)
{
	return false;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF
void MCEmscriptenPointerFromJSObject(MCJSObjectRef p_object, void *&r_ptr)
{
	r_ptr = reinterpret_cast<void*>(p_object);
}

extern "C" MC_DLLEXPORT_DEF
void MCEmscriptenPointerToJSObject(void *p_ptr, MCJSObjectRef &r_obj)
{
	// Out-params of type MCValueRef should be retained
	r_obj = MCValueRetain(reinterpret_cast<MCJSObjectRef>(p_ptr));
}

////////////////////////////////////////////////////////////////////////////////////////////////////


extern "C"
bool com_livecode_emscripten_Initialize (void)
{
	if (!MCJSCreateJSObjectTypeInfo())
		return false;
		
	return true;
}

extern "C"
void com_livecode_emscripten_Finalize (void)
{
}

////////////////////////////////////////////////////////////////

