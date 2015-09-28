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
#include "parsedef.h"
#include "filedefs.h"

#include "scriptpt.h"
#include "execpt.h"
#include "mcerror.h"
#include "util.h"

#include "globals.h"

void MCError::add(uint2 id, MCScriptPoint &sp)
{
	add(id, sp.getline(), sp.getpos(), sp.gettoken());
}

void MCError::add(uint2 id, uint2 line, uint2 pos)
{
	add(id, line, pos, MCnullmcstring);
}

void MCError::add(uint2 id, uint2 line, uint2 pos, uint32_t v)
{
	char t_buffer[U4L];
	sprintf(t_buffer, "%u", v);
	add(id, line, pos, t_buffer);
}

void MCError::add(uint2 id, uint2 line, uint2 pos, MCNameRef n)
{
	add(id, line, pos, MCNameGetOldString(n));
}

void MCError::add(uint2 id, uint2 line, uint2 pos, const MCString &token)
{
	if (MCerrorlock != 0 || thrown)
		return;
	if (line != 0 && errorline == 0)
	{
		errorline = line;
		errorpos = pos;
	}
	if (depth > 1024)
		return;
	char *newerror = new char[U2L * 3 + token.getlength()];
	if (token == MCnullmcstring)
		sprintf(newerror, "%d,%d,%d", id, line, pos);
	else
	{
		const char *eptr = token.getstring();
		int4 length = 0;
		while (length < (int4)token.getlength())
		{
			if (*eptr++ == '\n')
				break;
			length++;
		}
		sprintf(newerror, "%d,%d,%d,%*.*s", id, line, pos,
		        length, length, token.getstring());
	}
	MCU_addline(buffer, newerror, strlen(buffer) == 0);
	depth += 1;
	delete newerror;
}

void MCError::append(MCError& p_other)
{
	MCU_addline(buffer, p_other . buffer, strlen(buffer) == 0);
}

const MCString &MCError::getsvalue()
{
	if (!thrown)
		svalue.set(buffer, strlen(buffer));
	return svalue;
}

void MCError::copysvalue(const MCString &s, Boolean t)
{
	delete buffer;
	buffer = s.clone();
	thrown = t;
	if (thrown)
		svalue.set(buffer, strlen(buffer));
}

void MCError::clear()
{
	delete buffer;
	errorline = errorpos = 0;
	depth = 0;
	buffer = MCU_empty();
	thrown = False;
	if (this == MCeerror)
		MCerrorptr = NULL;
}

void MCError::geterrorloc(uint2 &line, uint2 &pos)
{
	line = errorline;
	pos = errorpos;
}
