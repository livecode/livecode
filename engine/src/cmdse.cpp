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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcio.h"

#include "dispatch.h"
#include "object.h"
#include "stack.h"
#include "card.h"
#include "aclip.h"
#include "vclip.h"
#include "player.h"
#include "group.h"
#include "scriptpt.h"

#include "handler.h"
#include "cmds.h"
#include "mcerror.h"
#include "chunk.h"
#include "param.h"
#include "util.h"
#include "date.h"
#include "debug.h"
#include "printer.h"
#include "variable.h"
#include "securemode.h"
#include "osspec.h"
#include "image.h"
#include "font.h"
#include "hndlrlst.h"

#include "globals.h"

#include "license.h"
#include "socket.h"

#include "exec.h"

MCAccept::~MCAccept()
{
	delete port;
	delete message;
}

Parse_stat MCAccept::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.skip_token(SP_ACCEPT, TT_UNDEFINED, AC_SECURE) == PS_NORMAL)
		secure = True;
	else if (sp.skip_token(SP_ACCEPT, TT_UNDEFINED, AC_DATAGRAM) == PS_NORMAL)
		datagram = True;
	
	Parse_stat t_stat = PS_NORMAL;
	
	if (PS_NORMAL == t_stat)
		t_stat = sp.skip_token(SP_ACCEPT, TT_UNDEFINED, AC_CONNECTIONS);
	
	if (PS_NORMAL == t_stat)
		t_stat = sp.skip_token(SP_ACCEPT, TT_UNDEFINED, AC_ON);
	
	if (PS_NORMAL == t_stat)
		t_stat = sp.skip_token(SP_ACCEPT, TT_UNDEFINED, AC_PORT);
	
	if (PS_NORMAL == t_stat)
		t_stat = sp.parseexp(False, True, &port);
	
	if (PS_NORMAL == t_stat)
		t_stat = sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH);
	
	if (PS_NORMAL == t_stat)
		t_stat = sp.skip_token(SP_SUGAR, TT_CHUNK, CT_UNDEFINED);
	
	if (PS_NORMAL == t_stat)
		t_stat = sp.parseexp(False, True, &message);
		
	if (PS_NORMAL != t_stat)
	{
		MCperror->add(PE_ACCEPT_BADEXP, sp);
		return PS_ERROR;
	}
	
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL
	        && sp.skip_token(SP_SSL, TT_STATEMENT, SSL_VERIFICATION) != PS_NORMAL)
	{
		//make error
		MCperror->add(PE_OPEN_BADMESSAGE, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_SUGAR, TT_PREP, PT_WITHOUT) == PS_NORMAL
	        && sp.skip_token(SP_SSL, TT_STATEMENT, SSL_VERIFICATION) == PS_NORMAL)
		secureverify = False;
	return PS_NORMAL;
}

void MCAccept::exec_ctxt(MCExecContext &ctxt)
{
    
    uinteger_t t_port;
    if (!ctxt . EvalExprAsUInt(port, EE_ACCEPT_BADEXP, t_port))
        return;
	
	if (t_port > UINT16_MAX)
	{
		ctxt . LegacyThrow(EE_ACCEPT_BADEXP);
		return;
	}
	
    MCNewAutoNameRef t_message;
    if (!ctxt . EvalExprAsNameRef(message, EE_ACCEPT_BADEXP, &t_message))
        return;
    
    if (datagram)
		MCNetworkExecAcceptDatagramConnectionsOnPort(ctxt, uint16_t(t_port), *t_message);
	else if (secure)
		MCNetworkExecAcceptSecureConnectionsOnPort(ctxt, uint16_t(t_port), *t_message, secureverify == True);
	else
		MCNetworkExecAcceptConnectionsOnPort(ctxt, uint16_t(t_port), *t_message);
}

MCBeep::~MCBeep()
{
	delete times;
}

Parse_stat MCBeep::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	MCScriptPoint oldsp(sp);
	MCerrorlock++;
	if (sp.parseexp(False, True, &times) != PS_NORMAL)
	{
		sp = oldsp;
		delete times;
		times = NULL;
	}
	MCerrorlock--;
	return PS_NORMAL;
}

void MCBeep::exec_ctxt(MCExecContext& ctxt)
{
	uinteger_t t_count;
	if (!ctxt . EvalOptionalExprAsUInt(times, 1, EE_BEEP_BADEXP, t_count))
		return;
	
	MCInterfaceExecBeep(ctxt, t_count);
}

void MCBreakPoint::exec_ctxt(MCExecContext& ctxt)
{
    MCDebuggingExecBreakpoint(ctxt, line, pos);
}

MCCancel::~MCCancel()
{
	delete m_id;
}

Parse_stat MCCancel::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp . skip_token(SP_RESET, TT_UNDEFINED, RT_PRINTING) == PS_NORMAL)
	{
		m_id = NULL;
	}
	else if (sp.parseexp(False, True, &m_id) != PS_NORMAL)
	{
		MCperror->add(PE_CANCEL_BADEXP, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCCancel::exec_ctxt(MCExecContext& ctxt)
{
    if (m_id == NULL)
		MCPrintingExecCancelPrinting(ctxt);
	
	else
	{
        integer_t t_id;
        if (!ctxt . EvalExprAsInt(m_id, EE_CANCEL_IDNAN, t_id))
            return;
        MCEngineExecCancelMessage(ctxt, t_id);
    }
}

MCClickCmd::~MCClickCmd()
{
	delete button;
	delete location;
}

Parse_stat MCClickCmd::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH);
	if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_BUTTON) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &button) != PS_NORMAL)
		{
			MCperror->add(PE_CLICK_BADBUTTONEXP, sp);
			return PS_ERROR;
		}
	}
