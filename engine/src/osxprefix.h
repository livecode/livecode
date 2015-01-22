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

#include "globdefs.h"

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

#include <unistd.h>

inline char *pStrcpy(unsigned char *dest, const unsigned char *src)
{
	BlockMove(src, dest, (long)*src + 1);
	return (char*)dest;
}

extern char *MCS_FSSpec2path(FSSpec *fileSpec);
extern OSErr MCS_path2FSSpec(const char *fname, FSSpec *fspec);

extern OSErr MCS_fsref_to_fsspec(const FSRef *p_fsref, FSSpec *r_fsspec);
extern OSErr MCS_pathtoref(const char *p_path, FSRef *r_ref);
extern OSErr MCS_pathtoref(const MCString& p_path, FSRef *r_ref);
// SN-2015-01-16: [[ Bug 14392 ]] Used in osxspec.cpp
extern bool MCS_apply_redirect(char*& x_path, bool p_is_file);
extern OSErr MCS_pathtoref_and_leaf(const char *p_path, FSRef& r_ref, UniChar*& r_leaf, UniCharCount& r_leaf_length);
extern char *MCS_fsref_to_path(FSRef& p_ref);

extern Boolean MCS_imeisunicode();
extern Boolean MCS_handle_sockets();

extern const char *MCS_openresourcefork_with_path(const char *p_path, SInt8 p_permission, bool p_create, SInt16 *r_fork_ref);
extern const char *MCS_openresourcefork_with_path(const MCString& p_path, SInt8 p_permission, bool p_create, SInt16 *r_fork_ref);
extern const char *MCS_openresourcefile_with_path(const char *p_path, SInt8 p_permission, bool p_create, SInt16 *r_fork_ref);
extern const char *MCS_openresourcefile_with_fsref(FSRef *p_fsref, SInt8 p_permission, bool p_create, SInt16 *r_fork_ref);
extern void MCS_closeresourcefile(SInt16 p_fork_ref);

class MCMacOSXTransferData;
