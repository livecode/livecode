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

////////////////////////////////////////////////////////////////////////////////
//
//  Private Source File:
//    deploy.cpp
//
//  Description:
//    This file contains common methods for use by the various deploy modules.
//    In particular, it contains the implementation of the MCDeployFile
//    abstraction, and implementation of the ide 'deploy' command.
//
//  Changes:
//    2009-06-20 MW Created.
//    2009-06-22 MW Committed first working version.
//    2009-06-23 MW Refactored common project writing code to here.
//    2009-06-24 MW Expanded MCDeployWriteProject to allow creation of more
//                  robust project info suitable for both revlets and exes.
//    2009-06-25 MW Added error catching to the deploy command.
//                  Fixed glitch in md5_append_file to make sure stream begins
//                  at start.
//    2009-06-26 MW Fixed error in MCDeployCompress* methods causing wrong data
//                  to be used (buffer overwritten in wrong place).
//    2009-06-29 MW Added in native path conversion to file paths in deploy.
//                  Added implementation of MCDeployCatch - used to get the last
//                  error thrown by a deploy method.
//                  Added MCDeployErrorToString - used to get a human-readable
//                  account of a deploy error.
//                  Added setting of 'version_info' member of params to enable
//                  creation of VERSIONINFO resource on Windows.
//    2009-06-30 MW Restructed projectinfo output to use new extensible section
//                  based format.
//    2009-07-01 MW Added check for media edition in deploy command.
//    2009-07-04 MW Rewrote project generation portion to use new capsule
//                  abstraction.
//    2009-07-07 MW Added support for manifest embedding on Windows (currently
//                  Enterprise only).
//                  Added support spill file creation.
//    2009-07-12 MW Added support for _internal sign command for Windows
//                  Authenticode signing.
//    2010-05-08 MW Added support for payload option when building installers.
//                  Adjusted icon and manifest option fetching to use filepath
//                  routines.
//    2010-09-06 MW Removed edition restrictions for switch to LiveCode.
//
////////////////////////////////////////////////////////////////////////////////

#include "prefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "execpt.h"
#include "handler.h"
#include "scriptpt.h"
#include "variable.h"
#include "statemnt.h"
#include "globals.h"
#include "param.h"
#include "dispatch.h"
#include "osspec.h"

#include "ide.h"
#include "deploy.h"
#include "mode.h"
#include "license.h"

#include "capsule.h"

////////////////////////////////////////////////////////////////////////////////

extern Boolean InitSSLCrypt(void);

////////////////////////////////////////////////////////////////////////////////

static const char *kMCDeployArchitectureStrings[] =
{
    "",
    "i386",
    "x86-64",
    "armv6",
    "armv7",
    "armv7s",
    "arm64",
    "ppc",
    "ppc64",
    nil,
};

static bool MCDeployMapArchitectureString(const MCString& p_string, MCDeployArchitecture& r_architecture)
{
    for(uindex_t i = 0; kMCDeployArchitectureStrings[i] != nil; i++)
    {
        // As 'p_string' is an MCString the '==' operator does a caseless comparison.
        if (p_string == kMCDeployArchitectureStrings[i])
        {
            r_architecture = (MCDeployArchitecture)i;
            return true;
        }
    }
    
    return false;
}

