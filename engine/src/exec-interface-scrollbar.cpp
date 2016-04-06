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
#include "util.h"
#include "group.h"
#include "image.h"
#include "redraw.h"
#include "scrolbar.h"
#include "mctheme.h"

#include "exec-interface.h"

//////////

enum MCInterfaceScrollbarStyle
{
	kMCScrollbarStyleScrollbar,
	kMCScrollbarStyleScale,
	kMCScrollbarStyleProgress,
};

static MCExecEnumTypeElementInfo _kMCInterfaceScrollbarStyleElementInfo[] =
{	
	{ MCscrollbarstring, kMCScrollbarStyleScrollbar, false },
	{ MCscalestring, kMCScrollbarStyleScale, false },
	{ MCprogressstring, kMCScrollbarStyleProgress, false },
};

static MCExecEnumTypeInfo _kMCInterfaceScrollbarStyleTypeInfo =
{
	"Interface.ScrollbarStyle",
	sizeof(_kMCInterfaceScrollbarStyleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceScrollbarStyleElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceScrollbarOrientationElementInfo[] =
{	
	{ "vertical", F_VERTICAL, false },
	{ "horizontal", F_HORIZONTAL, false },
};

static MCExecEnumTypeInfo _kMCInterfaceScrollbarOrientationTypeInfo =
{
	"Interface.ScrollbarOrientation",
	sizeof(_kMCInterfaceScrollbarOrientationElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceScrollbarOrientationElementInfo
};

//////////

MCExecEnumTypeInfo *kMCInterfaceScrollbarStyleTypeInfo = &_kMCInterfaceScrollbarStyleTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceScrollbarOrientationTypeInfo = &_kMCInterfaceScrollbarOrientationTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCScrollbar::Redraw(bool dirty)
{
	flags |= F_SAVE_ATTS;

	if (!opened || !dirty)
		return;

	compute_barsize();
	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	redrawall();
}

////////////////////////////////////////////////////////////////////////////////

void MCScrollbar::GetStyle(MCExecContext& ctxt, intenum_t& r_style)
{
	if (flags & F_SCALE)
		r_style = kMCScrollbarStyleScale;
	else if (flags & F_PROGRESS)
		r_style = kMCScrollbarStyleProgress;
	else
		r_style = kMCScrollbarStyleScrollbar;
}

void MCScrollbar::SetStyle(MCExecContext& ctxt, intenum_t p_style)
{
	flags &= ~F_SB_STYLE;

	if (p_style == kMCScrollbarStyleScale)
		flags |= F_SCALE;
	else if (p_style == kMCScrollbarStyleProgress)
			flags |= F_PROGRESS;

	if (!(flags & F_SCALE))
		flags &= ~F_SHOW_VALUE;

	Redraw();
}

void MCScrollbar::GetThumbSize(MCExecContext& ctxt, double& r_size)
{
	r_size = thumbsize;
}

void MCScrollbar::SetThumbSize(MCExecContext& ctxt, double p_size)
{
	if (p_size != thumbsize)
	{
		thumbsize = p_size;
		update(thumbpos, MCM_scrollbar_drag);
		pageinc = thumbsize;
		lineinc = thumbsize / 16.0;
	}

	Redraw();
}

void MCScrollbar::GetThumbPos(MCExecContext& ctxt, double& r_pos)
{
    // AL-2014-07-22: [[ Bug 12843 ]] Round thumbpos according to scrollbar number format
    MCAutoStringRef t_formatted_thumbpos;
    if (MCU_r8tos(thumbpos, nffw, nftrailing, nfforce, &t_formatted_thumbpos) &&
        MCTypeConvertStringToReal(*t_formatted_thumbpos, r_pos))
        return;
    
    ctxt . Throw();
}

void MCScrollbar::SetThumbPos(MCExecContext& ctxt, double p_pos)
{
	update(p_pos, MCM_scrollbar_drag);
	Redraw();
}

void MCScrollbar::GetLineInc(MCExecContext& ctxt, double& r_inc)
{
	r_inc = lineinc;
}

void MCScrollbar::SetLineInc(MCExecContext& ctxt, double p_inc)
{
	lineinc = p_inc;
	if (startvalue < endvalue)
		lineinc = fabs(lineinc);
	else
		lineinc = -fabs(lineinc);
	Redraw();
}

void MCScrollbar::GetPageInc(MCExecContext& ctxt, double& r_inc)
{
	r_inc = pageinc;
}

void MCScrollbar::SetPageInc(MCExecContext& ctxt, double p_inc)
{
	pageinc = p_inc;
	if (startvalue < endvalue)
		pageinc = fabs(pageinc);
	else
		pageinc = -fabs(pageinc);
	Redraw();
}

void MCScrollbar::GetOrientation(MCExecContext& ctxt, intenum_t& r_style)
{
	r_style = getstyleint(flags);
}

void MCScrollbar::GetNumberFormat(MCExecContext& ctxt, MCStringRef& r_format)
{
	if (MCU_getnumberformat(nffw, nftrailing, nfforce, r_format))
		return;

	ctxt . Throw();
}

void MCScrollbar::SetNumberFormat(MCExecContext& ctxt, MCStringRef p_format)
{
	bool t_dirty = false;
	uint2 fw, trailing, force;
	MCU_setnumberformat(p_format, fw, trailing, force);
	if (nffw != fw || nftrailing != trailing || nfforce != force)
	{
		flags |= F_HAS_VALUES;
		nffw = fw;
		nftrailing = trailing;
		nfforce = force;
		t_dirty = true;
	}

	Redraw(t_dirty);
}


void MCScrollbar::GetStartValue(MCExecContext& ctxt, MCStringRef& r_value)
{
	// MW-2013-08-27: [[ UnicodifyScrollbar ]] Simply return the instance var.
	r_value = MCValueRetain(startstring);
}

void MCScrollbar::SetStartValue(MCExecContext& ctxt, MCStringRef p_value)
{
	// MW-2013-08-27: [[ UnicodifyScrollbar ]] Update to use MCStringRef startstring.
	if (MCStringIsEmpty(p_value))
	{
		reset();
		return;
	}

	if (!MCTypeConvertStringToReal(p_value, startvalue))
	{
		ctxt . LegacyThrow(EE_OBJECT_NAN);
		return;
	}
	
	if (startvalue == 0.0 && endvalue == 65535.0)
		reset();
	else
	{
		flags |= F_HAS_VALUES;
		MCValueAssign(startstring, p_value);
	}
	update(thumbpos, MCM_scrollbar_drag);
	Redraw();
}

void MCScrollbar::GetEndValue(MCExecContext& ctxt, MCStringRef& r_value)
{
	// MW-2013-08-27: [[ UnicodifyScrollbar ]] Simply return the instance var.
	r_value = MCValueRetain(endstring);
}

void MCScrollbar::SetEndValue(MCExecContext& ctxt, MCStringRef p_value)
{
	// MW-2013-08-27: [[ UnicodifyScrollbar ]] Update to use MCStringRef startstring.
	if (MCStringIsEmpty(p_value))
	{
		reset();
		return;
	}
	
	if (!MCTypeConvertStringToReal(p_value, endvalue))
	{
		ctxt . LegacyThrow(EE_OBJECT_NAN);
		return;
	}
	
	if (startvalue == 0.0 && endvalue == 65535.0)
		reset();
	else
	{
		flags |= F_HAS_VALUES;
		MCValueAssign(endstring, p_value);
	}
	update(thumbpos, MCM_scrollbar_drag);
	Redraw();
}

void MCScrollbar::GetShowValue(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_SHOW_VALUE);
}

void MCScrollbar::SetShowValue(MCExecContext& ctxt, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, F_SHOW_VALUE);

	if (!(flags & F_SCALE))
		flags &= ~F_SHOW_VALUE;

	Redraw(t_dirty);
}

MCPlatformControlType MCScrollbar::getcontroltype()
{
    MCPlatformControlType t_type;
    t_type = MCObject::getcontroltype();
    
    if (t_type != kMCPlatformControlTypeGeneric)
        return t_type;
    else
        t_type = kMCPlatformControlTypeScrollBar;
    
    if ((flags & F_SB_STYLE) == F_SCALE)
        t_type = kMCPlatformControlTypeSlider;
    else if ((flags & F_SB_STYLE) == F_PROGRESS)
        t_type = kMCPlatformControlTypeProgressBar;
    else if (getwidgetthemetype() == WTHEME_TYPE_SMALLSCROLLBAR)
        t_type = kMCPlatformControlTypeSpinArrows;
    
    return t_type;
}
