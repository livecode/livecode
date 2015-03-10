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

#ifndef __REPORT__
#define __REPORT__

#ifdef __cplusplus
extern "C" {
#endif

void InitializeReports(void);
void FinalizeReports(void);

int ErrorsDidOccur(void);

void Fatal_OutOfMemory(void);
void Fatal_InternalInconsistency(const char *message);

void Error_CouldNotGenerateBytecode(void);
void Error_CouldNotGenerateInterface(void);
void Error_CouldNotOpenInputFile(const char *path);
void Error_CouldNotWriteOutputFile(const char *path);
void Error_CouldNotWriteInterfaceFile(const char *path);
void Error_MalformedToken(long position, const char *token);
void Error_MalformedSyntax(long position);
    
void Warning_EmptyUnicodeEscape(long position);
void Warning_UnicodeEscapeTooBig(long position);
    
#ifdef __cplusplus
}
#endif

#endif
