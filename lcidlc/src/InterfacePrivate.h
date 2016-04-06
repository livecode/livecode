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

#ifndef __INTERFACE_PRIVATE__
#define __INTERFACE_PRIVATE__

#ifndef __INTERFACE__
#include "Interface.h"
#endif

////////////////////////////////////////////////////////////////////////////////

struct Hook
{
	Position where;
	
	NameRef name;
	NameRef target;
};

struct EnumElement
{
	Position where;
	
	StringRef key;
	int64_t value;
};

struct Enum
{
	Position where;
	
	NameRef name;
	
	EnumElement *elements;
	uint32_t element_count;
};

typedef uint32_t Platform;
enum
{
	kPlatformMac,
	kPlatformWindows,
	kPlatformLinux,
	kPlatformIOS,
	kPlatformAndroid,
	
	__kPlatformCount__,
};

typedef uint32_t HandlerMapping;
enum
{
	kHandlerMappingNone,
	kHandlerMappingC,
	kHandlerMappingObjC,
	kHandlerMappingJava,
};

struct HandlerParameter
{
	Position where;
	
	ParameterType mode;
	NameRef name;
	NameRef type;
	// MERG-2013-06-14: [[ ExternalsApiV5 ]] Allow optional parameters without
	//   default.
    bool is_optional;
	ValueRef default_value;
};

struct HandlerVariant
{
	Position where;
	
	HandlerParameter *parameters;
	uint32_t parameter_count;
	uint32_t minimum_parameter_count;
	
	NameRef return_type;
	bool return_type_indirect;
	
	NameRef binding;
	
	HandlerMapping mappings[__kPlatformCount__];
};

struct Handler
{
	HandlerType type;
	/*bool is_java : 1;*/
	bool is_tail : 1;
	
	NameRef name;
	
	HandlerVariant *variants;
	uint32_t variant_count;
};

struct Interface
{
	bool invalid;

	Position where;
	
	NameRef name;
	NameRef qualified_name;

	bool use_cpp_exceptions : 1;
	bool use_cpp_naming : 1;
	bool use_objc_exceptions : 1;
	bool use_objc_objects : 1;
	
	HandlerMapping use_mappings[__kPlatformCount__];

	NameRef startup_hook;
	NameRef shutdown_hook;
	
	Enum *enums;
	uint32_t enum_count;
	
	Handler *handlers;
	uint32_t handler_count;
	
	Handler *current_handler;
};

////////////////////////////////////////////////////////////////////////////////

#endif
