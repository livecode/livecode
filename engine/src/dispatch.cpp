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
#include "scriptpt.h"
#include "widget-events.h"
#include "parentscript.h"

#include "exec.h"
#include "exec-interface.h"
#include "graphics_util.h"

#include "stackfileformat.h"

#define UNLICENSED_TIME 6.0
#ifdef _DEBUG_MALLOC_INC
#define LICENSED_TIME 1.0
#else
#define LICENSED_TIME 3.0
#endif

MCImage *MCDispatch::imagecache;

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
    
    m_showing_mnemonic_underline = false;

	m_externals = nil;

    m_transient_stacks = nil;
    
    // AL-2015-02-10: [[ Standalone Inclusions ]] Add resource mapping array to MCDispatch. This stores
    //  any universal name / relative path pairs included in a standalone executable for locating included
    //  resources.
    /* UNCHECKED */ MCArrayCreateMutable(m_library_mapping);
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
	
	MCMemoryDeleteArray(startdir); /* Allocated by MCStringConvertToCString() */
	MCMemoryDeleteArray(enginedir); /* Allocated by MCStringConvertToCString() */
	
	delete m_externals;
    // AL-2015-02-10: [[ Standalone Inclusions ]] Delete library mapping
    MCValueRelease(m_library_mapping);
}

bool MCDispatch::visit_self(MCObjectVisitor* p_visitor)
{
    return p_visitor -> OnObject(this);
}

bool MCDispatch::isdragsource(void)
{
	return m_drag_source;
}

bool MCDispatch::isdragtarget(void)
{
	return m_drag_target;
}

// bogus "cut" call actually checks license
Boolean MCDispatch::cut(Boolean home)
{
	if (home)
		return True;
	return MCnoui || (flags & F_WAS_LICENSED) != 0;
}

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
        // TODO[19681]: This can be removed when all engine messages are sent with
        // target.
        bool t_target_was_valid = MCtargetptr.IsValid();
    
		Exec_stat oldstat = stat;
		stat = m_externals -> Handle(this, htype, mess, params);

		// MW-2011-08-22: [[ Bug 9686 ]] Make sure we exit as soon as the
		//   message is handled.
		if (stat != ES_NOT_HANDLED && stat != ES_PASS)
			return stat;

		if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
			stat = ES_PASS;
        
        if (stat == ES_PASS || stat == ES_NOT_HANDLED)
        {
            if (t_target_was_valid && !MCtargetptr.IsValid())
            {
                stat = ES_NORMAL;
                t_has_passed = false;
            }
        }
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

    if ((stat == ES_NOT_HANDLED || stat == ES_PASS))
    {
        // TODO[19681]: This can be removed when all engine messages are sent with
        // target.
        bool t_target_was_valid = MCtargetptr.IsValid();
        
        extern Exec_stat MCEngineHandleLibraryMessage(MCNameRef name, MCParameter *params);
        stat = MCEngineHandleLibraryMessage(mess, params);
        
        if (stat == ES_PASS || stat == ES_NOT_HANDLED)
        {
            if (t_target_was_valid && !MCtargetptr.IsValid())
            {
                stat = ES_NORMAL;
                t_has_passed = false;
            }
        }
    }
    
	if (MCmessagemessages && stat != ES_PASS && MCtargetptr)
		MCtargetptr -> sendmessage(htype, mess, False);
		
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
		MCAutoValueRef t_string;
		if (!tstk->names(P_SHORT_NAME, &t_string))
			return false;
		if (!MCListAppend(*t_list, (MCStringRef)*t_string))
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
    /* Make sure no messages are sent when destroying the given stack as this
     * destruction method is only ever used for stacks which are not-yet-alive
     * (e.g. failed to deserialize) or in restricted contexts (e.g. licensing
     * dialog on startup). */
    Boolean oldstate = MClockmessages;
    MClockmessages = True;
    
	if (needremove)
    {
        MCStack *t_substacks = sptr -> getsubstacks();
        while(t_substacks != nullptr)
        {
            /* The MCStack::dodel() method removes the stack from its mainstack 
             * so we must explicitly delete it explicitly. Note that there is
             * no need to scheduledelete() in this case as destroystack() is
             * only called when it is known that no script is running from the
             * stack. */
            t_substacks -> dodel();
            delete t_substacks;
            
            /* Refetch the substacks list - the substack we just processed will
             * have been removed from it. */
            t_substacks = sptr -> getsubstacks();
        }
        
        /* Release any references to the mainstack */
        sptr -> dodel();
    }
	if (sptr == MCstaticdefaultstackptr)
		MCstaticdefaultstackptr = stacks;
	if (sptr == MCdefaultstackptr)
		MCdefaultstackptr = MCstaticdefaultstackptr;
	if (MCacptr && MCacptr->getmessagestack() == sptr)
		MCacptr->setmessagestack(NULL);
    
    /* Delete the stack explicitly. Note that there is no need to use
     * scheduledelete here as destroystack() is only called when it is known
     * that no script is running from the stack. */
	delete sptr;
    
    /* Restore the previous message lock state. */
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
	t_trial_stream = MCS_open(*t_trial_path, kMCOpenFileModeRead, True, False, 0);

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

	if (attempt_to_loadfile(stream, outpath, "%s/%@", startdir, sname))
		return True;

	if (attempt_to_loadfile(stream, outpath, "%s/%@", enginedir, sname))
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
		/* UNCHECKED */ MCStringDivideAtChar(t_rest_of_env, ENV_SEPARATOR, kMCStringOptionCompareExact, &t_env_path, t_next_rest_of_env);
		if (attempt_to_loadfile(stream, outpath, "%@/%@", *t_env_path, sname))
			t_found = true;

		MCValueRelease(t_rest_of_env);
		t_rest_of_env = t_next_rest_of_env;
	}

	MCValueRelease(t_rest_of_env);

	return t_found;
}

IO_stat readheader(IO_handle& stream, uint32_t& r_version)
{
	char tnewheader[kMCStackFileVersionStringLength + 1];
	if (IO_read(tnewheader, kMCStackFileVersionStringLength, stream) != IO_NORMAL)
		return IO_ERROR;
	tnewheader[kMCStackFileVersionStringLength] = '\0'; /* nul-terminate */
	
	// AL-2014-10-27: [[ Bug 12558 ]] Check for valid header prefix
	if (!MCStackFileParseVersionNumber(tnewheader, r_version))
	{
		char theader[kMCStackFileMetaCardVersionStringLength + 1];
		theader[kMCStackFileMetaCardVersionStringLength] = '\0';
		uint4 offset;
		strncpy(theader, tnewheader, kMCStackFileVersionStringLength);
		if (IO_read(theader + kMCStackFileVersionStringLength, kMCStackFileMetaCardVersionStringLength - kMCStackFileVersionStringLength, stream) == IO_NORMAL
			&& MCU_offset(kMCStackFileMetaCardSignature, theader, offset))
		{
			if (theader[offset - 1] != '\n' || theader[offset - 2] == '\r')
			{
				MCresult->sets("stack was corrupted by a non-binary file transfer");
				return IO_ERROR;
			}

			r_version = (theader[offset + kMCStackFileMetaCardSignatureLength] - '0') * 1000;
			r_version += (theader[offset + kMCStackFileMetaCardSignatureLength + 2] - '0') * 100;
		}
		else
			return IO_ERROR;
	}

	return IO_NORMAL;
}

bool MCDispatch::streamstackisscriptonly(IO_handle stream)
{
    uint32_t t_version;
    return readheader(stream, t_version) != IO_NORMAL;
}

