/*                                                                     -*-C++-*-
Copyright (C) 2003-2016 LiveCode Ltd.

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

#ifndef __MC_FOUNDATION_AUTO__
#define __MC_FOUNDATION_AUTO__

#ifndef __MC_FOUNDATION__
#include "foundation.h"
#endif

#include "foundation-span.h"

////////////////////////////////////////////////////////////////////////////////

template<typename T> class MCAutoValueRefBase;

template<typename T>
T In(const MCAutoValueRefBase<T>& p_auto)
{
    return p_auto . In();
}

template<typename T>
T& Out(MCAutoValueRefBase<T>& p_auto)
{
    return p_auto . Out();
}

template<typename T>
T& InOut(MCAutoValueRefBase<T>& p_auto)
{
    return p_auto . InOut();
}

template<typename T> class MCAutoValueRefBase
{
public:

    constexpr MCAutoValueRefBase(void) = default;
    
    inline MCAutoValueRefBase(const MCAutoValueRefBase& other) :
      m_value(nil)
    {
        Reset(other.m_value);
    }
    
    MCAutoValueRefBase(MCAutoValueRefBase&& other)
        : m_value(other.Take()) {}

    inline MCAutoValueRefBase(T p_value)
		: m_value(nil)
	{
        if (p_value)
            m_value = MCValueRetain(p_value);
	}

	inline ~MCAutoValueRefBase(void)
	{
		MCValueRelease(m_value);
	}

	inline T operator = (T value)
	{
		MCAssert(m_value == nil);
		m_value = (T)MCValueRetain(value);
		return value;
	}

    MCAutoValueRefBase& operator=(MCAutoValueRefBase&& other)
    {
        Give(other.Take());
        return *this;
    }

	inline T& operator & (void)
	{
		MCAssert(m_value == nil);
		return m_value;
	}

	inline T operator * (void) const
	{
		return m_value;
	}

    inline T operator -> (void) const
    {
        MCAssert(m_value != nullptr);
        return m_value;
    }
    
	bool IsSet() const
	{
		return m_value != nil;
	}

    void Reset(T value = nil)
    {
        if (value == m_value)
                return;
        
        if (m_value)
            MCValueRelease(m_value);
        m_value = (value) ? (T)MCValueRetain(value) : NULL;
    }
    
    // The give method places the given value into the container without
    // retaining it - the auto container is considered to now own the value.
    inline void Give(T value)
    {
        if (m_value)
            MCValueRelease(m_value);
        m_value = value;
    }
    
    // The take method removes the value from the container passing ownership
    // to the caller.
    inline T Take(void)
    {
        T t_value;
        t_value = m_value;
        m_value = nil;
        return t_value;
    }
    
    // The MakeUnique method inters the value-ref so that it uses an existing
    // uniqued value if present.
    inline bool MakeUnique(void)
    {
        if (!MCValueInterAndRelease(m_value, m_value))
        {
            return false;
        }
        return true;
    }
    
    friend T In<>(const MCAutoValueRefBase<T>&);
    friend T& Out<>(MCAutoValueRefBase<T>&);
    friend T& InOut<>(MCAutoValueRefBase<T>&);
    
protected:
	T m_value = nullptr;
    
    // Return the contents of the auto pointer in a form for an in parameter.
    T In(void) const
    {
        return m_value;
    }
    
    // Return the contents of the auto pointer in a form for an out parameter.
    T& Out(void)
    {
		MCAssert(m_value == nil);
		return m_value;
    }
    
    // Return the contents of the auto pointer in a form for an inout parameter.
    T& InOut(void)
    {
        return m_value;
    }

private:
    MCAutoValueRefBase<T>& operator = (MCAutoValueRefBase<T>& x);
};

template<typename T, bool (*MutableCopyAndRelease)(T, T&), bool (*ImmutableCopyAndRelease)(T, T&)>
class MCAutoMutableValueRefBase :
  public MCAutoValueRefBase<T>
{
public:
    constexpr MCAutoMutableValueRefBase() :
      MCAutoValueRefBase<T>()
    {
    }
    
    inline explicit MCAutoMutableValueRefBase(T p_value) :
      MCAutoValueRefBase<T>(p_value)
    {
    }
    
    inline T operator = (T value)
	{
        return MCAutoValueRefBase<T>::operator =(value);
	}
    
    inline bool MakeMutable(void)
    {
        return MutableCopyAndRelease(MCAutoValueRefBase<T>::m_value, MCAutoValueRefBase<T>::m_value);
    }
    
    inline bool MakeImmutable(void)
    {
        return ImmutableCopyAndRelease(MCAutoValueRefBase<T>::m_value, MCAutoValueRefBase<T>::m_value);
    }
    
private:
    MCAutoMutableValueRefBase<T, MutableCopyAndRelease, ImmutableCopyAndRelease>& operator = (MCAutoMutableValueRefBase<T, MutableCopyAndRelease, ImmutableCopyAndRelease>& x);
};

typedef MCAutoValueRefBase<MCValueRef> MCAutoValueRef;
typedef MCAutoValueRefBase<MCNumberRef> MCAutoNumberRef;
typedef MCAutoMutableValueRefBase<MCStringRef, MCStringMutableCopyAndRelease, MCStringCopyAndRelease> MCAutoStringRef;
typedef MCAutoMutableValueRefBase<MCArrayRef, MCArrayMutableCopyAndRelease, MCArrayCopyAndRelease> MCAutoArrayRef;
typedef MCAutoValueRefBase<MCListRef> MCAutoListRef;
typedef MCAutoValueRefBase<MCBooleanRef> MCAutoBooleanRef;
typedef MCAutoMutableValueRefBase<MCSetRef, MCSetMutableCopyAndRelease, MCSetCopyAndRelease> MCAutoSetRef;
typedef MCAutoValueRefBase<MCNameRef> MCNewAutoNameRef;
typedef MCAutoMutableValueRefBase<MCDataRef, MCDataMutableCopyAndRelease, MCDataCopyAndRelease> MCAutoDataRef;
typedef MCAutoValueRefBase<MCTypeInfoRef> MCAutoTypeInfoRef;
typedef MCAutoMutableValueRefBase<MCRecordRef, MCRecordMutableCopyAndRelease, MCRecordCopyAndRelease> MCAutoRecordRef;
typedef MCAutoValueRefBase<MCErrorRef> MCAutoErrorRef;
typedef MCAutoMutableValueRefBase<MCProperListRef, MCProperListMutableCopyAndRelease, MCProperListCopyAndRelease> MCAutoProperListRef;
typedef MCAutoValueRefBase<MCJavaObjectRef> MCAutoJavaObjectRef;

////////////////////////////////////////////////////////////////////////////////

template<typename T> class MCAutoValueRefArrayBase
{
public:
	constexpr MCAutoValueRefArrayBase() = default;
    
	~MCAutoValueRefArrayBase(void)
	{
		Reset();
	}
    
	//////////
    
	bool New(uindex_t p_size)
	{
		MCAssert(m_values == nil);
		return MCMemoryNewArray(p_size, m_values, m_value_count);
	}

	//////////

	void Give(T* p_array, uindex_t p_size)
	{
		MCAssert(m_values == nil);
		m_values = p_array;
		m_value_count = p_size;
	}

	void Take(T*& r_array, uindex_t& r_count)
	{
		r_array = m_values;
		r_count = m_value_count;

		m_values = nil;
		m_value_count = 0;
	}

	/* Take the contents of the array as an immutable MCProperList.
	 * The contents of the array will no longer be accessible. */
	bool TakeAsProperList (MCProperListRef& r_list)
	{
		MCProperListRef t_list;
		if (!MCProperListCreateAndRelease ((MCValueRef *) m_values,
		                                   m_value_count,
		                                   t_list))
			return false;

		m_values = nil;
		m_value_count = 0;

		r_list = t_list;
		return true;
	}

	/* Reset the managed array by releasing all the values in the
	 * array and the underlying array storage. */
	void Reset()
	{
        if (m_values != nil)
        {
            for (uindex_t i = 0; i < m_value_count; i++)
                MCValueRelease(m_values[i]);
            MCMemoryDeleteArray(m_values);
        }
	}

	//////////

    T* Ptr()
    {
        return m_values;
    }
    
    uindex_t Size()
    {
        return m_value_count;
    }
    
	T*& PtrRef()
	{
		MCAssert(m_values == nil);
		return m_values;
	}

	uindex_t& CountRef()
	{
		MCAssert(m_value_count == 0);
		return m_value_count;
	}

	//////////

	bool Resize(uindex_t p_new_size)
	{
		return MCMemoryResizeArray(p_new_size, m_values, m_value_count);
	}

	bool Extend(uindex_t p_new_size)
	{
		MCAssert(p_new_size >= m_value_count);
		return Resize(p_new_size);
	}

	void Shrink(uindex_t p_new_size)
	{
		MCAssert(p_new_size <= m_value_count);
		Resize(p_new_size);
	}

	//////////

	bool Append(MCAutoValueRefArrayBase<T> &p_array)
	{
		uindex_t t_index = Count();
		if (!Extend(t_index + p_array.Count()))
			return false;

		for (uindex_t i = 0; i < p_array.m_value_count; i++)
			m_values[t_index + i] = MCValueRetain(p_array.m_values[i]);

		return true;
	}
    
    bool Push(T p_value)
    {
        if (!Extend(m_value_count + 1))
            return false;
        m_values[m_value_count - 1] = MCValueRetain(p_value);
        return true;
    }

	//////////

	uindex_t Count(void)
	{
		return m_value_count;
	}

	//////////

	T* operator * (void) const
	{
		return m_values;
	}
    
    T& operator [] (const uindex_t p_index)
    {
        MCAssert(m_values != nil);
        return m_values[p_index];
    }

	MCSpan<T> Span() const
	{
		return MCMakeSpan(m_values, m_value_count);
	}
    
