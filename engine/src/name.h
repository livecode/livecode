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

#ifndef __MC_NAME__
#define __MC_NAME__

////////////////////////////////////////////////////////////////////////////////

typedef struct MCName *MCNameRef;

typedef uint32_t MCCompareOptions;

enum
{
	kMCCompareExact,
	kMCCompareCaseless
};

bool MCNameCreateWithStaticCString(const char *cstring, MCNameRef& r_name);

bool MCNameCreateWithCString(const char *cstring, MCNameRef& r_name);
bool MCNameCreateWithOldString(const MCString& string, MCNameRef& r_name);
bool MCNameCreateWithChars(const char *chars, uindex_t char_count, MCNameRef& r_name);

void MCNameDelete(MCNameRef name);

bool MCNameClone(MCNameRef name, MCNameRef& r_new_name);

uintptr_t MCNameGetCaselessSearchKey(MCNameRef name);

const char *MCNameGetCString(MCNameRef name);
MCString MCNameGetOldString(MCNameRef name);

char MCNameGetCharAtIndex(MCNameRef name, uindex_t at);

bool MCNameIsEmpty(MCNameRef name);

bool MCNameIsEqualTo(MCNameRef left, MCNameRef right, MCCompareOptions options);
bool MCNameIsEqualToCString(MCNameRef left, const char *cstring, MCCompareOptions options);
bool MCNameIsEqualToOldString(MCNameRef left, const MCString& string, MCCompareOptions options);

MCNameRef MCNameLookupWithCString(const char *cstring, MCCompareOptions options);
MCNameRef MCNameLookupWithOldString(const MCString& string, MCCompareOptions options);

bool MCNameInitialize(void);
void MCNameFinalize(void);

extern MCNameRef kMCEmptyName;

////////////////////////////////////////////////////////////////////////////////

class MCAutoNameRef
{
public:
	MCAutoNameRef(void)
	{
		m_name = nil;
	}

	~MCAutoNameRef(void)
	{
		MCNameDelete(m_name);
	}

	bool CreateWithCString(const char *p_name)
	{
		return MCNameCreateWithCString(p_name, m_name);
	}

	bool CreateWithOldString(const MCString& p_name)
	{
		return MCNameCreateWithOldString(p_name, m_name);
	}

	bool Clone(MCNameRef p_name)
	{
		return MCNameClone(p_name, m_name);
	}

	MCNameRef Take(void)
	{
		MCNameRef t_name;
		t_name = m_name;
		m_name = nil;
		return t_name;
	}

	operator MCNameRef& (void)
	{
		return m_name;
	}

	operator MCNameRef (void) const
	{
		return m_name;
	}

private:
	MCNameRef m_name;
};

////////////////////////////////////////////////////////////////////////////////

#endif
