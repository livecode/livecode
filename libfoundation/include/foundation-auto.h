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

#ifndef __MC_FOUNDATION_AUTO__
#define __MC_FOUNDATION_AUTO__

#ifndef __MC_FOUNDATION__
#include "foundation.h"
#endif

////////////////////////////////////////////////////////////////////////////////

template<typename T> class MCAutoValueRefBase
{
public:

	MCAutoValueRefBase(void)
	{
		m_value = nil;
	}

	~MCAutoValueRefBase(void)
	{
		MCValueRelease(m_value);
	}

	T operator = (T value)
	{
		MCAssert(m_value == nil);
		m_value = (T)MCValueRetain(value);
		return value;
	}

	T& operator & (void)
	{
		MCAssert(m_value == nil);
		return m_value;
	}

	T operator * (void) const
	{
		return m_value;
	}

private:
	T m_value;
    
    MCAutoValueRefBase<T>& operator = (MCAutoValueRefBase<T>& x);
};

typedef MCAutoValueRefBase<MCValueRef> MCAutoValueRef;
typedef MCAutoValueRefBase<MCNumberRef> MCAutoNumberRef;
typedef MCAutoValueRefBase<MCStringRef> MCAutoStringRef;
typedef MCAutoValueRefBase<MCArrayRef> MCAutoArrayRef;
typedef MCAutoValueRefBase<MCListRef> MCAutoListRef;
typedef MCAutoValueRefBase<MCBooleanRef> MCAutoBooleanRef;
typedef MCAutoValueRefBase<MCSetRef> MCAutoSetRef;

typedef MCAutoValueRefBase<MCNameRef> MCNewAutoNameRef;
typedef MCAutoValueRefBase<MCDataRef> MCAutoDataRef;


////////////////////////////////////////////////////////////////////////////////

