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

#include <limits>

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCTypeInfoRef kMCBoolTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCUIntTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCIntTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCFloatTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCDoubleTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCPointerTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCSizeTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCSSizeTypeInfo;

MCTypeInfoRef kMCForeignImportErrorTypeInfo;
MCTypeInfoRef kMCForeignExportErrorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignBoolTypeInfo() { return kMCBoolTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignUIntTypeInfo() { return kMCUIntTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignIntTypeInfo() { return kMCIntTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignFloatTypeInfo() { return kMCFloatTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignDoubleTypeInfo() { return kMCDoubleTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignPointerTypeInfo() { return kMCPointerTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignSizeTypeInfo() { return kMCSizeTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignSSizeTypeInfo() { return kMCSSizeTypeInfo; }

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCForeignValueCreate(MCTypeInfoRef p_typeinfo, void *p_contents, MCForeignValueRef& r_value)
{
    MCAssert(MCTypeInfoIsNamed(p_typeinfo));
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(p_typeinfo);
    
    __MCForeignValue *t_value;
    if (!__MCValueCreate(kMCValueTypeCodeForeignValue, sizeof(__MCForeignValue) + t_resolved_typeinfo -> foreign . descriptor . size, (__MCValue*&)t_value))
        return false;
    
    if (!t_resolved_typeinfo -> foreign . descriptor . copy(p_contents, t_value + 1))
    {
        MCMemoryDelete(t_value);
        return false;
    }
    
    t_value -> typeinfo = MCValueRetain(p_typeinfo);
    
    r_value = t_value;
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCForeignValueCreateAndRelease(MCTypeInfoRef p_typeinfo, void *p_contents, MCForeignValueRef& r_value)
{
    MCAssert(MCTypeInfoIsNamed(p_typeinfo));
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(p_typeinfo);
    
    __MCForeignValue *t_value;
    if (!__MCValueCreate(kMCValueTypeCodeForeignValue, sizeof(__MCForeignValue) + t_resolved_typeinfo -> foreign . descriptor . size, (__MCValue*&)t_value))
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

MC_DLLEXPORT_DEF
bool MCForeignValueExport(MCTypeInfoRef p_typeinfo, MCValueRef p_value, MCForeignValueRef& r_value)
{
    MCAssert(MCTypeInfoIsNamed(p_typeinfo));
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(p_typeinfo);
    
    __MCForeignValue *t_value;
    if (!__MCValueCreate(kMCValueTypeCodeForeignValue, sizeof(__MCForeignValue) + t_resolved_typeinfo -> foreign . descriptor . size, (__MCValue*&)t_value))
        return false;

    if (t_resolved_typeinfo->foreign.descriptor.doexport == nil ||
        !t_resolved_typeinfo->foreign.descriptor.doexport(p_value, false, t_value + 1))
    {
        MCMemoryDelete(t_value);
        return false;
    }
    
    t_value -> typeinfo = MCValueRetain(p_typeinfo);
    
    r_value = t_value;
    
    return true;
}

MC_DLLEXPORT_DEF
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
	
	MCValueRelease(self -> typeinfo);
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
	MCTypeInfoRef t_resolved_typeinfo;
	t_resolved_typeinfo = __MCTypeInfoResolve(self->typeinfo);

	bool (*t_describe_func)(void *, MCStringRef &);
	t_describe_func = t_resolved_typeinfo->foreign.descriptor.describe;

	if (NULL != t_describe_func)
		return t_describe_func (MCForeignValueGetContentsPtr (self),
		                        r_description);
	else
		return MCStringFormat(r_description, "<foreign: %p>", self);
}

////////////////////////////////////////////////////////////////////////////////

static bool
__throw_numeric_overflow(MCTypeInfoRef p_error, MCTypeInfoRef p_type)
{
    return MCErrorCreateAndThrow(p_error,
                                 "type", p_type,
                                 "reason", MCSTR("numeric overflow"),
                                 nil);
}

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

static bool
__uint_hash(void *value,
            hash_t & r_hash)
{
	r_hash = MCHashUInteger(*(uinteger_t *) value);
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

static bool
__size_hash (void *value,
             hash_t & r_hash)
{
	r_hash = MCHashUSize(*reinterpret_cast<size_t *>(value));
	return true;
}

static bool
__ssize_hash (void *value,
             hash_t & r_hash)
{
	r_hash = MCHashSize(*reinterpret_cast<ssize_t *>(value));
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

static bool
__size_import (void *contents,
               bool release,
               MCValueRef & r_value)
{
	size_t t_value = *static_cast<size_t *>(contents);

    if (t_value <= UINTEGER_MAX)
    {
        return MCNumberCreateWithUnsignedInteger(uinteger_t(t_value),
                                                 reinterpret_cast<MCNumberRef&>(r_value));
    }
#ifdef __64_BIT__
    else if (t_value <= (1ULL << std::numeric_limits<double>::digits))
    {
        return MCNumberCreateWithReal(double(t_value),
                                      reinterpret_cast<MCNumberRef&>(r_value));
    }
#endif
    else
    {
        return __throw_numeric_overflow(kMCForeignImportErrorTypeInfo,
                                        kMCSizeTypeInfo);
	}
}

static bool
__ssize_import (void *contents,
               bool release,
               MCValueRef & r_value)
{
	ssize_t t_value = *static_cast<ssize_t *>(contents);
    
    if (t_value >= INTEGER_MIN && t_value <= INTEGER_MAX)
    {
        return MCNumberCreateWithInteger(integer_t(t_value),
                                         reinterpret_cast<MCNumberRef&>(r_value));
    }
#ifdef __64_BIT__
    else if (t_value >= -(1LL << std::numeric_limits<double>::digits) &&
             t_value <= (1LL << std::numeric_limits<double>::digits))
    {
        return MCNumberCreateWithReal(double(t_value),
                                      reinterpret_cast<MCNumberRef&>(r_value));
    }
#endif
    else
    {
        return __throw_numeric_overflow(kMCForeignImportErrorTypeInfo,
                                        kMCSSizeTypeInfo);
	}
}

static bool __float_import(void *contents, bool release, MCValueRef& r_value)
{
    return MCNumberCreateWithReal(*(float *)contents, (MCNumberRef&)r_value);
}

static bool __double_import(void *contents, bool release, MCValueRef& r_value)
{
    return MCNumberCreateWithReal(*(double *)contents, (MCNumberRef&)r_value);
}

template <typename T>
static bool
__any_int_export(MCValueRef value,
                 bool release,
                 void *contents,
                 MCTypeInfoRef typeinfo)
{
    // Fetch the number as a double
    double t_value =
            MCNumberFetchAsReal(static_cast<MCNumberRef>(value));
 
    // First check that the value is within the contiguous integer range
    // of doubles. If that succeeds, then check it fits within the target
    // integer type.
    if (t_value < double(-(1LL << std::numeric_limits<double>::digits)) ||
        t_value > double(1LL << std::numeric_limits<double>::digits) ||
        t_value < double(std::numeric_limits<T>::min()) ||
        t_value > double(std::numeric_limits<T>::max()))
    {
        return __throw_numeric_overflow(kMCForeignExportErrorTypeInfo,
                                        typeinfo);
    }

    *(T *)contents = T(t_value);

    if (release)
        MCValueRelease(value);
    
    return true;
}

static bool
__int_export(MCValueRef value, bool release, void *contents)
{
    return __any_int_export<integer_t>(value, release, contents, kMCIntTypeInfo);
}

static bool
__uint_export(MCValueRef value, bool release, void *contents)
{
	return __any_int_export<uinteger_t>(value, release, contents, kMCUIntTypeInfo);
}

static bool
__size_export(MCValueRef value, bool release, void *contents)
{
    return __any_int_export<size_t>(value, release, contents, kMCSizeTypeInfo);
}

static bool
__ssize_export(MCValueRef value, bool release, void *contents)
{
    return __any_int_export<ssize_t>(value, release, contents, kMCSSizeTypeInfo);
}

static bool __float_export(MCValueRef value, bool release, void *contents)
{
    *(float *)contents = float(MCNumberFetchAsReal((MCNumberRef)value));
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

static bool
__bool_describe (void *contents,
                 MCStringRef & r_string)
{
	return MCStringCopy (MCSTR(*((bool *) contents) ? "<foreign true>" : "<foreign false>"), r_string);
}

static bool
__int_describe (void *contents,
                MCStringRef & r_string)
{
	return MCStringFormat (r_string, "<foreign integer %i>",
	                       *((integer_t *) contents));
}

static bool
__uint_describe (void *contents,
                 MCStringRef & r_string)
{
	return MCStringFormat (r_string, "<foreign unsigned integer %u>",
	                       *((uinteger_t *) contents));
}

static bool
__float_describe (void *contents,
                  MCStringRef & r_string)
{
	return MCStringFormat (r_string, "<foreign float %g>",
	                       (double) *((float *) contents));
}

static bool
__double_describe (void *contents,
                   MCStringRef & r_string)
{
	return MCStringFormat (r_string, "<foreign double %g>",
	                       *((double *) contents));
}

static bool
__pointer_describe (void *contents,
                    MCStringRef & r_string)
{
	return MCStringFormat (r_string, "<foreign pointer %p>",
	                       *((void **) contents));
}

static bool
__size_describe (void *contents,
                 MCStringRef & r_string)
{
	return MCStringFormat (r_string, "<foreign size %zu>",
	                       *((size_t *) contents));
}

static bool
__ssize_describe (void *contents,
                 MCStringRef & r_string)
{
	return MCStringFormat (r_string, "<foreign ssize %zd>",
	                       *((ssize_t *) contents));
}

static bool __build_typeinfo(const char *p_name, MCForeignTypeDescriptor *p_desc, MCTypeInfoRef& r_typeinfo)
{
    MCAutoStringRef t_name_string;
    if (!MCStringCreateWithCString(p_name, &t_name_string))
        return false;
    MCNewAutoNameRef t_name_name;
    if (!MCNameCreate(*t_name_string, &t_name_name))
        return false;
	
	if (!MCNamedForeignTypeInfoCreate(*t_name_name, p_desc, r_typeinfo))
		return false;
	
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
    d . describe = __bool_describe;
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
    d . describe = __int_describe;
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
    d . hash = __uint_hash;
    d . doimport = __uint_import;
    d . doexport = __uint_export;
    d . describe = __uint_describe;
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
    d . describe = __float_describe;
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
    d . describe = __double_describe;
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
    d . describe = __pointer_describe;
    if (!__build_typeinfo("__builtin__.pointer", &d, kMCPointerTypeInfo))
        return false;

	d . size = sizeof(size_t);
	d . basetype = kMCNullTypeInfo;
	d . bridgetype = kMCNumberTypeInfo;
#if SIZE_MAX == UINT64_MAX
	p = kMCForeignPrimitiveTypeUInt64;
#elif SIZE_MAX == UINT32_MAX
	p = kMCForeignPrimitiveTypeUInt32;
#else
#	error "Unsupported storage layout for size_t"
#endif
	d . layout = &p;
	d . layout_size = 1;
	d . initialize = nil;
	d . finalize = __numeric_finalize<size_t>;
	d . defined = nil;
	d . move = __numeric_copy<size_t>;
	d . copy = __numeric_copy<size_t>;
	d . equal = __numeric_equal<size_t>;
	d . hash = __size_hash;
	d . doimport = __size_import;
	d . doexport = __size_export;
	d . describe = __size_describe;
	if (!__build_typeinfo("__builtin__.size", &d, kMCSizeTypeInfo))
		return false;
    
	d . size = sizeof(ssize_t);
	d . basetype = kMCNullTypeInfo;
	d . bridgetype = kMCNumberTypeInfo;
#if SSIZE_MAX == INT64_MAX
	p = kMCForeignPrimitiveTypeUInt64;
#elif SSIZE_MAX == INT32_MAX
	p = kMCForeignPrimitiveTypeUInt32;
#else
#	error "Unsupported storage layout for ssize_t"
#endif
	d . layout = &p;
	d . layout_size = 1;
	d . initialize = nil;
	d . finalize = __numeric_finalize<ssize_t>;
	d . defined = nil;
	d . move = __numeric_copy<ssize_t>;
	d . copy = __numeric_copy<ssize_t>;
	d . equal = __numeric_equal<ssize_t>;
	d . hash = __ssize_hash;
	d . doimport = __ssize_import;
	d . doexport = __ssize_export;
	d . describe = __ssize_describe;
	if (!__build_typeinfo("__builtin__.ssize", &d, kMCSSizeTypeInfo))
		return false;
    
	/* ---------- */

	if (!MCNamedErrorTypeInfoCreate (MCNAME("livecode.lang.ForeignTypeImportError"), MCNAME("runtime"), MCSTR("error importing foreign '%{type}' value: %{reason}"), kMCForeignImportErrorTypeInfo))
		return false;
	if (!MCNamedErrorTypeInfoCreate (MCNAME("livecode.lang.ForeignTypeExportError"), MCNAME("runtime"), MCSTR("error exporting foreign '%{type}' value: %{reason}"), kMCForeignExportErrorTypeInfo))
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
	MCValueRelease(kMCSizeTypeInfo);
	MCValueRelease(kMCSSizeTypeInfo);

	MCValueRelease (kMCForeignImportErrorTypeInfo);
	MCValueRelease (kMCForeignExportErrorTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////
