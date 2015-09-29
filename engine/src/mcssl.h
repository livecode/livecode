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

#ifndef	MCSSL_H
#define	MCSSL_H

Boolean InitSSLCrypt();
void ShutdownSSL();

// MM-2014-02-14: [[ LibOpenSSL 1.0.1e ]] Added initialise.
void InitialiseSSL(void);

void SSL_ciphernames(MCExecPoint &ep);
unsigned long SSLError(char *errbuf);
char *SSL_encode(Boolean isdecrypt, char *ciphername,
                 const char *data, uint4 inlen,uint4 &outlen, //data to decrypt, length of that data, and pointer to descypted data length
                 const char *keystr, int4 keystrlen, Boolean ispassword, uint2 keylen,
                 const char *saltstr,  uint2 saltlen, const char *ivstr, uint2 ivlen);

bool MCCrypt_rsa_op(bool p_encrypt, RSA_KEYTYPE p_key_type, const char *p_message_in, uint32_t p_message_in_length,
			const char *p_key, uint32_t p_key_length, const char *p_passphrase,
			char *&r_message_out, uint32_t &r_message_out_length, char *&r_result, uint32_t &r_error);

// IM-2014-07-28: [[ Bug 12822 ]] Shared certificate loading function for SSL contexts.
bool MCSSLContextLoadCertificates(SSL_CTX *p_context, char **r_error);

#endif
