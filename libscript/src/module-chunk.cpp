/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
 This file is part of LiveCode.
 
 LiveCode is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License v3 as published by the Free
 Software Foundation.
 
 LiveCode is distributed the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include <foundation.h>
#include <foundation-auto.h>
#include <foundation-chunk.h>

static MCStringRef MCChunkGetChunkDelimiter()
{
    return MCSTR(",");
}

extern "C" MC_DLLEXPORT void MCChunkFetchChunkOf(index_t p_index, MCStringRef p_target, MCStringRef& r_chunk)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkStoreChunkOf(MCStringRef p_chunk, index_t p_index, MCStringRef& x_target)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkFetchChunkRangeOf(index_t p_start, index_t p_finish, MCStringRef p_target, MCStringRef& r_chunk)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfChunkChunkByRange(p_target, MCChunkGetChunkDelimiter(), kMCStringOptionCompareCaseless, p_start, p_finish, t_start, t_count);
    
    MCChunkEvalChunks
    
    
    
    
    if (t_count == 0 || t_start + t_count > MCStringGetLength(p_target))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    if (!MCStringCopySubstring(p_target, MCRangeMake(t_start, t_count), r_output))
        return;
}

extern "C" MC_DLLEXPORT void MCChunkStoreChunkRangeOf(MCStringRef p_chunk, index_t p_start, index_t p_finish, MCStringRef& x_target)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkFetchFirstChunkOf(MCStringRef p_target, MCStringRef& r_chunk)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkStoreFirstChunkOf(MCStringRef p_chunk, MCStringRef& x_target)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkFetchLastChunkOf(MCStringRef p_target, MCStringRef& r_chunk)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkStoreLastChunkOf(MCStringRef p_chunk, MCStringRef& x_target)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkExecDeleteChunkOf(index_t p_index, MCStringRef& x_target)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkExecDeleteChunkRangeOf(index_t p_start, index_t p_finish, MCStringRef& x_target)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkExecDeleteFirstChunkOf(MCStringRef& x_target)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkExecDeleteLastChunkOf(MCStringRef& x_target)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkEvalNumberOfChunksIn(MCStringRef p_target, uindex_t& r_count)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkEvalIsAmongTheChunksOf(MCStringRef p_needle, MCStringRef p_target, bool& r_result)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkEvalIsNotAmongTheChunksOf(MCStringRef p_needle, MCStringRef p_target, bool& r_result)
{
    
}

extern "C" MC_DLLEXPORT void MCChunkEvalOffsetOfChunkIn(bool p_partial, MCStringRef p_needle, MCStringRef p_target, uindex_t& r_count)
{
    
}

static bool add_chunk_to_list(void *context, MCStringRef p_string, MCRange p_range)
{
    MCProperListRef t_list;
    t_list = *(MCProperListRef *)context;
    
    MCAutoStringRef t_string;
    return MCStringCopySubstring(p_string, p_range, &t_string) &&
            MCProperListPushElementOntoBack(t_list, *t_string);
}

extern "C" MC_DLLEXPORT void MCChunkEvalChunksOf(MCStringRef p_target, MCProperListRef& r_chunks)
{
    MCProperListRef t_list;
    MCProperListCreateMutable(t_list);
    
    if (MCChunkApply(p_target, MCChunkGetChunkDelimiter(), kMCStringOptionCompareCaseless, add_chunk_to_list, &t_list))
        MCProperListCopy(t_list, r_chunks);
}

extern "C" MC_DLLEXPORT bool MCChunkRepeatForEachChunk(void*& x_iterator, MCStringRef& r_iterand, MCStringRef p_string)
{
    MCRange *t_range;
    bool t_first;
    t_first = false;
    
    if ((uintptr_t)x_iterator == 0)
    {
        t_first = true;
        MCMemoryAllocate(sizeof(MCRange), t_range);
        t_range -> offset = 0;
        t_range -> length = 0;
    }
    else
        t_range = (MCRange *)x_iterator;
    
    if (MCChunkIterate(*t_range, p_string, MCChunkGetChunkDelimiter(), kMCStringOptionCompareCaseless, t_first))
        MCStringCopySubstring(p_string, *t_range, r_iterand);
    else
    {
        MCMemoryDeallocate(t_range);
        return false;
    }
    
    x_iterator = (void *)(t_range);
    
    return true;
}