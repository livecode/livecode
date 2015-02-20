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

#include "execpt.h"
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

bool MCDeployFileOpen(const char *p_path, const char *p_mode, MCDeployFileRef& r_file)
{
	bool t_success;
	t_success = true;
	
	if (t_success)
		if (p_path == nil)
			t_success = false;
	
	// MW-2009-11-08: [[ Bug 8416 ]] Deploy not working when files in locations with
	//   accents in the path - this is due to (on OS X) not converting to UTF8.
	char *t_path;
	t_path = nil;
#if defined(_MACOSX)
	if (t_success)
		t_success = MCCStringFromNative(p_path, t_path);
#else
	if (t_success)
		t_success = MCCStringClone(p_path, t_path);
#endif
	
	FILE *t_file;
	t_file = nil;
	if (t_success)
	{
		t_file = fopen(t_path, p_mode);
		if (t_file == nil)
			t_success = false;
	}
	
	if (t_success)
		r_file = t_file;
	
	MCCStringFree(t_path);
	
	return t_success;
}

void MCDeployFileClose(MCDeployFileRef p_file)
{
	if (p_file != NULL)
		fclose(p_file);
}

bool MCDeployFileRead(MCDeployFileRef p_file, void *p_buffer, uint32_t p_buffer_size)
{
	if (fread(p_buffer, p_buffer_size, 1, p_file) != 1)
		return false;
	
	return true;
}

bool MCDeployFileReadAt(MCDeployFileRef p_file, void *p_buffer, uint32_t p_buffer_size, uint32_t p_at)
{
	if (fseek(p_file, p_at, SEEK_SET) != 0)
		return MCDeployThrow(kMCDeployErrorBadRead);
	
	// MW-2012-10-12: [[ Bug ]] If buffer size is 0 then don't attempt to read,
	//   (as there is nothing to read!).
	if (p_buffer_size != 0 && fread(p_buffer, p_buffer_size, 1, p_file) != 1)
		return MCDeployThrow(kMCDeployErrorBadRead);
	
	return true;
}

bool MCDeployFileSeek(MCDeployFileRef p_file, long p_offset, int p_origin)
{
	if (fseek(p_file, p_offset, p_origin) != 0)
		return false;
	
	return true;
}

bool MCDeployFileCopy(MCDeployFileRef p_dst, uint32_t p_at, MCDeployFileRef p_src, uint32_t p_from, uint32_t p_amount)
{
	if (fseek(p_src, p_from, SEEK_SET) != 0)
		return MCDeployThrow(kMCDeployErrorBadRead);
	
	if (fseek(p_dst, p_at, SEEK_SET) != 0)
		return MCDeployThrow(kMCDeployErrorBadWrite);
	
	while(p_amount > 0)
	{
		char t_buffer[4096];
		uint32_t t_size;
		t_size = MCU_min(4096U, p_amount);
		if (fread(t_buffer, t_size, 1, p_src) != 1)
			return MCDeployThrow(kMCDeployErrorBadRead);
		if (fwrite(t_buffer, t_size, 1, p_dst) != 1)
			return MCDeployThrow(kMCDeployErrorBadWrite);
		p_amount -= t_size;
	}
	
	return true;
}

bool MCDeployFileWriteAt(MCDeployFileRef p_dst, const void *p_buffer, uint32_t p_size, uint32_t p_at)
{
	if (fseek(p_dst, p_at, SEEK_SET) != 0)
		return MCDeployThrow(kMCDeployErrorBadWrite);
	
	if (fwrite(p_buffer, p_size, 1, p_dst) != 1)
		return MCDeployThrow(kMCDeployErrorBadWrite);
	
	return true;
}

bool MCDeployFileTruncate(MCDeployFileRef p_file, uint32_t p_size)
{
#ifdef _WINDOWS
	if (_chsize(_fileno(p_file), p_size) != 0)
		return false;
#else
	if (ftruncate(fileno(p_file), p_size) != 0)
		return false;
#endif
	return true;
}

bool MCDeployFileMeasure(MCDeployFileRef p_file, uint32_t& r_size)
{
	long t_pos;
	t_pos = ftell(p_file);
	
	if (fseek(p_file, 0, SEEK_END) != 0)
		return false;
	
	r_size = ftell(p_file);
	
	if (fseek(p_file, t_pos, SEEK_SET) != 0)
		return false;
	
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

