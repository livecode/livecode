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

#include "globals.h"
#include "debug.h"
#include "handler.h"
#include "eventqueue.h"
#include "globals.h"
#include "mbldc.h"
#include "chunk.h"
#include "scriptpt.h"
#include "osspec.h"


#include "image.h"

#include "mblsyntax.h"
#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCMiscStatusBarStyleElementInfo[] =
{
    { "default", kMCMiscStatusBarStyleDefault, false},
    { "translucent", kMCMiscStatusBarStyleTranslucent, false},
    { "opaque", kMCMiscStatusBarStyleOpaque, false},
    { "solid", kMCMiscStatusBarStyleSolid, false}
};

static MCExecEnumTypeInfo _kMCMiscStatusBarStyleTypeInfo =
{
    "Misc.StatusBarStyle",
    sizeof(_kMCMiscStatusBarStyleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCMiscStatusBarStyleElementInfo
};

MCExecEnumTypeInfo* kMCMiscStatusBarStyleTypeInfo = &_kMCMiscStatusBarStyleTypeInfo;

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

void MCMiscGetLaunchData(MCExecContext &ctxt, MCArrayRef &r_launch_data)
{
	if (MCSystemGetLaunchData(r_launch_data))
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

static intenum_t s_current_keyboard_display = 0;

void MCMiscExecSetKeyboardDisplay(MCExecContext& ctxt, intenum_t p_mode)
{
    if (MCSystemSetKeyboardDisplay(p_mode))
    {
        s_current_keyboard_display = p_mode;
        return;
    }
    
    ctxt.Throw();
}

void MCMiscExecGetKeyboardDisplay(MCExecContext& ctxt, intenum_t& r_mode)
{
    r_mode = s_current_keyboard_display;
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

    // PM-2015-03-16: [[ Bug 14333 ]] Make sure the object that triggered a mouse down msg is not focused, as this stops later mouse downs from working
    if (MCtargetptr)
        MCtargetptr -> munfocus();
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

void MCMiscGetIdentifierForVendor(MCExecContext& ctxt, MCStringRef& r_identifier)
{
    if (MCSystemGetIdentifierForVendor(r_identifier))
        return;
    
    ctxt . Throw();
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
    MCS_downloadurl(MCtargetptr, p_url, p_filename);
}

void MCMiscExecLibUrlSetSSLVerification(MCExecContext& ctxt, bool p_enabled)
{
    extern void MCS_seturlsslverification(bool enabled);
    MCS_seturlsslverification(p_enabled);
}

//////////////////////////////////////////////////////////////////////////////////////////

// SN-2014-12-11: [[ Merge-6.7.1-rc-4 ]]
void MCMiscGetIsVoiceOverRunning(MCExecContext& ctxt, bool& r_is_vo_running)
{
    if (MCSystemGetIsVoiceOverRunning(r_is_vo_running))
        return;

    ctxt . Throw();
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

void MCMiscExecExportImageToAlbum(MCExecContext& ctxt, MCStringRef p_data_or_id, MCStringRef p_file_name)
{
    bool t_is_raw_data = false;
    bool t_extension_found = false;
    MCAutoStringRef t_file_extension;
    
    // SN-2014-12-18: [[ Bug 13860 ]] Update the way images data are created.
    MCAutoDataRef t_raw_data, t_converted_data;
    MCAutoStringRef t_save_result;
    
    if (ctxt . ConvertToData(p_data_or_id, &t_converted_data))
    {
        if (MCImageDataIsPNG(*t_converted_data))
        {
            t_extension_found = MCStringCreateWithCString(".png\n", &t_file_extension);
        }
        else if (MCImageDataIsGIF(*t_converted_data))
        {
            t_extension_found = MCStringCreateWithCString(".gif\n", &t_file_extension);
        }
        else if (MCImageDataIsJPEG(*t_converted_data))
        {
            t_extension_found = MCStringCreateWithCString(".jpg\n", &t_file_extension);
        }
    }
    
    if (!t_extension_found)
    {
        MCLog("Type not found");
		uint4 parid;
		MCObject *objptr;
		MCChunk *tchunk = new (nothrow) MCChunk(False);
        MCerrorlock++;
		MCScriptPoint sp(p_data_or_id);
		Parse_stat stat = tchunk->parse(sp, False);
        if (stat != PS_NORMAL || !tchunk->getobj(ctxt, objptr, parid, True))
		{
            MCLog("could not find image");
			ctxt.SetTheResultToStaticCString("could not find image");
			MCerrorlock--;
			delete tchunk;
            return;
		}
		
		if (objptr -> gettype() != CT_IMAGE)
		{
            MCLog("not an image");
			ctxt.SetTheResultToStaticCString("not an image");
            return;
		}
		
		MCImage *t_image;
		t_image = static_cast<MCImage *>(objptr);
		if (t_image -> getcompression() == F_PNG)
        {
            /* UNCHECKED */ MCStringCreateWithCString(".png\n", &t_file_extension);
        }
        else if (t_image -> getcompression() == F_JPEG)
        {
            /* UNCHECKED */ MCStringCreateWithCString(".jpg\n", &t_file_extension);
        }
        else if (t_image -> getcompression() == F_GIF)
		{
            /* UNCHECKED */ MCStringCreateWithCString(".gif\n", &t_file_extension);
        }
        else
        {
            MCLog("not a supported image");
            ctxt.SetTheResultToStaticCString("not a supported format");
			return;
		}

        // SN-2014-12-18: [[ Bug 13860 ]] Only allow the use of the dataref as storing a filename for iOS
#ifdef __IOS__
        // PM-2014-12-12: [[ Bug 13860 ]] For referenced images we need the filename rather than the raw data
        if (t_image -> isReferencedImage())
        {
            // SN-2014-12-18: [[ Bug 13860 ]] We store the UTF-8 filename in the dataref for referenced images.
            MCAutoStringRef t_filename;
            char *t_utf8_filename;
            uindex_t t_byte_count;
            t_image -> getimagefilename(&t_filename);
            MCStringConvertToUTF8(*t_filename, t_utf8_filename, t_byte_count);
            MCDataCreateWithBytesAndRelease((byte_t*)t_utf8_filename, t_byte_count, &t_raw_data);
            t_is_raw_data = false;
        }
        else
        {
            t_image -> getrawdata(&t_raw_data);
            t_is_raw_data = true;
        }
#else
        t_image -> getrawdata(&t_raw_data);
#endif
    }
    // SN-2014-12-18: [[ Bug 13860 ]] If an extension was found, then we have raw data
    else
    {
        t_raw_data = *t_converted_data;
        t_is_raw_data = true;
    }
    
    // SN-2014-12-18: [[ Bug 13860 ]] Parameter added in case it's a filename, not raw data, in the DataRef
    MCSystemExportImageToAlbum(&t_save_result, *t_raw_data, p_file_name, *t_file_extension, t_is_raw_data);
    
    if (!MCStringIsEmpty(p_file_name))
    {
        if (*t_save_result != nil)
            ctxt.SetTheResultToValue(*t_save_result);
        else
            ctxt.SetTheResultToStaticCString("export failed");
    }
    else
        ctxt . SetTheResultToEmpty();
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

void MCMiscExecRequestPermission(MCExecContext& ctxt, MCStringRef p_permission, bool& r_granted)
{
    if (MCSystemRequestPermission(p_permission, r_granted))
        return;
    
    ctxt.Throw();
}

void MCMiscExecPermissionExists(MCExecContext& ctxt, MCStringRef p_permission, bool& r_exists)
{
    if (MCSystemPermissionExists(p_permission, r_exists))
        return;
    
    ctxt.Throw();
}

void MCMiscExecHasPermission(MCExecContext& ctxt, MCStringRef p_permission, bool& r_permission_granted)
{
    if (MCSystemHasPermission(p_permission, r_permission_granted))
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

void MCMiscGetBuildInfo(MCExecContext& ctxt, MCStringRef p_key, MCStringRef& r_value)
{
    if(MCSystemBuildInfo(p_key, r_value))
        return;
    
    ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCMiscExecEnableRemoteControl(MCExecContext& ctxt)
{
    if (MCSystemEnableRemoteControl())
        return;
    
    ctxt . Throw();
}

void MCMiscExecDisableRemoteControl(MCExecContext& ctxt)
{
    if (MCSystemDisableRemoteControl())
        return;
    
    ctxt . Throw();
}

void MCMiscGetRemoteControlEnabled(MCExecContext& ctxt, bool& r_enabled)
{
    if (MCSystemGetRemoteControlEnabled(r_enabled))
        return;
    
    ctxt . Throw();
}

void MCMiscSetRemoteControlDisplayProperties(MCExecContext& ctxt, MCArrayRef p_props)
{
    if (MCSystemSetRemoteControlDisplayProperties(ctxt, p_props))
        return;
    
    ctxt . Throw();
}
