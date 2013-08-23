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

#include "prefix.h"

#include "parsedef.h"
#include "filedefs.h"
#include "globdefs.h"
#include "objdefs.h"

#include "system.h"

#include "execpt.h"
#include "globals.h"
#include "system.h"
#include "osspec.h"
#include "mcerror.h"
#include "util.h"
#include "mcio.h"
#include "stack.h"
#include "handler.h"
#include "dispatch.h"
#include "card.h"
#include "group.h"
#include "button.h"
#include "param.h"
#include "mode.h"
#include "securemode.h"
#include "text.h"
#include "notify.h"

#include <sys/stat.h>
#include <time.h>
#include <Winsock2.h>
#include <Mstcpip.h>
#include <Mswsock.h>
#include <Shlobj.h>
#include <sys\Timeb.h>
#include <Iphlpapi.h>
#include <process.h>
#include <io.h> 


//////////////////////////////////////////////////////////////////////////////////

int *g_mainthread_errno;

//////////////////////////////////////////////////////////////////////////////////

#define PATH_DELIMITER '\\'

//////////////////////////////////////////////////////////////////////////////////

//WINNT does not delete registry entry if there are subkeys..this functions fixes that.
DWORD RegDeleteKeyNT(HKEY hStartKey, const char *pKeyName)
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

//////////////////////////////////////////////////////////////////////////////////

extern Boolean wsainit();

extern bool MCFiltersUrlEncode(MCStringRef p_source, MCStringRef& r_result);
extern bool MCStringsSplit(MCStringRef p_string, codepoint_t p_separator, MCStringRef*&r_strings, uindex_t& r_count);

//////////////////////////////////////////////////////////////////////////////////

static bool read_blob_from_pipe(HANDLE p_pipe, void*& r_data, uint32_t& r_data_length)
{
	uint32_t t_amount;
	DWORD t_read;
	if (!ReadFile(p_pipe, &t_amount, sizeof(uint32_t), &t_read, NULL) ||
		t_read != sizeof(uint32_t))
		return false;

	void *t_buffer;
	t_buffer = malloc(t_amount);
	if (t_buffer == nil)
		return false;

	if (!ReadFile(p_pipe, t_buffer, t_amount, &t_read, NULL) ||
		t_read != t_amount)
		return false;

	r_data = t_buffer;
	r_data_length = t_amount;

	return true;
}

static bool write_blob_to_pipe(HANDLE p_pipe, uint32_t p_count, const void *p_data)
{
	DWORD t_written;
	if (!WriteFile(p_pipe, &p_count, sizeof(p_count), &t_written, NULL) ||
		t_written != 4)
		return false;
	if (!WriteFile(p_pipe, p_data, p_count, &t_written, NULL) ||
		t_written != p_count)
		return false;
	return true;
}

// MW-2010-05-11: This call is only present on Vista and above, which is the place we
//   need it - so weakly bind.
typedef DWORD (APIENTRY *GetProcessIdOfThreadPtr)(HANDLE thread);
static DWORD DoGetProcessIdOfThread(HANDLE p_thread)
{
	// We can safely assume that the kernel dll is loaded!
	HMODULE t_module;
	t_module = GetModuleHandleA("Kernel32.dll");
	if (t_module == NULL)
		return -1;

	// Resolve the symbol
	void *t_ptr;
	t_ptr = GetProcAddress(t_module, "GetProcessIdOfThread");
	if (t_ptr == NULL)
		return -1;

	// Call it
	return ((GetProcessIdOfThreadPtr)t_ptr)(p_thread);
}

//////////////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////////////
static MMRESULT tid;

void CALLBACK MCS_tp(UINT id, UINT msg, DWORD user, DWORD dw1, DWORD dw2)
{
	MCalarm = True;
}

//////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	MCNameRef *token;
	uint4 winfolder;
}
sysfolders;

// need to add at least a temp folder and the system folder here
static sysfolders sysfolderlist[] = {
    {&MCN_desktop, CSIDL_DESKTOP},
    {&MCN_fonts, CSIDL_FONTS},
    {&MCN_documents, CSIDL_PERSONAL},
    {&MCN_start, CSIDL_STARTMENU},
    {&MCN_system, CSIDL_WINDOWS},
    {&MCN_home, CSIDL_PROFILE}, //TS-2007-08-20 Added so that "home" gives something useful across all platforms
    {&MCN_support, CSIDL_APPDATA},
};

bool MCS_specialfolder_to_csidl(MCNameRef p_folder, MCNumberRef& r_csidl)
{
	for (uindex_t i = 0 ; i < ELEMENTS(sysfolderlist) ; i++)
	{
		if (MCNameIsEqualTo(p_folder, *(sysfolderlist[i].token)))
		{
			return MCNumberCreateWithUnsignedInteger(sysfolderlist[i].winfolder, r_csidl);
		}
	}
    
	return false;
}

//////////////////////////////////////////////////////////////////////////////////

/* thread created to read data from child process's pipe */
static bool s_finished_reading = false;

static void readThreadDone(void *param)
{
	s_finished_reading = true;
}