if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AT) != PS_NORMAL)
	{
		MCperror->add(PE_CLICK_NOAT, sp);
		return PS_ERROR;
	}
	if (sp.parseexp(False, True, &location) != PS_NORMAL)
	{
		MCperror->add(PE_CLICK_BADLOCATIONEXP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
		return getmods(sp, mstate);
	return PS_NORMAL;
}

void MCClickCmd::exec_ctxt(MCExecContext& ctxt)
{
    uinteger_t t_which;
    if (!ctxt . EvalOptionalExprAsUInt(button, which, EE_CLICK_BADBUTTON, t_which))
        return;
    which = t_which;
    
    MCPoint t_location;
    if (!ctxt . EvalExprAsPoint(location, EE_CLICK_BADLOCATION, t_location))
        return;
    
    MCInterfaceExecClickCmd(ctxt, which, t_location, mstate);
}

MCDrag::~MCDrag()
{
	delete button;
	delete startloc;
	delete endloc;
}

Parse_stat MCDrag::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH);
	if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_BUTTON) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &button) != PS_NORMAL)
		{
			MCperror->add(PE_DRAG_BADBUTTONEXP, sp);
			return PS_ERROR;
		}
	}
	if (sp.skip_token(SP_FACTOR, TT_FROM, PT_FROM) != PS_NORMAL)
	{
		MCperror->add(PE_DRAG_NOFROM, sp);
		return PS_ERROR;
	}
	if (sp.parseexp(False, True, &startloc) != PS_NORMAL)
	{
		MCperror->add(PE_DRAG_BADSTARTLOCEXP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_TO, PT_TO) != PS_NORMAL)
	{
		MCperror->add(PE_DRAG_NOTO, sp);
		return PS_ERROR;
	}
	if (sp.parseexp(False, True, &endloc) != PS_NORMAL)
	{
		MCperror->add(PE_DRAG_BADENDLOCEXP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
		return getmods(sp, mstate);
	return PS_NORMAL;
}

void MCDrag::exec_ctxt(MCExecContext& ctxt)
{
    uinteger_t t_which;
    if (!ctxt . EvalOptionalExprAsUInt(button, which, EE_DRAG_BADBUTTON, t_which))
        return;
    which = t_which;
    
    MCPoint t_start;
    if (!ctxt . EvalExprAsPoint(startloc, EE_DRAG_BADSTARTLOC, t_start))
        return;
    
    MCPoint t_end;
    if (!ctxt . EvalExprAsPoint(endloc, EE_DRAG_BADENDLOC, t_end))
        return;
    
    MCInterfaceExecDrag(ctxt, which, t_start, t_end, mstate);
}

MCFocus::~MCFocus()
{
	delete object;
}

Parse_stat MCFocus::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	sp.skip_token(SP_FACTOR, TT_OF, PT_ON);

	// MW-2008-01-30: [[ Bug 5676 ]] Add "focus on nothing" to allow unfocusing
	//   all objects on a card.
	if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_NOTHING) == PS_NORMAL)
		object = NULL;
	else
	{
		object = new (nothrow) MCChunk(False);
		if (object->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_FOCUS_BADOBJECT, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

void MCFocus::exec_ctxt(MCExecContext &ctxt)
{
	if (object == NULL)
		MCInterfaceExecFocusOnNothing(ctxt);
	else
	{
		MCObject *optr;
		uint4 parid;
        if (!object->getobj(ctxt, optr, parid, True) ||
            !MCChunkTermIsControl(optr -> gettype()))
		{
            ctxt . LegacyThrow(EE_FOCUS_BADOBJECT);
            return;
		}
		MCInterfaceExecFocusOn(ctxt, optr);
    }
}

MCInsert::~MCInsert()
{
	delete target;
}

Parse_stat MCInsert::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);

	sp.skip_token(SP_FACTOR, TT_THE);
	if (sp.skip_token(SP_FACTOR, TT_PROPERTY, P_SCRIPT) != PS_NORMAL
	        || sp.skip_token(SP_FACTOR, TT_OF) != PS_NORMAL)
	{
		MCperror->add(PE_INSERT_NOSCRIPT, sp);
		return PS_ERROR;
	}
	target = new (nothrow) MCChunk(False);
	if (target->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add(PE_INSERT_BADOBJECT, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_PREP, PT_INTO) != PS_NORMAL)
	{
		MCperror->add(PE_INSERT_NOINTO, sp);
		return PS_ERROR;
	}
	if (sp.next(type) != PS_NORMAL
	        || sp.lookup(SP_INSERT, te) != PS_NORMAL)
	{
		MCperror->add(PE_INSERT_NOPLACE, sp);
		return PS_ERROR;
	}
	where = (Insert_point)te->which;
	return PS_NORMAL;
}

void MCInsert::exec_ctxt(MCExecContext &ctxt)
{
	MCObject *optr;
	uint4 parid;
    if (!target->getobj(ctxt, optr, parid, True))
	{
        ctxt . LegacyThrow(EE_INSERT_BADTARGET);
        return;
    }

    MCEngineExecInsertScriptOfObjectInto(ctxt, optr, where == IP_FRONT);
}

// MW-2008-11-05: [[ Dispatch Command ]] Implementation for the dispatch command.
MCDispatchCmd::~MCDispatchCmd(void)
{
	while(params != NULL)
	{
		MCParameter *t_param;
		t_param = params;
		params = params -> getnext();
		delete t_param;
	}
	
	delete target;
	delete message;
}

// Syntax is:
//   dispatch [ command | function ] <message: Expression> [ with <parameters: ParamList> ]
//   dispatch [ command | function ] <message: Expression> [ to <target: Chunk> ] [ with <parameters: ParamList> ]
Parse_stat MCDispatchCmd::parse(MCScriptPoint& sp)
{
	initpoint(sp);

	// MW-2009-09-11: Added support for command/function specification
	if (sp . skip_token(SP_HANDLER, TT_HANDLER, HT_FUNCTION) == PS_NORMAL)
		is_function = true;
	else
		sp . skip_token(SP_HANDLER, TT_HANDLER, HT_MESSAGE);

	if (sp . parseexp(False, True, &message) != PS_NORMAL)
	{
		MCperror->add(PE_DISPATCH_BADMESSAGE, sp);
		return PS_ERROR;
	}
	
	// MW-2008-12-04: Added 'to <target>' form to the syntax
	if (sp.skip_token(SP_FACTOR, TT_TO) == PS_NORMAL)
	{
		target = new (nothrow) MCChunk(False);
		if (target -> parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_DISPATCH_BADTARGET, sp);
			return PS_ERROR;
		}
	}
	
	if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		if (sp.is_eol() || getparams(sp, &params) != PS_NORMAL)
		{
			MCperror -> add(PE_DISPATCH_BADPARAMS, sp);
			return PS_ERROR;
		}
	}
    
    /* If there are any parameters then compute the number of containers needed
     * to execute the command. */
    if (params != nullptr)
    {
        container_count = params->count_containers();
    }
    
	return PS_NORMAL;
}

