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
#include "execpt.h"
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

#include "globals.h"

#include "license.h"
#include "socket.h"

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
	sp.skip_token(SP_ACCEPT, TT_UNDEFINED, AC_UNDEFINED); // connections
	sp.skip_token(SP_ACCEPT, TT_UNDEFINED, AC_UNDEFINED); // on
	sp.skip_token(SP_ACCEPT, TT_UNDEFINED, AC_UNDEFINED); // port
	if (sp.parseexp(False, True, &port) != PS_NORMAL)
	{
		MCperror->add(PE_ACCEPT_BADEXP, sp);
		return PS_ERROR;
	}
	sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH); // with
	sp.skip_token(SP_SUGAR, TT_CHUNK, CT_UNDEFINED); // message
	if (sp.parseexp(False, True, &message) != PS_NORMAL)
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

Exec_stat MCAccept::exec(MCExecPoint &ep)
{
#ifdef /* MCAccept */ LEGACY_EXEC
	// MW-2005-01-28: Fix bug 2412 - accept doesn't clear the result.
	MCresult -> clear(False);

	if (MCsecuremode & MC_SECUREMODE_NETWORK)
	{
		MCeerror->add(EE_NETWORK_NOPERM, line, pos);
		return ES_ERROR;
	}
	if (port->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ACCEPT_BADEXP, line, pos);
		return ES_ERROR;
	}
	uint2 port = ep.getuint2();
	if (message->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_ACCEPT_BADEXP, line, pos);
		return ES_ERROR;
	}

	MCAutoNameRef t_message_name;
	/* UNCHECKED */ ep . copyasnameref(t_message_name);

	MCSocket *s = MCS_accept(port, ep.getobj(), t_message_name, datagram, secure, secureverify, NULL);
	if (s != NULL)
	{
		MCU_realloc((char **)&MCsockets, MCnsockets,
		            MCnsockets + 1, sizeof(MCSocket *));
		MCsockets[MCnsockets++] = s;
	}

	return ES_NORMAL;
#endif /* MCAccept */
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

