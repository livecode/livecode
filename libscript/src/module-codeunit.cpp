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
#include <foundation-chunk.h>

extern "C" MC_DLLEXPORT_DEF void MCCodeunitEvalNumberOfCodeunitsIn(MCStringRef p_target, index_t& r_output)
{
    r_output = MCStringGetLength(p_target);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitFetchCodeunitRangeOf(index_t p_start, index_t p_finish, MCStringRef p_target, MCStringRef& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfCodeunitChunkByRangeInRange(p_target, nil, p_start, p_finish, true, false, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    if (!MCStringCopySubstring(p_target, MCRangeMake(t_start, t_count), r_output))
        return;
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitStoreCodeunitRangeOf(MCStringRef p_value, index_t p_start, index_t p_finish, MCStringRef& x_target)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfCodeunitChunkByRangeInRange(x_target, nil, p_start, p_finish, true, false, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(x_target, &t_string))
        return;
    
    if (!MCStringReplace(*t_string, MCRangeMake(t_start, t_count), p_value))
        return;
    
    MCAutoStringRef t_new_string;
    if (!MCStringCopy(*t_string, &t_new_string))
        return;
    
    MCValueAssign(x_target, *t_new_string);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitFetchCodeunitOf(index_t p_index, MCStringRef p_target, MCStringRef& r_output)
{
    MCCodeunitFetchCodeunitRangeOf(p_index, p_index, p_target, r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitStoreCodeunitOf(MCStringRef p_value, index_t p_index, MCStringRef& x_target)
{
    MCCodeunitStoreCodeunitRangeOf(p_value, p_index, p_index, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitEvalOffsetOfCodeunitsInRange(bool p_is_last, MCStringRef p_needle, MCStringRef p_target, MCRange p_range, uindex_t& r_output)
{
    uindex_t t_offset;
    t_offset = 0;
    if (!MCStringIsEmpty(p_needle))
    {
        bool t_found;
        if (p_is_last)
            t_found = MCStringLastIndexOfStringInRange(p_target, p_needle, p_range, kMCStringOptionCompareExact, t_offset);
        else
            t_found = MCStringFirstIndexOfStringInRange(p_target, p_needle, p_range, kMCStringOptionCompareExact, t_offset);
        
        // correct output index
        if (t_found)
        {
            t_offset -= p_range . offset;
            t_offset++;
        }
    }
    r_output = t_offset;
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitEvalOffsetOfCodeunits(bool p_is_last, MCStringRef p_needle, MCStringRef p_target, uindex_t& r_output)
{
    MCCodeunitEvalOffsetOfCodeunitsInRange(p_is_last, p_needle, p_target, MCRangeMake(0, UINDEX_MAX), r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitEvalOffsetOfCodeunitsAfter(bool p_is_last, MCStringRef p_needle, index_t p_after, MCStringRef p_target, uindex_t& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfCodeunitChunkByExpressionInRange(p_target, nil, p_after, true, true, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCCodeunitEvalOffsetOfCodeunitsInRange(p_is_last, p_needle, p_target, MCRangeMake(t_start + t_count, UINDEX_MAX), r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitEvalOffsetOfCodeunitsBefore(bool p_is_first, MCStringRef p_needle, index_t p_before, MCStringRef p_target, uindex_t& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfCodeunitChunkByExpressionInRange(p_target, nil, p_before, true, false, true, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCCodeunitEvalOffsetOfCodeunitsInRange(!p_is_first, p_needle, p_target, MCRangeMake(0, t_start), r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitEvalContains(MCStringRef p_source, MCStringRef p_needle, bool& r_result)
{
    r_result = MCStringContains(p_source, p_needle, kMCStringOptionCompareExact);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitEvalBeginsWith(MCStringRef p_source, MCStringRef p_prefix, bool& r_result)
{
    r_result = MCStringBeginsWith(p_source, p_prefix, kMCStringOptionCompareExact);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitEvalEndsWith(MCStringRef p_source, MCStringRef p_suffix, bool& r_result)
{
    r_result = MCStringEndsWith(p_source, p_suffix, kMCStringOptionCompareExact);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitFetchFirstCodeunitOf(MCStringRef p_target, MCStringRef& r_output)
{
    MCCodeunitFetchCodeunitOf(1, p_target, r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitStoreFirstCodeunitOf(MCStringRef p_value, MCStringRef& x_target)
{
    MCCodeunitStoreCodeunitOf(p_value, 1, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitFetchLastCodeunitOf(MCStringRef p_target, MCStringRef& r_output)
{
    MCCodeunitFetchCodeunitOf(-1, p_target, r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitStoreLastCodeunitOf(MCStringRef p_value, MCStringRef& x_target)
{
    MCCodeunitStoreCodeunitOf(p_value, -1, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitExecDeleteCodeunitRangeOf(index_t p_start, index_t p_finish, MCStringRef& x_target)
{
    MCCodeunitStoreCodeunitRangeOf(kMCEmptyString, p_start, p_finish, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitExecDeleteCodeunitOf(index_t p_index, MCStringRef& x_target)
{
    MCCodeunitStoreCodeunitOf(kMCEmptyString, p_index, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitExecDeleteFirstCodeunitOf(MCStringRef& x_target)
{
    MCCodeunitExecDeleteCodeunitOf(1, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCCodeunitExecDeleteLastCodeunitOf(MCStringRef& x_target)
{
    MCCodeunitExecDeleteCodeunitOf(-1, x_target);
}

// Iterate syntax methods have special calling convention at the moment:
//
// Post assignment of out / inout variables only occurs if the method returns true.
// If the method returns false, then it means iteration has finished *not* that an
// error has been thrown.
//
// This means that the iterand out binding will not be updated on the final test
// of the loop which means that:
//   repeat for each char tCodeunit in tVar
//   end repeat
// Will result in tCodeunit containing the value it had at the point of end repeat.
extern "C" MC_DLLEXPORT_DEF bool MCCodeunitRepeatForEachCodeunit(void*& x_iterator, MCStringRef& r_iterand, MCStringRef p_string)
{
    uintptr_t t_offset;
    t_offset = (uintptr_t)x_iterator;
    
    if (t_offset == MCStringGetLength(p_string))
        return false;
    
    if (!MCStringCopySubstring(p_string, MCRangeMake(t_offset, 1), r_iterand))
        return false;
    
    x_iterator = (void *)(t_offset + 1);
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_codeunit_Initialize(void)
{
    return true;
}

extern "C" void com_livecode_codeunit_Finalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
