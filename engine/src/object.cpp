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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "execpt.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "button.h"
#include "image.h"
#include "cdata.h"
#include "stacklst.h"
#include "sellst.h"
#include "undolst.h"
#include "hndlrlst.h"
#include "handler.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "param.h"
#include "font.h"
#include "util.h"
#include "debug.h"
#include "aclip.h"
#include "vclip.h"
#include "field.h"
#include "chunk.h"
#include "objectstream.h"
#include "parentscript.h"
#include "bitmapeffect.h"
#include "osspec.h"
#include "player.h"
#include "scrolbar.h"
#include "styledtext.h"

#include "globals.h"
#include "mctheme.h"

#include "license.h"
#include "context.h"
#include "mode.h"
#include "stacksecurity.h"

#include "graphicscontext.h"

#include "resolution.h"

// PM-2014-11-11: [[ Bug 13970 ]] Added for the MCplayers' syncbuffering call
#ifdef FEATURE_PLATFORM_PLAYER
#include "platform.h"
#endif

////////////////////////////////////////////////////////////////////////////////

uint1 MCObject::dashlist[2] = {4, 4};
uint1 MCObject::dotlist[2] = {1, 1};
int1 MCObject::dashoffset;
MCRectangle MCObject::selrect;
int2 MCObject::startx;
int2 MCObject::starty;
uint1 MCObject::menudepth;
MCStack *MCObject::attachedmenu;
uint2 MCObject::s_last_font_index = 0;

bool MCObject::s_loaded_parent_script_reference = false;

MCColor MCObject::maccolors[MAC_NCOLORS] = {
            { 0, 0x7070, 0x7070, 0x7070, 0, 0 },
            { 0, 0xCCCC, 0xCCCC, 0xFFFF, 0, 0 },
            { 0, 0x9999, 0x9999, 0xFFFF, 0, 0 },
            { 0, 0x6666, 0x6666, 0xCCCC, 0, 0 },
            { 0, 0x3333, 0x3333, 0x9999, 0, 0 },
            { 0, 0x0000, 0x0000, 0x5555, 0, 0 },
            { 0, 0xE8E8, 0xE8E8, 0xE8E8, 0, 0 }
        };

MCObject::MCObject()
{
	parent = NULL;
	obj_id = 0;
	/* UNCHECKED */ MCNameClone(kMCEmptyName, _name);
	flags = F_VISIBLE | F_SHOW_BORDER | F_3D | F_OPAQUE;
	fontheight = 0;
	dflags = 0;
	ncolors = 0;

	colors = NULL;
	colornames = NULL;
	npatterns = 0;
	patterns = NULL;
	opened = 0;
	script = NULL;
	hlist = NULL;
	scriptdepth = 0;
	state = CS_CLEAR;
	borderwidth = DEFAULT_BORDER;
	shadowoffset = DEFAULT_SHADOW;
	props = NULL;
	tooltip = NULL;
	altid = 0;
	ink = GXcopy;
	extraflags = 0;
	hashandlers = 0;

	blendlevel = 100;

	// MW-2004-11-26: Initialise rect (VG)
	rect . x = rect . y = rect . width = rect . height = 0;

	// MW-2008-10-25: Initialize the parent script link to NULL
	parent_script = NULL;

	// MW-2009-08-25: Initialize the handle to NULL
	m_weak_handle = NULL;

	// MW-2012-02-14: [[ Fonts ]] Initialize the font to nil.
	m_font = nil;

	// MW-2012-02-16: [[ LogFonts ]] Initialize the font flags to 0.
	m_font_flags = 0;

	// MW-2012-02-16: [[ LogFonts ]] Initialize the font's attrs to nil.
	m_font_attrs = nil;

	// MM-2012-09-05: [[ Property Listener ]]
	m_listening = false;
	m_properties_changed = kMCPropertyChangedMessageTypeNone;
	
	// MW-2012-10-10: [[ IdCache ]]
	m_in_id_cache = false;
	
	// IM-2013-04-16: Initialize to false;
	m_script_encrypted = false;
}

MCObject::MCObject(const MCObject &oref) : MCDLlist(oref)
{
	if (oref.parent == NULL)
		parent = MCdefaultstackptr;
	else
		parent = oref.parent;
	
	/* UNCHECKED */ MCNameClone(oref . getname(), _name);
	
	obj_id = 0;
	rect = oref.rect;
	flags = oref.flags;
	fontheight = oref.fontheight;
	dflags = oref.dflags;
	ncolors = oref.ncolors;
	if (ncolors > 0)
	{
		colors = new MCColor[ncolors];
		colornames = new char *[ncolors];
		uint2 i;
		for (i = 0 ; i < ncolors ; i++)
		{
			colors[i] = oref.colors[i];
			colornames[i] = strclone(oref.colornames[i]);
		}
	}
	else
	{
		colors = NULL;
		colornames = NULL;
	}
	npatterns = oref.npatterns;
	if (npatterns > 0)
	{
		/* UNCHECKED */ MCMemoryNewArray(npatterns, patterns);
		uint2 i;
		for (i = 0 ; i < npatterns ; i++)
			patterns[i].id = oref.patterns[i].id;
	}
	else
	{
		patterns = NULL;
	}
	opened = 0;
	script = strclone(oref.script);
	m_script_encrypted = oref.m_script_encrypted;
	hlist = NULL;
	scriptdepth = 0;
	state = oref.state & ~CS_SELECTED;
	borderwidth = oref.borderwidth;
	shadowoffset = oref.shadowoffset;
	/* UNCHECKED */ oref . clonepropsets(props);
	tooltip = strclone(oref.tooltip);
	altid = oref.altid;
	ink = oref.ink;
	extraflags = oref.extraflags;
	hashandlers = 0;

	blendlevel = oref . blendlevel;

	// MW-2008-10-25: Initialize the parent script link to a clone of the source
	//   object (if any).
	if (oref . parent_script != NULL)
		parent_script = oref . parent_script -> Clone(this);
	else
		parent_script = NULL;

	// MW-2009-08-25: Initialize the handle to NULL
	m_weak_handle = NULL;

	// MW-2012-02-14: [[ FontRefs ]] As objects start off closed, the font is not
	//   copied and starts nil.
	m_font = nil;

	// MW-2012-02-16: [[ LogFonts ]] Copy the other object's font flags.
	m_font_flags = oref . m_font_flags;

	// MW-2012-02-16: [[ LogFonts ]] Copy the other object's font attrs.
	copyfontattrs(oref);
	
	// MM-2012-09-05: [[ Property Listener ]]
	m_listening = false;
	m_properties_changed = kMCPropertyChangedMessageTypeNone;
	
	// MW-2012-10-10: [[ IdCache ]]
	m_in_id_cache = false;
}

MCObject::~MCObject()
{
	while (opened)
		close();

	// MW-2012-02-16: [[ LogFonts ]] Delete the font attrs (if any).
	clearfontattrs();

	// MW-2009-08-25: Clear the handle.
	if (m_weak_handle != NULL)
		m_weak_handle -> Clear();

	// MW-2008-10-25: Release the parent script use
	if (parent_script != NULL)
		parent_script -> Release();

	// MW-2009-11-03: Clear all current breakpoints for this object
	MCB_clearbreaks(this);

	if (MCerrorptr == this)
		MCerrorptr = NULL;
	if (state & CS_SELECTED)
		MCselected->remove(this);
	IO_freeobject(this);
	MCscreen->cancelmessageobject(this, NULL);
	removefrom(MCfrontscripts);
	removefrom(MCbackscripts);
	MCundos->freeobject(this);
	delete hlist;
	MCNameDelete(_name);
	delete colors;
	if (colornames != NULL)
	{
		while (ncolors--)
			delete colornames[ncolors];
		delete colornames;
	}
	MCMemoryDeleteArray(patterns);
	delete script;
	deletepropsets();
	delete tooltip;
	
	MCModeObjectDestroyed(this);

	// MW-2012-11-20: [[ IdCache ]] Make sure we delete the object from the cache - not
	//   all deletions vector through 'scheduledelete'.
	if (m_in_id_cache)
		getstack() -> uncacheobjectbyid(this);
}

Chunk_term MCObject::gettype() const
{
	return CT_UNDEFINED;
}

const char *MCObject::gettypestring()
{
	return MCcontrolstring;
}

// Object names are always compared effectively.
bool MCObject::hasname(MCNameRef p_other_name)
{
	return MCNameIsEqualTo(getname(), p_other_name, kMCCompareCaseless);
}

void MCObject::setname(MCNameRef p_new_name)
{
	MCNameDelete(_name);
	/* UNCHECKED */ MCNameClone(p_new_name, _name);
}

void MCObject::setname_cstring(const char *p_new_name)
{
	MCNameDelete(_name);
	/* UNCHECKED */ MCNameCreateWithCString(p_new_name, _name);
}

void MCObject::setname_oldstring(const MCString& p_new_name)
{
	MCNameDelete(_name);
	/* UNCHECKED */ MCNameCreateWithOldString(p_new_name, _name);
}

void MCObject::open()
{
	if (opened++ != 0)
		return;

	if (obj_id == 0 && parent != nil)
		obj_id = getstack()->newid();

	// MW-2012-02-14: [[ FontRefs ]] Map the object's font.
	mapfont();

	for (uint32_t i = 0 ; i < ncolors ; i++)
		MCscreen->alloccolor(colors[i]);

	for (uint32_t i = 0 ; i < npatterns ; i++)
		patterns[i].pattern = MCpatternlist->allocpat(patterns[i].id, this);
}

void MCObject::close()
{
	if (opened == 0 || --opened != 0)
		return;

	if (state & CS_MENU_ATTACHED)
		closemenu(False, True);

	for (uint32_t i = 0 ; i < npatterns ; i++)
		MCpatternlist->freepat(patterns[i].pattern);

	// MW-2012-02-14: [[ FontRefs ]] Unmap the object's font.
	unmapfont();

	if (state & CS_SELECTED)
		MCselected->remove(this);

    // MM-2012-05-32: [[ Bug ]] Make sure the closed object is not the drag target or source.  
    //      Causes crash on drag drop if target object no longer exists.
	if (this == MCdragdest)
		MCdragdest = nil;
	
	if (this == MCdragsource)
		MCdragsource = nil;
	
	if (MCfreescripts && scriptdepth == 0 && hlist != NULL && !hlist->hasvars())
	{
		delete hlist;
		hlist = NULL;
	}
}

void MCObject::kfocus()
{}

Boolean MCObject::kfocusnext(Boolean top)
{
	if (!(flags & F_TRAVERSAL_ON) || state & CS_KFOCUSED
	        || !(flags & F_VISIBLE || MCshowinvisibles) || flags & F_DISABLED)
		return False;
	return True;
}

Boolean MCObject::kfocusprev(Boolean bottom)
{
	if (!(flags & F_TRAVERSAL_ON) || state & CS_KFOCUSED
	        || !(flags & F_VISIBLE || MCshowinvisibles) || flags & F_DISABLED)
		return False;
	return True;
}

void MCObject::kunfocus()
{}

Boolean MCObject::kdown(const char *string, KeySym key)
{
	char kstring[U4L];
	sprintf(kstring, "%d", (int)key);
	if (message_with_args(MCM_raw_key_down, kstring) == ES_NORMAL)
		return True;
	if (key >= XK_F1 && key <= XK_F35)
	{
		char cstring[U2L];
		sprintf(cstring, "%d", (int)(key - XK_F1 + 1));
		if (message_with_args(MCM_function_key, cstring) == ES_NORMAL)
			return True;
		if (key == XK_F1 && message_with_args(MCM_help, string) == ES_NORMAL)
			return True;
		//return False;
	}
	switch (key)
	{
	//case XK_F1:
	case XK_osfHelp:
		if (message(MCM_help) == ES_NORMAL)
			return True;
		break;
	case XK_Tab:
		if (message(MCM_tab_key) == ES_NORMAL)
			return True;
		if (MCmodifierstate & MS_SHIFT)
			getcard()->kfocusprev(False);
		else
			getcard()->kfocusnext(False);
		break;
	case XK_Return:
		if (message(MCM_return_key) == ES_NORMAL)
			return True;
		break;
	case XK_KP_Enter:
		if (message(MCM_enter_key) == ES_NORMAL)
			return True;
		break;
	case XK_Escape:
		if (message(MCM_escape_key) == ES_NORMAL)
			return True;
		break;
	case XK_Delete:
		if (MCmodifierstate & MS_SHIFT)
		{
			if (message(MCM_cut_key) == ES_NORMAL)
				return True;
		}
		else
			if (message(MCM_delete_key) == ES_NORMAL)
				return True;
		break;
	case XK_BackSpace:
		if (MCmodifierstate & MS_MOD1)
		{
			if (message(MCM_undo_key) == ES_NORMAL)
				return True;
		}
		else
			if (message(MCM_backspace_key) == ES_NORMAL)
				return True;
		break;
	case XK_osfUndo:
		if (message(MCM_undo_key) == ES_NORMAL)
			return True;
		break;
	case XK_osfCut:
		if (message(MCM_cut_key) == ES_NORMAL)
			return True;
		break;
	case XK_osfCopy:
		if (message(MCM_copy_key) == ES_NORMAL)
			return True;
		break;
	case XK_osfPaste:
		if (message(MCM_paste_key) == ES_NORMAL)
			return True;
		break;
	case XK_Insert:
		if (MCmodifierstate & MS_CONTROL)
		{
			if (message(MCM_copy_key) == ES_NORMAL)
				return True;
		}
		else
			if (MCmodifierstate & MS_SHIFT)
			{
				if (message(MCM_paste_key) == ES_NORMAL)
					return True;
			}
		break;
	case XK_Left:
		if (message_with_args(MCM_arrow_key, "left") == ES_NORMAL)
			return True;
		break;
	case XK_Right:
		if (message_with_args(MCM_arrow_key, "right") == ES_NORMAL)
			return True;
		break;
	case XK_Up:
		if (message_with_args(MCM_arrow_key, "up") == ES_NORMAL)
			return True;
		break;
	case XK_Down:
		if (message_with_args(MCM_arrow_key, "down") == ES_NORMAL)
			return True;
		break;
	default:
		char tstring[U2L];
        // SN-2014-12-08: [[ Bug 12681 ]] Avoid to print the keycode in case we have a
        // numeric keypad keycode.
        if (key > 0xFF && (key < XK_KP_Space || key > XK_KP_Equal))
			sprintf(tstring, "%ld", key);
		else
			if ((uint1)string[0] < ' ' || MCmodifierstate & MS_CONTROL)
			{
				tstring[0] = (char)key;
				tstring[1] = '\0';
			}
			else
			{
				tstring[0] = string[0];
				tstring[1] = '\0';
			}
		if (MCmodifierstate & MS_CONTROL)
		{
			if ((key > 0xFF || string[0] != '\0' || tstring[0] == ' ')
			        && (message_with_args(MCM_command_key_down, tstring) == ES_NORMAL))
				return True;
		}
		else if (MCmodifierstate & MS_MOD1)
		{
			if ((key > 0xFF || string[0] != '\0' || tstring[0] == ' ')
			        && (message_with_args(MCM_option_key_down, tstring) == ES_NORMAL))
				return True;
		}
#ifdef _MACOSX
		else if (MCmodifierstate & MS_MAC_CONTROL)
		{
			if ((key > 0xFF || string[0] != '\0' || tstring[0] == ' ')
			        && (message_with_args(MCM_control_key_down, tstring) == ES_NORMAL))
				return True;
		}
#endif
		else
			if ((string[0] != '\0' && message_with_args(MCM_key_down, string) == ES_NORMAL))
					return True;
		break;
	}

	if ((key > 0xFF || (MCmodifierstate & (MS_CONTROL | MS_MAC_CONTROL | MS_ALT))) && MCstacks->doaccelerator(key))
		return true;

	if (!MCemacskeys && MCmodifierstate & MS_CONTROL)
	{
		switch (key)
		{
		case XK_C:
		case XK_c:
			if (message(MCM_copy_key) == ES_NORMAL)
				return True;
			break;
		case XK_V:
		case XK_v:
			if (message(MCM_paste_key) == ES_NORMAL)
				return True;
			break;
		case XK_X:
		case XK_x:
			if (message(MCM_cut_key) == ES_NORMAL)
				return True;
			break;
		case XK_Z:
		case XK_z:
			if (message(MCM_undo_key) == ES_NORMAL)
				return True;
			break;
		}
	}
	if (state & CS_MENU_ATTACHED && attachedmenu != NULL)
	{
		MCStack *oldmenu = attachedmenu;
		MCString pick;
		uint2 mh;
		switch (key)
		{
		case XK_Escape:
			closemenu(True, True);
			return True;
		case XK_space:
		case XK_Return:
		case XK_KP_Enter:
			closemenu(False, True);
			oldmenu->menukdown(string, key, pick, mh);
			delete (char *)pick.getstring();
			message_with_args(MCM_mouse_up, Button1);
			return True;
		default:
			MCButton *mbptr = attachedmenu->findmnemonic(string[0]);
			if (mbptr != NULL)
			{
				closemenu(False, True);
				oldmenu->menukdown(string, key, pick, mh);
				mbptr->activate(False, string[0]);
				message_with_args(MCM_mouse_up, Button1);
				return True;
			}
		}
	}
	return False;
}

Boolean MCObject::kup(const char *string, KeySym key)
{
	char kstring[U4L];
	sprintf(kstring, "%d", (int)key);
	if (message_with_args(MCM_raw_key_up, kstring) == ES_NORMAL)
		return True;

	// MW-2005-08-31: We need an unsigned comparison here - otherwise accented characters
	//   don't trigger a keyup!
	// OK-2010-04-01: [[Bug 6215]] - Need to also check for arrow keys here using the KeySym
	//   as they don't have an ascii code.
	if ((unsigned)(string[0]) >= 32 && string[0] != 127 && key != XK_Left && key != XK_Right && key != XK_Up && key != XK_Down)
		if (message_with_args(MCM_key_up, string) == ES_NORMAL)
			return True;
	return False;
}

Boolean MCObject::mfocus(int2 x, int2 y)
{
	if (state & CS_MENU_ATTACHED && attachedmenu != NULL)
	{
		int2 tx = x;
		int2 ty = y;
		getstack()->translatecoords(attachedmenu, tx, ty);
		attachedmenu->mfocus(tx, ty);
		return True;
	}
	else
		return False;
}

void MCObject::mdrag(void)
{
	message(MCM_drag_start);
	MCdragtargetptr = this;
}

void MCObject::mfocustake(MCControl *target)
{
	parent->mfocustake(target);
}

void MCObject::munfocus()
{}

