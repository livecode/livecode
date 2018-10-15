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

#ifndef __MC_NOTIFY__
#define __MC_NOTIFY__

bool MCNotifyInitialize(void);
void MCNotifyFinalize(void);

// MW-2010-09-04: Added 'safe' parameter. If true, the notification will only
//   be performed at the next script-safe point.
// MW-2014-10-23: [[ Bug 13721 ]] 'required' parameter says to always invoke the callback
//   whether during event processing or shutdown. In the processing state, a second argument of
//   0 will be passed to the callback. In shutdown state, a second argument of 1 will be passed.
bool MCNotifyPush(void (*callback)(void *), void *state, bool block, bool safe, bool required = false);

// MW-2010-09-04: If 'safe' is true then all notifications will be dispatched
//   otherwise only ones which are for non-script safe points will be.
bool MCNotifyDispatch(bool safe);

// MW-2013-06-14: [[ DesktopPingWait ]] Wake up any currently running 'wait'.
void MCNotifyPing(bool p_high_priority);

// Returns true if there is a pending notification
bool MCNotifyPending(void);

#endif
