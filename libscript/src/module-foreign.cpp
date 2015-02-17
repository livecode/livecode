/*                                                                     -*-c++-*-
 Copyright (C) 2015 Runtime Revolution Ltd.
 
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

MC_DLLEXPORT MCTypeInfoRef kMCNativeCStringTypeInfo;

////////////////////////////////////////////////////////////////////////////////

static bool __cstring_initialize(void *contents)
{
    *(void **)contents = nil;
    return true;
}

static void __cstring_finalize(void *contents)
{
    free(*(void **)contents);
}

static bool __cstring_defined(void *contents)
{
    return *(void **)contents != nil;
}

static bool __cstring_move(void *from, void *to)
{
    *(void **)to = *(void **)from;
    return true;
}

static bool __cstring_copy(void *from, void *to)
{
    if (*(void **)from == nil)
    {
        *(void **)to = nil;
        return true;
    }
    
    size_t t_length;
    t_length = strlen(*(char **)from) + 1;
    
    char *t_new_string;
    if (!MCMemoryNewArray(t_length, t_new_string))
        return false;
    
    MCMemoryCopy(t_new_string, from, t_length);
    
    *(char **)to = t_new_string;
    
    return true;
}

static bool __cstring_equal(void *left, void *right, bool& r_equal)
{
    if (*(void **)left == nil || *(void **)right == nil)
        return left == right;
    
    return strcmp(*(char **)left, *(char **)right) == 0;
}

static bool __cstring_hash(void *value, hash_t& r_hash)
{
    if (*(void **)value == nil)
    {
        r_hash = 0;
        return true;
    }
    
    r_hash = MCHashBytes(value, strlen(*(char **)value));
    
    return true;
}

static bool __nativecstring_import(void *contents, bool release, MCValueRef& r_value)
{
    if (release)
        return MCStringCreateWithCStringAndRelease(*(char **)contents, (MCStringRef&)r_value);
    
    return MCStringCreateWithCString(*(const char **)contents, (MCStringRef&)r_value);
}

static bool __nativecstring_export(MCValueRef value, bool release, void *contents)
{
    char *t_cstring_value;
    if (!MCStringConvertToCString((MCStringRef)value, t_cstring_value))
        return false;
    
    if (release)
        MCValueRelease(value);
    
    *(char **)contents = t_cstring_value;
    
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

bool MCForeignModuleInitialize(void)
{
    MCForeignPrimitiveType p;
    MCForeignTypeDescriptor d;
    d . size = sizeof(char *);
    d . basetype = kMCNullTypeInfo;
    d . bridgetype = kMCStringTypeInfo;
    p = kMCForeignPrimitiveTypePointer;
    d . layout = &p;
    d . layout_size = 1;
    d . initialize = __cstring_initialize;
    d . finalize = __cstring_finalize;
    d . defined = __cstring_defined;
    d . move = __cstring_move;
    d . copy = __cstring_copy;
    d . equal = __cstring_equal;
    d . hash = __cstring_hash;
    d . doimport = __nativecstring_import;
    d . doexport = __nativecstring_export;
    if (!__build_typeinfo("com.livecode.foreign.NativeCString", &d, kMCNativeCStringTypeInfo))
        return false;
    
    return true;
}

void MCForeignModuleFinalize(void)
{
    MCValueRelease(kMCNativeCStringTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////
