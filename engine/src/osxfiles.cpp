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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "typedefs.h"
#include "mcio.h"

#include "mcerror.h"
//#include "execpt.h"
#include "handler.h"
#include "util.h"

#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "button.h"
#include "control.h"
#include "param.h"
#include "securemode.h"
#include "osspec.h"

#include "license.h"

#include <sys/stat.h>
#include <sys/utsname.h>

#ifdef /* MCS_loadfile_dsk_mac */ LEGACY_SYSTEM
void MCS_loadfile(MCExecPoint &ep, Boolean binary)
{
	if (!MCSecureModeCanAccessDisk())
	{
		ep.clear();
		MCresult->sets("can't open file");
		return;
	}
	char *tpath = ep.getsvalue().clone();
	
	// MW-2010-10-17: [[ Bug 5246 ]] MCS_resolvepath can return nil if an unknown ~ is used.
	char *t_resolved_path;
	t_resolved_path = MCS_resolvepath(tpath);
	
	if (t_resolved_path == NULL)
	{
		MCresult -> sets("bad path");
		return;
	}
	
	char *newpath = path2utf(t_resolved_path);
	
	ep.clear();
	delete tpath;
	FILE *fptr = fopen(newpath, IO_READ_MODE);
	delete newpath;
	struct stat buf;
	if (fptr == NULL || fstat(fileno(fptr), (struct stat *)&buf))
		MCresult->sets("can't open file");
	else
	{
		char *buffer;
		/* UNCHECKED */ ep . reserve(buf . st_size, buffer); 
		if (buffer == NULL)
		{
			ep.clear();
			MCresult->sets("can't create data buffer");
		}
		else
		{
			uint4 tsize = fread(buffer, 1, buf.st_size, fptr);
			if (tsize != buf.st_size)
			{
				ep.clear();
				MCresult->sets("error reading file");
			}
			else
			{
				ep . commit(tsize);
				if (!binary)
					ep.texttobinary();
				MCresult->clear(False);
			}
			fclose(fptr);
		}
	}
}
#endif /* MCS_loadfile_dsk_mac */

#ifdef /* MCS_savefile_dsk_mac */ LEGACY_SYSTEM
void MCS_savefile(const MCString &fname, MCExecPoint &data, Boolean binary)
{
	if (!MCSecureModeCanAccessDisk())
	{
		MCresult->sets("can't open file");
		return;
	}
	char *tpath = fname.clone();
	
	char *t_resolved_path;
	t_resolved_path = MCS_resolvepath(tpath);

	if (t_resolved_path == NULL)
	{
		MCresult -> sets("bad path");
		return;
	}
	char *newpath = path2utf(t_resolved_path);

	FILE *fptr = fopen(newpath, IO_WRITE_MODE);
	if (fptr == NULL)
		MCresult->sets("can't open file");
	else
	{
		if (!binary)
			data.binarytotext();
		uint4 toWrite = data.getsvalue().getlength();
		if (fwrite(data.getsvalue().getstring(), 1, toWrite, fptr) != toWrite)
			MCresult->sets("error writing file");
		else
		{
			SetEOF(fileno(fptr), toWrite);
			MCresult->clear(False);
		}
		fclose(fptr);


		tpath = fname.clone();
		MCS_setfiletype(tpath);
	}
	delete tpath;
	delete newpath;
}
#endif /* MCS_savefile_dsk_mac */
