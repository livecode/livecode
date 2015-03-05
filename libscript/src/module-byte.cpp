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
#include <foundation-system.h>
#include <foundation-auto.h>
#include <foundation-chunk.h>

extern "C" MC_DLLEXPORT void MCByteEvalNumberOfBytesIn(MCDataRef p_source, uindex_t& r_output)
{
    r_output = MCDataGetLength(p_source);
}

extern "C" MC_DLLEXPORT void MCByteEvalIsAmongTheBytesOf(MCDataRef p_needle, MCDataRef p_target, bool p_is_not, bool& r_output)
{
    // Error if there is more than one byte.
    if (MCDataGetLength(p_needle) != 1)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("needle must be a single byte"), nil);
        return;
    }
    
    bool t_found = MCDataContains(p_target, p_needle);
    
    if (p_is_not)
        t_found = !t_found;
    
    r_output = t_found;
}

extern "C" MC_DLLEXPORT void MCByteEvalContainsBytes(MCDataRef p_target, MCDataRef p_needle, bool& r_output)
{
    r_output = MCDataContains(p_target, p_needle);
}
 
extern "C" MC_DLLEXPORT void MCByteEvalBeginsWithBytes(MCDataRef p_target, MCDataRef p_needle, bool& r_output)
{
    r_output = MCDataBeginsWith(p_target, p_needle);
}

extern "C" MC_DLLEXPORT void MCByteEvalEndsWithBytes(MCDataRef p_target, MCDataRef p_needle, bool& r_output)
{
    r_output = MCDataEndsWith(p_target, p_needle);
}

extern "C" MC_DLLEXPORT void MCByteEvalOffsetOfBytesInRange(MCDataRef p_needle, MCDataRef p_target, bool p_is_last, MCRange p_range, uindex_t& r_output)
{
    // Incoming range must be 0-based.
    uindex_t t_offset;
    t_offset = 0;
    
    bool t_found;
    
    if (!MCDataIsEmpty(p_needle))
    {
        if (!p_is_last)
            t_found = MCDataFirstIndexOf(p_target, p_needle, p_range, t_offset);
        else
            t_found = MCDataLastIndexOf(p_target, p_needle, p_range, t_offset);
        
        if (t_found)
            t_offset++;
    }
    
    r_output = t_offset;
}

extern "C" MC_DLLEXPORT void MCByteEvalOffsetOfBytes(MCDataRef p_needle, MCDataRef p_target, bool p_is_last, uindex_t& r_output)
{
    return MCByteEvalOffsetOfBytesInRange(p_needle, p_target, p_is_last, MCRangeMake(0, UINDEX_MAX), r_output);
}

