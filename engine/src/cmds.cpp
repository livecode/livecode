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

#include "globals.h"
#include "execpt.h"
#include "handler.h"
#include "scriptpt.h"
#include "newobj.h"
#include "chunk.h"
#include "param.h"
#include "mcerror.h"
#include "property.h"
#include "object.h"
#include "button.h"
#include "field.h"
#include "image.h"
#include "card.h"
#include "eps.h"
#include "graphic.h"
#include "group.h"
#include "scrolbar.h"
#include "player.h"
#include "aclip.h"
#include "vclip.h"
#include "stack.h"
#include "dispatch.h"
#include "stacklst.h"
#include "sellst.h"
#include "undolst.h"
#include "util.h"
#include "date.h"
#include "printer.h"
#include "debug.h"
#include "cmds.h"
#include "mode.h"
#include "osspec.h"
#include "hndlrlst.h"

#include "core.h"

#include "securemode.h"
#include "stacksecurity.h"

#include "license.h"

#ifdef _SERVER
#include "srvscript.h"
#endif

MCChoose::~MCChoose()
{
	delete etool;
}

Parse_stat MCChoose::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	sp.skip_token(SP_FACTOR, TT_THE);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_CHOOSE_NOTOKEN, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_TOOL, te) != PS_NORMAL)
	{
		sp.backup();
		if (sp.parseexp(False, True, &etool) != PS_NORMAL)
		{
			MCperror->add(PE_CHOOSE_BADEXP, sp);
			return PS_ERROR;
		}
		sp.skip_token(SP_TOOL, TT_END);
		return PS_NORMAL;
	}
	else
	{
		littool = (Tool)te->which;
		while (sp.skip_token(SP_TOOL, TT_TOOL) == PS_NORMAL)
			;
		sp.skip_token(SP_TOOL, TT_END);
	}
	return PS_NORMAL;
}

Exec_stat MCChoose::exec(MCExecPoint &ep)
{
#ifdef /* MCChoose */ LEGACY_EXEC
	if (etool != NULL)
		if (etool->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_CHOOSE_BADEXP, line, pos);
			return ES_ERROR;
		}
	return MCU_choose_tool(ep, littool, line, pos);
#endif /* MCChoose */
}

MCConvert::~MCConvert()
{
	delete container;
	delete source;
	delete it;
}

Parse_stat MCConvert::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	MCerrorlock++;
	container = new MCChunk(True);
	MCScriptPoint tsp(sp);
	if (container->parse(sp, False) != PS_NORMAL)
	{
		sp = tsp;
		MCerrorlock--;
		delete container;
		container = NULL;
		if (sp.parseexp(False, True, &source) != PS_NORMAL)
		{
			MCperror->add
			(PE_CONVERT_NOCONTAINER, sp);
			return PS_ERROR;
		}
		getit(sp, it);
	}
	else
		MCerrorlock--;
	if (sp.skip_token(SP_FACTOR, TT_FROM) == PS_NORMAL)
	{
		if (parsedtformat(sp, fform, fsform) != PS_NORMAL)
			return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_TO) != PS_NORMAL)
	{
		MCperror->add
		(PE_CONVERT_NOTO, sp);
		return PS_ERROR;
	}
	if (parsedtformat(sp, pform, sform) != PS_NORMAL)
		return PS_ERROR;
	return PS_NORMAL;
}

Parse_stat MCConvert::parsedtformat(MCScriptPoint &sp, Convert_form &firstform,
                                    Convert_form &secondform)
{
	const LT *te;
	Symbol_type type;
	Boolean needformat = True;
	Convert_form localeform = CF_UNDEFINED;
	while (True)
	{
		if (sp.next(type) != PS_NORMAL)
		{
			if (needformat)
			{
				MCperror->add
				(PE_CONVERT_NOFORMAT, sp);
				return PS_ERROR;
			}
			else
				return PS_NORMAL;
		}
		if (sp.lookup(SP_CONVERT, te) != PS_NORMAL)
		{
			if (needformat)
			{
				MCperror->add
				(PE_CONVERT_NOTFORMAT, sp);
				return PS_ERROR;
			}
			else
			{
				sp.backup();
				break;
			}
		}
		switch (te->which)
		{
		case CF_ABBREVIATED:
		case CF_SHORT:
		case CF_LONG:
		case CF_INTERNET:
			if (firstform == CF_UNDEFINED)
				firstform = (Convert_form)te->which;
			else
				secondform = (Convert_form)te->which;
			break;
		case CF_SYSTEM:
		case CF_ENGLISH:
			localeform = (Convert_form)te->which;
			break;
		case CF_DATE:
		case CF_TIME:
			if (firstform == CF_UNDEFINED)
				firstform = (Convert_form)(te->which + 1 + localeform);
			else
				if (firstform > CF_INTERNET)
				{
					if (secondform == CF_UNDEFINED)
						secondform = (Convert_form)(te->which + 1 + localeform);
					else
					{
						uint2 dummy = secondform;
						dummy += te->which + localeform;
						secondform = (Convert_form)dummy;
					}
				}
				else
				{
					uint2 dummy = firstform;
					dummy += te->which + localeform;
					firstform = (Convert_form)dummy;
				}
			needformat = False;
			break;
		default:
			firstform = (Convert_form)te->which;
			return PS_NORMAL;
		}
		if (sp.skip_token(SP_FACTOR, TT_BINOP, O_AND) == PS_NORMAL)
		{
			if (needformat)
			{
				MCperror->add
				(PE_CONVERT_BADAND, sp);
				return PS_ERROR;
			}
			else
				needformat = True;
		}
		else
			if (!needformat)
				break;
	}
	return PS_NORMAL;
}

Exec_stat MCConvert::exec(MCExecPoint &ep)
{
#ifdef /* MCConvert */ LEGACY_EXEC
	MCresult->clear(False);
	if (container != NULL)
	{
		if (container->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_CONVERT_CANTGET, line, pos);
			return ES_ERROR;
		}
	}
	else
		if (source->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_CONVERT_CANTGET, line, pos);
			return ES_ERROR;
		}
	if (!MCD_convert(ep, fform, fsform, pform, sform))
	{
		MCresult->sets("invalid date");
		return ES_NORMAL;
	}
	Exec_stat stat;
	if (it != NULL)
		stat = it->set
		       (ep);
	else
		stat = container->set
		       (ep, PT_INTO);
	if (stat != ES_NORMAL)
	{
		MCeerror->add
		(EE_CONVERT_CANTSET, line, pos, ep.getsvalue());
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCConvert */
}

MCDo::~MCDo()
{
	delete source;
	delete alternatelang;
}

