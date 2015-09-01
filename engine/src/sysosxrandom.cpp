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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "globals.h"

#include <Carbon/Carbon.h>

typedef const struct __SecRandom * SecRandomRef;
#define kSecRandomDefault (NULL)

typedef int (*SecRandomCopyBytesPtr)(SecRandomRef p_rnd, size_t p_count, uint8_t *p_bytes);

static SecRandomCopyBytesPtr s_sec_random_copy_bytes = nil;

////////////////////////////////////////////////////////////////////////////////

// MW-2013-05-21: [[ RandomBytes ]] Implementation of random byte generation for
//   Mac (Desktop and Server).
bool MCS_random_bytes(size_t p_count, MCDataRef& r_buffer)
{
	// IM-2014-08-06: [[ Bug 13038 ]] Enable this implementation on OSX + server
	// so we can remove the reliance on SSL from MCU_random_bytes
	
	// Now, on Lion and above SecRandomCopyBytes is available, so use that if
	// possible. ( Note that as we can't link against the 10.7 SDK, we have to
	// weak-link manually :( ).
    
    uint8_t *t_buffer;
    /* UNCHECKED */ MCMemoryAllocate(p_count, t_buffer);
	if (MCmajorosversion >= 0x1070)
	{
		if (s_sec_random_copy_bytes == nil)
		{
			CFBundleRef t_security_bundle;
			t_security_bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.security"));
			if (t_security_bundle != nil)
			{
				s_sec_random_copy_bytes = (SecRandomCopyBytesPtr)CFBundleGetFunctionPointerForName(t_security_bundle, CFSTR("SecRandomCopyBytes"));
				CFRelease(t_security_bundle);
			}
		}
		
		if (s_sec_random_copy_bytes != nil)
        {
			// IM-2014-04-16: [[ Bug 11860 ]] SecRandomCopyBytes returns 0 on success
			if (0 == s_sec_random_copy_bytes(NULL, p_count, t_buffer))
                return MCDataCreateWithBytesAndRelease((byte_t*)t_buffer, p_count, r_buffer);
            
            return false;
        }
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
	t_bytes = t_buffer;
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
	if (t_bytes_read == p_count)
        return MCDataCreateWithBytesAndRelease((byte_t*)t_buffer, p_count, r_buffer);
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////
