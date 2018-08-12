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

#ifndef __OUTPUT__
#define __OUTPUT__

#include <stdio.h>
#include <stdint.h>
#include <literal.h>

#ifdef __cplusplus
extern "C" {
#endif
	
void OutputFileBegin(FILE *p_file);
void OutputEnd(void);
void OutputWrite(const char *msg);
void OutputWriteI(const char *left, NameRef name, const char *right);
void OutputWriteS(const char *left, const char *string, const char *right);
void OutputWriteD(const char *left, double *number, const char *right);
void OutputWriteN(const char *left, intptr_t number, const char *right);
void OutputWriteXmlS(const char *p_left, const char *p_string, const char *p_right);

bool OutputIsValid(void);
	
#ifdef __cplusplus
}
#endif

#endif
