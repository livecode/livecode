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
// script debugger functions
//

// MW-2009-11-03: Add an 'info' field to the breakpoints
struct Breakpoint
{
	MCObject *object;
	uint4 line;
	char *info;
};

// set the breakpoints to "button 1, 3"

struct Watchvar
{
	MCObject *object;
	MCNameRef handlername;
	MCNameRef varname;
	char *expression;
};

// set the watchedvariables to "button 1, somehandler, somevar, someexp"

#define MAX_CONTEXTS 100

extern MCExecPoint *MCEPptr;
extern MCStack *MCtracestackptr;
extern Window MCtracewindow;
extern Boolean MCtrace;
extern Boolean MCtraceabort;
extern Boolean MCtracereturn;
extern MCObject *MCtracedobject;
extern uint2 MCtracedelay;
extern uint4 MCtraceuntil;

// MW-2009-11-03: Make the breakpoints array dynamic
extern uint2 MCnbreakpoints;
extern Breakpoint *MCbreakpoints;

extern uint2 MCnwatchedvars;
extern Watchvar *MCwatchedvars;

extern MCExecPoint *MCexecutioncontexts[MAX_CONTEXTS];
extern uint2 MCnexecutioncontexts;
extern uint2 MCdebugcontext;
extern Boolean MCmessagemessages;

extern void MCB_setmsg(MCExecPoint &ep);
extern void MCB_message(MCExecPoint &ep, MCNameRef message, MCParameter *p);
extern void MCB_prepmessage(MCExecPoint &ep, MCNameRef message, uint2 line, uint2 pos, uint2 id, const char *info = NULL);
extern void MCB_break(MCExecPoint &ep, uint2 line, uint2 pos);
extern void MCB_trace(MCExecPoint &ep, uint2 line, uint2 pos);
extern bool MCB_error(MCExecPoint &ep, uint2 line, uint2 pos, uint2 id);
extern void MCB_done(MCExecPoint &ep);
extern void MCB_setvar(MCExecPoint &ep, MCNameRef name);

extern void MCB_parsebreaks(MCExecPoint& breaks);
extern void MCB_unparsebreaks(MCExecPoint& breaks);
extern void MCB_clearbreaks(MCObject *object);

extern Exec_stat MCB_parsewatches(MCExecPoint& watches);
extern void MCB_unparsewatches(MCExecPoint& watches);
extern void MCB_clearwatches(void);
