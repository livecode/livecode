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

#include "w32prefix.h"

#include "core.h"
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

enum reg_accessmode
{
	kRegAccessDefault,
	kRegAccessAs32,
	kRegAccessAs64
};

typedef struct
{ //struct for WIN registry
	const char *token;
	HKEY key;
	reg_accessmode mode;
}
reg_keytype;

static reg_keytype Regkeys[] = {  //WIN registry root keys struct
                                   {"HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT, kRegAccessDefault},
                                   {"HKEY_CURRENT_USER", HKEY_CURRENT_USER, kRegAccessDefault},
                                   {"HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE, kRegAccessDefault},
                                   {"HKEY_LOCAL_MACHINE_32", HKEY_LOCAL_MACHINE, kRegAccessAs32},
                                   {"HKEY_LOCAL_MACHINE_64", HKEY_LOCAL_MACHINE, kRegAccessAs64},
                                   {"HKEY_USERS", HKEY_USERS, kRegAccessDefault},
                                   {"HKEY_PERFORMANCE_DATA", HKEY_PERFORMANCE_DATA, kRegAccessDefault},
                                   {"HKEY_CURRENT_CONFIG", HKEY_CURRENT_CONFIG, kRegAccessDefault},
                                   {"HKEY_DYN_DATA", HKEY_DYN_DATA, kRegAccessDefault}
                               };

typedef struct
{
	const char *token;
	DWORD type;
}
reg_datatype;

static reg_datatype RegDatatypes[] = {  //WIN registry root keys struct
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

void MCS_list_registry(MCExecPoint& p_context)
{
	char *t_full_key;
	t_full_key = p_context . getsvalue() . clone();

	p_context . clear();

	char *t_root_key;
	t_root_key = t_full_key;
	
	char *t_child_key;
	t_child_key = strchr(t_full_key, '\\');
	if (t_child_key != NULL)
		*t_child_key++ = '\0';
	else
		t_child_key = NULL;

	uint2 i;
	MCString s = t_root_key;
	for (i = 0 ; i < ELEMENTS(Regkeys) ; i++)
		if (s == Regkeys[i].token)
			break;

	HKEY t_key;
	if (i >= ELEMENTS(Regkeys) || RegOpenKeyExA(Regkeys[i] . key, t_child_key, 0, KEY_READ, &t_key) != ERROR_SUCCESS)
	{
		MCresult -> sets("bad key");
		delete t_full_key;
		return;
	}

	DWORD t_index;
	t_index = 0;
	for(;;)
	{
		LONG t_result;
		char t_name[256];
		DWORD t_name_length;
		t_name_length = 256;
		t_result = RegEnumKeyExA(t_key, t_index, t_name, &t_name_length, NULL, NULL, NULL, NULL);
		if (t_result == ERROR_NO_MORE_ITEMS)
			break;
		p_context . concatchars(t_name, t_name_length, EC_RETURN, t_index == 0);
		t_index += 1;
	}

	RegCloseKey(t_key);
	delete t_full_key;
}

void MCS_query_registry(MCExecPoint &dest, const char** type)
{
	HKEY hkey;
	char *key = dest.getsvalue().clone();
	//key = full path info such as: HKEY_LOCAL_MACHINE\\Software\\..\\valuename

	dest.clear();

	/* get the value name, it is at the end of the key          */
	char *str = strrchr(key, '\\');
	if (str == NULL)
	{ //invalid key path specified
		MCresult->sets("no key");
		delete key;
		return;
	}
	//chop off the end and make str point to the begining of the value name
	*str ++ = '\0';
	char *VName = str; //VName now points to the name of the value to be queryed

	/* get the root key, it is at the begining of the key       */
	str = strchr(key, '\\');
	if (str != NULL) /* key != HKEY_ROOT\.mc    case */
		*str ++ = '\0';  //str now pointing to the begining of subkey

	/** find the matching root key with the root string  **/
	uint2 i;
	MCString s = key;
	for (i = 0 ; i < ELEMENTS(Regkeys) ; i++)
		if (s == Regkeys[i].token)
			break;

	DWORD t_access;
	t_access = KEY_READ;
	if (MCmajorosversion >= 0x0501)
	{
		if (Regkeys[i].mode == kRegAccessAs32)
			t_access |= KEY_WOW64_32KEY;
		else if (Regkeys[i].mode == kRegAccessAs64)
			t_access |= KEY_WOW64_64KEY;
	}

	if (i >= ELEMENTS(Regkeys)
	        || (RegOpenKeyExA(Regkeys[i].key, str, 0, t_access, &hkey)
	            != ERROR_SUCCESS))
	{
		MCresult->sets("bad key");
		delete key;
		return;
	}

	//determine the size of value buffer needed

	LONG err = 0;
	DWORD VType;                         //value type
	DWORD VBufLen = 1;                   //value buffer len
	char *VValue;                        //value value
	err = RegQueryValueExA(hkey, VName, 0, NULL /*&VType*/, NULL, &VBufLen);
	if (err == ERROR_SUCCESS || err == ERROR_MORE_DATA)
	{
		VValue = new char[VBufLen]; //alloc space for the key value
		if ((err = RegQueryValueExA(hkey, VName, 0, &VType, (LPBYTE)VValue,
		                           &VBufLen)) == ERROR_SUCCESS && VBufLen)
		{
			DWORD t_original_buflen;
			t_original_buflen = VBufLen;
			if (VType == REG_SZ || VType == REG_EXPAND_SZ || VType == REG_MULTI_SZ)
			{
				while(VBufLen > 0 && VValue[VBufLen - 1] == '\0')
					VBufLen -= 1;
				if (VType == REG_MULTI_SZ && VBufLen < t_original_buflen)
					VBufLen += 1;
			}
			dest.copysvalue(VValue, VBufLen); //VBufLen - (VType == REG_SZ ? 1 : 0));
		}

		// MW-2006-01-15: Memory Leak
		delete VValue;
	}
	else
	{
		errno = err;
		MCresult->sets("can't find key");
	}
	RegCloseKey(hkey);
	delete key;

	if (type != NULL)
		for (i = 0 ; i < ELEMENTS(RegDatatypes) ; i++)
			if (VType == RegDatatypes[i].type)
			{
				*type = RegDatatypes[i] . token;
				break;
			}

	MCresult -> clear();

	return;
}

void MCS_set_registry(const char *key, MCExecPoint &dest, char *type)
{
	HKEY hkey = NULL;
	DWORD keyState; //REG_CREATED_NEW_KEY or REG_OPENED_EXISTING_KEY

	/* get the root key string, it is at the begining of the key       */
	char *str = (char *)strchr(key, '\\');
	*str++ = '\0';  //str now pointing to the begining of subkey string

	/* find the matching key for the root key string                   */
	MCString s = key;
	uint1 i;
	for (i = 0 ; i < ELEMENTS(Regkeys) ; i++)
		if (s == Regkeys[i].token)
			break;
	if (i >= ELEMENTS(Regkeys))
	{
		MCresult->sets("bad key");
		dest.setboolean(False);
		return;
	}
	/* get the value name string, it is at the end of the key          */
	char *VName = strrchr(str, '\\');
	if (VName == NULL)
	{ //invalid key path specified
		MCresult->sets("bad key specified");
		dest.setboolean(False);
		return;
	}
	*VName++ = '\0';
	if (RegCreateKeyExA(Regkeys[i].key, str, 0, NULL, REG_OPTION_NON_VOLATILE,
	                   KEY_ALL_ACCESS, NULL, &hkey, &keyState) != ERROR_SUCCESS)
	{
		MCS_seterrno(GetLastError());
		MCresult->sets("can't create key");
		dest.setboolean(False);
		return;
	}
	if (dest.getsvalue().getlength() == 0)
	{//delete this value
		if ((errno = RegDeleteValueA(hkey, VName)) != ERROR_SUCCESS)
		{
			MCS_seterrno(GetLastError());
			MCresult->sets("can't delete value");
			dest.setboolean(False);
			RegCloseKey(hkey); //write data to registry
			return;
		}
	}
	else
	{
		DWORD VType = REG_SZ;
		if (type != NULL)
			for (i = 0 ; i < ELEMENTS(RegDatatypes) ; i++)
				if (!MCU_strncasecmp(type, RegDatatypes[i].token, strlen(type) + 1))
				{
					VType = RegDatatypes[i].type;
					break;
				}
		char *VValue = dest.getsvalue().clone();
		uint4 VLength = dest.getsvalue().getlength();
		if (RegSetValueExA(hkey, VName, 0, VType, (const BYTE *)VValue,
		                  VLength + (VType == REG_SZ ? 1 : 0)) != ERROR_SUCCESS)
		{
			MCS_seterrno(GetLastError());
			MCresult->sets("can't set value");
			dest.setboolean(False);
			delete VValue;
			return;
		}
		delete VValue;
	}
	MCresult->clear(False);
	dest.setboolean(True);
	RegCloseKey(hkey); //write data to registry
}

//WINNT does not delete registry entry if there are subkeys..this functions fixes that.
DWORD RegDeleteKeyNT(HKEY hStartKey, char *pKeyName)
{
	DWORD dwRtn, dwSubKeyLength;
	char *pSubKey = NULL;
	char szSubKey[256];
	HKEY hKey;
	// Do not allow NULL or empty key name
	if (pKeyName && lstrlenA(pKeyName))
	{
		if ((dwRtn = RegOpenKeyExA(hStartKey, pKeyName,
		                          0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey))
		        == ERROR_SUCCESS)
		{
			while (dwRtn == ERROR_SUCCESS)
			{
				dwSubKeyLength = 256;
				dwRtn = RegEnumKeyExA(hKey, 0, szSubKey,	&dwSubKeyLength,
				                     NULL, NULL, NULL, NULL);
				if (dwRtn == ERROR_NO_MORE_ITEMS)
				{
					dwRtn = RegDeleteKeyA(hStartKey, pKeyName);
					break;
				}
				else
					if (dwRtn == ERROR_SUCCESS)
						dwRtn = RegDeleteKeyNT(hKey, szSubKey);
			}
			RegCloseKey(hKey);
			// Do not save return code because error
			// has already occurred
		}
	}
	else
		dwRtn = ERROR_BADKEY;
	return dwRtn;
}

void MCS_delete_registry(const char *key, MCExecPoint &dest)
{
	HKEY hkey = NULL;

	/* get the root key string, it is at the begining of the key       */
	char *str = (char *)strchr(key, '\\');
	if (!str)
	{
		MCresult->sets("no key");
		dest.setboolean(False);
		return;
	}
	*str++ = '\0';  //str now pointing to the begining of subkey string

	/* find the matching key for the root key string                   */
	MCString s = key;
	uint1 i;
	for (i = 0 ; i < ELEMENTS(Regkeys) ; i++)
	{
		if (s == Regkeys[i].token)
			break;
	}
	if (i >= ELEMENTS(Regkeys))
	{
		MCresult->sets("bad key");
		dest.setboolean(False);
		return;
	}
	errno = RegDeleteKeyNT(Regkeys[i].key, str);
	if (errno != ERROR_SUCCESS)
	{
		MCresult->sets("could not delete key");
		dest.setboolean(False);
	}
	else
	{
		MCresult->clear(False);
		dest.setboolean(True);
	}
}

////////////////////////////////////////////////////////////////////////////////
