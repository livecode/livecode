/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "execpt.h"
#include "handler.h"
#include "scriptpt.h"
#include "newobj.h"
#include "cmds.h"
#include "chunk.h"
#include "param.h"
#include "mcerror.h"
#include "property.h"
#include "object.h"
#include "field.h"
#include "graphic.h"
#include "group.h"
#include "stack.h"
#include "dispatch.h"
#include "util.h"
#include "debug.h"
#include "paragraf.h"
#include "variable.h"
#include "ide.h"
#include "hndlrlst.h"
#include "external.h"
#include "parentscript.h"
#include "card.h"

#include "globals.h"

// script flush <field chunk>
// script configure classes tExpr
// script configure operators tExpr
// script configure keywords tExpr
// script format (char|line) x to y of <field chunk>
// script colorize (char|line) x to y of <field chunk>
// script complete at char x of <field chunk>
// script tokenize <container>

class MCIdeState
{
public:
	void SetCommentDelta(uint4 p_line, int1 p_delta);
	int1 GetCommentDelta(uint4 p_line) const;

	uint4 GetCommentNesting(uint4 p_line) const;
	uint4 GetLineCount(void) const;

	static MCIdeState *Find(MCField *p_field);
	static void Flush(MCField *p_field);

private:
	MCIdeState(void);
	~MCIdeState(void);

	MCIdeState *f_next;
	MCField *f_field;

	uint4 f_line_count;
	int1 *f_line_properties;

	static MCIdeState *s_states;
	static MCIdeState *s_cache;
};

MCIdeState *MCIdeState::s_states = NULL;
MCIdeState *MCIdeState::s_cache = NULL;

MCIdeState::MCIdeState(void)
	: f_next(NULL), f_line_count(0), f_line_properties(0)
{
}

MCIdeState::~MCIdeState(void)
{
	if (f_line_properties != NULL)
		delete[] f_line_properties;
}

MCIdeState *MCIdeState::Find(MCField *p_field)
{
	if (s_cache != NULL && s_cache -> f_field == p_field)
		return s_cache;

	MCIdeState *t_state;
	for(t_state = s_states; t_state != NULL; t_state = t_state -> f_next)
		if (t_state -> f_field == p_field)
		{
			s_cache = t_state;
			return t_state;
		}

	t_state = new MCIdeState;
	t_state -> f_next = s_states;
	t_state -> f_field = p_field;
	s_states = t_state;

	s_cache = t_state;

	return t_state;
}

void MCIdeState::Flush(MCField *p_field)
{
	MCIdeState *t_state;
	t_state = Find(p_field);
	if (t_state != NULL)
	{
		if (t_state == s_states)
			s_states = t_state -> f_next;
		else
		{
			MCIdeState *t_previous;
			for(t_previous = s_states; t_previous -> f_next != t_state; t_previous = t_previous -> f_next)
				;
			t_previous -> f_next = t_state -> f_next;
		}

		if (t_state == s_cache)
			s_cache = NULL;

		delete t_state;
	}
}

void MCIdeState::SetCommentDelta(uint4 p_line, int1 p_delta)
{
	if (p_line > f_line_count)
	{
		f_line_properties = (int1 *)realloc(f_line_properties, sizeof(int1) * p_line);
		memset(f_line_properties + f_line_count, 0, p_line - f_line_count);
		f_line_count = p_line;
	}
	f_line_properties[p_line - 1] = p_delta; 
}

int1 MCIdeState::GetCommentDelta(uint4 p_line) const
{
	if (p_line > f_line_count)
		return 0;
	return f_line_properties[p_line - 1];
}

uint4 MCIdeState::GetCommentNesting(uint4 p_line) const
{
	uint4 t_nesting;
	t_nesting = 0;
	for(uint4 t_line = 1; t_line < p_line; t_line++)
		t_nesting += GetCommentDelta(t_line);
	return t_nesting;
}

uint4 MCIdeState::GetLineCount(void) const
{
	return f_line_count;
}

///////////////////////////////////////////////////////////////////////////////

Parse_stat MCIdeScriptAction::parse_target(MCScriptPoint& p_script,MCChunk*& r_target)
{
	Parse_stat t_status;
	t_status = PS_NORMAL;

	if (t_status == PS_NORMAL)
	{
		r_target = new MCChunk(True);
		t_status = r_target -> parse(p_script, False);
	}
	
	return t_status;
}

Parse_stat MCIdeScriptAction::parse_target_range(MCScriptPoint& p_script, Chunk_term& r_type, MCExpression*& r_start, MCExpression*& r_end, MCChunk*& r_target)
{
	Parse_stat t_status;
	t_status = PS_NORMAL;

	if (t_status == PS_NORMAL)
	{
		if (p_script . skip_token(SP_FACTOR, TT_CHUNK, CT_CHARACTER) == PS_NORMAL)
			r_type = CT_CHARACTER;
		else if (p_script . skip_token(SP_FACTOR, TT_CHUNK, CT_LINE) == PS_NORMAL)
			r_type = CT_LINE;
		else
			t_status = PS_ERROR;
	}

	if (t_status == PS_NORMAL)
		t_status = p_script . parseexp(False, False, &r_start);
	
	if (t_status == PS_NORMAL)
		t_status = p_script . skip_token(SP_FACTOR, TT_TO, PT_TO);

	if (t_status == PS_NORMAL)
		t_status = p_script . parseexp(False, False, &r_end);

	if (t_status == PS_NORMAL)
		t_status = p_script . skip_token(SP_FACTOR, TT_OF, PT_OF);

	if (t_status == PS_NORMAL)
		t_status = parse_target(p_script, r_target);

	return t_status;
}

Exec_stat MCIdeScriptAction::eval_target(MCExecPoint& p_exec, MCChunk *p_target, MCField*& r_target)
{
	Exec_stat t_status;
	t_status = ES_NORMAL;

	MCObject *t_object;
	uint4 t_part;
	if (t_status == ES_NORMAL)
		t_status = p_target -> getobj(p_exec, t_object, t_part, True);

	if (t_status == ES_NORMAL)
	{
		if (t_object ->	gettype() == CT_FIELD)
			r_target = (MCField *)t_object;
		else
		{
			MCeerror -> add(EE_CHUNK_BADOBJECTEXP, line, pos);
			t_status = ES_ERROR;
		}
	}

	return t_status;
}

Exec_stat MCIdeScriptAction::eval_target_range(MCExecPoint& p_exec, MCExpression *p_start, MCExpression *p_end, MCChunk *p_target, int4& r_start, int4& r_end, MCField*& r_target)
{
	Exec_stat t_status;
	t_status = ES_NORMAL;

	int4 t_start;
	int4 t_end;
	MCField *t_target;

	if (t_status == ES_NORMAL)
		t_status = p_start -> eval(p_exec);

	if (t_status == ES_NORMAL)
		t_status = p_exec . getint4(t_start, line, pos, EE_OBJECT_NAN);

	if (t_status == ES_NORMAL)
		t_status = p_end -> eval(p_exec);

	if (t_status == ES_NORMAL)
		t_status = p_exec . getint4(t_end, line, pos, EE_OBJECT_NAN);

	if (t_status == ES_NORMAL)
		t_status = eval_target(p_exec, p_target, t_target);

	// OK-2008-04-25 : If the chunk specified evaluates to a char or line number less than zero,
	// throw an error. This fixes potential crash.
	if (t_start < 0)
		return ES_ERROR;

	if (t_end < 0)
		return ES_ERROR;

	r_target = t_target;
	r_start = t_start;
	r_end = t_end;

	return t_status;
}

///////////////////////////////////////////////////////////////////////////////

MCIdeScriptFlush::MCIdeScriptFlush(void)
	: f_target(NULL)
{
}

MCIdeScriptFlush::~MCIdeScriptFlush(void)
{
	delete f_target;
}

Parse_stat MCIdeScriptFlush::parse(MCScriptPoint& p_script)
{
  Parse_stat t_status;
	t_status = PS_NORMAL;

	MCChunk *t_target;
	t_target = NULL;
	if (t_status == PS_NORMAL)
		t_status = parse_target(p_script, f_target);

	return t_status;
}

