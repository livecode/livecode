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

#ifndef _SECUREMODE_H
#define _SECUREMODE_H

// AL-2014-10-29: [[ Bug 13704 ]] Define security permissions bits
enum MCSecureModeType
{
    kMCSecureModeTypeDiskBit,
    kMCSecureModeTypeNetworkBit,
    kMCSecureModeTypeProcessBit,
    kMCSecureModeTypeRegistryReadBit,
    kMCSecureModeTypeRegistryWriteBit,
    kMCSecureModeTypePrintBit,
    kMCSecureModeTypePrivacyBit,
    kMCSecureModeTypeApplescriptBit,
    kMCSecureModeTypeDoalternateBit,
    kMCSecureModeTypeExternalBit,
    kMCSecureModeTypeExtensionBit,
};

#define MC_SECUREMODE_DISK				(1 << kMCSecureModeTypeDiskBit)
#define MC_SECUREMODE_NETWORK			(1 << kMCSecureModeTypeNetworkBit)
#define MC_SECUREMODE_PROCESS			(1 << kMCSecureModeTypeProcessBit)
#define MC_SECUREMODE_REGISTRY_READ		(1 << kMCSecureModeTypeRegistryReadBit)
#define MC_SECUREMODE_REGISTRY_WRITE	(1 << kMCSecureModeTypeRegistryWriteBit)
#define MC_SECUREMODE_PRINT				(1 << kMCSecureModeTypePrintBit)
#define MC_SECUREMODE_PRIVACY			(1 << kMCSecureModeTypePrivacyBit)
#define MC_SECUREMODE_APPLESCRIPT		(1 << kMCSecureModeTypeApplescriptBit)
#define MC_SECUREMODE_DOALTERNATE		(1 << kMCSecureModeTypeDoalternateBit)
#define MC_SECUREMODE_EXTERNAL			(1 << kMCSecureModeTypeExternalBit)
#define MC_SECUREMODE_EXTENSION            (1 << kMCSecureModeTypeExtensionBit)

#define MC_SECUREMODE_ALL			(MC_SECUREMODE_DISK | MC_SECUREMODE_NETWORK | MC_SECUREMODE_PROCESS \
										| MC_SECUREMODE_REGISTRY_READ | MC_SECUREMODE_REGISTRY_WRITE \
										| MC_SECUREMODE_PRINT | MC_SECUREMODE_PRIVACY | MC_SECUREMODE_APPLESCRIPT \
										| MC_SECUREMODE_DOALTERNATE | MC_SECUREMODE_EXTERNAL | MC_SECUREMODE_EXTENSION)

#define MC_SECUREMODE_MODECOUNT		(11)

extern const char *MCsecuremode_strings[MC_SECUREMODE_MODECOUNT];

// These methods check the current state of the secureMode property.
//
// The 'Check' methods ensure that the given access is possible, and return
// false if it is not. In this case an error is added to the error stack.
//
// The 'CanAccess' methods just return false if a given access is not available,
// in particular they do not add an error to the error stack.

bool MCSecureModeCheckDisk(uint2 line = 0, uint2 pos = 0);
bool MCSecureModeCheckNetwork(uint2 line = 0, uint2 pos = 0);
bool MCSecureModeCheckProcess(uint2 line = 0, uint2 pos = 0);
bool MCSecureModeCheckShell(uint2 line = 0, uint2 pos = 0);
bool MCSecureModeCheckRegistryRead(uint2 line = 0, uint2 pos = 0);
bool MCSecureModeCheckRegistryWrite(uint2 line = 0, uint2 pos = 0);
bool MCSecureModeCheckPrinter(uint2 line = 0, uint2 pos = 0);
bool MCSecureModeCheckPrivacy(uint2 line = 0, uint2 pos = 0);
bool MCSecureModeCheckAppleScript(uint2 line = 0, uint2 pos = 0);
bool MCSecureModeCheckDoAlternate(uint2 line = 0, uint2 pos = 0);
bool MCSecureModeCheckExternal(uint2 line = 0, uint2 pos = 0);
bool MCSecureModeCheckExtension(uint2 line = 0, uint2 pos = 0);

bool MCSecureModeCanAccessDisk(void);
bool MCSecureModeCanAccessNetwork(void);
bool MCSecureModeCanAccessPrinter(void);
bool MCSecureModeCanAccessDoAlternate(void);
bool MCSecureModeCanAccessExternal(void);
bool MCSecureModeCanAccessExtension(void);

// MW-2013-08-07: [[ Bug 10865 ]] New check method for whether AppleScript is
//   enabled.
bool MCSecureModeCanAccessAppleScript(void);

#endif
