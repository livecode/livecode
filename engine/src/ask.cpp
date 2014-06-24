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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "scriptpt.h"
#include "util.h"
#include "date.h"
#include "sellst.h"
#include "stack.h"
#include "aclip.h"
#include "player.h"
#include "card.h"
#include "param.h"
#include "mcerror.h"
#include "ans.h"
#include "debug.h"
#include "dispatch.h"
#include "securemode.h"

#include "globals.h"

#include "meta.h"
#include "ask.h"

extern const char *MCdialogtypes[];

MCAsk::~MCAsk(void)
{
	delete it;
	delete title;

	switch(mode)
	{
	case AT_PASSWORD:
	case AT_CLEAR:
		delete password . prompt;
		delete password . answer;
	break;

	case AT_FILE:
		delete file . prompt;
		delete file . initial;
		delete file . filter;
		for(uint4 t_type = 0; t_type < file . type_count; ++t_type)
			delete file . types[t_type];
		delete file . types;
	break;

	default:
		delete question . prompt;
		delete question . answer;
	break;
	}
}

Parse_stat MCAsk::parse(MCScriptPoint &sp)
{
	Parse_errors t_error = PE_UNDEFINED;

	Symbol_type t_type;
	const LT *t_literal;

	initpoint(sp);
	getit(sp, it);

	if (sp . next(t_type) == PS_NORMAL)
		if (sp . lookup(SP_ASK, t_literal) == PS_NORMAL)
			mode = (Ask_type)t_literal -> which;
		else
			sp . backup();

	// MW-2008-07-23: [[ Bug 6821 ]] ask files "foo" crashes.
	//   This is because the mode check is not strict enough. If the given ask
	//   mode is not known, we backup and then parse it as a question form.
	switch(mode)
	{
		case AT_PASSWORD:
			t_error = parse_password(sp);
		break;
	
		case AT_FILE:
			t_error = parse_file(sp);
		break;

		case AT_INFORMATION:
		case AT_QUESTION:
		case AT_ERROR:
		case AT_WARNING:
		case AT_UNDEFINED:
			t_error = parse_question(sp);
		break;

		default:
			sp . backup();
			t_error = parse_question(sp);
		break;
	}

	if (t_error == PE_UNDEFINED && sp . skip_token(SP_ASK, TT_UNDEFINED, AT_TITLED) == PS_NORMAL)
		if (sp . parseexp(False, True, &title) != PS_NORMAL)
			t_error = PE_ANSWER_BADTITLE;
	
	if (t_error == PE_UNDEFINED && sp . skip_token(SP_FACTOR, TT_PREP, PT_AS) == PS_NORMAL)
		if (sp . skip_token(SP_ASK, TT_UNDEFINED, AT_SHEET) == PS_NORMAL)
			sheet = True;
		else
			t_error = PE_ANSWER_BADRESPONSE;
			
	if (t_error != PE_UNDEFINED)
	{
		MCperror -> add(t_error, sp);
		return PS_ERROR;
	}
	
	return PS_NORMAL;
}

Parse_errors MCAsk::parse_password(MCScriptPoint& sp)
{
	Parse_errors t_error = PE_UNDEFINED;

	if (sp . skip_token(SP_ASK, TT_UNDEFINED, AT_CLEAR) == PS_NORMAL)
		mode = AT_CLEAR;

	if (sp . parseexp(False, True, &password . prompt) != PS_NORMAL)
		t_error = PE_ASK_BADQUESTION;

	if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		// MW-2012-10-12: [[ Bug 10377 ]] Check for 'hint' mode.
		if (sp . skip_token(SP_ASK, TT_UNDEFINED, AT_HINT) == PS_NORMAL)
			password . hint = true;
		else
			password . hint = false;
		if (sp . parseexp(False, True, &password . answer) != PS_NORMAL)
			t_error = PE_ASK_BADREPLY;
	}

	return t_error;
}

Parse_errors MCAsk::parse_question(MCScriptPoint& sp)
{
	Parse_errors t_error = PE_UNDEFINED;

	if (sp . parseexp(False, True, &question . prompt) != PS_NORMAL)
		t_error = PE_ASK_BADQUESTION;

	if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		// MW-2012-10-12: [[ Bug 10377 ]] Check for 'hint' mode.
		if (sp . skip_token(SP_ASK, TT_UNDEFINED, AT_HINT) == PS_NORMAL)
			question . hint = true;
		else
			question . hint = false;
		if (sp . parseexp(False, True, &question . answer) != PS_NORMAL)
			t_error = PE_ASK_BADREPLY;
	}

	return t_error;
}

