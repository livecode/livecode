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

#include "exec.h"

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
#include "stacksecurity.h"

#include "debug.h"

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

typedef struct
{
    MCDeployParameters* params;
    MCExecContext* ctxt;
}
MCDeployArrayApplyCallbackContext;

static bool MCDeployMapArchitectureString(MCStringRef p_string, MCDeployArchitecture& r_architecture)
{
    for(uindex_t i = 0; kMCDeployArchitectureStrings[i] != nil; i++)
    {
        // As 'p_string' is an MCString the '==' operator does a caseless comparison.
        if (MCStringIsEqualToCString(p_string, kMCDeployArchitectureStrings[i], kMCStringOptionCompareCaseless))
        {
            r_architecture = (MCDeployArchitecture)i;
            return true;
        }
    }

    return false;
}

static bool MCDeployPushMinOSVersion(MCDeployParameters* p_params, MCDeployArchitecture p_arch, MCStringRef p_vers_string)
{
    // Use sscanf to parse out the version string. We don't check the return value of
    // sscanf as we don't care - any malformed / missing components will come out as
    // 0.
    int t_major, t_minor, t_inc;
    t_major = t_minor = t_inc = 0;

    MCAutoPointer<char> t_native_string;
    MCStringConvertToCString(p_vers_string, &t_native_string);
    sscanf(*t_native_string, "%d.%d.%d", &t_major, &t_minor, &t_inc);

    if (!MCMemoryResizeArray(p_params -> min_os_version_count + 1, p_params -> min_os_versions, p_params -> min_os_version_count))
        return false;

    uint32_t t_version;
    t_version = (t_major & 0xFFFF) << 16;
    t_version |= (t_minor & 0xFF) << 8;
    t_version |= (t_inc & 0xFF) << 0;

    p_params -> min_os_versions[p_params -> min_os_version_count - 1] . architecture = p_arch;
    p_params -> min_os_versions[p_params -> min_os_version_count - 1] . version = t_version;

    return true;
}

static bool MCDeployGetArchitectures(void *context, MCArrayRef array, MCNameRef key, MCValueRef value)
{
    MCDeployArrayApplyCallbackContext *t_context;
    t_context = (MCDeployArrayApplyCallbackContext*)context;

    MCAutoStringRef t_value_as_string;
    MCDeployArchitecture t_arch;

    if (!t_context -> ctxt -> ConvertToString(value, &t_value_as_string))
        return false;

    if (!MCDeployMapArchitectureString(MCNameGetString(key), t_arch))
        return false;

    if (!MCDeployPushMinOSVersion(t_context -> params, t_arch, *t_value_as_string))
        return false;

    return true;
}

