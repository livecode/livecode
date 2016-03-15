/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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

#include <foundation-locale.h>

extern "C" MC_DLLEXPORT_DEF void MCStringEvalConcatenate(MCStringRef p_left, MCStringRef p_right, MCStringRef& r_output)
{
	if (!MCStringCreateWithStrings(r_output, p_left, p_right))
		return;
}

extern "C" MC_DLLEXPORT_DEF void MCStringExecPutStringBefore(MCStringRef p_source, MCStringRef& x_target)
{
    MCAutoStringRef t_string;
    MCStringEvalConcatenate(p_source, x_target == (MCStringRef)kMCNull ? kMCEmptyString : x_target, &t_string);
    
    if (MCErrorIsPending())
        return;
    
    MCValueAssign(x_target, *t_string);
}

extern "C" MC_DLLEXPORT_DEF void MCStringExecPutStringAfter(MCStringRef p_source, MCStringRef& x_target)
{
    MCAutoStringRef t_string;
    MCStringEvalConcatenate(x_target == (MCStringRef)kMCNull ? kMCEmptyString : x_target, p_source, &t_string);
    
    if (MCErrorIsPending())
        return;
    
    MCValueAssign(x_target, *t_string);
}

extern "C" MC_DLLEXPORT_DEF void MCStringExecReplace(MCStringRef p_pattern, MCStringRef p_replacement, MCStringRef& x_target)
{
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(x_target, &t_string))
        return;
    
    MCStringFindAndReplace(*t_string, p_pattern, p_replacement, kMCStringOptionCompareExact);
    
    MCAutoStringRef t_new_string;
    if (!MCStringCopy(*t_string, &t_new_string))
        return;
    
    MCValueAssign(x_target, *t_new_string);
}

extern "C" MC_DLLEXPORT_DEF void MCStringEvalConcatenateWithSpace(MCStringRef p_left, MCStringRef p_right, MCStringRef& r_output)
{
	if (!MCStringCreateWithStringsAndSeparator(r_output, ' ', p_left, p_right))
		return;
}

extern "C" MC_DLLEXPORT_DEF void MCStringEvalLowercaseOf(MCStringRef p_source, MCStringRef& r_output)
{
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(p_source, &t_string))
        return;
    
    if (!MCStringLowercase(*t_string, kMCLocaleBasic))
        return;
    
    if (!MCStringCopy(*t_string, r_output))
        return;
}

extern "C" MC_DLLEXPORT_DEF void MCStringEvalUppercaseOf(MCStringRef p_source, MCStringRef& r_output)
{
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(p_source, &t_string))
        return;
    
    if (!MCStringUppercase(*t_string, kMCLocaleBasic))
        return;
    
    if (!MCStringCopy(*t_string, r_output))
        return;
}

extern "C" MC_DLLEXPORT_DEF void MCStringEvalIsEqualTo(MCStringRef p_left, MCStringRef p_right, bool& r_result)
{
    r_result = MCStringIsEqualTo(p_left, p_right, kMCStringOptionCompareExact);
}

extern "C" MC_DLLEXPORT_DEF void MCStringEvalIsNotEqualTo(MCStringRef p_left, MCStringRef p_right, bool& r_result)
{
    r_result = !MCStringIsEqualTo(p_left, p_right, kMCStringOptionCompareExact);
}

extern "C" MC_DLLEXPORT_DEF void MCStringEvalIsLessThan(MCStringRef p_left, MCStringRef p_right, bool& r_result)
{
    r_result = MCStringCompareTo(p_left, p_right, kMCStringOptionCompareExact) < 0;
}

extern "C" MC_DLLEXPORT_DEF void MCStringEvalIsGreaterThan(MCStringRef p_left, MCStringRef p_right, bool& r_result)
{
    r_result = MCStringCompareTo(p_left, p_right, kMCStringOptionCompareExact) > 0;
}

extern "C" MC_DLLEXPORT_DEF void MCStringEvalEmpty(MCStringRef& r_output)
{
    r_output = MCValueRetain(kMCEmptyString);
}

////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_string_Initialize (void)
{
	return true;
}

extern "C" void com_livecode_string_Finalize (void)
{
}

////////////////////////////////////////////////////////////////
