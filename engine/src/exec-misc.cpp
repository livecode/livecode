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

#include "globals.h"
#include "debug.h"
#include "handler.h"
#include "eventqueue.h"
#include "globals.h"
#include "mbldc.h"
#include "chunk.h"
#include "scriptpt.h"

#include "image.h"

#include "mblsyntax.h"
#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_GET_METHOD(Misc, DeviceToken, 0)
MC_EXEC_DEFINE_GET_METHOD(Misc, LaunchUrl, 0)

MC_EXEC_DEFINE_EXEC_METHOD(Misc, Beep, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Misc, Vibrate, 1)

MC_EXEC_DEFINE_GET_METHOD(Misc, DeviceResolution, 1)
MC_EXEC_DEFINE_SET_METHOD(Misc, UseDeviceResolution, 1)
MC_EXEC_DEFINE_GET_METHOD(Misc, DeviceScale, 1)
MC_EXEC_DEFINE_GET_METHOD(Misc, PixelDensity, 1)

MC_EXEC_DEFINE_EXEC_METHOD(Misc, ShowStatusBar, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Misc, HideStatusBar, 0)
MC_EXEC_DEFINE_SET_METHOD(Misc, StatusBarStyle, 1)

MC_EXEC_DEFINE_SET_METHOD(Misc, KeyboardType, 1)
MC_EXEC_DEFINE_SET_METHOD(Misc, KeyboardReturnKey, 1);

MC_EXEC_DEFINE_GET_METHOD(Misc, PreferredLanguages, 1)
MC_EXEC_DEFINE_GET_METHOD(Misc, CurrentLocale, 1)

MC_EXEC_DEFINE_GET_METHOD(Misc, SystemIdentifier, 1)
MC_EXEC_DEFINE_GET_METHOD(Misc, ApplicationIdentifier, 1)

MC_EXEC_DEFINE_EXEC_METHOD(Misc, ClearTouches, 0)

MC_EXEC_DEFINE_GET_METHOD(Misc, ReachabilityTarget, 1)
MC_EXEC_DEFINE_SET_METHOD(Misc, ReachabilityTarget, 1)

MC_EXEC_DEFINE_EXEC_METHOD(Misc, ExportImageToAlbum, 2)

MC_EXEC_DEFINE_SET_METHOD(Misc, RedrawInterval, 1)
MC_EXEC_DEFINE_SET_METHOD(Misc, AnimateAutorotation, 1)

MC_EXEC_DEFINE_GET_METHOD(Misc, DoNotBackupFile, 2)
MC_EXEC_DEFINE_SET_METHOD(Misc, DoNotBackupFile, 2)
MC_EXEC_DEFINE_GET_METHOD(Misc, FileDateProtection, 2)
MC_EXEC_DEFINE_SET_METHOD(Misc, FileDateProtection, 2)

MC_EXEC_DEFINE_EXEC_METHOD(Misc, LibUrlDownloadToFile, 2)

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCMiscStatusBarStyleElementInfo[] =
{
    { "default", kMCMiscStatusBarStyleDefault},
    { "translucent", kMCMiscStatusBarStyleTranslucent},
    { "opaque", kMCMiscStatusBarStyleOpaque}
};