Boolean MCObject::mdown(uint2 which)
{
	if (state & CS_MENU_ATTACHED)
	{
		int2 tx = MCmousex;
		int2 ty = MCmousey;
		getstack()->translatecoords(attachedmenu, tx, ty);
		if (!MCU_point_in_rect(attachedmenu->getcard()->getrect(),
		                       tx - 1, ty - 1))
		{
			MCButton *focused = (MCButton *)attachedmenu->getcurcard()->getmfocused();

			while (focused != NULL && focused->gettype() == CT_BUTTON
			        && focused->getmenumode() == WM_CASCADE)
			{
				tx = MCmousex;
				ty = MCmousey;
				getstack()->translatecoords(focused->getmenu(), tx, ty);
				if (MCU_point_in_rect(focused->getmenu()->getcurcard()->getrect(),
				                      tx - 1, ty - 1))
				{
					state |= CS_MFOCUSED;
					return True;
				}
				focused = (MCButton *)focused->getmenu()->getcurcard()->getmfocused();
			}
			closemenu(False, True);
			state &= ~CS_MFOCUSED;
			return False;
		}
		else
		{
			state |= CS_MFOCUSED;
			attachedmenu->mdown(which);
			return True;
		}
	}
	else
		return False;
}

extern bool MCmenupoppedup;
Boolean MCObject::mup(uint2 which, bool p_release)
{
	if (state & CS_MENU_ATTACHED)
	{
		if (MCU_abs(MCmousex - startx) < MCdoubledelta
		        && MCU_abs(MCmousey - starty) < MCdoubledelta)
			return True;
		state &= ~CS_MFOCUSED;
		MCButton *focused = (MCButton *)attachedmenu->getcurcard()->getmfocused();
		if (focused != NULL && focused->gettype() == CT_BUTTON
		        && focused->getmenumode() == WM_CASCADE)
		{
			focused->mup(which, p_release); // send mup directly to cascade button
			closemenu(True, True);
		}
		else
		{
			MCStack *oldmenu = attachedmenu;
			closemenu(True, True);
			MCString pick;
			uint2 menuhistory;
			MCmenupoppedup = true;
			oldmenu->menumup(which, pick, menuhistory);
			MCmenupoppedup = false;
			delete (char *)pick.getstring();
		}
		return True;
	}
	else
		return False;
}

Boolean MCObject::doubledown(uint2 which)
{
	return False;
}

Boolean MCObject::doubleup(uint2 which)
{
	return False;
}

void MCObject::timer(MCNameRef mptr, MCParameter *params)
{
	if (MCNameIsEqualTo(mptr, MCM_idle, kMCCompareCaseless))
	{
		if (opened && hashandlers & HH_IDLE
		        && getstack()->gettool(this) == T_BROWSE)
			if (message(mptr, params, True, True) == ES_ERROR)
				senderror();
			else
				MCscreen->addtimer(this, MCM_idle, MCidleRate);
	}
	else
	{
		MCHandler handler(HT_MESSAGE);
		handler.clearpass(); // detect passed messages
		Exec_stat stat = message(mptr, params, True, True);
		if (stat == ES_NOT_HANDLED && !handler.getpass())
		{
			char *tptr = NULL;
			const char *t_mptr_cstring;
			t_mptr_cstring = MCNameGetCString(mptr);
			if (params != NULL)
			{
				MCExecPoint ep(this, NULL, NULL);
				params->eval(ep);
				char *p = ep.getsvalue().clone();
				tptr = new char[strlen(t_mptr_cstring) + ep.getsvalue().getlength() + 2];
				sprintf(tptr, "%s %s", t_mptr_cstring, p);
				delete p;
			}
			
			MCHandler *t_handler;
			t_handler = findhandler(HT_MESSAGE, mptr);
			if (t_handler == NULL || !t_handler -> isprivate())
				domess(params == NULL ? t_mptr_cstring : tptr);

			delete tptr;
		}
		if (stat == ES_ERROR && !MCNameIsEqualTo(mptr, MCM_error_dialog, kMCCompareCaseless))
			senderror();
	}
}

uint2 MCObject::gettransient() const
{
	return 0;
}

void MCObject::setrect(const MCRectangle &nrect)
{
	rect = nrect;
}

void MCObject::select()
{
	state |= CS_SELECTED;
}

void MCObject::deselect()
{
	state &= ~CS_SELECTED;
}

Boolean MCObject::del()
{
	fprintf(stderr, "Object: ERROR tried to delete %s\n", getname_cstring());
	return False;
}

void MCObject::paste(void)
{
	fprintf(stderr, "Object: ERROR tried to paste %s\n", getname_cstring());
}

void MCObject::undo(Ustruct *us)
{
	fprintf(stderr, "Object: ERROR tried to undo %s\n", getname_cstring());
}

void MCObject::freeundo(Ustruct *us)
{}

MCStack *MCObject::getstack()
{
	if (parent == NULL)
		return MCdefaultstackptr;
	return parent->getstack();
}

Exec_stat MCObject::exechandler(MCHandler *hptr, MCParameter *params)
{
	Exec_stat stat;
	if (MCcheckstack && MCU_abs(MCstackbottom - (char *)&stat) > MCrecursionlimit)
	{
		MCeerror->add(EE_RECURSION_LIMIT, 0, 0);
		MCerrorptr = this;
		return ES_ERROR;
	}

	if (MCmessagemessages)
		sendmessage(hptr -> gettype(), hptr -> getname(), True);
	
	scriptdepth++;
	if (scriptdepth == 255)
		MCfreescripts = False; // prevent recursion wrap
	MCExecPoint ep(this, hlist, hptr);
	if (MCtracestackptr != NULL && MCtracereturn)
	{
		Boolean oldtrace = MCtrace;
		if (MCtracestackptr == getstack())
			MCtrace = True;
		stat = hptr->exec(ep, params);
		if (MCtrace && !oldtrace)
		{
			MCB_done(ep);
			MCtrace = False;
		}
	}
	else
		stat = hptr->exec(ep, params);
	if (stat == ES_ERROR)
	{
		// MW-2011-06-23: [[ SERVER ]] If the handler has a file index, it
		//   isn't attached to an object. So record the error slightly
		//   differently.
		if (hptr -> getfileindex() == 0)
		{
			MCExecPoint ep(this, NULL, NULL);
			getprop(0, P_LONG_ID, ep, False);
			MCeerror->add(EE_OBJECT_NAME, 0, 0, ep.getsvalue());
		}
		else
		{
			char t_buffer[U2L];
			sprintf(t_buffer, "%u", hptr -> getfileindex());
			MCeerror -> add(EE_SCRIPT_FILEINDEX, 0, 0, t_buffer);
		}
	}
	scriptdepth--;

	return stat;
}

Exec_stat MCObject::execparenthandler(MCHandler *hptr, MCParameter *params, MCParentScriptUse *parentscript)
{
	Exec_stat stat;
	if (MCcheckstack && MCU_abs(MCstackbottom - (char *)&stat) > MCrecursionlimit)
	{
		MCeerror->add(EE_RECURSION_LIMIT, 0, 0);
		MCerrorptr = this;
		return ES_ERROR;
	}

	if (MCmessagemessages)
		sendmessage(hptr -> gettype(), hptr -> getname(), True);

	scriptdepth++;
	if (scriptdepth == 255)
		MCfreescripts = False; // prevent recursion wrap
	MCObject *t_parentscript_object = parentscript->GetParent()->GetObject();
	t_parentscript_object->scriptdepth++;
	if (t_parentscript_object->scriptdepth == 255)
		MCfreescripts = False; // prevent recursion wrap

	MCExecPoint ep(this, t_parentscript_object -> hlist, hptr);
	ep.setparentscript(parentscript);
	if (MCtracestackptr != NULL && MCtracereturn)
	{
		Boolean oldtrace = MCtrace;
		if (MCtracestackptr == getstack())
			MCtrace = True;
		stat = hptr->exec(ep, params);
		if (MCtrace && !oldtrace)
		{
			MCB_done(ep);
			MCtrace = False;
		}
	}
	else
		stat = hptr->exec(ep, params);
	if (stat == ES_ERROR)
	{
		MCExecPoint ep(this, NULL, NULL);
		parentscript -> GetParent() -> GetObject() -> getprop(0, P_LONG_ID, ep, False);
		MCeerror->add(EE_OBJECT_NAME, 0, 0, ep.getsvalue());
	}
	scriptdepth--;
	t_parentscript_object->scriptdepth--;

	return stat;
}

// MW-2012-08-08: [[ BeforeAfter ]] This handler looks for the given handler type
//   in a parentScript, if any, and executes it if found. [ Inherited parentscripts
//   should be ignored for now as the semantics for those is not clear ].
Exec_stat MCObject::handleparent(Handler_type p_handler_type, MCNameRef p_message, MCParameter *p_parameters)
{
	Exec_stat t_stat;
	t_stat = ES_NOT_HANDLED;
	
	// Fetch the first parentScript (Use).
	MCParentScriptUse *t_parentscript;
	t_parentscript = parent_script;

	// Loop until the chain is exhausted. (Note that this is the chain of
	// parentScript USES - it lies parallel the chain of parentScript
	// properties).
	while(t_parentscript != NULL)
	{
		// Fetch the object containing the script of this parentScript
		MCObject *t_parent_object;
		t_parent_object = t_parentscript -> GetParent() -> GetObject();

		// If the parent object hasn't been resolved, we are done.
		if (t_parent_object == NULL)
			break;

		// Make sure the parent object's script is compiled.
		t_parent_object -> parsescript(True);

		// If it has handlers, we have something to search, otherwise
		// continue to the next parentScript.
		if (t_parent_object -> hlist != NULL)
		{
			// Search for the handler in the parent object's handler list.
			MCHandler *t_parent_handler;
			if (t_parent_object -> hlist -> findhandler(p_handler_type, p_message, t_parent_handler) == ES_NORMAL)
			{
				// If the handler is not private then execute it.
				if (!t_parent_handler -> isprivate())
				{
					// Execute the handler we have found in parent context
					t_stat = execparenthandler(t_parent_handler, p_parameters, t_parentscript);

					// If the execution didn't fall through due to passing.
					if (t_stat != ES_PASS && t_stat != ES_NOT_HANDLED)
						return t_stat;
				}
			}
		}

		// MW-2013-05-30: [[ InheritedPscripts] Move to the next parentScript in
		//   the chain (making sure we use the parallel 'use' chain for this
		//   instance).
		t_parentscript = t_parentscript -> GetSuper();
	}
	
	return t_stat;
}

// MW-2009-01-29: [[ Bug ]] Cards and stack parentScripts don't work.
// This method first looks for a handler for the given message in its own script,
// and executes it. If one is not found, or the message is passed, it moves onto
// the parentScript of the object.
Exec_stat MCObject::handleself(Handler_type p_handler_type, MCNameRef p_message, MCParameter* p_parameters)
{	
	Exec_stat t_stat;
	t_stat = ES_NOT_HANDLED;

	// Make sure this object has its script compiled.
	parsescript(True);

	// MW-2012-08-08: [[ BeforeAfter ]] If we have a parentScript then see if there
	//   is a before handler to execute.
	if (p_handler_type == HT_MESSAGE && parent_script != nil)
	{
		// Try to invoke a before handler.
		t_stat = handleparent(HT_BEFORE, p_message, p_parameters);
		
		// If we encountered an exit all or error, we are done.
		if (t_stat == ES_ERROR || t_stat == ES_EXIT_ALL)
			return t_stat;
	}

	// If this object has handlers, then look for the required handler there
	// first.
	Exec_stat t_main_stat;
	t_main_stat = ES_NOT_HANDLED;
	if (hlist != NULL)
	{
		// Search for the handler in this object's handler list.
		MCHandler *t_handler;
		if (hlist -> findhandler(p_handler_type, p_message, t_handler) == ES_NORMAL)
		{
			// If the handler is not private, then execute it
			if (!t_handler -> isprivate())
			{
				// Execute the handler we have found.
				t_main_stat = exechandler(t_handler, p_parameters);

				// If there was an error we are done.
				if (t_stat == ES_ERROR)
					return t_main_stat;
			}
		}
	}
	
	// If the object has a parent script and the object's handler passed (or wasn't
	// handled) then try the parenscript.
	if (parent_script != nil && (t_main_stat == ES_PASS || t_main_stat == ES_NOT_HANDLED))
	{
		t_main_stat = handleparent(p_handler_type, p_message, p_parameters);
		if (t_main_stat == ES_ERROR)
			return t_main_stat;
	}
	
	// MW-2012-08-08: [[ BeforeAfter ]] If we have a parentScript then see if there
	//   is an after handler to execute.
	if (p_handler_type == HT_MESSAGE && parent_script != nil)
	{
		// Try to invoke after handler.
		t_stat = handleparent(HT_AFTER, p_message, p_parameters);
		
		// If we encountered an exit all or error, we are done.
		if (t_stat == ES_ERROR || t_stat == ES_EXIT_ALL)
			return t_stat;
	}

	// Return the result of executing the main handler in the object
	return t_main_stat;
}

Exec_stat MCObject::handle(Handler_type htype, MCNameRef mess, MCParameter *params, MCObject *pass_from)
{
	// MW-2009-01-28: [[ Bug ]] Card and stack parentScripts don't work.
	// First attempt to handle the message with the script and parentScript of this
	// object.
	Exec_stat stat;
	stat = handleself(htype, mess, params);

	if (pass_from != nil && parent != NULL)
	{
		if (stat == ES_PASS || stat == ES_NOT_HANDLED)
		{
			Exec_stat oldstat = stat;
			stat = parent->handle(htype, mess, params, this);
			if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
				stat = ES_PASS;
		}
	}

	if (stat == ES_ERROR && MCerrorptr == NULL)
		MCerrorptr = this;

	return stat;
}

void MCObject::closemenu(Boolean kfocus, Boolean disarm)
{
	if (state & CS_MENU_ATTACHED)
	{
		MCscreen->ungrabpointer();
		state &= ~CS_MENU_ATTACHED;
		MCdispatcher->removemenu();
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		if (gettype() >= CT_GROUP)
			static_cast<MCControl *>(this) -> layer_redrawall();
		if (kfocus && !(state & CS_MFOCUSED))
		{
			attachedmenu->setstate(True, CS_KFOCUSED); // override state
			attachedmenu->kunfocus();
		}
		MCButton *focused = (MCButton *)attachedmenu->getcurcard()->getmfocused();
		if (focused != NULL && focused->gettype() == CT_BUTTON
		        && focused->getmenumode() == WM_CASCADE)
			focused->closemenu(kfocus, disarm);
		attachedmenu->close();
		attachedmenu = NULL;
		menudepth--;
		if (MCmenuobjectptr == this)
			MCmenuobjectptr = NULL;
	}
}

void MCObject::recompute()
{
}

const MCRectangle& MCObject::getrect(void) const
{
	return rect;
}

MCRectangle MCObject::getrectangle(bool p_effective) const
{
    if (!p_effective)
        return getrect();
    
    MCRectangle t_rect = getrect();
    MCU_reduce_rect(t_rect, gettransient());
    return t_rect;
}

void MCObject::setflag(uint4 on, uint4 flag)
{
	if (on)
		flags |= flag;
	else
		flags &= ~flag;
}

void MCObject::setextraflag(uint4 on, uint4 flag)
{
	if (on)
		extraflags |= flag;
	else
		extraflags &= ~flag;
}

void MCObject::setstate(Boolean on, uint4 newstate)
{
	if (on)
		state |= newstate;
	else
		state &= ~newstate;
}

Exec_stat MCObject::setsprop(Properties which, const MCString &s)
{
	MCExecPoint ep(this, NULL, NULL);
	ep.setsvalue(s);
	return setprop(0, which, ep, False);
}

void MCObject::help()
{
	message(MCM_help);
	MCcurtool = MColdtool;
	getstack()->resetcursor(True);
}

MCCard *MCObject::getcard(uint4 cid)
{
	if (cid == 0)
		return getstack()->getchild(CT_THIS, MCnullmcstring, CT_CARD);
	return getstack()->getcardid(cid);
}

Window MCObject::getw()
{
	return getstack()->getwindow();
}

MCParentScript *MCObject::getparentscript(void) const
{
	return parent_script != NULL ? parent_script -> GetParent() : NULL;
}

Boolean MCObject::isvisible()
{
	MCObject *p = this;
	while (p->parent != NULL && p->parent->gettype() == CT_GROUP)
	{
		if (!(p->flags & F_VISIBLE))
			return False;
		p = p->parent;
	}
	return (p->flags & F_VISIBLE) != 0;
}

Boolean MCObject::resizeparent()
{
	if (parent != NULL && parent->gettype() == CT_GROUP)
	{
		MCGroup *gptr = (MCGroup *)parent;
        
		// MERG-2013-06-02: [[ GrpLckUpdates ]] Only recalculate the group if not locked.
        if (!gptr -> islocked())
		{
            // MM-2012-09-05: [[ Property Listener ]] Moving/resizing an object within a group will potentially effect the location/rect properties of the group.
            if (gptr->computeminrect((state & (CS_MOVE | CS_SIZE)) != 0))
            {
                if (state & CS_MOVE)
                    gptr -> signallisteners(P_LOCATION);
                else
                    gptr -> signallisteners(P_RECTANGLE);
                return True;
            }
			else 
                return False;
        }
	}
	return False;
}

Boolean MCObject::getforecolor(uint2 di, Boolean rev, Boolean hilite,
                               MCColor &c, MCPatternRef &r_pattern,
                               int2 &x, int2 &y, MCDC *dc, MCObject *o)
{
	uint2 i;
	if (dc->getdepth() > 1)
	{
		Boolean hasindex = getcindex(di, i);
        // MM-2013-08-28: [[ RefactorGraphics ]] We now pack alpha values into pixels meaning checking against MAXUNIT4 means white will always be ignored. Not sure why this check was here previously.
		if (hasindex) // && colors[i].pixel != MAXUINT4)
		{
			c = colors[i];
			return True;
		}
		else
			if (getpindex(di, i))
			{
				r_pattern = patterns[i].pattern;

				if (gettype() == CT_STACK)
					x = y = 0;
				else
				{
					x = rect.x;
					y = rect.y;
				}
				return False;
			}
			else
			{
				if (di == DI_FORE && flags & F_DISABLED)
				{
					c = dc->getgray();
					return True;
				}
				if (MClook != LF_MOTIF && hilite && flags & F_OPAQUE
				        && !(flags & F_DISABLED))
				{
					if (di == DI_BACK)
						c = dc->getwhite();
					else
						parent->getforecolor(di, rev, False, c, r_pattern, x, y, dc, o);
					return True;
				}
				if (parent && parent != MCdispatcher)
					return parent->getforecolor(di, rev, False, c, r_pattern, x, y, dc, o);
			}
	}

	switch (di)
	{

	case DI_TOP:
		rev = !rev;
	case DI_BOTTOM:
	case DI_FORE:
		if (rev)
			c = dc->getwhite();
		else
			c = dc->getblack();
		break;
	case DI_BACK:
#ifdef _MACOSX
		if (IsMacLFAM() && dc -> gettype() != CONTEXT_TYPE_PRINTER)
		{
			extern bool MCMacThemeGetBackgroundPattern(Window_mode p_mode, bool p_active, MCPatternRef &r_pattern);
			x = 0;
			y = 0;
			
			if (MCMacThemeGetBackgroundPattern(o -> getstack() -> getmode(), True, r_pattern))
				return False;
		}
#endif
		c = dc->getbg();
		break;
	case DI_HILITE:
		c = o->gettype() == CT_BUTTON ? MCaccentcolor : MChilitecolor;
		break;
	case DI_FOCUS:
		{
			MCColor *syscolors = MCscreen->getaccentcolors();
			if (syscolors != NULL)
				c = syscolors[3];
			else
				c = MCaccentcolor;
		}
		break;
	default:
		c = dc->getblack();
		break;
	}
	return True;
}

