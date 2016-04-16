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

#ifdef DeleteFile
#undef DeleteFile
#endif // DeleteFile
#ifdef GetCurrentTime
#undef GetCurrentTime
#endif // GetCurrentTime

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"
#include "mcio.h"
#include "system.h"

//#include "execpt.h"
#include "exec.h"
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
#include "scriptenvironment.h"
#include "text.h"
#include "notify.h"

#include "socket.h"

#include "w32dc.h"
#include "w32dsk-legacy.h"

#include <locale.h>
#include <sys/stat.h>
#include <time.h>
#include <Winsock2.h>
#include <Mstcpip.h>
#include <Mswsock.h>
#include <Shlobj.h>
#include <sys\Timeb.h>
#include <Iphlpapi.h>
#include <process.h>
#include <signal.h>
#include <io.h> 
#include <strsafe.h>
#include <Shlwapi.h>

//////////////////////////////////////////////////////////////////////////////////

int *g_mainthread_errno;

//////////////////////////////////////////////////////////////////////////////////

#define PATH_DELIMITER '\\'

//////////////////////////////////////////////////////////////////////////////////

extern bool MCFiltersUrlEncode(MCStringRef p_source, bool p_use_utf8, MCStringRef& r_result);
extern bool MCStringsSplit(MCStringRef p_string, codepoint_t p_separator, MCStringRef*&r_strings, uindex_t& r_count);

//////////////////////////////////////////////////////////////////////////////////

// This function is used to modify paths so that lengths > MAX_PATH characters
// are supported. It does, however, disable parsing of . and .. as well as
// converting / to \ so these must be done before calling this
static void legacy_path_to_nt_path(MCStringRef p_legacy, MCStringRef &r_nt)
{
    // Don't do anything if it isn't necessary
    if (MCStringGetLength(p_legacy) < (MAX_PATH - 1))
    {
        r_nt = MCValueRetain(p_legacy);
        return;
    }

	// UNC and local paths are treated differently
	if (MCStringGetCharAtIndex(p_legacy, 0) == '\\' && MCStringGetCharAtIndex(p_legacy, 1) == '\\')
	{
		// Check that an explicit NT path isn't already being specified
		if (MCStringGetCharAtIndex(p_legacy, 2) == '?'		// Legacy paths namespace
			|| MCStringGetCharAtIndex(p_legacy, 2) == '.')	// Device paths namespace
		{
			r_nt = MCValueRetain(p_legacy);
			return;
		}
		
		// UNC path
		MCStringRef t_temp;
		/* UNCHECKED */ MCStringCreateMutable(0, t_temp);
		/* UNCHECKED */ MCStringAppend(t_temp, MCSTR("\\\\?\\UNC\\"));
		/* UNCHECKED */ MCStringAppendSubstring(t_temp, p_legacy, MCRangeMake(2, MCStringGetLength(p_legacy) - 2));
		/* UNCHECKED */ MCStringCopyAndRelease(t_temp, r_nt);
	}
	else
	{
		// Local path
		/* UNCHECKED */ MCStringFormat(r_nt, "\\\\?\\%@", p_legacy);
	}
}

// This function strips the leading \\?\ or \\?\UNC\ from an NT-style path
static void nt_path_to_legacy_path(MCStringRef p_nt, MCStringRef &r_legacy)
{
	// Check for the leading NT path characters
	if (MCStringBeginsWithCString(p_nt, (const char_t*)"\\\\?\\UNC\\", kMCStringOptionCompareCaseless))
	{
		MCStringRef t_temp;
		/* UNCHECKED */ MCStringMutableCopySubstring(p_nt, MCRangeMake(8, MCStringGetLength(p_nt) - 8), t_temp);
		/* UNCHECKED */ MCStringPrepend(t_temp, MCSTR("\\\\"));
		/* UNCHECKED */ MCStringCopyAndRelease(t_temp, r_legacy);
	}
	else if (MCStringBeginsWithCString(p_nt, (const char_t*)"\\\\?\\", kMCStringOptionCompareCaseless))
	{
		/* UNCHECKED */ MCStringCopySubstring(p_nt, MCRangeMake(4, MCStringGetLength(p_nt) - 4), r_legacy);
	}
	else
	{
		// Not an NT-style path, no changes required.
		r_legacy = MCValueRetain(p_nt);
	}
}

static bool get_device_path(MCStringRef p_path, MCStringRef &r_device_path)
{
	// Device paths are opened as "COM<n>:" where <n> is an integer
	if (MCStringBeginsWithCString(p_path, (const char_t*)"COM", kMCStringOptionCompareCaseless)
		&& MCStringGetCharAtIndex(p_path, MCStringGetLength(p_path) - 1) == ':')
	{
		// Serial ports live in the NT device namespace (but don't have a ':' suffix)
		MCStringRef t_temp;
		return MCStringCreateMutable(0, t_temp)
                && MCStringAppend(t_temp, MCSTR("\\\\.\\"))
                && MCStringAppendSubstring(t_temp, p_path, MCRangeMake(0, MCStringGetLength(p_path) - 1))
                && MCStringCopyAndRelease(t_temp, r_device_path);
	}
    else
        return MCStringCopy(p_path, r_device_path);
}

//////////////////////////////////////////////////////////////////////////////////

// MW-2005-02-22: Make these global for opensslsocket.cpp
static Boolean wsainited = False;
HWND sockethwnd;
HANDLE g_socket_wakeup;

Boolean wsainit()
{
	if (!wsainited)
	{
		WORD request = MAKEWORD(1,1);
		WSADATA wsaData;
		if (WSAStartup(request, (LPWSADATA)&wsaData))
			MCresult->sets("can't find a usable winsock.dll");
		else
		{
			wsainited = True;
			
			// OK-2009-02-24: [[Bug 7628]]
			MCresult -> sets("");
#ifdef _WINDOWS_DESKTOP
			if (!MCnoui)
				sockethwnd = CreateWindowA(MC_WIN_CLASS_NAME, "MCsocket", WS_POPUP, 0, 0,
										   8, 8, NULL, NULL, MChInst, NULL);
#endif
		}
	}
	MCS_seterrno(0);
	return wsainited;
}

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

//////////////////////////////////////////////////////////////////////////////////

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

// MW-2004-11-28: A null FPE signal handler.
static void handle_fp_exception(int p_signal)
{
	p_signal = p_signal;
}

static bool handle_is_pipe(MCWinSysHandle p_handle)
{
	DWORD t_flags;

	int t_result;
	t_result = GetNamedPipeInfo((HANDLE)p_handle, &t_flags, NULL, NULL, NULL);

	return t_result != 0;
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
                                         {"sz", REG_SZ},
										 {"qword", REG_QWORD},
										 {"qwordlittleendian", REG_QWORD_LITTLE_ENDIAN}
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
	if (MCStringLastIndexOfChar(p_path, '\\', t_length, kMCStringOptionCompareExact, t_value_offset))
	{
		if (MCStringFirstIndexOfChar(p_path, '\\', 0, kMCStringOptionCompareExact, t_path_offset))
		{
			if (t_value_offset > t_path_offset)
				t_success = t_success && MCStringCopySubstring(p_path, MCRangeMake(t_path_offset + 1, t_value_offset - t_path_offset - 1), r_key);
		}
		t_success = t_success && MCStringCopySubstring(p_path, MCRangeMake(t_value_offset + 1, t_length - t_value_offset - 1), r_value);
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
			r_string = MCSTR(RegDatatypes[i].token);
			return true;
		}
	}

    // SN-2014-11-18: [[ Bug 14052 ]] Avoid to return a nil string (the result is not checked anyway).
    r_string = MCValueRetain(kMCEmptyString);
	return false;
}

