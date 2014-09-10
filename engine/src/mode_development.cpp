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

//#include "execpt.h"
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
#include "chunk.h"
#include "scriptpt.h"

#include "resolution.h"
#include "group.h"

#if defined(_WINDOWS_DESKTOP)
#include "w32prefix.h"
#include "w32dc.h"
#include "w32compat.h"

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
#ifdef LEGACY_EXEC
void MCInternalObjectListenerListListeners(MCExecPoint &ep);
#endif
void MCInternalObjectListenerGetListeners(MCExecContext& ctxt, MCStringRef*& r_listeners, uindex_t& r_count);
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
MCStringRef MCcrashreportfilename = nil;

////////////////////////////////////////////////////////////////////////////////
//
//  Property tables specific to DEVELOPMENT mode
//

MCPropertyInfo MCObject::kModeProperties[] =
{
	DEFINE_RO_OBJ_NON_EFFECTIVE_LIST_PROPERTY(P_REV_AVAILABLE_HANDLERS, LinesOfString, MCObject, RevAvailableHandlers)
	DEFINE_RO_OBJ_EFFECTIVE_LIST_PROPERTY(P_REV_AVAILABLE_HANDLERS, LinesOfString, MCObject, RevAvailableHandlers)
    DEFINE_RO_OBJ_ARRAY_PROPERTY(P_REV_AVAILABLE_VARIABLES, String, MCObject, RevAvailableVariables)
    DEFINE_RO_OBJ_PROPERTY(P_REV_AVAILABLE_VARIABLES, String, MCObject, RevAvailableVariablesNonArray)
};

MCObjectPropertyTable MCObject::kModePropertyTable =
{
	nil,
	sizeof(kModeProperties) / sizeof(kModeProperties[0]),
	&kModeProperties[0],
};

MCPropertyInfo MCStack::kModeProperties[] =
{
    DEFINE_RW_OBJ_PROPERTY(P_IDE_OVERRIDE, Bool, MCStack, IdeOverride)
    DEFINE_RO_OBJ_PROPERTY(P_REFERRING_STACK, String, MCStack, ReferringStack)
    DEFINE_RO_OBJ_LIST_PROPERTY(P_UNPLACED_GROUP_IDS, LinesOfUInt, MCStack, UnplacedGroupIds)
};

MCObjectPropertyTable MCStack::kModePropertyTable =
{
    &MCObject::kModePropertyTable,
	sizeof(kModeProperties) / sizeof(kModeProperties[0]),
	&kModeProperties[0],
};

MCPropertyInfo MCProperty::kModeProperties[] =
{    
	DEFINE_RW_PROPERTY(P_REV_CRASH_REPORT_SETTINGS, Array, Mode, RevCrashReportSettings)
    DEFINE_RO_PROPERTY(P_REV_MESSAGE_BOX_LAST_OBJECT, String, Mode, RevMessageBoxLastObject)
    DEFINE_RW_PROPERTY(P_REV_MESSAGE_BOX_REDIRECT, String, Mode, RevMessageBoxRedirect)
	DEFINE_RO_ARRAY_PROPERTY(P_REV_LICENSE_INFO, String, Mode, RevLicenseInfo)
    DEFINE_RO_PROPERTY(P_REV_LICENSE_INFO, String, Mode, RevLicenseInfo)
    DEFINE_RW_PROPERTY(P_REV_LICENSE_LIMITS, Array, Mode, RevLicenseLimits)
    DEFINE_RO_PROPERTY(P_REV_OBJECT_LISTENERS, LinesOfString, Mode, RevObjectListeners)
    DEFINE_RW_PROPERTY(P_REV_PROPERTY_LISTENER_THROTTLE_TIME, UInt32, Mode, RevPropertyListenerThrottleTime)
};

MCPropertyTable MCProperty::kModePropertyTable =
{
	sizeof(kModeProperties) / sizeof(kModeProperties[0]),
	&kModeProperties[0],
};

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
	virtual void exec_ctxt(MCExecContext &);
};

static MCStringRef s_command_path = nil;