void MCObject::setforeground(MCDC *dc, uint2 di, Boolean rev, Boolean hilite)
{
	uint2 idi = di;
	if (rev)
	{
		switch (idi)
		{
		case DI_TOP:
			idi = DI_BOTTOM;
			break;
		case DI_BOTTOM:
			idi = DI_TOP;
			break;
		case DI_BACK:
			idi = DI_HILITE;
			break;
		default:
			break;
		}
		rev = False;
	}

	MCColor color;
	MCPatternRef t_pattern = nil;
	int2 x, y;
	if (getforecolor(idi, rev, hilite, color, t_pattern, x, y, dc, this))
	{
		MCColor fcolor;
		if (dc->getdepth() == 1 && di != DI_BACK
		        && getforecolor((state & CS_HILITED && flags & F_OPAQUE)
		                        ? DI_HILITE : DI_BACK, False, False, fcolor,
		                        t_pattern, x, y, dc, this)
		        && color.pixel == fcolor.pixel)
			color.pixel ^= 1;
		dc->setforeground(color);
		dc->setfillstyle(FillSolid, nil, 0, 0);
	}
	else if (t_pattern == nil)
		dc->setfillstyle(FillStippled, nil, 0, 0);
	else
	{
		// MW-2011-09-22: [[ Layers ]] Check to see if the object is on a dynamic
		//   layer. If it is we use the object's origin; rather than the inherited
		//   one.
		MCObject *t_parent;
		t_parent = this;
		while(t_parent -> gettype() >= CT_GROUP)
		{
			// We use the layermode hint rather than the effective setting, as this
			// gives greater consistency of output.
			if (((MCControl *)t_parent) -> layer_getmodehint() != kMCLayerModeHintStatic)
			{
				x = rect . x;
				y = rect . y;
				break;
			}

			t_parent = t_parent -> getparent();
		}

		dc->setfillstyle(FillTiled, t_pattern, x, y);
	}
}

Boolean MCObject::setcolor(uint2 index, const MCString &data)
{
	uint2 i, j;
	if (data.getlength() == 0)
	{
		if (getcindex(index, i))
			destroycindex(index, i);
	}
	else
	{
		if (!getcindex(index, i))
		{
			i = createcindex(index);
			colors[i].red = colors[i].green = colors[i].blue = 0;
			if (opened)
				MCscreen->alloccolor(colors[i]);
		}
		MCColor oldcolor = colors[i];
		if (!MCscreen->parsecolor(data, &colors[i], &colornames[i]))
		{
			MCeerror->add
			(EE_OBJECT_BADCOLOR, 0, 0, data);
			return False;
		}
		j = i;
		if (getpindex(index, j))
		{
			if (opened)

				MCpatternlist->freepat(patterns[j].pattern);
			destroypindex(index, j);
		}
		if (opened)
			MCscreen->alloccolor(colors[i]);
	}
	return True;
}

Boolean MCObject::setcolors(const MCString &data)
{
	uint2 i, j, index;
	MCColor newcolors[8];
	char *newcolornames[8];
	for (i = 0 ; i < 8 ; i++)
		newcolornames[i] = NULL;
	if (!MCscreen->parsecolors(data, newcolors, newcolornames, 8))
	{
		MCeerror->add
		(EE_OBJECT_BADCOLORS, 0, 0, data);
		return False;
	}
	for (index = DI_FORE ; index <= DI_FOCUS ; index++)
	{
		// MW-2013-02-21: [[ Bug 10683 ]] Only clear the pattern if we are actually
		//   setting a color.
		if (newcolors[index] . flags != 0)
		{
			if (getpindex(index, j))
			{
				if (opened)
					MCpatternlist->freepat(patterns[j].pattern);
				destroypindex(index, j);
			}
		}
		if (!getcindex(index, i))
		{
			if (newcolors[index].flags)
			{
				i = createcindex(index);
				colors[i] = newcolors[index];
				if (opened)
					MCscreen->alloccolor(colors[i]);
				colornames[i] = newcolornames[index];
			}
		}
		else
		{
			if (newcolors[index].flags)
			{
				delete colornames[i];
				if (opened)
				{
					colors[i] = newcolors[index];
					MCscreen->alloccolor(colors[i]);
				}
				colornames[i] = newcolornames[index];
			}
			else
				destroycindex(index, i);
		}
	}
	return True;
}

Boolean MCObject::setpattern(uint2 newpixmap, const MCString &data)
{
	uint2 i;
	bool t_isopened;
	t_isopened = (opened != 0) || (gettype() == CT_STACK && static_cast<MCStack*>(this)->getextendedstate(ECS_ISEXTRAOPENED));
	if (data.getlength() == 0)
	{
		if (getpindex(newpixmap, i))
		{
			if (t_isopened)
				MCpatternlist->freepat(patterns[i].pattern);
			destroypindex(newpixmap, i);
		}
	}
	else
	{
		uint4 newid;
		if (!MCU_stoui4(data, newid))
		{
			MCeerror->add
			(EE_OBJECT_PIXMAPNAN, 0, 0, data);
			return False;
		}
		if (!getpindex(newpixmap, i))
			i = createpindex(newpixmap);
		else
			if (t_isopened)
				MCpatternlist->freepat(patterns[i].pattern);
		if (newid < PI_PATTERNS)
			newid += PI_PATTERNS;
		patterns[i].id = newid;
		if (t_isopened)
			patterns[i].pattern = MCpatternlist->allocpat(patterns[i].id, this);
		if (getcindex(newpixmap, i))
			destroycindex(newpixmap, i);
		}
	return True;
}

Boolean MCObject::setpatterns(const MCString &data)
{
	char *string = data.clone();
	char *sptr = string;
	uint2 p;
	Boolean done = False;
	for (p = P_FORE_PATTERN ; p <= P_FOCUS_PATTERN ; p++)
	{
		char *eptr;
		if ((eptr = strchr(sptr, '\n')) != NULL)
			*eptr++ = '\0';
		else
			eptr = &sptr[strlen(sptr)];
		if (!setpattern(p - P_FORE_PATTERN, sptr))
		{
			delete string;
			return False;
		}
		sptr = eptr;
	}
	delete string;
	return True;
}

Boolean MCObject::getcindex(uint2 di, uint2 &i)
{
	i = 0;
	uint2 j = DF_FORE_COLOR;
	while (di--)
	{
		if (dflags & j)
			i++;
		j <<= 1;
	}
	if (dflags & j)
		return True;
	else
		return False;
}

uint2 MCObject::createcindex(uint2 di)
{
	MCColor *oldcolors = colors;
	char **oldnames = colornames;
	ncolors++;
	colors = new MCColor[ncolors];
	colornames = new char *[ncolors];
	uint2 ri = 0;
	uint2 i = 0;
	uint2 c = 0;
	uint2 oc = 0;
	uint2 m = DF_FORE_COLOR;
	while (c < ncolors)
	{
		if (i == di)
		{
			dflags |= m;
			colornames[c] = NULL;
			ri = c++;
		}
		else
			if (dflags & m)
			{
				colors[c] = oldcolors[oc];
				colornames[c++] = oldnames[oc++];
			}
		i++;
		m <<= 1;
	}
	if (oldcolors != NULL)
	{
		delete oldcolors;
		delete oldnames;
	}
	return ri;
}

void MCObject::destroycindex(uint2 di, uint2 i)
{
	delete colornames[i];
	ncolors--;
	while (i < ncolors)
	{
		colors[i] = colors[i + 1];
		colornames[i] = colornames[i + 1];
		i++;
	}
	uint2 m = DF_FORE_COLOR;
	for (i = 0 ; i < di ; i++)
		m <<= 1;
	dflags &= ~m;
}

Boolean MCObject::getpindex(uint2 di, uint2 &i)
{
	i = 0;
	uint2 j = DF_FORE_PATTERN;
	while (di--)
	{
		if (dflags & j)
			i++;
		j <<= 1;
	}
	if (dflags & j)
		return True;
	else
		return False;
}

uint2 MCObject::createpindex(uint2 di)
{
	MCPatternInfo *oldpatterns = patterns;
	npatterns++;
	/* UNCHECKED */ MCMemoryNewArray(npatterns, patterns);
	uint2 ri = 0;
	uint2 i = 0;
	uint2 p = 0;
	uint2 op = 0;
	uint2 m = DF_FORE_PATTERN;
	while (p < npatterns)
	{
		if (i == di)
		{
			dflags |= m;
			ri = p++;
		}
		else
			if (dflags & m)
				patterns[p++] = oldpatterns[op++];
		i++;
		m <<= 1;
	}
	MCMemoryDeleteArray(oldpatterns);
	return ri;
}

void MCObject::destroypindex(uint2 di, uint2 i)
{
	npatterns--;
	while (i < npatterns)
	{
		patterns[i] = patterns[i + 1];
		i++;
	}
	uint2 m = DF_FORE_PATTERN;
	for (i = 0 ; i < di ; i++)
		m <<= 1;
	dflags &= ~m;
}

// MW-2012-02-21: [[ FieldExport ]] Compute the (effective) color for the given
//   index.
uint32_t MCObject::getcoloraspixel(uint2 di)
{
	uint2 t_index;
	if (!getcindex(di, t_index))
	{
		if (parent != nil && parent != MCdispatcher)
			return parent -> getcoloraspixel(di);
		switch(di)
		{
		case DI_BACK:
			return MCscreen -> background_pixel . pixel;
		case DI_HILITE:
			return MChilitecolor . pixel;
		case DI_FOCUS:
			return MCaccentcolor . pixel;
		case DI_TOP:
		case DI_BOTTOM:
		case DI_FORE:
		default:
			break;
		}
		return 0;
	}

	if (!opened)
		MCscreen -> alloccolor(colors[t_index]);

	return colors[t_index] . pixel;
}

// MW-2012-02-14: [[ FontRefs ]] New method for getting the font props from the
//   object without creating a fontstruct.
void MCObject::getfontattsnew(MCNameRef& fname, uint2 &size, uint2 &style)
{
	// MW-2012-02-19: [[ SplitTextAttrs ]] If we don't have all the attrs at this
	//   level, we must fetch the parent attrs first.
	if ((m_font_flags & FF_HAS_ALL_FATTR) != FF_HAS_ALL_FATTR)
	{
		if (this != MCdispatcher)
		{
			if (parent == nil)
				MCdefaultstackptr -> getfontattsnew(fname, size, style);
			else
				parent -> getfontattsnew(fname, size, style);
		}
		else
		{
			// This should never happen as the dispatcher always has font props
			// set.
			assert(false);
		}
	}

	// If we have a textfont, replace that value.
	if ((m_font_flags & FF_HAS_TEXTFONT) != 0)
		fname = m_font_attrs -> name;

	// If we have a textsize, replace that value.
	if ((m_font_flags & FF_HAS_TEXTSIZE) != 0)
		size = m_font_attrs -> size;

	// If we have a textstyle, replace that value.
	if ((m_font_flags & FF_HAS_TEXTSTYLE) != 0)
		style = m_font_attrs -> style;
}

void MCObject::getfontattsnew(const char *& fname, uint2 &size, uint2 &style)
{
	MCNameRef t_fname_name;
	getfontattsnew(t_fname_name, size, style);
	fname = MCNameGetCString(t_fname_name);
}

MCNameRef MCObject::gettextfont(void)
{
	MCNameRef fname;
	uint2 fsize, fstyle;
	getfontattsnew(fname, fsize, fstyle);
	return fname;
}

uint2 MCObject::gettextsize(void)
{
	MCNameRef fname;
	uint2 fsize, fstyle;
	getfontattsnew(fname, fsize, fstyle);
	return fsize;
}

uint2 MCObject::gettextstyle(void)
{
	MCNameRef fname;
	uint2 fsize, fstyle;
	getfontattsnew(fname, fsize, fstyle);
	return fstyle;
}

uint2 MCObject::gettextheight(void)
{
	if (fontheight == 0)
		return heightfromsize(gettextsize());
	return fontheight;
}

void MCObject::allowmessages(Boolean allow)
{
	if (allow)
		state &= ~CS_NO_MESSAGES;
	else
		state |= CS_NO_MESSAGES;
}

Exec_stat MCObject::conditionalmessage(uint32_t p_flag, MCNameRef p_message)
{
	// MW-2013-08-06: [[ Bug 11084 ]] Restructured to loop through object and
	//   its behavior chain.
	MCObject *t_object;
	t_object = this;
	while(t_object != nil)
	{
		// Make sure the script is parsed.
		t_object -> parsescript(True);
	
		// If the current object has the relevant handler we are done.
		if ((t_object -> hashandlers & p_flag) != 0)
			return message(p_message);
		
		// If the object has a parent script, skip to its parent script (if any).
		if (t_object -> parent_script != nil)
			t_object = t_object -> parent_script -> GetParent() -> GetObject();
		else
			t_object = nil;
	}

	return ES_NORMAL;
}

Exec_stat MCObject::dispatch(Handler_type p_type, MCNameRef p_message, MCParameter *p_params)
{
	// Fetch current default stack and target settings
	MCStack *t_old_stack;
	t_old_stack = MCdefaultstackptr;
	MCObject *t_old_target;
	t_old_target = MCtargetptr;
	
	// Cache the current 'this stack' (used to see if we should switch back
	// the default stack).
	MCStack *t_this_stack;
	t_this_stack = getstack();
	
	// Retarget this stack and the target to be relative to the target object
	MCdefaultstackptr = t_this_stack;
	MCtargetptr = this;

	// Dispatch the message
	Exec_stat t_stat;
	t_stat = MCU_dofrontscripts(p_type, p_message, p_params);
	Boolean olddynamic = MCdynamicpath;
	MCdynamicpath = MCdynamiccard != NULL;
	if (t_stat == ES_PASS || t_stat == ES_NOT_HANDLED)
		t_stat = handle(p_type, p_message, p_params, this);

	// Reset the default stack pointer and target - note that we use 'send'esque
	// semantics here. i.e. If the default stack has been changed, the change sticks.
	if (MCdefaultstackptr == t_this_stack)
		MCdefaultstackptr = t_old_stack;

	// Reset target pointer
	MCtargetptr = t_old_target;
	MCdynamicpath = olddynamic;
	
	return t_stat;
}

Exec_stat MCObject::message(MCNameRef mess, MCParameter *paramptr, Boolean changedefault, Boolean send, Boolean p_is_debug_message)
{
	MCStack *mystack = getstack();
	if (MClockmessages || MCexitall || state & CS_NO_MESSAGES || parent == NULL || (flags & F_DISABLED && mystack->gettool(this) == T_BROWSE && !send && !p_is_debug_message))
			return ES_NOT_HANDLED;

    // AL-2013-01-14: [[ Bug 11343 ]] Moved check and time addition to MCCard::mdown methods.
	//if (MCNameIsEqualTo(mess, MCM_mouse_down, kMCCompareCaseless) && hashandlers & HH_MOUSE_STILL_DOWN)
    //	MCscreen->addtimer(this, MCM_idle, MCidleRate);

	MCscreen->flush(mystack->getw());

	MCStack *oldstackptr = MCdefaultstackptr;
	MCObject *oldtargetptr = MCtargetptr;
	if (changedefault)
	{
		MCdefaultstackptr = mystack;
		MCtargetptr = this;
	}
	Boolean olddynamic = MCdynamicpath;
	MCdynamicpath = False;
	Exec_stat stat = ES_NOT_HANDLED;
	if (MCscreen->abortkey())
	{
		MCerrorptr = this;
		stat = ES_ERROR;
	}
	else
	{
		MCS_alarm(CHECK_INTERVAL);
		MCdebugcontext = MAXUINT2;
		stat = MCU_dofrontscripts(HT_MESSAGE, mess, paramptr);
		Window mywindow = mystack->getw();
		if ((stat == ES_NOT_HANDLED || stat == ES_PASS)
		        && (MCtracewindow == NULL
		            || memcmp(&mywindow, &MCtracewindow, sizeof(Window))))
		{
			// PASS STATE FIX
			Exec_stat oldstat = stat;
			stat = handle(HT_MESSAGE, mess, paramptr, this);
			if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
				stat = ES_PASS;
		}
	}
	if (!send || !changedefault || MCdefaultstackptr == mystack)
		MCdefaultstackptr = oldstackptr;
	MCtargetptr = oldtargetptr;
	MCdynamicpath = olddynamic;

	if (stat == ES_ERROR && MCerrorlock == 0 && !MCtrylock)
	{
		if (MCnoui)
		{
			uint2 line, pos;
			MCeerror->geterrorloc(line, pos);
			fprintf(stderr, "%s: Script execution error at line %d, column %d\n",
			        MCcmd, line, pos);
		}
		else
			if (!send)
				senderror();
		return ES_ERROR;
	}
	if (!send)
		MCresult->clear(False);
	return stat;
}

Exec_stat MCObject::message_with_args(MCNameRef mess, const MCString &v1)
{
	MCParameter p1;
	p1.sets_argument(v1);
	return message(mess, &p1);
}

Exec_stat MCObject::message_with_args(MCNameRef mess, const MCString &v1, const MCString &v2)
{
	MCParameter p1, p2;
	p1.sets_argument(v1);
	p1.setnext(&p2);
	p2.sets_argument(v2);
	return message(mess, &p1);
}

Exec_stat MCObject::message_with_args(MCNameRef mess, const MCString &v1, const MCString &v2, const MCString& v3)
{
	MCParameter p1, p2, p3;
	p1.sets_argument(v1);
	p1.setnext(&p2);
	p2.sets_argument(v2);
	p2.setnext(&p3);
	p3.sets_argument(v3);
	return message(mess, &p1);
}

Exec_stat MCObject::message_with_args(MCNameRef mess, const MCString &v1, const MCString &v2, const MCString& v3, const MCString& v4)
{
	MCParameter p1, p2, p3, p4;
	p1.sets_argument(v1);
	p1.setnext(&p2);
	p2.sets_argument(v2);
	p2.setnext(&p3);
	p3.sets_argument(v3);
	p3.setnext(&p4);
	p4.sets_argument(v4);
	return message(mess, &p1);
}

Exec_stat MCObject::message_with_args(MCNameRef mess, MCNameRef v1)
{
	MCParameter p1;
	p1.setnameref_unsafe_argument(v1);
	return message(mess, &p1);
}

Exec_stat MCObject::message_with_args(MCNameRef mess, MCNameRef v1, MCNameRef v2)
{
	MCParameter p1, p2;
	p1.setnameref_unsafe_argument(v1);
	p1.setnext(&p2);
	p2.setnameref_unsafe_argument(v2);
	return message(mess, &p1);
}

Exec_stat MCObject::message_with_args(MCNameRef mess, int4 v1)
{
	MCParameter p1;
	p1.setn_argument((real8)v1);
	return message(mess, &p1);
}

Exec_stat MCObject::message_with_args(MCNameRef mess, int4 v1, int4 v2)
{
	MCParameter p1, p2;
	p1.setn_argument((real8)v1);
	p1.setnext(&p2);
	p2.setn_argument((real8)v2);
	return message(mess, &p1);
}

