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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"


#include "exec.h"
#include "handler.h"
#include "scriptpt.h"
#include "variable.h"
#include "statemnt.h"
#include "globals.h"
#include "util.h"
#include "param.h"
#include "object.h"
#include "deploy.h"
#include "osspec.h"

#include <openssl/err.h>
#include <openssl/objects.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pkcs7.h>
#include <openssl/asn1t.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>

////////////////////////////////////////////////////////////////////////////////

// At first glance, digital signing is quite scary in terms of implementation;
// however, once one gets over the hurdle of nomenclature, it is actually quite
// straight-forward. Indeed, it is made even more straight-forward by the fact
// that OpenSSL pretty much does it all for us...
//
// The nomenclature is actually pretty horrendous but here goes a simplified
// view...
//
// PKCS stands for 'Public Key Cryptography Standards'. It is a collection of
// specifications devised and published by RSA Security. The ones of critical
// interest to us are:
//   - PKCS#7: this contains the definition of the digital signature
//       structure(s)
//   - PKCS#8: this contains the definition private key structure(s).
//   - PKCS#12: this contains the definitions of a container to store a
//       certificate and private key together.
//
// Now, all the data structures needed by crypto related activities are
// defined using an abstract notation called ASN.1 - this is really just a
// way of describing hierarchical data structures in text. Indeed, ASN.1 itself
// does nothing for us, other then to tell us what order/type/kind of fields
// are present in any of the data structures defined in the PKCS standards.
//
// However, ASN.1 becomes useful when an encoding format is applied to it.
// There are a number of these, but the primary one in use for the signing
// activities is DER. The DER format is just a way of encoding an ASN.1
// described structure as a byte stream.
//
// The key element in the code-signing process is the X.509 v3 certificate that
// is issued by a Certificate Authority. Again, this object is defined as
// an ASN.1 object. However, it should be noted that 'in the wild' such
// certificates will generally be stored using a PKCS#7 SignedData structure
// with no actual content.
//
// Thus, to sum up - the various PKCS specifications define all the required
// data structures using ASN.1. These data structures are read an written
// using an encoding format called DER.
//
// The good news is now that we have these basic ideas, is that we can
// forget about most of it - OpenSSL handles all the ASN.1/DER/PKCS stuff
// for us... This is assuming it is possible to understand the rather cryptic
// (and undocumented!) OpenSSL functions that do all this for us!

////////////////////////////////////////////////////////////////////////////////

// This section contains all the Authenticode related definitions and ASN.1
// structure types.

// These OID values are taken from <wintrust.h>
#define SPC_TIME_STAMP_REQUEST_OBJID        "1.3.6.1.4.1.311.3.2.1"
#define SPC_INDIRECT_DATA_OBJID             "1.3.6.1.4.1.311.2.1.4"
#define SPC_SP_AGENCY_INFO_OBJID            "1.3.6.1.4.1.311.2.1.10"
#define SPC_STATEMENT_TYPE_OBJID            "1.3.6.1.4.1.311.2.1.11"
#define SPC_SP_OPUS_INFO_OBJID              "1.3.6.1.4.1.311.2.1.12"
#define SPC_CERT_EXTENSIONS_OBJID           "1.3.6.1.4.1.311.2.1.14"
#define SPC_PE_IMAGE_DATA_OBJID             "1.3.6.1.4.1.311.2.1.15"
#define SPC_RAW_FILE_DATA_OBJID             "1.3.6.1.4.1.311.2.1.18"
#define SPC_STRUCTURED_STORAGE_DATA_OBJID   "1.3.6.1.4.1.311.2.1.19"
#define SPC_JAVA_CLASS_DATA_OBJID           "1.3.6.1.4.1.311.2.1.20"
#define SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID "1.3.6.1.4.1.311.2.1.21"
#define SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID "1.3.6.1.4.1.311.2.1.22"
#define SPC_CAB_DATA_OBJID                  "1.3.6.1.4.1.311.2.1.25"
#define SPC_GLUE_RDN_OBJID                  "1.3.6.1.4.1.311.2.1.25"    // obsolete!
#define SPC_MINIMAL_CRITERIA_OBJID          "1.3.6.1.4.1.311.2.1.26"
#define SPC_FINANCIAL_CRITERIA_OBJID        "1.3.6.1.4.1.311.2.1.27"
#define SPC_LINK_OBJID                      "1.3.6.1.4.1.311.2.1.28"
#define SPC_SIGINFO_OBJID                   "1.3.6.1.4.1.311.2.1.30"

//////////

/* AlgorithmIdentifier    ::=    SEQUENCE {
       algorithm           ObjectID,
       parameters          [0] EXPLICIT ANY OPTIONAL
   } */

struct AlgorithmIdentifier
{
	ASN1_OBJECT *algorithm;
	ASN1_NULL *parameters;
};

DECLARE_ASN1_FUNCTIONS(AlgorithmIdentifier)
ASN1_SEQUENCE(AlgorithmIdentifier) = {
	ASN1_SIMPLE(AlgorithmIdentifier, algorithm, ASN1_OBJECT),
	ASN1_SIMPLE(AlgorithmIdentifier, parameters, ASN1_NULL)
} ASN1_SEQUENCE_END(AlgorithmIdentifier)
IMPLEMENT_ASN1_FUNCTIONS(AlgorithmIdentifier)

//////////

/* DigestInfo ::= SEQUENCE {
       digestAlgorithm     AlgorithmIdentifier,
       digest              OCTETSTRING
   } */

struct DigestInfo
{
	AlgorithmIdentifier *algorithm;
	ASN1_OCTET_STRING *digest;
};

