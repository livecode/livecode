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

#include "param.h"
#include "mcerror.h"

#include "util.h"
#include "object.h"
#include "stack.h"
#include "date.h"
#include "osspec.h"
#include "globals.h"
#include "text.h"

#undef isatty
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pwd.h>
#include <dirent.h>
#include <dlfcn.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <mach-o/dyld.h>

#include <CoreServices/CoreServices.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>

////////////////////////////////////////////////////////////////////////////////

extern "C" CFStringRef CFStringCreateWithBytesNoCopy(CFAllocatorRef alloc, const UInt8 *bytes, CFIndex numBytes, CFStringEncoding encoding, Boolean isExternalRepresentation, CFAllocatorRef contentsDeallocator);

////////////////////////////////////////////////////////////////////////////////

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
		return ftruncate(fileno(m_stream), ftell(m_stream)) == 0;
	}
	
	virtual bool Sync(void) 
	{
		int64_t t_pos;
		t_pos = ftello(m_stream);
		return fseeko(m_stream, t_pos, SEEK_SET) == 0;
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
		return ftello(m_stream);
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
	
	FILE *GetStream(void)
	{
		return m_stream;
	}
    
    virtual bool TakeBuffer(void*& r_buffer, size_t& r_length)
    {
        return false;
    }
    
    virtual bool IsExhausted(void)
    {
        return feof(m_stream) == 1;
    }
	
private:
	FILE *m_stream;
};

////////////////////////////////////////////////////////////////////////////////

static Boolean do_backup(const char *, const char *);
static Boolean do_unbackup(MCStringRef , MCStringRef);
static Boolean do_createalias(const char *, const char *);
static char *do_resolvealias(const char *);
static char *do_getspecialfolder(const char *);
static char *do_tmpnam(void);

#ifndef __MAC_10_8
static char *strndup(const char *s, uint32_t l)
{
	char *r;
	r = new char[l + 1];
	strncpy(r, s, l);
	return r;
}
#endif

static struct { uint1 charset; CFStringEncoding encoding; } s_encoding_map[] =
{
	{ LCH_ENGLISH, kCFStringEncodingMacRoman },
	{ LCH_ROMAN, kCFStringEncodingMacRoman },
	{ LCH_JAPANESE, kCFStringEncodingMacJapanese },
	{ LCH_CHINESE, kCFStringEncodingMacChineseTrad },
	{ LCH_RUSSIAN, kCFStringEncodingMacCyrillic },
	{ LCH_TURKISH, kCFStringEncodingMacCyrillic },
	{ LCH_BULGARIAN, kCFStringEncodingMacCyrillic },
	{ LCH_UKRAINIAN, kCFStringEncodingMacCyrillic },
	{ LCH_ARABIC, kCFStringEncodingMacArabic },
	{ LCH_HEBREW, kCFStringEncodingMacHebrew },
	{ LCH_GREEK, kCFStringEncodingMacGreek },
	{ LCH_KOREAN, kCFStringEncodingMacKorean },
	{ LCH_POLISH, kCFStringEncodingMacCentralEurRoman },
	{ LCH_VIETNAMESE, kCFStringEncodingMacVietnamese },
	{ LCH_LITHUANIAN, kCFStringEncodingMacCentralEurRoman },
	{ LCH_THAI, kCFStringEncodingMacThai },
	{ LCH_SIMPLE_CHINESE, kCFStringEncodingMacChineseSimp },
#ifdef __LITTLE_ENDIAN__
	{ LCH_UNICODE, kCFStringEncodingUTF16LE }
#else
	{ LCH_UNICODE, kCFStringEncodingUTF16BE }
#endif
};

static CFStringEncoding lookup_encoding(uint1 p_charset)
{
	for(uint32_t i = 0; i < sizeof(s_encoding_map) / sizeof(s_encoding_map[0]); i++)
		if (s_encoding_map[i] . charset == p_charset)
			return s_encoding_map[i] . encoding;
	return kCFStringEncodingMacRoman;
}

