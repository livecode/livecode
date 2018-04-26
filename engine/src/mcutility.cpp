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

#include "prefix.h"

#include "globdefs.h"

////////////////////////////////////////////////////////////////////////////////

void *memdup(const void *p_src, unsigned int p_src_length)
{
	void *t_data;
	t_data = malloc(p_src_length);
	if (t_data != NULL)
		memcpy(t_data, p_src, p_src_length);
	return t_data;
}

Boolean strequal(const char *one, const char *two)
{
	if (one == two)
		return True;
	if (one == NULL || two == NULL)
		return False;
	return (Boolean)(strcmp(one, two) == 0);
}

Boolean strnequal(const char *one, const char *two, size_t n)
{
	if ((one == NULL) != (two == NULL))
		return False;
	if (one == NULL && two == NULL)
		return True;
	return (Boolean)(strncmp(one, two, n) == 0);
}

char *strclone(const char *one)
{
	char *two = NULL;
	if (one != NULL)
	{
		two = new (nothrow) char[strlen(one) + 1];
		strcpy(two, one);
	}
	return two;
}

char *strclone(const char *one, uint4 length)
{
	char *two = new (nothrow) char[length + 1];
	memcpy(two, one, length);
	two[length] = '\0';
	return two;
}

int4 MCU_strcasecmp(const char *a, const char *b)
{
	int4 d;	
	for(;;)
	{
		d = MCS_tolower(*a) - MCS_tolower(*b);
		
		if (d != 0)
			break;
		
		if (*a == '\0' || *b == '\0')
			break;

		a++;
		b++;
	}
	return d;
}

int4 MCU_strncasecmp(const char *one, const char *two, size_t n)
{
	const uint1 *optr = (const uint1 *)one;
	const uint1 *tptr = (const uint1 *)two;
	while (n--)
	{
		if (*optr != *tptr)
		{
			uint1 o = MCS_tolower(*optr);
			uint1 t = MCS_tolower(*tptr);
			if (o != t)
				return o - t;
		}
		optr++;
		tptr++;
	}
	return 0;
}

bool MCU_strcaseequal(const char *a, const char *b)
{
	if (a == b)
		return true;
	if (a == NULL || b == NULL)
		return false;
	return MCU_strcasecmp(a, b) == 0;
}

Boolean MCU_strchr(const char *&sptr, uint4 &l, char target, Boolean isunicode)
{
	const char *startptr = sptr;
	const char *eptr = sptr + l;
	if (!isunicode)
	{
		while (sptr < eptr)
		{
			if (*sptr == target)
			{
				l = eptr - sptr;
				return True;
			}
			sptr++;
		}
	}
	else
	{
		while (sptr < eptr)
		{
			if (MCU_comparechar(sptr, target, isunicode))
			{
				l = eptr - sptr;
				return True;
			}
			sptr += MCU_charsize(isunicode);
		}
	}
	sptr = startptr;
	return False;
}

////////////////////////////////////////////////////////////////////////////////

MCString::MCString(const char *s)
{
	sptr = s;
	if (sptr == NULL || !*sptr)
		length = 0;
	else
		length = strlen(s);
}

char *MCString::clone() const
{
	char *dptr = new (nothrow) char[length + 1];
	memcpy(dptr, sptr, length);
	dptr[length] = '\0';
	return dptr;
}

Boolean MCString::operator = (const char *s)
{
	sptr = s;
	if (s == NULL)
		length = 0;
	else
		length = strlen(s);
	return True;
}

Boolean MCString::operator ==(const MCString &two) const
{
	if (length != two.length)
		return False;
	if (sptr == two.sptr)
		return True;
	return (Boolean)(MCU_strncasecmp(sptr, two.sptr, length) == 0);
}

Boolean MCString::operator !=(const MCString &two) const
{
	if (length != two.length)
		return True;
	if (sptr == two.sptr)
		return False;
	return (Boolean)(MCU_strncasecmp(sptr, two.sptr, length) != 0);
}

Boolean MCString::equalexactly(const MCString& other) const
{
	if (length != other.length)
		return False;
	if (sptr == other.sptr)
		return True;
	return (Boolean)(strncmp(sptr, other.sptr, length) == 0);
}
