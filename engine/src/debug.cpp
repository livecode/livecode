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
#include "system.h"

////////////////////////////////////////////////////////////////////////////////

MCExecContext *MCECptr;
MCStackHandle MCtracestackptr;
Window MCtracewindow;
Boolean MCtrace;
Boolean MCtraceabort;
MCObjectHandle MCtracedobject;
Boolean MCtracereturn = True;
uint4 MCtraceuntil = MAXUINT2;
uint2 MCtracedelay = 500;

MCStackHandle MCmbstackptr = nil;

Breakpoint *MCbreakpoints = nil;
uint2 MCnbreakpoints = 0;

Watchvar *MCwatchedvars = nil;
uint2 MCnwatchedvars = 0;

MCExecContext *MCexecutioncontexts[MAX_CONTEXTS];
uint2 MCnexecutioncontexts = 0;
uint2 MCdebugcontext = MAXUINT2;
Boolean MCmessagemessages = False;
MCNameRef MClogmessage;

////////////////////////////////////////////////////////////////////////////////

#if defined(_SERVER)

#include "srvdebug.h"

void MCB_setmsg(MCExecContext& ctxt, MCStringRef p_string)
{
	// At some point we will add the ability to manipulate/look at the 'message box' in a
	// remote debugging session.
}

void MCB_trace(MCExecContext &ctxt, uint2 line, uint2 pos)
{
	MCServerDebugTrace(ctxt, line, pos);
}

void MCB_break(MCExecContext &ctxt, uint2 line, uint2 pos)
{
	MCServerDebugBreak(ctxt, line, pos);
}

bool MCB_error(MCExecContext &ctxt, uint2 line, uint2 pos, uint2 id)
{
	MCServerDebugError(ctxt, line, pos, id);
	
	// Increasing the error lock means that more MCB_error invocations won't occur as
	// we step back up the (script) call stack.
	MCerrorlock++;

	return true;
}

void MCB_done(MCExecContext &ctxt)
{
}

void MCB_setvar(MCExecContext &ctxt, MCValueRef p_value, MCNameRef name)
{
	MCServerDebugVariableChanged(ctxt, name);
}



void MCB_setvalue(MCExecContext &ctxt, MCExecValue p_value, MCNameRef name)
{
    if (!MCExecTypeIsValueRef(p_value . type))
    {
        MCAutoValueRef t_value;
        MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeValueRef, &(&t_value));
        MCB_setvar(ctxt, *t_value, name);
    }
    else
    {
        MCB_setvar(ctxt, p_value . valueref_value, name);
    }
}

#else

void MCB_setmsg(MCExecContext &ctxt, MCStringRef p_string)
{
    Exec_stat t_stat = ES_NOT_HANDLED;
    
    MCObject *t_target = nil;
    if (ctxt.GetObject() != nil)
        t_target = ctxt.GetObject();
    else if (MCdefaultstackptr . IsValid())
        t_target = MCdefaultstackptr;
    
    if (t_target != nil)
    {
		Boolean oldlock = MClockmessages;
		MClockmessages = False;

        MCAutoStringRef t_handler;
        t_handler = MCNameGetString(ctxt.GetHandler()->getname());
        MCParameter t_handler_parameter;
        t_handler_parameter.setvalueref_argument(*t_handler);
        
        MCAutoNumberRef t_line;
        MCParameter t_line_parameter;
        if (MCNumberCreateWithUnsignedInteger(ctxt.GetLine(), &t_line))
        {
            t_line_parameter.setvalueref_argument(*t_line);
            t_handler_parameter.setnext(&t_line_parameter);
        }
        
        bool t_added = false;
        if (MCnexecutioncontexts < MAX_CONTEXTS && ctxt.GetObject() != nil)
        {
            MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
            t_added = true;
        }
        
        t_stat = t_target -> message(MCM_msgchanged, &t_handler_parameter, True, True, False);
        
        if (t_added)
            MCnexecutioncontexts--;

		MClockmessages = oldlock;
    }
    
    if (t_stat == ES_NOT_HANDLED || t_stat == ES_PASS)
    {
        if (MCnoui)
        {
            MCAutoStringRefAsCString t_output;
            /* UNCHECKED */ t_output . Lock(p_string);
            MCS_write(*t_output, sizeof(char), strlen(*t_output), IO_stdout);
            uint4 length = MCStringGetLength(p_string);
            if (length && MCStringGetCharAtIndex(p_string, length - 1) != '\n')
                MCS_write("\n", sizeof(char), 1, IO_stdout);
            return;
        }
        else
        {
            MCsystem -> Debug(p_string);
        }
    }
    
}

