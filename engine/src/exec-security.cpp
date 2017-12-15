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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "util.h"
#include "mcssl.h"

#include "exec.h"

#include "stacksecurity.h"
#include "osspec.h"

////////////////////////////////////////////////////////////////////////////////

void MCSecurityEvalEncrypt(MCExecContext& ctxt, MCStringRef p_source, MCStringRef& r_dest)
{
	if (MCStackSecurityEncryptString(p_source, r_dest))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCSecurityEvalCipherNames(MCExecContext& ctxt, MCStringRef& r_names)
{
	MCAutoListRef t_list;
	MCAutoStringRef t_error;

#ifdef MCSSL
	if (SSL_ciphernames(&t_list, &t_error))
	{
		if (*t_error != nil)
			ctxt.SetTheResultToValue(*t_error);
		if (MCListCopyAsString(*t_list, r_names))
			return;
	}
#endif /* MCSSL */

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCSecurityEvalRandomBytes(MCExecContext& ctxt, uinteger_t p_byte_count, MCDataRef& r_bytes)
{
	if (!InitSSLCrypt())
	{
		ctxt.LegacyThrow(EE_SECURITY_NOLIBRARY);
		return;
	}

	if (MCSRandomData (p_byte_count, r_bytes))
	{
		ctxt.SetTheResultToEmpty();
		return;
	}
	
	ctxt.SetTheResultToCString("error: could not get random bytes");
}

////////////////////////////////////////////////////////////////////////////////

void MCSecurityExecRsaOperation(MCExecContext& ctxt, bool p_is_decrypt, MCStringRef p_data, bool p_is_public, MCStringRef p_key, MCStringRef p_passphrase)
{
	ctxt . SetTheResultToEmpty();
	
	MCAutoStringRef t_rsa_out;
	MCAutoStringRef t_result;
	uint32_t t_error;
	if (MCCrypt_rsa_op(!p_is_decrypt, p_is_public, p_data, p_key, p_passphrase, &t_rsa_out, &t_result, t_error))
		ctxt . SetItToValue(*t_rsa_out);
	else
	{
		if (*t_result != NULL)
			ctxt . SetTheResultToValue(*t_result);
		else
		{
			ctxt . LegacyThrow((Exec_errors)t_error);
			return;
		}
	}
}

void MCSecurityExecRsaEncrypt(MCExecContext& ctxt, MCStringRef p_data, bool p_is_public, MCStringRef p_key, MCStringRef p_passphrase)
{
	MCSecurityExecRsaOperation(ctxt, false, p_data, p_is_public, p_key, p_passphrase);
}

void MCSecurityExecRsaDecrypt(MCExecContext& ctxt, MCStringRef p_data, bool p_is_public, MCStringRef p_key, MCStringRef p_passphrase)
{
	MCSecurityExecRsaOperation(ctxt, true, p_data, p_is_public, p_key, p_passphrase);
}

void MCSecurityExecBlockOperation(MCExecContext& ctxt, bool p_is_decrypt, MCStringRef p_data, MCNameRef p_cipher, MCStringRef p_key, bool p_is_password, MCStringRef p_salt, MCStringRef p_iv, uint2 p_bit_rate)
{
	ctxt . SetTheResultToEmpty();
	
	MCAutoStringRef t_output;
	MCAutoStringRef t_result;
	uint32_t t_error;

	if (SSL_encode(p_is_decrypt, p_cipher, p_data, p_key, p_is_password, p_salt, p_iv, p_bit_rate, &t_output, &t_result, t_error))
		ctxt . SetItToValue(*t_output);
	else
	{
		if (*t_result != NULL)
			ctxt . SetTheResultToValue(*t_result);
		else
		{
			ctxt . LegacyThrow((Exec_errors)t_error);
			return;
		}
	}
}

void MCSecurityExecBlockEncryptWithPassword(MCExecContext& ctxt, MCStringRef p_data, MCNameRef p_cipher, MCStringRef p_password, MCStringRef p_salt, MCStringRef p_iv, uint2 p_bit_rate)
{
	MCSecurityExecBlockOperation(ctxt, false, p_data, p_cipher, p_password, true, p_salt, p_iv, p_bit_rate);
}

void MCSecurityExecBlockEncryptWithKey(MCExecContext& ctxt, MCStringRef p_data, MCNameRef p_cipher, MCStringRef p_key, MCStringRef p_iv, uint2 p_bit_rate)
{
	MCSecurityExecBlockOperation(ctxt, false, p_data, p_cipher, p_key, false, nil, p_iv, p_bit_rate);
}

void MCSecurityExecBlockDecryptWithPassword(MCExecContext& ctxt, MCStringRef p_data, MCNameRef p_cipher, MCStringRef p_password, MCStringRef p_salt, MCStringRef p_iv, uint2 p_bit_rate)
{
	MCSecurityExecBlockOperation(ctxt, true, p_data, p_cipher, p_password, true, p_salt, p_iv, p_bit_rate);
}

void MCSecurityExecBlockDecryptWithKey(MCExecContext& ctxt, MCStringRef p_data, MCNameRef p_cipher, MCStringRef p_key, MCStringRef p_iv, uint2 p_bit_rate)
{
	MCSecurityExecBlockOperation(ctxt, true, p_data, p_cipher, p_key, false, nil, p_iv, p_bit_rate);
}

////////////////////////////////////////////////////////////////////////////////

void MCSecurityGetSslCertificates(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (MCStringCreateWithCString(MCsslcertificates, r_value))
		return;

	ctxt . Throw();
}

void MCSecuritySetSslCertificates(MCExecContext& ctxt, MCStringRef p_value)
{
	delete MCsslcertificates;
    char* t_value;
    /* UNCHECKED */ MCStringConvertToCString(p_value, t_value);
	MCsslcertificates = t_value;
}

void MCSecurityExecSecureSocket(MCExecContext& ctxt, MCNameRef p_socket, bool p_secure_verify, MCNameRef p_end_hostname)
{
    uindex_t t_index;
	if (IO_findsocket(p_socket, t_index))
	{
		MCS_secure_socket(MCsockets[t_index], p_secure_verify, p_end_hostname);
		ctxt . SetTheResultToEmpty();
        return;
	}
    
    ctxt . SetTheResultToStaticCString("socket is not open");
}
