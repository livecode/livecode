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

#include "foundation-private.h"


////////////////////////////////////////////////////////////////////////////////

// An array of all possible (256) single byte DataRefs.
static MCDataRef *__kMCSingleBytes;

////////////////////////////////////////////////////////////////////////////////

// This method ensures there is 'count' bytes of empty space starting at 'at'
// in 'r_data'. It returns false if allocation fails.
static bool __MCDataExpandAt(MCDataRef r_data, uindex_t at, uindex_t count);

// This method removes 'count' bytes from byte sequence starting at 'at'.
static void __MCDataShrinkAt(MCDataRef r_data, uindex_t at, uindex_t count);

// This method clamps the given range to the valid limits for the byte sequence.
static void __MCDataClampRange(MCDataRef r_data, MCRange& x_range);

static bool __MCDataMakeImmutable(__MCData *self);

// Creates an indirect mutable data ref with contents.
static bool __MCDataCreateIndirect(__MCData *contents, __MCData*& r_string);

// Returns true if the data ref is indirect.
static bool __MCDataIsIndirect(__MCData *self);

// Creates an immutable data ref from this one, changing 'self' to indirect.
static bool __MCDataMakeIndirect(__MCData *self);

// Ensures the given mutable but indirect data ref is direct.
static bool __MCDataResolveIndirect(__MCData *self);