DECLARE_ASN1_FUNCTIONS(DigestInfo)
ASN1_SEQUENCE(DigestInfo) = {
	ASN1_SIMPLE(DigestInfo, algorithm, AlgorithmIdentifier),
	ASN1_SIMPLE(DigestInfo, digest, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(DigestInfo)
IMPLEMENT_ASN1_FUNCTIONS(DigestInfo)

//////////

/* SpcString ::= CHOICE {
    unicode                 [0] IMPLICIT BMPSTRING,
    ascii                   [1] IMPLICIT IA5STRING
} */

struct SpcString
{
	int type;
	union
	{
		ASN1_STRING *variant;
		ASN1_BMPSTRING *unicode;
		ASN1_IA5STRING *ascii;
	} d;
};

DECLARE_ASN1_FUNCTIONS(SpcString)
ASN1_CHOICE(SpcString) = {
	ASN1_IMP_OPT(SpcString, d.unicode, ASN1_BMPSTRING, 0),
	ASN1_IMP_OPT(SpcString, d.ascii, ASN1_IA5STRING, 1)
} ASN1_CHOICE_END(SpcString)
IMPLEMENT_ASN1_FUNCTIONS(SpcString)

//////////

/* SpcSerializedObject ::= SEQUENCE {
    classId             SpcUuid,
    serializedData      OCTETSTRING
}

SpcUuid ::= OCTETSTRING */

struct SpcSerializedObject
{
	ASN1_OCTET_STRING *class_id;
	ASN1_OCTET_STRING *data;
};

DECLARE_ASN1_FUNCTIONS(SpcSerializedObject)
ASN1_SEQUENCE(SpcSerializedObject) = {
	ASN1_SIMPLE(SpcSerializedObject, class_id, ASN1_OCTET_STRING),
	ASN1_SIMPLE(SpcSerializedObject, data, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(SpcSerializedObject)
IMPLEMENT_ASN1_FUNCTIONS(SpcSerializedObject)

//////////

/* SpcLink ::= CHOICE {
    url                     [0] IMPLICIT IA5STRING,
    moniker                 [1] IMPLICIT SpcSerializedObject,
    file                    [2] EXPLICIT SpcString
} */

struct SpcLink
{
	int type;
	union
	{
		ASN1_IA5STRING *url;
		SpcSerializedObject *moniker;
		SpcString *file;
	} d;
};

DECLARE_ASN1_FUNCTIONS(SpcLink)
ASN1_CHOICE(SpcLink) = {
	ASN1_IMP_OPT(SpcLink, d.url, ASN1_IA5STRING, 0),
	ASN1_IMP_OPT(SpcLink, d.moniker, SpcSerializedObject, 1),
	ASN1_EXP_OPT(SpcLink, d.file, SpcString, 2)
} ASN1_CHOICE_END(SpcLink)
IMPLEMENT_ASN1_FUNCTIONS(SpcLink)

//////////

/* SpcSpOpusInfo ::= SEQUENCE {
       programName             [0] EXPLICIT SpcString OPTIONAL,
       moreInfo                [1] EXPLICIT SpcLink OPTIONAL,
   } */

struct SpcSpOpusInfo
{
	SpcString *description;
	SpcLink *url;
};

DECLARE_ASN1_FUNCTIONS(SpcSpOpusInfo)
ASN1_SEQUENCE(SpcSpOpusInfo) = {
	ASN1_EXP_OPT(SpcSpOpusInfo, description, SpcString, 0),
	ASN1_EXP_OPT(SpcSpOpusInfo, url, SpcLink, 1)
} ASN1_SEQUENCE_END(SpcSpOpusInfo)
IMPLEMENT_ASN1_FUNCTIONS(SpcSpOpusInfo)

//////////

/* SpcPeImageData ::= SEQUENCE {
   flags                   SpcPeImageFlags DEFAULT { includeResources },
   file                    SpcLink
}

SpcPeImageFlags ::= BIT STRING {
    includeResources            (0),
    includeDebugInfo            (1),
    includeImportAddressTable   (2)
} */

struct SpcPeImageData
{
	ASN1_BIT_STRING *flags;
	SpcLink *file;
};

DECLARE_ASN1_FUNCTIONS(SpcPeImageData)
ASN1_SEQUENCE(SpcPeImageData) = {
	ASN1_OPT(SpcPeImageData, flags, ASN1_BIT_STRING),
	ASN1_EXP_OPT(SpcPeImageData, file, SpcLink, 0)
} ASN1_SEQUENCE_END(SpcPeImageData)
IMPLEMENT_ASN1_FUNCTIONS(SpcPeImageData)

//////////

// This is the 'official' definition of the object. However, we specialize
// for our purposes since 'value' is always a SPC_PE_IMAGE_DATA object in
// our case.
/* SpcAttributeTypeAndOptionalValue ::= SEQUENCE {
       type                    ObjectID,
       value                   [0] EXPLICIT ANY OPTIONAL
   } */

struct SpcAttributeTypeAndOptionalValue
{
	ASN1_OBJECT *type;
	SpcPeImageData *value;
};

DECLARE_ASN1_FUNCTIONS(SpcAttributeTypeAndOptionalValue)
IMPLEMENT_ASN1_FUNCTIONS(SpcAttributeTypeAndOptionalValue)

ASN1_SEQUENCE(SpcAttributeTypeAndOptionalValue) = {
	ASN1_SIMPLE(SpcAttributeTypeAndOptionalValue, type, ASN1_OBJECT),
	ASN1_SIMPLE(SpcAttributeTypeAndOptionalValue, value, SpcPeImageData)
} ASN1_SEQUENCE_END(SpcAttributeTypeAndOptionalValue)

//////////

/* SpcIndirectDataContent ::= SEQUENCE {
       data                    SpcAttributeTypeAndOptionalValue,
       messageDigest           DigestInfo
   } */

struct SpcIndirectDataContent
{
	SpcAttributeTypeAndOptionalValue *data;
	DigestInfo *digest;
};

DECLARE_ASN1_FUNCTIONS(SpcIndirectDataContent)
ASN1_SEQUENCE(SpcIndirectDataContent) = {
	ASN1_SIMPLE(SpcIndirectDataContent, data, SpcAttributeTypeAndOptionalValue),
	ASN1_SIMPLE(SpcIndirectDataContent, digest, DigestInfo)
} ASN1_SEQUENCE_END(SpcIndirectDataContent)
IMPLEMENT_ASN1_FUNCTIONS(SpcIndirectDataContent)

//////////

/* SpcTimeStampRequestBlob ::= SEQUENCE {
		type					ObjectID,
		signature				[0] EXPLICIT OCTETSTRING OPTIONAL
   } */

struct SpcTimeStampRequestBlob
{
	ASN1_OBJECT *type;
	ASN1_OCTET_STRING *signature;
};

DECLARE_ASN1_FUNCTIONS(SpcTimeStampRequestBlob)
ASN1_SEQUENCE(SpcTimeStampRequestBlob) = {
	ASN1_SIMPLE(SpcTimeStampRequestBlob, type, ASN1_OBJECT),
    ASN1_EXP_OPT(SpcTimeStampRequestBlob, signature, ASN1_OCTET_STRING, 0)
} ASN1_SEQUENCE_END(SpcTimeStampRequestBlob)
IMPLEMENT_ASN1_FUNCTIONS(SpcTimeStampRequestBlob)

//////////

/* SpcTimeStampRequest ::= SEQUENCE {
		type					ObjectID,
		blob					SpcTimeStampRequestBlob
   } */

struct SpcTimeStampRequest
{
	ASN1_OBJECT *type;
	SpcTimeStampRequestBlob *blob;
};

DECLARE_ASN1_FUNCTIONS(SpcTimeStampRequest)
ASN1_SEQUENCE(SpcTimeStampRequest) = {
    ASN1_SIMPLE(SpcTimeStampRequest, type, ASN1_OBJECT),
    ASN1_SIMPLE(SpcTimeStampRequest, blob, SpcTimeStampRequestBlob)
} ASN1_SEQUENCE_END(SpcTimeStampRequest)
IMPLEMENT_ASN1_FUNCTIONS(SpcTimeStampRequest)

////////////////////////////////////////////////////////////////////////////////

// This method throws an error of the principal generic type, with any additional
// OpenSSL info.
static bool MCDeployThrowOpenSSL(MCDeployError p_error)
{
	return MCDeployThrow(p_error);
}

////////////////////////////////////////////////////////////////////////////////

// This template simplifies conversion of a object structure to the DER
// binary encoding.
template<typename T> static bool i2d(int (*p_i2d)(T *, unsigned char **), T *p_object, uint8_t*& r_data, uint32_t& r_length)
{
	bool t_success;
	t_success = true;

	uint32_t t_length;
	t_length = 0;
	if (t_success)
	{
		t_length = p_i2d(p_object, nil);
		if (t_length == 0)
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadSignature);
	}

	uint8_t *t_data;
	t_data = nil;
	if (t_success)
	{
		t_data = (uint8_t *)OPENSSL_malloc(t_length);
		if (t_data == nil)
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorNoMemory);
	}

	if (t_success)
	{
		p_i2d(p_object, &t_data);
		r_data = t_data - t_length;
		r_length = t_length;
	}
	else
	{
		if (t_data != nil)
			OPENSSL_free(t_data);
	}

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// Authenticode code-signing uses a PKCS#7 SignedData structure as the digital
// signature. The signature asserts two things:
//   1) The file originates from a specific software publisher.
//   2) The file has not been altered since it was signed.
//
// The PKCS#7 SignedData structure contains a number of things:
//   - the executable file's hash value
//   - a signature created by the software publisher's private key
//   - the X.509 v3 certificates that bind the software publisher's signing key
//     to a legal entity.
//   - (optional) a description of the software publisher
//   - (optional) the software publisher's URL
//   - (optional) an Authenticode timestamp
//
// It should be noted that the timestamp can only be created with a valid TSA
// (time-stamping authority) - not all Authenticode CA's provide this facility.
// Having a timestamp, though, is a good thing as it unambiguously determines
// when the signature was created thus meaning later revocation or expiry of the
// digital certificate is irrelevant.

// This method loads a X.509v3 certificate stored in an empty PKCS#7 SignedData
// structure.
static bool MCDeployCertificateLoad(MCStringRef p_passphrase, MCStringRef p_certificate, PKCS7*& r_chain)
{
    // SN-2014-01-10: The certificate filename can be in unicode.
    // As a char* is required, the conversion must be done.
    // On Mac, UTF8 suits the filesystem encoding requirements
    // On Windows, BIO_new_file expects a UFT-8 path
    // On Linux, a SysString is used
#ifdef _LINUX
    MCAutoStringRefAsSysString t_certificate;
#else
    MCAutoStringRefAsUTF8String t_certificate;
#endif
    t_certificate . Lock(p_certificate);

	BIO *t_file;
    t_file = BIO_new_file(*t_certificate, "rb");
	if (t_file == nil)
		return MCDeployThrowOpenSSL(kMCDeployErrorNoCertificate);

	PKCS7 *t_cert;
	t_cert = d2i_PKCS7_bio(t_file, nil);

	// If we failed to read the cert as binary, try as base64 encoded (VeriSign seems
	// to provide such certs like that).
	if (t_cert == nil)
	{
		BIO_seek(t_file, 0);
		t_file = BIO_push(BIO_new(BIO_f_base64()), t_file);
		t_cert = d2i_PKCS7_bio(t_file, nil);
	}

	BIO_free_all(t_file);

	if (t_cert == nil)
		return MCDeployThrowOpenSSL(kMCDeployErrorBadCertificate);

	r_chain = t_cert;
	return true;
}

// This method loads a private key stored in either PKCS#8 format, or in a the
// Microsoft PVK format.
static bool MCDeploySignLoadPVK(MCStringRef, MCStringRef, EVP_PKEY*&);
static bool MCDeployPrivateKeyLoad(MCStringRef p_passphrase, MCStringRef p_privatekey, EVP_PKEY*& r_private_key)
{
	return MCDeploySignLoadPVK(p_privatekey, p_passphrase, r_private_key);
}

// This method loads a PKCS#12 privatekey/certificate pair.
static bool MCDeployCertStoreLoad(MCStringRef p_passphrase, MCStringRef p_store, PKCS7*& r_certificate, EVP_PKEY*& r_private_key)
{
	return false;
}

static bool MCDeployBuildSpcString(const char *p_value, bool p_is_ascii, SpcString*& r_string)
{
	bool t_success;
	t_success = true;

	SpcString *t_string;
	t_string = nil;
	if (t_success)
	{
		t_string = SpcString_new();
		if (t_string == nil)
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorNoMemory);
	}

	ASN1_STRING *t_value;
	t_value = nil;
	if (t_success)
	{
		if (!ASN1_mbstring_copy(&t_value, (const unsigned char *)p_value, strlen(p_value), MBSTRING_UTF8, p_is_ascii ? B_ASN1_IA5STRING : B_ASN1_BMPSTRING))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadString);
	}

	if (t_success)
	{
		t_string -> type = p_is_ascii ? 1 : 0;
		t_string -> d . variant = t_value;
		r_string = t_string;
	}
	else
	{
		if (t_string != nil)
			SpcString_free(t_string);
		if (t_value != nil)
			ASN1_STRING_free(t_value);
	}

	return t_success;
}

static bool MCDeployBuildSpcIndirectDataContent(BIO *p_hash, SpcIndirectDataContent*& r_content)
{
	bool t_success;
	t_success = true;

	// Read the hash from the bio
	uint8_t t_hash_data[EVP_MAX_MD_SIZE];
	uint32_t t_hash_length;
	if (t_success)
	{
		t_hash_length = BIO_gets(p_hash, (char *)t_hash_data, EVP_MAX_MD_SIZE);
		if (t_hash_length == 0)
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadHash);
	}

	// Construct all the ASN.1 objects we need.
	SpcIndirectDataContent *t_content;
	SpcAttributeTypeAndOptionalValue *t_data;
	DigestInfo *t_message_digest;
	AlgorithmIdentifier *t_digest_algorithm;
	SpcPeImageData *t_image_data;
	SpcLink *t_image_data_file;
	t_data = nil;
	t_message_digest = nil;
	t_digest_algorithm = nil;
	t_image_data = nil;
	t_content = nil;
	t_image_data_file = nil;
	if (t_success)
	{
		t_content = SpcIndirectDataContent_new();
		t_data = SpcAttributeTypeAndOptionalValue_new();
		t_message_digest = DigestInfo_new();
		t_digest_algorithm = AlgorithmIdentifier_new();
		t_image_data = SpcPeImageData_new();
		t_image_data_file = SpcLink_new();
		if (t_content == nil || t_data == nil || t_message_digest == nil || t_digest_algorithm == nil ||
			t_image_data == nil || t_image_data_file == nil)
			t_success = MCDeployThrow(kMCDeployErrorNoMemory);
	}

	if (t_success)
		t_success = MCDeployBuildSpcString("<<<Obsolete>>>", false, t_image_data_file -> d . file);

	if (t_success)
	{
		t_image_data -> flags = ASN1_BIT_STRING_new();
		if (t_image_data -> flags == nil)
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorNoMemory);
	}

	if (t_success)
	{
		t_message_digest -> digest = ASN1_OCTET_STRING_new();
		if (t_message_digest -> digest == nil ||
			!ASN1_OCTET_STRING_set(t_message_digest -> digest, t_hash_data, t_hash_length))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorNoMemory);
	}

	if (t_success)
	{
		t_digest_algorithm -> parameters = ASN1_NULL_new();
		if (t_digest_algorithm -> parameters == nil)
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorNoMemory);
	}

	if (t_success)
	{
		t_content -> data = t_data;
		t_content -> digest = t_message_digest;

		t_data -> type = OBJ_txt2obj(SPC_PE_IMAGE_DATA_OBJID, 0);
		t_data -> value = t_image_data;

		t_message_digest -> algorithm = t_digest_algorithm;

		t_digest_algorithm -> algorithm = OBJ_nid2obj(NID_sha1); // NID_md5

		t_image_data_file -> type = 2;
		t_image_data -> file = t_image_data_file;

		r_content = t_content;
	}
	else
	{
		if (t_image_data_file != nil)
			SpcLink_free(t_image_data_file);
		if (t_image_data != nil)
			SpcPeImageData_free(t_image_data);
		if (t_digest_algorithm != nil)
			AlgorithmIdentifier_free(t_digest_algorithm);
		if (t_message_digest != nil)
			DigestInfo_free(t_message_digest);
		if (t_data != nil)
			SpcAttributeTypeAndOptionalValue_free(t_data);
		if (t_content != nil)
			SpcIndirectDataContent_free(t_content);
	}

	return t_success;
}

