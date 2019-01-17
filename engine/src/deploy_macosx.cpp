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

#include "handler.h"
#include "scriptpt.h"
#include "variable.h"
#include "statemnt.h"
#include "uuid.h"

#include "deploy.h"

#if defined(_LINUX) || defined(_MACOSX)
#include <sys/stat.h>
#endif

////////////////////////////////////////////////////////////////////////////////
//
// This section contains definitions for the various structures needed to
// process the Mach-O executable format. These are taken from various mach headers
// In some cases the constant lists have been reduced since most are not relevant
// to what this module needs.
//

typedef int             vm_prot_t;

/*
 *      Protection values, defined as bits within the vm_prot_t type
 */

#define VM_PROT_NONE    ((vm_prot_t) 0x00)

#define VM_PROT_READ    ((vm_prot_t) 0x01)      /* read permission */
#define VM_PROT_WRITE   ((vm_prot_t) 0x02)      /* write permission */
#define VM_PROT_EXECUTE ((vm_prot_t) 0x04)      /* execute permission */

/*
 *      The default protection for newly-created virtual memory
 */

#define VM_PROT_DEFAULT (VM_PROT_READ|VM_PROT_WRITE)

/*
 *      The maximum privileges possible, for parameter checking.
 */

#define VM_PROT_ALL     (VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE)

typedef uint32_t       cpu_type_t;
typedef uint32_t       cpu_subtype_t;

/*
 * Capability bits used in the definition of cpu_type.
 */
#define CPU_ARCH_MASK   0xff000000              /* mask for architecture bits */
#define CPU_ARCH_ABI64  0x01000000              /* 64 bit ABI */

/*
 *      Machine types known by all.
 */
 
#define CPU_TYPE_ANY            ((cpu_type_t) -1)

#define CPU_TYPE_VAX            ((cpu_type_t) 1)
/* skip                         ((cpu_type_t) 2)        */
/* skip                         ((cpu_type_t) 3)        */
/* skip                         ((cpu_type_t) 4)        */
/* skip                         ((cpu_type_t) 5)        */
#define CPU_TYPE_MC680x0        ((cpu_type_t) 6)
#define CPU_TYPE_X86            ((cpu_type_t) 7)
#define CPU_TYPE_I386           CPU_TYPE_X86            /* compatibility */
#define CPU_TYPE_X86_64         (CPU_TYPE_X86 | CPU_ARCH_ABI64)

/* skip CPU_TYPE_MIPS           ((cpu_type_t) 8)        */
/* skip                         ((cpu_type_t) 9)        */
#define CPU_TYPE_MC98000        ((cpu_type_t) 10)
#define CPU_TYPE_HPPA           ((cpu_type_t) 11)
#define CPU_TYPE_ARM            ((cpu_type_t) 12)
#define CPU_TYPE_ARM64          (CPU_TYPE_ARM | CPU_ARCH_ABI64)
#define CPU_TYPE_MC88000        ((cpu_type_t) 13)
#define CPU_TYPE_SPARC          ((cpu_type_t) 14)
#define CPU_TYPE_I860           ((cpu_type_t) 15)
/* skip CPU_TYPE_ALPHA          ((cpu_type_t) 16)       */
/* skip                         ((cpu_type_t) 17)       */
#define CPU_TYPE_POWERPC                ((cpu_type_t) 18)
#define CPU_TYPE_POWERPC64              (CPU_TYPE_POWERPC | CPU_ARCH_ABI64)

/*
 * Capability bits used in the definition of cpu_subtype.
 */
#define CPU_SUBTYPE_MASK        0xff000000      /* mask for feature flags */
#define CPU_SUBTYPE_LIB64       0x80000000      /* 64 bit libraries */


/*
 *      Object files that are hand-crafted to run on any
 *      implementation of an architecture are tagged with
 *      CPU_SUBTYPE_MULTIPLE.  This functions essentially the same as
 *      the "ALL" subtype of an architecture except that it allows us
 *      to easily find object files that may need to be modified
 *      whenever a new implementation of an architecture comes out.
 *
 *      It is the responsibility of the implementor to make sure the
 *      software handles unsupported implementations elegantly.
 */
#define CPU_SUBTYPE_MULTIPLE            ((cpu_subtype_t) -1)
#define CPU_SUBTYPE_LITTLE_ENDIAN       ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_BIG_ENDIAN          ((cpu_subtype_t) 1)

/*
 *      I386 subtypes
 */

#define CPU_SUBTYPE_INTEL(f, m) ((cpu_subtype_t) (f) + ((m) << 4))

#define CPU_SUBTYPE_I386_ALL                    CPU_SUBTYPE_INTEL(3, 0)
#define CPU_SUBTYPE_386                                 CPU_SUBTYPE_INTEL(3, 0)
#define CPU_SUBTYPE_486                                 CPU_SUBTYPE_INTEL(4, 0)
#define CPU_SUBTYPE_486SX                               CPU_SUBTYPE_INTEL(4, 8) // 8 << 4 = 128
#define CPU_SUBTYPE_586                                 CPU_SUBTYPE_INTEL(5, 0)
#define CPU_SUBTYPE_PENT        CPU_SUBTYPE_INTEL(5, 0)
#define CPU_SUBTYPE_PENTPRO     CPU_SUBTYPE_INTEL(6, 1)
#define CPU_SUBTYPE_PENTII_M3   CPU_SUBTYPE_INTEL(6, 3)
#define CPU_SUBTYPE_PENTII_M5   CPU_SUBTYPE_INTEL(6, 5)
#define CPU_SUBTYPE_CELERON                             CPU_SUBTYPE_INTEL(7, 6)
#define CPU_SUBTYPE_CELERON_MOBILE              CPU_SUBTYPE_INTEL(7, 7)
#define CPU_SUBTYPE_PENTIUM_3                   CPU_SUBTYPE_INTEL(8, 0)
#define CPU_SUBTYPE_PENTIUM_3_M                 CPU_SUBTYPE_INTEL(8, 1)
#define CPU_SUBTYPE_PENTIUM_3_XEON              CPU_SUBTYPE_INTEL(8, 2)
#define CPU_SUBTYPE_PENTIUM_M                   CPU_SUBTYPE_INTEL(9, 0)
#define CPU_SUBTYPE_PENTIUM_4                   CPU_SUBTYPE_INTEL(10, 0)
#define CPU_SUBTYPE_PENTIUM_4_M                 CPU_SUBTYPE_INTEL(10, 1)
#define CPU_SUBTYPE_ITANIUM                             CPU_SUBTYPE_INTEL(11, 0)
#define CPU_SUBTYPE_ITANIUM_2                   CPU_SUBTYPE_INTEL(11, 1)
#define CPU_SUBTYPE_XEON                                CPU_SUBTYPE_INTEL(12, 0)
#define CPU_SUBTYPE_XEON_MP                             CPU_SUBTYPE_INTEL(12, 1)

#define CPU_SUBTYPE_INTEL_FAMILY(x)     ((x) & 15)
#define CPU_SUBTYPE_INTEL_FAMILY_MAX    15

#define CPU_SUBTYPE_INTEL_MODEL(x)      ((x) >> 4)
#define CPU_SUBTYPE_INTEL_MODEL_ALL     0

/*
 *      X86 subtypes.
 */

#define CPU_SUBTYPE_X86_ALL             ((cpu_subtype_t)3)
#define CPU_SUBTYPE_X86_64_ALL          ((cpu_subtype_t)3)
#define CPU_SUBTYPE_X86_ARCH1           ((cpu_subtype_t)4)

/*
 *      PowerPC subtypes
 */
#define CPU_SUBTYPE_POWERPC_ALL         ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_POWERPC_601         ((cpu_subtype_t) 1)
#define CPU_SUBTYPE_POWERPC_602         ((cpu_subtype_t) 2)
#define CPU_SUBTYPE_POWERPC_603         ((cpu_subtype_t) 3)
#define CPU_SUBTYPE_POWERPC_603e        ((cpu_subtype_t) 4)
#define CPU_SUBTYPE_POWERPC_603ev       ((cpu_subtype_t) 5)
#define CPU_SUBTYPE_POWERPC_604         ((cpu_subtype_t) 6)
#define CPU_SUBTYPE_POWERPC_604e        ((cpu_subtype_t) 7)
#define CPU_SUBTYPE_POWERPC_620         ((cpu_subtype_t) 8)
#define CPU_SUBTYPE_POWERPC_750         ((cpu_subtype_t) 9)
#define CPU_SUBTYPE_POWERPC_7400        ((cpu_subtype_t) 10)
#define CPU_SUBTYPE_POWERPC_7450        ((cpu_subtype_t) 11)
#define CPU_SUBTYPE_POWERPC_970         ((cpu_subtype_t) 100)

/*
 *	ARM subtypes
 */
#define CPU_SUBTYPE_ARM_ALL             ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_ARM_V4T             ((cpu_subtype_t) 5)
#define CPU_SUBTYPE_ARM_V6              ((cpu_subtype_t) 6)
#define CPU_SUBTYPE_ARM_V5TEJ           ((cpu_subtype_t) 7)
#define CPU_SUBTYPE_ARM_XSCALE		((cpu_subtype_t) 8)
#define CPU_SUBTYPE_ARM_V7		((cpu_subtype_t) 9)
#define CPU_SUBTYPE_ARM_V7F		((cpu_subtype_t) 10) /* Cortex A9 */
#define CPU_SUBTYPE_ARM_V7S		((cpu_subtype_t) 11) /* Swift */
#define CPU_SUBTYPE_ARM_V7K		((cpu_subtype_t) 12) /* Kirkwood40 */
#define CPU_SUBTYPE_ARM_V6M		((cpu_subtype_t) 14) /* Not meant to be run under xnu */
#define CPU_SUBTYPE_ARM_V7M		((cpu_subtype_t) 15) /* Not meant to be run under xnu */
#define CPU_SUBTYPE_ARM_V7EM		((cpu_subtype_t) 16) /* Not meant to be run under xnu */

#define CPU_SUBTYPE_ARM_V8		((cpu_subtype_t) 13)

/*
 *  ARM64 subtypes
 */
#define CPU_SUBTYPE_ARM64_ALL           ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_ARM64_V8            ((cpu_subtype_t) 1)

#define FAT_MAGIC       0xcafebabe
#define FAT_CIGAM       0xbebafeca      /* NXSwapLong(FAT_MAGIC) */

struct fat_header {
        uint32_t        magic;          /* FAT_MAGIC */
        uint32_t        nfat_arch;      /* number of structs that follow */
};

struct fat_arch {
        cpu_type_t      cputype;        /* cpu specifier (int) */
        cpu_subtype_t   cpusubtype;     /* machine specifier (int) */
        uint32_t        offset;         /* file offset to this object file */
        uint32_t        size;           /* size of this object file */
        uint32_t        align;          /* alignment as a power of 2 */
};

/*
 * The 32-bit mach header appears at the very beginning of the object file for
 * 32-bit architectures.
 */
struct mach_header {
        uint32_t        magic;          /* mach magic number identifier */
        cpu_type_t      cputype;        /* cpu specifier */
        cpu_subtype_t   cpusubtype;     /* machine specifier */
        uint32_t        filetype;       /* type of file */
        uint32_t        ncmds;          /* number of load commands */
        uint32_t        sizeofcmds;     /* the size of all the load commands */
        uint32_t        flags;          /* flags */
};

/* Constant for the magic field of the mach_header (32-bit architectures) */
#define MH_MAGIC        0xfeedface      /* the mach magic number */
#define MH_CIGAM        0xcefaedfe      /* NXSwapInt(MH_MAGIC) */

/*
 * The 64-bit mach header appears at the very beginning of object files for
 * 64-bit architectures.
 */
struct mach_header_64 {
	uint32_t	magic;		/* mach magic number identifier */
	cpu_type_t	cputype;	/* cpu specifier */
	cpu_subtype_t	cpusubtype;	/* machine specifier */
	uint32_t	filetype;	/* type of file */
	uint32_t	ncmds;		/* number of load commands */
	uint32_t	sizeofcmds;	/* the size of all the load commands */
	uint32_t	flags;		/* flags */
	uint32_t	reserved;	/* reserved */
};

/* Constant for the magic field of the mach_header_64 (64-bit architectures) */
#define MH_MAGIC_64 0xfeedfacf /* the 64-bit mach magic number */
#define MH_CIGAM_64 0xcffaedfe /* NXSwapInt(MH_MAGIC_64) */

/*
 * The layout of the file depends on the filetype.  For all but the MH_OBJECT
 * file type the segments are padded out and aligned on a segment alignment
 * boundary for efficient demand pageing.  The MH_EXECUTE, MH_FVMLIB, MH_DYLIB,
 * MH_DYLINKER and MH_BUNDLE file types also have the headers included as part
 * of their first segment.
 * 
 * The file type MH_OBJECT is a compact format intended as output of the
 * assembler and input (and possibly output) of the link editor (the .o
 * format).  All sections are in one unnamed segment with no segment padding. 
 * This format is used as an executable format when the file is so small the
 * segment padding greatly increases its size.
 *
 * The file type MH_PRELOAD is an executable format intended for things that
 * are not executed under the kernel (proms, stand alones, kernels, etc).  The
 * format can be executed under the kernel but may demand paged it and not
 * preload it before execution.
 *
 * A core file is in MH_CORE format and can be any in an arbritray legal
 * Mach-O file.
 *
 * Constants for the filetype field of the mach_header
 */
#define	MH_OBJECT	0x1		/* relocatable object file */
#define	MH_EXECUTE	0x2		/* demand paged executable file */
#define	MH_FVMLIB	0x3		/* fixed VM shared library file */
#define	MH_CORE		0x4		/* core file */
#define	MH_PRELOAD	0x5		/* preloaded executable file */
#define	MH_DYLIB	0x6		/* dynamically bound shared library */
#define	MH_DYLINKER	0x7		/* dynamic link editor */
#define	MH_BUNDLE	0x8		/* dynamically bound bundle file */
#define	MH_DYLIB_STUB	0x9		/* shared library stub for static */
					/*  linking only, no section contents */
#define	MH_DSYM		0xa		/* companion file with only debug */
					/*  sections */

/*
 * The load commands directly follow the mach_header.  The total size of all
 * of the commands is given by the sizeofcmds field in the mach_header.  All
 * load commands must have as their first two fields cmd and cmdsize.  The cmd
 * field is filled in with a constant for that command type.  Each command type
 * has a structure specifically for it.  The cmdsize field is the size in bytes
 * of the particular load command structure plus anything that follows it that
 * is a part of the load command (i.e. section structures, strings, etc.).  To
 * advance to the next load command the cmdsize can be added to the offset or
 * pointer of the current load command.  The cmdsize for 32-bit architectures
 * MUST be a multiple of 4 bytes and for 64-bit architectures MUST be a multiple
 * of 8 bytes (these are forever the maximum alignment of any load commands).
 * The padded bytes must be zero.  All tables in the object file must also
 * follow these rules so the file can be memory mapped.  Otherwise the pointers
 * to these tables will not work well or at all on some machines.  With all
 * padding zeroed like objects will compare byte for byte.
 */
