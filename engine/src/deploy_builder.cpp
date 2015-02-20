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

#define kMCDeployBuilderSignatureSize 256
#define kMCDeployBuilderWithoutSignatureOffset (ZIP_EOCD_BYTE_SIZE)
#define kMCDeployBuilderWithSignatureOffset (kMCDeployBuilderSignatureSize + ZIP_EOCD_BYTE_SIZE)

static bool MCDeployBuilderFindCommentWithEOCDAtOffset(IO_handle p_stream, int64_t p_offset)
{
    // Seek to where we should see the EOCD.
    if (MCS_seek_end(p_stream, -p_offset) != IO_NORMAL)
        return false;
    
    // Read the magic four bytes.
    char t_magic[4];
    if (MCS_readfixed(t_magic, 4, p_stream) != IO_NORMAL)
        return false;
    
    // If the magic doesn't match, the EOCD is not here.
    if (memcmp(t_magic, "\x50\x4b\x05\x06", 4) != 0)
        return false;
    
    // Seek to the start of the (comment size, comment string) pait.
    if (MCS_seek_end(p_stream, -p_offset + ZIP_EOCD_COMMENT_BYTE_SIZE_OFFSET) != IO_NORMAL)
        return false;
    
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
    
    // We assume that either the zip file has an empty comment section; or
    // it has one of the size of signature.
    if (!MCDeployBuilderFindCommentWithEOCDAtOffset(t_stream, kMCDeployBuilderWithoutSignatureOffset) &&
        !MCDeployBuilderFindCommentWithEOCDAtOffset(t_stream, kMCDeployBuilderWithSignatureOffset))
    {
        if (!MCErrorIsPending())
            MCErrorThrowGeneric(MCSTR("package file is not suitable for signing"));
        
        MCS_close(t_stream);
        
        return false;
    }
    
    r_stream = t_stream;
    
    return true;
}
                                            
// The 'sign' builder command signs the given package zip using the specified
// certificate.
void MCDeployBuilderSign(MCDeployBuilderTrustParameters& p_params)
{
    IO_handle t_stream;
    if (!MCDeployBuilderOpenZipAtComment(p_params . package_file, kMCOpenFileModeUpdate, t_stream))
        return;
    
    char t_signature[256];
    for(int i = 0; i < 256; i++)
        t_signature[i] = i;
    
    byte_t t_length[2];
    t_length[0] = 0;
    t_length[1] = 1;
    if (MCS_write(t_length, 1, 2, t_stream) == IO_ERROR ||
        MCS_write(t_signature, 1, 256, t_stream) == IO_ERROR)
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
