/*                                                                     -*-c++-*-
Copyright (C) 2015 Runtime Revolution Ltd.

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

#ifndef __MC_FOUNDATION_FILE_PRIVATE__
#define __MC_FOUNDATION_FILE_PRIVATE__

/* ----------------------------------------------------------------
 * Errors
 * ---------------------------------------------------------------- */

bool __MCFileThrowIOErrorWithErrno (MCStringRef p_native_path, MCStringRef p_message, int p_errno);
bool __MCFileThrowReadErrorWithErrno (MCStringRef p_native_path, int p_errno);
bool __MCFileThrowWriteErrorWithErrno (MCStringRef p_native_path, int p_errno);
bool __MCFileThrowOpenErrorWithErrno (MCStringRef p_native_path, int p_errno);
bool __MCFileThrowInvalidPathError (MCStringRef p_path);

/* ----------------------------------------------------------------
 * Per-platform whole-file IO
 * ---------------------------------------------------------------- */

bool __MCFileGetContents (MCStringRef p_native_path, MCDataRef & r_data);
bool __MCFileSetContents (MCStringRef p_native_path, MCDataRef p_data);

/* ----------------------------------------------------------------
 * Per-platform path manipulation
 * ---------------------------------------------------------------- */

bool __MCFilePathToNative (MCStringRef p_path, MCStringRef & r_native_path);
bool __MCFilePathFromNative (MCStringRef p_native_path, MCStringRef & r_path);

/* ----------------------------------------------------------------
 * Stream creation
 * ---------------------------------------------------------------- */

bool __MCFileCreateStream (MCStringRef p_native_path, intenum_t p_mode, MCStreamRef & r_stream);

#endif /* __MC_FOUNDATION_FILE_PRIVATE__ */
