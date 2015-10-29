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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"
#include "scriptpt.h"
//#include "execpt.h"
#include "mcerror.h"
#include "param.h"
#include "object.h"
#include "debug.h"
#include "util.h"
#include "handler.h"
#include "parentscript.h"
#include "osspec.h"

#include "globals.h"

#include "syntax.h"
#include "statemnt.h"

////////////////////////////////////////////////////////////////////////////////

MCExpression::MCExpression()
{
	rank = FR_VALUE;
	line = pos = 0;
	root = NULL;
	left = NULL;
	right = NULL;
}

MCExpression::~MCExpression()
{
	delete left;
	delete right;
}

MCVarref *MCExpression::getrootvarref(void)
{
	return NULL;
}

#ifdef LEGACY_EXEC
MCVariable *MCExpression::evalvar(MCExecPoint& ep)
{
	return NULL;
}
#endif

MCVariable *MCExpression::evalvar(MCExecContext& ctxt)
{
    return NULL;
}

#ifdef LEGACY_EXEC
Exec_stat MCExpression::evalcontainer(MCExecPoint& ep, MCContainer*& r_container)
{
	return ES_ERROR;
}
#endif

bool MCExpression::evalcontainer(MCExecContext& ctxt, MCContainer*& r_container)
{
    return false;
}

Parse_stat MCExpression::getexps(MCScriptPoint &sp, MCExpression *earray[],
                                 uint2 &ecount)
{
	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_LPAREN) != PS_NORMAL)
	{
		MCperror->add
		(PE_FACTOR_NOLPAREN, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_RPAREN) != PS_NORMAL)
	{
		while (True)
		{
			MCExpression *newexp = NULL;
			if (sp.parseexp(False, False, &newexp) != PS_NORMAL)
			{
				delete newexp;
				MCperror->add
				(PE_FACTOR_BADPARAM, sp);
				return PS_ERROR;
			}
			earray[ecount++] = newexp;
			if (sp.skip_token(SP_FACTOR, TT_RPAREN) == PS_NORMAL)
				break;
			Symbol_type type;
			if (ecount >= MAX_EXP || sp.next(type) != PS_NORMAL)
			{
				MCperror->add
				(PE_FACTOR_NORPAREN, sp);
				return PS_ERROR;
			}
			if (type != ST_SEP)
			{
				MCperror->add
				(PE_FACTOR_NOTSEP, sp);
				return PS_ERROR;
			}
		}
	}
	return PS_NORMAL;
}

void MCExpression::freeexps(MCExpression *earray[], uint2 ecount)
{
	while (ecount--)
		delete earray[ecount];
}

