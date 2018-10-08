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

#include "deploy.h"

#if defined(_LINUX) || defined(_MACOSX)
#include <sys/stat.h>
#endif

////////////////////////////////////////////////////////////////////////////////
//
// This section contains definitions for the various structures needed to
// process the ELF executable format. These are taken from /usr/include/elf.h
// on linux. In some cases the constant lists have been reduced since most are
// not relevant to what this module needs.
//

/* Type for a 16-bit quantity.  */
typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32_t Elf32_Word;
typedef	int32_t  Elf32_Sword;
typedef uint32_t Elf64_Word;
typedef	int32_t  Elf64_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64_t Elf32_Xword;
typedef	int64_t  Elf32_Sxword;
typedef uint64_t Elf64_Xword;
typedef	int64_t  Elf64_Sxword;

/* Type of addresses.  */
typedef uint32_t Elf32_Addr;
typedef uint64_t Elf64_Addr;

/* Type of file offsets.  */
typedef uint32_t Elf32_Off;
typedef uint64_t Elf64_Off;

/* Type for section indices, which are 16-bit quantities.  */
typedef uint16_t Elf32_Section;
typedef uint16_t Elf64_Section;

/* Type for version symbol information.  */
typedef Elf32_Half Elf32_Versym;
typedef Elf64_Half Elf64_Versym;

/* The ELF file header.  This appears at the start of every ELF file.  */

#define EI_NIDENT (16)

typedef struct
{
  unsigned char	e_ident[EI_NIDENT];	/* Magic number and other info */
  Elf32_Half	e_type;			/* Object file type */
  Elf32_Half	e_machine;		/* Architecture */
  Elf32_Word	e_version;		/* Object file version */
  Elf32_Addr	e_entry;		/* Entry point virtual address */
  Elf32_Off	e_phoff;		/* Program header table file offset */
  Elf32_Off	e_shoff;		/* Section header table file offset */
  Elf32_Word	e_flags;		/* Processor-specific flags */
  Elf32_Half	e_ehsize;		/* ELF header size in bytes */
  Elf32_Half	e_phentsize;		/* Program header table entry size */
  Elf32_Half	e_phnum;		/* Program header table entry count */
  Elf32_Half	e_shentsize;		/* Section header table entry size */
  Elf32_Half	e_shnum;		/* Section header table entry count */
  Elf32_Half	e_shstrndx;		/* Section header string table index */
} Elf32_Ehdr;

typedef struct
{
  unsigned char	e_ident[EI_NIDENT];	/* Magic number and other info */
  Elf64_Half	e_type;			/* Object file type */
  Elf64_Half	e_machine;		/* Architecture */
  Elf64_Word	e_version;		/* Object file version */
  Elf64_Addr	e_entry;		/* Entry point virtual address */
  Elf64_Off	e_phoff;		/* Program header table file offset */
  Elf64_Off	e_shoff;		/* Section header table file offset */
  Elf64_Word	e_flags;		/* Processor-specific flags */
  Elf64_Half	e_ehsize;		/* ELF header size in bytes */
  Elf64_Half	e_phentsize;		/* Program header table entry size */
  Elf64_Half	e_phnum;		/* Program header table entry count */
  Elf64_Half	e_shentsize;		/* Section header table entry size */
  Elf64_Half	e_shnum;		/* Section header table entry count */
  Elf64_Half	e_shstrndx;		/* Section header string table index */
} Elf64_Ehdr;

/* Fields in the e_ident array.  The EI_* macros are indices into the
   array.  The macros under each EI_* macro are the values the byte
   may have.  */

#define EI_MAG0		0		/* File identification byte 0 index */
#define ELFMAG0		0x7f		/* Magic number byte 0 */

#define EI_MAG1		1		/* File identification byte 1 index */
#define ELFMAG1		'E'		/* Magic number byte 1 */

#define EI_MAG2		2		/* File identification byte 2 index */
#define ELFMAG2		'L'		/* Magic number byte 2 */

#define EI_MAG3		3		/* File identification byte 3 index */
#define ELFMAG3		'F'		/* Magic number byte 3 */

/* Conglomeration of the identification bytes, for easy testing as a word.  */
#define	ELFMAG		"\177ELF"
#define	SELFMAG		4

#define EI_CLASS	4		/* File class byte index */
#define ELFCLASSNONE	0		/* Invalid class */
#define ELFCLASS32	1		/* 32-bit objects */
#define ELFCLASS64	2		/* 64-bit objects */
#define ELFCLASSNUM	3

#define EI_DATA		5		/* Data encoding byte index */
#define ELFDATANONE	0		/* Invalid data encoding */
#define ELFDATA2LSB	1		/* 2's complement, little endian */
#define ELFDATA2MSB	2		/* 2's complement, big endian */
#define ELFDATANUM	3

#define EI_VERSION	6		/* File version byte index */
					/* Value must be EV_CURRENT */