Parse_stat MCDo::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add(PE_DO_BADEXP, sp);
		return PS_ERROR;
	}
	
	if (sp.skip_token(SP_FACTOR, TT_IN, PT_IN) == PS_NORMAL)
	{
		if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_BROWSER) == PS_NORMAL)
			browser = True;
		else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_CALLER) == PS_NORMAL)
			caller = true;
		else
		{
			MCperror->add(PE_DO_BADENV, sp);
			return PS_ERROR;
		}
		
		return PS_NORMAL;
	}
	
	if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AS) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &alternatelang) != PS_NORMAL)
		{
			MCperror->add(PE_DO_BADLANG, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

Exec_stat MCDo::exec(MCExecPoint &ep)
{
#ifdef /* MCDo */ LEGACY_EXEC
	MCExecPoint *epptr;
	if (browser)
	{
		if (source->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_DO_BADEXP, line, pos);
			return ES_ERROR;
		}
		
		return MCModeExecuteScriptInBrowser(ep . getsvalue());
	}

	if (alternatelang != NULL)
	{
		if (alternatelang->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_DO_BADLANG, line, pos);
			return ES_ERROR;
		}
		char *langname = ep.getsvalue().clone();
		if (source->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_DO_BADEXP, line, pos);
			return ES_ERROR;
		}
		
		if (!MCSecureModeCheckDoAlternate(line, pos))
			return ES_ERROR;
		
		MCString s = ep.getsvalue();
		MCS_doalternatelanguage(s, langname);
		delete langname;
		return ES_NORMAL;
	}
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ep.setline(line);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
		added = True;
	}
	if (debug)
	{
		if (MCdebugcontext >= MCnexecutioncontexts)
			MCdebugcontext = MCnexecutioncontexts - 1;
		epptr = MCexecutioncontexts[MCdebugcontext];
	}
	else
	{
		if (caller)
		{
			if (MCnexecutioncontexts < 2)
			{
				if (added)
					MCnexecutioncontexts--;
				MCeerror -> add(EE_DO_NOCALLER, line, pos);
				return ES_ERROR;
			}
			
			epptr = MCexecutioncontexts[MCnexecutioncontexts - 2];
		}
		else
			epptr = &ep;
	}
	if (source->eval(*epptr) != ES_NORMAL)
	{
		if (added)
			MCnexecutioncontexts--;
		MCeerror->add(EE_DO_BADEXP, line, pos);
		return ES_ERROR;
	}
	// MW-2013-11-15: [[ Bug 11277 ]] If no handler, then evaluate in context of the
	//   server script object.
	Exec_stat stat;
    stat = ep.doscript(*epptr, line, pos);
    
	if (added)
		MCnexecutioncontexts--;
	return stat;
#endif /* MCDo */
}

typedef struct
{
	const char *token;
	const char *command;
}
DT;

static DT domenu_table[] = {
                               {"new stack...", "create stack\n\
                                set the mainStack of the topStack to \"Home\"\n\
                                modal \"Stack Properties\""},
                               {"open stack...", "answer file \"Choose a stack to open...\"\n\
                                if it is not empty then\n\
                                set the cursor to watch\n\
                                topLevel it\n\
                                end if"},
                               {"close stack", "close this stack"},
                               {"compact stack", "compact stack"},
                               {"print card", "print this card"},
                               {"print stack...", "print this stack"},
                               {"quit hypercard", "quit"},
                               {"quit", "quit"},
                               {"undo", "undo"},
                               {"cut", "cut"},
                               {"copy", "copy"},
                               {"paste", "paste"},
                               {"new card", "create card"},
                               {"delete card", "delete this card"},
                               {"cut card", "cut this card"},
                               {"copy card", "copy this card"},
                               {"background", "edit background"},
                               {"back", "go back"},
                               {"home", "go home"},
                               {"help", "help"},
                               {"recent", "go recent"},
                               {"first", "go to first card"},
                               {"prev", "go to prev card"},
                               {"next", "go to next card"},
                               {"last", "go to last card"},
                               {"find...", "modeless \"Find\""},
                               {"message", "modeless \"Message Box\""},
                               {"bring closer", "set the layer of the selobj to the layer of the selobj+1"},
                               {"send farther", "set the layer of the selobj to the layer of the selobj-1"},
                               {"new button", "create button"},
                               {"new field", "create field"},
                               {"new background", "create background"},
                           };

/* doMenu items
About Hypercard..., New Stack..., Open Stack..., Close Stack, Save A
Copy..., Compact Stack, Protect Stack..., Delete Stack..., Page
Setup..., Print Field..., Print Card, Print Stack..., Print Report...,
Quit Hypercard, Undo, Cut, Copy, Paste, New Card, Delete Card, Cut
Card, Copy Card, Text Style..., Background, Icon..., Back, Home, Help,
recent, First, Prev, Next, Last, Find..., Message, Scroll, Next
Window, Button Info..., Field Info..., Card Info..., Bkgnd Info...,
Stack Info..., Bring Closer, Send Farther, New Button, New Field, New
Background, Plain, Bold, Italic, Underline, Outline, Shadow, Condense,
Extend, Group, Other..., Select, Select All, Fill, Invert, Pickup,
Darken, Lighten, Trace Edges, Rotate Left, Rotate Right, Flip
Vertical, Flip Horizontal, Opaque, Transparent, Keep, Revert, grid,
Fatbits, Power Keys, Line Size..., brush Shape..., Edit pattern...,
Polygon Sides..., Draw filled, Draw centered, Draw multiple, Rotate,
Slant, distort, Perspective, Import Paint..., Export Paint..., New
Icon, Close Icon Editor, Duplicate Icon, Cut Icon, Copy Icon, Erase,
Frame, Gray, Mirror Horizontal, Mirror Vertical, Rotate 90, Shadow,
Delete Report, Cut Report, Copy Report, Report Items..., Report
Name..., New Report, New Item, Item Info..., Close Script, Save
Script, Revert To Saved, Print Script, Find Again, Find Selection,
Scroll To Selection, Replace..., Replace Again, Comment, Uncomment,
Set Checkpoint, Step, Step Into, Trace, Go, Trace Delay..., Abort,
Variable Watcher, Message Watcher
*/
const char *MCDoMenu::lookup(const MCString &s)
{
	uint2 size = ELEMENTS(domenu_table);
	while(size--)
		if (s == domenu_table[size].token)
			return domenu_table[size].command;
	return NULL;
}

MCDoMenu::~MCDoMenu()
{
	delete source;
}

Parse_stat MCDoMenu::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add
		(PE_DOMENU_BADEXP, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCDoMenu::exec(MCExecPoint &ep)
{
#ifdef /* MCDoMenu */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_DOMENU_BADEXP, line, pos);
		return ES_ERROR;
	}
	const char *dstring = lookup(ep.getsvalue());
	if (dstring == NULL)
	{
		char *tptr = ep.getsvalue().clone();
		ep.setstringf("doMenu \"%s\" not implemented", tptr);
		delete tptr;

		MCresult->sets(ep.getsvalue());
	}
	else
	{
		ep.getobj()->domess(dstring);
	}
	return ES_NORMAL;
#endif /* MCDoMenu */
}

MCEdit::~MCEdit()
{
	delete target;
    delete m_at;
}

Parse_stat MCEdit::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	sp.skip_token(SP_FACTOR, TT_THE);
	if (sp.skip_token(SP_FACTOR, TT_PROPERTY) != PS_NORMAL)
	{
		MCperror->add(PE_EDIT_NOSCRIPT, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_OF) != PS_NORMAL)
	{
		MCperror->add(PE_EDIT_NOOF, sp);
		return PS_ERROR;
	}
	target = new MCChunk(False);
	if (target->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add(PE_EDIT_NOTARGET, sp);
		return PS_ERROR;
	}
    // MERG 2013-9-13: [[ EditScriptChunk ]] Added line and column
    if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AT) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &m_at) != PS_NORMAL)
        {
            MCperror->add(PE_EDIT_NOAT, sp);
            return PS_ERROR;
        }
	}
	return PS_NORMAL;
}

Exec_stat MCEdit::exec(MCExecPoint &ep)
{
#ifdef /* MCEdit */ LEGACY_EXEC
	MCObject *optr;
	uint4 parid;
	if (target->getobj(ep, optr, parid, True) != ES_NORMAL)
	{
		MCeerror->add(EE_EDIT_BADTARGET, line, pos);
		return ES_ERROR;
	}

    // MERG 2013-9-13: [[ EditScriptChunk ]] Added at expression that's passed through as a second parameter to editScript
    MCString t_at;
    t_at = NULL;
    
    if (m_at != NULL)
    {
        if (m_at->eval(ep) != ES_NORMAL)
        {
            MCeerror->add
            (EE_EDIT_BADAT, line, pos);
            return ES_ERROR;
        }
        t_at = ep.getsvalue();
    }
    
    // MW-2010-10-13: [[ Bug 7476 ]] Make sure we temporarily turn off lock messages
	//   before invoking the method - since it requires message sending to work!
	Boolean t_old_lock;
	t_old_lock = MClockmessages;
	MClockmessages = False;
	optr->editscript(t_at);
	MClockmessages = t_old_lock;

	return ES_NORMAL;
#endif /* MCEdit */
}

