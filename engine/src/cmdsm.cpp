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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "scriptpt.h"

#include "cmds.h"
#include "handler.h"
#include "chunk.h"
#include "mcerror.h"
#include "globals.h"
#include "osspec.h"
#include "exec.h"
#include "syntax.h"
#include "variable.h"

#include <float.h>

////////////////////////////////////////////////////////////////////////

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
		dest = new (nothrow) MCChunk(True);
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

	return PS_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the source can be an array or number so we use 'tona'.
void MCAdd::exec_ctxt(MCExecContext &ctxt)
{
    MCExecValue t_src;
	
    if (!ctxt . EvaluateExpression(source, EE_ADD_BADSOURCE, t_src)
            || !ctxt . ConvertToNumberOrArray(t_src))
	{
		ctxt.LegacyThrow(EE_ADD_BADSOURCE);
        return;
    }
	
	MCExecValue t_dst;
    MCContainer t_dst_container;
	if (destvar != nil)
	{
        bool t_success;
        if (destvar -> needsContainer())
            t_success = destvar -> evalcontainer(ctxt, t_dst_container)
                            && t_dst_container.eval_ctxt(ctxt, t_dst);
        else
        {
            destvar -> eval_ctxt(ctxt, t_dst);
            t_success = !ctxt . HasError();
        }
        
        if (!t_success)
        {
            ctxt . LegacyThrow(EE_ADD_BADDEST);
            MCExecTypeRelease(t_src);
            return;
        }
            
	}
	else
    {
        if (!ctxt . EvaluateExpression(dest, EE_ADD_BADDEST, t_dst))
        {
            MCExecTypeRelease(t_src);
            return;
        }
    }

    if (!ctxt . ConvertToNumberOrArray(t_dst))
    {
        MCExecTypeRelease(t_src);
        MCExecTypeRelease(t_dst);
		ctxt.LegacyThrow(EE_ADD_BADDEST);
		return;
    }

	MCExecValue t_result;
    t_result . type = t_dst . type;
    if (t_src . type == kMCExecValueTypeArrayRef)
	{
        if (t_dst . type == kMCExecValueTypeArrayRef)
            MCMathExecAddArrayToArray(ctxt, t_src . arrayref_value, t_dst . arrayref_value, t_result . arrayref_value);
		else
		{
            ctxt . LegacyThrow(EE_ADD_MISMATCH);
            return;
		}
	}
	else
	{
        if (t_dst . type == kMCExecValueTypeArrayRef)
            MCMathExecAddNumberToArray(ctxt, t_src . double_value, t_dst . arrayref_value, t_result . arrayref_value);
		else
            MCMathExecAddNumberToNumber(ctxt, t_src . double_value, t_dst . double_value, t_result . double_value);
	}
    
    MCExecTypeRelease(t_src);
    MCExecTypeRelease(t_dst);
	
	if (!ctxt . HasError())
	{
		if (destvar != nil)
		{
            bool t_success;
            if (destvar -> needsContainer())
                t_success = t_dst_container.give_value(ctxt, t_result);
            else
                t_success = destvar -> give_value(ctxt, t_result);
            
            if (!t_success)
                ctxt . Throw();
		}
		else
		{
			if (dest->set(ctxt, PT_INTO, t_result))
				return;
			ctxt . LegacyThrow(EE_ADD_CANTSET);
		}
	}
}

void MCAdd::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	source -> compile(ctxt);

	if (destvar != nil)
		destvar -> compile_inout(ctxt);
	else
		dest -> compile_inout(ctxt);

	MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCMathExecAddArrayToArrayMethodInfo, 0, 1, 1);
	MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCMathExecAddNumberToArrayMethodInfo, 0, 1, 1);
	MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCMathExecAddNumberToNumberMethodInfo, 0, 1, 1);

	MCSyntaxFactoryEndStatement(ctxt);
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
		dest = new (nothrow) MCChunk(True);
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

	return PS_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the source can be an array or number so we use 'tona'.