Parse_stat MCExpression::get0params(MCScriptPoint &sp)
{
	MCExpression *earray[MAX_EXP];
	uint2 ecount = 0;
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount != 0)
	{
		freeexps(earray, ecount);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Parse_stat MCExpression::get0or1param(MCScriptPoint &sp, MCExpression **exp,
                                      Boolean the)
{
	if (the)
	{
		initpoint(sp);
		if (sp.skip_token(SP_FACTOR, TT_OF) != PS_NORMAL)
			return PS_NORMAL;
		if (sp.parseexp(False, False, exp) != PS_NORMAL)
		{
			MCperror->add
			(PE_FACTOR_BADPARAM, sp);
			return PS_ERROR;
		}
	}
	else
	{
		MCExpression *earray[MAX_EXP];
		uint2 ecount = 0;
		if (getexps(sp, earray, ecount) != PS_NORMAL || ecount > 1)
		{
			freeexps(earray, ecount);
			return PS_ERROR;
		}
		else
			if (ecount == 1)
				*exp = earray[0];
	}
	return PS_NORMAL;
}

Parse_stat MCExpression::get1param(MCScriptPoint &sp, MCExpression **exp,
                                   Boolean the)
{
	if (the)
	{
		initpoint(sp);
		if (sp.skip_token(SP_FACTOR, TT_OF) != PS_NORMAL)
		{
			MCperror->add
			(PE_FACTOR_NOOF, sp);
			return PS_ERROR;
		}
		if (sp.parseexp(True, False, exp) != PS_NORMAL)
		{
			MCperror->add
			(PE_FACTOR_BADPARAM, sp);
			return PS_ERROR;
		}
	}
	else
	{
		MCExpression *earray[MAX_EXP];
		uint2 ecount = 0;
		if (getexps(sp, earray, ecount) != PS_NORMAL || ecount != 1)
		{
			freeexps(earray, ecount);
			return PS_ERROR;
		}
		else
			*exp = earray[0];
	}
	return PS_NORMAL;
}

Parse_stat MCExpression::get1or2params(MCScriptPoint &sp, MCExpression **exp1,
                                       MCExpression **exp2, Boolean the)
{
	if (the)
	{
		initpoint(sp);
		if (sp.skip_token(SP_FACTOR, TT_OF) != PS_NORMAL)
		{
			MCperror->add
			(PE_FACTOR_NOOF, sp);
			return PS_ERROR;
		}
		if (sp.parseexp(True, False, exp1) != PS_NORMAL)
		{
			MCperror->add
			(PE_FACTOR_BADPARAM, sp);
			return PS_ERROR;
		}
	}
	else
	{
		MCExpression *earray[MAX_EXP];
		uint2 ecount = 0;
		if (getexps(sp, earray, ecount) != PS_NORMAL || ecount < 1 || ecount > 2)
		{
			freeexps(earray, ecount);
			return PS_ERROR;
		}
		*exp1 = earray[0];
		if (ecount == 2)
			*exp2 = earray[1];
	}
	return PS_NORMAL;
}

Parse_stat MCExpression::get2params(MCScriptPoint &sp, MCExpression **exp1,
                                    MCExpression **exp2)
{
	MCExpression *earray[MAX_EXP];
	uint2 ecount = 0;
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount != 2)
	{
		freeexps(earray, ecount);
		return PS_ERROR;
	}
	*exp1 = earray[0];
	*exp2 = earray[1];
	return PS_NORMAL;
}

Parse_stat MCExpression::get2or3params(MCScriptPoint &sp, MCExpression **exp1,
                                       MCExpression **exp2, MCExpression **exp3)
{
	MCExpression *earray[MAX_EXP];
	uint2 ecount = 0;
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount < 2 || ecount > 3)
	{
		freeexps(earray, ecount);
		return PS_ERROR;
	}
	*exp1 = earray[0];
	*exp2 = earray[1];
	if (ecount == 3)
		*exp3 = earray[2];
	return PS_NORMAL;
}

Parse_stat MCExpression::get3params(MCScriptPoint &sp, MCExpression **exp1,
                                    MCExpression **exp2, MCExpression **exp3)
{
	MCExpression *earray[MAX_EXP];
	uint2 ecount = 0;
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount != 3)
	{
		freeexps(earray, ecount);
		return PS_ERROR;
	}
	*exp1 = earray[0];
	*exp2 = earray[1];
	*exp3 = earray[2];
	return PS_NORMAL;
}

Parse_stat MCExpression::get4or5params(MCScriptPoint &sp, MCExpression **exp1,
                                       MCExpression **exp2, MCExpression **exp3,
                                       MCExpression **exp4, MCExpression **exp5)
{
	MCExpression *earray[MAX_EXP];
	uint2 ecount = 0;
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount < 4 || ecount > 5)
	{
		freeexps(earray, ecount);
		return PS_ERROR;
	}
	*exp1 = earray[0];
	*exp2 = earray[1];
	*exp3 = earray[2];
	*exp4 = earray[3];
	if (ecount == 5)
		*exp5 = earray[4];
	return PS_NORMAL;
}

Parse_stat MCExpression::get6params(MCScriptPoint &sp, MCExpression **exp1,
                                    MCExpression **exp2, MCExpression **exp3,
                                    MCExpression **exp4, MCExpression **exp5,
                                    MCExpression **exp6)
{
	MCExpression *earray[MAX_EXP];
	uint2 ecount = 0;
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount != 6)
	{
		freeexps(earray, ecount);
		return PS_ERROR;
	}
	*exp1 = earray[0];
	*exp2 = earray[1];
	*exp3 = earray[2];
	*exp4 = earray[3];
	*exp5 = earray[4];
	*exp6 = earray[5];
	return PS_NORMAL;
}

