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
