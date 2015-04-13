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
// MCVideoClip class declarations
//
#ifndef	VIDEOCLIP_H
#define	VIDEOCLIP_H

#include "control.h"

class MCVideoClip : public MCObject
{
	real8 scale;
	uint2 framerate;
	uint1 *frames;
	uint4 size;
public:
	MCVideoClip();
	MCVideoClip(const MCVideoClip &sref);
	// virtual functions from MCObject
	virtual ~MCVideoClip();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();
	virtual Exec_stat getprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective, bool recursive = false);
	virtual Exec_stat setprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Boolean del();
	virtual void paste(void);
	MCVideoClip *clone();
	char *getfile();
	real8 getscale()
	{
		return scale;
	}
	Boolean import(const char *fname, IO_handle stream);
	
	IO_stat load(IO_handle stream, const char *version);
	IO_stat extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);

	MCVideoClip *next()
	{
		return (MCVideoClip *)MCDLlist::next();
	}
	MCVideoClip *prev()
	{
		return (MCVideoClip *)MCDLlist::prev();
	}
	void totop(MCVideoClip *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCVideoClip *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCVideoClip *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCVideoClip *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCVideoClip *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCVideoClip *remove
	(MCVideoClip *&list)
	{
		return (MCVideoClip *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};
#endif
