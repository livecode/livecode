/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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

#ifndef __MC_FOUNDATION_HASH__
#define __MC_FOUNDATION_HASH__

#include <cmath>

#include "foundation-unicode.h"

////////////////////////////////////////////////////////////////////////////////

// These hash functions are taken from CoreFoundation - if they are good enough
// for Apple, they should be good enough for us :)

#define HASHFACTOR 2654435761U

// Hash an arbitrary sized UInt - the hash code generated is independent on the
// number of bytes representing the integer.
template <typename T>
static inline hash_t
__MCHashUInt(T i)
{
    hash_t h = 0;
    for (unsigned int o = 0; o < sizeof(T); o += sizeof(hash_t))
        h += HASHFACTOR * (hash_t) (i >> (o << 3));
    return h;
}

// Hash an arbitrary sized Int - the hash code generated is independent on the
// number of bytes representing the integer.
template <typename T>
static inline hash_t
__MCHashInt(T i)
{
    return __MCHashUInt((i >= 0) ? i : (-i));
}

// Hash a floating point value - the hash code generated is independent of the
// number of bytes representing the value and if the value is an integer it will
// be the same as using __MCHashInt.
template <typename T>
static inline hash_t
__MCHashFlt(T d)
{
    if (d < 0)
        d = -d;
    
    double i;
    i = std::floor(d + 0.5);
    
    hash_t t_integral_hash;
    t_integral_hash = HASHFACTOR * (hash_t)std::fmod(i, UINT32_MAX);
    
    return (hash_t)(t_integral_hash + (hash_t)((d - i) * UINT32_MAX));
}

////////////////////////////////////////////////////////////////////////////////

// Hash an arbitrary sequence of bytes.
class __MCHashBytesContext
{
private:
    void Step(byte_t p_byte)
    {
        uint32_t T1, T2;
        T1 = (m_hash << 4) + p_byte;
        T2 = T1 & 0xF0000000;
        if (T2)
        {
            T1 ^= (T2 >> 24);
        }
        T1 &= ~T2;
        m_hash = T1;
    }
    
public:
    // Default constructor for hashing a sequence of bytes from the start.
    __MCHashBytesContext(void)
        : m_hash(0)
    {
    }
    
    // Constructor for continuing a hash of a sequence of bytes.
    __MCHashBytesContext(hash_t p_initial_hash)
        : m_hash(p_initial_hash)
    {
    }
    
    // Reset hashing to an initial value.
    void Reset(hash_t p_initial_hash = 0)
    {
        m_hash = p_initial_hash;
    }
    
    // Hash a further p_length bytes.
    void Consume(const byte_t *p_bytes,
                 size_t p_length)
    {
        while(3 < p_length)
        {
            Step(*p_bytes++);
            Step(*p_bytes++);
            Step(*p_bytes++);
            Step(*p_bytes++);
            p_length -= 4;
        }
        
        switch(p_length)
        {
            case 3: Step(*p_bytes++);
            case 2: Step(*p_bytes++);
            case 1: Step(*p_bytes++);
            case 0: ;
        }
    }
    
    // Return the hash of all bytes processed so far.
    hash_t Current(void) const
    {
        return m_hash;
    }

private:
    hash_t m_hash;
};

////////////////////////////////////////////////////////////////////////////////

// Hash an arbitrary sequence of chars - it uses the Fowler-Noll-Vo 1a hash
// function.
//
// Note: This hashes the incoming chars exactly, and assumes that Unicode
// encoding is being used.
//
class __MCHashCharContext
{
private:
#ifdef __LARGE__
    // 64-bit variant
    const uint64_t kPrime = 1099511628211ULL;
    const uint64_t kOffset = 14695981039346656037ULL;
#else
    // 32-bit variant
    const uint32_t kPrime = 16777619UL;
    const uint32_t kOffset = 2166136261UL;
#endif
    
    // Perform a single step for a unicode code unit representable as a single
    // byte.
    void StepHalf(uint8_t p_char)
    {
        // Hash the byte
        m_hash ^= p_char;
        m_hash *= kPrime;
        m_hash *= kPrime;
    }
    
    // Perform a single step for a unicode code unit.
    void StepFull(uint16_t p_char)
    {
        // Hash the byte
        m_hash ^= p_char & 0xFF;
        m_hash *= kPrime;
        
        // Hash the second byte of the unichar
        m_hash ^= p_char >> 8;
        m_hash *= kPrime;
    }
    
public:
    // Default constructor for hashing a sequence of bytes from the start.
    __MCHashCharContext(void)
        : m_hash(kOffset)
    {
    }
    
    // Constructor for continuing a hash of a sequence of bytes.
    __MCHashCharContext(hash_t p_initial_hash)
        : m_hash(p_initial_hash)
    {
    }
    
    // Reset hashing to an initial value.
    void Reset(hash_t p_initial_hash = 0)
    {
        m_hash = p_initial_hash;
    }

    // Hash a single code unit which fits in a byte
    void ConsumeHalfChar(char_t p_char)
    {
        StepHalf(reinterpret_cast<uint8_t>(p_char));
    }
    
    // Hash a single code unit
    void ConsumeChar(unichar_t p_char)
    {
        StepFull(reinterpret_cast<uint16_t>(p_char));
    }
    
    // Hash a single codepoint
    void ConsumeCodepoint(codepoint_t p_codepoint)
    {
        unichar_t t_lead, t_trail;
        if (!MCUnicodeSplitIntoSurrogates(p_codepoint,
                                          t_lead,
                                          t_trail))
        {
            StepFull(reinterpret_cast<uint16_t>(t_lead));
        }
        else
        {
            StepFull(reinterpret_cast<uint16_t>(t_lead));
            StepFull(reinterpret_cast<uint16_t>(t_trail));
        }
    }
    
    // Return the hash of all bytes processed so far.
    hash_t Current(void) const
    {
        return m_hash;
    }
    
private:
    hash_t m_hash;
};

////////////////////////////////////////////////////////////////////////////////

#endif
