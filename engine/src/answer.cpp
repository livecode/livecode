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

#include "globals.h"

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

#include "meta.h"
#include "answer.h"
#include "printer.h"

#include "exec.h"

#include "platform.h"
#include "osspec.h"

MCAnswer::~MCAnswer()
{
	delete title;
	
	switch(mode)
	{
		case AT_COLOR:
			delete colour . initial;
		break;

		case AT_FILE:
		case AT_FILES:
			delete file . prompt;
			delete file . initial;
			delete file . filter;
			for(uint4 t_type = 0; t_type < file . type_count; ++t_type)
				delete file . types[t_type];
			delete[] file . types; /* Allocated with new[] */
		break;
		
		case AT_FOLDER:
		case AT_FOLDERS:
			delete folder . prompt;
			delete folder . initial;
		break;
		
		default:
			delete notify . prompt;
			for(uint4 t_button = 0; t_button < notify . button_count; ++t_button)
				delete notify . buttons[t_button];
			delete[] notify . buttons; /* Allocated with new[] */
		break;
	}
}

Parse_stat MCAnswer::parse(MCScriptPoint &sp)
{
	Parse_errors t_error = PE_UNDEFINED;
	
	Symbol_type t_type;
	const LT *t_literal;

	initpoint(sp);

	if (sp . skip_token(SP_ASK, TT_UNDEFINED, AT_PAGE) == PS_NORMAL)
	{
		if (sp . skip_token(SP_ASK, TT_UNDEFINED, AT_SETUP) == PS_NORMAL)
			mode = AT_PAGESETUP;
		else
			t_error = PE_ANSWER_BADQUESTION;

	}
	else if (sp . next(t_type) == PS_NORMAL)
	{
		if (sp . lookup(SP_ASK, t_literal) == PS_NORMAL)
			mode = (Ask_type)t_literal -> which;
		else
			sp . backup();
	}
			
	if (t_error == PE_UNDEFINED)
		switch(mode)
		{
			case AT_PAGESETUP:
				// MJ: adding support for "answer pagesetup" syntax, following existing code
				t_error = parse_pagesetup(sp);
			break;
			
			case AT_PRINTER:
				t_error = parse_printer(sp);
			break;
			
			case AT_EFFECT:
				t_error = parse_effect(sp);
			break;
			
			case AT_RECORD:
				t_error = parse_record(sp);
			break;
			
			case AT_COLOR:
				t_error = parse_colour(sp);
			break;
			
			case AT_FILE:
			case AT_FILES:
				t_error = parse_file(sp);
			break;

			case AT_FOLDER:
			case AT_FOLDERS:
				t_error = parse_folder(sp);
			break;
			
			default:
				t_error = parse_notify(sp);
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

Parse_errors MCAnswer::parse_pagesetup(MCScriptPoint& sp)
{
	return PE_UNDEFINED;
}

Parse_errors MCAnswer::parse_printer(MCScriptPoint& sp)
{
	return PE_UNDEFINED;
}

Parse_errors MCAnswer::parse_effect(MCScriptPoint& sp)
{
	return PE_UNDEFINED;
}

Parse_errors MCAnswer::parse_record(MCScriptPoint& sp)
{
	return PE_UNDEFINED;
}

Parse_errors MCAnswer::parse_colour(MCScriptPoint& sp)
{
	Parse_errors t_error = PE_UNDEFINED;

	if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
		if (sp . parseexp(False, True, &colour . initial) != PS_NORMAL)
			t_error = PE_ANSWER_BADRESPONSE;
	
	return t_error;
}

Parse_errors MCAnswer::parse_file(MCScriptPoint& sp)
{
	Parse_errors t_error = PE_UNDEFINED;

	file . prompt = NULL;
	file . initial = NULL;
	file . filter = NULL;
	file . type_count = 0;
	file . types = NULL;

	if (sp . parseexp(False, True, &file . prompt) != PS_NORMAL)
		t_error = PE_ANSWER_BADRESPONSE;
	
	if (t_error == PE_UNDEFINED && sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		if (sp . skip_token(SP_COMMAND, TT_STATEMENT, S_FILTER) == PS_NORMAL || sp . skip_token(SP_COMMAND, TT_STATEMENT, S_TYPE) == PS_NORMAL)
			sp . backup(), sp . backup();
		else if (sp . parseexp(False, True, &file . initial) != PS_NORMAL)
			t_error = PE_ANSWER_BADRESPONSE;
	}
		
	if (t_error == PE_UNDEFINED && sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		if (sp . skip_token(SP_COMMAND, TT_STATEMENT, S_FILTER) == PS_NORMAL)
		{
			if (sp . parseexp(False, True, &file . filter) != PS_NORMAL)
				t_error = PE_ANSWER_BADRESPONSE;
		}
		else if (sp . skip_token(SP_COMMAND, TT_STATEMENT, S_TYPE) == PS_NORMAL)
		{
			do
			{
				MCU_realloc((char **)&file . types, file . type_count, file . type_count + 1, sizeof(MCExpression *));
				if (sp . parseexp(True, True, &file . types[file . type_count]) != PS_NORMAL)
					t_error = PE_ANSWER_BADRESPONSE;
				else
					file . type_count += 1;
			}
			while(!t_error && sp . skip_token(SP_FACTOR, TT_BINOP, O_OR) == PS_NORMAL &&
						(sp . skip_token(SP_COMMAND, TT_STATEMENT, S_TYPE) == PS_NORMAL || sp . skip_token(SP_ASK, TT_UNDEFINED, AT_TYPES) == PS_NORMAL));
		}
		else
			t_error = PE_ANSWER_BADRESPONSE;
	}
	else if (t_error == PE_UNDEFINED && sp . skip_token(SP_FACTOR, TT_OF) == PS_NORMAL)
	{
		if (sp . skip_token(SP_COMMAND, TT_STATEMENT, S_TYPE) != PS_NORMAL ||
				sp . parseexp(False, True, &file . filter) != PS_NORMAL)
			t_error = PE_ANSWER_BADRESPONSE;
	}
		
	return t_error;
}

Parse_errors MCAnswer::parse_folder(MCScriptPoint& sp)
{
	Parse_errors t_error = PE_UNDEFINED;
		
	if (sp . parseexp(False, True, &folder . prompt) != PS_NORMAL)
		t_error = PE_ANSWER_BADRESPONSE;
	
	if (t_error == PE_UNDEFINED && sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
		if (sp . parseexp(False, True, &folder . initial) != PS_NORMAL)
			t_error = PE_ANSWER_BADRESPONSE;

	return t_error;
}

// MW-2005-06-04: Relaxed syntax slightly - allowed 'and' and 'or' for connectors
Parse_errors MCAnswer::parse_notify(MCScriptPoint& sp)
{
	Parse_errors t_error = PE_UNDEFINED;
	
	if (sp . parseexp(False, True, &notify . prompt) != PS_NORMAL)
		t_error = PE_ANSWER_BADRESPONSE;
	
	if (t_error == PE_UNDEFINED && sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		notify . buttons = NULL;
		notify . button_count = 0;

		do
		{
			MCU_realloc((char **)&notify . buttons, notify . button_count, notify . button_count + 1, sizeof(MCExpression *));
			if (sp . parseexp(True, True, &notify . buttons[notify . button_count]) != PS_NORMAL)
				t_error = PE_ANSWER_BADRESPONSE;
			else
				notify . button_count += 1;
		}
		while(t_error == PE_UNDEFINED && (sp . skip_token(SP_FACTOR, TT_BINOP, O_OR) == PS_NORMAL || sp . skip_token(SP_FACTOR, TT_BINOP, O_AND) == PS_NORMAL));
	}
	
	return t_error;
}

void MCAnswer::exec_ctxt(MCExecContext& ctxt)
{
	MCAutoStringRef t_title;
    if (!ctxt . EvalOptionalExprAsStringRef(title, kMCEmptyString, EE_ANSWER_BADTITLE, &t_title))
        return;
        
    switch(mode)
	{
        case AT_PAGESETUP:
            MCPrintingExecAnswerPageSetup(ctxt, sheet == True);
            break;
            
        case AT_PRINTER:
            MCPrintingExecAnswerPrinter(ctxt, sheet == True);
            break;
            
        case AT_EFFECT:
            MCMultimediaExecAnswerEffect(ctxt);
            break;
            
        case AT_RECORD:
            MCMultimediaExecAnswerRecord(ctxt);
            break;
            
        case AT_COLOR:
		{
			MCColor t_initial_color;
			MCColor *t_initial_color_ptr = &t_initial_color;
			
            // AL-2014-04-01: [[ Bug 12071 ]] If the intial color is empty (i.e. unset), pass nil ptr to dialog.
            ctxt . TryToEvalOptionalExprAsColor(colour . initial, nil, EE_ANSWER_BADQUESTION, t_initial_color_ptr);
            
			MCDialogExecAnswerColor(ctxt, t_initial_color_ptr, *t_title, sheet == True);
			break;
		}
            
        case AT_FILE:
        case AT_FILES:
		{
			MCAutoStringRef t_prompt, t_initial, t_filter;
			MCAutoStringRefArray t_types;
            if (!ctxt . EvalOptionalExprAsStringRef(file.prompt, kMCEmptyString, EE_ANSWER_BADQUESTION, &t_prompt))
                return;
			if (!ctxt . EvalOptionalExprAsNullableStringRef(file.initial, EE_ANSWER_BADQUESTION, &t_initial))
                return;
			if (!ctxt . EvalOptionalExprAsStringRef(file.filter, kMCEmptyString, EE_ANSWER_BADQUESTION, &t_filter))
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
            
            /* UNCHECKED */ t_types.Extend(file.type_count);
			for (uindex_t i = 0; i < file.type_count; i++)
			{
                if (!ctxt . EvalOptionalExprAsNullableStringRef(file.types[i], EE_ANSWER_BADQUESTION, t_types[i]))
                    return;
			}
            
			if (t_types.Count() > 0)
				MCDialogExecAnswerFileWithTypes(ctxt, mode == AT_FILES, *t_prompt, *t_initial_resolved, *t_types, t_types.Count(), *t_title, sheet == True);
			else if (*t_filter != nil)
				MCDialogExecAnswerFileWithFilter(ctxt, mode == AT_FILES, *t_prompt, *t_initial_resolved, *t_filter, *t_title, sheet == True);
			else
				MCDialogExecAnswerFile(ctxt, mode == AT_FILES, *t_prompt, *t_initial_resolved, *t_title, sheet == True);
            
			break;
		}

        case AT_FOLDER:
        case AT_FOLDERS:
		{
			MCAutoStringRef t_prompt, t_initial;
            if (!ctxt . EvalOptionalExprAsNullableStringRef(file.prompt, EE_ANSWER_BADQUESTION, &t_prompt))
                return;
			if (!ctxt . EvalOptionalExprAsNullableStringRef(file.initial, EE_ANSWER_BADQUESTION, &t_initial))
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
            
			MCDialogExecAnswerFolder(ctxt, mode == AT_FOLDERS, *t_prompt, *t_initial_resolved, *t_title, sheet == True);
            
			break;
		}
            
        default:
		{
			MCAutoStringRef t_prompt;
			MCAutoStringRefArray t_buttons;
            
            if (!ctxt . EvalOptionalExprAsNullableStringRef(notify.prompt, EE_ANSWER_BADRESPONSE, &t_prompt))
                return;
			            
			/* UNCHECKED */ t_buttons.Extend(notify.button_count);
			for (uindex_t i = 0; i < notify.button_count; i++)
			{
                if (!ctxt . EvalOptionalExprAsNullableStringRef(notify.buttons[i], EE_ANSWER_BADRESPONSE, t_buttons[i]))
                    return;
			}
            
			MCDialogExecAnswerNotify(ctxt, mode, *t_prompt, *t_buttons, t_buttons.Count(), *t_title, sheet == True);
		}
	}
}

