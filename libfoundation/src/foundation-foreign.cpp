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

MCTypeInfoRef kMCBoolTypeInfo;
MCTypeInfoRef kMCUIntTypeInfo;
MCTypeInfoRef kMCIntTypeInfo;
MCTypeInfoRef kMCFloatTypeInfo;
MCTypeInfoRef kMCDoubleTypeInfo;
MCTypeInfoRef kMCPointerTypeInfo;

////////////////////////////////////////////////////////////////////////////////

bool MCForeignValueCreate(MCTypeInfoRef p_typeinfo, void *p_contents, MCForeignValueRef& r_value)
{
    bool t_success;
    t_success = true;
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(p_typeinfo);
    
    __MCForeignValue *t_value;
    if (!__MCValueCreate(kMCValueTypeCodeForeignValue, p_typeinfo -> foreign . descriptor . size, (__MCValue*&)t_value))
        return false;
    
    if (!p_typeinfo -> foreign . descriptor . copy(p_contents, t_value + 1))
    {
        MCMemoryDelete(t_value);
        return false;
    }
    
    t_value -> typeinfo = MCValueRetain(p_typeinfo);
    
    r_value = t_value;
    
    return true;
}

bool MCForeignValueCreateAndRelease(MCTypeInfoRef p_typeinfo, void *p_contents, MCForeignValueRef& r_value)
{
    bool t_success;
    t_success = true;
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(p_typeinfo);
    
    __MCForeignValue *t_value;
    if (!__MCValueCreate(kMCValueTypeCodeForeignValue, p_typeinfo -> foreign . descriptor . size, (__MCValue*&)t_value))
        return false;
    
    if (!t_resolved_typeinfo -> foreign . descriptor . move(p_contents, t_value + 1))
    {
        MCMemoryDelete(t_value);
        return false;
    }
    
    t_value -> typeinfo = MCValueRetain(p_typeinfo);
    
    r_value = t_value;
    
    return true;
}

bool MCForeignValueExport(MCTypeInfoRef p_typeinfo, MCValueRef p_value, MCForeignValueRef& r_value)
{
    bool t_success;
    t_success = true;
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(p_typeinfo);
    
    __MCForeignValue *t_value;
    if (!__MCValueCreate(kMCValueTypeCodeForeignValue, p_typeinfo -> foreign . descriptor . size, (__MCValue*&)t_value))
        return false;

    if (!t_resolved_typeinfo -> foreign . descriptor . doexport(p_value, false, t_value + 1))
    {
        MCMemoryDelete(t_value);
        return false;
    }
    
    t_value -> typeinfo = MCValueRetain(p_typeinfo);
    
    r_value = t_value;
    
    return true;
}

void *MCForeignValueGetContentsPtr(MCValueRef self)
{
    return ((__MCForeignValue *)self) + 1;
}

////////////////////////////////////////////////////////////////////////////////

void __MCForeignValueDestroy(__MCForeignValue *self)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    t_resolved_typeinfo -> foreign . descriptor . finalize(self + 1);
}

hash_t __MCForeignValueHash(__MCForeignValue *self)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    hash_t t_hash;
    if (!t_resolved_typeinfo -> foreign . descriptor . hash(self + 1, t_hash))
        return 0;
    return t_hash;
}

bool __MCForeignValueIsEqualTo(__MCForeignValue *self, __MCForeignValue *other_self)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    bool t_result;
    if (!t_resolved_typeinfo -> foreign . descriptor . equal(self + 1, other_self + 1, t_result))
        return false;
    
    return t_result;
}

bool __MCForeignValueCopyDescription(__MCForeignValue *self, MCStringRef& r_description)
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////

// bool foreign type handlers

static void __bool_finalize(void *)
{
}

static bool __bool_copy(void *from, void *to)
{
    *(bool *)to = *(bool *)from; return true;
}

static bool __bool_equal(void *left, void *right, bool& r_equal)
{
    r_equal = *(bool *)left == *(bool *)right; return true;
}

static bool __bool_hash(void *value, hash_t& r_hash)
{
    r_hash = (hash_t)*(bool *)value; return true;
}

static bool __bool_import(void *contents, bool release, MCValueRef& r_value)
{
    if (*(bool *)contents)
        r_value = MCValueRetain(kMCTrue);
    else
        r_value = MCValueRetain(kMCFalse);
    return true;
}

static bool __bool_export(MCValueRef value, bool release, void *contents)
{
    *(bool *)contents = value == kMCTrue;
    if (release)
        MCValueRelease(value);
    return true;
}