Exec_stat MCIdeScriptFlush::exec(MCExecPoint& p_exec)
{
	Exec_stat t_status;
	t_status = ES_NORMAL;

	MCField *t_target;
	if (t_status == ES_NORMAL)
		t_status = eval_target(p_exec, f_target, t_target);

	if (t_status == ES_NORMAL)
		MCIdeState::Flush(t_target);

	return t_status;
}

///////////////////////////////////////////////////////////////////////////////

enum MCColourizeClass
{
	COLOURIZE_CLASS_NONE,
	COLOURIZE_CLASS_ERROR,
	COLOURIZE_CLASS_WHITESPACE,
	COLOURIZE_CLASS_CONTINUATION,
	COLOURIZE_CLASS_SEPARATOR,
	COLOURIZE_CLASS_SINGLE_COMMENT,
	COLOURIZE_CLASS_MULTI_COMMENT,
	COLOURIZE_CLASS_IDENTIFIER,
	COLOURIZE_CLASS_STRING,
	COLOURIZE_CLASS_NUMBER,
	COLOURIZE_CLASS_OPERATOR,
	COLOURIZE_CLASS_KEYWORD,

	__COLOURIZE_CLASS_COUNT__
};

struct MCColourizeStyle
{
	uint4 references;
	uint2 attributes;
	MCColor colour;
};

static MCColourizeStyle *s_script_styles = NULL;
static uint4 s_script_style_count = 0;

static uint1 *s_script_class_styles = NULL;
static uint1 *s_script_operator_styles = NULL;
static uint1 *s_script_keyword_styles = NULL;

#define SCRIPT_STYLE_ATTRIBUTE_COUNT 7
static const char *s_script_style_attribute_names[] =
{
	"bold",
	"italic",
	"oblique",
	"box",
	"raised",
	"underline",
	"strikeout"
};

static uint2 s_script_style_attribute_bits[] =
{
	FA_BOLD,
	FA_ITALIC,
	FA_OBLIQUE,
	FA_BOX,
	FA_3D_BOX,
	FA_UNDERLINE,
	FA_STRIKEOUT
};

static const char *s_script_class_names[] =
{
	"none",
	"error",
	"whitespace",
	"continuation",
	"separator",
	"singlecomment",
	"multicomment",
	"identifier",
	"literal",
	"number",
	"operator",
	"keyword"
};

typedef void (*MCColourizeCallback)(void *p_context, MCColourizeClass p_class, uint4 p_index, uint4 p_start, uint4 p_end);

#include "hashedstrings.cpp"

MCIdeScriptConfigure::MCIdeScriptConfigure(void)
	: f_type(TYPE_NONE),
		f_settings(NULL)
{
}

MCIdeScriptConfigure::~MCIdeScriptConfigure(void)
{
	delete f_settings;
}

Parse_stat MCIdeScriptConfigure::parse(MCScriptPoint& p_script)
{
	Parse_stat t_status;
	t_status = PS_NORMAL;

	Symbol_type t_type;
	if (t_status == PS_NORMAL && (p_script . next(t_type) != PS_NORMAL || t_type != ST_ID))
		t_status = PS_ERROR;

	if (t_status == PS_NORMAL && p_script . gettoken() == "classes")
		f_type = TYPE_CLASSES;
	else if (t_status == PS_NORMAL && p_script . gettoken() == "operators")
		f_type = TYPE_OPERATORS;
	else if (t_status == PS_NORMAL && p_script . gettoken() == "keywords")
		f_type = TYPE_KEYWORDS;
	else if (t_status == PS_NORMAL)
		t_status = PS_ERROR;

	if (t_status == PS_NORMAL)
		t_status = p_script . parseexp(False, False, &f_settings);
	
	return t_status;
}

static uint1 commit_style(MCColourizeStyle& p_style)
{
	uint4 t_index;
	for(t_index = 0; t_index < s_script_style_count; ++t_index)
		if (p_style . attributes == s_script_styles[t_index] . attributes &&
				p_style . colour . red == s_script_styles[t_index] . colour . red &&
				p_style . colour . green == s_script_styles[t_index] . colour . green &&
				p_style . colour . blue == s_script_styles[t_index] . colour . blue)
			break;

	if (t_index == s_script_style_count)
		for(t_index = 0; t_index < s_script_style_count; ++t_index)
			if (s_script_styles[t_index] . references == 0)
				break;

	if (t_index == s_script_style_count && s_script_style_count < 256)
	{
		s_script_style_count += 1; 
		s_script_styles = (MCColourizeStyle *)realloc(s_script_styles, sizeof(MCColourizeStyle) * s_script_style_count);
		memset(&s_script_styles[t_index], 0, sizeof(MCColourizeStyle));
	}
	else if (t_index == s_script_style_count)
		t_index = 0;

	s_script_styles[t_index] . references += 1;
	s_script_styles[t_index] . attributes = p_style . attributes;
	s_script_styles[t_index] . colour = p_style . colour;

	return t_index;
}

Exec_stat MCIdeScriptConfigure::exec(MCExecPoint& p_exec)
{
	Exec_stat t_stat;
	t_stat = ES_NORMAL;

	if (t_stat == ES_NORMAL)
		t_stat = f_settings -> eval(p_exec);

	MCVariableValue *t_settings;
	if (t_stat == ES_NORMAL)
	{
		t_settings = p_exec . getarray();
		if (t_settings == NULL)
			t_stat = ES_ERROR;
	}

	if (t_stat != ES_NORMAL)
		return t_stat;

	MCExecPoint t_value;
	t_value . clear();

	switch(f_type)
	{
	case TYPE_CLASSES:
		{
			if (s_script_class_styles == NULL)
				s_script_class_styles = new uint1[__COLOURIZE_CLASS_COUNT__];
			else
			{
				for(uint4 t_class = 0; t_class < __COLOURIZE_CLASS_COUNT__; ++t_class)
					s_script_styles[s_script_class_styles[t_class]] . references -= 1;
			}

			for(uint4 t_class = 0; t_class < __COLOURIZE_CLASS_COUNT__; ++t_class)
			{
				MCColourizeStyle t_style;
				memset(&t_style, 0, sizeof(MCColourizeStyle));

				t_value . setstringf("%s attributes", s_script_class_names[t_class]);
				if (t_settings -> fetch_element(t_value, t_value . getsvalue()) == ES_NORMAL)
				{
					for(uint4 t_attribute = 0; t_attribute < SCRIPT_STYLE_ATTRIBUTE_COUNT; ++t_attribute)
						if (strstr(t_value . getcstring(), s_script_style_attribute_names[t_attribute]) != NULL)
							t_style . attributes |= s_script_style_attribute_bits[t_attribute];
				}

				t_value . setstringf("%s color", s_script_class_names[t_class]);
				if (t_settings -> fetch_element(t_value, t_value . getsvalue()) == ES_NORMAL)
				{
					char *t_colour_name;
					t_colour_name = NULL;
					MCscreen -> parsecolor(t_value . getsvalue(), &t_style . colour, &t_colour_name);
					delete t_colour_name;
				}

				s_script_class_styles[t_class] = commit_style(t_style);
			}
		}
		break;

	case TYPE_KEYWORDS:
		{
			if (s_script_keyword_styles == NULL)
				s_script_keyword_styles = new uint1[SCRIPT_KEYWORD_COUNT];
			else
			{
				for(uint4 t_keyword = 0; t_keyword < SCRIPT_KEYWORD_COUNT; ++t_keyword)
					s_script_styles[s_script_keyword_styles[t_keyword]] . references -= 1;
			}

			for(uint4 t_keyword = 0; t_keyword < SCRIPT_KEYWORD_COUNT; ++t_keyword)
			{
				bool t_changed;
				t_changed = false;

				if (s_script_class_styles == NULL)
					t_changed = true;

				MCColourizeStyle t_style;
				memset(&t_style, 0, sizeof(MCColourizeStyle));

				t_value . setstringf("%s attributes", s_script_keywords[t_keyword]);
				if (t_settings -> fetch_element(t_value, t_value . getsvalue()) == ES_NORMAL)
				{
					for(uint4 t_attribute = 0; t_attribute < SCRIPT_STYLE_ATTRIBUTE_COUNT; ++t_attribute)
						if (strstr(t_value . getcstring(), s_script_style_attribute_names[t_attribute]) != NULL)
						{
							t_style . attributes |= s_script_style_attribute_bits[t_attribute];
							t_changed = true;
						}
				}

				t_value . setstringf("%s color", s_script_keywords[t_keyword]);
				if (t_settings -> fetch_element(t_value, t_value . getsvalue()) == ES_NORMAL)
				{
					char *t_colour_name;
					t_colour_name = NULL;
					if (MCscreen -> parsecolor(t_value . getsvalue(), &t_style . colour, &t_colour_name))
					{
						delete t_colour_name;
						t_changed = true;
					}
				}

				if (t_changed)
					s_script_keyword_styles[t_keyword] = commit_style(t_style);
				else
				{
					s_script_styles[s_script_class_styles[COLOURIZE_CLASS_KEYWORD]] . references += 1;
					s_script_keyword_styles[t_keyword] = s_script_class_styles[COLOURIZE_CLASS_KEYWORD];
				}
			}
		}
		break;

	case TYPE_OPERATORS:
		break;
	}

	return t_stat;
}

