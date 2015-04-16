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

#include "system.h"
#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "globals.h"

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

extern "C" void *load_module(const char *);
extern "C" void *resolve_symbol(void *, const char *);

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

void *resolve_symbol(void *p_module, const char *p_symbol)
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
	static MCStdioFileHandle *Open(const char *p_path, const char *p_mode)
	{
		FILE *t_stream;
		t_stream = fopen(p_path, p_mode);
		if (t_stream == NULL)
			return NULL;
		
		MCStdioFileHandle *t_handle;
		t_handle = new MCStdioFileHandle;
		t_handle -> m_stream = t_stream;
		
		return t_handle;
	}

    static MCStdioFileHandle *OpenFd(int fd, const char *p_mode)
	{
		FILE *t_stream;
		t_stream = fdopen(fd, p_mode);
		if (t_stream == NULL)
			return NULL;
		
		MCStdioFileHandle *t_handle;
		t_handle = new MCStdioFileHandle;
		t_handle -> m_stream = t_stream;
		
		return t_handle;
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
	
	virtual bool Write(const void *p_buffer, uint32_t p_length, uint32_t& r_written)
	{
		size_t t_amount;
		t_amount = fwrite(p_buffer, 1, p_length, m_stream);
		r_written = t_amount;
		
		if (t_amount < p_length)
			return false;
		
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
	
	virtual int64_t GetFileSize(void)
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
    static MCStdioFileDescriptorHandle *Open(const char *p_path, const char *p_mode)
	{
		MCStdioFileDescriptorHandle *t_handle;
		t_handle = new MCStdioFileDescriptorHandle;
		return t_handle;
	}
    
	static MCStdioFileDescriptorHandle *OpenFd(int fd, const char *p_mode)
	{
		MCStdioFileDescriptorHandle *t_handle;
		t_handle = new MCStdioFileDescriptorHandle;
		return t_handle;
	}
	
	virtual void Close(void)
	{
		delete this;
	}
	
	virtual bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read)
	{
        r_read = 0;
		return true;
	}
	
	virtual bool Write(const void *p_buffer, uint32_t p_length, uint32_t& r_written)
	{
        r_written = p_length;
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
	
	virtual int64_t GetFileSize(void)
	{
        return 0;
	}
	
	virtual void *GetFilePointer(void)
	{
		return NULL;
	}
	
};

real64_t MCIPhoneSystem::GetCurrentTime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv . tv_sec + tv . tv_usec / 1000000.0;
}

uint32_t MCIPhoneSystem::GetProcessId(void)
{
	return getpid();
}

char *MCIPhoneSystem::GetVersion(void)
{
	return strclone([[[UIDevice currentDevice] systemVersion] cStringUsingEncoding:NSMacOSRomanStringEncoding]);
}

char *MCIPhoneSystem::GetMachine(void)
{
	return strclone([[[UIDevice currentDevice] model] cStringUsingEncoding:NSMacOSRomanStringEncoding]);
}

char *MCIPhoneSystem::GetProcessor(void)
{
#if defined __i386__
	return strclone("i386");
#elif defined __amd64__
    return strclone("x86_64");
#elif defined __arm64__
    return strclone("arm64");
#else
	return strclone("ARM");
#endif
}

char *MCIPhoneSystem::GetAddress(void)
{
	extern char *MCcmd;
	char *t_address;
	t_address = new char[strlen(MCcmd) + strlen("iphone:") + 1];
	sprintf(t_address, "iphone:%s", MCcmd);
	return t_address;
}

void MCIPhoneSystem::Alarm(real64_t p_when)
{
}

void MCIPhoneSystem::Sleep(real64_t p_when)
{
	usleep((uint32_t)(p_when * 1000000.0));
}

void MCIPhoneSystem::SetEnv(const char *name, const char *value)
{
	setenv(name, value, 1);
}

char *MCIPhoneSystem::GetEnv(const char *name)
{
	return getenv(name);
}

bool MCIPhoneSystem::CreateFolder(const char *p_path)
{
	return mkdir(p_path, 0777) == 0;
}

