/*                                                                    -*-c++-*-
 Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "foundation-private.h"

/* Currently, proper sets are based on sorted proper lists containing
 * interned values.  This makes performance quite poor for large
 * sets. */

////////////////////////////////////////////////////////////////////////////////

static compare_t __MCProperSet_CompareValue (const MCValueRef left, const MCValueRef right);

static uindex_t __MCProperSetFindNearest (MCProperSetRef set, const MCValueRef p_needle, MCValueRef & r_nearest);

////////////////////////////////////////////////////////////////////////////////

bool
MCProperSetCreateMutable (MCProperSetRef & r_list)
{
	return MCProperListCreateMutable (r_list);
}

bool
MCProperSetCopy (MCProperSetRef set,
                 MCProperSetRef & r_new_set)
{
	return MCProperListCopy (set, r_new_set);
}

bool
MCProperSetCopyAndRelease (MCProperSetRef set,
                           MCProperSetRef & r_new_set)
{
	return MCProperListCopyAndRelease (set, r_new_set);
}

bool
MCProperSetMutableCopy (MCProperSetRef set,
                        MCProperSetRef & r_new_set)
{
	return MCProperListMutableCopy (set, r_new_set);
}

bool
MCProperSetMutableCopyAndRelease (MCProperSetRef set,
                                  MCProperSetRef & r_new_set)
{
	return MCProperListMutableCopyAndRelease (set, r_new_set);
}

bool
MCProperSetIsMutable (MCProperSetRef set)
{
	return MCProperListIsMutable (set);
}

bool
MCProperSetIsEmpty(MCProperSetRef set)
{
	return MCProperListIsEmpty (set);
}

uindex_t
MCProperSetGetCount (MCProperSetRef set)
{
	return MCProperListGetLength (set);
}

bool
MCProperSetIsEqualTo (MCProperSetRef set,
                      MCProperSetRef other)
{
	return MCProperListIsEqualTo (set, other);
}

bool MCProperSetIterate (MCProperSetRef set,
                         uintptr_t & x_iterator,
                         MCValueRef & r_element)
{
	return MCProperListIterate (set, x_iterator, r_element);
}

bool
MCProperSetApply (MCProperSetRef set,
                  MCProperSetApplyCallback p_callback,
                  void *context)
{
	return MCProperListApply (set, p_callback, context);
}

bool
MCProperSetCopyAsProperList (MCProperSetRef set,
                             MCProperListRef & r_list)
{
	return MCProperListCopy (set, r_list);
}

////////////////////////////////////////////////////////////////////////////////

bool
MCProperSetCreate (const MCValueRef *p_values,
                   uindex_t p_value_count,
                   MCProperSetRef & r_set)
{
	bool t_success = true;

	/* Make all values immutable and intern them */
	MCValueRef *t_interned = nil;
	if (t_success)
		t_success = MCMemoryNewArray (p_value_count, t_interned);

	for (uindex_t i = 0; t_success && i < p_value_count; ++i)
	{
		t_success = MCValueInter (p_values[i], t_interned[i]);
	}

	/* Create the list and sort it */
	MCProperListRef t_list = nil;

	if (t_success)
		t_success = MCProperListCreate (t_interned, p_value_count, t_list);
	if (t_success)
		t_success = MCProperListMutableCopyAndRelease (t_list, t_list);
	if (t_success)
		t_success = MCProperListSort (t_list, false, __MCProperSet_CompareValue);

	if (t_success)
		t_success = MCProperListCopyAndRelease (t_list, r_set);

	if (!t_success)
		MCValueRelease (t_list);

	MCMemoryDeleteArray (t_interned);

	return t_success;
}

bool
MCProperSetContains (MCProperSetRef set,
                     MCValueRef p_value)
{
	/* Sets never contain mutable values */
	if (MCValueIsMutable (p_value)) return false;

	MCValueRef t_interned;
	if (!MCValueInter (t_interned, t_interned))
		return false;

	MCValueRef t_nearest;
	__MCProperSetFindNearest (set, t_interned, t_nearest);
	return (t_nearest == t_interned);
}

bool
MCProperSetAddElement (MCProperSetRef set,
                       MCValueRef p_value)
{
	MCAutoValueRef t_interned;
	if (!MCValueInter (p_value, &t_interned))
		return false;

	MCValueRef t_nearest;
	uindex_t t_index;
	t_index = __MCProperSetFindNearest (set, *t_interned, t_nearest);
	if (t_nearest == *t_interned) return true;

	return MCProperListInsertElement (set, *t_interned, t_index);
}