///////////////////////////////////////////////////////////////////////////////

extern uint8_t type_table[256];


MCIdeScriptColourize::MCIdeScriptColourize(void)
	: f_target(NULL),
		f_start(0),
		f_end(0),
		f_type(CT_UNDEFINED)
{
}

MCIdeScriptColourize::~MCIdeScriptColourize(void)
{
	delete f_target;
	delete f_start;
	delete f_end;
}

Parse_stat MCIdeScriptColourize::parse(MCScriptPoint& p_script)
{
	Parse_stat t_status;
	t_status = PS_NORMAL;

	if (t_status == PS_NORMAL)
		t_status = parse_target_range(p_script, f_type, f_start, f_end, f_target);

	return t_status;
}

static void colourize_paragraph(void *p_context, MCColourizeClass p_class, uint4 p_index, uint4 t_start, uint4 t_end)
{
	MCParagraph *t_paragraph;
	t_paragraph = (MCParagraph *)p_context;

	MCColourizeStyle *t_style;
	t_style = NULL;
	if (p_class == COLOURIZE_CLASS_KEYWORD && s_script_keyword_styles != NULL)
		t_style = &s_script_styles[s_script_keyword_styles[p_index]];
	else if (s_script_class_styles != NULL)
		t_style = &s_script_styles[s_script_class_styles[p_class]];

	if (t_style != NULL)
	{
		t_paragraph -> setatts(t_start, t_end, P_FORE_COLOR, &t_style -> colour);
		t_paragraph -> setatts(t_start, t_end, P_TEXT_STYLE, (void *)t_style -> attributes);
	}
}


// Parameters
//   p_text : pointer to the buffer containing the text we are tokenizing
//   p_length : the length of p_text
//   x_index : the index of the char we are currently at. This value will be mutated to contain the index of the next char after the comment if one was found.
//   r_class : this will contain the type of comment that was found, if applicable
//   r_nesting_delta : this will contain the nesting depth difference from the original x_index to the value of x_index that is returned.
//   r_update_min_nesting : whether or not to update the min nesting
//   r_multiple_lines : whether or not the comment actually contains multiple lines.
// Returns
//   true if the start of a comment is found at char x_index of p_text, false otherwise.
// Description
//   If the function returns false, then all the return parameters are not changed.
static bool match_comment(const unsigned char *p_text, uint4 p_length, uint4 &x_index, MCColourizeClass &r_class, uint4 &r_nesting_delta, bool &r_update_min_nesting, bool &r_multiple_lines)
{
	r_nesting_delta = 0;
	r_update_min_nesting = false;
	r_multiple_lines = false;

	while(x_index < p_length)
	{
		switch(type_table[p_text[x_index]])
		{
			case ST_MIN:
				if (type_table[p_text[x_index + 1]] == ST_MIN)
				{
					r_class = COLOURIZE_CLASS_SINGLE_COMMENT;
					while(x_index < p_length && type_table[p_text[x_index]] != ST_EOL)
						x_index++;
		
					return true;
				}
				else
					return false;
				break;

			case ST_COM:
				r_class = COLOURIZE_CLASS_SINGLE_COMMENT;
				while(x_index < p_length && type_table[p_text[x_index]] != ST_EOL)
					x_index++;
				
				return true;
				break;

			case ST_OP:
				if (p_length - x_index >= 2 && p_text[x_index] == '/' && p_text[x_index + 1] == '/')
				{
					r_class = COLOURIZE_CLASS_SINGLE_COMMENT;
					while(x_index < p_length && type_table[p_text[x_index]] != ST_EOL)
						x_index++;

					return true;
				}
				else if (p_length - x_index >= 2 && p_text[x_index] == '/' && p_text[x_index + 1] == '*')
				{
					// As we only need to return the nesting difference, we start the nesting off at 0
					uint4 t_nesting;
					t_nesting = 0;

					x_index += 2;
					t_nesting += 1;
					r_class = COLOURIZE_CLASS_MULTI_COMMENT;
					while(x_index < p_length - 1 && t_nesting > 0)
					{
						if (type_table[p_text[x_index]] == ST_EOL)
							r_multiple_lines = true;

						if (p_text[x_index] == '*' && p_text[x_index + 1] == '/')
						{
							t_nesting -= 1;
							x_index += 2;

							r_update_min_nesting = true;
						}
						else
							x_index += 1;
					}
					if (t_nesting != 0 && x_index < p_length)
						x_index += 1;

					r_nesting_delta = t_nesting;

					return true;
				}
				else
					return false;
				break;
			default:
				return false;
		}
	}
	return false;

}

