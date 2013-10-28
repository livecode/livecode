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
#include "external.h"
#include "osspec.h"
#include "flst.h"
#include "globals.h"
#include "license.h"
#include "mode.h"
#include "redraw.h"
#include "printer.h"
#include "font.h"
#include "stacksecurity.h"

#include "exec.h"
#include "exec-interface.h"
#include "graphics_util.h"

#define UNLICENSED_TIME 6.0
#ifdef _DEBUG_MALLOC_INC
#define LICENSED_TIME 1.0
#else
#define LICENSED_TIME 3.0
#endif

MCImage *MCDispatch::imagecache;

#define VERSION_OFFSET 11

#define HEADERSIZE 255
static char header[HEADERSIZE] = "#!/bin/sh\n# MetaCard 2.4 stack\n# The following is not ASCII text,\n# so now would be a good time to q out of more\f\nexec mc $0 \"$@\"\n";

#define NEWHEADERSIZE 8
static const char *newheader = "REVO2700";
static const char *newheader5500 = "REVO5500";

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCDispatch::kProperties[] =
{
	DEFINE_RO_OBJ_PROPERTY(P_TEXT_FONT, String, MCDispatch, DefaultTextFont)
    DEFINE_RO_OBJ_PROPERTY(P_TEXT_SIZE, UInt32, MCDispatch, DefaultTextSize)
	DEFINE_RO_OBJ_ENUM_PROPERTY(P_TEXT_ALIGN, InterfaceTextAlign, MCDispatch, DefaultTextAlign)
    DEFINE_RO_OBJ_CUSTOM_PROPERTY(P_TEXT_STYLE, InterfaceTextStyle, MCDispatch, DefaultTextStyle)
	DEFINE_RO_OBJ_PROPERTY(P_TEXT_HEIGHT, UInt32, MCDispatch, DefaultTextHeight)
    
    DEFINE_RO_OBJ_PROPERTY(P_FORE_PIXEL, UInt32, MCDispatch, DefaultForePixel)
    DEFINE_RO_OBJ_PROPERTY(P_HILITE_PIXEL, UInt32, MCDispatch, DefaultForePixel)
    DEFINE_RO_OBJ_PROPERTY(P_BORDER_PIXEL, UInt32, MCDispatch, DefaultForePixel)
    DEFINE_RO_OBJ_PROPERTY(P_BOTTOM_PIXEL, UInt32, MCDispatch, DefaultForePixel)
    DEFINE_RO_OBJ_PROPERTY(P_SHADOW_PIXEL, UInt32, MCDispatch, DefaultForePixel)
    DEFINE_RO_OBJ_PROPERTY(P_FOCUS_PIXEL, UInt32, MCDispatch, DefaultForePixel)
    
    DEFINE_RO_OBJ_PROPERTY(P_BACK_PIXEL, UInt32, MCDispatch, DefaultBackPixel)
    
    DEFINE_RO_OBJ_PROPERTY(P_TOP_PIXEL, UInt32, MCDispatch, DefaultTopPixel)
    
    DEFINE_RO_OBJ_CUSTOM_PROPERTY(P_FORE_COLOR, InterfaceNamedColor, MCDispatch, DefaultForeColor)
    DEFINE_RO_OBJ_CUSTOM_PROPERTY(P_BORDER_COLOR, InterfaceNamedColor, MCDispatch, DefaultForeColor)
    DEFINE_RO_OBJ_CUSTOM_PROPERTY(P_TOP_COLOR, InterfaceNamedColor, MCDispatch, DefaultForeColor)
    DEFINE_RO_OBJ_CUSTOM_PROPERTY(P_BOTTOM_COLOR, InterfaceNamedColor, MCDispatch, DefaultForeColor)
    DEFINE_RO_OBJ_CUSTOM_PROPERTY(P_SHADOW_COLOR, InterfaceNamedColor, MCDispatch, DefaultForeColor)
    DEFINE_RO_OBJ_CUSTOM_PROPERTY(P_FOCUS_COLOR, InterfaceNamedColor, MCDispatch, DefaultForeColor)
    
    DEFINE_RO_OBJ_CUSTOM_PROPERTY(P_BACK_COLOR, InterfaceNamedColor, MCDispatch, DefaultBackColor)
    DEFINE_RO_OBJ_CUSTOM_PROPERTY(P_HILITE_COLOR, InterfaceNamedColor, MCDispatch, DefaultBackColor)

    DEFINE_RO_OBJ_PROPERTY(P_FORE_PATTERN, OptionalUInt32, MCDispatch, DefaultPattern)
    DEFINE_RO_OBJ_PROPERTY(P_BACK_PATTERN, OptionalUInt32, MCDispatch, DefaultPattern)
    DEFINE_RO_OBJ_PROPERTY(P_HILITE_PATTERN, OptionalUInt32, MCDispatch, DefaultPattern)
    DEFINE_RO_OBJ_PROPERTY(P_BORDER_PATTERN, OptionalUInt32, MCDispatch, DefaultPattern)
    DEFINE_RO_OBJ_PROPERTY(P_TOP_PATTERN, OptionalUInt32, MCDispatch, DefaultPattern)
    DEFINE_RO_OBJ_PROPERTY(P_BOTTOM_PATTERN, OptionalUInt32, MCDispatch, DefaultPattern)
    DEFINE_RO_OBJ_PROPERTY(P_SHADOW_PATTERN, OptionalUInt32, MCDispatch, DefaultPattern)
    DEFINE_RO_OBJ_PROPERTY(P_FOCUS_PATTERN, OptionalUInt32, MCDispatch, DefaultPattern)
};

