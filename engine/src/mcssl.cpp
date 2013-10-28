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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "param.h"
#include "mcerror.h"
#include "execpt.h"
#include "util.h"
#include "object.h"
#include "mcssl.h"
#include "mode.h"

#include "globals.h"

#include "license.h"

#ifdef MCSSL
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#endif

static Boolean cryptinited = False;

extern "C" int initialise_weak_link_crypto(void);
extern "C" int initialise_weak_link_ssl(void);

Boolean load_crypto_symbols()
{
#ifdef MCSSL

// MW-2009-07-21: Weakly link only on non-MacOSX platforms.
#if !defined(_MACOSX) && !defined(_SERVER)
	if (!initialise_weak_link_crypto())
		return False;
	if (!initialise_weak_link_ssl())
		return False;
#else
	return True;
#endif

	return True;
#else
	return False;
#endif
}




Boolean InitSSLCrypt()
{
	if (!cryptinited)
	{
		if (!load_crypto_symbols())
			return False;

#ifdef MCSSL
		ERR_load_crypto_strings();
		OpenSSL_add_all_ciphers();
		RAND_seed(&MCrandomseed,I4L);
		cryptinited = True;
#endif

	}
	return True;
}

// MW-2005-02-20: Seed RNG on Win32 with system events.
//   This routine is called every time a message arrives.
#ifdef _WINDOWS
typedef int (*RAND_eventPTR)(UINT iMsg, WPARAM wParam, LPARAM lParam);

void SeedSSL(UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static int s_no_crypto = FALSE;
	static RAND_eventPTR RAND_eventDL = NULL;
	if (!s_no_crypto && MCModeCollectEntropy())
	{
		if (RAND_eventDL == NULL)
		{
			HMODULE t_library;
			t_library = LoadLibraryA("revsecurity.dll");
			if (t_library != NULL)
				RAND_eventDL = (RAND_eventPTR)GetProcAddress(t_library, "RAND_event");

			s_no_crypto = (RAND_eventDL == NULL);
		}

		if (RAND_eventDL != NULL)
			RAND_eventDL(iMsg, wParam, lParam);
	}
}
#endif

void ShutdownSSL()
{
#ifdef MCSSL
	if (cryptinited)
	{

		EVP_cleanup();
		ERR_free_strings();
	}
#endif
}

void shutdown()
{}

//error buf should have a buffer of at least 256 bytes.
unsigned long SSLError(MCStringRef errbuf)
{
	if (!InitSSLCrypt())
	{
		errbuf = MCSTR("ssl library not found");
		return 0;
	}
#ifdef MCSSL
	unsigned long ecode = ERR_get_error();
	if (!MCStringIsEmpty(errbuf))
	{
		if (ecode)
        {
            MCAutoPointer<char> t_errbuf;
            t_errbuf = new char[256];
            ERR_error_string_n(ecode,&t_errbuf,255);
            /* UNCHECKED */ MCStringCreateWithCString(*t_errbuf, errbuf);
        }
		else
			errbuf = MCValueRetain(kMCEmptyString);
	}
	return ecode;
#else
	return 0;
#endif
}