////////////////////////////////////////////////////////////////////////////////
#ifdef LEGACY_SERVER
struct MCMacSystem: public MCSystemInterface
{
	virtual real64_t GetCurrentTime(void)
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return tv . tv_sec + tv . tv_usec / 1000000.0;
	}
	
	virtual uint32_t GetProcessId(void)
	{
		return getpid();
	}
	
	bool GetVersion(MCStringRef& r_string)
	{
		SInt32 t_major, t_minor, t_bugfix;
		Gestalt(gestaltSystemVersionMajor, &t_major);
		Gestalt(gestaltSystemVersionMinor, &t_minor);
		Gestalt(gestaltSystemVersionBugFix, &t_bugfix);
		return MCStringFormat(r_string, "%d.%d.%d", t_major, t_minor, t_bugfix);
	}
	
	virtual bool GetMachine(MCStringRef& r_string)
	{
		return MCStringCopy(MCNameGetString(MCN_unknown), r_string);
	}
	
	virtual MCNameRef GetProcessor(void)
	{
#ifdef __LITTLE_ENDIAN__
		return MCN_x86;
#else
		return MCN_motorola_powerpc;
#endif
	}
	
	virtual void GetAddress(MCStringRef& r_address)
	{
		extern MCStringRef MCcmd;	
		utsname u;
		uname(&u);
		MCStringFormat(r_address, "%s:%s", u.nodename, MCStringGetCString(MCcmd));
	}
	
	virtual void Alarm(real64_t p_when)
	{
	}
	
	virtual void Sleep(real64_t p_when)
	{
		usleep((uint32_t)(p_when * 1000000.0));
	}
	
	virtual void Debug(const char *message)
	{
	}
	
	virtual void SetEnv(MCStringRef p_name, MCStringRef p_value)
	{
        setenv(MCStringGetCString(p_name), MCStringGetCString(p_value), 1);
	}
	
	virtual void GetEnv(MCStringRef name, MCStringRef &r_env)
	{
		/* UNCHECKED */ MCStringCreateWithCString(getenv(MCStringGetCString(name)), r_env);
	}
	
	virtual bool CreateFolder(MCStringRef p_path)
	{
		return mkdir(MCStringGetCString(p_path), 0777) == 0;
	}
	
	virtual bool DeleteFolder(MCStringRef p_path)
	{
		return rmdir(MCStringGetCString(p_path)) == 0;
	}
	
	virtual bool DeleteFile(MCStringRef p_path)
	{
		return unlink(MCStringGetCString(p_path)) == 0;
	}
	
	virtual bool RenameFileOrFolder(MCStringRef p_old_name, MCStringRef p_new_name)
	{
		return rename(MCStringGetCString(p_old_name), MCStringGetCString(p_new_name)) == 0;
	}
	
	virtual bool BackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
	{
		return do_backup(MCStringGetCString(p_old_name), MCStringGetCString(p_new_name)) == True;
	}
	
	virtual bool UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
	{
		return do_unbackup(p_old_name, p_new_name) == True;
	}
	
	virtual bool CreateAlias(MCStringRef p_target, MCStringRef p_alias)
	{
		return do_createalias(MCStringGetCString(p_target), MCStringGetCString(p_alias)) == True;
	}
	
	virtual void ResolveAlias(MCStringRef p_target, MCStringRef& r_dest)
	{
		 /* UNCHECKED */ MCStringCreateWithCString(do_resolvealias(MCStringGetCString(p_target)), r_dest);
	}
	
	virtual bool GetCurrentFolder(MCStringRef& r_path)
	{
		MCAutoPointer<char> t_folder;
		t_folder = getcwd(NULL, 0);
		if (*t_folder == nil)
			return false;
		return MCStringCreateWithCString(*t_folder, r_path);
	}
	
	virtual bool SetCurrentFolder(MCStringRef p_path)
	{
		return chdir(MCStringGetCString(p_path)) == 0;
	}
	
	virtual char *GetStandardFolder(const char *name)
	{
		return do_getspecialfolder(name);
	}
	
	virtual bool FileExists(const char *p_path) 
	{
		struct stat t_info;
		
		bool t_found;
		t_found = stat(p_path, &t_info) == 0;
		if (t_found && !S_ISDIR(t_info.st_mode))
			return true;
		
		return false;
	}
	
	virtual bool FolderExists(const char *p_path)
	{
		struct stat t_info;
		
		bool t_found;
		t_found = stat(p_path, &t_info) == 0;
		if (t_found && S_ISDIR(t_info.st_mode))
			return true;
		
		return false;
	}
	
	virtual bool FileNotAccessible(MCStringRef p_path)
	{
		struct stat t_info;
		if (stat(MCStringGetCString(p_path), &t_info) != 0)
			return false;
		
		if (S_ISDIR(t_info . st_mode))
			return true;
		
		if ((t_info . st_mode & S_IWUSR) == 0)
			return true;
		
		return false;
	}
	
	virtual bool ChangePermissions(MCStringRef p_path, uint2 p_mask)
	{
		return chmod(MCStringGetCString(p_path), p_mask) == 0;
	}
	
	virtual uint2 UMask(uint2 p_mask)
	{
		return umask(p_mask);
	}
	
	virtual MCSystemFileHandle *OpenFile(MCStringRef p_path, uint32_t p_mode, bool p_map)
	{
		static const char *s_modes[] = { "r", "w", "r+", "a" };
		const char *t_path = MCStringGetCString(p_path);
		
		MCSystemFileHandle *t_handle;
		t_handle = MCStdioFileHandle::Open(t_path, s_modes[p_mode & 0xff]);
		if (t_handle == NULL && p_mode == kMCOpenFileModeUpdate)
			t_handle = MCStdioFileHandle::Open(t_path, "w+");
		
		return t_handle;
	}
	
	virtual MCSystemFileHandle *OpenStdFile(uint32_t i)
	{
		static const char *s_modes[] = { "r", "w", "w" };
		return MCStdioFileHandle::OpenFd(i, s_modes[i]);
	}
	
	virtual MCSystemFileHandle *OpenDevice(MCStringRef p_path, uint32_t p_mode, MCStringRef p_control_string)
	{
		return NULL;
	}
	
	virtual char *GetTemporaryFileName(void)
	{
		return do_tmpnam();
	}
	
	//////////
	
	virtual void *LoadModule(MCStringRef p_path)
	{	
		MCAutoStringRef t_resolved_path;
		/* UNCHECKED */ ResolveNativePath(p_path, &t_resolved_path);
	
		void *t_result;
		t_result = dlopen(MCStringGetCString(*t_resolved_path), RTLD_LAZY);
		return t_result;
	}
	
	virtual void *ResolveModuleSymbol(MCSysModuleHandle* p_module, MCStringRef p_symbol)
	{
		return dlsym((void *)p_module, MCStringGetCString(p_symbol));
	}
	
	virtual void UnloadModule(void *p_module)
	{
		dlclose(p_module);
	}
	
	//////////
	
	virtual bool GetExecutablePath(MCStringRef& r_path)
	{
		uint32_t bufsize = 0;
		_NSGetExecutablePath(NULL, &bufsize);
		char* buf = new char[bufsize];
		if (_NSGetExecutablePath(buf, &bufsize) != 0) {
			delete buf;
			return False;
		}

		MCAutoStringRef t_path;
		MCStringCreateWithCStringAndRelease(buf, *t_path);
		return ResolvePath(*t_path, r_path);
	}

	bool PathToNative(MCStringRef p_path, MCStringRef& r_native)
	{
		CFStringRef t_cf_path;
		t_cf_path = CFStringCreateWithCString(NULL, MCStringGetCString(p_path), kCFStringEncodingMacRoman);
		
		CFIndex t_used;
		t_used = CFStringGetMaximumSizeOfFileSystemRepresentation(t_cf_path);
		
		MCAutoNativeCharArray t_native;
		if (!t_native.New(t_used))
			return false;

		CFStringGetFileSystemRepresentation(t_cf_path, (char*)t_native.Chars(), t_used);
		t_native.Shrink(MCCStringLength((char*)t_native.Chars()));
		
		return t_native.CreateStringAndRelease(r_native);
	}
	
	bool PathFromNative(MCStringRef p_native, MCStringRef& r_path)
	{
		CFStringRef t_cf_path;
		t_cf_path = CFStringCreateWithFileSystemRepresentation(NULL, MCStringGetCString(p_native));
		
		CFIndex t_used;
		CFStringGetBytes(t_cf_path, CFRangeMake(0, CFStringGetLength(t_cf_path)), kCFStringEncodingMacRoman, '?', FALSE, NULL, 0, &t_used);
		
		MCAutoNativeCharArray t_path;
		if (!t_path.New(t_used))
			return false;;

		CFStringGetBytes(t_cf_path, CFRangeMake(0, CFStringGetLength(t_cf_path)), kCFStringEncodingMacRoman, '?', FALSE, (UInt8 *)t_path.Chars(), t_used, &t_used);
		
		return t_path.CreateStringAndRelease(r_path);
	}

	bool ResolvePath(MCStringRef p_path, MCStringRef& r_resolved)
	{
		MCAutoStringRef t_native;
		return PathToNative(p_path, &t_native) &&
			ResolveNativePath(*t_native, r_resolved);
	}
	
	bool ResolveNativePath(MCStringRef p_path, MCStringRef& r_resolved)
	{
		MCAutoStringRef t_tilde_path;
		if (MCStringGetCharAtIndex(p_path, 0) == '~')
		{
			uindex_t t_user_end;
			if (!MCStringFirstIndexOfChar(p_path, '/', 0, kMCStringOptionCompareExact, t_user_end))
				t_user_end = MCStringGetLength(p_path);
			
			// Prepend user name
			struct passwd *t_password;
			if (t_user_end == 1)
				t_password = getpwuid(getuid());
			else
			{
				MCAutoStringRef t_username;
				if (!MCStringCopySubstring(p_path, MCRangeMakeMinMax(1U, t_user_end), &t_username))
					return false;

				t_password = getpwnam(MCStringGetCString(*t_username));
			}
			
			if (t_password != NULL)
			{
				if (!MCStringCreateMutable(0, &t_tilde_path) ||
					!MCStringAppendNativeChars(*t_tilde_path, (const char_t *)t_password->pw_dir, MCCStringLength(t_password->pw_dir)) ||
					!MCStringAppendSubstring(*t_tilde_path, p_path, MCRangeMakeMinMax(t_user_end, MCStringGetLength(p_path))))
					return false;
			}
			else
				t_tilde_path = p_path;
		}
		else
			t_tilde_path = p_path;
		
		if (MCStringGetCharAtIndex(*t_tilde_path, 0) != '/')
		{
			MCAutoStringRef t_folder;
			if (!GetCurrentFolder(&t_folder))
				return false;

			MCAutoStringRef t_resolved;
			if (!MCStringMutableCopy(*t_folder, &t_resolved) ||
				!MCStringAppendChar(*t_resolved, '/') ||
				!MCStringAppend(*t_resolved, *t_tilde_path))
				return false;

			return MCStringCopy(*t_resolved, r_resolved);
		}
		else
			return MCStringCopy(*t_tilde_path, r_resolved);

	}
	
	bool LongFilePath(MCStringRef p_path, MCStringRef& r_long_path)
	{
		return MCStringCopy(p_path, r_long_path);
	}
	
	bool ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path)
	{
		return MCStringCopy(p_path, r_short_path);
	}
	
