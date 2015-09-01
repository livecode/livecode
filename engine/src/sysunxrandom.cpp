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

#include <unistd.h>
#include <fcntl.h>

////////////////////////////////////////////////////////////////////////////////

// MW-2013-05-21: [[ RandomBytes ]] System function for random bytes on Unix
//   systems (Linux and Android).
bool MCS_random_bytes(size_t p_count, MCDataRef& r_bytes)
{
	// Open the random device.
	int t_fd;
	t_fd = open("/dev/urandom", O_RDONLY);
	if (t_fd < 0)
		return false;
	
	// Loop until we've read enough bytes (or an error occurs)
    MCAutoByteArray t_bytes;
    t_bytes . New(p_count);
    
	size_t t_bytes_read;
	t_bytes_read = 0;
	while(t_bytes_read < p_count)
	{
		int t_read_count;
		t_read_count = read(t_fd, t_bytes . Bytes(), p_count - t_bytes_read);
		
		// If we read nothing, give up.
		if (t_read_count == 0)
			break;
		
		// If an non-interrupt error occured, give up.
		if (t_read_count < 0 && errno != EINTR)
			break;
		
		// Otherwise adjust count.
		t_bytes_read += t_read_count;
	}
	
	// Close the random device.
	close(t_fd);
	
	// If we read the correct number of bytes, then we are done.
	return t_bytes_read == p_count && t_bytes . CreateData(r_bytes);
	
}

////////////////////////////////////////////////////////////////////////////////