#ifdef MCSSL
bool load_pem_key(const char *p_data, uint32_t p_length, RSA_KEYTYPE p_type, const char *p_passphrase, EVP_PKEY *&r_key)
{
	bool t_success = true;
	BIO *t_data = NULL;
	EVP_PKEY *t_key = NULL;
	t_data = BIO_new_mem_buf((void*)p_data, p_length);
	t_success = t_data != NULL;
	char t_empty_pass[] = "";
	char *t_passphrase = (p_passphrase != NULL) ? (char*)p_passphrase : t_empty_pass;

	if (t_success)
	{
		switch (p_type)
		{
		case RSAKEY_PUBKEY:
			t_key = PEM_read_bio_PUBKEY(t_data, NULL, NULL, t_passphrase);
			t_success = (t_key != NULL);
			break;
		case RSAKEY_PRIVKEY:
			t_key = PEM_read_bio_PrivateKey(t_data, NULL, NULL, t_passphrase);
			t_success = (t_key != NULL);
			break;
		case RSAKEY_CERT:
			{
				X509* t_cert = NULL;
				t_cert = PEM_read_bio_X509_AUX(t_data, NULL, NULL, t_passphrase);
				t_success = (t_cert != NULL);
				if (t_success)
				{
					t_key = X509_get_pubkey(t_cert);
					t_success = (t_key != NULL);
					X509_free(t_cert);
				}
			}
			break;
		default:
			// error: unknown key type
			t_success = false;
		}
	}

	if (t_data != NULL)
		BIO_free(t_data);
	if (t_success)
		r_key = t_key;

	return t_success;
}
/* WRAPPER */ bool MCCrypt_rsa_op(bool p_encrypt, bool p_is_public, MCStringRef p_message_in, MCStringRef p_key, MCStringRef p_passphrase, MCStringRef &r_message_out, MCStringRef &r_result, uint32_t &r_error)
{
	char *t_message_out;
	uint32_t t_message_out_length;
	char *t_result;
	bool t_success = MCCrypt_rsa_op(p_encrypt, p_is_public ? RSAKEY_PUBKEY : RSAKEY_PRIVKEY, MCStringGetCString(p_message_in), MCStringGetLength(p_message_in), 
									MCStringGetCString(p_key), MCStringGetLength(p_key), p_passphrase != nil ? MCStringGetCString(p_passphrase) : nil, 
									t_message_out, t_message_out_length, t_result, r_error);
	if (t_success)
		/* UNCHECKED */ MCStringCreateWithNativeCharsAndRelease((char_t *) t_message_out, t_message_out_length, r_message_out);
	else
		/* UNCHECKED */ MCStringCreateWithNativeCharsAndRelease((char_t *) t_result, strlen(t_result), r_result);
	return t_success;
}

bool MCCrypt_rsa_op(bool p_encrypt, RSA_KEYTYPE p_key_type, const char *p_message_in, uint32_t p_message_in_length,
			const char *p_key, uint32_t p_key_length, const char *p_passphrase,
			char *&r_message_out, uint32_t &r_message_out_length, char *&r_result, uint32_t &r_error)
{
	bool t_success = true;
	EVP_PKEY *t_key = NULL;
	RSA *t_rsa = NULL;
	int32_t t_rsa_size;
	uint8_t *t_output_buffer = NULL;
	int32_t t_output_length;

	if (!InitSSLCrypt())
	{
		t_success = false;
		MCCStringClone("error: ssl library initialization failed", r_result);
	}

	if (t_success)
	{
		if (!load_pem_key(p_key, p_key_length, p_key_type, p_passphrase, t_key))
		{
			t_success = false;
			MCCStringClone("error: invalid key", r_result);
		}
	}

	if (t_success)
	{
		t_rsa = EVP_PKEY_get1_RSA(t_key);
		if (t_rsa == NULL)
		{
			t_success = false;
			MCCStringClone("error: not an RSA key", r_result);
		}
	}

	if (t_success)
	{
		t_rsa_size = RSA_size(t_rsa);
		if (!MCMemoryAllocate(t_rsa_size, t_output_buffer))
		{
			t_success = false;
			r_error = EE_NO_MEMORY;
		}
	}
	int (*t_rsa_func)(int, const unsigned char*, unsigned char*, RSA*, int) = NULL;
	if (t_success)
	{
		if (p_encrypt)
		{
			if (p_key_type == RSAKEY_PRIVKEY)
				t_rsa_func = RSA_private_encrypt;
			else
				t_rsa_func = RSA_public_encrypt;
			if (p_message_in_length >= unsigned(t_rsa_size - 11))
			{
				t_success = false;
				MCCStringClone("error: message too large", r_result);
			}
		}
		else
		{
			if (p_key_type == RSAKEY_PRIVKEY)
				t_rsa_func = RSA_private_decrypt;
			else
				t_rsa_func = RSA_public_decrypt;
			if (p_message_in_length != t_rsa_size)
			{
				t_success = false;
				MCCStringClone("error: invalid message size", r_result);
			}
		}
	}
	if (t_success)
	{
		t_output_length = t_rsa_func(p_message_in_length, (const uint8_t*)p_message_in, t_output_buffer, t_rsa, RSA_PKCS1_PADDING);
		if (t_output_length < 0)
		{
			t_success = false;
			MCCStringClone("error: SSL operation failed", r_result);
		}
	}

	if (t_rsa != NULL)
		RSA_free(t_rsa);
	if (t_key != NULL)
		EVP_PKEY_free(t_key);

	if (t_success)
	{
		r_message_out = (char*)t_output_buffer;
		r_message_out_length = t_output_length;
	}
	else
	{
		uint32_t t_err;
		t_err = ERR_get_error();
		if (t_err)
		{
			const char *t_ssl_error = ERR_reason_error_string(t_err);
			MCCStringAppendFormat(r_result, " (SSL error: %s)", t_ssl_error);
		}
		MCMemoryDeallocate(t_output_buffer);
	}

	return t_success;
}