#define CATALOG_MAX_ENTRIES 16
	bool ListFolderEntries(MCStringRef p_folder, MCSystemListFolderEntriesCallback p_callback, void *p_context)
	{
		OSStatus t_os_status;
		
		Boolean t_is_folder;
		FSRef t_current_fsref;

		MCAutoStringRefAsUTF8String t_path;
		if (p_folder == nil)
			/* UNCHECKED */ t_path . Lock(MCSTR("."));
		else
			/* UNCHECKED */ t_path . Lock(p_folder);

		t_os_status = FSPathMakeRef((const UInt8 *) *t_path, &t_current_fsref, &t_is_folder
		if (t_os_status != noErr || !t_is_folder)
			return false;
		
		// Output the default entry
		MCSystemFolderEntry t_entry;
		memset(&t_entry, 0, sizeof(t_entry));
		t_entry . name = "..";
		t_entry . is_folder = true;
		if (!p_callback(p_context, &t_entry))
			return false;
		
		// Create the iterator, pass kFSIterateFlat to iterate over the current subtree only
		FSIterator t_catalog_iterator;
		t_os_status = FSOpenIterator(&t_current_fsref, kFSIterateFlat, &t_catalog_iterator);
		if (t_os_status != noErr)
			return false;
		
		uint4 t_entry_count;
		t_entry_count = 0;
		
		ItemCount t_max_objects, t_actual_objects;
		t_max_objects = CATALOG_MAX_ENTRIES;
		t_actual_objects = 0;
		FSCatalogInfo t_catalog_infos[CATALOG_MAX_ENTRIES];
		HFSUniStr255 t_names[CATALOG_MAX_ENTRIES];
		
		FSCatalogInfoBitmap t_info_bitmap;
		t_info_bitmap = kFSCatInfoAllDates |
			kFSCatInfoPermissions |
			kFSCatInfoUserAccess |
			kFSCatInfoFinderInfo | 
			kFSCatInfoDataSizes |
			kFSCatInfoRsrcSizes |
			kFSCatInfoNodeFlags;
		
		MCExecPoint t_tmp_context(NULL, NULL, NULL);	
		OSErr t_oserror;
		do
		{
			t_oserror = FSGetCatalogInfoBulk(t_catalog_iterator, t_max_objects, &t_actual_objects, NULL, t_info_bitmap, t_catalog_infos, NULL, NULL, t_names);
			if (t_oserror != noErr && t_oserror != errFSNoMoreItems)
			{	// clean up and exit
				FSCloseIterator(t_catalog_iterator);
				return false;
			}
			
			for(uint4 t_i = 0; t_i < (uint4)t_actual_objects; t_i++)
			{
				// Convert the name.
				char t_native_name[256];
				uint4 t_native_length;
				t_native_length = 256;
				MCS_unicodetomultibyte((const char *)t_names[t_i] . unicode, t_names[t_i] . length * 2, t_native_name, t_native_length, t_native_length, LCH_ROMAN);
				for(uint4 i = 0; i < t_native_length; ++i)
					if (t_native_name[i] == '/')
						t_native_name[i] = ':';
				t_native_name[t_native_length] = '\0';
				t_entry . name = t_native_name;
				
				// Sizes
				t_entry . data_size = t_catalog_infos[t_i] . dataLogicalSize;
				t_entry . resource_size = t_catalog_infos[t_i] . rsrcLogicalSize;
				
				// Times
				CFAbsoluteTime t_creation_time;
				UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . createDate, &t_creation_time);
				t_creation_time += kCFAbsoluteTimeIntervalSince1970;
				
				CFAbsoluteTime t_modification_time;
				UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . contentModDate, &t_modification_time);
				t_modification_time += kCFAbsoluteTimeIntervalSince1970;
				
				CFAbsoluteTime t_access_time;
				UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . accessDate, &t_access_time);
				t_access_time += kCFAbsoluteTimeIntervalSince1970;
				
				CFAbsoluteTime t_backup_time;
				if (t_catalog_infos[t_i] . backupDate . highSeconds == 0 && t_catalog_infos[t_i] . backupDate . lowSeconds == 0 && t_catalog_infos[t_i] . backupDate . fraction == 0)
					t_backup_time = 0;
				else
				{
					UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . backupDate, &t_backup_time);
					t_backup_time += kCFAbsoluteTimeIntervalSince1970;
				}
				
				t_entry . creation_time = t_creation_time;
				t_entry . modification_time = t_modification_time;
				t_entry . access_time = t_access_time;
				t_entry . backup_time = t_backup_time;
								
				// Get the permissions.
				FSPermissionInfo *t_permissions;
				t_permissions = (FSPermissionInfo *)&(t_catalog_infos[t_i] . permissions);
				
				// Get the creator/type
				if (!t_entry . is_folder)
				{
					FileInfo *t_file_info;
					t_file_info = (FileInfo *) &t_catalog_infos[t_i] . finderInfo;
					uint4 t_creator;
					t_entry . file_creator = MCSwapInt32NetworkToHost(t_file_info -> fileCreator);
					uint4 t_type;
					t_entry . file_type = MCSwapInt32NetworkToHost(t_file_info -> fileType);
				}
				else
				{
					t_entry . file_creator = '????';
					t_entry . file_type = '????';
				}

				// Determine whether its a folder
				t_entry . is_folder = (t_catalog_infos[t_i] . nodeFlags & kFSNodeIsDirectoryMask) != 0;
			
				if (!p_callback(p_context, &t_entry))
				{
					FSCloseIterator(t_catalog_iterator);
					return false;
				}
			}
			
			t_entry_count += 1;
		}
		while(t_oserror != errFSNoMoreItems);
		
		FSCloseIterator(t_catalog_iterator);
		
		return true;
	}
	
	bool Shell(MCStringRef p_cmd, MCDataRef& r_data, int& r_retcode)
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
				write(t_to_child[1], MCStringGetCString(p_cmd), MCStringGetLength(p_cmd));
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
			r_data = (MCDataRef) t_data;
			//r_data_length = t_length;
			r_retcode = WEXITSTATUS(t_wait_stat);
		}
		else
		{
			if (t_data != NULL)
				free(t_data);
		}
		
		return t_success;
	}
	
	void Sleep(uint32_t p_microseconds)
	{
		usleep(p_microseconds);
	}
	
	char *GetHostName(void)
	{
		char t_hostname[256];
		gethostname(t_hostname, 256);
		return strdup(t_hostname);
	}
	
	bool HostNameToAddress(MCStringRef p_hostname, MCSystemHostResolveCallback p_callback, void *p_context)
	{
		struct hostent *he;
		he = gethostbyname(MCStringGetCString(p_hostname));
		if (he == NULL)
			return false;
		
		struct in_addr **ptr;
		ptr = (struct in_addr **)he -> h_addr_list;
		
		for(uint32_t i = 0; ptr[i] != NULL; i++)
		{
			MCAutoStringRef t_address;
			const char *t_addr_str = inet_ntoa(*ptr[i]);
			if (!MCStringCreateWithNativeChars((const char_t*)t_addr_str, MCCStringLength(t_addr_str), &t_address))
				return false;
			if (!p_callback(p_context, *t_address))
				return false;
		}
		
		return true;
	}

	bool AddressToHostName(MCStringRef p_address, MCSystemHostResolveCallback p_callback, void *p_context)
	{
		struct in_addr addr;
		if (!inet_aton(MCStringGetCString(p_address), &addr))
			return false;
			
		struct hostent *he;
		he = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
		if (he == NULL)
			return false;
		
		MCAutoStringRef t_name;
		return MCStringCreateWithNativeChars((char_t*)he->h_name, MCCStringLength(he->h_name), &t_name) &&
			p_callback(p_context, *t_name);
	}
	
	//////////////////
	
	uint32_t TextConvert(const void *p_string, uint32_t p_string_length, void *p_buffer, uint32_t p_buffer_length, uint32_t p_from_charset, uint32_t p_to_charset)
	{
		CFStringEncoding t_from, t_to;
		t_from = lookup_encoding(p_from_charset);
		t_to = lookup_encoding(p_to_charset);
		
		CFStringRef t_string;
		if (CFStringCreateWithBytesNoCopy != nil)
			t_string = CFStringCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)p_string, p_string_length, t_from, FALSE, kCFAllocatorNull);
		else
			t_string = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)p_string, p_string_length, t_from, FALSE);
		
		CFIndex t_used;
		CFStringGetBytes(t_string, CFRangeMake(0, CFStringGetLength(t_string)), t_to, '?', FALSE, (UInt8 *)p_buffer, p_buffer_length, &t_used);
		
		CFRelease(t_string);
		
		return t_used;
	}
	
	bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
	{
		static struct { uint32_t encoding; CFStringEncoding cfencoding; } s_text_encoding_map[] =
		{
			{kMCTextEncodingSymbol,},
			{kMCTextEncodingMacRoman, kCFStringEncodingMacRoman},
			{kMCTextEncodingWindows1252, kCFStringEncodingWindowsLatin1},
			{kMCTextEncodingWindowsNative + 437, kCFStringEncodingDOSLatinUS},
			{kMCTextEncodingWindowsNative + 850, kCFStringEncodingDOSLatin1},
			{kMCTextEncodingWindowsNative + 932, kCFStringEncodingDOSJapanese},
			{kMCTextEncodingWindowsNative + 949, kCFStringEncodingDOSKorean},
			{kMCTextEncodingWindowsNative + 1361, kCFStringEncodingWindowsKoreanJohab},
			{kMCTextEncodingWindowsNative + 936, kCFStringEncodingDOSChineseSimplif},
			{kMCTextEncodingWindowsNative + 950, kCFStringEncodingDOSChineseTrad},
			{kMCTextEncodingWindowsNative + 1253, kCFStringEncodingWindowsGreek},
			{kMCTextEncodingWindowsNative + 1254, kCFStringEncodingWindowsLatin5},
			{kMCTextEncodingWindowsNative + 1258, kCFStringEncodingWindowsVietnamese},
			{kMCTextEncodingWindowsNative + 1255, kCFStringEncodingWindowsHebrew},
			{kMCTextEncodingWindowsNative + 1256, kCFStringEncodingWindowsArabic},
			{kMCTextEncodingWindowsNative + 1257, kCFStringEncodingWindowsBalticRim},
			{kMCTextEncodingWindowsNative + 1251, kCFStringEncodingWindowsCyrillic},
			{kMCTextEncodingWindowsNative + 874, kCFStringEncodingDOSThai},
			{kMCTextEncodingWindowsNative + 1250, kCFStringEncodingWindowsLatin2},
			{65001, kCFStringEncodingUTF8},
		};
		
		if (p_input_length == 0)
		{
			r_used = 0;
			return true;
		}
		
		CFStringEncoding t_encoding;
		t_encoding = kCFStringEncodingMacRoman;
		for(uint32_t i = 0; i < sizeof(s_text_encoding_map) / sizeof(s_text_encoding_map[0]); i++)
			if (s_text_encoding_map[i] . encoding == p_input_encoding)
			{
				t_encoding = s_text_encoding_map[i] . cfencoding;
				break;
			}
		
		CFStringRef t_string;
		t_string = CFStringCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)p_input, p_input_length, t_encoding, FALSE, kCFAllocatorNull);
		
		CFIndex t_length;
		t_length = CFStringGetLength(t_string);
		
		if (t_length * 2 > p_output_length)
		{
			r_used = t_length * 2;
			return false;
		}
		
		CFStringGetCharacters(t_string, CFRangeMake(0, t_length), (UniChar *)p_output);
		r_used = t_length * 2;
		return true;
	}
	
	//////////////////
	
	bool Initialize(void)
	{
		return true;
	}
	
	void Finalize(void)
	{
	}
};
#endif /* LEGACY_SERVER */