// This method reads a stack from the given stream. The stack is set to
// have parent MCDispatch, and filename MCcmd. It is designed to be used
// for embedded stacks/deployed stacks/revlet stacks.
IO_stat MCDispatch::readstartupstack(IO_handle stream, MCStack*& r_stack)
{
    MCAutoStringRef t_filename;
	// MM-2013-10-30: [[ Bug 11333 ]] Set the filename of android mainstack to apk/mainstack (previously was just apk).
	//   This solves relative file path referencing issues.
#ifdef TARGET_SUBPLATFORM_ANDROID
    /* UNCHECKED */ MCStringFormat(&t_filename, "%@/mainstack", MCcmd);
#else
   	t_filename = MCcmd;
#endif

    const char* t_result = nullptr;
    MCStack* t_stack = nullptr;
    if (trytoreadbinarystack(*t_filename, kMCEmptyString, stream, this,
                             t_stack, t_result) != IO_NORMAL ||
        t_stack == nullptr)
    {
        return IO_ERROR;
    }
    
    // We are reading the startup stack, so this becomes the root of the
    // stack list. This must happen prior to resolving parent scripts
    // because otherwise there are no mainstacks, which can cause a
    // crash when searching substacks.
    stacks = t_stack;
    
	// Mark the stack as needed parentscript resolution. This is done after
	// aux stacks have been loaded.
	if (s_loaded_parent_script_reference)
		t_stack -> setextendedstate(True, ECS_USES_PARENTSCRIPTS);
    
    r_stack = t_stack;
	return IO_NORMAL;
}

