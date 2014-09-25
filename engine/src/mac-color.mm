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

#include "color.h"

#include "mac-internal.h"

// IM-2014-09-24: [[ Bug 13208 ]] Update color transform to use CoreGraphics API

////////////////////////////////////////////////////////////////////////////////

struct MCPlatformColorTransform
{
	uint32_t references;
	CGColorSpaceRef colorspace;
};

////////////////////////////////////////////////////////////////////////////////

void MCPlatformCreateColorTransform(const MCColorSpaceInfo& p_info, MCPlatformColorTransformRef& r_transform)
{
	bool t_success;
	t_success = true;
	
	CGColorSpaceRef t_colorspace;
	t_colorspace = nil;
	
	bool t_is_cmyk;
	t_is_cmyk = false;
	
	if (p_info . type == kMCColorSpaceEmbedded)
	{
		// Create colorspace using ICC data
		CFDataRef t_data;
		t_data = nil;
		
		t_success = nil != (t_data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (UInt8*)p_info.embedded.data, p_info.embedded.data_size, kCFAllocatorNull));
		
		if (t_success)
			t_success = nil != (t_colorspace = CGColorSpaceCreateWithICCProfile(t_data));
		
		if (t_data != nil)
			CFRelease(t_data);
	}
	else if (p_info . type == kMCColorSpaceStandardRGB)
		// Create sRGB colorspace
		t_success = nil != (t_colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB));
	else if (p_info . type == kMCColorSpaceCalibratedRGB)
	{
		// Create calibrated RGB colorspace, converting linear xy coords to CIE XYZ transform
		t_success = p_info.calibrated.gamma != 0;
		
		MCColorVector3 t_XYZ_white;
		MCColorMatrix3x3 t_XYZ_matrix;
		
		if (t_success)
			t_success = MCColorTransformLinearRGBToXYZ(MCColorVector2Make(p_info.calibrated.white_x, p_info.calibrated.white_y),
													   MCColorVector2Make(p_info.calibrated.red_x, p_info.calibrated.red_y),
													   MCColorVector2Make(p_info.calibrated.green_x, p_info.calibrated.green_y),
													   MCColorVector2Make(p_info.calibrated.blue_x, p_info.calibrated.blue_y),
													   t_XYZ_white, t_XYZ_matrix);
		
		if (t_success)
		{
			CGFloat t_white[3];
			t_white[0] = t_XYZ_white.x;
			t_white[1] = t_XYZ_white.y;
			t_white[2] = t_XYZ_white.z;
			
			CGFloat t_matrix[9];
			MCColorMatrix3x3GetElements(t_XYZ_matrix, t_matrix);
			
			CGFloat t_gamma[3];
			t_gamma[0] = t_gamma[1] = t_gamma[2] = 1.0 / p_info.calibrated.gamma;
			t_success = nil != (t_colorspace = CGColorSpaceCreateCalibratedRGB(t_white, nil, t_gamma, t_matrix));
		}
	}
	else
		t_success = false;
	
	MCPlatformColorTransform *t_colorxform;
	t_colorxform = nil;
	if (t_success)
		t_success = MCMemoryNew(t_colorxform);
	
	if (t_success)
	{
		t_colorxform->references = 1;
		t_colorxform->colorspace = t_colorspace;
	}
	else
	{
		if (t_colorspace != nil)
			CFRelease(t_colorspace);
		t_colorxform = nil;
	}
	
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
		if (p_transform -> colorspace != nil)
			CFRelease(p_transform->colorspace);
		
		MCMemoryDelete(p_transform);
	}
}

bool MCPlatformApplyColorTransform(MCPlatformColorTransformRef p_transform, MCImageBitmap *p_image)
{
	if (p_transform == nil)
		return false;
	
	bool t_success;
	t_success = true;
	
	CFDataRef t_data;
	t_data = nil;
	
	if (t_success)
		t_success = nil != (t_data = CFDataCreate(kCFAllocatorDefault, (UInt8*)p_image->data, p_image->stride * p_image->height));
	
	CGDataProviderRef t_provider;
	t_provider = nil;
	
	if (t_success)
		t_success = nil != (t_provider = CGDataProviderCreateWithCFData(t_data));
	
	CGImageRef t_image;
	t_image = nil;
	
	CGBitmapInfo t_dst_bm_info;
	t_dst_bm_info = kCGBitmapByteOrder32Host | (MCImageBitmapHasTransparency(p_image) ? kCGImageAlphaFirst : kCGImageAlphaNoneSkipFirst);
	
	if (t_success)
	{
		CGBitmapInfo t_bm_info;
		if (CGColorSpaceGetModel(p_transform->colorspace) == kCGColorSpaceModelCMYK)
			t_bm_info = kCGBitmapByteOrder32Host;
		else
			t_bm_info = t_dst_bm_info;
			
		t_success = nil != (t_image = CGImageCreate(p_image->width, p_image->height, 8, 32, p_image->stride, p_transform->colorspace, t_bm_info, t_provider, nil, false, kCGRenderingIntentDefault));
	}
	
	CGColorSpaceRef t_dst_colorspace;
	t_dst_colorspace = nil;
	
	if (t_success)
		t_success = nil != (t_dst_colorspace = CGColorSpaceCreateDeviceRGB());
	
	CGContextRef t_context;
	t_context = nil;
	
	if (t_success)
		t_success = nil != (t_context = CGBitmapContextCreate(p_image->data, p_image->width, p_image->height, 8, p_image->stride, t_dst_colorspace, t_dst_bm_info));
	
	if (t_success)
	{
		CGContextSetBlendMode(t_context, kCGBlendModeCopy);
		CGContextDrawImage(t_context, CGRectMake(0, 0, p_image->width, p_image->height), t_image);
	}
	
	if (t_context != nil)
		CFRelease(t_context);
	
	if (t_dst_colorspace != nil)
		CFRelease(t_dst_colorspace);
	
	if (t_image != nil)
		CFRelease(t_image);
	
	if (t_provider != nil)
		CFRelease(t_provider);
	
	if (t_data != nil)
		CFRelease(t_data);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCPlatformInitializeColorTransform(void)
{
	return true;
}

void MCPlatformFinalizeColorTransform(void)
{
}

////////////////////////////////////////////////////////////////////////////////