////////////////////////////////////////////////////////////////////////////////

static OSErr do_nativepathtoref(MCStringRef p_path, FSRef *r_ref)
{
	return FSPathMakeRef((const UInt8 *)MCStringGetCString(p_path), r_ref, NULL);
}

static OSErr do_nativepathtoref_and_leaf(const char *p_path, FSRef& r_ref, UniChar*& r_leaf, UniCharCount& r_leaf_length)
{
	OSErr t_error;
	t_error = noErr;
	
	char *t_native_path;
	t_native_path = strclone(p_path);
	
	char *t_native_path_leaf;
	t_native_path_leaf = strrchr(t_native_path, '/');
	if (t_native_path_leaf != NULL)
	{
		t_native_path_leaf[0] = '\0';
		t_native_path_leaf += 1;
	}
	else
		t_error = fnfErr;

	if (t_error == noErr)
		t_error = FSPathMakeRef((const UInt8 *)t_native_path, &r_ref, NULL);
	
	if (t_error == noErr)
	{
		unichar_t *t_leaf;
		t_leaf = new unichar_t[256];
		
		uint32_t t_leaf_length;
		t_leaf_length = UTF8ToUnicode(t_native_path_leaf, strlen(t_native_path_leaf), t_leaf, 256);
		
		r_leaf = (UniChar *)t_leaf;
		r_leaf_length = (UniCharCount)t_leaf_length;
	}
	
	free(t_native_path);
	
	return t_error;
}

