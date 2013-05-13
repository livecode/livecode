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

#include <Quartz/Quartz.h>

#include "visualeffect.h"

#define OBJC_ENTER_VOID \
	NSAutoreleasePool *__t_pool = [[NSAutoreleasePool alloc] init]; \
	if (__t_pool == NULL) return;

#define OBJC_ENTER(value) \
	NSAutoreleasePool *__t_pool = [[NSAutoreleasePool alloc] init]; \
	if (__t_pool == NULL) return (value);

#define OBJC_LEAVE \
	[__t_pool release];

static bool s_coreimage_present = false;

typedef struct __coreimage_visualeffect_t coreimage_visualeffect_t;
typedef coreimage_visualeffect_t *coreimage_visualeffect_ref_t;
struct __coreimage_visualeffect_t
{
	coreimage_visualeffect_t *next;
	NSString *name;
	rei_visualeffect_info_ref_t info;
};

static coreimage_visualeffect_ref_t coreimage_visualeffect_create(NSString *p_name)
{
	coreimage_visualeffect_ref_t t_effect = NULL;
	rei_visualeffect_info_ref_t t_info = NULL;
	CIFilter *t_filter = nil;
	NSArray *t_parameters = nil;
	NSDictionary *t_attributes = nil;
	int t_index;
	
	t_effect = malloc(sizeof(coreimage_visualeffect_t));
	
	if (t_effect != NULL)
		t_filter = [CIFilter filterWithName: p_name];
	
	if (t_filter != NULL)
		t_parameters = [t_filter inputKeys];
		
	if (t_parameters != NULL)
		t_attributes = [t_filter attributes];
	
	if (t_parameters != NULL)
		t_info = malloc(8 * [t_parameters count]);
	
	if (t_info != NULL)
	{
		t_info -> handle = (void *)t_effect;
		t_info -> parameter_count = 0;
		for(t_index = 0; t_index < [t_parameters count]; ++t_index)
		{
			NSString *t_name;
			NSString *t_class;
			rei_visualeffect_parameter_type_t t_type;
			char *t_static_name;

			t_name = [t_parameters objectAtIndex: t_index];
					
			if ([t_name isEqualToString: @"inputImage"])
				continue;
				
			if ([t_name isEqualToString: @"inputTargetImage"])
				continue;
						
			if ([t_name isEqualToString: @"inputTime"])
				continue;
						
			if (![t_name hasPrefix: @"input"])
				continue;
					
			t_class = [[t_attributes objectForKey: t_name] objectForKey: kCIAttributeClass];
			if ([t_class isEqualToString: @"NSNumber"])
				t_type = REI_VISUALEFFECT_PARAMETER_TYPE_NUMBER;
			else if ([t_class isEqualToString: @"CIVector"])
				t_type = REI_VISUALEFFECT_PARAMETER_TYPE_VECTOR;
			else if ([t_class isEqualToString: @"CIColor"])
				t_type = REI_VISUALEFFECT_PARAMETER_TYPE_COLOUR;
			else if ([t_class isEqualToString: @"CIImage"])
				t_type = REI_VISUALEFFECT_PARAMETER_TYPE_IMAGE;
			else
				continue;
				
			t_static_name = strdup([[t_name substringFromIndex: 5] UTF8String]);
			
			if (t_static_name != NULL)
			{
				t_info -> parameters[t_info -> parameter_count] . name = t_static_name;
				t_info -> parameters[t_info -> parameter_count] . type = t_type;
				t_info -> parameter_count += 1;
			}
		}
	}
	
	if (t_info != NULL)
	{
		[p_name retain];
		t_effect -> next = NULL;
		t_effect -> name = p_name;
		t_effect -> info = t_info;
	}
	else
	{
		free(t_info);
		free(t_effect);
		t_effect = NULL;
	}
	
	return t_effect;
}

static void coreimage_visualeffect_destroy(coreimage_visualeffect_ref_t p_effect)
{
	unsigned int t_index;
	[p_effect -> name release];

	for(t_index = 0; t_index < p_effect -> info -> parameter_count; ++t_index)
		free((char *)p_effect -> info -> parameters[t_index] . name);
		
	free(p_effect -> info);
	free(p_effect);
}

