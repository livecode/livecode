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

#include "globdefs.h"

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

#include <unistd.h>

inline char *pStrcpy(unsigned char *dest, const unsigned char *src)
{
	BlockMove(src, dest, (long)*src + 1);
	return (char*)dest;
}

extern OSErr MCS_path2FSSpec(const char *fname, FSSpec *fspec);
extern OSErr MCS_path2FSSpec(MCStringRef p_filename, FSSpec *fspec);

extern OSErr MCS_fsref_to_fsspec(const FSRef *p_fsref, FSSpec *r_fsspec);

extern OSErr MCS_mac_pathtoref(MCStringRef p_path, FSRef& r_ref);
extern bool MCS_mac_fsref_to_path(FSRef& p_ref, MCStringRef& r_path);

extern bool MCS_mac_openresourcefile_with_path(MCStringRef p_path, SInt8 p_permission, bool p_create, SInt16& r_fork_ref, MCStringRef& r_error);
extern void MCS_mac_closeresourcefile(SInt16 p_fork_ref);

extern bool MCS_mac_FSSpec2path(FSSpec *fileSpec, MCStringRef &r_path);

class MCMacOSXTransferData;
