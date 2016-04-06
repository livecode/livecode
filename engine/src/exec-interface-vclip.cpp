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
#include "mcio.h"
#include "sysdefs.h"

#include "globals.h"
#include "object.h"
#include "stack.h"
#include "cdata.h"
#include "objptr.h"
#include "field.h"
#include "object.h"
#include "button.h"
#include "card.h"
#include "exec.h"
#include "vclip.h"

#include "exec-interface.h"

//////////

void MCVideoClip::GetDontRefresh(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_DONT_REFRESH);	
}

void MCVideoClip::SetDontRefresh(MCExecContext& ctxt, bool setting)
{
	changeflag(setting, F_DONT_REFRESH);
}

void MCVideoClip::GetFrameRate(MCExecContext& ctxt, integer_t*& r_rate)
{
	if (flags & F_FRAME_RATE)
	
		*r_rate = (integer_t)framerate;
	else
		r_rate = nil;
}

void MCVideoClip::SetFrameRate(MCExecContext& ctxt, integer_t* p_rate)
{
	if (p_rate == nil)
		flags &= ~F_FRAME_RATE;
	else
	{
		framerate = *p_rate;
		flags |= F_FRAME_RATE;
	}
}

void MCVideoClip::GetScale(MCExecContext& ctxt, double& r_scale)
{
	r_scale = scale;
}

void MCVideoClip::SetScale(MCExecContext& ctxt, double p_scale)
{
	scale = p_scale;
	flags |= F_SCALE_FACTOR;
}

void MCVideoClip::GetSize(MCExecContext& ctxt, integer_t& r_size)
{
	r_size = size;
}

void MCVideoClip::GetText(MCExecContext& ctxt, MCStringRef& r_text)
{
	if (MCStringCreateWithNativeChars((const char_t *)frames, size, r_text))
		return;

	ctxt . Throw();
}

void MCVideoClip::SetText(MCExecContext& ctxt, MCStringRef p_text)
{
	delete frames;
    
    uindex_t t_size;
    MCStringConvertToNative(p_text, frames, t_size);
    size = t_size;
}
