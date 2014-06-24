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

#include "prefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "scriptpt.h"
#include "execpt.h"
#include "cmds.h"
#include "handler.h"
#include "chunk.h"
#include "mcerror.h"
#include "globals.h"
#include "osspec.h"

#include <float.h>

//

inline bool MCMathOpCommandComputeOverlap(MCExpression *p_source, MCExpression *p_dest, MCVarref *p_destvar)
{
	MCVarref *t_src_ref;
	t_src_ref = p_source -> getrootvarref();
	if (t_src_ref == NULL)
		return false;

	if (p_destvar != NULL)
		return t_src_ref -> rootmatches(p_destvar);

	MCVarref *t_dst_ref;
	t_dst_ref = p_dest -> getrootvarref();
	if (t_dst_ref == NULL)
		 return false;

	return t_src_ref -> rootmatches(t_dst_ref);
}

//

MCAdd::~MCAdd()
{
	delete source;
	delete dest;
	// MW-2013-08-01: [[ Bug 10925 ]] Only delete the destvar varref if dest is NULL,
	//   otherwise its owned by dest.
	if (dest == NULL)
		delete destvar;
}

Parse_stat MCAdd::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add(PE_ADD_BADEXP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_TO) == PS_ERROR)
	{
		MCperror->add(PE_ADD_NOTO, sp);
		return PS_ERROR;
	}
	Symbol_type type;
	// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
	//   execution outwith a handler.
	if (sp.next(type) != PS_NORMAL || type != ST_ID || sp.findvar(sp.gettoken_nameref(), &destvar) != PS_NORMAL)
	{
		sp.backup();
		dest = new MCChunk(True);
		if (dest->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_ADD_BADDEST, sp);
			return PS_ERROR;
		}
	}
	else
		destvar->parsearray(sp);
	
	// MW-2013-08-01: [[ Bug 10925 ]] If the dest chunk is just a var, extract the varref.
	if (dest != NULL && dest -> isvarchunk())
		destvar = dest -> getrootvarref();

	overlap = MCMathOpCommandComputeOverlap(source, dest, destvar);

	return PS_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the source can be an array or number so we use 'tona'.
Exec_stat MCAdd::exec(MCExecPoint &ep)
{
#ifdef /* MCAdd */ LEGACY_EXEC
	MCVariable *t_dst_var;
	MCVariableValue *t_dst_ref;
	t_dst_ref = NULL;

	if (source->eval(ep) != ES_NORMAL || ep.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_ADD_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	if (overlap)
		ep . grab();
	
	if (destvar != NULL && destvar -> evalcontainer(ep, t_dst_var, t_dst_ref) != ES_NORMAL)
	{
		MCeerror->add(EE_ADD_BADDEST, line, pos);
		return ES_ERROR;
	}

	if (t_dst_ref != NULL && t_dst_ref -> is_array())
	{
		if (t_dst_ref->factorarray(ep, O_PLUS) != ES_NORMAL)
		{
			MCeerror->add(EE_ADD_BADARRAY, line, pos);
			return ES_ERROR;
		}
		return ES_NORMAL;
	}

	if (ep.getformat() == VF_ARRAY)
	{
		MCeerror->add(EE_ADD_MISMATCH, line, pos);
		return ES_ERROR;
	}

	// Variable case
	real8 n1 = ep.getnvalue();
	if (t_dst_ref != NULL)
	{
		real8 n2;
		if (!t_dst_ref -> get_as_real(ep, n2))
		{
			MCeerror -> add(EE_ADD_BADDEST, line, pos);
			return ES_ERROR;
		}

		t_dst_ref -> assign_real(n1 + n2);

		if (t_dst_var != NULL)
			t_dst_var -> synchronize(ep, True);

		return ES_NORMAL;
	}

	// Chunk case
	if (dest->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ADD_BADDEST, line, pos);
		return ES_ERROR;
	}
	real8 n2 = ep.getnvalue();
	ep.setnvalue(n1 + n2);
	if (dest->set(ep, PT_INTO) != ES_NORMAL)
	{
		MCeerror->add(EE_ADD_CANTSET, line, pos);
		return ES_ERROR;
	}

	overlap = MCMathOpCommandComputeOverlap(source, dest, destvar);

	return ES_NORMAL;
#endif /* MCAdd */
}