#define EI_OSABI	7		/* OS ABI identification */
#define ELFOSABI_NONE		0	/* UNIX System V ABI */
#define ELFOSABI_SYSV		0	/* Alias.  */
#define ELFOSABI_HPUX		1	/* HP-UX */
#define ELFOSABI_NETBSD		2	/* NetBSD.  */
#define ELFOSABI_LINUX		3	/* Linux.  */
#define ELFOSABI_SOLARIS	6	/* Sun Solaris.  */
#define ELFOSABI_AIX		7	/* IBM AIX.  */
#define ELFOSABI_IRIX		8	/* SGI Irix.  */
#define ELFOSABI_FREEBSD	9	/* FreeBSD.  */
#define ELFOSABI_TRU64		10	/* Compaq TRU64 UNIX.  */
#define ELFOSABI_MODESTO	11	/* Novell Modesto.  */
#define ELFOSABI_OPENBSD	12	/* OpenBSD.  */
#define ELFOSABI_ARM		97	/* ARM */
#define ELFOSABI_STANDALONE	255	/* Standalone (embedded) application */

#define EI_ABIVERSION	8		/* ABI version */

#define EI_PAD		9		/* Byte index of padding bytes */

/* Legal values for e_type (object file type).  */

#define ET_NONE		0		/* No file type */
#define ET_REL		1		/* Relocatable file */
#define ET_EXEC		2		/* Executable file */
#define ET_DYN		3		/* Shared object file */
#define ET_CORE		4		/* Core file */
#define	ET_NUM		5		/* Number of defined types */
#define ET_LOOS		0xfe00		/* OS-specific range start */
#define ET_HIOS		0xfeff		/* OS-specific range end */
#define ET_LOPROC	0xff00		/* Processor-specific range start */
#define ET_HIPROC	0xffff		/* Processor-specific range end */

/* Legal values for e_machine (architecture).  */

/*** TRUNCATED ***/

#define EM_NONE		 0		/* No machine */
#define EM_M32		 1		/* AT&T WE 32100 */
#define EM_SPARC	 2		/* SUN SPARC */
#define EM_386		 3		/* Intel 80386 */
#define EM_68K		 4		/* Motorola m68k family */
#define EM_88K		 5		/* Motorola m88k family */
#define EM_860		 7		/* Intel 80860 */
#define EM_MIPS		 8		/* MIPS R3000 big-endian */
#define EM_S370		 9		/* IBM System/370 */
#define EM_MIPS_RS3_LE	10	/* MIPS R3000 little-endian */
#define EM_ARM		40      /* ARM */
#define EM_X86_64   62      /* AMD x86-64 */
#define EM_AARCH64  183     /* ARM AArch64 */

/* Legal values for e_version (version).  */

#define EV_NONE		0		/* Invalid ELF version */
#define EV_CURRENT	1		/* Current version */
#define EV_NUM		2
/* Section header.  */

typedef struct
{
  Elf32_Word	sh_name;		/* Section name (string tbl index) */
  Elf32_Word	sh_type;		/* Section type */
  Elf32_Word	sh_flags;		/* Section flags */
  Elf32_Addr	sh_addr;		/* Section virtual addr at execution */
  Elf32_Off	sh_offset;		/* Section file offset */
  Elf32_Word	sh_size;		/* Section size in bytes */
  Elf32_Word	sh_link;		/* Link to another section */
  Elf32_Word	sh_info;		/* Additional section information */
  Elf32_Word	sh_addralign;		/* Section alignment */
  Elf32_Word	sh_entsize;		/* Entry size if section holds table */
} Elf32_Shdr;

typedef struct
{
  Elf64_Word	sh_name;		/* Section name (string tbl index) */
  Elf64_Word	sh_type;		/* Section type */
  Elf64_Xword	sh_flags;		/* Section flags */
  Elf64_Addr	sh_addr;		/* Section virtual addr at execution */
  Elf64_Off	sh_offset;		/* Section file offset */
  Elf64_Xword	sh_size;		/* Section size in bytes */
  Elf64_Word	sh_link;		/* Link to another section */
  Elf64_Word	sh_info;		/* Additional section information */
  Elf64_Xword	sh_addralign;		/* Section alignment */
  Elf64_Xword	sh_entsize;		/* Entry size if section holds table */
} Elf64_Shdr;

/* Special section indices.  */

#define SHN_UNDEF	0		/* Undefined section */
#define SHN_LORESERVE	0xff00		/* Start of reserved indices */
#define SHN_LOPROC	0xff00		/* Start of processor-specific */
#define SHN_BEFORE	0xff00		/* Order section before all others
					   (Solaris).  */
#define SHN_AFTER	0xff01		/* Order section after all others
					   (Solaris).  */
#define SHN_HIPROC	0xff1f		/* End of processor-specific */
#define SHN_LOOS	0xff20		/* Start of OS-specific */
#define SHN_HIOS	0xff3f		/* End of OS-specific */
#define SHN_ABS		0xfff1		/* Associated symbol is absolute */
#define SHN_COMMON	0xfff2		/* Associated symbol is common */
#define SHN_XINDEX	0xffff		/* Index is in extra table.  */
#define SHN_HIRESERVE	0xffff		/* End of reserved indices */