MCObjectPropertyTable MCDispatch::kPropertyTable =
{
	&MCObject::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCDispatch::MCDispatch()
{
	license = NULL;
	stacks = NULL;
	fonts = NULL;
	setname_cstring("dispatch");
	handling = False;
	menu = NULL;
	panels = NULL;
	startdir = NULL;
	enginedir = NULL;
	flags = 0;

	m_drag_source = false;
	m_drag_target = false;
	m_drag_end_sent = false;

	m_externals = nil;
}

MCDispatch::~MCDispatch()
{	
	delete license;
	while (stacks != NULL)
	{
		MCStack *sptr = stacks->prev()->remove(stacks);
		delete sptr;
	}
	while (imagecache != NULL)
	{
		MCImage *iptr = imagecache->remove(imagecache);
		delete iptr;
	}
	delete fonts;
	
	delete startdir;
	delete enginedir;
	
	delete m_externals;
}

bool MCDispatch::isdragsource(void)
{
	return m_drag_source;
}

bool MCDispatch::isdragtarget(void)
{
	return m_drag_target;
}

Exec_stat MCDispatch::getprop_legacy(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
	switch (which)
	{
#ifdef /* MCDispatch::getprop */ LEGACY_EXEC
	case P_BACK_PIXEL:
		ep.setint(MCscreen->background_pixel.pixel & 0xFFFFFF);
		return ES_NORMAL;
	case P_TOP_PIXEL:
		ep.setint(MCscreen->white_pixel.pixel & 0xFFFFFF);
		return ES_NORMAL;
	case P_HILITE_PIXEL:
	case P_FORE_PIXEL:
	case P_BORDER_PIXEL:
	case P_BOTTOM_PIXEL:
	case P_SHADOW_PIXEL:
	case P_FOCUS_PIXEL:
		ep.setint(MCscreen->black_pixel.pixel & 0xFFFFFF);
		return ES_NORMAL;
	case P_BACK_COLOR:
	case P_HILITE_COLOR:
		ep.setstaticcstring("white");
		return ES_NORMAL;
	case P_FORE_COLOR:
	case P_BORDER_COLOR:
	case P_TOP_COLOR:
	case P_BOTTOM_COLOR:
	case P_SHADOW_COLOR:
	case P_FOCUS_COLOR:
		ep.setstaticcstring("black");
		return ES_NORMAL;
	case P_FORE_PATTERN:
	case P_BACK_PATTERN:
	case P_HILITE_PATTERN:
	case P_BORDER_PATTERN:
	case P_TOP_PATTERN:
	case P_BOTTOM_PATTERN:
	case P_SHADOW_PATTERN:
	case P_FOCUS_PATTERN:
		ep.clear();
		return ES_NORMAL;
	case P_TEXT_ALIGN:
		ep.setstaticcstring(MCleftstring);
		return ES_NORMAL;
	case P_TEXT_FONT:
		ep.setstaticcstring(DEFAULT_TEXT_FONT);
		return ES_NORMAL;
	case P_TEXT_HEIGHT:
		ep.setint(heightfromsize(DEFAULT_TEXT_SIZE));
		return ES_NORMAL;
	case P_TEXT_SIZE:
		ep.setint(DEFAULT_TEXT_SIZE);
		return ES_NORMAL;
	case P_TEXT_STYLE:
		ep.setstaticcstring(MCplainstring);
		return ES_NORMAL;
#endif /* MCDispatch::getprop */ 
	default:
		MCeerror->add(EE_OBJECT_GETNOPROP, 0, 0);
		return ES_ERROR;
	}
}

Exec_stat MCDispatch::setprop_legacy(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
#ifdef /* MCDispatch::setprop */ LEGACY_EXEC
	return ES_NORMAL;
#endif /* MCDispatch::setprop */
}

// bogus "cut" call actually checks license
Boolean MCDispatch::cut(Boolean home)
{
	if (home)
		return True;
	return MCnoui || (flags & F_WAS_LICENSED) != 0;
}


//extern Exec_stat MCHandlePlatformMessage(Handler_type htype, const MCString& mess, MCParameter *params);
Exec_stat MCDispatch::handle(Handler_type htype, MCNameRef mess, MCParameter *params, MCObject *pass_from)
{
	Exec_stat stat = ES_NOT_HANDLED;

	bool t_has_passed;
	t_has_passed = false;
	
	if (MCcheckstack && MCU_abs(MCstackbottom - (char *)&stat) > MCrecursionlimit)
	{
		MCeerror->add(EE_RECURSION_LIMIT, 0, 0);
		MCerrorptr = stacks;
		return ES_ERROR;
	}

	// MW-2011-06-30: Move handling of library stacks from MCStack::handle.
	if (MCnusing > 0)
	{
		for (uint32_t i = MCnusing; i > 0 && (stat == ES_PASS || stat == ES_NOT_HANDLED); i -= 1)
		{
			stat = MCusing[i - 1]->handle(htype, mess, params, nil);

			// MW-2011-08-22: [[ Bug 9686 ]] Make sure we exit as soon as the
			//   message is handled.
			if (stat != ES_NOT_HANDLED && stat != ES_PASS)
				return stat;

			if (stat == ES_PASS)
				t_has_passed = true;
		}

		if (t_has_passed && stat == ES_NOT_HANDLED)
			stat = ES_PASS;
	}

	if ((stat == ES_NOT_HANDLED || stat == ES_PASS) && MCbackscripts != NULL)
	{
		MCObjectList *optr = MCbackscripts;
		do
		{
			if (!optr->getremoved())
			{
				stat = optr->getobject()->handle(htype, mess, params, nil);
				if (stat != ES_NOT_HANDLED && stat != ES_PASS)
					return stat;
				if (stat == ES_PASS)
					t_has_passed = true;
			}
			optr = optr->next();
		}
		while (optr != MCbackscripts);
	}

	if ((stat == ES_NOT_HANDLED || stat == ES_PASS) && m_externals != nil)
	{
		Exec_stat oldstat = stat;
		stat = m_externals -> Handle(this, htype, mess, params);

		// MW-2011-08-22: [[ Bug 9686 ]] Make sure we exit as soon as the
		//   message is handled.
		if (stat != ES_NOT_HANDLED && stat != ES_PASS)
			return stat;

		if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
			stat = ES_PASS;
	}

//#ifdef TARGET_SUBPLATFORM_IPHONE
//	extern Exec_stat MCIPhoneHandleMessage(MCNameRef message, MCParameter *params);
//	if (stat == ES_NOT_HANDLED || stat == ES_PASS)
//	{
//		stat = MCIPhoneHandleMessage(mess, params);
//		
//		if (stat != ES_NOT_HANDLED && stat != ES_PASS)
//			return stat;
//	}
//#endif
//
//#ifdef _MOBILE
//	if (stat == ES_NOT_HANDLED || stat == ES_PASS)
//	{
//		stat = MCHandlePlatformMessage(htype, MCNameGetOldString(mess), params);
//
//		// MW-2011-08-22: [[ Bug 9686 ]] Make sure we exit as soon as the
//		//   message is handled.
//		if (stat != ES_NOT_HANDLED && stat != ES_PASS)
//			return stat;
//	}
//#endif

	if (MCmessagemessages && stat != ES_PASS)
		MCtargetptr->sendmessage(htype, mess, False);
		
	if (t_has_passed)
		return ES_PASS;

	return stat;
}

bool MCDispatch::getmainstacknames(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	MCStack *tstk = stacks;
	do
	{
		MCAutoStringRef t_string;
		if (!tstk->names(P_SHORT_NAME, &t_string))
			return false;
		if (!MCListAppend(*t_list, *t_string))
			return false;
		tstk = (MCStack *)tstk->next();
	}
	while (tstk != stacks);

	return MCListCopy(*t_list, r_list);
}

void MCDispatch::appendstack(MCStack *sptr)
{
	sptr->appendto(stacks);
	
	// MW-2013-03-20: [[ MainStacksChanged ]]
	MCmainstackschanged = True;
}

void MCDispatch::removestack(MCStack *sptr)
{
	sptr->remove(stacks);
	
	// MW-2013-03-20: [[ MainStacksChanged ]]
	MCmainstackschanged = True;
}

void MCDispatch::destroystack(MCStack *sptr, Boolean needremove)
{
	if (needremove)
		removestack(sptr);
	if (sptr == MCstaticdefaultstackptr)
		MCstaticdefaultstackptr = stacks;
	if (sptr == MCdefaultstackptr)
		MCdefaultstackptr = MCstaticdefaultstackptr;
	if (MCacptr != NULL && MCacptr->getmessagestack() == sptr)
		MCacptr->setmessagestack(NULL);
	Boolean oldstate = MClockmessages;
	MClockmessages = True;
	delete sptr;
	MClockmessages = oldstate;
}

static bool attempt_to_loadfile(IO_handle& r_stream, MCStringRef& r_path, const char *p_path_format, ...)
{
	MCAutoStringRef t_trial_path;

	va_list t_args;
	va_start(t_args, p_path_format);
	/* UNCHECKED */ MCStringFormatV(&t_trial_path, p_path_format, t_args);
	va_end(t_args);

	IO_handle t_trial_stream;
	t_trial_stream = MCS_open(*t_trial_path, kMCSOpenFileModeRead, True, False, 0);

	if (t_trial_stream != nil)
	{
		r_path = (MCStringRef)MCValueRetain(*t_trial_path);
		r_stream = t_trial_stream;
		return true;
	}

	return false;
}

Boolean MCDispatch::openstartup(MCStringRef sname, MCStringRef& outpath, IO_handle &stream)
{
	if (enginedir == nil)
		return False;

	if (attempt_to_loadfile(stream, outpath, "%s/%s", startdir, MCStringGetCString(sname)))
		return True;

	if (attempt_to_loadfile(stream, outpath, "%s/%s", enginedir, MCStringGetCString(sname)))
		return True;

	return False;
}

Boolean MCDispatch::openenv(MCStringRef sname, MCStringRef env,
                            MCStringRef& outpath, IO_handle &stream, uint4 offset)
{

	MCAutoStringRef t_env;
	if (!MCS_getenv(env, &t_env))
		return False;

	bool t_found;
	t_found = false;

	MCStringRef t_rest_of_env;
	t_rest_of_env = MCValueRetain(env);
	while(!t_found && !MCStringIsEmpty(t_rest_of_env))
	{
		MCAutoStringRef t_env_path;
		MCStringRef t_next_rest_of_env;
		/* UNCHECKED */ MCStringDivideAtChar(t_rest_of_env, ENV_SEPARATOR, kMCStringOptionCompareCaseless, &t_env_path, t_next_rest_of_env);
		if (attempt_to_loadfile(stream, outpath, "%s/%s", MCStringGetCString(*t_env_path), MCStringGetCString(sname)))
			t_found = true;

		MCValueRelease(t_rest_of_env);
		t_rest_of_env = t_next_rest_of_env;
	}

	MCValueRelease(t_rest_of_env);

	return t_found;
}

IO_stat readheader(IO_handle& stream, char *version)
{
	char tnewheader[NEWHEADERSIZE];
	if (IO_read(tnewheader, NEWHEADERSIZE, stream) == IO_NORMAL)
	{
		// MW-2012-03-04: [[ StackFile5500 ]] Check for either the 2.7 or 5.5 header.
		if (strncmp(tnewheader, newheader, NEWHEADERSIZE) == 0 ||
			strncmp(tnewheader, newheader5500, NEWHEADERSIZE) == 0)
		{
			sprintf(version, "%c.%c.%c.%c", tnewheader[4], tnewheader[5], tnewheader[6], tnewheader[7]);
			if (tnewheader[7] == '0')
			{
				version[5] = '\0';
				if (tnewheader[6] == '0')
					version[3] = '\0';
			}
		}
		else
		{
			char theader[HEADERSIZE + 1];
			theader[HEADERSIZE] = '\0';
			uint4 offset;
			strncpy(theader, tnewheader, NEWHEADERSIZE);
			if (IO_read(theader + NEWHEADERSIZE, HEADERSIZE - NEWHEADERSIZE, stream) == IO_NORMAL
		        && MCU_offset(SIGNATURE, theader, offset))
			{
				if (theader[offset - 1] != '\n' || theader[offset - 2] == '\r')
				{
					MCresult->sets("stack was corrupted by a non-binary file transfer");
					return IO_ERROR;
				}

				strncpy(version, &theader[offset + VERSION_OFFSET], 3);
				version[3] = '\0';
			}
			else
				return IO_ERROR;
		}
	}
	return IO_NORMAL;
}

// This method reads a stack from the given stream. The stack is set to
// have parent MCDispatch, and filename MCcmd. It is designed to be used
// for embedded stacks/deployed stacks/revlet stacks.
IO_stat MCDispatch::readstartupstack(IO_handle stream, MCStack*& r_stack)
{
	char version[8];
	uint1 charset, type;
	char *newsf;
	if (readheader(stream, version) != IO_NORMAL
	        || IO_read_uint1(&charset, stream) != IO_NORMAL
	        || IO_read_uint1(&type, stream) != IO_NORMAL
	        || IO_read_string(newsf, stream) != IO_NORMAL)
		return IO_ERROR;

	// MW-2008-10-20: [[ ParentScripts ]] Set the boolean flag that tells us whether
	//   parentscript resolution is required to false.
	s_loaded_parent_script_reference = false;

	MCtranslatechars = charset != CHARSET;
	delete newsf; // stackfiles is obsolete

	MCStack *t_stack = nil;
	/* UNCHECKED */ MCStackSecurityCreateStack(t_stack);

	t_stack -> setparent(this);
	t_stack -> setfilename(MCcmd);

	if (IO_read_uint1(&type, stream) != IO_NORMAL
	        || type != OT_STACK && type != OT_ENCRYPT_STACK
	        || t_stack->load(stream, version, type) != IO_NORMAL)
	{
		delete t_stack;
		return IO_ERROR;
	}

	if (t_stack->load_substacks(stream, version) != IO_NORMAL
	        || IO_read_uint1(&type, stream) != IO_NORMAL
	        || type != OT_END)
	{
		delete t_stack;
		return IO_ERROR;
	}

	// We are reading the startup stack, so this becomes the root of the
	// stack list.
	stacks = t_stack;

	r_stack = t_stack;

#ifndef _MOBILE
	// Make sure parent script references are up to date.
	if (s_loaded_parent_script_reference)
		t_stack -> resolveparentscripts();
#else
	// Mark the stack as needed parentscript resolution. This is done after
	// aux stacks have been loaded.
	if (s_loaded_parent_script_reference)
		t_stack -> setextendedstate(True, ECS_USES_PARENTSCRIPTS);
#endif

	return IO_NORMAL;
}

// MW-2012-02-17: [[ LogFonts ]] Load a stack file, ensuring we clear up any
//   font table afterwards - regardless of errors.
IO_stat MCDispatch::readfile(MCStringRef p_openpath, MCStringRef p_name, IO_handle &stream, MCStack *&sptr)
{
	// Various places like to call this function with the first two parameters as NULL
	if (p_openpath == nil)
		p_openpath = kMCEmptyString;
	if (p_name == nil)
		p_name = kMCEmptyString;
	
	IO_stat stat;
	stat = doreadfile(p_openpath, p_name, stream, sptr);

	MCLogicalFontTableFinish();

	return stat;
}

// MW-2012-02-17: [[ LogFonts ]] Actually load the stack file (wrapped by readfile
//   to handle font table cleanup).
IO_stat MCDispatch::doreadfile(MCStringRef p_openpath, MCStringRef p_name, IO_handle &stream, MCStack *&sptr)
{
	Boolean loadhome = False;
	char version[8];

	if (readheader(stream, version) == IO_NORMAL)
	{
		if (strcmp(version, MCNameGetCString(MCN_version_string)) > 0)
		{
			MCresult->sets("stack was produced by a newer version");
			return IO_ERROR;
		}

		// MW-2008-10-20: [[ ParentScripts ]] Set the boolean flag that tells us whether
		//   parentscript resolution is required to false.
		s_loaded_parent_script_reference = false;

		uint1 charset, type;
		char *newsf;
		if (IO_read_uint1(&charset, stream) != IO_NORMAL
		        || IO_read_uint1(&type, stream) != IO_NORMAL
		        || IO_read_string(newsf, stream) != IO_NORMAL)
		{
			MCresult->sets("stack is corrupted, check for ~ backup file");
			return IO_ERROR;
		}
		delete newsf; // stackfiles is obsolete
		MCtranslatechars = charset != CHARSET;
		sptr = nil;
		/* UNCHECKED */ MCStackSecurityCreateStack(sptr);
		if (stacks == NULL)
			sptr->setparent(this);
		else
			sptr->setparent(stacks);
			
		sptr->setfilename(p_openpath);

		if (MCModeCanLoadHome() && type == OT_HOME)
		{
			char *lstring = NULL;
			char *cstring = NULL;
			IO_read_string(lstring, stream);
			IO_read_string(cstring, stream);
			delete lstring;
			delete cstring;
		}

		MCresult -> clear();

		if (IO_read_uint1(&type, stream) != IO_NORMAL
		    || type != OT_STACK && type != OT_ENCRYPT_STACK
		    || sptr->load(stream, version, type) != IO_NORMAL)
		{
			if (MCresult -> isclear())
				MCresult->sets("stack is corrupted, check for ~ backup file");
			destroystack(sptr, False);
			sptr = NULL;
			return IO_ERROR;
		}
		
		// MW-2011-08-09: [[ Groups ]] Make sure F_GROUP_SHARED is set
		//   appropriately.
		sptr -> checksharedgroups();
		
		if (sptr->load_substacks(stream, version) != IO_NORMAL
		        || IO_read_uint1(&type, stream) != IO_NORMAL
		        || type != OT_END)
		{
			if (MCresult -> isclear())
				MCresult->sets("stack is corrupted, check for ~ backup file");
			destroystack(sptr, False);
			sptr = NULL;
			return IO_ERROR;
		}

		if (stacks != NULL)
		{
			MCStack *tstk = stacks;
			do
			{
				if (sptr->hasname(tstk->getname()))
				{
					MCAutoNameRef t_stack_name;
					/* UNCHECKED */ t_stack_name . Clone(sptr -> getname());

					delete sptr;
					sptr = NULL;
					
					
					if (MCStringIsEqualTo(tstk -> getfilename(), p_openpath, kMCStringOptionCompareCaseless))
						sptr = tstk;
					else
					{
						MCdefaultstackptr->getcard()->message_with_valueref_args(MCM_reload_stack, tstk->getname(), p_openpath);
						tstk = stacks;
						do
						{
							if (MCNameIsEqualTo(t_stack_name, tstk->getname(), kMCCompareCaseless))
							{
								sptr = tstk;
								break;
							}
							tstk = (MCStack *)tstk->next();
						}
						while (tstk != stacks);
					}

					return IO_NORMAL;
				}
				tstk = (MCStack *)tstk->next();
			}
			while (tstk != stacks);
		}
		
		appendstack(sptr);
		
		sptr->extraopen(false);

		// MW-2008-10-28: [[ ParentScript ]]
		// We just loaded a stackfile, so check to see if parentScript resolution
		// is required and if so do it.
		// MW-2009-01-28: [[ Inherited parentScripts ]]
		// Resolving parentScripts may allocate memory, so 'resolveparentscripts'
		// will return false if it fails to allocate what it needs. At some point
		// this needs to be dealt with by deleting the stack and returning an error,
		// *However* at the time of writing, 'readfile' isn't designed to handle
		// this - so we just ignore the result for now (note that all the 'load'
		// methods *fail* to check for no-memory errors!).
		if (s_loaded_parent_script_reference)
			sptr -> resolveparentscripts();
		
	}
	else
	{
		MCS_seek_set(stream, 0);
		if (stacks == NULL)
		{
			MCnoui = True;
			MCscreen = new MCUIDC;
			/* UNCHECKED */ MCStackSecurityCreateStack(stacks);
			MCdefaultstackptr = MCstaticdefaultstackptr = stacks;
			stacks->setparent(this);
			stacks->setname_cstring("revScript");
			uint4 size = (uint4)MCS_fsize(stream);
            MCAutoPointer<char> script;
            script = new char[size + 2];
            (*script)[size] = '\n';
            (*script)[size + 1] = '\0';
            if (IO_read(*script, size, stream) != IO_NORMAL)
                return IO_ERROR;
            MCAutoStringRef t_script_str;
            /* UNCHECKED */ MCStringCreateWithCString(*script, &t_script_str);
            if (!stacks -> setscript(*t_script_str))
                return IO_ERROR;
		}
		else
		{
			// MW-2008-06-12: [[ Bug 6476 ]] Media won't open HC stacks
			if (!MCdispatcher->cut(True) || hc_import(p_name, stream, sptr) != IO_NORMAL)
			{
				MCresult->sets("file is not a stack");
				return IO_ERROR;
			}
		}
	}
	return IO_NORMAL;
}

IO_stat MCDispatch::loadfile(MCStringRef p_name, MCStack *&sptr)
{
	IO_handle stream;
	MCAutoStringRef t_open_path;

	bool t_found;
	t_found = false;
	if (!t_found)
	{
		if ((stream = MCS_open(p_name, kMCSOpenFileModeRead, True, False, 0)) != NULL)
		{
			// This should probably use resolvepath().
			if (MCStringGetCharAtIndex(p_name, 0) != PATH_SEPARATOR 
				&& MCStringGetCharAtIndex(p_name, 1) != ':')
			{
				MCAutoStringRef t_curpath;
				
				/* UNCHECKED */ MCS_getcurdir(&t_curpath);
				/* UNCHECKED */ MCStringFormat(&t_open_path, "@s/%@", *t_curpath, p_name); 
			}
			else
				t_open_path = p_name;

			t_found = true;
		}
	}

	if (!t_found)
	{
		MCAutoStringRef t_leaf_name;
		uindex_t t_leaf_index;
		if (MCStringLastIndexOfChar(p_name, PATH_SEPARATOR, UINDEX_MAX, kMCStringOptionCompareCaseless, t_leaf_index))
			/* UNCHECKED */ MCStringCopySubstring(p_name, MCRangeMake(t_leaf_index + 1, MCStringGetLength(p_name) - (t_leaf_index + 1)), &t_leaf_name);
		else
			t_leaf_name = p_name;
		if ((stream = MCS_open(*t_leaf_name, kMCSOpenFileModeRead, True, False, 0)) != NULL)
		{
			MCAutoStringRef t_curpath;
			/* UNCHECKED */ MCS_getcurdir(&t_curpath);
			/* UNCHECKED */ MCStringFormat(&t_open_path, "%@/%@", *t_curpath, p_name); 
			t_found = true;
		}
	}

	if (!t_found)
	{
		if (openstartup(p_name, &t_open_path, stream) ||
		        openenv(p_name, MCSTR("MCPATH"), &t_open_path, stream, 0) ||
		        openenv(p_name, MCSTR("PATH"), &t_open_path, stream, 0))
			t_found = true;
	}

	if (!t_found)
	{

		MCAutoStringRef t_homename;
		if (MCS_getenv(MCSTR("HOME"), &t_homename))
		{
			MCAutoStringRef t_trimmed_homename;
			if (MCStringGetCharAtIndex(*t_homename, MCStringGetLength(*t_homename) - 1) == '/')
				/* UNCHECKED */ MCStringCopySubstring(*t_homename, MCRangeMake(0, MCStringGetLength(*t_homename) - 1), &t_trimmed_homename);
			else
				t_trimmed_homename = *t_homename;

			if (!t_found)
				t_found = attempt_to_loadfile(stream, &t_open_path, "%@/%@", *t_trimmed_homename, p_name);

			if (!t_found)
				t_found = attempt_to_loadfile(stream, &t_open_path, "%@/stacks/%@", *t_trimmed_homename, p_name);

			if (!t_found)
				t_found = attempt_to_loadfile(stream, &t_open_path, "%@/components/%@", *t_trimmed_homename, p_name);
		}
	}


	if (stream == NULL)
	{
		return IO_ERROR;
	}
	IO_stat stat = readfile(*t_open_path, p_name, stream, sptr);
	MCS_close(stream);
	return stat;
}

void MCDispatch::cleanup(IO_handle stream, MCStringRef linkname, MCStringRef bname)
{
	if (stream != NULL)
		MCS_close(stream);
	MCS_unlink(linkname);
	if (bname != NULL)
		MCS_unbackup(bname, linkname);
}

IO_stat MCDispatch::savestack(MCStack *sptr, const MCStringRef p_fname)
{
	IO_stat stat;
	stat = dosavestack(sptr, p_fname);

	MCLogicalFontTableFinish();

	return stat;
}

IO_stat MCDispatch::dosavestack(MCStack *sptr, const MCStringRef p_fname)
{
	if (MCModeCheckSaveStack(sptr, p_fname) != IO_NORMAL)
		return IO_ERROR;
	
	MCAutoStringRef t_linkname;

	if (!MCStringIsEmpty(p_fname))
		t_linkname = p_fname;
	else if (!MCStringIsEmpty(sptr -> getfilename()))
		t_linkname = sptr -> getfilename();
	else
	{
		MCresult -> sets("stack does not have filename");
		return IO_ERROR;
	}
	
	if (MCS_noperm(*t_linkname))
	{
		MCresult->sets("can't open stack file, no permission");
		return IO_ERROR;
	}

	MCStringRef oldfiletype;
	oldfiletype = (MCStringRef)MCValueRetain(MCfiletype);
	MCValueAssign(MCfiletype, MCstackfiletype);
	
	MCAutoStringRef t_backup;
	/* UNCHECKED */ MCStringFormat(&t_backup, "%s~", MCStringGetCString(*t_linkname)); 

	MCS_unlink(*t_backup);
	if (MCS_exists(*t_linkname, True) && !MCS_backup(*t_linkname, *t_backup))
	{
		MCresult->sets("can't open stack backup file");

		MCValueAssign(MCfiletype, oldfiletype);
		return IO_ERROR;
	}
	IO_handle stream;

	if ((stream = MCS_open(*t_linkname, kMCSOpenFileModeWrite, True, False, 0)) == NULL)
	{
		MCresult->sets("can't open stack file");
		cleanup(stream, *t_linkname, *t_backup);
		MCValueAssign(MCfiletype, oldfiletype);
		return IO_ERROR;
	}
	MCValueAssign(MCfiletype, oldfiletype);
	MCString errstring = "Error writing stack (disk full?)";
	
	// MW-2012-03-04: [[ StackFile5500 ]] Work out what header to emit, and the size.
	const char *t_header;
	uint32_t t_header_size;
	if (MCstackfileversion >= 5500)
		t_header = newheader5500, t_header_size = 8;
	else if (MCstackfileversion >= 2700)
		t_header = newheader, t_header_size = 8;
	else
		t_header = header, t_header_size = HEADERSIZE;
	
	if (IO_write(t_header, sizeof(char), t_header_size, stream) != IO_NORMAL
	        || IO_write_uint1(CHARSET, stream) != IO_NORMAL)
	{
		MCresult->sets(errstring);
		cleanup(stream, *t_linkname, *t_backup);
		return IO_ERROR;
	}

	if (IO_write_uint1(OT_NOTHOME, stream) != IO_NORMAL
	        || IO_write_string(NULL, stream) != IO_NORMAL)
	{ // was stackfiles
		MCresult->sets(errstring);
		cleanup(stream, *t_linkname, *t_backup);
		return IO_ERROR;
	}
	
	// MW-2012-02-22; [[ NoScrollSave ]] Adjust the rect by the current group offset.
	MCgroupedobjectoffset . x = 0;
	MCgroupedobjectoffset . y = 0;
	
	MCresult -> clear();
	if (sptr->save(stream, 0, false) != IO_NORMAL
	        || IO_write_uint1(OT_END, stream) != IO_NORMAL)
	{
		if (MCresult -> isclear())
			MCresult->sets(errstring);
		cleanup(stream, *t_linkname, *t_backup);
		return IO_ERROR;
	}
	MCS_close(stream);
	uint2 oldmask = MCS_getumask();
	uint2 newmask = ~oldmask & 00777;
	if (oldmask & 00400)
		newmask &= ~00100;
	if (oldmask & 00040)
		newmask &= ~00010;
	if (oldmask & 00004)
		newmask &= ~00001;
	MCS_setumask(oldmask);
	
	MCS_chmod(*t_linkname, newmask);
	
	if (!MCStringIsEmpty(sptr -> getfilename()) && !MCStringIsEqualTo(sptr -> getfilename(), *t_linkname, kMCCompareExact))
		MCS_copyresourcefork(sptr -> getfilename(), *t_linkname);
	else if (!MCStringIsEmpty(sptr -> getfilename()))
		MCS_copyresourcefork(*t_backup, *t_linkname);

	sptr->setfilename(*t_linkname);
	MCS_unlink(*t_backup);
	return IO_NORMAL;
}

#ifdef FEATURE_RELAUNCH_SUPPORT
extern bool relaunch_startup(MCStringRef p_id);
#endif

void send_relaunch(void)
{
#ifdef FEATURE_RELAUNCH_SUPPORT
	bool t_do_relaunch;
	t_do_relaunch = false;
	MCAutoStringRef t_id;

	t_do_relaunch = MCModeHandleRelaunch(&t_id);

	if (t_do_relaunch)
		if (relaunch_startup(*t_id))
			exit(0);
#endif
}

void send_startup_message(bool p_do_relaunch = true)
{
	if (p_do_relaunch)
		send_relaunch();

	MCdefaultstackptr -> setextendedstate(true, ECS_DURING_STARTUP);

	MCdefaultstackptr -> getcard() -> message(MCM_start_up);

	MCdefaultstackptr -> setextendedstate(false, ECS_DURING_STARTUP);
}

void MCDispatch::wclose(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL && !target -> getextendedstate(ECS_DISABLED_FOR_MODAL))
	{
		Exec_stat stat = target->getcurcard()->message(MCM_close_stack_request);
		if (stat == ES_NOT_HANDLED || stat == ES_PASS)
		{
			target->kunfocus();
			target->close();
			target->checkdestroy();
		}
	}
}