static DWORD readThread(Streamnode *process)
{
	IO_handle ihandle;
	ihandle = process -> ihandle;

	DWORD nread;
	ihandle->buffer = new char[READ_PIPE_SIZE];
	uint4 bsize = READ_PIPE_SIZE;
	ihandle->ioptr = ihandle->buffer;   //set ioptr member
	ihandle->len = 0; //set len member
	while (ihandle->fhandle != NULL)
	{
		if (!PeekNamedPipe(ihandle->fhandle, NULL, 0, NULL, &nread, NULL))
			break;
		if (nread == 0)
		{
			if (WaitForSingleObject(process -> phandle, 0) != WAIT_TIMEOUT)
				break;
			Sleep(100);
			continue;
		}
		if (ihandle->len + nread >= bsize)
		{
			uint4 newsize = ihandle->len + nread + READ_PIPE_SIZE;
			MCU_realloc(&ihandle->buffer, bsize, newsize, 1);
			bsize = newsize;
			ihandle->ioptr = ihandle->buffer;
		}
		if (!ReadFile(ihandle->fhandle, (LPVOID)&ihandle->buffer[ihandle->len],
		              nread, &nread, NULL)
		        || nread == 0)
		{
			ihandle->len += nread;
			break;
		}
		ihandle->len += nread;
		ihandle->flags = 0;
		if (ihandle->ioptr != ihandle->buffer)
		{
			ihandle->len -= ihandle->ioptr - ihandle->buffer;
			memcpy(ihandle->buffer, ihandle->ioptr, ihandle->len);
			ihandle->ioptr = ihandle->buffer;
		}
	}
	MCNotifyPush(readThreadDone, nil, false, false);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////
// OK-2009-01-28: [[Bug 7452]] - SpecialFolderPath not working with redirected My Documents folder on Windows.
bool MCS_getlongfilepath(MCStringRef p_short_path, MCStringRef& r_long_path)
{
	// If the path conversion was not succesful, then
	// we revert back to using the original path.
	MCAutoStringRef t_long_path;
	if (MCS_longfilepath(p_short_path, &t_long_path) && MCStringGetLength(*t_long_path) > 0)
	{
		r_long_path = MCValueRetain(*t_long_path);
		return true;
	}

	return MCStringCopy(p_short_path, r_long_path);
}

bool MCS_getcurdir_native(MCStringRef& r_path)
{
	MCAutoNativeCharArray t_buffer;
	DWORD t_path_len = GetCurrentDirectoryA(0, NULL);
	if (t_path_len == 0 || !t_buffer.New(t_path_len))
		return false;
	if (t_path_len - 1 != GetCurrentDirectoryA(t_path_len, (LPSTR)t_buffer.Chars()))
		return false;

	t_buffer.Shrink(t_path_len - 1);
	return t_buffer.CreateStringAndRelease(r_path);
}

bool MCS_native_path_exists(MCStringRef p_path, bool p_is_file)
{
	// MW-2010-10-22: [[ Bug 8259 ]] Use a proper Win32 API function - otherwise network shares don't work.
	DWORD t_attrs;
	t_attrs = GetFileAttributesA(MCStringGetCString(p_path));

	if (t_attrs == INVALID_FILE_ATTRIBUTES)
		return false;

	return p_is_file == ((t_attrs & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

bool MCS_path_exists(MCStringRef p_path, bool p_is_file)
{
#ifdef /* MCS_exists_dsk_w32 */ LEGACY_SYSTEM
	MCAutoStringRef t_resolved;
	// MW-2004-04-20: [[ Purify ]] If *newpath == 0 then we should return False
	if (!MCS_resolvepath(p_path, &t_resolved) || MCStringGetLength(*t_resolved) == 0)
		return false;
    
	// MW-2008-01-15: [[ Bug 4981 ]] - It seems that stat will fail for checking
	//   a folder 'C:' and requires that it be 'C:\'
	if (MCStringGetLength(*t_resolved) == 2 && MCStringGetCharAtIndex(*t_resolved, 1) == ':')
	{
		MCAutoStringRef t_drive_string;
		return MCStringMutableCopy(*t_resolved, &t_drive_string) &&
        MCStringAppendChar(*t_drive_string, '\\') &&
        MCS_native_path_exists(*t_drive_string, p_is_file);
	}
    
	// OK-2007-12-05 : Bug 5555, modified to allow paths with trailing backslashes on Windows.
	uindex_t t_path_len = MCStringGetLength(*t_resolved);
	char_t t_last_char = MCStringGetNativeCharAtIndex(*t_resolved, t_path_len - 1);
	if ((t_last_char == '\\' || t_last_char == '/') &&
		!(t_path_len == 3 && MCStringGetNativeCharAtIndex(*t_resolved, 1) == ':'))
	{
		MCAutoStringRef t_trimmed_string;
		return MCStringCopySubstring(*t_resolved, MCRangeMake(0, t_path_len - 1), &t_trimmed_string) &&
        MCS_native_path_exists(*t_trimmed_string, p_is_file);
	}
    
	return MCS_native_path_exists(*t_resolved, p_is_file);
#endif /* MCS_exists_dsk_w32 */
    // MW-2004-04-20: [[ Purify ]] If *newpath == 0 then we should return False
    if (MCStringGetLength(p_path) == 0)
        return False;
    
    // MW-2008-01-15: [[ Bug 4981 ]] - It seems that stat will fail for checking
    //   a folder 'C:' and requires that it be 'C:\'
    if (MCStringGetLength(p_path) == 2 && MCStringGetCharAtIndex(p_path, 1) == ':')
    {
        MCAutoStringRef t_drive_string;
        return MCStringMutableCopy(p_path, &t_drive_string) &&
        MCStringAppendChar(*t_drive_string, '\\') &&
        MCS_native_path_exists(*t_drive_string, p_is_file);
    }
    
    // OK-2007-12-05 : Bug 5555, modified to allow paths with trailing backslashes on Windows.
    uindex_t t_path_len = MCStringGetLength(p_path);
    char_t t_last_char = MCStringGetNativeCharAtIndex(p_path, t_path_len - 1);
    if ((t_last_char == '\\' || t_last_char == '/') &&
        !(t_path_len == 3 && MCStringGetNativeCharAtIndex(p_path, 1) == ':'))
    {
        MCAutoStringRef t_trimmed_string;
        return MCStringCopySubstring(p_path, MCRangeMake(0, t_path_len - 1), &t_trimmed_string) &&
        MCS_native_path_exists(*t_trimmed_string, p_is_file);
    }
    
    return MCS_native_path_exists(p_path, p_is_file);
	// MW-2010-10-22: [[ Bug 8259 ]] Use a proper Win32 API function - otherwise network shares don't work.
	DWORD t_attrs;
	t_attrs = GetFileAttributesA(MCStringGetCString(p_path));
    
	if (t_attrs == INVALID_FILE_ATTRIBUTES)
		return false;
    
	return p_is_file == ((t_attrs & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

bool MCS_path_append(MCStringRef p_base, unichar_t p_separator, MCStringRef p_component, MCStringRef& r_path)
{
	MCAutoStringRef t_path;
	if (!MCStringMutableCopy(p_base, &t_path))
		return false;
    
	if (MCStringGetCharAtIndex(p_base, MCStringGetLength(p_base) - 1) != p_separator &&
		!MCStringAppendChars(*t_path, &p_separator, 1))
		return false;
    
	return MCStringAppend(*t_path, p_component) && MCStringCopy(*t_path, r_path);
}

//////////////////////////////////////////////////////////////////////////////////

bool dns_servers_from_network_params(MCListRef& r_list)
{
	MCAutoListRef t_list;
	ULONG t_buffer_size = 0;
	MCAutoBlock<byte_t> t_buffer;
    
	FIXED_INFO *t_fixed_info = nil;
    
	errno = GetNetworkParams(t_fixed_info, &t_buffer_size);
	if (errno == ERROR_NO_DATA)
	{
		r_list = MCValueRetain(kMCEmptyList);
		return true;
	}
	else if (errno != ERROR_BUFFER_OVERFLOW)
		return false;
    
	if (!t_buffer.Allocate(t_buffer_size))
		return false;
    
	if (!MCListCreateMutable('\n', &t_list))
		return false;
    
	t_fixed_info = (FIXED_INFO*)*t_buffer;
	MCMemoryClear(t_fixed_info, t_buffer_size);
    
	errno = GetNetworkParams(t_fixed_info, &t_buffer_size);
    
	if (errno == ERROR_SUCCESS)
	{
		IP_ADDR_STRING *t_addr_string = &t_fixed_info->DnsServerList;
		while (t_addr_string != nil && t_addr_string->IpAddress.String[0] != '\0')
		{
			if (!MCListAppendCString(*t_list, t_addr_string->IpAddress.String))
				return false;
			t_addr_string = t_addr_string->Next;
		}
	}
    
	return MCListCopy(*t_list, r_list);
}

#define NAMESERVER_REG_KEY "HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\Services\\Tcpip\\Parameters\\NameServer"
bool dns_servers_from_registry(MCListRef& r_list)
{
	MCAutoStringRef t_key, t_value, t_type, t_error;
	if (!MCStringCreateWithCString(NAMESERVER_REG_KEY, &t_key))
		return false;
	if (!MCS_query_registry(*t_key, &t_value, &t_type, &t_error))
		return false;
    
	if (*t_error == nil)
	{
		r_list = MCValueRetain(kMCEmptyList);
		return true;
	}
    
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;
    
	const char_t *t_chars;
	uindex_t t_char_count;
	t_chars = MCStringGetNativeCharPtr(*t_value);
	t_char_count = MCStringGetLength(*t_value);
    
	uindex_t t_start = 0;
    
	for (uindex_t i = 0; i < t_char_count; i++)
	{
		if (t_chars[i] == ' ' || t_chars[i] == ',' || t_chars[i] == '\n')
		{
			if (!MCListAppendNativeChars(*t_list, t_chars + t_start, i - t_start))
				return false;
			t_start = i + 1;
		}
	}
	if (t_start < t_char_count)
	{
		if (!MCListAppendNativeChars(*t_list, t_chars + t_start, t_char_count - t_start))
			return false;
	}
    
	return MCListCopy(*t_list, r_list);
}

//////////////////////////////////////////////////////////////////////////////////

struct MCWindowsSystemService: public MCWindowsSystemServiceInterface
{
    virtual bool MCISendString(MCStringRef p_command, MCStringRef& r_result, bool& r_error)
    {
        DWORD t_error_code;
        char t_buffer[256];
        t_buffer[0] = '\0';
        t_error_code = mciSendStringA(MCStringGetCString(p_command), t_buffer, 255, NULL);
        r_error = t_error_code != 0;
        if (!r_error)
            return MCStringCreateWithCString(t_buffer, r_result);
        
        return mciGetErrorStringA(t_error_code, t_buffer, 255) &&
		MCStringCreateWithCString(t_buffer, r_result);
    }
    
    virtual bool QueryRegistry(MCStringRef p_key, MCStringRef& r_value, MCStringRef& r_type, MCStringRef& r_error)
    {
        MCAutoStringRef t_root, t_key, t_value;
        
        r_value = r_error = nil;
        
        //key = full path info such as: HKEY_LOCAL_MACHINE\\Software\\..\\valuename
		if (!MCS_registry_split_key(p_key, &t_root, &t_key, &t_value))
            return false;
        
        if (*t_value == nil)
        {
            /* RESULT */ //MCresult->sets("no key");
            return MCStringCreateWithCString("no key", r_error);
        }
        
        if (*t_key == nil)
            t_key = kMCEmptyString;
        
        HKEY t_hkey;
        MCAutoRegistryKey t_regkey;
        
        uint32_t t_access_mode = KEY_READ;
        
		if (!MCS_registry_root_to_hkey(*t_root, t_hkey, t_access_mode) ||
            (RegOpenKeyExA(t_hkey, MCStringGetCString(*t_key), 0, t_access_mode, &t_regkey) != ERROR_SUCCESS))
        {
            /* RESULT */ //MCresult->sets("bad key");
            return MCStringCreateWithCString("bad key", r_error);
        }
        
        LONG err = 0;
        MCAutoNativeCharArray t_buffer;
        DWORD t_buffer_len = 0;
        DWORD t_type;
        
        //determine the size of value buffer needed
        err = RegQueryValueExA(*t_regkey, MCStringGetCString(*t_value), 0, NULL, NULL, &t_buffer_len);
        if (err == ERROR_SUCCESS || err == ERROR_MORE_DATA)
        {
            if (!t_buffer.New(t_buffer_len))
                return false;
            if ((err = RegQueryValueExA(*t_regkey, MCStringGetCString(*t_value), 0, &t_type,
                                        (LPBYTE)t_buffer.Chars(), &t_buffer_len)) == ERROR_SUCCESS && t_buffer_len)
            {
                DWORD t_len;
                t_len = t_buffer_len;
                if (t_type == REG_SZ || t_type == REG_EXPAND_SZ || t_type == REG_MULTI_SZ)
                {
                    char_t *t_chars = t_buffer.Chars();
                    while(t_len > 0 && t_chars[t_len - 1] == '\0')
                        t_len -= 1;
                    if (t_type == REG_MULTI_SZ && t_len < t_buffer_len)
                        t_len += 1;
                }
                
                t_buffer.Shrink(t_len);
                return t_buffer.CreateStringAndRelease(r_value) &&
					MCS_registry_type_to_string(t_type, r_type);
            }
        }
        else
        {
            errno = err;
            /* RESULT */ //MCresult->sets("can't find key");
            return MCStringCreateWithCString("can't find key", r_error);
        }
        
        r_value = MCValueRetain(kMCEmptyString);
        return true;
    }
    
    virtual bool SetRegistry(MCStringRef p_key, MCStringRef p_value, MCStringRef p_type, MCStringRef& r_error)
    {
        MCAutoStringRef t_root, t_key, t_value;
        HKEY t_root_hkey;
        
		if (!MCS_registry_split_key(p_key, &t_root, &t_key, &t_value))
            return false;
        
        if (!MCS_registry_root_to_hkey(*t_root, t_root_hkey))
        {
            /* RESULT */ //MCresult->sets("bad key");
            return MCStringCreateWithCString("bad key", r_error);
        }
        if (*t_key == nil)
        {
            /* RESULT */ //MCresult->sets("bad key specified");
            return MCStringCreateWithCString("bad key specified", r_error);
        }
        
        MCAutoRegistryKey t_regkey;
        DWORD t_keystate;
        
        if (RegCreateKeyExA(t_root_hkey, MCStringGetCString(*t_key), 0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &t_regkey, &t_keystate) != ERROR_SUCCESS)
        {
            MCS_seterrno(GetLastError());
            /* RESULT */ //MCresult->sets("can't create key");
            return MCStringCreateWithCString("can't create key", r_error);
        }
        
        if (MCStringGetLength(p_value) == 0)
        {//delete this value
            if ((errno = RegDeleteValueA(*t_regkey, MCStringGetCString(*t_value))) != ERROR_SUCCESS)
            {
                MCS_seterrno(GetLastError());
                /* RESULT */ //MCresult->sets("can't delete value");
                return MCStringCreateWithCString("can't delete value", r_error);
            }
        }
        else
        {
            DWORD t_type;
            t_type = MCS_registry_type_from_string(p_type);
            
            const BYTE *t_byte_ptr = MCStringGetBytePtr(p_value);
            uint32_t t_length = MCStringGetLength(p_value);
            if (t_type == REG_SZ && t_byte_ptr[t_length - 1] != '\0')
                t_length++;
            
            if (RegSetValueExA(*t_regkey, MCStringGetCString(*t_value), 0, t_type,
                               t_byte_ptr, t_length) != ERROR_SUCCESS)
            {
                MCS_seterrno(GetLastError());
                /* RESULT */ //MCresult->sets("can't set value");
                return MCStringCreateWithCString("can't set value", r_error);
            }
        }
        
        r_error = nil;
        return true;
    }
    
    virtual bool DeleteRegistry(MCStringRef p_key, MCStringRef& r_error)
    {
        MCAutoStringRef t_root, t_key;
        
        if (!MCS_registry_split_key(p_key, &t_root, &t_key))
            return false;
        
        HKEY hkey = NULL;
        
        if (MCStringGetLength(*t_key) == 0)
        {
            /* RESULT */ //MCresult->sets("no key");
            return MCStringCreateWithCString("no key", r_error);
        }
        
		if (!MCS_registry_root_to_hkey(*t_root, hkey))
        {
            /* RESULT */ //MCresult->sets("bad key");
            return MCStringCreateWithCString("bad key", r_error);
        }
        
        errno = RegDeleteKeyNT(hkey, MCStringGetCString(*t_key));
        if (errno != ERROR_SUCCESS)
        {
            /* RESULT */ //MCresult->sets("could not delete key");
            return MCStringCreateWithCString("could not delete key", r_error);
        }
        else
        {
            /* RESULT */ MCresult->clear(False);
            r_error = nil;
            return true;
        }
    }
    
    virtual bool ListRegistry(MCStringRef p_path, MCListRef& r_list, MCStringRef& r_error)
    {
        MCAutoStringRef t_root, t_key;
        HKEY t_hkey;
        MCAutoRegistryKey t_regkey;
        
        if (!MCS_registry_split_key(p_path, &t_root, &t_key))
            return false;
        if (!MCS_registry_root_to_hkey(*t_root, t_hkey) ||
            RegOpenKeyExA(t_hkey, MCStringGetCString(*t_key), 0, KEY_READ, &t_regkey) != ERROR_SUCCESS)
        {
            return MCStringCreateWithCString("bad key", r_error);
        }
        
        MCAutoListRef t_list;
        if (!MCListCreateMutable('\n', &t_list))
            return false;
        
        DWORD t_max_key_length;
        if (ERROR_SUCCESS != RegQueryInfoKeyA(*t_regkey, NULL, NULL, NULL, NULL, &t_max_key_length, NULL, NULL, NULL, NULL, NULL, NULL))
            return false;
        
        DWORD t_index = 0;
        MCAutoArray<char> t_buffer;
        
        t_max_key_length++;
        if (!t_buffer.New(t_max_key_length))
            return false;
        
        LONG t_result = ERROR_SUCCESS;
        while (t_result == ERROR_SUCCESS)
        {
            DWORD t_name_length = t_max_key_length;
            t_result = RegEnumKeyExA(*t_regkey, t_index, t_buffer.Ptr(), &t_name_length, NULL, NULL, NULL, NULL);
            if (t_result == ERROR_SUCCESS && !MCListAppendCString(*t_list, t_buffer.Ptr()))
                return false;
            
            t_index += 1;
        }
        
        if (t_result != ERROR_NO_MORE_ITEMS)
            return false;
        
        r_error = nil;
        return MCListCopy(*t_list, r_list);
    }
    
    virtual void ResetTime()
    {
    }
    
    ////////////////////////////////////////////////////
    //  Windows-specific functions (using Windows-specific parameters)
    ////////////////////////////////////////////////////
    
    virtual bool GetCurrentDirNative()
    {
    }

    virtual bool GetLongFilePath(MCStringRef& r_path)
    {
    }

    virtual Boolean NativePathExists()
    {
    }

    virtual bool PathAppend(MCStringRef p_path)
    {
    }
    virtual bool SpecialFolderToCSIDL(MCStringRef p_folder, MCStringRef& CSIDL)
    {
    }
    //MCS_windows_elevation_bootstrap_main
};

// MW-2005-02-22: Make this global for now so it is accesible by opensslsocket.cpp
real8 curtime;
static real8 starttime;
static DWORD startcount;

struct MCWindowDesktop: public MCSystemInterface, public MCWindowsSystemService
{
    virtual MCServiceInterface *QueryService(MCServiceType p_type)
    {
        if (p_type == kMCServiceTypeWindowsSystem)
            return (MCWindowsSystemServiceInterface *)this;
        return nil;
    }
    
	virtual bool Initialize(void) = 0;
	virtual void Finalize(void)
    {
#ifdef /* MCS_shutdown_dsk_w32 */ LEGACY_SYSTEM
	OleUninitialize();
#endif /* MCS_shutdown_dsk_w32 */
        OleUninitialize();
    }
	
	virtual void Debug(MCStringRef p_string) = 0;
    
	virtual real64_t GetCurrentTime()
    {
#ifdef /* MCS_time_dsk_w32 */ LEGACY_SYSTEM
	if (startcount)
	{
		DWORD newcount = timeGetTime();
		if (newcount < startcount)
			startcount = newcount;
		else
		{
			curtime = starttime + (newcount - startcount) / 1000.0;
			return curtime;
		}
	}
	struct _timeb timebuffer;
	_ftime(&timebuffer);
	starttime = timebuffer.time + timebuffer.millitm / 1000.0;
	return starttime;
        return starttime;
#endif /* MCS_time_dsk_w32 */
        if (startcount)
        {
            DWORD newcount = timeGetTime();
            if (newcount < startcount)
                startcount = newcount;
            else
            {
                curtime = starttime + (newcount - startcount) / 1000.0;
                return curtime;
            }
        }
        struct _timeb timebuffer;
        _ftime(&timebuffer);
        starttime = timebuffer.time + timebuffer.millitm / 1000.0;
    }
    
    virtual void ResetTime(void)
    {
#ifdef /* MCS_reset_time_dsk_w32 */ LEGACY_SYSTEM
	if (!MClowrestimers)
	{
		startcount = 0;
		MCS_time();
		startcount = timeGetTime();
	}
#endif /* MCS_reset_time_dsk_w32 */
        if (!MClowrestimers)
        {
            startcount = 0;
            MCS_time();
            startcount = timeGetTime();
        }
    }
    
	virtual bool GetVersion(MCStringRef& r_string)
    {
#ifdef /* MCS_getsystemversion_dsk_w32 */ LEGACY_SYSTEM
	return MCStringFormat(r_string, "NT %d.%d", (MCmajorosversion >> 8) & 0xFF, MCmajorosversion & 0xFF);
#endif /* MCS_getsystemversion_dsk_w32 */
        return MCStringFormat(r_string, "NT %d.%d", (MCmajorosversion >> 8) & 0xFF, MCmajorosversion & 0xFF);
    }
    
	virtual bool GetMachine(MCStringRef& r_string)
    {
#ifdef /* MCS_getmachine_dsk_w32 */ LEGACY_SYSTEM
	return MCStringCopy(MCNameGetString(MCN_x86), r_string);
#endif /* MCS_getmachine_dsk_w32 */
        return MCStringCopy(MCNameGetString(MCN_x86), r_string);
    }
    
	virtual MCNameRef GetProcessor(void)
    {
#ifdef /* MCS_getprocessor_dsk_w32 */ LEGACY_SYSTEM
	return MCN_x86;
#endif /* MCS_getprocessor_dsk_w32 */
        return MCN_x86;
    }
    
	virtual bool GetAddress(MCStringRef& r_address)
    {
#ifdef /* MCS_getaddress_dsk_w32 */ LEGACY_SYSTEM
	if (!wsainit())
		return MCStringCreateWithCString("unknown", r_address);

	char *buffer = new char[MAXHOSTNAMELEN + 1];
	gethostname(buffer, MAXHOSTNAMELEN);
	return MCStringFormat(r_address, "%s:&d", buffer, MCcmd);
#endif /* MCS_getaddress_dsk_w32 */
        if (!wsainit())
            return MCStringCreateWithCString("unknown", r_address);
        
        char *buffer = new char[MAXHOSTNAMELEN + 1];
        gethostname(buffer, MAXHOSTNAMELEN);
        return MCStringFormat(r_address, "%s:&d", buffer, MCcmd);
    }
    
	virtual uint32_t GetProcessId(void)
    {
#ifdef /* MCS_getpid_dsk_w32 */ LEGACY_SYSTEM
	return _getpid();
#endif /* MCS_getpid_dsk_w32 */
        return _getpid();
    }
	
	virtual void Alarm(real64_t p_when)
    {
#ifdef /* MCS_alarm_dsk_w32 */ LEGACY_SYSTEM
    //no action
	if (!MCnoui)
		if (secs == 0)
		{
			if (tid != 0)
			{
				timeKillEvent(tid);
				tid = 0;
			}
		}
		else
			if (tid == 0)
				tid = timeSetEvent((UINT)(secs * 1000.0), 100, MCS_tp,
				                   0, TIME_PERIODIC);
#endif /* MCS_alarm_dsk_w32 */
        //no action
        if (!MCnoui)
            if (p_when == 0)
            {
                if (tid != 0)
                {
                    timeKillEvent(tid);
                    tid = 0;
                }
            }
            else
                if (tid == 0)
                    tid = timeSetEvent((UINT)(p_when * 1000.0), 100, MCS_tp,
                                       0, TIME_PERIODIC);
    }
    
	virtual void Sleep(real64_t p_when)
    {
#ifdef /* MCS_sleep_dsk_w32 */ LEGACY_SYSTEM
	Sleep((DWORD)(delay * 1000.0));  //takes milliseconds as parameter
#endif /* MCS_sleep_dsk_w32 */
        Sleep((DWORD)(p_when * 1000.0));  //takes milliseconds as parameter
    }
	
	virtual void SetEnv(MCStringRef p_name, MCStringRef p_value)
    {
#ifdef /* MCS_setenv_dsk_w32 */ LEGACY_SYSTEM
	char *dptr = new char[strlen(name) + strlen(value) + 2];
	sprintf(dptr, "%s=%s", name, value);
	_putenv(dptr);

	// MW-2005-10-29: Memory leak
	delete[] dptr;
#endif /* MCS_setenv_dsk_w32 */
#ifdef /* MCS_unsetenv_dsk_w32 */ LEGACY_SYSTEM
	char *dptr = new char[strlen(name) + 2];
	sprintf(dptr, "%s=", name);
	_putenv(dptr);

	// MW-2005-10-29: Memory leak
	delete[] dptr;
#endif /* MCS_unsetenv_dsk_w32 */
        MCAutoStringRef t_env;
        if (p_value != nil)
            MCStringFormat(&t_env, "%x=%x", p_name, p_value);
        else
            MCStringFormat(&t_env, "%x=", p_name);
        
        _putenv(MCStringGetCString(*t_env));
    }
                
	virtual bool GetEnv(MCStringRef p_name, MCStringRef& r_value)
    {
#ifdef /* MCS_getenv_dsk_w32 */ LEGACY_SYSTEM
	return getenv(name);
#endif /* MCS_getenv_dsk_w32 */
        return MCStringCreateWithCString(getenv(MCStringGetCString(p_name)), r_value);
    }
	
	virtual Boolean CreateFolder(MCStringRef p_path)
    {
#ifdef /* MCS_mkdir_dsk_w32 */ LEGACY_SYSTEM
	Boolean result = False;
	char *tpath = MCS_resolvepath(path);
	if (CreateDirectoryA(tpath, NULL))
		result = True;
	delete tpath;
	return result;
#endif /* MCS_mkdir_dsk_w32 */
        if (!CreateDirectoryA(MCStringGetCString(p_path), NULL))
            return False;
        
        return True;
    }
    
	virtual Boolean DeleteFolder(MCStringRef p_path)
    {
#ifdef /* MCS_rmdir_dsk_w32 */ LEGACY_SYSTEM
	Boolean result = False;
	char *tpath = MCS_resolvepath(path);
	if (RemoveDirectoryA(tpath))
		result = True;
	delete tpath;
	return result;
#endif /* MCS_rmdir_dsk_w32 */
        if (!RemoveDirectoryA(MCStringGetCString(p_path)))
            return False;
        
        return True;
    }
	
//	/* LEGACY */ virtual bool DeleteFile(const char *p_path)
//    {
//        Boolean done = remove(p_path) == 0;
//        if (!done)
//        { // bug in NT serving: can't delete full path from current dir
//            char dir[PATH_MAX];
//            GetCurrentDirectoryA(PATH_MAX, dir);
//            if (p[0] == '\\' && p[1] == '\\' && dir[0] == '\\' && dir[1] == '\\')
//            {
//                SetCurrentDirectoryA("C:\\");
//                done = remove(p_path) == 0;
//                SetCurrentDirectoryA(dir);
//            }
//        }
//        return done;
//    }
    
	virtual Boolean DeleteFile(MCStringRef p_path)
    {
#ifdef /* MCS_unlink_dsk_w32 */ LEGACY_SYSTEM
        char *p = MCS_resolvepath(path);
        Boolean done = remove(p) == 0;
        if (!done)
        { // bug in NT serving: can't delete full path from current dir
            char dir[PATH_MAX];
            GetCurrentDirectoryA(PATH_MAX, dir);
            if (p[0] == '\\' && p[1] == '\\' && dir[0] == '\\' && dir[1] == '\\')
            {
                SetCurrentDirectoryA("C:\\");
                done = remove(p) == 0;
                SetCurrentDirectoryA(dir);
            }
        }
        delete p;
        return done;
#endif /* MCS_unlink_dsk_w32 */
        Boolean done = remove(MCStringGetCString(p_path)) == 0;
        if (!done)
        { // bug in NT serving: can't delete full path from current dir
            char dir[PATH_MAX];
            GetCurrentDirectoryA(PATH_MAX, dir);
            if (MCStringGetNativeCharAtIndex(p_path, 0) == '\\' && MCStringGetNativeCharAtIndex(p_path, 1) == '\\' && dir[0] == '\\' && dir[1] == '\\')
            {
                SetCurrentDirectoryA("C:\\");
                done = remove(MCStringGetCString(p_path)) == 0;
                SetCurrentDirectoryA(dir);
            }
        }
        return done;
    }
	
	virtual Boolean RenameFileOrFolder(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_rename_dsk_w32 */ LEGACY_SYSTEM
	char *op = MCS_resolvepath(oldname);
	char *np = MCS_resolvepath(newname);
	Boolean done = rename(op, np) == 0;
	delete op;
	delete np;
	return done;
#endif /* MCS_rename_dsk_w32 */
        if (!rename(MCStringGetCString(p_old_name), MCStringGetCString(p_new_name)) == 0)
            return False;
        
        return True;
    }
	
	virtual Boolean BackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_backup_dsk_w32 */ LEGACY_SYSTEM
	return MCS_rename(oname, nname);
#endif /* MCS_backup_dsk_w32 */
        return RenameFileOrFolder(p_old_name, p_new_name);
    }
    
	virtual Boolean UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_unbackup_dsk_w32 */ LEGACY_SYSTEM
	return MCS_rename(oname, nname);
#endif /* MCS_unbackup_dsk_w32 */
        return MCS_rename(p_old_name, p_new_name);
    }
	
	virtual Boolean CreateAlias(MCStringRef p_target, MCStringRef p_alias)
    {
#ifdef /* MCS_createalias_dsk_w32 */ LEGACY_SYSTEM
	HRESULT err;
	char *source = MCS_resolvepath(srcpath);
	char *dest = MCS_resolvepath(dstpath);
	IShellLinkA *ISHLNKvar1;
	err = CoCreateInstance(CLSID_ShellLink, NULL,
	                       CLSCTX_INPROC_SERVER, IID_IShellLinkA,
	                       (void **)&ISHLNKvar1);
	if (SUCCEEDED(err))
	{
		IPersistFile *IPFILEvar1;
		if (source[1] != ':' && source[0] != '/')
		{
			char *tpath = MCS_getcurdir(); //prepend the current dir
			strcat(tpath, "/");
			strcat(tpath, source);
			delete source;
			MCU_path2native(tpath);
			source = tpath;
		}
		ISHLNKvar1->SetPath(source);
		char *buffer = strrchr( source, '\\' );
		if (buffer != NULL)
		{
			*(buffer+1) = '\0';
			ISHLNKvar1->SetWorkingDirectory(source);
		}
		err = ISHLNKvar1->QueryInterface(IID_IPersistFile, (void **)&IPFILEvar1);
		if (SUCCEEDED(err))
		{
			WORD DWbuffer[PATH_MAX];
			MultiByteToWideChar(CP_ACP, 0, dest, -1,
			                    (LPWSTR)DWbuffer, PATH_MAX);
			err = IPFILEvar1->Save((LPCOLESTR)DWbuffer, TRUE);
			IPFILEvar1->Release();
		}
		ISHLNKvar1->Release();
	}
	delete source;
	delete dest;
	return SUCCEEDED(err);
#endif /* MCS_createalias_dsk_w32 */
        HRESULT err;
        MCAutoStringRef t_src_path;
		t_src_path = p_target;
        IShellLinkA *ISHLNKvar1;
        err = CoCreateInstance(CLSID_ShellLink, NULL,
                               CLSCTX_INPROC_SERVER, IID_IShellLinkA,
                               (void **)&ISHLNKvar1);
        if (SUCCEEDED(err))
        {
            IPersistFile *IPFILEvar1;
            if (MCStringGetNativeCharAtIndex(*t_src_path, 1) != ':' && MCStringGetNativeCharAtIndex(*t_src_path, 0) != '/')
            {
                MCAutoStringRef t_path;
                /* UNCHECKED */ MCStringCreateMutable(0, &t_path);
                /* UNCHECKED */ GetCurrentFolder(&t_path); //prepend the current dir
                /* UNCHECKED */ MCStringAppendFormat(*t_path, "/%x", p_target);
                /* UNCHECKED */ MCU_path2native(*t_path, &t_src_path);
            }
            ISHLNKvar1->SetPath(MCStringGetCString(*t_src_path));
			char *t_src_path_char = strclone(MCStringGetCString(*t_src_path));
			char *buffer = strrchr(t_src_path_char, '\\' );
            if (buffer != NULL)
            {
                *(buffer+1) = '\0';
				ISHLNKvar1->SetWorkingDirectory(t_src_path_char);
            }
            err = ISHLNKvar1->QueryInterface(IID_IPersistFile, (void **)&IPFILEvar1);
            if (SUCCEEDED(err))
            {
                WORD DWbuffer[PATH_MAX];
                MultiByteToWideChar(CP_ACP, 0, MCStringGetCString(p_alias), -1,
                                    (LPWSTR)DWbuffer, PATH_MAX);
                err = IPFILEvar1->Save((LPCOLESTR)DWbuffer, TRUE);
                IPFILEvar1->Release();
            }
            ISHLNKvar1->Release();
			delete[] t_src_path_char;
        }
        return SUCCEEDED(err);
    }
    
	// NOTE: 'ResolveAlias' returns a standard (not native) path.
	virtual Boolean ResolveAlias(MCStringRef p_target, MCStringRef& r_dest)
    {
#ifdef /* MCS_resolvealias_dsk_w32 */ LEGACY_SYSTEM
	MCAutoStringRef t_resolved_path;
	
	if (!MCS_resolvepath(p_path, &t_resolved_path))
		return false;
	
	MCAutoNativeCharArray t_dest;
	if (!t_dest.New(PATH_MAX))
		return false;

	HRESULT hres;
	IShellLinkA* psl;
	WIN32_FIND_DATA wfd;
	t_dest.Chars()[0] = 0;
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
	                        IID_IShellLinkA, (LPVOID *) &psl);
	if (SUCCEEDED(hres))
	{
		IPersistFile* ppf;
		hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
		if (SUCCEEDED(hres))
		{
			WORD wsz[PATH_MAX];
			MultiByteToWideChar(CP_ACP, 0, MCStringGetCString(*t_resolved_path), -1, (LPWSTR)wsz, PATH_MAX);
			hres = ppf->Load((LPCOLESTR)wsz, STGM_READ);
			if (SUCCEEDED(hres))
			{
				hres = psl->Resolve(HWND_DESKTOP, SLR_ANY_MATCH|SLR_NO_UI|SLR_UPDATE);
				if (SUCCEEDED(hres))
				{
					hres = psl->GetPath((char*)t_dest.Chars(), PATH_MAX, (WIN32_FIND_DATAA *)&wfd,
					                    SLGP_SHORTPATH);
				}
			}
			ppf->Release();
		}
		psl->Release();
	}
	if (SUCCEEDED(hres))
	{
		MCAutoStringRef t_std_path;
		t_dest.Shrink(MCCStringLength((char*)t_dest.Chars()));
		return t_dest.CreateStringAndRelease(&t_std_path) && MCU_path2std(*t_std_path, r_resolved);
	}
	else
	{
		MCS_seterrno(GetLastError());
		return MCStringCreateWithCString("can't get", r_error);
	}
#endif /* MCS_resolvealias_dsk_w32 */
        MCAutoStringRef t_resolved_path;
        
        if (!ResolvePath(p_target, &t_resolved_path))
            return false;
        
        MCAutoNativeCharArray t_dest;
        if (!t_dest.New(PATH_MAX))
            return false;
        
        HRESULT hres;
        IShellLinkA* psl;
        WIN32_FIND_DATA wfd;
        t_dest.Chars()[0] = 0;
        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                IID_IShellLinkA, (LPVOID *) &psl);
        if (SUCCEEDED(hres))
        {
            IPersistFile* ppf;
            hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
            if (SUCCEEDED(hres))
            {
                WORD wsz[PATH_MAX];
                MultiByteToWideChar(CP_ACP, 0, MCStringGetCString(*t_resolved_path), -1, (LPWSTR)wsz, PATH_MAX);
                hres = ppf->Load((LPCOLESTR)wsz, STGM_READ);
                if (SUCCEEDED(hres))
                {
                    hres = psl->Resolve(HWND_DESKTOP, SLR_ANY_MATCH|SLR_NO_UI|SLR_UPDATE);
                    if (SUCCEEDED(hres))
                    {
                        hres = psl->GetPath((char*)t_dest.Chars(), PATH_MAX, (WIN32_FIND_DATAA *)&wfd,
                                            SLGP_SHORTPATH);
                    }
                }
                ppf->Release();
            }
            psl->Release();
        }
        if (SUCCEEDED(hres))
        {
            MCAutoStringRef t_std_path;
            t_dest.Shrink(MCCStringLength((char*)t_dest.Chars()));
            return t_dest.CreateStringAndRelease(&t_std_path) && MCU_path2std(*t_std_path, r_dest);
        }
        else
        {
            MCS_seterrno(GetLastError());
            //return MCStringCreateWithCString("can't get", r_error);
			MCresult -> sets("can't get");
			return true;
        }
    }
	
	virtual void GetCurrentFolder(MCStringRef& r_path)
    {
#ifdef /* MCS_getcurdir_dsk_w32 */ LEGACY_SYSTEM
	MCAutoStringRef t_path;
	return MCS_getcurdir_native(&t_path) &&
		MCU_path2std(*t_path, r_path);
#endif /* MCS_getcurdir_dsk_w32 */
        MCAutoStringRef t_path;
        MCS_getcurdir_native(&t_path);
		/* UNCHECKED */ MCU_path2std(*t_path, r_path);
    }
	///* LEGACY */ char *GetCurrentFolder(void);
    
	virtual Boolean SetCurrentFolder(MCStringRef p_path)
    {
#ifdef /* MCS_setcurdir_dsk_w32 */ LEGACY_SYSTEM
	MCAutoStringRef t_new_path;
	if (MCS_resolvepath(p_path, &t_new_path))
		return SetCurrentDirectoryA((LPCSTR)MCStringGetCString(*t_new_path)) == TRUE;

	return false;
#endif /* MCS_setcurdir_dsk_w32 */
        if (!SetCurrentDirectoryA((LPCSTR)MCStringGetCString(p_path)))
            return False;
            
        return True;
    }
	
	// NOTE: 'GetStandardFolder' returns a standard (not native) path.
	virtual Boolean GetStandardFolder(MCNameRef p_type, MCStringRef& r_folder)
    {
#ifdef /* MCS_getspecialfolder_dsk_w32 */ LEGACY_SYSTEM
	bool t_wasfound = false;
	uint32_t t_special_folder = 0;
	MCAutoStringRef t_native_path;
	if (MCStringIsEqualTo(p_special, MCNameGetString(MCN_temporary), kMCStringOptionCompareCaseless))
	{
		MCAutoNativeCharArray t_buffer;
		uindex_t t_length;
		t_length = GetTempPathA(0, NULL);
		if (t_length != 0)
		{
			if (!t_buffer.New(t_length))
				return false;
			t_length = GetTempPathA(t_length, (LPSTR)t_buffer.Chars());
			if (t_length != 0)
			{
				if (!t_buffer.CreateStringAndRelease(&t_native_path))
					return false;
				t_wasfound = true;
			}
		}
	}
	//else if (MCStringIsEqualTo(p_special, MCNameGetString(MCN_system), kMCStringOptionCompareCaseless))
	//{
		//char *buf;
		//ep.reserve(PATH_MAX, buf);
		//if (GetWindowsDirectoryA(buf, PATH_MAX))
		//{
		//	wasfound = True;
		//	ep.commit(strlen(buf));
		//}
	//}
	else
	{
		if (ctxt.ConvertToUnsignedInteger(p_special, t_special_folder) ||
			MCS_specialfolder_to_csidl(p_special, t_special_folder))
		{
			LPITEMIDLIST lpiil;
			MCAutoNativeCharArray t_buffer;
			if (!t_buffer.New(PATH_MAX))
				return false;

			t_wasfound = (SHGetSpecialFolderLocation(HWND_DESKTOP, t_special_folder, &lpiil) == 0) &&
				SHGetPathFromIDListA(lpiil, (LPSTR)t_buffer.Chars());

			CoTaskMemFree(lpiil);

			if (t_wasfound && !t_buffer.CreateStringAndRelease(&t_native_path))
				return false;
		}
	}
	if (t_wasfound)
	{
		MCAutoStringRef t_standard_path;
		// TS-2008-06-16: [[ Bug 6403 ]] - specialFolderPath() returns 8.3 paths
		// First we need to swap to standard path seperator
		if (!MCU_path2std(*t_native_path, &t_standard_path))
			return false;

		// OK-2009-01-28: [[Bug 7452]]
		// And call the function to expand the path - if cannot convert to a longfile path,
		// we should return what we already had!
		return MCS_getlongfilepath(*t_standard_path, r_path);
	}
	else
	{
		r_path = MCValueRetain(kMCEmptyString);
		return true;
	}
#endif /* MCS_getspecialfolder_dsk_w32 */
        bool t_wasfound = false;
        MCAutoNumberRef t_special_folder;
        MCAutoStringRef t_native_path;
        if (MCNameIsEqualTo(p_type, MCN_temporary))
        {
            MCAutoNativeCharArray t_buffer;
            uindex_t t_length;
            t_length = GetTempPathA(0, NULL);
            if (t_length != 0)
            {
                if (!t_buffer.New(t_length))
                    return false;
                t_length = GetTempPathA(t_length, (LPSTR)t_buffer.Chars());
                if (t_length != 0)
                {
                    if (!t_buffer.CreateStringAndRelease(&t_native_path))
                        return false;
                    t_wasfound = true;
                }
            }
        }
        //else if (MCStringIsEqualTo(p_special, MCNameGetString(MCN_system), kMCStringOptionCompareCaseless))
        //{
		//char *buf;
		//ep.reserve(PATH_MAX, buf);
		//if (GetWindowsDirectoryA(buf, PATH_MAX))
		//{
		//	wasfound = True;
		//	ep.commit(strlen(buf));
		//}
        //}
        else
        {
            if (MCNumberParse(MCNameGetString(p_type), &t_special_folder) ||
                MCS_specialfolder_to_csidl(p_type, &t_special_folder))
            {
                LPITEMIDLIST lpiil;
                MCAutoNativeCharArray t_buffer;
                if (!t_buffer.New(PATH_MAX))
                    return false;
                
                t_wasfound = (SHGetSpecialFolderLocation(HWND_DESKTOP, MCNumberFetchAsUnsignedInteger(*t_special_folder), &lpiil) == 0) &&
				SHGetPathFromIDListA(lpiil, (LPSTR)t_buffer.Chars());
                
                CoTaskMemFree(lpiil);
                
                if (t_wasfound && !t_buffer.CreateStringAndRelease(&t_native_path))
                    return false;
            }
        }
        if (t_wasfound)
        {
            MCAutoStringRef t_standard_path;
            // TS-2008-06-16: [[ Bug 6403 ]] - specialFolderPath() returns 8.3 paths
            // First we need to swap to standard path seperator
            if (!MCU_path2std(*t_native_path, &t_standard_path))
                return false;
            
            // OK-2009-01-28: [[Bug 7452]]
            // And call the function to expand the path - if cannot convert to a longfile path,
            // we should return what we already had!
            return MCS_getlongfilepath(*t_standard_path, r_folder);
        }
        else
        {
            r_folder = MCValueRetain(kMCEmptyString);
            return true;
        }
    }
	
    virtual real8 GetFreeDiskSpace() = 0;
    virtual Boolean GetDevices(MCStringRef& r_devices) = 0;
    virtual Boolean GetDrives(MCStringRef& r_drives) = 0;
    
	virtual Boolean FileExists(MCStringRef p_path)
    {
        return MCS_path_exists(p_path, True);
    }
    
	virtual Boolean FolderExists(MCStringRef p_path)
    {
        return MCS_path_exists(p_path, False);
    }
    
	virtual Boolean FileNotAccessible(MCStringRef p_path)
    {
#ifdef /* MCS_noperm_dsk_w32 */ LEGACY_SYSTEM
	struct stat buf;
	if (stat(path, &buf))
		return False;
	if (buf.st_mode & S_IFDIR)
		return True;
	if (!(buf.st_mode & _S_IWRITE))
		return True;
	return False;
#endif /* MCS_noperm_dsk_w32 */
        struct stat buf;
        if (stat(MCStringGetCString(p_path), &buf))
            return False;
        if (buf.st_mode & S_IFDIR)
            return True;
        if (!(buf.st_mode & _S_IWRITE))
            return True;
        return False;
    }
	
	virtual Boolean ChangePermissions(MCStringRef p_path, uint2 p_mask)
    {
#ifdef /* MCS_chmod_dsk_w32 */ LEGACY_SYSTEM
	if (_chmod(path, mask) != 0)
		return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_chmod_dsk_w32 */
        if (_chmod(MCStringGetCString(p_path), p_mask) != 0)
            return IO_ERROR;
        return IO_NORMAL;
    }
	virtual uint2 UMask(uint2 p_mask)
    {
#ifdef /* MCS_umask_dsk_w32 */ LEGACY_SYSTEM
	return _umask(mask);
#endif /* MCS_umask_dsk_w32 */
        return _umask(p_mask);
    }
	
	virtual IO_handle OpenFile(MCStringRef p_path, intenum_t p_mode, Boolean p_map, uint32_t p_offset)
    {
#ifdef /* MCS_open_dsk_w32 */ LEGACY_SYSTEM
	Boolean appendmode = False;
	DWORD omode = 0;		//file open mode
	DWORD createmode = OPEN_ALWAYS;
	DWORD fa = FILE_ATTRIBUTE_NORMAL; //file flags & attribute
	char *newpath = MCS_resolvepath(path);
	HANDLE hf = NULL;
	IO_handle handle;

	bool t_device = false;
	bool t_serial_device = false;

	// MW-2008-08-18: [[ Bug 6941 ]] Update device logic.
	//   To open COM<n> for <n> > 9 we need to use '\\.\COM<n>'.
	uint4 t_path_len;
	t_path_len = strlen(newpath);
	if (*newpath != '\0' && newpath[t_path_len - 1] == ':')
	{
		if (MCU_strncasecmp(newpath, "COM", 3) == 0)
		{
			// If the path length > 4 then it means it must have double digits so rewrite
			if (t_path_len > 4)
			{
				char *t_rewritten_path;
				t_rewritten_path = new char[t_path_len + 4 + 1];
				sprintf(t_rewritten_path, "\\\\.\\%s", newpath);
				delete newpath;
				newpath = t_rewritten_path;
				t_path_len += 4;
			}
			
			// Strictly speaking, we don't need the ':' at the end of the path, so we remove it.
			newpath[t_path_len - 1] = '\0';

			t_serial_device = true;
		}
		
		t_device = true;
	}

	if (strequal(mode, IO_READ_MODE))
	{
		omode = GENERIC_READ;
		createmode = OPEN_EXISTING;
	}
	if (strequal(mode, IO_WRITE_MODE))
	{
		omode = GENERIC_WRITE;
		createmode = CREATE_ALWAYS;
	}
	if (strequal(mode, IO_UPDATE_MODE))
		omode = GENERIC_WRITE | GENERIC_READ;
	if (strequal(mode, IO_APPEND_MODE))
	{
		omode = GENERIC_WRITE;
		appendmode = True;
	}

	DWORD sharemode;
	if (t_device)
	{
		createmode = OPEN_EXISTING;
		sharemode = 0;
	}
	else
		sharemode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	if ((hf = CreateFileA(newpath, omode, sharemode, NULL,
	                     createmode, fa, NULL)) == INVALID_HANDLE_VALUE)
	{
		delete newpath;
		return NULL;
	}
	delete newpath;

	if (t_serial_device)
	{
		DCB dcb;
		dcb . DCBlength = sizeof(DCB);
		if (!GetCommState(hf, &dcb) || !BuildCommDCBA(MCserialcontrolsettings, &dcb)
		        || !SetCommState(hf, &dcb))
		{
			MCS_seterrno(GetLastError());
			CloseHandle(hf);
			MCresult->sets("SetCommState error");
			return NULL;
		}
		COMMTIMEOUTS timeout;         //set timeout to prevent blocking
		memset(&timeout, 0, sizeof(COMMTIMEOUTS));
		timeout.ReadIntervalTimeout = MAXDWORD;
		timeout.WriteTotalTimeoutConstant = 2000;
		if (!SetCommTimeouts(hf, (LPCOMMTIMEOUTS)&timeout))
		{
			MCS_seterrno(GetLastError());
			CloseHandle(hf);
			MCresult->sets("SetCommTimeouts error");
			return NULL;
		}
		map = False;
	}

	handle = new IO_header((MCWinSysHandle)hf, NULL, 0, 0);

	if (appendmode) //if append mode, move file ptr to the end of file
		SetFilePointer(hf, 0, NULL, FILE_END);

	if (map && MCmmap && (omode == GENERIC_READ) //if memory map file
	        && (handle->mhandle = (MCWinSysHandle)CreateFileMappingA(hf, NULL, PAGE_READONLY,
	                                                0, 0, NULL)) != NULL)
	{
		handle->len = GetFileSize(hf, NULL);
		handle->buffer = (char*)MapViewOfFile(handle->mhandle,
		                                      FILE_MAP_READ, 0, 0, 0);
		handle->ioptr = handle->buffer;
		if (handle->buffer == NULL)
		{
			CloseHandle(handle->mhandle);
			if (offset != 0) //move file ptr to the offset position
				SetFilePointer(hf, offset, NULL, FILE_BEGIN);
		}
		else
			handle->ioptr += offset;
	}
	else
		if (offset != 0) //move file ptr to the offset position
			SetFilePointer(hf, offset, NULL, FILE_BEGIN);

	return handle;
#endif /* MCS_open_dsk_w32 */
    }
    
	virtual IO_handle OpenStdFile(uint32_t fd, intenum_t mode) = 0;
	virtual IO_handle OpenDevice(MCStringRef p_path, const char *p_control_string, uint32_t p_offset) = 0;
	
	// NOTE: 'GetTemporaryFileName' returns a native path.
	virtual void GetTemporaryFileName(MCStringRef& r_tmp_name)
    {
#ifdef /* MCS_tmpnam_dsk_w32 */ LEGACY_SYSTEM
	MCAutoStringRef t_path;
	MCAutoStringRef t_stdpath;
	MCAutoStringRef t_long_path;
	MCAutoPointer<char> t_fname;

	t_fname = _tempnam("\\tmp", "tmp");
	uindex_t t_length;

	const char *t_ptr = strrchr(*t_fname, '\\');
	if (t_ptr == nil)
		t_length = strlen(*t_fname);
	else
		t_length = t_ptr - *t_fname;

	if (!MCStringCreateWithNativeChars((const char_t*)*t_fname, t_length, &t_path) ||
		!MCU_path2std(*t_path, &t_stdpath) ||
		!MCS_longfilepath(*t_stdpath, &t_long_path))
		return false;

	if (t_ptr == nil)
		return MCStringCopy(*t_long_path, r_string);

	MCAutoStringRef t_tmp_name;
	return MCStringMutableCopy(*t_long_path, &t_tmp_name) &&
		MCStringAppendFormat(*t_tmp_name, "/%s", t_ptr + 1) &&
		MCStringCopy(*t_tmp_name, r_string);
#endif /* MCS_tmpnam_dsk_w32 */
        MCAutoStringRef t_path;
        MCAutoStringRef t_stdpath;
        MCAutoStringRef t_long_path;
        MCAutoPointer<char> t_fname;
        
        t_fname = _tempnam("\\tmp", "tmp");
        uindex_t t_length;
        
        const char *t_ptr = strrchr(*t_fname, '\\');
        if (t_ptr == nil)
            t_length = strlen(*t_fname);
        else
            t_length = t_ptr - *t_fname;
        
        if (!MCStringCreateWithNativeChars((const char_t*)*t_fname, t_length, &t_path) ||
            !MCU_path2std(*t_path, &t_stdpath) ||
            !LongFilePath(*t_stdpath, &t_long_path))
		{
			r_tmp_name = MCValueRetain(kMCEmptyString);
            return;
		}
        
        if (t_ptr == nil)
		{
			/* UNCHECKED */ MCStringCopy(*t_long_path, r_tmp_name);
			return;
		}
        
        MCAutoStringRef t_tmp_name;
        /* UNCHECKED */ MCStringMutableCopy(*t_long_path, &t_tmp_name) &&
			MCStringAppendFormat(*t_tmp_name, "/%s", t_ptr + 1) &&
			MCStringCopy(*t_tmp_name, r_tmp_name);
    }
	///* LEGACY */ virtual char *GetTemporaryFileName(void) = 0;
	
	virtual MCSysModuleHandle LoadModule(MCStringRef p_path)
    {
#ifdef /* MCS_loadmodule_dsk_w32 */ LEGACY_SYSTEM
	char *t_native_filename;
	t_native_filename = MCS_resolvepath(p_filename);
	if (t_native_filename == NULL)
		return NULL;

	// MW-2011-02-28: [[ Bug 9410 ]] Use the Ex form of LoadLibrary and ask it to try
	//   to resolve dependent DLLs from the folder containing the DLL first.
	HMODULE t_handle;
	t_handle = LoadLibraryExA(t_native_filename, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (t_handle == NULL)
	{
		delete t_native_filename;
		return NULL;
	}

	delete t_native_filename;

	return (MCSysModuleHandle)t_handle;
#endif /* MCS_loadmodule_dsk_w32 */        
        // MW-2011-02-28: [[ Bug 9410 ]] Use the Ex form of LoadLibrary and ask it to try
        //   to resolve dependent DLLs from the folder containing the DLL first.
        HMODULE t_handle;
        t_handle = LoadLibraryExA(MCStringGetCString(p_path), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        
        return (MCSysModuleHandle)t_handle;
    }
    
	virtual MCSysModuleHandle ResolveModuleSymbol(MCSysModuleHandle p_module, MCStringRef p_symbol)
    {
#ifdef /* MCS_resolvemodulesymbol_dsk_w32 */ LEGACY_SYSTEM
	return GetProcAddress((HMODULE)p_module, p_symbol);
#endif /* MCS_resolvemodulesymbol_dsk_w32 */
        return (MCSysModuleHandle)GetProcAddress((HMODULE)p_module, MCStringGetCString(p_symbol));
    }
	virtual void UnloadModule(MCSysModuleHandle p_module)
    {
#ifdef /* MCS_unloadmodule_dsk_w32 */ LEGACY_SYSTEM
	FreeLibrary((HMODULE)p_module);
#endif /* MCS_unloadmodule_dsk_w32 */
        FreeLibrary((HMODULE)p_module);
    }
	
	virtual bool ListFolderEntries(bool p_files, bool p_detailed, MCListRef& r_list)
    {
#ifdef /* MCS_getentries_dsk_w32 */ LEGACY_SYSTEM
	MCAutoListRef t_list;

	if (!MCListCreateMutable('\n', &t_list))
		return false;

	WIN32_FIND_DATAA data;
	HANDLE ffh;            //find file handle
	uint4 t_entry_count;
	t_entry_count = 0;
	Boolean ok = False;

	MCAutoStringRef t_curdir_native;
	MCAutoStringRef t_search_path;

	const char *t_separator;
	/* UNCHECKED */ MCS_getcurdir_native(&t_curdir_native);
	if (MCStringGetCharAtIndex(*t_curdir_native, MCStringGetLength(*t_curdir_native) - 1) != '\\')
		t_separator = "\\";
	else
		t_separator = "";
	/* UNCHECKED */ MCStringFormat(&t_search_path, "%s%s*.*", MCStringGetCString(*t_curdir_native), t_separator);

	/*
	* Now open the directory for reading and iterate over the contents.
	*/
	ffh = FindFirstFileA(MCStringGetCString(*t_search_path), &data);
	if (ffh == INVALID_HANDLE_VALUE)
		return false;

	do
	{
		if (strequal(data.cFileName, "."))
			continue;
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !p_files
		        || !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && p_files)
		{
			MCAutoStringRef t_detailed_string;
			if (p_detailed)
			{
				MCAutoStringRef t_filename_string;
				/* UNCHECKED */ MCStringCreateWithNativeChars((char_t *)data.cFileName, MCCStringLength(data.cFileName), &t_filename_string);
				MCAutoStringRef t_urlencoded_string;
				/* UNCHECKED */ MCFiltersUrlEncode(*t_filename_string, &t_urlencoded_string);
				struct _stati64 buf;
				_stati64(data.cFileName, &buf);
				// MW-2007-02-26: [[ Bug 4474 ]] - Fix issue with detailed files not working on windows due to time field lengths
				// MW-2007-12-10: [[ Bug 606 ]] - Make unsupported fields appear as empty
				/* UNCHECKED */ MCStringFormat(&t_detailed_string,
					"%s,%I64d,,%ld,%ld,%ld,,,,%03o,",
					MCStringGetCString(*t_urlencoded_string),
					buf.st_size, (long)buf.st_ctime, (long)buf.st_mtime,
					(long)buf.st_atime, buf.st_mode & 0777);
			}

			if (p_detailed)
				/* UNCHECKED */ MCListAppend(*t_list, *t_detailed_string);
			else
				/* UNCHECKED */ MCListAppendNativeChars(*t_list, (char_t *)data.cFileName, MCCStringLength(data.cFileName));

			t_entry_count += 1;
		}
	}
	while (FindNextFileA(ffh, &data));
	FindClose(ffh);

	return MCListCopy(*t_list, r_list);
#endif /* MCS_getentries_dsk_w32 */
        MCAutoListRef t_list;
        
        if (!MCListCreateMutable('\n', &t_list))
            return false;
        
        WIN32_FIND_DATAA data;
        HANDLE ffh;            //find file handle
        uint4 t_entry_count;
        t_entry_count = 0;
        Boolean ok = False;
        
        MCAutoStringRef t_curdir_native;
        MCAutoStringRef t_search_path;
        
        const char *t_separator;
        /* UNCHECKED */ MCS_getcurdir_native(&t_curdir_native);
        if (MCStringGetCharAtIndex(*t_curdir_native, MCStringGetLength(*t_curdir_native) - 1) != '\\')
            t_separator = "\\";
        else
            t_separator = "";
        /* UNCHECKED */ MCStringFormat(&t_search_path, "%s%s*.*", MCStringGetCString(*t_curdir_native), t_separator);
        
        /*
         * Now open the directory for reading and iterate over the contents.
         */
        ffh = FindFirstFileA(MCStringGetCString(*t_search_path), &data);
        if (ffh == INVALID_HANDLE_VALUE)
            return false;
        
        do
        {
            if (strequal(data.cFileName, "."))
                continue;
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !p_files
		        || !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && p_files)
            {
                MCAutoStringRef t_detailed_string;
                if (p_detailed)
                {
                    MCAutoStringRef t_filename_string;
                    /* UNCHECKED */ MCStringCreateWithNativeChars((char_t *)data.cFileName, MCCStringLength(data.cFileName), &t_filename_string);
                    MCAutoStringRef t_urlencoded_string;
                    /* UNCHECKED */ MCFiltersUrlEncode(*t_filename_string, &t_urlencoded_string);
                    struct _stati64 buf;
                    _stati64(data.cFileName, &buf);
                    // MW-2007-02-26: [[ Bug 4474 ]] - Fix issue with detailed files not working on windows due to time field lengths
                    // MW-2007-12-10: [[ Bug 606 ]] - Make unsupported fields appear as empty
                    /* UNCHECKED */ MCStringFormat(&t_detailed_string,
                                                   "%s,%I64d,,%ld,%ld,%ld,,,,%03o,",
                                                   MCStringGetCString(*t_urlencoded_string),
                                                   buf.st_size, (long)buf.st_ctime, (long)buf.st_mtime,
                                                   (long)buf.st_atime, buf.st_mode & 0777);
                }
                
                if (p_detailed)
				/* UNCHECKED */ MCListAppend(*t_list, *t_detailed_string);
                else
				/* UNCHECKED */ MCListAppendNativeChars(*t_list, (char_t *)data.cFileName, MCCStringLength(data.cFileName));
                
                t_entry_count += 1;
            }
        }
        while (FindNextFileA(ffh, &data));
        FindClose(ffh);
        
        return MCListCopy(*t_list, r_list);
    }
    
	virtual bool PathToNative(MCStringRef p_path, MCStringRef& r_native) = 0;
	virtual bool PathFromNative(MCStringRef p_native, MCStringRef& r_livecode_path)
	{
#ifdef /* MCU_path2std */ LEGACY_SYSTEM
	uindex_t t_length = MCStringGetLength(p_path);
	if (t_length == 0)
		return MCStringCopy(p_path, r_stdpath);

	MCAutoNativeCharArray t_path;
	if (!t_path.New(t_length))
		return false;

	const char_t *t_src = MCStringGetNativeCharPtr(p_path);
	char_t *t_dst = t_path.Chars();

	for (uindex_t i = 0; i < t_length; i++)
	{
#ifdef _MACOSX
		if (t_src[i] == '/')
			t_dst[i] = ':';
		else if (t_src[i] == ':')
			t_dst[i] = '/';
		else
			t_dst[i] = t_src[i];
#else
		if (t_src[i] == '/')
			t_dst[i] = '\\';
		else if (t_src[i] == '\\')
			t_dst[i] = '/';
		else
			t_dst[i] = t_src[i];
#endif
	}

	return t_path.CreateStringAndRelease(r_stdpath);
#endif /* MCU_path2std */
		uindex_t t_length = MCStringGetLength(p_native);
		if (t_length == 0)
			return MCStringCopy(p_native, r_livecode_path);

		MCAutoNativeCharArray t_path;
		if (!t_path.New(t_length))
			return false;

		const char_t *t_src = MCStringGetNativeCharPtr(p_native);
		char_t *t_dst = t_path.Chars();

		for (uindex_t i = 0; i < t_length; i++)
		{
			if (t_src[i] == '/')
				t_dst[i] = '\\';
			else if (t_src[i] == '\\')
				t_dst[i] = '/';
			else
				t_dst[i] = t_src[i];
		}

		return t_path.CreateStringAndRelease(r_livecode_path);
	}

	virtual bool ResolvePath(MCStringRef p_path, MCStringRef& r_resolved_path) = 0;
    
	///* LEGACY */ char *ResolvePath(const char *p_rev_path);
	virtual bool ResolveNativePath(MCStringRef p_path, MCStringRef& r_resolved_path) = 0;
	///* LEGACY */ char *ResolveNativePath(const char *p_rev_path);
	
	virtual bool LongFilePath(MCStringRef p_path, MCStringRef& r_long_path)
    {
#ifdef /* MCS_longfilepath_dsk_w32 */ LEGACY_SYSTEM
	MCAutoStringRef t_resolved_path;
	if (!MCS_resolvepath(p_path, &t_resolved_path))
		return false;

	MCAutoStringRef t_long_path;
	if (!MCStringCreateMutable(0, &t_long_path))
		return false;

	MCAutoStringRefArray t_components;
	if (!MCStringsSplit(*t_resolved_path, '\\', t_components.PtrRef(), t_components.CountRef()))
		return false;

	for (uindex_t i = 0; i < t_components.Count(); i++)
	{
		if (MCStringGetCharAtIndex(t_components[i], 1) == ':')
		{
			if (!MCStringAppend(*t_long_path, t_components[i]))
				return false;
		}
		else
		{
			MCAutoStringRef t_short_path;
			if (!MCS_path_append(*t_long_path, PATH_DELIMITER, t_components[i], &t_short_path))
				return false;

			// Convert token to long name
			WIN32_FIND_DATAA wfd;
			HANDLE handle;

			handle = FindFirstFileA(MCStringGetCString(*t_short_path), &wfd);
			if (handle == INVALID_HANDLE_VALUE)
			{
				MCS_seterrno(GetLastError());
				r_long_path = MCValueRetain(kMCEmptyString);
				return true;
			}
			MCAutoStringRef t_filename;
			bool t_success = MCStringAppendFormat(*t_long_path, "\\%s", wfd.cFileName);
			FindClose(handle);
			if (!t_success)
				return false;
		}
	}

	return MCU_path2std(*t_long_path, r_long_path);
#endif /* MCS_longfilepath_dsk_w32 */        
        MCAutoStringRef t_long_path;
        if (!MCStringCreateMutable(0, &t_long_path))
            return false;
        
        MCAutoStringRefArray t_components;
        if (!MCStringsSplit(p_path, '\\', t_components.PtrRef(), t_components.CountRef()))
            return false;
        
        for (uindex_t i = 0; i < t_components.Count(); i++)
        {
            if (MCStringGetCharAtIndex(t_components[i], 1) == ':')
            {
                if (!MCStringAppend(*t_long_path, t_components[i]))
                    return false;
            }
            else
            {
                MCAutoStringRef t_short_path;
                if (!MCS_path_append(*t_long_path, PATH_DELIMITER, t_components[i], &t_short_path))
                    return false;
                
                // Convert token to long name
                WIN32_FIND_DATAA wfd;
                HANDLE handle;
                
                handle = FindFirstFileA(MCStringGetCString(*t_short_path), &wfd);
                if (handle == INVALID_HANDLE_VALUE)
                {
                    MCS_seterrno(GetLastError());
                    r_long_path = MCValueRetain(kMCEmptyString);
                    return true;
                }
                MCAutoStringRef t_filename;
                bool t_success = MCStringAppendFormat(*t_long_path, "\\%s", wfd.cFileName);
                FindClose(handle);
                if (!t_success)
                    return false;
            }
        }
        
        return MCU_path2std(*t_long_path, r_long_path);
    }
    
	virtual bool ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path)
    {
#ifdef /* MCS_shortfilepath_dsk_w32 */ LEGACY_SYSTEM
	MCAutoStringRef t_resolved, t_short_path;
	MCAutoNativeCharArray t_buffer;

	if (!MCS_resolvepath(p_path, &t_resolved) ||
		!t_buffer.New(PATH_MAX + 1))
		return false;

	if (!GetShortPathNameA(MCStringGetCString(*t_resolved), (LPSTR)t_buffer.Chars(), PATH_MAX))
	{
		MCS_seterrno(GetLastError());
		r_short_path = MCValueRetain(kMCEmptyString);
		return true;
	}
	t_buffer.Shrink(MCCStringLength((const char*)t_buffer.Chars()));
	return t_buffer.CreateStringAndRelease(&t_short_path) &&
		MCU_path2std(*t_short_path, r_short_path);
#endif /* MCS_shortfilepath_dsk_w32 */
        MCAutoStringRef t_short_path;
        MCAutoNativeCharArray t_buffer;
        
        if (!t_buffer.New(PATH_MAX + 1))
            return false;
        
        if (!GetShortPathNameA(MCStringGetCString(p_path), (LPSTR)t_buffer.Chars(), PATH_MAX))
        {
            MCS_seterrno(GetLastError());
            r_short_path = MCValueRetain(kMCEmptyString);
            return true;
        }
        t_buffer.Shrink(MCCStringLength((const char*)t_buffer.Chars()));
        return t_buffer.CreateStringAndRelease(&t_short_path) &&
		MCU_path2std(*t_short_path, r_short_path);
    }
    
	virtual bool Shell(MCStringRef filename, MCDataRef& r_data, int& r_retcode) = 0;
    
	virtual char *GetHostName(void) = 0;
	virtual bool HostNameToAddress(MCStringRef p_hostname, MCSystemHostResolveCallback p_callback, void *p_context) = 0;
	virtual bool AddressToHostName(MCStringRef p_address, MCSystemHostResolveCallback p_callback, void *p_context) = 0;
    
	virtual uint32_t TextConvert(const void *p_string, uint32_t p_string_length, void *r_buffer, uint32_t p_buffer_length, uint32_t p_from_charset, uint32_t p_to_charset) = 0;
	virtual bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4& p_output_length, uint4& r_used) = 0;
    
    virtual void CheckProcesses(void)
    {
#ifdef /* MCS_checkprocesses_dsk_w32 */ LEGACY_SYSTEM
	uint2 i;
	for (i = 0 ; i < MCnprocesses ; i++)
		if (MCprocesses[i].phandle != NULL)
		{
			DWORD err = WaitForSingleObject(MCprocesses[i].phandle, 0);
			if (err == WAIT_OBJECT_0 || err == WAIT_FAILED)
			{
				// MW-2010-05-17: Make sure we keep the process around long enough to
				//   read in all its data.
				uint32_t t_available;
				if (MCprocesses[i].ihandle == NULL || !PeekNamedPipe(MCprocesses[i].ihandle->fhandle, NULL, 0, NULL, (DWORD *)&t_available, NULL))
					t_available = 0;
				if (t_available != 0)
					continue;

				// MW-2010-10-25: [[ Bug 9134 ]] Make sure the we mark the stream as 'ATEOF'
				if (MCprocesses[i] . ihandle != nil)
					MCprocesses[i] . ihandle -> flags |= IO_ATEOF;

				DWORD retcode;
				GetExitCodeProcess(MCprocesses[i].phandle, &retcode);
				MCprocesses[i].retcode = retcode;
				MCprocesses[i].pid = 0;
				MCprocesses[i].phandle = NULL;
				Sleep(0);
				if (MCprocesses[i].thandle != NULL)
				{
					TerminateThread(MCprocesses[i].thandle, 0);
					MCprocesses[i].thandle = NULL;
				}
			}
		}
#endif /* MCS_checkprocesses_dsk_w32 */
    }
    
    virtual uint32_t GetSystemError(void) = 0;
    
    virtual IO_stat RunCommand(MCStringRef p_command, MCStringRef& r_output)
    {
#ifdef /* MCS_runcmd_dsk_w32 */ LEGACY_SYSTEM
	IO_cleanprocesses();
	SECURITY_ATTRIBUTES saAttr;
	/* Set the bInheritHandle flag so pipe handles are inherited. */
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	Boolean created = True;
	HANDLE hChildStdinRd = NULL;
	HANDLE hChildStdinWr = NULL;
	HANDLE hChildStdoutRd = NULL;
	HANDLE hChildStdoutWr = NULL;
	HANDLE hChildStderrWr = NULL;
	if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)
	        || !CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
		created = False;

	// MW-2012-08-06: [[ Bug 10161 ]] Make sure our ends of the pipes are not inherited
	//   into the child.
	if (!SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0) ||
		!SetHandleInformation(hChildStdinWr, HANDLE_FLAG_INHERIT, 0))
		created = False;

	PROCESS_INFORMATION piProcInfo;
	STARTUPINFOA siStartInfo;
	memset(&siStartInfo, 0, sizeof(STARTUPINFOA));
	siStartInfo.cb = sizeof(STARTUPINFOA);
	siStartInfo.dwFlags = STARTF_USESTDHANDLES;
	siStartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	if (MChidewindows)
		siStartInfo.wShowWindow = SW_HIDE;
	else
		siStartInfo.wShowWindow = SW_SHOW;
	siStartInfo.hStdInput = hChildStdinRd;
	siStartInfo.hStdOutput = hChildStdoutWr;

	ep.insert(" /C ", 0, 0);
	ep.insert(MCshellcmd, 0, 0);
	char *pname = ep.getsvalue().clone();
	MCU_realloc((char **)&MCprocesses, MCnprocesses,
	            MCnprocesses + 1, sizeof(Streamnode));
	uint4 index = MCnprocesses;
	MCprocesses[index].name = (MCNameRef)MCValueRetain(MCM_shell);
	MCprocesses[index].mode = OM_NEITHER;
	MCprocesses[index].ohandle = NULL;
	MCprocesses[index].ihandle = new IO_header((MCWinSysHandle)hChildStdoutRd, NULL, 0, 0);
	// MW-2012-09-10: [[ Bug 10230 ]] Make sure we mark this IO handle as a pipe.
	MCprocesses[index].ihandle -> is_pipe = true;
	if (created)
	{
		HANDLE phandle = GetCurrentProcess();
		DuplicateHandle(phandle, hChildStdoutWr, phandle, &hChildStderrWr,
		                0, TRUE, DUPLICATE_SAME_ACCESS);
		siStartInfo.hStdError = hChildStderrWr;
		DWORD threadID = 0;
		if (CreateProcessA(NULL, pname, NULL, NULL, TRUE, CREATE_NEW_CONSOLE,
		                  NULL, NULL, &siStartInfo, &piProcInfo))
		{
			MCprocesses[MCnprocesses].pid = piProcInfo.dwProcessId;
			MCprocesses[MCnprocesses++].phandle = (MCWinSysHandle)piProcInfo.hProcess;
			MCprocesses[index].thandle = (MCWinSysHandle)CreateThread(NULL, 0,	(LPTHREAD_START_ROUTINE)readThread, &MCprocesses[index], 0, &threadID);
			if (MCprocesses[index].thandle == NULL)
				created = False;
			else
				SetThreadPriority(MCprocesses[index].thandle, THREAD_PRIORITY_HIGHEST);
		}
		else
		{
			MCS_seterrno(GetLastError());
			created = False;
		}
	}
	BOOL isclosed = CloseHandle(hChildStdinRd);
	isclosed = CloseHandle(hChildStdinWr);
	isclosed = CloseHandle(hChildStdoutWr);
	isclosed = CloseHandle(hChildStderrWr);
	if (!created)
	{
		CloseHandle(hChildStdoutRd);
		Sleep(0);
		MCeerror->add(EE_SHELL_BADCOMMAND, 0, 0, pname);
		delete pname;
		return IO_ERROR;
	}

	s_finished_reading = false;

	do
	{
		if (MCscreen->wait(READ_INTERVAL, False, False))
		{
			MCeerror->add(EE_SHELL_ABORT, 0, 0, pname);
			if (MCprocesses[index].pid != 0)
			{
				TerminateProcess(MCprocesses[index].phandle, 0);
				MCprocesses[index].pid = 0;
				TerminateThread(MCprocesses[index].thandle, 0);
				CloseHandle(piProcInfo.hProcess);
				CloseHandle(piProcInfo.hThread);
			}
			MCS_close(MCprocesses[index].ihandle);
			IO_cleanprocesses();
			delete pname;
			return IO_ERROR;
		}
	}
	while(!s_finished_reading);
	MCS_checkprocesses();
	if (MCprocesses[index].pid == 0)
	{
		Sleep(0);
		TerminateThread(MCprocesses[index].thandle, 0);
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}
	if (MCprocesses[index].retcode)
	{
		MCExecPoint ep2(ep);
		ep2.setint(MCprocesses[index].retcode);
		MCresult->set(ep2);
	}
	else
		MCresult->clear(False);
	ep.copysvalue(MCprocesses[index].ihandle->buffer, MCprocesses[index].ihandle->len);
	delete MCprocesses[index].ihandle->buffer;
	MCS_close(MCprocesses[index].ihandle);
	IO_cleanprocesses();
	delete pname;
	ep.texttobinary();
	return IO_NORMAL;
#endif /* MCS_runcmd_dsk_w32 */
        IO_cleanprocesses();
        SECURITY_ATTRIBUTES saAttr;
        /* Set the bInheritHandle flag so pipe handles are inherited. */
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;
        
        Boolean created = True;
        HANDLE hChildStdinRd = NULL;
        HANDLE hChildStdinWr = NULL;
        HANDLE hChildStdoutRd = NULL;
        HANDLE hChildStdoutWr = NULL;
        HANDLE hChildStderrWr = NULL;
        if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)
	        || !CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
            created = False;
        
        // MW-2012-08-06: [[ Bug 10161 ]] Make sure our ends of the pipes are not inherited
        //   into the child.
        if (!SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0) ||
            !SetHandleInformation(hChildStdinWr, HANDLE_FLAG_INHERIT, 0))
            created = False;
        
        PROCESS_INFORMATION piProcInfo;
        STARTUPINFOA siStartInfo;
        memset(&siStartInfo, 0, sizeof(STARTUPINFOA));
        siStartInfo.cb = sizeof(STARTUPINFOA);
        siStartInfo.dwFlags = STARTF_USESTDHANDLES;
        siStartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        if (MChidewindows)
            siStartInfo.wShowWindow = SW_HIDE;
        else
            siStartInfo.wShowWindow = SW_SHOW;
        siStartInfo.hStdInput = hChildStdinRd;
        siStartInfo.hStdOutput = hChildStdoutWr;
        
		MCAutoStringRef t_mutable_cmd;

		/* UNCHECKED */ MCStringCreateMutable(0, &t_mutable_cmd);
		/* UNCHECKED */ MCStringFormat(&t_mutable_cmd, "%s\"%s\"%x", " /C ", MCshellcmd, p_command);
        char *pname = strclone(MCStringGetCString(*t_mutable_cmd));
        MCU_realloc((char **)&MCprocesses, MCnprocesses,
                    MCnprocesses + 1, sizeof(Streamnode));
        uint4 index = MCnprocesses;
        MCprocesses[index].name = (MCNameRef)MCValueRetain(MCM_shell);
        MCprocesses[index].mode = OM_NEITHER;
        MCprocesses[index].ohandle = NULL;
        MCprocesses[index].ihandle = new IO_header((MCWinSysHandle)hChildStdoutRd, NULL, 0, 0);
        // MW-2012-09-10: [[ Bug 10230 ]] Make sure we mark this IO handle as a pipe.
        MCprocesses[index].ihandle -> is_pipe = true;
        if (created)
        {
            HANDLE phandle = GetCurrentProcess();
            DuplicateHandle(phandle, hChildStdoutWr, phandle, &hChildStderrWr,
                            0, TRUE, DUPLICATE_SAME_ACCESS);
            siStartInfo.hStdError = hChildStderrWr;
            DWORD threadID = 0;
            if (CreateProcessA(NULL, pname, NULL, NULL, TRUE, CREATE_NEW_CONSOLE,
                               NULL, NULL, &siStartInfo, &piProcInfo))
            {
                MCprocesses[MCnprocesses].pid = piProcInfo.dwProcessId;
                MCprocesses[MCnprocesses++].phandle = (MCWinSysHandle)piProcInfo.hProcess;
                MCprocesses[index].thandle = (MCWinSysHandle)CreateThread(NULL, 0,	(LPTHREAD_START_ROUTINE)readThread, &MCprocesses[index], 0, &threadID);
                if (MCprocesses[index].thandle == NULL)
                    created = False;
                else
                    SetThreadPriority(MCprocesses[index].thandle, THREAD_PRIORITY_HIGHEST);
            }
            else
            {
                MCS_seterrno(GetLastError());
                created = False;
            }
        }
        BOOL isclosed = CloseHandle(hChildStdinRd);
        isclosed = CloseHandle(hChildStdinWr);
        isclosed = CloseHandle(hChildStdoutWr);
        isclosed = CloseHandle(hChildStderrWr);
        if (!created)
        {
            CloseHandle(hChildStdoutRd);
            Sleep(0);
            MCeerror->add(EE_SHELL_BADCOMMAND, 0, 0, pname);
            delete pname;
            return IO_ERROR;
        }
        
        s_finished_reading = false;
        
        do
        {
            if (MCscreen->wait(READ_INTERVAL, False, False))
            {
                MCeerror->add(EE_SHELL_ABORT, 0, 0, pname);
                if (MCprocesses[index].pid != 0)
                {
                    TerminateProcess(MCprocesses[index].phandle, 0);
                    MCprocesses[index].pid = 0;
                    TerminateThread(MCprocesses[index].thandle, 0);
                    CloseHandle(piProcInfo.hProcess);
                    CloseHandle(piProcInfo.hThread);
                }
                MCS_close(MCprocesses[index].ihandle);
                IO_cleanprocesses();
                delete pname;
                return IO_ERROR;
            }
        }
        while(!s_finished_reading);
        CheckProcesses();
        if (MCprocesses[index].pid == 0)
        {
            Sleep(0);
            TerminateThread(MCprocesses[index].thandle, 0);
            CloseHandle(piProcInfo.hProcess);
            CloseHandle(piProcInfo.hThread);
        }
        if (MCprocesses[index].retcode)
        {
			MCExecPoint t_ep(nil, nil, nil);
			t_ep.setint(MCprocesses[index].retcode);
			MCresult -> set(t_ep);
        }
        else
            MCresult->clear(False);

		MCExecPoint t_ep;
        t_ep.copysvalue(MCprocesses[index].ihandle->buffer, MCprocesses[index].ihandle->len);

        MCS_close(MCprocesses[index].ihandle);
        IO_cleanprocesses();
        delete pname;
        t_ep.texttobinary();

		r_output = (MCStringRef) MCValueRetain(t_ep.getvalueref());
        return IO_NORMAL;
    }
    
    // MW-2010-05-09: Updated to add 'elevated' parameter for executing binaries
    //   at increased privilege level.
    virtual bool StartProcess(MCNameRef p_name, MCStringRef p_doc, Open_mode p_mode, Boolean p_elevated)
    {
#ifdef /* MCS_startprocess_dsk_w32 */ LEGACY_SYSTEM
	Boolean reading = mode == OM_READ || mode == OM_UPDATE;
	Boolean writing = mode == OM_APPEND || mode == OM_WRITE || mode == OM_UPDATE;
	MCU_realloc((char **)&MCprocesses, MCnprocesses, MCnprocesses + 1,
	            sizeof(Streamnode));
	MCprocesses[MCnprocesses].name = (MCNameRef)MCValueRetain(p_name);
	MCprocesses[MCnprocesses].mode = mode;
	MCprocesses[MCnprocesses].ihandle = NULL;
	MCprocesses[MCnprocesses].ohandle = NULL;
	MCprocesses[MCnprocesses].phandle = NULL; //process handle
	MCprocesses[MCnprocesses].thandle = NULL; //child thread handle

	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	Boolean created = True;
	HANDLE t_process_handle = NULL;
	DWORD t_process_id = 0;
	HANDLE hChildStdinWr = NULL;
	HANDLE hChildStdoutRd = NULL;
	const char *t_error;
	t_error = nil;
	if (created)
	{
		MCAutoStringRef t_cmdline;
		if (doc != nil && *doc != '\0')
			/* UNCHECKED */ MCStringFormat(&t_cmdline, "%s \"%s\"", MCNameGetCString(p_name), doc);
		else
			t_cmdline = MCNameGetString(p_name);
		
		// There's no such thing as Elevation before Vista (majorversion 6)
		if (!elevated || MCmajorosversion < 0x0600)
		{
			HANDLE hChildStdinRd = NULL;
			HANDLE hChildStdoutWr = NULL;
			HANDLE hChildStderrWr = NULL;
			if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)
					|| !CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
				created = False;
			else
			{
				// Make sure we turn off inheritence for the read side of stdout and write side of stdin
				SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);
				SetHandleInformation(hChildStdinWr, HANDLE_FLAG_INHERIT, 0);

				// Clone the write handle to be stderr too
				HANDLE phandle = GetCurrentProcess();
				DuplicateHandle(phandle, hChildStdoutWr, phandle, &hChildStderrWr, 0, TRUE, DUPLICATE_SAME_ACCESS);
			}

			if (created)
			{
				PROCESS_INFORMATION piProcInfo;
				STARTUPINFOA siStartInfo;
				memset(&siStartInfo, 0, sizeof(STARTUPINFOA));
				siStartInfo.cb = sizeof(STARTUPINFOA);
				siStartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
				if (MChidewindows)
					siStartInfo.wShowWindow = SW_HIDE;
				else
					siStartInfo.wShowWindow = SW_SHOW;
				siStartInfo.hStdInput = hChildStdinRd;
				siStartInfo.hStdOutput = hChildStdoutWr;
				siStartInfo.hStdError = hChildStderrWr;
				if (CreateProcessA(NULL, (LPSTR)MCStringGetCString(*t_cmdline), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &siStartInfo, &piProcInfo))
				{
					t_process_handle = piProcInfo . hProcess;
					t_process_id = piProcInfo . dwProcessId;
					CloseHandle(piProcInfo . hThread);
				}
				else
					created = False;
			}

			CloseHandle(hChildStdinRd);
			CloseHandle(hChildStdoutWr);
			CloseHandle(hChildStderrWr);
		}
		else
		{
			// Unfortunately, one cannot use any 'CreateProcess' type calls to
			// elevate a process - one must use ShellExecuteEx. This unfortunately
			// means we have no way of *directly* passing things like env vars and
			// std handles to it. Instead, we do the following:
			//   1) Launch ourselves with the parameter '-elevated-slave'
			//   2) Wait until either the target process vanishes, or we get
			//      a thread message posted to us with a pair of pipe handles.
			//   3) Write the command line and env strings to the pipe
			//   4) Wait for a further message with process handle and id
			//   5) Carry on with the handles we were given to start with
			// If the launched process vanished before (4) it is treated as failure.

			char t_parameters[64];
			sprintf(t_parameters, "-elevated-slave%08x", GetCurrentThreadId());

			SHELLEXECUTEINFOA t_info;
			memset(&t_info, 0, sizeof(SHELLEXECUTEINFO));
			t_info . cbSize = sizeof(SHELLEXECUTEINFO);
			t_info . fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE ;
			t_info . hwnd = (HWND)MCdefaultstackptr -> getrealwindow();
			t_info . lpVerb = "runas";
			t_info . lpFile = MCcmd;
			t_info . lpParameters = t_parameters;
			t_info . nShow = SW_HIDE;
			if (ShellExecuteExA(&t_info) && (uintptr_t)t_info . hInstApp > 32)
			{
				MSG t_msg;
				t_msg . message = WM_QUIT;
				while(!PeekMessageA(&t_msg, (HWND)-1, WM_USER + 10, WM_USER + 10, PM_REMOVE))
					if (MsgWaitForMultipleObjects(1, &t_info . hProcess, FALSE, INFINITE, QS_POSTMESSAGE) == WAIT_OBJECT_0)
					{
						created = False;
						break;
					}

				if (created && t_msg . message == WM_USER + 10)
				{
					HANDLE t_output_pipe, t_input_pipe;
					t_input_pipe = (HANDLE)t_msg . wParam;
					t_output_pipe = (HANDLE)t_msg . lParam;

					// Get the environment strings to send across
					char *t_env_strings;
					uint32_t t_env_length;
#undef GetEnvironmentStrings
					t_env_strings = GetEnvironmentStrings();
					if (t_env_strings != nil)
					{
						t_env_length = 0;
						while(t_env_strings[t_env_length] != '\0' || t_env_strings[t_env_length + 1] != '\0')
							t_env_length += 1;
						t_env_length += 2;
					}

					// Write out the cmd line and env strings
					const char *cmdline;
					cmdline = MCStringGetCString(*t_cmdline);
					if (write_blob_to_pipe(t_output_pipe, strlen(cmdline) + 1, cmdline) &&
						write_blob_to_pipe(t_output_pipe, t_env_length, t_env_strings))
					{
						// Now we should have a process id and handle waiting for us.
						MSG t_msg;
						t_msg . message = WM_QUIT;
						while(!PeekMessageA(&t_msg, (HWND)-1, WM_USER + 10, WM_USER + 10, PM_REMOVE))
							if (MsgWaitForMultipleObjects(1, &t_info . hProcess, FALSE, INFINITE, QS_POSTMESSAGE) == WAIT_OBJECT_0)
							{
								created = False;
								break;
							}

						if (created && t_msg . message == WM_USER + 10 && t_msg . lParam != NULL)
						{
							t_process_id = (DWORD)t_msg . wParam;
							t_process_handle = (HANDLE)t_msg . lParam;
						}
						else
							created = False;
					}
					else
						created = False;

					FreeEnvironmentStringsA(t_env_strings);

					hChildStdinWr = t_output_pipe;
					hChildStdoutRd = t_input_pipe;
				}
				else
					created = False;

				CloseHandle(t_info . hProcess);
			}
			else
			{
				if ((uintptr_t)t_info . hInstApp == SE_ERR_ACCESSDENIED)
					t_error = "access denied";
				created = False;
			}
		}
	}
	if (created)
	{
		if (writing)
		{
			MCprocesses[MCnprocesses].ohandle = new IO_header((MCWinSysHandle)hChildStdinWr, NULL, 0, 0);
			// MW-2012-09-10: [[ Bug 10230 ]] Make sure we mark this IO handle as a pipe.
			MCprocesses[MCnprocesses].ohandle -> is_pipe = true;
		}
		else
			CloseHandle(hChildStdinWr);
		if (reading)
		{
			MCprocesses[MCnprocesses].ihandle = new IO_header((MCWinSysHandle)hChildStdoutRd, NULL, 0, 0);
			// MW-2012-09-10: [[ Bug 10230 ]] Make sure we mark this IO handle as a pipe.
			MCprocesses[MCnprocesses].ihandle -> is_pipe = true;
		}
		else
			CloseHandle(hChildStdoutRd);
	}
	if (!created)
	{
		MCresult->sets(t_error == nil ? "not opened" : t_error);
		MCS_seterrno(GetLastError());
		CloseHandle(hChildStdinWr);
		CloseHandle(hChildStdoutRd);
	}
	else
	{
		MCresult->clear(False);
		MCprocesses[MCnprocesses].pid = t_process_id;
		MCprocesses[MCnprocesses++].phandle = (MCWinSysHandle)t_process_handle;
	}
