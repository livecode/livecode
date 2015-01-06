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

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

static bool __check_conformance(MCTypeInfoRef p_typeinfo, const MCValueRef *p_values, uindex_t p_value_count, uindex_t& x_offset)
{
    if (p_typeinfo -> record . base != kMCNullTypeInfo)
    {
        MCTypeInfoRef t_resolved_typeinfo;
        t_resolved_typeinfo = __MCTypeInfoResolve(p_typeinfo);
        if (!__check_conformance(t_resolved_typeinfo, p_values, p_value_count, x_offset))
            return false;
    }
    
    if (x_offset + p_typeinfo -> record . field_count > p_value_count)
        return MCErrorThrowGeneric(nil);
    
    for(uindex_t i = 0; i < p_typeinfo -> record . field_count; i++)
        if (MCTypeInfoConforms(MCValueGetTypeInfo(p_values[x_offset + i]), p_typeinfo -> record . fields[i] . type))
            return MCErrorThrowGeneric(nil);
    
    return true;
}

bool MCRecordCreate(MCTypeInfoRef p_typeinfo, const MCValueRef *p_values, uindex_t p_value_count, MCRecordRef& r_record)
{
    bool t_success;
    t_success = true;
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(p_typeinfo);
    
    uindex_t t_offset;
    if (!__check_conformance(t_resolved_typeinfo, p_values, p_value_count, t_offset))
        return false;
    
    __MCRecord *self;
    self = nil;
    if (t_success)
        t_success = __MCValueCreate(kMCValueTypeCodeRecord, self);
    
    // Here 'offset' will be the total count of all fields.
    if (t_success)
        t_success = MCMemoryNewArray(t_offset, self -> fields);
    
    if (t_success)
    {
        for(uindex_t i = 0; i < p_value_count; i++)
            self -> fields[i] = MCValueRetain(p_values[i]);
    
        self -> typeinfo = MCValueRetain(p_typeinfo);
    
        r_record = self;
    }
    else
    {
        MCMemoryDeleteArray(self -> fields);
        MCMemoryDelete(self);
    }
    
    return t_success;
    
}

bool MCRecordCreateMutable(MCTypeInfoRef p_typeinfo, MCRecordRef& r_record)
{
    bool t_success;
    t_success = true;
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(p_typeinfo);
    
    uindex_t t_field_count;
    t_field_count = __MCRecordTypeInfoGetFieldCount(t_resolved_typeinfo);
    
    __MCRecord *self;
    self = nil;
    if (t_success)
        t_success = __MCValueCreate(kMCValueTypeCodeRecord, self);
    
    if (t_success)
        t_success = MCMemoryNewArray(t_field_count, self -> fields);
    
    if (t_success)
    {
        for(uindex_t i = 0; i < t_field_count; i++)
            self -> fields[i] = MCValueRetain(kMCNull);
        
        self -> typeinfo = MCValueRetain (p_typeinfo);
        self -> flags |= kMCRecordFlagIsMutable;
        
        r_record = self;
    }
    else
    {
        MCMemoryDeleteArray(self -> fields);
        MCMemoryDelete(self);
    }
    
    return t_success;
}

bool MCRecordCopy(MCRecordRef self, MCRecordRef& r_new_record)
{
    if (!MCRecordIsMutable(self))
    {
        MCValueRetain(self);
        r_new_record = self;
        return true;
    }
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    return MCRecordCreate(self -> typeinfo, self -> fields, __MCRecordTypeInfoGetFieldCount(t_resolved_typeinfo), r_new_record);
}