#else // !defined(MCSSL)
bool MCCrypt_rsa_op(bool p_encrypt, RSA_KEYTYPE p_key_type, const char *p_message_in, uint32_t p_message_in_length,
			const char *p_key, uint32_t p_key_length, const char *p_passphrase,
			char *&r_message_out, uint32_t &r_message_out_length, char *&r_result, uint32_t &r_error)
{
	return false;
}

bool MCCrypt_rsa_op(bool p_encrypt, bool p_is_public, MCStringRef p_message_in, MCStringRef p_key, MCStringRef p_passphrase, 
					MCStringRef &r_message_out, MCStringRef &r_result, uint32_t &r_error)
{
	return false;
}
#endif

/* WRAPPER */ bool SSL_encode(bool p_is_decrypt, MCNameRef p_cipher, MCStringRef p_data, MCStringRef p_key, bool p_is_password,
							  MCStringRef p_salt, MCStringRef p_iv, uint2 p_bit_rate, MCStringRef &r_output, MCStringRef &r_result, uint32_t &r_error)
{
	if (!InitSSLCrypt())
	{
		/* UNCHECKED */ MCStringCreateWithCString("ssl library not found", r_result);
		return false;
	}

#ifdef MCSSL
	uint4 t_outlen;
	char *t_ssl_encode;
	t_ssl_encode = SSL_encode(p_is_decrypt, MCNameGetCString(p_cipher), MCStringGetCString(p_data), MCStringGetLength(p_data), 
								t_outlen, MCStringGetCString(p_key), MCStringGetLength(p_key), p_is_password, p_bit_rate,
								p_salt != nil ? MCStringGetCString(p_salt) : nil, p_salt != nil ? MCStringGetLength(p_salt) : 0, 
								p_iv != nil ? MCStringGetCString(p_iv) : nil, p_iv != nil ? MCStringGetLength(p_iv) : 0);

	if (!t_ssl_encode)
	{
		if (t_outlen == 789)
			/* UNCHECKED */ MCStringCreateWithCString("invalid cipher name", r_result);
		else if (t_outlen == 790)
			/* UNCHECKED */ MCStringCreateWithCString("invalid keystring for specified keysize", r_result);
		else if (t_outlen == 791)
			r_error = EE_NO_MEMORY;
		else
		{
			uint32_t t_err;
			char *t_result;
			t_err = ERR_get_error();
			if (t_err)
			{
				const char *t_ssl_error = ERR_reason_error_string(t_err);
				MCCStringAppendFormat(t_result, " (SSL error: %s)", t_ssl_error);
				/* UNCHECKED */ MCStringCreateWithNativeCharsAndRelease((char_t *) t_result, strlen(t_result), r_result);
			}
		}
		return false;
	}
	else
	{
		/* UNCHECKED */ MCStringCreateWithNativeCharsAndRelease((char_t *)t_ssl_encode, t_outlen, r_output);
		return true;
	}
#else
	return false;
#endif
}