// This method follows along the same lines as MCComref::exec
void MCDispatchCmd::exec_ctxt(MCExecContext &ctxt)
{
    MCNewAutoNameRef t_message;
    if (!ctxt . EvalExprAsNameRef(message, EE_DISPATCH_BADMESSAGEEXP, &t_message))
        return;
	
	// Evaluate the target object (if we parsed a 'target' chunk).
	MCObjectPtr t_target;
	MCObjectPtr *t_target_ptr;
	if (target != nil)
	{
        if (!target->getobj(ctxt, t_target, True))
		{
            ctxt . LegacyThrow(EE_DISPATCH_BADTARGET);
            return;
		}
		t_target_ptr = &t_target;
	}
	else
		t_target_ptr = nil;
    
    /* Attempt to allocate the number of containers needed for the call. */
    MCAutoPointer<MCContainer[]> t_containers = new MCContainer[container_count];
    if (!t_containers)
    {
        ctxt.LegacyThrow(EE_NO_MEMORY);
        return;
    }
    
	/* If the argument list is successfully evaluated, then do the dispatch. */
	if (MCKeywordsExecSetupCommandOrFunction(ctxt,
                                             params,
                                             *t_containers,
                                             line,
                                             pos,
                                             is_function))
    {
        if (!ctxt.HasError())
        {
            ctxt.SetLineAndPos(line, pos);
            MCEngineExecDispatch(ctxt,
                                 is_function ? HT_FUNCTION : HT_MESSAGE,
                                 *t_message,
                                 t_target_ptr,
                                 params);
        }
    }
    
    /* Clean up the evaluated argument list */
    MCKeywordsExecTeardownCommandOrFunction(params);
}

MCLogCmd::~MCLogCmd(void)
{
    while(params != NULL)
    {
        MCParameter *t_param;
        t_param = params;
        params = params -> getnext();
        delete t_param;
    }
}

Parse_stat MCLogCmd::parse(MCScriptPoint& sp)
{
    initpoint(sp);
    
    if (getparams(sp, &params) != PS_NORMAL)
    {
        MCperror -> add(PE_STATEMENT_BADPARAMS, sp);
        return PS_ERROR;
    }
    
    /* If there are any parameters then compute the number of containers needed
     * to execute the command. */
    if (params != nullptr)
    {
        container_count = params->count_containers();
    }
    
    return PS_NORMAL;
}

// This method follows along the same lines as MCComref::exec
void MCLogCmd::exec_ctxt(MCExecContext &ctxt)
{
    // no-op if logMessage is empty
    if (MCNameIsEmpty(MClogmessage))
    {
        return;
    }
    
    /* Attempt to allocate the number of containers needed for the call. */
    MCAutoPointer<MCContainer[]> t_containers = new MCContainer[container_count];
    if (!t_containers)
    {
        ctxt.LegacyThrow(EE_NO_MEMORY);
        return;
    }
    
    /* If the argument list is successfully evaluated, then do the dispatch. */
    if (MCKeywordsExecSetupCommandOrFunction(ctxt,
                                             params,
                                             *t_containers,
                                             line,
                                             pos,
                                             false))
    {
        if (!ctxt.HasError())
        {
            ctxt.SetLineAndPos(line, pos);
            MCHandler * t_handler = nullptr;
            MCKeywordsExecResolveCommandOrFunction(ctxt, MClogmessage, false, t_handler);
            MCKeywordsExecCommandOrFunction(ctxt, t_handler, params, MClogmessage, line, pos, false, false);
        }
    }
    
    /* Clean up the evaluated argument list */
    MCKeywordsExecTeardownCommandOrFunction(params);
    
    if (MCresultmode == kMCExecResultModeReturn)
    {
        // Do nothing!
    }
    else if (MCresultmode == kMCExecResultModeReturnValue)
    {
        // Set 'it' to the result and clear the result
        MCAutoValueRef t_value;
        if (!MCresult->eval(ctxt, &t_value))
        {
            ctxt.Throw();
            return;
        }
        
        ctxt.SetItToValue(*t_value);
        ctxt.SetTheResultToEmpty();
    }
    else if (MCresultmode == kMCExecResultModeReturnError)
    {
        // Set 'it' to empty
        ctxt.SetItToEmpty();
        // Leave the result as is but make sure we reset the 'return mode' to default.
        MCresultmode = kMCExecResultModeReturn;
    }
}


