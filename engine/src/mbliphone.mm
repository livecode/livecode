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

#include "system.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "globals.h"
#include "uidc.h"

#include "variable.h"

#include "socket.h"

#undef isatty
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/time.h>
#include <dirent.h>
#include <dlfcn.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach-o/loader.h>
#include <mach-o/getsect.h>
#include <mach-o/dyld.h>
#include <Security/SecRandom.h>

#import <Foundation/Foundation.h>
#import <UIKit/UIDevice.h>

#include "mbliphone.h"

////////////////////////////////////////////////////////////////////////////////

extern bool MCSystemLaunchUrl(MCStringRef p_url);

////////////////////////////////////////////////////////////////////////////////

uint1 *MCuppercasingtable;
uint1 *MClowercasingtable;
real8 curtime;

////////////////////////////////////////////////////////////////////////////////

@implementation NSString (com_runrev_livecode_NSStringAdditions)
	- (const char *)nativeCString
	{
		// The length of a native string is at most the length of the string.
		NSUInteger t_bytes_available, t_bytes_used;
		t_bytes_available = [self length];
		
		char *t_buffer;
		/* UNCHECKED */ t_buffer = (char *)malloc(t_bytes_available + 1);
		
		[self getBytes: t_buffer
				maxLength: t_bytes_available
				usedLength: &t_bytes_used
				encoding: NSMacOSRomanStringEncoding
				options: NSStringEncodingConversionAllowLossy
				range: NSMakeRange(0, [self length])
				remainingRange: nil];
				
		t_buffer[t_bytes_used] = '\0';
		
		NSData *t_data;
		t_data = [NSData dataWithBytesNoCopy: t_buffer length: t_bytes_used + 1 freeWhenDone: YES];
		
		return (const char *)[t_data bytes];
	}

@end

////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void *load_module(const char *);
extern "C" MC_DLLEXPORT_DEF void *resolve_symbol(void *, const char *);

struct LibExport
{
	const char *name;
	void *address;
};

struct LibInfo
{
	const char **name;
	struct LibExport *exports;
};

MC_DLLEXPORT_DEF
void *load_module(const char *p_path)
{
	const char *t_last_component;
	t_last_component = strrchr(p_path, '/');
	if (t_last_component == nil)
		t_last_component = p_path;
	else
		t_last_component += 1;
	
    // MW-2013-06-26: [[ Bug 10914 ]] Make sure we fetch the section in a way
    //   compatible with ASLR (the section address needs to be modified by the
    //   'slide' for the image).
    unsigned long t_section_data_size;
    char *t_section_data;
    t_section_data = getsectdata("__DATA", "__libs", &t_section_data_size);
    if (t_section_data != nil)
    {  
        t_section_data += (unsigned long)_dyld_get_image_vmaddr_slide(0);
        
        LibInfo **t_libs;
		t_libs = (LibInfo **)t_section_data;
		for(uint32_t k = 0; k < t_section_data_size / sizeof(void *); k++)
		{
			const char *t_lib_name;
			t_lib_name = *(t_libs[k] -> name);
			if (!MCCStringBeginsWithCaseless(t_last_component, t_lib_name))
				continue;
			if (MCCStringLength(t_last_component) != MCCStringLength(t_lib_name) + 6)
				continue;
			
			return (void *)((uintptr_t)t_libs[k] | 1);
		}

    }
	
	return NULL;	
}

void *resolve_symbol(void *p_module, const char *p_symbol) __attribute__((__visibility__("default")))

{
	LibInfo *t_lib;
	t_lib = (LibInfo *)((uintptr_t)p_module & ~1);
	
	for(uint32_t i = 0; t_lib -> exports[i] . name != nil; i++)
		if (MCCStringEqual(p_symbol, t_lib -> exports[i] . name))
			return t_lib -> exports[i] . address;
	
	return nil;
}

static bool is_static_module(void *p_module)
{
	return (((uintptr_t)p_module) & 1) != 0;
}