char *SSL_encode(Boolean isdecrypt, const char *ciphername,
                 const char *data, uint4 inlen, uint4 &outlen, //data to decrypt, length of that data, and pointer to descypted data length
                 const char *keystr, int4 keystrlen, Boolean ispassword, uint2 keylen,
                 const char *saltstr,  uint2 saltlen, const char *ivstr, uint2 ivlen)
{ //password or key, optional key length
	if (!InitSSLCrypt())
		return NULL;
#ifdef MCSSL

	// MW-2011-05-24: [[ Bug 9536 ]] The key length is a function of 'keylen' and for some ciphers
	//   can be larger than 32 bytes. For now, increase the buffer to 256 bytes given a maximum
	//   theoretical key length of 2048.
	uint1 iv[EVP_MAX_IV_LENGTH], key[256];
	if (keylen > 2048)
		return NULL;

	const EVP_CIPHER *cipher=EVP_get_cipherbyname(ciphername);

	uint1 operation = isdecrypt ? 0: 1;

	//get cipher object
	if (!cipher)
	{
		outlen = 789;
		return NULL;
	}

	static const char magic[]="Salted__";

	int4 res = 0;

	//set up cipher context
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init(&ctx);
	//init context with cipher and specify operation
	if (EVP_CipherInit(&ctx, cipher,NULL, NULL, operation) == 0)
		return NULL;

	//try setting keylength if specified. This will fail for some ciphers.
	if (keylen && EVP_CIPHER_CTX_set_key_length(&ctx, keylen/8) == 0)
		return NULL;
	//get new keylength in bytes
	int4 curkeylength = EVP_CIPHER_CTX_key_length(&ctx);
	//zero key and iv
	memset(key,0,EVP_MAX_KEY_LENGTH);
	memset(iv,0,EVP_MAX_IV_LENGTH);
	//if password combine with salt value to generate key

	unsigned char saltbuf[PKCS5_SALT_LEN];
	memset(saltbuf,0,sizeof(saltbuf));

	char *tend = (char *)data + inlen;

	// MW-2004-12-02: Fix bug 2411 - a NULL salt should result in a random one being
	//    generated, if saltstr is NULL and saltlen is zero then the salt should
	//    be taken as being the empty string.
	if (ispassword)
	{
		if (saltstr == NULL)
			RAND_bytes(saltbuf, sizeof(saltbuf));
		else
			memcpy(saltbuf,saltstr,MCU_min(saltlen,PKCS5_SALT_LEN));

		// MW-2004-12-02: We should only do this if we are decrypting
		if (isdecrypt && inlen > sizeof(magic) && memcmp(data,magic,sizeof(magic)-1) == 0)
		{
			data += sizeof(magic) - 1;

			if (saltstr == NULL || saltlen == 0)
				memcpy(saltbuf,data,sizeof(saltbuf));

			data += sizeof(saltbuf);
		}

		curkeylength =	EVP_BytesToKey(cipher,EVP_md5(),(const unsigned char *)saltbuf,(unsigned char *)keystr,
		                              keystrlen,1,key,iv);
	}
	else
	{//otherwise copy to key
		if (keystrlen != curkeylength)
		{ //sanity check then passed wrong size for key
			outlen = 790;
			return NULL;
		}
		else
			memcpy(key,keystr,curkeylength);
	}

	if (ivstr != NULL && ivlen > 0)
	{
		memset(iv,0,EVP_MAX_IV_LENGTH);
		memcpy(iv,ivstr,MCU_min(ivlen,EVP_MAX_IV_LENGTH));
	}

	if (EVP_CipherInit(&ctx, NULL, key, iv, operation) == 0)
		return NULL;
	int4 tmp, ol;
	ol = 0;

	//allocate memory to hold encrypted/decrypted data + an extra block + null terminator for block ciphers.
	unsigned char *outdata = (unsigned char *)malloc(inlen + EVP_CIPHER_CTX_block_size(&ctx) + 1 + sizeof(magic) + sizeof(saltbuf));
	//do encryption/decryption
	if (outdata == NULL)
	{
		outlen = 791;
		return NULL;
	}

	// MW-2004-12-02: Only prepend the salt if we generated the key (i.e. password mode)
	if (!isdecrypt && ispassword)
	{
		memcpy(&outdata[ol],magic,sizeof(magic)-1);
		ol += sizeof(magic)-1;
		memcpy(&outdata[ol],saltbuf,sizeof(saltbuf));
		ol += sizeof(saltbuf);
	}


	// MW-2007-02-13: [[Bug 4258]] - SSL now fails an assertion if datalen == 0
	if (tend - data > 0)
	{
		if (EVP_CipherUpdate(&ctx,&outdata[ol],&tmp,(unsigned char *)data,tend-data) == 0)
		{
			delete outdata;
			return NULL;
		}
		ol += tmp;
	}

	//for padding
	if (EVP_CipherFinal(&ctx,&outdata[ol],&tmp) == 0)
	{
		delete outdata;
		return NULL;
	}
	outlen = ol + tmp;

	//cleam up context and return data
	EVP_CIPHER_CTX_cleanup(&ctx);
	outdata[outlen] = 0; //null terminate data

	return (char *)outdata;
#else
	return NULL;
#endif
}

