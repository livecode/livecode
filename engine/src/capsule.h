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

#ifndef __MC_CAPSULE__
#define __MC_CAPSULE__

////////////////////////////////////////////////////////////////////////////////

// A capsule is sequence of sections, each independent and capable of being
// loaded as soon as all their data is available. The sections are used by the
// RRE or standalone engines to configure the environment for the running of
// the application including, but not limited to, checking security permissions,
// validating the binary data and loading the principal stack file.

// The sections are loaded in order on startup, with any unrecognized types
// ignored and skipped over. This facility means that the format can remain
// backward-compatible while still being updated with new features as newer
// engines can pick up new section types, and ignore the old ones that are
// present to allow the capsule to run in an older version.

// The sections are designed so that they can be processed as soon as all their
// data has been loaded/streamed. This means that actions can start to be taken
// before all the data has arrived.

// Sections that are fixed records are only checked for minimum data size. This
// means that some sections can be easily extended without impacting load into
// older runtime version. For example, the header section could be augmented
// with extra fields in a later version which matching runtimes could pick up
// by checking the size.

// As the capsule format is a byte-oriented stream, endianness of data-types
// is important. All data-types that are longer than 1 byte in length will
// be serialized in network (big) endian format.

// Sections always start on a 32-bit word boundary, however the size in the
// tag is always the exact byte length of the stream - this obviates the need
// for an additional size field in things such as the Stack and Image section.
// The reason for the padding is so that records can be loaded into memory whole
// and then byteswapped, rather than requiring individual elements to be
// loaded separately.

// The types of the currently defined capsule sections are defined by the
// MCCapsuleSectionType enum:
enum MCCapsuleSectionType
{
	// The epilogue section marks the end of the capsule stream. Any data
	// present beyond this is an error. (Note that this is *before* prologue
	// in the enum because it should have id 0 which matches the fact it is
	// of zero size).
	kMCCapsuleSectionTypeEpilogue,

	// The prologue section contains information pertaining to the capsule as
	// a whole.
	kMCCapsuleSectionTypePrologue,

	// A digest section contains the md5 digest of all data up to but not
	// including the digest section tag.
	kMCCapsuleSectionTypeDigest,

	// The stack section contains the principal (binary) stackfile to be loaded on
	// startup.
	kMCCapsuleSectionTypeMainStack,

    // The stack section contains the principal (script-only) stackfile to be loaded on
    // startup.
    kMCCapsuleSectionTypeScriptOnlyMainStack,
    
	// An external section contains a external that should be loaded into the
	// environment on startup;.
	kMCCapsuleSectionTypeExternal,
    
    // Module to be loaded on startup.
    kMCCapsuleSectionTypeModule,

    // Auxiliary stack sections contain other mainstacks that should be loaded
	// alongside the mainstack (but not opened initially).
	kMCCapsuleSectionTypeAuxiliaryStack,
    
    // Script-only auxiliary stack sections contain other (script-only) mainstacks that should be loaded
    // alongside the mainstack (but not opened initially).
    kMCCapsuleSectionTypeScriptOnlyAuxiliaryStack,
	
	// Simulator redirect sections contain mappings from engine relative
	// paths to absolute paths on the host system.
	kMCCapsuleSectionTypeRedirect,
	
	// Startup script to be executed after all stacks have loaded but before
	// the main stack is opened.
	kMCCapsuleSectionTypeStartupScript,
    
    // Font mapping sections contain a mapping from a font name to another font
    // name (usually PostScript name). Whenever a font name is looked up it is
    // indirected through the font map first (and only once - not iteratively).
    kMCCapsuleSectionTypeFontmap,

    // AL-2015-02-10: [[ Standalone Inclusions ]] Library consists of the mappings from universal names
    //  of resources to their platform-specific paths relative to the executable.
    kMCCapsuleSectionTypeLibrary,
    
    // MW-2016-02-17: [[ LicenseChecks ]] License consists of the array-encoded
    //   'revLicenseInfo' array in use at the point the standalone was built.
    kMCCapsuleSectionTypeLicense,
	
	// MW-2016-02-17: [[ Trial ]] If a banner is present, it is serialized as a
	//   stackfile in this section.
	kMCCapsuleSectionTypeBanner,
};

// Each section begins with a header that defines its type and length. This is
// then immediately followed by length bytes of data. Headers themselves can be
// one or two words in length. The first word has structure:
//   bits 0..23 - length[0..23]
//   bits 24..30 - type[0..6]
//   bit 31 - extension
// If the extension bit is set, it means there is an additional word:
//   bits 0..7 - length[24..31]
//   bits 8..31 - type[7..30]
// (Obviously the ordering of the bit fields could be either way around, except
//  that with this order, it is possible to determine the header length with a 
//  single byte of look-ahead as when serialized in big-endian the initial type
//  and extension bit will be the first byte).

////////////////////

// The Epilogue section is currently empty:
struct MCCapsuleEpilogueSection
{
};

////////////////////

// The Prologue section is defined by the following structure:
struct MCCapsulePrologueSection
{
	uint32_t banner_timeout;
	uint32_t program_timeout;
};

IO_stat MCCapsulePrologueSectionRead(IO_handle p_stream, MCCapsulePrologueSection& r_prologue);