MCFind::~MCFind()
{
	delete tofind;
	delete field;
}

Parse_stat MCFind::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);

	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_FIND_NOSTRING, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_FIND, te) == PS_NORMAL)
		mode = (Find_mode)te->which;
	else
		sp.backup();
	if (sp.parseexp(False, True, &tofind) != PS_NORMAL)
	{
		MCperror->add
		(PE_FIND_BADSTRING, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_IN) == PS_NORMAL)
	{
		field = new MCChunk(False);
		if (field->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_FIND_BADFIELD, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

Exec_stat MCFind::exec(MCExecPoint &ep)
{
#ifdef /* MCFind */ LEGACY_EXEC
	if (tofind->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_FIND_BADSTRING, line, pos);
		return ES_ERROR;
	}
	if (ep.getsvalue().getlength() == 0)
	{
		if (MCfoundfield != NULL)
			MCfoundfield->clearfound();
		MCresult->sets(MCnotfoundstring);
		return ES_NORMAL;
	}
	MCdefaultstackptr->find(ep, mode, ep.getsvalue(), field);
	return ES_NORMAL;
#endif /* MCFind */
}

MCGet::~MCGet()
{
	delete value;
	delete it;
}

Parse_stat MCGet::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &value) != PS_NORMAL)
	{
		MCperror->add
		(PE_GET_BADEXP, sp);
		return PS_ERROR;
	}
	getit(sp, it);
	return PS_NORMAL;
}

Exec_stat MCGet::exec(MCExecPoint &ep)
{
#ifdef /* MCGet */ LEGACY_EXEC
	if (value->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_GET_BADEXP, line, pos);
		return ES_ERROR;
	}
	if (it->set
	        (ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_GET_CANTSET, line, pos, ep.getsvalue());
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCGet */
}

MCMarking::~MCMarking()
{
	delete where;
	delete tofind;
	delete field;
	delete card;
}

Parse_stat MCMarking::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);

	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_MARK_NOCARDS, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_MARK, te) != PS_NORMAL)
	{
		sp.backup();
		card = new MCChunk(False);
		if (card->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_MARK_NOTCARDS, sp);
			return PS_ERROR;
		}
		return PS_NORMAL;
	}
	if (te->which == MC_ALL)
	{
		if (sp.skip_token(SP_MARK, TT_UNDEFINED, MC_CARDS) != PS_NORMAL)
		{
			MCperror->add
			(PE_MARK_NOCARDS, sp);
			return PS_ERROR;
		}
		return PS_NORMAL;
	}
	if (te->which != MC_CARDS)
	{
		MCperror->add
		(PE_MARK_NOCARDS, sp);
		return PS_ERROR;
	}
if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_MARK_NOBYORWHERE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_MARK,te) != PS_NORMAL
	        || te->which != MC_BY && te->which != MC_WHERE)
	{
		MCperror->add
		(PE_MARK_NOTBYORWHERE, sp);
		return PS_ERROR;
	}
	if (te->which == MC_WHERE)
	{
		if (sp.parseexp(False, True, &where) != PS_NORMAL)
		{
			MCperror->add
			(PE_MARK_BADWHEREEXP, sp);
			return PS_ERROR;
		}
		return PS_NORMAL;
	}
	if (sp.skip_token(SP_MARK, TT_UNDEFINED, MC_FINDING) != PS_NORMAL)
	{
		MCperror->add
		(PE_MARK_NOFINDING, sp);
		return PS_ERROR;
	}
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_MARK_NOSTRING, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_FIND, te) == PS_NORMAL)
		mode = (Find_mode)te->which;
	else
		sp.backup();
	if (sp.parseexp(False, True, &tofind) != PS_NORMAL)
	{
		MCperror->add
		(PE_MARK_BADSTRING, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_IN) == PS_NORMAL)
	{
		field = new MCChunk(False);
		if (field->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_MARK_BADFIELD, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

Exec_stat MCMarking::exec(MCExecPoint &ep)
{
#ifdef /* MCMarking */ LEGACY_EXEC
	if (card != NULL)
	{
		MCObject *optr;
		uint4 parid;
		if (card->getobj(ep, optr, parid, True) != ES_NORMAL
		        || optr->gettype() != CT_CARD)
		{
			MCeerror->add
			(EE_MARK_BADCARD, line, pos);
			return ES_ERROR;
		}
		ep.setboolean(mark);
		return optr->setprop(0, P_MARKED, ep, False);
	}
	if (tofind == NULL)
		MCdefaultstackptr->mark(ep, where, mark);
	else
	{
		if (tofind->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_MARK_BADSTRING, line, pos);
			return ES_ERROR;
		}
		MCdefaultstackptr->markfind(ep, mode, ep.getsvalue(), field, mark);
	}
	return ES_NORMAL;
#endif /* MCMarking */
}

MCPut::~MCPut()
{
	delete source;
	delete dest;
	
	// cookie
	delete name;
	delete domain;
	delete path;
	delete expires;
}
		
// put [ unicode | binary ] <expr>
// put [ unicode ] ( content | markup ) <expr>
// put [ new ] header <expr>
// put [ secure ] [ httponly ] cookie <name> [ for <path> ] [ on <domain> ] to <value> [ until <expiry> ]
// put <expr> ( into | after | before ) message [ box ]
// put <expr> ( into | after | before ) <chunk>
//
Parse_stat MCPut::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	
	// IM-2011-08-22: [[ SERVER ]] Add support for new put variant.
	// Parse: put [ secure ] [ httponly ] cookie <name> [ for path ] [ on domain ] with <value> [ until expires ]
	if (sp . skip_token(SP_SERVER, TT_SERVER, SK_SECURE) == PS_NORMAL)
		is_secure = true;
	
	if (sp . skip_token(SP_SERVER, TT_SERVER, SK_HTTPONLY) == PS_NORMAL)
		is_httponly = true;
	
	if (sp . skip_token(SP_SERVER, TT_PREP, PT_COOKIE) == PS_NORMAL)
	{
		prep = PT_COOKIE;
		if (sp . parseexp(True, False, &name) != PS_NORMAL)
		{
			MCperror->add(PE_PUT_BADEXP, sp);
			return PS_ERROR;
		}
		if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_FOR) == PS_NORMAL)
		{
			if (sp . parseexp(True, False, &path) != PS_NORMAL)
			{
				MCperror->add(PE_PUT_BADEXP, sp);
				return PS_ERROR;
			}
		}
		if (sp . skip_token(SP_FACTOR, TT_OF, PT_ON) == PS_NORMAL)
		{
			if (sp . parseexp(True, False, &domain) != PS_NORMAL)
			{
				MCperror->add(PE_PUT_BADEXP, sp);
				return PS_ERROR;
			}
		}
		sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH);
		if (sp . parseexp(True, False, &source) != PS_NORMAL)
		{
			MCperror->add(PE_PUT_BADEXP, sp);
			return PS_ERROR;
		}
		if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_UNTIL) == PS_NORMAL)
		{
			if (sp . parseexp(True, False, &expires) != PS_NORMAL)
			{
				MCperror->add(PE_PUT_BADEXP, sp);
				return PS_ERROR;
			}
		}
		
		return PS_NORMAL;
	}
	
	if (is_secure || is_httponly)
	{
		MCperror->add(PE_PUT_BADPREP, sp);
		return PS_ERROR;
	}
	
	// MW-2011-06-22: [[ SERVER ]] Add support for new put variant.
	// Parse: put new header <expr>
	if (sp.skip_token(SP_SERVER, TT_SERVER, SK_NEW) == PS_NORMAL)
	{
		if (sp.skip_token(SP_SERVER, TT_PREP, PT_HEADER) == PS_NORMAL)
		{
			prep = PT_NEW_HEADER;
			if (sp . parseexp(False, True, &source) != PS_NORMAL)
			{
				MCperror->add(PE_PUT_BADEXP, sp);
				return PS_ERROR;
			}
			
			return PS_NORMAL;
		}
		else
			sp.backup();			
	}

	// MW-2012-02-23: [[ UnicodePut ]] Store whether 'unicode' was present
	//   in the ast.
	if (sp . skip_token(SP_SERVER, TT_SERVER, SK_UNICODE) == PS_NORMAL)
		is_unicode = true;

	// MW-2011-06-22: [[ SERVER ]] Add support for new put variant.
	// Parse: put [ unicode ] ( header | content | markup ) <expr>	
	if (sp.next(type) == PS_NORMAL)
	{
		if (sp.lookup(SP_SERVER, te) == PS_NORMAL && te -> type == TT_PREP)
	{
			prep = (Preposition_type)te -> which;
			if (is_unicode && (prep == PT_HEADER || prep == PT_BINARY))
			{
				MCperror->add(PE_PUT_BADPREP, sp);
				return PS_ERROR;
			}
			
			if (sp . parseexp(False, True, &source) != PS_NORMAL)
			{
				MCperror->add(PE_PUT_BADEXP, sp);
				return PS_ERROR;
			}
			
			return PS_NORMAL;
		}
		else
			sp.backup();
	}

	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add(PE_PUT_BADEXP, sp);
		return PS_ERROR;
	}
	
	if (sp.next(type) != PS_NORMAL)
		return PS_NORMAL;
		
	if (sp.lookup(SP_FACTOR, te) != PS_NORMAL || te->type != TT_PREP)
	{
		sp.backup();
		return PS_NORMAL;
	}
	prep = (Preposition_type)te->which;
	if (prep != PT_BEFORE && prep != PT_INTO && prep != PT_AFTER)
	{
		MCperror->add(PE_PUT_BADPREP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_MESSAGE) == PS_NORMAL)
	{
		sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_MESSAGE); // "box"
		return PS_NORMAL;
	}
	dest = new MCChunk(True);
	if (dest->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add(PE_PUT_BADCHUNK, sp);
		return PS_ERROR;
	}

	MCVarref *t_src_ref, *t_dst_ref;
	t_src_ref = source -> getrootvarref();
	t_dst_ref = dest -> getrootvarref();
	overlap = t_src_ref != NULL && t_dst_ref != NULL && t_src_ref -> rootmatches(t_dst_ref);

	return PS_NORMAL;
}