bool MCDeployParameters::InitWithArray(MCExecContext &ctxt, MCArrayRef p_array)
{
	MCStringRef t_temp_string;
	MCArrayRef t_temp_array;
	
	if (!ctxt.CopyOptElementAsFilepath(p_array, MCNAME("engine_ppc"), false, t_temp_string))
		return false;
	MCValueAssign(engine_ppc, t_temp_string);
	MCValueRelease(t_temp_string);
	
	if (!ctxt.CopyOptElementAsFilepath(p_array, MCNAME("engine_x86"), false, t_temp_string))
		return false;
	MCValueAssign(engine_x86, t_temp_string);
	MCValueRelease(t_temp_string);
	
	if (MCStringIsEmpty(engine_ppc) && MCStringIsEmpty(engine_x86))
	{
		if (!ctxt.CopyElementAsFilepath(p_array, MCNAME("engine"), false, t_temp_string))
			return false;
		MCValueAssign(engine, t_temp_string);
		MCValueRelease(t_temp_string);
	}
	
	if (!ctxt.CopyOptElementAsFilepath(p_array, MCNAME("stackfile"), false, t_temp_string))
		return false;
	MCValueAssign(stackfile, t_temp_string);
	MCValueRelease(t_temp_string);
	
    if (!ctxt.CopyOptElementAsFilepathArray(p_array, MCNAME("auxiliary_stackfiles"), false, t_temp_array))
		return false;
    MCValueAssign(auxiliary_stackfiles, t_temp_array);
	MCValueRelease(t_temp_array);
	
    // The externals listed by the IDE are LF separated
	if (!ctxt.CopyOptElementAsString(p_array, MCNAME("externals"), false, t_temp_string))
		return false;
    MCStringSplit(t_temp_string, MCSTR("\n"), nil, kMCStringOptionCompareExact, t_temp_array);
    MCValueAssign(externals, t_temp_array);
    MCValueRelease(t_temp_string);
    MCValueRelease(t_temp_array);
	
	if (!ctxt.CopyOptElementAsString(p_array, MCNAME("startup_script"), false, t_temp_string))
		return false;
	MCValueAssign(startup_script, t_temp_string);
	MCValueRelease(t_temp_string);
	
    // The redirects listed by the IDE are LF separated
	if (!ctxt.CopyOptElementAsString(p_array, MCNAME("redirects"), false, t_temp_string))
		return false;
    MCStringSplit(t_temp_string, MCSTR("\n"), nil, kMCStringOptionCompareExact, t_temp_array);
    MCValueAssign(redirects, t_temp_array);
    MCValueRelease(t_temp_string);
    MCValueRelease(t_temp_array);
	
	if (!ctxt.CopyOptElementAsFilepath(p_array, MCNAME("appicon"), false, t_temp_string))
		return false;
	MCValueAssign(app_icon, t_temp_string);
	MCValueRelease(t_temp_string);

	if (!ctxt.CopyOptElementAsFilepath(p_array, MCNAME("docicon"), false, t_temp_string))
		return false;
	MCValueAssign(doc_icon, t_temp_string);
	MCValueRelease(t_temp_string);
	
	if (!ctxt.CopyOptElementAsFilepath(p_array, MCNAME("manifest"), false, t_temp_string))
		return false;
	MCValueAssign(manifest, t_temp_string);
	MCValueRelease(t_temp_string);
	
	if (!ctxt.CopyOptElementAsFilepath(p_array, MCNAME("payload"), false, t_temp_string))
		return false;
	MCValueAssign(payload, t_temp_string);
	MCValueRelease(t_temp_string);
	
	if (!ctxt.CopyOptElementAsFilepath(p_array, MCNAME("spill"), false, t_temp_string))
		return false;
	MCValueAssign(spill, t_temp_string);
	MCValueRelease(t_temp_string);
	
	if (!ctxt.CopyElementAsFilepath(p_array, MCNAME("output"), false, t_temp_string))
		return false;
	MCValueAssign(output, t_temp_string);
	MCValueRelease(t_temp_string);
	
	if (!ctxt.CopyOptElementAsArray(p_array, MCNAME("version"), false, t_temp_array))
		return false;
	MCValueAssign(version_info, t_temp_array);
	MCValueRelease(t_temp_array);

    // AL-2015-02-10: [[ Standalone Inclusions ]] Fetch the resource mappings, if any.
    if (!ctxt.CopyOptElementAsString(p_array, MCNAME("library"), false, t_temp_string))
        return false;
    MCStringSplit(t_temp_string, MCSTR("\n"), nil, kMCStringOptionCompareExact, t_temp_array);
    MCValueAssign(library, t_temp_array);
    MCValueRelease(t_temp_string);
    MCValueRelease(t_temp_array);

    // SN-2015-02-16: [[ iOS Font mapping ]] Read the fontmappings options from the deploy parameters.
    if (!ctxt.CopyOptElementAsString(p_array, MCNAME("fontmappings"), false, t_temp_string))
        return false;
    MCStringSplit(t_temp_string, MCSTR("\n"), nil, kMCStringOptionCompareExact, t_temp_array);
    MCValueAssign(fontmappings, t_temp_array);
    MCValueRelease(t_temp_string);
    MCValueRelease(t_temp_array);

    // The 'min_os_version' is either a string or an array. If it is a string then
    // it encodes the version against the 'Unknown' architecture which is interpreted
    // by the deploy command to mean all architectures. Otherwise, the keys in the
    // array are assumed to be architecture names and each is pushed on the array.
    // If the 'min_os_version' is empty, then no change is brought to the binaries.
    // If multiple entries are present, then the 'unknown' mapping is used for any
    // architecture not explicitly specified. The current architecture strings that are
    // known are:
    //   i386, x86-64, armv6, armv7, armv7s, arm64, ppc, ppc64
    // The empty string is taken to be 'unknown'.
    if (!ctxt . CopyOptElementAsArray(p_array, MCNAME("min_os_version"), false, t_temp_array))
        return false;

    // SN-2015-02-04: [[ Merge-6.7.2 ]] If the array is empty, try to convert to a string.
    if (!MCArrayIsEmpty(t_temp_array))
    {
        MCDeployArrayApplyCallbackContext t_context;
        t_context . ctxt = &ctxt;
        t_context . params = this;

        bool t_success;
        t_success = MCArrayApply(t_temp_array, MCDeployGetArchitectures, (void*)&t_context);
        MCValueRelease(t_temp_array);

        if (!t_success)
            return false;
    }
    else
    {
        MCValueRelease(t_temp_array);
        if (!ctxt . CopyOptElementAsString(p_array, MCNAME("min_os_version"), false, t_temp_string))
            return false;
        
        if (!MCStringIsEmpty(t_temp_string))
            MCDeployPushMinOSVersion(this, kMCDeployArchitecture_Unknown, t_temp_string);
        
        MCValueRelease(t_temp_string);
    }
	
	if (!ctxt.CopyOptElementAsFilepathArray(p_array, MCNAME("modules"), false, t_temp_array))
		return false;
	MCValueAssign(modules, t_temp_array);
	MCValueRelease(t_temp_array);
    
    MCAutoStringRef t_architectures_string;
    if (!ctxt.CopyOptElementAsString(p_array, MCNAME("architectures"), false, &t_architectures_string))
        return false;
    if (!MCStringIsEmpty(*t_architectures_string))
    {
        // Split the string up into items
        MCAutoProperListRef t_architectures;
        if (!MCStringSplitByDelimiter(*t_architectures_string, MCSTR(","), kMCStringOptionCompareExact, &t_architectures))
            return false;
        
        // Process the architectures
        MCValueRef t_architecture;
        for (uindex_t i = 0; i < MCProperListGetLength(*t_architectures); i++)
        {
            // Fetch this item and make sure it is a string
            t_architecture = MCProperListFetchElementAtIndex(*t_architectures, i);
            if (t_architecture == nil || MCValueGetTypeCode(t_architecture) != kMCValueTypeCodeString)
                return false;
            
            // Map it to an architecture ID
            MCDeployArchitecture t_id;
            if (!MCDeployMapArchitectureString((MCStringRef)t_architecture, t_id))
                return false;
            
            // Append it to the list of desired architectures
            if (!architectures.Push(t_id))
                return false;
        }
    }
	
	// If the 'banner_class' string is present, then set the class override.
	MCAutoStringRef t_banner_class;
	if (!ctxt . CopyOptElementAsString(p_array, MCNAME("banner_class"), false, &t_banner_class))
		return false;
	if (MCStringIsEqualToCString(*t_banner_class,
								 "commercial",
								 kMCStringOptionCompareCaseless))
		banner_class = kMCLicenseClassEvaluation;
	else if (MCStringIsEqualToCString(*t_banner_class,
									  "professional",
									  kMCStringOptionCompareCaseless))
		banner_class = kMCLicenseClassProfessionalEvaluation;
	
    
    if (!ctxt.CopyOptElementAsString(p_array, MCNAME("uuid"), false, t_temp_string))
        return false;
    MCValueAssign(uuid, t_temp_string);
    MCValueRelease(t_temp_string);
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

static bool MCDeployWriteDefinePrologueSection(const MCDeployParameters& p_params, MCDeployCapsuleRef p_capsule)
{
	MCCapsulePrologueSection t_prologue;
	t_prologue . banner_timeout = p_params . banner_timeout;
	t_prologue . program_timeout = p_params . timeout;
	
	MCDeployByteSwapRecord(true, "ll", &t_prologue, sizeof(t_prologue));
	
	return MCDeployCapsuleDefine(p_capsule, kMCCapsuleSectionTypePrologue, &t_prologue, sizeof(t_prologue));
}

static bool MCDeployWriteDefineLicenseSection(const MCDeployParameters& p_params, MCDeployCapsuleRef p_capsule)
{
	bool t_success = true;
    
    IO_handle t_stream_handle;
    t_stream_handle = nullptr;
    if (t_success)
    {
        t_stream_handle = MCS_fakeopenwrite();
        t_success = t_stream_handle != nullptr;
    }
    
    if (t_success)
    {
        t_success = IO_write_uint1((uint1)MClicenseparameters . license_class, t_stream_handle) == IO_NORMAL;
    }
    
    if (t_success && MClicenseparameters . addons != nullptr)
    {
        t_success = IO_write_valueref_new(MClicenseparameters . addons, t_stream_handle) == IO_NORMAL;
    }
    
    //////////
    
    void *t_buffer;
    size_t t_length = 0;
    t_buffer = nullptr;
    if (t_success)
    {
        t_success = MCS_closetakingbuffer(t_stream_handle, t_buffer, t_length) == IO_NORMAL;
    }
    
    
    if (t_success)
    {
        t_success = MCDeployCapsuleDefine(p_capsule,
								 kMCCapsuleSectionTypeLicense,
								 t_buffer,
								 uint32_t(t_length));
    }
    
    if (t_buffer != nullptr)
    {
        free(t_buffer);
    }
    
    return t_success;
}

static bool MCDeployWriteDefineBannerSecion(const MCDeployParameters& p_params, MCDeployCapsuleRef p_capsule)
{
	return MCDeployCapsuleDefine(p_capsule, kMCCapsuleSectionTypeBanner, MCDataGetBytePtr(p_params . banner_stackfile), MCDataGetLength(p_params . banner_stackfile));
}

// This method generates the standalone specific capsule elements. This is
// just a Standalone Prologue section at the moment.
static bool MCDeployWriteCapsuleDefineStandaloneSections(const MCDeployParameters& p_params, MCDeployCapsuleRef p_capsule)
{
	bool t_success;
	t_success = true;
	
	// First emit the prologue.
	if (t_success)
		t_success = MCDeployWriteDefinePrologueSection(p_params, p_capsule);

    // Next emit the license info.
    if (t_success)
		t_success = MCDeployWriteDefineLicenseSection(p_params, p_capsule);
	
	// Next emit the banner.
	if (t_success &&
		!MCDataIsEmpty(p_params . banner_stackfile))
		t_success = MCDeployWriteDefineBannerSecion(p_params, p_capsule);
	
	return t_success;
}

static bool MCDeployCapsuleDefineFromStackFile(MCDeployCapsuleRef p_self, MCStringRef p_filename, MCDeployFileRef p_file, bool p_mainstack)
{
    MCAutoDataRef t_contents;
    if (!MCS_loadbinaryfile(p_filename, &t_contents))
        return false;
    
    IO_handle t_stream = nil;
    t_stream = MCS_fakeopen(MCDataGetBytePtr(*t_contents),MCDataGetLength(*t_contents));
    
    if (t_stream == nil)
        return false;
    
    bool t_script_only = MCdispatcher -> streamstackisscriptonly(t_stream);
    MCS_close(t_stream);
    
    MCCapsuleSectionType t_type;
    if (p_mainstack)
    {
        if (t_script_only)
        {
            t_type = kMCCapsuleSectionTypeScriptOnlyMainStack;
        }
        else
        {
            t_type = kMCCapsuleSectionTypeMainStack;
        }
    }
    else
    {
        if (t_script_only)
        {
            t_type = kMCCapsuleSectionTypeScriptOnlyAuxiliaryStack;
        }
        else
        {
            t_type = kMCCapsuleSectionTypeAuxiliaryStack;
        }
    }
    
    return MCDeployCapsuleDefineFromFile(p_self, t_type, p_file);
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
	if (t_success && !MCDeployFileOpen(p_params . stackfile, kMCOpenFileModeRead, t_stackfile))
		t_success = MCDeployThrow(kMCDeployErrorNoStackfile);

	// Open the spill file, if required
	MCDeployFileRef t_spill;
	t_spill = NULL;
	if (t_success && !MCStringIsEmpty(p_params . spill) && !MCDeployFileOpen(p_params . spill, kMCOpenFileModeCreate, t_spill))
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
		for(uindex_t i = 0; i < MCArrayGetCount(p_params.redirects) && t_success; i++)
		{
			MCValueRef t_val;
            /* UNCHECKED */ MCArrayFetchValueAtIndex(p_params.redirects, i + 1, t_val);
			t_success = MCDeployCapsuleDefineString(t_capsule, kMCCapsuleSectionTypeRedirect, (MCStringRef)t_val);
		}

    ////////
    
    // AL-2015-02-10: [[ Standalone Inclusions ]] Add the resource mappings, if any.
    if (t_success)
        for(uindex_t i = 0; i < MCArrayGetCount(p_params.library) && t_success; i++)
        {
            MCValueRef t_val;
            /* UNCHECKED */ MCArrayFetchValueAtIndex(p_params.library, i + 1, t_val);
            t_success = MCDeployCapsuleDefineString(t_capsule, kMCCapsuleSectionTypeLibrary, (MCStringRef)t_val);
        }
    
    ////////
    
    // Add all the modules before the stacks, this is so that widgets can resolve
    // themselves on load.
    MCAutoArray<MCDeployFileRef> t_module_files;
	if (t_success)
		t_success = t_module_files . New(MCArrayGetCount(p_params . modules));
    if (t_success)
        for(uindex_t i = 0; i < MCArrayGetCount(p_params.modules) && t_success; i++)
        {
            MCValueRef t_module_filename;
            /* UNCHECKED */ MCArrayFetchValueAtIndex(p_params .modules, i + 1, t_module_filename);
			if (t_success && !MCDeployFileOpen((MCStringRef)t_module_filename, kMCOpenFileModeRead, t_module_files[i]))
				t_success = MCDeployThrow(kMCDeployErrorNoModule);
            if (t_success)
                t_success = MCDeployCapsuleDefineFromFile(t_capsule, kMCCapsuleSectionTypeModule, t_module_files[i]);
        }
    
    ////////
    
			
    // Add any font mappings
    if (t_success)
        for(uint32_t i = 0; i < MCArrayGetCount(p_params.fontmappings) && t_success; i++)
        {
            MCValueRef t_val;
            /* UNCHECKED */ MCArrayFetchValueAtIndex(p_params.fontmappings, i + 1, t_val);
            t_success = MCDeployCapsuleDefineString(t_capsule, kMCCapsuleSectionTypeFontmap, (MCStringRef)t_val);
        }

	// Now we add the main stack
	if (t_success)
		t_success = MCDeployCapsuleDefineFromStackFile(t_capsule, p_params . stackfile, t_stackfile, true);

	// Now we add the auxillary stackfiles, if any
	MCAutoArray<MCDeployFileRef> t_aux_stackfiles;
	if (t_success)
        t_success = t_aux_stackfiles . New(MCArrayGetCount(p_params . auxiliary_stackfiles));
	if (t_success)
        for(uindex_t i = 0; i < MCArrayGetCount(p_params.auxiliary_stackfiles) && t_success; i++)
		{
			MCValueRef t_val;
            /* UNCHECKED */ MCArrayFetchValueAtIndex(p_params.auxiliary_stackfiles, i + 1, t_val);
			if (t_success && !MCDeployFileOpen((MCStringRef)t_val, kMCOpenFileModeRead, t_aux_stackfiles[i]))
				t_success = MCDeployThrow(kMCDeployErrorNoAuxStackfile);
			if (t_success)
                t_success = MCDeployCapsuleDefineFromStackFile(t_capsule, (MCStringRef)t_val, t_aux_stackfiles[i], false);
		}

	// Now add the externals, if any
	if (t_success)
		for(uindex_t i = 0; i < MCArrayGetCount(p_params.externals) && t_success; i++)
		{
			MCValueRef t_val;
            /* UNCHECKED */ MCArrayFetchValueAtIndex(p_params.externals, i + 1, t_val);
			t_success = MCDeployCapsuleDefineString(t_capsule, kMCCapsuleSectionTypeExternal, (MCStringRef)t_val);
		}
			
	// Now add the startup script, if any.
	if (t_success && (!MCStringIsEmpty(p_params . startup_script)))
		t_success = MCDeployCapsuleDefineString(t_capsule, kMCCapsuleSectionTypeStartupScript, p_params . startup_script);
    
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
	for(uindex_t i = 0; i < t_aux_stackfiles . Size(); i++)
		MCDeployFileClose(t_aux_stackfiles[i]);
    for(uindex_t i = 0; i < t_module_files . Size(); i++)
        MCDeployFileClose(t_module_files[i]);
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
		if (!MCStringIsEmpty(p_params . spill))
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
	if (t_success && !MCDeployFileOpen(p_params . payload, kMCOpenFileModeRead, t_payload))
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
    m_platform = PLATFORM_NONE;
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
		if (sp . token_is_cstring("windows"))
			m_platform = PLATFORM_WINDOWS;
		else if (sp . token_is_cstring("linux"))
			m_platform = PLATFORM_LINUX;
		else if (sp . token_is_cstring("macosx"))
			m_platform = PLATFORM_MACOSX;
		else if (sp . token_is_cstring("ios"))
			m_platform = PLATFORM_IOS;
		else if (sp . token_is_cstring("android"))
			m_platform = PLATFORM_ANDROID;
		else if (sp . token_is_cstring("winmobile"))
			m_platform = PLATFORM_WINMOBILE;
		else if (sp . token_is_cstring("linuxmobile"))
			m_platform = PLATFORM_LINUXMOBILE;
		else if (sp . token_is_cstring("iosembedded"))
			m_platform = PLATFORM_IOS_EMBEDDED;
		else if (sp . token_is_cstring("androidembedded"))
			m_platform = PLATFORM_ANDROID_EMBEDDED;
		else if (sp . token_is_cstring("emscripten"))
			m_platform = PLATFORM_EMSCRIPTEN;
		else
			return PS_ERROR;
	}
	else
		return PS_ERROR;

	return sp . parseexp(False, True, &m_params);
}