private:
	T* m_values = nullptr;
    uindex_t m_value_count = 0;
};

typedef MCAutoValueRefArrayBase<MCValueRef> MCAutoValueRefArray;
typedef MCAutoValueRefArrayBase<MCNumberRef> MCAutoNumberRefArray;
typedef MCAutoValueRefArrayBase<MCStringRef> MCAutoStringRefArray;
typedef MCAutoValueRefArrayBase<MCDataRef> MCAutoDataRefArray;
typedef MCAutoValueRefArrayBase<MCArrayRef> MCAutoArrayRefArray;
typedef MCAutoValueRefArrayBase<MCListRef> MCAutoListRefArray;
typedef MCAutoValueRefArrayBase<MCBooleanRef> MCAutoBooleanRefArray;
typedef MCAutoValueRefArrayBase<MCSetRef> MCAutoSetRefArray;
typedef MCAutoValueRefArrayBase<MCNameRef> MCAutoNameRefArray;

////////////////////////////////////////////////////////////////////////////////

class MCAutoStringRefAsUTF16String
{
public:
    constexpr MCAutoStringRefAsUTF16String() = default;

	bool Lock(MCStringRef p_string)
	{
		return MCStringUnicodeCopy(p_string, &m_string);
	}
	void Unlock()
	{
		m_string.Reset();
	}
	const unichar_t *Ptr() const
	{
		MCAssert(!MCStringIsNative(*m_string));
		return MCStringGetCharPtr(*m_string);
	}
	uindex_t Size() const
	{
		MCAssert(!MCStringIsNative(*m_string));
		return MCStringGetLength(*m_string);
	}
	const unichar_t *operator*() const
	{
		return Ptr();
	}
private:
	MCAutoStringRef m_string;
};

