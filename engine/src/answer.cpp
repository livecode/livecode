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

#include "globals.h"
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

#include "meta.h"
#include "answer.h"
#include "printer.h"

#include "exec.h"
#include "syntax.h"

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
			delete file . types;
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
			delete notify . buttons;
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
			if (!ctxt . EvalOptionalExprAsStringRef(file.initial, kMCEmptyString, EE_ANSWER_BADQUESTION, &t_initial))
                return;
			if (!ctxt . EvalOptionalExprAsStringRef(file.filter, kMCEmptyString, EE_ANSWER_BADQUESTION, &t_filter))
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
                // IM-2014-08-06: [[ Bug 13096 ]] Allow file dialogs to work with relative paths by resolving to absolute
                if (!MCS_resolvepath(*t_initial, &t_initial_resolved))
                {
                    ctxt . LegacyThrow(EE_NO_MEMORY);
                    return;
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

#ifdef LEGACY_EXEC
Exec_stat MCAnswer::exec(MCExecPoint& ep)
{
	Exec_errors t_error = EE_UNDEFINED;
	Meta::cstring_value t_title;

	t_error = Meta::evaluate(ep, title, t_title, EE_ANSWER_BADTITLE);
	if (!t_error)
		switch(mode)
		{
		case AT_PAGESETUP:
			t_error = exec_pagesetup(ep, t_title);
		break;
		
		case AT_PRINTER:
			t_error = exec_printer(ep, t_title);
		break;

		case AT_EFFECT:
			t_error = exec_effect(ep, t_title);
		break;

		case AT_RECORD:
			t_error = exec_record(ep, t_title);
		break;

		case AT_COLOR:
			t_error = exec_colour(ep, t_title);
		break;

		case AT_FILE:
		case AT_FILES:
			t_error = exec_file(ep, t_title);
		break;

		case AT_FOLDER:
		case AT_FOLDERS:
			t_error = exec_folder(ep, t_title);
		break;

		default:
			t_error = exec_notify(ep, t_title);
		break;
		}

	if (!t_error)
		ep . getit() -> set(ep);
	else
		MCeerror -> add(t_error, line, pos);

	return t_error ? ES_ERROR : ES_NORMAL;
}
#endif

#ifdef LEGACY_EXEC
Exec_errors MCAnswer::exec_pagesetup(MCExecPoint& ep, const char *p_title)
{
	Exec_errors t_error;
	t_error = EE_UNDEFINED;

	MCresult -> clear(False);

	if (!MCSecureModeCanAccessPrinter())
		return EE_PRINT_NOPERM;

	if (MCsystemPS && MCscreen -> hasfeature(PLATFORM_FEATURE_OS_PRINT_DIALOGS))
		MCresult -> sets(MCprinter -> ChoosePaper(sheet == True));
	else
	{
		t_error = exec_custom(ep, "Page Setup Dialog", "pagesetup", 0);
		
		if (ep . getsvalue() != MCnullmcstring)
			MCresult -> sets(MCcancelstring);
	}

	return t_error;
}
#endif

#ifdef LEGACY_EXEC
Exec_errors MCAnswer::exec_printer(MCExecPoint& ep, const char *p_title)
{
	Exec_errors t_error;
	t_error = EE_UNDEFINED;

	MCresult -> clear(False);
	
	if (!MCSecureModeCanAccessPrinter())
		return EE_PRINT_NOPERM;

	if (MCsystemPS && MCscreen -> hasfeature(PLATFORM_FEATURE_OS_PRINT_DIALOGS))
		MCresult -> sets(MCprinter -> ChoosePrinter(sheet == True));
	else
	{
		t_error = exec_custom(ep, "Print Dialog", "printer", 0);
		
		if (ep . getsvalue() != MCnullmcstring)
			MCresult -> sets(MCcancelstring);
	}

	return t_error;
}
#endif

#ifdef LEGACY_EXEC
Exec_errors MCAnswer::exec_effect(MCExecPoint& ep, const char *p_title)
{
	MCresult -> clear(False);
	extern Boolean MCQTEffectsDialog(MCExecPoint &ep, const char *p_title, Boolean sheet);
	MCQTEffectsDialog(ep, p_title, sheet);
	return EE_UNDEFINED;
}
#endif

#ifdef LEGACY_EXEC
Exec_errors MCAnswer::exec_record(MCExecPoint& ep, const char *p_title)
{
	MCresult -> clear(False);
    
#ifdef FEATURE_PLATFORM_RECORDER

    extern MCPlatformSoundRecorderRef MCrecorder;
    if (MCrecorder == nil)
        MCPlatformSoundRecorderCreate(MCrecorder);
    
    if (MCrecorder != nil)
    {
        MCPlatformSoundRecorderBeginConfigurationDialog(MCrecorder);
        
        MCPlatformDialogResult t_result;
        
        for (;;)
        {
            t_result = MCPlatformSoundRecorderEndConfigurationDialog(MCrecorder);
            if (t_result != kMCPlatformDialogResultContinue)
                break;
            
            MCscreen -> wait(REFRESH_INTERVAL, True, True);
        }
        
        ep.clear();
        
        if (t_result == kMCPlatformDialogResultCancel)
            MCresult->sets(MCcancelstring);
    }    
#else
	extern void MCQTRecordDialog(MCExecPoint& ep, const char *p_title, Boolean sheet);
	MCQTRecordDialog(ep, p_title, sheet);
#endif
	return EE_UNDEFINED;
}
#endif

#ifdef LEGACY_EXEC
Exec_errors MCAnswer::exec_colour(MCExecPoint& ep, const char *p_title)
{
	Exec_errors t_error = EE_UNDEFINED;

	Meta::cstring_value t_initial;

	t_error = Meta::evaluate(ep, colour . initial, t_initial, EE_ANSWER_BADQUESTION);
	if (!t_error)
	{
		MCresult -> clear(False);

		if (MCsystemCS && MCscreen -> hasfeature ( PLATFORM_FEATURE_OS_COLOR_DIALOGS ) )
			MCA_color(ep, p_title, t_initial, sheet);
		else
			t_error = exec_custom(ep, MCcsnamestring, "color", 2, p_title, *t_initial);
		
		if (ep . getsvalue() == MCnullmcstring)
			MCresult -> sets(MCcancelstring);
	}

	return t_error;
}
#endif

#ifdef LEGACY_EXEC
Exec_errors MCAnswer::exec_file(MCExecPoint& ep, const char *p_title)
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
	
	MCresult -> clear(False);

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

	// Now we have checked all parameters, we check for access.
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
		if (MCsystemFS && MCscreen -> hasfeature ( PLATFORM_FEATURE_OS_FILE_DIALOGS ) )
		{
			unsigned int t_options = 0;
			if (sheet)
				t_options |= MCA_OPTION_SHEET;
			if (mode == AT_FILES)
				t_options |= MCA_OPTION_PLURAL;

			if (t_types != NULL)
				MCA_file_with_types(ep, p_title, t_prompt, t_type_strings, t_type_count, t_initial_resolved, t_options);
			else
				MCA_file(ep, p_title, t_prompt, t_filter, t_initial_resolved, t_options);
		}
		else
		{
			MCExecPoint ep2(ep);
			ep2 . clear();
			for(uint4 t_type = 0; t_type < t_type_count; ++t_type)
				ep2 . concatcstring(t_type_strings[t_type], EC_RETURN, t_type == 0);
			t_error = exec_custom(ep, MCfsnamestring, mode == AT_FILE ? "file" : "files", 5, p_title, *t_prompt, *t_filter, t_initial_resolved, ep2 . getsvalue() . getstring());
		}
		
		if (ep . getsvalue() == MCnullmcstring && t_types == NULL)
			MCresult -> sets(MCcancelstring);
	}

	if (t_initial_resolved != nil)
		MCCStringFree(t_initial_resolved);

	delete[] t_types;
	delete t_type_strings;

	return t_error;
}
#endif

