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

#include "foundation-private.h"
#include "foundation-hash.h"

////////////////////////////////////////////////////////////////////////////////

static bool __check_conformance(MCTypeInfoRef p_typeinfo, const MCValueRef *p_values, uindex_t p_value_count, uindex_t& x_offset)
{
    if (x_offset + p_typeinfo -> record . field_count > p_value_count)
        return MCErrorThrowGeneric(MCSTR("record does not conform to target type: not enough fields"));
    
    for(uindex_t i = 0; i < p_typeinfo -> record . field_count; i++)
        if (MCTypeInfoConforms(MCValueGetTypeInfo(p_values[x_offset + i]), p_typeinfo -> record . fields[i] . type))
            return MCErrorThrowGenericWithMessage(MCSTR("record field %{field} does not conform to target type %{type}"),
                                                  "field",
                                                  p_values[x_offset + i],
                                                  "type",
                                                  p_typeinfo->record.fields[i].type,
                                                  nullptr);
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCRecordCreate(MCTypeInfoRef p_typeinfo, const MCValueRef *p_values, uindex_t p_value_count, MCRecordRef& r_record)
{
	MCAssert(p_values != nil || p_value_count == 0);

    bool t_success;
    t_success = true;
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(p_typeinfo);

    MCAssert(MCTypeInfoIsRecord(t_resolved_typeinfo));
    
    uindex_t t_offset = 0;
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

MC_DLLEXPORT_DEF
bool MCRecordCreateMutable(MCTypeInfoRef p_typeinfo, MCRecordRef& r_record)
{
    bool t_success;
    t_success = true;
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(p_typeinfo);

    MCAssert(MCTypeInfoIsRecord(p_typeinfo));

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

MC_DLLEXPORT_DEF
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

MC_DLLEXPORT_DEF
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

MC_DLLEXPORT_DEF
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

MC_DLLEXPORT_DEF
bool MCRecordMutableCopyAndRelease(MCRecordRef self, MCRecordRef& r_new_record)
{
    if (MCRecordMutableCopy(self, r_new_record))
    {
        MCValueRelease(self);
        return true;
    }
    
    return false;
}

MC_DLLEXPORT_DEF
bool MCRecordIsMutable(MCRecordRef self)
{
	__MCAssertIsRecord(self);
    return (self -> flags & kMCRecordFlagIsMutable) != 0;
}

static bool __fetch_value(MCTypeInfoRef p_typeinfo, MCRecordRef self, MCNameRef p_field, MCValueRef& r_value)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    for(uindex_t i = 0; i < t_resolved_typeinfo -> record . field_count; i++)
        if (MCNameIsEqualToCaseless(p_field, t_resolved_typeinfo -> record . fields[i] . name))
        {
            r_value = self -> fields[i];
            return true;
        }
    
    return false;
}

MC_DLLEXPORT_DEF
bool MCRecordFetchValue(MCRecordRef self, MCNameRef p_field, MCValueRef& r_value)
{
	__MCAssertIsRecord(self);
	__MCAssertIsName(p_field);
    return __fetch_value(self -> typeinfo, self, p_field, r_value);
}

static bool __store_value(MCTypeInfoRef p_typeinfo, MCRecordRef self, MCNameRef p_field, MCValueRef p_value)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    for(uindex_t i = 0; i < t_resolved_typeinfo -> record . field_count; i++)
        if (MCNameIsEqualToCaseless(p_field, t_resolved_typeinfo -> record . fields[i] . name))
        {
            if (!MCTypeInfoConforms(MCValueGetTypeInfo(p_value), t_resolved_typeinfo -> record . fields[i] . type))
                return MCErrorThrowGeneric(nil);
            
            self -> fields[i] = MCValueRetain(p_value);
            return true;
        }
    
    return false;
}

MC_DLLEXPORT_DEF
bool MCRecordStoreValue(MCRecordRef self, MCNameRef p_field, MCValueRef p_value)
{
	__MCAssertIsRecord(self);
	__MCAssertIsName(p_field);
	MCAssert(nil != p_value);
    return __store_value(self -> typeinfo, self, p_field, p_value);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF bool
MCRecordEncodeAsArray(MCRecordRef record,
                      MCArrayRef & r_array)
{
	__MCAssertIsRecord(record);

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

MC_DLLEXPORT_DEF bool
MCRecordDecodeFromArray(MCArrayRef array,
                        MCTypeInfoRef p_typeinfo,
                        MCRecordRef & r_record)
{
	__MCAssertIsArray(array);
	MCAssert(MCTypeInfoIsRecord(p_typeinfo));

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

MC_DLLEXPORT_DEF bool
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

	t_hash = MCHashObjectStream(t_hash, MCHashPointer(self->typeinfo));

    for(uindex_t i = 0; i < __MCRecordTypeInfoGetFieldCount(t_resolved_typeinfo); i++)
    {
        t_hash = MCHashObjectStream(t_hash, MCValueHash(self -> fields[i]));
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