typedef MCAutoStringRefAsUTF16String MCAutoStringRefAsWString;

////////////////////////////////////////////////////////////////////////////////

#if defined(__WINDOWS__)
typedef MCAutoStringRefAsUTF16String MCAutoStringRefAsLPCWSTR;

/* Always ensures that its internal string buffer is always owned by
 * the auto class, so that it's safe to pass to Windows API functions
 * that take an LPWSTR rather than an LPCWSTR. */
class MCAutoStringRefAsLPWSTR
{
public:
	constexpr MCAutoStringRefAsLPWSTR() = default;

	~MCAutoStringRefAsLPWSTR()
	{
		Unlock();
	}
	bool Lock(MCStringRef p_string)
	{
		MCAssert(nil == m_buffer);
		m_size = MCStringGetLength(p_string);
		return MCStringConvertToWString(p_string, m_buffer);
	}
	void Unlock()
	{
		MCMemoryDeleteArray(m_buffer);
		m_buffer = nil;
	}
	unichar_t *Ptr()
	{
		MCAssert(nil != m_buffer);
		return m_buffer;
	}
	uindex_t Size() const
	{
		MCAssert(nil != m_buffer);
		return m_size;
	}
	unichar_t *operator*()
	{
		return Ptr();
	}
private:
	unichar_t *m_buffer = nullptr;
	uindex_t m_size = 0;
};
#endif /* __WINDOWS__ */

////////////////////////////////////////////////////////////////////////////////

class MCAutoStringRefAsUTF8String
{
public:
	constexpr MCAutoStringRefAsUTF8String() = default;
    
    ~MCAutoStringRefAsUTF8String(void)
    {
        Unlock();
    }
    
    bool Lock(MCStringRef p_string)
    {
        return MCStringConvertToUTF8(p_string, m_utf8string, m_size);
    }
    
    void Unlock(void)
    {
        MCMemoryDeleteArray(m_utf8string);
        m_utf8string = nil;
    }
    
    const char *operator * (void) const
    {
        return m_utf8string;
    }
    
    uindex_t Size()
    {
        return m_size;
    }
    
private:
	char *m_utf8string = nullptr;
    uindex_t m_size = 0;
};

