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

static compare_t MCSortCompareText(void *context, MCValueRef p_left, MCValueRef p_right)
{
    // Since there are no default type conversions, sort all strings before anything else.
    MCStringOptions t_options = *(MCStringOptions *)context;
    
    bool t_left_string, t_right_string;
    t_left_string = MCValueGetTypeCode(p_left) == kMCValueTypeCodeString;
    t_right_string = MCValueGetTypeCode(p_right) == kMCValueTypeCodeString;
    
    if (t_left_string)
    {
        if (!t_right_string)
            return 0;

        return MCStringCompareTo((MCStringRef)p_left, (MCStringRef)p_right, t_options);
    }
    else
    {
        if (t_right_string)
            return 1;
        
        return 0;
    }
}

static compare_t MCSortCompareBinary(void *context, MCValueRef p_left, MCValueRef p_right)
{
    // Since there are no default type conversions, sort all data before anything else.
    bool t_left_data, t_right_data;
    t_left_data = MCValueGetTypeCode(p_left) == kMCValueTypeCodeData;
    t_right_data = MCValueGetTypeCode(p_right) == kMCValueTypeCodeData;
    
    if (t_left_data)
    {
        if (!t_right_data)
            return 0;
        
        return MCDataCompareTo((MCDataRef)p_left, (MCDataRef)p_right);
    }
    else
    {
        if (t_right_data)
            return 1;
        
        return 0;
    }
}

static compare_t MCSortCompareNumeric(void *context, MCValueRef p_left, MCValueRef p_right)
{
    // Since there are no default type conversions, sort all numbers before anything else.
    bool t_left_data, t_right_data;
    t_left_data = MCValueGetTypeCode(p_left) == kMCValueTypeCodeNumber;
    t_right_data = MCValueGetTypeCode(p_right) == kMCValueTypeCodeNumber;
    
    if (t_left_data)
    {
        if (!t_right_data)
            return 0;
        
        return (MCNumberFetchAsReal((MCNumberRef)p_left) < MCNumberFetchAsReal((MCNumberRef)p_right)) ? -1 : 1;
    }
    else
    {
        if (t_right_data)
            return 1;
        
        return 0;
    }
}

static compare_t MCSortCompareDateTime(void *context, MCValueRef p_left, MCValueRef p_right)
{
    // Date time objects not yet implemented
    return 0;
}

extern "C" void MCSortExecSortListAscendingText(MCProperListRef& x_target)
{
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    // For now, just compare caseless.
    MCStringOptions t_option;
    t_option = kMCStringOptionCompareCaseless;
    MCProperListStableSort(*t_mutable_list, false, MCSortCompareText, &t_option);
    
    MCAutoProperListRef t_sorted_list;
    if (!MCProperListCopy(*t_mutable_list, &t_sorted_list))
        return;
    
    MCValueAssign(x_target, *t_sorted_list);
}

extern "C" void MCSortExecSortListDescendingText(MCProperListRef& x_target)
{
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    // For now, just compare caseless.
    MCStringOptions t_option;
    t_option = kMCStringOptionCompareCaseless;
    MCProperListStableSort(*t_mutable_list, true, MCSortCompareText, &t_option);
    
    MCAutoProperListRef t_sorted_list;
    if (!MCProperListCopy(*t_mutable_list, &t_sorted_list))
        return;
    
    MCValueAssign(x_target, *t_sorted_list);
}

extern "C" void MCSortExecSortListAscendingBinary(MCProperListRef& x_target)
{
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListStableSort(*t_mutable_list, false, MCSortCompareBinary, nil);
    
    MCAutoProperListRef t_sorted_list;
    if (!MCProperListCopy(*t_mutable_list, &t_sorted_list))
        return;
    
    MCValueAssign(x_target, *t_sorted_list);
}

extern "C" void MCSortExecSortListDescendingBinary(MCProperListRef& x_target)
{
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListStableSort(*t_mutable_list, true, MCSortCompareBinary, nil);
    
    MCAutoProperListRef t_sorted_list;
    if (!MCProperListCopy(*t_mutable_list, &t_sorted_list))
        return;
    
    MCValueAssign(x_target, *t_sorted_list);
}

extern "C" void MCSortExecSortListAscendingNumeric(MCProperListRef& x_target)
{
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListStableSort(*t_mutable_list, false, MCSortCompareNumeric, nil);
    
    MCAutoProperListRef t_sorted_list;
    if (!MCProperListCopy(*t_mutable_list, &t_sorted_list))
        return;
    
    MCValueAssign(x_target, *t_sorted_list);
}

