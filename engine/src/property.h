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
// MCProperty class declarations
//
#ifndef	PROPERTY_H
#define	PROPERTY_H

#include "express.h"

class MCChunk;

class MCProperty : public MCExpression
{
	Chunk_term tocount;
	Chunk_term ptype;
	Properties which;
	MCChunk *target;
	Functions function;
	MCObject *parent;
	MCVarref *destvar;
	Boolean effective;
	MCNameRef customprop;
	MCExpression *customindex;
public:
	MCProperty();
	virtual ~MCProperty();
	virtual Parse_stat parse(MCScriptPoint &sp, Boolean the);
	virtual Exec_stat set(MCExecPoint &);
	virtual Exec_stat eval(MCExecPoint &);
	MCObject *getobj(MCExecPoint &ep);

private:
	Exec_stat resolveprop(MCExecPoint& ep, Properties& r_prop, MCNameRef& r_prop_name, MCNameRef& r_index_name);

	Exec_stat mode_set(MCExecPoint& ep);
	Exec_stat mode_eval(MCExecPoint& ep);
};
#endif
