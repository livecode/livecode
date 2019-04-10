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

#include "param.h"
#include "mcerror.h"

#include "util.h"
#include "object.h"
#include "mode.h"

#include "globals.h"
#include "system.h"
#include "osspec.h"

#include "license.h"

#ifdef MCSSL
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#endif

#ifdef _WIN32
#  include <WinCrypt.h>
#endif

#ifdef _MACOSX
#include "osxprefix.h"
#endif

#include "mcssl.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef _MACOSX
extern char *path2utf(char *path);
#endif

extern "C" int initialise_weak_link_crypto(void);
extern "C" int initialise_weak_link_ssl(void);
extern "C" void finalise_weak_link_crypto(void);
extern "C" void finalise_weak_link_ssl(void);

////////////////////////////////////////////////////////////////////////////////

static Boolean cryptinited = False;

#ifdef MCSSL
// IM-2014-07-28: [[ Bug 12822 ]] OS-specified root certificates
static STACK_OF(X509) *s_ssl_system_root_certs;
// IM-2014-07-28: [[ Bug 12822 ]] OS-specified CRLs
static STACK_OF(X509_CRL) *s_ssl_system_crls;
#endif

Boolean load_crypto_symbols()
{
#ifdef MCSSL

// MW-2009-07-21: Weakly link only on non-MacOSX platforms.
#if !defined(_SERVER)
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
        OPENSSL_init_ssl(0, NULL);
        
        uint32_t t_randomseed_bytes[4];
		RAND_seed(t_randomseed_bytes, sizeof(t_randomseed_bytes));
        MCrandomseed = t_randomseed_bytes[0];
		cryptinited = True;
		
		s_ssl_system_root_certs = nil;
		s_ssl_system_crls = nil;
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

//error buf should have a buffer of at least 256 bytes.
unsigned long SSLError(MCStringRef& errbuf)
{
	if (!InitSSLCrypt())
	{
        // SN-2015-07-02: [[ Bug 15568 ]] Create a StringRef - avoid over-release
        /* UNCHECKED */ MCStringCreateWithCString("ssl library not found", errbuf);
		return 0;
	}
#ifdef MCSSL
	unsigned long ecode = ERR_get_error();

    // SN-2015-07-02: [[ Bug 15568 ]] Mis-translation to StringRef from 6.7:
    //  errbuf won't be nil, but will always be empty though.
    if (ecode)
    {
        /* UNCHECKED */ MCAutoPointer<char[]> t_errbuf = new (nothrow) char[256];
        ERR_error_string_n(ecode,&t_errbuf,255);
        /* UNCHECKED */ MCStringCreateWithCString(*t_errbuf, errbuf);
    }
    else
        errbuf = MCValueRetain(kMCEmptyString);

	return ecode;
#else
    // SN-2015-07-02: [[ Bug 15568 ]] We don't let errbuf unset.
    errbuf = MCValueRetain(kMCEmptyString);
	return 0;
#endif
}

#ifdef MCSSL
bool load_pem_key(const char *p_data, uint32_t p_length, RSA_KEYTYPE p_type, const char *p_passphrase, RSA *&r_rsa)
{
	bool t_success = true;
	BIO *t_data = NULL;
	EVP_PKEY *t_key = NULL;
    RSA *t_rsa = NULL;
	t_data = BIO_new_mem_buf((void*)p_data, p_length);
	t_success = t_data != NULL;
	char t_empty_pass[] = "";
	char *t_passphrase = (p_passphrase != NULL) ? (char*)p_passphrase : t_empty_pass;

	if (t_success)
	{
		switch (p_type)
		{
		case RSAKEY_PUBKEY:
            {
                if (!PEM_read_bio_PUBKEY(t_data, &t_key, NULL, t_passphrase))
                {
                    // There is no way to reset the BIO so create a new one to check
                    // for a PKCS#1 format key
                    BIO *t_rsa_data = BIO_new_mem_buf((void*)p_data, p_length);
                    if (t_rsa_data != NULL)
                    {
                        t_success = PEM_read_bio_RSAPublicKey(t_rsa_data, &t_rsa, NULL, t_passphrase);
                        BIO_free(t_rsa_data);
                    }
                }
            }
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
	if (t_success && t_key != NULL)
    {
        if (t_rsa == NULL)
            t_rsa = EVP_PKEY_get1_RSA(t_key);
        
        t_success = t_rsa != NULL;
        
        EVP_PKEY_free(t_key);
    }
    
    if (t_success)
        r_rsa = t_rsa;

	return t_success;
}
/* WRAPPER */ bool MCCrypt_rsa_op(bool p_encrypt, bool p_is_public, MCStringRef p_message_in, MCStringRef p_key, MCStringRef p_passphrase, MCStringRef &r_message_out, MCStringRef &r_result, uint32_t &r_error)
{
	char *t_message_out;
	uint32_t t_message_out_length;
    char *t_result = nil;
	MCAutoPointer<char>t_message_in, t_key, t_passphrase;
    /* UNCHECKED */ MCStringConvertToCString(p_message_in, &t_message_in);
    /* UNCHECKED */ MCStringConvertToCString(p_key, &t_key);
    /* UNCHECKED */ MCStringConvertToCString(p_passphrase, &t_passphrase);
    
    bool t_success = MCCrypt_rsa_op(p_encrypt, p_is_public ? RSAKEY_PUBKEY : RSAKEY_PRIVKEY, *t_message_in, MCStringGetLength(p_message_in),*t_key, MCStringGetLength(p_key), p_passphrase != nil ? *t_passphrase : nil, t_message_out, t_message_out_length, t_result, r_error);
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
    if (!InitSSLCrypt())
    {
        MCCStringClone("error: ssl library initialization failed", r_result);
        return false;
    }
    
    bool t_success = true;
	RSA *t_rsa = NULL;
	int32_t t_rsa_size;
	uint8_t *t_output_buffer = NULL;
	int32_t t_output_length;

	if (t_success)
	{
		if (!load_pem_key(p_key, p_key_length, p_key_type, p_passphrase, t_rsa))
		{
			t_success = false;
			MCCStringClone("error: invalid key", r_result);
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

    MCAutoPointer<char>t_data, t_key, t_salt, t_iv, t_cipher;
    /* UNCHECKED */ MCStringConvertToCString(MCNameGetString(p_cipher), &t_cipher);
    /* UNCHECKED */ MCStringConvertToCString(p_data, &t_data);
    /* UNCHECKED */ MCStringConvertToCString(p_key, &t_key);

    if (p_salt != nil)
        /* UNCHECKED */ MCStringConvertToCString(p_salt, &t_salt);
    if (p_iv != nil)
        /* UNCHECKED */ MCStringConvertToCString(p_iv, &t_iv);
    
    t_ssl_encode = SSL_encode(p_is_decrypt, *t_cipher, *t_data, MCStringGetLength(p_data), t_outlen, *t_key, MCStringGetLength(p_key), p_is_password, p_bit_rate, p_salt != nil ? *t_salt : nil, p_salt != nil ? MCStringGetLength(p_salt) : 0, p_iv != nil ? *t_iv : nil, p_iv != nil ? MCStringGetLength(p_iv) : 0);

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
            char *t_result = nil;
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

	//set up cipher context
    MCAutoCustomPointer<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free> ctx = EVP_CIPHER_CTX_new();
	EVP_CIPHER_CTX_reset(*ctx);
	
	//init context with cipher and specify operation
	if (EVP_CipherInit(*ctx, cipher,NULL, NULL, operation) == 0)
		return NULL;

	//try setting keylength if specified. This will fail for some ciphers.
	if (keylen && EVP_CIPHER_CTX_set_key_length(*ctx, keylen/8) == 0)
		return NULL;
	//get new keylength in bytes
	int4 curkeylength = EVP_CIPHER_CTX_key_length(*ctx);
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

	
	// Re-initialise the cipher
	EVP_CIPHER_CTX_reset(*ctx);
	if (EVP_CipherInit(*ctx, cipher, key, iv, operation) == 0)
		return NULL;
	if (keylen && EVP_CIPHER_CTX_set_key_length(*ctx, keylen/8) == 0)
		return NULL;
	
	int4 tmp, ol;
	ol = 0;

	//allocate memory to hold encrypted/decrypted data + an extra block + null terminator for block ciphers.
	unsigned char *outdata = nil;
	if (!MCMemoryAllocate(inlen + EVP_CIPHER_CTX_block_size(*ctx) + 1 + sizeof(magic) + sizeof(saltbuf), outdata))
	{
		outlen = 791;
		return NULL;
	}
	//do encryption/decryption

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
		if (EVP_CipherUpdate(*ctx, &outdata[ol], &tmp, (unsigned char *)data, tend-data) == 0)
		{
			MCMemoryDeallocate(outdata);
			return NULL;
		}
		ol += tmp;
	}

	//for padding
	if (EVP_CipherFinal(*ctx, &outdata[ol], &tmp) == 0)
	{
		MCMemoryDeallocate(outdata);
		return NULL;
	}
	outlen = ol + tmp;

	//cleam up context and return data
	EVP_CIPHER_CTX_reset(*ctx);
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
	if (!InitSSLCrypt())
	{
		MCAutoStringRef sslerrbuf;
		SSLError(&sslerrbuf);
        r_error = MCValueRetain(*sslerrbuf);
	}
	else
	{
		OBJ_NAME_do_all_sorted(OBJ_NAME_TYPE_CIPHER_METH, list_ciphers_cb, &t_context);
	}

	return t_context.success && MCListCopy(t_context.list, r_list);
}

#else
bool SSL_ciphernames(MCListRef& r_list, MCStringRef& r_error)
{
	r_list = MCValueRetain(kMCEmptyList);
	return true;
}
#endif

bool SSL_random_bytes(const void *p_buffer, uindex_t p_count)
{
#ifdef MCSSL
	return RAND_bytes((unsigned char *)p_buffer, p_count) == 1;
#endif
	return false;
}

// MM-2014-02-14: [[ LibOpenSSL 1.0.1e ]] Initialise the openlSSL module.
void InitialiseSSL(void)
{
#ifdef MCSSL
	cryptinited = False;
#endif
}

void ShutdownSSL()
{
#ifdef MCSSL
	if (cryptinited)
	{
#if !defined(_SERVER)
		finalise_weak_link_crypto();
		finalise_weak_link_ssl();
#endif
		cryptinited = False;
	}
#endif
}

void shutdown()
{}

////////////////////////////////////////////////////////////////////////////////
// IM-2014-07-28: [[ Bug 12822 ]] Common certificate loading code refactored from opensslsocket.cpp

#ifdef MCSSL
bool load_ssl_ctx_certs_from_folder(SSL_CTX *p_ssl_ctx, const char *p_path);
bool load_ssl_ctx_certs_from_file(SSL_CTX *p_ssl_ctx, const char *p_path);
bool ssl_set_default_certificates(SSL_CTX *p_ssl_ctx);
#endif

bool MCSSLContextLoadCertificates(SSL_CTX *p_ssl_ctx, MCStringRef *r_error)
{
#ifdef MCSSL
	bool t_success;
	t_success = true;
	
	if (MCsslcertificates && MCCStringLength(MCsslcertificates) > 0)
	{
		MCString *certs = NULL;
		uint2 ncerts = 0;

		/* UNCHECKED */ MCU_break_string(MCsslcertificates, certs, ncerts);
		if (t_success && ncerts > 0)
		{
			uint2 i;
			for (i = 0; t_success && i < ncerts; i++)
			{
                MCAutoStringRef t_oldcertpath;
                if (t_success)
                    t_success = MCStringCreateWithOldString(certs[i], &t_oldcertpath);
				
                MCAutoStringRef t_certpath;
                if (t_success)
                    t_success = MCS_resolvepath(*t_oldcertpath, &t_certpath);
                
                MCAutoStringRefAsUTF8String t_certpath_utf8;
                if (t_success)
                    t_success = t_certpath_utf8.Lock(*t_certpath);
                
                if (t_success)
                    t_success = (MCS_exists(*t_certpath, True) && load_ssl_ctx_certs_from_file(p_ssl_ctx, *t_certpath_utf8)) ||
                    (MCS_exists(*t_certpath, False) && load_ssl_ctx_certs_from_folder(p_ssl_ctx, *t_certpath_utf8));
				
				if (!t_success)
				{
					if (r_error != nil)
						MCStringFormat(*r_error, "Error loading CA file and/or directory %@", *t_certpath);
				}
			}
		}
		if (certs != NULL)
			delete certs;
	}
	else
	{
		if (!ssl_set_default_certificates(p_ssl_ctx))
		{
			if (r_error != nil)
				*r_error = MCSTR("Error loading default CAs");
			
			t_success = false;
		}
	}
	
	return t_success;
#endif
    return false;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef MCSSL
struct cert_folder_load_context_t
{
	const char *path;
	SSL_CTX *ssl_context;
};

bool cert_dir_list_callback(void *context, const MCSystemFolderEntry *p_entry)
{
	bool t_success = true;
	cert_folder_load_context_t *t_context = (cert_folder_load_context_t*)context;
	
	if (!p_entry -> is_folder && MCStringEndsWith(p_entry -> name, MCSTR(".pem"), kMCCompareCaseless))
	{
		MCAutoStringRef t_file_path;
		t_success = MCStringFormat(&t_file_path, "%s/%@", t_context->path, p_entry -> name);
        
        MCAutoStringRefAsUTF8String t_certpath_utf8;
        if (t_success)
            t_success = t_certpath_utf8.Lock(*t_file_path);
        
		if (t_success)
			t_success = load_ssl_ctx_certs_from_file(t_context->ssl_context, *t_certpath_utf8);
	}
	return t_success;
}

bool load_ssl_ctx_certs_from_folder(SSL_CTX *p_ssl_ctx, const char *p_path)
{
	bool t_success = true;
	
	cert_folder_load_context_t t_context;
	t_context.path = p_path;
	
	t_context.ssl_context = p_ssl_ctx;
	
	t_success = MCsystem -> ListFolderEntries(nil, (MCSystemListFolderEntriesCallback)cert_dir_list_callback, &t_context);
	
	return t_success;
}

bool load_ssl_ctx_certs_from_file(SSL_CTX *p_ssl_ctx, const char *p_path)
{
	return SSL_CTX_load_verify_locations(p_ssl_ctx, p_path, NULL) != 0;
}

#if defined(TARGET_PLATFORM_MACOS_X) || defined(_WIN32)

void free_x509_stack(STACK_OF(X509) *p_stack)
{
	if (p_stack != NULL)
	{
		while (sk_X509_num(p_stack) > 0)
		{
			X509 *t_x509 = sk_X509_pop(p_stack);
			X509_free(t_x509);
		}
		sk_X509_free(p_stack);
	}
}

void free_x509_crl_stack(STACK_OF(X509_CRL) *p_stack)
{
	if (p_stack != NULL)
	{
		while (sk_X509_CRL_num(p_stack) > 0)
		{
			X509_CRL *t_crl = sk_X509_CRL_pop(p_stack);
			X509_CRL_free(t_crl);
		}
		sk_X509_CRL_free(p_stack);
	}
}

bool ssl_ctx_add_cert_stack(SSL_CTX *p_ssl_ctx, STACK_OF(X509) *p_cert_stack, STACK_OF(X509_CRL) *p_crl_stack)
{
	bool t_success = true;
	
	X509_STORE *t_cert_store = NULL;
	
	t_success = NULL != (t_cert_store = SSL_CTX_get_cert_store(p_ssl_ctx));
	
	if (t_success && p_cert_stack != NULL)
	{
		for (int32_t i = 0; t_success && i < sk_X509_num(p_cert_stack); i++)
		{
			X509 *t_x509 = sk_X509_value(p_cert_stack, i);
			if (0 == X509_STORE_add_cert(t_cert_store, t_x509))
			{
				if (ERR_GET_REASON(ERR_get_error()) != X509_R_CERT_ALREADY_IN_HASH_TABLE)
					t_success = false;
			}
		}
	}
	
	if (t_success && p_crl_stack != NULL)
	{
		for (int32_t i = 0; t_success && i < sk_X509_CRL_num(p_crl_stack); i++)
		{
			X509_CRL *t_crl = sk_X509_CRL_value(p_crl_stack, i);
			if (0 == X509_STORE_add_crl(t_cert_store, t_crl))
			{
				t_success = false;
			}
		}
	}
	return t_success;
}

bool export_system_root_cert_stack(STACK_OF(X509) *&r_x509_stack);
bool export_system_crl_stack(STACK_OF(X509_CRL) *&r_crl_stack);

bool ssl_set_default_certificates(SSL_CTX *p_ssl_ctx)
{
	bool t_success;
	t_success = true;
	
	if (s_ssl_system_root_certs == nil)
		t_success = export_system_root_cert_stack(s_ssl_system_root_certs);
	
	if (t_success && s_ssl_system_crls == nil)
		t_success = export_system_crl_stack(s_ssl_system_crls);
		
	if (t_success)
		t_success = ssl_ctx_add_cert_stack(p_ssl_ctx, s_ssl_system_root_certs, s_ssl_system_crls);
		
	return t_success;
}

#else

static const char *s_ssl_bundle_paths[] = {
	"/etc/ssl/certs/ca-bundle.crt",
	"/etc/ssl/certs/ca-certificates.crt",
	"/etc/pki/tls/certs/ca-bundle.crt",
};

static const char *s_ssl_hash_dir_paths[] = {
	"/etc/ssl/certs",
	"/etc/pki/tls/certs",
};

bool ssl_set_default_certificates(SSL_CTX *p_ssl_ctx)
{
	bool t_success = true;
	bool t_found = false;
	uint32_t t_path_count = 0;
	
	t_path_count = sizeof(s_ssl_bundle_paths) / sizeof(const char*);
	for (uint32_t i = 0; t_success && !t_found && i < t_path_count; i++)
	{
		if (MCS_exists(MCSTR(s_ssl_bundle_paths[i]), true))
		{
			t_success = load_ssl_ctx_certs_from_file(p_ssl_ctx, s_ssl_bundle_paths[i]);
			if (t_success)
				t_found = true;
		}
	}
	
	t_path_count = sizeof(s_ssl_hash_dir_paths) / sizeof(const char*);
	for (uint32_t i = 0; t_success && !t_found && i < t_path_count; i++)
	{
		if (MCS_exists(MCSTR(s_ssl_hash_dir_paths[i]), false))
		{
			t_success = load_ssl_ctx_certs_from_folder(p_ssl_ctx, s_ssl_bundle_paths[i]);
			if (t_success)
				t_found = true;
		}
	}
	
	return t_success;
}

#endif

#ifdef TARGET_PLATFORM_MACOS_X
bool export_system_root_cert_stack(STACK_OF(X509) *&r_x509_stack)
{
	bool t_success = true;
	
	CFArrayRef t_anchors = NULL;
	STACK_OF(X509) *t_stack = NULL;
	
	t_success = noErr == SecTrustCopyAnchorCertificates(&t_anchors);
	
	t_stack = sk_X509_new(NULL);
	if (t_success)
	{
		UInt32 t_anchor_count = CFArrayGetCount(t_anchors);
		for (UInt32 i = 0; t_success && i < t_anchor_count; i++)
		{
			X509 *t_x509 = NULL;
			const unsigned char* t_data_ptr = NULL;
			UInt32 t_data_len = 0;
			
			CSSM_DATA t_cert_data;
			t_success = noErr == SecCertificateGetData((SecCertificateRef)CFArrayGetValueAtIndex(t_anchors, i), &t_cert_data);
			
			if (t_success)
			{
				t_data_ptr = t_cert_data.Data;
				t_data_len = t_cert_data.Length;
				t_success = NULL != (t_x509 = d2i_X509(NULL, &t_data_ptr, t_data_len));
			}
			if (t_success)
				t_success = 0 != sk_X509_push(t_stack, t_x509);
		}
	}
	
	if (t_anchors != NULL)
		CFRelease(t_anchors);
	
	if (t_success)
		r_x509_stack = t_stack;
	else if (t_stack != NULL)
		free_x509_stack(t_stack);

	return t_success;
}

bool export_system_crl_stack(STACK_OF(X509_CRL) *&r_crls)
{
	r_crls = NULL;
	return true;
}

#elif defined(_WIN32)

bool export_system_root_cert_stack(STACK_OF(X509) *&r_cert_stack)
{
	bool t_success = true;

	STACK_OF(X509) *t_cert_stack = NULL;
	HCERTSTORE t_cert_store = NULL;
	PCCERT_CONTEXT t_cert_enum = NULL;

	t_success = NULL != (t_cert_stack = sk_X509_new(NULL));

	if (t_success)
		t_success = NULL != (t_cert_store = CertOpenSystemStoreW(NULL, L"ROOT"));

	while (t_success && NULL != (t_cert_enum = CertEnumCertificatesInStore(t_cert_store, t_cert_enum)))
	{
		bool t_valid = true;
		if (CertVerifyTimeValidity(NULL, t_cert_enum->pCertInfo))
			t_valid = false;
		if (t_valid)
		{
			X509 *t_x509 = NULL;
			const unsigned char *t_data = (const unsigned char*) t_cert_enum->pbCertEncoded;
			long t_len = t_cert_enum->cbCertEncoded;

			t_success = NULL != (t_x509 = d2i_X509(NULL, &t_data, t_len));

			if (t_success)
				t_success = 0 != sk_X509_push(t_cert_stack, t_x509);
		}
	}

	if (t_cert_store != NULL)
		CertCloseStore(t_cert_store, 0);

	if (t_success)
		r_cert_stack = t_cert_stack;
	else
		free_x509_stack(t_cert_stack);

	return t_success;
}

bool export_system_crl_stack(STACK_OF(X509_CRL) *&r_crls)
{
	bool t_success = true;

	STACK_OF(X509_CRL) *t_crl_stack = NULL;
	HCERTSTORE t_cert_store = NULL;
	PCCRL_CONTEXT t_crl_enum = NULL;

	t_success = NULL != (t_crl_stack = sk_X509_CRL_new(NULL));

	if (t_success)
		t_success = NULL != (t_cert_store = CertOpenSystemStoreW(NULL, L"ROOT"));

	while (t_success && NULL != (t_crl_enum = CertEnumCRLsInStore(t_cert_store, t_crl_enum)))
	{
		bool t_valid = true;
		if (CertVerifyCRLTimeValidity(NULL, t_crl_enum->pCrlInfo))
			t_valid = false;
		if (t_valid)
		{
			X509_CRL *t_crl = NULL;
			const unsigned char *t_data = (const unsigned char*)t_crl_enum->pbCrlEncoded;
			long t_len = t_crl_enum->cbCrlEncoded;

			t_success = NULL != (t_crl = d2i_X509_CRL(NULL, &t_data, t_len));

			if (t_success)
				t_success = 0 != sk_X509_CRL_push(t_crl_stack, t_crl);
		}
	}

	if (t_cert_store != NULL)
		CertCloseStore(t_cert_store, 0);

	if (t_success)
		r_crls = t_crl_stack;
	else
		free_x509_crl_stack(t_crl_stack);

	return t_success;
}

#else

bool export_system_root_cert_stack(STACK_OF(X509) *&r_cert_stack)
{
	r_cert_stack = NULL;
	return true;
}

bool export_system_crl_stack(STACK_OF(X509_CRL) *&r_crls)
{
	r_crls = NULL;
	return true;
}

#endif

#endif /* MCSSL */


////////////////////////////////////////////////////////////////////////////////

#if defined(TARGET_SUBPLATFORM_IPHONE)

#ifdef MCSSL

#include <Security/Security.h>
static SecCertificateRef x509_to_SecCertificateRef(X509 *p_cert)
{
    bool t_success;
    t_success = true;
    
    size_t t_cert_size;
    byte_t *t_cert_data;
    t_cert_data = NULL;
    if (t_success)
    {
        t_cert_size = i2d_X509(p_cert, &t_cert_data);
        t_success = t_cert_size > 0;
    }
    
    CFDataRef t_cf_cert_data;
    t_cf_cert_data = NULL;
    if (t_success)
    {
        t_cf_cert_data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, t_cert_data, t_cert_size, kCFAllocatorNull);
        t_success = t_cf_cert_data != NULL;
    }
    
    SecCertificateRef t_sec_cert;
    t_sec_cert = NULL;
    if (t_success)
    {
        t_sec_cert = SecCertificateCreateWithData(kCFAllocatorDefault, t_cf_cert_data);
        t_success = t_sec_cert != NULL;
    }
    
    if (t_cert_data != NULL)
        MCMemoryDeallocate(t_cert_data);
    if (t_cf_cert_data != NULL)
        CFRelease(t_cf_cert_data);

    if (t_success)
        return t_sec_cert;
    else
        return NULL;
}
#endif /* MCSSL */

// MM-2015-06-04: [[ MobileSockets ]] Return true if we should trust the
//   certificates in the given SSL connection, false otherwise.
//
//   iOS doesn't directly expose the system's root certificates, meaning we
//   cannot pass those certificates on to OpenSSL and use its verification
//   routines.
//
//   Instead we must verify each certificate using the iOS APIs which allows us
//   access to the root certificates indirectly.
//
bool MCSSLVerifyCertificate(SSL *ssl, MCStringRef p_host_name, MCStringRef &r_error)
{
#ifdef MCSSL
    bool t_success;
    t_success = true;
    
    MCStringRef t_error;
    t_error = NULL;
    
    STACK_OF(X509) *t_cert_stack;
    t_cert_stack = NULL;
    if (t_success)
    {
        t_cert_stack = SSL_get_peer_cert_chain(ssl);
        t_success = t_cert_stack != NULL;
    }

    CFMutableArrayRef t_certs;
    t_certs = NULL;
    if (t_success)
    {
        t_certs = CFArrayCreateMutable(kCFAllocatorDefault, sk_X509_num(t_cert_stack), NULL);
        t_success = t_certs != NULL;
    }
    
    if (t_success)
    {
        for (uint32_t i = 0; i < sk_X509_num(t_cert_stack) && t_success; i++)
        {
            X509 *t_cert;
            t_cert = NULL;
            if (t_success)
            {
                t_cert = sk_X509_value(t_cert_stack, i);
                t_success = t_cert != NULL;
            }
            
            SecCertificateRef t_sec_cert;
            t_sec_cert = NULL;
            if (t_success)
            {
                t_sec_cert = x509_to_SecCertificateRef(t_cert);
                t_success = t_sec_cert != NULL;
            }
            
            if (t_success)
                CFArrayAppendValue(t_certs, t_sec_cert);
        }
    }
    
    CFStringRef t_host_name;
    t_host_name = NULL;
    if (t_success)
        t_success = MCStringConvertToCFStringRef(p_host_name, t_host_name);
    
    SecPolicyRef t_policies;
    t_policies = NULL;
    if (t_success)
    {
        t_policies = SecPolicyCreateSSL(true, t_host_name);
        t_success = t_policies != NULL;
    }
    
    SecTrustRef t_trust;
    t_trust = NULL;
    if (t_success)
        t_success = SecTrustCreateWithCertificates(t_certs, t_policies, &t_trust) == noErr;
    
    SecTrustResultType t_verification_result;
    if (t_success)
        t_success = SecTrustEvaluate(t_trust, &t_verification_result) == noErr;
    
    if (t_success)
    {
        switch (t_verification_result)
        {
            case kSecTrustResultUnspecified:
            case kSecTrustResultProceed:
                break;
            case kSecTrustResultDeny:
                t_error = MCSTR("The user has chosen not to trust this certificate");
                t_success = false;
                break;
            case kSecTrustResultRecoverableTrustFailure:
            case kSecTrustResultFatalTrustFailure:
                t_error = MCSTR("A certificate in the chain is defective");
                t_success = false;
                break;
            default:
                t_success = false;
                break;
        }
    }
    
    MCStringRef t_formatted_error;
    t_formatted_error = NULL;
    if (!t_success)
    {
        /* UNCHECKED */ MCStringCreateMutable(0, t_formatted_error);
        /* UNCHECKED */ MCStringAppendFormat(t_formatted_error, "-Error with certificate: \n");
        
        // We don't know the exact certificate in the chain which caused the verification to fail so just report the details of the first.
        // Also, since we can't use the iOS APIs to get the cert info, use OpenSSL routines instead.
        X509 *t_cert;
        t_cert = NULL;
        if (t_cert_stack != NULL)
            t_cert = sk_X509_value(t_cert_stack, 0);
        
        if (t_cert != NULL)
        {
            char t_cstring_error[256];
            
            X509_NAME_oneline(X509_get_issuer_name(t_cert), t_cstring_error, 256);
            /* UNCHECKED */ MCStringAppendFormat(t_formatted_error, "  issuer   = %s\n", t_cstring_error);
            
            X509_NAME_oneline(X509_get_subject_name(t_cert), t_cstring_error, 256);
            /* UNCHECKED */ MCStringAppendFormat(t_formatted_error, "  subject  = %s\n", t_cstring_error);
        }
        
        if (t_error != NULL)
            /* UNCHECKED */ MCStringAppendFormat(t_formatted_error, "  err: %@\n", t_error);
    }
    
    if (t_error != NULL)
        MCValueRelease(t_error);
    if (t_certs != NULL)
    {
        for (uint32_t i = 0; i < CFArrayGetCount(t_certs); i++)
        {
            CFTypeRef t_value;
            t_value = CFArrayGetValueAtIndex(t_certs, i);
            if (t_value != NULL)
                CFRelease(t_value);
        }
        CFRelease(t_certs);
    }
    if (t_host_name != NULL)
        CFRelease(t_host_name);
    if (t_policies != NULL)
        CFRelease(t_policies);
    if (t_trust != NULL)
        CFRelease(t_trust);
    
    if (t_formatted_error != NULL)
        /* UNCHECKED */ MCStringCopyAndRelease(t_formatted_error, r_error);
    
    return t_success;
#else
    return false;
#endif /* MCSSL */
}

#endif /* TARGET_SUBPLATFORM_IPHONE */

#if defined(TARGET_SUBPLATFORM_ANDROID)

#include "mblandroidutil.h"

// MM-2015-06-11: [[ MobileSockets ]] Return true if we should trust the
//   certificates in the given SSL connection, false otherwise.
//
// Similar to iOS, on Android we must use the OS routines to verify SSL certs.
// Extract the certs from the connection and check they can be trusted.
// Most of the work here is done on the Java side of things.
//
bool MCSSLVerifyCertificate(SSL *ssl, MCStringRef p_host_name, MCStringRef &r_error)
{
#ifdef MCSSL
    bool t_success;
    t_success = true;
    
    STACK_OF(X509) *t_cert_stack;
    t_cert_stack = NULL;
    if (t_success)
    {
        t_cert_stack = SSL_get_peer_cert_chain(ssl);
        t_success = t_cert_stack != NULL;
    }

    MCAutoArrayRef t_cert_array;
    if (t_success)
        t_success = MCArrayCreateMutable(&t_cert_array);
    
    if (t_success)
    {
        for (uint32_t i = 0; i < sk_X509_num(t_cert_stack) && t_success; i++)
        {
            X509 *t_cert;
            t_cert = NULL;
            if (t_success)
            {
                t_cert = sk_X509_value(t_cert_stack, i);
                t_success = t_cert != NULL;
            }
            
            uindex_t t_cert_size;
            byte_t *t_cert_data;
            t_cert_data = NULL;
            if (t_success)
            {
                t_cert_size = i2d_X509(t_cert, &t_cert_data);
                t_success = t_cert_size > 0;
            }
            
            MCAutoDataRef t_cert_data_ref;
            if (t_success)
                t_success = MCDataCreateWithBytesAndRelease(t_cert_data, t_cert_size, &t_cert_data_ref);
            
            if (t_success)
                t_success = MCArrayStoreValueAtIndex(*t_cert_array, i + 1, *t_cert_data_ref);
        }
    }

    if (t_success)
    {
        MCAndroidEngineCall("verifyCertificateChainIsTrusted", "b@@", &t_success, *t_cert_array, p_host_name);
        if (!t_success)
            MCAndroidEngineCall("getLastCertificateVerificationError", "x", &r_error);
    }
    
    return t_success;
#else
    return false;
#endif /* MCSSL */
}

#endif /* TARGET_SUBPLATFORM_ANDROID */

////////////////////////////////////////////////////////////////////////////////