////////////////////////////////////////////////////////////////////////

static char *my_strndup(const char *s, uint32_t l)
{
	char *r;
	r = new char[l + 1];
	strncpy(r, s, l);
	// MW-2010-06-15: [[ Bug ]] Make sure things are nul-terminated!
	r[l] = '\0';
	return r;
}

class MCStdioFileHandle: public MCSystemFileHandle
{
public:
    
    MCStdioFileHandle(FILE *p_fptr)
    {
        m_stream = p_fptr;
    }
	
	virtual void Close(void)
	{
		fclose(m_stream);
		delete this;
	}
	
	virtual bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read)
	{
		size_t t_amount;
		t_amount = fread(p_buffer, 1, p_length, m_stream);
		r_read = t_amount;
		
		if (t_amount < p_length)
			return ferror(m_stream) == 0;
		
		return true;
	}
	
	virtual bool Write(const void *p_buffer, uint32_t p_length)
	{
		size_t t_amount;
		t_amount = fwrite(p_buffer, 1, p_length, m_stream);
		
		if (t_amount < p_length)
			return false;
		
		return true;
	}
    
    virtual bool IsExhausted()
    {
        return feof(m_stream);
    }
    
    virtual bool TakeBuffer(void *&r_buffer, size_t &r_length)
    {
        r_length = 0;
        return true;
    }
	
	virtual bool Seek(int64_t offset, int p_dir)
	{
		return fseeko(m_stream, offset, p_dir < 0 ? SEEK_END : (p_dir > 0 ? SEEK_SET : SEEK_CUR)) == 0;
	}
	
	virtual bool Truncate(void)
	{
		return ftruncate(fileno(m_stream), ftell(m_stream));
	}
	
	virtual bool Sync(void) 
	{
		int64_t t_pos;
		t_pos = ftell(m_stream);
		return fseek(m_stream, t_pos, SEEK_SET) == 0;
	}
	
	virtual bool Flush(void)
	{
		return fflush(m_stream) == 0;
	}
	
	virtual bool PutBack(char p_char)
	{
		return ungetc(p_char, m_stream) != EOF;
	}
	
	virtual int64_t Tell(void)
	{
		return ftell(m_stream);
	}
	
	virtual uint64_t GetFileSize(void)
	{
		struct stat t_info;
		if (fstat(fileno(m_stream), &t_info) != 0)
			return 0;
		return t_info . st_size;
	}
	
	virtual void *GetFilePointer(void)
	{
		return NULL;
	}
	
private:
	FILE *m_stream;
};

// MM-2012-11-22: [[ Bug 10540 ]] - For iOS 6 the standard io streams appear to no longer work.  Instead, use NSLog for output (no input supported).
class MCStdioFileDescriptorHandle: public MCSystemFileHandle
{
public:
    
    MCStdioFileDescriptorHandle()
    {
    }
	
	virtual void Close(void)
	{
		delete this;
	}
    
    virtual bool IsExhausted()
    {
        return false;
    }
    
    virtual bool TakeBuffer(void*& r_buffer, size_t& r_length)
    {
        r_length = 0;
        return true;
    }
	
	virtual bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read)
	{
        r_read = 0;
		return true;
	}
	
	virtual bool Write(const void *p_buffer, uint32_t p_length)
	{
        NSLog(@"%s", p_buffer);
		return true;
	}
	
	virtual bool Seek(int64_t offset, int p_dir)
	{
        return true;
	}
	
	virtual bool Truncate(void)
	{
		return true;
	}
	
	virtual bool Sync(void)
	{
		return true;
	}
	
	virtual bool Flush(void)
	{
		return true;
	}
	
	virtual bool PutBack(char p_char)
	{
		return true;
	}
	
	virtual int64_t Tell(void)
	{
		return 0;
	}
	
	virtual uint64_t GetFileSize(void)
	{
        return 0;
	}
	
	virtual void *GetFilePointer(void)
	{
		return NULL;
	}
	
};