template<typename T> class MCAutoValueRefArrayBase
{
public:
	MCAutoValueRefArrayBase(void)
	{
		m_values = nil;
        m_value_count = 0;
	}
    
	~MCAutoValueRefArrayBase(void)
	{
        if (m_values != nil)
        {
            for (uindex_t i = 0; i < m_value_count; i++)
                MCValueRelease(m_values[i]);
            MCMemoryDeleteArray(m_values);
        }
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

	//////////

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
    
    T& operator [] (const int p_index)
    {
        MCAssert(m_values != nil);
        return m_values[p_index];
    }
    
private:
	T* m_values;
    uindex_t m_value_count;
};

typedef MCAutoValueRefArrayBase<MCValueRef> MCAutoValueRefArray;
typedef MCAutoValueRefArrayBase<MCNumberRef> MCAutoNumberRefArray;
typedef MCAutoValueRefArrayBase<MCStringRef> MCAutoStringRefArray;
typedef MCAutoValueRefArrayBase<MCArrayRef> MCAutoArrayRefArray;
typedef MCAutoValueRefArrayBase<MCListRef> MCAutoListRefArray;
typedef MCAutoValueRefArrayBase<MCBooleanRef> MCAutoBooleanRefArray;
typedef MCAutoValueRefArrayBase<MCSetRef> MCAutoSetRefArray;
typedef MCAutoValueRefArrayBase<MCNameRef> MCAutoNameRefArray;

////////////////////////////////////////////////////////////////////////////////

class MCAutoStringRefAsWString
{
public:
    MCAutoStringRefAsWString(void)
    {
        m_ref = nil;
        m_wstring = nil;
    }
    
    ~MCAutoStringRefAsWString(void)
    {
        Unlock();
    }
    
    bool Lock(MCStringRef p_string)
    {
        if (p_string == nil)
			return false;
		
		if (MCStringIsNative(p_string))
        {
            m_ref = nil;
            return MCStringConvertToWString(p_string, m_wstring);
        }
        
        m_ref = MCValueRetain(p_string);
        // The pointer returned is not const-declared but rather cast as const before
        // being returned, so that casting away the const is safe.
        m_wstring = const_cast<unichar_t*>(MCStringGetCharPtr(p_string));
        return true;
    }
    
    void Unlock(void)
    {
        if (m_ref != nil)
        {
            MCValueRelease(m_ref);
            m_ref = nil;
        }
        else
            MCMemoryDeleteArray(m_wstring);
        m_wstring = nil;
    }
    
    unichar_t *operator * (void)
    {
        return m_wstring;
    }
    
private:
    MCStringRef m_ref;
    unichar_t *m_wstring;
};

////////////////////////////////////////////////////////////////////////////////

class MCAutoStringRefAsUTF8String
{
public:
    MCAutoStringRefAsUTF8String(void)
    {
        m_utf8string = nil;
    }
    
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
    
    char *operator * (void)
    {
        return m_utf8string;
    }
    
    uindex_t Size()
    {
        return m_size;
    }
    
private:
    char *m_utf8string;
    uindex_t m_size;
};

////////////////////////////////////////////////////////////////////////////////

#if defined(__MAC__) || defined(__IOS__)
#include <CoreFoundation/CoreFoundation.h>
class MCAutoStringRefAsCFString
{
public:
    MCAutoStringRefAsCFString(void) :
        m_cfstring(nil)
    {}
    
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
    CFStringRef m_cfstring;
};

#endif // __MAC__ || __IOS__

////////////////////////////////////////////////////////////////////////////////

class MCAutoStringRefAsSysString
{
public:
    MCAutoStringRefAsSysString()
    {
        m_bytes = nil;
        m_byte_count = 0;
    }

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
    char *m_bytes;
    size_t m_byte_count;
};

////////////////////////////////////////////////////////////////////////////////

#if 0
#ifdef __WINDOWS__
#include <WTypes.h>
class MCAutoStringRefAsBSTR
{
public:
    MCAutoStringRefAsBSTR(void)
    {
    }
    
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
    BSTR m_bstr;
};
#endif // __WINDOWS__
#endif

////////////////////////////////////////////////////////////////////////////////

class MCAutoStringRefAsCString
{
public:
    MCAutoStringRefAsCString(void)
    {
        m_ref = nil;
        m_cstring = nil;
    }
    
    ~MCAutoStringRefAsCString(void)
    {
        Unlock();
    }
    
    bool Lock(MCStringRef p_string)
    {
        if (!MCStringIsNative(p_string))
        {
            m_ref = nil;
            return MCStringConvertToCString(p_string, m_cstring);
        }

        m_ref = MCValueRetain(p_string);
        m_cstring = nil;

        return true;
    }
    
    void Unlock(void)
    {
        if (m_ref != nil)
        {
            MCValueRelease(m_ref);
            m_ref = nil;
        }
        else
            MCMemoryDeleteArray(m_cstring);
        m_cstring = nil;
    }
    
    const char *operator * (void)
    {
        if (m_ref != nil)
            return (const char*)MCStringGetNativeCharPtr(m_ref);
        else
            return (const char*)m_cstring;
    }
    
private:
    MCStringRef m_ref;
    char *m_cstring;
};

////////////////////////////////////////////////////////////////////////////////

class MCAutoStringRefAsNativeChars
{
public:
    MCAutoStringRefAsNativeChars(void)
    {
        m_string = nil;
    }

    ~MCAutoStringRefAsNativeChars(void)
    {
        Unlock();
    }

    bool Lock(MCStringRef p_string, char_t *&r_chars, uindex_t &r_length)
    {
        bool t_success;
        t_success = true;

        uindex_t t_length;
        if (MCStringIsNative(p_string))
        {
            t_length = MCStringGetLength(p_string);
            t_success =  MCMemoryNewArray(t_length, m_string);
            memcpy(m_string, (const char*)MCStringGetNativeCharPtr(p_string), t_length);
        }
        else
            t_success =  MCStringConvertToNative(p_string, (char_t*&)m_string, t_length);

        if (t_success)
        {
            r_chars = (char_t*)m_string;
            r_length = t_length;
        }

        return t_success;
    }

    void Unlock(void)
    {
        MCMemoryDeleteArray(m_string);
        m_string = nil;
    }

private:
    char *m_string;
};

////////////////////////////////////////////////////////////////////////////////

template<typename T> class MCAutoPointer
{
public:
	MCAutoPointer(void)
	{
		m_ptr = nil;
	}

	~MCAutoPointer(void)
	{
		delete m_ptr;
	}

	T* operator = (T* value)
	{
		delete m_ptr;
		m_ptr = value;
		return value;
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
		r_ptr = m_ptr;
		m_ptr = nil;
	}

private:
	T *m_ptr;
};

template<typename T, void (*FREE)(const void*)> class MCAutoCustomPointer
{
	
public:
	MCAutoCustomPointer(void)
	{
		m_ptr = nil;
	}
	
	~MCAutoCustomPointer(void)
	{
		FREE(m_ptr);
	}
	
	T operator = (T value)
	{
		FREE(m_ptr);
		m_ptr = value;
		return value;
	}
	
	T& operator & (void)
	{
		MCAssert(m_ptr == nil);
		return m_ptr;
	}
	
	T operator * (void) const
	{
		return m_ptr;
	}
	
	void Take(T&r_ptr)
	{
		r_ptr = m_ptr;
		m_ptr = nil;
	}
	
private:
	T m_ptr;
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
	MCAutoArray(void)
	{
		m_ptr = nil;
		m_size = 0;
	}

	~MCAutoArray(void)
	{
		MCMemoryDeleteArray(m_ptr);
	}

	//////////

	T* Ptr()
	{
		return m_ptr;
	}

	uindex_t Size()
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

	T& operator [] (const int p_index)
	{
		return m_ptr[p_index];
	}

private:
	T *m_ptr;
	uindex_t m_size;
};

////////////////////////////////////////////////////////////////////////////////

class MCAutoNativeCharArray
{
public:
	MCAutoNativeCharArray(void)
	{
		m_chars = nil;
		m_char_count = 0;
	}

	~MCAutoNativeCharArray(void)
	{
		if (m_chars != nil)
			MCMemoryDeleteArray(m_chars);
	}

	//////////

	char_t *Chars(void) const
	{
		return m_chars;
	}

	size_t CharCount(void) const
	{
		return m_char_count;
	}

	//////////
	
	void Give(char_t *p_chars, uindex_t p_char_count)
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
		return MCStringCreateWithNativeChars(m_chars, m_char_count, r_string);
	}

	bool CreateStringAndRelease(MCStringRef& r_string)
	{
		if (MCStringCreateWithNativeChars(m_chars, m_char_count, r_string))
		{
            MCMemoryDeleteArray(m_chars);
			m_chars = nil;
			m_char_count = 0;
			return true;
		}
		return false;
	}

private:
	char_t *m_chars;
	uindex_t m_char_count;
};

class MCAutoByteArray
{
public:
	MCAutoByteArray(void)
	{
		m_bytes = nil;
		m_byte_count = 0;
	}
	
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
	char_t *m_bytes;
	uindex_t m_byte_count;
};

////////////////////////////////////////////////////////////////////////////////

#endif