/* Legal values for sh_type (section type).  */

#define SHT_NULL	  0		/* Section header table entry unused */
#define SHT_PROGBITS	  1		/* Program data */
#define SHT_SYMTAB	  2		/* Symbol table */
#define SHT_STRTAB	  3		/* String table */
#define SHT_RELA	  4		/* Relocation entries with addends */
#define SHT_HASH	  5		/* Symbol hash table */
#define SHT_DYNAMIC	  6		/* Dynamic linking information */
#define SHT_NOTE	  7		/* Notes */
#define SHT_NOBITS	  8		/* Program space with no data (bss) */
#define SHT_REL		  9		/* Relocation entries, no addends */
#define SHT_SHLIB	  10		/* Reserved */
#define SHT_DYNSYM	  11		/* Dynamic linker symbol table */
#define SHT_INIT_ARRAY	  14		/* Array of constructors */
#define SHT_FINI_ARRAY	  15		/* Array of destructors */
#define SHT_PREINIT_ARRAY 16		/* Array of pre-constructors */
#define SHT_GROUP	  17		/* Section group */
#define SHT_SYMTAB_SHNDX  18		/* Extended section indeces */
#define	SHT_NUM		  19		/* Number of defined types.  */
#define SHT_LOOS	  0x60000000	/* Start OS-specific.  */
#define SHT_GNU_HASH	  0x6ffffff6	/* GNU-style hash table.  */
#define SHT_GNU_LIBLIST	  0x6ffffff7	/* Prelink library list */
#define SHT_CHECKSUM	  0x6ffffff8	/* Checksum for DSO content.  */
#define SHT_LOSUNW	  0x6ffffffa	/* Sun-specific low bound.  */
#define SHT_SUNW_move	  0x6ffffffa
#define SHT_SUNW_COMDAT   0x6ffffffb
#define SHT_SUNW_syminfo  0x6ffffffc
#define SHT_GNU_verdef	  0x6ffffffd	/* Version definition section.  */
#define SHT_GNU_verneed	  0x6ffffffe	/* Version needs section.  */
#define SHT_GNU_versym	  0x6fffffff	/* Version symbol table.  */
#define SHT_HISUNW	  0x6fffffff	/* Sun-specific high bound.  */
#define SHT_HIOS	  0x6fffffff	/* End OS-specific type */
#define SHT_LOPROC	  0x70000000	/* Start of processor-specific */
#define SHT_HIPROC	  0x7fffffff	/* End of processor-specific */
#define SHT_LOUSER	  0x80000000	/* Start of application-specific */
#define SHT_HIUSER	  0x8fffffff	/* End of application-specific */

/* Legal values for sh_flags (section flags).  */

#define SHF_WRITE	     (1 << 0)	/* Writable */
#define SHF_ALLOC	     (1 << 1)	/* Occupies memory during execution */
#define SHF_EXECINSTR	     (1 << 2)	/* Executable */
#define SHF_MERGE	     (1 << 4)	/* Might be merged */
#define SHF_STRINGS	     (1 << 5)	/* Contains nul-terminated strings */
#define SHF_INFO_LINK	     (1 << 6)	/* `sh_info' contains SHT index */
#define SHF_LINK_ORDER	     (1 << 7)	/* Preserve order after combining */
#define SHF_OS_NONCONFORMING (1 << 8)	/* Non-standard OS specific handling
					   required */
#define SHF_GROUP	     (1 << 9)	/* Section is member of a group.  */
#define SHF_TLS		     (1 << 10)	/* Section hold thread-local data.  */
#define SHF_MASKOS	     0x0ff00000	/* OS-specific.  */
#define SHF_MASKPROC	     0xf0000000	/* Processor-specific */
#define SHF_ORDERED	     (1 << 30)	/* Special ordering requirement
					   (Solaris).  */
#define SHF_EXCLUDE	     (1U << 31)	/* Section is excluded unless
					   referenced or allocated (Solaris).*/

/* Program segment header.  */

typedef struct
{
  Elf32_Word	p_type;			/* Segment type */
  Elf32_Off	p_offset;		/* Segment file offset */
  Elf32_Addr	p_vaddr;		/* Segment virtual address */
  Elf32_Addr	p_paddr;		/* Segment physical address */
  Elf32_Word	p_filesz;		/* Segment size in file */
  Elf32_Word	p_memsz;		/* Segment size in memory */
  Elf32_Word	p_flags;		/* Segment flags */
  Elf32_Word	p_align;		/* Segment alignment */
} Elf32_Phdr;

typedef struct
{
  Elf64_Word	p_type;			/* Segment type */
  Elf64_Word	p_flags;		/* Segment flags */
  Elf64_Off	p_offset;		/* Segment file offset */
  Elf64_Addr	p_vaddr;		/* Segment virtual address */
  Elf64_Addr	p_paddr;		/* Segment physical address */
  Elf64_Xword	p_filesz;		/* Segment size in file */
  Elf64_Xword	p_memsz;		/* Segment size in memory */
  Elf64_Xword	p_align;		/* Segment alignment */
} Elf64_Phdr;