Exec_stat MCPut::exec(MCExecPoint &ep)
{
#ifdef /* MCPut */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_PUT_BADEXP, line, pos);
		return ES_ERROR;
	}
	if (dest != NULL)
	{
		if (overlap)
			ep . grab();

		Exec_stat t_stat;
		if (!is_unicode)
			t_stat = dest -> set(ep, prep);
		else
			t_stat = dest -> setunicode(ep, prep);
		if (t_stat != ES_NORMAL)
		{
			MCeerror->add(EE_PUT_CANTSET, line, pos);
			return ES_ERROR;
		}
		return ES_NORMAL;
	}
	else
	{
		// MW-2011-06-22: [[ SERVER ]] Handle the non-dest forms of put.
		MCSPutKind t_kind;
		switch(prep)
		{
			case PT_UNDEFINED:
				t_kind = is_unicode ? kMCSPutUnicodeOutput : kMCSPutOutput;
				break;
			case PT_INTO:
				t_kind = kMCSPutIntoMessage;
				break;
			case PT_AFTER:
				t_kind = kMCSPutAfterMessage;
				break;
			case PT_BEFORE:
				t_kind = kMCSPutBeforeMessage;
				break;
			case PT_HEADER:
				t_kind = kMCSPutHeader;
				break;
			case PT_NEW_HEADER:
				t_kind = kMCSPutNewHeader;
				break;
			case PT_CONTENT:
				t_kind = is_unicode ? kMCSPutUnicodeContent : kMCSPutContent;
				break;
			case PT_MARKUP:
				t_kind = is_unicode ? kMCSPutUnicodeMarkup : kMCSPutMarkup;
				break;
			case PT_BINARY:
				t_kind = kMCSPutBinaryOutput;
				break;

			case PT_COOKIE:
				return exec_cookie(ep);
				
			default:
				t_kind = kMCSPutNone;
				break;
		}

		if (!MCS_put(ep, t_kind, ep . getsvalue()))
		{
			MCeerror->add(EE_PUT_CANTSETINTO, line, pos);
			return ES_ERROR;
		}

		return ES_NORMAL;
	}
#endif /* MCPut */
}
#ifdef /* MCPut::exec_cookie */ LEGACY_EXEC
#if defined(_SERVER)
bool MCServerSetCookie(const MCString &p_name, const MCString &p_value, uint32_t p_expires, const MCString &p_path, const MCString &p_domain, bool p_secure, bool p_http_only);
Exec_stat MCPut::exec_cookie(MCExecPoint &ep)
{
	char *t_name = NULL;
	char *t_value = NULL;
	char *t_path = NULL;
	char *t_domain = NULL;
	
	uint32_t t_name_len = 0;
	uint32_t t_value_len = 0;
	uint32_t t_path_len = 0;
	uint32_t t_domain_len = 0;
	
	uint32_t t_expires = 0;
	
	bool t_success = true;
	
	
	t_value = ep.getsvalue().clone();
	t_value_len = ep.getsvalue().getlength();
	
	t_success = (ES_NORMAL == name->eval(ep));
	
	if (t_success)
	{
		t_name = ep.getsvalue().clone();
		t_name_len = ep.getsvalue().getlength();
		if (expires != NULL)
			t_success = expires->eval(ep);
	}
	if (t_success && expires)
		t_success = ep.isempty() || MCU_stoui4(ep.getsvalue(), t_expires);
	
	if (t_success && path)
	{
		t_success = path->eval(ep);
		if (t_success)
		{
			t_path = ep.getsvalue().clone();
			t_path_len = ep.getsvalue().getlength();
		}
	}
	
	if (t_success && domain)
	{
		t_success = domain->eval(ep);
		if (t_success)
		{
			t_domain = ep.getsvalue().clone();
			t_domain_len = ep.getsvalue().getlength();
		}
	}
		
	if(t_success)
		t_success = MCServerSetCookie(MCString(t_name, t_name_len), MCString(t_value, t_value_len), t_expires, MCString(t_path, t_path_len), MCString(t_domain, t_domain_len), is_secure, is_httponly);

	MCCStringFree(t_name);
	MCCStringFree(t_value);
	MCCStringFree(t_path);
	MCCStringFree(t_domain);

	if (!t_success)
	{
		MCeerror->add(EE_PUT_CANTSETINTO, line, pos);
		return ES_ERROR;
	}
	ep.clear();
	return ES_NORMAL;
}
#else // !defined(_SERVER)
Exec_stat MCPut::exec_cookie(MCExecPoint &ep)
{
	MCeerror->add(EE_PUT_CANTSET, line, pos);
	return ES_ERROR;
}
#endif
#endif /* MCPut::exec_cookie */