////////////////////

// A Digest section consists of an md5-digest of all data up to but not including
// the digest section's tag. There can be as many digest sections as desired, and
// if the digest fails to match at any point, it is an error.
struct MCCapsuleDigestSection
{
	uint8_t digest[16];
};

// The Stack section contains the stackfile data for the principal stack.
struct MCCapsuleStackSection
{
	// uint8_t data[]
};

// The Stack section contains the stackfile data for an auxiliary stack.
struct MCCapsuleAuxiliaryStackSection
{
	// uint8_t data[]
};

// The External section contains the name of an external to load on startup
struct MCCapsuleExternalSection
{
	// char name[];
};

////////////////////////////////////////////////////////////////////////////////

// The MCCapsuleRef opaque type represents a capsule while it is being loaded/
// processed. It is used by the standalone and runtime engines to launch an
// application. In order to support streaming, capsule loading follows a fill/
// process model - the caller provides data to the capsule via the Fill method
// and then can request that more sections try to be processed by calling
// Process. Note that the capsule code itself does nothing to interpret the
// sections - that is completely up to the caller. It merely provides the data.

// Given how things currently work, it would be possible to construct a rogue
// revlet that causes a significant amount of memory to be used in buffering
// an unfinished section. In order to avoid this, there is a hard limit on the
// maximum size of a section that can be buffering (note that buffering only
// occurs up to the point the capsule has been completely filled with all the
// data needed to parse the whole revlet). When a section is encountered that
// hits this hard limit the capsule becomes 'blocked' and no more sections
// will be processed until all data is available.

// The opaque type definition
typedef struct MCCapsule *MCCapsuleRef;

// The capsule callback definition.
typedef bool (*MCCapsuleCallback)(void *state, const uint8_t *digest, MCCapsuleSectionType type, uint32_t length, IO_handle stream);

// This method opens a capsule making it ready to receive data via Fill. When
// sections are processed, they will invoke the specified callback
bool MCCapsuleOpen(MCCapsuleCallback callback, void *callback_state, MCCapsuleRef& r_self);

// This method closes a capsule freeing any data it has associated with it.
void MCCapsuleClose(MCCapsuleRef self);

// This method returns true if the capsule is currently blocked because the
// size of an intermediate buffer has overflown the internal limit. In this
// case, the caller must wait until all data is available before continuing.
bool MCCapsuleBlocked(MCCapsuleRef self);

// This method provides data to the capsule from the given buffer. The capsule
// takes a copy and stores the data ready for processing. If the finished flag
// is true, it means there is no more data to come.
bool MCCapsuleFill(MCCapsuleRef self, const void *data, uint32_t data_length, bool finished);
bool MCCapsuleFillNoCopy(MCCapsuleRef self, const void *data, uint32_t data_length, bool finished);

// This method provides data to the capsule from the given file. The capsule
// will open the file at this point, but not read any data until required. It
// will close the file as soon as it is finished with it. It will start reading
// data at the given offset and for the given number of bytes.
bool MCCapsuleFillFromFile(MCCapsuleRef self, MCStringRef path, uint32_t offset, bool finished);

// The process method attempts to parse as many sections as it can, invoking the
// callback for each one. Note that a section cannot be processed until the capsule
// has access to all its data.
bool MCCapsuleProcess(MCCapsuleRef self);

////////////////////////////////////////////////////////////////////////////////

// The MCDeployCapsule opaque type represents a capsule currently under
// construction. It is used by the 'deploy' module to generate capsules when
// building standalones and revlets. Methods that can throw errors have
// a 'bool' return type. The error codes are shared with the deploy modules.

// The opaque type definition.
typedef struct MCDeployCapsule *MCDeployCapsuleRef;

// This method creates a deploy capsule, it initially contains no sections.
bool MCDeployCapsuleCreate(MCDeployCapsuleRef& r_self);

// This method destroys the deploy capsule, freeing all data it currently holds.
// (Note that this has no effect on any capsules that have been written).
void MCDeployCapsuleDestroy(MCDeployCapsuleRef self);

// This method appends a new section of the given type containing the
// specified data.
bool MCDeployCapsuleDefine(MCDeployCapsuleRef self, MCCapsuleSectionType type, const void *data, uint32_t data_size);
bool MCDeployCapsuleDefineString(MCDeployCapsuleRef self, MCCapsuleSectionType type, MCStringRef p_string);

// This method appends a new section of the given type contining the data
// held in the given file - the caller is responsible for managing the lifetime
// of the file.
bool MCDeployCapsuleDefineFromFile(MCDeployCapsuleRef self, MCCapsuleSectionType type, MCDeployFileRef file);

// This method appends a digest section to the given capsule.
bool MCDeployCapsuleChecksum(MCDeployCapsuleRef self);

// This method writes out the capsule data to the given file, starting at byte
// x_offset, and returning the resulting offset. The stream can be optionally
// compressed and/or masked. If split_file is not nil, all but 4K of the data
// (the middle part) will be output to that file.
bool MCDeployCapsuleGenerate(MCDeployCapsuleRef self, MCDeployFileRef file, MCDeployFileRef split_file, uint32_t& x_offset);

////////////////////////////////////////////////////////////////////////////////

#endif
