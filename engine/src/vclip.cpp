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

//#include "execpt.h"
#include "util.h"
#include "date.h"
#include "sellst.h"
#include "stack.h"
#include "vclip.h"
#include "player.h"
#include "mcerror.h"
#include "osspec.h"

#include "globals.h"
#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCVideoClip::kProperties[] =
{
	DEFINE_RW_OBJ_PROPERTY(P_DONT_REFRESH, Bool, MCVideoClip, DontRefresh)
	DEFINE_RW_OBJ_PROPERTY(P_FRAME_RATE, OptionalInt16, MCVideoClip, FrameRate)
	DEFINE_RW_OBJ_PROPERTY(P_SCALE, Double, MCVideoClip, Scale)
	DEFINE_RO_OBJ_PROPERTY(P_SIZE, Int16, MCVideoClip, Size)
	DEFINE_RW_OBJ_PROPERTY(P_TEXT, String, MCVideoClip, Text)
};

MCObjectPropertyTable MCVideoClip::kPropertyTable =
{
	&MCObject::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCVideoClip::MCVideoClip()
{
	scale = 1.0;
	framerate = 0;
	size = 0;
	frames = NULL;
}

MCVideoClip::MCVideoClip(const MCVideoClip &mref) : MCObject(mref)
{
	framerate = mref.framerate;
	scale = mref.scale;
	size = mref.size;
	if (mref.frames != NULL)
	{
		frames = new uint1[size];
		memcpy((char *)frames, (char *)mref.frames, size);
	}
	else
		frames = NULL;
}

MCVideoClip::~MCVideoClip()
{
	delete frames;
}

Chunk_term MCVideoClip::gettype() const
{
	return CT_VIDEO_CLIP;
}

const char *MCVideoClip::gettypestring()
{
	return MCvideostring;
}

#ifdef LEGACY_EXEC
Exec_stat MCVideoClip::getprop_legacy(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
	switch (which)
	{
#ifdef /* MCVideoClip::getprop */ LEGACY_EXEC
	case P_DONT_REFRESH:
		ep.setboolean(getflag(F_DONT_REFRESH));
		break;
	case P_FRAME_RATE:
		if (flags & F_FRAME_RATE)
			ep.setint(framerate);
		else
			ep.clear();
		break;
	case P_SCALE:
		ep.setnvalue(scale);
		break;
	case P_SIZE:
		ep.setint(size);
		break;
	case P_TEXT:
		{
			MCString s((const char *)frames, size);
			ep.setsvalue(s);
		}
		break;
#endif /* MCVideoClip::getprop */
	default:
		return MCObject::getprop_legacy(parid, which, ep, effective);
	}
	return ES_NORMAL;
}
#endif

#ifdef LEGACY_EXEC
Exec_stat MCVideoClip::setprop_legacy(uint4 parid, Properties p, MCExecPoint &ep, Boolean effective)
{
	MCString data = ep.getsvalue();

	Boolean dirty = False;
	switch (p)
	{
#ifdef /* MCVideoClip::setprop */ LEGACY_EXEC
	case P_DONT_REFRESH:
		if (!MCU_matchflags(data, flags, F_DONT_REFRESH, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		return ES_NORMAL;
	case P_FRAME_RATE:
		if (data.getlength() == 0)
			flags &= ~F_FRAME_RATE;
		else
		{
			if (!MCU_stoui2(data, framerate))
			{
				MCeerror->add
				(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			flags |= F_FRAME_RATE;
		}
		return ES_NORMAL;
	case P_SCALE:
		if (!MCU_stor8(data, scale))
		{
			MCeerror->add
			(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		flags |= F_SCALE_FACTOR;
		return ES_NORMAL;
	case P_TEXT:
		delete frames;
		size = data.getlength();
		frames = new uint1[size];
		memcpy(frames, data.getstring(), size);
		return ES_NORMAL;
#endif /* MCVideoClip::setprop */
	default:
		break;
	}
	return MCObject::setprop_legacy(parid, p, ep, effective);
}
#endif

Boolean MCVideoClip::del()
{
	getstack()->removevclip(this);
	return True;
}

void MCVideoClip::paste(void)
{
	MCdefaultstackptr -> appendvclip(this);
}

MCVideoClip *MCVideoClip::clone()
{
	return new MCVideoClip(*this);
}

bool MCVideoClip::getfile(MCStringRef& r_file)
{
	if (frames != NULL)
	{
        MCAutoStringRef t_tmpfile;
        MCS_tmpnam(&t_tmpfile);
		//char *tmpfile = strclone(MCS_tmpnam());
		
		IO_handle tstream;
		if ((tstream = MCS_open(*t_tmpfile, kMCOpenFileModeWrite, False, False, 0)) == NULL)
			return false;
		IO_stat stat = IO_write(frames, sizeof(int1), size, tstream);
		MCS_close(tstream);
		if (stat != IO_NORMAL)
		{
			MCS_unlink(*t_tmpfile);
			return false;
		}
		r_file = MCValueRetain(*t_tmpfile);
		return true;
	}
	return false;
}

Boolean MCVideoClip::import(MCStringRef fname, IO_handle fstream)
{
    uindex_t t_last_slash;
    MCStringRef t_path;
    if (MCStringLastIndexOfChar(fname, PATH_SEPARATOR, UINDEX_MAX, kMCCompareExact, t_last_slash))
        /* UNCHECKED */ MCStringCopySubstring(fname, MCRangeMake(t_last_slash + 1, MCStringGetLength(fname) - t_last_slash - 1), t_path);
    else
        t_path = MCValueRetain(fname);
    
	MCNewAutoNameRef t_path_name;
    /* UNCHECKED */ MCNameCreateAndRelease(t_path, &t_path_name);
	setname(*t_path_name);
	size = (uint4)MCS_fsize(fstream);
	frames = new uint1[size];
	if (MCS_readfixed(frames, size, fstream) != IO_NORMAL)
		return False;
	return True;
}

IO_stat MCVideoClip::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part)
{
	return defaultextendedsave(p_stream, p_part);
}

IO_stat MCVideoClip::extendedload(MCObjectInputStream& p_stream, uint32_t p_version, uint4 p_length)
{
	return defaultextendedload(p_stream, p_version, p_length);
}

IO_stat MCVideoClip::save(IO_handle stream, uint4 p_part, bool p_force_ext)
{
	IO_stat stat;

	if ((stat = IO_write_uint1(OT_VIDEO_CLIP, stream)) != IO_NORMAL)
		return stat;
	if ((stat = MCObject::save(stream, p_part, p_force_ext)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint4(size, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write(frames, sizeof(uint1), size, stream)) != IO_NORMAL)
		return stat;
	if (flags & F_FRAME_RATE)
		if ((stat = IO_write_uint2(framerate, stream)) != IO_NORMAL)
			return stat;
	if (flags & F_SCALE_FACTOR)
		if ((stat = IO_write_int4(MCU_r8toi4(scale), stream)) != IO_NORMAL)
			return stat;
	return savepropsets(stream);
}

IO_stat MCVideoClip::load(IO_handle stream, uint32_t version)
{
	IO_stat stat;

	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_uint4(&size, stream)) != IO_NORMAL)
		return stat;
	if (size != 0)
	{
		frames = new uint1[size];
		if ((stat = IO_read(frames, size, stream)) != IO_NORMAL)
			return stat;
	}
	if (flags & F_FRAME_RATE)
		if ((stat = IO_read_uint2(&framerate, stream)) != IO_NORMAL)
			return stat;
	if (flags & F_SCALE_FACTOR)
	{
		int4 i;
		if ((stat = IO_read_int4(&i, stream)) != IO_NORMAL)
			return stat;
		scale = MCU_i4tor8(i);
	}
	return loadpropsets(stream, version);
}