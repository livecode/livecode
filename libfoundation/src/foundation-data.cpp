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

#include "foundation-private.h"



////////////////////////////////////////////////////////////////////////////////

// This method ensures there is 'count' bytes of empty space starting at 'at'
// in 'r_data'. It returns false if allocation fails.
static bool __MCDataExpandAt(MCDataRef r_data, uindex_t at, uindex_t count);

// This method removes 'count' bytes from byte sequence starting at 'at'.
static void __MCDataShrinkAt(MCDataRef r_data, uindex_t at, uindex_t count);

// This method clamps the given range to the valid limits for the byte sequence.
static void __MCDataClampRange(MCDataRef r_data, MCRange& x_range);

static bool __MCDataMakeImmutable(__MCData *self);

////////////////////////////////////////////////////////////////////////////////


bool MCDataCreateWithBytes(const byte_t *p_bytes, uindex_t p_byte_count, MCDataRef& r_data)
{
	bool t_success;
	t_success = true;
    
	__MCData *self;
	self = nil;
	if (t_success)
		t_success = __MCValueCreate(kMCValueTypeCodeData, self);
    
	if (t_success)
		t_success = MCMemoryNewArray(p_byte_count, self -> bytes);
    
	if (t_success)
	{
        MCMemoryCopy(self -> bytes, p_bytes, p_byte_count);
        self -> byte_count = p_byte_count;
		r_data = self;
	}
	else
	{
		if (self != nil)
			MCMemoryDeleteArray(self -> bytes);
        
		MCMemoryDelete(self);
	}
    
	return t_success;
}

bool MCDataCreateWithBytesAndRelease(byte_t *p_bytes, uindex_t p_byte_count, MCDataRef& r_data)
{
    if (MCDataCreateWithBytes(p_bytes, p_byte_count, r_data))
    {
        MCMemoryDeallocate(p_bytes);
        return true;
    }
    
    return false;
}

bool MCDataIsEmpty(MCDataRef p_data)
{
	return p_data -> byte_count == 0;
}

uindex_t MCDataGetLength(MCDataRef p_data)
{
    return p_data->byte_count;
}

const byte_t *MCDataGetBytePtr(MCDataRef p_data)
{
    return p_data->bytes;
}

byte_t MCDataGetByteAtIndex(MCDataRef p_data, uindex_t p_index)
{
    return p_data->bytes[p_index];
}

hash_t MCDataHash(MCDataRef p_data);
bool MCDataIsEqualTo(MCDataRef p_left, MCDataRef p_right)
{
    if (p_left -> byte_count != p_right -> byte_count)
        return false;
    
    return MCMemoryCompare(p_left -> bytes, p_right -> bytes, p_left -> byte_count) == 0;
}

compare_t MCDataCompareTo(MCDataRef p_left, MCDataRef p_right);

// Mutable data methods

bool MCDataCreateMutable(uindex_t p_initial_capacity, MCDataRef& r_data)
{
	bool t_success;
	t_success = true;
    
	__MCData *self;
	self = nil;
	if (t_success)
		t_success = __MCValueCreate(kMCValueTypeCodeData, self);
    
	if (t_success)
		t_success = __MCDataExpandAt(self, 0, p_initial_capacity);
    
	if (t_success)
	{
        self->flags |= kMCDataFlagIsMutable;
		r_data = self;
	}
    
	return t_success;
}

bool MCDataCopy(MCDataRef p_data, MCDataRef& r_new_data)
{
    if (!MCDataIsMutable(p_data))
    {
        MCValueRetain(p_data);
        r_new_data = p_data;
        return true;
    }
    
    return MCDataCreateWithBytes(p_data->bytes, p_data->byte_count, r_new_data);
}

