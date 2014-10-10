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

void Error_MalformedToken(long p_position, const char *p_token)
{
    long t_row, t_column;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    fprintf(stderr, "row %ld, col %ld: Illegal token '%s'\n", t_row, t_column, p_token);
    
    s_error_count += 1;
}

void Error_MalformedSyntax(long p_position)
{
    long t_row, t_column;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    fprintf(stderr, "row %ld, col %ld: Syntax error\n", t_row, t_column);
    
    s_error_count += 1;
}

void Error_IdentifierPreviouslyDeclared(long p_position, NameRef p_name, long p_previous_position)
{
    long t_row, t_column;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    fprintf(stderr, "row %ld, col %ld: Identifier already declared\n", t_row, t_column);
    
    s_error_count += 1;
}

void Error_IdentifierNotDeclared(long p_position, NameRef p_name)
{
    long t_row, t_column;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    const char *t_string;
    GetStringOfNameLiteral(p_name, &t_string);
    fprintf(stderr, "row %ld, col %ld: Identifier '%s' not declared\n", t_row, t_column, t_string);
    
    s_error_count += 1;
}

void Error_InvalidNameForSyntaxMarkVariable(long p_position, NameRef p_name)
{
    long t_row, t_column;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    const char *t_string;
    GetStringOfNameLiteral(p_name, &t_string);
    fprintf(stderr, "row %ld, col %ld: '%s' is not a valid name for a mark variable\n", t_row, t_column, t_string);
    
    s_error_count += 1;
}

void Error_SyntaxMarkVariableNotAllowedInDelimiter(long p_position)
{
    long t_row, t_column;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    fprintf(stderr, "row %ld, col %ld: Mark variables are not allowed in delimiter clauses\n", t_row, t_column);
    
    s_error_count += 1;
}

void Error_SyntaxMarkVariableAlreadyDefined(long p_position, NameRef p_name)
{
    long t_row, t_column;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    const char *t_string;
    GetStringOfNameLiteral(p_name, &t_string);
    fprintf(stderr, "row %ld, col %ld: Mark variable '%s' has previously been assigned\n", t_row, t_column, t_string);
    
    s_error_count += 1;
}

void Error_CouldNotOpenInputFile(const char *p_file)
{
    fprintf(stderr, "Could not open input file '%s'\n", p_file);
    s_error_count += 1;
}

static void _Error(long p_position, const char *p_message)
{
    long t_row, t_column;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    fprintf(stderr, "row %ld, col %ld: %s\n", t_row, t_column, p_message);
    s_error_count += 1;
}

static void _ErrorI(long p_position, const char *p_message, NameRef p_name)
{
    long t_row, t_column;
    GetColumnOfPosition(p_position, &t_column);
    GetRowOfPosition(p_position, &t_row);
    const char *t_string;
    GetStringOfNameLiteral(p_name, &t_string);
    fprintf(stderr, "row %ld, col %ld:", t_row, t_column);
    fprintf(stderr, p_message, t_string);
    fprintf(stderr, "\n");
    s_error_count += 1;
}

#define DEFINE_ERROR(Name, Message) \
    void Error_##Name(long p_position) { _Error(p_position, Message); }

#define DEFINE_ERROR_I(Name, Message) \
    void Error_##Name(long p_position, NameRef p_id) { _ErrorI(p_position, Message, p_id); }

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

DEFINE_ERROR_I(NotBoundToAHandler, "'%s' is not a handler")
DEFINE_ERROR_I(NotBoundToAPhrase, "'%s' is not a syntax phrase")
DEFINE_ERROR_I(NotBoundToASyntaxMark, "'%s' is not a mark variable")
DEFINE_ERROR_I(NotBoundToASyntaxRule, "'%s' is not a syntax rule")
DEFINE_ERROR_I(NotBoundToAType, "'%s' is not a type")

////////////////////////////////////////////////////////////////////////////////

void yyerror(const char *p_text)
{
    long t_position;
    GetCurrentPosition(&t_position);
    
    long t_row, t_column;
    GetColumnOfPosition(t_position, &t_column);
    GetRowOfPosition(t_position, &t_row);
    
    fprintf(stderr, "row %ld, col %ld: Parsing error - %s\n", t_row, t_column, p_text);
    
    s_error_count += 1;
}

////////////////////////////////////////////////////////////////////////////////
