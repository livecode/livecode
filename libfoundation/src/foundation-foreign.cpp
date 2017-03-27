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
#include <type_traits>

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCTypeInfoRef kMCBoolTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCUIntTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCIntTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCFloatTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCDoubleTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCPointerTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCSizeTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCSSizeTypeInfo;

template <typename T> struct ForeignTypeTraits {
    /* Fails compile if instantiated */
    static_assert(sizeof(T) == 0, "Missing foreign type trait specialization");
};
template <> struct ForeignTypeTraits<bool> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeBool;
    static constexpr auto is_optional = false;
    static constexpr auto is_bridgable = true;
    static constexpr auto& type_info = kMCBoolTypeInfo;
    static constexpr auto& base_type_info = kMCNullTypeInfo;
    using bridge_type = MCBooleanRef;
    static constexpr auto& bridge_type_info = kMCBooleanTypeInfo;
};
template <> struct ForeignTypeTraits<uinteger_t> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeUInt32;
    static constexpr auto is_optional = false;
    static constexpr auto is_bridgable = true;
    static constexpr auto& type_info = kMCUIntTypeInfo;
    static constexpr auto describe_format = "<foreign unsigned integer %u>";
    static constexpr auto& base_type_info = kMCNullTypeInfo;
    using bridge_type = MCNumberRef;
    static constexpr auto& bridge_type_info = kMCNumberTypeInfo;
    static constexpr auto& hash_func = MCHashUInteger;
};
template <> struct ForeignTypeTraits<integer_t> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeSInt32;
    static constexpr auto is_optional = false;
    static constexpr auto is_bridgable = true;
    static constexpr auto& type_info = kMCIntTypeInfo;
    static constexpr auto describe_format = "<foreign integer %i>";
    static constexpr auto& base_type_info = kMCNullTypeInfo;
    using bridge_type = MCNumberRef;
    static constexpr auto& bridge_type_info = kMCNumberTypeInfo;
    static constexpr auto& hash_func = MCHashInteger;
};
template <> struct ForeignTypeTraits<float> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeFloat32;
    static constexpr auto is_optional = false;
    static constexpr auto is_bridgable = true;
    static constexpr auto& type_info = kMCFloatTypeInfo;
    static constexpr auto& base_type_info = kMCNullTypeInfo;
    using bridge_type = MCNumberRef;
    static constexpr auto& bridge_type_info = kMCNumberTypeInfo;
    static constexpr auto& hash_func = MCHashDouble;
};
template <> struct ForeignTypeTraits<double> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypeFloat64;
    static constexpr auto is_optional = false;
    static constexpr auto is_bridgable = true;
    static constexpr auto& type_info = kMCDoubleTypeInfo;
    static constexpr auto describe_format = "<foreign double %g>";
    static constexpr auto& base_type_info = kMCNullTypeInfo;
    using bridge_type = MCNumberRef;
    static constexpr auto& bridge_type_info = kMCNumberTypeInfo;
    static constexpr auto& hash_func = MCHashDouble;
};
template <> struct ForeignTypeTraits<void *> {
    static constexpr auto primitive_type = kMCForeignPrimitiveTypePointer;
    static constexpr auto is_optional = true;
    static constexpr auto is_bridgable = false;
    static constexpr auto& type_info = kMCPointerTypeInfo;
    static constexpr auto describe_format = "<foreign pointer %p>";
    static constexpr auto& base_type_info = kMCNullTypeInfo;
    static constexpr auto& hash_func = MCHashPointer;
};
template <> struct ForeignTypeTraits<size_t> {
    static_assert(SIZE_MAX == UINT64_MAX || SIZE_MAX == UINT32_MAX,
                  "Unsupported size for size_t");
    static constexpr auto primitive_type =
        ((SIZE_MAX == UINT64_MAX) ?
         kMCForeignPrimitiveTypeUInt64 :
         kMCForeignPrimitiveTypeUInt32);
    static constexpr auto is_optional = false;
    static constexpr auto is_bridgable = true;
    static constexpr auto& type_info = kMCSizeTypeInfo;
    static constexpr auto describe_format = "<foreign size %zu>";
    static constexpr auto& base_type_info = kMCNullTypeInfo;
    using bridge_type = MCNumberRef;
    static constexpr auto& bridge_type_info = kMCNumberTypeInfo;
    static constexpr auto& hash_func = MCHashUSize;
};
template <> struct ForeignTypeTraits<ssize_t> {
    static_assert(SSIZE_MAX == INT64_MAX || SSIZE_MAX == INT32_MAX,
                  "Unsupported size for ssize_t");
    static constexpr auto primitive_type =
        ((SSIZE_MAX == INT64_MAX) ?
         kMCForeignPrimitiveTypeSInt64 :
         kMCForeignPrimitiveTypeSInt32);
    static constexpr auto is_optional = false;
    static constexpr auto is_bridgable = true;
    static constexpr auto& type_info = kMCSSizeTypeInfo;
    static constexpr auto describe_format = "<foreign signed size %zd>";
    static constexpr auto& base_type_info = kMCNullTypeInfo;
    using bridge_type = MCNumberRef;
    static constexpr auto& bridge_type_info = kMCNumberTypeInfo;
    static constexpr auto& hash_func = MCHashSize;
};

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
    if (!__MCValueCreateExtended(kMCValueTypeCodeForeignValue,
                                 t_resolved_typeinfo -> foreign . descriptor . size,
                                 t_value))
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
    if (!__MCValueCreateExtended(kMCValueTypeCodeForeignValue,
                                 t_resolved_typeinfo -> foreign . descriptor . size,
                                 t_value))
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
    if (!__MCValueCreateExtended(kMCValueTypeCodeForeignValue,
                                 t_resolved_typeinfo -> foreign . descriptor . size,
                                 t_value))
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
                                 "type", ForeignTypeTraits<OverflowedType>::type_info,
                                 "reason", MCSTR("numeric overflow"),
                                 nil);
}