#endif /* MCS_startprocess_dsk_w32 */
        Boolean reading = p_mode == OM_READ || p_mode == OM_UPDATE;
        Boolean writing = p_mode == OM_APPEND || p_mode == OM_WRITE || p_mode == OM_UPDATE;
        MCU_realloc((char **)&MCprocesses, MCnprocesses, MCnprocesses + 1,
                    sizeof(Streamnode));
        MCprocesses[MCnprocesses].name = (MCNameRef)MCValueRetain(p_name);
        MCprocesses[MCnprocesses].mode = p_mode;
        MCprocesses[MCnprocesses].ihandle = NULL;
        MCprocesses[MCnprocesses].ohandle = NULL;
        MCprocesses[MCnprocesses].phandle = NULL; //process handle
        MCprocesses[MCnprocesses].thandle = NULL; //child thread handle
        
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;
        
        Boolean created = True;
        HANDLE t_process_handle = NULL;
        DWORD t_process_id = 0;
        HANDLE hChildStdinWr = NULL;
        HANDLE hChildStdoutRd = NULL;
        const char *t_error;
        t_error = nil;
        if (created)
        {
            MCAutoStringRef t_cmdline;
            if (MCStringGetLength(p_doc) != nil && MCStringGetNativeCharAtIndex(p_doc, 0) != '\0')
			{
				/* UNCHECKED */ MCStringCreateMutable(0, &t_cmdline);
				/* UNCHECKED */ MCStringFormat(&t_cmdline, "%s \"%s\"", MCNameGetCString(p_name), MCStringGetCString(p_doc));
			}
            else
                t_cmdline = (MCStringRef) MCValueRetain(MCNameGetString(p_name));
            
            // There's no such thing as Elevation before Vista (majorversion 6)
            if (!p_elevated || MCmajorosversion < 0x0600)
            {
                HANDLE hChildStdinRd = NULL;
                HANDLE hChildStdoutWr = NULL;
                HANDLE hChildStderrWr = NULL;
                if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)
					|| !CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
                    created = False;
                else
                {
                    // Make sure we turn off inheritence for the read side of stdout and write side of stdin
                    SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);
                    SetHandleInformation(hChildStdinWr, HANDLE_FLAG_INHERIT, 0);
                    
                    // Clone the write handle to be stderr too
                    HANDLE phandle = GetCurrentProcess();
                    DuplicateHandle(phandle, hChildStdoutWr, phandle, &hChildStderrWr, 0, TRUE, DUPLICATE_SAME_ACCESS);
                }
                
                if (created)
                {
                    PROCESS_INFORMATION piProcInfo;
                    STARTUPINFOA siStartInfo;
                    memset(&siStartInfo, 0, sizeof(STARTUPINFOA));
                    siStartInfo.cb = sizeof(STARTUPINFOA);
                    siStartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
                    if (MChidewindows)
                        siStartInfo.wShowWindow = SW_HIDE;
                    else
                        siStartInfo.wShowWindow = SW_SHOW;
                    siStartInfo.hStdInput = hChildStdinRd;
                    siStartInfo.hStdOutput = hChildStdoutWr;
                    siStartInfo.hStdError = hChildStderrWr;
                    if (CreateProcessA(NULL, (LPSTR)MCStringGetCString(*t_cmdline), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &siStartInfo, &piProcInfo))
                    {
                        t_process_handle = piProcInfo . hProcess;
                        t_process_id = piProcInfo . dwProcessId;
                        CloseHandle(piProcInfo . hThread);
                    }
                    else
                        created = False;
                }
                
                CloseHandle(hChildStdinRd);
                CloseHandle(hChildStdoutWr);
                CloseHandle(hChildStderrWr);
            }
            else
            {
                // Unfortunately, one cannot use any 'CreateProcess' type calls to
                // elevate a process - one must use ShellExecuteEx. This unfortunately
                // means we have no way of *directly* passing things like env vars and
                // std handles to it. Instead, we do the following:
                //   1) Launch ourselves with the parameter '-elevated-slave'
                //   2) Wait until either the target process vanishes, or we get
                //      a thread message posted to us with a pair of pipe handles.
                //   3) Write the command line and env strings to the pipe
                //   4) Wait for a further message with process handle and id
                //   5) Carry on with the handles we were given to start with
                // If the launched process vanished before (4) it is treated as failure.
                
                char t_parameters[64];
                sprintf(t_parameters, "-elevated-slave%08x", GetCurrentThreadId());
                
                SHELLEXECUTEINFOA t_info;
                memset(&t_info, 0, sizeof(SHELLEXECUTEINFO));
                t_info . cbSize = sizeof(SHELLEXECUTEINFO);
                t_info . fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE ;
                t_info . hwnd = (HWND)MCdefaultstackptr -> getrealwindow();
                t_info . lpVerb = "runas";
                t_info . lpFile = MCcmd;
                t_info . lpParameters = t_parameters;
                t_info . nShow = SW_HIDE;
                if (ShellExecuteExA(&t_info) && (uintptr_t)t_info . hInstApp > 32)
                {
                    MSG t_msg;
                    t_msg . message = WM_QUIT;
                    while(!PeekMessageA(&t_msg, (HWND)-1, WM_USER + 10, WM_USER + 10, PM_REMOVE))
                        if (MsgWaitForMultipleObjects(1, &t_info . hProcess, FALSE, INFINITE, QS_POSTMESSAGE) == WAIT_OBJECT_0)
                        {
                            created = False;
                            break;
                        }
                    
                    if (created && t_msg . message == WM_USER + 10)
                    {
                        HANDLE t_output_pipe, t_input_pipe;
                        t_input_pipe = (HANDLE)t_msg . wParam;
                        t_output_pipe = (HANDLE)t_msg . lParam;
                        
                        // Get the environment strings to send across
                        char *t_env_strings;
                        uint32_t t_env_length;
#undef GetEnvironmentStrings
                        t_env_strings = GetEnvironmentStrings();
                        if (t_env_strings != nil)
                        {
                            t_env_length = 0;
                            while(t_env_strings[t_env_length] != '\0' || t_env_strings[t_env_length + 1] != '\0')
                                t_env_length += 1;
                            t_env_length += 2;
                        }
                        
                        // Write out the cmd line and env strings
                        const char *cmdline;
                        cmdline = MCStringGetCString(*t_cmdline);
                        if (write_blob_to_pipe(t_output_pipe, strlen(cmdline) + 1, cmdline) &&
                            write_blob_to_pipe(t_output_pipe, t_env_length, t_env_strings))
                        {
                            // Now we should have a process id and handle waiting for us.
                            MSG t_msg;
                            t_msg . message = WM_QUIT;
                            while(!PeekMessageA(&t_msg, (HWND)-1, WM_USER + 10, WM_USER + 10, PM_REMOVE))
                                if (MsgWaitForMultipleObjects(1, &t_info . hProcess, FALSE, INFINITE, QS_POSTMESSAGE) == WAIT_OBJECT_0)
                                {
                                    created = False;
                                    break;
                                }
                            
                            if (created && t_msg . message == WM_USER + 10 && t_msg . lParam != NULL)
                            {
                                t_process_id = (DWORD)t_msg . wParam;
                                t_process_handle = (HANDLE)t_msg . lParam;
                            }
                            else
                                created = False;
                        }
                        else
                            created = False;
                        
                        FreeEnvironmentStringsA(t_env_strings);
                        
                        hChildStdinWr = t_output_pipe;
                        hChildStdoutRd = t_input_pipe;
                    }
                    else
                        created = False;
                    
                    CloseHandle(t_info . hProcess);
                }
                else
                {
                    if ((uintptr_t)t_info . hInstApp == SE_ERR_ACCESSDENIED)
                        t_error = "access denied";
                    created = False;
                }
            }
        }
        if (created)
        {
            if (writing)
            {
                MCprocesses[MCnprocesses].ohandle = new IO_header((MCWinSysHandle)hChildStdinWr, NULL, 0, 0);
                // MW-2012-09-10: [[ Bug 10230 ]] Make sure we mark this IO handle as a pipe.
                MCprocesses[MCnprocesses].ohandle -> is_pipe = true;
            }
            else
                CloseHandle(hChildStdinWr);
            if (reading)
            {
                MCprocesses[MCnprocesses].ihandle = new IO_header((MCWinSysHandle)hChildStdoutRd, NULL, 0, 0);
                // MW-2012-09-10: [[ Bug 10230 ]] Make sure we mark this IO handle as a pipe.
                MCprocesses[MCnprocesses].ihandle -> is_pipe = true;
            }
            else
                CloseHandle(hChildStdoutRd);
        }
        if (!created)
        {
            MCresult->sets(t_error == nil ? "not opened" : t_error);
            MCS_seterrno(GetLastError());
            CloseHandle(hChildStdinWr);
            CloseHandle(hChildStdoutRd);
        }
        else
        {
            MCresult->clear(False);
            MCprocesses[MCnprocesses].pid = t_process_id;
            MCprocesses[MCnprocesses++].phandle = (MCWinSysHandle)t_process_handle;
        }
    }
    
    virtual void CloseProcess(uint2 p_index)
    {
#ifdef /* MCS_closeprocess_dsk_w32 */ LEGACY_SYSTEM
	if (MCprocesses[index].thandle  != NULL)
	{
		TerminateThread(MCprocesses[index].thandle, 0);
		MCprocesses[index].thandle = NULL;
	}
	if (MCprocesses[index].ihandle != NULL)
	{
		MCS_close(MCprocesses[index].ihandle);
		MCprocesses[index].ihandle = NULL;
	}
	if (MCprocesses[index].ohandle != NULL)
	{
		MCS_close(MCprocesses[index].ohandle);
		MCprocesses[index].ohandle = NULL;
	}
	MCprocesses[index].mode = OM_NEITHER;
#endif /* MCS_closeprocess_dsk_w32 */
        if (MCprocesses[p_index].thandle  != NULL)
        {
            TerminateThread(MCprocesses[p_index].thandle, 0);
            MCprocesses[p_index].thandle = NULL;
        }
        if (MCprocesses[p_index].ihandle != NULL)
        {
            MCS_close(MCprocesses[p_index].ihandle);
            MCprocesses[p_index].ihandle = NULL;
        }
        if (MCprocesses[p_index].ohandle != NULL)
        {
            MCS_close(MCprocesses[p_index].ohandle);
            MCprocesses[p_index].ohandle = NULL;
        }
        MCprocesses[p_index].mode = OM_NEITHER;
    }
    
    virtual void Kill(int4 p_pid, int4 p_sig)
    {
#ifdef /* MCS_kill_dsk_w32 */ LEGACY_SYSTEM
	uint2 i;
	for (i = 0 ; i < MCnprocesses ; i++)
	{
		if (pid == MCprocesses[i].pid)
		{
			if (MCprocesses[i].thandle  != NULL)
			{
				TerminateThread(MCprocesses[i].thandle, 0);
				MCprocesses[i].thandle = NULL;
			}
			TerminateProcess(MCprocesses[i].phandle, 0);
			MCprocesses[i].phandle = NULL;
			MCprocesses[i].pid = 0;
			break;
		}
	}
#endif /* MCS_kill_dsk_w32 */
        uint2 i;
        for (i = 0 ; i < MCnprocesses ; i++)
        {
            if (p_pid == MCprocesses[i].pid)
            {
                if (MCprocesses[i].thandle  != NULL)
                {
                    TerminateThread(MCprocesses[i].thandle, 0);
                    MCprocesses[i].thandle = NULL;
                }
                TerminateProcess(MCprocesses[i].phandle, 0);
                MCprocesses[i].phandle = NULL;
                MCprocesses[i].pid = 0;
                break;
            }
        }
    }
    
    virtual void KillAll(void)
    {
#ifdef /* MCS_killall_dsk_w32 */ LEGACY_SYSTEM
	uint2 i;
	for (i = 0 ; i < MCnprocesses ; i++)
	{
		//kill MCprocesses[i] here
		if (MCprocesses[i].ihandle != NULL || MCprocesses[i].ohandle != NULL)
			TerminateProcess(MCprocesses[i].phandle, 0);
		MCprocesses[i].phandle = NULL;
	}
#endif /* MCS_killall_dsk_w32 */
        uint2 i;
        for (i = 0 ; i < MCnprocesses ; i++)
        {
            //kill MCprocesses[i] here
            if (MCprocesses[i].ihandle != NULL || MCprocesses[i].ohandle != NULL)
                TerminateProcess(MCprocesses[i].phandle, 0);
            MCprocesses[i].phandle = NULL;
        }
    }
    
    virtual int GetErrno(void)
    {
#ifdef /* MCS_geterrno_dsk_w32 */ LEGACY_SYSTEM
	return *g_mainthread_errno;
#endif /* MCS_geterrno_dsk_w32 */
        return *g_mainthread_errno;
    }
    
    // MW-2007-12-14: [[ Bug 5113 ]] Slow-down on mathematical operations - make sure
    //   we access errno directly to stop us having to do a thread-local-data lookup.
    virtual void SetErrno(int p_errno)
    {
#ifdef /* MCS_seterrno_dsk_w32 */ LEGACY_SYSTEM
	*g_mainthread_errno = value;
#endif /* MCS_seterrno_dsk_w32 */
        *g_mainthread_errno = p_errno;
    }
    
    virtual void LaunchDocument(MCStringRef p_document) = 0;
    virtual void LaunchUrl(MCStringRef p_document) = 0;
    
    virtual void DoAlternateLanguage(MCStringRef p_script, MCStringRef p_language) = 0;
    virtual bool AlternateLanguage(MCListRef& r_list) = 0;
    
    virtual bool GetDNSservers(MCListRef& r_list)
    {
#ifdef /* MCS_getDNSservers_dsk_w32 */ LEGACY_SYSTEM
	MCAutoListRef t_list;
	if (!dns_servers_from_network_params(&t_list))
		return false;

	if (!MCListIsEmpty(*t_list))
		return MCListCopy(*t_list, r_list);

	return dns_servers_from_registry(r_list);
#endif /* MCS_getDNSservers_dsk_w32 */
        MCAutoListRef t_list;
        if (!dns_servers_from_network_params(&t_list))
            return false;
        
        if (!MCListIsEmpty(*t_list))
            return MCListCopy(*t_list, r_list);
        
        return dns_servers_from_registry(r_list);
    }
};