MCDivide::~MCDivide()
{
	delete source;
	delete dest;
	// MW-2013-08-01: [[ Bug 10925 ]] Only delete the destvar varref if dest is NULL,
	//   otherwise its owned by dest.
	if (dest == NULL)
		delete destvar;
}

Parse_stat MCDivide::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
	//   execution outwith a handler.
	Symbol_type type;
	if (sp.next(type) != PS_NORMAL
	        || type != ST_ID || sp . findvar(sp.gettoken_nameref(), &destvar) != PS_NORMAL)
	{
		sp.backup();
		dest = new MCChunk(True);
		if (dest->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_DIVIDE_BADDEST, sp);
			return PS_ERROR;
		}
	}
	else
		destvar->parsearray(sp);
	if (sp.skip_token(SP_FACTOR, TT_PREP) == PS_ERROR)
	{
		MCperror->add(PE_DIVIDE_NOBY, sp);
		return PS_ERROR;
	}
	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add(PE_DIVIDE_BADEXP, sp);
		return PS_ERROR;
	}
	
	// MW-2013-08-01: [[ Bug 10925 ]] If the dest chunk is just a var, extract the varref.
	if (dest != NULL && dest -> isvarchunk())
		destvar = dest -> getrootvarref();
	
	overlap = MCMathOpCommandComputeOverlap(source, dest, destvar);

	return PS_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the source can be an array or number so we use 'tona'.
Exec_stat MCDivide::exec(MCExecPoint &ep)
{
#ifdef /* MCDivide */ LEGACY_EXEC
	MCVariable *t_dst_var;
	MCVariableValue *t_dst_ref;
	t_dst_ref = NULL;

	if (source->eval(ep) != ES_NORMAL || ep.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_DIVIDE_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	if (overlap)
		ep . grab();

	if (destvar != NULL && destvar -> evalcontainer(ep, t_dst_var, t_dst_ref) != ES_NORMAL)
	{
		MCeerror->add(EE_DIVIDE_BADDEST, line, pos);
		return ES_ERROR;
	}

	if (t_dst_ref != NULL && t_dst_ref -> is_array())
	{
		if (t_dst_ref->factorarray(ep, O_OVER) != ES_NORMAL)
		{
			MCeerror->add(EE_DIVIDE_BADARRAY, line, pos);
			return ES_ERROR;
		}
		return ES_NORMAL;
	}

	if (ep.getformat() == VF_ARRAY)
	{
		MCeerror->add(EE_DIVIDE_MISMATCH, line, pos);
		return ES_ERROR;
	}

	// Variable case
	real8 n2 = ep.getnvalue();
	if (t_dst_ref != NULL)
	{
		real8 n1;
		if (!t_dst_ref -> get_as_real(ep, n1))
		{
			MCeerror -> add(EE_ADD_BADDEST, line, pos);
			return ES_ERROR;
		}
		
		MCS_seterrno(0);
		n1 /= n2;
		if (n1 == MCinfinity || MCS_geterrno() != 0)
		{
			MCS_seterrno(0);
			if (n2 == 0.0)
				MCeerror->add(EE_DIVIDE_ZERO, line, pos);
			else
				MCeerror->add(EE_DIVIDE_RANGE, line, pos);
			return ES_ERROR;
		}
		t_dst_ref -> assign_real(n1);

		if (t_dst_var != NULL)
			t_dst_var -> synchronize(ep, True);

		return ES_NORMAL;
	}

	// Chunk case
	if (dest->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_DIVIDE_BADDEST, line, pos);
		return ES_ERROR;
	}
	real8 n1 = ep.getnvalue();
	MCS_seterrno(0);
	n1 = n1 / n2;
	if (n1 == MCinfinity || MCS_geterrno() != 0)
	{
		MCS_seterrno(0);
		if (n2 == 0.0)
			MCeerror->add(EE_DIVIDE_ZERO, line, pos);
		else
			MCeerror->add(EE_DIVIDE_RANGE, line, pos);
		return ES_ERROR;
	}
	ep.setnvalue(n1);
	if (dest->set(ep, PT_INTO) != ES_NORMAL)
	{
		MCeerror->add(EE_DIVIDE_CANTSET, line, pos);
		return ES_ERROR;
	}

	return ES_NORMAL;
#endif /* MCDivide */
}

