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
#include "card.h"
#include "image.h"
#include "magnify.h"

#include "globals.h"

MCMagnify::MCMagnify()
{}

MCMagnify::MCMagnify(const MCMagnify &mref) : MCControl(mref)
{}

MCMagnify::~MCMagnify()
{}

Chunk_term MCMagnify::gettype() const
{
	return CT_MAGNIFY;
}

bool MCMagnify::visit_self(MCObjectVisitor* p_visitor)
{
    return p_visitor -> OnControl(this);
}

void MCMagnify::open()
{
	MCObject::open();
	MCmagnifier = this;
	inside = False;
}

void MCMagnify::close()
{
	MCObject::close();
	if (MCmagimage)
		MCmagimage->endmag(False);
	MCmagnifier = nil;
}

Boolean MCMagnify::kfocusnext(Boolean top)
{
	return False;
}

Boolean MCMagnify::kfocusprev(Boolean bottom)
{
	return False;
}

Boolean MCMagnify::mfocus(int2 x, int2 y)
{
	if (MCmagimage)
	{
		if (!inside)
		{
			getstack()->setcursor(MCmagimage->getstack()->getcursor(), True);
			inside = True;
		}
		return MCmagimage->magmfocus(x, y);
	}
	return False;
}

void MCMagnify::munfocus()
{
	inside = False;
}

Boolean MCMagnify::mdown(uint2 which)
{
	if (MCmagimage)
		return MCmagimage->magmdown(which);
	return False;
}

Boolean MCMagnify::mup(uint2 which, bool p_release)
{
	if (MCmagimage)
		return MCmagimage->magmup(which);
	return False;
}

Boolean MCMagnify::doubledown(uint2 which)
{
	if (MCmagimage)
		return MCmagimage->magdoubledown(which);
	return False;
}

Boolean MCMagnify::doubleup(uint2 which)
{
	if (MCmagimage)
		return MCmagimage->magdoubleup(which);
	return False;
}

IO_stat MCMagnify::save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;

	if ((stat = IO_write_uint1(OT_MAGNIFY, stream)) != IO_NORMAL)
		return stat;
	if ((stat = MCObject::save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
		return stat;

	return IO_NORMAL;
}

MCControl *MCMagnify::clone(Boolean attach, Object_pos p, bool invisible)
{
	MCMagnify *newmagnify = new (nothrow) MCMagnify(*this);
	if (attach)
		newmagnify->attach(p, invisible);
	return newmagnify;
}

// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
void MCMagnify::draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite)
{
	if (MCmagimage)
		MCmagimage->magredrawrect(dc, dirty);
}

IO_stat MCMagnify::load(IO_handle stream, uint32_t version)
{
	return MCObject::load(stream, version);
}

IO_stat MCMagnify::extendedload(MCObjectInputStream& p_stream, uint32_t p_version, uint4 p_length)
{
	return defaultextendedload(p_stream, p_version, p_length);
}

IO_stat MCMagnify::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version)
{
	return defaultextendedsave(p_stream, p_part, p_version);
}