static NSArray *sg_effects = NULL;
static coreimage_visualeffect_ref_t sg_effect_infos = NULL;

rei_boolean_t coreimage_visualeffect_initialise(void)
{
	OBJC_ENTER(false)
		
	NS_DURING
		if (NSClassFromString(@"CIFilter") != nil)
			sg_effects = [[CIFilter filterNamesInCategory: kCICategoryTransition] retain];
	NS_HANDLER
		sg_effects = NULL;
	NS_ENDHANDLER

	OBJC_LEAVE
	
	return sg_effects != NULL;
}

void coreimage_visualeffect_finalise(void)
{
	OBJC_ENTER_VOID

	NS_DURING
		[sg_effects release];
		
		coreimage_visualeffect_ref_t t_effect;
		t_effect = sg_effect_infos;
		while(t_effect != NULL)
		{
			coreimage_visualeffect_ref_t t_next;
			t_next = t_effect -> next;
			coreimage_visualeffect_destroy(t_effect);
			t_effect = t_next;
		}
	NS_HANDLER
	NS_ENDHANDLER

	OBJC_LEAVE
}

rei_boolean_t coreimage_visualeffect_lookup(const char *p_name, rei_visualeffect_info_ref_t *r_info)
{
	NSString *t_name = nil;
	coreimage_visualeffect_ref_t t_effect = NULL;
	
	OBJC_ENTER(false)
	
	NS_DURING
		
		t_name = [NSString stringWithCString: p_name];
		if (t_name == NULL)
			return false;

		for(t_effect = sg_effect_infos; t_effect != NULL; t_effect = t_effect -> next)
			if ([t_name isEqualToString: t_effect -> name])
				break;

		unsigned int t_index;
		if (t_effect == NULL)
			for(t_index = 0; t_index < [sg_effects count]; ++t_index)
			{
				if ([t_name isEqualToString: [sg_effects objectAtIndex: t_index]])
				{
					t_effect = coreimage_visualeffect_create(t_name);
					t_effect -> next = sg_effect_infos;
					sg_effect_infos = t_effect;
					break;
				}
			}
		
		if (t_effect != NULL)
			*r_info = t_effect -> info;
	NS_HANDLER
		t_effect = NULL;
	NS_ENDHANDLER
	
	OBJC_LEAVE
	
	return t_effect != NULL;
}

static CGrafPtr sg_current_graf_port = NULL;
static CGContextRef sg_current_cg_context = NULL;
static CIFilter *sg_current_filter = nil;
static CIContext *sg_current_context = nil;
static rei_rectangle_t sg_current_area;

