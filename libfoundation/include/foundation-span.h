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
#include <iterator>
#include <type_traits>

static const std::ptrdiff_t kMCSpanDynamicExtent = -1;

namespace MC
{
namespace Details
{
template <typename Span, bool IsConst>
class MCSpanIterator
{
    typedef typename Span::element_type element_type_;

public:
    /* These type members are required in order to allow
     * std::iterator_traits to work with this class.  In C++17, this
     * is a requirement for conformance with the Iterator concept. */
    typedef typename std::random_access_iterator_tag iterator_category;
    typedef typename Span::index_type difference_type;
    typedef typename std::remove_const<element_type_>::type value_type;
    typedef typename std::conditional<IsConst,
                                      const element_type_,
                                      element_type_>::type & reference;
    typedef typename std::add_pointer<reference>::type pointer;

    /* The const version of this MCSpanIterator type should be a
     * friend so that its fields are accessible.  N.b. the converse is
     * _not_ true. */
    friend class MCSpanIterator<Span, true>;

    /* ---------- Constructors */
    constexpr MCSpanIterator() : m_span(nullptr), m_index(0) {}

    constexpr MCSpanIterator(const Span* p_span,
                             typename Span::index_type p_index)
        : m_span(p_span),
          m_index(p_index)
    {
        /* TODO[C++14] Some compilers don't allow statements in
         * constexpr constructors yet */
#if NEEDS_CPP_14
        MCAssert(p_span == nullptr ||
                 (p_index >= 0 && p_index < p_span->length())),
#endif
    }

    /* Allow the creation of an iterator over a const span from an
     * iterator over a non-const span. */
    constexpr MCSpanIterator(const MCSpanIterator<Span, false>& other)
        : m_span(other.m_span), m_index(other.m_index) {}

    /* ---------- Assignment ops */
    /* TODO[C++11] MCSpanIterator& operator=(const MCSpanIterator& other) = default; */
    MCSpanIterator& operator=(const MCSpanIterator& other)
    {
        m_span = other.m_span;
        m_index = other.m_index;
        return *this;
    }

    /* ---------- Element access */
    constexpr reference operator*() const
    {
        return MCAssert(m_span != nullptr), (*m_span)[m_index];
    }

    constexpr pointer operator->() const
    {
        return MCAssert(m_span != nullptr), &((*m_span)[m_index]);
    }

    /* ---------- Traversal ops */
    /* TODO[C++14] Make these operators constexpr */
    MCSpanIterator& operator++()
    {
        return
            MCAssert(m_span != nullptr &&
                     m_index >= 0 && m_index < m_span->length()),
            ++m_index,
            *this;
    }

    MCSpanIterator operator++(int)
    {
        auto t_iter = *this;
        ++(*this);
        return t_iter;
    }

    MCSpanIterator& operator--()
    {
        return
            MCAssert(m_span != nullptr &&
                     m_index > 0 && m_index <= m_span->length()),
            --m_index,
            *this;
    }

    MCSpanIterator operator--(int)
    {
        auto t_iter = *this;
        --(*this);
        return t_iter;
    }

    MCSpanIterator operator+(difference_type p_offset) const
    {
        auto t_iter = *this;
        return t_iter += p_offset;
    }

    MCSpanIterator& operator+=(difference_type p_offset)
    {
        return
            MCAssert(m_span != nullptr &&
                     (m_index + p_offset) >= 0 &&
                     (m_index + p_offset) < m_span->length()),
            m_index += p_offset,
            *this;
    }

    MCSpanIterator operator-(difference_type p_offset) const
    {
        auto t_iter = *this;
        return t_iter -= p_offset;
    }

    MCSpanIterator& operator-=(difference_type p_offset)
    {
        return *this += -p_offset;
    }

    /* ---------- Iterator comparisons */

    /* Measure the distance between two iterators over the same
     * span */
    constexpr difference_type operator-(const MCSpanIterator& p_rhs) const
    {
        return MCAssert(m_span == p_rhs.m_span), m_index - p_rhs.m_index;
    }

    constexpr friend bool operator==(const MCSpanIterator& p_lhs,
                                     const MCSpanIterator& p_rhs)
    {
        return p_lhs.m_span == p_rhs.m_span && p_lhs.m_index == p_rhs.m_index;
    }

    constexpr friend bool operator!=(const MCSpanIterator& p_lhs,
                                     const MCSpanIterator& p_rhs)
    {
        return !(p_lhs == p_rhs);
    }

    constexpr friend bool operator<(const MCSpanIterator& p_lhs,
                                    const MCSpanIterator& p_rhs)
    {
        return MCAssert(p_lhs.m_span == p_rhs.m_span),
            p_lhs.m_index < p_rhs.m_index;
    }

    constexpr friend bool operator <=(const MCSpanIterator& p_lhs,
                                      const MCSpanIterator& p_rhs)
    {
        return !(p_rhs < p_lhs);
    }

