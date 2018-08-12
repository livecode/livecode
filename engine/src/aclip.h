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

#ifndef	AUDIOCLIP_H
#define	AUDIOCLIP_H

#include "object.h"

#ifdef TARGET_PLATFORM_LINUX
#include "lnxaudio.h"
#endif

enum Audio_format {
    AF_SLINEAR,
    AF_MULAW,
    AF_ULINEAR
};

#ifdef _MACOSX
#define PLAY_RATE 1000
#define LOOP_RATE 16
#else
#define PLAY_RATE 250
#define LOOP_RATE 250
#endif

typedef MCObjectProxy<MCAudioClip>::Handle MCAudioClipHandle;

class MCAudioClip : public MCObject, public MCMixinObjectHandle<MCAudioClip>
{
public:
    
    enum { kObjectType = CT_AUDIO_CLIP };
    using MCMixinObjectHandle<MCAudioClip>::GetHandle;
    
private:
    
	friend class MCHcsnd;
	uint4 size;
	int1 *samples;
	uint2 format;
	uint2 nchannels;
	uint2 swidth;
	uint2 rate;
	uint4 osize;
	int1 *osamples;
	uint2 oformat;
	uint2 onchannels;
	uint2 oswidth;
	uint2 orate;
	Boolean disposable;
	uint2 loudness;
	static MCStack *mstack;
	static Boolean supported;
	static uint4 curindex;
	static real8 endtime;
	static Boolean looping;
	static IO_handle dstream;
	
#ifdef TARGET_PLATFORM_LINUX
	X11Audio *x11audio ;
#endif 
	
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
public:
	MCAudioClip();
	MCAudioClip(const MCAudioClip &cref);
	// virtual functions from MCObject
	virtual ~MCAudioClip();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    
    virtual bool visit_self(MCObjectVisitor *p_visitor);
    
	virtual void timer(MCNameRef mptr, MCParameter *params);

	virtual Boolean del(bool p_check_flag);
	virtual void paste(void);

	// MCAudioClip functions
	uint2 getloudness()
	{
		return loudness;
	}
	void init();
	Exec_stat set(char *d);
	void convert_mulawtolin16();
	void convert_mulawtoulin8();
	void convert_slin8toslin16();
	void convert_slintoulin();
	void convert_ulintoslin();
    void convert_tocontainer(void*& r_data, size_t& r_data_size);
	Boolean isdisposable();
	Boolean issupported();
	void setdisposable();
	void setlooping(Boolean loop);
	Boolean import(MCStringRef fname, IO_handle stream);
	Boolean open_audio();
	Boolean play();
    void stop(Boolean abort);
    bool isPlaying();

    IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
    IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);
	IO_stat load(IO_handle stream, uint32_t version);
	IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);

	MCStack *getmessagestack()
	{
		return mstack;
	}
	void setmessagestack(MCStack *newstack)
	{
		mstack = newstack;
	}
	MCAudioClip *next()
	{
		return (MCAudioClip *)MCDLlist::next();
	}
	MCAudioClip *prev()
	{
		return (MCAudioClip *)MCDLlist::prev();
	}
	void totop(MCAudioClip *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCAudioClip *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCAudioClip *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCAudioClip *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCAudioClip *node)
	{
		MCDLlist::splitat((MCDLlist *)node);
	}
	MCAudioClip *remove(MCAudioClip *&list)
	{
		return (MCAudioClip *)MCDLlist::remove((MCDLlist *&)list);
	}

	////////// PROPERTY ACCESSORS

	void GetPlayDestination(MCExecContext& ctxt, intenum_t& r_dest);
	void SetPlayDestination(MCExecContext& ctxt, intenum_t p_dest);
	void GetPlayLoudness(MCExecContext& ctxt, integer_t& r_value);
	void SetPlayLoudness(MCExecContext& ctxt, integer_t p_value);
	void GetSize(MCExecContext& ctxt, uinteger_t& r_size);

};
#endif
