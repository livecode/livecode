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

MCTypeInfoRef kMCAnyTypeInfo;
MCTypeInfoRef kMCNullTypeInfo;
MCTypeInfoRef kMCBooleanTypeInfo;
MCTypeInfoRef kMCNumberTypeInfo;
MCTypeInfoRef kMCStringTypeInfo;
MCTypeInfoRef kMCNameTypeInfo;
MCTypeInfoRef kMCDataTypeInfo;
MCTypeInfoRef kMCArrayTypeInfo;
MCTypeInfoRef kMCSetTypeInfo;
MCTypeInfoRef kMCListTypeInfo;
MCTypeInfoRef kMCProperListTypeInfo;

////////////////////////////////////////////////////////////////////////////////

static intenum_t __MCTypeInfoGetExtendedTypeCode(MCTypeInfoRef self)
{
    return (self -> flags & 0xff);
}

////////////////////////////////////////////////////////////////////////////////

bool MCTypeInfoIsAlias(MCTypeInfoRef self)
{
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCTypeInfoTypeIsAlias;
}

bool MCTypeInfoIsNamed(MCTypeInfoRef self)
{
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCTypeInfoTypeIsNamed;
}

bool MCTypeInfoIsOptional(MCTypeInfoRef self)
{
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCTypeInfoTypeIsOptional;
}

bool MCTypeInfoIsHandler(MCTypeInfoRef self)
{
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCValueTypeCodeHandler;
}

bool MCTypeInfoIsRecord(MCTypeInfoRef self)
{
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCValueTypeCodeRecord;
}

bool MCTypeInfoIsError(MCTypeInfoRef self)
{
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCValueTypeCodeError;
}

bool MCTypeInfoResolve(MCTypeInfoRef self, MCResolvedTypeInfo& r_resolution)
{
    intenum_t t_ext_typecode;
    t_ext_typecode = __MCTypeInfoGetExtendedTypeCode(self);

    if (t_ext_typecode == kMCTypeInfoTypeIsAlias)
        return MCTypeInfoResolve(self -> alias . typeinfo, r_resolution);

    if (t_ext_typecode == kMCTypeInfoTypeIsNamed)
    {
        // Attempt to resolve the binding, this will throw an error if it fails.
        MCTypeInfoRef t_next_type;
        if (!MCNamedTypeInfoResolve(self, t_next_type))
            return false;
        
        // We've successfully resolved the type, so return this one as the resolution.
        r_resolution . named_type = self;
        r_resolution . type = t_next_type;
        
        return true;
    }

    if (t_ext_typecode == kMCTypeInfoTypeIsOptional)
    {
        if (!MCTypeInfoResolve(self -> optional . basetype, r_resolution))
            return false;
        
        r_resolution . is_optional = true;
        
        return true;
    }
    
    // Resolving any other form of type, returns the (un-named) naked typeinfo.
    r_resolution . is_optional = false;
    r_resolution . named_type = nil;
    r_resolution . type = self;
    
    return true;
}