#ifdef MCSSL

typedef struct _ciphernames_context
{
	MCListRef list;
	bool success;
} ciphernames_context;

//list ciphers and default key lengths for each
void list_ciphers_cb(const OBJ_NAME *name,void *buffer)
{
	if(!islower((unsigned char)*name->name))
		return;

	ciphernames_context *t_context = (ciphernames_context*)buffer;
	if (!t_context->success)
		return;

	if (*name->name)
	{
		MCAutoStringRef t_string;
		const EVP_CIPHER *cipher=EVP_get_cipherbyname(name->name);
		if (cipher)
			t_context->success = MCStringFormat(&t_string, "%s,%d", name->name, EVP_CIPHER_key_length(cipher) * 8) &&
			MCListAppend(t_context->list, *t_string);
		else
			t_context->success = MCListAppendCString(t_context->list, name->name);
	}
}

bool SSL_ciphernames(MCListRef& r_list, MCStringRef& r_error)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	ciphernames_context t_context;
	t_context.list = *t_list;
	t_context.success = true;

	// MW-2004-12-29: Integrate Tuviah's fixes
	//static char sslcipherlist[] = "bf,128\nbf-cbc,128\nbf-cfb,128\nbf-ecb,128\nbf-ofb,128\nblowfish,128\ncast,128\ncast-cbc,128\ncast5-cbc,128\ncast5-cfb,128\ncast5-ecb,128\ncast5-ofb,128\ndes,64\ndes-cbc,64\ndes-cfb,64\ndes-ecb,64\ndes-ede,128\ndes-ede-cbc,128\ndes-ede-cfb,128\ndes-ede-ofb,128\ndes-ede3,192\ndes-ede3-cbc,192\ndes-ede3-cfb,192\ndes-ede3-ofb,192\ndes-ofb,64\ndes3,192\ndesx,192\ndesx-cbc,192\nrc2,128\nrc2-40-cbc,40\nrc2-64-cbc,64\nrc2-cbc,128\nrc2-cfb,128\nrc2-ecb,128\nrc2-ofb,128\nrc4,128\nrc4-40,40\nrc5,128\nrc5-cbc,128\nrc5-cfb,128\nrc5-ecb,128\nrc5-ofb,128";

	if (!InitSSLCrypt())
	{
		MCAutoStringRef sslerrbuf;
		SSLError(&sslerrbuf);
        r_error = MCValueRetain(*sslerrbuf);
	}

	OBJ_NAME_do_all_sorted(OBJ_NAME_TYPE_CIPHER_METH, list_ciphers_cb, &t_context);

	return t_context.success && MCListCopy(t_context.list, r_list);
}

#else
bool SSL_ciphernames(MCListRef& r_list, MCStringRef& r_error)
{
	r_list = MCValueRetain(kMCEmptyList);
	return true;
}
#endif