MCQuit::~MCQuit()
{
	delete retcode;
}

Parse_stat MCQuit::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	MCerrorlock++;
	sp.parseexp(False, True, &retcode);
	MCerrorlock--;
	return PS_NORMAL;
}

Exec_stat MCQuit::exec(MCExecPoint &ep)
{
#ifdef /* MCQuit */ LEGACY_EXEC
// MW-2011-06-22: [[ SERVER ]] Don't send messages in server-mode.
#ifndef _SERVER
	switch(MCdefaultstackptr->getcard()->message(MCM_shut_down_request))
	{
	case ES_PASS:
	case ES_NOT_HANDLED:
		break;
	default:
		return ES_NORMAL;
	}
	// IM-2013-05-01: [[ BZ 10586 ]] remove #ifdefs so this message is sent
	// here on Android in the same place as (almost) everything else
	MCdefaultstackptr->getcard()->message(MCM_shut_down);
#endif
	if (retcode != NULL && retcode->eval(ep) == ES_NORMAL
	        && ep.ton() == ES_NORMAL)
		MCretcode = ep.getint4();
	MCquit = True;
	MCquitisexplicit = True;
	MCexitall = True;
	MCtracestackptr = NULL;
	MCtraceabort = True;
	MCtracereturn = True;
	return ES_NORMAL;
#endif /* MCQuit */
}

Parse_stat MCReset::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	sp.skip_token(SP_FACTOR, TT_THE);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_CHOOSE_NOTOKEN, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_RESET, te) != PS_NORMAL)
	{
		MCperror->add
		(PE_RESET_NOTYPE, sp);
		return PS_ERROR;
	}
	which = (Reset_type)te->which;
	return PS_NORMAL;
}

Exec_stat MCReset::exec(MCExecPoint &ep)
{
#ifdef /* MCReset */ LEGACY_EXEC
	switch (which)
	{
	case RT_CURSORS:
		MCModeResetCursors();
		break;
	case RT_PAINT:
		MCeditingimage = nil;

		MCbrush = 8;
		MCspray = 31;
		MCeraser = 2;
		MCcentered = False;
		MCfilled = False;
		MCgrid = False;
		MCgridsize = 8;
		MClinesize = 1;
		MCmultiple = False;
		MCmultispace = 1;
		MCpattern = 1;
		MCpolysides = 4;
		MCroundends = False;
		MCslices = 16;
		MCmagnification = 8;

		MCpatternlist->freepat(MCpenpattern);
		MCpencolor.red = MCpencolor.green = MCpencolor.blue = 0x0;
		MCscreen->alloccolor(MCpencolor);

		MCpatternlist->freepat(MCbrushpattern);
		MCbrushcolor.red = MCbrushcolor.green = MCbrushcolor.blue = 0xFFFF;
		MCscreen->alloccolor(MCbrushcolor);
		break;
	case RT_PRINTING:
		MCprinter -> Reset();
		if (MCprinter != MCsystemprinter)
		{
			delete MCprinter;
			MCprinter = MCsystemprinter;
		}
	break;
	case RT_TEMPLATE_AUDIO_CLIP:
		delete MCtemplateaudio;
		MCtemplateaudio = new MCAudioClip;
		break;
	case RT_TEMPLATE_BUTTON:
		delete MCtemplatebutton;
		MCtemplatebutton = new MCButton;
		break;
	case RT_TEMPLATE_CARD:
		delete MCtemplatecard;
		MCtemplatecard = new MCCard;
		break;
	case RT_TEMPLATE_EPS:
		delete MCtemplateeps;
		MCtemplateeps = new MCEPS;
		break;
	case RT_TEMPLATE_FIELD:
		delete MCtemplatefield;
		MCtemplatefield = new MCField;
		break;
	case RT_TEMPLATE_GRAPHIC:
		delete MCtemplategraphic;
		MCtemplategraphic = new MCGraphic;
		break;
	case RT_TEMPLATE_GROUP:
		delete MCtemplategroup;
		MCtemplategroup = new MCGroup;
		break;
	case RT_TEMPLATE_IMAGE:
		delete MCtemplateimage;
		MCtemplateimage = new MCImage;
		break;
	case RT_TEMPLATE_SCROLLBAR:
		delete MCtemplatescrollbar;
		MCtemplatescrollbar = new MCScrollbar;
		break;
	case RT_TEMPLATE_PLAYER:
		delete MCtemplateplayer;
		MCtemplateplayer = new MCPlayer;
		break;
	case RT_TEMPLATE_STACK:
		delete MCtemplatestack;
		/* UNCHECKED */ MCStackSecurityCreateStack(MCtemplatestack);
		break;
	case RT_TEMPLATE_VIDEO_CLIP:
		delete MCtemplatevideo;
		MCtemplatevideo = new MCVideoClip;
		break;
	default:
		break;
	}
	return ES_NORMAL;
#endif /* MCReset */
}

MCReturn::~MCReturn()
{
	delete source;
	delete url;
	delete var;
}

Parse_stat MCReturn::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add
		(PE_RETURN_BADEXP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_URL))
		{
			// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
			//   execution outwith a handler.
			Symbol_type type;
			if (sp.next(type) != PS_NORMAL || sp.findvar(sp.gettoken_nameref(), &var) != PS_NORMAL)
				sp.backup();
			else
				var->parsearray(sp);
		}
		if (var == NULL)
		{
			sp.skip_token(SP_FACTOR, TT_FUNCTION, F_CACHED_URLS);
			if (sp.parseexp(False, True, &url) != PS_NORMAL)
			{
				MCperror->add
				(PE_RETURN_BADEXP, sp);
				return PS_ERROR;
			}
		}
	}
	return PS_NORMAL;
}


// MW-2007-07-03: [[ Bug 4570 ]] - Using the return command now causes a
//   RETURN_HANDLER status rather than EXIT_HANDLER. This is used to not
//   clear the result in this case. (see MCHandler::exec).
Exec_stat MCReturn::exec(MCExecPoint &ep)
{
#ifdef /* MCReturn */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_RETURN_BADEXP, line, pos);
		return ES_ERROR;
	}
	MCresult -> store(ep, False);
	if (url != NULL)
	{
		if (url->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_RETURN_BADEXP, line, pos);
			return ES_ERROR;
		}
		MCurlresult -> store(ep, False);
	}
	else
		if (var != NULL)
		{
			if (var->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_RETURN_BADEXP, line, pos);
				return ES_ERROR;
			}
			MCurlresult->store(ep, False);
			var->dofree(ep);
		}

	return ES_RETURN_HANDLER;
#endif /* MCReturn */
}

uint4 MCReturn::linecount()
{
	return 0;
}

Parse_stat MCScriptError::parse(MCScriptPoint &sp)
{
	return PS_ERROR; // catch on/function/getprop/setprop
}

MCSet::~MCSet()
{
	delete target;
	delete value;
}

