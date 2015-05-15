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

#ifndef __MC_DEPLOY__
#define __MC_DEPLOY__

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
	char *engine;
	char *engine_ppc;
	char *engine_x86;

	// When building on Windows, this might contain an array with the versioninfo
	// fields.
	MCVariableValue *version_info;

    // When building for Mac/iOS, you can specify a min os version per arch
    // slice.
    MCDeployMinOSVersion *min_os_versions;
    uindex_t min_os_version_count;
    
	// The root stackfile to be included in the standalone.
	char *stackfile;
	// The array of auxiliary stackfiles to be included in the standalone.
	char **auxiliary_stackfiles;
	uint32_t auxiliary_stackfile_count;
	// The array of externals to be loaded on startup by the standalone.
	char **externals;
	uint32_t external_count;
	// The script to be executed on startup by the standalone.
	char *startup_script;
	
	// If true, then the standalone will have an implicit timeout
	uint32_t timeout;
	
	// The list of redirection mappings
	char **redirects;
	uint32_t redirect_count;
    
    // The list of font mappings
    char **fontmappings;
    uint32_t fontmapping_count;
    
    // AL-2015-02-10: [[ Standalone Inclusions ]] The list of resource mappings.
    char **library;
    uint32_t library_count;
    
	// On Windows, the icon files to be inserted into the resource directory.
	char *app_icon;
	char *doc_icon;

	// On Windows, the path to the manifest file to be included in the executable
	// (optional).
	char *manifest;

	// When building an installer, the path to the payload to be inserted
	char *payload;

	// If this is non-nil, it contains a file to be used to 'spill' data above
	// 4K into. It is designed for use on OS X to reduce standalone file size.
	char *spill;

	// The output path for the new executable.
	char *output;
};

Exec_stat MCDeployToWindows(const MCDeployParameters& p_params);
Exec_stat MCDeployToLinux(const MCDeployParameters& p_params);
Exec_stat MCDeployToMacOSX(const MCDeployParameters& p_params);
Exec_stat MCDeployToIOS(const MCDeployParameters& p_params, bool embedded);
Exec_stat MCDeployToAndroid(const MCDeployParameters& p_params);

////////////////////////////////////////////////////////////////////////////////

struct MCDeploySignParameters
{
	// The passphrase needed to decrypt any of the other files (if needed).
	char *passphrase;

	// The path to the software publishing certificate stored in a PKCS#7
	// SignedInfo structure.
	char *certificate;

	// The path to the private key file stored in a PKCS#8 PrivateKey
	// structure or the Microsoft PVK file format.
	char *privatekey;

	// The path to either a PKCS#12 certificate/privatekey store. This is
	// only needed if the previous two fields are nil.
	char *certstore;

	// The url to use for timestamping - if needed
	char *timestamper;

	// The UTF-8 string describing the pprogram name/description
	char *description;
	// The UTF-8 string contining the URL of the company
	char *url;

	// The path to the executable to sign
	char *input;

	// The outpath for the resulting signed executable
	char *output;
};

bool MCDeploySignWindows(const MCDeploySignParameters& p_params);

////////////////////////////////////////////////////////////////////////////////

struct MCDeployDietParameters
{
	// The input engine to process
	char *input;

	// Which architectures to keep (if present)
	bool keep_x86;
	bool keep_x86_64;
	bool keep_ppc;
	bool keep_ppc64;
	bool keep_arm;

	// Whether to keep symbols
	bool keep_debug_symbols;

	// Where to put the output
	char *output;
};

Exec_stat MCDeployDietMacOSX(const MCDeployDietParameters& p_params);

////////////////////////////////////////////////////////////////////////////////

struct MCDeployDmgItem
{
	uint32_t parent;
	char *name;
	bool is_folder;

	// BSD permission info
	uint32_t owner_id;
	uint32_t group_id;
	uint32_t file_mode;

	// If any of these are 0 they take on 'default' values.
	uint32_t create_date;
	uint32_t content_mod_date;
	uint32_t attribute_mod_date;
	uint32_t access_date;
	uint32_t backup_date;

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

Exec_stat MCDeployExtractMacOSX(const char *p_filename, const char *p_segment, const char *p_section, void*& r_data, uint32_t& r_data_size);

////////////////////////////////////////////////////////////////////////////////

enum MCDeployError
{
	kMCDeployErrorNone,
	kMCDeployErrorNoMemory,
	kMCDeployErrorNoEngine,
	kMCDeployErrorNoStackfile,
	kMCDeployErrorNoAuxStackfile,
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

	// An error occured while building the signature
	kMCDeployErrorBadSignature,

	// An error occured while trying to convert a string
	kMCDeployErrorBadString,

	// An error occured while trying to compute the hash
	kMCDeployErrorBadHash,

	// An error occured while trying to fetch a timestamp
	kMCDeployErrorTimestampFailed,

	// An error occured decoding the timestamp response
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
typedef FILE *MCDeployFileRef;

bool MCDeployFileOpen(const char *p_path, const char *p_mode , MCDeployFileRef& r_file);
void MCDeployFileClose(MCDeployFileRef p_file);

bool MCDeployFileRead(MCDeployFileRef p_file, void *p_buffer, uint32_t p_buffer_size);
bool MCDeployFileReadAt(MCDeployFileRef p_file, void *p_buffer, uint32_t p_buffer_size, uint32_t p_at);
bool MCDeployFileSeek(MCDeployFileRef p_file, long p_offset, int p_origin);
bool MCDeployFileCopy(MCDeployFileRef p_dst, uint32_t p_at, MCDeployFileRef p_src, uint32_t p_from, uint32_t p_amount);
bool MCDeployFileWriteAt(MCDeployFileRef p_dst, const void *p_buffer, uint32_t p_size, uint32_t p_at);
bool MCDeployFileTruncate(MCDeployFileRef p_dst, uint32_t amount);
bool MCDeployFileMeasure(MCDeployFileRef p_file, uint32_t& r_size);

////////////////////////////////////////////////////////////////////////////////

void MCDeployByteSwap32(bool p_to_network, uint32_t& p_var);
void MCDeployByteSwapRecord(bool p_to_network, const char *p_format, void *p_data, uint32_t p_data_size);

////////////////////////////////////////////////////////////////////////////////

bool MCDeployWriteCapsule(const MCDeployParameters& p_params, MCDeployFileRef p_output, uint32_t& x_offset);
bool MCDeployWriteProject(const MCDeployParameters& p_params, bool p_to_network, MCDeployFileRef p_output, uint32_t p_output_offset, uint32_t& r_project_size);
bool MCDeployWritePayload(const MCDeployParameters& p_params, bool p_to_network, MCDeployFileRef p_output, uint32_t p_output_offset, uint32_t& r_payload_size);

////////////////////////////////////////////////////////////////////////////////

#endif
