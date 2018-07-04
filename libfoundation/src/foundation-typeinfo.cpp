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

#include <ffi.h>

#include "foundation-private.h"
#include "foundation-hash.h"

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCTypeInfoRef kMCAnyTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCNullTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCBooleanTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCNumberTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCStringTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCNameTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCDataTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCArrayTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCSetTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCListTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCProperListTypeInfo;

MC_DLLEXPORT_DEF MCTypeInfoRef MCAnyTypeInfo() { return kMCAnyTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCNullTypeInfo() { return kMCNullTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCBooleanTypeInfo() { return kMCBooleanTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCNumberTypeInfo() { return kMCNumberTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCStringTypeInfo() { return kMCStringTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCNameTypeInfo() { return kMCNameTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCDataTypeInfo() { return kMCDataTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCArrayTypeInfo() { return kMCArrayTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCSetTypeInfo() { return kMCSetTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCListTypeInfo() { return kMCListTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCProperListTypeInfo() { return kMCProperListTypeInfo; }

////////////////////////////////////////////////////////////////////////////////


static intenum_t __MCTypeInfoGetExtendedTypeCode(MCTypeInfoRef self)
{
    return (self -> flags & 0xff);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCTypeInfoIsAlias(MCTypeInfoRef self)
{
	__MCAssertIsTypeInfo(self);
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCTypeInfoTypeIsAlias;
}

MC_DLLEXPORT_DEF
bool MCTypeInfoIsNamed(MCTypeInfoRef self)
{
	__MCAssertIsTypeInfo(self);
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCTypeInfoTypeIsNamed;
}

MC_DLLEXPORT_DEF
bool MCTypeInfoIsOptional(MCTypeInfoRef self)
{
	__MCAssertIsTypeInfo(self);
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCTypeInfoTypeIsOptional;
}

MC_DLLEXPORT_DEF
bool MCTypeInfoIsHandler(MCTypeInfoRef self)
{
	__MCAssertIsTypeInfo(self);
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCValueTypeCodeHandler;
}

MC_DLLEXPORT_DEF
bool MCTypeInfoIsRecord(MCTypeInfoRef self)
{
	__MCAssertIsTypeInfo(self);
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCValueTypeCodeRecord;
}

MC_DLLEXPORT_DEF
bool MCTypeInfoIsError(MCTypeInfoRef self)
{
	__MCAssertIsTypeInfo(self);
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCValueTypeCodeError;
}

MC_DLLEXPORT_DEF
bool MCTypeInfoIsForeign(MCTypeInfoRef self)
{
	__MCAssertIsTypeInfo(self);
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCTypeInfoTypeIsForeign;
}

MC_DLLEXPORT_DEF
bool MCTypeInfoIsCustom(MCTypeInfoRef self)
{
	__MCAssertIsTypeInfo(self);
    return __MCTypeInfoGetExtendedTypeCode(self) == kMCValueTypeCodeCustom;
}

MC_DLLEXPORT_DEF
MCValueRef MCTypeInfoGetDefault(MCTypeInfoRef self)
{
    __MCAssertIsTypeInfo(self);
    switch(__MCTypeInfoGetExtendedTypeCode(self))
    {
        case kMCValueTypeCodeNull:
            return kMCNull;
        case kMCValueTypeCodeBoolean:
            return kMCFalse;
        case kMCValueTypeCodeNumber:
            return kMCZero;
        case kMCValueTypeCodeName:
            return kMCEmptyName;
        case kMCValueTypeCodeString:
            return kMCEmptyString;
        case kMCValueTypeCodeData:
            return kMCEmptyData;
        case kMCValueTypeCodeArray:
            return kMCEmptyArray;
        case kMCValueTypeCodeList:
            return kMCEmptyList;
        case kMCValueTypeCodeSet:
            return kMCEmptySet;
        case kMCValueTypeCodeProperList:
            return kMCEmptyProperList;
        case kMCValueTypeCodeCustom:
            return nil;
        case kMCValueTypeCodeRecord:
            return nil;
        case kMCValueTypeCodeHandler:
            return nil;
        case kMCValueTypeCodeTypeInfo:
            return nil;
        case kMCValueTypeCodeError:
            return nil;
        case kMCValueTypeCodeForeignValue:
            return nil;
        
        case kMCTypeInfoTypeIsOptional:
            return kMCNull;
            
        case kMCTypeInfoTypeIsAlias:
            return MCTypeInfoGetDefault(self -> alias . typeinfo);
            
        case kMCTypeInfoTypeIsNamed:
            return MCTypeInfoGetDefault(self -> named . typeinfo);
            
        default:
            return nil;
    }
}

MC_DLLEXPORT_DEF
bool MCTypeInfoResolve(MCTypeInfoRef self, MCResolvedTypeInfo& r_resolution)
{
	__MCAssertIsTypeInfo(self);

    intenum_t t_ext_typecode;
    t_ext_typecode = __MCTypeInfoGetExtendedTypeCode(self);

    if (t_ext_typecode == kMCTypeInfoTypeIsAlias)
        return MCTypeInfoResolve(self -> alias . typeinfo, r_resolution);

    if (t_ext_typecode == kMCTypeInfoTypeIsNamed)
    {
        // Attempt to resolve the binding, this will throw an error if it fails.
        MCTypeInfoRef t_next_type = nullptr;
        if (!MCNamedTypeInfoResolve(self, t_next_type))
            return false;
        
        // We've successfully resolved the type, so return this one as the resolution.
        r_resolution . named_type = self;
        r_resolution . type = t_next_type;
        r_resolution . is_optional = false;
        
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

MC_DLLEXPORT_DEF
bool MCTypeInfoConforms(MCTypeInfoRef source, MCTypeInfoRef target)
{
    // We require that source is concrete for all but handler types (as handlers
    // have unnamed typeinfos which we need to compare with potentially named
    // handler type typeinfos).
    MCAssert(MCTypeInfoIsNamed(source) || MCTypeInfoIsHandler(source) || MCTypeInfoIsOptional(source));
	
	// If the two types are the same, they conform.
	if (source == target)
	{
		return true;
	}
	
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

MC_DLLEXPORT_DEF
bool MCResolvedTypeInfoConforms(const MCResolvedTypeInfo& source, const MCResolvedTypeInfo& target)
{
    // If source and target are the same, we are done - as they are named types.
    if (source . named_type != nil &&
        source . named_type == target . named_type)
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
        // If both sides are foreign, do they have a bridge type in common?
        if (MCTypeInfoIsForeign(target.type))
        {
            if (source.type->foreign.descriptor.bridgetype != kMCNullTypeInfo &&
                source.type->foreign.descriptor.bridgetype == target.type->foreign.descriptor.bridgetype)
                return true;
        }
        
        // Check to see if the target is the source's bridge type.
        if (source . type -> foreign . descriptor . bridgetype != kMCNullTypeInfo &&
            target . named_type == source . type -> foreign . descriptor . bridgetype)
            return true;
        
        // Now check to see if the target is one of the source's supertypes.
        for(MCTypeInfoRef t_supertype = source . type; t_supertype != kMCNullTypeInfo; t_supertype = __MCTypeInfoResolve(t_supertype) -> foreign . descriptor . basetype)
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
    
    // If the source is of record type, then the target must be the same type.
    if (MCTypeInfoIsRecord(source . type))
    {
        return false;
    }
    
    // If the source is of custom type, then the target must be the same type or
    // one of the source's super types.
    if (MCTypeInfoIsCustom(source . type))
    {
        // Now check to see if the target is one of the source's supertypes.
        for(MCTypeInfoRef t_supertype = source . type; t_supertype != kMCNullTypeInfo; t_supertype = __MCTypeInfoResolve(t_supertype) -> custom . base)
            if (target . named_type == t_supertype)
                return true;
        
        return false;
    }
    
    // If the source is a handler type then we must check conformance with the
    // dst handler type.
    if (MCTypeInfoIsHandler(source . type))
    {
        // If the other type is not a handler, then we are done.
        if (!MCTypeInfoIsHandler(target . type))
            return false;
        
        // The number of parameters must conform.
        if (MCHandlerTypeInfoGetParameterCount(source . type) != MCHandlerTypeInfoGetParameterCount(target . type))
            return false;
        
        // The source return type must conform to the target (i.e. the return value
        // of the concrete handler, must be assignable to the return value of the
        // abstract handler).
        if (!MCTypeInfoConforms(MCHandlerTypeInfoGetReturnType(source . type), MCHandlerTypeInfoGetReturnType(target . type)))
            return false;
    
        // The modes of each parameter must match, and conformance must correspond to the
        // mode.
        for(uindex_t i = 0; i < MCHandlerTypeInfoGetParameterCount(source . type); i++)
        {
            if (MCHandlerTypeInfoGetParameterMode(source . type, i) != MCHandlerTypeInfoGetParameterMode(target . type, i))
                return false;
            
            // Out parameters - source must conform to target.
            if (MCHandlerTypeInfoGetParameterMode(source . type, i) != kMCHandlerTypeFieldModeOut)
            {
                if (!MCTypeInfoConforms(MCHandlerTypeInfoGetParameterType(source . type, i), MCHandlerTypeInfoGetParameterType(target . type, i)))
                    return false;
            }
            
            // In parameters - target must conform to source.
            if (MCHandlerTypeInfoGetParameterMode(source . type, i) != kMCHandlerTypeFieldModeIn)
            {
                if (!MCTypeInfoConforms(MCHandlerTypeInfoGetParameterType(target . type, i), MCHandlerTypeInfoGetParameterType(source . type, i)))
                    return false;
            }
        }
        
        return true;
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
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

MC_DLLEXPORT_DEF
bool MCAliasTypeInfoCreate(MCNameRef p_name, MCTypeInfoRef p_target, MCTypeInfoRef& r_typeinfo)
{
	__MCAssertIsName(p_name);
	__MCAssertIsTypeInfo(p_target);

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

MC_DLLEXPORT_DEF
MCNameRef MCAliasTypeInfoGetName(MCTypeInfoRef self)
{
	MCAssert(MCTypeInfoIsAlias(self));
    return self -> alias . name;
}

MC_DLLEXPORT_DEF
MCTypeInfoRef MCAliasTypeInfoGetTarget(MCTypeInfoRef self)
{
	MCAssert(MCTypeInfoIsAlias(self));
    return self -> alias . typeinfo;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCNamedTypeInfoCreate(MCNameRef p_name, MCTypeInfoRef& r_typeinfo)
{
	__MCAssertIsName(p_name);

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

MC_DLLEXPORT_DEF
MCNameRef MCNamedTypeInfoGetName(MCTypeInfoRef self)
{
	MCAssert(MCTypeInfoIsNamed(self));
    return self -> named . name;
}

MC_DLLEXPORT_DEF
bool MCNamedTypeInfoIsBound(MCTypeInfoRef self)
{
	MCAssert(MCTypeInfoIsNamed(self));
    return self -> named . typeinfo != nil;
}

MC_DLLEXPORT_DEF
MCTypeInfoRef MCNamedTypeInfoGetBoundTypeInfo(MCTypeInfoRef self)
{
	MCAssert(MCTypeInfoIsNamed(self));
    return self -> named . typeinfo;
}

MC_DLLEXPORT_DEF
bool MCNamedTypeInfoBind(MCTypeInfoRef self, MCTypeInfoRef p_target)
{
	MCAssert(MCTypeInfoIsNamed(self));
	__MCAssertIsTypeInfo(p_target);
    if (self -> named . typeinfo != nil)
        return MCErrorThrowGenericWithMessage(MCSTR("Can't bind typeinfo %{name}: already bound to %{self}"),
                                              "name", p_target->named.name,
                                              "self", self->named.name,
                                              nullptr);
    
    self -> named . typeinfo = MCValueRetain(p_target);
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCNamedTypeInfoUnbind(MCTypeInfoRef self)
{
	MCAssert(MCTypeInfoIsNamed(self));

    if (self -> named . typeinfo == nil)
        return MCErrorThrowGeneric(MCSTR("Can't unbind typeinfo: not bound"));
    
    MCValueRelease(self -> named . typeinfo);
    self -> named . typeinfo = nil;
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCNamedTypeInfoResolve(MCTypeInfoRef self, MCTypeInfoRef& r_bound_type)
{
	MCAssert(MCTypeInfoIsNamed(self));

    if (self -> named . typeinfo == nil)
        return MCErrorThrowGeneric(MCSTR("Can't resolve typeinfo: not bound"));
    
    r_bound_type = self -> named . typeinfo;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCOptionalTypeInfoCreate(MCTypeInfoRef p_base, MCTypeInfoRef& r_new_type)
{
	__MCAssertIsTypeInfo(p_base);

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

MC_DLLEXPORT_DEF
MCTypeInfoRef MCOptionalTypeInfoGetBaseTypeInfo(MCTypeInfoRef p_base)
{
	MCAssert(MCTypeInfoIsOptional(p_base));

    return p_base -> optional . basetype;
}

////////////////////////////////////////////////////////////////////////////////

static ffi_type *__map_primitive_type(MCForeignPrimitiveType p_type)
{
    switch(p_type)
    {
        case kMCForeignPrimitiveTypeVoid:
            return &ffi_type_void;
        case kMCForeignPrimitiveTypeBool:
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

MC_DLLEXPORT_DEF
bool MCForeignTypeInfoCreate(const MCForeignTypeDescriptor *p_descriptor, MCTypeInfoRef& r_typeinfo)
{
	MCAssert(nil != p_descriptor);

    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    if (!MCMemoryNewArray(p_descriptor -> layout_size, self -> foreign . descriptor . layout, self -> foreign . descriptor . layout_size))
	{
		MCValueRelease (self);
        return false;
	}
    
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
    self -> foreign . descriptor . describe = p_descriptor -> describe;
    self -> foreign . descriptor . promotedtype = MCValueRetain(p_descriptor->promotedtype);
    self -> foreign . descriptor . promote = p_descriptor -> promote;
    
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

MC_DLLEXPORT_DEF
const MCForeignTypeDescriptor *MCForeignTypeInfoGetDescriptor(MCTypeInfoRef unresolved_self)
{
	MCTypeInfoRef self;
	self = __MCTypeInfoResolve(unresolved_self);

	MCAssert(MCTypeInfoIsForeign(self));

    return &self -> foreign . descriptor;
}

MC_DLLEXPORT_DEF
void *MCForeignTypeInfoGetLayoutType(MCTypeInfoRef unresolved_self)
{
	MCTypeInfoRef self;
	self = __MCTypeInfoResolve(unresolved_self);

	MCAssert(MCTypeInfoIsForeign(self));

    return self -> foreign . ffi_layout_type;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCRecordTypeInfoCreate(const MCRecordTypeFieldInfo *p_fields, index_t p_field_count, MCTypeInfoRef& r_typeinfo)
	
{
	MCAssert(nil != p_fields || p_field_count == 0);

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

	for (index_t i = 0; i < p_field_count; ++i)
	{
		__MCAssertIsName(p_fields[i].name);
		__MCAssertIsTypeInfo(p_fields[i].type);
		/* Verify that the field names are all caselessly distinct.
		 * N.b. O(N^2) algorithm is inefficient, but will only be run
		 * in debug builds and will only happen once per type. */
		for (index_t j = 0; j < i; ++j)
		{
			MCAssert(!MCNameIsEqualToCaseless(p_fields[i] . name, p_fields[j] . name));
		}
        self -> record . fields[i] . name = MCValueRetain(p_fields[i] . name);
        self -> record . fields[i] . type = MCValueRetain(p_fields[i] . type);
    }
    self -> record . field_count = p_field_count;
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

MC_DLLEXPORT_DEF
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
    return self->record.field_count;
}

MC_DLLEXPORT_DEF
MCNameRef MCRecordTypeInfoGetFieldName(MCTypeInfoRef unresolved_self, uindex_t p_index)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeRecord);

	MCAssert(p_index < self->record.field_count);
	return self -> record . fields[p_index] . name;
}

MC_DLLEXPORT_DEF
MCTypeInfoRef MCRecordTypeInfoGetFieldType(MCTypeInfoRef unresolved_self, uindex_t p_index)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeRecord);

	MCAssert(p_index < self->record.field_count);
	return self -> record . fields[p_index] . type;
}

////////////////////////////////////////////////////////////////////////////////

static bool MCCommonHandlerTypeInfoCreate(bool p_is_foreign, const MCHandlerTypeFieldInfo *p_fields, index_t p_field_count, MCTypeInfoRef p_return_type, MCTypeInfoRef& r_typeinfo)
{
	__MCAssertIsTypeInfo(p_return_type);
	MCAssert(nil != p_fields || 0 == p_field_count);

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

    if (p_is_foreign)
        self -> flags |= kMCTypeInfoFlagHandlerIsForeign;

    for (index_t i = 0; i < p_field_count; ++i)
    {
	    __MCAssertIsTypeInfo(p_fields[i].type);
        
        if (p_fields[i].mode == kMCHandlerTypeFieldModeVariadic)
        {
            if (i == 0 || p_field_count != i + 1)
            {
                MCValueRelease(self);
                return MCErrorThrowGeneric(MCSTR("Variadic parameter cannot be first, and must be last"));
            }
            
            p_field_count = i;
            self->flags |= kMCTypeInfoFlagHandlerIsVariadic;
            break;
        }
        
        self -> handler . fields[i] . type = MCValueRetain(p_fields[i] . type);
        self -> handler . fields[i] . mode = p_fields[i] . mode;
    }
    self -> handler . field_count = p_field_count;
    
    self -> handler . return_type = MCValueRetain(p_return_type);
    
    self -> handler . layout_args= nil;
    self -> handler . layouts = nil;
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

MC_DLLEXPORT_DEF
bool MCHandlerTypeInfoCreate(const MCHandlerTypeFieldInfo *p_fields, index_t p_field_count, MCTypeInfoRef p_return_type, MCTypeInfoRef& r_typeinfo)
{
    return MCCommonHandlerTypeInfoCreate(false, p_fields, p_field_count, p_return_type, r_typeinfo);
}

MC_DLLEXPORT_DEF
bool MCForeignHandlerTypeInfoCreate(const MCHandlerTypeFieldInfo *p_fields, index_t p_field_count, MCTypeInfoRef p_return_type, MCTypeInfoRef& r_typeinfo)
{
    return MCCommonHandlerTypeInfoCreate(true, p_fields, p_field_count, p_return_type, r_typeinfo);
}

MC_DLLEXPORT_DEF
bool MCHandlerTypeInfoIsForeign(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);

    MCAssert(MCTypeInfoIsHandler(self));

    return (self -> flags & kMCTypeInfoFlagHandlerIsForeign) != 0;
}

MC_DLLEXPORT_DEF
bool MCHandlerTypeInfoIsVariadic(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);

    MCAssert(MCTypeInfoIsHandler(self));

    return (self -> flags & kMCTypeInfoFlagHandlerIsVariadic) != 0;
}

MC_DLLEXPORT_DEF
MCTypeInfoRef MCHandlerTypeInfoGetReturnType(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeHandler);
    
    return self -> handler . return_type;
}

MC_DLLEXPORT_DEF
uindex_t MCHandlerTypeInfoGetParameterCount(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);

    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeHandler);
    
    return self -> handler . field_count;
}

MC_DLLEXPORT_DEF
MCHandlerTypeFieldMode MCHandlerTypeInfoGetParameterMode(MCTypeInfoRef unresolved_self, uindex_t p_index)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);

    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeHandler);
    MCAssert(self -> handler . field_count > p_index);
    
    return self -> handler . fields[p_index] . mode;
}

MC_DLLEXPORT_DEF
MCTypeInfoRef MCHandlerTypeInfoGetParameterType(MCTypeInfoRef unresolved_self, uindex_t p_index)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);

    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeHandler);
    MCAssert(self -> handler . field_count > p_index);
    
    return self -> handler . fields[p_index] . type;
}

bool MCHandlerTypeInfoGetLayoutType(MCTypeInfoRef unresolved_self, int p_abi, void*& r_cif)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert((self -> flags & kMCTypeInfoTypeCodeMask) == kMCValueTypeCodeHandler);
    
    // If a layout for the given ABI already exists, then return it.
    for(MCHandlerTypeLayout *t_layout = self -> handler . layouts; t_layout != nil; t_layout = t_layout -> next)
        if (t_layout -> abi == p_abi)
        {
            r_cif = &t_layout -> cif;
            return true;
        }
    
    // If we haven't computed the layout args yet, do so.
    if (self -> handler . layout_args == nil)
    {
        MCTypeInfoRef t_return_type;
        t_return_type = self -> handler . return_type;
        
        MCResolvedTypeInfo t_resolved_return_type;
        if (!MCTypeInfoResolve(t_return_type, t_resolved_return_type))
            return MCErrorThrowUnboundType(t_return_type);
        
        ffi_type *t_ffi_return_type;
        if (t_resolved_return_type.named_type != kMCNullTypeInfo)
        {
            if (MCTypeInfoIsForeign(t_resolved_return_type . type))
                t_ffi_return_type = (ffi_type *)MCForeignTypeInfoGetLayoutType(t_resolved_return_type . type);
            else
                t_ffi_return_type = &ffi_type_pointer;
        }
        else
            t_ffi_return_type = &ffi_type_void;

        uindex_t t_arity;
        t_arity = self -> handler . field_count;
        
        // We need arity + 1 ffi_type slots, as we use the first slot to store
        // the return type (if any).
        MCAutoPointer<ffi_type*[]> t_ffi_arg_types =
            new (std::nothrow) ffi_type*[t_arity + 1];
        if (!t_ffi_arg_types)
            return false;
        
        t_ffi_arg_types[0] = t_ffi_return_type;
        
        for(uindex_t i = 0; i < t_arity; i++)
        {
            MCTypeInfoRef t_type;
            MCHandlerTypeFieldMode t_mode;
            t_type = self -> handler . fields[i] . type;
            t_mode = self -> handler . fields[i] . mode;
            
            MCResolvedTypeInfo t_resolved_type;
            if (!MCTypeInfoResolve(t_type, t_resolved_type))
                return MCErrorThrowUnboundType(t_type);
            
            if (t_mode == kMCHandlerTypeFieldModeIn)
            {
                if (MCTypeInfoIsForeign(t_resolved_type . type))
                    t_ffi_arg_types[i + 1] = (ffi_type *)MCForeignTypeInfoGetLayoutType(t_resolved_type . type);
                else
                    t_ffi_arg_types[i + 1] = &ffi_type_pointer;
            }
            else
                t_ffi_arg_types[i + 1] = &ffi_type_pointer;
        }
        
        self -> handler . layout_args = t_ffi_arg_types.Release();
    }
    
    // Now we must create a new layout object.
    MCHandlerTypeLayout *t_layout;
    if (!MCMemoryAllocate(sizeof(MCHandlerTypeLayout) + sizeof(ffi_cif), t_layout))
        return false;

	t_layout -> abi = p_abi;
    
    if (ffi_prep_cif((ffi_cif *)&t_layout -> cif, (ffi_abi)p_abi, self -> handler . field_count, self -> handler . layout_args[0], self -> handler . layout_args + 1) != FFI_OK)
    {
        MCMemoryDeallocate(t_layout);
        return MCErrorThrowGeneric(MCSTR("unexpected libffi failure"));
    }
    
    t_layout -> next = self -> handler . layouts;
    self -> handler . layouts = t_layout;
    
    r_cif = &t_layout -> cif;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCErrorTypeInfoCreate(MCNameRef p_domain, MCStringRef p_message, MCTypeInfoRef& r_typeinfo)
{
	__MCAssertIsName(p_domain);
	__MCAssertIsString(p_message);

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
}

MC_DLLEXPORT_DEF
MCNameRef MCErrorTypeInfoGetDomain(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
	MCAssert(MCTypeInfoIsError(self));
    return self -> error . domain;
}

MC_DLLEXPORT_DEF
MCStringRef MCErrorTypeInfoGetMessage(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);
    
    MCAssert(MCTypeInfoIsError(self));
    return self -> error . message;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCNamedErrorTypeInfoCreate(MCNameRef p_name, MCNameRef p_domain, MCStringRef p_message, MCTypeInfoRef &r_typeinfo)
{
	MCAutoTypeInfoRef t_type, t_named_type;
	
	if (!MCErrorTypeInfoCreate(p_domain, p_message, &t_type))
		return false;
	
	if (!MCNamedTypeInfoCreate(p_name, &t_named_type))
		return false;
	
	if (!MCNamedTypeInfoBind(*t_named_type, *t_type))
		return false;
	
	r_typeinfo = MCValueRetain(*t_named_type);
	
	return true;
}

MC_DLLEXPORT_DEF
bool MCNamedCustomTypeInfoCreate(MCNameRef p_name, MCTypeInfoRef base, const MCValueCustomCallbacks *callbacks, MCTypeInfoRef& r_typeinfo)
{
	MCAutoTypeInfoRef t_type, t_named_type;
	
	if (!MCCustomTypeInfoCreate(base, callbacks, &t_type))
		return false;
	
	if (!MCNamedTypeInfoCreate(p_name, &t_named_type))
		return false;
	
	if (!MCNamedTypeInfoBind(*t_named_type, *t_type))
		return false;
	
	r_typeinfo = MCValueRetain(*t_named_type);
	
	return true;
}

MC_DLLEXPORT_DEF
bool MCNamedForeignTypeInfoCreate(MCNameRef p_name, const MCForeignTypeDescriptor *p_descriptor, MCTypeInfoRef& r_typeinfo)
{
	MCAutoTypeInfoRef t_type, t_named_type;
	
	if (!MCForeignTypeInfoCreate(p_descriptor, &t_type))
		return false;
	
	if (!MCNamedTypeInfoCreate(p_name, &t_named_type))
		return false;
	
	if (!MCNamedTypeInfoBind(*t_named_type, *t_type))
		return false;
	
	r_typeinfo = MCValueRetain(*t_named_type);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCCustomTypeInfoCreate(MCTypeInfoRef p_base, const MCValueCustomCallbacks *p_callbacks, MCTypeInfoRef& r_typeinfo)
{
	__MCAssertIsTypeInfo(p_base);
	MCAssert(nil != p_callbacks);

    __MCTypeInfo *self;
    if (!__MCValueCreate(kMCValueTypeCodeTypeInfo, self))
        return false;
    
    self -> flags |= kMCValueTypeCodeCustom;
    self -> custom . callbacks = *p_callbacks;
    self -> custom . base = MCValueRetain(p_base);
    
    if (MCValueInterAndRelease(self, r_typeinfo))
        return true;
    
    MCValueRelease(self);
    
    return false;
}

MCTypeInfoRef MCCustomTypeInfoGetBaseType(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);

    MCAssert(MCTypeInfoIsCustom(self));
    return self -> custom . base;
}

MC_DLLEXPORT_DEF
const MCValueCustomCallbacks *MCCustomTypeInfoGetCallbacks(MCTypeInfoRef unresolved_self)
{
    MCTypeInfoRef self;
    self = __MCTypeInfoResolve(unresolved_self);

    MCAssert(MCTypeInfoIsCustom(self));
    return &self -> custom . callbacks;
}

////////////////////////////////////////////////////////////////////////////////

MCTypeInfoRef __MCTypeInfoResolve(__MCTypeInfo *self)
{
	__MCAssertIsTypeInfo(self);

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
        MCValueRelease(self->foreign.descriptor.promotedtype);
    }
    else if (t_ext_typecode == kMCValueTypeCodeRecord)
    {
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
        delete[] self -> handler . layout_args;
        while(self -> handler . layouts != nil)
        {
            MCHandlerTypeLayout *t_layout;
            t_layout = self -> handler . layouts;
            self -> handler . layouts = self -> handler . layouts -> next;
            MCMemoryDeallocate(t_layout);
        }
    }
    else if (t_ext_typecode == kMCValueTypeCodeError)
    {
        MCValueRelease(self -> error . domain);
        MCValueRelease(self -> error . message);
	}
    else if (t_ext_typecode == kMCValueTypeCodeCustom)
    {
		MCValueRelease(self -> custom . base);
    }
}

hash_t __MCTypeInfoHash(__MCTypeInfo *self)
{
    hash_t t_hash;
    t_hash = 0;
    
    intenum_t t_code;
    t_code = __MCTypeInfoGetExtendedTypeCode(self);
    
    t_hash = MCHashObjectStream(t_hash, t_code);
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
            t_hash = MCHashObjectStream(t_hash,
                                        self -> alias . name,
                                        self -> alias . typeinfo);
        }
    }
    else if (t_code == kMCTypeInfoTypeIsNamed)
    {
        // Named types are only hashed on the name as a named type can only be bound
        // to a single type at any one time (for obvious reasons!).
        t_hash = MCHashObjectStream(t_hash, self -> alias . name);
    }
    else if (t_code == kMCTypeInfoTypeIsOptional)
    {
        t_hash = MCHashObjectStream(t_hash, self -> optional . basetype);
    }
    else if (t_code == kMCTypeInfoTypeIsForeign)
    {
        // All foreign typeinfos are unique regardless of callbacks passed. So just hash the pointer.
        t_hash = MCHashPointer(self);
    }
    else if (t_code == kMCValueTypeCodeRecord)
    {
        t_hash = MCHashObjectStream(t_hash, self -> record . field_count);
        t_hash = MCHashSpanStream(t_hash,
                                  MCMakeSpan(self -> record . fields,
                                             self -> record . field_count));
    }
    else if (t_code == kMCValueTypeCodeHandler)
    {
        t_hash = MCHashObjectStream(t_hash,
                                    self -> handler . field_count,
                                    self -> handler . return_type);
        t_hash = MCHashSpanStream(t_hash,
                                  MCMakeSpan(self -> handler . fields,
                                             self -> handler . field_count));
    }
    else if (t_code == kMCValueTypeCodeError)
    {
        t_hash = MCHashObjectStream(t_hash,
                                    self -> error . domain,
                                    self -> error . message);
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
        return MCNameIsEqualToCaseless(self -> alias . name, other_self -> alias . name) &&
                self -> alias . typeinfo == other_self -> alias . typeinfo;
    
    if (t_code == kMCTypeInfoTypeIsNamed)
    {
        if (self -> named . name == kMCEmptyName || other_self -> named . name == kMCEmptyName)
            return false;
        return MCNameIsEqualToCaseless(self -> named . name, other_self -> named . name);
    }
    
    if (t_code == kMCTypeInfoTypeIsOptional)
        return self -> optional . basetype == other_self -> optional . basetype;
    
    if (t_code == kMCTypeInfoTypeIsForeign)
        return self == other_self;
    
    if (t_code == kMCValueTypeCodeRecord)
    {
		return self == other_self;
    }
    else if (t_code == kMCValueTypeCodeHandler)
    {
        if ((self -> flags & kMCTypeInfoFlagHandlerIsForeign) != (other_self -> flags & kMCTypeInfoFlagHandlerIsForeign))
            return false;
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
	MCAutoStringRef tOptionalPart;
	if (MCTypeInfoIsOptional (self))
		tOptionalPart = MCSTR("optional ");
	else
		tOptionalPart = kMCEmptyString;

	MCAutoStringRef tNamePart;
	if (MCTypeInfoIsNamed (self))
	{
		tNamePart = MCNameGetString (MCNamedTypeInfoGetName (self));
	}
	else
	{
		if (!MCStringFormat (&tNamePart, "unnamed[%p]", self))
			return false;
	}

	return MCStringFormat (r_description, "<type: %@%@>",
	                       *tOptionalPart, *tNamePart);
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

bool __MCTypeInfoInitialize(void)
{
    return
        __create_named_builtin(MCNAME("livecode.lang.undefined"), kMCValueTypeCodeNull, kMCNullTypeInfo) &&
        __create_named_builtin(MCNAME("livecode.lang.boolean"), kMCValueTypeCodeBoolean, kMCBooleanTypeInfo) &&
        __create_named_builtin(MCNAME("livecode.lang.number"), kMCValueTypeCodeNumber, kMCNumberTypeInfo) &&
        __create_named_builtin(MCNAME("livecode.lang.string"), kMCValueTypeCodeString, kMCStringTypeInfo) &&
        __create_named_builtin(MCNAME("livecode.lang.name"), kMCValueTypeCodeName, kMCNameTypeInfo) &&
        __create_named_builtin(MCNAME("livecode.lang.data"), kMCValueTypeCodeData, kMCDataTypeInfo) &&
        __create_named_builtin(MCNAME("livecode.lang.array"), kMCValueTypeCodeArray, kMCArrayTypeInfo) &&
        __create_named_builtin(MCNAME("livecode.lang.stringlist"), kMCValueTypeCodeList, kMCListTypeInfo) &&
        __create_named_builtin(MCNAME("livecode.lang.set"), kMCValueTypeCodeSet, kMCSetTypeInfo) &&
        __create_named_builtin(MCNAME("livecode.lang.list"), kMCValueTypeCodeProperList, kMCProperListTypeInfo) &&
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