Parse_stat MCExpression::getvariableparams(MCScriptPoint &sp, uint32_t p_min_params, uint32_t p_param_count, ...)
{
	Parse_stat t_stat = PS_NORMAL;
	
	MCExpression *earray[MAX_EXP];
	uint16_t ecount = 0;
	
	va_list t_params;
	va_start(t_params, p_param_count);
	
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount < p_min_params || ecount > p_param_count)
	{
		freeexps(earray, ecount);
		t_stat = PS_ERROR;
	}
	else
	{
		for (uint32_t i = 0; i < p_param_count; i++)
			*(va_arg(t_params, MCExpression **)) = (i < ecount) ? earray[i] : NULL;
	}

	va_end(t_params);
	
	return t_stat;
}

Parse_stat MCExpression::getparams(MCScriptPoint &sp, MCParameter **params)
{
	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_LPAREN) != PS_NORMAL)
	{
		MCperror->add
		(PE_FACTOR_NOLPAREN, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_RPAREN) == PS_NORMAL)
		return PS_NORMAL;
	MCParameter *pptr = NULL;
	while (True)
	{
		if (pptr == NULL)
			*params = pptr = new MCParameter;
		else
		{
			pptr->setnext(new MCParameter);
			pptr = pptr->getnext();
		}
		if (pptr->parse(sp) != PS_NORMAL)
		{
			MCperror->add
			(PE_FACTOR_BADPARAM, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_FACTOR, TT_RPAREN) == PS_NORMAL)
			break;
		Symbol_type type;
		if (sp.next(type) != PS_NORMAL)
		{
			MCperror->add
			(PE_FACTOR_NORPAREN, sp);
			return PS_ERROR;
		}
		if (type != ST_SEP)
		{
			MCperror->add
			(PE_FACTOR_NOTSEP, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_FACTOR, TT_RPAREN) == PS_NORMAL)
		{
			pptr->setnext(new MCParameter);
			break;
		}
	}
	return PS_NORMAL;
}

