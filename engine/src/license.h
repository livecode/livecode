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

////////////////////////////////////////////////////////////////////////////////
//
//  Private Header File:
//    license.h
//
//  Description:
//    This file contains the definitions for things relating to licensing mode.
//
//  Changes:
//    2009-07-01 Updated to use new edition/class enum values.
//               Removed obsolete licensing parameters.
//               Added externals/enable_externals licensing parameters.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __LICENSE_H
#define __LICENSE_H

enum
{
	kMCLicenseDeployToWindows = 1 << 0,
	kMCLicenseDeployToMacOSX = 1 << 1,
	kMCLicenseDeployToLinux = 1 << 2,
	kMCLicenseDeployToIOS = 1 << 3,
	kMCLicenseDeployToAndroid = 1 << 4,
	kMCLicenseDeployToWinMobile = 1 << 5,
	kMCLicenseDeployToLinuxMobile = 1 << 6,
	kMCLicenseDeployToServer = 1 << 7,
	kMCLicenseDeployToIOSEmbedded = 1 << 8,
	kMCLicenseDeployToAndroidEmbedded = 1 << 9,
    kMCLicenseDeployToHTML5 = 1 << 10,
    kMCLicenseDeployToFileMaker = 1 << 11,
};

enum
{
	kMCLicenseClassNone,
	kMCLicenseClassCommunity,
	kMCLicenseClassEvaluation,
	kMCLicenseClassCommercial,
	kMCLicenseClassProfessionalEvaluation,
	kMCLicenseClassProfessional,
};

struct MCLicenseParameters
{
    MCStringRef license_token;
    MCStringRef license_name;
    MCStringRef license_organization;
	uint32_t license_class;
	uint4 license_multiplicity;

	uint4 script_limit;
	uint4 do_limit;
	uint4 using_limit;
	uint4 insert_limit;
	
	uint32_t deploy_targets;

	MCArrayRef addons;
};

extern MCLicenseParameters MClicenseparameters;
extern Boolean MCenvironmentactive;

#endif
