/* Copyright (C) 2017 LiveCode Ltd.
 
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

#include "gtest/gtest.h"

#include "foundation.h"
#include "foundation-auto.h"

#include <limits>
#include <type_traits>

typedef MCAutoValueRefBase<MCForeignValueRef> MCAutoForeignValueRef;

template<typename T>
void check_creation(MCTypeInfoRef p_type_info, T p_value)
{
    MCAutoForeignValueRef t_boxed_value;
    EXPECT_TRUE(MCForeignValueCreate(p_type_info, &p_value, &t_boxed_value));
    EXPECT_EQ(*static_cast<T *>(MCForeignValueGetContentsPtr(*t_boxed_value)), p_value);
}

template<typename T>
void check_equality(MCTypeInfoRef p_type_info, T p_left, T p_right, bool p_expected)
{
    MCAutoForeignValueRef t_left_box, t_right_box;
    EXPECT_TRUE(MCForeignValueCreate(p_type_info, &p_left, &t_left_box));
    EXPECT_TRUE(MCForeignValueCreate(p_type_info, &p_right, &t_right_box));
    EXPECT_EQ(MCValueIsEqualTo(*t_left_box, *t_right_box), p_expected);
}

template<typename T, typename U = T>
void check_hash(MCTypeInfoRef p_type_info, T p_value, hash_t (*p_hash_func)(U p_value))
{
    MCAutoForeignValueRef t_boxed_value;
    EXPECT_TRUE(MCForeignValueCreate(p_type_info, &p_value, &t_boxed_value));
    EXPECT_EQ(MCValueHash(*t_boxed_value), p_hash_func(p_value));
}

template<typename T>
void check_describe(MCTypeInfoRef p_type_info, T p_value, const char *p_format_str)
{
    char t_format[256];
    sprintf(t_format, p_format_str, p_value);
    MCAutoForeignValueRef t_boxed_value;
    EXPECT_TRUE(MCForeignValueCreate(p_type_info, &p_value, &t_boxed_value));
    MCAutoStringRef t_description;
    EXPECT_TRUE(MCValueCopyDescription(*t_boxed_value, &t_description));
    if (t_description.IsSet())
        EXPECT_STREQ(MCStringGetCString(*t_description), t_format);
}
    
template<typename T, typename U, typename W = T>
void check_export(MCTypeInfoRef p_type_info, bool (*p_bridge)(W, U&), T p_value)
{
    MCAutoValueRefBase<U> t_bridge_value;
    EXPECT_TRUE(p_bridge(p_value, &t_bridge_value));
    MCAutoForeignValueRef t_boxed_value;
    EXPECT_TRUE(MCForeignValueExport(p_type_info, *t_bridge_value, &t_boxed_value));
    if (t_boxed_value.IsSet())
        EXPECT_EQ(*static_cast<T *>(MCForeignValueGetContentsPtr(*t_boxed_value)), p_value);
}

void check_integral_export_overflow(MCTypeInfoRef p_type_info, MCTypeInfoRef p_err_type_info, int64_t p_value)
{
    MCAutoNumberRef t_bridge_value;
    EXPECT_TRUE(MCNumberCreateWithReal(p_value, &t_bridge_value));
    MCAutoForeignValueRef t_boxed_value;
    EXPECT_FALSE(MCForeignValueExport(p_type_info, *t_bridge_value, &t_boxed_value));
    if (!t_boxed_value.IsSet())
    {
        MCAutoErrorRef t_error;
        EXPECT_TRUE(MCErrorCatch(&t_error));
        EXPECT_EQ(MCValueGetTypeInfo(*t_error), p_err_type_info);
    }
}

template<typename T, typename U, typename W = T>
void check_export_raw(MCTypeInfoRef p_type_info, bool (*p_bridge)(W, U&), T p_value)
{
    MCAutoValueRefBase<U> t_bridge_value;
    EXPECT_TRUE(p_bridge(p_value, &t_bridge_value));
    const MCForeignTypeDescriptor *t_desc = MCForeignTypeInfoGetDescriptor(p_type_info);
    EXPECT_TRUE(t_desc->doexport != nullptr);
    T t_exported_value;
    EXPECT_TRUE(t_desc->doexport(t_desc, *t_bridge_value, false, &t_exported_value));
    EXPECT_EQ(t_exported_value, p_value);
}

template<typename T>
void check_integral_raw_export_overflow(MCTypeInfoRef p_type_info, MCTypeInfoRef p_err_type_info, int64_t p_value)
{
    MCAutoNumberRef t_bridge_value;
    EXPECT_TRUE(MCNumberCreateWithReal(p_value, &t_bridge_value));
    const MCForeignTypeDescriptor *t_desc = MCForeignTypeInfoGetDescriptor(p_type_info);
    EXPECT_TRUE(t_desc->doexport != nullptr);
    MCAutoForeignValueRef t_boxed_value;
    T t_exported_value = 0;
    EXPECT_FALSE(t_desc->doexport(t_desc, *t_bridge_value, false, &t_exported_value));
    if (t_exported_value != 0)
    {
        MCAutoErrorRef t_error;
        EXPECT_TRUE(MCErrorCatch(&t_error));
        EXPECT_EQ(MCValueGetTypeInfo(*t_error), p_err_type_info);
    }
}

template<typename T, typename U, typename W = T>
void check_import_raw(MCTypeInfoRef p_type_info, bool (*p_bridge)(W, U&), T p_value)
{
    MCAutoValueRefBase<U> t_bridge_value;
    EXPECT_TRUE(p_bridge(p_value, &t_bridge_value));
    const MCForeignTypeDescriptor *t_desc = MCForeignTypeInfoGetDescriptor(p_type_info);
    EXPECT_TRUE(t_desc->doimport != nullptr);
    MCAutoValueRef t_imported_value;
    EXPECT_TRUE(t_desc->doimport(t_desc, &p_value, false, &t_imported_value));
    if (t_imported_value.IsSet())
        EXPECT_TRUE(MCValueIsEqualTo(*t_imported_value, *t_bridge_value));
}

template<typename T>
void check_integral_raw_import_overflow(MCTypeInfoRef p_type_info, MCTypeInfoRef p_err_type_info, T p_value)
{
    const MCForeignTypeDescriptor *t_desc = MCForeignTypeInfoGetDescriptor(p_type_info);
    EXPECT_TRUE(t_desc->doimport != nullptr);
    MCAutoValueRef t_imported_value;
    EXPECT_FALSE(t_desc->doimport(t_desc, &p_value, false, &t_imported_value));
    if (!t_imported_value.IsSet())
    {
        MCAutoErrorRef t_error;
        EXPECT_TRUE(MCErrorCatch(&t_error));
        EXPECT_EQ(MCValueGetTypeInfo(*t_error), p_err_type_info);
    }
}

/* Bool Type Tests */

