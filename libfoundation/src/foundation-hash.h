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

#ifndef MC_FOUNDATION_HASH_H
#define MC_FOUNDATION_HASH_H

#include <cmath>
#include <limits>
#include <type_traits>

/* ----------------------------------------------------------------
 * Integer hashes
 * ---------------------------------------------------------------- */

// These hash functions are taken from CoreFoundation - if they are good enough
// for Apple, they should be good enough for us :)

static const hash_t kMCHashFactor = 2654435761U;

template<typename ValueType, typename Enable = void>
struct MCHashIntImpl {
    static_assert(sizeof(ValueType) == 0,
                  "Missing MCHashImpl specialization"); };

// Hash an arbitrary sized Int - the hash code generated is
// independent on the number of bytes representing the integer or the
// sign of the integer.
template<typename Integer>
inline hash_t MCHashInt(Integer i)
{
    return MCHashIntImpl<Integer>::hash(i);
}

/* Specialization of integer hashing for any size of unsigned integer. */
template<typename Unsigned>
struct MCHashIntImpl<
    Unsigned, typename std::enable_if<std::is_unsigned<Unsigned>::value>::type>
{
    static hash_t hash(Unsigned i)
    {
        hash_t h = 0;
        for (decltype(sizeof(Unsigned)) o = 0;
             o < sizeof(Unsigned);
             o += sizeof(hash_t))
            h += kMCHashFactor * hash_t(i >> (o << 3));
        return h;
    }
};

/* Specialization of integer hashing for any size of signed integer. */
template<typename Signed>
struct MCHashIntImpl<
    Signed, typename std::enable_if<std::is_signed<Signed>::value>::type>
{
    static hash_t hash(Signed i)
    {
        using Unsigned = typename std::make_unsigned<Signed>::type;
        return MCHashInt<Unsigned>(i >= 0 ? i : -i);
    }
};

/* ----------------------------------------------------------------
 * Floating-point hashes
 * ---------------------------------------------------------------- */

// Hash a floating point value - the hash code generated is
// independent of the number of bytes representing the value and if
// the value is an integer it will be the same as using MCHashInt.
template <typename FloatingPoint>
inline hash_t MCHashFloatingPoint(FloatingPoint f)
{
    if (f < 0)
        f = -f;

    double truncated = std::floor(f + 0.5);
    hash_t t_int_hash = MCHashInt(hash_t(std::fmod(truncated, UINT32_MAX)));
    return t_int_hash + hash_t((truncated - f) * UINT32_MAX);
}

/* ----------------------------------------------------------------
 * Byte sequence hashing
 * ---------------------------------------------------------------- */

/* The ELF hash algorithm used in the ELF object file format. */
class MCHashBytesContext
{
    hash_t m_hash = 0;

    void step(byte_t p_byte)
    {
        uint32_t T1 = (m_hash << 4) + p_byte;
        uint32_t T2 = T1 & 0xF0000000;
        if (T2)
        {
            T1 ^= (T2 >> 24);
        }
        T1 &= ~T2;
        m_hash = T1;
    }

public:
    constexpr MCHashBytesContext() = default;

    constexpr MCHashBytesContext(hash_t p_initial_hash)
    : m_hash(p_initial_hash)
    {}

    operator hash_t() const { return m_hash; }

    void consume(const byte_t *p_bytes,
                 size_t p_length)
    {
        while (p_length--) step(*p_bytes++);
    }
};

/* ----------------------------------------------------------------
 * Object representation hashing
 * ---------------------------------------------------------------- */

/* These functions are compatible with `MCHashBytes()` and
 * `MCHashBytesStream()`, and are used when it's desirable to hash a
 * single value's C++ object representation.  They are only enabled
 * for TriviallyCopyable types, i.e. types where the value
 * representation is a subset of the object representation. */

template <typename T>
inline hash_t MCHashObject(T p_object)
{
    /* TODO[C++11] Enable this assertion once all our C++ compilers support it. */
    /* static_assert(std::is_trivially_copyable<T>::value,
           "MCHashObject can only be used with trivially copyable types"); */
    return MCHashBytes(reinterpret_cast<const void*>(&p_object),
                       sizeof(p_object));
}

template <typename T>
inline hash_t MCHashObjectStream(hash_t p_start,
                                 T p_object)
{
    /* TODO[C++11] Enable this assertion once all our C++ compilers support it. */
    /* static_assert(std::is_trivially_copyable<T>::value,
           "MCHashObjectStream can only be used with trivially copyable types"); */
    return MCHashBytesStream(p_start,
                             reinterpret_cast<const void*>(&p_object),
                             sizeof(p_object));
}

/* Hash multiple objects of possibly-differing types into a hash
 * stream. */
template <typename Head, typename... Tail>
inline hash_t MCHashObjectStream(hash_t p_start,
                                 Head p_object,
                                 Tail ... p_others)
{
    return MCHashObjectStream(MCHashObjectStream(p_start, p_object),
                              p_others ...);
}

#endif /*!MC_FOUNDATION_HASH_H*/
