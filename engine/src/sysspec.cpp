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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "param.h"
#include "mcerror.h"
//#include "execpt.h"
#include "util.h"
#include "object.h"
#include "socket.h"
#include "osspec.h"

#include "globals.h"
#include "text.h"
#include "stacksecurity.h"
#include "securemode.h"
#include "securemode.h"

#include "system.h"

#include "foundation.h"

#include <signal.h>
#ifdef _WIN32
#include <float.h> // _isnan()
#endif 

////////////////////////////////////////////////////////////////////////////////

extern bool MCSystemLaunchUrl(MCStringRef p_document);
extern char *MCSystemGetVersion(void);
extern MCNameRef MCSystemGetProcessor(void);
extern char *MCSystemGetAddress(void);
extern uint32_t MCSystemPerformTextConversion(const char *string, uint32_t string_length, char *buffer, uint32_t buffer_length, uint1 from_charset, uint1 to_charset);

////////////////////////////////////////////////////////////////////////////////

MCSystemInterface *MCsystem;

#ifdef TARGET_SUBPLATFORM_ANDROID
static volatile int *s_mainthread_errno;
#else
static int *s_mainthread_errno;
#endif

////////////////////////////////////////////////////////////////////////////////

extern MCSystemInterface *MCDesktopCreateMacSystem(void);
extern MCSystemInterface *MCDesktopCreateWindowsSystem(void);
extern MCSystemInterface *MCDesktopCreateLinuxSystem(void);
extern MCSystemInterface *MCMobileCreateIPhoneSystem(void);
extern MCSystemInterface *MCMobileCreateAndroidSystem(void);

////////////////////////////////////////////////////////////////////////////////

#ifdef _SERVER
extern "C" char *__cxa_demangle(const char *, char *, size_t *, int*);

char *strndup(const char *s, size_t n)
{
	char *r;
	r = (char *)malloc(n + 1);
	strncpy(r, s, n);
	r[n] = '\0';
	return r;
}

#ifdef _LINUX_SERVER
#include <execinfo.h>
#include <dlfcn.h>

#define dl_info Dl_info

static void handle_backtrace()
{
	void *t_callstack[16];
	int t_frame_count;
	t_frame_count = backtrace(t_callstack, 16);
	
	for(int i = 1; i < t_frame_count; i++)
	{
		dl_info t_info;
		if (dladdr(t_callstack[i], &t_info) != 0 && t_info . dli_sname != NULL)
		{
			bool t_handled;
			t_handled = false;
			
			if (t_info . dli_sname[0] == '_' && t_info . dli_sname[1] == 'Z')
			{
				int t_status;
				char *t_symbol;
				t_symbol = __cxa_demangle(t_info . dli_sname, NULL, NULL, &t_status);
				if (t_status == 0)
				{
					fprintf(stderr, "  in %s @ %u\n", t_symbol, (char *)t_callstack[i] - (char *)t_info . dli_saddr);
					t_handled = true;
				}
			}
			
			if (!t_handled)
				fprintf(stderr, "  in %s @ %u\n", t_info . dli_sname, (char *)t_callstack[i] - (char *)t_info . dli_saddr);
		}
		else
			fprintf(stderr, "  in <unknown> @ %p\n", t_callstack[i]);
	}
}
#else
void handle_backtrace(void)
{
}
#endif

#ifdef _WINDOWS_SERVER
static void handle_signal(int p_signal)
{
	switch(p_signal)
	{
		case SIGTERM:
			fprintf(stderr, "livecode-server exited by request\n");
			MCquit = True;
			MCexitall = True;
			break;
		case SIGILL:
		case SIGSEGV:
		case SIGABRT:
			fprintf(stderr, "livecode-server exited due to fatal signal %d\n", p_signal);
			handle_backtrace();
			exit(-1);
			break;
		case SIGINT:
			// We received an interrupt so let the debugger (if present) handle it.
			extern void MCServerDebugInterrupt();
			MCServerDebugInterrupt();
			break;
		case SIGFPE:
			errno = EDOM;
			break;
		default:
			break;
	}
}
#else
static void handle_signal(int p_signal)
{
	switch(p_signal)
	{
		case SIGUSR1:
			MCsiguser1++;
			break;
		case SIGUSR2:
			MCsiguser2++;
			break;
		case SIGTERM:
			fprintf(stderr, "livecode-server exited by request\n");
			MCquit = True;
			MCexitall = True;
			break;
		case SIGILL:
		case SIGSEGV:
		case SIGABRT:
			fprintf(stderr, "livecode-server exited due to fatal signal %d\n", p_signal);
			handle_backtrace();
			exit(-1);
			break;
		case SIGINT:
			// We received an interrupt so let the debugger (if present) handle it.
			extern void MCServerDebugInterrupt();
			MCServerDebugInterrupt();
			break;
		case SIGHUP:
		case SIGQUIT:
			fprintf(stderr, "livecode-server exited due to termination signal %d\n", p_signal);
			exit(1);
			break;
		case SIGFPE:
			MCS_seterrno(EDOM);
			break;
		case SIGCHLD:
			break;
		case SIGALRM:
			break;
		case SIGPIPE:
			break;
		default:
			break;
	}
}
#endif
#endif

void MCS_common_init(void)
{	
	MCsystem -> Initialize();    
    MCsystem -> SetErrno(errno);
	
	MCinfinity = HUGE_VAL;

	// MW-2013-10-08: [[ Bug 11259 ]] We use our own tables on linux since
	//   we use a fixed locale which isn't available on all systems.
#if !defined(_LINUX_SERVER) && !defined(_LINUX_DESKTOP) && !defined(_WINDOWS_DESKTOP) && !defined(_WINDOWS_SERVER)
	MCuppercasingtable = new uint1[256];
	for(uint4 i = 0; i < 256; ++i)
		MCuppercasingtable[i] = (uint1)toupper((uint1)i);
	
	MClowercasingtable = new uint1[256];
	for(uint4 i = 0; i < 256; ++i)
		MClowercasingtable[i] = (uint1)tolower((uint1)i);
#endif
	
	MCStackSecurityInit();
}

