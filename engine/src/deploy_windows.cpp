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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"


#include "exec.h"
#include "handler.h"
#include "scriptpt.h"
#include "variable.h"
#include "statemnt.h"
#include "osspec.h"

#include "deploy.h"

constexpr uint32_t kAddressToPEAddress = 60;
constexpr uint32_t kPEAddressSize = 4;
constexpr uint32_t kMagicOffset = 0x18;
constexpr uint16_t kHeaderMagic32 = 0x10b;
constexpr uint16_t kHeaderMagic64 = 0x20b;

////////////////////////////////////////////////////////////////////////////////
//
// This section contains definitions for the various structures needed to
// process the PE format. On Windows many of these are defined in winnt.h which
// is always included. Obviously, on other platforms this is not present so we
// replicate them here.
//

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)    ((LONG)(intptr_t)&(((type *)0)->field))
#endif

// Defining common types for 32 and 64 bit
#if !defined(_WIN32) && !defined(_WIN64)

typedef char CHAR;
typedef unsigned short WCHAR;

typedef unsigned char BYTE;
typedef unsigned short WORD;

// FG-2014-09-17: [[ Bugfix 13463 ]] "long" is 64 bits on Linux x86_64
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uintptr_t LONG_PTR;

#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_OS2_SIGNATURE                 0x454E      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x454C      // LE
#define IMAGE_VXD_SIGNATURE                 0x454C      // LE
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00

typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
    WORD   e_magic;                     // Magic number
    WORD   e_cblp;                      // Bytes on last page of file
    WORD   e_cp;                        // Pages in file
    WORD   e_crlc;                      // Relocations
    WORD   e_cparhdr;                   // Size of header in paragraphs
    WORD   e_minalloc;                  // Minimum extra paragraphs needed
    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD   e_ss;                        // Initial (relative) SS value
    WORD   e_sp;                        // Initial SP value
    WORD   e_csum;                      // Checksum
    WORD   e_ip;                        // Initial IP value
    WORD   e_cs;                        // Initial (relative) CS value
    WORD   e_lfarlc;                    // File address of relocation table
    WORD   e_ovno;                      // Overlay number
    WORD   e_res[4];                    // Reserved words
    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD   e_oeminfo;                   // OEM information; e_oemid specific
    WORD   e_res2[10];                  // Reserved words
    LONG   e_lfanew;                    // File address of new exe header
  } IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

//
// File header format.
//

#define IMAGE_SIZEOF_FILE_HEADER             20

#define IMAGE_FILE_RELOCS_STRIPPED           0x0001  // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  // File is executable  (i.e. no unresolved externel references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004  // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008  // Local symbols stripped from file.
#define IMAGE_FILE_AGGRESIVE_WS_TRIM         0x0010  // Agressively trim working set
#define IMAGE_FILE_LARGE_ADDRESS_AWARE       0x0020  // App can handle >2gb addresses
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080  // Bytes of machine word are reversed.
#define IMAGE_FILE_32BIT_MACHINE             0x0100  // 32 bit word machine.
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200  // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   0x0400  // If Image is on removable media, copy and run from the swap file.
#define IMAGE_FILE_NET_RUN_FROM_SWAP         0x0800  // If Image is on Net, copy and run from the swap file.
#define IMAGE_FILE_SYSTEM                    0x1000  // System File.
#define IMAGE_FILE_DLL                       0x2000  // File is a DLL.
#define IMAGE_FILE_UP_SYSTEM_ONLY            0x4000  // File should only be run on a UP machine
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000  // Bytes of machine word are reversed.

#define IMAGE_FILE_MACHINE_UNKNOWN           0
#define IMAGE_FILE_MACHINE_I386              0x014c  // Intel 386.
#define IMAGE_FILE_MACHINE_R3000             0x0162  // MIPS little-endian, 0x160 big-endian
#define IMAGE_FILE_MACHINE_R4000             0x0166  // MIPS little-endian
#define IMAGE_FILE_MACHINE_R10000            0x0168  // MIPS little-endian
#define IMAGE_FILE_MACHINE_WCEMIPSV2         0x0169  // MIPS little-endian WCE v2
#define IMAGE_FILE_MACHINE_ALPHA             0x0184  // Alpha_AXP
#define IMAGE_FILE_MACHINE_SH3               0x01a2  // SH3 little-endian
#define IMAGE_FILE_MACHINE_SH3DSP            0x01a3
#define IMAGE_FILE_MACHINE_SH3E              0x01a4  // SH3E little-endian
#define IMAGE_FILE_MACHINE_SH4               0x01a6  // SH4 little-endian
#define IMAGE_FILE_MACHINE_SH5               0x01a8  // SH5
#define IMAGE_FILE_MACHINE_ARM               0x01c0  // ARM Little-Endian
#define IMAGE_FILE_MACHINE_THUMB             0x01c2
#define IMAGE_FILE_MACHINE_AM33              0x01d3
#define IMAGE_FILE_MACHINE_POWERPC           0x01F0  // IBM PowerPC Little-Endian
#define IMAGE_FILE_MACHINE_POWERPCFP         0x01f1
#define IMAGE_FILE_MACHINE_IA64              0x0200  // Intel 64
#define IMAGE_FILE_MACHINE_MIPS16            0x0266  // MIPS
#define IMAGE_FILE_MACHINE_ALPHA64           0x0284  // ALPHA64
#define IMAGE_FILE_MACHINE_MIPSFPU           0x0366  // MIPS
#define IMAGE_FILE_MACHINE_MIPSFPU16         0x0466  // MIPS
#define IMAGE_FILE_MACHINE_AXP64             IMAGE_FILE_MACHINE_ALPHA64
#define IMAGE_FILE_MACHINE_TRICORE           0x0520  // Infineon
#define IMAGE_FILE_MACHINE_CEF               0x0CEF
#define IMAGE_FILE_MACHINE_EBC               0x0EBC  // EFI Byte Code
#define IMAGE_FILE_MACHINE_AMD64             0x8664  // AMD64 (K8)
#define IMAGE_FILE_MACHINE_M32R              0x9041  // M32R little-endian
#define IMAGE_FILE_MACHINE_CEE               0xC0EE

//
// Directory format.
//

typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD   VirtualAddress;
    DWORD   Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16


// Directory Entries

#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
//      IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor

//
// Section header format.
//

#define IMAGE_SIZEOF_SHORT_NAME              8

typedef struct _IMAGE_SECTION_HEADER {
    BYTE    Name[IMAGE_SIZEOF_SHORT_NAME];
    union {
            DWORD   PhysicalAddress;
            DWORD   VirtualSize;
    } Misc;
    DWORD   VirtualAddress;
    DWORD   SizeOfRawData;
    DWORD   PointerToRawData;
    DWORD   PointerToRelocations;
    DWORD   PointerToLinenumbers;
    WORD    NumberOfRelocations;
    WORD    NumberOfLinenumbers;
    DWORD   Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define IMAGE_SIZEOF_SECTION_HEADER          40

//
// Section characteristics.
//
//      IMAGE_SCN_TYPE_REG                   0x00000000  // Reserved.
//      IMAGE_SCN_TYPE_DSECT                 0x00000001  // Reserved.
//      IMAGE_SCN_TYPE_NOLOAD                0x00000002  // Reserved.
//      IMAGE_SCN_TYPE_GROUP                 0x00000004  // Reserved.
#define IMAGE_SCN_TYPE_NO_PAD                0x00000008  // Reserved.
//      IMAGE_SCN_TYPE_COPY                  0x00000010  // Reserved.

#define IMAGE_SCN_CNT_CODE                   0x00000020  // Section contains code.
#define IMAGE_SCN_CNT_INITIALIZED_DATA       0x00000040  // Section contains initialized data.
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA     0x00000080  // Section contains uninitialized data.

#define IMAGE_SCN_LNK_OTHER                  0x00000100  // Reserved.
#define IMAGE_SCN_LNK_INFO                   0x00000200  // Section contains comments or some other type of information.
//      IMAGE_SCN_TYPE_OVER                  0x00000400  // Reserved.
#define IMAGE_SCN_LNK_REMOVE                 0x00000800  // Section contents will not become part of image.
#define IMAGE_SCN_LNK_COMDAT                 0x00001000  // Section contents comdat.
//                                           0x00002000  // Reserved.
//      IMAGE_SCN_MEM_PROTECTED - Obsolete   0x00004000
#define IMAGE_SCN_NO_DEFER_SPEC_EXC          0x00004000  // Reset speculative exceptions handling bits in the TLB entries for this section.
#define IMAGE_SCN_GPREL                      0x00008000  // Section content can be accessed relative to GP
#define IMAGE_SCN_MEM_FARDATA                0x00008000
//      IMAGE_SCN_MEM_SYSHEAP  - Obsolete    0x00010000
#define IMAGE_SCN_MEM_PURGEABLE              0x00020000
#define IMAGE_SCN_MEM_16BIT                  0x00020000
#define IMAGE_SCN_MEM_LOCKED                 0x00040000
#define IMAGE_SCN_MEM_PRELOAD                0x00080000

#define IMAGE_SCN_ALIGN_1BYTES               0x00100000  //
#define IMAGE_SCN_ALIGN_2BYTES               0x00200000  //
#define IMAGE_SCN_ALIGN_4BYTES               0x00300000  //
#define IMAGE_SCN_ALIGN_8BYTES               0x00400000  //
#define IMAGE_SCN_ALIGN_16BYTES              0x00500000  // Default alignment if no others are specified.
#define IMAGE_SCN_ALIGN_32BYTES              0x00600000  //
#define IMAGE_SCN_ALIGN_64BYTES              0x00700000  //
#define IMAGE_SCN_ALIGN_128BYTES             0x00800000  //
#define IMAGE_SCN_ALIGN_256BYTES             0x00900000  //
#define IMAGE_SCN_ALIGN_512BYTES             0x00A00000  //
#define IMAGE_SCN_ALIGN_1024BYTES            0x00B00000  //
#define IMAGE_SCN_ALIGN_2048BYTES            0x00C00000  //
#define IMAGE_SCN_ALIGN_4096BYTES            0x00D00000  //
#define IMAGE_SCN_ALIGN_8192BYTES            0x00E00000  //
// Unused                                    0x00F00000
#define IMAGE_SCN_ALIGN_MASK                 0x00F00000

#define IMAGE_SCN_LNK_NRELOC_OVFL            0x01000000  // Section contains extended relocations.
#define IMAGE_SCN_MEM_DISCARDABLE            0x02000000  // Section can be discarded.
#define IMAGE_SCN_MEM_NOT_CACHED             0x04000000  // Section is not cachable.
#define IMAGE_SCN_MEM_NOT_PAGED              0x08000000  // Section is not pageable.
#define IMAGE_SCN_MEM_SHARED                 0x10000000  // Section is shareable.
#define IMAGE_SCN_MEM_EXECUTE                0x20000000  // Section is executable.
#define IMAGE_SCN_MEM_READ                   0x40000000  // Section is readable.
#define IMAGE_SCN_MEM_WRITE                  0x80000000  // Section is writeable.

//
// Resource Format.
//

//
// Resource directory consists of two counts, following by a variable length
// array of directory entries.  The first count is the number of entries at
// beginning of the array that have actual names associated with each entry.
// The entries are in ascending order, case insensitive strings.  The second
// count is the number of entries that immediately follow the named entries.
// This second count identifies the number of entries that have 16-bit integer
// Ids as their name.  These entries are also sorted in ascending order.
//
// This structure allows fast lookup by either name or number, but for any
// given resource entry only one form of lookup is supported, not both.
// This is consistant with the syntax of the .RC file and the .RES file.
//

typedef struct _IMAGE_RESOURCE_DIRECTORY {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    WORD    NumberOfNamedEntries;
    WORD    NumberOfIdEntries;
//  IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[];
} IMAGE_RESOURCE_DIRECTORY, *PIMAGE_RESOURCE_DIRECTORY;

#define IMAGE_RESOURCE_NAME_IS_STRING        0x80000000
#define IMAGE_RESOURCE_DATA_IS_DIRECTORY     0x80000000
//
// Each directory contains the 32-bit Name of the entry and an offset,
// relative to the beginning of the resource directory of the data associated
// with this directory entry.  If the name of the entry is an actual text
// string instead of an integer Id, then the high order bit of the name field
// is set to one and the low order 31-bits are an offset, relative to the
// beginning of the resource directory of the string, which is of type
// IMAGE_RESOURCE_DIRECTORY_STRING.  Otherwise the high bit is clear and the
// low-order 16-bits are the integer Id that identify this resource directory
// entry. If the directory entry is yet another resource directory (i.e. a
// subdirectory), then the high order bit of the offset field will be
// set to indicate this.  Otherwise the high bit is clear and the offset
// field points to a resource data entry.
//

typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
    union {
        struct {
            DWORD NameOffset:31;
            DWORD NameIsString:1;
        };
        DWORD   Name;
#ifdef __BIG_ENDIAN__
		WORD __pad;
		WORD Id;
#else
        WORD    Id;
#endif
    };
    union {
        DWORD   OffsetToData;
        struct {
#ifdef __BIG_ENDIAN__
            DWORD   DataIsDirectory:1;
            DWORD   OffsetToDirectory:31;
#else
            DWORD   OffsetToDirectory:31;
            DWORD   DataIsDirectory:1;
#endif
        };
    };
} IMAGE_RESOURCE_DIRECTORY_ENTRY, *PIMAGE_RESOURCE_DIRECTORY_ENTRY;