void MCDivide::exec_ctxt(MCExecContext &ctxt)
{
	MCExecValue t_src;
    
    if (!ctxt . EvaluateExpression(source, EE_DIVIDE_BADSOURCE, t_src)
            || !ctxt . ConvertToNumberOrArray(t_src))
	{
		ctxt.LegacyThrow(EE_DIVIDE_BADSOURCE);
        return;
    }
	
	MCExecValue t_dst;
	MCContainer t_dst_container;
	if (destvar != nil)
	{
        bool t_success;
        if (destvar -> needsContainer())
            t_success = destvar -> evalcontainer(ctxt, t_dst_container)
                            && t_dst_container.eval_ctxt(ctxt, t_dst);
        else
        {
            destvar -> eval_ctxt(ctxt, t_dst);
            t_success = !ctxt.HasError();
        }
        
        if (!t_success)
        {
            ctxt . LegacyThrow(EE_DIVIDE_BADDEST);
            MCExecTypeRelease(t_src);
            return;
        }
	}
	else
	{
        if (!ctxt . EvaluateExpression(dest, EE_DIVIDE_BADDEST, t_dst))
        {
            MCExecTypeRelease(t_src);
            return;
        }
	}

    if (!ctxt . ConvertToNumberOrArray(t_dst))
    {
        MCExecTypeRelease(t_src);
		MCExecTypeRelease(t_dst);
		ctxt.LegacyThrow(EE_DIVIDE_BADDEST);
        return;
    }
	
	MCExecValue t_result;
    t_result . type = t_dst . type;
    if (t_src . type == kMCExecValueTypeArrayRef)
	{
        if (t_dst . type == kMCExecValueTypeArrayRef)
            MCMathExecDivideArrayByArray(ctxt, t_dst .arrayref_value, t_src . arrayref_value, t_result . arrayref_value);
		else
		{
            ctxt . LegacyThrow(EE_DIVIDE_MISMATCH);
            return;
		}
	}
	else
	{
        if (t_dst . type == kMCExecValueTypeArrayRef)
            MCMathExecDivideArrayByNumber(ctxt, t_dst . arrayref_value, t_src . double_value, t_result . arrayref_value);
		else
            MCMathExecDivideNumberByNumber(ctxt, t_dst . double_value, t_src . double_value, t_result . double_value);
	}
    
    MCExecTypeRelease(t_src);
    MCExecTypeRelease(t_dst);
	
	if (!ctxt . HasError())
	{
		if (destvar != nil)
		{
            bool t_success;
            
            if (destvar -> needsContainer())
                t_success = t_dst_container.give_value(ctxt, t_result);
            else
                t_success = destvar -> give_value(ctxt, t_result);
            
            if (!t_success)
                ctxt . Throw();
		}
		else
		{
            if (dest->set(ctxt, PT_INTO, t_result))
                return;

			ctxt . LegacyThrow(EE_DIVIDE_CANTSET);
		}
    }
}

void MCDivide::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	source -> compile(ctxt);

	if (destvar != nil)
		destvar -> compile_inout(ctxt);
	else
		dest -> compile_inout(ctxt);

	MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCMathExecDivideArrayByArrayMethodInfo, 0, 1, 1);
	MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCMathExecDivideArrayByNumberMethodInfo, 0, 1, 1);
	MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCMathExecDivideNumberByNumberMethodInfo, 0, 1, 1);

	MCSyntaxFactoryEndStatement(ctxt);
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
		dest = new (nothrow) MCChunk(True);
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

	return PS_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the source can be an array or number so we use 'tona'.
