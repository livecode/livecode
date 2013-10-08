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
#include "exec.h"

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

void MCB_setmsg(MCStringRef p_msg)
{
    
}

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

void MCB_error(MCExecPoint &ep, uint2 line, uint2 pos, uint2 id)
{
	MCServerDebugError(ep, line, pos, id);
	
	// Increasing the error lock means that more MCB_error invocations won't occur as
	// we step back up the (script) call stack.
	MCerrorlock++;
}

void MCB_done(MCExecPoint &ep)
{
}

void MCB_setvar(MCExecPoint &ep, MCNameRef name)
{
	MCServerDebugVariableChanged(ep, name);
}

void MCB_setvar(MCExecContext &ctxt, MCValueRef p_value, MCNameRef name)
{
    
}

#else

void MCB_setmsg(MCExecContext &ctxt, MCStringRef p_msg)
{
    ctxt . GetEP() . setvalueref(p_msg);
    MCB_setmsg(ctxt . GetEP());
}

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
			MCCard *cptr = MCmbstackptr->getchild(CT_THIS, kMCEmptyString, CT_CARD);
			MCField *fptr = (MCField *)cptr->getchild(CT_FIRST, kMCEmptyString, CT_FIELD, CT_CARD);
			if (fptr != NULL)
			{
				MCAutoStringRef t_string;
				ep . copyasstringref(&t_string);
				fptr->settext(0, *t_string, False);
			}
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
	/* UNCHECKED */ MCVariable::createwithname(MCNAME("MCdebugresult"), MCresult);
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

void MCB_prepmessage(MCExecPoint &ep, MCNameRef mess, uint2 line, uint2 pos, uint2 id, MCStringRef p_info)
{
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ep.setline(line);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
		added = True;
	}
	MCParameter p1, p2, p3, p4;
	p1.setvalueref_argument(ep.gethandler()->getname());
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
	else if (!MCStringIsEmpty(p_info))
	{
		p3.setnext(&p4);
		p4.setvalueref_argument(p_info);
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

void MCB_error(MCExecPoint &ep, uint2 line, uint2 pos, uint2 id)
{
	// OK-2009-03-25: [[Bug 7517]] - The crash described in this bug report is probably caused by a stack overflow. This overflow is due to
	// errors being thrown in the IDE (or in this case GLX2) component of the debugger. This should prevent traceError from recursing.
	if (s_in_trace_error)
		return;
	
	s_in_trace_error = true;
	MCB_prepmessage(ep, MCM_trace_error, line, pos, id);
	MCerrorlock++; // suppress errors as stack unwinds
	s_in_trace_error = false;
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
	p2.setvalueref_argument(name);
	p2.setnext(&p3);
	p3.sets_argument(ep.getsvalue());
	MCB_message(ep, MCM_update_var, &p1);
	if (added)
		MCnexecutioncontexts--;
}

void MCB_setvar(MCExecContext &ctxt, MCValueRef p_value, MCNameRef name)
{
    MCExecPoint& ep = ctxt . GetEP();
    
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
		added = True;
	}
	MCParameter p1, p2, p3;
	p1.setn_argument(ep.getline());
	p1.setnext(&p2);
	p2.setvalueref_argument(name);
	p2.setnext(&p3);
    
    MCValueRef t_value;
    MCValueCopy(p_value, t_value);

	p3.setvalueref_argument(t_value);
    
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
			MCValueAssign(MCbreakpoints[n] . info, kMCEmptyString);
		}
	
	if (p_for_object == NULL)
	{
		MCnbreakpoints = 0;
		free(MCbreakpoints);
		MCbreakpoints = nil;
	}
}

