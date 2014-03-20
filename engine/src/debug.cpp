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

#include "execpt.h"
#include "hndlrlst.h"
#include "handler.h"
#include "param.h"
#include "mcerror.h"
#include "object.h"
#include "stack.h"
#include "card.h"
#include "field.h"
#include "util.h"
#include "debug.h"
#include "parentscript.h"
#include "chunk.h"
#include "scriptpt.h"
#include "dispatch.h"

#include "globals.h"
#include "mode.h"

////////////////////////////////////////////////////////////////////////////////

MCExecPoint *MCEPptr;
MCStack *MCtracestackptr;
Window MCtracewindow;
Boolean MCtrace;
Boolean MCtraceabort;
MCObject *MCtracedobject;
Boolean MCtracereturn = True;
uint4 MCtraceuntil = MAXUINT2;
uint2 MCtracedelay = 500;

// MW-2004-11-17: Added to allow deletion of Message Box
MCStack *MCmbstackptr = NULL;

Breakpoint *MCbreakpoints = nil;
uint2 MCnbreakpoints = 0;

Watchvar *MCwatchedvars = nil;
uint2 MCnwatchedvars = 0;

MCExecPoint *MCexecutioncontexts[MAX_CONTEXTS];
uint2 MCnexecutioncontexts = 0;
uint2 MCdebugcontext = MAXUINT2;
Boolean MCmessagemessages = False;

static int2 depth;

////////////////////////////////////////////////////////////////////////////////

#if defined(_SERVER)

#include "srvdebug.h"

void MCB_setmsg(MCExecPoint& ep)
{
	// At some point we will add the ability to manipulate/look at the 'message box' in a
	// remote debugging session.
}

void MCB_trace(MCExecPoint &ep, uint2 line, uint2 pos)
{
	MCServerDebugTrace(ep, line, pos);
}

void MCB_break(MCExecPoint &ep, uint2 line, uint2 pos)
{
	MCServerDebugBreak(ep, line, pos);
}

bool MCB_error(MCExecPoint &ep, uint2 line, uint2 pos, uint2 id)
{
	MCServerDebugError(ep, line, pos, id);
	
	// Increasing the error lock means that more MCB_error invocations won't occur as
	// we step back up the (script) call stack.
	MCerrorlock++;

	return true;
}

void MCB_done(MCExecPoint &ep)
{
}

void MCB_setvar(MCExecPoint &ep, MCNameRef name)
{
	MCServerDebugVariableChanged(ep, name);
}

#else

void MCB_setmsg(MCExecPoint &ep)
{
	if (MCnoui)
	{
		MCS_write(ep.getsvalue().getstring(), sizeof(char),
		          ep.getsvalue().getlength(), IO_stdout);
		uint4 length = ep.getsvalue().getlength();
		if (length && ep.getsvalue().getstring()[length - 1] != '\n')
			MCS_write("\n", sizeof(char), 1, IO_stdout);
		return;
	}
	ep.grabsvalue();  // in case source variable changes while opening MB
	
	if (!MCModeHandleMessageBoxChanged(ep))
	{
		// MW-2004-11-17: Now use global 'MCmbstackptr' instead
		if (MCmbstackptr == NULL)
			MCmbstackptr = MCdispatcher->findstackname(MCmessagenamestring);
			
		if (MCmbstackptr != NULL)
		{
			Window_mode newmode = MCmbstackptr->userlevel() == 0 ? WM_MODELESS
														: (Window_mode)(MCmbstackptr->userlevel() + WM_TOP_LEVEL_LOCKED);
			
			// MW-2011-07-05: [[ Bug 9608 ]] The 'ep' that is passed through to us does
			//   not necessarily have an attached object any more. Given that the 'rel'
			//   parameter of the open stack call is unused, computing it from that
			//   context is redundent.
			if (MCmbstackptr->getmode() != newmode)
				MCmbstackptr->openrect(MCmbstackptr -> getrect(), newmode, NULL, WP_DEFAULT,OP_NONE);
			else
				MCmbstackptr->raise();
			MCCard *cptr = MCmbstackptr->getchild(CT_THIS, MCnullmcstring, CT_CARD);
			MCField *fptr = (MCField *)cptr->getchild(CT_FIRST, MCnullmcstring,
											CT_FIELD, CT_CARD);
			if (fptr != NULL)
				fptr->settext(0, ep.getsvalue(), False);
		}
	}
}