////////////////////////////////////////////////////////////////////////////////

#if defined(__MAC__) || defined(__IOS__)
#include <CoreFoundation/CoreFoundation.h>
class MCAutoStringRefAsCFString
{
public:
    constexpr MCAutoStringRefAsCFString(void) = default;
    
    ~MCAutoStringRefAsCFString(void)
    {
        Unlock();
    }
    
    bool Lock(MCStringRef p_string)
    {
        return MCStringConvertToCFStringRef(p_string, m_cfstring);
    }
    
    void Unlock(void)
    {
        if (m_cfstring != nil)
            CFRelease(m_cfstring);
        m_cfstring = nil;
    }
    
    CFStringRef operator * (void)
    {
        return m_cfstring;
    }
    
private:
    CFStringRef m_cfstring = nullptr;
};

#endif

////////////////////////////////////////////////////////////////////////////////

class MCAutoStringRefAsPascalString
{
public:
    constexpr MCAutoStringRefAsPascalString() = default;
    
    ~MCAutoStringRefAsPascalString(void)
    {
        Unlock();
    }
    
    bool Lock(MCStringRef p_string)
    {
        // A Pascal-style counted string can only contain a maximum of 255 chars
        // so the string length has to be clamped to that.
        uindex_t t_length = MCStringGetLength(p_string);
        if (t_length > 255)
            return false;
        
        // Allocate a buffer, adding an extra char for the count byte
        MCMemoryNewArray(t_length + 1, m_pascal_string);
        
        // Copy the string to the buffer, leaving a byte at the beginning for
        // the count.
        MCStringGetNativeChars(p_string, MCRangeMake(0, t_length), m_pascal_string+1);
        
        // Write the count byte
        *m_pascal_string = uint8_t(t_length);
        return true;
    }
    
    void Unlock(void)
    {
        MCMemoryDeleteArray(m_pascal_string);
        m_pascal_string = nil;
    }
    
    const unsigned char* operator * (void) const
    {
        return m_pascal_string;
    }
    
private:
    unsigned char *m_pascal_string = nullptr;
};

////////////////////////////////////////////////////////////////////////////////

class MCAutoStringRefAsSysString
{
public:
    constexpr MCAutoStringRefAsSysString() = default;

    ~MCAutoStringRefAsSysString()
    {
        Unlock();
    }

    bool Lock(MCStringRef p_string)
    {
        MCAssert(m_bytes == nil);
        return MCStringConvertToSysString(p_string, m_bytes, m_byte_count);
    }

    void Unlock()
    {
        if (m_bytes != nil)
        {
            free(m_bytes);
            m_bytes = nil;
            m_byte_count = 0;
        }
    }

    const char *operator * () const
    {
        return Ptr();
    }
    
    const char *Ptr(void) const
    {
        MCAssert(m_bytes != nil);
        return m_bytes;
    }
    
    size_t Size(void) const
    {
        return m_byte_count;
    }

private:
    char *m_bytes = nullptr;
    size_t m_byte_count = 0;
};

////////////////////////////////////////////////////////////////////////////////

#if 0
#ifdef __WINDOWS__
#include <WTypes.h>
class MCAutoStringRefAsBSTR
{
public:
    constexpr MCAutoStringRefAsBSTR() = default;
    
    ~MCAutoStringRefAsBSTR(void)
    {
        Unlock();
    }
    
    bool Lock(MCStringRef p_string)
    {
        return MCStringConvertToBSTR(p_string, m_bstr);
    }
    
    void Unlock(void)
    {
        SysFreeString(m_bstr);        
        m_bstr = nil;
    }
    
    BSTR operator * (void)
    {
        return m_bstr;
    }
    
private:
    BSTR m_bstr = nullptr;
};
#endif // __WINDOWS__
#endif

////////////////////////////////////////////////////////////////////////////////

class MCAutoStringRefAsNativeChars
{
public:
	constexpr MCAutoStringRefAsNativeChars() = default;

	bool Lock(MCStringRef p_string)
	{
		return MCStringNativeCopy(p_string, &m_string);
	}
	bool Lock(MCStringRef p_string,
	          const char_t * & r_buffer,
	          uindex_t & r_length)
	{
		if (!Lock(p_string))
			return false;
		r_buffer = MCStringGetNativeCharPtrAndLength(*m_string,
		                                             r_length);
		return true;
	}
	void Unlock()
	{
		m_string.Reset();
	}
	operator bool () const
	{
		return m_string.IsSet();
	}
	const char_t * operator*() const
	{
		MCAssert(MCStringIsNative(*m_string));
		return MCStringGetNativeCharPtr(*m_string);
	}
	uindex_t Size() const
	{
		MCAssert(MCStringIsNative(*m_string));
		uindex_t t_length;
		(void) MCStringGetNativeCharPtrAndLength(*m_string, t_length);
		return t_length;
	}
	MCSpan<const char_t> Span() const
	{
		return MCMakeSpan(operator*(), Size());
	}
private:
	MCAutoStringRef m_string;
};

