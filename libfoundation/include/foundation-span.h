/*                                                                     -*-C++-*-
 * Copyright (C) 2016 LiveCode Ltd.
 *
 * This file is part of LiveCode.
 *
 * LiveCode is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v3 as published
 * by the Free Software Foundation.
 *
 * LiveCode is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LiveCode.  If not see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __MC_FOUNDATION_SPAN_H__
#define __MC_FOUNDATION_SPAN_H__

/* An MCSpan<T> represents a "view" onto an array of values of type T.
 * It is intended to be used anywhere where a pointer & length would
 * otherwise be stored on the stack, either in function parameters or
 * in local variables.  Note that an MCSpan<T> does _not_ own the
 * buffer to which it points; you should use an `MCAutoArray` or
 * similar when you require an owned array.
 *
 * MCSpan<T> behaves like a pointer + length in many respects, but the
 * key differences are that:
 *
 * - All accesses are checked
 *
 * - Its view onto the underlying array can only be narrowed, not
 *   widened
 *
 * MCSpan is heavily inspired by gsl::span from
 * https://github.com/Microsoft/GSL
 */

/* In the C++ standard, operator[] for pointers is defined to take an
 * argument of std::ptrdiff_t.  This is used throughout this class in
 * order to make sure that there is as much commonality as possible
 * between the behaviour of an MCSpan and an array pointer. */

/* TODO[C++14] Some of the constexpr methods in MCSpan use assertions,
 * and have to do ugly chaining using the comma operator in a return
 * statement in order to comply with the C++11 restrictions on
 * constexpr functions.  When LiveCode is built with C++14, these
 * methods should be refactored. */

#include "foundation.h"
#include <cstddef>

static const std::ptrdiff_t kMCSpanDynamicExtent = -1;

template <typename ElementType>
class MCSpan
{
public:
	typedef std::ptrdiff_t IndexType;
	typedef ElementType* ElementPtr;
	typedef ElementType& ElementRef;
	typedef ElementType&& ElementRRef;

	/* ---------- Constructors */
	constexpr MCSpan() = default;
	constexpr MCSpan(decltype(nullptr)) {}
	constexpr MCSpan(ElementPtr p_ptr, IndexType p_count)
		: m_data(p_ptr), m_length(p_count) {}

	template <size_t N>
	constexpr MCSpan(ElementType (&arr)[N])
		: m_data(&arr[0]), m_length(N) {}

	constexpr MCSpan(const MCSpan& other) = default;

	constexpr MCSpan(MCSpan&& other) = default;

	~MCSpan() = default;

	/* ---------- Assignment ops */
	MCSpan& operator=(const MCSpan& other) = default;

	MCSpan& operator=(MCSpan&& other) = default;

	/* ---------- Subspans */
	constexpr MCSpan first(IndexType p_count) const
	{
        return MCAssert(p_count >= 0 && p_count <= size()),
            MCSpan(data(), p_count);
	}

	constexpr MCSpan last(IndexType p_count) const
	{
        return MCAssert(p_count >= 0 && p_count <= size()),
            MCSpan(data() + (size() - p_count), p_count);
	}

	constexpr MCSpan subspan(IndexType p_offset,
	                               IndexType p_count = kMCSpanDynamicExtent) const
	{
        return
            MCAssert(p_offset == 0 || (p_offset > 0 && p_offset <= size())),
            MCAssert(p_count == kMCSpanDynamicExtent ||
                     (p_count >= 0 && p_offset + p_count <= size())),
            MCSpan(data() + p_offset,
                   p_count == kMCSpanDynamicExtent ? size() - p_offset : p_count);
	}

	constexpr MCSpan operator+(IndexType p_offset) const
	{
		return subspan(p_offset);
	}

	MCSpan& operator+=(IndexType p_offset)
	{
		return advance(p_offset);
	}

	/* ---------- Pointer arithmetic */
	MCSpan& operator++()
	{
		return advance(1);
	}
	MCSpan operator++(int)
	{
		auto t_ret = *this;
		++(*this);
		return t_ret;
	}

