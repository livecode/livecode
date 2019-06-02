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
#include <AclAPI.h>

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
		/* UNCHECKED */ MCStringAppendSubstring(t_temp, p_legacy, MCRangeMakeMinMax(2, MCStringGetLength(p_legacy)));
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
		/* UNCHECKED */ MCStringMutableCopySubstring(p_nt, MCRangeMakeMinMax(8, MCStringGetLength(p_nt)), t_temp);
		/* UNCHECKED */ MCStringPrepend(t_temp, MCSTR("\\\\"));
		/* UNCHECKED */ MCStringCopyAndRelease(t_temp, r_legacy);
	}
	else if (MCStringBeginsWithCString(p_nt, (const char_t*)"\\\\?\\", kMCStringOptionCompareCaseless))
	{
		/* UNCHECKED */ MCStringCopySubstring(p_nt, MCRangeMakeMinMax(4, MCStringGetLength(p_nt)), r_legacy);
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
				t_success = t_success && MCStringCopySubstring(p_path, MCRangeMakeMinMax(t_path_offset + 1, t_value_offset), r_key);
		}
		t_success = t_success && MCStringCopySubstring(p_path, MCRangeMakeMinMax(t_value_offset + 1, t_length), r_value);
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

void CALLBACK MCS_tp(UINT id, UINT msg, DWORD_PTR user, DWORD_PTR dw1, DWORD_PTR dw2)
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
		if (MCNameIsEqualToCaseless(p_folder, *(sysfolderlist[i].token)))
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
	char* t_buffer = new (nothrow) char[t_bufsize];

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
			if (!MCStringCopySubstring(t_string, MCRangeMakeMinMax(t_start, i), &t_substring) || 
					!MCListAppend(*t_list, *t_substring))
				return false;
			t_start = i + 1;
		}
	}
	if (t_start < t_char_count)
	{
		MCAutoStringRef t_final_string;
		if (!MCStringCopySubstring(t_string, MCRangeMakeMinMax(t_start, MCStringGetLength(t_string)), &t_final_string) ||
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
		if (!SetEndOfFile(m_handle))
			return false;

		return true;
	}

	virtual bool Sync(void)
	{
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
		// SN-2014-06-16 
		// FileFlushBuffer returns non-zero on success
		// (which is the opposite of NO_ERROR
		if (FlushFileBuffers(m_handle) == 0)
			return false;

		return true;
	}
	
	virtual bool PutBack(char p_char)
	{
		if (m_putback != -1)
			return false;
			
		m_putback = p_char;
		
		return true;
	}
	
	virtual int64_t Tell(void)
	{
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

	virtual uint64_t GetFileSize(void)
	{
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
		IO_stdin = MCsystem -> OpenFd(STD_INPUT_HANDLE, kMCOpenFileModeRead);
		IO_stdout = MCsystem -> OpenFd(STD_OUTPUT_HANDLE, kMCOpenFileModeWrite);
		IO_stderr = MCsystem -> OpenFd(STD_ERROR_HANDLE, kMCOpenFileModeWrite);

		setlocale(LC_CTYPE, MCnullstring);
		setlocale(LC_COLLATE, MCnullstring);

#ifdef _WINDOWS_SERVER
		WORD request = MAKEWORD(1, 1);
		WSADATA t_data;
		WSAStartup(request, &t_data);
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
        
        MCAutoNumberRef t_enabled;
        if (*t_value != nil && ctxt.ConvertToNumber(*t_value, &t_enabled) && MCNumberFetchAsInteger(*t_enabled) == 1)
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
        OleUninitialize();
    }
	
	virtual void Debug(MCStringRef p_string)
	{
		MCAutoStringRefAsWString t_string;
		if (t_string.Lock(p_string))
			OutputDebugStringW(*t_string);
	}

	virtual real64_t GetCurrentTime()
    {
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
        if (!MClowrestimers)
        {
            startcount = 0;
            MCS_time();
            startcount = timeGetTime();
        }
    }
    
	virtual bool GetVersion(MCStringRef& r_string)
    {
        return MCStringFormat(r_string, "NT %d.%d", (MCmajorosversion >> 8) & 0xFF, MCmajorosversion & 0xFF);
    }
    
	virtual bool GetMachine(MCStringRef& r_string)
    {
        return MCS_getprocessor(r_string);
    }
    
	virtual bool GetAddress(MCStringRef& r_address)
    {
        if (!wsainit())
		{
			r_address = MCSTR("unknown");
			return true;
		}
        
        char *buffer = new (nothrow) char[MAXHOSTNAMELEN + 1];
        gethostname(buffer, MAXHOSTNAMELEN);
		buffer[MAXHOSTNAMELEN] = '\0';
        return MCStringFormat(r_address, "%s:%@", buffer, MCcmd);
    }
    
	virtual uint32_t GetProcessId(void)
    {
        return _getpid();
    }
	
	virtual void Alarm(real64_t p_when)
    {
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
        SleepEx((DWORD)(p_when * 1000.0), False);  //takes milliseconds as parameter
    }
	
	virtual void SetEnv(MCStringRef p_name, MCStringRef p_value)
    {
		MCAutoStringRefAsWString t_name_wstr, t_value_wstr;
		/* UNCHECKED */ t_name_wstr.Lock(p_name);
		/* UNCHECKED */ t_value_wstr.Lock(p_value);

		/* UNCHECKED */ SetEnvironmentVariableW(*t_name_wstr, *t_value_wstr);
    }
                
	virtual bool GetEnv(MCStringRef p_name, MCStringRef& r_value)
    {
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
		MCAutoStringRefAsWString t_path_wstr;
		/* UNCHECKED */ t_path_wstr.Lock(p_path);
	
		if (!CreateDirectoryW(*t_path_wstr, NULL))
            return False;
        
        return True;
    }
    
	virtual Boolean DeleteFolder(MCStringRef p_path)
    {
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
		MCAutoStringRefAsWString t_path_wstr;
		/* UNCHECKED */ t_path_wstr.Lock(p_path);

		return DeleteFileW(*t_path_wstr);
    }
	
	virtual Boolean RenameFileOrFolder(MCStringRef p_old_name, MCStringRef p_new_name)
    {
		MCAutoStringRefAsWString t_old_wstr, t_new_wstr;
		/* UNCHECKED */ t_old_wstr.Lock(p_old_name);
		/* UNCHECKED */ t_new_wstr.Lock(p_new_name);
	
		// The Ex form is used to allow directory renaming across drives.
		// Write-through means this doesn't return until a cross-drive move is complete.
		return MoveFileExW(*t_old_wstr, *t_new_wstr, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH);
    }
	
	virtual Boolean BackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
    {
        return RenameFileOrFolder(p_old_name, p_new_name);
    }
    
	virtual Boolean UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
    {
        return MCS_rename(p_old_name, p_new_name);
    }
	
	virtual Boolean CreateAlias(MCStringRef p_target, MCStringRef p_alias)
    {
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
        return MCS_getcurdir_native(r_path);
    }
    
	virtual Boolean SetCurrentFolder(MCStringRef p_path)
    {
		MCAutoStringRefAsWString t_path_wstr;
		if (!t_path_wstr.Lock(p_path))
			return false;

        if (!SetCurrentDirectoryW(*t_path_wstr))
            return False;
            
        return True;
    }
	
	// NOTE: 'GetStandardFolder' returns a native path.
	virtual Boolean GetStandardFolder(MCNameRef p_type, MCStringRef& r_folder)
    {
        bool t_wasfound = false;
        MCAutoNumberRef t_special_folder;
        MCAutoStringRef t_native_path;
        if (MCNameIsEqualToCaseless(p_type, MCN_temporary))
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
        else if (MCNameIsEqualToCaseless(p_type, MCN_engine)
                 || MCNameIsEqualToCaseless(p_type, MCN_resources))
        {
            MCAutoStringRef t_engine_folder;
            uindex_t t_last_slash;
            
            if (!MCStringLastIndexOfChar(MCcmd, '/', UINDEX_MAX, kMCStringOptionCompareExact, t_last_slash))
                t_last_slash = MCStringGetLength(MCcmd);
            
            MCAutoStringRef t_livecode_path;
            if (!MCStringCopySubstring(MCcmd, MCRangeMake(0, t_last_slash), &t_livecode_path))
                return False;

            if (!PathToNative(*t_livecode_path, &t_native_path))
                return False;
            
            t_wasfound = true;
            
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
		DWORD sc, bs, fc, tc;
		GetDiskFreeSpace(NULL, &sc, &bs, &fc, &tc);
		return ((real8)bs * (real8)sc * (real8)fc);
	}


    virtual Boolean GetDevices(MCStringRef& r_devices)
	{
		r_devices = MCValueRetain(kMCEmptyString);
		return true;
	}


    virtual Boolean GetDrives(MCStringRef& r_drives)
	{
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
        return _umask(p_mask);
    }
    
	virtual IO_handle OpenFile(MCStringRef p_path, intenum_t p_mode, Boolean p_map)
    {
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
					t_handle = new (nothrow) MCStdioFileHandle((MCWinSysHandle)t_file_handle);
                    t_close_file_handler = t_handle == NULL;
				}
				else
				{
					t_handle = new (nothrow) MCMemoryMappedFileHandle(t_file_mapped_handle, t_buffer, t_len);
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
				t_handle = new (nothrow) MCStdioFileHandle((MCWinSysHandle)t_file_handle);
                t_close_file_handler = t_handle == NULL;
            }
		}
		else
        {
			t_handle = new (nothrow) MCStdioFileHandle((MCWinSysHandle)t_file_handle);
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
		t_stdio_handle = new (nothrow) MCStdioFileHandle((MCWinSysHandle)t_handle, true);

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

	virtual bool ListFolderEntries(MCStringRef p_folder, MCSystemListFolderEntriesCallback p_callback, void *x_context)
    {
        WIN32_FIND_DATAW data;
        HANDLE ffh;            //find file handle

        MCAutoStringRef t_dir_native;
        MCAutoStringRef t_search_path;
        
		// The search is done in the current directory
		if (p_folder == nil)
			MCS_getcurdir_native(&t_dir_native);
		else
			&t_dir_native = MCValueRetain (p_folder);

		// Search strings need to have a wild-card added
		if (MCStringGetCharAtIndex(*t_dir_native, MCStringGetLength(*t_dir_native) - 1) == '\\')
		{
			/* UNCHECKED */ MCStringFormat(&t_search_path, "%@*", *t_dir_native);
		}
		else
		{
			/* UNCHECKED */ MCStringFormat(&t_search_path, "%@\\*", *t_dir_native);
		}

		// Iterate through the contents of the directory
		MCAutoStringRefAsWString t_search_wstr;
		if (!t_search_wstr.Lock(*t_search_path))
			return false;
		ffh = FindFirstFileW(*t_search_wstr, &data);
		if (ffh == INVALID_HANDLE_VALUE)
		{
			return false;
		}

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

	virtual bool PathToNative(MCStringRef p_path, MCStringRef& r_native)
	{
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
		// Parameters for MCSystemInterface functions are always native paths
		return ResolveNativePath(p_path, r_resolved_path);
	}
    
	static bool isValidSerialPortPath(MCStringRef p_path)
    {
        // All serial port paths end with ":", and are at least 4 chars long
        uindex_t t_len = MCStringGetLength(p_path);
		
        if (t_len < 4 || MCStringGetCharAtIndex(p_path, t_len-1) != ':')
            return false;
        
        typedef struct
        {
            const char *m_prefix;
            bool m_numbered;
        } SerialPortInfo;
		
        SerialPortInfo serialPortNames[]=
        {
            {"COM", true},
            {"LPT", true},
            {"CON", false},
            {"PRN", false},
            {"AUX", false},
            {"NUL", false},
        };
		
        size_t t_size = sizeof(serialPortNames) / sizeof(serialPortNames[0]);
        for (int i=0; i<t_size; i++)
        {
            /* Must always begin with the correct prefix */
            if (!MCStringBeginsWithCString(p_path, (const char_t*)serialPortNames[i].m_prefix, kMCStringOptionCompareCaseless))
            {
                continue;
            }
			
            // If not numbered, then there should only be prefix + ":"
            // (and we already checked for the ":" earlier)
            if (!serialPortNames[i].m_numbered)
            {
                return t_len == 4;
            }
			
            /* Check for a numbered part, which must be non-empty */
            if (t_len <= 4)
            {
                return false;
            }
			
            for (int t_offset = 3; t_offset < t_len - 1; ++t_offset)
            {
                if (!isdigit(MCStringGetCharAtIndex(p_path, t_offset)))
                {
                    return false;
                }
            }
            return true;
        }
        return false;
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
		
		// PM-2016-03-15: [[ Bug 16300 ]] Detect correctly the case of a serial port path
		else if (isValidSerialPortPath(p_path))
		{
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
		MCAutoStringRefAsLPWSTR t_wcmd;
		t_wcmd . Lock(t_cmd);
        MCU_realloc((char **)&MCprocesses, MCnprocesses,
                    MCnprocesses + 1, sizeof(Streamnode));
        uint4 index = MCnprocesses;
        MCprocesses[index].name = (MCNameRef)MCValueRetain(MCM_shell);
        MCprocesses[index].mode = OM_NEITHER;
        MCprocesses[index].ohandle = new (nothrow) MCMemoryFileHandle;
		MCprocesses[index].ihandle = new (nothrow) MCStdioFileHandle((MCWinSysHandle)hChildStdoutRd, true);
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
		return GetLastError();
	}
    
    // MW-2010-05-09: Updated to add 'elevated' parameter for executing binaries
    //   at increased privilege level.
    virtual bool StartProcess(MCNameRef p_name, MCStringRef p_doc, intenum_t p_mode, Boolean p_elevated)
    {
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

					MCAutoStringRefAsLPWSTR t_cmdline_wstr;
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
				bool t_access_denied = false;
				created = StartElevatedProcess(*t_cmdline, hChildStdinWr, hChildStdoutRd, t_process_handle, t_process_id, t_access_denied);
				if (!created &&
					t_access_denied)
					t_error = "access denied";
			}
        }
        if (created)
        {
            if (writing)
				MCprocesses[MCnprocesses].ohandle = new (nothrow) MCStdioFileHandle((MCWinSysHandle)hChildStdinWr, true);
            else
                CloseHandle(hChildStdinWr);

            if (reading)
				MCprocesses[MCnprocesses].ihandle = new (nothrow) MCStdioFileHandle((MCWinSysHandle)hChildStdoutRd, true);
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

	BOOL StartElevatedProcess(MCStringRef p_cmd_line, HANDLE& r_child_stdin_wr, HANDLE& r_child_stdout_rd, HANDLE& r_child_process, DWORD& r_child_id, bool& r_access_denied)
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

		// This is true if the slave died at some point during the transaction.
		// Generally meaning that an error occurred which means it can't launch
		// the process we want.
		BOOL t_slave_died = FALSE;
		BOOL t_slave_deaf = FALSE;

		// The slave (mediating) process we run has a specific command-line
		// which encodes the current thread id of this process.
        unichar_t t_parameters[64];
        wsprintfW(t_parameters, L"-elevated-slave%08x", GetCurrentThreadId());

		// We need the command line of the running engine as a wstring.
		MCAutoStringRefAsWString t_slave_wstr;
		// We need the command line of the requested process as a wstring.
		MCAutoStringRefAsWString t_cmd_line_wstr;
		if (!t_slave_wstr.Lock(MCcmd) ||
			!t_cmd_line_wstr.Lock(p_cmd_line))
		{
			t_slave_died = TRUE;
			goto cleanup;
		}

		// This records the Win32 error code if one of the API calls fails.
		DWORD t_error = ERROR_SUCCESS;
		// This is the handle of the slave process used to launch the requested process.
		HANDLE t_slave_process = NULL;
		// This is the thread id of the main thread in the slave process.
		DWORD t_slave_thread_id = 0;
		// This is the slave's security descriptor containing it's sid.
		PSECURITY_DESCRIPTOR t_slave_security_descriptor = NULL;
		PSID t_slave_sid = NULL;
		// This is our security descriptor containing our original DACL.
		PSECURITY_DESCRIPTOR t_master_security_descriptor = NULL;
		PACL t_master_dacl = NULL;
		// This is our augmented DACL.
		PACL t_augmented_master_dacl = NULL;
		// These are the pipes used to communicate first with the slave, and then
		// with the requested process.
		HANDLE t_output_pipe = NULL;
		HANDLE t_input_pipe = NULL;
		// These are our environment strings.
		LPWCH t_env_strings = NULL;
		size_t t_env_length = 0;
		// These are the requested process handle and id.
		HANDLE t_process_handle = NULL;
		DWORD t_process_id = 0;

		// First we run the engine itself as administrator, passing to it our
		// thread-id as part of the command-line arguments.
        SHELLEXECUTEINFOW t_info;
        memset(&t_info, 0, sizeof(SHELLEXECUTEINFOW));
        t_info . cbSize = sizeof(SHELLEXECUTEINFOW);
        t_info . fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE ;
        t_info . hwnd = (HWND)MCdefaultstackptr -> getrealwindow();
        t_info . lpVerb = L"runas";
        t_info . lpFile = *t_slave_wstr;
        t_info . lpParameters = t_parameters;
        t_info . nShow = SW_HIDE;
        if (!ShellExecuteExW(&t_info) ||
			(uintptr_t)t_info . hInstApp < 32)
        {
            if ((uintptr_t)t_info . hInstApp == SE_ERR_ACCESSDENIED)
				r_access_denied = true;

			t_error = GetLastError();
			t_slave_died = TRUE;
			goto cleanup;
		}

		t_slave_process = t_info.hProcess;

		// We now have a process handle, but we need a thread id so that
		// we can notify the slave when to continue. The slave sends us
		// the thread id immediately after launch as a thread message.
        MSG t_msg;
        t_msg . message = WM_QUIT;
        while(!PeekMessageW(&t_msg, (HWND)-1, WM_USER + 10, WM_USER + 10, PM_REMOVE))
            if (MsgWaitForMultipleObjects(1, &t_slave_process, FALSE, INFINITE, QS_POSTMESSAGE) == WAIT_OBJECT_0)
            {
				t_slave_died = TRUE;
                goto cleanup;
            }
			
		if (t_msg.message != WM_USER + 10)
		{
			t_slave_deaf = TRUE;
			goto cleanup;
		}

		t_slave_thread_id = t_msg.wParam;

		// Now we have a process handle, we must allow that process's user to
		// open our process handle with PROCESS_DUP_HANDLE right. This is a
		// multi-step process:
		//   1) First we get the SID of the slave process.
		//   2) Get the current DACL of this process.
		//   3) Add an explicit access right to PROCESS_DUP_HANDLE for the SID
		//      of the slave process to the fetched DACL
		//   4) Set the DACL of our process to the augmented one.

		// Step 1
		if (ERROR_SUCCESS != GetSecurityInfo(
								t_info.hProcess,
								SE_KERNEL_OBJECT,
								OWNER_SECURITY_INFORMATION,
								&t_slave_sid,
								NULL,
								NULL,
								NULL,
								&t_slave_security_descriptor))
		{
			t_error = GetLastError();
			goto cleanup;
		}
		
		// Step 2
		if (ERROR_SUCCESS != GetSecurityInfo(
								GetCurrentProcess(),
								SE_KERNEL_OBJECT,
								DACL_SECURITY_INFORMATION,
								NULL,
								NULL,
								&t_master_dacl,
								NULL,
								&t_master_security_descriptor))
		{
			t_error = GetLastError();
			goto cleanup;
		}

		// Step 3
		EXPLICIT_ACCESS t_explicit_access;
		ZeroMemory(&t_explicit_access, sizeof(EXPLICIT_ACCESS));
		t_explicit_access.grfAccessPermissions = PROCESS_DUP_HANDLE;
		t_explicit_access.grfAccessMode = GRANT_ACCESS;
		t_explicit_access.grfInheritance = NO_INHERITANCE;
		t_explicit_access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		t_explicit_access.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
		t_explicit_access.Trustee.ptstrName = (LPSTR)t_slave_sid;
		if (ERROR_SUCCESS != SetEntriesInAcl(
								1,
								&t_explicit_access,
								t_master_dacl, &t_augmented_master_dacl))
		{
			t_error = GetLastError();
			goto cleanup;
		}

		// Step 4
		if (ERROR_SUCCESS != SetSecurityInfo(
								GetCurrentProcess(),
								SE_KERNEL_OBJECT,
								DACL_SECURITY_INFORMATION,
								NULL,
								NULL,
								t_augmented_master_dacl,
								NULL))
		{
			t_error = GetLastError();
			goto cleanup;
		}

		// It should now be possible for the slave process to open our process
		// with DUP_HANDLE privilege so we notify it to continue to do so.
		if (!PostThreadMessageA(t_slave_thread_id, WM_NULL, 0, 0))
		{
			t_error = GetLastError();
			goto cleanup;
		}

		// Once the slave has opened us, it will send us some pipe handles which
		// we use (initially) to send the actual command line and environment
		// variables across so the slave can do the appropriate CreateProcess call.
        t_msg . message = WM_QUIT;
        while(!PeekMessageW(&t_msg, (HWND)-1, WM_USER + 10, WM_USER + 10, PM_REMOVE))
            if (MsgWaitForMultipleObjects(1, &t_info . hProcess, FALSE, INFINITE, QS_POSTMESSAGE) == WAIT_OBJECT_0)
            {
                t_slave_died = TRUE;
                goto cleanup;
            }

		if (t_msg.message != WM_USER + 10)
		{
			t_slave_deaf = TRUE;
			goto cleanup;
		}

        t_input_pipe = (HANDLE)t_msg . wParam;
        t_output_pipe = (HANDLE)t_msg . lParam;

		// At this point, the next thing to do is to fetch the environment var
		// strings and then pipe them to the slave.
		t_env_strings = GetEnvironmentStringsW();
        if (t_env_strings != nil)
		{
			// The environment block is terminated with a double-null                           
			t_env_length = 0;
            while(t_env_strings[t_env_length] != '\0' || t_env_strings[t_env_length + 1] != '\0')
                t_env_length += 1;
            t_env_length += 2;
        }

		// We write the requested command line and the current env vars
		// to the slave process next.
		if (!write_blob_to_pipe(t_output_pipe, sizeof(wchar_t) * (MCStringGetLength(p_cmd_line) + 1), *t_cmd_line_wstr) ||
            !write_blob_to_pipe(t_output_pipe, sizeof(wchar_t) * t_env_length, t_env_strings))
		{
			t_slave_deaf = TRUE;
			goto cleanup;
		}

		// Now we wait again for the slave to tell us whether it succeeded
		// in opening the requested process or not.
		t_msg . message = WM_QUIT;
		while(!PeekMessageA(&t_msg, (HWND)-1, WM_USER + 10, WM_USER + 10, PM_REMOVE))
			if (MsgWaitForMultipleObjects(1, &t_info . hProcess, FALSE, INFINITE, QS_POSTMESSAGE) == WAIT_OBJECT_0)
			{
				t_slave_died = TRUE;
				break;
			}

		if (t_msg.message != WM_USER + 10)
		{
			t_slave_deaf = TRUE;
			goto cleanup;
		}

		t_process_id = (DWORD)t_msg.wParam;
		t_process_handle = (HANDLE)t_msg.lParam;

cleanup:
		// If we have an augmented dacl, make sure we reset our dcl to the
		// original.
		if (t_augmented_master_dacl != NULL)
		{
			SetSecurityInfo(GetCurrentProcess(),
							SE_KERNEL_OBJECT,
							DACL_SECURITY_INFORMATION,
							NULL,
							NULL,
							t_master_dacl,
							NULL);
			LocalFree(t_augmented_master_dacl);
		}

		if (t_slave_security_descriptor != NULL)
		{
			LocalFree(t_slave_security_descriptor);
		}
		
		if (t_master_security_descriptor != NULL)
		{
			LocalFree(t_master_security_descriptor);
		}

		// If we have env vars, free them
		if (t_env_strings != NULL)
		{
			FreeEnvironmentStringsW(t_env_strings);
		}

		// If there was an error but the slave did not die we must
		// get it to terminate.
		if (!t_slave_died)
		{
			if (t_error != ERROR_SUCCESS || t_slave_deaf)
			{
				PostThreadMessage(t_slave_thread_id, WM_NULL, 1, 0);
			}
			CloseHandle(t_slave_process);
		}

		if (t_slave_died ||
			t_error != ERROR_SUCCESS ||
			t_slave_deaf)
		{
			return FALSE;
		}

		r_child_stdin_wr = t_output_pipe;
		r_child_stdout_rd = t_input_pipe;
		r_child_process = t_process_handle;
		r_child_id = t_process_id;

		return TRUE;
	}
    
    virtual void CloseProcess(uint2 p_index)
    {
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
        return *g_mainthread_errno;
    }
    
    // MW-2007-12-14: [[ Bug 5113 ]] Slow-down on mathematical operations - make sure
    //   we access errno directly to stop us having to do a thread-local-data lookup.
    virtual void SetErrno(int p_errno)
    {
        *g_mainthread_errno = p_errno;
    }
    
    virtual void LaunchDocument(MCStringRef p_document)
	{
		MCS_do_launch(p_document);
	}

    virtual void LaunchUrl(MCStringRef p_document)
	{
		MCS_do_launch(p_document);
	}
    
    virtual void DoAlternateLanguage(MCStringRef p_script, MCStringRef p_language)
	{
		MCScriptEnvironment *t_environment;
		t_environment = MCscreen -> createscriptenvironment(p_language);
		if (t_environment == NULL)
			MCresult -> sets("alternate language not found");
		else
		{
			MCAutoStringRef t_result;
			t_environment -> Run(p_script, &t_result);
			t_environment -> Release();

			if (*t_result != nil)
			{
				MCresult -> setvalueref(*t_result);
			}
			else
				MCresult -> sets("execution error");
		}
	}

    virtual bool AlternateLanguages(MCListRef& r_list)
	{
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
        MCAutoListRef t_list;
        if (!dns_servers_from_network_params(&t_list))
            return false;
        
        if (!MCListIsEmpty(*t_list))
            return MCListCopy(*t_list, r_list);
        
        return dns_servers_from_registry(r_list);
    }
    
    virtual void ShowMessageDialog(MCStringRef p_title,
                                   MCStringRef p_message)
    {
        MCAutoStringRefAsWString t_title_w;
        if (!t_title_w . Lock(p_title))
            return;
        
        MCAutoStringRefAsWString t_message_w;
        if (!t_message_w . Lock(p_message))
            return;
        
        MessageBoxW(HWND_DESKTOP, *t_message_w, *t_title_w, MB_APPLMODAL | MB_OK);
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

	// Post our thread id back to the master.
	if (t_success)
	{
		if (!PostThreadMessageA(t_parent_thread_id, WM_USER + 10, GetCurrentThreadId(), 0))
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
	
	// Now we have the thread handle, we wait for a message from the master
	// to tell us to continue after it has allowed this process to open the
	// master process.
    MSG t_msg;
    t_msg . message = WM_QUIT;
    while(!PeekMessageW(&t_msg, (HWND)-1, WM_NULL, WM_NULL, PM_REMOVE))
        if (MsgWaitForMultipleObjects(1, &t_parent_thread, FALSE, INFINITE, QS_POSTMESSAGE) == WAIT_OBJECT_0)
        {
			t_success = false;
			break;
        }

	// If we didn't get a WM_NULL message or get one with wParam == 1
	// then we failed.
	if (t_msg.message != WM_NULL ||
		t_msg.wParam == 1)
		t_success = false;

	// Open the parent's process - this should now be allowed with PROCESS_DUP_HANDLE
	// right due to the fettling in the master process which occurred previously.
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

////////////////////////////////////////////////////////////////////////////////

bool MCS_get_browsers(MCStringRef &r_browsers)
{
    r_browsers = nullptr;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