////////////////////////////////////////////////////////////////////////////////

class MCAutoStringRefAsCString : public MCAutoStringRefAsNativeChars
{
public:
	const char * operator* () const
	{
		return reinterpret_cast<const char *>(MCAutoStringRefAsNativeChars::operator*());
	}
	MCSpan<const char> Span() const
	{
		return MCMakeSpan(operator*(), Size());
	}
};

////////////////////////////////////////////////////////////////////////////////

/* This version of MCAutoPointer should be used when you need to
 * manage a value created with the "new" operator.  For example:
 *
 *     MCAutoPointer<int> number;
 *     number = new (nothrow) int;
 */
template<typename T> class MCAutoPointer
{
public:
    typedef T* pointer;

    constexpr MCAutoPointer() = default;

    constexpr MCAutoPointer(decltype(nullptr)) {}

    /* Construct the managed pointer using a specific value. */
    constexpr MCAutoPointer(pointer p) : m_ptr(p) {}

    /* Construct managed pointer by moving a pointer from another
     * managed pointer of the same type. */
    MCAutoPointer(MCAutoPointer&& other) : m_ptr(other.Release()) {}

    ~MCAutoPointer() { Reset(); }

    /* Destroy the managed pointer, and take ownership of the value
     * provided.  Providing no value results in the managed pointer
     * being cleared after calling Reset(). */
    void Reset(pointer value = nullptr)
    {
        delete m_ptr;
        m_ptr = value;
    }

    /* Relinquish ownership of the managed pointer.  The MCAutoPointer is
     * null after Release() is called. */
    pointer Release() ATTRIBUTE_UNUSED
    {
        pointer t_result = m_ptr;
        m_ptr = nullptr;
        return t_result;
    }

    /* Test whether the MCAutoPointer has been set. */
    operator bool() { return m_ptr != nullptr; }

    /* Return the pointer managed by the MCAutoPointer, or nullptr if
     * there is no managed pointer. */
    pointer Get() { return m_ptr; }

    MCAutoPointer& operator=(MCAutoPointer&& other)
    {
        Reset(other.Release());
        return *this;
    }

	T*& operator & (void)
	{
		MCAssert(m_ptr == nil);
		return m_ptr;
	}

	T* operator -> (void)
	{
		MCAssert(m_ptr != nil);
		return m_ptr;
	}

	T *operator * (void) const
	{
		return m_ptr;
	}

	void Take(T*&r_ptr)
	{
        r_ptr = Release();
	}

private:
	T *m_ptr = nullptr;
};

/* This version of MCAutoPointer should be used when you need to
 * manage a value created with the "new[]" operator.  For example:
 *
 *     MCAutoPointer<int[]> numbers;
 *     numbers = new (nothrow) int[25];
 */
template<typename T> class MCAutoPointer<T[]>
{
public:
    typedef T* pointer;

    constexpr MCAutoPointer() = default;

    constexpr MCAutoPointer(decltype(nullptr)) {}

    /* Construct the managed pointer using a specific value. */
    constexpr MCAutoPointer(pointer p) : m_ptr(p) {}

    /* Construct managed pointer by moving a pointer from another
     * managed pointer of the same type. */
    MCAutoPointer(MCAutoPointer&& other) : m_ptr(other.Release()) {}

    ~MCAutoPointer() { Reset(); }

    /* Destroy the managed pointer, and take ownership of the value
     * provided.  Providing no value results in the managed pointer
     * being cleared after calling Reset(). */
    void Reset(pointer value = nullptr)
    {
        delete[] m_ptr;
        m_ptr = value;
    }

    /* Relinquish ownership of the managed pointer.  The MCAutoPointer is
     * null after Release() is called. */
    pointer Release() ATTRIBUTE_UNUSED
    {
        pointer t_result = m_ptr;
        m_ptr = nullptr;
        return t_result;
    }

    /* Test whether the MCAutoPointer has been set. */
    operator bool() { return m_ptr != nullptr; }

    /* Return the pointer managed by the MCAutoPointer, or nullptr if
     * there is no managed pointer. */
    pointer Get() { return m_ptr; }

    MCAutoPointer& operator=(MCAutoPointer&& other)
    {
        Reset(other.Release());
        return *this;
    }

	T*& operator & (void)
	{
		MCAssert(m_ptr != nil);
		return m_ptr;
	}

	T* operator -> (void)
	{
		MCAssert(m_ptr != nil);
		return m_ptr;
	}

	T* operator * (void)
	{
		return m_ptr;
	}

	void Take(T* & r_ptr)
	{
        r_ptr = Release();
	}
    
    T& operator [] (size_t x)
    {
        return m_ptr[x];
    }
	
private:
	T *m_ptr = nullptr;
};

