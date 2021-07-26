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

#ifndef __MC_DEBUG_H__
#define __MC_DEBUG_H__

#include "object.h"
#include "stack.h"

//
// script debugger functions
//

// MW-2009-11-03: Add an 'info' field to the breakpoints
struct Breakpoint
{
	Breakpoint(MCObjectHandle p_object, uint32_t p_line, MCStringRef p_info) :
      object(p_object),
      line(p_line),
      info(p_info)
    {
    }
    
    void Clear()
    {
        object = nil;
        line = 0;
        info.Reset();
    }
    
    MCObjectHandle object;
	uint4 line;
	MCAutoStringRef info;
};

// set the breakpoints to "button 1, 3"

struct Watchvar
{
	Watchvar(MCObjectHandle p_object, MCNameRef p_handlername, MCNameRef p_varname, MCStringRef p_expression) :
      object(p_object),
      handlername(p_handlername),
      varname(p_varname),
      expression(p_expression)
    {
    }
    
    void Clear()
    {
        object = nil;
        handlername.Reset();
        varname.Reset();
        expression.Reset();
    }
    
    MCObjectHandle object;
	MCNewAutoNameRef handlername;
	MCNewAutoNameRef varname;
	MCAutoStringRef expression;
};

// set the watchedvariables to "button 1, somehandler, somevar, someexp"

#define MAX_CONTEXTS 100

extern MCExecContext *MCECptr;
extern MCStackHandle MCtracestackptr;
extern Window MCtracewindow;
extern Boolean MCtrace;
extern Boolean MCtraceabort;
extern Boolean MCtracereturn;
extern MCObjectHandle MCtracedobject;
extern uint2 MCtracedelay;
extern uint4 MCtraceuntil;

// MW-2009-11-03: Make the breakpoints array dynamic
extern uint2 MCnbreakpoints;
extern Breakpoint *MCbreakpoints;

extern uint2 MCnwatchedvars;
extern Watchvar *MCwatchedvars;

extern MCExecContext *MCexecutioncontexts[MAX_CONTEXTS];
extern uint2 MCnexecutioncontexts;
extern uint2 MCdebugcontext;
extern Boolean MCmessagemessages;
extern MCNameRef MClogmessage;

struct MCExecValue;

extern void MCB_setmsg(MCExecContext &ctxt, MCStringRef p_string);
extern void MCB_message(MCExecContext &ctxt, MCNameRef message, MCParameter *p);
extern void MCB_prepmessage(MCExecContext &ctxt, MCNameRef message, uint2 line, uint2 pos, uint2 id, MCStringRef p_info = kMCEmptyString);
extern void MCB_break(MCExecContext &ctxt, uint2 line, uint2 pos);
extern void MCB_trace(MCExecContext &ctxt, uint2 line, uint2 pos);
extern bool MCB_error(MCExecContext &ctxt, uint2 line, uint2 pos, uint2 id);
extern void MCB_done(MCExecContext &ctxt);
extern void MCB_setvar(MCExecContext &ctxt, MCValueRef p_value, MCNameRef name);
extern void MCB_setvalue(MCExecContext &ctxt, MCExecValue p_value, MCNameRef name);

extern void MCB_parsebreaks(MCExecContext& ctxt, MCStringRef p_input);
extern bool MCB_unparsebreaks(MCStringRef& r_value);
extern void MCB_clearbreaks(MCObject *object);

extern void MCB_parsewatches(MCExecContext& ctxt, MCStringRef p_input);
extern bool MCB_unparsewatches(MCStringRef &r_watches);
extern void MCB_clearwatches(void);

#endif
