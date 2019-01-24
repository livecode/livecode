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

#ifdef HAVE_VALGRIND
#  include <valgrind/memcheck.h>
#endif /* HAVE_VALGRIND */

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

static bool __MCValueInter(__MCValue *value, bool release, MCValueRef& r_unique_value);
static void __MCValueUninter(__MCValue *value);

////////////////////////////////////////////////////////////////////////////////

MCTypeInfoRef __MCCustomValueResolveTypeInfo(__MCValue *p_value)
{
    __MCCustomValue *t_value;
    t_value = (__MCCustomValue*)p_value;
    return __MCTypeInfoResolve(t_value -> typeinfo);
}

MC_DLLEXPORT_DEF
bool MCValueCreateCustom(MCTypeInfoRef p_typeinfo, size_t p_extra_bytes, MCValueRef& r_value)
{
	__MCAssertIsTypeInfo(p_typeinfo);

	__MCValue *t_value;
	if (!__MCValueCreate(kMCValueTypeCodeCustom, sizeof(__MCCustomValue) + p_extra_bytes, t_value))
		return false;

	__MCCustomValue *self;
	self = (__MCCustomValue *)t_value;
	self -> typeinfo = MCValueRetain(p_typeinfo);

	r_value = self;
	
	return true;
}

MC_DLLEXPORT_DEF
MCValueTypeCode MCValueGetTypeCode(MCValueRef p_value)
{
	__MCValue *self = (__MCValue *)p_value;

	MCAssert(self != nil);
    __MCAssertIsValue(self);

	return __MCValueGetTypeCode(self);
}

MC_DLLEXPORT_DEF
MCTypeInfoRef MCValueGetTypeInfo(MCValueRef p_value)
{
	MCAssert(p_value != nil);
    __MCAssertIsValue(p_value);
    
    switch(MCValueGetTypeCode(p_value))
    {
        case kMCValueTypeCodeNull:
            return kMCNullTypeInfo;
        case kMCValueTypeCodeBoolean:
            return kMCBooleanTypeInfo;
        case kMCValueTypeCodeNumber:
            return kMCNumberTypeInfo;
        case kMCValueTypeCodeName:
            return kMCNameTypeInfo;
        case kMCValueTypeCodeString:
            return kMCStringTypeInfo;
        case kMCValueTypeCodeData:
            return kMCDataTypeInfo;
        case kMCValueTypeCodeArray:
            return kMCArrayTypeInfo;
        case kMCValueTypeCodeList:
            return kMCListTypeInfo;
        case kMCValueTypeCodeSet:
            return kMCSetTypeInfo;
        case kMCValueTypeCodeProperList:
            return kMCProperListTypeInfo;
        case kMCValueTypeCodeCustom:
            return ((__MCCustomValue *)p_value) -> typeinfo;
        case kMCValueTypeCodeRecord:
            return ((__MCRecord *)p_value) -> typeinfo;
        case kMCValueTypeCodeHandler:
            return ((__MCHandler *)p_value) -> typeinfo;
        case kMCValueTypeCodeError:
            return ((__MCError *)p_value) -> typeinfo;
        case kMCValueTypeCodeForeignValue:
            return ((__MCForeignValue *)p_value) -> typeinfo;
    }
    
    MCLog("%p, %d", p_value, MCValueGetTypeCode(p_value));
    
    MCUnreachableReturn(kMCNullTypeInfo);
}

MC_DLLEXPORT_DEF
uindex_t MCValueGetRetainCount(MCValueRef p_value)
{
	__MCValue *self = (__MCValue *)p_value;
    
	MCAssert(self != nil);
    __MCAssertIsValue(self);
    
    return self -> references;
}

MC_DLLEXPORT_DEF
MCValueRef MCValueRetain(MCValueRef p_value)
{
	__MCValue *self = (__MCValue *)p_value;

	MCAssert(self != nil);
    __MCAssertIsValue(self);
    
	MCAssert(self -> references != UINT32_MAX);
	self -> references += 1;

	return self;
}

MC_DLLEXPORT_DEF
void MCValueRelease(MCValueRef p_value)
{
	__MCValue *self = (__MCValue *)p_value;

	if (self == nil)
        return;
    
    __MCAssertIsValue(self);
        
    uint32_t t_new_references;
    t_new_references = self -> references - 1;
    if (t_new_references == 0)
    {
        __MCValueDestroy(self);
        return;
    }
    
    self -> references = t_new_references;
    return;
}

MC_DLLEXPORT_DEF
bool MCValueCopy(MCValueRef p_value, MCValueRef& r_immutable_copy)
{
	MCAssert(p_value != nil);
    __MCAssertIsValue(p_value);
    
	__MCValue *t_copy;
	if (__MCValueImmutableCopy((__MCValue *)p_value, false, t_copy))
	{
		r_immutable_copy = (MCValueRef)t_copy;
		return true;
	}
	
	return false;
}

MC_DLLEXPORT_DEF
bool MCValueCopyAndRelease(MCValueRef p_value, MCValueRef& r_immutable_copy)
{
	MCAssert(p_value != nil);
    __MCAssertIsValue(p_value);
    
	__MCValue *t_copy;
	if (__MCValueImmutableCopy((__MCValue *)p_value, true, t_copy))
	{
		r_immutable_copy = (MCValueRef)t_copy;
		return true;
	}
	
	return false;
}