static CIImage *qd_to_ci(CGrafPtr p_port, rei_rectangle_ref_t p_area)
{
	NSMutableData *t_data = nil;
	CIImage *t_result = nil;
	PixMapHandle t_source_pixmap_handle;
	PixMapPtr t_source_pixmap;
	char *t_source_addr;
	unsigned int t_source_row_bytes;
	unsigned int *t_dest_addr;
	bool t_offscreen;
	
	t_offscreen = IsPortOffscreen(p_port);
	
	if (!t_offscreen)
		LockPortBits(p_port);
	
	t_source_pixmap_handle = t_offscreen ? GetGWorldPixMap(p_port) : GetPortPixMap(p_port);
	
	LockPixels(t_source_pixmap_handle);
	HLock((Handle)t_source_pixmap_handle);
	
	t_source_addr = GetPixBaseAddr(t_source_pixmap_handle);
	t_source_pixmap = *t_source_pixmap_handle;
	t_source_row_bytes = GetPixRowBytes(t_source_pixmap_handle); //(t_source_pixmap -> pmExt != NULL ? (*(SInt32 **)t_source_pixmap -> pmExt)[7] : t_source_pixmap -> rowBytes);
	t_data = [NSMutableData dataWithLength: p_area -> width * p_area -> height * 4];

	if (t_data != NULL)
	{
		int t_x, t_y;
		Rect t_bounds;
		GetPixBounds(t_source_pixmap_handle, &t_bounds);
				
		if (t_bounds . right - t_bounds . left != p_area -> width || t_bounds . bottom - t_bounds . top != p_area -> height)
			t_source_addr += t_source_row_bytes * -t_bounds . top + 4 * -t_bounds . left;
		
		t_dest_addr = (unsigned int *)[t_data mutableBytes];
		
		switch(t_source_pixmap -> pixelFormat)
		{
			case k16BE555PixelFormat:
				if (t_bounds . right - t_bounds . left != p_area -> width || t_bounds . bottom - t_bounds . top != p_area -> height)
					t_source_addr += 2 * p_area -> x + t_source_row_bytes * p_area -> y;
				for(t_y = 0; t_y < p_area -> height; ++t_y, t_source_addr += t_source_row_bytes)
					for(t_x = 0; t_x < p_area -> width; ++t_x)
						t_dest_addr[t_y * p_area -> width + t_x] =
							  ((((((unsigned short *)t_source_addr)[t_x] >> 10) << 3) + 7) << 16)
							| (((((((unsigned short *)t_source_addr)[t_x] >> 5) & 0x1f) << 3) + 7) << 8)
							| (((((unsigned short *)t_source_addr)[t_x] & 0x1f) << 3) + 7);
			break;
			
			case k32BGRAPixelFormat:
			case k24BGRPixelFormat:
				if (t_bounds . right - t_bounds . left != p_area -> width || t_bounds . bottom - t_bounds . top != p_area -> height)
					t_source_addr += 4 * p_area -> x + t_source_row_bytes * p_area -> y;
				for(t_y = 0; t_y < p_area -> height; ++t_y, t_source_addr += t_source_row_bytes)
					for(t_x = 0; t_x < p_area -> width; ++t_x)
						t_dest_addr[t_y * p_area -> width + t_x] = CFSwapInt32(((unsigned int *)t_source_addr)[t_x]); // 0xFF000000 | ((unsigned int *)t_source_addr)[t_x] & 0xFFFFFF;
			break;
			
			case k24RGBPixelFormat:
			case k32ARGBPixelFormat:
				if (t_bounds . right - t_bounds . left != p_area -> width || t_bounds . bottom - t_bounds . top != p_area -> height)
					t_source_addr += 4 * p_area -> x + t_source_row_bytes * p_area -> y;
				for(t_y = 0; t_y < p_area -> height; ++t_y, t_source_addr += t_source_row_bytes)
					for(t_x = 0; t_x < p_area -> width; ++t_x)
						t_dest_addr[t_y * p_area -> width + t_x] = ((unsigned int *)t_source_addr)[t_x]; // 0xFF000000 | ((unsigned int *)t_source_addr)[t_x] & 0xFFFFFF;
			break;
		}
	}
	
	HUnlock((Handle)t_source_pixmap_handle);
	UnlockPixels(t_source_pixmap_handle);
	
	UnlockPortBits(p_port);

	if (t_data != NULL)
	{
		CGColorSpaceRef t_color_space;
		t_color_space = CGColorSpaceCreateDeviceRGB();
		t_result = [CIImage imageWithBitmapData: t_data bytesPerRow: p_area -> width * 4 size: CGSizeMake(p_area -> width, p_area -> height) format: kCIFormatARGB8 colorSpace: t_color_space];
		CGColorSpaceRelease(t_color_space);
	}

	return t_result;
}