MCServiceInterface *MCIPhoneSystem::QueryService(MCServiceType type)
{
    return nil;
}

real64_t MCIPhoneSystem::GetCurrentTime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	curtime = tv . tv_sec + tv . tv_usec / 1000000.0;
    return curtime;
}

uint32_t MCIPhoneSystem::GetProcessId(void)
{
	return getpid();
}

bool MCIPhoneSystem::GetVersion(MCStringRef& r_string)
{
	return MCStringCreateWithCFStringRef((CFStringRef)[[UIDevice currentDevice] systemVersion], r_string);
}

bool MCIPhoneSystem::GetMachine(MCStringRef& r_string)
{
    NSString *t_machine = [[UIDevice currentDevice] model];
#if TARGET_IPHONE_SIMULATOR
    t_machine = [t_machine stringByAppendingString:@" Simulator"];
#endif
    
    return MCStringCreateWithCFStringRef((CFStringRef)t_machine, r_string);
}

bool MCIPhoneSystem::GetAddress(MCStringRef& r_address)
{
//    bool MCS_getaddress(MCStringRef &r_address)
//    {
//        r_address = kMCEmptyString;
//        return true;
//    }    
	extern MCStringRef MCcmd;
    return MCStringFormat(r_address, "iphone:%@", MCcmd);
}

void MCIPhoneSystem::Alarm(real64_t p_when)
{
}

void MCIPhoneSystem::Sleep(real64_t p_when)
{
	usleep((uint32_t)(p_when * 1000000.0));
}

void MCIPhoneSystem::SetEnv(MCStringRef p_name, MCStringRef p_value)
{
    MCAutoStringRefAsUTF8String t_utf8_name, t_utf8_value;
    /* UNCHECKED */ t_utf8_name . Lock(p_name);
    /* UNCHECKED */ t_utf8_value . Lock(p_value);
	setenv(*t_utf8_name, *t_utf8_value, 1);
}

bool MCIPhoneSystem::GetEnv(MCStringRef p_name, MCStringRef& r_env)
{
    MCAutoStringRefAsUTF8String t_utf8_name;
    /* UNCHECKED */ t_utf8_name . Lock(p_name);
    return MCStringCreateWithCString(getenv(*t_utf8_name), r_env);
}

Boolean MCIPhoneSystem::CreateFolder(MCStringRef p_path)
{
    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path . Lock(p_path);
	if (mkdir(*t_utf8_path, 0777) != 0)
        return False;
    
    return True;
}

Boolean MCIPhoneSystem::DeleteFolder(MCStringRef p_path)
{
    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path . Lock(p_path);
	if (rmdir(*t_utf8_path) != 0)
        return False;
    
    return True;
}

Boolean MCIPhoneSystem::DeleteFile(MCStringRef p_path)
{
    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path . Lock(p_path);
	if (unlink(*t_utf8_path) != 0)
        return False;
    
    return True;
}

Boolean MCIPhoneSystem::RenameFileOrFolder(MCStringRef p_old_name, MCStringRef p_new_name)
{
    MCAutoStringRefAsUTF8String t_old_name_utf8, t_new_name_utf8;
    /* UNCHECKED */ t_old_name_utf8 . Lock(p_old_name);
    /* UNCHECKED */ t_new_name_utf8 . Lock(p_new_name);
    
	if (rename(*t_old_name_utf8, *t_new_name_utf8) != 0)
        return False;
    
    return True;
}

Boolean MCIPhoneSystem::BackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
{
    MCAutoStringRefAsUTF8String t_old_name_utf8, t_new_name_utf8;
    /* UNCHECKED */ t_old_name_utf8 . Lock(p_old_name);
    /* UNCHECKED */ t_new_name_utf8 . Lock(p_new_name);
	if (rename(*t_old_name_utf8, *t_new_name_utf8) != 0)
        return False;
    
    return True;
}