void MCS_init(void)
{
#if defined(_WINDOWS_SERVER)
	MCsystem = MCDesktopCreateWindowsSystem();
#elif defined(_MAC_SERVER)
	MCsystem = MCDesktopCreateMacSystem();
#elif defined(_LINUX_SERVER) /*|| defined(_DARWIN_SERVER)*/
	MCsystem = MCDesktopCreateLinuxSystem();
#elif defined(_MAC_DESKTOP)
    MCsystem = MCDesktopCreateMacSystem();
#elif defined(_WINDOWS_DESKTOP)
    MCsystem = MCDesktopCreateWindowsSystem();
#elif defined(_LINUX_DESKTOP)
    MCsystem = MCDesktopCreateLinuxSystem();
#elif defined (_IOS_MOBILE)
    MCsystem = MCMobileCreateIPhoneSystem();
#elif defined (_ANDROID_MOBILE)
    MCsystem = MCMobileCreateAndroidSystem();
#else
#error Unknown server platform.
#endif

#ifdef _SERVER
#ifndef _WINDOWS_SERVER
	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);
	signal(SIGBUS, handle_signal);
	signal(SIGHUP, handle_signal);
	signal(SIGQUIT, handle_signal);
	signal(SIGCHLD, handle_signal);
	signal(SIGALRM, handle_signal);
	signal(SIGPIPE, handle_signal);
#endif
	
	signal(SIGTERM, handle_signal);
	signal(SIGILL, handle_signal);
	signal(SIGSEGV, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGABRT, handle_signal);
	signal(SIGFPE, handle_signal);
    
#endif // _SERVER

	MCS_common_init();
}

void MCS_shutdown(void)
{
	MCsystem -> Finalize();
}

////////////////////////////////////////////////////////////////////////////////

real8 MCS_time(void)
{
	return MCsystem -> GetCurrentTime();
}

void MCS_setenv(MCStringRef p_name_string, MCStringRef p_value_string)
{
	MCsystem -> SetEnv(p_name_string, p_value_string);
}

void MCS_unsetenv(MCStringRef p_name_string)
{
	MCsystem -> SetEnv(p_name_string, NULL);
}

bool MCS_getenv(MCStringRef p_name_string, MCStringRef& r_result)
{
    return MCsystem -> GetEnv(p_name_string, r_result);
}

real8 MCS_getfreediskspace(void)
{
    return MCsystem -> GetFreeDiskSpace();
}

void MCS_launch_document(MCStringRef p_document)
{
    MCsystem -> LaunchDocument(p_document);
}

void MCS_launch_url(MCStringRef p_document_string)
{
    MCsystem -> LaunchUrl(p_document_string);
}

/* WRAPPER */
Boolean MCS_getspecialfolder(MCNameRef p_type, MCStringRef& r_path)
{
    MCAutoStringRef t_path;
    if (!MCsystem -> GetStandardFolder(p_type, &t_path))
        return False;
    
    return MCS_pathfromnative(*t_path, r_path);
}

void MCS_doalternatelanguage(MCStringRef p_script, MCStringRef p_language)
{
    MCsystem -> DoAlternateLanguage(p_script, p_language);
}

bool MCS_alternatelanguages(MCListRef& r_list)
{
    return MCsystem -> AlternateLanguages(r_list);
}

void MCS_nativetoutf16(const char *p_native, uint4 p_native_length, unsigned short *&r_utf16, uint4& r_utf16_length)
{
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringCreateWithBytes((const byte_t *)p_native, p_native_length, kMCStringEncodingNative, false, &t_string);
    byte_t *t_bytes;
    uindex_t t_len;
    /* UNCHECKED */ MCStringConvertToBytes(*t_string, kMCStringEncodingUTF16, false, t_bytes, t_len);
    r_utf16 = (unsigned short *)t_bytes;
    r_utf16_length = (uint4) t_len;
}

void MCS_utf16tonative(const unsigned short *p_utf16, uint4 p_utf16_length, char *&r_native, uint4& r_native_length)
{
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringCreateWithBytes((const byte_t *)p_utf16, p_utf16_length, kMCStringEncodingUTF16, false, &t_string);
    byte_t *t_bytes;
    uindex_t t_len;
    /* UNCHECKED */ MCStringConvertToBytes(*t_string, kMCStringEncodingNative, false, t_bytes, t_len);
    r_native = (char *)t_bytes;
    r_native_length = (uint4)t_len;
}

void MCS_nativetoutf8(const char *p_native, uint4 p_native_length, char *&r_utf8, uint4& r_utf8_length)
{
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringCreateWithBytes((const byte_t *)p_native, p_native_length, kMCStringEncodingNative, false, &t_string);
    byte_t *t_bytes;
    uindex_t t_len;
    /* UNCHECKED */ MCStringConvertToBytes(*t_string, kMCStringEncodingUTF8, false, t_bytes, t_len);
    r_utf8 = (char *)t_bytes;
    r_utf8_length = (uint4) t_len;
}

void MCS_utf8tonative(const char *p_utf8, uint4 p_utf8_length, char *&r_native, uint4& r_native_length)
{
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringCreateWithBytes((const byte_t *)p_utf8, p_utf8_length, kMCStringEncodingUTF8, false, &t_string);
    byte_t *t_bytes;
    uindex_t t_len;
    /* UNCHECKED */ MCStringConvertToBytes(*t_string, kMCStringEncodingNative, false, t_bytes, t_len);
    r_native = (char *)t_bytes;
    r_native_length = (uint4)t_len;
}

void MCS_seterrno(int value)
{
//	*s_mainthread_errno = value;
    MCsystem -> SetErrno(value);
}

int MCS_geterrno(void)
{
//	return *s_mainthread_errno;
    return MCsystem -> GetErrno();
}

void MCS_sleep(real8 p_delay)
{
	MCsystem -> Sleep(p_delay);
}

void MCS_alarm(real8 p_delay)
{
	MCsystem -> Alarm(p_delay);
}

uint32_t MCS_getpid(void)
{
	return MCsystem -> GetProcessId();
}

////////////////////////////////////////////////////////////////////////////////

// As there is no clear way (at present) to see how to indirect these through
// the system interface we just implement them as is in srvwindows.cpp, providing
// the default 'dummy' functions here.

// Fixed by using MCServiceInterface
bool MCS_query_registry(MCStringRef p_key, MCValueRef& r_value, MCStringRef& r_type, MCStringRef& r_error)
{
    MCWindowsSystemServiceInterface *t_service;
    t_service = (MCWindowsSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeWindowsSystem);
    
    if (t_service != nil)
        return t_service -> QueryRegistry(p_key, r_value, r_type, r_error);
    
	return MCStringCreateWithCString("not supported", r_error);
}

#ifdef LEGACY_EXEC
/* LEGACY */
void MCS_query_registry(MCExecPoint &dest)
{
	MCAutoStringRef t_key;
	/* UNCHECKED */ dest.copyasstringref(&t_key);
	MCAutoStringRef t_type, t_error;
	MCAutoValueRef t_value;
	/* UNCHECKED */ MCS_query_registry(*t_key, &t_value, &t_type, &t_error);
	if (*t_error != nil)
	{
		dest.clear();
		/* UNCHECKED */ MCresult->setvalueref(*t_error);
	}
	else
	{
		dest.setvalueref(*t_value);
		MCresult->clear();
	}
}
#endif

