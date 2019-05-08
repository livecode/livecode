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

#include "mcerror.h"

#include "dispatch.h"
#include "stack.h"
#include "globals.h"
#include "securemode.h"

const char *MCsecuremode_strings[MC_SECUREMODE_MODECOUNT] = {
	"disk",
	"network",
	"process",
	"registryRead",
	"registryWrite",
	"printing",
	"privacy", 
	"applescript",
	"doalternate",
	"external"
};

bool MCSecureModeCanAccessDisk(void)
{
	return ((MCsecuremode & MC_SECUREMODE_DISK) == 0);
}

bool MCSecureModeCheckDisk(uint2 line, uint2 pos)
{
	if ((MCsecuremode & MC_SECUREMODE_DISK) == 0)
		return true;

	MCeerror -> add(EE_DISK_NOPERM, line, pos);

	return false;
}

bool MCSecureModeCanAccessPrinter(void)
{
	return ((MCsecuremode & MC_SECUREMODE_PRINT) == 0);
}

// MW-2013-08-07: [[ Bug 10865 ]] New check method for whether AppleScript is
//   enabled.
bool MCSecureModeCanAccessAppleScript(void)
{
	return ((MCsecuremode & MC_SECUREMODE_APPLESCRIPT) == 0);
}

bool MCSecureModeCheckPrinter(uint2 line, uint2 pos)
{
	if ((MCsecuremode & MC_SECUREMODE_PRINT) == 0)
		return true;

	MCeerror -> add(EE_PRINT_NOPERM, line, pos);

	return false;
}

bool MCSecureModeCanAccessNetwork(void)
{
	return ((MCsecuremode & MC_SECUREMODE_NETWORK) == 0);
}

bool MCSecureModeCheckNetwork(uint2 line, uint2 pos)
{
	if ((MCsecuremode & MC_SECUREMODE_NETWORK) == 0)
		return true;

	MCeerror -> add(EE_NETWORK_NOPERM, line, pos);

	return false;
}

bool MCSecureModeCheckProcess(uint2 line, uint2 pos)
{
	if ((MCsecuremode & MC_SECUREMODE_PROCESS) == 0)
		return true;

	MCeerror -> add(EE_PROCESS_NOPERM, line, pos);

	return false;
}

bool MCSecureModeCheckAppleScript(uint2 line, uint2 pos)
{
	if ((MCsecuremode & MC_SECUREMODE_APPLESCRIPT) == 0)
		return true;
		
	MCeerror -> add(EE_SECUREMODE_APPLESCRIPT_NOPERM, line, pos);
	
	return false;
}

bool MCSecureModeCanAccessDoAlternate(void)
{
	return ((MCsecuremode & MC_SECUREMODE_DOALTERNATE) == 0);
}

bool MCSecureModeCheckDoAlternate(uint2 line, uint2 pos)
{
	if ((MCsecuremode & MC_SECUREMODE_DOALTERNATE) == 0)
		return true;
		
	MCeerror -> add(EE_SECUREMODE_DOALTERNATE_NOPERM, line, pos);
	
	return false;
}

bool MCSecureModeCanAccessExternal(void)
{
	return ((MCsecuremode & MC_SECUREMODE_EXTERNAL) == 0);
}

bool MCSecureModeCanAccessExtension(void)
{
    return ((MCsecuremode & MC_SECUREMODE_EXTENSION) == 0);
}

bool MCSecureModeCheckPrivacy(uint2 line, uint2 pos)
{
	if ((MCsecuremode & MC_SECUREMODE_PRIVACY) == 0)
		return true;

	MCeerror -> add(EE_PRIVACY_NOPERM, line, pos);

	return false;
}
