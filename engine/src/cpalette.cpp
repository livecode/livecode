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


#include "util.h"
#include "sellst.h"
#include "stack.h"
#include "cpalette.h"
#include "mcerror.h"

#include "globals.h"

#include "context.h"

MCColors::MCColors()
{
	selectedcolor = 0;
}

MCColors::MCColors(const MCColors &cref) : MCControl(cref)
{
	selectedcolor = 0;
}

MCColors::~MCColors()
{}

Chunk_term MCColors::gettype() const
{
	return CT_COLOR_PALETTE;
}

const char *MCColors::gettypestring()
{
	return MCcolorstring;
}

bool MCColors::visit_self(MCObjectVisitor* p_visitor)
{
    return p_visitor -> OnControl(this);
}

static void getcells(uint2 &xcells, uint2 &ycells)
{
	switch (MCscreen->getdepth())
	{
	case 1:
		xcells = 2;
		ycells = 1;
		break;
	case 2:
		xcells = 2;
		ycells = 2;
		break;
	case 4:
		xcells = 4;
		ycells = 4;
		break;
	case 6:
		xcells = 8;
		ycells = 8;
		break;
	default:
		xcells = 16;
		ycells = 16;
		break;
	}
}

Boolean MCColors::mfocus(int2 x, int2 y)
{
	if (!(flags & F_VISIBLE || showinvisible()))
		return False;
	if (state & CS_MOVE || state & CS_SIZE)
		return MCControl::mfocus(x, y);
	mx = x;
	my = y;
	if (MCU_point_in_rect(rect, x, y))
	{
		switch (getstack()->gettool(this))
		{
		case T_BROWSE:
		case T_POINTER:
			return True;
		default:
			return False;
		}
	}
	else
		if (state & CS_MFOCUSED)
			return True;
		else
			return False;
}

Boolean MCColors::mdown(uint2 which)
{
	if (state & CS_MFOCUSED)
		return False;
	state |= CS_MFOCUSED;
	MCColor color;
	switch (which)
	{
	case Button1:
		switch (getstack()->gettool(this))
		{
		case T_BROWSE:
			uint2 xcells;
			uint2 ycells;
			getcells(xcells, ycells);
			MCscreen->getpaletteentry((my - rect.y) * ycells / rect.height * xcells
			                          + (mx - rect.x) * xcells / rect.width, color);
			selectedcolor = MCColorGetPixel(color);
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			layer_redrawall();
			message_with_valueref_args(MCM_mouse_down, MCSTR("1"));
			break;
		case T_POINTER:
			start(True);
			break;
		default:
			return False;
		}
		break;
	case Button2:
	case Button3:
		message_with_args(MCM_mouse_down, which);
		break;
	}
	return True;
}

Boolean MCColors::mup(uint2 which, bool p_release)
{
	if (!(state & CS_MFOCUSED))
		return False;
	state &= ~CS_MFOCUSED;
	switch (which)
	{
	case Button1:
		switch (getstack()->gettool(this))
		{
		case T_BROWSE:
			message_with_valueref_args(MCM_mouse_up, MCSTR("1"));
			break;
		case T_POINTER:
			end(true, p_release);
			break;
		default:
			return False;
		}
		break;
	case Button2:
	case Button3:
		message_with_args(MCM_mouse_up, which);
		break;
	}
	return True;
}

Boolean MCColors::count(Chunk_term type, MCObject *stop, uint2 &num)
{
	if (type == CT_COLOR_PALETTE || type == CT_LAYER)
		num++;
	if (stop == this)
		return True;
	return False;
}

MCControl *MCColors::clone(Boolean attach, Object_pos p, bool invisible)
{
	MCColors *newcolors = new (nothrow) MCColors(*this);
	if (attach)
		newcolors->attach(p, invisible);
	return newcolors;
}

// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
void MCColors::draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite)
{
	uint2 xcells;
	uint2 ycells;
	getcells(xcells, ycells);

	uint2 i;
	uint2 j;
	MCRectangle rrect = MCU_reduce_rect(rect, borderwidth);
	MCRectangle trect;
	MCColor c;

	c = dc->getwhite();
	dc->setfillstyle(FillSolid, nil, 0, 0);
	for (i = 0 ; i < ycells ; i++)
		for (j = 0 ; j < xcells ; j++)
		{
			trect.x = j * rrect.width / xcells;
			trect.y = i * rrect.height / ycells;
			trect.width = (j + 1) * rrect.width / xcells - trect.x;
			trect.height = (i + 1) * rrect.height / ycells - trect.y;
			trect.x += rrect.x;
			trect.y += rrect.y;
			MCscreen->getpaletteentry(i * xcells + j, c);
			dc->setforeground(c);
			dc->fillrect(trect);
			if (MCColorGetPixel(c) == selectedcolor)
				draw3d(dc, trect, ETCH_SUNKEN, borderwidth);
		}
	if (flags & F_SHOW_BORDER)
	{
		if (flags & F_3D)
			draw3d(dc, rect, ETCH_SUNKEN, borderwidth);
		else
			drawborder(dc, rect, borderwidth);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  SAVING AND LOADING
//

IO_stat MCColors::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version)
{
	return MCObject::defaultextendedsave(p_stream, p_part, p_version);
}

IO_stat MCColors::extendedload(MCObjectInputStream& p_stream, uint32_t p_version, uint4 p_length)
{
	return MCObject::defaultextendedload(p_stream, p_version, p_length);
}

IO_stat MCColors::save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;

	if ((stat = IO_write_uint1(OT_COLORS, stream)) != IO_NORMAL)
		return stat;
	if ((stat = MCObject::save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
		return stat;
	return savepropsets(stream, p_version);
}

IO_stat MCColors::load(IO_handle stream, uint32_t version)
{
	IO_stat stat;
	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return checkloadstat(stat);
	return loadpropsets(stream, version);
}