void MCIdeDeploy::exec_ctxt(MCExecContext& ctxt)
{
    bool t_soft_error;
    t_soft_error = false;
    bool t_has_error;
    t_has_error = false;
    
    // Clear the result as we return an error there
	ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_array;
    if (!ctxt . EvalExprAsArrayRef(m_params, EE_UNDEFINED, &t_array))
        return;

    MCDeployParameters t_params;
    t_has_error = !t_params.InitWithArray(ctxt, *t_array);
	
	// If platform is iOS and we are not Mac then error
#ifndef _MACOSX
	if (!t_has_error && (m_platform == PLATFORM_IOS || m_platform == PLATFORM_IOS_EMBEDDED))
	{
		ctxt . SetTheResultToCString("ios deployment not supported on this platform");
		t_soft_error = true;
        t_has_error = true;
	}
#endif

	// If the banner_class field is set and we are not a trial license, we
	// override the license class with that specified.
	MCLicenseClass t_license_class = kMCLicenseClassNone;
	if (MClicenseparameters . license_class == kMCLicenseClassCommercial)
	{
		// If we have a commercial license, then we only allow a commercial
		// evaluation.
		if (t_params . banner_class == kMCLicenseClassEvaluation)
			t_license_class = kMCLicenseClassEvaluation;
	}
	else if (MClicenseparameters . license_class == kMCLicenseClassProfessional)
	{
		// If we are a professional license, then we allow any kind of
		// trial.
		t_license_class = t_params . banner_class;
	}
	
	if (t_license_class == kMCLicenseClassNone)
		t_license_class = MClicenseparameters . license_class;
	
	t_params . banner_class = t_license_class;
	
	// Now check to see if we should build a trial - this if the license class is a
	// trail, or the banner_class override is specified and the chosen option is
	// compatible with the license class.
	bool t_is_trial;
    t_is_trial = false;
	if (t_license_class == kMCLicenseClassEvaluation ||
		t_license_class == kMCLicenseClassProfessionalEvaluation)
		t_is_trial = true;
	
	// Now, if we are not licensed for a target, then its an error. If we are in trial
	// mode, however, all platforms are licensed (apart from embedded) they just will
	// timeout.
	bool t_is_licensed;
	t_is_licensed = false;
	
    if (MCnoui && MClicenseparameters . license_class == kMCLicenseClassCommunity)
        t_is_licensed = true;
	else if (t_is_trial &&
			 m_platform != PLATFORM_IOS_EMBEDDED &&
			 m_platform != PLATFORM_ANDROID_EMBEDDED)
		t_is_licensed = true;
	else if (m_platform == PLATFORM_WINDOWS)
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
	else if (m_platform == PLATFORM_EMSCRIPTEN)
		t_is_licensed = (MClicenseparameters . deploy_targets & kMCLicenseDeployToHTML5) != 0;

	if (!t_is_licensed)
	{
		ctxt . SetTheResultToCString("not licensed to deploy to target platform");
		t_soft_error = true;
		t_has_error = true;
	}
	
	if (t_is_trial &&
		m_platform == PLATFORM_EMSCRIPTEN)
	{
		ctxt . SetTheResultToCString("trial of html5 is not possible");
		t_soft_error = true;
		t_has_error = true;
	}
	
	uint32_t t_platform = PLATFORM_NONE;
	switch(m_platform)
	{
		case PLATFORM_MACOSX:
			t_platform = kMCLicenseDeployToMacOSX;
			break;
		case PLATFORM_WINDOWS:
			t_platform = kMCLicenseDeployToWindows;
			break;
		case PLATFORM_LINUX:
			t_platform = kMCLicenseDeployToLinux;
			break;
		case PLATFORM_IOS:
			t_platform = kMCLicenseDeployToIOS;
			break;
		case PLATFORM_ANDROID:
			t_platform = kMCLicenseDeployToAndroid;
			break;
		case PLATFORM_IOS_EMBEDDED:
			t_platform = kMCLicenseDeployToIOSEmbedded;
			break;
		case PLATFORM_ANDROID_EMBEDDED:
			t_platform = kMCLicenseDeployToAndroidEmbedded;
			break;
		case PLATFORM_EMSCRIPTEN:
			t_platform = kMCLicenseDeployToHTML5;
			break;
	}
	
	if (!t_has_error)
	{
		// If this is a trial then set the timeout.
		if (t_is_trial)
		{
			if (m_platform != PLATFORM_IOS &&
				m_platform != PLATFORM_ANDROID &&
				m_platform != PLATFORM_EMSCRIPTEN)
				t_params . timeout = 5 * 60;
			else
				t_params . timeout = 1 * 60;
			
			t_params . banner_timeout = 10;
		}
		
		// Pass the deploy parameters through any stack security related steps.
		if (!MCStackSecurityPreDeploy(t_platform, t_params))
		{
			t_soft_error = true;
			t_has_error = true;
		}
	}
	
	if (!t_has_error)
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
		else if (m_platform == PLATFORM_EMSCRIPTEN)
			MCDeployToEmscripten(t_params);

		MCDeployError t_error;
		t_error = MCDeployCatch();
		if (t_error != kMCDeployErrorNone)
            ctxt . SetTheResultToCString(MCDeployErrorToString(t_error));
        else
            ctxt . SetTheResultToEmpty();
    }
    
    if (t_has_error && !t_soft_error)
        ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