Parse_stat MCMessage::parse(MCScriptPoint &sp)
{
	initpoint(sp);

    MCScriptPoint oldsp(sp);
    if (send && sp.skip_token(SP_FACTOR, TT_PROPERTY, P_SCRIPT) == PS_NORMAL)
    {
        MCerrorlock++;
        if (sp.parseexp(False, True, &(&message)) != PS_NORMAL)
        {
            sp = oldsp;
        }
        else
        {
            script = True;
        }
        MCerrorlock--;
    }

    if (!script && sp.parseexp(False, True, &(&message)) != PS_NORMAL)
    {
        MCperror->add(PE_SEND_BADEXP, sp);
        return PS_ERROR;
    }
    
	if (sp.skip_token(SP_FACTOR, TT_TO) != PS_NORMAL
	        && sp.skip_token(SP_FACTOR, TT_OF) != PS_NORMAL)
		return PS_NORMAL;
	if (sp.skip_token(SP_ASK, TT_UNDEFINED, AT_PROGRAM) == PS_NORMAL)
	{
		program = True;
		if (sp.parseexp(False, True, &(&in)) != PS_NORMAL)
		{
			MCperror->add(PE_SEND_BADTARGET, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL
		        && sp.skip_token(SP_COMMAND, TT_STATEMENT, S_REPLY) != PS_NORMAL)
			if (sp.parseexp(False, True, &(&eventtype)) != PS_NORMAL)
			{
				MCperror->add(PE_SEND_BADEVENTTYPE, sp);
				return PS_ERROR;
			}
		if (sp.skip_token(SP_SUGAR, TT_PREP, PT_WITHOUT) == PS_NORMAL)
		{
			sp.skip_token(SP_COMMAND, TT_STATEMENT, S_REPLY);
			sp.skip_token(SP_MOVE, TT_UNDEFINED, MM_WAITING);
			reply = False;
		}
	}
	else
	{
		target = new (nothrow) MCChunk(False);
		if (target->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_SEND_BADTARGET, sp);
			return PS_ERROR;
		}

        if (script && sp.skip_eol() != PS_NORMAL)
        {
            MCperror->add(PE_SEND_SCRIPTINTIME, sp);
            return PS_ERROR;
        }
        
        return gettime(sp, &(&in), units);
	}
	return PS_NORMAL;
}

void MCMessage::exec_ctxt(MCExecContext &ctxt)
{
		
	if (program)
	{
        MCAutoStringRef t_message;
        if (!ctxt . EvalExprAsStringRef(*message, EE_SEND_BADEXP, &t_message))
            return;

        MCAutoStringRef t_program;
        if (!ctxt . EvalExprAsStringRef(*in, EE_SEND_BADPROGRAMEXP, &t_program))
            return;

        MCAutoStringRef t_event_type;
        if (!ctxt . EvalOptionalExprAsNullableStringRef(*eventtype, EE_SEND_BADEXP, &t_event_type))
            return;

		MCScriptingExecSendToProgram(ctxt, *t_message, *t_program, *t_event_type, reply == True);
	}
	else
	{
        MCAutoStringRef t_message;
        if (!ctxt . EvalExprAsStringRef(*message, EE_SEND_BADEXP, &t_message))
            return;
		
		MCObjectPtr t_target;
		MCObjectPtr *t_target_ptr;
		if (*target != nil)
		{
            if (!target -> getobj(ctxt, t_target, True))
			{
                ctxt . LegacyThrow(EE_SEND_BADTARGET);
                return;
			}
			t_target_ptr = &t_target;
		}
		else
			t_target_ptr = nil;

		if (*in != nil)
		{
            double t_delay;

            if (!ctxt . EvalExprAsDouble(*in, EE_SEND_BADINEXP, t_delay))
                return;

            MCEngineExecSendInTime(ctxt, *t_message, t_target, t_delay, units);
		}
        else
        {
            ctxt . SetLineAndPos(line, pos);
            if (!script)
            {
                if (!send)
                    MCEngineExecCall(ctxt, *t_message, t_target_ptr);
                else
                    MCEngineExecSend(ctxt, *t_message, t_target_ptr);
            }
            else
            {
                MCEngineExecSendScript(ctxt, *t_message, t_target_ptr);
            }
        }
    }
}

MCMove::~MCMove()
{
	delete object;
	delete startloc;
	delete endloc;
	delete durationexp;
}

Parse_stat MCMove::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	object = new (nothrow) MCChunk(False);
	if (object->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add(PE_MOVE_BADOBJECT, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_FROM, PT_FROM) == PS_NORMAL)
		if (sp.parseexp(False, True, &startloc) != PS_NORMAL)
		{
			MCperror->add(PE_MOVE_BADSTARTLOCEXP, sp);
			return PS_ERROR;
		}
	if (sp.skip_token(SP_FACTOR, TT_TO, PT_TO) != PS_NORMAL)
	{
		if (sp.skip_token(SP_FACTOR, TT_TO, PT_RELATIVE) != PS_NORMAL)
		{
			MCperror->add(PE_MOVE_NOTO, sp);
			return PS_ERROR;
		}
		relative = True;
	}
	if (sp.parseexp(False, True, &endloc) != PS_NORMAL)
	{
		MCperror->add(PE_MOVE_BADENDLOCEXP, sp);
		return PS_ERROR;
	}
	if (gettime(sp, &durationexp, units) != PS_NORMAL)
		return PS_ERROR;
	if (sp.skip_token(SP_SUGAR, TT_PREP, PT_WITHOUT) == PS_NORMAL)
	{
		if (sp.skip_token(SP_MOVE, TT_UNDEFINED, MM_MESSAGES) == PS_NORMAL)
			messages = False;
		else
			if (sp.skip_token(SP_MOVE, TT_UNDEFINED, MM_WAITING) == PS_NORMAL)
				waiting = False;
			else
			{
				MCperror->add(PE_MOVE_BADWITHOUT, sp);
				return PS_ERROR;
			}
	}
	return PS_NORMAL;
}

void MCMove::exec_ctxt(MCExecContext &ctxt)
{
	MCObject *optr;
	uint4 parid;

    if (!object->getobj(ctxt, optr, parid, True))
	{
        ctxt . LegacyThrow(EE_MOVE_BADOBJECT);
        return;
	}

    real8 duration;
    if (!ctxt . EvalOptionalExprAsDouble(durationexp, 0.0, EE_MOVE_BADDURATION, duration))
        return;

	if (startloc != NULL)
    {
        MCPoint t_to, t_from;

        if (!ctxt . EvalExprAsPoint(endloc, EE_MOVE_BADENDLOC, t_to))
            return;

        if (!ctxt . EvalExprAsPoint(startloc, EE_MOVE_BADSTARTLOC, t_from))
            return;

		MCInterfaceExecMoveObjectBetween(ctxt, optr, t_from, t_to, duration, units, waiting == True, messages == True);
	}
	else
	{	
		MCAutoArray<MCPoint> t_points;
		MCAutoStringRef t_motion;
        if (!ctxt . EvalExprAsStringRef(endloc, EE_MOVE_BADENDLOC, &t_motion))
            return;

		if (!MCU_parsepoints(t_points.PtrRef(), t_points.SizeRef(), *t_motion))
		{
            ctxt . LegacyThrow(EE_MOVE_ENDNAP);
            return;
		}

		MCInterfaceExecMoveObjectAlong(ctxt, optr, t_points.Ptr(), t_points.Size(), relative == True, duration, units, waiting == True, messages == True);		
   }
}

