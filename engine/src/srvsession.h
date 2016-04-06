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

#ifndef SRVSESSION_H
#define SRVSESSION_H

// session
typedef struct
{
	char*		id;
	char*		ip;
	char*		filename;
	real64_t	expires;
	
	MCSystemFileHandle * filehandle;
	
	// session data
	uint32_t	data_length;
	char *		data;
} MCSession, *MCSessionRef;

bool MCSessionStart(MCStringRef p_session_id, MCSessionRef &r_session);

bool MCSessionCommit(MCSessionRef p_session);
void MCSessionDiscard(MCSessionRef p_session);
bool MCSessionExpire(MCStringRef p_id);

bool MCSessionCleanup(void);

#endif//SRVSESSION_H