/* Legal values for p_type (segment type).  */

#define	PT_NULL		0		/* Program header table entry unused */
#define PT_LOAD		1		/* Loadable program segment */
#define PT_DYNAMIC	2		/* Dynamic linking information */
#define PT_INTERP	3		/* Program interpreter */
#define PT_NOTE		4		/* Auxiliary information */
#define PT_SHLIB	5		/* Reserved */
#define PT_PHDR		6		/* Entry for header table itself */
#define PT_TLS		7		/* Thread-local storage segment */
#define	PT_NUM		8		/* Number of defined types */
#define PT_LOOS		0x60000000	/* Start of OS-specific */
#define PT_GNU_EH_FRAME	0x6474e550	/* GCC .eh_frame_hdr segment */
#define PT_GNU_STACK	0x6474e551	/* Indicates stack executability */
#define PT_GNU_RELRO	0x6474e552	/* Read-only after relocation */
#define PT_LOSUNW	0x6ffffffa
#define PT_SUNWBSS	0x6ffffffa	/* Sun Specific segment */
#define PT_SUNWSTACK	0x6ffffffb	/* Stack segment */
#define PT_HISUNW	0x6fffffff
#define PT_HIOS		0x6fffffff	/* End of OS-specific */
#define PT_LOPROC	0x70000000	/* Start of processor-specific */
#define PT_HIPROC	0x7fffffff	/* End of processor-specific */

/* Legal values for p_flags (segment flags).  */

#define PF_X		(1 << 0)	/* Segment is executable */
#define PF_W		(1 << 1)	/* Segment is writable */
#define PF_R		(1 << 2)	/* Segment is readable */
#define PF_MASKOS	0x0ff00000	/* OS-specific */
#define PF_MASKPROC	0xf0000000	/* Processor-specific */

/* Legal values for note segment descriptor types for core files. */

#define NT_PRSTATUS	1		/* Contains copy of prstatus struct */
#define NT_FPREGSET	2		/* Contains copy of fpregset struct */
#define NT_PRPSINFO	3		/* Contains copy of prpsinfo struct */
#define NT_PRXREG	4		/* Contains copy of prxregset struct */
#define NT_TASKSTRUCT	4		/* Contains copy of task structure */
#define NT_PLATFORM	5		/* String from sysinfo(SI_PLATFORM) */
#define NT_AUXV		6		/* Contains copy of auxv array */
#define NT_GWINDOWS	7		/* Contains copy of gwindows struct */
#define NT_ASRS		8		/* Contains copy of asrset struct */
#define NT_PSTATUS	10		/* Contains copy of pstatus struct */
#define NT_PSINFO	13		/* Contains copy of psinfo struct */
#define NT_PRCRED	14		/* Contains copy of prcred struct */
#define NT_UTSNAME	15		/* Contains copy of utsname struct */
#define NT_LWPSTATUS	16		/* Contains copy of lwpstatus struct */
#define NT_LWPSINFO	17		/* Contains copy of lwpinfo struct */
#define NT_PRFPXREG	20		/* Contains copy of fprxregset struct*/

/* Legal values for the note segment descriptor types for object files.  */

#define NT_VERSION	1		/* Contains a version string.  */

////////////////////////////////////////////////////////////////////////////////

struct MCLinuxELF32Traits
{
	typedef Elf32_Ehdr Ehdr;
	typedef Elf32_Shdr Shdr;
	typedef Elf32_Phdr Phdr;
	
	inline static uint32_t round(uint32_t x)
	{
		return (x + 3) & ~3;
	}
};

struct MCLinuxELF64Traits
{
	typedef Elf64_Ehdr Ehdr;
	typedef Elf64_Shdr Shdr;
	typedef Elf64_Phdr Phdr;
	
	inline static uint32_t round(uint32_t x)
	{
		return (x + 7) & ~7;
	}
};

////////////////////////////////////////////////////////////////////////////////

static void swap_uint32(uint32_t& x)
{
	MCDeployByteSwap32(false, x);
}

static void swap_ElfX_Ehdr(Elf32_Ehdr& x)
{
	MCDeployByteSwapRecord(false, "bbbbbbbbbbbbbbbbsslllllssssss", &x, sizeof(Elf32_Ehdr));
}

static void swap_ElfX_Shdr(Elf32_Shdr& x)
{
	MCDeployByteSwapRecord(false, "llllllllll", &x, sizeof(Elf32_Shdr));
}

static void swap_ElfX_Phdr(Elf32_Phdr& x)
{
	MCDeployByteSwapRecord(false, "llllllll", &x, sizeof(Elf32_Phdr));
}

static void swap_ElfX_Ehdr(Elf64_Ehdr& x)
{
	MCDeployByteSwapRecord(false, "bbbbbbbbbbbbbbbbsslqqqlssssss", &x, sizeof(Elf64_Ehdr));
}