Parse_errors MCAsk::parse_file(MCScriptPoint& sp)
{
	Parse_errors t_error = PE_UNDEFINED;

	if (sp . parseexp(False, True, &file . prompt) != PS_NORMAL)
		t_error = PE_ASK_BADQUESTION;

	if (t_error == PE_UNDEFINED && sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		if (sp . skip_token(SP_COMMAND, TT_STATEMENT, S_FILTER) == PS_NORMAL || sp . skip_token(SP_COMMAND, TT_STATEMENT, S_TYPE) == PS_NORMAL)
			sp . backup(), sp . backup();
		else if (sp . parseexp(False, True, &file . initial) != PS_NORMAL)
			t_error = PE_ASK_BADREPLY;
	}

	if (t_error == PE_UNDEFINED && sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		if (sp . skip_token(SP_COMMAND, TT_STATEMENT, S_FILTER) == PS_NORMAL)
		{
			if (sp . parseexp(False, True, &file . filter) != PS_NORMAL)
				t_error = PE_ASK_BADREPLY;
		}
		else if (sp . skip_token(SP_COMMAND, TT_STATEMENT, S_TYPE) == PS_NORMAL)
		{
			do
			{
				MCU_realloc((char **)&file . types, file . type_count, file . type_count + 1, sizeof(MCExpression *));
				if (sp . parseexp(True, True, &file . types[file . type_count]) != PS_NORMAL)
					t_error = PE_ASK_BADREPLY;
				else
					file . type_count += 1;
			}
			while(!t_error && sp . skip_token(SP_FACTOR, TT_BINOP, O_OR) == PS_NORMAL &&
						(sp . skip_token(SP_COMMAND, TT_STATEMENT, S_TYPE) == PS_NORMAL || sp . skip_token(SP_ASK, TT_UNDEFINED, AT_TYPES) == PS_NORMAL));
		}
		else
			t_error = PE_ASK_BADREPLY;
	}

	return t_error;
}

Exec_stat MCAsk::exec(class MCExecPoint& ep)
{
	Exec_errors t_error = EE_UNDEFINED;
	Meta::cstring_value t_title;

	MCresult -> clear(False);

	t_error = Meta::evaluate(ep, title, t_title, EE_ANSWER_BADTITLE);
	if (!t_error)
		switch(mode)
		{
		case AT_PASSWORD:
		case AT_CLEAR:
			t_error = exec_password(ep, t_title);
		break;

		case AT_FILE:
			t_error = exec_file(ep, t_title);
		break;

		default:
			t_error = exec_question(ep, t_title);
		break;
		}

	if (!t_error)
		it -> set(ep);
	else
		MCeerror -> add(t_error, line, pos);

	return t_error ? ES_ERROR : ES_NORMAL;
}

#ifdef /* MCAsk::exec_question */ LEGACY_EXEC
Exec_errors MCAsk::exec_question(MCExecPoint& ep, const char *p_title)
{
	Exec_errors t_error = EE_UNDEFINED;

	Meta::cstring_value t_prompt, t_answer;
	
	MCresult -> clear(False);
	
	t_error = Meta::evaluate(ep, question . prompt, t_prompt, EE_ASK_BADREPLY, question . answer, t_answer, EE_ASK_BADREPLY);
	
#ifndef _MOBILE
	if (!t_error)
	{
		bool t_cancelled;
		t_error = exec_custom(ep, t_cancelled, MCasknamestring, MCdialogtypes[mode], 3, p_title, *t_prompt, *t_answer);
		if (t_cancelled)
			MCresult -> sets(MCcancelstring);
	}
#else
	if (!t_error)
	{
		char *t_result;
		t_result = MCscreen -> popupaskdialog(AT_QUESTION, p_title, *t_prompt, *t_answer, question . hint);
		if (t_result != nil)
			ep . copysvalue(t_result);
		else
		{
			ep.clear();
			MCresult -> sets(MCcancelstring);
		}
		delete t_result;
	}
#endif

	return t_error;
}
#endif /* MCAsk::exec_question */

#ifdef /* MCAsk::exec_password */ LEGACY_EXEC
Exec_errors MCAsk::exec_password(MCExecPoint& ep, const char *p_title)
{
	Exec_errors t_error = EE_UNDEFINED;

	Meta::cstring_value t_prompt, t_answer;
	
	MCresult -> clear(False);
	
	t_error = Meta::evaluate(ep, password . prompt, t_prompt, EE_ASK_BADREPLY, password . answer, t_answer, EE_ASK_BADREPLY);

#ifndef _MOBILE
	if (!t_error)
	{
		bool t_cancelled;
		t_error = exec_custom(ep, t_cancelled, MCasknamestring, MCdialogtypes[mode], 3, p_title, *t_prompt, *t_answer);
		if (t_cancelled)
			MCresult -> sets(MCcancelstring);
	}
#else
	if (!t_error)
	{
		char *t_result;
		t_result = MCscreen -> popupaskdialog(AT_PASSWORD, p_title, *t_prompt, *t_answer, password . hint);
		if (t_result != nil)
			ep . copysvalue(t_result);
		else
		{
			ep.clear();
			MCresult -> sets(MCcancelstring);
		}
		delete t_result;
	}
#endif
	
	return t_error;
}
#endif /* MCAsk::exec_password */

