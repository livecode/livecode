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
#include <foundation-chunk.h>

void MCByteEvalNumberOfBytesIn(MCDataRef p_source, uindex_t& r_output)
{
    r_output = MCDataGetLength(p_source);
}

void MCByteEvalIsAmongTheBytesOf(MCDataRef p_needle, MCDataRef p_target, bool p_is_not, bool& r_output)
{
    // Error if there is more than one byte.
    if (MCDataGetLength(p_needle) != 1)
        return;
    
    bool t_found = MCDataContains(p_target, p_needle);
    
    if (p_is_not)
        t_found = !t_found;
    
    r_output = t_found;
}

void MCByteEvalContainsBytes(MCDataRef p_target, MCDataRef p_needle, bool& r_output)
{
    r_output = MCDataContains(p_target, p_needle);
}
 
void MCByteEvalBeginsWithBytes(MCDataRef p_target, MCDataRef p_needle, bool& r_output)
{
    r_output = MCDataBeginsWith(p_target, p_needle);
}

void MCByteEvalEndsWithBytes(MCDataRef p_target, MCDataRef p_needle, bool& r_output)
{
    r_output = MCDataEndsWith(p_target, p_needle);
}

void MCByteEvalOffsetOfBytesInRange(MCDataRef p_needle, MCDataRef p_target, bool p_is_last, MCRange p_range, uindex_t& r_output)
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

void MCByteEvalOffsetOfBytes(MCDataRef p_needle, MCDataRef p_target, bool p_is_last, uindex_t& r_output)
{
    return MCByteEvalOffsetOfBytesInRange(p_needle, p_target, p_is_last, MCRangeMake(0, UINDEX_MAX), r_output);
}

void MCByteEvalOffsetOfBytesAfter(MCDataRef p_needle, MCDataRef p_target, uindex_t p_after, bool p_is_last, uindex_t& r_output)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfByteChunkByExpression(p_target, p_after, t_start, t_count);
    
    return MCByteEvalOffsetOfBytesInRange(p_needle, p_target, p_is_last, MCRangeMake(t_start + t_count, UINDEX_MAX), r_output);
}

void MCByteEvalOffsetOfBytesBefore(MCDataRef p_needle, MCDataRef p_target, uindex_t p_before, bool p_is_first, uindex_t& r_output)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfByteChunkByExpression(p_target, p_before, t_start, t_count);
    
    return MCByteEvalOffsetOfBytesInRange(p_needle, p_target, !p_is_first, MCRangeMake(0, t_start), r_output);
}

void MCByteFetchByteRangeOf(index_t p_start, index_t p_finish, MCDataRef p_target, MCDataRef& r_output)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfByteChunkByRange(p_target, p_start, p_finish, t_start, t_count);
    if (!MCDataCopyRange(p_target, MCRangeMake(t_start, t_count), r_output))
        return;
}

void MCByteStoreByteRangeOf(MCDataRef p_value, index_t p_start, index_t p_finish, MCDataRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfByteChunkByRange(x_target, p_start, p_finish, t_start, t_count);
    
    MCAutoDataRef t_data;
    if (!MCDataMutableCopy(x_target, &t_data))
        return;
    
    if (!MCDataReplace(*t_data, MCRangeMake(t_start, t_count), p_value))
        return;
    
    MCValueRelease(x_target);
    if (!MCDataCopy(*t_data, x_target))
        return;
}

void MCByteFetchByteOf(index_t p_index, MCDataRef p_target, MCDataRef& r_output)
{
    MCByteFetchByteRangeOf(p_index, p_index, p_target, r_output);
}

void MCByteStoreByteOf(MCDataRef p_value, index_t p_index, MCDataRef& x_target)
{
    MCByteStoreByteRangeOf(p_value, p_index, p_index, x_target);
}

void MCByteStoreAfterByteOf(MCDataRef p_value, index_t p_index, MCDataRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfByteChunkByExpression(x_target, p_index, t_start, t_count);
    
    MCAutoDataRef t_data;
    if (!MCDataMutableCopy(x_target, &t_data))
        return;
    
    if (!MCDataInsert(*t_data, t_start + t_count, p_value))
        return;
    
    MCValueRelease(x_target);
    if (!MCDataCopy(*t_data, x_target))
        return;
}

void MCByteStoreBeforeByteOf(MCDataRef p_value, index_t p_index, MCDataRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfByteChunkByRange(x_target, p_index, p_index, t_start, t_count);
    
    MCAutoDataRef t_data;
    if (!MCDataMutableCopy(x_target, &t_data))
        return;
    
    if (!MCDataInsert(*t_data, t_start, p_value))
        return;
    
    MCValueRelease(x_target);
    if (!MCDataCopy(*t_data, x_target))
        return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

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
