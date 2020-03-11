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
#include "mcio.h"

#include "globals.h"
#include "debug.h"
#include "handler.h"

#include "exec.h"

#include "stack.h"
#include "util.h"
#include "parentscript.h"
#include "mcerror.h"
#include "param.h"

#include "chunk.h"
#include "scriptpt.h"
#include "osspec.h"

////////////////////////////////////////////////////////////////////////////////

void MCDebuggingExecDebugDo(MCExecContext& ctxt, MCStringRef p_script, uinteger_t p_line, uinteger_t p_pos)
{
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ctxt.SetLineAndPos(p_line, p_pos);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
		added = True;
	}

	if (MCdebugcontext >= MCnexecutioncontexts)
		MCdebugcontext = MCnexecutioncontexts - 1;

	MCExecContext *t_ctxt_ptr;
	t_ctxt_ptr = MCexecutioncontexts[MCdebugcontext];

    // Do not permit script to be executed in an inaccessible context
    if (t_ctxt_ptr->GetObject()->getstack()->iskeyed())
        t_ctxt_ptr->doscript(*t_ctxt_ptr, p_script, p_line, p_pos);
    else
        ctxt.LegacyThrow(EE_STACK_NOKEY);
    
    // AL-2014-03-21: [[ Bug 11940 ]] Ensure the debug context is not permanently in a state of error.
    t_ctxt_ptr -> IgnoreLastError();
    
	if (added)
		MCnexecutioncontexts--;
}

////////////////////////////////////////////////////////////////////////////////

void MCDebuggingExecBreakpoint(MCExecContext& ctxt, uinteger_t p_line, uinteger_t p_pos)
{
    // Ignore breakpoints in inaccessible stacks
    if (ctxt.GetObject()->getstack()->iskeyed())
        MCB_break(ctxt, p_line, p_pos);
}

////////////////////////////////////////////////////////////////////////////////

void MCDebuggingGetTraceAbort(MCExecContext& ctxtm, bool& r_value)
{
	r_value = MCtraceabort == True;
}

void MCDebuggingSetTraceAbort(MCExecContext& ctxtm, bool p_value)
{
	MCtraceabort = p_value ? True : False;
}

void MCDebuggingGetTraceDelay(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCtracedelay;
}

void MCDebuggingSetTraceDelay(MCExecContext& ctxt, uinteger_t p_value)
{
	MCtracedelay = p_value;
}

void MCDebuggingGetTraceReturn(MCExecContext& ctxtm, bool& r_value)
{
	r_value = MCtracereturn == True;
}

void MCDebuggingSetTraceReturn(MCExecContext& ctxtm, bool p_value)
{
	MCtracereturn = p_value ? True : False;
}

void MCDebuggingGetTraceStack(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (!MCtracestackptr)
	{
		r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
		return;
	}

	MCAutoValueRef t_value;
	if (MCtracestackptr -> names(P_NAME, &t_value))
		if (ctxt.ConvertToString(*t_value, r_value))
		return;

	ctxt . Throw();
}

void MCDebuggingSetTraceStack(MCExecContext& ctxt, MCStringRef p_value)
{
	if (MCStringGetLength(p_value) == 0)
	{
		MCtracestackptr = nil;
		return;
	}

	MCStack *sptr;
	sptr = MCdefaultstackptr->findstackname_string(p_value);
	
	if (sptr == nil)
	{
		ctxt . LegacyThrow(EE_PROPERTY_NODEFAULTSTACK);
		return;
	}

	MCtracestackptr = sptr;
}

void MCDebuggingGetTraceUntil(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCtraceuntil;
}

void MCDebuggingSetTraceUntil(MCExecContext& ctxt, uinteger_t p_value)
{
	MCtraceuntil = p_value;
}

void MCDebuggingGetMessageMessages(MCExecContext& ctxtm, bool& r_value)
{
	r_value = MCmessagemessages == True;
}

void MCDebuggingSetMessageMessages(MCExecContext& ctxtm, bool p_value)
{
	MCmessagemessages = p_value ? True : False;
}

void MCDebuggingGetBreakpoints(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (MCB_unparsebreaks(r_value))
		return;

	r_value = MCValueRetain(kMCEmptyString);
}

void MCDebuggingSetBreakpoints(MCExecContext& ctxt, MCStringRef p_value)
{
	MCB_parsebreaks(ctxt, p_value);
}

