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

#ifndef __REI_VISUALEFFECT_H
#define __REI_VISUALEFFECT_H

typedef void *rei_handle_t;
typedef unsigned char rei_uint8_t;
typedef unsigned short rei_uint16_t;
typedef unsigned int rei_uint32_t;
typedef unsigned int rei_boolean_t;
typedef const char *rei_identifier_t;
typedef double rei_real64_t;

typedef struct __rei_rectangle_t rei_rectangle_t;
typedef rei_rectangle_t *rei_rectangle_ref_t;
struct __rei_rectangle_t
{
	rei_uint32_t x, y;
	rei_uint32_t width, height;
};

typedef struct __rei_imagedata_t rei_imagedata_t;
typedef rei_imagedata_t *rei_imagedata_ref_t;
struct __rei_imagedata_t
{
	rei_uint16_t width, height;
	rei_uint32_t *data;
};

enum __rei_visualeffect_parameter_type_t
{
	REI_VISUALEFFECT_PARAMETER_TYPE_UNDEFINED,
	REI_VISUALEFFECT_PARAMETER_TYPE_NUMBER,
	REI_VISUALEFFECT_PARAMETER_TYPE_STRING,
	REI_VISUALEFFECT_PARAMETER_TYPE_COLOUR,
	REI_VISUALEFFECT_PARAMETER_TYPE_VECTOR,
	REI_VISUALEFFECT_PARAMETER_TYPE_IMAGE
};
typedef enum __rei_visualeffect_parameter_type_t rei_visualeffect_parameter_type_t;

typedef struct __rei_visualeffect_parameter_info_t rei_visualeffect_parameter_info_t;
struct __rei_visualeffect_parameter_info_t
{
	rei_identifier_t name;
	rei_visualeffect_parameter_type_t type;
};

typedef struct __rei_visualeffect_info_t rei_visualeffect_info_t;
typedef rei_visualeffect_info_t *rei_visualeffect_info_ref_t;

struct __rei_visualeffect_info_t
{
	rei_handle_t handle;
	rei_uint32_t parameter_count;
	rei_visualeffect_parameter_info_t parameters[0];
};

typedef struct __rei_visualeffect_parameter_t rei_visualeffect_parameter_t;
typedef rei_visualeffect_parameter_t *rei_visualeffect_parameter_ref_t;
struct __rei_visualeffect_parameter_t
{
	rei_identifier_t name;
	rei_visualeffect_parameter_type_t type;
	union
	{
		rei_real64_t number;
		const char *string;
		struct
		{
			rei_uint32_t length;
			rei_real64_t *coefficients;
		} vector;
		struct
		{
			rei_uint8_t red;
			rei_uint8_t green;
			rei_uint8_t blue;
			rei_uint8_t alpha;
		} colour;
		rei_imagedata_t image;
	} value;
};

typedef struct __rei_visualeffect_parameter_list_t rei_visualeffect_parameter_list_t;
typedef rei_visualeffect_parameter_list_t *rei_visualeffect_parameter_list_ref_t;
struct __rei_visualeffect_parameter_list_t
{
	rei_uint32_t length;
	rei_visualeffect_parameter_t entries[0];
};

//typedef rei_boolean_t (*rei_visualeffect_initialiser_t)(void);
//typedef rei_boolean_t (*rei_visualeffect_finaliser_t)(void);
//typedef rei_boolean_t (*rei_visualeffect_lookup_method_t)(const char *p_name, rei_visualeffect_info_ref_t *r_info);
//typedef rei_boolean_t (*rei_visualeffect_begin_method_t)(rei_handle_t p_handle, CGrafPtr p_target, CGrafPtr p_source_a, CGrafPtr p_source_b, rei_rectangle_ref_t p_area, rei_visualeffect_parameters_ref_t p_parameters);
//typedef rei_boolean_t (*rei_visualeffect_step_method_t)(float p_time);
//typedef rei_boolean_t (*rei_visualeffect_end_method_t)(void);


#endif
