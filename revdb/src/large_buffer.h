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

#ifndef __LARGE_BUFFER__
#define __LARGE_BUFFER__

#include "stddef.h"

class large_buffer_t
{
public:
	large_buffer_t(void)
		: m_data(NULL),
		  m_frontier(NULL),
		  m_capacity(0)
	{
	}

	~large_buffer_t(void)
	{
		if (m_data != NULL)
			free(m_data);
	}

	void append(char p_char)
	{
		append(&p_char, 1);
	}

	void append(const void *p_data, unsigned int p_length)
	{
		if (m_frontier - m_data + p_length > m_capacity)
			if (!extend(p_length))
				return;

		memcpy(m_frontier, p_data, p_length);
		m_frontier = m_frontier + p_length;
	}

    void *ptr(void)
    {
        return m_data;
    }
    
    ptrdiff_t length(void)
    {
        return m_frontier - m_data;
    }
    
	void grab(void*& r_data, unsigned int& r_length)
	{
		r_length = m_frontier - m_data;
		r_data = (void *)realloc(m_data, r_length);
		
		m_data = NULL;
		m_frontier = NULL;
		m_capacity = 0;
	}

private:
	char *m_data;
	char *m_frontier;
	unsigned int m_capacity;

	bool extend(unsigned int p_amount)
	{
		unsigned int t_new_capacity;
		t_new_capacity = (m_frontier - m_data) + p_amount;
		if (t_new_capacity < 4096)
			t_new_capacity = (t_new_capacity + 4095) & ~4095;
		else if (t_new_capacity < 65536)
			t_new_capacity = (t_new_capacity + 65535) & ~65535;
		else
			t_new_capacity = (t_new_capacity + 1024 * 1024 - 1) & ~(1024 * 1024 - 1);

		char *t_new_data;
		t_new_data = (char *)realloc(m_data, t_new_capacity);
		if (t_new_data == NULL)
		{
			t_new_data = (char *)realloc(m_data, (m_frontier - m_data) + p_amount);
			if (t_new_data == NULL)
				return false;
		}

		m_frontier = t_new_data + (m_frontier - m_data);
		m_data = t_new_data;
		m_capacity = t_new_capacity;

		return true;
	}
};

#endif
