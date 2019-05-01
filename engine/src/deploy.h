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

#ifndef __MC_DEPLOY__
#define __MC_DEPLOY__

#include "mcio.h"
#include "license.h"

////////////////////////////////////////////////////////////////////////////////

enum MCDeployArchitecture
{
    kMCDeployArchitecture_Unknown,
    kMCDeployArchitecture_I386,
    kMCDeployArchitecture_X86_64,
    kMCDeployArchitecture_ARMV6,
    kMCDeployArchitecture_ARMV7,
    kMCDeployArchitecture_ARMV7S,
    kMCDeployArchitecture_ARM64,
    kMCDeployArchitecture_PPC,
    kMCDeployArchitecture_PPC64,
};

struct MCDeployMinOSVersion
{
    // The architecture this version applies to.
    MCDeployArchitecture architecture;
    // The version word encoded as nibbles XXXX.YY.ZZ for X.Y.Z.
    uint32_t version;
};

struct MCDeployParameters
{
	// The path to the engine binaries to use. On Windows and Linux this should
	// be just 'engine'. On OS X, set one other or both to form a single or
	// universal architecture standalone.
	MCStringRef engine;
	MCStringRef engine_ppc;
	MCStringRef engine_x86;

	// When building on Windows, this might contain an array with the versioninfo
	// fields.
	MCArrayRef version_info;

    // When building for Mac/iOS, you can specify a min os version per arch
    // slice.
    MCDeployMinOSVersion *min_os_versions;
    uindex_t min_os_version_count;
    
    // The root stackfile to be included in the standalone.
	MCStringRef stackfile;
	
    // The array of auxiliary stackfiles to be included in the standalone.
    MCArrayRef auxiliary_stackfiles;

	// The array of externals to be loaded on startup by the standalone.
	MCArrayRef externals;

	// The script to be executed on startup by the standalone.
	MCStringRef startup_script;
	
	// If true, then the standalone will have an implicit timeout
	uint32_t timeout;
	
    // The list of redirection mappings
	MCArrayRef redirects;

    // The list of font mappings
    MCArrayRef fontmappings;

    // AL-2015-02-10: [[ Standalone Inclusions ]] The list of resource mappings.
    MCArrayRef library;
    
	// On Windows, the icon files to be inserted into the resource directory.
	MCStringRef app_icon;
	MCStringRef doc_icon;

	// On Windows, the path to the manifest file to be included in the executable
	// (optional).
	MCStringRef manifest;

	// When building an installer, the path to the payload to be inserted
	MCStringRef payload;

	// If this is non-nil, it contains a file to be used to 'spill' data above
	// 4K into. It is designed for use on OS X to reduce standalone file size.
	MCStringRef spill;

	// The output path for the new executable.
	MCStringRef output;
    
    // The list of modules to include.
    MCArrayRef modules;
    
    // List of architectures to retain when building universal binaries
    MCAutoArray<MCDeployArchitecture> architectures;
	
	// This can be set to commercial or professional trial. In that
	// case, the standalone will be built in that mode.
	MCLicenseClass banner_class;
	
	// The timeout for the banner that's displayed before startup.
	uint32_t banner_timeout;
	
	// The data for the banner stackfile.
	MCDataRef banner_stackfile;
	
    // When building for Mac/iOS, there is a UUID field which can be set.
    MCStringRef uuid;
	
	MCDeployParameters()
	{
		engine			= MCValueRetain(kMCEmptyString);
		engine_x86		= MCValueRetain(kMCEmptyString);
		engine_ppc		= MCValueRetain(kMCEmptyString);
		version_info	= MCValueRetain(kMCEmptyArray);
		stackfile		= MCValueRetain(kMCEmptyString);
        auxiliary_stackfiles = MCValueRetain(kMCEmptyArray);
		externals		= MCValueRetain(kMCEmptyArray);
		startup_script	= MCValueRetain(kMCEmptyString);
		timeout			= 0;
		redirects		= MCValueRetain(kMCEmptyArray);
		app_icon		= MCValueRetain(kMCEmptyString);
		doc_icon		= MCValueRetain(kMCEmptyString);
		manifest		= MCValueRetain(kMCEmptyString);
		payload			= MCValueRetain(kMCEmptyString);
		spill			= MCValueRetain(kMCEmptyString);
        output			= MCValueRetain(kMCEmptyString);
        modules         = MCValueRetain(kMCEmptyArray);
        library         = MCValueRetain(kMCEmptyArray);
        
        // SN-2015-04-23: [[ Merge-6.7.5-rc-1 ]] Initialise fontmappings array
        fontmappings    = MCValueRetain(kMCEmptyArray);
        
        // SN-2015-02-04: [[ Merge-6.7.2 ]] Init the versions pointer / count
        min_os_versions = nil;
        min_os_version_count = 0;
		
		banner_timeout = 0;
		banner_stackfile = MCValueRetain(kMCEmptyData);
        banner_class = kMCLicenseClassNone;
        
        uuid = MCValueRetain(kMCEmptyString);
	}
	