static void restart_revolution(void)
{
#if defined(TARGET_PLATFORM_WINDOWS)
    MCAutoStringRefAsUTF8String t_command_path;
    t_command_path . Lock(s_command_path);
	_spawnl(_P_NOWAIT, *t_command_path, *t_command_path, NULL);
#elif defined(TARGET_PLATFORM_MACOS_X) || defined(TARGET_PLATFORM_LINUX)
	if (fork() == 0)
	{
        MCAutoStringRefAsUTF8String t_mccmd;
        t_mccmd . Lock(MCcmd);
		usleep(250000);
		execl(*t_mccmd, *t_mccmd, NULL);
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

void MCRevRelicense::exec_ctxt(MCExecContext& ctxt)
{
	switch(MCdefaultstackptr -> getcard() -> message(MCM_shut_down_request))
	{
	case ES_PASS:
	case ES_NOT_HANDLED:
		break;
		
	default:
        ctxt . SetTheResultToCString("cancelled");
        return;
	}
	
    if (MClicenseparameters . license_token == NULL || MCStringIsEmpty(MClicenseparameters . license_token))
	{
		ctxt . SetTheResultToCString("no token");
		return;
	}

    if (!MCS_unlink(MClicenseparameters . license_token))
	{
		ctxt . SetTheResultToCString("token deletion failed");
		return;
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
	
    char *t_startdir_cstring, *t_mccmd;
    /* UNCHECKED */ MCStringConvertToCString(*t_startdir, t_startdir_cstring);
    /* UNCHECKED */ MCStringConvertToCString(MCcmd, t_mccmd);
    startdir = t_startdir_cstring;
    enginedir = t_mccmd;


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
    
    IO_handle stream = MCS_fakeopen(MCDataGetBytePtr(t_decompressed), MCDataGetLength(t_decompressed));
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
        MCExecContext ctxt(nil, nil, nil);
        MCValueRef t_valueref;
        t_valueref = nil;
        MCValueRef t_valueref2;
        t_valueref2 = nil;
		MCresult -> eval(ctxt, t_valueref);
		
		if (MCValueIsEmpty(t_valueref))
		{
			sptr -> open();
			MCImage::init();
			
			X_main_loop();
			MCresult -> eval(ctxt, t_valueref2);
			if (MCValueIsEmpty(t_valueref2))
            {
                MCValueRelease(t_valueref);
                MCValueRelease(t_valueref2);
				return IO_NORMAL;
            }
            else
                MCValueAssign(t_valueref, t_valueref2);
                
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
        ctxt . ConvertToName(t_valueref, &t_name);

		sptr = findstackname(*t_name);
        if (t_valueref != nil)
            MCValueRelease(t_valueref);
        if (t_valueref2 != nil)
            MCValueRelease(t_valueref2);

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

#ifdef LEGACY_EXEC
Exec_stat MCStack::mode_getprop(uint4 parid, Properties which, MCExecPoint &ep, MCStringRef carray, Boolean effective)
{
#ifdef /* MCStack::mode_getprop */ LEGACY_EXEC
	switch(which)
	{
	case P_REFERRING_STACK:
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
#endif /* MCStack::mode_getprop */
    return ES_ERROR;
}
#endif


#ifdef LEGACY_EXEC
Exec_stat MCStack::mode_setprop(uint4 parid, Properties which, MCExecPoint &ep, MCStringRef cprop, MCStringRef carray, Boolean effective)
{
#ifdef /* MCStack::mode_setprop */ LEGACY_EXEC
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
#endif /* MCStack::mode_setprop */
    return ES_ERROR;
}
#endif

void MCStack::mode_load(void)
{
	// We introduce the notion of an 'IDE' stack - such a stack is set by giving it
	// a custom property 'ideOverride' with value true.
	if (props != NULL)
	{
		MCAutoNameRef t_ide_override_name;
		/* UNCHECKED */ t_ide_override_name . CreateWithCString("ideOverride");

		MClockmessages++;
        MCExecValue t_value;
        MCExecContext ctxt(nil, nil, nil);
        getcustomprop(ctxt, kMCEmptyName, t_ide_override_name, t_value);
		MClockmessages--;

		bool t_treat_as_ide;
        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value, kMCExecValueTypeBool, &t_treat_as_ide);
        if (!ctxt . HasError() && t_treat_as_ide)
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

////////////////////////////////////////////////////////////////////////////////

#ifdef LEGACY_EXEC
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
#endif

#ifdef LECACY_EXEC
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
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of MCObject::getmodeprop for DEVELOPMENT mode.
//

// IM-2014-02-25: [[ Bug 11841 ]] MCObjectList helper functions
bool MCObjectListAppend(MCObjectList *&x_list, MCObject *p_object, bool p_unique)
{
	if (p_unique && x_list != nil)
	{
		MCObjectList *t_object;
		t_object = x_list;
		
		do
		{
			if (t_object->getobject() == p_object)
				return true;
			t_object = t_object->next();
		}
		while (t_object != x_list);
	}

	MCObjectList *t_newobject;
	t_newobject = nil;
	
	t_newobject = new MCObjectList(p_object);
	
	if (t_newobject == nil)
		return false;
	
	if (x_list == nil)
		x_list = t_newobject;
	else
		x_list->append(t_newobject);
	
	return true;
}

bool MCObjectListAppend(MCObjectList *&x_list, MCObjectList *p_list, bool p_unique)
{
	bool t_success;
	t_success = true;
	
	if (p_list != nil)
	{
		MCObjectList *t_object;
		t_object = p_list;
		
		do
		{
			if (!t_object->getremoved())
				t_success = MCObjectListAppend(x_list, t_object->getobject(), p_unique);
			t_object = t_object->next();
		}
		while (t_success && t_object != p_list);
	}
	
	return t_success;
}

void MCObjectListFree(MCObjectList *p_list)
{
	if (p_list == nil)
		return;
	
	while (p_list->next() != p_list)
		delete p_list->next();
	
	delete p_list;
}

#ifdef LEGACY_EXEC
Exec_stat MCObject::mode_getprop(uint4 parid, Properties which, MCExecPoint &ep, const MCString &carray, Boolean effective)
{
#ifdef /* MCObject::mode_getprop */ LEGACY_EXEC
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
            // MW-2014-07-25: [[ Bug 12819 ]] Make sure we don't list handlers of passworded stacks.
            if (getstack() -> iskeyed())
            {
                parsescript(False);
                if (hlist != NULL)
                    t_first = hlist -> enumerate(ep, t_first);
            }
		}
		else
		{
			bool t_success;
			t_success = true;
			
			// IM-2014-02-25: [[ Bug 11841 ]] Collect non-repeating objects in the message path
			MCObjectList *t_object_list;
			t_object_list = nil;
			
			// MM-2013-09-10: [[ Bug 10634 ]] Make we search both parent scripts and library stacks for handlers.
			t_success = MCObjectListAppend(t_object_list, MCfrontscripts, true);
			

			for (MCObject *t_object = this; t_success && t_object != NULL; t_object = t_object -> parent)
			{
				t_object -> parsescript(False);
				t_success = MCObjectListAppend(t_object_list, t_object, true);
			}

			if (t_success)
				t_success = MCObjectListAppend(t_object_list, MCbackscripts, true);

			for (uint32_t i = 0; t_success && i < MCnusing; i++)
			{
				if (MCusing[i] == this)
					continue;
				t_success = MCObjectListAppend(t_object_list, MCusing[i], true);
			}
			
			// IM-2014-02-25: [[ Bug 11841 ]] Enumerate the handlers for each object
			if (t_success)
				t_first = enumerate_handlers_for_list(t_object_list, nil, ep, t_first);
			
			MCObjectListFree(t_object_list);
		}
	}
	break;

	// OK-2008-04-23 : Added for script editor
	case P_REV_AVAILABLE_VARIABLES:
	{
		ep.clear();
        // MW-2014-07-25: [[ Bug 12819 ]] Make sure we don't list variables of passworded stacks.
		if (hlist == NULL || !getstack() -> iskeyed())
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

		uindex_t t_comma_pos;
        if (!MCStringFirstIndexOfChar(carray, ',', 0, kMCCompareExact, t_comma_pos))
		{
			return ES_NORMAL;
		}

		// The handler name begins after the comma character
		uindex_t t_handler_name_pos;
        t_handler_name_pos = t_comma_pos + 1;

		// The handler code must be the first char of the string
		char t_handler_code;
		t_handler_code = MCStringGetNativeCharAtIndex(carray, 0);
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
        
        MCAutoStringRef t_handler_substring;
        MCStringCopySubstring(carray, MCRangeMake(t_handler_name_pos, MCStringGetLength(carray) - t_handler_name_pos), &t_handler_substring);

		MCAutoNameRef t_handler_name;
		MCNameCreate(*t_handler_substring, t_handler_name);

		Exec_stat t_status;

		// The handler list class allows us to locate the handler, just return empty if it can't be found.
		MCHandler *t_handler;
		t_status = hlist -> findhandler(t_handler_type, t_handler_name, t_handler);
		if (t_status != ES_NORMAL)
		{
			return ES_NORMAL;
		}

		if (t_handler != NULL)
			t_handler -> getvarnames(ep, true);

		return ES_NORMAL;
	}
	break;

	default:
		return ES_NOT_HANDLED;
	}

	return ES_NORMAL;
#endif /* MCObject::mode_getprop */
    return ES_ERROR;
}
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of MCProperty::mode_eval/mode_set for DEVELOPMENT mode.
//

#ifdef LEGACY_EXEC
Exec_stat MCProperty::mode_set(MCExecPoint& ep)
{
#ifdef /* MCProperty::mode_set */ LEGACY_EXEC
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
#endif /* MCProperty::mode_set */
    return ES_ERROR;
}
#endif


#ifdef LEGACY_EXEC
Exec_stat MCProperty::mode_eval(MCExecPoint& ep)
{
#ifdef /* MCProperty::mode_eval */ LEGACY_EXEC
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
#endif /* MCProperty::mode_eval */
    return ES_ERROR;
}
#endif

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

bool MCModeHandleMessageBoxChanged(MCExecContext& ctxt, MCStringRef p_string)
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
	if (ctxt.GetObject() != nil)
		t_src_object = ctxt.GetObject();
	
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
		MCNameClone(ctxt.GetHandler()->getname(), MCmessageboxlasthandler);
		
        MCmessageboxlastline = ctxt . GetLine();
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

			((MCField *)MCmessageboxredirect) -> settext(0, p_string, False);
		}
		else
		{
			MCAutoNameRef t_msg_changed;
			/* UNCHECKED */ t_msg_changed . CreateWithCString("msgchanged");
			
			bool t_added = false;
			if (MCnexecutioncontexts < MAX_CONTEXTS && ctxt.GetObject() != nil)
			{
				MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
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
	if (t_window == NULL && MCtopstackptr != NULL)
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
    else
        MCscreen -> configureIME(x, y);
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

	MiniDumpWriteDumpPtr t_write_minidump = NULL;
	if (t_dbg_help_module != NULL)
		t_write_minidump = (MiniDumpWriteDumpPtr)GetProcAddress(t_dbg_help_module, "MiniDumpWriteDump");

	char *t_path = NULL;
	if (t_write_minidump != NULL)
		t_path = MCS_resolvepath(MCStringGetCString(MCcrashreportfilename));

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

// IM-2014-08-08: [[ Bug 12372 ]] Allow IDE pixel scaling to be enabled / disabled
// on startup depending on the usePixelScaling registry value
bool MCModeGetPixelScalingEnabled()
{
    MCAutoStringRef t_type, t_error;
    MCAutoValueRef t_value;
	MCS_query_registry(MCSTR("HKEY_CURRENT_USER\\Software\\LiveCode\\IDE\\usePixelScaling"), &t_value, &t_type, &t_error);

	if (!MCresult->isempty())
	{
		MCresult->clear();
		return true;
	}

	// IM-2014-08-14: [[ Bug 12372 ]] PixelScaling is enabled by default.
	if (MCValueIsEmpty(*t_value))
        return true;
    
    bool t_result;
    MCExecContext ctxt(nil, nil, nil);
    if (!ctxt . ConvertToBool(*t_value, t_result))
        return false;
    
    return t_result;
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

////////////////////////////////////////////////////////////////////////////////
//
//  Refactored object property getters and setters for DEVELOPMENT mode.
//

void MCStack::GetReferringStack(MCExecContext& ctxt, MCStringRef& r_id)
{
    r_id = MCValueRetain(kMCEmptyString);
}

void MCStack::GetUnplacedGroupIds(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_ids)
{
    if (controls == nil)
    {
        r_count = 0;
        return;
    }

    MCControl *t_control;
    t_control = controls;
    
    MCAutoArray<uinteger_t> t_ids;
    
    do
    {
        if (t_control -> gettype() == CT_GROUP && t_control -> getparent() != curcard)
            t_ids . Push(t_control -> getid());
        
        t_control = t_control -> next();
    }
    while(t_control != controls);
    
    t_ids . Take(r_ids, r_count);
}

void MCStack::GetIdeOverride(MCExecContext& ctxt, bool& r_value)
{
    r_value = getextendedstate(ECS_IDE);
}

void MCStack::SetIdeOverride(MCExecContext& ctxt, bool p_value)
{
    setextendedstate(p_value, ECS_IDE);
}

void MCObject::GetRevAvailableHandlers(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_handlers)
{
    // MW-2010-07-09: [[ Bug 8848 ]] Previously scripts were being compiled into
    //   separate hlists causing script local variable loss in the behavior
    //   case. Instead we just use parsescript in non-reporting mode.
    
    parsescript(False);
    if (hlist == nil)
    {
        r_count = 0;
        return;
    }
    
    hlist -> enumerate(ctxt, true, r_count, r_handlers);
}

void MCObject::GetEffectiveRevAvailableHandlers(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_handlers)
{
    bool t_first;
    t_first = true;
    
    MCAutoArray<MCStringRef> t_handlers;
    
    // IM-2014-02-25: [[ Bug 11841 ]] Collect non-repeating objects in the message path
    MCObjectList *t_object_list;
    t_object_list = nil;
    
    bool t_success;
    t_success = true;
    
    // MM-2013-09-10: [[ Bug 10634 ]] Make we search both parent scripts and library stacks for handlers.
    t_success = MCObjectListAppend(t_object_list, MCfrontscripts, true);
    
    for (MCObject *t_object = this; t_success && t_object != NULL; t_object = t_object -> parent)
    {
        t_object -> parsescript(False);
        t_success = MCObjectListAppend(t_object_list, t_object, true);
    }
    
    if (t_success)
        t_success = MCObjectListAppend(t_object_list, MCbackscripts, true);
    
    for (uint32_t i = 0; t_success && i < MCnusing; i++)
    {
        if (MCusing[i] == this)
            continue;
        t_success = MCObjectListAppend(t_object_list, MCusing[i], true);
    }
    
    // IM-2014-02-25: [[ Bug 11841 ]] Enumerate the handlers for each object
    if (t_success)
    {
        MCObjectList *t_object_ref;
        t_object_ref = t_object_list;
        do
        {
            // AL-2014-05-23: [[ Bug 12491 ]] The object list checks for uniqueness,
            //  so no need to check if the object is itself a frontscript.
            
            t_first = true;
            MCHandlerlist *t_handler_list;
            
            if (!t_object_ref -> getremoved() && t_object_ref -> getobject() -> getstack() -> iskeyed())
                t_handler_list = t_object_ref -> getobject() -> hlist;
            else
                t_handler_list = NULL;
            
            if (t_handler_list != NULL)
            {
                MCStringRef *t_handler_array;
                t_handler_array = nil;
                uindex_t t_count;
                
                t_first = t_handler_list -> enumerate(ctxt, t_first, t_count, t_handler_array);
            
                for (uindex_t i = 0; i < t_count; i++)
                    t_handlers . Push(t_handler_array[i]);
                
                MCMemoryDeleteArray(t_handler_array);
            }
            
            t_object_ref = t_object_ref -> next();
        }
        while(t_object_ref != t_object_list);
    }
    
    MCObjectListFree(t_object_list);
     
    t_handlers . Take(r_handlers, r_count);
}

void MCObject::GetRevAvailableVariablesNonArray(MCExecContext& ctxt, MCStringRef& r_variables)
{
    GetRevAvailableVariables(ctxt, nil, r_variables);
}

void MCObject::GetRevAvailableVariables(MCExecContext& ctxt, MCNameRef p_key, MCStringRef& r_variables)
{
    // OK-2008-04-23 : Added for script editor
    if (hlist == NULL)
    {
        r_variables = MCValueRetain(kMCEmptyString);
        return;
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
    if (p_key == nil)
    {
        MCAutoListRef t_list;
        if (!MCListCreateMutable('\n', &t_list))
            return;
        
        MCAutoListRef t_global_list, t_local_list;
        
        if (!(hlist->getlocalnames(&t_local_list) &&
              MCListAppend(*t_list, *t_local_list)))
        {
            ctxt . Throw();
            return;
        }
        
        if (!(hlist->getglobalnames(&t_global_list) &&
                MCListAppend(*t_list, *t_global_list)))
        {
            ctxt . Throw();
            return;
        }
        MCListCopyAsString(*t_list, r_variables);
        return;
    }

    
    MCStringRef t_key;
    t_key = MCNameGetString(p_key);
    
    // The handler name begins after the comma character
    MCAutoStringRef t_handler_name;
    uindex_t t_comma_offset;
    if (!MCStringFirstIndexOfChar(t_key, ',', 0, kMCCompareExact, t_comma_offset))
    {
        r_variables = MCValueRetain(kMCEmptyString);
        return;
    }
    
    if (!MCStringCopySubstring(t_key, MCRangeMake(t_comma_offset + 1, MCStringGetLength(t_key) - t_comma_offset - 1), &t_handler_name))
    {
        ctxt . Throw();
        return;
    }
    
    // The handler code must be the first char of the string
    const char_t t_handler_code = MCStringGetNativeCharAtIndex(t_key, 0);
    
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
    
    Exec_stat t_status;
    MCNewAutoNameRef t_name;
    MCNameCreate(*t_handler_name, &t_name);
    // The handler list class allows us to locate the handler, just return empty if it can't be found.
    MCHandler *t_handler;
    t_status = hlist -> findhandler(t_handler_type, *t_name, t_handler);

    if (t_handler != NULL)
    {
        MCAutoListRef t_list;
        t_handler -> getvarnames(true, &t_list);
        MCListCopyAsString(*t_list, r_variables);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//  Refactored global property getters and setters for DEVELOPMENT mode.
//

void MCModeSetRevCrashReportSettings(MCExecContext& ctxt, MCArrayRef p_settings)
{
    bool t_case_sensitive = ctxt . GetCaseSensitive();
    MCValueRef t_verbose, t_filename;
    if (MCArrayFetchValue(p_settings, t_case_sensitive, MCNAME("verbose"), t_verbose))
        MCcrashreportverbose = (MCBooleanRef)t_verbose == kMCTrue;
    
    if (MCArrayFetchValue(p_settings, t_case_sensitive, MCNAME("filename"), t_filename))
    {
        if (MCcrashreportfilename != nil)
            MCValueRelease(MCcrashreportfilename);
        if (MCStringIsEmpty((MCStringRef)t_filename))
            MCcrashreportfilename = nil;
        else
            MCcrashreportfilename = MCValueRetain((MCStringRef)t_filename);
    }
}

void MCModeSetRevLicenseLimits(MCExecContext& ctxt, MCArrayRef p_settings)
{
    if(!MCenvironmentactive)
        return;
    
    bool t_case_sensitive = ctxt . GetCaseSensitive();
    MCValueRef t_value;
    MCStringRef t_string;
    if (MCArrayFetchValue(p_settings, t_case_sensitive, MCNAME("token"), t_value)
            && ctxt . ConvertToString(t_value, t_string))
    {
        MCValueRelease(MClicenseparameters . license_token);
        MClicenseparameters . license_token = t_string;
    }
    
    if (MCArrayFetchValue(p_settings, t_case_sensitive, MCNAME("name"), t_value)
            && ctxt . ConvertToString(t_value, t_string))
    {
        MCValueRelease(MClicenseparameters . license_name);
        MClicenseparameters . license_name = t_string;
    }
    
    if (MCArrayFetchValue(p_settings, t_case_sensitive, MCNAME("organization"), t_value)
            && ctxt . ConvertToString(t_value, t_string))
    {
        MCValueRelease( MClicenseparameters . license_organization);
         MClicenseparameters . license_organization = t_string;
    }
    
    if (MCArrayFetchValue(p_settings, t_case_sensitive, MCNAME("class"), t_value))
    {
        MCAutoStringRef t_class;
        if (ctxt . ConvertToString(t_value, &t_class))
        {
            static struct { const char *tag; uint32_t value; } s_class_map[] =
            {
                { "community", kMCLicenseClassCommunity },
                { "commercial", kMCLicenseClassCommercial },
                { "professional", kMCLicenseClassProfessional },
                { "", kMCLicenseClassNone }
            };
            
            uint4 t_index;
            for(t_index = 0; s_class_map[t_index] . tag != NULL; ++t_index)
                if (MCStringIsEqualToCString(*t_class, s_class_map[t_index] . tag, kMCCompareCaseless))
                    break;
            
            MClicenseparameters . license_class = s_class_map[t_index] . value;
        }
        else
            MClicenseparameters . license_class = kMCLicenseClassNone;
    }
    
    if (MCArrayFetchValue(p_settings, t_case_sensitive, MCNAME("multiplicity"), t_value))
        MClicenseparameters . license_multiplicity = MCNumberFetchAsUnsignedInteger((MCNumberRef)t_value);
    
    if (MCArrayFetchValue(p_settings, t_case_sensitive, MCNAME("scriptlimit"), t_value))
    {
        integer_t t_limit;
        t_limit = MCNumberFetchAsInteger((MCNumberRef)t_value);
        MClicenseparameters . script_limit = t_limit <= 0 ? 0 : t_limit;
    }
    
    if (MCArrayFetchValue(p_settings, t_case_sensitive, MCNAME("dolimit"), t_value))
    {
        integer_t t_limit;
        t_limit = MCNumberFetchAsInteger((MCNumberRef)t_value);
        MClicenseparameters . do_limit = t_limit <= 0 ? 0 : t_limit;
    }
    
    if (MCArrayFetchValue(p_settings, t_case_sensitive, MCNAME("usinglimit"), t_value))
    {
        integer_t t_limit;
        t_limit = MCNumberFetchAsInteger((MCNumberRef)t_value);
        MClicenseparameters . using_limit = t_limit <= 0 ? 0 : t_limit;
    }
    
    if (MCArrayFetchValue(p_settings, t_case_sensitive, MCNAME("insertlimit"), t_value))
    {
        integer_t t_limit;
        t_limit = MCNumberFetchAsInteger((MCNumberRef)t_value);
        MClicenseparameters . insert_limit = t_limit <= 0 ? 0 : t_limit;
    }
    
    if (MCArrayFetchValue(p_settings, t_case_sensitive, MCNAME("deploy"), t_value))
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
        
        MCAutoStringRef t_params;
        if (ctxt . ConvertToString(t_value, &t_params))
        {
            MCAutoArrayRef t_split_strings;
            MCValueRef t_fetched_string;
            if (MCStringSplit(*t_params, MCSTR(","), nil, kMCCompareExact, &t_split_strings))
            {
                for(uint32_t i = 0; i < MCArrayGetCount(*t_split_strings); i++)
                {
                    // Fetch the string value created with MCStringSplit
                    MCArrayFetchValueAtIndex(*t_split_strings, i+1, t_fetched_string);
                    
                    for(uint32_t j = 0; j < sizeof(s_deploy_map) / sizeof(s_deploy_map[0]); j++)
                        if (MCStringIsEqualToCString((MCStringRef)t_fetched_string, s_deploy_map[j] . tag, kMCStringOptionCompareCaseless))
                        {
                            MClicenseparameters . deploy_targets |= s_deploy_map[j] . value;
                            break;
                        }
                }
            }
        }
    }
    
    if (MCArrayFetchValue(p_settings, t_case_sensitive, MCNAME("addons"), t_value) && MCValueIsArray(t_value))
    {
        MCValueRelease(MClicenseparameters . addons);
        MCArrayCopy((MCArrayRef)t_value, MClicenseparameters . addons);
    }
}

static MCObject *getobj(MCExecContext& ctxt, MCStringRef p_string)
{    
    MCObject *objptr = NULL;
    MCChunk *tchunk = new MCChunk(False);
    MCerrorlock++;
    MCScriptPoint sp(p_string);
    if (tchunk->parse(sp, False) == PS_NORMAL)
    {
        uint4 parid;
        tchunk->getobj(ctxt, objptr, parid, True);
    }
    MCerrorlock--;
    delete tchunk;
    return objptr;
}

void MCModeSetRevMessageBoxRedirect(MCExecContext& ctxt, MCStringRef p_target)
{
    MCObject *t_object;
    t_object = getobj(ctxt, p_target);
    if (t_object != NULL)
        MCmessageboxredirect = t_object;
    else
        MCmessageboxredirect = NULL;
}
            
void MCModeSetRevPropertyListenerThrottleTime(MCExecContext& ctxt, uinteger_t p_time)
{
#ifdef FEATURE_PROPERTY_LISTENER
    // MM-2012-11-06: [[ Property Listener ]]
    MCpropertylistenerthrottletime = p_time;
#endif
}

void MCModeGetRevMessageBoxLastObject(MCExecContext& ctxt, MCStringRef& r_object)
{
    if (MCmessageboxlastobject != NULL && MCmessageboxlastobject->Exists())
    {
        bool t_success;

        MCAutoStringRef t_obj, t_long_id;
        MCAutoValueRef t_id_value;
        t_success = MCStringCreateMutable(0, &t_obj);
        
        if (t_success)
            t_success = MCmessageboxlastobject->Get()->names(P_LONG_ID, &t_id_value);
        if (t_success && ctxt . ConvertToString(*t_id_value, &t_long_id))
            t_success = MCStringAppendFormat(*t_obj, "%@,%@,%u", *t_long_id, MCNameGetString(MCmessageboxlasthandler), MCmessageboxlastline);

        if (t_success && MCmessageboxlastobject->Get()->getparentscript() != nil)
        {
            MCAutoStringRef t_long_id;
            MCAutoValueRef t_id_value;

            t_success = MCmessageboxlastobject->Get()->getparentscript()->GetObject()->names(P_LONG_ID, &t_id_value);
            if (t_success && ctxt . ConvertToString(*t_id_value, &t_long_id))
                t_success = MCStringAppendFormat(*t_obj, ",%@", *t_long_id);
        }
        if (t_success && MCStringCopy(*t_obj, r_object))
            return;
    }
    else
    {
        r_object = MCValueRetain(kMCEmptyString);
        return;
    }
    
    ctxt . Throw();
}

void MCModeGetRevMessageBoxRedirect(MCExecContext& ctxt, MCStringRef& r_id)
{
    if (MCmessageboxredirect != NULL)
    {
        MCAutoValueRef t_long_id;
        if (MCmessageboxredirect -> names(P_LONG_ID, &t_long_id) &&
            ctxt . ConvertToString(*t_long_id, r_id))
            return;
    }
    else
    {
        r_id = MCValueRetain(kMCEmptyString);
        return;
    }
    
    ctxt . Throw();
}

void MCModeGetRevLicenseLimits(MCExecContext& ctxt, MCArrayRef& r_limits)
{
    r_limits = MCValueRetain(kMCEmptyArray);
}

void MCModeGetRevCrashReportSettings(MCExecContext& ctxt, MCArrayRef& r_settings)
{
    r_settings = MCValueRetain(kMCEmptyArray);
}

void MCModeGetRevLicenseInfo(MCExecContext& ctxt, MCStringRef& r_info)
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
    
    bool t_success;
    
    MCAutoStringRef t_info;
    t_success = MCStringCreateMutable(0, &t_info);
    
    if (t_success)
        t_success = MCStringAppendFormat(*t_info, "%@\n%@\n%s\n%u\n", MClicenseparameters . license_name, MClicenseparameters . license_organization, s_class_types[MClicenseparameters . license_class], MClicenseparameters . license_multiplicity);
    
    if (MClicenseparameters . deploy_targets != 0)
    {
        bool t_first;
        t_first = true;
        for(uint32_t i = 0; t_success && i < 9; i++)
        {
            if ((MClicenseparameters . deploy_targets & (1 << i)) != 0)
            {
                t_success = MCStringAppendFormat(*t_info, t_first ? "%s" : ",%s", s_deploy_targets[i]);
                t_first = false;
            }
        }
    }
    
    if (t_success)
        t_success = MCStringAppendNativeChar(*t_info, '\n');
    
    if (t_success && MClicenseparameters . addons != nil)
    {
        MCAutoStringRef t_keys;
        t_success = (MCArrayListKeys(MClicenseparameters . addons, ',', &t_keys) &&
                    MCStringAppendFormat(*t_info, "\n%@", *t_keys));
    }
    
    if (t_success)
        if (MCStringAppendFormat(*t_info, "\n%s", MCStringIsEmpty(MClicenseparameters . license_token) ? "Global" : "Local") &&
                MCStringCopy(*t_info, r_info))
            return;
    
    ctxt . Throw();
}

void MCModeGetRevLicenseInfo(MCExecContext& ctxt, MCNameRef p_key, MCStringRef& r_info)
{
    MCValueRef t_value;
    if (MClicenseparameters . addons == nil ||
        !MCArrayFetchValue(MClicenseparameters . addons, ctxt . GetCaseSensitive(), p_key, t_value))
        {
            r_info = MCValueRetain(kMCEmptyString);
            return;
        }
    
    if (ctxt . ConvertToString(t_value, r_info))
        return;
    
    ctxt . Throw();
}

void MCModeGetRevObjectListeners(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_listeners)
{
#ifdef FEATURE_PROPERTY_LISTENER
    // MM-2012-09-05: [[ Property Listener ]]
    MCInternalObjectListenerGetListeners(ctxt, r_listeners, r_count);
#endif			
    r_count = 0;
}
void MCModeGetRevPropertyListenerThrottleTime(MCExecContext& ctxt, uinteger_t& r_time)
{
#ifdef FEATURE_PROPERTY_LISTENER
    r_time = MCpropertylistenerthrottletime;
#endif			
    r_time = 0;
}