void MCDispatch::wkfocus(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL)
		target->kfocus();
}

void MCDispatch::wkunfocus(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL)
		target->kunfocus();
}

Boolean MCDispatch::wkdown(Window w, const char *string, KeySym key)
{
	if (menu != NULL)
		return menu->kdown(string, key);

	MCStack *target = findstackd(w);
	if (target == NULL || !target->kdown(string, key))
	{
		if (MCmodifierstate & MS_MOD1)
		{
			MCButton *bptr = MCstacks->findmnemonic(MCS_tolower(string[0]));
			if (bptr != NULL)
			{
				bptr->activate(True, (uint2)key);
				return True;
			}
		}
	}
	else
		if (target != NULL)
			return True;
	return False;
}

void MCDispatch::wkup(Window w, const char *string, KeySym key)
{
	if (menu != NULL)
		menu->kup(string, key);
	else
	{
		MCStack *target = findstackd(w);
		if (target != NULL)
			target->kup(string, key);
	}
}

void MCDispatch::wmfocus_stack(MCStack *target, int2 x, int2 y)
{
	// IM-2013-09-23: [[ FullscreenMode ]] transform view -> stack coordinates
	MCPoint t_stackloc;
	if (menu != NULL)
	{
		t_stackloc = menu->getstack()->windowtostackloc(MCPointMake(x, y));
		menu->mfocus(t_stackloc.x, t_stackloc.y);
	}
	else
	{
		if (target != NULL)
		{
			t_stackloc = target->windowtostackloc(MCPointMake(x, y));
			target->mfocus(t_stackloc.x, t_stackloc.y);
		}
	}
}

