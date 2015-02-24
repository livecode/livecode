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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "handler.h"
#include "scriptpt.h"
#include "variable.h"
#include "statemnt.h"

#include "osspec.h"
#include "ide.h"
#include "stacksecurity.h"
#include "mcio.h"

#include "script.h"
#include "script-auto.h"

////////////////////////////////////////////////////////////////////////////////

static bool MCDeployBuilderCreateModuleInputStreamFromFile(MCStringRef p_filename, MCStreamRef& r_stream)
{
    // If the file does not exist - it is an error.
    if (!MCS_exists(p_filename, True))
    {
        if (!MCErrorIsPending())
            MCErrorThrowGeneric(MCSTR("module file not found"));
        return false;
    }
    
    // If the file cannot be loaded - it is an error.
    MCAutoDataRef t_data;
    if (!MCS_loadbinaryfile(p_filename, Out(t_data)))
    {
        if (!MCErrorIsPending())
            MCErrorThrowGeneric(MCSTR("module file not loadable"));
        return false;
    }
    
    // Now create an input stream from the data.
    MCAutoValueRefBase<MCStreamRef> t_stream;
    if (!MCMemoryInputStreamCreate(MCDataGetBytePtr(In(t_data)), MCDataGetLength(In(t_data)), Out(t_stream)))
        return false;
    
    r_stream = t_stream . Take();
    
    return true;
}

/////////

// The 'validate' builder command checks that the given file is a valid module
// file. This means it can be at least be loaded by the engine.
void MCDeployBuilderValidate(MCStringRef p_filename)
{
    // At the moment validation means that the module can be loaded - i.e. it is
    // a valid module file.
    
    MCAutoValueRefBase<MCStreamRef> t_stream;
    if (!MCDeployBuilderCreateModuleInputStreamFromFile(p_filename, Out(t_stream)))
        return;
    
    // Attempt to load the module from the stream.
    MCAutoScriptModuleRef t_module;
    if (!MCScriptCreateModuleFromStream(In(t_stream), &t_module))
    {
        if (!MCErrorIsPending())
            MCErrorThrowGeneric(MCSTR("module file is not valid"));
        return;
    }
    
    // If we get here we have nothing to do - the module is valid, so we don't
    // throw an error and so return a 0 exit code and print nothing to stderr.
}

//////////

