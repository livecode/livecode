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

#include <Cocoa/Cocoa.h>

#include "core.h"
#include "globdefs.h"
#include "uidc.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

////////////////////////////////////////////////////////////////////////////////

struct MCPlatformColorTransform
{
	uint32_t references;
	CMWorldRef world;
	bool is_cmyk;
};

////////////////////////////////////////////////////////////////////////////////

static CMProfileRef s_dst_profile = nil;
static CMProfileRef s_srgb_profile = nil;

////////////////////////////////////////////////////////////////////////////////

void MCPlatformCreateColorTransform(const MCColorSpaceInfo& p_info, MCPlatformColorTransformRef& r_transform)
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
		t_src_profile = s_srgb_profile;
	else
		t_success = false;
	
	CMAppleProfileHeader t_header;
	if (t_success)
		if (CMGetProfileHeader(t_src_profile, &t_header) != noErr)
			t_success = false;
	
	CMWorldRef t_world;
	t_world = nil;
	if (t_success)
		if (NCWNewColorWorld(&t_world, t_src_profile, s_dst_profile) != noErr)
			t_success = false;
	
	MCPlatformColorTransform *t_colorxform;
	t_colorxform = nil;
	if (t_success)
		t_success = MCMemoryNew(t_colorxform);
	
	if (t_success)
	{
		t_colorxform -> references = 1;
		t_colorxform -> world = t_world;
		t_colorxform -> is_cmyk = (t_header . cm2 . dataColorSpace == cmCMYKData);
	}
	else
	{
		if (t_colorxform != nil)
			MCPlatformReleaseColorTransform(t_colorxform);
		t_colorxform = nil;
	}
	
	if (t_src_profile != nil && t_src_profile != s_srgb_profile)
		CMCloseProfile(t_src_profile);
	
	r_transform = t_colorxform;
}

void MCPlatformRetainColorTransform(MCPlatformColorTransformRef p_transform)
{
	if (p_transform == nil)
		return;
		
	p_transform -> references += 1;
}

void MCPlatformReleaseColorTransform(MCPlatformColorTransformRef p_transform)
{
	if (p_transform == nil)
		return;
		
	p_transform -> references -= 1;
	if (p_transform -> references == 0)
	{
		if (p_transform -> world != nil)
			CWDisposeColorWorld(p_transform -> world);
		
		MCMemoryDelete(p_transform);
	}
}

static void byte_swap_bitmap_data(MCImageBitmap *p_bitmap)
{
	uint8_t *t_src_row = (uint8_t*)p_bitmap->data;
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		uint32_t *t_src_pixel = (uint32_t*)t_src_row;
		for (uindex_t x = 0; x < p_bitmap->width; x++)
		{
			*t_src_pixel = MCByteSwappedFromHost32(*t_src_pixel);
			t_src_pixel++;
		}
		t_src_row += p_bitmap->stride;
	}
}

bool MCPlatformApplyColorTransform(MCPlatformColorTransformRef p_transform, MCImageBitmap *p_image)
{
	if (p_transform == nil)
		return false;
		
	CMBitmap t_bitmap;
	t_bitmap . image = (char *)p_image -> data;
	t_bitmap . width = p_image -> width;
	t_bitmap . height = p_image -> height;
	t_bitmap . rowBytes = p_image -> stride;
	t_bitmap . pixelSize = 32;
	
	// Note the byte-swapping here... ColorSync only accepts 'big-endian' order
	// pixel formats, and our RGB ones come in (and are required on out) in host order.
	
	if (p_transform -> is_cmyk)
		t_bitmap . space = cmCMYK32Space;
	else
	{
		byte_swap_bitmap_data(p_image);
		t_bitmap . space = cmARGB32Space;
	}
	
	t_bitmap . user1 = 0;
	t_bitmap . user2 = 0;
	bool t_success;
	t_success = true;
	if (CWMatchBitmap(p_transform -> world, &t_bitmap, nil, nil, nil) != noErr)
		t_success = false;
	
	byte_swap_bitmap_data(p_image);
}

////////////////////////////////////////////////////////////////////////////////

bool MCPlatformInitializeColorTransform(void)
{
	CMGetDefaultProfileBySpace(cmRGBData, &s_dst_profile);
	
	CMProfileLocation t_location;
	t_location . locType = cmPathBasedProfile;
	strcpy(t_location . u . pathLoc . path, "/System/Library/ColorSync/Profiles/sRGB Profile.icc");
	CMOpenProfile(&s_srgb_profile, &t_location);
	
	return true;
}

void MCPlatformFinalizeColorTransform(void)
{
	if (s_dst_profile != nil)
	{
		CMCloseProfile(s_dst_profile);
		s_dst_profile = nil;
	}
	
	if (s_srgb_profile != nil)
	{
		CMCloseProfile(s_srgb_profile);
		s_srgb_profile = nil;
	}
}

////////////////////////////////////////////////////////////////////////////////
