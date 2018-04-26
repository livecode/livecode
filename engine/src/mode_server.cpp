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
#include "property.h"

#include "globals.h"
#include "license.h"
#include "mode.h"
#include "revbuild.h"
#include "deploy.h"
#include "capsule.h"
#include "player.h"
#include "internal.h"

#include "srvscript.h"

////////////////////////////////////////////////////////////////////////////////
//
//  Globals specific to SERVER mode
//

// AL-2014-11-07: [[ Bug 13919 ]] Set the script limits to 0 for server community
//  as script only stacks are subjected to license parameter checks

MCLicenseParameters MClicenseparameters =
{
	NULL, NULL, NULL, kMCLicenseClassNone, 0,
	0, 0, 0, 0,
	0,
	NULL,
};

////////////////////////////////////////////////////////////////////////////////
//
//  Property tables specific to SERVER mode
//

MCPropertyInfo MCObject::kModeProperties[] =
{
	{ P_UNDEFINED, false, kMCPropertyTypeAny, nil, nil, nil, false, false, kMCPropertyInfoChunkTypeNone}
};

MCObjectPropertyTable MCObject::kModePropertyTable =
{
	nil,
	0,
	nil,
};

MCPropertyInfo MCStack::kModeProperties[] =
{
	{ P_UNDEFINED, false, kMCPropertyTypeAny, nil, nil, nil, false, false, kMCPropertyInfoChunkTypeNone}
};

MCObjectPropertyTable MCStack::kModePropertyTable =
{
	nil,
	0,
	nil,
};

MCPropertyInfo MCProperty::kModeProperties[] =
{
	{ P_UNDEFINED, false, kMCPropertyTypeAny, nil, nil, nil, false, false, kMCPropertyInfoChunkTypeNone}
};

MCPropertyTable MCProperty::kModePropertyTable =
{
	0,
	nil,
};

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of MCDispatch::startup method for SERVER mode.
//

IO_stat MCDispatch::startup(void)
{
	stacks = new (nothrow) MCServerScript;
	stacks -> setname_cstring("Home");
	stacks -> setparent(this);

	MCdefaultstackptr = MCstaticdefaultstackptr = stacks;

	stacks -> open();

	return IO_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of MCStack::mode* hooks for SERVER mode.
//

#ifdef _WINDOWS_SERVER
MCSysWindowHandle MCStack::getrealwindow(void)
{
	return nil;
}
#endif

void MCStack::mode_load(void)
{
}

void MCStack::mode_getrealrect(MCRectangle& r_rect)
{
	MCU_set_rect(r_rect, 0, 0, 0, 0);
}

void MCStack::mode_takewindow(MCStack *other)
{
}

void MCStack::mode_takefocus(void)
{
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

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of mode hooks for SERVER mode.
//

// In standalone mode, the standalone stack built into the engine cannot
// be saved.
IO_stat MCModeCheckSaveStack(MCStack *sptr, const MCStringRef filename)
{
	if (sptr == MCdispatcher -> getstacks())
	{
		MCresult->sets("can't save into standalone");
		return IO_ERROR;
	}

	return IO_NORMAL;
}

// In standalone mode, the environment depends on various command-line/runtime
// globals.
MCNameRef MCModeGetEnvironment(void)
{
	return MCN_server;
}

uint32_t MCModeGetEnvironmentType(void)
{
	return kMCModeEnvironmentTypeServer;
}

// SN-2015-01-16: [[ Bug 14295 ]] Not implemented for server
void MCModeGetResourcesFolder(MCStringRef &r_resources_folder)
{
    // Not implemented on server
    r_resources_folder = MCValueRetain(kMCEmptyString);
}

// In standalone mode, we are never licensed.
bool MCModeGetLicensed(void)
{
	return false;
}

// In standalone mode, the executable is $0 if there is an embedded stack.
bool MCModeIsExecutableFirstArgument(void)
{
	return true;
}

// In server mode, we have command line name / arguments
bool MCModeHasCommandLineArguments(void)
{
    return true;
}

// In server mode, we have environment variables
bool
MCModeHasEnvironmentVariables()
{
	return true;
}

// In standalone mode, we only automatically open stacks if there isn't an
// embedded stack.
bool MCModeShouldLoadStacksOnStartup(void)
{
	return false;
}

// In standalone mode, we try to work out what went wrong...
void MCModeGetStartupErrorMessage(MCStringRef& r_caption, MCStringRef& r_text)
{
	r_caption = MCSTR("Initialization Error");
	if (MCValueGetTypeCode(MCresult -> getvalueref()) == kMCValueTypeCodeString)
		r_text = MCValueRetain((MCStringRef)MCresult -> getvalueref());
	else
		r_text = MCSTR("unknown reason");
}

// In standalone mode, we can only set an object's script if has non-zero id.
bool MCModeCanSetObjectScript(uint4 obj_id)
{
	return obj_id != 0;
}

// In standalone mode, we must check the old CANT_STANDALONE flag.
bool MCModeShouldCheckCantStandalone(void)
{
	return true;
}

// The standalone mode causes a relaunch message.
bool MCModeHandleRelaunch(MCStringRef & r_id)
{
	return false;
}

// The standalone mode's startup stack depends on whether it has a stack embedded.
const char *MCModeGetStartupStack(void)
{
	return NULL;
}

bool MCModeCanLoadHome(void)
{
	return false;
}

MCStatement *MCModeNewCommand(int2 which)
{
	return NULL;
}

MCExpression *MCModeNewFunction(int2 which)
{
	return NULL;
}

bool MCModeShouldQueueOpeningStacks(void)
{
	return false;
}

bool MCModeShouldPreprocessOpeningStacks(void)
{
	return false;
}

Window MCModeGetParentWindow(void)
{
	return NULL;
}

bool MCModeCanAccessDomain(MCStringRef p_name)
{
	return false;
}

void MCModeQueueEvents(void)
{
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
}

void MCModeConfigureIme(MCStack *p_stack, bool p_enabled, int32_t x, int32_t y)
{
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

// The server engine has its home stack sit in the message path for all stacks so
// we return true here.
bool MCModeHasHomeStack(void)
{
	return true;
}

// Pixel scaling is disabled on the server.
bool MCModeCanEnablePixelScaling()
{
	return false;
}

// IM-2014-08-08: [[ Bug 12372 ]] Pixel scaling is disabled on the server.
bool MCModeGetPixelScalingEnabled(void)
{
	return false;
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
// Implementation of Linux-specific mode hooks for SERVER mode.
//

#ifdef _LINUX_SERVER
void MCModePreSelectHook(int& maxfd, fd_set& rfds, fd_set& wfds, fd_set& efds)
{
}

void MCModePostSelectHook(fd_set& rfds, fd_set& wfds, fd_set& efds)
{
}

#endif

////////////////////////////////////////////////////////////////////////////////
//
// Implementation of internal verbs
//

// The internal verb table used by the '_internal' command
MCInternalVerbInfo MCinternalverbs[] =
{
	{ nil, nil, nil }
};
