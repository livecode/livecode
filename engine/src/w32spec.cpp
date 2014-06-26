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

#include "w32prefix.h"

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
#include "globals.h"
#include "scriptenvironment.h"
#include "meta.h"
#include "securemode.h"
#include "socket.h"
#include "notify.h"
#include "osspec.h"

#include "w32dc.h"

#include <locale.h>
#include <signal.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <io.h>
#include <process.h>
#include <float.h>
#include <iphlpapi.h>

int *g_mainthread_errno;

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

void MCS_init()
{
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
}

void MCS_shutdown()
{
	OleUninitialize();
}

// MW-2007-12-14: [[ Bug 5113 ]] Slow-down on mathematical operations - make sure
//   we access errno directly to stop us having to do a thread-local-data lookup.
void MCS_seterrno(int value)
{
	*g_mainthread_errno = value;
}

int MCS_geterrno()
{
	return *g_mainthread_errno;
}

static MMRESULT tid;

void CALLBACK MCS_tp(UINT id, UINT msg, DWORD user, DWORD dw1, DWORD dw2)
{
	MCalarm = True;
}

void MCS_alarm(real8 secs)
{ //no action
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
}

void MCS_checkprocesses()
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
}

void MCS_closeprocess(uint2 index)
{
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
}

void MCS_kill(int4 pid, int4 sig)
{
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
}

void MCS_killall()
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

// MW-2005-02-22: Make this global for now so it is accesible by opensslsocket.cpp
real8 curtime;
static real8 starttime;
static DWORD startcount;

real8 MCS_time()
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

real8 MCS_starttime(void)
{
	return starttime;
}

void MCS_reset_time()
{
	if (!MClowrestimers)
	{
		startcount = 0;
		MCS_time();
		startcount = timeGetTime();
	}
}

void MCS_sleep(real8 delay)
{
	Sleep((DWORD)(delay * 1000.0));  //takes milliseconds as parameter
}

char *MCS_getenv(const char *name)
{
	return getenv(name);
}

void MCS_setenv(const char *name, const char *value)
{
	char *dptr = new char[strlen(name) + strlen(value) + 2];
	sprintf(dptr, "%s=%s", name, value);
	_putenv(dptr);

	// MW-2005-10-29: Memory leak
	delete[] dptr;
}

void MCS_unsetenv(const char *name)
{
	char *dptr = new char[strlen(name) + 2];
	sprintf(dptr, "%s=", name);
	_putenv(dptr);

	// MW-2005-10-29: Memory leak
	delete[] dptr;
}

int4 MCS_rawopen(const char *path, int4 flags)
{
	return 0;
}

int4 MCS_rawclose(int4 fd)
{
	return 0;
}

Boolean MCS_rename(const char *oldname, const char *newname)
{
	char *op = MCS_resolvepath(oldname);
	char *np = MCS_resolvepath(newname);
	Boolean done = rename(op, np) == 0;
	delete op;
	delete np;
	return done;
}

Boolean MCS_backup(const char *oname, const char *nname)
{
	return MCS_rename(oname, nname);
}

Boolean MCS_unbackup(const char *oname, const char *nname)
{
	return MCS_rename(oname, nname);
}

Boolean MCS_unlink(const char *path)
{
	char *p = MCS_resolvepath(path);
	Boolean done = remove
		               (p) == 0;
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
}

// returns in native path format
const char *MCS_tmpnam()
{
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
}

// returns in native path format
char *MCS_resolvepath(const char *path)
{
	if (path == NULL)
	{
		char *tpath = MCS_getcurdir();
		MCU_path2native(tpath);
		return tpath;
	}
	char *cstr = strclone(path);
	MCU_path2native(cstr);
	return cstr;
}

static inline bool is_legal_drive(char p_char)
{
	return (p_char >= 'a' && p_char <= 'z') || (p_char >= 'A' && p_char <= 'Z');
}