MCIdeSign::MCIdeSign(void)
{
	m_params = NULL;
    m_platform = PLATFORM_NONE;
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
		if (sp . token_is_cstring("windows"))
			m_platform = PLATFORM_WINDOWS;
		else if (sp . token_is_cstring("linux"))
			m_platform = PLATFORM_LINUX;
		else if (sp . token_is_cstring("macosx"))
			m_platform = PLATFORM_MACOSX;
		else
			return PS_ERROR;
	}
	else
		return PS_ERROR;

	return sp . parseexp(False, True, &m_params);
}

void MCIdeSign::exec_ctxt(MCExecContext &ctxt)
{
	// Clear the result as we return an error there
	ctxt . SetTheResultToEmpty();

    MCAutoArrayRef t_array;
    if (!ctxt . EvalExprAsArrayRef(m_params, EE_UNDEFINED, &t_array))
        return;
    MCDeploySignParameters t_params;

	if (!ctxt . HasError())
		if (!ctxt.CopyOptElementAsString(*t_array, MCNAME("passphrase"), false, t_params.passphrase))
			ctxt . Throw();
	if (!ctxt . HasError())
		if (!ctxt.CopyOptElementAsFilepath(*t_array, MCNAME("certificate"), false, t_params.certificate))
			ctxt . Throw();
	if (!ctxt . HasError())
		if (!ctxt.CopyOptElementAsFilepath(*t_array, MCNAME("privatekey"), false, t_params.privatekey))
			ctxt . Throw();
	if (!ctxt . HasError())
		if (!ctxt.CopyOptElementAsFilepath(*t_array, MCNAME("certstore"), false, t_params.certstore))
			ctxt . Throw();
	if (!ctxt . HasError())
		if (!ctxt.CopyOptElementAsString(*t_array, MCNAME("timestamper"), false, t_params.timestamper))
			ctxt . Throw();
	if (!ctxt . HasError())
		if (!ctxt.CopyOptElementAsString(*t_array, MCNAME("description"), false, t_params.description))
			ctxt . Throw();
	if (!ctxt . HasError())
		if (!ctxt.CopyOptElementAsString(*t_array, MCNAME("url"), false, t_params.url))
			ctxt . Throw();
	if (!ctxt . HasError())
		if (!ctxt.CopyElementAsFilepath(*t_array, MCNAME("input"), false, t_params.input))
			ctxt . Throw();
	if (!ctxt . HasError())
		if (!ctxt.CopyElementAsFilepath(*t_array, MCNAME("output"), false, t_params.output))
			ctxt . Throw();
	
	if (!ctxt . HasError())
		if (!MCValueIsEmpty(t_params . certstore) && (!MCValueIsEmpty(t_params . certificate) || !MCValueIsEmpty(t_params . privatekey)))
			ctxt . Throw();

	if (!ctxt . HasError())
		if (MCValueIsEmpty(t_params . certstore) && (MCValueIsEmpty(t_params . certificate) || MCValueIsEmpty(t_params . privatekey)))
			ctxt . Throw();

	bool t_can_sign;
	t_can_sign = true;
	if (!ctxt . HasError() && !InitSSLCrypt())
	{
		ctxt . SetTheResultToCString("could not initialize SSL");
		t_can_sign = false;
	}

	if (t_can_sign && !ctxt . HasError())
	{
        MCExecContext *t_old_ec;
        t_old_ec = MCECptr;
        
        MCECptr = &ctxt;
        
		if (m_platform == PLATFORM_WINDOWS)
			MCDeploySignWindows(t_params);
        
        MCECptr = t_old_ec;

		MCDeployError t_error;
		t_error = MCDeployCatch();
		if (t_error != kMCDeployErrorNone)
			ctxt . SetTheResultToCString(MCDeployErrorToString(t_error));
	}

	return;
}