constexpr uint32_t kSecurityEntryOffset32 = 152;
constexpr uint32_t kSecurityEntryOffset64 = 168;

// This method checks to see if the input file is a valid Windows EXE (well, as
// valid as we need it to be). It then returns the offset to the PE header if
// successful. Additionally, we return the offset of the certificate entry, or
// the length of the file (if no current cert).
static bool MCDeploySignCheckWindowsExecutable(MCDeployFileRef p_input, uint32_t& r_pe_offset, uint32_t& r_cert_offset, MCDeployArchitecture &r_architecture)
{
	// First get the length of the input file
	uint32_t t_length;
	if (!MCDeployFileMeasure(p_input, t_length))
		return false;

	uint32_t t_offset;
	if (!MCDeployWindowsPEHeaderOffset(p_input, t_offset))
		return false;

	MCDeployArchitecture t_arch;
	if (!MCDeployWindowsArchitecture(p_input, t_offset, t_arch))
		return false;

	uint32_t t_security_offset = t_offset;
	if (t_arch == kMCDeployArchitecture_I386)
	{
		t_security_offset += kSecurityEntryOffset32;
	}
	else
	{
		t_security_offset += kSecurityEntryOffset64;
	}

	// Now read in the existing cert fields. Note that the offset here is
	// that of the 5th data directory entry
	uint32_t t_cert_section[2];
	if (!MCDeployFileReadAt(p_input, t_cert_section, 2 * sizeof(uint32_t), t_security_offset))
	{
		return MCDeployThrow(kMCDeployErrorWindowsBadNTSignature);
	}

	MCDeployByteSwap32(false, t_cert_section[0]);
	MCDeployByteSwap32(false, t_cert_section[1]);

	// Make sure that if there is an existing certicate, that it is at the end
	// of the file.
	if (t_cert_section[0] != 0 && t_cert_section[0] + t_cert_section[1] != t_length)
		return MCDeployThrow(kMCDeployErrorWindowsBadSecuritySection);

	if (t_cert_section[0] != 0)
		r_cert_offset = t_cert_section[0];
	else
		r_cert_offset = t_length;

	r_pe_offset = t_offset;
	r_architecture = t_arch;

	return true;
}

