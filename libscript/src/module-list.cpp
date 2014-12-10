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

extern "C" MC_DLLEXPORT void MCListEvalHeadOf(MCProperListRef p_target, MCValueRef& r_output)
{
    r_output = MCValueRetain(MCProperListFetchHead(p_target));
}

extern "C" MC_DLLEXPORT void MCListEvalTailOf(MCProperListRef p_target, MCValueRef& r_output)
{
    r_output = MCValueRetain(MCProperListFetchTail(p_target));
}

extern "C" MC_DLLEXPORT void MCListExecPushSingleElementOnto(MCValueRef p_value, bool p_is_front, MCProperListRef& x_target)
{
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    if (p_is_front)
    {
        if (!MCProperListPushElementOntoFront(*t_mutable_list, p_value))
            return;
    }
    else
    {
        if (!MCProperListPushElementOntoBack(*t_mutable_list, p_value))
            return;
    }
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
    
//    ctxt . Throw();
}

extern "C" MC_DLLEXPORT void MCListExecPopElementInto(bool p_is_front, MCProperListRef& x_source, MCValueRef& r_output)
{
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_source, &t_mutable_list))
        return;
    
    if (p_is_front)
    {
        if (!MCProperListPopFront(*t_mutable_list, r_output))
            return;
    }
    else
    {
        if (!MCProperListPopBack(*t_mutable_list, r_output))
            return;
    }
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_source, *t_immutable);
    
//    ctxt . Throw();
}

extern "C" MC_DLLEXPORT void MCListEvalNumberOfElementsIn(MCProperListRef p_target, uindex_t& r_output)
{
    r_output = MCProperListGetLength(p_target);
}

extern "C" MC_DLLEXPORT void MCListEvalIsAmongTheElementsOf(MCValueRef p_needle, MCProperListRef p_target, bool& r_output)
{
    uindex_t t_dummy;
    r_output = MCProperListFirstIndexOfElement(p_target, p_needle, 0, t_dummy);
}

extern "C" MC_DLLEXPORT void MCListEvalContainsElements(MCProperListRef p_target, MCProperListRef p_needle, bool& r_output)
{
    uindex_t t_dummy;
    r_output = MCProperListFirstIndexOfList(p_target, p_needle, 0, t_dummy);
}

extern "C" MC_DLLEXPORT void MCListFetchElementOf(index_t p_index, MCProperListRef p_target, MCValueRef& r_output)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfElementChunkByExpression(p_target, p_index, t_start, t_count);
    
    if (t_count == 0)
        return;
    
    if (t_start + t_count > MCProperListGetLength(p_target))
        return;
    
    r_output = MCValueRetain(MCProperListFetchElementAtIndex(p_target, t_start));
}

extern "C" MC_DLLEXPORT void MCListStoreElementOf(MCValueRef p_value, index_t p_index, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfElementChunkByExpression(x_target, p_index, t_start, t_count);
    
    if (t_count == 0)
        return;
    
    if (t_start + t_count > MCProperListGetLength(x_target))
        return;
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListRemoveElements(*t_mutable_list, t_start, t_count);
    MCProperListInsertElement(*t_mutable_list, p_value, t_start);
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
}

extern "C" MC_DLLEXPORT void MCListFetchElementRangeOf(index_t p_start, index_t p_finish, MCProperListRef p_target, MCProperListRef& r_output)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfElementChunkByRange(p_target, p_start, p_finish, t_start, t_count);
    
    if (t_count == 0)
        return;
    
    if (t_start + t_count > MCProperListGetLength(p_target))
        return;
    
    MCProperListCopySublist(p_target, MCRangeMake(t_start, t_count), r_output);
}

extern "C" MC_DLLEXPORT void MCListStoreElementRangeOf(MCValueRef p_value, index_t p_start, index_t p_finish, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfElementChunkByRange(x_target, p_start, p_finish, t_start, t_count);
    
    if (t_count == 0)
        return;
    
    if (t_start + t_count > MCProperListGetLength(x_target))
        return;
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListRemoveElements(*t_mutable_list, t_start, t_count);
    MCProperListInsertElement(*t_mutable_list, p_value, t_start);
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
}

extern "C" MC_DLLEXPORT void MCListFetchIndexOf(MCProperListRef p_target, index_t p_index, MCValueRef& r_output)
{
    MCListFetchElementOf(p_index, p_target, r_output);
}