MCMultiply::~MCMultiply()
{
	delete source;
	delete dest;
	// MW-2013-08-01: [[ Bug 10925 ]] Only delete the destvar varref if dest is NULL,
	//   otherwise its owned by dest.
	if (dest == NULL)
		delete destvar;
}

Parse_stat MCMultiply::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
	//   execution outwith a handler.
	Symbol_type type;
	if (sp.next(type) != PS_NORMAL
	        || type != ST_ID || sp . findvar(sp.gettoken_nameref(), &destvar) != PS_NORMAL)
	{
		sp.backup();
		dest = new MCChunk(True);
		if (dest->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_MULTIPLY_BADDEST, sp);
			return PS_ERROR;
		}
	}
	else
		destvar->parsearray(sp);
	if (sp.skip_token(SP_FACTOR, TT_PREP) == PS_ERROR)
	{
		MCperror->add
		(PE_MULTIPLY_NOBY, sp);
		return PS_ERROR;
	}
	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add
		(PE_MULTIPLY_BADEXP, sp);
		return PS_ERROR;
	}
	
	// MW-2013-08-01: [[ Bug 10925 ]] If the dest chunk is just a var, extract the varref.
	if (dest != NULL && dest -> isvarchunk())
		destvar = dest -> getrootvarref();
	
	overlap = MCMathOpCommandComputeOverlap(source, dest, destvar);

	return PS_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the source can be an array or number so we use 'tona'.
Exec_stat MCMultiply::exec(MCExecPoint &ep)
{
#ifdef /* MCMultiply */ LEGACY_EXEC
	MCVariable *t_dst_var;
	MCVariableValue *t_dst_ref;
	t_dst_ref = NULL;

	if (source->eval(ep) != ES_NORMAL || ep.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_MULTIPLY_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	if (overlap)
		ep . grab();

	if (destvar != NULL && destvar -> evalcontainer(ep, t_dst_var, t_dst_ref) != ES_NORMAL)
	{
		MCeerror->add(EE_MULTIPLY_BADDEST, line, pos);
		return ES_ERROR;
	}

	if (t_dst_ref != NULL && t_dst_ref -> is_array())
	{
		if (t_dst_ref->factorarray(ep, O_TIMES) != ES_NORMAL)
		{
			MCeerror->add(EE_MULTIPLY_BADARRAY, line, pos);
			return ES_ERROR;
		}
		return ES_NORMAL;
	}

	if (ep.getformat() == VF_ARRAY)
	{
		MCeerror->add(EE_MULTIPLY_MISMATCH, line, pos);
		return ES_ERROR;
	}

	// Variable case
	real8 n2 = ep.getnvalue();
	if (t_dst_ref != NULL)
	{
		real8 n1;
		if (!t_dst_ref -> get_as_real(ep, n1))
		{
			MCeerror -> add(EE_MULTIPLY_BADDEST, line, pos);
			return ES_ERROR;
		}
		
		MCS_seterrno(0);
		n1 *= n2;
		if (n1 == MCinfinity || MCS_geterrno() != 0)
		{
			MCS_seterrno(0);
			MCeerror->add(EE_MULTIPLY_RANGE, line, pos);
			return ES_ERROR;
		}
		t_dst_ref -> assign_real(n1);

		if (t_dst_var != NULL)
			t_dst_var -> synchronize(ep, True);

		return ES_NORMAL;
	}

	// Chunk case
	if (dest->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_MULTIPLY_BADDEST, line, pos);
		return ES_ERROR;
	}
	real8 n1 = ep.getnvalue();
	MCS_seterrno(0);
	n1 *= n2;
	if (n1 == MCinfinity || MCS_geterrno() != 0)
	{
		MCS_seterrno(0);
		MCeerror->add(EE_MULTIPLY_RANGE, line, pos);
		return ES_ERROR;
	}
	ep.setnvalue(n1);
	if (dest->set(ep, PT_INTO) != ES_NORMAL)
	{
		MCeerror->add(EE_MULTIPLY_CANTSET, line, pos);
		return ES_ERROR;
	}

	return ES_NORMAL;
#endif /* MCMultiply */
}

