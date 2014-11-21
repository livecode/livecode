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

MCTypeInfoRef kMCNullTypeInfo;
MCTypeInfoRef kMCBooleanTypeInfo;
MCTypeInfoRef kMCNumberTypeInfo;
MCTypeInfoRef kMCStringTypeInfo;
MCTypeInfoRef kMCDataTypeInfo;
MCTypeInfoRef kMCArrayTypeInfo;
MCTypeInfoRef kMCSetTypeInfo;
MCTypeInfoRef kMCListTypeInfo;

////////////////////////////////////////////////////////////////////////////////

static bool __MCTypeInfoIsNamed(MCTypeInfoRef self);

////////////////////////////////////////////////////////////////////////////////

MCValueTypeCode MCTypeInfoGetTypeCode(MCTypeInfoRef self)
{
    MCTypeInfoRef t_actual_typeinfo;
    t_actual_typeinfo = __MCTypeInfoResolve(self);
    return t_actual_typeinfo -> flags & kMCTypeInfoTypeCodeMask;
}

MCNameRef MCTypeInfoGetName(MCTypeInfoRef self)
{
    if (__MCTypeInfoIsNamed(self))
        return self -> named . name;
    return nil;
}

bool MCTypeInfoIsOptional(MCTypeInfoRef self)
{
    return false;
}

bool MCTypeInfoConforms(MCTypeInfoRef source, MCTypeInfoRef target)
{
    return source == target;
}

bool MCTypeInfoBind(MCNameRef p_name, MCTypeInfoRef p_typeinfo, MCTypeInfoRef& r_typeinfo)
{
    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    self -> flags |= 0xff;
    self -> named . name = MCValueRetain(p_name);
    self -> named . typeinfo = __MCTypeInfoResolve(p_typeinfo);
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    return false;
}

bool MCTypeInfoBindAndRelease(MCNameRef p_name, MCTypeInfoRef p_typeinfo, MCTypeInfoRef& r_typeinfo)
{
    if (MCTypeInfoBind(p_name, p_typeinfo, r_typeinfo))
    {
        MCValueRelease(p_typeinfo);
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCRecordTypeInfoCreate(const MCRecordTypeFieldInfo *p_fields, index_t p_field_count, MCTypeInfoRef p_base, MCTypeInfoRef& r_typeinfo)
{
	MCTypeInfoRef t_resolved_base;
	t_resolved_base = nil;
	if (p_base != nil)
	{
		t_resolved_base = __MCTypeInfoResolve(p_base);
		MCAssert((t_resolved_base -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeRecord);
	}

    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    if (!MCMemoryNewArray(p_field_count, self -> record . fields))
    {
        MCMemoryDelete(self);
        return false;
    }
    
    self -> flags |= kMCValueTypeCodeRecord;

	/* If the p_field_count < 0 then the p_fields are expected to be
	 * terminated by a custodian with name = nil. */
	uindex_t i;
	i = 0;
	while ((p_field_count >= 0) ? (i < p_field_count) : (p_fields[i].name != nil))
	{
		/* Verify that the field names are all caselessly distinct.
		 * N.b. O(N^2) algorithm is inefficient, but will only be run
		 * in debug builds and will only happen once per type. */
		for (uindex_t j = 0; j < i; ++j)
		{
			MCAssert(!MCNameIsEqualTo(p_fields[i] . name, p_fields[j] . name));
		}
        self -> record . fields[i] . name = MCValueRetain(p_fields[i] . name);
        self -> record . fields[i] . type = MCValueRetain(p_fields[i] . type);
		++i;
    }
    self -> record . field_count = p_field_count;
    self -> record . base = MCValueRetain(t_resolved_base != nil ? t_resolved_base : kMCNullTypeInfo);
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

MCTypeInfoRef MCRecordTypeInfoGetBaseType(MCTypeInfoRef unresolved_self)
{
	MCTypeInfoRef self;
	self = __MCTypeInfoResolve(unresolved_self);
	MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeRecord);
    return self -> record . base;
}

uindex_t MCRecordTypeInfoGetFieldCount(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeRecord);

	/* Sum field counts of all base record types */
	uindex_t t_field_count;
	t_field_count = 0;
	while (self != kMCNullTypeInfo) {
		t_field_count += self -> record . field_count;
		self = MCRecordTypeInfoGetBaseType (self);
	};
    
    return t_field_count;
}

MCNameRef MCRecordTypeInfoGetFieldName(MCTypeInfoRef unresolved_self, uindex_t p_index)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeRecord);

	MCTypeInfoRef t_base_type;
	uindex_t t_base_index;
	__MCRecordTypeInfoGetBaseTypeForField(self, p_index,
	                                      t_base_type, t_base_index);

	return t_base_type -> record . fields[t_base_index] . name;
}

