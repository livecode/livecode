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

#ifndef __MC_MODE__
#define __MC_MODE__

class MCStatement;
class MCExpression;

enum
{
	kMCModeEnvironmentTypeUnknown,
	kMCModeEnvironmentTypeEditor,
	kMCModeEnvironmentTypeDesktop,
	kMCModeEnvironmentTypeHelper,
	kMCModeEnvironmentTypeInstaller,
	kMCModeEnvironmentTypeShell,
	kMCModeEnvironmentTypeBrowser,
	kMCModeEnvironmentTypePlayer,
	kMCModeEnvironmentTypeServer,
	kMCModeEnvironmentTypeMobile,
    kMCModeEnvironmentTypeEmbedded,
};


// This hook is called on startup (on Windows) before anything else.
//
void MCModePreMain(void);

// This hook is used to check if the current mode can save the given stack
// to the given file.
//
// The hook is called by MCDispatch::savestack and should return IO_NORMAL
// or IO_ERROR depending on whether the check succeeds or not.
//
IO_stat MCModeCheckSaveStack(MCStack *stack, const MCStringRef p_filename);

// This hook is used to work out the appropriate 'environment' string based
// on the mode and various globals.
//
// The hook is called by MCEnvironment::eval and should return the
// appropriate string constant.
//
MCNameRef MCModeGetEnvironment(void);
uint32_t MCModeGetEnvironmentType(void);


// SN-2015-01-16: [[ Bug 14295 ]] Added mode-specific way to get the resources folder
// This hook is used to work out what is the resource folder on Mac.
// On a standalone built, it is <app>.app/Contents/Resources/_MacOS
// On the other modes, it is 'the filename of this stack'
//
void MCModeGetResourcesFolder(MCStringRef &r_resources_folder);

// This hook is used to work out whether the engine has been licensed.
//
// The hook is called by MCLicensed::eval.
//
bool MCModeGetLicensed(void);

// This hook is used to determine whether the executable should be $0
// or not.
//
// This hook is called by X_init.
//
bool MCModeIsExecutableFirstArgument(void);

// This hook is used to determine if we populate the command line name and
// arguments at startup.
//
// This hook is called by X_open.
//
bool MCModeHasCommandLineArguments(void);

// This hook is used to determine if we populate the environment
// variables at startup.
//
// This hook is called by X_open.
//
bool MCModeHasEnvironmentVariables(void);

// This hook is used to determine if any stacks on the command-line
// should be opened on startup.
//
// This hook is called by X_init.
//
bool MCModeShouldLoadStacksOnStartup(void);

// This hook is used to generate the alert message on Windows, should
// initialization fail.
//
// This hook is called by WinMain.
//
void MCModeGetStartupErrorMessage(MCStringRef& r_caption, MCStringRef& r_text);

// This hook is used to determine if a given object can have its
// script set.
//
// This hook is called by MCObject::setprop (P_SCRIPT)
//
bool MCModeCanSetObjectScript(uint4 obj_id);

// This hook is used to determine if the old CANT_STANDALONE flag
// should be interepreted as if we are in a standalone.
//
// This hook is called by MCObject::load
//
bool MCModeShouldCheckCantStandalone(void);

// This hook is used to compute the origin to be added
// to each object.
//
// This hook is called by MCObject::save
//
uint4 MCModeComputeObjectOrigin(uint4 extraflags);

// This hook is used to work out the parameters for the 'relaunch'
// feature.
//
// This hook is called by send_relaunch (dispatch.cpp)
//
bool MCModeHandleRelaunch(MCStringRef & r_id);

// This hook is used to work out what stack to startup with.
//
// This hook is called by X_init.
//
const char *MCModeGetStartupStack(void);

// This hook is used to determine if a 'HOME' stack can be loaded.
//
// This hook is called by MCDispatch::readfile.
//
bool MCModeCanLoadHome(void);

// This hook is used to initialize crash reporting settings.
//
// This hook is called by WinMain.
//
void MCModeSetupCrashReporting(void);

// These hooks are used to create mode-specific syntax objects.
//
// They are called by the MCN_new_* methods.
//
MCStatement *MCModeNewCommand(int2 which);
MCExpression *MCModeNewFunction(int2 which);

// This hook is used to determine whether to queue stacks that are wanting to
// be opened (e.g. via an AppleEvent OpenDoc event).
//
// It is called by DoOpenDoc (osxspec.cpp).
//
bool MCModeShouldQueueOpeningStacks(void);

// This hook is used to determine whether to process pending stack open
// messages (i.e. OpenDoc) during init.
//
// It is called by MCS_init (osxspec)
//
bool MCModeShouldPreprocessOpeningStacks(void);

// This hook is used to determine what the 'parent' window should be when a
// dialog is opened.
//
Window MCModeGetParentWindow(void);

// This hook is used to determine whether a network resource can be accessed
// while security limitations are in effect
bool MCModeCanAccessDomain(MCStringRef p_name);

#if defined(_LINUX) || defined (_LINUX_SERVER)
void MCModePreSelectHook(int& maxfd, fd_set& rfds, fd_set& wfds, fd_set& efds);
void MCModePostSelectHook(fd_set& rfds, fd_set& wfds, fd_set& efds);
#endif

// This hook is called whenever the engine wants to process more events.
void MCModeQueueEvents(void);

// This hook is used to invoke JavaScript in the browser when running in plugin
// mode.
Exec_stat MCModeExecuteScriptInBrowser(MCStringRef p_script);

// This hook is used to activate (passive) IME.
void MCModeActivateIme(MCStack *stack, bool activate);
// This hook is used to configure an IME session.
void MCModeConfigureIme(MCStack *stack, bool enabled, int32_t x, int32_t y);

// This hook is used to determine whether windows should be created locally.
bool MCModeMakeLocalWindows(void);

// These hooks show and hide tooltips
void MCModeShowToolTip(int32_t x, int32_t y, uint32_t text_size, uint32_t bg_color, MCStringRef text_font, MCStringRef message);
void MCModeHideToolTip(void);

// This hook is used to handle the reset cursors action.
void MCModeResetCursors(void);

// This hook is used to determine if the ssl library is loaded on startup to start entropy collection
bool MCModeCollectEntropy(void);

// This hook is used to determine whether there is a notion of 'home' stack.
// The 'home' stack is one which sits at the root of all others and all
// messages pass through it from other stacks. Server and IDE engines have
// home stacks, standalones and installers do not.
bool MCModeHasHomeStack(void);

// Property getters & setters

#ifdef MODE_DEVELOPMENT
void MCModeGetRevCrashReportSettings(MCExecContext& ctxt, MCArrayRef& r_settings);
void MCModeSetRevCrashReportSettings(MCExecContext& ctxt, MCArrayRef p_settings);
void MCModeGetRevObjectListeners(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_listeners);
void MCModeGetRevPropertyListenerThrottleTime(MCExecContext& ctxt, uinteger_t& r_time);
void MCModeSetRevPropertyListenerThrottleTime(MCExecContext& ctxt, uinteger_t p_time);
#endif

// IM-2014-08-08: [[ Bug 12372 ]] Check if pixel scaling should be enabled.
bool MCModeGetPixelScalingEnabled(void);

// Check if this mode is allowed to change the pixel scaling settings.
bool MCModeCanEnablePixelScaling(void);

// Hook called in X_close for any mode-specific finalization.
void MCModeFinalize(void);

#endif
