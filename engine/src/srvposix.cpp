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

////////////////////////////////////////////////////////////////////////////////

bool MCS_get_temporary_folder(MCStringRef &r_temp_folder)
{
	bool t_success = true;

	MCAutoStringRef t_tmpdir_string;
    if (!MCS_getenv(MCSTR("TMPDIR"), &t_tmpdir_string))
        t_tmpdir_string = MCSTR("/tmp");

    if (!MCStringIsEmpty(*t_tmpdir_string))
    {
        if (MCStringGetNativeCharAtIndex(*t_tmpdir_string, MCStringGetLength(*t_tmpdir_string) - 1) == '/')
            t_success = MCStringCopySubstring(*t_tmpdir_string, MCRangeMake(0, MCStringGetLength(*t_tmpdir_string) - 1), r_temp_folder);
		else
            t_success = MCStringCopy(*t_tmpdir_string, r_temp_folder);
	}

	return t_success;
}

class MCStdioFileHandle;

bool MCS_create_temporary_file(MCStringRef p_path, MCStringRef p_prefix, IO_handle &r_file, MCStringRef &r_name)
{
    MCAutoCustomPointer<char,MCCStringFree> t_temp_file;
    if (!MCCStringFormat(&t_temp_file, "%s/%sXXXXXXXX", MCStringGetCString(p_path), MCStringGetCString(p_prefix)))
		return false;
	
	int t_fd;
    t_fd = mkstemp(*t_temp_file);
    if (t_fd == -1)
        return false;
	
    if (!MCStringCreateWithCString(*t_temp_file, r_name))
        return false;

    r_file = MCsystem->OpenFd(t_fd, kMCOpenFileModeWrite);
	return true;
}

bool MCSystemLockFile(IO_handle p_file, bool p_shared, bool p_wait)
{
    // FRAGILE? In case p_file is a MCMemoryMappedFile, getFilePointer returns a char*...
	int t_fd = fileno((FILE*)p_file->GetFilePointer());
	int32_t t_op = 0;
	
	if (p_shared)
		t_op = LOCK_SH;
	else
		t_op = LOCK_EX;
	
	if (!p_wait)
		t_op |= LOCK_NB;
	
	return 0 == flock(t_fd, t_op);
}