#ifdef LEGACY_EXEC
Exec_errors MCAnswer::exec_folder(MCExecPoint& ep, const char *p_title)
{
	Exec_errors t_error = EE_UNDEFINED;

	Meta::cstring_value t_prompt, t_initial;

	t_error = Meta::evaluate(ep,
											folder . prompt, t_prompt, EE_ANSWER_BADQUESTION,
											folder . initial, t_initial, EE_ANSWER_BADRESPONSE);

	MCresult -> clear(False);

	// Now we have checked all parameters, we check for access.
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
		unsigned int t_options = 0;
		if (sheet)
			t_options |= MCA_OPTION_SHEET;
		if (mode == AT_FOLDERS)
			t_options |= MCA_OPTION_PLURAL;

		if (MCsystemFS)
			MCA_folder(ep, p_title, t_prompt, t_initial_resolved, t_options);
		else
			t_error = exec_custom(ep, MCfsnamestring, mode == AT_FOLDER ? "folder" : "folders", 4, p_title, *t_prompt, NULL, t_initial_resolved);

		if (ep . getsvalue() == MCnullmcstring)
			MCresult -> sets(MCcancelstring);
	}

	if (t_initial_resolved != nil)
		MCCStringFree(t_initial_resolved);

	return t_error;
}
#endif