void MCMultiply::exec_ctxt(MCExecContext &ctxt)
{
    MCExecValue t_src;

    if(!ctxt . EvaluateExpression(source, EE_MULTIPLY_BADSOURCE, t_src)
            || !ctxt . ConvertToNumberOrArray(t_src))
	{
		ctxt.LegacyThrow(EE_MULTIPLY_BADSOURCE);
        return;
    }
	
	MCExecValue t_dst;
	MCContainer t_dst_container;
	if (destvar != nil)
	{
        bool t_success;
        if (destvar -> needsContainer())
            t_success = destvar -> evalcontainer(ctxt, t_dst_container)
                            && t_dst_container.eval_ctxt(ctxt, t_dst);
        else
        {
            destvar -> eval_ctxt(ctxt, t_dst);
            t_success = !ctxt . HasError();
        }
        
        if (!t_success)
        {
            ctxt . LegacyThrow(EE_MULTIPLY_BADDEST);
            MCExecTypeRelease(t_src);
            return;
        }
	}
	else
	{
        if (!ctxt . EvaluateExpression(dest, EE_MULTIPLY_BADDEST, t_dst))
        {
            MCExecTypeRelease(t_src);
            return;
        }
	}

    if (!ctxt . ConvertToNumberOrArray(t_dst))
    {
        MCExecTypeRelease(t_src);
		MCExecTypeRelease(t_dst);
		ctxt.LegacyThrow(EE_MULTIPLY_BADDEST);
        return;
    }
	
	MCExecValue t_result;
    t_result . type = t_dst . type;
    if (t_src . type == kMCExecValueTypeArrayRef)
	{
        if (t_dst . type == kMCExecValueTypeArrayRef)
            MCMathExecMultiplyArrayByArray(ctxt, t_dst . arrayref_value, t_src . arrayref_value, t_result . arrayref_value);
		else
		{
            ctxt . LegacyThrow(EE_MULTIPLY_MISMATCH);
            return;
		}
	}
	else
	{
        if (t_dst . type == kMCExecValueTypeArrayRef)
            MCMathExecMultiplyArrayByNumber(ctxt, t_dst . arrayref_value, t_src . double_value, t_result . arrayref_value);
		else
            MCMathExecMultiplyNumberByNumber(ctxt, t_dst . double_value, t_src . double_value, t_result . double_value);
	}
    
    MCExecTypeRelease(t_src);
    MCExecTypeRelease(t_dst);
	
	if (!ctxt . HasError())
	{
		if (destvar != nil)
		{
            bool t_success;
            
            if (destvar -> needsContainer())
                t_success = t_dst_container.give_value(ctxt, t_result);
            else
                t_success = destvar -> give_value(ctxt, t_result);
            
            if (!t_success)
                ctxt . Throw();
		}
		else
		{            
			if (dest->set(ctxt, PT_INTO, t_result))
                return;

			ctxt . LegacyThrow(EE_MULTIPLY_CANTSET);
		}
    }
}

void MCMultiply::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	source -> compile(ctxt);

	if (destvar != nil)
		destvar -> compile_inout(ctxt);
	else
		dest -> compile_inout(ctxt);

	MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCMathExecMultiplyArrayByArrayMethodInfo, 0, 1, 1);
	MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCMathExecMultiplyArrayByNumberMethodInfo, 0, 1, 1);
	MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCMathExecMultiplyNumberByNumberMethodInfo, 0, 1, 1);

	MCSyntaxFactoryEndStatement(ctxt);
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
		dest = new (nothrow) MCChunk(True);
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

	return PS_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the source can be an array or number so we use 'tona'.
void MCSubtract::exec_ctxt(MCExecContext &ctxt)
{
	MCExecValue t_src;

    if (!ctxt . EvaluateExpression(source, EE_SUBTRACT_BADSOURCE, t_src)
            || !ctxt . ConvertToNumberOrArray(t_src))
	{
		ctxt.LegacyThrow(EE_SUBTRACT_BADSOURCE);
        return;
    }
	
	MCExecValue t_dst;
	MCContainer t_dst_container;
	if (destvar != nil)
	{
        bool t_success;
        if (destvar -> needsContainer())
            t_success = destvar -> evalcontainer(ctxt, t_dst_container)
                            && t_dst_container.eval_ctxt(ctxt, t_dst);
        else
        {
            destvar -> eval_ctxt(ctxt, t_dst);
            t_success = !ctxt . HasError();
        }
        
        if (!t_success)
        {
            ctxt . LegacyThrow(EE_SUBTRACT_BADDEST);
            MCExecTypeRelease(t_src);
            return;
        }
	}
	else
	{
        if (!ctxt . EvaluateExpression(dest, EE_SUBTRACT_BADDEST, t_dst))
        {
            MCExecTypeRelease(t_src);
            return;
        }
	}

    if (!ctxt . ConvertToNumberOrArray(t_dst))
    {
        MCExecTypeRelease(t_src);
		MCExecTypeRelease(t_dst);
		ctxt.LegacyThrow(EE_SUBTRACT_BADDEST);
        return;
    }
	
	MCExecValue t_result;
    t_result . type = t_dst . type;
    if (t_src . type == kMCExecValueTypeArrayRef)
	{
        if (t_dst . type == kMCExecValueTypeArrayRef)
            MCMathExecSubtractArrayFromArray(ctxt, t_src . arrayref_value, t_dst . arrayref_value, t_result . arrayref_value);
		else
		{
            ctxt . LegacyThrow(EE_SUBTRACT_MISMATCH);
            return;
		}
	}
	else
	{
        if (t_dst . type == kMCExecValueTypeArrayRef)
            MCMathExecSubtractNumberFromArray(ctxt, t_src . double_value, t_dst . arrayref_value, t_result . arrayref_value);
		else
            MCMathExecSubtractNumberFromNumber(ctxt, t_src . double_value, t_dst . double_value, t_result . double_value);
	}
    
    MCExecTypeRelease(t_src);
    MCExecTypeRelease(t_dst);
	
	if (!ctxt . HasError())
	{
		if (destvar != nil)
		{
            bool t_success;
            
            if (destvar -> needsContainer())
                t_success =  t_dst_container.give_value(ctxt, t_result);
            else
                t_success = destvar -> give_value(ctxt, t_result);
            
            if (!t_success)
                ctxt . Throw();
		}
		else
		{
			if (dest->set(ctxt, PT_INTO, t_result))
                return;

			ctxt . LegacyThrow(EE_SUBTRACT_CANTSET);
		}
    }
}