// pointer handlers

static bool __pointer_initialize(void *contents)
{
    *(void **)contents = nil;
    return true;
}

static void __pointer_finalize(void *contents)
{
}

static bool __pointer_defined(void *contents)
{
    return *(void **)contents != nil;
}

static bool __pointer_copy(void *from, void *to)
{
    *(void **)to = *(void **)from;
    return true;
}

static bool __pointer_equal(void *left, void *right, bool& r_equal)
{
    r_equal = *(void **)left == *(void **)right;
    return true;
}

static bool __pointer_hash(void *value, hash_t& r_hash)
{
    r_hash = MCHashPointer(*(void **)value);
    return true;
}


// numeric foreign type handlers

template<typename T> static void __numeric_finalize(void *)
{
}

template<typename T> static bool __numeric_copy(void *from, void *to)
{
    *(T *)to = *(T *)from;
    return true;
}

template<typename T> static bool __numeric_equal(void *left, void *right, bool& r_equal)
{
    r_equal = *(T *)left == *(T *)right;
    return true;
}

static bool __int_hash(void *value, hash_t& r_hash)
{
    r_hash = MCHashInteger(*(integer_t *)value);
    return true;
}

static bool __float_hash(void *value, hash_t& r_hash)
{
    r_hash = MCHashDouble(*(float *)value);
    return true;
}

static bool __double_hash(void *value, hash_t& r_hash)
{
    r_hash = MCHashDouble(*(double *)value);
    return true;
}

static bool __int_import(void *contents, bool release, MCValueRef& r_value)
{
    return MCNumberCreateWithInteger(*(integer_t *)contents, (MCNumberRef&)r_value);
}

static bool __uint_import(void *contents, bool release, MCValueRef& r_value)
{
    return MCNumberCreateWithUnsignedInteger(*(uinteger_t *)contents, (MCNumberRef&)r_value);
}

static bool __float_import(void *contents, bool release, MCValueRef& r_value)
{
    return MCNumberCreateWithReal(*(float *)contents, (MCNumberRef&)r_value);
}

static bool __double_import(void *contents, bool release, MCValueRef& r_value)
{
    return MCNumberCreateWithReal(*(double *)contents, (MCNumberRef&)r_value);
}

static bool __int_export(MCValueRef value, bool release, void *contents)
{
    *(integer_t *)contents = MCNumberFetchAsInteger((MCNumberRef)value);
    if (release)
        MCValueRelease(value);
    return true;
}

static bool __uint_export(MCValueRef value, bool release, void *contents)
{
    *(uinteger_t *)contents = MCNumberFetchAsUnsignedInteger((MCNumberRef)value);
    if (release)
        MCValueRelease(value);
    return true;
}

static bool __float_export(MCValueRef value, bool release, void *contents)
{
    *(float *)contents = MCNumberFetchAsReal((MCNumberRef)value);
    if (release)
        MCValueRelease(value);
    return true;
}

static bool __double_export(MCValueRef value, bool release, void *contents)
{
    *(double *)contents = MCNumberFetchAsReal((MCNumberRef)value);
    if (release)
        MCValueRelease(value);
    return true;
}

static bool __build_typeinfo(const char *p_name, MCForeignTypeDescriptor *p_desc, MCTypeInfoRef& r_typeinfo)
{
    MCAutoStringRef t_name_string;
    if (!MCStringCreateWithCString(p_name, &t_name_string))
        return false;
    MCNewAutoNameRef t_name_name;
    if (!MCNameCreate(*t_name_string, &t_name_name))
        return false;
    
    MCAutoTypeInfoRef t_base_typeinfo;
    if (!MCForeignTypeInfoCreate(p_desc, &t_base_typeinfo))
        return false;
    
    MCTypeInfoRef t_named_typeinfo;
    if (!MCNamedTypeInfoCreate(*t_name_name, t_named_typeinfo))
        return false;
    if (!MCNamedTypeInfoBind(t_named_typeinfo, *t_base_typeinfo))
        return false;
    
    r_typeinfo = t_named_typeinfo;
    
    return true;
}

