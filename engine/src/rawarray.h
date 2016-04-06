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

#ifndef __MC_RAW_ARRAY__
#define __MC_RAW_ARRAY__

////////////////////////////////////////////////////////////////////////////////

template<class T> class MCRawArray
{
public:
	MCRawArray(void);
	~MCRawArray(void);

	uindex_t Count(void) const;
	void Compact(void);
	void Clear(void);

	T operator [] (uindex_t index) const;
	T& operator [] (uindex_t index);

	bool Append(T value);
	bool AppendSeq(const T* values, uindex_t value_count);
	bool Prepend(T value);
	bool PrependSeq(const T* values, uindex_t value_count);
	bool Replace(uindex_t at, T value);
	bool ReplaceSeq(uindex_t at, uindex_t count, const T* values, uindex_t value_count);
	bool Insert(uindex_t at, T value);
	bool InsertSeq(uindex_t at, const T* values, uindex_t value_count);
	void Remove(uindex_t at);
	void RemoveSeq(uindex_t at, uindex_t count);

private:
	T *m_elements;
	uindex_t m_count;
};

template<class T> inline MCRawArray<T>::MCRawArray(void)
{
	m_elements = nil;
	m_count = 0;
}

template<class T> inline MCRawArray<T>::~MCRawArray(void)
{
	MCMemoryDeleteArray(m_elements);
}

template<class T> inline uindex_t MCRawArray<T>::Count(void) const
{
	return m_count;
}

template<class T> inline T MCRawArray<T>::operator [] (uindex_t p_index) const
{
	return m_elements[p_index];
}

template<class T> inline T& MCRawArray<T>::operator [] (uindex_t p_index)
{
	return m_elements[p_index];
}

template<class T> inline bool MCRawArray<T>::Append(T p_value)
{
	if (!MCMemoryResizeArray(m_count + 1, m_elements, m_count))
		return false;
	m_elements[m_count - 1] = p_value;
	return true;
}

template<class T> inline bool MCRawArray<T>::Insert(uindex_t p_at, T p_value)
{
	if (!MCMemoryResizeArray(m_count + 1, m_elements, m_count))
		return false;
	MCMemoryMove(m_elements + p_at + 1, m_elements + p_at, sizeof(T) * (m_count - p_at - 1));
	m_elements[p_at] = p_value;
	return true;
}

template<class T> inline void MCRawArray<T>::Remove(uindex_t p_at)
{
	MCMemoryMove(m_elements + p_at, m_elements + p_at + 1, sizeof(T) * (m_count - p_at - 1));
	m_count -= 1;
}

////////////////////////////////////////////////////////////////////////////////

#endif