// This method copies data from the input file to the output BIO.
static bool MCDeploySignCopyFileAt(BIO *p_output, MCDeployFileRef p_input, uint32_t p_offset, uint32_t p_amount)
{
	bool t_success;
	t_success = true;

	if (!MCDeployFileSeekSet(p_input, p_offset))
		return MCDeployThrow(kMCDeployErrorBadRead);

	while(p_amount > 0)
	{
		char t_buffer[4096];
		int t_size;
		t_size = MCU_min(4096U, p_amount);
		
		if (!MCDeployFileRead(p_input, t_buffer, t_size))
			return MCDeployThrow(kMCDeployErrorBadRead);
		
		if (BIO_write(p_output, t_buffer, t_size) != t_size)
			return MCDeployThrowOpenSSL(kMCDeployErrorBadWrite);

		p_amount -= t_size;
	}

	return true;
}

// This method reconstructs the output executable from the input executable
// while computing the hash of the critical parts of the file.
static bool MCDeploySignHashWindowsExecutable(MCDeployFileRef p_input, BIO *p_output, uint32_t p_pe_offset, uint32_t p_cert_offset, MCDeployArchitecture p_architecture, BIO*& r_hash)
{
	bool t_success;
	t_success = true;

	// First construct a hash BIO and chain it to the output bio.
	BIO *t_hash;
	t_hash = nil;
	if (t_success)
	{
		t_hash = BIO_new(BIO_f_md());
		if (t_hash == nil)
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorNoMemory);

		BIO_set_md(t_hash, EVP_sha1()); // EVP_md5
		BIO_push(t_hash, p_output);
	}

	// Now we generate the new output file, hashing the appropriate portions of
	// the input executable as we go.

	// The first part of the output file is everything up to the start of the
	// 'CheckSum' field of the IMAGE_OPTIONAL_HEADER structure. This is at
	// offset 88 in both IMAGE_OPTIONAL_HEADER32 and IMAGE_OPTIONAL_HEADER64.
	// This part is part of the hash.
	if (t_success)
		t_success = MCDeploySignCopyFileAt(t_hash, p_input, 0, p_pe_offset + 88);

	// Next we write out an empty (0) CheckSum field - this section is not
	// hashed.
	if (t_success)
	{
		uint32_t t_checksum;
		t_checksum = 0;
		if (BIO_write(p_output, &t_checksum, sizeof(t_checksum)) != sizeof(uint32_t))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadWrite);
	}

	uint32_t t_security_offset = p_pe_offset;
	if (p_architecture == kMCDeployArchitecture_I386)
	{
		t_security_offset += kSecurityEntryOffset32;
	}
	else
	{
		t_security_offset += kSecurityEntryOffset64;
	}

	// Next is the section of the header after the CheckSum field and up to
	// the 'Security' data directory entry.
	if (t_success)
		t_success = MCDeploySignCopyFileAt(t_hash, p_input, p_pe_offset + 92, t_security_offset - (p_pe_offset + 92));

	// Now write out the (current) value of the Security data directory entry,
	// but not into the hash.
	if (t_success)
		t_success = MCDeploySignCopyFileAt(p_output, p_input, t_security_offset, 8);

	// After the Security data directory, everything up to the cert offset is
	// hashed.
	if (t_success)
		t_success = MCDeploySignCopyFileAt(t_hash, p_input, t_security_offset + 8, p_cert_offset - (t_security_offset + 8));

	// Finally we round the output up to the nearest 8 bytes.
	if (t_success && (p_cert_offset % 8 != 0))
	{
		uint8_t t_pad[8];
		MCMemoryClear(t_pad, 8);

		int t_pad_length;
		t_pad_length = 8 - (p_cert_offset % 8);

		if (BIO_write(t_hash, t_pad, t_pad_length) != t_pad_length)
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadWrite);
	}

	// If we have failed, make sure we free the hash BIO, otherwise we return
	// it.
	if (t_success)
		r_hash = t_hash;
	else
		BIO_free(t_hash);

	return t_success;
}

static bool MCDeploySignWindowsAddOpusInfo(const MCDeploySignParameters& p_params, PKCS7_SIGNER_INFO *p_sign_info)
{
	bool t_success;
	t_success = true;

	SpcSpOpusInfo *t_opus;
	t_opus = nil;
	if (t_success)
	{
		t_opus = SpcSpOpusInfo_new();
		if (t_opus == nil)
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorNoMemory);
	}

	if (t_success && p_params . description != nil)
    {
        MCAutoStringRefAsUTF8String t_utf8_string;
        t_success = t_utf8_string . Lock(p_params . description)
                && MCDeployBuildSpcString(*t_utf8_string, false, t_opus -> description);
    }
	
	if (t_success && p_params . url != nil)
	{
		t_opus -> url = SpcLink_new();
		if (t_opus -> url != nil)
		{
			if (ASN1_mbstring_copy(&t_opus -> url -> d . url, MCStringGetNativeCharPtr(p_params.url), MCStringGetLength(p_params.url), MBSTRING_UTF8, B_ASN1_IA5STRING))
				t_opus -> url -> type = 0;
			else
				t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadString);
		}
		else
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorNoMemory);
	}
	
	uint8_t *t_opus_data;
	uint32_t t_opus_length;
	t_opus_data = nil;
	t_opus_length = 0;
	if (t_success)
		t_success = i2d(i2d_SpcSpOpusInfo, t_opus, t_opus_data, t_opus_length);

	ASN1_STRING *t_opus_string;
	t_opus_string = nil;
	if (t_success)
	{
		t_opus_string = ASN1_STRING_new();
		if (t_opus_string == nil ||
			!ASN1_STRING_set(t_opus_string, t_opus_data, t_opus_length))
			t_success = MCDeployThrow(kMCDeployErrorNoMemory);
	}

	if (t_success)
	{
		if (!PKCS7_add_signed_attribute(p_sign_info, OBJ_txt2nid(SPC_SP_OPUS_INFO_OBJID), V_ASN1_SEQUENCE, t_opus_string))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadSignature);
	}

	// Note that this is not owned by the sig if we failed.
	if (!t_success)
	{
		if (t_opus_string != nil)
			ASN1_STRING_free(t_opus_string);
	}

	if (t_opus_data != nil)
		OPENSSL_free(t_opus_data);
	if (t_opus != nil)
		SpcSpOpusInfo_free(t_opus);

	return t_success;
}