/* struct compare_arrays_t
{
	MCArrayRef other_array;
	bool case_sensitive;
	MCExecPoint *ep;
	compare_t result;
};

bool MCExpression::compare_array_element(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
	compare_arrays_t *ctxt;
	ctxt = (compare_arrays_t *)p_context;

	MCValueRef t_other_value;
	if (MCArrayFetchValue(ctxt -> other_array, ctxt -> case_sensitive, p_key, t_other_value))
	{
		MCExecPoint ep1(*(ctxt -> ep)), ep2(*(ctxt -> ep));
		ep1 . setvalueref(p_value);
		ep2 . setvalueref(t_other_value);
		ctxt -> result = MCExpression::compare_values(ep1, ep2, ctxt -> ep, true);
		if (ctxt -> result == 0)
			return true;
	}
	else
		ctxt -> result = MAXUINT2;

	return false;
}

// OK-2009-02-17: [[Bug 7693]] - This function implements array comparison.
// The return value is 0 if the arrays are equal, non-zero otherwise. For now this is left as an int2
// for more easy compatibility with compare_values.
int2 MCExpression::compare_arrays(MCExecPoint &ep1, MCExecPoint &ep2, MCExecPoint *p_context)
{
	MCArrayRef t_array1, t_array2;
	t_array1 = ep1 . getarrayref();
	t_array2 = ep2 . getarrayref();

	if (MCArrayGetCount(t_array1) > MCArrayGetCount(t_array2))
		return 1;

	if (MCArrayGetCount(t_array1) < MCArrayGetCount(t_array2))
		return -1;
	
	compare_arrays_t t_ctxt;
	t_ctxt . other_array = t_array2;
	t_ctxt . case_sensitive = p_context -> getcasesensitive() == True;
	t_ctxt . ep = p_context;
	t_ctxt . result = 0;
	MCArrayApply(t_array1, compare_array_element, &t_ctxt);

	return t_ctxt . result;
}

int2 MCExpression::compare_values(MCExecPoint &ep1, MCExecPoint &ep2, MCExecPoint *p_context, bool p_compare_arrays)
{
	int2 i;
	// OK-2009-02-17: [[Bug 7693]] - Allow arrays to be compared providing that
	// both arguments are arrays.
	// MW-2012-12-11: [[ ArrayComp ]] If the format of the ep is array then it
	//   must be non-empty (vars self-normalize). Thus if both are arrays then
	//   compare as arrays; otherwise if either is an array they become empty
	//   in string context and as such are less (or more) than whatever the
	//   otherside has.
	if (p_compare_arrays)
	{
		bool t_ep1_array, t_ep2_array;
		t_ep1_array = ep1 . isarray()
		t_ep2_array = ep2 . isarray();
		if (t_ep1_array && t_ep2_array)
			return compare_arrays(ep1, ep2, p_context);
		if (t_ep1_array)
			return -1;
		if (t_ep2_array)
			return 1;
	}

	Boolean n1 = True;
	Boolean n2 = True;
	if (ep1.isstring())
	{
		n1 = False;
		const char *sptr = ep1.getsvalue().getstring();
		uint4 l = ep1.getsvalue().getlength();
		MCU_skip_spaces(sptr, l);
		if (l)
		{
			char c = *sptr;
			if (isdigit((uint1)c) || c == '.' || c == '-' || c == '+')
			{
				MCS_seterrno(0);
				n1 = ep1.ton() == ES_NORMAL && MCS_geterrno() != ERANGE;
			}
		}
	}
	if (n1)
		if (ep2.isstring())
		{
			n2 = False;
			const char *sptr = ep2.getsvalue().getstring();
			uint4 l = ep2.getsvalue().getlength();
			MCU_skip_spaces(sptr, l);
			if (l)
			{
				char c = *sptr;
				if (isdigit((uint1)c) || c == '.' || c == '-' || c == '+')
				{
					MCS_seterrno(0);
					n2 = ep2.ton() == ES_NORMAL && MCS_geterrno() != ERANGE;
				}
			}
		}
	if (n1 && n2)
	{
		i = 1;
		real8 d1 = fabs(ep1.getnvalue());
		real8 d2 = fabs(ep2.getnvalue());
		real8 min = d1 < d2 ? d1 : d2;
		if (min < MC_EPSILON)
		{
			if (fabs(ep1.getnvalue() - ep2.getnvalue()) < MC_EPSILON)
				i = 0;
		}
		else
			if (fabs(ep1.getnvalue() - ep2.getnvalue()) / min < MC_EPSILON)
				i = 0;
		if (i)
			if (ep1.getnvalue() < ep2.getnvalue())
				i = -1;
	}
	else
	{
		uint4 l1 = ep1.getsvalue().getlength();
		uint4 l2 = ep2.getsvalue().getlength();
		if (ep1.getcasesensitive())
			i = memcmp(ep1.getsvalue().getstring(),
			           ep2.getsvalue().getstring(), MCU_min(l1, l2));
		else
			i = MCU_strncasecmp(ep1.getsvalue().getstring(),
			                    ep2.getsvalue().getstring(), MCU_min(l1, l2));
		if (i == 0 && l1 != l2)
			i = l1 > l2 ? 1 : -1;
	}
	return i;
}

Exec_stat MCExpression::compare(MCExecPoint &ep1, int2 &i, bool p_compare_arrays)
{
	MCExecPoint ep2(ep1);
	MCExecPoint t_original_ep(ep1);

	if (left->eval(ep1) != ES_NORMAL)
	{
		MCeerror->add
		(EE_FACTOR_BADLEFT, line, pos);
		return ES_ERROR;
	}

	if (right->eval(ep2) != ES_NORMAL)
	{
		MCeerror->add
		(EE_FACTOR_BADRIGHT, line, pos);
		return ES_ERROR;
	}

	// OK-2009-02-17: [[Bug 7693]] - Moved to enable array comparison. Using the p_compare_arrays allows us to ensure
	// that only MCEquals and MCIs are allowed to compare arrays for now, but we can simply remove this if we wish to also
	// allow things like MCLessThan to compare arrays. However to do this we would need to deal with the situation where
	// it is hard to determine which array is bigger, e.g. where they have the same number of elements, but different keys.
	i = compare_values(ep1, ep2, &t_original_ep, p_compare_arrays);
	
	return ES_NORMAL;
}*/

