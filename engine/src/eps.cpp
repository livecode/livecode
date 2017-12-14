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
#include "mcerror.h"
#include "sellst.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "eps.h"

#include "globals.h"
#include "context.h"

#include "stackfileformat.h"

real8 MCEPS::xf;
real8 MCEPS::yf;

MCEPS::MCEPS()
{
	flags |= F_RETAIN_POSTSCRIPT;
	size = 0;
	postscript = MCU_empty();
	prolog = MCU_empty();
	tx = 0;
	ty = 0;
	ex = 612;
	ey = 792;
	angle = 0;
	xscale = yscale = 0.5;
	xf = (MCscreen->getwidthmm() * 72.0) / (MCscreen->getwidth() * 25.4);
	yf = (MCscreen->getheightmm() * 72.0)  / (MCscreen->getheight() * 25.4);
	rect.width = (uint2)(ex * xscale / xf);
	rect.height = (uint2)(ey * yscale / yf);
	if (flags & F_SHOW_BORDER)
	{
		rect.width += borderwidth << 1;
		rect.height += borderwidth << 1;
	}
	image = NULL;
	pagecount = 0;
	curpage = 1;
	pageIndex = NULL;
}

MCEPS::MCEPS(const MCEPS &sref) : MCControl(sref)
{
	size = sref.size;
	postscript = strclone(sref.postscript);
	prolog = strclone(sref.prolog);
	xscale = sref.xscale;
	yscale = sref.yscale;
	angle = sref.angle;
	tx = sref.tx;
	ty = sref.ty;
	ex = sref.ex;
	ey = sref.ey;
	curpage = sref.curpage;
	pagecount = sref.pagecount;
	if (pagecount != 0)
	{
		pageIndex = new (nothrow) uint4[pagecount];
		uint2 i = pagecount;
		while (i--)
			pageIndex[i] = sref.pageIndex[i];
	}
	else
		pageIndex = NULL;
	if (sref.image == NULL)
		image = NULL;
	else
	{
		image = new (nothrow) MCImage(*sref.image);
		image->setparent(this);
	}
}

MCEPS::~MCEPS()
{
	delete[] postscript; /* Allocated with new[] */
	delete[] prolog; /* Allocated with new [] */
	delete pageIndex;
	delete image;
}

Chunk_term MCEPS::gettype() const
{
	return CT_EPS;
}

const char *MCEPS::gettypestring()
{
	return MCepsstring;
}

bool MCEPS::visit_self(MCObjectVisitor* p_visitor)
{
    return p_visitor -> OnEps(this);
}

Boolean MCEPS::mdown(uint2 which)
{
	if (state & CS_MFOCUSED)
		return False;
	state |= CS_MFOCUSED;
	switch (which)
	{
	case Button1:
		switch (getstack()->gettool(this))
		{
		case T_BROWSE:
			message_with_valueref_args(MCM_mouse_down, MCSTR("1"));
			break;
		case T_POINTER:
			start(True);
			break;
		case T_HELP:
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

Boolean MCEPS::mup(uint2 which, bool p_release)
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
			if (!p_release && MCU_point_in_rect(rect, mx, my))
                message_with_valueref_args(MCM_mouse_up, MCSTR("1"));
            else
                message_with_valueref_args(MCM_mouse_release, MCSTR("1"));
			break;
		case T_POINTER:
			end(true, p_release);
			break;
		case T_HELP:
			help();
			break;
		default:
			return False;
		}
		break;
	case Button2:
	case Button3:
		if (!p_release && MCU_point_in_rect(rect, mx, my))
			message_with_args(MCM_mouse_up, which);
        else
            message_with_args(MCM_mouse_release, which);
		break;
	}
	return True;
}

void MCEPS::applyrect(const MCRectangle &nrect)
{
	if (rect.width != nrect.width || rect.height != nrect.height)
	{
		rect = nrect;
		resetscale();
	}
	else
		rect = nrect;
}

IO_stat MCEPS::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version)
{
	return defaultextendedsave(p_stream, p_part, p_version);
}

IO_stat MCEPS::extendedload(MCObjectInputStream& p_stream, uint32_t p_version, uint4 p_length)
{
	return defaultextendedload(p_stream, p_version, p_length);
}

