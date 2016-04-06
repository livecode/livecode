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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "osxdc.h"

////////////////////////////////////////////////////////////////////////////////

struct MCMacOSXColorTransform
{
	CMWorldRef world;
	bool is_cmyk;
};

MCColorTransformRef MCScreenDC::createcolortransform(const MCColorSpaceInfo& p_info)
{
	bool t_success;
	t_success = true;
	
	CMProfileRef t_src_profile;
	t_src_profile = nil;
	bool t_is_cmyk;
	t_is_cmyk = false;
	if (p_info . type == kMCColorSpaceEmbedded)
	{
		CMProfileLocation t_location;
		t_location . locType = cmBufferBasedProfile;
		t_location . u . bufferLoc . buffer = p_info . embedded . data;
		t_location . u . bufferLoc . size = p_info . embedded . data_size;
		if (CMOpenProfile(&t_src_profile, &t_location) != noErr)
			t_success = false;
	}
	else if (p_info . type == kMCColorSpaceStandardRGB)
		t_src_profile = m_srgb_profile;
	else
		t_success = false;
	
	CMAppleProfileHeader t_header;
	if (t_success)
		if (CMGetProfileHeader(t_src_profile, &t_header) != noErr)
			t_success = false;
	
	CMWorldRef t_world;
	t_world = nil;
	if (t_success)
		if (NCWNewColorWorld(&t_world, t_src_profile, m_dst_profile) != noErr)
			t_success = false;
	
	MCMacOSXColorTransform *t_colorxform;
	t_colorxform = nil;
	if (t_success)
		t_success = MCMemoryNew(t_colorxform);
	
	if (t_success)
	{
		t_colorxform -> world = t_world;
		t_colorxform -> is_cmyk = (t_header . cm2 . dataColorSpace == cmCMYKData);
	}
	else
	{
		destroycolortransform(t_colorxform);
		t_colorxform = nil;
	}
	
	if (t_src_profile != nil && t_src_profile != m_srgb_profile)
		CMCloseProfile(t_src_profile);
	
	return t_colorxform;
	
}

void MCScreenDC::destroycolortransform(MCColorTransformRef transform)
{
	MCMacOSXColorTransform *t_transform;
	t_transform = (MCMacOSXColorTransform *)transform;
	
	if (t_transform == nil)
		return;
	
	if (t_transform -> world != nil)
		CWDisposeColorWorld(t_transform -> world);
	
	MCMemoryDelete(t_transform);
}

static void byte_swap_bitmap_data(MCImageBitmap *p_bitmap)
{
	uint8_t *t_src_row = (uint8_t*)p_bitmap->data;
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		uint32_t *t_src_pixel = (uint32_t*)t_src_row;
		for (uindex_t x = 0; x < p_bitmap->width; x++)
		{
			*t_src_pixel = MCSwapInt32HostToBig(*t_src_pixel);
			t_src_pixel++;
		}
		t_src_row += p_bitmap->stride;
	}
}

bool MCScreenDC::transformimagecolors(MCColorTransformRef transform, MCImageBitmap *image)
{
	MCMacOSXColorTransform *t_transform;
	t_transform = (MCMacOSXColorTransform *)transform;
	
	CMBitmap t_bitmap;
	t_bitmap . image = (char *)image -> data;
	t_bitmap . width = image -> width;
	t_bitmap . height = image -> height;
	t_bitmap . rowBytes = image -> stride;
	t_bitmap . pixelSize = 32;
	
	// Note the byte-swapping here... ColorSync only accepts 'big-endian' order
	// pixel formats, and our RGB ones come in (and are required on out) in host order.
	
	if (t_transform -> is_cmyk)
		t_bitmap . space = cmCMYK32Space;
	else
	{
		byte_swap_bitmap_data(image);
		t_bitmap . space = cmARGB32Space;
	}
	
	t_bitmap . user1 = 0;
	t_bitmap . user2 = 0;
	bool t_success;
	t_success = true;
	if (CWMatchBitmap(t_transform -> world, &t_bitmap, nil, nil, nil) != noErr)
		t_success = false;
	
	byte_swap_bitmap_data(image);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
