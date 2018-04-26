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

#include "mcerror.h"
#include "globals.h"
#include "exec.h"
#include "osspec.h"

#include "stack.h"

////////////////////////////////////////////////////////////////////////////////

void MCIdeExecEditScriptOfObject(MCExecContext &ctxt, MCObject *p_object, MCStringRef p_at)
{
	// MW-2010-10-13: [[ Bug 7476 ]] Make sure we temporarily turn off lock messages
	//   before invoking the method - since it requires message sending to work!
	Boolean t_old_lock;
	t_old_lock = MClockmessages;
	MClockmessages = False;
	p_object->editscript(p_at);
	MClockmessages = t_old_lock;
}

////////////////////////////////////////////////////////////////////////////////

void MCIdeExecHideMessageBox(MCExecContext& ctxt)
{
	MCStack *mb = ctxt . GetObject()->getstack()->findstackname(MCN_messagename);
	if (mb != NULL)
		mb->close();
}

void MCIdeExecShowMessageBox(MCExecContext& ctxt)
{
	MCStack *mb = ctxt . GetObject()->getstack()->findstackname(MCN_messagename);

	// MW-2007-08-14: [[ Bug 3310 ]] - "show message box" toplevels rather than palettes
	if (mb != NULL)
		mb->openrect(ctxt . GetObject()->getstack()->getrect(), WM_PALETTE, NULL, WP_DEFAULT, OP_NONE);
}

////////////////////////////////////////////////////////////////////////////////