void MCSubtract::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	source -> compile(ctxt);

	if (destvar != nil)
		destvar -> compile_inout(ctxt);
	else
		dest -> compile_inout(ctxt);

	MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCMathExecSubtractArrayFromArrayMethodInfo, 0, 1, 1);
	MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCMathExecSubtractNumberFromArrayMethodInfo, 0, 1, 1);
	MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCMathExecSubtractNumberFromNumberMethodInfo, 0, 1, 1);

	MCSyntaxFactoryEndStatement(ctxt);
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
		  (sp . token_is_cstring("column") ||
		   sp . token_is_cstring("row")))
	{
		if (sp . token_is_cstring("column"))
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

void MCArrayOp::exec_ctxt(MCExecContext &ctxt)
{
	MCAutoStringRef t_element_del;
	MCAutoStringRef t_key_del;
	uint4 chunk;
	chunk = mode;
	switch(chunk)
	{
		case TYPE_USER:
			if (element != NULL)
			{
                if (!ctxt . EvalExprAsStringRef(element, EE_ARRAYOP_BADEXP, &t_element_del))
                    return;

                if (!ctxt . EvalOptionalExprAsNullableStringRef(key, EE_ARRAYOP_BADEXP, &t_key_del))
                    return;
			}
		break;
        case TYPE_ROW:
            t_element_del = ctxt.GetRowDelimiter();
            break;
        case TYPE_COLUMN:
            t_element_del = ctxt.GetColumnDelimiter();
            break;
        case TYPE_LINE:
            t_element_del = ctxt.GetLineDelimiter();
            break;
        case TYPE_ITEM:
            t_element_del = ctxt.GetItemDelimiter();
            break;
        case TYPE_WORD:
        case TYPE_TOKEN:
        case TYPE_CHARACTER:
		default:
            ctxt . Throw();
            return;
	}

	MCContainer t_container;
    MCAutoValueRef t_container_value;
    if (!destvar -> evalcontainer(ctxt, t_container))
	{
        ctxt . LegacyThrow(EE_ARRAYOP_BADEXP);
        return;
    }

    if (!t_container.eval(ctxt, &t_container_value))
    {
        ctxt . Throw();
        return;
    }

	if (is_combine)
	{
        MCAutoArrayRef t_array;
        if (!ctxt . ConvertToArray(*t_container_value, &t_array))
            return;

		MCAutoStringRef t_string;
		if (form == FORM_NONE)
		{
            // SN-2014-09-01: [[ Bug 13297 ]] Combining by column deserves its own function as it is too
            // different from combining by row
            if (chunk == TYPE_ROW)
                MCArraysExecCombineByRow(ctxt, *t_array, &t_string);
            else if (chunk == TYPE_COLUMN)
                MCArraysExecCombineByColumn(ctxt, *t_array, &t_string);
            else
				MCArraysExecCombine(ctxt, *t_array, *t_element_del, *t_key_del, &t_string);
		}
		else if (form == FORM_SET)
			MCArraysExecCombineAsSet(ctxt, *t_array, *t_element_del, &t_string);

        if (!ctxt . HasError())
            t_container.set(ctxt, *t_string);
	}
	else
	{
		MCAutoStringRef t_string;
        if (!ctxt . ConvertToString(*t_container_value, &t_string))
            return;

		MCAutoArrayRef t_array;
		if (form == FORM_NONE)
		{
			if (chunk == TYPE_COLUMN)
				MCArraysExecSplitByColumn(ctxt, *t_string, &t_array);
			else
				MCArraysExecSplit(ctxt, *t_string, *t_element_del, *t_key_del, &t_array);
		}
		else if (form == FORM_SET)
			MCArraysExecSplitAsSet(ctxt, *t_string, *t_element_del, &t_array);

		if (!ctxt . HasError())
            t_container.set(ctxt, *t_array);
    }
}

void MCArrayOp::compile(MCSyntaxFactoryRef ctxt)
{	
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	destvar -> compile_inout(ctxt);

	if (mode == TYPE_USER && element != nil)
		element -> compile(ctxt);
	else
		MCSyntaxFactoryEvalConstantNil(ctxt);

	if (mode == TYPE_USER && key != nil)
		key -> compile(ctxt);
	else
		MCSyntaxFactoryEvalConstantNil(ctxt);

	if (is_combine)
	{
		if (form == FORM_NONE)
		{
            // SN-2014-09-01: [[ Bug 13297 ]] Combining by column deserves its own function as it is too
            // different from combining by row
            if (mode == TYPE_ROW)
                MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCArraysExecCombineByRowMethodInfo, 0, 0);
            else if (mode == TYPE_COLUMN)
                MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCArraysExecCombineByColumnMethodInfo, 0, 0);
			else
				MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCArraysExecCombineMethodInfo, 0, 1, 2, 0);
		}
		else if (form == FORM_SET)
			MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCArraysExecCombineAsSetMethodInfo, 0, 1, 0);
	}
	else
	{
		if (form == FORM_NONE)
		{
			if (mode == TYPE_COLUMN)
				MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCArraysExecSplitByColumnMethodInfo, 0, 0);
			else
				MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCArraysExecSplitMethodInfo, 0, 1, 2, 0);
		}
		else if (form == FORM_SET)
			MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCArraysExecSplitAsSetMethodInfo, 0, 1, 0);
	}

	MCSyntaxFactoryEndStatement(ctxt);
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

	return PS_NORMAL;
}