#ifdef LEGACY_EXEC
Exec_errors MCAnswer::exec_notify(MCExecPoint& ep, const char *p_title)
{
	Exec_errors t_error = EE_UNDEFINED;

	Meta::cstring_value t_prompt, t_buttons;
	
	t_error = Meta::evaluate(ep, notify . prompt, t_prompt, EE_ANSWER_BADRESPONSE);

	MCresult -> clear(False);

#ifndef _MOBILE
	
	if (!t_error)
	{
		MCExecPoint ep2(ep);
		ep . clear();
		for(unsigned int t_button = 0; t_button < notify . button_count && !t_error; ++t_button)
		{
			if (notify . buttons[t_button] -> eval(ep2) == ES_NORMAL)
				ep . concatmcstring(ep2 . getsvalue(), EC_RETURN, t_button == 0);
			else
				t_error = EE_ANSWER_BADRESPONSE;
		}

		t_buttons = ep;
	}

	if (!t_error)
		t_error = exec_custom(ep, MCanswernamestring, MCdialogtypes[mode], 3, p_title, *t_prompt, *t_buttons);
	
	if (ep . getsvalue() == MCnullmcstring)
		MCresult -> sets(MCcancelstring);
	
#else
	char **t_button_names;
	t_button_names = new char *[notify . button_count];
	memset(t_button_names, 0, sizeof(char *) * notify . button_count);
	if (!t_error)
	{
		MCExecPoint ep2(ep);
		ep . clear();
		for(unsigned int t_button = 0; t_button < notify . button_count && !t_error; ++t_button)
			if (notify . buttons[t_button] -> eval(ep2) == ES_NORMAL)
				t_button_names[t_button] = ep2 . getsvalue() . clone();
	}

	if (!t_error)
	{
		uint32_t t_type;
		switch(mode)
		{
		case AT_ERROR: t_type = kMCAnswerDialogTypeError; break;
		case AT_QUESTION: t_type = kMCAnswerDialogTypeQuestion; break;
		case AT_WARNING: t_type = kMCAnswerDialogTypeWarning; break;
		default: t_type = kMCAnswerDialogTypeInformation; break;
		}

		int32_t t_result;
		t_result = MCscreen -> popupanswerdialog((const char **)t_button_names, notify . button_count, t_type, p_title, *t_prompt);
		
		ep . clear();
		
		if (t_result == -1)
			MCresult -> sets(MCcancelstring);
		else if (notify . button_count == 0)
			ep . clear();
		else
			ep . copysvalue(t_button_names[t_result]);
	}

	for(uint32_t i = 0; i < notify . button_count; i++)
		delete t_button_names[i];
	delete[] t_button_names;
	
#endif

	return t_error;
}
#endif