//
// For resource directory entries that have actual string names, the Name
// field of the directory entry points to an object of the following type.
// All of these string objects are stored together after the last resource
// directory entry and before the first resource data object.  This minimizes
// the impact of these variable length objects on the alignment of the fixed
// size directory entry objects.
//

typedef struct _IMAGE_RESOURCE_DIRECTORY_STRING {
    WORD    Length;
    CHAR    NameString[ 1 ];
} IMAGE_RESOURCE_DIRECTORY_STRING, *PIMAGE_RESOURCE_DIRECTORY_STRING;


typedef struct _IMAGE_RESOURCE_DIR_STRING_U {
    WORD    Length;
    WCHAR   NameString[ 1 ];
} IMAGE_RESOURCE_DIR_STRING_U, *PIMAGE_RESOURCE_DIR_STRING_U;


//
// Each resource data entry describes a leaf node in the resource directory
// tree.  It contains an offset, relative to the beginning of the resource
// directory of the data for the resource, a size field that gives the number
// of bytes of data at that offset, a CodePage that should be used when
// decoding code point values within the resource data.  Typically for new
// applications the code page would be the unicode code page.
//

typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
    DWORD   OffsetToData;
    DWORD   Size;
    DWORD   CodePage;
    DWORD   Reserved;
} IMAGE_RESOURCE_DATA_ENTRY, *PIMAGE_RESOURCE_DATA_ENTRY;

typedef struct tagVS_FIXEDFILEINFO
{
    DWORD   dwSignature;            /* e.g. 0xfeef04bd */
    DWORD   dwStrucVersion;         /* e.g. 0x00000042 = "0.42" */
    DWORD   dwFileVersionMS;        /* e.g. 0x00030075 = "3.75" */
    DWORD   dwFileVersionLS;        /* e.g. 0x00000031 = "0.31" */
    DWORD   dwProductVersionMS;     /* e.g. 0x00030010 = "3.10" */
    DWORD   dwProductVersionLS;     /* e.g. 0x00000031 = "0.31" */
    DWORD   dwFileFlagsMask;        /* = 0x3F for version "0.42" */
    DWORD   dwFileFlags;            /* e.g. VFF_DEBUG | VFF_PRERELEASE */
    DWORD   dwFileOS;               /* e.g. VOS_DOS_WINDOWS16 */
    DWORD   dwFileType;             /* e.g. VFT_DRIVER */
    DWORD   dwFileSubtype;          /* e.g. VFT2_DRV_KEYBOARD */
    DWORD   dwFileDateMS;           /* e.g. 0 */
    DWORD   dwFileDateLS;           /* e.g. 0 */
} VS_FIXEDFILEINFO;
#endif

