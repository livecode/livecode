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

// Creates an indirect mutable array with contents.
static bool __MCArrayCreateIndirect(__MCArray *contents, __MCArray*& r_array);

// Returns the index of the table size of the array.
static uindex_t __MCArrayGetTableSizeIndex(__MCArray *self);

// Returns true if the array is indirect.
static bool __MCArrayIsIndirect(__MCArray *self);

// Replaces all the values in the array with immutable copies.
static bool __MCArrayMakeContentsImmutable(__MCArray *self);

// Creates an immutable array from this one, changing 'self' to indirect.
static bool __MCArrayMakeIndirect(__MCArray *self);

// Ensures the given mutable but indirect array is direct.
static bool __MCArrayResolveIndirect(__MCArray *self);

// Rehash the table adjusting capacity by delta.
static bool __MCArrayRehash(__MCArray *self, index_t by);

// Returns the number of entries in the key-value table for the array.
static uindex_t __MCArrayGetTableSize(__MCArray *self);

// Returns the maximum number of entries for a given array size that minimises rehashing.
static uindex_t __MCArrayGetTableCapacity(__MCArray *self);

// Looks for a key-value slot in the array with the given key. If the key was
// found 'true' is returned; otherwise 'false'. On return 'slot' will be the
// slot in which the key is found, could be placed, or UINDEX_MAX if there is
// no more room (and the key isn't there).
static bool __MCArrayFindKeyValueSlot(__MCArray *self, bool case_sensitive, MCNameRef key, uindex_t& r_slot);

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCArrayCreate(bool p_case_sensitive, const MCNameRef *p_keys, const MCValueRef *p_values, uindex_t p_length, MCArrayRef& r_array)
{
	if (p_length == 0)
	{
		if (nil != kMCEmptyArray)
		{
			r_array = MCValueRetain(kMCEmptyArray);
			return true;
		}
	}
	else
	{
		MCAssert(nil != p_keys);
		MCAssert(nil != p_values);
	}

	bool t_success;
	t_success = true;

	MCArrayRef t_array;
	t_array = nil;
	if (t_success)
		t_success = MCArrayCreateMutable(t_array);

	if (t_success)
		for(uindex_t i = 0; i < p_length && t_success; i++)
			t_success = MCArrayStoreValue(t_array, p_case_sensitive, p_keys[i], p_values[i]);

	if (t_success)
		return MCArrayCopyAndRelease(t_array, r_array);

	MCValueRelease(t_array);
	return false;
}

MC_DLLEXPORT_DEF
bool MCArrayCreateMutable(MCArrayRef& r_array)
{
	if (!__MCValueCreate(kMCValueTypeCodeArray, r_array))
		return false;

	r_array -> flags |= kMCArrayFlagIsMutable;

	return true;
}	

MC_DLLEXPORT_DEF
bool MCArrayCopy(MCArrayRef self, MCArrayRef& r_new_array)
{
	__MCAssertIsArray(self);

	// If we aren't mutable, then we can just copy directly.
	if (!MCArrayIsMutable(self))
	{
		r_new_array = MCValueRetain(self);
		return true;
	}

	// If we mutable, but still indirect just take the contents array.
	if (__MCArrayIsIndirect(self))
	{
		r_new_array = MCValueRetain(self -> contents);
		return true;
	}

	// Make the contents immutable.
	if (!__MCArrayMakeContentsImmutable(self))
		return false;

	// Make the array indirect.
	if (!__MCArrayMakeIndirect(self))
		return false;

	// Return a copy of the contents.
	r_new_array = MCValueRetain(self -> contents);
	return true;
}

MC_DLLEXPORT_DEF
bool MCArrayCopyAndRelease(MCArrayRef self, MCArrayRef& r_new_array)
{
	__MCAssertIsArray(self);

	// If we aren't mutable, then new array is just us.
	if (!MCArrayIsMutable(self))
	{
		r_new_array = self;
		return true;
	}

	// If we are indirect, then new array is the contents, we are released.
	if (__MCArrayIsIndirect(self))
	{
		r_new_array = MCValueRetain(self -> contents);
		MCValueRelease(self);
		return true;
	}

	// We need to make an immutable copy, so first change all our contents
	// to immutable.
	if (!__MCArrayMakeContentsImmutable(self))
		return false;

	// If we have a reference count of one 'self' becomes the immutable copy.
	if (self -> references == 1)
	{
		// We are no longer immutable.
		self -> flags &= ~kMCArrayFlagIsMutable;

		// Return this as the new array.
		r_new_array = self;
		return true;
	}

	// Otherwise we must build a new indirect value.
	if (!__MCArrayMakeIndirect(self))
		return false;

	// Reduce our reference count.
	self -> references -= 1;

	// Return a copy of the contents.
	r_new_array = MCValueRetain(self -> contents);
	return true;
}