Parse_stat MCExpression::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	return PS_NORMAL;
}

#ifdef LEGACY_EXEC
Exec_stat MCExpression::eval(MCExecPoint &ep)
{
    MCAssert(false);
    MCExecContext ctxt(ep . GetEC());
	
	MCAutoValueRef t_value;
	eval_valueref(ctxt, &t_value);
	if (!ctxt . HasError())
    {
        ep . setvalueref(*t_value);
		return ES_NORMAL;
    }
	
	return ctxt . Catch(line, pos);
}
#endif

void MCExpression::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    fprintf(stderr, "ERROR: eval method for expression not implemented properly\n");
    abort();
}

void MCExpression::eval_typed(MCExecContext& ctxt, MCExecValueType p_type, void *r_value)
{
	MCExecValue t_value;
	eval_ctxt(ctxt, t_value);
	if (!ctxt . HasError())
		MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , p_type, r_value);
}


void MCExpression::initpoint(MCScriptPoint &sp)
{
	line = sp.getline();
	pos = sp.getpos();
}

void MCExpression::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

void MCExpression::compile_out(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

////////////////////////////////////////////////////////////////////////////////

MCFuncref::MCFuncref(MCNameRef inname)
{
	/* UNCHECKED */ MCNameClone(inname, name);
	handler = nil;
	params = NULL;
	resolved = false;
    global_handler = false;
}

MCFuncref::~MCFuncref()
{
	while (params != NULL)
	{
		MCParameter *tmp = params;
		params = params->getnext();
		delete tmp;
	}
	MCNameDelete(name);
}

Parse_stat MCFuncref::parse(MCScriptPoint &sp, Boolean the)
{
	parent = sp.getobj();
	initpoint(sp);
	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_FUNCTION_BADPARAMS, sp);
		return PS_ERROR;
	}
    
    if (MCIsGlobalHandler(name))
    {
        global_handler = true;
        resolved = true;
    }
    
	return PS_NORMAL;
}