#if !defined(_WIN32)
//
// Optional header format.
//
typedef struct _IMAGE_OPTIONAL_HEADER_32 {
    //
    // Standard fields.
    //

    WORD    Magic;
    BYTE    MajorLinkerVersion;
    BYTE    MinorLinkerVersion;
    DWORD   SizeOfCode;
    DWORD   SizeOfInitializedData;
    DWORD   SizeOfUninitializedData;
    DWORD   AddressOfEntryPoint;
    DWORD   BaseOfCode;
    DWORD   BaseOfData;

    //
    // NT additional fields.
    //

    DWORD   ImageBase; // l
    DWORD   SectionAlignment; // l
    DWORD   FileAlignment; // l
    WORD    MajorOperatingSystemVersion; // s
    WORD    MinorOperatingSystemVersion; // s
    WORD    MajorImageVersion; // s
    WORD    MinorImageVersion; // s
    WORD    MajorSubsystemVersion; // s
    WORD    MinorSubsystemVersion; // s
    DWORD   Win32VersionValue; // l
    DWORD   SizeOfImage; // l
    DWORD   SizeOfHeaders; // l
    DWORD   CheckSum; // l
    WORD    Subsystem; // s
    WORD    DllCharacteristics; // s
    DWORD   SizeOfStackReserve; // l
    DWORD   SizeOfStackCommit; // l
    DWORD   SizeOfHeapReserve; // l
    DWORD   SizeOfHeapCommit; // l
    DWORD   LoaderFlags; // l
    DWORD   NumberOfRvaAndSizes; // l
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

// The following section should move to the template args
// in the templated version of MCDeployWindows

/*
typedef IMAGE_OPTIONAL_HEADER32             IMAGE_OPTIONAL_HEADER;
typedef PIMAGE_OPTIONAL_HEADER32            PIMAGE_OPTIONAL_HEADER;
*/
#define IMAGE_NT_OPTIONAL_HDR_MAGIC         IMAGE_NT_OPTIONAL_HDR32_MAGIC


typedef struct _IMAGE_NT_HEADERS {
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

// The following section should move to the template args
// in the templated version of MCDeployWindows

/*
typedef IMAGE_NT_HEADERS32                  IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS32                 PIMAGE_NT_HEADERS;
*/

#endif // if !defined(_WIN32)

#if !defined(_WIN64) && !defined(_WINNT_)

typedef uint64_t ULONGLONG;

//
// Optional header format.
//

typedef struct _IMAGE_OPTIONAL_HEADER_64 {
	// Standard fields

	WORD        Magic; // s
	BYTE        MajorLinkerVersion; // b
	BYTE        MinorLinkerVersion; // b
	DWORD       SizeOfCode; // l
	DWORD       SizeOfInitializedData; // l
	DWORD       SizeOfUninitializedData; // l
	DWORD       AddressOfEntryPoint; // l
	DWORD       BaseOfCode; // l

	// NT Fields

	ULONGLONG   ImageBase; // q
	DWORD       SectionAlignment; // l
	DWORD       FileAlignment; // l
	WORD        MajorOperatingSystemVersion; // s
	WORD        MinorOperatingSystemVersion; // s
	WORD        MajorImageVersion; // s
	WORD        MinorImageVersion; // s
	WORD        MajorSubsystemVersion; // s
	WORD        MinorSubsystemVersion; // s
	DWORD       Win32VersionValue; // l
	DWORD       SizeOfImage; // l
	DWORD       SizeOfHeaders; // l
	DWORD       CheckSum; // l
	WORD        Subsystem; // s
	WORD        DllCharacteristics; // s
	ULONGLONG   SizeOfStackReserve; // q
	ULONGLONG   SizeOfStackCommit; // q
	ULONGLONG   SizeOfHeapReserve; // q
	ULONGLONG   SizeOfHeapCommit; // q
	DWORD       LoaderFlags; // l
	DWORD       NumberOfRvaAndSizes; // l
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];

	// sbbl lll l q ll ss ss ss l l l l s s q q q q l l 
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

// The following section should move to the template args
// in the templated version of MCDeployWindows

/*
typedef IMAGE_OPTIONAL_HEADER64             IMAGE_OPTIONAL_HEADER;
typedef PIMAGE_OPTIONAL_HEADER64            PIMAGE_OPTIONAL_HEADER;
*/
#define IMAGE_NT_OPTIONAL_HDR_MAGIC         IMAGE_NT_OPTIONAL_HDR64_MAGIC


typedef struct _IMAGE_NT_HEADERS_64 {
	DWORD Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

// The following section should move to the template args
// in the templated version of MCDeployWindows

/*
typedef IMAGE_NT_HEADERS64                  IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS64                 PIMAGE_NT_HEADERS;
*/

#endif // if !defined(_WIN64)

// The following structures are for those used in ICO files and in ICON and
// GROUP_ICON resources. These (for some reason) do not appear in any of the
// windows headers, so we replicate them for all platforms here.

typedef struct
{
    BYTE        bWidth;          // Width, in pixels, of the image
    BYTE        bHeight;         // Height, in pixels, of the image
    BYTE        bColorCount;     // Number of colors in image (0 if >=8bpp)
    BYTE        bReserved;       // Reserved ( must be 0)
    WORD        wPlanes;         // Color Planes
    WORD        wBitCount;       // Bits per pixel
    DWORD       dwBytesInRes;    // How many bytes in this resource?
    DWORD       dwImageOffset;   // Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;
#define sizeof_ICONDIRENTRY 16

typedef struct
{
    WORD           idReserved;   // Reserved (must be 0)
    WORD           idType;       // Resource Type (1 for icons)
    WORD           idCount;      // How many images?
//    ICONDIRENTRY   idEntries[]; // An entry for each image (idCount of 'em)
} ICONDIR, *LPICONDIR;
#define sizeof_ICONDIR 6

typedef struct
{
   BYTE   bWidth;               // Width, in pixels, of the image
   BYTE   bHeight;              // Height, in pixels, of the image
   BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
   BYTE   bReserved;            // Reserved
   WORD   wPlanes;              // Color Planes
   WORD   wBitCount;            // Bits per pixel
   DWORD   dwBytesInRes;         // how many bytes in this resource?
   WORD   nID;                  // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;
#define sizeof_GRPICONDIRENTRY 14

typedef struct 
{
   WORD            idReserved;   // Reserved (must be 0)
   WORD            idType;       // Resource type (1 for icons)
   WORD            idCount;      // How many images?
   GRPICONDIRENTRY   idEntries[1]; // The entries for each image
} GRPICONDIR, *LPGRPICONDIR;
#define sizeof_GRPICONDIR 6

////////////////////////////////////////////////////////////////////////////////

// These methods swap the little-endian values we need/find in a windows exe
// to host byte-order. Obviously they will be trivial on little-endian
// architectures.

static inline void swap_uint16(uint16_t& x)
{
#ifdef __BIG_ENDIAN__
	x = ((x & 0xff) << 8) | ((x >> 8) & 0xff);
#endif
}

static inline uint16_t swapped_uint16(uint16_t x)
{
#ifdef __BIG_ENDIAN__
	return((x & 0xff) << 8) | ((x >> 8) & 0xff);
#else
	return x;
#endif
}

static inline void swap_uint16s(uint16_t *x, uint32_t n)
{
#ifdef __BIG_ENDIAN__
	for(uint32_t i = 0; i < n; i++)
		swap_uint16(x[i]);
#endif
}

static inline void swap_uint32(uint32_t& x)
{
#ifdef __BIG_ENDIAN__
	x = ((x >> 24) & 0xff) | ((x >> 8) & 0xff00) | ((x & 0xff00) << 8) | ((x & 0xff) << 24);
#endif
}

// Now, in theory (for byte-swapping at least) DWORD == long == uint32. However
// reference parameters need to be of the exact same base type (even if there
// are trivial conversions) so we make separate routines. Note that rather
// than replicate the swap, we convert swap and convert back - this stops
// signedness issues and also will stop 'type-punned' pointer issues if we
// were to try and do it by pulling the values through *(<type> *)&v type
// patterns.
static inline void swap_dword(DWORD& x)
{
#ifdef __BIG_ENDIAN__
	uint32_t y;
	y = x;
	swap_uint32(y);
	x = y;
#endif
}

static inline void swap_long(LONG& x)
{
#ifdef __BIG_ENDIAN__
	uint32_t y;
	y = x;
	swap_uint32(y);
	x = y;
#endif
}

// This is a simple pattern based endian swap routine. The format string f consists of
// a string of 'l', 's', 'b', ' ' or any combination. Spaces are ignored, 'b' causes the
// next byte of data to be skipped, 's' causes the next two bytes to be swapped, and 'l'
// the next four.
static inline void swap_format(const char *f, void *p, uint32_t s)
{
	MCDeployByteSwapRecord(false, f, p, s);
}

// These routines swap the PE structures we need. Note that we don't bother
// wrapping these in __BIG_ENDIAN__ switches, since (hopefully) even the poorest
// of optimizers will notice that the functions they are calling are no-ops :o)

template<typename DeployPlatformTrait>
static inline void swap_IMAGE_NT_HEADERS(typename DeployPlatformTrait::IMAGE_NT_HEADERS& x)
{
	DeployPlatformTrait::swap_IMAGE_NT_HEADERS(x);
}

static inline void swap_IMAGE_DOS_HEADER(IMAGE_DOS_HEADER& x)
{
	swap_uint16s((uint16_t *)&x, 30);
	swap_long(x . e_lfanew);
}

static inline void swap_IMAGE_SECTION_HEADER(IMAGE_SECTION_HEADER& x)
{
	// MW-2009-07-14: Incorrect format for record - extra 'l' before pair of 's'
	swap_format("bbbbbbbbllllllssl", &x, sizeof(IMAGE_SECTION_HEADER));
}

static inline void swap_IMAGE_RESOURCE_DATA_ENTRY(IMAGE_RESOURCE_DATA_ENTRY& x)
{
	swap_format("llll", &x, sizeof(IMAGE_RESOURCE_DATA_ENTRY));
}

static inline void swap_IMAGE_RESOURCE_DIRECTORY(IMAGE_RESOURCE_DIRECTORY& x)
{
	swap_format("llssss", &x, sizeof(IMAGE_RESOURCE_DIRECTORY));
}

static inline void swap_IMAGE_RESOURCE_DIRECTORY_ENTRY(IMAGE_RESOURCE_DIRECTORY_ENTRY& x)
{
	swap_dword(x . Name);
	swap_dword(x . OffsetToData);
}

static inline void swap_ICONDIR(ICONDIR& x)
{
	swap_format("sss", &x, sizeof_ICONDIR);
}

static inline void swap_ICONDIRENTRY(ICONDIRENTRY& x)
{
	swap_format("bbbbssll", &x, sizeof_ICONDIRENTRY);
}

static inline void swap_GRPICONDIR(GRPICONDIR& x)
{
	swap_format("sss", &x, sizeof_GRPICONDIR);
}

static inline void swap_GRPICONDIRENTRY(GRPICONDIRENTRY& x)
{
	swap_format("bbbbssls", &x, sizeof_GRPICONDIRENTRY);
}

////////////////////////////////////////////////////////////////////////////////
//
// This section contains methods for operating on a Windows PE resource tree.
//

struct MCWindowsPE32Traits
{
	typedef IMAGE_OPTIONAL_HEADER32             IMAGE_OPTIONAL_HEADER;
	typedef PIMAGE_OPTIONAL_HEADER32            PIMAGE_OPTIONAL_HEADER;

	typedef IMAGE_NT_HEADERS32                  IMAGE_NT_HEADERS;
	typedef PIMAGE_NT_HEADERS32                 PIMAGE_NT_HEADERS;

	static inline void swap_IMAGE_NT_HEADERS(IMAGE_NT_HEADERS& x)
	{
		swap_format("l sslllss", &x, FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader));
		swap_format("sbbllllll lllssssssllllssllllll ll ll ll ll ll ll ll ll ll ll ll ll ll ll ll ll", &x.OptionalHeader, x.FileHeader.SizeOfOptionalHeader);
	}
};

struct MCWindowsPE64Traits
{
	typedef IMAGE_OPTIONAL_HEADER64             IMAGE_OPTIONAL_HEADER;
	typedef PIMAGE_OPTIONAL_HEADER64			PIMAGE_OPTIONAL_HEADER;

	typedef IMAGE_NT_HEADERS64                  IMAGE_NT_HEADERS;
	typedef PIMAGE_NT_HEADERS64                 PIMAGE_NT_HEADERS;

	static inline void swap_IMAGE_NT_HEADERS(IMAGE_NT_HEADERS& x)
	{
		swap_format("l sslllss", &x, FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader));
		swap_format("sbblllll qllssssssllllssqqqqll ll ll ll ll ll ll ll ll ll ll ll ll ll ll ll ll", &x.OptionalHeader, x.FileHeader.SizeOfOptionalHeader);
	}
};

struct MCWindowsResources
{
	// The id of the resource
	uint32_t id;

	// The name of the resource - note either an id or a name is valid, but
	// not both. An unnamd entry wil have a NULL name pointer.
	uint32_t name_length;
	uint16_t *name;

	// Whether this is a directory or entry
	bool is_table;
	union
	{
		// FG-2014-09-17: [[ Bugfix 13463 ]]
        // The members of this union should be aligned with similarly-sized
        // fields in order to prevent issues on 64-bit systems (in particular,
        // a bool should not be lined up with a pointer as compilers are allowed
        // to write anything they like into the high-order bytes).
        struct
		{
			uint32_t entry_count;
            uint32_t _pad_codepage;     // PADDING
            bool _pad_in_file;          // PADDING
			MCWindowsResources *entries;
		} table;

		struct
		{
			// The size of the data attached to the resource.
			uint32_t size;

			// The codepage of the data
			uint32_t codepage;

			// Whether the data resides in the source file.
			bool in_file;

			// Either the offset into the source file of the data, or the pointer
			// to the data if new. This depends on data_in_file.
			union
			{
				uint32_t offset;
				void *buffer;
			};
		} data;
	};
};

static void MCWindowsResourcesInitialize(MCWindowsResources& self)
{
	memset(&self, 0, sizeof(MCWindowsResources));
}

static void MCWindowsResourcesFinalize(MCWindowsResources& self);
static void MCWindowsResourcesFinalizeData(MCWindowsResources& self)
{
	if (self . is_table)
	{
		for(uint32_t i = 0; i < self . table . entry_count; i++)
			MCWindowsResourcesFinalize(self . table . entries[i]);
		delete[] self . table . entries;

		self . is_table = false;
	}
	else if (!self . data . in_file)
		free(self . data . buffer);

	self . data . size = 0;
	self . data . codepage = 0;
	self . data . in_file = false;
	self . data . buffer = NULL;
}

static void MCWindowsResourcesFinalize(MCWindowsResources& self)
{
	if (self . name != NULL)
		delete[] self . name;

	MCWindowsResourcesFinalizeData(self);
}

static bool MCWindowsResourcesFind(MCWindowsResources& self, uint32_t p_id, MCWindowsResources*& r_res)
{
	if (!self . is_table)
	{
		MCWindowsResourcesFinalizeData(self);
		self . is_table = true;
		self . table . entry_count = 0;
	}

	for(uint32_t i = 0; i < self . table . entry_count; i++)
		if (self . table . entries[i] . id == p_id)
		{
			r_res = &self . table . entries[i];
			return true;
		}

	MCWindowsResources *t_new_entries;
	t_new_entries = (MCWindowsResources *)realloc(self . table . entries, sizeof(MCWindowsResources) * (self . table . entry_count + 1));
	if (t_new_entries == NULL)
		return MCDeployThrow(kMCDeployErrorNoMemory);

	self . table . entries = t_new_entries;
	self . table . entry_count += 1;

	MCWindowsResourcesInitialize(self . table . entries[self . table . entry_count - 1]);
	self . table . entries[self . table . entry_count - 1] . id = p_id;

	r_res = &self . table . entries[self . table . entry_count - 1];

	return true;
}

static void MCWindowsResourcesSet(MCWindowsResources& self, void *p_data, uint32_t p_size)
{
	MCWindowsResourcesFinalizeData(self);

	self . is_table = false;
	self . data . size = p_size;
	self . data . in_file = false;
	self . data . codepage = 0;
	self . data . buffer = p_data;
}

////////////////////////////////////////////////////////////////////////////////

// This method cleans out any existing icon resources in the exe
static void MCWindowsResourcesClearIcons(MCWindowsResources& self)
{
	MCWindowsResources *t_branch;
	if (MCWindowsResourcesFind(self, 3, t_branch))
	{
		MCWindowsResourcesFinalizeData(*t_branch);
		t_branch -> is_table = true;
		t_branch -> table . entry_count = 0;
	}

	if (MCWindowsResourcesFind(self, 14, t_branch))
	{
		MCWindowsResourcesFinalizeData(*t_branch);
		t_branch -> is_table = true;
		t_branch -> table . entry_count = 0;
	}
}

// This method inserts the given icon file into a set of resources using the given id.
static bool MCWindowsResourcesAddIcon(MCWindowsResources& self, MCStringRef p_icon_file, uint32_t p_id, uint32_t p_culture_id)
{
	bool t_success;
	t_success = true;

	// First thing to do is try to and open the icon file
	MCDeployFileRef t_icon;
	t_icon = NULL;
	if (t_success)
		t_success = MCDeployFileOpen(p_icon_file, kMCOpenFileModeRead, t_icon);

	// Next read the header - care here to ensure correct structure size
	ICONDIR t_dir;
	if (t_success)
		t_success = MCDeployFileRead(t_icon, &t_dir, sizeof_ICONDIR);

	if (t_success)
		swap_ICONDIR(t_dir);

	// Now read in the entries - care here to ensure correct structure size
	ICONDIRENTRY *t_entries;
	t_entries = NULL;
	if (t_success)
	{
		t_entries = new (nothrow) ICONDIRENTRY[t_dir . idCount];
		if (t_entries == NULL)
			t_success = MCDeployThrow(kMCDeployErrorNoMemory);
	}

	if (t_success)
		for(uint32_t i = 0; i < t_dir . idCount && t_success; i++)
		{
			t_success = MCDeployFileRead(t_icon, &t_entries[i], sizeof_ICONDIRENTRY);
			if (t_success)
				swap_ICONDIRENTRY(t_entries[i]);
		}

	// Now we construct the resource hierarchy - first get the icon dir so
	// we know what ids to use.
	MCWindowsResources *t_icon_dir;
	if (t_success)
		t_success = MCWindowsResourcesFind(self, 3, t_icon_dir);

	uint32_t t_last_icon_id;
	t_last_icon_id = 0;
	if (t_success && t_icon_dir -> is_table)
		for(uint32_t i = 0; i < t_icon_dir -> table . entry_count; i++)
			t_last_icon_id = MCU_max(t_icon_dir -> table . entries[i] . id, t_last_icon_id);

	// And now get the grpicon dir
	MCWindowsResources *t_grpicon_dir;
	if (t_success)
		t_success = MCWindowsResourcesFind(self, 14, t_grpicon_dir);

	// First lets add in the group icon resource - we will overwrite one if already
	// present with the same id.
	MCWindowsResources *t_grpicon;
	if (t_success)
		t_success =
			MCWindowsResourcesFind(*t_grpicon_dir, p_id, t_grpicon) &&
			MCWindowsResourcesFind(*t_grpicon, p_culture_id, t_grpicon);

	// We have our grpicon resource leaf, so now we construct the grpicon data
	// and set.
	uint8_t *t_grpicon_data;
	t_grpicon_data = NULL;
	if (t_success)
	{
		t_grpicon_data = new (nothrow) uint8_t[sizeof_GRPICONDIR + sizeof_GRPICONDIRENTRY * t_dir . idCount];
		if (t_grpicon_data == NULL)
			t_success = MCDeployThrow(kMCDeployErrorNoMemory);
	}

	if (t_success)
	{
		uint8_t *t_ptr;
		t_ptr = t_grpicon_data;

		GRPICONDIR t_grp_dir;
		t_grp_dir . idReserved = 0;
		t_grp_dir . idType = 1;
		t_grp_dir . idCount = t_dir . idCount;

		swap_GRPICONDIR(t_grp_dir);
		memcpy(t_ptr, &t_grp_dir, sizeof_GRPICONDIR);
		t_ptr += sizeof_GRPICONDIR;

		for(uint32_t i = 0; i < t_dir . idCount; i++)
		{
			GRPICONDIRENTRY t_grp_entry;
			t_grp_entry . bWidth = t_entries[i] . bWidth;
			t_grp_entry . bHeight = t_entries[i] . bHeight;
			t_grp_entry . bColorCount = t_entries[i] . bColorCount;
			t_grp_entry . bReserved = 0;
			t_grp_entry . wPlanes = t_entries[i] . wPlanes;
			t_grp_entry . wBitCount = t_entries[i] . wBitCount;
			t_grp_entry . dwBytesInRes = t_entries[i] . dwBytesInRes;
			t_grp_entry . nID = t_last_icon_id + i + 1;
			swap_GRPICONDIRENTRY(t_grp_entry);
			memcpy(t_ptr, &t_grp_entry, sizeof_GRPICONDIRENTRY);
			t_ptr += sizeof_GRPICONDIRENTRY;
		}

		// Note: Set takes ownership of the given buffer
		MCWindowsResourcesSet(*t_grpicon, t_grpicon_data, sizeof_GRPICONDIR + sizeof_GRPICONDIRENTRY * t_dir . idCount);
		t_grpicon_data = NULL;
	}

	// Now we've constructed the grpicon resource, we must add the icons
	if (t_success)
		for(uint32_t i = 0; i < t_dir . idCount && t_success; i++)
		{
			// First allocate memory and load the image data
			uint8_t *t_image;
			t_image = new (nothrow) uint8_t[t_entries[i] . dwBytesInRes];
			if (t_image != NULL)
				t_success = MCDeployFileReadAt(t_icon, t_image, t_entries[i] . dwBytesInRes, t_entries[i] . dwImageOffset);

			// Now try to create a new resource
			MCWindowsResources *t_icon_res;
			if (t_success)
				t_success = MCWindowsResourcesFind(*t_icon_dir, t_last_icon_id + i + 1, t_icon_res) &&
					MCWindowsResourcesFind(*t_icon_res, p_culture_id, t_icon_res);

			if (t_success)
			{
				MCWindowsResourcesSet(*t_icon_res, t_image, t_entries[i] . dwBytesInRes);
				t_image = NULL;
			}

			delete[] t_image;
		}

	delete[] t_grpicon_data;
	delete[] t_entries;

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

struct MCWindowsVersionInfo
{
	MCWindowsVersionInfo *next;
	bool is_text;
	char *key;
	void *value;
	uint32_t value_length;
	MCWindowsVersionInfo *children;
};

static bool MCWindowsVersionInfoAdd(MCWindowsVersionInfo *p_parent, const char *p_key, bool p_is_text, const void *p_value, uint32_t p_value_length, MCWindowsVersionInfo*& r_child)
{
	bool t_success;
	t_success = true;

	MCWindowsVersionInfo *t_child;
	t_child = NULL;
	if (t_success)
	{
		t_child = new (nothrow) MCWindowsVersionInfo;
		if (t_child == NULL)
			t_success = MCDeployThrow(kMCDeployErrorNoMemory);
	}

	void *t_value;
	t_value = NULL;
	if (t_success && p_value != NULL)
	{
		t_value = malloc(p_value_length);
		if (t_value != NULL)
			memcpy(t_value, p_value, p_value_length);
		else
			t_success = MCDeployThrow(kMCDeployErrorNoMemory);
	}

	char *t_key;
	t_key = NULL;
	if (t_success)
	{
		t_key = strdup(p_key);
		if (t_key == NULL)
			t_success = MCDeployThrow(kMCDeployErrorNoMemory);
	}

	if (t_success)
	{
		t_child -> is_text = p_is_text;
		t_child -> key = t_key;
		t_child -> value = t_value;
		t_child -> value_length = p_value_length;
		t_child -> next = NULL;
		t_child -> children = NULL;
		if (p_parent != NULL)
		{
			t_child -> next = p_parent -> children;
			p_parent -> children = t_child;
		}

		r_child = t_child;
	}
	else
	{
		free(t_key);
		free(t_value);
		delete t_child;
	}

	return t_success;
}

static void MCWindowsVersionInfoMeasure(MCWindowsVersionInfo *self, uint32_t& x_size)
{
	// General structure is:
	//   WORD length
	//   WORD valuelength
	//   WORD type
	//   WORD key[]
	//   if value then
	//     (pad to 32-bit)
	//     (value)
	//   if children then
	//     (pad to 32-bit)
	//     (children)

	x_size += sizeof(WORD) * 3;

	x_size += sizeof(WORD) * (strlen(self -> key) + 1);

	if (self -> value != NULL)
	{
		x_size = (x_size + 3) & ~3;
		x_size += self -> value_length;
	}

	if (self -> children != NULL)
	{
		for(MCWindowsVersionInfo *t_child = self -> children; t_child != NULL; t_child = t_child -> next)
		{
			x_size = (x_size + 3) & ~3;
			MCWindowsVersionInfoMeasure(t_child, x_size);
		}
	}
}

static void MCWindowsVersionInfoBuild(MCWindowsVersionInfo *self, void *p_buffer, uint32_t& x_offset)
{
	// Calculate the size of the record by measuring from the current offset.
	uint32_t t_offset_after;
	t_offset_after = x_offset;
	MCWindowsVersionInfoMeasure(self, t_offset_after);

	WORD t_length, t_value_length, t_type;
	t_length = swapped_uint16(t_offset_after - x_offset);
	t_value_length = swapped_uint16(self -> is_text ? self -> value_length / 2 : self -> value_length);
	t_type = swapped_uint16(self -> is_text ? 1 : 0);

	uint8_t *t_buffer;
	t_buffer = (uint8_t *)p_buffer;

	// Header
	memcpy(t_buffer + x_offset, &t_length, sizeof(WORD));
	x_offset += sizeof(WORD);
	memcpy(t_buffer + x_offset, &t_value_length, sizeof(WORD));
	x_offset += sizeof(WORD);
	memcpy(t_buffer + x_offset, &t_type, sizeof(WORD));
	x_offset += sizeof(WORD);
	for(uint32_t i = 0; i < strlen(self -> key) + 1; i++)
	{
		WCHAR t_char;
		t_char = swapped_uint16(self -> key[i]);
		memcpy(t_buffer + x_offset, &t_char, sizeof(WORD));
		x_offset += sizeof(WORD);
	}

	// Value
	if (self -> value != NULL)
	{
		x_offset = (x_offset + 3) & ~3;
		memcpy(t_buffer + x_offset, self -> value, self -> value_length);
		x_offset += self -> value_length;
	}

	// Children
	if (self -> children != NULL)
	{
		for(MCWindowsVersionInfo *t_child = self -> children; t_child != NULL; t_child = t_child -> next)
		{
			x_offset = (x_offset + 3) & ~3;
			MCWindowsVersionInfoBuild(t_child, t_buffer, x_offset);
		}
	}
}

static void MCWindowsVersionInfoDestroy(MCWindowsVersionInfo *self)
{
	free(self -> key);
	free(self -> value);
	while(self -> children != NULL)
	{
		MCWindowsVersionInfo *t_child;
		t_child = self -> children;
		self -> children = self -> children -> next;
		MCWindowsVersionInfoDestroy(t_child);
	}
	delete self;
}

static uint64_t MCWindowsVersionInfoParseVersion(MCStringRef p_string)
{
	uint32_t a, b, c, d;
    MCAutoStringRefAsUTF8String t_string_utf8;
    /* UNCHECKED */ t_string_utf8 . Lock(p_string);
	if (sscanf(*t_string_utf8, "%u.%u.%u.%u", &a, &b, &c, &d) != 4)
		return 0;
	return 0ULL | ((uint64_t)a << 48) | ((uint64_t)b << 32) | (c << 16) | d;
}

static bool add_version_info_entry(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
    // If there is no context, then we have nothing to add the entry to.
    if (p_context == nil)
        return true;
    
    MCExecContext ctxt(nil, nil, nil);
	MCAutoStringRef t_value;
	if (!ctxt . ConvertToString(p_value, &t_value))
        return false;
    
	MCAutoArray<byte_t> t_bytes;
	if (!MCStringConvertToBytes(*t_value, kMCStringEncodingUTF16LE, false, t_bytes . PtrRef(), t_bytes . SizeRef()))
        return false;
    
    // FG-2014-09-17: [[ Bugfix 13463 ]] Convert may return 0 bytes for the empty string
	if (t_bytes . Size() == 0 || t_bytes[t_bytes . Size() - 1] != '\0' || t_bytes[t_bytes . Size() - 2] != '\0')
	{
        if (!t_bytes . Push('\0') ||
            !t_bytes . Push('\0'))
            return false;
	}

    MCAutoStringRefAsCString t_key_str;
    if (!t_key_str.Lock(MCNameGetString(p_key)))
        return false;

    MCWindowsVersionInfo *t_string;
    return MCWindowsVersionInfoAdd((MCWindowsVersionInfo *)p_context, *t_key_str, true, t_bytes . Ptr(), t_bytes . Size(), t_string);
}

static bool MCWindowsResourcesAddVersionInfo(MCWindowsResources& self, MCArrayRef p_info)
{
    MCExecContext ctxt(nil, nil, nil);

	bool t_success;
	t_success = true;

	uint64_t t_file_version, t_product_version;
	t_file_version = t_product_version = 0;
	if (t_success)
	{
        MCValueRef t_value;
            
        if (MCArrayFetchValue(p_info, false, MCNAME("FileVersion"), t_value))
		{
			MCAutoStringRef t_string;
			/* UNCHECKED */ ctxt . ConvertToString(t_value, &t_string);
            t_file_version = MCWindowsVersionInfoParseVersion(*t_string); 
		}
		if (MCArrayFetchValue(p_info, false, MCNAME("ProductVersion"), t_value))
		{
			MCAutoStringRef t_string;
			/* UNCHECKED */ ctxt . ConvertToString(t_value, &t_string);
            t_product_version = MCWindowsVersionInfoParseVersion(*t_string);
		}
	}
	
	MCWindowsVersionInfo *t_version_info;
	t_version_info = NULL;
	if (t_success)
	{
		VS_FIXEDFILEINFO t_fixed_info;
		t_fixed_info . dwSignature = 0xFEEF04BD;
		t_fixed_info . dwStrucVersion = 0x00010000;
		t_fixed_info . dwFileFlagsMask = 0x3f;
		t_fixed_info . dwFileFlags = 0x28;
		t_fixed_info . dwFileOS = 0x40004;
		t_fixed_info . dwFileType = 0x1;
		t_fixed_info . dwFileSubtype = 0x0;
		t_fixed_info . dwFileDateMS = 0;
		t_fixed_info . dwFileDateLS = 0;
		t_fixed_info . dwFileVersionMS = (uint32_t)(t_file_version >> 32);
		t_fixed_info . dwFileVersionLS = (uint32_t)(t_file_version & 0xffffffff);
		t_fixed_info . dwProductVersionMS = (uint32_t)(t_product_version >> 32);
		t_fixed_info . dwProductVersionLS = (uint32_t)(t_product_version & 0xffffffff);
		MCDeployByteSwapRecord(false, "lllllllllllll", &t_fixed_info, sizeof(VS_FIXEDFILEINFO));
		t_success = MCWindowsVersionInfoAdd(NULL, "VS_VERSION_INFO", false, &t_fixed_info, sizeof(VS_FIXEDFILEINFO), t_version_info);
	}

	MCWindowsVersionInfo *t_var_file_info;
	if (t_success)
	{
		uint32_t t_var_info;
		t_var_info = (1200 << 16) | 0x0409;
		MCDeployByteSwap32(false, t_var_info);
		t_success =
			MCWindowsVersionInfoAdd(t_version_info, "VarFileInfo", true, NULL, 0, t_var_file_info) && 
			MCWindowsVersionInfoAdd(t_var_file_info, "Translation", false, &t_var_info, sizeof(DWORD), t_var_file_info);
	}

	MCWindowsVersionInfo *t_string_file_info;
	if (t_success)
		t_success = MCWindowsVersionInfoAdd(t_version_info, "StringFileInfo", true, NULL, 0, t_string_file_info);

	MCWindowsVersionInfo *t_string_table;
	if (t_success)
		t_success = MCWindowsVersionInfoAdd(t_string_file_info, "040904b0", true, NULL, 0, t_string_table);

	if (t_success)
		t_success = MCArrayApply(p_info, add_version_info_entry, t_string_table);

	void *t_data;
	uint32_t t_data_size;
	t_data = NULL;
	if (t_success)
	{
		t_data_size = 0;
		MCWindowsVersionInfoMeasure(t_version_info, t_data_size);
		t_data = malloc(t_data_size);
		if (t_data != NULL)
		{
			uint32_t t_offset;
			t_offset = 0;
			memset(t_data, 0, t_data_size);
			MCWindowsVersionInfoBuild(t_version_info, t_data, t_offset);
		}
		else
			t_success = MCDeployThrow(kMCDeployErrorNoMemory);
	}

	MCWindowsResources *t_resource;
	t_resource = NULL;
	if (t_success)
		t_success =
			MCWindowsResourcesFind(self, 16, t_resource) &&
			MCWindowsResourcesFind(*t_resource, 1, t_resource) &&
			MCWindowsResourcesFind(*t_resource, 0x0409, t_resource);

	if (t_success)
		MCWindowsResourcesSet(*t_resource, t_data, t_data_size);
	else
		free(t_data);

	MCWindowsVersionInfoDestroy(t_version_info);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static bool MCWindowsResourcesAddManifest(MCWindowsResources& self, MCStringRef p_manifest_path)
{
	bool t_success;
	t_success = true;

	// First thing to do is try to and open the manifest file
	MCDeployFileRef t_manifest;
	t_manifest = NULL;
	if (t_success)
		t_success = MCDeployFileOpen(p_manifest_path, kMCOpenFileModeRead, t_manifest);

	// Measure the manifest
	uint32_t t_size;
	if (t_success)
		t_success = MCDeployFileMeasure(t_manifest, t_size);

	// Allocate the memory for the manifest
	void *t_data;
	t_data = NULL;
	if (t_success)
	{
		t_data = malloc(t_size);
		if (t_data == NULL)
			t_success = MCDeployThrow(kMCDeployErrorNoMemory);
	}

	// Read it in
	if (t_success)
		t_success = MCDeployFileRead(t_manifest, t_data, t_size);

	// Find the relevant resource leaf
	MCWindowsResources *t_leaf;
	if (t_success)
		t_success = MCWindowsResourcesFind(self, 24, t_leaf) &&
			MCWindowsResourcesFind(*t_leaf, 1, t_leaf) &&
			MCWindowsResourcesFind(*t_leaf, 0x0409, t_leaf);

	// Add the resource
	if (t_success)
		MCWindowsResourcesSet(*t_leaf, t_data, t_size);
	else
		free(t_data);

	MCDeployFileClose(t_manifest);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static bool MCWindowsReadResourceEntryName(MCDeployFileRef p_file, uint32_t p_start, uint32_t p_size, uint32_t p_at, MCWindowsResources& r_entry)
{
	// A resource directory string consists of a length followed by a sequence of uint16s.
	uint16_t t_length;
	if (!MCDeployFileReadAt(p_file, &t_length, sizeof(uint16_t), p_start + p_at))
		return false;

	p_at += sizeof(uint16_t);
	swap_uint16(t_length);
	r_entry . name_length = t_length;

	r_entry . name = new (nothrow) uint16_t[r_entry . name_length];
	if (r_entry . name == NULL)
		return MCDeployThrow(kMCDeployErrorNoMemory);

	if (!MCDeployFileReadAt(p_file, r_entry . name, sizeof(uint16_t) * r_entry . name_length, p_start + p_at))
		return false;

	swap_uint16s(r_entry . name, r_entry . name_length);

	return true;
}

static bool MCWindowsReadResourceEntryData(MCDeployFileRef p_file, uint32_t p_address, uint32_t p_start, uint32_t p_size, uint32_t p_at, MCWindowsResources& r_entry)
{
	IMAGE_RESOURCE_DATA_ENTRY t_data;
	if (!MCDeployFileReadAt(p_file, &t_data, sizeof(IMAGE_RESOURCE_DATA_ENTRY), p_start + p_at))
		return false;

	p_at += sizeof(IMAGE_RESOURCE_DATA_ENTRY);
	swap_IMAGE_RESOURCE_DATA_ENTRY(t_data);

	r_entry . data . in_file = true;
	r_entry . data . codepage = t_data . CodePage;
	r_entry . data . size = t_data . Size;
	r_entry . data . offset = p_start + t_data . OffsetToData - p_address;

	return true;
}

static bool MCWindowsReadResourceDir(MCDeployFileRef p_file, uint32_t p_address, uint32_t p_start, uint32_t p_size, uint32_t p_at, MCWindowsResources& r_resources)
{
	IMAGE_RESOURCE_DIRECTORY t_dir;
	if (!MCDeployFileReadAt(p_file, &t_dir, sizeof(IMAGE_RESOURCE_DIRECTORY), p_start + p_at))
		return false;

	p_at += sizeof(IMAGE_RESOURCE_DIRECTORY);
	swap_IMAGE_RESOURCE_DIRECTORY(t_dir);

	// Make sure we have enough room in the table.
	r_resources . is_table = true;
	r_resources . table . entry_count = t_dir . NumberOfIdEntries + t_dir . NumberOfNamedEntries;
	r_resources . table . entries = new (nothrow) MCWindowsResources[r_resources . table . entry_count];
	if (r_resources . table . entries == NULL)
		return MCDeployThrow(kMCDeployErrorNoMemory);

	for(uint32_t i = 0; i < r_resources . table . entry_count; i++)
		MCWindowsResourcesInitialize(r_resources . table . entries[i]);

	// Now load the entries
	for(uint32_t i = 0; i < r_resources . table . entry_count; i++)
	{
		// Read in the new entry from disk
		IMAGE_RESOURCE_DIRECTORY_ENTRY t_entry;
		if (!MCDeployFileReadAt(p_file, &t_entry, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY), p_start + p_at))
			return false;

		// Advance our current input pointer
		p_at += sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);

		// Swap to host byte order.
		swap_IMAGE_RESOURCE_DIRECTORY_ENTRY(t_entry);

		if (i < t_dir . NumberOfNamedEntries)
		{
			// Get the name, if it is named

			if (!MCWindowsReadResourceEntryName(p_file, p_start, p_size, t_entry . NameOffset, r_resources . table . entries[i]))
				return false;
		}
		else
		{
			// Otherwise it has an id

			r_resources . table . entries[i] . id = t_entry . Name;
		}

		if (t_entry . DataIsDirectory)
		{
			// If the entry is a directory, we recurse
			if (!MCWindowsReadResourceDir(p_file, p_address, p_start, p_size, t_entry . OffsetToDirectory, r_resources . table . entries[i]))
				return false;
		}
		else
		{
			// This entry is not a table, so mark it as such.
			r_resources . table . entries[i] . is_table = false;

			// We need to read the data entry
			if (!MCWindowsReadResourceEntryData(p_file, p_address, p_start, p_size, t_entry . OffsetToData, r_resources . table . entries[i]))
				return false;
		}
	}

	return true;
}

static bool MCWindowsResourcesRead(MCDeployFileRef p_file, uint32_t p_address, uint32_t p_start, uint32_t p_size, MCWindowsResources& r_resources)
{
	MCWindowsResourcesInitialize(r_resources);
	return MCWindowsReadResourceDir(p_file, p_address, p_start, p_size, 0, r_resources);
}

static void MCWindowsResourcesMeasureDir(MCWindowsResources& self, uint32_t& x_header, uint32_t& x_string, uint32_t& x_desc, uint32_t& x_data)
{
	// The header record for the dir
	x_header += sizeof(IMAGE_RESOURCE_DIRECTORY);

	// One entry record per entry
	x_header += sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) * self . table . entry_count;

	// Now process each entry
	for(uint32_t i = 0; i < self . table . entry_count; i++)
	{
		MCWindowsResources& t_entry = self . table . entries[i];

		// If the entry is named - make room for that.
		if (t_entry . name != NULL)
			x_string += 1 + self . name_length;

		// Now add size depending on type
		if (t_entry . is_table)
			MCWindowsResourcesMeasureDir(t_entry, x_header, x_string, x_desc, x_data);
		else
		{
			x_desc += sizeof(IMAGE_RESOURCE_DATA_ENTRY);
			x_data += (t_entry . data . size + 7) & ~7;
		}
	}
}

static uint32_t MCWindowsResourcesMeasure(MCWindowsResources& self)
{
	uint32_t t_header_size, t_string_size, t_desc_size, t_data_size;
	t_header_size = t_string_size = t_desc_size = t_data_size = 0;

	// Work out the various sizes of the section pieces
	MCWindowsResourcesMeasureDir(self, t_header_size, t_string_size, t_desc_size, t_data_size);

	// Now round up the string size
	t_string_size = (t_string_size + 7) & ~7;

	// Compute the total size
	uint32_t t_size;
	t_size = t_header_size + t_string_size + t_desc_size + t_data_size;

	// And round up
	t_size = (t_size + 4095) & ~4095;

	return t_size;
}

static bool MCWindowsResourcesWriteDir(MCWindowsResources& self, uint32_t& x_header, uint32_t& x_string, uint32_t& x_desc, uint32_t& x_data, uint32_t& x_address, MCDeployFileRef p_dst, uint32_t p_dst_at, MCDeployFileRef p_src, uint32_t p_src_at)
{
	// First construct the directory header
	IMAGE_RESOURCE_DIRECTORY t_header;
	t_header . Characteristics = 0;
	t_header . TimeDateStamp = 0;
	t_header . MajorVersion = 0;
	t_header . MinorVersion = 0;
	t_header . NumberOfNamedEntries = 0;
	t_header . NumberOfIdEntries = 0;

	for(uint32_t i = 0; i < self . table . entry_count; i++)
		if (self . table . entries[i] . name != NULL)
			t_header . NumberOfNamedEntries += 1;
		else
			t_header . NumberOfIdEntries += 1;

	swap_IMAGE_RESOURCE_DIRECTORY(t_header);

	// Now we can write out the header
	if (!MCDeployFileWriteAt(p_dst, &t_header, sizeof(IMAGE_RESOURCE_DIRECTORY), p_dst_at + x_header))
		return false;

	// Now we need to keep track of our entry pointer, but need to advance the
	// header offset by the full size (including entry records).
	uint32_t t_header_ptr;
	t_header_ptr = x_header + sizeof(IMAGE_RESOURCE_DIRECTORY);

	x_header += sizeof(IMAGE_RESOURCE_DIRECTORY) + sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) * self . table . entry_count;

	// Now write out the individual entries
	for(uint32_t i = 0; i < self . table . entry_count; i++)
	{
		MCWindowsResources& t_entry = self . table . entries[i];
		IMAGE_RESOURCE_DIRECTORY_ENTRY t_entry_header;

		if (t_entry . name == NULL)
		{
			t_entry_header . Name = t_entry . id;
		}
		else
		{
			t_entry_header . NameIsString = true;
			t_entry_header . NameOffset = x_string;

			uint16_t t_length;
			t_length = t_entry . name_length;
			swap_uint16(t_length);

			if (!MCDeployFileWriteAt(p_dst, &t_length, sizeof(uint16_t), p_dst_at + x_string))
				return false;

			x_string += sizeof(uint16_t);

			swap_uint16s(t_entry . name, t_entry . name_length);
			if (!MCDeployFileWriteAt(p_dst, t_entry . name, sizeof(uint16_t) * t_entry . name_length, p_dst_at + x_string))
				return false;

			x_string += sizeof(uint16_t) * t_entry . name_length;
		}

		if (t_entry . is_table)
		{
			t_entry_header . DataIsDirectory = true;
			t_entry_header . OffsetToDirectory = x_header;

			if (!MCWindowsResourcesWriteDir(t_entry, x_header, x_string, x_desc, x_data, x_address, p_dst, p_dst_at, p_src, p_src_at))
				return false;
		}
		else
		{
			t_entry_header . OffsetToData = x_desc;

			IMAGE_RESOURCE_DATA_ENTRY t_data_entry;
			t_data_entry . CodePage = t_entry . data . codepage;
			t_data_entry . OffsetToData = x_address;
			t_data_entry . Reserved = 0;
			t_data_entry . Size = t_entry . data . size;

			swap_IMAGE_RESOURCE_DATA_ENTRY(t_data_entry);
			if (!MCDeployFileWriteAt(p_dst, &t_data_entry, sizeof(IMAGE_RESOURCE_DATA_ENTRY), p_dst_at + x_desc))
				return false;

			if (t_entry . data . in_file)
			{
				if (!MCDeployFileCopy(p_dst, p_dst_at + x_data, p_src, t_entry . data . offset, t_entry . data . size))
					return false;
			}
			else
			{
				if (!MCDeployFileWriteAt(p_dst, t_entry . data . buffer, t_entry . data . size, p_dst_at + x_data))
					return false;
			}

			x_desc += sizeof(IMAGE_RESOURCE_DATA_ENTRY);
			x_address += (t_entry . data . size + 7) & ~7;
			x_data += (t_entry . data . size + 7) & ~7;
		}

		swap_IMAGE_RESOURCE_DIRECTORY_ENTRY(t_entry_header);

		if (!MCDeployFileWriteAt(p_dst, &t_entry_header, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY), p_dst_at + t_header_ptr))
			return false;

		t_header_ptr += sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);
	}

	return true;
}

