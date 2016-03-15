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

#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "lnxdc.h"

#include "lcms2.h"

////////////////////////////////////////////////////////////////////////////////

static bool MCuselcms = false;
static bool MClcmsresolved = false;

extern "C" int initialise_weak_link_lcms(void);

MCColorTransformRef MCScreenDC::createcolortransform(const MCColorSpaceInfo& p_info)
{
	if (!MClcmsresolved)
	{
		MCuselcms = initialise_weak_link_lcms() != 0;
		MClcmsresolved = true;
	}

	if (!MCuselcms)
		return nil;

	bool t_success;
	t_success = true;

	cmsHPROFILE t_in_profile, t_out_profile;
	t_in_profile = t_out_profile = nil;
	if (p_info . type == kMCColorSpaceEmbedded)
	{
		if (t_success)
		{
			t_in_profile = cmsOpenProfileFromMem(p_info . embedded . data, p_info . embedded . data_size);
			if (t_in_profile == nil)
				t_success = false;
		}

		if (t_success)
		{
			t_out_profile = cmsCreate_sRGBProfile();
			if (t_out_profile == nil)
				t_success = false;
		}
	}
	else if (p_info . type == kMCColorSpaceStandardRGB)
	{
		// X11 is by default sRGB
		t_success = false;
	}
	else if (p_info . type == kMCColorSpaceCalibratedRGB)
	{
		if (t_success)
		{
			cmsCIExyY t_whitepoint;
			t_whitepoint . x = p_info . calibrated . white_x;
			t_whitepoint . y = p_info . calibrated . white_y;
			t_whitepoint . Y = 1.0;

			cmsCIExyYTRIPLE t_primaries;
			t_primaries . Red . x = p_info . calibrated . red_x;
			t_primaries . Red . y = p_info . calibrated . red_y;
			t_primaries . Red . Y = 1.0;
			t_primaries . Green . x = p_info . calibrated . green_x;
			t_primaries . Green . y = p_info . calibrated . green_y;
			t_primaries . Green . Y = 1.0;
			t_primaries . Blue . x = p_info . calibrated . blue_x;
			t_primaries . Blue . y = p_info . calibrated . blue_y;
			t_primaries . Blue . Y = 1.0;

			cmsToneCurve* t_gamma_table[3];
			t_gamma_table[0] = t_gamma_table[1] = t_gamma_table[2] = cmsBuildGamma(NULL, 1.0 / p_info . calibrated . gamma);
			if (t_gamma_table[0] != nil)
			{
				t_in_profile = cmsCreateRGBProfile(&t_whitepoint, &t_primaries, t_gamma_table);
				if (t_in_profile == nil)
					t_success = false;

				cmsFreeToneCurve(t_gamma_table[0]);
			}
			else
				t_success = false;
		}

		if (t_success)
		{
			t_out_profile = cmsCreate_sRGBProfile();
			if (t_out_profile == nil)
				t_success = false;
		}
	}
	else
		t_success = false;

	cmsHTRANSFORM t_transform;
	t_transform = nil;
	if (t_success)
	{
		cmsColorSpaceSignature t_input_sig;
		t_input_sig = cmsGetColorSpace(t_in_profile);

		cmsUInt32Number t_input_type;
		if (t_input_sig == cmsSigCmykData)
			t_input_type = TYPE_CMYK_8;
		else if (t_input_sig == cmsSigRgbData)
			t_input_type = TYPE_BGRA_8;
		else
			t_input_type = 0;

		if (t_input_type != 0)
			t_transform = cmsCreateTransform(t_in_profile, t_input_type, t_out_profile, TYPE_BGRA_8, INTENT_PERCEPTUAL, 0);
	}

	if (t_in_profile != nil)
		cmsCloseProfile(t_in_profile);

	if (t_out_profile != nil)
		cmsCloseProfile(t_out_profile);

	return t_transform;
}

void MCScreenDC::destroycolortransform(MCColorTransformRef p_transform)
{
	if (!MCuselcms)
		return;

	if (p_transform != nil)
		cmsDeleteTransform(p_transform);
}

bool MCScreenDC::transformimagecolors(MCColorTransformRef p_transform, MCImageBitmap *p_image)
{
	if (!MCuselcms)
		return false;

	for(int32_t i = 0; i < p_image -> height; i++)
	{
		void *t_scanline;
		t_scanline = (uint8_t*)p_image -> data + i * p_image -> stride;
		cmsDoTransform(p_transform, t_scanline, t_scanline, p_image -> width);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