static char *do_fsreftopath(FSRef& p_ref)
{
	char *t_path;
	t_path = new char[PATH_MAX + 1];
	FSRefMakePath(&p_ref, (UInt8 *)t_path, PATH_MAX);
	
	char *t_std_path;
	t_std_path = MCsystem -> PathFromNative(t_path);
	
	delete t_path;
	
	return t_std_path;
}

static char *do_fsreftonativepath(FSRef& p_ref)
{
	char *t_path;
	t_path = new char[PATH_MAX + 1];
	FSRefMakePath(&p_ref, (UInt8 *)t_path, PATH_MAX);
	
	char *t_native_path;
	t_native_path = strdup(t_path);
	
	delete t_path;
	
	return t_native_path;
}

static Boolean do_backup(const char *p_src_path, const char *p_dst_path)
{
	bool t_error;
	t_error = false;
	
	FSRef t_src_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		MCAutoStringRef t_src_path_str;
		/* UNCHECKED */ MCStringCreateWithCString(p_src_path, &t_src_path_str);
		t_os_error = do_nativepathtoref(*t_src_path_str, &t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	FSRef t_dst_parent_ref;
	FSRef t_dst_ref;
	UniChar *t_dst_leaf;
	t_dst_leaf = NULL;
	UniCharCount t_dst_leaf_length;
	if (!t_error)
	{
		OSErr t_os_error;
		MCAutoStringRef t_dst_path_str;
		/* UNCHECKED */ MCStringCreateWithCString(p_dst_path, &t_dst_path_str);
		t_os_error = do_nativepathtoref(*t_dst_path_str, &t_dst_ref);
		if (t_os_error == noErr)
			FSDeleteObject(&t_dst_ref);
		
		// Get the information to create the file
		t_os_error = do_nativepathtoref_and_leaf(p_dst_path, t_dst_parent_ref, t_dst_leaf, t_dst_leaf_length);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	FSCatalogInfo t_dst_catalog;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSGetCatalogInfo(&t_src_ref, kFSCatInfoFinderInfo, &t_dst_catalog, NULL, NULL, NULL);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	if (!t_error)
	{
		const char *MCfiletype_cstring = MCStringGetCString(MCfiletype);
		memcpy(&((FileInfo *) t_dst_catalog . finderInfo) -> fileType, MCfiletype_cstring + 4, 4);
		memcpy(&((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator, MCfiletype_cstring, 4);
		
		((FileInfo *) t_dst_catalog . finderInfo) -> fileType = MCSwapInt32NetworkToHost(((FileInfo *) t_dst_catalog . finderInfo) -> fileType);
		((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator = MCSwapInt32NetworkToHost(((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator);
	}	
	MCStringChar
	
	bool t_created_dst;
	t_created_dst = false;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSCreateFileUnicode(&t_dst_parent_ref, t_dst_leaf_length, t_dst_leaf, kFSCatInfoFinderInfo, &t_dst_catalog, &t_dst_ref, NULL);
		if (t_os_error == noErr)
			t_created_dst = true;
		else
			t_error = true;
	}
	
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSExchangeObjects(&t_src_ref, &t_dst_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	if (t_error && t_created_dst)
		FSDeleteObject(&t_dst_ref);
	
	if (t_dst_leaf != NULL)
		delete t_dst_leaf;
	
	if (t_error)
	{
		MCAutoStringRef t_src_path, t_dst_path;
		/* UNCHECKED */ MCStringCreateWithCString(p_src_path, &t_src_path);
		/* UNCHECKED */ MCStringCreateWithCString(p_dst_path, &t_dst_path);
		t_error = !MCsystem -> RenameFileOrFolder(*t_src_path, *t_dst_path);

	}
	
	return !t_error;
}

static Boolean do_unbackup(MCStringRef p_src_path, MCStringRef p_dst_path)
{
	bool t_error;
	t_error = false;
	
	FSRef t_src_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = do_nativepathtoref(p_src_path, &t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	FSRef t_dst_ref;
	if (!t_error)
	{		
		OSErr t_os_error;
		t_os_error = do_nativepathtoref(p_dst_path, &t_dst_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	// It appears that the source file here is the ~file, the backup file.
	// So copy it over to p_dst_path, and delete it.
	if (!t_error)
	{	
		OSErr t_os_error;
		t_os_error = FSExchangeObjects(&t_src_ref, &t_dst_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSDeleteObject(&t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	if (t_error)
	{
		t_error = !MCsystem -> RenameFileOrFolder(p_src_path, p_dst_path);
	}
	
	return !t_error;
}

static Boolean do_createalias(const char *p_source_path, const char *p_dest_path)
{
	bool t_error;
	t_error = false;
	
	// Check if the destination exists already and return an error if it does
	if (!t_error)
	{
		FSRef t_dst_ref;
		OSErr t_os_error;
		MCAutoStringRef t_dest_path;
		/* UNCHECKED */ MCStringCreateWithCString(p_dest_path, &t_dest_path);
		t_os_error = do_nativepathtoref(*t_dest_path, &t_dst_ref);
		if (t_os_error == noErr)
			return False; // we expect an error
	}
	
	FSRef t_src_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		MCAutoStringRef t_source_path;
		/* UNCHECKED */ MCStringCreateWithCString(p_source_path, &t_source_path);
		t_os_error = do_nativepathtoref(*t_source_path, &t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	FSRef t_dst_parent_ref;
	UniChar *t_dst_leaf_name;
	UniCharCount t_dst_leaf_name_length;
	t_dst_leaf_name = NULL;
	t_dst_leaf_name_length = 0;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = do_nativepathtoref_and_leaf(p_dest_path, t_dst_parent_ref, t_dst_leaf_name, t_dst_leaf_name_length);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	AliasHandle t_alias;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSNewAlias(NULL, &t_src_ref, &t_alias);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	IconRef t_src_icon;
	t_src_icon = NULL;
	if (!t_error)
	{
		OSErr t_os_error;
		SInt16 t_unused_label;
		t_os_error = GetIconRefFromFileInfo(&t_src_ref, 0, NULL, kFSCatInfoNone, NULL, kIconServicesNormalUsageFlag, &t_src_icon, &t_unused_label);
		if (t_os_error != noErr)
			t_src_icon = NULL;
	}
	
	IconFamilyHandle t_icon_family;
	t_icon_family = NULL;
	if (!t_error && t_src_icon != NULL)
	{
		OSErr t_os_error;
		IconRefToIconFamily(t_src_icon, kSelectorAllAvailableData, &t_icon_family);
	}
	
	HFSUniStr255 t_fork_name;
	if (!t_error)
		FSGetResourceForkName(&t_fork_name);
	
	FSRef t_dst_ref;
	FSSpec t_dst_spec;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSCreateResourceFile(&t_dst_parent_ref, t_dst_leaf_name_length, t_dst_leaf_name,
										  kFSCatInfoNone, NULL, t_fork_name . length, t_fork_name . unicode, &t_dst_ref, &t_dst_spec);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	ResFileRefNum t_res_file;
	bool t_res_file_opened;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSOpenResourceFile(&t_dst_ref, t_fork_name . length, t_fork_name . unicode, 3, &t_res_file);
		if (t_os_error != noErr)
			t_error = true;
		else
			t_res_file_opened = true;
	}
	
	if (!t_error)
	{
		AddResource((Handle)t_alias, rAliasType, 0, (ConstStr255Param)"");
		if (ResError() != noErr)
			t_error = true;
	}
	
	if (!t_error && t_icon_family != NULL)
		AddResource((Handle)t_icon_family, 'icns', -16496, NULL);
	
	if (t_res_file_opened)
		CloseResFile(t_res_file);
	
	if (!t_error)
	{
		FSCatalogInfo t_info;
		FSGetCatalogInfo(&t_dst_ref, kFSCatInfoFinderInfo, &t_info, NULL, NULL, NULL);
		((FileInfo *)&t_info . finderInfo) -> finderFlags |= kIsAlias;
		if (t_icon_family != NULL)
			((FileInfo *)&t_info . finderInfo) -> finderFlags |= kHasCustomIcon;
		FSSetCatalogInfo(&t_dst_ref, kFSCatInfoFinderInfo, &t_info);		
	}
	
	if (t_src_icon != NULL)
		ReleaseIconRef(t_src_icon);
	
	if (t_dst_leaf_name != NULL)
		delete t_dst_leaf_name;
	
	if (t_error)
	{
		if (t_icon_family != NULL)
			DisposeHandle((Handle)t_icon_family);
		FSDeleteObject(&t_dst_ref);
	}
	
	return !t_error;
}

static char *do_resolvealias(const char *p_path)
{
	FSRef t_fsref;
	MCAutoStringRef t_path;
	/* UNCHECKED */ MCStringCreateWithCString(p_path, &t_path);
	if (do_nativepathtoref(*t_path, &t_fsref) != noErr)
		return NULL;
	
	Boolean t_is_folder;
	Boolean t_is_alias;
	OSErr t_os_error;
	t_os_error = FSResolveAliasFile(&t_fsref, TRUE, &t_is_folder, &t_is_alias);
	if (t_os_error != noErr || !t_is_alias) 
		return false;


	return do_fsreftopath(t_fsref);
}

typedef struct
{
	const char *token;
	unsigned long macfolder;
	OSType domain;
}
sysfolders;

static sysfolders sysfolderlist[] = {
	{"Apple", 'amnu', kOnAppropriateDisk},
	{"Desktop", 'desk', kOnAppropriateDisk},
	{"Control", 'ctrl', kOnAppropriateDisk},
	{"Extension",'extn', kOnAppropriateDisk},
	{"Fonts",'font', kOnAppropriateDisk},
	{"Preferences",'pref', kUserDomain},
	{"Temporary",'temp', kUserDomain},
	{"System", 'macs', kOnAppropriateDisk},
	// TS-2007-08-20: Added to allow a common notion of "home" between all platforms
	{"Home", 'cusr', kUserDomain},
	// MW-2007-09-11: Added for uniformity across platforms
	{"Documents", 'docs', kUserDomain}
};

static char *do_getspecialfolder(const char *p_name)
{
	const char *t_error;
	t_error = NULL;
	
	FSRef t_folder_ref;
	if (t_error == NULL)
	{
		bool t_found_folder;
		t_found_folder = false;
		
		uint4 t_mac_folder;
		if (strlen(p_name) == 4)
		{
			memcpy(&t_mac_folder, p_name, 4);
			t_mac_folder = MCSwapInt32NetworkToHost(t_mac_folder);
		}
		else
			t_mac_folder = 0;
		
		OSErr t_os_error;
		uint2 t_i;
		for (t_i = 0 ; t_i < ELEMENTS(sysfolderlist); t_i++)
			if (MCCStringEqualCaseless(p_name, sysfolderlist[t_i] . token) || t_mac_folder == sysfolderlist[t_i] . macfolder)
			{
				Boolean t_create_folder;
				t_create_folder = sysfolderlist[t_i] . domain == kUserDomain ? kCreateFolder : kDontCreateFolder;
				t_os_error = FSFindFolder(sysfolderlist[t_i] . domain, sysfolderlist[t_i] . macfolder, t_create_folder, &t_folder_ref);
				if (t_os_error == noErr)
				{
					t_found_folder = true;
					break;
				}
			}
		
		if (!t_found_folder && strlen(p_name) == 4)
		{
			OSErr t_os_error;
			t_os_error = FSFindFolder(kOnAppropriateDisk, t_mac_folder, kDontCreateFolder, &t_folder_ref);
			if (t_os_error == noErr)
				t_found_folder = true;
		}
		
		if (!t_found_folder)
			return NULL;
	}
	
	char *t_folder_path;
	t_folder_path = NULL;
	if (t_error == NULL)
	{
		t_folder_path = do_fsreftopath(t_folder_ref);
		if (t_folder_path == NULL)
			return NULL;
	}
	
	return t_folder_path;
}

static char *do_tmpnam(void)
{
	char *t_path;
	t_path = NULL;
	
	FSRef t_folder_ref;
	if (FSFindFolder(kOnSystemDisk, kTemporaryFolderType, TRUE, &t_folder_ref) == noErr)
	{
		char *t_temp_file;
		t_temp_file = do_fsreftonativepath(t_folder_ref);
		MCCStringAppendFormat(t_temp_file, "/tmp.%d.XXXXXXXX", getpid());
		
		int t_fd;
		t_fd = mkstemp(t_temp_file);
		if (t_fd != -1)
		{
			close(t_fd);
			unlink(t_temp_file);
			t_path = t_temp_file;
		}
	}
	
	if (t_path == nil)
		return NULL;
	
	char *t_std_path;
	t_std_path = MCsystem -> PathFromNative(t_path);
	free(t_path);
	
	return t_std_path;
}

////////////////////////////////////////////////////////////////////////////////

MCSystemInterface *MCServerCreateMacSystem(void)
{
	return new MCMacSystem;
}
	
////////////////////////////////////////////////////////////////////////////////

bool MCS_isnan(double v)
{
	return isnan(v) != 0;
}

////////////////////////////////////////////////////////////////////////////////

//This method needs to be updated (the implementation is wrong) after fixing the server stuff (in srvposix.cpp as well)
bool MCS_get_temporary_folder(MCStringRef &r_temp_folder)
{
	bool t_success = true;

	const char *t_tmpdir = NULL;
	int32_t t_tmpdir_len = 0;
	MCAutoStringRef t_tmpdir_string;
	MCS_getenv(MCSTR("TMPDIR"), &t_tmpdir_string);

	/* UNCHECKED */ MCStringCreateWithCString(t_tmpdir, &t_tmpdir_string);
	

	if (t_tmpdir == NULL)
		t_tmpdir = "/tmp";
		
	if (t_success)
	{
		t_tmpdir_len = MCCStringLength(t_tmpdir);
		t_success = t_tmpdir_len > 0;
	}

	if (t_success)
	{
        char *t_temp_folder = strdup(MCStringGetCString(r_temp_folder));
		if (t_tmpdir[t_tmpdir_len - 1] == '/')
			t_success = MCCStringCloneSubstring(t_tmpdir, t_tmpdir_len - 1, t_temp_folder);
		else
			t_success = MCCStringClone(t_tmpdir, t_temp_folder);
	}

	return t_success;
}

bool MCS_create_temporary_file(MCStringRef p_path, MCStringRef p_prefix, IO_handle &r_file, MCStringRef &r_name)
{
	char *t_temp_file = NULL;
	if (!MCCStringFormat(t_temp_file, "%s/%sXXXXXXXX", p_path, MCStringGetCString(p_prefix)))
		return false;
	
	int t_fd;
	t_fd = mkstemp(t_temp_file);
	/* UNCHECKED */ MCStringCreateWithCString(t_temp_file, r_name);
	MCCStringFree(t_temp_file);
	if (t_fd == -1)
	{
		return false;
	}
	
    r_file = MCStdioFileHandle :: OpenFd(t_fd, "w+");
	
	return true;
}

bool MCSystemLockFile(MCSystemFileHandle *p_file, bool p_shared, bool p_wait)
{
	int t_fd = fileno(((MCStdioFileHandle*)p_file)->GetStream());
	int32_t t_op = 0;
	
	if (p_shared)
		t_op = LOCK_SH;
	else
		t_op = LOCK_EX;
	
	if (!p_wait)
		t_op |= LOCK_NB;
	
	return 0 == flock(t_fd, t_op);
}

////////////////////////////////////////////////////////////////////////////////
// MM-2013-09-13: [[ RefactorGraphics ]] Required for server font support.

#define ELEMENTS(table) (sizeof(table) / sizeof(table[0]))

struct  LangID2Charset
{
	Lang_charset charset;
	ScriptCode scriptcode;
};

static LangID2Charset scriptcodetocharsets[] = {
	{ LCH_ENGLISH, smRoman },
	{ LCH_ROMAN, smRoman },
	{ LCH_JAPANESE, smJapanese },
	{ LCH_CHINESE, smTradChinese },
	{ LCH_RUSSIAN, smCyrillic },
	{ LCH_TURKISH, smCyrillic },
	{ LCH_BULGARIAN, smCyrillic },
	{ LCH_UKRAINIAN, smCyrillic },
	{ LCH_ARABIC, smArabic },
	{ LCH_HEBREW, smHebrew },
	{ LCH_GREEK, smGreek },
	{ LCH_KOREAN, smKorean },
	{ LCH_POLISH, smCentralEuroRoman },
	{ LCH_VIETNAMESE, smVietnamese },
	{ LCH_LITHUANIAN, smCentralEuroRoman },
	{ LCH_THAI, smThai },
	{ LCH_SIMPLE_CHINESE, smSimpChinese },
	{ LCH_UNICODE, smUnicodeScript }
};

uint2 MCS_charsettolangid(uint1 charset)
{
	uint2 i;
	for (i = 0; i < ELEMENTS(scriptcodetocharsets); i++)
		if (scriptcodetocharsets[i].charset == charset)
			return scriptcodetocharsets[i].scriptcode;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
