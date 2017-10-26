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


#include "handler.h"
#include "scriptpt.h"
#include "variable.h"
#include "statemnt.h"
#include "globals.h"

#include "ide.h"
#include "deploy.h"
#include "mode.h"
#include "license.h"

#include "capsule.h"

#include "mcio.h"

#ifdef _WINDOWS_DESKTOP
#include <io.h>
#elif defined(_MAC_DESKTOP) || defined(_LINUX_DESKTOP)
#include <unistd.h>
#endif

////////////////////////////////////////////////////////////////////////////////

void MCDeployByteSwap32(bool p_to_network, uint32_t& x)
{
#ifdef __BIG_ENDIAN__
	if (p_to_network)
		return;
#else
	if (!p_to_network)
		return;
#endif
	
	x = ((x >> 24) & 0xff) | ((x >> 8) & 0xff00) | ((x & 0xff00) << 8) | ((x & 0xff) << 24);
}

void MCDeployByteSwapRecord(bool p_to_network, const char *f, void *p, uint32_t s)
{
#ifdef __BIG_ENDIAN__
	if (p_to_network)
		return;
#else
	if (!p_to_network)
		return;
#endif
	
	while(*f != '\0' && s > 0)
	{
		uint32_t n;
		if (*f == 'b')
			n = 1;
		else if (*f == 's')
		{
			if (s < 2)
				break;
			n = 2;
			
			uint16_t x;
			x = *(uint16_t *)p;
			x = ((x & 0xff) << 8) | ((x >> 8) & 0xff);
			*(uint16_t *)p = x;
			
		}
		else if (*f == 'l')
		{
			if (s < 4)
				break;
			n = 4;
			
			uint32_t x;
			x = *(uint32_t *)p;
			x = ((x >> 24) & 0xff) | ((x >> 8) & 0xff00) | ((x & 0xff00) << 8) | ((x & 0xff) << 24);
			*(uint32_t *)p = x;
		}
		else if (*f == 'q')
		{
			if (s < 8)
				break;
			n = 8;

			uint64_t x;
			x = *(uint64_t *)p;
			x = ((x >> 56) & 0xff) | ((x >> 40) & 0xff00) | ((x >> 24) & 0xff0000) | ((x >> 8) & 0xff000000) |
				((x & 0xff000000) << 8) | ((x & 0xff0000) << 24) | ((x & 0xff00) << 40) | ((x & 0xff) << 56);
			*(uint64_t *)p = x;
		}
		else
			n = 0;
		
		p = (uint8_t *)p + n;
		f += 1;	
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCDeployFileOpen(MCStringRef p_path, intenum_t p_mode, MCDeployFileRef& r_file)
{
	bool t_success;
	t_success = true;
	
	if (MCStringIsEmpty(p_path))
		return false;
    
    IO_handle t_handle ;
    if (p_mode == kMCOpenFileModeCreate)
        t_handle = MCS_deploy_open(p_path, p_mode);
    else
        t_handle = MCS_open(p_path, p_mode, false, false, 0);
	
	t_success = (t_handle != nil);
	
	if (t_success)
		r_file = t_handle;
	
	return t_success;
}

void MCDeployFileClose(MCDeployFileRef p_file)
{
	if (p_file != nil)
		MCS_close(p_file);
}

bool MCDeployFileRead(MCDeployFileRef p_file, void *p_buffer, uint32_t p_buffer_size)
{
	if (MCS_readfixed(p_buffer, p_buffer_size, p_file) != IO_NORMAL)
		return false;
	
	return true;
}

bool MCDeployFileReadAt(MCDeployFileRef p_file, void *p_buffer, uint32_t p_buffer_size, uint32_t p_at)
{
	if (MCS_seek_set(p_file, p_at) != IO_NORMAL)
		return MCDeployThrow(kMCDeployErrorBadRead);
	
	// MW-2012-10-12: [[ Bug ]] If buffer size is 0 then don't attempt to read,
	//   (as there is nothing to read!).
	if (p_buffer_size != 0 && MCS_readfixed(p_buffer, p_buffer_size, p_file) != IO_NORMAL)
		return MCDeployThrow(kMCDeployErrorBadRead);
	
	return true;
}

bool MCDeployFileSeekSet(MCDeployFileRef p_file, long p_offset)
{
	if (MCS_seek_set(p_file, p_offset) != IO_NORMAL)
		return false;
	
	return true;
}

bool MCDeployFileCopy(MCDeployFileRef p_dst, uint32_t p_at, MCDeployFileRef p_src, uint32_t p_from, uint32_t p_amount)
{
	if (MCS_seek_set(p_src, p_from) != IO_NORMAL)
		return MCDeployThrow(kMCDeployErrorBadRead);
    
    if (MCS_seek_set(p_dst, p_at) != IO_NORMAL)
		return MCDeployThrow(kMCDeployErrorBadWrite);
    
	while(p_amount > 0)
	{
		char t_buffer[4096];
		uint32_t t_size;
		t_size = MCU_min(4096U, p_amount);
		if (MCS_readfixed(t_buffer, t_size, p_src) != IO_NORMAL)
			return MCDeployThrow(kMCDeployErrorBadRead);
		if (MCS_write(t_buffer, t_size, 1, p_dst) != IO_NORMAL)
			return MCDeployThrow(kMCDeployErrorBadWrite);
		p_amount -= t_size;
	}
	
	return true;
}

bool MCDeployFileWriteAt(MCDeployFileRef p_dst, const void *p_buffer, uint32_t p_size, uint32_t p_at)
{
	if (MCS_seek_set(p_dst, p_at) != IO_NORMAL)
		return MCDeployThrow(kMCDeployErrorBadWrite);
	
	if (MCS_write(p_buffer, p_size, 1, p_dst) != IO_NORMAL)
		return MCDeployThrow(kMCDeployErrorBadWrite);
    
	return true;
}

bool MCDeployFileMeasure(MCDeployFileRef p_file, uint32_t& r_size)
{
	r_size = MCS_fsize(p_file);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static MCDeployError s_deploy_last_error;

bool MCDeployThrow(MCDeployError status)
{
	s_deploy_last_error = status;
	return false;
}

MCDeployError MCDeployCatch(void)
{
	MCDeployError t_result;
	t_result = s_deploy_last_error;
	s_deploy_last_error = kMCDeployErrorNone;
	return t_result;
}

const char *MCDeployErrorToString(MCDeployError p_error)
{
	switch(p_error)
	{
		case kMCDeployErrorNone:
			return "";
		case kMCDeployErrorNoMemory:
			return "out of memory";
		case kMCDeployErrorNoEngine:
			return "could not open standalone engine file";
		case kMCDeployErrorNoStackfile:
			return "could not open stackfile";
		case kMCDeployErrorNoAuxStackfile:
			return "could not open auxiliary stackfile";
		case kMCDeployErrorNoOutput:
			return "could not open output file";
		case kMCDeployErrorNoSpill:
			return "could not open spill file";
		case kMCDeployErrorNoPayload:
			return "could not open payload file";
        case kMCDeployErrorNoModule:
            return "could not open module file";
		case kMCDeployErrorBadFile:
		case kMCDeployErrorBadRead:
		case kMCDeployErrorBadWrite:
		case kMCDeployErrorBadCompress:
			return "i/o error";
			
		case kMCDeployErrorWindowsNoDOSHeader:
		case kMCDeployErrorWindowsBadDOSSignature:
		case kMCDeployErrorWindowsBadDOSHeader:
		case kMCDeployErrorWindowsNoNTHeader:
		case kMCDeployErrorWindowsBadNTSignature:
		case kMCDeployErrorWindowsBadSectionHeaderOffset:
		case kMCDeployErrorWindowsNoSectionHeaders:
		case kMCDeployErrorWindowsMissingSections:
		case kMCDeployErrorWindowsNoResourceSection:
		case kMCDeployErrorWindowsNoProjectSection:
			return "invalid windows standalone engine file";

		case kMCDeployErrorWindowsNoPayloadSection:
			return "invalid windows installer engine file";
			
		case kMCDeployErrorWindowsBadAppIcon:
			return "could not load app icon";
		case kMCDeployErrorWindowsBadDocIcon:
			return "could not load doc icon";
		case kMCDeployErrorWindowsBadManifest:
			return "could not load manifest";
			
		case kMCDeployErrorLinuxNoHeader:
		case kMCDeployErrorLinuxBadHeaderMagic:
		case kMCDeployErrorLinuxBadHeaderType:
		case kMCDeployErrorLinuxBadImage:
		case kMCDeployErrorLinuxBadSectionSize:
		case kMCDeployErrorLinuxBadSectionTable:
		case kMCDeployErrorLinuxBadSegmentSize:
		case kMCDeployErrorLinuxBadProgramTable:
		case kMCDeployErrorLinuxBadStringIndex:
		case kMCDeployErrorLinuxBadString:
		case kMCDeployErrorLinuxNoProjectSection:
		case kMCDeployErrorLinuxNoPayloadSection:
		case kMCDeployErrorLinuxBadSectionOrder:
		case kMCDeployErrorLinuxNoProjectSegment:
		case kMCDeployErrorLinuxPayloadNotInProjectSegment:
			return "invalid linux standalone engine file";
			
		case kMCDeployErrorMacOSXNoHeader:
		case kMCDeployErrorMacOSXBadHeader:
		case kMCDeployErrorMacOSXBadCommand:
		case kMCDeployErrorMacOSXNoLinkEditSegment:
		case kMCDeployErrorMacOSXNoProjectSegment:
		case kMCDeployErrorMacOSXBadSegmentOrder:
		case kMCDeployErrorMacOSXUnknownLoadCommand:
		case kMCDeployErrorMacOSXBadCpuType:
		case kMCDeployErrorMacOSXBadTarget:
			return "invalid mac/ios standalone engine file";

		case kMCDeployErrorEmscriptenBadStack:
			return "could not prepare startup stack";
		
		case kMCDeployErrorTrialBannerError:
			return "could not create trial banner";
            
        case kMCDeployErrorInvalidUuid:
            return "invalid uuid";

		case kMCDeployErrorNoCertificate:
			return "could not load certificate";
		case kMCDeployErrorBadCertificate:
			return "could not decode certificate";
		case kMCDeployErrorEmptyCertificate:
			return "certificate chain empty";

		case kMCDeployErrorNoPrivateKey:
			return "could not load private key";
		case kMCDeployErrorBadPrivateKey:
			return "could not decode private key";

		case kMCDeployErrorCertMismatch:
			return "private key does not match certificate";
		case kMCDeployErrorNoPassword:
			return "no password specified";
		case kMCDeployErrorBadPassword:
			return "bad password specified";

		case kMCDeployErrorBadSignature:
		case kMCDeployErrorBadString:
		case kMCDeployErrorBadHash:
			return "signature build error";

		case kMCDeployErrorTimestampFailed:
			return "unable to contact timestamping authority";
		case kMCDeployErrorBadTimestamp:
			return "invalid timestamp returned";

		case kMCDeployErrorNoArchs:
			return "no architectures left";

		default:
			break;
	}
	
	return "unknown error";
}

