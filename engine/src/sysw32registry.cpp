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

#include "w32prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "param.h"
#include "mcerror.h"
#include "execpt.h"
#include "util.h"
#include "object.h"
#include "stack.h"
#include "osspec.h"
#include "globals.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct
{ //struct for WIN registry
	const char *token;
	HKEY key;
	uint32_t mode;
}
reg_keytype;

static reg_keytype Regkeys[] = {  //WIN registry root keys struct
                                   {"HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT, 0},
                                   {"HKEY_CURRENT_USER", HKEY_CURRENT_USER, 0},
                                   {"HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE, 0},
                                   {"HKEY_LOCAL_MACHINE_32", HKEY_LOCAL_MACHINE, KEY_WOW64_32KEY},
                                   {"HKEY_LOCAL_MACHINE_64", HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY},
                                   {"HKEY_USERS", HKEY_USERS, 0},
                                   {"HKEY_PERFORMANCE_DATA", HKEY_PERFORMANCE_DATA, 0},
                                   {"HKEY_CURRENT_CONFIG", HKEY_CURRENT_CONFIG, 0},
                                   {"HKEY_DYN_DATA", HKEY_DYN_DATA, 0}
                               };

typedef struct
{
	const char *token;
	DWORD type;
}
reg_datatype;

static reg_datatype RegDatatypes[] = {  //WIN registry value types struct
                                         {"binary", REG_BINARY},
                                         {"dword", REG_DWORD},
                                         {"dwordlittleendian", REG_DWORD_LITTLE_ENDIAN},
                                         {"dwordbigendian", REG_DWORD_BIG_ENDIAN},
                                         {"expandsz", REG_EXPAND_SZ},
                                         {"link", REG_LINK},
                                         {"multisz", REG_MULTI_SZ},
                                         {"none", REG_NONE},
                                         {"resourcelist", REG_RESOURCE_LIST},
                                         {"string", REG_SZ},
                                         {"sz", REG_SZ}
                                     };

bool MCS_registry_split_key(MCStringRef p_path, MCStringRef& r_root, MCStringRef& r_key)
{
	uindex_t t_length = MCStringGetLength(p_path);
	uindex_t t_offset = t_length;
	if (!MCStringFirstIndexOfChar(p_path, '\\', 0, kMCStringOptionCompareExact, t_offset))
	{
		t_offset = t_length;
	}
	return MCStringCopySubstring(p_path, MCRangeMake(0, t_offset), r_root) &&
		MCStringCopySubstring(p_path, MCRangeMake(t_offset + 1, t_length), r_key);
}

bool MCS_registry_split_key(MCStringRef p_path, MCStringRef& r_root, MCStringRef& r_key, MCStringRef& r_value)
{
	// only copy components out if they are present - <ROOT>\<KEY>\<VALUE>, <ROOT>\<VALUE>, <ROOT>
	// (components may be empty)
	bool t_success = true;
	uindex_t t_length = MCStringGetLength(p_path);
	uindex_t t_path_offset = t_length;
	uindex_t t_value_offset = t_length;
	if (MCStringLastIndexOfChar(p_path, '\\', MCStringGetLength(p_path), kMCStringOptionCompareExact, t_value_offset))
	{
		if (MCStringFirstIndexOfChar(p_path, '\\', 0, kMCStringOptionCompareExact, t_path_offset))
		{
			if (t_value_offset > t_path_offset)
				t_success = t_success && MCStringCopySubstring(p_path, MCRangeMake(t_path_offset + 1, t_value_offset - t_path_offset - 1), r_key);
			else
				t_value_offset = t_length;
		}
		t_success = t_success && MCStringCopySubstring(p_path, MCRangeMake(t_value_offset + 1, t_length), r_value);
	}
	return t_success && MCStringCopySubstring(p_path, MCRangeMake(0, t_path_offset), r_root);
}

bool MCS_registry_root_to_hkey(MCStringRef p_root, HKEY& r_hkey)
{
	for (uindex_t i = 0 ; i < ELEMENTS(Regkeys) ; i++)
	{
		if (MCStringIsEqualToCString(p_root, Regkeys[i].token, kMCCompareCaseless))
		{
			r_hkey = Regkeys[i].key;
			return true;
		}
	}
	return false;
}

bool MCS_registry_root_to_hkey(MCStringRef p_root, HKEY& r_hkey, uint32_t& x_access_mode)
{
	for (uindex_t i = 0 ; i < ELEMENTS(Regkeys) ; i++)
	{
		if (MCStringIsEqualToCString(p_root, Regkeys[i].token, kMCCompareCaseless))
		{
			r_hkey = Regkeys[i].key;
			if (MCmajorosversion >= 0x0501)
				x_access_mode |= Regkeys[i].mode;
			return true;
		}
	}
	return false;
}

bool MCS_registry_type_to_string(uint32_t p_type, MCStringRef& r_string)
{
	for (uindex_t i = 0 ; i < ELEMENTS(RegDatatypes) ; i++)
	{
		if (p_type == RegDatatypes[i].type)
		{
			return MCStringCreateWithCString(RegDatatypes[i].token, r_string);
		}
	}

	return false;
}

DWORD MCS_registry_type_from_string(MCStringRef p_string)
{
	if (p_string != nil)
	{
		for (uindex_t i = 0; i < ELEMENTS(RegDatatypes); i++)
		{
			if (MCStringIsEqualToCString(p_string, RegDatatypes[i].token, kMCCompareCaseless))
				return RegDatatypes[i].type;
		}
	}

	return REG_SZ;
}

class MCAutoRegistryKey
{
public:
	MCAutoRegistryKey(void)
	{
		m_key = NULL;
	}

	~MCAutoRegistryKey(void)
	{
		if (m_key != NULL)
			RegCloseKey(m_key);
	}

	HKEY operator = (HKEY value)
	{
		MCAssert(m_key == NULL);
		m_key = value;
		return m_key;
	}

	HKEY* operator & (void)
	{
		MCAssert(m_key == NULL);
		return &m_key;
	}

	HKEY operator * (void) const
	{
		return m_key;
	}

private:
	HKEY m_key;
};
//
//bool MCS_list_registry(MCStringRef p_path, MCListRef& r_list, MCStringRef& r_error)
//{
//	MCAutoStringRef t_root, t_key;
//	HKEY t_hkey;
//	MCAutoRegistryKey t_regkey;
//
//	if (!MCS_registry_split_key(p_path, &t_root, &t_key))
//		return false;
//	if (!MCS_registry_root_to_hkey(*t_root, t_hkey) ||
//		RegOpenKeyExA(t_hkey, MCStringGetCString(*t_key), 0, KEY_READ, &t_regkey) != ERROR_SUCCESS)
//	{
//		return MCStringCreateWithCString("bad key", r_error);
//	}
//
//	MCAutoListRef t_list;
//	if (!MCListCreateMutable('\n', &t_list))
//		return false;
//
//	DWORD t_max_key_length;
//	if (ERROR_SUCCESS != RegQueryInfoKeyA(*t_regkey, NULL, NULL, NULL, NULL, &t_max_key_length, NULL, NULL, NULL, NULL, NULL, NULL))
//		return false;
//
//	DWORD t_index = 0;
//	MCAutoArray<char> t_buffer;
//
//	t_max_key_length++;
//	if (!t_buffer.New(t_max_key_length))
//		return false;
//
//	LONG t_result = ERROR_SUCCESS;
//	while (t_result == ERROR_SUCCESS)
//	{
//		DWORD t_name_length = t_max_key_length;
//		t_result = RegEnumKeyExA(*t_regkey, t_index, t_buffer.Ptr(), &t_name_length, NULL, NULL, NULL, NULL);
//		if (t_result == ERROR_SUCCESS && !MCListAppendCString(*t_list, t_buffer.Ptr()))
//			return false;
//
//		t_index += 1;
//	}
//
//	if (t_result != ERROR_NO_MORE_ITEMS)
//		return false;
//
//	r_error = nil;
//	return MCListCopy(*t_list, r_list);
//}
//
//bool MCS_query_registry(MCStringRef p_key, MCStringRef& r_value, MCStringRef& r_type, MCStringRef& r_error)
//{
//	MCAutoStringRef t_root, t_key, t_value;
//
//	r_value = r_error = nil;
//
//	//key = full path info such as: HKEY_LOCAL_MACHINE\\Software\\..\\valuename
//	if (!MCS_registry_split_key(p_key, &t_root, &t_key, &t_value))
//		return false;
//
//	if (*t_value == nil)
//	{
//		/* RESULT */ //MCresult->sets("no key");
//		return MCStringCreateWithCString("no key", r_error);
//	}
//
//	if (*t_key == nil)
//		t_key = kMCEmptyString;
//
//	HKEY t_hkey;
//	MCAutoRegistryKey t_regkey;
//
//	uint32_t t_access_mode = KEY_READ;
//
//	if (!MCS_registry_root_to_hkey(*t_root, t_hkey, t_access_mode) ||
//		(RegOpenKeyExA(t_hkey, MCStringGetCString(*t_key), 0, t_access_mode, &t_regkey) != ERROR_SUCCESS))
//	{
//		/* RESULT */ //MCresult->sets("bad key");
//		return MCStringCreateWithCString("bad key", r_error);
//	}
//
//	LONG err = 0;
//	MCAutoNativeCharArray t_buffer;
//	DWORD t_buffer_len = 0;
//	DWORD t_type;
//
//	//determine the size of value buffer needed
//	err = RegQueryValueExA(*t_regkey, MCStringGetCString(*t_value), 0, NULL, NULL, &t_buffer_len);
//	if (err == ERROR_SUCCESS || err == ERROR_MORE_DATA)
//	{
//		if (!t_buffer.New(t_buffer_len))
//			return false;
//		if ((err = RegQueryValueExA(*t_regkey, MCStringGetCString(*t_value), 0, &t_type,
//			(LPBYTE)t_buffer.Chars(), &t_buffer_len)) == ERROR_SUCCESS && t_buffer_len)
//		{
//			DWORD t_len;
//			t_len = t_buffer_len;
//			if (t_type == REG_SZ || t_type == REG_EXPAND_SZ || t_type == REG_MULTI_SZ)
//			{
//				char_t *t_chars = t_buffer.Chars();
//				while(t_len > 0 && t_chars[t_len - 1] == '\0')
//					t_len -= 1;
//				if (t_type == REG_MULTI_SZ && t_len < t_buffer_len)
//					t_len += 1;
//			}
//
//			t_buffer.Shrink(t_len);
//			return t_buffer.CreateStringAndRelease(r_value) &&
//                MCS_registry_type_to_string(t_type, r_type);
//		}
//	}
//	else
//	{
//		errno = err;
//		/* RESULT */ //MCresult->sets("can't find key");
//		return MCStringCreateWithCString("can't find key", r_error);
//	}
//
//	r_value = MCValueRetain(kMCEmptyString);
//	return true;
//}
//
//void MCS_query_registry(MCExecPoint &dest)
//{
//	MCAutoStringRef t_key;
//	/* UNCHECKED */ dest.copyasstringref(&t_key);
//	MCAutoStringRef t_value, t_type, t_error;
//	/* UNCHECKED */ MCS_query_registry(*t_key, &t_value, &t_type, &t_error);
//	if (*t_error != nil)
//	{
//		dest.clear();
//		/* UNCHECKED */ MCresult->setvalueref(*t_error);
//	}
//	else
//	{
//		dest.setvalueref(*t_value);
//		MCresult->clear();
//	}
//}
//
//bool MCS_set_registry(MCStringRef p_key, MCStringRef p_value, MCStringRef p_type, MCStringRef& r_error)
//{
//	MCAutoStringRef t_root, t_key, t_value;
//	HKEY t_root_hkey;
//
//	if (!MCS_registry_split_key(p_key, &t_root, &t_key, &t_value))
//		return false;
//
//	if (!MCS_registry_root_to_hkey(*t_root, t_root_hkey))
//	{
//		/* RESULT */ //MCresult->sets("bad key");
//		return MCStringCreateWithCString("bad key", r_error);
//	}
//	if (*t_key == nil)
//	{
//		/* RESULT */ //MCresult->sets("bad key specified");
//		return MCStringCreateWithCString("bad key specified", r_error);
//	}
//
//	MCAutoRegistryKey t_regkey;
//	DWORD t_keystate;
//
//	if (RegCreateKeyExA(t_root_hkey, MCStringGetCString(*t_key), 0, NULL, REG_OPTION_NON_VOLATILE,
//	                   KEY_ALL_ACCESS, NULL, &t_regkey, &t_keystate) != ERROR_SUCCESS)
//	{
//		MCS_seterrno(GetLastError());
//		/* RESULT */ //MCresult->sets("can't create key");
//		return MCStringCreateWithCString("can't create key", r_error);
//	}
//
//	if (MCStringGetLength(p_value) == 0)
//	{//delete this value
//		if ((errno = RegDeleteValueA(*t_regkey, MCStringGetCString(*t_value))) != ERROR_SUCCESS)
//		{
//			MCS_seterrno(GetLastError());
//			/* RESULT */ //MCresult->sets("can't delete value");
//			return MCStringCreateWithCString("can't delete value", r_error);
//		}
//	}
//	else
//	{
//		DWORD t_type;
//		t_type = MCS_registry_type_from_string(p_type);
//
//		const BYTE *t_byte_ptr = MCStringGetBytePtr(p_value);
//		uint32_t t_length = MCStringGetLength(p_value);
//		if (t_type == REG_SZ && t_byte_ptr[t_length - 1] != '\0')
//			t_length++;
//
//		if (RegSetValueExA(*t_regkey, MCStringGetCString(*t_value), 0, t_type,
//			t_byte_ptr, t_length) != ERROR_SUCCESS)
//		{
//			MCS_seterrno(GetLastError());
//			/* RESULT */ //MCresult->sets("can't set value");
//			return MCStringCreateWithCString("can't set value", r_error);
//		}
//	}
//
//	r_error = nil;
//	return true;
//}
//
////WINNT does not delete registry entry if there are subkeys..this functions fixes that.
//DWORD RegDeleteKeyNT(HKEY hStartKey, const char *pKeyName)
//{
//	DWORD dwRtn, dwSubKeyLength;
//	char *pSubKey = NULL;
//	char szSubKey[256];
//	HKEY hKey;
//	// Do not allow NULL or empty key name
//	if (pKeyName && lstrlenA(pKeyName))
//	{
//		if ((dwRtn = RegOpenKeyExA(hStartKey, pKeyName,
//		                          0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey))
//		        == ERROR_SUCCESS)
//		{
//			while (dwRtn == ERROR_SUCCESS)
//			{
//				dwSubKeyLength = 256;
//				dwRtn = RegEnumKeyExA(hKey, 0, szSubKey,	&dwSubKeyLength,
//				                     NULL, NULL, NULL, NULL);
//				if (dwRtn == ERROR_NO_MORE_ITEMS)
//				{
//					dwRtn = RegDeleteKeyA(hStartKey, pKeyName);
//					break;
//				}
//				else
//					if (dwRtn == ERROR_SUCCESS)
//						dwRtn = RegDeleteKeyNT(hKey, szSubKey);
//			}
//			RegCloseKey(hKey);
//			// Do not save return code because error
//			// has already occurred
//		}
//	}
//	else
//		dwRtn = ERROR_BADKEY;
//	return dwRtn;
//}
//
//bool MCS_delete_registry(MCStringRef p_key, MCStringRef& r_error)
//{
//	MCAutoStringRef t_root, t_key;
//
//	if (!MCS_registry_split_key(p_key, &t_root, &t_key))
//		return false;
//
//	HKEY hkey = NULL;
//
//	if (MCStringGetLength(*t_key) == 0)
//	{
//		/* RESULT */ //MCresult->sets("no key");
//		return MCStringCreateWithCString("no key", r_error);
//	}
//
//	if (!MCS_registry_root_to_hkey(*t_root, hkey))
//	{
//		/* RESULT */ //MCresult->sets("bad key");
//		return MCStringCreateWithCString("bad key", r_error);
//	}
//
//	errno = RegDeleteKeyNT(hkey, MCStringGetCString(*t_key));
//	if (errno != ERROR_SUCCESS)
//	{
//		/* RESULT */ //MCresult->sets("could not delete key");
//		return MCStringCreateWithCString("could not delete key", r_error);
//	}
//	else
//	{
//		/* RESULT */ MCresult->clear(False);
//		r_error = nil;
//		return true;
//	}
//}

////////////////////////////////////////////////////////////////////////////////