MCSubtract::~MCSubtract()
{
	delete source;
	delete dest;
	// MW-2013-08-01: [[ Bug 10925 ]] Only delete the destvar varref if dest is NULL,
	//   otherwise its owned by dest.
	if (dest == NULL)
		delete destvar;
}

Parse_stat MCSubtract::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add
		(PE_SUBTRACT_BADEXP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_FROM) == PS_ERROR)
	{
		MCperror->add
		(PE_SUBTRACT_NOFROM, sp);
		return PS_ERROR;
	}
	// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
	//   execution outwith a handler.
	Symbol_type type;
	if (sp.next(type) != PS_NORMAL
	        || type != ST_ID || sp . findvar(sp.gettoken_nameref(), &destvar) != PS_NORMAL)
	{
		sp.backup();
		dest = new MCChunk(True);
		if (dest->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_SUBTRACT_BADDEST, sp);
			return PS_ERROR;
		}
	}
	else
		destvar->parsearray(sp);
	
	// MW-2013-08-01: [[ Bug 10925 ]] If the dest chunk is just a var, extract the varref.
	if (dest != NULL && dest -> isvarchunk())
		destvar = dest -> getrootvarref();
	
	overlap = MCMathOpCommandComputeOverlap(source, dest, destvar);

	return PS_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the source can be an array or number so we use 'tona'.
Exec_stat MCSubtract::exec(MCExecPoint &ep)
{
#ifdef /* MCSubtract */ LEGACY_EXEC
	MCVariable *t_dst_var;
	MCVariableValue *t_dst_ref;
	t_dst_ref = NULL;

	if (source->eval(ep) != ES_NORMAL || ep.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_SUBTRACT_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	if (overlap)
		ep . grab();

	if (destvar != NULL && destvar -> evalcontainer(ep, t_dst_var, t_dst_ref) != ES_NORMAL)
	{
		MCeerror->add(EE_SUBTRACT_BADDEST, line, pos);
		return ES_ERROR;
	}

	if (t_dst_ref != NULL && t_dst_ref -> is_array())
	{
		if (t_dst_ref->factorarray(ep, O_MINUS) != ES_NORMAL)
		{
			MCeerror->add(EE_SUBTRACT_BADARRAY, line, pos);
			return ES_ERROR;
		}
		return ES_NORMAL;
	}

	if (ep.getformat() == VF_ARRAY)
	{
		MCeerror->add(EE_SUBTRACT_MISMATCH, line, pos);
		return ES_ERROR;
	}
	// Variable case
	real8 n1 = ep.getnvalue();
	if (t_dst_ref != NULL)
	{
		real8 n2;
		if (!t_dst_ref -> get_as_real(ep, n2))
		{
			MCeerror -> add(EE_SUBTRACT_BADDEST, line, pos);
			return ES_ERROR;
		}
		
		t_dst_ref -> assign_real(n2 - n1);

		if (t_dst_var != NULL)
			t_dst_var -> synchronize(ep, True);

		return ES_NORMAL;
	}

	// Chunk case
	if (dest->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_SUBTRACT_BADDEST, line, pos);
		return ES_ERROR;
	}
	real8 n2 = ep.getnvalue();
	ep.setnvalue(n2 - n1);
	if (dest->set(ep, PT_INTO) != ES_NORMAL)
	{
		MCeerror->add(EE_SUBTRACT_CANTSET, line, pos);
		return ES_ERROR;
	}

	return ES_NORMAL;
#endif /* MCSubtract */
}