static void
test_bool_type(MCTypeInfoRef p_type, const char *p_false, const char *p_true)
{
    /* Check Copy */
    check_creation<bool>(p_type, false);
    check_creation<bool>(p_type, true);

    /* Check Equality */
    check_equality<bool>(p_type, false, false, true);
    check_equality<bool>(p_type, true, true, true);
    check_equality<bool>(p_type, false, true, false);
    check_equality<bool>(p_type, true, false, false);

    /* Check Hash */
    check_hash<bool>(p_type, false, MCHashBool);
    check_hash<bool>(p_type, true, MCHashBool);

    /* Check describe */
    check_describe<bool>(p_type, false,p_false);
    check_describe<bool>(p_type, true, p_true);
    
    /* Check export (via api) - from Boolean */
    check_export<bool>(p_type, MCBooleanCreateWithBool, false);
    check_export<bool>(p_type, MCBooleanCreateWithBool, true);
    
    /* Check export (direct) - from Boolean */
    check_export_raw<bool>(p_type, MCBooleanCreateWithBool, false);
    check_export_raw<bool>(p_type, MCBooleanCreateWithBool, true);
    
    /* Check import (direct) - to Boolean */
    check_import_raw<bool>(p_type, MCBooleanCreateWithBool, false);
    check_import_raw<bool>(p_type, MCBooleanCreateWithBool, true);
}

TEST(foreign, Bool)
{
    test_bool_type(kMCBoolTypeInfo, "<foreign bool false>", "<foreign bool true>");
}

TEST(foreign, CBool)
{
    test_bool_type(kMCCBoolTypeInfo, "<foreign c bool false>", "<foreign c bool true>");
}

/* Pointer Type Tests */

