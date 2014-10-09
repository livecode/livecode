#include <stdio.h>

#include "literal.h"

void BeginStatementSyntaxRule(NameRef p_name)
{
}

void BeginExpressionSyntaxRule(NameRef p_name)
{
}

void BeginLeftUnaryOperatorSyntaxRule(NameRef p_name, long p_precedence)
{
}

void BeginRightUnaryOperatorSyntaxRule(NameRef p_name, long p_precedence)
{
}

void BeginLeftBinaryOperatorSyntaxRule(NameRef p_name, long p_precedence)
{
}

void BeginRightBinaryOperatorSyntaxRule(NameRef p_name, long p_precedence)
{
}

void BeginNeutralBinaryOperatorSyntaxRule(NameRef p_name, long p_precedence)
{
}

void EndSyntaxRule(void)
{
}

void BeginSyntaxGrammar(void)
{
}

void EndSyntaxGrammar(void)
{
}

void ConcatenateSyntaxGrammar(void)
{
}

void AlternateSyntaxGrammar(void)
{
}

void RepeatSyntaxGrammar(void)
{
}

void PushEmptySyntaxGrammar(void)
{
}

void PushKeywordSyntaxGrammar(const char *p_keyword)
{
}

void PushMarkedDescentSyntaxGrammar(int p_mark, NameRef p_rule)
{
}

void PushDescentSyntaxGrammar(NameRef p_rule)
{
}

void PushMarkedTrueSyntaxGrammar(int p_mark)
{
}

void PushMarkedFalseSyntaxGrammar(int p_mark)
{
}

void PushMarkedIntegerSyntaxGrammar(int p_mark, int p_value)
{
}

void PushMarkedStringSyntaxGrammar(int p_mark, int p_value)
{
}

void BeginSyntaxMappings(void)
{
}

void EndSyntaxMappings(void)
{
}

void BeginMethodSyntaxMapping(NameRef p_name)
{
}

void EndMethodSyntaxMapping(void)
{
}

void PushUndefinedArgumentSyntaxMapping(void)
{
}

void PushTrueArgumentSyntaxMapping(void)
{
}

void PushFalseArgumentSyntaxMapping(void)
{
}

void PushIntegerArgumentSyntaxMapping(long p_value)
{
}

void PushStringArgumentSyntaxMapping(const char *p_value)
{
}

void PushMarkArgumentSyntaxMapping(long p_index)
{
}