bool MCIPhoneSystem::DeleteFolder(const char *p_path)
{
	return rmdir(p_path) == 0;
}

bool MCIPhoneSystem::DeleteFile(const char *p_path)
{
	return unlink(p_path) == 0;
}

bool MCIPhoneSystem::RenameFileOrFolder(const char *p_old_name, const char *p_new_name)
{
	return rename(p_old_name, p_new_name) == 0;
}

bool MCIPhoneSystem::BackupFile(const char *p_old_name, const char *p_new_name)
{
	return rename(p_old_name, p_new_name) == 0;
}

bool MCIPhoneSystem::UnbackupFile(const char *p_old_name, const char *p_new_name)
{
	return rename(p_old_name, p_new_name) == 0;
}

bool MCIPhoneSystem::CreateAlias(const char *p_target, const char *p_alias)
{
	return symlink(p_target, p_alias) == 0;
}

char *MCIPhoneSystem::ResolveAlias(const char *p_target)
{
	return strdup(p_target);
}

char *MCIPhoneSystem::GetCurrentFolder(void)
{
	return getcwd(NULL, 0);
}

bool MCIPhoneSystem::SetCurrentFolder(const char *p_path)
{
	return chdir(p_path) == 0;
}

bool MCIPhoneSystem::FileExists(const char *p_path) 
{
	struct stat t_info;
	
	bool t_found;
	t_found = stat(p_path, &t_info) == 0;
	if (t_found && (t_info.st_mode & S_IFDIR) == 0)
		return true;
	
	return false;
}

bool MCIPhoneSystem::FolderExists(const char *p_path)
{
	struct stat t_info;
	
	bool t_found;
	t_found = stat(p_path, &t_info) == 0;
	if (t_found && (t_info.st_mode & S_IFDIR) != 0)
		return true;
	
	return false;
}

bool MCIPhoneSystem::FileNotAccessible(const char *p_path)
{
	struct stat t_info;
	if (stat(p_path, &t_info) != 0)
		return false;
	
	if ((t_info . st_mode & S_IFDIR) != 0)
		return true;
	
	if ((t_info . st_mode & S_IWUSR) == 0)
		return true;
	
	return false;
}

bool MCIPhoneSystem::ChangePermissions(const char *p_path, uint2 p_mask)
{
	return chmod(p_path, p_mask) == 0;
}

uint2 MCIPhoneSystem::UMask(uint2 p_mask)
{
	return umask(p_mask);
}

MCSystemFileHandle *MCIPhoneSystem::OpenFile(const char *p_path, uint32_t p_mode, bool p_map)
{
	static const char *s_modes[] = { "r", "w", "r+", "a" };

	MCSystemFileHandle *t_handle;
	t_handle = MCStdioFileHandle::Open(p_path, s_modes[p_mode & 0xff]);
	if (t_handle == NULL && p_mode == kMCSystemFileModeUpdate)
		t_handle = MCStdioFileHandle::Open(p_path, "w+");
	
	return t_handle;
}

MCSystemFileHandle *MCIPhoneSystem::OpenStdFile(uint32_t i)
{
	static const char *s_modes[] = { "r", "w", "w" };
    // MM-2012-11-22: [[ Bug 10540 ]] - For iOS 6, use MCStdioFileDescriptorHandle for stdio streams.
    //  This just wraps NSLog for output.  No input supported.
    if (MCmajorosversion < 600)
        return MCStdioFileHandle::OpenFd(i, s_modes[i]);
    else
        return MCStdioFileDescriptorHandle::OpenFd(i, s_modes[i]);
}

MCSystemFileHandle *MCIPhoneSystem::OpenDevice(const char *p_path, uint32_t p_mode, const char *p_control_string)
{
	return NULL;
}

char *MCIPhoneSystem::GetTemporaryFileName(void)
{
	return strdup(tmpnam(NULL));
}

