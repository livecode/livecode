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
#include <foundation-stdlib.h>

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

// Returns true if the list is indirect.
bool MCProperListIsIndirect(MCProperListRef self);

// Creates an indirect mutable list with contents.
static bool __MCProperListCreateIndirect(__MCProperList *contents, __MCProperList*& r_list);

// Returns true if the list is indirect.
static bool __MCProperListIsIndirect(__MCProperList *self);

// Replaces all the values in the list with immutable copies.
static bool __MCProperListMakeContentsImmutable(__MCProperList *self);

// Creates an immutable list from this one, changing 'self' to indirect.
static bool __MCProperListMakeIndirect(__MCProperList *self);

// Ensures the given mutable but indirect list is direct.
static bool __MCProperListResolveIndirect(__MCProperList *self);

static bool __MCProperListExpandAt(MCProperListRef self, uindex_t p_at, uindex_t p_count);

static bool __MCProperListShrinkAt(MCProperListRef self, uindex_t p_at, uindex_t p_count);

static void __MCProperListClampRange(MCProperListRef self, MCRange& x_range);

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCProperListCreate(const MCValueRef *p_values, uindex_t p_length, MCProperListRef& r_list)
{
	bool t_success;
	t_success = true;

	MCProperListRef t_list;
	t_list = nil;
	if (t_success)
		t_success = MCProperListCreateMutable(t_list);

	if (t_success)
        t_success = MCProperListPushElementsOntoBack(t_list, p_values, p_length);

	if (t_success)
		return MCProperListCopyAndRelease(t_list, r_list);

	MCValueRelease(t_list);
	return false;
}

MC_DLLEXPORT_DEF
bool MCProperListCreateWithForeignValues(MCTypeInfoRef p_typeinfo, const void *p_values, uindex_t p_value_count, MCProperListRef& r_list)
{
    __MCAssertIsForeignTypeInfo(p_typeinfo);
    MCAssert(p_values != nullptr || p_value_count == 0);

    MCAutoProperListRef t_list;
    if (!MCProperListCreateMutable(&t_list))
    {
        return false;
    }

    const MCForeignTypeDescriptor *t_descriptor =
            MCForeignTypeInfoGetDescriptor(p_typeinfo);

    while(p_value_count > 0)
    {
        MCAutoValueRef t_value;
        if (t_descriptor->doimport != nil)
        {
            if (!t_descriptor->doimport(t_descriptor,
                                        (void *)p_values,
                                        false,
                                        &t_value))
            {
                   return false;
            }
        }
        else
        {
            if (!MCForeignValueCreate(p_typeinfo,
                                      (void *)p_values,
                                      (MCForeignValueRef&)&t_value))
            {
                   return false;
            }
        }

        if (!MCProperListPushElementOntoBack(*t_list,
                                             *t_value))
        {
            return false;
        }

        p_value_count -= 1;
        p_values = (byte_t *)p_values + t_descriptor->size;
    }

    if (!t_list.MakeImmutable())
    {
            return false;
    }

    r_list = t_list.Take();

    return true;
}

