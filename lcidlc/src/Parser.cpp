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

#include <stdio.h>

#include "Scanner.h"
#include "Interface.h"
#include "Parser.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct Parser *ParserRef;

enum ParserError
{
	kParserErrorNone,
	kParserErrorEndExpected,
	kParserErrorIdentifierExpected,
	kParserErrorStringExpected,
	kParserErrorIntegerExpected,
	kParserErrorConstantExpected,
	kParserErrorKeywordExpected,
	kParserErrorKeywordsExpected,
};

enum ParserKeyword
{
	kParserKeyword__None,
	kParserKeywordExternal,
	kParserKeywordUse,
	kParserKeywordOn,
	kParserKeywordCall,
	kParserKeywordEnum,
	kParserKeywordAs,
	kParserKeywordCommand,
	kParserKeywordFunction,
	kParserKeywordOptional,
	kParserKeywordDefault,
	kParserKeywordIn,
	kParserKeywordOut,
	kParserKeywordInOut,
	kParserKeywordRef,
	kParserKeywordReturn,
	kParserKeywordIndirect,
	kParserKeywordJava,
	kParserKeywordNative,
	kParserKeywordMethod,
	kParserKeywordTail,
	kParserKeywordObjc,
	kParserKeywordC,
	kParserKeywordNone,
};

struct Parser
{
	ScannerRef scanner;
	InterfaceRef interface;
	
	// Position of the last matched token
	Position position;
	
	// Indicates if a syntax error occurs
	bool syntax_error;
};

////////////////////////////////////////////////////////////////////////////////

static const char *s_parser_keyword_strings[] =
{
	nil,
	"external",
	"use",
	"on",
	"call",
	"enum",
	"as",
	"command",
	"function",
	"optional",
	"default",
	"in",
	"out",
	"inout",
	"ref",
	"return",
	"indirect",
	"java",
	"native",
	"method",
	"tail",
	"objc",
	"c",
	"none",
};

////////////////////////////////////////////////////////////////////////////////

extern const char *g_input_filename;

static bool ParserReport(ParserRef self, Position p_where, ParserError p_error, const char *p_hints[])
{
	fprintf(stderr, "%s:%d:%d: error: ", g_input_filename, PositionGetRow(p_where), PositionGetColumn(p_where));
	
	switch(p_error)
	{
	case kParserErrorEndExpected:
		fprintf(stderr, "syntax error\n");
		break;
	case kParserErrorIdentifierExpected:
		fprintf(stderr, "syntax error - identifier expected\n");
		break;
	case kParserErrorStringExpected:
		fprintf(stderr, "syntax error - string constant expected\n");
		break;
	case kParserErrorIntegerExpected:
		fprintf(stderr, "syntax error - integer constant expected\n");
		break;
	case kParserErrorConstantExpected:
		fprintf(stderr, "syntax error - string or numeric constant expected\n");
		break;
	case kParserErrorKeywordExpected:
		fprintf(stderr, "syntax error - '%s' expected\n", p_hints[0]);
		break;
	case kParserErrorKeywordsExpected:
		{
			uint32_t i;
			fprintf(stderr, "syntax error - \n");
			for(i = 0; p_hints[i + 1] != nil; i++)
				fprintf(stderr, "%s'%s'", i == 0 ? "" : ", ", p_hints[i]);
			fprintf(stderr, "%s'%s' expected\n", i == 0 ? "" : " or ", p_hints[i]);
		}
		break;
	case kParserErrorNone:
		MCUnreachableReturn(false);
		break;
	}
	
	self -> syntax_error = true;
	
	return false;
}

static void ParserMark(ParserRef self)
{
	ScannerMark(self -> scanner);
}

////////////////////////////////////////////////////////////////////////////////

static bool ParserWillMatchToken(ParserRef self, TokenType p_type, bool p_same_row = false)
{
	const Token *t_token;
	if (!ScannerRetrieve(self -> scanner, t_token))
		return false;

    if (p_same_row && PositionGetRow(self -> position) != PositionGetRow(t_token -> start))
        return false;
    
    if (t_token -> type != p_type)
		return false;
	
	return true;
}

static bool ParserWillMatchKeyword(ParserRef self, ParserKeyword p_keyword, bool p_same_row = false)
{
    const Token *t_token;
	if (!ScannerRetrieve(self -> scanner, t_token))
		return false;
    
    if (p_same_row && PositionGetRow(self -> position) != PositionGetRow(t_token -> start))
        return false;

	if (t_token -> type != kTokenTypeIdentifier ||
		!NameEqualToCString(t_token -> value, s_parser_keyword_strings[p_keyword]))
		return false;
	
	return true;
}

