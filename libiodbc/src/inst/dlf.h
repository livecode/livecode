/*
 *  dlf.h
 *
 *  $Id: dlf.h,v 1.2 2006/01/20 15:58:35 source Exp $
 *
 *  Dynamic Library Loader (mapping to SVR4)
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1995 by Ke Jin <kejin@empress.com>
 *  Copyright (C) 1996-2006 by OpenLink Software <iodbc@openlinksw.com>
 *  All Rights Reserved.
 *
 *  This software is released under the terms of either of the following
 *  licenses:
 *
 *      - GNU Library General Public License (see LICENSE.LGPL)
 *      - The BSD License (see LICENSE.BSD).
 *
 *  Note that the only valid version of the LGPL license as far as this
 *  project is concerned is the original GNU Library General Public License
 *  Version 2, dated June 1991.
 *
 *  While not mandated by the BSD license, any patches you make to the
 *  iODBC source code may be contributed back into the iODBC project
 *  at your discretion. Contributions will benefit the Open Source and
 *  Data Access community as a whole. Submissions may be made at:
 *
 *      http://www.iodbc.org
 *
 *
 *  GNU Library Generic Public License Version 2
 *  ============================================
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; only
 *  Version 2 of the License dated June 1991.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  The BSD License
 *  ===============
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *  3. Neither the name of OpenLink Software Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OPENLINK OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_DLF_H
#define _DLF_H
#include <iodbc.h>

#if defined(HAVE_SHL_LOAD)
#define DLDAPI_HP_SHL
#elif defined(HAVE_LIBDL)
#define DLDAPI_SVR4_DLFCN
#elif defined(HAVE_DYLD)
#define DLDAPI_MACX
#endif

#if defined(DLDAPI_SVR4_DLFCN)
#include <dlfcn.h>
#elif defined(DLDAPI_AIX_LOAD)
#include <dlfcn.h>
#elif defined(DLDAPI_VMS_IODBC) || defined(DLDAPI_MACX)
extern void *iodbc_dlopen (char * path, int mode);
extern void *iodbc_dlsym (void * hdll, char * sym);
extern char *iodbc_dlerror ();
extern int iodbc_dlclose (void * hdll);
#else
extern void *dlopen (char * path, int mode);
extern void *dlsym (void * hdll, char * sym);
extern char *dlerror ();
extern int dlclose (void * hdll);
#endif


#ifdef DLDAPI_MACX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mach-o/dyld.h"

#define RTLD_LAZY		0x1
#define RTLD_NOW		0x2
#define RTLD_LOCAL		0x4
#define RTLD_GLOBAL		0x8
#define RTLD_NOLOAD		0x10
#define RTLD_SHARED		0x20	/* not used, the default */
#define RTLD_UNSHARED		0x40
#define RTLD_NODELETE		0x80
#define RTLD_LAZY_UNDEF		0x100


enum ofile_type
{
  OFILE_UNKNOWN,
  OFILE_FAT,
  OFILE_ARCHIVE,
  OFILE_Mach_O
};

enum byte_sex
{
  UNKNOWN_BYTE_SEX,
  BIG_ENDIAN_BYTE_SEX,
  LITTLE_ENDIAN_BYTE_SEX
};


/*
 * The structure describing an architecture flag with the string of the flag
 * name, and the cputype and cpusubtype.
 */
struct arch_flag
{
  char *name;
  cpu_type_t cputype;
  cpu_subtype_t cpusubtype;
};

/*
 * The structure used by ofile_*() routines for object files.
 */
struct ofile
{
  char *file_name;		   /* pointer to name malloc'ed by ofile_map */
  char *file_addr;		   /* pointer to vm_allocate'ed memory       */
  unsigned long file_size;	   /* size of vm_allocate'ed memory          */
  enum ofile_type file_type;	   /* type of the file                       */

  struct fat_header *fat_header;   /* If a fat file these are filled in and  */
  struct fat_arch *fat_archs;	   /*   if needed converted to host byte sex */

