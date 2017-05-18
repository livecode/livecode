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

extern "C" MC_DLLEXPORT_DEF void MCListEvalHeadOf(MCProperListRef p_target, MCValueRef& r_output)
{
	if (MCProperListIsEmpty (p_target))
	{
		MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
		return;
	}

    r_output = MCValueRetain(MCProperListFetchHead(p_target));
}

extern "C" MC_DLLEXPORT_DEF void MCListEvalTailOf(MCProperListRef p_target, MCValueRef& r_output)
{
	if (MCProperListIsEmpty (p_target))
	{
		MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
		return;
	}

    r_output = MCValueRetain(MCProperListFetchTail(p_target));
}

extern "C" MC_DLLEXPORT_DEF void MCListExecPushSingleElementOnto(MCValueRef p_value, bool p_is_front, MCProperListRef& x_target)
{
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
 
    MCValueRef t_value;
    t_value = p_value != nil ? p_value : kMCNull;
    
    if (p_is_front)
    {
        if (!MCProperListPushElementOntoFront(*t_mutable_list, t_value))
            return;
    }
    else
    {
        if (!MCProperListPushElementOntoBack(*t_mutable_list, t_value))
            return;
    }
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
}

extern "C" MC_DLLEXPORT_DEF MCValueRef MCListExecPopElement(bool p_is_front, MCProperListRef& x_source)
{
    MCAutoProperListRef t_mutable_list;
    MCAutoValueRef t_result;

    if (MCProperListIsEmpty (x_source))
	{
		MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("pop from empty list"), nil);
		return nil;
	}

    if (!MCProperListMutableCopy(x_source, &t_mutable_list))
        return NULL;
    
    if (p_is_front)
    {
	    if (!MCProperListPopFront(*t_mutable_list, &t_result))
            return NULL;
    }
    else
    {
        if (!MCProperListPopBack(*t_mutable_list, &t_result))
            return NULL;
    }
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return NULL;
    
    MCValueAssign(x_source, *t_immutable);

    return t_result.Take();
}

extern "C" MC_DLLEXPORT_DEF void MCListEvalNumberOfElementsIn(MCProperListRef p_target, uindex_t& r_output)
{
    r_output = MCProperListGetLength(p_target);
}

extern "C" MC_DLLEXPORT_DEF void MCListEvalIsAmongTheElementsOf(MCValueRef p_needle, MCProperListRef p_target, bool& r_output)
{
    MCValueRef t_value;
    t_value = p_needle != nil ? p_needle : kMCNull;
    
    uindex_t t_dummy;
    r_output = MCProperListFirstIndexOfElement(p_target, t_value, 0, t_dummy);
}

extern "C" MC_DLLEXPORT_DEF void MCListEvalContainsElements(MCProperListRef p_target, MCProperListRef p_needle, bool& r_output)
{
    uindex_t t_dummy;
    r_output = MCProperListFirstOffsetOfList(p_target, p_needle, 0, t_dummy);
}