static MCExecEnumTypeInfo _kMCMiscStatusBarStyleTypeInfo =
{
    "Misc.StatusBarStyle",
    sizeof(_kMCMiscStatusBarStyleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCMiscStatusBarStyleElementInfo
};

MCExecEnumTypeInfo* kMCMiscStatusBarStyleTypeInfo = &_kMCMiscStatusBarStyleTypeInfo;

static MCExecEnumTypeElementInfo _kMCMiscKeyboardTypeElementInfo[] =
{
    { "default", kMCMiscKeyboardTypeDefault},
    { "alphabet", kMCMiscKeyboardTypeAlphabet},
    { "numeric", kMCMiscKeyboardTypeNumeric},
    { "decimal", kMCMiscKeyboardTypeDecimal},
    { "number", kMCMiscKeyboardTypeNumber},
    { "phone", kMCMiscKeyboardTypePhone},
    { "email", kMCMiscKeyboardTypeEmail},
    { "url", kMCMiscKeyboardTypeUrl},
    { "contact", kMCMiscKeyboardTypeContact}
};

static MCExecEnumTypeInfo _kMCMiscKeyboardTypeTypeInfo =
{
    "Misc.KeyboardType",
    sizeof(_kMCMiscKeyboardTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCMiscKeyboardTypeElementInfo
};

MCExecEnumTypeInfo* kMCMiscKeyboardTypeTypeInfo = &_kMCMiscKeyboardTypeTypeInfo;

static MCExecEnumTypeElementInfo _kMCMiscKeyboardReturnKeyElementInfo[] =
{
    { "default", kMCMiscKeyboardReturnKeyDefault},
    { "go", kMCMiscKeyboardReturnKeyGo},
    { "google", kMCMiscKeyboardReturnKeyGoogle},
    { "join", kMCMiscKeyboardReturnKeyJoin},
    { "next", kMCMiscKeyboardReturnKeyNext},
    { "route", kMCMiscKeyboardReturnKeyRoute},
    { "search", kMCMiscKeyboardReturnKeySearch},
    { "send", kMCMiscKeyboardReturnKeySend},
    { "yahoo", kMCMiscKeyboardReturnKeyYahoo},
    { "done", kMCMiscKeyboardReturnKeyDone},
    { "emergency call", kMCMiscKeyboardReturnKeyEmergencyCall}
};

static MCExecEnumTypeInfo _kMCMiscKeyboardReturnKeyTypeInfo =
{
    "Misc.KeyboardReturnKey",
    sizeof(_kMCMiscKeyboardReturnKeyElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCMiscKeyboardReturnKeyElementInfo
};

MCExecEnumTypeInfo* kMCMiscKeyboardReturnKeyTypeInfo = &_kMCMiscKeyboardReturnKeyTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCMiscGetDeviceToken(MCExecContext& ctxt, MCStringRef& r_token)
{
    if(MCSystemGetDeviceToken(r_token))
        return;
    
    ctxt.Throw();
}

void MCMiscGetLaunchUrl(MCExecContext& ctxt, MCStringRef& r_url)
{
    if(MCSystemGetLaunchUrl(r_url))
        return;
    
    ctxt.Throw();
}

void MCMiscExecBeep(MCExecContext& ctxt, int32_t* p_number_of_times)
{
    int32_t t_number_of_times = 1;
    
    if (p_number_of_times != nil)
        t_number_of_times = *p_number_of_times;
    
    if (MCSystemBeep(t_number_of_times))
        return;
    
    ctxt.Throw();
}

void MCMiscExecVibrate(MCExecContext& ctxt, int32_t* p_number_of_times)
{
    int32_t t_number_of_times = 1;
    
    if (p_number_of_times != nil)
        t_number_of_times = *p_number_of_times;
    
    if (MCSystemVibrate(t_number_of_times))
        return;
    
    ctxt.Throw();
}

void MCMiscGetDeviceResolution(MCExecContext& ctxt, MCStringRef& r_resolution)
{
    if(MCSystemGetDeviceResolution(r_resolution))
        return;
    
    ctxt.Throw();
}

void MCMiscSetUseDeviceResolution(MCExecContext& ctxt, bool p_use_device_res, bool p_use_control_device_res)
{
    if (MCSystemSetDeviceUseResolution(p_use_control_device_res, p_use_control_device_res))
        return;
    
    ctxt.Throw();
}

void MCMiscGetDeviceScale(MCExecContext& ctxt, real64_t& r_scale)
{
    if(MCSystemGetDeviceScale(r_scale))
        return;
    
    ctxt.Throw();
}


void MCMiscGetPixelDensity(MCExecContext& ctxt, real64_t& r_density)
{
    if (MCSystemGetPixelDensity(r_density))
        return;

    ctxt.Throw();
}

void MCMiscSetStatusBarStyle(MCExecContext& ctxt, intenum_t p_style)
{
    if (MCSystemSetStatusBarStyle(p_style))
        return;
    
    ctxt.Throw();
}

void MCMiscExecShowStatusBar(MCExecContext& ctxt)
{
    if (MCSystemShowStatusBar())
        return;
    
    ctxt.Throw();
}

void MCMiscExecHideStatusBar(MCExecContext& ctxt)
{
    if (MCSystemHideStatusBar())
        return;
    
    ctxt.Throw();
}

void MCMiscSetKeyboardType(MCExecContext& ctxt, intenum_t p_keyboard_type)
{
    if (MCSystemSetKeyboardType(p_keyboard_type))
        return;
    
    ctxt.Throw();
}

void MCMiscSetKeyboardReturnKey(MCExecContext& ctxt, intenum_t p_keyboard_return_key)
{
    if (MCSystemSetKeyboardReturnKey(p_keyboard_return_key))
        return;
    
    ctxt.Throw();
}

void MCMiscGetPreferredLanguages(MCExecContext& ctxt, MCStringRef& r_languages)
{
    if (MCSystemGetPreferredLanguages(r_languages))
        return;
    
    ctxt.Throw();
}

void MCMiscGetCurrentLocale(MCExecContext& ctxt, MCStringRef& r_current_locale)
{
    if (MCSystemGetCurrentLocale(r_current_locale))
        return;
    
    ctxt.Throw();
}

void MCMiscExecClearTouches(MCExecContext& ctxt)
{
    MCscreen -> wait(1/25.0, False, False);
    static_cast<MCScreenDC *>(MCscreen) -> clear_touches();
    MCEventQueueClearTouches();
}

void MCMiscGetSystemIdentifier(MCExecContext& ctxt, MCStringRef& r_identifier)
{
    if (MCSystemGetSystemIdentifier(r_identifier))
        return;
        
    ctxt.Throw();
}

void MCMiscGetApplicationIdentifier(MCExecContext& ctxt, MCStringRef& r_identifier)
{
    if (MCSystemGetApplicationIdentifier(r_identifier))
        return;
    
    ctxt.Throw();
}

void MCMiscSetReachabilityTarget(MCExecContext& ctxt, MCStringRef p_hostname)
{
    if (MCSystemSetReachabilityTarget(p_hostname))
        return;
    
    ctxt.Throw();
}


void MCMiscGetReachabilityTarget(MCExecContext& ctxt, MCStringRef& r_hostname)
{
    if (MCSystemGetReachabilityTarget(r_hostname))
        return;
    
    ctxt.Throw();
}

void MCMiscExecLibUrlDownloadToFile(MCExecContext& ctxt, MCStringRef p_url, MCStringRef p_filename)
{
    extern void MCS_downloadurl(MCObject *p_target, const char *p_url, const char *p_file);

    MCS_downloadurl(MCtargetptr, MCStringGetCString(p_url), MCStringGetCString(p_filename));
}

//////////////////////////////////////////////////////////////////////

static bool is_png_data(MCStringRef p_data)
{
    MCAutoStringRef t_format;
    MCStringCreateWithCString("\211PNG", &t_format);
	return MCStringGetLength(p_data) > 4 && MCStringSubstringContains(p_data, MCRangeMake(0, 4), *t_format, kMCStringOptionCompareExact);
}

static bool is_gif_data(MCStringRef p_data)
{
    MCAutoStringRef t_format;
    MCStringCreateWithCString("GIF8", &t_format);
	return MCStringGetLength(p_data) > 4 && MCStringSubstringContains(p_data, MCRangeMake(0, 4), *t_format, kMCStringOptionCompareExact);
}

static bool is_jpeg_data(MCStringRef p_data)
{
    MCAutoStringRef t_format;
    MCStringCreateWithCString("\xff\xd8", &t_format);
	return MCStringGetLength(p_data) > 2 && MCStringSubstringContains(p_data, MCRangeMake(0, 2), *t_format, kMCStringOptionCompareExact);
}

void MCMiscExecExportImageToAlbum(MCExecContext& ctxt, MCStringRef p_raw_data, MCStringRef p_file_name)
{
    bool t_file_extension_ok = false;
    MCAutoStringRef t_file_extension;
    
	if (is_png_data(p_raw_data))
    {
        t_file_extension_ok = MCStringCreateWithCString(".png\n", &t_file_extension);
    }
    else if (is_gif_data(p_raw_data))
    {
        t_file_extension_ok = MCStringCreateWithCString(".gif\n", &t_file_extension);
    }
    else if (is_jpeg_data(p_raw_data))
    {
        t_file_extension_ok = MCStringCreateWithCString(".jpg\n", &t_file_extension);
    }
    
    if (!t_file_extension_ok)
    {
        MCLog("Type not found", nil);
		uint4 parid;
		MCObject *objptr;
		MCChunk *tchunk = new MCChunk(False);
		MCerrorlock++;
		MCScriptPoint sp(ctxt.GetEP());
		Parse_stat stat = tchunk->parse(sp, False);
		if (stat != PS_NORMAL || tchunk->getobj(ctxt.GetEP(), objptr, parid, True) != ES_NORMAL)
		{
            MCLog("could not find image", nil);
			ctxt.SetTheResultToStaticCString("could not find image");
			MCerrorlock--;
			delete tchunk;
            ctxt.Throw();
		}
		
		if (objptr -> gettype() != CT_IMAGE)
		{
            MCLog("not an image", nil);
			ctxt.SetTheResultToStaticCString("not an image");
            ctxt.Throw();
		}
		
		MCImage *t_image;
		t_image = static_cast<MCImage *>(objptr);
		if (t_image -> getcompression() == F_PNG)
        {
            t_file_extension_ok = MCStringCreateWithCString(".png\n", &t_file_extension);
        }
        else if (t_image -> getcompression() == F_JPEG)
        {
            t_file_extension_ok = MCStringCreateWithCString(".gif\n", &t_file_extension);
        }
        else if (t_image -> getcompression() == F_GIF)
		{
            t_file_extension_ok = MCStringCreateWithCString(".jpg\n", &t_file_extension);
        }
        else
        {
            MCLog("not a supported image", nil);
            ctxt.SetTheResultToStaticCString("not a supported format");
			ctxt.Throw();
		}
        MCLog("MCHandleExportImageToAlbum() converting to raw data", nil);
    }
    
    // See if the user provided us with a file name
    if (t_file_extension_ok)
    {
        MCAutoStringRef t_save_result;
        MCSystemExportImageToAlbum(&t_save_result, p_raw_data, p_file_name, *t_file_extension);
        
        ctxt.SetTheResultToValue(*t_save_result);
    }
    else
    {
        ctxt.SetTheResultToStaticCString("export failed");
        ctxt.Throw();
    }
}

void MCMiscSetRedrawInterval(MCExecContext& ctxt, int32_t p_interval)
{
    if (MCSystemSetRedrawInterval(p_interval))
        return;
    
    ctxt.Throw();
}

void MCMiscSetAnimateAutorotation(MCExecContext& ctxt, bool p_enabled)
{
    if (MCSystemSetAnimateAutorotation(p_enabled))
        return;
    
    ctxt.Throw();
}

void MCMiscGetDoNotBackupFile(MCExecContext& ctxt, MCStringRef p_path, bool& r_no_backup)
{
    if (MCSystemFileGetDoNotBackup(p_path, r_no_backup))
        return;
    
    ctxt.Throw();
}

void MCMiscSetDoNotBackupFile(MCExecContext& ctxt, MCStringRef p_path, bool p_no_backup)
{
    if (MCSystemFileSetDoNotBackup(p_path, p_no_backup))
        return;
    
    ctxt.Throw();
}

void MCMiscGetFileDataProtection(MCExecContext& ctxt, MCStringRef p_path, MCStringRef& p_protection_string)
{
    if (MCSystemFileGetDataProtection(p_path, p_protection_string))
        return;
    
    ctxt.Throw();
}

void MCMiscSetFileDataProtection(MCExecContext& ctxt, MCStringRef p_path, MCStringRef p_protection_string)
{
    MCAutoStringRef t_status;
    if (MCSystemFileSetDataProtection(p_path, p_protection_string, &t_status))
        return;
    
    ctxt.SetTheResultToValue(* t_status);
    ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