  /* 
   *  If this is a fat file then these are valid and filled in 
   */
  unsigned long narch;		   /* the current architecture               */
  enum ofile_type arch_type;	   /* the type of file for this arch.        */
  struct arch_flag arch_flag;	   /* the arch_flag for this arch, the name  */
				   /*   field is pointing at space malloc'ed */
				   /*   by ofile_map.                        */

  /* 
   *  If this structure is currently referencing an archive member or 
   *  an object file that is an archive member these are valid and filled in. 
   */
  unsigned long member_offset;	   /* logical offset to the member starting  */
  char *member_addr;		   /* pointer to the member contents         */
  unsigned long member_size;	   /* actual size of the member (not rounded)*/
  struct ar_hdr *member_ar_hdr;	   /* pointer to the ar_hdr for this member  */
  char *member_name;		   /* name of this member                    */
  unsigned long member_name_size;  /* size of the member name                */
  enum ofile_type member_type;	   /* the type of file for this member       */
  cpu_type_t archive_cputype;	   /* if the archive contains objects then   */
   cpu_subtype_t		   /*   these two fields reflect the object  */
   archive_cpusubtype;		   /*   at are in the archive.               */

  /* 
   *  If this structure is currently referencing a dynamic library module 
   *  these are valid and filled in. 
   */
  struct dylib_module *modtab;	   /* the module table                       */
  unsigned long nmodtab;	   /* the number of module table entries     */
  struct dylib_module		   /* pointer to the dylib_module for this   */
      *dylib_module;		   /*   module                               */
  char *dylib_module_name;	   /* the name of the module                 */

  /* 
   *  If this structure is currently referencing an object file these are
   *  valid and filled in.  The mach_header and load commands have been 
   *  converted to the host byte sex if needed 
   */
  char *object_addr;		   /* the address of the object file         */
  unsigned long object_size;	   /* the size of the object file            */
  enum byte_sex object_byte_sex;   /* the byte sex of the object file        */
  struct mach_header *mh;	   /* the mach_header of the object file     */
  struct load_command		   /* the start of the load commands         */
      *load_commands;
};


/*
 * The structure of a dlopen() handle.
 */
struct dlopen_handle
{
  dev_t dev;		/* the path's device and inode number from stat(2) */
  ino_t ino;
  int dlopen_mode;	/* current dlopen mode for this handle */
  int dlopen_count;	/* number of times dlopen() called on this handle */
  NSModule module;	/* the NSModule returned by NSLinkModule() */
  struct dlopen_handle *prev;
  struct dlopen_handle *next;
};
#endif /* DLDAPI_MACX */

#ifndef RTLD_LOCAL
#define RTLD_LOCAL	0	/* Only if not defined by dlfcn.h */
#endif
#ifndef RTLD_LAZY
#define RTLD_LAZY	1
#endif

#ifdef RTLD_NOW
#define OPL_DL_MODE	(RTLD_NOW | RTLD_LOCAL)
#else
#define OPL_DL_MODE	(RTLD_LAZY | RTLD_LOCAL)
#endif

#if defined(DLDAPI_VMS_IODBC) || defined(DLDAPI_MACX)
#define	DLL_OPEN(dll)		(void*)iodbc_dlopen((char*)(dll), OPL_DL_MODE)
#define	DLL_PROC(hdll, sym)	(void*)iodbc_dlsym((void*)(hdll), (char*)sym)
#define	DLL_ERROR()		(char*)iodbc_dlerror()
#define	DLL_CLOSE(hdll)		iodbc_dlclose((void*)(hdll))
#else
#define	DLL_OPEN(dll)		(void*)dlopen((char*)(dll), OPL_DL_MODE)
#define	DLL_PROC(hdll, sym)	(void*)dlsym((void*)(hdll), (char*)sym)
#define	DLL_ERROR()		(char*)dlerror()
#define	DLL_CLOSE(hdll)		dlclose((void*)(hdll))
#endif

#endif /* _DLF_H */