struct load_command {
	uint32_t cmd;		/* type of load command */
	uint32_t cmdsize;	/* total size of command in bytes */
};

/*
 * After MacOS X 10.1 when a new load command is added that is required to be
 * understood by the dynamic linker for the image to execute properly the
 * LC_REQ_DYLD bit will be or'ed into the load command constant.  If the dynamic
 * linker sees such a load command it it does not understand will issue a
 * "unknown load command required for execution" error and refuse to use the
 * image.  Other load commands without this bit that are not understood will
 * simply be ignored.
 */
#define LC_REQ_DYLD 0x80000000

/* Constants for the cmd field of all load commands, the type */
#define	LC_SEGMENT	0x1	/* segment of this file to be mapped */
#define	LC_SYMTAB	0x2	/* link-edit stab symbol table info */
#define	LC_SYMSEG	0x3	/* link-edit gdb symbol table info (obsolete) */
#define	LC_THREAD	0x4	/* thread */
#define	LC_UNIXTHREAD	0x5	/* unix thread (includes a stack) */
#define	LC_LOADFVMLIB	0x6	/* load a specified fixed VM shared library */
#define	LC_IDFVMLIB	0x7	/* fixed VM shared library identification */
#define	LC_IDENT	0x8	/* object identification info (obsolete) */
#define LC_FVMFILE	0x9	/* fixed VM file inclusion (internal use) */
#define LC_PREPAGE      0xa     /* prepage command (internal use) */
#define	LC_DYSYMTAB	0xb	/* dynamic link-edit symbol table info */
#define	LC_LOAD_DYLIB	0xc	/* load a dynamically linked shared library */
#define	LC_ID_DYLIB	0xd	/* dynamically linked shared lib ident */
#define LC_LOAD_DYLINKER 0xe	/* load a dynamic linker */
#define LC_ID_DYLINKER	0xf	/* dynamic linker identification */
#define	LC_PREBOUND_DYLIB 0x10	/* modules prebound for a dynamically */
				/*  linked shared library */
#define	LC_ROUTINES	0x11	/* image routines */
#define	LC_SUB_FRAMEWORK 0x12	/* sub framework */
#define	LC_SUB_UMBRELLA 0x13	/* sub umbrella */
#define	LC_SUB_CLIENT	0x14	/* sub client */
#define	LC_SUB_LIBRARY  0x15	/* sub library */
#define	LC_TWOLEVEL_HINTS 0x16	/* two-level namespace lookup hints */
#define	LC_PREBIND_CKSUM  0x17	/* prebind checksum */

/*
 * load a dynamically linked shared library that is allowed to be missing
 * (all symbols are weak imported).
 */
#define	LC_LOAD_WEAK_DYLIB (0x18 | LC_REQ_DYLD)

#define	LC_SEGMENT_64	0x19	/* 64-bit segment of this file to be
				   mapped */
#define	LC_ROUTINES_64	0x1a	/* 64-bit image routines */
#define LC_UUID		0x1b	/* the uuid */
#define LC_RPATH       (0x1c | LC_REQ_DYLD)    /* runpath additions */
#define LC_CODE_SIGNATURE 0x1d	/* local of code signature */
#define LC_SEGMENT_SPLIT_INFO 0x1e /* local of info to split segments */
#define LC_REEXPORT_DYLIB (0x1f | LC_REQ_DYLD) /* load and re-export dylib */
#define	LC_LAZY_LOAD_DYLIB 0x20	/* delay load of dylib until first use */
#define	LC_ENCRYPTION_INFO 0x21	/* encrypted segment information */
#define LC_VERSION_MIN_MACOSX 0x24   /* build for MacOSX min OS version */
#define LC_VERSION_MIN_IPHONEOS 0x25 /* build for iPhoneOS min OS version */
#define LC_FUNCTION_STARTS 0x26
#define LC_DATA_IN_CODE 0x29
#define LC_DYLIB_CODE_SIGN_DRS 0x2B
#define LC_DYLD_INFO 0x22
#define LC_DYLD_INFO_ONLY 0x80000022
#define LC_SOURCE_VERSION 0x2A
#define LC_ENCRYPTION_INFO_64 0x2C

// MM-2014-09-30: [[ iOS 8 Support ]] Used by iOS 8 simulator builds.
#define LC_MAIN (0x28|LC_REQ_DYLD) /* replacement for LC_UNIXTHREAD */

// PM-2018-10-02: [[ iOS 12 Support ]] Used by iOS device builds when min_version=12
#define LC_BUILD_VERSION 0x32 /* build for platform min OS version */

/*
 * A variable length string in a load command is represented by an lc_str
 * union.  The strings are stored just after the load command structure and
 * the offset is from the start of the load command structure.  The size
 * of the string is reflected in the cmdsize field of the load command.
 * Once again any padded bytes to bring the cmdsize field to a multiple
 * of 4 bytes must be zero.
 */
union lc_str {
	uint32_t	offset;	/* offset to the string */
#ifndef __LP64__
	char		*ptr;	/* pointer to the string */
#endif 
};

/*
 * Dynamicly linked shared libraries are identified by two things.  The
 * pathname (the name of the library as found for execution), and the
 * compatibility version number.  The pathname must match and the compatibility
 * number in the user of the library must be greater than or equal to the
 * library being used.  The time stamp is used to record the time a library was
 * built and copied into user so it can be use to determined if the library used
 * at runtime is exactly the same as used to built the program.
 */
struct dylib {
    union lc_str  name;			/* library's path name */
    uint32_t timestamp;			/* library's build time stamp */
    uint32_t current_version;		/* library's current version number */
    uint32_t compatibility_version;	/* library's compatibility vers number*/
};

/*
 * A dynamically linked shared library (filetype == MH_DYLIB in the mach header)
 * contains a dylib_command (cmd == LC_ID_DYLIB) to identify the library.
 * An object that uses a dynamically linked shared library also contains a
 * dylib_command (cmd == LC_LOAD_DYLIB, LC_LOAD_WEAK_DYLIB, or
 * LC_REEXPORT_DYLIB) for each library it uses.
 */
struct dylib_command {
	uint32_t	cmd;		/* LC_ID_DYLIB, LC_LOAD_{,WEAK_}DYLIB,
					   LC_REEXPORT_DYLIB */
	uint32_t	cmdsize;	/* includes pathname string */
	struct dylib	dylib;		/* the library identification */
};

/*
 * The segment load command indicates that a part of this file is to be
 * mapped into the task's address space.  The size of this segment in memory,
 * vmsize, maybe equal to or larger than the amount to map from this file,
 * filesize.  The file is mapped starting at fileoff to the beginning of
 * the segment in memory, vmaddr.  The rest of the memory of the segment,
 * if any, is allocated zero fill on demand.  The segment's maximum virtual
 * memory protection and initial virtual memory protection are specified
 * by the maxprot and initprot fields.  If the segment has sections then the
 * section structures directly follow the segment command and their size is
 * reflected in cmdsize.
 */
struct segment_command { /* for 32-bit architectures */
	uint32_t	cmd;		/* LC_SEGMENT */
	uint32_t	cmdsize;	/* includes sizeof section structs */
	char		segname[16];	/* segment name */
	uint32_t	vmaddr;		/* memory address of this segment */
	uint32_t	vmsize;		/* memory size of this segment */
	uint32_t	fileoff;	/* file offset of this segment */
	uint32_t	filesize;	/* amount to map from the file */
	vm_prot_t	maxprot;	/* maximum VM protection */
	vm_prot_t	initprot;	/* initial VM protection */
	uint32_t	nsects;		/* number of sections in segment */
	uint32_t	flags;		/* flags */
};

/*
 * The 64-bit segment load command indicates that a part of this file is to be
 * mapped into a 64-bit task's address space.  If the 64-bit segment has
 * sections then section_64 structures directly follow the 64-bit segment
 * command and their size is reflected in cmdsize.
 */
struct segment_command_64 { /* for 64-bit architectures */
	uint32_t	cmd;		/* LC_SEGMENT_64 */
	uint32_t	cmdsize;	/* includes sizeof section_64 structs */
	char		segname[16];	/* segment name */
	uint64_t	vmaddr;		/* memory address of this segment */
	uint64_t	vmsize;		/* memory size of this segment */
	uint64_t	fileoff;	/* file offset of this segment */
	uint64_t	filesize;	/* amount to map from the file */
	vm_prot_t	maxprot;	/* maximum VM protection */
	vm_prot_t	initprot;	/* initial VM protection */
	uint32_t	nsects;		/* number of sections in segment */
	uint32_t	flags;		/* flags */
};

/*
 * A segment is made up of zero or more sections.  Non-MH_OBJECT files have
 * all of their segments with the proper sections in each, and padded to the
 * specified segment alignment when produced by the link editor.  The first
 * segment of a MH_EXECUTE and MH_FVMLIB format file contains the mach_header
 * and load commands of the object file before its first section.  The zero
 * fill sections are always last in their segment (in all formats).  This
 * allows the zeroed segment padding to be mapped into memory where zero fill
 * sections might be. The gigabyte zero fill sections, those with the section
 * type S_GB_ZEROFILL, can only be in a segment with sections of this type.
 * These segments are then placed after all other segments.
 *
 * The MH_OBJECT format has all of its sections in one segment for
 * compactness.  There is no padding to a specified segment boundary and the
 * mach_header and load commands are not part of the segment.
 *
 * Sections with the same section name, sectname, going into the same segment,
 * segname, are combined by the link editor.  The resulting section is aligned
 * to the maximum alignment of the combined sections and is the new section's
 * alignment.  The combined sections are aligned to their original alignment in
 * the combined section.  Any padded bytes to get the specified alignment are
 * zeroed.
 *
 * The format of the relocation entries referenced by the reloff and nreloc
 * fields of the section structure for mach object files is described in the
 * header file <reloc.h>.
 */
struct section { /* for 32-bit architectures */
	char		sectname[16];	/* name of this section */
	char		segname[16];	/* segment this section goes in */
	uint32_t	addr;		/* memory address of this section */
	uint32_t	size;		/* size in bytes of this section */
	uint32_t	offset;		/* file offset of this section */
	uint32_t	align;		/* section alignment (power of 2) */
	uint32_t	reloff;		/* file offset of relocation entries */
	uint32_t	nreloc;		/* number of relocation entries */
	uint32_t	flags;		/* flags (section type and attributes)*/
	uint32_t	reserved1;	/* reserved (for offset or index) */
	uint32_t	reserved2;	/* reserved (for count or sizeof) */
};

struct section_64 { /* for 64-bit architectures */
	char		sectname[16];	/* name of this section */
	char		segname[16];	/* segment this section goes in */
	uint64_t	addr;		/* memory address of this section */
	uint64_t	size;		/* size in bytes of this section */
	uint32_t	offset;		/* file offset of this section */
	uint32_t	align;		/* section alignment (power of 2) */
	uint32_t	reloff;		/* file offset of relocation entries */
	uint32_t	nreloc;		/* number of relocation entries */
	uint32_t	flags;		/* flags (section type and attributes)*/
	uint32_t	reserved1;	/* reserved (for offset or index) */
	uint32_t	reserved2;	/* reserved (for count or sizeof) */
	uint32_t	reserved3;	/* reserved */
};

/*
 * The flags field of a section structure is separated into two parts a section
 * type and section attributes.  The section types are mutually exclusive (it
 * can only have one type) but the section attributes are not (it may have more
 * than one attribute).
 */
#define SECTION_TYPE		 0x000000ff	/* 256 section types */
#define SECTION_ATTRIBUTES	 0xffffff00	/*  24 section attributes */

/* Constants for the type of a section */
#define	S_REGULAR		0x0	/* regular section */
#define	S_ZEROFILL		0x1	/* zero fill on demand section */
#define	S_CSTRING_LITERALS	0x2	/* section with only literal C strings*/
#define	S_4BYTE_LITERALS	0x3	/* section with only 4 byte literals */
#define	S_8BYTE_LITERALS	0x4	/* section with only 8 byte literals */
#define	S_LITERAL_POINTERS	0x5	/* section with only pointers to */
					/*  literals */

/*
 * The symtab_command contains the offsets and sizes of the link-edit 4.3BSD
 * "stab" style symbol table information as described in the header files
 * <nlist.h> and <stab.h>.
 */
struct symtab_command {
	uint32_t	cmd;		/* LC_SYMTAB */
	uint32_t	cmdsize;	/* sizeof(struct symtab_command) */
	uint32_t	symoff;		/* symbol table offset */
	uint32_t	nsyms;		/* number of symbol table entries */
	uint32_t	stroff;		/* string table offset */
	uint32_t	strsize;	/* string table size in bytes */
};

struct nlist {
	int32_t n_strx;
	uint8_t n_type;
	uint8_t n_sect;
	int16_t n_desc;
	uint32_t n_value;
};

#define N_UNDF  0x00            /* undefined */
#define N_ABS   0x02            /* absolute address */
#define N_TEXT  0x04            /* text segment */
#define N_DATA  0x06            /* data segment */
#define N_BSS   0x08            /* bss segment */
#define N_COMM  0x12            /* common reference */
#define N_FN    0x1e            /* file name */

#define N_EXT   0x01            /* external (global) bit, OR'ed in */
#define N_TYPE  0x1e            /* mask for all the type bits */

#define N_FORMAT        "%08x"  /* namelist value format; XXX */
#define N_STAB          0x0e0   /* mask for debugger symbols -- stab(5) */

/*
 * This is the second set of the symbolic information which is used to support
 * the data structures for the dynamically link editor.
 *
 * The original set of symbolic information in the symtab_command which contains
 * the symbol and string tables must also be present when this load command is
 * present.  When this load command is present the symbol table is organized
 * into three groups of symbols:
 *	local symbols (static and debugging symbols) - grouped by module
 *	defined external symbols - grouped by module (sorted by name if not lib)
 *	undefined external symbols (sorted by name if MH_BINDATLOAD is not set,
 *	     			    and in order the were seen by the static
 *				    linker if MH_BINDATLOAD is set)
 * In this load command there are offsets and counts to each of the three groups
 * of symbols.
 *
 * This load command contains a the offsets and sizes of the following new
 * symbolic information tables:
 *	table of contents
 *	module table
 *	reference symbol table
 *	indirect symbol table
 * The first three tables above (the table of contents, module table and
 * reference symbol table) are only present if the file is a dynamically linked
 * shared library.  For executable and object modules, which are files
 * containing only one module, the information that would be in these three
 * tables is determined as follows:
 * 	table of contents - the defined external symbols are sorted by name
 *	module table - the file contains only one module so everything in the
 *		       file is part of the module.
 *	reference symbol table - is the defined and undefined external symbols
 *
 * For dynamically linked shared library files this load command also contains
 * offsets and sizes to the pool of relocation entries for all sections
 * separated into two groups:
 *	external relocation entries
 *	local relocation entries
 * For executable and object modules the relocation entries continue to hang
 * off the section structures.
 */
