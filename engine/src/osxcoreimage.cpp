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

#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "player.h"
#include "field.h"
#include "stacklst.h"
#include "cardlst.h"
#include "sellst.h"
#include "mcerror.h"
#include "util.h"
#include "param.h"

#include "debug.h"
#include "globals.h"
#include "visual.h"
#include "image.h"

#include "visualeffect.h"

static bool s_coreimage_initialized = false;

extern rei_boolean_t coreimage_visualeffect_initialise(void);
extern void coreimage_visualeffect_finalise(void);
extern rei_boolean_t coreimage_visualeffect_lookup(const char *p_name, rei_visualeffect_info_ref_t *r_info);
extern rei_boolean_t coreimage_visualeffect_begin(rei_handle_t p_handle, MCGImageRef p_image_a, MCGImageRef p_image_b, rei_rectangle_ref_t p_area, float p_surface_height, rei_visualeffect_parameter_list_ref_t p_parameters);
extern rei_boolean_t coreimage_visualeffect_step(MCStackSurface *p_target, float p_time);
extern rei_boolean_t coreimage_visualeffect_end(void);

void MCCoreImageRegister(void)
{
	s_coreimage_initialized = coreimage_visualeffect_initialise();
}

void MCCoreImageUnregister(void)
{
	if (s_coreimage_initialized)
	{
		coreimage_visualeffect_finalise();
		s_coreimage_initialized = false;
	}
}