Exec_stat MCBeep::exec(MCExecPoint &ep)
{
#ifdef /* MCBeep */ LEGACY_EXEC
	uint4 i = 1;
	if (times != NULL)
	{
		if (times->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
		{
			MCeerror->add(EE_BEEP_BADEXP, line, pos);
			return ES_ERROR;
		}
		i = ep.getuint4();
	}
	while (i--)
	{
		MCscreen->beep();
		
		// MW-2010-01-08: [[ Bug 1690 ]] We need a break on all beeps but the last
		if (i >= 1)
		{
			// MW-2008-03-17: [[ Bug 6098 ]] Make sure we check for an abort from wait
			if (MCscreen->wait(BEEP_INTERVAL, False, False))
			{
				MCeerror -> add(EE_WAIT_ABORT, line, pos);
				return ES_ERROR;
			}
		}
	}
	return ES_NORMAL;
#endif /* MCBeep */
}

Exec_stat MCBreakPoint::exec(MCExecPoint &ep)
{
#ifdef /* MCBreakPoint */ LEGACY_EXEC
	MCB_break(ep, getline(), getpos());
	return ES_NORMAL;
#endif /* MCBreakPoint */
}

MCCancel::~MCCancel()
{
	delete id;
}

Parse_stat MCCancel::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp . skip_token(SP_RESET, TT_UNDEFINED, RT_PRINTING) == PS_NORMAL)
	{
		id = NULL;
	}
	else if (sp.parseexp(False, True, &id) != PS_NORMAL)
	{
		MCperror->add(PE_CANCEL_BADEXP, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCCancel::exec(MCExecPoint &ep)
{
#ifdef /* MCCancel */ LEGACY_EXEC
	if (id == NULL)
	{
		MCprinter -> Cancel();
		if (MCprinter != MCsystemprinter)
		{
			delete MCprinter;
			MCprinter = MCsystemprinter;
		}

		return ES_NORMAL;
	}

	if (id->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_CANCEL_IDNAN, line, pos);
		return ES_ERROR;
	}
	if (ep.getuint4() != 0)
		MCscreen->cancelmessageid(ep.getuint4());
	return ES_NORMAL;
#endif /* MCCancel */
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

Exec_stat MCClickCmd::exec(MCExecPoint &ep)
{
#ifdef /* MCClickCmd */ LEGACY_EXEC
	if (button != NULL)
	{
		if (button->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
		{
			MCeerror->add(EE_CLICK_BADBUTTON, line, pos);
			return ES_ERROR;
		}
		which = ep.getuint2();
	}
	if (location->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_CLICK_BADLOCATION, line, pos);
		return ES_ERROR;
	}
	int2 x, y;
	if (!MCU_stoi2x2(ep.getsvalue(), x, y))
	{
		MCeerror->add(EE_CLICK_NAP, line, pos, ep.getsvalue());
		return ES_ERROR;
	}
	if (!MCdefaultstackptr->getopened()
	        || !MCdefaultstackptr->mode_haswindow())
	{
		MCeerror->add(EE_CLICK_STACKNOTOPEN, line, pos, ep.getsvalue());
		return ES_ERROR;
	}
	uint2 oldmstate = MCmodifierstate;
	uint2 oldbstate = MCbuttonstate;
	int2 oldmx = MCmousex;
	int2 oldmy = MCmousey;
	MCmousex = x;
	MCmousey = y;
	MCStack *oldms = MCmousestackptr;
	MCmodifierstate = mstate;
	MCbuttonstate |= 0x1L << (which - 1);
	MCmousestackptr = MCdefaultstackptr;
	MCdispatcher->wmfocus_stack(MCdefaultstackptr, x, y);
	MCmodifierstate = mstate;
	MCbuttonstate |= 0x1L << (which - 1);
	MCdispatcher->wmdown_stack(MCdefaultstackptr, which);
	// **** NULL POINTER FIX
	if (MCmousestackptr != NULL)
		MCscreen->sync(MCmousestackptr->getw());
	Boolean abort = MCscreen->wait(CLICK_INTERVAL, False, False);
	MCclicklocx = x;
	MCclicklocy = y;
	MCmodifierstate = mstate;
	MCbuttonstate &= ~(0x1L << (which - 1));
	MCdispatcher->wmup_stack(MCdefaultstackptr, which);
	MCmodifierstate = oldmstate;
	MCbuttonstate = oldbstate;
	MCControl *mfocused = MCdefaultstackptr->getcard()->getmfocused();
	if (mfocused != NULL
	        && (mfocused->gettype() == CT_GRAPHIC
	            && mfocused->getstate(CS_CREATE_POINTS)
	            || (mfocused->gettype() == CT_IMAGE && mfocused->getstate(CS_DRAW)
	                && MCdefaultstackptr->gettool(mfocused) == T_POLYGON)))
		mfocused->doubleup(1); // cancel polygon create
	if (oldms == NULL || oldms->getmode() != 0)
	{
		MCmousestackptr = oldms;
		MCmousex = oldmx;
		MCmousey = oldmy;
		if (oldms != NULL)
			MCdispatcher->wmfocus_stack(oldms, oldmx, oldmy);
	}
	if (abort)
	{
		MCeerror->add(EE_CLICK_ABORT, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCClickCmd */
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

Exec_stat MCDrag::exec(MCExecPoint &ep)
{
#ifdef /* MCDrag */ LEGACY_EXEC
	if (button != NULL)
	{
		if (button->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
		{
			MCeerror->add(EE_DRAG_BADBUTTON, line, pos);
			return ES_ERROR;
		}
		which = ep.getuint2();
	}
	if (startloc->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_DRAG_BADSTARTLOC, line, pos);
		return ES_ERROR;
	}
	int2 sx, sy;
	if (!MCU_stoi2x2(ep.getsvalue(), sx, sy))
	{
		MCeerror->add(EE_DRAG_STARTNAP, line, pos, ep.getsvalue());
		return ES_ERROR;
	}
	if (endloc->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_DRAG_BADENDLOC, line, pos);
		return ES_ERROR;
	}
	int2 ex, ey;
	if (!MCU_stoi2x2(ep.getsvalue(), ex, ey))
	{
		MCeerror->add(EE_DRAG_ENDNAP, line, pos, ep.getsvalue());
		return ES_ERROR;
	}
	uint2 oldmstate = MCmodifierstate;
	uint2 oldbstate = MCbuttonstate;
	int2 oldx = MCmousex;
	int2 oldy = MCmousey;
	MCmodifierstate = mstate;
	MCbuttonstate = 0x1 << (which - 1);
	MCmousex = sx;
	MCmousey = sy;
	//MCdragging = True;
	MCscreen->setlockmods(True);
	MCdefaultstackptr->mfocus(sx, sy);
	MCdefaultstackptr->mdown(which);
	if (MCdragspeed == 0)
	{
		MCmousex = ex;
		MCmousey = ey;
		MCdefaultstackptr->mfocus(ex, ey);
		MCdefaultstackptr->mup(which);
		MCscreen->setlockmods(False);
		MCmodifierstate = oldmstate;
		MCbuttonstate = oldbstate;
		MCmousex = oldx;
		MCmousey = oldy;
		return ES_NORMAL;
	}
	MCscreen->sync(MCdefaultstackptr->getw());
	real8 dx = MCU_abs(ex - sx);
	real8 dy = MCU_abs(ey - sy);
	real8 ix = 0.0;
	if (dx != 0.0)
		ix = dx / (ex - sx);
	real8 iy = 0.0;
	if (dy != 0.0)
		iy = dy / (ey - sy);
	real8 starttime = MCS_time();
	real8 curtime = starttime;
	real8 duration = 0.0;
	if (MCdragspeed != 0)
		duration = sqrt((double)(dx * dx + dy * dy)) / (real8)MCdragspeed;
	real8 endtime = starttime + duration;
	Boolean abort = False;
	MCdragging = True;
	int2 x = sx;
	int2 y = sy;
	while (x != ex || y != ey)
	{
		int2 oldx = x;
		int2 oldy = y;
		x = sx + (int2)(ix * (dx * (curtime - starttime) / duration));
		y = sy + (int2)(iy * (dy * (curtime - starttime) / duration));
		if (MCscreen->wait((real8)MCsyncrate / 1000.0, False, True))
		{
			abort = True;
			break;
		}
		curtime = MCS_time();
		if (curtime >= endtime)
		{
			x = ex;
			y = ey;
			curtime = endtime;
		}
		if (x != oldx || y != oldy)
			MCdefaultstackptr->mfocus(x, y);
	}
	MCdefaultstackptr->mup(which);
	MCmodifierstate = oldmstate;
	MCbuttonstate = oldbstate;
	MCmousex = oldx;
	MCmousey = oldy;
	MCscreen->setlockmods(False);
	MCdragging = False;
	if (abort)
	{
		MCeerror->add(EE_DRAG_ABORT, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCDrag */
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
		object = new MCChunk(False);
		if (object->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_FOCUS_BADOBJECT, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

Exec_stat MCFocus::exec(MCExecPoint &ep)
{
#ifdef /* MCFocus */ LEGACY_EXEC
	MCObject *optr;
	uint4 parid;
	if (object == NULL)
	{
		if (MCfocusedstackptr != NULL && MCfocusedstackptr -> getcard() != NULL)
			MCfocusedstackptr -> getcard() -> kunfocus();
#ifdef _MOBILE
		// Make sure the IME is forced closed if explicitly asked to be.
		MCscreen -> closeIME();
#endif
	}
	else
	{
		if (object->getobj(ep, optr, parid, True) != ES_NORMAL
				|| optr->gettype() < CT_BUTTON || !optr->getflag(F_TRAVERSAL_ON))
		{
			MCeerror->add(EE_FOCUS_BADOBJECT, line, pos);
			return ES_ERROR;
		}
		optr->getstack()->kfocusset((MCControl *)optr);
	}
	return ES_NORMAL;
#endif /* MCFocus */
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
	target = new MCChunk(False);
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

Exec_stat MCInsert::exec(MCExecPoint &ep)
{
#ifdef /* MCInsert */ LEGACY_EXEC
	MCObject *optr;
	uint4 parid;
	if (target->getobj(ep, optr, parid, True) != ES_NORMAL
	        || !optr->parsescript(True))
	{
		MCeerror->add(EE_INSERT_BADTARGET, line, pos);
		return ES_ERROR;
	}
	MCObjectList *&listptr = where == IP_FRONT ? MCfrontscripts : MCbackscripts;
	optr->removefrom(listptr);
	uint4 count = 0;
	if (listptr != NULL)
	{
		MCObjectList *olptr = listptr;
		do
		{
			if (!olptr->getremoved())
				count++;
			olptr = olptr->next();
		}
		while (olptr != listptr);
	}
	if (MClicenseparameters . insert_limit > 0 && count >= MClicenseparameters . insert_limit)
	{
		MCeerror->add(EE_INSERT_NOTLICENSED, line, pos);
		return ES_ERROR;
	}
	MCObjectList *olptr = new MCObjectList(optr);
	olptr->insertto(listptr);
	return ES_NORMAL;
#endif /* MCInsert */
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
	delete it;
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
		target = new MCChunk(False);
		if (target -> parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_DISPATCH_BADTARGET, sp);
			return PS_ERROR;
		}
	}
	
	if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		if (getparams(sp, &params) != PS_NORMAL)
		{
			MCperror -> add(PE_DISPATCH_BADPARAMS, sp);
			return PS_ERROR;
		}
	}
	
	getit(sp, it);
	
	return PS_NORMAL;
}

// This method follows along the same lines as MCComref::exec
Exec_stat MCDispatchCmd::exec(MCExecPoint& ep)
{
#ifdef /* MCDispatchCmd */ LEGACY_EXEC
	if (MCscreen->abortkey())
	{
		MCeerror->add(EE_HANDLER_ABORT, line, pos);
		return ES_ERROR;
	}
	
	if (message -> eval(ep) != ES_NORMAL)
	{
		MCeerror -> add(EE_DISPATCH_BADMESSAGEEXP, line, pos);
		return ES_ERROR;
	}
	
	MCAutoNameRef t_message;
	/* UNCHECKED */ ep . copyasnameref(t_message);
	
	// Evaluate the target object (if we parsed a 'target' chunk).
	MCObject *t_object;
	uint4 t_object_part_id;
	if (target == NULL)
		t_object = ep.getobj();
	else if (target->getobj(ep, t_object, t_object_part_id, True) != ES_NORMAL)
	{
		MCeerror->add(EE_DISPATCH_BADTARGET, line, pos);
		return ES_ERROR;
	}
	
	// Evaluate the parameter list
	Exec_stat stat;
	MCParameter *tptr = params;
	while (tptr != NULL)
	{
		// Get the pointer to the variable this parameter maps to or NULL
		// if it is an expression.
		MCVariable* t_var;
		t_var = tptr -> evalvar(ep);

		if (t_var == NULL)
		{
			tptr -> clear_argument();
			while ((stat = tptr->eval(ep)) != ES_NORMAL && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
				MCB_error(ep, line, pos, EE_STATEMENT_BADPARAM);
			if (stat != ES_NORMAL)
			{
				MCeerror->add(EE_STATEMENT_BADPARAM, line, pos);
				return ES_ERROR;
			}
			tptr->set_argument(ep);
		}
		else
			tptr->set_argument_var(t_var);

		tptr = tptr->getnext();
	}
	
	// Fetch current default stack and target settings
	MCStack *t_old_stack;
	t_old_stack = MCdefaultstackptr;
	MCObject *t_old_target;
	t_old_target = MCtargetptr;
	
	// Cache the current 'this stack' (used to see if we should switch back
	// the default stack).
	MCStack *t_this_stack;
	t_this_stack = t_object -> getstack();
	
	// Retarget this stack and the target to be relative to the target object
	MCdefaultstackptr = t_this_stack;
	MCtargetptr = t_object;

	// MW-2012-10-30: [[ Bug 10478 ]] Turn off lockMessages before dispatch.
	Boolean t_old_lock;
	t_old_lock = MClockmessages;
	MClockmessages = False;
	
	// Add a new entry in the execution contexts
	MCExecPoint *oldep = MCEPptr;
	MCEPptr = &ep;
	stat = ES_NOT_HANDLED;
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ep.setline(line);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
		added = True;
	}

	// Dispatch the message
	stat = MCU_dofrontscripts(is_function ? HT_FUNCTION : HT_MESSAGE, t_message, params);
	Boolean olddynamic = MCdynamicpath;
	MCdynamicpath = MCdynamiccard != NULL;
	if (stat == ES_PASS || stat == ES_NOT_HANDLED)
		switch(stat = t_object->handle(is_function ? HT_FUNCTION : HT_MESSAGE, t_message, params, t_object))
		{
		case ES_ERROR:
			MCeerror -> add(EE_DISPATCH_BADCOMMAND, line, pos, t_message);
			break;
		default:
			break;
		}
	
	// Set 'it' appropriately
	switch(stat)
	{
	case ES_NOT_HANDLED:
	case ES_NOT_FOUND:
		it -> sets(MCString("unhandled"));
		stat = ES_NORMAL;
		break;
		
	case ES_PASS:
		it -> sets(MCString("passed"));
		stat = ES_NORMAL;
		break;
	
	case ES_EXIT_HANDLER:
	case ES_NORMAL:
		it -> sets(MCString("handled"));
		stat = ES_NORMAL;
	break;
	
	default:
		it -> clear();
	break;
	}
	
	// Reset the default stack pointer and target - note that we use 'send'esque
	// semantics here. i.e. If the default stack has been changed, the change sticks.
	if (MCdefaultstackptr == t_this_stack)
		MCdefaultstackptr = t_old_stack;

	// Reset target pointer
	MCtargetptr = t_old_target;
	MCdynamicpath = olddynamic;
	
	// MW-2012-10-30: [[ Bug 10478 ]] Restore lockMessages.
	MClockmessages = t_old_lock;
	
	// Remove our entry from the contexts list
	MCEPptr = oldep;
	if (added)
		MCnexecutioncontexts--;
	
	return stat;
#endif /* MCDispatchCmd */
}

MCMessage::~MCMessage()
{
	delete message;
	delete eventtype;
	delete target;
	delete in;
}

Parse_stat MCMessage::parse(MCScriptPoint &sp)
{
	initpoint(sp);

	h = sp.gethandler();
	if (sp.parseexp(False, True, &message) != PS_NORMAL)
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
		if (sp.parseexp(False, True, &in) != PS_NORMAL)
		{
			MCperror->add(PE_SEND_BADTARGET, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL
		        && sp.skip_token(SP_COMMAND, TT_STATEMENT, S_REPLY) != PS_NORMAL)
			if (sp.parseexp(False, True, &eventtype) != PS_NORMAL)
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
		target = new MCChunk(False);
		if (target->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_SEND_BADTARGET, sp);
			return PS_ERROR;
		}
		return gettime(sp, &in, units);
	}
	return PS_NORMAL;
}

Exec_stat MCMessage::exec(MCExecPoint &ep)
{
#ifdef /* MCMessage */ LEGACY_EXEC
	if (program)
	{
		if (MCsecuremode & MC_SECUREMODE_PROCESS)
		{
			MCeerror->add(EE_PROCESS_NOPERM, line, pos);
			return ES_ERROR;
		}
		if (in->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_SEND_BADPROGRAMEXP, line, pos);
			return ES_ERROR;
		}
		char *pname = ep.getsvalue().clone();
		char *etstring = NULL;
		if (eventtype != NULL)
		{
			if (eventtype->eval(ep) != ES_NORMAL || ep.getsvalue().getlength() != 8)
			{
				MCeerror->add(EE_SEND_BADEXP, line, pos);
				return ES_ERROR;
			}
			etstring = ep.getsvalue().clone();
		}
		if (message->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_SEND_BADEXP, line, pos);
			delete pname;
			delete etstring;
			return ES_ERROR;
		}
		MCS_send(ep.getsvalue(), pname, etstring, reply);
		delete pname;
		delete etstring;
		return ES_NORMAL;
	}
	MCObject *optr;
	uint4 parid;
	if (target == NULL)
		optr = ep.getobj();
	else
		if (target->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_SEND_BADTARGET, line, pos);
			return ES_ERROR;
		}
	real8 delay = 0.0;
	if (in != NULL)
	{
		if (in->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
		{
			MCeerror->add(EE_SEND_BADINEXP, line, pos);
			return ES_ERROR;
		}
		delay = ep.getnvalue();
		switch (units)
		{
		case F_MILLISECS:
			delay /= 1000.0;
			break;
		case F_TICKS:
			delay /= 60.0;
			break;
		default:
			break;
		}
	}
	if (message->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_SEND_BADEXP, line, pos);
		return ES_ERROR;
	}
	char *mptr = ep.getsvalue().clone();
	MCParameter *params = NULL;
	MCParameter *tparam = NULL;
	char *sptr = mptr;
	while (*sptr && !isspace((uint1)*sptr))
		sptr++;
	MCerrorlock++;
	if (*sptr)
	{
		*sptr++ = '\0';
		char *startptr = sptr;
		while (*startptr)
		{
			while (*sptr && *sptr != ',')
				if (*sptr == '"')
				{
					sptr++;
					while (*sptr && *sptr++ != '"')
						;
				}
				else
					sptr++;
			if (*sptr)
				*sptr++ = '\0';
			MCString pdata = startptr;
			ep.setsvalue(pdata);
			
			MCParameter *newparam = new MCParameter;

			// MW-2011-08-11: [[ Bug 9668 ]] Make sure we copy 'pdata' if we use it, since
			//   mptr (into which it points) only lasts as long as this method call.
			if (h->eval(ep) == ES_NORMAL)
				newparam->set_argument(ep);
			else
				newparam->copysvalue_argument(pdata);

			if (tparam == NULL)
				params = tparam = newparam;
			else
			{
				tparam->setnext(newparam);
				tparam = newparam;
			}
			startptr = sptr;
		}
	}
	MCerrorlock--;

	// Convert the message c-string to a name
	MCAutoNameRef t_mptr_as_name;
	/* UNCHECKED */ t_mptr_as_name . CreateWithCString(mptr);

	if (in == NULL)
	{
		Boolean oldlock = MClockmessages;
		MClockmessages = False;
		Exec_stat stat;
		Boolean added = False;
		if (MCnexecutioncontexts < MAX_CONTEXTS)
		{
			ep.setline(line);
			MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
			added = True;
		}
		if ((stat = optr->message(t_mptr_as_name, params, send, True)) == ES_NOT_HANDLED)
		{
			MCHandler *t_handler;
			t_handler = optr -> findhandler(HT_MESSAGE, t_mptr_as_name);
			if (t_handler != NULL && t_handler -> isprivate())
				MCeerror -> add(EE_SEND_BADEXP, line, pos, mptr);
			else
			{
				char *tptr = mptr;
				if (params != NULL)
				{
					params->eval(ep);
					char *p = ep.getsvalue().clone();
					tptr = new char[strlen(mptr) + ep.getsvalue().getlength() + 2];
					sprintf(tptr, "%s %s", mptr, p);
					delete p;
				}
				if ((stat = optr->domess(tptr)) == ES_ERROR)
					MCeerror->add(EE_STATEMENT_BADCOMMAND, line, pos, mptr);
				if (tptr != mptr)
					delete tptr;
			}
		}
		else if (stat == ES_PASS)
			stat = ES_NORMAL;
		else if (stat == ES_ERROR)
			MCeerror->add(EE_SEND_BADEXP, line, pos, mptr);
		while (params != NULL)
		{
			MCParameter *tmp = params;
			params = params->getnext();
			delete tmp;
		}
		delete mptr;
		if (added)
			MCnexecutioncontexts--;
		MClockmessages = oldlock;
		return stat;
	}
	else
	{
		MCscreen->addmessage(optr, t_mptr_as_name, MCS_time() + delay, params);
		delete mptr;
	}
	return ES_NORMAL;
#endif /* MCMessage */
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
	object = new MCChunk(False);
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

Exec_stat MCMove::exec(MCExecPoint &ep)
{
#ifdef /* MCMove */ LEGACY_EXEC
	MCObject *optr;
	uint4 parid;
	if (object->getobj(ep, optr, parid, True) != ES_NORMAL)
	{
		MCeerror->add(EE_MOVE_BADOBJECT, line, pos);
		return ES_ERROR;
	}
	if (endloc->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_MOVE_BADENDLOC, line, pos);
		return ES_ERROR;
	}
	MCPoint *pts;
	uint2 npts;
	if (startloc != NULL)
	{
		pts = new MCPoint[2];
		npts = 2;
		if (!MCU_stoi2x2(ep.getsvalue(), pts[1].x, pts[1].y))
		{
			MCeerror->add(EE_MOVE_ENDNAP, line, pos, ep.getsvalue());
			delete pts;
			return ES_ERROR;
		}
		if (startloc->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_MOVE_BADSTARTLOC, line, pos);
			return ES_ERROR;
		}
		if (!MCU_stoi2x2(ep.getsvalue(), pts[0].x, pts[0].y))
		{
			MCeerror->add(EE_MOVE_STARTNAP, line, pos, ep.getsvalue());
			return ES_ERROR;
		}
	}
	else
	{
		MCPoint *tpts = NULL;
		uint2 tnpts = 0;
		if (!MCU_parsepoints(tpts, tnpts, ep.getsvalue()))
		{
			MCeerror->add(EE_MOVE_ENDNAP, line, pos, ep.getsvalue());
			delete tpts;
			return ES_ERROR;
		}
		if (tnpts == 1)
		{
			pts = new MCPoint[2];
			npts = 2;
			MCRectangle trect = optr->getrect();
			pts[0].x = trect.x + (trect.width >> 1);
			pts[0].y = trect.y + (trect.height >> 1);
			pts[1] = tpts[0];
			delete tpts;
			if (relative)
			{
				pts[1].x += pts[0].x;
				pts[1].y += pts[0].y;
			}
		}
		else
		{
			pts = tpts;
			npts = tnpts;
		}
	}
	if (npts < 2)
		return ES_NORMAL;
	real8 duration = 0.0;
	if (durationexp != NULL)
	{
		if (durationexp->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_MOVE_BADDURATION, line, pos);
			return ES_ERROR;
		}
		if (ep.getreal8(duration, line, pos, EE_MOVE_DURATIONNAN) != ES_NORMAL)
			return ES_ERROR;
		switch (units)
		{
		case F_MILLISECS:
			duration /= 1000.0;
			break;
		case F_TICKS:
			duration /= 60.0;
			break;
		default:
			break;
		}
	}
	MCscreen->addmove(optr, pts, npts, duration, waiting);
	if (waiting)
	{
		if (MCscreen->wait(duration, messages, False))
		{
			MCeerror->add(EE_MOVE_ABORT, line, pos);
			return ES_ERROR;
		}
		MCscreen->stopmove(optr, True);
	}
	return ES_NORMAL;
#endif /* MCMove */
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
		stack = new MCChunk(False);
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
		pause = False;
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
		stack = new MCChunk(False);
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

Exec_stat MCMM::exec(MCExecPoint &ep)
{
#ifdef /* MCMM */ LEGACY_EXEC
	MCresult->clear(False);
	if (prepare && image)
	{
		uint4 parid;
		MCObject *t_object;
		if (stack -> getobj(ep, t_object, parid, True) != ES_NORMAL ||
			t_object -> gettype() != CT_IMAGE)
		{
			MCeerror->add(EE_PLAY_BADCLIP, line, pos);
			return ES_ERROR;
		}
		static_cast<MCImage *>(t_object) -> prepareimage();
		return ES_NORMAL;
	}
	else if (prepare && image_file)
	{
		if (clip->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_PLAY_BADCLIP, line, pos);
			return ES_ERROR;
		}
		
		char *t_filename;
		t_filename = MCS_resolvepath(ep . getcstring());
		
		MCU_fix_path(t_filename);
		
		MCImageRep *t_rep;
		MCImageRepGetReferenced(t_filename, t_rep);
		
		uindex_t t_width, t_height;
		t_rep -> GetGeometry(t_width, t_height);
		
		t_rep -> Release();
		
		return ES_NORMAL;
	}
	
	if (audio)
		MCU_play_stop();
	if (stop)
	{
#ifdef _MOBILE
		extern bool MCSystemStopVideo();
		if (!MCSystemStopVideo())
			MCresult->sets("no video support");
		return ES_NORMAL;
#endif
	}
	if (clip == NULL)
	{
		if (video && MCplayers != NULL)
			if (stepforward)
				MCplayers->playstepforward();
			else if (stepback)
					MCplayers->playstepback();
			else if (pause)
						MCplayers->playpause(True);
			else if (stop)
							if (MCplayers->isdisposable())
							{
								MCPlayer *tptr = MCplayers;
								tptr->playstop();
							}
							else
								MCplayers->playpause(True);
			else if (!MCplayers->playpause(False))
							{
								MCPlayer *tptr = MCplayers;
								tptr->playstop();
							}
		return ES_NORMAL;
	}
	MCStack *sptr = MCdefaultstackptr;
	if (stack != NULL)
	{
		uint4 parid;
		MCObject *optr = MCdefaultstackptr;
		if (stack->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_PLAY_BADCLIP, line, pos);
			return ES_ERROR;
		}
		sptr = optr->getstack();
	}
	if (clip->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_PLAY_BADCLIP, line, pos);
		return ES_ERROR;
	}
	if (video)
	{
#ifdef _MOBILE
		extern bool MCSystemPlayVideo(const char *p_filename);
		MCExecPoint *t_old_ep;
		t_old_ep = MCEPptr;
		MCEPptr = &ep;
		if (!MCSystemPlayVideo(ep.getcstring()))
			MCresult->sets("no video support");
		MCEPptr = t_old_ep;
		return ES_NORMAL;
#endif
	}
	if (video || player)
	{
		IO_cleanprocesses();
		MCPlayer *tptr;
		if (player)
			tptr = (MCPlayer *)sptr->getcard()->getchild(etype, ep.getsvalue(), CT_PLAYER, ptype);
		else
		{
			// Lookup the name we are searching for. If it doesn't exist, then no object can
			// have it as a name.
			tptr = nil;
			if (etype == CT_EXPRESSION)
			{
				MCNameRef t_obj_name;
				t_obj_name = MCNameLookupWithOldString(ep.getsvalue(), kMCCompareCaseless);
				if (t_obj_name != nil)
				{
			tptr = MCplayers;
			while (tptr != NULL)
			{
						if (tptr -> hasname(t_obj_name))
							break;
						tptr = tptr->getnextplayer();
					}
				}
			}
			else if (etype == CT_ID)
				{
				tptr = MCplayers;
				while (tptr != NULL)
				{
					if (tptr -> getaltid() == ep.getnvalue())
					break;
				tptr = tptr->getnextplayer();
			}
		}
			if (tptr != nil && !prepare)
				tptr->setflag(True, F_VISIBLE);
		}
		if (tptr != NULL)
		{
			if (pause)
				tptr->playpause(True);
			else if (stepforward)
					tptr->playstepforward();
			else if (stepback)
						tptr->playstepback();
			else if (stop)
							if (tptr->isdisposable())
								tptr->playstop();
							else
								tptr->playpause(True);
			else if (!prepare)
								if (!tptr->playpause(False))
									tptr->playstop();
			return ES_NORMAL;
		}
		if (pause || stop || stepforward || stepback)
		{
			MCresult->sets("videoClip is not playing");
			return ES_NORMAL;
		}
		const char *vcname = NULL;
		char *fname = NULL;
		Boolean tmpfile = False;
		MCVideoClip *vcptr;
		real8 scale;
		Boolean dontrefresh;
		if ((vcptr = (MCVideoClip *)sptr->getAV(etype, ep.getsvalue(),
		                                        CT_VIDEO_CLIP)) == NULL
		        && (vcptr = (MCVideoClip *)sptr->getobjname(CT_VIDEO_CLIP,
		                    ep.getsvalue())) == NULL)
		{
			if (ep.getsvalue().getlength() < 4096)
			{
				fname = ep.getsvalue().clone();
				vcname = fname;
				if (!MCS_exists(fname, True))
				{
					delete fname;
					fname = NULL;
					MCU_geturl(ep);
					if (ep.getsvalue().getlength() == 0)
					{
						MCresult->sets("no data in videoClip");
						return ES_NORMAL;
					}
				}
			}
			if (fname == NULL)
			{
				fname = strclone(MCS_tmpnam());
				IO_handle tstream;
				if ((tstream = MCS_open(fname, IO_WRITE_MODE, False, False, 0)) == NULL)
				{
					delete fname;
					MCresult->sets("error opening temp file");
					return ES_NORMAL;
				}
				IO_stat stat = IO_write(ep.getsvalue().getstring(), sizeof(int1),
				                        ep.getsvalue().getlength(), tstream);
				MCS_close(tstream);
				if (stat != IO_NORMAL)
				{
					MCS_unlink(fname);
					delete fname;
					MCresult->sets("error writing videoClip");
					return ES_NORMAL;
				}
				tmpfile = True;
			}
			scale = MCtemplatevideo->getscale();
			dontrefresh = MCtemplatevideo->getflag(F_DONT_REFRESH);
		}
		else
		{
			vcname = vcptr->getname_cstring();
			fname = vcptr->getfile();
			scale = vcptr->getscale();
			dontrefresh = vcptr->getflag(F_DONT_REFRESH);
			tmpfile = True;
		}
		tptr = (MCPlayer *)MCtemplateplayer->clone(False, OP_NONE, false);
		tptr->setsprop(P_SHOW_BORDER, MCfalsemcstring);
		tptr->setfilename(vcname, fname, tmpfile);
		tptr->open();
		if (prepare)
			tptr->setflag(False, F_VISIBLE);
		MCRectangle trect = tptr->getrect();
		if (loc != NULL)
		{
			if (loc->eval(ep) != ES_NORMAL)
			{
				if (tptr->isdisposable())
					delete tptr;
				MCeerror->add(EE_PLAY_BADLOC, line, pos);
				return ES_ERROR;
			}
			if (!MCU_stoi2x2(ep.getsvalue(), trect.x, trect.y))
			{
				if (tptr->isdisposable())
					delete tptr;
				MCeerror->add(EE_PLAY_BADLOC, line, pos, ep.getsvalue());
				return ES_ERROR;
			}
		}
		else
		{
			MCRectangle crect = tptr->getcard()->getrect();
			trect.x = crect.width >> 1;
			trect.y = crect.height >> 1;
		}
		trect.width = trect.height = 1;
		tptr->setrect(trect);
		tptr->setscale(scale);
		tptr->setflag(dontrefresh, F_DONT_REFRESH);
		char *optionstring;
		if (options != NULL)
		{
			if (options->eval(ep) != ES_NORMAL)
			{
				if (tptr->isdisposable())
					delete tptr;
				MCeerror->add(EE_PLAY_BADOPTIONS, line, pos);
				return ES_ERROR;
			}
			optionstring = ep.getsvalue().clone();
		}
		else
			optionstring = strclone(MCnullstring);
		if (looping)
			tptr->setflag(True, F_LOOPING);
		if (prepare && !tptr->prepare(optionstring)
		        || !prepare && !tptr->playstart(optionstring))
		{
			if (tptr->isdisposable())
				delete tptr;
			delete optionstring;
			return ES_NORMAL;
		}
		delete optionstring;
	}
	else
	{
		if (!MCtemplateaudio->issupported())
		{
#ifdef _MOBILE
			extern bool MCSystemPlaySound(const char *p_filename, bool p_looping);
			if (!MCSystemPlaySound(ep.getcstring(), looping == True))
			MCresult->sets("no sound support");
#endif
			return ES_NORMAL;
		}
		if ((MCacptr = (MCAudioClip *)(sptr->getAV(etype, ep.getsvalue(),
		                               CT_AUDIO_CLIP))) == NULL
		        && (MCacptr = (MCAudioClip *)sptr->getobjname(CT_AUDIO_CLIP,
		                      ep.getsvalue())) == NULL)
		{
			char *fname = ep.getsvalue().clone();
			IO_handle stream;
			if (!MCS_exists(fname, True)
			        || (stream = MCS_open(fname, IO_READ_MODE, True, False, 0)) == NULL)
			{
				MCU_geturl(ep);
				if (ep.getsvalue().getlength() == 0)
				{
					delete fname;
					MCresult->sets("no data in audioClip");
					return ES_NORMAL;
				}
				stream = MCS_fakeopen(ep.getsvalue());
			}
			MCacptr = new MCAudioClip;
			MCacptr->setdisposable();
			if (!MCacptr->import(fname, stream))
			{
				MCS_close(stream);
				MCresult->sets("error reading audioClip");
				delete fname;
				delete MCacptr;
				MCacptr = NULL;
				return ES_ERROR;
			}
			MCS_close(stream);
			delete fname;
		}
		MCacptr->setlooping(looping);
		MCU_play();
		if (MCacptr != NULL)
			MCscreen->addtimer(MCacptr, MCM_internal, looping ? LOOP_RATE : PLAY_RATE);
	}
	return ES_NORMAL;