static void tokenize(const unsigned char *p_text, uint4 p_length, uint4 p_in_nesting, uint4& r_out_nesting, uint4& r_min_nesting, MCColourizeCallback p_callback, void *p_context)
{
	uint4 t_index;
	t_index = 0;

	uint4 t_nesting;
	t_nesting = p_in_nesting;

	uint4 t_min_nesting;
	t_min_nesting = t_nesting;

	MCColourizeClass t_class;
	t_class = COLOURIZE_CLASS_NONE;

	if (p_length == 0)
	{
		r_min_nesting = p_in_nesting;
		r_out_nesting = p_in_nesting;
		return;
	}

	if (t_nesting > 0)
	{
		while(t_nesting > 0 && t_index < p_length - 1)
		{
			if (p_text[t_index] == '/' && p_text[t_index + 1] == '*')
			{
				t_nesting += 1;
				t_index += 2;
			}
			else if (p_text[t_index] == '*' && p_text[t_index + 1] == '/')
			{
				t_nesting -= 1;
				t_index += 2;

				t_min_nesting = MCU_min(t_min_nesting, t_nesting);
			}
			else
				t_index += 1;
		}

		if (t_nesting != 0 && t_index < p_length)
			t_index += 1;

		p_callback(p_context, COLOURIZE_CLASS_MULTI_COMMENT, 0, 0, t_index);
	}

	while(t_index < p_length)
	{
		uint4 t_class_index;
		t_class_index = 0;

		uint4 t_start, t_end;
		t_start = t_index;

		MCColourizeClass t_comment_class;
		uint4 t_nesting_delta;
		bool t_update_min_nesting;
		bool t_multiple_lines;

		if (match_comment(p_text, p_length, t_index, t_comment_class, t_nesting_delta, t_update_min_nesting, t_multiple_lines))
		{
			t_end = t_index;
			t_nesting = t_nesting + t_nesting_delta;

			if (t_update_min_nesting)
				t_min_nesting = MCU_min(t_min_nesting, t_nesting);

			t_class = t_comment_class;

		}
		else
		{
			switch(type_table[p_text[t_index]])
			{
				case ST_SPC:
					t_class = COLOURIZE_CLASS_WHITESPACE;
					while(t_index < p_length && type_table[p_text[t_index]] == ST_SPC)
						t_index++;
					t_end = t_index;
				break;

				case ST_MIN:
					t_index++;
					t_class = COLOURIZE_CLASS_OPERATOR;
					while(t_index < p_length && type_table[p_text[t_index + 1]] == ST_OP)
						t_index++;
					t_end = t_index;
				break;

				case ST_OP:
					t_class = COLOURIZE_CLASS_OPERATOR;
					while(t_index < p_length && type_table[p_text[t_index]] == ST_OP)
						t_index++;
					t_end = t_index;
				break;

				case ST_LP:
				case ST_RP:
				case ST_LB:
				case ST_RB:
				case ST_SEP:
					t_index++;
					t_class = COLOURIZE_CLASS_OPERATOR;
					t_end = t_index;
				break;

				case ST_ESC:
					t_index++;
					p_callback(p_context, COLOURIZE_CLASS_CONTINUATION, 0, t_start, t_index);
					t_start = t_index;
					while(t_index < p_length && type_table[p_text[t_index]] == ST_SPC)
						t_index++;
					if (t_start != t_index)
						p_callback(p_context, COLOURIZE_CLASS_WHITESPACE, 0, t_start, t_index);

					t_start = t_index;
					t_class = COLOURIZE_CLASS_ERROR;

					// OK-2008-05-19 : Comments are permitted after continuation chars, providing that they don't contain multiple lines.
					if (match_comment(p_text, p_length, t_index, t_comment_class, t_nesting_delta, t_update_min_nesting, t_multiple_lines))
					{
						t_end = t_index;
						if (!t_multiple_lines)
							t_class = t_comment_class;

						p_callback(p_context, t_class, 0, t_start, t_end);
						t_start = t_end;

						// Once we've had a comment, there may be more stuff after it ends (if its a multi-line one), this stuff should be colourized
						// as errors, even though it will actually compile. 
						t_class = COLOURIZE_CLASS_ERROR;
						while(t_index < p_length && (type_table[p_text[t_index]] != ST_EOL))
							t_index++;

						t_end = t_index;
					}
					else
					{
						while(t_index < p_length && (type_table[p_text[t_index]] != ST_EOL))
							t_index++;

						t_end = t_index;
					}

				break;
					
				case ST_SEMI:
				case ST_EOL:
					t_index++;
					t_class = COLOURIZE_CLASS_SEPARATOR;
					t_end = t_index;
				break;

				case ST_EOF:
				case ST_ERR:
					t_index++;
					t_class = COLOURIZE_CLASS_ERROR;
					t_end = t_index;
				break;

				// MW-2011-07-18: Make sure we handle ST_TAG like ST_ID - ST_TAG
				//  is only used in server-mode when tag processing is turned on
				case ST_TAG:
				case ST_ID:
				{
					char t_keyword[SCRIPT_KEYWORD_LARGEST + 1];
					uint4 t_hash;
					uint4 t_klength;
					t_hash = SCRIPT_KEYWORD_SALT;
					t_klength = 0;
					t_class = COLOURIZE_CLASS_KEYWORD;
					while(t_index < p_length && (type_table[p_text[t_index]] == ST_ID || type_table[p_text[t_index]] == ST_NUM || type_table[p_text[t_index]] == ST_TAG))
					{
						if (t_class == COLOURIZE_CLASS_KEYWORD)
						{
							if (t_klength == SCRIPT_KEYWORD_LARGEST)
								t_class = COLOURIZE_CLASS_IDENTIFIER;
							else
							{
								t_keyword[t_klength] = MCS_tolower(p_text[t_index]);
								t_hash = (t_hash ^ t_keyword[t_klength]) + ((t_hash << 26) + (t_hash >> 6));
								t_klength++;
							}
						}
						t_index++;
					}
					// MW-2013-08-23: [[ Bug 11122 ]] Special-case '$#'.
					if (t_index < p_length && t_klength == 1 && t_keyword[0] == '$' && p_text[t_index] == '#')
					{
						t_keyword[t_klength] = '#';
						t_klength++;
						t_index++;
					}
					t_end = t_index;
					if (t_class == COLOURIZE_CLASS_KEYWORD)
					{
						t_keyword[t_klength] = '\0';
						t_hash = script_keyword_hash(t_hash);
						if (t_hash >= SCRIPT_KEYWORD_COUNT || strcmp(s_script_keywords[t_hash], t_keyword) != 0)
							t_class = COLOURIZE_CLASS_IDENTIFIER;
						else
						{
							t_class_index = t_hash;
							t_class = COLOURIZE_CLASS_KEYWORD;
						}
					}
				}
				break;

				case ST_LIT:
					t_index++;
					while(t_index < p_length && type_table[p_text[t_index]] != ST_EOL && (type_table[p_text[t_index]] != ST_LIT))
						t_index++;
					if (t_index < p_length && type_table[p_text[t_index]] == ST_LIT)
					{
						t_index++;
						t_class = COLOURIZE_CLASS_STRING;
					}
					else
						t_class = COLOURIZE_CLASS_ERROR;
					t_end = t_index;
				break;

				case ST_NUM:
					t_class = COLOURIZE_CLASS_NUMBER;
					while(t_index < p_length && type_table[p_text[t_index]] == ST_NUM)
						t_index++;
					t_end = t_index;
				break;
			}
		}

		p_callback(p_context, t_class, t_class_index, t_start, t_end);
	}

	r_out_nesting = t_nesting;
	r_min_nesting = t_min_nesting;
}

static void TokenizeField(MCField *p_field, MCIdeState *p_state, Chunk_term p_type, uint4 p_start, uint4 p_end, MCColourizeCallback p_callback, bool p_mutate = true)
{
	if (p_field -> getparagraphs() == NULL)
		return;
	
	uint4 t_start;
	t_start = p_start;
	
	uint4 t_end;
	t_end = p_end;
	
	MCField *t_target;
	t_target = p_field;
	
	MCIdeState *t_state;
	t_state = p_state;
	
	MCParagraph *t_sentinal_paragraph;
	MCParagraph *t_first_paragraph;
	MCParagraph *t_last_paragraph;

	uint4 t_first_line;
	uint4 t_last_line;

	if (t_start > t_end)
		t_end = t_start;

	t_sentinal_paragraph = t_target -> getparagraphs();
	if (p_type == CT_CHARACTER)
	{
		// MW-2012-02-23: [[ CharChunk ]] Convert the 1-based char indices to 1-based field indices.
		int32_t si, ei;
		si = 0;
		ei = INT32_MAX;
		t_target -> resolvechars(0, si, ei, t_start - 1, t_end - t_start + 1);
		t_start = si + 1;
		t_end = ei;
		
		// Takes si / ei as 1-based field indices.
		t_target -> charstoparagraphs(t_start, t_end, t_first_paragraph, t_last_paragraph, t_first_line, t_last_line);
	}
	else
	{
		t_target -> linestoparagraphs(t_start, t_end, t_first_paragraph, t_last_paragraph);
		t_first_line = t_start;
		t_last_line = t_end;
	}

	uint4 t_old_nesting, t_new_nesting;
	t_old_nesting = t_new_nesting = t_state -> GetCommentNesting(t_first_line);

	MCParagraph *t_paragraph;
	t_paragraph = t_first_paragraph;

	uint4 t_line;
	t_line = t_first_line;
	
	MCExecPoint ep;

	for(t_line = t_first_line, t_paragraph = t_first_paragraph; t_line <= t_last_line; t_line++, t_paragraph = t_paragraph -> next())
	{
		// MW-2012-02-23: [[ FieldChars ]] Nativize the paragraph so tokenization
		//   works.
		char *t_text;
		uint4 t_length;
		uint4 t_min_nesting, t_nesting;
		t_text = ep . getbuffer(t_paragraph -> gettextsize());
		t_length = 0;
		t_paragraph -> nativizetext(true, t_text, t_length);
		t_paragraph -> clearzeros();
		tokenize((const unsigned char *)t_text, t_length, t_new_nesting, t_nesting, t_min_nesting, p_callback, t_paragraph);
		
		t_old_nesting += t_state -> GetCommentDelta(t_line);
		if (p_mutate)
			t_state -> SetCommentDelta(t_line, t_nesting - t_new_nesting);
		t_new_nesting = t_nesting;
	}

	if (p_mutate)
		while(t_paragraph != t_sentinal_paragraph && t_new_nesting != t_old_nesting)
		{
			// MW-2012-02-23: [[ FieldChars ]] Nativize the paragraph so tokenization
			//   works.
			char *t_text;
			uint4 t_length;
			uint4 t_min_nesting, t_nesting;
			t_text = ep . getbuffer(t_paragraph -> gettextsize());
			t_length = 0;
			t_paragraph -> nativizetext(true, t_text, t_length);
			t_paragraph -> clearzeros();
			tokenize((const unsigned char *)t_text, t_length, t_new_nesting, t_nesting, t_min_nesting, p_callback, t_paragraph);

			t_old_nesting += t_state -> GetCommentDelta(t_line);
			t_state -> SetCommentDelta(t_line, t_nesting - t_new_nesting);
			t_new_nesting = t_nesting;

			t_paragraph = t_paragraph -> next();
			t_line++;
		}

	do
	{
		t_paragraph = t_paragraph -> prev();
		t_paragraph -> layout();
	}
	while(t_paragraph != t_first_paragraph);

	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	t_target -> layer_redrawall();
}