#ifdef /* MCAsk::exec_file */ LEGACY_EXEC
Exec_errors MCAsk::exec_file(MCExecPoint& ep, const char *p_title)
{
	Exec_errors t_error = EE_UNDEFINED;

	Meta::cstring_value t_prompt, t_initial, t_filter;
	Meta::cstring_value *t_types = NULL;
	char **t_type_strings = NULL;
	uint4 t_type_count = 0;

	t_error = Meta::evaluate(ep,
								file . prompt, t_prompt, EE_ANSWER_BADQUESTION,
								file . initial, t_initial, EE_ANSWER_BADRESPONSE,
								file . filter, t_filter, EE_ANSWER_BADRESPONSE);
	
	if (!t_error && file . type_count > 0)
	{
		t_types = new Meta::cstring_value[file . type_count];
		for(uint4 t_type_index = 0; t_type_index < file . type_count && !t_error; ++t_type_index)
		{
			t_error = Meta::evaluate(ep, file . types[t_type_index], t_types[t_type_index], EE_ANSWER_BADRESPONSE);
			if (!t_error)
				for(char *t_type_string = strtok(*t_types[t_type_index], "\n"); t_type_string != NULL; t_type_string = strtok(NULL, "\n"))
				{
					MCU_realloc((char **)&t_type_strings, t_type_count, t_type_count + 1, sizeof(char *));
					t_type_strings[t_type_count++] = t_type_string;
				}
		}
	}

	if (!t_error && !MCSecureModeCanAccessDisk())
		t_error = EE_DISK_NOPERM;

	if (!t_error)
	{
		if (MCsystemFS)
		{
			unsigned int t_options = 0;
			if (sheet)
				t_options |= MCA_OPTION_SHEET;

			if (t_types != NULL)
				MCA_ask_file_with_types(ep, p_title, t_prompt, t_type_strings, t_type_count, t_initial, t_options);
			else
				MCA_ask_file(ep, p_title, t_prompt, t_filter, t_initial, t_options);
		}
		else
		{
			bool t_cancelled;
			MCExecPoint ep2(ep);
			ep2 . clear();
			for(uint4 t_type = 0; t_type < t_type_count; ++t_type)
				ep2 . concatcstring(t_type_strings[t_type], EC_RETURN, t_type == 0);
			t_error = exec_custom(ep, t_cancelled, MCfsnamestring, mode == AT_FILE ? "file" : "files", 5, p_title, *t_prompt, *t_filter, *t_initial, ep2 . getsvalue() . getstring());
		}
		
		if (ep . getsvalue() == MCnullmcstring && t_types == NULL)
			MCresult -> sets(MCcancelstring);
	}

	delete[] t_types;
	delete t_type_strings;

	return t_error;
}
#endif /* MCAsk::exec_file */

#ifdef /* MCAsk::exec_custom */ LEGACY_EXEC
Exec_errors MCAsk::exec_custom(MCExecPoint& ep, bool& p_cancelled, const MCString& p_stack, const char *p_type, unsigned int p_count, ...)
{
	ep . setstringf("ask %s", p_type);

	va_list t_args;
	va_start(t_args, p_count);
	for(; p_count > 0; p_count -= 1)
	{
		const char *t_string = va_arg(t_args, const char *);
		ep . concatcstring(t_string == NULL ? "" : t_string, EC_NULL, false);
	}
	va_end(t_args);

	MCdialogdata -> store(ep, True);

	MCStack *t_stack;
	t_stack = ep . getobj() -> getstack() -> findstackname(p_stack);

	Boolean t_old_trace = MCtrace;
	MCtrace = False;
	if (t_stack != NULL)
	{
		MCdialogdata -> store(ep, True);
		if (MCdefaultstackptr -> getopened() || MCtopstackptr == NULL)
			t_stack -> openrect(MCdefaultstackptr -> getrect(), sheet ? WM_SHEET : WM_MODAL, sheet ? MCdefaultstackptr: NULL, WP_DEFAULT, OP_NONE);
		else
			t_stack -> openrect(MCtopstackptr -> getrect(), sheet ? WM_SHEET : WM_MODAL, sheet ? MCtopstackptr : NULL, WP_DEFAULT, OP_NONE);
	}
	MCtrace = t_old_trace;

	MCdialogdata -> fetch(ep);
	if (ep . getsvalue() . getlength() == 1 && *(ep . getsvalue() . getstring()) == '\0')
	{
		ep . clear();
		p_cancelled = true;
	}
	else
		p_cancelled = false;

	return EE_UNDEFINED;
}
#endif /* MCAsk::exec_custom */