rei_boolean_t coreimage_visualeffect_begin(rei_handle_t p_handle, CGrafPtr p_target, CGrafPtr p_source_a, CGrafPtr p_source_b, rei_rectangle_ref_t p_area, rei_visualeffect_parameters_ref_t p_parameters)
{
	coreimage_visualeffect_ref_t t_effect = (coreimage_visualeffect_ref_t)p_handle;
	CIFilter *t_filter = nil;
	CIImage *t_image_a = nil;
	CIImage *t_image_b = nil;
	CIContext *t_context = nil;
	CGContextRef t_cg_context = NULL;
	bool t_bound = false;
	
	OBJC_ENTER(false)

	NS_DURING
	
	t_filter = [CIFilter filterWithName: t_effect -> name];
	
	if (t_filter != nil)
		t_image_a = qd_to_ci(p_source_a, p_area);
		
	if (t_image_a != nil)
		t_image_b = qd_to_ci(p_source_b, p_area);
		
	if (t_image_b != nil)
	{
		if (IsPortOffscreen(p_target))
		{
			PixMapHandle t_target_pixmap;
			uint32_t t_target_stride;
			uint8_t *t_target_ptr;

			t_target_pixmap = GetGWorldPixMap(p_target);
			LockPixels(t_target_pixmap);
			
			Rect t_bounds;
			GetPixBounds(t_target_pixmap, &t_bounds);
			t_target_stride = GetPixRowBytes(t_target_pixmap);
			t_target_ptr = (uint8_t *)GetPixBaseAddr(t_target_pixmap);
			t_target_ptr += t_target_stride * -t_bounds . top - t_bounds . left * 4;
			
			// MW-2011-10-01: [[ Bug 9769 ]] Make sure we use the correct byte order.
			CGColorSpaceRef t_color_space;
			t_color_space = CGColorSpaceCreateDeviceRGB();
			t_cg_context = CGBitmapContextCreate(t_target_ptr, t_bounds . right - t_bounds . left, t_bounds . bottom - t_bounds . top, 8, t_target_stride, t_color_space, kCGBitmapByteOrder32Host | kCGImageAlphaPremultipliedFirst);
			CGColorSpaceRelease(t_color_space);
		}
		else if (QDBeginCGContext(p_target, &t_cg_context) != noErr)
			t_cg_context = NULL;
	}
	
	if (t_cg_context != NULL)
		t_context = [CIContext contextWithCGContext: t_cg_context options: nil];
	
	if (t_context != NULL)
	{
		unsigned int t_index;
		unsigned int t_length;
		
		t_length = p_parameters == NULL ? 0 : p_parameters -> length;
		
		[t_filter setDefaults];
		
		for(t_index = 0; t_index < t_length; ++t_index)
		{
			NSString *t_key = nil;
			NSObject *t_value = nil;
			
			t_key = [NSString stringWithFormat: @"input%s", p_parameters -> entries[t_index] . name];
			if (t_key == nil)
				break;
			
			switch(p_parameters -> entries[t_index] . type)
			{
				case REI_VISUALEFFECT_PARAMETER_TYPE_NUMBER:
					t_value = [NSNumber numberWithDouble: p_parameters -> entries[t_index] . value . number];
				break;
				
				case REI_VISUALEFFECT_PARAMETER_TYPE_STRING:
				break;
				
				case REI_VISUALEFFECT_PARAMETER_TYPE_COLOUR:
				{
					float t_red = p_parameters -> entries[t_index] . value . colour . red / 255.0;
					float t_green = p_parameters -> entries[t_index] . value . colour . green / 255.0;
					float t_blue = p_parameters -> entries[t_index] . value . colour . blue / 255.0;
					float t_alpha = p_parameters -> entries[t_index] . value . colour . alpha / 255.0;
					t_value = [CIColor colorWithRed: t_red green: t_green blue: t_blue alpha: t_alpha];
				}
				break;
				
				case REI_VISUALEFFECT_PARAMETER_TYPE_VECTOR:
				{
					float *t_vector_as_float;
					unsigned int t_count;
					unsigned int t_jndex;
				
					t_count = p_parameters -> entries[t_index] . value . vector . length;
				
					t_vector_as_float = alloca(sizeof(float) * t_count);
					if (t_vector_as_float != NULL)
					{
						for(t_jndex = 0; t_jndex < t_count; ++t_jndex)
							t_vector_as_float[t_jndex] = (float)p_parameters -> entries[t_index] . value . vector . coefficients[t_jndex];
						t_value = [CIVector vectorWithValues: t_vector_as_float count: t_count];
						free(p_parameters -> entries[t_index] . value . vector . coefficients);
					}
				}
				break;
				
				case REI_VISUALEFFECT_PARAMETER_TYPE_IMAGE:
				{
					unsigned int t_width, t_height;
					unsigned int *t_data;
					t_width = p_parameters -> entries[t_index] . value . image . width;
					t_height = p_parameters -> entries[t_index] . value . image . height;
					t_data = p_parameters -> entries[t_index] . value . image . data;
					CGColorSpaceRef t_color_space;
					t_color_space = CGColorSpaceCreateDeviceRGB();
					t_value = [CIImage imageWithBitmapData: [NSData dataWithBytesNoCopy: t_data length: t_width * t_height * 4] bytesPerRow: t_width * 4 size: CGSizeMake(t_width, t_height) format: kCIFormatARGB8 colorSpace: t_color_space];
					CGColorSpaceRelease(t_color_space);
				}
				break;
				
				case REI_VISUALEFFECT_PARAMETER_TYPE_UNDEFINED:
					t_value = nil;
				break;
			}
			
			if (t_value == nil)
				break;
				
			[t_filter setValue: t_value forKey: t_key];
		}
		
		if (t_index == t_length)
			t_bound = true;
	}
	
	if (t_bound)
	{
		[t_filter setValue: t_image_a forKey: @"inputImage"];
		[t_filter setValue: t_image_b forKey: @"inputTargetImage"];
	
		[t_filter retain];
		sg_current_filter = t_filter;
		
		[t_context retain];
		sg_current_context = t_context;
		
		sg_current_graf_port = p_target;
		sg_current_cg_context = t_cg_context;
		sg_current_area = *p_area;
	}
	
	NS_HANDLER
		t_bound = false;
		
		if (sg_current_context != NULL)
			[sg_current_context release];
		if (sg_current_filter != NULL)
			[sg_current_filter release];
		if (sg_current_cg_context != NULL)
		{
			if (IsPortOffscreen(sg_current_cg_context))
				CGContextRelease(sg_current_cg_context);
			else
				QDEndCGContext(sg_current_graf_port, &sg_current_cg_context);
		}
	
		sg_current_context = NULL;
		sg_current_filter = NULL;
		sg_current_cg_context = NULL;
	NS_ENDHANDLER;
	
	OBJC_LEAVE
	
	return t_bound;
}