////////////////////////////////////////////////////////////////////////////////

MCIdeDiet::MCIdeDiet(void)
{
	m_params = NULL;
    m_platform = PLATFORM_NONE;
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
		if (sp . token_is_cstring("windows"))
			m_platform = PLATFORM_WINDOWS;
		else if (sp . token_is_cstring("linux"))
			m_platform = PLATFORM_LINUX;
		else if (sp . token_is_cstring("macosx"))
			m_platform = PLATFORM_MACOSX;
		else
			return PS_ERROR;
	}
	else
		return PS_ERROR;

	return sp . parseexp(False, True, &m_params);
}

void MCIdeDiet::exec_ctxt(MCExecContext& ctxt)
{
	// Clear the result as we return an error there
	ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_array;
    if (!ctxt . EvalExprAsArrayRef(m_params, EE_UNDEFINED, &t_array))
        return;

	MCDeployDietParameters t_params;

	if (!ctxt . HasError())
		if (!ctxt.CopyElementAsFilepath(*t_array, MCNAME("input"), false, t_params.input))
			ctxt . Throw();
	if (!ctxt . HasError())
		if (!ctxt.CopyElementAsFilepath(*t_array, MCNAME("output"), false, t_params.output))
            ctxt . Throw();	
	if (!ctxt . HasError())
		if (!ctxt.CopyOptElementAsBoolean(*t_array, MCNAME("keep_x86"), false, t_params.keep_x86))
			ctxt . Throw();
	if (!ctxt . HasError())
		if (!ctxt.CopyOptElementAsBoolean(*t_array, MCNAME("keep_x86_64"), false, t_params.keep_x86_64))
			ctxt . Throw();
	if (!ctxt . HasError())
		if (!ctxt.CopyOptElementAsBoolean(*t_array, MCNAME("keep_ppc"), false, t_params.keep_ppc))
			ctxt . Throw();
	if (!ctxt . HasError())
		if (!ctxt.CopyOptElementAsBoolean(*t_array, MCNAME("keep_ppc64"), false, t_params.keep_ppc64))
			ctxt . Throw();
	if (!ctxt . HasError())
		if (!ctxt.CopyOptElementAsBoolean(*t_array, MCNAME("keep_arm"), false, t_params.keep_arm))
			ctxt . Throw();

	if (!ctxt . HasError())
		if (!ctxt.CopyOptElementAsBoolean(*t_array, MCNAME("keep_debug_symbols"), false, t_params.keep_debug_symbols))
			ctxt . Throw();

	if (!ctxt . HasError())
	{
		if (m_platform == PLATFORM_MACOSX)
			MCDeployDietMacOSX(t_params);

		MCDeployError t_error;
		t_error = MCDeployCatch();
		if (t_error != kMCDeployErrorNone)
			ctxt . SetTheResultToCString(MCDeployErrorToString(t_error));
	}
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

void MCIdeDmgDump::exec_ctxt(MCExecContext &ctxt)
{
	// Clear the result as we return an error there
	ctxt . SetTheResultToEmpty();

    MCAutoStringRef t_string;
    if (!ctxt . EvalExprAsStringRef(m_filename, EE_UNDEFINED, &t_string))
        return;
	
	if (!ctxt . HasError())
	{
        MCAutoPointer<char> temp;
        if (!MCStringConvertToCString(*t_string, &temp))
        {
            ctxt . Throw();
            return;
        }
		FILE *t_output;
		t_output = fopen("C:\\Users\\Mark\\Desktop\\dmg.txt", "w");
        if (t_output != nil)
        {
            MCDeployDmgDump(*temp, stdfile_log, t_output);
            fclose(t_output);
        }
    }
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

void MCIdeDmgBuild::exec_ctxt(MCExecContext& ctxt)
{
    // Clear the result as we return an error there
	ctxt . SetTheResultToEmpty();

	/////////

    MCAutoArrayRef t_array;
    if (!ctxt . EvalExprAsArrayRef(m_items, EE_UNDEFINED, &t_array))
        return;
    
    if (!MCArrayIsSequence(*t_array))
    {
        ctxt . Throw();
        return;
    }
	
	MCAutoArray<MCDeployDmgItem> t_items;
	if (!ctxt . HasError())
	{
        if (!t_items . New(MCArrayGetCount(*t_array)))
            ctxt . Throw();
	}

	if (!ctxt . HasError())
		for(uindex_t i = 0; i < t_items . Size() && !ctxt . HasError(); i++)
		{
			MCValueRef t_val = nil;
			if (!MCArrayFetchValueAtIndex(*t_array, i + 1, t_val))
				ctxt . Throw();
			
			MCAutoArrayRef t_item_array;
			if (!ctxt . HasError())
				if (!ctxt.ConvertToArray(t_val, &t_item_array))
					ctxt . Throw();


			if (!ctxt . HasError())
			{
				MCAutoStringRef t_type;
				if (ctxt.CopyOptElementAsString(*t_item_array, MCNAME("type"), false, &t_type))
				{
					if (MCStringIsEqualToCString(*t_type, "folder", kMCCompareCaseless))
						t_items[i].is_folder = true;
					else if (MCStringIsEqualToCString(*t_type, "file", kMCCompareCaseless))
						t_items[i].is_folder = false;
					else
						ctxt . Throw();
				}
				else
					ctxt . Throw();
			}

			if (!ctxt . HasError())
				if (!ctxt.CopyElementAsUnsignedInteger(*t_item_array, MCNAME("parent"), false, t_items[i].parent))
					ctxt . Throw();
			if (!ctxt . HasError())
				if (!ctxt.CopyElementAsString(*t_item_array, MCNAME("name"), false, t_items[i].name))
					ctxt . Throw();

			if (!ctxt . HasError())
			{
				ctxt.CopyElementAsUnsignedInteger(*t_item_array, MCNAME("owner_id"), false, t_items[i].owner_id);
				ctxt.CopyElementAsUnsignedInteger(*t_item_array, MCNAME("group_id"), false, t_items[i].group_id);
				ctxt.CopyElementAsUnsignedInteger(*t_item_array, MCNAME("file_mode"), false, t_items[i].group_id);
				
				ctxt.CopyElementAsUnsignedInteger(*t_item_array, MCNAME("create_date"), false, t_items[i].create_date);
				ctxt.CopyElementAsUnsignedInteger(*t_item_array, MCNAME("content_mod_date"), false, t_items[i].content_mod_date);
				ctxt.CopyElementAsUnsignedInteger(*t_item_array, MCNAME("attribute_mod_date"), false, t_items[i].attribute_mod_date);
				ctxt.CopyElementAsUnsignedInteger(*t_item_array, MCNAME("access_date"), false, t_items[i].access_date);
				ctxt.CopyElementAsUnsignedInteger(*t_item_array, MCNAME("backup_date"), false, t_items[i].backup_date);
			}
		}

	/////////

    // SN-2015-06-19: [[ CID 100294 ]] Check the return value.
    MCAutoStringRef t_string;
	if (!ctxt . HasError()
            && ctxt . EvalExprAsStringRef(m_filename, EE_UNDEFINED, &t_string))
	{
        MCAutoPointer<char> temp;
        if (!MCStringConvertToCString(*t_string, &temp))
        {
            ctxt . Throw();
            return;
        }
		MCDeployDmgParameters t_params;
		t_params . items = t_items . Ptr();
		t_params . item_count = t_items . Size();
		t_params . output = *temp;
		if (!MCDeployDmgBuild(t_params))
			ctxt . Throw();
	}

	////////

	return;
}

////////////////////////////////////////////////////////////////////////////////

MCIdeExtract::MCIdeExtract(void)
{
	m_filename = nil;
	m_segment_name = nil;
	m_section_name = nil;
}

MCIdeExtract::~MCIdeExtract(void)
{
	delete m_filename;
	delete m_segment_name;
	delete m_section_name;
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
	
	return PS_NORMAL;
}

void MCIdeExtract::exec_ctxt(MCExecContext& ctxt)
{
	
	MCAutoStringRef t_segment;
    if (!ctxt . EvalExprAsStringRef(m_segment_name, EE_IDE_EXTRACT_BADSEGMENT, &t_segment))
        return;
	
	MCAutoStringRef t_section;
    if (!ctxt . EvalExprAsStringRef(m_section_name, EE_IDE_EXTRACT_BADSECTION, &t_section))
        return;
	
	MCAutoStringRef t_filename;
    if (!ctxt . EvalExprAsStringRef(m_filename, EE_IDE_EXTRACT_BADFILENAME, &t_filename))
        return;

	void *t_data;
	uint32_t t_data_size;
    Exec_stat t_stat;
    t_stat = MCDeployExtractMacOSX(*t_filename, *t_segment, *t_section, t_data, t_data_size);
    if (t_stat == ES_NORMAL)
    {
        MCAutoStringRef t_string;
        /* UNCHECKED */ MCStringCreateWithNativeChars((const char_t*)t_data, t_data_size, &t_string);
        ctxt . SetItToValue(*t_string);
    }
    else
        ctxt . SetItToEmpty();
}

////////////////////////////////////////////////////////////////////////////////