bool MCRecordCopyAndRelease(MCRecordRef self, MCRecordRef& r_new_record)
{
    // If the MCRecord is immutable we just pass it through (as we are releasing it).
    if (!MCRecordIsMutable(self))
    {
        r_new_record = self;
        return true;
    }
    
    // If the reference is 1, convert it to an immutable MCData
    if (self->references == 1)
    {
        self->flags &= ~kMCRecordFlagIsMutable;
        r_new_record = self;
        return true;
    }
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    // Otherwise make a copy of the data and then release the original
    bool t_success;
    t_success = MCRecordCreate(self -> typeinfo, self -> fields, __MCRecordTypeInfoGetFieldCount(t_resolved_typeinfo), r_new_record);
    MCValueRelease(self);
    
    return t_success;
}

bool MCRecordMutableCopy(MCRecordRef self, MCRecordRef& r_new_record)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    MCRecordRef t_new_self;
    if (!MCRecordCreate(self -> typeinfo, self -> fields, __MCRecordTypeInfoGetFieldCount(t_resolved_typeinfo), t_new_self))
        return false;
    
    t_new_self -> flags |= kMCRecordFlagIsMutable;
    
    r_new_record = t_new_self;
    
    return true;
}

bool MCRecordMutableCopyAndRelease(MCRecordRef self, MCRecordRef& r_new_record)
{
    if (MCRecordMutableCopy(self, r_new_record))
    {
        MCValueRelease(self);
        return true;
    }
    
    return false;
}

bool MCRecordIsMutable(MCRecordRef self)
{
    return (self -> flags & kMCRecordFlagIsMutable) != 0;
}

static bool __fetch_value(MCTypeInfoRef p_typeinfo, MCRecordRef self, MCNameRef p_field, MCValueRef& r_value)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    for(uindex_t i = 0; i < t_resolved_typeinfo -> record . field_count; i++)
        if (MCNameIsEqualTo(p_field, t_resolved_typeinfo -> record . fields[i] . name))
        {
            r_value = self -> fields[i];
            return true;
        }
    
    if (p_typeinfo -> record . base != kMCNullTypeInfo)
        return __fetch_value(p_typeinfo -> record . base, self, p_field, r_value);
    
    return false;
}

bool MCRecordFetchValue(MCRecordRef self, MCNameRef p_field, MCValueRef& r_value)
{
    return __fetch_value(self -> typeinfo, self, p_field, r_value);
}

static bool __store_value(MCTypeInfoRef p_typeinfo, MCRecordRef self, MCNameRef p_field, MCValueRef p_value)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    for(uindex_t i = 0; i < t_resolved_typeinfo -> record . field_count; i++)
        if (MCNameIsEqualTo(p_field, t_resolved_typeinfo -> record . fields[i] . name))
        {
            if (!MCTypeInfoConforms(MCValueGetTypeInfo(p_value), t_resolved_typeinfo -> record . fields[i] . type))
                return MCErrorThrowGeneric(nil);
            
            self -> fields[i] = MCValueRetain(p_value);
            return true;
        }
    
    if (p_typeinfo -> record . base != kMCNullTypeInfo)
        return __store_value(p_typeinfo -> record . base, self, p_field, p_value);
    
    return false;
}

bool MCRecordStoreValue(MCRecordRef self, MCNameRef p_field, MCValueRef p_value)
{
    return __store_value(self -> typeinfo, self, p_field, p_value);
}

////////////////////////////////////////////////////////////////////////////////

bool
MCRecordCopyAsBaseType(MCRecordRef self,
                       MCTypeInfoRef p_base_typeinfo,
                       MCRecordRef & r_new_record)
{
	bool t_success;
	MCValueRetain(self);
	t_success = MCRecordCopyAsBaseTypeAndRelease(self,
	                                             p_base_typeinfo,
	                                             r_new_record);
	if (!t_success) MCValueRelease(self);
	return t_success;
}

