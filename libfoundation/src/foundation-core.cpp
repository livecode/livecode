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

	if (!__MCArrayInitialize())
		return false;

	if (!__MCListInitialize())
		return false;

	if (!__MCSetInitialize())
		return false;
    
    if (!__MCDataInitialize())
        return false;
    
    if (!__MCLocaleInitialize())
        return false;

	return true;
}

void MCFinalize(void)
{
	__MCLocaleFinalize();
    __MCSetFinalize();
	__MCListFinalize();
	__MCArrayFinalize();
	__MCNameFinalize();
	__MCStringFinalize();
    __MCDataFinalize();
	__MCValueFinalize();
    __MCUnicodeFinalize();
}

////////////////////////////////////////////////////////////////////////////////

bool MCMemoryAllocate(size_t p_size, void*& r_block)
{
	void *t_block;
	t_block = malloc(p_size != 0 ? p_size : 4);
	if (t_block != nil)
	{
		r_block = t_block;
		return true;
	}
	return MCErrorThrow(kMCErrorOutOfMemory);
}

bool MCMemoryAllocateCopy(const void *p_block, size_t p_block_size, void*& r_block)
{
	if (MCMemoryAllocate(p_block_size, r_block))
	{
		MCMemoryCopy(r_block, p_block, p_block_size);
		return true;
	}
	return MCErrorThrow(kMCErrorOutOfMemory);
}

bool MCMemoryReallocate(void *p_block, size_t p_new_size, void*& r_new_block)
{
	void *t_new_block;
	t_new_block = realloc(p_block, p_new_size != 0 ? p_new_size : 4);
	if (t_new_block != nil)
	{
		r_new_block = t_new_block;
		return true;
	}
	return MCErrorThrow(kMCErrorOutOfMemory);
}

void MCMemoryDeallocate(void *p_block)
{
	free(p_block);
}

//////////

bool MCMemoryNew(size_t p_size, void*& r_record)
{
	if (MCMemoryAllocate(p_size, r_record))
	{
		MCMemoryClear(r_record, p_size);
		return true;
	}
	return false;
}

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

// These hash functions are taken from CoreFoundation - if they are good enough
// for Apple, they should be good enough for us :)

#define HASHFACTOR 2654435761U

hash_t MCHashInteger(integer_t i)
{
	return ((i > 0) ? (hash_t)i : (hash_t)(-i)) * HASHFACTOR;
}

hash_t MCHashDouble(double d)
{
	double i;
	if (d < 0)
		d = -d;
	i = floor(d + 0.5);
	
	hash_t t_integral_hash;
	t_integral_hash = HASHFACTOR * (hash_t)fmod(i, (double)UINT32_MAX);

	return (hash_t)(t_integral_hash + (hash_t)((d - i) * UINT32_MAX));
}

#define ELF_STEP(B) T1 = (H << 4) + B; T2 = T1 & 0xF0000000; if (T2) T1 ^= (T2 >> 24); T1 &= (~T2); H = T1;

hash_t MCHashBytes(const void *p_bytes, size_t length)
{
	uint8_t *bytes = (uint8_t *)p_bytes;

    /* The ELF hash algorithm, used in the ELF object file format */
    uint32_t H = 0, T1, T2;
    int32_t rem = length;

    while (3 < rem)
	{
		ELF_STEP(bytes[length - rem]);
		ELF_STEP(bytes[length - rem + 1]);
		ELF_STEP(bytes[length - rem + 2]);
		ELF_STEP(bytes[length - rem + 3]);
		rem -= 4;
    }

    switch (rem)
	{
    case 3:  ELF_STEP(bytes[length - 3]);
    case 2:  ELF_STEP(bytes[length - 2]);
    case 1:  ELF_STEP(bytes[length - 1]);
    case 0:  ;
    }

    return H;
}

#undef ELF_STEP

////////////////////////////////////////////////////////////////////////////////