void MCB_message(MCExecContext &ctxt, MCNameRef mess, MCParameter *p)
{
	Boolean exitall = MCexitall;
	MCSaveprops sp;
	MCU_saveprops(sp);
	MCU_resetprops(True);

	MCtrace = False;
	if (MCtracestackptr)
		MCtracewindow = MCtracestackptr->getw();
	else
		MCtracewindow = ctxt.GetObject()->getw();
	MCVariable *oldresult = MCresult;
	/* UNCHECKED */ MCVariable::createwithname(MCNAME("MCdebugresult"), MCresult);
	MCtracereturn = False;
	MCtraceabort = False;
    
    MCExecResultMode t_oldresultmode = MCresultmode;

	Boolean oldcheck;
	oldcheck = MCcheckstack;
	MCcheckstack = False;

	// OK-2008-05-19 : Bug 3915. If the object is disabled, message() will not send messages to it
	// unless the "send" parameter is specified.

	// OK-2008-11-28: [[Bug 7491]] - It seems that using the "send" parameter causes problems with the MetaCard debugger
	// So instead of doing that, I've added a new optional parameter to MCObject::send, called p_force, and used this instead.
	if (ctxt.GetObject() -> message(mess, p, True, False, True) == ES_NORMAL)
	{
		MCcheckstack = oldcheck;
		
        while (!MCtracereturn)
		{
			MCU_resetprops(True);
			MCscreen->wait(REFRESH_INTERVAL, True, True);
		}
		
        if (!MCtracedobject)
			MCtracedobject = ctxt.GetObject();
        
		if (MCtraceabort)
		{
			MCtraceabort = False;
			exitall = True;
		}
		else
			if (MCtracestackptr)
				MCtrace = True;
	 }
	 MCcheckstack = oldcheck;
	 MCtracewindow = NULL;
	 delete MCresult;
	 MCresult = oldresult;
	 MCU_restoreprops(sp);
	 MCexitall = exitall;
     MCresultmode = t_oldresultmode;

}

void MCB_prepmessage(MCExecContext &ctxt, MCNameRef mess, uint2 line, uint2 pos, uint2 id, MCStringRef p_info)
{
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ctxt.SetLineAndPos(line, pos);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
		added = True;
	}
	MCParameter p1, p2, p3, p4;
	p1.setvalueref_argument(ctxt.GetHandler()->getname());
	p1.setnext(&p2);
	p2.setn_argument((real8)line);
	p2.setnext(&p3);
	p3.setn_argument((real8)pos);
	if (id != 0)
	{
		p3.setnext(&p4);
		MCeerror->add(id, line, pos);

		MCAutoValueRef t_val;
		ctxt.GetObject()->names(P_LONG_ID, &t_val);
		MCeerror->add(EE_OBJECT_NAME, 0, 0, *t_val);

        MCAutoStringRef t_error;
        MCeerror -> copyasstringref(&t_error);
        p4.setvalueref_argument(*t_error);
	}
	else if (!MCStringIsEmpty(p_info))
	{
		p3.setnext(&p4);
		p4.setvalueref_argument(p_info);
	}
    MCDeletedObjectsFreezePool();
    MCB_message(ctxt, mess, &p1);
    MCDeletedObjectsThawPool();
	if (id != 0)
		MCeerror->clear();
	if (added)
		MCnexecutioncontexts--;
}

void MCB_trace(MCExecContext &ctxt, uint2 line, uint2 pos)
{
	uint2 i;
    
    // MW-2015-03-03: [[ Bug 13110 ]] If this is an internal handler as a result of do
    //   then *don't* debug it.
    if (ctxt . GetHandler() -> getname() == MCM_message)
        return;
	
    if (MCtrace && (MCtraceuntil == MAXUINT2 || MCnexecutioncontexts == MCtraceuntil))
	{
		MCtraceuntil = MAXUINT2;
		MCB_prepmessage(ctxt, MCM_trace, line, pos, 0);
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
				t_parentscript = ctxt . GetParentScript();
				if ((t_parentscript == NULL && MCbreakpoints[i].object == ctxt.GetObject()) ||
					(t_parentscript != NULL && MCbreakpoints[i].object == t_parentscript -> GetParent() -> GetObject()))
                    MCB_prepmessage(ctxt, MCM_trace_break, line, pos, 0, *MCbreakpoints[i].info);
			}
	}
}

void MCB_break(MCExecContext &ctxt, uint2 line, uint2 pos)
{
    // We hit a breakpoint - end all modal loops
    MCscreen->breakModalLoops();
    
    MCB_prepmessage(ctxt, MCM_trace_break, line, pos, 0);
}

bool s_in_trace_error = false;