Exec_stat MCObject::message_with_args(MCNameRef mess, int4 v1, int4 v2, int4 v3)
{
	MCParameter p1, p2, p3;
	p1.setn_argument((real8)v1);
	p1.setnext(&p2);
	p2.setn_argument((real8)v2);
	p2.setnext(&p3);
	p3.setn_argument((real8)v3);
	return message(mess, &p1);
}

Exec_stat MCObject::message_with_args(MCNameRef mess, int4 v1, int4 v2, int4 v3, int4 v4)
{
	MCParameter p1, p2, p3, p4;
	p1.setn_argument((real8)v1);
	p1.setnext(&p2);
	p2.setn_argument((real8)v2);
	p2.setnext(&p3);
	p3.setn_argument((real8)v3);
	p3.setnext(&p4);
	p4.setn_argument((real8)v4);
	return message(mess, &p1);
}

void MCObject::senderror()
{
	char *perror = NULL;
	if (!MCperror->isempty())
	{
		MCExecPoint ep(this, NULL, NULL);
		MCerrorptr->getprop(0, P_LONG_ID, ep, False);
		MCperror->add
		(PE_OBJECT_NAME, 0, 0, ep.getsvalue());
		perror = MCperror->getsvalue().clone();
		MCperror->clear();
	}
	if (MCerrorptr == NULL)
		MCerrorptr = this;
	MCscreen->delaymessage(MCerrorlockptr == NULL ? MCerrorptr : MCerrorlockptr, MCM_error_dialog, MCeerror->getsvalue().clone(), perror);
	MCeerror->clear();
	MCerrorptr = NULL;
}

void MCObject::sendmessage(Handler_type htype, MCNameRef m, Boolean h)
{
	static const char *htypes[] =	{
		"undefined",
		"message",
		"function",
		"getprop",
		"setprop",
		"before",
		"after",
		"private"
	};
	enum { max_htype = (sizeof(htypes)/sizeof(htypes[0])) - 1 };

	MCAssert(htype <= max_htype);
	MCStaticAssert(max_htype == HT_MAX);

	MCmessagemessages = False;
	MCExecPoint ep(this, NULL, NULL);
	MCresult->fetch(ep);

	// MW-2011-08-22: [[ Bug 9686 ]] Make sure we save the value of 'the result'
	//   to be restored after the (not)handled message has been processed.
	ep.grab();

	if (h)
		message_with_args(MCM_message_handled, htypes[htype], MCNameGetOldString(m));
	else
		message_with_args(MCM_message_not_handled, htypes[htype], MCNameGetOldString(m));

	MCresult->store(ep, False);

	MCmessagemessages = True;
}

Exec_stat MCObject::names(Properties which, MCExecPoint &ep, uint4 parid)
{
	const char *itypestring = gettypestring();
	char *tmptypestring = NULL;
	if (parent != NULL && gettype() >= CT_BUTTON && getstack()->hcaddress())
	{
		tmptypestring = new char[strlen(itypestring) + 7];
		if (parent->gettype() == CT_GROUP)
			sprintf(tmptypestring, "%s %s", "bkgnd", itypestring);
		else
			sprintf(tmptypestring, "%s %s", "card", itypestring);
		itypestring = tmptypestring;
	}
	Exec_stat stat = ES_NORMAL;
	switch (which)
	{
	case P_ID:
	case P_SHORT_ID:
		ep.setint(obj_id);
		break;
	case P_ABBREV_ID:
		ep.setstringf("%s id %d", itypestring, obj_id);
		break;
	case P_LONG_ID:
		if (parent == NULL)
		{
			char *buffer = ep.getbuffer(strlen(itypestring) + 13);
			sprintf(buffer, "the template%s", itypestring);
			buffer[12] = MCS_toupper(buffer[12]);
			ep.setstrlen();
		}
		else
		{
			MCExecPoint ep2(ep);
			parent->getprop(parid, P_LONG_ID, ep2, False);
			if (gettype() == CT_GROUP && parent->gettype() == CT_STACK)
				itypestring = "bkgnd";
			ep.setstringf("%s id %d of ", itypestring, obj_id);
			ep.appendmcstring(ep2.getsvalue());
		}
		break;
	case P_NAME:
	case P_ABBREV_NAME:
		if (isunnamed())
			stat = getprop(parid, P_ABBREV_ID, ep, False);
		else
			ep.setstringf("%s \"%s\"", itypestring, getname_cstring());
		break;
	case P_SHORT_NAME:
		if (isunnamed())
			stat = names(P_ABBREV_ID, ep, parid);
		else
			ep.setnameref_unsafe(getname());
		break;
	case P_LONG_NAME:
		// MW-2013-01-15: [[ Bug 2629 ]] If this control is unnamed, use the abbrev id form
		//   but *only* for this control (continue with names the rest of the way).
		if (parent == NULL)
		{
			if (!isunnamed())
				ep.setstringf("%s \"%s\"", itypestring, getname_cstring());
			else
				ep.setstringf("%s id %d", itypestring, obj_id);
		}
		else
		{
			MCExecPoint ep2(ep);
			parent->getprop(parid, P_LONG_NAME, ep2, False);
			if (gettype() == CT_GROUP && parent->gettype() == CT_STACK)
				itypestring = "bkgnd";
			if (!isunnamed())
				ep.setstringf("%s \"%s\" of ", itypestring, getname_cstring());
			else
				ep.setstringf("%s id %d of ", itypestring, obj_id);
			ep.appendmcstring(ep2.getsvalue());
		}
		break;
	default:
		stat = ES_ERROR;
		break;
	}
	delete tmptypestring;
	return stat;
}

// MW-2012-10-17: [[ Bug 10476 ]] Returns true if message should be fired.
static bool should_send_message(MCHandlerlist *p_hlist, MCNameRef p_message)
{
	MCHandler *hptr;

	if (p_hlist -> findhandler(HT_MESSAGE, p_message, hptr) == ES_NORMAL && !hptr -> isprivate())
		return true;
		
	if (p_hlist -> findhandler(HT_BEFORE, p_message, hptr) == ES_NORMAL)
		return true;
		
	if (p_hlist -> findhandler(HT_AFTER, p_message, hptr) == ES_NORMAL)
		return true;
		
	return false;
}

// report - send scriptParsingError on parse failure
// force - reparse the script into the existing hlist object
//   (or create one if there isn't one there already)
Boolean MCObject::parsescript(Boolean report, Boolean force)
{
	if (!force && hashandlers & HH_DEAD_SCRIPT)
		return False;
	if (script == nil || parent == NULL)
        hashandlers = 0;
	else
		if (force || hlist == NULL)
		{
			MCscreen->cancelmessageobject(this, MCM_idle);
			hashandlers = 0;
			if (hlist == NULL)
				hlist = new MCHandlerlist;
			
			getstack() -> unsecurescript(this);
			
			Parse_stat t_stat;
			t_stat = hlist -> parse(this, script);
			
			getstack() -> securescript(this);
			
			if (t_stat != PS_NORMAL)
			{
				hashandlers |= HH_DEAD_SCRIPT;
				if (report && parent != NULL)
				{
					MCExecPoint ep(this, NULL, NULL);
					getprop(0, P_LONG_ID, ep, False);
					MCperror->add(PE_OBJECT_NAME, 0, 0, ep.getsvalue());
					message_with_args(MCM_script_error, MCperror->getsvalue());
					MCperror->clear();
				}
				delete hlist;
				hlist = NULL;
				return False;
			}
			else
			{
				if (should_send_message(hlist, MCM_idle))
				{
					hashandlers |= HH_IDLE;
					if (opened)
						MCscreen->addtimer(this, MCM_idle, MCidleRate);
				}
				if (should_send_message(hlist, MCM_mouse_within))
					hashandlers |= HH_MOUSE_WITHIN;
				if (should_send_message(hlist, MCM_mouse_still_down))
					hashandlers |= HH_MOUSE_STILL_DOWN;
				if (should_send_message(hlist, MCM_preopen_control))
					hashandlers |= HH_PREOPEN_CONTROL;
				if (should_send_message(hlist, MCM_open_control))
					hashandlers |= HH_OPEN_CONTROL;
				if (should_send_message(hlist, MCM_close_control))
					hashandlers |= HH_CLOSE_CONTROL;
				if (should_send_message(hlist, MCM_resize_control))
					hashandlers |= HH_RESIZE_CONTROL;
			}
		}
	return True;
}

bool MCObject::handlesmessage(MCNameRef p_message)
{
	MCObject *t_object;
	t_object = this;
	while(t_object != nil)
	{
		if (t_object -> hlist != nil && should_send_message(t_object -> hlist, p_message))
            return true;
		
		// If the object has a parent script, skip to its parent script (if any).
		if (t_object -> parent_script != nil)
			t_object = t_object -> parent_script -> GetParent() -> GetObject();
		else
			t_object = nil;
	}
    
	return false;
}

Bool MCObject::hashandler(Handler_type p_type, MCNameRef p_message)
{
	return findhandler(p_type, p_message) != NULL;
}

MCHandler *MCObject::findhandler(Handler_type p_type, MCNameRef p_message)
{
	if (hlist != NULL || parsescript(False, False) && hlist != NULL)
	{
		MCHandler *t_handler;
		if (hlist -> findhandler(p_type, p_message, t_handler) == ES_NORMAL)
			return t_handler;
	}

	return NULL;
}

void MCObject::drawshadow(MCDC *dc, const MCRectangle &drect, int2 soffset)
{
	setforeground(dc, DI_SHADOW, False);
	MCRectangle trect;
	if (soffset < 0)
	{
		trect.x = drect.x;
		trect.y = drect.y;
		trect.width = drect.width + soffset;
		trect.height = -soffset;
	}
	else
	{
		trect.x = drect.x + soffset;
		trect.y = drect.y + drect.height - soffset;
		trect.width = drect.width - soffset;
		trect.height = soffset;
	}

	dc->fillrect(trect);
	if (soffset < 0)
	{
		trect.y = drect.y - soffset;
		trect.width = -soffset;
		trect.height = drect.height + (soffset << 1);
	}
	else
	{
		trect.x = drect.x + drect.width - soffset;
		trect.y = drect.y + soffset;
		trect.width = soffset;
		trect.height = drect.height - soffset;
	}
	dc->fillrect(trect);
}

static inline void gen_3d_top_points(MCPoint p_points[6], int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom, uint32_t p_width)
{
	p_points[0].x = p_left;
	p_points[0].y = p_top;
	p_points[1].x = p_right;
	p_points[1].y = p_top;
	p_points[2].x = p_right;
	p_points[2].y = p_top + p_width;
	p_points[3].x = p_left + p_width;
	p_points[3].y = p_top + p_width;
	p_points[4].x = p_left + p_width;
	p_points[4].y = p_bottom;
	p_points[5].x = p_left;
	p_points[5].y = p_bottom;
}

static inline void gen_3d_bottom_points(MCPoint p_points[6], int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom, uint32_t p_width)
{
	p_points[0].x = p_right;
	p_points[0].y = p_bottom;
	p_points[1].x = p_left;
	p_points[1].y = p_bottom;
	p_points[2].x = p_left + p_width;
	p_points[2].y = p_bottom - p_width;
	p_points[3].x = p_right - p_width;
	p_points[3].y = p_bottom - p_width;
	p_points[4].x = p_right - p_width;
	p_points[4].y = p_top + p_width;
	p_points[5].x = p_right;
	p_points[5].y = p_top;
}

// IM-2013-09-06: [[ RefactorGraphics ]] Render all emulated theme 3D borders as polygons
void MCObject::draw3d(MCDC *dc, const MCRectangle &drect,
                      Etch style, uint2 bwidth)
{
	// MW-2013-10-29: [[ Bug 11324 ]] If the border width is zero, then don't render.
	if (bwidth == 0)
		return;
	
	bwidth = MCU_min(bwidth, drect.height >> 1);
	if (bwidth == 0)
		return;
	MCSegment tb[DEFAULT_BORDER * 2];
	MCSegment bb[DEFAULT_BORDER * 2];
	MCSegment *t = tb;
	MCSegment *b = bb;
	if (bwidth > DEFAULT_BORDER)
	{
		t = new MCSegment[bwidth * 2];
		b = new MCSegment[bwidth * 2];
	}
	int2 lx = drect.x;
	int2 rx = drect.x + drect.width;
	int2 ty = drect.y;
	int2 by = drect.y + drect.height;
	uint2 i;

	Boolean reversed = style == ETCH_SUNKEN || style == ETCH_SUNKEN_BUTTON;
	switch (MClook)
	{
	case LF_AM:
	case LF_MAC:
	case LF_WIN95:
		if (bwidth == DEFAULT_BORDER)
		{
			MCPoint t_points[6];

			// MM-2013-11-26: [[ Bug 11523 ]] Tweak the positioning of top points.
			gen_3d_top_points(t_points, lx + 1, ty + 1, rx, by, 1);
			if (style == ETCH_RAISED_SMALL || style == ETCH_SUNKEN_BUTTON)
				if (reversed)
					dc->setforeground(dc->getblack());
				else
					if (flags & F_OPAQUE)
						dc->setforeground(dc->getgray());
					else
						setforeground(dc, DI_BACK, False);
			else
				setforeground(dc, DI_TOP, reversed);
			dc->fillpolygon(t_points, 6);

			gen_3d_top_points(t_points, lx + 2, ty + 2, rx - 1, by - 1, 1);
			if (style == ETCH_RAISED_SMALL || style == ETCH_SUNKEN_BUTTON)
				setforeground(dc, DI_TOP, reversed);
			else
				if (reversed)
					dc->setforeground(dc->getblack());
				else
					setforeground(dc, DI_BACK, False);
			dc->fillpolygon(t_points, 6);

			gen_3d_bottom_points(t_points, lx + 1, ty + 1, rx - 1, by - 1, 1);
			if (MClook != LF_MAC && MClook != LF_AM || style != ETCH_SUNKEN)
				if (reversed)
					if (gettype() == CT_FIELD)
						parent->setforeground(dc, DI_BACK, False);
					else
						setforeground(dc, DI_BACK, False);
				else
					setforeground(dc, DI_BOTTOM, False);
			dc->fillpolygon(t_points, 6);

			gen_3d_bottom_points(t_points, lx, ty, rx, by, 1);
			if (reversed)
				setforeground(dc, DI_TOP, False);
			else
				dc->setforeground(dc->getblack());
			dc->fillpolygon(t_points, 6);
			break;
		}
	case LF_MOTIF:
		// IM-2013-09-03: [[ RefactorGraphics ]] render Motif 3D border using polygons instead of line segments
		MCPoint t_points[6];

		gen_3d_top_points(t_points, lx, ty, rx, by, bwidth);
		setforeground(dc, DI_TOP, reversed);
		dc->fillpolygon(t_points, 6);

		gen_3d_bottom_points(t_points, lx, ty, rx, by, bwidth);
		setforeground(dc, DI_BOTTOM, reversed);
		dc->fillpolygon(t_points, 6);
		break;
	}
	if (t != tb)
	{
		delete t;
		delete b;
	}
}

void MCObject::drawborder(MCDC *dc, const MCRectangle &drect, uint2 bwidth)
{
	// MW-2013-10-29: [[ Bug 11324 ]] If the border width is zero, then don't render.
	if (bwidth == 0)
		return;
	
	// MM-2013-09-30: [[ Bug 11241 ]] Make sure we set the foreground color of the dc before drawing.
	setforeground(dc, DI_BORDER, False);
	
	// IM-2013-09-06: [[ RefactorGraphics ]] rewrite to use drawrect with inside line width
	uint2 t_linesize, t_linestyle, t_capstyle, t_joinstyle;
	real8 t_miter_limit;

	dc->getlineatts(t_linesize, t_linestyle, t_capstyle, t_joinstyle);
	dc->getmiterlimit(t_miter_limit);

	dc->setlineatts(bwidth, t_linestyle, t_capstyle, JoinMiter);
	dc->setmiterlimit(2.0);

	dc->drawrect(drect, true);

	dc->setlineatts(t_linesize, t_linestyle, t_capstyle, t_joinstyle);
	dc->setmiterlimit(t_miter_limit);
}

void MCObject::positionrel(const MCRectangle &drect,
                           Object_pos xpos, Object_pos ypos)
{
	int2 x, y;
	uint2 width, height;

	x = drect.x;
	y = drect.y;
	width = drect.width;
	height = drect.height;

	switch (xpos)
	{
	case OP_NONE:
		break;
	case OP_LEFT:
		rect.x = x - rect.width;
		break;
	case OP_ALIGN_LEFT:
		rect.x = x;
		break;
	case OP_CENTER:
		rect.x = x - ((rect.width - width) >> 1);
		break;
	case OP_ALIGN_RIGHT:
		rect.x = x + width - rect.width;
		break;
	case OP_RIGHT:
		rect.x = x + width;
		break;
	default:
		break;
	}
	switch (ypos)
	{
	case OP_NONE:
		break;
	case OP_TOP:
		rect.y = y - rect.height;
		break;
	case OP_ALIGN_TOP:
		rect.y = y;
		break;
	case OP_MIDDLE:
		rect.y = y - ((rect.height - height) >> 1);
		break;
	case OP_ALIGN_BOTTOM:
		rect.y = y + height - rect.height;
		break;
	case OP_BOTTOM:
		rect.y = y + height;
		break;
	default:
		break;
	}
}

Exec_stat MCObject::domess(const char *sptr)
{
	const char *temp = "on message\n%s\nend message\n";
	char *tscript = new char[strlen(temp) + strlen(sptr) - 1];
	sprintf(tscript, temp, sptr);
	MCHandlerlist *handlist = new MCHandlerlist;
	// SMR 1947, suppress parsing errors
	MCerrorlock++;
	if (handlist->parse(this, tscript) != PS_NORMAL)
	{
		MCerrorlock--;
		delete handlist;
		delete tscript;
		return ES_ERROR;
	}
	MCerrorlock--;
	MCObject *oldtargetptr = MCtargetptr;
	MCtargetptr = this;
	MCHandler *hptr;
	handlist->findhandler(HT_MESSAGE, MCM_message, hptr);
	MCExecPoint ep(this, handlist, hptr);
	Boolean oldlock = MClockerrors;
	MClockerrors = True;
	Exec_stat stat = hptr->exec(ep, NULL);
	MClockerrors = oldlock;
	delete handlist;
	delete tscript;
	MCtargetptr = oldtargetptr;
	if (stat == ES_NORMAL)
		return ES_NORMAL;
	else
	{
		MCeerror->clear(); // clear out bogus error messages
		return ES_ERROR;
	}
}