struct dysymtab_command {
    uint32_t cmd;	/* LC_DYSYMTAB */
    uint32_t cmdsize;	/* sizeof(struct dysymtab_command) */

    /*
     * The symbols indicated by symoff and nsyms of the LC_SYMTAB load command
     * are grouped into the following three groups:
     *    local symbols (further grouped by the module they are from)
     *    defined external symbols (further grouped by the module they are from)
     *    undefined symbols
     *
     * The local symbols are used only for debugging.  The dynamic binding
     * process may have to use them to indicate to the debugger the local
     * symbols for a module that is being bound.
     *
     * The last two groups are used by the dynamic binding process to do the
     * binding (indirectly through the module table and the reference symbol
     * table when this is a dynamically linked shared library file).
     */
    uint32_t ilocalsym;	/* index to local symbols */
    uint32_t nlocalsym;	/* number of local symbols */

    uint32_t iextdefsym;/* index to externally defined symbols */
    uint32_t nextdefsym;/* number of externally defined symbols */

    uint32_t iundefsym;	/* index to undefined symbols */
    uint32_t nundefsym;	/* number of undefined symbols */

    /*
     * For the for the dynamic binding process to find which module a symbol
     * is defined in the table of contents is used (analogous to the ranlib
     * structure in an archive) which maps defined external symbols to modules
     * they are defined in.  This exists only in a dynamically linked shared
     * library file.  For executable and object modules the defined external
     * symbols are sorted by name and is use as the table of contents.
     */
    uint32_t tocoff;	/* file offset to table of contents */
    uint32_t ntoc;	/* number of entries in table of contents */

    /*
     * To support dynamic binding of "modules" (whole object files) the symbol
     * table must reflect the modules that the file was created from.  This is
     * done by having a module table that has indexes and counts into the merged
     * tables for each module.  The module structure that these two entries
     * refer to is described below.  This exists only in a dynamically linked
     * shared library file.  For executable and object modules the file only
     * contains one module so everything in the file belongs to the module.
     */
    uint32_t modtaboff;	/* file offset to module table */
    uint32_t nmodtab;	/* number of module table entries */

    /*
     * To support dynamic module binding the module structure for each module
     * indicates the external references (defined and undefined) each module
     * makes.  For each module there is an offset and a count into the
     * reference symbol table for the symbols that the module references.
     * This exists only in a dynamically linked shared library file.  For
     * executable and object modules the defined external symbols and the
     * undefined external symbols indicates the external references.
     */
    uint32_t extrefsymoff;	/* offset to referenced symbol table */
    uint32_t nextrefsyms;	/* number of referenced symbol table entries */

    /*
     * The sections that contain "symbol pointers" and "routine stubs" have
     * indexes and (implied counts based on the size of the section and fixed
     * size of the entry) into the "indirect symbol" table for each pointer
     * and stub.  For every section of these two types the index into the
     * indirect symbol table is stored in the section header in the field
     * reserved1.  An indirect symbol table entry is simply a 32bit index into
     * the symbol table to the symbol that the pointer or stub is referring to.
     * The indirect symbol table is ordered to match the entries in the section.
     */
    uint32_t indirectsymoff; /* file offset to the indirect symbol table */
    uint32_t nindirectsyms;  /* number of indirect symbol table entries */

    /*
     * To support relocating an individual module in a library file quickly the
     * external relocation entries for each module in the library need to be
     * accessed efficiently.  Since the relocation entries can't be accessed
     * through the section headers for a library file they are separated into
     * groups of local and external entries further grouped by module.  In this
     * case the presents of this load command who's extreloff, nextrel,
     * locreloff and nlocrel fields are non-zero indicates that the relocation
     * entries of non-merged sections are not referenced through the section
     * structures (and the reloff and nreloc fields in the section headers are
     * set to zero).
     *
     * Since the relocation entries are not accessed through the section headers
     * this requires the r_address field to be something other than a section
     * offset to identify the item to be relocated.  In this case r_address is
     * set to the offset from the vmaddr of the first LC_SEGMENT command.
     * For MH_SPLIT_SEGS images r_address is set to the the offset from the
     * vmaddr of the first read-write LC_SEGMENT command.
     *
     * The relocation entries are grouped by module and the module table
     * entries have indexes and counts into them for the group of external
     * relocation entries for that the module.
     *
     * For sections that are merged across modules there must not be any
     * remaining external relocation entries for them (for merged sections
     * remaining relocation entries must be local).
     */
    uint32_t extreloff;	/* offset to external relocation entries */
    uint32_t nextrel;	/* number of external relocation entries */

    /*
     * All the local relocation entries are grouped together (they are not
     * grouped by their module since they are only used if the object is moved
     * from it staticly link edited address).
     */
    uint32_t locreloff;	/* offset to local relocation entries */
    uint32_t nlocrel;	/* number of local relocation entries */

};
/*
 * An indirect symbol table entry is simply a 32bit index into the symbol table 
 * to the symbol that the pointer or stub is refering to.  Unless it is for a
 * non-lazy symbol pointer section for a defined symbol which strip(1) as 
 * removed.  In which case it has the value INDIRECT_SYMBOL_LOCAL.  If the
 * symbol was also absolute INDIRECT_SYMBOL_ABS is or'ed with that.
 */
#define INDIRECT_SYMBOL_LOCAL   0x80000000
#define INDIRECT_SYMBOL_ABS     0x40000000

/* a table of contents entry */
struct dylib_table_of_contents {
    uint32_t symbol_index;      /* the defined external symbol
                                   (index into the symbol table) */
    uint32_t module_index;      /* index into the module table this symbol
                                   is defined in */
};      

/* a module table entry */
struct dylib_module {
    uint32_t module_name;       /* the module name (index into string table) */

    uint32_t iextdefsym;        /* index into externally defined symbols */
    uint32_t nextdefsym;        /* number of externally defined symbols */
    uint32_t irefsym;           /* index into reference symbol table */
    uint32_t nrefsym;           /* number of reference symbol table entries */
    uint32_t ilocalsym;         /* index into symbols for local symbols */
    uint32_t nlocalsym;         /* number of local symbols */

    uint32_t iextrel;           /* index into external relocation entries */
    uint32_t nextrel;           /* number of external relocation entries */

    uint32_t iinit_iterm;       /* low 16 bits are the index into the init
                                   section, high 16 bits are the index into
                                   the term section */
    uint32_t ninit_nterm;       /* low 16 bits are the number of init section
                                   entries, high 16 bits are the number of
                                   term section entries */

    uint32_t                    /* for this module address of the start of */
        objc_module_info_addr;  /*  the (__OBJC,__module_info) section */
    uint32_t                    /* for this module size of */
        objc_module_info_size;  /*  the (__OBJC,__module_info) section */
};

/* a 64-bit module table entry */
struct dylib_module_64 {
    uint32_t module_name;	/* the module name (index into string table) */
    
    uint32_t iextdefsym;	/* index into externally defined symbols */
    uint32_t nextdefsym;	/* number of externally defined symbols */
    uint32_t irefsym;		/* index into reference symbol table */
    uint32_t nrefsym;		/* number of reference symbol table entries */
    uint32_t ilocalsym;		/* index into symbols for local symbols */
    uint32_t nlocalsym;		/* number of local symbols */
    
    uint32_t iextrel;		/* index into external relocation entries */
    uint32_t nextrel;		/* number of external relocation entries */
    
    uint32_t iinit_iterm;	/* low 16 bits are the index into the init
                             section, high 16 bits are the index into
                             the term section */
    uint32_t ninit_nterm;      /* low 16 bits are the number of init section
                                entries, high 16 bits are the number of
                                term section entries */
    
    uint32_t			/* for this module size of */
    objc_module_info_size;	/*  the (__OBJC,__module_info) section */
    uint64_t			/* for this module address of the start of */
    objc_module_info_addr;	/*  the (__OBJC,__module_info) section */
};

/* 
 * The entries in the reference symbol table are used when loading the module
 * (both by the static and dynamic link editors) and if the module is unloaded
 * or replaced.  Therefore all external symbols (defined and undefined) are
 * listed in the module's reference table.  The flags describe the type of
 * reference that is being made.  The constants for the flags are defined in
 * <mach-o/nlist.h> as they are also used for symbol table entries.
 */
struct dylib_reference {
    uint32_t isym:24,           /* index into the symbol table */
                  flags:8;      /* flags to indicate the type of reference */
};

/*
 * Format of a relocation entry of a Mach-O file.  Modified from the 4.3BSD
 * format.  The modifications from the original format were changing the value
 * of the r_symbolnum field for "local" (r_extern == 0) relocation entries.
 * This modification is required to support symbols in an arbitrary number of
 * sections not just the three sections (text, data and bss) in a 4.3BSD file.
 * Also the last 4 bits have had the r_type tag added to them.
 */
struct relocation_info {
   int32_t      r_address;      /* offset in the section to what is being
                                   relocated */
   uint32_t     r_symbolnum:24, /* symbol index if r_extern == 1 or section
                                   ordinal if r_extern == 0 */
                r_pcrel:1,      /* was relocated pc relative already */
                r_length:2,     /* 0=byte, 1=word, 2=long, 3=quad */
                r_extern:1,     /* does not include value of sym referenced */
                r_type:4;       /* if not 0, machine specific relocation type */
};
#define R_ABS   0               /* absolute relocation type for Mach-O files */

/*
 * The twolevel_hints_command contains the offset and number of hints in the
 * two-level namespace lookup hints table.
 */
struct twolevel_hints_command {
    uint32_t cmd;	/* LC_TWOLEVEL_HINTS */
    uint32_t cmdsize;	/* sizeof(struct twolevel_hints_command) */
    uint32_t offset;	/* offset to the hint table */
    uint32_t nhints;	/* number of hints in the hint table */
};

struct linkedit_data_command
{
	uint32_t cmd;
	uint32_t cmdsize;
	uint32_t dataoff;
	uint32_t datasize;
};

struct dyld_info_command
{
	uint32_t cmd;
	uint32_t cmdsize;
	uint32_t rebase_off;
	uint32_t rebase_size;
	uint32_t bind_off;
	uint32_t bind_size;
	uint32_t weak_bind_off;
	uint32_t weak_bind_size;
	uint32_t lazy_bind_off;
	uint32_t lazy_bind_size;
	uint32_t export_off;
	uint32_t export_size;
};

/*
 * The version_min_command contains the min OS version on which this
 * binary was built to run.
 */
struct version_min_command {
    uint32_t	cmd;		/* LC_VERSION_MIN_MACOSX or
                             LC_VERSION_MIN_IPHONEOS  */
    uint32_t	cmdsize;	/* sizeof(struct min_version_command) */
    uint32_t	version;	/* X.Y.Z is encoded in nibbles xxxx.yy.zz */
    uint32_t	sdk;		/* X.Y.Z is encoded in nibbles xxxx.yy.zz */
};

/*
 * The uuid load command contains a single 128-bit unique random number that
 * identifies an object produced by the static link editor.
 */
struct uuid_command {
    uint32_t	cmd;		/* LC_UUID */
    uint32_t	cmdsize;	/* sizeof(struct uuid_command) */
    uint8_t	uuid[16];	/* the 128-bit uuid */
};

////////////////////////////////////////////////////////////////////////////////

struct mach_32bit
{
    typedef ::mach_header mach_header;
    typedef ::segment_command segment_command;
    typedef ::section section;
    typedef uint32_t field;
    typedef int32_t sfield;
    static const int seg_load_command = LC_SEGMENT;
};

struct mach_64bit
{
    typedef ::mach_header_64 mach_header;
    typedef ::segment_command_64 segment_command;
    typedef ::section_64 section;
    typedef uint64_t field;
    typedef int64_t sfield;
    static const int seg_load_command = LC_SEGMENT_64;
};

////////////////////////////////////////////////////////////////////////////////

static void swap_uint32(bool p_to_network, uint32_t& x)
{
	MCDeployByteSwapRecord(p_to_network, "l", &x, sizeof(uint32_t));
}

static void swap_fat_header(bool p_to_network, fat_header& x)
{
	MCDeployByteSwapRecord(p_to_network, "ll", &x, sizeof(fat_header));
}

static void swap_fat_arch(bool p_to_network, fat_arch& x)
{
	MCDeployByteSwapRecord(p_to_network, "lllll", &x, sizeof(fat_arch));
}

static void swap_mach_header(bool p_to_network, mach_header& x)
{
	MCDeployByteSwapRecord(p_to_network, "lllllll", &x, sizeof(mach_header));
}

static void swap_load_command_hdr(bool p_to_network, load_command& x)
{
	MCDeployByteSwapRecord(p_to_network, "ll", &x, sizeof(load_command));
}

static void swap_segment_command(bool p_to_network, segment_command& x)
{
	MCDeployByteSwapRecord(p_to_network, "llbbbbbbbbbbbbbbbbllllllll", &x, sizeof(segment_command));
	for(uint32_t i = 0; i < x . nsects; i++)
	{
		section *t_section;
		t_section = (section *)(&x + 1);
		MCDeployByteSwapRecord(p_to_network, "lllllllll", &t_section -> addr, sizeof(section) - 32);
	}
}

static void unswap_segment_command(bool p_to_network, segment_command& x)
{
	for(uint32_t i = 0; i < x . nsects; i++)
	{
		section *t_section;
		t_section = (section *)(&x + 1);
		MCDeployByteSwapRecord(p_to_network, "lllllllll", &t_section -> addr, sizeof(section) - 32);
	}
	MCDeployByteSwapRecord(p_to_network, "llbbbbbbbbbbbbbbbbllllllll", &x, sizeof(segment_command));
}

static void swap_twolevel_hints_command(bool p_to_network, twolevel_hints_command& x)
{
	MCDeployByteSwapRecord(p_to_network, "llll", &x, sizeof(twolevel_hints_command));
}

static void swap_symtab_command(bool p_to_network, symtab_command& x)
{
	MCDeployByteSwapRecord(p_to_network, "llllll", &x, sizeof(symtab_command));
}

static void swap_dysymtab_command(bool p_to_network, dysymtab_command& x)
{
	MCDeployByteSwapRecord(p_to_network, "llllllllllllllllllll", &x, sizeof(dysymtab_command));
}

static void swap_dylib_command(bool p_to_network, dylib_command& x)
{
	MCDeployByteSwapRecord(p_to_network, "llllll", &x, sizeof(dylib_command));
}

static void swap_nlist(bool p_to_network, nlist& x)
{
	MCDeployByteSwapRecord(p_to_network, "lbbsl", &x, sizeof(nlist));
}