Exec_stat MCIdeScriptColourize::exec(MCExecPoint& p_exec)
{
	Exec_stat t_status;
	t_status = ES_NORMAL;

	int4 t_start;
	int4 t_end;
	MCField *t_target;
	if (t_status == ES_NORMAL)
		t_status = eval_target_range(p_exec, f_start, f_end, f_target, t_start, t_end, t_target);

	MCIdeState *t_state;
	if (t_status == ES_NORMAL)
		t_state = MCIdeState::Find(t_target);

	if (t_status == ES_NORMAL && t_target -> getparagraphs() != NULL)
	{
		TokenizeField(t_target, t_state, f_type, t_start, t_end, colourize_paragraph);
	}

	return t_status;
}

///////////////////////////////////////////////////////////////////////////////

MCIdeScriptReplace::MCIdeScriptReplace(void)
	: f_target(NULL), f_start(NULL), f_end(NULL), f_text(NULL), f_type(CT_UNDEFINED)
{
}

MCIdeScriptReplace::~MCIdeScriptReplace(void)
{
	delete f_text;
	delete f_target;
	delete f_start;
	delete f_end;
}

Parse_stat MCIdeScriptReplace::parse(MCScriptPoint& p_script)
{
	Parse_stat t_status;
	t_status = PS_NORMAL;

	if (t_status == PS_NORMAL)
		t_status = parse_target_range(p_script, f_type, f_start, f_end, f_target);
		
	if (t_status == PS_NORMAL)
		t_status = p_script . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH);

	if (t_status == PS_NORMAL)
		t_status = p_script . parseexp(False, False, &f_text);

	return t_status;
}

Exec_stat MCIdeScriptReplace::exec(MCExecPoint& p_exec)
{
	Exec_stat t_status;
	t_status = ES_NORMAL;

	int4 t_start;
	int4 t_end;
	MCField *t_target;
	if (t_status == ES_NORMAL)
		t_status = eval_target_range(p_exec, f_start, f_end, f_target, t_start, t_end, t_target);

	MCIdeState *t_state;
	if (t_status == ES_NORMAL)
		t_state = MCIdeState::Find(t_target);

	MCExecPoint t_text(NULL, NULL, NULL);
	if (t_status == ES_NORMAL)
		t_status = f_text -> eval(t_text);

	if (t_status == ES_NORMAL)
	{
		int4 t_start_index, t_end_index;
		t_start_index = t_start;
		t_end_index = t_end;
		
		t_start_index -= 1;
		
		if (t_start_index > t_end_index)
			t_end_index = t_start_index;
		
		// MW-2012-02-23: [[ FieldChars ]] Resolve the field indices from the chunk
		//   and set the range of text.
		int32_t si, ei;
		si = 0;
		ei = INT32_MAX;
		t_target -> resolvechars(0, si, ei, t_start_index, t_end_index - t_start_index);
		t_target -> settextindex(0, si, ei, t_text . getsvalue(), False);
		
		TokenizeField(t_target, t_state, CT_CHARACTER, t_start, t_start + t_text . getsvalue() . getlength() - 1, colourize_paragraph);
		
		t_target -> replacecursor(True, True);
	}

	return t_status;
}

///////////////////////////////////////////////////////////////////////////////

MCIdeScriptDescribe::MCIdeScriptDescribe(void)
	: f_type(TYPE_NONE)
{
}

MCIdeScriptDescribe::~MCIdeScriptDescribe(void)
{
}

Parse_stat MCIdeScriptDescribe::parse(MCScriptPoint& p_script)
{
	Parse_stat t_status;
	t_status = PS_NORMAL;

	Symbol_type t_type;
	if (t_status == PS_NORMAL && (p_script . next(t_type) != PS_NORMAL || t_type != ST_ID))
		t_status = PS_ERROR;

	if (t_status == PS_NORMAL && p_script . gettoken() == "classes")
		f_type = TYPE_CLASSES;
	else if (t_status == PS_NORMAL && p_script . gettoken() == "operators")
		f_type = TYPE_OPERATORS;
	else if (t_status == PS_NORMAL && p_script . gettoken() == "keywords")
		f_type = TYPE_KEYWORDS;
	else if (t_status == PS_NORMAL && p_script . gettoken() == "styles")
		f_type = TYPE_STYLES;
	else if (t_status == PS_NORMAL)
	{
		if (p_script . gettoken() == "class")
			f_type = TYPE_CLASS_STYLES;
		else if (p_script . gettoken() == "keyword")
			f_type = TYPE_KEYWORD_STYLES;
		else if (p_script . gettoken() == "operator")
			f_type = TYPE_OPERATOR_STYLES;
		else
			t_status = PS_ERROR;

		if (t_status == PS_NORMAL && (p_script . next(t_type) != PS_NORMAL || t_type != ST_ID))
			t_status = PS_ERROR;

		if (t_status == PS_NORMAL)
			t_status = (p_script . gettoken() == "styles") ? PS_NORMAL : PS_ERROR;
	}
	else if (t_status == PS_NORMAL)
		t_status = PS_ERROR;

	return t_status;
}

Exec_stat MCIdeScriptDescribe::exec(MCExecPoint& p_exec)
{
	Exec_stat t_status;
	t_status = ES_NORMAL;

	p_exec . clear();

	switch(f_type)
	{
	case TYPE_CLASSES:
		for(uint4 t_class = 1; t_class < __COLOURIZE_CLASS_COUNT__; ++t_class)
			p_exec . concatcstring(s_script_class_names[t_class], EC_RETURN, t_class == 1);
	break;

	case TYPE_KEYWORDS:
		for(uint4 t_keyword = 0; t_keyword < SCRIPT_KEYWORD_COUNT; ++t_keyword)
		{
			if (s_script_keywords[t_keyword][0] != '\0')
				p_exec . concatcstring(s_script_keywords[t_keyword], EC_RETURN, t_keyword == 0);
		}
	break;
	}

	if (t_status == ES_NORMAL)
		MCresult -> store(p_exec, False);

	return t_status;
}

///////////////////////////////////////////////////////////////////////////////

MCIdeScriptStrip::MCIdeScriptStrip(void)
	: f_type(CT_UNDEFINED),
	  f_start(NULL),
	  f_end(NULL),
	  f_target(NULL),
	  f_output(NULL)
{
}

MCIdeScriptStrip::~MCIdeScriptStrip(void)
{
	delete f_start;
	delete f_end;
	delete f_target;
	delete f_output;
}

Parse_stat MCIdeScriptStrip::parse(MCScriptPoint& p_script)
{
	Parse_stat t_status;
	t_status = PS_NORMAL;

	if (t_status == PS_NORMAL)
		t_status = parse_target_range(p_script, f_type, f_start, f_end, f_target);

	if (t_status == PS_NORMAL)
		getit(p_script, f_output);

	return t_status;
}

static MCExecPoint *s_strip_paragraph_ep;

static void strip_paragraph(void *p_context, MCColourizeClass p_class, uint4 p_index, uint4 t_start, uint4 t_end)
{
	MCParagraph *t_paragraph;
	t_paragraph = (MCParagraph *)p_context;

	bool t_first;
	t_first = s_strip_paragraph_ep -> getsvalue() . getlength() == 0;

	switch(p_class)
	{
	case COLOURIZE_CLASS_NONE:
	case COLOURIZE_CLASS_ERROR:
	case COLOURIZE_CLASS_WHITESPACE:
	case COLOURIZE_CLASS_SINGLE_COMMENT:
	case COLOURIZE_CLASS_MULTI_COMMENT:
	break;
	
	default:
		s_strip_paragraph_ep -> concatchars(t_paragraph -> gettext() + t_start, t_end - t_start, EC_SPACE, t_first);
	break;
	}
}

