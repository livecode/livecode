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

#ifndef __MC_REDRAW__
#define __MC_REDRAW__

bool MCRedrawIsScreenLocked(void);
void MCRedrawSaveLockScreen(uint2& r_lock);
void MCRedrawRestoreLockScreen(uint2 lock);

void MCRedrawLockScreen(void);
void MCRedrawUnlockScreen(void);
void MCRedrawUnlockScreenWithEffects(void);
void MCRedrawForceUnlockScreen(void);

bool MCRedrawIsScreenDirty(void);
void MCRedrawDirtyScreen(void);

void MCRedrawScheduleUpdateForStack(MCStack *stack);
void MCRedrawDoUpdateScreen(void);

bool MCRedrawIsScreenUpdateEnabled(void);
void MCRedrawDisableScreenUpdates(void);
void MCRedrawEnableScreenUpdates(void);

// MW-2011-11-01: This global variable is used to check and see if an update is
//   required in performance critical places.
extern bool MCredrawupdatescreenneeded;

inline void MCRedrawUpdateScreen(void)
{
	if (MCredrawupdatescreenneeded)
		MCRedrawDoUpdateScreen();
}

#endif
