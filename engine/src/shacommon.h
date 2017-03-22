/*                                                                     -*-c++-*-
Copyright (C) 2017 LiveCode Ltd.

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

/* Add some definitions normally provided by librhash headers */

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define I64(x) x##ui64
#else
#define I64(x) x##LL
#endif

#define be2me_32 MCSwapInt32BigToHost
#define be2me_64 MCSwapInt64BigToHost
#define le2me_32 MCSwapInt32LittleToHost
#define le2me_64 MCSwapInt64LittleToHost

template <typename T>
static inline void
be_copy(unsigned char* to,
        size_t index,
        T* from,
        size_t length)
{
    auto dst_end = to + index + length;
    auto src = from;
    auto dst = to + index;
    for ( ; dst < dst_end; src += 1, dst += sizeof(*src))
    {
        auto v = MCSwapIntBigToHost(*src);
        memcpy(dst, &v, sizeof(v));
    }
}
#define be32_copy be_copy<uint32_t>
#define be64_copy be_copy<uint64_t>

static inline void
me64_to_le_str(unsigned char *to,
               uint64_t *from,
               size_t length)
{
	auto dst_end = to + length;
	auto src = from;
	auto dst = to;
	for ( ; dst < dst_end; src += 1, dst += sizeof(*src))
	{
		auto v = MCSwapIntHostToLittle(*src);
		memcpy(dst, &v, sizeof(v));
	}
}

template <typename T> static inline bool
IS_ALIGNED(const void *p) { return (uintptr_t(p) % sizeof(T)) == 0; }
#define IS_ALIGNED_32 IS_ALIGNED<uint32_t>
#define IS_ALIGNED_64 IS_ALIGNED<uint64_t>

/* ROTL/ROTR macros rotate a 32/64-bit word left/right by n bits */
#define ROTL32(dword, n) ((dword) << (n) ^ ((dword) >> (32 - (n))))
#define ROTR32(dword, n) ((dword) >> (n) ^ ((dword) << (32 - (n))))
#define ROTL64(qword, n) ((qword) << (n) ^ ((qword) >> (64 - (n))))
#define ROTR64(qword, n) ((qword) >> (n) ^ ((qword) << (64 - (n))))
