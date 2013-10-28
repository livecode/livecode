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
#include "dispatch.h"
#include "stack.h"
#include "tooltip.h"
#include "card.h"
#include "field.h"
#include "button.h"
#include "image.h"
#include "aclip.h"
#include "vclip.h"
#include "stacklst.h"
#include "mcerror.h"
#include "hc.h"
#include "util.h"
#include "param.h"
#include "debug.h"
#include "statemnt.h"
#include "funcs.h"
#include "magnify.h"
#include "sellst.h"
#include "undolst.h"
#include "styledtext.h"
#include "hndlrlst.h"
#include "handler.h"
#include "property.h"
#include "internal.h"
#include "player.h"
#include "objptr.h"
#include "osspec.h"

#include "globals.h"
#include "license.h"
#include "mode.h"
#include "revbuild.h"
#include "parentscript.h"

#include "resolution.h"
#include "group.h"

#if defined(_WINDOWS_DESKTOP)
#include "w32prefix.h"
#include "w32dc.h"
#include <process.h>

// MW-2013-04-18: [[ Bug ]] Temporarily undefine 'GetObject' so that the reference
//   to the GetObject() parentscript object method works. (Platform-specific stuff
//   needs to move into separate files!)
#undef GetObject

#elif defined(_MAC_DESKTOP)
#include "osxprefix.h"
#elif defined(_LINUX_DESKTOP)
#include <unistd.h>
#endif

////////////////////////////////////////////////////////////////////////////////

 void X_main_loop(void);

//////////

bool MCFiltersDecompress(MCDataRef p_source, MCDataRef& r_result);

////////////////////////////////////////////////////////////////////////////////

// MM-2012-09-05: [[ Property Listener ]]
#ifdef FEATURE_PROPERTY_LISTENER
void MCInternalObjectListenerMessagePendingListeners(void);
void MCInternalObjectListenerListListeners(MCExecPoint &ep);
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  Globals specific to DEVELOPMENT mode
//

MCLicenseParameters MClicenseparameters =
{
	NULL, NULL, NULL, kMCLicenseClassNone, 0,
	10, 10, 50, 10,
	0,
	NULL,
};

Boolean MCenvironmentactive = False;

MCObject *MCmessageboxredirect = NULL;

// IM-2013-04-16: [[ BZ 10836 ]] Provide reference to the last "put" source
// as a global property, the "revMessageBoxLastObject"
MCObjectHandle *MCmessageboxlastobject = nil;
MCNameRef MCmessageboxlasthandler = nil;
uint32_t MCmessageboxlastline = 0;

Boolean MCcrashreportverbose = False;
char *MCcrashreportfilename = NULL;

////////////////////////////////////////////////////////////////////////////////
//
//  Commands and functions specific to development mode.
//

class MCRevRelicense : public MCStatement
{
public:
	MCRevRelicense()
	{
	}
	
	virtual ~MCRevRelicense();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual Exec_stat exec(MCExecPoint &);
};

static MCStringRef s_command_path = nil;

static void restart_revolution(void)
{
#if defined(TARGET_PLATFORM_WINDOWS)
	_spawnl(_P_NOWAIT, MCStringGetCString(s_command_path), MCStringGetCString(s_command_path), NULL);
#elif defined(TARGET_PLATFORM_MACOS_X)
	if (fork() == 0)
	{
		usleep(250000);
		execl(MCStringGetCString(MCcmd), MCStringGetCString(MCcmd), NULL);
	}
#elif defined(TARGET_PLATFORM_LINUX)
	if (fork() == 0)
	{
		usleep(250000);
		execl(MCStringGetCString(MCcmd), MCStringGetCString(MCcmd), NULL);
	}
#else
#error restart not defined
#endif
}

MCRevRelicense::~MCRevRelicense(void)
{
}

Parse_stat MCRevRelicense::parse(MCScriptPoint& sp)
{
  initpoint(sp);
	
	return PS_NORMAL;
}

