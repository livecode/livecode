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

//#include "execpt.h"
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
#include "exec.h"

#include "osspec.h"

#include "globals.h"

#include "meta.h"
#include "ask.h"

#include "syntax.h"



MCAsk::~MCAsk(void)
{
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

void MCAsk::exec_ctxt(class MCExecContext& ctxt)
{
    
    MCAutoStringRef t_title;
    if (!ctxt . EvalOptionalExprAsNullableStringRef(title, EE_ANSWER_BADTITLE, &t_title))
        return;

	switch(mode)
	{
        case AT_PASSWORD:
        case AT_CLEAR:
        {
            MCAutoStringRef t_prompt, t_answer;
            if (!ctxt . EvalOptionalExprAsNullableStringRef(password . prompt, EE_ASK_BADREPLY, &t_prompt))
                return;
            if (!ctxt . EvalOptionalExprAsNullableStringRef(password . answer, EE_ASK_BADREPLY, &t_answer))
                return;
            MCDialogExecAskPassword(ctxt, mode == AT_CLEAR, *t_prompt, *t_answer, password . hint, *t_title, sheet == True);
        }
            break;
            
        case AT_FILE:
        {
            MCAutoStringRef t_prompt, t_initial, t_filter;
            MCAutoStringRefArray t_types;
            
            if (!ctxt . EvalOptionalExprAsNullableStringRef(file.prompt, EE_ANSWER_BADQUESTION, &t_prompt))
                return;
            if (!ctxt . EvalOptionalExprAsNullableStringRef(file.initial, EE_ANSWER_BADQUESTION, &t_initial))
                return;
            if (!ctxt . EvalOptionalExprAsNullableStringRef(file.filter, EE_ANSWER_BADQUESTION, &t_filter))
                return;
            
            MCAutoStringRef t_initial_resolved;
            if (*t_initial != nil)
            {
                // IM-2014-08-06: [[ Bug 13096 ]] Allow file dialogs to work with relative paths by resolving to absolute
                if (!MCS_resolvepath(*t_initial, &t_initial_resolved))
                {
                    ctxt . LegacyThrow(EE_NO_MEMORY);
                    return;
                }
            }
            
            if (file . type_count > 0)
            {
                /* UNCHECKED */ t_types.Extend(file.type_count);
                for (uindex_t i = 0; i < file.type_count; i++)
                {
                    if (!ctxt . EvalOptionalExprAsNullableStringRef(file.types[i], EE_ANSWER_BADQUESTION, t_types[i]))
                        return;
                }
            }
            
            if (t_types.Count() > 0)
                MCDialogExecAskFileWithTypes(ctxt, *t_prompt, *t_initial_resolved, *t_types, t_types . Count(), *t_title, sheet == True);
            else if (*t_filter != nil)
                MCDialogExecAskFileWithFilter(ctxt, *t_prompt, *t_initial_resolved, *t_filter, *t_title, sheet == True);
            else
                MCDialogExecAskFile(ctxt, *t_prompt, *t_initial_resolved, *t_title, sheet == True);
        }
            break;
            
        default:
        {
            MCAutoStringRef t_prompt, t_answer;
            if (!ctxt . EvalOptionalExprAsNullableStringRef(question . prompt, EE_ASK_BADREPLY, &t_prompt))
                return;
            if (!ctxt . EvalOptionalExprAsNullableStringRef(question . answer, EE_ASK_BADREPLY, &t_answer))
                return;
            MCDialogExecAskQuestion(ctxt, mode, *t_prompt, *t_answer, question . hint, *t_title, sheet == True);
        }
            break;
	}
}

#ifdef LEGACY_EXEC
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
#endif

#ifdef LEGACY_EXEC
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
#endif

#ifdef LEGACY_EXEC
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

	char *t_initial_resolved;
	t_initial_resolved = nil;

	if (!t_error && t_initial != nil)
	{
		// IM-2014-08-06: [[ Bug 13096 ]] Allow file dialogs to work with relative paths by resolving to absolute
		t_initial_resolved = MCS_get_canonical_path(t_initial);
		if (nil == t_initial_resolved)
			t_error == EE_NO_MEMORY;
	}

	if (!t_error)
	{
		if (MCsystemFS)
		{
			unsigned int t_options = 0;
			if (sheet)
				t_options |= MCA_OPTION_SHEET;
			
			if (t_types != NULL)
				MCA_ask_file_with_types(ep, p_title, t_prompt, t_type_strings, t_type_count, t_initial_resolved, t_options);
			else
				MCA_ask_file(ep, p_title, t_prompt, t_filter, t_initial_resolved, t_options);
		}
		else
		{
			bool t_cancelled;
			MCExecPoint ep2(ep);
			ep2 . clear();
			for(uint4 t_type = 0; t_type < t_type_count; ++t_type)
				ep2 . concatcstring(t_type_strings[t_type], EC_RETURN, t_type == 0);
			t_error = exec_custom(ep, t_cancelled, MCfsnamestring, mode == AT_FILE ? "file" : "files", 5, p_title, *t_prompt, *t_filter, t_initial_resolved, ep2 . getsvalue() . getstring());
		}
		
		if (ep . getsvalue() == MCnullmcstring && t_types == NULL)
			MCresult -> sets(MCcancelstring);
	}

	delete[] t_types;
	delete t_type_strings;
	
	return t_error;
}
#endif

#ifdef LEGACY_EXEC
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
#endif

void MCAsk::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	switch(mode)
	{
	case AT_PASSWORD:
	case AT_CLEAR:
		MCSyntaxFactoryEvalConstantBool(ctxt, mode == AT_CLEAR);

		if (password . prompt != nil)
			password . prompt -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		if (password . answer != nil)
			password . answer -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryEvalConstantBool(ctxt, password . hint);

		if (title != nil)
			title -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryEvalConstantBool(ctxt, sheet == True);

		MCSyntaxFactoryExecMethod(ctxt, kMCDialogExecAskPasswordMethodInfo);
		break;

	case AT_FILE:
		if (file . prompt != nil)
			file . prompt -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		if (file . initial != nil)
			file . initial -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		if (file . filter != nil)
			file . filter -> compile(ctxt);
		else if (file . type_count > 0)
		{
			for (uindex_t i = 0; i < file . type_count; i++)
				file . types[i] -> compile(ctxt);

			MCSyntaxFactoryEvalList(ctxt, file . type_count);
		}

		if (title != nil)
			title -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryEvalConstantBool(ctxt, sheet == True);

		if (file . type_count > 0)
			MCSyntaxFactoryExecMethod(ctxt, kMCDialogExecAskFileWithTypesMethodInfo);
		else if (file . filter != nil)
			MCSyntaxFactoryExecMethod(ctxt, kMCDialogExecAskFileWithFilterMethodInfo);
		else
			MCSyntaxFactoryExecMethod(ctxt, kMCDialogExecAskFileMethodInfo);
		break;

	default:
		MCSyntaxFactoryEvalConstantBool(ctxt, mode == AT_CLEAR);

		if (question . prompt != nil)
			question . prompt -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		if (question . answer != nil)
			question . answer -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryEvalConstantBool(ctxt, question . hint);

		if (title != nil)
			title -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryEvalConstantBool(ctxt, sheet == True);

		MCSyntaxFactoryExecMethod(ctxt, kMCDialogExecAskQuestionMethodInfo);
		break;
	}

	MCSyntaxFactoryEndStatement(ctxt);
}