	~MCDeployParameters()
    {
        MCValueRelease(engine);
        MCValueRelease(engine_x86);
        MCValueRelease(engine_ppc);
        MCValueRelease(version_info);
        MCValueRelease(stackfile);
        MCValueRelease(auxiliary_stackfiles);
        MCValueRelease(externals);
        MCValueRelease(startup_script);
        MCValueRelease(redirects);
        MCValueRelease(app_icon);
        MCValueRelease(doc_icon);
        MCValueRelease(manifest);
        MCValueRelease(payload);
        MCValueRelease(spill);
        MCValueRelease(output);
        MCValueRelease(modules);
        MCValueRelease(library);
        MCMemoryDeleteArray(min_os_versions);
		MCValueRelease(banner_stackfile);
        MCValueRelease(uuid);
	}
	
	// Creates using an array of parameters
	bool InitWithArray(MCExecContext &ctxt, MCArrayRef p_array);
};

Exec_stat MCDeployToWindows(const MCDeployParameters& p_params);
Exec_stat MCDeployToLinux(const MCDeployParameters& p_params);
Exec_stat MCDeployToMacOSX(const MCDeployParameters& p_params);
Exec_stat MCDeployToIOS(const MCDeployParameters& p_params, bool embedded);
Exec_stat MCDeployToAndroid(const MCDeployParameters& p_params);
Exec_stat MCDeployToEmscripten(const MCDeployParameters& p_params);

////////////////////////////////////////////////////////////////////////////////

struct MCDeploySignParameters
{
	// The passphrase needed to decrypt any of the other files (if needed).
	MCStringRef passphrase;

	// The path to the software publishing certificate stored in a PKCS#7
	// SignedInfo structure.
	MCStringRef certificate;

	// The path to the private key file stored in a PKCS#8 PrivateKey
	// structure or the Microsoft PVK file format.
	MCStringRef privatekey;

	// The path to either a PKCS#12 certificate/privatekey store. This is
	// only needed if the previous two fields are nil.
	MCStringRef certstore;

	// The url to use for timestamping - if needed
	MCStringRef timestamper;

	// The UTF-8 string describing the pprogram name/description
	MCStringRef description;
	// The UTF-8 string contining the URL of the company
	MCStringRef url;

	// The path to the executable to sign
	MCStringRef input;

	// The outpath for the resulting signed executable
	MCStringRef output;
	
	MCDeploySignParameters()
	{
		passphrase	= nil;
		certificate	= nil;
		privatekey	= nil;
		certstore	= nil;
		timestamper	= nil;
		description	= nil;
		url			= nil;
		input		= nil;
		output		= nil;
	}
	
	~MCDeploySignParameters()
	{
		if (passphrase != nil)
			MCValueRelease(passphrase);
		if (certificate != nil)
			MCValueRelease(certificate);
		if (privatekey != nil)
			MCValueRelease(privatekey);
		if (certstore != nil)
			MCValueRelease(certstore);
		if (timestamper != nil)
			MCValueRelease(timestamper);
		if (description != nil)
			MCValueRelease(description);
		if (url != nil)
			MCValueRelease(url);
		if (input != nil)
			MCValueRelease(input);
		if (output != nil)
			MCValueRelease(output);
	}
};

bool MCDeploySignWindows(const MCDeploySignParameters& p_params);

////////////////////////////////////////////////////////////////////////////////

struct MCDeployDietParameters
{
	// The input engine to process
	MCStringRef input;

	// Which architectures to keep (if present)
	MCBooleanRef keep_x86;
	MCBooleanRef keep_x86_64;
	MCBooleanRef keep_ppc;
	MCBooleanRef keep_ppc64;
	MCBooleanRef keep_arm;

	// Whether to keep symbols
	MCBooleanRef keep_debug_symbols;

	// Where to put the output
	MCStringRef output;
	
	MCDeployDietParameters()
	{
		input		= nil;
		output		= nil;
		keep_x86	= nil;
		keep_x86_64	= nil;
		keep_ppc	= nil;
		keep_ppc64	= nil;
		keep_arm	= nil;
		keep_debug_symbols = nil;
	}
	