/* ----------------------------------------------------------------
 * Generic implementations
 * ---------------------------------------------------------------- */

namespace /* anonymous */ {

template <typename ValueType>
bool initialize(void *contents)
{
    *static_cast<ValueType*>(contents) = ValueType();
    return true;
}

template <typename ValueType>
void finalize (void *contents)
{
}

template <typename ValueType>
bool defined (void *contents)
{
    static_assert(sizeof(ValueType) == 0, "This type is not optional");
    return true;
}

template <>
bool defined<void*> (void *contents)
{
    return nullptr != *static_cast<void**>(contents);
}

template <typename ValueType>
bool equal(void *left,
                   void *right,
                   bool& r_equal)
{
    r_equal = (*static_cast<ValueType*>(left) == *static_cast<ValueType*>(right));
    return true;
}

template <typename ValueType>
bool copy(void *from, void *to)
{
    *static_cast<ValueType*>(to) = *static_cast<ValueType*>(from);
    return true;
}

template <typename ValueType>
bool move(void *from, void *to)
{
    return copy<ValueType>(from, to);
}

/* We need to be able to partially specialize hash() to cope
 * with types that have a "trivial" hand-off to a standard
 * libfoundation hash function.  Unfortunately, you can't partially
 * specialize a function template.  The correct thing to do in this
 * case is to have a single hash<>() template that hands off
 * to a functor template -- because struct templates _can_ be
 * partially specialized.
 *
 * See also "Why not specialize function templates?"
 * http://gotw.ca/publications/mill17.htm
 */

/* The default Hash functor template calls the hash_func from
 * the foreign type trait. */
template<typename ValueType, typename Enable = void>
struct Hash
{
    static bool hash(ValueType value, hash_t& r_hash)
    {
        return ForeignTypeTraits<ValueType>::hash_func(value);
    }
};

template <typename ValueType>
bool hash(void *value,
                  hash_t & r_hash)
{
    r_hash = Hash<ValueType>::hash(*static_cast<ValueType*>(value), r_hash);
    return true;
}

/* The default Describe functor template uses a format
 * string defined in the foreign type trait. */
template <typename ValueType, typename Enable = void>
struct Describe
{
    static bool describe(ValueType value, MCStringRef& r_string)
    {
        return MCStringFormat(r_string,
                              ForeignTypeTraits<ValueType>::describe_format,
                              value);
    }
};

template <typename ValueType>
bool describe(void *contents,
                      MCStringRef& r_string)
{
    return Describe<ValueType>::describe(*static_cast<ValueType*>(contents), r_string);
}

/* There's not really any way to generically perform import or export,
 * because it's so type-dependent.  The default implementation of
 * Import/Export will fail to compile. */
template <typename ValueType, typename Enable = void>
struct DoExport
{
    static_assert(sizeof(ValueType) == 0, "Missing export specialization");
};
template <typename ValueType, typename Enable = void>
struct DoImport
{
    static_assert(sizeof(ValueType) == 0, "Missing import specialization");
};

template <typename ValueType>
bool doexport(MCValueRef p_value,
              bool p_release,
              void *contents)
{
    using BridgeType = typename ForeignTypeTraits<ValueType>::bridge_type;
    MCAssert(contents != nullptr);
    return DoExport<ValueType>::doexport(static_cast<BridgeType>(p_value),
                                         p_release,
                                         *static_cast<ValueType*>(contents));
}

template <typename ValueType>
bool doimport(void *p_contents,
              bool p_release,
              MCValueRef& r_value)
{
    using BridgeType = typename ForeignTypeTraits<ValueType>::bridge_type;
    MCAssert(p_contents != nullptr);
    return DoImport<ValueType>::doimport(*static_cast<ValueType*>(p_contents),
                                         p_release,
                                         reinterpret_cast<BridgeType&>(r_value));
}

/* ---------- bool specializations */

template <>
struct Hash<bool>
{
    static bool hash(bool value, hash_t& r_hash)
    {
        return hash_t(value);
    }
};

/* We need to specialize describe for bool because it returns fixed
 * strings instead of using a format string. */
template <>
struct Describe<bool>
{
    static bool describe(bool value, MCStringRef& r_string)
    {
        return MCStringCopy(MCSTR(value ? "<foreign true>" : "<foreign false>"),
                            r_string);
    }
};

template <>
struct DoImport<bool>
{
    static bool doimport(bool p_contents, bool p_release, MCBooleanRef& r_value)
    {
        r_value = MCValueRetain(p_contents ? kMCTrue : kMCFalse);
        return true;
    }
};

template <>
struct DoExport<bool>
{
    static bool doexport(MCBooleanRef p_value, bool p_release, bool& r_contents)
    {
        r_contents = (p_value == kMCTrue);
        return true;
    }
};

/* ---------- pointer specializations */

/* ---------- floating-point numeric specializations */

/* It's necessary to specialize describe for float because there's no
 * format code for 32-bit floating point values, so an additional cast
 * is required. */
template <>
struct Describe<float>
{
    static bool describe(float value, MCStringRef& r_string)
    {
        return MCStringFormat(r_string, "<foreign float %g>", double{value});
    }
};

template <typename RealType>
struct DoExport<
    RealType, typename std::enable_if<std::is_floating_point<RealType>::value>::type>
{
    static bool doexport(MCNumberRef p_value, bool p_release, RealType& r_contents)
    {
        r_contents = RealType(MCNumberFetchAsReal(p_value));
        if (p_release)
            MCValueRelease (p_value);
        return true;
    }
};

template <typename RealType>
struct DoImport<
    RealType, typename std::enable_if<std::is_floating_point<RealType>::value>::type>
{
    static bool doimport(RealType p_contents, bool p_release, MCNumberRef& r_value)
    {
        return MCNumberCreateWithReal(p_contents, r_value);
    }
};

/* ---------- integer numeric specializations */

template <typename IntType>
struct DoExport<
    IntType, typename std::enable_if<std::is_integral<IntType>::value>::type>
{
    static bool doexport(MCNumberRef p_value, bool p_release, IntType& r_contents)
    {
        // Fetch the number as a double
        double t_value = MCNumberFetchAsReal(p_value);

        // First check that the value is within the contiguous integer range
        // of doubles. If that succeeds, then check it fits within the target
        // integer type.
        if (t_value < double(-(1LL << std::numeric_limits<double>::digits)) ||
            t_value > double(1LL << std::numeric_limits<double>::digits) ||
            t_value < double(std::numeric_limits<IntType>::min()) ||
            t_value > double(std::numeric_limits<IntType>::max()))
        {
            return __throw_numeric_overflow<IntType>(kMCForeignExportErrorTypeInfo);
        }

        r_contents = IntType(t_value);

        if (p_release)
            MCValueRelease(p_value);

        return true;
    }
};

template <typename IntType>
struct DoImport<
    IntType, typename std::enable_if<std::is_integral<IntType>::value &&
                                     std::is_signed<IntType>::value>::type>
{
    static bool doimport(IntType p_contents, bool p_release, MCNumberRef& r_value)
    {
        if (p_contents >= INTEGER_MIN && p_contents <= INTEGER_MAX)
        {
            return MCNumberCreateWithInteger(integer_t(p_contents), r_value);
        }
        else if (p_contents >= -(1LL << std::numeric_limits<double>::digits) &&
                 p_contents <= (1LL << std::numeric_limits<double>::digits))
        {
            return MCNumberCreateWithReal(double(p_contents), r_value);
        }
        else
        {
            return __throw_numeric_overflow<IntType>(kMCForeignExportErrorTypeInfo);
        }
    }
};

template <typename UIntType>
struct DoImport<
    UIntType, typename std::enable_if<std::is_integral<UIntType>::value &&
                                      std::is_unsigned<UIntType>::value>::type>
{
    static bool doimport(UIntType p_contents, bool p_release, MCNumberRef& r_value)
    {
        if (p_contents <= UINTEGER_MAX)
        {
            return MCNumberCreateWithUnsignedInteger(uinteger_t(p_contents), r_value);
        }
        else if (p_contents <= (1ULL << std::numeric_limits<double>::digits))
        {
            return MCNumberCreateWithReal(double(p_contents), r_value);
        }
        else
        {
            return __throw_numeric_overflow<UIntType>(kMCForeignExportErrorTypeInfo);
        }
    }
};



template <typename ValueType>
class DescriptorBuilder {
public:
    static bool create(const char *p_name)
    {
        MCTypeInfoRef& destination = ForeignTypeTraits<ValueType>::type_info;

        MCForeignPrimitiveType primitive =
            ForeignTypeTraits<ValueType>::primitive_type;

        /* TODO[C++17] Some of the fields in here are left empty, and
         * are filled in by calling setup_optional() and
         * setup_bridge().  We could use constexpr if instead (see
         * below). */
        MCForeignTypeDescriptor d = {
            sizeof(ValueType),
            ForeignTypeTraits<ValueType>::base_type_info,
            nullptr, /* bridgetype */
            &primitive, /* layout */
            1, /* layout_size */
            nullptr, /* initialize */
            finalize<ValueType>,
            nullptr, /* defined */
            move<ValueType>,
            copy<ValueType>,
            equal<ValueType>,
            hash<ValueType>,
            nullptr, /* doimport */
            nullptr, /* doexport */
            describe<ValueType>
        };

        setup_optional<ValueType>(d);
        setup_bridge<ValueType>(d);

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
                  ForeignTypeTraits<OptionalType>::is_optional, int>::type = 0>
    static void setup_optional(MCForeignTypeDescriptor& d)
    {
        d.initialize = initialize<OptionalType>;
        d.defined = defined<OptionalType>;
    }

    template <typename RequiredType,
              typename std::enable_if<
                  !ForeignTypeTraits<RequiredType>::is_optional, int>::type = 0>
    static void setup_optional(MCForeignTypeDescriptor& d)
    {
        d.initialize = nullptr;
        d.defined = nullptr;
    }

    /* Setup the doimport() and doexport() methods depending on
     * whether the ValueType is optional or not. */
    template <typename BridgableType,
              typename std::enable_if<
                  ForeignTypeTraits<BridgableType>::is_bridgable, int>::type = 0>
    static void setup_bridge(MCForeignTypeDescriptor& d)
    {
        d.bridgetype = ForeignTypeTraits<BridgableType>::bridge_type_info;
        d.doimport = doimport<BridgableType>;
        d.doexport = doexport<BridgableType>;
    }

    template <typename UnbridgeableType,
              typename std::enable_if<
                  !ForeignTypeTraits<UnbridgeableType>::is_bridgable, int>::type = 0>
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
    if (!(DescriptorBuilder<bool>::create("__builtin__.bool") &&
          DescriptorBuilder<integer_t>::create("__builtin__.int") &&
          DescriptorBuilder<uinteger_t>::create("__builtin__.uint") &&
          DescriptorBuilder<float>::create("__builtin__.float") &&
          DescriptorBuilder<double>::create("__builtin__.double") &&
          DescriptorBuilder<void*>::create("__builtin__.pointer") &&
          DescriptorBuilder<size_t>::create("__builtin__.size") &&
          DescriptorBuilder<ssize_t>::create("__builtin__ssize")))
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