char *MCIPhoneSystem::GetStandardFolder(const char *p_folder)
{
	char *t_path;
	t_path = nil;
	if (strcasecmp(p_folder, "temporary") == 0)
	{
		t_path = strdup([NSTemporaryDirectory() cStringUsingEncoding: NSMacOSRomanStringEncoding]);
		
		// MW-2012-09-18: [[ Bug 10279 ]] Remove trailing slash, if any.
		// MW-2012-10-04: [[ Bug 10435 ]] Actually use a NUL character, rather than a '0'!
		if (t_path[strlen(t_path) - 1] == '/')
			t_path[strlen(t_path) - 1] = '\0';
	}
	else if (strcasecmp(p_folder, "documents") == 0)
	{
		NSArray *t_paths;
		t_paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		t_path = strdup([[t_paths objectAtIndex: 0] cString]);
	}
	else if (strcasecmp(p_folder, "home") == 0)
	{
		t_path = strdup([NSHomeDirectory() cStringUsingEncoding: NSMacOSRomanStringEncoding]);
	}
	else if (strcasecmp(p_folder, "cache") == 0)
	{
		NSArray *t_paths;
		t_paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
		t_path = strdup([[t_paths objectAtIndex: 0] cString]);
    }
    // SN-2015-04-16: [[ Bug 14295 ]] The resources folder on Mobile is the same
    //   as the engine folder.
	else if (strcasecmp(p_folder, "engine") == 0 ||
            strcasecmp(p_folder, "resources") == 0)
	{
		extern char *MCcmd;
		t_path = my_strndup(MCcmd, strrchr(MCcmd, '/') - MCcmd);
	}
	else if (strcasecmp(p_folder, "library") == 0)
	{
		NSArray *t_paths;
		t_paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
		t_path = strdup([[t_paths objectAtIndex: 0] cString]);
	}
	return t_path;
}

//////////

void *MCIPhoneSystem::LoadModule(const char *p_path)
{
	void *t_module;
	t_module = load_module(p_path);
	if (t_module != NULL)
		return t_module;
	
	char *t_path;
	t_path = ResolveNativePath(p_path);
	if (t_path != NULL)
		t_module = dlopen(t_path, RTLD_LAZY);
	
	delete t_path;
	
	return t_module;
}

void *MCIPhoneSystem::ResolveModuleSymbol(void *p_module, const char *p_symbol)
{
	if (is_static_module(p_module))
		return resolve_symbol(p_module, p_symbol);

	return dlsym(p_module, p_symbol);
}

void MCIPhoneSystem::UnloadModule(void *p_module)
{
	if (is_static_module(p_module))
		return;
		
	dlclose(p_module);
}

////

char *MCIPhoneSystem::LongFilePath(const char *p_path)
{
	return strclone(p_path);
}

char *MCIPhoneSystem::ShortFilePath(const char *p_path)
{
	return strclone(p_path);
}

char *MCIPhoneSystem::PathToNative(const char *p_path)
{
	return strdup(p_path);
}

char *MCIPhoneSystem::PathFromNative(const char *p_path)
{
	return strdup(p_path);
}

char *MCIPhoneSystem::ResolvePath(const char *p_path)
{
	return ResolveNativePath(p_path);
}

char *MCIPhoneSystem::ResolveNativePath(const char *p_path)
{
	char *t_absolute_path;
	if (p_path[0] != '/')
	{
		char *t_folder;
		t_folder = GetCurrentFolder();
		t_absolute_path = new char[strlen(t_folder) + strlen(p_path) + 2];
		strcpy(t_absolute_path, t_folder);
		strcat(t_absolute_path, "/");
		strcat(t_absolute_path, p_path);
		
		// MW-2011-07-04: GetCurrentFolder() returns a string that needs to be
		//   freed.
		free(t_folder);
	}
	else
		t_absolute_path = strdup(p_path);
	
	return t_absolute_path;
}