#if /* MCFuncref::eval */ LEGACY_EXEC
Exec_stat MCFuncref::eval(MCExecPoint &ep)
{
	MCExecContext ctxt(ep);
	if (MCscreen->abortkey())
	{
		MCeerror->add(EE_HANDLER_ABORT, line, pos);
		return ES_ERROR;
	}

	if (!resolved)
	{
		// MW-2008-01-28: [[ Inherited parentScripts ]]
		// If we are in parentScript context, then the object we search for
		// private handlers in is the parentScript's object, rather than the
		// ep's.
		MCParentScriptUse *t_parentscript;
		t_parentscript = ep . getparentscript();

		MCObject *t_object;
		if (t_parentscript == NULL)
			t_object = ep . getobj();
		else
			t_object = t_parentscript -> GetParent() -> GetObject();

		// MW-2008-10-28: [[ ParentScripts ]] Private handlers are resolved
		//   relative to the object containing the handler we are executing.
		MCHandler *t_resolved_handler;
		t_resolved_handler = t_object -> findhandler(HT_FUNCTION, name);
		if (t_resolved_handler != NULL && t_resolved_handler -> isprivate())
			handler = t_resolved_handler;

		resolved = true;
    }

	// Go through all the parameters to the function, if they are not variables, clear their current value. Each parameter stores an expression
	// which allows its value to be re-evaluated in a given context. Re-evaluate each in the context of ep and set it to the new value.
	// As the ep should contain the context of the caller at this point, the expression should be evaluated in that context.
	MCParameter *tptr = params;
	MCexitall = False;
	while (tptr != NULL)
	{
		MCVariable* t_var;
		t_var = tptr -> evalvar(ep);

		if (t_var == NULL)
		{
			tptr -> clear_argument();
			Exec_stat stat;
			MCExecContext ctxt(ep);
			while ((stat = tptr->eval(ep)) != ES_NORMAL && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
				MCB_error(ctxt, line, pos, EE_FUNCTION_BADSOURCE);
			if (stat != ES_NORMAL)
			{
				MCeerror->add(EE_FUNCTION_BADSOURCE, line, pos);
				return ES_ERROR;
			}
			tptr->set_argument(ep);
		}
		else
			tptr->set_argument_var(t_var);

		tptr = tptr->getnext();
	}

	// MW-2008-12-17: [[ Bug 7463 ]] Make sure we use the object from the execpoint, rather
	//   than the 'parent' field in this.
	MCObject *p = ep.getobj();
	MCExecContext *oldctxt = MCECptr;
	MCECptr = &ctxt;
	Exec_stat stat = ES_NOT_HANDLED;
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ep.setline(line);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
		added = True;
	}
    
#ifdef _MOBILE
    if (platform_message)
    {
        stat = MCHandlePlatformMessage(name, params);
    }
#endif
    
	if (handler != nil)
	{   
        // MW-2008-10-28: [[ ParentScripts ]] If we are in the context of a
        //   parent, then use a special method.
        // MW-2009-01-28: [[ Inherited parentScripts ]]
        // If we are in parentScript context, then pass the parentScript in use to execparenthandler.
        if (ep . getparentscript() == NULL)
            stat = parent -> exechandler(handler, params);
        else
            stat = ep.getobj() -> execparenthandler(handler, params, ep . getparentscript());
        
        switch(stat)
        {
            case ES_ERROR:
            case ES_PASS:
                MCeerror->add(EE_FUNCTION_BADFUNCTION, line, pos, handler -> getname());
                if (MCerrorptr == NULL)
                    MCerrorptr = parent;
                stat = ES_ERROR;
                break;
                
            case ES_EXIT_HANDLER:
                stat = ES_NORMAL;
                break;
                
            default:
                break;
        }
        
		MCECptr = oldctxt;
		if (added)
			MCnexecutioncontexts--;
	}
	else
	{
		stat = MCU_dofrontscripts(HT_FUNCTION, name, params);
		Boolean olddynamic = MCdynamicpath;
		MCdynamicpath = MCdynamiccard != NULL;
		if (stat == ES_PASS || stat == ES_NOT_HANDLED)
		{
			// PASS STATE FIX
			Exec_stat oldstat = stat;
			stat = p->handle(HT_FUNCTION, name, params, p);
			if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
				stat = ES_PASS;
		}
		MCECptr = oldctxt;
		MCdynamicpath = olddynamic;
		if (added)
			MCnexecutioncontexts--;
	}
     
	// MW-2007-08-09: [[ Bug 5705 ]] Throws inside private functions don't trigger an
	//   exception.
	if (stat != ES_NORMAL && stat != ES_PASS && stat != ES_EXIT_HANDLER)
	{
		MCeerror->add(EE_FUNCTION_BADFUNCTION, line, pos, name);
		return ES_ERROR;
	}

	MCresult->eval(ep);
    
	return ES_NORMAL;
}
#endif

void MCFuncref::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    MCKeywordsExecCommandOrFunction(ctxt, resolved, handler, params, name, line, pos, global_handler, true);
    
    Exec_stat stat = ctxt . GetExecStat();
    
   	// MW-2007-08-09: [[ Bug 5705 ]] Throws inside private functions don't trigger an
	//   exception.
	if (stat != ES_NORMAL && stat != ES_PASS && stat != ES_EXIT_HANDLER)
	{
		ctxt . LegacyThrow(EE_FUNCTION_BADFUNCTION, name);
		return;
	}

	if (MCresult->eval(ctxt, r_value . valueref_value))
	{
        r_value . type = kMCExecValueTypeValueRef;
		return;
    }
    
    ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////