MC_DLLEXPORT_DEF
hash_t MCValueHash(MCValueRef p_value)
{
	__MCValue *self = (__MCValue *)p_value;
    
	MCAssert(self != nil);
    __MCAssertIsValue(self);
    
	switch(__MCValueGetTypeCode(self))
	{
	case kMCValueTypeCodeNull:
		return (hash_t)0xdeadbeef;
	case kMCValueTypeCodeBoolean:
		return p_value == kMCTrue ? 0xfeedbeef : 0xdeadfeed;
	case kMCValueTypeCodeName:
		return __MCNameHash((__MCName *)self);
	case kMCValueTypeCodeNumber:
		return __MCNumberHash((__MCNumber *)self);
	case kMCValueTypeCodeString:
		return __MCStringHash((__MCString *)self);
	case kMCValueTypeCodeArray:
		return __MCArrayHash((__MCArray *)self);
	case kMCValueTypeCodeList:
		return __MCListHash((__MCList *)self);
	case kMCValueTypeCodeSet:
		return __MCSetHash((__MCSet *)self);
    case kMCValueTypeCodeData:
        return __MCDataHash((__MCData*) self);
	case kMCValueTypeCodeCustom:
		{
			MCTypeInfoRef t_typeinfo;
			hash_t (*t_hash_func)(MCValueRef);
			t_typeinfo = __MCCustomValueResolveTypeInfo(self);
			t_hash_func = t_typeinfo -> custom . callbacks . hash;
			return ((t_hash_func != NULL) ?
			        t_hash_func (p_value) :
			        __MCCustomDefaultHash (p_value));
		}
    case kMCValueTypeCodeProperList:
        return __MCProperListHash((__MCProperList *)self);
    case kMCValueTypeCodeRecord:
        return __MCRecordHash((__MCRecord*) self);
    case kMCValueTypeCodeHandler:
        return __MCHandlerHash((__MCHandler*) self);    
    case kMCValueTypeCodeTypeInfo:
        return __MCTypeInfoHash((__MCTypeInfo*) self);
    case kMCValueTypeCodeError:
        return __MCErrorHash((__MCError*)self);
    case kMCValueTypeCodeForeignValue:
        return __MCForeignValueHash((__MCForeignValue *)self);
	default:
        break;
	}

	MCUnreachableReturn(false);
}

MC_DLLEXPORT_DEF
bool MCValueIsEqualTo(MCValueRef p_value, MCValueRef p_other_value)
{
	__MCValue *self = (__MCValue *)p_value;
	__MCValue *other_self = (__MCValue *)p_other_value;

	MCAssert(self != nil);
    __MCAssertIsValue(self);
    
	MCAssert(other_self != nil);
    __MCAssertIsValue(other_self);
    
	// If the pointers are the same, we are equal.
	if (self == other_self)
		return true;

	// If the typecodes are different, we are not equal.
	if (__MCValueGetTypeCode(self) != __MCValueGetTypeCode(other_self))
		return false;

    // If both values are interred, then they can't be equal.
    if (MCValueIsUnique(p_value) && MCValueIsUnique(p_other_value))
        return false;
    
	switch(__MCValueGetTypeCode(self))
	{
	// There is only one null value, so if we get here, we are not equal.
	case kMCValueTypeCodeNull:
		return false;
	// Booleans are singletons, so if we get here, we are not equal.
	case kMCValueTypeCodeBoolean:
		return false;
	// Names are singletons, so if we get here, we are not equal.
	case kMCValueTypeCodeName:
		return false;
	// Defer to the number comparison method.
	case kMCValueTypeCodeNumber:
		return __MCNumberIsEqualTo((__MCNumber *)self, (__MCNumber *)other_self);
	// Defer to the string comparison method.
	case kMCValueTypeCodeString:
		return __MCStringIsEqualTo((__MCString *)self, (__MCString *)other_self);
	// Defer to the array comparison method.
	case kMCValueTypeCodeArray:
		return __MCArrayIsEqualTo((__MCArray *)self, (__MCArray *)other_self);
	// Defer to the list comparison method.
	case kMCValueTypeCodeList:
		return __MCListIsEqualTo((__MCList *)self, (__MCList *)other_self);
	case kMCValueTypeCodeSet:
		return __MCSetIsEqualTo((__MCSet *)self, (__MCSet *)other_self);
    case kMCValueTypeCodeData:
        return __MCDataIsEqualTo((__MCData*)self, (__MCData*)other_self);
	// Defer to the custom comparison method, but only if the typeinfo are
	// the same.
	case kMCValueTypeCodeCustom:
		if (((__MCCustomValue *)self) -> typeinfo == ((__MCCustomValue *)other_self) -> typeinfo)
		{
			MCTypeInfoRef t_typeinfo;
			bool (*t_equal_func)(MCValueRef, MCValueRef);
			t_typeinfo = __MCCustomValueResolveTypeInfo(self);
			t_equal_func = t_typeinfo -> custom . callbacks . equal;
			return ((t_equal_func != NULL) ?
			        t_equal_func (p_value, p_other_value) :
			        __MCCustomDefaultEqual (p_value, p_other_value));
		}
		return false;
    case kMCValueTypeCodeProperList:
        return __MCProperListIsEqualTo((__MCProperList*)self, (__MCProperList*)other_self);
    case kMCValueTypeCodeRecord:
        return __MCRecordIsEqualTo((__MCRecord*)self, (__MCRecord*)other_self);
    case kMCValueTypeCodeHandler:
        return __MCHandlerIsEqualTo((__MCHandler*)self, (__MCHandler*)other_self);    
    case kMCValueTypeCodeTypeInfo:
        return __MCTypeInfoIsEqualTo((__MCTypeInfo *)self, (__MCTypeInfo *)other_self);
    case kMCValueTypeCodeError:
        return __MCErrorIsEqualTo((__MCError *)self, (__MCError *)other_self);
    case kMCValueTypeCodeForeignValue:
        return __MCForeignValueIsEqualTo((__MCForeignValue *)self, (__MCForeignValue *)other_self);
	// Shouldn't happen!
	default:
		break;
	}

	// We should never get here!
	MCUnreachableReturn(false);
}