static void swap_relocation_info(bool p_to_network, relocation_info& x)
{
	MCDeployByteSwapRecord(p_to_network, "ll", &x, sizeof(relocation_info));
}

static void swap_version_min_command(bool p_to_network, version_min_command& x)
{
    MCDeployByteSwapRecord(p_to_network, "llll", &x, sizeof(version_min_command));
}

static void swap_load_command(bool p_to_network, uint32_t p_type, load_command* x)
{
	switch(p_type)
	{
		case LC_SEGMENT:
			if (p_type != x -> cmd)
				swap_segment_command(p_to_network, *(segment_command *)x);
			else
				unswap_segment_command(p_to_network, *(segment_command *)x);
			break;

		case LC_TWOLEVEL_HINTS:
			swap_twolevel_hints_command(p_to_network, *(twolevel_hints_command *)x);
			break;

		case LC_SYMTAB:
			swap_symtab_command(p_to_network, *(symtab_command *)x);
			break;

		case LC_DYSYMTAB:
			swap_dysymtab_command(p_to_network, *(dysymtab_command *)x);
			break;
			
		case LC_LOAD_DYLIB:
			swap_dylib_command(p_to_network, *(dylib_command *)x);
			break;

		case LC_UUID:
		case LC_THREAD:
		case LC_UNIXTHREAD:
		case LC_LOAD_DYLINKER:
		case LC_MAIN:
			swap_load_command_hdr(p_to_network, *x);
			break;

        case LC_VERSION_MIN_MACOSX:
        case LC_VERSION_MIN_IPHONEOS:
        case LC_BUILD_VERSION:
            swap_version_min_command(p_to_network, *(version_min_command *)x);
            break;
		
        default:
			swap_load_command_hdr(p_to_network, *x);
			break;
	}
}

template<typename T>static void relocate_segment_command(typename T::segment_command *x, typename T::sfield p_file_delta, typename T::sfield p_address_delta)
{
	x -> vmaddr += p_address_delta;
	x -> fileoff += p_file_delta;

	for(uint32_t i = 0; i < x -> nsects; i++)
	{
		section *t_section;
		t_section = (section *)(x + 1);
		t_section -> addr += p_address_delta;
		t_section -> offset += p_file_delta;
		t_section -> reloff += p_file_delta;
	}
}

static void relocate_twolevel_hints_command(twolevel_hints_command *x, int32_t p_file_delta, int32_t p_address_delta)
{
	x -> offset += p_file_delta;
}

static void relocate_symtab_command(symtab_command *x, int32_t p_file_delta, int32_t p_address_delta)
{
	if (x -> symoff != 0)
		x -> symoff += p_file_delta;
	if (x -> stroff != 0)
		x -> stroff += p_file_delta;
}

static void relocate_dysymtab_command(dysymtab_command *x, int32_t p_file_delta, int32_t p_address_delta)
{
	if (x -> tocoff != 0)
		x -> tocoff += p_file_delta;
	if (x -> modtaboff != 0)
		x -> modtaboff += p_file_delta;
	if (x -> extrefsymoff != 0)
		x -> extrefsymoff += p_file_delta;
	if (x -> indirectsymoff != 0)
		x -> indirectsymoff += p_file_delta;
	if (x -> extreloff != 0)
		x -> extreloff += p_file_delta;
	if (x -> locreloff != 0)
		x -> locreloff += p_file_delta;
}

static void relocate_code_signature_command(linkedit_data_command *x, int32_t p_file_delta, int32_t p_address_delta)
{
	x -> dataoff += p_file_delta;
}

static void relocate_dyld_info_command(dyld_info_command *x, int32_t p_file_delta, int32_t p_address_delta)
{
	if (x -> rebase_off != 0)
		x -> rebase_off += p_file_delta;
	if (x -> bind_off != 0)
		x -> bind_off += p_file_delta;
	if (x -> weak_bind_off != 0)
		x -> weak_bind_off += p_file_delta;
	if (x -> lazy_bind_off != 0)
		x -> lazy_bind_off += p_file_delta;
	if (x -> export_off != 0)
		x -> export_off += p_file_delta;
}

static void relocate_function_starts_command(linkedit_data_command *x, int32_t p_file_delta, int32_t p_address_delta)
{
	x -> dataoff += p_file_delta;
}


/*static void relocate_encryption_info_command(encryption_info_command *x, int32_t p_file_data, int32_t p_address_delta)
{
}*/

////////////////////////////////////////////////////////////////////////////////

// MW-2014-10-02: [[ Bug 13536 ]] iOS Segment alignment has to be 16k now - this
//   should be made a parameter at some point. Additionally, for FAT binary
//   generation, the existing alignment in the header should be followed for
//   binary concatenation.
#define MACHO_ALIGNMENT 16384
#define MACHO_ALIGN(x) ((x + MACHO_ALIGNMENT - 1) & ~(MACHO_ALIGNMENT - 1))

////////////////////////////////////////////////////////////////////////////////

static MCDeployArchitecture MCDeployMachArchToDeployArchitecture(cpu_type_t p_type, cpu_subtype_t p_subtype)
{
    MCDeployArchitecture t_arch;
    if (p_type == CPU_TYPE_X86)
        t_arch = kMCDeployArchitecture_I386;
    else if (p_type == CPU_TYPE_X86_64)
        t_arch = kMCDeployArchitecture_X86_64;
    else if (p_type == CPU_TYPE_ARM && p_subtype == CPU_SUBTYPE_ARM_V6)
        t_arch = kMCDeployArchitecture_ARMV6;
    else if (p_type == CPU_TYPE_ARM && p_subtype == CPU_SUBTYPE_ARM_V7)
        t_arch = kMCDeployArchitecture_ARMV7;
    else if (p_type == CPU_TYPE_ARM && p_subtype == CPU_SUBTYPE_ARM_V7S)
        t_arch = kMCDeployArchitecture_ARMV7S;
    else if (p_type == CPU_TYPE_ARM64)
        t_arch = kMCDeployArchitecture_ARM64;
    else if (p_type == CPU_TYPE_POWERPC)
        t_arch = kMCDeployArchitecture_PPC;
    else if (p_type == CPU_TYPE_POWERPC64)
        t_arch = kMCDeployArchitecture_PPC64;
    else
        t_arch = kMCDeployArchitecture_Unknown;
    
    return t_arch;
}

static bool MCDeployToMacOSXFetchMinOSVersion(const MCDeployParameters& p_params, mach_header& p_header, uint32_t& r_version)
{
    // First work out what DeployArchitecture to look for.
    MCDeployArchitecture t_arch;
    t_arch = MCDeployMachArchToDeployArchitecture(p_header.cputype, p_header.cpusubtype);
    
    // Search for both the architecture in the mach header and for the 'unknown'
    // architecture. If the real arch is found, then we use that version; otherwise
    // if there is an unknown arch then we use that version. If neither are found we
    // return false which means the caller can do nothing.
    int t_unknown_index, t_found_index;
    t_unknown_index = -1;
    t_found_index = -1;
    for(uindex_t i = 0; i < p_params . min_os_version_count; i++)
        if (p_params . min_os_versions[i] . architecture == t_arch &&
            t_found_index < 0)
            t_found_index = (signed)i;
        else if (p_params . min_os_versions[i] . architecture == kMCDeployArchitecture_Unknown &&
                 t_unknown_index < 0)
            t_unknown_index = (signed)i;
    
    if (t_found_index < 0 && t_unknown_index < 0)
        return false;
    
    if (t_found_index >= 0)
    {
        r_version = p_params . min_os_versions[t_found_index] . version;
        return true;
    }
    
    r_version = p_params . min_os_versions[t_unknown_index] . version;
    return true;
}

template<typename T> bool MCDeployToMacOSXMainBody(const MCDeployParameters& p_params, bool p_big_endian, MCDeployFileRef p_engine, uint32_t p_engine_offset, uint32_t t_engine_size, uint32_t& x_offset, MCDeployFileRef p_output, mach_header& t_header, load_command **t_commands, uint32_t t_command_count)
{
    bool t_success;
    t_success = true;
    
	// MW-2013-05-04: [[ iOSDeployMisc ]] Search for a rogue 'misc' segment.
	// Next check that the executable is in a form we can work with
	typename T::segment_command *t_linkedit_segment, *t_project_segment, *t_payload_segment, *t_misc_segment;
	uint32_t t_linkedit_index, t_project_index, t_payload_index, t_misc_index;
	t_linkedit_segment = t_project_segment = t_payload_segment = t_misc_segment = NULL;
	if (t_success)
		for(uint32_t i = 0; i < t_header . ncmds; i++)
			if (t_commands[i] -> cmd == T::seg_load_command)
			{
				typename T::segment_command *t_command;
				t_command = (typename T::segment_command *)t_commands[i];
				if (memcmp(t_command -> segname, "__LINKEDIT", 11) == 0)
				{
					t_linkedit_index = i;
					t_linkedit_segment = t_command;
				}
				else if (memcmp(t_command -> segname, "__PROJECT", 10) == 0)
				{
					t_project_index = i;
					t_project_segment = t_command;
				}
				else if (memcmp(t_command -> segname, "__PAYLOAD", 10) == 0)
				{
					t_payload_index = i;
					t_payload_segment = t_command;
				}
				else if (memcmp(t_command -> segname, "__MISC", 6) == 0)
				{
					t_misc_index = i;
					t_misc_segment = t_command;
				}
			}
    
	// Make sure we have both segments we think should be there
	if (t_success && t_linkedit_segment == NULL)
		t_success = MCDeployThrow(kMCDeployErrorMacOSXNoLinkEditSegment);
	if (t_success && t_project_segment == NULL)
		t_success = MCDeployThrow(kMCDeployErrorMacOSXNoProjectSegment);
	if (t_success && !(MCStringIsEmpty(p_params . payload)) && t_payload_segment == NULL)
		t_success = MCDeployThrow(kMCDeployErrorMacOSXNoPayloadSegment);
	
	// MW-2013-05-04: [[ iOSDeployMisc ]] If 'misc' segment is present but not between
	//   linkedit and project, then all is well.
	if (t_success &&
		t_misc_segment != nil && (t_linkedit_index != t_misc_index + 1 || t_misc_index != t_project_index + 1))
		t_misc_segment = nil;
	
	// MW-2013-05-04: [[ iOSDeployMisc ]] Check 'misc' segment (if potentially an issue) is
	//   in a place we can handle it.
	// Make sure linkedit follows project, or linkedit follows misc, follows project
	if (t_success &&
		(t_linkedit_index != t_project_index + 1) &&
		(t_misc_segment != nil && (t_linkedit_index != t_misc_index + 1 || t_misc_index != t_project_index + 1)))
		t_success = MCDeployThrow(kMCDeployErrorMacOSXBadSegmentOrder);
    
	// Make sure project follows payload (if required)
	if (t_success && t_payload_segment != NULL && t_project_index != t_payload_index + 1)
		t_success = MCDeployThrow(kMCDeployErrorMacOSXBadSegmentOrder);
    
	// Make sure no segments follow linkedit
	if (t_success)
		for(uint32_t i = t_linkedit_index + 1; i < t_header . ncmds && t_success; i++)
			if (t_commands[i] -> cmd == T::seg_load_command)
				t_success = MCDeployThrow(kMCDeployErrorMacOSXBadSegmentOrder);
    
	// Next we write out everything up to the beginning of the project (or payload)
	// segment, followed by our new segments themselves. This gives us the info we
	// need to recalculate the required header info.
    
	typename T::field t_output_offset;
	t_output_offset = 0;
	if (t_success)
	{
		if (t_payload_segment == NULL)
			t_output_offset = t_project_segment -> fileoff;
		else
			t_output_offset = t_payload_segment -> fileoff;
	}
    
	// Write out the original data up to the beginning of the project section.
	if (t_success)
		t_success = MCDeployFileCopy(p_output, x_offset, p_engine, p_engine_offset, t_output_offset);
    
	// Write out the payload segment (if necessary)
	typename T::field t_payload_offset;
    uint32_t t_payload_size;
	t_payload_offset = 0;
	t_payload_size = 0;
	if (t_success && t_payload_segment != NULL)
	{
		t_payload_offset = x_offset + t_output_offset;
		if (!MCStringIsEmpty(p_params . payload))
			t_success = MCDeployWritePayload(p_params, p_big_endian, p_output, t_payload_offset, t_payload_size);
        // MW-2014-10-02: [[ Bug 13536 ]] Use macro to align to required alignment.
		if (t_success)
			t_output_offset += MACHO_ALIGN(t_payload_size);
	}
    
	// Write out the project info struct
	typename T::field t_project_offset;
    uint32_t t_project_size;
	t_project_offset = 0;
	t_project_size = 0;
	if (t_success)
	{
		t_project_offset = x_offset + t_output_offset;
		t_success = MCDeployWriteProject(p_params, p_big_endian, p_output, t_project_offset, t_project_size);
	}
    
	// Now we update the relevant pieces of the header and command table.
	typename T::field t_old_linkedit_offset, t_old_project_offset, t_old_payload_offset;
	if (t_success)
	{
        // MW-2014-10-02: [[ Bug 13536 ]] Use macro to align to required alignment.
		t_project_size = MACHO_ALIGN(t_project_size);
		t_old_project_offset = t_project_segment -> fileoff;
		t_project_segment -> filesize = t_project_size;
		t_project_segment -> vmsize = t_project_size;
		((typename T::section *)(t_project_segment + 1))[0] . size = t_project_size;
        
		if (t_payload_segment != nil)
		{
            // MW-2014-10-02: [[ Bug 13536 ]] Use macro to align to required alignment.
			t_payload_size = MACHO_ALIGN(t_payload_size);
			t_old_payload_offset = t_payload_segment -> fileoff;
			t_payload_segment -> filesize = t_payload_size;
			t_payload_segment -> vmsize = t_payload_size;
			((typename T::section *)(t_payload_segment + 1))[0] . size = t_payload_size;
			relocate_segment_command<T>(t_project_segment, (t_payload_segment -> fileoff + t_payload_size) - t_old_project_offset, (t_payload_segment -> fileoff + t_payload_size) - t_old_project_offset);
		}
		
		// MW-2013-05-04: [[ iOSDeployMisc ]] If there is a 'misc' segment then use a different
		//   'post' project offset (which will include 'misc').
		if (t_misc_segment == nil)
			t_old_linkedit_offset = t_linkedit_segment -> fileoff;
		else
			t_old_linkedit_offset = t_misc_segment -> fileoff;
        
		// Now go through, updating the offsets for all load commands after
		// and including linkedit. We also update the uuid load command here,
        // if one has been provided.
		typename T::sfield t_file_delta, t_address_delta;
		t_file_delta = (t_project_segment -> fileoff + t_project_size) - t_old_linkedit_offset;
		t_address_delta = t_file_delta;
		for(uint32_t i = t_linkedit_index; i < t_header . ncmds && t_success; i++)
		{
			switch(t_commands[i] -> cmd)
			{
                // Relocate the commands we know about that contain file offset
                case LC_SEGMENT:
                    relocate_segment_command<mach_32bit>((segment_command *)t_commands[i], t_file_delta, t_address_delta);
                    break;
                case LC_SEGMENT_64:
                    relocate_segment_command<mach_64bit>((segment_command_64 *)t_commands[i], t_file_delta, t_address_delta);
                    break;
                case LC_TWOLEVEL_HINTS:
                    relocate_twolevel_hints_command((twolevel_hints_command *)t_commands[i], t_file_delta, t_address_delta);
                    break;
                case LC_SYMTAB:
                    relocate_symtab_command((symtab_command *)t_commands[i], t_file_delta, t_address_delta);
                    break;
                case LC_DYSYMTAB:
                    relocate_dysymtab_command((dysymtab_command *)t_commands[i], t_file_delta, t_address_delta);
                    break;
                case LC_CODE_SIGNATURE:
                    relocate_code_signature_command((linkedit_data_command *)t_commands[i], t_file_delta, t_address_delta);
                    break;
                case LC_DYLD_INFO:
                case LC_DYLD_INFO_ONLY:
                    relocate_dyld_info_command((dyld_info_command *)t_commands[i], t_file_delta, t_address_delta);
                    break;
                case LC_FUNCTION_STARTS:
                case LC_DYLIB_CODE_SIGN_DRS:
                case LC_DATA_IN_CODE:
                    relocate_function_starts_command((linkedit_data_command *)t_commands[i], t_file_delta, t_address_delta);
                    break;
                    
                // Update the uuid, if one has been provided.
                case LC_UUID:
                    if (!MCStringIsEmpty(p_params.uuid))
                    {
                        MCAutoStringRefAsCString t_uuid_cstring;
                        MCUuid t_uuid;
                        if (t_uuid_cstring.Lock(p_params.uuid) &&
                            MCUuidFromCString(*t_uuid_cstring, t_uuid))
                        {
                            uuid_command *t_uuid_cmd = (uuid_command *)t_commands[i];
                            MCUuidToBytes(t_uuid, t_uuid_cmd->uuid);
                        }
                        else
                            t_success = MCDeployThrow(kMCDeployErrorInvalidUuid);
                    }
                    break;
                    
                // These commands have no file offsets
                case LC_THREAD:
                case LC_UNIXTHREAD:
                case LC_LOAD_DYLIB:
                case LC_LOAD_WEAK_DYLIB:
                case LC_LOAD_DYLINKER:
                case LC_ENCRYPTION_INFO:
                case LC_ENCRYPTION_INFO_64:
                case LC_SOURCE_VERSION:
                case LC_MAIN:
                case LC_BUILD_VERSION:
                    break;
                    
                // We rewrite the contents of these commands as appropriate to
                // the 'min_os_versions' list in the params.
                case LC_VERSION_MIN_MACOSX:
                case LC_VERSION_MIN_IPHONEOS:
                {
                    // Notice that we leave the SDK version alone - this is tied
                    // to linkage and so is probably unwise to adjust.
                    uint32_t t_version;
                    if (MCDeployToMacOSXFetchMinOSVersion(p_params, t_header, t_version))
                        ((version_min_command *)t_commands[i]) -> version = t_version;
                }
                break;
                    
                // Any others that are present are an error since we don't know
                // what to do with them.
                default:
                    t_success = MCDeployThrow(kMCDeployErrorMacOSXUnknownLoadCommand);
                    break;
			}
		}
	}
    
	// Overwrite the mach header
	if (t_success)
	{
        if (p_big_endian)
            swap_mach_header(p_big_endian, t_header);
        
        typename T::mach_header t_actual_header;
        memset(&t_actual_header, 0, sizeof(t_actual_header));
        memcpy(&t_actual_header, &t_header, sizeof(t_header));
        
		t_success = MCDeployFileWriteAt(p_output, &t_actual_header, sizeof(t_actual_header), x_offset);
	}
    
	// Overwrite the load commands array
	if (t_success)
	{
		typename T::field t_offset;
		t_offset = x_offset + sizeof(typename T::mach_header);
		for(uint32_t i = 0; i < t_command_count && t_success; i++)
		{
			typename T::field t_size;
			t_size = t_commands[i] -> cmdsize;
			swap_load_command(p_big_endian, t_commands[i] -> cmd, t_commands[i]);
			t_success = MCDeployFileWriteAt(p_output, t_commands[i], t_size, t_offset);
			if (t_success)
				t_offset += t_size;
		}
	}
    
	// Output the linkedit segment and beyond.
	if (t_success)
		t_success = MCDeployFileCopy(p_output, t_project_offset + t_project_size, p_engine, p_engine_offset + t_old_linkedit_offset, t_engine_size - t_old_linkedit_offset);
    
	// Update the offset in case we are producing a fat binary.
	if (t_success)
		x_offset += t_output_offset + t_project_size + (t_engine_size - t_old_linkedit_offset);
    
	// Free the temporary data structures we needed.
	if (t_commands != NULL)
	{
		for(uint32_t i = 0; i < t_command_count; i++)
			free(t_commands[i]);
		delete[] t_commands;
	}
    
	return t_success;
}

