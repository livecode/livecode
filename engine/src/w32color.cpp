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

#include "w32dc.h"

#include "color.h"

////////////////////////////////////////////////////////////////////////////////

struct MCWindowsColorTransform
{
	HTRANSFORM transform;
	bool is_cmyk;
};

static inline uint32_t float_to_fixed2_30(float p_value)
{
	return (uint32_t)((1 << 30) * p_value);
}

static inline CIEXYZ CIEXYZMake(MCGFloat x, MCGFloat y, MCGFloat z)
{
	CIEXYZ t_cie;
	t_cie.ciexyzX = float_to_fixed2_30(x);
	t_cie.ciexyzY = float_to_fixed2_30(y);
	t_cie.ciexyzZ = float_to_fixed2_30(z);

	return t_cie;
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
		// IM-2014-09-26: [[ Bug 13208 ]] Convert RGB xy endpoints to XYZ transform matrix
		MCColorVector3 t_white;
		MCColorMatrix3x3 t_matrix;

		t_success = MCColorTransformLinearRGBToXYZ(
			MCColorVector2Make(p_info.calibrated.white_x, p_info.calibrated.white_y),
			MCColorVector2Make(p_info.calibrated.red_x, p_info.calibrated.red_y),
			MCColorVector2Make(p_info.calibrated.green_x, p_info.calibrated.green_y),
			MCColorVector2Make(p_info.calibrated.blue_x, p_info.calibrated.blue_y),
			t_white, t_matrix);

		if (t_success)
		{
			uint32_t t_gamma;
			// gamma is specified as 8.8 fixed point shifted by 8 bits
			t_gamma = (0xFFFF & (uint32_t)((1 << 8) / p_info.calibrated.gamma)) << 8;

			LOGCOLORSPACEA t_colorspace;
			t_colorspace . lcsSignature = LCS_SIGNATURE;
			t_colorspace . lcsVersion = 0x400;
			t_colorspace . lcsSize = sizeof(LOGCOLORSPACEA);
			t_colorspace . lcsCSType = LCS_CALIBRATED_RGB;
			t_colorspace . lcsIntent = LCS_GM_GRAPHICS;

			// Read off channel XYZ from the matrix columns
			t_colorspace.lcsEndpoints.ciexyzRed = CIEXYZMake(t_matrix.m[0][0], t_matrix.m[1][0], t_matrix.m[2][0]);
			t_colorspace.lcsEndpoints.ciexyzGreen = CIEXYZMake(t_matrix.m[0][1], t_matrix.m[1][1], t_matrix.m[2][1]);
			t_colorspace.lcsEndpoints.ciexyzBlue = CIEXYZMake(t_matrix.m[0][2], t_matrix.m[1][2], t_matrix.m[2][2]);

			t_colorspace . lcsGammaRed = t_gamma;
			t_colorspace . lcsGammaGreen = t_gamma;
			t_colorspace . lcsGammaBlue = t_gamma;
			t_colorspace . lcsFilename[0] = '\0';
			t_transform = CreateColorTransformA(&t_colorspace, m_srgb_profile, nil, 0);

			t_success = t_transform != nil;
		}
	}
	else if (p_info . type == kMCColorSpaceStandardRGB)
	{
		// TODO - This isn't quite right, disable for now
#if WIN32_COLOR_PROFILE_FIX_ME
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