#endif /* MCMM */
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

Exec_stat MCReply::exec(MCExecPoint &ep)
{
#ifdef /* MCReply */ LEGACY_EXEC
	char *k = NULL;
	if (keyword != NULL)
	{
		if (message->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_REPLY_BADKEYWORDEXP, line, pos);
			return ES_ERROR;
		}
		k = ep.getsvalue().clone();
	}
	if (message->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_REPLY_BADMESSAGEEXP, line, pos);
		delete k;
		return ES_ERROR;
	}
	MCS_reply(ep.getsvalue(), k, error);
	delete k;
	return ES_NORMAL;
#endif /* MCReply */
}

MCRequest::~MCRequest()
{
	delete message;
	delete program;
	delete it;
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
	getit(sp, it);
	return PS_NORMAL;
}

Exec_stat MCRequest::exec(MCExecPoint &ep)
{
#ifdef /* MCRequest */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_PROCESS)
	{
		MCeerror->add(EE_REQUEST_NOPERM, line, pos);
		return ES_ERROR;
	}
	char *result = NULL;
	if (ae != AE_UNDEFINED)
	{
		if (program == NULL)
			ep.clear();
		else
			if (program->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_REQUEST_BADKEYWORDEXP, line, pos);
				return ES_ERROR;
			}
		result = MCS_request_ae(ep.getsvalue(), ae);
	}
	else
	{
		if (program->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_REQUEST_BADPROGRAMEXP, line, pos);
			return ES_ERROR;
		}
		char *p = ep.getsvalue().clone();
		if (message->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_REQUEST_BADMESSAGEEXP, line, pos);
			delete p;
			return ES_ERROR;
		}
		result = MCS_request_program(ep.getsvalue(), p);
		delete p;
	}
	if (result == NULL)
		ep.clear();
	else
	{
		ep.copysvalue(result, strlen(result));
		delete result;
	}
	it->set(ep);
	return ES_NORMAL;