bool MCS_set_registry(MCStringRef p_key, MCValueRef p_value, MCSRegistryValueType p_type, MCStringRef& r_error)
{
    MCWindowsSystemServiceInterface *t_service;
    t_service = (MCWindowsSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeWindowsSystem);
    
if (t_service != nil)
        return t_service -> SetRegistry(p_key, p_value, p_type, r_error);
    
	return MCStringCreateWithCString("not supported", r_error);
}

bool MCS_delete_registry(MCStringRef p_key, MCStringRef& r_error)
{
    MCWindowsSystemServiceInterface *t_service;
    t_service = (MCWindowsSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeWindowsSystem);
    
    if (t_service != nil)
        return t_service -> DeleteRegistry(p_key, r_error);
		
	return MCStringCreateWithCString("not supported", r_error);
}

bool MCS_list_registry(MCStringRef p_path, MCListRef& r_list, MCStringRef& r_error)
{
    MCWindowsSystemServiceInterface *t_service;
    t_service = (MCWindowsSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeWindowsSystem);
    
    if (t_service != nil)
    {
		// SN-2014-12-15: [[ Bug 14219 ]] The path to keys must have 
		//  backslashes, not slashes
        //MCAutoStringRef t_native_path;
        //if (!MCS_pathtonative(p_path, &t_native_path))
        //    return false;
        
        return t_service -> ListRegistry(p_path, r_list, r_error);        
    }
    
	return MCStringCreateWithCString("not supported", r_error);
}

#ifndef __WINDOWS__

// For Win32, this function is implemented in dskw32.cpp
MCSRegistryValueType MCS_registry_type_from_string(MCStringRef)
{
	return kMCSRegistryValueTypeNone;
}

#endif

void MCS_reset_time(void)
{
    MCWindowsSystemServiceInterface *t_service;
    t_service = (MCWindowsSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeWindowsSystem);
    
    if (t_service != nil)
    {
		t_service -> ResetTime();
	}

//	MCresult -> sets("not supported");
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_getsystemversion(MCStringRef& r_string)
{
	return MCsystem->GetVersion(r_string);
}

MCNameRef MCS_getprocessor(void)
{
	return MCsystem -> GetProcessor();
}

bool MCS_getmachine(MCStringRef& r_string)
{
	return MCsystem->GetMachine(r_string);
}

bool MCS_getaddress(MCStringRef& r_address)
{
	return MCsystem -> GetAddress(r_address);
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCS_mkdir(MCStringRef p_path)
{
    MCAutoStringRef t_native_path;
	MCAutoStringRef t_resolved_path;
    
    if (!MCS_resolvepath(p_path, &t_resolved_path))
        return False;
    
    if (!MCS_pathtonative(*t_resolved_path, &t_native_path))
        return False;
    
	if (MCsystem -> CreateFolder(*t_native_path) == False)
        return False;

	return True;
}

Boolean MCS_rmdir(MCStringRef p_path)
{
	MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path;
    
	if (!MCS_resolvepath(p_path, &t_resolved_path))
        return False;
    
    if (!MCS_pathtonative(*t_resolved_path, &t_native_path))
        return False;
	
	return MCsystem -> DeleteFolder(*t_native_path);
}

Boolean MCS_rename(MCStringRef p_old_name, MCStringRef p_new_name)
{
	MCAutoStringRef t_old_resolved_path, t_new_resolved_path;
    MCAutoStringRef t_old_native_path, t_new_native_path;
    
	if (!MCS_resolvepath(p_old_name, &t_old_resolved_path) || !MCS_resolvepath(p_new_name, &t_new_resolved_path))
        return False;
    
    if (!MCS_pathtonative(*t_old_resolved_path, &t_old_native_path) || !MCS_pathtonative(*t_new_resolved_path, &t_new_native_path))
        return False;
	
	return MCsystem -> RenameFileOrFolder(*t_old_native_path, *t_new_native_path);
}

/* LEGACY */
bool MCS_unlink(const char *p_path)
{
	MCAutoStringRef t_resolved_path;
	if (!MCStringCreateWithCString(p_path, &t_resolved_path))
        return false;
	
	return MCS_unlink(*t_resolved_path) == True;
}

Boolean MCS_unlink(MCStringRef p_path)
{
	MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path;
    
	if (!MCS_resolvepath(p_path, &t_resolved_path))
        return False;
    
    if (!MCS_pathtonative(*t_resolved_path, &t_native_path))
        return False;
	
	return MCsystem -> DeleteFile(*t_native_path);
}

Boolean MCS_backup(MCStringRef p_old_name, MCStringRef p_new_name)
{
    MCAutoStringRef t_old_resolved, t_new_resolved;
    MCAutoStringRef t_old_native, t_new_native;
    
    if (!MCS_resolvepath(p_old_name, &t_old_resolved) || !MCS_resolvepath(p_new_name, &t_new_resolved))
        return False;
    
    if (!MCS_pathtonative(*t_old_resolved, &t_old_native) || !MCS_pathtonative(*t_new_resolved, &t_new_native))
        return False;
        
	return MCsystem -> BackupFile(*t_old_native, *t_new_native);
}

Boolean MCS_unbackup(MCStringRef p_old_name, MCStringRef p_new_name)
{
    MCAutoStringRef t_old_resolved, t_new_resolved;
    MCAutoStringRef t_old_native, t_new_native;
    
    if (!MCS_resolvepath(p_old_name, &t_old_resolved) || !MCS_resolvepath(p_new_name, &t_new_resolved))
        return False;
    
    if (!MCS_pathtonative(*t_old_resolved, &t_old_native) || !MCS_pathtonative(*t_new_resolved, &t_new_native))
        return False;
    
	return MCsystem -> UnbackupFile(*t_old_resolved, *t_new_resolved);
}

Boolean MCS_createalias(MCStringRef p_target, MCStringRef p_alias)
{
    MCAutoStringRef t_target_resolved, t_alias_resolved;
    MCAutoStringRef t_target_native, t_alias_native;
    
    if (!MCS_resolvepath(p_target, &t_target_resolved) || !MCS_resolvepath(p_alias, &t_alias_resolved))
        return False;
    
    if (!MCS_pathtonative(*t_target_resolved, &t_target_native) || !MCS_pathtonative(*t_alias_resolved, &t_alias_native))
        return False;
    
	return MCsystem -> CreateAlias(*t_target_resolved, *t_alias_resolved);
}

Boolean MCS_resolvealias(MCStringRef p_path, MCStringRef& r_resolved)
{
    MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path;
    
    if (!MCS_resolvepath(p_path, &t_resolved_path))
        return False;
    
    if (!MCS_pathtonative(*t_resolved_path, &t_native_path))
        return False;
    
	return MCsystem -> ResolveAlias(*t_resolved_path, r_resolved);
}

bool MCS_setresource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_id, MCStringRef p_name,
							MCStringRef p_flags, MCStringRef p_value, MCStringRef& r_error)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    if (t_service != nil)
    {
        MCAutoStringRef t_native_path;
        
        if (!MCS_pathtonative(p_source, &t_native_path))
            return false;
        
        return t_service -> SetResource(*t_native_path, p_type, p_id, p_name, p_flags, p_value, r_error);
    }
    
	return MCStringCreateWithCString("not supported", r_error);
}