bool MCB_error(MCExecContext &ctxt, uint2 line, uint2 pos, uint2 id)
{
    // An unhandled error has been thrown - end all modal loops
    MCscreen->breakModalLoops();
    
    // OK-2009-03-25: [[Bug 7517]] - The crash described in this bug report is probably caused by a stack overflow. This overflow is due to
	// errors being thrown in the IDE (or in this case GLX2) component of the debugger. This should prevent traceError from recursing.
	if (s_in_trace_error)
		return false;
	
	s_in_trace_error = true;
	MCB_prepmessage(ctxt, MCM_trace_error, line, pos, id);
	MCerrorlock++; // suppress errors as stack unwinds
	s_in_trace_error = false;
    return true;
}

void MCB_done(MCExecContext &ctxt)
{
	MCB_message(ctxt, MCM_trace_done, NULL);
}

void MCB_setvar(MCExecContext &ctxt, MCValueRef p_value, MCNameRef name)
{
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
		added = True;
	}

	MCParameter p1, p2, p3;
    p1.setn_argument(ctxt . GetLine());
	p1.setnext(&p2);
	p2.setvalueref_argument(name);
	p2.setnext(&p3);
	p3.setvalueref_argument(p_value);
	MCB_message(ctxt, MCM_update_var, &p1);

	if (added)
		MCnexecutioncontexts--;
}

void MCB_setvalue(MCExecContext &ctxt, MCExecValue p_value, MCNameRef name)
{
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
		added = True;
	}
    
	MCParameter p1, p2, p3;
    MCExecValue t_copy;
    p1.setn_argument(ctxt . GetLine());
	p1.setnext(&p2);
	p2.setvalueref_argument(name);
	p2.setnext(&p3);
    MCExecTypeCopy(p_value, t_copy);
	p3.give_exec_argument(t_copy);
	MCB_message(ctxt, MCM_update_var, &p1);
    
	if (added)
		MCnexecutioncontexts--;
}

#endif

////////////////////////////////////////////////////////////////////////////////

