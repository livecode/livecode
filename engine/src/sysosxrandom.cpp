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

#include "mcssl.h"

////////////////////////////////////////////////////////////////////////////////

// MW-2013-05-21: [[ RandomBytes ]] Implementation of random byte generation for
//   Mac (Desktop and Server).
bool MCS_random_bytes(size_t p_count, void *p_buffer)
{
	// On Mac (both Server and Desktop) OpenSSL is always available so we will
	// never get here.
	return false;
}

////////////////////////////////////////////////////////////////////////////////

// This is alternative code which doesn't rely on OpenSSL (not needed now, but
// maybe in the future).
#if 0
{
	// Now, on Lion and above SecRandomCopyBytes is available, so use that if
	// possible. ( Note that as we can't link against the 10.7 SDK, we have to
	// weak-link manually :( ).
	if (MCmajorosversion >= 0x1070)
	{
		if (s_sec_random_copy_bytes == nil)
		{
			CFBundleRef t_security_bundle;
			t_security_bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.Security"));
			if (t_security_bundle != nil)
			{
				s_sec_random_copy_bytes = (SecRandomCopyBytesPtr)CFBundleGetFunctionPointerForName(t_security_bundle, CFSTR("SetRandomCopyBytes"));
				CFRelease(t_security_bundle);
			}
		}
		
		if (s_sec_random_copy_bytes != nil)
			return s_sec_random_copy_bytes(NULL, p_count, (uint8_t *)p_buffer);
			}
	
	// Otherwise attempt to use /dev/urandom
	int t_fd;
	t_fd = open("/dev/urandom", O_RDONLY);
	if (t_fd < 0)
		return false;
	
	// Loop until we've read enough bytes (or an error occurs)
	uint8_t *t_bytes;
	size_t t_bytes_read;
	t_bytes_read = 0;
	t_bytes = (uint8_t *)p_buffer;
	while(t_bytes_read < p_count)
	{
		int t_read_count;
		t_read_count = read(t_fd, t_bytes, p_count - t_bytes_read);
		
		// If we read nothing, give up.
		if (t_read_count == 0)
			break;
		
		// If an non-interrupt error occured, give up.
		if (t_read_count < 0 && errno != EINTR)
			break;
		
		// Otherwise advance pointers, adjust counts.
		t_bytes += t_read_count;
		t_bytes_read += t_read_count;
	}
	
	// Close the random device.
	close(t_fd);
	
	// If we read the correct number of bytes, then we are done.
	return t_bytes_read == p_count;
}
#endif

////////////////////////////////////////////////////////////////////////////////