	~MCDeployDietParameters()
	{
		if (input != nil)
			MCValueRelease(input);
		if (output != nil)
			MCValueRelease(output);
		
		if (keep_x86 != nil)
			MCValueRelease(keep_x86);
		if (keep_x86_64 != nil)
			MCValueRelease(keep_x86_64);
		if (keep_ppc != nil)
			MCValueRelease(keep_ppc);
		if (keep_ppc64 != nil)
			MCValueRelease(keep_ppc64);
		if (keep_arm != nil)
			MCValueRelease(keep_arm);
		if (keep_debug_symbols != nil)
			MCValueRelease(keep_debug_symbols);
	}
};

Exec_stat MCDeployDietMacOSX(const MCDeployDietParameters& p_params);

////////////////////////////////////////////////////////////////////////////////

struct MCDeployDmgItem
{
	uinteger_t parent;
	MCStringRef name;
	bool is_folder;

	// BSD permission info
	uinteger_t owner_id;
	uinteger_t group_id;
	uinteger_t file_mode;

	// If any of these are 0 they take on 'default' values.
	uinteger_t create_date;
	uinteger_t content_mod_date;
	uinteger_t attribute_mod_date;
	uinteger_t access_date;
	uinteger_t backup_date;

	union
	{
		struct
		{
			char *data_fork;

			uint32_t file_type;
			uint32_t file_creator;

			int16_t location_x;
			int16_t location_y;
		} file;

		struct
		{
			int16_t window_x;
			int16_t window_y;
			int16_t window_width;
			int16_t window_height;

			int16_t location_x;
			int16_t location_y;
		} folder;
	};
	
	MCDeployDmgItem()
	{
		name = nil;
	}
	
	~MCDeployDmgItem()
	{
		if (name != nil)
			MCValueRelease(name);
	}
};

struct MCDeployDmgParameters
{
	// The items to include in the dmg
	MCDeployDmgItem *items;
	uint32_t item_count;

	// Where to put the output
	char *output;
};

Exec_stat MCDeployDmgBuild(MCDeployDmgParameters& params);

bool MCDeployDmgDump(const char *p_dmg_file, void (*p_log)(void *, const char *, ...), void *p_context);

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCDeployExtractMacOSX(MCStringRef p_filename, MCStringRef p_segment, MCStringRef p_section, void*& r_data, uint32_t& r_data_size);

////////////////////////////////////////////////////////////////////////////////

enum MCDeployError
{
	kMCDeployErrorNone,
	kMCDeployErrorNoMemory,
	kMCDeployErrorNoEngine,
	kMCDeployErrorNoStackfile,
	kMCDeployErrorNoAuxStackfile,
    kMCDeployErrorNoModule,
	kMCDeployErrorNoOutput,
	kMCDeployErrorNoSpill,
	kMCDeployErrorNoPayload,
	kMCDeployErrorBadFile,
	kMCDeployErrorBadRead,
	kMCDeployErrorBadWrite,
	kMCDeployErrorBadCompress,

	kMCDeployErrorWindowsNoDOSHeader,
	kMCDeployErrorWindowsBadDOSSignature,
	kMCDeployErrorWindowsBadDOSHeader,
	kMCDeployErrorWindowsNoNTHeader,
	kMCDeployErrorWindowsBadNTSignature,
	kMCDeployErrorWindowsBadSectionHeaderOffset,
	kMCDeployErrorWindowsNoSectionHeaders,
	kMCDeployErrorWindowsMissingSections,
	kMCDeployErrorWindowsNoResourceSection,
	kMCDeployErrorWindowsNoProjectSection,
	kMCDeployErrorWindowsNoPayloadSection,
	kMCDeployErrorWindowsBadAppIcon,
	kMCDeployErrorWindowsBadDocIcon,
	kMCDeployErrorWindowsBadManifest,
	kMCDeployErrorWindowsBadSecuritySection,

	kMCDeployErrorLinuxNoHeader,
	kMCDeployErrorLinuxBadHeaderMagic,
	kMCDeployErrorLinuxBadHeaderType,
	kMCDeployErrorLinuxBadImage,
	kMCDeployErrorLinuxBadSectionSize,
	kMCDeployErrorLinuxBadSectionTable,
	kMCDeployErrorLinuxBadSegmentSize,
	kMCDeployErrorLinuxBadProgramTable,
	kMCDeployErrorLinuxBadStringIndex,
	kMCDeployErrorLinuxBadString,
	kMCDeployErrorLinuxNoProjectSection,
	kMCDeployErrorLinuxNoPayloadSection,
	kMCDeployErrorLinuxBadSectionOrder,
	kMCDeployErrorLinuxNoProjectSegment,
	kMCDeployErrorLinuxPayloadNotInProjectSegment,