// MW-2010-05-09: This is bootstrap 'main' that effectively implemented a CreateProcess
//   via 'ShellExecute' 'runas'.
int MCS_windows_elevation_bootstrap_main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	bool t_success;
	t_success = true;

	// Fetch the thread id (present immediately after '-elevated-slave' as hex).
	uint32_t t_parent_thread_id;
	if (t_success)
	{
		char *t_end;
		t_parent_thread_id = strtoul(lpCmdLine + 15, &t_end, 16);
		if (t_end != lpCmdLine + strlen(lpCmdLine))
			t_success = false;
	}

	// Open the parent's thread
	HANDLE t_parent_thread;
	t_parent_thread = nil;
	if (t_success)
	{
		t_parent_thread = OpenThread(SYNCHRONIZE | THREAD_QUERY_INFORMATION, FALSE, t_parent_thread_id);
		if (t_parent_thread == nil)
			t_success = false;
	}

	// Open the parent's process
	HANDLE t_parent_process;
	t_parent_process = nil;
	if (t_success)
	{
		t_parent_process = OpenProcess(SYNCHRONIZE | PROCESS_DUP_HANDLE, FALSE, DoGetProcessIdOfThread(t_parent_thread));
		if (t_parent_process == nil)
			t_success = false;
	}

	// Create a pipe the write end of which we will give to the parent
	HANDLE t_fromparent_read, t_fromparent_write;
	HANDLE t_toparent_read, t_toparent_write;
	HANDLE t_toparent_write_dup;
	t_fromparent_read = t_fromparent_write = nil;
	t_toparent_read = t_toparent_write = nil;
	t_toparent_write_dup = nil;
	if (t_success)
	{
		SECURITY_ATTRIBUTES t_attr;
		t_attr . nLength = sizeof(SECURITY_ATTRIBUTES);
		t_attr . bInheritHandle = TRUE;
		t_attr . lpSecurityDescriptor = NULL;
		if (!CreatePipe(&t_fromparent_read, &t_fromparent_write, &t_attr, 0) ||
			!CreatePipe(&t_toparent_read, &t_toparent_write, &t_attr, 0) ||
			!DuplicateHandle(GetCurrentProcess(), t_toparent_write, GetCurrentProcess(), &t_toparent_write_dup, 0, TRUE, DUPLICATE_SAME_ACCESS))
			t_success = false;
	}

	// Make sure the ends we are duplicating are not inheritable
	if (t_success)
	{
		SetHandleInformation(t_fromparent_write, HANDLE_FLAG_INHERIT, 0);
		SetHandleInformation(t_toparent_read, HANDLE_FLAG_INHERIT, 0);
	}

	// Duplicate the appropriate ends into the parent
	HANDLE t_parent_data_write, t_parent_data_read;
	t_parent_data_write = nil;
	t_parent_data_read = nil;
	if (t_success)
	{
		if (!DuplicateHandle(GetCurrentProcess(), t_fromparent_write, t_parent_process, &t_parent_data_write, 0, FALSE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE) ||
			!DuplicateHandle(GetCurrentProcess(), t_toparent_read, t_parent_process, &t_parent_data_read, 0, FALSE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE))
			t_success = false;
	}

	// Post the pipe handles to the parent
	if (t_success)
		PostThreadMessageA(t_parent_thread_id, WM_USER + 10, (WPARAM)t_parent_data_read, (LPARAM)t_parent_data_write);

	// Now we must read the command line and env strings
	uint32_t t_cmd_line_length;
	void *t_cmd_line;
	t_cmd_line_length = 0;
	t_cmd_line = nil;
	if (t_success)
		t_success = read_blob_from_pipe(t_fromparent_read, t_cmd_line, t_cmd_line_length);

	uint32_t t_env_strings_length;
	void *t_env_strings;
	t_env_strings_length = 0;
	t_env_strings = nil;
	if (t_success)
		t_success = read_blob_from_pipe(t_fromparent_read, t_env_strings, t_env_strings_length);

	// If we succeeded in reading those, we are all set to create the process
	HANDLE t_process_handle, t_thread_handle;
	DWORD t_process_id;
	t_thread_handle = NULL;
	t_process_handle = NULL;
	t_process_id = 0;
	if (t_success)
	{
		PROCESS_INFORMATION piProcInfo;
		STARTUPINFOA siStartInfo;
		memset(&siStartInfo, 0, sizeof(STARTUPINFOA));
		siStartInfo.cb = sizeof(STARTUPINFOA);
		siStartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		siStartInfo.wShowWindow = SW_HIDE;
		siStartInfo.hStdInput = t_fromparent_read;
		siStartInfo.hStdOutput = t_toparent_write;
		siStartInfo.hStdError = t_toparent_write_dup;
		if (CreateProcessA(NULL, (LPSTR)t_cmd_line, NULL, NULL, TRUE, CREATE_NEW_CONSOLE | CREATE_SUSPENDED, t_env_strings, NULL, &siStartInfo, &piProcInfo))
		{
			t_process_handle = piProcInfo . hProcess;
			t_process_id = piProcInfo . dwProcessId;
			t_thread_handle = piProcInfo . hThread;
		}
		else
		{
			DWORD t_error;
			t_error = GetLastError();
			t_success = false;
		}
	}

	// If the process started, then try to duplicate its handle into the parent
	HANDLE t_parent_process_handle;
	t_parent_process_handle = NULL;
	if (t_success)
	{
		if (!DuplicateHandle(GetCurrentProcess(), t_process_handle, t_parent_process, &t_parent_process_handle, 0, FALSE, DUPLICATE_SAME_ACCESS))
			t_success = false;
	}

	// Now tell the parent process about the handle and id
	if (t_success)
		PostThreadMessage(t_parent_thread_id, WM_USER + 10, (WPARAM)t_process_id, (LPARAM)t_parent_process_handle);

	// If everything happened as we expected, resume the process. Otherwise we
	// terminate it.
	if (t_success)
		ResumeThread(t_thread_handle);
	else
		TerminateProcess(t_process_handle, 0);

	// Free up our resources
	free(t_env_strings);
	free(t_cmd_line);
	if (t_thread_handle != nil)
		CloseHandle(t_thread_handle);
	if (t_process_handle != nil)
		CloseHandle(t_process_handle);
	if (t_fromparent_read != nil)
		CloseHandle(t_fromparent_read);
	if (t_toparent_write != nil)
		CloseHandle(t_toparent_write);
	if (t_toparent_write_dup != nil)
		CloseHandle(t_toparent_write_dup);
	if (t_parent_thread != nil)
		CloseHandle(t_parent_thread);
	if (t_parent_process != nil)
		CloseHandle(t_parent_process);

	return 0;
}