extern "C" MC_DLLEXPORT_DEF void MCListFetchElementOf(index_t p_index, MCProperListRef p_target, MCValueRef& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfElementChunkByExpressionInRange(p_target, nil, p_index, true, false, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    r_output = MCValueRetain(MCProperListFetchElementAtIndex(p_target, t_start));
}

extern "C" MC_DLLEXPORT_DEF void MCListStoreElementOf(MCValueRef p_value, index_t p_index, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfElementChunkByExpressionInRange(x_target, nil, p_index, true, false, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCValueRef t_value;
    t_value = p_value != nil ? p_value : kMCNull;
    
    MCProperListRemoveElements(*t_mutable_list, t_start, t_count);
    MCProperListInsertElement(*t_mutable_list, t_value, t_start);
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
}

extern "C" MC_DLLEXPORT_DEF void MCListFetchElementRangeOf(index_t p_start, index_t p_finish, MCProperListRef p_target, MCProperListRef& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfElementChunkByRangeInRange(p_target, nil, p_start, p_finish, true, false, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCProperListCopySublist(p_target, MCRangeMake(t_start, t_count), r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCListStoreElementRangeOf(MCValueRef p_value, index_t p_start, index_t p_finish, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfElementChunkByRangeInRange(x_target, nil, p_start, p_finish, true, false, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCValueRef t_value;
    t_value = p_value != nil ? p_value : kMCNull;
    
    MCProperListRemoveElements(*t_mutable_list, t_start, t_count);
    MCProperListInsertElement(*t_mutable_list, t_value, t_start);
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
}

extern "C" MC_DLLEXPORT_DEF void MCListFetchIndexOf(MCProperListRef p_target, index_t p_index, MCValueRef& r_output)
{
    MCListFetchElementOf(p_index, p_target, r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCListStoreIndexOf(MCValueRef p_value, MCProperListRef& x_target, index_t p_index)
{
    MCValueRef t_value;
    t_value = p_value != nil ? p_value : kMCNull;
    MCListStoreElementOf(t_value, p_index, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCListStoreAfterElementOf(MCValueRef p_value, index_t p_index, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfElementChunkByExpressionInRange(x_target, nil, p_index, true, true, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    t_start += t_count;
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCValueRef t_value;
    t_value = p_value != nil ? p_value : kMCNull;
    
    MCProperListInsertElement(*t_mutable_list, t_value, t_start);
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
}

extern "C" MC_DLLEXPORT_DEF void MCListStoreBeforeElementOf(MCValueRef p_value, index_t p_index, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfElementChunkByExpressionInRange(x_target, nil, p_index, true, false, true, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCValueRef t_value;
    t_value = p_value != nil ? p_value : kMCNull;
    
    MCProperListInsertElement(*t_mutable_list, t_value, t_start);
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
}

extern "C" MC_DLLEXPORT_DEF void MCListFetchFirstElementOf(MCProperListRef p_target, MCValueRef& r_output)
{
    MCListFetchElementOf(1, p_target, r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCListStoreFirstElementOf(MCValueRef p_value, MCProperListRef& x_target)
{
    MCValueRef t_value;
    t_value = p_value != nil ? p_value : kMCNull;
    MCListStoreElementOf(t_value, 1, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCListFetchLastElementOf(MCProperListRef p_target, MCValueRef& r_output)
{
    MCListFetchElementOf(-1, p_target, r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCListStoreLastElementOf(MCValueRef p_value, MCProperListRef& x_target)
{
    MCValueRef t_value;
    t_value = p_value != nil ? p_value : kMCNull;
    MCListStoreElementOf(t_value, -1, x_target);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void MCListSpliceIntoElementRangeOf(MCProperListRef p_list, index_t p_start, index_t p_finish, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfElementChunkByRangeInRange(x_target, nil, p_start, p_finish, true, false, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListRemoveElements(*t_mutable_list, t_start, t_count);
    MCProperListInsertList(*t_mutable_list, p_list, t_start);
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
}

extern "C" MC_DLLEXPORT_DEF void MCListSpliceIntoElementOf(MCProperListRef p_list, index_t p_index, MCProperListRef& x_target)
{
    MCListSpliceIntoElementRangeOf(p_list, p_index, p_index, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCListSpliceBeforeElementOf(MCProperListRef p_list, index_t p_index, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfElementChunkByExpressionInRange(x_target, nil, p_index, true, false, true, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;

    MCProperListInsertList(*t_mutable_list, p_list, t_start);
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
}

extern "C" MC_DLLEXPORT_DEF void MCListSpliceAfterElementOf(MCProperListRef p_list, index_t p_index, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfElementChunkByExpressionInRange(x_target, nil, p_index, true, true, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    t_start += t_count;
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListInsertList(*t_mutable_list, p_list, t_start);
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
}

extern "C" MC_DLLEXPORT_DEF void MCListSpliceBefore(MCProperListRef p_list, MCProperListRef& x_target)
{
    MCListSpliceBeforeElementOf(p_list, 1, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCListSpliceAfter(MCProperListRef p_list, MCProperListRef& x_target)
{
    MCListSpliceAfterElementOf(p_list, -1, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCListSpliceIntoFirstElementOf(MCProperListRef p_list, MCProperListRef& x_target)
{
    MCListSpliceIntoElementOf(p_list, 1, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCListSpliceIntoLastElementOf(MCProperListRef p_list, MCProperListRef& x_target)
{
    MCListSpliceIntoElementOf(p_list, -1, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCListExecDeleteElementRangeOf(index_t p_start, index_t p_finish, MCProperListRef& x_target)
{
    MCListSpliceIntoElementRangeOf(kMCEmptyProperList, p_start, p_finish, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCListExecDeleteElementOf(index_t p_index, MCProperListRef& x_target)
{
    MCListSpliceIntoElementOf(kMCEmptyProperList, p_index, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCListExecDeleteFirstElementOf(MCProperListRef& x_target)
{
    MCListExecDeleteElementOf(1, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCListExecDeleteLastElementOf(MCProperListRef& x_target)
{
    MCListExecDeleteElementOf(-1, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCListEvalEmpty(MCProperListRef& r_output)
{
    r_output = MCValueRetain(kMCEmptyProperList);
}

extern "C" MC_DLLEXPORT_DEF bool MCListRepeatForEachElement(void*& x_iterator, MCValueRef& r_iterand, MCProperListRef p_list)
{
    uintptr_t t_offset;
    t_offset = (uintptr_t)x_iterator;
    
    if (t_offset == MCProperListGetLength(p_list))
        return false;
    
    r_iterand = MCValueRetain(MCProperListFetchElementAtIndex(p_list, t_offset));
    
    x_iterator = (void *)(t_offset + 1);
    
    return true;
}

extern "C" MC_DLLEXPORT_DEF void MCListEvalBeginsWith(MCProperListRef p_list, MCProperListRef p_prefix, bool& r_output)
{
    r_output = MCProperListBeginsWithList(p_list, p_prefix);
}

extern "C" MC_DLLEXPORT_DEF void MCListEvalEndsWith(MCProperListRef p_list, MCProperListRef p_suffix, bool& r_output)
{
    r_output = MCProperListEndsWithList(p_list, p_suffix);
}

extern "C" MC_DLLEXPORT_DEF void MCListEvalIsEqualTo(MCProperListRef p_left, MCProperListRef p_right, bool& r_output)
{
    r_output = MCProperListIsEqualTo(p_left, p_right);
}

extern "C" MC_DLLEXPORT_DEF void MCListEvalIsNotEqualTo(MCProperListRef p_left, MCProperListRef p_right, bool& r_output)
{
    r_output = !MCProperListIsEqualTo(p_left, p_right);
}

////////////////////////////////////////////////////////////////

static void
MCListEvalIndexOfElementInRange (bool p_is_last,
                                 MCValueRef p_needle,
                                 MCProperListRef p_haystack,
                                 MCRange p_range,
                                 uindex_t & r_output)
{
	if (MCProperListIsEmpty (p_haystack))
	{
		r_output = 0;
		return;
	}

	uindex_t t_offset = 0;
	bool t_found = false;
	if (!p_is_last)
		t_found = MCProperListFirstIndexOfElementInRange (p_haystack, p_needle,
		                                                  p_range, t_offset);
	else
		t_found = MCProperListLastIndexOfElementInRange (p_haystack, p_needle,
		                                                 p_range, t_offset);

	if (t_found)
		r_output = t_offset + p_range.offset + 1;
	else
		r_output = 0;
}

extern "C" MC_DLLEXPORT_DEF void
MCListEvalIndexOfElement (bool p_is_last,
                          MCValueRef p_needle,
                          MCProperListRef p_haystack,
                          uindex_t & r_output)
{
	MCRange t_range = MCRangeMake (0, UINDEX_MAX);
	MCListEvalIndexOfElementInRange (p_is_last, p_needle, p_haystack, t_range, r_output);
}

extern "C" MC_DLLEXPORT_DEF void
MCListEvalIndexOfElementAfter (bool p_is_last,
                               MCValueRef p_needle,
                               index_t p_after,
                               MCProperListRef p_haystack,
                               uindex_t & r_output)
{
	uindex_t t_start, t_count;
	if (!MCChunkGetExtentsOfElementChunkByExpressionInRange (p_haystack, nil,
	        p_after, true, true, false, t_start, t_count) &&
	    p_after != 0)
	{
		MCErrorCreateAndThrow (kMCGenericErrorTypeInfo, "reason",
		                       MCSTR("chunk index out of range"), nil);
		return;
	}

	MCListEvalIndexOfElementInRange (p_is_last, p_needle, p_haystack,
	                                 MCRangeMake(t_start + t_count, UINDEX_MAX),
	                                 r_output);
}

extern "C" MC_DLLEXPORT_DEF void
MCListEvalIndexOfElementBefore (bool p_is_first,
                               MCValueRef p_needle,
                               index_t p_before,
                               MCProperListRef p_haystack,
                               uindex_t & r_output)
{
	uindex_t t_start, t_count;
	if (p_before == 0)
	{
		t_start = UINDEX_MAX;
	} else if (!MCChunkGetExtentsOfElementChunkByExpressionInRange (p_haystack,
	                nil, p_before, true, false, true, t_start, t_count))
	{
		MCErrorCreateAndThrow (kMCGenericErrorTypeInfo, "reason",
		                       MCSTR("chunk index out of range"), nil);
		return;
	}

	MCListEvalIndexOfElementInRange (!p_is_first, p_needle, p_haystack,
	                                 MCRangeMake(0, t_start),
	                                 r_output);
}

////////////////////////////////////////////////////////////////

static void
MCListEvalOffsetOfListInRange (bool p_is_last,
                               MCProperListRef p_needle,
                               MCProperListRef p_haystack,
                               MCRange p_range,
                               uindex_t & r_output)
{
	if (MCProperListIsEmpty (p_haystack))
	{
		r_output = 0;
		return;
	}

	uindex_t t_offset = 0;
	bool t_found = false;
	if (!p_is_last)
		t_found = MCProperListFirstOffsetOfListInRange (p_haystack, p_needle,
		                                                p_range, t_offset);
	else
		t_found = MCProperListLastOffsetOfListInRange (p_haystack, p_needle,
		                                               p_range, t_offset);

	if (t_found)
		r_output = t_offset + p_range.offset + 1;
	else
		r_output = 0;
}

extern "C" MC_DLLEXPORT_DEF void
MCListEvalOffsetOfList (bool p_is_last,
                        MCProperListRef p_needle,
                        MCProperListRef p_haystack,
                        uindex_t & r_output)
{
	MCRange t_range = MCRangeMake (0, UINDEX_MAX);
	MCListEvalOffsetOfListInRange (p_is_last, p_needle, p_haystack, t_range, r_output);
}

extern "C" MC_DLLEXPORT_DEF void
MCListEvalOffsetOfListAfter (bool p_is_last,
                             MCProperListRef p_needle,
                             index_t p_after,
                             MCProperListRef p_haystack,
                             uindex_t & r_output)
{
	uindex_t t_start, t_count;
	if (!MCChunkGetExtentsOfElementChunkByExpressionInRange (p_haystack, nil,
	        p_after, true, true, false, t_start, t_count) &&
	    p_after != 0)
	{
		MCErrorCreateAndThrow (kMCGenericErrorTypeInfo, "reason",
		                       MCSTR("chunk index out of range"), nil);
		return;
	}

	MCListEvalOffsetOfListInRange (p_is_last, p_needle, p_haystack,
	                               MCRangeMake(t_start + t_count, UINDEX_MAX),
	                               r_output);
}

extern "C" MC_DLLEXPORT_DEF void
MCListEvalOffsetOfListBefore (bool p_is_first,
                              MCProperListRef p_needle,
                              index_t p_before,
                              MCProperListRef p_haystack,
                              uindex_t & r_output)
{
	uindex_t t_start, t_count;
	if (p_before == 0)
	{
		t_start = UINDEX_MAX;
	} else if (!MCChunkGetExtentsOfElementChunkByExpressionInRange (p_haystack,
	                nil, p_before, true, false, true, t_start, t_count))
	{
		MCErrorCreateAndThrow (kMCGenericErrorTypeInfo, "reason",
		                       MCSTR("chunk index out of range"), nil);
		return;
	}

	MCListEvalOffsetOfListInRange (!p_is_first, p_needle, p_haystack,
	                               MCRangeMake(0, t_start),
	                               r_output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void
MCListEvalConcatenate(MCProperListRef p_left,
                      MCProperListRef p_right,
                      MCProperListRef & r_result)
{
	MCAutoProperListRef t_result;
	if (!MCProperListMutableCopy(p_left, &t_result))
		return;
	if (!MCProperListAppendList(*t_result, p_right))
		return;
	r_result = t_result.Take();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void
MCListExecReverseElementsOf(MCProperListRef& x_list)
{
    MCAutoProperListRef t_list;
    if (!MCProperListMutableCopy(x_list, &t_list))
        return;
    if (!MCProperListReverse(*t_list))
        return;
    if (!t_list.MakeImmutable())
        return;
    MCValueAssign(x_list, *t_list);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_list_Initialize(void)
{
    return true;
}

extern "C" void com_livecode_list_Finalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _TEST
extern void log(const char *module, const char *test, bool result);
#define log_result(test, result) log("LIST MODULE", test, result)
void MCListRunTests()
{
    MCAutoProperListRef t_list;
    MCProperListCreateMutable(&t_list);
    
    MCProperListPushElementOntoBack(*t_list, kMCEmptyArray);
    MCProperListPushElementOntoBack(*t_list, kMCEmptyString);
    MCProperListPushElementOntoBack(*t_list, kMCEmptyName);
    MCProperListPushElementOntoBack(*t_list, kMCEmptyProperList);
    MCProperListPushElementOntoBack(*t_list, kMCEmptySet);
    
//  MCListEvalNumberOfElementsIn(MCProperListRef p_target, uindex_t& r_output)
    
    uindex_t t_num;
    MCListEvalNumberOfElementsIn(*t_list, t_num);
    log_result("number of elements in", t_num == 5);
    
    MCValueRef t_value;
    t_value = nil;
    
//  MCListEvalHeadOf(MCProperListRef p_target, MCValueRef& r_output)
    MCListEvalHeadOf(*t_list, t_value);
    
    log_result("head of", kMCEmptyArray == t_value);
 
//  MCListEvalTailOf(MCProperListRef p_target, MCValueRef& r_output)
    MCListEvalTailOf(*t_list, t_value);
    
    log_result("head of", kMCEmptySet == t_value);
    
//  MCListFetchElementOf(index_t p_index, MCProperListRef p_target, MCValueRef& r_output)
    MCListFetchElementOf(3, *t_list, t_value);

    MCAssert(t_value != nil);
    log_result("fetch element x of", MCValueIsEqualTo(t_value, kMCEmptyName));
    
    /*MCListFetchElementRangeOf(index_t p_start, index_t p_finish, MCProperListRef p_target, MCProperListRef& r_output)*/
    MCAutoProperListRef t_sublist;
    MCListFetchElementRangeOf(2, 3, *t_list, &t_sublist);
    
    MCAssert(*t_sublist != nil);
    log_result("fetch element range of", MCProperListGetLength(*t_sublist) == 2);
    
    /*MCListEvalContains(MCProperListRef p_target, MCProperListRef p_needle, bool& r_output)*/
    bool t_result;
    MCListEvalContains(*t_list, *t_sublist, t_result);
    log_result("contains", t_result);
    
    /*MCListEvalIsAmongTheElementsOf(MCValueRef p_needle, MCProperListRef p_target, bool& r_output)*/
    bool t_is_among;
    MCListEvalIsAmongTheElementsOf(kMCEmptyName, *t_list, t_is_among);

    log_result("is among elements of", t_is_among);
    
//  MCListExecPopElementInto(MCProperListRef& x_source, bool p_is_front, MCValueRef& r_output)
    MCAutoValueRef t_popped;
    MCProperListRef t_lst;
    MCProperListMutableCopy(*t_list, t_lst);
    MCListExecPopElementInto(t_lst, false, &t_popped);
    MCListEvalNumberOfElementsIn(t_lst, t_num);
    
    log_result("pop from back", *t_popped == kMCEmptySet && t_num == 4);
    
    MCAutoValueRef t_front_popped;
    MCListExecPopElementInto(t_lst, true, &t_front_popped);
    MCListEvalNumberOfElementsIn(t_lst, t_num);
    
    log_result("pop from front", *t_front_popped == kMCEmptyArray && t_num == 3);
    
//  void MCListStoreElementRangeOf(MCValueRef p_value, index_t p_start, index_t p_finish, MCProperListRef& x_target)
    
    MCListStoreElementRangeOf(*t_sublist, 1, -1, t_lst);
    MCListEvalNumberOfElementsIn(t_lst, t_num);
    
    log_result("store range of", t_num == 1 && MCValueGetTypeCode(MCProperListFetchHead(t_lst)) == kMCValueTypeCodeProperList);
    
//  void MCListSpliceIntoElementOf(MCProperListRef p_list, index_t p_index, MCProperListRef& x_target)
    
    MCListSpliceIntoElementOf(*t_sublist, 1, t_lst);
    MCListEvalNumberOfElementsIn(t_lst, t_num);
    MCListEvalIsAmongTheElementsOf(*t_sublist, t_lst, t_result);
    
    log_result("splice into element", t_num == 2 && !t_result);
    
    MCValueRelease(t_lst);
}
#endif
