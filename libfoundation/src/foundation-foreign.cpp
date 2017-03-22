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
#include "foundation-hash.h"

#include <limits>
#include <type_traits>

////////////////////////////////////////////////////////////////////////////////

// The C bool type
MC_DLLEXPORT_DEF MCTypeInfoRef kMCBoolTypeInfo;

// The C float type
MC_DLLEXPORT_DEF MCTypeInfoRef kMCFloatTypeInfo;
// The C double type
MC_DLLEXPORT_DEF MCTypeInfoRef kMCDoubleTypeInfo;

// The C void * type (with enforced non-nullptr check).
MC_DLLEXPORT_DEF MCTypeInfoRef kMCPointerTypeInfo;

// The standard sized integer types
MC_DLLEXPORT_DEF MCTypeInfoRef kMCUInt8TypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCInt8TypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCUInt16TypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCInt16TypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCUInt32TypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCInt32TypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCUInt64TypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCInt64TypeInfo;

// The size_t and ssize_t types which are architecture / platform dependent
MC_DLLEXPORT_DEF MCTypeInfoRef kMCSizeTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCSSizeTypeInfo;

// The C unsigned long / long types which are architecture / platform dependent
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCULongTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCLongTypeInfo;

// The uinteger_t and integer_t types, which are engine dependent
MC_DLLEXPORT_DEF MCTypeInfoRef kMCUIntTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCIntTypeInfo;

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignBoolTypeInfo() { return kMCBoolTypeInfo; }

MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignFloatTypeInfo() { return kMCFloatTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignDoubleTypeInfo() { return kMCDoubleTypeInfo; }

MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignPointerTypeInfo() { return kMCPointerTypeInfo; }

MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignUInt8TypeInfo() { return kMCUInt8TypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignInt8TypeInfo() { return kMCInt8TypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignUInt16TypeInfo() { return kMCUInt16TypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignInt16TypeInfo() { return kMCInt16TypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignUInt32TypeInfo() { return kMCUInt32TypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignInt32TypeInfo() { return kMCInt32TypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignUInt64TypeInfo() { return kMCUInt64TypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignInt64TypeInfo() { return kMCInt64TypeInfo; }

MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignSizeTypeInfo() { return kMCSizeTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignSSizeTypeInfo() { return kMCSSizeTypeInfo; }

MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignCULongTypeInfo() { return kMCCULongTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignCLongTypeInfo() { return kMCCLongTypeInfo; }

MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignUIntTypeInfo() { return kMCUIntTypeInfo; }
MC_DLLEXPORT_DEF MCTypeInfoRef MCForeignIntTypeInfo() { return kMCIntTypeInfo; }

////////////////////////////////////////////////////////////////////////////////

struct bool_type_desc_t {
    using c_type = bool;
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeBool;
    static constexpr auto& base_type_info = kMCNullTypeInfo;
    static constexpr auto& type_info = kMCBoolTypeInfo;
    static constexpr auto is_optional = false;
    static constexpr auto is_bridgable = true;
    using bridge_type = MCBooleanRef;
    static constexpr auto& bridge_type_info = kMCBooleanTypeInfo;
    static constexpr auto& hash_func = MCHashBool;
};

template<typename CType>
struct numeric_type_desc_t {
    using c_type = CType;
    static constexpr auto& base_type_info = kMCNullTypeInfo;
    static constexpr auto is_optional = false;
    static constexpr auto is_bridgable = true;
    using bridge_type = MCNumberRef;
    static constexpr auto& bridge_type_info = kMCNumberTypeInfo;
};

template<typename CType>
struct integral_type_desc_t: public numeric_type_desc_t<CType> {
    static constexpr auto& hash_func = MCHashInt<CType>;
};

struct uint8_type_desc_t: public integral_type_desc_t<uint8_t> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeUInt8;
    static constexpr auto& type_info = kMCUInt8TypeInfo;
    static constexpr auto describe_format = "<foreign 8-bit unsigned integer %u>";
};
struct int8_type_desc_t: public integral_type_desc_t<int8_t> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeSInt8;
    static constexpr auto& type_info = kMCInt8TypeInfo;
    static constexpr auto describe_format = "<foreign 8-bit integer %d>";
};
struct uint16_type_desc_t: public integral_type_desc_t<uint16_t> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeUInt16;
    static constexpr auto& type_info = kMCUInt16TypeInfo;
    static constexpr auto describe_format = "<foreign 16-bit unsigned integer %u>";
};
struct int16_type_desc_t: public integral_type_desc_t<int16_t> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeSInt16;
    static constexpr auto& type_info = kMCInt16TypeInfo;
    static constexpr auto describe_format = "<foreign 16-bit integer %d>";
};
struct uint32_type_desc_t: public integral_type_desc_t<uint32_t> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeUInt32;
    static constexpr auto& type_info = kMCUInt32TypeInfo;
    static constexpr auto describe_format = "<foreign 32-bit unsigned integer %u>";
};
struct int32_type_desc_t: public integral_type_desc_t<int32_t> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeSInt32;
    static constexpr auto& type_info = kMCInt32TypeInfo;
    static constexpr auto describe_format = "<foreign 32-bit integer %d>";
};
struct uint64_type_desc_t: public integral_type_desc_t<uint64_t> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeUInt64;
    static constexpr auto& type_info = kMCUInt64TypeInfo;
    static constexpr auto describe_format = "<foreign 64-bit unsigned integer %llu>";
};
struct int64_type_desc_t: public integral_type_desc_t<int64_t> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeSInt64;
    static constexpr auto& type_info = kMCInt64TypeInfo;
    static constexpr auto describe_format = "<foreign 64-bit integer %lld>";
};

struct culong_type_desc_t: public integral_type_desc_t<unsigned long> {
    static_assert(ULONG_MAX == UINT32_MAX || ULONG_MAX == UINT64_MAX,
                  "Unsupported size for long");
    static constexpr auto primitive_type = 
        ((ULONG_MAX == UINT64_MAX) ?
         kMCForeignPrimitiveTypeUInt64 :
         kMCForeignPrimitiveTypeUInt32);
    static constexpr auto& type_info = kMCCULongTypeInfo;
    static constexpr auto describe_format = "<foreign unsigned long integer %lu>";
};
struct clong_type_desc_t: public integral_type_desc_t<long> {
    static_assert(LONG_MAX == INT32_MAX || LONG_MAX == INT64_MAX,
                  "Unsupported size for long");
    static constexpr auto primitive_type = 
        ((LONG_MAX == INT64_MAX) ?
         kMCForeignPrimitiveTypeSInt64 :
         kMCForeignPrimitiveTypeSInt32);
    static constexpr auto& type_info = kMCCLongTypeInfo;
    static constexpr auto describe_format = "<foreign long integer %li>";
    static constexpr auto& hash_func = MCHashInteger;
};

struct uint_type_desc_t: public integral_type_desc_t<uinteger_t> {
    static_assert(UINTEGER_MAX == UINT32_MAX,
                  "Unsupported size for uinteger_t");
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeUInt32;
    static constexpr auto& type_info = kMCUIntTypeInfo;
    static constexpr auto describe_format = "<foreign unsigned integer %u>";
};
struct int_type_desc_t: public integral_type_desc_t<integer_t> {
    static_assert(INTEGER_MAX == INT32_MAX,
                  "Unsupported size for integer_t");
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeSInt32;
    static constexpr auto& type_info = kMCIntTypeInfo;
    static constexpr auto describe_format = "<foreign integer %i>";
    static constexpr auto& hash_func = MCHashInteger;
};
struct size_type_desc_t: public integral_type_desc_t<size_t>  {
    static_assert(SIZE_MAX == UINT64_MAX || SIZE_MAX == UINT32_MAX,
                  "Unsupported size for size_t");
    static constexpr auto primitive_type =
        ((SIZE_MAX == UINT64_MAX) ?
         kMCForeignPrimitiveTypeUInt64 :
         kMCForeignPrimitiveTypeUInt32);
    static constexpr auto& type_info = kMCSizeTypeInfo;
    static constexpr auto describe_format = "<foreign size %zu>";
    static constexpr auto& hash_func = MCHashUSize;
};
struct ssize_type_desc_t: public integral_type_desc_t<ssize_t>  {
    static_assert(SSIZE_MAX == INT64_MAX || SSIZE_MAX == INT32_MAX,
                  "Unsupported size for ssize_t");
    static constexpr auto primitive_type =
        ((SSIZE_MAX == INT64_MAX) ?
         kMCForeignPrimitiveTypeSInt64 :
         kMCForeignPrimitiveTypeSInt32);
    static constexpr auto& type_info = kMCSSizeTypeInfo;
    static constexpr auto describe_format = "<foreign signed size %zd>";
    static constexpr auto& hash_func = MCHashSize;
};

struct float_type_desc_t: public numeric_type_desc_t<float> {
    using c_type = float;
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeFloat32;
    static constexpr auto& type_info = kMCFloatTypeInfo;
    static constexpr auto describe_format = "<foreign float %g>";
    static constexpr auto& hash_func = MCHashDouble;
};
struct double_type_desc_t: public numeric_type_desc_t<double> {
    using c_type = double;
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeFloat64;
    static constexpr auto& type_info = kMCDoubleTypeInfo;
    static constexpr auto describe_format = "<foreign double %g>";
    static constexpr auto& hash_func = MCHashDouble;
};

struct pointer_type_desc_t {
    using c_type = void*;
    static constexpr auto primitive_type = kMCForeignPrimitiveTypePointer;
    static constexpr auto is_optional = true;
    static constexpr auto is_bridgable = false;
    static constexpr auto& type_info = kMCPointerTypeInfo;
    static constexpr auto describe_format = "<foreign pointer %p>";
    static constexpr auto& base_type_info = kMCNullTypeInfo;
    static constexpr auto& hash_func = MCHashPointer;
};

MCTypeInfoRef kMCForeignImportErrorTypeInfo;
MCTypeInfoRef kMCForeignExportErrorTypeInfo;

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

template <typename OverflowedType>
static bool
__throw_numeric_overflow(MCTypeInfoRef p_error)
{
    return MCErrorCreateAndThrow(p_error,
                                 "type", OverflowedType::type_info,
                                 "reason", MCSTR("numeric overflow"),
                                 nil);
}

/* ----------------------------------------------------------------
 * Generic implementations
 * ---------------------------------------------------------------- */

namespace /* anonymous */ {

/* We need to be able to partially specialize describe(), doimport()
 * and doexport() as they have very type specific requirements.
 * Unfortunately, you can't partially specialize a function template.
 * The correct thing to do in this case is to have a single function
 * template that hands off to a functor template -- because struct
 * templates _can_ be partially specialized.
 *
 * See also "Why not specialize function templates?"
 * http://gotw.ca/publications/mill17.htm
 */

template<typename TypeDesc>
struct Describe
{
    static bool describe(typename TypeDesc::c_type p_value, MCStringRef& r_description)
    {
        return MCStringFormat(r_description, TypeDesc::describe_format, p_value);
    }
};

template<typename TypeDesc, typename Enable = void>
struct DoExport
{
    static_assert(sizeof(typename TypeDesc::c_type) == 0, "Missing export specialization");
    static bool doexport(typename TypeDesc::bridge_type p_boxed_value, typename TypeDesc::c_type& r_value)
    {
        return false;
    }
};

template<typename TypeDesc, typename Enable = void>
struct DoImport
{
    static_assert(sizeof(typename TypeDesc::c_type) == 0, "Missing import specialization");
    static bool doimport(typename TypeDesc::c_type p_value, typename TypeDesc::bridge_type& r_boxed_value)
    {
        return false;
    }
};

template <typename TypeDesc>
bool initialize(void *contents)
{
    static_assert(TypeDesc::is_optional, "This type is not optional");
    *static_cast<typename TypeDesc::c_type *>(contents) = typename TypeDesc::c_type();
    return true;
}

template <typename TypeDesc>
void finalize(void *contents)
{
}

template <typename TypeDesc>
bool defined(void *contents)
{
    static_assert(TypeDesc::is_optional, "This type is not optional");
    return *static_cast<typename TypeDesc::c_type *>(contents) != typename TypeDesc::c_type();
}

template <typename TypeDesc>
bool equal(void *left, void *right, bool& r_equal)
{
    r_equal = *static_cast<typename TypeDesc::c_type *>(left) == *static_cast<typename TypeDesc::c_type *>(right);
    return true;
}

template <typename TypeDesc>
bool copy(void *from, void *to)
{
    *static_cast<typename TypeDesc::c_type *>(to) = *static_cast<typename TypeDesc::c_type *>(from);
    return true;
}

template <typename TypeDesc>
bool move(void *from, void *to)
{
    return copy<TypeDesc>(from, to);
}

template <typename TypeDesc>
bool hash(void *value, hash_t& r_hash)
{
    r_hash = TypeDesc::hash_func(*static_cast<typename TypeDesc::c_type *>(value));
    return true;
}

template <typename TypeDesc>
bool describe(void *contents, MCStringRef& r_string)
{
    return Describe<TypeDesc>::describe(*static_cast<typename TypeDesc::c_type *>(contents), r_string);
}

template <typename TypeDesc>
bool doexport(MCValueRef p_value, bool p_release, void *contents)
{
    static_assert(TypeDesc::is_bridgable, "This type is not bridgable");
    
    if (!DoExport<TypeDesc>::doexport(static_cast<typename TypeDesc::bridge_type>(p_value),
                                      *static_cast<typename TypeDesc::c_type *>(contents)))
    {
        return false;
    }
    
    if (p_release)
    {
        MCValueRelease(p_value);
    }
    
    return true;
}

template <typename TypeDesc>
bool doimport(void *contents, bool p_release, MCValueRef& r_value)
{
    static_assert(TypeDesc::is_bridgable, "This type is not bridgable");
    bool t_success = DoImport<TypeDesc>::doimport(*static_cast<typename TypeDesc::c_type *>(contents),
                                        reinterpret_cast<typename TypeDesc::bridge_type&>(r_value));
    return t_success;
}

/* ---------- bool specializations */


/* We need to specialize describe for bool because it returns fixed
 * strings instead of using a format string. */
template <>
struct Describe<bool_type_desc_t>
{
    static bool describe(bool p_value, MCStringRef& r_string)
    {
        return MCStringFormat(r_string, p_value ? "<foreign true>" : "<foreign false>");
    }
};

template <>
struct DoImport<bool_type_desc_t>
{
    static bool doimport(bool p_value, MCBooleanRef& r_boxed_value)
    {
        r_boxed_value = MCValueRetain(p_value ? kMCTrue : kMCFalse);
        return true;
    }
};

template <>
struct DoExport<bool_type_desc_t>
{
    static bool doexport(MCValueRef p_boxed_value, bool& r_value)
    {
        r_value = (p_boxed_value == kMCTrue);
        return true;
    }
};

/* ---------- pointer specializations */

/* ---------- floating-point numeric specializations */

template <typename RealType>
struct DoExport<
    RealType, typename std::enable_if<std::is_floating_point<typename RealType::c_type>::value>::type>
{
    static bool doexport(MCNumberRef p_boxed_value, typename RealType::c_type& r_value)
    {
        r_value = typename RealType::c_type(MCNumberFetchAsReal(p_boxed_value));
        return true;
    }
};

template <typename RealType>
struct DoImport<
    RealType, typename std::enable_if<std::is_floating_point<typename RealType::c_type>::value>::type>
{
    static bool doimport(typename RealType::c_type& p_value, MCNumberRef& r_boxed_value)
    {
        return MCNumberCreateWithReal(p_value, r_boxed_value);
    }
};

/* ---------- integer numeric specializations */

template <typename IntType>
struct DoExport<
    IntType, typename std::enable_if<std::is_integral<typename IntType::c_type>::value>::type>
{
    static bool doexport(MCNumberRef p_boxed_value, typename IntType::c_type& r_value)
    {
        // Fetch the number as a double
        double t_value = MCNumberFetchAsReal(p_boxed_value);

        // First check that the value is within the contiguous integer range
        // of doubles. If that succeeds, then check it fits within the target
        // integer type.
        if (t_value < double(-(1LL << std::numeric_limits<double>::digits)) ||
            t_value > double(1LL << std::numeric_limits<double>::digits) ||
            t_value < double(std::numeric_limits<typename IntType::c_type>::min()) ||
            t_value > double(std::numeric_limits<typename IntType::c_type>::max()))
        {
            return __throw_numeric_overflow<IntType>(kMCForeignExportErrorTypeInfo);
        }

        r_value = typename IntType::c_type(t_value);

        return true;
    }
};

template <typename IntType>
struct DoImport<
    IntType, typename std::enable_if<std::is_integral<typename IntType::c_type>::value &&
                                     std::is_signed<typename IntType::c_type>::value>::type>
{
    static bool doimport(typename IntType::c_type p_value, MCNumberRef& r_boxed_value)
    {
        if (p_value >= INTEGER_MIN && p_value <= INTEGER_MAX)
        {
            return MCNumberCreateWithInteger(integer_t(p_value), r_boxed_value);
        }
        else if (p_value >= -(1LL << std::numeric_limits<double>::digits) &&
                 p_value <= (1LL << std::numeric_limits<double>::digits))
        {
            return MCNumberCreateWithReal(double(p_value), r_boxed_value);
        }
        else
        {
            return __throw_numeric_overflow<IntType>(kMCForeignExportErrorTypeInfo);
        }
    }
};

template <typename UIntType>
struct DoImport<
    UIntType, typename std::enable_if<std::is_integral<typename UIntType::c_type>::value &&
                                      std::is_unsigned<typename UIntType::c_type>::value>::type>
{
    static bool doimport(typename UIntType::c_type p_value, MCNumberRef& r_boxed_value)
    {
        if (p_value <= UINTEGER_MAX)
        {
            return MCNumberCreateWithUnsignedInteger(uinteger_t(p_value), r_boxed_value);
        }
        else if (p_value <= (1ULL << std::numeric_limits<double>::digits))
        {
            return MCNumberCreateWithReal(double(p_value), r_boxed_value);
        }
        else
        {
            return __throw_numeric_overflow<UIntType>(kMCForeignExportErrorTypeInfo);
        }
    }
};

template <typename TypeDesc>
class DescriptorBuilder {
public:
    static bool create(const char *p_name)
    {
        MCTypeInfoRef& destination = TypeDesc::type_info;

        MCForeignPrimitiveType primitive =
            TypeDesc::primitive_type;

        /* TODO[C++17] Some of the fields in here are left empty, and
         * are filled in by calling setup_optional() and
         * setup_bridge().  We could use constexpr if instead (see
         * below). */
        MCForeignTypeDescriptor d = {
            sizeof(typename TypeDesc::c_type),
            TypeDesc::base_type_info,
            nullptr, /* bridgetype */
            &primitive, /* layout */
            1, /* layout_size */
            nullptr, /* initialize */
            finalize<TypeDesc>,
            nullptr, /* defined */
            move<TypeDesc>,
            copy<TypeDesc>,
            equal<TypeDesc>,
            hash<TypeDesc>,
            nullptr, /* doimport */
            nullptr, /* doexport */
            describe<TypeDesc>
        };

        setup_optional<TypeDesc>(d);
        setup_bridge<TypeDesc>(d);

        MCAutoStringRef t_name_string;
        if (!MCStringCreateWithCString(p_name, &t_name_string))
            return false;
        MCNewAutoNameRef t_name_name;
        if (!MCNameCreate(*t_name_string, &t_name_name))
            return false;

        if (!MCNamedForeignTypeInfoCreate(*t_name_name, &d, destination))
            return false;

        return true;
    }

private:

    /* TODO[C++17] We need to use the setup_optional() and
     * setup_bridge() methods as separate templates because it's
     * important not to try to instantiate the callback functions for
     * types for which they are invalid (e.g. doimport<void*>() will
     * fail to compile).  This can easily be replaced with constexpr
     * if. */

    /* Setup the initialize() and defined() methods depending on
     * whether the ValueType is optional or not. */
    template <typename OptionalType,
              typename std::enable_if<
                  OptionalType::is_optional, int>::type = 0>
    static void setup_optional(MCForeignTypeDescriptor& d)
    {
        d.initialize = initialize<OptionalType>;
        d.defined = defined<OptionalType>;
    }

    template <typename RequiredType,
              typename std::enable_if<
                  !RequiredType::is_optional, int>::type = 0>
    static void setup_optional(MCForeignTypeDescriptor& d)
    {
        d.initialize = nullptr;
        d.defined = nullptr;
    }

    /* Setup the doimport() and doexport() methods depending on
     * whether the ValueType is optional or not. */
    template <typename BridgableType,
              typename std::enable_if<
                  BridgableType::is_bridgable, int>::type = 0>
    static void setup_bridge(MCForeignTypeDescriptor& d)
    {
        d.bridgetype = BridgableType::bridge_type_info;
        d.doimport = doimport<BridgableType>;
        d.doexport = doexport<BridgableType>;
    }

    template <typename UnbridgeableType,
              typename std::enable_if<
                  !UnbridgeableType::is_bridgable, int>::type = 0>
    static void setup_bridge(MCForeignTypeDescriptor& d)
    {
        d.bridgetype = kMCNullTypeInfo;
        d.doimport = nullptr;
        d.doexport = nullptr;
    }
};

} /* anonymous namespace */

/* ---------------------------------------------------------------- */

bool __MCForeignValueInitialize(void)
{
    if (!(DescriptorBuilder<bool_type_desc_t>::create("__builtin__.bool") &&
          DescriptorBuilder<float_type_desc_t>::create("__builtin__.float") &&
          DescriptorBuilder<double_type_desc_t>::create("__builtin__.double") &&
          DescriptorBuilder<pointer_type_desc_t>::create("__builtin__.pointer") &&
          DescriptorBuilder<uint8_type_desc_t>::create("__builtin__.uint8") &&
          DescriptorBuilder<int8_type_desc_t>::create("__builtin__.int8") &&
          DescriptorBuilder<uint16_type_desc_t>::create("__builtin__.uint16") &&
          DescriptorBuilder<int16_type_desc_t>::create("__builtin__.int16") &&
          DescriptorBuilder<uint32_type_desc_t>::create("__builtin__.uint32") &&
          DescriptorBuilder<int32_type_desc_t>::create("__builtin__.int32") &&
          DescriptorBuilder<uint64_type_desc_t>::create("__builtin__.uint64") &&
          DescriptorBuilder<int64_type_desc_t>::create("__builtin__.int64") &&
          DescriptorBuilder<size_type_desc_t>::create("__builtin__.size") &&
          DescriptorBuilder<ssize_type_desc_t>::create("__builtin__ssize") &&
          DescriptorBuilder<culong_type_desc_t>::create("__builtin__.culong") &&
          DescriptorBuilder<clong_type_desc_t>::create("__builtin__.clong") &&
          DescriptorBuilder<uint_type_desc_t>::create("__builtin__.uint") &&
          DescriptorBuilder<int_type_desc_t>::create("__builtin__.int")))
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
    MCValueRelease(kMCFloatTypeInfo);
    MCValueRelease(kMCDoubleTypeInfo);
    MCValueRelease(kMCPointerTypeInfo);
    MCValueRelease(kMCUInt8TypeInfo);
    MCValueRelease(kMCInt8TypeInfo);
    MCValueRelease(kMCUInt16TypeInfo);
    MCValueRelease(kMCInt16TypeInfo);
    MCValueRelease(kMCUInt32TypeInfo);
    MCValueRelease(kMCInt32TypeInfo);
    MCValueRelease(kMCUInt64TypeInfo);
    MCValueRelease(kMCInt64TypeInfo);
	MCValueRelease(kMCSizeTypeInfo);
	MCValueRelease(kMCSSizeTypeInfo);
	MCValueRelease(kMCCULongTypeInfo);
	MCValueRelease(kMCCLongTypeInfo);
    MCValueRelease(kMCUIntTypeInfo);
    MCValueRelease(kMCIntTypeInfo);

	MCValueRelease (kMCForeignImportErrorTypeInfo);
	MCValueRelease (kMCForeignExportErrorTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////