MC_DLLEXPORT_DEF
bool MCArrayMutableCopy(MCArrayRef self, MCArrayRef& r_new_array)
{
	__MCAssertIsArray(self);

	// If the array is immutable, then the new mutable array will be indirect
	// referencing it. [ non-mutable arrays cannot be indirect so self does not
	// need resolving ].
	if (!MCArrayIsMutable(self))
		return __MCArrayCreateIndirect(self, r_new_array);

	// If the array is already indirect, we just create a new reference to the
	// same contents.
	if (__MCArrayIsIndirect(self))
		return __MCArrayCreateIndirect(self -> contents, r_new_array);

	// If the array is mutable, we make it immutable and indirect and share
	// the indirect copy.
	if (!__MCArrayMakeContentsImmutable(self))
		return false;

	// Make self indirect, and fetch its contents.
	if (!__MCArrayMakeIndirect(self))
		return false;

	// Finally, create a new indirect array with contents of self.
	return __MCArrayCreateIndirect(self -> contents, r_new_array);
}

MC_DLLEXPORT_DEF
bool MCArrayMutableCopyAndRelease(MCArrayRef self, MCArrayRef& r_new_array)
{
	__MCAssertIsArray(self);

	if (self -> references == 1)
	{
		if (!MCArrayIsMutable(self))
			self -> flags |= kMCArrayFlagIsMutable;

		r_new_array = self;
		return true;
	}

	if (!MCArrayMutableCopy(self, r_new_array))
		return false;

	self -> references -= 1;
	return true;
}

MC_DLLEXPORT_DEF
bool MCArrayApply(MCArrayRef self, MCArrayApplyCallback p_callback, void *p_context)
{
	__MCAssertIsArray(self);
	MCAssert(nil != p_callback);

	// Make sure we are iterating over the correct contents.
	MCArrayRef t_contents;
	if (!__MCArrayIsIndirect(self))
		t_contents = self;
	else
		t_contents = self -> contents;

	uindex_t t_used;
	t_used = t_contents -> key_value_count;

	uindex_t t_count;
	t_count = __MCArrayGetTableSize(t_contents);
	for(uindex_t i = 0; t_used > 0 && i < t_count; i++)
	{
		if (t_contents -> key_values[i] . value == UINTPTR_MIN || t_contents -> key_values[i] . value == UINTPTR_MAX)
			continue;

		if (!p_callback(p_context, self, t_contents -> key_values[i] . key, (MCValueRef)t_contents -> key_values[i] . value))
			return false;

		t_used -= 1;
	}

	return true;
}

