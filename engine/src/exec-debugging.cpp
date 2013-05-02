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
#include "mcio.h"

#include "globals.h"
#include "debug.h"
#include "handler.h"

#include "exec.h"

#include "stack.h"
#include "util.h"
#include "parentscript.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EXEC_METHOD(Debugging, Breakpoint, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Debugging, DebugDo, 3)
MC_EXEC_DEFINE_GET_METHOD(Debugging, TraceAbort, 1)
MC_EXEC_DEFINE_SET_METHOD(Debugging, TraceAbort, 1)
MC_EXEC_DEFINE_GET_METHOD(Debugging, TraceDelay, 1)
MC_EXEC_DEFINE_SET_METHOD(Debugging, TraceDelay, 1)
MC_EXEC_DEFINE_GET_METHOD(Debugging, TraceReturn, 1)
MC_EXEC_DEFINE_SET_METHOD(Debugging, TraceReturn, 1)
MC_EXEC_DEFINE_GET_METHOD(Debugging, TraceStack, 1)
MC_EXEC_DEFINE_SET_METHOD(Debugging, TraceStack, 1)
MC_EXEC_DEFINE_GET_METHOD(Debugging, TraceUntil, 1)
MC_EXEC_DEFINE_SET_METHOD(Debugging, TraceUntil, 1)
MC_EXEC_DEFINE_GET_METHOD(Debugging, MessageMessages, 1)
MC_EXEC_DEFINE_SET_METHOD(Debugging, MessageMessages, 1)
MC_EXEC_DEFINE_GET_METHOD(Debugging, Breakpoints, 1)
MC_EXEC_DEFINE_SET_METHOD(Debugging, Breakpoints, 1)
MC_EXEC_DEFINE_GET_METHOD(Debugging, DebugContext, 1)
MC_EXEC_DEFINE_SET_METHOD(Debugging, DebugContext, 1)
MC_EXEC_DEFINE_GET_METHOD(Debugging, ExecutionContexts, 1)
MC_EXEC_DEFINE_GET_METHOD(Debugging, WatchedVariables, 1)
MC_EXEC_DEFINE_SET_METHOD(Debugging, WatchedVariables, 1)

////////////////////////////////////////////////////////////////////////////////

void MCDebuggingExecDebugDo(MCExecContext& ctxt, MCStringRef p_script, int p_line, int p_pos)
{
	MCExecPoint& ep = ctxt . GetEP();

	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ep.setline(p_line);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
		added = True;
	}

	if (MCdebugcontext >= MCnexecutioncontexts)
		MCdebugcontext = MCnexecutioncontexts - 1;

	MCExecPoint *t_ep_ptr;
	t_ep_ptr = MCexecutioncontexts[MCdebugcontext];

	t_ep_ptr -> setvalueref(p_script);
	ep . gethandler() -> doscript(*t_ep_ptr, p_line, p_pos);

	if (added)
		MCnexecutioncontexts--;
}

////////////////////////////////////////////////////////////////////////////////

void MCDebuggingExecBreakpoint(MCExecContext& ctxt, uint2 p_line, uint2 p_pos)
{
	MCB_break(ctxt . GetEP(), p_line, p_pos);
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
	if (MCtracestackptr == nil)
	{
		r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCtracestackptr -> names(P_NAME, r_value))
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
	sptr = MCdefaultstackptr->findstackname(MCStringGetOldString(p_value));
	
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
		MCAutoStringRef t_context_id;
		t_success = MCexecutioncontexts[MCdebugcontext]->getobj()->names(P_LONG_ID, &t_context_id) &&
					MCListAppend(*t_list, *t_context_id);
	}
	
	if (t_success)
		t_success = MCListAppend(*t_list, MCexecutioncontexts[MCdebugcontext]->gethandler()->getname());

	if (t_success)
		t_success = MCListAppendInteger(*t_list, MCexecutioncontexts[MCdebugcontext]->getline());
	
	if (t_success)
		t_success = MCListCopyAsString(*t_list, r_value);
	
	if (t_success)
		return;
	
	ctxt . Throw();
}

void MCDebuggingSetDebugContext(MCExecContext& ctxt, MCStringRef p_value)
{
	uindex_t t_length = MCStringGetLength(p_value);
	bool t_in_quotes;
	t_in_quotes = false;
	uindex_t t_offset;

	for (t_offset = 0; t_offset < t_length; t_offset++)
	{
		if (!t_in_quotes && MCStringGetNativeCharAtIndex(p_value, t_offset) == ',')
			break;

		if (MCStringGetNativeCharAtIndex(p_value, t_offset) == '"')
			t_in_quotes = !t_in_quotes;
	}

	if (t_offset < t_length)
	{
		MCAutoStringRef t_head;
		MCAutoStringRef t_tail;
		int4 t_line;
		MCObjectPtr t_object;		

		if (MCStringDivideAtIndex(p_value, t_offset, &t_head, &t_tail) &&
			MCInterfaceTryToResolveObject(ctxt, *t_head, t_object) &&
			MCU_strtol(*t_tail, t_line))
		{
			for (uint2 i = 0; i < MCnexecutioncontexts; i++)
			{
				if (MCexecutioncontexts[i] -> getobj() == t_object . object && 
					MCexecutioncontexts[i] -> getline() == t_line)
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
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt . GetEP();
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
			
			if (t_success)
			{
				MCAutoStringRef t_context_id;
				t_success = MCexecutioncontexts[i]->getobj()->names(P_LONG_ID, &t_context_id) &&
							MCListAppend(*t_context, *t_context_id);
			}
			
			if (t_success)
				t_success = MCListAppend(*t_context, MCexecutioncontexts[i]->gethandler()->getname());
			
			if (t_success)
			{
				MCAutoStringRef t_line;
				t_success = MCStringFormat(&t_line, "%d", MCexecutioncontexts[i]->getline()) &&
							MCListAppend(*t_context, *t_line);
			}
			
			if (t_success && MCexecutioncontexts[i] -> getparentscript() != NULL)
			{
				MCAutoStringRef t_parent;
				t_success = MCexecutioncontexts[i] -> getparentscript() -> GetParent() -> GetObject() -> names(P_LONG_ID, &t_parent) &&
							MCListAppend(*t_context, *t_parent);
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