/* Copyright (C) 2016 LiveCode Ltd.
 
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

#ifndef __MC_STACKFILEFORMAT_H__
#define __MC_STACKFILEFORMAT_H__

#define kMCStackFileFormatVersion_1_0 (1000)
#define kMCStackFileFormatVersion_1_3 (1300)
#define kMCStackFileFormatVersion_1_4 (1400)
#define kMCStackFileFormatVersion_2_0 (2000)
#define kMCStackFileFormatVersion_2_3 (2300)
#define kMCStackFileFormatVersion_2_4 (2400)
#define kMCStackFileFormatVersion_2_7 (2700)
#define kMCStackFileFormatVersion_5_5 (5500)
#define kMCStackFileFormatVersion_7_0 (7000)
#define kMCStackFileFormatVersion_8_0 (8000)
#define kMCStackFileFormatVersion_8_1 (8100)

#define kMCStackFileFormatMinimumExportVersion kMCStackFileFormatVersion_2_4
#define kMCStackFileFormatCurrentVersion kMCStackFileFormatVersion_8_1


#define kMCStackFileVersionStringPrefix "REVO"
#define kMCStackFileVersionStringPrefixLength 4

#define kMCStackFileVersionString_2_7 "REVO2700"
#define kMCStackFileVersionString_5_5 "REVO5500"
#define kMCStackFileVersionString_7_0 "REVO7000"
#define kMCStackFileVersionString_8_0 "REVO8000"
#define kMCStackFileVersionString_8_1 "REVO8100"
#define kMCStackFileVersionStringLength 8

#define kMCStackFileMetaCardVersionString "#!/bin/sh\n# MetaCard 2.4 stack\n# The following is not ASCII text,\n# so now would be a good time to q out of more\f\nexec mc $0 \"$@\"\n"
#define kMCStackFileMetaCardVersionStringLength 255
#define kMCStackFileMetaCardSignature "# MetaCard "
#define kMCStackFileMetaCardSignatureLength 11

// parse the version number following the version string prefix
extern bool MCStackFileParseVersionNumber(const char *p_buffer, uint32_t &r_version);

// map the given version number to one of the supported output format versions
extern uint32_t MCStackFileMapToSupportedVersion(uint32_t p_version);

// return the version header string to use for a given version
extern void MCStackFileGetHeaderForVersion(uint32_t p_version, const char *&r_header, uint32_t &r_size);


#endif // __MC_STACKFILEFORMAT_H__