bool MCIPhoneSystem::ListFolderEntries(MCSystemListFolderEntriesCallback p_callback, void *p_context)
{
	DIR *t_dir;
	t_dir = opendir(".");
	if (t_dir == NULL)
		return false;
	
	MCSystemFolderEntry t_entry;
	memset(&t_entry, 0, sizeof(MCSystemFolderEntry));
	
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
		
		struct stat t_stat;
		stat(t_dir_entry -> d_name, &t_stat);
		
		t_entry . name = t_dir_entry -> d_name;
		t_entry . data_size = t_stat . st_size;
		t_entry . resource_size = 0;
		t_entry . modification_time = t_stat . st_mtime;
		t_entry . access_time = t_stat . st_atime;
		t_entry . user_id = t_stat . st_uid;
		t_entry . group_id = t_stat . st_gid;
		t_entry . permissions = t_stat . st_mode & 0777;
		t_entry . is_folder = (t_stat . st_mode & S_IFDIR) != 0;
		
		t_success = p_callback(p_context, &t_entry);
	}
	
	closedir(t_dir);
	
	return t_success;
}

bool MCIPhoneSystem::Shell(const char *p_cmd, uint32_t p_cmd_length, void*& r_data, uint32_t& r_data_length, int& r_retcode)
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
			write(t_to_child[1], p_cmd, p_cmd_length);
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
		r_data = realloc(t_data, t_length);
		r_data_length = t_length;
		r_retcode = WEXITSTATUS(t_wait_stat);
	}
	else
	{
		if (t_data != NULL)
			free(t_data);
	}
	
	return t_success;
}

char *MCIPhoneSystem::GetHostName(void)
{
	char t_hostname[256];
	gethostname(t_hostname, 256);
	return strdup(t_hostname);
}

bool MCIPhoneSystem::HostNameToAddress(const char *p_hostname, MCSystemHostResolveCallback p_callback, void *p_context)
{
	struct hostent *he;
	he = gethostbyname(p_hostname);
	if (he == NULL)
		return false;
	
	struct in_addr **ptr;
	ptr = (struct in_addr **)he -> h_addr_list;
	
	for(uint32_t i = 0; ptr[i] != NULL; i++)
		if (!p_callback(p_context, inet_ntoa(*ptr[i])))
			return false;
	
	return true;
}

bool MCIPhoneSystem::AddressToHostName(const char *p_address, MCSystemHostResolveCallback p_callback, void *p_context)
{
	struct in_addr addr;
	if (!inet_aton(p_address, &addr))
		return false;
		
	struct hostent *he;
	he = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
	if (he == NULL)
		return false;
	
	return p_callback(p_context, he -> h_name);
}

void MCIPhoneSystem::Debug(const char *p_msg)
{
    // MM-2012-09-07: [[ Bug 10320 ]] put does not write to console on Mountain Lion
    NSString *t_msg;
    t_msg = [[NSString alloc] initWithCString: p_msg encoding: NSMacOSRomanStringEncoding];
    NSLog(@"%@", t_msg);
    [t_msg release];
}

//////////////////

bool MCIPhoneSystem::Initialize(void)
{
	return true;
}

void MCIPhoneSystem::Finalize(void)
{
}

//////////////////

MCSystemInterface *MCMobileCreateSystem(void)
{
	return new MCIPhoneSystem;
}

//////////////////

// MW-2013-05-21: [[ RandomBytes ]] System function for random bytes on iOS.
bool MCS_random_bytes(size_t p_count, void* p_buffer)
{
	// IM-2014-04-16: [[ Bug 11860 ]] SecRandomCopyBytes returns 0 on success
	return SecRandomCopyBytes(kSecRandomDefault, p_count, (uint8_t *)p_buffer) == 0;
}

//////////////////

extern "C" void *IOS_LoadModule(const char *name);
extern "C" void *IOS_ResolveSymbol(void *module, const char *name);

// MW-2013-10-08: [[ LibOpenSSL101e ]] This functions are used by the stubs to load
//   modules / resolve symbols.
void *IOS_LoadModule(const char *name)
{
	return MCsystem -> LoadModule(name);
}

void *IOS_ResolveSymbol(void *module, const char *name)
{
	return MCsystem -> ResolveModuleSymbol(module, name);
}