void MCDebuggingGetDebugContext(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (MCdebugcontext == MAXUINT2)
	{
		r_value = MCValueRetain(kMCEmptyString);
		return;
	}

	bool t_success;
	t_success = true;

	MCAutoListRef t_list;

	if (t_success)
		t_success = MCListCreateMutable(',', &t_list);

	if (t_success)
	{
		MCAutoValueRef t_context_id;
		t_success = MCexecutioncontexts[MCdebugcontext]->GetObject()->names(P_LONG_ID, &t_context_id) &&
					MCListAppend(*t_list, *t_context_id);
	}
	
    // Don't display the handler name if the stack script is not available
    if (MCexecutioncontexts[MCdebugcontext]->GetObject()->getstack()->iskeyed())
    {
        if (t_success)
            t_success = MCListAppend(*t_list, MCexecutioncontexts[MCdebugcontext]->GetHandler()->getname());

        if (t_success)
            t_success = MCListAppendInteger(*t_list, MCexecutioncontexts[MCdebugcontext] -> GetLine());
    }
    else
    {
        if (t_success)
            t_success = MCListAppend(*t_list, MCNAME("<protected>")) && MCListAppendInteger(*t_list, 0);
    }
	
	if (t_success)
		t_success = MCListCopyAsString(*t_list, r_value);
	
	if (t_success)
		return;
	
	ctxt . Throw();
}

void MCDebuggingSetDebugContext(MCExecContext& ctxt, MCStringRef p_value)
{
	uindex_t t_length = MCStringGetLength(p_value);
	uindex_t t_offset;

	if (MCStringLastIndexOfChar(p_value, ',', t_length, kMCStringOptionCompareExact, t_offset))
	{
		MCAutoStringRef t_head;
		MCAutoStringRef t_tail;
		int4 t_line;
		MCObjectPtr t_object;		

		if (MCStringDivideAtIndex(p_value, t_offset, &t_head, &t_tail) &&
			MCInterfaceTryToResolveObject(ctxt, *t_head, t_object) &&
			MCU_strtol(*t_tail, t_line))
		{
            // If this object isn't debuggable, fail
            if (!t_object.object->getstack()->iskeyed())
            {
                ctxt.LegacyThrow(EE_STACK_NOKEY);
                return;
            }
            
            for (uint2 i = 0; i < MCnexecutioncontexts; i++)
			{
				if (MCexecutioncontexts[i] -> GetObject() == t_object . object && 
                    MCexecutioncontexts[i] -> GetLine() == t_line)
				{
					MCdebugcontext = i;
					break;
				}
			}
			return;
		}
	}
	else
	{
		int4 i;
		if (MCU_strtol(p_value, i) && i <= MCnexecutioncontexts)
			MCdebugcontext = i - 1;
		else
			MCdebugcontext = MAXUINT2;
		return;
	}

	ctxt . Throw();
}

void MCDebuggingGetExecutionContexts(MCExecContext& ctxt, MCStringRef& r_value)
{
	int i;
	bool added = false;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
		added = true;
	}

	bool t_success;
	t_success = true;

	MCAutoListRef t_context_list;
	if (t_success)
		t_success = MCListCreateMutable('\n', &t_context_list);

	if (t_success)
	{
		for (i = 0 ; i < MCnexecutioncontexts ; i++)
		{
			MCAutoListRef t_context;
			t_success = MCListCreateMutable(',', &t_context);
			
            // Don't display context information when not available
            if (MCexecutioncontexts[i]->GetObject()->getstack()->iskeyed())
            {
                if (t_success)
                {
                    MCAutoValueRef t_context_id;
                    t_success = MCexecutioncontexts[i]->GetObject()->names(P_LONG_ID, &t_context_id) &&
                                MCListAppend(*t_context, *t_context_id);
                }
                
                // PM-2014-04-14: [[Bug 12125]] Do this check to avoid a crash in LC server
                if (t_success && MCexecutioncontexts[i]->GetHandler() != NULL)
                    t_success = MCListAppend(*t_context, MCexecutioncontexts[i]->GetHandler()->getname());
                
                if (t_success)
                {
                    MCAutoStringRef t_line;
                    t_success = MCStringFormat(&t_line, "%d", MCexecutioncontexts[i] -> GetLine()) &&
                                MCListAppend(*t_context, *t_line);
                }
                
                if (t_success && MCexecutioncontexts[i] -> GetParentScript() != NULL)
                {
                    MCAutoValueRef t_parent;
                    t_success = MCexecutioncontexts[i] -> GetParentScript() -> GetParent() -> GetObject() -> names(P_LONG_ID, &t_parent) &&
                                MCListAppend(*t_context, *t_parent);
                }
            }
            else
            {
                if (t_success)
                {
                    MCAutoValueRef t_context_id;
                    t_success = MCexecutioncontexts[i]->GetObject()->names(P_LONG_ID, &t_context_id)
                                && MCListAppend(*t_context, *t_context_id)
                                && MCListAppend(*t_context, MCNAME("<protected>"))
                                && MCListAppendInteger(*t_context, 0);
                }
 
            }

			if (t_success)
				t_success = MCListAppend(*t_context_list, *t_context);
		}
	}
	if (added)
		MCnexecutioncontexts--;
	
	if (t_success)
		t_success = MCListCopyAsString(*t_context_list, r_value);

	if (!t_success)
		r_value = MCValueRetain(kMCEmptyString);

}

