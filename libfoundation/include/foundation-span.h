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
	MCSpan()
		: m_data(nullptr), m_length(0) {}
	MCSpan(decltype(nullptr))
		: m_data(nullptr), m_length(0) {}
	MCSpan(ElementPtr p_ptr, IndexType p_count)
		: m_data(p_ptr), m_length(p_count) {}

	template <size_t N>
	MCSpan(ElementType (&arr)[N])
		: m_data(&arr[0]), m_length(N) {}

	/* TODO[C++11] MCSpan(MCSpan &other) = default; */
	MCSpan(MCSpan& other)
		: m_data(other.m_data), m_length(other.m_length) {}

	/* TODO[C++11] MCSpan(MCSpan &&other) = default; */
	MCSpan(MCSpan&& other)
		: m_data(static_cast<ElementPtr &&>(other.m_data)),
		  m_length(static_cast<IndexType &&>(other.m_length)) {}

	/* TODO[C++11] ~MCSpan() = default; */
	~MCSpan() {}

	/* ---------- Assignment ops */
	/* TODO[C++11] MCSpan& operator=(const MCSpan& other) = default; */
	MCSpan& operator=(const MCSpan& other) {
		m_data = other.m_data;
		m_length = other.m_length;
		return *this;
	}

	/* TODO[C++11] MCSpan& operator=(MCSpan&& other) = default; */
	MCSpan& operator=(MCSpan&& other) {
		m_data = static_cast<ElementPtr &&>(other.m_data);
		m_length = static_cast<IndexType &&>(other.m_length);
		return *this;
	}

	/* ---------- Subspans */
	MCSpan first(IndexType p_count) const
	{
		MCAssert(p_count >= 0 && p_count <= size());
		return MCSpan(data(), p_count);
	}

	MCSpan last(IndexType p_count) const
	{
		MCAssert(p_count >= 0 && p_count <= size());
		return MCSpan(data() + (size() - p_count), p_count);
	}

	MCSpan subspan(IndexType p_offset,
	               IndexType p_count = kMCSpanDynamicExtent) const
	{
		MCAssert(p_offset == 0 || (p_offset > 0 && p_offset <= size()));
		MCAssert(p_count == kMCSpanDynamicExtent ||
		         (p_count >= 0 && p_offset + p_count <= size()));
		return MCSpan(data() + p_offset,
		              p_count == kMCSpanDynamicExtent ? size() - p_offset : p_count);
	}

	MCSpan operator+(IndexType p_offset) const
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
	MCSpan& operator++(int)
	{
		auto t_ret = *this;
		++(*this);
		return t_ret;
	}

	/* ---------- Observers */
	IndexType length() const { return size(); }
	IndexType size() const { return m_length; }
	IndexType lengthBytes() const { return sizeBytes(); }
	IndexType sizeBytes() const { return m_length * sizeof(ElementType); }
	bool empty() const { return size() == 0; }

	/* ---------- Element access */
	ElementRef operator[](IndexType p_index) const
	{
		MCAssert(p_index >= 0 && p_index < size());
		return data()[p_index];
	}
	ElementRef operator*() const
	{
		return (*this)[0];
	}
	ElementPtr operator->() const
	{
		return &((*this)[0]);
	}

	ElementPtr data() const { return m_data; }

protected:

	MCSpan& advance(IndexType p_offset)
	{
		MCAssert(p_offset == 0 || (p_offset > 0 && p_offset <= size()));
		m_data += p_offset;
		m_length -= p_offset;
		return *this;
	}

	ElementPtr m_data;
	IndexType m_length;
};

template <typename ElementType>
MCSpan<ElementType> operator+(typename MCSpan<ElementType>::IndexType n,
                              const MCSpan<ElementType>& rhs)
{
	return rhs + n;
}

template <typename ElementType>
MCSpan<ElementType> MCMakeSpan(ElementType *p_ptr,
                               typename MCSpan<ElementType>::IndexType p_count)
{
	return MCSpan<ElementType>(p_ptr, p_count);
}

/* TODO[C++11] The default value for ElementType should be byte_t */
template <typename ElementType>
MCSpan<ElementType> MCDataGetSpan(MCDataRef p_data)
{
	MCAssert(MCDataGetLength(p_data) % sizeof(ElementType) == 0);
	auto t_ptr = reinterpret_cast<ElementType*>(MCDataGetBytePtr(p_data));
	size_t t_length = MCDataGetLength(p_data) / sizeof(ElementType);
	return MCMakeSpan(t_ptr, t_length);
}

#endif /* !__MC_FOUNDATION_SPAN_H__ */
