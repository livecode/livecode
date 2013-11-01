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
// MCExpression class declarations
//
#ifndef	EXPRESSION_H
#define	EXPRESSION_H

#define MAX_EXP 7

#ifndef __MC_EXEC__
#include "exec.h"
#endif

class MCExpression
{
protected:
	uint2 line;
	uint2 pos;
	Factor_rank rank;
	MCExpression *root;
	MCExpression *left;
	MCExpression *right;

public:
	MCExpression();
	virtual ~MCExpression();

	virtual MCExecValueType getvaluetype(void);
	
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);

	// Evaluate the exoression as a value, and place its value into ep.
	virtual Exec_stat eval(MCExecPoint &ep);
	
	// valueref, stringref, dataref, nameref, arrayref
	// bool, uint, int, double
	// enum, set, custom (?)
	virtual void eval_valueref(MCExecContext& ctxt, MCValueRef& r_value);
    virtual void eval_booleanref(MCExecContext& ctxt, MCBooleanRef& r_value);
    virtual void eval_stringref(MCExecContext& ctxt, MCStringRef& r_value);
    virtual void eval_dataref(MCExecContext& ctxt, MCDataRef& r_value);
    virtual void eval_nameref(MCExecContext& ctxt, MCNameRef& r_value);
    virtual void eval_numberref(MCExecContext& ctxt, MCNumberRef& r_value);
    virtual void eval_arrayref(MCExecContext& ctxt, MCArrayRef& r_value);
    
	virtual void eval_bool(MCExecContext& ctxt, bool& r_value);
	virtual void eval_int(MCExecContext& ctxt, integer_t& r_value);
	virtual void eval_uint(MCExecContext& ctxt, uinteger_t& r_value);
	virtual void eval_double(MCExecContext& ctxt, double& r_value);
	virtual void eval_char(MCExecContext& ctxt, char_t& r_value);
	virtual void eval_point(MCExecContext& ctxt, MCPoint& r_value);
	virtual void eval_rectangle(MCExecContext& ctxt, MCRectangle& r_value);
	virtual void eval_color(MCExecContext& ctxt, MCColor& r_value);
    
	// Compile the syntax into the (new) tree for use by the new evaluator.
	virtual void compile(MCSyntaxFactoryRef);
	virtual void compile_out(MCSyntaxFactoryRef);
	
	// Evaluate the expression as a container, and place the reference to
	// the container's value in r_ref.
	virtual Exec_stat evalcontainer(MCExecPoint& ep, MCContainer*& r_container);

	// Return the variable to which this expression refers in the context of
	// ep, or NULL if it is not just a variable expression.
	virtual MCVariable *evalvar(MCExecPoint& ep);

	// Return the var-ref which lies at the root of this expression. 
	// A return value of NULL means that there is no root variable.
	// The purpose of this call is to analyze (after parsing) whether the
	// left and right hand side of an variable mutation command share the
	// same variable. It is designed to be used at parse-time, not exec-time.
	virtual MCVarref *getrootvarref(void);

	void setrank(Factor_rank newrank)
	{
		rank = newrank;
	}
	void setroot(MCExpression *newroot)
	{
		root = newroot;
	}
	void setleft(MCExpression *newleft)
	{
		left = newleft;
	}
	void setright(MCExpression *newright)
	{
		right = newright;
	}
	Factor_rank getrank()
	{
		return rank;
	}
	MCExpression *getroot()
	{
		return root;
	}
	MCExpression *getleft()
	{
		return left;
	}
	MCExpression *getright()
	{
		return right;
	}
	Parse_stat getexps(MCScriptPoint &sp, MCExpression *earray[], uint2 &ecount);
	void freeexps(MCExpression *earray[], uint2 ecount);
	Parse_stat get0params(MCScriptPoint &);
	Parse_stat get0or1param(MCScriptPoint &sp, MCExpression **exp, Boolean the);
	Parse_stat get1param(MCScriptPoint &, MCExpression **exp, Boolean the);
	Parse_stat get1or2params(MCScriptPoint &, MCExpression **e1,
	                         MCExpression **e2, Boolean the);
	Parse_stat get2params(MCScriptPoint &, MCExpression **e1, MCExpression **e2);
	Parse_stat get2or3params(MCScriptPoint &, MCExpression **exp1,
	                         MCExpression **exp2, MCExpression **exp3);
	Parse_stat get3params(MCScriptPoint &, MCExpression **exp1,
	                      MCExpression **exp2, MCExpression **exp3);
	Parse_stat get4or5params(MCScriptPoint &, MCExpression **exp1,
	                         MCExpression **exp2, MCExpression **exp3,
	                         MCExpression **exp4, MCExpression **exp5);
	Parse_stat get6params(MCScriptPoint &, MCExpression **exp1,
	                      MCExpression **exp2, MCExpression **exp3,
	                      MCExpression **exp4, MCExpression **exp5,
	                      MCExpression **exp6);
	Parse_stat getvariableparams(MCScriptPoint &sp, uint32_t p_min_params, uint32_t p_param_count, ...);
	Parse_stat getparams(MCScriptPoint &spt, MCParameter **params);
	void initpoint(MCScriptPoint &);
	Exec_stat compare(MCExecPoint &, int2 &i, bool p_compare_arrays = false);
	
	static int2 compare_arrays(MCExecPoint &ep1, MCExecPoint &ep2, MCExecPoint *p_context);
	static int2 compare_values(MCExecPoint &ep1, MCExecPoint &ep2, MCExecPoint *p_context, bool p_compare_arrays);
	static bool compare_array_element(void *context, MCArrayRef array, MCNameRef key, MCValueRef value);
};

class MCFuncref : public MCExpression
{
	MCNameRef name;
    MCHandler *handler;
	MCObject *parent;
	MCParameter *params;
	bool resolved : 1;
    bool platform_message : 1;
public:
	MCFuncref(MCNameRef);
	virtual ~MCFuncref();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &ep);
};
#endif
