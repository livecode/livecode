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

//
// MAC plaform specific routines  MACSPEC.CPP
//

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "typedefs.h"
#include "mcio.h"

#include "mcerror.h"
#include "execpt.h"
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
#include "license.h"
#include "mode.h"
#include "socket.h"
#include "osspec.h"

#include "osxdc.h"

#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <Security/Authorization.h>
#include <Security/AuthorizationTags.h>

bool MCS_generate_uuid(char p_buffer[128])
{
	CFUUIDRef t_uuid;
	t_uuid = CFUUIDCreate(kCFAllocatorDefault);
	if (t_uuid != NULL)
	{
		CFStringRef t_uuid_string;
		
		t_uuid_string = CFUUIDCreateString(kCFAllocatorDefault, t_uuid);
		if (t_uuid_string != NULL)
		{
			CFStringGetCString(t_uuid_string, p_buffer, 127, kCFStringEncodingMacRoman);
			CFRelease(t_uuid_string);
		}
		
		CFRelease(t_uuid);

		return true;
	}

	return false;
}
