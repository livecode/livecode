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
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "mode.h"

#include "execpt.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "globals.h"
#include "util.h"

#include <locale.h>
#include <iconv.h>
#include <langinfo.h>

////////////////////////////////////////////////////////////////////////////////

bool X_init(int argc, MCStringRef argv[], MCStringRef envp[]);
void X_main_loop_iteration();
int X_close();

bool MCStringCreateWithSysString(const char *p_system_string, size_t p_len, MCStringRef &r_string);
bool MCStringConvertToSysString(MCStringRef p_string, const char * &r_system_string, size_t &r_len);

////////////////////////////////////////////////////////////////////////////////

void X_main_loop(void)
{
	while(!MCquit)
		X_main_loop_iteration();
}

int main(int argc, char *argv[], char *envp[])
{
	// On Linux, the argv and envp could be in pretty much any format. The
	// safest thing to do is let the C library's iconv convert to a known
	// format. To do this, the system locale needs to be retrieved.
	setlocale(LC_ALL, "");
	MCsysencoding = strclone(nl_langinfo(CODESET));
	
	// Convert the argv array to StringRefs
	MCStringRef* t_argv;
	/* UNCHECKED */ MCMemoryNewArray(argc, t_argv);
	for (int i = 0; i < argc; i++)
	{
		/* UNCHECKED */ MCStringCreateWithSysString(argv[i], strlen(argv[i]), t_argv[i]);
	}
	
	// Convert the envp array to StringRefs
	int envc = 0;
	MCStringRef* t_envp = nil;
	while (envp[envc] != NULL)
	{
		uindex_t t_count = envc + 1;
		/* UNCHECKED */ MCMemoryResizeArray(t_count + 1, t_envp, t_count);
		/* UNCHECKED */ MCStringCreateWithSysString(envp[envc], strlen(envp[envc]), t_envp[envc]);
		envc++;
	}
	
	// Terminate the envp array
	t_envp[envc] = nil;
	
	extern int MCSystemElevatedMain(int, char* argv[]);
	if (argc == 3&& strcmp(argv[1], "-elevated-slave") == 0)
		return MCSystemElevatedMain(argc, argv);
	
	if (!MCInitialize())
		exit(-1);

	if (!X_init(argc, t_argv, t_envp))
	{
		MCExecPoint ep;
		if (MCresult != nil)
		{
			MCresult -> eval(ep);
			fprintf(stderr, "Startup error - %s\n", ep . getcstring());
		}
		exit(-1);
	}
	
	// Clean up the created argv/envp StringRefs
	for (int i = 0; i < argc; i++)
		MCValueRelease(t_argv[i]);
	for (int i = 0; i < envc; i++)
		MCValueRelease(t_envp[i]);
	MCMemoryDeleteArray(t_argv);
	MCMemoryDeleteArray(t_envp);

	X_main_loop();

	int t_exit_code = X_close();

	MCFinalize();

	exit(t_exit_code);
}

////////////////////////////////////////////////////////////////////////////////

static bool do_iconv(iconv_t fd, const char *in, size_t in_len, char * &out, size_t &out_len)
{
	// Begin conversion. As a start, assume both encodings take the same
	// space. This is probably wrong but the array is grown as needed.
	size_t t_status = 0;
	uindex_t t_alloc_remain = 0;
	MCAutoArray<char> t_out;
	char * t_out_cursor;
	t_out.New(in_len);
	t_alloc_remain = t_out.Size();
	t_out_cursor = t_out.Ptr();
	while (in_len > 0)
	{
		// Resize the destination array if it has been exhausted
		t_status = iconv(fd, (char**)&in, &in_len, &t_out_cursor, &t_alloc_remain);
		
		// Did the conversion fail?
		if (t_status == (size_t)-1)
		{
			// Insufficient output space errors can be fixed and retried
			if (errno == E2BIG)
			{
				// Increase the size of the output array
				uindex_t t_offset;
				t_offset = t_out_cursor - t_out.Ptr();
				t_out.Extend(t_offset + t_alloc_remain + 1);
				
				// Adjust the pointers because the output buffer may have moved
				t_out_cursor = t_out.Ptr() + t_offset;
				t_alloc_remain = t_out.Size() - t_offset;		// Remaining size, not total size
				
				// Try the conversion again
				continue;
			}
			else
			{
				// The error is one of the following:
				//	EILSEQ	-	input byte invalid for input encoding
				//	EINVAL	-	incomplete multibyte character at end of input
				//	EBADF	-	invalid conversion file descriptor
				// None of these are recoverable so abort
				return false;
			}
		}
		else
		{
			// No error, conversion should be complete.
			MCAssert(in_len == 0);
		}
	}
	
	// Conversion has been completed
	t_out.Take(out, out_len);
	return true;
}

bool MCStringCreateWithSysString(const char *p_system_string, size_t p_len, MCStringRef &r_string)
{
	// Create the pseudo-FD that iconv uses for character conversion. The most
	// convenient form is UTF-16 as StringRefs can be constructed directly from that.
	iconv_t t_fd = iconv_open("UTF-16", MCsysencoding);
	
	// Convert the string
	char *t_utf16_bytes;
	size_t t_utf16_byte_len;
	bool t_success;
	t_success = do_iconv(t_fd, p_system_string, p_len, t_utf16_bytes, t_utf16_byte_len);
	iconv_close(t_fd);
	
	if (!t_success)
		return false;
	
	// Create the StringRef
	MCStringRef t_string;
	t_success = MCStringCreateWithBytes((const byte_t*)t_utf16_bytes, t_utf16_byte_len, kMCStringEncodingUTF16, false, t_string);
	MCMemoryDeleteArray(t_utf16_bytes);
	
	if (!t_success)
		return false;
	
	r_string = t_string;
	return true;
}

bool MCStringConvertToSysString(MCStringRef p_string, const char * &r_system_string, size_t &r_len)
{
	// Create the pseudo-FD that iconv uses for character conversion. For
	// efficiency, convert straight from the internal format.
	iconv_t t_fd;
	const char *t_mc_string;
	size_t t_mc_len;
	if (MCStringIsNative(p_string) && MCStringGetNativeCharPtr(p_string) != nil)
	{
		t_fd = iconv_open(MCsysencoding, "ISO-8859-1");
		t_mc_string = (const char *)MCStringGetNativeCharPtr(p_string);
		t_mc_len = MCStringGetLength(p_string);
	}
	else
	{
		t_fd = iconv_open(MCsysencoding, "UTF-16");
		t_mc_string = (const char *)MCStringGetCharPtr(p_string);
		t_mc_len = MCStringGetLength(p_string) * sizeof(unichar_t);
	}
	
	// Perform the conversion
	bool t_success;
	char *t_sys_string;
	size_t t_sys_len;
	t_success = do_iconv(t_fd, t_mc_string, t_mc_len, t_sys_string, t_sys_len);
	iconv_close(t_fd);
	
	if (!t_success)
		return false;
	
	r_system_string = t_sys_string;
	r_len = t_sys_len;
	return true;
}