// This method adds a counter-signature to our signature that is used to verify the time
// at which it was generated. It uses the timestamping authority URL specified in the
// 'timestamper' parameter. The flow of this code is cribbed from 'osslsigncode.c' - the
// open-source implementation of the SignCode.exe utility.
static bool MCDeploySignWindowsAddTimeStamp(const MCDeploySignParameters& p_params, PKCS7 *p_signature)
{
	bool t_success;
	t_success = true;

	// Fetch the signatures 'signer_info' structure
	PKCS7_SIGNER_INFO *t_signer_info;
	t_signer_info = sk_PKCS7_SIGNER_INFO_value(p_signature -> d . sign -> signer_info, 0);

	// Build a request to send to the time-stamp authority. If there is a 'blob'
    // field in this request, then we must reset the signature field before freeing
    // it as that is borrowed from elsewhere.
	SpcTimeStampRequest *t_request;
	t_request = nil;
	if (t_success)
	{
		t_request = SpcTimeStampRequest_new();
		if (t_request != nil)
		{
			t_request -> type = OBJ_txt2obj(SPC_TIME_STAMP_REQUEST_OBJID, 1);
			t_request -> blob = SpcTimeStampRequestBlob_new();
			if (t_request -> blob != nil)
			{
				t_request -> blob -> type = OBJ_nid2obj(NID_pkcs7_data);
				t_request -> blob -> signature = t_signer_info -> enc_digest;
			}
			else
				t_success = MCDeployThrow(kMCDeployErrorNoMemory);
		}
		else
			t_success = MCDeployThrow(kMCDeployErrorNoMemory);
	}

	// Convert the request to the binary encoding
	uint8_t *t_request_data;
	uint32_t t_request_length;
	t_request_data = nil;
	t_request_length = 0;
	if (t_success)
		t_success = i2d(i2d_SpcTimeStampRequest, t_request, t_request_data, t_request_length);

	// Convert the request to base64
    MCAutoDataRef t_req_dataref;
    MCAutoStringRef t_req_base64;
    /* UNCHECKED */ MCDataCreateWithBytes(t_request_data, t_request_length, &t_req_dataref);
    MCU_base64encode(*t_req_dataref, &t_req_base64);
    
	// Request the timestamp from the tsa
	if (t_success)
	{
		// Set the HTTP headers appropriately to make sure no unpleasant caching goes on.
		MCValueAssign(MChttpheaders, MCSTR("Content-Type: application/octet-stream\nAccept: application/octet-stream\nUser-Agent: Transport\nCache-Control: no-cache"));

		// Use libURL to do the post - we attempt this 5 times with increasing sleep
		// periods. This is because it looks like the timestamping service is a little
		// unreliable (well, VeriSign's timestamping service anyway!).
		uint32_t t_retry_count, t_retry_interval;
		t_retry_count = 5;
		t_retry_interval = 50;

		bool t_failed;
		t_failed = true;
		while(t_retry_count > 0)
		{
			MCParameter t_data, t_url;
			t_data . setvalueref_argument(*t_req_base64);
			t_data . setnext(&t_url);
			t_url . setvalueref_argument(p_params . timestamper);
            extern MCExecContext *MCECptr;
			if (MCECptr->GetObject() -> message(MCM_post_url, &t_data, False, True) == ES_NORMAL &&
				MCresult -> isempty())
			{
				t_failed = false;
				break;
			}

			// Sleep for retry_interval millisecs.
			MCS_sleep(t_retry_interval / 1000.0);

			t_retry_count -= 1;
			t_retry_interval *= 2;
		}

		if (t_failed)
			t_success = MCDeployThrow(kMCDeployErrorTimestampFailed);
	}

	// Now convert the reply to binary.
    MCAutoValueRef t_result_value;
    MCAutoStringRef t_result_base64;
    MCAutoDataRef t_result_data;
    extern MCExecContext *MCECptr;
	
    if (t_success)
    {
		MCurlresult -> copyasvalueref(&t_result_value);
        t_success = MCECptr->ConvertToString(*t_result_value, &t_result_base64);
    }
    
    if (t_success)
        MCU_base64decode(*t_result_base64, &t_result_data);

	// Decode the PKCS7 structure
	PKCS7 *t_counter_sig;
	t_counter_sig = nil;
	if (t_success)
	{
		const unsigned char *t_data;
		t_data = (const unsigned char *)MCDataGetBytePtr(*t_result_data);
		int t_length;
		t_length = MCDataGetLength(*t_result_data);
		t_counter_sig = d2i_PKCS7(&t_counter_sig, &t_data, t_length);
		if (t_counter_sig == nil)
			t_success = MCDeployThrow(kMCDeployErrorBadTimestamp);
	}

	// Add the certificates we need to our signature
	if (t_success)
		for(int32_t i = sk_X509_num(t_counter_sig -> d . sign -> cert) - 1; i >= 0 && t_success; i--)
			if (!PKCS7_add_certificate(p_signature, sk_X509_value(t_counter_sig -> d . sign -> cert, i)))
				t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadTimestamp);

	// Now encode the counter signature...
	uint8_t *t_counter_sig_data;
	uint32_t t_counter_sig_length;
	t_counter_sig_data = nil;
	t_counter_sig_length = 0;
	if (t_success)
		t_success = i2d(i2d_PKCS7_SIGNER_INFO, sk_PKCS7_SIGNER_INFO_value(t_counter_sig -> d . sign -> signer_info, 0), t_counter_sig_data, t_counter_sig_length);

	// Turn it into a ASN1_STRING
	ASN1_STRING *t_counter_sig_string;
	t_counter_sig_string = nil;
	if (t_success)
	{
		t_counter_sig_string = ASN1_STRING_new();
		if (t_counter_sig_string == nil ||
			!ASN1_STRING_set(t_counter_sig_string, t_counter_sig_data, t_counter_sig_length))
			t_success = MCDeployThrow(kMCDeployErrorNoMemory);
	}

	// Finally set the appropriate attribute
	if (t_success)
		if (!PKCS7_add_attribute(t_signer_info, NID_pkcs9_countersignature, V_ASN1_SEQUENCE, t_counter_sig_string))
			t_success = MCDeployThrow(kMCDeployErrorBadTimestamp);

	// If no success, then the counter sig string will not be owned.
	if (!t_success)
		if (t_counter_sig_string != nil)
			ASN1_STRING_free(t_counter_sig_string);

	if (t_counter_sig_data != nil)
		OPENSSL_free(t_counter_sig_data);

	if (t_counter_sig != nil)
		PKCS7_free(t_counter_sig);

	if (t_request_data != nil)
		OPENSSL_free(t_request_data);

	// Free the request - making sure we zero out the field that we shared with
	// the sig.
	if (t_request != nil)
	{
        // If we have a 'blob' field then we will have potentially borrowed a
        // signature from another data structure so must unhook that here to
        // stop it being freed in SpcTimeStampRequest_free.
        if (t_request -> blob != nil)
            t_request -> blob -> signature = nil;
		SpcTimeStampRequest_free(t_request);
	}

	return t_success;
}

static bool s_objects_created = false;

