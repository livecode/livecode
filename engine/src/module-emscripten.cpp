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

#include <foundation.h>
#include <foundation-auto.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

extern bool MCEmscriptenSystemEvaluateJavaScriptAsString(MCStringRef p_script, MCStringRef &r_result);

#if defined(__EMSCRIPTEN__)

extern "C" MC_DLLEXPORT_DEF bool MCEmscriptenEvaluateJavaScriptAsString(MCStringRef p_script, MCStringRef &r_result)
{
	return MCEmscriptenSystemEvaluateJavaScriptAsString(p_script, r_result);
}

#else // !defined(__EMSCRIPTEN__)

extern "C" MC_DLLEXPORT_DEF bool MCEmscriptenEvaluateJavaScriptAsString(MCStringRef p_script, MCStringRef &r_result)
{
	return false;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void MCEmscriptenPointerFromObjectRef(uintptr_t p_ref, void *&r_ptr)
{
	r_ptr = reinterpret_cast<void*>(p_ref);
}

extern "C" MC_DLLEXPORT_DEF void MCEmscriptenPointerToObjectRef(void *p_ptr, uintptr_t &r_ref)
{
	r_ref = reinterpret_cast<uintptr_t>(p_ptr);
}

////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_emscripten_Initialize (void)
{
	return true;
}

extern "C" void com_livecode_emscripten_Finalize (void)
{
}

////////////////////////////////////////////////////////////////

