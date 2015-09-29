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

//
// MCError class declarations
//
#ifndef	ERROR_H
#define	ERROR_H

class MCScriptPoint;
class MCExecPoint;

class MCError
{
	MCString svalue;
	char *buffer;
	uint2 errorline;
	uint2 errorpos;
	uint2 depth;
	Boolean thrown;
public:
	MCError()
	{
		buffer = MCU_empty();
		errorline = errorpos = 0;
		depth = 0;
		thrown = False;
	}
	~MCError()
	{
		delete buffer;
	}
	void add(uint2 id, MCScriptPoint &);
	void add(uint2 id, uint2 line, uint2 pos);
	void add(uint2 id, uint2 line, uint2 pos, uint32_t);
	void add(uint2 id, uint2 line, uint2 pos, const MCString &);
	void add(uint2 id, uint2 line, uint2 pos, MCNameRef);
	void append(MCError& string);
	const MCString &getsvalue();
	void copysvalue(const MCString &s, Boolean t);
	void clear();
	Boolean isempty()
	{
		return strlen(buffer) == 0;
	}
	void geterrorloc(uint2 &line, uint2 &pos);
};
#endif