bool __MCForeignValueInitialize(void)
{
    MCForeignPrimitiveType p;
    MCForeignTypeDescriptor d;
    d . size = sizeof(bool);
    d . basetype = kMCNullTypeInfo;
    d . bridgetype = kMCBooleanTypeInfo;
    p = kMCForeignPrimitiveTypeBool;
    d . layout = &p;
    d . layout_size = 1;
    d . initialize = nil;
    d . finalize = __bool_finalize;
    d . defined = nil;
    d . move = __bool_copy;
    d . copy = __bool_copy;
    d . equal = __bool_equal;
    d . hash = __bool_hash;
    d . doimport = __bool_import;
    d . doexport = __bool_export;
    if (!__build_typeinfo("__builtin__.bool", &d, kMCBoolTypeInfo))
        return false;
    
    d . size = sizeof(integer_t);
    d . basetype = kMCNullTypeInfo;
    d . bridgetype = kMCNumberTypeInfo;
    p = kMCForeignPrimitiveTypeSInt32;
    d . layout = &p;
    d . layout_size = 1;
    d . initialize = nil;
    d . finalize = __numeric_finalize<integer_t>;
    d . defined = nil;
    d . move = __numeric_copy<integer_t>;
    d . copy = __numeric_copy<integer_t>;
    d . equal = __numeric_equal<integer_t>;
    d . hash = __int_hash;
    d . doimport = __int_import;
    d . doexport = __int_export;
    if (!__build_typeinfo("__builtin__.int", &d, kMCIntTypeInfo))
        return false;
    
    d . size = sizeof(uinteger_t);
    d . basetype = kMCNullTypeInfo;
    d . bridgetype = kMCNumberTypeInfo;
    p = kMCForeignPrimitiveTypeUInt32;
    d . layout = &p;
    d . layout_size = 1;
    d . initialize = nil;
    d . finalize = __numeric_finalize<uinteger_t>;
    d . defined = nil;
    d . move = __numeric_copy<uinteger_t>;
    d . copy = __numeric_copy<uinteger_t>;
    d . equal = __numeric_equal<uinteger_t>;
    d . hash = __int_hash;
    d . doimport = __uint_import;
    d . doexport = __uint_export;
    if (!__build_typeinfo("__builtin__.uint", &d, kMCUIntTypeInfo))
        return false;
    
    d . size = sizeof(float);
    d . basetype = kMCNullTypeInfo;
    d . bridgetype = kMCNumberTypeInfo;
    p = kMCForeignPrimitiveTypeFloat32;
    d . layout = &p;
    d . layout_size = 1;
    d . initialize = nil;
    d . finalize = __numeric_finalize<float>;
    d . defined = nil;
    d . move = __numeric_copy<float>;
    d . copy = __numeric_copy<float>;
    d . equal = __numeric_equal<float>;
    d . hash = __float_hash;
    d . doimport = __float_import;
    d . doexport = __float_export;
    if (!__build_typeinfo("__builtin__.float", &d, kMCFloatTypeInfo))
        return false;
    
    d . size = sizeof(double);
    d . basetype = kMCNullTypeInfo;
    d . bridgetype = kMCNumberTypeInfo;
    p = kMCForeignPrimitiveTypeFloat64;
    d . layout = &p;
    d . layout_size = 1;
    d . initialize = nil;
    d . finalize = __numeric_finalize<double>;
    d . defined = nil;
    d . move = __numeric_copy<double>;
    d . copy = __numeric_copy<double>;
    d . equal = __numeric_equal<double>;
    d . hash = __double_hash;
    d . doimport = __double_import;
    d . doexport = __double_export;
    if (!__build_typeinfo("__builtin__.double", &d, kMCDoubleTypeInfo))
        return false;
    
    d . size = sizeof(void *);
    d . basetype = kMCNullTypeInfo;
    d . bridgetype = kMCNullTypeInfo;
    p = kMCForeignPrimitiveTypePointer;
    d . layout = &p;
    d . layout_size = 1;
    d . initialize = __pointer_initialize;
    d . finalize = __pointer_finalize;
    d . defined = __pointer_defined;
    d . move = __pointer_copy;
    d . copy = __pointer_copy;
    d . equal = __pointer_equal;
    d . hash = __pointer_hash;
    d . doimport = nil;
    d . doexport = nil;
    if (!__build_typeinfo("__builtin__.pointer", &d, kMCPointerTypeInfo))
        return false;
    
    return true;
}

void __MCForeignValueFinalize(void)
{
    MCValueRelease(kMCBoolTypeInfo);
    MCValueRelease(kMCIntTypeInfo);
    MCValueRelease(kMCUIntTypeInfo);
    MCValueRelease(kMCFloatTypeInfo);
    MCValueRelease(kMCDoubleTypeInfo);
    MCValueRelease(kMCPointerTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////