void MCDispatch::wmfocus(Window w, int2 x, int2 y)
{
	MCStack *target = findstackd(w);
	wmfocus_stack(target, x, y);
}

void MCDispatch::wmunfocus(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL)
		target->munfocus();
}

void MCDispatch::wmdrag(Window w)
{
	if (!MCModeMakeLocalWindows())
		return;

	if (isdragsource())
		return;

	MCStack *target = findstackd(w);

	if (target != NULL)
		target->mdrag();

	MCPasteboard *t_pasteboard;
	t_pasteboard = MCdragdata -> GetSource();

	// OK-2009-03-13: [[Bug 7776]] - Check for null MCdragtargetptr to hopefully fix crash.
	if (t_pasteboard != NULL && MCdragtargetptr != NULL)
	{
		m_drag_source = true;
		m_drag_end_sent = false;

		// MW-2009-02-02: [[ Improved image search ]]
		// Search for the appropriate image object using the standard method - note
		// here we search relative to the target of the dragStart message.
		MCImage *t_image;
		t_image = NULL;
		if (MCdragimageid != 0)
			t_image = MCdragtargetptr != NULL ? MCdragtargetptr -> resolveimageid(MCdragimageid) : resolveimageid(MCdragimageid);
		
		MCdragsource = MCdragtargetptr;

		if (MCdragtargetptr->gettype() > CT_CARD)
		{
			MCControl *cptr = (MCControl *)MCdragtargetptr;
			cptr->munfocus();
			cptr->getcard()->ungrab();
		}
		MCdragtargetptr->getstack()->resetcursor(True);
		MCdragtargetptr -> getstack() -> munfocus();

		MCdragaction = MCscreen -> dodragdrop(t_pasteboard, MCallowabledragactions, t_image, t_image != NULL ? &MCdragimageoffset : NULL);

		dodrop(true);
		MCdragdata -> ResetSource();

		MCdragsource = NULL;
		MCdragdest = NULL;
		MCdropfield = NULL;
		MCdragtargetptr = NULL;
		m_drag_source = false;
	}
	else
	{
		MCdragdata -> ResetSource();
		MCdragsource = NULL;
		MCdragdest = NULL;
		MCdropfield = NULL;
		MCdragtargetptr = NULL;
		m_drag_source = false;
	}
}