// Makes direct mutable data ref indirect, referencing r_new_data.
static bool __MCDataCopyMutable(__MCData *self, __MCData*& r_new_data);

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCDataCreateWithBytes(const byte_t *p_bytes, uindex_t p_byte_count, MCDataRef& r_data)
{
	/* Special case for empty data */
	if ((p_byte_count == 0 || nil == p_bytes) && nil != kMCEmptyData)
	{
		r_data = MCValueRetain(kMCEmptyData);
		return true;
	}

	/* Check that if the byte count is non-zero, the source pointer is valid */
	MCAssert(p_byte_count == 0 || nil != p_bytes);

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
		if (nil != p_bytes)
		{
			MCMemoryCopy(self -> bytes, p_bytes, p_byte_count);
		}
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

MC_DLLEXPORT_DEF
bool MCDataCreateWithBytesAndRelease(byte_t *p_bytes, uindex_t p_byte_count, MCDataRef& r_data)
{
	MCAssert(nil != p_bytes);

    // AL-2014-11-17: Create with bytes and release should just take the bytes.
    
    bool t_success;
    t_success = true;
    
    __MCData *self;
    self = nil;
    if (t_success)
        t_success = __MCValueCreate(kMCValueTypeCodeData, self);
    
    if (t_success)
    {
        self -> bytes = p_bytes;
        self -> byte_count = p_byte_count;
        r_data = self;
    }
    else
        MCMemoryDelete(self);
    
    return t_success;
}

MC_DLLEXPORT_DEF
bool
MCDataCreateWithData(MCDataRef& r_data, MCDataRef p_one, MCDataRef p_two)
{
	__MCAssertIsData(p_one);
	__MCAssertIsData(p_two);

    // Resolve any indirection
    if (__MCDataIsIndirect(p_one))
        p_one = p_one->contents;
    if (__MCDataIsIndirect(p_two))
        p_two = p_two->contents;
    
    // Calculate how much output space is required
    uindex_t t_length = p_one->byte_count + p_two->byte_count;
    
    // Create the new data object
    __MCData *self = nil;
    if (!__MCValueCreate(kMCValueTypeCodeData, self))
        return false;
    
    // Allocate memory for the output
    if (!MCMemoryNewArray(t_length, self->bytes))
    {
        MCValueRelease(self);
        return false;
    }
    
    // Copy the bytes to the object
    MCMemoryCopy(self->bytes, p_one->bytes, p_one->byte_count);
    MCMemoryCopy(self->bytes + p_one->byte_count, p_two->bytes, p_two->byte_count);
    
    // Set the byte count
    self->byte_count = p_one->byte_count + p_two->byte_count;
    
    // Done
    r_data = self;
    return true;
}

MC_DLLEXPORT_DEF
bool MCDataConvertStringToData(MCStringRef string, MCDataRef& r_data)
{
	__MCAssertIsString(string);

    // AL-2014-12-12: [[ Bug 14208 ]] Implement MCDataConvertStringToData reduce the overhead in
    //  converting from non-native string to data.
    
    MCStringRef t_native_copy;
    t_native_copy = nil;
    if (!MCStringNativeCopy(string, t_native_copy))
        return false;

    if (t_native_copy -> references != 1 || MCStringIsMutable(t_native_copy))
    {
        uindex_t t_native_length;
        const byte_t *t_data = (const byte_t *)MCStringGetNativeCharPtrAndLength(t_native_copy, t_native_length);
        if (MCDataCreateWithBytes(t_data, t_native_length, r_data))
        {
            MCValueRelease(t_native_copy);
            return true;
        }
        
        return false;
    }
    
    bool t_success;
    t_success = true;
    
    __MCData *self;
    self = nil;
    if (t_success)
        t_success = __MCValueCreate(kMCValueTypeCodeData, self);
    
    if (t_success)
    {
        self -> bytes = (byte_t *)t_native_copy -> native_chars;
        self -> byte_count = t_native_copy -> char_count;
        r_data = self;
        
        t_native_copy -> native_chars = nil;
        t_native_copy -> char_count = 0;
    }
    else
        MCMemoryDelete(self);
    
    if (t_success)
        MCValueRelease(t_native_copy);
    
    return t_success;
}

MC_DLLEXPORT_DEF
bool MCDataIsEmpty(MCDataRef p_data)
{
	__MCAssertIsData(p_data);

    if (__MCDataIsIndirect(p_data))
        p_data = p_data -> contents;
    
	return p_data -> byte_count == 0;
}

MC_DLLEXPORT_DEF
uindex_t MCDataGetLength(MCDataRef p_data)
{
	__MCAssertIsData(p_data);

    if (__MCDataIsIndirect(p_data))
        p_data = p_data -> contents;
    
    return p_data->byte_count;
}

MC_DLLEXPORT_DEF
const byte_t *MCDataGetBytePtr(MCDataRef p_data)
{
	__MCAssertIsData(p_data);

    if (__MCDataIsIndirect(p_data))
        p_data = p_data -> contents;
    
    return p_data->bytes;
}

MC_DLLEXPORT_DEF
byte_t MCDataGetByteAtIndex(MCDataRef p_data, uindex_t p_index)
{
	__MCAssertIsData(p_data);

    if (__MCDataIsIndirect(p_data))
        p_data = p_data -> contents;
    
    return p_data->bytes[p_index];
}

MC_DLLEXPORT_DEF
bool MCDataIsEqualTo(MCDataRef p_left, MCDataRef p_right)
{
	__MCAssertIsData(p_left);
	__MCAssertIsData(p_right);

    if (__MCDataIsIndirect(p_left))
        p_left = p_left -> contents;

    if (__MCDataIsIndirect(p_right))
        p_right = p_right -> contents;
    
    if (p_left -> byte_count != p_right -> byte_count)
        return false;
    
    return MCMemoryCompare(p_left -> bytes, p_right -> bytes, p_left -> byte_count) == 0;
}

// Mutable data methods

MC_DLLEXPORT_DEF
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
	else
	{
		MCValueRelease (self);
	}
    
	return t_success;
}

MC_DLLEXPORT_DEF
bool MCDataCopy(MCDataRef p_data, MCDataRef& r_new_data)
{
	__MCAssertIsData(p_data);

    if (!MCDataIsMutable(p_data))
    {
        MCValueRetain(p_data);
        r_new_data = p_data;
        return true;
    }
    
    if (__MCDataIsIndirect(p_data))
    {
        r_new_data = MCValueRetain(p_data -> contents);
        return true;
    }
    
    // Make the mutable direct data immutable, assign the
    // new data as that, and make self indirect referencing it.
    
    return __MCDataCopyMutable(p_data, r_new_data);
}

