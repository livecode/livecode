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
		delete[] file . types; /* Allocated with new[] */
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
	{
		if (sp . lookup(SP_ASK, t_literal) == PS_NORMAL)
			mode = (Ask_type)t_literal -> which;
		else
			sp . backup();
	}

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
			mode = AT_QUESTION;
			t_error = parse_question(sp);
		break;
	}

	if (t_error == PE_UNDEFINED && sp . skip_token(SP_ASK, TT_UNDEFINED, AT_TITLED) == PS_NORMAL)
		if (sp . parseexp(False, True, &title) != PS_NORMAL)
			t_error = PE_ANSWER_BADTITLE;
	
	if (t_error == PE_UNDEFINED && sp . skip_token(SP_FACTOR, TT_PREP, PT_AS) == PS_NORMAL)
	{
		if (sp . skip_token(SP_ASK, TT_UNDEFINED, AT_SHEET) == PS_NORMAL)
			sheet = True;
		else
			t_error = PE_ANSWER_BADRESPONSE;
	}
			
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
                // We only want to resolve the path if it is relative
                // (otherwise it will be created where LiveCode is located)
                if (MCStringContains(*t_initial, MCSTR("/"), kMCStringOptionCompareExact))
                {
                    // IM-2014-08-06: [[ Bug 13096 ]] Allow file dialogs to work with relative paths by resolving to absolute
                    if (!MCS_resolvepath(*t_initial, &t_initial_resolved))
                    {
                        ctxt . LegacyThrow(EE_NO_MEMORY);
                        return;
                    }
                }
                else
                {
                    // We simply take the initial path as it is
                    t_initial_resolved = *t_initial;
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