MCTypeInfoRef MCRecordTypeInfoGetFieldType(MCTypeInfoRef unresolved_self, uindex_t p_index)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeRecord);

	MCTypeInfoRef t_base_type;
	uindex_t t_base_index;
	__MCRecordTypeInfoGetBaseTypeForField(self, p_index,
	                                      t_base_type, t_base_index);

	return t_base_type -> record . fields[t_base_index] . type;
}

bool
MCRecordTypeInfoIsDerivedFrom(MCTypeInfoRef unresolved_self,
                              MCTypeInfoRef unresolved_other)
{
	MCTypeInfoRef self, other;
	self = __MCTypeInfoResolve(unresolved_self);
	other = __MCTypeInfoResolve(unresolved_other);

	MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeRecord);
	MCAssert((other -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeRecord);

	MCTypeInfoRef t_base;
	while (t_base != kMCNullTypeInfo)
	{
		if (t_base == other) return true;
		t_base = MCRecordTypeInfoGetBaseType(t_base);
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void
__MCRecordTypeInfoGetBaseTypeForField (__MCTypeInfo *self,
                                       uindex_t p_index,
                                       __MCTypeInfo *& r_base,
                                       uindex_t & r_base_index)
{
	uindex_t t_total_field_count;
	t_total_field_count = MCRecordTypeInfoGetFieldCount(self);
	MCAssert(t_total_field_count > p_index);

	/* Search for the base record type where the requested field is
	 * defined. */
	uindex_t t_base_field_count;
	t_base_field_count = t_total_field_count;
	while (t_base_field_count > p_index)
	{
		MCAssert (self != kMCNullTypeInfo);
		t_base_field_count -= self -> record . field_count;
		self = MCRecordTypeInfoGetBaseType(self);
	}

	r_base = self;
	r_base_index = p_index - t_base_field_count;
}

////////////////////////////////////////////////////////////////////////////////

bool MCHandlerTypeInfoCreate(const MCHandlerTypeFieldInfo *p_fields, index_t p_field_count, MCTypeInfoRef p_return_type, MCTypeInfoRef& r_typeinfo)
{
    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    if (!MCMemoryNewArray(p_field_count, self -> handler . fields))
    {
        MCMemoryDelete(self);
        return false;
    }
    
    self -> flags |= kMCValueTypeCodeHandler;

	uindex_t i;
	i = 0;
	/* If the p_field_count < 0 then the p_fields are expected to be
	 * terminated by a custodian with name = nil. */
	while ((p_field_count >= 0) ? (i < p_field_count) : p_fields[i].name != nil)
    {
        self -> handler . fields[i] . name = MCValueRetain(p_fields[i] . name);
        self -> handler . fields[i] . type = MCValueRetain(p_fields[i] . type);
        self -> handler . fields[i] . mode = p_fields[i] . mode;
		++i;
    }
    self -> handler . field_count = p_field_count;
    
    self -> handler . return_type = MCValueRetain(p_return_type);
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

MCTypeInfoRef MCHandlerTypeInfoGetReturnType(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeHandler);
    
    return self -> handler . return_type;
}

uindex_t MCHandlerTypeInfoGetParameterCount(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);

    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeHandler);
    
    return self -> handler . field_count;
}

MCNameRef MCHandlerTypeInfoGetParameterName(MCTypeInfoRef unresolved_self, uindex_t p_index)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeHandler);
    MCAssert(self -> handler . field_count > p_index);
    
    return self -> handler . fields[p_index] . name;
}