typedef bool (*MCDeployValidateHeaderCallback)(const MCDeployParameters& params, mach_header& header, load_command **commands);

static bool MCDeployToMacOSXReadHeader(bool p_big_endian, MCDeployFileRef p_engine, uint32_t p_offset, mach_header& r_header, load_command**& r_commands)
{
	// Read the mach-o header.
	if (!MCDeployFileReadAt(p_engine, &r_header, sizeof(mach_header), p_offset))
		return MCDeployThrow(kMCDeployErrorMacOSXNoHeader);

    if (p_big_endian)
        swap_mach_header(p_big_endian, r_header);

	// Validate the header
	if ((r_header . magic != MH_MAGIC && r_header . magic != MH_MAGIC_64) ||
		(r_header . filetype != MH_EXECUTE && r_header . filetype != MH_BUNDLE && r_header . filetype != MH_OBJECT))
		return MCDeployThrow(kMCDeployErrorMacOSXBadHeader);

	// Allocate memory for the load commands
	MCAutoCustomPointerArray<load_command*, MCMemoryDelete> t_commands;
	if (!t_commands.New(r_header . ncmds))
		return MCDeployThrow(kMCDeployErrorNoMemory);

	// And read them in
	uint32_t t_offset;
    if (r_header . magic == MH_MAGIC)
        t_offset = p_offset + sizeof(mach_header);
    else
        t_offset = p_offset + sizeof(mach_header_64);
	for(uint32_t i = 0; i < r_header . ncmds; i++)
	{
		// First read the command header
		load_command t_command;
		if (!MCDeployFileReadAt(p_engine, &t_command, sizeof(load_command), t_offset))
			return MCDeployThrow(kMCDeployErrorMacOSXBadCommand);
        if (p_big_endian)
            swap_load_command_hdr(p_big_endian, t_command);

		// Now allocate memory for the full command record
		if (!MCMemoryAllocate(t_command . cmdsize, t_commands[i]))
			return MCDeployThrow(kMCDeployErrorNoMemory);

		// Read in all of it
		if (!MCDeployFileReadAt(p_engine, t_commands[i], t_command . cmdsize, t_offset))
			return MCDeployThrow(kMCDeployErrorMacOSXBadCommand);

		// And swap if we are actually interested in the contents otherwise
		// just swap the header.
        if (p_big_endian)
            swap_load_command(p_big_endian, t_command . cmd, t_commands[i]);

		// Move to the next command
		t_offset += t_command . cmdsize;
	}

	uindex_t t_command_count;
	t_commands.Take(r_commands, t_command_count);
	return true;
}

// The method is the work-horse of Mac OS X deployment - it builds an exe for
// either PPC or x86 depending on the value of <ppc>. The resulting binary is
// written to output at the given offset, and this var is updated with the offset
// of the end of the binary.
//
// The Mach-O format consists of a header followed by a sequence of load
// commands. Each load command can perform a number of actions, the main one
// begine that of defining a segment. A so-called LC_SEGMENT command consists of
// a sequence of sections that map sequentially into the virtual memory range
// defined by the segment.
//
// The overall structure of the standalone engine as built for the purpose of
// deployment is:
//   header
//   load segment __PAGEZERO
//   load segment __TEXT
//     <main exe read-only sections>
//   load segment __DATA
//     <main exe read-write sections>
//   load segment __OBJC
//     <objective-C related data>
//   load segment __PAYLOAD
//     <our sections for insertion of removable data>
//   load segment __PROJECT
//     <our sections for insertion of rev data>
//   load segment __LINKEDIT
//     <system dynamic linking data>
//   <non mapped secion load commands>
//
// Thus (hopefully) all we need to do is parse the load commands, look for
// the one that loads the '__PROJECT' segment, rewrite that and update the
// location of the __LINKEDIT section.
//

// MW-2011-09-19: Updated to take an engine size/offset to support universal
//   binary building on iOS.
static bool MCDeployToMacOSXMain(const MCDeployParameters& p_params, bool p_big_endian, MCDeployFileRef p_engine, uint32_t p_engine_offset, uint32_t p_engine_size, uint32_t& x_offset, MCDeployFileRef p_output, MCDeployValidateHeaderCallback p_validate_header_callback)
{
	bool t_success;
	t_success = true;

	// Calculate the size of the input engine (in theory we could calculate this
	// from the segments/load commands but thats a little fiddly).
	uint32_t t_engine_size;
	if (p_engine_size != 0)
		t_engine_size = p_engine_size;
	else if (t_success && !MCDeployFileMeasure(p_engine, t_engine_size))
		t_success = MCDeployThrow(kMCDeployErrorBadFile);

	// Read in the header and load command list
    mach_header t_header;
	load_command **t_commands;
	uint32_t t_command_count;
	t_commands = NULL;
	if (t_success)
	{
		t_success = MCDeployToMacOSXReadHeader(p_big_endian, p_engine, p_engine_offset, t_header, t_commands);
		t_command_count = t_header . ncmds;
	}

	if (t_success && p_validate_header_callback != nil)
		t_success = p_validate_header_callback(p_params, t_header, t_commands);

    if (t_success)
    {
        if ((t_header . cputype & CPU_ARCH_ABI64) == 0)
            t_success = MCDeployToMacOSXMainBody<mach_32bit>(p_params, p_big_endian, p_engine, p_engine_offset, t_engine_size, x_offset, p_output, t_header, t_commands, t_command_count);
        else
            t_success = MCDeployToMacOSXMainBody<mach_64bit>(p_params, p_big_endian, p_engine, p_engine_offset, t_engine_size, x_offset, p_output, t_header, t_commands, t_command_count);
    }
    
    return t_success;
}

