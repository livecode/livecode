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

#include <ffi.h>

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

MCTypeInfoRef kMCOptionalBooleanTypeInfo;
MCTypeInfoRef kMCOptionalNumberTypeInfo;
MCTypeInfoRef kMCOptionalStringTypeInfo;
MCTypeInfoRef kMCOptionalNameTypeInfo;
MCTypeInfoRef kMCOptionalDataTypeInfo;
MCTypeInfoRef kMCOptionalArrayTypeInfo;
MCTypeInfoRef kMCOptionalSetTypeInfo;
MCTypeInfoRef kMCOptionalListTypeInfo;
MCTypeInfoRef kMCOptionalProperListTypeInfo;

////////////////////////////////////////////////////////////////////////////////

static bool __MCTypeInfoIsNamed(MCTypeInfoRef self);

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

bool MCTypeInfoIsForeign(MCTypeInfoRef self)
{
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCTypeInfoTypeIsForeign;
}

bool MCTypeInfoIsEnum(MCTypeInfoRef self)
{
	return __MCTypeInfoGetExtendedTypeCode(self) == kMCValueTypeCodeEnum;
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
    // We require that source is concrete - this means that it must be a named
    // type.
    MCAssert(MCTypeInfoIsNamed(source));
    
    // Resolve the source type.
    MCResolvedTypeInfo t_resolved_source;
    if (!MCTypeInfoResolve(source, t_resolved_source))
    {
        MCAssert(false);
        return false;
    }
    
    // We require that target is resolvable.
    MCResolvedTypeInfo t_resolved_target;
    if (!MCTypeInfoResolve(target, t_resolved_target))
    {
        MCAssert(false);
        return false;
    }
    
    return MCResolvedTypeInfoConforms(t_resolved_source, t_resolved_target);
}

