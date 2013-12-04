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

//
// MCProperty class declarations
//
#ifndef	PROPERTY_H
#define	PROPERTY_H

#include "express.h"

class MCChunk;

struct MCPropertyTable
{
	uindex_t size;
	MCPropertyInfo *table;
};

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
	//virtual Exec_stat set(MCExecPoint &);
	//virtual Exec_stat eval(MCExecPoint &);
    virtual void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
    virtual void set(MCExecContext& ctxt, MCExecValue p_value);
#ifdef LEGACY_EXEC
	MCObject *getobj(MCExecPoint &ep);
#endif
    MCObject *getobj(MCExecContext &ctxt);
    
    const MCPropertyTable *getmodepropertytable(void) const { return &kModePropertyTable; }

private:
#ifdef LEGACY_EXEC
	Exec_stat eval_variable(MCExecPoint& ep);
	Exec_stat eval_function(MCExecPoint& ep);
	Exec_stat eval_global_property(MCExecPoint& ep);
	Exec_stat eval_object_property(MCExecPoint& ep);
	Exec_stat eval_count(MCExecPoint& ep);
  
    Exec_stat set_variable(MCExecPoint& ep);
	Exec_stat set_global_property(MCExecPoint& ep);
	Exec_stat set_object_property(MCExecPoint& ep);
#endif

    void eval_variable_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
    void eval_function_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
    void eval_global_property_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
    void eval_object_property_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
    void eval_count_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
    void set_variable(MCExecContext& ctxt, MCExecValue p_value);
	void set_global_property(MCExecContext& ctxt, MCExecValue p_value);
	void set_object_property(MCExecContext& ctxt, MCExecValue p_value);

#ifdef LEGACY_EXEC
	Exec_stat resolveprop(MCExecPoint& ep, Properties& r_prop, MCNameRef& r_prop_name, MCNameRef& r_index_name);
#endif
    bool resolveprop(MCExecContext& ctxt, Properties& r_which, MCNameRef& r_prop_name, MCNameRef& r_index_name);

    static MCPropertyInfo kModeProperties[];
	static MCPropertyTable kModePropertyTable;
#ifdef LEGACY_EXEC
    Exec_stat mode_set(MCExecPoint& ep);
	Exec_stat mode_eval(MCExecPoint& ep);
#endif
};
#endif