Exec_stat MCObject::eval(const char *sptr, MCExecPoint &ep)
{
	const char *temp = "on eval\nreturn %s\nend eval\n";
	char *tscript = new char[strlen(temp) + strlen(sptr) - 1];
	sprintf(tscript, temp, sptr);
	MCHandlerlist *handlist = new MCHandlerlist;
	if (handlist->parse(this, tscript) != PS_NORMAL)
	{
		ep.setstaticcstring("Error parsing expression\n");
		delete handlist;
		delete tscript;
		return ES_ERROR;
	}
	MCObject *oldtargetptr = MCtargetptr;
	MCtargetptr = this;
	MCHandler *hptr;
	MCHandler *oldhandler = ep.gethandler();
	MCHandlerlist *oldhlist = ep.gethlist();
	handlist->findhandler(HT_MESSAGE, MCM_eval, hptr);
	ep.sethlist(handlist);
	ep.sethandler(hptr);
	Boolean oldlock = MClockerrors;
	MClockerrors = True;
	Exec_stat stat;
	if (hptr->exec(ep, NULL) != ES_NORMAL)
	{
		ep.setstaticcstring("Error parsing expression\n");
		stat = ES_ERROR;
	}
	else
	{
		MCresult->fetch(ep);
		stat = ES_NORMAL;
	}
	MClockerrors = oldlock;
	MCtargetptr = oldtargetptr;
	ep.sethlist(oldhlist);
	ep.sethandler(oldhandler);
	delete tscript;
	delete handlist;
	return stat;
}

// MERG 2013-9-13: [[ EditScriptChunk ]] Added at expression that's passed through as a second parameter to editScript
void MCObject::editscript(MCString p_at)
{
	MCExecPoint ep(this, NULL, NULL);
	getprop(0, P_LONG_ID, ep, False);
    if (p_at != NULL)
        getcard()->message_with_args(MCM_edit_script, ep.getsvalue(), p_at);
    else
        getcard()->message_with_args(MCM_edit_script, ep.getsvalue());
}

void MCObject::removefrom(MCObjectList *l)

{
	if (l != NULL)
	{
		MCObjectList *optr = l;
		do
		{
			if (optr->getobject() == this)
			{
				optr->setremoved(True);
				return;
			}
			optr = optr->next();
		}
		while (optr != l);
	}
}

Boolean MCObject::attachmenu(MCStack *sptr)
{
	if (attachedmenu != NULL)
		return False;
	attachedmenu = sptr;
	MCscreen->grabpointer(getw());
	MCdispatcher->addmenu(this);
	state |= CS_MENU_ATTACHED;
	menudepth++;
	MCmenuobjectptr = this;
	startx = MCmousex;
	starty = MCmousey;
	return True;
}

void MCObject::alloccolors()
{
	MCColor *syscolors = MCscreen->getaccentcolors();
	if (syscolors != NULL)
	{
		maccolors[MAC_THUMB_TOP] = syscolors[1];
		maccolors[MAC_THUMB_BACK] = syscolors[2];
		maccolors[MAC_THUMB_BOTTOM] = syscolors[3];
		maccolors[MAC_THUMB_GRIP] = syscolors[4];
		maccolors[MAC_THUMB_HILITE] = syscolors[6];
	}
	uint2 i = MAC_NCOLORS;
	while (i--)
		MCscreen->alloccolor(maccolors[i]);
}

MCImageBitmap *MCObject::snapshot(const MCRectangle *p_clip, const MCPoint *p_size, MCGFloat p_scale_factor, bool p_with_effects)
{
	Chunk_term t_type;
	t_type = gettype();

	if (t_type == CT_STACK)
		return NULL;

	MCBitmapEffectsRef t_effects;
	t_effects = nil;
	if (t_type != CT_CARD && p_with_effects)
		t_effects = static_cast<MCControl *>(this) -> getbitmapeffects();

	MCRectangle t_effective_rect;
	if (t_type == CT_CARD)
		t_effective_rect = getrect();
	else
	{
		t_effective_rect = MCU_reduce_rect(static_cast<MCControl *>(this) -> getrect(), -static_cast<MCControl *>(this) -> gettransient());
		if (t_effects != nil)
			MCBitmapEffectsComputeBounds(t_effects, t_effective_rect, t_effective_rect);
	}
	
	MCRectangle r;
	if (p_clip != nil)
		r = MCU_intersect_rect(t_effective_rect, *p_clip);
	else
		r = t_effective_rect;

	// MW-2006-02-27: If the resulting image would be of zero width or height we shouldn't do anything
	if (r . width == 0 || r . height == 0)
		return NULL;

	uint32_t t_context_width = r.width;
	uint32_t t_context_height = r.height;
	if (p_size != nil)
	{
		t_context_width = p_size->x;
		t_context_height = p_size->y;
	}

	MCImageBitmap *t_bitmap = nil;
	/* UNCHECKED */ MCImageBitmapCreate(ceil(t_context_width * p_scale_factor), ceil(t_context_height * p_scale_factor), t_bitmap);
	MCImageBitmapClear(t_bitmap);

	MCGContextRef t_gcontext = nil;
	/* UNCHECKED */ MCGContextCreateWithPixels(t_bitmap->width, t_bitmap->height, t_bitmap->stride, t_bitmap->data, true, t_gcontext);

	// IM-2013-07-24: [[ ResIndependence ]] take snapshot at specified scale, rather than device scale
	MCGContextScaleCTM(t_gcontext, p_scale_factor, p_scale_factor);
	
	MCGAffineTransform t_transform = MCGAffineTransformMakeTranslation(-r.x, -r.y);
	if (p_size != nil)
		t_transform = MCGAffineTransformScale(t_transform, p_size->x / (float)r.width, p_size->y / (float)r.height);

	MCGContextConcatCTM(t_gcontext, t_transform);
	
	// MW-2014-01-07: [[ bug 11632 ]] Use the offscreen variant of the context so its
	//   type field is appropriate for use by the player.
	MCContext *t_context = new MCOffscreenGraphicsContext(t_gcontext);
	t_context -> setclip(r);

	// MW-2011-01-29: [[ Bug 9355 ]] Make sure we only open a control if it needs it!
	// IM-2013-03-19: [[ BZ 10753 ]] Any parents of this object must also be opened to
	// safely & correctly snapshot objects with inherited patterns
	// MW-2013-03-25: [[ Bug ]] Make sure use appropriate methods to open/close the objects.
    // SN-2014-01-30: [[ Bug 11721 ]] Make sure the parentless templates are handled properly
    // as they need a temporary parent
    bool t_parent_added = false;
	MCObject *t_opened_control = nil;
	if (opened == 0)
    {
        if (parent == nil)
        {
            setparent(MCdefaultstackptr -> getcard());
            t_parent_added = true;
        }
        
		t_opened_control = this;
    }
    
	if (t_opened_control != nil)
	{
		t_opened_control -> open();
		while (t_opened_control->getparent() != nil && t_opened_control->getparent()->opened == 0)
		{
			t_opened_control = t_opened_control->getparent();
			t_opened_control -> MCObject::open();
		}
	}

	if (t_type == CT_CARD)
		((MCCard *)this) -> draw(t_context, r, true);
	else
	{
		t_context -> setopacity(blendlevel * 255 / 100);
		t_context -> setfunction(GXblendSrcOver);

        // PM-2014-11-11: [[ Bug 13970 ]] Make sure each player is buffered correctly for export snapshot
        for(MCPlayer *t_player = MCplayers; t_player != nil; t_player = t_player -> getnextplayer())
            t_player -> syncbuffering(t_context);
            
#ifdef FEATURE_PLATFORM_PLAYER
        MCPlatformWaitForEvent(0.0, true);
#endif
		if (t_effects != nil)
			t_context -> begin_with_effects(t_effects, static_cast<MCControl *>(this) -> getrect());
		// MW-2011-09-06: [[ Redraw ]] Render the control isolated, but not as a sprite.
		((MCControl *)this) -> draw(t_context, r, true, false);
		if (t_effects != nil)
			t_context -> end();
	}
	
	// MW-2013-03-25: [[ Bug ]] Make sure use appropriate methods to open/close the objects.
	if (t_opened_control != nil)
	{
        // SN-2014-01-30: [[ Bug 11721 ]] Remove the temporary added parent for the parentless object (template)
        if (t_parent_added)
            setparent(nil);
		MCObject *t_closing_control;
		t_closing_control = this;
		t_closing_control -> close();
		while(t_closing_control != t_opened_control)
		{
			t_closing_control = t_closing_control -> getparent();
			t_closing_control -> MCObject::close();
		}
	}

	delete t_context;
	MCGContextRelease(t_gcontext);
	return t_bitmap;
}