bool
MCRecordCopyAsBaseTypeAndRelease(MCRecordRef self,
                                 MCTypeInfoRef p_base_typeinfo,
                                 MCRecordRef & r_new_record)
{
	MCAssert(MCRecordTypeInfoIsDerivedFrom(self -> typeinfo, p_base_typeinfo));

	/* If there's only one reference, just swap the typeinfo out and
	 * make it immutable */
	if (self -> references == 1)
	{
		MCValueRelease(self -> typeinfo);
		self -> typeinfo = MCValueRetain(p_base_typeinfo);
		self -> flags &= ~kMCRecordFlagIsMutable;
		r_new_record = self;
		return true;
	}

	if (!MCRecordCreate(p_base_typeinfo, self -> fields,
	                    MCRecordTypeInfoGetFieldCount (p_base_typeinfo),
	                    r_new_record))
		return false;

	MCValueRelease(self);
	return true;
}

bool
MCRecordCopyAsDerivedType(MCRecordRef self,
                          MCTypeInfoRef p_derived_typeinfo,
                          MCRecordRef & r_new_record)
{
	bool t_success;
	MCValueRetain(self);
	t_success = MCRecordCopyAsDerivedTypeAndRelease(self,
	                                                p_derived_typeinfo,
	                                                r_new_record);
	if (!t_success) MCValueRelease(self);
	return t_success;
}

bool
MCRecordCopyAsDerivedTypeAndRelease(MCRecordRef self,
                                    MCTypeInfoRef p_derived_typeinfo,
                                    MCRecordRef & r_new_record)
{
	MCAssert(MCRecordTypeInfoIsDerivedFrom(p_derived_typeinfo, self -> typeinfo));

	uindex_t t_field_count, t_new_field_count;
	t_field_count = MCRecordTypeInfoGetFieldCount(self -> typeinfo);
	t_new_field_count = MCRecordTypeInfoGetFieldCount(p_derived_typeinfo);

	/* If there's only one reference, then we need to: 1) swap the
	 * typeinfo, 2) resize the field array and fill the new fields
	 * with null values, and 3) make the record immutable. */
	if (self -> references == 1)
	{
		/* Resize the array */
		if (!MCMemoryResizeArray(t_new_field_count, self -> fields, t_field_count))
			return false;
		/* Clear new values */
		for (uindex_t i = t_field_count; i < t_new_field_count; ++i)
			self -> fields[i] = MCValueRetain(kMCNull);

		/* Set the typeinfo and make immutable */
		MCValueRelease(self -> typeinfo);
		self -> typeinfo = MCValueRetain(p_derived_typeinfo);
		self -> flags &= ~kMCRecordFlagIsMutable;

		r_new_record = self;
		return true;
	}

	/* Otherwise, create and manually populate the new array */
	MCRecordRef t_result;
	if (!MCRecordCreateMutable(p_derived_typeinfo, t_result))
		return false;

	for (uindex_t i = 0; i < t_field_count; ++i)
		t_result -> fields[i] = MCValueRetain(self -> fields[i]);

	return MCRecordCopyAndRelease(t_result, r_new_record);
}

////////////////////////////////////////////////////////////////////////////////

bool
MCRecordEncodeAsArray(MCRecordRef record,
                      MCArrayRef & r_array)
{
	MCAssert (record != nil);

	MCTypeInfoRef t_typeinfo = MCValueGetTypeInfo (record);
	uindex_t t_num_fields = MCRecordTypeInfoGetFieldCount (t_typeinfo);

	/* Each field name in the record is used as a key in the array.
	 * N.b. records decode/encode to dictionaries, not maps, so field
	 * names are case insensitive. */
	MCArrayRef t_new_array;
	if (!MCArrayCreateMutable (t_new_array))
		return false;

	for (uindex_t i = 0; i < t_num_fields; ++i)
	{
		MCNameRef t_field_name = MCRecordTypeInfoGetFieldName (t_typeinfo, i);
		MCValueRef t_field_value;

		if (!(MCRecordFetchValue (record, t_field_name, t_field_value) &&
		      !MCArrayStoreValue (t_new_array, false,
		                          t_field_name, t_field_value)))
		{
			MCValueRelease (t_new_array);
			return false;
		}
	}

	return MCArrayCopyAndRelease (t_new_array, r_array);
}