bool MCDeploySignWindows(const MCDeploySignParameters& p_params)
{
	bool t_success;
	t_success = true;

	// Make sure OpenSSL knows about the SHA1 digest.
    EVP_add_digest(EVP_sha1());

	// First open input and output executable files
	MCDeployFileRef t_input;
	t_input = nil;
	if (t_success && !MCDeployFileOpen(p_params . input, kMCOpenFileModeRead, t_input))
		t_success = MCDeployThrow(kMCDeployErrorNoEngine);

	BIO *t_output;
	t_output = nil;
	if (t_success)
	{
        MCAutoStringRefAsUTF8String t_params_output_utf8;
        /* UNCHECKED */ t_params_output_utf8 . Lock(p_params . output);
		t_output = BIO_new_file(*t_params_output_utf8, "wb");
		if (t_output == nil)
			t_success = MCDeployThrow(kMCDeployErrorNoOutput);
	}

	// If a certificate store path has been given, then use that to load the SPC/PK,
	// otherwise load them separately.
	PKCS7* t_cert_chain;
	EVP_PKEY* t_privatekey;
	t_cert_chain = nil;
	t_privatekey = nil;
	if (t_success && !MCValueIsEmpty(p_params . certstore))
        t_success = MCDeployCertStoreLoad(p_params . passphrase,
                                          p_params . certstore,
										  t_cert_chain, t_privatekey);
	else if (t_success)
		t_success =
            MCDeployCertificateLoad(p_params . passphrase,
                                    p_params . certificate,
									t_cert_chain) &&
            MCDeployPrivateKeyLoad(p_params . passphrase,
                                   p_params . privatekey,
                                   t_privatekey);

	// Next we check the input file, and compute the hash, writing out the new
	// version of the executable as we go.
	uint32_t t_pe_offset, t_cert_offset;
	MCDeployArchitecture t_arch;
	if (t_success)
		t_success = MCDeploySignCheckWindowsExecutable(t_input, t_pe_offset, t_cert_offset, t_arch);

	BIO *t_hash;
	t_hash = nil;
	if (t_success)
		t_success = MCDeploySignHashWindowsExecutable(t_input, t_output, t_pe_offset, t_cert_offset, t_arch, t_hash);

	// Next we create a PKCS#7 object ready for filling with the stuff we need for
	// Authenticode.
	PKCS7 *t_signature;
	t_signature = nil;
	if (t_success)
	{
		t_signature = PKCS7_new();
		if (t_signature == nil)
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadSignature);
	}

	// The next thing to do is set the type of object our PKCS7 structure contains.
	// This is NID_pkcs7_signed - i.e. a SignedData object.
	if (t_success)
	{
		if (!PKCS7_set_type(t_signature, NID_pkcs7_signed))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadSignature);
	}

	// Now what *should* be contained within the certificate file we loaded is
	// actually a certificate chain. The certificate we use for signing is the
	// first of these - and so we attempt to extract it.
	// Note that the certificate is owned by the chain, and so does not need to
	// be independently freed.
	X509 *t_certificate;
	t_certificate = nil;
	if (t_success)
	{
		if (sk_X509_num(t_cert_chain -> d . sign -> cert) >= 1)
			t_certificate = sk_X509_value(t_cert_chain -> d . sign -> cert, 0);
		else
			t_success = MCDeployThrow(kMCDeployErrorEmptyCertificate);
	}

	// Now check that the cert we are going to use to sign with, matches the private
	// key - note that this only checks the n and d parameters of the keys which I
	// discovered after many hours of debugging my code, only to discover my private
	// key reading function was wrong!
	if (t_success && !X509_check_private_key(t_certificate, t_privatekey))
			t_success = MCDeployThrow(kMCDeployErrorCertMismatch);

	// The various ASN.1 structures we are going to use require a number of ObjectIDs.
	// We register them all here, to save having to check the return value of OBJ_txt2obj.
	if (t_success && !s_objects_created)
	{
		if (!OBJ_create(SPC_INDIRECT_DATA_OBJID, SPC_INDIRECT_DATA_OBJID, SPC_INDIRECT_DATA_OBJID) ||
			!OBJ_create(SPC_PE_IMAGE_DATA_OBJID, SPC_PE_IMAGE_DATA_OBJID, SPC_PE_IMAGE_DATA_OBJID) ||
			!OBJ_create(SPC_STATEMENT_TYPE_OBJID, SPC_STATEMENT_TYPE_OBJID, SPC_STATEMENT_TYPE_OBJID) ||
			!OBJ_create(SPC_SP_OPUS_INFO_OBJID, SPC_SP_OPUS_INFO_OBJID, SPC_SP_OPUS_INFO_OBJID))
		{
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadSignature);
		}
		
		s_objects_created = true;
	}

	// Authenticode signatures require a single SignerInfo structure to be present.
	// To create this we add a signature for the certificate we just located.
	// Note that the signer info object we create here is owned by the signature,
	// and so doesn't need to be freed explicitly.
	PKCS7_SIGNER_INFO *t_sign_info;
	t_sign_info = nil;
	if (t_success)
	{
		t_sign_info = PKCS7_add_signature(t_signature, t_certificate, t_privatekey, EVP_sha1()); // EVP_md5()
		if (t_sign_info == nil)
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadSignature);
	}

	// The Authenticode content type is SPC_INDIRECT_DATA_OBJID. This is a signed
	// attribute.
	if (t_success)
	{
		if (!PKCS7_add_signed_attribute(t_sign_info, NID_pkcs9_contentType, V_ASN1_OBJECT, OBJ_txt2obj(SPC_INDIRECT_DATA_OBJID, 0)))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadSignature);
	}

	// Here we should add the SpcOpusInfo attribute
	if (t_success)
		t_success = MCDeploySignWindowsAddOpusInfo(p_params, t_sign_info);

	// The next thing to do is to create the signature content. For Authenticode
	// this is an SpcIndirectDataContent object. This is built with the digest
	// taken from the hash BIO.
	SpcIndirectDataContent *t_content;
	t_content = nil;
	if (t_success)
		t_success = MCDeployBuildSpcIndirectDataContent(t_hash, t_content);

	// Now we have the content data, we add it to the PKCS7 structure.
	if (t_success)
	{
		if (!PKCS7_content_new(t_signature, NID_pkcs7_data))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadSignature);
	}

	// Next, make sure we store the list of certificates needed to validate the
	// signature, but in reverse order.
	if (t_success)
		for(int32_t i = sk_X509_num(t_cert_chain -> d . sign -> cert) - 1; i >= 0 && t_success; i--)
			if (!PKCS7_add_certificate(t_signature, sk_X509_value(t_cert_chain -> d . sign -> cert, i)))
				t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadSignature);

	// Serialize the data first - this creates a byte sequence representing
	// the SpcIndirectDataContent object.
	uint8_t *t_content_data;
	uint32_t t_content_size;
	t_content_data = nil;
	t_content_size = 0;
	if (t_success)
		t_success = i2d(i2d_SpcIndirectDataContent, t_content, t_content_data, t_content_size);

	// Now we have the byte stream that is the content of the signature (i.e.
	// the bit that is encrypted/checksummed etc.) we write it to the sig.
	// Notice that the content data buffer is adjusted slightly. This is because
	// the actualy encryption/digest is *only* applied to the content of the
	// ASN.1 type - in this case the first two bytes will be the sequence
	// header. (Or at least this is my interpreatation of existing code/a
	// brief look at the relevant RFCs!).
	if (t_success)
	{
		BIO *t_stream;
		t_stream = PKCS7_dataInit(t_signature, nil);
		if (t_stream == nil ||
			BIO_write(t_stream, t_content_data + 2, t_content_size - 2) != int(t_content_size - 2) ||
			!PKCS7_dataFinal(t_signature, t_stream))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadSignature);

		if (t_stream != nil)
			BIO_free(t_stream);
	}

	// We now set the content fields of the signature correctly. We use the
	// serialized version of the content since the asn1 stuff for PKCS7 knows
	// nothing of our ASN1 object.
	if (t_success)
	{
		ASN1_STRING *t_string;
		ASN1_TYPE *t_type;
		t_string = ASN1_STRING_new();
		t_type = ASN1_TYPE_new();
		if (t_string != nil && t_type != nil && ASN1_STRING_set(t_string, t_content_data, t_content_size))
		{
			t_type -> type = V_ASN1_SEQUENCE;
			t_type -> value . sequence = t_string;
			t_signature -> d . sign -> contents -> type = OBJ_txt2obj(SPC_INDIRECT_DATA_OBJID, 0);
			t_signature -> d . sign -> contents -> d . other = t_type;
		}
		else
		{
			if (t_type != nil)
				ASN1_TYPE_free(t_type);
			if (t_string != nil)
				ASN1_STRING_free(t_string);
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadSignature);
		}
	}

	// Now we have our signature, we timestamp it - but only if we were
	// given a timestamp authority url.
	if (t_success && !MCValueIsEmpty(p_params . timestamper))
		t_success = MCDeploySignWindowsAddTimeStamp(p_params, t_signature);

	// We now have a complete PKCS7 SignedData object which is now serialized.
	uint8_t *t_signature_data;
	uint32_t t_signature_size;
	t_signature_data = nil;
	t_signature_size = 0;
	if (t_success)
		t_success = i2d(i2d_PKCS7, t_signature, t_signature_data, t_signature_size);

	// Finally we append the output file with the 'Attribute Certificate Table'...
	if (t_success)
	{
		// The table goes at the end and has the following structure:
		//   DWORD dwLength = <t_signature_size>
		//   WORD wRevision = 0x0200 (WIN_CERT_REVISION_2_0)
		//   WORD wCertificateType = 0x0002 = (WIN_CERT_TYPE_PKCS_SIGNED_DATA)
		//   BYTE bCertificate[]
		//   PAD TO 8 bytes
		struct { uint32_t dwLength; uint16_t wRevision; uint16_t wCertificateType; } t_table;
		t_table . dwLength = 8 + t_signature_size;
		t_table . wRevision = 0x0200;
		t_table . wCertificateType = 0x0002;
		MCDeployByteSwapRecord(false, "lss", &t_table, sizeof(t_table));
		
		// Write out the table header
		if (BIO_write(t_output, &t_table, sizeof(t_table)) != sizeof(t_table))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadWrite);

		// Write out the certificate data
		if (BIO_write(t_output, t_signature_data, t_signature_size) != int(t_signature_size))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadWrite);

		// Write out the padding
		if (t_signature_size % 8 != 0)
		{
			uint8_t t_pad[8];
			int t_pad_length;
			t_pad_length = 8 - (t_signature_size % 8);
			MCMemoryClear(t_pad, t_pad_length);
			if (BIO_write(t_output, t_pad, t_pad_length) != t_pad_length)
				t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadWrite);
		}
	}

	// ... And update the SECURITY table entry in the PE header appropraitely.
	if (t_success)
	{
		uint32_t t_security_offset = t_pe_offset;
		if (t_arch == kMCDeployArchitecture_I386)
		{
			t_security_offset += kSecurityEntryOffset32;
		}
		else
		{
			t_security_offset += kSecurityEntryOffset64;
		}

		uint32_t t_entry[2];

		// First entry is the offset of the cert, making sure it is padded to 8
		// bytes.
		t_entry[0] = (t_cert_offset + 7) & ~7;

		// Next item is the length of the attribute cert table
		t_entry[1] = 8 + t_signature_size + (t_signature_size % 8 != 0 ? (8 - (t_signature_size % 8)) : 0);

		MCDeployByteSwapRecord(false, "ll", t_entry, sizeof(t_entry));
		if (BIO_seek(t_output, t_security_offset) == -1 ||
			BIO_write(t_output, t_entry, sizeof(t_entry)) != sizeof(t_entry))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadWrite);
	}

	// Free up resources
	if (t_signature_data != nil)
		OPENSSL_free(t_signature_data);
	if (t_content_data != nil)
		OPENSSL_free(t_content_data);
	if (t_content != nil)
		SpcIndirectDataContent_free(t_content);
	if (t_signature != nil)
		PKCS7_free(t_signature);
	if (t_privatekey != nil)
		EVP_PKEY_free(t_privatekey);
	if (t_cert_chain != nil)
		PKCS7_free(t_cert_chain);
	if (t_hash != nil)
		BIO_free(t_hash);
	if (t_output != nil)
		BIO_free(t_output);
	if (t_input != nil)
		MCDeployFileClose(t_input);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