extern "C" MC_DLLEXPORT void MCListStoreIndexOf(MCValueRef p_value, MCProperListRef& x_target, index_t p_index)
{
    MCListStoreElementOf(p_value, p_index, x_target);
}

extern "C" MC_DLLEXPORT void MCListStoreAfterElementOf(MCValueRef p_value, index_t p_index, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    t_start += t_count;
    MCChunkGetExtentsOfElementChunkByExpression(x_target, p_index, t_start, t_count);
    
    if (t_count == 0)
        return;
    
    if (t_start + t_count > MCProperListGetLength(x_target))
        return;
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListInsertElement(*t_mutable_list, p_value, t_start);
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
}

extern "C" MC_DLLEXPORT void MCListStoreBeforeElementOf(MCValueRef p_value, index_t p_index, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfElementChunkByExpression(x_target, p_index, t_start, t_count);

    if (t_count == 0)
        return;
    
    if (t_start + t_count > MCProperListGetLength(x_target))
        return;
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;
    
    MCProperListInsertElement(*t_mutable_list, p_value, t_start);
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void MCListSpliceIntoElementRangeOf(MCProperListRef p_list, index_t p_start, index_t p_finish, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfElementChunkByRange(x_target, p_start, p_finish, t_start, t_count);
    
    if (t_count == 0)
        return;
    
    if (t_start + t_count > MCProperListGetLength(x_target))
        return;
    
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

extern "C" MC_DLLEXPORT void MCListSpliceIntoElementOf(MCProperListRef p_list, index_t p_index, MCProperListRef& x_target)
{
    MCListSpliceIntoElementRangeOf(p_list, p_index, p_index, x_target);
}

extern "C" MC_DLLEXPORT void MCListSpliceBeforeElementOf(MCProperListRef p_list, index_t p_index, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfElementChunkByExpression(x_target, p_index, t_start, t_count);
    
    if (t_count == 0)
        return;
    
    if (t_start + t_count > MCProperListGetLength(x_target))
        return;
    
    MCAutoProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(x_target, &t_mutable_list))
        return;

    MCProperListInsertList(*t_mutable_list, p_list, t_start);
    
    MCAutoProperListRef t_immutable;
    if (!MCProperListCopy(*t_mutable_list, &t_immutable))
        return;
    
    MCValueAssign(x_target, *t_immutable);
}

extern "C" MC_DLLEXPORT void MCListSpliceAfterElementOf(MCProperListRef p_list, index_t p_index, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfElementChunkByExpression(x_target, p_index, t_start, t_count);
    
    if (t_count == 0)
        return;
    
    if (t_start + t_count > MCProperListGetLength(x_target))
        return;
    
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

extern "C" MC_DLLEXPORT void MCListEvalEmpty(MCProperListRef& r_output)
{
    r_output = MCValueRetain(kMCEmptyProperList);
}

extern "C" bool MCListRepeatForEachElement(void*& x_iterator, MCValueRef& r_iterand, MCProperListRef p_list)
{
    uindex_t t_offset;
    t_offset = (uindex_t)x_iterator;
    
    if (t_offset == MCProperListGetLength(p_list))
        return false;
    
    r_iterand = MCValueRetain(MCProperListFetchElementAtIndex(p_list, t_offset));
    
    x_iterator = (void *)(t_offset + 1);
    
    return true;
}

extern "C" void MCListEvalBeginsWith(MCProperListRef p_list, MCProperListRef p_prefix, bool& r_output)
{
    r_output = MCProperListBeginsWithList(p_list, p_prefix);
}

extern "C" void MCListEvalEndsWith(MCProperListRef p_list, MCProperListRef p_suffix, bool& r_output)
{
    r_output = MCProperListEndsWithList(p_list, p_suffix);
}

extern "C" void MCListEvalIsEqualTo(MCProperListRef p_left, MCProperListRef p_right, bool& r_output)
{
    r_output = MCProperListIsEqualTo(p_left, p_right);
}

extern "C" void MCListEvalIsNotEqualTo(MCProperListRef p_left, MCProperListRef p_right, bool& r_output)
{
    r_output = !MCProperListIsEqualTo(p_left, p_right);
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