template <typename T, void (Deleter)(T*)>
class MCAutoCustomPointer
{
public:
 
    typedef T* pointer;
    
    constexpr MCAutoCustomPointer() = default;
    
    constexpr MCAutoCustomPointer(T*&& take) : m_ptr(take) {}
    
	~MCAutoCustomPointer()
	{
		Deleter(m_ptr);
	}
	
	MCAutoCustomPointer& operator= (T*&& value)
	{
		Deleter(m_ptr);
		m_ptr = value;
		return *this;
	}
	
	T*& operator& ()
	{
		MCAssert(m_ptr == nil);
		return m_ptr;
	}
	
	T* operator* () const
	{
		return m_ptr;
	}

    T* operator -> (void)
    {
        MCAssert(m_ptr != nullptr);
        return m_ptr;
    }

    operator bool () const
    {
        return m_ptr != nil;
    }
	
	void ReleaseTo(T*& r_ptr)
	{
		r_ptr = m_ptr;
		m_ptr = nil;
	}
	
	T* Release()
	{
		T* t_ptr = m_ptr;
		m_ptr = nil;
		return t_ptr;
	}
	
private:
    
	T* m_ptr = nullptr;
};

////////////////////////////////////////////////////////////////////////////////

template<typename T> class MCAutoBlock
{
public:
	MCAutoBlock(void)
	{
		m_ptr = nil;
	}

	~MCAutoBlock(void)
	{
		MCMemoryDeallocate(m_ptr);
	}

	bool Allocate(size_t p_size)
	{
		MCAssert(m_ptr == nil);
		return MCMemoryAllocate(p_size, m_ptr);
	}

	bool Reallocate(size_t p_new_size)
	{
		return MCMemoryReallocate(p_new_size, m_ptr, m_ptr);
	}

	void Deallocate(void)
	{
		MCMemoryDeallocate(m_ptr);
		m_ptr = nil;
	}

	T *operator * (void)
	{
		return m_ptr;
	}

private:
	T *m_ptr;
};

////////////////////////////////////////////////////////////////////////////////

template<typename T> class MCAutoArray
{
public:
    constexpr MCAutoArray() = default;

	~MCAutoArray(void)
	{
		MCMemoryDeleteArray(m_ptr);
	}

	//////////

	T* Ptr()
	{
		return m_ptr;
	}

	uindex_t Size() const
	{
		return m_size;
	}

	//////////

	bool New(uindex_t p_size)
	{
		MCAssert(m_ptr == nil);
		return MCMemoryNewArray(p_size, m_ptr, m_size);
	}

	void Delete(void)
	{
		MCMemoryDeleteArray(m_ptr);
		m_ptr = nil;
		m_size = 0;
	}

	//////////

	bool Resize(uindex_t p_new_size)
	{
		return MCMemoryResizeArray(p_new_size, m_ptr, m_size);
	}

	bool Extend(uindex_t p_new_size)
	{
		MCAssert(p_new_size >= m_size);
		return Resize(p_new_size);
	}

	void Shrink(uindex_t p_new_size)
	{
		MCAssert(p_new_size <= m_size);
		Resize(p_new_size);
	}

	//////////

	bool Push(T p_value)
	{
		if (!Extend(m_size + 1))
			return false;
		m_ptr[m_size - 1] = p_value;
		return true;
	}

	//////////

	T*& PtrRef()
	{
		MCAssert(m_ptr == nil);
		return m_ptr;
	}

	uindex_t& SizeRef()
	{
		MCAssert(m_size == 0);
		return m_size;
	}

	//////////

	void Take(T*& r_array, uindex_t& r_count)
	{
		r_array = m_ptr;
		r_count = m_size;

		m_ptr = nil;
		m_size = 0;
	}

	//////////

	T& operator [] (uindex_t p_index)
	{
		return m_ptr[p_index];
	}
    
    const T& operator [] (uindex_t p_index) const
    {
        return m_ptr[p_index];
    }

private:
	T *m_ptr = nullptr;
	uindex_t m_size = 0;
};

/* Use the MCAutoArrayZeroedNonPod class when T has a destructor which will
 * only delete non-null members. */
