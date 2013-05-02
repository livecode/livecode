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
#include "sysdefs.h"

#include "globals.h"
#include "object.h"
#include "stack.h"
#include "cdata.h"
#include "objptr.h"
#include "field.h"
#include "object.h"
#include "button.h"
#include "card.h"
#include "exec.h"
#include "aclip.h"
#include "player.h"

#include "exec-interface.h"

//////////

static MCExecEnumTypeElementInfo _kMCInterfacePlayDestinationElementInfo[] =
{	
	{ "external", 0, false },
	{ "internal", 1, false },
};

static MCExecEnumTypeInfo _kMCInterfacePlayDestinationTypeInfo =
{
	"Interface.PlayDestination",
	sizeof(_kMCInterfacePlayDestinationElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfacePlayDestinationElementInfo
};

////////////////////////////////////////////////////////////////////////////////

MCExecEnumTypeInfo *kMCInterfacePlayDestinationTypeInfo = &_kMCInterfacePlayDestinationTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCAudioClip::GetPlayProp(MCExecContext& ctxt, integer_t& r_loudness)
{
	if (this == MCtemplateaudio)
	{
		extern bool MCSystemGetPlayLoudness(uint2& r_loudness);
#ifdef _MOBILE
		if (MCSystemGetPlayLoudness(loudness))
#else
		if (false)
#endif
			;
		else if (!supported)
			loudness = 0;
		else
			getloudness(loudness);
	}
	r_loudness = loudness;
}

void MCAudioClip::SetPlayProp(MCExecContext& ctxt, uint2 p_loudness)
{
	if (this == MCtemplateaudio)
	{
		extern bool MCSystemSetPlayLoudness(uint2 p_loudness);
#ifdef _MOBILE
		if (MCSystemSetPlayLoudness(p_loudness))
			return ES_NORMAL;
#endif
		if (MCplayers != NULL)
		{
			MCPlayer *tptr = MCplayers;
			while (tptr != NULL)
			{
				tptr->setvolume(p_loudness);
				tptr = tptr->getnextplayer();
			}
		}
		setloudness(p_loudness);
	}
}

void MCAudioClip::GetPlayDestination(MCExecContext& ctxt, intenum_t& r_dest)
{
	integer_t t_loudness;
	GetPlayProp(ctxt, t_loudness);
	r_dest = (flags & F_EXTERNAL) ? 0 : 1;
}

void MCAudioClip::SetPlayDestination(MCExecContext& ctxt, intenum_t p_dest)
{
	if (p_dest == 0)
		flags |= F_EXTERNAL;
	else
		flags &= ~F_EXTERNAL;

	SetPlayProp(ctxt, loudness);
}

void MCAudioClip::GetPlayLoudness(MCExecContext& ctxt, integer_t& r_value)
{
	GetPlayProp(ctxt, r_value);
}
void MCAudioClip::SetPlayLoudness(MCExecContext& ctxt, integer_t p_value)
{
	loudness = MCU_max(MCU_min(p_value, 100), 0);
	if (loudness == 100)
		flags &= ~F_LOUDNESS;
	else
		flags |= F_LOUDNESS;

	SetPlayProp(ctxt, loudness);
}

void MCAudioClip::GetSize(MCExecContext& ctxt, uinteger_t& r_size)
{
	r_size = size;
}