MCArrayOp::~MCArrayOp()
{
	delete destvar;
	delete element;
	delete key;
}


Parse_stat MCArrayOp::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	Symbol_type type;

	// MW-2008-08-20: [[ Bug 6954 ]] Split/Combine don't work on array keys
	// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
	//   execution outwith a handler.
	if (sp.next(type) != PS_NORMAL || type != ST_ID
	        || sp.findvar(sp.gettoken_nameref(), &destvar) != PS_NORMAL
			|| destvar -> parsearray(sp) != PS_NORMAL)
	{
		MCperror->add(PE_ARRAYOP_BADARRAY, sp);
		return PS_ERROR;
	}
	
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) != PS_NORMAL
	        && sp.skip_token(SP_FACTOR, TT_PREP, PT_BY) != PS_NORMAL
	        && sp.skip_token(SP_START, TT_UNDEFINED, SC_USING) != PS_NORMAL)
	{
		MCperror->add(PE_ARRAYOP_NOWITH, sp);
		return PS_ERROR;
	}
	
	if (sp . next(type) == PS_NORMAL && type == ST_ID && 
		  (sp . gettoken() == "column" || sp . gettoken() == "row"))
	{
		if (sp . gettoken() == "column")
			mode = TYPE_COLUMN;
		else
			mode = TYPE_ROW;
	}
	else
	{
		sp.backup();

		if (sp.parseexp(True, False, &element) != PS_NORMAL)
		{
			MCperror->add(PE_ARRAYOP_BADEXP, sp);
			return PS_ERROR;
		}

		if (sp.skip_token(SP_FACTOR, TT_BINOP, O_AND) == PS_NORMAL)
			if (sp.parseexp(True, False, &key) != PS_NORMAL)
			{
				MCperror->add(PE_ARRAYOP_BADEXP, sp);
				return PS_ERROR;
			}
	}
	
	if (sp . skip_token(SP_FACTOR, TT_PREP, PT_AS) == PS_NORMAL)
	{
		if (sp . skip_token(SP_COMMAND, TT_STATEMENT, S_SET) != PS_NORMAL ||
			key != nil)
		{
			MCperror -> add(PE_ARRAYOP_BADFORM, sp);
			return PS_ERROR;
		}
		
		form = FORM_SET;
	}

	return PS_NORMAL;
}

Exec_stat MCArrayOp::exec(MCExecPoint &ep)
{
#ifdef /* MCArrayOp */ LEGACY_EXEC
	uint1 e;
	uint1 k = '\0';
	uint4 chunk;
	chunk = mode;
	switch(chunk)
	{
		case TYPE_USER:
			if (element != NULL)
			{
				if (element->eval(ep) != ES_NORMAL || ep.tos() != ES_NORMAL
								|| ep.getsvalue().getlength() != 1)
				{
					MCeerror->add(EE_ARRAYOP_BADEXP, line, pos);
					return ES_ERROR;
				}
				e = ep.getsvalue().getstring()[0];
				if (key != NULL)
				{
					if (key->eval(ep) != ES_NORMAL || ep.tos() != ES_NORMAL
									|| ep.getsvalue().getlength() != 1)
					{
						MCeerror->add(EE_ARRAYOP_BADEXP, line, pos);
						return ES_ERROR;
					}
					k = ep.getsvalue().getstring()[0];
				}
			}
		break;
		case TYPE_ROW:
			e = ep . getrowdel();
		break;
		case TYPE_COLUMN:
			e = ep . getcolumndel();
		break;
		case TYPE_LINE:
			e = ep . getlinedel();
		break;
		case TYPE_ITEM:
			e = ep . getitemdel();
		break;
		case TYPE_WORD:
		case TYPE_TOKEN:
		case TYPE_CHARACTER:
		default:
			return ES_ERROR;
		break;
	}

	MCVariable *t_dst_var;
	MCVariableValue *t_dst_ref;
	if (destvar -> evalcontainer(ep, t_dst_var, t_dst_ref) != ES_NORMAL)
	{
		MCeerror -> add(EE_ARRAYOP_BADEXP, line, pos);
		return ES_ERROR;
	}

	if (is_combine)
	{
		if (form == FORM_NONE)
		{
			if (chunk == TYPE_COLUMN)
				t_dst_ref -> combine_column(e, ep . getrowdel(), ep);
			else
				t_dst_ref -> combine(e, k, ep);
		}
		else
			t_dst_ref -> combine_as_set(e, ep);
	}
	else
	{
		if (form == FORM_NONE)
		{
			if (chunk == TYPE_COLUMN)
				t_dst_ref -> split_column(e, ep . getrowdel(), ep);
			else
				t_dst_ref -> split(e, k, ep);
		}
		else
			t_dst_ref -> split_as_set(e, ep);
	}

	if (t_dst_var != NULL)
		t_dst_var -> synchronize(ep, True);

	return ES_NORMAL;
#endif /* MCArrayOp */
}

