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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "hndlrlst.h"
#include "handler.h"
#include "scriptpt.h"
#include "statemnt.h"
#include "object.h"
#include "chunk.h"
#include "param.h"
#include "mcerror.h"
#include "debug.h"
#include "util.h"
#include "parentscript.h"
#include "redraw.h"
#include "uidc.h"

#include "globals.h"

#ifdef _MOBILE
extern bool MCIsPlatformMessage(MCNameRef handler_name);
extern Exec_stat MCHandlePlatformMessage(MCNameRef p_message, MCParameter *p_parameters);
#endif

////////////////////////////////////////////////////////////////////////////////

static Exec_stat MCKeywordsExecuteStatements(MCExecContext& ctxt, MCStatement *p_statements, Exec_errors p_error)
{
    Exec_stat stat = ES_NORMAL;
    MCStatement *tspr = p_statements;
    while (tspr != NULL)
    {
        if (MCtrace || MCnbreakpoints)
        {
            MCB_trace(ctxt, tspr->getline(), tspr->getpos());
            if (MCexitall)
                break;
        }
        ctxt . GetEP() . setline(tspr->getline());
        
        stat = tspr->exec(ctxt . GetEP());
        
        // MW-2011-08-17: [[ Redraw ]] Flush any screen updates.
        MCRedrawUpdateScreen();
        
        switch(stat)
        {
            case ES_NORMAL:
                if (MCexitall)
                    return ES_NORMAL;
                tspr = tspr->getnext();
                break;
            case ES_NEXT_REPEAT:
                tspr = NULL;
                break;
            case ES_EXIT_REPEAT:
                return stat;
            case ES_EXIT_SWITCH:
                return ES_NORMAL;
            case ES_ERROR:
                if ((MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
                    do
                    {
                        ctxt . IgnoreLastError();
                        MCB_error(ctxt, tspr->getline(), tspr->getpos(),
                                  EE_REPEAT_BADSTATEMENT);
                    }
                while (MCtrace && (stat = tspr->exec(ctxt . GetEP())) != ES_NORMAL);
                if (stat == ES_ERROR)
                {
                    if (MCexitall)  
                        return ES_NORMAL;
                    
                    ctxt . LegacyThrow(p_error);
                    return stat;
                }
                else
                    tspr = tspr->getnext();
                break;
            default:
                return stat;
        }
    }
    return stat;
}

void MCKeywordsExecCommandOrFunction(MCExecContext& ctxt, bool resolved, MCHandler *handler, MCParameter *params, MCNameRef name, uint2 line, uint2 pos, bool platform_message, bool is_function)
{
	if (MCscreen->abortkey())
	{
		ctxt . LegacyThrow(EE_HANDLER_ABORT);
		return;
	}
    
	if (!resolved)
	{
		// MW-2008-01-28: [[ Inherited parentScripts ]]
		// If we are in parentScript context, then the object we search for
		// private handlers in is the parentScript's object, rather than the
		// ep's.
		MCParentScriptUse *t_parentscript;
		t_parentscript = ctxt . GetParentScript();
        
		MCObject *t_object;
		if (t_parentscript == NULL)
			t_object = ctxt . GetObject();
        else
            t_object = t_parentscript -> GetParent() -> GetObject();
        
        // MW-2008-10-28: [[ ParentScripts ]] Private handlers are resolved
        //   relative to the object containing the handler we are executing.
        MCHandler *t_resolved_handler;
		t_resolved_handler = t_object -> findhandler(is_function ? HT_FUNCTION : HT_MESSAGE, name);
		if (t_resolved_handler != NULL && t_resolved_handler -> isprivate())
			handler = t_resolved_handler;
        
        resolved = true;
    }
	
    if (is_function)
        MCexitall = False;
    
	// Go through all the parameters to the function, if they are not variables, clear their current value. Each parameter stores an expression
	// which allows its value to be re-evaluated in a given context. Re-evaluate each in the context of ep and set it to the new value.
	// As the ep should contain the context of the caller at this point, the expression should be evaluated in that context.
    Exec_stat stat;
	MCParameter *tptr = params;
	while (tptr != NULL)
	{
		MCVariable* t_var;
		t_var = tptr -> evalvar(ctxt);
        
		if (t_var == NULL)
		{
			tptr -> clear_argument();
            MCAutoValueRef t_value;
			if (!ctxt . TryToEvaluateParameter(tptr, line, pos, is_function ? EE_FUNCTION_BADSOURCE : EE_STATEMENT_BADPARAM, &t_value))
                return;
			tptr->setvalueref_argument(*t_value);
		}
		else
			tptr->set_argument_var(t_var);
        
        tptr = tptr->getnext();
    }
	MCObject *p = ctxt . GetObject();
	MCExecContext *oldctxt = MCECptr;
	MCECptr = &ctxt;
	stat = ES_NOT_HANDLED;
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ctxt . GetEP() . setline(line);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
		added = True;
	}
    
    if (platform_message)
    {
#ifdef _MOBILE
        extern Exec_stat MCHandlePlatformMessage(MCNameRef p_message, MCParameter *p_parameters);
        stat = MCHandlePlatformMessage(name, params);
#endif
    }
	else if (handler != nil)
	{
        // MW-2008-10-28: [[ ParentScripts ]] If we are in the context of a
        //   parent, then use a special method.
        if (ctxt . GetParentScript() == nil)
            stat = p -> exechandler(handler, params);
        else
            stat = p -> execparenthandler(handler, params, ctxt . GetParentScript());
        
        switch(stat)
        {
            case ES_ERROR:
            case ES_PASS:
                MCeerror->add(is_function ? EE_FUNCTION_BADFUNCTION : EE_STATEMENT_BADCOMMAND, line, pos, handler -> getname());
                if (MCerrorptr == NULL)
                    MCerrorptr = p;
                stat = ES_ERROR;
                break;
                
            case ES_EXIT_HANDLER:
                stat = ES_NORMAL;
                break;
                
            default:
                break;
        }
	}
	else
	{
		stat = MCU_dofrontscripts(is_function ? HT_FUNCTION : HT_MESSAGE, name, params);
		Boolean olddynamic = MCdynamicpath;
		MCdynamicpath = MCdynamiccard != NULL;
		if (stat == ES_PASS || stat == ES_NOT_HANDLED)
        {
            if (is_function)
            {
                // PASS STATE FIX
                Exec_stat oldstat = stat;
                stat = p->handle(HT_FUNCTION, name, params, p);
                if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
                    stat = ES_PASS;
            }
            else
            {
                switch (stat = p->handle(HT_MESSAGE, name, params, p))
                {
                    case ES_ERROR:
                    case ES_NOT_FOUND:
                    case ES_NOT_HANDLED:
                    case ES_PASS:
                        MCeerror->add(EE_STATEMENT_BADCOMMAND, line, pos, name);
                        stat = ES_ERROR;
                        break;
                    case ES_EXIT_HANDLER:
                        stat = ES_NORMAL;
                        break;
                    default:
                        break;
                }
            }
        }
		
		MCdynamicpath = olddynamic;
	}
	MCECptr = oldctxt;
	if (added)
		MCnexecutioncontexts--;
	ctxt . SetExecStat(stat);
}

////////////////////////////////////////////////////////////////////////////////

void MCKeywordsExecSwitch(MCExecContext& ctxt, MCExpression *condition, MCExpression **cases, uindex_t case_count, int2 default_case, uint2 *case_offsets, MCStatement *statements, uint2 line, uint2 pos)
{
    MCAutoValueRef t_value;
    MCAutoStringRef t_cond;
	if (condition != NULL)
	{
        if (!ctxt . TryToEvaluateExpression(condition, line, pos, EE_SWITCH_BADCOND, &t_value))
            return;
        
        if (!ctxt . ConvertToString(*t_value, &t_cond))
        {
            ctxt . LegacyThrow(EE_SWITCH_BADCOND);
            return;
        }
	}
    else
        t_cond = MCValueRetain(kMCTrueString);
    
	int2 match = default_case;
	uint2 i;
	for (i = 0 ; i < case_count ; i++)
	{
        MCAutoValueRef t_case;
        MCAutoStringRef t_case_string;
        
        if (!ctxt . TryToEvaluateExpression(cases[i], line, pos, EE_SWITCH_BADCASE, &t_case))
            return;
        
        if (!ctxt . ConvertToString(*t_case, &t_case_string))
        {
            ctxt . LegacyThrow(EE_SWITCH_BADCASE);
            return;
        }
        
		if (MCStringIsEqualTo(*t_cond, *t_case_string, ctxt . GetCaseSensitive() ? kMCStringOptionCompareExact : kMCStringOptionCompareCaseless))
        {
            match = case_offsets[i];
            break;
        }
	}
    
	if (match >= 0)
	{
		MCStatement *tspr = statements;
		while (match--)
			tspr = tspr->getnext();
        
        ctxt . SetExecStat(MCKeywordsExecuteStatements(ctxt, tspr, EE_SWITCH_BADSTATEMENT));
    }
}

void MCKeywordsExecIf(MCExecContext& ctxt, MCExpression *condition, MCStatement *thenstatements, MCStatement *elsestatements, uint2 line, uint2 pos)
{
    bool then;
    if (!ctxt . TryToEvaluateExpressionAsNonStrictBool(condition, line, pos, EE_IF_BADCOND, then))
        return;
    
	MCStatement *tspr;
	if (then)
		tspr = thenstatements;
	else
		tspr = elsestatements;
    
    ctxt . SetExecStat(MCKeywordsExecuteStatements(ctxt, tspr, EE_IF_BADSTATEMENT));
}

void MCKeywordsExecuteRepeatStatements(MCExecContext& ctxt, MCStatement *statements, uint2 line, uint2 pos, bool& r_done)
{
    Exec_stat stat = MCKeywordsExecuteStatements(ctxt, statements, EE_REPEAT_BADSTATEMENT);
    if ((stat == ES_NORMAL && MCexitall) || (stat != ES_NEXT_REPEAT && stat != ES_NORMAL))
    {
        r_done = true;
        return;
    }
    if (MCscreen->abortkey())
    {
        r_done = true;
        ctxt . LegacyThrow(EE_REPEAT_ABORT);
        return;
    }
    if (MCtrace || MCnbreakpoints)
    {
        MCB_trace(ctxt, line, pos);
        if (MCexitall)
            r_done = true;
    }
}

void MCKeywordsExecRepeatFor(MCExecContext& ctxt, MCStatement *statements, MCExpression *endcond, MCVarref *loopvar, File_unit each, uint2 line, uint2 pos)
{
    MCAutoArrayRef t_array;
    MCAutoStringRef t_string;
    MCRange t_chunk_range;
    t_chunk_range = MCRangeMake(0,0);
    uindex_t t_length = 0;
    MCAutoValueRef t_condition;
	MCNameRef t_key;
	MCValueRef t_value;
	uintptr_t t_iterator;
    Parse_stat ps;
    MCScriptPoint *sp = nil;
    int4 count = 0;
    
    if (!ctxt . TryToEvaluateExpression(endcond, line, pos, EE_REPEAT_BADFORCOND, &t_condition))
        return;
    
    if (loopvar != NULL)
    {
        if (each == FU_ELEMENT || each == FU_KEY)
        {
            if (!ctxt . ConvertToArray(*t_condition, &t_array))
                return;
            
            t_iterator = 0;
            
            if (!MCArrayIterate(*t_array, t_iterator, t_key, t_value))
                return;
        }
        else
        {
            if (!ctxt . ConvertToString(*t_condition, &t_string))
                return;
            
            t_length = MCStringGetLength(*t_string);
        }
    }
    else
    {
        if (!ctxt . ConvertToInteger(*t_condition, count))
            return;
        count = MCU_max(count, 0);
    }
    
    bool done;
    done = false;
    
    bool endnext;
    endnext = false;
    
    bool t_found;
    t_found = false;
    
    while (!done)
    {
        MCAutoStringRef t_unit;
        if (loopvar != NULL)
        {
            switch (each)
            {
                case FU_KEY:
                {
                    // MW-2010-12-15: [[ Bug 9218 ]] Make a copy of the key so that it can't be mutated
                    //   accidentally.
                    MCNewAutoNameRef t_key_copy;
                    MCNameClone(t_key, &t_key_copy);
                    loopvar -> set(ctxt, *t_key_copy);
                    if (!MCArrayIterate(*t_array, t_iterator, t_key, t_value))
                        endnext = true;
                }
                    break;
                case FU_ELEMENT:
                {
                    loopvar -> set(ctxt, t_value);
                    if (!MCArrayIterate(*t_array, t_iterator, t_key, t_value))
                        endnext = true;
                }
                    break;
                default:
                {
                    switch (each)
                    {
                        case FU_LINE:
                            t_found = MCStringsFindNextChunk(ctxt, *t_string, CT_LINE, t_length, t_chunk_range, t_found, endnext);
                            break;
                        case FU_ITEM:
                            t_found = MCStringsFindNextChunk(ctxt, *t_string, CT_ITEM, t_length, t_chunk_range, t_found, endnext);
                            break;
                        case FU_WORD:
                            t_found = MCStringsFindNextChunk(ctxt, *t_string, CT_WORD, t_length, t_chunk_range, t_found, endnext);
                            break;
                        case FU_TOKEN:
                            t_found = MCStringsFindNextChunk(ctxt, *t_string, CT_TOKEN, t_length, t_chunk_range, t_found, endnext);
                            break;
                        case FU_CHARACTER:
                        default:
                            t_found = MCStringsFindNextChunk(ctxt, *t_string, CT_CHARACTER, t_length, t_chunk_range, t_found, endnext);
                            break;
                    }
                    if (!t_found)
                    {
                        &t_unit = MCValueRetain(kMCEmptyString);
                        done = true;
                    }
                    else
                        MCStringCopySubstring(*t_string, t_chunk_range, &t_unit);
                }
            }
            // MW-2010-12-15: [[ Bug 9218 ]] Added KEY to the type of repeat that already
            //   copies the value.
            // MW-2011-02-08: [[ Bug ]] Make sure we don't use 't_unit' if the repeat type is 'key' or
            //   'element'.
            // Set the loop variable to whatever the value was in the last iteration.
            if (each != FU_ELEMENT && each != FU_KEY)
                loopvar->set(ctxt, *t_unit);
        }
        else
            done = count-- == 0;
        
        if (!done)
            MCKeywordsExecuteRepeatStatements(ctxt, statements, line, pos, done);
        
        if (done)
        {
            if (*t_unit == nil)
            {
                bool t_true = true;
            }
            // Reset the loop variable to whatever the value was in the last iteration.
            if (loopvar != nil && each != FU_ELEMENT && each != FU_KEY)
                loopvar->set(ctxt, *t_unit);
        }
        
        done = done || endnext;
    }
}

void MCKeywordsExecRepeatWith(MCExecContext& ctxt, MCStatement *statements, MCExpression *step, MCExpression *startcond, MCExpression *endcond, MCVarref *loopvar, real8 stepval, uint2 line, uint2 pos)
{
    real8 endn = 0.0;
    MCAutoValueRef t_condition, t_step;
    
    if (step != NULL)
    {
        if (!ctxt . TryToEvaluateExpression(step, line, pos, EE_REPEAT_BADWITHSTEP, &t_step))
            return;
        
        if (!ctxt . ConvertToReal(*t_condition, stepval) || stepval == 0.0)
        {
            ctxt . LegacyThrow(EE_REPEAT_BADWITHSTEP);
            return;
        }
        
    }
    
    if (!ctxt . TryToEvaluateExpression(startcond, line, pos, EE_REPEAT_BADWITHSTART, &t_condition))
        return;
    
    real8 t_loop;
    if (!ctxt . ConvertToReal(*t_condition, t_loop))
    {
        ctxt . LegacyThrow(EE_REPEAT_BADWITHSTART);
        return;
    }
    
    t_loop -= stepval;
    
    MCAutoNumberRef t_loop_var;
    if (!MCNumberCreateWithReal(t_loop, &t_loop_var) || !ctxt . TryToSetVariable(loopvar, line, pos, EE_REPEAT_BADWITHVAR, *t_loop_var))
        return;
    
    MCAutoValueRef t_end_condition;
    if (!ctxt . TryToEvaluateExpression(endcond, line, pos, EE_REPEAT_BADWITHSTART, &t_end_condition) ||
        !ctxt . ConvertToReal(*t_end_condition, endn))
        return;
    
    bool done;
    done = false;
    
    while (!done)
    {
        MCAutoValueRef t_loop_pos;
        real8 t_cur_value;
        if (!ctxt . TryToEvaluateExpression(loopvar, line, pos, EE_REPEAT_BADWITHVAR, &t_loop_pos) ||
            !ctxt . ConvertToReal(*t_loop_pos, t_cur_value))
            return;
        
        if (stepval < 0)
        {
            if (t_cur_value <= endn)
                done = true;
        }
        else
            if (t_cur_value >= endn)
                done = true;
        
        if (!done)
        {
            MCAutoNumberRef t_cur_loop;
            if (!MCNumberCreateWithReal(t_cur_value + stepval, &t_cur_loop) || !ctxt . TryToSetVariable(loopvar, line, pos, EE_REPEAT_BADWITHVAR, *t_cur_loop))
                return;
            
            MCKeywordsExecuteRepeatStatements(ctxt, statements, line, pos, done);
        }
        
        if (done)
            break;
    }
    
}

void MCKeywordsExecRepeatForever(MCExecContext& ctxt, MCStatement *statements, uint2 line, uint2 pos)
{
    bool done;
    done = false;
    while (!done)
        MCKeywordsExecuteRepeatStatements(ctxt, statements, line, pos, done);
}

void MCKeywordsExecRepeatUntil(MCExecContext& ctxt, MCStatement *statements, MCExpression *endcond, uint2 line, uint2 pos)
{
    bool done;
    done = false;
    
    while (!done)
    {
        if (!ctxt . TryToEvaluateExpressionAsNonStrictBool(endcond, line, pos, EE_REPEAT_BADUNTILCOND, done))            return;
        if (!done)
            MCKeywordsExecuteRepeatStatements(ctxt, statements, line, pos, done);
    }
}

void MCKeywordsExecRepeatWhile(MCExecContext& ctxt, MCStatement *statements, MCExpression *endcond, uint2 line, uint2 pos)
{
    bool done;
    done = false;
    
    while (!done)
    {
        MCAutoValueRef t_value;
        bool not_done;
        if (!ctxt . TryToEvaluateExpressionAsNonStrictBool(endcond, line, pos, EE_REPEAT_BADUNTILCOND, not_done))
            return;
        
        done = !not_done;
        
        if (not_done)
            MCKeywordsExecuteRepeatStatements(ctxt, statements, line, pos, done);
    }
}

void MCKeywordsExecTry(MCExecContext& ctxt, MCStatement *trystatements, MCStatement *catchstatements, MCStatement *finallystatements, MCVarref *errorvar, uint2 line, uint2 pos)
{
	Try_state state = TS_TRY;
	MCStatement *tspr = trystatements;
	Exec_stat stat;
	Exec_stat retcode = ES_NORMAL;
	MCtrylock++;
	while (tspr != NULL)
	{
		if (MCtrace || MCnbreakpoints)
		{
			MCB_trace(ctxt, tspr->getline(), tspr->getpos());
			if (MCexitall)
				break;
		}
		ctxt . GetEP() . setline(tspr->getline());
        
		stat = tspr->exec(ctxt . GetEP());
        
		// MW-2011-08-17: [[ Redraw ]] Flush any screen updates.
		MCRedrawUpdateScreen();
        
		switch(stat)
		{
            case ES_NORMAL:
                tspr = tspr->getnext();
                if (MCexitall)
                {
                    retcode = ES_NORMAL;
                    tspr = NULL;
                }
                
                if (tspr == NULL && state != TS_FINALLY)
                {
                    if (state == TS_CATCH)
                        MCeerror->clear();
                    
                    tspr = finallystatements;
                    state = TS_FINALLY;
                }
                break;
            case ES_ERROR:
                if ((MCtrace || MCnbreakpoints) && state != TS_TRY)
                    do
                    {
                        MCB_error(ctxt, tspr->getline(), tspr->getpos(), EE_TRY_BADSTATEMENT);
                    }
				while(MCtrace && (stat = tspr->exec(ctxt . GetEP())) != ES_NORMAL);
                
                if (stat == ES_ERROR)
                {
                    if (MCexitall)
                    {
                        retcode = ES_NORMAL;
                        tspr = NULL;
                    }
                    else
                        if (state != TS_TRY)
                        {
                            MCtrylock--;
                            return;
                        }
                        else
                        {
                            if (errorvar != NULL)
                            {
                                MCAutoStringRef t_error;
                                MCeerror -> copyasstringref(&t_error);
                                errorvar->evalvar(ctxt)->setvalueref(*t_error);
                            }
                            
                            // MW-2007-09-04: At this point we need to clear the execution error
                            //   stack so that errors inside the catch statements are reported
                            //   correctly.
                            MCeerror->clear();
                            MCperror->clear();
                            
                            
                            tspr = catchstatements;
                            state = TS_CATCH;
                            
                            // MW-2007-07-03: [[ Bug 3029 ]] - If there is no catch clause
                            //   we end up skipping the finally as the loop terminates
                            //   before a state transition is made, thus we force it here.
                            if (catchstatements == NULL)
                            {
                                MCeerror -> clear();
                                tspr = finallystatements;
                                state = TS_FINALLY;
                            }
                        }
                }
                else
                    tspr = tspr->getnext();
                break;
            case ES_PASS:
                if (state == TS_CATCH)
                {
                    MCAutoStringRef t_string;
                    if (ctxt . ConvertToString(errorvar->evalvar(ctxt)->getvalueref(), &t_string))
                    {
                        MCeerror->copystringref(*t_string, False);
                        MCeerror->add(EE_TRY_BADSTATEMENT, line, pos);
                        stat = ES_ERROR;
                    }
                }
            default:
                if (state == TS_FINALLY)
                {
                    MCeerror->clear();
                    retcode = ES_NORMAL;
                    tspr = NULL;
                }
                else
                {
                    retcode = stat;
                    tspr = finallystatements;
                    state = TS_FINALLY;
                }
		}
	}
	if (state == TS_CATCH)
		MCeerror->clear();
	MCtrylock--;
	ctxt . SetExecStat(retcode);
}

void MCKeywordsExecExit(MCExecContext& ctxt, Exec_stat stat)
{
    if (stat == ES_EXIT_ALL)
	{
		MCexitall = True;
		ctxt . SetExecStat(ES_NORMAL);
	}
	ctxt . SetExecStat(stat);
}

void MCKeywordsExecBreak(MCExecContext& ctxt)
{
    ctxt . SetExecStat(ES_EXIT_SWITCH);
}

void MCKeywordsExecNext(MCExecContext& ctxt)
{
    ctxt . SetExecStat(ES_NEXT_REPEAT);
}

void MCKeywordsExecPassAll(MCExecContext& ctxt)
{
	ctxt . SetExecStat(ES_PASS_ALL);
}

void MCKeywordsExecPass(MCExecContext& ctxt)
{
	ctxt . SetExecStat(ES_PASS);
}

void MCKeywordsExecThrow(MCExecContext& ctxt, MCStringRef p_error)
{
	ctxt . UserThrow(p_error);
}

////////////////////////////////////////////////////////////////////////////////