void MCB_message(MCExecPoint &ep, MCNameRef mess, MCParameter *p)
{
	Boolean exitall = MCexitall;
	MCSaveprops sp;
	MCU_saveprops(sp);
	MCU_resetprops(True);

	MCtrace = False;
	if (MCtracestackptr != NULL)
		MCtracewindow = MCtracestackptr->getw();
	else
		MCtracewindow = ep.getobj()->getw();
	MCVariable *oldresult = MCresult;
	/* UNCHECKED */ MCVariable::createwithname_cstring("MCdebugresult", MCresult);
	MCtracereturn = False;
	MCtraceabort = False;

	Boolean oldcheck;
	oldcheck = MCcheckstack;
	MCcheckstack = False;

	// OK-2008-05-19 : Bug 3915. If the object is disabled, message() will not send messages to it
	// unless the "send" parameter is specified.

	// OK-2008-11-28: [[Bug 7491]] - It seems that using the "send" parameter causes problems with the MetaCard debugger
	// So instead of doing that, I've added a new optional parameter to MCObject::send, called p_force, and used this instead.
	if (ep . getobj() -> message(mess, p, True, False, True) == ES_NORMAL)
	{
		MCcheckstack = oldcheck;
		//  if (depth++ > 1)
		//   fprintf(stderr, "Debug depth %d\n", depth);
		 while (!MCtracereturn)
		{
			MCU_resetprops(True);
			MCscreen->wait(REFRESH_INTERVAL, True, True);
		}
		//  depth--;
		if (MCtracedobject == NULL)
			MCtracedobject = ep.getobj();
		if (MCtraceabort)
		{
			MCtraceabort = False;
			exitall = True;
		}
		else
			if (MCtracestackptr != NULL)
				MCtrace = True;
	 }
	 MCcheckstack = oldcheck;
	 MCtracewindow = NULL;
	 delete MCresult;
	 MCresult = oldresult;
	 MCU_restoreprops(sp);
	 MCexitall = exitall;
}

void MCB_prepmessage(MCExecPoint &ep, MCNameRef mess, uint2 line, uint2 pos, uint2 id, const char *info)
{
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ep.setline(line);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
		added = True;
	}
	MCParameter p1, p2, p3, p4;
	p1.setnameref_unsafe_argument(ep.gethandler()->getname());
	p1.setnext(&p2);
	p2.setn_argument((real8)line);
	p2.setnext(&p3);
	p3.setn_argument((real8)pos);
	if (id != 0)
	{
		p3.setnext(&p4);
		MCeerror->add(id, line, pos);

		ep.getobj()->getprop(0, P_LONG_ID, ep, False);
		MCeerror->add(EE_OBJECT_NAME, 0, 0, ep.getsvalue());
		p4.sets_argument(MCeerror->getsvalue());
	}
	else if (info != NULL)
	{
		p3.setnext(&p4);
		p4.sets_argument(info);
	}
	MCB_message(ep, mess, &p1);
	if (id != 0)
		MCeerror->clear();
	if (added)
		MCnexecutioncontexts--;
}

void MCB_trace(MCExecPoint &ep, uint2 line, uint2 pos)
{
	uint2 i;

	if (MCtrace && (MCtraceuntil == MAXUINT2 || MCnexecutioncontexts == MCtraceuntil))
	{
		MCtraceuntil = MAXUINT2;
		MCB_prepmessage(ep, MCM_trace, line, pos, 0);
	}
	else
	{
		// MW-2009-01-28: [[ Inherited parentScripts ]]
		// If in parentScript context, we check against the parentScript's object
		// rather than the ep's object.
		for (i = 0 ; i < MCnbreakpoints ; i++)
			if (MCbreakpoints[i].line == line)
			{
				MCParentScriptUse *t_parentscript;
				t_parentscript = ep . getparentscript();
				if (t_parentscript == NULL && MCbreakpoints[i].object == ep.getobj() ||
					t_parentscript != NULL && MCbreakpoints[i].object == t_parentscript -> GetParent() -> GetObject())
				MCB_prepmessage(ep, MCM_trace_break, line, pos, 0, MCbreakpoints[i].info);
			}
	}
}

void MCB_break(MCExecPoint &ep, uint2 line, uint2 pos)
{
	MCB_prepmessage(ep, MCM_trace_break, line, pos, 0);
}

bool s_in_trace_error = false;