void MCDispatch::wmdown_stack(MCStack *target, uint2 which)
{
	if (menu != NULL)
		menu -> mdown(which);
	else
	{
		if (!isdragsource())
		{
			MCallowabledragactions = DRAG_ACTION_COPY;
			MCdragaction = DRAG_ACTION_NONE;
			MCdragimageid = 0;
			MCdragimageoffset . x = 0;
			MCdragimageoffset . y = 0;
			MCdragdata -> ResetSource();
		}
		
		if (target != NULL)
			target->mdown(which);
	}
}

void MCDispatch::wmdown(Window w, uint2 which)
{
	MCStack *target = findstackd(w);
	wmdown_stack(target, which);
}

void MCDispatch::wmup_stack(MCStack *target, uint2 which)
{
	if (menu != NULL)
		menu->mup(which);
	else
	{
		if (target != NULL)
			target->mup(which);
	}
}

void MCDispatch::wmup(Window w, uint2 which)
{
	MCStack *target = findstackd(w);
	wmup_stack(target, which);
}

void MCDispatch::wdoubledown(Window w, uint2 which)
{
	if (menu != NULL)
		menu->doubledown(which);
	else
	{
		MCStack *target = findstackd(w);
		if (target != NULL)
			target->doubledown(which);
	}
}

void MCDispatch::wdoubleup(Window w, uint2 which)
{
	if (menu != NULL)
		menu->doubleup(which);
	else
	{
		MCStack *target = findstackd(w);
		if (target != NULL)
			target->doubleup(which);
	}
}

void MCDispatch::kfocusset(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL)
		target->kfocusset(NULL);
}

void MCDispatch::wmdragenter(Window w, MCPasteboard *p_data)
{
	
	MCStack *target = findstackd(w);
	
	m_drag_target = true;

	if (m_drag_source)
		MCdragdata -> SetTarget(MCdragdata -> GetSource());
	else
		MCdragdata -> SetTarget(p_data);

	if (MCmousestackptr != NULL && target != MCmousestackptr)
		MCmousestackptr -> munfocus();

	MCmousestackptr = target;
}

MCDragAction MCDispatch::wmdragmove(Window w, int2 x, int2 y)
{
	// We must also issue a new focus event if the modifierstate
	// changes.
	static uint4 s_old_modifiers = 0;

	MCStack *target = findstackd(w);
	
	// IM-2013-10-08: [[ FullscreenMode ]] Translate mouse location to stack coords
	MCPoint t_mouseloc;
	t_mouseloc = target->windowtostackloc(MCPointMake(x, y));
	
	if (MCmousex != t_mouseloc.x || MCmousey != t_mouseloc.y || MCmodifierstate != s_old_modifiers)
	{
		MCmousex = t_mouseloc.x;
		MCmousey = t_mouseloc.y;
		s_old_modifiers = MCmodifierstate;
		target -> mfocus(t_mouseloc.x, t_mouseloc.y);
	}
	return MCdragaction;
}