template<typename T> class MCAutoArrayZeroedNonPod
{
public:
    constexpr MCAutoArrayZeroedNonPod() = default;
    
    ~MCAutoArrayZeroedNonPod(void)
    {
        for(uindex_t i = 0; i < m_size; i++)
            m_ptr[i].~T();
        MCMemoryDeleteArray(m_ptr);
    }
    
    //////////
    
    T* Ptr()
    {
        return m_ptr;
    }
    
    uindex_t Size() const
    {
        return m_size;
    }
    
    //////////
    
    bool New(uindex_t p_size)
    {
        MCAssert(m_ptr == nil);
        return MCMemoryNewArray(p_size, m_ptr, m_size);
    }
    
    void Delete(void)
    {
        MCMemoryDeleteArray(m_ptr);
        m_ptr = nil;
        m_size = 0;
    }
    
    //////////
    
    bool Resize(uindex_t p_new_size)
    {
        return MCMemoryResizeArray(p_new_size, m_ptr, m_size);
    }
    
    bool Extend(uindex_t p_new_size)
    {
        MCAssert(p_new_size >= m_size);
        return Resize(p_new_size);
    }
    
    void Shrink(uindex_t p_new_size)
    {
        MCAssert(p_new_size <= m_size);
        Resize(p_new_size);
    }
    
    //////////
    
    bool Push(T p_value)
    {
        if (!Extend(m_size + 1))
            return false;
        m_ptr[m_size - 1] = p_value;
        return true;
    }
    
    //////////
    
    T*& PtrRef()
    {
        MCAssert(m_ptr == nil);
        return m_ptr;
    }
    
    uindex_t& SizeRef()
    {
        MCAssert(m_size == 0);
        return m_size;
    }
    
    //////////
    
    void Take(T*& r_array, uindex_t& r_count)
    {
        r_array = m_ptr;
        r_count = m_size;
        
        m_ptr = nil;
        m_size = 0;
    }
    
    //////////
    
    T& operator [] (uindex_t p_index)
    {
        return m_ptr[p_index];
    }
    
    const T& operator [] (uindex_t p_index) const
    {
        return m_ptr[p_index];
    }
    
private:
    T *m_ptr = nullptr;
    uindex_t m_size = 0;
};