Exec_stat MCIdeScriptStrip::exec(MCExecPoint& p_exec)
{
	Exec_stat t_status;
	t_status = ES_NORMAL;

	int4 t_start;
	int4 t_end;
	MCField *t_target;
	if (t_status == ES_NORMAL)
		t_status = eval_target_range(p_exec, f_start, f_end, f_target, t_start, t_end, t_target);

	MCIdeState *t_state;
	if (t_status == ES_NORMAL)
		t_state = MCIdeState::Find(t_target);

	if (t_status == ES_NORMAL && t_target -> getparagraphs() != NULL)
	{
		MCExecPoint ep(NULL, NULL, NULL);
		s_strip_paragraph_ep = &ep;
		TokenizeField(t_target, t_state, f_type, t_start, t_end, strip_paragraph, false);
		f_output -> set(ep);
	}

	return t_status;
}

///////////////////////////////////////////////////////////////////////////////

MCIdeScriptTokenize::MCIdeScriptTokenize(void)
	: m_script(NULL), m_with_location(false)
{
}

MCIdeScriptTokenize::~MCIdeScriptTokenize(void)
{
	delete m_script;
}

Parse_stat MCIdeScriptTokenize::parse(MCScriptPoint& sp)
{
	initpoint(sp);

	Parse_stat t_stat;
	t_stat = PS_NORMAL;

	if (t_stat == PS_NORMAL)
	{
		m_script = new MCChunk(True);
		t_stat = m_script -> parse(sp, False);
	}

	if (t_stat == PS_NORMAL && sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		if (sp . skip_token(SP_FACTOR, TT_PROPERTY, P_LOCATION) == PS_NORMAL)
			m_with_location = true;
		else
			t_stat = PS_ERROR;
	}

	return t_stat;
}

Exec_stat MCIdeScriptTokenize::exec(MCExecPoint& ep)
{
	Exec_stat t_stat;
	t_stat = ES_NORMAL;

	// By default, the result is empty
	MCresult -> clear();

	if (t_stat == ES_NORMAL)
		t_stat = m_script -> eval(ep);

	if (t_stat == ES_NORMAL)
	{
		// SP takes a copy of the string in ep in this form.
		MCScriptPoint sp(ep);

		// Clear the ep, its where we accumulate our result
		ep . clear();

		// This flag will be set if the last thing we output was a newline
		bool t_first_on_line;
		t_first_on_line = true;

		// If this flag is true then it means we are currently inside an 'escaping'
		// block - i.e. inside the parameter list of format/matchText/matchChunk.
		bool t_in_escapes;
		t_in_escapes = false;

		// We record the number of brackets required to turn off 'escapes' - this 
		// is only meaningful is t_in_escapes is true.
		uint32_t t_escapes_depth;
		t_escapes_depth = 0;

		for(;;)
		{
			Parse_stat t_stat;
			Symbol_type t_type;

			// Fetch the next token - and return an error in the result if
			// its lexically malformed.
			t_stat = sp . next(t_type);
			if (t_stat == PS_ERROR)
			{
				MCresult -> sets("bad script");
				return ES_NORMAL;
			}

			// If we've got an EOF, then we are done
			if (t_stat == PS_EOF || t_type == ST_EOF)
				break;

			// If we've got an EOL then output a newline, and skip.
			if (t_stat == PS_EOL || t_type == ST_EOL)
			{
				// Skip-eol returns either PS_NORMAL, or PS_EOF - we are done
				// if it was an eof.
				if (sp . skip_eol() == PS_EOF)
					break;
				
				// Output a line break but only if we aren't already first on
				// the line
				if (!t_first_on_line)
					ep . appendnewline();

				// Set the first on line flag to stop a tab being output before
				// the next token
				t_first_on_line = true;

				continue;
			}

			// Otherwise we have a normal token - if it is a literal though, we need
			// to adjust so we get the quotes back.
			MCString t_token;
			t_token = sp . gettoken();
			if (t_type == ST_LIT)
				t_token . set(t_token . getstring() - 1, t_token . getlength() + 2);

			// Now, if we have found an id, we need to check to see if we are looking
			// at a potential format(), matchText() or matchChunk() function call.
			// At present I believe it is enough to look for the sequence:
			//   ( "format" | "matchChunk" | "matchText" ) "("
			// This is because the only place these id's can exist is as a function.
			if (!t_in_escapes && t_type == ST_ID && (sp . gettoken() == "format" || sp . gettoken() == "matchChunk" || sp . gettoken() == "matchText"))
			{
				Parse_stat t_next_stat;
				Symbol_type t_next_type;
				t_next_stat = sp . next(t_next_type);
				if (t_next_stat == PS_NORMAL && t_next_type == ST_LP)
					t_in_escapes = true;
				sp . backup();
			}

			// If we are processing escapes and are looking at some kind of bracket
			// we adjust the depth appropriately.
			if (t_in_escapes)
			{
				if (t_type == ST_LP)
				{
					if (t_escapes_depth == 0)
						sp . allowescapes(True);
					t_escapes_depth += 1;
				}
				else if (t_type == ST_RP)
				{
					t_escapes_depth -= 1;
					if (t_escapes_depth == 0)
					{
						sp . allowescapes(False);
						t_in_escapes = false;
					}
				}
			}

			// If we don't want location info, just concatenate the token. Otherwise
			// output line and row first.
			if (!m_with_location)
				ep . concatmcstring(t_token, EC_TAB, t_first_on_line);
			else
			{
				if (!t_first_on_line)
					ep . appendchar('\t');
				ep . appendstringf("%u,%u %.*s", sp . getline(), sp . getpos(), t_token . getlength(), t_token . getstring());
			}

			// We are no longer the first on the line
			t_first_on_line = false;
		}

		// We have our output, so now set the chunk back to it
		t_stat = m_script -> set(ep, PT_INTO);
	}

	return t_stat;
}

///////////////////////////////////////////////////////////////////////////////

MCIdeScriptClassify::MCIdeScriptClassify(void)
	: m_script(NULL), m_target(NULL), m_output(NULL)
{
}

MCIdeScriptClassify::~MCIdeScriptClassify(void)
{
	delete m_script;
	delete m_target;
	delete m_output;
}

Parse_stat MCIdeScriptClassify::parse(MCScriptPoint& p_script)
{
	Parse_stat t_status;
	t_status = PS_NORMAL;

	if (t_status == PS_NORMAL)
		t_status = p_script . parseexp(False, False, &m_script);

	if (t_status == PS_NORMAL)
		t_status = p_script . skip_token(SP_FACTOR, TT_IN, PT_IN);
	
	if (t_status == PS_NORMAL)
	{
		m_target = new MCChunk(False);
		t_status = m_target -> parse(p_script, False);
	}
	
	if (t_status == PS_NORMAL)
		getit(p_script, m_output);

	return t_status;
}

static bool searchforhandlerinlist(MCHandlerlist *p_list, MCNameRef p_name, Handler_type& r_type)
{
	if (p_list == nil)
		return false;
	
	if (p_list -> hashandler(HT_MESSAGE, p_name))
	{
		r_type = HT_MESSAGE;
		return true;
	}
	
	if (p_list -> hashandler(HT_FUNCTION, p_name))
	{
		r_type = HT_FUNCTION;
		return true;
	}
	
	return false;
}

static bool searchforhandlerinexternallist(MCExternalHandlerList *p_list, MCNameRef p_name, Handler_type& r_type)
{
	if (p_list == nil)
		return false;
	
	if (p_list -> HasHandler(p_name, HT_MESSAGE))
	{
		r_type = HT_MESSAGE;
		return true;
	}
	
	if (p_list -> HasHandler(p_name, HT_FUNCTION))
	{
		r_type = HT_FUNCTION;
		return true;
	}
	
	return false;
}