IO_stat MCEPS::save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;

	if ((stat = IO_write_uint1(OT_MCEPS, stream)) != IO_NORMAL)
		return stat;
	if ((stat = MCObject::save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint4(size, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write(postscript, sizeof(char), size, stream)) != IO_NORMAL)
		return stat;
	// MW-2013-11-19: [[ UnicodeFileFormat ]] EPS is always ASCII so legacy.
	if ((stat = IO_write_cstring_legacy(prolog, stream, 2)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_int4(MCU_r8toi4(xscale), stream)) != IO_NORMAL)
		return stat;
	if (flags & F_SCALE_INDEPENDENTLY)
		if ((stat = IO_write_int4(MCU_r8toi4(yscale), stream)) != IO_NORMAL)
			return stat;
	if ((stat = IO_write_int2(angle, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_int2(tx, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_int2(ty, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(ex, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(ey, stream)) != IO_NORMAL)
		return stat;
	if (flags & F_RETAIN_IMAGE)
		if ((stat = image->save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
			return stat;
	if ((stat = IO_write_uint2(curpage, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(pagecount, stream)) != IO_NORMAL)
		return stat;
	uint2 i;
	for (i = 0 ; i < pagecount ; i++)
		if ((stat = IO_write_uint4(pageIndex[i], stream)) != IO_NORMAL)
			return stat;
	return savepropsets(stream, p_version);
}

MCControl *MCEPS::clone(Boolean attach, Object_pos p, bool invisible)
{
	MCEPS *neweps = new (nothrow) MCEPS(*this);
	if (attach)
		neweps->attach(p, invisible);
	return neweps;
}

// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
void MCEPS::draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite)
{
	MCRectangle trect = rect;
	if (flags & F_SHOW_BORDER)
		trect = MCU_reduce_rect(trect, borderwidth);

	if (flags & F_OPAQUE)
	{
		setforeground(dc, DI_BACK, False);
		dc->fillrect(trect);
	}
	if (state & (CS_SIZE | CS_MOVE))
	{
		dc->setlineatts(0, LineDoubleDash, CapButt, JoinBevel);
		dc->setforeground(dc->getblack());
		dc->setbackground(dc->getwhite());
		dc->setfillstyle(FillSolid, nil, 0, 0);
		dc->setdashes(0, dashlist, 2);
		MCLineSegment segs[2];
		segs[0].x1 = segs[1].x1 = trect.x;
		segs[0].x2 = segs[1].x2 = trect.x + trect.width;
		segs[0].y1 = segs[1].y2 = trect.y;
		segs[0].y2 = segs[1].y1 = trect.y + trect.height;
		dc->drawsegments(segs, 2);
		dc->setlineatts(0, LineSolid, CapButt, JoinBevel);
		dc->setbackground(MCzerocolor);
	}

#ifdef HAVE_EPS
	else
	{
		const char *psprolog = postscript;
		uint4 psprologlength = 0;
		const char *ps;
		uint4 length;
		if ((pagecount == 0) || (pagecount == 1))
		{
			ps = postscript;
			length = strlen(postscript);
		}
		else
		{
			psprologlength = pageIndex[0];
			ps = &postscript[pageIndex[curpage - 1] - 1];
			if (curpage != pagecount)
				length = pageIndex[curpage] - pageIndex[curpage - 1];
			else
				length = size - pageIndex[curpage - 1];
		}
		setforeground(dc, DI_FORE, False);
		const char *fontname;
		uint2 fheight, fontsize, fontstyle;
		MCFontStruct *font;
		getfontatts(fontname, fheight, fontsize, fontstyle, font);
		dc->draweps(trect.x * xf, -(trect.y * yf), angle, xscale, yscale,
		            tx, ty, prolog, psprolog, psprologlength, ps, length,
		            fontname, fontsize, fontstyle, font, trect);
	}
#endif

	if (flags & F_SHOW_BORDER)
		trect = MCU_reduce_rect(trect, -borderwidth);
	if (flags & F_SHOW_BORDER)
    {
		if (flags & F_3D)
			draw3d(dc, trect, ETCH_SUNKEN, borderwidth);
		else
			drawborder(dc, trect, borderwidth);
    }
	if (getstate(CS_KFOCUSED))
		drawfocus(dc, dirty);
}

IO_stat MCEPS::load(IO_handle stream, uint32_t version)
{
	IO_stat stat;

	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return checkloadstat(stat);
	delete postscript;
	delete prolog;
	if ((stat = IO_read_uint4(&size, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	postscript = new (nothrow) char[size + 1];
	if ((stat = IO_read(postscript, size, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	postscript[size] = '\0';
	// MW-2013-11-19: [[ UnicodeFileFormat ]] EPS is always ASCII so legacy.
	if ((stat = IO_read_cstring_legacy(prolog, stream, 2)) != IO_NORMAL)
		return checkloadstat(stat);
	int4 i;
	if ((stat = IO_read_int4(&i, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	xscale = MCU_i4tor8(i);
	if (flags & F_SCALE_INDEPENDENTLY)
	{
		if ((stat = IO_read_int4(&i, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		yscale = MCU_i4tor8(i);
	}
	else
		yscale = xscale;
	if ((stat = IO_read_int2(&angle, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_int2(&tx, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_int2(&ty, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_uint2(&ex, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_uint2(&ey, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if (flags & F_RETAIN_IMAGE)
	{
		image = new (nothrow) MCImage;
		image->setparent(this);
		if ((stat = image->load(stream, version)) != IO_NORMAL)
			return checkloadstat(stat);
	}
	if (version > kMCStackFileFormatVersion_1_3)
	{
		if ((stat = IO_read_uint2(&curpage, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if ((stat = IO_read_uint2(&pagecount, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if (pagecount > 0)
		{
			pageIndex = new (nothrow) uint4[pagecount];
			for (i = 0 ; i < pagecount ; i++)
				if ((stat = IO_read_uint4(&pageIndex[i], stream)) != IO_NORMAL)
					return checkloadstat(stat);
		}
	}
	return loadpropsets(stream, version);
}

void MCEPS::setextents()
{
	tx = 0;
	ty = 0;
	ex = 612;
	ey = 792;

	uint4 offset;
	uint1 *bptr = (uint1 *)postscript;
	if (*bptr == 0xC5)
	{
		MCswapbytes = !MCswapbytes;
		uint4 *uint4ptr = (uint4 *)postscript;
		uint4 t_offset = swap_uint4(&uint4ptr[1]);
		size = swap_uint4(&uint4ptr[2]);
		MCswapbytes = !MCswapbytes;
		char *newps = new (nothrow) char[size];
		memcpy(newps, &postscript[t_offset], size);
		delete postscript;
		postscript = newps;
	}
	if (MCU_offset("\n%%BeginPreview:", postscript, offset))
	{
		uint4 eoffset;
		if (MCU_offset("\n%%EndPreview", postscript, eoffset))
		{
			eoffset += 13;
			memmove(&postscript[offset], &postscript[eoffset], size - eoffset);
			size -= eoffset - offset;
		}
	}
	if (MCU_offset("\n%%BoundingBox:", postscript, offset))
	{
		if (strnequal(&postscript[offset + 16], "(atend)", 7))
		{
			uint4 newoffset;
			MCU_offset("\n%%BoundingBox:", &postscript[offset + 24], newoffset);
			offset += newoffset + 24;
		}
		real8 llx, lly, urx, ury;
		const char *sptr = &postscript[offset + 14];
		
		// MW-2005-04-26: Remove 'const' to get it compiling using GCC 4.0 on MacOS X
		char *newptr;
		llx = strtod(sptr, &newptr);
		if (newptr != sptr)
		{
			sptr = newptr;
			lly = strtod(sptr, &newptr);
			if (newptr != sptr)
			{
				sptr = newptr;
				urx = strtod(sptr, &newptr);
				if (newptr != sptr)
				{
					sptr = newptr;
					ury = strtod(sptr, &newptr);
					if (newptr != sptr)
					{
						tx = (int2)llx;
						ty = (int2)lly;
						ex = (uint2)(urx - llx);
						ey = (uint2)(ury - lly);
					}
				}
			}
		}
	}
	offset = 0;
	uint4 newoffset = 0;
	pagecount = 0;
	while (MCU_offset("\n%%Page:", &postscript[offset], newoffset))
	{
		MCU_realloc((char **)&pageIndex, pagecount, pagecount + 1, sizeof(uint4));
		offset += newoffset + 1;
		pageIndex[pagecount] = offset;
		pagecount++;
	}
}

void MCEPS::resetscale()
{
	MCRectangle trect = rect;
	if (flags & F_SHOW_BORDER)
		trect = MCU_reduce_rect(trect, borderwidth);
	if (flags & F_SCALE_INDEPENDENTLY)
	{
		xscale = trect.width * xf / ex;
		yscale = trect.height * yf / ey;
	}
	else
	{
		if (trect.width * ey < trect.height * ex)
			xscale = trect.width * xf / ex;
		else
			xscale = trect.height * xf / ey;
		yscale = xscale;
	}
}

Boolean MCEPS::import(MCStringRef fname, IO_handle stream)
{
	size = (uint4)MCS_fsize(stream);
	delete postscript;
	postscript = new (nothrow) char[size + 1];
	if (IO_read(postscript, size, stream) != IO_NORMAL)
		return False;
	postscript[size] = '\0';
	uindex_t t_sep;
    MCStringRef t_fname;
    if (MCStringLastIndexOfChar(fname, PATH_SEPARATOR, UINDEX_MAX, kMCCompareExact, t_sep))
        /* UNCHECKED */ MCStringCopySubstring(fname, MCRangeMakeMinMax(t_sep + 1, MCStringGetLength(fname)), t_fname);
    else
        t_fname = MCValueRetain(fname);
    
    MCNewAutoNameRef t_name;
    if (!MCNameCreateAndRelease(t_fname, &t_name))
        return False;
    setname(*t_name);
	setextents();
	rect.width = (uint2)(ex * xscale / xf);
	rect.height = (uint2)(ey * yscale / yf);
	if (flags & F_SHOW_BORDER)
	{
		rect.width += borderwidth << 1;
		rect.height += borderwidth << 1;
	}
	return True;
}