MCSRegistryValueType MCS_registry_type_from_string(MCStringRef p_string)
{
	if (p_string != nil)
	{
		for (uindex_t i = 0; i < ELEMENTS(RegDatatypes); i++)
		{
			if (MCStringIsEqualToCString(p_string, RegDatatypes[i].token, kMCCompareCaseless))
				return (MCSRegistryValueType)RegDatatypes[i].type;
		}
	}

	return kMCSRegistryValueTypeSz;
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

///////////////////////////////////////////////////////////////////////////////
static MMRESULT tid;

void CALLBACK MCS_tp(UINT id, UINT msg, DWORD user, DWORD dw1, DWORD dw2)
{
	MCalarm = True;
}

///////////////////////////////////////////////////////////////////////////////

#define DEFINE_GUID_(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
// {F0B7A1A2-9847-11cf-8F20-00805F2CD064}
DEFINE_GUID_(CATID_ActiveScriptParse, 0xf0b7a1a2, 0x9847, 0x11cf, 0x8f, 0x20, 0x00, 0x80, 0x5f, 0x2c, 0xd0, 0x64);

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////

/* thread created to read data from child process's pipe */
static bool s_finished_reading = false;

static void readThreadDone(void *param)
{
	s_finished_reading = true;
}

static DWORD readThread(Streamnode *process)
{
	DWORD nread;
	uint32_t t_bufsize = READ_PIPE_SIZE;
	char* t_buffer = new char[t_bufsize];

	while (process -> ihandle != NULL)
	{
		if (!PeekNamedPipe(process -> ihandle -> GetFilePointer(), NULL, 0, NULL, &nread, NULL))
			break;
		if (nread == 0)
		{
			if (WaitForSingleObject(process -> phandle, 0) != WAIT_TIMEOUT)
				break;
			Sleep(100);
			continue;
		}
		if (!ReadFile(process -> ihandle -> GetFilePointer(), (LPVOID)t_buffer,
				t_bufsize, &nread, NULL)
		        || nread == 0)
					break;
		
		/* UNCHECKED */ process -> ohandle -> Write(t_buffer, nread);
	}

	delete[] t_buffer;
	MCNotifyPush(readThreadDone, nil, false, false);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////

bool MCS_getcurdir_native(MCStringRef& r_path)
{
	// NOTE:
	// Get/SetCurrentDirectory are not supported by Windows in multithreaded environments
	MCAutoArray<unichar_t> t_buffer;

	// Retrieve the length of the current directory
	DWORD t_path_len = GetCurrentDirectoryW(0, NULL);
	/* UNCHECKED */ t_buffer.New(t_path_len);

	DWORD t_result = GetCurrentDirectoryW(t_path_len, t_buffer.Ptr());
	if (t_result == 0 || t_result >= t_path_len)
	{
		// Something went wrong
		return false;
	}

	return MCStringCreateWithChars(t_buffer.Ptr(), t_result, r_path);
}

bool MCS_native_path_exists(MCStringRef p_path, bool p_is_file)
{
	MCAutoStringRefAsWString t_path_wstr;
	/* UNCHECKED */ t_path_wstr.Lock(p_path);
	
	// MW-2010-10-22: [[ Bug 8259 ]] Use a proper Win32 API function - otherwise network shares don't work.
	DWORD t_attrs;
	t_attrs = GetFileAttributesW(*t_path_wstr);

	if (t_attrs == INVALID_FILE_ATTRIBUTES)
		return false;

	return p_is_file == ((t_attrs & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

bool MCS_path_exists(MCStringRef p_path, bool p_is_file)
{
#ifdef /* MCS_exists_dsk_w32 */ LEGACY_SYSTEM
	char *newpath = MCS_resolvepath(path);
	//MS's stat() fails if there is a trailing '\\'. Workaround is to delete it
	// MW-2004-04-20: [[ Purify ]] If *newpath == 0 then we should return False
	if (*newpath == '\0')
	{
		delete newpath;
		return False;
	}
	
	// MW-2008-01-15: [[ Bug 4981 ]] - It seems that stat will fail for checking
	//   a folder 'C:' and requires that it be 'C:\'
	if (strlen(newpath) == 2 && newpath[1] == ':')
	{
		// newpath is of form "<driveletter>:"
		char *t_modified_path;
		t_modified_path = new char[strlen(newpath) + 2];
		strcpy(t_modified_path, newpath);
		strcat(t_modified_path, "\\");
		delete newpath;
		newpath = t_modified_path;
		// newpath is of form "<driverletter>:\"
	}
    
	// OK-2007-12-05 : Bug 5555, modified to allow paths with trailing backslashes on Windows.
	if ((newpath[strlen(newpath) - 1] == '\\' || newpath[strlen(newpath) - 1] == '/')
        && (strlen(newpath) != 3 || newpath[1] != ':'))
		newpath[strlen(newpath) - 1] = '\0';
    
	// MW-2010-10-22: [[ Bug 8259 ]] Use a proper Win32 API function - otherwise network shares don't work.
	DWORD t_attrs;
	t_attrs = GetFileAttributesA(newpath);
	delete newpath;
    
	if (t_attrs == INVALID_FILE_ATTRIBUTES)
		return False;
    
	return file == ((t_attrs & FILE_ATTRIBUTE_DIRECTORY) == 0);
#endif /* MCS_exists_dsk_w32 */
    // MW-2004-04-20: [[ Purify ]] If *newpath == 0 then we should return False
    if (MCStringGetLength(p_path) == 0)
        return False;
    
    // MW-2008-01-15: [[ Bug 4981 ]] - It seems that stat will fail for checking
    //   a folder 'C:' and requires that it be 'C:\'
	// TODO: still necessary with GetFileAttributes instead of stat?
    /*if (MCStringGetLength(p_path) == 2 && MCStringGetCharAtIndex(p_path, 1) == ':')
    {
        MCAutoStringRef t_drive_string;
        return MCStringMutableCopy(p_path, &t_drive_string) &&
        MCStringAppendChar(*t_drive_string, '\\') &&
        MCS_native_path_exists(*t_drive_string, p_is_file);
    }*/
    
    // OK-2007-12-05 : Bug 5555, modified to allow paths with trailing backslashes on Windows.
	// TODO: still necessary with GetFileAttributes instead of stat?
    uindex_t t_path_len = MCStringGetLength(p_path);
    unichar_t t_last_char = MCStringGetCharAtIndex(p_path, t_path_len - 1);
    if ((t_last_char == '\\' || t_last_char == '/') &&
        !(t_path_len == 3 && MCStringGetCharAtIndex(p_path, 1) == ':'))
    {
        MCAutoStringRef t_trimmed_string;
        return MCStringCopySubstring(p_path, MCRangeMake(0, t_path_len - 1), &t_trimmed_string) &&
        MCS_native_path_exists(*t_trimmed_string, p_is_file);
    }
    
    return MCS_native_path_exists(p_path, p_is_file);
}

bool MCS_path_append(MCStringRef p_base, unichar_t p_separator, MCStringRef p_component, MCStringRef& r_path)
{
	MCAutoStringRef t_path;
	if (!MCStringMutableCopy(p_base, &t_path))
		return false;
    
	if (MCStringGetCharAtIndex(p_base, MCStringGetLength(p_base) - 1) != p_separator &&
		!MCStringAppendChars(*t_path, &p_separator, 1))
		return false;
    
	if (!MCStringAppend(*t_path, p_component))
		return false;
	
	r_path = MCValueRetain(*t_path);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////

static inline bool is_legal_drive(char p_char)
{
	return (p_char >= 'a' && p_char <= 'z') || (p_char >= 'A' && p_char <= 'Z');
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

#define NAMESERVER_REG_KEY "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\NameServer"
bool dns_servers_from_registry(MCListRef& r_list)
{
	MCAutoStringRef t_key, t_type, t_error;
	MCAutoValueRef t_value;
	t_key = MCSTR(NAMESERVER_REG_KEY);
	if (!MCS_query_registry(*t_key, &t_value, &t_type, &t_error))
		return false;
    
	if (!MCStringIsEmpty(*t_error))
	{
		r_list = MCValueRetain(kMCEmptyList);
		return true;
	}
    
	// If the type is wrong, abort
	if (MCValueGetTypeCode(*t_value) != kMCValueTypeCodeString)
		return false;

	MCStringRef t_string = (MCStringRef)*t_value;

	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;
    
	const unichar_t *t_chars;
	uindex_t t_char_count;
	t_chars = MCStringGetCharPtr(t_string);
	t_char_count = MCStringGetLength(t_string);
    
	uindex_t t_start = 0;
    
	for (uindex_t i = 0; i < t_char_count; i++)
	{
		if (t_chars[i] == ' ' || t_chars[i] == ',' || t_chars[i] == '\n')
		{
			MCAutoStringRef t_substring;
			if (!MCStringCopySubstring(t_string, MCRangeMake(t_start, i - t_start), &t_substring) || 
					!MCListAppend(*t_list, *t_substring))
				return false;
			t_start = i + 1;
		}
	}
	if (t_start < t_char_count)
	{
		MCAutoStringRef t_final_string;
		if (!MCStringCopySubstring(t_string, MCRangeMake(t_start, MCStringGetLength(t_string) - t_start), &t_final_string) ||
			!MCListAppend(*t_list, *t_final_string))
			return false;
	}
    
	return MCListCopy(*t_list, r_list);
}

//////////////////////////////////////////////////////////////////////////////////

static void MCS_do_launch(MCStringRef p_document)
{
	MCAutoStringRefAsWString t_document_wstr;
	/* UNCHECKED */ t_document_wstr.Lock(p_document);
	
	// MW-2011-02-01: [[ Bug 9332 ]] Adjust the 'show' hint to normal to see if that makes
	//   things always appear on top...
	int t_result;
	t_result = (int)ShellExecuteW(NULL, L"open", *t_document_wstr, NULL, NULL, SW_SHOWNORMAL);

	if (t_result < 32)
	{
		switch(t_result)
		{
		case ERROR_BAD_FORMAT:
		case SE_ERR_ACCESSDENIED:
		case SE_ERR_FNF:
		case SE_ERR_PNF:
		case SE_ERR_SHARE:
		case SE_ERR_DLLNOTFOUND:
			MCresult -> sets("can't open file");
		break;

		case SE_ERR_ASSOCINCOMPLETE:
		case SE_ERR_NOASSOC:
			MCresult -> sets("no association");
		break;

		case SE_ERR_DDEBUSY:
		case SE_ERR_DDEFAIL:
		case SE_ERR_DDETIMEOUT:
			MCresult -> sets("request failed");
		break;

		case 0:
		case SE_ERR_OOM:
			MCresult -> sets("no memory");
		break;
		}
	}
	else
		MCresult -> clear();
}

//////////////////////////////////////////////////////////////////////////////////
struct MCWindowsSystemService: public MCWindowsSystemServiceInterface
{
    virtual bool MCISendString(MCStringRef p_command, MCStringRef& r_result, bool& r_error)
    {
        MCAutoStringRefAsWString t_command_wstr;
		/* UNCHECKED */ t_command_wstr.Lock(p_command);
		
		DWORD t_error_code;
        WCHAR t_buffer[256];
        t_buffer[0] = '\0';
        t_error_code = mciSendStringW(*t_command_wstr, t_buffer, 255, NULL);
        r_error = t_error_code != 0;
        if (!r_error)
		{
			size_t t_length;
			if (SUCCEEDED(StringCchLengthW(t_buffer, 256, &t_length)))
				return MCStringCreateWithChars(t_buffer, t_length, r_result);
		}
        
        size_t t_length;
		return mciGetErrorStringW(t_error_code, t_buffer, 255) &&
			SUCCEEDED(StringCchLengthW(t_buffer, 256, &t_length)) &&
			MCStringCreateWithChars(t_buffer, t_length, r_result);
    }
    
    virtual bool QueryRegistry(MCStringRef p_key, MCValueRef& r_value, MCStringRef& r_type, MCStringRef& r_error)
    {
        MCAutoStringRef t_root, t_key, t_value;
        
        r_value = r_error = nil;
        
        //key = full path info such as: HKEY_LOCAL_MACHINE\\Software\\..\\valuename
		if (!MCS_registry_split_key(p_key, &t_root, &t_key, &t_value))
            return false;
        
        if (*t_value == nil)
        {
            /* RESULT */ //MCresult->sets("no key");
			r_error = MCSTR("no key");
			r_value = MCValueRetain(kMCEmptyString);
			return true;
        }
        
        if (*t_key == nil)
            t_key = kMCEmptyString;
        
        HKEY t_hkey;
        MCAutoRegistryKey t_regkey;
        
        uint32_t t_access_mode = KEY_READ;
        
		MCAutoStringRefAsWString t_key_wstr;
		/* UNCHECKED */ t_key_wstr.Lock(*t_key);
		
		if (!MCS_registry_root_to_hkey(*t_root, t_hkey, t_access_mode) ||
            (RegOpenKeyExW(t_hkey, *t_key_wstr, 0, t_access_mode, &t_regkey) != ERROR_SUCCESS))
        {
            /* RESULT */ //MCresult->sets("bad key");
			r_error = MCSTR("bad key");
			return true;
        }
        
        LONG err = 0;
		MCAutoArray<BYTE> t_buffer;
        DWORD t_buffer_len = 0;
        DWORD t_type;
        
		MCAutoStringRefAsWString t_value_wstr;
		/* UNCHECKED */ t_value_wstr.Lock(*t_value);

        //determine the size of value buffer needed
		err = RegQueryValueExW(*t_regkey, *t_value_wstr, 0, NULL, NULL, &t_buffer_len);
		if (err == ERROR_SUCCESS)
        {
            if (!t_buffer.New(t_buffer_len))
                return false;
			if ((err = RegQueryValueExW(*t_regkey, *t_value_wstr, 0, &t_type, t_buffer.Ptr(), &t_buffer_len))
				== ERROR_SUCCESS && t_buffer_len > 0)
            {
				// Sets the type
				MCS_registry_type_to_string(t_type, r_type);

				// Convert the value from the registry to the appropriate value type
				switch (t_type)
				{
				case REG_LINK:
					if (!MCStringCreateWithChars((const unichar_t*)t_buffer.Ptr(), t_buffer_len/2, (MCStringRef&)r_value))
						return false;
					break;

				case REG_EXPAND_SZ:
				case REG_MULTI_SZ:
				case REG_SZ:
					{						
						DWORD t_unicode_len;
						t_unicode_len = t_buffer_len / 2;

						unichar_t *t_chars = (unichar_t*)t_buffer.Ptr();
						// Get rid of any trailing NULL character
						while(t_unicode_len > 0 && t_chars[t_unicode_len - 1] == '\0')
							t_unicode_len -= 1;
					
						if (t_type == REG_MULTI_SZ && t_unicode_len < t_buffer_len / 2)
							t_unicode_len += 1;
         
						return MCStringCreateWithChars((unichar_t*)t_buffer.Ptr(), t_unicode_len, (MCStringRef&)r_value);
					}

				case REG_NONE:
					{
						// No data. Return an empty string ref as the nearest equivalent
						r_value = MCValueRetain(kMCEmptyString);
						break;
					}
					
				case REG_DWORD:
				case REG_QWORD:
				case REG_BINARY:
				default:
					{
						// Binary or unsupported type. Return as a binary blob
						// (For compatibility with existing scripts, DWORD and QWORD are binary)
						MCAutoDataRef t_data;
						if (!MCDataCreateWithBytes((const byte_t*)t_buffer.Ptr(), t_buffer_len, (MCDataRef&)t_data))
							return false;

						r_value = MCValueRetain(*t_data);
						break;
					}
				}
            }
        }
        else if (err == ERROR_FILE_NOT_FOUND)
        {
            // The query may have been for a value that is really a key, either
            // with or without a default/un-named value attached to it. Try
            // opening the path to the value as a key.
            MCAutoStringRef t_alt_key;
            MCAutoStringRefAsWString t_alt_key_wstr;
            if (MCStringIsEmpty(*t_key))
                t_alt_key = *t_value;
            else
                /* UNCHECKED */ MCStringFormat(&t_alt_key, "%@\\%@", *t_key, *t_value);
            /* UNCHECKED */ t_alt_key_wstr.Lock(*t_alt_key);
            
            MCAutoRegistryKey t_alt_regkey;
            err = RegOpenKeyExW(t_hkey, *t_alt_key_wstr, 0, t_access_mode, &t_alt_regkey);
            if (err == ERROR_SUCCESS)
            {
                // Key exists
                r_value = MCValueRetain(kMCEmptyString);
                MCS_registry_type_to_string(REG_NONE, r_type);
            }
        }
        
        if (err != ERROR_SUCCESS)
        {
            errno = err;
            r_error = MCSTR("can't find key");
            return true;
        }
        
        return true;
    }
    
    virtual bool SetRegistry(MCStringRef p_key, MCValueRef p_value, MCSRegistryValueType p_type, MCStringRef& r_error)
    {
        MCAutoStringRef t_root, t_key, t_value;
        HKEY t_root_hkey;
        
		if (!MCS_registry_split_key(p_key, &t_root, &t_key, &t_value))
            return false;
        
        if (!MCS_registry_root_to_hkey(*t_root, t_root_hkey))
        {
            /* RESULT */ //MCresult->sets("bad key");
			r_error = MCSTR("bad key");
			return true;
        }
        if (*t_key == nil)
        {
            /* RESULT */ //MCresult->sets("bad key specified");
			r_error = MCSTR("bad key specified");
			return true;
        }
        
        MCAutoRegistryKey t_regkey;
        DWORD t_keystate;
        
		MCAutoStringRefAsWString t_key_wstr;
		/* UNCHECKED */ t_key_wstr.Lock(*t_key);

        if (RegCreateKeyExW(t_root_hkey, *t_key_wstr, 0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &t_regkey, &t_keystate) != ERROR_SUCCESS)
        {
            MCS_seterrno(GetLastError());
            /* RESULT */ //MCresult->sets("can't create key");
			r_error = MCSTR("can't create key");
			return true;
        }
        
		MCAutoStringRefAsWString t_value_wstr;
		/* UNCHECKED */ t_value_wstr.Lock(*t_value);

        if (MCValueIsEmpty(p_value))
        {//delete this value
            if ((errno = RegDeleteValueW(*t_regkey, *t_value_wstr)) != ERROR_SUCCESS)
            {
                MCS_seterrno(GetLastError());
                /* RESULT */ //MCresult->sets("can't delete value");
				r_error = MCSTR("can't delete value");
            }
            return true;
        }
        else
        {
            // The type enumeration is designed to have values equivalent to the REG_* values
			DWORD t_type;
            t_type = (DWORD)p_type;

			switch (t_type)
			{
			case REG_SZ:
			case REG_EXPAND_SZ:
			case REG_MULTI_SZ:
			case REG_LINK:
				{
					// Expecting a StringRef type
					if (MCValueGetTypeCode(p_value) != kMCValueTypeCodeString)
						return false;

					MCAutoStringRefAsWString t_wstring;
					/* UNCHECKED */ t_wstring.Lock((MCStringRef)p_value);

					// NOTE: for string types, the length *must* include the terminating NULL
					if (RegSetValueExW(*t_regkey, *t_value_wstr, 0, t_type, 
						(const BYTE*)*t_wstring, (MCStringGetLength((MCStringRef)p_value) + 1) * sizeof(unichar_t)) == ERROR_SUCCESS)
					{
						r_error = nil;
						return true;
					}
					break;
				}

			case REG_NONE:
				{
					// There is no value to set but the type still needs to be specified
					if (RegSetValueExW(*t_regkey, *t_value_wstr, 0, t_type, NULL, 0) == ERROR_SUCCESS)
					{
						r_error = nil;
						return true;
					}
					break;
				}

			case REG_BINARY:
			case REG_DWORD:
			case REG_DWORD_BIG_ENDIAN:
			case REG_QWORD:
			default:
				{
					// Expecting a DataRef type
					if (MCValueGetTypeCode(p_value) != kMCValueTypeCodeData)
						return false;

					if (RegSetValueExW(*t_regkey, *t_value_wstr, 0, t_type,
						MCDataGetBytePtr((MCDataRef)p_value), MCDataGetLength((MCDataRef)p_value)) == ERROR_SUCCESS)
					{
						r_error = nil;
						return true;
					}
					break;
				}
			}
            
			// If we get to this point, something failed
			MCS_seterrno(GetLastError());
			r_error = MCSTR("can't set value");
			return true;
        }
        
		// Shouldn't get here
		MCAssert(false);
		return false;
    }
    
    virtual bool DeleteRegistry(MCStringRef p_key, MCStringRef& r_error)
    {
        MCAutoStringRef t_root, t_key;
        
        if (!MCS_registry_split_key(p_key, &t_root, &t_key))
            return false;
        
        HKEY hkey = NULL;
        
        if (MCStringIsEmpty(*t_key))
        {
            /* RESULT */ //MCresult->sets("no key");
			r_error = MCSTR("no key");
            return true;
        }
        
		if (!MCS_registry_root_to_hkey(*t_root, hkey))
        {
            /* RESULT */ //MCresult->sets("bad key");
			r_error = MCSTR("bad key");
			return true;
        }

		MCAutoStringRefAsWString t_key_wstr;
		/* UNCHECKED */ t_key_wstr.Lock(*t_key);

        LSTATUS t_error;
		t_error = SHDeleteKeyW(hkey, *t_key_wstr);

        if (t_error != ERROR_SUCCESS)
        {
            /* RESULT */ //MCresult->sets("could not delete key");
			r_error = MCSTR("could not delete key");
			return true;
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

		MCAutoStringRefAsWString t_key_wstr;
		/* UNCHECKED */ t_key_wstr.Lock(*t_key);

        if (!MCS_registry_root_to_hkey(*t_root, t_hkey) ||
            RegOpenKeyExW(t_hkey, *t_key_wstr, 0, KEY_READ, &t_regkey) != ERROR_SUCCESS)
        {
            r_error = MCSTR("bad key");
			return true;
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
};

struct MCMemoryMappedFileHandle: public MCMemoryFileHandle
{
	MCMemoryMappedFileHandle(MCWinSysHandle p_handle, const void *p_buffer, uint32_t p_length):
		MCMemoryFileHandle(p_buffer, p_length)
	{
		m_handle = p_handle;
	}

	virtual void Close()
	{
#ifdef /* MCS_close_dsk_w32 */ LEGACY_SYSTEM
	if (stream->buffer != NULL)
	{ //memory map file
		if (stream->mhandle != NULL)
		{
			UnmapViewOfFile(stream->buffer);
			CloseHandle(stream->mhandle);
		}
	}
	if (!(stream->flags & IO_FAKE))
		CloseHandle(stream->fhandle);
	delete stream;
	stream = NULL;
    return IO_NORMAL;
#endif /* MCS_close_dsk_w32 */
		/* UNCHECKED */ UnmapViewOfFile(m_buffer);
		CloseHandle((HANDLE)m_handle);
		MCMemoryFileHandle::Close();
	}

private:
	MCWinSysHandle m_handle;
};


struct MCStdioFileHandle: public MCSystemFileHandle
{
	MCStdioFileHandle(MCWinSysHandle p_handle, bool p_is_pipe = false)
	{
		m_handle = p_handle;
		m_is_eof = false;
		m_is_pipe = p_is_pipe;
		m_putback = -1;
	}

	virtual void Close(void)
	{
		CloseHandle(m_handle);
		delete this;
	}
	
    virtual bool IsExhausted(void)
	{
		return m_is_eof;
	}
	
	virtual bool Read(void *p_buffer, uint32_t p_byte_size, uint32_t& r_read)
	{
#ifdef /* MCS_read_dsk_w32 */ LEGACY_SYSTEM
	if (MCabortscript || ptr == NULL || stream == NULL)
		return IO_ERROR;

	if ((stream -> flags & IO_FAKEWRITE) == IO_FAKEWRITE)
		return IO_ERROR;

	// MW-2009-06-25: If this is a custom stream, call the appropriate callback.
	// MW-2009-06-30: Refactored to common (platform-independent) implementation
	//   in mcio.cpp
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_read(ptr, size, n, stream);

	LPVOID sptr = ptr;
	DWORD nread;
	Boolean result = False;
	IO_stat istat = IO_NORMAL;

	if (stream->buffer != NULL)
	{ //memory map file or process with a thread
		uint4 nread = size * n;     //into the IO_handle's buffer
		if (nread > stream->len - (stream->ioptr - stream->buffer))
		{
			n = (stream->len - (stream->ioptr - stream->buffer)) / size;
			nread = size * n;
			istat = IO_EOF;
		}
		if (nread == 1)
		{
			char *tptr = (char *)ptr;
			*tptr = *stream->ioptr++;
		}
		else
		{
			memcpy(ptr, stream->ioptr, nread);
			stream->ioptr += nread;
		}
		return istat;
	}
	
	if (stream -> fhandle == 0)
	{
		MCS_seterrno(GetLastError());
		n = 0;
		return IO_ERROR;
	}
	
	// If this is named pipe, handle things differently -- we first peek to see how
	// much is available to read.
	// MW-2012-09-10: [[ Bug 10230 ]] If this stream is a pipe then handle that case.
	if (stream -> is_pipe)
	{
		// See how much data is available - if this fails then return eof or an error
		// depending on 'GetLastError()'.
		uint32_t t_available;
		if (!PeekNamedPipe(stream -> fhandle, NULL, 0, NULL, (DWORD *)&t_available, NULL))
		{
			n = 0;

			DWORD t_error;
			t_error = GetLastError();
			if (t_error == ERROR_HANDLE_EOF || t_error == ERROR_BROKEN_PIPE)
			{
				stream -> flags |= IO_ATEOF;
				return IO_EOF;
			}

			MCS_seterrno(GetLastError());
			return IO_ERROR;
		}

		// Adjust for putback
		int32_t t_adjust;
		t_adjust = 0;
		if (stream -> putback != -1)
			t_adjust = 1;

		// Calculate how many elements we can read, and how much we need to read
		// to make them.
		uint32_t t_count, t_byte_count;
		t_count = MCU_min((t_available + t_adjust) / size, n);
		t_byte_count = t_count * size;

		// Copy in the putback char if any
		uint1 *t_dst_ptr;
		t_dst_ptr = (uint1*)sptr;
		if (stream -> putback != -1)
		{
			*t_dst_ptr++ = (uint1)stream -> putback;
			stream -> putback = -1;
		}

		// Now read all the data we can - here we check for EOF also.
		uint32_t t_amount_read;
		IO_stat t_stat;
		t_stat = IO_NORMAL;
		t_amount_read = 0;
		if (t_byte_count - t_adjust > 0)
			if (!ReadFile(stream -> fhandle, (LPVOID)t_dst_ptr, t_byte_count - t_adjust, (DWORD *)&t_amount_read, NULL))
			{
				if (GetLastError() == ERROR_HANDLE_EOF)
				{
					stream -> flags |= IO_ATEOF;
					t_stat = IO_EOF;
				}
				else
				{
					MCS_seterrno(GetLastError());
					t_stat = IO_ERROR;
				}
			}

		// Return the number of objects of 'size' bytes that were read.
		n = (t_amount_read + t_adjust) / size;

		return t_stat;
	}

	if (stream -> putback != -1)
	{
		*((uint1 *)sptr) = (uint1)stream -> putback;
		stream -> putback = -1;
		
		if (!ReadFile(stream -> fhandle, (LPVOID)((char *)sptr + 1), (DWORD)size * n - 1, &nread, NULL))
		{
			MCS_seterrno(GetLastError());
			n = (nread + 1) / size;
			return IO_ERROR;
		}
		
		nread += 1;
	}
	else if (!ReadFile(stream->fhandle, (LPVOID)sptr, (DWORD)size * n, &nread, NULL))
	{
		MCS_seterrno(GetLastError());
		n = nread / size;
		return IO_ERROR;
	}

	if (nread < size * n)
	{
		stream->flags |= IO_ATEOF;
		n = nread / size;
		return IO_EOF;
	}
	else
		stream->flags &= ~IO_ATEOF;

	n = nread / size;
	return IO_NORMAL;
#endif /* MCS_read_dsk_w32 */
		LPVOID sptr = p_buffer;
		DWORD nread;
		Boolean result = False;
		IO_stat istat = IO_NORMAL;
		
		if (m_handle == 0)
		{
			MCS_seterrno(GetLastError());
			r_read = 0;
			return false;
		}
		
		// If this is named pipe, handle things differently -- we first peek to see how
		// much is available to read.
		// MW-2012-09-10: [[ Bug 10230 ]] If this stream is a pipe then handle that case.
		if (m_is_pipe)
		{
			// See how much data is available - if this fails then return eof or an error
			// depending on 'GetLastError()'.
			uint32_t t_available;
			if (!PeekNamedPipe(m_handle, NULL, 0, NULL, (DWORD *)&t_available, NULL))
			{
				r_read = 0;

				DWORD t_error;
				t_error = GetLastError();
				if (t_error == ERROR_HANDLE_EOF || t_error == ERROR_BROKEN_PIPE)
				{
					m_is_eof = true;
					return false;
				}

				m_is_eof = false;

				MCS_seterrno(GetLastError());
				return false;
			}

			// Adjust for putback
			int32_t t_adjust;
			t_adjust = 0;
			if (m_putback != -1)
				t_adjust = 1;

			// Calculate how many elements we can read, and how much we need to read
			// to make them.
			uint32_t t_byte_count;
			t_byte_count = MCU_min((t_available + t_adjust), p_byte_size);

			// Copy in the putback char if any
			uint1 *t_dst_ptr;
			t_dst_ptr = (uint1*)sptr;
			if (m_putback != -1)
			{
				*t_dst_ptr++ = (uint1)m_putback;
				m_putback = -1;
			}

			// Now read all the data we can - here we check for EOF also.
			uint32_t t_amount_read;
			IO_stat t_stat;
			t_stat = IO_NORMAL;
			t_amount_read = 0;
			if (t_byte_count - t_adjust > 0)
				if (!ReadFile(m_handle, (LPVOID)t_dst_ptr, t_byte_count - t_adjust, (DWORD *)&t_amount_read, NULL))
				{
					if (GetLastError() == ERROR_HANDLE_EOF)
					{
						t_stat = IO_EOF;
					}
					else
					{
						MCS_seterrno(GetLastError());
						t_stat = IO_ERROR;
					}
				}

			// Return the number of bytes that were read.
			r_read = (t_amount_read + t_adjust);

			if (t_stat == IO_EOF)
			{
				m_is_eof = true;
				return false;
			}

			m_is_eof = false;

			if (t_stat != IO_NORMAL)
				return false;

			return true;
		}

		if (m_putback != -1)
		{
			*((uint1 *)sptr) = (uint1)m_putback;
			m_putback = -1;
			
			if (!ReadFile(m_handle, (LPVOID)((char *)sptr + 1), (DWORD)p_byte_size - 1, &nread, NULL))
			{
				MCS_seterrno(GetLastError());
				r_read = (nread + 1);
				return false;
			}
			
			nread += 1;
		}
		else
		{
			// The Win32 ReadFile call has an annoying limitation - it cannot read more
			// than 64MB in each call. Just in case there are additional circumstances
			// that lower it further, perform the read 4MB at a tile.
			BOOL t_read_success;
			DWORD t_offset, t_remaining;
			t_read_success = TRUE;
			t_offset = 0;
			t_remaining = p_byte_size;
			while (t_read_success && t_remaining > 0)
			{
				// Only read up to 4MB at a time.
				DWORD t_readsize;
				t_readsize = MCU_min(t_remaining, 0x00400000);
				
				t_read_success = ReadFile(m_handle, (LPVOID)((char*)sptr + t_offset), t_readsize, &nread, NULL);
				t_offset += nread;
				if (!t_read_success)
				{
					MCS_seterrno(GetLastError());
					r_read = t_offset;
					return false;
				}
				
				// SN-2014-08-11: [[ Bug 13145 ]] If ReadFile can't read more, but no error is triggered, we should stop here,
				//  but return true. The new imageLoader reads buffer by buffer, and doesn't expect and error when reading the
				//  the last buffer (which might ask for more than remaining in the file).
				// IM-2015-03-17: [[ Bug 14960 ]] GetLastError is only meaningful if ReadFile fails. A return value of TRUE
				//  with 0 bytes read is used to indicate EOF for synchronous file handles.
				if (nread == 0)
				{
					r_read = t_offset;
					m_is_eof = true;
					return true;
				}

				t_remaining -= nread;
			}
			nread = t_offset;
		}

		if (nread < p_byte_size)
		{
			r_read = nread;
			m_is_eof = true;
			return false;
		}
 
		m_is_eof = false;
		r_read = nread;
		return true;
	}

	virtual bool Write(const void *p_buffer, uint32_t p_length)
	{
#ifdef /* MCS_write_dsk_w32 */ LEGACY_SYSTEM
	if (stream == IO_stdin)
		return IO_NORMAL;

	if (stream == NULL)
		return IO_ERROR;

	if ((stream -> flags & IO_FAKEWRITE) == IO_FAKEWRITE)
		return MCU_dofakewrite(stream -> buffer, stream -> len, ptr, size, n);

	if (stream -> fhandle == 0)
		return IO_ERROR;

	DWORD nwrote;
	if (!WriteFile(stream->fhandle, (LPVOID)ptr, (DWORD)size * n,
	               &nwrote, NULL))
	{
		MCS_seterrno(GetLastError());
		return IO_ERROR;
	}
	if (nwrote != size * n)
		return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_write_dsk_w32 */
		uint32_t t_written;
		if (this == IO_stdin)
			return true; // Shouldn't it return false???

		if (m_handle == NULL)
			return false;

		if (!WriteFile(m_handle, (LPVOID)p_buffer, (DWORD)p_length,
					   (LPDWORD)&t_written, NULL))
		{
			MCS_seterrno(GetLastError());
			return false;
		}
		if (t_written != p_length)
			return false;

		return true;
	}

	virtual bool Seek(int64_t p_offset, int p_dir)
	{
#ifdef /* MCS_seek_cur_dsk_w32 */ LEGACY_SYSTEM
	IO_stat is = IO_NORMAL;

	// MW-2009-06-25: If this is a custom stream, call the appropriate callback.
	// MW-2009-06-30: Refactored to common implementation in mcio.cpp.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_seek_cur(stream, offset);

	if (stream->buffer != NULL)
		IO_set_stream(stream, stream->ioptr + offset);
	else
		is = MCS_seek_do(stream -> fhandle, offset, FILE_CURRENT);
	return is;
#endif /* MCS_seek_cur_dsk_w32 */
#ifdef /* MCS_seek_set_dsk_w32 */ LEGACY_SYSTEM
	// MW-2009-06-30: If this is a custom stream, call the appropriate callback.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_seek_set(stream, offset);

	IO_stat is = IO_NORMAL;
	if (stream->buffer != NULL)
		IO_set_stream(stream, stream->buffer + offset);
	else
		is = MCS_seek_do(stream -> fhandle, offset, FILE_BEGIN);
	return is;
#endif /* MCS_seek_set_dsk_w32 */
#ifdef /* MCS_seek_end_dsk_w32 */ LEGACY_SYSTEM
	IO_stat is = IO_NORMAL;
	if (stream->buffer != NULL)
		IO_set_stream(stream, stream->buffer + stream->len + offset);
	else
		is = MCS_seek_do(stream -> fhandle, offset, FILE_END);
	return is;
#endif /* MCS_seek_end_dsk_w32 */
#ifdef /* MCS_seek_do_dsk_w32 */ LEGACY_SYSTEM
	LONG high_offset;
	high_offset = (LONG)(p_offset >> 32);
	DWORD fp;
	fp = SetFilePointer(p_file, (LONG)(p_offset & 0xFFFFFFFF),
	                   &high_offset, p_type);
	if (fp == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_seek_do_dsk_w32 */
		LONG high_offset;
		high_offset = (LONG)(p_offset >> 32);
		DWORD fp;

		fp = SetFilePointer(m_handle, (LONG)(p_offset & 0xFFFFFFFF),
			&high_offset, p_dir > 0 ? FILE_BEGIN : (p_dir < 0 ? FILE_END : FILE_CURRENT));

		if (fp == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
			return false;

		return true;
	}
	
	virtual bool Truncate(void)
	{
#ifdef /* MCS_trunc_dsk_w32 */ LEGACY_SYSTEM
	if (SetEndOfFile(stream->fhandle))
		return IO_NORMAL;
	else
		return IO_ERROR;
#endif /* MCS_trunc_dsk_w32 */
		if (!SetEndOfFile(m_handle))
			return false;

		return true;
	}

	virtual bool Sync(void)
	{
#ifdef /* MCS_sync_dsk_w32 */ LEGACY_SYSTEM
	//get the current file position pointer
	LONG fph;
	fph = 0;
	DWORD fp = SetFilePointer(stream->fhandle, 0, &fph, FILE_CURRENT);
	if (fp == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return IO_ERROR;
	DWORD nfp = SetFilePointer(stream->fhandle, fp, &fph, FILE_BEGIN);
	if (nfp == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return IO_ERROR;
	else
		return IO_NORMAL;
#endif /* MCS_sync_dsk_w32 */
		//get the current file position pointer
		LONG fph;
		fph = 0;
		DWORD fp = SetFilePointer(m_handle, 0, &fph, FILE_CURRENT);
		if (fp == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
			return false;
		DWORD nfp = SetFilePointer(m_handle, fp, &fph, FILE_BEGIN);
		if (nfp == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
			return false;
		
		return true;
	}

	virtual bool Flush(void)
	{
#ifdef /* MCS_flush_dsk_w32 */ LEGACY_SYSTEM 
	//flush output buffer
	if (FlushFileBuffers(stream->fhandle) != NO_ERROR)
		return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_flush_dsk_w32 */ //flush output buffer
		// SN-2014-06-16 
		// FileFlushBuffer returns non-zero on success
		// (which is the opposite of NO_ERROR
		if (FlushFileBuffers(m_handle) == 0)
			return false;

		return true;
	}
	
	virtual bool PutBack(char p_char)
	{
#ifdef /* MCS_putback_dsk_w32 */ LEGACY_SYSTEM
	if (stream -> buffer != NULL)
		return MCS_seek_cur(stream, -1);

	if (stream -> putback != -1)
		return IO_ERROR;
		
	stream -> putback = c;
	
	return IO_NORMAL;
#endif /* MCS_putback_dsk_w32 */
		if (m_putback != -1)
			return false;
			
		m_putback = p_char;
		
		return true;
	}
	
	virtual int64_t Tell(void)
	{
#ifdef /* MCS_tell_dsk_w32 */ LEGACY_SYSTEM
	// MW-2009-06-30: If this is a custom stream, call the appropriate callback.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_tell(stream);

	if (stream->buffer != NULL)
		return stream->ioptr - stream->buffer;

	DWORD low;
	LONG high;
	high = 0;
	low = SetFilePointer(stream -> fhandle, 0, &high, FILE_CURRENT);
	return low | ((int64_t)high << 32);
#endif /* MCS_tell_dsk_w32 */
		DWORD low;
		LONG high;
		high = 0;
		low = SetFilePointer(m_handle, 0, &high, FILE_CURRENT);
		return low | ((int64_t)high << 32);
	}
	
	virtual void *GetFilePointer(void)
	{
		return m_handle;
	}

	virtual int64_t GetFileSize(void)
	{
#ifdef /* MCS_fsize_dsk_w32 */ LEGACY_SYSTEM
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_fsize(stream);

	if (stream->flags & IO_FAKE)
		return stream->len;
	else
	{
		DWORD t_high_word, t_low_word;
		t_low_word = GetFileSize(stream -> fhandle, &t_high_word);
		if (t_low_word != INVALID_FILE_SIZE || GetLastError() == NO_ERROR)
			return (int64_t)t_low_word | (int64_t)t_high_word << 32;
	}
	return 0;
#endif /* MCS_fsize_dsk_w32 */
		DWORD t_high_word, t_low_word;
		t_low_word = ::GetFileSize(m_handle, &t_high_word);
		if (t_low_word != INVALID_FILE_SIZE || GetLastError() == NO_ERROR)
			return (int64_t)t_low_word | (int64_t)t_high_word << 32;

		return 0;
	}
    
    virtual bool TakeBuffer(void*& r_buffer, size_t& r_length)
	{
		return false;
	}

private:
	MCWinSysHandle m_handle;
	int m_putback;
	// MW-2012-09-10: [[ Bug 10230 ]] If true, it means this IO handle is a pipe.
	bool m_is_pipe;
	bool m_is_eof;
};

// MW-2005-02-22: Make this global for now so it is accesible by opensslsocket.cpp
real8 curtime;
static real8 starttime;
static DWORD startcount;

struct MCWindowsDesktop: public MCSystemInterface, public MCWindowsSystemService
{
    virtual MCServiceInterface *QueryService(MCServiceType p_type)
    {
        if (p_type == kMCServiceTypeWindowsSystem)
            return (MCWindowsSystemServiceInterface *)this;
        return nil;
    }
    
	virtual bool Initialize(void)
	{
#ifdef /* MCS_init_dsk_w32 */ LEGACY_SYSTEM
        IO_stdin = new IO_header((MCWinSysHandle)GetStdHandle(STD_INPUT_HANDLE), NULL, 0, 0);
        IO_stdin -> is_pipe = handle_is_pipe(IO_stdin -> fhandle);
        IO_stdout = new IO_header((MCWinSysHandle)GetStdHandle(STD_OUTPUT_HANDLE), NULL, 0, 0);
        IO_stdout -> is_pipe = handle_is_pipe(IO_stdout -> fhandle);
        IO_stderr = new IO_header((MCWinSysHandle)GetStdHandle(STD_ERROR_HANDLE), NULL, 0, 0);
        IO_stderr -> is_pipe = handle_is_pipe(IO_stderr -> fhandle);
        
        setlocale(LC_CTYPE, MCnullstring);
        setlocale(LC_COLLATE, MCnullstring);
        
        // MW-2004-11-28: The ctype array seems to have changed in the latest version of VC++
        ((unsigned short *)_pctype)[160] &= ~_SPACE;
        
        MCinfinity = HUGE_VAL;
        MCS_time(); // force init
        if (timeBeginPeriod(1) == TIMERR_NOERROR)
            MCS_reset_time();
        else
            MClowrestimers = True;
        MCExecPoint ep;
        ep.setstaticcstring("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ProxyEnable");
        MCS_query_registry(ep, NULL);
        if (ep.getsvalue().getlength() && ep.getsvalue().getstring()[0])
        {
            ep.setstaticcstring("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ProxyServer");
            MCS_query_registry(ep, NULL);
            if (ep.getsvalue().getlength())
                MChttpproxy = ep . getsvalue() . clone();
        }
        else
        {
            ep.setstaticcstring("HKEY_CURRENT_USER\\Software\\Netscape\\Netscape Navigator\\Proxy Information\\HTTP_Proxy");
            MCS_query_registry(ep, NULL);
            if (ep.getsvalue().getlength())
            {
                char *t_host;
                int4 t_port;
                t_host = ep.getsvalue().clone();
                ep.setstaticcstring("HKEY_CURRENT_USER\\Software\\Netscape\\Netscape Navigator\\Proxy Information\\HTTP_ProxyPort");
                MCS_query_registry(ep, NULL);
                ep.ston();
                t_port = ep.getint4();
                ep.setstringf("%s:%d", t_host, t_port);
                ep . setstrlen();
                MChttpproxy = ep . getsvalue() . clone();
                delete t_host;
            }
        }
        
        // On NT systems 'cmd.exe' is the command processor
        MCshellcmd = strclone("cmd.exe");
        
        // MW-2005-05-26: Store a global variable containing major OS version...
        OSVERSIONINFOA osv;
        memset(&osv, 0, sizeof(OSVERSIONINFOA));
        osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        GetVersionExA(&osv);
        MCmajorosversion = osv . dwMajorVersion << 8 | osv . dwMinorVersion;
        
        // MW-2012-09-19: [[ Bug ]] Adjustment to tooltip metrics for Windows.
        if (MCmajorosversion >= 0x0500)
        {
            MCttsize = 11;
            MCttfont = "Tahoma";
        }
        else if (MCmajorosversion >= 0x0600)
        {
            MCttsize = 11;
            MCttfont = "Segoe UI";
        }
        
        OleInitialize(NULL); //for drag & drop
        
        // MW-2004-11-28: Install a signal handler for FP exceptions - these should be masked
        //   so it *should* be unnecessary but Win9x plays with the FP control word.
        signal(SIGFPE, handle_fp_exception);

#endif /* MCS_init_dsk_w32 */
		IO_stdin = MCsystem -> OpenFd(STD_INPUT_HANDLE, kMCOpenFileModeRead);
		IO_stdout = MCsystem -> OpenFd(STD_OUTPUT_HANDLE, kMCOpenFileModeWrite);
		IO_stderr = MCsystem -> OpenFd(STD_ERROR_HANDLE, kMCOpenFileModeWrite);

		setlocale(LC_CTYPE, MCnullstring);
		setlocale(LC_COLLATE, MCnullstring);

#ifdef _WINDOWS_SERVER
		WORD request = MAKEWORD(1, 1);
		WSADATA t_data;
		WSAStartup(request, &t_data);

		return true;
#endif // _WINDOWS_SERVER

		// MW-2004-11-28: The ctype array seems to have changed in the latest version of VC++
		((unsigned short *)_pctype)[160] &= ~_SPACE;

		MCinfinity = HUGE_VAL;
		MCS_time(); // force init
		if (timeBeginPeriod(1) == TIMERR_NOERROR)
			MCS_reset_time();
		else
			MClowrestimers = True;

        MCExecContext ctxt(nil, nil, nil);
		MCStringRef t_key;
		t_key = MCSTR("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ProxyEnable");
        MCAutoStringRef t_type, t_error;
        MCAutoValueRef t_value;
        MCS_query_registry(t_key, &t_value, &t_type, &t_error);
		if (*t_value != nil && !MCValueIsEmpty(*t_value))
		{
            MCStringRef t_key2;
            t_key2 = MCSTR("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ProxyServer");
            MCAutoStringRef t_type2, t_error2;
            MCAutoValueRef t_value2;

			MCS_query_registry(t_key2, &t_value2, &t_type2, &t_error2);

			if (*t_value2 != nil && !MCValueIsEmpty(*t_value2))
			{
				MCAutoStringRef t_http_proxy;
				/* UNCHECKED */ ctxt . ConvertToString(*t_value2, &t_http_proxy);
				MCValueAssign(MChttpproxy, *t_http_proxy);
			}
		}
		else
		{
            MCStringRef t_key3;
            t_key3 = MCSTR("HKEY_CURRENT_USER\\Software\\Netscape\\Netscape Navigator\\Proxy Information\\HTTP_Proxy");
            MCAutoStringRef t_type3, t_error3;
            MCAutoValueRef t_value3;
            MCS_query_registry(t_key3, &t_value3, &t_type3, &t_error3);
            
            if (*t_value3 != nil && !MCValueIsEmpty(*t_value3))
			{
				MCAutoStringRef t_host;
                /* UNCHECKED */ ctxt . ConvertToString(*t_value3, &t_host);
                MCStringRef t_key4;
                t_key4 = MCSTR("HKEY_CURRENT_USER\\Software\\Netscape\\Netscape Navigator\\Proxy Information\\HTTP_ProxyPort");
                MCAutoStringRef t_type4, t_error4;
                MCAutoValueRef t_value4;
                MCS_query_registry(t_key4, &t_value4, &t_type4, &t_error4);
				MCAutoNumberRef t_port;
                /* UNCHECKED */ ctxt . ConvertToNumber(*t_value4, &t_port);
				MCAutoStringRef t_http_proxy;
                /* UNCHECKED */ MCStringFormat(&t_http_proxy, "%@:%@", *t_host, *t_port);
				MCValueAssign(MChttpproxy, *t_http_proxy);
            }
		}
        

		// On NT systems 'cmd.exe' is the command processor
		MCValueAssign(MCshellcmd, MCSTR("cmd.exe"));

		// MW-2005-05-26: Store a global variable containing major OS version...
		OSVERSIONINFOA osv;
		memset(&osv, 0, sizeof(OSVERSIONINFOA));
		osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
		GetVersionExA(&osv);
		MCmajorosversion = osv . dwMajorVersion << 8 | osv . dwMinorVersion;

		// MW-2012-09-19: [[ Bug ]] Adjustment to tooltip metrics for Windows.
		if (MCmajorosversion >= 0x0500)
		{
			MCttsize = 11;
			MCValueAssign(MCttfont, MCSTR("Tahoma"));
		}
		else if (MCmajorosversion >= 0x0600)
		{
			MCttsize = 11;
			MCValueAssign(MCttfont, MCSTR("Segoe UI"));
		}

		OleInitialize(NULL); //for drag & drop

		// MW-2004-11-28: Install a signal handler for FP exceptions - these should be masked
		//   so it *should* be unnecessary but Win9x plays with the FP control word.
		signal(SIGFPE, handle_fp_exception);

		return true;
	}

	virtual void Finalize(void)
    {
#ifdef /* MCS_shutdown_dsk_w32 */ LEGACY_SYSTEM
	OleUninitialize();
#endif /* MCS_shutdown_dsk_w32 */
        OleUninitialize();
    }
	
	virtual void Debug(MCStringRef p_string)
	{
	}

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
		return starttime;
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
        static Meta::static_ptr_t<char> buffer;
        if (buffer == NULL)
            buffer = new char[9 + 2 * I4L];
        sprintf(buffer, "NT %d.%d", (MCmajorosversion >> 8) & 0xFF, MCmajorosversion & 0xFF);
        return buffer;
#endif /* MCS_getsystemversion_dsk_w32 */
        return MCStringFormat(r_string, "NT %d.%d", (MCmajorosversion >> 8) & 0xFF, MCmajorosversion & 0xFF);
    }
    
	virtual bool GetMachine(MCStringRef& r_string)
    {
#ifdef /* MCS_getmachine_dsk_w32 */ LEGACY_SYSTEM
	return "x86";
#endif /* MCS_getmachine_dsk_w32 */
		r_string = MCValueRetain(MCNameGetString(MCN_x86));
		return true;
    }
    
	virtual MCNameRef GetProcessor(void)
    {
#ifdef /* MCS_getprocessor_dsk_w32 */ LEGACY_SYSTEM
	return "x86";
#endif /* MCS_getprocessor_dsk_w32 */
        return MCN_x86;
    }
    
	virtual bool GetAddress(MCStringRef& r_address)
    {
#ifdef /* MCS_getaddress_dsk_w32 */ LEGACY_SYSTEM
        static char *buffer;
        if (!wsainit())
            return "unknown";
        if (buffer == NULL)
            buffer = new char[MAXHOSTNAMELEN + strlen(MCcmd) + 2];
        gethostname(buffer, MAXHOSTNAMELEN);
        strcat(buffer, ":");
        strcat(buffer, MCcmd);
        return buffer;
#endif /* MCS_getaddress_dsk_w32 */
        if (!wsainit())
		{
			r_address = MCSTR("unknown");
			return true;
		}
        
        char *buffer = new char[MAXHOSTNAMELEN + 1];
        gethostname(buffer, MAXHOSTNAMELEN);
		buffer[MAXHOSTNAMELEN] = '\0';
        return MCStringFormat(r_address, "%s:%@", buffer, MCcmd);
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
        SleepEx((DWORD)(p_when * 1000.0), False);  //takes milliseconds as parameter
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
		MCAutoStringRefAsWString t_name_wstr, t_value_wstr;
		/* UNCHECKED */ t_name_wstr.Lock(p_name);
		/* UNCHECKED */ t_value_wstr.Lock(p_value);

		/* UNCHECKED */ SetEnvironmentVariableW(*t_name_wstr, *t_value_wstr);
    }
                
	virtual bool GetEnv(MCStringRef p_name, MCStringRef& r_value)
    {
#ifdef /* MCS_getenv_dsk_w32 */ LEGACY_SYSTEM
	return getenv(name);
#endif /* MCS_getenv_dsk_w32 */
	MCAutoStringRefAsWString t_name_wstr;
	/* UNCHECKED */ t_name_wstr.Lock(p_name);
	
	// Get the size of the named environment variable
	size_t t_size;
	t_size = GetEnvironmentVariableW(*t_name_wstr, NULL, 0);

	if (t_size == 0)
	{
		// Variable may just be empty
		if (GetLastError() == ERROR_ENVVAR_NOT_FOUND)
		{
			// Variable does not exist
			return false;
		}
		else
		{
			// Otherwise, the variable must have been empty
			r_value = MCValueRetain(kMCEmptyString);
			return true;
		}
	}

	MCAutoArray<unichar_t> t_string;
	if (!t_string.New(t_size))
		return false;

	DWORD t_result;
	t_result = GetEnvironmentVariableW(*t_name_wstr, t_string.Ptr(), t_size);

	if (t_result == 0 || t_result > t_size)
	{
		// Zero means the call failed.
		// It also fails if the buffer was too small (shouldn't happen)
		return false;
	}

	return MCStringCreateWithChars(t_string.Ptr(), t_result, r_value);
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
		MCAutoStringRefAsWString t_path_wstr;
		/* UNCHECKED */ t_path_wstr.Lock(p_path);
	
		if (!CreateDirectoryW(*t_path_wstr, NULL))
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
		MCAutoStringRefAsWString t_path_wstr;
		/* UNCHECKED */ t_path_wstr.Lock(p_path);

        if (!RemoveDirectoryW(*t_path_wstr))
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
		MCAutoStringRefAsWString t_path_wstr;
		/* UNCHECKED */ t_path_wstr.Lock(p_path);

		return DeleteFileW(*t_path_wstr);
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
		MCAutoStringRefAsWString t_old_wstr, t_new_wstr;
		/* UNCHECKED */ t_old_wstr.Lock(p_old_name);
		/* UNCHECKED */ t_new_wstr.Lock(p_new_name);
	
		// The Ex form is used to allow directory renaming across drives.
		// Write-through means this doesn't return until a cross-drive move is complete.
		return MoveFileExW(*t_old_wstr, *t_new_wstr, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH);
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
        IShellLinkW *ISHLNKvar1;
        err = CoCreateInstance(CLSID_ShellLink, NULL,
                               CLSCTX_INPROC_SERVER, IID_IShellLinkW,
                               (void **)&ISHLNKvar1);
        if (SUCCEEDED(err))
        {
            IPersistFile *IPFILEvar1;
			MCAutoStringRefAsWString t_target_wstr;
			/* UNCHECKED */ t_target_wstr.Lock(p_target);

            ISHLNKvar1->SetPath(*t_target_wstr);

			// Set the working directory to the leading components of the path
			uindex_t t_index;
			if (MCStringLastIndexOfChar(p_target, '\\', UINDEX_MAX, kMCStringOptionCompareExact, t_index))
			{
				MCAutoStringRef t_working_dir;
				MCAutoStringRefAsWString t_working_dir_wstr;
				/* UNCHECKED */ MCStringCopySubstring(p_target, MCRangeMake(0, t_index), &t_working_dir);
				/* UNCHECKED */ t_working_dir_wstr.Lock(*t_working_dir);

				ISHLNKvar1->SetWorkingDirectory(*t_working_dir_wstr);
			}

			// The shell link itself is an abstract object and needs to be stored to a file
			err = ISHLNKvar1->QueryInterface(IID_IPersistFile, (void **)&IPFILEvar1);
			if (SUCCEEDED(err))
			{
				// The IPersistFile interface expects a UTF-16 string
				MCAutoStringRefAsWString t_alias_wstr;
				/* UNCHECKED */ t_alias_wstr.Lock(p_alias);

				err = IPFILEvar1->Save(*t_alias_wstr, TRUE);
			}
     
            ISHLNKvar1->Release();
        }
        return SUCCEEDED(err);
    }
    
	// NOTE: 'ResolveAlias' returns a standard (not native) path.
	virtual Boolean ResolveAlias(MCStringRef p_target, MCStringRef& r_dest)
    {
#ifdef /* MCS_resolvealias_dsk_w32 */ LEGACY_SYSTEM
        char *tpath = ep.getsvalue().clone();
        char *source = MCS_resolvepath(tpath);
        delete tpath;
        char *dest = ep.getbuffer(PATH_MAX);
        HRESULT hres;
        IShellLinkA* psl;
        char szGotPath[PATH_MAX];
        WIN32_FIND_DATA wfd;
        *dest = 0;
        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                IID_IShellLinkA, (LPVOID *) &psl);
        if (SUCCEEDED(hres))
        {
            IPersistFile* ppf;
            hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
            if (SUCCEEDED(hres))
            {
                WORD wsz[PATH_MAX];
                MultiByteToWideChar(CP_ACP, 0, source, -1, (LPWSTR)wsz, PATH_MAX);
                hres = ppf->Load((LPCOLESTR)wsz, STGM_READ);
                if (SUCCEEDED(hres))
                {
                    hres = psl->Resolve(HWND_DESKTOP, SLR_ANY_MATCH|SLR_NO_UI|SLR_UPDATE);
                    if (SUCCEEDED(hres))
                    {
                        hres = psl->GetPath(szGotPath, PATH_MAX, (WIN32_FIND_DATAA *)&wfd,
                                            SLGP_SHORTPATH);
                        lstrcpyA(dest, szGotPath);
                    }
                }
                ppf->Release();
            }
            psl->Release();
        }
        delete source;
        if (SUCCEEDED(hres))
        {
            MCU_path2std(ep.getbuffer(0));
            ep.setstrlen();
        }
        else
        {
            MCresult->sets("can't get");
            MCS_seterrno(GetLastError());
            ep.clear();
        }
#endif /* MCS_resolvealias_dsk_w32 */
        MCAutoStringRef t_resolved_path;
        
        // Can't do anything if the shortcut doesn't exist
		if (!ResolvePath(p_target, &t_resolved_path))
            return false;
        
        HRESULT hres;
        IShellLinkW* psl;
        WIN32_FIND_DATAW wfd;
		MCAutoStringRef t_retrieved_path;

		// Create the shell link object
        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                IID_IShellLinkW, (void **) &psl);

        if (SUCCEEDED(hres))
        {
			// The contents of this link need to be loaded from the shortcut file           
			IPersistFile* ppf;
            hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
            if (SUCCEEDED(hres))
            {
				// The IPersistFile interface expects a UTF-16 string
				MCAutoStringRefAsWString t_alias_wstr;
				/* UNCHECKED */ t_alias_wstr.Lock(p_target);

                hres = ppf->Load(*t_alias_wstr, STGM_READ);
                if (SUCCEEDED(hres))
                {
                    hres = psl->Resolve(HWND_DESKTOP, SLR_ANY_MATCH|SLR_NO_UI|SLR_UPDATE);
                    if (SUCCEEDED(hres))
                    {
						// The returned path cannot be longer than MAX_PATH
						MCAutoArray<unichar_t> t_buffer;
						/* UNCHECKED */ t_buffer.New(MAX_PATH);

						hres = psl->GetPath(t_buffer.Ptr(), t_buffer.Size(), &wfd, SLGP_SHORTPATH);
						
						// What is the length of the path that was retrieved?
						size_t t_path_len;
						/* UNCHECKED */ StringCchLengthW(t_buffer.Ptr(), t_buffer.Size(), &t_path_len);

						/* UNCHECKED */ MCStringCreateWithChars(t_buffer.Ptr(), t_path_len, &t_retrieved_path);
                    }
                }
                ppf->Release();
            }
            psl->Release();
        }
        if (SUCCEEDED(hres))
        {
            // This function needs to return a standard (not native) path
			return MCS_pathfromnative(*t_retrieved_path, r_dest);
        }
        else
        {
            MCS_seterrno(GetLastError());
            //return MCStringCreateWithCString("can't get", r_error);
			MCresult -> sets("can't get");
			return false;
        }
    }
	
	virtual bool GetCurrentFolder(MCStringRef& r_path)
    {
#ifdef /* MCS_getcurdir_dsk_w32 */ LEGACY_SYSTEM
        char *dptr = new char[PATH_MAX + 2];
        GetCurrentDirectoryA(PATH_MAX +1, (LPSTR)dptr);
        MCU_path2std(dptr);
        return dptr;
#endif /* MCS_getcurdir_dsk_w32 */
        return MCS_getcurdir_native(r_path);
    }
    
	virtual Boolean SetCurrentFolder(MCStringRef p_path)
    {
#ifdef /* MCS_setcurdir_dsk_w32 */ LEGACY_SYSTEM
        char *newpath = MCS_resolvepath(path);
        BOOL done = SetCurrentDirectoryA((LPCSTR)newpath);
        delete newpath;
        return done;
#endif /* MCS_setcurdir_dsk_w32 */
		MCAutoStringRefAsWString t_path_wstr;
		if (!t_path_wstr.Lock(p_path))
			return false;

        if (!SetCurrentDirectoryW(*t_path_wstr))
            return False;
            
        return True;
    }
	
	// NOTE: 'GetStandardFolder' returns a standard (not native) path.
	virtual Boolean GetStandardFolder(MCNameRef p_type, MCStringRef& r_folder)
    {
#ifdef /* MCS_getspecialfolder_dsk_w32 */ LEGACY_SYSTEM
        Boolean wasfound = False;
        uint4 specialfolder = 0;
        if (ep.getsvalue() == "temporary")
        {
            if (GetTempPathA(PATH_MAX, ep.getbuffer(PATH_MAX)))
            {
                char *sptr = strrchr(ep.getbuffer(0), '\\');
                if (sptr != NULL)
                    *sptr = '\0';
                
                wasfound = True;
            }
        }
        else
            if (ep.getsvalue() == "system")
            {
                if (GetWindowsDirectoryA(ep.getbuffer(PATH_MAX), PATH_MAX))
                    wasfound = True;
            }
            else
            {
                if (ep.ton() == ES_NORMAL)
                {
                    specialfolder = ep.getuint4();
                    wasfound = True;
                }
                else
                {
                    uint1 i;
                    for (i = 0 ; i < ELEMENTS(sysfolderlist) ; i++)
                        if (ep.getsvalue() == sysfolderlist[i].token)
                        {
                            specialfolder = sysfolderlist[i].winfolder;
                            wasfound = True;
                            break;
                        }
                }
                if (wasfound)
                {
                    LPITEMIDLIST lpiil;
                    LPMALLOC lpm;
                    SHGetMalloc(&lpm);
                    wasfound = False;
                    if (SHGetSpecialFolderLocation(HWND_DESKTOP, specialfolder,
                                                   &lpiil) == 0
				        && SHGetPathFromIDListA(lpiil, ep.getbuffer(PATH_MAX)))
                        wasfound = True;
                    lpm->Free(lpiil);
                    lpm->Release();
                }
            }
        if (wasfound)
        {
            // TS-2008-06-16: [[ Bug 6403 ]] - specialFolderPath() returns 8.3 paths
            // First we need to swap to standard path seperator
            MCU_path2std(ep.getbuffer(0));
            // Next copy ep.buffer into ep.svalue
            ep.setsvalue(ep.getbuffer(0));
            
            // OK-2009-01-28: [[Bug 7452]]
            // And call the function to expand the path - if cannot convert to a longfile path,
            // we should return what we already had!
            char *t_long_path;
            uint4 t_path_length;
            MCS_getlongfilepath(ep, t_long_path, t_path_length);
            
            // MW-2010-10-22: [[ Bug 7988 ]] Make sure the result is empty regardless of outcome of prevous call.
            MCresult -> clear();
            
            char *t_old_buffer;
            t_old_buffer = ep . getbuffer(0);
            ep . clear();
            delete t_old_buffer;
            
            ep . setbuffer(t_long_path, t_path_length);
            ep.setstrlen();
        }
        else
        {
            ep.clear();
            MCresult->sets("folder not found");
        }
#endif /* MCS_getspecialfolder_dsk_w32 */
        bool t_wasfound = false;
        MCAutoNumberRef t_special_folder;
        MCAutoStringRef t_native_path;
        if (MCNameIsEqualTo(p_type, MCN_temporary))
        {
            MCAutoArray<unichar_t> t_buffer;
            uindex_t t_length;
            t_length = GetTempPathW(0, NULL);

            if (t_length != 0)
            {
                if (!t_buffer.New(t_length))
                    return false;
                t_length = GetTempPathW(t_length, t_buffer.Ptr());
                if (t_length != 0)
                {
                    if (!MCStringCreateWithChars(t_buffer.Ptr(), t_length, &t_native_path))
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
        // SN-2014-08-08: [[ Bug 13026 ]] Fix ported from 6.7
        else if (MCNameIsEqualTo(p_type, MCN_engine, kMCCompareCaseless)
                 // SN-2015-04-20: [[ Bug 14295 ]] If we are here, we are a standalone
                 // so the resources folder is the engine folder.
                 || MCNameIsEqualTo(p_type, MCN_engine, kMCCompareCaseless))
        {
            uindex_t t_last_slash;
            
            if (!MCStringLastIndexOfChar(MCcmd, '/', UINDEX_MAX, kMCStringOptionCompareExact, t_last_slash))
                t_last_slash = MCStringGetLength(MCcmd);
            
            return MCStringCopySubstring(MCcmd, MCRangeMake(0, t_last_slash), r_folder) ? True : False;
        }
        else
        {
			if (MCNumberParseUnicodeChars(MCStringGetCharPtr(MCNameGetString(p_type)), MCStringGetLength(MCNameGetString(p_type)), &t_special_folder) ||
                MCS_specialfolder_to_csidl(p_type, &t_special_folder))
            {
                t_wasfound = true;
				
				PIDLIST_ABSOLUTE ppidl;
				HRESULT hResult = SHGetFolderLocation(NULL, MCNumberFetchAsInteger(*t_special_folder), NULL, 0, &ppidl);
				if (hResult != S_OK)
					t_wasfound = false;

				// The SHGetPathFromIDList function won't return paths > MAX_PATH
				MCAutoArray<unichar_t> t_buffer;
				if (t_wasfound && !t_buffer.New(MAX_PATH))
					return false;

				if (t_wasfound && !SHGetPathFromIDListW(ppidl, t_buffer.Ptr()))
					t_wasfound = false;
				
				// Get the length of the returned path
				size_t t_pathlen;
				if (t_wasfound && StringCchLengthW(t_buffer.Ptr(), t_buffer.Size(), &t_pathlen) != S_OK)
					return false;

				// Path was successfully retrieved
				if (t_wasfound && !MCStringCreateWithChars(t_buffer.Ptr(), t_pathlen, &t_native_path))
					return false;
            }
        }
        if (t_wasfound)
        {            
            // OK-2009-01-28: [[Bug 7452]]
            // And call the function to expand the path - if cannot convert to a longfile path,
            // we should return what we already had!
			if (!LongFilePath(*t_native_path, r_folder))
				return MCStringCopy(*t_native_path, r_folder);

			return true;
        }
        else
        {
            r_folder = MCValueRetain(kMCEmptyString);
            return true;
        }
    }
	
    virtual real8 GetFreeDiskSpace()
	{
#ifdef /* MCS_getfreediskspace_dsk_w32 */ LEGACY_SYSTEM
	DWORD sc, bs, fc, tc;
	GetDiskFreeSpace(NULL, &sc, &bs, &fc, &tc);
	return ((real8)bs * (real8)sc * (real8)fc);
#endif /* MCS_getfreediskspace_dsk_w32 */
		DWORD sc, bs, fc, tc;
		GetDiskFreeSpace(NULL, &sc, &bs, &fc, &tc);
		return ((real8)bs * (real8)sc * (real8)fc);
	}


    virtual Boolean GetDevices(MCStringRef& r_devices)
	{
#ifdef /* MCS_getdevices_dsk_w32 */ LEGACY_SYSTEM
        ep.clear();
        return True;
#endif /* MCS_getdevices_dsk_w32 */
		r_devices = MCValueRetain(kMCEmptyString);
		return true;
	}


    virtual Boolean GetDrives(MCStringRef& r_drives)
	{
#ifdef /* MCS_getdrives_dsk_w32 */ LEGACY_SYSTEM
        DWORD maxsize = GetLogicalDriveStringsA(0, NULL);
        char *sptr = ep.getbuffer(maxsize);
        char *dptr = sptr;
        GetLogicalDriveStringsA(maxsize, sptr);
        while (True)
        {
            if (*sptr == '\\')
                sptr++;
            else
            {
                *dptr = *sptr++;
                if (*dptr++ == '\0')
                    if (*sptr == '\0')
                        break;
                    else
                        *(dptr - 1) = '\n';
            }
        }
        ep.setstrlen();
        return True;
#endif /* MCS_getdrives_dsk_w32 */
		MCAutoListRef t_list;
		MCAutoBlock<char_t> t_buffer;
		DWORD maxsize = GetLogicalDriveStringsA(0, NULL);
		if (!t_buffer . Allocate(maxsize) || !MCListCreateMutable('\n', &t_list))
			return false;

		char_t *sptr = *t_buffer;
		char_t *dptr = sptr;
		GetLogicalDriveStringsA(maxsize, (LPSTR)sptr);
		while (True)
		{
			if (*sptr == '\\')
				sptr++;
			else
			{
				*dptr = *sptr++;
				if (*dptr++ == '\0')
					if (*sptr == '\0')
						break;
					else
						*(dptr - 1) = '\n';
			}
		}
		return MCListAppendNativeChars(*t_list, *t_buffer, dptr - 1 - *t_buffer) &&
			MCListCopyAsString(*t_list, r_drives);
	}
    
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
        MCAutoStringRefAsWString t_path_wstr;
		if (!t_path_wstr.Lock(p_path))
			return False;
	
		DWORD t_result;
		t_result = GetFileAttributesW(*t_path_wstr);

		if (t_result == INVALID_FILE_ATTRIBUTES)
			return False;

		// If the file is a directory or read-only it isn't accessible
		if (t_result & FILE_ATTRIBUTE_DIRECTORY)
			return True;
		if (t_result & FILE_ATTRIBUTE_READONLY)
			return True;

		// File passed our checks
		return False;
    }
	
	virtual Boolean ChangePermissions(MCStringRef p_path, uint2 p_mask)
    {
#ifdef /* MCS_chmod_dsk_w32 */ LEGACY_SYSTEM
	if (_chmod(path, mask) != 0)
		return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_chmod_dsk_w32 */
		MCAutoStringRefAsWString t_path_wstr;
		if (!t_path_wstr.Lock(p_path))
			return False;

		// Get the current file attributes
		DWORD t_attrib;
		t_attrib = GetFileAttributesW(*t_path_wstr);

		if (t_attrib == INVALID_FILE_ATTRIBUTES)
			return False;

		// The POSIX security model is an extremely poor fit for the WinNT security model...
		// If any of the "write" bits are set, make the file writable. Read/execute are always allowed
		if (p_mask & 0222)
		{
			t_attrib &= ~FILE_ATTRIBUTE_READONLY;

			// If all attributes have been cleared, it should be set to "normal"
			if (t_attrib == 0)
				t_attrib = FILE_ATTRIBUTE_NORMAL;
		}
		else
		{
			// The "normal" attribute needs to be cleared if it is set
			t_attrib &= ~FILE_ATTRIBUTE_NORMAL;
			t_attrib |= FILE_ATTRIBUTE_READONLY;
		}

		// Update the attributes
		return SetFileAttributesW(*t_path_wstr, t_attrib);
    }
	
	virtual uint2 UMask(uint2 p_mask)
    {
#ifdef /* MCS_umask_dsk_w32 */ LEGACY_SYSTEM
	return _umask(mask);
#endif /* MCS_umask_dsk_w32 */
        return _umask(p_mask);
    }
    
	virtual IO_handle OpenFile(MCStringRef p_path, intenum_t p_mode, Boolean p_map)
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
		Boolean appendmode = False;
		DWORD omode = 0;		//file open mode
		DWORD createmode = OPEN_ALWAYS;
		DWORD fa = FILE_ATTRIBUTE_NORMAL; //file flags & attribute
		HANDLE t_file_handle = NULL;
		IO_handle t_handle;
		t_handle = nil;

		bool t_device = false;
		bool t_serial_device = false;
        // SN-2015-04-13: [[ Bug 14696 ]] Close the file handler in case our
        //  MCStdioFileHandler could not be created.
        bool t_close_file_handler = false;

		// SN-2015-02-26: [[ Bug 14612 ]] Also process the device path
		//  translation when using <open file>
		MCAutoStringRef t_devicepath;
		if (!get_device_path(p_path, &t_devicepath))
			return NULL;

		// Is this a device path?
		if (MCStringBeginsWithCString(*t_devicepath, (const char_t*)"\\\\.\\", kMCStringOptionCompareExact))
		{
			t_device = true;

			// Is this a path to a serial port?
			if (MCStringBeginsWithCString(*t_devicepath, (const char_t*)"\\\\.\\COM", kMCStringOptionCompareCaseless))
				t_serial_device = true;
        }

		if (p_mode == kMCOpenFileModeRead)
		{
			omode = GENERIC_READ;
			createmode = OPEN_EXISTING;
		}
        if (p_mode== kMCOpenFileModeWrite || p_mode == kMCOpenFileModeCreate)
		{
			omode = GENERIC_WRITE;
			createmode = CREATE_ALWAYS;
		}

		// SN-2014-05-02 [[ Bug 12061 ]] Can't test app on Android
		// Issue when reading the deployed file when deploying to Linux/Android
		if (p_mode == kMCOpenFileModeCreate)
			omode |= GENERIC_READ;
		if (p_mode == kMCOpenFileModeUpdate)
			omode = GENERIC_WRITE | GENERIC_READ;
		if (p_mode == kMCOpenFileModeAppend)
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

		MCAutoStringRefAsWString t_path_wstr;
		/* UNCHECKED */ t_path_wstr.Lock(*t_devicepath);

		t_file_handle = CreateFileW(*t_path_wstr, omode, sharemode, NULL,
							 createmode, fa, NULL);
		
		if (t_file_handle == INVALID_HANDLE_VALUE)
		{
			return NULL;
		}

		if (t_serial_device)
		{
			MCAutoStringRefAsWString t_serial_wstr;
			/* UNCHECKED */ t_serial_wstr.Lock(MCserialcontrolsettings);
			
			DCB dcb;
			dcb . DCBlength = sizeof(DCB);
			if (!GetCommState(t_file_handle, &dcb) || !BuildCommDCBW(*t_serial_wstr, &dcb)
					|| !SetCommState(t_file_handle, &dcb))
			{
				MCS_seterrno(GetLastError());
				CloseHandle(t_file_handle);
				MCresult->sets("SetCommState error");
				return NULL;
			}
			COMMTIMEOUTS timeout;         //set timeout to prevent blocking
			memset(&timeout, 0, sizeof(COMMTIMEOUTS));
			timeout.ReadIntervalTimeout = MAXDWORD;
			timeout.WriteTotalTimeoutConstant = 2000;
			if (!SetCommTimeouts(t_file_handle, (LPCOMMTIMEOUTS)&timeout))
			{
				MCS_seterrno(GetLastError());
				CloseHandle(t_file_handle);
				MCresult->sets("SetCommTimeouts error");
				return NULL;
			}
			p_map = False;
		}

		if (p_map && (omode == GENERIC_READ)) //if memory map file
		{
			MCWinSysHandle t_file_mapped_handle = 
				(MCWinSysHandle)CreateFileMappingA(t_file_handle, NULL, PAGE_READONLY,
														0, 0, NULL);

			if (t_file_mapped_handle != NULL)
			{
				uint32_t t_len = GetFileSize(t_file_handle, NULL);
				char *t_buffer = (char*)MapViewOfFile(t_file_mapped_handle,
													  FILE_MAP_READ, 0, 0, 0);
				
				// SN-2015-03-02: [[ Bug 14696 ]] If the file is too large,
				//  the file mapping won't work, and a normal file handler
				//  should be used.
				if (t_buffer == NULL)
				{
					CloseHandle(t_file_mapped_handle);
                    t_handle = new MCBufferedFileHandle(new MCStdioFileHandle((MCWinSysHandle)t_file_handle));
                    t_close_file_handler = t_handle == NULL;
				}
				else
				{
					t_handle = new MCMemoryMappedFileHandle(t_file_mapped_handle, t_buffer, t_len);
                    // SN-2015-04-13: [[ Bug 14696 ]] We don't want to leave a
                    //  file handler open in case the memory mapped file could
                    //  not be allocated. We always close the normal file handle
                    if (t_handle == NULL)
                        CloseHandle(t_file_mapped_handle);
                    t_close_file_handler = true;
				}
			}
			// SN-2014-11-27: [[ Bug 14110 ]] A StdioFileHandle should be created if the file mapping failed
			// (for empty files for instance).
			else
            {
                t_handle = new MCBufferefFileHandle(new MCStdioFileHandle((MCWinSysHandle)t_file_handle));
                t_close_file_handler = t_handle == NULL;
            }
		}
		else
        {
            t_handle = new MCBufferFileHandle(new MCStdioFileHandle((MCWinSysHandle)t_file_handle));
            t_close_file_handler = t_handle == NULL;
        }

        // SN-2015-04-13: [[ Bug 14696 ]] We close the Windows file handle in
        //  case we did not successfully create an MCStdioFileHandle.
        if (t_close_file_handler)
            CloseHandle(t_file_handle);
        
		return t_handle;
    }
    
	virtual IO_handle OpenFd(uint32_t p_fd, intenum_t p_mode)
	{
		IO_handle t_stdio_handle;
		HANDLE t_handle;

		// No need to open p_fd, as on Windows only STD_INPUT_HANDLE, STD_OUTPUT_HANDLE 
		// and STD_ERROR_HANDLE can be processed by GetStdHandle
		t_handle = GetStdHandle(p_fd);  

		if (t_handle == INVALID_HANDLE_VALUE)
			return nil;

		// Since we can only have an STD fd, we know we have a pipe.
		t_stdio_handle = new MCBufferedFileHandle(new MCStdioFileHandle((MCWinSysHandle)t_handle, true));

		return t_stdio_handle;
	}

	virtual IO_handle OpenDevice(MCStringRef p_path, intenum_t p_mode)
	{
		// For Windows, the path is used to determine whether a file or a device is being opened
		// SN-2015-02-16: [[ Bug 14612 ]] <open file "COM:"> should do the same as
		//  <open device "COM:">, so no difference in the path
		//  translation must exist between MCWindowsDesktop::OpenDevice
		//  and MCWindowsDesktop::OpenFile
		return OpenFile(p_path, p_mode, True);
	}
	
	// NOTE: 'GetTemporaryFileName' returns a non-native path.
	virtual bool GetTemporaryFileName(MCStringRef& r_tmp_name)
    {
#ifdef /* MCS_tmpnam_dsk_w32 */ LEGACY_SYSTEM
        MCExecPoint ep(NULL, NULL, NULL);
        
        // MW-2008-06-19: Make sure fname is stored in a static to keep the (rather
        //   unplesant) current semantics of the call.
        static char *fname;
        if (fname != NULL)
            delete fname;
        
        // TS-2008-06-18: [[ Bug 6403 ]] - specialFolderPath() returns 8.3 paths
        fname = _tempnam("\\tmp", "tmp");
        
        char *t_ptr = (char*)strrchr(fname, '\\');
        if (t_ptr != NULL)
            *t_ptr = 0 ;
        
        MCU_path2std(fname);
        ep.setsvalue(fname);
        MCS_longfilepath(ep);
        
        if (t_ptr != NULL)
            ep.appendstringf("/%s", ++t_ptr);
        
        // MW-2008-06-19: Make sure we delete this version of fname, since we don't
        //   need it anymore.
        delete fname;
        
        // MW-2008-06-19: Use ep . getsvalue() . clone() to make sure we get a copy
        //   of the ExecPoint's string as a NUL-terminated (C-string) string.
        fname = ep . getsvalue() . clone();
        
        return fname;
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
            !MCS_pathfromnative(*t_path, &t_stdpath) ||
            !LongFilePath(*t_stdpath, &t_long_path))
		{
			r_tmp_name = MCValueRetain(kMCEmptyString);
            return true;
		}
        
        if (t_ptr == nil)
		{
			r_tmp_name = MCValueRetain(*t_long_path);
			return true;
		}
        
        MCAutoStringRef t_tmp_name;
        if (!MCStringMutableCopy(*t_long_path, &t_tmp_name) ||
			!MCStringAppendFormat(*t_tmp_name, "/%s", t_ptr + 1))
			return false;
	
		MCStringCopy(*t_tmp_name, r_tmp_name);
		return true;
    }
	
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
		MCAutoStringRefAsWString t_path_wstr;
		if (!t_path_wstr.Lock(p_path))
			return NULL;
	
		// MW-2011-02-28: [[ Bug 9410 ]] Use the Ex form of LoadLibrary and ask it to try
        //   to resolve dependent DLLs from the folder containing the DLL first.
        HMODULE t_handle;
        t_handle = LoadLibraryExW(*t_path_wstr, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        
        return (MCSysModuleHandle)t_handle;
    }
    
	virtual MCSysModuleHandle ResolveModuleSymbol(MCSysModuleHandle p_module, MCStringRef p_symbol)
    {
#ifdef /* MCS_resolvemodulesymbol_dsk_w32 */ LEGACY_SYSTEM
	return GetProcAddress((HMODULE)p_module, p_symbol);
#endif /* MCS_resolvemodulesymbol_dsk_w32 */
        // NOTE: symbol addresses are never Unicode and only an ANSI call exists
		return (MCSysModuleHandle)GetProcAddress((HMODULE)p_module, MCStringGetCString(p_symbol));
    }
	virtual void UnloadModule(MCSysModuleHandle p_module)
    {
#ifdef /* MCS_unloadmodule_dsk_w32 */ LEGACY_SYSTEM
	FreeLibrary((HMODULE)p_module);
#endif /* MCS_unloadmodule_dsk_w32 */
        FreeLibrary((HMODULE)p_module);
    }
	
	// Utility function: converts FILETIME to a Unix time
	static uint32_t FiletimeToUnix(const FILETIME& ft)
	{
		// Assemble the FILETIME into a 64-bit integer
		uint64_t u64;
		u64 = (uint64_t(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;

		// Number of 100ns intervals between 1601-01-01 and 1970-01-01
		// (according to Microsoft; they don't say which calendar...)
		u64 -= 116444736000000000ULL;

		// Convert from 100ns intervals into seconds
		u64 /= 10000000;

		// Like all Unix times, this will overflow in 2038
		return uint32_t(u64);
	}

	virtual bool ListFolderEntries(MCSystemListFolderEntriesCallback p_callback, void *x_context)
    {
#ifdef /* MCS_getentries_dsk_w32 */ LEGACY_SYSTEM
        p_context . clear();
        
        WIN32_FIND_DATAA data;
        HANDLE ffh;            //find file handle
        uint4 t_entry_count;
        t_entry_count = 0;
        Boolean ok = False;
        char *tpath = MCS_getcurdir();
        MCU_path2native(tpath);
        char *spath = new char [strlen(tpath) + 5];//path to be searched
        strcpy(spath, tpath);
        if (tpath[strlen(tpath) - 1] != '\\')
            strcat(spath, "\\");
        strcat(spath, "*.*");
        delete tpath;
        /*
         * Now open the directory for reading and iterate over the contents.
         */
        ffh = FindFirstFileA(spath, &data);
        if (ffh == INVALID_HANDLE_VALUE)
        {
            delete spath;
            return;
        }
        MCExecPoint ep(NULL, NULL, NULL);
        do
        {
            if (strequal(data.cFileName, "."))
                continue;
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !p_files
		        || !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && p_files)
            {
                char tbuf[PATH_MAX * 3 + U4L * 4 + 22];
                if (p_detailed)
                {
                    ep.copysvalue(data.cFileName, strlen(data.cFileName));
                    MCU_urlencode(ep);
                    struct _stati64 buf;
                    _stati64(data.cFileName, &buf);
                    // MW-2007-02-26: [[ Bug 4474 ]] - Fix issue with detailed files not working on windows due to time field lengths
                    // MW-2007-12-10: [[ Bug 606 ]] - Make unsupported fields appear as empty
                    sprintf(tbuf, "%*.*s,%I64d,,%ld,%ld,%ld,,,,%03o,",
                            (int)ep.getsvalue().getlength(), (int)ep.getsvalue().getlength(),
                            ep.getsvalue().getstring(), buf.st_size, (long)buf.st_ctime,
                            (long)buf.st_mtime, (long)buf.st_atime, buf.st_mode & 0777);
                }
                
                if (p_detailed)
                    p_context . concatcstring(tbuf, EC_RETURN, t_entry_count == 0);
                else
                    p_context . concatcstring(data.cFileName, EC_RETURN, t_entry_count == 0);
                
                t_entry_count += 1;
            }
        }
        while (FindNextFileA(ffh, &data));
        FindClose(ffh);
        delete spath;
#endif /* MCS_getentries_dsk_w32 */        
        WIN32_FIND_DATAW data;
        HANDLE ffh;            //find file handle

        MCAutoStringRef t_curdir_native;
        MCAutoStringRef t_search_path;
        
		// The search is done in the current directory
		MCS_getcurdir_native(&t_curdir_native);

		// Search strings need to have a wild-card added
		if (MCStringGetCharAtIndex(*t_curdir_native, MCStringGetLength(*t_curdir_native) - 1) == '\\')
		{
			/* UNCHECKED */ MCStringFormat(&t_search_path, "%@*", *t_curdir_native);
		}
		else
		{
			/* UNCHECKED */ MCStringFormat(&t_search_path, "%@\\*", *t_curdir_native);
		}

		// Iterate through the contents of the directory
		MCAutoStringRefAsWString t_search_wstr;
		if (!t_search_wstr.Lock(*t_search_path))
			return false;
		ffh = FindFirstFileW(*t_search_wstr, &data);

		do
		{
			// Don't list the current directory
			if (lstrcmpiW(data.cFileName, L".") == 0)
				continue;

			// Retrieve as many of the file attributes as Windows supports
			// The MCSystemFolderEntry::name field is a C string :-(
			// It's actually an array of wchar, so it allows unicode characters...
			MCSystemFolderEntry t_entry;

			MCStringRef t_unicode_name;

            if (data.cFileName && MCStringCreateWithChars(data.cFileName, wcslen(data.cFileName), t_unicode_name))
				t_entry.name = t_unicode_name;		// MCStringRef is now in use to handle the unicode chars
			else
                t_entry.name = kMCEmptyString;

			t_entry.data_size = (uint64_t(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
			t_entry.resource_size = 0;			// Doesn't exist
			t_entry.creation_time = FiletimeToUnix(data.ftCreationTime);
			t_entry.modification_time = FiletimeToUnix(data.ftLastWriteTime);
			t_entry.access_time = FiletimeToUnix(data.ftLastAccessTime);
			t_entry.backup_time = 0;			// Doesn't exist
			t_entry.user_id = 0;				// Exists but is incompatible
			t_entry.group_id = 0;				// Exists but is incompatible
			t_entry.permissions = 0555;			// All files support read/write for all
			t_entry.file_creator = 0;			// Doesn't exist
			t_entry.file_type = 0;				// Doesn't exist
			t_entry.is_folder = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

			// Fix-up the permissions if write access is permitted
			if (!(data.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
				t_entry.permissions |= 0222;

			// Hand the structure to the callback
			p_callback(x_context, &t_entry);

			MCValueRelease(t_unicode_name);
		}
		while (FindNextFileW(ffh, &data));
		FindClose(ffh);

		return true;
    }
    
    // ST-2014-12-18: [[ Bug 14259 ]] Returns the executable from the system tools, not from argv[0]
	virtual bool GetExecutablePath(MCStringRef& r_path)
	{
		WCHAR* wcFileNameBuf = new WCHAR[MAX_PATH+1];
		DWORD dwFileNameLen = GetModuleFileNameW(NULL, wcFileNameBuf, MAX_PATH+1);
		
		MCAutoStringRef t_path;
		MCStringCreateWithWStringAndRelease((unichar_t*)wcFileNameBuf, &t_path);
		return PathFromNative(*t_path, r_path); 
	}

	virtual bool PathToNative(MCStringRef p_path, MCStringRef& r_native)
	{
#ifdef /* MCU_path2native */ LEGACY_SYSTEM
	if (dptr == NULL || !*dptr)
		return;
#if defined _WIN32
	do
	{
		if (*dptr == '/')
			*dptr = '\\';
		else if (*dptr == '\\')
			*dptr = '/';
	}
	while (*++dptr);
#endif
#endif /* MCU_path2native */
		if (MCStringIsEmpty(p_path))
		{
			r_native = MCValueRetain(kMCEmptyString);
			return true;
		}

		// The / and \ characters in the path need to be swapped
		MCAutoArray<unichar_t> t_swapped;
		t_swapped.New(MCStringGetLength(p_path));

		for (uindex_t i = 0; i < MCStringGetLength(p_path); i++)
		{
			unichar_t t_char;
			t_char = MCStringGetCharAtIndex(p_path, i);
			if (t_char == '/')
				t_swapped[i] = '\\';
			else if (t_char == '\\')
				t_swapped[i] = '/';
			else
				t_swapped[i] = t_char;
		}

		// Add the NT-style path prefix, if required
		MCAutoStringRef t_swapped_str;
		/* UNCHECKED */ MCStringCreateWithChars(t_swapped.Ptr(), t_swapped.Size(), &t_swapped_str);
		legacy_path_to_nt_path(*t_swapped_str, r_native);
		return true;
	}

	virtual bool PathFromNative(MCStringRef p_native, MCStringRef& r_livecode_path)
	{
#ifdef /* MCU_path2std */ LEGACY_SYSTEM
        if (dptr == NULL || !*dptr)
            return;
        do
        {
#ifdef _MACOSX
            if (*dptr == '/')
                *dptr = ':';
            else
                if (*dptr == ':')
#else
                    if (*dptr == '/')
                        *dptr = '\\';
                    else
                        if (*dptr == '\\')
#endif
                            
                            *dptr = '/';
        }
        while (*++dptr);
#endif /* MCU_path2std */
		if (MCStringIsEmpty(p_native))
		{
			r_livecode_path = MCValueRetain(kMCEmptyString);
			return true;
		}

		// Remove any NT-style path prefix
		MCAutoStringRef t_legacy_path;
		nt_path_to_legacy_path(p_native, &t_legacy_path);

		// The / and \ characters in the path need to be swapped
		MCAutoArray<unichar_t> t_swapped;
		t_swapped.New(MCStringGetLength(*t_legacy_path));

		for (uindex_t i = 0; i < MCStringGetLength(*t_legacy_path); i++)
		{
			unichar_t t_char;
			t_char = MCStringGetCharAtIndex(*t_legacy_path, i);
			if (t_char == '/')
				t_swapped[i] = '\\';
			else if (t_char == '\\')
				t_swapped[i] = '/';
			else
				t_swapped[i] = t_char;
		}

		return MCStringCreateWithChars(t_swapped.Ptr(), t_swapped.Size(), r_livecode_path);
	}

	virtual bool ResolvePath(MCStringRef p_path, MCStringRef& r_resolved_path)
	{
#ifdef /* MCS_resolvepath_dsk_w32 */ LEGACY_SYSTEM
        if (path == NULL)
        {
            char *tpath = MCS_getcurdir();
            MCU_path2native(tpath);
            return tpath;
        }
        char *cstr = strclone(path);
        MCU_path2native(cstr);
        return cstr;
#endif /* MCS_resolvepath_dsk_w32 */
		// Parameters for MCSystemInterface functions are always native paths
		return ResolveNativePath(p_path, r_resolved_path);
	}
    
	virtual bool ResolveNativePath(MCStringRef p_path, MCStringRef& r_resolved_path)
	{
		if (MCStringGetLength(p_path) == 0)
			return MCS_getcurdir_native(r_resolved_path);

		MCAutoStringRef t_canonised_path;
		bool t_success;
		t_success = true;
		
		// Taken from LiveCode 6.7's w32spec.cpp MCS_get_canonical_path
		// The following rules are used to process paths on Windows:
		// - an absolute UNIX path is mapped to an absolute windows path using the drive of the CWD:
		// /foo/bar -> CWD-DRIVE:/foo/bar
		// - an absolute windows path is left as is:
		// //foo/bar -> //foo/bar
		// C:/foo/bar -> C:/foo/bar
		// - a relative path is prefixed by the CWD:
		// foo/bar -> CWD/foo/bar
		// Note: / and \ are treated the same, but not changed. 
		// Note: When adding a path separator \ is used in LiveCode 7.0
		// since we are suppose to have a native path as input for MCSystem functions

		// We store the first chars in this static to make the if
		// statements more readable
		char_t t_first_chars[2];
        t_first_chars[0] = MCStringGetNativeCharAtIndex(p_path, 0);
		t_first_chars[1] = MCStringGetNativeCharAtIndex(p_path, 1);

		if ((t_first_chars[0] == '/' && t_first_chars[1] != '/')
				|| (t_first_chars[0] == '\\' && t_first_chars[1] != '\\'))
		{
			// path in root of current drive
			MCAutoStringRef t_curdir;
			if (t_success)
				t_success = MCS_getcurdir_native(&t_curdir);
		
			if (t_success)
			t_success = MCStringFormat(&t_canonised_path, 
							"%c:%@", 
							MCStringGetNativeCharAtIndex(*t_curdir, 0),
							p_path);
		}
		else if ((is_legal_drive(t_first_chars[0]) && t_first_chars[1] == ':')
				|| (t_first_chars[0] == '/' && t_first_chars[1] == '/')
				|| (t_first_chars[0] == '\\' && t_first_chars[1] == '\\'))
		{
			// absolute path
			t_canonised_path = p_path;
		}
		else
		{
			// relative to current folder
			MCAutoStringRef t_curdir;
			t_success = MCS_getcurdir_native(&t_curdir);

			if (t_success)
				t_success = MCStringFormat(&t_canonised_path,
						   "%@\\%@",
						   *t_curdir,
						   p_path);
		}

		if (t_success)
			r_resolved_path = MCValueRetain(*t_canonised_path);

		return t_success;
	}
	
	virtual bool LongFilePath(MCStringRef p_path, MCStringRef& r_long_path)
    {
#ifdef /* MCS_longfilepath_dsk_w32 */ LEGACY_SYSTEM
        char *tpath = ep.getsvalue().clone();
        char *shortpath = MCS_resolvepath(tpath);
        delete tpath;
        char *longpath = ep.getbuffer(PATH_MAX);
        char *p, *pStart;
        char buff[PATH_MAX];
        WIN32_FIND_DATAA wfd;
        HANDLE handle;
        int i;
        
        // Keep strings null-terminated
        *buff = '\0';
        *longpath = '\0';
        //
        p = shortpath;
        while (p != NULL)
        {
            // Find next
            p = strchr(pStart = p, PATH_DELIMITER);
            // See if a token was found
            if (p != pStart)
            {
                i = strlen(buff);
                // Append token to temp buffer
                if (p == NULL)
                {
                    strcpy(buff + i, pStart);
                }
                else
                {
                    *p = '\0';
                    strcpy(buff + i, pStart);
                    *p = PATH_DELIMITER;
                }
                // Copy token unmodified if drive specifier
                if (strchr(buff + i, ':') != NULL)
                    strcat(longpath, buff + i);
                else
                {
                    // Convert token to long name
                    handle = FindFirstFileA(buff, &wfd);
                    if (handle == INVALID_HANDLE_VALUE)
                    {
                        MCresult->sets("can't get");
                        MCS_seterrno(GetLastError());
                        ep.clear();
                        delete shortpath;
                        return;
                    }
                    strcat(longpath, wfd.cFileName);
                    FindClose(handle);
                }
            }
            // Copy terminator
            if (p != NULL)
            {
                buff[i = strlen(buff)] = *p;
                buff[i + 1] = '\0';
                longpath[i = strlen(longpath)] = *p;
                longpath[i + 1] = '\0';
            }
            // Bump pointer
            if (p)
                p++;
        }
        MCU_path2std(ep.getbuffer(0));
        ep.setstrlen();
        delete shortpath;
#endif /* MCS_longfilepath_dsk_w32 */
		// The path given to longfilepath can't be already resolved - as it remains the same
		// for UNIX-based OS, resolving it's not done in the MCS_* function like usual
		MCAutoStringRef t_resolved_path;
		if (!ResolveNativePath(p_path, &t_resolved_path))
			return false;

		MCAutoStringRefAsWString t_short_wstr;
		if (!t_short_wstr.Lock(*t_resolved_path))
			return false;

		// Retrieve the length of the long file name
		DWORD t_length;
		t_length = GetLongPathNameW(*t_short_wstr, NULL, 0);

		if (t_length == 0)
		{
			MCS_seterrno(GetLastError());
			return false;
		}

		MCAutoArray<unichar_t> t_buffer;
		if (!t_buffer.New(t_length))
			return false;

		DWORD t_result;
		t_result = GetLongPathNameW(*t_short_wstr, t_buffer.Ptr(), t_buffer.Size());

		// Check that the path was successfully copied
		if (t_result == 0 || t_result >= t_length)
		{
			MCS_seterrno(GetLastError());
			return false;
		}

		// The new way the longpath is built leaves the terminating backslash
		char t_bdag = (char)t_buffer[t_result - 1];
		if (t_buffer[t_result - 1] == '\\')
			--t_result;

		return MCStringCreateWithChars(t_buffer.Ptr(), t_result, r_long_path);
    }
    
	virtual bool ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path)
    {
#ifdef /* MCS_shortfilepath_dsk_w32 */ LEGACY_SYSTEM
        char *tpath = ep.getsvalue().clone();
        char *newpath = MCS_resolvepath(tpath);
        delete tpath;
        if (!GetShortPathNameA(newpath, ep.getbuffer(PATH_MAX), PATH_MAX))
        {
            MCresult->sets("can't get");
            MCS_seterrno(GetLastError());
            ep.clear();
        }
        else
        {
            MCU_path2std(ep.getbuffer(0));
            ep.setstrlen();
        }
        delete newpath;
#endif /* MCS_shortfilepath_dsk_w32 */
		// The path given to shortfilepath can't be already resolved - as it remains the same
		// for UNIX-based OS, resolving it's not done in the MCS_* function like usual
		MCAutoStringRef t_resolved_path;
		if (!ResolveNativePath(p_path, &t_resolved_path))
			return false;

		MCAutoStringRefAsWString t_long_wstr;
		if (!t_long_wstr.Lock(*t_resolved_path))
			return false;

		// How long is the short path?
		DWORD t_length;
		t_length = GetShortPathNameW(*t_long_wstr, NULL, 0);

		if (t_length == 0)
		{
			MCS_seterrno(GetLastError());
			return false;
		}

		MCAutoArray<unichar_t> t_buffer;
		if (!t_buffer.New(t_length))
			return false;

		DWORD t_result;
		t_result = GetShortPathNameW(*t_long_wstr, t_buffer.Ptr(), t_buffer.Size());

		if (t_result == 0 || t_result >= t_length)
		{
			MCS_seterrno(GetLastError());
			return false;
		}

		return MCStringCreateWithChars(t_buffer.Ptr(), t_result, r_short_path);
    }
    
	virtual bool Shell(MCStringRef p_command, MCDataRef& r_data, int& r_retcode)
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
        MCprocesses[index].name = strclone("shell");
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
            MCresult->store(ep2, False);
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
        STARTUPINFOW siStartInfo;
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
        
		MCStringRef t_cmd;
        // SN-2014-06-16 [[ Bug 12648 ]] Shell command does not accept spaces despite being quoted (Windows)
        // Fix for 7
		/* UNCHECKED */ MCStringFormat(t_cmd, "%@ /C \"%@\"", MCshellcmd, p_command);
		MCAutoStringRefAsWString t_wcmd;
		t_wcmd . Lock(t_cmd);
        MCU_realloc((char **)&MCprocesses, MCnprocesses,
                    MCnprocesses + 1, sizeof(Streamnode));
        uint4 index = MCnprocesses;
        MCprocesses[index].name = (MCNameRef)MCValueRetain(MCM_shell);
        MCprocesses[index].mode = OM_NEITHER;
        MCprocesses[index].ohandle = new MCMemoryFileHandle;
		MCprocesses[index].ihandle = new MCStdioFileHandle((MCWinSysHandle)hChildStdoutRd, true);
        if (created)
        {
            HANDLE phandle = GetCurrentProcess();
            DuplicateHandle(phandle, hChildStdoutWr, phandle, &hChildStderrWr,
                            0, TRUE, DUPLICATE_SAME_ACCESS);
            siStartInfo.hStdError = hChildStderrWr;
            DWORD threadID = 0;
            if (CreateProcessW(NULL, *t_wcmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE,
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
            MCeerror->add(EE_SHELL_BADCOMMAND, 0, 0, t_cmd);
			MCValueRelease(t_cmd);
            return false;
        }
        
        s_finished_reading = false;
        
        do
        {
            if (MCscreen->wait(READ_INTERVAL, False, False))
            {
                MCeerror->add(EE_SHELL_ABORT, 0, 0, t_cmd);
                if (MCprocesses[index].pid != 0)
                {
                    TerminateProcess(MCprocesses[index].phandle, 0);
                    MCprocesses[index].pid = 0;
                    TerminateThread(MCprocesses[index].thandle, 0);
                    CloseHandle(piProcInfo.hProcess);
                    CloseHandle(piProcInfo.hThread);
                }
                MCS_close(MCprocesses[index].ihandle);
                MCprocesses[index].ihandle = nil;
                IO_cleanprocesses();                
				MCValueRelease(t_cmd);
                return false;
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

        r_retcode = MCprocesses[index].retcode;

		void *t_buffer;
		uint32_t t_buf_size;
		bool t_success;

		t_success = MCS_closetakingbuffer_uint32(MCprocesses[index].ohandle, t_buffer, t_buf_size) == IO_NORMAL;
        MCprocesses[index].ohandle = nil;
        
        IO_cleanprocesses();
        MCValueRelease(t_cmd);

		if (t_success)
			t_success = MCDataCreateWithBytes((byte_t*) t_buffer, t_buf_size, r_data);

		delete[] t_buffer;
		return t_success;
	}
    
	virtual uint32_t TextConvert(const void *p_string, uint32_t p_string_length, void *r_buffer, uint32_t p_buffer_length, uint32_t p_from_charset, uint32_t p_to_charset)
	{
#ifdef /* MCS_multibytetounicode_dsk_w32 */ LEGACY_SYSTEM
	char szLocaleData[6];
	uint2 codepage = 0;
	GetLocaleInfoA(MAKELCID(MCS_charsettolangid(charset), SORT_DEFAULT) ,
	               LOCALE_IDEFAULTANSICODEPAGE, szLocaleData, 6);
	codepage = (uint2)strtoul(szLocaleData, NULL, 10);
	uint4 dsize = MultiByteToWideChar( codepage, 0, s, len, (LPWSTR)d,
	                                   destbufferlength >> 1);
	destlen = dsize << 1;
#endif /* MCS_multibytetounicode_dsk_w32 */
#ifdef /* MCS_unicodetomultibyte_dsk_w32 */ LEGACY_SYSTEM
	char szLocaleData[6];
	uint2 codepage = 0;
	GetLocaleInfoA(MAKELCID(MCS_charsettolangid(charset), SORT_DEFAULT)
	               , LOCALE_IDEFAULTANSICODEPAGE, szLocaleData, 6);
	codepage = (uint2)strtoul(szLocaleData, NULL, 10);
	uint4 dsize = WideCharToMultiByte( codepage, 0, (LPCWSTR)s, len >> 1,
	                                   d, destbufferlength, NULL, NULL);
	destlen = dsize;
#endif /* MCS_unicodetomultibyte_dsk_w32 */
		char szLocaleData[6];
		uint2 codepage = 0;
		GetLocaleInfoA(MAKELCID(MCS_charsettolangid(p_to_charset), SORT_DEFAULT)
					   , LOCALE_IDEFAULTANSICODEPAGE, szLocaleData, 6);
		codepage = (uint2)strtoul(szLocaleData, NULL, 10);
		uint4 dsize;
		const char *t_string_ptr = (char*)p_string;
		const char *t_buffer_ptr = (char*)r_buffer;
		if (p_from_charset == LCH_UNICODE)
			dsize = WideCharToMultiByte( codepage, 0, (LPCWSTR)t_string_ptr, p_string_length >> 1,
										   (LPSTR)t_buffer_ptr, p_buffer_length, NULL, NULL);
		else
		{
			dsize = MultiByteToWideChar( codepage, 0, (LPCSTR)t_string_ptr, p_string_length, (LPWSTR)t_buffer_ptr,
										   p_buffer_length >> 1);
			// SN-2014-12-15: [[ Bug 14203 ]] The required size must be adapted as it was beforehand
			dsize <<= 1;
		}

		return dsize;
	}

	virtual bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4& p_output_length, uint4& r_used)
	{
#ifdef /* MCSTextConvertToUnicode_dsk_w32 */ LEGACY_SYSTEM
	if (p_input_length == 0)
	{
		r_used = 0;
		return true;
	}

	UINT t_codepage;
	if (p_input_encoding >= kMCTextEncodingWindowsNative)
		t_codepage = p_input_encoding - kMCTextEncodingWindowsNative;
	else if (p_input_encoding >= kMCTextEncodingMacNative)
		t_codepage = 10000 + p_input_encoding - kMCTextEncodingMacNative;
	else
	{
		r_used = 0;
		return true;
	}

	// MW-2009-08-27: It is possible for t_codepage == 65001 which means UTF-8. In this case we can't
	//   use the precomposed flag...

	int t_required_size;
	t_required_size = MultiByteToWideChar(t_codepage, t_codepage == 65001 ? 0 : MB_PRECOMPOSED, (LPCSTR)p_input, p_input_length, NULL, 0);
	if (t_required_size > (int)p_output_length / 2)
	{
		r_used = t_required_size * 2;
		return false;
	}

	int t_used;
	t_used = MultiByteToWideChar(t_codepage, t_codepage == 65001 ? 0 : MB_PRECOMPOSED, (LPCSTR)p_input, p_input_length, (LPWSTR)p_output, p_output_length);
	r_used = t_used * 2;

	return true; 
#endif /* MCSTextConvertToUnicode_dsk_w32 */
		if (p_input_length == 0)
		{
			r_used = 0;
			return true;
		}

		UINT t_codepage;
		if (p_input_encoding >= kMCTextEncodingWindowsNative)
			t_codepage = p_input_encoding - kMCTextEncodingWindowsNative;
		else if (p_input_encoding >= kMCTextEncodingMacNative)
			t_codepage = 10000 + p_input_encoding - kMCTextEncodingMacNative;
		else
		{
			r_used = 0;
			return true;
		}

		// MW-2009-08-27: It is possible for t_codepage == 65001 which means UTF-8. In this case we can't
		//   use the precomposed flag...

		int t_required_size;
		t_required_size = MultiByteToWideChar(t_codepage, t_codepage == 65001 ? 0 : MB_PRECOMPOSED, (LPCSTR)p_input, p_input_length, NULL, 0);
		if (t_required_size > (int)p_output_length / 2)
		{
			r_used = t_required_size * 2;
			return false;
		}

		int t_used;
		t_used = MultiByteToWideChar(t_codepage, t_codepage == 65001 ? 0 : MB_PRECOMPOSED, (LPCSTR)p_input, p_input_length, (LPWSTR)p_output, p_output_length);
		r_used = t_used * 2;

		return true; 
	}
    
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
					return;

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
                if (MCprocesses[i].ihandle == NULL || !PeekNamedPipe(MCprocesses[i].ihandle->GetFilePointer(), NULL, 0, NULL, (DWORD *)&t_available, NULL))
                    t_available = 0;
                if (t_available != 0)
                    return;
                
				/* TODO: set end of file...
                // MW-2010-10-25: [[ Bug 9134 ]] Make sure the we mark the stream as 'ATEOF'
                if (MCprocesses[i] . ihandle != nil)
                    MCprocesses[i] . ihandle -> flags |= IO_ATEOF;
                */

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
    }
    
	virtual uint32_t GetSystemError(void)
	{
#ifdef /* MCS_getsyserror_dsk_w32 */ LEGACY_SYSTEM
	return GetLastError();
#endif /* MCS_getsyserror_dsk_w32 */
		return GetLastError();
	}
    
    // MW-2010-05-09: Updated to add 'elevated' parameter for executing binaries
    //   at increased privilege level.
    virtual bool StartProcess(MCNameRef p_name, MCStringRef p_doc, intenum_t p_mode, Boolean p_elevated)
    {
#ifdef /* MCS_startprocess_dsk_w32 */ LEGACY_SYSTEM
        Boolean reading = mode == OM_READ || mode == OM_UPDATE;
        Boolean writing = mode == OM_APPEND || mode == OM_WRITE || mode == OM_UPDATE;
        MCU_realloc((char **)&MCprocesses, MCnprocesses, MCnprocesses + 1,
                    sizeof(Streamnode));
        MCprocesses[MCnprocesses].name = name;
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
            char *cmdline = name;
            if (doc != NULL && *doc != '\0')
            {
                cmdline = new char[strlen(name) + strlen(doc) + 4];
                sprintf(cmdline, "%s \"%s\"", name, doc);
            }
            
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
                    if (CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &siStartInfo, &piProcInfo))
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
            
            if (doc != NULL)
            {
                if (*doc != '\0')
                    delete cmdline;
                delete doc;
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
        MCprocesses[MCnprocesses].mode = (Open_mode)p_mode;
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
            if (p_doc != nil && !MCStringIsEmpty(p_doc))
				/* UNCHECKED */ MCStringFormat(&t_cmdline, "%@ \"%@\"", p_name, p_doc);
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
                    STARTUPINFOW siStartInfo;
                    memset(&siStartInfo, 0, sizeof(STARTUPINFOW));
                    siStartInfo.cb = sizeof(STARTUPINFOW);
                    siStartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
                    if (MChidewindows)
                        siStartInfo.wShowWindow = SW_HIDE;
                    else
                        siStartInfo.wShowWindow = SW_SHOW;
                    siStartInfo.hStdInput = hChildStdinRd;
                    siStartInfo.hStdOutput = hChildStdoutWr;
                    siStartInfo.hStdError = hChildStderrWr;

					MCAutoStringRefAsWString t_cmdline_wstr;
					/* UNCHECKED */ t_cmdline_wstr.Lock(*t_cmdline);

                    if (CreateProcessW(NULL, *t_cmdline_wstr, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &siStartInfo, &piProcInfo))
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
                
                unichar_t t_parameters[64];
                wsprintfW(t_parameters, L"-elevated-slave%08x", GetCurrentThreadId());
                
				MCAutoStringRefAsWString t_cmd_wstr;
				/* UNCHECKED */ t_cmd_wstr.Lock(MCcmd);

                SHELLEXECUTEINFOW t_info;
                memset(&t_info, 0, sizeof(SHELLEXECUTEINFOW));
                t_info . cbSize = sizeof(SHELLEXECUTEINFOW);
                t_info . fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE ;
                t_info . hwnd = (HWND)MCdefaultstackptr -> getrealwindow();
                t_info . lpVerb = L"runas";
                t_info . lpFile = *t_cmd_wstr;
                t_info . lpParameters = t_parameters;
                t_info . nShow = SW_HIDE;
                if (ShellExecuteExW(&t_info) && (uintptr_t)t_info . hInstApp > 32)
                {
                    MSG t_msg;
                    t_msg . message = WM_QUIT;
                    while(!PeekMessageW(&t_msg, (HWND)-1, WM_USER + 10, WM_USER + 10, PM_REMOVE))
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
						LPWCH lpEnvStrings;
						lpEnvStrings = GetEnvironmentStringsW();
						size_t t_env_length = 0;
                        if (lpEnvStrings != nil)
						{
							// The environment block is terminated with a double-null                           
							t_env_length = 0;
                            while(lpEnvStrings[t_env_length] != '\0' || lpEnvStrings[t_env_length + 1] != '\0')
                                t_env_length += 1;
                            t_env_length += 2;
                        }
                        
                        // Write out the cmd line and env strings
                        MCAutoStringRefAsWString t_cmdline_wstr;
                        t_cmdline_wstr.Lock(*t_cmdline);
                        if (write_blob_to_pipe(t_output_pipe, sizeof(wchar_t) * (MCStringGetLength(*t_cmdline) + 1), *t_cmdline_wstr) &&
                            write_blob_to_pipe(t_output_pipe, sizeof(wchar_t) * t_env_length, lpEnvStrings))
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
                        
                        FreeEnvironmentStringsW(lpEnvStrings);
                        
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
				MCprocesses[MCnprocesses].ohandle = new MCStdioFileHandle((MCWinSysHandle)hChildStdinWr, true);
            else
                CloseHandle(hChildStdinWr);

            if (reading)
				MCprocesses[MCnprocesses].ihandle = new MCStdioFileHandle((MCWinSysHandle)hChildStdoutRd, true);
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

		if (!created)
			return False;

		return True;
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

	virtual Boolean Poll(real8 p_delay, int p_fd)
	{
#ifdef /* MCS_poll_dsk_w32 */ LEGACY_SYSTEM
	Boolean handled = False;
	int4 n;
	uint2 i;
	fd_set rmaskfd, wmaskfd, emaskfd;
	FD_ZERO(&rmaskfd);
	FD_ZERO(&wmaskfd);
	FD_ZERO(&emaskfd);
	uint4 maxfd = 0;
	if (!MCnoui)
	{
		FD_SET(fd, &rmaskfd);
		maxfd = fd;
	}
	for (i = 0 ; i < MCnsockets ; i++)
	{
		if (MCsockets[i]->doread)
		{
			MCsockets[i]->readsome();
			i = 0;
		}
	}
	for (i = 0 ; i < MCnsockets ; i++)
	{
		if (MCsockets[i]->connected && !MCsockets[i]->closing
		        && !MCsockets[i]->shared || MCsockets[i]->accepting)
			FD_SET(MCsockets[i]->fd, &rmaskfd);
		if (!MCsockets[i]->connected || MCsockets[i]->wevents != NULL)
			FD_SET(MCsockets[i]->fd, &wmaskfd);
		FD_SET(MCsockets[i]->fd, &emaskfd);
		if (MCsockets[i]->fd > maxfd)
			maxfd = MCsockets[i]->fd;
		if (MCsockets[i]->added)
		{
			delay = 0.0;
			MCsockets[i]->added = False;
			handled = True;
		}
	}
	struct timeval timeoutval;
	timeoutval.tv_sec = (long)delay;
	timeoutval.tv_usec = (long)((delay - floor(delay)) * 1000000.0);
	n = select(maxfd + 1, &rmaskfd, &wmaskfd, &emaskfd, &timeoutval);
	if (n <= 0)
		return handled;
	for (i = 0 ; i < MCnsockets ; i++)
	{
		if (FD_ISSET(MCsockets[i]->fd, &emaskfd))
		{
			if (!MCsockets[i]->waiting)
			{
				MCsockets[i]->error = strclone("select error");
				MCsockets[i]->doclose();
			}
		}
		else
		{
			if (FD_ISSET(MCsockets[i]->fd, &wmaskfd))
				MCsockets[i]->writesome();
			if (FD_ISSET(MCsockets[i]->fd, &rmaskfd) && !MCsockets[i]->shared)
				MCsockets[i]->readsome();
		}
	}
	return n != 0;
#endif /* MCS_poll_dsk_w32 */
		Boolean handled = False;
		int4 n;
		uint2 i;
		fd_set rmaskfd, wmaskfd, emaskfd;
		FD_ZERO(&rmaskfd);
		FD_ZERO(&wmaskfd);
		FD_ZERO(&emaskfd);
		int4 maxfd = 0;
		if (!MCnoui)
		{
			FD_SET(p_fd, &rmaskfd);
			maxfd = p_fd;
		}
		for (i = 0 ; i < MCnsockets ; i++)
		{
			if (MCsockets[i]->doread)
			{
				MCsockets[i]->readsome();
				i = 0;
			}
		}
        handled = MCSocketsAddToFileDescriptorSets(maxfd, rmaskfd, wmaskfd, emaskfd);
        if (handled)
            p_delay = 0.0;
		struct timeval timeoutval;
		timeoutval.tv_sec = (long)p_delay;
		timeoutval.tv_usec = (long)((p_delay - floor(p_delay)) * 1000000.0);
		n = select(maxfd + 1, &rmaskfd, &wmaskfd, &emaskfd, &timeoutval);
		if (n <= 0)
			return handled;
        MCSocketsHandleFileDescriptorSets(rmaskfd, wmaskfd, emaskfd);
		return n != 0;
	}
    
    virtual Boolean IsInteractiveConsole(int p_fd)
	{
		if (_isatty(p_fd) != 0)
			return True;

		return False;
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
    
    virtual void LaunchDocument(MCStringRef p_document)
	{
#ifdef /* MCS_launch_document_dsk_w32 */ LEGACY_SYSTEM
        char *t_native_document;
        
        t_native_document = MCS_resolvepath(p_document);
        delete p_document;
        
        // MW-2007-12-13: [[ Bug 5680 ]] Might help if we actually passed the correct
        //   pointer to do_launch!
        MCS_do_launch(t_native_document);
        
        delete t_native_document;
#endif /* MCS_launch_document_dsk_w32 */
		MCS_do_launch(p_document);
	}

    virtual void LaunchUrl(MCStringRef p_document)
	{
#ifdef /* MCS_launch_url_dsk_w32 */ LEGACY_SYSTEM
        MCS_do_launch(p_document);
        
        // MW-2007-12-13: <p_document> is owned by the callee
        delete p_document;
#endif /* MCS_launch_url_dsk_w32 */
		MCS_do_launch(p_document);
	}
    
    virtual void DoAlternateLanguage(MCStringRef p_script, MCStringRef p_language)
	{
#ifdef /* MCS_doalternatelanguage_dsk_w32 */ LEGACY_SYSTEM
        MCScriptEnvironment *t_environment;
        t_environment = MCscreen -> createscriptenvironment(langname);
        if (t_environment == NULL)
            MCresult -> sets("alternate language not found");
        else
        {
            MCExecPoint ep(NULL, NULL, NULL);
            ep . setsvalue(s);
            ep . nativetoutf8();
            
            char *t_result;
            t_result = t_environment -> Run(ep . getcstring());
            t_environment -> Release();
            
            if (t_result != NULL)
            {
                ep . setsvalue(t_result);
                ep . utf8tonative();
                MCresult -> copysvalue(ep . getsvalue());
            }
            else
                MCresult -> sets("execution error");
        }
#endif /* MCS_doalternatelanguage_dsk_w32 */
		MCScriptEnvironment *t_environment;
		t_environment = MCscreen -> createscriptenvironment(p_language);
		if (t_environment == NULL)
			MCresult -> sets("alternate language not found");
		else
		{
			MCAutoStringRefAsUTF8String t_utf8;
			/* UNCHECKED */ t_utf8.Lock(p_script);
			
			MCAutoStringRef t_result;
			MCAutoStringRef t_string;
			/* UNCHECKED */ MCStringCreateWithCString(*t_utf8, &t_string);
			t_environment -> Run(*t_string, &t_result);
			t_environment -> Release();

			if (*t_result != nil)
			{
				MCAutoDataRef t_data;
				MCAutoStringRef t_native;
				MCDataCreateWithBytes((const byte_t*)MCStringGetCString(*t_result), MCStringGetLength(*t_result), &t_data);

				MCStringDecode(*t_data, kMCStringEncodingUTF8, false, &t_native);
				MCresult -> setvalueref(*t_native);
			}
			else
				MCresult -> sets("execution error");
		}
	}

    virtual bool AlternateLanguages(MCListRef& r_list)
	{
#ifdef /* MCS_alternatelanguages_dsk_w32 */ LEGACY_SYSTEM
        ep.clear();
        
        HRESULT t_result;
        t_result = S_OK;
        
        ICatInformation *t_cat_info;
        t_cat_info = NULL;
        if (t_result == S_OK)
            t_result = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatInformation, (void **)&t_cat_info);
        
        IEnumCLSID *t_cls_enum;
        t_cls_enum = NULL;
        if (t_result == S_OK)
            t_result = t_cat_info -> EnumClassesOfCategories(1, &CATID_ActiveScriptParse, (ULONG)-1, NULL, &t_cls_enum);
        
        if (t_result == S_OK)
        {
            GUID t_cls_uuid;
            unsigned int t_index;
            t_index = 0;
            
            while(t_cls_enum -> Next(1, &t_cls_uuid, NULL) == S_OK)
            {
                LPOLESTR t_prog_id;
                if (ProgIDFromCLSID(t_cls_uuid, &t_prog_id) == S_OK)
                {
                    MCExecPoint t_unicode_ep(NULL, NULL, NULL);
                    t_unicode_ep . setsvalue(MCString((char *)t_prog_id, wcslen(t_prog_id) * 2));
                    t_unicode_ep . utf16tonative();
                    ep . concatmcstring(t_unicode_ep . getsvalue(), EC_RETURN, t_index == 0);
                    t_index += 1;
                    
                    CoTaskMemFree(t_prog_id);
                }
            }
        }
        
        if (t_cls_enum != NULL)
            t_cls_enum -> Release();
        
        if (t_cat_info != NULL)
            t_cat_info -> Release();
#endif /* MCS_alternatelanguages_dsk_w32 */
		MCAutoListRef t_list;
		
		if (!MCListCreateMutable('\n', &t_list))
			return false;

		bool t_success = true;
		HRESULT t_result;
		t_result = S_OK;

		ICatInformation *t_cat_info;
		t_cat_info = NULL;
		if (t_result == S_OK)
			t_result = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatInformation, (void **)&t_cat_info);
		
		IEnumCLSID *t_cls_enum;
		t_cls_enum = NULL;
		if (t_result == S_OK)
			t_result = t_cat_info -> EnumClassesOfCategories(1, &CATID_ActiveScriptParse, (ULONG)-1, NULL, &t_cls_enum);

		if (t_result == S_OK)
		{
			GUID t_cls_uuid;

			while(t_success && t_cls_enum -> Next(1, &t_cls_uuid, NULL) == S_OK)
			{
				LPOLESTR t_prog_id;
				if (ProgIDFromCLSID(t_cls_uuid, &t_prog_id) == S_OK)
				{
					MCAutoStringRef t_string;
					t_success = MCStringCreateWithChars(t_prog_id, wcslen(t_prog_id), &t_string) && MCListAppend(*t_list, *t_string);

					CoTaskMemFree(t_prog_id);
				}
			}
		}

		if (t_cls_enum != NULL)
			t_cls_enum -> Release();

		if (t_cat_info != NULL)
			t_cat_info -> Release();
		
		return t_success && MCListCopy(*t_list, r_list);
	}
    
    virtual bool GetDNSservers(MCListRef& r_list)
    {
#ifdef /* MCS_getDNSservers_dsk_w32 */ LEGACY_SYSTEM
        ep.clear();
        
        ULONG bl = sizeof(FIXED_INFO);
        FIXED_INFO *fi = (FIXED_INFO *)new char[bl];
        memset(fi, 0, bl);
        if ((errno = GetNetworkParams(fi, &bl)) == ERROR_BUFFER_OVERFLOW)
        {
            delete fi;
            fi = (FIXED_INFO *)new char[bl];
            memset(fi, 0, bl);
            errno = GetNetworkParams(fi, &bl);
        }
        IP_ADDR_STRING *pIPAddr = &fi->DnsServerList;
        if (errno == ERROR_SUCCESS && *pIPAddr->IpAddress.String)
        {
            uint2 i = 0;
            do
            {
                ep.concatcstring(pIPAddr->IpAddress.String, EC_RETURN, i++ == 0);
                pIPAddr = pIPAddr->Next;
            }
            while (pIPAddr != NULL);
        }
        delete fi;
        
        if (ep.getsvalue().getlength() == 0)
        {
            MCScreenDC *pms = (MCScreenDC *)MCscreen;
            ep.setstaticcstring("HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\Services\\Tcpip\\Parameters\\NameServer");
            MCS_query_registry(ep, NULL);
            char *sptr = ep.getbuffer(0);
            uint4 l = ep.getsvalue().getlength();
            while (l--)
                if (sptr[l] == ' ' || sptr[l] == ',')
                    sptr[l] = '\n';
        }
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
		STARTUPINFOW siStartInfo;
		memset(&siStartInfo, 0, sizeof(STARTUPINFOW));
		siStartInfo.cb = sizeof(STARTUPINFOW);
		siStartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		siStartInfo.wShowWindow = SW_HIDE;
		siStartInfo.hStdInput = t_fromparent_read;
		siStartInfo.hStdOutput = t_toparent_write;
		siStartInfo.hStdError = t_toparent_write_dup;
		if (CreateProcessW(NULL, (LPWSTR)t_cmd_line, NULL, NULL, TRUE, CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_CONSOLE | CREATE_SUSPENDED, t_env_strings, NULL, &siStartInfo, &piProcInfo))
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

void MCS_multibytetounicode(const char *s, uint4 len, char *d,
                            uint4 destbufferlength, uint4 &destlen,
                            uint1 charset)
{
	char szLocaleData[6];
	uint2 codepage = 0;
	GetLocaleInfoA(MAKELCID(MCS_charsettolangid(charset), SORT_DEFAULT) ,
	               LOCALE_IDEFAULTANSICODEPAGE, szLocaleData, 6);
	codepage = (uint2)strtoul(szLocaleData, NULL, 10);
	uint4 dsize = MultiByteToWideChar( codepage, 0, s, len, (LPWSTR)d,
	                                   destbufferlength >> 1);
	destlen = dsize << 1;
}

void MCS_unicodetomultibyte(const char *s, uint4 len, char *d,
                            uint4 destbufferlength, uint4 &destlen,
                            uint1 charset)
{
	char szLocaleData[6];
	uint2 codepage = 0;
	GetLocaleInfoA(MAKELCID(MCS_charsettolangid(charset), SORT_DEFAULT)
	               , LOCALE_IDEFAULTANSICODEPAGE, szLocaleData, 6);
	codepage = (uint2)strtoul(szLocaleData, NULL, 10);
	uint4 dsize = WideCharToMultiByte( codepage, 0, (LPCWSTR)s, len >> 1,
	                                   d, destbufferlength, NULL, NULL);
	destlen = dsize;
}

MCSystemInterface *MCDesktopCreateWindowsSystem(void)
{
	return new MCWindowsDesktop;
}

struct  LangID2Charset
{
	Lang_charset charset;
	LANGID langid;
};

static LangID2Charset langidtocharsets[] = {
    { LCH_ENGLISH, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT) },
    { LCH_ROMAN, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT)},
    { LCH_JAPANESE, MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT) },
    { LCH_CHINESE, MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL) },
    { LCH_ARABIC, MAKELANGID(LANG_ARABIC, SUBLANG_DEFAULT) },
    { LCH_RUSSIAN, MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT) },
    { LCH_TURKISH, MAKELANGID(LANG_TURKISH, SUBLANG_DEFAULT) },
    { LCH_BULGARIAN, MAKELANGID(LANG_BULGARIAN, SUBLANG_DEFAULT) },
    { LCH_POLISH, MAKELANGID(LANG_POLISH, SUBLANG_DEFAULT) },
    { LCH_UKRAINIAN, MAKELANGID(LANG_UKRAINIAN, SUBLANG_DEFAULT) },
    { LCH_HEBREW, MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT)},
    { LCH_GREEK, MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT) },
    { LCH_KOREAN, MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT) },
    { LCH_THAI,	MAKELANGID(LANG_THAI, SUBLANG_DEFAULT) },
    { LCH_SIMPLE_CHINESE, MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)},
    { LCH_UNICODE, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)}
};

uint1 MCS_langidtocharset(uint2 langid)
{
	uint2 i;
	for (i = 0; i < ELEMENTS(langidtocharsets); i++)
		if (langidtocharsets[i].langid == langid)
			return langidtocharsets[i].charset;
	return 0;
}

uint2 MCS_charsettolangid(uint1 charset)
{
	uint2 i;
	for (i = 0; i < ELEMENTS(langidtocharsets); i++)
		if (langidtocharsets[i].charset == charset)
			return langidtocharsets[i].langid;
	return 0;
}

bool MCS_generate_uuid(char p_buffer[128])
{
	GUID t_guid;
	if (CoCreateGuid(&t_guid) == S_OK)
	{
		unsigned char __RPC_FAR *t_guid_string;
		if (UuidToStringA(&t_guid, &t_guid_string) == RPC_S_OK)
		{
			strcpy(p_buffer, (char *)t_guid_string);
			RpcStringFreeA(&t_guid_string);
		}
        
		return true;
	}
    
	return false;
}

////////////////////////////////////////////////////////////////////////////////

