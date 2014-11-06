/*                                                                   -*- c++ -*-
Copyright (C) 2003-2014 Runtime Revolution Ltd.

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

#include "util.h"
#include "system.h"

#include "stackdir.h"

#include "stackdir_private.h"

/* This file contains code relating to loading Expanded LiveCode
 * Stackfiles.
 */

/* ================================================================
 * High-level entry points
 * ================================================================ */

/* ----------------------------------------------------------------
 * [Public] High-level operations
 * ---------------------------------------------------------------- */

bool
MCStackdirIONewLoad (MCStackdirIORef & op)
{
	if (!MCStackdirIONew (op)) return false;
	op->m_type = kMCStackdirIOTypeLoad;
	return true;
}

void
MCStackdirIOSetConflictPermitted (MCStackdirIORef op,
								  bool enabled)
{
	MCStackdirIOAssertLoad (op);
	op->m_load_allow_conflicts = enabled;
}

/* ----------------------------------------------------------------
 * [Private] High-level operations
 * ---------------------------------------------------------------- */

void
MCStackdirIOCommitLoad (MCStackdirIORef op)
{
	/* FIXME */
}