MCHandlerTypeFieldMode MCHandlerTypeInfoGetParameterMode(MCTypeInfoRef unresolved_self, uindex_t p_index)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);

    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeHandler);
    MCAssert(self -> handler . field_count > p_index);
    
    return self -> handler . fields[p_index] . mode;
}

MCTypeInfoRef MCHandlerTypeInfoGetParameterType(MCTypeInfoRef unresolved_self, uindex_t p_index)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);

    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeHandler);
    MCAssert(self -> handler . field_count > p_index);
    
    return self -> handler . fields[p_index] . type;
}

////////////////////////////////////////////////////////////////////////////////

bool MCErrorTypeInfoCreate(MCNameRef p_domain, MCStringRef p_message, MCTypeInfoRef& r_typeinfo)
{
    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    self -> error . domain = MCValueRetain(p_domain);
    self -> error . message = MCValueRetain(p_message);
    
    r_typeinfo = self;
    
    return true;
}

MCNameRef MCErrorTypeInfoGetDomain(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    return self -> error . domain;
}

MCStringRef MCErrorTypeInfoGetMessage(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    return self -> error . message;
}

////////////////////////////////////////////////////////////////////////////////

static bool __MCTypeInfoCreateBuiltin(MCValueTypeCode p_code, MCTypeInfoRef& r_typeinfo)
{
    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    self -> flags |= p_code & 0xff;
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

static MCValueTypeCode __MCTypeInfoGetTypeCode(MCTypeInfoRef self)
{
    return (self -> flags & kMCTypeInfoTypeCodeMask);
}

static bool __MCTypeInfoIsNamed(MCTypeInfoRef self)
{
    return __MCTypeInfoGetTypeCode(self) == 0xff;
}

MCTypeInfoRef __MCTypeInfoResolve(MCTypeInfoRef self)
{
    if (__MCTypeInfoIsNamed(self))
        return self -> named . typeinfo;
    return self;
}

////////////////////////////////////////////////////////////////////////////////

void __MCTypeInfoDestroy(__MCTypeInfo *self)
{
    if (__MCTypeInfoIsNamed(self))
    {
        MCValueRelease(self -> named . name);
        MCValueRelease(self -> named . typeinfo);
    }
    else if (__MCTypeInfoGetTypeCode(self) == kMCValueTypeCodeRecord)
    {
        MCValueRelease(self -> record . base);
        for(uindex_t i = 0; i < self -> record . field_count; i++)
        {
            MCValueRelease(self -> record . fields[i] . name);
            MCValueRelease(self -> record . fields[i] . type);
        }
        MCMemoryDeleteArray(self -> record . fields);
    }
    else if (__MCTypeInfoGetTypeCode(self) == kMCValueTypeCodeHandler)
    {
        for(uindex_t i = 0; i < self -> handler . field_count; i++)
        {
            MCValueRelease(self -> handler . fields[i] . name);
            MCValueRelease(self -> handler . fields[i] . type);
        }
        MCValueRelease(self -> handler . return_type);
        MCMemoryDeleteArray(self -> handler . fields);
    }
    else if (__MCTypeInfoGetTypeCode(self) == kMCValueTypeCodeError)
    {
        MCValueRelease(self -> error . domain);
        MCValueRelease(self -> error . message);
    }
}

hash_t __MCTypeInfoHash(__MCTypeInfo *self)
{
    hash_t t_hash;
    t_hash = 0;
    
    uint32_t t_code;
    t_code = __MCTypeInfoGetTypeCode(self);
    
    t_hash = MCHashBytesStream(t_hash, &t_code, sizeof(uint32_t));
    if (__MCTypeInfoIsNamed(self))
        t_hash = MCHashBytesStream(t_hash, &self -> named, sizeof(self -> named));
    else if (t_code == kMCValueTypeCodeRecord)
    {
        t_hash = MCHashBytesStream(t_hash, &self -> record . base, sizeof(self -> record . base));
        t_hash = MCHashBytesStream(t_hash, &self -> record . field_count, sizeof(self -> record . field_count));
        t_hash = MCHashBytesStream(t_hash, self -> record . fields, sizeof(MCRecordTypeFieldInfo) * self -> record . field_count);
    }
    else if (t_code == kMCValueTypeCodeHandler)
    {
        t_hash = MCHashBytesStream(t_hash, &self -> handler . field_count, sizeof(self -> handler . field_count));
        t_hash = MCHashBytesStream(t_hash, &self -> handler . return_type, sizeof(self -> handler . return_type));
        t_hash = MCHashBytesStream(t_hash, self -> handler . fields, sizeof(MCRecordTypeFieldInfo) * self -> handler . field_count);
    }
    else if (t_code == kMCValueTypeCodeError)
    {
        t_hash = MCHashBytesStream(t_hash, &self -> error . domain, sizeof(self -> error . domain));
        t_hash = MCHashBytesStream(t_hash, self -> error . message, sizeof(self -> error . message));
    }
    
    return t_hash;
}

bool __MCTypeInfoIsEqualTo(__MCTypeInfo *self, __MCTypeInfo *other_self)
{
    if (__MCTypeInfoGetTypeCode(self) != __MCTypeInfoGetTypeCode(other_self))
        return false;
    
    MCValueTypeCode t_code;
    t_code = __MCTypeInfoGetTypeCode(self);
    if (t_code == 0xff)
        return MCNameIsEqualTo(self -> named . name, other_self -> named . name) &&
                    self -> named . typeinfo == other_self -> named . typeinfo;
    
    if (t_code == kMCValueTypeCodeRecord)
    {
        if (self -> record . base != other_self -> record . base)
            return false;
        
        if (self -> record . field_count != other_self -> record . field_count)
            return false;
        
        for(uindex_t i = 0; i < self -> record . field_count; i++)
            if (!MCNameIsEqualTo(self -> record . fields[i] . name, other_self -> record . fields[i] . name) ||
                self -> record . fields[i] . type != other_self -> record . fields[i] . type)
                return false;
    }
    else if (t_code == kMCValueTypeCodeHandler)
    {
        if (self -> handler . field_count != other_self -> handler . field_count)
            return false;
        if (self -> handler . return_type != other_self -> handler . return_type)
            return false;
        
        for(uindex_t i = 0; i < self -> handler . field_count; i++)
            if (!MCNameIsEqualTo(self -> handler . fields[i] . name, other_self -> handler . fields[i] . name) ||
                self -> handler . fields[i] . type != other_self -> handler . fields[i] . type ||
                self -> handler . fields[i] . mode != other_self -> handler . fields[i] . mode)
                return false;
    }
    else if (t_code == kMCValueTypeCodeError)
    {
        if (self -> error . domain != other_self -> error . domain)
            return false;
        if (self -> error . message != other_self -> error . message)
            return false;
    }
    
    return true;
}

bool __MCTypeInfoCopyDescription(__MCTypeInfo *self, MCStringRef& r_description)
{
    return false;
}

bool __MCTypeInfoInitialize(void)
{
    return
        __MCTypeInfoCreateBuiltin(kMCValueTypeCodeNull, kMCNullTypeInfo) &&
        __MCTypeInfoCreateBuiltin(kMCValueTypeCodeBoolean, kMCBooleanTypeInfo) &&
        __MCTypeInfoCreateBuiltin(kMCValueTypeCodeNumber, kMCNumberTypeInfo) &&
        __MCTypeInfoCreateBuiltin(kMCValueTypeCodeString, kMCStringTypeInfo) &&
        __MCTypeInfoCreateBuiltin(kMCValueTypeCodeData, kMCDataTypeInfo) &&
        __MCTypeInfoCreateBuiltin(kMCValueTypeCodeArray, kMCArrayTypeInfo) &&
        __MCTypeInfoCreateBuiltin(kMCValueTypeCodeList, kMCListTypeInfo) &&
        __MCTypeInfoCreateBuiltin(kMCValueTypeCodeSet, kMCSetTypeInfo);
}

void __MCTypeInfoFinalize(void)
{
    MCValueRelease(kMCNullTypeInfo);
    MCValueRelease(kMCBooleanTypeInfo);
    MCValueRelease(kMCNumberTypeInfo);
    MCValueRelease(kMCStringTypeInfo);
    MCValueRelease(kMCDataTypeInfo);
    MCValueRelease(kMCArrayTypeInfo);
    MCValueRelease(kMCListTypeInfo);
    MCValueRelease(kMCSetTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////