static bool searchforhandlerinobject(MCObject *p_object, MCNameRef p_handler, Handler_type& r_type)
{
	if (p_object == nil)
		return false;
	
	if (searchforhandlerinlist(p_object -> gethandlers(), p_handler, r_type))
		return true;
	
	if (p_object -> getparentscript() != nil)
	{
		MCObject *t_behavior;
		t_behavior = p_object -> getparentscript() -> GetObject();
		if (t_behavior != nil)
		{
			t_behavior -> parsescript(True);
			if (searchforhandlerinobject(t_behavior, p_handler, r_type))
				return true;
		}
	}
	
	if (p_object -> gettype() == CT_STACK &&
		searchforhandlerinexternallist(((MCStack *)p_object) -> getexternalhandlers(), p_handler, r_type))
		return true;
	
	return false;
}

static bool searchforhandlerinlibrarystacks(MCNameRef p_handler, Handler_type& r_type)
{
	for (uint32_t i = 0; i < MCnusing; i++)
	{
		if (searchforhandlerinobject(MCusing[i], p_handler, r_type))
			return true;
	}
	
	return false;	
}

static bool searchforhandlerinobjectlist(MCObjectList *p_list, MCNameRef p_name, Handler_type& r_type)
{
	if (p_list == nil)
		return false;
	
	MCObjectList *t_object_ref;
	t_object_ref = p_list;
	do
	{
		if (!t_object_ref -> getremoved() &&
			searchforhandlerinobject(t_object_ref -> getobject(), p_name, r_type))
			return true;
		
		t_object_ref = t_object_ref -> next();
	}
	while (t_object_ref != p_list);

	return false;
}

// MM-2013-07-26: [[ Bug 11017 ]] Make sure we search the full message path for handler.
//    FrontScripts
//      Behavior chain of FrontScript objects
//      (if stack, then externals)
//    Object
//      Behavior chain of object
//      (if stack, then externals)
//    Object's Ancestor's
//      Behavior chain of object ancestors
//      (if stack, then externals)
//    Backscripts
//      Behavior chain of backscript objects
//      (if stack, then externals)
//    Library Stacks
//      Behavior chain of stack
//      Externals of stack
static bool searchforhandler(MCObject *p_object, MCNameRef p_handler, Handler_type& r_type)
{
	if (searchforhandlerinobjectlist(MCfrontscripts, p_handler, r_type))
		return true;
	
	for (MCObject *t_object = p_object; t_object != nil; t_object = t_object -> getparent())
	{
		t_object -> parsescript(False);
		if (searchforhandlerinobject(t_object, p_handler, r_type))
			return true;	
	}
	
	if (searchforhandlerinobjectlist(MCbackscripts, p_handler, r_type))
		return true;
	
	if (searchforhandlerinlibrarystacks(p_handler, r_type))
		return true;
	
	return false;
}

Exec_stat MCIdeScriptClassify::exec(MCExecPoint& ep)
{
	Exec_stat t_stat;
	t_stat = ES_NORMAL;

	// By default, the result is empty
	MCresult -> clear();

	// Evaluate the target object we are classifying in the context of.
	MCObject *t_object;
	uint32_t t_part_id;
	t_object = nil;
	if (t_stat == ES_NORMAL)
		t_stat = m_target -> getobj(ep, t_object, t_part_id, True);
	
	// Evaluate the script.
	if (t_stat == ES_NORMAL)
		t_stat = m_script -> eval(ep);
	
	// First try a (command) call.
	char *t_call_error;
	uint2 t_call_pos;
	t_call_error = nil;
	if (t_stat == ES_NORMAL)
	{
		// SP takes a copy of the string in ep in this form.
		MCScriptPoint sp(ep);
		
		// Clear the parse errors.
		MCperror -> clear();
		
		// Now see if we can parse a call.
		MCComref *t_call;
		Symbol_type t_type;
		t_call = nil;
		if (sp . next(t_type) == PS_NORMAL)
		{
			const LT *t_entry;
			if (sp . lookup(SP_COMMAND, t_entry) != PS_NORMAL)
			{
				if (t_type != ST_ID)
					MCperror -> add(PE_HANDLER_NOCOMMAND, sp);
				else
				{
					// Now we must search to see what kind of handler (if any)
					// might get invoked and if it is a command, then we use
					// a comref.
					Handler_type t_htype;
					if (searchforhandler(t_object, sp.gettoken_nameref(), t_htype) &&
						t_htype == HT_MESSAGE)
						t_call = new MCComref(sp.gettoken_nameref());
				}
			}
		}
		
		if (t_call == nil ||
			t_call -> parse(sp) != PS_NORMAL ||
			sp . next(t_type) != PS_EOF)
		{
			t_call_error = MCperror -> getsvalue() . clone();
			t_call_pos = sp . getline() * 1000 + sp . getpos();
		}
		
		delete t_call;
	}
	
	// First try an expression.
	char *t_expr_error;
	uint2 t_expr_pos;
	t_expr_error = nil;
	if (t_stat == ES_NORMAL)
	{
		// SP takes a copy of the string in ep in this form.
		MCScriptPoint sp(ep);
		
		// Clear the parse errors.
		MCperror -> clear();
	
		// Now see if we can parse an expression
		MCExpression *t_expr;
		Symbol_type t_type;
		t_expr = nil;
		if (sp . parseexp(False, True, &t_expr) != PS_NORMAL ||
			sp . next(t_type) != PS_EOF)
		{
			t_expr_error = MCperror -> getsvalue() . clone();
			t_expr_pos = sp . getline() * 1000 + sp . getpos();
		}
			
		delete t_expr;
	}
	
	// Now try a command.
	char *t_cmd_error;
	uint2 t_cmd_pos;
	t_cmd_error = nil;
	if (t_stat == ES_NORMAL)
	{
		// SP takes a copy of the string in ep in this form.
		MCScriptPoint sp(ep);
		
		// Clear the parse errors.
		MCperror -> clear();
	
		// Now see if we can parse a command.
		MCStatement *t_statement;
		Symbol_type t_type;
		t_statement = nil;
		if (sp . next(t_type) == PS_NORMAL)
		{
			const LT *t_entry;
			if (sp . lookup(SP_COMMAND, t_entry) == PS_NORMAL)
			{
				if (t_entry -> type == TT_STATEMENT)
					t_statement = MCN_new_statement(t_entry -> which);
			}
		}
		
		if (t_statement == nil ||
			t_statement -> parse(sp) != PS_NORMAL ||
			sp . next(t_type) != PS_EOF)
		{
			t_cmd_error = MCperror -> getsvalue() . clone();
			t_cmd_pos = sp . getline() * 1000 + sp . getpos();
		}
			
		delete t_statement;
	}
	
	// If we have a call expression, then its a command.
	if (t_call_error == nil ||
		t_cmd_error == nil)
		m_output -> sets("command");
	else if (t_expr_error == nil)
		m_output -> sets("expression");
	else
	{
		const char *t_error;
		if (t_expr_pos > MCU_max(t_call_pos, t_cmd_pos))
			t_error = t_expr_error;
		else if (t_call_pos > t_cmd_pos)
			t_error = t_call_error;
		else
			t_error = t_cmd_error;
		
		MCresult -> copysvalue(t_error);
		m_output -> sets("neither");
	}

	delete t_expr_error;
	delete t_cmd_error;
	delete t_call_error;
	
	return ES_NORMAL;
}

///////////////////////////////////////////////////////////////////////////////

// filter controls of stack <stack> where <prop> <op> <pattern>

struct MCIdeFilterControlsVisitor: public MCObjectVisitor
{
	MCExecPoint& m_ep;
	MCIdeFilterControlsProperty m_property;
	MCIdeFilterControlsOperator m_operator;
	MCExecPoint m_pattern;
	
	uint32_t m_card_id;
	
	MCIdeFilterControlsVisitor(MCExecPoint& ep, MCIdeFilterControlsProperty p_property, MCIdeFilterControlsOperator p_operator, const char *p_pattern)
		: m_ep(ep), m_property(p_property), m_operator(p_operator)
	{
		m_pattern . setstaticcstring(p_pattern);
	}
	
	virtual bool OnCard(MCCard *p_card)
	{
		m_card_id = p_card -> getid();
		return MCObjectVisitor::OnCard(p_card);
	}
	
