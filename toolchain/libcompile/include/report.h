/* Copyright (C) 2016 LiveCode Ltd.
 
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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int s_is_werror_enabled;
extern int s_verbose_level;

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
void Error_MalformedToken(intptr_t position, const char *token);
void Error_MalformedSyntax(intptr_t position);
void Error_IntegerLiteralOutOfRange(intptr_t position);
void Error_InvalidIntegerLiteral(intptr_t position);
void Error_InvalidDoubleLiteral(intptr_t position);
    
void Warning_EmptyUnicodeEscape(intptr_t position);
void Warning_UnicodeEscapeTooBig(intptr_t position);

void Error_Bootstrap(const char *format, ...);

void Debug(const char *p_format, ...);
void Debug_Emit(const char *p_format, ...);
void Debug_Depend(const char *p_format, ...);

#ifdef __cplusplus
}
#endif

#endif