char *MCS_get_canonical_path(const char *path)
{
	char *t_path = NULL;
	char *t_curdir = NULL;

	if (path == NULL)
		return NULL;

	if (path[0] == '/' && path[1] != '/')
	{
		// path in root of current drive
		t_curdir = MCS_getcurdir();

		int t_path_len;
		t_path_len = strlen(path);
		while (path[0] == '/')
		{
			path ++;
			t_path_len --;
		}
		
		t_path = (char*)malloc(3 + t_path_len + 1);
		t_path[0] = t_curdir[0]; t_path[1] = ':'; t_path[2] = '/';
		strcpy(t_path + 3, path);
	}
	else if (is_legal_drive(path[0]) && path[1] == ':')
	{
		// absolute path
		t_path = strclone(path);
	}
	else
	{
		// relative to current folder
		t_curdir = MCS_getcurdir();
		int t_curdir_len;
		t_curdir_len = strlen(t_curdir);

		while (t_curdir_len > 0 && t_curdir[t_curdir_len - 1] == '/')
			t_curdir_len--;

		int t_path_len;
		t_path_len = strlen(path);

		while (t_path_len > 0 && path[0] == '/')
		{
			path ++;
			t_path_len --;
		}

		t_path = (char*)malloc(t_curdir_len + 1 + t_path_len + 1);
		memcpy(t_path, t_curdir, t_curdir_len);
		t_path[t_curdir_len] = '/';
		memcpy(t_path + t_curdir_len + 1, path, t_path_len + 1);
	}

	MCU_fix_path(t_path);
	return t_path;
}

char *MCS_getcurdir()
{
	char *dptr = new char[PATH_MAX + 2];
	GetCurrentDirectoryA(PATH_MAX +1, (LPSTR)dptr);
	MCU_path2std(dptr);
	return dptr;
}

Boolean MCS_setcurdir(const char *path)
{
	char *newpath = MCS_resolvepath(path);
	BOOL done = SetCurrentDirectoryA((LPCSTR)newpath);
	delete newpath;
	return done;
}

#define ENTRIES_CHUNK 4096

// MH-2007-03-30: [[ Bug 4293 ]] This bug is a Mac bug, but caused a prototype change to the MCS_getentries function and the use of an ExecPont to return information.
void MCS_getentries(MCExecPoint &p_context, bool p_files, bool p_detailed)
{
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
}


void MCS_getDNSservers(MCExecPoint &ep)
{
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
}

Boolean MCS_getdevices(MCExecPoint &ep)
{
	ep.clear();
	return True;
}

Boolean MCS_getdrives(MCExecPoint &ep)
{
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
}

Boolean MCS_noperm(const char *path)
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

Boolean MCS_exists(const char *path, Boolean file)
{
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
}

int64_t MCS_fsize(IO_handle stream)
{
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
}

Boolean MCS_nodelay(int4 fd)
{
	return True;
}

IO_handle MCS_fakeopen(const MCString &data)
{
	return new IO_header(NULL, (char *)data.getstring(), data.getlength(), IO_FAKE);
}

IO_handle MCS_fakeopenwrite(void)
{
	return new IO_header(NULL, NULL, 0, IO_FAKEWRITE);
}

IO_handle MCS_fakeopencustom(MCFakeOpenCallbacks *p_callbacks, void *p_state)
{
	return new IO_header(NULL, (char *)p_state, (uint32_t)p_callbacks, IO_FAKECUSTOM);
}

IO_stat MCS_fakeclosewrite(IO_handle& stream, char*& r_buffer, uint4& r_length)
{
	if ((stream -> flags & IO_FAKEWRITE) != IO_FAKEWRITE)
	{
		r_buffer = NULL;
		r_length = 0;
		MCS_close(stream);
		return IO_ERROR;
	}

	r_buffer = (char *)realloc(stream -> buffer, stream -> len);
	r_length = stream -> len;

	MCS_close(stream);

	return IO_NORMAL;
}

