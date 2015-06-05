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

#include "osspec.h"

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
#include <sys/wait.h>

#include <sys/file.h>

#include <iconv.h>
#include "text.h"

struct ConverterRecord
{
	ConverterRecord *next;
	const char *encoding;
	iconv_t converter;
};

static iconv_t fetch_converter(const char *p_encoding)
{
	static ConverterRecord *s_records = NULL;
	
	ConverterRecord *t_previous, *t_current;
	for(t_previous = NULL, t_current = s_records; t_current != NULL; t_previous = t_current, t_current = t_current -> next)
		if (t_current -> encoding == p_encoding)
			break;
			
	if (t_current == NULL)
	{
		iconv_t t_converter;
		t_converter = iconv_open("UTF-16", p_encoding);
		
		ConverterRecord *t_record;
		t_record = new ConverterRecord;
		t_record -> next = s_records;
		t_record -> encoding = p_encoding;
		t_record -> converter = t_converter;
		s_records = t_record;
		
		return t_record -> converter;
	}
	
	if (t_previous != NULL)
	{
		t_previous -> next = t_current -> next;
		t_current -> next = s_records;
		s_records = t_current;
	}
	
	return s_records -> converter;
}

#ifdef _DARWIN_SERVER
#define ftello64(a) ftello(a)
#define fseeko64(a, b, c) fseeko(a, b, c)
#define fstat64(a, b) fstat(a, b)
#define stat64 stat

#define opendir64(a) opendir(a)
#define readdir64(a) readdir(a)
#define closedir64(a) closedir(a)
#define DIR64 DIR
#endif

struct MCDateTime
{
	int4 year;
	int4 month;
	int4 day;
	int4 hour;
	int4 minute;
	int4 second;
	int4 bias;
};

#ifndef _LINUX_SERVER
static char *strndup(const char *s, uint32_t l)
{
	char *r;
	r = new char[l + 1];
	strncpy(r, s, l);
	return r;
}
#endif

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
		
		// MW-2011-06-27: [[ SERVER ]] Turn off buffering for output stderr / stdout
		if (fd == 1 || fd == 2)
			setbuf(t_stream, NULL);

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
		return ftruncate(fileno(m_stream), ftell(m_stream)) == 0;
	}
	
	virtual bool Sync(void) 
	{
		int64_t t_pos;
		t_pos = ftello64(m_stream);
		return fseeko64(m_stream, t_pos, SEEK_SET) == 0;
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
		return ftello64(m_stream);
	}
	
	virtual int64_t GetFileSize(void)
	{
		struct stat64 t_info;
		if (fstat64(fileno(m_stream), &t_info) != 0)
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
	
private:
	FILE *m_stream;
};