MCMM::~MCMM()
{
	delete clip;
	delete stack;
	delete tempo;
	delete loc;
	delete options;
}

Parse_stat MCMM::parse(MCScriptPoint &sp)
{
	initpoint(sp);

	if (prepare && sp.skip_token(SP_FACTOR, TT_CHUNK, CT_IMAGE) == PS_NORMAL)
	{
		if (sp . skip_token(SP_THERE, TT_UNDEFINED, TM_FILE) == PS_NORMAL)
		{
			if (sp.parseexp(False, True, &clip) != PS_NORMAL)
			{
				MCperror->add(PE_PLAY_BADCLIP, sp);
				return PS_ERROR;
			}
			image_file = True;
			return PS_NORMAL;
		}
		
		sp.backup();
		stack = new (nothrow) MCChunk(False);
		if (stack->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_PLAY_BADSTACK, sp);
			return PS_ERROR;
		}
	
		image = True;
		return PS_NORMAL;
	}
	
	if (sp.skip_token(SP_PLAY, TT_UNDEFINED, PP_VIDEO) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &clip) != PS_NORMAL)
		{
			MCperror->add(PE_PLAY_BADCLIP, sp);
			return PS_ERROR;
		}
		video = True;
		return PS_NORMAL;
	}
	
	if (sp.skip_token(SP_PLAY, TT_UNDEFINED, PP_STEP) == PS_NORMAL)
	{
		if (sp.skip_token(SP_PLAY, TT_UNDEFINED, PP_FORWARD) == PS_NORMAL)
			stepforward = True;
		if (sp.skip_token(SP_PLAY, TT_UNDEFINED, PP_BACK) == PS_NORMAL)
			stepback = True;
		video = True;
	}
	if (sp.skip_token(SP_PLAY, TT_UNDEFINED, PP_PAUSE) == PS_NORMAL)
		pause = True;
	if (sp.skip_token(SP_PLAY, TT_UNDEFINED, PP_RESUME) == PS_NORMAL)
		resume = True;
	if (sp.skip_token(SP_PLAY, TT_UNDEFINED, PP_STOP) == PS_NORMAL)
		stop = True;
	if (sp.skip_token(SP_PLAY, TT_UNDEFINED, PP_AUDIO_CLIP) == PS_NORMAL)
		audio = True;
	if (sp.skip_token(SP_PLAY, TT_UNDEFINED, PP_VIDEO_CLIP) == PS_NORMAL)
		video = True;
	if (sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_BACKGROUND) == PS_NORMAL)
		ptype = CT_BACKGROUND;
	if (sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_CARD) == PS_NORMAL)
		ptype = CT_CARD;
	if (sp.skip_token(SP_PLAY, TT_UNDEFINED, PP_PLAYER) == PS_NORMAL)
		player = True;
	if (sp.skip_token(SP_FACTOR, TT_PROPERTY, P_ID) == PS_NORMAL)
		etype = CT_ID;
	if (stop && !video && !player)
	{
		audio = True;
		return PS_NORMAL;
	}
	if (!audio && !video && !player)
		audio = True;
	sp.skip_token(SP_FACTOR, TT_CHUNK, CT_URL);
	MCerrorlock++;
	if (sp.parseexp(False, True, &clip) != PS_NORMAL)
	{
		MCerrorlock--;
		if (stepback || stepforward || stop)
			return PS_NORMAL;
		MCperror->add(PE_PLAY_BADCLIP, sp);
		return PS_ERROR;
	}
	MCerrorlock--;
	if (sp.skip_token(SP_FACTOR, TT_OF) == PS_NORMAL)
	{
		stack = new (nothrow) MCChunk(False);
		if (stack->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_PLAY_BADSTACK, sp);
			return PS_ERROR;
		}
	}
	if (sp.skip_token(SP_PLAY, TT_UNDEFINED, PP_LOOPING) == PS_NORMAL)
		looping = True;
	if (video)
	{
		if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AT) == PS_NORMAL)
		{
			if (sp.parseexp(False, True, &loc) != PS_NORMAL)
			{
				MCperror->add(PE_PLAY_BADLOC, sp);
				return PS_ERROR;
			}
		}
		if (sp.skip_token(SP_PLAY, TT_UNDEFINED, PP_OPTIONS) == PS_NORMAL)
		{
			if (sp.parseexp(False, True, &options) != PS_NORMAL)
			{
				MCperror->add(PE_PLAY_BADOPTIONS, sp);
				return PS_ERROR;
			}
		}
	}
	else
	{
		if (sp.skip_token(SP_PLAY, TT_UNDEFINED, PP_TEMPO) == PS_NORMAL)
		{
			if (sp.parseexp(False, True, &tempo) != PS_NORMAL)
			{
				MCperror->add(PE_PLAY_BADTEMPO, sp);
				return PS_ERROR;
			}
		}
		MCScriptPoint notessp(sp);
		MCerrorlock++;
		if (sp.parseexp(False, True, &loc) != PS_NORMAL)
		{
			sp = notessp;
			delete loc;
			loc = NULL;
		}
		MCerrorlock--;
	}
	return PS_NORMAL;
}