MC_DLLEXPORT_DEF
bool MCDataCopyAndRelease(MCDataRef p_data, MCDataRef& r_new_data)
{
	__MCAssertIsData(p_data);

    // If the MCData is immutable we just pass it through (as we are releasing it).
    if (!MCDataIsMutable(p_data))
    {
        r_new_data = p_data;
        return true;
    }
    
    // If the data ref is indirect then retain its reference, and release
    if (__MCDataIsIndirect(p_data))
    {
        r_new_data = MCValueRetain(p_data -> contents);
        MCValueRelease(r_new_data);
        return true;
    }
    
    // If the reference is 1, convert it to an immutable MCData
    if (p_data->references == 1)
    {
        __MCDataMakeImmutable(p_data);
        p_data -> flags &= ~kMCDataFlagIsMutable;
        p_data -> capacity = 0;
        r_new_data = p_data;
        return true;
    }
    
    // Otherwise, make a new indirect data ref
    if (!__MCDataMakeIndirect(p_data))
        return false;
    
    // And retain the contents
    r_new_data = MCValueRetain(p_data -> contents);
    return true;
}

MC_DLLEXPORT_DEF
bool MCDataMutableCopy(MCDataRef p_data, MCDataRef& r_mutable_data)
{
	__MCAssertIsData(p_data);

    // If p_data is immutable, then the new mutable data ref will be indirect
    // referencing it.
    if (!MCDataIsMutable(p_data))
        return __MCDataCreateIndirect(p_data, r_mutable_data);
    
    // If the data ref is already indirect, we just create a new reference to its contents
    if (__MCDataIsIndirect(p_data))
        return __MCDataCreateIndirect(p_data -> contents, r_mutable_data);
    
    // If the data is mutable, we make it indirect and share
    // the indirect copy.
    if (!__MCDataMakeIndirect(p_data))
        return false;
    
    return __MCDataCreateIndirect(p_data -> contents, r_mutable_data);
}

MC_DLLEXPORT_DEF
bool MCDataMutableCopyAndRelease(MCDataRef p_data, MCDataRef& r_mutable_data)
{
	__MCAssertIsData(p_data);

    if (p_data -> references == 1)
    {
        if (!MCDataIsMutable(p_data))
            p_data -> flags |= kMCDataFlagIsMutable;
        
        r_mutable_data = p_data;
        return true;
    }
    
    if (!MCDataMutableCopy(p_data, r_mutable_data))
        return false;
    
    p_data -> references -= 1;
    return true;
}

MC_DLLEXPORT_DEF
bool MCDataCopyRange(MCDataRef self, MCRange p_range, MCDataRef& r_new_data)
{
	__MCAssertIsData(self);

    if (__MCDataIsIndirect(self))
        self = self -> contents;
    
    __MCDataClampRange(self, p_range);
    
    // We special-case when the length of the range to be copied is 1. Rather
    // than create a brand new DataRef, we use one of the pre-allocated ones.
    if (p_range . length == 1)
    {
        r_new_data = MCValueRetain(__kMCSingleBytes[self -> bytes[p_range . offset]]);
        return true;
    }
    
    return MCDataCreateWithBytes(self -> bytes + p_range . offset, p_range . length, r_new_data);
}

MC_DLLEXPORT_DEF
bool MCDataCopyRangeAndRelease(MCDataRef self, MCRange p_range, MCDataRef& r_new_data)
{
    if (MCDataCopyRange(self, p_range, r_new_data))
    {
        MCValueRelease(self);
        return true;
    }
    
    return false;
}

MC_DLLEXPORT_DEF
bool MCDataIsMutable(const MCDataRef p_data)
{
	__MCAssertIsData(p_data);

    return (p_data->flags & kMCDataFlagIsMutable) != 0;
}

MC_DLLEXPORT_DEF
bool MCDataAppend(MCDataRef r_data, MCDataRef p_suffix)
{
    MCAssert(MCDataIsMutable(r_data));
    __MCAssertIsData(p_suffix);
    
    if (__MCDataIsIndirect(p_suffix))
        p_suffix = p_suffix -> contents;
    
    if (r_data == p_suffix)
    {
        // Must copy first the suffix
        MCAutoDataRef t_suffix_copy;
        if(!MCDataCopy(r_data, &t_suffix_copy))
            return false;
        
        // Copy succeeded: can process recursively the appending, and then release the newly created suffix.
        return MCDataAppend(r_data, *t_suffix_copy);
    }
    
    return MCDataAppendBytes(r_data, p_suffix -> bytes, p_suffix -> byte_count);
}