bool MCS_getresource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_value, MCStringRef& r_error)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    if (t_service != nil)
    {
        MCAutoStringRef t_native_path;
        
        if (!MCS_pathtonative(p_source, &t_native_path))
            return false;
        
        return t_service -> GetResource(*t_native_path, p_type, p_name, r_value, r_error);
    }
    
	return MCStringCreateWithCString("not supported", r_error);
}

bool MCS_copyresource(MCStringRef p_source, MCStringRef p_dest, MCStringRef p_type,
					  MCStringRef p_name, MCStringRef p_newid, MCStringRef& r_error)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    if (t_service != nil)
    {
        MCAutoStringRef t_native_source, t_native_dest;
        
        if (!MCS_pathtonative(p_source, &t_native_source) || !MCS_pathtonative(p_dest, &t_native_dest))
            return false;
        
        return t_service -> CopyResource(*t_native_source, *t_native_dest, p_type, p_name, p_newid, r_error);
    }
    
	return MCStringCreateWithCString("not supported", r_error);
}

void MCS_copyresourcefork(MCStringRef p_source, MCStringRef p_dst)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    if (t_service != nil)
    {
        MCAutoStringRef t_native_source, t_native_dest;
        
        if (!MCS_pathtonative(p_source, &t_native_source) || !MCS_pathtonative(p_dst, &t_native_dest))
            return;
        
        t_service -> CopyResourceFork(*t_native_source, *t_native_dest);
		
		return;
    }
    
//    MCresult -> sets("not supported");
}

bool MCS_deleteresource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_error)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    if (t_service != nil)
    {
        MCAutoStringRef t_native_source;
        
        if (!MCS_pathtonative(p_source, &t_native_source))
            return false;
        
        return t_service -> DeleteResource(*t_native_source, p_type, p_name, r_error);
    }
    
	return MCStringCreateWithCString("not supported", r_error);
}

bool MCS_getresources(MCStringRef p_source, MCStringRef p_type, MCListRef& r_list, MCStringRef& r_error)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    if (t_service != nil)
    {
        MCAutoStringRef t_native_path;
        
        if (!MCS_pathtonative(p_source, &t_native_path))
            return false;
        
        return t_service -> GetResources(*t_native_path, p_type, r_list, r_error);
    }
    
	return MCStringCreateWithCString("not supported", r_error);
}

void MCS_loadresfile(MCStringRef p_filename, MCStringRef& r_data)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    if (t_service != nil)
    {
        MCAutoStringRef t_resolved_path;
        MCAutoStringRef t_native_path;
        
        if (!MCS_resolvepath(p_filename, &t_resolved_path) || !MCS_pathtonative(*t_resolved_path, &t_native_path))
            return;
        
        t_service -> LoadResFile(*t_native_path, r_data);
		
		return;
    }
    
    MCresult -> sets("not supported");
}

void MCS_saveresfile(MCStringRef p_path, MCDataRef p_data)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    if (t_service != nil)
    {
        MCAutoStringRef t_resolved_path;
        MCAutoStringRef t_native_path;
        
        if (!MCS_resolvepath(p_path, &t_resolved_path) || !MCS_pathtonative(*t_resolved_path, &t_native_path))
            return;
        
        t_service -> SaveResFile(*t_native_path, p_data);
		
		return;
    }
    
    MCresult -> sets("not supported");
}

bool MCS_longfilepath(MCStringRef p_path, MCStringRef& r_long_path)
{
    MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path, t_native_long_path;
    
    if (!(MCS_resolvepath(p_path, &t_resolved_path) &&
          MCS_pathtonative(*t_resolved_path, &t_native_path)))
        return false;
    
    if (!MCsystem->LongFilePath(*t_native_path, &t_native_long_path))
        return false;
    
    return MCS_pathfromnative(*t_native_long_path, r_long_path);
}

bool MCS_shortfilepath(MCStringRef p_path, MCStringRef& r_short_path)
{
    MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path, t_native_long_path;
    
    if (!(MCS_resolvepath(p_path, &t_resolved_path) &&
          MCS_pathtonative(*t_resolved_path, &t_native_path)))
        return false;
    
    if (!MCsystem->ShortFilePath(*t_native_path, &t_native_long_path))
        return false;
    
    return MCS_pathfromnative(*t_native_long_path, r_short_path);
}

Boolean MCS_setcurdir(MCStringRef p_path)
{
	MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path;
    
	if (!MCS_resolvepath(p_path, &t_resolved_path))
        return False;
    
    if (!MCS_pathtonative(*t_resolved_path, &t_native_path))
        return False;
    
    return MCsystem -> SetCurrentFolder(*t_native_path);
}

void MCS_getcurdir(MCStringRef& r_path)
{
	MCAutoStringRef t_current_native;
    if (!MCsystem->GetCurrentFolder(&t_current_native))
    {
        r_path = MCValueRetain(kMCEmptyString);
        return;
    }
    
    if (!MCsystem->PathFromNative(*t_current_native, r_path))
        r_path = MCValueRetain(kMCEmptyString);
}

struct MCS_getentries_state
{
	bool files;
	bool details;
	MCListRef list;
};

bool MCFiltersUrlEncode(MCStringRef p_source, bool p_use_utf8, MCStringRef& r_result);