    constexpr friend bool operator >(const MCSpanIterator& p_lhs,
                                     const MCSpanIterator& p_rhs)
    {
        return p_rhs < p_lhs;
    }

    constexpr friend bool operator >=(const MCSpanIterator& p_lhs,
                                      const MCSpanIterator& p_rhs)
    {
        return !(p_rhs > p_lhs);
    }

    void swap(MCSpanIterator& p_other)
    {
        std::swap(m_index, p_other.m_index);
        std::swap(m_span, p_other.m_span);
    }

protected:
    const Span* m_span;
    difference_type m_index;
};

template <typename Span, bool IsConst>
constexpr MCSpanIterator<Span, IsConst>
operator+(typename MCSpanIterator<Span, IsConst>::difference_type n,
          const MCSpanIterator<Span, IsConst>& p_rhs)
{
    return p_rhs + n;
}

template <typename Span, bool IsConst>
constexpr MCSpanIterator<Span, IsConst>
operator-(typename MCSpanIterator<Span, IsConst>::difference_type n,
          const MCSpanIterator<Span, IsConst>& p_rhs)
{
    return p_rhs - n;
}

} /* namespace Details */
} /* namespace MC */

template <typename ElementType>
class MCSpan
{
public:
    typedef ElementType element_type;
	typedef std::ptrdiff_t index_type;
	typedef element_type* pointer;
	typedef element_type& reference;

    typedef MC::Details::MCSpanIterator<MCSpan<element_type>, false> iterator;
    typedef MC::Details::MCSpanIterator<MCSpan<element_type>, true> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	/* ---------- Constructors */
	constexpr MCSpan() = default;
	constexpr MCSpan(decltype(nullptr)) {}
	constexpr MCSpan(pointer p_ptr, index_type p_count)
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
	constexpr MCSpan first(index_type p_count) const
	{
        return MCAssert(p_count >= 0 && p_count <= size()),
            MCSpan(data(), p_count);
	}

	constexpr MCSpan last(index_type p_count) const
	{
        return MCAssert(p_count >= 0 && p_count <= size()),
            MCSpan(data() + (size() - p_count), p_count);
	}

	constexpr MCSpan subspan(index_type p_offset,
	                               index_type p_count = kMCSpanDynamicExtent) const
	{
        return
            MCAssert(p_offset == 0 || (p_offset > 0 && p_offset <= size())),
            MCAssert(p_count == kMCSpanDynamicExtent ||
                     (p_count >= 0 && p_offset + p_count <= size())),
            MCSpan(data() + p_offset,
                   p_count == kMCSpanDynamicExtent ? size() - p_offset : p_count);
	}

	/* ---------- Observers */
	constexpr index_type length() const { return size(); }
	constexpr index_type size() const { return m_length; }
	constexpr index_type lengthBytes() const { return sizeBytes(); }
	constexpr index_type sizeBytes() const
	{
		return m_length * sizeof(ElementType);
	}
	constexpr bool empty() const { return size() == 0; }

	/* ---------- Element access */
	constexpr reference operator[](index_type p_index) const
	{
        return MCAssert(p_index >= 0 && p_index < size()),
            data()[p_index];
	}
	constexpr reference operator*() const
	{
		return (*this)[0];
	}
	constexpr pointer operator->() const
	{
		return &((*this)[0]);
	}

	constexpr pointer data() const { return m_data; }

    /* ---------- Iterator support */
    iterator begin() const { return iterator(this, 0); }
    iterator end() const { return iterator(this, length()); }

    const_iterator cbegin() const { return const_iterator(this, 0); }
    const_iterator cend() const { return const_iterator(this, length()); }

    reverse_iterator rbegin() const { return reverse_iterator(end()); }
    reverse_iterator rend() const { return reverse_iterator(begin()); }

    const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }

protected:

	MCSpan& advance(index_type p_offset)
	{
		MCAssert(p_offset == 0 || (p_offset > 0 && p_offset <= size()));
		m_data += p_offset;
		m_length -= p_offset;
		return *this;
	}

	pointer m_data = nullptr;
	index_type m_length = 0;
};

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
           typename MCSpan<ElementType>::index_type p_count)
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

template <typename ElementType>
inline void MCMemoryClearSecure(MCSpan<ElementType> x_span)
{
    MCMemoryClearSecure(reinterpret_cast<byte_t*>(x_span.data()),
                        x_span.sizeBytes());
}

MC_DLLEXPORT
bool MCArrayStoreValueOnPath(MCArrayRef array,
                             bool case_sensitive,
                             MCSpan<MCNameRef> path,
                             MCValueRef value);

MC_DLLEXPORT
bool MCArrayFetchValueOnPath(MCArrayRef array,
                             bool case_sensitive,
                             MCSpan<MCNameRef> path,
                             MCValueRef& r_value);

MC_DLLEXPORT
bool MCArrayRemoveValueOnPath(MCArrayRef array,
                              bool case_sensitive,
                              MCSpan<MCNameRef> path);

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
