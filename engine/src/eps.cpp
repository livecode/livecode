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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "execpt.h"
#include "util.h"
#include "mcerror.h"
#include "sellst.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "eps.h"

#include "globals.h"
#include "context.h"

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
		pageIndex = new uint4[pagecount];
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
		image = new MCImage(*sref.image);
		image->setparent(this);
	}
}

MCEPS::~MCEPS()
{
	delete postscript;
	delete prolog;
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
			message_with_args(MCM_mouse_down, "1");
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

Boolean MCEPS::mup(uint2 which)
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
			if (MCU_point_in_rect(rect, mx, my))
				message_with_args(MCM_mouse_up, "1");
			break;
		case T_POINTER:
			end();
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
		if (MCU_point_in_rect(rect, mx, my))
			message_with_args(MCM_mouse_up, which);
		break;
	}
	return True;
}

void MCEPS::setrect(const MCRectangle &nrect)
{
	if (rect.width != nrect.width || rect.height != nrect.height)
	{
		rect = nrect;
		resetscale();
	}
	else
		rect = nrect;
}

Exec_stat MCEPS::getprop(uint4 parid, Properties which, MCExecPoint& ep, Boolean effective)
{
	switch (which)
	{
#ifdef /* MCEPS::getprop */ LEGACY_EXEC
	case P_SIZE:
		ep.setint(size);
		break;
	case P_ANGLE:
		ep.setr8(angle, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
		break;
	case P_POSTSCRIPT:
		ep.setsvalue(postscript);
		break;
	case P_PROLOG:
		ep.setsvalue(prolog);
		break;
	case P_RETAIN_IMAGE:
		ep.setboolean(getflag(F_RETAIN_IMAGE));
		break;
	case P_RETAIN_POSTSCRIPT:
		ep.setboolean(getflag(F_RETAIN_POSTSCRIPT));
		break;
	case P_SCALE_INDEPENDENTLY:
		ep.setboolean(getflag(F_SCALE_INDEPENDENTLY));
		break;
	case P_BOUNDING_RECT:
		ep.setrectangle(tx, ty, tx + ex, ty + ey);
		break;
	case P_SCALE:
	case P_X_SCALE:
		ep.setr8(xscale, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
		break;
	case P_Y_SCALE:
		ep.setr8(yscale, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
		break;
	case P_X_OFFSET:
		ep.setint(tx);
		break;
	case P_Y_OFFSET:
		ep.setint(ty);
		break;
	case P_X_EXTENT:
		ep.setint(ex);
		break;
	case P_Y_EXTENT:
		ep.setint(ey);
		break;
	case P_CURRENT_PAGE:
		ep.setint(MCU_max(curpage, 1));
		break;
	case P_PAGE_COUNT:
		ep.setint(MCU_max(pagecount, 1));
		break;
#endif /* MCEPS::getprop */ 
	default:
		return MCControl::getprop(parid, which, ep, effective);
	}
	return ES_NORMAL;
}

Exec_stat MCEPS::setprop(uint4 parid, Properties p, MCExecPoint &ep, Boolean effective)
{
	Boolean dirty = True;
	real8 n;
	int2 i;
	int2 i1, i2, i3, i4;
	MCString data = ep.getsvalue();

	switch (p)
	{
#ifdef /* MCEPS::setprop */ LEGACY_EXEC
	case P_TRAVERSAL_ON:
	case P_SHOW_BORDER:
		if (MCControl::setprop(parid, p, ep, effective) != ES_NORMAL)
			return ES_ERROR;
		resetscale();
		break;
	case P_ANGLE:
		if (!MCU_stoi2(data, i))
		{
			MCeerror->add
			(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		angle = i;
		break;
	case P_POSTSCRIPT:
		delete postscript;
		postscript = data.clone();
		size = data.getlength() + 1;
		setextents();
		resetscale();
		break;
	case P_PROLOG:
		delete prolog;
		prolog = data.clone();
		break;
	case P_RETAIN_IMAGE:
		if (!MCU_matchflags(data, flags, F_RETAIN_IMAGE, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		dirty = False;
		break;
	case P_RETAIN_POSTSCRIPT:
		if (!MCU_matchflags(data, flags, F_RETAIN_POSTSCRIPT, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		dirty = False;
		break;
	case P_SCALE_INDEPENDENTLY:
		if (!MCU_matchflags(data, flags, F_SCALE_INDEPENDENTLY, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty)
			resetscale();
		break;
	case P_BOUNDING_RECT:
		if (!MCU_stoi2x4(data, i1, i2, i3, i4))
		{
			MCeerror->add
			(EE_OBJECT_NAR, 0, 0, data);
			return ES_ERROR;
		}
		if (tx != i1 || ty != i2 || tx + ex != i3 || ty + ey != i4)
		{
			tx = i1;
			ty = i2;
			ex = MCU_max(i3 - i1, 1);
			ey = MCU_max(i4 - i2, 1);
			resetscale();
		}
		else
			dirty = False;
		break;
	case P_SCALE:
		if (!MCU_stor8(data, n))
		{
			MCeerror->add
			(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		xscale = yscale = n;
		break;
	case P_X_SCALE:
		if (!MCU_stor8(data, n))
		{
			MCeerror->add
			(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		xscale = n;
		ex = (uint2)(rect.width * xf / xscale + 0.5);
		flags |= F_SCALE_INDEPENDENTLY;
		break;
	case P_Y_SCALE:
		if (!MCU_stor8(data, n))
		{
			MCeerror->add
			(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		yscale = n;
		ey = (uint2)(rect.height * yf / yscale + 0.5);
		flags |= F_SCALE_INDEPENDENTLY;
		break;
	case P_X_OFFSET:
		if (!MCU_stoi2(data, i))
		{
			MCeerror->add
			(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		tx = i;
		break;
	case P_Y_OFFSET:
		if (!MCU_stoi2(data, i))
		{
			MCeerror->add
			(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		ty = i;
		break;
	case P_X_EXTENT:
		if (!MCU_stoi2(data, i))
		{
			MCeerror->add
			(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		ex = i;
		resetscale();
		break;
	case P_Y_EXTENT:
		if (!MCU_stoi2(data, i))
		{
			MCeerror->add
			(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		ey = i;
		resetscale();
		break;
	case P_CURRENT_PAGE:    //set eps current page to display
		if (!MCU_stoi2(data, i))
		{
			MCeerror->add
			(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		if ((uint2)i > pagecount)
			curpage = pagecount;
		else
			curpage = i;
		break;
#endif /* MCEPS::setprop */
	default:
		return MCControl::setprop(parid, p, ep, effective);
	}
	if (dirty && opened)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	return ES_NORMAL;
}

IO_stat MCEPS::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part)
{
	return defaultextendedsave(p_stream, p_part);
}

IO_stat MCEPS::extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length)
{
	return defaultextendedload(p_stream, p_version, p_length);
}

IO_stat MCEPS::save(IO_handle stream, uint4 p_part, bool p_force_ext)
{
	IO_stat stat;

	if ((stat = IO_write_uint1(OT_MCEPS, stream)) != IO_NORMAL)
		return stat;
	if ((stat = MCObject::save(stream, p_part, p_force_ext)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint4(size, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write(postscript, sizeof(char), size, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_string(prolog, stream)) != IO_NORMAL)
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
		if ((stat = image->save(stream, p_part, p_force_ext)) != IO_NORMAL)
			return stat;
	if ((stat = IO_write_uint2(curpage, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(pagecount, stream)) != IO_NORMAL)
		return stat;
	uint2 i;
	for (i = 0 ; i < pagecount ; i++)
		if ((stat = IO_write_uint4(pageIndex[i], stream)) != IO_NORMAL)
			return stat;
	return savepropsets(stream);
}

MCControl *MCEPS::clone(Boolean attach, Object_pos p, bool invisible)
{
	MCEPS *neweps = new MCEPS(*this);
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
		MCSegment segs[2];
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
		if (flags & F_3D)
			draw3d(dc, trect, ETCH_SUNKEN, borderwidth);
		else
			drawborder(dc, trect, borderwidth);
	if (getstate(CS_KFOCUSED))
		drawfocus(dc, dirty);
	if (state & CS_SELECTED)
		drawselected(dc);
}

IO_stat MCEPS::load(IO_handle stream, const char *version)
{
	IO_stat stat;

	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return stat;
	delete postscript;
	delete prolog;
	if ((stat = IO_read_uint4(&size, stream)) != IO_NORMAL)
		return stat;
	postscript = new char[size + 1];
	if ((stat = IO_read(postscript, sizeof(char), size, stream)) != IO_NORMAL)
		return stat;
	postscript[size] = '\0';
	if ((stat = IO_read_string(prolog, stream)) != IO_NORMAL)
		return stat;
	int4 i;
	if ((stat = IO_read_int4(&i, stream)) != IO_NORMAL)
		return stat;
	xscale = MCU_i4tor8(i);
	if (flags & F_SCALE_INDEPENDENTLY)
	{
		if ((stat = IO_read_int4(&i, stream)) != IO_NORMAL)
			return stat;
		yscale = MCU_i4tor8(i);
	}
	else
		yscale = xscale;
	if ((stat = IO_read_int2(&angle, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_int2(&tx, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_int2(&ty, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_uint2(&ex, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_uint2(&ey, stream)) != IO_NORMAL)
		return stat;
	if (flags & F_RETAIN_IMAGE)
	{
		image = new MCImage;
		image->setparent(this);
		if ((stat = image->load(stream, version)) != IO_NORMAL)
			return stat;
	}
	if (strncmp(version, "1.3", 3) > 0)
	{
		if ((stat = IO_read_uint2(&curpage, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_read_uint2(&pagecount, stream)) != IO_NORMAL)
			return stat;
		if (pagecount > 0)
		{
			pageIndex = new uint4[pagecount];
			for (i = 0 ; i < pagecount ; i++)
				if ((stat = IO_read_uint4(&pageIndex[i], stream)) != IO_NORMAL)
					return stat;
		}
	}
	return loadpropsets(stream);
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
		uint4 offset = swap_uint4(&uint4ptr[1]);
		size = swap_uint4(&uint4ptr[2]);
		MCswapbytes = !MCswapbytes;
		char *newps = new char[size];
		memcpy(newps, &postscript[offset], size);
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

Boolean MCEPS::import(char *fname, IO_handle stream)
{
	size = (uint4)MCS_fsize(stream);
	delete postscript;
	postscript = new char[size + 1];
	if (IO_read(postscript, sizeof(char), size, stream) != IO_NORMAL)
		return False;
	postscript[size] = '\0';
	const char *tname = strrchr(fname, PATH_SEPARATOR);
	if (tname != NULL)
		tname += 1;
	else
		tname = fname;
	setname_cstring(tname);
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