static bool MCS_getentries_callback(void *p_context, const MCSystemFolderEntry *p_entry)
{
	MCS_getentries_state *t_state;
	t_state = static_cast<MCS_getentries_state *>(p_context);
	
	if (!t_state -> files != p_entry -> is_folder)
		return true;
    
#if defined(_MACOSX)
    // Mac doesn't list the '..' folder
    if (p_entry -> is_folder && MCListIsEmpty(t_state -> list)
            && !MCStringIsEqualToCString(p_entry -> name, "..", kMCStringOptionCompareExact))
        MCListAppendCString(t_state -> list, "..");
#endif
	
	if (t_state -> details)
	{
        MCAutoStringRef t_details;
#ifdef _WIN32
		/* UNCHECKED */ MCStringFormat(&t_details,
                                       "%@,%I64d,,%ld,%ld,%ld,,,,%03o,",
                                       p_entry -> name,
                                       p_entry -> data_size,
                                       p_entry -> creation_time,
                                       p_entry -> modification_time,
                                       p_entry -> access_time,
                                       p_entry -> permissions);
#elif defined(_MACOSX)
		/* UNCHECKED */ MCStringFormat(&t_details,
                                       "%@,%lld,%lld,%u,%u,%u,%u,%d,%d,%03o,%.8s",
                                       p_entry -> name,
                                       p_entry -> data_size,
                                       p_entry -> resource_size,
                                       p_entry -> creation_time,
                                       p_entry -> modification_time,
                                       p_entry -> access_time,
                                       p_entry -> backup_time,
                                       p_entry -> user_id,
                                       p_entry -> group_id,
                                       p_entry -> permissions,
                                       p_entry -> file_type);
#else
		/* UNCHECKED */ MCStringFormat(&t_details,
                                       "%@,%lld,,,%u,%u,,%d,%d,%03o,",
                                       p_entry -> name,
                                       p_entry -> data_size,
                                       p_entry -> modification_time,
                                       p_entry -> access_time,
                                       p_entry -> user_id,
                                       p_entry -> group_id,
                                       p_entry -> permissions);
        
#endif

		/* UNCHECKED */ MCListAppend(t_state->list, *t_details);
	}
	else
    /* UNCHECKED */ MCListAppendFormat(t_state->list, "%@", p_entry -> name);
	
	return true;
}

bool MCS_getentries(bool p_files, bool p_detailed, MCListRef& r_list)
{    
	MCAutoListRef t_list;
    
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	MCS_getentries_state t_state;	
	t_state.files = p_files;
	t_state.details = p_detailed;
	t_state.list = *t_list;
	
	if (!MCsystem -> ListFolderEntries(MCS_getentries_callback, (void*)&t_state))
		return false;
    
	if (!MCListCopy(*t_list, r_list))
        return false;
    
    return true;
}

Boolean MCS_chmod(MCStringRef p_path, uint2 p_mask)
{
    MCAutoStringRef t_resolved;
    MCAutoStringRef t_native;
    
    if (!(MCS_resolvepath(p_path, &t_resolved) &&
          MCS_pathtonative(*t_resolved, &t_native)))
        return False;
    
	return MCsystem -> ChangePermissions(*t_native, p_mask);
}

int4 MCS_getumask(void)
{
	int4 t_old_mask;
	t_old_mask = MCsystem -> UMask(0);
	MCsystem -> UMask(t_old_mask);
	return t_old_mask;
}

void MCS_setumask(uint2 p_mask)
{
    MCsystem -> UMask(p_mask);
}

/* WRAPPER */
Boolean MCS_exists(MCStringRef p_path, bool p_is_file)
{
	// Shortcut: this is necessary because MCS_resolvepath turns an empty path
    // into the path to the current directory, which is really not wanted here.
    if (MCStringIsEmpty(p_path))
        return False;
    
    MCAutoStringRef t_resolved;
    MCAutoStringRef t_native;
	if (!(MCS_resolvepath(p_path, &t_resolved) && MCS_pathtonative(*t_resolved, &t_native)))
		return False;
    
    Boolean t_success;
	if (p_is_file)
		t_success = MCsystem->FileExists(*t_native);
	else
		t_success = MCsystem->FolderExists(*t_native);
    
    if (!t_success)
        return False;
    
    return True;
}

Boolean MCS_noperm(MCStringRef p_path)
{
    MCAutoStringRef t_resolved;
    MCAutoStringRef t_native;
    if (!(MCS_resolvepath(p_path, &t_resolved) && MCS_pathtonative(*t_resolved, &t_native)))
        return False;
    
	return MCsystem -> FileNotAccessible(*t_native);
}

Boolean MCS_getdrives(MCStringRef& r_drives)
{
    return MCsystem -> GetDrives(r_drives);
}

Boolean MCS_getdevices(MCStringRef& r_devices)
{
    return MCsystem -> GetDevices(r_devices);
}

bool MCS_resolvepath(MCStringRef p_path, MCStringRef& r_resolved)
{
    MCAutoStringRef t_native;
    MCAutoStringRef t_native_resolved;
    MCAutoStringRef t_std_resolved;
    
    if (!MCS_pathtonative(p_path, &t_native))
        return false;
    
	if (!MCsystem -> ResolvePath(*t_native, &t_native_resolved))
        return false;
    
    if (!MCS_pathfromnative(*t_native_resolved, &t_std_resolved))
        return false;
    
    MCU_fix_path(*t_std_resolved, r_resolved);
    return true;
}

bool MCS_pathtonative(MCStringRef p_livecode_path, MCStringRef& r_native_path)
{
    return MCsystem -> PathToNative(p_livecode_path, r_native_path);
}

bool MCS_pathfromnative(MCStringRef p_native_path, MCStringRef& r_livecode_path)
{
    return MCsystem -> PathFromNative(p_native_path, r_livecode_path);
}

////////////////////////////////////////////////////////////////////////////////

IO_handle MCS_fakeopen(const void *p_data, uindex_t p_size)
{
	MCMemoryFileHandle *t_handle;
    t_handle = new MCMemoryFileHandle(p_data, p_size);
	return t_handle;
}

IO_handle MCS_fakeopenwrite(void)
{
	MCMemoryFileHandle *t_handle;
	t_handle = new MCMemoryFileHandle();
	return t_handle;
}

IO_stat MCS_closetakingbuffer(IO_handle& p_stream, void*& r_buffer, size_t& r_length)
{
    bool t_success;

    t_success = p_stream -> TakeBuffer(r_buffer, r_length);
	
	MCS_close(p_stream);
	
    if (!t_success)
        return IO_ERROR;
    
	return IO_NORMAL;
}

IO_stat MCS_writeat(const void *p_buffer, uint32_t p_size, uint32_t p_pos, IO_handle p_stream)
{
    uint64_t t_old_pos;
    bool t_success;
    
    t_old_pos = p_stream -> Tell();
    
    t_success = p_stream -> Seek(p_pos, kMCSystemFileSeekSet);
    
    if (t_success)
        t_success = p_stream -> Write(p_buffer, p_size);
    
    if (t_success)
        t_success = p_stream -> Seek(t_old_pos, kMCSystemFileSeekSet);
    
    if (!t_success)
        return IO_ERROR;
    
    return IO_NORMAL;
}

bool MCS_tmpnam(MCStringRef& r_path)
{
	return MCsystem->GetTemporaryFileName(r_path);
}

////////////////////////////////////////////////////////////////////////////////

IO_handle MCS_fakeopencustom(MCFakeOpenCallbacks *p_callbacks, void *p_state)
{
	MCSystemFileHandle *t_handle;
	t_handle = new MCCustomFileHandle(p_callbacks, p_state);
	return t_handle;
}

////////////////////////////////////////////////////////////////////////////////