MC_DLLEXPORT_DEF
bool MCValueCopyDescription(MCValueRef p_value, MCStringRef& r_desc)
{
	__MCValue *self = (__MCValue *)p_value;
    
	MCAssert(self != nil);
    __MCAssertIsValue(self);
    
	switch(__MCValueGetTypeCode(self))
	{
	case kMCValueTypeCodeNull:
		return MCStringCopy (MCSTR("<null>"), r_desc);
	case kMCValueTypeCodeBoolean:
		return MCStringCopy (MCSTR(p_value == kMCTrue ? "true" : "false"), r_desc);
	case kMCValueTypeCodeNumber:
		return __MCNumberCopyDescription((__MCNumber *)p_value, r_desc);
	case kMCValueTypeCodeString:
		return __MCStringCopyDescription((__MCString *)p_value, r_desc);
	case kMCValueTypeCodeName:
		return __MCNameCopyDescription((__MCName *)p_value, r_desc);
	case kMCValueTypeCodeArray:
		return __MCArrayCopyDescription((__MCArray *)p_value, r_desc);
	case kMCValueTypeCodeList:
		return __MCListCopyDescription((__MCList *)p_value, r_desc);
	case kMCValueTypeCodeSet:
		return __MCSetCopyDescription((__MCSet *)p_value, r_desc);
    case kMCValueTypeCodeData:
        return __MCDataCopyDescription((__MCData*)p_value, r_desc);
	case kMCValueTypeCodeCustom:
		return __MCCustomCopyDescription((__MCCustomValue *) p_value, r_desc);
    case kMCValueTypeCodeProperList:
        return __MCProperListCopyDescription((__MCProperList*)p_value, r_desc);
    case kMCValueTypeCodeRecord:
        return __MCRecordCopyDescription((__MCRecord*)p_value, r_desc);
    case kMCValueTypeCodeHandler:
        return __MCHandlerCopyDescription((__MCHandler*)p_value, r_desc);
    case kMCValueTypeCodeTypeInfo:
        return __MCTypeInfoCopyDescription((__MCTypeInfo*)p_value, r_desc);
    case kMCValueTypeCodeError:
        return __MCErrorCopyDescription((__MCError*)p_value, r_desc);
    case kMCValueTypeCodeForeignValue:
        return __MCForeignValueCopyDescription((__MCForeignValue *)p_value, r_desc);
	default:
		return MCStringCopy (MCSTR("<unknown>"), r_desc);
	}
	MCUnreachableReturn(false);
}

MC_DLLEXPORT_DEF
void MCValueLog(MCValueRef p_value)
{
    MCAutoStringRef t_desc;
    if (MCValueCopyDescription(p_value, &t_desc))
        MCLog("%p: %@", p_value, *t_desc);
    else
        MCLog("%p: Cannot describe", p_value);
}

//////////

MC_DLLEXPORT_DEF
bool MCValueIsMutable(MCValueRef p_value)
{
	__MCValue *self = (__MCValue *)p_value;
    
	MCAssert(self != nil);
    __MCAssertIsValue(self);
    
    if (__MCValueGetTypeCode(self) != kMCValueTypeCodeCustom)
        return false;

	MCTypeInfoRef t_typeinfo;
	bool (*t_is_mutable_func)(MCValueRef);
	t_typeinfo = __MCCustomValueResolveTypeInfo(self);
	t_is_mutable_func = t_typeinfo -> custom . callbacks . is_mutable;
	return ((t_is_mutable_func != NULL) ?
	        t_is_mutable_func (p_value) :
	        __MCCustomDefaultIsMutable (p_value));
}

MC_DLLEXPORT_DEF
bool MCValueMutableCopy(MCValueRef p_value, MCValueRef& r_mutable_copy)
{
	__MCValue *self = (__MCValue *)p_value;
    
	MCAssert(self != nil);
    __MCAssertIsValue(self);
    
    if (__MCValueGetTypeCode(self) != kMCValueTypeCodeCustom)
        return false;
    
	MCTypeInfoRef t_typeinfo;
	bool (*t_mutable_copy_func)(MCValueRef, bool, MCValueRef &);
	t_typeinfo = __MCCustomValueResolveTypeInfo(self);
	t_mutable_copy_func = t_typeinfo -> custom . callbacks . mutable_copy;
	return ((t_mutable_copy_func != NULL) ?
	        t_mutable_copy_func (p_value, false, r_mutable_copy) :
	        __MCCustomDefaultMutableCopy (p_value, false, r_mutable_copy));
}

