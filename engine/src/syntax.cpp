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

#include "syntax.h"

////////////////////////////////////////////////////////////////////////////////

struct MCSyntaxFactoryMethod
{
	integer_t line;
	integer_t pos;
	uindex_t operand_start;
};

struct MCSyntaxFactory
{
	uindex_t operand_count;
	uindex_t *operand_stack;
	uindex_t operand_capacity;
	uindex_t operand_index;
	
	MCSyntaxFactoryMethod *method_stack;
	uindex_t method_capacity;
	uindex_t method_index;
	
	MCStringRef log;
};

////////////////////////////////////////////////////////////////////////////////

static void _log_begin(MCSyntaxFactoryRef self, const char *tag, ...);
static void _log_end(MCSyntaxFactoryRef self, const char *tag, ...);
static void _log(MCSyntaxFactoryRef self, const char *format, ...);

////////////////////////////////////////////////////////////////////////////////

bool MCSyntaxFactoryCreate(MCSyntaxFactoryRef& r_self)
{
	MCMemoryNew(r_self);
	MCStringCreateMutable(0, r_self -> log);
	return true;
}

void MCSyntaxFactoryDestroy(MCSyntaxFactoryRef self)
{
	MCMemoryDelete(self);
}

void MCSyntaxFactoryCopyLog(MCSyntaxFactoryRef self, MCStringRef& r_log)
{
	MCStringCopy(self -> log, r_log);
}

bool MCSyntaxFactoryIsValid(MCSyntaxFactoryRef self)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void MCSyntaxFactoryMark(MCSyntaxFactoryRef self, integer_t line, integer_t pos)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCSyntaxFactoryBeginHandlerList(MCSyntaxFactoryRef self)
{
	_log_begin(self, "begin handler list");
}

void MCSyntaxFactoryEndHandlerList(MCSyntaxFactoryRef self)
{
	_log_end(self, "end handler list");
}

void MCSyntaxFactoryDefineGlobal(MCSyntaxFactoryRef self, MCNameRef p_name)
{
	_log(self, "global %@", p_name);
}

void MCSyntaxFactoryDefineLocal(MCSyntaxFactoryRef self, MCNameRef p_name, MCValueRef p_value)
{
	if (p_value == nil)
		_log(self, "local %@", p_name);
	else
	{
		MCAutoStringRef t_value;
		MCValueCopyDescription(p_value, &t_value);
		_log(self, "local %@ = \"%@\"", p_name, *t_value);
	}
}

void MCSyntaxFactoryDefineConstant(MCSyntaxFactoryRef self, MCNameRef p_name, MCValueRef p_value)
{
	MCAutoStringRef t_value;
	MCValueCopyDescription(p_value, &t_value);
	_log(self, "local %@ = \"%@\"", p_name, *t_value);

}

////////////////////////////////////////////////////////////////////////////////

void MCSyntaxFactoryBeginHandler(MCSyntaxFactoryRef self, MCSyntaxHandlerType p_type, MCNameRef p_name)
{
	static const char *s_handler_types[] =
	{
		"", "on", "before", "after", "command", "private command", "function", "private function", "set prop", "get prop"
	};
	self -> operand_count = 0;
	self -> operand_index = 0;
	_log_begin(self, "begin %s handler %@", s_handler_types[p_type], p_name);
}

void MCSyntaxFactoryEndHandler(MCSyntaxFactoryRef self)
{
	_log_end(self, "end handler");
}

void MCSyntaxFactoryDefineParameter(MCSyntaxFactoryRef self, MCNameRef p_name, bool p_is_ref)
{
	_log(self, "%sparam %@", p_is_ref ? "ref " : "", p_name);
}

////////////////////////////////////////////////////////////////////////////////

static void MCSyntaxFactoryPushOperand(MCSyntaxFactoryRef self)
{
	if (self -> operand_index + 1 > self -> operand_capacity)
		/* UNCHECKED */ MCMemoryResizeArray((self -> operand_capacity == 0 ? 16 : self -> operand_capacity) * 2, self -> operand_stack, self -> operand_capacity);
	
	self -> operand_stack[self -> operand_index++] = self -> operand_count++;
}

void MCSyntaxFactoryEvalConstant(MCSyntaxFactoryRef self, MCValueRef p_value)
{
	MCAutoStringRef t_value_string;
	/* UNCHECKED */ MCValueCopyDescription(p_value, &t_value_string);
	_log(self, "%d: push value(%@)", self -> operand_count, *t_value_string);
	MCSyntaxFactoryPushOperand(self);
}

void MCSyntaxFactoryEvalConstantOldString(MCSyntaxFactoryRef self, const MCString& p_value)
{
	MCAutoStringRef t_value_string;
	/* UNCHECKED */ MCStringCreateWithNativeChars((const char_t *)p_value . getstring(), p_value . getlength(), &t_value_string);
	MCSyntaxFactoryEvalConstant(self, *t_value_string);
}