Exec_stat MCRevRelicense::exec(MCExecPoint& ep)
{
	switch(MCdefaultstackptr -> getcard() -> message(MCM_shut_down_request))
	{
	case ES_PASS:
	case ES_NOT_HANDLED:
		break;
		
	default:
		MCresult -> sets("cancelled");
		
		return ES_NORMAL;
	}
	
	if (MClicenseparameters . license_token == NULL || strlen(MClicenseparameters . license_token) == 0)
	{
		MCresult -> sets("no token");
		return ES_NORMAL;
	}
	
	MCAutoStringRef license_token_string;
	/* UNCHECKED */ MCStringCreateWithCString(MClicenseparameters . license_token, &license_token_string);
	if (!MCS_unlink(*license_token_string))
	{
		MCresult -> sets("token deletion failed");
		return ES_NORMAL;
	}

	MCretcode = 0;
	MCquit = True;
	MCexitall = True;
	MCtracestackptr = NULL;
	MCtraceabort = True;
	MCtracereturn = True;
    
    MCAutoStringRef t_command_path;
    MCS_resolvepath(MCcmd, &t_command_path);
	
	s_command_path = MCValueRetain(*t_command_path);

	atexit(restart_revolution);
	
	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of MCDispatch::startup method for DEVELOPMENT mode.
//

extern void send_relaunch(void);
extern void send_startup_message(bool p_do_relaunch = true);

extern uint4 MCstartupstack_length;
extern uint1 MCstartupstack[];

IO_stat MCDispatch::startup(void)
{
	IO_stat stat;
	MCStack *sptr;

	// set up image cache before the first stack is opened
	MCCachedImageRep::init();
    MCAutoStringRef t_startdir;
    MCS_getcurdir(&t_startdir);
	
	startdir = strdup(MCStringGetCString(*t_startdir));
	enginedir = strdup(MCStringGetCString(MCcmd));

	char *eptr;
	eptr = strrchr(enginedir, PATH_SEPARATOR);
	if (eptr != NULL)
		*eptr = '\0';
	else
		*enginedir = '\0';

	MCDataRef t_decompressed;
	MCDataRef t_compressed;
	/* UNCHECKED */ MCDataCreateWithBytes((const char_t*)MCstartupstack, MCstartupstack_length, t_compressed);
	/* UNCHECKED */ MCFiltersDecompress(t_compressed, t_decompressed);
	MCValueRelease(t_compressed);
    
	IO_handle stream = MCS_fakeopen(MCDataGetOldString(t_decompressed));
	if ((stat = MCdispatcher -> readfile(NULL, NULL, stream, sptr)) != IO_NORMAL)
	{
		MCS_close(stream);
		return stat;
	}

	MCS_close(stream);

	/* FRAGILE */ memset((void *)MCDataGetBytePtr(t_decompressed), 0, MCDataGetLength(t_decompressed));
	MCValueRelease(t_decompressed);

	// Temporary fix to make sure environment stack doesn't get lost behind everything.
#if defined(_MACOSX)
	ProcessSerialNumber t_psn = { 0, kCurrentProcess };
	SetFrontProcess(&t_psn);
#elif defined(_WINDOWS)
	SetForegroundWindow(((MCScreenDC *)MCscreen) -> getinvisiblewindow());
#endif
	
	MCenvironmentactive = True;
	sptr -> setfilename(MCcmd);
	MCdefaultstackptr = MCstaticdefaultstackptr = stacks;

	{
		MCdefaultstackptr -> setextendedstate(true, ECS_DURING_STARTUP);
		MCdefaultstackptr -> message(MCM_start_up, nil, False, True);
		MCdefaultstackptr -> setextendedstate(false, ECS_DURING_STARTUP);
	}
	
	if (!MCquit)
	{
		MCExecPoint ep;
		MCresult -> eval(ep);
		ep . appendchar('\0');
		if (ep . getsvalue() . getlength() == 1)
		{
			sptr -> open();
			MCImage::init();
			
			X_main_loop();

			MCresult -> eval(ep);
			ep . appendchar('\0');
			if (ep . getsvalue() . getlength() == 1)
				return IO_NORMAL;
		}

		// TODO: Script Wiping
		// if (sptr -> getscript() != NULL)
		//	memset(sptr -> getscript(), 0, strlen(sptr -> getscript()));

		destroystack(sptr, True);
		MCtopstackptr = NULL;
		MCquit = False;
		MCenvironmentactive = False;

		send_relaunch();

		MCNewAutoNameRef t_name;
		/* UNCHECKED */ ep.copyasnameref(&t_name);
		sptr = findstackname(*t_name);

		if (sptr == NULL && (stat = loadfile(MCNameGetString(*t_name), sptr)) != IO_NORMAL)
			return stat;
	}

	if (!MCquit)
	{
		// OK-2007-11-13 : Bug 5525, after opening the IDE engine, the allowInterrupts should always default to false,
		// regardless of what the environment stack may have set it to.
		MCallowinterrupts = true;
		sptr -> setparent(this);
		MCdefaultstackptr = MCstaticdefaultstackptr = stacks;
		send_startup_message(false);
		if (!MCquit)
			sptr -> open();
	}

	return IO_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of MCStack::mode* hooks for DEVELOPMENT mode.
//

static MCStack *s_links = NULL;

struct MCStackModeData
{
	MCStack *next;
	MCStack *previous;
	MCStack *referrer;
};

void MCStack::mode_create(void)
{
	m_mode_data = new MCStackModeData;
	m_mode_data -> next = NULL;
	m_mode_data -> previous = NULL;
	m_mode_data -> referrer = MCtargetptr != NULL ? MCtargetptr -> getstack() : NULL;
	s_links = this;
}

void MCStack::mode_copy(const MCStack& stack)
{
	mode_create();
}

void MCStack::mode_destroy(void)
{
	MCStack *t_previous_link;
	bool t_link_found;
	t_link_found = false;
	t_previous_link = NULL;
	for(MCStack *t_link = s_links; t_link != NULL; t_link = t_link -> m_mode_data -> next)
	{
		if (t_link -> m_mode_data -> referrer == this)
			t_link -> m_mode_data -> referrer = m_mode_data -> referrer;
			
		if (!t_link_found)
		{
			t_link_found = t_link == this;
			if (!t_link_found)
				t_previous_link = t_link;
		}
	}
			
	if (t_previous_link == NULL)
		s_links = m_mode_data -> next;
	else
		t_previous_link -> m_mode_data -> next = m_mode_data -> next;

	delete m_mode_data;
}

Exec_stat MCStack::mode_getprop(uint4 parid, Properties which, MCExecPoint &ep, MCStringRef carray, Boolean effective)
{
	switch(which)
	{
	case P_REFERRING_STACK:
		if (m_mode_data -> referrer != NULL)
			return m_mode_data -> referrer -> getprop(0, P_LONG_ID, ep, False);
		else
			ep . clear();
	break;

	case P_UNPLACED_GROUP_IDS:
		ep . clear();
		
		if (controls != NULL)
		{
			uint4 t_count;
			t_count = 0;
			
			MCControl *t_control;
			t_control = controls;
			do
			{
				if (t_control -> gettype() == CT_GROUP && t_control -> getparent() != curcard)
				{
					ep . concatuint(t_control -> getid(), EC_RETURN, t_count == 0);
					t_count += 1;
				}
					
				t_control = t_control -> next();
			}
			while(t_control != controls);
		}
	break;

	case P_IDE_OVERRIDE:
		ep.setboolean(getextendedstate(ECS_IDE));
	break;

	default:
		return ES_NOT_HANDLED;
	}
	return ES_NORMAL;
}

Exec_stat MCStack::mode_setprop(uint4 parid, Properties which, MCExecPoint &ep, MCStringRef cprop, MCStringRef carray, Boolean effective)
{
	switch(which)
	{
	case P_IDE_OVERRIDE:
		Boolean t_state;
		if (!MCU_stob(ep . getsvalue(), t_state))
		{
			MCeerror -> add(EE_OBJECT_NAB, 0, 0, ep . getsvalue());
			return ES_ERROR;
		}

		setextendedstate(t_state == True, ECS_IDE);
	break;

	default:
		return ES_NOT_HANDLED;
	}

	return ES_NORMAL;
}

void MCStack::mode_load(void)
{
	// We introduce the notion of an 'IDE' stack - such a stack is set by giving it
	// a custom property 'ideOverride' with value true.
	if (props != NULL)
	{
		MCAutoNameRef t_ide_override_name;
		/* UNCHECKED */ t_ide_override_name . CreateWithCString("ideOverride");

		MCExecPoint ep;
		MClockmessages++;
		getcustomprop(ep, kMCEmptyName, t_ide_override_name);
		MClockmessages--;

		Boolean t_treat_as_ide;
		if (MCU_stob(ep . getsvalue(), t_treat_as_ide) && t_treat_as_ide)
			setextendedstate(true, ECS_IDE);
	}
}

void MCStack::mode_getrealrect(MCRectangle& r_rect)
{
	MCscreen->getwindowgeometry(window, r_rect);
}

void MCStack::mode_takewindow(MCStack *other)
{
}

void MCStack::mode_takefocus(void)
{
	MCscreen->setinputfocus(window);
}

bool MCStack::mode_needstoopen(void)
{
	return true;
}

void MCStack::mode_setgeom(void)
{
}

void MCStack::mode_setcursor(void)
{
	MCscreen->setcursor(window, cursor);
}

bool MCStack::mode_openasdialog(void)
{
	return true;
}

void MCStack::mode_closeasdialog(void)
{
}

void MCStack::mode_openasmenu(MCStack *grab)
{
}

void MCStack::mode_closeasmenu(void)
{
}

bool MCStack::mode_haswindow(void)
{
	return window != DNULL;
}

void MCStack::mode_constrain(MCRectangle& rect)
{
}

#ifdef _WINDOWS
MCSysWindowHandle MCStack::getrealwindow(void)
{
	return window->handle.window;
}

MCSysWindowHandle MCStack::getqtwindow(void)
{
	return window->handle.window;
}
#endif

#ifdef _MACOSX
MCSysWindowHandle MCStack::getqtwindow(void)
{
	return window->handle.window;
}
#endif

////////////////////////////////////////////////////////////////////////////////

static bool enumerate_handlers_for_object(MCObject *p_object, MCExecPoint &ep, bool p_first)
{
	if (p_object == NULL)
		return p_first;
	
	if (!p_first)
		ep . appendnewline();	
	p_first = true;
	
	MCHandlerlist *t_handlers;
	t_handlers = p_object -> gethandlers();
	if (t_handlers != NULL && p_object -> getstack() -> iskeyed())
		p_first = t_handlers -> enumerate(ep, p_first);
	
	if (p_object -> gettype() == CT_CARD)
	{
		MCCard *t_card;
		t_card = (MCCard *) p_object;
		
		MCObjptr *t_card_objs;
		t_card_objs = t_card -> getobjptrs();		
		
		if (t_card_objs != NULL)
		{
			MCObjptr *t_object_ptr;
			t_object_ptr = t_card_objs -> prev();
			do
			{
				MCGroup *t_group;
				t_group = t_object_ptr -> getrefasgroup();
				if (t_group != NULL && t_group -> isbackground())
				{
					t_group -> parsescript(False);
					p_first = enumerate_handlers_for_object(t_group, ep, p_first);
				}
				t_object_ptr = t_object_ptr -> prev();
			}
			while (t_object_ptr != t_card_objs -> prev());
		}
	}
	
	if (p_object -> getparentscript() != NULL)
	{
		MCObject *t_behavior;
		t_behavior = p_object -> getparentscript() -> GetObject();
		if (t_behavior != NULL)
		{
			t_behavior -> parsescript(False);
			p_first = enumerate_handlers_for_object(t_behavior, ep, p_first);
		}
	}
	
	return p_first;
}

static bool enumerate_handlers_for_list(MCObjectList *p_list, MCObject *p_ignore, MCExecPoint &ep, bool p_first)
{
	if (p_list == NULL)
		return p_first;
	
	MCObjectList *t_object_ref;
	t_object_ref = p_list;
	do
	{
		if (p_ignore != t_object_ref -> getobject())
			if (!t_object_ref -> getremoved())
				p_first = enumerate_handlers_for_object(t_object_ref -> getobject(), ep, p_first);
		
		t_object_ref = t_object_ref -> next();
	}
	while (t_object_ref != p_list);
	
	return p_first;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of MCObject::getmodeprop for DEVELOPMENT mode.
//

Exec_stat MCObject::mode_getprop(uint4 parid, Properties which, MCExecPoint &ep, MCStringRef carray, Boolean effective)
{
	switch(which)
	{
	// MW-2010-07-09: [[ Bug 8848 ]] Previously scripts were being compiled into
	//   separate hlists causing script local variable loss in the behavior
	//   case. Instead we just use parsescript in non-reporting mode.
	case P_REV_AVAILABLE_HANDLERS:
	{
		bool t_first;
		t_first = true;
		ep . clear();

		if (!effective)
		{
			parsescript(False);
			if (hlist != NULL)
				t_first = hlist -> enumerate(ep, t_first);
		}
		else
		{
			// MM-2013-09-10: [[ Bug 10634 ]] Make we search both parent scripts and library stacks for handlers.
			t_first = enumerate_handlers_for_list(MCfrontscripts, this, ep, t_first);

			for (MCObject *t_object = this; t_object != NULL; t_object = t_object -> parent)
			{
				t_object -> parsescript(False);
				t_first = enumerate_handlers_for_object(t_object, ep, t_first);
			}

			t_first = enumerate_handlers_for_list(MCbackscripts, this, ep, t_first);

			for (uint32_t i = 0; i < MCnusing; i++)
			{
				if (MCusing[i] == this)
					continue;
				t_first = enumerate_handlers_for_object(MCusing[i], ep, t_first);
			}			
		}
	}
	break;

	// OK-2008-04-23 : Added for script editor
	case P_REV_AVAILABLE_VARIABLES:
	{
		ep.clear();
		if (hlist == NULL)
		{
			return ES_NORMAL;
		}

		// A handler can be specified using array notation in the form <handler_type>,<handler_name>.
		// Where handler type is a single letter using the same conventation as the revAvailableHandlers.
		//
		// If a handler is specified, the list of variables for that handler is returned in the same format
		// as the variableNames property.
		//
		// If no handler is specified, the property returns the list of script locals for the object followed
		// by the list of script-declared globals.
		// 
		// At the moment, no errors are thrown, just returns empty if it doesn't like something.
		if (carray == NULL)
		{
			hlist -> appendlocalnames(ep);
			ep . appendnewline();
			hlist -> appendglobalnames(ep, true);
			return ES_NORMAL;
		}

		char *t_key;
		t_key = strdup(MCStringGetCString(carray));
		if (t_key == NULL)
			return ES_NORMAL;

		char *t_comma;
		t_comma = strchr(t_key, ',');
		if (t_comma == NULL)
		{
			free(t_key);
			return ES_NORMAL;
		}

		// The handler name begins after the comma character
		char *t_handler_name_cstring;
		t_handler_name_cstring = t_comma + 1;

		// The handler code must be the first char of the string
		char t_handler_code;
		t_handler_code = *t_key;
		t_handler_code = MCS_toupper(t_handler_code);

		Handler_type t_handler_type;
		switch (t_handler_code)
		{
		case 'M':
			t_handler_type = HT_MESSAGE;
			break;
		case 'C':
			t_handler_type = HT_MESSAGE;
			break;
		case 'F':
			t_handler_type = HT_FUNCTION;
			break;
		case 'G':
			t_handler_type = HT_GETPROP;
			break;
		case 'S':
			t_handler_type = HT_SETPROP;
			break;
		default:
			t_handler_type = HT_MESSAGE;
		}

		MCAutoNameRef t_handler_name;
		/* UNCHECKED */ t_handler_name . CreateWithCString(t_handler_name_cstring);

		Exec_stat t_status;

		// The handler list class allows us to locate the handler, just return empty if it can't be found.
		MCHandler *t_handler;
		t_status = hlist -> findhandler(t_handler_type, t_handler_name, t_handler);
		if (t_status != ES_NORMAL)
		{
			free(t_key);
			return ES_NORMAL;
		}

		if (t_handler != NULL)
			t_handler -> getvarnames(ep, true);

		free(t_key);
		return ES_NORMAL;
	}
	break;

	default:
		return ES_NOT_HANDLED;
	}

	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of MCProperty::mode_eval/mode_set for DEVELOPMENT mode.
//

Exec_stat MCProperty::mode_set(MCExecPoint& ep)
{
	switch(which)
	{
	case P_REV_CRASH_REPORT_SETTINGS:
		{
			MCAutoArrayRef t_settings;
			if (!ep . copyasarrayref(&t_settings))
				break;

			MCExecPoint ep_key(ep);

			if (ep_key . fetcharrayelement_cstring(*t_settings, "verbose") == ES_NORMAL)
				MCU_stob(ep_key . getsvalue(), MCcrashreportverbose);

			if (ep_key . fetcharrayelement_cstring(*t_settings, "filename") == ES_NORMAL)
			{
				if (MCcrashreportfilename != NULL)
					delete MCcrashreportfilename;
				MCcrashreportfilename = ep_key . getsvalue() . getlength() > 0 ? ep_key . getsvalue() . clone() : NULL;
			}
		}
		break;

	case P_REV_LICENSE_LIMITS:
		{
			if(!MCenvironmentactive)
				break;

			MCAutoArrayRef t_settings;
			if (!ep . copyasarrayref(&t_settings))
				break;

			MCExecPoint ep_key(ep);

			if (ep_key . fetcharrayelement_cstring(*t_settings, "token"))
				MClicenseparameters . license_token = ep_key . getsvalue() . clone();

			if (ep_key . fetcharrayelement_cstring(*t_settings, "name"))
				MClicenseparameters . license_name = ep_key . getsvalue() . clone();

			if (ep_key . fetcharrayelement_cstring(*t_settings, "organization"))
				MClicenseparameters . license_organization = ep_key . getsvalue() . clone();

			if (ep_key . fetcharrayelement_cstring(*t_settings, "class"))
			{
				static struct { const char *tag; uint32_t value; } s_class_map[] =
				{
					{ "community", kMCLicenseClassCommunity },
					{ "commercial", kMCLicenseClassCommercial },
					{ "professional", kMCLicenseClassProfessional },
					{ NULL, kMCLicenseClassNone }
				};
				
				uint4 t_index;
				for(t_index = 0; s_class_map[t_index] . tag != NULL; ++t_index)
					if (ep_key . getsvalue() == s_class_map[t_index] . tag)
						break;

				MClicenseparameters . license_class = s_class_map[t_index] . value;
			}

			if (ep_key . fetcharrayelement_cstring(*t_settings, "multiplicity") && ep_key . ton() == ES_NORMAL)
				MClicenseparameters . license_multiplicity = ep_key . getuint4();

			if (ep_key . fetcharrayelement_cstring(*t_settings, "scriptlimit") && ep_key . ton() == ES_NORMAL)
				MClicenseparameters . script_limit = ep_key . getnvalue() <= 0 ? 0 : ep_key . getuint4();

			if (ep_key . fetcharrayelement_cstring(*t_settings, "dolimit") && ep_key . ton() == ES_NORMAL)
				MClicenseparameters . do_limit = ep_key . getnvalue() <= 0 ? 0 : ep_key . getuint4();

			if (ep_key . fetcharrayelement_cstring(*t_settings, "usinglimit") && ep_key . ton() == ES_NORMAL)
				MClicenseparameters . using_limit = ep_key . getnvalue() <= 0 ? 0 : ep_key . getuint4();

			if (ep_key . fetcharrayelement_cstring(*t_settings, "insertlimit") && ep_key . ton() == ES_NORMAL)
				MClicenseparameters . insert_limit = ep_key . getnvalue() <= 0 ? 0 : ep_key . getuint4();

			if (ep_key . fetcharrayelement_cstring(*t_settings, "deploy"))
			{
				static struct { const char *tag; uint32_t value; } s_deploy_map[] =
				{
					{ "windows", kMCLicenseDeployToWindows },
					{ "macosx", kMCLicenseDeployToMacOSX },
					{ "linux", kMCLicenseDeployToLinux },
					{ "ios", kMCLicenseDeployToIOS },
					{ "android", kMCLicenseDeployToAndroid },
					{ "winmobile", kMCLicenseDeployToWinMobile },
					{ "meego", kMCLicenseDeployToLinuxMobile },
					{ "server", kMCLicenseDeployToServer },
					{ "ios-embedded", kMCLicenseDeployToIOSEmbedded },
					{ "android-embedded", kMCLicenseDeployToIOSEmbedded },
				};

				MClicenseparameters . deploy_targets = 0;

				uint32_t t_target_count;
				char **t_targets;
				t_target_count = 0;
				t_targets = nil;
				if (MCCStringSplit(ep_key . getcstring(), ',', t_targets, t_target_count))
				{
					for(uint32_t i = 0; i < t_target_count; i++)
					{
						for(uint32_t j = 0; j < sizeof(s_deploy_map) / sizeof(s_deploy_map[0]); j++)
							if (MCCStringEqualCaseless(s_deploy_map[j] . tag, t_targets[i]))
							{
								MClicenseparameters . deploy_targets |= s_deploy_map[j] . value;
								break;
							}
					}
					MCCStringArrayFree(t_targets, t_target_count);
				}
			}

			if (ep_key . fetcharrayelement_cstring(*t_settings, "addons") && ep_key . isarray())
				/* UNCHECKED */ ep_key . copyasarrayref(MClicenseparameters . addons);
		}
		break;

	case P_REV_MESSAGE_BOX_REDIRECT:
		{
			MCObject *t_object;
			t_object = getobj(ep);
			if (t_object != NULL)
				MCmessageboxredirect = t_object;
			else
				MCmessageboxredirect = NULL;
		}
		break;

#ifdef FEATURE_PROPERTY_LISTENER
	// MM-2012-11-06: [[ Property Listener ]]
	case P_REV_PROPERTY_LISTENER_THROTTLE_TIME:
		if (ep.getuint4(MCpropertylistenerthrottletime, line, pos, EE_OBJECT_NAN) != ES_NORMAL)
			return ES_ERROR;			
		break;
#endif
			
	default:
		return ES_NOT_HANDLED;
	}

	return ES_NORMAL;
}

Exec_stat MCProperty::mode_eval(MCExecPoint& ep)
{
	switch(which)
	{
	case P_REV_MESSAGE_BOX_LAST_OBJECT:
		if (MCmessageboxlastobject != NULL && MCmessageboxlastobject->Exists())
		{
			MCmessageboxlastobject->Get()->names_old(P_LONG_ID, ep, 0);
			ep.concatnameref(MCmessageboxlasthandler, EC_COMMA, false);
			ep.concatuint(MCmessageboxlastline, EC_COMMA, false);
			if (MCmessageboxlastobject->Get()->getparentscript() != nil)
			{
				MCExecPoint ep2;
				MCmessageboxlastobject->Get()->getparentscript()->GetObject()->names_old(P_LONG_ID, ep2, 0);
				ep.concatmcstring(ep2.getsvalue(), EC_COMMA, false);
			}
		}
		else
			ep.clear();
		break;
	case P_REV_MESSAGE_BOX_REDIRECT:
		if (MCmessageboxredirect != NULL)
			return MCmessageboxredirect -> names_old(P_LONG_ID, ep, 0);
		ep . clear();
		break;
	case P_REV_LICENSE_LIMITS:
		ep.clear();
		break;
	case P_REV_CRASH_REPORT_SETTINGS:
		ep.clear();
		break;
	case P_REV_LICENSE_INFO:
	{
		if (ep . isempty())
		{
			static const char *s_class_types[] =
			{
				"",
				"Community",
				"Commercial",
				"Professional",
			};

			static const char *s_deploy_targets[] =
			{
				"Windows",
				"Mac OS X",
				"Linux",
				"iOS",
				"Android",
				"Windows Mobile",
				"Linux Mobile",
				"Server",
				"iOS Embedded",
				"Android Embedded",
			};

			ep . clear();
			ep . concatcstring(MClicenseparameters . license_name, EC_RETURN, true);
			ep . concatcstring(MClicenseparameters . license_organization, EC_RETURN, false);
			ep . concatcstring(s_class_types[MClicenseparameters . license_class], EC_RETURN, false);
			ep . concatuint(MClicenseparameters . license_multiplicity, EC_RETURN, false);
			
			ep . appendnewline();
			if (MClicenseparameters . deploy_targets != 0)
			{
				bool t_first;
				t_first = true;
				for(uint32_t i = 0; i < 9; i++)
				{
					if ((MClicenseparameters . deploy_targets & (1 << i)) != 0)
					{
						ep . concatcstring(s_deploy_targets[i], EC_COMMA, t_first);
						t_first = false;
					}
				}
			}

			ep . appendnewline();
			if (MClicenseparameters . addons != nil)
			{
				MCAutoStringRef t_keys;
				/* UNCHECKED */ MCArrayListKeys(MClicenseparameters . addons, ',', &t_keys);
				/* UNCHECKED */ ep . concatstringref(*t_keys, EC_RETURN, false);
			}

			ep . concatcstring(MCnullmcstring == MClicenseparameters . license_token ? "Global" : "Local", EC_RETURN, false);
		}
		else
		{
			if (MClicenseparameters . addons == nil ||
				!ep . fetcharrayelement_oldstring(MClicenseparameters . addons, ep . getsvalue()))
				ep . clear();
		}
	}
	break;
#ifdef FEATURE_PROPERTY_LISTENER
	// MM-2012-09-05: [[ Property Listener ]]
	case P_REV_OBJECT_LISTENERS:
		MCInternalObjectListenerListListeners(ep);
		break;
	case P_REV_PROPERTY_LISTENER_THROTTLE_TIME:
		ep.setnvalue(MCpropertylistenerthrottletime);
		break;
#endif			
	default:
		return ES_NOT_HANDLED;
	}

	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of mode hooks for DEVELOPMENT mode.
//

// All stacks can be saved in development mode, so this is a no-op.
IO_stat MCModeCheckSaveStack(MCStack *stack, const MCStringRef p_filename)
{
	return IO_NORMAL;
}

// For development mode, the environment depends on the license edition
MCNameRef MCModeGetEnvironment(void)
{
	return MCN_development;
}

uint32_t MCModeGetEnvironmentType(void)
{
	return kMCModeEnvironmentTypeEditor;
}


// In development mode, we are always licensed.
bool MCModeGetLicensed(void)
{
	return true;
}

// In development mode, we don't want the executable added as an argument.
bool MCModeIsExecutableFirstArgument(void)
{
	return false;
}

// In development mode, we always automatically open stacks.
bool MCModeShouldLoadStacksOnStartup(void)
{
	return true;
}

// In development mode, we just issue a generic error.
void MCModeGetStartupErrorMessage(MCStringRef& r_caption, MCStringRef& r_text)
{
	r_caption = MCSTR("Initialization Error");
	r_text = MCSTR("Error while initializing development environment");
}

// In development mode, we can set any object's script.
bool MCModeCanSetObjectScript(uint4 obj_id)
{
	return true;
}

// In development mode, we don't check the old CANT_STANDALONE flag.
bool MCModeShouldCheckCantStandalone(void)
{
	return false;
}

bool MCModeHandleMessageBoxChanged(MCExecPoint& ep)
{
	// IM-2013-04-16: [[ BZ 10836 ]] update revMessageBoxLastObject
	// if the source of the change is not within the message box
	MCObject *t_msg_box = nil;
	if (MCmessageboxredirect != nil)
		t_msg_box = MCmessageboxredirect;
	else
	{
		if (MCmbstackptr == nil)
			MCmbstackptr = MCdispatcher->findstackname(MCN_messagename);
		t_msg_box = MCmbstackptr;
	}
	
	MCObject *t_src_object = nil;
	if (ep.getobj() != nil)
		t_src_object = ep.getobj();
	
	bool t_in_msg_box = false;
	
	MCObject *t_obj_ptr = t_src_object;
	while (t_obj_ptr != nil)
	{
		if (t_obj_ptr == t_msg_box)
		{
			t_in_msg_box = true;
			break;
		}
		t_obj_ptr = t_obj_ptr->getparent();
	}
	
	if (!t_in_msg_box)
	{
		if (MCmessageboxlastobject != nil)
			MCmessageboxlastobject->Release();
		MCmessageboxlastobject = t_src_object->gethandle();
		
		MCNameDelete(MCmessageboxlasthandler);
		MCmessageboxlasthandler = nil;
		MCNameClone(ep.gethandler()->getname(), MCmessageboxlasthandler);
		
		MCmessageboxlastline = ep.getline();
	}
	
	if (MCmessageboxredirect != NULL)
	{
		if (MCmessageboxredirect -> gettype() == CT_FIELD)
		{
			MCStack *t_msg_stack;
			t_msg_stack = MCmessageboxredirect -> getstack();
			Window_mode newmode = t_msg_stack -> userlevel() == 0 ? WM_MODELESS
										: (Window_mode)(t_msg_stack -> userlevel() + WM_TOP_LEVEL_LOCKED);
			
			// MW-2011-07-05: [[ Bug 9608 ]] The 'ep' that is passed through to us does
			//   not necessarily have an attached object any more. Given that the 'rel'
			//   parameter of the open stack call is unused, computing it from that
			//   context is redundent.
			if (t_msg_stack -> getmode() != newmode)
				t_msg_stack -> openrect(t_msg_stack -> getrect(), newmode, NULL, WP_DEFAULT, OP_NONE);
			else
				t_msg_stack -> raise();

			MCAutoStringRef t_string;
			ep . copyasstringref(&t_string);
			((MCField *)MCmessageboxredirect) -> settext(0, *t_string, False);
		}
		else
		{
			MCAutoNameRef t_msg_changed;
			/* UNCHECKED */ t_msg_changed . CreateWithCString("msgchanged");
			
			bool t_added;
			if (MCnexecutioncontexts < MAX_CONTEXTS && ep . getobj() != nil)
			{
				MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
				t_added = true;
			}
			
			MCmessageboxredirect -> message(t_msg_changed);
		
			if (t_added)
				MCnexecutioncontexts--;
		}

		return true;
	}

	return false;
}

bool MCModeHandleRelaunch(MCStringRef & r_id)
{
	r_id = MCSTR("LiveCodeTools");
	return true;
}

const char *MCModeGetStartupStack(void)
{
	return NULL;
}

bool MCModeCanLoadHome(void)
{
	return true;
}

MCStatement *MCModeNewCommand(int2 which)
{
	switch(which)
	{
	case S_INTERNAL:
		return new MCInternal;
	case S_REV_RELICENSE:
		return new MCRevRelicense;
	default:
		break;
	}

	return NULL;
}

MCExpression *MCModeNewFunction(int2 which)
{
	return NULL;
}

void MCModeObjectDestroyed(MCObject *object)
{
	if (MCmessageboxredirect == object)
		MCmessageboxredirect = NULL;
}

bool MCModeShouldQueueOpeningStacks(void)
{
	return MCscreen == NULL || MCenvironmentactive;
}

bool MCModeShouldPreprocessOpeningStacks(void)
{
	return true;
}

Window MCModeGetParentWindow(void)
{
	Window t_window;
	t_window = MCdefaultstackptr -> getwindow();
	if (t_window == DNULL && MCtopstackptr != NULL)
		t_window = MCtopstackptr -> getwindow();
	return t_window;
}

bool MCModeCanAccessDomain(MCStringRef p_name)
{
	return false;
}

void MCModeQueueEvents(void)
{
#ifdef FEATURE_PROPERTY_LISTENER
	// MM-2012-09-05: [[ Property Listener ]]
	MCInternalObjectListenerMessagePendingListeners();
	
	// MW-2013-03-20: [[ MainStacksChanged ]]
	if (MCmainstackschanged)
	{
		MCdefaultstackptr -> message(MCM_main_stacks_changed);
		MCmainstackschanged = False;
	}
#endif
}

Exec_stat MCModeExecuteScriptInBrowser(MCStringRef p_script)
{
	MCeerror -> add(EE_ENVDO_NOTSUPPORTED, 0, 0);
	return ES_ERROR;
}

bool MCModeMakeLocalWindows(void)
{
	return true;
}

void MCModeActivateIme(MCStack *p_stack, bool p_activate)
{
	MCscreen -> activateIME(p_activate);
}

void MCModeConfigureIme(MCStack *p_stack, bool p_enabled, int32_t x, int32_t y)
{
	if (!p_enabled)
		MCscreen -> clearIME(p_stack -> getwindow());
}

void MCModeShowToolTip(int32_t x, int32_t y, uint32_t text_size, uint32_t bg_color, MCStringRef text_font, MCStringRef message)
{
}

void MCModeHideToolTip(void)
{
}

void MCModeResetCursors(void)
{
	MCscreen -> resetcursors();
}

bool MCModeCollectEntropy(void)
{
	return true;
}

// The IDE engine has its home stack sit in the message path for all stacks so
// we return true here.
bool MCModeHasHomeStack(void)
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of remote dialog methods
//

void MCRemoteFileDialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint32_t p_type_count, MCStringRef p_initial_folder, MCStringRef p_initial_file, bool p_save, bool p_files, MCStringRef &r_value)
{
}

void MCRemoteColorDialog(MCStringRef p_title, uint32_t p_red, uint32_t p_green, uint32_t p_blue, bool& r_chosen, MCColor& r_chosen_color)
{
}

void MCRemoteFolderDialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef &r_value)
{
}

void MCRemotePrintSetupDialog(MCDataRef p_config_data, MCDataRef &r_reply_data, uint32_t &r_result)
{
}

void MCRemotePageSetupDialog(MCDataRef p_config_data, MCDataRef &r_reply_data, uint32_t &r_result)
{
}

#ifdef _MACOSX
uint32_t MCModePopUpMenu(MCMacSysMenuHandle p_menu, int32_t p_x, int32_t p_y, uint32_t p_index, MCStack *p_stack)
{
	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of Windows-specific mode hooks for DEVELOPMENT mode.
//

#ifdef TARGET_PLATFORM_WINDOWS

#include <dbghelp.h>

typedef BOOL (WINAPI *MiniDumpWriteDumpPtr)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);

LONG WINAPI unhandled_exception_filter(struct _EXCEPTION_POINTERS *p_exception_info)
{
	if (MCcrashreportfilename == NULL)
		return EXCEPTION_EXECUTE_HANDLER;

	HMODULE t_dbg_help_module = NULL;
	t_dbg_help_module = LoadLibraryA("dbghelp.dll");

	MiniDumpWriteDumpPtr t_write_minidump;
	if (t_dbg_help_module != NULL)
		t_write_minidump = (MiniDumpWriteDumpPtr)GetProcAddress(t_dbg_help_module, "MiniDumpWriteDump");

	char *t_path = NULL;
	if (t_write_minidump != NULL)
		t_path = MCS_resolvepath(MCcrashreportfilename);

	HANDLE t_file = NULL;
	if (t_path != NULL)
		t_file = CreateFileA(t_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

	BOOL t_minidump_written = FALSE;
	if (t_file != NULL)
	{
		MINIDUMP_EXCEPTION_INFORMATION t_info;
		t_info . ThreadId = GetCurrentThreadId();
		t_info . ExceptionPointers = p_exception_info;
		t_info . ClientPointers = FALSE;
		t_minidump_written = t_write_minidump(GetCurrentProcess(), GetCurrentProcessId(), t_file, MCcrashreportverbose ? MiniDumpWithDataSegs : MiniDumpNormal, &t_info, NULL, NULL);
	}

	if (t_file != NULL)
		CloseHandle(t_file);

	if (t_path != NULL)
		delete t_path;
	
	if (t_dbg_help_module != NULL)
		FreeLibrary(t_dbg_help_module);

	return EXCEPTION_EXECUTE_HANDLER;
}

void MCModeSetupCrashReporting(void)
{
	SetUnhandledExceptionFilter(unhandled_exception_filter);
}

bool MCModeHandleMessage(LPARAM lparam)
{
	return false;
}

typedef BOOL (WINAPI *AttachConsolePtr)(DWORD id);
void MCModePreMain(void)
{
	HMODULE t_kernel;
	t_kernel = LoadLibraryA("kernel32.dll");
	if (t_kernel != nil)
	{
		void *t_attach_console;
		t_attach_console = GetProcAddress(t_kernel, "AttachConsole");
		if (t_attach_console != nil)
		{
			((AttachConsolePtr)t_attach_console)(-1);
			return;
		}
	}
}

bool MCPlayer::mode_avi_closewindowonplaystop()
{
	return true;
}

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of Mac OS X-specific mode hooks for DEVELOPMENT mode.
//

#ifdef _MACOSX

bool MCModePreWaitNextEvent(Boolean anyevent)
{
	return false;
}

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of Linux-specific mode hooks for DEVELOPMENT mode.
//

#ifdef _LINUX

void MCModePreSelectHook(int& maxfd, fd_set& rfds, fd_set& wfds, fd_set& efds)
{
}

void MCModePostSelectHook(fd_set& rfds, fd_set& wfds, fd_set& efds)
{
}

#endif