IO_stat MCDispatch::readscriptonlystartupstack(IO_handle stream, uindex_t p_length, MCStack*& r_stack)
{
    MCAutoStringRef t_filename;
    // MM-2013-10-30: [[ Bug 11333 ]] Set the filename of android mainstack to apk/mainstack (previously was just apk).
    //   This solves relative file path referencing issues.
#ifdef TARGET_SUBPLATFORM_ANDROID
    /* UNCHECKED */ MCStringFormat(&t_filename, "%@/mainstack", MCcmd);
#else
   	t_filename = MCcmd;
#endif
    
    const char* t_result = nullptr;
    MCStack* t_stack = nullptr;
    
    // Read a script-only stack from the stream
    if (trytoreadscriptonlystackofsize(*t_filename, stream,
                                       p_length, this,
                                       t_stack, t_result) != IO_NORMAL
        || t_stack == nullptr)
        return IO_ERROR;

    // We are reading the startup stack, so this becomes the root of the
    // stack list.
    stacks = t_stack;
    
    // Mark the stack as needed parentscript resolution. This is done after
    // aux stacks have been loaded.
    if (s_loaded_parent_script_reference)
        t_stack -> setextendedstate(True, ECS_USES_PARENTSCRIPTS);
    
    r_stack = t_stack;
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

IO_stat MCDispatch::trytoreadbinarystack(MCStringRef p_openpath,
                                         MCStringRef p_name,
                                         IO_handle &x_stream,
                                         MCObject* p_parent,
                                         MCStack* &r_stack,
                                         const char* &r_result)
{
    uint32_t t_version;
    if (readheader(x_stream, t_version) != IO_NORMAL)
    {
        return IO_NORMAL;
    }
    
    if (t_version > kMCStackFileFormatCurrentVersion)
    {
        r_result = "stack was produced by a newer version";
        return checkloadstat(IO_ERROR);
    }
    
    // MW-2008-10-20: [[ ParentScripts ]] Set the boolean flag that tells us whether
    //   parentscript resolution is required to false.
    s_loaded_parent_script_reference = false;
    
    // MW-2013-11-19: [[ UnicodeFileFormat ]] newsf is no longer used.
    uint1 charset, type;
    if (IO_read_uint1(&charset, x_stream) != IO_NORMAL
        || IO_read_uint1(&type, x_stream) != IO_NORMAL
        || IO_discard_cstring_legacy(x_stream, 2) != IO_NORMAL)
    {
        r_result = "stack is corrupted, check for ~ backup file";
        return checkloadstat(IO_ERROR);
    }

    MCtranslatechars = charset != CHARSET;

    MCStack *t_stack;
    if (!MCStackSecurityCreateStack(t_stack))
    {
        r_result = "couldn't create stack";
        return checkloadstat(IO_ERROR);
    }
    
    if (p_parent != nullptr)
        t_stack -> setparent(p_parent);
    else if (stacks != nullptr)
        t_stack->setparent(stacks);
    else
        t_stack->setparent(this);

    t_stack->setfilename(p_openpath);
    
    if (MCModeCanLoadHome() && type == OT_HOME)
    {
        // MW-2013-11-19: [[ UnicodeFileFormat ]] These strings are never written out, so
        //   legacy.
        if (IO_discard_cstring_legacy(x_stream, 2) != IO_NORMAL
            || IO_discard_cstring_legacy(x_stream, 2) != IO_NORMAL)
        {
            r_result = "stack is corrupted, check for ~ backup file";
            return checkloadstat(IO_ERROR);
        }
    }
    
    if (IO_read_uint1(&type, x_stream) != IO_NORMAL
        || (type != OT_STACK && type != OT_ENCRYPT_STACK)
        || t_stack->load(x_stream, t_version, type) != IO_NORMAL)
    {
        r_result = "stack is corrupted, check for ~ backup file";
        destroystack(t_stack, False);
        return checkloadstat(IO_ERROR);
    }
    
    // MW-2011-08-09: [[ Groups ]] Make sure F_GROUP_SHARED is set
    //   appropriately.
    t_stack -> checksharedgroups();
    
    if (t_stack->load_substacks(x_stream, t_version) != IO_NORMAL
        || IO_read_uint1(&type, x_stream) != IO_NORMAL
        || type != OT_END)
    {
        r_result = "stack is corrupted, check for ~ backup file";
        destroystack(t_stack, False);
        return checkloadstat(IO_ERROR);
    }
    
    r_stack = t_stack;
    return IO_NORMAL;
}

static MCStack* script_only_stack_from_bytes(uint8_t *p_bytes,
                                             uindex_t p_size,
                                             MCStringEncoding p_encoding)
{
    MCAutoStringRef t_raw_script_string, t_lc_script_string;
    MCStringLineEndingStyle t_line_encoding_style;
    
    if (!MCStringCreateWithBytes(p_bytes, p_size, p_encoding, false,
                                 &t_raw_script_string) ||
        !MCStringNormalizeLineEndings(*t_raw_script_string,
                                      kMCStringLineEndingStyleLF, 
                                      kMCStringLineEndingOptionNormalizePSToLineEnding |
                                      kMCStringLineEndingOptionNormalizeLSToVT,
                                      &t_lc_script_string,
                                      &t_line_encoding_style))
    {
        return nullptr;
    }
    
    // Now attempt to parse the header line:
    //   'script' <string>
    MCScriptPoint sp(*t_lc_script_string);
    
    // Parse 'script' token.
    if (sp . skip_token(SP_FACTOR, TT_PROPERTY, P_SCRIPT) != PS_NORMAL)
    {
        return nullptr;
    }
    
    // Parse <string> token.
    Symbol_type t_type;
    if (sp . next(t_type) != PS_NORMAL || t_type != ST_LIT)
    {
        return nullptr;
    }
    
    MCNewAutoNameRef t_script_name = sp.gettoken_nameref();
    
    // If 'with' is next then parse the behavior reference.
    MCNewAutoNameRef t_behavior_name;
    if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
    {
        // Ensure 'behavior' is next 
        if (sp.skip_token(SP_FACTOR, TT_PROPERTY, P_PARENT_SCRIPT) != PS_NORMAL)
        {
            return nullptr;
        }
        
        // Read the behavior name
        if (sp.next(t_type) != PS_NORMAL || t_type != ST_LIT)
        {
            return nullptr;
        }
        
        t_behavior_name = sp.gettoken_nameref();
    }

    // Parse end of line.
    Parse_stat t_stat;
    t_stat = sp . next(t_type);
    if (t_stat != PS_EOL && t_stat != PS_EOF)
        return nullptr;
    
    // MW-2014-10-23: [[ Bug ]] Make sure we trim the correct number of lines.
    // SN-2014-10-16: [[ Merge-6.7.0-rc-3 ]] Update to StringRef
    // Now trim the ep down to the remainder of the script.
    // Trim the header.
    uint32_t t_lines = sp.getline();
    uint32_t t_index = 0;
    
    // Jump over the possible lines before the string token
    while (MCStringFirstIndexOfChar(*t_lc_script_string, '\n', t_index,
                                    kMCStringOptionCompareExact, t_index)
           && t_lines > 0)
    {
        t_lines -= 1;
    }
    
    // Add one to the index so we include the LF
    t_index += 1;
    
    // t_line now has the last LineFeed of the token
    MCAutoStringRef t_lc_script_body;
    
    // We copy the body of the stack script
    if (!MCStringCopySubstring(*t_lc_script_string,
                               MCRangeMake(t_index,
                                           MCStringGetLength(*t_lc_script_string)
                                           - t_index), &t_lc_script_body))
    {
        return nullptr;
    }
    
    // Create a stack.
    MCStack *t_stack;
    if (!MCStackSecurityCreateStack(t_stack))
        return nullptr;
    
    // Set it up as script only.
    t_stack -> setasscriptonly(*t_lc_script_body);
    
    // Set its name.
    t_stack -> setname(*t_script_name);
    
    // Save line endings from raw script string to restore when saving file.
    t_stack -> setlineencodingstyle(t_line_encoding_style);

    // If we parsed a behavior reference, then set it.
    if (*t_behavior_name != nullptr)
    {
        t_stack->setparentscript_onload(0, *t_behavior_name);
    }
    
    return t_stack;
}

IO_stat MCDispatch::trytoreadscriptonlystackofsize(MCStringRef p_openpath,
                                                   IO_handle &x_stream,
                                                   uindex_t p_size,
                                                   MCObject* p_parent,
                                                   MCStack* &r_stack,
                                                   const char* &r_result)
{
    MCAutoPointer<byte_t> t_bytes = new byte_t[p_size];
    if (IO_read(*t_bytes, p_size, x_stream) == IO_ERROR)
    {
        return checkloadstat(IO_ERROR);
    }
    
    uindex_t t_bom_size = 0;
    MCFileEncodingType t_file_encoding =
        MCS_resolve_BOM_from_bytes(*t_bytes, p_size, t_bom_size);
    
    MCStringEncoding t_string_encoding;
    switch (t_file_encoding)
    {
        case kMCFileEncodingUTF8:
            t_string_encoding = kMCStringEncodingUTF8;
            break;
        case kMCFileEncodingUTF16:
            t_string_encoding = kMCStringEncodingUTF16;
            break;
        case kMCFileEncodingUTF16BE:
            t_string_encoding = kMCStringEncodingUTF16BE;
            break;
        case kMCFileEncodingUTF16LE:
            t_string_encoding = kMCStringEncodingUTF16LE;
            break;
        default:
            // Assume native
            t_string_encoding = kMCStringEncodingNative;
            break;
    }
    
    MCStack *t_stack = script_only_stack_from_bytes(*t_bytes + t_bom_size,
                                                    p_size - t_bom_size,
                                                    t_string_encoding);
    if (t_stack == nullptr)
        return IO_NORMAL;
    
    // Set its parent.
    if (p_parent != nullptr)
        t_stack -> setparent(p_parent);
    else if (stacks != nullptr)
        t_stack->setparent(stacks);
    else
        t_stack->setparent(this);
    
    // Set its filename.
    t_stack->setfilename(p_openpath);
    
    // Make it invisible
    t_stack -> setflag(False, F_VISIBLE);
    
    r_stack = t_stack;
    return IO_NORMAL;
}

IO_stat MCDispatch::trytoreadscriptonlystack(MCStringRef p_openpath,
                                             IO_handle &x_stream,
                                             MCObject* p_parent,
                                             MCStack* &r_stack,
                                             const char* &r_result)
{
    // Load the file into memory - we need to process a byteorder mark and any
    // line endings.
    uindex_t t_size = static_cast<uindex_t>(MCS_fsize(x_stream));
    
    if (trytoreadscriptonlystackofsize(p_openpath, x_stream, t_size,
                                       p_parent, r_stack, r_result)
        != IO_NORMAL)
    {
        r_result = "failed to load script only stack";
        return checkloadstat(IO_ERROR);
    }
    
    return IO_NORMAL;
}

void MCDispatch::processstack(MCStringRef p_openpath, MCStack* &x_stack)
{
    if (stacks != NULL)
    {
        MCStack *tstk = stacks;
        do
        {
            if (x_stack->hasname(tstk->getname()))
            {
                MCNewAutoNameRef t_stack_name = x_stack->getname();
                
                delete x_stack;
                x_stack = nullptr;
                
                
                if (MCStringIsEqualTo(tstk -> getfilename(), p_openpath, kMCStringOptionCompareCaseless))
                    x_stack = tstk;
                else
                {
                    MCdefaultstackptr->getcard()->message_with_valueref_args(MCM_reload_stack, tstk->getname(), p_openpath);
                    tstk = stacks;
                    do
                    {
                        if (MCNameIsEqualToCaseless(*t_stack_name, tstk->getname()))
                        {
                            x_stack = tstk;
                            break;
                        }
                        tstk = (MCStack *)tstk->next();
                    }
                    while (tstk != stacks);
                }
                return;
            }
            tstk = (MCStack *)tstk->next();
        }
        while (tstk != stacks);
    }
    
    appendstack(x_stack);
    
    x_stack->extraopen(false);
    
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
    {
        x_stack -> resolveparentscripts();
        x_stack -> setextendedstate(True, ECS_USES_PARENTSCRIPTS);
    }
}

// MW-2012-02-17: [[ LogFonts ]] Actually load the stack file (wrapped by readfile
//   to handle font table cleanup).
IO_stat MCDispatch::doreadfile(MCStringRef p_openpath, MCStringRef p_name, IO_handle &stream, MCStack* &r_stack)
{
    MCresult -> clear();
    
    const char* t_result = nullptr;
    MCStack *t_stack = nullptr;
    
	if (trytoreadbinarystack(p_openpath, p_name, stream, nullptr,
                             t_stack, t_result) != IO_NORMAL)
    {
        /* UNCHECKED */ MCresult -> setvalueref(MCSTR(t_result));
        return IO_ERROR;
    }
    
    // If there was no IO error but it wasn't a binary stack then try as script-only
    if (t_stack == nullptr)
    {
        // Reset to position 0.
        MCS_seek_set(stream, 0);
    
        if (trytoreadscriptonlystack(p_openpath, stream, nullptr,
                                     t_stack, t_result) != IO_NORMAL)
        {
            /* UNCHECKED */ MCresult -> setvalueref(MCSTR(t_result));
            return IO_ERROR;
        }
    }
    
    // MW-2014-09-30: [[ ScriptOnlyStack ]] If we managed to load a stack as either binary
    //   or script, then do the normal processing.
    if (t_stack != nullptr)
    {
        processstack(p_openpath, t_stack);
        r_stack = t_stack;
        return IO_NORMAL;
    }
    
    // MW-2014-09-30: [[ ScriptOnlyStack ]] Finally attempt to load the script in legacy
    //   modes - either as a single script, or as a HyperCard conversion.
    MCS_seek_set(stream, 0);
    if (stacks == NULL)
    {
        MCnoui = True;
        MCscreen = new (nothrow) MCUIDC;
        /* UNCHECKED */ MCStackSecurityCreateStack(stacks);
        MCdefaultstackptr = MCstaticdefaultstackptr = stacks;
        stacks->setparent(this);
        stacks->setname_cstring("revScript");
        uint4 size = (uint4)MCS_fsize(stream);
        /* UNCHECKED */ MCAutoPointer<char[]> script = new (nothrow) char[size + 2];
        script[size] = '\n';
        script[size + 1] = '\0';
        if (IO_read(*script, size, stream) != IO_NORMAL)
            return IO_ERROR;
        MCAutoStringRef t_script_str;
        /* UNCHECKED */ MCStringCreateWithCString(*script, &t_script_str);
        if (!stacks -> setscript_from_commandline(*t_script_str))
            return IO_ERROR;
    }
    else
    {
        // MW-2008-06-12: [[ Bug 6476 ]] Media won't open HC stacks
        if (!MCdispatcher->cut(True) || hc_import(p_name, stream, t_stack) != IO_NORMAL)
        {
            MCresult->sets("file is not a stack");
            return IO_ERROR;
        }
        r_stack = t_stack;
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
		if ((stream = MCS_open(p_name, kMCOpenFileModeRead, True, False, 0)) != NULL)
        {
            // SN-20015-06-01: [[ Bug 15432 ]] We want to use MCS_resolvepath to
            //  keep consistency and let '~' be resolved as it is in MCS_open
            //  MCS_resolve_path leaves a backslash-delimited path on Windows,
            //  and MCS_get_canonical_path is made to cope with this.
            //  In 7.0, MCS_resolvepath does not return a native path.
            t_found = MCS_resolvepath(p_name, &t_open_path);
		}
	}
    
	if (!t_found)
    {
        // SN-2014-11-18: [[ Bug 14043 ]] If p_path is was not correct, we then use the leaf, and append it to different locations
        //  in all the next steps.
        MCAutoStringRef t_leaf_name;
		uindex_t t_leaf_index;
		if (MCStringLastIndexOfChar(p_name, PATH_SEPARATOR, UINDEX_MAX, kMCStringOptionCompareExact, t_leaf_index))
			/* UNCHECKED */ MCStringCopySubstring(p_name, MCRangeMakeMinMax(t_leaf_index + 1, MCStringGetLength(p_name)), &t_leaf_name);
		else
			t_leaf_name = p_name;
		if ((stream = MCS_open(*t_leaf_name, kMCOpenFileModeRead, True, False, 0)) != NULL)
        {
            t_found = MCS_resolvepath(*t_leaf_name, &t_open_path);
		}

        if (!t_found)
        {
            // SN-2014-11-18: [[ Bug 14043 ]] The whole path was appended, instead of only the leaf.
            if (openstartup(*t_leaf_name, &t_open_path, stream) ||
                openenv(*t_leaf_name, MCSTR("MCPATH"), &t_open_path, stream, 0) ||
                openenv(*t_leaf_name, MCSTR("PATH"), &t_open_path, stream, 0))
                t_found = true;
        }
        
        if (!t_found)
        {
            
            MCAutoStringRef t_homename;
            if (MCS_getenv(MCSTR("HOME"), &t_homename) && !MCStringIsEmpty(*t_homename))
            {
                MCAutoStringRef t_trimmed_homename;
                if (MCStringGetCharAtIndex(*t_homename, MCStringGetLength(*t_homename) - 1) == '/')
                /* UNCHECKED */ MCStringCopySubstring(*t_homename, MCRangeMake(0, MCStringGetLength(*t_homename) - 1), &t_trimmed_homename);
                else
                    t_trimmed_homename = *t_homename;
                
                // SN-2014-11-18: [[ Bug 14043 ]] The whole path was appended, instead of only the leaf.
                if (!t_found)
                    t_found = attempt_to_loadfile(stream, &t_open_path, "%@/%@", *t_trimmed_homename, *t_leaf_name);
                
                if (!t_found)
                    t_found = attempt_to_loadfile(stream, &t_open_path, "%@/stacks/%@", *t_trimmed_homename, *t_leaf_name);
                
                if (!t_found)
                    t_found = attempt_to_loadfile(stream, &t_open_path, "%@/components/%@", *t_trimmed_homename, *t_leaf_name);
            }
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

IO_stat MCDispatch::savestack(MCStack *sptr, const MCStringRef p_fname, uint32_t p_version)
{
    IO_stat stat;
    
    // MW-2014-09-30: [[ ScriptOnlyStack ]] If the stack is scriptOnly, then save
    //   it differently.
    if (sptr -> isscriptonly())
    {
        stat = dosavescriptonlystack(sptr, p_fname);
    }
    else
    {
		/* If no version was specified, assume that current format was requested */
		if (UINT32_MAX == p_version)
		{
			p_version = kMCStackFileFormatCurrentVersion;
		}

		/* If the stack doesn't contain any features requiring a more recent version, use 7.0 format. */
		if (p_version > kMCStackFileFormatVersion_7_0 && sptr->geteffectiveminimumstackfileversion() <= kMCStackFileFormatVersion_7_0)
			p_version = kMCStackFileFormatVersion_7_0;

        stat = dosavestack(sptr, p_fname, p_version);
        
        MCLogicalFontTableFinish();
    }
    
	return stat;
}

// MW-2014-09-30: [[ ScriptOnlyStack ]] Script only stacks get saved as a text file.
//   Everything but the stack script is lost.
IO_stat MCDispatch::dosavescriptonlystack(MCStack *sptr, const MCStringRef p_fname)
{
    if (MCModeCheckSaveStack(sptr, p_fname) != IO_NORMAL)
		return IO_ERROR;
	
    MCAutoStringRef linkname;
    if (!MCStringIsEmpty(p_fname))
        linkname = p_fname;
	else if (!MCStringIsEmpty(sptr -> getfilename()))
		linkname = sptr -> getfilename();
    else
    {
        MCresult->sets("stack does not have a filename");
        return IO_ERROR;
    }

    if (*linkname == NULL)
	{
		MCresult->sets("can't open stack script file, bad path");
		return IO_ERROR;
	}
	if (MCS_noperm(*linkname))
	{
		MCresult->sets("can't open stack script file, no permission");
		return IO_ERROR;
	}
    
    // Compute the body of the script file.
	MCAutoStringRef t_converted;

	// MW-2014-10-24: [[ Bug 13791 ]] We need to post-process the generated string on some
	//   platforms for line-ending conversion so temporarily need a stringref - hence we
	//   put the processing in its own block.
	{
		MCAutoStringRef t_script_body;

        // Ensure script isn't encrypted if a password was removed in session
        sptr -> unsecurescript(sptr);
        
        // Write out the standard script stack header with behavior reference 
        // (if applicable) and then the script itself
        MCParentScript *t_parent_script = sptr->getparentscript();
        if (t_parent_script != nullptr &&
            t_parent_script->GetObjectId() == 0)
        {
            MCStringFormat(&t_script_body,
                           "script \"%@\" with behavior \"%@\"\n%@",
                           sptr->getname(),
                           t_parent_script->GetObjectStack(),
                           sptr->_getscript());
        }
        else
        {
            MCStringFormat(&t_script_body, "script \"%@\"\n%@", sptr -> getname(), sptr->_getscript());
        }

        MCStringNormalizeLineEndings(*t_script_body, 
                                     sptr -> getlineencodingstyle(), 
                                     false,
                                     &t_converted, 
                                     nullptr);
	}
    
    // Open the output stream.
	IO_handle stream;
    if ((stream = MCS_open(*linkname, kMCOpenFileModeWrite, True, False, 0)) == NULL)
	{
		MCresult->sets("can't open stack script file");
        return IO_ERROR;
    }

    // Convert the output string to UTF-8
    MCAutoStringRefAsUTF8String t_utf8_script;
    t_utf8_script .Lock(*t_converted);

    // Write out the byte-order mark, followed by the script body.
    if (IO_write("\xEF\xBB\xBF", 1, 3, stream) != IO_NORMAL ||
        IO_write(*t_utf8_script, 1, t_utf8_script . Size(), stream) != IO_NORMAL)
    {
        MCresult -> sets("error writing stack script file");
        MCS_close(stream);
        return IO_ERROR;
    }
    
    // Close the stream.
    MCS_close(stream);
    
    // Set the filename.
    sptr->setfilename(*linkname);
    
    return IO_NORMAL;
}

IO_stat MCDispatch::dosavestack(MCStack *sptr, const MCStringRef p_fname, uint32_t p_version)
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
		MCresult -> sets("stack does not have a filename");
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
	/* UNCHECKED */ MCStringFormat(&t_backup, "%@~", *t_linkname); 

	MCS_unlink(*t_backup);
	if (MCS_exists(*t_linkname, True) && !MCS_backup(*t_linkname, *t_backup))
	{
		MCresult->sets("can't open stack backup file");

		MCValueAssign(MCfiletype, oldfiletype);
		return IO_ERROR;
	}
	IO_handle stream;

	if ((stream = MCS_open(*t_linkname, kMCOpenFileModeWrite, True, False, 0)) == NULL)
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
	MCStackFileGetHeaderForVersion(p_version, t_header, t_header_size);
	
	if (IO_write(t_header, sizeof(char), t_header_size, stream) != IO_NORMAL
	        || IO_write_uint1(CHARSET, stream) != IO_NORMAL)
	{
		MCresult->sets(errstring);
		cleanup(stream, *t_linkname, *t_backup);
		return IO_ERROR;
	}
	
	// MW-2013-11-19: [[ UnicodeFileFormat ]] Writing out for backwards-compatibility,
	//   so legacy.
	if (IO_write_uint1(OT_NOTHOME, stream) != IO_NORMAL
	        || IO_write_cstring_legacy(NULL, stream, 2) != IO_NORMAL)
	{ // was stackfiles
		MCresult->sets(errstring);
		cleanup(stream, *t_linkname, *t_backup);
		return IO_ERROR;
	}
	
	// MW-2012-02-22; [[ NoScrollSave ]] Adjust the rect by the current group offset.
	MCgroupedobjectoffset . x = 0;
	MCgroupedobjectoffset . y = 0;
	
	MCresult -> clear();
	if (sptr->save(stream, 0, false, p_version) != IO_NORMAL
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

// Important: This function is on the emterpreter whitelist. If its
// signature function changes, the mangled name must be updated in
// em-whitelist.json
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

Boolean MCDispatch::wkdown(Window w, MCStringRef p_string, KeySym key)
{
    // Trigger a redraw as mnemonic underlines will need to be drawn
    if ((MCmodifierstate & MS_ALT) && !m_showing_mnemonic_underline)
    {
        m_showing_mnemonic_underline = true;
        MCRedrawDirtyScreen();
    }
    
    if (menu != NULL)
		return menu->kdown(p_string, key);
    
	MCStack *target = findstackd(w);
	if (target == NULL || !target->kdown(p_string, key))
	{
		if (MCmodifierstate & MS_MOD1)
		{
            MCButton *bptr = MCstacks->findmnemonic(key);
			if (bptr != NULL)
			{
				bptr->activate(True, key);
				return True;
			}
		}
	}
	else
		if (target != NULL)
			return True;
	return False;
}

void MCDispatch::wkup(Window w, MCStringRef p_string, KeySym key)
{
    // Trigger a redraw as mnemonic underlines will need to be cleared
    if (!(MCmodifierstate & MS_ALT) && m_showing_mnemonic_underline)
    {
        m_showing_mnemonic_underline = false;
        MCRedrawDirtyScreen();
    }
    
    if (menu != NULL)
		menu->kup(p_string, key);
	else
	{
        MCStack *target = findstackd(w);
		if (target != NULL)
			target->kup(p_string, key);
	}
}

void MCDispatch::wmfocus_stack(MCStack *target, int2 x, int2 y)
{
	// IM-2013-09-23: [[ FullscreenMode ]] transform view -> stack coordinates
	MCPoint t_stackloc;
	t_stackloc = MCPointMake(x, y);

	// IM-2014-02-12: [[ StackScale ]] mfocus will translate target stack to menu stack coords
	//   so in both cases we pass target stack coords.
	// IM-2014-02-14: [[ StackScale ]] Don't try to convert if target is null
	if (target != nil)
		t_stackloc = target->windowtostackloc(t_stackloc);

	if (menu != NULL)
		menu->mfocus(t_stackloc.x, t_stackloc.y);
	else if (target != NULL)
		target->mfocus(t_stackloc.x, t_stackloc.y);
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

    // Don't re-enter the drag-and-drop modal loop
	if (isdragsource())
		return;

	MCStack *target = findstackd(w);

	if (target != NULL)
		target->mdrag();

	// Did an object indicate that it is draggable (by setting itself as the
    // drag object) and by putting some data on the drag board?
	if (!MCdragboard->IsEmpty() && MCdragtargetptr)
	{
		m_drag_source = true;
		m_drag_end_sent = false;

		// Search for the appropriate image object using the standard method - note
		// here we search relative to the target of the dragStart message.
		MCImage *t_image;
		t_image = NULL;
		if (MCdragimageid != 0)
			t_image = MCdragtargetptr ? MCdragtargetptr -> resolveimageid(MCdragimageid) : resolveimageid(MCdragimageid);
		
		MCdragsource = MCdragtargetptr;

		// PLATFORM-TODO: This is needed at the moment to make sure that we don't
		//   get the selection 'going away' when we start dragging. At the moment
		//   MouseRelease is mapped to mup without messages, which isn't quite
		//   correct from the point of view of the field.
		if (MCdragtargetptr->gettype() > CT_CARD)
		{
			/* FIXME This is horrible */
			if (MCdragtargetptr->gettype() != CT_WIDGET)
			{
				MCdragtargetptr.GetAs<MCControl>()->munfocus();
			}
			else
			{
				MCwidgeteventmanager->event_munfocus(MCdragtargetptr.GetAs<MCWidget>());
			}
			MCdragtargetptr->getcard()->ungrab();
		}
		MCdragtargetptr->getstack()->resetcursor(True);
		MCdragtargetptr -> getstack() -> munfocus();
		
        // Ensure all of the data placed onto the drag board has been passed to
        // the OS' drag board.
        MCdragboard->PushUpdates(true);
        
        // Begin the drag-drop modal loop
		MCdragaction = MCscreen -> dodragdrop(w, MCallowabledragactions, t_image, t_image != NULL ? &MCdragimageoffset : NULL);

        // Perform the drop operation.
		dodrop(true);
        
        // Clear the drag board as its contents are no longer required. A manual
        // push is required as the drag board doesn't update automatically.
        MCdragboard->Clear();
        MCdragboard->PushUpdates(true);

		MCdragsource = nil;
		MCdragdest = nil;
		MCdropfield = nil;
		MCdragtargetptr = nil;
		m_drag_source = false;
	}
	else
	{
        // Ensure that anything that got added to the drag board has been
        // removed (this might happen, for example, if a script encountered
        // an error after placing some of the drag data).
        MCdragboard->Clear();
		MCdragsource = nil;
		MCdragdest = nil;
		MCdropfield = nil;
		MCdragtargetptr = nil;
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
            // We are not currently a source for a drag-and-drop operation so
            // reset all drag settings in case this is the start of one.
            MCallowabledragactions = DRAG_ACTION_COPY;
			MCdragaction = DRAG_ACTION_NONE;
			MCdragimageid = 0;
			MCdragimageoffset . x = 0;
			MCdragimageoffset . y = 0;
            MCdragboard->Clear();
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
		menu->mup(which, false);
	else
	{
		if (target != NULL)
			target->mup(which, false);
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

void MCDispatch::wmdragenter(Window w)
{
    // Find the stack that is being dragged over
    MCStack *target = findstackd(w);
	
    // LiveCode is now the drop target for this drag-and-drop operation
	m_drag_target = true;

    // Update the contents of the drag board
    MCdragboard->PullUpdates();

    // Change the mouse focus to the stack that has had the drag enter it
	if (MCmousestackptr && !MCmousestackptr.IsBoundTo(target))
		MCmousestackptr -> munfocus();

	MCmousestackptr = target;
}

MCDragAction MCDispatch::wmdragmove(Window w, int2 x, int2 y)
{
	// We must also issue a new focus event if the modifierstate
	// changes.
	static uint4 s_old_modifiers = 0;

	MCStack *target = findstackd(w);
    if (target == nil)
        return DRAG_ACTION_NONE;
	
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
    // No stacks have mouse focus now
    MCStack *target = findstackd(w);
	if (target != nullptr && MCmousestackptr.IsBoundTo(target))
	{
		MCmousestackptr -> munfocus();
		MCmousestackptr = nil;
	}
    
    // We are no longer the drop target and no longer care about the drag data.
    MCdragboard->PushUpdates();
    MCdragboard->ReleaseData();
	m_drag_target = false;
}

MCDragAction MCDispatch::wmdragdrop(Window w)
{
	// MW-2011-02-08: Make sure we store the drag action that is in effect now
	//   otherwise it can change as a result of message sends which is bad :o)
	uint32_t t_drag_action;
	t_drag_action = MCdragaction;
	
    // If a drag action was selected, do it.
	if (t_drag_action != DRAG_ACTION_NONE)
		dodrop(false);

    // The drag operation has ended. Remove the drag board contents.
    MCmousestackptr = findstackd(w);
    MCdragboard->Clear();
	m_drag_target = false;

	return t_drag_action;
}

void MCDispatch::property(Window w, Atom atom)
{
}

void MCDispatch::wreshape(Window p_window)
{
	MCStack *t_stack;
	t_stack = findstackd(p_window);
	if (t_stack == nil)
		return;
	
	t_stack -> view_configure(true);
	
	// The wreshape() invocation occurs as a direct result of a system resize window
	// request. These can occur whilst nested inside a system modal loop thus the normal
	// force unlock which occurs at the root loop does not happen. Therefore we
	// do that here to ensure that a 'lock screen' inside a resizeStack does not cause
	// subsequent resize requests (in the same user resizing action) to not have an
	// effect.
	MCRedrawForceUnlockScreen();

	// Now make sure we force an update screen.
	MCRedrawUpdateScreen();
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

MCFontlist *MCDispatch::getfontlist()
{
	/* There's lots of code in the engine that immediately
	 * dereferences the result of getfontlist().  So, *require* it to
	 * return valid font list, by loading a font if necessary. */
	if (nil == fonts)
	{
		MCFontRef t_font;
		bool t_success =
			MCPlatformGetControlThemePropFont(kMCPlatformControlTypeGeneric,
			                                  kMCPlatformControlPartNone,
			                                  kMCPlatformControlStateNormal,
			                                  kMCPlatformThemePropertyTextFont,
			                                  t_font);
		MCAssert(t_success); /* Can't proceed if this fails */
	}

	MCAssert(nil != fonts);
	return fonts;
}

MCFontStruct *MCDispatch::loadfont(MCNameRef fname, uint2 &size, uint2 style, Boolean printer)
{
#if defined(_LINUX_DESKTOP)
	if (fonts == NULL)
		fonts = MCFontlistCreateNew();
    //	if (fonts == NULL)
    //		fonts = MCFontlistCreateOld();
#elif defined(_LINUX_SERVER)
	// MM-2013-09-13: [[ RefactorGraphics ]] Server font support.
	if (fonts == NULL)
		fonts = MCFontlistCreateNew();
#else
	if (fonts == nil)
		fonts = new (nothrow) MCFontlist;
#endif
	return fonts->getfont(fname, size, style, printer);
}

MCFontStruct *MCDispatch::loadfontwithhandle(MCSysFontHandle p_handle, MCNameRef p_name)
{
#if defined(_MACOSX) || defined (_MAC_SERVER) || defined (TARGET_SUBPLATFORM_IPHONE)
    if (fonts == nil)
        fonts = new (nothrow) MCFontlist;
    return fonts->getfontbyhandle(p_handle, p_name);
#else
    return NULL;
#endif
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
		/* UNCHECKED */ MCStringLowercase(*t_name, kMCBasicLocale);
		
		// Remove all special characters from the input string
		// TODO: what about other 'special' chars added by unicode?
        //  => the unicode chars shouldn't be changed
		MCStringRef t_replace = MCSTR("\r\n\t *?<>/\\()[]{}|'`\"");
        uindex_t t_offset;
		for (uindex_t i = 0; i < MCStringGetLength(*t_name); i++)
		{
			if (MCStringFirstIndexOfChar(t_replace, MCStringGetCharAtIndex(*t_name, i), 0, kMCStringOptionCompareExact, t_offset))
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

bool MCDispatch::foreachstack(MCStackForEachCallback p_callback, void *p_context)
{
	bool t_continue;
	t_continue = true;
	
	if (stacks)
	{
		MCStack *t_stack;
		t_stack = stacks;
		
		do
		{
			t_continue = t_stack->foreachstack(p_callback, p_context);
			
			t_stack = (MCStack*)t_stack->next();
		}
		while (t_continue && t_stack != stacks);
	}
	
	return t_continue;
}

bool MCDispatch::foreachchildstack(MCStack *p_stack, MCStackForEachCallback p_callback, void *p_context)
{
	bool t_continue;
	t_continue = true;
	
	if (stacks)
	{
		MCStack *t_stack;
		t_stack = stacks;
		
		do
		{
			t_continue = t_stack->foreachchildstack(p_callback, p_context);
			
			t_stack = (MCStack*)t_stack->next();
		}
		while (t_continue && t_stack != stacks);
	}
	
	return t_continue;
}

MCStack *MCDispatch::findstackwindowid(uintptr_t p_win_id)
{
	if (p_win_id == 0)
		return NULL;
	
	if (stacks != NULL)
	{
		MCStack *tstk = stacks;
		do
		{
			MCStack *foundstk;
			if ((foundstk = tstk->findstackwindowid(p_win_id)) != NULL)
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
			if ((foundstk = tstk->findstackwindowid(p_win_id)) != NULL)
				return foundstk;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != panels);
	}

	if (m_transient_stacks != nil)
    {
        MCStack *tstk = m_transient_stacks;
        do
        {
            MCStack *foundstk;
            if ((foundstk = tstk -> findstackwindowid(p_win_id)) != NULL)
                return foundstk;
			tstk = (MCStack *)tstk->next();
        }
        while(tstk != m_transient_stacks);
    }
    
    return NULL;
}

MCStack *MCDispatch::findstackd(Window w)
{
	// IM-2014-07-09: [[ Bug 12225 ]] Use window ID to find stack
	return findstackwindowid(MCscreen->dtouint((Drawable)w));
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

MCObject *MCDispatch::getobjname(Chunk_term type, MCNameRef p_name)
{
	if (stacks != NULL)
	{
		MCStack *tstk = stacks;
		do
		{
			MCObject *optr;
			if ((optr = tstk->getsubstackobjname(type, p_name)) != NULL)
				return optr;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}

	if (type == CT_IMAGE)
	{
		MCNewAutoNameRef t_image_name;
        uindex_t t_colon;
        t_colon = 0;
        if (MCStringFirstIndexOfChar(MCNameGetString(p_name), ':', 0, kMCCompareExact, t_colon))
			/* UNCHECKED */ t_image_name = MCValueRetain(p_name);
		
		MCImage *iptr = imagecache;
		if (iptr != NULL)
		{
			do
			{
check:
				if (*t_image_name != nil && iptr -> hasname(*t_image_name))
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

        uindex_t t_second_colon;
        if (MCStringFirstIndexOfChar(MCNameGetString(p_name), ':', t_colon, kMCCompareExact, t_second_colon))
		{
            MCExecContext default_ctxt(MCdefaultstackptr, nil, nil);
            MCExecContext *ctxt = MCECptr == NULL ? &default_ctxt : MCECptr;
            default_ctxt . SetTheResultToEmpty();

            MCAutoValueRef t_output;
            MCU_geturl(*ctxt, MCNameGetString(p_name), &t_output);
            // SN-2014-05-09 [[ Bug 12409 ]] Fields in LC 7 fail to display binfile url imagesource
            // isempty is not what we want to use, since it returns false for a cleared result
			if (MCresult->isclear() || MCresult->isempty())
            {
                MCAutoDataRef t_data;

                if (MCValueGetTypeCode(*t_output) == kMCValueTypeCodeData)
                {
                    t_data = (MCDataRef)*t_output;
                }
                else
                {
                    MCAutoStringRef t_output_str;
                    /* UNCHECKED */ ctxt -> ConvertToString(*t_output, &t_output_str);
                    
                    if (MCStringIsNative(*t_output_str))
                    {
                        const char_t *t_bytes = MCStringGetNativeCharPtr(*t_output_str);
                        MCDataCreateWithBytes((byte_t*)t_bytes, MCStringGetLength(*t_output_str), &t_data);
                    }
                    else
                    {
                        const unichar_t *t_bytes = MCStringGetCharPtr(*t_output_str);
                        MCDataCreateWithBytes((byte_t*)t_bytes, MCStringGetLength(*t_output_str) * 2, &t_data);
                    }
                }
                
				iptr = new (nothrow) MCImage;
                iptr->appendto(imagecache);
                iptr->SetText(*ctxt, *t_data);
				iptr->setname(*t_image_name);
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
	{
		MCObject *t_menu = menu;
		menu->closemenu(True, True);
		
		// Ensure the menuobjectptr is set to nil. We can't do this in
		// closemenu itself, as elsewhere menuPick is sent *after*
		// calling closemenu and there is likely to be code that relies
		// on 'the menuButton' during handling of menuPick
		if (MCmenuobjectptr == t_menu)
			MCmenuobjectptr = nil;
	}
}

void MCDispatch::appendpanel(MCStack *sptr)
{
	sptr->appendto(panels);
}

void MCDispatch::removepanel(MCStack *sptr)
{
	sptr->remove(panels);
}

bool MCDispatch::is_transient_stack(MCStack *sptr)
{
	if (m_transient_stacks != NULL)
	{
		MCStack *tstk = m_transient_stacks;
		do
		{
			if (tstk == sptr)
				return true;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != m_transient_stacks);
	}
	return false;
}

void MCDispatch::add_transient_stack(MCStack *sptr)
{
	sptr->appendto(m_transient_stacks);
}

void MCDispatch::remove_transient_stack(MCStack *sptr)
{
    
	sptr->remove(m_transient_stacks);
}

void MCDispatch::timer(MCNameRef p_message, MCParameter *p_parameters)
{
	if (MCNameIsEqualToCaseless(p_message, MCM_internal))
	{
		MCStackSecurityExecutionTimeout();
	}
	else
		MCObject::timer(p_message, p_parameters);
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
		if (!MCStringCopySubstring(p_external, MCRangeMakeMinMax(t_slash_index + 1, t_ext_length), t_external_leaf))
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
#else
    // AL-2015-02-10: [[ SB Inclusions ]] New module loading utility deals with path resolution
    t_filename = MCValueRetain(p_external);
#endif
	
	if (m_externals == nil)
		m_externals = new (nothrow) MCExternalHandlerList;
	
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
    // We haven't created a new object as the result of this paste (yet...)
    r_objptr = NULL;

	if (MCactivefield)
	{
        // There is an active field so paste the clipboard into it.
        MCParagraph *t_paragraphs;
        t_paragraphs = MCclipboard->CopyAsParagraphs(MCactivefield);

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
	
	if (MCactiveimage && MCclipboard->HasImage())
	{
        // There is a selected image object and there is an image on the
        // clipboard so paste the image into it.
        MCAutoDataRef t_data;
		if (MCclipboard->CopyAsImage(&t_data))
        {
            MCExecContext ctxt(nil, nil, nil);

			MCImage *t_image;
			t_image = new (nothrow) MCImage;
			t_image -> open();
			t_image -> openimage();
			t_image -> SetText(ctxt, *t_data);
			MCactiveimage -> pasteimage(t_image);
			t_image -> closeimage();
			t_image -> close();

			delete t_image;
			return true; 
		}

		return false;
	}
	
	if (MCdefaultstackptr && (p_explicit || MCdefaultstackptr -> gettool(MCdefaultstackptr) == T_POINTER))
	{
		MCObject *t_objects;
		t_objects = NULL;

        // Attempt to lock the clipboard so we have a consistent view while we
        // check for various pasteable data formats.
		if (!MCclipboard->Lock())
			return false;
        
        // Does the clipboard contain LiveCode objects?
		if (MCclipboard->HasLiveCodeObjects())
		{
			MCAutoDataRef t_data;
			if (MCclipboard->CopyAsLiveCodeObjects(&t_data))
				t_objects = MCObject::unpickle(*t_data, MCdefaultstackptr);
		}
        // What about image data (limited to the formats LiveCode can understand
        // natively)?
		else if (MCclipboard->HasImage())
		{
			MCAutoDataRef t_data;
			if (MCclipboard->CopyAsImage(&t_data))
            {
                MCExecContext ctxt(nil, nil, nil);
				
				t_objects = new (nothrow) MCImage(*MCtemplateimage);
				t_objects -> open();
				static_cast<MCImage *>(t_objects) -> SetText(ctxt, *t_data);
				t_objects -> close();
			}
		}
        
        // Let the clipboard update automatically again.
        MCclipboard->Unlock();

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
	if (!m_drag_end_sent && MCdragsource && (!MCdragdest || MCdragaction == DRAG_ACTION_NONE))
	{
		// We are only the source
		m_drag_end_sent = true;
        
#ifdef WIDGETS_HANDLE_DND
        if (MCdragsource->gettype() == CT_WIDGET)
            MCwidgeteventmanager->event_dnd_end(reinterpret_cast<MCWidget*>(MCdragsource));
        else
#endif
            MCdragsource -> message(MCM_drag_end);

		// OK-2008-10-21 : [[Bug 7316]] - Cursor in script editor follows mouse after dragging to non-LiveCode target.
		// I have no idea why this apparently only happens in the script editor, but this seems to fix it and doesn't seem too risky :)
		// MW-2008-10-28: [[ Bug 7316 ]] - This happens because the script editor is doing stuff with drag messages
		//   causing the default engine behaviour to be overriden. In this case, some things have to happen to the field
		//   when the drag is over. Note that we have to check that the source was a field in this case since we don't
		//   need to do anything if it is not!
		// IM-2014-02-28: [[ Bug 11715 ]] dragsource may have changed or unset after sending message so check for valid ptr
		if (MCdragsource && MCdragsource -> gettype() == CT_FIELD)
		{
			MCField *t_field = MCdragsource.GetAs<MCField>();
			t_field -> setstate(False, CS_DRAG_TEXT);
			t_field -> computedrag();
			t_field -> getstack() -> resetcursor(True);
		}

		return;
	}
	
	if (p_source)
		return;

	// Setup global variables for a field drop
	MCdropfield = nil;
	MCdropchar = 0;

	findex_t t_start_index, t_end_index;
	t_start_index = t_end_index = 0;
	if (MCdragdest && MCdragdest -> gettype() == CT_FIELD)
	{
		MCdropfield = MCdragdest.GetAs<MCField>();
		if (MCdragdest -> getstate(CS_DRAG_TEXT))
		{
			MCdropfield -> locmark(False, False, False, False, True, t_start_index, t_end_index);
			MCdropchar = t_start_index;
		}
	}

	// If source is a field and the engine handled the start of the drag operation
	bool t_auto_source;
	t_auto_source = MCdragsource && MCdragsource -> gettype() == CT_FIELD && MCdragsource -> getstate(CS_SOURCE_TEXT);

	// If dest is a field and the engine handled the accepting of the operation
	bool t_auto_dest;
	t_auto_dest = MCdragdest && MCdragdest -> gettype() == CT_FIELD && MCdragdest -> getstate(CS_DRAG_TEXT);

    // Is the engine handling this drag internally AND the same field is both
    // the source and destination for the drag?
	if (t_auto_source && t_auto_dest && MCdragsource == MCdragdest)
	{
		// Source and target are the same field
		MCField *t_field = MCdragsource.GetAs<MCField>();

		findex_t t_from_start_index, t_from_end_index;
		t_field -> selectedmark(False, t_from_start_index, t_from_end_index, False);

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

        // Give the script the opportunity to intercept the drop
		if (t_field -> message(MCM_drag_drop) != ES_NORMAL)
		{
			MCParagraph *t_paragraphs;
            t_paragraphs = MCdragboard->CopyAsParagraphs(MCdropfield);

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

	findex_t t_src_start, t_src_end;
	t_src_start = t_src_end = 0;
	if (t_auto_source)
		MCdragsource.GetAs<MCField>()->selectedmark(False, t_src_start, t_src_end, False);

	bool t_auto_drop = MCdragdest.IsValid();
    if (t_auto_drop)
    {
#ifdef WIDGETS_HANDLE_DND
        if (MCdragdest->gettype() == CT_WIDGET)
        {
            MCwidgeteventmanager->event_dnd_drop(reinterpret_cast<MCWidget*>(MCdragdest));
            t_auto_drop = false;
        }
        else
#endif
        {
            t_auto_drop = MCdragdest -> message(MCM_drag_drop) != ES_NORMAL;
        }
    }

    // Is the engine handling this drag-and-drop operation internally AND both
    // the source and target are fields?
    //
    // There is a case above for if they are the same field so getting here
    // implies that the source and destination are different fields.
	if (t_auto_dest && t_auto_drop && MCdragboard != NULL && MCdropfield)
	{
		// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
		//   textChanged message by a lock screen pair.
		MCRedrawLockScreen();

		// Process an automatic drop action
		MCdropfield -> seltext(t_start_index, t_start_index, True);

        // Convert the clipboard contents to paragraphs
		MCParagraph *t_paragraphs;
        t_paragraphs = MCdragboard->CopyAsParagraphs(MCdropfield);
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
	else if (MCdropfield)
	{
		MCdropfield->setstate(False, CS_DRAG_TEXT);
		MCdropfield->computedrag();
		MCdropfield -> getstack() -> resetcursor(True);
	}

	bool t_auto_end;
	if (MCdragsource)
	{
		m_drag_end_sent = true;
#ifdef WIDGETS_HANDLE_DND
        if (MCdragsource->gettype() == CT_WIDGET)
        {
            MCwidgeteventmanager->event_dnd_end(reinterpret_cast<MCWidget*>(MCdragsource));
            t_auto_end = false;
        }
        else
#endif
        {
            t_auto_end = MCdragsource -> message(MCM_drag_end) != ES_NORMAL;
        }
	}
	else
		t_auto_end = false;

	if (t_auto_source && t_auto_end && MCdragsource && MCdragaction == DRAG_ACTION_MOVE)
	{
		// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
		//   textChanged message by a lock screen pair.
		MCRedrawLockScreen();
		MCdragsource.GetAs<MCField>()->deletetext(t_src_start, t_src_end);
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
		if (MCdefaultcursor == MCcursors[i])
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

bool MCDispatch::GetColor(MCExecContext& ctxt, Properties which, bool effective, MCInterfaceNamedColor& r_color)
{
    // SN-2014-12-05: [[ Bug 14154 ]] Added forgotten properties
    if (which == P_FORE_COLOR
            || which ==  P_BORDER_COLOR
            || which == P_TOP_COLOR
            || which == P_BOTTOM_COLOR
            || which == P_SHADOW_COLOR
            || which == P_FOCUS_COLOR)
        GetDefaultForeColor(ctxt, r_color);
    else if (which == P_BACK_COLOR
             || which == P_HILITE_COLOR)
        GetDefaultBackColor(ctxt, r_color);
    else
        r_color . name = MCValueRetain(kMCEmptyString);
    
    return true;
}

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
    r_pixel = MCColorGetPixel(MCscreen->black_pixel) & 0xFFFFFF;
}

void MCDispatch::GetDefaultBackPixel(MCExecContext& ctxt, uinteger_t& r_pixel)
{
    r_pixel = MCColorGetPixel(MCscreen->background_pixel) & 0xFFFFFF;
}

void MCDispatch::GetDefaultTopPixel(MCExecContext& ctxt, uinteger_t& r_pixel)
{
    r_pixel = MCColorGetPixel(MCscreen->white_pixel) & 0xFFFFFF;
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

// AL-2015-02-10: [[ Standalone Inclusions ]] Add functions to fetch relative paths present
//  in the resource mapping array of MCdispatcher.
void MCDispatch::addlibrarymapping(MCStringRef p_mapping)
{
    MCAutoStringRef t_name, t_target;
    MCNewAutoNameRef t_name_as_nameRef;

    if (!MCStringDivideAtChar(p_mapping, ':', kMCStringOptionCompareExact, &t_name, &t_target)
            || !MCNameCreate(*t_name, &t_name_as_nameRef))
        return;

    MCArrayStoreValue(m_library_mapping, false, *t_name_as_nameRef, *t_target);
}

// SN-2015-04-07: [[ Bug 15164 ]] Change p_name to be a StringRef.
bool MCDispatch::fetchlibrarymapping(MCStringRef p_name, MCStringRef& r_path)
{
    MCNewAutoNameRef t_name;
    MCStringRef t_value;

    if (!MCNameCreate(p_name, &t_name))
        return false;

    // m_library_mapping only stores strings (function above)
    if (!MCArrayFetchValue(m_library_mapping, false, *t_name, (MCValueRef&)t_value))
        return false;

    if (MCStringIsEmpty(t_value))
        return false;

    r_path = MCValueRetain(t_value);
    return true;
}

bool MCDispatch::haslibrarymapping(MCStringRef p_name)
{
    MCAutoStringRef t_mapping;
    return fetchlibrarymapping(p_name, &t_mapping);
}

bool MCDispatch::recomputefonts(MCFontRef, bool p_force)
{
    // Call the general recompute function first
    MCObject::recomputefonts(NULL, p_force);
    
    // Recompute the fonts for all stacks
    MCStack* t_stack = stacks;
    do
    {
        t_stack->recomputefonts(m_font, p_force);
        t_stack = t_stack->next();
    }
    while (t_stack != stacks);
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCDispatch::resolveparentscripts()
{
    if (stacks != NULL)
    {
        MCStack* t_stack = stacks;
        do
        {
            if (t_stack -> getextendedstate(ECS_USES_PARENTSCRIPTS))
                t_stack -> resolveparentscripts();
            
            t_stack = t_stack->next();
        }
        while(t_stack != stacks);
    }
}

