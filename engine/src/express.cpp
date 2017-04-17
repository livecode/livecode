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
#include "execpt.h"
#include "mcerror.h"
#include "param.h"
#include "object.h"
#include "debug.h"
#include "util.h"
#include "handler.h"
#include "parentscript.h"
#include "osspec.h"

#include "globals.h"
#include "stack.h"
#include "card.h"

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

MCVariable *MCExpression::evalvar(MCExecPoint& ep)
{
	return NULL;
}

Exec_stat MCExpression::evalcontainer(MCExecPoint& ep, MCVariable*& r_var, MCVariableValue*& r_ref)
{
	return ES_ERROR;
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

// OK-2009-02-17: [[Bug 7693]] - This function implements array comparison.
// The return value is 0 if the arrays are equal, non-zero otherwise. For now this is left as an int2
// for more easy compatibility with compare_values.
int2 MCExpression::compare_arrays(MCExecPoint &ep1, MCExecPoint &ep2, MCExecPoint *p_context)
{
	MCVariableArray *t_array1, *t_array2;
	t_array1 = ep1 . getarray() -> get_array();
	t_array2 = ep2 . getarray() -> get_array();

	// If one of the arrays has more keys it is bigger, otherwise we don't know yet.
	if (t_array1 -> getnfilled() > t_array2 -> getnfilled())
		return 1;
	else if (t_array2 -> getnfilled() > t_array1 -> getnfilled())
		return -1;
		
	// From here onwards, we know that the arrays have the same number of elements, so begin
	// iterating through to compare them.
	uint4 t_index1, t_index2;
	t_index1 = 0;
	t_index2 = 0;

	MCHashentry *t_entry1, *t_entry2;
	t_entry1 = NULL;
	t_entry2 = NULL;
	
	int2 t_result;
	t_result = 0;
	do
	{
		t_entry1 = t_array1 -> getnextkey(t_index1, t_entry1);

		// As we know the two arrays have the same number of elements, if this number of elements happens to be zero, we
		// can straight away say they are equal, without bothering to lookup the corresponding element in the other array.
		if (t_entry1 == NULL)
			return 0;

		t_entry2 = t_array2 -> lookuphash(t_entry1 -> string, p_context -> getcasesensitive(), false);

		// If the corresponding element in the second array cannot be found, this means we have been asked to compare two
		// arrays with the same number of elements, but different keys. If this is the case, we cannot compare them beyond
		// saying that they are not equal (however this is all we need for now anyway).
		if (t_entry2 == NULL)
			return MAXINT2;

		// Given that the two key names are the same, we need to compare the values.
		// This is done simply by making a recursive call to compare_values. We create
		// new exec points from p_context to ensure nothing gets messed up.
		MCExecPoint t_ep1(*p_context);
		MCExecPoint t_ep2(*p_context);
		t_entry1 -> value . fetch(t_ep1, false);
		t_entry2 -> value . fetch(t_ep2, false);
		t_result = compare_values(t_ep1, t_ep2, p_context, true);
		if (t_result != 0)
			return t_result;

	} while (t_entry1 != NULL);
	
	return t_result;
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
		t_ep1_array = ep1 . getformat() == VF_ARRAY;
		t_ep2_array = ep2 . getformat() == VF_ARRAY;
		if (t_ep1_array && t_ep2_array)
			return compare_arrays(ep1, ep2, p_context);
		if (t_ep1_array)
			return -1;
		if (t_ep2_array)
			return 1;
	}

	Boolean n1 = True;
	Boolean n2 = True;
	if (ep1.getformat() == VF_STRING)
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
		if (ep2.getformat() == VF_STRING)
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
}

Parse_stat MCExpression::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	return PS_NORMAL;
}

Exec_stat MCExpression::eval(MCExecPoint &ep)
{
	return ES_ERROR;
}

void MCExpression::initpoint(MCScriptPoint &sp)
{
	line = sp.getline();
	pos = sp.getpos();
}

MCFuncref::MCFuncref(MCNameRef inname)
{
	/* UNCHECKED */ MCNameClone(inname, name);
	handler = nil;
	params = NULL;
	resolved = false;
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
	return PS_NORMAL;
}

Exec_stat MCFuncref::eval(MCExecPoint &ep)
{
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
			while ((stat = tptr->eval(ep)) != ES_NORMAL && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
				MCB_error(ep, line, pos, EE_FUNCTION_BADSOURCE);
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
	MCExecPoint *oldep = MCEPptr;
	MCEPptr = &ep;
	Exec_stat stat = ES_NOT_HANDLED;
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ep.setline(line);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
		added = True;
	}
	
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
		MCEPptr = oldep;
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
				
			// PM-2015-07-09: [[ Bug 15253 ]] When setting property to function in msg box, search for the function handler in the mainstack/card script
			if (stat == ES_NOT_HANDLED)
			{
				MCStack * sptr = MCdefaultstackptr;
				stat = sptr -> getcard() -> handle(HT_FUNCTION, name, params, sptr -> getcard());
			}
		}
		MCEPptr = oldep;
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

	MCresult->fetch(ep);
	if (ep.getformat() == VF_STRING || ep.getformat() == VF_BOTH)
		ep.grabsvalue();
	else if (ep.getformat() == VF_ARRAY)
		ep.grabarray();

	return ES_NORMAL;
}