/* LEGACY */
#if 0
IO_handle MCS_open(const char *p_path, const char *p_mode, Boolean p_map, Boolean p_driver, uint4 p_offset)
{
    char *t_resolved_path;
    t_resolved_path = MCS_resolvepath(p_path);
    
    uint32_t t_mode;
    if (strequal(p_mode, IO_READ_MODE))
        t_mode = kMCOpenFileModeRead;
    else if (strequal(p_mode, IO_WRITE_MODE))
        t_mode = kMCOpenFileModeWrite;
    else if (strequal(p_mode, IO_UPDATE_MODE))
        t_mode = kMCOpenFileModeUpdate;
    else if (strequal(p_mode, IO_APPEND_MODE))
        t_mode = kMCOpenFileModeAppend;
    
    MCSystemFileHandle *t_handle;
    if (!p_driver)
        t_handle = MCsystem -> OpenFile(t_resolved_path, t_mode, p_map && MCmmap);
    else
        t_handle = MCsystem -> OpenDevice(t_resolved_path, t_mode, MCserialcontrolsettings);
    
    // MW-2011-06-12: Fix memory leak - make sure we delete the resolved path.
    delete t_resolved_path;
    
    if (t_handle == NULL)
        return NULL;
    
    if (p_offset != 0)
        t_handle -> Seek(p_offset, kMCSystemFileSeekSet);
    
    return new IO_header(t_handle, 0);;
}
#endif 

IO_handle MCS_deploy_open(MCStringRef path, intenum_t p_mode)
{
    return MCsystem -> DeployOpen(path, p_mode);
}

IO_handle MCS_open(MCStringRef path, intenum_t p_mode, Boolean p_map, Boolean p_driver, uint32_t p_offset)
{
	MCAutoStringRef t_resolved;
    MCAutoStringRef t_native;
    
	if (!(MCS_resolvepath(path, &t_resolved) && MCS_pathtonative(*t_resolved, &t_native)))
        return NULL;
	
	IO_handle t_handle;
	if (!p_driver)
    {
		t_handle = MCsystem -> OpenFile(*t_native, p_mode, p_map && MCmmap);
    }
	else
	{
        t_handle = MCsystem -> OpenDevice(*t_native, p_mode);
	}
	
	// MW-2011-06-12: Fix memory leak - make sure we delete the resolved path.
    //	delete t_resolved_path;

	if (t_handle == NULL)
		return NULL;
#ifdef OLD_IO_HANDLE
    if (p_mode == kMCOpenFileModeAppend)
        t_handle -> flags |= IO_SEEKED;
#endif

    if (p_mode == kMCOpenFileModeAppend)
        t_handle -> Seek(0, kMCSystemFileSeekEnd);
    else if (p_offset > 0)
        t_handle -> Seek(p_offset, kMCSystemFileSeekSet);
	
	return t_handle;
}

void MCS_close(IO_handle &x_stream)
{
	x_stream -> Close();
}

// Inspects the BOM of a text file to retrieve its encoding
MCFileEncodingType MCS_resolve_BOM(IO_handle x_stream)
{
    uint1 t_BOM[4];
    int64_t t_size;
    uint32_t t_size_read;
    uint32_t t_position;
    MCFileEncodingType t_encoding;
    t_encoding = kMCFileEncodingNative;

    t_size = x_stream -> GetFileSize();

    t_position = x_stream -> Tell();
    x_stream -> Seek(0, 1);    
    
    // Reading to find a UTF-32 BOM
    if (t_size > 3)
    {
        if (x_stream -> Read(t_BOM, 4, t_size_read))
        {
            if (t_BOM[0] == 0xFF
                    && t_BOM[1] == 0xFE
                    && t_BOM[2] == 0x0
                    && t_BOM[3] == 0x0)
                t_encoding = kMCFileEncodingUTF32LE;
            else if (t_BOM[0] == 0x0
                     && t_BOM[1] == 0x0
                     && t_BOM[2] == 0xFE
                     && t_BOM[3] == 0xFF)
                t_encoding = kMCFileEncodingUTF32BE;
            else
                x_stream -> Seek(0,1);
        }
    }

    if (t_encoding == kMCFileEncodingNative && t_size > 1)
    {
        if (x_stream -> Read(t_BOM, 2, t_size_read))
        {
            if (t_BOM[0] == 0xFE && t_BOM[1] == 0xFF)
                t_encoding = kMCFileEncodingUTF16BE;
            else if (t_BOM[0] == 0xFF && t_BOM[1] == 0xFE)
                t_encoding = kMCFileEncodingUTF16LE;
            else
                x_stream -> Seek(0, 1);
        }
    }

    if (t_encoding == kMCFileEncodingNative && t_size > 2)
    {
        if (x_stream -> Read(t_BOM, 3, t_size_read)
                && t_size_read == 3
                && t_BOM[0] == 0xEF
                && t_BOM[1] == 0xBB
                && t_BOM[2] == 0xBF)
            t_encoding = kMCFileEncodingUTF8;
    }

    x_stream -> Seek(t_position, 1);
    return t_encoding;
}

