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

#ifndef __INTERNAL_H
#define __INTERNAL_H

#include "statemnt.h"

////////////////////////////////////////////////////////////////////////////////

class MCScriptPoint;
class MCStatement;

////////////////////////////////////////////////////////////////////////////////

struct MCInternalVerbInfo
{
	const char *first_token;
	const char *second_token;
	MCStatement *(*factory)(void);
};

class MCInternal: public MCStatement
{
	MCStatement *f_statement;

public:
	MCInternal(void);

	virtual ~MCInternal(void);
	virtual Parse_stat parse(MCScriptPoint&);
    virtual void exec_ctxt(MCExecContext & ctxt);
};

inline MCInternal::MCInternal(void)
  : f_statement(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////

#endif