void MCB_clearbreaks(MCObject *p_for_object)
{
	for(size_t n = 0; n < MCnbreakpoints; ++n)
    {
		if (p_for_object == nil || !MCbreakpoints[n].object || MCbreakpoints[n].object == p_for_object)
		{
			MCbreakpoints[n].Clear();
		}
    }
	
	if (p_for_object == nil && MCbreakpoints != nil)
	{
		MCnbreakpoints = 0;
		MCMemoryDestroy(MCbreakpoints);
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
			if (MCbreakpoints[i].object)
			{
				MCAutoListRef t_breakpoint;
				t_success = MCListCreateMutable(',', &t_breakpoint);
				
				if (t_success)
				{
					MCAutoValueRef t_breakpoint_id;
					t_success = MCbreakpoints[i] . object -> names(P_LONG_ID, &t_breakpoint_id) &&
								MCListAppend(*t_breakpoint, *t_breakpoint_id);
				}
							
				if (t_success)
				{
					MCAutoStringRef t_line;
					t_success = MCStringFormat(&t_line, "%d", MCbreakpoints[i] . line) &&
								MCListAppend(*t_breakpoint, *t_line);
				}
				
				if (t_success && !MCStringIsEmpty(*MCbreakpoints[i].info))
				{
					t_success = MCListAppend(*t_breakpoint, *MCbreakpoints[i].info);
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
		t_found = MCStringFirstIndexOfChar(p_input, '\n', t_last_offset, kMCCompareExact, t_return_offset);

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

		if (t_success && t_offset < t_length)
		{
			MCAutoStringRef t_head;
			MCAutoStringRef t_tail;
			MCObjectPtr t_object;		

			if (t_success)
				t_success = MCStringDivideAtIndex(*t_break, t_offset, &t_head, &t_tail);

			MCAutoStringRef t_line_string;
			MCAutoStringRef t_info;

			if (t_success)
				t_success = MCStringDivideAtChar(*t_tail, ',', kMCCompareExact, &t_line_string, &t_info);
			
            // AL-2015-07-31: [[ Bug 15822 ]] Don't abort parsing if a given line is not correctly formatted
            bool t_valid_break;
            t_valid_break = t_success;
            
            if (t_valid_break)
                t_valid_break = MCInterfaceTryToResolveObject(ctxt, *t_head, t_object);
            
			int32_t t_line;
            t_line = 0;
            
			if (t_valid_break)
                t_valid_break = MCU_strtol(*t_line_string, t_line) && t_line > 0;
            
            if (t_valid_break)
			{
				Breakpoint *t_new_breakpoints;
                if (!MCMemoryReallocate(MCbreakpoints, sizeof(Breakpoint) * (MCnbreakpoints + 1), t_new_breakpoints))
                    break;
                
                MCbreakpoints = t_new_breakpoints;
                new (&MCbreakpoints[MCnbreakpoints]) Breakpoint(t_object.object->GetHandle(), t_line, *t_info);
                MCnbreakpoints++;
			}
		}
		t_last_offset = t_return_offset + 1;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCB_clearwatches(void)
{
	for (size_t i = 0; i < MCnwatchedvars; i++)
	{
		MCwatchedvars[i].Clear();
	}

    if (MCwatchedvars != nil)
    {
        MCnwatchedvars = 0;
        MCMemoryDestroy(MCwatchedvars);
        MCwatchedvars = nil;
    }
}

void MCB_parsewatches(MCExecContext& ctxt, MCStringRef p_input)
{
	MCB_clearwatches();

	uindex_t t_return_offset = 0;
	uindex_t t_last_offset = 0;
	uindex_t t_input_length;

	t_input_length = MCStringGetLength(p_input);
	
	if (t_input_length == 0)
		return;
	
	bool t_found;
	t_found = true;

	bool t_success;
	t_success = true;

	while (t_found && t_success)
	{
		uindex_t t_length;
		MCAutoStringRef t_watch;
		t_found = MCStringFirstIndexOfChar(p_input, '\n', t_last_offset, kMCCompareExact, t_return_offset);

		if (!t_found) //last line
			t_length = t_input_length - t_last_offset;
		else
			t_length = t_return_offset - t_last_offset;

		t_success = MCStringCopySubstring(p_input, MCRangeMake(t_last_offset, t_length), &t_watch);

		MCAutoStringRef t_obj;
		MCAutoStringRef t_obj_tail;

		if (t_success)
			t_success = MCStringDivideAtChar(*t_watch, ',', kMCCompareExact, &t_obj, &t_obj_tail);

        // SN-2014-09-18: [[ Bug 13453 ]] The input is object, handler, variable, condition
		MCAutoStringRef t_hname;
		MCAutoStringRef t_hname_tail;

		if (t_success)
			t_success = MCStringDivideAtChar(*t_obj_tail, ',', kMCCompareExact, &t_hname, &t_hname_tail);

		MCAutoStringRef t_vname;
		MCAutoStringRef t_express;

		if (t_success)
			t_success = MCStringDivideAtChar(*t_hname_tail, ',', kMCCompareExact, &t_vname, &t_express);

        MCObjectPtr t_object;
		bool t_resolved_object;
		if (t_success)
			t_resolved_object = MCInterfaceTryToResolveObject(ctxt, *t_obj, t_object);
        
        if (t_success)
        {
			// OK-2010-01-14: [[Bug 6506]] - Allow globals in watchedVariables
			//   If the object and handler are empty we assume its a global, otherwise
			//   do the previous behavior.

			if ((MCStringIsEmpty(*t_obj) && MCStringIsEmpty(*t_hname)) ||
				t_object . object != nil)
			{
				Watchvar *t_new_watches;
                if (!MCMemoryReallocate(MCwatchedvars, sizeof(Watchvar) * (MCnwatchedvars + 1), t_new_watches))
                {
                    t_success = false;
                    break;
                }
                
                MCObjectHandle t_objecthandle;
                MCNewAutoNameRef t_handlername;
                MCNewAutoNameRef t_varname;
                
                if (t_resolved_object)
                    t_objecthandle = t_object.object;
                
                /* UNCHECKED */ MCNameCreate(*t_hname, &t_handlername);
                /* UNCHECKED */ MCNameCreate(*t_vname, &t_varname);
                
                MCwatchedvars = t_new_watches;
                new (&MCwatchedvars[MCnwatchedvars]) Watchvar(t_objecthandle, *t_handlername, *t_varname, *t_express);
                MCnwatchedvars++;
			}
		}
		t_last_offset = t_return_offset + 1;
	}
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
			MCAutoListRef t_watched_var;
			t_success = MCListCreateMutable(',', &t_watched_var);
			
			if (t_success)
			{
				if (MCwatchedvars[i].object)
				{
					MCAutoValueRef t_var_id;
					t_success = MCwatchedvars[i] . object -> names(P_LONG_ID, &t_var_id) &&
							MCListAppend(*t_watched_var, *t_var_id);
				}
				else
					t_success = MCListAppend(*t_watched_var, kMCEmptyString);
			}
						
			if (t_success)
			{
				if (!MCwatchedvars[i].handlername.IsSet())
					t_success = MCListAppend(*t_watched_var, kMCEmptyString);
				else
					t_success = MCListAppend(*t_watched_var, *MCwatchedvars[i].handlername);
			}
			
			if (t_success)
				t_success = MCListAppend(*t_watched_var, *MCwatchedvars[i].varname);

			// SN-2014-09-18: [[ Bug 13453 ]] A watched variable's expression is never nil
			if (t_success)
				t_success = MCListAppend(*t_watched_var, *MCwatchedvars[i].expression);

			if (t_success)
				t_success = MCListAppend(*t_watches_list, *t_watched_var);
		}
	}

	if (t_success)
		t_success = MCListCopyAsString(*t_watches_list, r_watches);

	return t_success;
	
}

