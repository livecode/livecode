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

#ifndef MC_FOUNDATION_STRING_HASH_H
#define MC_FOUNDATION_STRING_HASH_H

#include <foundation-hash.h>
#include <foundation-unicode.h>

/* Anonymous namespace to ensure that the functions defined here don't
 * get any external linkage. */
namespace {

/* ----------------------------------------------------------------
 * Character sequence hashing
 * ---------------------------------------------------------------- */

/* Uses the Fowler-Noll-Vo FNV-1a hash */

class MCHashCharsContext
{
    static const hash_t kPrime = (sizeof(hash_t) == sizeof(uint64_t) ?
                                  1099511628211ULL : 16777619UL);
    static const hash_t kOffset = (sizeof(hash_t) == sizeof(uint64_t) ?
                                   14695981039346656037ULL : 2166136261UL);

    hash_t m_hash = kOffset;

    // Perform a single step for a unicode code unit representable as a single
    // byte.
    void step(char_t p_char)
    {
        m_hash ^= p_char;
        m_hash *= kPrime;
        m_hash *= kPrime;
    }

    // Peform a single step for a two-byte code unit
    void step(unichar_t p_char)
    {
        // Hash the lower byte
        m_hash ^= p_char & 0xFF;
        m_hash *= kPrime;

        // Hash the upper byte
        m_hash ^= p_char >> 8;
        m_hash *= kPrime;
    }

public:
    constexpr MCHashCharsContext() = default;

    constexpr MCHashCharsContext(hash_t p_initial_hash)
    : m_hash(p_initial_hash)
    {}

    operator hash_t() const { return m_hash; }

    /* Hash single code units */
    void consume(char_t c) { step(c); }
    void consume(unichar_t u) { step(u); }

    /* Hash a full codepoint, treating it as a single codeunit if it
     * falls in the BMP and as two codeunits otherwise. */
    void consume(codepoint_t p_codepoint)
    {
        unichar_t t_leading = 0;
        unichar_t t_trailing = 0;
        if (!MCUnicodeCodepointToSurrogates(p_codepoint, t_leading, t_trailing))
        {
            step(t_leading);
        }
        else
        {
            step(t_leading);
            step(t_trailing);
        }
    }
};

} /* anonymous namespace */

#endif /* !MC_FOUNDATION_STRING_HASH_H */