void MCDebuggingGetWatchedVariables(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (MCB_unparsewatches(r_value))
		return;

	r_value = MCValueRetain(kMCEmptyString);
}

void MCDebuggingSetWatchedVariables(MCExecContext& ctxt, MCStringRef p_value)
{
	MCB_parsewatches(ctxt, p_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCDebuggingGetLogMessage(MCExecContext& ctxt, MCStringRef& r_value)
{
    r_value = MCValueRetain(MCNameGetString(MClogmessage));
}

void MCDebuggingSetLogMessage(MCExecContext& ctxt, MCStringRef p_value)
{
    MCNewAutoNameRef t_logmessage;
    if (!MCNameCreate(p_value, &t_logmessage))
    {
        ctxt.Throw();
        return;
    }
    
    MCValueAssign(MClogmessage, *t_logmessage);
}

////////////////////////////////////////////////////////////////////////////////

void MCDebuggingExecAssert(MCExecContext& ctxt, int type, bool p_eval_success, bool p_result)
{
    switch(type)
	{
		case ASSERT_TYPE_NONE:
		case ASSERT_TYPE_TRUE:
			// If the expression threw an error, then just throw.
			if (!p_eval_success)
            {
                ctxt . Throw();
                return;
            }
			
			// If the expression is true, we are done.
			if (p_result)
				return;
            break;
            
		case ASSERT_TYPE_FALSE:
			// If the expression threw an error, then just throw.
			if (!p_eval_success)
            {
                ctxt . Throw();
                return;
            }
			
			// If the expression is not true, we are done. (this uses the same logic as if).
			if (!p_result)
				return;
            break;
            
		case ASSERT_TYPE_SUCCESS:
			if (p_eval_success)
				return;
			break;
            
		case ASSERT_TYPE_FAILURE:
			if (!p_eval_success)
				return;
			break;
			
		default:
			assert(false);
			break;
	}
	
	// Clear the execution error.
	MCeerror -> clear();
	
	// Dispatch 'assertError <handler>, <line>, <pos>, <object>'
	MCParameter t_handler, t_line, t_pos, t_object;
	if (ctxt . GetHandler() != NULL) {
		t_handler.setvalueref_argument(ctxt . GetHandler() -> getname());
	}
	t_handler.setnext(&t_line);
	t_line.setn_argument((real8)ctxt . GetLine());
	t_line.setnext(&t_pos);
	t_pos.setn_argument((real8)ctxt . GetPos());
	t_pos.setnext(&t_object);
    MCAutoValueRef t_long_id;
	ctxt . GetObject() -> getvariantprop(ctxt, 0, P_LONG_ID, False, &t_long_id);
	t_object.setvalueref_argument(*t_long_id);
	
	ctxt . GetObject() -> message(MCM_assert_error, &t_handler);
}

////////////////////////////////////////////////////////////////////////////////

void MCDebuggingExecPutIntoMessage(MCExecContext& ctxt, MCStringRef p_value, int p_where)
{
	if (!MCS_put(ctxt, p_where == PT_INTO ? kMCSPutIntoMessage : (p_where == PT_BEFORE ? kMCSPutBeforeMessage : kMCSPutAfterMessage), p_value))
		ctxt . LegacyThrow(EE_PUT_CANTSETINTO);
}
