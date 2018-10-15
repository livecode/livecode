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

#ifndef __EM_JAVASCRIPT_H__
#define __EM_JAVASCRIPT_H__

////////////////////////////////////////////////////////////////////////////////

#include <jsobject.h>

extern "C" MC_DLLEXPORT
bool MCEmscriptenJSEvaluateScript(MCStringRef p_script, MCValueRef &r_result);

extern "C" MC_DLLEXPORT
bool MCEmscriptenJSEvaluateScriptWithArguments(MCStringRef p_script, MCProperListRef p_args, MCValueRef &r_result);

extern "C" MC_DLLEXPORT
bool MCEmscriptenJSWrapHandler(MCHandlerRef p_handler, MCJSObjectRef &r_wrapper);

////////////////////////////////////////////////////////////////////////////////

bool MCEmscriptenJSInitialize();
void MCEmscriptenJSFinalize();

////////////////////////////////////////////////////////////////////////////////

#endif