Parse_stat MCSet::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_THE) == PS_ERROR)
	{
		MCperror->add(PE_SET_NOTHE, sp);
		return PS_ERROR;
	}
	target = new MCProperty;
	if (target->parse(sp, True) != PS_NORMAL)
	{
		MCperror->add(PE_SET_NOPROP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_TO) != PS_NORMAL)
	{
		MCperror->add(PE_SET_NOTO, sp);
		return PS_ERROR;
	}
	if (sp.parseexp(False, True, &value) != PS_NORMAL)
	{
		MCperror->add(PE_SET_BADEXP, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCSet::exec(MCExecPoint &ep)
{
#ifdef /* MCSet */ LEGACY_EXEC
	if (value->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_SET_BADEXP, line, pos);
		return ES_ERROR;
	}
	ep.grabsvalue();
	MCresult->clear(False);
	if (target->set
	        (ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_SET_BADSET, line, pos, ep.getsvalue());
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCSet */
}

MCSort::~MCSort()
{
	delete of;
	delete by;
}

Parse_stat MCSort::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);

	while (True)
	{
		if (sp.next(type) != PS_NORMAL)
			if (of == NULL && chunktype == CT_FIELD)
			{
				MCperror->add
				(PE_SORT_NOTARGET, sp);
				return PS_ERROR;
			}
			else
				break;
		if (sp.lookup(SP_SORT, te) == PS_NORMAL)
		{
			switch (te->which)
			{
			case ST_OF:
				of = new MCChunk(True);
				if (of->parse(sp, False) != PS_NORMAL)
				{
					MCperror->add
					(PE_SORT_BADTARGET, sp);
					return PS_ERROR;
				}
				break;
			case ST_BY:
				if (chunktype == CT_FIELD && of == NULL)
					chunktype = CT_CARD;
				if (sp.parseexp(False, True, &by) != PS_NORMAL)
				{
					MCperror->add
					(PE_SORT_BADEXPRESSION, sp);
					return PS_ERROR;
				}
				if (of == NULL && chunktype == CT_FIELD)
				{
					MCperror->add
					(PE_SORT_NOTARGET, sp);
					return PS_ERROR;
				}
				return PS_NORMAL;
			case ST_LINES:
				chunktype = CT_LINE;
				break;
			case ST_ITEMS:
				chunktype = CT_ITEM;
				break;
			case ST_MARKED:
				chunktype = CT_MARKED;
				break;
			case ST_CARDS:
				if (chunktype != CT_MARKED)
					chunktype = CT_CARD;
				break;
			case ST_TEXT:
			case ST_NUMERIC:
			case ST_INTERNATIONAL:
			case ST_DATETIME:
				format = (Sort_type)te->which;
				break;
			case ST_ASCENDING:
			case ST_DESCENDING:
				direction = (Sort_type)te->which;
				break;
			}
		}
		else
		{
			sp.backup();
			if (of == NULL)
			{
				of = new MCChunk(True);
				if (of->parse(sp, False) != PS_NORMAL)
				{
					MCperror->add
					(PE_SORT_BADTARGET, sp);
					return PS_ERROR;
				}
			}
			else
				break;
		}
	}
	return PS_NORMAL;
}
#ifdef /* MCSort::sort_container */ LEGACY_EXEC
Exec_stat MCSort::sort_container(MCExecPoint &p_exec_point, Chunk_term p_type, Sort_type p_direction, Sort_type p_form, MCExpression *p_by)
{
	MCSortnode *t_items;
	uint4 t_item_count;
	t_item_count = 0;

	// If sorting items of the container, then we use the current itemDelimiter to split each item,
	// all other forms of search default to the lineDelimiter for now. Note that this is a slight
	// change of behavior as previously sorting containers by line ignored the lineDelimiter and
	// always delimited by ascii 10.
	char t_delimiter;
	if (p_type == CT_ITEM)
		t_delimiter = p_exec_point . getitemdel();
	else
		t_delimiter = p_exec_point . getlinedel();

	if (t_delimiter == '\0')
		return ES_NORMAL;

	// Calculate the number of items we need to sort, store this in t_item_count.
	uint4 t_item_size;
	t_item_size = p_exec_point . getsvalue() . getlength();

	char *t_item_text;
	t_item_text = p_exec_point . getsvalue() . clone();

	char *t_string_pointer;
	t_string_pointer = t_item_text;

	char *t_end_pointer;
	bool t_trailing_delim = false;
	while ((t_end_pointer = strchr(t_string_pointer, t_delimiter)) != NULL)
	{
		// knock out last delim for lines with a trailing return char
		if (p_type != CT_ITEM && t_end_pointer[1] == '\0')
		{
			t_end_pointer[0] = '\0';
			t_trailing_delim = true;
		}
		else
			t_item_count++;
		t_string_pointer = t_end_pointer + 1;
	}

	// OK-2008-12-11: [[Bug 7503]] - If there are 0 items in the string, don't carry out the search,
	// this keeps the behavior consistent with previous versions of Revolution.
	if (t_item_count < 1)
	{
		delete t_item_text;
		return ES_NORMAL;
	}

	// Now we know the item count, we can allocate an array of MCSortnodes to store them.
	t_items = new MCSortnode[t_item_count + 1];
	t_item_count = 0;
	t_string_pointer = t_item_text;

	// Next, populate the MCSortnodes with all the items to be sorted
	MCString t_string;
	do
	{
		if ((t_end_pointer = strchr(t_string_pointer, t_delimiter)) != NULL)
		{
			*t_end_pointer++ = '\0';
			t_string . set(t_string_pointer, t_end_pointer - t_string_pointer - 1);
		}
		else
			t_string . set(t_string_pointer, strlen(t_string_pointer));

		MCExecPoint t_exec_point2(p_exec_point);
		additem(t_exec_point2, t_items, t_item_count, p_form, t_string, p_by);

		t_items[t_item_count - 1] . data = (void *)t_string_pointer;
		t_string_pointer = t_end_pointer;
	}
	while (t_end_pointer != NULL);

	// Sort the array
	MCU_sort(t_items, t_item_count, p_direction, p_form);

	// Build the output string
	char *t_output;
	t_output = new char[t_item_size + 1];
	*t_output = '\0';
	
	uint4 t_length;
	t_length = 0;

	for (unsigned int i = 0; i < t_item_count; i++)
	{
		uint4 t_item_length;
		t_item_length = strlen((const char *)t_items[i] . data);
		strncpy(&t_output[t_length], (const char *)t_items[i] . data, t_item_length);
		t_length = t_length + t_item_length;

		if ((p_form == ST_INTERNATIONAL || p_form == ST_TEXT) && (!p_exec_point . getcasesensitive() || p_by != NULL))
			delete t_items[i] . svalue;

		if (t_trailing_delim || i < t_item_count - 1)
			t_output[t_length++] = t_delimiter;
	}
	t_output[t_length] = '\0';

	p_exec_point . grabbuffer(t_output, t_length);

	delete t_item_text;
	delete t_items;
	return ES_NORMAL;
}
#endif /* MCSort::sort_container */