bool MCB_error(MCExecPoint &ep, uint2 line, uint2 pos, uint2 id)
{
	// OK-2009-03-25: [[Bug 7517]] - The crash described in this bug report is probably caused by a stack overflow. This overflow is due to
	// errors being thrown in the IDE (or in this case GLX2) component of the debugger. This should prevent traceError from recursing.
	if (s_in_trace_error)
		return false;
	
	s_in_trace_error = true;
	MCB_prepmessage(ep, MCM_trace_error, line, pos, id);
	MCerrorlock++; // suppress errors as stack unwinds
	s_in_trace_error = false;

	return true;
}

void MCB_done(MCExecPoint &ep)
{
	MCB_message(ep, MCM_trace_done, NULL);
}

void MCB_setvar(MCExecPoint &ep, MCNameRef name)
{
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
		added = True;
	}
	MCParameter p1, p2, p3;
	p1.setn_argument(ep.getline());
	p1.setnext(&p2);
	p2.setnameref_unsafe_argument(name);
	p2.setnext(&p3);
	p3.sets_argument(ep.getsvalue());
	MCB_message(ep, MCM_update_var, &p1);
	if (added)
		MCnexecutioncontexts--;
}

#endif

////////////////////////////////////////////////////////////////////////////////

void MCB_clearbreaks(MCObject *p_for_object)
{
	for(unsigned int n = 0; n < MCnbreakpoints; ++n)
		if (p_for_object == NULL || MCbreakpoints[n] . object == p_for_object)
		{
			MCbreakpoints[n] . object = NULL;
			delete MCbreakpoints[n] . info;
			MCbreakpoints[n] . info = NULL;
		}
	
	if (p_for_object == NULL)
	{
		MCnbreakpoints = 0;
		free(MCbreakpoints);
		MCbreakpoints = nil;
}
}

void MCB_unparsebreaks(MCExecPoint& ep)
{
	ep.clear();
	
	// MW-2005-06-26: Fix breakpoint crash issue - ignore any breakpoints with NULL object
	for (uint32_t i = 0 ; i < MCnbreakpoints ; i++)
		if (MCbreakpoints[i] . object != NULL)
		{
			MCExecPoint ep2(ep);
			MCbreakpoints[i].object->getprop(0, P_LONG_ID, ep2, False);
			ep.concatmcstring(ep2.getsvalue(), EC_RETURN, i == 0);
			ep.concatuint(MCbreakpoints[i] . line, EC_COMMA, false);
			if (MCbreakpoints[i].info != NULL)
				ep.concatcstring(MCbreakpoints[i].info, EC_COMMA, false);
		}
}

static MCObject *getobj(MCExecPoint& ep)
{
	MCObject *objptr = NULL;
	MCChunk *tchunk = new MCChunk(False);
	MCerrorlock++;
	MCScriptPoint sp(ep);
	if (tchunk->parse(sp, False) == PS_NORMAL)
	{
		uint4 parid;
		tchunk->getobj(ep, objptr, parid, True);
	}
	MCerrorlock--;
	delete tchunk;
	return objptr;
}

void MCB_parsebreaks(MCExecPoint& ep)
{
	MCB_clearbreaks(NULL);
	
	char *buffer = ep.getsvalue().clone();
	char *eptr = buffer;

	while ((eptr = strtok(eptr, "\n")) != NULL)
	{
		bool t_in_quote;
		t_in_quote = false;
		
		// Find the end of the long id
		char *line_ptr;
		line_ptr = eptr;
		while(*line_ptr != '\0')
		{
			if (t_in_quote)
			{
				if (*line_ptr == '"')
					t_in_quote = false;
			}
			else
			{
				if (*line_ptr == '"')
					t_in_quote = true;
				else if (*line_ptr == ',')
					break;
			}
			line_ptr++;
		}

		*line_ptr++ = '\0';
		
		char *info_ptr;
		info_ptr = strchr(line_ptr, ',');
		if (info_ptr != NULL)
			*info_ptr++;
			
		MCObject *t_object;
		ep.setsvalue(eptr);
		t_object = getobj(ep);
		
		uint32_t t_line;
		t_line = strtoul(line_ptr, NULL, 10);
		
		if (t_object != nil && t_line != 0)
		{
			Breakpoint *t_new_breakpoints;
			t_new_breakpoints = (Breakpoint *)realloc(MCbreakpoints, sizeof(Breakpoint) * (MCnbreakpoints + 1));
			if (t_new_breakpoints != nil)
			{
				MCbreakpoints = t_new_breakpoints;
				MCbreakpoints[MCnbreakpoints] . object = t_object;
				MCbreakpoints[MCnbreakpoints] . line = t_line;
				if (info_ptr != nil)
					MCbreakpoints[MCnbreakpoints] . info = strdup(info_ptr);
				else
					MCbreakpoints[MCnbreakpoints] . info = NULL;
				MCnbreakpoints++;
			}
		}
		
		eptr = NULL;
	}
	delete buffer;
}