	kMCDeployErrorMacOSXNoHeader,
	kMCDeployErrorMacOSXBadHeader,
	kMCDeployErrorMacOSXBadCommand,
	kMCDeployErrorMacOSXNoLinkEditSegment,
	kMCDeployErrorMacOSXNoProjectSegment,
	kMCDeployErrorMacOSXNoPayloadSegment,
	kMCDeployErrorMacOSXBadSegmentOrder,
	kMCDeployErrorMacOSXUnknownLoadCommand,
	kMCDeployErrorMacOSXBadCpuType,
	kMCDeployErrorMacOSXBadTarget,

	/* An error occurred while creating the startup stack */
	kMCDeployErrorEmscriptenBadStack,
	
	/* An error occurred with the pre-deploy step */
	kMCDeployErrorTrialBannerError,
    
    /* The uuid field was invalid */
    kMCDeployErrorInvalidUuid,

	// SIGN ERRORS

	kMCDeployErrorNoCertificate,
	kMCDeployErrorBadCertificate,
	kMCDeployErrorEmptyCertificate,
	kMCDeployErrorNoPrivateKey,
	kMCDeployErrorBadPrivateKey,

	// The privatekey does not match the cert
	kMCDeployErrorCertMismatch,

	// A password is needed, but was not given
	kMCDeployErrorNoPassword,
	// The password did not match
	kMCDeployErrorBadPassword,

	// An error occurred while building the signature
	kMCDeployErrorBadSignature,

	// An error occurred while trying to convert a string
	kMCDeployErrorBadString,

	// An error occurred while trying to compute the hash
	kMCDeployErrorBadHash,

	// An error occurred while trying to fetch a timestamp
	kMCDeployErrorTimestampFailed,

	// An error occurred decoding the timestamp response
	kMCDeployErrorBadTimestamp,


	// DIET ERRORS

	kMCDeployErrorNoArchs,
	kMCDeployErrorCannotDiet,
};

bool MCDeployThrow(MCDeployError status);
MCDeployError MCDeployCatch(void);

const char *MCDeployErrorToString(MCDeployError p_error);

////////////////////////////////////////////////////////////////////////////////

// The MCDeployFileRef is an opaque type with a set of methods allowing easy
// file manipulation designed for the kinds of operations needed by the deploy
// methods. It should not be used outside of these routines.
typedef IO_handle MCDeployFileRef;

bool MCDeployFileOpen(MCStringRef p_path, intenum_t p_mode, MCDeployFileRef& r_file);
void MCDeployFileClose(MCDeployFileRef p_file);

bool MCDeployFileRead(MCDeployFileRef p_file, void *p_buffer, uint32_t p_buffer_size);
bool MCDeployFileReadAt(MCDeployFileRef p_file, void *p_buffer, uint32_t p_buffer_size, uint32_t p_at);
bool MCDeployFileSeekSet(MCDeployFileRef p_file, long p_offset);
bool MCDeployFileCopy(MCDeployFileRef p_dst, uint32_t p_at, MCDeployFileRef p_src, uint32_t p_from, uint32_t p_amount);
bool MCDeployFileWriteAt(MCDeployFileRef p_dst, const void *p_buffer, uint32_t p_size, uint32_t p_at);
bool MCDeployFileMeasure(MCDeployFileRef p_file, uint32_t& r_size);

////////////////////////////////////////////////////////////////////////////////

void MCDeployByteSwap32(bool p_to_network, uint32_t& p_var);
void MCDeployByteSwapRecord(bool p_to_network, const char *p_format, void *p_data, uint32_t p_data_size);

////////////////////////////////////////////////////////////////////////////////

bool MCDeployWriteCapsule(const MCDeployParameters& p_params, MCDeployFileRef p_output, uint32_t& x_offset);
bool MCDeployWriteProject(const MCDeployParameters& p_params, bool p_to_network, MCDeployFileRef p_output, uint32_t p_output_offset, uint32_t& r_project_size);
bool MCDeployWritePayload(const MCDeployParameters& p_params, bool p_to_network, MCDeployFileRef p_output, uint32_t p_output_offset, uint32_t& r_payload_size);

////////////////////////////////////////////////////////////////////////////////

bool MCDeployWindowsPEHeaderOffset(MCDeployFileRef p_file, uint32_t &r_pe_offset);
bool MCDeployWindowsArchitecture(MCDeployFileRef p_file, uint32_t p_pe_offset, MCDeployArchitecture &r_platform);

////////////////////////////////////////////////////////////////////////////////

#endif