bool
MCRecordDecodeFromArray(MCArrayRef array,
                        MCTypeInfoRef p_typeinfo,
                        MCRecordRef & r_record)
{
	/* Create the result record */
	MCRecordRef t_new_record;
	if (!MCRecordCreateMutable (p_typeinfo, t_new_record))
		return false;

	/* We require each field name in the record to be a key in the
	 * array, and for the types to match.  N.b. records decode/encode
	 * to dictionaries, not maps, so field names are case
	 * insensitive. */
	uindex_t t_num_fields = MCRecordTypeInfoGetFieldCount (p_typeinfo);
	for (uindex_t i = 0; i < t_num_fields; ++i)
	{
		MCNameRef t_field_name = MCRecordTypeInfoGetFieldName (p_typeinfo, i);
		MCValueRef t_field_value;
		if (!(MCArrayFetchValue (array, false, t_field_name, t_field_value) &&
		      MCRecordStoreValue (t_new_record, t_field_name, t_field_value)))
		{
			MCValueRelease (t_new_record);
			return false;
		}
	}

	return MCRecordCopyAndRelease (t_new_record, r_record);
}

////////////////////////////////////////////////////////////////////////////////

bool
MCRecordIterate(MCRecordRef record,
                uintptr_t& x_iterator,
                MCNameRef& r_field_name,
                MCValueRef& r_field_value)
{
	MCTypeInfoRef t_typeinfo = MCValueGetTypeInfo (record);
	uindex_t t_num_fields = MCRecordTypeInfoGetFieldCount (t_typeinfo);
    
    /* If the iterator parameter has reached the field count then we are
     * done. */
    if (x_iterator >= (uintptr_t)t_num_fields)
        return false;
    
    r_field_name = MCRecordTypeInfoGetFieldName (t_typeinfo, x_iterator);
    r_field_value = record -> fields[x_iterator];
    
    x_iterator += 1;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void __MCRecordDestroy(__MCRecord *self)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    for(uindex_t i = 0; i < __MCRecordTypeInfoGetFieldCount(t_resolved_typeinfo); i++)
        MCValueRelease(self -> fields[i]);
    MCValueRelease(self -> typeinfo);
    MCMemoryDelete(self -> fields);
}

hash_t __MCRecordHash(__MCRecord *self)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    hash_t t_hash;
    t_hash = 0;

	hash_t t_typeinfo_hash;
	t_typeinfo_hash = MCHashPointer(self -> typeinfo);
	t_hash = MCHashBytesStream(t_hash, &t_typeinfo_hash, sizeof(hash_t));

    for(uindex_t i = 0; i < __MCRecordTypeInfoGetFieldCount(t_resolved_typeinfo); i++)
    {
        hash_t t_element_hash;
        t_element_hash = MCValueHash(self -> fields[i]);
        t_hash = MCHashBytesStream(t_hash, &t_element_hash, sizeof(hash_t));
    }
    return t_hash;
}

bool __MCRecordIsEqualTo(__MCRecord *self, __MCRecord *other_self)
{
    // Records must have the same typeinfo to be equal.
    if (self -> typeinfo != other_self -> typeinfo)
        return false;
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    // Each field within the record must be equal to be equal.
    for(uindex_t i = 0; i < __MCRecordTypeInfoGetFieldCount(t_resolved_typeinfo); i++)
        if (!MCValueIsEqualTo(self -> fields[i], other_self -> fields[i]))
            return false;

    return true;
}

bool __MCRecordCopyDescription(__MCRecord *self, MCStringRef &r_description)
{
    return false;
}

bool __MCRecordImmutableCopy(__MCRecord *self, bool p_release, __MCRecord *&r_immutable_value)
{
    if (!p_release)
        return MCRecordCopy(self, r_immutable_value);
    return MCRecordCopyAndRelease(self, r_immutable_value);
}

bool __MCRecordInitialize(void)
{
	return true;
}

void __MCRecordFinalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////