#if LEGACY_EMBEDDED_DEPLOY
static bool MCDeployToMacOSXEmbedded(const MCDeployParameters& p_params, bool p_big_endian, MCDeployFileRef p_engine, uint32_t p_engine_offset, uint32_t p_engine_size, uint32_t& x_offset, MCDeployFileRef p_output)
{
	bool t_success;
	t_success = true;
	
	// Calculate the size of the input engine (in theory we could calculate this
	// from the segments/load commands but thats a little fiddly).
	uint32_t t_engine_size;
	if (p_engine_size != 0)
		t_engine_size = p_engine_size;
	else if (t_success && !MCDeployFileMeasure(p_engine, t_engine_size))
		t_success = MCDeployThrow(kMCDeployErrorBadFile);
	
	// Read in the header and load command list
	mach_header t_header;
	load_command **t_commands;
	uint32_t t_command_count;
	t_commands = NULL;
	if (t_success)
	{
		t_success = MCDeployToMacOSXReadHeader(p_big_endian, p_engine, p_engine_offset, t_header, t_commands);
		t_command_count = t_header . ncmds;
	}
	
	if (t_header . filetype != MH_OBJECT)
		t_success = MCDeployThrow(kMCDeployErrorBadFile);
	
	// Next check that the object file is in a form we can work with
	segment_command *t_main_segment, *t_last_segment;
	section *t_project_section, *t_last_section;
	uindex_t t_main_segment_index;
	t_main_segment = t_last_segment = NULL;
	t_project_section = t_last_section = NULL;
	if (t_success)
		for(uindex_t i = 0; i < t_header . ncmds; i++)
			if (t_commands[i] -> cmd == LC_SEGMENT)
			{
				segment_command *t_command;
				t_command = (segment_command *)t_commands[i];
				
				// This is the latest segment in the load command list.
				t_last_segment = t_command;
				
				// If we've already found the project section, continue.
				if (t_project_section != NULL)
					break;
				
				// Check to see if it contains the project section.
				for(uindex_t j = 0; j < t_command -> nsects; j++)
				{
					section *t_section;
					t_section = &((section *)(t_command + 1))[j];
					if (memcmp(t_section -> segname, "__PROJECT", 10) == 0)
					{
						t_main_segment = t_command;
						t_main_segment_index = i;
						t_project_section = t_section;
					}
					
					t_last_section = t_section;
				}
			}

	// Make sure we have found a project section.
	if (t_success && t_project_section == NULL)
		t_success = MCDeployThrow(kMCDeployErrorMacOSXNoProjectSegment);
	
	// Make sure the last segment is the main segment.
	if (t_success && (t_last_segment != t_main_segment))
		t_success = MCDeployThrow(kMCDeployErrorMacOSXBadSegmentOrder);
	
	// Next we write out everything up to the beginning of the project (or payload)
	// segment, followed by our new segments themselves. This gives us the info we
	// need to recalculate the required header info.
	
	uint32_t t_output_offset;
	t_output_offset = 0;
	if (t_success)
		t_output_offset = t_project_section -> offset;
	
	// Write out the original data up to the beginning of the project section.
	if (t_success)
		t_success = MCDeployFileCopy(p_output, x_offset, p_engine, p_engine_offset, t_output_offset);
	
	// Write out the project info struct
	uint32_t t_project_size, t_project_offset;
	t_project_offset = 0;
	t_project_size = 0;
	if (t_success)
	{
		t_project_offset = x_offset + t_output_offset;
		t_success = MCDeployWriteProject(p_params, p_big_endian, p_output, t_project_offset, t_project_size);
	}
	
	// Now we update the relevant pieces of the header and command table.
	uint32_t t_old_project_end_offset;
	if (t_success)
	{
		t_old_project_end_offset = t_project_section -> offset + t_project_section -> size;
		
        // MW-2014-10-02: [[ Bug 13536 ]] Use macro to align to required alignment.
		t_project_size = MACHO_ALIGN(t_project_size);
			
		int32_t t_file_delta, t_address_delta;
		t_file_delta = t_project_size - t_project_section -> size;
		t_address_delta = t_file_delta;
		
		t_project_section -> size = t_project_size;
		t_main_segment -> filesize += t_file_delta;
		t_main_segment -> vmsize += t_file_delta;
		
		// Update all the offsets in sections after the project section. This
		// is needed as in simulator builds we get an 'unwind' section after the
		// project.
		for(section *t_section = t_project_section + 1; t_section <= t_last_section; t_section++)
		{
			if (t_section -> offset > t_output_offset)
				t_section -> offset += t_file_delta;
			if (t_section -> reloff > t_output_offset)
				t_section -> reloff += t_file_delta;
		}
		
		// Now go through, updating the offsets for all load commands after
		// and including linkedit.
		for(uindex_t i = t_main_segment_index + 1; i < t_header . ncmds && t_success; i++)
		{
			switch(t_commands[i] -> cmd)
			{
					// Relocate the commands we know about that contain file offset
				case LC_SEGMENT:
					relocate_segment_command((segment_command *)t_commands[i], t_file_delta, t_address_delta);
					break;
				case LC_TWOLEVEL_HINTS:
					relocate_twolevel_hints_command((twolevel_hints_command *)t_commands[i], t_file_delta, t_address_delta);
					break;
				case LC_SYMTAB:
					relocate_symtab_command((symtab_command *)t_commands[i], t_file_delta, t_address_delta);
					break;
				case LC_DYSYMTAB:
					relocate_dysymtab_command((dysymtab_command *)t_commands[i], t_file_delta, t_address_delta);
					break;
				case LC_CODE_SIGNATURE:
					relocate_code_signature_command((linkedit_data_command *)t_commands[i], t_file_delta, t_address_delta);
					break;
				case LC_DYLD_INFO:
				case LC_DYLD_INFO_ONLY:
					relocate_dyld_info_command((dyld_info_command *)t_commands[i], t_file_delta, t_address_delta);
					break;
				case LC_FUNCTION_STARTS:
				case LC_DYLIB_CODE_SIGN_DRS:
					relocate_function_starts_command((linkedit_data_command *)t_commands[i], t_file_delta, t_address_delta);
					break;
					
					// These commands have no file offsets
				case LC_UUID:
				case LC_THREAD:
				case LC_UNIXTHREAD:
				case LC_LOAD_DYLIB:
				case LC_LOAD_WEAK_DYLIB:
				case LC_LOAD_DYLINKER:
				case LC_ENCRYPTION_INFO:
				case LC_VERSION_MIN_MACOSX:
				case LC_VERSION_MIN_IPHONEOS:
				case LC_SOURCE_VERSION:
					break;
					
					// Any others that are present are an error since we don't know
					// what to do with them.
				default:
					t_success = MCDeployThrow(kMCDeployErrorMacOSXUnknownLoadCommand);
					break;
			}
		}
		
		// Now go through the initial segment command(s) and make sure we update
		// the relocation offsets.
		for(uindex_t i = 0; i <= t_main_segment_index && t_success; i++)
		{
			if (t_commands[i] -> cmd != LC_SEGMENT)
				continue;
			
			segment_command *t_segment;
			t_segment = (segment_command *)t_commands[i];
			for(uindex_t j = 0; j < t_segment -> nsects; j++)
			{
				section *t_section;
				t_section = &((section *)(t_segment + 1))[j];
				if (t_section -> reloff > t_output_offset)
					t_section -> reloff += t_file_delta;
			}
		}
	}
	
	// Overwrite the mach header
	if (t_success)
	{
		swap_mach_header(p_big_endian, t_header);
		t_success = MCDeployFileWriteAt(p_output, &t_header, sizeof(mach_header), x_offset);
	}
	
	// Overwrite the load commands array
	if (t_success)
	{
		uint32_t t_offset;
		t_offset = x_offset + sizeof(mach_header);
		for(uint32_t i = 0; i < t_command_count && t_success; i++)
		{
			uint32_t t_size;
			t_size = t_commands[i] -> cmdsize;
			swap_load_command(p_big_endian, t_commands[i] -> cmd, t_commands[i]);
			t_success = MCDeployFileWriteAt(p_output, t_commands[i], t_size, t_offset);
			if (t_success)
				t_offset += t_size;
		}
	}
	
	// Output the linkedit segment and beyond.
	if (t_success)
		t_success = MCDeployFileCopy(p_output, t_project_offset + t_project_size, p_engine, p_engine_offset + t_old_project_end_offset, t_engine_size - t_old_project_end_offset);
	
	// Update the offset in case we are producing a fat binary.
	if (t_success)
		x_offset += t_output_offset + t_project_size + t_engine_size - t_old_project_end_offset;
	
	// Free the temporary data structures we needed.
	if (t_commands != NULL)
	{
		for(uint32_t i = 0; i < t_command_count; i++)
			free(t_commands[i]);
		delete[] t_commands;
	}
	
	return t_success;
}
#endif

// MW-2013-06-13: This method builds a capsule into each part of a (potentially) fat binary.
static bool MCDeployToMacOSXFat(const MCDeployParameters& p_params, bool p_embedded, MCDeployFileRef p_engine, MCDeployFileRef p_output, MCDeployValidateHeaderCallback p_validate_header_callback)
{
	bool t_success;
	t_success = true;
	
	// Next read in the fat header.
	fat_header t_fat_header;
	if (t_success && !MCDeployFileReadAt(p_engine, &t_fat_header, sizeof(fat_header), 0))
		t_success = MCDeployThrow(kMCDeployErrorMacOSXNoHeader);
	
	// Swap the header - note that the 'fat_*' structures are always in network
	// byte-order.
	if (t_success)
		swap_fat_header(true, t_fat_header);
	
	// If this isn't a fat binary, then we assume its just a single arch binary.
	if (t_success && t_fat_header . magic != FAT_MAGIC && t_fat_header . magic != FAT_CIGAM)
	{
		uint32_t t_output_offset;
		t_output_offset = 0;
		if (!p_embedded)
            t_success = MCDeployToMacOSXMain(p_params, false, p_engine, 0, 0, t_output_offset, p_output, p_validate_header_callback);
#if LEGACY_EMBEDDED_DEPLOY
		else
			t_success = MCDeployToMacOSXEmbedded(p_params, false, p_engine, 0, 0, t_output_offset, p_output);
#endif
	}
	else
	{
		// The output offset for the slice is aligned up to fat_arch.align after the
        // fat header.
		uint32_t t_output_offset;
		t_output_offset = sizeof(fat_header);
		
		// The fat_arch structures follow the fat header directly
        uint32_t t_header_read_offset, t_header_write_offset;
		t_header_read_offset = t_header_write_offset = sizeof(fat_header);
		
		// Loop through all the fat headers.
        uint32_t t_slice_count = 0;
		for(uint32_t i = 0; i < t_fat_header . nfat_arch && t_success; i++)
		{
			fat_arch t_fat_arch;
			if (!MCDeployFileReadAt(p_engine, &t_fat_arch, sizeof(fat_arch), t_header_read_offset))
				t_success = MCDeployThrow(kMCDeployErrorMacOSXBadHeader);
			
            // Ensure the header has the appropriate byte order
            if (t_success)
                swap_fat_arch(true, t_fat_arch);
            
            // Is this slice for an architecture we want to keep?
            bool t_want_slice = true;
            if (t_success && p_params.architectures.Size() > 0)
            {
                // Get the architecture for this slice and check whether it is
                // in the list of desired slices or not
                t_want_slice = false;
                MCDeployArchitecture t_arch = MCDeployMachArchToDeployArchitecture(t_fat_arch.cputype, t_fat_arch.cpusubtype);
                for (uindex_t j = 0; j < p_params.architectures.Size(); j++)
                {
                    if (p_params.architectures[j] == t_arch)
                    {
                        t_want_slice = true;
                        break;
                    }
                }
            }
            
            // Do we want to keep this architecture?
            if (t_want_slice)
            {
                uint32_t t_last_output_offset;
                if (t_success)
                {
                    // Round the end of the last engine up to the nearest page boundary.
                    t_output_offset = (t_output_offset + ((1 << t_fat_arch . align))) & ~((1 << t_fat_arch . align) - 1);
                    
                    // Record the end of the last engine.
                    t_last_output_offset = t_output_offset;
                    
                    // Write out this arch's portion.
                    if (!p_embedded)
                        t_success = MCDeployToMacOSXMain(p_params, false, p_engine, t_fat_arch . offset, t_fat_arch . size, t_output_offset, p_output, p_validate_header_callback);
    #if LEGACY_EMBEDDED_DEPLOY
                    else
                        t_success = MCDeployToMacOSXEmbedded(p_params, false, p_engine, 0, 0, t_output_offset, p_output);
    #endif
                }
                
                if (t_success)
                {
                    // Update the fat header.
                    t_fat_arch . offset = t_last_output_offset;
                    t_fat_arch . size = t_output_offset - t_last_output_offset;
                    
                    // Put it back to network byte order.
                    swap_fat_arch(true, t_fat_arch);
                    
                    // Write out the header.
                    t_success = MCDeployFileWriteAt(p_output, &t_fat_arch, sizeof(t_fat_arch), t_header_write_offset);
                }
                
                // We've written another slice
                t_slice_count++;
                t_header_write_offset += sizeof(fat_arch);
            }
			
            t_header_read_offset += sizeof(fat_arch);
		}
		
		// Final step is to update the fat header.
		if (t_success)
		{
            t_fat_header.nfat_arch = t_slice_count;
            swap_fat_header(true, t_fat_header);
			t_success = MCDeployFileWriteAt(p_output, &t_fat_header, sizeof(t_fat_header), 0);
		}
	}
	
	return true;
}

// This method verifies that the given engine is for Mac. It checks the CPU is
// is either PPC or x86, and that the executable loads Cocoa.
static bool MCDeployValidateMacEngine(const MCDeployParameters& p_params, mach_header& p_header, load_command **p_commands)
{
	// Check the CPU type is PowerPC or X86
	if (p_header . cputype != CPU_TYPE_POWERPC &&
		p_header . cputype != CPU_TYPE_X86 &&
        p_header . cputype != CPU_TYPE_X86_64)
		return MCDeployThrow(kMCDeployErrorMacOSXBadCpuType);

	// Check that Cocoa is one of the libraries linked to
	bool t_found_cocoa;
	t_found_cocoa = false;
	for(uint32_t i = 0; i < p_header . ncmds; i++)
		if (p_commands[i] -> cmd == LC_LOAD_DYLIB)
		{
			dylib_command *t_command;
			t_command = (dylib_command *)p_commands[i];

			const char *t_name;
			t_name = (const char *)t_command + t_command -> dylib . name . offset;
			if (MCCStringBeginsWith(t_name, "/System/Library/Frameworks/Cocoa.framework"))
				t_found_cocoa = true;
		}

	if (!t_found_cocoa)
		return MCDeployThrow(kMCDeployErrorMacOSXBadTarget);

	return true;
}

// This method attempts to build a Mac OS X standalone using the given deployment
// parameters.
//
Exec_stat MCDeployToMacOSX(const MCDeployParameters& p_params)
{
	bool t_success;
	t_success = true;

	// MW-2013-06-13: First check that we either have 'engine' or (ppc and/or x86).
	if (t_success &&
		(!MCStringIsEmpty(p_params . engine) && (!MCStringIsEmpty(p_params . engine_ppc) || !MCStringIsEmpty(p_params . engine_x86))))
		t_success = MCDeployThrow(kMCDeployErrorNoEngine);
	
	// MW-2013-06-13: Next check that we have at least one engine.
	if (t_success &&
		(MCStringIsEmpty(p_params . engine) && MCStringIsEmpty(p_params . engine_ppc) && MCStringIsEmpty(p_params . engine_x86)))
		t_success = MCDeployThrow(kMCDeployErrorNoEngine);
	
	// Now open the files.
	MCDeployFileRef t_engine, t_engine_ppc, t_engine_x86, t_output;
	t_engine = t_engine_ppc = t_engine_x86 = t_output = NULL;
    if (t_success &&
		((!MCStringIsEmpty(p_params . engine) && !MCDeployFileOpen(p_params . engine, kMCOpenFileModeRead, t_engine)) ||
		 (!MCStringIsEmpty(p_params . engine_ppc) && !MCDeployFileOpen(p_params . engine_ppc, kMCOpenFileModeRead, t_engine_ppc)) ||
		 (!MCStringIsEmpty(p_params . engine_x86) && !MCDeployFileOpen(p_params . engine_x86, kMCOpenFileModeRead, t_engine_x86))))
		t_success = MCDeployThrow(kMCDeployErrorNoEngine);
	
	if (t_success && !MCDeployFileOpen(p_params . output, kMCOpenFileModeCreate, t_output))
		t_success = MCDeployThrow(kMCDeployErrorNoOutput);

	// MW-2013-06-13:  If we have a single engine, process that in the appropriate
	//   architecture (or for all archs if fat). Otherwise, use the specific ppc or
	//   x86 engines.
	if (t_success && t_engine != NULL)
		t_success = MCDeployToMacOSXFat(p_params, false, t_engine, t_output, MCDeployValidateMacEngine);
	else if (t_success)
	{
		uint32_t t_offset;
		t_offset = 0;
		if (t_engine_ppc != NULL && t_engine_x86 != NULL)
		{
			// If we are producing a universal binary then first output the PPC
			// and x86 executable blocks.
			uint32_t t_ppc_offset, t_ppc_size;
			if (t_success)
			{
				t_offset = t_ppc_offset = 4096;
				t_success = MCDeployToMacOSXMain(p_params, true, t_engine_ppc, 0, 0, t_offset, t_output, MCDeployValidateMacEngine);
				t_ppc_size = t_offset - t_ppc_offset;
			}

			uint32_t t_x86_offset, t_x86_size;
			if (t_success)
			{
				t_x86_offset = t_offset = (t_offset + 4095) & ~4095;
				t_success = MCDeployToMacOSXMain(p_params, false, t_engine_x86, 0, 0, t_offset, t_output, MCDeployValidateMacEngine);
				t_x86_size = t_offset - t_x86_offset;
			}

			// Then overwrite the beginning with the fat header.
			if (t_success)
			{
				fat_header t_header;
				t_header . magic = FAT_MAGIC;
				t_header . nfat_arch = 2;

				fat_arch t_ppc_arch;
				t_ppc_arch . cputype = CPU_TYPE_POWERPC;
				
				// MM-2014-02-06: [[ GCC Update ]] Updating the version of GCC to 4.2 has meant that we now produce ppc7400 only executables (rather than ppc all).
				t_ppc_arch . cpusubtype = CPU_SUBTYPE_POWERPC_7400;
				
				t_ppc_arch . offset = t_ppc_offset;
				t_ppc_arch . size = t_ppc_size;
				t_ppc_arch . align = 12;

				fat_arch t_x86_arch;
				t_x86_arch . cputype = CPU_TYPE_X86;
				t_x86_arch . cpusubtype = CPU_SUBTYPE_X86_ALL;
				t_x86_arch . offset = t_x86_offset;
				t_x86_arch . size = t_x86_size;
				t_x86_arch . align = 12;

				swap_fat_header(true, t_header);
				swap_fat_arch(true, t_ppc_arch);
				swap_fat_arch(true, t_x86_arch);

				t_success =
					MCDeployFileWriteAt(t_output, &t_header, sizeof(fat_header), 0) &&
					MCDeployFileWriteAt(t_output, &t_ppc_arch, sizeof(fat_arch), sizeof(fat_header)) &&
					MCDeployFileWriteAt(t_output, &t_x86_arch, sizeof(fat_arch), sizeof(fat_header) + sizeof(fat_arch));
			}
		}
		else if (t_engine_ppc != NULL)
			t_success = MCDeployToMacOSXMain(p_params, true, t_engine_ppc, 0, 0, t_offset, t_output, MCDeployValidateMacEngine);
		else if (t_engine_x86 != NULL)
			t_success = MCDeployToMacOSXMain(p_params, false, t_engine_x86, 0, 0, t_offset, t_output, MCDeployValidateMacEngine);
	}

	MCDeployFileClose(t_output);
	MCDeployFileClose(t_engine);
	MCDeployFileClose(t_engine_x86);
	MCDeployFileClose(t_engine_ppc);
	
	// OK-2010-02-22: [[Bug 8624]] - Standalones not being made executable if they contain accented chars in path.
	MCStringRef t_path;
	t_path = p_params.output;
	
	// If on Mac OS X or Linux, make the file executable
#if defined(_MACOSX) || defined(_LINUX)
	if (t_success)
    {
        MCAutoStringRefAsUTF8String t_utf8_path;
        /* UNCHECKED */ t_utf8_path . Lock(t_path);
		chmod(*t_utf8_path, 0755);
    }
#endif

	return t_success ? ES_NORMAL : ES_ERROR;
}

