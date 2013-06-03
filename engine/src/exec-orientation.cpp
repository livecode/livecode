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

#include "globals.h"
#include "debug.h"
#include "handler.h"

#include "exec.h"
#include "exec-orientation.h"

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

static MCExecEnumTypeElementInfo _kMCOrientationOrientationElementInfo[] =
{
	{ "unknown", ORIENTATION_UNKNOWN },
	{ "portrait", ORIENTATION_PORTRAIT },
	{ "portrait upside down", ORIENTATION_PORTRAIT_UPSIDE_DOWN },
	{ "landscape right", ORIENTATION_LANDSCAPE_RIGHT },
	{ "landscape left", ORIENTATION_LANDSCAPE_LEFT },
	{ "face up", ORIENTATION_FACE_UP },
	{ "face down", ORIENTATION_FACE_DOWN },
};

static MCExecEnumTypeInfo _kMCOrientationOrientationTypeInfo =
{
	"Orientation.Orientation",
	sizeof(_kMCOrientationOrientationElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCOrientationOrientationElementInfo
};

////////////////////////////////////////////////////////////////////////////////

void MCOrientationGetDeviceOrientation(MCExecContext& ctxt, intenum_t& r_orientation)
{
	MCSystemGetDeviceOrientation(ctxt, r_orientation);
}

void MCOrientationGetOrientation(MCExecContext& ctxt, intenum_t& r_orientation)
{
	MCSystemGetOrientation(ctxt, r_orientation);
}

void MCOrientationGetAllowedOrientations(MCExecContext& ctxt, intset_t& r_orientation)
{
	MCSystemGetAllowedOrientations(ctxt, r_orientation);
}

void MCOrientationSetAllowedOrientations(MCExecContext& ctxt, intset_t p_orientations)
{
	MCSystemSetAllowedOrientations(ctxt, p_orientations);
}

void MCOrientationGetOrientationLocked(MCExecContext& ctxt, bool& r_locked)
{
	MCSystemGetOrientationLocked(ctxt, r_locked);
}

void MCOrientationExecLockOrientation(MCExecContext& ctxt)
{
	MCSystemLockOrientation(ctxt);
}

void MCOrientationExecUnlockOrientation(MCExecContext& ctxt)
{
	MCSystemLockOrientation(ctxt);
}

////////////////////////////////////////////////////////////////////////////////