MC_DLLEXPORT_DEF
bool MCDataAppendBytes(MCDataRef self, const byte_t *p_bytes, uindex_t p_byte_count)
{
    MCAssert(MCDataIsMutable(self));
    MCAssert(nil != p_bytes);
    
    // Ensure the data ref is not indirect.
    if (__MCDataIsIndirect(self))
        if (!__MCDataResolveIndirect(self))
            return false;
    
    // Expand the capacity if necessary
    if (!__MCDataExpandAt(self, self->byte_count, p_byte_count))
        return false;
    
    MCMemoryCopy(self->bytes + self->byte_count - p_byte_count, p_bytes, p_byte_count);

    return true;
}

MC_DLLEXPORT_DEF
bool MCDataAppendByte(MCDataRef r_data, byte_t p_byte)
{
    return MCDataAppendBytes(r_data, &p_byte, 1);
}

MC_DLLEXPORT_DEF
bool MCDataPrepend(MCDataRef r_data, MCDataRef p_prefix)
{
    MCAssert(MCDataIsMutable(r_data));
    __MCAssertIsData(p_prefix);
    
    if (__MCDataIsIndirect(p_prefix))
        p_prefix = p_prefix -> contents;
    
    if (r_data == p_prefix)
    {
        // Must copy first the prefix
        MCAutoDataRef t_prefix_copy;
        if(!MCDataCopy(r_data, &t_prefix_copy))
            return false;
        
        // Copy succeeded: can process recursively the appending, and then release the newly created suffix.
        return MCDataPrepend(r_data, *t_prefix_copy);
    }
    
    return MCDataPrependBytes(r_data, p_prefix -> bytes, p_prefix -> byte_count);
}

