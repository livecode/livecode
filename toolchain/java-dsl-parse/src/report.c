/* Copyright (C) 2016 LiveCode Ltd.
 
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

#include "report.h"
#include "position.h"
#include "literal.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

////////////////////////////////////////////////////////////////////////////////

static int s_error_count;
int s_verbose_level;
int s_is_werror_enabled;

void InitializeReports(void)
{
    s_error_count = 0;
    s_verbose_level = 0;
}

void FinalizeReports(void)
{
}

int ErrorsDidOccur(void)
{
    return s_error_count != 0 ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////

void Fatal_OutOfMemory(void)
{
    fprintf(stderr, "*** OUT OF MEMORY ***\n");
    exit(1);
}

void Fatal_InternalInconsistency(const char *p_message)
{
    fprintf(stderr, "*** INTERNAL INCONSISTENCY (%s) ***\n", p_message);
    exit(1);
}

////////////////////////////////////////////////////////////////////////////////

void
Debug_Emit(const char *p_format, ...)
{
	va_list t_args;
	
	//if (s_verbose_level < 1)
	//	return;

	va_start(t_args, p_format);

	fprintf(stdout, "debug: [Emit] ");
	vfprintf(stdout, p_format, t_args);
	fprintf(stdout, "\n");

	va_end(t_args);
}

void
Debug_Depend(const char *p_format, ...)
{
	va_list t_args;
	
	if (s_verbose_level < 1)
		return;
    
	va_start(t_args, p_format);
    
	fprintf(stderr, "debug: [Depend] ");
	vfprintf(stderr, p_format, t_args);
	fprintf(stderr, "\n");
    
	va_end(t_args);
}

////////////////////////////////////////////////////////////////////////////////

void Error_Bootstrap(const char *p_format, ...)
{
    va_list t_args;
    va_start(t_args, p_format);
    fprintf(stderr, "error: ");
    vfprintf(stderr, p_format, t_args);
    fprintf(stderr, "\n");
    va_end(t_args);
    s_error_count += 1;
}

////////////////////////////////////////////////////////////////////////////////

void Error_CouldNotGenerateBytecode (void)
{
	fprintf(stderr, "error: Could not generate bytecode\n");
	++s_error_count;
}

void Error_CouldNotOpenInputFile(const char *p_file)
{
    fprintf(stderr, "error: Could not open input file '%s'\n", p_file);
    s_error_count += 1;
}

void Error_CouldNotWriteOutputFile(const char *p_file)
{
	fprintf(stderr, "error: Could not write output file '%s'\n", p_file);
	++s_error_count;
}

void Error_CouldNotGenerateInterface (void)
{
	fprintf(stderr, "error: Could not generate interface\n");
	++s_error_count;
}

void Error_CouldNotWriteInterfaceFile(const char *p_file)
{
	fprintf(stderr, "error: Could not write interface file '%s'\n", p_file);
	++s_error_count;
}

static void _PrintPosition(long p_position)
{
    long t_row, t_column;
    FileRef t_file;
    const char *t_path;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    GetFileOfPosition(p_position, &t_file);
    GetFilePath(t_file, &t_path);
    fprintf(stderr, "%s:%ld:%ld: ", t_path, t_row, t_column);
}

static void _Error(long p_position, const char *p_message)
{
    _PrintPosition(p_position);
    fprintf(stderr, "error: %s\n", p_message);
    s_error_count += 1;
}

static void _Warning(long p_position, const char *p_message)
{
    if (s_is_werror_enabled)
    {
        _Error(p_position, p_message);
    }
    else
    {
        _PrintPosition(p_position);
        fprintf(stderr, "warning: %s\n", p_message);
    }
}

static void _ErrorS(long p_position, const char *p_message, const char *p_string)
{
    long t_row, t_column;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    _PrintPosition(p_position);
    fprintf(stderr, "error: ");
    fprintf(stderr, p_message, p_string);
    fprintf(stderr, "\n");
    s_error_count += 1;
}

static void _WarningS(long p_position, const char *p_message, const char *p_string)
{
    long t_row, t_column;
    
    if (s_is_werror_enabled)
    {
       _ErrorS(p_position, p_message, p_string);
    }
    else
    {
        GetColumnOfPosition(p_position, &t_column);
        GetRowOfPosition(p_position, &t_row);
        _PrintPosition(p_position);
        fprintf(stderr, "warning: ");
        fprintf(stderr, p_message, p_string);
        fprintf(stderr, "\n");
    }
}

static void _ErrorI(long p_position, const char *p_message, NameRef p_name)
{
    const char *t_string;
    GetStringOfNameLiteral(p_name, &t_string);
    _ErrorS(p_position, p_message, t_string);
}

static void _WarningI(long p_position, const char *p_message, NameRef p_name)
{
	const char *t_string;
	GetStringOfNameLiteral(p_name, &t_string);
	_WarningS(p_position, p_message, t_string);
}

#define DEFINE_ERROR(Name, Message) \
    void Error_##Name(long p_position) { _Error(p_position, Message); }

#define DEFINE_ERROR_I(Name, Message) \
    void Error_##Name(long p_position, NameRef p_id) { _ErrorI(p_position, Message, p_id); }

#define DEFINE_ERROR_S(Name, Message) \
    void Error_##Name(long p_position, const char *p_string) { _ErrorS(p_position, Message, p_string); }

DEFINE_ERROR_I(UnableToFindImportedPackage, "Unable to find imported package '%s'");
DEFINE_ERROR_I(UnableToFindImportedDefinition, "Unable to find imported definition '%s'");

DEFINE_ERROR(ClassesMayOnlyInheritFromClasses, "Classes may only inherit from classes");
DEFINE_ERROR(ClassesMayOnlyImplementInterfaces, "Classes may only implement interfaces");
DEFINE_ERROR(InterfacesMayOnlyInheritFromInterfaces, "Interfaces may only inherit from interfaces");

DEFINE_ERROR(GenericTypeMismatch, "Generic type parameters do not match definition");

DEFINE_ERROR(ConstantsMustBeSimple, "Constant definitions must be a literal expression")

DEFINE_ERROR_S(MalformedToken, "Illegal token '%s'");
DEFINE_ERROR_S(MalformedEscapedString, "Illegal escape in string '%s'");
DEFINE_ERROR(MalformedSyntax, "Syntax error");
DEFINE_ERROR(IntegerLiteralOutOfRange, "Integer literal too big");

DEFINE_ERROR_I(IdentifierPreviouslyDeclared, "Identifier '%s' already declared");
DEFINE_ERROR_I(IdentifierNotDeclared, "Identifier '%s' not declared");

DEFINE_ERROR_I(NotBoundToAHandler, "'%s' is not a handler")
DEFINE_ERROR_I(NotBoundToAVariable, "'%s' is not a variable")
DEFINE_ERROR_I(NotBoundToAVariableOrHandler, "'%s' is not a variable or handler")
DEFINE_ERROR_I(NotBoundToAConstantOrVariableOrHandler, "'%s' is not a constant, a variable nor a handler")
DEFINE_ERROR_I(NotBoundToAType, "'%s' is not a type")

#define DEFINE_WARNING(Name, Message) \
    void Warning_##Name(long p_position) { _Warning(p_position, Message); }
#define DEFINE_WARNING_I(Name, Message) \
    void Warning_##Name(long p_position, NameRef p_id) { _WarningI(p_position, Message, p_id); }
#define DEFINE_WARNING_S(Name, Message) \
	void Warning_##Name(long p_position, const char *p_string) { _WarningS(p_position, Message, p_string); }

DEFINE_WARNING(EmptyUnicodeEscape, "Unicode escape sequence specified with no nibbles")
DEFINE_WARNING(UnicodeEscapeTooBig, "Unicode escape sequence too big, replaced with U+FFFD");
DEFINE_WARNING_S(DeprecatedTypeName, "Deprecated type name: use '%s'")
DEFINE_WARNING_I(UnsuitableNameForDefinition, "All-lowercase name '%s' may cause future syntax error")
DEFINE_WARNING_S(DeprecatedSyntax, "Deprecated syntax: %s")

////////////////////////////////////////////////////////////////////////////////

void yyerror(const char *p_text)
{
    long t_position;
    GetCurrentPosition(&t_position);
    _ErrorS(t_position, "Parsing error: %s", p_text);
}

////////////////////////////////////////////////////////////////////////////////