#endif /* MCRequest */
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
			target = new MCChunk(False);
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
		target = new MCChunk(False);
		if (target->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_START_BADCHUNK, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

bool MCServerStartSession();
Exec_stat MCStart::exec(MCExecPoint &ep)
{
#ifdef /* MCStart */ LEGACY_EXEC
	if (mode == SC_USING)
	{
        // TD-2013-06-12: [[ DynamicFonts ]] Look for font.
        if (font != NULL)
        {
            if (MCsecuremode & MC_SECUREMODE_DISK)
            {
                MCeerror->add(EE_DISK_NOPERM, line, pos);
                return ES_ERROR;
            }
            
            // MERG-2013-08-14: [[ DynamicFonts ]] Refactored to use MCFontLoad
            if (font->eval(ep) != ES_NORMAL)
            {
				MCeerror->add(EE_FONT_BADFILEEXP, line, pos);
				return ES_ERROR;
			}
			
			MCAutoPointer<char> t_resolved_path;
			t_resolved_path = MCS_resolvepath(ep . getcstring());
            if (MCFontLoad(ep, *t_resolved_path , is_globally) != ES_NORMAL)
				MCresult -> sets("can't load font file");
			else
				MCresult -> clear();
        }
        else
        {
            MCStack *sptr = NULL;
            if (target != NULL)
            {
                MCObject *optr;
                uint4 parid;
                
                if (target->getobj(ep, optr, parid, True) != ES_NORMAL
                        || optr->gettype() != CT_STACK)
                {
                    MCeerror->add(EE_START_BADTARGET, line, pos);
                    return ES_ERROR;
                }
                sptr = (MCStack *)optr;
            }
            else
            {
                if (stack->eval(ep) != ES_NORMAL
                        || (sptr = MCdefaultstackptr->findstackname(ep.getsvalue())) == NULL
                        || !sptr->parsescript(True))
                {
                    MCeerror->add(EE_START_BADTARGET, line, pos);
                    return ES_ERROR;
                }
            }
            uint2 i = MCnusing;
            while (i--)
                if (MCusing[i] == sptr)
                {
                    MCnusing--;
                    while (i < MCnusing)
                    {
                        MCusing[i] = MCusing[i + 1];
                        i++;
                    }
                    break;
                }

            if (MClicenseparameters . using_limit > 0 && MCnusing >= MClicenseparameters . using_limit)
            {
                MCeerror->add(EE_START_NOTLICENSED, line, pos);
                return ES_ERROR;
            }
            MCU_realloc((char **)&MCusing, MCnusing, MCnusing + 1, sizeof(MCStack *));
            MCusing[MCnusing++] = sptr;
            if (sptr->message(MCM_library_stack) == ES_ERROR)
                return ES_ERROR;
        }
	}
	else if (mode == SC_SESSION)
	{
#ifdef _SERVER
		if (!MCServerStartSession())
		{
			MCeerror->add(EE_UNDEFINED, line, pos);
			return ES_ERROR;
		}
#else
		MCeerror->add(EE_SESSION_BADCONTEXT, line, pos);
		return ES_ERROR;
#endif
	}
	else
	{
		MCObject *optr;
		uint4 parid;
		if (target->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_START_BADTARGET, line, pos);
			return ES_ERROR;
		}
		if (optr->gettype() == CT_PLAYER)
		{
			MCPlayer *pptr = (MCPlayer *)optr;
			pptr->playstart(NULL);
		}
		else
		{
			if (optr->gettype() != CT_GROUP)
			{
				MCeerror->add(EE_START_NOTABACKGROUND, line, pos);
				return ES_ERROR;
			}
			if (optr->getstack()->islocked())
			{
				MCeerror->add(EE_START_LOCKED, line, pos);
				return ES_ERROR;
			}
			MCGroup *gptr = (MCGroup *)optr;
			gptr->getstack()->startedit(gptr);
		}
	}
	return ES_NORMAL;
#endif /* MCStart */
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
			target = new MCChunk(False);
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
		target = new MCChunk(False);
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
Exec_stat MCStop::exec(MCExecPoint &ep)
{
#ifdef /* MCStop */ LEGACY_EXEC
	MCObject *optr = NULL;
	uint4 parid;

	if (target != NULL)
		if (target->getobj(ep, optr, parid, True) != ES_NORMAL
		        || optr == NULL && mode != SC_EDITING)
		{
			MCeerror->add(EE_STOP_BADTARGET, line, pos);
			return ES_ERROR;
		}
	switch (mode)
	{
	case SC_EDITING:
		if (optr != NULL)
		{
			if (optr->gettype() != CT_GROUP)
			{
				MCeerror->add(EE_STOP_NOTABACKGROUND, line, pos);
				return ES_ERROR;
			}
			MCGroup *gptr = (MCGroup *)optr;
			gptr->getstack()->stopedit();
		}
		else
			MCdefaultstackptr->stopedit();
		break;
	case SC_MOVING:
		MCscreen->stopmove(optr, False);
		break;
	case SC_PLAYER:
	case SC_PLAYING:
		if (optr == NULL)
			MCU_play_stop();
		else
			if (optr->gettype() == CT_PLAYER)
			{
				MCPlayer *player = (MCPlayer *)optr;
				if (player->isdisposable())
					player->playstop();
				else
					player->playpause(True);
			}
			else
				MCU_play_stop();
		break;
	case SC_RECORDING:
		MCtemplateplayer->stoprecording();
		break;
	case SC_USING:
		{
            // TD-2013-06-12: [[ DynamicFonts ]] Look for font.
            if (font != NULL)
            {
                // MERG-2013-08-14: [[ DynamicFonts ]] Refactored to use MCFontUnload
                if (font->eval(ep) != ES_NORMAL)
				{
					MCeerror->add(EE_FONT_BADFILEEXP, line, pos);
					return ES_ERROR;
				}
                	
				MCAutoPointer<char> t_resolved_path;
				t_resolved_path = MCS_resolvepath(ep . getcstring());
				if (MCFontUnload(ep, *t_resolved_path) != ES_NORMAL)
					MCresult -> sets("can't unload font file");
				else
					MCresult -> clear();
            }
            else
            {
                MCStack *sptr = NULL;
                if (target != NULL)
                {
                    MCObject *optr;
                    uint4 parid;
                    if (target->getobj(ep, optr, parid, True) != ES_NORMAL
                            || optr->gettype() != CT_STACK)
                    {
                        MCeerror->add(EE_STOP_BADTARGET, line, pos);
                        return ES_ERROR;
                    }
                    sptr = (MCStack *)optr;
                }
                else
                    if (stack->eval(ep) != ES_NORMAL
                            || (sptr = MCdefaultstackptr->findstackname(ep.getsvalue())) == NULL)
                    {
                        MCeerror->add(EE_STOP_BADTARGET, line, pos);
                        return ES_ERROR;
                    }
                uint2 i = MCnusing;
                while (i--)
                    if (MCusing[i] == sptr)
                    {
                        MCnusing--;
                        while (i < MCnusing)
                        {
                            MCusing[i] = MCusing[i + 1];
                            i++;
                        }
                        break;
                    }
                sptr->message(MCM_release_stack);
            }
		}
		break;
	case SC_SESSION:
		{
#ifdef _SERVER
			if (!MCServerStopSession())
			{
				MCeerror->add(EE_UNDEFINED, line, pos);
				return ES_ERROR;
			}
#else
			MCeerror->add(EE_SESSION_BADCONTEXT, line, pos);
			return ES_ERROR;
#endif
		}
	default:
		break;
	}
	return ES_NORMAL;
#endif /* MCStop */
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

Exec_stat MCType::exec(MCExecPoint &ep)
{
#ifdef /* MCType */ LEGACY_EXEC
	if (message->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_TYPE_BADSTRINGEXP, line, pos);
		return ES_ERROR;
	}
	uint2 oldstate = MCmodifierstate;
	MCmodifierstate = mstate;
	MCdefaultstackptr->kfocus();
	const char *sptr = ep.getsvalue().getstring();
	uint2 length = ep.getsvalue().getlength();
	uint2 i;
	char string[2];
	string[1] = '\0';
	real8 nexttime = MCS_time();
	for (i = 0 ; i < length ; i++)
	{
		KeySym keysym = (unsigned char)sptr[i];
		if (keysym < 0x20 || keysym == 0xFF)
		{
			if (keysym == 0x0A)
				keysym = 0x0D;
			keysym |= 0xFF00;
			string[0] = '\0';
		}
		else
			string[0] = sptr[i];
		MCdefaultstackptr->kdown(string, keysym);
		MCdefaultstackptr->kup(string, keysym);
		nexttime += (real8)MCtyperate / 1000.0;
		real8 delay = nexttime - MCS_time();
		if (MCscreen->wait(delay, False, False))
		{
			MCmodifierstate = oldstate;
			MCeerror->add(EE_TYPE_ABORT, line, pos);
			return ES_ERROR;
		}
	}
	MCmodifierstate = oldstate;
	return ES_NORMAL;
#endif /* MCType */
}
