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

#include "mcerror.h"
#include "util.h"

#include "globals.h"

void MCError::add(uint2 id, MCScriptPoint &sp)
{
	add(id, sp.getline(), sp.getpos(), sp.gettoken_stringref());
}

void MCError::add(uint2 id, uint2 line, uint2 pos)
{
	add(id, line, pos, kMCEmptyString);
}

void MCError::add(uint2 id, uint2 line, uint2 pos, const char *msg)
{
    MCAutoStringRef t_string;
    MCStringCreateWithCString(msg, &t_string);
	add(id, line, pos, *t_string);
}

void MCError::add(uint2 id, uint2 line, uint2 pos, uint32_t v)
{
	char t_buffer[U4L];
	sprintf(t_buffer, "%u", v);
	add(id, line, pos, t_buffer);
}

void MCError::add(uint2 id, uint2 line, uint2 pos, MCValueRef n)
{
    if (MCerrorlock || thrown)
		return;
    
    if (n != nil)
	{
		MCAutoStringRef t_string;
		MCValueConvertToStringForSave(n, &t_string);
		doadd(id, line, pos, *t_string);
	}
	else
	{
		doadd(id, line, pos, kMCEmptyString);
	}
}

void MCError::doadd(uint2 id, uint2 line, uint2 pos, MCStringRef p_token)
{
	if (line != 0 && errorline == 0)
	{
		errorline = line;
		errorpos = pos;
	}
	if (depth > 1024)
		return;

    MCAutoStringRef newerror;
    
	if (MCStringIsEmpty(p_token))
		/* UNCHECKED */ MCStringFormat(&newerror, "%d,%d,%d", id, line, pos);
	else
	{
        MCStringRef t_line = nil;
        uindex_t t_newline;
        if (MCStringFirstIndexOfChar(p_token, '\n', 0, kMCCompareExact, t_newline))
            /* UNCHECKED */ MCStringCopySubstring(p_token, MCRangeMake(0, t_newline), t_line);

		/* UNCHECKED */ MCStringFormat(&newerror, "%d,%d,%d,%@", id, line, pos, t_line != nil ? t_line : p_token);
        
        MCValueRelease(t_line);
	}
    if (!MCStringIsEmpty(*buffer))
        /* UNCHECKED */ MCStringAppendChar(*buffer, '\n');
    
	/* UNCHECKED */ MCStringAppend(*buffer, *newerror);
	depth += 1;
}

void MCError::append(MCError& p_other)
{
	/* UNCHECKED */ MCStringAppendFormat(*buffer,
                                         MCStringIsEmpty(*buffer) ? "%@" : "\n%@",
                                         *p_other.buffer);
}

void MCError::copystringref(MCStringRef s, Boolean t)
{
    buffer.Reset();
    /* UNCHECKED */ MCStringMutableCopy(s, &buffer);
	thrown = t;
}

bool MCError::copyasstringref(MCStringRef &r_string)
{
	return MCStringCopy(*buffer, r_string);
}

void MCError::clear()
{
	errorline = errorpos = 0;
	depth = 0;
    buffer.Reset();
    /* UNCHECKED */ MCStringCreateMutable(0, &buffer);
	thrown = False;
	if (this == MCeerror)
		MCerrorptr = nil;
}

void MCError::geterrorloc(uint2 &line, uint2 &pos)
{
	line = errorline;
	pos = errorpos;
}