bool MCDataCopyAndRelease(MCDataRef p_data, MCDataRef& r_new_data)
{
    // If the MCData is immutable we just pass it through (as we are releasing it).
    if (!MCDataIsMutable(p_data))
    {
        r_new_data = p_data;
        return true;
    }
    
    // If the reference is 1, convert it to an immutable MCData
    if (p_data->references == 1)
    {
        __MCDataMakeImmutable(p_data);
        p_data->flags &= ~kMCDataFlagIsMutable;
        r_new_data = p_data;
        return true;
    }
    
    // Otherwise make a copy of the data and then release the original
    bool t_success;
    t_success = MCDataCreateWithBytes(p_data->bytes, p_data->byte_count, r_new_data);
    MCValueRelease(p_data);
    
    return t_success;
}

bool MCDataMutableCopy(MCDataRef p_data, MCDataRef& r_mutable_data)
{
    MCDataRef t_mutable_data;
    if (!MCDataCreateMutable(p_data->byte_count, t_mutable_data))
        return false;
    
    MCMemoryCopy(t_mutable_data->bytes, p_data->bytes, p_data->byte_count);
    t_mutable_data->byte_count = p_data->byte_count;
    r_mutable_data = t_mutable_data;
    return true;
}

bool MCDataMutableCopyAndRelease(MCDataRef p_data, MCDataRef& r_mutable_data)
{
    if (MCDataMutableCopy(p_data, r_mutable_data))
    {
        MCValueRelease(p_data);
        return true;
    }
    
    return false;
}

bool MCDataCopyRange(MCDataRef self, MCRange p_range, MCDataRef& r_new_data)
{
    __MCDataClampRange(self, p_range);
    
    return MCDataCreateWithBytes(self -> bytes + p_range . offset, p_range . length, r_new_data);
}

bool MCDataCopyRangeAndRelease(MCDataRef self, MCRange p_range, MCDataRef& r_new_data)
{
    if (MCDataCopyRange(self, p_range, r_new_data))
    {
        MCValueRelease(self);
        return true;
    }
    
    return false;
}

bool MCDataIsMutable(const MCDataRef p_data)
{
    return (p_data->flags & kMCDataFlagIsMutable) != 0;
}

bool MCDataAppend(MCDataRef r_data, MCDataRef p_suffix)
{
    MCAssert(MCDataIsMutable(r_data));
    
    if (r_data == p_suffix)
    {
        // Must copy first the suffix
        MCDataRef t_suffix_copy;
        if(!MCDataCopy(r_data, t_suffix_copy))
            return false;
        
        // Copy succeeded: can process recursively the appending, and then release the newly created suffix.
        bool t_success = MCDataAppend(r_data, t_suffix_copy);
        MCValueRelease(t_suffix_copy);
        return t_success;
    }
    
    if (!__MCDataExpandAt(r_data, r_data->byte_count, p_suffix->byte_count))
        return false;
    
    MCMemoryCopy(r_data->bytes + r_data->byte_count - p_suffix->byte_count, p_suffix->bytes, p_suffix->byte_count);
    
    return true;
}

bool MCDataAppendBytes(MCDataRef r_data, const byte_t *p_bytes, uindex_t p_byte_count)
{
    MCAssert(MCDataIsMutable(r_data));
    
    // Expand the capacity if necessary
    if (!__MCDataExpandAt(r_data, r_data->byte_count, p_byte_count))
        return false;
    
    MCMemoryCopy(r_data->bytes + r_data->byte_count - p_byte_count, p_bytes, p_byte_count);

    return true;
}

bool MCDataAppendByte(MCDataRef r_data, byte_t p_byte)
{
    MCAssert(MCDataIsMutable(r_data));
    
    return MCDataAppendBytes(r_data, &p_byte, 1);
}

bool MCDataPrepend(MCDataRef r_data, MCDataRef p_prefix)
{
    MCAssert(MCDataIsMutable(r_data));
    
    if (r_data == p_prefix)
    {
        // Must copy first the prefix
        MCDataRef t_prefix_copy;
        if(!MCDataCopy(r_data, t_prefix_copy))
            return false;
        
        // Copy succeeded: can process recursively the appending, and then release the newly created suffix.
        bool t_success = MCDataPrepend(r_data, t_prefix_copy);
        MCValueRelease(t_prefix_copy);
        return t_success;
    }
    
    // Ensure there is room enough to copy the prefix
    if (!__MCDataExpandAt(r_data, 0, p_prefix->byte_count))
        return false;
    
    MCMemoryCopy(r_data->bytes, p_prefix->bytes, p_prefix->byte_count);

    return true;
}