TEST(foreign, Pointer)
{
    const MCForeignTypeDescriptor *t_desc = MCForeignTypeInfoGetDescriptor(kMCPointerTypeInfo);
    
    /* Check Copy */
    check_creation<void*>(kMCPointerTypeInfo, nullptr);
    check_creation<void*>(kMCPointerTypeInfo, &kMCPointerTypeInfo);

    /* Check Equality */
    check_equality<void*>(kMCPointerTypeInfo, nullptr, nullptr, true);
    check_equality<void*>(kMCPointerTypeInfo, &kMCPointerTypeInfo, &kMCPointerTypeInfo, true);
    check_equality<void*>(kMCPointerTypeInfo, nullptr, &kMCPointerTypeInfo, false);
    check_equality<void*>(kMCPointerTypeInfo, &kMCPointerTypeInfo, nullptr, false);

    /* Check Hash */
    check_hash<void*>(kMCPointerTypeInfo, nullptr, MCHashPointer);
    check_hash<void*>(kMCPointerTypeInfo, &kMCPointerTypeInfo, MCHashPointer);

    /* Check describe */
    check_describe<void*>(kMCPointerTypeInfo, nullptr, "<foreign pointer %p>");
    check_describe<void*>(kMCPointerTypeInfo, &kMCPointerTypeInfo, "<foreign pointer %p>");
    
    /* Check initialize */
    ASSERT_NE(t_desc->initialize, nullptr);
    if (t_desc->initialize != nullptr)
    {
        void *t_test = &kMCPointerTypeInfo;
        ASSERT_TRUE(t_desc->initialize(&t_test));
        ASSERT_EQ(t_test, nullptr);
    }
    
    /* Check defined */
    ASSERT_NE(t_desc->defined, nullptr);
    if (t_desc->defined != nullptr)
    {
        void *t_test_null = nullptr;
        ASSERT_FALSE(t_desc->defined(&t_test_null));
        ASSERT_TRUE(t_desc->defined(&kMCPointerTypeInfo));
    }
}

/* Real Type Tests */

template<typename T, typename U, typename W = T>
void test_numeric(MCTypeInfoRef p_type_info, T x, T y, hash_t (*p_hash)(W value), const char *p_format_str, bool (*p_bridge)(W value, U& r_bridged_value))
{
    check_creation<T>(p_type_info, x);
    check_creation<T>(p_type_info, y);
    
    /* Check Equality */
    check_equality<T>(p_type_info, x, x, true);
    check_equality<T>(p_type_info, y, y, true);
    check_equality<T>(p_type_info, x, y, false);
    check_equality<T>(p_type_info, y, x, false);

    /* Check Hash */
    check_hash<T>(p_type_info, x, p_hash);
    check_hash<T>(p_type_info, y, p_hash);

    /* Check describe */
    check_describe<T>(p_type_info, x, p_format_str);
    check_describe<T>(p_type_info, y, p_format_str);
    
    /* Check export (via api) */
    check_export<T>(p_type_info, p_bridge, x);
    check_export<T>(p_type_info, p_bridge, y);
    
    /* Check export (direct) */
    check_export_raw<T>(p_type_info, p_bridge, x);
    check_export_raw<T>(p_type_info, p_bridge, y);
    
    /* Check import (direct) */
    check_import_raw<T>(p_type_info, p_bridge, x);
    check_import_raw<T>(p_type_info, p_bridge, y);
}

TEST(foreign, Float)
{
    test_numeric<float, MCNumberRef, double>(kMCFloatTypeInfo, FLT_MIN, FLT_MAX, MCHashDouble, "<foreign float %g>", MCNumberCreateWithReal);
}

TEST(foreign, Double)
{
    test_numeric<double>(kMCDoubleTypeInfo, DBL_MIN, DBL_MAX, MCHashDouble, "<foreign double %lg>", MCNumberCreateWithReal);
}

TEST(foreign, NaturalFloat)
{
#ifdef __32_BIT__
    test_numeric<float>(kMCNaturalFloatTypeInfo, FLT_MIN, FLT_MAX, MCHashDouble, "<foreign natural float %lg>", MCNumberCreateWithReal);
#else
    test_numeric<double>(kMCNaturalFloatTypeInfo, DBL_MIN, DBL_MAX, MCHashDouble, "<foreign natural float %lg>", MCNumberCreateWithReal);
#endif
}

/* Integral Type Tests */

template<typename T>
hash_t __hash_int(T p_value)
{
    if (std::is_signed<T>::value)
        return MCHashInt64(p_value);
    return MCHashUInt64(p_value);
}

template<typename T>
bool __bridge_int(T p_value, MCNumberRef& r_number)
{
    if (sizeof(T) > sizeof(integer_t))
    {
        return MCNumberCreateWithReal(p_value, r_number);
    }

    if (std::is_signed<T>::value)
    {
        return MCNumberCreateWithInteger(p_value, r_number);
    }
    
    return MCNumberCreateWithUnsignedInteger(p_value, r_number);
}

