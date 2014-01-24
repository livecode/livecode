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

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"

#include "graphics.h"
#include "resolution.h"

// IM-2014-01-24: [[ HiDPI ]] Removed unused "use system pixel scale" functions

static MCGFloat s_res_pixel_scale = 1.0;

void MCResHandleScaleChanged()
{
	// Trigger update of stack windows
	MCdispatcher->sync_stack_windows();
}

MCGFloat MCResGetPixelScale(void)
{
	return s_res_pixel_scale;
}

void MCResSetPixelScale(MCGFloat p_scale, bool p_send_update)
{
	MCGFloat t_old_scale;
	t_old_scale = MCResGetPixelScale();
	
	s_res_pixel_scale = p_scale;
	
	if (p_send_update && p_scale != t_old_scale)
		MCResHandleScaleChanged();
}