IO_handle MCS_open(const char *path, const char *mode,
                   Boolean map, Boolean driver, uint4 offset)
{
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
}

IO_stat MCS_close(IO_handle &stream)
{
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
}

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

IO_stat MCS_runcmd(MCExecPoint &ep)
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

	// SN-2014-06-16 [[ Bug 12648 ]] Shell command does not accept spaces despite being quoted (Windows)
	// Quotes surrounding the command call are needed to preserve the quotes of the command and command arguments
	ep.insert("\"", 0, 0);
	ep.appendchar('\"');
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
}

uint2 MCS_umask(uint2 mask)
{
	return _umask(mask);
}

IO_stat MCS_chmod(const char *path, uint2 mask)
{
	if (_chmod(path, mask) != 0)
		return IO_ERROR;
	return IO_NORMAL;
}

int4 MCS_getumask()
{
	return 0;
}

void MCS_setumask(int4 newmask)
{}

IO_stat MCS_trunc(IO_handle stream)
{
	if (SetEndOfFile(stream->fhandle))
		return IO_NORMAL;
	else
		return IO_ERROR;
}

Boolean MCS_mkdir(const char *path)
{
	Boolean result = False;
	char *tpath = MCS_resolvepath(path);
	if (CreateDirectoryA(tpath, NULL))
		result = True;
	delete tpath;
	return result;
}

Boolean MCS_rmdir(const char *path)
{
	Boolean result = False;
	char *tpath = MCS_resolvepath(path);
	if (RemoveDirectoryA(tpath))
		result = True;
	delete tpath;
	return result;
}

IO_stat MCS_flush(IO_handle stream)
{  //flush output buffer
	if (FlushFileBuffers(stream->fhandle) != NO_ERROR)
		return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_sync(IO_handle stream)
{
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
}

Boolean MCS_eof(IO_handle stream)
{
	if (stream->buffer != NULL)
		return stream->ioptr - stream->buffer >= (int4)stream->len;
	return (stream->flags & IO_ATEOF) != 0;
}

static IO_stat MCS_seek_do(HANDLE p_file, int64_t p_offset, DWORD p_type)
{
	LONG high_offset;
	high_offset = (LONG)(p_offset >> 32);
	DWORD fp;
	fp = SetFilePointer(p_file, (LONG)(p_offset & 0xFFFFFFFF),
	                   &high_offset, p_type);
	if (fp == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_seek_cur(IO_handle stream, int64_t offset)
{
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
}

IO_stat MCS_seek_set(IO_handle stream, int64_t offset)
{
	// MW-2009-06-30: If this is a custom stream, call the appropriate callback.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_seek_set(stream, offset);

	IO_stat is = IO_NORMAL;
	if (stream->buffer != NULL)
		IO_set_stream(stream, stream->buffer + offset);
	else
		is = MCS_seek_do(stream -> fhandle, offset, FILE_BEGIN);
	return is;
}

IO_stat MCS_seek_end(IO_handle stream, int64_t offset)
{
	IO_stat is = IO_NORMAL;
	if (stream->buffer != NULL)
		IO_set_stream(stream, stream->buffer + stream->len + offset);
	else
		is = MCS_seek_do(stream -> fhandle, offset, FILE_END);
	return is;
}

int64_t MCS_tell(IO_handle stream)
{
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
}

IO_stat MCS_putback(char c, IO_handle stream)
{
	if (stream -> buffer != NULL)
		return MCS_seek_cur(stream, -1);

	if (stream -> putback != -1)
		return IO_ERROR;
		
	stream -> putback = c;
	
	return IO_NORMAL;
}

IO_stat MCS_read(void *ptr, uint4 size, uint4 &n, IO_handle stream)
{
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
}


IO_stat MCS_write(const void *ptr, uint4 size, uint4 n, IO_handle stream)
{
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
}

bool MCS_isfake(IO_handle stream)
{
	return (stream -> flags & IO_FAKEWRITE) != 0;
}

uint4 MCS_faketell(IO_handle stream)
{
	return stream -> len;
}

void MCS_fakewriteat(IO_handle stream, uint4 p_pos, const void *p_buffer, uint4 p_size)
{
	memcpy(stream -> buffer + p_pos, p_buffer, p_size);
}

// MW-2005-02-22: Make these global for opensslsocket.cpp
static Boolean wsainited = False;
HWND sockethwnd;

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
			if (!MCnoui)
				sockethwnd = CreateWindowA(MC_WIN_CLASS_NAME, "MCsocket", WS_POPUP, 0, 0,
				                          8, 8, NULL, NULL, MChInst, NULL);
		}
	}
	MCS_seterrno(0);
	return wsainited;
}



