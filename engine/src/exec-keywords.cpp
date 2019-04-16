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
        ctxt . SetLineAndPos(tspr->getline(), tspr->getpos());
        
       // stat = tspr->exec(ctxt . GetEP());
        tspr->exec_ctxt(ctxt);
        stat = ctxt . GetExecStat();
        ctxt . IgnoreLastError();
        
        MCActionsRunAll();
        
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
            // SN-2014-08-06: [[ Bug 13122 ]] We want to know that we got an EXIT_SWITCH,
            //  since we might be inside an IF statement
            case ES_EXIT_REPEAT:
            case ES_EXIT_SWITCH:
                return stat;
            case ES_ERROR:
                if ((MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
                    do
                    {
                        if (!MCB_error(ctxt, tspr->getline(), tspr->getpos(),
                                  p_error))
                            break;
                        ctxt . IgnoreLastError();
                        tspr->exec_ctxt(ctxt);
                    }
                while (MCtrace && (stat = ctxt . GetExecStat()) != ES_NORMAL);
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

void MCKeywordsExecResolveCommandOrFunction(MCExecContext& ctxt, MCNameRef p_name, bool is_function, MCHandler*& r_handler)
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
    t_resolved_handler = t_object -> findhandler(is_function ? HT_FUNCTION : HT_MESSAGE, p_name);
    if (t_resolved_handler == NULL ||
        !t_resolved_handler -> isprivate())
    {
        t_resolved_handler = nullptr;
    }
    r_handler = t_resolved_handler;
}

bool MCKeywordsExecSetupCommandOrFunction(MCExecContext& ctxt, MCParameter *params, MCContainer *containers, uint2 line, uint2 pos, bool is_function)
{
    MCParameter *tptr = params;
    uindex_t t_container_index = 0;
    while (tptr != NULL)
    {
        /* If the parameter evaluates as a container, then place the result
         * into the next available container slot and bump the index; otherwise
         * evaluate as an expression. */
        if (tptr -> evalcontainer(ctxt, containers[t_container_index]))
        {
            tptr -> set_argument_container(&containers[t_container_index]);
            t_container_index += 1;
        }
        else
        {
            MCExecValue t_value;
            
            if (!ctxt.TryToEvaluateParameter(tptr,
                                             line,
                                             pos, 
                                             is_function ? EE_FUNCTION_BADSOURCE : EE_STATEMENT_BADPARAM,
                                             t_value))
            {
                return false;
            }
            
            tptr -> clear_argument();
            tptr->give_exec_argument(t_value);
        }
        
        tptr = tptr->getnext();
    }
    
    return true;
}

void MCKeywordsExecTeardownCommandOrFunction(MCParameter *params)
{
    // AL-2014-09-17: [[ Bug 13465 ]] Clear parameters after executing dispatch
    MCParameter *tptr = params;
    while (tptr != NULL)
    {
        tptr -> clear_argument();
        tptr = tptr->getnext();
    }
}

void MCKeywordsExecCommandOrFunction(MCExecContext& ctxt, MCHandler *handler, MCParameter *params, MCNameRef name, uint2 line, uint2 pos, bool global_handler, bool is_function)
{    
	if (MCscreen->abortkey())
	{
		ctxt . LegacyThrow(EE_HANDLER_ABORT);
		return;
	}
        
    if (is_function)
        MCexitall = False;

    Exec_stat stat;
    stat = ES_NOT_HANDLED;
    MCObject *p = ctxt . GetObject();
	MCExecContext *oldctxt = MCECptr;
	MCECptr = &ctxt;
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ctxt . SetLineAndPos(line, pos);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
		added = True;
	}
    
    if (handler != nil)
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
                if (!MCerrorptr)
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
		MCdynamicpath = MCdynamiccard.IsValid();
		if (stat == ES_PASS || stat == ES_NOT_HANDLED)
        {
            if (is_function)
            {
                // PASS STATE FIX
                Exec_stat oldstat = stat;
                stat = p->handle(HT_FUNCTION, name, params, p);
                if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
                    stat = ES_PASS;
                
                /* The following clause was pulled in from MCFuncref::eval_ctxt
                 * it is not quite clear why this code path is different from
                 * commands; however without the clause a test failure occurs
                 * in 'TestExtensionLibraryHandlerCallErrors'. */
                // MW-2007-08-09: [[ Bug 5705 ]] Throws inside private functions don't trigger an
                //   exception.
                if (!global_handler &&
                    stat != ES_NORMAL &&
                    stat != ES_PASS &&
                    stat != ES_EXIT_HANDLER)
                {
                    MCeerror->add(EE_FUNCTION_BADFUNCTION, line, pos, name);
                    stat = ES_ERROR;
                }
            }
            else
            {
                switch (stat = p->handle(HT_MESSAGE, name, params, p))
                {
                    case ES_ERROR:
                    case ES_NOT_FOUND:
                    case ES_NOT_HANDLED:
                    case ES_PASS:
                        if (!global_handler)
                        {
                            MCeerror->add(EE_STATEMENT_BADCOMMAND, line, pos, name);
                            stat = ES_ERROR;
                        }
                        break;
                    case ES_EXIT_HANDLER:
                        stat = ES_NORMAL;
                        break;
                    default:
                        break;
                }
            }
            
            if (global_handler && (stat == ES_NOT_FOUND || stat == ES_NOT_HANDLED))
            {
                if (!MCRunGlobalHandler(name, params, stat))
                    stat = ES_NOT_HANDLED;
                
                // AL-2014-03-14: Currently no mobile handler's execution is halted when ES_ERROR
                //  is returned. Error info is returned via the result.
#ifdef _MOBILE
                if (stat != ES_NOT_HANDLED)
                    stat = ES_NORMAL;
#endif
            }
        }
		
		MCdynamicpath = olddynamic;
	}
	MCECptr = oldctxt;
	if (added)
		MCnexecutioncontexts--;
    
    if (stat != ES_NORMAL && stat != ES_PASS && stat != ES_EXIT_HANDLER)
        ctxt . SetExecStat(stat);
    else
        ctxt . SetExecStat(ES_NORMAL);
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
        
		if (MCStringIsEqualTo(*t_cond, *t_case_string, ctxt . GetStringComparisonType()))
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
        
        // SN-2014-08-06: [[ Bug 13122 ]] If we get an EXIT_SWITCH, it's all right
        Exec_stat t_stat;
        t_stat = MCKeywordsExecuteStatements(ctxt, tspr, EE_SWITCH_BADSTATEMENT);
        if (t_stat == ES_EXIT_SWITCH)
            t_stat = ES_NORMAL;
        
        ctxt . SetExecStat(t_stat);
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

    r_done = false;
    if ((stat == ES_NORMAL && MCexitall) || (stat != ES_NEXT_REPEAT && stat != ES_NORMAL))
    {
        r_done = true;
        if (stat != ES_EXIT_REPEAT)
            ctxt . SetExecStat(stat);
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

void MCKeywordsExecRepeatCount(MCExecContext& ctxt, MCStatement *statements, MCExpression *endcond, uint2 line, uint2 pos)
{
    MCAutoValueRef t_condition;
    
    if (!ctxt . TryToEvaluateExpression(endcond, line, pos, EE_REPEAT_BADFORCOND, &t_condition))
        return;
    
    // SN-2015-01-14: [[ Bug 14377 ]] Throw an error as it used to be done
    integer_t t_count;
    if (!ctxt . ConvertToInteger(*t_condition, t_count) && (MCtrace || MCnbreakpoints)
        && !MCtrylock && !MClockerrors)
    {
        MCB_error(ctxt, line, pos, EE_REPEAT_BADFORCOND);
        return;
    }
    
    while(t_count > 0)
    {
        bool t_done;
        MCKeywordsExecuteRepeatStatements(ctxt, statements, line, pos, t_done);
        if (t_done)
            break;
        
        t_count -= 1;
    }
}

void MCKeywordsExecRepeatFor(MCExecContext& ctxt, MCStatement *statements, MCExpression *endcond, MCVarref *loopvar, File_unit each, uint2 line, uint2 pos)
{
    MCAutoArrayRef t_array;
    MCAutoStringRef t_string;
    MCAutoDataRef t_data;
    MCRange t_chunk_range;
    t_chunk_range = MCRangeMake(0,0);
    uindex_t t_length = 0;
    MCAutoValueRef t_condition;
	MCNameRef t_key;
	MCValueRef t_value;
	uintptr_t t_iterator;
    // SN2015-06-15: [[ Bug 15457 ]] The index can be a negative index.
    index_t t_sequenced_iterator;
    const byte_t *t_data_ptr;
    
    MCAutoPointer<MCTextChunkIterator> tci;
    
    if (!ctxt . TryToEvaluateExpression(endcond, line, pos, EE_REPEAT_BADFORCOND, &t_condition))
        return;
    
    bool t_sequence_array;
    t_sequence_array = false;

    if (each == FU_ELEMENT || each == FU_KEY)
    {
        if (!ctxt . ConvertToArray(*t_condition, &t_array))
            return;
        
        // SN-2015-06-15: [[ Bug 15457 ]] If this is a numerical array, do
        //  it in order - even if it does not start at 1
        if (each == FU_ELEMENT && MCArrayIsNumericSequence(*t_array, t_sequenced_iterator))
        {
            t_sequence_array = true;
            if (!MCArrayFetchValueAtIndex(*t_array, t_sequenced_iterator, t_value))
                return;
        }
        else
        {
            t_iterator = 0;
            if (!MCArrayIterate(*t_array, t_iterator, t_key, t_value))
                return;
        }
    }
    else if (each == FU_BYTE)
    {
        if (!ctxt . ConvertToData(*t_condition, &t_data))
            return;
        
        t_length = MCDataGetLength(*t_data);
        t_data_ptr = MCDataGetBytePtr(*t_data);
    }
    else
    {
        if (!ctxt . ConvertToString(*t_condition, &t_string))
            return;
        
        switch (each)
        {
            case FU_LINE:
                tci = MCStringsTextChunkIteratorCreate(ctxt, *t_string, CT_LINE);
                break;
            case FU_PARAGRAPH:
                tci = MCStringsTextChunkIteratorCreate(ctxt, *t_string, CT_PARAGRAPH);
                break;
            case FU_SENTENCE:
                tci = MCStringsTextChunkIteratorCreate(ctxt, *t_string, CT_SENTENCE);
                break;
            case FU_ITEM:
                tci = MCStringsTextChunkIteratorCreate(ctxt, *t_string, CT_ITEM);
                break;
            case FU_WORD:
                tci = MCStringsTextChunkIteratorCreate(ctxt, *t_string, CT_WORD);
                break;
            case FU_TRUEWORD:
                tci = MCStringsTextChunkIteratorCreate(ctxt, *t_string, CT_TRUEWORD);
                break;
            case FU_TOKEN:
                tci = MCStringsTextChunkIteratorCreate(ctxt, *t_string, CT_TOKEN);
                break;
            case FU_CODEPOINT:
                tci = MCStringsTextChunkIteratorCreate(ctxt, *t_string, CT_CODEPOINT);
                break;
            case FU_CODEUNIT:
                tci = MCStringsTextChunkIteratorCreate(ctxt, *t_string, CT_CODEUNIT);
                break;
            case FU_CHARACTER:
            default:
                tci = MCStringsTextChunkIteratorCreate(ctxt, *t_string, CT_CHARACTER);
                break;
        } 
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
        MCAutoDataRef t_byte;
        switch (each)
        {
            case FU_KEY:
            {
                loopvar -> set(ctxt, t_key);
                if (!MCArrayIterate(*t_array, t_iterator, t_key, t_value))
                    endnext = true;
            }
            break;
                
            case FU_ELEMENT:
            {
                loopvar -> set(ctxt, t_value);
                // SN-2015-06-15: [[ Bug 15457 ]] Sequenced, numeric arrays
                //  have their own iterator
                if (t_sequence_array)
                {
                    if (!MCArrayFetchValueAtIndex(*t_array, ++t_sequenced_iterator, t_value))
                        endnext = true;
                }
                else
                {
                    if (!MCArrayIterate(*t_array, t_iterator, t_key, t_value))
                        endnext = true;
                }
            }
            break;
            
            case FU_BYTE:
            {
                // SN-2014-04-14 [[ Bug 12184 ]] If we have no data at all, we don't want to start the loop
                if (t_length)
                {
                    MCDataCreateWithBytes(t_data_ptr++, 1, &t_byte);
                    
                    endnext = (--t_length) == 0;
                }
                else
                    done = true;
            }
            break;
                
            default:
            {
                t_found = MCStringsTextChunkIteratorNext(ctxt, *tci);
                endnext = tci -> IsExhausted();
                
                if (!t_found)
                {
                    t_unit = kMCEmptyString;
                    done = true;
                }
                else
                {
                    t_unit.Reset();
                    tci -> CopyString(&t_unit);
                }
            }
            break;
        }
        // MW-2010-12-15: [[ Bug 9218 ]] Added KEY to the type of repeat that already
        //   copies the value.
        // MW-2011-02-08: [[ Bug ]] Make sure we don't use 't_unit' if the repeat type is 'key' or
        //   'element'.
        // Set the loop variable to whatever the value was in the last iteration.
        if (each == FU_BYTE)
        {
            // SN-2014-04-14 [[ Bug 12184 ]] We don't need to set anything since we are not going in the loop
            if (!done)
                loopvar -> set(ctxt, *t_byte);
        }
        else if (each != FU_ELEMENT && each != FU_KEY)
            loopvar -> set(ctxt, *t_unit);
        
        if (!done)
            MCKeywordsExecuteRepeatStatements(ctxt, statements, line, pos, done);
        
        if (endnext)
        {
            // Reset the loop variable to whatever the value was in the last iteration.
            if (loopvar != nil)
            {
                if (each == FU_BYTE)
                    loopvar -> set(ctxt, *t_byte);
                else if (each != FU_ELEMENT && each != FU_KEY)
                    loopvar -> set(ctxt, *t_unit);
            }
        }
        
        done = done || endnext;
    }
}

void MCKeywordsExecRepeatWith(MCExecContext& ctxt, MCStatement *statements, MCExpression *step, MCExpression *startcond, MCExpression *endcond, MCVarref *loopvar, real8 stepval, uint2 line, uint2 pos)
{
    real8 endn = 0.0;
    
    if (step != NULL)
    {
        if (!ctxt . TryToEvaluateExpressionAsDouble(step, line, pos, EE_REPEAT_BADWITHSTEP, stepval) || stepval == 0)
        {
            ctxt . LegacyThrow(EE_REPEAT_BADWITHSTEP);
            return;
        }
        
    }
    
    real8 t_loop;
    if (!ctxt . TryToEvaluateExpressionAsDouble(startcond, line, pos, EE_REPEAT_BADWITHSTART, t_loop))
    {
        ctxt . LegacyThrow(EE_REPEAT_BADWITHSTART);
        return;
    }
    
    t_loop -= stepval;
    
    MCExecValue t_loop_var;
    t_loop_var . type = kMCExecValueTypeDouble;
    t_loop_var . double_value = t_loop;
    if (!ctxt . TryToSetVariable(loopvar, line, pos, EE_REPEAT_BADWITHVAR, t_loop_var))
        return;
    
    if (!ctxt . TryToEvaluateExpressionAsDouble(endcond, line, pos, EE_REPEAT_BADWITHSTART, endn))
        return;
    
    bool done;
    done = false;
    
    while (!done)
    {
        real8 t_cur_value;
        if (!ctxt . TryToEvaluateExpressionAsDouble(loopvar, line, pos, EE_REPEAT_BADWITHVAR, t_cur_value))
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
            t_loop_var . double_value = t_cur_value + stepval;
            if (!ctxt . TryToSetVariable(loopvar, line, pos, EE_REPEAT_BADWITHVAR, t_loop_var))
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
        if (!ctxt . TryToEvaluateExpressionAsNonStrictBool(endcond, line, pos, EE_REPEAT_BADUNTILCOND, done))
            return;
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
		ctxt . SetLineAndPos(tspr->getline(), tspr->getpos());
        
		//stat = tspr->exec(ctxt . GetEP());
        tspr->exec_ctxt(ctxt);
        stat = ctxt . GetExecStat();
        ctxt . IgnoreLastError();
        
        MCActionsRunAll();
        
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
                    // Everything has executed normally but there may have been an
                    // error added on another event. The trylock needs refactoring to
                    // ensure a trylock on one event can't cause issues in another
                    // event.
                    MCeerror->clear();
                    
                    tspr = finallystatements;
                    state = TS_FINALLY;
                }
                break;
            case ES_ERROR:
                if ((MCtrace || MCnbreakpoints) && state != TS_TRY)
                    do
                    {
                        if (MCB_error(ctxt, tspr->getline(), tspr->getpos(), EE_TRY_BADSTATEMENT))
                            break;
                        ctxt.IgnoreLastError();
                        tspr->exec_ctxt(ctxt);
                    }
				while(MCtrace && (stat = ctxt . GetExecStat()) != ES_NORMAL);
                
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
                                errorvar->set(ctxt, *t_error);
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
                    MCAutoValueRef t_value;
                    MCAutoStringRef t_string;
                    if ((errorvar->eval(ctxt, &t_value), !ctxt.HasError()) &&
                        ctxt . ConvertToString(*t_value, &t_string))
                    {
                        MCeerror->copystringref(*t_string, False);
                    }
                    
                    MCeerror->add(EE_TRY_BADSTATEMENT, line, pos);
                    stat = ES_ERROR;
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
        // SN-2014-09-17: [[ Bug 13430 ]] Missing return.
        return;
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