IO_stat MCS_putback(char p_char, IO_handle p_stream)
{
	if (!p_stream -> PutBack(p_char))
		return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_seek_cur(IO_handle p_stream, int64_t p_offset)
{
    if (!p_stream -> Seek(p_offset, kMCSystemFileSeekCurrent))
        return IO_ERROR;
    return IO_NORMAL;
}

IO_stat MCS_seek_set(IO_handle p_stream, int64_t p_offset)
{
    if (!p_stream -> Seek(p_offset, kMCSystemFileSeekSet))
        return IO_ERROR;
    return IO_NORMAL;
}

IO_stat MCS_seek_end(IO_handle p_stream, int64_t p_offset)
{
    if (!p_stream -> Seek(p_offset, kMCSystemFileSeekEnd))
        return IO_ERROR;
    return IO_NORMAL;
}

bool MCS_loadtextfile(MCStringRef p_filename, MCStringRef& r_text)
{
	if (!MCSecureModeCanAccessDisk())
	{
		MCresult->sets("can't open file");
		return false;
	}
    
	MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path;
	
	if (!(MCS_resolvepath(p_filename, &t_resolved_path) && MCS_pathtonative(*t_resolved_path, &t_native_path)))
        return false;
	
	IO_handle t_file;
    t_file = MCsystem -> OpenFile(*t_native_path, (intenum_t)kMCOpenFileModeRead, false);
	
	if (t_file == NULL)
	{
		MCresult -> sets("can't open file");
		return false;
	}
	
    bool t_success;
	uint32_t t_size;
	t_size = (uint32_t)t_file -> GetFileSize();
	
	MCAutoNativeCharArray t_buffer;
	t_success = t_buffer . New(t_size);
	
	if (t_success)
        t_success = MCS_readfixed(t_buffer.Chars(), t_size, t_file) == IO_NORMAL;

    if (t_success)
    {
        MCFileEncodingType t_file_encoding;
        MCAutoStringRef t_text;
        uindex_t t_bom_size;

        t_bom_size = 0;
        t_buffer . Shrink(t_size);

        t_file_encoding = MCS_resolve_BOM(t_file);
        
        if (t_file_encoding == kMCFileEncodingUTF16
                || t_file_encoding == kMCFileEncodingUTF16BE
                || t_file_encoding == kMCFileEncodingUTF16LE)
            t_bom_size = 2;
        else if (t_file_encoding == kMCFileEncodingUTF8)
            t_bom_size = 3;

        if (t_success)
            t_success =  MCStringCreateWithBytes((byte_t*)t_buffer.Chars() + t_bom_size, t_buffer.CharCount() - t_bom_size, MCS_file_to_string_encoding(t_file_encoding), false, &t_text);
        
        if (t_success)
            t_success = MCStringConvertLineEndingsToLiveCode(*t_text, r_text);
        
        MCresult -> clear();
    }

	t_file -> Close();
    
	if (!t_success)
	{
		MCresult -> sets("error reading file");
        return false;
	}
    
    return true;
}

bool MCS_loadbinaryfile(MCStringRef p_filename, MCDataRef& r_data)
{
	if (!MCSecureModeCanAccessDisk())
	{
		MCresult->sets("can't open file");
		return false;
	}
    
	MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path;
	
	if (!(MCS_resolvepath(p_filename, &t_resolved_path) && MCS_pathtonative(*t_resolved_path, &t_native_path)))
        return false;
	
	IO_handle t_file;
    t_file = MCsystem -> OpenFile(*t_native_path, (intenum_t)kMCOpenFileModeRead, false);
	
	if (t_file == NULL)
	{
		MCresult -> sets("can't open file");
		return false;
	}
	
    bool t_success;
	uint32_t t_size;
	t_size = (uint32_t)t_file -> GetFileSize();
	
	MCAutoByteArray t_buffer;
	t_success = t_buffer . New(t_size);
	
	if (t_success)
        t_success = MCS_readfixed(t_buffer.Bytes(), t_size, t_file) == IO_NORMAL;
    
    if (t_success)
    {
		t_buffer . Shrink(t_size);
        t_success = t_buffer.CreateData(r_data);
    }
    
    if (t_success)
    {
        MCresult -> clear();
    }
    
	t_file -> Close();
    
	if (!t_success)
	{
		MCresult -> sets("error reading file");
        return false;
	}
    
    return true;	
}

bool MCS_savetextfile(MCStringRef p_filename, MCStringRef p_string)
{
    // AL-2014-10-29: Reinstate secure mode check when trying to save file
    if (!MCSecureModeCanAccessDisk())
	{
		MCresult->sets("can't open file");
		return false;
	}
    
	MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path;
	
	if (!(MCS_resolvepath(p_filename, &t_resolved_path) && MCS_pathtonative(*t_resolved_path, &t_native_path)))
        return false;
	
    // MW-2014-10-24: [[ Bug 13797 ]] Don't create executable file.
	IO_handle t_file;
    t_file = MCsystem -> OpenFile(*t_native_path, (intenum_t)kMCOpenFileModeWrite, false);
	
	if (t_file == NULL)
	{
		MCresult -> sets("can't open file");
		return false;
	}
    
    bool t_success;
    t_success = true;
    
    // convert the line endings before writing
    MCAutoStringRef t_converted;
    if (t_success)
        t_success = MCStringConvertLineEndingsFromLiveCode(p_string, &t_converted);
    
    // Need to convert the string to a binary string
    MCAutoDataRef t_data;
    /* UNCHECKED */ MCStringEncode(*t_converted, kMCStringEncodingNative, false, &t_data);
    
	if (!t_file -> Write(MCDataGetBytePtr(*t_data), MCDataGetLength(*t_data)))
		MCresult -> sets("error writing file");
	
	t_file -> Close();
    
    if (!MCresult -> isclear())
        return false;
    
    return true;
}

bool MCS_savebinaryfile(MCStringRef p_filename, MCDataRef p_data)
{
    // AL-2014-10-29: Reinstate secure mode check when trying to save file
    if (!MCSecureModeCanAccessDisk())
	{
		MCresult->sets("can't open file");
		return false;
	}
    
	MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path;
	bool t_success = true;
	
	if (!(MCS_resolvepath(p_filename, &t_resolved_path) && MCS_pathtonative(*t_resolved_path, &t_native_path)))
        return false;
	
    // MW-2014-10-24: [[ Bug 13797 ]] Don't create executable file.
	IO_handle t_file;
    t_file = MCsystem -> OpenFile(*t_native_path, (intenum_t)kMCOpenFileModeWrite, false);
	
	if (t_file == NULL)
	{
		MCresult -> sets("can't open file");
		return false;
	}
    
	if (!t_file -> Write(MCDataGetBytePtr(p_data), MCDataGetLength(p_data)))
	{
		MCresult -> sets("error writing file");
		t_success = false;
	}
	
	t_file -> Close();
	
    return t_success;
}

int64_t MCS_fsize(IO_handle p_stream)
{
	return p_stream -> GetFileSize();
}

IO_stat MCS_readfixed(void *p_ptr, uint32_t p_byte_size, IO_handle p_stream)
{
	if (MCabortscript || p_ptr == NULL || p_stream == NULL)
		return IO_ERROR;
    
    uint32_t t_read;
	
    if (!p_stream -> Read(p_ptr, p_byte_size, t_read) ||
        t_read != p_byte_size)
        return IO_ERROR;
    
    return IO_NORMAL;
}

IO_stat MCS_readall(void *p_ptr, uint32_t p_byte_count, IO_handle p_stream, uint32_t& r_bytes_read)
{
	if (MCabortscript || p_ptr == NULL || p_stream == NULL)
		return IO_ERROR;
    
    if (!p_stream -> Read(p_ptr, p_byte_count, r_bytes_read))
        return IO_ERROR;
    
    if (p_stream -> IsExhausted())
        return IO_EOF;
    
    return IO_NORMAL;
}

IO_stat MCS_write(const void *p_ptr, uint32_t p_size, uint32_t p_count, IO_handle p_stream)
{
	if (p_stream == IO_stdin)
		return IO_NORMAL;
	
	if (p_stream == NULL)
		return IO_ERROR;
	
	uint32_t t_to_write;
	t_to_write = p_size * p_count;
	
	if (!p_stream -> Write(p_ptr, t_to_write))
		return IO_ERROR;
	
	return IO_NORMAL;
}

IO_stat MCS_flush(IO_handle p_stream)
{
	if (!p_stream -> Flush())
		return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_trunc(IO_handle p_stream)
{
	if (!p_stream -> Truncate())
		return IO_ERROR;
	return IO_NORMAL;
}

int64_t MCS_tell(IO_handle p_stream)
{
	return p_stream -> Tell();
}

IO_stat MCS_sync(IO_handle p_stream)
{
	if (!p_stream -> Sync())
		return IO_ERROR;
	return IO_NORMAL;
}

Boolean MCS_eof(IO_handle p_stream)
{
	return (p_stream -> Tell() == p_stream ->  GetFileSize());
}

////////////////////////////////////////////////////////////////////////////////

void MCS_send(MCStringRef p_message, MCStringRef p_program, MCStringRef p_eventtype, Boolean reply)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    
    if (t_service != nil)
    {
	    t_service -> Send(p_message, p_program, p_eventtype, reply);
		return;
	}
    
	MCresult->sets("not supported");
}

void MCS_reply(MCStringRef p_message, MCStringRef p_keyword, Boolean p_error)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    
    if (t_service != nil)
	{
        t_service -> Reply(p_message, p_keyword, p_error);
		return;
	}
    
	MCresult->sets("not supported");
}

