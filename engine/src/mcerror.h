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

class MCError
{
	MCAutoStringRef buffer;
	uint2 errorline;
	uint2 errorpos;
	uint2 depth;
	Boolean thrown;
public:
	MCError()
	{
		/* UNCHECKED */ MCStringCreateMutable(0, &buffer);
		errorline = errorpos = 0;
		depth = 0;
		thrown = False;
	}
	void add(uint2 id, MCScriptPoint &);
	void add(uint2 id, uint2 line, uint2 pos);
	void add(uint2 id, uint2 line, uint2 pos, uint32_t);
	// void add(uint2 id, uint2 line, uint2 pos, const MCString &);
	void add(uint2 id, uint2 line, uint2 pos, const char *);
	void add(uint2 id, uint2 line, uint2 pos, MCValueRef);
	void append(MCError& string);
	void copystringref(MCStringRef s, Boolean t);
	bool copyasstringref(MCStringRef &r_string);
	bool isthrown(void) const {return thrown;}
	void clear();
	Boolean isempty()
	{
		return MCStringIsEmpty(*buffer);
	}
	void geterrorloc(uint2 &line, uint2 &pos);

private:
	void doadd(uint2 id, uint2 line, uint2 pos, MCStringRef token);
};
#endif

