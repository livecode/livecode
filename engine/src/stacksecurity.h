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

#ifndef STACKSECURITY_H
#define STACKSECURITY_H

#include "stack.h"

////////////////////////////////////////////////////////////////////////////////

void MCStackSecurityInit(void);

//////////

bool MCStackSecurityEncryptString(MCStringRef p_string, MCStringRef &r_enc);

//////////

bool MCStackSecurityCreateStack(MCStack *&r_stack);
bool MCStackSecurityCopyStack(const MCStack *p_stack, MCStack *&r_copy);

bool MCStackSecurityCreateObjectInputStream(IO_handle p_stream, uint32_t p_length, bool p_new_format, MCObjectInputStream *&r_object_stream);
bool MCStackSecurityCreateObjectOutputStream(IO_handle p_stream, MCObjectOutputStream *&r_object_stream);

//////////

bool MCStackSecurityIsIOEncrypted(void);
bool MCStackSecurityIsIOEncryptionEnabled(void);
void MCStackSecuritySetIOEncryptionEnabled(bool p_encrypted);

//////////

IO_stat MCStackSecurityWrite(const char *p_string, uint32_t p_length, IO_handle p_stream);
IO_stat MCStackSecurityRead(char *r_string, uint32_t p_length, IO_handle p_stream);
IO_stat MCStackSecurityReadUTF8StringRef(MCStringRef& r_string, uint32_t p_length, IO_handle p_stream);

//////////

void MCStackSecurityProcessCapsule(void *p_start, void *p_finish);

//////////

struct MCDeployParameters;
bool MCStackSecurityPreDeploy(uint32_t p_platform, MCDeployParameters& p_params);
void MCStackSecurityExecutionTimeout(void);

////////////////////////////////////////////////////////////////////////////////

#endif