////////////////////////////////////////////////////////////////////////////////

void MCB_clearwatches(void)
{
	while (MCnwatchedvars--)
	{
		MCNameDelete(MCwatchedvars[MCnwatchedvars].handlername);
		MCNameDelete(MCwatchedvars[MCnwatchedvars].varname);
		delete MCwatchedvars[MCnwatchedvars].expression;
	}
	MCnwatchedvars = 0;

	free(MCwatchedvars);
	MCwatchedvars = nil;
}

Exec_stat MCB_parsewatches(MCExecPoint& ep)
{
	MCB_clearwatches();

	char *buffer = ep.getsvalue().clone();
	char *eptr = buffer;
	while ((eptr = strtok(eptr, "\n")) != NULL)
	{
		char *expressptr, *vnameptr, *hnameptr;
		MCObject *objptr;

		objptr = nil;
		vnameptr = nil;
		hnameptr = nil;
		expressptr = strrchr(eptr, ',');
		
		if (expressptr != NULL)
		{
			*expressptr++ = '\0';
			vnameptr = strrchr(eptr, ',');
		}

		if (vnameptr != NULL)
		{
			*vnameptr++ = '\0';
			hnameptr = strrchr(eptr, ',');
		}

		if (hnameptr != NULL)
		{
			*hnameptr++ = '\0';
			ep.setsvalue(eptr);
			objptr = getobj(ep);
		}
	
		// OK-2010-01-14: [[Bug 6506]] - Allow globals in watchedVariables
		//   If the object and handler are empty we assume its a global, otherwise
		//   do the previous behavior.
		// MW-2010-01-19: Just tidying up a bit - notice that strclone returns nil
		//   if the input is nil, so we don't need two separate clauses.
		if (strcmp(eptr, "") == 0 && strcmp(hnameptr, "") == 0 ||
			objptr != nil)
		{
			Watchvar *t_new_watches;
			t_new_watches = (Watchvar *)realloc(MCwatchedvars, sizeof(Watchvar) * (MCnwatchedvars + 1));
			if (t_new_watches != nil)
			{
				MCwatchedvars = t_new_watches;
				MCwatchedvars[MCnwatchedvars] . object = objptr;
				if (strcmp(hnameptr, "") != 0)
					/* UNCHECKED */ MCNameCreateWithCString(hnameptr, MCwatchedvars[MCnwatchedvars] . handlername);
				else
					MCwatchedvars[MCnwatchedvars] . handlername = nil;
				/* UNCHECKED */ MCNameCreateWithCString(vnameptr, MCwatchedvars[MCnwatchedvars] . varname);
				MCwatchedvars[MCnwatchedvars] . expression = strclone(expressptr);
				MCnwatchedvars++;
			}
		}

		eptr = NULL;
	}

	delete buffer;

	return ES_NORMAL;
}

void MCB_unparsewatches(MCExecPoint& ep)
{
	ep.clear();
	for (uint32_t i = 0 ; i < MCnwatchedvars ; i++)
	{
		MCExecPoint ep2(ep);
		ep2 . clear();

		// OK-2010-01-14: [[Bug 6506]] - WatchedVariables support for globals
		if (MCwatchedvars[i] . object != NULL)
			MCwatchedvars[i].object->getprop(0, P_LONG_ID, ep2, False);
		
		ep.concatmcstring(ep2.getsvalue(), EC_RETURN, i == 0);

		if (MCwatchedvars[i] . handlername == NULL)
			ep . concatcstring("", EC_COMMA, false);
		else
			ep . concatnameref(MCwatchedvars[i].handlername, EC_COMMA, false);

		ep.concatnameref(MCwatchedvars[i].varname, EC_COMMA, false);
		if (MCwatchedvars[i].expression != NULL)
			ep.concatcstring(MCwatchedvars[i].expression, EC_COMMA, false);
	}
}

