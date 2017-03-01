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
#include <foundation-stdlib.h>

#include "foundation-private.h"
#include "foundation-hash.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef __LINUX__
const char *__MCSysCharset;
#endif

////////////////////////////////////////////////////////////////////////////////

bool MCInitialize(void)
{
    if (!__MCUnicodeInitialize())
        return false;
    
    if (!__MCValueInitialize())
		return false;
    
	if (!__MCStringInitialize())
		return false;
    
	if (!__MCNameInitialize())
		return false;
    
    if (!__MCErrorInitialize())
        return false;
    
    if (!__MCTypeInfoInitialize())
        return false;
    
    if (!__MCForeignValueInitialize())
        return false;
    
    if (!__MCNumberInitialize())
        return false;
    
	if (!__MCArrayInitialize())
		return false;
    
	if (!__MCListInitialize())
		return false;
    
	if (!__MCSetInitialize())
		return false;
    
    if (!__MCDataInitialize())
        return false;
    
    if (!__MCRecordInitialize())
        return false;
    
    if (!__MCLocaleInitialize())
        return false;

    if (!__MCProperListInitialize())
        return false;
    
    if (!__MCStreamInitialize())
        return false;
    
    if (!__MCJavaInitialize())
        return false;
    
	return true;
}

void MCFinalize(void)
{
    __MCStreamFinalize();
    __MCProperListFinalize();
	__MCLocaleFinalize();
    __MCRecordFinalize();
    __MCDataFinalize();
    __MCSetFinalize();
	__MCListFinalize();
	__MCArrayFinalize();
    __MCNumberFinalize();
    __MCForeignValueFinalize();
    __MCTypeInfoFinalize();
    __MCErrorFinalize();
	__MCNameFinalize();
	__MCStringFinalize();
    __MCUnicodeFinalize();
    __MCJavaFinalize();
    
    // Finalize values last
	__MCValueFinalize();
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCMemoryAllocate(size_t p_size, void*& r_block)
{
	void *t_block;
	t_block = malloc(p_size != 0 ? p_size : 4);
	if (t_block != nil)
	{
		r_block = t_block;
		return true;
	}
	return MCErrorThrowOutOfMemory();
}

MC_DLLEXPORT_DEF
bool MCMemoryAllocateCopy(const void *p_block, size_t p_block_size, void*& r_block)
{
	if (MCMemoryAllocate(p_block_size, r_block))
	{
		MCMemoryCopy(r_block, p_block, p_block_size);
		return true;
	}
	return MCErrorThrowOutOfMemory();
}

MC_DLLEXPORT_DEF
bool MCMemoryReallocate(void *p_block, size_t p_new_size, void*& r_new_block)
{
	void *t_new_block;
	t_new_block = realloc(p_block, p_new_size != 0 ? p_new_size : 4);
	if (t_new_block != nil)
	{
		r_new_block = t_new_block;
		return true;
	}
	return MCErrorThrowOutOfMemory();
}

MC_DLLEXPORT_DEF
void MCMemoryDeallocate(void *p_block)
{
	free(p_block);
}

//////////

MC_DLLEXPORT_DEF
bool MCMemoryNew(size_t p_size, void*& r_record)
{
	if (MCMemoryAllocate(p_size, r_record))
	{
		MCMemoryClear(r_record, p_size);
		return true;
	}
	return false;
}

MC_DLLEXPORT_DEF
void MCMemoryDelete(void *p_record)
{
	MCMemoryDeallocate(p_record);
}

//////////

bool MCMemoryNewArray(uindex_t p_count, size_t p_size, void*& r_array)
{
	if (MCMemoryAllocate(p_count * p_size, r_array))
	{
		MCMemoryClear(r_array, p_count * p_size);
		return true;
	}
	return false;
}

bool MCMemoryNewArray(uindex_t p_count, size_t p_size, void*& r_array, uindex_t& r_count)
{
	if (MCMemoryAllocate(p_count * p_size, r_array))
	{
		MCMemoryClear(r_array, p_count * p_size);
		r_count = p_count;
		return true;
	}
	return false;
}

bool MCMemoryResizeArray(uindex_t p_new_count, size_t p_size, void*& x_array, uindex_t& x_count)
{
	if (MCMemoryReallocate(x_array, p_new_count * p_size, x_array))
	{
		if (p_new_count > x_count)
			MCMemoryClear(static_cast<char *>(x_array) + x_count * p_size, (p_new_count - x_count) * p_size);
		x_count = p_new_count;
		return true;
	}
	return false;
}

void MCMemoryDeleteArray(void *p_array)
{
	MCMemoryDeallocate(p_array);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
hash_t MCHashInteger(integer_t i)
{
	return __MCHashInt (i);
}

MC_DLLEXPORT_DEF
hash_t
MCHashUInteger (uinteger_t i)
{
	return __MCHashUInt(i);
}

MC_DLLEXPORT_DEF
hash_t
MCHashSize (ssize_t i)
{
	return __MCHashInt (i);
}

hash_t
MCHashUSize (size_t i)
{
	return __MCHashUInt (i);
}

MC_DLLEXPORT_DEF
hash_t MCHashPointer(const void *p)
{
	return __MCHashUInt((uintptr_t) p);
}

MC_DLLEXPORT_DEF
hash_t MCHashDouble(double d)
{
    return __MCHashFlt(d);
}

MC_DLLEXPORT_DEF
hash_t MCHashBytes(const void *p_bytes, size_t p_length)
{
    MCAssert(nil != p_bytes || 0 == p_length);

    __MCHashBytesContext t_hash;
    t_hash.Consume(reinterpret_cast<const byte_t *>(p_bytes),
                   p_length);
    
    return t_hash.Current();
}

MC_DLLEXPORT_DEF
hash_t MCHashBytesStream(hash_t p_start, const void *p_bytes, size_t p_length)
{
	MCAssert(p_bytes != nil || 0 == p_length);
    
    __MCHashBytesContext t_hash(p_start);
    t_hash.Consume(reinterpret_cast<const byte_t *>(p_bytes),
                   p_length);
    
    return t_hash.Current();
}

// Note: MCHashNativeChars is implemented in foundation-string-native.cpp.h
// Note: MCHashChars is implemented in foundation-unicode.cpp

////////////////////////////////////////////////////////////////////////////////
