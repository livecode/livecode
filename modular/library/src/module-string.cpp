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

void MCStringEvalConcatenate(MCStringRef p_left, MCStringRef p_right, MCStringRef& r_output)
{
    if (!MCStringFormat(r_output, "%@%@", p_left, p_right))
        return;
}

void MCStringExecPutStringBefore(MCStringRef p_source, MCStringRef& x_target)
{
    MCAutoStringRef t_string;
    MCStringEvalConcatenate(p_source, x_target, &t_string);
    
    MCValueAssign(x_target, *t_string);
}

void MCStringExecPutStringAfter(MCStringRef p_source, MCStringRef& x_target)
{
    MCAutoStringRef t_string;
    MCStringEvalConcatenate(x_target, p_source, &t_string);
    
    MCValueAssign(x_target, *t_string);
}

void MCStringEvalConcatenateWithSpace(MCStringRef p_left, MCStringRef p_right, MCStringRef& r_output)
{
    if (!MCStringFormat(r_output, "%@ %@", p_left, p_right))
        return;
}

void MCStringEvalBeginsWith(MCHandlerContext& ctxt, MCStringRef p_source, MCStringRef p_prefix, bool& r_result)
{
    r_result = MCStringBeginsWith(p_source, p_prefix, ctxt . GetStringComparisonOptions());
}

void MCStringEvalEndsWith(MCHandlerContext& ctxt, MCStringRef p_source, MCStringRef p_suffix, bool& r_result)
{
    r_result = MCStringBeginsWith(p_source, p_prefix, ctxt . GetStringComparisonOptions());
}

void MCStringEvalLowercaseOf(MCStringRef p_source, MCStringRef& r_output)
{
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(p_source, &t_string))
        return;
    
    if (!MCStringLowercase(*t_string, kMCBasicLocale))
        return;
    
    if (!MCStringCopy(*t_string, r_output))
        return;
}

void MCStringEvalUppercaseOf(MCStringRef p_source, MCStringRef& r_output)
{
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(p_source, &t_string))
        return;
    
    if (!MCStringUppercase(*t_string, kMCBasicLocale))
        return;
    
    if (!MCStringCopy(*t_string, r_output))
        return;
}

void MCStringEvalOffset(MCHandlerContext& ctxt, MCStringRef p_needle, MCStringRef p_source, uindex_t& r_output)
{
    MCStringEvalOffsetAfter(ctxt, p_needle, p_after, p_source, r_output);
}

void MCStringEvalOffsetAfter(MCHandlerContext& ctxt, MCStringRef p_needle, uindex_t p_after, MCStringRef p_source, uindex_t& r_output)
{
    uindex_t t_output = 0;
    if (!MCStringIsEmpty(p_needle))
        MCStringFirstIndexOf(p_source, p_needle, p_after, ctxt . GetStringComparisonOptions(), t_output);
    r_output = t_output - p_after + 1;
}

void MCStringEvalLastOffset(MCHandlerContext& ctxt, MCStringRef p_needle, MCStringRef p_source, uindex_t& r_output)
{
    MCStringEvalLastOffsetBefore(ctxt, p_needle, UINDEX_MAX, p_source, r_output);
}

void MCStringEvalLastOffsetBefore(MCHandlerContext& ctxt, MCStringRef p_needle, uindex_t p_before, MCStringRef p_source, uindex_t& r_output)
{
    uindex_t t_output = 0;
    if (!MCStringIsEmpty(p_needle))
        MCStringFirstIndexOf(p_source, p_needle, p_before, ctxt . GetStringComparisonOptions(), t_output);
    r_output = t_output + 1;
}
