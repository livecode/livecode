/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

extern "C" MC_DLLEXPORT void MCStringEvalConcatenate(MCStringRef p_left, MCStringRef p_right, MCStringRef& r_output)
{
    if (!MCStringFormat(r_output, "%@%@", p_left, p_right))
        return;
}

extern "C" MC_DLLEXPORT void MCStringExecPutStringBefore(MCStringRef p_source, MCStringRef& x_target)
{
    MCAutoStringRef t_string;
    MCStringEvalConcatenate(p_source, x_target == (MCStringRef)kMCNull ? kMCEmptyString : x_target, &t_string);
    
    if (MCErrorIsPending())
        return;
    
    MCValueAssign(x_target, *t_string);
}

extern "C" MC_DLLEXPORT void MCStringExecPutStringAfter(MCStringRef p_source, MCStringRef& x_target)
{
    MCAutoStringRef t_string;
    MCStringEvalConcatenate(x_target == (MCStringRef)kMCNull ? kMCEmptyString : x_target, p_source, &t_string);
    
    if (MCErrorIsPending())
        return;
    
    MCValueAssign(x_target, *t_string);
}

extern "C" MC_DLLEXPORT void MCStringExecReplace(MCStringRef p_pattern, MCStringRef p_replacement, MCStringRef& x_target)
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

extern "C" MC_DLLEXPORT void MCStringEvalConcatenateWithSpace(MCStringRef p_left, MCStringRef p_right, MCStringRef& r_output)
{
    if (!MCStringFormat(r_output, "%@ %@", p_left, p_right))
        return;
}

extern "C" MC_DLLEXPORT void MCStringEvalLowercaseOf(MCStringRef p_source, MCStringRef& r_output)
{
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(p_source, &t_string))
        return;
    
    if (!MCStringLowercase(*t_string, kMCLocaleBasic))
        return;
    
    if (!MCStringCopy(*t_string, r_output))
        return;
}

extern "C" MC_DLLEXPORT void MCStringEvalUppercaseOf(MCStringRef p_source, MCStringRef& r_output)
{
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(p_source, &t_string))
        return;
    
    if (!MCStringUppercase(*t_string, kMCLocaleBasic))
        return;
    
    if (!MCStringCopy(*t_string, r_output))
        return;
}

extern "C" MC_DLLEXPORT void MCStringEvalIsEqualTo(MCStringRef p_left, MCStringRef p_right, bool& r_result)
{
    r_result = MCStringIsEqualTo(p_left, p_right, kMCStringOptionCompareExact);
}

extern "C" MC_DLLEXPORT void MCStringEvalIsNotEqualTo(MCStringRef p_left, MCStringRef p_right, bool& r_result)
{
    r_result = !MCStringIsEqualTo(p_left, p_right, kMCStringOptionCompareExact);
}

extern "C" MC_DLLEXPORT void MCStringEvalIsLessThan(MCStringRef p_left, MCStringRef p_right, bool& r_result)
{
    r_result = MCStringCompareTo(p_left, p_right, kMCStringOptionCompareExact) < 0;
}

extern "C" MC_DLLEXPORT void MCStringEvalIsGreaterThan(MCStringRef p_left, MCStringRef p_right, bool& r_result)
{
    r_result = MCStringCompareTo(p_left, p_right, kMCStringOptionCompareExact) > 0;
}

extern "C" MC_DLLEXPORT void MCStringEvalCodeOfChar(MCStringRef p_string, uinteger_t& r_code)
{
    uindex_t t_length;
    t_length = MCStringGetLength(p_string);
    if (t_length == 0 || t_length > 2)
        goto notacodepoint_exit;
    
    codepoint_t t_code;
    t_code = MCStringGetCodepointAtIndex(p_string, 0);
    if (t_length > 1 &&
        t_code < 65536)
        goto notacodepoint_exit;
    
    r_code = t_code;
    
    return;
    
notacodepoint_exit:
    MCErrorThrowGeneric(MCSTR("not a single code character"));
}

extern "C" MC_DLLEXPORT void MCStringEvalCharWithCode(uinteger_t p_code, MCStringRef& r_string)
{
    if (p_code >= 1 << 21)
    {
        MCErrorThrowGeneric(MCSTR("code out of range"));
        return;
    }
    
    if (p_code >= 1 << 16)
    {
        unichar_t t_codeunits[2];
        t_codeunits[0] =  unichar_t((p_code - 0x10000) >> 10) + 0xD800;
        t_codeunits[1] = unichar_t((p_code - 0x10000) & 0x3FF) + 0xDC00;
        MCStringCreateWithChars(t_codeunits, 2, r_string);
        return;
    }
    
    unichar_t t_codeunit;
    t_codeunit = (unichar_t)p_code;
    MCStringCreateWithChars(&t_codeunit, 1, r_string);
}

extern "C" MC_DLLEXPORT void MCStringEvalEmpty(MCStringRef& r_output)
{
    r_output = MCValueRetain(kMCEmptyString);
}
