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

bool SSL_ciphernames(MCListRef& r_list, MCStringRef& r_error);
unsigned long SSLError(MCStringRef& errbuf);
char *SSL_encode(Boolean isdecrypt, const char *ciphername,
                 const char *data, uint4 inlen,uint4 &outlen, //data to decrypt, length of that data, and pointer to descypted data length
                 const char *keystr, int4 keystrlen, Boolean ispassword, uint2 keylen,
                 const char *saltstr,  uint2 saltlen, const char *ivstr, uint2 ivlen);

bool SSL_encode(bool p_is_decrypt, MCNameRef p_cipher, MCStringRef p_data, MCStringRef p_key, bool p_is_password,
				MCStringRef p_salt, MCStringRef p_iv, uint2 p_bit_rate, MCStringRef &r_output, MCStringRef &r_result, uint32_t &r_error);

bool MCCrypt_rsa_op(bool p_encrypt, RSA_KEYTYPE p_key_type, const char *p_message_in, uint32_t p_message_in_length,
					const char *p_key, uint32_t p_key_length, const char *p_passphrase,
					char *&r_message_out, uint32_t &r_message_out_length, char *&r_result, uint32_t &r_error);

bool SSL_random_bytes(const void *p_buffer, uindex_t p_count);


bool MCCrypt_rsa_op(bool p_encrypt, bool p_is_public, MCStringRef p_message_in, MCStringRef p_key, 
					MCStringRef p_passphrase, MCStringRef &r_message_out, MCStringRef &r_result, uint32_t &r_error);

// IM-2014-07-28: [[ Bug 12822 ]] Shared certificate loading function for SSL contexts.
bool MCSSLContextLoadCertificates(SSL_CTX *p_context, MCStringRef *r_error);

#if defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID)
// MM-2015-06-04: [[ MobileSockets ]] Return true if we should trust the certificates in the given SSL connection.
bool MCSSLVerifyCertificate(SSL *ssl, MCStringRef p_host_name, MCStringRef &r_error);
#endif

#endif