struct MCPosixSystem: public MCSystemInterface
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

	virtual char *GetVersion(void)
	{
		char *buffer;
		struct utsname u;
		uname(&u);
		buffer = new char[strlen(u.sysname) + strlen(u.release) + 2];
		sprintf(buffer, "%s %s", u.sysname, u.release);
		return buffer;
	}

	virtual char *GetMachine(void)
	{
		struct utsname u;
		uname(&u);
		return strclone(u.machine);
	}

	virtual char *GetProcessor(void)
	{
#ifdef __LITTLE_ENDIAN__
		return strclone("x86");
#else
		return strclone("Motorola PowerPC");
#endif
	}

	virtual char *GetAddress(void)
	{
		extern char *MCcmd;
		char *buffer;
		utsname u;
		uname(&u);
		buffer = new char[strlen(u.nodename) + strlen(MCcmd) + 4];
		sprintf(buffer, "%s:%s", u.nodename, MCcmd);
		return buffer;
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
	
	virtual void SetEnv(const char *name, const char *value)
	{
		setenv(name, value, 1);
	}
	
	virtual char *GetEnv(const char *name)
	{
		return getenv(name);
	}
	
	virtual bool CreateFolder(const char *p_path)
	{
		return mkdir(p_path, 0777) == 0;
	}
	
	virtual bool DeleteFolder(const char *p_path)
	{
		return rmdir(p_path) == 0;
	}
	
	virtual bool DeleteFile(const char *p_path)
	{
		return unlink(p_path) == 0;
	}
	
	virtual bool RenameFileOrFolder(const char *p_old_name, const char *p_new_name)
	{
		return rename(p_old_name, p_new_name) == 0;
	}
	
	virtual bool BackupFile(const char *p_old_name, const char *p_new_name)
	{
		return rename(p_old_name, p_new_name) == 0;
	}
	
	virtual bool UnbackupFile(const char *p_old_name, const char *p_new_name)
	{
		return rename(p_old_name, p_new_name) == 0;
	}
	
	virtual bool CreateAlias(const char *p_target, const char *p_alias)
	{
		return symlink(p_target, p_alias) == 0;
	}
	
	virtual char *ResolveAlias(const char *p_target)
	{
		return strdup(p_target);
	}
	
	virtual char *GetCurrentFolder(void)
	{
		return getcwd(NULL, 0);
	}
	
	virtual bool SetCurrentFolder(const char *p_path)
	{
		return chdir(p_path) == 0;
	}
	
	virtual char *GetStandardFolder(const char *name)
	{
		return NULL;
	}
	
	virtual bool FileExists(const char *p_path) 
	{
		struct stat64 t_info;
		
		bool t_found;
		t_found = stat64(p_path, &t_info) == 0;
		if (t_found && (t_info.st_mode & S_IFDIR) == 0)
			return true;
		
		return false;
	}
	
	virtual bool FolderExists(const char *p_path)
	{
		struct stat64 t_info;
		
		bool t_found;
		t_found = stat64(p_path, &t_info) == 0;
		if (t_found && (t_info.st_mode & S_IFDIR) != 0)
			return true;
		
		return false;
	}
	
	virtual bool FileNotAccessible(const char *p_path)
	{
		struct stat64 t_info;
		if (stat64(p_path, &t_info) != 0)
			return false;
		
		if ((t_info . st_mode & S_IFDIR) != 0)
			return true;
		
		if ((t_info . st_mode & S_IWUSR) == 0)
			return true;
		
		return false;
	}
	
	virtual bool ChangePermissions(const char *p_path, uint2 p_mask)
	{
		return chmod(p_path, p_mask) == 0;
	}
	
	virtual uint2 UMask(uint2 p_mask)
	{
		return umask(p_mask);
	}
	
	virtual MCSystemFileHandle *OpenFile(const char *p_path, uint32_t p_mode, bool p_map)
	{
		static const char *s_modes[] = { "r", "w", "r+", "a" };

		MCSystemFileHandle *t_handle;
		t_handle = MCStdioFileHandle::Open(p_path, s_modes[p_mode & 0xff]);
		if (t_handle == NULL && p_mode == kMCSystemFileModeUpdate)
			t_handle = MCStdioFileHandle::Open(p_path, "w+");
		
		return t_handle;
	}
	
	virtual MCSystemFileHandle *OpenStdFile(uint32_t i)
	{
		static const char *s_modes[] = { "r", "w", "w" };
		return MCStdioFileHandle::OpenFd(i, s_modes[i]);
	}
	
	virtual MCSystemFileHandle *OpenDevice(const char *p_path, uint32_t p_mode, const char *p_control_string)
	{
		return NULL;
	}
	
	virtual char *GetTemporaryFileName(void)
	{
		return strdup(tmpnam(NULL));
	}
	
	//////////
	
	virtual void *LoadModule(const char *p_path)
	{
		char *t_path;
		t_path = ResolveNativePath(p_path);
		if (t_path != NULL)
		{
			void *t_result;
			t_result = dlopen(t_path, RTLD_LAZY);
			delete t_path;
			return t_result;
		}
		return NULL;
	}
	
	virtual void *ResolveModuleSymbol(void *p_module, const char *p_symbol)
	{
		return dlsym(p_module, p_symbol);
	}
	
	virtual void UnloadModule(void *p_module)
	{
		dlclose(p_module);
	}
	
	////
	
	char *PathToNative(const char *p_path)
	{
		return strdup(p_path);
	}
	
	char *PathFromNative(const char *p_path)
	{
		return strdup(p_path);
	}
	
	char *ResolvePath(const char *p_path)
	{
		return ResolveNativePath(p_path);
	}
	
	char *ResolveNativePath(const char *p_path)
	{
		char *t_tilde_path;
		t_tilde_path = NULL;
		if (p_path[0] == '~')
		{
			const char *t_user_end;
			t_user_end = strchr(p_path, '/');
			if (t_user_end == NULL)
				t_user_end = p_path + strlen(p_path);
			
			// Prepend user name
			struct passwd *t_password;
			if (t_user_end - p_path == 1)
				t_password = getpwuid(getuid());
			else
			{
				char *t_username;
				t_username = strndup(p_path, t_user_end - p_path);
				t_password = getpwnam(t_username);
				delete t_username;
			}
			
			if (t_password != NULL)
			{
				t_tilde_path = new char[strlen(t_password -> pw_dir) + strlen(p_path) - (t_user_end - p_path) + 1];
				strcpy(t_tilde_path, t_password -> pw_dir);
				strcat(t_tilde_path, t_user_end);
			}
			else
				t_tilde_path = strdup(p_path);
		}
        else if (path[0] != '/')
        {
            // SN-2015-06-05: [[ Bug 15432 ]] Fix resolvepath on Linux: we want an
            //  absolute path.
            char *t_curfolder;
            t_curfolder = MCS_getcurdir();
            tildepath = new char[strlen(t_curfolder) + strlen(path) + 2];
            /* UNCHECKED */ sprintf(tildepath, "%s/%s", t_curfolder, path);

            delete t_curfolder;
        }
		else
			t_tilde_path = strdup(p_path);
		
		char *t_absolute_path;
		if (t_tilde_path[0] != '/')
		{
			char *t_folder;
			t_folder = GetCurrentFolder();
			t_absolute_path = new char[strlen(t_folder) + strlen(t_tilde_path) + 2];
			strcpy(t_absolute_path, t_folder);
			strcat(t_absolute_path, "/");
			strcat(t_absolute_path, t_tilde_path);
			delete t_tilde_path;
			
			// MW-2011-07-04: GetCurrentFolder() returns a string that must be
			//   freed.
			free(t_folder);
		}
		else
			t_absolute_path = t_tilde_path;
		
		return t_absolute_path;
	}
	
	char *LongFilePath(const char *p_path)
	{
		return strclone(p_path);
	}
	
	char *ShortFilePath(const char *p_path)
	{
		return strclone(p_path);
	}
	
	bool ListFolderEntries(MCSystemListFolderEntriesCallback p_callback, void *p_context)
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
			struct dirent64 *t_dir_entry;
			t_dir_entry = readdir64(t_dir);
			if (t_dir_entry == NULL)
				break;
			
			if (strcmp(t_dir_entry -> d_name, ".") == 0)
				continue;
			
			struct stat64 t_stat;
			stat64(t_dir_entry -> d_name, &t_stat);
			
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
	
	bool Shell(const char *p_cmd, uint32_t p_cmd_length, void*& r_data, uint32_t& r_data_length, int& r_retcode)
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
	
	bool HostNameToAddress(const char *p_hostname, MCSystemHostResolveCallback p_callback, void *p_context)
	{
		/* Use inet_aton to check whether p_hostname is
		 * already an IP address; if so, don't bother calling
		 * gethostbyname(). */
		struct in_addr t_inaddr;
		if (inet_aton (p_hostname, &t_inaddr))
		{
			return p_callback (p_context, inet_ntoa (t_inaddr));
		}

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
	
	bool AddressToHostName(const char *p_address, MCSystemHostResolveCallback p_callback, void *p_context)
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
	
	//////////////////
	
	uint32_t TextConvert(const void *p_string, uint32_t p_string_length, void *p_buffer, uint32_t p_buffer_length, uint32_t p_from_charset, uint32_t p_to_charset)
	{
		if (p_from_charset == p_to_charset)
		{
			if (p_buffer != NULL)
				memcpy(p_buffer, p_string, p_string_length);

			return p_string_length;
		}
		else if (p_from_charset == LCH_UNICODE)
		{
			if (p_buffer != NULL)
			{
				for(uint32_t i = 0; i < p_string_length; i++)
					if (((unichar_t *)p_string)[i] < 256)
						((unsigned char *)p_buffer)[i] = ((unichar_t *)p_string)[i] & 0xff;
					else
						((unsigned char *)p_buffer)[i] = '?';
			}

			return p_string_length / 2;
		}
		else if (p_to_charset == LCH_UNICODE)
		{
			if (p_buffer != NULL)
			{
				for(uint32_t i = 0; i < p_string_length; i++)
					((unichar_t *)p_buffer)[i] = ((unsigned char *)p_string)[i];
			}

			return p_string_length * 2;
		}

		return 0;
	}
	
	// MM-2012-03-23: Copied over implemteation of MCSTextConvertToUnicode.
	bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
	{
		if (p_input_length == 0)
		{
			r_used = 0;
			return true;
		}
		
		if (p_output_length == 0)
		{
			r_used = p_input_length * 4;
			return false;
		}
		
		const char *t_encoding;
		t_encoding = NULL;
		
		if (p_input_encoding >= kMCTextEncodingWindowsNative)
		{
			struct { uint4 codepage; const char *encoding; } s_codepage_map[] =
			{
				{437, "CP437" },
				{850, "CP850" },
				{932, "CP932" },
				{949, "CP949" },
				{1361, "CP1361" },
				{936, "CP936" },
				{950, "CP950" },
				{1253, "WINDOWS-1253" },
				{1254, "WINDOWS-1254" },
				{1258, "WINDOWS-1258" },
				{1255, "WINDOWS-1255" },
				{1256, "WINDOWS-1256" },
				{1257, "WINDOWS-1257" },
				{1251, "WINDOWS-1251" },
				{874, "CP874" },
				{1250, "WINDOWS-1250" },
				{1252, "WINDOWS-1252" },
				{10000, "MACINTOSH" }
			};
			
			for(uint4 i = 0; i < sizeof(s_codepage_map) / sizeof(s_codepage_map[0]); ++i)
				if (s_codepage_map[i] . codepage == p_input_encoding - kMCTextEncodingWindowsNative)
				{
					t_encoding = s_codepage_map[i] . encoding;
					break;
				}
			
		}
		else if (p_input_encoding >= kMCTextEncodingMacNative)
			t_encoding = "MACINTOSH";
		
		iconv_t t_converter;
		t_converter = fetch_converter(t_encoding);
		
		if (t_converter == NULL)
		{
			r_used = 0;
			return true;
		}
		
		char *t_in_bytes;
		char *t_out_bytes;
		size_t t_in_bytes_left;
		size_t t_out_bytes_left;
		
		t_in_bytes = (char *)p_input;
		t_in_bytes_left = p_input_length;
		t_out_bytes = (char *)p_output;
		t_out_bytes_left = p_output_length;
		
		iconv(t_converter, NULL, NULL, &t_out_bytes, &t_out_bytes_left);
		
		if (iconv(t_converter, &t_in_bytes, &t_in_bytes_left, &t_out_bytes, &t_out_bytes_left) == (size_t)-1)
		{
			r_used = 4 * p_input_length;
			return false;
		}
		
		r_used = p_output_length - t_out_bytes_left;
		
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

////////////////////////////////////////////////////////////////////////////////

MCSystemInterface *MCServerCreatePosixSystem(void)
{
	return new MCPosixSystem;
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_isnan(double v)
{
	return isnan(v) != 0;
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_get_temporary_folder(char *&r_temp_folder)
{
	bool t_success = true;

	const char *t_tmpdir = NULL;
	int32_t t_tmpdir_len = 0;
	t_tmpdir = MCS_getenv("TMPDIR");

	if (t_tmpdir == NULL)
		t_tmpdir = "/tmp";

	if (t_success)
	{
		t_tmpdir_len = MCCStringLength(t_tmpdir);
		t_success = t_tmpdir_len > 0;
	}

	if (t_success)
	{
		if (t_tmpdir[t_tmpdir_len - 1] == '/')
			t_success = MCCStringCloneSubstring(t_tmpdir, t_tmpdir_len - 1, r_temp_folder);
		else
			t_success = MCCStringClone(t_tmpdir, r_temp_folder);
	}

	return t_success;
}

bool MCS_create_temporary_file(const char *p_path, const char *p_prefix, IO_handle &r_file, char *&r_name)
{
	char *t_temp_file = NULL;
	if (!MCCStringFormat(t_temp_file, "%s/%sXXXXXXXX", p_path, p_prefix))
		return false;
	
	int t_fd;
	t_fd = mkstemp(t_temp_file);
	if (t_fd == -1)
	{
		MCCStringFree(t_temp_file);
		return false;
	}
	
	r_name = t_temp_file;
	r_file = new IO_header(MCStdioFileHandle :: OpenFd(t_fd, "w+"), 0);
	
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