static void swap_ElfX_Shdr(Elf64_Shdr& x)
{
	MCDeployByteSwapRecord(false, "llqqqqllqq", &x, sizeof(Elf64_Shdr));
}

static void swap_ElfX_Phdr(Elf64_Phdr& x)
{
	MCDeployByteSwapRecord(false, "llqqqqqq", &x, sizeof(Elf64_Phdr));
}

////////////////////////////////////////////////////////////////////////////////

static bool MCDeployIsValidAndroidArch(uint16_t p_machine)
{
    if (p_machine == EM_ARM ||
        p_machine == EM_AARCH64 ||
        p_machine == EM_386 ||
        p_machine == EM_X86_64)
        return true;
    
    return false;
}

template<typename T>
static bool MCDeployToLinuxReadHeader(MCDeployFileRef p_file, bool p_is_android, typename T::Ehdr& r_header)
{
	// Read the header
	if (!MCDeployFileRead(p_file, &r_header, sizeof(typename T::Ehdr)))
		return MCDeployThrow(kMCDeployErrorLinuxNoHeader);

	// Validate the ident field to make sure the exe is what we expect.
	if (r_header . e_ident[EI_MAG0] != ELFMAG0 ||
		r_header . e_ident[EI_MAG1] != ELFMAG1 ||
		r_header . e_ident[EI_MAG2] != ELFMAG2 ||
		r_header . e_ident[EI_MAG3] != ELFMAG3)
		return MCDeployThrow(kMCDeployErrorLinuxBadHeaderMagic);

	// MW-2013-05-03: [[ Linux64 ]] Class could be 32 or 64-bit.
	if ((r_header . e_ident[EI_CLASS] != ELFCLASS32 && r_header . e_ident[EI_CLASS] != ELFCLASS64) ||
		r_header . e_ident[EI_DATA] != ELFDATA2LSB ||
		r_header . e_ident[EI_VERSION] != EV_CURRENT)
		return MCDeployThrow(kMCDeployErrorLinuxBadHeaderType);

	// Swap the fields as appropriate
	swap_ElfX_Ehdr(r_header);

	// Now check the header fields that aren't part of the ident
	if (!p_is_android)
	{
		// MW-2013-04-29: [[ Linux64 ]] Allow any type of machine architecture.
		//   (in particular, ARM and x64 in addition to x386).
		if (r_header . e_type != ET_EXEC ||
			r_header . e_version != EV_CURRENT)
			return MCDeployThrow(kMCDeployErrorLinuxBadImage);
	}
	else
	{
		if (r_header . e_type != ET_DYN ||
			!MCDeployIsValidAndroidArch(r_header.e_machine) ||
			r_header . e_version != EV_CURRENT)
			return MCDeployThrow(kMCDeployErrorLinuxBadImage);
	}

	return true;
}

template<typename T>
static bool MCDeployToLinuxReadSectionHeaders(MCDeployFileRef p_file, typename T::Ehdr& p_header, typename T::Shdr*& r_table)
{
	// First check that we can read the section headers - they must be the size
	// we think they should be.
	if (p_header . e_shentsize != sizeof(typename T::Shdr))
		return MCDeployThrow(kMCDeployErrorLinuxBadSectionSize);

	// Allocate the array of the entries
	r_table = new (nothrow) typename T::Shdr[p_header . e_shnum];
	if (r_table == NULL)
		return MCDeployThrow(kMCDeployErrorNoMemory);

	// Next read each entry in from the file
	for(uint32_t i = 0; i < p_header . e_shnum; i++)
	{
		if (!MCDeployFileReadAt(p_file, &r_table[i], sizeof(typename T::Shdr), p_header . e_shoff + i * p_header . e_shentsize))
			return MCDeployThrow(kMCDeployErrorLinuxBadSectionTable);

		swap_ElfX_Shdr(r_table[i]);
	}

	return true;
}

template<typename T>
static bool MCDeployToLinuxReadProgramHeaders(MCDeployFileRef p_file, typename T::Ehdr& p_header, typename T::Phdr*& r_table)
{
	// First check that we can read the program headers - they must be the size
	// we think they should be.
	if (p_header . e_phentsize != sizeof(typename T::Phdr))
		return MCDeployThrow(kMCDeployErrorLinuxBadSegmentSize);

	// Allocate the array of the entries
	r_table = new (nothrow) typename T::Phdr[p_header . e_phnum];
	if (r_table == NULL)
		return MCDeployThrow(kMCDeployErrorNoMemory);

	// Next read each entry in from the file
	for(uint32_t i = 0; i < p_header . e_phnum; i++)
	{
		if (!MCDeployFileReadAt(p_file, &r_table[i], sizeof(typename T::Phdr), p_header . e_phoff + i * p_header . e_phentsize))
			return MCDeployThrow(kMCDeployErrorLinuxBadProgramTable);

		swap_ElfX_Phdr(r_table[i]);
	}

	return true;
}