MCSetOp::~MCSetOp()
{
	delete destvar;
	delete source;
}

Parse_stat MCSetOp::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
	//   execution outwith a handler.
	Symbol_type type;
	if (sp.next(type) != PS_NORMAL || type != ST_ID
	        || sp.findvar(sp.gettoken_nameref(), &destvar) != PS_NORMAL
			|| destvar -> parsearray(sp) != PS_NORMAL)
	{
		MCperror->add(PE_ARRAYOP_BADARRAY, sp);
		return PS_ERROR;
	}

	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_ERROR
	        && sp.skip_token(SP_FACTOR, TT_PREP, PT_BY) == PS_ERROR)
	{
		MCperror->add(PE_ARRAYOP_NOWITH, sp);
		return PS_ERROR;
	}

	if (sp.parseexp(True, False, &source) != PS_NORMAL)
	{
		MCperror->add(PE_ARRAYOP_BADEXP, sp);
		return PS_ERROR;
	}
    
    // MERG-2013-08-26: [[ RecursiveArrayOp ]] Support nested arrays in union and intersect
    recursive = sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_RECURSIVELY) == PS_NORMAL;

	MCVarref *t_src_ref, *t_dst_ref;
	t_src_ref = source -> getrootvarref();
	t_dst_ref = destvar -> getrootvarref();
	overlap = t_src_ref != NULL && t_dst_ref != NULL && t_src_ref -> rootmatches(t_dst_ref);

	return PS_NORMAL;
}

Exec_stat MCSetOp::exec(MCExecPoint &ep)
{
#ifdef /* MCSetOp */ LEGACY_EXEC
	// ARRAYEVAL
	if (source -> eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_ARRAYOP_BADEXP, line, pos);
		return ES_ERROR;
	}

	if (ep . getformat() != VF_ARRAY)
		ep . clear();

	if (overlap)
		ep . grab();

	MCVariable *t_dst_var;
	MCVariableValue *t_dst_ref;
	if (destvar -> evalcontainer(ep, t_dst_var, t_dst_ref) != ES_NORMAL)
	{
		MCeerror -> add(EE_ARRAYOP_BADEXP, line, pos);
		return ES_ERROR;
	}

	MCVariableValue *t_src_ref;
	t_src_ref = ep . getarray();
	
	// Do nothing if source and dest are the same
	if (t_src_ref == t_dst_ref)
		return ES_NORMAL;

	if (intersect)
	{
		if (t_src_ref == NULL)
			t_dst_ref -> assign_empty();
		else
			// MERG-2013-08-26: [[ RecursiveArrayOp ]] Support nested arrays in union and intersect
            t_dst_ref -> intersectarray(*t_src_ref,recursive);
	}
	else
	{
		if (t_src_ref == NULL)
			return ES_NORMAL;

		// MERG-2013-08-26: [[ RecursiveArrayOp ]] Support nested arrays in union and intersect
        t_dst_ref -> unionarray(*t_src_ref,recursive);
	}

	if (t_dst_var != NULL)
		t_dst_var -> synchronize(ep, True);

	return ES_NORMAL;
#endif /* MCSetOp */
}