bool MCB_unparsebreaks(MCStringRef& r_value)
{
	bool t_success;
	t_success = true;

	MCAutoListRef t_breakpoint_list;
	if (t_success)
		t_success = MCListCreateMutable('\n', &t_breakpoint_list);

	if (t_success)
	{
		// MW-2005-06-26: Fix breakpoint crash issue - ignore any breakpoints with NULL object
		for (uint32_t i = 0 ; i < MCnbreakpoints ; i++)
		{
			if (MCbreakpoints[i] . object != NULL)
			{
				MCAutoListRef t_breakpoint;
				t_success = MCListCreateMutable(',', &t_breakpoint);
				
				if (t_success)
				{
					MCAutoStringRef t_breakpoint_id;
					t_success = MCbreakpoints[i] . object -> names(P_LONG_ID, &t_breakpoint_id) &&
								MCListAppend(*t_breakpoint, *t_breakpoint_id);
				}
							
				if (t_success)
				{
					MCAutoStringRef t_line;
					t_success = MCStringFormat(&t_line, "%d", MCbreakpoints[i] . line) &&
								MCListAppend(*t_breakpoint, *t_line);
				}
				
				if (t_success && !MCStringIsEmpty(MCbreakpoints[i] . info))
				{
					t_success = MCListAppend(*t_breakpoint, MCbreakpoints[i] . info);
				}

				if (t_success)
					t_success = MCListAppend(*t_breakpoint_list, *t_breakpoint);
			}
		}
	}
	if (t_success)
		t_success = MCListCopyAsString(*t_breakpoint_list, r_value);

	return t_success;
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
			if (!MCStringIsEmpty(MCbreakpoints[i].info))
				ep.concatstringref(MCbreakpoints[i].info, EC_COMMA, false);
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

void MCB_parsebreaks(MCExecContext& ctxt, MCStringRef p_input)
{
	MCB_clearbreaks(NULL);

	uindex_t t_return_offset = 0;
	uindex_t t_last_offset = 0;
	uindex_t t_input_length;

	t_input_length = MCStringGetLength(p_input);
	
	bool t_found;
	t_found = true;
	
	bool t_success;
	t_success = true;

	while (t_found && t_success)
	{
		uindex_t t_length;
		MCAutoStringRef t_break;
		t_found = MCStringFirstIndexOfChar(p_input, '\n', t_last_offset, kMCCompareCaseless, t_return_offset);

		if (!t_found) //last line
			t_length = t_input_length - t_last_offset;
		else
			t_length = t_return_offset - t_last_offset;

		t_success = MCStringCopySubstring(p_input, MCRangeMake(t_last_offset, t_length), &t_break);

		bool t_in_quotes;
		t_in_quotes = false;
		uindex_t t_offset;

		if (t_success)
		{
			for (t_offset = 0; t_offset < t_length; t_offset++)
			{
				if (!t_in_quotes && MCStringGetCharAtIndex(*t_break, t_offset) == ',')
					break;

				if (MCStringGetCharAtIndex(*t_break, t_offset) == '"')
					t_in_quotes = !t_in_quotes;
			}
		}

		if (t_offset < t_length)
		{
			MCAutoStringRef t_head;
			MCAutoStringRef t_tail;
			MCObjectPtr t_object;		

			if (t_success)
				t_success = MCStringDivideAtIndex(*t_break, t_offset, &t_head, &t_tail);
			
			if (t_success)
				t_success = MCInterfaceTryToResolveObject(ctxt, *t_head, t_object);

			MCAutoStringRef t_line_string;
			MCAutoStringRef t_info;

			if (t_success)
				t_success = MCStringDivideAtChar(*t_tail, ',', kMCCompareCaseless, &t_line_string, &t_info);
			
			int32_t t_line;

			if (t_success)
				t_success = MCU_strtol(*t_line_string, t_line);

			if (t_success && t_line > 0)
			{
				Breakpoint *t_new_breakpoints;
				t_new_breakpoints = (Breakpoint *)realloc(MCbreakpoints, sizeof(Breakpoint) * (MCnbreakpoints + 1));
				if (t_new_breakpoints != nil)
				{
					MCbreakpoints = t_new_breakpoints;
					MCbreakpoints[MCnbreakpoints] . object = t_object . object;
					MCbreakpoints[MCnbreakpoints] . line = t_line;
					MCbreakpoints[MCnbreakpoints] . info = MCValueRetain(*t_info);
					MCnbreakpoints++;
				}
			}
		}
		t_last_offset = t_return_offset + 1;
	}
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
				MCStringRef t_info;
				if (info_ptr != nil)
					/* UNCHECKED */ MCStringCreateWithCString(info_ptr, t_info);
				else
					t_info = MCValueRetain(kMCEmptyString);
				MCbreakpoints[MCnbreakpoints] . info = t_info;
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

void MCB_parsewatches(MCExecContext& ctxt, MCStringRef p_input)
{
	MCB_clearwatches();

	uindex_t t_return_offset = 0;
	uindex_t t_last_offset = 0;
	uindex_t t_input_length;

	t_input_length = MCStringGetLength(p_input);
	bool t_found;
	t_found = true;

	bool t_success;
	t_success = true;

	while (t_found && t_success)
	{
		uindex_t t_length;
		MCAutoStringRef t_watch;
		t_found = MCStringFirstIndexOfChar(p_input, '\n', t_last_offset, kMCCompareCaseless, t_return_offset);

		if (!t_found) //last line
			t_length = t_input_length - t_last_offset;
		else
			t_length = t_return_offset - t_last_offset;

		t_success = MCStringCopySubstring(p_input, MCRangeMake(t_last_offset, t_length), &t_watch);

		MCAutoStringRef t_obj;
		MCAutoStringRef t_obj_tail;

		if (t_success)
			t_success = MCStringDivideAtChar(*t_watch, ',', kMCCompareCaseless, &t_obj, &t_obj_tail);

		MCAutoStringRef t_express;
		MCAutoStringRef t_express_tail;

		if (t_success)
			t_success = MCStringDivideAtChar(*t_obj_tail, ',', kMCCompareCaseless, &t_express, &t_express_tail);

		MCAutoStringRef t_vname;
		MCAutoStringRef t_hname;

		if (t_success)
			t_success = MCStringDivideAtChar(*t_express_tail, ',', kMCCompareCaseless, &t_vname, &t_hname);

		MCObjectPtr t_object;

		if (t_success)
		{
			MCInterfaceTryToResolveObject(ctxt, *t_obj, t_object);

			// OK-2010-01-14: [[Bug 6506]] - Allow globals in watchedVariables
			//   If the object and handler are empty we assume its a global, otherwise
			//   do the previous behavior.

			if (MCStringGetLength(*t_obj) == 0 && MCStringGetLength(*t_hname) == 0 ||
				t_object . object != nil)
			{
				Watchvar *t_new_watches;
				t_new_watches = (Watchvar *)realloc(MCwatchedvars, sizeof(Watchvar) * (MCnwatchedvars + 1));
				if (t_new_watches != nil)
				{
					MCwatchedvars = t_new_watches;
					MCwatchedvars[MCnwatchedvars] . object = t_object . object;
					if (MCStringGetLength(*t_hname) != 0)
						/* UNCHECKED */ MCNameCreate(*t_hname, MCwatchedvars[MCnwatchedvars] . handlername);
					else
						MCwatchedvars[MCnwatchedvars] . handlername = nil;
					/* UNCHECKED */ MCNameCreate(*t_vname, MCwatchedvars[MCnwatchedvars] . varname);
					MCwatchedvars[MCnwatchedvars] . expression = strclone(MCStringGetCString(*t_express));
					MCnwatchedvars++;
				}
			}
		}
		t_last_offset = t_return_offset + 1;
	}
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

bool MCB_unparsewatches(MCStringRef &r_watches)
{
	bool t_success;
	t_success = true;

	MCAutoListRef t_watches_list;
	if (t_success)
		t_success = MCListCreateMutable('\n', &t_watches_list);

	if (t_success)
	{
		for (uint32_t i = 0 ; i < MCnwatchedvars ; i++)
		{		
		// OK-2010-01-14: [[Bug 6506]] - WatchedVariables support for globals
			if (MCwatchedvars[i] . object != NULL)
			{
				MCAutoListRef t_watched_var;
				t_success = MCListCreateMutable(',', &t_watched_var);
				
				if (t_success)
				{
					MCAutoStringRef t_var_id;
					t_success = MCwatchedvars[i] . object -> names(P_LONG_ID, &t_var_id) &&
								MCListAppend(*t_watched_var, *t_var_id);
				}
							
				if (t_success)
				{
					if (MCwatchedvars[i] . handlername == NULL)
						t_success = MCListAppend(*t_watched_var, kMCEmptyString);
					else
						t_success = MCListAppend(*t_watched_var, MCwatchedvars[i].handlername);
				}
				
				if (t_success)
					t_success = MCListAppend(*t_watched_var, MCwatchedvars[i].varname);

				if (t_success && MCwatchedvars[i].expression != NULL)
				{
					MCAutoStringRef t_expression;
					t_success = MCStringCreateWithCString(MCwatchedvars[i].expression, &t_expression) &&
								MCListAppend(*t_watched_var, *t_expression);
				}

				if (t_success)
					t_success = MCListAppend(*t_watches_list, *t_watched_var);
			}
		}
	}

	if (t_success)
		t_success = MCListCopyAsString(*t_watches_list, r_watches);

	return t_success;
	
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