// This method verifies that the given engine is for iOS. It checks the CPU is
// is either ARM or x86, and that the executable loads Cocoa.
static bool MCDeployValidateIOSEngine(const MCDeployParameters& p_params, mach_header& p_header, load_command **p_commands)
{
	// Check the CPU type is ARM or X86
	if ((p_header . cputype & ~CPU_ARCH_ABI64) != CPU_TYPE_ARM &&
		(p_header . cputype & ~CPU_ARCH_ABI64)  != CPU_TYPE_X86)
		return MCDeployThrow(kMCDeployErrorMacOSXBadCpuType);

	// Check that UIKit is one of the libraries linked to
	bool t_found_uikit;
	t_found_uikit = false;
	for(uint32_t i = 0; i < p_header . ncmds; i++)
		if (p_commands[i] -> cmd == LC_LOAD_DYLIB)
		{
			dylib_command *t_command;
			t_command = (dylib_command *)p_commands[i];

			const char *t_name;
			t_name = (const char *)t_command + t_command -> dylib . name . offset;
			if (MCCStringBeginsWith(t_name, "/System/Library/Frameworks/UIKit.framework"))
				t_found_uikit = true;
		}

	if (!t_found_uikit)
		return MCDeployThrow(kMCDeployErrorMacOSXBadTarget);

	return true;
}

// This method attempts to build an iOS standalone using the given deployment
// parameters.
//
Exec_stat MCDeployToIOS(const MCDeployParameters& p_params, bool p_embedded)
{
	bool t_success;
	t_success = true;
	
	// First thing we do is open the input engine.
	MCDeployFileRef t_engine, t_output;
	t_engine = t_output = NULL;
	if (t_success &&
		((MCStringIsEmpty(p_params . engine)) || !MCDeployFileOpen(p_params . engine, kMCOpenFileModeRead, t_engine)))
		t_success = MCDeployThrow(kMCDeployErrorNoEngine);
	
	// Make sure we can open the output file.
	if (t_success && !MCDeployFileOpen(p_params . output, kMCOpenFileModeCreate, t_output))
		t_success = MCDeployThrow(kMCDeployErrorNoOutput);
	
	// Generate the binary.
	if (t_success)
		t_success = MCDeployToMacOSXFat(p_params, p_embedded, t_engine, t_output, MCDeployValidateIOSEngine);

	MCDeployFileClose(t_output);
	MCDeployFileClose(t_engine);
	
	// OK-2010-02-22: [[Bug 8624]] - Standalones not being made executable if they contain accented chars in path.
	MCStringRef t_path;
	t_path = p_params.output;
	
	// If on Mac OS X or Linux, make the file executable
#if defined(_MACOSX) || defined(_LINUX)
	if (t_success)
    {
		MCAutoStringRefAsUTF8String t_utf8_path;
        /* UNCHECKED */ t_utf8_path . Lock(t_path);
		chmod(*t_utf8_path, 0755);
    }
#endif
	
	return t_success ? ES_NORMAL : ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCDeployMacOSXArchitectureCallback)(void *context, MCDeployFileRef p_exe, const fat_arch& p_arch);