bool MCTypeInfoConforms(MCTypeInfoRef source, MCTypeInfoRef target)
{
    // If the typeinfos are the same, conformance is obvious.
    if (source == target)
        return true;
    
    // Otherwise we resolve source and target
    MCResolvedTypeInfo t_resolved_source, t_resolved_target;
    /* RESOLVE UNCHECKED */ MCTypeInfoResolve(source, t_resolved_source);
    /* RESOLVE UNCHECKED */ MCTypeInfoResolve(target, t_resolved_target);
    
    // Check null (undefined) type conformance.
    if (t_resolved_source . type == kMCNullTypeInfo)
        return t_resolved_target . is_optional;
    
    // Check any type conformance - this is fine as source is only optional if
    // target is.
    if (t_resolved_target . type == kMCAnyTypeInfo)
        return !t_resolved_source . is_optional || t_resolved_target . is_optional;
    
    // Check record conformance - the named source type has to be either the named
    // target type, or one of its super-types.
    if (MCTypeInfoIsRecord(t_resolved_target . type))
    {
        // Check up the base-type chain.
        MCTypeInfoRef t_target_type;
        for(t_target_type = t_resolved_target . named_type; t_target_type != nil; t_target_type = t_target_type -> named . typeinfo -> record . base)
            if (t_resolved_source . named_type != t_resolved_target . named_type)
                return false;
        
        // Now it comes down to optionality.
        return !t_resolved_source . is_optional || t_resolved_target . is_optional;
    }
    
    // If the source and target underlying types are the same and they are not records,
    // then its up to optionality to decide conformance.
    if (t_resolved_target . type == t_resolved_source . type)
        return !t_resolved_source . is_optional || t_resolved_target . is_optional;
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCBuiltinTypeInfoCreate(MCValueTypeCode p_code, MCTypeInfoRef& r_typeinfo)
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

////////////////////////////////////////////////////////////////////////////////

bool MCAliasTypeInfoCreate(MCNameRef p_name, MCTypeInfoRef p_target, MCTypeInfoRef& r_typeinfo)
{
    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    self -> flags |= kMCTypeInfoTypeIsAlias & 0xff;
    self -> alias . name = MCValueRetain(p_name);
    self -> alias . typeinfo = MCValueRetain(p_target);
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

MCNameRef MCAliasTypeInfoGetName(MCTypeInfoRef self)
{
    return self -> alias . name;
}

MCTypeInfoRef MCAliasTypeInfoGetTarget(MCTypeInfoRef self)
{
    return self -> alias . typeinfo;
}

////////////////////////////////////////////////////////////////////////////////

bool MCNamedTypeInfoCreate(MCNameRef p_name, MCTypeInfoRef& r_typeinfo)
{
    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    self -> flags |= kMCTypeInfoTypeIsNamed & 0xff;
    self -> named . name = MCValueRetain(p_name);
    
    // Note that we don't do anything with the 'typeinfo' field of the named typeinfo.
    // This is because it does not form part of the uniqueness of the typeinfo, thus
    // when we inter we get an existing named typeinfo with the same name.
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

bool MCNamedTypeInfoIsBound(MCTypeInfoRef self)
{
    return self -> named . typeinfo != nil;
}

MCTypeInfoRef MCNamedTypeInfoGetBoundTypeInfo(MCTypeInfoRef self)
{
    return self -> named . typeinfo;
}

bool MCNamedTypeInfoBind(MCTypeInfoRef self, MCTypeInfoRef p_target)
{
    if (self -> named . typeinfo != nil)
        return MCErrorThrowGeneric();
    
    self -> named . typeinfo = MCValueRetain(p_target);
    
    return true;
}

bool MCNamedTypeInfoUnbind(MCTypeInfoRef self)
{
    if (self -> named . typeinfo == nil)
        return MCErrorThrowGeneric();
    
    MCValueRelease(self -> named . typeinfo);
    self -> named . typeinfo = nil;
    
    return true;
}

bool MCNamedTypeInfoResolve(MCTypeInfoRef self, MCTypeInfoRef& r_bound_type)
{
    if (self -> named . typeinfo == nil)
        return MCErrorThrowGeneric();
    
    r_bound_type = self -> named . typeinfo;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCOptionalTypeInfoCreate(MCTypeInfoRef p_base, MCTypeInfoRef& r_new_type)
{
    if (__MCTypeInfoGetExtendedTypeCode(p_base) == kMCTypeInfoTypeIsOptional)
    {
        r_new_type = MCValueRetain(p_base);
        return true;
    }
    
    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    self -> flags |= kMCTypeInfoTypeIsOptional;
    self -> optional . basetype = MCValueRetain(p_base);
    
    if (MCValueInterAndRelease(self, r_new_type))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

MCTypeInfoRef MCOptionalTypeInfoGetBaseTypeInfo(MCTypeInfoRef p_base)
{
    return p_base -> optional . basetype;
}

////////////////////////////////////////////////////////////////////////////////

bool MCCustomTypeInfoCreate(const MCValueCustomCallbacks *callbacks, MCTypeInfoRef r_typeinfo)
{
    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    self -> flags |= kMCValueTypeCodeCustom;
    self -> custom . callbacks = *callbacks;
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCRecordTypeInfoCreate(const MCRecordTypeFieldInfo *p_fields, index_t p_field_count, MCTypeInfoRef p_base, MCTypeInfoRef& r_typeinfo)
{
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
        self -> record . fields[i] . name = MCValueRetain(p_fields[i] . name);
        self -> record . fields[i] . type = MCValueRetain(p_fields[i] . type);
		++i;
    }
    self -> record . field_count = p_field_count;
    self -> record . base = MCValueRetain(p_base != nil ? p_base : kMCNullTypeInfo);
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

MCTypeInfoRef MCRecordTypeInfoGetBaseType(MCTypeInfoRef self)
{
    return self -> record . base;
}

uindex_t MCRecordTypeInfoGetFieldCount(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeRecord);
    
    return self -> record . field_count;
}

MCNameRef MCRecordTypeInfoGetFieldName(MCTypeInfoRef unresolved_self, uindex_t p_index)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeRecord);
    MCAssert(self -> record . field_count > p_index);
    
    return self -> record . fields[p_index] . name;
}

MCTypeInfoRef MCRecordTypeInfoGetFieldType(MCTypeInfoRef unresolved_self, uindex_t p_index)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeRecord);
    MCAssert(self -> record . field_count > p_index);
    
    return self -> record . fields[p_index] . type;
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
	 * terminated by a custodian with type = nil. */
	while ((p_field_count >= 0) ? (i < p_field_count) : p_fields[i].type != nil)
    {
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
    
    self -> flags |= kMCValueTypeCodeError;
    
    self -> error . domain = MCValueRetain(p_domain);
    self -> error . message = MCValueRetain(p_message);
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
    
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

bool MCCustomTypeInfoCreate(const MCValueCustomCallbacks *p_callbacks, MCTypeInfoRef& r_typeinfo)
{
    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    self -> flags |= kMCValueTypeCodeCustom;
    
    self -> custom . callbacks = *p_callbacks;
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

const MCValueCustomCallbacks *MCCustomTypeInfoGetCallbacks(MCTypeInfoRef self)
{
    return &self -> custom . callbacks;
}

////////////////////////////////////////////////////////////////////////////////

MCTypeInfoRef __MCTypeInfoResolve(__MCTypeInfo *self)
{
    if (__MCTypeInfoGetExtendedTypeCode(self) != kMCTypeInfoTypeIsNamed)
        return self;
    
    return self -> named . typeinfo;
}

void __MCTypeInfoDestroy(__MCTypeInfo *self)
{
    intenum_t t_ext_typecode;
    t_ext_typecode = __MCTypeInfoGetExtendedTypeCode(self);
    
    if (t_ext_typecode == kMCTypeInfoTypeIsAlias)
    {
        MCValueRelease(self -> alias . name);
        MCValueRelease(self -> alias . typeinfo);
    }
    else if (t_ext_typecode == kMCTypeInfoTypeIsNamed)
    {
        MCValueRelease(self -> named . name);
        MCValueRelease(self -> named . typeinfo);
    }
    else if (t_ext_typecode == kMCTypeInfoTypeIsOptional)
    {
        MCValueRelease(self -> optional . basetype);
    }
    else if (t_ext_typecode == kMCValueTypeCodeRecord)
    {
        MCValueRelease(self -> record . base);
        for(uindex_t i = 0; i < self -> record . field_count; i++)
        {
            MCValueRelease(self -> record . fields[i] . name);
            MCValueRelease(self -> record . fields[i] . type);
        }
        MCMemoryDeleteArray(self -> record . fields);
    }
    else if (t_ext_typecode == kMCValueTypeCodeHandler)
    {
        for(uindex_t i = 0; i < self -> handler . field_count; i++)
        {
            MCValueRelease(self -> handler . fields[i] . type);
        }
        MCValueRelease(self -> handler . return_type);
        MCMemoryDeleteArray(self -> handler . fields);
    }
    else if (t_ext_typecode == kMCValueTypeCodeError)
    {
        MCValueRelease(self -> error . domain);
        MCValueRelease(self -> error . message);
    }
}

hash_t __MCTypeInfoHash(__MCTypeInfo *self)
{
    hash_t t_hash;
    t_hash = 0;
    
    intenum_t t_code;
    t_code = __MCTypeInfoGetExtendedTypeCode(self);
    
    t_hash = MCHashBytesStream(t_hash, &t_code, sizeof(uint32_t));
    if (t_code == kMCTypeInfoTypeIsAlias)
    {
        // Aliases are only equal if both name and type are the same. This is because
        // they are informative (for debugging purposes) rather than having any
        // semantic value.
        t_hash = MCHashBytesStream(t_hash, &self -> alias . name, sizeof(self -> alias . name));
        t_hash = MCHashBytesStream(t_hash, &self -> alias . typeinfo, sizeof(self -> alias . typeinfo));
    }
    else if (t_code == kMCTypeInfoTypeIsNamed)
    {
        // Named types are only hashed on the name as a named type can only be bound
        // to a single type at any one time (for obvious reasons!).
        t_hash = MCHashBytesStream(t_hash, &self -> alias . name, sizeof(self -> alias . name));
    }
    else if (t_code == kMCTypeInfoTypeIsOptional)
    {
        t_hash = MCHashBytesStream(t_hash, &self -> optional . basetype, sizeof(self -> optional . basetype));
    }
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
    else if (t_code == kMCValueTypeCodeCustom)
    {
        // All custom typeinfos are unique regardless of callbacks passed. So just hash the pointer.
        t_hash = MCHashPointer(self);
    }

    return t_hash;
}

bool __MCTypeInfoIsEqualTo(__MCTypeInfo *self, __MCTypeInfo *other_self)
{
    if (__MCTypeInfoGetExtendedTypeCode(self) != __MCTypeInfoGetExtendedTypeCode(other_self))
        return false;
    
    intenum_t t_code;
    t_code = __MCTypeInfoGetExtendedTypeCode(self);
    
    if (t_code == kMCTypeInfoTypeIsAlias)
        return MCNameIsEqualTo(self -> alias . name, other_self -> alias . name) &&
                self -> alias . typeinfo == other_self -> alias . typeinfo;
    
    if (t_code == kMCTypeInfoTypeIsNamed)
        return MCNameIsEqualTo(self -> named . name, other_self -> named . name);
    
    if (t_code == kMCTypeInfoTypeIsOptional)
        return self -> optional . basetype == other_self -> optional . basetype;
        
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
            if (self -> handler . fields[i] . type != other_self -> handler . fields[i] . type ||
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
    else if (t_code == kMCValueTypeCodeCustom)
    {
        if (self != other_self)
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
        MCBuiltinTypeInfoCreate(kMCValueTypeCodeNull, kMCNullTypeInfo) &&
        MCBuiltinTypeInfoCreate(kMCValueTypeCodeBoolean, kMCBooleanTypeInfo) &&
        MCBuiltinTypeInfoCreate(kMCValueTypeCodeNumber, kMCNumberTypeInfo) &&
        MCBuiltinTypeInfoCreate(kMCValueTypeCodeString, kMCStringTypeInfo) &&
        MCBuiltinTypeInfoCreate(kMCValueTypeCodeName, kMCNameTypeInfo) &&
        MCBuiltinTypeInfoCreate(kMCValueTypeCodeData, kMCDataTypeInfo) &&
        MCBuiltinTypeInfoCreate(kMCValueTypeCodeArray, kMCArrayTypeInfo) &&
        MCBuiltinTypeInfoCreate(kMCValueTypeCodeList, kMCListTypeInfo) &&
        MCBuiltinTypeInfoCreate(kMCValueTypeCodeSet, kMCSetTypeInfo) &&
        MCBuiltinTypeInfoCreate(kMCValueTypeCodeProperList, kMCProperListTypeInfo) &&
        MCBuiltinTypeInfoCreate(kMCTypeInfoTypeIsAny, kMCAnyTypeInfo);
}

void __MCTypeInfoFinalize(void)
{
    MCValueRelease(kMCNullTypeInfo);
    MCValueRelease(kMCBooleanTypeInfo);
    MCValueRelease(kMCNumberTypeInfo);
    MCValueRelease(kMCStringTypeInfo);
    MCValueRelease(kMCNameTypeInfo);
    MCValueRelease(kMCDataTypeInfo);
    MCValueRelease(kMCArrayTypeInfo);
    MCValueRelease(kMCListTypeInfo);
    MCValueRelease(kMCSetTypeInfo);
    MCValueRelease(kMCProperListTypeInfo);
    MCValueRelease(kMCAnyTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////
