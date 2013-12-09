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
//#include "execpt.h"
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

#include "exec.h"

#include "w32dc.h"

#include <locale.h>
#include <signal.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <io.h>
#include <process.h>
#include <float.h>
#include <iphlpapi.h>

#ifdef /* MCS_loadfile_dsk_w32 */ LEGACY_SYSTEM
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
#endif /* MCS_loadfile_dsk_w32 */

#ifdef /* MCS_savefile_dsk_w32 */ LEGACY_SYSTEM
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
#endif /* MCS_savefile_dsk_w32 */