bool MCDataPrependBytes(MCDataRef r_data, const byte_t *p_bytes, uindex_t p_byte_count)
{
    MCAssert(MCDataIsMutable(r_data));
        
    // Ensure there is room enough to copy the prefix
    if (!__MCDataExpandAt(r_data, 0, p_byte_count))
        return false;
    
    MCMemoryCopy(r_data->bytes, p_bytes, p_byte_count);
    
    return true;
}

bool MCDataPrependByte(MCDataRef r_data, byte_t p_byte)
{
    return MCDataPrependBytes(r_data, &p_byte, 1);
}

bool MCDataInsert(MCDataRef r_data, uindex_t p_at, MCDataRef p_new_data)
{
    MCAssert(MCDataIsMutable(r_data));
    
    if (r_data == p_new_data)
    {
        // Must copy first the prefix
        MCDataRef t_new_data_copy;
        if(!MCDataCopy(r_data, t_new_data_copy))
            return false;
        
        // Copy succeeded: can process recursively the appending, and then release the newly created suffix.
        bool t_success = MCDataPrepend(r_data, t_new_data_copy);
        MCValueRelease(t_new_data_copy);
        return t_success;
    }
    
    // Ensure there is room enough to copy the prefix
    if (!__MCDataExpandAt(r_data, p_at, p_new_data->byte_count))
        return false;
    
    MCMemoryCopy(r_data->bytes, p_new_data->bytes, p_new_data->byte_count);
    
    return true;
}

bool MCDataRemove(MCDataRef r_data, MCRange p_range)
{
	MCAssert(MCDataIsMutable(r_data));
    
	__MCDataClampRange(r_data, p_range);
    
	// Copy down the bytes above
	__MCDataShrinkAt(r_data, p_range.offset, p_range.length);
    
	// We succeeded.
	return true;
}

bool MCDataReplace(MCDataRef r_data, MCRange p_range, MCDataRef p_new_data)
{
	MCAssert(MCDataIsMutable(r_data));
    
	if (r_data == p_new_data)
	{
        MCDataRef t_new_data;
        
        if (!MCDataCopy(p_new_data, t_new_data))
            return false;
        
        bool t_success = MCDataReplace(r_data, p_range, t_new_data);
        MCValueRelease(t_new_data);
        return t_success;
    }
    
    __MCDataClampRange(r_data, p_range);
    
    // Work out the new size of the string.
    uindex_t t_new_char_count;
    t_new_char_count = r_data->byte_count - p_range.length + p_new_data->byte_count;
    
    if (t_new_char_count > r_data->byte_count)
    {
        // Expand the string at the end of the range by the amount extra we
        // need.
        if (!__MCDataExpandAt(r_data, p_range.offset + p_range.length, t_new_char_count - r_data->byte_count))
            return false;
    }
    else if (t_new_char_count < r_data->byte_count)
    {
        // Shrink the last part of the range by the amount less we need.
        __MCDataShrinkAt(r_data, p_range.offset + (p_range.length - (r_data->byte_count - t_new_char_count)), (r_data->byte_count - t_new_char_count));
    }
    
    // Copy across the replacement chars.
    MCMemoryCopy(r_data->bytes + p_range.offset, p_new_data->bytes, p_new_data->byte_count);
    
    // We succeeded.
    return true;
}

bool MCDataPad(MCDataRef p_data, byte_t p_byte, uindex_t p_count)
{
	if (!__MCDataExpandAt(p_data, p_data -> byte_count, p_count))
		return false;
	
	memset(p_data -> bytes + p_data -> byte_count - p_count, p_byte, p_count);
	return true;
}

compare_t MCDataCompareTo(MCDataRef p_left, MCDataRef p_right)
{
    compare_t t_result;
    t_result = memcmp(p_left -> bytes, p_right -> bytes, MCMin(p_left -> byte_count, p_right -> byte_count));
    
    if (t_result != 0)
        return t_result;
    
    return p_left -> byte_count - p_right -> byte_count;
}