rei_boolean_t coreimage_visualeffect_step(float p_time)
{
	rei_boolean_t t_result = true;
	Rect t_rect;

	OBJC_ENTER(false)

	GetPortBounds(sg_current_graf_port, &t_rect);

	NS_DURING
		CGContextClearRect(sg_current_cg_context, CGRectMake(sg_current_area . x, t_rect . bottom - (sg_current_area . y + sg_current_area . height), sg_current_area . width, sg_current_area . height));
		[sg_current_filter setValue: [NSNumber numberWithFloat: p_time] forKey: @"inputTime"];
		[sg_current_context drawImage: [sg_current_filter valueForKey: @"outputImage"] atPoint: CGPointMake(sg_current_area . x, t_rect . bottom - (sg_current_area . y + sg_current_area . height)) fromRect: CGRectMake(0, 0, sg_current_area . width, sg_current_area . height)];
		CGContextFlush(sg_current_cg_context);
	NS_HANDLER
		t_result = false;
	NS_ENDHANDLER
	
	OBJC_LEAVE
	
	return t_result;
}

rei_boolean_t coreimage_visualeffect_end(void)
{
	rei_boolean_t t_result = false;
	
	OBJC_ENTER(false)
	
	NS_DURING
		if (sg_current_filter != NULL)
			[sg_current_filter release];
		if (sg_current_context != NULL)
			[sg_current_context release];
		if (sg_current_cg_context != NULL)
			QDEndCGContext(sg_current_graf_port, &sg_current_cg_context);
	NS_HANDLER
		t_result = false;
	NS_ENDHANDLER
	
	sg_current_filter = NULL;
	sg_current_context = NULL;
	sg_current_cg_context = NULL;
	
	OBJC_LEAVE

	return t_result;
}