void MCSyntaxFactoryEvalConstantInt(MCSyntaxFactoryRef self, integer_t p_value)
{
	_log(self, "%d: push int(%d)", self -> operand_count, p_value);
	MCSyntaxFactoryPushOperand(self);
}

void MCSyntaxFactoryEvalConstantUInt(MCSyntaxFactoryRef self, uinteger_t p_value)
{
	_log(self, "%d: push uint(%u)", self -> operand_count, p_value);
	MCSyntaxFactoryPushOperand(self);
}

void MCSyntaxFactoryEvalConstantDouble(MCSyntaxFactoryRef self, double p_value)
{
	_log(self, "%d: push double(%lf)", self -> operand_count, p_value);
	MCSyntaxFactoryPushOperand(self);
}

void MCSyntaxFactoryEvalConstantLegacyPoint(MCSyntaxFactoryRef self, MCPoint p_value)
{
	_log(self, "%d: push point (%d, %d)", self -> operand_count, p_value . x, p_value . y);
	MCSyntaxFactoryPushOperand(self);
}

void MCSyntaxFactoryEvalConstantLegacyRectangle(MCSyntaxFactoryRef self, MCRectangle p_value)
{
	_log(self, "%d: push rectangle (%d, %d, %u, &u)", self -> operand_count, p_value . x, p_value . y, p_value . width, p_value . height);
	MCSyntaxFactoryPushOperand(self);
}

void MCSyntaxFactoryEvalConstantEnum(MCSyntaxFactoryRef self, const MCExecEnumTypeInfo *p_enum_info, intenum_t p_value)
{
	_log(self, "%d: push enum %s(%d)", self -> operand_count, p_enum_info -> name, p_value);
	MCSyntaxFactoryPushOperand(self);
}

void MCSyntaxFactoryEvalConstantNil(MCSyntaxFactoryRef self)
{
	_log(self, "%d: push nil", self -> operand_count);
	MCSyntaxFactoryPushOperand(self);
}

void MCSyntaxFactoryEvalConstantBool(MCSyntaxFactoryRef self, bool p_value)
{
	_log(self, "%d: push bool(%s)", self -> operand_count, p_value ? "true" : "false");
	MCSyntaxFactoryPushOperand(self);
}

////////////////////////////////////////////////////////////////////////////////

static void MCSyntaxFactoryPushMethod(MCSyntaxFactoryRef self, integer_t p_line, integer_t p_pos)
{
	if (self -> method_index + 1 > self -> method_capacity)
		MCMemoryResizeArray((self -> method_capacity == 0 ? 16 : self -> method_capacity) * 2, self -> method_stack, self -> method_capacity);
	
	self -> method_stack[self -> method_index] . line = p_line;
	self -> method_stack[self -> method_index] . pos = p_pos;
	self -> method_stack[self -> method_index] . operand_start = self -> operand_index;
	self -> method_index++;
}

static void MCSyntaxFactoryPopMethod(MCSyntaxFactoryRef self)
{
	self -> operand_index = self -> method_stack[self -> method_index - 1] . operand_start;
	self -> method_index -= 1;
}

void MCSyntaxFactoryBeginStatement(MCSyntaxFactoryRef self, integer_t line, integer_t pos)
{
	_log_begin(self, "begin statement");
	MCSyntaxFactoryPushMethod(self, line, pos);
}

void MCSyntaxFactoryEndStatement(MCSyntaxFactoryRef self)
{
	_log_end(self, "end statement");
	MCSyntaxFactoryPopMethod(self);
}

void MCSyntaxFactoryBeginExpression(MCSyntaxFactoryRef self, integer_t line, integer_t pos)
{
	_log_begin(self, "begin expression");
	MCSyntaxFactoryPushOperand(self);
	MCSyntaxFactoryPushMethod(self, line, pos);
}

void MCSyntaxFactoryEndExpression(MCSyntaxFactoryRef self)
{
	_log_end(self, "end expression");
	MCSyntaxFactoryPopMethod(self);
}

void MCSyntaxFactoryExecUnimplemented(MCSyntaxFactoryRef self)
{
	_log(self, "-: exec unimplemented");
}

void MCSyntaxFactoryExecMethod(MCSyntaxFactoryRef self, const MCExecMethodInfo *p_method)
{
	MCAutoListRef t_list;
	MCListCreateMutable(',', &t_list);
	for(uindex_t i = 0; i < p_method -> arity; i++)
		MCListAppendInteger(*t_list, self -> operand_stack[self -> method_stack[self -> method_index - 1] . operand_start + i]);
	
	MCAutoStringRef t_list_string;
	MCListCopyAsString(*t_list, &t_list_string);
	_log(self, "-: exec %s(%@)", p_method -> name, *t_list_string);
}