bool MCDataContains(MCDataRef p_data, MCDataRef p_needle)
{
    uindex_t t_needle_byte_count, t_byte_count;
    t_needle_byte_count = p_needle -> byte_count;
    t_byte_count = p_data -> byte_count;
    
    if (t_needle_byte_count > t_byte_count)
        return false;
    
    const byte_t *t_bytes;
    t_bytes = p_data -> bytes;
    
    bool t_found = false;
    for (uindex_t i = 0; i < t_byte_count - t_needle_byte_count + 1; i++)
        if (MCMemoryCompare(t_bytes++, p_needle -> bytes, sizeof(byte_t) * t_needle_byte_count) == 0)
        {
            t_found = true;
            break;
        }
    
    return t_found;
}

bool MCDataBeginsWith(MCDataRef p_data, MCDataRef p_needle)
{
    uindex_t t_needle_byte_count, t_byte_count;
    t_needle_byte_count = p_needle -> byte_count;
    t_byte_count = p_data -> byte_count;
    
    if (t_needle_byte_count > t_byte_count)
        return false;
    
    return MCMemoryCompare(p_data -> bytes, p_needle -> bytes, sizeof(byte_t) * t_needle_byte_count) == 0;
}

bool MCDataEndsWith(MCDataRef p_data, MCDataRef p_needle)
{
    uindex_t t_needle_byte_count, t_byte_count;
    t_needle_byte_count = p_needle -> byte_count;
    t_byte_count = p_data -> byte_count;
    
    if (t_needle_byte_count > t_byte_count)
        return false;
    
    return MCMemoryCompare(p_data -> bytes + t_byte_count - t_needle_byte_count, p_needle -> bytes, sizeof(byte_t) * t_needle_byte_count) == 0;
}

bool MCDataFirstIndexOf(MCDataRef p_data, MCDataRef p_chunk, MCRange t_range, uindex_t& r_index)
{
    __MCDataClampRange(p_data, t_range);
    
    uindex_t t_limit, t_chunk_byte_count;
    t_chunk_byte_count = MCDataGetLength(p_chunk);
    t_limit = t_range . offset + t_range . length - t_chunk_byte_count + 1;
    
    const byte_t *t_bytes = MCDataGetBytePtr(p_data);
    const byte_t *t_chunk_bytes = MCDataGetBytePtr(p_chunk);
    
    uindex_t t_offset, t_result;
    t_result = 0;
    
    bool t_found;
    t_found = false;
    
    for (t_offset = t_range . offset; t_offset < t_limit; t_offset++)
        if (MCMemoryCompare(t_bytes + t_offset, t_chunk_bytes, sizeof(byte_t) * t_chunk_byte_count) == 0)
        {
            t_result = t_offset - t_range . offset;
            t_found = true;
            break;
        }
    
    r_index = t_result;
    return t_found;
}


bool MCDataLastIndexOf(MCDataRef p_data, MCDataRef p_chunk, MCRange t_range, uindex_t& r_index)
{
    __MCDataClampRange(p_data, t_range);
    
    uindex_t t_limit, t_chunk_byte_count;
    t_chunk_byte_count = MCDataGetLength(p_chunk);
    t_limit = t_range . offset + t_range . length - t_chunk_byte_count + 1;
    
    const byte_t *t_bytes = MCDataGetBytePtr(p_data);
    const byte_t *t_chunk_bytes = MCDataGetBytePtr(p_chunk);
    
    uindex_t t_offset, t_result;
    t_result = 0;
    
    bool t_found;
    t_found = false;
    
    while (--t_limit)
        if (MCMemoryCompare(t_bytes + t_limit, t_chunk_bytes, sizeof(byte_t) * t_chunk_byte_count) == 0)
        {
            t_result = t_limit - t_range . offset;
            t_found = true;
            break;
        }
    
    r_index = t_result;
    return t_found;
}


