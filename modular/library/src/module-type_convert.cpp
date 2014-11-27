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

static bool MCProperListCombine(void *context, MCValueRef p_value)
{
    MCListRef t_list = *(MCListRef *)context;
    return MCListAppend(t_list, p_value);
}

extern "C" void MCTypeConvertExecSplitByDelimiter(MCStringRef p_target, MCStringRef p_delimiter, MCProperListRef& r_output)
{
    if (!MCStringSplitByDelimiter(p_target, p_delimiter, kMCStringOptionCompareCaseless, r_output))
		return;
}

extern "C" void MCTypeConvertExecCombineWithDelimiter(MCProperListRef p_target, MCStringRef p_delimiter, MCStringRef& r_output)
{
    MCListRef t_list;
    MCListCreateMutable(p_delimiter, t_list);
    
    if (!MCProperListApply(p_target, MCProperListCombine, &t_list))
        return;
    
    MCListCopyAsStringAndRelease(t_list, r_output);
}