void MCMM::exec_ctxt(MCExecContext &ctxt)
{
	ctxt . SetTheResultToEmpty();
	
	if (prepare && image)
	{
		uint4 parid;
		MCObject *t_object;
        if (!stack -> getobj(ctxt, t_object, parid, True) ||
			t_object -> gettype() != CT_IMAGE)
		{
            ctxt . LegacyThrow(EE_PLAY_BADCLIP);
            return;
		}
		
		MCGraphicsExecPrepareImage(ctxt, static_cast<MCImage *>(t_object));
	}
	else if (prepare && image_file)
    {
		MCAutoStringRef t_filename;
        if (!ctxt . EvalExprAsStringRef(clip, EE_PLAY_BADCLIP, &t_filename))
            return;
		
		MCGraphicsExecPrepareImageFile(ctxt, *t_filename);
	}
	else if (clip == NULL)
	{
		if (video)
		{
			if (stepforward)
				MCMultimediaExecPlayLastVideoOperation(ctxt, PP_FORWARD);
			else if (stepback)
				MCMultimediaExecPlayLastVideoOperation(ctxt, PP_BACK);
			else if (pause)
				MCMultimediaExecPlayLastVideoOperation(ctxt, PP_PAUSE);
			else if (stop)
				MCMultimediaExecPlayLastVideoOperation(ctxt, PP_STOP);
			else if (resume)
				MCMultimediaExecPlayLastVideoOperation(ctxt, PP_RESUME);
			else
				MCMultimediaExecPlayLastVideoOperation(ctxt, PP_UNDEFINED);
		}
		// AL-2014-09-12: [[ Bug 13428 ]] The only valid audio action without a clip is stop
		else if (audio)
		{
			MCMultimediaExecStopPlaying(ctxt);
		}
		
		// PM-2015-09-23: [[ Bug 15994 ]] Calling 'play stop' on mobile should stop the currently played video
		if (stop)
		{
#ifdef _MOBILE
			MCMultimediaExecPlayVideoOperation(ctxt, nil, etype, kMCEmptyString, PP_STOP);
#endif
		}
	}
	else
	{
		MCObject *optr = nil;
		if (stack != NULL)
		{
			uint4 parid;
            if (!stack->getobj(ctxt, optr, parid, True) ||
				optr -> gettype() != CT_STACK)
			{
                ctxt . LegacyThrow(EE_PLAY_BADCLIP);
                return;
			}
		}
		MCAutoStringRef t_clip_name;
        if (!ctxt . EvalExprAsStringRef(clip, EE_PLAY_BADCLIP, &t_clip_name))
            return;

		if (player)
		{
			if (pause)
				MCMultimediaExecPlayPlayerOperation(ctxt, (MCStack *)optr, etype, *t_clip_name, ptype, PP_PAUSE);
			else if (stepforward)
				MCMultimediaExecPlayPlayerOperation(ctxt, (MCStack *)optr, etype, *t_clip_name, ptype, PP_FORWARD);	
			else if (stepback)
				MCMultimediaExecPlayPlayerOperation(ctxt, (MCStack *)optr, etype, *t_clip_name, ptype, PP_BACK);			
			else if (stop)
				MCMultimediaExecPlayPlayerOperation(ctxt, (MCStack *)optr, etype, *t_clip_name, ptype, PP_STOP);
			else if (resume)
				MCMultimediaExecPlayPlayerOperation(ctxt, (MCStack *)optr, etype, *t_clip_name, ptype, PP_RESUME);
			else
				MCMultimediaExecPlayPlayerOperation(ctxt, (MCStack *)optr, etype, *t_clip_name, ptype, PP_UNDEFINED);			
		}
		else if (video)
		{
			if (pause)
				MCMultimediaExecPlayVideoOperation(ctxt, (MCStack *)optr, etype, *t_clip_name, PP_PAUSE);
			else if (stepforward)
				MCMultimediaExecPlayVideoOperation(ctxt, (MCStack *)optr, etype, *t_clip_name, PP_FORWARD);	
			else if (stepback)
				MCMultimediaExecPlayVideoOperation(ctxt, (MCStack *)optr, etype, *t_clip_name, PP_BACK);			
			else if (stop)
				MCMultimediaExecPlayVideoOperation(ctxt, (MCStack *)optr, etype, *t_clip_name, PP_STOP);
			else if (resume)
				MCMultimediaExecPlayVideoOperation(ctxt, (MCStack *)optr, etype, *t_clip_name, PP_RESUME);
			else
			{
                MCPoint t_loc;
                MCPoint *t_loc_ptr = &t_loc;

                if (!ctxt . EvalOptionalExprAsPoint(loc, nil, EE_PLAY_BADLOC, t_loc_ptr))
                    return;

				MCAutoStringRef t_options;
                if (!ctxt . EvalOptionalExprAsNullableStringRef(options, EE_PLAY_BADOPTIONS, &t_options))
                    return;

				if (!prepare)
					MCMultimediaExecPlayVideoClip(ctxt, (MCStack *)optr, etype, *t_clip_name, looping == True, t_loc_ptr, *t_options);
				else
					MCMultimediaExecPrepareVideoClip(ctxt, (MCStack *)optr, etype, *t_clip_name, looping == True, t_loc_ptr, *t_options);
			}
		}
		else
			MCMultimediaExecPlayAudioClip(ctxt, (MCStack *)optr, etype, *t_clip_name, looping == True);
    }
}

MCReply::~MCReply()
{
	delete message;
	delete keyword;
}