#if defined(__MAC__) || defined (__IOS__)
bool MCDataConvertToCFDataRef(MCDataRef p_data, CFDataRef& r_cfdata)
{
    r_cfdata = CFDataCreate(nil, MCDataGetBytePtr(p_data), MCDataGetLength(p_data));
    return r_cfdata != nil;
}
#endif

static void __MCDataClampRange(MCDataRef p_data, MCRange& x_range)
{
	uindex_t t_left, t_right;
	t_left = MCMin(x_range . offset, p_data -> byte_count);
	t_right = MCMin(x_range . offset + MCMin(x_range . length, UINDEX_MAX - x_range . offset), p_data->byte_count);
	x_range . offset = t_left;
	x_range . length = t_right - t_left;
}

static void __MCDataShrinkAt(MCDataRef r_data, uindex_t p_at, uindex_t p_count)
{
	// Shift the bytes above 'at' down to remove 'count'
	MCMemoryMove(r_data->bytes + p_at, r_data->bytes + (p_at + p_count), r_data->byte_count - (p_at + p_count));
    
	// Now adjust the length of the string.
	r_data->byte_count -= p_count;
    
	// TODO: Shrink the buffer if its too big.
}

static bool __MCDataExpandAt(MCDataRef r_data, uindex_t p_at, uindex_t p_count)
{
	// Fetch the capacity.
	uindex_t t_capacity;
	t_capacity = r_data->capacity;
    
	// The capacity field stores the total number of chars that could fit not
	// including the implicit NUL, so if we fit, we can fast-track.
	if (t_capacity != 0 && r_data->byte_count + p_count <= t_capacity)
	{
		// Shift up the bytes above
		MCMemoryMove(r_data->bytes + p_at + p_count,r_data->bytes + p_at, r_data->byte_count - p_at);
        
		// Increase the byte_count.
		r_data->byte_count += p_count;
        
		// We succeeded.
		return true;
	}
    
	// If we get here then we need to reallocate first.
    
	// Base capacity - current length + inserted length
	uindex_t t_new_capacity;
	t_new_capacity = r_data->byte_count + p_count;
    
	// Capacity rounded up to a suitable boundary (at some point this should
	// be a function of the string's size).
	t_new_capacity = (t_new_capacity + 63) & ~63;
    
    // Reallocate.
	if (!MCMemoryReallocate(r_data->bytes, t_new_capacity, r_data->bytes))
		return false;
    
	// Shift up the bytes above
	MCMemoryMove(r_data->bytes + p_at + p_count, r_data->bytes + p_at, r_data->byte_count - p_at);
    
	// Increase the byte_count.
	r_data->byte_count += p_count;
    
	// Update the capacity
	r_data -> capacity = t_new_capacity;
    
	// We succeeded.
	return true;
}

static bool __MCDataMakeImmutable(__MCData *self)
{
    if (!MCMemoryResizeArray(self -> byte_count, self -> bytes, self -> byte_count))
        return false;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MCDataRef kMCEmptyData;

bool __MCDataInitialize(void)
{
    if (!MCDataCreateWithBytes(nil, 0, kMCEmptyData))
        return false;
    
    return true;
}

void __MCDataFinalize(void)
{
    MCValueRelease(kMCEmptyData);
}

void __MCDataDestroy(__MCData *self)
{
    if (self -> bytes != nil)
        MCMemoryDeleteArray(self -> bytes);
}

bool __MCDataImmutableCopy(__MCData *self, bool p_release, __MCData *&r_immutable_value)
{
    if (!p_release)
        return MCDataCopy(self, r_immutable_value);
    return MCDataCopyAndRelease(self, r_immutable_value);
}

bool __MCDataIsEqualTo(__MCData *self, __MCData *p_other_data)
{
    return MCDataIsEqualTo(self, p_other_data);
}

hash_t __MCDataHash(__MCData *self)
{
    return MCHashBytes(self -> bytes, self -> byte_count);
}

bool __MCDataCopyDescription(__MCData *self, MCStringRef &r_description)
{
    return false;
}