void MCSort::additem(MCExecPoint &ep, MCSortnode *&items, uint4 &nitems, Sort_type form, MCString &s, MCExpression *by)
{
	if (by != NULL)
	{
		MCerrorlock++;
		ep.setsvalue(s);
		MCeach->store(ep, False);
		if (by->eval(ep) == ES_NORMAL)
			s = ep.getsvalue();
		else
			s = MCnullmcstring;
		MCerrorlock--;
	}
	switch (form)
	{
	case ST_DATETIME:
		ep.setsvalue(s);
		if (MCD_convert(ep, CF_UNDEFINED, CF_UNDEFINED, CF_SECONDS, CF_UNDEFINED))
		{
			if (!MCU_stor8(ep.getsvalue(), items[nitems].nvalue))
				items[nitems].nvalue = -MAXREAL8;
		}
		else
			items[nitems].nvalue = -MAXREAL8;
		break;
	case ST_NUMERIC:
		{
			const char *sptr = s.getstring();
			uint4 length = s.getlength();
			
			// MW-2013-03-21: [[ Bug ]] Make sure we skip any whitespace before the
			//   number starts - making it consistent with string->number conversions
			//   elsewhere.
			MCU_skip_spaces(sptr, length);
			
		    // REVIEW - at the moment the numeric prefix of the string is used to
			//   derive the sort key e.g. 1000abc would get processed as 1000.
			while (length && (isdigit((uint1)*sptr) ||
			                  *sptr == '.' || *sptr == '-' || *sptr == '+'))
			{
				sptr++;
				length--;
			}
			s.setlength(s.getlength() - length);
			if (!MCU_stor8(s, items[nitems].nvalue))
				items[nitems].nvalue = -MAXREAL8;
		}
		break;
	default:
		if (ep.getcasesensitive() && by == NULL)
			items[nitems].svalue = (char *)s.getstring();
		else
			if (ep.getcasesensitive())
				items[nitems].svalue = s.clone();
			else
			{
#if defined(_MAC_DESKTOP) || defined(_IOS_MOBILE)
				if (form == ST_INTERNATIONAL)
				{
					extern char *MCSystemLowercaseInternational(const MCString& s);
					items[nitems].svalue = MCSystemLowercaseInternational(s);
				}
				else
#endif

				{
					items[nitems].svalue = new char[s.getlength() + 1];
					MCU_lower(items[nitems].svalue, s);
					items[nitems].svalue[s.getlength()] = '\0';
				}
			}
		break;
	}
	nitems++;
}

Exec_stat MCSort::exec(MCExecPoint &ep)
{
#ifdef /* MCSort */ LEGACY_EXEC
	if (of == NULL && chunktype == CT_FIELD)
	{
		MCeerror->add
		(EE_SORT_NOTARGET, line, pos);
		return ES_ERROR;
	}
	MCObject *optr = NULL;
	uint4 parid;
	if (of != NULL)
	{
		MCerrorlock++;
		if (of->getobj(ep, optr, parid, False) != ES_NORMAL
		        || optr->gettype() == CT_BUTTON)
		{
			MCerrorlock--;
			if (of->eval(ep) != ES_NORMAL)
			{
				MCeerror->add
				(EE_SORT_BADTARGET, line, pos);
				return ES_ERROR;
			}
		}
		else
			MCerrorlock--;
		if (optr != NULL && optr->gettype() > CT_GROUP && chunktype <= CT_GROUP)
			chunktype = CT_LINE;
	}
	else
		optr = MCdefaultstackptr;
	if (chunktype == CT_CARD || chunktype == CT_MARKED)
	{
		MCStack *sptr = (MCStack *)optr;
		if (optr == NULL || optr->gettype() != CT_STACK
		        || sptr->sort(ep, direction, format, by,
		                      chunktype == CT_MARKED) != ES_NORMAL)
		{
			MCeerror->add
			(EE_SORT_CANTSORT, line, pos);
			return ES_ERROR;
		}
	}
	else
	{
		if (optr == NULL || optr->gettype() == CT_BUTTON)
		{
			if (sort_container(ep, chunktype, direction, format, by) != ES_NORMAL)
			{
				MCeerror->add(EE_SORT_CANTSORT, line, pos);
				return ES_ERROR;
			}
			of -> set(ep, PT_INTO);
		}
		else
		{
			MCField *fptr = (MCField *)optr;
			if (optr->gettype() != CT_FIELD || !of->nochunks()
			        || fptr->sort(ep, parid, chunktype, direction,
			                      format, by) != ES_NORMAL)
			{
				MCeerror->add
				(EE_SORT_CANTSORT, line, pos);
				return ES_ERROR;
			}
		}
	}
	return ES_NORMAL;
#endif /* MCSort */
}



MCWait::~MCWait()
{
	delete duration;
}

Parse_stat MCWait::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);

	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_WAIT_NODURATION, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_REPEAT, te) == PS_NORMAL)
		condition = (Repeat_form)te->which;
	else
	{
		condition = RF_FOR;
		sp.backup();
	}

	if (sp.skip_token(SP_MOVE, TT_UNDEFINED, MM_MESSAGES) == PS_NORMAL)
		messages = True;
	else
	{
		if (sp.parseexp(False, True, &duration) != PS_NORMAL)
		{
			MCperror->add
			(PE_WAIT_BADCOND, sp);
			return PS_ERROR;
		}
		if (condition == RF_FOR)
			if (sp.skip_token(SP_FACTOR, TT_FUNCTION, F_MILLISECS) == PS_NORMAL)
				units = F_MILLISECS;
			else
				if (sp.skip_token(SP_FACTOR, TT_FUNCTION, F_SECONDS) == PS_NORMAL
				        || sp.skip_token(SP_FACTOR, TT_CHUNK, CT_SECOND) == PS_NORMAL)
					units = F_SECONDS;
				else
					if (sp.skip_token(SP_FACTOR, TT_FUNCTION, F_TICKS) == PS_NORMAL)
						units = F_TICKS;
					else
						units = F_TICKS;
		if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
		{
			sp.skip_token(SP_MOVE, TT_UNDEFINED, MM_MESSAGES);
			messages = True;
		}
	}
	return PS_NORMAL;
}

Exec_stat MCWait::exec(MCExecPoint &ep)
{
#ifdef /* MCWait */ LEGACY_EXEC
	while (True)
	{
		MCU_play();
		if (duration == NULL)
		{
			if (MCscreen->wait(MCmaxwait, messages, messages) || MCabortscript)
			{
				MCeerror->add(EE_WAIT_ABORT, line, pos);
				return ES_ERROR;
			}
			break;
		}
		else
		{
			if (duration->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_WAIT_BADEXP, line, pos);
				return ES_ERROR;
			}
			switch (condition)
			{
			case RF_FOR:
				real8 delay;
				if (ep.getreal8(delay, line, pos, EE_WAIT_NAN) != ES_NORMAL)
					return ES_ERROR;
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
				if (MCscreen->wait(delay, messages, False))
				{
					MCeerror->add(EE_WAIT_ABORT, line, pos);
					return ES_ERROR;
				}
				return ES_NORMAL;
			case RF_UNTIL:
				if (ep.getsvalue() == MCtruemcstring)
					return ES_NORMAL;
				if (MCscreen->wait(WAIT_INTERVAL, messages, True))
				{
					MCeerror->add(EE_WAIT_ABORT, line, pos);
					return ES_ERROR;
				}
				break;
			case RF_WHILE:
				if (ep.getsvalue() == MCfalsemcstring)
					return ES_NORMAL;
				if (MCscreen->wait(WAIT_INTERVAL, messages, True))
				{
					MCeerror->add(EE_WAIT_ABORT, line, pos);
					return ES_ERROR;
				}
				break;
			default:
				return ES_ERROR;
			}
		}
	}
	return ES_NORMAL;
#endif /* MCWait */
}

MCInclude::~MCInclude(void)
{
	delete filename;
}

Parse_stat MCInclude::parse(MCScriptPoint& sp)
{
	initpoint(sp);
	
	if (sp . parseexp(False, True, &filename) != PS_NORMAL)
	{
		MCperror -> add(PE_INCLUDE_BADFILENAME, sp);
		return PS_ERROR;
	}
	
	return PS_NORMAL;
}

Exec_stat MCInclude::exec(MCExecPoint& ep)
{	
#ifdef /* MCInclude */ LEGACY_EXEC
	if (filename -> eval(ep) != ES_NORMAL)
	{
		MCeerror -> add(EE_INCLUDE_BADFILENAME, line, pos);
		return ES_ERROR;
	}

#ifdef _SERVER
	MCServerScript *t_script;
	t_script = static_cast<MCServerScript *>(ep . getobj());

	if (t_script -> GetIncludeDepth() > 16)
	{
		MCeerror -> add(EE_INCLUDE_TOOMANY, line, pos);
		return ES_ERROR;
	}
	
	if (!t_script -> Include(ep, ep . getcstring(), is_require))
	{
		MCeerror -> add(EE_SCRIPT_ERRORPOS, line, pos);
		return ES_ERROR;
	}
	
	return ES_NORMAL;
#else
	MCeerror -> add(is_require ? EE_REQUIRE_BADCONTEXT : EE_INCLUDE_BADCONTEXT, line, pos);
	return ES_ERROR;
#endif
#endif /* MCInclude */
}