Parse_stat MCReply::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.skip_token(SP_LOCK, TT_UNDEFINED, LC_ERRORS) == PS_NORMAL)
		error = True;
	if (sp.parseexp(False, True, &message) != PS_NORMAL)
	{
		MCperror->add(PE_REPLY_BADEXP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		sp.skip_token(SP_SUGAR, TT_CHUNK, CT_UNDEFINED);
		if (sp.parseexp(True, False, &keyword) != PS_NORMAL)
		{
			MCperror->add(PE_REPLY_BADKEYWORD, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

void MCReply::exec_ctxt(MCExecContext& ctxt)
{
    MCAutoStringRef t_message;
    if (!ctxt . EvalExprAsStringRef(message, EE_REPLY_BADMESSAGEEXP, &t_message))
        return;
    
    MCAutoStringRef t_keyword;
    if (!error)
    {
        if (!ctxt . EvalOptionalExprAsNullableStringRef(keyword, EE_REPLY_BADKEYWORDEXP, &t_keyword))
            return;
	}
    
    if (!error)
		MCScriptingExecReply(ctxt, *t_message, *t_keyword);
	else
		MCScriptingExecReplyError(ctxt, *t_message);
}

MCRequest::~MCRequest()
{
	delete message;
	delete program;
}

Parse_stat MCRequest::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.skip_token(SP_AE, TT_UNDEFINED, AE_AE) == PS_NORMAL)
	{
		Symbol_type type;
		const LT *te;
		if (sp.next(type) != PS_NORMAL)
		{
			MCperror->add(PE_REQUEST_NOTYPE, sp);
			return PS_ERROR;
		}
		if (sp.lookup(SP_AE, te) != PS_NORMAL)
		{
			MCperror->add(PE_REQUEST_NOTTYPE, sp);
			return PS_ERROR;
		}
		ae = (Apple_event)te->which;
		if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
		{
			sp.skip_token(SP_SUGAR, TT_CHUNK, CT_UNDEFINED);
			if (sp.parseexp(False, True, &program) != PS_NORMAL)
			{
				MCperror->add(PE_REQUEST_BADEXP, sp);
				return PS_ERROR;
			}
		}
	}
	else
	{
		if (sp.parseexp(False, True, &message) != PS_NORMAL)
		{
			MCperror->add(PE_REQUEST_BADEXP, sp);
			return PS_ERROR;
		}
		sp.skip_token(SP_FACTOR, TT_FROM, PT_FROM);
		sp.skip_token(SP_FACTOR, TT_OF, PT_OF);
		sp.skip_token(SP_ASK, TT_UNDEFINED, AT_PROGRAM);
		if (sp.parseexp(False, True, &program) != PS_NORMAL)
		{
			MCperror->add(PE_REQUEST_BADPROGRAM, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

void MCRequest::exec_ctxt(MCExecContext& ctxt)
{
    if (ae != AE_UNDEFINED)
	{
		MCAutoStringRef t_program;
        if (!ctxt . EvalOptionalExprAsNullableStringRef(program, EE_REQUEST_BADKEYWORDEXP, &t_program))
            return;
		MCScriptingExecRequestAppleEvent(ctxt, ae, *t_program);
	}
    
    else
	{
		MCAutoStringRef t_message;
        if (!ctxt . EvalExprAsStringRef(message, EE_REQUEST_BADMESSAGEEXP, &t_message))
            return;
        
		MCAutoStringRef t_program;
        if (!ctxt . EvalExprAsStringRef(program, EE_REQUEST_BADPROGRAMEXP, &t_program))
            return;
        MCScriptingExecRequestFromProgram(ctxt, *t_message, *t_program);
	}
}

MCStart::~MCStart()
{
	delete target;
	delete stack;
    delete font;
}

Parse_stat MCStart::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (mode == SC_UNDEFINED)
	{
		if (sp.next(type) != PS_NORMAL)
		{
			MCperror->add(PE_START_NOTYPE, sp);
			return PS_ERROR;
		}
		if (sp.lookup(SP_START, te) != PS_NORMAL)
		{
			MCperror->add(PE_START_NOTTYPE, sp);
			return PS_ERROR;
		}
		mode = (Start_constants)te->which;
	}
	if (mode == SC_USING)
	{        
		if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_STACK) == PS_NORMAL
		        || sp.skip_token(SP_FACTOR, TT_CHUNK, CT_THIS) == PS_NORMAL)
		{
			sp.backup();
			target = new (nothrow) MCChunk(False);
			if (target->parse(sp, False) != PS_NORMAL)
			{
				MCperror->add(PE_START_BADCHUNK, sp);
				return PS_ERROR;
			}
		}
        // TD-2013-06-12: [[ DynamicFonts ]] Look for font
        else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_FONT) == PS_NORMAL)
        {
            if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_FILE) != PS_NORMAL)
            {
                MCperror->add(PE_START_BADCHUNK, sp);
                return PS_ERROR;
            }
            
            if (sp . parseexp(False, True, &font) != PS_NORMAL)
            {
                MCperror->add(PE_START_BADCHUNK, sp);
                return PS_ERROR;
            }
        
            is_globally = (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_GLOBALLY) == PS_NORMAL);
        }
		else
        {
			if (sp.parseexp(False, True, &stack) != PS_NORMAL)
			{
				MCperror->add(PE_START_BADCHUNK, sp);
				return PS_ERROR;
			}
        }
	}
	else if (mode == SC_SESSION)
	{
		return PS_NORMAL;
	}
	else
	{
		if (mode == SC_PLAYER)
			sp.backup();
		target = new (nothrow) MCChunk(False);
		if (target->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_START_BADCHUNK, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

bool MCServerStartSession();
void MCStart::exec_ctxt(MCExecContext &ctxt)
{
	if (mode == SC_USING)
	{
        // TD-2013-06-12: [[ DynamicFonts ]] Look for font.
        if (font != NULL)
        {
            if (MCsecuremode & MC_SECUREMODE_DISK)
            {
                ctxt . LegacyThrow(EE_DISK_NOPERM);
                return;
            }
            
            MCAutoStringRef t_font;
            if (!ctxt . EvalExprAsStringRef(font, EE_FONT_BADFILEEXP, &t_font))
                return;

            MCTextExecStartUsingFont(ctxt, *t_font, is_globally);
        }
		else if (target != NULL)
		{
			MCObject *optr;
			uint4 parid;
            if (!target->getobj(ctxt, optr, parid, True)
			        || optr->gettype() != CT_STACK)
			{
                ctxt . LegacyThrow(EE_START_BADTARGET);
                return;
			}
			MCEngineExecStartUsingStack(ctxt, (MCStack *)optr);
		}
		else
        {
			MCAutoStringRef t_name;
            if (!ctxt . EvalExprAsStringRef(stack, EE_START_BADTARGET, &t_name))
                return;

			MCEngineExecStartUsingStackByName(ctxt, *t_name);
		}
	}
	else if (mode == SC_SESSION)
	{
#ifdef _SERVER
		MCServerExecStartSession(ctxt);
#else
        ctxt . LegacyThrow(EE_SESSION_BADCONTEXT);
        return;
#endif
	}
	else
	{
		MCObject *optr;
		uint4 parid;
        if (!target->getobj(ctxt, optr, parid, True))
		{
            ctxt . LegacyThrow(EE_START_BADTARGET);
            return;
		}
		if (optr->gettype() == CT_PLAYER)
		{
			MCMultimediaExecStartPlayer(ctxt, (MCPlayer *)optr);
		}
		else
		{
			if (optr->gettype() != CT_GROUP)
			{
                ctxt . LegacyThrow(EE_START_NOTABACKGROUND);
                return;
			}
			MCInterfaceExecStartEditingGroup(ctxt, (MCGroup *)optr);
		}
    }
}

MCStop::~MCStop()
{
	delete target;
	delete stack;
    delete font;
}

Parse_stat MCStop::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);

	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_STOP_NOTYPE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_START, te) != PS_NORMAL)
	{
		MCperror->add(PE_STOP_NOTTYPE, sp);
		return PS_ERROR;
	}
	mode = (Start_constants)te->which;
	if (mode == SC_RECORDING)
		return PS_NORMAL;
	if (mode == SC_SESSION)
		return PS_NORMAL;
	if (mode == SC_USING)
	{
		if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_STACK) == PS_NORMAL
		        || sp.skip_token(SP_FACTOR, TT_CHUNK, CT_THIS) == PS_NORMAL)
		{
			sp.backup();
			target = new (nothrow) MCChunk(False);
			if (target->parse(sp, False) != PS_NORMAL)
			{
				MCperror->add(PE_START_BADCHUNK, sp);
				return PS_ERROR;
			}
		}
        // TD-2013-06-20: [[ DynamicFonts ]] Look for font
        else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_FONT) == PS_NORMAL)
        {
            if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_FILE) != PS_NORMAL)
            {
                MCperror->add(PE_START_BADCHUNK, sp);
                return PS_ERROR;
            }
            
            if (sp . parseexp(False, True, &font) != PS_NORMAL)
            {
                MCperror->add(PE_START_BADCHUNK, sp);
                return PS_ERROR;
            }
        }
		else
			if (sp.parseexp(False, True, &stack) != PS_NORMAL)
			{
				MCperror->add(PE_START_BADCHUNK, sp);
				return PS_ERROR;
			}
	}
	else
	{
		if (mode == SC_PLAYER)
			sp.backup();
		target = new (nothrow) MCChunk(False);
		MCScriptPoint oldsp(sp);
		MCerrorlock++;
		if (target->parse(sp, False) != PS_NORMAL)
		{
			MCerrorlock--;
			if (mode == SC_EDITING)
			{
				delete target;
				target = NULL;
				sp = oldsp;
				return PS_NORMAL;
			}
			else
			{
				MCperror->add(PE_STOP_BADCHUNK, sp);
				return PS_ERROR;
			}
		}
		MCerrorlock--;
	}
	return PS_NORMAL;
}