Boolean MCIPhoneSystem::UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
{
    MCAutoStringRefAsUTF8String t_old_name_utf8, t_new_name_utf8;
    /* UNCHECKED */ t_old_name_utf8 . Lock(p_old_name);
    /* UNCHECKED */ t_new_name_utf8 . Lock(p_new_name);
	if (rename(*t_old_name_utf8, *t_new_name_utf8) != 0)
        return False;
    
    return True;
}

Boolean MCIPhoneSystem::CreateAlias(MCStringRef p_target, MCStringRef p_alias)
{
    MCAutoStringRefAsUTF8String t_target_utf8, t_alias_utf8;
    /* UNCHECKED */ t_target_utf8 . Lock(p_target);
    /* UNCHECKED */ t_alias_utf8 . Lock(p_alias);
	if (symlink(*t_target_utf8, *t_alias_utf8) != 0)
        return False;
    
    return True;
}

Boolean MCIPhoneSystem::ResolveAlias(MCStringRef p_target, MCStringRef& r_dest)
{
    if (!MCStringCopy(p_target, r_dest))
        return False;
    
    return True;
}

bool MCIPhoneSystem::GetCurrentFolder(MCStringRef& r_path)
{
	MCAutoPointer<char> t_folder;
	t_folder = getcwd(NULL, 0);
	if (*t_folder == nil)
		return false;
	return MCStringCreateWithCString(*t_folder, r_path);
}

Boolean MCIPhoneSystem::SetCurrentFolder(MCStringRef p_path)
{
    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path . Lock(p_path);
	if (chdir(*t_utf8_path) != 0)
        return False;
    
    return True;
}

Boolean MCIPhoneSystem::FileExists(MCStringRef p_path)
{
	struct stat t_info;
    
    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path . Lock(p_path);
	
	bool t_found;
	t_found = stat(*t_utf8_path, &t_info) == 0;
	if (t_found && !S_ISDIR(t_info.st_mode))
		return True;
	
	return False;
}

Boolean MCIPhoneSystem::FolderExists(MCStringRef p_path)
{
	struct stat t_info;
	
    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path . Lock(p_path);
	bool t_found;
	t_found = stat(*t_utf8_path, &t_info) == 0;
	if (t_found && S_ISDIR(t_info.st_mode))
		return True;
	
	return False;
}

Boolean MCIPhoneSystem::FileNotAccessible(MCStringRef p_path)
{
	struct stat t_info;
    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path . Lock(p_path);
	if (stat(*t_utf8_path, &t_info) != 0)
		return false;
	
	if (S_ISDIR(t_info . st_mode))
		return true;
	
	if ((t_info . st_mode & S_IWUSR) == 0)
		return true;
	
	return false;
}

Boolean MCIPhoneSystem::ChangePermissions(MCStringRef p_path, uint2 p_mask)
{
    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path . Lock(p_path);
	if (chmod(*t_utf8_path, p_mask) != 0)
        return False;
    
    return True;
}

uint2 MCIPhoneSystem::UMask(uint2 p_mask)
{
	return umask(p_mask);
}

IO_handle MCIPhoneSystem::OpenFile(MCStringRef p_path, intenum_t p_mode, Boolean p_map)
{
	static const char *s_modes[] = { "r", "w", "r+", "a" };
    uint1 t_mode;
    
    switch (p_mode)
    {
    case kMCOpenFileModeRead:
        t_mode = 0;
        break;
    case kMCOpenFileModeWrite:
        t_mode = 1;
        break;
    case kMCOpenFileModeUpdate:
        t_mode = 2;
        break;
    case kMCOpenFileModeAppend:
        t_mode = 3;
        break;
    }
    
    FILE *t_stream;
    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path . Lock(p_path);
    t_stream = fopen(*t_utf8_path, s_modes[t_mode]);
    
	if (t_stream == NULL && p_mode == kMCOpenFileModeUpdate)
		t_stream = fopen(*t_utf8_path, "w+");
    
    if (t_stream == NULL)
        return NULL;
    
    IO_handle t_handle;
    t_handle = new MCStdioFileHandle(t_stream);
    
    return t_handle;
}