bool MCObject::isselectable(bool p_only_object) const
{
	if (p_only_object)
		return getextraflag(EF_CANT_SELECT) == False;

	const MCObject *t_object;
	t_object = this;
	do
	{
		if (t_object -> getextraflag(EF_CANT_SELECT))
			return false;
		t_object = t_object -> getparent();
	}
	while(t_object != NULL && t_object -> gettype() >= CT_BACKGROUND);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//  SAVING AND LOADING
//

IO_stat MCObject::load(IO_handle stream, const char *version)
{
	IO_stat stat;
	uint2 i;

	if ((stat = IO_read_uint4(&obj_id, stream)) != IO_NORMAL)
		return stat;

	MCNameRef t_name;
	if ((stat = IO_read_nameref(t_name, stream)) != IO_NORMAL)
		return stat;
	MCNameDelete(_name);
	_name = t_name;

	if ((stat = IO_read_uint4(&flags, stream)) != IO_NORMAL)
		return stat;

	// MW-2012-02-19: [[ SplitTextAttrs ]] If we have a font flag, then it means
	//   we must start off the font flags with all attrs set - this might be
	//   overridden in extended data though.
	if (getflag(F_FONT))
		m_font_flags |= FF_HAS_ALL_FATTR;

	// MW-2012-02-17: [[ IntrinsicUnicode ]] If we have a font, then the unicode-tag
	//  flag comes from there. Otherwise we take the parent's object (if any).
	// MW-2012-02-19: [[ SplitTextAttrs ]] If there is a font flag, then we must
	//   read in the font record. We record the font index at the moment and
	//   process everything at the end.
	bool t_has_font_index;
	uint2 t_font_index;
	t_has_font_index = false;
	if (flags & F_FONT)
	{
		if (strncmp(version, "1.3", 3) > 0)
		{
			if ((stat = IO_read_uint2(&t_font_index, stream)) != IO_NORMAL)
				return stat;
			if ((stat = IO_read_uint2(&fontheight, stream)) != IO_NORMAL)
				return stat;

			// MW-2012-02-19: [[ SplitTextAttrs ]] We have a font index for processing
			//   later on.
			t_has_font_index = true;
		}
		else
		{
			char *fontname;
			uint2 fontsize, fontstyle;
			if ((stat = IO_read_string(fontname, stream)) != IO_NORMAL)
				return stat;
			if ((stat = IO_read_uint2(&fontheight, stream)) != IO_NORMAL)
				return stat;
			if ((stat = IO_read_uint2(&fontsize, stream)) != IO_NORMAL)
				return stat;
			if ((stat = IO_read_uint2(&fontstyle, stream)) != IO_NORMAL)
				return stat;
			setfontattrs(fontname, fontsize, fontstyle);
			delete fontname;
		}
	}
	else if (parent != nil && (parent -> m_font_flags & FF_HAS_UNICODE_TAG) != 0)
		m_font_flags |= FF_HAS_UNICODE_TAG;

	if (flags & F_SCRIPT)
	{
		if ((stat = IO_read_string(script, stream)) != IO_NORMAL)
			return stat;
        
        // SN-2014-11-07: [[ Bug 13957 ]] It's possible to get a NULL script but having the
        //  F_SCRIPT flag. Unset the flag in case it's needed
        if (script == NULL)
            flags &= ~F_SCRIPT;
		getstack() -> securescript(this);
	}

	if ((stat = IO_read_uint2(&dflags, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_uint2(&ncolors, stream)) != IO_NORMAL)
		return stat;
	if (ncolors > 0)
	{
		colors = new MCColor[ncolors];
		colornames = new char *[ncolors];
		for (i = 0 ; i < ncolors ; i++)
		{
			if ((stat = IO_read_mccolor(colors[i], stream)) != IO_NORMAL)
				break;
			if ((stat = IO_read_string(colornames[i], stream)) != IO_NORMAL)
				break;
			colors[i].pixel = i;
		}
		if (stat != IO_NORMAL)
		{
			while (i < ncolors)
				colornames[i++] = NULL;
			return stat;
		}
	}
	if ((stat = IO_read_uint2(&npatterns, stream)) != IO_NORMAL)
		return stat;
	uint2 addflags = npatterns & 0xFFF0;
	npatterns &= 0x0F;
	if (npatterns > 0)
	{
		/* UNCHECKED */ MCMemoryNewArray(npatterns, patterns);
		for (i = 0 ; i < npatterns ; i++)
			if ((stat = IO_read_uint4(&patterns[i].id, stream)) != IO_NORMAL)
				return stat;
	}
	if ((stat = IO_read_int2(&rect.x, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_int2(&rect.y, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_uint2(&rect.width, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_uint2(&rect.height, stream)) != IO_NORMAL)
		return stat;
	if (addflags & AF_CUSTOM_PROPS)
		if ((stat = loadunnamedpropset(stream)) != IO_NORMAL)
			return stat;
	if (addflags & AF_BORDER_WIDTH)
		if ((stat = IO_read_uint1(&borderwidth, stream)) != IO_NORMAL)
			return stat;
	if (addflags & AF_SHADOW_OFFSET)
		if ((stat = IO_read_int1(&shadowoffset, stream)) != IO_NORMAL)
			return stat;
	if (addflags & AF_TOOL_TIP)
	{
		// MW-2012-03-09: [[ StackFile5500 ]] If the version is 5.5 and above, then
		//   the tooltip will be encoded in UTF-8 so we must convert if file format
		//   is older.
		// MW-2012-03-13: [[ UnicodeToolTip ]] If the file format is older than 5.5
		//   then convert native to utf-8.
		if (strncmp(version, "5.5", 3) < 0)
		{
			char *t_native_tooltip;
			if ((stat = IO_read_string(t_native_tooltip, stream)) != IO_NORMAL)
				return stat;
			MCExecPoint ep;
			ep . setsvalue(t_native_tooltip);
			ep . nativetoutf8();
			tooltip = ep . getsvalue() . clone();
			delete t_native_tooltip;
		}
		else
		{
			// MW-2012-09-19: [[ Bug 10233 ]] When we read in the tooltip, make sure
			//   we don't translate it as it is encoded as UTF-8.
			if ((stat = IO_read_string_no_translate(tooltip, stream)) != IO_NORMAL)
				return stat;
		}
	}
	if (addflags & AF_ALT_ID)
		if ((stat = IO_read_uint2(&altid, stream)) != IO_NORMAL)
			return stat;
	if (addflags & AF_INK)
		if ((stat = IO_read_uint1(&ink, stream)) != IO_NORMAL)
			return stat;
	if (addflags & AF_CANT_SELECT)
		extraflags |= EF_CANT_SELECT;
	if (addflags & AF_NO_FOCUS_BORDER)
		extraflags |= EF_NO_FOCUS_BORDER;
	if (addflags & AF_EXTENDED)
	{
		uint4 t_length;
		stat = IO_read_uint4(&t_length, stream);
		if (stat == IO_NORMAL)
		{
			// The jiggery pokery with length here is to do with oddities surround MCX
			// encryption. We decode with an extra byte at the end, but it won't have
			// been encoded.
			// The upshot is that the inputstream is told about the full length, but
			// we pass t_length - 1 to extendedload (after adjusting for script). We
			// then verify we've read a nice NUL byte at the end.
			MCObjectInputStream *t_stream = nil;
			/* UNCHECKED */ MCStackSecurityCreateObjectInputStream(stream, t_length, t_stream);
			t_length -= 1;

			stat = t_stream -> ReadCString(script);
			if (stat == IO_NORMAL)
			{
				if (MCtranslatechars && script != NULL)
				{
#ifdef __MACROMAN__
					IO_iso_to_mac(script, strlen(script));
#else
					IO_mac_to_iso(script, strlen(script));
#endif
				}
				t_length -= script == NULL ? 1 : strlen(script) + 1;
				
				if (script != nil)
					getstack() -> securescript(this);
			}

			if (stat == IO_NORMAL && t_length > 0)
				stat = extendedload(*t_stream, version, t_length);

			// Read the implicit nul byte
			if (stat == IO_NORMAL)
			{
				uint1 t_byte;
				stat = t_stream -> ReadU8(t_byte);
				if (stat == IO_NORMAL && t_byte != 0)
					stat = IO_ERROR;
			}

			// Make sure we flush the rest of the (unknown) stream
			if (stat == IO_NORMAL)
				stat = t_stream -> Flush();
			
			delete t_stream;
		}
		
		if (stat != IO_NORMAL)
			return stat;

		if (script != NULL)
			flags |= F_SCRIPT;
	}
	else if (addflags & AF_LONG_SCRIPT)
	{
		if ((stat = IO_read_string(script, stream, 4)) != IO_NORMAL)
			return stat;
		flags |= F_SCRIPT;
		
		getstack() -> securescript(this);
	}

	if (addflags & AF_BLEND_LEVEL)
		if ((stat = IO_read_uint1(&blendlevel, stream)) != IO_NORMAL)
			return stat;

	// MW-2013-03-28: The restrictions byte is no longer relevant due to new
	//   licensing.
	if (strcmp(version, "2.7") >= 0)
	{
		uint1 t_restrictions;
		if ((stat = IO_read_uint1(&t_restrictions, stream)) != IO_NORMAL)
			return stat;
	}

	// MW-2012-02-19: [[ SplitTextAttrs ]] Now that we've read the extended props
	//   it is safe to process the font index.
	if (t_has_font_index)
	{
		// MW-2012-02-17: [[ LogFonts ]] If the object is not a stack then
		//   we can load the font attrs now (since we have a font-table).
		//   Otherwise we store the index, and the stack save method will
		//   resolve after the font table is loaded. (Note we leave F_FONT set
		//   in the stack case, so it knows to resolve later!).
		if (gettype() != CT_STACK)
		{
			flags &= ~F_FONT;
			loadfontattrs(t_font_index);
		}
		else
			s_last_font_index = t_font_index;
	}
	else
		flags &= ~F_FONT;

	return IO_NORMAL;
}

IO_stat MCObject::save(IO_handle stream, uint4 p_part, bool p_force_ext)
{
	IO_stat stat;
	uint2 i;
	bool t_extended;
	t_extended = MCstackfileversion >= 2700 && p_force_ext;

	// Check whether there are any custom properties with array values and if so, force extension
	if (hasarraypropsets())
		t_extended = true;

	// MW-2008-10-28: [[ ParentScripts ]] Make sure we mark this as extended if there
	//   is a non-NULL parent_script
	if (parent_script != NULL)
		t_extended = true;

	// MW-2009-08-24: [[ Bitmap Effects ]] If we are a control and we have bitmap effects
	//   then we are extended.
	// MW-2011-11-24: [[ LayerMode Save ]] If we are a control and have non-static layermode
	//   then we are extended.
	if (gettype() >= CT_GROUP)
		if (static_cast<MCControl *>(this) -> getbitmapeffects() != NULL ||
			static_cast<MCControl *>(this) -> layer_getmodehint() != kMCLayerModeHintStatic)
			t_extended = true;

	// MW-2012-02-19: [[ SplitTextAttrs ]] If we need font flags, we need to be extended.
	if (needtosavefontflags())
		t_extended = true;

	// MW-2012-02-19: [[ SplitTextAttrs ]] Work out whether we need a font record.
	bool t_need_font;
	t_need_font = needtosavefontrecord();

	uint4 t_written_id;
	if (p_part != 0)
		t_written_id = 0;
	else
		t_written_id = obj_id;

	// If p_part != 0 it means we are saving a card or one specific control on a card.
	// In this case, we write the object id as 0, since it will be re-assigned when
	// reconstructed.
	//
	if ((stat = IO_write_uint4(t_written_id, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_nameref(_name, stream)) != IO_NORMAL)
		return stat;

	uint32_t t_old_flags;
	t_old_flags = flags;

	// MW-2012-02-19: [[ SplitTextAttrs ]] If we need a font record, then set the flag.
	if (t_need_font)
		flags |= F_FONT;

	uint2 addflags = npatterns;
	if (t_extended)
		addflags |= AF_EXTENDED;
	if (flags & F_SCRIPT && strlen(script) >= MAXUINT2 || t_extended)
	{
		addflags |= AF_LONG_SCRIPT;
		flags &= ~F_SCRIPT;
	}
	stat = IO_write_uint4(flags, stream);
	if (addflags & AF_LONG_SCRIPT && script != NULL)
		flags |= F_SCRIPT;

	flags = t_old_flags;

	if (stat != IO_NORMAL)
		return stat;

	// MW-2012-02-19: [[ SplitTextAttrs ]] Serialize a font record if we need to.
	if (t_need_font)
	{
		// MW-2012-02-17: [[ LogFonts ]] Delegate to 'savefontattrs()' to compute which
		//   fontindex the object should use.
		if ((stat = IO_write_uint2(savefontattrs(), stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint2(fontheight, stream)) != IO_NORMAL)
			return stat;
	}
	if (flags & F_SCRIPT && !(addflags & AF_LONG_SCRIPT))
	{
		getstack() -> unsecurescript(this);
		stat = IO_write_string(script, stream);
		getstack() -> securescript(this);
		if (stat != IO_NORMAL)
			return stat;
	}
	if ((stat = IO_write_uint2(dflags, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(ncolors, stream)) != IO_NORMAL)
		return stat;
	for (i = 0 ; i < ncolors ; i++)
		if ((stat = IO_write_mccolor(colors[i], stream)) != IO_NORMAL
		        || (stat = IO_write_string(colornames[i], stream)) != IO_NORMAL)
			return stat;
	if (props != NULL)
		addflags |= AF_CUSTOM_PROPS;
	if (borderwidth != DEFAULT_BORDER)
		addflags |= AF_BORDER_WIDTH;
	if (shadowoffset != DEFAULT_SHADOW)
		addflags |= AF_SHADOW_OFFSET;
	if (tooltip != NULL)
		addflags |= AF_TOOL_TIP;
	if (altid != 0)
		addflags |= AF_ALT_ID;
	if (ink != GXcopy)
		addflags |= AF_INK;

//---- New in 2.7
	if (MCstackfileversion >= 2700)
	{
		if (blendlevel != 100)
			addflags |= AF_BLEND_LEVEL;
	}
//----

	if (extraflags & EF_CANT_SELECT)
		addflags |= AF_CANT_SELECT;
	if (extraflags & EF_LINK_COLORS)
		addflags |= AF_LINK_COLORS;
	if (extraflags & EF_NO_FOCUS_BORDER)
		addflags |= AF_NO_FOCUS_BORDER;

	if ((stat = IO_write_uint2(addflags, stream)) != IO_NORMAL)
		return stat;
	for (i = 0 ; i < npatterns ; i++)
		if ((stat = IO_write_uint4(patterns[i].id, stream)) != IO_NORMAL)
			return stat;
	// MW-2012-02-22; [[ NoScrollSave ]] Adjust the rect by the current group offset.
	if ((stat = IO_write_int2(rect.x + MCgroupedobjectoffset . x, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_int2(rect.y + MCgroupedobjectoffset . y, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(rect.width, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(rect.height, stream)) != IO_NORMAL)
		return stat;
	if (addflags & AF_CUSTOM_PROPS)
		if ((stat = saveunnamedpropset(stream)) != IO_NORMAL)
			return stat;
	if (addflags & AF_BORDER_WIDTH)
		if ((stat = IO_write_uint1(borderwidth, stream)) != IO_NORMAL)
			return stat;
	if (addflags & AF_SHADOW_OFFSET)
		if ((stat = IO_write_int1(shadowoffset, stream)) != IO_NORMAL)
			return stat;
	if (addflags & AF_TOOL_TIP)
	{
		// MW-2012-03-09: [[ StackFile5500 ]] If the version is 5.5 and above, then
		//   the tooltip will be encoded in UTF-8 so we must convert for earlier
		//   versions.
		// MW-2012-03-13: [[ UnicodeToolTip ]] If the file format is older than 5.5
		//   then convert utf-8 to native before saving.
		if (MCstackfileversion < 5500)
		{
			MCExecPoint ep;
			char *t_native_tooltip;
			ep . setsvalue(tooltip);
			ep . utf8tonative();
			t_native_tooltip = ep . getsvalue() . clone();
			if ((stat = IO_write_string(t_native_tooltip, stream)) != IO_NORMAL)
			{
				delete t_native_tooltip;
				return stat;
			}
			delete t_native_tooltip;
		}
		else
		{
			if ((stat = IO_write_string(tooltip, stream)) != IO_NORMAL)
				return stat;
		}
	}
	if (addflags & AF_ALT_ID)
		if ((stat = IO_write_uint2(altid, stream)) != IO_NORMAL)
			return stat;

//---- New in 2.7
	uint1 t_converted_ink;
	if (MCstackfileversion >= 2700)
		t_converted_ink = ink;
	else
		t_converted_ink = ink >= 0x19 ? GXcopy : ink;
//----

	if (addflags & AF_INK)
		if ((stat = IO_write_uint1(t_converted_ink, stream)) != IO_NORMAL)
			return stat;

	if (t_extended)
	{
		uint4 t_length_offset;
		if (MCS_isfake(stream))
			t_length_offset = MCS_faketell(stream);
		else
			t_length_offset = (uint4)MCS_tell(stream);

		stat = IO_write_uint4(t_length_offset, stream);

		if (stat == IO_NORMAL)
		{
			MCObjectOutputStream *t_stream = nil;
			/* UNCHECKED */ MCStackSecurityCreateObjectOutputStream(stream, t_stream);
			getstack() -> unsecurescript(this);
			stat = t_stream -> WriteCString(script);
			getstack() -> securescript(this);
			if (stat == IO_NORMAL)
				stat = extendedsave(*t_stream, p_part);
			if (stat == IO_NORMAL)
				stat = t_stream -> Flush(true);
			
			delete t_stream;
		}
		if (stat == IO_NORMAL)
			stat = IO_write_uint1(0, stream);
		if (stat == IO_NORMAL)
		{
			uint4 t_cur_offset;
			if (MCS_isfake(stream))
				t_cur_offset = MCS_faketell(stream);
			else
				t_cur_offset = (uint4)MCS_tell(stream);

			uint4 t_length;
			t_length = MCSwapInt32HostToNetwork(t_cur_offset - t_length_offset - 4);

			if (MCS_isfake(stream))
				MCS_fakewriteat(stream, t_length_offset, &t_length, sizeof(uint4));
			else
			{
				stat = MCS_seek_set(stream, t_length_offset);
				if (stat == IO_NORMAL)
					stat = MCS_write(&t_length, sizeof(uint4), 1, stream);
				if (stat == IO_NORMAL)
					stat = MCS_seek_set(stream, t_cur_offset);
			}
		}
		if (stat != IO_NORMAL)
			return stat;
	}
	else if (addflags & AF_LONG_SCRIPT)
	{
		getstack() -> unsecurescript(this);
		stat = IO_write_string(script, stream, 4);
		getstack() -> securescript(this);
		if (stat != IO_NORMAL)
			return stat;
	}

//---- New in 2.7
	if (MCstackfileversion >= 2700)
	{
		if (addflags & AF_BLEND_LEVEL)
			if ((stat = IO_write_uint1(blendlevel, stream)) != IO_NORMAL)
				return stat;

		// Write out the restrictions byte as if it were (old-style) Enterprise.
		// This ensures the stackfile can still be opened in older versions.
		uint1 t_restrictions;
		t_restrictions = 6;
		t_restrictions |= (~t_restrictions) << 4;

		// Mix the restrictions byte into the form expected by older engines (otherwise
		// they will fail to load).
		uint4 x;
		x = t_restrictions | (t_restrictions << 8);
		x |= x << 16;
		x ^= (t_written_id | ((65535 - t_written_id) << 16)) ^ flags;
		t_restrictions = ((x >> 24) & 0x88) | (x & 0x60) | ((x & 0x1100) >> 8) | ((x & 0x060000) >> 16);
	
		if ((stat = IO_write_uint1(t_restrictions, stream)) != IO_NORMAL)
			return stat;
	}
//----

	return IO_NORMAL;
}

IO_stat MCObject::defaultextendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_remaining)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	if (p_remaining > 0)
	{
		uint4 t_flags, t_length, t_header_size;
		t_stat = p_stream . ReadTag(t_flags, t_length, t_header_size);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . Mark();
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . Skip(t_length);
		if (t_stat == IO_NORMAL)
			p_remaining -= t_length + t_header_size;
	}

	if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedload(p_stream, p_version, p_remaining);

	return t_stat;
}

IO_stat MCObject::defaultextendedsave(MCObjectOutputStream& p_stream, uint4 p_part)
{
	IO_stat t_stat;
	t_stat = p_stream . WriteTag(0, 0);
	if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedsave(p_stream, p_part);

	return t_stat;
}

IO_stat MCObject::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part)
{
	// First calculate the size of the array custom property data
	uint32_t t_prop_size;
	t_prop_size = measurearraypropsets();

	// Calculate the tag to write out
	uint32_t t_flags;
	t_flags = 0;

	uint32_t t_size;
	t_size = 0;

	if (t_prop_size != 0)
	{
		t_flags |= OBJECT_EXTRA_ARRAYPROPS;

		// We append an additional '0' to the end of the list of props for termination.
		t_size += t_prop_size + 4;
	}

	if (parent_script != NULL)
	{
		t_flags |= OBJECT_EXTRA_PARENTSCRIPT;
		
		// Parent scripts are written out as:
		//   uint8 count
		//   uint8 index
		//   if (index & (1 << 7))
		//     uint32 id
		//     cstring stack
		//     cstring mainstack
		t_size += 1 + 1 + 4 + strlen(MCNameGetCString(parent_script -> GetParent() -> GetObjectStack())) + 1;
		t_size += 1; // was mainstack reference
	}

	// MW-2009-09-24: Slight oversight on my part means that there is no record
	//   in place for 'control' only fields. There have never been any of these
	//   before until the addition of bitmap effects. Therefore we write them out
	//   in the object record.
	MCBitmapEffects *t_bitmap_effects;
	t_bitmap_effects = gettype() >= CT_GROUP ? static_cast<MCControl *>(this) -> getbitmapeffects() : NULL;
	if (t_bitmap_effects != NULL)
	{
		t_flags |= OBJECT_EXTRA_BITMAPEFFECTS;
		t_size += MCBitmapEffectsWeigh(t_bitmap_effects);
	}

	// MW-2011-11-24: [[ LayerMode Save ]] If we are a control, and have a layerMode that
	//   is not static, we need an extra byte.
	if (gettype() >= CT_GROUP && static_cast<MCControl *>(this) -> layer_getmodehint() != kMCLayerModeHintStatic)
	{
		t_flags |= OBJECT_EXTRA_LAYERMODE;
		t_size += 1;
	}

	// MW-2012-02-19: [[ SplitTextAttrs ]] If we need to save the font flags, then make
	//   sure we include it in the tag flags.
	if (needtosavefontflags())
	{
		t_flags |= OBJECT_EXTRA_FONTFLAGS;
		t_size += 1;
	}

	// If the tag is of zero length, write nothing.
	if (t_size == 0)
		return IO_NORMAL;

	// Otherwise write out stuff
	IO_stat t_stat;
	t_stat = p_stream . WriteTag(t_flags, t_size);
	
	if (t_stat == IO_NORMAL && (t_flags & OBJECT_EXTRA_ARRAYPROPS) != 0)
		t_stat = savearraypropsets(p_stream);
	
	if (t_stat == IO_NORMAL && (t_flags & OBJECT_EXTRA_PARENTSCRIPT) != 0)
	{
		t_stat = p_stream . WriteU8(1);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . WriteU8(128 + 0);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . WriteU32(parent_script -> GetParent() -> GetObjectId());
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . WriteNameRef(parent_script -> GetParent() -> GetObjectStack());
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . WriteCString(nil); // was mainstack reference
	}

	if (t_stat == IO_NORMAL && (t_flags & OBJECT_EXTRA_BITMAPEFFECTS) != 0)
		t_stat = MCBitmapEffectsPickle(t_bitmap_effects, p_stream);

	// MW-2011-11-24: [[ LayerMode Save ]] If we are a control, with non-static layerMode then
	//   write out the mode as a byte.
	if (t_stat == IO_NORMAL && (t_flags & OBJECT_EXTRA_LAYERMODE) != 0)
		t_stat = p_stream . WriteU8(static_cast<MCControl *>(this) -> layer_getmodehint());

	// MW-2012-02-19: [[ SplitTextAttrs ]] If we have partial font settings, or are unicode with
	//   no font settings we need to write out font flags.
	if (t_stat == IO_NORMAL && (t_flags & OBJECT_EXTRA_FONTFLAGS) != 0)
	{
		// Write out the three persistent font flags.
		t_stat = p_stream . WriteU8(m_font_flags & (FF_HAS_ALL_FATTR));
	}

	return IO_NORMAL;
}

IO_stat MCObject::extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length)
{
	if (p_length == 0)
		return IO_NORMAL;

	IO_stat t_stat;

	uint32_t t_flags, t_length, t_header_length;
	t_stat = p_stream . ReadTag(t_flags, t_length, t_header_length);

	if (t_stat == IO_NORMAL)
		t_stat = p_stream . Mark();

	if (t_stat == IO_NORMAL && (t_flags & OBJECT_EXTRA_ARRAYPROPS) != 0)
		t_stat = loadarraypropsets(p_stream);

	if (t_stat == IO_NORMAL && (t_flags & OBJECT_EXTRA_PARENTSCRIPT) != 0)
	{
		uint8_t t_count;
		t_stat = p_stream . ReadU8(t_count);

		uint8_t t_index;
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . ReadU8(t_index);

		uint32_t t_id;
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . ReadU32(t_id);

		MCNameRef t_stack;
		t_stack = NULL;
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . ReadNameRef(t_stack);

		// This is no longer used, but might remain in older stackfiles.
		char *t_mainstack;
		t_mainstack = NULL;
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . ReadCString(t_mainstack);

		if (t_stat == IO_NORMAL)
		{
			parent_script = MCParentScript::Acquire(this, t_id, t_stack);
			if (parent_script == NULL)
				t_stat = IO_ERROR;

			s_loaded_parent_script_reference = true;
		}

		MCNameDelete(t_stack);
		delete t_mainstack;
	}
	
	if (t_stat == IO_NORMAL && (t_flags & OBJECT_EXTRA_BITMAPEFFECTS) != 0)
	{
		MCBitmapEffectsRef t_effects;
		t_effects = NULL;
		t_stat = MCBitmapEffectsUnpickle(t_effects, p_stream);
		if (t_stat == IO_NORMAL)
			static_cast<MCControl *>(this) -> setbitmapeffects(t_effects);
	}
	
	// MW-2011-11-24: [[ LayerMode Save ]] If a layerMode byte is present then read it
	//   in.
	if (t_stat == IO_NORMAL && (t_flags & OBJECT_EXTRA_LAYERMODE) != 0)
	{
		uint8_t t_mode_hint;
		t_stat = p_stream . ReadU8(t_mode_hint);
		if (t_stat == IO_NORMAL)
			static_cast<MCControl *>(this) -> layer_setmodehint((MCLayerModeHint)t_mode_hint);
	}

	// MW-2012-02-19: [[ SplitTextAttrs ]] If a font-flag byte is present then
	//   load and apply to m_font_flags. Notice that we clear the existing flags
	//   as they may have been set as a result of detection of a F_FONT flag, but
	//   the presence of this record supercedes that.
	if (t_stat == IO_NORMAL && (t_flags & OBJECT_EXTRA_FONTFLAGS) != 0)
	{
		uint8_t t_font_flags;
		t_stat = p_stream . ReadU8(t_font_flags);
		if (t_stat == IO_NORMAL)
			m_font_flags = (t_font_flags & ~FF_HAS_ALL_FATTR) | (t_font_flags & FF_HAS_ALL_FATTR);
	}

	if (t_stat == IO_NORMAL)
		t_stat = p_stream . Skip(t_length);

	return t_stat;
}


// MW-2008-10-28: [[ ParentScripts ]] This method attempts to resolve the
//   parentscript reference for this object (if any).
// MW-2013-05-30: [[ InheritedPscripts ]] This method returns false if there
//   was not enough memory to complete the resolution.
bool MCObject::resolveparentscript(void)
{
	// If there is no parent script, just return.
	if (parent_script == NULL)
		return true;

	// Get the underlying parent script object.
	MCParentScript *t_script;
	t_script = parent_script -> GetParent();

	// If the parent script is blocked, just return
	if (t_script -> IsBlocked())
		return true;

	// We have a parent script, so use MCdispatcher to try and find the
	// stack.
	MCStack *t_stack;
	t_stack = getstack() -> findstackname(MCNameGetOldString(t_script -> GetObjectStack()));

	// Next search for the control we need.
	MCControl *t_control;
	t_control = NULL;
	if (t_stack != NULL)
		t_control = t_stack -> getcontrolid(CT_BUTTON, t_script -> GetObjectId(), true);

	// If we found a control, resolve the parent script. Otherwise block it.
	if (t_control != NULL)
	{
		t_script -> Resolve(t_control);

		// MW-2015-05-30: [[ InheritedPscripts ]] Next we must ensure the
		//   existence of the inheritence hierarchy, so resolve the parentScript's
		//   parentScript.
		if (!t_control -> resolveparentscript())
			return false;

		// MW-2015-05-30: [[ InheritedPscripts ]] And then make sure it creates its
		//   super-use chain.
		if (!parent_script -> Inherit())
			return false;
	}
	else
		t_script -> Block();

	return true;
}

// MW-2009-02-02: [[ Improved image search ]]
// This method implements the new image search order for objects, taking into account
// the behavior hierarchy. We first search the behavior chain's for successive ancestors
// of the object, up to and including its stack. If this fails, fall back to the original
// search.
MCImage *MCObject::resolveimage(const MCString& p_name, uint4 p_image_id)
{
	// If the name string ptr is nil, then this is an id search.
	bool t_is_id;
	t_is_id = false;
	if (p_name . getstring() == nil)
		t_is_id = true;

	MCControl *t_control;
	t_control = NULL;
	
	// Start with the behavior chain of the object itself.
	MCObject *t_target;
	t_target = this;

	// OK-2009-03-13: [[Bug 7742]] - Crash when copying text containing an imageSource character to another application
	while (t_target != NULL)
	{
		// Loop up the behavior chain of the current target.
		MCParentScript *t_behavior;
		t_behavior = t_target -> getparentscript();
		while(t_behavior != NULL)
		{
			// Fetch the behavior's resolved object - if there is none then
			// we are done.
			MCObject *t_behavior_object;
			t_behavior_object = t_behavior -> GetObject();
			if (t_behavior_object == NULL)
				break;
			
			// Fetch the behavior object's owning stack.
			MCStack *t_behavior_stack;
			t_behavior_stack = t_behavior_object -> getstack();
			
			// Search for the image id on the behavior's stack, breaking if
			// we are successful.
			if (t_is_id)
				t_control = t_behavior_stack -> getcontrolid(CT_IMAGE, p_image_id, true);
			else
				t_control = t_behavior_stack -> getcontrolname(CT_IMAGE, p_name);
			if (t_control != NULL)
				break;

			// MW-2013-05-30: [[ InheritedPscripts ]] Step to the next behavior
			//   in the chain.
			t_behavior = t_behavior_object -> getparentscript();
		}
		
		// If we found the control, break.
		if (t_control != NULL)
			break;
		
		// If the current target is a stack, we are done.
		if (t_target -> gettype() == CT_STACK)
			break;
		
		// Otherwise, iterate up the owner chain
		t_target = t_target -> parent;
	}
	
	// If we didn't find the control, then fallback to the old resolution
	// mechanism.
	if (t_control == NULL)
	{
		if (t_is_id)
			t_control = static_cast<MCControl *>(getstack() -> getobjid(CT_IMAGE, p_image_id));
		else
			t_control = static_cast<MCControl *>(getstack() -> getobjname(CT_IMAGE, p_name));
	}
	
	return static_cast<MCImage *>(t_control);
}

MCImage *MCObject::resolveimageid(uint32_t p_id)
{
	return resolveimage(MCString(nil, 0), p_id);
}

MCImage *MCObject::resolveimagename(const MCString& p_name)
{
	return resolveimage(p_name, 0);
}

MCObjectHandle *MCObject::gethandle(void)
{
	if (m_weak_handle != NULL)
	{
		m_weak_handle -> Retain();
		return m_weak_handle;
	}

	m_weak_handle = new MCObjectHandle(this);
	if (m_weak_handle == NULL)
		return NULL;

	m_weak_handle -> Retain();

	return m_weak_handle;
}

///////////////////////////////////////////////////////////////////////////////

struct MCObjectChangeIdVisitor: public MCObjectVisitor
{
	uint32_t old_card_id;
	uint32_t new_card_id;

	void Process(MCCdata *p_cdata)
	{
		if (p_cdata == nil)
			return;

		MCCdata *t_ptr;
		t_ptr = p_cdata;
		do
		{
			if (t_ptr -> getid() == old_card_id)
			{
				t_ptr -> setid(new_card_id);
				return;
			}

			t_ptr = t_ptr -> next();
		}
		while(t_ptr != p_cdata);
	}

	bool OnField(MCField *p_field)
	{
		Process(p_field -> getcdata());
		return true;
	}

	bool OnButton(MCButton *p_button)
	{
		Process(p_button -> getcdata());
		return true;
	}
};

Exec_stat MCObject::changeid(uint32_t p_new_id)
{
	if (obj_id == p_new_id)
		return ES_NORMAL;

	// MW-2010-05-18: (Silently) don't allow id == 0 - this prevents people working around the
	//   script limits, which don't come into effect on objects with 0 id.
	if (p_new_id == 0)
		return ES_NORMAL;
	
	// MW-2011-02-08: Don't allow id change if the parent is nil as this means its a template
	//   object which doesn't really have an id.
	if (parent == nil)
		return ES_NORMAL;
	
	MCStack *t_stack;
	t_stack = getstack();

	if (t_stack -> isediting())
	{
		MCeerror -> add(EE_OBJECT_NOTWHILEEDITING, 0, 0);
		return ES_ERROR;
	}

	// If the stack's id is less than the requested id then we are fine
	// since the stack id is always greater or equal to the highest numbered
	// control/card id. Otherwise, check the whole list of controls and cards.
	if (p_new_id <= t_stack -> getid())
	{
		if (t_stack -> getcontrolid(CT_LAYER, p_new_id) != NULL ||
			t_stack -> findcardbyid(p_new_id) != NULL)
		{
			MCeerror->add(EE_OBJECT_IDINUSE, 0, 0, p_new_id);
			return ES_ERROR;
		}
	}
	else
		t_stack -> obj_id = p_new_id;

	// If the object is a card, we have to reset all the control's data
	// id's.
	// If the object is not a card, but has a card as parent, we need to
	// reset the card's objptr id for it.
	if (gettype() == CT_CARD)
	{
		MCObjectChangeIdVisitor t_visitor;
		t_visitor . old_card_id = obj_id;
		t_visitor . new_card_id = p_new_id;
		t_stack -> visit(VISIT_STYLE_DEPTH_FIRST, 0, &t_visitor);
	}
	else if (parent -> gettype() == CT_CARD)
		static_cast<MCCard *>(parent) -> resetid(obj_id, p_new_id);

	// MW-2012-10-10: [[ IdCache ]] If the object is in the cache, then remove
	//   it since its id is changing.
	if (m_in_id_cache)
		t_stack -> uncacheobjectbyid(this);

	uint4 oldid = obj_id;
	obj_id = p_new_id;
	message_with_args(MCM_id_changed, oldid, obj_id);

	return ES_NORMAL;
}

///////////////////////////////////////////////////////////////////////////////

// IM-2013-10-17: [[ FullscreenMode ]] Removed struct fields not related to image masks
struct object_mask_info
{
	// IM-2013-10-17: [[ FullscreenMode ]] The mask image bitmap
	MCImageBitmap *image;

	// MM-2012-10-03: [[ ResIndependence ]] The scale of the mask.
	MCGFloat scale;

	// IM-2013-10-17: [[ FullscreenMode ]] top-left corner of the mask image in stack coords
	MCPoint origin;
	
	// This is freed after processing.
	MCImageBitmap *temp_bitmap;
};

// This method computes as small an non-transparent rect as it can for the
// given shape.
static MCRectangle compute_objectshape_rect(MCObjectShape& p_shape)
{
	if (p_shape . type == kMCObjectShapeEmpty)
		return MCU_make_rect(0, 0, 0, 0);
	
	if (p_shape . type == kMCObjectShapeRectangle)
		return MCU_intersect_rect(p_shape . bounds, p_shape . rectangle);
	
	if (p_shape . type == kMCObjectShapeMask)
	{
		MCImageBitmap *t_mask;
		t_mask = p_shape . mask . bits;
		
		// IM-2013-10-17: [[ ResIndependence ]] Apply image scale factor when computing rect
		MCGFloat t_scale;
		t_scale = p_shape . mask . scale == 0.0 ? 1.0 : p_shape . mask . scale;
		
		MCRectangle t_mask_rect;
		t_mask_rect = MCRectangleMake(p_shape . mask . origin . x, p_shape . mask . origin . y, ceilf(p_shape . mask . bits -> width / t_scale), ceilf(p_shape . mask . bits -> height) / t_scale);
		
		return MCU_intersect_rect(p_shape . bounds, t_mask_rect);
	}

	// Must be complex.
	return p_shape . bounds;
}

// This method computes the mask details for a given shape, rasterizing the object
// if necessary in the process.
static void compute_objectshape_mask(MCObject *p_object, const MCObjectShape& p_shape, const MCRectangle& p_rect, uint32_t p_threshold, object_mask_info& r_mask)
{
	// Make sure everything is 0.
	memset(&r_mask, 0, sizeof(r_mask));
	
	// IM-2013-10-17: [[ FullscreenMode ]] Only compute the mask for images & complex shapes
	MCAssert(p_shape . type != kMCObjectShapeRectangle);
	
	if (p_shape . type == kMCObjectShapeMask)
	{
		// IM-2013-10-17: [[ FullscreenMode ]] Simplified mask info
		r_mask . scale = p_shape . mask . scale == 0.0 ? 1.0 : p_shape . mask . scale;
		r_mask . image = p_shape . mask . bits;
		r_mask . origin = p_shape . mask . origin;
		
		return;
	}
	
	// We should only ever get here if the object is a control!
	assert(p_object -> gettype() >= CT_GROUP);
	
	// Otherwise we are in the complex case and must rasterize and extract a
	// temporary mask.
	
	// IM-2013-10-17: [[ FullscreenMode ]] Make sure our rect is clipped to the object rect
	MCRectangle t_rect;
	t_rect = MCU_intersect_rect(p_rect, p_object->getrect());
	
	MCImageBitmap *t_snapshot = nil;
	/* UNCHECKED */ MCImageBitmapCreate(t_rect.width, t_rect.height, t_snapshot);
	MCImageBitmapClear(t_snapshot);

	MCGContextRef t_context = nil;
	/* UNCHECKED */ MCGContextCreateWithPixels(t_snapshot->width, t_snapshot->height, t_snapshot->stride, t_snapshot->data, true, t_context);
	MCGContextTranslateCTM(t_context, -(MCGFloat)t_rect.x, -(MCGFloat)t_rect.y);
	MCGContextClipToRect(t_context, MCRectangleToMCGRectangle(t_rect));

	MCContext *t_gfxcontext = nil;
	/* UNCHECKED */ t_gfxcontext = new MCGraphicsContext(t_context);
	
	// Make sure the object is opened.
	bool t_needs_open;
	t_needs_open = p_object -> getopened() == 0;
	if (t_needs_open)
		p_object -> open();
	
	// Render the object into the context (isolated).
	((MCControl *)p_object) -> draw(t_gfxcontext, t_rect, true, false);
	
	// Close the object if we opened it.
	if (t_needs_open)
		p_object -> close();
	
	delete t_gfxcontext;
	MCGContextRelease(t_context);

	// IM-2013-08-15: [[ ResIndependence ]] Use bitmap as soft mask instead of extracting sharp mask
	r_mask . temp_bitmap = t_snapshot;
	
	// Now set up the mask structure.
	// IM-2013-10-17: [[ FullscreenMode ]] Simplified mask info
	/* OVERHAUL - REVISIT: we should render at the device scale */
	r_mask . scale = 1.0;
	// IM-2013-11-22: [[ Bug 11494 ]] Set the object mask to our new snapshot image
	r_mask . image = r_mask . temp_bitmap;
	r_mask . origin = MCPointMake(t_rect.x, t_rect.y);

	return;
}

// Returns true if any pixels within the given area have opacity above the threshold level
static bool mask_intersects_with_rect(const MCRectangle &p_rect, const object_mask_info &p_mask, uint8_t p_threshold)
{
	MCRectangle t_scaled_rect;
	t_scaled_rect = MCGRectangleGetIntegerBounds(MCGRectangleScale(MCRectangleToMCGRectangle(p_rect), p_mask.scale));

	MCRectangle t_scaled_mask_rect;
	t_scaled_mask_rect = MCRectangleMake(floorf(p_mask.origin.x * p_mask.scale), floorf(p_mask.origin.y * p_mask.scale), p_mask.image->width, p_mask.image->height);
	
	MCRectangle t_rect;
	t_rect = MCU_intersect_rect(t_scaled_rect, t_scaled_mask_rect);
	
	if (t_rect.width == 0 || t_rect.height == 0)
		return false;
	
	// check for opacity in the mask over the given rect
	uint8_t *t_src_ptr;
	t_src_ptr = (uint8_t*)p_mask.image->data;
	t_src_ptr += (t_rect.y - t_scaled_mask_rect.y) * p_mask.image->stride + (t_rect.x - t_scaled_mask_rect.x) * sizeof(uint32_t);
	
	for (uint32_t y = 0; y < t_rect.height; y++)
	{
		uint32_t *t_src_row;
		t_src_row = (uint32_t*)t_src_ptr;
		
		for (uint32_t x = 0; x < t_rect.width; x++)
			if (MCGPixelGetNativeAlpha(*t_src_ptr++) > p_threshold)
				return true;
		
		t_src_ptr += p_mask.image->stride;
	}
	
	return false;
}

// Fill the 1bpp scanline buffer, using the nearest pixel to the scaled positions
static void mask_fill_scanline(const object_mask_info &p_mask, uint8_t p_threshold, uint32_t p_y, MCGFloat p_start_x, MCGFloat p_step, uint32_t p_width, uint8_t *p_scanline)
{
	uint32_t *t_src_ptr;
	t_src_ptr = p_mask.image->data;
	
	t_src_ptr += p_y * (p_mask.image->stride / sizeof(uint32_t));
	
	uint32_t t_x;
	t_x = floorf(p_start_x);

	p_start_x -= t_x;
	
	t_src_ptr += t_x;
	
	uint32_t i;
	for(i = 0; i < (p_width & ~7); i += 8)
	{
		uint8_t t_mask;
		t_mask = 0;
		
		if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 7;
		p_start_x += p_step;
		if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 6;
		p_start_x += p_step;
		if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 5;
		p_start_x += p_step;
		if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 4;
		p_start_x += p_step;
		if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 3;
		p_start_x += p_step;
		if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 2;
		p_start_x += p_step;
		if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 1;
		p_start_x += p_step;
		if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 0;
		p_start_x += p_step;

		p_scanline[i / 8] = t_mask;
	}
	
	uint32_t t_mask;
	t_mask = 0;
	switch(p_width % 8)
	{
		case 7:
			if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 1;
			p_start_x += p_step;
		case 6:
			if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 2;
			p_start_x += p_step;
		case 5:
			if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 3;
			p_start_x += p_step;
		case 4:
			if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 4;
			p_start_x += p_step;
		case 3:
			if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 5;
			p_start_x += p_step;
		case 2:
			if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 6;
			p_start_x += p_step;
		case 1:
			if (MCGPixelGetNativeAlpha(t_src_ptr[(uint32_t)floorf(p_start_x)]) >= p_threshold) t_mask |= 1 << 7;
			p_start_x += p_step;

			p_scanline[i / 8] = t_mask;
			break;
			
		default:
			break;
	}
}

bool MCObject::intersects(MCObject *p_other, uint32_t p_threshold)
{
	// If the threshold is > 255 then the masks are empty, so no intersection.
	if (p_threshold > 255)
		return false;

	// Fetch the stacks we need.
	MCStack *t_this_stack, *t_other_stack;
	t_this_stack = getstack();
	t_other_stack = p_other -> getstack();

	// If the bounds of the object's don't intersect, then we are done.
	if (MCU_empty_rect(MCU_intersect_rect(t_this_stack -> recttoroot(getrect()), t_other_stack -> recttoroot(p_other -> getrect()))))
		return false;
	
	// If the threshold is 0 then we are doing bounds checking, in which
	// case we intersect, and so are done.
	if (p_threshold == 0)
		return true;
	
	// Next compute the shape of the object. This will give us a rect, mask or
	// complex shape.
	MCObjectShape t_this_shape, t_other_shape;
	/* UNCHECKED */ lockshape(t_this_shape);
	/* UNCHECKED */ p_other -> lockshape(t_other_shape);
	
	// Compute the intersection in screen co-ords.
	// IM-2013-10-17: [[ FullscreenMode ]] Perform transformations & comparisons using MCGRectangles
	MCRectangle t_this_objectshape_rect;
	t_this_objectshape_rect = compute_objectshape_rect(t_this_shape);
	
	MCRectangle t_other_objectshape_rect;
	t_other_objectshape_rect = compute_objectshape_rect(t_other_shape);
	
	MCGRectangle t_this_root_rect;
	MCGRectangle t_other_root_rect;
	
	t_this_root_rect = MCGRectangleApplyAffineTransform(MCRectangleToMCGRectangle(t_this_objectshape_rect), t_this_stack->getroottransform());
	t_other_root_rect = MCGRectangleApplyAffineTransform(MCRectangleToMCGRectangle(t_other_objectshape_rect), t_other_stack->getroottransform());

	MCGRectangle t_root_rect;
	t_root_rect = MCGRectangleIntersection(t_this_root_rect, t_other_root_rect);
	
	bool t_intersects;
	if (MCGRectangleIsEmpty(t_root_rect))
	{
		// If the actual intersection is empty, we don't intersect.
		t_intersects = false;
	}
	else if (t_this_shape . type == kMCObjectShapeRectangle && t_other_shape . type == kMCObjectShapeRectangle)
	{
		// If both shapes are rects, then we are done (they must intersect).
		t_intersects = true;
	}
	else if (t_this_shape . type == kMCObjectShapeRectangle && t_other_shape . type != kMCObjectShapeRectangle)
	{
		// IM-2013-10-17: Add special case handling for image / rect intersection
		MCGRectangle t_other_rect;
		t_other_rect = MCGRectangleApplyAffineTransform(t_root_rect, MCGAffineTransformInvert(t_other_stack->getroottransform()));
		
		MCRectangle t_int_rect;
		t_int_rect = MCGRectangleGetIntegerInterior(t_other_rect);
		
		object_mask_info t_other_mask;
		compute_objectshape_mask(p_other, t_other_shape, t_int_rect, p_threshold, t_other_mask);
		
		t_intersects = mask_intersects_with_rect(t_int_rect, t_other_mask, p_threshold);
		
		// Free the temporary masks that were generated (if any).
		MCImageFreeBitmap(t_other_mask . temp_bitmap);
	}
	else if (t_this_shape . type != kMCObjectShapeRectangle && t_other_shape . type == kMCObjectShapeRectangle)
	{
		// IM-2013-10-17: Add special case handling for image / rect intersection
		MCGRectangle t_this_rect;
		t_this_rect = MCGRectangleApplyAffineTransform(t_root_rect, MCGAffineTransformInvert(t_this_stack->getroottransform()));
		
		MCRectangle t_int_rect;
		t_int_rect = MCGRectangleGetIntegerInterior(t_this_rect);
		
		object_mask_info t_this_mask;
		compute_objectshape_mask(p_other, t_this_shape, t_int_rect, p_threshold, t_this_mask);
		
		t_intersects = mask_intersects_with_rect(t_int_rect, t_this_mask, p_threshold);
		
		// Free the temporary masks that were generated (if any).
		MCImageFreeBitmap(t_this_mask . temp_bitmap);
	}
	else
	{
		// IM-2013-10-17: [[ FullscreenMode ]] Perform transformations & comparisons using MCGRectangles
		// Now compute the rects of interest in both the objects.
		
		// IM-2013-10-24: [[ FullscreenMode ]] Use integer rects to avoid mask buffer
		// underruns caused by floating-point rounding errors.
		MCRectangle t_other_rect;
		t_other_rect = MCGRectangleGetIntegerInterior(MCGRectangleApplyAffineTransform(t_root_rect, MCGAffineTransformInvert(t_other_stack->getroottransform())));
		
		MCRectangle t_this_rect;
		t_this_rect = MCGRectangleGetIntegerInterior(MCGRectangleApplyAffineTransform(t_root_rect, MCGAffineTransformInvert(t_this_stack->getroottransform())));
		
		// Now resolve the masks - this may result in a temporary image being
		// generated in <mask>.temp_bits - this is freed at the end.
		object_mask_info t_this_mask, t_other_mask;
		compute_objectshape_mask(this, t_this_shape, t_this_rect, p_threshold, t_this_mask);
		compute_objectshape_mask(p_other, t_other_shape, t_other_rect, p_threshold, t_other_mask);
		
		// IM-2013-10-17: [[ FullscreenMode ]] Use integer bounds when testing pixels
		MCRectangle t_int_rect;
		t_int_rect = MCGRectangleGetIntegerInterior(t_root_rect);
		
		// Now check for intersection by processing a scanline at a time.
		int32_t t_scanline_width;
		t_scanline_width = (t_int_rect . width + 7) / 8;
		
		// We accumulate the normalized mask a scanline at a time.
		uint8_t *t_this_scanline, *t_other_scanline;
		MCMemoryNewArray(t_scanline_width, t_this_scanline);
		MCMemoryNewArray(t_scanline_width, t_other_scanline);
				
		// IM-2013-10-17: [[ FullscreenMode ]] Precompute initial pixel coords & row/column increments
		MCGFloat t_this_x, t_this_y, t_this_x_inc, t_this_y_inc;
		t_this_x = t_this_mask.scale * (t_this_rect.x - t_this_mask.origin.x);
		t_this_y = t_this_mask.scale * (t_this_rect.y - t_this_mask.origin.y);
		t_this_x_inc = t_this_mask.scale * ((MCGFloat)t_this_rect.width / (MCGFloat)t_int_rect.width);
		t_this_y_inc = t_this_mask.scale * ((MCGFloat)t_this_rect.height / (MCGFloat)t_int_rect.height);
		
		// IM-2013-10-17: [[ FullscreenMode ]] Precompute initial pixel coords & row/column increments
		MCGFloat t_other_x, t_other_y, t_other_x_inc, t_other_y_inc;
		t_other_x = t_other_mask.scale * (t_other_rect.x - t_other_mask.origin.x);
		t_other_y = t_other_mask.scale * (t_other_rect.y - t_other_mask.origin.y);
		t_other_x_inc = t_other_mask.scale * ((MCGFloat)t_other_rect.width / (MCGFloat)t_int_rect.width);
		t_other_y_inc = t_other_mask.scale * ((MCGFloat)t_other_rect.height / (MCGFloat)t_int_rect.height);
		
		// Now check for overlap!
		t_intersects = false;
		// IM-2013-05-10: optimize - exit from outer loop if intersect found
		for(int32_t y = 0; !t_intersects && y < t_int_rect . height; y++)
		{
			// Fill the scanline for this.
			mask_fill_scanline(t_this_mask, p_threshold, (uint32_t)floorf(t_this_y), t_this_x, t_this_x_inc, t_int_rect.width, t_this_scanline);
			t_this_y += t_this_y_inc;
			
			// Fill the scanline for other.
			mask_fill_scanline(t_other_mask, p_threshold, (uint32_t)floorf(t_other_y), t_other_x, t_other_x_inc, t_int_rect.width, t_other_scanline);
			t_other_y += t_other_y_inc;
			
			// Check to see if they intersect.
			for(int32_t x = 0; !t_intersects && x < t_scanline_width; x++)
				t_intersects = (t_this_scanline[x] & t_other_scanline[x]) != 0;
		}
		
		// Scanlines aren't needed anymore.
		MCMemoryDeleteArray(t_this_scanline);
		MCMemoryDeleteArray(t_other_scanline);
		
		// Free the temporary masks that were generated (if any).
		MCImageFreeBitmap(t_this_mask . temp_bitmap);
		MCImageFreeBitmap(t_other_mask . temp_bitmap);
	}
	
	p_other -> unlockshape(t_other_shape);
	unlockshape(t_this_shape);
	
	return t_intersects;
}

bool MCObject::lockshape(MCObjectShape& r_shape)
{
	// By default we assume an object is complex - requiring rendering to
	// determine its shape.
	r_shape . type = kMCObjectShapeComplex;
	r_shape . bounds = getrect();
	return true;
}

void MCObject::unlockshape(MCObjectShape& p_shape)
{
}

// MW-2012-02-14: [[ FontRefs ]] New method which maps the object's concrete font
//   on open.
void MCObject::mapfont(void)
{
	// MW-2012-02-24: [[ FontRefs ]] Fix a problem with images used as icons.
	//   Images don't use the fontref, so don't do anything if we are an image.

	if (gettype() == CT_IMAGE)
		return;
	
	// MW-2012-03-02: [[ Bug 10044 ]] If the parent isn't open, then we won't have a
	//   font. This causes problems for some things (like import snapshot) so in this
	//   case we ask the parent to map it's font.
	// MW-2012-03-12: [[ Bug 10078 ]] Only map the parent if the font is nil otherwise
	//   stacks with substacks have their fonts unmapped incorrectly.
	bool t_mapped_parent;
	t_mapped_parent = false;
	if (parent != nil && parent -> m_font == nil)
	{
		t_mapped_parent = true;
		parent -> mapfont();
	}
	
	// MW-2013-12-19: [[ Bug 11606 ]] Make sure we check for a stack using ideal layout
	//   as this requires new font computation.
	// If we have a font setting, then we create a new font. Otherwise we just
	// copy the parent's font.
	if (hasfontattrs() || (gettype() == CT_STACK && static_cast<MCStack *>(this) -> getuseideallayout()))
	{
		// MW-2012-02-19: [[ SplitTextAttrs ]] Compute the attrs to write out. If we don't
		//   have all of the attrs, fetch the inherited ones.
		MCNameRef t_textfont;
		uint2 t_textstyle, t_textsize;
		getfontattsnew(t_textfont, t_textsize, t_textstyle);

		// Map the font style from a text style.
		MCFontStyle t_font_style;
		t_font_style = MCFontStyleFromTextStyle(t_textstyle);

		// If the parent has printer metrics, make sure we do too.
		// MW-2012-08-30: [[ Bug 10295 ]] If this is a stack and it has formatForPrinting
		//   set, make sure we create a printer font.
		// MW-2013-12-04: [[ Bug 11513 ]] Make sure we check for ideal layout, rather than
		//   just for formatForPrinting.
		if (parent != nil && MCFontHasPrinterMetrics(parent -> m_font) ||
			gettype() == CT_STACK && ((MCStack *)this) -> getuseideallayout())
			t_font_style |= kMCFontStylePrinterMetrics;

		// Create our font.
		/* UNCHECKED */ MCFontCreate(t_textfont, t_font_style, t_textsize, m_font);
	}
	else if (parent != nil)
	{
		if (parent -> m_font == nil)
		{
			MCLog("[ %p ] parent font == nil (%d)", this, gettype());
		}
		else
			m_font = MCFontRetain(parent -> m_font);
	}
	else
	{
		// This should never happen as the only object with nil parent when
		// opened should be MCdispatcher, which always has font attrs.
		assert(false);
	}
	
	// MW-2012-03-02: [[ Bug 10044 ]] If we had to temporarily map the parent's font
	//   then unmap it here.
	// MW-2012-03-12: [[ Bug 10078 ]] Only unmap the parent if we mapped it in the
	//   first place.
	if (t_mapped_parent)
		parent -> unmapfont();
}

// MW-2012-02-14: [[ FontRefs ]] New method which unmaps the object's concrete font
//   on close.
void MCObject::unmapfont(void)
{
	MCFontRelease(m_font);
	m_font = nil;
}

// MW-2012-02-14: [[ FontRefs ]] New method which updates the object's concrete font
//   when a text property, or inherited text property changes.
bool MCObject::recomputefonts(MCFontRef p_parent_font)
{
	// MW-2012-02-19: [[ SplitTextAttrs ]] If the object has no font attrs, then just
	//   inherit.
	if (!hasfontattrs())
	{
		if (p_parent_font == m_font)
			return false;

		MCFontRelease(m_font);
		m_font = MCFontRetain(p_parent_font);

		return true;
	}

	MCFontRef t_current_font;
	t_current_font = MCFontRetain(m_font);
	
	unmapfont();
	mapfont();
	
	bool t_changed;
	t_changed = t_current_font != m_font;

	MCFontRelease(t_current_font);

	return t_changed;
}

///////////////////////////////////////////////////////////////////////////////

// MW-2012-02-17: [[ LogFonts ]] Copy the font attrs from the other object - this
//   assumes m_font_attrs hasn't been initialized. Note that this is callled 
//   from object-copy constructor, and the font flags for this are already the
//   same as other.
void MCObject::copyfontattrs(const MCObject& p_other)
{
	// If there are no font attrs, then do nothing.
	if (!hasfontattrs())
	{
		m_font_attrs = nil;
		return;
	}

	/* UNCHECKED */ MCMemoryNew(m_font_attrs);
	if ((m_font_flags & FF_HAS_TEXTFONT) != 0)
		MCNameClone(p_other . m_font_attrs -> name, m_font_attrs -> name);
	if ((m_font_flags & FF_HAS_TEXTSIZE) != 0)
		m_font_attrs -> size = p_other . m_font_attrs -> size;
	if ((m_font_flags & FF_HAS_TEXTSTYLE) != 0)
		m_font_attrs -> style = p_other . m_font_attrs -> style;
}

// MW-2012-02-17: [[ LogFonts ]] Set all font attrs to empty.
void MCObject::clearfontattrs(void)
{
	if (m_font_attrs == nil)
		return;

	MCNameDelete(m_font_attrs -> name);
	delete m_font_attrs;
	m_font_attrs = nil;

	// MW-2012-02-19: [[ SplitTextAttrs ]] Unset all the individual fontattr flags.
	m_font_flags &= ~FF_HAS_ALL_FATTR;
}

// MW-2012-02-17: [[ LogFonts ]] Set the object's font attrs to the ones specified
//   by the given index.
void MCObject::loadfontattrs(uint2 p_index)
{
	// Lookup the font attrs details in the logical font table.
	MCNameRef t_textfont;
	uint2 t_textsize, t_textstyle;
	bool t_unicode;
	MCLogicalFontTableLookup(p_index, t_textfont, t_textstyle, t_textsize, t_unicode);

	// If the font had a unicode tag, then mark the object as having one.
	if (t_unicode)
		m_font_flags |= FF_HAS_UNICODE_TAG;

	// If any of the attrs are not in the font flags, we clear them (size / style == 0
	// font = nil).
	if ((m_font_flags & FF_HAS_TEXTFONT) == 0)
		t_textfont = nil;
	if ((m_font_flags & FF_HAS_TEXTSTYLE) == 0)
		t_textstyle = 0;
	if ((m_font_flags & FF_HAS_TEXTSIZE) == 0)
		t_textsize = 0;

	// Configure the attrs.
	setfontattrs(FF_HAS_ALL_FATTR, t_textfont, t_textsize, t_textstyle);
}

// MW-2012-02-17: [[ LogFonts ]] Compute the index to be saved into the stackfile
//   based on the object's font attrs.
uint2 MCObject::savefontattrs(void)
{
	// Fetch the object's font attrs. This ensures appropriate inherited values are
	// used when an object has unicode and so a font needs to be synthesized.
	MCNameRef t_textfont;
	uint2 t_textsize, t_textstyle;
	getfontattsnew(t_textfont, t_textsize, t_textstyle);

	// Now lookup the index for the given font attrs in the logical font table.
	return MCLogicalFontTableMap(t_textfont, t_textstyle, t_textsize, (m_font_flags & FF_HAS_UNICODE) != 0);
}

// MW-2012-02-17: [[ LogFonts ]] Set the logical font attrs to the given values. Note
//   that we ignore the object's settings of the font flags here since they might not
//   reflect reality (i.e. on load).
void MCObject::setfontattrs(uint32_t p_which, MCNameRef p_textfont, uint2 p_textsize, uint2 p_textstyle)
{
	if (p_which == 0)
	{
		if (m_font_attrs != nil)
			MCNameDelete(m_font_attrs -> name);
		delete m_font_attrs;
		m_font_attrs = nil;
		m_font_flags &= ~FF_HAS_ALL_FATTR;
		return;
	}

	if (m_font_attrs == nil)
		/* UNCHECKED */ MCMemoryNew(m_font_attrs);

	if ((p_which & FF_HAS_TEXTFONT) != 0)
	{
		MCNameDelete(m_font_attrs -> name);
		if (p_textfont != nil && !MCNameIsEmpty(p_textfont))
		{
			/* UNCHECKED */ MCNameClone(p_textfont, m_font_attrs -> name);
			m_font_flags |= FF_HAS_TEXTFONT;
		}
		else
		{
			m_font_attrs -> name = nil;
			m_font_flags &= ~FF_HAS_TEXTFONT;
		}
	}

	if ((p_which & FF_HAS_TEXTSIZE) != 0)
	{
		if (p_textsize != 0)
		{
			m_font_attrs -> size = p_textsize;
			m_font_flags |= FF_HAS_TEXTSIZE;
		}
		else
		{
			m_font_attrs -> size = 0;
			m_font_flags &= ~FF_HAS_TEXTSIZE;
		}
	}

	if ((p_which & FF_HAS_TEXTSTYLE) != 0)
	{
		if (p_textstyle != 0)
		{
			m_font_attrs -> style = p_textstyle;
			m_font_flags |= FF_HAS_TEXTSTYLE;
		}
		else
		{
			m_font_attrs -> style = 0;
			m_font_flags &= ~FF_HAS_TEXTSTYLE;
		}
	}
}

// MW-2012-02-17: [[ LogFonts ]] Set the logical font attrs to the given values
//   using a c-string for the name.
void MCObject::setfontattrs(const char *p_textfont, uint2 p_textsize, uint2 p_textstyle)
{
	MCAutoNameRef t_textfont_name;
	/* UNCHECKED */ t_textfont_name . CreateWithCString(p_textfont);
	setfontattrs(FF_HAS_ALL_FATTR, t_textfont_name, p_textsize, p_textstyle);
}

// MW-2012-02-19: [[ SplitTextAttrs ]] This method returns true if any of the font
//   attrs are set.
bool MCObject::hasfontattrs(void) const
{
	return (m_font_flags & FF_HAS_ALL_FATTR) != 0;
}

// MW-2012-02-19: [[ SplitTextAttrs ]] This method returns true if the fontflags
//   extended record is needed. We only need this if we are going to generate a
//   font record to serialize, and the object has partial (or no) font attrs set.
bool MCObject::needtosavefontflags(void) const
{
	return needtosavefontrecord() && (m_font_flags & FF_HAS_ALL_FATTR) != FF_HAS_ALL_FATTR;
}

// MW-2012-02-19: [[ SplitTextAttrs ]] This method returns true if a font record
//   is needed when saving. This occurs if any of the font attr are set, the
//   object is marked as unicode or the object has a fontheight.
// MW-2012-03-13: [[ Bug 10083 ]] Also need to save record if parent unicodeness
//   is different from ours.
bool MCObject::needtosavefontrecord(void) const
{
	return hasfontattrs() || hasunicode() || fontheight != 0 || (parent != nil && parent -> hasunicode() != hasunicode());
}

// MW-2012-06-08: [[ Relayer ]] No-op - only implemented for containers.
void MCObject::relayercontrol(MCControl *source, MCControl *target)
{
}

void MCObject::relayercontrol_remove(MCControl *p_control)
{
}

void MCObject::relayercontrol_insert(MCControl *p_control, MCControl *p_target)
{
}

void MCObject::scheduledelete(void)
{
	appendto(MCtodelete);
	if (m_weak_handle != nil)
	{
		m_weak_handle -> Clear();
		m_weak_handle = nil;
	}
	
	// MW-2012-10-10: [[ IdCache ]] Remove the object from the stack's id cache
	//   (if it is in it!).
	if (m_in_id_cache)
		getstack() -> uncacheobjectbyid(this);
}

MCRectangle MCObject::measuretext(const MCString& p_text, bool p_is_unicode)
{
    bool t_mapped_font;
    t_mapped_font = false;
    if (!opened && m_font == nil)
    {
        mapfont();
        t_mapped_font = true;
    }
    
    MCRectangle t_bounds;
    t_bounds . x = 0;
	// MW-2013-08-23: [[ MeasureText ]] Shortcut if no text - useful for just
	//   getting the font ascent/descent (as used in MCGroup methods).
	// MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
	if (p_text . getlength() != 0)
		t_bounds . width = MCFontMeasureText(m_font, p_text . getstring(), p_text . getlength(), p_is_unicode, getstack() -> getdevicetransform());
	else
		t_bounds . width = 0;
    t_bounds . y = -MCFontGetAscent(m_font);
    t_bounds . height = MCFontGetDescent(m_font) + MCFontGetAscent(m_font);
    
    if (t_mapped_font)
        unmapfont();
    
    return t_bounds;
}

///////////////////////////////////////////////////////////////////////////////

bool MCObject::visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor *p_visitor)
{
	return p_visitor -> OnObject(this);
}

///////////////////////////////////////////////////////////////////////////////

MCObjectVisitor::~MCObjectVisitor(void)
{
}

bool MCObjectVisitor::OnObject(MCObject *p_object)
{
	return true;
}

bool MCObjectVisitor::OnControl(MCControl *p_control)
{
	return OnObject(p_control);
}

bool MCObjectVisitor::OnStack(MCStack *p_stack)
{
	return OnObject(p_stack);
}

bool MCObjectVisitor::OnAudioClip(MCAudioClip *p_audio_clip)
{
	return OnObject(p_audio_clip);
}

bool MCObjectVisitor::OnVideoClip(MCVideoClip *p_video_clip)
{
	return OnObject(p_video_clip);
}

bool MCObjectVisitor::OnCard(MCCard *p_card)
{
	return OnObject(p_card);
}

bool MCObjectVisitor::OnGroup(MCGroup *p_group)
{
	return OnControl(p_group);
}

bool MCObjectVisitor::OnField(MCField *p_field)
{
	return OnControl(p_field);
}

bool MCObjectVisitor::OnButton(MCButton *p_button)
{
	return OnControl(p_button);
}

bool MCObjectVisitor::OnImage(MCImage *p_image)
{
	return OnControl(p_image);
}

bool MCObjectVisitor::OnScrollbar(MCScrollbar *p_scrollbar)
{
	return OnControl(p_scrollbar);
}

bool MCObjectVisitor::OnPlayer(MCPlayer *p_player)
{
	return OnControl(p_player);
}

bool MCObjectVisitor::OnStyledText(MCStyledText *p_styled_text)
{
	return OnObject(p_styled_text);
}

bool MCObjectVisitor::OnParagraph(MCParagraph *p_paragraph)
{
	return true;
}

bool MCObjectVisitor::OnBlock(MCBlock *p_block)
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////

MCObjectHandle::MCObjectHandle(MCObject *p_object)
{
	m_object = p_object;
	m_references = 0;
}

MCObjectHandle::~MCObjectHandle(void)
{
}

MCObject *MCObjectHandle::Get(void)
{
	return m_object;
}

void MCObjectHandle::Clear(void)
{
	m_object = NULL;
}

void MCObjectHandle::Retain(void)
{
	m_references += 1;
}

void MCObjectHandle::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
	{
		if (m_object != NULL)
			m_object -> m_weak_handle = NULL;
		delete this;
	}
}

bool MCObjectHandle::Exists(void)
{
	return m_object != NULL;
}
