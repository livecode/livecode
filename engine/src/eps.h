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

//
// Eps class declarations
//
#ifndef	MCEPS_H
#define	MCEPS_H

#include "mccontrol.h"

class MCEPS;
typedef MCObjectProxy<MCEPS>::Handle MCEPSHandle;

class MCEPS : public MCControl, public MCMixinObjectHandle<MCEPS>
{
public:
    
    enum { kObjectType = CT_EPS };
    using MCMixinObjectHandle<MCEPS>::GetHandle;
    
private:
    
	uint4 size;
	char *postscript;
	char *prolog;
	real8 xscale;
	real8 yscale;
	int2 angle;
	int2 tx;
	int2 ty;
	uint2 ex;
	uint2 ey;
	uint2 curpage;
	uint2 pagecount;
	uint4 *pageIndex;
	MCImage *image;
	static real8 xf;
	static real8 yf;
	static char defs[];
public:
	MCEPS();
	MCEPS(const MCEPS &sref);
	// virtual functions from MCObject
	virtual ~MCEPS();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();
    
    virtual bool visit_self(MCObjectVisitor *p_visitor);
    
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual void applyrect(const MCRectangle &nrect);

	// virtual functions from MCControl
	IO_stat load(IO_handle stream, uint32_t version);
	IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);

	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);

	// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);

	// Eps functions
	void setextents();
	void resetscale();
	Boolean import(MCStringRef fname, IO_handle stream);
};
#endif
