/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#ifndef __MCUTILITY_H
#define __MCUTILITY_H

#ifndef _STRING_H
#include <string.h>
#endif

#include "foundation-unicode.h"

////////////////////////////////////////////////////////////////////////////////

void *memdup(const void *p_src, unsigned int p_src_length);

Boolean strequal(const char *one, const char *two);
Boolean strnequal(const char *one, const char *two, size_t n);
char *strclone(const char *one);
char *strclone(const char *one, uint4 length);

int4 MCU_strcasecmp(const char *a, const char *b);
int4 MCU_strncasecmp(const char *one, const char *two, size_t n);
bool MCU_strcaseequal(const char *a, const char *b);
Boolean MCU_strchr(const char *&sptr, uint4 &l, char target, Boolean isunicode);

////////////////////////////////////////////////////////////////////////////////

inline char *MCU_empty()
{
	return strclone("");
}

inline void MCU_skip_spaces(MCStringRef p_input, uindex_t& x_offset)
{
    while (MCUnicodeIsWhitespace(MCStringGetCharAtIndex(p_input, x_offset)))
        x_offset++;
}

inline void MCU_skip_spaces(const char *&sptr, uint4 &l)
{
	while (l && isspace((uint1)*sptr))
	{
		sptr++;
		l--;
	}
}

inline void MCU_skip_char(const char *&sptr, uint4 &l)
{
	if (l)
	{
		sptr++;
		l--;
	}
}


inline Boolean MCU_comparechar(const char *sptr, char target, Boolean isunicode)
{
	if (isunicode)
	{
		uint2 *uchar = (uint2 *)sptr;
		return *uchar == (uint2)target;
	}
	else
		return *sptr == target;
}

inline uint1 MCU_charsize(Boolean isunicode)
{
	return isunicode ? 2 : 1;
}

////////////////////////////////////////////////////////////////////////////////

class MCString
{
protected:
	const char *sptr;
	uint4 length;
	
public:
	MCString(void)
	{
			sptr = NULL;
			length = 0;
	}
	
	MCString(const char *s, uint4 l)
	{
		sptr = s;
		length = l;
	}
	
	MCString(const char *s);
	
	~MCString(void)
	{
	}
	
	void set(const char *s, uint4 l)
	{
		sptr = s;
		length = l;
	}
	
	void setstring(const char *s)
	{
		sptr = s;
	}
	
	void setlength(uint4 l)
	{
		length = l;
	}
	
	void get(const char *&s, uint4 &l)
	{
		sptr = s;
		length = l;
	}
	
	const char *getstring() const
	{
		return sptr;
	}
	
	uint4 getlength() const
	{
		return length;
	}
	
	char *clone() const;
	
	// Split this MCString into two at the given character.
	// Returns true if a split is found, otherwise r_head is equal to
	// this and r_tail is empty.
	bool split(char p_char, MCString& r_head, MCString& r_tail);
	
	Boolean operator=(const char *s);

	Boolean operator=(const MCString &s)
	{
		sptr = s.sptr;
		length = s.length;
		return True;
	}
	
	Boolean operator==(const MCString &two) const;
	Boolean operator!=(const MCString &two) const;
	Boolean equalexactly(const MCString& other) const;
};

struct MCDictionary
{
public:
	MCDictionary(void);
	~MCDictionary(void);

	void Set(uint4 p_id, MCString p_value);
	bool Get(uint4 p_id, MCString& r_value);

	bool Unpickle(const void *p_buffer, uint4 p_length);
	void Pickle(void*& r_buffer, uint4& r_length);

private:
	struct Node
	{
		Node *next;
		uint4 key;
		void *buffer;
		uint4 length;
	};

	Node *Find(uint4 p_id);
	uint32_t Checksum(const void *p_data, uint32_t p_length);

	Node *m_nodes;
};

extern MCString MCtruemcstring;
extern MCString MCfalsemcstring;
extern MCString MCnullmcstring;

inline const MCString &MCU_btos(uint4 condition)
{
	return condition ? MCtruemcstring : MCfalsemcstring;
}

inline uint4 MCU_abs(int4 source)
{
	return source > 0 ? source : -source;
}

inline int4 MCU_min(int4 one, int4 two) {return one > two ? two : one;}
inline uint4 MCU_min(uint4 one, uint4 two) {return one > two ? two : one;}

inline int4 MCU_max(int4 one, int4 two) {return one > two ? one : two;}
inline uint4 MCU_max(uint4 one, uint4 two) {return one > two ? one : two;}

inline float32_t MCU_max(float32_t one, float32_t two) {return one > two ? one : two;}
inline float32_t MCU_min(float32_t one, float32_t two) {return one > two ? two : one;}

inline int4 MCU_clamp(int4 v, int4 lower, int4 upper) {return v < lower ? lower : (v > upper ? upper : v);}

inline real8 MCU_fmin(real8 one, real8 two)
{
	return one > two ? two : one;
}

inline real8 MCU_fmax(real8 one, real8 two)
{
	return one > two ? one : two;
}

extern Boolean MCswapbytes;

inline uint4 swap_uint4(uint4 *dest)
{
	if (MCswapbytes)
	{
		uint1 *tptr = (uint1 *)dest;
		uint1 tmp = tptr[0];
		tptr[0] = tptr[3];
		tptr[3] = tmp;
		tmp = tptr[1];
		tptr[1] = tptr[2];
		tptr[2] = tmp;
	}
	return *dest;
}

inline int4 swap_int4(int4 *dest)
{
	if (MCswapbytes)
	{
		uint1 *tptr = (uint1 *)dest;
		uint1 tmp = tptr[0];
		tptr[0] = tptr[3];
		tptr[3] = tmp;
		tmp = tptr[1];
		tptr[1] = tptr[2];
		tptr[2] = tmp;
	}
	return *dest;
}

inline uint2 swap_uint2(uint2 *dest)
{
	if (MCswapbytes)
	{
		uint1 *tptr = (uint1 *)dest;
		uint1 tmp = tptr[0];
		tptr[0] = tptr[1];
		tptr[1] = tmp;
	}
	return *dest;
}

inline int2 swap_int2(int2 *dest)
{
	if (MCswapbytes)
	{
		uint1 *tptr = (uint1 *)dest;
		uint1 tmp = tptr[0];
		tptr[0] = tptr[1];
		tptr[1] = tmp;
	}
	return *dest;
}

inline uint2 get_uint2(char *source)
{
	uint2 result;
	uint1 *tptr = (uint1 *)&result;
	*tptr++ = *source++;
	*tptr = *source;
	return swap_uint2(&result);
}

inline uint4 get_uint4(char *source)
{
	uint4 result;
	uint1 *tptr = (uint1 *)&result;
	*tptr++ = *source++;
	*tptr++ = *source++;
	*tptr++ = *source++;
	*tptr = *source;
	return swap_uint4(&result);
}

inline int2 swap_int2(int2 x)
{
	if (MCswapbytes)
		return (int2)((x << 8) | ((uint2)x >> 8));
	return x;
}

inline int2 swap_int2(uint2 x)
{
	if (MCswapbytes)
		return (x << 8) | (x >> 8);
	return x;
}

inline char *MC_strchr(const char *s, int c) // HACK for bug in GCC 2.5.x
{
	return strchr((char *)s, c);
}

inline Boolean MCU_ispunct(uint1 x)
{
	return ispunct(x) != 0 && x != 39 && x != 212 && x != 213;
}

#endif