IO_handle MCIPhoneSystem::OpenFd(uint32_t p_fd, intenum_t p_mode)
{
	static const char *s_modes[] = { "r", "w", "w" };
    
    FILE *t_stream;
    t_stream = fdopen(p_fd, s_modes[p_fd]);
    
    if (t_stream == NULL)
        return NULL;
    
    IO_handle t_handle;
    
    // MM-2012-11-22: [[ Bug 10540 ]] - For iOS 6, use MCStdioFileDescriptorHandle for stdio streams.
    //  This just wraps NSLog for output.  No input supported.
    if (MCmajorosversion < 600)
        t_handle = new MCStdioFileHandle(t_stream);
    else
        t_handle = new MCStdioFileDescriptorHandle();
    
    return t_handle;
}

IO_handle MCIPhoneSystem::OpenDevice(MCStringRef p_path, intenum_t p_mode)
{
	return NULL;
}

bool MCIPhoneSystem::GetTemporaryFileName(MCStringRef& r_tmp_name)
{
	return MCStringCreateWithCString(tmpnam(NULL), r_tmp_name);
}

Boolean MCIPhoneSystem::GetStandardFolder(MCNameRef p_type, MCStringRef& r_folder)
{
	MCAutoStringRef t_path;
	
	if (MCNameIsEqualToCaseless(p_type, MCN_temporary))
	{
        MCAutoStringRef t_temp;
        MCStringCreateWithCFStringRef((CFStringRef)NSTemporaryDirectory() , &t_temp);
		
		// MW-2012-09-18: [[ Bug 10279 ]] Remove trailing slash, if any.
		// MW-2012-10-04: [[ Bug 10435 ]] Actually use a NUL character, rather than a '0'!
		if (MCStringEndsWith(*t_temp, MCSTR("/"), kMCCompareExact))
			/* UNCHECKED */ MCStringCopySubstring(*t_temp, MCRangeMake(0, MCStringGetLength(*t_temp) - 1), &t_path);
        else
            /* UNCHECKED */ MCStringCopy(*t_temp, &t_path);
	}
	else if (MCNameIsEqualToCaseless(p_type, MCN_documents))
	{
		NSArray *t_paths;
		t_paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        MCStringCreateWithCFStringRef((CFStringRef)[t_paths objectAtIndex: 0] , &t_path);
	}
	else if (MCNameIsEqualToCaseless(p_type, MCN_home))
	{
        MCStringCreateWithCFStringRef((CFStringRef)NSHomeDirectory() , &t_path);
	}
	else if (MCStringIsEqualToCString(MCNameGetString(p_type), "cache", kMCCompareCaseless))
	{
		NSArray *t_paths;
        t_paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
		MCStringCreateWithCFStringRef((CFStringRef)[t_paths objectAtIndex: 0] , &t_path);
	}
    // SN-2015-04-16: [[ Bug 14295 ]] The resources folder on Mobile is the same
    //   as the engine folder.
    else if (MCNameIsEqualToCaseless(p_type, MCN_engine)
             || MCNameIsEqualToCaseless(p_type, MCN_resources))
	{
		extern MCStringRef MCcmd;
        uindex_t t_index;
        t_index = MCStringGetLength(MCcmd);
        /* UNCHECKED */ MCStringLastIndexOfChar(MCcmd, '/', t_index, kMCCompareExact, t_index);
        /* UNCHECKED */ MCStringCopySubstring(MCcmd, MCRangeMake(0, t_index), &t_path);
                    
	}
	else if (MCStringIsEqualToCString(MCNameGetString(p_type), "library", kMCCompareCaseless))
	{
		NSArray *t_paths;
		t_paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
		MCStringCreateWithCFStringRef((CFStringRef)[t_paths objectAtIndex: 0] , &t_path);

	}
    
    if (*t_path == nil)
        return False;
    
    r_folder = MCValueRetain(*t_path);
    return True;
}