extern "C" void MCSortExecSortListDescendingNumeric(MCProperListRef& x_target)
{
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListStableSort(*t_mutable_list, true, MCSortCompareNumeric, nil);
    
    MCAutoProperListRef t_sorted_list;
    if (!MCProperListCopy(*t_mutable_list, &t_sorted_list))
        return;
    
    MCValueAssign(x_target, *t_sorted_list);
}

void MCSortExecSortListAscendingDateTime(MCProperListRef& x_target)
{
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListStableSort(*t_mutable_list, false, MCSortCompareDateTime, nil);
    
    MCAutoProperListRef t_sorted_list;
    if (!MCProperListCopy(*t_mutable_list, &t_sorted_list))
        return;
    
    MCValueAssign(x_target, *t_sorted_list);
}

void MCSortExecSortListDescendingDateTime(MCProperListRef& x_target)
{
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListStableSort(*t_mutable_list, true, MCSortCompareDateTime, nil);
    
    MCAutoProperListRef t_sorted_list;
    if (!MCProperListCopy(*t_mutable_list, &t_sorted_list))
        return;
    
    MCValueAssign(x_target, *t_sorted_list);
}

/*

void MCSortExecSortStringListAscendingNumeric(MCProperListRef<MCStringRef>& x_target)
{
    MCProperListSort(x_target, true, kMCProperListSortTypeNumeric);
}
 
void MCSortExecSortStringListDescendingNumeric(MCProperListRef<MCStringRef>& x_target)
{
    MCProperListSort(x_target, false, kMCProperListSortTypeNumeric);
}
 
*/

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _TEST
extern void log(const char *module, const char *test, bool result);
#define log_result(test, result) log("SORT MODULE", test, result)
void MCSortRunTests()
{
/*
     void MCSortExecSortListAscendingText(MCProperListRef& x_target)
     void MCSortExecSortListDescendingText(MCProperListRef& x_target)
     void MCSortExecSortListAscendingBinary(MCProperListRef& x_target)
     void MCSortExecSortListDescendingBinary(MCProperListRef& x_target)
     void MCSortExecSortListAscendingNumeric(MCProperListRef& x_target)
     void MCSortExecSortListDescendingNumeric(MCProperListRef& x_target)
     void MCSortExecSortListAscendingDateTime(MCProperListRef& x_target)
     void MCSortExecSortListDescendingDateTime(MCProperListRef& x_target)
*/
    
    MCAutoNumberRef t_1n, t_2n;
    MCNumberCreateWithInteger(1, &t_1n);
    MCNumberCreateWithInteger(2, &t_2n);

    MCAutoStringRef t_string1, t_string2;
    MCStringCreateWithNativeChars((const char_t *)"a", 1, &t_string1);
    MCStringCreateWithNativeChars((const char_t *)"b", 1, &t_string2);
    
    byte_t t_bytes[2] = { 0x01, 0x02 };
    MCAutoDataRef t_data1, t_data2;
    MCDataCreateWithBytes((const byte_t *)t_bytes, 1, &t_data1);
    MCDataCreateWithBytes((const byte_t *)(t_bytes + 1), 1, &t_data2);

    MCAutoArray<MCValueRef> t_list_elts;
    t_list_elts . Push(*t_2n);
    t_list_elts . Push(*t_string2);
    t_list_elts . Push(*t_data2);
    t_list_elts . Push(*t_1n);
    t_list_elts . Push(*t_string1);
    t_list_elts . Push(*t_data1);

    MCProperListRef t_list;
    MCProperListCreateMutable(t_list);
    MCProperListPushElementsOntoBack(t_list, t_list_elts . Ptr(), t_list_elts . Size());
    MCSortExecSortListAscendingNumeric(t_list);
    // should be 1,2,b,0x02,a,0x01
    
    log_result("sort numeric", MCNumberFetchAsInteger((MCNumberRef)MCProperListFetchHead(t_list)) == 1);
    log_result("sort numeric stable", MCDataIsEqualTo((MCDataRef)MCProperListFetchTail(t_list), *t_data1));

    MCSortExecSortListAscendingText(t_list);
    // should be a,b,1,2,0x02,0x01
    
    log_result("sort text", MCStringIsEqualTo((MCStringRef)MCProperListFetchHead(t_list), *t_string1, kMCStringOptionCompareCaseless));
    log_result("sort text stable", MCDataIsEqualTo((MCDataRef)MCProperListFetchTail(t_list), *t_data1));
    
    MCSortExecSortListAscendingBinary(t_list);
    // should be 0x01,0x02,a,b,1,2
    
    log_result("sort binary", MCDataIsEqualTo((MCDataRef)MCProperListFetchHead(t_list), *t_data1));
    log_result("sort binary stable", MCNumberFetchAsInteger((MCNumberRef)MCProperListFetchTail(t_list)) == 2);
    
    MCValueRelease(t_list);
}
#endif
