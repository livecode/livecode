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
#include "execpt.h"
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

////////////////////////////////////////////////////////////////////////////////

void MCS_common_init(void)
{	
	MCsystem -> Initialize();    
    MCsystem -> SetErrno(errno);
	
	IO_stdin = MCsystem -> OpenFd(0, kMCSystemFileModeRead) ;
	IO_stdout = MCsystem -> OpenFd(1, kMCSystemFileModeWrite);
	IO_stderr = MCsystem -> OpenFd(2, kMCSystemFileModeWrite);
	
	MCinfinity = HUGE_VAL;
    
	MCuppercasingtable = new uint1[256];
	for(uint4 i = 0; i < 256; ++i)
		MCuppercasingtable[i] = (uint1)toupper((uint1)i);
	
	MClowercasingtable = new uint1[256];
	for(uint4 i = 0; i < 256; ++i)
		MClowercasingtable[i] = (uint1)tolower((uint1)i);
	
	MCStackSecurityInit();
}

void MCS_init(void)
{
#if defined(_WINDOWS_SERVER)
	MCsystem = MCServerCreateWindowsSystem();
#elif defined(_MAC_SERVER)
	MCsystem = MCServerCreateMacSystem();
#elif defined(_LINUX_SERVER) || defined(_DARWIN_SERVER)
	MCsystem = MCServerCreatePosixSystem();
#elif defined(_MAC_DESKTOP)
    MCsystem = MCDesktopCreateMacSystem();
#elif defined(_WINDOWS_DESKTOP)
    MCsystem = MCDesktopCreateWindowsSystem();
#elif defined(_LINUX_DESKTOP)
    MCsystem = MCDesktopCreateLinuxSystem();
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
    
#if defined(_SERVER) || defined(_MOBILE)
	MCS_common_init();
#else
    MCsystem -> Initialize();
#endif
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

void MCS_reset_time(void)
{
    MCsystem -> ResetTime();
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
	MCsystem -> GetEnv(p_name_string, r_result);
    if (r_result != nil)
        return true;
    return false;
        
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
bool MCS_query_registry(MCStringRef p_key, MCStringRef& r_value, MCStringRef& r_type, MCStringRef& r_error)
{
    MCWindowsSystemServiceInterface *t_service;
    t_service = (MCWindowsSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeWindowsSystem);
    
    if (t_service != nil)
        return t_service -> QueryRegistry(p_key, r_value, r_type, r_error);
    
	return MCStringCreateWithCString("not supported", r_error);
}

/* LEGACY */
void MCS_query_registry(MCExecPoint &dest)
{
	MCAutoStringRef t_key;
	/* UNCHECKED */ dest.copyasstringref(&t_key);
	MCAutoStringRef t_value, t_type, t_error;
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

bool MCS_set_registry(MCStringRef p_key, MCStringRef p_value, MCStringRef p_type, MCStringRef& r_error)
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
        MCAutoStringRef t_native_path;
        if (!MCS_pathtonative(p_path, &t_native_path))
            return false;
        
        return t_service -> ListRegistry(*t_native_path, r_list, r_error);        
    }
    
	return MCStringCreateWithCString("not supported", r_error);
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
	
	return MCS_unlink(*t_resolved_path);
}

Boolean MCS_unlink(MCStringRef p_path)
{
	MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path;
    
	if (!MCS_resolvepath(p_path, &t_resolved_path))
        return False;
    
    if (!MCS_pathtonative(*t_resolved_path, &t_native_path))
        return False;
	
	return MCsystem -> DeleteFile(*t_resolved_path);
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
    
    if (!MCS_resolvepath(p_target, &t_target_resolved) || !MCS_resolvepath(p_target, &t_alias_resolved))
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
    }
    
    MCresult -> sets("not supported");
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


//Boolean MCS_setcurdir(const char *p_folder)
//{
//    char *t_resolved_folder;
//    t_resolved_folder = MCS_resolvepath(p_folder);
//    
//    bool t_success;
//    t_success = MCsystem -> SetCurrentFolder(t_resolved_folder);
//    delete t_resolved_folder;
//    return t_success;
//}

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

//struct MCS_getentries_state
//{
//	bool files;
//	bool details;
//	MCAutoListRef list;
//};

//bool MCFiltersUrlEncode(MCStringRef p_source, MCStringRef& r_result);

//static bool MCS_getentries_callback(void *p_context, const MCSystemFolderEntry *p_entry)
//{
//	MCS_getentries_state *t_state;
//	t_state = static_cast<MCS_getentries_state *>(p_context);
//	
//	if (!t_state -> files != p_entry -> is_folder)
//		return true;
//	
//	MCStringRef t_detailed_string;
//	if (t_state -> details)
//	{
//		MCStringRef t_filename_string;
//		/* UNCHECKED */ MCStringCreateWithNativeChars((const char_t *)p_entry->name, MCCStringLength(p_entry->name), t_filename_string);
//		MCStringRef t_urlencoded_string;
//		/* UNCHECKED */ MCFiltersUrlEncode(t_filename_string, t_urlencoded_string);
//		MCValueRelease(t_filename_string);
//#ifdef _WIN32
//		/* UNCHECKED */ MCStringFormat(t_detailed_string,
//                                       "%*.*s,%I64d,,%ld,%ld,%ld,,,,%03o,",
//                                       MCStringGetLength(t_urlencoded_string), MCStringGetLength(t_urlencoded_string),
//                                       MCStringGetNativeCharPtr(t_urlencoded_string),
//                                       p_entry -> data_size,
//                                       p_entry -> creation_time,
//                                       p_entry -> modification_time,
//                                       p_entry -> access_time,
//                                       p_entry -> permissions);
//#else
//		/* UNCHECKED */ MCStringFormat(t_detailed_string,
//                                       "%*.*s,%lld,,,%u,%u,,%d,%d,%03o,",
//                                       MCStringGetLength(t_urlencoded_string), MCStringGetLength(t_urlencoded_string),
//                                       MCStringGetNativeCharPtr(t_urlencoded_string),
//                                       p_entry -> data_size,
//                                       p_entry -> modification_time, p_entry -> access_time,
//                                       p_entry -> user_id, p_entry -> group_id,
//                                       p_entry -> permissions);
//#endif
//		MCValueRelease(t_urlencoded_string);
//	}
//	if (t_state -> details)
//	{
//		/* UNCHECKED */ MCListAppend(*(t_state->list), t_detailed_string);
//		MCValueRelease(t_detailed_string);
//	}
//	else
//    /* UNCHECKED */ MCListAppendCString(*(t_state->list), p_entry->name);
//	
//	return true;
//}

bool MCS_getentries(bool p_files, bool p_detailed, MCListRef& r_list)
{    
	MCAutoListRef t_list;
    
	if (!MCListCreateMutable('\n', &(t_list)))
		return False;
	
	if (!MCsystem -> ListFolderEntries(p_files, p_detailed, &t_list))
		return False;
    
	if (!MCListCopy(*t_list, r_list))
        return False;
    
    return True;
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
//
//uint2 MCS_umask(uint2 p_mask)
//{
//	return MCsystem -> UMask(p_mask);
//}

/* WRAPPER */
Boolean MCS_exists(MCStringRef p_path, bool p_is_file)
{
	MCAutoStringRef t_resolved;
    MCAutoStringRef t_native;
	if (!(MCS_resolvepath(p_path, &t_resolved) && MCS_pathtonative(*t_resolved, &t_native)))
		return False;
    
    bool t_success;
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

IO_handle MCS_fakeopen(const MCString& data)
{
	MCMemoryFileHandle *t_handle;
	t_handle = new MCMemoryFileHandle(data . getstring(), data . getlength());
	return t_handle;
}

IO_handle MCS_fakeopenwrite(void)
{
	MCMemoryFileHandle *t_handle;
	t_handle = new MCMemoryFileHandle();
	return t_handle;
}

//IO_stat MCS_fakeclosewrite(IO_handle& p_stream, char*& r_buffer, uint4& r_length)
//{
//	if ((p_stream -> flags & IO_FAKEWRITE) != IO_FAKEWRITE)
//	{
//		r_buffer = NULL;
//		r_length = 0;
//		MCS_close(p_stream);
//		return IO_ERROR;
//	}
//	
//	MCMemoryFileHandle *t_handle;
//	t_handle = static_cast<MCMemoryFileHandle *>(p_stream -> handle);
//	t_handle -> TakeBuffer(r_buffer, r_length);
//	
//	MCS_close(p_stream);
//	
//	return IO_NORMAL;
//}

IO_stat MCS_closetakingbuffer(IO_handle& p_stream, void*& r_buffer, size_t& r_length)
{
    bool t_success;

    t_success = p_stream -> TakeBuffer(r_buffer, r_length);
	
	MCS_close(p_stream);
	
    if (!t_success)
        return IO_ERROR;
    
	return IO_NORMAL;
}
//
//uint32_t MCS_faketell(IO_handle stream)
//{
//	return (uint32_t)static_cast<MCSystemFileHandle *>(stream -> handle) -> Tell();
//}
//
//bool MCS_isfake(IO_handle p_stream)
//{
//	return (p_stream -> flags & IO_FAKEWRITE) != 0;
//}

IO_stat MCS_writeat(const void *p_buffer, uint32_t p_size, uint32_t p_pos, IO_handle p_stream)
{
    uint64_t t_old_pos;
    uint32_t t_written;
    bool t_success;
    
    t_old_pos = p_stream -> Tell();
    
    t_success = p_stream -> Seek(p_pos, SEEK_SET);
    
    if (t_success)
        t_success = p_stream -> Write(p_buffer, p_size);
    
    if (t_success)
        t_success = p_stream -> Seek(t_old_pos, SEEK_SET);
    
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
#ifdef /* MCS_open */ LEGACY_SYSTEM
IO_handle MCS_open(const char *p_path, const char *p_mode, Boolean p_map, Boolean p_driver, uint4 p_offset)
{
    char *t_resolved_path;
    t_resolved_path = MCS_resolvepath(p_path);
    
    uint32_t t_mode;
    if (strequal(p_mode, IO_READ_MODE))
        t_mode = kMCSystemFileModeRead;
    else if (strequal(p_mode, IO_WRITE_MODE))
        t_mode = kMCSystemFileModeWrite;
    else if (strequal(p_mode, IO_UPDATE_MODE))
        t_mode = kMCSystemFileModeUpdate;
    else if (strequal(p_mode, IO_APPEND_MODE))
        t_mode = kMCSystemFileModeAppend;
    
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
        t_handle -> Seek(p_offset, 1);
    
    return new IO_header(t_handle, 0);;
}
#endif /* MCS_open */


IO_handle MCS_open(MCStringRef path, intenum_t p_mode, Boolean p_map, Boolean p_driver, uint32_t p_offset)
{
	MCAutoStringRef t_resolved;
    MCAutoStringRef t_native;
    
	if (!(MCS_resolvepath(path, &t_resolved) && MCS_pathtonative(*t_resolved, &t_native)))
        return NULL;
	
	IO_handle t_handle;
	if (!p_driver)
		t_handle = MCsystem -> OpenFile(*t_native, p_mode, p_map && MCmmap, p_offset);
	else
	{
		t_handle = MCsystem -> OpenDevice(*t_native, p_offset);
	}
	
	// MW-2011-06-12: Fix memory leak - make sure we delete the resolved path.
    //	delete t_resolved_path;
	
	if (t_handle == NULL)
		return NULL;
#ifdef OLD_IO_HANDLE
    if (p_mode == kMCSystemFileModeAppend)
        t_handle -> flags |= IO_SEEKED;
#endif
	
	return t_handle;
}

void MCS_close(IO_handle &x_stream)
{
	x_stream -> Close();
}

IO_stat MCS_putback(char p_char, IO_handle p_stream)
{
	if (!p_stream -> PutBack(p_char))
		return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_seek_cur(IO_handle p_stream, int64_t p_offset)
{
	if (!p_stream -> Seek(p_offset, 0))
		return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_seek_set(IO_handle p_stream, int64_t p_offset)
{
	if (!p_stream -> Seek(p_offset, 1))
		return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_seek_end(IO_handle p_stream, int64_t p_offset)
{
	if (!p_stream -> Seek(p_offset, -1))
		return IO_ERROR;
	return IO_NORMAL;
}

#if 0
void MCS_loadfile(MCExecPoint& ep, Boolean p_binary)
{
	MCAutoStringRef t_resolved_path;
	MCAutoStringRef t_filename_string;
	ep . copyasstringref(&t_filename_string);
	
	MCS_resolvepath(*t_filename_string, &t_resolved_path);
	
	MCSystemFileHandle *t_file;
	t_file = MCsystem -> OpenFile(*t_resolved_path, kMCSystemFileModeRead, false);
	
	if (t_file == NULL)
	{
		// MW-2011-05-23: [[ Bug 9549 ]] Make sure we empty the result if opening the file
		//   failed.
		ep . clear();
		MCresult -> sets("can't open file");
		return;
	}
	
	uint32_t t_size;
	t_size = (uint32_t)t_file -> GetFileSize();
	
	MCAutoNativeCharArray t_buffer;
	/* UNCHECKED */ t_buffer . New(t_size);
	//t_buffer = ep . getbuffer(t_size);
	
	uint32_t t_read;
	if (t_buffer.Chars() != NULL &&
		t_file -> Read(t_buffer.Chars(), t_size, t_read) &&
		t_read == t_size)
	{
		MCAutoStringRef t_string;
		t_buffer . Shrink(t_size);
		t_buffer . CreateStringAndRelease(&t_string);
		ep . setvalueref(*t_string);
        
		if (!p_binary)
			ep . texttobinary();
		MCresult -> clear(False);
	}
	else
	{
		ep . clear();
		MCresult -> sets("error reading file");
	}
    
	t_file -> Close();
}
#endif // 0

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
	t_file = MCsystem -> OpenFile(*t_native_path, (intenum_t)kMCSystemFileModeRead, false, 0);
	
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
	
	uint32_t t_read;
	if (t_success)
        t_success = MCS_readfixed(t_buffer.Chars(), t_size, t_file) == IO_NORMAL;

    if (t_success)
    {
        MCExecPoint ep(nil, nil, nil);
        MCAutoStringRef t_string;
        
		t_buffer . Shrink(t_size);
		t_success = t_buffer . CreateStringAndRelease(&t_string);
        
        ep . setvalueref(*t_string);
        ep . texttobinary();
        
        t_success = ep.copyasstring(r_text);
        MCresult -> clear(False);
        ep.clear();
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
	t_file = MCsystem -> OpenFile(*t_native_path, (intenum_t)kMCSystemFileModeRead, false, 0);
	
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
	
	uint32_t t_read;
	if (t_success)
        t_success = MCS_readfixed(t_buffer.Chars(), t_size, t_file) == IO_NORMAL;
    
    if (t_success)
    {
        MCAutoStringRef t_string;
		t_buffer . Shrink(t_size);
        t_success = t_buffer.CreateString(&t_string) &&
                    MCDataCreateWithBytes(MCStringGetBytePtr(*t_string), MCStringGetLength(*t_string), r_data);
    }
    
    if (t_success)
    {
        MCresult -> clear(False);
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
	MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path;
	
	if (!(MCS_resolvepath(p_filename, &t_resolved_path) && MCS_pathtonative(*t_resolved_path, &t_native_path)))
        return false;
	
	IO_handle t_file;
	t_file = MCsystem -> OpenFile(*t_native_path, (intenum_t)kMCSystemFileModeWrite, false, 0);
	
	if (t_file == NULL)
	{
		MCresult -> sets("can't open file");
		return false;
	}
    
    // Need to convert the string to a binary string
    MCAutoStringRef t_string;
    MCExecPoint ep(nil, nil, nil);
    /* UNCHECKED */ ep . setvalueref(p_string);
    ep . binarytotext();
    /* UNCHECKED */ ep . copyasstring(&t_string);
    
	if (!t_file -> Write(MCStringGetBytePtr(*t_string), MCStringGetLength(*t_string)))
		MCresult -> sets("error writing file");
	
	t_file -> Close();
    
    if (!MCresult -> isclear())
        return false;
    
    return true;
}

bool MCS_savebinaryfile(MCStringRef p_filename, MCDataRef p_data)
{
	MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path;
	
	if (!(MCS_resolvepath(p_filename, &t_resolved_path) && MCS_pathtonative(*t_resolved_path, &t_native_path)))
        return false;
	
	IO_handle t_file;
	t_file = MCsystem -> OpenFile(*t_native_path, (intenum_t)kMCSystemFileModeWrite, false, 0);
	
	if (t_file == NULL)
	{
		MCresult -> sets("can't open file");
		return false;
	}
    
	uint32_t t_written;
	if (!t_file -> Write(MCDataGetBytePtr(p_data), MCDataGetLength(p_data)))
		MCresult -> sets("error writing file");
	
	t_file -> Close();
	
    if (!MCresult -> isclear())
		return false;
    
    return true;
}

//void MCS_loadresfile(MCExecPoint& ep)
//{
//	ep . clear();
//	MCresult -> sets("not supported");
//}

//void MCS_saveresfile(const MCString& p_filename, const MCString p_data)
//{
//	MCresult -> sets("not supported");
//}

int64_t MCS_fsize(IO_handle p_stream)
{
	return p_stream -> GetFileSize();
}

//IO_stat MCS_read(void *p_ptr, uint4 p_size, uint4& p_count, IO_handle p_stream)
//{
//	if (MCabortscript || p_ptr == NULL || p_stream == NULL)
//		return IO_ERROR;
//	
//	if ((p_stream -> flags & IO_FAKEWRITE) == IO_FAKEWRITE)
//		return IO_ERROR;
//	
//	uint32_t t_to_read;
//	t_to_read = p_size * p_count;
//	
//	uint32_t t_read;
//	if (!p_stream -> handle -> Read(p_ptr, t_to_read, t_read))
//	{
//		p_count = t_read / p_size;
//		return IO_ERROR;
//	}
//	
//	p_count = t_read / p_size;
//	
//	if (t_read < t_to_read)
//	{
//		p_stream -> flags |= IO_ATEOF;
//		p_count = t_read / p_size;
//		return IO_EOF;
//	}
//	
//	p_stream -> flags &= ~IO_ATEOF;
//	
//	return IO_NORMAL;
//}

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
	
	uint32_t t_written;
	if (!p_stream -> Write(p_ptr, t_to_write))
		return IO_ERROR;
	
	return IO_NORMAL;
}

//IO_handle MCS_writeat(const void *p_buffer, uint32_t p_size, uint32_t p_pos, IO_handle p_stream)
//{
//    IO_stat stat;
//    stat = p_stream -> handle -> Seek(p_pos, SEEK_SET);
//    if (stat == IO_NORMAL)
//        stat = p_stream -> handle -> Write(p_buffer, p_size, <#uint32_t &r_written#>)
//    if (stat == IO_NORMAL)
//        stat = MCS_seek_set(stream, t_cur_offset);
//    
//    return stat;
//}

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

bool MCS_get_canonical_path(MCStringRef p_path, MCStringRef& r_path)
{
    MCAutoStringRef t_resolved_path;
    MCAutoStringRef t_native_path;
	
    if (!(MCS_resolvepath(p_path, &t_resolved_path) && MCS_pathtonative(*t_resolved_path, &t_native_path)))
        return false;
    
	MCsystem -> GetCanonicalPath(*t_native_path, r_path);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCS_send(MCStringRef p_message, MCStringRef p_program, MCStringRef p_eventtype, Boolean reply)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    
    if (t_service != nil)
        t_service -> Send(p_message, p_program, p_eventtype, reply);
    
	MCresult->sets("not supported");
}

void MCS_reply(MCStringRef p_message, MCStringRef p_keyword, Boolean p_error)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    
    if (t_service != nil)
        t_service -> Reply(p_message, p_keyword, p_error);
    
	MCresult->sets("not supported");
}

void MCS_request_ae(MCStringRef p_message, uint2 p_ae, MCStringRef& r_value)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    
    if (t_service != nil)
        t_service -> RequestAE(p_message, p_ae, r_value);
    
	MCresult->sets("not supported");
}

bool MCS_request_program(MCStringRef p_message, MCStringRef p_program, MCStringRef& r_result)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface *)MCsystem -> QueryService(kMCServiceTypeMacSystem);
    
    if (t_service != nil)
        t_service -> RequestProgram(p_message, p_program, r_result);
    
	MCresult->sets("not supported");
}

////////////////////////////////////////////////////////////////////////////////

//IO_stat MCS_runcmd(MCExecPoint& ep)
//{
//	void *t_data;
//	uint32_t t_data_length;
//	int t_return_code;
//    
//	MCAutoStringRef t_filename_string;
//	/* UNCHECKED */ MCStringCreateWithCString(ep . getsvalue() . getstring(), &t_filename_string);
//	if (!MCsystem -> Shell(*t_filename_string,(MCDataRef &) t_data, t_return_code))
//	{
//		MCresult -> clear(False);
//		MCeerror -> add(EE_SHELL_BADCOMMAND, 0, 0, ep.getsvalue());
//		return IO_ERROR;
//	}
//	ep . grabbuffer((char *)t_data, t_data_length);
//	
//	MCresult -> setnvalue(t_return_code);
//	
//	return IO_NORMAL;
//}

IO_stat MCS_runcmd(MCStringRef p_command, MCStringRef& r_output)
{
    // TODO Change to MCDataRef or change Shell to MCStringRef
    MCAutoDataRef t_data;
    
    int t_retcode;
    if (!MCsystem -> Shell(p_command, &t_data, t_retcode))
        return IO_ERROR;
    
    MCresult -> setnvalue(t_retcode);
    if (!MCStringCreateWithNativeChars((char_t*)MCDataGetBytePtr(*t_data), MCDataGetLength(*t_data), r_output))
    {
        r_output = MCValueRetain(kMCEmptyString);
    }
    return IO_NORMAL;
}

void MCS_startprocess(MCNameRef p_app, MCStringRef p_doc, Open_mode p_mode, Boolean p_elevated)
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

//MCSocket::~MCSocket(void)
//{
//}
//
//MCSocket::MCSocket(MCNameRef n, MCObject *o, MCNameRef m, Boolean d, MCSocketHandle sock,Boolean a, Boolean s, Boolean issecure)
//{
//}
//
//void MCSocket::close(void)
//{
//}
//
//void MCSocket::doclose(void)
//{
//}
//
//Boolean MCSocket::init(MCSocketHandle newfd)
//{
//	return False;
//}
//
//MCSocket *MCS_open_socket(MCNameRef p_name, Boolean p_datagram, MCObject *p_object, MCNameRef p_message, Boolean p_secure, Boolean p_ssl_verify, MCStringRef p_ssl_cert_file)
//{
//	return NULL;
//}
//
//void MCS_close_socket(MCSocket *p_socket)
//{
//}
//
//
//void MCS_read_socket(MCSocket *p_socket, MCObject *p_object, MCStringRef p_until, MCNameRef p_message, MCDataRef& r_read_data)
//{
//}
//
//void MCS_write_socket(const MCStringRef p_data, MCSocket *p_socket, MCObject *p_object, MCNameRef p_message)
//{
//}
//
//MCSocket *MCS_accept(uint2 p_port, MCObject* p_object, MCNameRef p_message, Boolean p_datagram, Boolean p_secure, Boolean p_ssl_verify, MCStringRef p_ssl_cert_file)
//{
//	return NULL;
//}
//
//bool MCS_ha(MCSocket *s, MCStringRef& r_string)
//{
//	r_string = MCValueRetain(kMCEmptyString);
//	return true;
//}
//
//bool MCS_pa(MCSocket *p_socket, MCStringRef& r_string)
//{
//	r_string = MCValueRetain(kMCEmptyString);
//	return true;
//}
//
//bool MCS_hn(MCStringRef& r_string)
//{
//	char *t_host_name;
//	t_host_name = MCsystem -> GetHostName();
//	if (t_host_name == NULL)
//	{
//		r_string = MCValueRetain(kMCEmptyString);
//		return true;
//	}
//	
//	bool t_success;
//	t_success = MCStringCreateWithCString(t_host_name, r_string);
//	
//	delete t_host_name;
//    
//	return t_success;
//}
//
//static bool MCS_resolve_callback(void *p_context, MCStringRef p_host)
//{
//	MCListRef t_list = (MCListRef)p_context;
//	return MCListAppend(t_list, p_host);
//}
//
//bool MCS_aton(MCStringRef p_address, MCStringRef& r_name)
//{
//	MCAutoListRef t_list;
//	if (!MCListCreateMutable('\n', &t_list))
//		return false;
//	if (!MCsystem -> AddressToHostName(p_address, MCS_resolve_callback, *t_list))
//	{
//		r_name = MCValueRetain(kMCEmptyString);
//		MCresult -> sets("invalid host address");
//	}
//	else
//	{
//		MCresult -> clear();
//		return MCListCopyAsString(*t_list, r_name);
//	}
//	return true;
//}
//
//bool MCS_ntoa(MCStringRef p_hostname, MCObject *p_target, MCNameRef p_message, MCListRef& r_addr)
//{
//	if (!MCNameIsEqualTo(p_message, kMCEmptyName))
//	{
//		MCresult -> sets("not supported");
//		r_addr = MCValueRetain(kMCEmptyList);
//		return true;
//	}
//	
//	MCAutoListRef t_list;
//	if (!MCListCreateMutable('\n', &t_list))
//		return false;
//	if (!MCsystem -> HostNameToAddress(p_hostname, MCS_resolve_callback, *t_list))
//	{
//		r_addr = MCValueRetain(kMCEmptyList);
//		MCresult -> sets("invalid host name");
//	}
//	else
//	{
//		MCresult -> clear();
//		return MCListCopy(*t_list, r_addr);
//	}
//	return true;
//}

bool MCS_getDNSservers(MCListRef& r_list)
{
    return MCsystem -> GetDNSservers(r_list);
}
//
//void MCS_dnsresolve(MCStringRef p_hostname, MCStringRef& r_dns)
//{
//	return;
//}

bool MCS_hostaddress(MCStringRef& r_host_address)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCS_poll(real8 p_delay, int p_fd)
{
//	MCsystem -> Sleep(p_delay);
//	return False;
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
    return _isnan(p_number);
#else
    return isnan(p_number);
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

////////////////////////////////////////////////////////////////////////////////

bool MCS_changeprocesstype(bool p_to_foreground)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface*) MCsystem -> QueryService(kMCServiceTypeMacSystem);
    
    if (!t_service != nil)
        return t_service -> ChangeProcessType(p_to_foreground);
    
    MCresult -> sets("not supported");
    return true;
}

bool MCS_processtypeisforeground(void)
{
    MCMacSystemServiceInterface *t_service;
    t_service = (MCMacSystemServiceInterface*) MCsystem -> QueryService(kMCServiceTypeMacSystem);
    
    if (!t_service != nil)
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

/* TEMPORARY WRAPPER METHODS */
/* Should be replaced with native versions that use MCValueRefs */

//void MCSystemInterface::GetTemporaryFileName(MCStringRef &r_path)
//{
//	MCAutoPointer<char> t_path;
//	t_path = GetTemporaryFileName();
//	return *t_path != nil && MCStringCreateWithCString(*t_path, r_path);
//}

/* LEGACY COMPATABILITY METHODS */
//char *MCSystemInterface::PathFromNative(const char *p_native)
//{
//	MCAutoStringRef t_native;
//	MCAutoStringRef t_path;
//	char *t_cstring = nil;
//	if (MCStringCreateWithCString(p_native, &t_native) &&
//		PathFromNative(*t_native, &t_path) &&
//		MCCStringClone(MCStringGetCString(*t_path), t_cstring))
//		return t_cstring;
//	return nil;
//}

//char *MCSystemInterface::GetCurrentFolder(void)
//{
//	MCAutoStringRef t_path;
//	char *t_cstring = nil;
//	if (GetCurrentFolder(&t_path) &&
//		MCCStringClone(MCStringGetCString(*t_path), t_cstring))
//		return t_cstring;
//	return nil;
//}

/*
 char *MCSystemInterface::ResolvePath(const char *p_path)
 {
 MCAutoStringRef t_path;
 MCAutoStringRef t_resolved;
 char *t_cstring = nil;
 bool t_success;
 t_success = true;
 
 if (t_success)
 t_success = MCStringCreateWithCString(p_path, &t_path);
 
 if (t_success)
 t_success = ResolvePath(*t_path, &t_resolved);
 
 if (t_success)
 t_success = MCCStringClone(MCStringGetCString(*t_resolved), t_cstring);
 
 if (t_success)
 return t_cstring;
 }
 */

/*
 char *MCSystemInterface::ResolveNativePath(const char *p_path)
 {
 MCAutoStringRef t_path;
 MCAutoStringRef t_resolved;
 char *t_cstring = nil;
 bool t_success;
 t_success = true;
 
 if (t_success)
 t_success = MCStringCreateWithCString(p_path, &t_path);
 
 if (t_success)
 t_success = ResolveNativePath(*t_path, &t_resolved);
 
 if (t_success)
 t_success = MCCStringClone(MCStringGetCString(*t_resolved), t_cstring);
 
 if (t_success)
 return t_cstring;
 
 return nil;
 }
 */

////////////////////////////////////////////////////////////////////////////////
