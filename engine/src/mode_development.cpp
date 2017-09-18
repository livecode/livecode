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
void MCInternalObjectListenerGetListeners(MCExecContext& ctxt, MCStringRef*& r_listeners, uindex_t& r_count);
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  Globals specific to DEVELOPMENT mode
//

MCLicenseParameters MClicenseparameters =
{
	NULL, NULL, NULL, kMCLicenseClassNone, 0,
	0, 0, 0, 0,
	0,
	NULL,
};

Boolean MCcrashreportverbose = False;
MCStringRef MCcrashreportfilename = nil;

////////////////////////////////////////////////////////////////////////////////
//
//  Property tables specific to DEVELOPMENT mode
//

MCPropertyInfo MCObject::kModeProperties[] =
{
	{ P_UNDEFINED, false, kMCPropertyTypeAny, nil, nil, nil, false, false, kMCPropertyInfoChunkTypeNone}
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
    DEFINE_RO_OBJ_LIST_PROPERTY(P_UNPLACED_GROUP_IDS, LinesOfLooseUInt, MCStack, UnplacedGroupIds)
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

static void restart_livecode(void)
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
	MCtracestackptr = nil;
	MCtraceabort = True;
	MCtracereturn = True;
    
    MCAutoStringRef t_command_path;
    MCS_resolvepath(MCcmd, &t_command_path);
	
	s_command_path = MCValueRetain(*t_command_path);

	atexit(restart_livecode);
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
    
    if (MCnoui && MCnstacks > 0 && MClicenseparameters . license_class == kMCLicenseClassCommunity)
    {
        if (MCdispatcher -> loadfile(MCstacknames[0], sptr) != IO_NORMAL)
        {
            MCresult -> setvalueref(MCSTR("failed to read stackfile"));
            return IO_ERROR;
        }
        
        MCMemoryMove(MCstacknames, MCstacknames + 1, sizeof(MCStringRef) * (MCnstacks-1));
        MCnstacks -= 1;
    }
    else if (MCnoui)
    {
        MCresult -> setvalueref(MCSTR("cannot run in command line mode"));
        return IO_ERROR;
    }
    else
    {
        MCAutoDataRef t_decompressed;
        {
            MCAutoDataRef t_compressed;
            /* UNCHECKED */ MCDataCreateWithBytes((const char_t*)MCstartupstack,
                                                  MCstartupstack_length,
                                                  &t_compressed);
            /* UNCHECKED */ MCFiltersDecompress(*t_compressed, &t_decompressed);
        }
        
        IO_handle stream = MCS_fakeopen(MCDataGetBytePtr(*t_decompressed),
                                        MCDataGetLength(*t_decompressed));
        if ((stat = MCdispatcher -> readfile(NULL, NULL, stream, sptr)) != IO_NORMAL)
        {
            if (MCdispatcher -> loadfile(MCstacknames[0], sptr) != IO_NORMAL)
            {
                MCresult -> sets("failed to read stackfile");
                return IO_ERROR;
            }
            
            MCMemoryMove(MCstacknames, MCstacknames + 1, sizeof(MCStack *) * (MCnstacks - 1));
            MCnstacks -= 1;
        }

        MCS_close(stream);

        /* FRAGILE */ memset((void *)MCDataGetBytePtr(*t_decompressed), 0,
                             MCDataGetLength(*t_decompressed));

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
            MCtopstackptr = nil;
            MCquit = False;
            MCenvironmentactive = False;
            
            send_relaunch();
            MCNewAutoNameRef t_name;
            if(!ctxt . ConvertToName(t_valueref, &t_name))
            {
                ctxt . Throw();
                return IO_ERROR;
            }

            sptr = findstackname(*t_name);
            if (t_valueref != nil)
                MCValueRelease(t_valueref);
            if (t_valueref2 != nil)
                MCValueRelease(t_valueref2);

            if (sptr == NULL && (stat = loadfile(MCNameGetString(*t_name), sptr)) != IO_NORMAL)
                return stat;
        }
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

void MCStack::mode_load(void)
{
	// We introduce the notion of an 'IDE' stack - such a stack is set by giving it
	// a custom property 'ideOverride' with value true.
	if (props != NULL)
	{
        bool t_old_lock = MClockmessages;
		MClockmessages = true;
        MCExecValue t_value;
        MCExecContext ctxt(nil, nil, nil);
        getcustomprop(ctxt, kMCEmptyName, MCNAME("ideOverride"), nil, t_value);
		MClockmessages = t_old_lock;

		bool t_treat_as_ide;
        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value, kMCExecValueTypeBool, &t_treat_as_ide);
        if (!ctxt . HasError() && t_treat_as_ide)
			m_is_ide_stack = true;
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
//
//  Implementation of MCObject::getmodeprop for DEVELOPMENT mode.
//

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
	if (MCnoui)
        return MCN_development_cmdline;
    else
        return MCN_development;
}

uint32_t MCModeGetEnvironmentType(void)
{
	return kMCModeEnvironmentTypeEditor;
}

// SN-2015-01-16: [[ Bug 14295 ]] Development-mode is not standalone
void MCModeGetResourcesFolder(MCStringRef &r_resources_folder)
{
    MCS_getresourcesfolder(false, r_resources_folder);
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

// In development mode, we don't have command line arguments / name
bool MCModeHasCommandLineArguments(void)
{
    return false;
}

// In development mode, we process environment variables
bool
MCModeHasEnvironmentVariables()
{
	return true;
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
	if (t_window == NULL && MCtopstackptr)
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
        MCdispatcher -> gethome() -> message(MCM_main_stacks_changed);
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

void MCModeFinalize(void)
{
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

    HANDLE t_file = nullptr;
    if (t_write_minidump != nullptr)
    {
        MCAutoStringRef t_path;
        MCAutoStringRefAsWString t_path_w32;
        if (MCS_resolvepath(MCcrashreportfilename, &t_path) &&
            t_path_w32.Lock(*t_path))
        {
            t_file = CreateFileW(*t_path_w32, GENERIC_WRITE, 0, nullptr,
                                 CREATE_ALWAYS, 0, nullptr);
        }
    }

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

// Pixel scaling can be enabled in Development mode
bool MCModeCanEnablePixelScaling()
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
    r_value = m_is_ide_stack;
}

void MCStack::SetIdeOverride(MCExecContext& ctxt, bool p_value)
{
    m_is_ide_stack = p_value;
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
            
void MCModeSetRevPropertyListenerThrottleTime(MCExecContext& ctxt, uinteger_t p_time)
{
#ifdef FEATURE_PROPERTY_LISTENER
	// MM-2012-11-06: [[ Property Listener ]]
	MCpropertylistenerthrottletime = p_time;
#endif
}

void MCModeGetRevCrashReportSettings(MCExecContext& ctxt, MCArrayRef& r_settings)
{
    r_settings = MCValueRetain(kMCEmptyArray);
}

void MCModeGetRevObjectListeners(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_listeners)
{
#ifdef FEATURE_PROPERTY_LISTENER
    // MM-2012-09-05: [[ Property Listener ]]
    MCInternalObjectListenerGetListeners(ctxt, r_listeners, r_count);
#else
    r_count = 0;
#endif
}
void MCModeGetRevPropertyListenerThrottleTime(MCExecContext& ctxt, uinteger_t& r_time)
{
#ifdef FEATURE_PROPERTY_LISTENER
    r_time = MCpropertylistenerthrottletime;
#else
    r_time = 0;
#endif
}