static bool ParserWillMatchKeywords(ParserRef self, ParserKeyword *p_keywords, bool p_same_row = false)
{
	for(uint32_t i = 0; p_keywords[i] != kParserKeyword__None; i++)
		if (ParserWillMatchKeyword(self, p_keywords[i], p_same_row))
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////

static bool ParserMatchIdentifier(ParserRef self, NameRef& r_name)
{
	const Token *t_token;
	if (!ScannerRetrieve(self -> scanner, t_token))
		return false;
		
	if (t_token -> type != kTokenTypeIdentifier)
		return ParserReport(self, t_token -> start, kParserErrorIdentifierExpected, nil);
		
	if (!ScannerAdvance(self -> scanner))
		return false;
	
	self -> position = t_token -> start;
	
	r_name = t_token -> value;
		
	return true;
}

static bool ParserMatchEnd(ParserRef self)
{
	const Token *t_token;
	if (!ScannerRetrieve(self -> scanner, t_token))
		return false;
		
	if (t_token -> type != kTokenTypeEnd)
		return ParserReport(self, t_token -> start, kParserErrorEndExpected, nil);
		
	if (!ScannerAdvance(self -> scanner))
		return false;
		
	self -> position = t_token -> start;
	
	return true;
}

static bool ParserMatchString(ParserRef self, ValueRef& r_value)
{
	const Token *t_token;
	if (!ScannerRetrieve(self -> scanner, t_token))
		return false;
		
	if (t_token -> type != kTokenTypeString)
		return ParserReport(self, t_token -> start, kParserErrorStringExpected, nil);
		
	if (!ScannerAdvance(self -> scanner))
		return false;
		
	self -> position = t_token -> start;
	
	r_value = t_token -> value;
	
	return true;
}

static bool ParserMatchInteger(ParserRef self, ValueRef& r_value)
{
	const Token *t_token;
	if (!ScannerRetrieve(self -> scanner, t_token))
		return false;
		
	if (t_token -> type != kTokenTypeNumber ||
		!NumberIsInteger(t_token -> value))
		return ParserReport(self, t_token -> start, kParserErrorIntegerExpected, nil);
		
	if (!ScannerAdvance(self -> scanner))
		return false;
		
	self -> position = t_token -> start;
	
	r_value = t_token -> value;
	
	return true;
}

static bool ParserMatchConstant(ParserRef self, ValueRef& r_value)
{
	const Token *t_token;
	if (!ScannerRetrieve(self -> scanner, t_token))
		return false;
    
	// MERG-2013-06-14: [[ ExternalsApiV5 ]] Check for a 'boolean' constant.
	bool t_is_bool = (t_token->type == kTokenTypeIdentifier);
	bool t_success = true;
	ValueRef t_bool_value = nil;
    if (t_is_bool)
    {
        bool t_bool;
        
        if (t_is_bool)
        {
            t_bool = !NameEqualToCString(t_token -> value, "false");
            t_is_bool = !t_bool;
        }
        
        if (!t_is_bool)
        {
            t_bool = NameEqualToCString(t_token -> value, "true");
            t_is_bool = t_bool;
        }
        
        if (t_is_bool)
            t_success = BooleanCreateWithBool(t_bool, t_bool_value);
    }
	else if (!(t_token -> type == kTokenTypeString ||
	           t_token -> type == kTokenTypeNumber))
	{
		return ParserReport(self, t_token -> start, kParserErrorConstantExpected, nil);
	}

	if (t_success)
		t_success = ScannerAdvance(self -> scanner);

	self -> position = t_token -> start;

	if (t_success)
	{
		if (t_is_bool)
			r_value = t_bool_value;
		else
			r_value = ValueRetain(t_token -> value);
	}
	else
	{
		if (t_bool_value != nil)
			ValueRelease(t_bool_value);
	}

	return t_success;
}

static bool ParserMatchKeyword(ParserRef self, ParserKeyword p_keyword)
{
	const Token *t_token;
	if (!ScannerRetrieve(self -> scanner, t_token))
		return false;

	if (t_token -> type != kTokenTypeIdentifier ||
		!NameEqualToCString(t_token -> value, s_parser_keyword_strings[p_keyword]))
		return ParserReport(self, t_token -> start, kParserErrorKeywordExpected, &s_parser_keyword_strings[p_keyword]);

	if (!ScannerAdvance(self -> scanner))
		return false;

	self -> position = t_token -> start;
	
	return true;
}

static bool ParserSkipKeyword(ParserRef self, ParserKeyword p_keyword, bool& r_skipped, bool p_same_row = false)
{
	if (ParserWillMatchKeyword(self, p_keyword, p_same_row))
	{
		if (!ParserMatchKeyword(self, p_keyword))
			return false;
			
		r_skipped = true;
	}
	else
		r_skipped = false;
		
	return true;
}

static bool ParserSkipToken(ParserRef self, TokenType p_token, bool& r_skipped, bool p_same_row = false)
{
	if (ParserWillMatchToken(self, p_token, p_same_row))
	{
		const Token *t_token;
		if (!ScannerRetrieve(self -> scanner, t_token))
			return false;
		
		if (!ScannerAdvance(self -> scanner))
			return false;
		
		self -> position = t_token -> start;
		
		r_skipped = true;
	}
	else
		r_skipped = false;

	return true;
}

static bool ParserMatchKeywords(ParserRef self, ParserKeyword *p_keywords, ParserKeyword& r_matched)
{
	for(uint32_t i = 0; p_keywords[i] != kParserKeyword__None; i++)
	{
		bool t_found;
		if (!ParserSkipKeyword(self, p_keywords[i], t_found))
			return false;
		
		if (t_found)
		{
			r_matched = p_keywords[i];
			return true;
		}
	}
	
	const char *t_keyword_cstrings[16];
	for(uint32_t i = 0; i < 16; i++)
	{
		t_keyword_cstrings[i] = s_parser_keyword_strings[p_keywords[i]];
		if (t_keyword_cstrings[i] == nil)
			break;
	}
	
	const Token *t_token;
	if (!ScannerRetrieve(self -> scanner, t_token))
		return false;
		
	return ParserReport(self, t_token -> start, kParserErrorKeywordsExpected, t_keyword_cstrings);
}

////////////////////////////////////////////////////////////////////////////////

static bool ParserReduceInterface(ParserRef self);
static bool ParserReduceHookClause(ParserRef self);
static bool ParserReduceDefinition(ParserRef self);
static bool ParserReduceEnumDefinition(ParserRef self);
static bool ParserReduceHandlerDefinition(ParserRef self);
static bool ParserReduceEnumElementDefinition(ParserRef self);
static bool ParserReduceParameterDefinition(ParserRef self);
static bool ParserReduceReturnDefinition(ParserRef self);
static bool ParserReduceCallDefinition(ParserRef self);
static bool ParserReduceUseDefinition(ParserRef self);

// interface
//   : 'external' ID
//     { use-clause }
//     { hook-clause }
//     { definition }
//
static bool ParserReduceInterface(ParserRef self)
{
	ParserMark(self);

	if (!ParserMatchKeyword(self, kParserKeywordExternal))
		return false;
	
	Position t_position;
	t_position = self -> position;
	
	NameRef t_external_name;
	if (!ParserMatchIdentifier(self, t_external_name) ||
		!InterfaceBegin(self -> interface, t_position, t_external_name))
		return false;
	
	while(ParserWillMatchKeyword(self, kParserKeywordUse))
		if (!ParserReduceUseDefinition(self))
			return false;
	
	while(ParserWillMatchKeyword(self, kParserKeywordOn))
		if (!ParserReduceHookClause(self))
			return false;
	
	static ParserKeyword s_definition_keywords[] = { kParserKeywordEnum, kParserKeywordCommand, kParserKeywordFunction, kParserKeywordTail, kParserKeywordUse, kParserKeyword__None };
	while(ParserWillMatchKeywords(self, s_definition_keywords))
		if (!ParserReduceDefinition(self))
			return false;
			
	if (!ParserMatchEnd(self))
		return false;
		
	if (!InterfaceEnd(self -> interface))
		return false;
		
	return true;
}

// hook-clause
//   : 'on' ID 'call' ID
//
static bool ParserReduceHookClause(ParserRef self)
{
	ParserMark(self);
	
	if (!ParserMatchKeyword(self, kParserKeywordOn))
		return false;
		
	Position t_position;
	t_position = self -> position;
	
	NameRef t_hook_name;
	if (!ParserMatchIdentifier(self, t_hook_name))
		return false;
	
	if (!ParserMatchKeyword(self, kParserKeywordCall))
		return false;
		
	NameRef t_target_name;
	if (!ParserMatchIdentifier(self, t_target_name))
		return false;
	
	if (!InterfaceDefineHook(self -> interface, t_position, t_hook_name, t_target_name))
		return false;
		
	return true;
}

// definition
//   : enum-definition
//   | command-definition
//   | function-definition
//   | method-definition
//   | use-definition
//
static bool ParserReduceDefinition(ParserRef self)
{
	ParserMark(self);
	
	/*if (ParserWillMatchKeyword(self, kParserKeywordEnum))
		return ParserReduceEnumDefinition(self);
	else if (ParserWillMatchKeyword(self, kParserKeywordCommand))
		return ParserReduceCommandDefinition(self);
	else if (ParserWillMatchKeyword(self, kParserKeywordFunction))
		return ParserReduceFunctionDefinition(self);
	else if (ParserWillMatchKeyword(self, kParserKeywordJava))
		return ParserReduceJavaDefinition(self);
	else if (ParserWillMatchKeyword(self, kParserKeywordNative))
		return ParserReduceNativeDefinition(self);
	else if (ParserWillMatchKeyword(self, kParserKeywordTail))
		return ParserReduceTailDefinition(self);*/
		
	if (ParserWillMatchKeyword(self, kParserKeywordEnum))
		return ParserReduceEnumDefinition(self);
	
	if (ParserWillMatchKeyword(self, kParserKeywordUse))
		return ParserReduceUseDefinition(self);
	
	static ParserKeyword s_handler_prefixes[] = { kParserKeywordCommand, kParserKeywordFunction, kParserKeywordTail, kParserKeyword__None };
	if (ParserWillMatchKeywords(self, s_handler_prefixes))
		return ParserReduceHandlerDefinition(self);
		
	return false;
}

////////////////////////////////////////////////////////////////////////////////

// enum-definition
//   : enum ID
//       { STRING as INTEGER }
//
static bool ParserReduceEnumDefinition(ParserRef self)
{
	ParserMark(self);
	
	if (!ParserMatchKeyword(self, kParserKeywordEnum))
		return false;
		
	Position t_position;
	t_position = self -> position;
	
	NameRef t_enum_name;
	if (!ParserMatchIdentifier(self, t_enum_name))
		return false;
		
	if (!InterfaceBeginEnum(self -> interface, t_position, t_enum_name))
		return false;
	
	while(ParserWillMatchToken(self, kTokenTypeString))
		if (!ParserReduceEnumElementDefinition(self))
			return false;
	
	if (!InterfaceEndEnum(self -> interface))
		return false;
	
	return true;
}

static bool ParserReduceEnumElementDefinition(ParserRef self)
{
	ParserMark(self);
	
	StringRef t_element_name;
	if (!ParserMatchString(self, t_element_name))
		return false;

	if (!ParserMatchKeyword(self, kParserKeywordAs))
		return false;
		
	Position t_position;
	t_position = self -> position;
	
	NumberRef t_element_value;
	if (!ParserMatchInteger(self, t_element_value))
		return false;
		
	if (!InterfaceDefineEnumElement(self -> interface, t_position, t_element_name, t_element_value))
		return false;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

// MERG-2015-11-16: Ensure use clause doesn't clobber use definition

// use-definition
//   : 'use' ID [ 'on' { ID , ',' } ]
//
static bool ParserReduceUseDefinition(ParserRef self)
{
	ParserMark(self);
	
	if (!ParserMatchKeyword(self, kParserKeywordUse))
		return false;
	
	Position t_position;
	t_position = self -> position;
	
	NameRef t_type_name;
	if (!ParserMatchIdentifier(self, t_type_name))
		return false;
    
    bool t_skipped;
    if (!ParserSkipKeyword(self, kParserKeywordOn, t_skipped, true))
        return false;
    
    if (!t_skipped)
        return InterfaceDefineUse(self -> interface, t_position, t_type_name);
        
	for(;;)
	{
		NameRef t_platform_name;
		if (!ParserMatchIdentifier(self, t_platform_name))
			return false;
		
		if (!InterfaceDefineUseOnPlatform(self -> interface, t_position, t_type_name, t_platform_name))
			return false;
		
		bool t_skipped_comma;
		if (!ParserSkipToken(self, kTokenTypeComma, t_skipped_comma, true))
			return false;
		
		if (!t_skipped_comma)
			break;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

// handler-definition
//   : [ 'tail' ] ( [ 'command' | 'function' | 'method' ] ) ) ID
//       { parameter-definition }
//       [ return ID ]
//       [ call ID ]
//
static bool ParserReduceHandlerDefinition(ParserRef self)
{
	ParserMark(self);
	
	Position t_position;
	t_position = self -> position;
	
	bool t_is_tail;
	if (!ParserSkipKeyword(self, kParserKeywordTail, t_is_tail))
		return false;
	
	/* bool t_is_java, t_is_native;
	if (!ParserSkipKeyword(self, kParserKeywordJava, t_is_java))
		return false;
	if (!t_is_java && !ParserSkipKeyword(self, kParserKeywordNative, t_is_native))
		return false;*/
		
	HandlerType t_type;
	bool t_skipped;
	if (ParserSkipKeyword(self, kParserKeywordCommand, t_skipped) && t_skipped)
		t_type = kHandlerTypeCommand;
	else if (ParserSkipKeyword(self, kParserKeywordFunction, t_skipped) && t_skipped)
		t_type = kHandlerTypeFunction;
/*	else if (ParserSkipKeyword(self, kParserKeywordMethod, t_skipped) && t_skipped)
		t_type = kHandlerTypeMethod;*/
	else
		return false;
	
	NameRef t_name;
	if (!ParserMatchIdentifier(self, t_name))
		return false;
	
	HandlerAttributes t_attr;
	t_attr = 0;
	/*if (t_is_java)
		t_attr |= kHandlerAttributeIsJava;*/
	if (t_is_tail)
		t_attr |= kHandlerAttributeIsTail;
		
	if (!InterfaceBeginHandler(self -> interface, t_position, t_type, t_attr, t_name))
		return false;
				
	static ParserKeyword s_parameter_prefixes[] = { kParserKeywordOptional, kParserKeywordIn, kParserKeywordOut, kParserKeywordInOut, kParserKeywordRef, kParserKeyword__None };
	while(ParserWillMatchKeywords(self, s_parameter_prefixes))
		if (!ParserReduceParameterDefinition(self))
			return false;
	
	if (ParserWillMatchKeyword(self, kParserKeywordReturn))
		if (!ParserReduceReturnDefinition(self))
			return false;
			
	if (ParserWillMatchKeyword(self, kParserKeywordCall))
		if (!ParserReduceCallDefinition(self))
			return false;

	if (!InterfaceEndHandler(self -> interface))
		return false;

	return true;
}

/*static bool ParserReduceCommandDefinition(ParserRef self)
{
	ParserMark(self);
	
	if (!ParserMatchKeyword(self, kParserKeywordCommand))
		return false;
		
	return ParserReduceHandlerDefinition(self, kHandlerTypeCommand);
}

static bool ParserReduceFunctionDefinition(ParserRef self)
{
	ParserMark(self);
	
	if (!ParserMatchKeyword(self, kParserKeywordFunction))
		return false;
		
	return ParserReduceHandlerDefinition(self, kHandlerTypeFunction);
}

static bool ParserReduceJavaDefinition(ParserRef self)
{
	ParserMark(self);
	
	if (!ParserMatchKeyword(self, kParserKeywordJava))
		return false;
		
	if (ParserMatchKeyword(self, kParserKeyword
	
	if (!ParserMatchKeyword(self, kParserKeywordMethod))
		return false;
	
	return ParserReduceHandlerDefinition(self, kHandlerTypeJava);
}

static bool ParserReduceNativeDefinition(ParserRef self)
{
	ParserMark(self);
	
	if (!ParserMatchKeyword(self, kParserKeywordNative))
		return false;
	
	if (!ParserMatchKeyword(self, kParserKeywordMethod))
		return false;
	
	return ParserReduceHandlerDefinition(self, kHandlerTypeNative);
}

static bool ParserReduceTailDefinition(ParserRef self)
{
	ParserMark(self);
	
	if (!ParserMatchKeyword(self, kParserKeywordTail))
		return false;
		
	
		
	if (ParserWillMatchKeyword(self, kParserKeywordCommand))
	{
		if (!ParserMatchKeyword(self, kParserKeywordCommand))
			return false;
			
		return ParserReduceHandlerDefinition(self, kHandlerTypeTailCommand);
	}
	
	if (ParserWillMatchKeyword(self, kParserKeywordFunction))
	{
		if (!ParserMatchKeyword(self, kParserKeywordFunction))
			return false;
			
		return ParserReduceHandlerDefinition(self, kHandlerTypeTailFunction);
	}

	return false;
}*/

// parameter-definition
//   : ( 'in' | 'out' | 'inout' ) ID 'as' ID
//   | 'optional' 'in' ID 'as' ID 'default' ( STRING | NUMBER )
//
static bool ParserReduceParameterDefinition(ParserRef self)
{
	ParserMark(self);
	
	bool t_is_optional;
	if (!ParserSkipKeyword(self, kParserKeywordOptional, t_is_optional))
		return false;

	Position t_position;
	t_position = self -> position;
		
	static ParserKeyword s_param_type_prefixes[] = { kParserKeywordIn, kParserKeywordOut, kParserKeywordInOut, kParserKeywordRef, kParserKeyword__None };
	ParserKeyword t_keyword;
	if (!ParserMatchKeywords(self, s_param_type_prefixes, t_keyword))
		return false;
	
	ParameterType t_parameter_type;
	if (t_keyword == kParserKeywordIn)
		t_parameter_type = kParameterTypeIn;
	else if (t_keyword == kParserKeywordOut)
		t_parameter_type = kParameterTypeOut;
	else if (t_keyword == kParserKeywordInOut)
		t_parameter_type = kParameterTypeInOut;
	else if (t_keyword == kParserKeywordRef)
		t_parameter_type = kParameterTypeRef;
		
	if (!t_is_optional)
		t_position = self -> position;
	
	NameRef t_name;
	if (!ParserMatchIdentifier(self, t_name))
		return false;
		
	if (!ParserMatchKeyword(self, kParserKeywordAs))
		return false;
		
	NameRef t_type;
	if (!ParserMatchIdentifier(self, t_type))
		return false;
    
	ValueRef t_default;
	t_default = nil;
	if (t_is_optional)
	{
		// MERG-2013-06-14: [[ ExternalsApiV5 ]] 'default' clause is now optional.
		bool t_is_default;
        if (!ParserSkipKeyword(self, kParserKeywordDefault, t_is_default))
			return false;
		
		// If default keyword was present, then match a constant.
        if (t_is_default)
            if (!ParserMatchConstant(self, t_default))
                return false;
    }
	
    bool t_success;
    t_success = InterfaceDefineHandlerParameter(self -> interface, t_position, t_parameter_type, t_name, t_type, t_is_optional, t_default);
    
    ValueRelease(t_default);
    
    return t_success;
}

static bool ParserReduceReturnDefinition(ParserRef self)
{
	ParserMark(self);
	
	if (!ParserMatchKeyword(self, kParserKeywordReturn))
		return false;
		
	Position t_position;
	t_position = self -> position;
		
	NameRef t_type;
	if (!ParserMatchIdentifier(self, t_type))
		return false;
		
	bool t_is_indirect;
	if (!ParserSkipKeyword(self, kParserKeywordIndirect, t_is_indirect))
		return false;
		
	if (!InterfaceDefineHandlerReturn(self -> interface, t_position, t_type, t_is_indirect))
		return false;

	return true;
}

static bool ParserReduceCallDefinition(ParserRef self)
{
	ParserMark(self);
	
	if (!ParserMatchKeyword(self, kParserKeywordCall))
		return false;

	Position t_position;
	t_position = self -> position;
	
	NameRef t_name;
	if (!ParserMatchIdentifier(self, t_name))
		return false;
		
	if (!InterfaceDefineHandlerBinding(self -> interface, t_position, t_name))
		return false;
		
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool ParserRun(ScannerRef p_scanner, InterfaceRef& r_interface)
{
	bool t_success;
	t_success = true;
	
	InterfaceRef t_interface;
	t_interface = nil;
	if (t_success)
		t_success = InterfaceCreate(t_interface);
		
	Parser t_parser;
	if (t_success)
	{
		t_parser . scanner = p_scanner;
		t_parser . interface = t_interface;
		t_parser . position = 0;
		t_parser . syntax_error = false;
		
		t_success = ParserReduceInterface(&t_parser);
		if (t_success && t_parser . syntax_error)
			t_success = true;
	}
	
	if (t_success)
	{
		if (t_parser . syntax_error)
		{
			InterfaceDestroy(t_interface);
			t_interface = nil;
		}
		
		r_interface = t_interface;
	}
	else
		InterfaceDestroy(t_interface);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
