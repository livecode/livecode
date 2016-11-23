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

#ifdef FEATURE_PLATFORM_AUDIO
#include "platform.h"
#elif defined(_WINDOWS_DESKTOP)
// SN-2014-06-26 [[ PlatformPlayer ]]
// These 2 definitions must be accessible from exec-interface-aclip
extern HWAVEOUT hwaveout;  //handle to audio device opened
extern WAVEHDR wh;         //wave header structure
#endif

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

void MCAudioClip::GetPlayDestination(MCExecContext& ctxt, intenum_t& r_dest)
{
	r_dest = (flags & F_EXTERNAL) ? 0 : 1;
}

void MCAudioClip::SetPlayDestination(MCExecContext& ctxt, intenum_t p_dest)
{
	if (p_dest == 0)
		flags |= F_EXTERNAL;
	else
		flags &= ~F_EXTERNAL;
}

void MCAudioClip::GetPlayLoudness(MCExecContext& ctxt, integer_t& r_value)
{
	r_value = loudness;
}

void MCAudioClip::SetPlayLoudness(MCExecContext& ctxt, integer_t p_value)
{
	loudness = MCU_max(MCU_min(p_value, 100), 0);
	if (loudness == 100)
		flags &= ~F_LOUDNESS;
	else
		flags |= F_LOUDNESS;
}

void MCAudioClip::GetSize(MCExecContext& ctxt, uinteger_t& r_size)
{
	r_size = size;
}