bool
MCProperSetRemoveElement (MCProperSetRef set,
                          MCValueRef p_value)
{
	MCAutoValueRef t_interned;
	if (!MCValueInter (p_value, &t_interned))
		return false;

	MCValueRef t_nearest;
	uindex_t t_index;
	t_index = __MCProperSetFindNearest (set, *t_interned, t_nearest);
	if (t_nearest != *t_interned) return true;

	return MCProperListRemoveElement (set, t_index);
}

bool
MCProperSetUnion (MCProperSetRef set,
                  MCProperSetRef other,
                  MCProperSetRef & r_union)
{
	/* Add the smaller set into the bigger one */
	if (MCProperSetGetCount (set) < MCProperSetGetCount (other))
		return MCProperSetUnion (other, set, r_union);

	bool t_success = true;

	MCProperSetRef t_result = nil;
	if (t_success)
		t_success = MCProperSetMutableCopy (set, t_result);

	uintptr_t t_iterator = 0;
	MCValueRef t_element;
	while (t_success && MCProperSetIterate (other, t_iterator, t_element))
		t_success = MCProperSetAddElement (t_result, t_element);

	if (t_success)
		t_success = MCProperSetCopyAndRelease (t_result, r_union);

	if (!t_success)
		MCValueRelease (t_result);

	return t_success;
}

bool
MCProperSetDifference (MCProperSetRef set,
                       MCProperSetRef other,
                       MCProperSetRef & r_difference)
{
	bool t_success = true;

	MCProperSetRef t_result = nil;
	if (t_success)
		t_success = MCProperSetMutableCopy (set, t_result);

	uintptr_t t_iterator = 0;
	MCValueRef t_element;
	while (t_success && MCProperSetIterate (other, t_iterator, t_element))
		t_success = MCProperSetRemoveElement (t_result, t_element);

	if (t_success)
		t_success = MCProperSetCopyAndRelease (t_result, r_difference);

	if (!t_success)
		MCValueRelease (t_result);

	return t_success;
}

bool
MCProperSetIntersection (MCProperSetRef set,
                         MCProperSetRef other,
                         MCProperSetRef & r_intersection)
{
	/* We have to iterate twice over one of the sets, so start off with the
	 * smallest one */
	if (MCProperSetGetCount (set) > MCProperSetGetCount (other))
		return MCProperSetIntersection (other, set, r_intersection);

	MCAutoProperSetRef t_difference;
	if (!MCProperSetDifference (set, other, &t_difference))
		return false;

	return MCProperSetDifference (set, *t_difference, r_intersection);
}

bool
MCProperSetDisjunction (MCProperSetRef set,
                        MCProperSetRef other,
                        MCProperSetRef & r_disjunction)
{
	bool t_success = true;

	MCProperSetRef t_result = nil;
	if (t_success)
		t_success = MCProperSetMutableCopy (set, t_result);

	uintptr_t t_iterator = 0;
	MCValueRef t_element;
	while (t_success && MCProperSetIterate (other, t_iterator, t_element))
	{
		MCValueRef t_nearest;
		uindex_t t_index;
		t_index = __MCProperSetFindNearest (t_result, t_element, t_nearest);

		/* If the element is in the result set, remove it.  If the element
		 * isn't in the result set, add it. */
		if (t_nearest == t_element)
			t_success = MCProperListRemoveElement (t_result, t_index);
		else
			t_success = MCProperListInsertElement (t_result, t_element, t_index);
	}

	if (t_success)
		t_success = MCProperSetCopyAndRelease (t_result, r_disjunction);

	if (!t_success)
		MCValueRelease (t_result);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static compare_t
__MCProperSet_CompareValue (const MCValueRef left,
                            const MCValueRef right)
{
	/* All values should be interned, so just perform a pointer comparison */
	return (compare_t) ((intptr_t) left - (intptr_t) right);
}

static uindex_t
__MCProperSetFindNearest (MCProperSetRef self,
                          MCValueRef p_needle,
                          MCValueRef & t_nearest)
{
	/* Binary search */
	uindex_t t_max, t_min, t_mid;
	compare_t t_diff;

	t_min = 0;
	t_max = MCProperSetGetCount (self);

	while (t_max >= t_min)
	{
		t_mid = (t_max + t_min) / 2;
		t_nearest = MCProperListFetchElementAtIndex (self, t_mid);

		/* All values are interned and list is sorted */
		t_diff = __MCProperSet_CompareValue (t_nearest, p_needle);

		if (t_diff == 0)
			return t_mid;
		if (t_diff < 0)
			t_min = t_mid + 1;
		else
			t_max = t_mid - 1;
	}

	/* Not found! t_min should be the insertion location */
	return t_min;
}