void MCS_request_ae(MCStringRef p_message, uint2 p_ae, MCStringRef& r_value)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    
    if (t_service != nil)
    {
	    t_service -> RequestAE(p_message, p_ae, r_value);
		return;
	}
    
	MCresult->sets("not supported");
}

bool MCS_request_program(MCStringRef p_message, MCStringRef p_program, MCStringRef& r_result)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    
    if (t_service != nil)
        return t_service -> RequestProgram(p_message, p_program, r_result);
    
	MCresult->sets("not supported");
	return true;
}

////////////////////////////////////////////////////////////////////////////////

IO_stat MCS_runcmd(MCStringRef p_command, MCStringRef& r_output)
{
    // TODO Change to MCDataRef or change Shell to MCStringRef
    MCAutoDataRef t_data;
    
    int t_retcode = 0;
    if (!MCsystem -> Shell(p_command, &t_data, t_retcode))
        return IO_ERROR;

    if (t_retcode)
        MCresult -> setnvalue(t_retcode);
    else
        MCresult -> clear();
    
    MCAutoStringRef t_data_string;
    // MW-2013-08-07: [[ Bug 11089 ]] The MCSystem::Shell() call returns binary data,
	//   so since uses of MCS_runcmd() expect text, we need to do EOL conversion.
    if (!MCStringCreateWithNativeChars((char_t*)MCDataGetBytePtr(*t_data), MCDataGetLength(*t_data), &t_data_string))
        r_output = MCValueRetain(kMCEmptyString);
    else
    {
        // SN-2014-10-14: [[ Bug 13658 ]] Get the behaviour back to what it was in 6.x:
        //  line-ending conversion for servers and Windows only
#if defined(_SERVER) || defined(_WINDOWS)
        if (!MCStringConvertLineEndingsToLiveCode(*t_data_string, r_output))
            r_output = MCValueRetain(kMCEmptyString);
#else
        r_output = MCValueRetain(*t_data_string);
#endif
    }
    
    return IO_NORMAL;
}

void MCS_startprocess(MCNameRef p_app, MCStringRef p_doc, intenum_t p_mode, Boolean p_elevated)
{
    MCsystem -> StartProcess(p_app, p_doc, p_mode, p_elevated);
}

void MCS_closeprocess(uint2 p_index)
{
    MCsystem -> CloseProcess(p_index);
}

void MCS_checkprocesses(void)
{
    MCsystem -> CheckProcesses();
}

void MCS_kill(int4 p_pid, int4 p_signal)
{
    MCsystem -> Kill(p_pid, p_signal);
}

void MCS_killall(void)
{
    MCsystem -> KillAll();
}

bool MCSTextConvertToUnicode(MCTextEncoding p_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
{
	return MCsystem -> TextConvertToUnicode(p_encoding, p_input, p_input_length, p_output, p_output_length, r_used);
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_getDNSservers(MCListRef& r_list)
{
    return MCsystem -> GetDNSservers(r_list);
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCS_poll(real8 p_delay, int p_fd)
{
    return MCsystem -> Poll(p_delay, p_fd);
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_isinteractiveconsole(int p_fd)
{
    return MCsystem -> IsInteractiveConsole(p_fd);
}

bool MCS_isnan(double p_number)
{
#ifdef _WIN32
    return (_isnan(p_number) != 0);
#else
    return (isnan(p_number) != 0);
#endif
}

////////////////////////////////////////////////////////////////////////////////

MCSysModuleHandle MCS_loadmodule(MCStringRef p_filename)
{
    MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path;
    
    if (!(MCS_resolvepath(p_filename, &t_resolved_path) && MCS_pathtonative(*t_resolved_path, &t_native_path)))
        return NULL;
    
	return MCsystem -> LoadModule(*t_native_path);
}

MCSysModuleHandle MCS_resolvemodulesymbol(MCSysModuleHandle p_module, MCStringRef p_symbol)
{

	return MCsystem -> ResolveModuleSymbol(p_module, p_symbol);
}

void MCS_unloadmodule(MCSysModuleHandle p_module)
{
	MCsystem -> UnloadModule(p_module);
}

// TODO: move somewhere better
#if defined(_LINUX_DESKTOP) || defined(_LINUX_SERVER)
MCLocaleRef MCS_getsystemlocale()
{
    // TODO: implement properly
    MCLocaleRef t_locale;
    /* UNCHECKED */ MCLocaleCreateWithName(MCSTR("en_US"), t_locale);
    return t_locale;
}
#endif

////////////////////////////////////////////////////////////////////////////////

bool MCS_changeprocesstype(bool p_to_foreground)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface*) MCsystem -> QueryService(kMCServiceTypeMacSystem);
    
    if (t_service != nil)
        return t_service -> ChangeProcessType(p_to_foreground);
    
    MCresult -> sets("not supported");
    return true;
}

bool MCS_processtypeisforeground(void)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface*) MCsystem -> QueryService(kMCServiceTypeMacSystem);
    
    if (t_service != nil)
        return t_service -> ProcessTypeIsForeground();
    
    MCresult -> sets("not supported");
    return true;
}

////////////////////////////////////////////////////////////////////////////////

uint32_t MCS_getsyserror(void)
{
    return MCsystem -> GetSystemError();
}

bool MCS_mcisendstring(MCStringRef p_command, MCStringRef& r_result, bool& r_error)
{
    MCWindowsSystemServiceInterface *t_service;
    t_service = (MCWindowsSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeWindowsSystem);
    
    if (t_service != nil)
        return t_service -> MCISendString(p_command, r_result, r_error);
	
    return MCStringCreateWithCString("not supported", r_result);
}

////////////////////////////////////////////////////////////////////////////////