////

bool MCIPhoneSystem::LongFilePath(MCStringRef p_path, MCStringRef& r_long_path)
{
	return MCStringCopy(p_path, r_long_path);
}

bool MCIPhoneSystem::ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path)
{
	return MCStringCopy(p_path, r_short_path);
}

bool MCIPhoneSystem::PathToNative(MCStringRef p_path, MCStringRef& r_native)
{
	return MCStringCopy(p_path, r_native);
}

bool MCIPhoneSystem::PathFromNative(MCStringRef p_native, MCStringRef& r_path)
{
	return MCStringCopy(p_native, r_path);
}

bool MCIPhoneSystem::ResolvePath(MCStringRef p_path, MCStringRef& r_resolved)
{
    char *t_absolute_path;
	if (MCStringGetCharAtIndex(p_path, 0) != '/')
	{
		MCAutoStringRef t_folder;
		if (!GetCurrentFolder(&t_folder))
			return false;
        
		MCAutoStringRef t_resolved;
		if (!MCStringMutableCopy(*t_folder, &t_resolved) ||
			!MCStringAppendChar(*t_resolved, '/') ||
			!MCStringAppend(*t_resolved, p_path))
			return false;
        
		return MCStringCopy(*t_resolved, r_resolved);
	}
	else
		return MCStringCopy(p_path, r_resolved);
}

bool MCIPhoneSystem::ListFolderEntries(MCStringRef p_folder, MCSystemListFolderEntriesCallback p_callback, void *p_context)
{
	MCAutoStringRefAsUTF8String t_path;
	if (p_folder == nil)
	  /* UNCHECKED */ t_path . Lock(MCSTR("."));
	else
	  /* UNCHECKED */ t_path . Lock(p_folder);

	DIR *t_dir;
	t_dir = opendir(*t_path);
	if (t_dir == NULL)
		return false;
	
	MCSystemFolderEntry t_entry;
	memset(&t_entry, 0, sizeof(MCSystemFolderEntry));
	
	/* For each directory entry, we need to construct a path that can
	 * be passed to stat(2).  Allocate a buffer large enough for the
	 * path, a path separator character, and any possible filename. */
	size_t t_path_len = strlen(*t_path);
	size_t t_entry_path_len = t_path_len + 1 + NAME_MAX;
	char *t_entry_path = new char[t_entry_path_len + 1];
	strcpy (t_entry_path, *t_path);
	if ((*t_path)[t_path_len - 1] != '/')
	{
		strcat (t_entry_path, "/");
		++t_path_len;
	}

	bool t_success;
	t_success = true;
	while(t_success)
	{
		struct dirent *t_dir_entry;
		t_dir_entry = readdir(t_dir);
		if (t_dir_entry == NULL)
			break;
		
		if (strcmp(t_dir_entry -> d_name, ".") == 0)
			continue;
		
		/* Truncate the directory entry path buffer to the path
		 * separator. */
		t_entry_path[t_path_len] = 0;
		strcat (t_entry_path, t_dir_entry->d_name);

		struct stat t_stat;
		stat(t_entry_path, &t_stat);
                
        MCStringRef t_unicode_str;
        MCStringCreateWithBytes((byte_t*)t_dir_entry -> d_name, strlen(t_dir_entry -> d_name), kMCStringEncodingUTF8, false, t_unicode_str);
		
		t_entry . name = t_unicode_str;
		t_entry . data_size = t_stat . st_size;
		t_entry . resource_size = 0;
		t_entry . modification_time = t_stat . st_mtime;
		t_entry . access_time = t_stat . st_atime;
		t_entry . user_id = t_stat . st_uid;
		t_entry . group_id = t_stat . st_gid;
		t_entry . permissions = t_stat . st_mode & 0777;
		t_entry . is_folder = S_ISDIR(t_stat . st_mode);
		
		t_success = p_callback(p_context, &t_entry);
        
        MCValueRelease(t_unicode_str);
	}
	
	delete t_entry_path;
	closedir(t_dir);
	
	return t_success;
}

