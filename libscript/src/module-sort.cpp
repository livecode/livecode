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

static compare_t MCSortCompareText(void *context, MCValueRef p_left, MCValueRef p_right)
{
    // Since there are no default type conversions, sort all strings before anything else.
    MCStringOptions t_options = *(MCStringOptions *)context;
    
    return MCStringCompareTo((MCStringRef)p_left, (MCStringRef)p_right, t_options);
}

static compare_t MCSortCompareBinary(void *context, MCValueRef p_left, MCValueRef p_right)
{
    return MCDataCompareTo((MCDataRef)p_left, (MCDataRef)p_right);
}

static compare_t MCSortCompareNumeric(void *context, MCValueRef p_left, MCValueRef p_right)
{
    return (MCNumberFetchAsReal((MCNumberRef)p_left) < MCNumberFetchAsReal((MCNumberRef)p_right)) ? -1 : 1;
}

static compare_t MCSortCompareDateTime(void *context, MCValueRef p_left, MCValueRef p_right)
{
    // Date time objects not yet implemented
    return 0;
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortList(MCProperListRef& x_target, bool p_descending)
{
    MCValueTypeCode t_type;
    if (!MCProperListIsHomogeneous(x_target, t_type))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("list elements are not all of the same type"), nil);
        return;
    }
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    switch (t_type)
    {
        case kMCValueTypeCodeString:
            // AL-2015-02-13: [[ Bug 14599 ]] Use exact comparison here for consistency.
            MCStringOptions t_option;
            t_option = kMCStringOptionCompareExact;
            MCProperListStableSort(*t_mutable_list, p_descending, MCSortCompareText, &t_option);
            break;
        case kMCValueTypeCodeData:
            MCProperListStableSort(*t_mutable_list, p_descending, MCSortCompareBinary, nil);
            break;
        case kMCValueTypeCodeNumber:
            MCProperListStableSort(*t_mutable_list, p_descending, MCSortCompareNumeric, nil);
            break;
        default:
            MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("list type does not have default comparison operator"), nil);
            return;
    }
    
    MCAutoProperListRef t_sorted_list;
    if (!MCProperListCopy(*t_mutable_list, &t_sorted_list))
        return;
    
    MCValueAssign(x_target, *t_sorted_list);
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListAscending(MCProperListRef& x_target)
{
    MCSortExecSortList(x_target, false);
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListDescending(MCProperListRef& x_target)
{
    MCSortExecSortList(x_target, true);
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListText(MCProperListRef& x_target, bool p_descending)
{
    if (!MCProperListIsListOfType(x_target, kMCValueTypeCodeString))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("list contains non-string element"), nil);
        return;
    }
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    // AL-2015-02-13: [[ Bug 14599 ]] Use exact comparison here for consistency.
    MCStringOptions t_option;
    t_option = kMCStringOptionCompareExact;
    MCProperListStableSort(*t_mutable_list, p_descending, MCSortCompareText, &t_option);
    
    MCAutoProperListRef t_sorted_list;
    if (!MCProperListCopy(*t_mutable_list, &t_sorted_list))
        return;
    
    MCValueAssign(x_target, *t_sorted_list);
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListAscendingText(MCProperListRef& x_target)
{
    MCSortExecSortListText(x_target, false);
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListDescendingText(MCProperListRef& x_target)
{
    MCSortExecSortListText(x_target, true);
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListBinary(MCProperListRef& x_target, bool p_descending)
{
    if (!MCProperListIsListOfType(x_target, kMCValueTypeCodeData))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("list contains non-data element"), nil);
        return;
    }
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListStableSort(*t_mutable_list, p_descending, MCSortCompareBinary, nil);
    
    MCAutoProperListRef t_sorted_list;
    if (!MCProperListCopy(*t_mutable_list, &t_sorted_list))
        return;
    
    MCValueAssign(x_target, *t_sorted_list);
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListAscendingBinary(MCProperListRef& x_target)
{
    MCSortExecSortListBinary(x_target, false);
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListDescendingBinary(MCProperListRef& x_target)
{
    MCSortExecSortListBinary(x_target, true);
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListNumeric(MCProperListRef& x_target, bool p_descending)
{
    if (!MCProperListIsListOfType(x_target, kMCValueTypeCodeNumber))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("list contains non-numeric element"), nil);
        return;
    }
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListStableSort(*t_mutable_list, p_descending, MCSortCompareNumeric, nil);
    
    MCAutoProperListRef t_sorted_list;
    if (!MCProperListCopy(*t_mutable_list, &t_sorted_list))
        return;
    
    MCValueAssign(x_target, *t_sorted_list);
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListAscendingNumeric(MCProperListRef& x_target)
{
    MCSortExecSortListNumeric(x_target, false);
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListDescendingNumeric(MCProperListRef& x_target)
{
    MCSortExecSortListNumeric(x_target, true);
}

static void MCSortExecSortListDateTime(MCProperListRef& x_target, bool p_descending)
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

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListAscendingDateTime(MCProperListRef& x_target)
{
    MCSortExecSortListDateTime(x_target, false);
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListDescendingDateTime(MCProperListRef& x_target)
{
   MCSortExecSortListDateTime(x_target, true);
}

////////////////////////////////////////////////////////////////

static compare_t MCSortCompareUsingHandler(void *context, MCValueRef p_left, MCValueRef p_right)
{
    MCHandlerRef t_handler;
    t_handler = *(MCHandlerRef *)context;
    
    MCAutoValueRefArray t_values;
    t_values.Push(p_left);
    t_values.Push(p_right);
    
    MCAutoValueRef t_result;
    MCHandlerInvoke(t_handler, t_values . Ptr(), t_values . Size(), &t_result);
    
    if (*t_result != nil && MCValueGetTypeCode(*t_result) == kMCValueTypeCodeNumber)
        return MCNumberFetchAsInteger((MCNumberRef)*t_result);
    
    return 0;
}

extern "C" MC_DLLEXPORT_DEF void MCSortExecSortListUsingHandler(MCProperListRef& x_target, MCHandlerRef p_handler)
{
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListStableSort(*t_mutable_list, false, MCSortCompareUsingHandler, &p_handler);
    
    MCAutoProperListRef t_sorted_list;
    if (!MCProperListCopy(*t_mutable_list, &t_sorted_list))
        return;
    
    MCValueAssign(x_target, *t_sorted_list);
}

////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_sort_Initialize (void)
{
	return true;
}

extern "C" void com_livecode_sort_Finalize (void)
{
}

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