static Exec_stat MCDeployPushMinOSVersion(MCDeployParameters& p_params, MCDeployArchitecture p_arch, const char *p_vers_string)
{
    // Use sscanf to parse out the version string. We don't check the return value of
    // sscanf as we don't care - any malformed / missing components will come out as
    // 0.
    int t_major, t_minor, t_inc;
    t_major = t_minor = t_inc = 0;
    sscanf(p_vers_string, "%d.%d.%d", &t_major, &t_minor, &t_inc);
    
    if (!MCMemoryResizeArray(p_params . min_os_version_count + 1, p_params . min_os_versions, p_params . min_os_version_count))
        return ES_ERROR;
    
    uint32_t t_version;
    t_version = (t_major & 0xFFFF) << 16;
    t_version |= (t_minor & 0xFF) << 8;
    t_version |= (t_inc & 0xFF) << 0;
    
    p_params . min_os_versions[p_params . min_os_version_count - 1] . architecture = p_arch;
    p_params . min_os_versions[p_params . min_os_version_count - 1] . version = t_version;
    
    return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

static bool MCDeployWriteDefinePrologueSection(const MCDeployParameters& p_params, MCDeployCapsuleRef p_capsule)
{
	MCCapsulePrologueSection t_prologue;
	
	return MCDeployCapsuleDefine(p_capsule, kMCCapsuleSectionTypePrologue, &t_prologue, sizeof(t_prologue));
}

// This method generates the standalone specific capsule elements. This is
// just a Standalone Prologue section at the moment.
static bool MCDeployWriteCapsuleDefineStandaloneSections(const MCDeployParameters& p_params, MCDeployCapsuleRef p_capsule)
{
	bool t_success;
	t_success = true;

	if (t_success)
		t_success = MCDeployWriteDefinePrologueSection(p_params, p_capsule);

	return t_success;
}

// This method constructs and then writes out a capsule to the given output file.
// The capsule contents is derived from the deploy parameters structure.
// The offset in the file after writing is returned in x_offset.
bool MCDeployWriteCapsule(const MCDeployParameters& p_params, MCDeployFileRef p_output, uint32_t& x_offset)
{
	bool t_success;
	t_success = true;

	// Open the stackfile.
	MCDeployFileRef t_stackfile;
	t_stackfile = NULL;
	if (t_success && !MCDeployFileOpen(p_params . stackfile, "rb", t_stackfile))
		t_success = MCDeployThrow(kMCDeployErrorNoStackfile);

	// Open the spill file, if required
	MCDeployFileRef t_spill;
	t_spill = NULL;
	if (t_success && p_params . spill != nil && !MCDeployFileOpen(p_params . spill, "wb+", t_spill))
		t_success = MCDeployThrow(kMCDeployErrorNoSpill);

	// First create our deployment capsule
	MCDeployCapsuleRef t_capsule;
	t_capsule = nil;
	if (t_success)
		t_success = MCDeployCapsuleCreate(t_capsule);

	// Next, the first thing to do is to add the the header section.
	if (t_success)
		t_success = MCDeployWriteCapsuleDefineStandaloneSections(p_params, t_capsule);

	// Add any redirects
	if (t_success)
		for(uint32_t i = 0; i < p_params . redirect_count && t_success; i++)
			t_success = MCDeployCapsuleDefine(t_capsule, kMCCapsuleSectionTypeRedirect, p_params . redirects[i], MCCStringLength(p_params . redirects[i]) + 1);

	// Now we add the main stack
	if (t_success)
		t_success = MCDeployCapsuleDefineFromFile(t_capsule, kMCCapsuleSectionTypeStack, t_stackfile);

	// Now we add the auxillary stackfiles, if any
	MCDeployFileRef *t_aux_stackfiles;
	t_aux_stackfiles = nil;
	if (t_success)
		t_success = MCMemoryNewArray(p_params . auxillary_stackfile_count, t_aux_stackfiles);
	if (t_success)
		for(uint32_t i = 0; i < p_params . auxillary_stackfile_count && t_success; i++)
		{
			if (t_success && !MCDeployFileOpen(p_params . auxillary_stackfiles[i], "rb", t_aux_stackfiles[i]))
				t_success = MCDeployThrow(kMCDeployErrorNoAuxStackfile);
			if (t_success)
				t_success = MCDeployCapsuleDefineFromFile(t_capsule, kMCCapsuleSectionTypeAuxillaryStack, t_aux_stackfiles[i]);
		}
	
	// Now add the externals, if any
	if (t_success)
		for(uint32_t i = 0; i < p_params . external_count && t_success; i++)
			t_success = MCDeployCapsuleDefine(t_capsule, kMCCapsuleSectionTypeExternal, p_params . externals[i], MCCStringLength(p_params . externals[i]) + 1);

	// Now add the startup script, if any.
	if (t_success && p_params . startup_script != nil)
		t_success = MCDeployCapsuleDefine(t_capsule, kMCCapsuleSectionTypeStartupScript, p_params . startup_script, MCCStringLength(p_params . startup_script) + 1);

	// Now a digest
	if (t_success)
		t_success = MCDeployCapsuleChecksum(t_capsule);

	// Finally the epilogue
	if (t_success)
		t_success = MCDeployCapsuleDefine(t_capsule, kMCCapsuleSectionTypeEpilogue, nil, 0);

	// Now we write it
	if (t_success)
		t_success = MCDeployCapsuleGenerate(t_capsule, p_output, t_spill, x_offset);

	MCDeployCapsuleDestroy(t_capsule);
	for(uint32_t i = 0; i < p_params . auxillary_stackfile_count; i++)
		MCDeployFileClose(t_aux_stackfiles[i]);
	MCMemoryDeleteArray(t_aux_stackfiles);
	MCDeployFileClose(t_spill);
	MCDeployFileClose(t_stackfile);

	return t_success;
}

// This method writes out a project capsule. This consists of a length uint32_t
// followed by the capsule data. The size returned always falls on a 4-byte
// boundary.
bool MCDeployWriteProject(const MCDeployParameters& p_params, bool p_to_network, MCDeployFileRef p_output, uint32_t p_output_offset, uint32_t& r_project_size)
{
	bool t_success;
	t_success = true;

	// A capsule struct is simply a uint32_t followed by the data
	uint32_t t_offset;
	t_offset = p_output_offset + sizeof(uint32_t);

	// First write the capsule to the output file, leaving room for the size field.
	// Note that a capsule is always a multiple of four bytes in length, so offset
	// will be rounded to a nice value.
	if (t_success)
		t_success = MCDeployWriteCapsule(p_params, p_output, t_offset);

	// Work out the size of the capsule struct (including size field)
	uint32_t t_project_size;
	if (t_success)
		t_project_size = t_offset - p_output_offset;

	// Now write out the size field
	if (t_success)
	{
		uint32_t t_swapped_size;
		t_swapped_size = t_project_size;
		if (p_params . spill != NULL)
			t_swapped_size |= 1U << 31;
		MCDeployByteSwap32(p_to_network, t_swapped_size); 
		t_success = MCDeployFileWriteAt(p_output, &t_swapped_size, sizeof(uint32_t), p_output_offset);
	}

	// Return the project size 
	if (t_success)
		r_project_size = t_project_size;

	return t_success;
}

// This method writes out a payload capsule. This is a length uint32_t followed
// by the contents of the given file. The size returned always falls on a 4-byte
// boundary.
bool MCDeployWritePayload(const MCDeployParameters& p_params, bool p_to_network, MCDeployFileRef p_output, uint32_t p_output_offset, uint32_t& r_payload_size)
{
	bool t_success;
	t_success = true;

	// First try to open the payload file
	MCDeployFileRef t_payload;
	t_payload = nil;
	if (t_success && !MCDeployFileOpen(p_params . payload, "rb", t_payload))
		t_success = MCDeployThrow(kMCDeployErrorNoPayload);

	// Next measure the file to find out how big it is
	uint32_t t_payload_size;
	t_payload_size = 0;
	if (t_success)
		t_success = MCDeployFileMeasure(t_payload, t_payload_size);

	// Now write out the uint32_t size field (including itself)
	if (t_success)
	{
		uint32_t t_swapped_size;
		t_swapped_size = t_payload_size + sizeof(uint32_t);
		MCDeployByteSwap32(p_to_network, t_swapped_size);
		t_success = MCDeployFileWriteAt(p_output, &t_swapped_size, sizeof(uint32_t), p_output_offset);
	}
	
	// Next copy across the payload data
	if (t_success)
		t_success = MCDeployFileCopy(p_output, p_output_offset + sizeof(uint32_t), t_payload, 0, t_payload_size);

	// Return the actual payload size, including padding up to nearest 4 byte
	// boundary.
	if (t_success)
	{
		t_payload_size += sizeof(uint32_t);
		t_payload_size = (t_payload_size + 3) & ~3;
		r_payload_size = t_payload_size;
	}

	// Close any file that we open
	if (t_payload != nil)
		MCDeployFileClose(t_payload);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

MCIdeDeploy::MCIdeDeploy(void)
{
	m_params = NULL;
}

MCIdeDeploy::~MCIdeDeploy(void)
{
	delete m_params;
}

Parse_stat MCIdeDeploy::parse(MCScriptPoint& sp)
{
	Symbol_type t_type;
	if (sp . next(t_type) == PS_NORMAL && t_type == ST_ID)
	{
		if (sp . gettoken() == "windows")
			m_platform = PLATFORM_WINDOWS;
		else if (sp . gettoken() == "linux")
			m_platform = PLATFORM_LINUX;
		else if (sp . gettoken() == "macosx")
			m_platform = PLATFORM_MACOSX;
		else if (sp . gettoken() == "ios")
			m_platform = PLATFORM_IOS;
		else if (sp . gettoken() == "android")
			m_platform = PLATFORM_ANDROID;
		else if (sp . gettoken() == "winmobile")
			m_platform = PLATFORM_WINMOBILE;
		else if (sp . gettoken() == "linuxmobile")
			m_platform = PLATFORM_LINUXMOBILE;
		else if (sp . gettoken() == "iosembedded")
			m_platform = PLATFORM_IOS_EMBEDDED;
		else if (sp . gettoken() == "androidembedded")
			m_platform = PLATFORM_ANDROID_EMBEDDED;
		else
			return PS_ERROR;
	}
	else
		return PS_ERROR;

	return sp . parseexp(False, True, &m_params);
}

static Exec_stat fetch_opt_cstring(MCExecPoint& ep, MCVariableValue *array, const char *key, char*& r_result)
{
	if (array -> fetch_element(ep, key) != ES_NORMAL)
		return ES_ERROR;
	if (ep . getsvalue() == MCnullmcstring)
		r_result = NULL;
	else
		r_result = ep . getsvalue() . clone();
	return ES_NORMAL;
}

static Exec_stat fetch_opt_uint32(MCExecPoint& ep, MCVariableValue *array, const char *key, uint32_t& r_result)
{
	if (array -> fetch_element(ep, key) != ES_NORMAL)
		return ES_ERROR;
	if (ep . ston() != ES_NORMAL)
		return ES_ERROR;
	r_result = ep . getuint4();
	return ES_NORMAL;
}

static Exec_stat fetch_opt_filepath(MCExecPoint& ep, MCVariableValue *array, const char *key, char*& r_result)
{
	if (array -> fetch_element(ep, key) != ES_NORMAL)
		return ES_ERROR;
	if (ep . getsvalue() == MCnullmcstring)
		r_result = NULL;
	else
		r_result = MCS_resolvepath(ep . getcstring());
	return ES_NORMAL;
}

static Exec_stat fetch_opt_boolean(MCExecPoint& ep, MCVariableValue *array, const char *key, bool& r_result)
{
	if (array -> fetch_element(ep, key) != ES_NORMAL)
		return ES_ERROR;
	if (ep . getsvalue() == MCtruemcstring)
		r_result = true;
	else if (ep . getsvalue() == MCfalsemcstring)
		r_result = false;
	else if (!ep . isempty())
		return ES_ERROR;
	return ES_NORMAL;
}

static Exec_stat fetch_cstring(MCExecPoint& ep, MCVariableValue *array, const char *key, char *& r_result)
{
	if (!array -> has_element(ep, key))
		return ES_ERROR;
	return fetch_opt_cstring(ep, array, key, r_result);
}

static Exec_stat fetch_uint32(MCExecPoint& ep, MCVariableValue *array, const char *key, uint32_t& r_result)
{
	if (!array -> has_element(ep, key))
		return ES_ERROR;
	return fetch_opt_uint32(ep, array, key, r_result);
}

static Exec_stat fetch_filepath(MCExecPoint& ep, MCVariableValue *array, const char *key, char *& r_result)
{
	if (!array -> has_element(ep, key))
		return ES_ERROR;
	return fetch_opt_filepath(ep, array, key, r_result);
}

static Exec_stat fetch_filepath_array(MCExecPoint& ep, MCVariableValue *array, const char *key, char**& r_filepaths, uint32_t& r_filepath_count)
{
	bool t_success;
	t_success = true;
	
	char *t_paths_string;
	t_paths_string = nil;
	if (t_success)
		if (fetch_opt_cstring(ep, array, key, t_paths_string) == ES_ERROR)
			t_success = false;
	
	char **t_paths;
	uint32_t t_path_count;
	t_paths = nil;
	t_path_count = 0;
	if (t_success)
		t_success = MCCStringSplit(t_paths_string, '\n', t_paths, t_path_count);
	
	if (t_success)
		for(uint32_t i = 0; i < t_path_count; i++)
		{
			char *t_unresolved_path;
			t_unresolved_path = t_paths[i];
			t_paths[i] = MCS_resolvepath(t_unresolved_path);
			MCCStringFree(t_unresolved_path);
		}
	
	if (t_success)
	{
		r_filepaths = t_paths;
		r_filepath_count = t_path_count;
	}
	else
		MCCStringArrayFree(t_paths, t_path_count);
	
	MCCStringFree(t_paths_string);
	
	return t_success ? ES_NORMAL : ES_ERROR;
}

static Exec_stat fetch_cstring_array(MCExecPoint& ep, MCVariableValue *array, const char *key, char**& r_strings, uint32_t& r_string_count)
{
	bool t_success;
	t_success = true;
	
	char *t_strings_list;
	t_strings_list = nil;
	if (t_success)
		if (fetch_opt_cstring(ep, array, key, t_strings_list) == ES_ERROR)
			t_success = false;
	
	char **t_strings;
	uint32_t t_string_count;
	t_strings = nil;
	t_string_count = 0;
	if (t_success)
		t_success = MCCStringSplit(t_strings_list, '\n', t_strings, t_string_count);
	
	if (t_success)
	{
		r_strings = t_strings;
		r_string_count = t_string_count;
	}
	
	MCCStringFree(t_strings_list);
	
	return t_success ? ES_NORMAL : ES_ERROR;
	
}

Exec_stat MCIdeDeploy::exec(MCExecPoint& ep)
{
	Exec_stat t_stat;
	t_stat = ES_NORMAL;

	bool t_soft_error;
	t_soft_error = false;

	// Clear the result as we return an error there
	MCresult -> clear();

	if (t_stat == ES_NORMAL)
		t_stat = m_params -> eval(ep);

	MCVariableValue *t_array;
	if (t_stat == ES_NORMAL)
	{
		t_array = ep . getarray();
		if (t_array == NULL)
			return ES_ERROR;
	}

	MCExecPoint ep2(ep);

	MCDeployParameters t_params;
	memset(&t_params, 0, sizeof(MCDeployParameters));

	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_filepath(ep2, t_array, "engine_ppc", t_params . engine_ppc);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_filepath(ep2, t_array, "engine_x86", t_params . engine_x86);
	if (t_stat == ES_NORMAL && t_params . engine_ppc == NULL && t_params . engine_x86 == NULL)
		t_stat = fetch_opt_filepath(ep2, t_array, "engine", t_params . engine);

	if (t_stat == ES_NORMAL)
		t_stat = fetch_filepath(ep2, t_array, "stackfile", t_params . stackfile);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_filepath_array(ep2, t_array, "auxillary_stackfiles", t_params . auxillary_stackfiles, t_params . auxillary_stackfile_count);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_cstring_array(ep2, t_array, "externals", t_params . externals, t_params . external_count);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_cstring(ep2, t_array, "startup_script", t_params . startup_script);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_cstring_array(ep2, t_array, "redirects", t_params . redirects, t_params . redirect_count);

	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_filepath(ep2, t_array, "appicon", t_params . app_icon);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_filepath(ep2, t_array, "docicon", t_params . doc_icon);

	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_filepath(ep2, t_array, "manifest", t_params . manifest);

	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_filepath(ep2, t_array, "payload", t_params . payload);

	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_filepath(ep2, t_array, "spill", t_params . spill);

	if (t_stat == ES_NORMAL)
		t_stat = fetch_filepath(ep2, t_array, "output", t_params . output);

	if (t_stat == ES_NORMAL)
	{
		t_stat = t_array -> fetch_element(ep2, "version");
		if (t_stat == ES_NORMAL && ep2 . getarray() != NULL)
			t_params . version_info = new MCVariableValue(*ep2 . getarray());
	}
    
    // The 'min_os_version' is either a string or an array. If it is a string then
    // it encodes the version against the 'Unknown' architecture which is interpreted
    // by the deploy command to mean all architectures. Otherwise, the keys in the
    // array are assumed to be architecture names and each is pushed on the array.
    // If multiple entries are present, then the 'unknown' mapping is used for any
    // architecture not explicitly specified. The current architecture strings that are
    // known are:
    //   i386, x86_64, armv6, armv7, armv7s, arm64, ppc, ppc64
    // The empty string is taken to be 'unknown'.
    if (t_stat == ES_NORMAL)
    {
        t_stat = t_array -> fetch_element(ep2, "min_os_version");
        if (t_stat == ES_NORMAL)
        {
            if (ep2 . getformat() == VF_ARRAY)
            {
                MCExecPoint ep3(ep2);
                MCHashentry *t_entry;
                uindex_t t_index;
                t_entry = nil;
                t_index = 0;
                for(;;)
                {
                    if (t_stat == ES_ERROR)
                        break;
                    
                    t_entry = ep2 . getarray() -> get_array() -> getnextelement(t_index, t_entry, False, ep);
                    if (t_entry == nil)
                        break;
                    
                    MCDeployArchitecture t_arch;
                    if (!MCDeployMapArchitectureString(t_entry -> string, t_arch))
                        continue;
                    
                    t_stat = t_entry -> value . fetch(ep3);
                    
                    if (t_stat == ES_NORMAL)
                        t_stat = MCDeployPushMinOSVersion(t_params, t_arch, ep3 . getcstring());
                }
            }
            else
                t_stat = MCDeployPushMinOSVersion(t_params, kMCDeployArchitecture_Unknown, ep2 . getcstring());
        }
    }
	
	// If platform is iOS and we are not Mac then error
#ifndef _MACOSX
	if (t_stat == ES_NORMAL && (m_platform == PLATFORM_IOS || m_platform == PLATFORM_IOS_EMBEDDED))
	{
		MCresult -> sets("ios deployment not supported on this platform");
		t_soft_error = true;
		t_stat = ES_ERROR;
	}
#endif

	// Now, if we are not licensed for a target, then its an error.
	bool t_is_licensed;
	t_is_licensed = false;
	if (m_platform == PLATFORM_WINDOWS)
		t_is_licensed = (MClicenseparameters . deploy_targets & kMCLicenseDeployToWindows) != 0;
	else if (m_platform == PLATFORM_MACOSX)
		t_is_licensed = (MClicenseparameters . deploy_targets & kMCLicenseDeployToMacOSX) != 0;
	else if (m_platform == PLATFORM_LINUX)
		t_is_licensed = (MClicenseparameters . deploy_targets & kMCLicenseDeployToLinux) != 0;
	else if (m_platform == PLATFORM_IOS)
		t_is_licensed = (MClicenseparameters . deploy_targets & kMCLicenseDeployToIOS) != 0;
	else if (m_platform == PLATFORM_ANDROID)
		t_is_licensed = (MClicenseparameters . deploy_targets & kMCLicenseDeployToAndroid) != 0;
	else if (m_platform == PLATFORM_IOS_EMBEDDED)
		t_is_licensed = (MClicenseparameters . deploy_targets & kMCLicenseDeployToIOSEmbedded) != 0;
	else if (m_platform == PLATFORM_ANDROID_EMBEDDED)
		t_is_licensed = (MClicenseparameters . deploy_targets & kMCLicenseDeployToAndroidEmbedded) != 0;

	if (!t_is_licensed)
	{
		MCresult -> sets("not licensed to deploy to target platform");
		t_soft_error = true;
		t_stat = ES_ERROR;
	}

	if (t_stat == ES_NORMAL)
	{
		if (m_platform == PLATFORM_WINDOWS)
			MCDeployToWindows(t_params);
		else if (m_platform == PLATFORM_LINUX)
			MCDeployToLinux(t_params);
		else if (m_platform == PLATFORM_MACOSX)
			MCDeployToMacOSX(t_params);
		else if (m_platform == PLATFORM_IOS)
			MCDeployToIOS(t_params, false);
		else if (m_platform == PLATFORM_ANDROID)
			MCDeployToAndroid(t_params);
		else if (m_platform == PLATFORM_IOS_EMBEDDED)
			MCDeployToIOS(t_params, true);

		MCDeployError t_error;
		t_error = MCDeployCatch();
		if (t_error != kMCDeployErrorNone)
			MCresult -> sets(MCDeployErrorToString(t_error));
		else
			MCresult -> clear();
	}

	delete t_params . output;
	delete t_params . spill;
	delete t_params . stackfile;
	MCCStringArrayFree(t_params . auxillary_stackfiles, t_params . auxillary_stackfile_count);
	MCCStringArrayFree(t_params . externals, t_params . external_count);
	delete t_params . startup_script;
	MCCStringArrayFree(t_params . redirects, t_params . redirect_count);
	delete t_params . app_icon;
	delete t_params . doc_icon;
	delete t_params . manifest;
	delete t_params . payload;
	delete t_params . engine;
	delete t_params . engine_ppc;
	delete t_params . engine_x86;
	delete t_params . version_info;
    MCMemoryDeleteArray(t_params . min_os_versions);

	if (t_stat == ES_ERROR && t_soft_error)
		return ES_NORMAL;

	return t_stat;
}

////////////////////////////////////////////////////////////////////////////////

MCIdeSign::MCIdeSign(void)
{
	m_params = NULL;
}

MCIdeSign::~MCIdeSign(void)
{
	delete m_params;
}

Parse_stat MCIdeSign::parse(MCScriptPoint& sp)
{
	Symbol_type t_type;

	if (sp . next(t_type) == PS_NORMAL && t_type == ST_ID)
	{
		if (sp . gettoken() == "windows")
			m_platform = PLATFORM_WINDOWS;
		else if (sp . gettoken() == "linux")
			m_platform = PLATFORM_LINUX;
		else if (sp . gettoken() == "macosx")
			m_platform = PLATFORM_MACOSX;
		else
			return PS_ERROR;
	}
	else
		return PS_ERROR;

	return sp . parseexp(False, True, &m_params);
}

Exec_stat MCIdeSign::exec(MCExecPoint& ep)
{
	Exec_stat t_stat;
	t_stat = ES_NORMAL;

	// Clear the result as we return an error there
	MCresult -> clear();

	if (t_stat == ES_NORMAL)
		t_stat = m_params -> eval(ep);

	MCVariableValue *t_array;
	if (t_stat == ES_NORMAL)
	{
		t_array = ep . getarray();
		if (t_array == NULL)
			return ES_ERROR;
	}

	MCExecPoint ep2(ep);

	MCDeploySignParameters t_params;
	memset(&t_params, 0, sizeof(MCDeploySignParameters));

	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_cstring(ep2, t_array, "passphrase", t_params . passphrase);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_filepath(ep2, t_array, "certificate", t_params . certificate);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_filepath(ep2, t_array, "privatekey", t_params . privatekey);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_filepath(ep2, t_array, "certstore", t_params . certstore);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_cstring(ep2, t_array, "timestamper", t_params . timestamper);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_cstring(ep2, t_array, "description", t_params . description);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_cstring(ep2, t_array, "url", t_params . url);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_filepath(ep2, t_array, "input", t_params . input);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_filepath(ep2, t_array, "output", t_params . output);

	if (t_stat == ES_NORMAL)
		if (t_params . certstore != NULL && (t_params . certificate != NULL || t_params . privatekey != NULL))
			t_stat = ES_ERROR;

	if (t_stat == ES_NORMAL)
		if (t_params . certstore == NULL && (t_params . certificate == NULL || t_params . privatekey == NULL))
			t_stat = ES_ERROR;

	bool t_can_sign;
	t_can_sign = true;
	if (t_stat == ES_NORMAL && !InitSSLCrypt())
	{
		MCresult -> sets("could not initialize SSL");
		t_can_sign = false;
	}

	if (t_can_sign && t_stat == ES_NORMAL)
	{
		if (m_platform == PLATFORM_WINDOWS)
			MCDeploySignWindows(t_params);

		MCDeployError t_error;
		t_error = MCDeployCatch();
		if (t_error != kMCDeployErrorNone)
			MCresult -> sets(MCDeployErrorToString(t_error));
	}

	delete t_params . passphrase;
	delete t_params . certificate;
	delete t_params . privatekey;
	delete t_params . certstore;
	delete t_params . timestamper;
	delete t_params . description;
	delete t_params . url;
	delete t_params . input;
	delete t_params . output;

	return t_stat;
}

////////////////////////////////////////////////////////////////////////////////

MCIdeDiet::MCIdeDiet(void)
{
	m_params = NULL;
}

MCIdeDiet::~MCIdeDiet(void)
{
	delete m_params;
}

Parse_stat MCIdeDiet::parse(MCScriptPoint& sp)
{
	Symbol_type t_type;

	if (sp . next(t_type) == PS_NORMAL && t_type == ST_ID)
	{
		if (sp . gettoken() == "windows")
			m_platform = PLATFORM_WINDOWS;
		else if (sp . gettoken() == "linux")
			m_platform = PLATFORM_LINUX;
		else if (sp . gettoken() == "macosx")
			m_platform = PLATFORM_MACOSX;
		else
			return PS_ERROR;
	}
	else
		return PS_ERROR;

	return sp . parseexp(False, True, &m_params);
}

Exec_stat MCIdeDiet::exec(MCExecPoint& ep)
{
	Exec_stat t_stat;
	t_stat = ES_NORMAL;

	// Clear the result as we return an error there
	MCresult -> clear();

	if (t_stat == ES_NORMAL)
		t_stat = m_params -> eval(ep);

	MCVariableValue *t_array;
	if (t_stat == ES_NORMAL)
	{
		t_array = ep . getarray();
		if (t_array == NULL)
			return ES_ERROR;
	}

	MCExecPoint ep2(ep);

	MCDeployDietParameters t_params;
	memset(&t_params, 0, sizeof(MCDeployDietParameters));

	if (t_stat == ES_NORMAL)
		t_stat = fetch_filepath(ep2, t_array, "input", t_params . input);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_filepath(ep2, t_array, "output", t_params . output);

	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_boolean(ep2, t_array, "keep_x86", t_params . keep_x86);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_boolean(ep2, t_array, "keep_x86_64", t_params . keep_x86_64);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_boolean(ep2, t_array, "keep_ppc", t_params . keep_ppc);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_boolean(ep2, t_array, "keep_ppc64", t_params . keep_ppc64);
	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_boolean(ep2, t_array, "keep_arm", t_params . keep_arm);

	if (t_stat == ES_NORMAL)
		t_stat = fetch_opt_boolean(ep2, t_array, "keep_debug_symbols", t_params . keep_debug_symbols);


	if (t_stat == ES_NORMAL)
	{
		if (m_platform == PLATFORM_MACOSX)
			MCDeployDietMacOSX(t_params);

		MCDeployError t_error;
		t_error = MCDeployCatch();
		if (t_error != kMCDeployErrorNone)
			MCresult -> sets(MCDeployErrorToString(t_error));
	}

	delete t_params . input;
	delete t_params . output;

	return t_stat;
}

////////////////////////////////////////////////////////////////////////////////

static void stdfile_log(void *stream, const char *format, ...)
{
	va_list t_args;
	va_start(t_args, format);
	vfprintf((FILE *)stream, format, t_args);
	va_end(t_args);
}

/////////

MCIdeDmgDump::MCIdeDmgDump(void)
{
	m_filename = NULL;
}

MCIdeDmgDump::~MCIdeDmgDump(void)
{
	delete m_filename;
}

Parse_stat MCIdeDmgDump::parse(MCScriptPoint& sp)
{
	return sp . parseexp(False, True, &m_filename);
}

Exec_stat MCIdeDmgDump::exec(MCExecPoint& ep)
{
	Exec_stat t_stat;
	t_stat = ES_NORMAL;

	// Clear the result as we return an error there
	MCresult -> clear();

	if (t_stat == ES_NORMAL)
		t_stat = m_filename -> eval(ep);

	if (t_stat == ES_NORMAL)
	{
		FILE *t_output;
		t_output = fopen("C:\\Users\\Mark\\Desktop\\dmg.txt", "w");
		MCDeployDmgDump(ep . getcstring(), stdfile_log, t_output);
		fclose(t_output);
	}

	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

MCIdeDmgBuild::MCIdeDmgBuild(void)
{
	m_items = NULL;
	m_filename = NULL;
}

MCIdeDmgBuild::~MCIdeDmgBuild(void)
{
	delete m_items;
	delete m_filename;
}

Parse_stat MCIdeDmgBuild::parse(MCScriptPoint& sp)
{
	if (sp . parseexp(False, True, &m_items) != PS_NORMAL)
		return PS_ERROR;
	if (sp . skip_token(SP_FACTOR, TT_TO, PT_TO) != PS_NORMAL)
		return PS_ERROR;
	if (sp . parseexp(False, True, &m_filename) != PS_NORMAL)
		return PS_ERROR;
	return PS_NORMAL;
}

Exec_stat MCIdeDmgBuild::exec(MCExecPoint& ep)
{
	Exec_stat t_stat;
	t_stat = ES_NORMAL;

	// Clear the result as we return an error there
	MCresult -> clear();

	/////////

	if (t_stat == ES_NORMAL)
		t_stat = m_items -> eval(ep);
		
	if (t_stat == ES_NORMAL && ep . getformat() != VF_ARRAY)
		t_stat = ES_ERROR;

	MCVariableArray *t_array;
	if (t_stat == ES_NORMAL)
	{
		t_array = ep . getarray() -> get_array();
		if (!t_array -> issequence())
			t_stat = ES_ERROR;
	}

	MCDeployDmgItem *t_items;
	uint32_t t_item_count;
	t_items = nil;
	t_item_count = 0;
	if (t_stat == ES_NORMAL)
	{
		if (MCMemoryNewArray(t_array -> getnfilled(), t_items))
			t_item_count = t_array -> getnfilled();
		else
			t_stat = ES_ERROR;
	}

	MCExecPoint ep2(ep);
	if (t_stat == ES_NORMAL)
		for(uint32_t i = 0; i < t_item_count && t_stat == ES_NORMAL; i++)
		{
			MCHashentry *t_element;
			t_element = t_array -> lookupindex(i + 1, False);
			if (t_element == nil)
				t_stat = ES_ERROR;

			if (t_stat == ES_NORMAL)
				t_stat = t_element -> value . fetch(ep2);

			MCVariableValue *t_item_array;
			if (t_stat == ES_NORMAL)
			{
				t_item_array = ep2 . getarray();
				if (t_item_array == nil)
					t_stat = ES_ERROR;
			}

			if (t_stat == ES_NORMAL)
			{
				t_stat = t_item_array -> fetch_element(ep2, "type");
				if (t_stat == ES_NORMAL)
				{
					if (ep2 . getsvalue() == "folder")
						t_items[i] . is_folder = true;
					else if (ep2 . getsvalue() == "file")
						t_items[i] . is_folder = false;
					else
						t_stat = ES_ERROR;
				}
			}

			if (t_stat == ES_NORMAL)
				t_stat = fetch_uint32(ep2, t_item_array, "parent", t_items[i] . parent);
			if (t_stat == ES_NORMAL)
				t_stat = fetch_cstring(ep2, t_item_array, "name", t_items[i] . name);

			if (t_stat == ES_NORMAL)
				t_stat = fetch_opt_uint32(ep2, t_item_array, "owner_id", t_items[i] . owner_id);
			if (t_stat == ES_NORMAL)
				t_stat = fetch_opt_uint32(ep2, t_item_array, "group_id", t_items[i] . group_id);
			if (t_stat == ES_NORMAL)
				t_stat = fetch_opt_uint32(ep2, t_item_array, "file_mode", t_items[i] . file_mode);

			if (t_stat == ES_NORMAL)
				t_stat = fetch_opt_uint32(ep2, t_item_array, "create_date", t_items[i] . create_date);
			if (t_stat == ES_NORMAL)
				t_stat = fetch_opt_uint32(ep2, t_item_array, "content_mod_date", t_items[i] . content_mod_date);
			if (t_stat == ES_NORMAL)
				t_stat = fetch_opt_uint32(ep2, t_item_array, "attribute_mod_date", t_items[i] . attribute_mod_date);
			if (t_stat == ES_NORMAL)
				t_stat = fetch_opt_uint32(ep2, t_item_array, "access_date", t_items[i] . access_date);
			if (t_stat == ES_NORMAL)
				t_stat = fetch_opt_uint32(ep2, t_item_array, "backup_date", t_items[i] . backup_date);
		}

	/////////

	if (t_stat == ES_NORMAL)
		t_stat = m_filename -> eval(ep);

	if (t_stat == ES_NORMAL)
	{
		MCDeployDmgParameters t_params;
		t_params . items = t_items;
		t_params . item_count = t_item_count;
		t_params . output = (char *)ep . getcstring();
		if (!MCDeployDmgBuild(t_params))
			t_stat = ES_ERROR;
	}

	////////

	for(uint32_t i = 0; i < t_item_count; i++)
		MCCStringFree(t_items[i] . name);
	MCMemoryDeleteArray(t_items);

	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

MCIdeExtract::MCIdeExtract(void)
{
	m_filename = nil;
	m_segment_name = nil;
	m_section_name = nil;
	m_it = nil;
}

MCIdeExtract::~MCIdeExtract(void)
{
	delete m_filename;
	delete m_segment_name;
	delete m_section_name;
	delete m_it;
}

// _internal extract x y from z
Parse_stat MCIdeExtract::parse(MCScriptPoint& sp)
{
	if (sp . parseexp(False, True, &m_segment_name) != PS_NORMAL)
		return PS_ERROR;
	if (sp . parseexp(False, True, &m_section_name) != PS_NORMAL)
		return PS_ERROR;
	if (sp . skip_token(SP_FACTOR, TT_FROM, PT_FROM) != PS_NORMAL)
		return PS_ERROR;
	if (sp . parseexp(False, True, &m_filename) != PS_NORMAL)
		return PS_ERROR;
	
	getit(sp, m_it);
	
	return PS_NORMAL;
}

Exec_stat MCIdeExtract::exec(MCExecPoint& ep)
{
	Exec_stat t_stat;
	t_stat = ES_NORMAL;
	
	char *t_segment;
	t_segment = nil;
	if (t_stat == ES_NORMAL)
		t_stat = m_segment_name -> eval(ep);
	if (t_stat == ES_NORMAL)
		t_segment = ep . getsvalue() . clone();
	
	char *t_section;
	t_section = nil;
	if (t_stat == ES_NORMAL)
		t_stat = m_section_name -> eval(ep);
	if (t_stat == ES_NORMAL)
		t_section = ep . getsvalue() . clone();
	
	char *t_filename;
	t_filename = nil;
	if (t_stat == ES_NORMAL)
		t_stat = m_filename -> eval(ep);
	if (t_stat == ES_NORMAL)
		t_filename = ep . getsvalue() . clone();
	
	void *t_data;
	uint32_t t_data_size;
	if (t_stat == ES_NORMAL)
		t_stat = MCDeployExtractMacOSX(t_filename, t_segment, t_section, t_data, t_data_size);
	
	if (t_stat == ES_NORMAL)
	{
		ep . grabbuffer((char *)t_data, t_data_size);
		m_it -> set(ep);
	}
	else
		m_it -> clear();
	
	delete t_filename;
	delete t_section;
	delete t_segment;
	
	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////