// IM-2013-08-29: [[ RefactorGraphics ]] add surface height param to pass through to coreimage_visualeffect_begin
bool MCCoreImageEffectBegin(const char *p_name, MCGImageRef p_source_a, MCGImageRef p_source_b, const MCRectangle& p_rect, MCGFloat p_surface_height, MCEffectArgument *p_arguments)
{	
	if (!s_coreimage_initialized)
		return false;
	
	rei_visualeffect_info_ref_t t_info;
	if (!coreimage_visualeffect_lookup(p_name, &t_info))
		return false;

	rei_rectangle_t t_rect;
	t_rect . x = p_rect . x;
	t_rect . y = p_rect . y;
	t_rect . width = p_rect . width;
	t_rect . height = p_rect . height;
	
	unsigned int t_argument_count = 0;
	for(MCEffectArgument *t_argument = p_arguments; t_argument != NULL; t_argument = t_argument -> next)
		t_argument_count += 1;
	
	rei_visualeffect_parameter_list_ref_t t_parameters;
	t_parameters = (rei_visualeffect_parameter_list_ref_t)alloca(sizeof(rei_visualeffect_parameter_list_t) + sizeof(rei_visualeffect_parameter_t) * t_argument_count);
	if (t_parameters == NULL)
		return false;
	
	unsigned int t_index = 0;
	
	t_parameters -> length = t_argument_count;
	for(unsigned int t_parameter = 0; t_parameter < t_info -> parameter_count; ++t_parameter)
	{
		rei_visualeffect_parameter_type_t t_type = REI_VISUALEFFECT_PARAMETER_TYPE_UNDEFINED;
		rei_identifier_t t_name = NULL;
		bool t_success = true;
		MCEffectArgument *t_argument;
		
		for(t_argument = p_arguments; t_argument != NULL; t_argument = t_argument -> next)
            // AL-2014-08-14: [[ Bug 13176 ]] argument key is caseless comparison
			if (MCStringIsEqualToCString(t_argument -> key, t_info -> parameters[t_parameter] . name, kMCStringOptionCompareCaseless))
			{
				t_name = t_info -> parameters[t_parameter] . name;
				t_type = t_info -> parameters[t_parameter] . type;
				break;
			}
				
		if (t_type == REI_VISUALEFFECT_PARAMETER_TYPE_UNDEFINED)
			continue;
			
		t_parameters -> entries[t_index] . name = t_name;
		t_parameters -> entries[t_index] . type = t_type;
			
		switch(t_type)
		{
			case REI_VISUALEFFECT_PARAMETER_TYPE_NUMBER:
                /* UNCHECKED */ MCStringToDouble(t_argument -> value, t_parameters -> entries[t_index] . value . number);
			break;
			
			case REI_VISUALEFFECT_PARAMETER_TYPE_STRING:
            {
                char *t_value;
                /* UNCHECKED */ MCStringConvertToCString(t_argument -> value, t_value);
				t_parameters -> entries[t_index] . value . string = t_value;
                delete t_value;
            }
			break;
			
			case REI_VISUALEFFECT_PARAMETER_TYPE_COLOUR:
			{
				MCColor t_colour;
				if (MCscreen -> parsecolor(t_argument -> value, t_colour, NULL))
				{
					t_parameters -> entries[t_index] . value . colour . red = t_colour . red >> 8;
					t_parameters -> entries[t_index] . value . colour . green = t_colour . green >> 8;
					t_parameters -> entries[t_index] . value . colour . blue = t_colour . blue >> 8;
					t_parameters -> entries[t_index] . value . colour . alpha = 255;
				}
				else
					t_success = false;
			}
			break;
			
			case REI_VISUALEFFECT_PARAMETER_TYPE_VECTOR:
			{
				double t_vector[4];
				unsigned int t_count = 0;
				uindex_t t_pos = 0;
				uindex_t t_comma, t_length;
                t_length = MCStringGetLength(t_argument -> value);
                // AL-2014-08-14: [[ Bug 13176 ]] Parse last vector element correctly  
				do
				{
					if (!MCStringFirstIndexOfChar(t_argument -> value, ',', t_pos, kMCCompareExact, t_comma))
						t_comma = t_length;
					MCAutoStringRef t_substring;
					if (!MCStringCopySubstring(t_argument -> value, MCRangeMakeMinMax(t_pos, t_comma), &t_substring))
						t_success = false;

					if (!MCStringToDouble(*t_substring, t_vector[t_count]))
						t_success = false;
					
					t_pos = t_comma + 1;
					t_count += 1;
				}
				while(t_success && t_count < 4 && t_comma < t_length);
				if (t_success)
				{
					t_parameters -> entries[t_index] . value . vector . length = t_count;
					
					double *t_new_vector;
					t_new_vector = (double *)malloc(sizeof(double) * t_count);
					for(unsigned int t_coeff = 0; t_coeff < t_count; ++t_coeff)
						t_new_vector[t_coeff] = t_vector[t_coeff];
						
					t_parameters -> entries[t_index] . value . vector . coefficients = t_new_vector;
				}
			}
			break;
			
			case REI_VISUALEFFECT_PARAMETER_TYPE_IMAGE:
				MCImage *t_image;
				if (MCStringBeginsWith(t_argument -> value, MCSTR("id "), kMCStringOptionCompareCaseless))
                {
                    MCAutoStringRef t_id;
                    /* UNCHECKED */ MCStringCopySubstring(t_argument -> value, MCRangeMakeMinMax(3, MCStringGetLength(t_argument -> value)), &t_id); 
                    integer_t t_int;
                    /* UNCHECKED */ MCStringToInteger(*t_id, t_int);
					t_image = (MCImage *)(MCdefaultstackptr -> getobjid(CT_IMAGE, t_int));
                }
				else
                {
                    MCNewAutoNameRef t_value;
                    /* UNCHECKED */ MCNameCreate(t_argument -> value, &t_value);
					t_image = (MCImage *)(MCdefaultstackptr -> getobjname(CT_IMAGE, *t_value));
                }
				if (t_image != NULL)
				{
					MCRectangle t_image_rect;
					t_image_rect = t_image -> getrect();
					
					rei_uint32_t *t_data;
					t_data = (rei_uint32_t *)malloc(t_image_rect . width * t_image_rect . height * 4);
					if (t_data != NULL)
					{
						MCImageBitmap *t_bitmap = nil;
						if (t_image->lockbitmap(t_bitmap, true))
							MCImageBitmapPremultiplyRegion(t_bitmap, 0, 0, t_bitmap->width, t_bitmap->height, t_image_rect.width * sizeof(uint32_t), t_data);
						else
							MCMemoryClear(t_data, t_image_rect.width * sizeof(uint32_t) * t_image_rect.height);
						t_image->unlockbitmap(t_bitmap);
						
						t_parameters -> entries[t_index] . value . image . width = t_image_rect . width;
						t_parameters -> entries[t_index] . value . image . height = t_image_rect . height;
						t_parameters -> entries[t_index] . value . image . data = t_data;
					}
					else
						t_success = false;
				}
				else
					t_success = false;
			break;
			
			default:
				t_success = false;
			break;
		}
		
		if (!t_success)
			break;
			
		t_index += 1;
	}
	
	if (t_index != t_argument_count)
	{
		for(unsigned int t_arg = 0; t_arg < t_index; ++t_arg)
			if (t_parameters -> entries[t_arg] . type == REI_VISUALEFFECT_PARAMETER_TYPE_IMAGE)
				free(t_parameters -> entries[t_arg] . value . image . data);
	}
	else
	{
	  if (!coreimage_visualeffect_begin(t_info -> handle, p_source_a, p_source_b, &t_rect, p_surface_height, t_parameters))
			return false;
	}
	
	return true;
}

bool MCCoreImageEffectStep(MCStackSurface *p_target, float p_time)
{
	return coreimage_visualeffect_step(p_target, p_time);
}

void MCCoreImageEffectEnd(void)
{
	coreimage_visualeffect_end();
}