#ifdef LEGACY_EXEC
Exec_errors MCAnswer::exec_custom(MCExecPoint& ep, const MCString& p_stack, const char *p_type, unsigned int p_count, ...)
{
	ep . setstringf("answer %s", p_type);
	
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
		if (MCdefaultstackptr -> getopened() || MCtopstackptr == NULL)
			t_stack -> openrect(MCdefaultstackptr -> getrect(), sheet ? WM_SHEET : WM_MODAL, sheet ? MCdefaultstackptr: NULL, WP_DEFAULT, OP_NONE);
		else
			t_stack -> openrect(MCtopstackptr -> getrect(), sheet ? WM_SHEET : WM_MODAL, sheet ? MCtopstackptr : NULL, WP_DEFAULT, OP_NONE);
	}
	MCtrace = t_old_trace;

	MCdialogdata -> fetch(ep);

	return EE_UNDEFINED;
}
#endif

void MCAnswer::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	switch(mode)
	{
	case AT_PAGESETUP:
	case AT_PRINTER:
		if (title != nil)
			title -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryEvalConstantBool(ctxt, sheet == True);

		if (mode == AT_PAGESETUP)
			MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCPrintingExecAnswerPageSetupMethodInfo, 1);
		else
			MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCPrintingExecAnswerPrinterMethodInfo, 1);
		break;
	
	case AT_EFFECT:
		if (title != nil)
			title -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryExecMethod(ctxt, kMCMultimediaExecAnswerEffectMethodInfo);
		break;

	case AT_RECORD:
		if (title != nil)
			title -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryExecMethod(ctxt, kMCMultimediaExecAnswerRecordMethodInfo);
		break;

	case AT_COLOR:
		if (colour . initial != nil)
			colour . initial -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		if (title != nil)
			title -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryEvalConstantBool(ctxt, sheet == True);

		MCSyntaxFactoryExecMethod(ctxt, kMCDialogExecAnswerColorMethodInfo);
		break;

	case AT_FILE:
	case AT_FILES:
		MCSyntaxFactoryEvalConstantBool(ctxt, mode == AT_FILES);

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
			MCSyntaxFactoryExecMethod(ctxt, kMCDialogExecAnswerFileWithTypesMethodInfo);
		else if (file . filter != nil)
			MCSyntaxFactoryExecMethod(ctxt, kMCDialogExecAnswerFileWithFilterMethodInfo);
		else
			MCSyntaxFactoryExecMethod(ctxt, kMCDialogExecAnswerFileMethodInfo);
		break;

	case AT_FOLDER:
	case AT_FOLDERS:
		MCSyntaxFactoryEvalConstantBool(ctxt, mode == AT_FOLDERS);

		if (file . prompt != nil)
			file . prompt -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		if (file . initial != nil)
			file . initial -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		if (title != nil)
			title -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryEvalConstantBool(ctxt, sheet == True);
		
		MCSyntaxFactoryExecMethod(ctxt, kMCDialogExecAnswerFolderMethodInfo);
		break;

	default:	
		MCSyntaxFactoryEvalConstantInt(ctxt, mode);

		if (notify . prompt != nil)
			notify . prompt -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		if (notify . button_count > 0)
		{
			for (uindex_t i = 0; i < notify . button_count; i++)
				notify . buttons[i] -> compile(ctxt);

			MCSyntaxFactoryEvalList(ctxt, notify . button_count);
		}

		if (title != nil)
			title -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryEvalConstantBool(ctxt, sheet == True);
		
		MCSyntaxFactoryExecMethod(ctxt, kMCDialogExecAnswerNotifyMethodInfo);
		break;
	}

	MCSyntaxFactoryEndStatement(ctxt);
}
