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

#include "w32prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "w32dc.h"


////////////////////////////////////////////////////////////////////////////////

struct MCWindowsColorTransform
{
	HTRANSFORM transform;
	bool is_cmyk;
};

static void ciexy_to_xyz(double x, double y, CIEXYZ& r_xyz)
{
	r_xyz . ciexyzX = (uint32_t)((x / y) * (1 << 30));
	r_xyz . ciexyzY = (uint32_t)(1 * (1 << 30));
	r_xyz . ciexyzZ = (uint32_t)((1.0 - x - y) * (1 << 30));
}

MCColorTransformRef MCScreenDC::createcolortransform(const MCColorSpaceInfo& p_info)
{
	bool t_success;
	t_success = true;

	HTRANSFORM t_transform;
	t_transform = nil;
	bool t_is_cmyk;
	t_is_cmyk = false;
	if (p_info . type == kMCColorSpaceCalibratedRGB)
	{
		// TODO - This isn't quite right, disable for now
#if 0
		LOGCOLORSPACEA t_colorspace;
		t_colorspace . lcsSignature = LCS_SIGNATURE;
		t_colorspace . lcsVersion = 0x400;
		t_colorspace . lcsSize = sizeof(LOGCOLORSPACEA);
		t_colorspace . lcsCSType = LCS_CALIBRATED_RGB;
		t_colorspace . lcsIntent = LCS_GM_GRAPHICS;
		ciexy_to_xyz(p_info . calibrated . red_x, p_info . calibrated . red_y, t_colorspace . lcsEndpoints.ciexyzRed);
		ciexy_to_xyz(p_info . calibrated . green_x, p_info . calibrated . green_y, t_colorspace . lcsEndpoints.ciexyzGreen);
		ciexy_to_xyz(p_info . calibrated . blue_x, p_info . calibrated . blue_y, t_colorspace . lcsEndpoints.ciexyzBlue);
		t_colorspace . lcsGammaRed = (uint32_t)(p_info . calibrated . gamma * 0x10000);
		t_colorspace . lcsGammaGreen = (uint32_t)(p_info . calibrated . gamma * 0x10000);
		t_colorspace . lcsGammaBlue = (uint32_t)(p_info . calibrated . gamma * 0x10000);
		t_colorspace . lcsFilename[0] = '\0';
		t_transform = CreateColorTransformA(&t_colorspace, m_srgb_profile, nil, 0);
#endif
	}
	else if (p_info . type == kMCColorSpaceStandardRGB)
	{
		// TODO - This isn't quite right, disable for now
#if 0
		HPROFILE t_profiles[2];
		t_profiles[0] = m_srgb_profile;
		t_profiles[1] = m_srgb_profile;

		DWORD t_intents[1];
		t_intents[0] = (DWORD)p_info . standard . intent;

		t_transform = CreateMultiProfileTransform(t_profiles, 2, t_intents, 1, 0, 0);
#endif

		t_transform = nil;
	}
	else if (p_info . type == kMCColorSpaceEmbedded)
	{
		PROFILE t_profile_info;
		t_profile_info . dwType = PROFILE_MEMBUFFER;
		t_profile_info . pProfileData = p_info . embedded . data;
		t_profile_info . cbDataSize = p_info . embedded . data_size;

		HPROFILE t_src_profile;
		t_src_profile = OpenColorProfileA(&t_profile_info, PROFILE_READ, FILE_SHARE_READ, OPEN_EXISTING);
		if (t_src_profile == nil)
			t_success = false;

		PROFILEHEADER t_src_header;
		if (t_success)
			if (!GetColorProfileHeader(t_src_profile, &t_src_header))
				t_success = false;

		if (t_success)
		{
			t_is_cmyk = (t_src_header . phDataColorSpace == SPACE_CMYK);

			HPROFILE t_profiles[2];
			t_profiles[0] = t_src_profile;
			t_profiles[1] = m_srgb_profile;

			DWORD t_intents[1];
			t_intents[0] = INTENT_RELATIVE_COLORIMETRIC;

			t_transform = CreateMultiProfileTransform(t_profiles, 2, t_intents, 1, 0, 0);
			if (t_transform == nil)
				t_success = false;
		}

		if (t_src_profile != nil)
			CloseColorProfile(t_src_profile);
	}


	MCWindowsColorTransform *t_colorxform;
	t_colorxform = nil;
	if (t_success)
		t_success = MCMemoryNew(t_colorxform);

	if (t_success)
	{
		t_colorxform -> transform = t_transform;
		t_colorxform -> is_cmyk = t_is_cmyk;
	}
	else
	{
		destroycolortransform(t_colorxform);
		t_colorxform = nil;
	}

	return t_colorxform;
}

void MCScreenDC::destroycolortransform(MCColorTransformRef transform)
{
	MCWindowsColorTransform *t_transform;
	t_transform = (MCWindowsColorTransform *)transform;

	if (t_transform == nil)
		return;

	if (t_transform -> transform != nil)
		DeleteColorTransform(t_transform -> transform);

	MCMemoryDelete(t_transform);
}

bool MCScreenDC::transformimagecolors(MCColorTransformRef transform, MCImageBitmap *p_image)
{
	MCWindowsColorTransform *t_transform;
	t_transform = (MCWindowsColorTransform *)transform;

	bool t_success;
	t_success = true;

	if (t_success)
	{
		// If the src color space is of CMYK type, then we assume the input data
		// is in that colorspace.
		BMFORMAT t_input_format;
		if (t_transform -> is_cmyk)
			t_input_format = BM_KYMCQUADS;
		else
			t_input_format = BM_xRGBQUADS;

		if (t_transform != nil &&
			!TranslateBitmapBits(t_transform -> transform, p_image -> data, t_input_format, p_image -> width, p_image -> height, p_image -> stride, p_image -> data, BM_xRGBQUADS, p_image -> stride, nil, INDEX_DONT_CARE))
			t_success = false;
	}

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