bool MCResolvedTypeInfoConforms(const MCResolvedTypeInfo& source, const MCResolvedTypeInfo& target)
{
    // If source and target are the same, we are done.
    if (source . named_type == target . named_type)
        return true;
    
    // If source is undefined, then target must be optional.
    if (source . named_type == kMCNullTypeInfo)
        return target . is_optional;
    
    // If the target is any, then all is well.
    if (target . named_type == kMCAnyTypeInfo)
        return true;
    
    // If source is of foreign type then target must be the source's bridge type
    // the source type, or one of the source's supertypes.
    if (MCTypeInfoIsForeign(source . type))
    {
        // Check to see if the target is the source's bridge type.
        if (source . type -> foreign . descriptor . bridgetype != kMCNullTypeInfo &&
            target . named_type == source . type -> foreign . descriptor . bridgetype)
            return true;
        
        // Now check to see if the target is one of the source's supertypes.
        for(MCTypeInfoRef t_supertype = source . type; t_supertype != kMCNullTypeInfo; t_supertype = t_supertype -> foreign . descriptor . basetype)
            if (target . named_type == t_supertype)
                return true;
        
        return false;
    }
    
    // If the target is of foreign type, then the source must be the target's
    // bridge type.
    if (MCTypeInfoIsForeign(target . type))
    {
        if (target . type -> foreign . descriptor . bridgetype != kMCNullTypeInfo &&
            target . type -> foreign . descriptor . bridgetype == source . named_type)
            return true;
        
        return false;
    }
    
	// If the target is of enum type, then the source type must
	// conform to the type of one of the enum's permitted values.
	if (MCTypeInfoIsEnum (target . type))
	{
		for (uindex_t i = 0; i < target . type -> enum_ . value_count; ++i)
		{
			MCValueRef t_enum_value = target . type -> enum_ . values[i];
			if (MCTypeInfoConforms (source . named_type,
			                        MCValueGetTypeInfo (t_enum_value)))
				return true;
		}
		return false;
	}

    // If the source is of record type, then the target must be the same type of
    // one of the source's super types.
    if (MCTypeInfoIsRecord(source . type))
    {
        // Now check to see if the target is one of the source's supertypes.
        for(MCTypeInfoRef t_supertype = source . type; t_supertype != kMCNullTypeInfo; t_supertype = t_supertype -> record . base)
            if (target . named_type == t_supertype)
                return true;
        
        return false;
    }
    
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

static ffi_type *__map_primitive_type(MCForeignPrimitiveType p_type)
{
    switch(p_type)
    {
        case kMCForeignPrimitiveTypeVoid:
            return &ffi_type_void;
        case kMCForeignPrimitiveTypeBool:
            if (sizeof(bool) == 1)
                return &ffi_type_uint8;
            if (sizeof(bool) == 2)
                return &ffi_type_uint16;
            if (sizeof(bool) == 4)
                return &ffi_type_uint32;
            return &ffi_type_uint64;
        case kMCForeignPrimitiveTypeUInt8:
            return &ffi_type_uint8;
        case kMCForeignPrimitiveTypeSInt8:
            return &ffi_type_sint8;
        case kMCForeignPrimitiveTypeUInt16:
            return &ffi_type_uint16;
        case kMCForeignPrimitiveTypeSInt16:
            return &ffi_type_sint16;
        case kMCForeignPrimitiveTypeUInt32:
            return &ffi_type_uint32;
        case kMCForeignPrimitiveTypeSInt32:
            return &ffi_type_sint32;
        case kMCForeignPrimitiveTypeUInt64:
            return &ffi_type_uint64;
        case kMCForeignPrimitiveTypeSInt64:
            return &ffi_type_sint64;
        case kMCForeignPrimitiveTypeFloat32:
            return &ffi_type_float;
        case kMCForeignPrimitiveTypeFloat64:
            return &ffi_type_double;
        case kMCForeignPrimitiveTypePointer:
            return &ffi_type_pointer;
    }
    
    MCUnreachable();
    
    return nil;
}

static bool __MCForeignTypeInfoComputeLayoutType(MCTypeInfoRef self)
{
    // If the typeinfo has a layout size of size 1, then it is just a value.
    if (self -> foreign . descriptor . layout_size == 1)
        self -> foreign . ffi_layout_type = __map_primitive_type(self -> foreign . descriptor . layout[0]);
    else
    {
        ffi_type *t_type;
        if (!MCMemoryNew(t_type))
            return false;
        if (!MCMemoryNewArray(self -> foreign . descriptor . layout_size + 1, t_type -> elements))
        {
            MCMemoryDelete(t_type);
            return false;
        }
        
        t_type -> alignment = 0;
        t_type -> type = FFI_TYPE_STRUCT;
        for(uindex_t i = 0; i < self -> foreign . descriptor . layout_size; i++)
            t_type -> elements[i] = __map_primitive_type(self -> foreign . descriptor . layout[i]);
        t_type -> elements[self -> foreign . descriptor . layout_size] = NULL;
        
        self -> foreign . ffi_layout_type = t_type;
    }
    
    return true;
}

bool MCForeignTypeInfoCreate(const MCForeignTypeDescriptor *p_descriptor, MCTypeInfoRef& r_typeinfo)
{
    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    if (!MCMemoryNewArray(p_descriptor -> layout_size, self -> foreign . descriptor . layout, self -> foreign . descriptor . layout_size))
        return false;
    
    self -> flags |= kMCTypeInfoTypeIsForeign;
    
    self -> foreign . descriptor . size = p_descriptor -> size;
    self -> foreign . descriptor . basetype = MCValueRetain(p_descriptor -> basetype);
    self -> foreign . descriptor . bridgetype = MCValueRetain(p_descriptor -> bridgetype);
    MCMemoryCopy(self -> foreign . descriptor . layout, p_descriptor -> layout, p_descriptor -> layout_size * sizeof(self -> foreign . descriptor . layout[0]));
    self -> foreign . descriptor . initialize = p_descriptor -> initialize;
    self -> foreign . descriptor . finalize = p_descriptor -> finalize;
    self -> foreign . descriptor . defined = p_descriptor -> defined;
    self -> foreign . descriptor . move = p_descriptor -> move;
    self -> foreign . descriptor . copy = p_descriptor -> copy;
    self -> foreign . descriptor . equal = p_descriptor -> equal;
    self -> foreign . descriptor . hash = p_descriptor -> hash;
    self -> foreign . descriptor . doimport = p_descriptor -> doimport;
    self -> foreign . descriptor . doexport = p_descriptor -> doexport;
    
    if (!__MCForeignTypeInfoComputeLayoutType(self))
    {
        MCValueRelease(self);
        return false;
    }
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

const MCForeignTypeDescriptor *MCForeignTypeInfoGetDescriptor(MCTypeInfoRef self)
{
    return &self -> foreign . descriptor;
}

void *MCForeignTypeInfoGetLayoutType(MCTypeInfoRef self)
{
    return self -> foreign . ffi_layout_type;
}

////////////////////////////////////////////////////////////////////////////////

bool MCRecordTypeInfoCreate(const MCRecordTypeFieldInfo *p_fields, index_t p_field_count, MCTypeInfoRef p_base, MCTypeInfoRef& r_typeinfo)
	
{
	MCTypeInfoRef t_resolved_base;
	t_resolved_base = kMCNullTypeInfo;
	if (p_base != kMCNullTypeInfo)
	{
		t_resolved_base = __MCTypeInfoResolve(p_base);
		MCAssert(MCTypeInfoIsRecord (t_resolved_base));
	}

	/* If the p_field_count < 0 then the p_fields are expected to be
	 * terminated by a custodian with name = nil. */
	if (p_field_count < 0)
		for (p_field_count = 0; p_fields[p_field_count].name != nil; ++p_field_count);

    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    if (!MCMemoryNewArray(p_field_count, self -> record . fields))
    {
        MCMemoryDelete(self);
        return false;
    }
    
    self -> flags |= kMCValueTypeCodeRecord;

	for (uindex_t i = 0; i < p_field_count; ++i)
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
    }
    self -> record . field_count = p_field_count;
    self -> record . base = MCValueRetain(t_resolved_base);
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

MCTypeInfoRef MCRecordTypeInfoGetBaseType(MCTypeInfoRef unresolved_self)
{
	MCTypeInfoRef self;
	self = __MCTypeInfoResolve(unresolved_self);
	MCAssert(MCTypeInfoIsRecord (self));
    return self -> record . base;
}

uindex_t MCRecordTypeInfoGetFieldCount(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert(MCTypeInfoIsRecord (self));
    return __MCRecordTypeInfoGetFieldCount(self);
}

uindex_t
__MCRecordTypeInfoGetFieldCount(MCTypeInfoRef self)
{
	/* Sum field counts of all base record types */
	uindex_t t_field_count;
	t_field_count = 0;
	while (self != kMCNullTypeInfo) {
		t_field_count += self -> record . field_count;
		self = MCRecordTypeInfoGetBaseType (self);
	}
    
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
MCRecordTypeInfoIsDerivedFrom(MCTypeInfoRef self,
                              MCTypeInfoRef other)
{
	MCAssert(MCTypeInfoIsRecord (self));
	MCAssert(MCTypeInfoIsRecord (other));
	return MCTypeInfoConforms(self, other);
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
    
	/* If the p_field_count < 0 then the p_fields are expected to be
	 * terminated by a custodian with name = nil. */
	if (p_field_count < 0)
		for (p_field_count = 0; p_fields[p_field_count].type != nil; ++p_field_count);

    if (!MCMemoryNewArray(p_field_count, self -> handler . fields))
    {
        MCMemoryDelete(self);
        return false;
    }
    
    self -> flags |= kMCValueTypeCodeHandler;

	for (uindex_t i = 0; i < p_field_count; ++i)
    {
        self -> handler . fields[i] . type = MCValueRetain(p_fields[i] . type);
        self -> handler . fields[i] . mode = p_fields[i] . mode;
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

static int
__MCEnumTypeInfoCreate_ValueCompare (const void *a,
                                     const void *b)
{
	/* Since this is called with interned values, it's okay just to do
	 * a pointer comparison. */
	return (int) ((intptr_t) a - (intptr_t) b);
}

bool
MCEnumTypeInfoCreate (const MCValueRef *p_values,
                      index_t p_value_count,
                      MCTypeInfoRef & r_typeinfo)
{
	/* If no explicit value count was provided, count the values by looking
	 * for the null custodian */
	if (p_value_count < 0)
		for (p_value_count = 0; p_values[p_value_count] != nil; ++p_value_count);

	/* Create the type info structure and allocate enough memory for the value
	 * array */
	__MCTypeInfo *self;
	if (!__MCValueCreate (kMCValueTypeCodeTypeInfo, self))
		return false;
	if (!MCMemoryNewArray (p_value_count, self->enum_.values))
	{
		MCMemoryDelete (self);
		return false;
	}

	self->flags |= kMCValueTypeCodeEnum;

	bool t_success = true;
	for (uindex_t i = 0; t_success && i < p_value_count; ++i)
	{
		/* Intern the value, to make it unique */
		MCValueRef t_interned;
		if (t_success)
			t_success = MCValueInter(p_values[i], self->enum_.values[i]);
	}

	if (t_success)
	{
		/* Put values in some canonical order */
		qsort (self->enum_.values,
		       p_value_count,
		       sizeof(*(self->enum_.values)),
		       __MCEnumTypeInfoCreate_ValueCompare);

		/* Ensure that values are unique */
		if (p_value_count > 1)
			for (uindex_t i = 1; i < p_value_count; ++i)
				MCAssert (self->enum_.values[i] != self->enum_.values[i-1]);

		self->enum_.value_count = p_value_count;
	}

	if (t_success)
		t_success = MCValueInterAndRelease(self, r_typeinfo);

	if (!t_success)
		MCValueRelease(self);

	return t_success;
}

uindex_t
MCEnumTypeInfoGetValueCount (MCTypeInfoRef p_unresolved)
{
	MCTypeInfoRef self = __MCTypeInfoResolve (p_unresolved);

	MCAssert (MCTypeInfoIsEnum (self));

	return self->enum_.value_count;
}

MCValueRef
MCEnumTypeInfoGetValue (MCTypeInfoRef p_unresolved,
                        uindex_t p_index)
{
	MCTypeInfoRef self = __MCTypeInfoResolve (p_unresolved);

	MCAssert (MCTypeInfoIsEnum (self));
	MCAssert (p_index < self->enum_.value_count);

	return self->enum_.values[p_index];
}

bool
MCEnumTypeInfoHasValue (MCTypeInfoRef p_unresolved,
                        const MCValueRef p_value)
{
	MCTypeInfoRef self = __MCTypeInfoResolve (p_unresolved);

	MCAssert (MCTypeInfoIsEnum (self));

	for (uindex_t i = 0; i < self->enum_.value_count; ++i)
		if (MCValueIsEqualTo (p_value, self->enum_.values[i]))
			return true;

	return false;
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
    
	MCAssert (self -> named . typeinfo != nil);
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
    else if (t_ext_typecode == kMCTypeInfoTypeIsForeign)
    {
        MCValueRelease(self -> foreign . descriptor . basetype);
        MCValueRelease(self -> foreign . descriptor . bridgetype);
        MCMemoryDeleteArray(self -> foreign . descriptor . layout);
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
    else if (t_ext_typecode == kMCValueTypeCodeEnum)
    {
		for (uindex_t i = 0; i < self -> enum_ . value_count; ++i)
		{
			MCValueRelease (self -> enum_ . values[i]);
		}
		MCMemoryDeleteArray (self -> enum_ . values);
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
        // If the alias name is empty, then we treat it as a unique unnamed type.
        if (self -> alias . name == kMCEmptyName)
            t_hash = MCHashPointer(self);
        else
        {
            // Aliases are only equal if both name and type are the same. This is because
            // they are informative (for debugging purposes) rather than having any
            // semantic value.
            t_hash = MCHashBytesStream(t_hash, &self -> alias . name, sizeof(self -> alias . name));
            t_hash = MCHashBytesStream(t_hash, &self -> alias . typeinfo, sizeof(self -> alias . typeinfo));
        }
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
    else if (t_code == kMCTypeInfoTypeIsForeign)
    {
        // All foreign typeinfos are unique regardless of callbacks passed. So just hash the pointer.
        t_hash = MCHashPointer(self);
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
	else if (t_code == kMCValueTypeCodeEnum)
	{
		t_hash = MCHashBytesStream(t_hash, &self -> enum_ . value_count,
		                           sizeof(self -> enum_ . value_count));
		t_hash = MCHashBytesStream(t_hash, &self -> enum_ . values,
		                           (sizeof(*(self -> enum_ . values)) *
		                            self -> enum_ . value_count));
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
    {
        if (self -> named . name == kMCEmptyName || other_self -> named . name == kMCEmptyName)
            return false;
        return MCNameIsEqualTo(self -> named . name, other_self -> named . name);
    }
    
    if (t_code == kMCTypeInfoTypeIsOptional)
        return self -> optional . basetype == other_self -> optional . basetype;
    
    if (t_code == kMCTypeInfoTypeIsForeign)
        return self == other_self;
    
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
	else if (t_code == kMCValueTypeCodeEnum)
	{
		if (self -> enum_ . value_count != other_self -> enum_ . value_count)
			return false;
		for (uindex_t i = 0; i < self -> enum_ . value_count; ++i)
		{
			if (!MCValueIsEqualTo(self -> enum_ . values[i],
			                      other_self -> enum_ . values[i]))
				return false;
		}
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

static bool __create_named_builtin(MCNameRef p_name, MCValueTypeCode p_code, MCTypeInfoRef& r_typeinfo)
{
    MCAutoTypeInfoRef t_raw_typeinfo;
    if (!MCBuiltinTypeInfoCreate(p_code, &t_raw_typeinfo))
        return false;
    
    MCAutoTypeInfoRef t_typeinfo;
    if (!MCNamedTypeInfoCreate(p_name, &t_typeinfo))
        return false;
    
    if (!MCNamedTypeInfoBind(*t_typeinfo, *t_raw_typeinfo))
        return false;
    
    r_typeinfo = MCValueRetain(*t_typeinfo);
    
    return true;
}

static bool __create_named_builtin(MCNameRef p_name, MCValueTypeCode p_code, MCTypeInfoRef & r_typeinfo, MCTypeInfoRef & r_optional_typeinfo)
{
	if (!__create_named_builtin (p_name, p_code, r_typeinfo))
		return false;

	MCAutoTypeInfoRef t_optional;
	if (!MCOptionalTypeInfoCreate (r_typeinfo, &t_optional))
		return false;

	r_optional_typeinfo = MCValueRetain (*t_optional);
	return true;
}

bool __MCTypeInfoInitialize(void)
{
    return
        __create_named_builtin(MCNAME("livecode.lang.undefined"), kMCValueTypeCodeNull, kMCNullTypeInfo) &&
	    __create_named_builtin(MCNAME("livecode.lang.boolean"), kMCValueTypeCodeBoolean, kMCBooleanTypeInfo, kMCOptionalBooleanTypeInfo) &&
	    __create_named_builtin(MCNAME("livecode.lang.number"), kMCValueTypeCodeNumber, kMCNumberTypeInfo, kMCOptionalNumberTypeInfo) &&
	    __create_named_builtin(MCNAME("livecode.lang.string"), kMCValueTypeCodeString, kMCStringTypeInfo, kMCOptionalStringTypeInfo) &&
	    __create_named_builtin(MCNAME("livecode.lang.name"), kMCValueTypeCodeName, kMCNameTypeInfo, kMCOptionalNameTypeInfo) &&
	    __create_named_builtin(MCNAME("livecode.lang.data"), kMCValueTypeCodeData, kMCDataTypeInfo, kMCOptionalDataTypeInfo) &&
	    __create_named_builtin(MCNAME("livecode.lang.array"), kMCValueTypeCodeArray, kMCArrayTypeInfo, kMCOptionalArrayTypeInfo) &&
	    __create_named_builtin(MCNAME("livecode.lang.stringlist"), kMCValueTypeCodeList, kMCListTypeInfo, kMCOptionalListTypeInfo) &&
	    __create_named_builtin(MCNAME("livecode.lang.set"), kMCValueTypeCodeSet, kMCSetTypeInfo, kMCOptionalSetTypeInfo) &&
	    __create_named_builtin(MCNAME("livecode.lang.list"), kMCValueTypeCodeProperList, kMCProperListTypeInfo, kMCOptionalProperListTypeInfo) &&
        __create_named_builtin(MCNAME("livecode.lang.any"), kMCTypeInfoTypeIsAny, kMCAnyTypeInfo);
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
