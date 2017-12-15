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
// MCMagnify class declarations
//
#ifndef	MAGNIFY_H
#define	MAGNIFY_H

#include "mccontrol.h"

class MCMagnify;
typedef MCObjectProxy<MCMagnify>::Handle MCMagnifyHandle;

class MCMagnify : public MCControl, public MCMixinObjectHandle<MCMagnify>
{
public:
    
    enum { kObjectType = CT_MAGNIFY };
    using MCMixinObjectHandle<MCMagnify>::GetHandle;
    
protected:
	uint2 inside;
public:
	MCMagnify();
	MCMagnify(const MCMagnify &mref);
	// virtual functions from MCObject
	virtual ~MCMagnify();
	virtual Chunk_term gettype() const;
    
    virtual bool visit_self(MCObjectVisitor *p_visitor);
    
	virtual void open();
	virtual void close();
	virtual Boolean kfocusnext(Boolean top);
	virtual Boolean kfocusprev(Boolean bottom);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void munfocus();
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	// virtual functions from control
	IO_stat load(IO_handle stream, uint32_t version);
	IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);

	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);

	// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);
};
#endif