#define TEST_INTEGRAL(name, type, format) \
    TEST(foreign, name) \
    { \
        test_integral<type>(kMC##name##TypeInfo, "<foreign " format ">"); \
    }

#define INT_NUMBER_MIN -(1LL << std::numeric_limits<double>::digits)
#define INT_NUMBER_MAX 1LL << std::numeric_limits<double>::digits

template<typename T>
void test_integral(MCTypeInfoRef p_type_info, const char *p_format)
{
    bool t_check_export_overflow = false;
    bool t_check_import_min_overflow = false;
    bool t_check_import_max_overflow = false;
    
    int64_t t_min, t_max;
    
    // For C integer types of less than 64-bits in size, we can represent the
    // whole range in MCNumberRefs. For 64-bit size integral types, we can only
    // represent the exact integer range of double. Thus we compute the minimum
    // and maximum values based on the type size and arity.
    if (sizeof(T) <= 4)
    {
        t_min = std::numeric_limits<T>::min();
        t_max = std::numeric_limits<T>::max();
        t_check_export_overflow = true;
        t_check_import_min_overflow = false;
        t_check_import_max_overflow = false;
    }
    else
    {
        if (std::is_signed<T>::value)
        {
            t_min = INT_NUMBER_MIN;
            t_check_import_min_overflow = true;
        }
        else
        {
            t_min = 0;
            t_check_import_min_overflow = false;

        }
        
        t_max = INT_NUMBER_MAX;
        t_check_import_max_overflow = true;
    }
    
    // Check all the base characteristics.
    test_numeric<T, MCNumberRef>(p_type_info, t_min, t_max, __hash_int, p_format, __bridge_int);
    
    // If the minimum of the type is greater than the min integer number can
    // hold, then check exporting a smaller value throws an error.
    if (t_check_export_overflow)
    {
        check_integral_export_overflow(p_type_info, kMCForeignExportErrorTypeInfo, int64_t(t_min) - 1);
        check_integral_raw_export_overflow<T>(p_type_info, kMCForeignExportErrorTypeInfo, int64_t(t_min) - 1);
    }
    
    // If the maximum of the type is less than the max integer number can hold,
    // then check exporting a larger value throws an error.
    if (t_check_export_overflow)
    {
        check_integral_export_overflow(p_type_info, kMCForeignExportErrorTypeInfo, int64_t(t_max) + 1);
        check_integral_raw_export_overflow<T>(p_type_info, kMCForeignExportErrorTypeInfo, int64_t(t_max) + 1);
    }
    
    // If the minimum value of the type is less than the minimum integer number
    // can hold, check importing a smaller value throws an error.
    if (t_check_import_min_overflow)
    {
        check_integral_raw_import_overflow<T>(p_type_info, kMCForeignImportErrorTypeInfo, t_min - 1);
    }
    
    // If the maximum value of the type is greater than the maximum integer
    // number can hold, check importing a larger value throws an error.
    if (t_check_import_max_overflow)
    {
        check_integral_raw_import_overflow<T>(p_type_info, kMCForeignImportErrorTypeInfo, t_max + 1);
    }
}

TEST_INTEGRAL(UInt8, uint8_t, "8-bit unsigned integer %u")
TEST_INTEGRAL(SInt8, int8_t, "8-bit signed integer %d")
TEST_INTEGRAL(UInt16, uint16_t, "16-bit unsigned integer %u")
TEST_INTEGRAL(SInt16, int16_t, "16-bit signed integer %d")
TEST_INTEGRAL(UInt32, uint32_t, "32-bit unsigned integer %u")
TEST_INTEGRAL(SInt32, int32_t, "32-bit signed integer %d")
TEST_INTEGRAL(UInt64, uint64_t, "64-bit unsigned integer %llu")
TEST_INTEGRAL(SInt64, int64_t, "64-bit signed integer %lld")

TEST_INTEGRAL(UIntSize, size_t, "unsigned size %zu")
TEST_INTEGRAL(SIntSize, ssize_t, "signed size %zd")
TEST_INTEGRAL(UIntPtr, uintptr_t, "unsigned intptr %zu")
TEST_INTEGRAL(SIntPtr, intptr_t, "signed intptr %zd")

TEST_INTEGRAL(NaturalUInt, uintptr_t, "natural unsigned integer %zu")
TEST_INTEGRAL(NaturalSInt, intptr_t, "natural signed integer %zd")

TEST_INTEGRAL(CChar, char, "c char '%c'");
TEST_INTEGRAL(CUChar, unsigned char, "c unsigned char %u")
TEST_INTEGRAL(CSChar, signed char, "c signed char %d")
TEST_INTEGRAL(CUShort, unsigned short, "c unsigned short %u")
TEST_INTEGRAL(CSShort, signed short, "c signed short %d")
TEST_INTEGRAL(CUInt, unsigned int, "c unsigned int %u")
TEST_INTEGRAL(CSInt, signed int, "c signed int %d")
TEST_INTEGRAL(CULong, unsigned long, "c unsigned long %lu")
TEST_INTEGRAL(CSLong, signed long, "c signed long %ld")
TEST_INTEGRAL(CULongLong, unsigned long long, "c unsigned long long %llu")
TEST_INTEGRAL(CSLongLong, signed long long, "c signed long long %lld")

TEST_INTEGRAL(UInt, uinteger_t, "unsigned integer %u")
TEST_INTEGRAL(SInt, integer_t, "signed integer %d")