MC_DLLEXPORT_DEF
bool MCValueMutableCopyAndRelease(MCValueRef p_value, MCValueRef& r_mutable_copy)
{
	__MCValue *self = (__MCValue *)p_value;
    
	MCAssert(self != nil);
    __MCAssertIsValue(self);
    
    if (__MCValueGetTypeCode(self) != kMCValueTypeCodeCustom)
        return false;
    
	MCTypeInfoRef t_typeinfo;
	bool (*t_mutable_copy_func)(MCValueRef, bool, MCValueRef &);
	t_typeinfo = __MCCustomValueResolveTypeInfo(self);
	t_mutable_copy_func = t_typeinfo -> custom . callbacks . mutable_copy;
	return ((t_mutable_copy_func != NULL) ?
	        t_mutable_copy_func (p_value, true, r_mutable_copy) :
	        __MCCustomDefaultMutableCopy (p_value, true, r_mutable_copy));
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCValueIsUnique(MCValueRef p_value)
{
	__MCValue *self = (__MCValue *)p_value;
    
	MCAssert(self != nil);
    __MCAssertIsValue(self);
    
	switch(__MCValueGetTypeCode(self))
	{
	case kMCValueTypeCodeNull:
	case kMCValueTypeCodeBoolean:
	case kMCValueTypeCodeName:
		return true;
	case kMCValueTypeCodeCustom:
		if (__MCCustomValueResolveTypeInfo(self) -> custom . callbacks . is_singleton)
			return true;
	default:
		break;
	}

	return (self -> flags & kMCValueFlagIsInterred) != 0;
}

MC_DLLEXPORT_DEF
bool MCValueInter(MCValueRef p_value, MCValueRef& r_unique_value)
{
	MCAssert(p_value != nil);
    __MCAssertIsValue(p_value);
    
	// If the value is already unique then this is just a copy.
	if (MCValueIsUnique(p_value))
	{
		MCValueRetain(p_value);
		r_unique_value = p_value;
		return true;
	}

	return __MCValueInter((__MCValue *)p_value, false, r_unique_value);
}

MC_DLLEXPORT_DEF
bool MCValueInterAndRelease(MCValueRef p_value, MCValueRef& r_unique_value)
{
	MCAssert(p_value != nil);
    __MCAssertIsValue(p_value);
    
	// If the value is already unique then this is just a copy but since
	// we also need to release value, we just return ourselves.
	if (MCValueIsUnique(p_value))
	{
		r_unique_value = p_value;
		return true;
	}

	return __MCValueInter((__MCValue *)p_value, true, r_unique_value);
}

////////////////////////////////////////////////////////////////////////////////

// This is the layout of a valueref which is on the free-list.
struct __MCFreedValue: public __MCValue
{
    __MCFreedValue *next;
};

// MW-2014-03-21: [[ Faster ]] Memory allocation is relatively slow, therefore
//   we use a per-typecode pool of previously used __MCValue's. This saves a
//   a per-value malloc, particularly for types which are short-lived (such
//   as numbers).
struct MCValuePool
{
    __MCFreedValue *values;
    uindex_t count;
};
static MCValuePool *s_value_pools;

// Stores the number of pools that we have.
uindex_t kMCValuePoolCount = kMCValueTypeCodeList + 1;

bool __MCValueCreate(MCValueTypeCode p_type_code, size_t p_size, __MCValue*& r_value)
{
	void *t_value;
	
    // MW-2014-03-21: [[ Faster ]] If we are pooling this typecode, and the
    //   pool isn't empty grab the ptr from there.
    if (p_type_code <= kMCValueTypeCodeList && s_value_pools[p_type_code] . count > 0)
    {
        t_value = s_value_pools[p_type_code] . values;

#ifdef HAVE_VALGRIND
		/* Valgrind support */
		/* Verify that the next buffer in the free list has actually
		 * been previously allocated to us and we're allowed to use
		 * it.  The first few bytes of the buffer should contain the
		 * address of the following buffer (if there is one). */
		VALGRIND_MAKE_MEM_UNDEFINED(t_value, p_size);
		VALGRIND_MAKE_MEM_DEFINED(t_value, sizeof (__MCFreedValue));
#endif /* HAVE_VALGRIND */

        // Check that the value we are about to return has not been corrupted
        // due to a dangling reference.
        MCAssert(((__MCFreedValue *)t_value) -> references == 0 &&
                 ((__MCFreedValue *)t_value) -> flags == UINT32_MAX);
        
        s_value_pools[p_type_code] . count -= 1;
        s_value_pools[p_type_code] . values = ((__MCFreedValue *)t_value) -> next;
        MCMemoryClear(t_value, p_size);
	}
    else
    {
        // The minimum size of a valueref has to be sizeof(MCValuePoolLink).
        // This is to ensure we have enough space to chain in the free list.
        
        if (p_size < sizeof(__MCFreedValue))
            p_size = sizeof(__MCFreedValue);
        
        if (!MCMemoryNew(p_size, t_value))
            return false;
    }
    
	__MCValue *self = (__MCValue *)t_value;

	self -> references = 1;
	self -> flags = (p_type_code << 28);
    
	r_value = self;

	return true;
}

void __MCValueDestroy(__MCValue *self)
{
    MCValueTypeCode t_code;
    t_code = __MCValueGetTypeCode(self);
    
	if ((self -> flags & kMCValueFlagIsInterred) != 0)
    {
        if (t_code != kMCValueTypeCodeName)
        {
            __MCValueUninter(self);
        }
    }

	switch(t_code)
	{
	case kMCValueTypeCodeNull:
	case kMCValueTypeCodeBoolean:
	case kMCValueTypeCodeNumber:
		break;
	case kMCValueTypeCodeString:
		__MCStringDestroy((__MCString *)self);
		break;
	case kMCValueTypeCodeName:
		__MCNameDestroy((__MCName *)self);
		break;
	case kMCValueTypeCodeArray:
		__MCArrayDestroy((__MCArray *)self);
		break;
	case kMCValueTypeCodeList:
		__MCListDestroy((__MCList *)self);
		break;
	case kMCValueTypeCodeSet:
		__MCSetDestroy((__MCSet *)self);
		break;
    case kMCValueTypeCodeData:
        __MCDataDestroy((__MCData *)self);
        break;
    case kMCValueTypeCodeProperList:
        __MCProperListDestroy((__MCProperList *)self);
        break;
	case kMCValueTypeCodeCustom:
		{
			MCTypeInfoRef t_typeinfo;
			void (*t_destroy_func)(MCValueRef);
			t_typeinfo = __MCCustomValueResolveTypeInfo(self);
			t_destroy_func = t_typeinfo -> custom . callbacks . destroy;
			if (t_destroy_func != NULL)
				t_destroy_func (self);
			else
				__MCCustomDefaultDestroy (self);
		}
        break;
    case kMCValueTypeCodeRecord:
        __MCRecordDestroy((__MCRecord *)self);
        break;
    case kMCValueTypeCodeHandler:
        __MCHandlerDestroy((__MCHandler *)self);
        break;
    case kMCValueTypeCodeTypeInfo:
        __MCTypeInfoDestroy((__MCTypeInfo *)self);
        break;
    case kMCValueTypeCodeError:
        __MCErrorDestroy((__MCError *)self);
        break;
    case kMCValueTypeCodeForeignValue:
        __MCForeignValueDestroy((__MCForeignValue *)self);
        break;
    default:
        // Shouldn't get here
        MCUnreachableReturn();
	}
	
	// Ensure that an immediate abort will be caused in Debug mode if a destroyed MCValueRef pointer is passed to
	// a libfoundation function
#ifdef _DEBUG
    self -> references = 0;
	self -> flags = UINT32_MAX;
#endif

    // MW-2014-03-21: [[ Faster ]] If we are pooling this typecode, and the
    //   pool isn't full, add it to the pool.
    if (t_code <= kMCValueTypeCodeList && s_value_pools[t_code] . count < 32)
    {
        s_value_pools[t_code] . count += 1;
        ((__MCFreedValue *)self) -> next = s_value_pools[t_code] . values;
        s_value_pools[t_code] . values = (__MCFreedValue *)self;

#ifdef HAVE_VALGRIND
		/* Valgrind support */
		/* Mark the pooled buffer as inaccessible. If anything tries
		 * to access it, Valgrind will log an error message. */
		size_t t_size;
		switch (t_code)
		{
		case kMCValueTypeCodeNull:    t_size = sizeof(__MCNull);    break;
		case kMCValueTypeCodeBoolean: t_size = sizeof(__MCBoolean); break;
		case kMCValueTypeCodeNumber:  t_size = sizeof(__MCNumber);  break;
		case kMCValueTypeCodeName:    t_size = sizeof(__MCName);    break;
		case kMCValueTypeCodeString:  t_size = sizeof(__MCString);  break;
		case kMCValueTypeCodeData:    t_size = sizeof(__MCData);    break;
		case kMCValueTypeCodeArray:   t_size = sizeof(__MCArray);   break;
		case kMCValueTypeCodeList:    t_size = sizeof(__MCList);    break;
		default:                      MCUnreachable();
		}
		VALGRIND_MAKE_MEM_NOACCESS(self, t_size);
#endif /* HAVE_VALGRIND */

		return;
    }
	
	MCMemoryDelete(self);
}

////////////////////////////////////////////////////////////////////////////////

struct __MCUniqueValueBucket
{
	hash_t hash;
	uintptr_t value;
};

// Prime numbers. Values above 100 have been adjusted up so that the
// malloced block size will be just below a multiple of 512; values
// above 1200 have been adjusted up to just below a multiple of 4096.
 const uindex_t __kMCValueHashTableSizes[] = {
    0, 3, 7, 13, 23, 41, 71, 127, 191, 251, 383, 631, 1087, 1723,
    2803, 4523, 7351, 11959, 19447, 31231, 50683, 81919, 132607,
    214519, 346607, 561109, 907759, 1468927, 2376191, 3845119,
    6221311, 10066421, 16287743, 26354171, 42641881, 68996069,
    111638519, 180634607, 292272623, 472907251,
#ifdef __HUGE__
    765180413UL, 1238087663UL, 2003267557UL, 3241355263UL, 5244622819UL,
#if NEED_64_BIT_INDICIES
    8485977589UL, 13730600407UL, 22216578047UL, 35947178479UL,
    58163756537UL, 94110934997UL, 152274691561UL, 246385626107UL,
    398660317687UL, 645045943807UL, 1043706260983UL, 1688752204787UL,
    2732458465769UL, 4421210670577UL, 7153669136377UL,
    11574879807461UL, 18728548943849UL, 30303428750843UL,
#endif
#endif
    UINDEX_MAX /* Custodian */
};

const uindex_t __kMCValueHashTableCapacities[] = {
    0, 3, 6, 11, 19, 32, 52, 85, 118, 155, 237, 390, 672, 1065,
    1732, 2795, 4543, 7391, 12019, 19302, 31324, 50629, 81956,
    132580, 214215, 346784, 561026, 907847, 1468567, 2376414,
    3844982, 6221390, 10066379, 16287773, 26354132, 42641916,
    68996399, 111638327, 180634415, 292272755,
#ifdef __HUGE__
    472907503UL, 765180257UL, 1238087439UL, 2003267722UL, 3241355160UL,
#if NEED_64_BIT_INDICIES
    5244622578UL, 8485977737UL, 13730600347UL, 22216578100UL,
    35947178453UL, 58163756541UL, 94110935011UL, 152274691274UL,
    246385626296UL, 398660317578UL, 645045943559UL, 1043706261135UL,
    1688752204693UL, 2732458465840UL, 4421210670552UL,
    7153669136706UL, 11574879807265UL, 18728548943682UL,
#endif
#endif
    UINDEX_MAX /* Custodian */
};

static uindex_t s_unique_value_count = 0;
static uint8_t s_unique_value_capacity_idx = 0;
static __MCUniqueValueBucket *s_unique_values = nil;

static uindex_t __MCValueFindUniqueValueBucket(__MCValue *p_value, hash_t p_hash)
{
	// Compute the number of buckets.
	uindex_t t_capacity;
	t_capacity = __kMCValueHashTableSizes[s_unique_value_capacity_idx];

	// Fold the hash code appropriately.
	uindex_t t_h1;
#if defined(__ARM__) && 0 // TODO 
	t_h1 = __MCHashFold(p_hash, s_unique_value_capacity_idx);
#else
	t_h1 = p_hash % t_capacity;
#endif

	// The initial index to probe.
	uindex_t t_probe;
	t_probe = t_h1;

	// The target for a new entry - if it ens up being UINDX_MAX it means the
	// table is full.
	uindex_t t_target_slot;
	t_target_slot = UINDEX_MAX;

	// Loop over all possible entries in the table - this is done starting at
	// probe.
	for(uindex_t i = 0; i < t_capacity; i++)
	{
		// Fetch the ptr to the bucket under consideration.
		__MCUniqueValueBucket *t_bucket;
		t_bucket = &s_unique_values[t_probe];

		// Take action depending on what is stored there.
		if (t_bucket -> value == UINTPTR_MIN)
		{
			// If the bucket's value is 0 then it means it is the end of the
			// chain. This means we are done, so set the target slot to this
			// bucket if there hasn't been a deleted one before now.
			if (t_target_slot == UINDEX_MAX)
				t_target_slot = t_probe;
		
			// We are done so exit the loop.
			break;
		}
		else if (t_bucket -> value == UINTPTR_MAX)
		{
			// If we encounter a deleted bucket then take that as the potential
			// target slot - if we haven't found one before.
			if (t_target_slot == UINDEX_MAX)
				t_target_slot = t_probe;
		}
		else
		{
			// If the slot has a value and it is equal to the one we are looking
			// for, we are done.
			if (p_value == (__MCValue *)t_bucket -> value ||
				(p_hash == t_bucket -> hash && MCValueIsEqualTo(p_value, (__MCValue *)t_bucket -> value)))
				return t_probe;
		}

		// Move to the next bucket.
		t_probe += 1;

		// Make sure we loop around to the start.
		if (t_capacity <= t_probe)
			t_probe -= t_capacity;
	}

	// If we get here it means the value was not found, so return the last target
	// slot we encountered.
	return t_target_slot;
}

static uindex_t __MCValueFindUniqueValueBucketForRemove(__MCValue *p_value, hash_t p_hash)
{
	// Compute the number of buckets.
	uindex_t t_capacity;
	t_capacity = __kMCValueHashTableSizes[s_unique_value_capacity_idx];

	// Fold the has code appropriately.
	uindex_t t_h1;
#if defined(__ARM__) && 0 // TODO
	t_h1 = __MCHashFold(p_hash, s_unique_value_capacity_idx);
#else
	t_h1 = p_hash % t_capacity;
#endif

	// The initial index to probe.
	uindex_t t_probe;
	t_probe = t_h1;

	// Loop over all possible entries in the table.
	for(uindex_t i = 0; i < t_capacity; i++)
	{
		// Fetch the ptr to the bucket under consideration.
		__MCUniqueValueBucket *t_bucket;
		t_bucket = &s_unique_values[t_probe];

		// If we find an undefined entry, then the value isn't present so we are done.
		if (t_bucket -> value == UINTPTR_MIN)
			return UINDEX_MAX;

		// If we find an entry containing p_value, we are done.
		if (t_bucket -> value == (uintptr_t)p_value)
			return t_probe;

		// Move to the next bucket.
		t_probe += 1;

		// Make sure we loop around to the start.
		if (t_capacity <= t_probe)
			t_probe -= t_capacity;
	}

	// If we get here then it means we couldn't find the value - this shouldn't happen
	// as we only call this method if the value is in the table. Anyhow, we return
	// 'UINDEX_MAX' to mark as not found.
	return UINDEX_MAX;
}

static uindex_t __MCValueFindUniqueValueBucketAfterRehash(__MCValue *p_value, hash_t p_hash)
{
	// Compute the number of buckets.
	uindex_t t_capacity;
	t_capacity = __kMCValueHashTableSizes[s_unique_value_capacity_idx];

	// Fold the has code appropriately.
	uindex_t t_h1;
#if defined(__ARM__) && 0 // TODO
	t_h1 = __MCHashFold(p_hash, s_unique_value_capacity_idx);
#else
	t_h1 = p_hash % t_capacity;
#endif

	// The initial index to probe.
	uindex_t t_probe;
	t_probe = t_h1;

	// Loop over all possible entries in the table.
	for(uindex_t i = 0; i < t_capacity; i++)
	{
		// Fetch the ptr to the bucket under consideration.
		__MCUniqueValueBucket *t_bucket;
		t_bucket = &s_unique_values[t_probe];

		// If we find an undefined entry, then we are done.
		if (t_bucket -> value == UINTPTR_MIN)
			return t_probe;

		// Move to the next bucket.
		t_probe += 1;

		// Make sure we loop around to the start.
		if (t_capacity <= t_probe)
			t_probe -= t_capacity;
	}

	// If we get here then it means there was no free space in the table - this shouldn't
	// be possible as we only call this method when there will be free space. Anyhow, we
	// return 'UINDEX_MAX' to mark as failed.
	return UINDEX_MAX;
}

static bool __MCValueRehashUniqueValues(index_t p_new_item_count)
{
	// Get the current capacity index.
	uindex_t t_new_capacity_idx;
	t_new_capacity_idx = s_unique_value_capacity_idx;
	if (p_new_item_count != 0)
	{
		// If we are shrinking we just shrink down to the level needed by the currently
		// used buckets.
		if (p_new_item_count < 0)
			p_new_item_count = 0;

		// Work out the smallest possible capacity greater than the requested capacity.
		uindex_t t_new_capacity_req;
		t_new_capacity_req = s_unique_value_count + p_new_item_count;
		for(t_new_capacity_idx = 0;
		    t_new_capacity_req > __kMCValueHashTableCapacities[t_new_capacity_idx];
		    ++t_new_capacity_idx);
	}

	// Fetch the old capacity and table.
	uindex_t t_old_capacity;
	__MCUniqueValueBucket *t_old_buckets;
	t_old_capacity = __kMCValueHashTableSizes[s_unique_value_capacity_idx];
	t_old_buckets = s_unique_values;

	// Create the new table.
	uindex_t t_new_capacity;
	__MCUniqueValueBucket *t_new_buckets;
	t_new_capacity = __kMCValueHashTableSizes[t_new_capacity_idx];
	if (!MCMemoryNewArray(t_new_capacity, t_new_buckets))
		return false;

	// Update the vars.
	s_unique_value_capacity_idx = t_new_capacity_idx;
	s_unique_values = t_new_buckets;

	// Now rehash the values from the old table.
	for(uindex_t i = 0; i < t_old_capacity; i++)
	{
		if (t_old_buckets[i] . value != UINTPTR_MIN && t_old_buckets[i] . value != UINTPTR_MAX)
		{
			uindex_t t_target_slot;
			t_target_slot = __MCValueFindUniqueValueBucketAfterRehash((__MCValue *)t_old_buckets[i] . value, t_old_buckets[i] . hash);

			// This assertion should never trigger - something is very wrong if it does!
			MCAssert(t_target_slot != UINDEX_MAX);

			s_unique_values[t_target_slot] . hash = t_old_buckets[i] . hash;
			s_unique_values[t_target_slot] . value = t_old_buckets[i] . value;
		}
	}

	// Delete the old table.
	MCMemoryDeleteArray(t_old_buckets);

	// We are done!
	return true;
}

static bool __MCValueInter(__MCValue *self, bool p_release, MCValueRef& r_unique_self)
{
	// Compute the hash code for the value.
	hash_t t_hash;
	t_hash = MCValueHash(self);

	// See if the value is already in the table.
	uindex_t t_target_slot;
	t_target_slot = __MCValueFindUniqueValueBucket(self, t_hash);

	// If a slot wasn't found, then rehash.
	if (t_target_slot == UINDEX_MAX)
	{
		if (!__MCValueRehashUniqueValues(1))
			return false;

		t_target_slot = __MCValueFindUniqueValueBucketAfterRehash(self, t_hash);
	}

	// If we still don't have a slot then just fail (this could happen if
	// memory is exhausted).
	if (t_target_slot == UINDEX_MAX)
		return false;

	// If the slot is not empty and not deleted then take that value.
	if (s_unique_values[t_target_slot] . value != UINTPTR_MIN &&
		s_unique_values[t_target_slot] . value != UINTPTR_MAX)
	{
		if (p_release && (MCValueRef)s_unique_values[t_target_slot] . value == self)
			r_unique_self = self;
        else
        {
            r_unique_self = MCValueRetain((MCValueRef)s_unique_values[t_target_slot] . value);
            
            if (p_release)
                MCValueRelease(self);
        }
        
		return true;
	}

	// Otherwise we must first ensure we have an immutable version of the
	// value and then insert it into the table.
	if (!__MCValueImmutableCopy(self, p_release, self))
		return false;

	self -> flags |= kMCValueFlagIsInterred;
	s_unique_values[t_target_slot] . hash = t_hash;
	s_unique_values[t_target_slot] . value = (uintptr_t)self;
	s_unique_value_count += 1;

	// Finally return the value in the target slot.
	r_unique_self = self;
	return true;
}

static void __MCValueUninter(__MCValue *self)
{
	// Compute the hash code for the value.
	hash_t t_hash;
	t_hash = MCValueHash(self);

	// Search for the value in the table.
	uindex_t t_target_slot;
	t_target_slot = __MCValueFindUniqueValueBucketForRemove(self, t_hash);

	// If we found it (we always should) then mark the bucket as deleted.
	if (t_target_slot != UINDEX_MAX)
	{
		s_unique_values[t_target_slot] . hash = 0;
		s_unique_values[t_target_slot] . value = (uintptr_t)UINTPTR_MAX;
		self -> flags &= ~kMCValueFlagIsInterred;
		s_unique_value_count -= 1;

		// TODO: Shrink the table if necessary (?)
	}
}

bool __MCValueImmutableCopy(__MCValue *self, bool p_release, __MCValue*& r_new_value)
{
	switch(__MCValueGetTypeCode(self))
	{
	case kMCValueTypeCodeString:
	{
		__MCString *t_new_value;
		if (__MCStringImmutableCopy((__MCString *)self, p_release, t_new_value))
			return r_new_value = t_new_value, true;
	}
	return false;

	case kMCValueTypeCodeArray:
	{
		__MCArray *t_new_value;
		if (__MCArrayImmutableCopy((__MCArray *)self, p_release, t_new_value))
			return r_new_value = t_new_value, true;
	}
	return false;

	case kMCValueTypeCodeList:
	{
		__MCList *t_new_value;
		if (__MCListImmutableCopy((__MCList *)self, p_release, t_new_value))
			return r_new_value = t_new_value, true;
	}
	return false;

	case kMCValueTypeCodeSet:
	{
		__MCSet *t_new_value;
		if (__MCSetImmutableCopy((__MCSet *)self, p_release, t_new_value))
			return r_new_value = t_new_value, true;
	}
	return false;

    case kMCValueTypeCodeData:
    {
        __MCData *t_new_value;
        if (__MCDataImmutableCopy((__MCData*)self, p_release, t_new_value))
            return r_new_value = t_new_value, true;
    }
    return false;
            
    case kMCValueTypeCodeProperList:
    {
        __MCProperList *t_new_value;
        if (__MCProperListImmutableCopy((__MCProperList*)self, p_release, t_new_value))
            return r_new_value = t_new_value, true;
    }
    return false;

	case kMCValueTypeCodeCustom:
	{
		MCValueRef t_new_value;
		MCTypeInfoRef t_typeinfo;
		bool (*t_copy_func)(MCValueRef, bool, MCValueRef &);
		t_typeinfo = __MCCustomValueResolveTypeInfo(self);
		t_copy_func = t_typeinfo -> custom . callbacks . copy;
		if ((t_copy_func != NULL) ?
		    t_copy_func (self, p_release, t_new_value) :
		    __MCCustomDefaultCopy (self, p_release, t_new_value))
			return r_new_value = (__MCValue *) t_new_value, true;
	}
	return false;
            
    case kMCValueTypeCodeRecord:
    {
        __MCRecord *t_new_value;
        if (__MCRecordImmutableCopy((__MCRecord*)self, p_release, t_new_value))
            return r_new_value = t_new_value, true;
    }
    return false;
            
	default:
		break;
	}

	if (!p_release)
		MCValueRetain(self);

	r_new_value = self;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCBooleanCreateWithBool(bool p_value, MCBooleanRef& r_boolean)
{
    r_boolean = MCValueRetain(p_value ? kMCTrue : kMCFalse);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCNullRef kMCNull;
MC_DLLEXPORT_DEF MCBooleanRef kMCTrue;
MC_DLLEXPORT_DEF MCBooleanRef kMCFalse;

bool __MCValueInitialize(void)
{
    if (!MCMemoryNewArray(kMCValuePoolCount, s_value_pools))
        return false;
    
	if (!__MCValueCreate(kMCValueTypeCodeNull, kMCNull))
		return false;

	if (!__MCValueCreate(kMCValueTypeCodeBoolean, kMCTrue))
		return false;
	
	if (!__MCValueCreate(kMCValueTypeCodeBoolean, kMCFalse))
		return false;

	if (!__MCValueRehashUniqueValues(1))
		return false;
    
	return true;
}

void __MCValueFinalize(void)
{
    // First delete the constant valuerefs.
    MCValueRelease(kMCFalse);
    kMCFalse = nil;
    
    MCValueRelease(kMCTrue);
    kMCTrue = nil;
    
    MCValueRelease(kMCNull);
    kMCNull = nil;
    
    // Next delete the unique value array.
    MCMemoryDeleteArray(s_unique_values);
    s_unique_values = nil;
    s_unique_value_count = 0;
    s_unique_value_capacity_idx = 0;
    
    // Make sure to delete the value pools last, as they need to be around until
    // all other valuerefs have been deleted.
    for(uindex_t i = 0; i < kMCValuePoolCount; i++)
        while(s_value_pools[i] . count > 0)
        {
            __MCFreedValue *t_value;
            t_value = s_value_pools[i] . values;
            
#ifdef HAVE_VALGRIND
			/* Valgrind support */
			/* The first few bytes of the buffer actually contain the
			 * address of the following buffer. */
			VALGRIND_MAKE_MEM_DEFINED(t_value, sizeof (__MCFreedValue));
#endif /* HAVE_VALGRIND */
            
			s_value_pools[i] . values = t_value -> next;
			s_value_pools[i] . count -= 1;
            MCMemoryDelete(t_value);
        }
	MCMemoryDeleteArray(s_value_pools);
    s_value_pools = nil;
}

////////////////////////////////////////////////////////////////////////////////