void MCSyntaxFactoryExecMethodWithArgs(MCSyntaxFactoryRef self, const MCExecMethodInfo *p_method, ...)
{
	va_list t_args;
	va_start(t_args, p_method);
	
	MCAutoListRef t_list;
	MCListCreateMutable(',', &t_list);
	for(uindex_t i = 0; i < p_method -> arity; i++)
		MCListAppendInteger(*t_list, self -> operand_stack[self -> method_stack[self -> method_index - 1] . operand_start + va_arg(t_args, int)]);
	
	va_end(t_args);
	
	MCAutoStringRef t_list_string;
	MCListCopyAsString(*t_list, &t_list_string);
	_log(self, "-: exec %s(%@)", p_method -> name, *t_list_string);
}

void MCSyntaxFactoryEvalUnimplemented(MCSyntaxFactoryRef self)
{
	_log(self, "%d: eval unimplemented", self -> operand_stack[self -> method_stack[self -> method_index - 1] . operand_start - 1]);
	//MCSyntaxFactoryPushOperand(self);
}

void MCSyntaxFactoryEvalMethod(MCSyntaxFactoryRef self, const MCExecMethodInfo *p_method)
{
	MCAutoListRef t_list;
	MCListCreateMutable(',', &t_list);
	for(uindex_t i = 0; i < p_method -> arity - 1; i++)
		MCListAppendInteger(*t_list, self -> operand_stack[self -> method_stack[self -> method_index - 1] . operand_start + i]);
	
	MCAutoStringRef t_list_string;
	MCListCopyAsString(*t_list, &t_list_string);
	_log(self, "%d: eval %s(%@)", self -> operand_stack[self -> method_stack[self -> method_index - 1] . operand_start - 1], p_method -> name, *t_list_string);
}

void MCSyntaxFactoryEvalMethodWithArgs(MCSyntaxFactoryRef self, const MCExecMethodInfo *p_method, ...)
{
	va_list t_args;
	va_start(t_args, p_method);
	
	MCAutoListRef t_list;
	MCListCreateMutable(',', &t_list);
	for(uindex_t i = 0; i < p_method -> arity - 1; i++)
		MCListAppendInteger(*t_list, self -> operand_stack[self -> method_stack[self -> method_index - 1] . operand_start + va_arg(t_args, int)]);
	
	va_end(t_args);
	
	MCAutoStringRef t_list_string;
	MCListCopyAsString(*t_list, &t_list_string);
	_log(self, "%d: eval %s(%@)", self -> operand_stack[self -> method_stack[self -> method_index - 1] . operand_start - 1], p_method -> name, *t_list_string);
}

void MCSyntaxFactoryEvalList(MCSyntaxFactoryRef self, uindex_t p_count)
{
	MCAutoListRef t_list;
	MCListCreateMutable(',', &t_list);
	for(uindex_t i = 0; i < p_count; i++)
		MCListAppendInteger(*t_list, self -> operand_index - p_count + i);
	
	MCAutoStringRef t_list_string;
	MCListCopyAsString(*t_list, &t_list_string);
	_log(self, "%d: push list(%@)", self -> operand_count, *t_list_string);
	
}

void MCSyntaxFactoryEvalResult(MCSyntaxFactoryRef self)
{
	_log(self, "%d: return %d", self -> operand_stack[self -> method_stack[self -> method_index - 1] . operand_start - 1], self -> operand_stack[self -> operand_index - 1]);
}

////////////////////////////////////////////////////////////////////////////////

static int _log_indent = 0;

static void _log_begin(MCSyntaxFactoryRef self, const char *p_format, ...)
{
	va_list t_args;
	va_start(t_args, p_format);
	MCAutoStringRef t_formatted_string;
	MCStringFormatV(&t_formatted_string, p_format, t_args);
	va_end(t_args);

	_log(self, "%@", *t_formatted_string);
	_log_indent += 2;
}

static void _log_end(MCSyntaxFactoryRef self, const char *p_format, ...)
{
	va_list t_args;
	va_start(t_args, p_format);
	MCAutoStringRef t_formatted_string;
	MCStringFormatV(&t_formatted_string, p_format, t_args);
	va_end(t_args);
	
	_log_indent -= 2;
	_log(self, "%@", *t_formatted_string);
}

static void _log(MCSyntaxFactoryRef self, const char *p_format, ...)
{
	static const char *s_spaces = "                                ";
	
	MCStringAppendFormat(self -> log, "%.*s", _log_indent, s_spaces);
	
	va_list t_args;
	va_start(t_args, p_format);
	MCStringAppendFormatV(self -> log, p_format, t_args);
	va_end(t_args);
	
	MCStringAppendNativeChar(self -> log, '\n');
}

////////////////////////////////////////////////////////////////////////////////
