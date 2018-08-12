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
// MCVideoClip class declarations
//
#ifndef	VIDEOCLIP_H
#define	VIDEOCLIP_H

#include "mccontrol.h"

typedef MCObjectProxy<MCVideoClip>::Handle MCVideoClipHandle;

class MCVideoClip : public MCObject, public MCMixinObjectHandle<MCVideoClip>
{
public:
    
    enum { kObjectType = CT_VIDEO_CLIP };
    using MCMixinObjectHandle<MCVideoClip>::GetHandle;
    
private:
    
	real8 scale;
	uint2 framerate;
	uint1 *frames;
	uint4 size;

	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
public:
	MCVideoClip();
	MCVideoClip(const MCVideoClip &sref);
	// virtual functions from MCObject
	virtual ~MCVideoClip();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual Boolean del(bool p_check_flag);
	virtual void paste(void);

	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    
    virtual bool visit_self(MCObjectVisitor *p_visitor);
    
	MCVideoClip *clone();
	bool getfile(MCStringRef& r_file);
	real8 getscale()
	{
		return scale;
	}
	Boolean import(MCStringRef fname, IO_handle stream);
	
	IO_stat load(IO_handle stream, uint32_t version);
	IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);

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
	
	////////// PROPERTY ACCESSORS

	void GetDontRefresh(MCExecContext& ctxt, bool& r_setting);
	void SetDontRefresh(MCExecContext& ctxt, bool setting);
	void GetFrameRate(MCExecContext& ctxt, integer_t*& r_rate);
	void SetFrameRate(MCExecContext& ctxt, integer_t* p_rate);
	void GetScale(MCExecContext& ctxt, double& r_scale);
	void SetScale(MCExecContext& ctxt, double p_scale);
	void GetSize(MCExecContext& ctxt, integer_t& r_size);
	void GetText(MCExecContext& ctxt, MCStringRef& r_text);
	void SetText(MCExecContext& ctxt, MCStringRef p_text);

};
#endif
