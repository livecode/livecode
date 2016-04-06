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

#include "globals.h"
#include "debug.h"
#include "handler.h"

#include "mblsyntax.h"
#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

static MCExecSetTypeElementInfo _kMCOrientationOrientationsElementInfo[] =
{
	{ "unknown", ORIENTATION_UNKNOWN_BIT },
	{ "portrait", ORIENTATION_PORTRAIT_BIT },
	{ "portrait upside down", ORIENTATION_PORTRAIT_UPSIDE_DOWN_BIT },
	{ "landscape right", ORIENTATION_LANDSCAPE_RIGHT_BIT },
	{ "landscape left", ORIENTATION_LANDSCAPE_LEFT_BIT },
	{ "face up", ORIENTATION_FACE_UP_BIT },
	{ "face down", ORIENTATION_FACE_DOWN_BIT },
};

static MCExecSetTypeInfo _kMCOrientationOrientationsTypeInfo =
{
	"Orientation.Orientations",
	sizeof(_kMCOrientationOrientationsElementInfo) / sizeof(MCExecSetTypeElementInfo),
	_kMCOrientationOrientationsElementInfo
};

//////////
// AL-2014-09-22: [[ Bug 13426 ]] Don't use bit-shifted values for orientation state enum
static MCExecEnumTypeElementInfo _kMCOrientationOrientationElementInfo[] =
{
	{ "unknown", ORIENTATION_UNKNOWN_BIT },
	{ "portrait", ORIENTATION_PORTRAIT_BIT },
	{ "portrait upside down", ORIENTATION_PORTRAIT_UPSIDE_DOWN_BIT },
	{ "landscape right", ORIENTATION_LANDSCAPE_RIGHT_BIT },
	{ "landscape left", ORIENTATION_LANDSCAPE_LEFT_BIT },
	{ "face up", ORIENTATION_FACE_UP_BIT },
	{ "face down", ORIENTATION_FACE_DOWN_BIT },
};

static MCExecEnumTypeInfo _kMCOrientationOrientationTypeInfo =
{
	"Orientation.Orientation",
	sizeof(_kMCOrientationOrientationElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCOrientationOrientationElementInfo
};

////////////////////////////////////////////////////////////////////////////////

MCExecSetTypeInfo *kMCOrientationOrientationsTypeInfo = &_kMCOrientationOrientationsTypeInfo;
MCExecEnumTypeInfo *kMCOrientationOrientationTypeInfo = &_kMCOrientationOrientationTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCOrientationGetDeviceOrientation(MCExecContext& ctxt, intenum_t& r_orientation)
{
	MCOrientation t_orientation;
	MCSystemGetDeviceOrientation(t_orientation);
	r_orientation = (intenum_t)t_orientation;
}

void MCOrientationGetOrientation(MCExecContext& ctxt, intenum_t& r_orientation)
{
	MCOrientation t_orientation;
	MCSystemGetOrientation(t_orientation);
	r_orientation = (intenum_t)t_orientation;
}

void MCOrientationGetAllowedOrientations(MCExecContext& ctxt, intset_t& r_orientations)
{
	uint32_t t_orientations;
	MCSystemGetAllowedOrientations(t_orientations);
	r_orientations = (intset_t)t_orientations;
}

void MCOrientationSetAllowedOrientations(MCExecContext& ctxt, intset_t p_orientations)
{	
	uint32_t t_orientations;
	t_orientations = (uint32_t)p_orientations;
	MCSystemSetAllowedOrientations(t_orientations);
}

void MCOrientationGetOrientationLocked(MCExecContext& ctxt, bool& r_locked)
{
	MCSystemGetOrientationLocked(r_locked);
}

void MCOrientationExecLockOrientation(MCExecContext& ctxt)
{
	MCSystemLockOrientation();
}

void MCOrientationExecUnlockOrientation(MCExecContext& ctxt)
{
	MCSystemUnlockOrientation();
}

////////////////////////////////////////////////////////////////////////////////