MC_DLLEXPORT_DEF
bool MCProperListConvertToForeignValues(MCProperListRef self, MCTypeInfoRef p_typeinfo, void*& r_values_ptr, uindex_t& r_values_count)
{
    __MCAssertIsForeignTypeInfo(p_typeinfo);
    
    const MCForeignTypeDescriptor *t_descriptor =
    MCForeignTypeInfoGetDescriptor(p_typeinfo);
    
    uindex_t t_values_count = MCProperListGetLength(self);
    void *t_values = nullptr;
    if (!MCMemoryNew(t_values_count * t_descriptor->size, t_values))
    {
        return false;
    }
    
    byte_t *t_values_ptr = (byte_t*)t_values;
    for(uindex_t t_index = 0; t_index < t_values_count; t_index++)
    {
        MCValueRef t_value = MCProperListFetchElementAtIndex(self, t_index);
        if (MCValueGetTypeInfo(t_value) == p_typeinfo)
        {
            MCMemoryCopy(t_values_ptr, MCForeignValueGetContentsPtr(t_value), t_descriptor->size);
        }
        else if (MCValueGetTypeInfo(t_value) == t_descriptor->bridgetype)
        {
            if (!t_descriptor->doexport(t_descriptor, t_value, false, t_values_ptr))
            {
                MCMemoryDelete(t_values);
                return false;
            }
        }
        else
        {
            MCMemoryDelete(t_values);
            return false;
        }
        t_values_ptr += t_descriptor->size;
    }
    
    r_values_ptr = t_values;
    r_values_count = t_values_count;
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCProperListCreateMutable(MCProperListRef& r_list)
{
	if (!__MCValueCreate(kMCValueTypeCodeProperList, r_list))
		return false;

	r_list -> flags |= kMCProperListFlagIsMutable;

	return true;
}	

bool
MCProperListCreateAndRelease(MCValueRef *p_values,
                             uindex_t p_length,
                             MCProperListRef& r_list)
{
	__MCProperList *t_list;
	if (!__MCValueCreate(kMCValueTypeCodeProperList, t_list))
		return false;

	t_list -> list = p_values;
	t_list -> length = p_length;

	r_list = t_list;
	return true;
}

MC_DLLEXPORT_DEF
bool MCProperListCopy(MCProperListRef self, MCProperListRef& r_new_list)
{
	// If we aren't mutable, then we can just copy directly.
	if (!MCProperListIsMutable(self))
	{
		r_new_list = MCValueRetain(self);
		return true;
	}

	// If we mutable, but still indirect just take the contents list.
	if (__MCProperListIsIndirect(self))
	{
		r_new_list = MCValueRetain(self -> contents);
		return true;
	}

	// Make the contents immutable.
	if (!__MCProperListMakeContentsImmutable(self))
		return false;

	// Make the array indirect.
	if (!__MCProperListMakeIndirect(self))
		return false;

	// Return a copy of the contents.
	r_new_list = MCValueRetain(self -> contents);
	return true;
}

MC_DLLEXPORT_DEF
bool MCProperListCopyAndRelease(MCProperListRef self, MCProperListRef& r_new_list)
{
	// If we aren't mutable, then new list is just us.
	if (!MCProperListIsMutable(self))
	{
		r_new_list = self;
		return true;
	}

	// If we are indirect, then new list is the contents, we are released.
	if (__MCProperListIsIndirect(self))
	{
		r_new_list = MCValueRetain(self -> contents);
		MCValueRelease(self);
		return true;
	}

	// We need to make an immutable copy, so first change all our contents
	// to immutable.
	if (!__MCProperListMakeContentsImmutable(self))
		return false;

	// If we have a reference count of one 'self' becomes the immutable copy.
	if (self -> references == 1)
	{
		// We are no longer immutable.
		self -> flags &= ~kMCProperListFlagIsMutable;

		// Return this as the new list.
		r_new_list = self;
		return true;
	}

	// Otherwise we must build a new indirect value.
	if (!__MCProperListMakeIndirect(self))
		return false;

	// Reduce our reference count.
	self -> references -= 1;

	// Return a copy of the contents.
	r_new_list = MCValueRetain(self -> contents);
	return true;
}

MC_DLLEXPORT_DEF
bool MCProperListMutableCopy(MCProperListRef self, MCProperListRef& r_new_list)
{
	// If the list is immutable, then the new mutable list will be indirect
	// referencing it. [ non-mutable lists cannot be indirect so self does not
	// need resolving ].
	if (!MCProperListIsMutable(self))
		return __MCProperListCreateIndirect(self, r_new_list);

	// If the list is already indirect, we just create a new reference to the
	// same contents.
	if (__MCProperListIsIndirect(self))
		return __MCProperListCreateIndirect(self -> contents, r_new_list);

	// If the list is mutable, we make it immutable and indirect and share
	// the indirect copy.
	if (!__MCProperListMakeContentsImmutable(self))
		return false;

	// Make self indirect, and fetch its contents.
	if (!__MCProperListMakeIndirect(self))
		return false;

	// Finally, create a new indirect list with contents of self.
	return __MCProperListCreateIndirect(self -> contents, r_new_list);
}

MC_DLLEXPORT_DEF
bool MCProperListMutableCopyAndRelease(MCProperListRef self, MCProperListRef& r_new_list)
{
	if (self -> references == 1)
	{
		if (!MCProperListIsMutable(self))
			self -> flags |= kMCProperListFlagIsMutable;

		r_new_list = self;
		return true;
	}

	if (!MCProperListMutableCopy(self, r_new_list))
		return false;

	self -> references -= 1;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCProperListIsMutable(MCProperListRef self)
{
	return (self -> flags & kMCProperListFlagIsMutable) != 0;
}

bool MCProperListIsIndirect(MCProperListRef self)
{
	return (self -> flags & kMCProperListFlagIsIndirect) != 0;
}

MC_DLLEXPORT_DEF
uindex_t MCProperListGetLength(MCProperListRef self)
{
	if (!__MCProperListIsIndirect(self))
		return self -> length;
	return self -> contents -> length;
}

MC_DLLEXPORT_DEF
bool MCProperListIsEmpty(MCProperListRef self)
{
	if (!__MCProperListIsIndirect(self))
        return (self -> length == 0);
    
    return (self -> contents -> length == 0);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCProperListPushElementsOntoBack(MCProperListRef self, const MCValueRef *p_values, uindex_t p_length)
{
    return MCProperListInsertElements(self, p_values, p_length, MCProperListGetLength(self));
}

MC_DLLEXPORT_DEF
bool MCProperListPushElementsOntoFront(MCProperListRef self, const MCValueRef *p_values, uindex_t p_length)
{
    return MCProperListInsertElements(self, p_values, p_length, 0);
}

MC_DLLEXPORT_DEF
bool MCProperListPushElementOntoBack(MCProperListRef self, const MCValueRef p_value)
{
    return MCProperListPushElementsOntoBack(self, &p_value, 1);
}

MC_DLLEXPORT_DEF
bool MCProperListPushElementOntoFront(MCProperListRef self, const MCValueRef p_value)
{
    return MCProperListPushElementsOntoFront(self, &p_value, 1);
}

MC_DLLEXPORT_DEF
bool MCProperListAppendList(MCProperListRef self, MCProperListRef p_value)
{
    if (MCProperListIsIndirect(p_value))
        p_value = p_value -> contents;
    
    if (p_value != self)
        return MCProperListPushElementsOntoBack(self, p_value -> list, p_value -> length);
    
    MCAutoProperListRef t_list;
	if (!MCProperListCopy(p_value, &t_list))
		return false;

    return MCProperListAppendList(self, *t_list);
}

MC_DLLEXPORT_DEF
bool MCProperListInsertElements(MCProperListRef self, const MCValueRef *p_values, uindex_t p_length, index_t p_index)
{
    MCAssert(MCProperListIsMutable(self));
    
    if (__MCProperListIsIndirect(self))
        if (!__MCProperListResolveIndirect(self))
            return false;
    
    if (!__MCProperListExpandAt(self, p_index, p_length))
        return false;
    
    for (uindex_t i = 0; i < p_length; i++)
        self -> list[i + p_index] = MCValueRetain(p_values[i]);
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCProperListInsertElement(MCProperListRef self, MCValueRef p_value, index_t p_index)
{
    return MCProperListInsertElements(self, &p_value, 1, p_index);
}

MC_DLLEXPORT_DEF
bool MCProperListInsertList(MCProperListRef self, MCProperListRef p_value, index_t p_index)
{
    if (MCProperListIsIndirect(p_value))
        p_value = p_value -> contents;
    
    if (p_value != self)
        return MCProperListInsertElements(self, p_value -> list, p_value -> length, p_index);
    
    MCAutoProperListRef t_list;
	if (!MCProperListCopy(p_value, &t_list))
		return false;

    return MCProperListInsertList(self, *t_list, p_index);
}

MC_DLLEXPORT_DEF
bool MCProperListRemoveElements(MCProperListRef self, uindex_t p_start, uindex_t p_count)
{
    MCAssert(MCProperListIsMutable(self));
    
    if (__MCProperListIsIndirect(self))
        if (!__MCProperListResolveIndirect(self))
            return false;
    
    MCAutoArray<MCValueRef> t_values;
    for (uindex_t i = p_start; i < p_start + p_count; i++)
	{
		if (!t_values . Push(self -> list[i]))
			return false;
	}
    
    if (!__MCProperListShrinkAt(self, p_start, p_count))
        return false;
    
    for (uindex_t i = 0; i < t_values . Size(); i++)
        MCValueRelease(t_values[i]);
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCProperListRemoveElement(MCProperListRef self, uindex_t p_index)
{
    return MCProperListRemoveElements(self, p_index, 1);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
MCValueRef MCProperListFetchHead(MCProperListRef self)
{
    if (MCProperListIsIndirect(self))
        self = self -> contents;

	MCAssert (self->length > 0);

    return self -> list[0];
}

MC_DLLEXPORT_DEF
MCValueRef MCProperListFetchTail(MCProperListRef self)
{
    if (MCProperListIsIndirect(self))
        self = self -> contents;

	MCAssert (self->length > 0);

    return self -> list[self -> length - 1];
}

MC_DLLEXPORT_DEF
MCValueRef MCProperListFetchElementAtIndex(MCProperListRef self, uindex_t p_index)
{
    if (MCProperListIsIndirect(self))
        self = self -> contents;
    
    if (p_index >= self -> length)
        return kMCNull;
    
    return self -> list[p_index];
}

MC_DLLEXPORT_DEF
bool MCProperListPopBack(MCProperListRef self, MCValueRef& r_value)
{
    MCAssert(MCProperListIsMutable(self));
    
    if (__MCProperListIsIndirect(self))
        if (!__MCProperListResolveIndirect(self))
            return false;

	MCAssert (self -> length > 0);

    MCValueRef t_value;
    t_value = self -> list[self -> length - 1];
    
    if (!__MCProperListShrinkAt(self, self -> length - 1, 1))
        return false;
    
    r_value = t_value;
    return true;
}

MC_DLLEXPORT_DEF
bool MCProperListPopFront(MCProperListRef self, MCValueRef& r_value)
{
    MCAssert(MCProperListIsMutable(self));
    
    if (__MCProperListIsIndirect(self))
        if (!__MCProperListResolveIndirect(self))
            return false;

	MCAssert (self -> length > 0);

    MCValueRef t_value;
    t_value = self -> list[0];
    
    if (!__MCProperListShrinkAt(self, 0, 1))
        return false;
    
    r_value = t_value;
    return true;
}

MC_DLLEXPORT_DEF
bool MCProperListCopySublist(MCProperListRef self, MCRange p_range, MCProperListRef& r_elements)
{
    if (MCProperListIsIndirect(self))
        self = self -> contents;
    
    __MCProperListClampRange(self, p_range);
    return MCProperListCreate(self -> list + p_range . offset, p_range . length, r_elements);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCProperListFirstIndexOfElement(MCProperListRef self, MCValueRef p_needle, uindex_t p_after, uindex_t& r_offset)
{
	return MCProperListFirstIndexOfElementInRange(self, p_needle,
	            MCRangeMake(p_after, UINDEX_MAX), r_offset);
}

MC_DLLEXPORT_DEF bool
MCProperListFirstIndexOfElementInRange (MCProperListRef self,
                                        MCValueRef p_needle,
                                        MCRange p_range,
                                        uindex_t & r_offset)
{
	if (MCProperListIsIndirect (self))
		self = self->contents;

	__MCProperListClampRange (self, p_range);

	for (uindex_t t_offset = 0; /* Relative to start of range */
	     t_offset < p_range.length;
	     ++t_offset)
	{
		uindex_t t_index = p_range.offset + t_offset;
		if (MCValueIsEqualTo(p_needle, self->list[t_index]))
		{
			r_offset = t_offset;
			return true;
		}
	}

	return false;
}

MC_DLLEXPORT_DEF bool
MCProperListLastIndexOfElementInRange (MCProperListRef self,
                                       MCValueRef p_needle,
                                       MCRange p_range,
                                       uindex_t & r_offset)
{
	if (MCProperListIsIndirect (self))
		self = self->contents;

	__MCProperListClampRange (self, p_range);

	uindex_t t_offset = p_range.length; /* Relative to start of range */
	while (0 < t_offset--)
	{
		uindex_t t_index = p_range.offset + t_offset;
		if (MCValueIsEqualTo (p_needle, self->list[t_index]))
		{
			r_offset = t_offset;
			return true;
		}
	}
	return false;
}

MC_DLLEXPORT_DEF
bool MCProperListFirstOffsetOfList(MCProperListRef self, MCProperListRef p_needle, uindex_t p_after, uindex_t& r_offset)
{
	return MCProperListFirstOffsetOfListInRange (self, p_needle,
	            MCRangeMake (p_after, UINDEX_MAX), r_offset);
}

MC_DLLEXPORT_DEF bool
MCProperListFirstOffsetOfListInRange (MCProperListRef self,
                                      MCProperListRef p_needle,
                                      MCRange p_range,
                                      uindex_t & r_offset)
{
	if (MCProperListIsIndirect(p_needle))
		p_needle = p_needle->contents;

	/* Empty lists are never found */
	if (0 == p_needle->length)
		return false;

	if (MCProperListIsIndirect(self))
		self = self->contents;

	__MCProperListClampRange (self, p_range);

	/* If the range is too short to contain the needle, the needle
	 * clearly can't be found. */
	if (p_range.length < p_needle->length)
		return false;

	/* Search algorithm: look forward along the range for the first
	 * occurrence of the *last* element of the needle, then work
	 * backward along the needle to see if the lists match. */

	for (uindex_t t_offset = 0; /* Relative to start of range */
	     t_offset <= p_range.length - p_needle->length;
	     ++t_offset)
	{
		bool t_match = true;

		/* Correlate the two lists at this offset */
		for (uindex_t t_needle_rindex = 0; /* Relative to *end* of needle */
		     t_needle_rindex < p_needle->length && t_match;
		     ++t_needle_rindex)
		{
			uindex_t t_needle_index = p_needle->length - t_needle_rindex - 1;
			uindex_t t_self_index = p_range.offset + t_offset + t_needle_index;

			t_match = MCValueIsEqualTo (p_needle->list[t_needle_index],
			                            self->list[t_self_index]);
		}

		if (t_match)
		{
			r_offset = t_offset;
			return true;
		}
	}
	return false;
}

MC_DLLEXPORT_DEF bool
MCProperListLastOffsetOfListInRange (MCProperListRef self,
                                     MCProperListRef p_needle,
                                     MCRange p_range,
                                     uindex_t & r_offset)
{
	if (MCProperListIsIndirect (p_needle))
		p_needle = p_needle->contents;

	/* Empty lists are never found */
	if (0 == p_needle->length)
		return false;

	if (MCProperListIsIndirect (self))
		self = self->contents;

	__MCProperListClampRange (self, p_range);

	/* If the range is too short to contain the needle, the needle clearly
	 * can't be found. */
	if (p_range.length < p_needle->length)
		return false;

	/* Search algorithm: look backward along the range for the first
	 * occurrence of the first element of the needle, then work
	 * forward along the needle to see if the lists match. */

	/* t_roffset is the reverse offset of the first index in a
	 * possible match, relative to the last element in the range
	 * (i.e. t_roffset = 0 for the last element). */
	for (uindex_t t_roffset = p_needle->length - 1;
	     t_roffset < p_range.length;
	     ++t_roffset)
	{
		/* Offset of first element in the match, relative to start of
		 * range (i.e. t_offset = 0 for the first element) */
		uindex_t t_offset = p_range.length - t_roffset - 1;

		bool t_match = true;

		/* Correlate the two lists at this offset */
		for (uindex_t t_needle_index = 0; /* Relative to start of needle */
		     t_needle_index < p_needle->length && t_match;
		     ++t_needle_index)
		{
			uindex_t t_self_index = p_range.offset + t_offset + t_needle_index;

			t_match = MCValueIsEqualTo (p_needle->list[t_needle_index],
			                            self->list[t_self_index]);
		}

		if (t_match)
		{
			r_offset = t_offset;
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCProperListIterate(MCProperListRef self, uintptr_t& x_iterator, MCValueRef& r_element)
{
    if (MCProperListIsIndirect(self))
        self = self -> contents;
    
    if (x_iterator == self -> length)
        return false;
    
    r_element = self -> list[x_iterator];
    
    x_iterator += 1;
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCProperListApply(MCProperListRef self, MCProperListApplyCallback p_callback, void *context)
{
    if (MCProperListIsIndirect(self))
        self = self -> contents;
    
    for (uindex_t i = 0; i < self -> length; i++)
        if (!p_callback(context, self -> list[i]))
            return false;
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCProperListMap(MCProperListRef self, MCProperListMapCallback p_callback, MCProperListRef& r_new_list, void *context)
{
    if (MCProperListIsIndirect(self))
        self = self -> contents;
    
	MCAutoValueRefArray t_values;
	if (!t_values.New (self -> length))
		return false;

    bool t_success;
    t_success = true;
    
    for (uindex_t i = 0; t_success && i < self -> length; i++)
    {
        MCValueRef t_value;
		t_value = NULL;

        if (!p_callback(context, self -> list[i], t_value))
            t_success = false;
        

		/* In case the callback returns a value into t_value but also
		 * indicates failure, make sure to release t_value on
		 * failure. */
		if (t_success)
			t_values[i] = t_value;
		else
			MCValueRelease (t_value);
    }
    
    if (t_success)
		t_success = t_values.TakeAsProperList (r_new_list);

    return t_success;
}

MC_DLLEXPORT_DEF
bool MCProperListSort(MCProperListRef self, bool p_reverse, MCProperListQuickSortCallback p_callback)
{
    MCAssert(MCProperListIsMutable(self));
 
    uindex_t t_item_count;
    t_item_count = MCProperListGetLength(self);
    
    if (t_item_count < 2)
        return true;
    
    if (MCProperListIsIndirect(self))
        if (!__MCProperListResolveIndirect(self))
            return false;

    qsort(self -> list, self -> length, sizeof(MCValueRef), (int (*)(const void *, const void *))p_callback);

    return true;
}

static void MCProperListDoStableSort(MCValueRef*& list, uindex_t p_item_count, MCValueRef*& p_temp, bool p_reverse, MCProperListCompareElementCallback p_callback, void *context)
{
    if (p_item_count <= 1)
        return;
    
    uint32_t t_first_half_count = p_item_count / 2;
    uint32_t t_second_half_count = p_item_count - t_first_half_count;
    MCValueRef *t_first_half = list;
    MCValueRef *t_second_half = t_first_half + t_first_half_count;
    
    // Sort the halves recursively
    MCProperListDoStableSort(t_first_half, t_first_half_count, p_temp, p_reverse, p_callback, context);
    MCProperListDoStableSort(t_second_half, t_second_half_count, p_temp, p_reverse, p_callback, context);
    
    // And interleave appropriately
    MCValueRef *tmp = p_temp;
    while (t_first_half_count > 0 && t_second_half_count > 0)
    {
        compare_t t_result;
        t_result = p_callback(context, *t_first_half, *t_second_half);
        bool t_take_first;
        t_take_first = p_reverse ? t_result >= 0 : t_result <= 0;
        
        if (t_take_first)
        {
            *tmp++ = *t_first_half++;
            t_first_half_count--;
        }
        else
        {
            *tmp++ = *t_second_half++;
            t_second_half_count--;
        }
    }
    
    for (uindex_t i = 0; i < t_first_half_count; i++)
        tmp[i] = t_first_half[i];
    for (uindex_t i = 0; i < (p_item_count - t_second_half_count); i++)
        list[i] = p_temp[i];
}

MC_DLLEXPORT_DEF
bool MCProperListStableSort(MCProperListRef self, bool p_reverse, MCProperListCompareElementCallback p_callback, void *context)
{
    MCAssert(MCProperListIsMutable(self));
    
    uindex_t t_item_count;
    t_item_count = MCProperListGetLength(self);
        
    if (t_item_count < 2)
        return true;
        
    if (__MCProperListIsIndirect(self))
        if (!__MCProperListResolveIndirect(self))
            return false;
    
    MCValueRef *t_temp_array = new (nothrow) MCValueRef[t_item_count];
    
    MCProperListDoStableSort(self -> list, t_item_count, t_temp_array, p_reverse, p_callback, context);
    
    delete[] t_temp_array;
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCProperListIsEqualTo(MCProperListRef self, MCProperListRef p_other)
{
    return __MCProperListIsEqualTo(self, p_other);
}

MC_DLLEXPORT_DEF
bool MCProperListBeginsWithList(MCProperListRef self, MCProperListRef p_prefix)
{
    // If the list is indirect, get the contents.
    MCProperListRef t_contents;
    if (!__MCProperListIsIndirect(self))
        t_contents = self;
    else
        t_contents = self -> contents;
    
    // If the other list is indirect, get its contents.
    MCProperListRef t_other_contents;
    if (!__MCProperListIsIndirect(p_prefix))
        t_other_contents = p_prefix;
    else
        t_other_contents = p_prefix -> contents;
    
    if (t_other_contents -> length > t_contents -> length)
        return false;
    
    for(uindex_t i = 0; i < t_other_contents -> length; i++)
    {
        if (!MCValueIsEqualTo(t_contents -> list[i], t_other_contents -> list[i]))
            return false;
    }
    
    // If we get here it means all values in the p_prefix are the same as their equivalents in self.
    return true;
}

MC_DLLEXPORT_DEF
bool MCProperListEndsWithList(MCProperListRef self, MCProperListRef p_suffix)
{
    // If the list is indirect, get the contents.
    MCProperListRef t_contents;
    if (!__MCProperListIsIndirect(self))
        t_contents = self;
    else
        t_contents = self -> contents;
    
    // If the other list is indirect, get its contents.
    MCProperListRef t_other_contents;
    if (!__MCProperListIsIndirect(p_suffix))
        t_other_contents = p_suffix;
    else
        t_other_contents = p_suffix -> contents;
    
    if (t_other_contents -> length > t_contents -> length)
        return false;
    
    for(uindex_t i = 1; i <= t_other_contents -> length; i++)
    {
        if (!MCValueIsEqualTo(t_contents -> list[t_contents -> length - i], t_other_contents -> list[t_other_contents -> length - i]))
            return false;
    }
    
    // If we get here it means all values in the p_suffix are the same as their equivalents in self.
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCProperListIsListOfType(MCProperListRef self, MCValueTypeCode p_type)
{
    // If the list is indirect, get the contents.
    MCProperListRef t_contents;
    if (!__MCProperListIsIndirect(self))
        t_contents = self;
    else
        t_contents = self -> contents;
    
    for(uindex_t i = 0; i < t_contents -> length; i++)
    {
        if (MCValueGetTypeCode(t_contents -> list[i]) != p_type)
            return false;
    }
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCProperListIsHomogeneous(MCProperListRef self, MCValueTypeCode& r_type)
{
    if (MCProperListIsEmpty(self))
    {
        r_type = kMCValueTypeCodeNull;
        return true;
    }
    
    // If the list is indirect, get the contents.
    MCProperListRef t_contents;
    if (!__MCProperListIsIndirect(self))
        t_contents = self;
    else
        t_contents = self -> contents;
    
    MCValueTypeCode t_type;
    t_type = MCValueGetTypeCode(t_contents -> list[0]);
    
    if (MCProperListIsListOfType(t_contents, t_type))
    {
        r_type = t_type;
        return true;
    }
    
    return false;
}

MC_DLLEXPORT_DEF
bool MCProperListReverse(MCProperListRef self)
{
    MCAssert(MCProperListIsMutable(self));

    // Ensure the list ref is not indirect
    if (__MCProperListIsIndirect(self))
        if (!__MCProperListResolveIndirect(self))
            return false;

    MCInplaceReverse(self->list, self->length);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCProperListConvertToArray(MCProperListRef p_list, MCArrayRef& r_array)
{
    MCAutoArrayRef t_array;
    if (!MCArrayCreateMutable(&t_array))
    {
        return false;
    }
    
    for(uindex_t t_index = 0; t_index < MCProperListGetLength(p_list); t_index++)
    {
        if (!MCArrayStoreValueAtIndex(*t_array, t_index + 1, MCProperListFetchElementAtIndex(p_list, t_index)))
        {
            return false;
        }
    }
    
    if (!t_array.MakeImmutable())
    {
        return false;
    }
    
    r_array = t_array.Take();
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void __MCProperListDestroy(__MCProperList *self)
{
	if (__MCProperListIsIndirect(self))
		MCValueRelease(self -> contents);
	else
	{
		for(uindex_t i = 0; i < self -> length; i++)
			MCValueRelease(self -> list[i]);

		MCMemoryDeleteArray(self -> list);
	}
}

hash_t __MCProperListHash(__MCProperList *self)
{
	return (hash_t)self -> length;
}

bool __MCProperListIsEqualTo(__MCProperList *self, __MCProperList *other_self)
{
	// If the list is indirect, get the contents.
	MCProperListRef t_contents;
	if (!__MCProperListIsIndirect(self))
		t_contents = self;
	else
		t_contents = self -> contents;

	// If the other list is indirect, get its contents.
	MCProperListRef t_other_contents;
	if (!__MCProperListIsIndirect(other_self))
		t_other_contents = other_self;
	else
		t_other_contents = other_self -> contents;

	if (t_contents -> length != t_other_contents -> length)
		return false;

	for(uindex_t i = 0; i < t_contents -> length; i++)
	{
		if (!MCValueIsEqualTo(t_contents -> list[i], t_other_contents -> list[i]))
            return false;
	}

	// If we get here it means all values in the list are the same.
	return true;
}

bool __MCProperListCopyDescription(__MCProperList *self, MCStringRef& r_string)
{
	/* Shortcut for empty lists */
	if (MCProperListIsEmpty (self))
		return MCStringCopy (MCSTR("[]"), r_string);

	MCAutoListRef t_contents_list;
	if (!MCListCreateMutable (MCSTR(", "), &t_contents_list))
		return false;

	uintptr_t t_iter = 0;
	MCValueRef t_value;
	while (MCProperListIterate (self, t_iter, t_value))
	{
        // AL-2015-03-26: [[ Bug 15005 ]] Use MCValueCopyDescription to ensure the
        //  description of a proper list is as full as possible.
        MCAutoStringRef t_string;
        if (!MCValueCopyDescription(t_value, &t_string))
            return false;
        
		if (!MCListAppend (*t_contents_list, *t_string))
			return false;
	}

	MCAutoStringRef t_contents_string;
	if (!MCListCopyAsString (*t_contents_list, &t_contents_string))
		return false;

	return MCStringFormat(r_string, "[%@]", *t_contents_string);
}

bool __MCProperListImmutableCopy(__MCProperList *self, bool p_release, __MCProperList*& r_immutable_self)
{
	if (!p_release)
		return MCProperListCopy(self, r_immutable_self);

	return MCProperListCopyAndRelease(self, r_immutable_self);
}

////////////////////////////////////////////////////////////////////////////////

static void __MCProperListClampRange(MCProperListRef self, MCRange& x_range)
{
	uindex_t t_left, t_right;
	t_left = MCMin(x_range . offset, self -> length);
	t_right = MCMin(x_range . offset + MCMin(x_range . length, UINDEX_MAX - x_range . offset), self -> length);
	x_range . offset = t_left;
	x_range . length = t_right - t_left;
}

static bool __MCProperListExpandAt(MCProperListRef self, uindex_t p_at, uindex_t p_count)
{
    MCAssert(!MCProperListIsIndirect(self));
    
    uindex_t t_old_length;
    t_old_length = self -> length;
    
    if (!MCMemoryResizeArray(self -> length + p_count, self -> list, self -> length))
        return false;
    
    MCMemoryMove(self -> list + p_at + p_count, self -> list + p_at, (t_old_length - p_at) * sizeof(MCValueRef));
    
    return true;
}

static bool __MCProperListShrinkAt(MCProperListRef self, uindex_t p_at, uindex_t p_count)
{
    MCAssert(!MCProperListIsIndirect(self));
    
    MCMemoryMove(self -> list + p_at, self -> list + p_at + p_count, (self -> length - (p_at + p_count)) * sizeof(MCValueRef));
    
    if (!MCMemoryResizeArray(self -> length - p_count, self -> list, self -> length))
        return false;
    
    return true;
}

static bool __MCProperListCreateIndirect(__MCProperList *p_contents, __MCProperList*& r_list)
{
	MCProperListRef self;
	if (!__MCValueCreate(kMCValueTypeCodeProperList, self))
		return false;

	self -> flags |= kMCProperListFlagIsMutable | kMCProperListFlagIsIndirect;
	self -> contents = MCValueRetain(p_contents);

	r_list = self;
	return true;
}


static bool __MCProperListIsIndirect(__MCProperList *self)
{
	return (self -> flags & kMCProperListFlagIsIndirect) != 0;
}

static bool __MCProperListMakeContentsImmutable(__MCProperList *self)
{
	for(uindex_t i = 0; i < self -> length; i++)
	{
        __MCValue *t_new_value;
        if (!__MCValueImmutableCopy((__MCValue *)self -> list[i], true, t_new_value))
            return false;

        self -> list[i] = t_new_value;
	}

	return true;
}

static bool __MCProperListMakeIndirect(__MCProperList *self)
{
	// If we are already indirect, there's nothing to do.
	if (__MCProperListIsIndirect(self))
		return true;

	// Our key-values are now all immutable, so create a new immutable list
	// with them.
	MCProperListRef t_list;
	if (!__MCValueCreate(kMCValueTypeCodeProperList, t_list))
		return false;

	// Fill in our new list.
	t_list -> length = self -> length;
	t_list -> list = self -> list;

	// 'self' now becomes indirect with a reference to the new list.
	self -> flags |= kMCProperListFlagIsIndirect;
	self -> contents = t_list;
	return true;
}

static bool __MCProperListResolveIndirect(__MCProperList *self)
{
	// Make sure we are indirect.
	MCAssert(__MCProperListIsIndirect(self));

	// Fetch the contents.
	MCProperListRef t_contents;
	t_contents = self -> contents;

	// If the contents only has a single reference, then re-absorb; otherwise
	// copy.
	if (self -> contents -> references == 1)
	{
		self -> length = t_contents -> length;
		self -> list = t_contents -> list;

		t_contents -> list = nil;
		t_contents -> length = 0;
	}
	else
	{
		uindex_t t_size;
		t_size = t_contents -> length;

		if (!MCMemoryNewArray(t_size, self -> list))
			return false;

		self -> length = t_contents -> length;

		for(uindex_t i = 0; i < t_size; i++)
            self -> list[i] = MCValueRetain(t_contents -> list[i]);
	}

	// Make sure the list is no longer marked as indirect.
	self -> flags &= ~kMCProperListFlagIsIndirect;

	// Destroy the contents.
	MCValueRelease(t_contents);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCProperListRef kMCEmptyProperList;

bool __MCProperListInitialize(void)
{
	if (!MCProperListCreate(nil, 0, kMCEmptyProperList))
		return false;

	return true;
}

void __MCProperListFinalize(void)
{
	MCValueRelease(kMCEmptyProperList);
}

////////////////////////////////////////////////////////////////////////////////