extern "C" MC_DLLEXPORT void MCByteEvalOffsetOfBytesAfter(MCDataRef p_needle, MCDataRef p_target, uindex_t p_after, bool p_is_last, uindex_t& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfByteChunkByExpressionInRange(p_target, nil, p_after, true, true, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    return MCByteEvalOffsetOfBytesInRange(p_needle, p_target, p_is_last, MCRangeMake(t_start + t_count, UINDEX_MAX), r_output);
}

extern "C" MC_DLLEXPORT void MCByteEvalOffsetOfBytesBefore(MCDataRef p_needle, MCDataRef p_target, uindex_t p_before, bool p_is_first, uindex_t& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfByteChunkByExpressionInRange(p_target, nil, p_before, true, false, true, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    return MCByteEvalOffsetOfBytesInRange(p_needle, p_target, !p_is_first, MCRangeMake(0, t_start), r_output);
}

extern "C" MC_DLLEXPORT void MCByteFetchByteRangeOf(index_t p_start, index_t p_finish, MCDataRef p_target, MCDataRef& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfByteChunkByRangeInRange(p_target, nil, p_start, p_finish, true, false, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    if (!MCDataCopyRange(p_target, MCRangeMake(t_start, t_count), r_output))
        return;
}

extern "C" MC_DLLEXPORT void MCByteStoreByteRangeOf(MCDataRef p_value, index_t p_start, index_t p_finish, MCDataRef& x_target)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfByteChunkByRangeInRange(x_target, nil, p_start, p_finish, true, false, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCAutoDataRef t_data;
    if (!MCDataMutableCopy(x_target, &t_data))
        return;
    
    if (!MCDataReplace(*t_data, MCRangeMake(t_start, t_count), p_value))
        return;
    
    MCAutoDataRef t_new_data;
    if (!MCDataCopy(*t_data, &t_new_data))
        return;
    
    MCValueAssign(x_target, *t_new_data);
}

extern "C" MC_DLLEXPORT void MCByteFetchByteOf(index_t p_index, MCDataRef p_target, MCDataRef& r_output)
{
    MCByteFetchByteRangeOf(p_index, p_index, p_target, r_output);
}

extern "C" MC_DLLEXPORT void MCByteStoreByteOf(MCDataRef p_value, index_t p_index, MCDataRef& x_target)
{
    MCByteStoreByteRangeOf(p_value, p_index, p_index, x_target);
}

extern "C" MC_DLLEXPORT void MCByteFetchFirstByteOf(MCDataRef p_target, MCDataRef& r_output)
{
    MCByteFetchByteOf(1, p_target, r_output);
}

extern "C" MC_DLLEXPORT void MCByteStoreFirstByteOf(MCDataRef p_value, MCDataRef& x_target)
{
    MCByteStoreByteOf(p_value, 1, x_target);
}

extern "C" MC_DLLEXPORT void MCByteFetchLastByteOf(MCDataRef p_target, MCDataRef& r_output)
{
    MCByteFetchByteOf(-1, p_target, r_output);
}

extern "C" MC_DLLEXPORT void MCByteStoreLastByteOf(MCDataRef p_value, MCDataRef& x_target)
{
    MCByteStoreByteOf(p_value, -1, x_target);
}

extern "C" MC_DLLEXPORT void MCByteExecDeleteByteRangeOf(index_t p_start, index_t p_finish, MCDataRef& x_target)
{
    MCByteStoreByteRangeOf(kMCEmptyData, p_start, p_finish, x_target);
}

extern "C" MC_DLLEXPORT void MCByteExecDeleteByteOf(index_t p_index, MCDataRef& x_target)
{
    MCByteStoreByteOf(kMCEmptyData, p_index, x_target);
}

extern "C" MC_DLLEXPORT void MCByteExecDeleteFirstByteOf(MCDataRef& x_target)
{
    MCByteExecDeleteByteOf(1, x_target);
}

extern "C" MC_DLLEXPORT void MCByteExecDeleteLastByteOf(MCDataRef& x_target)
{
    MCByteExecDeleteByteOf(-1, x_target);
}

extern "C" MC_DLLEXPORT bool MCByteRepeatForEachByte(void*& x_iterator, MCDataRef& r_iterand, MCDataRef p_data)
{
    uintptr_t t_offset;
    t_offset = (uintptr_t)x_iterator;
    
    if (t_offset == MCDataGetLength(p_data))
        return false;
    
    if (!MCDataCopyRange(p_data, MCRangeMake(t_offset, 1), r_iterand))
        return false;
    
    x_iterator = (void *)(t_offset + 1);
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void
MCDataExecRandomBytes (uindex_t p_count, MCDataRef & r_data)
{
	/* UNCHECKED */ MCSRandomData (p_count, r_data);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _TEST
extern void log(const char *module, const char *test, bool result);
#define log_result(test, result) log("BYTE MODULE", test, result)
void MCByteRunTests()
{
    byte_t bytes[8] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7 };
    
    MCAutoDataRef t_data;
    MCDataCreateWithBytes(bytes, 8, &t_data);

// void MCByteEvalNumberOfBytesIn(MCDataRef p_source, uindex_t& r_output)
    uindex_t t_num;
    MCByteEvalNumberOfBytesIn(*t_data, t_num);
    
    log_result("number of bytes", t_num == 8);
    
// void MCByteEvalIsAmongTheBytesOf(MCDataRef p_needle, MCDataRef p_target, bool p_is_not, bool& r_output)
    
    MCAutoDataRef t_byte;
    MCDataCreateWithBytes(bytes + 5, 1, &t_byte);
    bool t_result;
    MCByteEvalIsAmongTheBytesOf(*t_byte, *t_data, false, t_result);
    
    log_result("is among the bytes", t_result);
    
    MCByteEvalIsAmongTheBytesOf(kMCEmptyData, *t_data, true, t_result);
    
    MCAutoDataRef t_other_byte;
    byte_t byte = 0x08;
    MCDataCreateWithBytes(&byte, 1, &t_other_byte);
    
    log_result("is not among the bytes", t_result);
    
// void MCByteEvalContainsBytes(MCDataRef p_target, MCDataRef p_needle, bool& r_output)
    
    MCByteEvalContainsBytes(*t_data, kMCEmptyData, t_result);
    
    log_result("data contains empty", t_result);
    
    MCAutoDataRef t_sub_data;
    MCDataCopyRange(*t_data, MCRangeMake(2, 5), &t_sub_data);
    MCByteEvalContainsBytes(*t_data, *t_sub_data, t_result);
    
    log_result("data contains subdata", t_result);
    
    MCByteEvalContainsBytes(*t_data, *t_other_byte, t_result);
    t_result = !t_result;
    
    log_result("data does not contain other", t_result);
    
    MCAutoDataRef t_begin, t_end;
    
    MCDataCopyRange(*t_data, MCRangeMake(0, 4), &t_begin);
    MCDataCopyRange(*t_data, MCRangeMake(4, 4), &t_end);

//    void MCByteEvalBeginsWithBytes(MCDataRef p_target, MCDataRef p_needle, bool& r_output)
    
    MCByteEvalBeginsWithBytes(*t_data, *t_begin, t_result);
    
    log_result("begins with", t_result);
    
    MCByteEvalBeginsWithBytes(*t_data, *t_end, t_result);
    t_result = !t_result;
    
    log_result("does not begin with", t_result);
    
//    void MCByteEvalEndsWithBytes(MCDataRef p_target, MCDataRef p_needle, bool& r_output)
    MCByteEvalEndsWithBytes(*t_data, *t_end, t_result);
    
    log_result("ends with", t_result);
    
    MCByteEvalEndsWithBytes(*t_data, *t_begin, t_result);
    t_result = !t_result;
    
    log_result("does not end with", t_result);
    
//    void MCByteEvalOffsetOfBytesInRange(MCDataRef p_needle, MCDataRef p_target, bool p_is_last, MCRange p_range, uindex_t& r_output)
    
    MCAutoDataRef t_quadruple;
    MCDataMutableCopy(*t_begin, &t_quadruple);
    MCDataAppend(*t_quadruple, *t_quadruple);
    MCDataAppend(*t_quadruple, *t_quadruple);
    
    uindex_t t_offset;
    MCByteEvalOffsetOfBytesAfter(*t_begin, *t_quadruple, 6, false, t_offset);
    
    log_result("first offset of bytes after", t_offset == 3);
    
    MCByteEvalOffsetOfBytesAfter(*t_begin, *t_quadruple, 0, true, t_offset);
    
    log_result("last offset of bytes after", t_offset == 13);
    
    MCByteEvalOffsetOfBytesAfter(*t_begin, *t_quadruple, 13, true, t_offset);
    
    log_result("last offset of bytes after no occurrences", t_offset == 0);
    
    MCByteEvalOffsetOfBytesBefore(*t_begin, *t_quadruple, 11, false, t_offset);
    
    log_result("last offset of bytes before", t_offset == 5);
    
    MCByteEvalOffsetOfBytesBefore(*t_begin, *t_quadruple, 17, true, t_offset);
    
    log_result("first offset of bytes before", t_offset == 1);
    
    MCByteEvalOffsetOfBytesBefore(*t_begin, *t_quadruple, 4, true, t_offset);
    
    log_result("first offset of bytes before no occurrences", t_offset == 0);
}
#endif