void MCDispatch::wmdragleave(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL && target == MCmousestackptr)
	{
		MCmousestackptr -> munfocus();
		MCmousestackptr = NULL;
	}
	MCdragdata -> ResetTarget();
	m_drag_target = false;
}

MCDragAction MCDispatch::wmdragdrop(Window w)
{
	MCStack *target;
	target = findstackd(w);
	
	// MW-2011-02-08: Make sure we store the drag action that is in effect now
	//   otherwise it can change as a result of message sends which is bad :o)
	uint32_t t_drag_action;
	t_drag_action = MCdragaction;
	
	if (t_drag_action != DRAG_ACTION_NONE)
		dodrop(false);

	MCmousestackptr = NULL;
	MCdragdata -> ResetTarget();
	m_drag_target = false;

	return t_drag_action;
}

void MCDispatch::property(Window w, Atom atom)
{
}

void MCDispatch::configure(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL)
		target->view_configure(true);
}

void MCDispatch::enter(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL)
		target->enter();
}

void MCDispatch::redraw(Window w, MCRegionRef p_dirty_region)
{
	MCStack *target = findstackd(w);
	if (target == NULL)
		return;

	target -> updatewindow(p_dirty_region);
}

MCFontStruct *MCDispatch::loadfont(MCNameRef fname, uint2 &size, uint2 style, Boolean printer)
{
#if defined(_LINUX_DESKTOP)
	if (fonts == NULL)
		fonts = MCFontlistCreateNew();
	if (fonts == NULL)
		fonts = MCFontlistCreateOld();
#elif defined(_LINUX_SERVER)
	// MM-2013-09-13: [[ RefactorGraphics ]] Server font support.
	if (fonts == NULL)
		fonts = MCFontlistCreateNew();
#else
	if (fonts == nil)
		fonts = new MCFontlist;
#endif
	return fonts->getfont(fname, size, style, printer);
}

