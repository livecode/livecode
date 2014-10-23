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

void MCListEvalHeadOf(MCProperListRef p_target, MCValueRef& r_output)
{
    r_output = MCValueRetain(MCProperListFetchHead(p_target));
}

void MCListEvalTailOf(MCProperListRef p_target, MCValueRef& r_output)
{
    r_output = MCValueRetain(MCProperListFetchTail(p_target));
}

void MCListExecPushSingleElementOnto(MCValueRef p_value, MCProperListRef& x_target)
{
    if (MCProperListPushElement(x_target, p_value))
        return;
    
//    ctxt . Throw();
}

void MCListExecPushMultipleElementsOnto(MCProperListRef p_value, MCProperListRef& x_target)
{
    if (MCProperListAppendList(x_target, p_value))
        return;
    
//    ctxt . Throw();
}

void MCListExecPopElementInto(MCProperListRef& x_source, MCValueRef& r_output)
{
    if (MCProperListPop(x_source, r_output))
        return;
    
//    ctxt . Throw();
}

void MCListExecInsertSingleElementIntoListAt(MCValueRef p_value, MCProperListRef& x_target, index_t p_index)
{
    if (MCProperListInsertElement(x_target, p_value, p_index))
        return;
    
    //    ctxt . Throw();
}

void MCListExecInsertMultipleElementsIntoListAt(MCProperListRef p_value, MCProperListRef& x_target, index_t p_index)
{
    if (MCProperListInsertList(x_target, p_value, p_index))
        return;
    
    //    ctxt . Throw();
}

void MCListEvalNumberOfElementsIn(MCProperListRef p_target, uindex_t& r_output)
{
    r_output = MCProperListGetLength(p_target);
}

void MCListEvalIsAmongTheElementsOf(MCValueRef p_needle, MCProperListRef p_target, bool& r_output)
{
    uindex_t t_dummy;
    r_output = MCProperListFirstIndexOfElement(p_target, p_needle, 0, t_dummy);
}

void MCListEvalContains(MCProperListRef p_target, MCProperListRef p_needle, bool& r_output)
{
    uindex_t t_dummy;
    r_output = MCProperListFirstIndexOfList(p_target, p_needle, 0, t_dummy);
}

void MCListFetchElementOf(index_t p_index, MCProperListRef p_target, MCValueRef& r_output)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfElementChunkByExpression(p_target, p_index, t_start, t_count);
    r_output = MCValueRetain(MCProperListFetchElementAtIndex(p_target, t_start));
}

void MCListStoreElementOf(MCValueRef p_value, index_t p_index, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfElementChunkByExpression(x_target, p_index, t_start, t_count);
    MCProperListInsertElement(x_target, p_value, t_start);
}

void MCListFetchElementRangeOf(index_t p_start, index_t p_finish, MCProperListRef p_target, MCProperListRef& r_output)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfElementChunkByRange(p_target, p_start, p_finish, t_start, t_count);
    MCProperListCopySublist(p_target, MCRangeMake(p_start, p_finish - p_start + 1), r_output);
}

void MCListStoreElementRangeOf(MCValueRef p_value, index_t p_start, index_t p_finish, MCProperListRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfElementChunkByRange(x_target, p_start, p_finish, t_start, t_count);
    // PUT INTO AMBIGUITY
}