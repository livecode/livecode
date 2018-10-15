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

#include <position.h>
#include <report.h>
#include <literal.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

extern int IsDependencyCompile(void);

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
Debug(const char *p_format, ...)
{
	va_list t_args;

	if (s_verbose_level < 1)
		return;

	va_start(t_args, p_format);

	fprintf(stderr, "debug: ");
	vfprintf(stderr, p_format, t_args);
	fprintf(stderr, "\n");

	va_end(t_args);
}

void
Debug_Emit(const char *p_format, ...)
{
	va_list t_args;
	
	if (s_verbose_level < 1)
		return;

	va_start(t_args, p_format);

	fprintf(stderr, "debug: [Emit] ");
	vfprintf(stderr, p_format, t_args);
	fprintf(stderr, "\n");

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

static void _PrintPosition(intptr_t p_position)
{
    intptr_t t_row, t_column;
    FileRef t_file;
    const char *t_path;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    GetFileOfPosition(p_position, &t_file);
    GetFilePath(t_file, &t_path);
    fprintf(stderr, "%s:%ld:%ld: ", t_path, t_row, t_column);
}

/* Print the source code line that contains p_position, and another
 * line below it with a caret pointing "up" to the exact character
 * that the position specifies. */
static void _PrintContext(intptr_t p_position)
{
	const char *t_raw_text = NULL;
	char *t_text = NULL;
	intptr_t t_column;
	int i;
	GetRowTextOfPosition(p_position, &t_raw_text);
	GetColumnOfPosition(p_position, &t_column);

	if (NULL == t_raw_text)
		return;

	/* Replace ASCII control characters in the raw text with spaces.
	 * This has two goals: it simplifies aligning the context
	 * indicator caret, and it ensures that control characters in the
	 * LCB source code can't screw with your terminal emulator.  Note
	 * that this doesn't try to cope with invalid Unicode values. */
	t_text = strdup(t_raw_text);
	if (NULL == t_text)
		Fatal_OutOfMemory();
	for (i = 0; t_text[i] != 0; ++i)
	{
		if (t_text[i] < 0x20 || t_text[i] == 0x7F)
			t_text[i] = ' ';
	}

	fprintf(stderr, " %s\n %*c\n", t_text, (int)t_column, '^');
	free(t_text);
}

static void _Error(intptr_t p_position, const char *p_message)
{
    _PrintPosition(p_position);
    fprintf(stderr, "error: %s\n", p_message);
    _PrintContext(p_position);
    s_error_count += 1;
}

static void _Warning(intptr_t p_position, const char *p_message)
{
    if (IsDependencyCompile())
        return;

    if (s_is_werror_enabled)
    {
        _Error(p_position, p_message);
    }
    else
    {
        _PrintPosition(p_position);
        fprintf(stderr, "warning: %s\n", p_message);
        _PrintContext(p_position);
    }
}

static void _ErrorS(intptr_t p_position, const char *p_message, const char *p_string)
{
    intptr_t t_row, t_column;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    _PrintPosition(p_position);
    fprintf(stderr, "error: ");
    fprintf(stderr, p_message, p_string);
    fprintf(stderr, "\n");
    _PrintContext(p_position);
    s_error_count += 1;
}

static void _WarningS(intptr_t p_position, const char *p_message, const char *p_string)
{
    if (IsDependencyCompile())
        return;

    if (s_is_werror_enabled)
    {
       _ErrorS(p_position, p_message, p_string);
    }
    else
    {
        _PrintPosition(p_position);
        fprintf(stderr, "warning: ");
        fprintf(stderr, p_message, p_string);
        fprintf(stderr, "\n");
        _PrintContext(p_position);
    }
}

static void _ErrorI(intptr_t p_position, const char *p_message, NameRef p_name)
{
    const char *t_string;
    GetStringOfNameLiteral(p_name, &t_string);
    _ErrorS(p_position, p_message, t_string);
}

static void _WarningI(intptr_t p_position, const char *p_message, NameRef p_name)
{
	const char *t_string;
	GetStringOfNameLiteral(p_name, &t_string);
	_WarningS(p_position, p_message, t_string);
}

#define DEFINE_ERROR(Name, Message) \
    void Error_##Name(intptr_t p_position) { _Error(p_position, Message); }

#define DEFINE_ERROR_I(Name, Message) \
    void Error_##Name(intptr_t p_position, NameRef p_id) { _ErrorI(p_position, Message, p_id); }

#define DEFINE_ERROR_S(Name, Message) \
void Error_##Name(intptr_t p_position, const char *p_string) { _ErrorS(p_position, Message, p_string); }

DEFINE_ERROR_I(UnableToFindImportedPackage, "Unable to find imported package '%s'");
DEFINE_ERROR_I(UnableToFindImportedDefinition, "Unable to find imported definition '%s'");

DEFINE_ERROR(ClassesMayOnlyInheritFromClasses, "Classes may only inherit from classes");
DEFINE_ERROR(ClassesMayOnlyImplementInterfaces, "Classes may only implement interfaces");
DEFINE_ERROR(InterfacesMayOnlyInheritFromInterfaces, "Interfaces may only inherit from interfaces");

DEFINE_ERROR(GenericTypeMismatch, "Generic type parameters do not match definition");

DEFINE_ERROR_I(UnableToFindImportedModule, "Unable to find imported module '%s'");

DEFINE_ERROR_S(MalformedToken, "Illegal token '%s'");
DEFINE_ERROR_S(MalformedEscapedString, "Illegal escape in string '%s'");
DEFINE_ERROR(MalformedSyntax, "Syntax error");
DEFINE_ERROR(InvalidIntegerLiteral, "Malformed integer literal");
DEFINE_ERROR(InvalidDoubleLiteral, "Malformed real literal");
DEFINE_ERROR(IntegerLiteralOutOfRange, "Integer literal too big");
DEFINE_ERROR(DoubleLiteralOutOfRange, "Real literal too big");
DEFINE_ERROR(IllegalNamespaceOperator, "Namespace operator invalid in declaration context");
DEFINE_ERROR_I(InvalidNameForNamespace, "Namespace component '%s' must not begin with a digit")

DEFINE_ERROR_I(IdentifierPreviouslyDeclared, "Identifier '%s' already declared");
DEFINE_ERROR_I(IdentifierNotDeclared, "Identifier '%s' not declared");
DEFINE_ERROR_I(InvalidNameForSyntaxMarkVariable, "'%s' is not a valid name for a mark variable");
DEFINE_ERROR(SyntaxMarkVariableNotAllowedInDelimiter, "Mark variables are not allowed in delimiter clauses");
DEFINE_ERROR_I(SyntaxMarkVariableAlreadyDefined, "Mark variable '%s' has previously been assigned");

DEFINE_ERROR(ExpressionSyntaxCannotStartWithExpression, "Expression syntax cannot start with an expression");
DEFINE_ERROR(ExpressionSyntaxCannotFinishWithExpression, "Expression syntax cannot finish with an expression");
DEFINE_ERROR(PrefixSyntaxCannotStartWithExpression, "Prefix operator syntax cannot start with an expression");
DEFINE_ERROR(PrefixSyntaxMustFinishWithExpression, "Prefix syntax must finish with an expression");
DEFINE_ERROR(PostfixSyntaxMustStartWithExpression, "Postfix operator syntax must start with an expression");
DEFINE_ERROR(PostfixSyntaxCannotFinishWithExpression, "Postfix operator syntax cannot finish with an expression");
DEFINE_ERROR(BinarySyntaxMustStartWithExpression, "Binary operator syntax must start with an expression");
DEFINE_ERROR(BinarySyntaxMustFinishWithExpression, "Binary operator syntax must finish with an expression");
DEFINE_ERROR(ElementSyntaxCannotBeNullable, "Element clause in repetition cannot match nothing");
DEFINE_ERROR(OnlyKeywordsAllowedInDelimiterSyntax, "Delimiter clause in repetition must only contain keywords");
DEFINE_ERROR(SyntaxMarksMustBeConstant, "Syntax marks must be constant");
DEFINE_ERROR(OptionalSyntaxCannotContainOnlyMarks, "Optional syntax cannot just contain marks");
DEFINE_ERROR_I(SyntaxMarkVariableAlreadyDefinedWithDifferentType, "Mark variable '%s' has previously been assigned with different type");

DEFINE_ERROR_I(NotBoundToAHandler, "'%s' is not a handler")
DEFINE_ERROR_I(NotBoundToAVariable, "'%s' is not a variable")
DEFINE_ERROR_I(NotBoundToAVariableOrHandler, "'%s' is not a variable or handler")
DEFINE_ERROR_I(NotBoundToAConstantOrVariableOrHandler, "'%s' is not a constant, a variable nor a handler")
DEFINE_ERROR_I(NotBoundToAPhrase, "'%s' is not a syntax phrase")
DEFINE_ERROR_I(NotBoundToASyntaxMark, "'%s' is not a mark variable")
DEFINE_ERROR_I(NotBoundToASyntaxRule, "'%s' is not a syntax rule")
DEFINE_ERROR_I(NotBoundToAType, "'%s' is not a type")
DEFINE_ERROR_I(NotBoundToAConstantSyntaxValue, "'%s' must be assigned a constant value")

DEFINE_ERROR(TooManyArgumentsPassedToHandler, "Too many arguments for specified handler")
DEFINE_ERROR(TooFewArgumentsPassedToHandler, "Too few arguments for specified handler")
DEFINE_ERROR(HandlersBoundToSyntaxMustNotReturnAValue, "Handlers bound to syntax must not return a value")
DEFINE_ERROR(ConstantSyntaxArgumentMustBindToInParameter, "Constant syntax arguments must bind to in parameter")
DEFINE_ERROR(ContextSyntaxArgumentMustBindToInParameter, "'context' syntax argument must bind to in parameter")
DEFINE_ERROR(InputSyntaxArgumentMustBindToInParameter, "'input' syntax argument must bind to in parameter")
DEFINE_ERROR(OutputSyntaxArgumentMustBindToOutParameter, "'output' syntax argument must bind to out parameter")
DEFINE_ERROR(ContainerSyntaxArgumentMustBindToInParameter, "'container' syntax argument must bind to in parameter")
DEFINE_ERROR(IteratorSyntaxArgumentMustBindToInOutParameter, "'iterator' syntax argument must bind to inout parameter")
DEFINE_ERROR(PhraseBoundMarkSyntaxArgumentMustBindToInParameter, "Syntax mark argument which is of phrase type must bind to in parameter")
DEFINE_ERROR(VariableSyntaxArgumentMustBindToConsistentMode, "Syntax mark argument bound to parameters of different mode")

DEFINE_ERROR(SyntaxMethodArgumentsMustMatch, "Syntax method arguments must be in the same order as in the syntax rule")
DEFINE_ERROR(LSyntaxMethodArgumentsDontConform, "Assignment syntax method arguments must start with 'input' and then match order of syntax rule")
DEFINE_ERROR(RSyntaxMethodArgumentsDontConform, "Evaluate syntax method arguments must match order of syntax rule then end with 'output'")
DEFINE_ERROR(ExpressionSyntaxMethodArgumentsDontConform, "Expression syntax method arguments must either have 'input' first, or 'output' last (but not both)")
DEFINE_ERROR(IterateSyntaxMethodArgumentsDontConform, "Iterate syntax method arguments must start with 'iterator', end with 'container' and the rest must match order of syntax rule")

DEFINE_ERROR(HandlersBoundToSyntaxMustBePublic, "Handlers bound to syntax rules must be public")

DEFINE_ERROR(IterateSyntaxMethodMustReturnBoolean, "Iterate syntax methods must return boolean or CBool")
DEFINE_ERROR(PhraseSyntaxMethodMustReturnAValue, "Phrase syntax methods must return a value")

DEFINE_ERROR(NonAssignableExpressionUsedForOutContext, "Non-container expression used for out context")
DEFINE_ERROR(NonEvaluatableExpressionUsedForInContext, "Non-evaluatable expression used for in context")
DEFINE_ERROR(SyntaxNotAllowedInThisContext, "This syntax is not allowed for this module type")

DEFINE_ERROR(VariableMustHaveHighLevelType, "Inappropriate type for variable")
DEFINE_ERROR(ParameterMustHaveHighLevelType, "Inappropriate type for parameter")

DEFINE_ERROR_I(CannotAssignToHandlerId, "'%s' is a handler id and cannot be assigned to")
DEFINE_ERROR_I(CannotAssignToConstantId, "'%s' is a constant id and cannot be assigned to")

DEFINE_ERROR(NonHandlerTypeVariablesCannotBeCalled, "Variables must have handler type to be called")

DEFINE_ERROR(ConstantsMustBeSimple, "Constant definitions must be a literal expression")

DEFINE_ERROR_I(HandlerNotSuitableForPropertyGetter, "'%s' has inappropriate signature to be a property getter")
DEFINE_ERROR_I(HandlerNotSuitableForPropertySetter, "'%s' has inappropriate signature to be a property setter")

DEFINE_ERROR_I(DependentModuleNotIncludedWithInputs, "Module '%s' not found in input list")
DEFINE_ERROR_I(InterfaceFileNameMismatch, "Module '%s' has mismatched name in interface file")
               
DEFINE_ERROR_S(UnsuitableStringForKeyword, "Keyword '%s' is ambiguous with identifiers")

DEFINE_ERROR(NextRepeatOutOfContext, "'next repeat' must appear within a repeat")
DEFINE_ERROR(ExitRepeatOutOfContext, "'exit repeat' must appear within a repeat")

DEFINE_ERROR(NoReturnTypeSpecifiedForForeignHandler, "Foreign handlers must specify a return type")
DEFINE_ERROR(NoTypeSpecifiedForForeignHandlerParameter, "Foreign handler parameters must be typed")

DEFINE_ERROR(ConstantArrayKeyIsNotStringLiteral, "Array keys must be strings")
DEFINE_ERROR(ListExpressionTooLong, "List expressions can have at most 254 elements")
DEFINE_ERROR(ArrayExpressionTooLong, "Array expressions can have at most 127 keys")

DEFINE_ERROR_I(UnknownOpcode, "Unknown opcode '%s'")
DEFINE_ERROR(OpcodeArgumentMustBeLabel, "Opcode argument must be a label")
DEFINE_ERROR(OpcodeArgumentMustBeRegister, "Opcode argument must be a temporary variable, local variable or parameter variable")
DEFINE_ERROR(OpcodeArgumentMustBeConstant, "Opcode argument must be a literal expression")
DEFINE_ERROR(OpcodeArgumentMustBeHandler, "Opcode argument must be a handler id")
DEFINE_ERROR(OpcodeArgumentMustBeVariable, "Opcode argument must be a module variable")
DEFINE_ERROR(OpcodeArgumentMustBeDefinition, "Opcode argument must be a module variable, constant id or handler id")
DEFINE_ERROR(IllegalNumberOfArgumentsForOpcode, "Wrong number of arguments for opcode")

DEFINE_ERROR(BytecodeNotAllowedInSafeContext, "Bytecode blocks can only be present in unsafe context")
DEFINE_ERROR_I(UnsafeHandlerCallNotAllowedInSafeContext, "Unsafe handler '%s' can only be called in unsafe context")

DEFINE_ERROR(VariadicParameterMustBeLast, "Variadic parameter must be the last")
DEFINE_ERROR(VariadicParametersOnlyAllowedInForeignHandlers, "Variadic parameters only allowed in foreign handlers")
DEFINE_ERROR(VariadicArgumentNotExplicitlyTyped, "Variadic arguments must be an explicitly-typed variable")

#define DEFINE_WARNING(Name, Message) \
    void Warning_##Name(intptr_t p_position) { _Warning(p_position, Message); }
#define DEFINE_WARNING_I(Name, Message) \
    void Warning_##Name(intptr_t p_position, NameRef p_id) { _WarningI(p_position, Message, p_id); }
#define DEFINE_WARNING_S(Name, Message) \
	void Warning_##Name(intptr_t p_position, const char *p_string) { _WarningS(p_position, Message, p_string); }

DEFINE_WARNING(EmptyUnicodeEscape, "Unicode escape sequence specified with no nibbles")
DEFINE_WARNING(UnicodeEscapeTooBig, "Unicode escape sequence too big, replaced with U+FFFD");
DEFINE_WARNING_S(DeprecatedTypeName, "Deprecated type name: use '%s'")
DEFINE_WARNING_I(UnsuitableNameForDefinition, "All-lowercase name '%s' may cause future syntax error")
DEFINE_WARNING_I(UnsuitableNameForNamespace, "Non-lowercase namespace component '%s' may cause future syntax error")
DEFINE_WARNING_S(DeprecatedSyntax, "Deprecated syntax: %s")

////////////////////////////////////////////////////////////////////////////////

void yyerror(const char *p_text)
{
    intptr_t t_position;
    GetCurrentPosition(&t_position);
    _ErrorS(t_position, "Parsing error: %s", p_text);
}

////////////////////////////////////////////////////////////////////////////////