MCStack *MCDispatch::findstackname(MCNameRef p_name)
{
	if (p_name == nil || MCNameIsEmpty(p_name))
		return NULL;

	MCStack *tstk = stacks;
	if (tstk != NULL)
	{
		do
		{
			MCStack *foundstk;
			if ((foundstk = (MCStack *)tstk->findsubstackname(p_name)) != NULL)
				return foundstk;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}

	tstk = stacks;
	if (tstk != NULL)
	{
		do
		{
			MCStack *foundstk;
			if ((foundstk = (MCStack *)tstk->findstackfile(p_name)) != NULL)
				return foundstk;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}

	if (loadfile(MCNameGetString(p_name), tstk) != IO_NORMAL)
	{
		MCAutoStringRef t_name;
		/* UNCHECKED */ MCStringMutableCopy(MCNameGetString(p_name), &t_name);
		/* UNCHECKED */ MCStringLowercase(*t_name);
		
		// Remove all special characters from the input string
		// TODO: what about other 'special' chars added by unicode?
		MCStringRef t_replace = MCSTR("\r\n\t *?<>/\\()[]{}|'`\"");
		MCRange t_range = MCRangeMake(0, MCStringGetLength(t_replace));
		for (uindex_t i = 0; i < MCStringGetLength(*t_name); i++)
		{
			if (MCStringCountChar(t_replace, t_range, MCStringGetCharAtIndex(*t_name, i), kMCStringOptionCompareExact))
				/* UNCHECKED */ MCStringReplace(*t_name, MCRangeMake(i, 1), MCSTR("_"));
		}
		
		MCAutoStringRef t_name_mc;
		/* UNCHECKED */ MCStringFormat(&t_name_mc, "%@.mc", *t_name);
		if (loadfile(*t_name_mc, tstk) != IO_NORMAL)
		{
			MCAutoStringRef t_name_rev;
			/* UNCHECKED */ MCStringFormat(&t_name_rev, "%@.rev", *t_name);
			if (loadfile(*t_name_rev, tstk) != IO_NORMAL)
				return NULL;
		}
	}

	return tstk;
}

MCStack *MCDispatch::findstackid(uint4 fid)
{
	if (fid == 0)
		return NULL;
	MCStack *tstk = stacks;
	do
	{
		MCStack *foundstk;
		if ((foundstk = (MCStack *)tstk->findsubstackid(fid)) != NULL)
			return foundstk;
		tstk = (MCStack *)tstk->next();
	}
	while (tstk != stacks);
	return NULL;
}

MCStack *MCDispatch::findchildstackd(Window w,uint2 cindex)
{
	uint2 ccount = 0;
	if (stacks != NULL)
	{
		MCStack *tstk = stacks;
		do
		{
			MCStack *foundstk;
			if ((foundstk =
			            (MCStack *)tstk->findchildstackd(w,ccount,cindex))!= NULL)
				return foundstk;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}
	return NULL;
}

MCStack *MCDispatch::findstackd(Window w)
{
	if (w == DNULL)
		return NULL;
	
	if (stacks != NULL)
	{
		MCStack *tstk = stacks;
		do
		{
			MCStack *foundstk;
			if ((foundstk = tstk->findstackd(w)) != NULL)
				return foundstk;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}
	if (panels != NULL)
	{
		MCStack *tstk = panels;
		do
		{
			MCStack *foundstk;
			if ((foundstk = tstk->findstackd(w)) != NULL)
				return foundstk;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != panels);
	}

	// MW-2006-04-24: [[ Purify ]] It is possible to get here after MCtooltip has been
	//   deleted. So MCtooltip is now NULL in this situation and we test for it here.
	if (MCtooltip != NULL && MCtooltip->findstackd(w))
		return MCtooltip;
	return NULL;
}

MCObject *MCDispatch::getobjid(Chunk_term type, uint4 inid)
{
	if (stacks != NULL)
	{
		MCStack *tstk = stacks;
		do
		{
			MCObject *optr;
			if ((optr = tstk->getsubstackobjid(type, inid)) != NULL)
				return optr;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}
	return NULL;
}

MCObject *MCDispatch::getobjname(Chunk_term type, MCStringRef s)
{
	if (stacks != NULL)
	{
		MCStack *tstk = stacks;
		do
		{
			MCObject *optr;
			if ((optr = tstk->getsubstackobjname(type, s)) != NULL)
				return optr;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}

	if (type == CT_IMAGE)
	{
		const char *sptr = MCStringGetCString(s);
		uint4 l = MCStringGetLength(s);

		MCAutoNameRef t_image_name;
		if (MCU_strchr(sptr, l, ':'))
			/* UNCHECKED */ MCNameCreate(s, t_image_name);
		
		MCImage *iptr = imagecache;
		if (iptr != NULL)
		{
			do
			{
check:
				if (t_image_name != nil && iptr -> hasname(t_image_name))
					return iptr;
				if (!iptr->getopened())
				{
					iptr->remove(imagecache);
					delete iptr;
					iptr = imagecache;
					if (iptr == NULL)
						break;
					goto check;
				}
				iptr = (MCImage *)iptr->next();
			}
			while (iptr != imagecache);
		}

		if (MCU_strchr(sptr, l, ':'))
		{
			MCresult->clear(False);
			MCExecPoint ep(MCdefaultstackptr, NULL, NULL);
			MCExecPoint *epptr = MCEPptr == NULL ? &ep : MCEPptr;
			epptr->setvalueref(s);
			MCU_geturl(*epptr);
			if (MCresult->isempty())
			{
				iptr = new MCImage;
				iptr->appendto(imagecache);
				iptr->setprop(0, P_TEXT, *epptr, False);
				iptr->setname(t_image_name);
				return iptr;
			}
		}
	}
	return NULL;
}

MCStack *MCDispatch::gethome()
{
	return stacks;
}

Boolean MCDispatch::ismainstack(MCStack *sptr)
{
	if (stacks != NULL)
	{
		MCStack *tstk = stacks;
		do
		{
			if (tstk == sptr)
				return True;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}
	return False;
}

void MCDispatch::addmenu(MCObject *target)
{
	menu = target;
	target->getcard()->ungrab();
}

void MCDispatch::removemenu()
{
	//  menu->getstack()->mfocus(MCmousex, MCmousey); //disrupts card kfocus
	menu = NULL;
}

void MCDispatch::closemenus()
{
	if (menu != NULL)
		menu->closemenu(True, True);
}

void MCDispatch::appendpanel(MCStack *sptr)
{
	sptr->appendto(panels);
}

void MCDispatch::removepanel(MCStack *sptr)
{
	sptr->remove(panels);
}

///////////////////////////////////////////////////////////////////////////////

bool MCDispatch::loadexternal(MCStringRef p_external)
{
	MCStringRef t_filename;
#if defined(TARGET_SUBPLATFORM_ANDROID)
	extern bool revandroid_loadExternalLibrary(MCStringRef p_external, MCStringRef &r_filename);
	// MW-2013-08-07: [[ ExternalsApiV5 ]] Make sure we only use the leaf name
	//   of the external when loading.
    uindex_t t_slash_index;
    uindex_t t_ext_length = MCStringGetLength(p_external);
    MCStringRef t_external_leaf;
    
	if (MCStringLastIndexOfChar(p_external, '/', t_ext_length, kMCStringOptionCompareExact, t_slash_index))
    {
		if (!MCStringCopySubstring(p_external, MCRangeMake(t_slash_index + 1, t_ext_length - t_slash_index - 1), t_external_leaf))
            return false;
    }
    else
        t_external_leaf = MCValueRetain(p_external);

	if (!revandroid_loadExternalLibrary(t_external_leaf, t_filename))
    {
        MCValueRelease(t_external_leaf);
		return false;
    }

	// Don't try and load any drivers as externals.
	if (MCStringBeginsWithCString(t_external_leaf, (const char_t *)"db", kMCStringOptionCompareExact))
	{
        MCValueRelease(t_external_leaf);
		MCValueRelease(t_filename);
		return true;
	}
    
    MCValueRelease(t_external_leaf);
#elif !defined(_SERVER)
	if (MCStringBeginsWithCString(p_external, (const char_t *)"/", kMCStringOptionCompareExact))
	{
		t_filename = MCValueRetain(p_external);
	}
	else
	{
		uindex_t t_separator;
		MCStringLastIndexOfChar(MCcmd, '/', 0, kMCStringOptionCompareExact, t_separator);
		if (!MCStringMutableCopySubstring(MCcmd, MCRangeMake(0, t_separator), t_filename))
			return false;
		if (!MCStringAppendFormat(t_filename, "/%@", p_external))
		{
			MCValueRelease(t_filename);
			return false;
		}
	}

#else
	t_filename = MCValueRetain(p_external);
#endif
	
	if (m_externals == nil)
		m_externals = new MCExternalHandlerList;
	
	bool t_loaded;
	t_loaded = m_externals -> Load(t_filename);
	MCValueRelease(t_filename);
	
	if (m_externals -> IsEmpty())
	{
		delete m_externals;
		m_externals = nil;
}

	return t_loaded;
}

///////////////////////////////////////////////////////////////////////////////

// We have three contexts to be concerned with:
//   - text editing : MCactivefield != NULL
//   - image editing : MCactiveimage != NULL
//   - stack editing
//
// We try each of these in turn, attempting appropriate things in each case.
//
bool MCDispatch::dopaste(MCObject*& r_objptr, bool p_explicit)
{
	r_objptr = NULL;

	if (MCactivefield != NULL)
	{
		MCParagraph *t_paragraphs;
		t_paragraphs = MCclipboarddata -> FetchParagraphs(MCactivefield);

		//

		if (t_paragraphs != NULL)
		{
			// MW-2012-03-16: [[ Bug ]] Fetch the current active field since it can be
			//   unset as a result of pasting (due to scrollbarDrag).
			MCField *t_field;
			t_field = MCactivefield;

			// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
			//   textChanged message by a lock screen pair.
			MCRedrawLockScreen();
			t_field -> pastetext(t_paragraphs, true);
			MCRedrawUnlockScreen();

			// MW-2012-02-08: [[ TextChanged ]] Invoke textChanged as this method
			//   was called as a result of a user action (paste cmd, paste key).
			t_field -> textchanged();
			return true;
		}
	}
	
	if (MCactiveimage != NULL && MCclipboarddata -> HasImage())
	{
		MCAutoDataRef t_data;
		if (MCclipboarddata -> Fetch(TRANSFER_TYPE_IMAGE, &t_data))
		{
			MCExecPoint ep(NULL, NULL, NULL);
			MCExecContext ctxt(ep);

			MCImage *t_image;
			t_image = new MCImage;
			t_image -> open();
			t_image -> openimage();
			MCAutoStringRef t_string_data;
			/* UNCHECKED */ MCStringCreateWithNativeChars((const char_t *)MCDataGetBytePtr(*t_data), MCDataGetLength(*t_data), &t_string_data);
			t_image -> setstringprop(ctxt, 0, P_TEXT, False, *t_string_data);
			MCactiveimage -> pasteimage(t_image);
			t_image -> closeimage();
			t_image -> close();

			delete t_image;
			return true; 
		}

		return false;
	}
	
	if (MCdefaultstackptr != NULL && (p_explicit || MCdefaultstackptr -> gettool(MCdefaultstackptr) == T_POINTER))
	{
		MCObject *t_objects;
		t_objects = NULL;

		if (!MCclipboarddata -> Lock())
			return false;
		if (MCclipboarddata -> HasObjects())
		{
			MCAutoDataRef t_data;
			if (MCclipboarddata -> Fetch(TRANSFER_TYPE_OBJECTS, &t_data))
				t_objects = MCObject::unpickle(*t_data, MCdefaultstackptr);
		}
		else if (MCclipboarddata -> HasImage())
		{
			MCAutoDataRef t_data;
			if (MCclipboarddata -> Fetch(TRANSFER_TYPE_IMAGE, &t_data))
			{
				MCExecPoint ep(NULL, NULL, NULL);
				MCExecContext ctxt(ep);
				
				t_objects = new MCImage(*MCtemplateimage);
				t_objects -> open();
				MCAutoStringRef t_string_data;
				/* UNCHECKED */ MCStringCreateWithNativeChars((const char_t *)MCDataGetBytePtr(*t_data), MCDataGetLength(*t_data), &t_string_data);
				t_objects -> setstringprop(ctxt, 0, P_TEXT, False, *t_string_data);
				t_objects -> close();
			}
		}
		MCclipboarddata -> Unlock();

		//

		if (t_objects != NULL)
		{
			MCselected -> clear(False);
			MCselected -> lockclear();

			while(t_objects != NULL)
			{
				MCObject *t_object;
				t_object = t_objects -> remove(t_objects);
				t_object -> paste();

				// OK-2009-04-02: [[Bug 7881]] - Parentscripts broken by cut and pasting object
				t_object -> resolveparentscript();

				if (t_object -> getparent() == NULL)
					delete t_object;
				else
					r_objptr = t_object;
			}

			MCselected -> unlockclear();
	
			return true;
		}
	}

	return false;
}

void MCDispatch::dodrop(bool p_source)
{
	if (!m_drag_end_sent && MCdragsource != NULL && (MCdragdest == NULL || MCdragaction == DRAG_ACTION_NONE))
	{
		// We are only the source
		m_drag_end_sent = true;
		MCdragsource -> message(MCM_drag_end);

		// OK-2008-10-21 : [[Bug 7316]] - Cursor in script editor follows mouse after dragging to non-Revolution target.
		// I have no idea why this apparently only happens in the script editor, but this seems to fix it and doesn't seem too risky :)
		// MW-2008-10-28: [[ Bug 7316 ]] - This happens because the script editor is doing stuff with drag messages
		//   causing the default engine behaviour to be overriden. In this case, some things have to happen to the field
		//   when the drag is over. Note that we have to check that the source was a field in this case since we don't
		//   need to do anything if it is not!
		if (MCdragsource -> gettype() == CT_FIELD)
		{
			MCField *t_field;
			t_field = static_cast<MCField *>(MCdragsource);
			t_field -> setstate(False, CS_DRAG_TEXT);
			t_field -> computedrag();
			t_field -> getstack() -> resetcursor(True);
		}

		return;
	}
	
	if (p_source)
		return;

	// Setup global variables for a field drop
	MCdropfield = NULL;
	MCdropchar = 0;

	int4 t_start_index, t_end_index;
	t_start_index = t_end_index = 0;
	if (MCdragdest != NULL && MCdragdest -> gettype() == CT_FIELD)
	{
		MCdropfield = static_cast<MCField *>(MCdragdest);
		if (MCdragdest -> getstate(CS_DRAG_TEXT))
		{
			MCdropfield -> locmark(False, False, False, False, True, t_start_index, t_end_index);
			MCdropchar = t_start_index;
		}
	}

	// If source is a field and the engine handled the start of the drag operation
	bool t_auto_source;
	t_auto_source = MCdragsource != NULL && MCdragsource -> gettype() == CT_FIELD && MCdragsource -> getstate(CS_SOURCE_TEXT);

	// If dest is a field and the engine handled the accepting of the operation
	bool t_auto_dest;
	t_auto_dest = MCdragdest != NULL && MCdragdest -> gettype() == CT_FIELD && MCdragdest -> getstate(CS_DRAG_TEXT);

	if (t_auto_source && t_auto_dest && MCdragsource == MCdragdest)
	{
		// Source and target is the same field
		MCField *t_field;
		t_field = static_cast<MCField *>(MCdragsource);

		int4 t_from_start_index, t_from_end_index;
		t_field -> selectedmark(False, t_from_start_index, t_from_end_index, False, False);

		// We are dropping in the target selection - so just send the messages and do nothing
		if (t_start_index >= t_from_start_index && t_start_index < t_from_end_index)
		{
			t_field -> message(MCM_drag_drop);
			t_field -> message(MCM_drag_end);
			t_field -> setstate(False, CS_DRAG_TEXT);
			t_field -> computedrag();
			t_field -> getstack() -> resetcursor(True);
			return;
		}

		if (t_field -> message(MCM_drag_drop) != ES_NORMAL)
		{
			MCParagraph *t_paragraphs;
			t_paragraphs = MCdragdata -> FetchParagraphs(MCdropfield);

			// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
			//   textChanged message by a lock screen pair.
			MCRedrawLockScreen();

			if (MCdragaction == DRAG_ACTION_MOVE)
			{
				MCdropfield -> movetext(t_paragraphs, t_start_index);
				Ustruct *us = MCundos->getstate();
				if (us != NULL && us->type == UT_MOVE_TEXT)
					MCdropfield->seltext(us -> ud.text.index, us -> ud.text.index + us->ud.text.newchars, False, True);
			}
			else
			{
				MCdropfield -> seltext(t_start_index, t_start_index, True);

				MCdropfield -> pastetext(t_paragraphs, true);

				Ustruct *us = MCundos->getstate();
				if (us != NULL && us->type == UT_TYPE_TEXT)
					MCdropfield->seltext(t_start_index, t_start_index + us->ud.text.newchars, False, True);
			}

			// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
			//   textChanged message by a lock screen pair.
			MCRedrawUnlockScreen();
			
			// MW-2012-02-08: [[ TextChanged ]] Invoke textChanged as this method
			//   was called as a result of a user action (result of drop in field).
			MCactivefield -> textchanged();
		}

		MCdropfield->setstate(False, CS_DRAG_TEXT);
		MCdropfield->computedrag();
		MCdropfield -> getstack() -> resetcursor(True);

		return;
	}

	int4 t_src_start, t_src_end;
	t_src_start = t_src_end = 0;
	if (t_auto_source)
		static_cast<MCField *>(MCdragsource) -> selectedmark(False, t_src_start, t_src_end, False, False);

	bool t_auto_drop;
	t_auto_drop = MCdragdest != NULL && MCdragdest -> message(MCM_drag_drop) != ES_NORMAL;

	if (t_auto_dest && t_auto_drop && MCdragdata != NULL && MCdropfield != NULL)
	{
		// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
		//   textChanged message by a lock screen pair.
		MCRedrawLockScreen();

		// Process an automatic drop action
		MCdropfield -> seltext(t_start_index, t_start_index, True);

		MCParagraph *t_paragraphs;
		t_paragraphs = MCdragdata -> FetchParagraphs(MCdropfield);
		MCdropfield -> pastetext(t_paragraphs, true);

		Ustruct *us = MCundos->getstate();
		if (us != NULL && us->type == UT_TYPE_TEXT)
			MCdropfield->seltext(t_start_index, t_start_index + us->ud.text.newchars, False, True);
		MCdropfield->setstate(False, CS_DRAG_TEXT);
		MCdropfield->computedrag();
		MCdropfield -> getstack() -> resetcursor(True);
		
		// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
		//   textChanged message by a lock screen pair.
		MCRedrawUnlockScreen();

		// MW-2012-02-08: [[ TextChanged ]] Invoke textChanged as this method
		//   was called as a result of a user action (drop from different field).
		MCactivefield -> textchanged();
	}
	else if (MCdropfield != NULL)
	{
		MCdropfield->setstate(False, CS_DRAG_TEXT);
		MCdropfield->computedrag();
		MCdropfield -> getstack() -> resetcursor(True);
	}

	bool t_auto_end;
	if (MCdragsource != NULL)
	{
		m_drag_end_sent = true;
		t_auto_end = MCdragsource -> message(MCM_drag_end) != ES_NORMAL;
	}
	else
		t_auto_end = false;

	if (t_auto_source && t_auto_end && MCdragsource != NULL && MCdragaction == DRAG_ACTION_MOVE)
	{
		// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
		//   textChanged message by a lock screen pair.
		MCRedrawLockScreen();
		static_cast<MCField *>(MCdragsource) -> deletetext(t_src_start, t_src_end);
		MCRedrawUnlockScreen();

		// MW-2012-02-08: [[ TextChanged ]] Invoke textChanged as this method
		//   was called as a result of a user action (move from one field to another).
		MCactivefield -> textchanged();
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCDispatch::clearcursors(void)
{
	for(uint32_t i = 0; i < PI_NCURSORS; i++)
	{
		if (MCcursor == MCcursors[i])
			MCcursor = nil;
		if (MCdefaultcursor = MCcursors[i])
			MCdefaultcursor = nil;
	}

	MCStack *t_stack;
	t_stack = stacks;
	do
	{
		t_stack -> clearcursor();
		t_stack = t_stack -> next();
	}
	while(t_stack != stacks -> prev());
}

////////////////////////////////////////////////////////////////////////////////

void MCDispatch::changehome(MCStack *stack)
{
	MCStack *t_stack;
	t_stack = stacks;
	do
	{
		if (t_stack -> getparent() == stacks)
			t_stack -> setparent(stack);
		t_stack = t_stack -> next();
	}
	while(t_stack != stacks -> prev());

	stack -> setparent(this);
	stack -> totop(stacks);
}

////////////////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS_DESKTOP
void MCDispatch::freeprinterfonts()
{
	fonts->freeprinterfonts();
}
#endif

void MCDispatch::flushfonts(void)
{
	delete fonts;
	fonts = nil;
}

MCFontlist *MCFontlistGetCurrent(void)
{
	return MCdispatcher -> getfontlist();
}

////////////////////////////////////////////////////////////////////////////////

void MCDispatch::GetDefaultTextFont(MCExecContext& ctxt, MCStringRef& r_font)
{
	if (MCStringCreateWithCString(DEFAULT_TEXT_FONT, r_font))
		return;

	ctxt . Throw();
}

void MCDispatch::GetDefaultTextSize(MCExecContext& ctxt, uinteger_t& r_size)
{
	r_size = DEFAULT_TEXT_SIZE;
}

void MCDispatch::GetDefaultTextStyle(MCExecContext& ctxt, MCInterfaceTextStyle& r_style)
{
	r_style . style = FA_DEFAULT_STYLE;
}

void MCDispatch::GetDefaultTextAlign(MCExecContext& ctxt, intenum_t& r_align)
{
    r_align = F_ALIGN_LEFT;
}

void MCDispatch::GetDefaultTextHeight(MCExecContext& ctxt, uinteger_t& r_height)
{
    r_height = heightfromsize(DEFAULT_TEXT_SIZE);
}

void MCDispatch::GetDefaultForePixel(MCExecContext& ctxt, uinteger_t& r_pixel)
{
    r_pixel = MCscreen->black_pixel.pixel & 0xFFFFFF;
}

void MCDispatch::GetDefaultBackPixel(MCExecContext& ctxt, uinteger_t& r_pixel)
{
    r_pixel = MCscreen->background_pixel.pixel & 0xFFFFFF;
}

void MCDispatch::GetDefaultTopPixel(MCExecContext& ctxt, uinteger_t& r_pixel)
{
    r_pixel = MCscreen->white_pixel.pixel & 0xFFFFFF;
}

void MCDispatch::GetDefaultForeColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
    if (MCStringCreateWithCString("black", r_color . name))
        return;
    
    ctxt . Throw();
}

void MCDispatch::GetDefaultBackColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
    if (MCStringCreateWithCString("white", r_color . name))
        return;
    
    ctxt . Throw();
}

void MCDispatch::GetDefaultPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    r_pattern = nil;
}

////////////////////////////////////////////////////////////////////////////////
