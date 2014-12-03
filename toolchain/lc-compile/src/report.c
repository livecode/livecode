#include "report.h"
#include "position.h"
#include "literal.h"

#include <stdio.h>
#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////

static int s_error_count;

void InitializeReports(void)
{
    s_error_count = 0;
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

void Error_CouldNotOpenInputFile(const char *p_file)
{
    fprintf(stderr, "Could not open input file '%s'\n", p_file);
    s_error_count += 1;
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
    fprintf(stderr, "%s\n", p_message);
    s_error_count += 1;
}

static void _ErrorS(long p_position, const char *p_message, const char *p_string)
{
    long t_row, t_column;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    _PrintPosition(p_position);
    fprintf(stderr, p_message, p_string);
    fprintf(stderr, "\n");
    s_error_count += 1;
}

static void _ErrorI(long p_position, const char *p_message, NameRef p_name)
{
    const char *t_string;
    GetStringOfNameLiteral(p_name, &t_string);
    _ErrorS(p_position, p_message, t_string);
}

#define DEFINE_ERROR(Name, Message) \
    void Error_##Name(long p_position) { _Error(p_position, Message); }

#define DEFINE_ERROR_I(Name, Message) \
    void Error_##Name(long p_position, NameRef p_id) { _ErrorI(p_position, Message, p_id); }

#define DEFINE_ERROR_S(Name, Message) \
void Error_##Name(long p_position, const char *p_string) { _ErrorS(p_position, Message, p_string); }

DEFINE_ERROR_S(MalformedToken, "Illegal token '%s'");
DEFINE_ERROR(MalformedSyntax, "Syntax error");
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

DEFINE_ERROR(HandlersBoundToSyntaxMustBePublic, "Handlers bound to syntax rules must be public")

////////////////////////////////////////////////////////////////////////////////

void yyerror(const char *p_text)
{
    long t_position;
    GetCurrentPosition(&t_position);
    
    _PrintPosition(t_position);
    fprintf(stderr, "Parsing error - %s\n", p_text);
    
    s_error_count += 1;
}

////////////////////////////////////////////////////////////////////////////////