uint4 MCS_getpid()
{
	return _getpid();
}

const char *MCS_getaddress()
{
	static char *buffer;
	if (!wsainit())
		return "unknown";
	if (buffer == NULL)
		buffer = new char[MAXHOSTNAMELEN + strlen(MCcmd) + 2];
	gethostname(buffer, MAXHOSTNAMELEN);
	strcat(buffer, ":");
	strcat(buffer, MCcmd);
	return buffer;
}

const char *MCS_getmachine()
{
	return "x86";
}

const char *MCS_getprocessor()
{
	return "x86";
}

real8 MCS_getfreediskspace(void)
{
	DWORD sc, bs, fc, tc;
	GetDiskFreeSpace(NULL, &sc, &bs, &fc, &tc);
	return ((real8)bs * (real8)sc * (real8)fc);
}

const char *MCS_getsystemversion()
{
	static Meta::static_ptr_t<char> buffer;
	if (buffer == NULL)
		buffer = new char[9 + 2 * I4L];
	sprintf(buffer, "NT %d.%d", (MCmajorosversion >> 8) & 0xFF, MCmajorosversion & 0xFF);
	return buffer;
}



void MCS_loadfile(MCExecPoint &ep, Boolean binary)
{
	if (!MCSecureModeCanAccessDisk())
	{
		ep.clear();
		MCresult->sets("can't open file");
		return;
	}
	char *tpath = ep.getsvalue().clone();
	char *newpath = MCS_resolvepath(tpath);
	ep.clear();
	delete tpath;
	HANDLE hf = CreateFileA(newpath, GENERIC_READ, FILE_SHARE_READ, NULL,
	                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	delete newpath;
	if (hf == INVALID_HANDLE_VALUE)
	{
		MCS_seterrno(GetLastError());
		MCresult->sets("can't open file");
	}
	else
	{
		DWORD fsize;
		DWORD nread = 0;
		if ((fsize = GetFileSize(hf, NULL)) == 0xFFFFFFFF
		        || ep.getbuffer(fsize) == NULL
		        || !ReadFile(hf, ep.getbuffer(fsize), fsize, &nread, NULL)
		        || nread != fsize)
		{
			ep.clear();
			MCS_seterrno(GetLastError());
			MCresult->sets("error reading file");
		}
		else
		{
			ep.setlength(fsize);
			if (!binary)
				ep.texttobinary();
			MCresult->clear(False);
		}
		CloseHandle(hf);
	}
}

void MCS_loadresfile(MCExecPoint &ep)
{
	ep.clear();
	MCresult->sets("not supported");
}

void MCS_savefile(const MCString &fname, MCExecPoint &data, Boolean binary)
{
	if (!MCSecureModeCanAccessDisk())
	{
		MCresult->sets("can't open file");
		return;
	}

	char *tpath = fname.clone();
	char *newpath = MCS_resolvepath(tpath);
	delete tpath;
	HANDLE hf = CreateFileA(newpath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
	                       OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	delete newpath;
	if (hf == INVALID_HANDLE_VALUE)
	{
		MCS_seterrno(GetLastError());
		MCresult->sets("can't open file");
	}
	else
	{
		if (!binary)
			data.binarytotext();
		DWORD nwrote;
		if (!WriteFile(hf, data.getsvalue().getstring(),
		               data.getsvalue().getlength(), &nwrote, NULL)
		        || nwrote != (DWORD)data.getsvalue().getlength())
		{
			MCS_seterrno(GetLastError());
			MCresult->sets("error writing file");
		}
		else
		{
			SetEndOfFile(hf);
			MCresult->clear(False);
		}
		CloseHandle(hf);
	}
}

void MCS_saveresfile(const MCString &s, const MCString data)
{
	MCresult->sets("not supported");
}

Boolean MCS_poll(real8 delay, int fd)
{
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
}



//FILE and APPLEEVENT
void MCS_send(const MCString &message, const char *program,
              const char *eventtype, Boolean reply)
{
	MCresult->sets("not supported");
}

void MCS_reply(const MCString &message, const char *keyword, Boolean error)
{
	MCresult->sets("not supported");
}

char *MCS_request_ae(const MCString &message, uint2 ae)
{
	MCresult->sets("not supported");
	return NULL;
}

char *MCS_request_program(const MCString &message, const char *program)
{
	MCresult->sets("not supported");
	return NULL;
}

void MCS_copyresourcefork(const char *source, const char *dest)
{}

void MCS_copyresource(const char *source, const char *dest,
                      const char *type, const char *name,
                      const char *newid)
{
	MCresult->sets("not supported");
}

void MCS_deleteresource(const char *source, const char *type,
                        const char *name)
{
	MCresult->sets("not supported");
}

void MCS_getresource(const char *source, const char *type,
                     const char *name, MCExecPoint &ep)
{
	ep.clear();
	MCresult->sets("not supported");
}

char *MCS_getresources(const char *source, const char *type)
{
	MCresult->sets("not supported");
	return NULL;
}

void MCS_setresource(const char *source, const char *type,
                     const char *name, const char *id, const char *flags,
                     const MCString &s)
{
	MCresult->sets("not supported");
}

typedef struct
{
	const char *token;
	uint4 winfolder;
}
sysfolders;

// need to add at least a temp folder and the system folder here
static sysfolders sysfolderlist[] = {
                                        {"desktop", CSIDL_DESKTOP},
                                        {"fonts", CSIDL_FONTS},
                                        {"documents", CSIDL_PERSONAL},
                                        {"start", CSIDL_STARTMENU},
										{"home", CSIDL_PROFILE}, //TS-2007-08-20 Added so that "home" gives something useful across all platforms
										// MW-2012-10-08: [[ Bug 10277 ]] Maps to roming appdata folder.
										{"support", CSIDL_APPDATA},
                                    };



// OK-2009-01-28: [[Bug 7452]] - SpecialFolderPath not working with redirected My Documents folder on Windows.
void MCS_getlongfilepath(MCExecPoint ep, char *&r_long_path, uint4 &r_length)
{
	// Clone the original exec point so that MCS_longfilepath doesn't overwrite it
	MCExecPoint *ep2;
	ep2 = new MCExecPoint(ep);

	// Attempt to expand the path
	MCS_longfilepath(*ep2);
	
	// If the path conversion was not succesful, and ep2 was cleared, then
	// we revert back to using the original ep.
	MCExecPoint *ep3;
	if (ep2 -> getsvalue() . getlength() == 0)
		ep3 = &ep;
	else
		ep3 = ep2;

	char *t_long_path;
	t_long_path = (char *)malloc(ep3 -> getsvalue() . getlength() + 1);
	memcpy(t_long_path, ep3 -> getsvalue() . getstring(), ep3 -> getsvalue() . getlength());
	t_long_path[ep3 -> getsvalue() . getlength()] = '\0';

	r_long_path = t_long_path;
	r_length = ep3 -> getsvalue() . getlength();

	delete ep2;
}


void MCS_getspecialfolder(MCExecPoint &ep)
{
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
}

void MCS_shortfilepath(MCExecPoint &ep)
{
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
}

#define PATH_DELIMITER '\\'


void MCS_longfilepath(MCExecPoint &ep)
{
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
}

Boolean MCS_createalias(char *srcpath, char *dstpath)
{
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
}

void MCS_resolvealias(MCExecPoint &ep)
{
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
}

void MCS_doalternatelanguage(MCString &s, const char *langname)
{
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
}

#define DEFINE_GUID_(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
// {F0B7A1A2-9847-11cf-8F20-00805F2CD064}
DEFINE_GUID_(CATID_ActiveScriptParse, 0xf0b7a1a2, 0x9847, 0x11cf, 0x8f, 0x20, 0x00, 0x80, 0x5f, 0x2c, 0xd0, 0x64);

void MCS_alternatelanguages(MCExecPoint &ep)
{
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

Boolean MCS_isleadbyte(uint1 charset, char *s)
{
	if (!charset)
		return False;
	char szLocaleData[6];
	GetLocaleInfoA(MAKELCID(MCS_charsettolangid(charset), SORT_DEFAULT),
	               LOCALE_IDEFAULTANSICODEPAGE, szLocaleData, 6);
	uint2 codepage = (uint2)strtoul(szLocaleData, NULL, 10);
	return IsDBCSLeadByteEx(codepage, *s);
}

static void MCS_do_launch(char *p_document)
{
	// MW-2011-02-01: [[ Bug 9332 ]] Adjust the 'show' hint to normal to see if that makes
	//   things always appear on top...
	int t_result;
	t_result = (int)ShellExecuteA(NULL, "open", p_document, NULL, NULL, SW_SHOWNORMAL);

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

void MCS_launch_document(char *p_document)
{
	char *t_native_document;

	t_native_document = MCS_resolvepath(p_document);
	delete p_document;

	// MW-2007-12-13: [[ Bug 5680 ]] Might help if we actually passed the correct
	//   pointer to do_launch!
	MCS_do_launch(t_native_document);

	delete t_native_document;
}

void MCS_launch_url(char *p_document)
{
	MCS_do_launch(p_document);
	
	// MW-2007-12-13: <p_document> is owned by the callee
	delete p_document;
}

MCSysModuleHandle MCS_loadmodule(const char *p_filename)
{
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
}

void MCS_unloadmodule(MCSysModuleHandle p_module)
{
	FreeLibrary((HMODULE)p_module);
}

void *MCS_resolvemodulesymbol(MCSysModuleHandle p_module, const char *p_symbol)
{
	return GetProcAddress((HMODULE)p_module, p_symbol);
}

bool MCS_processtypeisforeground(void)
{
	return true;
}

bool MCS_changeprocesstype(bool to_foreground)
{
	if (!to_foreground)
		return false;
	return true;
}

bool MCS_isatty(int fd)
{
	return _isatty(fd) != 0;
}

bool MCS_isnan(double v)
{
	return _isnan(v) != 0;
}

uint32_t MCS_getsyserror(void)
{
	return GetLastError();
}

bool MCS_mcisendstring(const char *p_command, char p_buffer[256])
{
	DWORD t_error_code;
	t_error_code = mciSendStringA(p_command, p_buffer, 255, NULL);
	if (t_error_code == 0)
		return true;

	if (!mciGetErrorStringA(t_error_code, p_buffer, 255))
		sprintf(p_buffer, "Error %d", t_error_code);

	return false;
}

void MCS_system_alert(const char *p_title, const char *p_message)
{
	MessageBoxA(NULL, p_message, p_title, MB_OK | MB_ICONSTOP);
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

static bool write_blob_to_pipe(HANDLE p_pipe, uint32_t p_count, void *p_data)
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

////////////////////////////////////////////////////////////////////////////////

// MW-2010-05-09: Updated to add 'elevated' parameter for executing binaries
//   at increased privilege level.
void MCS_startprocess(char *name, char *doc, Open_mode mode, Boolean elevated)
{
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
}

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

////////////////////////////////////////////////////////////////////////////////