bool MCIPhoneSystem::Shell(MCStringRef filename, MCDataRef& r_data, int& r_retcode)
{
	int t_to_parent[2];
	pid_t t_pid;
	if (pipe(t_to_parent) == 0)
	{
		int t_to_child[2];
		if (pipe(t_to_child) == 0)
		{
			t_pid = fork();
			if (t_pid == 0)
			{
				// CHILD PROCESS SIDE
				
				// Close the writing side of the pipe <parent -> child>
				close(t_to_child[1]);
				// Close the child's stdin
				close(0);
				// Move the read side of the parent->child pipe to stdin
				dup(t_to_child[0]);
				close(t_to_child[0]);
				
				// Close the reading side of the pipe <child -> parent>
				close(t_to_parent[0]);
				// Close child's stdout
				close(1);
				// Copy the writing side of the pipe <child -> parent> to stdout
				dup(t_to_parent[1]);
				
				// Close child's stderr
				close(2);
				// Move the writing side of the pipe <child -> parent> to stderr
				dup(t_to_parent[1]);
				close(t_to_parent[1]);
				
				// Launch the standard 'sh' shell processor
				execl("/bin/sh", "/bin/sh", "-s", NULL);
				_exit(-1);
			}
			
			// ORIGINAL PROCESS SIDE
			
			// Close the reading side of the pipe <parent -> child>
			close(t_to_child[0]);
			// Write the command to it
            MCAutoStringRefAsUTF8String t_mccmd_utf8;
            t_mccmd_utf8 . Lock(MCcmd);
			write(t_to_child[1], *t_mccmd_utf8, t_mccmd_utf8.Size());
			write(t_to_child[1], "\n", 1);
			
			// Close the writing side of the pipe <parent -> child>
			close(t_to_child[1]);
			
			// Close the writing side of the pipe <child -> parent>
			close(t_to_parent[1]);
			
			// Make the reading side of the pipe <child -> parent> non-blocking
			fcntl(t_to_parent[0], F_SETFL, (fcntl(t_to_parent[0], F_GETFL, 0) & O_APPEND) | O_NONBLOCK);
		}
		else
		{
			close(t_to_child[0]);
			close(t_to_child[1]);
			return false;
		}
	}
	else
		return false;
	
	void *t_data;
	t_data = NULL;
	
	uint32_t t_length;
	t_length = 0;
	
	uint32_t t_capacity;
	t_capacity = 0;
	
	bool t_success;
	t_success = true;
	for(;;)
	{
		int t_available;
		t_available = 0;
		
		ioctl(t_to_parent[0], FIONREAD, (char *)&t_available);
		
		t_available += 16384;
		if (t_length + t_available > t_capacity)
		{
			t_capacity = t_length + t_available;
			
			void *t_new_data;
			t_new_data = realloc(t_data, t_capacity);
			if (t_new_data == NULL)
			{
				t_success = false;
				break;
			}
			
			t_data = t_new_data;
		}
		
		errno = 0;
		
		int t_read;
		t_read = read(t_to_parent[0], (char *)t_data + t_length, t_available);
		if (t_read <= 0)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
				break;
			
			pollfd t_poll_fd;
			t_poll_fd . fd = t_to_parent[0];
			t_poll_fd . events = POLLIN;
			t_poll_fd . revents = 0;
			
			int t_result;
			t_result = poll(&t_poll_fd, 1, -1);
			if (t_result != 1)
				break;
		}
		else
			t_length += t_read;
	}
	
	close(t_to_parent[0]);
	
	pid_t t_wait_result;
	int t_wait_stat;
	t_wait_result = waitpid(t_pid, &t_wait_stat, WNOHANG);
	if (t_wait_result == 0)
	{
		kill(t_pid, SIGKILL);
		waitpid(t_pid, &t_wait_stat, 0);
	}
	else
		t_wait_stat = 0;
	
	if (t_success)
	{
		/* UNCHECKED */ MCDataCreateWithBytesAndRelease((byte_t *)realloc(t_data, t_length), t_length, r_data);
		
		r_retcode = WEXITSTATUS(t_wait_stat);
	}
	else
	{
		if (t_data != NULL)
			free(t_data);
	}
	
	return t_success;
}