template<typename T>
static bool MCDeployToLinuxReadString(MCDeployFileRef p_file, typename T::Shdr& p_string_header, uint32_t p_index, char*& r_string)
{
	bool t_success = true;

	// First check that the index is valid
	if (p_index >= p_string_header . sh_size)
		return MCDeployThrow(kMCDeployErrorLinuxBadStringIndex);

	// As the string table does not contain any string lengths and they are
	// just NUL terminated, we must gradually load portions until a NUL is
	// reached.
	char *t_buffer = nullptr;
	uint32_t t_length = 0;
    
	while(t_success)
	{
		// Compute how much data to read - this is either the fixed chunk
		// size of 32, or the rest of the data in the section; whichever is
		// smaller.
		uint32_t t_request;
		t_request = MCU_min(32U, p_string_header . sh_size - (p_index + t_length));

		// If we are about to request 0 bytes, then this is a malformed
		// string.
		if (t_request == 0)
			t_success = MCDeployThrow(kMCDeployErrorLinuxBadString);

		// Try to read the chunk of data
		char t_piece[32];
		if (t_success)
			t_success = MCDeployFileReadAt(p_file, t_piece, t_request, p_string_header . sh_offset + p_index + t_length);

		// Adjust the size of request, by whether there is NUL in the piece
		uint32_t t_piece_size;
		t_piece_size = 0;
		if (t_success)
		{
			if (memchr(t_piece, '\0', t_request) == NULL)
				t_piece_size = t_request;
			else
				t_piece_size = strlen(t_piece);

			// If the resulting length is less than request, it means it
			// contains a NUL, thus increase length by one to include it.
			if (t_piece_size < t_request)
				t_piece_size += 1;
		}

		// Resize the output buffer and copy in the new data
		if (t_success)
		{
			char *t_new_buffer;
			t_new_buffer = (char *)realloc(t_buffer, t_length + t_piece_size);
			if (t_new_buffer != NULL)
			{
				t_buffer = t_new_buffer;
				memcpy(t_buffer + t_length, t_piece, t_piece_size);
				t_length += t_piece_size;
			}
			else
				t_success = MCDeployThrow(kMCDeployErrorNoMemory);
		}

		// Next check to see if we are done, by looking at the last
		// (new) byte in the buffer.
		if (t_success && t_buffer[t_length - 1] == '\0')
			break;
	}

	if (t_success)
		r_string = t_buffer;
	else
		free(t_buffer);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// This method attempts to build a Linux standalone using the given deployment
// parameters. Linux uses the ELF executable format and we require that the
// standalone engine is built with the '.project' section being the last mapped
// segment/section in the file. Thus the structure of our standalone engine
// is as follows:
//   ELF Header
//   ELF Program Table (Segments)
//   Mapped section data
//   Payload section
//   Project section
//   Unmapped section data
//   ELF Section Table
//
// To ensure the correct placement of the project section, the standalone
// engine must be built with a custom ld script (e.g. 'standalone.link'). With
// this specific structure, building the standalone is straight forward - it
// is just a case of reading in the headers/tables, updating offsets and
// writing stuff out with the project data included.
//
// Note that this method validates the structure to a good extent so build
// errors should be caught relatively easily.
//
template<typename T>
Exec_stat MCDeployToELF(const MCDeployParameters& p_params, bool p_is_android)
{
	bool t_success;
	t_success = true;

	// First thing we do is open the files.
	MCDeployFileRef t_engine, t_output;
	t_engine = t_output = NULL;
	if (t_success && !MCDeployFileOpen(p_params . engine, kMCOpenFileModeRead, t_engine))
		t_success = MCDeployThrow(kMCDeployErrorNoEngine);
	if (t_success && !MCDeployFileOpen(p_params . output, kMCOpenFileModeCreate, t_output))
		t_success = MCDeployThrow(kMCDeployErrorNoOutput);

	// Now read in the main ELF header
	typename T::Ehdr t_header;
	if (t_success)
		t_success = MCDeployToLinuxReadHeader<T>(t_engine, p_is_android, t_header);

	// Next read in the section header table
	typename T::Shdr *t_section_headers;
	t_section_headers = NULL;
	if (t_success)
		t_success = MCDeployToLinuxReadSectionHeaders<T>(t_engine, t_header, t_section_headers);

	// Next read in the program header table
	typename T::Phdr *t_program_headers;
	t_program_headers = NULL;
	if (t_success)
		t_success = MCDeployToLinuxReadProgramHeaders<T>(t_engine, t_header, t_program_headers);

	// Now we have the section header, we search for the 'project' and
	// 'payload' sections.
	typename T::Shdr *t_project_section, *t_payload_section;
	t_project_section = NULL;
	t_payload_section = NULL;
	for(uint32_t i = 0; t_success && i < t_header . e_shnum && t_project_section == NULL; i++)
	{
        MCAutoPointer<char> t_section_name;
		t_success = MCDeployToLinuxReadString<T>(t_engine, t_section_headers[t_header . e_shstrndx], t_section_headers[i] . sh_name, &t_section_name);

		// Notice that we compare 9 bytes, this is to ensure we match .project
		// only and not .project<otherchar> (i.e. we match the NUL char).
		if (t_success && memcmp(*t_section_name, ".project", 9) == 0)
			t_project_section = &t_section_headers[i];
		if (t_success && memcmp(*t_section_name, ".payload", 9) == 0)
			t_payload_section = &t_section_headers[i];
	}

	if (t_success && t_project_section == NULL)
		t_success = MCDeployThrow(kMCDeployErrorLinuxNoProjectSection);

	if (t_success && (!MCStringIsEmpty(p_params . payload)) && t_payload_section == NULL)
		t_success = MCDeployThrow(kMCDeployErrorLinuxNoPayloadSection);

	// Next check that there are no loadable sections after the project section.
	// (This check implies there are no loadable segments after the project
	// section since the section table is ordered by vaddr within segments and
    // we check segment ordering later).
	if (t_success)
    {
		for (typename T::Shdr* t_section = t_project_section + 1; t_section < t_section_headers + t_header . e_shnum; t_section += 1)
        {
			 if (t_section -> sh_addr > t_project_section -> sh_addr)
			 {
				 t_success = MCDeployThrow(kMCDeployErrorLinuxBadSectionOrder);
				 break;
			 }
         }
    }

	// Now we must search for the segment containing the payload/project sections.
	// At present, we required that these sections sit in their own segment and
	// that segment must be the last.
	typename T::Phdr *t_project_segment;
	t_project_segment = NULL;
	for(uint32_t i = 0; t_success && i < t_header . e_phnum; i++)
		if (t_project_section -> sh_addr >= t_program_headers[i] . p_vaddr &&
			t_project_section -> sh_addr + t_project_section -> sh_size <= t_program_headers[i] . p_vaddr + t_program_headers[i] . p_memsz)
		{
			t_project_segment = &t_program_headers[i];
			break;
		}

	if (t_success && t_project_segment == NULL)
		t_success = MCDeployThrow(kMCDeployErrorLinuxNoProjectSegment);

	// Make sure the payload section is in the project segment.
	if (t_success && t_payload_section != NULL &&
		(t_payload_section -> sh_addr > t_project_segment -> p_vaddr + t_project_segment -> p_memsz ||
		 t_payload_section -> sh_addr + t_payload_section -> sh_size < t_project_segment -> p_vaddr))
		 t_success = MCDeployThrow(kMCDeployErrorLinuxPayloadNotInProjectSegment);

	// Next we write out everything before we get to project (or payload)
	// section and then write out our sections themselves. After this we have
	// enough info to rewrite the headers.

	uint32_t t_output_offset;
	t_output_offset = 0;
	if (t_success)
	{
		if (t_payload_section != NULL)
			t_output_offset = t_payload_section -> sh_offset;
		else
			t_output_offset = t_project_section -> sh_offset;
	}

	// Write out the original data up to the beginning of the project section or
	// payload sections.
	if (t_success)
		t_success = MCDeployFileCopy(t_output, 0, t_engine, 0, t_output_offset);

	// Write out the payload section (if any)
	uint32_t t_payload_size;
	t_payload_size = 0;
	if (t_success && t_payload_section != NULL)
	{
		t_success = MCDeployWritePayload(p_params, false, t_output, t_output_offset, t_payload_size);
		if (t_success)
		{
			t_payload_size = T::round(t_payload_size);
			t_output_offset += t_payload_size;
		}
	}

	// Write out the project info struct
	uint32_t t_project_size, t_project_offset;
	t_project_size = 0;
	t_project_offset = 0;
	if (t_success)
	{
		t_project_offset = t_output_offset;
		t_success = MCDeployWriteProject(p_params, false, t_output, t_output_offset, t_project_size);
		if (t_success)
		{
			t_project_size = T::round(t_project_size);
			t_output_offset += t_project_size;
		}
	}

	// Next use the project size to compute the updated header values we need.
	uint32_t t_section_table_size, t_segment_table_size;
	uint32_t t_section_table_offset, t_segment_table_offset;
	uint32_t t_end_offset, t_end_size;
	if (t_success)
	{
		// Compute the shift that will have occurred to any post-project sections
		int32_t t_project_delta, t_payload_delta;
		t_project_delta = t_project_size - t_project_section -> sh_size;
		if (t_payload_section != NULL)
			t_payload_delta = t_payload_size - t_payload_section -> sh_size;
		else
			t_payload_delta = 0;

        // The sections of the file are not strictly ordered so the 
        // remainder of the file requires examining the whole section table.
        t_end_offset = t_project_section->sh_offset + t_project_section->sh_size;
            
        t_end_size = 0;
        for (size_t i = 0; i < t_header.e_shnum; i++)
        {
            uint32_t t_section_end = t_section_headers[i].sh_offset + t_section_headers[i].sh_size;
            if (t_section_end > t_end_offset + t_end_size)
                t_end_size = t_section_end - t_end_offset;
        }

		//

		if (t_payload_section != nil)
			t_payload_section -> sh_size = t_payload_size;

		t_project_section -> sh_offset = t_project_offset;
		t_project_section -> sh_addr += t_payload_delta;
		t_project_section -> sh_size = t_project_size;

		t_project_segment -> p_filesz += t_payload_delta + t_project_delta;
		t_project_segment -> p_memsz += t_payload_delta + t_project_delta;

        // Update the sections that follow the project/payload
		for (typename T::Shdr* t_section = t_project_section + 1; t_section < t_section_headers + t_header . e_shnum; t_section += 1)
        {
			if (t_section -> sh_offset >= t_end_offset)
				t_section -> sh_offset += t_project_delta + t_payload_delta;
        }

        // Adjust the section header table offset if it comes after the adjusted
        // sections.
        if (t_header.e_shoff >= t_end_offset)
            t_header.e_shoff += t_project_delta + t_payload_delta;
              
        // Adjust the segment header table offset if it coems after the adjusted
        // sections.
        if (t_header.e_phoff >= t_end_offset)
            t_header.e_phoff += t_project_delta + t_payload_delta;

		// Update the segments that follow the project/payload
        // Note that segments aren't necessarily in increasing order of file
        // offset.
        for (typename T::Phdr* t_segment = t_program_headers; t_segment < t_program_headers + t_header.e_phnum; t_segment += 1)
        {
            // Skip segments that precede the segment we adjusted
            if (t_segment == t_project_segment || t_segment->p_offset < t_end_offset)
                continue;
            
            // If any of these segments is a LOAD segment, we've broken the file!
            if (t_segment->p_type == PT_LOAD)
            {
                t_success = MCDeployThrow(kMCDeployErrorLinuxBadSectionOrder);
                break;
            }
            
            // Adjust the file offset for the segment contents
            t_segment->p_offset += t_payload_delta + t_project_delta;
        }

		t_section_table_size = t_header . e_shnum * sizeof(typename T::Shdr);
		t_section_table_offset = t_header . e_shoff;

		t_segment_table_size = t_header . e_phnum * sizeof(typename T::Phdr);
		t_segment_table_offset = t_header . e_phoff;

		//

		for(uint32_t i = 0; i < t_header . e_shnum; i++)
			swap_ElfX_Shdr(t_section_headers[i]);

		for(uint32_t i = 0; i < t_header . e_phnum; i++)
			swap_ElfX_Phdr(t_program_headers[i]);

		swap_ElfX_Ehdr(t_header);
	}

	// Overwrite the updated header and program headers
	if (t_success)
		t_success = MCDeployFileWriteAt(t_output, &t_header, sizeof(typename T::Ehdr), 0);
	if (t_success)
		t_success = MCDeployFileWriteAt(t_output, t_program_headers, t_segment_table_size, t_segment_table_offset);

	// Write out the remaining original data
	if (t_success)
		t_success = MCDeployFileCopy(t_output, t_output_offset, t_engine, t_end_offset, t_end_size);

	// Finally write out the new section table
	if (t_success)
		t_success = MCDeployFileWriteAt(t_output, t_section_headers, t_section_table_size, t_section_table_offset);

	delete[] t_program_headers;
	delete[] t_section_headers;

	MCDeployFileClose(t_engine);
	MCDeployFileClose(t_output);

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

Exec_stat MCDeployToELF(const MCDeployParameters& p_params, bool p_is_android)
{
    bool t_success;
    t_success = true;
    
    // MW-2013-05-03: [[ Linux64 ]] Snoop the engine type from the ident field.
    
    MCDeployFileRef t_engine;
    t_engine = NULL;
    if (t_success && !MCDeployFileOpen(p_params . engine, kMCOpenFileModeRead, t_engine))
        t_success = MCDeployThrow(kMCDeployErrorNoEngine);
    
    char t_ident[EI_NIDENT];
    if (t_success && !MCDeployFileRead(t_engine, t_ident, EI_NIDENT))
        t_success = MCDeployThrow(kMCDeployErrorLinuxNoHeader);
    
    if (t_success)
    {
        if (t_ident[EI_CLASS] == ELFCLASS32)
            return MCDeployToELF<MCLinuxELF32Traits>(p_params, p_is_android);
        else if (t_ident[EI_CLASS] == ELFCLASS64)
            return MCDeployToELF<MCLinuxELF64Traits>(p_params, p_is_android);
        
        t_success = MCDeployThrow(kMCDeployErrorLinuxBadHeaderType);
    }
    
    return t_success ? ES_NORMAL : ES_ERROR;
}

// This method attempts to build a Linux standalone using the given deployment
// parameters.
//
Exec_stat MCDeployToLinux(const MCDeployParameters& p_params)
{
    return MCDeployToELF(p_params, false);
}

// This method attempts to build an Android standalone using the given deployment
// parameters.
//
Exec_stat MCDeployToAndroid(const MCDeployParameters& p_params)
{
	return MCDeployToELF(p_params, true);
}

////////////////////////////////////////////////////////////////////////////////
