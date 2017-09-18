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
#include <foundation-stdlib.h>

#include "foundation-private.h"
#include "foundation-hash.h"
#include "foundation-string-hash.h"

#if defined(__WINDOWS__)
#   include <Windows.h>
#endif

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
    
    if (!__MCObjcInitialize())
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
    __MCObjcFinalize();

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

/* Securely clear the memory located at dst, using volatile to try to
 * ensure that the compiler doesn't optimise it out. */
MC_DLLEXPORT
void MCMemoryClearSecure(byte_t* dst,
                         size_t size)
{
#if defined(__WINDOWS__)
    SecureZeroMemory(dst, size);
#else
    MCMemoryClear(dst, size);
    /* Add a barrier to prevent the compiler from optimizing the above
     * MCMemoryClear() call away.  Without this, both clang and GCC
     * will optimise out the memory clear.*/
    __asm__ __volatile__ ( "" : : "r"(dst) : "memory" );
#endif
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
hash_t MCHashBool(bool b)
{
    return hash_t(b);
}

MC_DLLEXPORT_DEF
hash_t MCHashInteger(integer_t i)
{
	return MCHashInt(i);
}

MC_DLLEXPORT_DEF
hash_t
MCHashUInteger (uinteger_t i)
{
	return MCHashInt(i);
}

MC_DLLEXPORT_DEF
hash_t
MCHashSize (ssize_t i)
{
	return MCHashInt(i);
}

MC_DLLEXPORT_DEF
hash_t
MCHashUSize (size_t i)
{
	return MCHashInt(i);
}

MC_DLLEXPORT_DEF
hash_t
MCHashInt64(int64_t i)
{
	return MCHashInt(i);
}

hash_t
MCHashUInt64(uint64_t i)
{
	return MCHashInt(i);
}
MC_DLLEXPORT_DEF
hash_t MCHashPointer(const void *p)
{
	return MCHashInt(reinterpret_cast<uintptr_t>(p));
}

MC_DLLEXPORT_DEF
hash_t MCHashDouble(double d)
{
    return MCHashFloatingPoint(d);
}

MC_DLLEXPORT_DEF
hash_t MCHashBytes(const void *p_bytes, size_t length)
{
    MCHashBytesContext t_context;
    t_context.consume(reinterpret_cast<const byte_t *>(p_bytes), length);
    return t_context;
}

MC_DLLEXPORT_DEF
hash_t MCHashBytes(MCSpan<const byte_t> p_bytes)
{
    return MCHashBytes(p_bytes.data(), p_bytes.size());
}

MC_DLLEXPORT_DEF
hash_t MCHashBytesStream(hash_t p_start, const void *p_bytes, size_t length)
{
    MCHashBytesContext t_context(p_start);
    t_context.consume(reinterpret_cast<const byte_t *>(p_bytes), length);
    return t_context;
}

MC_DLLEXPORT_DEF
hash_t MCHashBytesStream(hash_t p_start, MCSpan<const byte_t> p_bytes)
{
    return MCHashBytesStream(p_start, p_bytes.data(), p_bytes.size());
}

template <typename CodeUnit>
static hash_t hash_chars(CodeUnit *p_chars, size_t char_count)
{
    MCHashCharsContext t_context;
    while (char_count-- > 0) t_context.consume(*p_chars++);
    return t_context;
}

MC_DLLEXPORT_DEF
hash_t MCHashNativeChars(const char_t *chars, size_t char_count)
{
    return hash_chars(chars, char_count);
}

MC_DLLEXPORT_DEF
hash_t MCHashNativeChars(MCSpan<const char_t> p_chars)
{
    return hash_chars(p_chars.data(), p_chars.size());
}

MC_DLLEXPORT
hash_t MCHashChars(const unichar_t *chars, size_t char_count)
{
    return hash_chars(chars, char_count);
}

MC_DLLEXPORT_DEF
hash_t MCHashChars(MCSpan<const unichar_t> p_chars)
{
    return hash_chars(p_chars.data(), p_chars.size());
}

////////////////////////////////////////////////////////////////////////////////