void MCIPhoneSystem::Debug(MCStringRef p_string)
{
    // MM-2012-09-07: [[ Bug 10320 ]] put does not write to console on Mountain Lion
    // AL-2014-03-25: [[ Bug 11985 ]] NSStrings created with stringWithMCStringRef are autoreleased.
    NSString *t_msg;
    t_msg = MCStringConvertToAutoreleasedNSString(p_string);
    NSLog(@"%@", t_msg);
}

int MCIPhoneSystem::GetErrno(void)
{
    return errno;
}

void MCIPhoneSystem::SetErrno(int p_errno)
{
    errno = p_errno;
}

//////////////////

bool MCIPhoneSystem::Initialize(void)
{
    IO_stdin = OpenFd(0, kMCOpenFileModeRead);
    IO_stdout = OpenFd(1, kMCOpenFileModeWrite);
    IO_stderr = OpenFd(2, kMCOpenFileModeWrite);
    
    // Initialize our case mapping tables
    
    MCuppercasingtable = new uint1[256];
    for(uint4 i = 0; i < 256; ++i)
        MCuppercasingtable[i] = (uint1)toupper((uint1)i);
    
    MClowercasingtable = new uint1[256];
    for(uint4 i = 0; i < 256; ++i)
        MClowercasingtable[i] = (uint1)tolower((uint1)i);
    
	return true;
}

void MCIPhoneSystem::Finalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////

real8 MCIPhoneSystem::GetFreeDiskSpace()
{
    return 0.0;
}

Boolean MCIPhoneSystem::GetDevices(MCStringRef &r_devices)
{
    return False;
}

Boolean MCIPhoneSystem::GetDrives(MCStringRef& r_devices)
{
    return False;
}

void MCIPhoneSystem::CheckProcesses(void)
{
    return;
}

uint32_t MCIPhoneSystem::GetSystemError(void)
{
    return errno;
}

bool MCIPhoneSystem::StartProcess(MCNameRef p_name, MCStringRef p_doc, intenum_t p_mode, Boolean p_elevated)
{
    return false;
}

void MCIPhoneSystem::CloseProcess(uint2 p_index)
{
    return;
}

void MCIPhoneSystem::Kill(int4 p_pid, int4 p_sig)
{
    return;
}

void MCIPhoneSystem::KillAll()
{
    return;
}

Boolean MCIPhoneSystem::Poll(real8 p_delay, int p_fd)
{
    return False;
}

Boolean MCIPhoneSystem::IsInteractiveConsole(int p_fd)
{
    return False;
}

void MCIPhoneSystem::LaunchDocument(MCStringRef p_document)
{
    return;
}

void MCIPhoneSystem::LaunchUrl(MCStringRef p_url)
{
    // AL-2014-06-26: [[ Bug 12700 ]] Implement launch url
	if (!MCSystemLaunchUrl(p_url))
        MCresult -> sets("no association");
}

void MCIPhoneSystem::DoAlternateLanguage(MCStringRef p_script, MCStringRef p_language)
{
    return;
}

bool MCIPhoneSystem::AlternateLanguages(MCListRef &r_list)
{
    return False;
}

bool MCIPhoneSystem::GetDNSservers(MCListRef &r_list)
{
    return False;
}

void MCIPhoneSystem::ShowMessageDialog(MCStringRef p_title,
                                       MCStringRef p_message)
{
    if (MCscreen == nil)
        return;
    
    MCscreen -> popupanswerdialog(nil, 0, 0, p_title, p_message, true);
}

//////////////////

MCSystemInterface *MCMobileCreateIPhoneSystem(void)
{
	return new MCIPhoneSystem;
}

//////////////////