MCEcho::~MCEcho(void)
{
}

Parse_stat MCEcho::parse(MCScriptPoint& sp)
{
	initpoint(sp);
	data = sp . gettoken();
	return PS_NORMAL;
}

Exec_stat MCEcho::exec(MCExecPoint& ep)
{
#ifdef /* MCEcho */ LEGACY_EXEC
	if (!MCS_put(ep, kMCSPutBinaryOutput, data) != IO_NORMAL)
		MCexitall = True;

	return ES_NORMAL;
#endif /* MCEcho */
}

MCResolveImage::~MCResolveImage(void)
{
    delete m_relative_object;
    delete m_id_or_name;
    delete m_it;
}

Parse_stat MCResolveImage::parse(MCScriptPoint &p_sp)
{
    Parse_stat t_stat;
    t_stat = PS_NORMAL;
    
    // Fetch a reference to 'it'
    getit(p_sp, m_it);
    
    if (t_stat == PS_NORMAL)
        t_stat =  p_sp.skip_token(SP_FACTOR, TT_CHUNK, CT_IMAGE);
	
	// Parse the optional 'id' token
    m_is_id = (PS_NORMAL == p_sp . skip_token(SP_FACTOR, TT_PROPERTY, P_ID));
    
    // Parse the id_or_name expression
    if (t_stat == PS_NORMAL)
        t_stat = p_sp . parseexp(False, True, &m_id_or_name);
    
    if (t_stat != PS_NORMAL)
    {
        MCperror->add
        (PE_RESOLVE_BADIMAGE, p_sp);
        return PS_ERROR;
    }
    
    // Parse the 'relative to' tokens
    if (t_stat == PS_NORMAL)
        t_stat = p_sp . skip_token(SP_FACTOR, TT_TO, PT_RELATIVE);
    
    if (t_stat == PS_NORMAL)
        t_stat = p_sp . skip_token(SP_FACTOR, TT_TO, PT_TO);
    
    // Parse the target object clause
    if (t_stat == PS_NORMAL)
    {
        m_relative_object = new MCChunk(false);
        t_stat = m_relative_object -> parse(p_sp, False);
    }
    else
    {
        MCperror->add
        (PE_RESOLVE_BADOBJECT, p_sp);
        return PS_ERROR;
    }
    return t_stat;
}

Exec_stat MCResolveImage::exec(MCExecPoint &p_ep)
{
    Exec_stat t_stat;
    t_stat = ES_NORMAL;
    
    uint4 t_part_id;
    MCObject *t_relative_object;
    if (t_stat == ES_NORMAL)
        t_stat = m_relative_object -> getobj(p_ep, t_relative_object, t_part_id, True);
    
    if (t_stat == ES_NORMAL)
        t_stat = m_id_or_name -> eval(p_ep);
    
    MCImage *t_found_image;
    t_found_image = nil;
    if (t_stat == ES_NORMAL)
    {
        if (m_is_id)
        {
            if (p_ep . ton() == ES_ERROR)
            {
                MCeerror -> add(EE_VARIABLE_NAN, line, pos);
                return ES_ERROR;
            }
            
            t_found_image = t_relative_object -> resolveimageid(p_ep . getuint4());
        }
        else
            t_found_image = t_relative_object -> resolveimagename(p_ep . getsvalue());
        
        if (t_found_image != nil)
            t_stat = t_found_image -> getprop(0, P_LONG_ID, p_ep, False);
        else
            p_ep . clear();
    }
    
    if (t_stat == ES_NORMAL)
        t_stat = m_it -> set(p_ep);
    
    return t_stat;
    
}

////////////////////////////////////////////////////////////////////////////////
//
//  MW-2013-11-14: [[ AssertCmd ]] Implementation of 'assert' command.
//

MCAssertCmd::~MCAssertCmd(void)
{
	delete m_expr;
}

// assert <expr>
// assert true <expr>
// assert false <expr>
// assert success <expr>
// assert failure <expr>
Parse_stat MCAssertCmd::parse(MCScriptPoint& sp)
{
	initpoint(sp);
	
	// See if there is a type token
	MCScriptPoint temp_sp(sp);
	if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_TRUE) == PS_NORMAL)
		m_type = TYPE_TRUE;
	else if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_FALSE) == PS_NORMAL)
		m_type = TYPE_FALSE;
	else if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_SUCCESS) == PS_NORMAL)
		m_type = TYPE_SUCCESS;
	else if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_FAILURE) == PS_NORMAL)
		m_type = TYPE_FAILURE;
	else
		m_type = TYPE_NONE;
	
	// Now try to parse an expression
	if (sp.parseexp(False, True, &m_expr) == PS_NORMAL)
		return PS_NORMAL;

	// Now if we are not of NONE type, then backup and try for just an
	// expression (TYPE_NONE).
	if (m_type != TYPE_NONE)
	{
		MCperror -> clear();
		sp = temp_sp;
	}
	
	// Parse the expression again (if not NONE, otherwise we already have
	// a badexpr error to report).
	if (m_type == TYPE_NONE ||
		sp.parseexp(False, True, &m_expr) != PS_NORMAL)
	{
		MCperror -> add(PE_ASSERT_BADEXPR, sp);
		return PS_ERROR;
	}
	
	// We must be of type none.
	m_type = TYPE_NONE;
	
	return PS_NORMAL;
}

Exec_stat MCAssertCmd::exec(MCExecPoint& ep)
{
	Exec_stat t_stat;
	t_stat = ES_NORMAL;
	
	t_stat = m_expr -> eval(ep);
	
	switch(m_type)
	{
		case TYPE_NONE:
		case TYPE_TRUE:
			// If the expression threw an error, then just throw.
			if (t_stat != ES_NORMAL)
				return ES_ERROR;
			
			// If the expression is true, we are done.
			if (ep.getsvalue() == MCtruemcstring)
				return ES_NORMAL;
		break;
		
		case TYPE_FALSE:
			// If the expression threw an error, then just throw.
			if (t_stat != ES_NORMAL)
				return ES_ERROR;
			
			// If the expression is not true, we are done. (this uses the same logic as if).
			if (ep.getsvalue() != MCtruemcstring)
				return ES_NORMAL;
		break;
		
		case TYPE_SUCCESS:
			if (t_stat == ES_NORMAL)
				return ES_NORMAL;
			break;
		
		case TYPE_FAILURE:
			if (t_stat == ES_ERROR)
				return ES_NORMAL;
			break;
			
		default:
			assert(false);
			break;
	}
	
	// Clear the execution error.
	MCeerror -> clear();
	
	// Dispatch 'assertError <handler>, <line>, <pos>, <object>'
	MCParameter t_handler, t_line, t_pos, t_object;
	t_handler.setnameref_unsafe_argument(ep.gethandler()->getname());
	t_handler.setnext(&t_line);
	t_line.setn_argument((real8)line);
	t_line.setnext(&t_pos);
	t_pos.setn_argument((real8)pos);
	t_pos.setnext(&t_object);
	ep.getobj()->getprop(0, P_LONG_ID, ep, False);
	t_object.sets_argument(ep.getsvalue());
	
	return ep.getobj() -> message(MCM_assert_error, &t_handler);
}
