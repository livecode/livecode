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

#include <windows.h>
#include <sys/timeb.h>
#include <iphlpapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <sys/stat.h>
#include <io.h>
#include <direct.h>
#include <float.h>
#undef GetCurrentTime

#include "system.h"
#include "filesystem.h"

#include <fcntl.h>
#include <crtdbg.h>

#include "text.h"

////////////////////////////////////////////////////////////////////////////////

struct MCDateTimeLocale
{
	const char *weekday_names[7];
	const char *abbrev_weekday_names[7];
	const char *month_names[12];
	const char *abbrev_month_names[12];
	const char *date_formats[3];
	const char *time_formats[2];
	const char *time24_formats[2];
	const char *time_morning_suffix;
	const char *time_evening_suffix;
};

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

static int inet_aton(const char *cp, struct in_addr *inp)
{
	unsigned long rv = inet_addr(cp);
	if (rv == -1)
		return False;
	memcpy(inp, &rv, sizeof(unsigned long));
	return True;
}

static void CloseHandleSafely(HANDLE& x_handle)
{
	if (x_handle == NULL)
		return;

	CloseHandle(x_handle);
	x_handle = NULL;
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

static uint1 MCS_langidtocharset(uint2 langid)
{
	uint2 i;
	for (i = 0; i < ELEMENTS(langidtocharsets); i++)
		if (langidtocharsets[i].langid == langid)
			return langidtocharsets[i].charset;
	return 0;
}

static uint2 MCS_charsettolangid(uint1 charset)
{
	uint2 i;
	for (i = 0; i < ELEMENTS(langidtocharsets); i++)
		if (langidtocharsets[i].charset == charset)
			return langidtocharsets[i].langid;
	return 0;
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
		return _fseeki64_nolock(m_stream, offset, p_dir < 0 ? SEEK_END : (p_dir > 0 ? SEEK_SET : SEEK_CUR)) == 0;
	}
	
	virtual bool Truncate(void)
	{
		return _chsize(fileno(m_stream), ftell(m_stream)) == 0;
	}
	
	virtual bool Sync(void) 
	{
		int64_t t_pos;
		t_pos = _ftelli64_nolock(m_stream);
		return _fseeki64_nolock(m_stream, t_pos, SEEK_SET) == 0;
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
		return _ftelli64(m_stream);
	}
	
	virtual int64_t GetFileSize(void)
	{
		struct _stat64 t_info;
		if (_fstat64(fileno(m_stream), &t_info) != 0)
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

struct MCWindowsSystem: public MCSystemInterface
{
	virtual real64_t GetCurrentTime(void)
	{
		static real8 curtime = 0.0;
		static real8 starttime = 0.0;
		static DWORD startcount = 0;

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
	
	virtual uint32_t GetProcessId(void)
	{
		return GetCurrentProcessId();
	}

	virtual char *GetVersion(void)
	{
		char *buffer;
		buffer = new char[9 + 2 * I4L];
		sprintf(buffer, "NT %d.%d", (MCmajorosversion >> 8) & 0xFF, MCmajorosversion & 0xFF);
		return buffer;
	}

	virtual char *GetMachine(void)
	{
		return strclone("x86");
	}

	virtual char *GetProcessor(void)
	{
		return strclone("x86");
	}

	virtual char *GetAddress(void)
	{
		char *buffer;
		buffer = new char[MAXHOSTNAMELEN + strlen(MCcmd) + 2];
		gethostname(buffer, MAXHOSTNAMELEN);
		strcat(buffer, ":");
		strcat(buffer, MCcmd);
		return buffer;
	}
	
	virtual void Alarm(real64_t p_interval)
	{
	}
	
	virtual void Sleep(real64_t p_duration)
	{
		::Sleep((DWORD)(p_duration * 1000.0));
	}

	virtual void Debug(const char *message)
	{
	}
	
	virtual void SetEnv(const char *name, const char *value)
	{
		if (value == NULL)
		{
			char *dptr = new char[strlen(name) + 2];
			sprintf(dptr, "%s=", name);
			_putenv(dptr);
			delete[] dptr;
		}
		else
		{
			char *dptr = new char[strlen(name) + strlen(value) + 2];
			sprintf(dptr, "%s=%s", name, value);
			_putenv(dptr);
			delete[] dptr;
		}
	}

	virtual char *GetEnv(const char *name)
	{
		return getenv(name);
	}
	
	virtual bool CreateFolder(const char *p_path)
	{
		return _mkdir(p_path) == 0;
	}
	
	virtual bool DeleteFolder(const char *p_path)
	{
		return _rmdir(p_path) == 0;
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
	
	virtual bool CreateAlias(const char *in_source, const char *dest)
	{
		char *source;
		source = strdup(in_source);
		HRESULT err;
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
			char *buffer = strrchr(source, '\\' );
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
		free(source);
		return SUCCEEDED(err);
	}
	
	virtual char *ResolveAlias(const char *source)
	{
		char *dest = new char[PATH_MAX];
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
		if (SUCCEEDED(hres))
		{
			MCU_path2std(dest);
		}
		else
		{
			delete dest;
			dest = NULL;
		}
		return dest;
	}
	
	virtual char *GetCurrentFolder(void)
	{
		char *dptr = new char[PATH_MAX + 2];
		GetCurrentDirectoryA(PATH_MAX +1, (LPSTR)dptr);
		return dptr;
	}
	
	virtual bool SetCurrentFolder(const char *p_path)
	{
		BOOL done = SetCurrentDirectoryA((LPCSTR)p_path);
		return done == TRUE;
	}
	
	char *GetStandardFolder(const char *p_name)
	{
		static struct {const char *token; uint4 winfolder;} sysfolderlist[] =
		{
            {"desktop", CSIDL_DESKTOP},
            {"fonts", CSIDL_FONTS},
            {"documents", CSIDL_PERSONAL},
            {"start", CSIDL_STARTMENU},
			{"home", CSIDL_PROFILE} //TS-2007-08-20 Added so that "home" gives something useful across all platforms
        };

		char t_path[PATH_MAX];

		Boolean wasfound = False;
		uint4 specialfolder = 0;
		if (MCCStringEqualCaseless(p_name, "temporary"))
		{
			if (GetTempPathA(PATH_MAX, t_path))
			{
				char *sptr = strrchr(t_path, '\\');
				if (sptr != NULL)
					*sptr = '\0';

				wasfound = True;
			}
		}
		else if (MCCStringEqualCaseless(p_name, "system"))
		{
			if (GetWindowsDirectoryA(t_path, PATH_MAX))
				wasfound = True;
		}
		else
		{
			double t_real_value;
			if (MCU_stor8(p_name, t_real_value, False))
			{
				specialfolder = (uint32_t)t_real_value;
				wasfound = True;
			}
			else
			{
				uint1 i;
				for (i = 0 ; i < ELEMENTS(sysfolderlist) ; i++)
					if (MCCStringEqualCaseless(p_name, sysfolderlist[i].token))
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
						&& SHGetPathFromIDListA(lpiil, t_path))
					wasfound = True;
				lpm->Free(lpiil);
				lpm->Release();
			}
		}

		if (wasfound)
		{
			MCU_path2std(t_path);

			char *t_long_path;
			t_long_path = LongFilePath(t_path);
			if (t_long_path == nil)
				t_long_path = strdup(t_path);

			return t_long_path;
		}

		return NULL;
	}

	bool Exists(const char *path, Boolean file)
	{
		if (*path == '\0')
			return False;
		
		// MW-2008-01-15: [[ Bug 4981 ]] - It seems that stat will fail for checking
		//   a folder 'C:' and requires that it be 'C:\'
		char *newpath;
		if (strlen(path) == 2 && path[1] == ':')
		{
			// newpath is of form "<driveletter>:"
			char *t_modified_path;
			t_modified_path = new char[strlen(path) + 2];
			strcpy(t_modified_path, path);
			strcat(t_modified_path, "\\");
			newpath = t_modified_path;
		}
		else
			newpath = strdup(path);

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
	}

	virtual bool FileExists(const char *p_path) 
	{
		return Exists(p_path, True);
	}
	
	virtual bool FolderExists(const char *p_path)
	{
		return Exists(p_path, False);
	}
	
	virtual bool FileNotAccessible(const char *path)
	{
		struct stat buf;
		if (stat(path, &buf))
			return False;
		if (buf.st_mode & S_IFDIR)
			return True;
		if (!(buf.st_mode & _S_IWRITE))
			return True;
		return False;
	}
	
	virtual bool ChangePermissions(const char *p_path, uint2 p_mask)
	{
		return chmod(p_path, p_mask) == 0;
	}
	
	virtual uint2 UMask(uint2 mask)
	{
		return _umask(mask);
	}
	
	virtual MCSystemFileHandle *OpenFile(const char *p_path, uint32_t p_mode, bool p_map)
	{
		static const char *s_modes[] = { "rb", "wb", "rb+", "ab" };

		MCSystemFileHandle *t_handle;
		t_handle = MCStdioFileHandle::Open(p_path, s_modes[p_mode & 0xff]);
		if (t_handle == NULL && p_mode == kMCSystemFileModeUpdate)
			t_handle = MCStdioFileHandle::Open(p_path, "wb+");
		
		return t_handle;
	}
	
	virtual MCSystemFileHandle *OpenStdFile(uint32_t i)
	{
		_setmode(i, _O_BINARY);
		static const char *s_modes[] = { "rb", "wb", "wb" };
		return MCStdioFileHandle::OpenFd(i, s_modes[i]);
	}
	
	virtual MCSystemFileHandle *OpenDevice(const char *p_path, uint32_t p_mode, const char *p_control_string)
	{
		return NULL;
	}
	
	virtual char *GetTemporaryFileName(void)
	{
		MCExecPoint ep(nil, nil, nil);

		// MW-2008-06-19: Make sure fname is stored in a static to keep the (rather
		//   unpleasant) current semantics of the call.
		static char *fname;
		if (fname != NULL)
			delete fname;

		// TS-2008-06-18: [[ Bug 6403 ]] - specialFolderPath() returns 8.3 paths
		fname = _tempnam("\\tmp", "tmp");

		char *t_ptr = (char*)strrchr(fname, '\\');
		if (t_ptr != NULL)
			*t_ptr = 0 ;
		
		MCU_path2std(fname);

		char *t_long_fname;
		t_long_fname = LongFilePath(fname);
		if (t_long_fname == nil)
			ep . setsvalue(fname);
		else
		{
			delete fname;
			fname = t_long_fname;
			ep . setsvalue(fname);
		}
		
		if (t_ptr != NULL)
			ep.appendstringf("/%s", ++t_ptr);

		// MW-2008-06-19: Make sure we delete this version of fname, since we don't
		//   need it anymore.
		delete fname;

		// MW-2008-06-19: Use ep . getsvalue() . clone() to make sure we get a copy
		//   of the ExecPoint's string as a NUL-terminated (C-string) string.
		fname = ep . getsvalue() . clone();

		return fname;
	}
	
	//////////
	
	virtual void *LoadModule(const char *p_path)
	{
		char *t_native_path;
		t_native_path = ResolvePath(p_path);

		HMODULE t_handle;
		t_handle = LoadLibraryExA(t_native_path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
		
		free(t_native_path);

		return t_handle;
	}
	
	virtual void *ResolveModuleSymbol(void *p_module, const char *p_symbol)
	{
		return GetProcAddress((HMODULE)p_module, p_symbol);
	}
	
	virtual void UnloadModule(void *p_module)
	{
		FreeLibrary((HMODULE)p_module);
	}
	
	////
	
	char *LongFilePath(const char *p_path)
	{
		char *shortpath = MCS_resolvepath(p_path);
		char *longpath = new char[PATH_MAX];
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
			p = strchr(pStart = p, '\\');
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
					*p = '\\';
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
						delete longpath;
						delete shortpath;
						return nil;
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
		MCU_path2std(longpath);
		delete shortpath;
		return longpath;
	}

	char *ShortFilePath(const char *p_path)
	{
		char *shortpath = new char[PATH_MAX];
		char *newpath = MCS_resolvepath(p_path);
		if (!GetShortPathNameA(newpath, shortpath, PATH_MAX))
		{
			delete shortpath;
			shortpath = nil;
		}
		else
			MCU_path2std(shortpath);
		delete newpath;
		return shortpath;
	}

	////

	char *PathToNative(const char *p_path)
	{
		char *t_path;
		t_path = strdup(p_path);
		MCU_path2native(t_path);
		return t_path;
	}
	
	char *PathFromNative(const char *p_path)
	{
		char *t_path;
		t_path = strdup(p_path);
		MCU_path2std(t_path);
		return t_path;
	}
	
	char *ResolvePath(const char *p_path)
	{
		return ResolveNativePath(p_path);
	}
	
	char *ResolveNativePath(const char *path)
	{
		if (path == NULL)
		{
			char *tpath = GetCurrentFolder();
			return tpath;
		}

		DWORD t_count;
		t_count = GetFullPathNameA(path, 0, NULL, NULL);

		char *t_path;
		t_path = new char[t_count];
		GetFullPathNameA(path, t_count, t_path, NULL);

		return t_path;
	}
	
	
	bool ListFolderEntries(MCSystemListFolderEntriesCallback p_callback, void *p_context)
	{
		MCSystemFolderEntry t_entry;
		memset(&t_entry, 0, sizeof(MCSystemFolderEntry));
		
		WIN32_FIND_DATAA data;
		HANDLE ffh;            //find file handle
		uint4 t_entry_count;
		t_entry_count = 0;
		Boolean ok = False;
		char *tpath = GetCurrentFolder();
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
			return true;
		}

		bool t_success;
		t_success = true;
		while(t_success)
		{
			if (!strequal(data.cFileName, "."))
			{
				struct _stati64 t_stat;
				_stati64(data.cFileName, &t_stat);

				t_entry . name = data . cFileName;
				t_entry . data_size = t_stat . st_size;
				t_entry . resource_size = 0;
				t_entry . creation_time = (uint32_t)t_stat . st_ctime;
				t_entry . modification_time = (uint32_t)t_stat . st_mtime;
				t_entry . access_time = (uint32_t)t_stat . st_atime;
				t_entry . permissions = t_stat . st_mode & 0777;
				t_entry . is_folder = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

				t_success = p_callback(p_context, &t_entry);
			}

			if (!FindNextFileA(ffh, &data))
				break;
		}
		FindClose(ffh);
		delete spath;

		return t_success;
	}
	
	bool Shell(const char *p_cmd, uint32_t p_cmd_length, void*& r_data, uint32_t& r_data_length, int& r_retcode)
	{
		bool t_success;
		t_success = true;

		// Construct the command that we are to execute. This consists of
		// 'cmd.exe /C <cmd>'
		char *t_command;
		t_command = nil;
		if (t_success)
			t_success = MCCStringFormat(t_command, "cmd.exe /C %.*s", p_cmd_length, p_cmd);

		// The pipe handles need to be inherited by the shell process, this is
		// indicated via the security attributes.
		SECURITY_ATTRIBUTES t_attr;
		t_attr . nLength = sizeof(SECURITY_ATTRIBUTES);
		t_attr . bInheritHandle = TRUE;
		t_attr . lpSecurityDescriptor = NULL;

		// The child/parent handles corresponding to the various ends of the
		// pipes. These are as follows:
		//   child stdin -> read side of stdin pipe
		//   child stdout -> write side of stdout pipe
		//   parent stdin -> write side of stdin pipe
		//   parent stdout -> read size of stdout pipe
		HANDLE t_child_stdin, t_parent_stdin, t_child_stdout, t_parent_stdout;
		t_child_stdin = t_parent_stdin = NULL;
		t_child_stdout = t_parent_stdout = NULL;
		if (t_success)
			t_success =
				CreatePipe(&t_child_stdin, &t_parent_stdin, &t_attr, 0) &&
				CreatePipe(&t_parent_stdout, &t_child_stdout, &t_attr, 0);

		// The stderr handle is special as we treat stderr and stdout as one
		// and thus we just duplicate a child stderr handle as the same as
		// the child stdout handle.
		HANDLE t_child_stderr;
		t_child_stderr = NULL;
		if (t_success)
			t_success = DuplicateHandle(GetCurrentProcess(), t_child_stdout, GetCurrentProcess(), &t_child_stderr, 0, TRUE, DUPLICATE_SAME_ACCESS) == TRUE;

		// Now launch the process (or at least attempt to)
		PROCESS_INFORMATION t_process;
		MCMemoryClear(&t_process, sizeof(PROCESS_INFORMATION));
		if (t_success)
		{
			STARTUPINFOA t_startup;
			MCMemoryClear(&t_startup, sizeof(STARTUPINFOA));
			t_startup . cb = sizeof(STARTUPINFOA);
			t_startup . dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			if (MChidewindows)
				t_startup . wShowWindow = SW_HIDE;
			else
				t_startup . wShowWindow = SW_SHOW;
			t_startup . hStdInput = t_child_stdin;
			t_startup . hStdOutput = t_child_stdout;
			t_startup . hStdError = t_child_stderr;

			t_success = CreateProcessA(NULL, t_command, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &t_startup, &t_process) == TRUE;
		}

		// At this point we close the child handles regardless since we don't need
		// them anymore.
		CloseHandleSafely(t_child_stdin);
		CloseHandleSafely(t_child_stdout);
		CloseHandleSafely(t_child_stderr);

		// Now loop until the process finishes doing its stuff, accumulating the
		// data as we go. Notice that we use a 4K granularity to allocations. This
		// is because shell commands have a tendency to 'trickle' data and resizing
		// the data array everytime would be expensive.
		void *t_data;
		uint32_t t_data_length, t_data_capacity;
		t_data = nil;
		t_data_length = 0;
		t_data_capacity = 0;
		for(;;)
		{
			// Attempt to read 4K from the pipe. This function will block until it
			// reads 4K, or the pipe is closed.
			char t_buffer[4096];
			DWORD t_bytes_read;
			if (!ReadFile(t_parent_stdout, t_buffer, sizeof(t_buffer), &t_bytes_read, NULL))
				break;

			// If we need a bigger data buffer, then reallocate it - notice we use
			// 'reallocate' rather than 'resize array' this is because we don't need
			// it to be initialized.
			if (t_data_length + t_bytes_read > t_data_capacity)
			{
				uint32_t t_new_data_capacity;
				t_new_data_capacity = (t_data_length + t_bytes_read + 4095) & ~4095;
				if (!MCMemoryReallocate(t_data, t_new_data_capacity, t_data))
					break;

				t_data_capacity = t_new_data_capacity;
			}

			// Accumulate the read data into the buffer.
			MCMemoryCopy((char *)t_data + t_data_length, t_buffer, t_bytes_read);
			t_data_length += t_bytes_read;
		}

		if (t_success)
		{
			r_data = t_data;
			r_data_length = t_data_length;
		}
		else
			MCMemoryDeallocate(t_data);

		CloseHandleSafely(t_process . hThread);
		CloseHandleSafely(t_process . hProcess);
		CloseHandleSafely(t_parent_stdin);
		CloseHandleSafely(t_parent_stdout);
		MCCStringFree(t_command);

		return t_success;
	}

	char *GetHostName(void)
	{
		char t_hostname[256];
		gethostname(t_hostname, 256);
		return strdup(t_hostname);
	}
	
	bool HostNameToAddress(const char *p_hostname, MCSystemHostResolveCallback p_callback, void *p_context)
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
		if (p_from_charset == LCH_UNICODE)
		{
			char szLocaleData[6];
			uint2 codepage = 0;
			GetLocaleInfoA(MAKELCID(MCS_charsettolangid(p_to_charset), SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, szLocaleData, 6);
			codepage = (uint2)strtoul(szLocaleData, NULL, 10);
			uint4 dsize = WideCharToMultiByte(codepage, 0, (LPCWSTR)p_string, p_string_length >> 1, (LPSTR)p_buffer, p_buffer_length, NULL, NULL);
			return dsize;
		}
		else if (p_to_charset == LCH_UNICODE)
		{
			char szLocaleData[6];
			uint2 codepage = 0;
			GetLocaleInfoA(MAKELCID(MCS_charsettolangid(p_from_charset), SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, szLocaleData, 6);
			codepage = (uint2)strtoul(szLocaleData, NULL, 10);
			uint4 dsize = MultiByteToWideChar(codepage, 0, (LPCSTR)p_string, p_string_length, (LPWSTR)p_buffer, p_buffer_length >> 1);
			return dsize << 1;
		}

		uint32_t t_ulength;
		t_ulength = TextConvert(p_string, p_string_length, NULL, 0, p_from_charset, LCH_UNICODE);

		char *t_ubuffer;
		t_ubuffer = new char[t_ulength];
		TextConvert(p_string, p_string_length, t_ubuffer, t_ulength, p_from_charset, LCH_UNICODE);

		uint32_t t_used;
		t_used = TextConvert(t_ubuffer, t_ulength, p_buffer, p_buffer_length, LCH_UNICODE, p_to_charset);

		delete[] t_ubuffer;

		return t_used;
	}
	
	// MM-2012-03-23: Copied over implemteation of MCSTextConvertToUnicode.
	bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
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

	//////////////////
	
	bool Initialize(void)
	{
		WORD request = MAKEWORD(1, 1);
		WSADATA t_data;
		WSAStartup(request, &t_data);

		return true;
	}
	
	void Finalize(void)
	{
	}
};

MCSystemInterface *MCServerCreateWindowsSystem(void)
{
	return new MCWindowsSystem;
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_isnan(double v)
{
	return _isnan(v) != 0;
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_get_temporary_folder(char *&r_temp_folder)
{
	WCHAR t_tmpdir[MAX_PATH];
	int32_t t_tmpdir_len = 0;
	t_tmpdir_len = GetTempPathW(MAX_PATH, t_tmpdir);

	return MCFileSystemPathFromNative(t_tmpdir, r_temp_folder);
}

bool MCS_create_temporary_file(const char *p_path, const char *p_prefix, IO_handle &r_file, char *&r_name)
{
	bool t_success = true;
	bool t_have_file = false;
	
	char *t_temp_file = NULL;
	GUID t_guid;
	WCHAR *t_guid_utf16 = NULL;
	
	HANDLE t_temp_handle = NULL;
	
	while (t_success && !t_have_file)
	{
		CoCreateGuid(&t_guid);
		
		if (t_temp_file != NULL)
		{
			MCCStringFree(t_temp_file);
			t_temp_file = NULL;
		}
		if (t_guid_utf16 != NULL)
		{
			CoTaskMemFree(t_guid_utf16);
			t_guid_utf16 = NULL;
		}

		t_success = S_OK == StringFromCLSID(t_guid, &t_guid_utf16);
		if (t_success)
		{
			char *t_guid_string = (char*)t_guid_utf16;
			uint32_t i;
			for (i = 0; t_guid_utf16[i] != '\0'; i++)
				t_guid_string[i] = t_guid_utf16[i] & 0xFF;
			t_guid_string[i] = '\0';
			t_success = MCCStringFormat(t_temp_file, "%s/%s%s", p_path, p_prefix, t_guid_string);
		}
		
		if (t_success)
		{
			t_temp_handle = CreateFileA(t_temp_file, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
			if (t_temp_handle != INVALID_HANDLE_VALUE)
				t_have_file = true;
			else
				t_success = GetLastError() == ERROR_FILE_EXISTS;
		}
	}
	
	if (t_success)
	{
		r_file = new IO_header(MCStdioFileHandle :: OpenFd(_open_osfhandle((intptr_t)t_temp_handle, _O_RDWR), "w+"), 0);
		r_name = t_temp_file;
	}
	else
		MCCStringFree(t_temp_file);
	
	CoTaskMemFree(t_guid_utf16);
	return t_success;
}

bool MCSystemLockFile(MCSystemFileHandle *p_file, bool p_shared, bool p_wait)
{
	int t_fd = fileno(((MCStdioFileHandle*)p_file)->GetStream());
	
	HANDLE t_fhandle = (HANDLE)_get_osfhandle(t_fd);

	bool t_success = true;

	if (!p_shared)
	{
		DWORD t_flags = 0;
	
		if (!p_shared)
			t_flags |= LOCKFILE_EXCLUSIVE_LOCK;
		if (!p_wait)
			t_flags |= LOCKFILE_FAIL_IMMEDIATELY;
	
		OVERLAPPED t_range;
		ZeroMemory(&t_range, sizeof(t_range));
		t_success = FALSE != LockFileEx(t_fhandle, t_flags, 0, MAXDWORD, MAXDWORD, &t_range);
	}
	
	return t_success;
}



