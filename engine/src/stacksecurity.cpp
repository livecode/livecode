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

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "objectstream.h"
#include "mcio.h"
#include "license.h"

#include "stacksecurity.h"

////////////////////////////////////////////////////////////////////////////////

void MCStackSecurityInit(void)
{
	// MW-2013-11-07: [[ CmdLineStack ]] Mark the license type as community.
	MClicenseparameters . license_class = kMCLicenseClassCommunity;  
}

///////////

bool MCStackSecurityEncryptString(MCStringRef p_string, MCStringRef &r_enc)
{
	return false;
}

///////////

bool MCStackSecurityCreateStack(MCStack *&r_stack)
{
	r_stack = new (nothrow) MCStack();
	return r_stack != nil;
}

bool MCStackSecurityCopyStack(const MCStack *p_stack, MCStack *&r_copy)
{
	r_copy = new (nothrow) MCStack(*p_stack);
	return r_copy != nil;
}

///////////

bool MCStackSecurityCreateObjectInputStream(IO_handle p_stream, uint32_t p_length, bool p_new_format, MCObjectInputStream *&r_object_stream)
{
	r_object_stream = new (nothrow) MCObjectInputStream(p_stream, p_length, p_new_format);
	return r_object_stream != nil;
}

bool MCStackSecurityCreateObjectOutputStream(IO_handle p_stream, MCObjectOutputStream *&r_object_stream)
{
	r_object_stream = new (nothrow) MCObjectOutputStream(p_stream);
	return r_object_stream != nil;
}

//////////

bool MCStackSecurityIsIOEncrypted(void)
{
	return false;
}

bool MCStackSecurityIsIOEncryptionEnabled(void)
{
	return false;
}

void MCStackSecuritySetIOEncryptionEnabled(bool p_encrypted)
{
}

//////////

IO_stat MCStackSecurityWrite(const char *p_string, uint32_t p_length, IO_handle p_stream)
{
	IO_stat t_stat;
	t_stat = MCS_write(p_string, sizeof(char), p_length, p_stream);
	
	return t_stat;
}

IO_stat MCStackSecurityRead(char *r_string, uint32_t p_length, IO_handle p_stream)
{
	IO_stat t_stat;
	t_stat = MCS_readfixed(r_string, p_length, p_stream);
	
	return t_stat;
}

IO_stat MCStackSecurityReadUTF8StringRef(MCStringRef& r_string, uint32_t p_length, IO_handle p_stream)
{
    MCAutoArray<byte_t> t_utf8_string;
    if (!t_utf8_string.New(p_length))
        return IO_ERROR;
    
    if (MCStackSecurityRead(reinterpret_cast<char *>(t_utf8_string.Ptr()),
                            p_length,
                            p_stream) != IO_NORMAL)
        return IO_ERROR;
    
    if (!MCStringCreateWithBytes(t_utf8_string.Ptr(),
                                 p_length,
                                 kMCStringEncodingUTF8,
                                 false,
                                 r_string))
        return IO_ERROR;
    
    return IO_NORMAL;
}

///////////

void MCStackSecurityProcessCapsule(void *p_start, void *p_finish)
{
}

///////////

bool MCStackSecurityPreDeploy(uint32_t p_platform, MCDeployParameters& p_params)
{
	return true;
}

void MCStackSecurityExecutionTimeout(void)
{
}

////////////////////////////////////////////////////////////////////////////////