bool MCServerStopSession();

void MCStop::exec_ctxt(MCExecContext &ctxt)
{
	MCObject *optr = NULL;
	uint4 parid;

	if (target != NULL)
        if (!target->getobj(ctxt, optr, parid, True)
            || (optr == NULL && mode != SC_EDITING))
		{
            ctxt . LegacyThrow(EE_STOP_BADTARGET);
            return;
		}

	switch (mode)
	{
	case SC_EDITING:
		if (optr != NULL)
		{
			if (optr->gettype() != CT_GROUP)
			{
                ctxt . LegacyThrow(EE_STOP_NOTABACKGROUND);
                return;
			}
			MCInterfaceExecStopEditingGroup(ctxt, (MCGroup *)optr);
		}
		else
			MCInterfaceExecStopEditingDefaultStack(ctxt);
		break;
	case SC_MOVING:
		MCInterfaceExecStopMovingObject(ctxt, optr);
		break;
	case SC_PLAYER:
	case SC_PLAYING:
		if (optr == NULL)
			MCMultimediaExecStopPlaying(ctxt);
		else
			MCMultimediaExecStopPlayingObject(ctxt, optr);
		break;
	case SC_RECORDING:
		MCMultimediaExecStopRecording(ctxt);
		break;
    case SC_USING:
		{
            // TD-2013-06-12: [[ DynamicFonts ]] Look for font.
            if (font != NULL)
            {
                MCAutoStringRef t_font;
                if (!ctxt . EvalExprAsStringRef(font, EE_FONT_BADFILEEXP, &t_font))
                    return;

                MCTextExecStopUsingFont(ctxt, *t_font);
            }
            else if (target != NULL)
			{
				if (!target->getobj(ctxt, optr, parid, True)
				        || optr->gettype() != CT_STACK)
				{
                    ctxt . LegacyThrow(EE_STOP_BADTARGET);
                    return;
				}
				MCEngineExecStopUsingStack(ctxt, (MCStack *)optr);
			}
			else
            {
                MCAutoStringRef t_name;
                if (!ctxt . EvalExprAsStringRef(stack, EE_STOP_BADTARGET, &t_name))
                    return;

				MCEngineExecStopUsingStackByName(ctxt, *t_name);
			}
		}
		break;
	case SC_SESSION:
		{
#ifdef _SERVER
			MCServerExecStopSession(ctxt);
#else
            ctxt . LegacyThrow(EE_SESSION_BADCONTEXT);
            return;
#endif
		}
	default:
		break;
    }
}

MCType::~MCType()
{
	delete message;
}

Parse_stat MCType::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &message) != PS_NORMAL)
	{
		MCperror->add(PE_TYPE_BADEXP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
		return getmods(sp, mstate);
	return PS_NORMAL;
}

void MCType::exec_ctxt(MCExecContext &ctxt)
{
    MCAutoStringRef t_typing;
    if (!ctxt . EvalExprAsStringRef(message, EE_TYPE_BADSTRINGEXP, &t_typing))
        return;

    MCInterfaceExecType(ctxt, *t_typing, mstate);
}