MC_DLLEXPORT_DEF
bool MCDataPrependBytes(MCDataRef self, const byte_t *p_bytes, uindex_t p_byte_count)
{
    MCAssert(MCDataIsMutable(self));
    MCAssert(nil != p_bytes);
    
    // Ensure the data ref is not indirect.
    if (__MCDataIsIndirect(self))
        if (!__MCDataResolveIndirect(self))
            return false;
    
    // Ensure there is room enough to copy the prefix
    if (!__MCDataExpandAt(self, 0, p_byte_count))
        return false;
    
    MCMemoryCopy(self->bytes, p_bytes, p_byte_count);
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCDataPrependByte(MCDataRef r_data, byte_t p_byte)
{
    return MCDataPrependBytes(r_data, &p_byte, 1);
}

MC_DLLEXPORT_DEF
bool MCDataInsert(MCDataRef r_data, uindex_t p_at, MCDataRef p_new_data)
{
    MCAssert(MCDataIsMutable(r_data));
    __MCAssertIsData(p_new_data);
    
    if (__MCDataIsIndirect(p_new_data))
        p_new_data = p_new_data -> contents;
    
    if (r_data == p_new_data)
    {
        // Must copy first the prefix
        MCAutoDataRef t_new_data_copy;
        if(!MCDataCopy(r_data, &t_new_data_copy))
            return false;
        
        // Copy succeeded: can process recursively the appending, and then release the newly created suffix.
        return MCDataPrepend(r_data, *t_new_data_copy);
    }
    
    return MCDataInsertBytes(r_data, p_at, p_new_data -> bytes, p_new_data -> byte_count);
}

MC_DLLEXPORT_DEF
bool MCDataInsertBytes(MCDataRef self, uindex_t p_at, const byte_t *p_bytes, uindex_t p_byte_count)
{
    MCAssert(MCDataIsMutable(self));
    MCAssert(nil != p_bytes);
    
    // Ensure the data ref is not indirect.
    if (__MCDataIsIndirect(self))
        if (!__MCDataResolveIndirect(self))
            return false;
    
    // Ensure there is room enough to copy the new bytes
    if (!__MCDataExpandAt(self, p_at, p_byte_count))
        return false;
    
    MCMemoryCopy(self -> bytes, p_bytes, p_byte_count);
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCDataRemove(MCDataRef self, MCRange p_range)
{
	MCAssert(MCDataIsMutable(self));
    
    // Ensure the data ref is not indirect.
    if (__MCDataIsIndirect(self))
        if (!__MCDataResolveIndirect(self))
            return false;
    
	__MCDataClampRange(self, p_range);
    
	// Copy down the bytes above
	__MCDataShrinkAt(self, p_range.offset, p_range.length);
    
	// We succeeded.
	return true;
}

MC_DLLEXPORT_DEF
bool MCDataReplace(MCDataRef r_data, MCRange p_range, MCDataRef p_new_data)
{
	MCAssert(MCDataIsMutable(r_data));
	__MCAssertIsData(p_new_data);
    
    if (__MCDataIsIndirect(p_new_data))
        p_new_data = p_new_data -> contents;
    
	if (r_data == p_new_data)
	{
        MCAutoDataRef t_new_data;
        
        if (!MCDataCopy(p_new_data, &t_new_data))
            return false;
        
        return MCDataReplace(r_data, p_range, *t_new_data);
    }
    
    return MCDataReplaceBytes(r_data, p_range, p_new_data -> bytes, p_new_data -> byte_count);
}

MC_DLLEXPORT_DEF
bool MCDataReplaceBytes(MCDataRef self, MCRange p_range, const byte_t *p_bytes, uindex_t p_byte_count)
{
    MCAssert(MCDataIsMutable(self));
    MCAssert(nil != p_bytes);
    
    // Ensure the data ref is not indirect.
    if (__MCDataIsIndirect(self))
        if (!__MCDataResolveIndirect(self))
            return false;
    
    __MCDataClampRange(self, p_range);
    
    // Work out the new size of the string.
    uindex_t t_new_char_count;
    t_new_char_count = self->byte_count - p_range.length + p_byte_count;
    
    if (t_new_char_count > self->byte_count)
    {
        // Expand the string at the end of the range by the amount extra we
        // need.
        if (!__MCDataExpandAt(self, p_range.offset + p_range.length, t_new_char_count - self->byte_count))
            return false;
    }
    else if (t_new_char_count < self->byte_count)
    {
        // Shrink the last part of the range by the amount less we need.
        __MCDataShrinkAt(self, p_range.offset + (p_range.length - (self->byte_count - t_new_char_count)), (self->byte_count - t_new_char_count));
    }
    
    // Copy across the replacement chars.
    MCMemoryCopy(self->bytes + p_range.offset, p_bytes, p_byte_count);
    
    // We succeeded.
    return true;
}

MC_DLLEXPORT_DEF
bool MCDataPad(MCDataRef self, byte_t p_byte, uindex_t p_count)
{
    MCAssert(MCDataIsMutable(self));
    
    // Ensure the data ref is not indirect.
    if (__MCDataIsIndirect(self))
        if (!__MCDataResolveIndirect(self))
            return false;
    
	if (!__MCDataExpandAt(self, self -> byte_count, p_count))
		return false;
	
	memset(self -> bytes + self -> byte_count - p_count, p_byte, p_count);
	return true;
}

MC_DLLEXPORT_DEF
bool MCDataReverse(MCDataRef self)
{
    MCAssert(MCDataIsMutable(self));

    // Ensure the data ref is not indirect
    if (__MCDataIsIndirect(self))
        if (!__MCDataResolveIndirect(self))
            return false;

    MCInplaceReverse(self->bytes, self->byte_count);
    return true;
}

MC_DLLEXPORT_DEF
compare_t MCDataCompareTo(MCDataRef p_left, MCDataRef p_right)
{
	__MCAssertIsData(p_left);
	__MCAssertIsData(p_right);

    if (__MCDataIsIndirect(p_left))
        p_left = p_left -> contents;
    
    if (__MCDataIsIndirect(p_right))
        p_right = p_right -> contents;
    
    compare_t t_result;
    t_result = memcmp(p_left -> bytes, p_right -> bytes, MCMin(p_left -> byte_count, p_right -> byte_count));
    
    if (t_result != 0)
        return t_result;
    
    return p_left -> byte_count - p_right -> byte_count;
}

MC_DLLEXPORT_DEF
bool MCDataContains(MCDataRef p_data, MCDataRef p_needle)
{
	__MCAssertIsData(p_data);
	__MCAssertIsData(p_needle);

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

MC_DLLEXPORT_DEF
bool MCDataBeginsWith(MCDataRef p_data, MCDataRef p_needle)
{
	__MCAssertIsData(p_data);
	__MCAssertIsData(p_needle);

    uindex_t t_needle_byte_count, t_byte_count;
    t_needle_byte_count = p_needle -> byte_count;
    t_byte_count = p_data -> byte_count;
    
    if (t_needle_byte_count > t_byte_count)
        return false;
    
    return MCMemoryCompare(p_data -> bytes, p_needle -> bytes, sizeof(byte_t) * t_needle_byte_count) == 0;
}

MC_DLLEXPORT_DEF
bool MCDataEndsWith(MCDataRef p_data, MCDataRef p_needle)
{
	__MCAssertIsData(p_data);
	__MCAssertIsData(p_needle);

    uindex_t t_needle_byte_count, t_byte_count;
    t_needle_byte_count = p_needle -> byte_count;
    t_byte_count = p_data -> byte_count;
    
    if (t_needle_byte_count > t_byte_count)
        return false;
    
    return MCMemoryCompare(p_data -> bytes + t_byte_count - t_needle_byte_count, p_needle -> bytes, sizeof(byte_t) * t_needle_byte_count) == 0;
}

MC_DLLEXPORT_DEF
bool MCDataFirstIndexOf(MCDataRef p_data, MCDataRef p_chunk, MCRange p_range, uindex_t& r_index)
{
	__MCAssertIsData(p_data);
	__MCAssertIsData(p_chunk);
	
	__MCDataClampRange(p_data, p_range);
	
	if (p_range.length == 0)
		return false;
    
    uindex_t t_limit, t_chunk_byte_count, t_data_byte_count;
    t_chunk_byte_count = MCDataGetLength(p_chunk);
	t_data_byte_count = MCDataGetLength(p_data);
	
	if (t_chunk_byte_count == 0)
		return false;
	else if (t_data_byte_count < t_chunk_byte_count)
		return false;
	else if (p_range.length < t_chunk_byte_count)
		return false;

	t_limit = p_range . offset + t_data_byte_count- t_chunk_byte_count + 1;
    
    const byte_t *t_bytes = MCDataGetBytePtr(p_data);
    const byte_t *t_chunk_bytes = MCDataGetBytePtr(p_chunk);
    
    uindex_t t_offset, t_result;
    t_result = 0;
    
    bool t_found;
    t_found = false;
    
    for (t_offset = p_range . offset; t_offset < t_limit; t_offset++)
        if (MCMemoryCompare(t_bytes + t_offset, t_chunk_bytes, sizeof(byte_t) * t_chunk_byte_count) == 0)
        {
            t_result = t_offset - p_range . offset;
            t_found = true;
            break;
        }
    
    r_index = t_result;
    return t_found;
}

MC_DLLEXPORT_DEF bool
MCDataLastIndexOf (MCDataRef self,
                   MCDataRef p_needle,
                   MCRange p_range,
                   uindex_t & r_offset)
{
	const byte_t *t_needle = MCDataGetBytePtr (p_needle);
	uindex_t t_needle_len = MCDataGetLength (p_needle);

	/* Empty data is never found */
	if (0 == t_needle_len)
		return false;

	__MCDataClampRange (self, p_range);

	/* If the range is too short to contain the needle, the needle clearly
	 * can't be found. */
	if (p_range.length < t_needle_len)
		return false;

	const byte_t *t_haystack = MCDataGetBytePtr (self);

	for (uindex_t t_roffset = t_needle_len - 1;
	     t_roffset < p_range.length;
	     ++t_roffset)
	{
		/* Offset of first byte of match, relative to start of range
		 * (i.e. t_offset = 0 for the first byte) */
		uindex_t t_offset = p_range.length - t_roffset - 1;

		uindex_t t_haystack_offset = p_range.offset + t_offset;

		if (0 == MCMemoryCompare (t_haystack + t_haystack_offset, t_needle,
		                          sizeof(byte_t) * t_needle_len))
		{
			r_offset = t_offset;
			return true;
		}
	}
	return false;
}

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

MC_DLLEXPORT_DEF MCDataRef kMCEmptyData;

bool __MCDataInitialize(void)
{
    if (!MCDataCreateWithBytes(nil, 0, kMCEmptyData))
        return false;
    
    // Allocate the array for the 256 single byte DataRefs.
    if (!MCMemoryNewArray(256, __kMCSingleBytes))
        return false;
    
    // Create each single byte DataRef.
    for(uindex_t i = 0; i < 256; i++)
    {
        byte_t t_byte;
        t_byte = (byte_t)i;
        if (!MCDataCreateWithBytes(&t_byte, 1, __kMCSingleBytes[i]))
            return false;
    }
    
    return true;
}

void __MCDataFinalize(void)
{
    // Deallocate all the single byte DataRefs and their holding array.
    for(uindex_t i = 0; i < 256; i++)
        MCValueRelease(__kMCSingleBytes[i]);
    MCMemoryDeleteArray(__kMCSingleBytes);
    
    MCValueRelease(kMCEmptyData);
}

void __MCDataDestroy(__MCData *self)
{
    if (__MCDataIsIndirect(self))
    {
        MCValueRelease(self -> contents);
    }
    else
    {
        if (self -> bytes != nil)
            MCMemoryDeleteArray(self -> bytes);
    }
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
	return MCStringFormat (r_description, "<data: %u B>",
	                       MCDataGetLength (self));
}

////////////////////////////////////////////////////////////////////////////////

static bool __MCDataCreateIndirect(__MCData *data, __MCData*& r_data)
{
    MCDataRef self;
    if (!__MCValueCreate(kMCValueTypeCodeData, self))
        return false;
    
    self -> contents = MCValueRetain(data);
    self -> flags |= kMCDataFlagIsIndirect | kMCDataFlagIsMutable;
    
    r_data = self;
    return true;
}

static bool __MCDataIsIndirect(__MCData *self)
{
    return (self -> flags & kMCDataFlagIsIndirect) != 0;
}

static bool __MCDataMakeIndirect(__MCData *self)
{
    // If we are already indirect, there's nothing to do.
    if (__MCDataIsIndirect(self))
        return true;
    
    if (!__MCDataMakeImmutable(self))
        return false;
    
    // Create a new direct data ref for self to reference
    MCDataRef t_data;
    if (!__MCValueCreate(kMCValueTypeCodeData, t_data))
        return false;
    
    // Share the buffer and assign flags & count
    t_data -> flags |= self -> flags;
    t_data -> flags &= ~kMCDataFlagIsMutable;
    t_data -> byte_count = self -> byte_count;
    t_data -> bytes = self -> bytes;
    
    // 'self' now becomes indirect with a reference to the new data ref.
    self -> flags |= kMCDataFlagIsIndirect;
    self -> contents = t_data;
    return true;
}

static bool __MCDataResolveIndirect(__MCData *self)
{
    // Make sure we are indirect.
    MCAssert(__MCDataIsIndirect(self));
    
    // Fetch the reference.
    MCDataRef t_data;
    t_data = self -> contents;
    
    // If the data ref only has a single reference, then re-absorb; otherwise
    // copy.
    if (self -> contents -> references == 1)
    {
        self -> byte_count = t_data -> byte_count;
        self -> capacity = t_data -> capacity;
        self -> flags |= t_data -> flags;
        self -> bytes = t_data -> bytes;

        t_data -> byte_count = 0;
        t_data -> bytes = nil;

        MCValueRelease(t_data);
    }
    else
    {
        MCValueRelease(self -> contents);
        
        if (!MCMemoryNewArray(t_data -> byte_count, self -> bytes))
            return false;
        
        MCMemoryCopy(self -> bytes, t_data -> bytes, t_data -> byte_count);

        self -> byte_count = t_data -> byte_count;
        self -> capacity = t_data -> byte_count;
    }
    
    self -> flags &= ~kMCDataFlagIsIndirect;
    
    return true;
}

static bool __MCDataCopyMutable(__MCData *self, __MCData*& r_new_data)
{
    if (!__MCDataMakeImmutable(self))
        return false;
    
    __MCData *t_data;
    t_data = nil;
    
    if (self -> byte_count == 0)
    {
        t_data = MCValueRetain(kMCEmptyData);
        MCMemoryDeleteArray(self -> bytes);
    }
    else
    {
        if (!__MCValueCreate(kMCValueTypeCodeData, t_data))
            return false;
        
        t_data -> byte_count = self -> byte_count;
        t_data -> bytes = self -> bytes;
        t_data -> capacity = 0;
    }
    
    self -> byte_count = 0;
    self -> bytes = nil;
    self -> contents = MCValueRetain(t_data);
    self -> flags |= kMCDataFlagIsIndirect;
    
    r_new_data = t_data;
    return true;
}