	virtual bool OnObject(MCObject *p_object)
	{
		MCExecPoint t_left_ep;
		
		switch(m_property)
		{
			case kMCIdeFilterPropertyScriptLines:
			{
				char *t_script;
				uindex_t t_count;
				t_script = p_object -> getscript();
				t_count = 0;
				if (t_script != nil)
				{
					while(*t_script != '\0')
					{
						if (*t_script == '\n')
							t_count += 1;
						t_script++;
					}
				}
				t_left_ep . setuint(t_count);
			}
			break;
			case kMCIdeFilterPropertyName:
				t_left_ep . setnameref_unsafe(p_object -> getname());
				break;
			case kMCIdeFilterPropertyVisible:
				t_left_ep . setboolean(p_object -> getflag(F_VISIBLE));
				break;
			case kMCIdeFilterPropertyType:
				t_left_ep . setstaticcstring(p_object -> gettypestring());
				break;
		}
		
		bool t_accept;
		t_accept = false;
		switch(m_operator)
		{
			case kMCIdeFilterOperatorLessThan:
				t_accept = MCExpression::compare_values(t_left_ep, m_pattern, &m_ep, false) < 0;
				break;
			case kMCIdeFilterOperatorLessThanOrEqual:
				t_accept = MCExpression::compare_values(t_left_ep, m_pattern, &m_ep, false) <= 0;
				break;
			case kMCIdeFilterOperatorEqual:
				t_accept = MCExpression::compare_values(t_left_ep, m_pattern, &m_ep, false) == 0;
				break;
			case kMCIdeFilterOperatorNotEqual:
				t_accept = MCExpression::compare_values(t_left_ep, m_pattern, &m_ep, false) != 0;
				break;
			case kMCIdeFilterOperatorGreaterThanOrEqual:
				t_accept = MCExpression::compare_values(t_left_ep, m_pattern, &m_ep, false) >= 0;
				break;
			case kMCIdeFilterOperatorGreaterThan:
				t_accept = MCExpression::compare_values(t_left_ep, m_pattern, &m_ep, false) > 0;
				break;
			case kMCIdeFilterOperatorBeginsWith:
			{
				MCString t_whole, t_part;
				t_whole = t_left_ep . getsvalue();
				t_part = m_pattern . getsvalue();
				if (t_whole . getlength() < t_part . getlength())
					t_accept = false;
				else
					t_accept = MCU_strncasecmp(t_whole . getstring(), t_part . getstring(), t_part . getlength()) == 0;
			}
			break;
			case kMCIdeFilterOperatorEndsWith:
			{
				MCString t_whole, t_part;
				t_whole = t_left_ep . getsvalue();
				t_part = m_pattern . getsvalue();
				if (t_whole . getlength() < t_part . getlength())
					t_accept = false;
				else
					t_accept = MCU_strncasecmp(t_whole . getstring() + t_whole . getlength() - t_part . getlength(), t_part . getstring(), t_part . getlength()) == 0;
			}
			break;
			case kMCIdeFilterOperatorContains:
			{
				uint4 i;
				i = 0;
				t_accept = MCU_offset(m_pattern.getsvalue(), t_left_ep.getsvalue(), i, False) == True;
				
			}
			break;
		}
		
		if (t_accept)
			m_ep . appendstringf(m_ep . isempty() ? "%d,%d" : "\n%d,%d", p_object -> getid(), m_card_id);
		
		return true;
	}
};

MCIdeFilterControls::MCIdeFilterControls(void)
	: m_property(kMCIdeFilterPropertyNone), m_operator(kMCIdeFilterOperatorNone), m_pattern(nil), m_stack(nil), m_it(nil)
{
}

MCIdeFilterControls::~MCIdeFilterControls(void)
{
	delete m_pattern;
	delete m_stack;
	delete m_it;
}

Parse_stat MCIdeFilterControls::parse(MCScriptPoint& sp)
{
	Parse_stat t_stat;
	t_stat = PS_NORMAL;
	
	if (t_stat == PS_NORMAL)
		t_stat = sp . skip_token(SP_FACTOR, TT_OF, PT_OF);

	if (t_stat == PS_NORMAL)
	{
		m_stack = new MCChunk(false);
		t_stat = m_stack -> parse(sp, False);
	}
	
	if (t_stat == PS_NORMAL)
		t_stat = sp . skip_token(SP_MARK, TT_UNDEFINED, MC_WHERE);
	
	Symbol_type t_type;
	if (t_stat == PS_NORMAL && (sp . next(t_type) != PS_NORMAL || t_type != ST_ID))
		t_stat = PS_ERROR;
	
	if (t_stat == PS_NORMAL)
	{
		if (sp . gettoken() == "scriptlines")
			m_property = kMCIdeFilterPropertyScriptLines;
		else if (sp . gettoken() == "name")
			m_property = kMCIdeFilterPropertyName;
		else if (sp . gettoken() == "visible")
			m_property = kMCIdeFilterPropertyVisible;
		else if (sp . gettoken() == "type")
			m_property = kMCIdeFilterPropertyType;
		else
			t_stat = PS_ERROR;
	}
	
	if (t_stat == PS_NORMAL)
	{
		if (sp . skip_token(SP_FACTOR, TT_BINOP, O_LT) == PS_NORMAL)
			m_operator = kMCIdeFilterOperatorLessThan;
		else if (sp . skip_token(SP_FACTOR, TT_BINOP, O_LE) == PS_NORMAL)
			m_operator = kMCIdeFilterOperatorLessThanOrEqual;
		else if (sp . skip_token(SP_FACTOR, TT_BINOP, O_EQ) == PS_NORMAL)
			m_operator = kMCIdeFilterOperatorEqual;
		else if (sp . skip_token(SP_FACTOR, TT_BINOP, O_NE) == PS_NORMAL)
			m_operator = kMCIdeFilterOperatorNotEqual;
		else if (sp . skip_token(SP_FACTOR, TT_BINOP, O_GE) == PS_NORMAL)
			m_operator = kMCIdeFilterOperatorGreaterThanOrEqual;
		else if (sp . skip_token(SP_FACTOR, TT_BINOP, O_GT) == PS_NORMAL)
			m_operator = kMCIdeFilterOperatorGreaterThan;
		else if (sp . skip_token(SP_FACTOR, TT_BINOP, O_BEGINS_WITH) == PS_NORMAL)
		{
			if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
				m_operator = kMCIdeFilterOperatorBeginsWith;
			else
				t_stat = PS_ERROR;
		}
		else if (sp . skip_token(SP_FACTOR, TT_BINOP, O_ENDS_WITH) == PS_NORMAL)
		{
			if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
				m_operator = kMCIdeFilterOperatorEndsWith;
			else
				t_stat = PS_ERROR;
		}
		else if (sp . skip_token(SP_FACTOR, TT_BINOP, O_CONTAINS) == PS_NORMAL)
			m_operator = kMCIdeFilterOperatorContains;
	}
	
	if (t_stat == PS_NORMAL)
		t_stat = sp . parseexp(False, True, &m_pattern);
		
	if (t_stat == PS_NORMAL)
		getit(sp, m_it);
	
	return t_stat;
}

Exec_stat MCIdeFilterControls::exec(MCExecPoint& ep)
{
	Exec_stat t_stat;
	t_stat = ES_NORMAL;
	
	MCObject *t_stack;
	uint32_t t_part_id;
	if (t_stat == ES_NORMAL)
		t_stat = m_stack -> getobj(ep, t_stack, t_part_id, True);
	
	if (t_stat == ES_NORMAL && t_stack -> gettype() != CT_STACK)
		t_stat = ES_ERROR;
	
	MCAutoPointer<char> t_pattern;
	if (t_stat == ES_NORMAL)
	{
		t_stat = m_pattern -> eval(ep);
		if (t_stat == ES_NORMAL)
			t_pattern = ep . getsvalue() . clone();
	}
	
	if (t_stat == ES_NORMAL)
	{
		MCIdeFilterControlsVisitor t_visitor(ep, m_property, m_operator, *t_pattern);
		
		ep . clear();
		
		MCCard *t_card, *t_cards;
		t_cards = ((MCStack *)t_stack) -> getcards();
		t_card = t_cards;
		do
		{
			t_card -> visit(VISIT_STYLE_DEPTH_LAST, 0, &t_visitor);
			t_card = t_card -> next();
		}
		while(t_card != t_cards);

		m_it -> set(ep, False);
	}
	
	return t_stat;
}

///////////////////////////////////////////////////////////////////////////////
