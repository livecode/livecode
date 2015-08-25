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

#ifndef __INTERFACE__
#define __INTERFACE__

#ifndef __VALUE__
#include "Value.h"
#endif

#ifndef __POSITION__
#include "Position.h"
#endif

typedef struct Interface *InterfaceRef;

enum HandlerType
{
	kHandlerTypeCommand,
	kHandlerTypeFunction,
	kHandlerTypeTailCommand,
	kHandlerTypeTailFunction,
	kHandlerTypeJava,
	kHandlerTypeNative,
};

enum ParameterType
{
	kParameterTypeIn,
	kParameterTypeOut,
	kParameterTypeInOut,
	kParameterTypeRef,
};

bool InterfaceCreate(InterfaceRef& interface);
void InterfaceDestroy(InterfaceRef interface);

bool InterfaceBegin(InterfaceRef interface, Position where, NameRef name);
bool InterfaceDefineUse(InterfaceRef interface, Position where, NameRef use);
bool InterfaceDefineHook(InterfaceRef interface, Position where, NameRef handler, NameRef target);

bool InterfaceBeginEnum(InterfaceRef interface, Position where, NameRef name);
bool InterfaceDefineEnumElement(InterfaceRef interface, Position where, StringRef element, ValueRef value);
bool InterfaceEndEnum(InterfaceRef interface);

bool InterfaceBeginHandler(InterfaceRef interface, Position where, HandlerType type, NameRef name);
bool InterfaceDefineHandlerParameter(InterfaceRef interface, Position where, ParameterType param_type, NameRef name, NameRef type, ValueRef default_value);
bool InterfaceDefineHandlerReturn(InterfaceRef interface, Position where, NameRef type, bool indirect);
bool InterfaceDefineHandlerBinding(InterfaceRef interface, Position where, NameRef name);
bool InterfaceEndHandler(InterfaceRef interface);

bool InterfaceEnd(InterfaceRef interface);

bool InterfaceGenerate(InterfaceRef interface, const char *filename);

#endif