	/* ---------- Observers */
	constexpr IndexType length() const { return size(); }
	constexpr IndexType size() const { return m_length; }
	constexpr IndexType lengthBytes() const { return sizeBytes(); }
	constexpr IndexType sizeBytes() const
	{
		return m_length * sizeof(ElementType);
	}
	constexpr bool empty() const { return size() == 0; }

	/* ---------- Element access */
	constexpr ElementRef operator[](IndexType p_index) const
	{
        return MCAssert(p_index >= 0 && p_index < size()),
            data()[p_index];
	}
	constexpr ElementRef operator*() const
	{
		return (*this)[0];
	}
	constexpr ElementPtr operator->() const
	{
		return &((*this)[0]);
	}

	constexpr ElementPtr data() const { return m_data; }

protected:

	MCSpan& advance(IndexType p_offset)
	{
		MCAssert(p_offset == 0 || (p_offset > 0 && p_offset <= size()));
		m_data += p_offset;
		m_length -= p_offset;
		return *this;
	}

	ElementPtr m_data = nullptr;
	IndexType m_length = 0;
};

template <typename ElementType>
constexpr MCSpan<ElementType>
operator+(typename MCSpan<ElementType>::IndexType n,
          const MCSpan<ElementType>& rhs)
{
	return rhs + n;
}

/* TODO[C++17] Remove when we have class and struct template type inference */
template<typename ElementType, size_t N>
constexpr MCSpan<ElementType>
MCMakeSpan(ElementType (&arr)[N])
{
    return MCSpan<ElementType>(arr, N);
}

template <typename ElementType>
constexpr MCSpan<ElementType>
MCMakeSpan(ElementType *p_ptr,
           typename MCSpan<ElementType>::IndexType p_count)
{
	return MCSpan<ElementType>(p_ptr, p_count);
}

template <typename ElementType = byte_t>
inline MCSpan<ElementType> MCDataGetSpan(MCDataRef p_data)
{
	MCAssert(MCDataGetLength(p_data) % sizeof(ElementType) == 0);
	auto t_ptr = reinterpret_cast<ElementType*>(MCDataGetBytePtr(p_data));
	size_t t_length = MCDataGetLength(p_data) / sizeof(ElementType);
	return MCMakeSpan(t_ptr, t_length);
}

/* ----------------------------------------------------------------
 * Span-based overloads for pointer+range libfoundation functions
 * ---------------------------------------------------------------- */

MC_DLLEXPORT
hash_t MCHashBytes(MCSpan<const byte_t> bytes);

MC_DLLEXPORT
hash_t MCHashBytesStream(hash_t previous,
                         MCSpan<const byte_t> bytes);

MC_DLLEXPORT
hash_t MCHashNativeChars(MCSpan<const char_t> chars);

MC_DLLEXPORT
hash_t MCHashChars(MCSpan<const unichar_t> chars);

template <typename ElementType>
inline hash_t MCHashSpan(MCSpan<ElementType> p_span)
{
    /* TODO[C++11] Enable this assertion once all our C++ compilers support it. */
    /* static_assert(std::is_trivially_copyable<ElementType>::value,
           "MCHashObjectSpan can only be used with trivially copyable types"); */
    return MCHashBytes(p_span.data(), p_span.sizeBytes());
}

template <typename ElementType>
inline hash_t MCHashSpanStream(hash_t p_previous,
                               MCSpan<ElementType> p_span)
{
    /* TODO[C++11] Enable this assertion once all our C++ compilers support it. */
    /* static_assert(std::is_trivially_copyable<ElementType>::value,
           "MCHashObjectSpanStream can only be used with trivially copyable types"); */
    return MCHashBytesStream(p_previous,
                             p_span.data(), p_span.sizeBytes());
}

template <typename ElementType>
inline bool MCProperListCreateWithForeignValues(MCTypeInfoRef p_typeinfo, const MCSpan<ElementType> p_values, MCProperListRef& r_list)
{
    return MCProperListCreateWithForeignValues(p_typeinfo,
                                               p_values.data(),
                                               p_values.length(),
                                               r_list);
}

#endif /* !__MC_FOUNDATION_SPAN_H__ */