static bool MCWindowsResourcesWrite(MCWindowsResources& self, uint32_t p_address, MCDeployFileRef p_dst, uint32_t p_dst_at, MCDeployFileRef p_src, uint32_t p_src_at)
{
	// To write out the resources, we first measure
	uint32_t t_size, t_header_size, t_string_size, t_desc_size, t_data_size;
	t_header_size = t_string_size = t_desc_size = t_data_size = 0;
	MCWindowsResourcesMeasureDir(self, t_header_size, t_string_size, t_desc_size, t_data_size);
	t_string_size = (t_string_size + 7) & ~7;
	t_size = t_header_size + t_string_size + t_desc_size + t_data_size;
	t_size = (t_size + 4095) & ~4095;

	// This means we can work out he part offsets
	uint32_t t_header_offset, t_string_offset, t_desc_offset, t_data_offset;
	t_header_offset = 0;
	t_string_offset = t_header_offset + t_header_size;
	t_desc_offset = t_string_offset + t_string_size;
	t_data_offset = t_desc_offset + t_desc_size;

	p_address += t_data_offset;

	// Now we can write out the whole thing in one pass
	if (!MCWindowsResourcesWriteDir(self, t_header_offset, t_string_offset, t_desc_offset, t_data_offset, p_address, p_dst, p_dst_at, p_src, p_src_at))
		return false;

	// Now if the data_offset is not at the size needed, write a zero byte at the
	// last position to force padding.
	uint8_t t_zero;
	t_zero = 0;
	if (t_data_offset != t_size)
		if (!MCDeployFileWriteAt(p_dst, &t_zero, sizeof(uint8_t), p_dst_at + t_size - 1))
			return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// This section contains the actual deploy methods - they read in the parts of
// PE, appropriately munge it and write out a new one.
//

template<typename DeployPlatformTrait>
static bool MCDeployToWindowsReadHeaders(MCDeployFileRef p_file, IMAGE_DOS_HEADER& r_dos_header, typename DeployPlatformTrait::IMAGE_NT_HEADERS& r_nt_header, IMAGE_SECTION_HEADER*& r_section_headers)
{
	if (!MCDeployFileRead(p_file, &r_dos_header, sizeof(IMAGE_DOS_HEADER)))
		return MCDeployThrow(kMCDeployErrorWindowsNoDOSHeader);

	swap_IMAGE_DOS_HEADER(r_dos_header);

	if (r_dos_header . e_magic != IMAGE_DOS_SIGNATURE)
		return MCDeployThrow(kMCDeployErrorWindowsBadDOSSignature);

	if (!MCDeployFileSeekSet(p_file, r_dos_header . e_lfanew))
		return MCDeployThrow(kMCDeployErrorWindowsBadDOSHeader);

	if (!MCDeployFileRead(p_file, &r_nt_header, sizeof(typename DeployPlatformTrait::IMAGE_NT_HEADERS)))
		return MCDeployThrow(kMCDeployErrorWindowsNoNTHeader);

	DeployPlatformTrait::swap_IMAGE_NT_HEADERS(r_nt_header);

	if (r_nt_header . Signature != IMAGE_NT_SIGNATURE)
		return MCDeployThrow(kMCDeployErrorWindowsBadNTSignature);

	if (!MCDeployFileSeekSet(p_file, r_dos_header . e_lfanew + FIELD_OFFSET(typename DeployPlatformTrait::IMAGE_NT_HEADERS, OptionalHeader) + r_nt_header . FileHeader . SizeOfOptionalHeader))
		return MCDeployThrow(kMCDeployErrorWindowsBadSectionHeaderOffset);

	r_section_headers = new (nothrow) IMAGE_SECTION_HEADER[r_nt_header . FileHeader . NumberOfSections];
	if (r_section_headers == NULL)
		return MCDeployThrow(kMCDeployErrorNoMemory);

	if (!MCDeployFileRead(p_file, r_section_headers, sizeof(IMAGE_SECTION_HEADER) * r_nt_header . FileHeader . NumberOfSections))
		return MCDeployThrow(kMCDeployErrorWindowsNoSectionHeaders);

	for(uint32_t i = 0; i < r_nt_header . FileHeader . NumberOfSections; i++)
		swap_IMAGE_SECTION_HEADER(r_section_headers[i]);

	return true;
}

// This method attempts to build a Windows standalone using the given deployment
// parameters. The Windows executable format consists of:
//   DOS Header
//   NT Header
//   List of OS-used regions/sections
//   List of sections
//   Section data
//
// To build the standalone, we:
//   1) read in the precompiled standalone.exe
//   2) expand the extra sections within the headers to accomodate the new data
//   3) read in the resources section and augment with icons, manifest and info
//      as necessary
//   4) reconstruct the output executable from inputs
//
// It is assumed that the '.project' section immediately precedes the '.rsrc.
// section, and that (as is usual) the '.rsrc' section is at the end of the
// executable.
//
template<typename DeployPlatformTrait>
Exec_stat MCDeployToWindows(const MCDeployParameters& p_params)
{
	typedef typename DeployPlatformTrait::IMAGE_NT_HEADERS IMAGE_NT_HEADERS;

	bool t_success;
	t_success = true;

    // Are we running deploy just for the purpose of changing the EXE icons?
    bool t_icons_only = false;
    if (MCStringIsEmpty(p_params.stackfile) && !MCStringIsEmpty(p_params.app_icon))
        t_icons_only = true;
    
	// First thing to do is to open the files.
	MCDeployFileRef t_engine, t_output;
	t_engine = t_output = NULL;
	if (t_success && !MCDeployFileOpen(p_params . engine, kMCOpenFileModeRead, t_engine))
		t_success = MCDeployThrow(kMCDeployErrorNoEngine);
	if (t_success && !MCDeployFileOpen(p_params . output, kMCOpenFileModeCreate, t_output))
		t_success = MCDeployThrow(kMCDeployErrorNoOutput);

	// First load the headers we need
	IMAGE_DOS_HEADER t_dos_header;
	IMAGE_NT_HEADERS t_nt_header;
	IMAGE_SECTION_HEADER *t_section_headers;
	t_section_headers = NULL;
	if (t_success)
		t_success = MCDeployToWindowsReadHeaders<DeployPlatformTrait>(t_engine, t_dos_header, t_nt_header, t_section_headers);

	IMAGE_SECTION_HEADER *t_payload_section, *t_project_section, *t_resource_section;
    t_payload_section = t_project_section = t_resource_section = nil;
    
    
	uint32_t t_section_count;
    
    uint32_t t_output_offset = 0;
    uint32_t t_base_address = 0;
    
    bool t_swap_payload = false;
    if (t_success)
	{
		t_section_count = t_nt_header . FileHeader . NumberOfSections;
        
        IMAGE_SECTION_HEADER *t_temp_section;
        for (uint32_t t_index = 0; t_index < t_section_count; t_index++)
        {
            t_temp_section = &t_section_headers[t_index];
            
            if (memcmp(t_temp_section -> Name, ".payload", 8) == 0)
            {
                t_payload_section = t_temp_section;
                if (t_output_offset == 0)
                {
                    t_output_offset = t_temp_section -> PointerToRawData;
                    t_base_address = t_temp_section -> VirtualAddress;
                }
                else
                {
                    // payload is should be third last
                    t_swap_payload = true;
                }
            }
            else if (memcmp(t_temp_section -> Name, ".project", 8) == 0)
            {
                t_project_section = t_temp_section;
                if (t_output_offset == 0)
                {
                    t_output_offset = t_temp_section -> PointerToRawData;
                    t_base_address = t_temp_section -> VirtualAddress;
                }
            }
            else if (memcmp(t_temp_section -> Name, ".rsrc", 5) == 0)
            {
                t_resource_section = t_temp_section;
                if (t_output_offset == 0)
                {
                    t_output_offset = t_temp_section -> PointerToRawData;
                    t_base_address = t_temp_section -> VirtualAddress;
                }
            }
        }
    }
    
	// Next we check that there are at least two sections, and they are the
	// right ones.
	if (t_success && t_section_count < 2)
		t_success = MCDeployThrow(kMCDeployErrorWindowsMissingSections);
	if (t_success && t_resource_section == nil)
		t_success = MCDeployThrow(kMCDeployErrorWindowsNoResourceSection);
	if (t_success && !t_icons_only && t_project_section == nil)
		t_success = MCDeployThrow(kMCDeployErrorWindowsNoProjectSection);
	if (t_success && !MCStringIsEmpty(p_params . payload) && t_payload_section == nil)
		t_success = MCDeployThrow(kMCDeployErrorWindowsNoPayloadSection);
    
	// Read in the resources
	MCWindowsResources t_resources;
	MCWindowsResourcesInitialize(t_resources);
	if (t_success)
		t_success = MCWindowsResourcesRead(t_engine,
			t_nt_header . OptionalHeader . DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE] . VirtualAddress,
			t_resource_section -> PointerToRawData,
			t_resource_section -> SizeOfRawData,
			t_resources);

	// Augment the resources as appropriate
	
	// If we are setting both app and doc icons, clear out the existing
	// icon resources
	if (t_success && !MCStringIsEmpty(p_params . app_icon) && !MCStringIsEmpty(p_params . doc_icon))
		MCWindowsResourcesClearIcons(t_resources);

	if (t_success && !MCStringIsEmpty(p_params . app_icon))
	{
		t_success = MCWindowsResourcesAddIcon(t_resources, p_params . app_icon, 111, 0x0409);
		if (!t_success)
			t_success = MCDeployThrow(kMCDeployErrorWindowsBadAppIcon);
	}
	if (t_success && !MCStringIsEmpty(p_params . doc_icon))
	{
		t_success = MCWindowsResourcesAddIcon(t_resources, p_params . doc_icon, 112, 0x0409);
		if (!t_success)
			t_success = MCDeployThrow(kMCDeployErrorWindowsBadDocIcon);
	}

	// If there is a version info array, then build a VERSIONINFO resource and
	// add it.
	if (t_success && !MCArrayIsEmpty(p_params . version_info))
		t_success = MCWindowsResourcesAddVersionInfo(t_resources, p_params . version_info);

	// Add the manifest to the resources
	if (t_success && !MCStringIsEmpty(p_params . manifest))
		t_success = MCWindowsResourcesAddManifest(t_resources, p_params . manifest);

	// Now we have the various references we need and have prepared the
	// data to output. We write out the first part of the exe and then the main
	// project data-stream. This will then give us the necessary size info to
	// update the headers.

	// Write out everything up to the beginning of the payload (if present) else
	// the project section.
	if (t_success)
	{
		t_success = MCDeployFileCopy(t_output, 0, t_engine, 0, t_output_offset);
	}
    
    uint32_t t_base_offset = t_output_offset;
    
    // Write out the payload capsule struct (if needed)
	uint32_t t_payload_size = 0;
    if (t_success && t_payload_section != nil)
	{
        if (!MCStringIsEmpty(p_params . payload))
        {
            t_success = MCDeployWritePayload(p_params, false, t_output, t_output_offset, t_payload_size);
            if (t_success)
                t_output_offset += (t_payload_size + 4095) & ~4095;
        }
        else
        {
            t_success = MCDeployFileCopy(t_output, t_output_offset, t_engine, t_payload_section -> PointerToRawData, t_payload_section -> SizeOfRawData);
            if (t_success)
            {
                t_payload_size = t_payload_section -> SizeOfRawData;
                t_output_offset += (t_payload_section -> SizeOfRawData + 4095) & ~4095;
            }
        }
    }

	// Write out the project capsule struct
	uint32_t t_project_size;
	t_project_size = 0;
	if (t_success && t_project_section != nil)
    {
        if (!t_icons_only)
        {
            t_success = MCDeployWriteProject(p_params, false, t_output, t_output_offset, t_project_size);
        }
        else
        {
            t_project_size = t_project_section -> SizeOfRawData;
            t_success = MCDeployFileCopy(t_output, t_output_offset, t_engine, t_project_section -> PointerToRawData, t_project_size);
        }
    }

	// Next use the project size to compute the updated header values we need.
	uint32_t t_optional_header_size, t_optional_header_offset, t_section_headers_offset;
	uint32_t t_resource_section_old_address, t_resource_section_address;
	uint32_t t_resource_section_offset, t_resource_section_old_offset;
	if (t_success)
	{
		t_optional_header_size = MCU_min(sizeof(t_nt_header . OptionalHeader), (uint4)t_nt_header . FileHeader . SizeOfOptionalHeader);
		t_optional_header_offset = t_dos_header . e_lfanew + FIELD_OFFSET(typename DeployPlatformTrait::IMAGE_NT_HEADERS, OptionalHeader);
		t_section_headers_offset = t_optional_header_offset + t_nt_header . FileHeader . SizeOfOptionalHeader;

		uint32_t t_payload_section_size, t_payload_section_delta;
		t_payload_section_size = (t_payload_size + 4095) & ~4095;
		t_payload_section_delta = t_payload_section == nil ? 0 : t_payload_section_size - t_payload_section -> SizeOfRawData;

		uint32_t t_project_section_size, t_project_section_delta;
		t_project_section_size = (t_project_size + 4095) & ~4095;
        t_project_section_delta = t_project_section == nil ? 0 : t_project_section_size - t_project_section -> SizeOfRawData;

		uint32_t t_resource_section_size;
		t_resource_section_size = MCWindowsResourcesMeasure(t_resources);
		t_resource_section_old_offset = t_resource_section -> PointerToRawData;
		t_resource_section_old_address = t_resource_section -> VirtualAddress;

		// Resize the payload section (if present)
		if (t_payload_section != nil)
		{
			t_payload_section -> SizeOfRawData = t_payload_section_size;
			t_payload_section -> Misc . VirtualSize = t_payload_section_size;
            t_payload_section -> PointerToRawData = t_base_offset;
            t_payload_section -> VirtualAddress = t_base_address;

			t_project_section -> VirtualAddress = t_payload_section -> VirtualAddress + t_payload_section_size;
			t_project_section -> PointerToRawData = t_payload_section -> PointerToRawData + t_payload_section_size;
		}
        
		// Resize and shift up the project section (if present)
        if (t_project_section != nil)
        {
            t_project_section -> SizeOfRawData = t_project_section_size;
            t_project_section -> Misc . VirtualSize = t_project_section_size;
        }

		// Resize and shift up the resource section.
        if (t_project_section != nil)
        {
            t_resource_section -> VirtualAddress = t_project_section -> VirtualAddress + t_project_section_size;
            t_resource_section -> PointerToRawData = t_project_section -> PointerToRawData + t_project_section_size;
        }
		t_resource_section -> SizeOfRawData = t_resource_section_size;
		t_resource_section -> Misc . VirtualSize = t_resource_section_size;

		t_resource_section_offset = t_resource_section -> PointerToRawData;
		t_resource_section_address = t_resource_section -> VirtualAddress;
        
        if (t_swap_payload)
        {
            IMAGE_SECTION_HEADER t_swap_header = *t_payload_section;
            t_section_headers[t_section_count - 2] = *t_project_section;
            t_section_headers[t_section_count - 3] = t_swap_header;
        }

		// Update the resource data directory entry and the size of image/initialized data
		t_nt_header . OptionalHeader . SizeOfImage = t_resource_section_address + t_resource_section_size;
        
        t_nt_header . OptionalHeader . SizeOfInitializedData = 0;
		for(uint32_t i = 0; i < t_section_count; i++)
			if (t_section_headers[i] . Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
				t_nt_header . OptionalHeader . SizeOfInitializedData += MCU_max((unsigned)t_section_headers[i] . SizeOfRawData, (unsigned)(t_section_headers[i] . Misc . VirtualSize + 4095) & ~4095);

		// Update the data directory
		t_nt_header . OptionalHeader . DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE] . VirtualAddress = t_resource_section -> VirtualAddress;
		t_nt_header . OptionalHeader . DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE] . Size = t_resource_section_size;

		// Byte swap everything we are about to write out
		DeployPlatformTrait::swap_IMAGE_NT_HEADERS(t_nt_header);
		for(uint32_t i = 0; i < t_section_count; i++)
			swap_IMAGE_SECTION_HEADER(t_section_headers[i]);
	}

	// Now overwrite the nt optional header with our newly computed one
	if (t_success)
		t_success = MCDeployFileWriteAt(t_output, &t_nt_header . OptionalHeader, t_optional_header_size, t_optional_header_offset);

	// Next we overwrite our section headers array
	if (t_success)
		t_success = MCDeployFileWriteAt(t_output, t_section_headers, sizeof(IMAGE_SECTION_HEADER) * t_section_count, t_section_headers_offset);

	// Finally write out the modified resources tree
	if (t_success)
		t_success = MCWindowsResourcesWrite(t_resources, t_resource_section_address, t_output, t_resource_section_offset, t_engine, t_resource_section_old_offset);

	MCWindowsResourcesFinalize(t_resources);

	if (t_section_headers != NULL)
		delete[] t_section_headers;

	MCDeployFileClose(t_engine);
	MCDeployFileClose(t_output);

	return t_success ? ES_NORMAL : ES_ERROR;
}

