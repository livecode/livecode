/*                                                                     -*-c++-*-
 Copyright (C) 2015 LiveCode Ltd.
 
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

////////////////////////////////////////////////////////////////////////////////

#include <foundation.h>
#include <foundation-auto.h>

////////////////////////////////////////////////////////////////////////////////

MCTypeInfoRef kMCNativeCStringTypeInfo;
MCTypeInfoRef kMCWStringTypeInfo;
MCTypeInfoRef kMCUTF8StringTypeInfo;

extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCNativeCStringTypeInfo(void) { return kMCNativeCStringTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCWStringTypeInfo(void) { return kMCWStringTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCUTF8StringTypeInfo(void) { return kMCUTF8StringTypeInfo; }

MCTypeInfoRef kMCForeignZStringNullErrorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

/* Reject strings that contain nul characters; we can't convert
 * them to nul-terminated wide character strings without data
 * loss. */
static bool
MCForeignEvalStringNonNull (MCStringRef t_string)
{
	/* Reject strings that contain nul characters; we can't convert
	 * them to nul-terminated wide character strings without data
	 * loss. */
	uindex_t t_ignored;
	if (MCStringFirstIndexOfChar (t_string, 0, 0,
	                              kMCStringOptionCompareExact, t_ignored))
	{
		MCErrorCreateAndThrow (kMCForeignZStringNullErrorTypeInfo, nil);
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static bool __cbuffer_initialize(void *contents)
{
    *(void **)contents = nil;
    return true;
}

static void __cbuffer_finalize(void *contents)
{
    free(*(void **)contents);
}

static bool __cbuffer_defined(void *contents)
{
    return *(void **)contents != nil;
}

static bool __cbuffer_move(const MCForeignTypeDescriptor*, void *from, void *to)
{
    *(void **)to = *(void **)from;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

static bool __cstring_copy(const MCForeignTypeDescriptor*, void *from, void *to)
{
    if (*(void **)from == nil)
    {
        *(void **)to = nil;
        return true;
    }

    const char *t_old_string = *(char **) from;
    
    size_t t_length;
    t_length = strlen(t_old_string) + 1;
    
    char *t_new_string;
    if (!MCMemoryNewArray(t_length, t_new_string))
        return false;
    
    MCMemoryCopy(t_new_string, t_old_string, t_length);
    
    *(char **)to = t_new_string;
    
    return true;
}

static bool __cstring_equal(const MCForeignTypeDescriptor*, void *left, void *right, bool& r_equal)
{
    if (*(void **)left == nil || *(void **)right == nil)
		r_equal = (left == right);
	else
		r_equal = strcmp(*(char **)left, *(char **)right) == 0;
	return true;
}

static bool __cstring_hash(const MCForeignTypeDescriptor*, void *value, hash_t& r_hash)
{
    if (*(void **)value == nil)
    {
        r_hash = 0;
        return true;
    }
    
    r_hash = MCHashBytes(value, strlen(*(char **)value));
    
    return true;
}

static bool __nativecstring_import(const MCForeignTypeDescriptor*, void *contents, bool release, MCValueRef& r_value)
{
    if (release)
        return MCStringCreateWithCStringAndRelease(*(char **)contents, (MCStringRef&)r_value);
    
    return MCStringCreateWithCString(*(const char **)contents, (MCStringRef&)r_value);
}

static bool __nativecstring_export(const MCForeignTypeDescriptor*, MCValueRef value, bool release, void *contents)
{
	if (!MCForeignEvalStringNonNull ((MCStringRef) value))
		return false;

    char *t_cstring_value;
    if (!MCStringConvertToCString((MCStringRef)value, t_cstring_value))
        return false;
    
    if (release)
        MCValueRelease(value);
    
    *(char **)contents = t_cstring_value;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

/* Compute length excluding trailing nul */
static size_t
__wstring_len (const unichar_t *value)
{
	uindex_t len;
	for (len = 0; value[len] != 0; ++len);
	return len;
}

static bool
__wstring_copy (const MCForeignTypeDescriptor*,
                void *from,
                void *to)
{
	if (nil == *(void **) from)
	{
		*(void **)to = nil;
		return true;
	}

	const unichar_t *t_old_string = *(unichar_t **) from;

	/* Allocate enough space for string + trailing nul */
	size_t t_length;
	t_length = sizeof (unichar_t) * (1 + __wstring_len (t_old_string));

	unichar_t *t_new_string;
	if (!MCMemoryNewArray (t_length, t_new_string))
		return false;

	MCMemoryCopy (t_new_string, t_old_string, t_length);

	*(unichar_t **)to = t_new_string;

	return true;
}

static bool
__wstring_equal (const MCForeignTypeDescriptor*,
                 void *left,
                 void *right,
                 bool & r_equal)
{
	if (nil == *(void **) left || nil == *(void **) right)
		return left == right;

	const unichar_t *t_left = *(unichar_t **) left;
	const unichar_t *t_right = *(unichar_t **) right;

	size_t i = 0;
	while (true)
	{
		if (0 == t_left[i] && 0 == t_right[i])
			break;
		if (t_left[i] != t_right[i])
			return false;
		++i;
	}
	return true;
}

static bool
__wstring_hash (const MCForeignTypeDescriptor*,
                void *value,
                hash_t & r_hash)
{
	if (nil == *(void **)value)
	{
		r_hash = 0;
		return true;
	}

	const unichar_t *t_value = *(unichar_t **) value;
	r_hash = MCHashBytes (t_value, __wstring_len (t_value) * sizeof(unichar_t));
	return true;
}

static bool
__wstring_import (const MCForeignTypeDescriptor*,
                  void *contents,
                  bool release,
                  MCValueRef & r_value)
{
	if (release)
		return MCStringCreateWithWStringAndRelease (*(unichar_t **) contents,
		                                            (MCStringRef &) r_value);
	else
		return MCStringCreateWithWString(*(const unichar_t **) contents,
		                                 (MCStringRef &) r_value);
}

static bool
__wstring_export (const MCForeignTypeDescriptor*,
                  MCValueRef value,
                  bool release,
                  void *contents)
{
	if (!MCForeignEvalStringNonNull ((MCStringRef) value))
		return false;

	unichar_t *t_wstring_value;
	if (!MCStringConvertToWString ((MCStringRef) value, t_wstring_value))
		return false;

	if (release)
		MCValueRelease (value);

	*(unichar_t **)contents = t_wstring_value;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

static bool
__utf8string_import (const MCForeignTypeDescriptor*,
                     void *contents,
                     bool release,
                     MCValueRef & r_value)
{
	char *t_utf8string;
	t_utf8string = *(char **) contents;

	if (release)
		return MCStringCreateWithBytesAndRelease((byte_t*)t_utf8string, t_utf8string?strlen(t_utf8string):0, kMCStringEncodingUTF8, false, (MCStringRef&)r_value);
	else
		return MCStringCreateWithBytes((byte_t*)t_utf8string, t_utf8string?strlen(t_utf8string):0, kMCStringEncodingUTF8, false, (MCStringRef&)r_value);
}

static bool
__utf8string_export (const MCForeignTypeDescriptor*,
                     MCValueRef value,
                     bool release,
                     void *contents)
{
	if (!MCForeignEvalStringNonNull ((MCStringRef) value))
		return false;
	
	char *t_utf8string_value;
	if (!MCStringConvertToUTF8String((MCStringRef)value, t_utf8string_value))
		return false;
	
	if (release)
		MCValueRelease (value);
	
	*(char **)contents = t_utf8string_value;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_foreign_Initialize(void)
{
    MCForeignPrimitiveType p;
    MCForeignTypeDescriptor d;
    d . size = sizeof(char *);
    d . basetype = kMCNullTypeInfo;
    d . bridgetype = kMCStringTypeInfo;
    p = kMCForeignPrimitiveTypePointer;
    d . layout = &p;
    d . layout_size = 1;
    d . initialize = __cbuffer_initialize;
    d . finalize = __cbuffer_finalize;
    d . defined = __cbuffer_defined;
    d . move = __cbuffer_move;
    d . copy = __cstring_copy;
    d . equal = __cstring_equal;
    d . hash = __cstring_hash;
    d . doimport = __nativecstring_import;
    d . doexport = __nativecstring_export;
    d . describe = nullptr;
    d . promotedtype = kMCNullTypeInfo;
    d . promote = nullptr;
    if (!__build_typeinfo("com.livecode.foreign.NativeCString", &d, kMCNativeCStringTypeInfo))
        return false;

	d . size = sizeof(unichar_t *);
	d . basetype = kMCNullTypeInfo;
	d . bridgetype = kMCStringTypeInfo;
	p = kMCForeignPrimitiveTypePointer;
	d . layout = &p;
	d . layout_size = 1;
	d . initialize = __cbuffer_initialize;
	d . finalize = __cbuffer_finalize;
	d . defined = __cbuffer_defined;
	d . move = __cbuffer_move;
	d . copy = __wstring_copy;
	d . equal = __wstring_equal;
	d . hash = __wstring_hash;
	d . doimport = __wstring_import;
	d . doexport = __wstring_export;
    d . describe = nullptr;
    d . promotedtype = kMCNullTypeInfo;
    d . promote = nullptr;
	if (!__build_typeinfo("com.livecode.foreign.WString", &d, kMCWStringTypeInfo))
		return false;

	d . size = sizeof(char *);
	d . basetype = kMCNullTypeInfo;
	d . bridgetype = kMCStringTypeInfo;
	p = kMCForeignPrimitiveTypePointer;
	d . layout = &p;
	d . layout_size = 1;
	d . initialize = __cbuffer_initialize;
	d . finalize = __cbuffer_finalize;
	d . defined = __cbuffer_defined;
	d . move = __cbuffer_move;
	d . copy = __cstring_copy;
	d . equal = __cstring_equal;
	d . hash = __cstring_hash;
	d . doimport = __utf8string_import;
	d . doexport = __utf8string_export;
    d . describe = nullptr;
    d . promotedtype = kMCNullTypeInfo;
    d . promote = nullptr;
	if (!__build_typeinfo("com.livecode.foreign.UTF8String", &d, kMCUTF8StringTypeInfo))
		return false;
	
	/* ---------- */

	if (!MCNamedErrorTypeInfoCreate (MCNAME("com.livecode.foreign.NullInZStringError"), MCNAME("foreign"), MCSTR("cannot export char U+0000 in nul-terminated string buffer"), kMCForeignZStringNullErrorTypeInfo))
		return false;

    return true;
}

extern "C" void com_livecode_foreign_Finalize(void)
{
    MCValueRelease(kMCNativeCStringTypeInfo);
	MCValueRelease(kMCWStringTypeInfo);

	MCValueRelease(kMCForeignZStringNullErrorTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////