void MCSetOp::exec_ctxt(MCExecContext &ctxt)
{
	// ARRAYEVAL
    MCAutoValueRef t_src;
    if (!ctxt . EvalExprAsValueRef(source, EE_ARRAYOP_BADEXP, &t_src))
        return;
    
	MCContainer t_container;
    if (!destvar -> evalcontainer(ctxt, t_container))
	{
        ctxt . LegacyThrow(EE_ARRAYOP_BADEXP);
        return;
	}

    MCAutoValueRef t_dst;
    if (!t_container.eval(ctxt, &t_dst))
        return;

    MCAutoValueRef t_dst_value;
	if (intersect)
    {
        // MERG-2013-08-26: [[ RecursiveArrayOp ]] Support nested arrays in union and intersect
        if (recursive)
            MCArraysExecIntersectRecursive(ctxt, *t_dst, *t_src, &t_dst_value);
        else
            MCArraysExecIntersect(ctxt, *t_dst, *t_src, &t_dst_value);
    }
	else
    {
        // MERG-2013-08-26: [[ RecursiveArrayOp ]] Support nested arrays in union and intersect
        if (recursive)
            MCArraysExecUnionRecursive(ctxt, *t_dst, *t_src, &t_dst_value);
        else
            MCArraysExecUnion(ctxt, *t_dst, *t_src, &t_dst_value);
    }

	if (!ctxt . HasError())
        t_container.set(ctxt, *t_dst_value);
}

void MCSetOp::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	// MUTABLE ARRAY 
	destvar -> compile(ctxt);
	source -> compile(ctxt);

    if (intersect)
    {
        if (recursive)
            MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCArraysExecIntersectRecursiveMethodInfo, 0, 1, 0);
        else
            MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCArraysExecIntersectMethodInfo, 0, 1, 0);
    }
    else
    {
        if (recursive)
            MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCArraysExecUnionRecursiveMethodInfo, 0, 1, 0);
        else
            MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCArraysExecUnionMethodInfo, 0, 1, 0);
    }

	MCSyntaxFactoryEndStatement(ctxt);
}