struct PVK_HEADER
{
	uint32_t magic;
	uint32_t reserved;
	uint32_t key_type;
	uint32_t is_encrypted;
	uint32_t salt_length;
	uint32_t key_length;
};

struct PVK_BLOBHEADER
{
	uint8_t bType;
	uint8_t bVersion;
	uint16_t reserved;
	uint32_t aiKeyAlg;
};

struct PVK_RSAKEY
{
	uint32_t magic;
	uint32_t length;
	uint32_t exponent;
};

static bool read_le_bignum(uint8_t*& x_data, uint32_t p_bytes, BIGNUM*& r_bignum)
{
	uint8_t *t_be_data;
	t_be_data = (uint8_t *)OPENSSL_malloc(p_bytes);
	if (t_be_data == nil)
		return false;

	for(uint32_t i = 0; i < p_bytes; i++)
		t_be_data[i] = x_data[p_bytes - i - 1];

	BIGNUM *t_bignum;
	t_bignum = BN_bin2bn(t_be_data, p_bytes, nil);

	OPENSSL_free(t_be_data);
	
	if (t_bignum == nil)
		return false;

	x_data += p_bytes;
	r_bignum = t_bignum;

	return true;
}

bool MCDeploySignLoadPVK(MCStringRef p_filename, MCStringRef p_passphrase, EVP_PKEY*& r_key)
{
	bool t_success;
	t_success = true;

    // SN-2014-01-10: The private key filename can be in unicode.
    // As a char* is required, the conversion must be done.
    // On Mac, UTF8 suits the filesystem encoding requirements
    // On Windows, BIO_new_file expects a UFT-8 path
    // On Linux, a SysString is used
#ifdef _LINUX
    MCAutoStringRefAsSysString t_private_key;
#else
    MCAutoStringRefAsUTF8String t_private_key;
#endif
    t_success = t_private_key . Lock(p_filename);

	// First try and open the input file
	BIO *t_input;
	t_input = nil;
	if (t_success)
	{
        t_input = BIO_new_file(*t_private_key, "rb");
		if (t_input == nil)
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorNoPrivateKey);
	}

	// Now try and read the PVK header
	PVK_HEADER t_header;
	if (t_success)
	{
		if (BIO_read(t_input, &t_header, sizeof(PVK_HEADER)) == sizeof(PVK_HEADER))
			MCDeployByteSwapRecord(false, "llllll", &t_header, sizeof(PVK_HEADER));
		else
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadPrivateKey);
	}

	// Check the magic
	if (t_success && t_header . magic != 0xb0b5f11e)
		t_success = MCDeployThrow(kMCDeployErrorBadPrivateKey);

	// Next we read the salt (if any)
	uint8_t *t_salt;
	t_salt = nil;
	if (t_success && t_header . salt_length > 0)
	{
		if (t_success)
			t_success = MCMemoryNewArray(t_header . salt_length, t_salt);
		if (t_success)
			if (BIO_read(t_input, t_salt, t_header . salt_length) != int(t_header . salt_length))
				t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadPrivateKey);
	}

	// After the salt comes the BLOB header
	PVK_BLOBHEADER t_blob_header;
	if (t_success)
	{
		if (BIO_read(t_input, &t_blob_header, sizeof(PVK_BLOBHEADER)) == sizeof(PVK_BLOBHEADER))
			MCDeployByteSwapRecord(false, "bbsl", &t_blob_header, sizeof(PVK_BLOBHEADER));
		else
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadPrivateKey);
	}

	// Now we try and read in the (potentially encrypted) private key data.
	uint8_t *t_key_data;
	uint32_t t_key_length;
	t_key_data = nil;
	t_key_length = 0;
	if (t_success)
	{
		// Note BLOBHEADER was the first 8 bytes.
		t_key_length = t_header . key_length - 8;
		t_success = MCMemoryNewArray(t_key_length, t_key_data);
	}
	if (t_success)
		if (BIO_read(t_input, t_key_data, t_key_length) != int(t_key_length))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorBadPrivateKey);

	// We now have everything we need to attempt to decrypt the key (if necessary).
	if (t_success && t_header . is_encrypted)
	{
		// First check we have a password
		if (t_success && p_passphrase == nil)
			t_success = MCDeployThrow(kMCDeployErrorNoPassword);
		
		// Now allocate a new key data array to decrypt into. Note we make this
		// array 8 bytes longer to make sure decryption has enough room (output
		// of decryption is rounded up to the nearest block - 8 byte - boundary).
		uint8_t *t_new_key_data;
		t_new_key_data = nil;
		if (t_success)
			t_success = MCMemoryNewArray(t_key_length + 8, t_new_key_data);

		// Compute the passkey. This is done by taking the first 16 bytes
		// of SHA1(salt & passphrase).
		uint8_t t_passkey[EVP_MAX_KEY_LENGTH];
        MCAutoCustomPointer<EVP_MD_CTX, EVP_MD_CTX_free> t_md = EVP_MD_CTX_new();
		if (t_success && EVP_DigestInit(*t_md, EVP_sha1()))
        {
            MCAutoStringRefAsCString t_passphrase;
            t_success = t_passphrase . Lock(p_passphrase);

			EVP_DigestUpdate(*t_md, t_salt, t_header . salt_length);
            EVP_DigestUpdate(*t_md, *t_passphrase, strlen(*t_passphrase));
			EVP_DigestFinal(*t_md, t_passkey, NULL);
		}

		// Now, first we see if the PVK can be decrypted using the strong form
		// of password generation - that is the first 16 bytes of the hash we
		// just made.
		MCAutoCustomPointer<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free> t_cipher = EVP_CIPHER_CTX_new();
		int t_cipher_output;
		t_cipher_output = 0;
		if (t_success && EVP_DecryptInit(*t_cipher, EVP_rc4(), t_passkey, nil))
		{
			EVP_DecryptUpdate(*t_cipher, t_new_key_data, &t_cipher_output, t_key_data, t_header . key_length);
			EVP_DecryptFinal(*t_cipher, t_new_key_data + t_cipher_output, &t_cipher_output);
		}

		// Check to see if 'RSA2' is the first four bytes of the output, and if
		// not try the weak form of password generation - that is the first 5
		// bytes of our hash followed by 11 zero bytes.
		if (t_success && !MCMemoryEqual(t_new_key_data, "RSA2", 4))
		{
			t_cipher_output = 0;
			MCMemoryClear(t_passkey + 5, 11);
			if (EVP_DecryptInit(*t_cipher, EVP_rc4(), t_passkey, nil))
			{
				EVP_DecryptUpdate(*t_cipher, t_new_key_data, &t_cipher_output, t_key_data, t_header . key_length);
				EVP_DecryptFinal(*t_cipher, t_new_key_data + t_cipher_output, &t_cipher_output);
			}
		}

		// If we get to this point and the output data doesn't have the header
		// we need - its a password error.
		if (t_success && !MCMemoryEqual(t_new_key_data, "RSA2", 4))
			t_success = MCDeployThrow(kMCDeployErrorBadPassword);

		// Switch the key data over on success
		if (t_success)
		{
			MCMemoryDeleteArray(t_key_data);
			t_key_data = t_new_key_data;
		}
		else
			MCMemoryDeleteArray(t_new_key_data);
	}

	// At this point we have decrypted the key so now must convert it to an OpenSSL RSA struct.
	RSA *t_rsa;
	t_rsa = nil;
	if (t_success)
	{
		t_rsa = RSA_new();
		if (t_rsa == nil)
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorNoMemory);
	}

	// The first 12 bytes of the key data are (magic, length, exponent) all as 32-bit words.
	PVK_RSAKEY t_rsa_header;
	uint8_t *t_rsa_data;
	if (t_success && t_key_length >= 12)
	{
		MCMemoryCopy(&t_rsa_header, t_key_data, 12);
		t_rsa_data = t_key_data + 12;
		MCDeployByteSwapRecord(false, "bbbbll", &t_rsa_header, sizeof(t_rsa_header));
	}
	else
		t_success = MCDeployThrow(kMCDeployErrorBadPrivateKey);

	// The size of the key in the header is stored in bits, so convert to bytes
	uint32_t t_key_byte_length;
	if (t_success)
		t_key_byte_length = t_rsa_header . length >> 3;

	// Now check that we have enough data for all the parts of the key.
	if (t_success && t_key_length < 12 + (t_key_byte_length / 2) * 9)
		t_success = MCDeployThrow(kMCDeployErrorBadPrivateKey);

	// Now we can happily start extracting all the numbers. These are stored as
	// byte sequences in little-endian order.

	// The exponent is first - this is just a 32-bit number, so we deal with it explicitly.
    typedef MCAutoCustomPointer<BIGNUM, BN_free> MCAutoBignum;
    MCAutoBignum t_rsa_e;
	if (t_success)
	{
        t_rsa_e = BN_new();
		if (!t_rsa_e || !BN_set_word(*t_rsa_e, t_rsa_header.exponent))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorNoMemory);
	}
	
	// The rest of the numbers for the RSA2 key need special processing.
    MCAutoBignum t_rsa_n;
    MCAutoBignum t_rsa_p;
    MCAutoBignum t_rsa_q;
    MCAutoBignum t_rsa_dmp1;
    MCAutoBignum t_rsa_dmq1;
    MCAutoBignum t_rsa_iqmp;
    MCAutoBignum t_rsa_d;
	if (t_success)
		if (!read_le_bignum(t_rsa_data, t_key_byte_length, &t_rsa_n) ||
			!read_le_bignum(t_rsa_data, t_key_byte_length / 2, &t_rsa_p) ||
			!read_le_bignum(t_rsa_data, t_key_byte_length / 2, &t_rsa_q) ||
			!read_le_bignum(t_rsa_data, t_key_byte_length / 2, &t_rsa_dmp1) ||
			!read_le_bignum(t_rsa_data, t_key_byte_length / 2, &t_rsa_dmq1) ||
			!read_le_bignum(t_rsa_data, t_key_byte_length / 2, &t_rsa_iqmp) ||
			!read_le_bignum(t_rsa_data, t_key_byte_length, &t_rsa_d))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorNoMemory);
    
    RSA_set0_key(t_rsa, t_rsa_n.Release(), t_rsa_e.Release(), t_rsa_d.Release());
    RSA_set0_factors(t_rsa, t_rsa_p.Release(), t_rsa_q.Release());
    RSA_set0_crt_params(t_rsa, t_rsa_dmp1.Release(), t_rsa_dmq1.Release(), t_rsa_iqmp.Release());

	// We now have the RSA key, so wrap it in an EVP_PKEY and return.
	EVP_PKEY *t_pkey;
	t_pkey = nil;
	if (t_success)
	{
		t_pkey = EVP_PKEY_new();
		if (t_pkey == nil ||
			!EVP_PKEY_assign_RSA(t_pkey, t_rsa))
			t_success = MCDeployThrowOpenSSL(kMCDeployErrorNoMemory);
	}

	if (t_success)
		r_key = t_pkey;
	else
	{
		if (t_pkey != nil)
			EVP_PKEY_free(t_pkey);
		if (t_rsa != nil)
			RSA_free(t_rsa);
	}

	// Cleanup
	MCMemoryDeleteArray(t_key_data);
	MCMemoryDeleteArray(t_salt);
	if (t_input != nil)
		BIO_free(t_input);

	return t_success;
}

//////////////////////////////////////////////////////////////////////
