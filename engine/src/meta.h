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

#ifndef __META_H
#define __META_H

namespace Meta
{

template<typename T> class static_ptr_t
{
	T *f_pointer;

public:
	static_ptr_t(void)
		: f_pointer(NULL)
	{
	}

	~static_ptr_t(void)
	{
		if (f_pointer != NULL)
			delete f_pointer;
	}

	T *operator = (T *p_value)
	{
		f_pointer = p_value;
		return f_pointer;
	}

	T *operator -> (void)
	{
		return f_pointer;
	}

	operator T *(void)
	{
		return f_pointer;
	}
};

class simple_string
{
	const char *f_data;
	size_t f_length;

public:
	simple_string(void)
		: f_data(NULL), f_length(0)
	{
	}

	simple_string(const simple_string& p_other)
		: f_data(p_other . f_data), f_length(p_other . f_length)
	{
	}

	simple_string(const char *p_string)
		: f_data(p_string), f_length(strlen(p_string))
	{
	}

	simple_string(const MCString& p_string)
		: f_data(p_string . getstring()), f_length(p_string . getlength())
	{
	}

	simple_string(const char *p_string, size_t p_length)
		: f_data(p_string), f_length(p_length)
	{
	}

	simple_string& operator = (const simple_string& p_other)
	{
		assign(p_other . f_data, p_other . f_length);
		return *this;
	}

	simple_string& operator = (const char *p_string)
	{
		assign(p_string, strlen(p_string));
		return *this;
	}

	simple_string& operator = (const MCString& p_other)
	{
		assign(p_other . getstring(), p_other . getlength());
		return *this;
	}
	
	bool operator == (const simple_string& p_other) const
	{
		return f_length == p_other . f_length && strncmp(f_data, p_other . f_data, f_length) == 0;
	}

	size_t length(void) const
	{
		return f_length;
	}

	const char *operator * (void) const
	{
		return f_data;
	}

	operator MCString(void) const
	{
		return MCString(f_data, uint4(f_length));
	}

private:
	void assign(const char *p_data, size_t p_length)
	{
		f_data = p_data;
		f_length = p_length;
	}
};

class simple_string_buffer
{
	char *f_data;
	unsigned int f_length;

public:
	simple_string_buffer(void)
		: f_data(NULL), f_length(0)
	{
	}

	~simple_string_buffer(void)
	{
		free(f_data);
	}

	void clear(void)
	{
		free(f_data);
		f_data = NULL;
		f_length = 0;
	}

	void resize(unsigned int p_new_length)
	{
		f_data = (char *)realloc(f_data, p_new_length + 1);
		f_length = p_new_length;
	}

	unsigned int length(void) const
	{
		return f_length;
	}

	char *buffer(void) const
	{
		return f_data;
	}

	char *operator *(void)
	{
		return f_data;
	}

	operator simple_string(void)
	{
		return simple_string(f_data, f_length);
	}
};

class itemised_string
{
	unsigned int f_count;
	simple_string *f_items;

public:
	itemised_string(void)
		: f_count(0), f_items(NULL)
	{
	}

	itemised_string(const char *p_string, unsigned int p_length, char p_delimiter, bool p_quoted = false)
		: f_count(0), f_items(NULL)
	{
		initialise(p_string, p_length, p_delimiter, p_quoted);
	}

	itemised_string(const char *p_string, char p_delimiter, bool p_quoted = false)
		: f_count(0), f_items(NULL)
	{
		initialise(p_string, strlen(p_string), p_delimiter, p_quoted);
	}

	itemised_string(const simple_string& p_string, char p_delimiter, bool p_quoted = false)
		: f_count(0), f_items(NULL)
	{
		initialise(*p_string, p_string . length(), p_delimiter, p_quoted);
	}

	~itemised_string(void)
	{
		delete[] f_items;
	}

	void assign(const char *p_string, unsigned int p_length, char p_delimiter, bool p_quoted = false)
	{
		delete[] f_items;
		initialise(p_string, p_length, p_delimiter, p_quoted);
	}

	void assign(const simple_string& p_string, char p_delimiter, bool p_quoted = false)
	{
		delete[] f_items;
		initialise(*p_string, p_string . length(), p_delimiter, p_quoted);
	}

	unsigned int count(void) const
	{
		return f_count;
	}

	const simple_string& operator [] (unsigned int p_index) const
	{
		return f_items[p_index];
	}

private:
	void initialise(const char *p_string, size_t p_length, char p_delimiter, bool p_quoted)
	{
		unsigned int t_count;
		unsigned int *t_items;
		bool t_in_quote = false;

		t_count = 0;
		t_items = NULL;

		for(unsigned int t_offset = 0; t_offset <= p_length; t_offset += 1)
			if (p_quoted && p_string[t_offset] == '"')
				t_in_quote = !t_in_quote;
			else if ((t_offset == p_length && p_string[t_offset - 1] != p_delimiter) || (p_string[t_offset] == p_delimiter && !t_in_quote))
			{
				unsigned int *t_new_items;
				t_new_items = (unsigned int *)realloc(t_items, sizeof(unsigned int) * (t_count + 1));
				if (t_new_items == NULL)
					break;
				t_items = t_new_items;
				t_items[t_count++] = t_offset;
			}

		if (t_items != NULL)
		{
			f_items = new (nothrow) simple_string[t_count];
			if (f_items != NULL)
			{
				f_count = t_count;
				for(unsigned int t_item = 0, t_offset = 0; t_item < t_count; t_offset = t_items[t_item] + 1, t_item += 1)
				{
					unsigned int t_adjust;
					t_adjust = (p_quoted && p_string[t_offset] == '"' ? 1 : 0);
					f_items[t_item] = simple_string(p_string + t_offset + t_adjust, t_items[t_item] - t_offset - 2 * t_adjust);
				}
			}
			free(t_items);
		}
		else
		{
			f_items = new (nothrow) simple_string[1];
			if (f_items != NULL)
			{
				f_count = 1;
				f_items[0] = simple_string(p_string, p_length);
			}
		}
	}
};

};

#endif