MC_DLLEXPORT_DEF
bool MCArrayIterate(MCArrayRef self, uintptr_t& x_iterator, MCNameRef& r_key, MCValueRef& r_value)
{
	__MCAssertIsArray(self);

	// Make sure we are iterating over the correct contents.
	MCArrayRef t_contents;
	if (!__MCArrayIsIndirect(self))
		t_contents = self;
	else
		t_contents = self -> contents;

	uindex_t t_count;
	t_count = __MCArrayGetTableSize(t_contents);
	if (x_iterator == t_count)
		return false;

	for(uindex_t i = x_iterator; i < t_count; i += 1)
	{
		x_iterator += 1;
		if (t_contents -> key_values[i] . value != UINTPTR_MIN && t_contents -> key_values[i] . value != UINTPTR_MAX)
		{
			r_key = t_contents -> key_values[i] . key;
			r_value = (MCValueRef)t_contents -> key_values[i] . value;
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCArrayIsMutable(MCArrayRef self)
{
	__MCAssertIsArray(self);

	return (self -> flags & kMCArrayFlagIsMutable) != 0;
}

MC_DLLEXPORT_DEF
uindex_t MCArrayGetCount(MCArrayRef self)
{
	__MCAssertIsArray(self);

	if (!__MCArrayIsIndirect(self))
		return self -> key_value_count;
	return self -> contents -> key_value_count;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCArrayFetchValue(MCArrayRef self, bool p_case_sensitive, MCNameRef p_key, MCValueRef& r_value)
{
	return MCArrayFetchValueOnPath(self, p_case_sensitive, &p_key, 1, r_value);
}

MC_DLLEXPORT_DEF
bool MCArrayFetchValueOnPath(MCArrayRef self, bool p_case_sensitive, const MCNameRef *p_path, uindex_t p_path_length, MCValueRef& r_value)
{
	__MCAssertIsArray(self);
	MCAssert(nil != p_path);
	MCAssert(0 < p_path_length);
	__MCAssertIsName(p_path[0]);

	// If the array is indirect, get the contents.
	MCArrayRef t_contents;
	if (!__MCArrayIsIndirect(self))
		t_contents = self;
	else
		t_contents = self -> contents;

	// Lookup the slot for the first part of the path.
	uindex_t t_slot;
	if (!__MCArrayFindKeyValueSlot(t_contents, p_case_sensitive, p_path[0], t_slot))
		return false;

	// We found a slot successfully matching the key so get the value.
	MCValueRef t_value;
	t_value = (MCValueRef)t_contents -> key_values[t_slot] . value;

	// If the path length is one, then we are done.
	if (p_path_length == 1)
	{
		r_value = t_value;
		return true;
	}

	// If the value isn't an array then we can't continue with the lookup.
	if (MCValueGetTypeCode(t_value) != kMCValueTypeCodeArray)
		return false;

	// Otherwise, look up the next step in the path.
	return MCArrayFetchValueOnPath((MCArrayRef)t_value, p_case_sensitive, p_path + 1, p_path_length - 1, r_value);
}

bool MCArrayFetchValueOnPath(MCArrayRef array,
                             bool case_sensitive,
                             const MCSpan<MCNameRef> path,
                             MCValueRef& value)
{
    return MCArrayFetchValueOnPath(array, case_sensitive,
                                   path.data(), path.size(),
                                   value);
}

//////////////////////

MC_DLLEXPORT_DEF
bool MCArrayStoreValue(MCArrayRef self, bool p_case_sensitive, MCNameRef p_key, MCValueRef p_value)
{
	return MCArrayStoreValueOnPath(self, p_case_sensitive, &p_key, 1, p_value);
}

MC_DLLEXPORT_DEF
bool MCArrayStoreValueOnPath(MCArrayRef self, bool p_case_sensitive, const MCNameRef *p_path, uindex_t p_path_length, MCValueRef p_new_value)
{
	// The array must be mutable.
	MCAssert(MCArrayIsMutable(self));
	MCAssert(nil != p_path);
	MCAssert(0 < p_path_length);
	__MCAssertIsName(p_path[0]);

	// Ensure it is not indirect.
	if (__MCArrayIsIndirect(self))
		if (!__MCArrayResolveIndirect(self))
			return false;

	// Lookup the slot for the first element in the path.
	bool t_found;
	uindex_t t_slot;
	t_found = __MCArrayFindKeyValueSlot(self, p_case_sensitive, p_path[0], t_slot);
	if (t_found)
	{
		// Get the value.
		MCValueRef t_value;
		t_value = (MCValueRef)self -> key_values[t_slot] . value;

		// If the path length is 1, then just set the value and return.
		if (p_path_length == 1)
		{
			MCValueRelease(t_value);
			self -> key_values[t_slot] . value = (uintptr_t)MCValueRetain(p_new_value);
			return true;
		}

		// If the value is an array then recurse.
		if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeArray)
		{
			// If the array isn't mutable, then make a mutable copy replacing the existing
			// value.
			MCArrayRef t_mutable_array;
			if (!MCArrayIsMutable((MCArrayRef)t_value))
			{
				if (!MCArrayMutableCopyAndRelease((MCArrayRef)t_value, t_mutable_array))
					return false;
				self -> key_values[t_slot] . value = (uintptr_t)t_mutable_array;
			}
			else
				t_mutable_array = (MCArrayRef)t_value;

			return MCArrayStoreValueOnPath(t_mutable_array, p_case_sensitive, p_path + 1, p_path_length - 1, p_new_value);
		}
	}
	else
	{
        // AL-2014-07-15: [[ Bug 12532 ]] Rehash according to hash table capacities rather than sizes
		if (t_slot == UINDEX_MAX || self -> key_value_count >= __MCArrayGetTableCapacity(self))
		{
			if (!__MCArrayRehash(self, 1))
				return false;

			__MCArrayFindKeyValueSlot(self, p_case_sensitive, p_path[0], t_slot);
		}

		if (p_path_length == 1)
		{
			self -> key_values[t_slot] . key = MCValueRetain(p_path[0]);
			self -> key_values[t_slot] . value = (uintptr_t)MCValueRetain(p_new_value);
			self -> key_value_count += 1;
			return true;
		}
	}

	// If the value isn't an array, then create one.
	MCArrayRef t_array;
	if (!MCArrayCreateMutable(t_array))
		return false;

	// Build up the new array's value.
	if (!MCArrayStoreValueOnPath(t_array, p_case_sensitive, p_path + 1, p_path_length - 1, p_new_value))
	{
		MCValueRelease(t_array);
		return false;
	}

	// We've successfully built the rest of the path, so replace or add the new key-value.
	if (t_found)
		MCValueRelease((MCValueRef)self -> key_values[t_slot] . value);
	else
	{
		self -> key_values[t_slot] . key = MCValueRetain(p_path[0]);
		self -> key_value_count += 1;
	}
	
	self -> key_values[t_slot] . value = (uintptr_t)t_array;

	return true;
}

bool MCArrayStoreValueOnPath(MCArrayRef array,
                             bool case_sensitive,
                             const MCSpan<MCNameRef> path,
                             MCValueRef value)
{
    return MCArrayStoreValueOnPath(array, case_sensitive,
                                   path.data(), path.size(),
                                   value);
}

//////////////////////

MC_DLLEXPORT_DEF
bool MCArrayRemoveValue(MCArrayRef self, bool p_case_sensitive, MCNameRef p_key)
{
	return MCArrayRemoveValueOnPath(self, p_case_sensitive, &p_key, 1);
}

MC_DLLEXPORT_DEF
bool MCArrayRemoveValueOnPath(MCArrayRef self, bool p_case_sensitive, const MCNameRef *p_path, uindex_t p_path_length)
{
	// The array must be mutable.
	MCAssert(MCArrayIsMutable(self));
	MCAssert(nil != p_path);
	MCAssert(0 < p_path_length);

	// Ensure it is not indirect.
	if (__MCArrayIsIndirect(self))
		if (!__MCArrayResolveIndirect(self))
			return false;

	// Look up the first slot in the path.
	uindex_t t_slot;
	if (__MCArrayFindKeyValueSlot(self, p_case_sensitive, p_path[0], t_slot))
	{
		MCValueRef t_value;
		t_value = (MCValueRef)self -> key_values[t_slot] . value;

		// If the path length is one, then just remove the key.
		if (p_path_length == 1)
		{
			MCValueRelease(self -> key_values[t_slot] . key);
			MCValueRelease(t_value);

			self -> key_values[t_slot] . key = nil;
			self -> key_values[t_slot] . value = UINTPTR_MAX;
			self -> key_value_count -= 1;

			if (__MCArrayGetTableSizeIndex(self) > 2 &&
				self -> key_value_count < __kMCValueHashTableCapacities[__MCArrayGetTableSizeIndex(self) - 2])
				__MCArrayRehash(self, -1);

			return true;
		}

		// If the value is an array then recurse.
		if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeArray)
		{
			// If the array isn't mutable, then make a mutable copy replacing the existing
			// value.
			MCArrayRef t_mutable_array;
            if (!MCArrayIsMutable((MCArrayRef)t_value))
			{
				if (!MCArrayMutableCopyAndRelease((MCArrayRef)t_value, t_mutable_array))
					return false;
				self -> key_values[t_slot] . value = (uintptr_t)t_mutable_array;
			}
			else
				t_mutable_array = (MCArrayRef)t_value;

			return MCArrayRemoveValueOnPath(t_mutable_array, p_case_sensitive, p_path + 1, p_path_length - 1);
		}
	}

	// Otherwise there is nothing more to do.
	return true;
}

bool MCArrayRemoveValueOnPath(MCArrayRef array,
                              bool case_sensitive,
                              MCSpan<MCNameRef> path)
{
    return MCArrayRemoveValueOnPath(array, case_sensitive,
                                    path.data(), path.size());
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCArrayFetchValueAtIndex(MCArrayRef self, index_t p_index, MCValueRef& r_value)
{
    __MCAssertIsArray(self);
    
    MCNameRef t_key =
            MCNameLookupIndex(p_index);
    
    if (t_key == nil)
    {
        return false;
    }
    
    return MCArrayFetchValue(self, true, t_key, r_value);
}

MC_DLLEXPORT_DEF
bool MCArrayStoreValueAtIndex(MCArrayRef self, index_t p_index, MCValueRef p_value)
{
    __MCAssertIsArray(self);
    
    MCNewAutoNameRef t_key;
    if (!MCNameCreateWithIndex(p_index,
                               &t_key))
    {
        return false;
    }
    
    return MCArrayStoreValue(self, true, *t_key, p_value);
}

bool
MCArrayRemoveValueAtIndex(MCArrayRef self, index_t p_index)
{
    __MCAssertIsArray(self);
    
    MCNameRef t_key =
            MCNameLookupIndex(p_index);
    
    if (t_key == nil)
    {
        return true;
    }
    
    return MCArrayRemoveValue(self, true, t_key);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCArrayIsEmpty(MCArrayRef self)
{
	return MCArrayGetCount(self) == 0;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCArrayConvertToProperList(MCArrayRef p_array, MCProperListRef& r_list)
{
    MCAutoProperListRef t_list;
    if (!MCProperListCreateMutable(&t_list))
    {
        return false;
    }
    
    for(uindex_t t_index = 1; t_index <= MCArrayGetCount(p_array); t_index++)
    {
        MCValueRef t_value;
        if (!MCArrayFetchValueAtIndex(p_array, t_index, t_value))
        {
            r_list = nullptr;
            return true;
        }
        
        if (!MCProperListPushElementOntoBack(*t_list, t_value))
        {
            return false;
        }
    }
    
    if (!t_list.MakeImmutable())
    {
        return false;
    }
    
    r_list = t_list.Take();
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void __MCArrayDestroy(__MCArray *self)
{
	if (__MCArrayIsIndirect(self))
		MCValueRelease(self -> contents);
	else
	{
		uindex_t t_used;
		t_used = self -> key_value_count;

		uindex_t t_count;
		t_count = __MCArrayGetTableSize(self);
		for(uindex_t i = 0; t_used > 0 && i < t_count; i++)
		{
			if (self -> key_values[i] . value == UINTPTR_MIN || self -> key_values[i] . value == UINTPTR_MAX)
				continue;

			MCValueRelease(self -> key_values[i] . key);
			MCValueRelease((MCValueRef)self -> key_values[i] . value);

			t_used -= 1;
		}

		MCMemoryDeleteArray(self -> key_values);
	}
}

hash_t __MCArrayHash(__MCArray *self)
{
	return (hash_t)self -> key_value_count;
}

bool __MCArrayIsEqualTo(__MCArray *self, __MCArray *other_self)
{
	// If the array is indirect, get the contents.
	MCArrayRef t_contents;
	if (!__MCArrayIsIndirect(self))
		t_contents = self;
	else
		t_contents = self -> contents;

	// If the other array is indirect, get its contents.
	MCArrayRef t_other_contents;
	if (!__MCArrayIsIndirect(other_self))
		t_other_contents = other_self;
	else
		t_other_contents = other_self -> contents;

	if (t_contents -> key_value_count != t_other_contents -> key_value_count)
		return false;

	uindex_t t_used;
	t_used = t_contents -> key_value_count;

	uindex_t t_count;
	t_count = __MCArrayGetTableSize(t_contents);

	for(uindex_t i = 0; t_used > 0 && i < t_count; i++)
	{
		// If the given slot is not used, then skip it.
		if (t_contents -> key_values[i] . value == UINTPTR_MIN || t_contents -> key_values[i] . value == UINTPTR_MAX)
			continue;

		// If we don't find a key in the other array matching one in this then
		// the arrays aren't equal.
		uindex_t t_slot;
		if (!__MCArrayFindKeyValueSlot(t_other_contents, true, t_contents -> key_values[i] . key, t_slot))
			return false;

		// Otherwise, they are only equal if the values are the same.
		if (!MCValueIsEqualTo((MCValueRef)t_contents -> key_values[i] . value,
                              (MCValueRef)t_other_contents -> key_values[t_slot] . value))
			return false;

		// We've compared one more key, so used count goes down.
		t_used -= 1;
	}

	// If we get here it means all key/values match.
	return true;
}

bool __MCArrayCopyDescription(__MCArray *self, MCStringRef& r_string)
{
	/* Shortcut for empty arrays */
	if (MCArrayIsEmpty (self))
		return MCStringCopy (MCSTR("{}"), r_string);

	MCAutoListRef t_contents_list;
	if (!MCListCreateMutable (MCSTR(", "), &t_contents_list))
		return false;

	uintptr_t t_iter = 0;
	MCNameRef t_key;
	MCValueRef t_value;
	while (MCArrayIterate (self, t_iter, t_key, t_value))
	{
        // AL-2015-06-19:[[ Bug 15529 ]] Call MCValueCopyDescription to convert arbitrary array values to string
        MCAutoStringRef t_value_string;
        if (!MCValueCopyDescription(t_value, &t_value_string))
            return false;
		if (!MCListAppendFormat (*t_contents_list, "%@: %@", t_key, *t_value_string))
			return false;
	}

	MCAutoStringRef t_contents_string;
	if (!MCListCopyAsString (*t_contents_list, &t_contents_string))
		return false;

	return MCStringFormat(r_string, "{%@}", *t_contents_string);
}

bool __MCArrayImmutableCopy(__MCArray *self, bool p_release, __MCArray*& r_immutable_self)
{
	if (!p_release)
		return MCArrayCopy(self, r_immutable_self);

	return MCArrayCopyAndRelease(self, r_immutable_self);
}

////////////////////////////////////////////////////////////////////////////////

static bool __MCArrayCreateIndirect(__MCArray *p_contents, __MCArray*& r_array)
{
	MCArrayRef self;
	if (!__MCValueCreate(kMCValueTypeCodeArray, self))
		return false;

	self -> flags |= kMCArrayFlagIsMutable | kMCArrayFlagIsIndirect;
	self -> contents = MCValueRetain(p_contents);

	r_array = self;
	return true;
}

static uindex_t __MCArrayGetTableSizeIndex(__MCArray *self)
{
	return self -> flags & kMCArrayFlagCapacityIndexMask;
}

static void __MCArraySetTableSizeIndex(__MCArray *self, uindex_t p_new_index)
{
	self -> flags = (self -> flags & ~kMCArrayFlagCapacityIndexMask) | p_new_index;
}

static uindex_t __MCArrayGetTableSize(__MCArray *self)
{
	return __kMCValueHashTableSizes[self -> flags & kMCArrayFlagCapacityIndexMask];
}

static uindex_t __MCArrayGetTableCapacity(__MCArray *self)
{
	return __kMCValueHashTableCapacities[self -> flags & kMCArrayFlagCapacityIndexMask];
}

static bool __MCArrayIsIndirect(__MCArray *self)
{
	return (self -> flags & kMCArrayFlagIsIndirect) != 0;
}

static bool __MCArrayMakeContentsImmutable(__MCArray *self)
{
	uindex_t t_used, t_count;
	t_used = self -> key_value_count;
	t_count = __MCArrayGetTableSize(self);
	for(uindex_t i = 0; t_used > 0 && i < t_count; i++)
	{
		if (self -> key_values[i] . value != UINTPTR_MIN && self -> key_values[i] . value != UINTPTR_MAX)
		{
			__MCValue *t_new_value;
			if (!__MCValueImmutableCopy((__MCValue *)self -> key_values[i] . value, true, t_new_value))
				return false;

			self -> key_values[i] . value = (uintptr_t)t_new_value;
		}
	}

	return true;
}

static bool __MCArrayMakeIndirect(__MCArray *self)
{
	// If we are already indirect, there's nothing to do.
	if (__MCArrayIsIndirect(self))
		return true;

	// Our key-values are now all immutable, so create a new immutable array
	// with them.
	MCArrayRef t_array;
	if (!__MCValueCreate(kMCValueTypeCodeArray, t_array))
		return false;

	// Fill in our new array.
	t_array -> flags |= self -> flags & kMCArrayFlagCapacityIndexMask;
	t_array -> key_value_count = self -> key_value_count;
	t_array -> key_values = self -> key_values;

	// 'self' now becomes indirect with a reference to the new array.
	self -> flags |= kMCArrayFlagIsIndirect;
	self -> contents = t_array;
	return true;
}

static bool __MCArrayResolveIndirect(__MCArray *self)
{
	// Make sure we are indirect.
	MCAssert(__MCArrayIsIndirect(self));

	// Fetch the contents.
	MCArrayRef t_contents;
	t_contents = self -> contents;

	// If the contents only has a single reference, then re-absorb; otherwise
	// copy.
	if (self -> contents -> references == 1)
	{
		self -> key_values = t_contents -> key_values;
		self -> key_value_count = t_contents -> key_value_count;

		t_contents -> key_values = nil;
		t_contents -> key_value_count = 0;
	}
	else
	{
		uindex_t t_size;
		t_size = __MCArrayGetTableSize(t_contents);

		if (!MCMemoryNewArray(t_size, self -> key_values))
			return false;

		self -> key_value_count = t_contents -> key_value_count;

		for(uindex_t i = 0; i < t_size; i++)
		{
			if (t_contents -> key_values[i] . value != UINTPTR_MIN && t_contents -> key_values[i] . value != UINTPTR_MAX)
			{
				self -> key_values[i] . value = (uintptr_t)MCValueRetain((MCValueRef)t_contents -> key_values[i] . value);
				self -> key_values[i] . key = MCValueRetain(t_contents -> key_values[i] . key);
			}
            else
            {
                // Ensure we don't break any hash chains
                self -> key_values[i] = t_contents -> key_values[i];
            }
		}
	}

	// Make sure we take the index from the flags.
	__MCArraySetTableSizeIndex(self, __MCArrayGetTableSizeIndex(t_contents));

	// Make sure the array is no longer marked as indirect.
	self -> flags &= ~kMCArrayFlagIsIndirect;

	// Destroy the contents.
	MCValueRelease(t_contents);

	return true;
}

static bool __MCArrayFindKeyValueSlot(__MCArray *self, bool p_case_sensitive, MCNameRef p_key, uindex_t& r_slot)
{
	// Get the table size.
	uindex_t t_size;
	t_size = __MCArrayGetTableSize(self);
	if (t_size == 0 || self -> key_values == nil)
	{
		r_slot = UINDEX_MAX;
		return false;
	}

	// Get the hash.
	uindex_t t_hash;
	t_hash = MCValueHash(p_key);

	// Fold the hash code appropriately.
	uindex_t t_h1;
#if defined(__ARM__) && 0 // TODO	
	t_h1 = __MCHashFold(t_hash, __MCArrayGetTableSizeIndex(self));
#else
	t_h1 = t_hash % t_size;
#endif

	// The initial index to probe.
	uindex_t t_probe;
	t_probe = t_h1;

	// The target for a new entry - if it ends up being UINDEX_MAX it means the
	// table is full.
	uindex_t t_target_slot;
	t_target_slot = UINDEX_MAX;

	// Loop over all key value pairs - starting at probe.
	for(uindex_t i = 0; i < t_size; i++)
	{
		__MCArrayKeyValue *t_entry;
		t_entry = &self -> key_values[t_probe];

		if (t_entry -> value == UINTPTR_MIN)
		{
			// The end of the chain - we are done.
			if (t_target_slot == UINDEX_MAX)
				t_target_slot = t_probe;
			r_slot = t_probe;
			return false;
		}

		if (t_entry -> value == UINTPTR_MAX)
		{
			if (t_target_slot == UINDEX_MAX)
				t_target_slot = t_probe;
		}
		else
		{
            if (MCNameIsEqualTo(t_entry -> key, p_key, !p_case_sensitive ? kMCStringOptionCompareCaseless : kMCStringOptionCompareExact))
			{
				r_slot = t_probe;
				return true;
			}
		}

		t_probe += 1;
		if (t_size <= t_probe)
			t_probe -= t_size;
	}

	// If we get here the name wasn't found.
	r_slot = t_target_slot;
	return false;
}

static bool __MCArrayRehash(__MCArray *self, index_t p_by)
{
	uindex_t t_new_capacity_idx;
	t_new_capacity_idx = __MCArrayGetTableSizeIndex(self);
	if (p_by != 0)
	{
		if (p_by < 0)
			p_by = 0;

		uindex_t t_new_capacity_req;
		t_new_capacity_req = self -> key_value_count + p_by;
		for(t_new_capacity_idx = 0;
		    t_new_capacity_req > __kMCValueHashTableCapacities[t_new_capacity_idx];
		    ++t_new_capacity_idx);
	}

	uindex_t t_old_capacity;
	__MCArrayKeyValue *t_old_key_values;
	t_old_capacity = __MCArrayGetTableSize(self);
	t_old_key_values = self -> key_values;

	uindex_t t_new_capacity;
	__MCArrayKeyValue *t_new_key_values;
	t_new_capacity = __kMCValueHashTableSizes[t_new_capacity_idx];
	if (!MCMemoryNewArray(t_new_capacity, t_new_key_values))
		return false;

	__MCArraySetTableSizeIndex(self, t_new_capacity_idx);
	self -> key_values = t_new_key_values;

	for(uindex_t i = 0; i < t_old_capacity; i++)
	{
		if (t_old_key_values[i] . value != UINTPTR_MIN && t_old_key_values[i] . value != UINTPTR_MAX)
		{
			uindex_t t_target_slot;
			__MCArrayFindKeyValueSlot(self, true, t_old_key_values[i] . key, t_target_slot);

			MCAssert(t_target_slot != UINDEX_MAX);

			t_new_key_values[t_target_slot] = t_old_key_values[i];
		}
	}

	MCMemoryDeleteArray(t_old_key_values);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCArrayRef kMCEmptyArray;

bool __MCArrayInitialize(void)
{
	if (!MCArrayCreate(false, nil, nil, 0, kMCEmptyArray))
		return false;

	return true;
}

void __MCArrayFinalize(void)
{
	MCValueRelease(kMCEmptyArray);
}

////////////////////////////////////////////////////////////////////////////////

void __MCArrayDump(MCArrayRef array)
{
	// If the array is indirect, get the contents.
	MCArrayRef t_contents;
	if (!__MCArrayIsIndirect(array))
		t_contents = array;
	else
		t_contents = array -> contents;

	uindex_t t_size;
	t_size = __MCArrayGetTableSize(t_contents);

	for(uindex_t i = 0; i < t_size; i++)
	{
		__MCArrayKeyValue *t_entry;
		t_entry = &t_contents -> key_values[i];

		if (t_entry -> value != UINTPTR_MIN && t_entry -> value != UINTPTR_MAX)
		{
			MCNameRef t_key;
			t_key = t_entry -> key;

			MCValueRef t_value;
			t_value = (MCValueRef)t_entry -> value;
			
			if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeArray)
			{
                MCLog("[%@]:", t_key);
				__MCArrayDump((MCArrayRef)t_value);
			}
			else
			{
				MCAutoStringRef t_desc;
				MCValueCopyDescription(t_value, &t_desc);
                MCLog("[%@] = %@", t_key, *t_desc);
			}
		}
	}
}

/*
bool MCArrayFetchValue(MCArrayRef self, bool p_case_sensitive, const MCNameRef *p_path, uindex_t p_path_length, MCValueRef& r_value)
{
	MCArrayRef t_container;
	if (p_path_length == 1)
		t_container = self;
	else
		t_container = __MCArrayResolvePath(self, p_case_sensitive, p_path, p_path_length - 1);

	uindex_t t_slot;
	if (__MCArrayFindKeyValueSlotForFetch(t_container, p_case_sensitive, p_path[p_path_length - 1], t_slot))
	{
		r_value = (MCValueRef)t_container -> key_values[t_slot] . value;
		return true;
	}

	return false;
}

bool MCArrayStoreValue(MCArrayRef self, bool p_case_sensitive, const MCNameRef *p_path, uindex_t p_path_length, MCValueRef p_value)
{
	MCArrayRef t_container;
	if (p_path_length == 1)
		t_container = self;
	else if (!__MCArrayResolveMutablePath(self, p_case_sensitive, p_path, p_path_length - 1, t_container))
		return false;

	uindex_t t_slot;
	if (__MCArrayFindKeyValueSlotForStore(t_container, p_case_sensitive, p_path[p_path_length - 1], t_slot))
	{
		uintptr_t t_value_ptr;
		t_value_ptr = t_container -> key_values[t_slot] . value;
		
		MCValueRetain(p_value);
		if (t_value_ptr != UINTPTR_MIN && t_value_ptr != UINTPTR_MAX)
		{
			MCValueRelease((MCValueRef)t_value_ptr);
			t_container -> key_values[t_slot] . value = p_value;
		}
		else
		{
			t_container -> key_values[t_slot] . name = MCValueRetain(p_path[p_path_length - 1]);
			t_container -> key_values[t_slot] . value = (uintptr_t)p_value;
			t_container -> key_value_count += 1;
		}
	}
	else
		return false;

	return true;
}

bool MCArrayRemoveValue(MCArrayRef self, bool p_case_sensitive, const MCNameRef *p_path, uindex_t p_path_length)
{
	MCArrayRef t_container;
	if (p_path_length == 1)
		t_container = self;
	else if (!__MCArrayResolveMutablePath(self, p_case_sensitive, p_path, p_path_length - 1, t_container))
		return false;

	uindex_t t_slot;
	if (__MCArrayFindKeyValueSlotForRemove(t_container, p_case_sensitive, p_path[p_path_length - 1], t_slot))
	{
		MCValueRelease(t_container -> key_values[t_slot] . key);
		MCValueRelease((MCValueRef)t_container -> key_values[t_slot] . value);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

static MCArrayRef __MCArrayResolvePath(MCArrayRef self, bool p_case_sensitive, const MCNameRef *p_path, uindex_t p_path_length)
{
	MCArrayRef t_container;
	t_container = self;
	for(uindex_t i = 0; i < p_path_length)
	{
		uindex_t t_slot;
		if (__MCArrayFindKeyValueSlotForFetch(t_container, p_case_sensitive, p_path[i], t_slot))
		{
			MCValueRef t_value;
			t_value = (MCValueRef)t_container -> key_values[t_slot] . value;
			if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeArray)
			{
				t_container = (MCArrayRef)t_value;
				continue;
			}
		}

		// If we get here then either the key wasn't found, or the value found
		// was not an array - either way the path resolves to empty.
		break;
	}

	return t_container;
}

static bool __MCArrayResolveMutablePath(MCArrayRef self, bool p_case_sensitive, const MCNameRef *p_path, uindex_t p_path_length, MCArrayRef& r_array)
{
	MCArrayRef t_container;
	t_container = self;
	for(t_path_index = 0; t_path_index < p_path_length; t_path_index++)
	{
		MCArrayRef t_next_array;

		// Lookup the slot for the given key.
		uindex_t t_slot;
		if (__MCArrayFindKeyValueSlotForStore(t_container, p_case_sensitive, p_path[t_path_index], t_slot))
		{
			uintptr_t t_value_ptr;
			t_value_ptr = t_container -> key_values[t_slot] . value;

			if (t_value_ptr != UINTPTR_MIN && t_value_ptr != UINTPTR_MAX)
			{
				// At this point we have a slot and it has a value within it. So fetch it.
				MCValueRef t_value;
				t_value = (MCValueRef)t_container -> key_values[t_slot] . value;

				// If the value is already an array then must ensure it is mutable; otherwise
				// we must change it to an array.
				if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeArray)
				{
					if (MCArrayIsMutable((MCArrayRef)t_value))
						t_next_array = (MCArrayRef)t_value;
					else
					{
						if (MCArrayMutableCopyAndRelease((MCArrayRef)t_value, t_next_array))
							t_container -> key_values[t_slot] . value = (uintptr_t)t_next_array;
						else
							return false;
					}
				}
				else if (MCArrayCreateMutable(t_next_array))
				{
					MCValueRelease(t_value);
					t_container -> key_values[t_slot] . value = (uintptr_t)t_next_array;
				}
				else
					return false;
			}
			else
			{
				// At this point a new slot has been created but has yet to be
				// filled. So create a new array and fill it.
				if (MCArrayCreateMutable(t_next_array))
				{
					t_container -> key_values[t_slot] . key = MCValueRetain(p_path[t_path_index]);
					t_container -> key_values[t_slot] . value = (uintptr_t)t_next_array;
					t_container -> key_value_count += 1;
				}
				else
					return false;
			}
		}
		else
			return false;

		t_container = t_next_array;
	}

	// At this point 't_container' contains a mutable array for the last key
	// in the path.
	r_array = t_container;
	return true;
}*/