// Mach-O fat executables consist are constructed thus:
//   fat_header
//     magic
//     nfat_arch
//   fat_arch*
//     cputype
//     cpusubtype
//     offset
//     size
//     align
//   mach_header*
//     magic
//     cputype
//     cpusubtype
//     ...
//  The 'mach_header' structures are always page-aligned.
//
static bool MCDeployForEachMacOSXArchitecture(MCDeployFileRef p_exe, MCDeployMacOSXArchitectureCallback p_callback, void *p_context)
{
	// First see if we have a fat header.
	fat_header t_fat_header;
	if (!MCDeployFileReadAt(p_exe, &t_fat_header, sizeof(fat_header), 0))
		return MCDeployThrow(kMCDeployErrorMacOSXNoHeader);

	// Swap the header - note that the 'fat_*' structures are always in network
	// byte-order.
	swap_fat_header(true, t_fat_header);

	if (t_fat_header . magic == FAT_MAGIC || t_fat_header . magic == FAT_CIGAM)
	{
		// This is fat binary, so iterate through each architecture calling the callback
		// each time.
		
		// The fat_arch structures follow the fat header directly
		uint32_t t_offset;
		t_offset = sizeof(fat_header);

		for(uint32_t i = 0; i < t_fat_header . nfat_arch; i++)
		{
			fat_arch t_fat_arch;
			if (!MCDeployFileReadAt(p_exe, &t_fat_arch, sizeof(fat_arch), t_offset))
				return MCDeployThrow(kMCDeployErrorMacOSXBadHeader);

            swap_fat_arch(t_fat_header . magic == FAT_MAGIC, t_fat_arch);

			if (!p_callback(p_context, p_exe, t_fat_arch))
				return false;

			t_offset += sizeof(fat_arch);
		}
	}
	else
	{
		// This is a single arch binary, so read the mach-header and construct a fat
		// arch structure to pass to the callback.
		mach_header t_mach_header;
		if (!MCDeployFileReadAt(p_exe, &t_mach_header, sizeof(mach_header), 0))
			return MCDeployThrow(kMCDeployErrorMacOSXNoHeader);

		// The mach-O structures are in the byte-order of the target architecture,
		// we can detect this by looking at the 'magic' field.
		if (t_mach_header . magic == MH_MAGIC)
			swap_mach_header(false, t_mach_header);
		else if (t_mach_header . magic == MH_CIGAM)
			swap_mach_header(true, t_mach_header);
		else if (t_mach_header . magic == MH_MAGIC_64)
        {
            // MW-2015-01-06: [[ ARM64 ]] No need to byteswap 64-bit (we don't support PPC64)
        }
        else
			return MCDeployThrow(kMCDeployErrorMacOSXBadHeader);

		// Fetch the size of the file
		uint32_t t_size;
		if (!MCDeployFileMeasure(p_exe, t_size))
			return MCDeployThrow(kMCDeployErrorBadFile);

		// We can now build the fat_arch structure
		fat_arch t_fat_arch;
		t_fat_arch . cputype = t_mach_header . cputype;
		t_fat_arch . cpusubtype = t_mach_header . cpusubtype;
		t_fat_arch . offset = 0;
		t_fat_arch . size = t_size;
		t_fat_arch . align = 12;
		if (!p_callback(p_context, p_exe, t_fat_arch))
			return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

struct MCDeployDietContext
{
	// The original input parameters to the diet command
	const MCDeployDietParameters *params;

	// The number of architectures that will be present in the output file
	uint32_t arch_count;

	// The current offset at which to write the next fat_arch structure
	uint32_t fat_arch_offset;

	// The current offset at which to write the next macho structure
	uint32_t macho_header_offset;

	// The file to write the output to
	MCDeployFileRef output;
};

static bool diet_include_arch(const MCDeployDietParameters *p_params, uint32_t p_cputype)
{
	switch(p_cputype)
	{
	case CPU_TYPE_X86:
		return p_params -> keep_x86;
	case CPU_TYPE_X86_64:
		return p_params -> keep_x86_64;
	case CPU_TYPE_ARM:
		return p_params -> keep_arm;
	case CPU_TYPE_POWERPC:
		return p_params -> keep_ppc;
	case CPU_TYPE_POWERPC64:
		return p_params -> keep_ppc64;
	default:
		break;
	}
	return false;
}

// This callback methods checks to see if the architecture presented to it 
// will be included in the output file, based on the provided 'params'.
static bool diet_count_archs(void *p_context, MCDeployFileRef p_exe, const fat_arch& p_header)
{
	MCDeployDietContext *context;
	context = (MCDeployDietContext *)p_context;

	if (diet_include_arch(context -> params, p_header . cputype))
		context -> arch_count += 1;

	return true;
}

// This method adds a new string to the given string table.
static bool diet_add_symbol_string(const char *p_string, char*& x_table, uint32_t& x_size, int32_t& r_offset)
{
	uint32_t t_length;
	t_length = MCCStringLength(p_string) + 1;

	int32_t t_offset;
	t_offset = 0;
	if (t_length > 1)
		while(t_offset + t_length <= x_size)
		{
			uint32_t t_next_length;
			t_next_length = MCCStringLength(x_table + t_offset) + 1;
			
			if (t_next_length < t_length)
			{
				t_offset += t_next_length;
				continue;
			}

			if (t_next_length > t_length)
				t_offset += t_next_length - t_length;

			if (MCMemoryEqual(p_string, x_table + t_offset, t_length))
			{
				r_offset = t_offset;
				return true;
			}

			t_offset += t_length;
		}

	if (!MCMemoryReallocate(x_table, x_size + t_length, x_table))
		return false;

	MCMemoryCopy(x_table + x_size, p_string, t_length);
	r_offset = x_size;
	x_size += t_length;

	return true;
}

// This method processes the given executable, stripping out symbols as specified
// in the 'params'.
static bool diet_strip_symbols(MCDeployDietContext& context, bool p_big_endian, MCDeployFileRef p_input, uint32_t p_input_offset, uint32_t p_input_size, uint32_t& r_output_size)
{
	bool t_success;
	t_success = true;

	// First read the Mach-O header.
	mach_header t_header;
	load_command **t_commands;
	t_commands = NULL;
	if (t_success)
		t_success = MCDeployToMacOSXReadHeader(p_big_endian, p_input, p_input_offset, t_header, t_commands);

	uint32_t t_command_count;
	if (t_success)
		t_command_count = t_header . ncmds;

	// Find the link-edit segment and symtab commands
	segment_command *t_linkedit_segment;
	symtab_command *t_symtab_command;
	dysymtab_command *t_dysymtab_command;
	t_linkedit_segment = nil;
	t_symtab_command = nil;
	t_dysymtab_command = nil;
	if (t_success)
    {
		for(uint32_t i = 0; i < t_header . ncmds; i++)
        {
			if (t_commands[i] -> cmd == LC_SEGMENT)
			{
				segment_command *t_command;
				t_command = (segment_command *)t_commands[i];
				if (memcmp(t_command -> segname, "__LINKEDIT", 11) == 0)
					t_linkedit_segment = t_command;
			}
			else if (t_commands[i] -> cmd == LC_SYMTAB)
				t_symtab_command = (symtab_command *)t_commands[i];
			else if (t_commands[i] -> cmd == LC_DYSYMTAB)
				t_dysymtab_command = (dysymtab_command *)t_commands[i];
        }
    }

	// The 'linkedit' segment contains all the data used by the symtab
	// and dysymtab commands. We need to rebuild this segment in the following
	// way:
	//   dysymtab locrel
	//   symbol table
	//   dysymtab extrel
	//   dysymtab indirect
	//   string table
	//   [ dysymtab toc -- NOT SUPPORTED YET ]
	//   [ dysymtab modtab -- NOT SUPPORTED YET ]
	//   [ dysymtab extref -- NOT SUPPORTED YET ]
	// The dysymtab tables are copied verbatim (if present) with offsets in the
	// relevant command updated appropriately.
	
	// If the exe contains a toc, modtab or extref table, we don't currently
	// know how to strip it.
	if (t_success &&
		(t_dysymtab_command -> ntoc != 0 ||
			t_dysymtab_command -> nmodtab != 0 ||
			t_dysymtab_command -> nextrefsyms != 0))
		t_success = MCDeployThrow(kMCDeployErrorCannotDiet);

	// So next we load the symbol table - this is a list of 'nlist' structures. The
	// table starts at the offset symoff (relative to input offset).
	nlist *t_symbols;
	t_symbols = nil;
	if (t_success)
		t_success = MCMemoryNewArray(t_symtab_command -> nsyms, t_symbols);
	if (t_success)
		t_success = MCDeployFileReadAt(p_input, t_symbols, sizeof(nlist) * t_symtab_command -> nsyms, p_input_offset + t_symtab_command -> symoff);
	if (t_success)
		for(uint32_t i = 0; i < t_symtab_command -> nsyms; i++)
			swap_nlist(p_big_endian, t_symbols[i]);

	// Now load the string table.
	char *t_strings;
	t_strings = nil;
	if (t_success)
		t_success =
			MCMemoryNewArray(t_symtab_command -> strsize, t_strings) &&
			MCDeployFileReadAt(p_input, t_strings, t_symtab_command -> strsize, p_input_offset + t_symtab_command -> stroff);
	
	// The symbol map, is used to map old symbol indices to new ones.
	uint32_t *t_symbol_map;
	t_symbol_map = nil;
	if (t_success)
		t_success = MCMemoryNewArray(t_symtab_command -> nsyms, t_symbol_map);
	
	// Now loop through the symbol table, removing any N_STAB entries. This is
	// equivalent to doing 'strip -S'.
	char *t_new_strings;
	uint32_t t_new_symbol_count, t_new_strings_size;
	t_new_symbol_count = 0;
	t_new_strings_size = 0;
	t_new_strings = nil;
	if (t_success)
	{
		int32_t t_offset;
		for(uint32_t i = 0; i < 4 && t_success; i++)
			t_success = diet_add_symbol_string("", t_new_strings, t_new_strings_size, t_offset);
		
		for(uint32_t i = 0; i < t_symtab_command -> nsyms && t_success; i++)
			if ((t_symbols[i] . n_type & N_STAB) == 0)
			{
				nlist t_sym;
				t_sym = t_symbols[i];

				t_symbols[t_new_symbol_count] = t_sym;
				t_success = diet_add_symbol_string(t_strings + t_sym . n_strx, t_new_strings, t_new_strings_size, t_symbols[t_new_symbol_count] . n_strx);

				t_symbol_map[i] = t_new_symbol_count++;
			}
		
		if (t_success)
			t_success = diet_add_symbol_string("", t_new_strings, t_new_strings_size, t_offset);
	}

	// We now need to rebuild the output file:
	//   1) Write out everything up to the start of the LINKEDIT segment
	//   2) Write out the locrel table
	//   2) Write out the reduced symbol table
	//   3) Write out the extrel and indirect tables
	//   4) Write out the new strings table
	//   5) Update all the load commands as necessary
	uint32_t t_offset;
	t_offset = context . macho_header_offset;

	// Everything before the LINK-EDIT segment
	if (t_success)
	{
		t_success = MCDeployFileCopy(context . output, t_offset, p_input, p_input_offset, t_linkedit_segment -> fileoff);
		t_offset += t_linkedit_segment -> fileoff;
	}

	// The locrel table
	if (t_success)
	{
		if (t_dysymtab_command -> nlocrel != 0)
			t_success = MCDeployFileCopy(context . output, t_offset, p_input, p_input_offset + t_dysymtab_command -> locreloff, t_dysymtab_command -> nlocrel * sizeof(relocation_info));

		t_dysymtab_command -> locreloff = t_dysymtab_command -> nlocrel != 0 ? t_offset : 0;
		t_offset += t_dysymtab_command -> nlocrel * sizeof(relocation_info);
	}

	// The new symbol table
	if (t_success)
	{
		// Convert the nlist array to target byte-order
		for(uint32_t i = 0; i < t_new_symbol_count; i++)
			swap_nlist(p_big_endian, t_symbols[i]);

		// Append the symtab
		t_success = MCDeployFileWriteAt(context . output, t_symbols, sizeof(nlist) * t_new_symbol_count, t_offset);

		t_symtab_command -> symoff = t_offset;
		t_offset += sizeof(nlist) * t_new_symbol_count;
	}

	// The extrel table
	if (t_success)
	{
		uint32_t t_start_offset;
		t_start_offset = t_offset;

		for(uint32_t i = 0; i < t_dysymtab_command -> nextrel && t_success; i++)
		{
			relocation_info t_info;
			t_success = MCDeployFileReadAt(p_input, &t_info, sizeof(relocation_info), p_input_offset + t_dysymtab_command -> extreloff + sizeof(relocation_info) * i);
			if (t_success)
			{
				swap_relocation_info(p_big_endian, t_info);
				if (t_info . r_extern)
					t_info . r_symbolnum = t_symbol_map[t_info . r_symbolnum];
				swap_relocation_info(p_big_endian, t_info);
				t_success = MCDeployFileWriteAt(context . output, &t_info, sizeof(relocation_info), t_offset);
				t_offset += sizeof(relocation_info);
			}
		}

		t_dysymtab_command -> extreloff = t_dysymtab_command -> nextrel != 0 ? t_start_offset : 0;
	}

	// The indirect symbol table
	if (t_success)
	{
		uint32_t t_start_offset;
		t_start_offset = t_offset;

		for(uint32_t i = 0; i < t_dysymtab_command -> nindirectsyms && t_success; i++)
		{
			uint32_t t_indsym;
			t_success = MCDeployFileReadAt(p_input, &t_indsym, sizeof(uint32_t), p_input_offset + t_dysymtab_command -> indirectsymoff + sizeof(uint32_t) * i);
			if (t_success)
			{
				swap_uint32(p_big_endian, t_indsym);
				if (t_indsym != INDIRECT_SYMBOL_LOCAL && t_indsym != (INDIRECT_SYMBOL_LOCAL | INDIRECT_SYMBOL_ABS))
					t_indsym = t_symbol_map[t_indsym];
				swap_uint32(p_big_endian, t_indsym);
				t_success = MCDeployFileWriteAt(context . output, &t_indsym, sizeof(uint32_t), t_offset);
				t_offset += sizeof(uint32_t);
			}
		}

		t_dysymtab_command -> indirectsymoff = t_start_offset;
	}

	// The new strings table
	if (t_success)
	{
		t_success = MCDeployFileWriteAt(context . output, t_new_strings, t_new_strings_size, t_offset);

		t_symtab_command -> stroff = t_offset;
		t_offset += t_new_strings_size;
	}

	// Now write out the updated segment commands
	if (t_success)
	{
		t_symtab_command -> nsyms = t_new_symbol_count;
		t_symtab_command -> strsize = t_new_strings_size;

		t_dysymtab_command -> ilocalsym = t_symbol_map[t_dysymtab_command -> ilocalsym];
		t_dysymtab_command -> iextdefsym = t_symbol_map[t_dysymtab_command -> iextdefsym];
		t_dysymtab_command -> iundefsym = t_symbol_map[t_dysymtab_command -> iundefsym];

		t_dysymtab_command -> nlocalsym = t_dysymtab_command -> iextdefsym - t_dysymtab_command -> ilocalsym;
		t_dysymtab_command -> nextdefsym = t_dysymtab_command -> iundefsym - t_dysymtab_command -> iextdefsym;
		t_dysymtab_command -> nundefsym = t_new_symbol_count - t_dysymtab_command -> iundefsym;

		t_linkedit_segment -> filesize = t_offset - t_linkedit_segment -> fileoff;
		t_linkedit_segment -> vmsize = t_linkedit_segment -> filesize;

		uint32_t t_hdr_offset;
		t_hdr_offset = context . macho_header_offset + sizeof(mach_header);
		for(uint32_t i = 0; i < t_command_count && t_success; i++)
		{
			uint32_t t_size;
			t_size = t_commands[i] -> cmdsize;
			swap_load_command(p_big_endian, t_commands[i] -> cmd, t_commands[i]);
			t_success = MCDeployFileWriteAt(context . output, t_commands[i], t_size, t_hdr_offset);
			if (t_success)
				t_hdr_offset += t_size;
		}
	}

	if (t_success)
		r_output_size = t_offset;

	// Free the temporary data structures we needed.
	MCMemoryDeallocate(t_new_strings);
	MCMemoryDeleteArray(t_symbol_map);
	MCMemoryDeleteArray(t_strings);
	MCMemoryDeleteArray(t_symbols);
	if (t_commands != NULL)
	{
		for(uint32_t i = 0; i < t_command_count; i++)
			free(t_commands[i]);
		delete[] t_commands;
	}

	return t_success;
}

// This callback processes the given arch and includes it in the output file
// but only if it should be included, based on the 'params'.
static bool diet_process_archs(void *p_context, MCDeployFileRef p_exe, const fat_arch& p_header)
{
	MCDeployDietContext *context;
	context = (MCDeployDietContext *)p_context;

	// Ignore any architectures that we don't want.
	if (!diet_include_arch(context -> params, p_header . cputype))
		return true;

	// Now process the executable for the given arch.
	uint32_t t_size;
	if (!diet_strip_symbols(*context, p_header . cputype == CPU_TYPE_POWERPC || p_header . cputype == CPU_TYPE_POWERPC64, p_exe, p_header . offset, p_header . size, t_size))
		return false;

	// If there is a fat header, then write out its arch structure
	if (context -> arch_count > 1)
	{
		fat_arch t_arch;
		t_arch . cputype = p_header . cputype;
		t_arch . cpusubtype = p_header . cpusubtype;
		t_arch . offset = context -> macho_header_offset;
		t_arch . size = t_size;
		t_arch . align = 12;
		swap_fat_arch(true, t_arch);
		if (!MCDeployFileWriteAt(context -> output, &t_arch, sizeof(fat_arch), context -> fat_arch_offset))
			return false;

		context -> fat_arch_offset += sizeof(fat_arch);
	}

	// The next macho exe goes after this one, after rounding up to the nearest
	// page.
	context -> macho_header_offset += (t_size + 4095) & ~4095;

	return true;
}

// This method strips a Mac OS X executable, optionally thinning it at the same
// time.
Exec_stat MCDeployDietMacOSX(const MCDeployDietParameters& p_params)
{
	bool t_success;
	t_success = true;

	// First thing we do is open the files.
	MCDeployFileRef t_engine, t_output;
	t_engine = t_output = NULL;
	if (t_success && !MCDeployFileOpen(p_params . input, kMCOpenFileModeRead, t_engine))
		t_success = MCDeployThrow(kMCDeployErrorNoEngine);
	
	if (t_success && !MCDeployFileOpen(p_params . output, kMCOpenFileModeWrite, t_output))
		t_success = MCDeployThrow(kMCDeployErrorNoOutput);

	// Next we count the number of architectures that we will be including in
	// the output file - this determines if need a fat header or not.
	MCDeployDietContext t_context;
	if (t_success)
	{
		t_context . params = &p_params;
		t_context . arch_count = 0;
		t_context . fat_arch_offset = 0;
		t_context . macho_header_offset = 0;
		t_context . output = nil;
		t_success = MCDeployForEachMacOSXArchitecture(t_engine, diet_count_archs, &t_context);
	}

	// If we got zero output architectures its an error
	if (t_success && t_context . arch_count == 0)
		t_success = MCDeployThrow(kMCDeployErrorNoArchs);

	// Next, if we have more than one output arch, write out a fat header and
	// set the offsets appropriately.
	if (t_success && t_context . arch_count > 1)
	{
		if (t_success)
		{
			fat_header t_fat_header;
			t_fat_header . magic = FAT_MAGIC;
			t_fat_header . nfat_arch = t_context . arch_count;
			swap_fat_header(true, t_fat_header);
			t_success = MCDeployFileWriteAt(t_output, &t_fat_header, sizeof(fat_header), 0);
		}
		
		if (t_success)
		{
			// First fat_arch structure occurs after the fat header
			t_context . fat_arch_offset = sizeof(fat_header);
			// First macho_header starts after the fat structures, rounded up to the nearest page
			t_context . macho_header_offset = (sizeof(fat_header) + t_context . arch_count * sizeof(fat_arch) + 4095) & ~4095;
		}
	}
	
	// Now iterate through the input binary again, this time processing the
	// executables for each arch as we go.
	if (t_success)
	{
		t_context . output = t_output;
		t_success = MCDeployForEachMacOSXArchitecture(t_engine, diet_process_archs, &t_context);
	}

	MCDeployFileClose(t_engine);
	MCDeployFileClose(t_output);

	return t_success ? ES_NORMAL : ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

struct MCDeployExtractContext
{
	const char *segment;
	const char *section;
	uint32_t offset;
	uint32_t size;
	bool error;
};

template<typename T> static bool MCDeployExtractArchCallbackBody(MCDeployExtractContext *context, MCDeployFileRef, const fat_arch& p_header, mach_header& t_header, load_command **t_commands, uint32_t t_command_count)
{
	typename T::section *t_section;
	t_section = nil;
	for(uint32_t i = 0; i < t_header . ncmds; i++)
	{
        if (t_commands[i] -> cmd != T::seg_load_command)
			continue;
		
		typename T::segment_command *t_segment;
		t_segment = (typename T::segment_command *)t_commands[i];
		
		for(uint32_t j = 0; j < t_segment -> nsects; j++)
		{
			typename T::section *t_next_section;
			t_next_section = &((typename T::section *)(t_segment + 1))[j];
			if (memcmp(t_next_section -> segname, context -> segment, MCMin(16U, MCCStringLength(context -> segment))) != 0)
				continue;
			
			if (memcmp(t_next_section -> sectname, context -> section, MCMin(16U, MCCStringLength(context -> section))) != 0)
				continue;
			
			t_section = t_next_section;
			break;
		}
		
		if (t_section != nil)
			break;
	}
	
	if (t_section != nil)
	{
		context -> offset = p_header . offset + t_section -> offset;
		context -> size = t_section -> size;
	}
    
	if (t_commands != NULL)
	{
		for(uint32_t i = 0; i < t_command_count; i++)
			free(t_commands[i]);
		delete[] t_commands;
	}
	
	return context -> offset == 0;
}

static bool MCDeployExtractArchCallback(void *p_context, MCDeployFileRef p_exe, const fat_arch& p_header)
{
	MCDeployExtractContext *context;
	context = (MCDeployExtractContext *)p_context;
	
	// First read the Mach-O header.
	mach_header t_header;
	load_command **t_commands;
	t_commands = NULL;
	if (!MCDeployToMacOSXReadHeader(false, p_exe, p_header . offset, t_header, t_commands))
	{
		context -> error = true;
		return false;
	}
	
	uint32_t t_command_count;
	t_command_count = t_header . ncmds;
	
    if ((t_header . cputype & CPU_ARCH_ABI64) == 0)
        return MCDeployExtractArchCallbackBody<mach_32bit>(context, p_exe, p_header, t_header, t_commands, t_command_count);
    
    return MCDeployExtractArchCallbackBody<mach_64bit>(context, p_exe, p_header, t_header, t_commands, t_command_count);
}

// This method extracts a segment from a Mach-O file - if the file is a fat
// binary, it uses the first image in the file.
Exec_stat MCDeployExtractMacOSX(MCStringRef p_filename, MCStringRef p_segment, MCStringRef p_section, void*& r_data, uint32_t& r_data_size)
{
	bool t_success;
	t_success = true;
	
	// First thing we do is open the input file.
	MCDeployFileRef t_input;
	t_input = NULL;
	if (t_success && !MCDeployFileOpen(p_filename, kMCOpenFileModeRead, t_input))
		t_success = MCDeployThrow(kMCDeployErrorNoEngine);
	
	// Next just run the callback to get the offset of the section within the
	// file.
	MCDeployExtractContext t_context;
    MCAutoStringRefAsCString t_section;
    MCAutoStringRefAsCString t_segment;
    
	if (t_success)
        t_success = t_section . Lock(p_section) && t_segment . Lock(p_segment);
    
    if (t_success)
    {
        t_context . section = (const char*) *t_section;
		t_context . segment = (const char*) *t_segment;
		t_context . offset = 0;
		t_context . size = 0;
		t_context . error = false;
		if (!MCDeployForEachMacOSXArchitecture(t_input, MCDeployExtractArchCallback, &t_context))
			t_success = t_context . offset != 0 && !t_context . error;
	}
	
	void *t_data;
	t_data = nil;
	if (t_success)
		t_success = MCMemoryAllocate(t_context . size, t_data);
	
	if (t_success)
		t_success = MCDeployFileReadAt(t_input, t_data, t_context . size, t_context . offset);
	
	if (t_success)
	{
		r_data = t_data;
		r_data_size = t_context . size;
	}
	else
		MCMemoryDeallocate(t_data);

	return t_success ? ES_NORMAL : ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
