/*
 *  inifile.h
 *
 *  $Id: inifile.h,v 1.4 2006/01/20 15:58:35 source Exp $
 *
 *  Read/Write .ini files
 *
 *  The iODBC driver manager.
 *
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


#ifndef _INIFILE_H
#define _INIFILE_H

#include <fcntl.h>
#ifndef _MAC
#include <sys/types.h>
#endif

/* configuration file entry */
typedef struct TCFGENTRY
  {
    char *section;
    char *id;
    char *value;
    char *comment;
    unsigned short flags;
  }
TCFGENTRY, *PCFGENTRY;

/* values for flags */
#define CFE_MUST_FREE_SECTION	0x8000
#define CFE_MUST_FREE_ID	0x4000
#define CFE_MUST_FREE_VALUE	0x2000
#define CFE_MUST_FREE_COMMENT	0x1000

/* configuration file */
typedef struct TCFGDATA
  {
    char *fileName;		/* Current file name */

    int dirty;			/* Did we make modifications? */

    char *image;		/* In-memory copy of the file */
    size_t size;		/* Size of this copy (excl. \0) */
    time_t mtime;		/* Modification time */

    unsigned int numEntries;
    unsigned int maxEntries;
    PCFGENTRY entries;

    /* Compatibility */
    unsigned int cursor;
    char *section;
    char *id;
    char *value;
    char *comment;
    unsigned short flags;

  }
TCONFIG, *PCONFIG;

#define CFG_VALID		0x8000
#define CFG_EOF			0x4000

#define CFG_ERROR		0x0000
#define CFG_SECTION		0x0001
#define CFG_DEFINE		0x0002
#define CFG_CONTINUE		0x0003

#define CFG_TYPEMASK		0x000F
#define CFG_TYPE(X)		((X) & CFG_TYPEMASK)
#define _iodbcdm_cfg_valid(X)	((X) != NULL && ((X)->flags & CFG_VALID))
#define _iodbcdm_cfg_eof(X)	((X)->flags & CFG_EOF)
#define _iodbcdm_cfg_section(X)	(CFG_TYPE((X)->flags) == CFG_SECTION)
#define _iodbcdm_cfg_define(X)	(CFG_TYPE((X)->flags) == CFG_DEFINE)
#define _iodbcdm_cfg_cont(X)	(CFG_TYPE((X)->flags) == CFG_CONTINUE)

int _iodbcdm_cfg_init (PCONFIG * ppconf, const char *filename, int doCreate);
int _iodbcdm_cfg_done (PCONFIG pconfig);
int _iodbcdm_cfg_freeimage (PCONFIG pconfig);
int _iodbcdm_cfg_refresh (PCONFIG pconfig);
int _iodbcdm_cfg_storeentry (PCONFIG pconfig, char *section, char *id,
    char *value, char *comment, int dynamic);
int _iodbcdm_cfg_rewind (PCONFIG pconfig);
int _iodbcdm_cfg_nextentry (PCONFIG pconfig);
int _iodbcdm_cfg_find (PCONFIG pconfig, char *section, char *id);
int _iodbcdm_cfg_next_section (PCONFIG pconfig);

int _iodbcdm_cfg_write (PCONFIG pconfig, char *section, char *id, char *value);
int _iodbcdm_cfg_commit (PCONFIG pconfig);
int _iodbcdm_cfg_getstring (PCONFIG pconfig, char *section, char *id,
    char **valptr);
int _iodbcdm_cfg_getlong (PCONFIG pconfig, char *section, char *id,
    long *valptr);
int _iodbcdm_cfg_getshort (PCONFIG pconfig, char *section, char *id,
    short *valptr);
int _iodbcdm_cfg_search_init (PCONFIG * ppconf, const char *filename,
    int doCreate);
int _iodbcdm_list_entries (PCONFIG pCfg, LPCSTR lpszSection,
    LPSTR lpszRetBuffer, int cbRetBuffer);
int _iodbcdm_list_sections (PCONFIG pCfg, LPSTR lpszRetBuffer, int cbRetBuffer);
BOOL do_create_dsns (PCONFIG pCfg, PCONFIG pInfCfg, LPSTR szDriver,
    LPSTR szDSNS, LPSTR szDiz);
BOOL install_from_ini (PCONFIG pCfg, PCONFIG pOdbcCfg, LPSTR szInfFile,
    LPSTR szDriver, BOOL drivers);
int install_from_string (PCONFIG pCfg, PCONFIG pOdbcCfg, LPSTR lpszDriver,
    BOOL drivers);

#endif /* _INIFILE_H */