// Version of MCAutoArray that applies the provided deallocator to each element of the array when freed
template <typename T, void (*FREE)(T)> class MCAutoCustomPointerArray
{
public:
    constexpr MCAutoCustomPointerArray() = default;
	
	~MCAutoCustomPointerArray(void)
	{
		FreeElements(MCRangeMake(0, m_size));
		
		MCMemoryDeleteArray(m_ptr);
	}
	
	//////////
	
	T* Ptr()
	{
		return m_ptr;
	}
	
	uindex_t Size() const
	{
		return m_size;
	}
	
	//////////
	
	bool New(uindex_t p_size)
	{
		MCAssert(m_ptr == nil);
		return MCMemoryNewArray(p_size, m_ptr, m_size);
	}
	
	void Delete(void)
	{
		FreeElements(MCRangeMake(0, m_size));
		
		MCMemoryDeleteArray(m_ptr);
		m_ptr = nil;
		m_size = 0;
	}
	
	//////////
	
	bool Resize(uindex_t p_new_size)
	{
		if (p_new_size < m_size)
			FreeElements(MCRangeMakeMinMax(p_new_size, m_size));
		
		return MCMemoryResizeArray(p_new_size, m_ptr, m_size);
	}
	
	bool Extend(uindex_t p_new_size)
	{
		MCAssert(p_new_size >= m_size);
		return Resize(p_new_size);
	}
	
	void Shrink(uindex_t p_new_size)
	{
		MCAssert(p_new_size <= m_size);
		Resize(p_new_size);
	}
	
	//////////
	
	bool Push(T p_value)
	{
		if (!Extend(m_size + 1))
			return false;
		m_ptr[m_size - 1] = p_value;
		return true;
	}
	
	//////////
	
	T*& PtrRef()
	{
		MCAssert(m_ptr == nil);
		return m_ptr;
	}
	
	uindex_t& SizeRef()
	{
		MCAssert(m_size == 0);
		return m_size;
	}
	
	//////////
	
	void Take(T*& r_array, uindex_t& r_count)
	{
		r_array = m_ptr;
		r_count = m_size;
		
		m_ptr = nil;
		m_size = 0;
	}
	
	//////////
	
	T& operator [] (uindex_t p_index)
	{
		return m_ptr[p_index];
	}
	
	const T& operator [] (uindex_t p_index) const
	{
		return m_ptr[p_index];
	}
	
private:
    T *m_ptr = nullptr;
    uindex_t m_size = 0;
	
	void FreeElements(const MCRange &p_elements)
	{
		uindex_t t_end;
		t_end = (uindex_t)MCMin(m_size, p_elements.offset + p_elements.length);
		
		for (uindex_t i = p_elements.offset; i < t_end; i++)
		{
			if (m_ptr[i] != nil)
			{
				FREE(m_ptr[i]);
				m_ptr[i] = nil;
			}
		}
	}
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, MCStringEncoding ENCODING> class MCAutoCharArray
{
public:
    constexpr MCAutoCharArray() = default;

	~MCAutoCharArray(void)
	{
		if (m_chars != nil)
			MCMemoryDeleteArray(m_chars);
	}

	//////////

	T *Chars(void) const
	{
		return m_chars;
	}

	size_t CharCount(void) const
	{
		return m_char_count;
	}

	//////////
	
	void Give(T *p_chars, uindex_t p_char_count)
	{
		MCAssert(m_chars == nil);
		m_chars = p_chars;
		m_char_count = p_char_count;
	}
	
	//////////

	bool New(uindex_t p_size)
	{
		MCAssert(m_chars == nil);
		return MCMemoryNewArray(p_size, m_chars, m_char_count);
	}

	void Delete(void)
	{
		MCMemoryDeleteArray(m_chars);
		m_chars = nil;
		m_char_count = 0;
	}

	//////////

	bool Resize(uindex_t p_new_size)
	{
		return MCMemoryResizeArray(p_new_size, m_chars, m_char_count);
	}

	bool Extend(uindex_t p_new_size)
	{
		MCAssert(p_new_size >= m_char_count);
		return Resize(p_new_size);
	}

	void Shrink(uindex_t p_new_size)
	{
		MCAssert(p_new_size <= m_char_count);
		Resize(p_new_size);
	}

	//////////

	bool CreateString(MCStringRef& r_string)
	{
		return MCStringCreateWithBytes(m_chars, m_char_count * sizeof(T), ENCODING, false, r_string);
	}

	bool CreateStringAndRelease(MCStringRef& r_string)
	{
		if (MCStringCreateWithBytes(m_chars, m_char_count * sizeof(T), ENCODING, false, r_string))
		{
            MCMemoryDeleteArray(m_chars);
			m_chars = nil;
			m_char_count = 0;
			return true;
		}
		return false;
	}

private:
	T *m_chars = nullptr;
	uindex_t m_char_count = 0;
};

typedef MCAutoCharArray<char_t, kMCStringEncodingNative> MCAutoNativeCharArray;
typedef MCAutoCharArray<char_t, kMCStringEncodingUTF8> MCAutoUTF8CharArray;

//////////

class MCAutoByteArray
{
public:
    constexpr MCAutoByteArray() = default;
	
	~MCAutoByteArray(void)
	{
		MCMemoryDeleteArray(m_bytes);
	}
	
	//////////
	
	byte_t *Bytes(void) const
	{
		return m_bytes;
	}
	
	size_t ByteCount(void) const
	{
		return m_byte_count;
	}
	
	//////////
	
	void Give(byte_t *p_bytes, uindex_t p_byte_count)
	{
		MCAssert(m_bytes == nil);
		m_bytes = p_bytes;
		m_byte_count = p_byte_count;
	}
	
	//////////
	
	bool New(uindex_t p_size)
	{
		MCAssert(m_bytes == nil);
		return MCMemoryNewArray(p_size, m_bytes, m_byte_count);
	}
	
	void Delete(void)
	{
		MCMemoryDeleteArray(m_bytes);
		m_bytes = nil;
		m_byte_count = 0;
	}
	
	//////////
	
	bool Resize(uindex_t p_new_size)
	{
		return MCMemoryResizeArray(p_new_size, m_bytes, m_byte_count);
	}
	
	bool Extend(uindex_t p_new_size)
	{
		MCAssert(p_new_size >= m_byte_count);
		return Resize(p_new_size);
	}
	
	void Shrink(uindex_t p_new_size)
	{
		MCAssert(p_new_size <= m_byte_count);
		Resize(p_new_size);
	}
	
	//////////
	
	bool CreateData(MCDataRef& r_data)
	{
		return MCDataCreateWithBytes(m_bytes, m_byte_count, r_data);
	}
	
	bool CreateDataAndRelease(MCDataRef& r_data)
	{
		if (MCDataCreateWithBytesAndRelease(m_bytes, m_byte_count, r_data))
		{
			m_bytes = nil;
			m_byte_count = 0;
			return true;
		}
		return false;
	}
	
private:
	char_t *m_bytes = nullptr;
	uindex_t m_byte_count = 0;
};

////////////////////////////////////////////////////////////////////////////////

#endif