bool MCDeployWindowsPEHeaderOffset(MCDeployFileRef p_file, uint32_t &r_pe_offset)
{
	// Now check the first two bytes - these should be MZ
	char t_buffer[4];
	if (!MCDeployFileReadAt(p_file, t_buffer, 2, 0) ||
		!MCMemoryEqual(t_buffer, "MZ", 2))
	{
		return MCDeployThrow(kMCDeployErrorWindowsBadDOSSignature);
	}

	// Now read in the offset to the pe header - this resides at
	// byte offset 60 (member e_lfanew in IMAGE_DOS_HEADER).
	uint32_t t_offset;
	if (!MCDeployFileReadAt(p_file, &t_offset, kPEAddressSize, kAddressToPEAddress))
	{
		return MCDeployThrow(kMCDeployErrorWindowsBadDOSHeader);
	}

	// Swap from non-network to host byte order
	MCDeployByteSwap32(false, t_offset);
	r_pe_offset = t_offset;

	return true;
}

// Do some basic validation on the NT header and return the file architecture
bool MCDeployWindowsArchitecture(MCDeployFileRef p_file, uint32_t p_pe_offset, MCDeployArchitecture &r_platform)
{
	bool t_success;
	t_success = true;

	uint32_t t_length;
	if (!MCDeployFileMeasure(p_file, t_length))
		return false;

	// Confirm NT Signature at offset 
	char t_buffer[4];
	if (t_success && (!MCDeployFileReadAt(p_file, t_buffer, 4, p_pe_offset) ||
		!MCMemoryEqual(t_buffer, "PE\0\0", 4)))
	{
		t_success = MCDeployThrow(kMCDeployErrorWindowsNoNTHeader);
	}

	uint16_t t_magic;
	if (t_success && !MCDeployFileReadAt(p_file, &t_magic, sizeof(uint16_t), p_pe_offset + kMagicOffset))
	{
		t_success = MCDeployThrow(kMCDeployErrorWindowsNoNTHeader);
	}

	if (t_success)
	{
		swap_uint16(t_magic);

		switch (t_magic)
		{
		case kHeaderMagic32:
			r_platform = kMCDeployArchitecture_I386;
			break;
		case kHeaderMagic64:
			r_platform = kMCDeployArchitecture_X86_64;
			break;
		default:
			t_success = MCDeployThrow(kMCDeployErrorWindowsNoNTHeader);
			break;
		}
	}

	return t_success;
}

Exec_stat MCDeployToWindows(const MCDeployParameters& p_params)
{
	bool t_success = true;
	
	MCDeployFileRef t_engine;
	t_engine = nullptr;
	if (t_success && !MCDeployFileOpen(p_params.engine, kMCOpenFileModeRead, t_engine))
	{
		t_success = MCDeployThrow(kMCDeployErrorNoEngine);
	}

	uint32_t t_pe_offset;
	if (t_success)
	{
		t_success = MCDeployWindowsPEHeaderOffset(t_engine, t_pe_offset);
	}

	MCDeployArchitecture t_arch;
	if (t_success)
	{
		t_success = MCDeployWindowsArchitecture(t_engine, t_pe_offset, t_arch);
	}

	if (t_engine != nullptr)
	{
		MCDeployFileClose(t_engine);
	}

	if (t_success)
	{
		switch (t_arch)
		{
		case kMCDeployArchitecture_I386: {
			return MCDeployToWindows<MCWindowsPE32Traits>(p_params);
		}
		case kMCDeployArchitecture_X86_64: {
			return MCDeployToWindows<MCWindowsPE64Traits>(p_params);
		}
		default:
			MCUnreachableReturn(ES_ERROR)
		}
	}

	return ES_ERROR;
}