// The 'protect' builder command performs the commercial protectection process
// on the given module file.
void MCDeployBuilderProtect(MCStringRef p_filename)
{
    // The protection primitive is implemented in stacksecurity.cpp - which has
    // differing implementations in community and commercial however we do
    // exactly the same thing on both to ensure that the messages are similar.
    
    MCAutoValueRefBase<MCStreamRef> t_input;
    if (!MCDeployBuilderCreateModuleInputStreamFromFile(p_filename, Out(t_input)))
        return;
    
    // Create a memory output stream to store the protected module in.
    MCAutoValueRefBase<MCStreamRef> t_output;
    if (!MCMemoryOutputStreamCreate(Out(t_output)))
        return;
    
    // Now add the protected module processing to it.
    MCAutoValueRefBase<MCStreamRef> t_protected_output;
    if (!MCStackSecurityProcessModule(In(t_output), Out(t_protected_output)))
        return;
    
    // Transfer data from the input stream to the output stream.
    size_t t_bytes_available;
    if (!MCStreamGetAvailableForRead(In(t_input), t_bytes_available))
        return;
    
    while(t_bytes_available > 0)
    {
        size_t t_to_read;
        t_to_read = MCMin(t_bytes_available, 4096U);
        
        byte_t t_bytes[4096];
        if (!MCStreamRead(In(t_input), t_bytes, t_to_read))
            return;
        
        if (!MCStreamWrite(In(t_protected_output), t_bytes, t_to_read))
            return;
        
        t_bytes_available -= t_to_read;
    }
    
    // Fetch the content of the output stream as a dataref.
    MCAutoDataRef t_buffer;
    if (!MCMemoryOutputStreamFinishAsDataRef(In(t_output), Out(t_buffer)))
        return;

    // Attempt to save the dataref to the original file.
    if (!MCS_savebinaryfile(p_filename, In(t_buffer)))
    {
        if (!MCErrorIsPending())
            MCErrorThrowGeneric(MCSTR("module file is not writable"));
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////

struct MCDeployBuilderTrustParameters
{
    MCStringRef package_file;
    MCStringRef certificate_file;
    MCStringRef key_file;
    MCStringRef passphrase;
};

struct zip_eocd_t
{
    uint32_t magic;
    uint16_t disc_number;
    uint16_t first_disc;
    uint16_t this_record_count;
    uint16_t total_record_count;
    uint32_t cd_byte_size;
    uint32_t cd_byte_offset;
    uint16_t comment_byte_size;
};

#define ZIP_EOCD_BYTE_SIZE 22
#define ZIP_EOCD_COMMENT_BYTE_SIZE_OFFSET (ZIP_EOCD_BYTE_SIZE - sizeof(uint16_t))

static bool __read_at_offset_from_end(IO_handle p_stream, int64_t p_offset, byte_t *p_buffer, size_t p_buffer_size, bool& r_out_of_range)
{
    int64_t t_fsize;
    t_fsize = MCS_fsize(p_stream);
    if (MCAbs(p_offset) > t_fsize)
    {
        r_out_of_range = true;
        return true;
    }
    
    if (MCS_seek_end(p_stream, p_offset) != IO_NORMAL ||
        MCS_readfixed(p_buffer, p_buffer_size, p_stream) != IO_NORMAL)
    {
        if (!MCErrorIsPending())
            MCErrorThrowGeneric(MCSTR("io error"));
        return false;
    }
    
    return true;
}

// This method looks at the ZIP file in the given stream and returns the offset
// of the comment field *if it can be used for a whole-file signature*.
//
// Only ZIP files which either have an empty comment field; or ZIP files which
// have been previously signed with a whole-file signature are suitable.
//
// If the ZIP file is suitable then r_offset is set to the offset within the file
// from the start of the stream.
//
// If the ZIP file is not suitable, then r_offset is set to 0.
//
// If an error occurs whilst processing the stream, false is returned.
//
static bool MCDeployBuilderFindOffsetForSignatureCommentInZip(IO_handle p_stream, int64_t& r_offset)
{
    bool t_out_of_range;
    t_out_of_range = false;

    // If the file is a commentless ZIP archive then the EOCD will be 22 bytes
    // from the end of the file, meaning that we will find the byte sequence
    // 0x50, 0x4b, 0x05, 0x06 there. Additionally, the final two bytes of the file
    // will by 0x00, 0x00 (zero comment length).
    byte_t t_eocd_magic[4];
    if (!__read_at_offset_from_end(p_stream, -ZIP_EOCD_BYTE_SIZE, t_eocd_magic, 4, t_out_of_range))
        return false;
    
    // If the field was read successfully and is the magic byte sequence then this
    // is a commentless archive and so suitable. The offset is -2 since the comment
    // length is the last two bytes of the archive.
    if (!t_out_of_range &&
        memcmp(t_eocd_magic, "\x50\x4b\x05\x06", 4) == 0)
    {
        r_offset = sizeof(uint16_t);
        return true;
    }
    
    // If the file is a previously signed ZIP archive then we take:
    //   sig_comment_size = zip[-2:-1]
    //   sig_offset = zip[-6:-5]
    //   sig_pad = zip[-4:-3]
    //   eocd_comment_size = zip[-sig_comment_size-2:-sig_comment_size-1]
    //   eocd_header = zip[-sig_comment_size-22:-sig_comment_size-21]
    //   message_terminator = zip[-sig_offset:-sig_offset]
    // With conditions:
    //   sig_comment_size == eocd_comment_size
    //   sig_offset < sig_comment_size
    //   sig_pad = 0xffff
    //   message_terminator == '\0'
    //   eocd_header == 0x50, 0x4b, 0x05, 0x06
    //
    
    byte_t t_sig_fields[6];
    if (!__read_at_offset_from_end(p_stream, -6, t_sig_fields, sizeof(t_sig_fields), t_out_of_range))
        return false;
        
    int32_t t_sig_comment_size, t_sig_pad, t_sig_offset;
    t_sig_comment_size = (t_sig_fields[4] << 0) | (t_sig_fields[5] << 8);
    t_sig_pad = (t_sig_fields[2] << 0) | (t_sig_fields[3] << 8);
    t_sig_offset = (t_sig_fields[0] << 0) | (t_sig_fields[1] << 8);
    
    byte_t t_message_terminator;
    if (!__read_at_offset_from_end(p_stream, -t_sig_offset - 1, &t_message_terminator, 1, t_out_of_range))
        return false;
    
    byte_t t_eocd_bytes[ZIP_EOCD_BYTE_SIZE];
    if (!__read_at_offset_from_end(p_stream, -t_sig_comment_size - ZIP_EOCD_BYTE_SIZE, t_eocd_bytes, sizeof(t_eocd_bytes), t_out_of_range))
        return false;

    byte_t *t_eocd_header;
    t_eocd_header = t_eocd_bytes;
    
    uint32_t t_eocd_comment_size;
    t_eocd_comment_size = t_eocd_bytes[20] | (t_eocd_bytes[21] << 8);
    
    // If this fails to satisfy the constraints for a signed archive, then it is
    // not suitable.
    if (t_out_of_range ||
        t_sig_comment_size != t_eocd_comment_size ||
        t_sig_offset >= t_sig_comment_size ||
        t_sig_pad != 0xffff ||
        t_message_terminator != 0 ||
        memcmp(t_eocd_header, "\x50\x4b\x05\x06", 4) != 0)
    {
        r_offset = -1;
        return true;
    }
    
    r_offset = t_eocd_comment_size + sizeof(uint16_t);
    
    return true;
}

static bool MCDeployBuilderOpenZipAtComment(MCStringRef p_filename, MCOpenFileMode p_mode, IO_handle& r_stream)
{
    // If the file does not exist - it is an error.
    if (!MCS_exists(p_filename, True))
    {
        if (!MCErrorIsPending())
            MCErrorThrowGeneric(MCSTR("package file not found"));
        return false;
    }
    
    // Try to open the file - if we cannot it is an error.
    IO_handle t_stream;
    t_stream = MCS_open(p_filename, p_mode, False, False, 0);
    if (t_stream == nil)
    {
        if (!MCErrorIsPending())
            MCErrorThrowGeneric(MCSTR("could not open package file"));
        return false;
    }
    
    // Find the offset of a suitable (comment size, comment) field in the zip.
    int64_t t_offset;
    if (!MCDeployBuilderFindOffsetForSignatureCommentInZip(t_stream, t_offset))
        goto error_exit;
    
    if (t_offset < 0)
    {
        MCErrorThrowGeneric(MCSTR("package file is not suitable for signing"));
        goto error_exit;
    }
    
    if (MCS_seek_end(t_stream, -t_offset) != IO_NORMAL)
    {
        if (!MCErrorIsPending())
            MCErrorThrowGeneric(MCSTR("io error"));
        goto error_exit;
    }
    
    r_stream = t_stream;
    
    return true;
    
error_exit:
    MCS_close(t_stream);
    return false;
}
                                            
// The 'sign' builder command signs the given package zip using the specified
// certificate.
void MCDeployBuilderSign(MCDeployBuilderTrustParameters& p_params)
{
    IO_handle t_stream;
    if (!MCDeployBuilderOpenZipAtComment(p_params . package_file, kMCOpenFileModeUpdate, t_stream))
        return;
    
    const char *t_message;
    size_t t_message_length;
    t_message = "signed by LiveCode";
    t_message_length = strlen(t_message);
    
    uint8_t *t_signature;
    size_t t_signature_length;
    t_signature = NULL;
    t_signature_length = 0;
    
    // We append 6 bytes to the end of the signature (offset:u2, pad:u1, pad:u1, length:u2).
    // So the offset from the end to the start of it is (siglength + 6)
    uint32_t t_offset_to_signature;
    t_offset_to_signature = t_signature_length + 6;
    
    // The total comment length is the message + a NUL byte terminator + the signature +
    // the 6 extra bytes.
    uint32_t t_comment_length;
    t_comment_length = t_message_length + 1 + t_signature_length + 6;
    
    // The encoded form of all the values we write out at the offset.
    const char *t_enc_message;
    uint8_t t_enc_terminator;
    uint8_t *t_enc_signature;
    uint8_t t_enc_offset_to_signature[2];
    uint8_t t_enc_pad[2];
    uint8_t t_enc_comment_length[2];
    t_enc_message = t_message;
    t_enc_terminator = 0;
    t_enc_signature = t_signature;
    t_signature_length = 0;
    t_enc_offset_to_signature[0] = t_offset_to_signature & 0xFF;
    t_enc_offset_to_signature[1] = (t_offset_to_signature >> 8) & 0xFF;
    t_enc_comment_length[0] = t_comment_length & 0xFF;
    t_enc_comment_length[1] = (t_comment_length >> 8) & 0xFF;
    t_enc_pad[0] = 0xFF;
    t_enc_pad[1] = 0xFF;
    
    // Our stream is at the offset of the EOCD comment length field. So we write out:
    //   enc_comment_length
    //   enc_message[message_length]
    //   enc_signature[signature_length]
    //   enc_offset_to_signature
    //   enc_pad
    //   enc_comment_length
    if (MCS_write(t_enc_comment_length, sizeof(t_enc_comment_length), 1, t_stream) == IO_ERROR ||
        MCS_write(t_enc_message, 1, t_message_length, t_stream) == IO_ERROR ||
        MCS_write(&t_enc_terminator, sizeof(t_enc_terminator), 1, t_stream) == IO_ERROR ||
        MCS_write(t_enc_signature, 1, t_signature_length, t_stream) == IO_ERROR ||
        MCS_write(t_enc_offset_to_signature, sizeof(t_enc_offset_to_signature), 1, t_stream) == IO_ERROR ||
        MCS_write(t_enc_pad, sizeof(t_enc_pad), 1, t_stream) == IO_ERROR ||
        MCS_write(t_enc_comment_length, sizeof(t_enc_comment_length), 1, t_stream) == IO_ERROR ||
        MCS_trunc(t_stream) == IO_ERROR)
    {
        if (!MCErrorIsPending())
            MCErrorThrowGeneric(MCSTR("could not update package file with new signature"));
    }
    
    MCS_close(t_stream);
}

// The 'check' builder command checks the given package zip has been signed
// with the given certificate.
void MCDeployBuilderCheck(MCDeployBuilderTrustParameters& p_params)
{
    
}

////////////////////////////////////////////////////////////////////////////////

MCIdeBuilderValidate::MCIdeBuilderValidate(void)
{
	m_module_file = NULL;
}

MCIdeBuilderValidate::~MCIdeBuilderValidate(void)
{
	delete m_module_file;
}

Parse_stat MCIdeBuilderValidate::parse(MCScriptPoint& sp)
{
	if (sp . parseexp(False, True, &m_module_file) != PS_NORMAL)
		return PS_ERROR;
	return PS_NORMAL;
}

void MCIdeBuilderValidate::exec_ctxt(MCExecContext& ctxt)
{
    ctxt . SetTheResultToEmpty();
    
    MCAutoStringRef t_module_file;
    if (!ctxt . EvalExprAsStringRef(m_module_file, EE_UNDEFINED, &t_module_file))
        return;
    
    MCDeployBuilderValidate(*t_module_file);
    
    MCAutoErrorRef t_error;
    if (MCErrorCatch(&t_error))
        ctxt . SetTheResultToValue(MCErrorGetMessage(*t_error));
}

/////////

MCIdeBuilderProtect::MCIdeBuilderProtect(void)
{
	m_module_file = NULL;
}

MCIdeBuilderProtect::~MCIdeBuilderProtect(void)
{
	delete m_module_file;
}

Parse_stat MCIdeBuilderProtect::parse(MCScriptPoint& sp)
{
	if (sp . parseexp(False, True, &m_module_file) != PS_NORMAL)
		return PS_ERROR;
	return PS_NORMAL;
}

void MCIdeBuilderProtect::exec_ctxt(MCExecContext& ctxt)
{
    ctxt . SetTheResultToEmpty();
    
    MCAutoStringRef t_module_file;
    if (!ctxt . EvalExprAsStringRef(m_module_file, EE_UNDEFINED, &t_module_file))
        return;
    
    MCDeployBuilderProtect(*t_module_file);
    
    MCAutoErrorRef t_error;
    if (MCErrorCatch(&t_error))
        ctxt . SetTheResultToValue(MCErrorGetMessage(*t_error));
}

/////////

MCIdeBuilderSign::MCIdeBuilderSign(void)
{
	m_params = NULL;
}

MCIdeBuilderSign::~MCIdeBuilderSign(void)
{
	delete m_params;
}

Parse_stat MCIdeBuilderSign::parse(MCScriptPoint& sp)
{
	return sp . parseexp(False, True, &m_params);
}

void MCIdeBuilderSign::exec_ctxt(MCExecContext& ctxt)
{
}

/////////

MCIdeBuilderCheck::MCIdeBuilderCheck(void)
{
	m_params = NULL;
}

MCIdeBuilderCheck::~MCIdeBuilderCheck(void)
{
	delete m_params;
}

Parse_stat MCIdeBuilderCheck::parse(MCScriptPoint& sp)
{
	return sp . parseexp(False, True, &m_params);
}

void MCIdeBuilderCheck::exec_ctxt(MCExecContext& ctxt)
{
}

//////////
