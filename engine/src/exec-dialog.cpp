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
#include "mcio.h"

#include "globals.h"
#include "field.h"
#include "stack.h"
#include "card.h"
#include "variable.h"

#include "exec.h"
#include "debug.h"

#include "ans.h"
#include "securemode.h"

////////////////////////////////////////////////////////////////////////////////

static MCNameRef *s_dialog_types[] = 
{
	&MCN_plain,
	&MCN_clear,
	&MCN_color,
	&MCN_effect,
	&MCN_error,
	&MCN_file,
	&MCN_folder,
	&MCN_information,
	&MCN_password,
	&MCN_printer,
	&MCN_program,
	&MCN_question,
	&MCN_record,
	&MCN_titled,
	&MCN_warning,
};

////////////////////////////////////////////////////////////////////////////////

void MCDialogExecAnswerColor(MCExecContext &ctxt, MCColor *p_initial_color, MCStringRef p_title, bool p_as_sheet)
{
    MCAutoStringRef t_value;
	bool t_chosen = false;
	if (MCsystemCS && MCscreen->hasfeature(PLATFORM_FEATURE_OS_COLOR_DIALOGS))
	{
		MCColor t_color;
		if (p_initial_color != nil)
			t_color = *p_initial_color;
		else
			t_color = MCpencolor;
		
		MCColor t_chosen_color;
		if (!MCA_color(p_title, t_color, p_as_sheet, t_chosen, t_chosen_color))
		{
			ctxt.Throw();
			return;
		}

		if (t_chosen)
		{
			if (!MCStringFormat(&t_value, "%d,%d,%d",
                t_chosen_color.red >> 8, t_chosen_color.green >> 8, t_chosen_color.blue >> 8))
			{
				ctxt.Throw();
				return;
			}
		}
	}
	else
	{
		MCAutoStringRef t_initial_color_as_string;
		if (p_initial_color != nil)
		{
			if (!MCStringFormat(&t_initial_color_as_string, "%d,%d,%d", p_initial_color -> red >> 8, p_initial_color -> green >> 8, p_initial_color -> blue >> 8))
			{
				ctxt . Throw();
				return;
			}
		}
		else
			t_initial_color_as_string = kMCEmptyString;
		
		MCStringRef t_args[2];
		t_args[0] = p_title;
		t_args[1] = *t_initial_color_as_string;
		MCDialogExecCustomAnswerDialog(ctxt, MCN_color_chooser, MCN_color, p_as_sheet, t_args, 2, &t_value);
		if (ctxt.HasError())
			return;

		if (MCStringGetLength(*t_value) == 0)
			t_chosen = false;
	}
    
    // SN-2014-08-11: [[ Bug 13144 ]] it should be set to empty if nothing has been made
	if (t_chosen)
    {
        ctxt.SetItToValue(*t_value);
		ctxt.SetTheResultToEmpty();
    }
	else
    {
        ctxt.SetItToEmpty();
		ctxt.SetTheResultToValue(MCN_cancel);
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCDialogExecAnswerFile(MCExecContext &ctxt, bool p_plural, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef p_title, bool p_sheet)
{
	MCDialogExecAnswerFileWithFilter(ctxt, p_plural, p_prompt, p_initial, nil, p_title, p_sheet);
}

void MCDialogExecAnswerFileWithFilter(MCExecContext &ctxt, bool p_plural, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef p_filter, MCStringRef p_title, bool p_sheet)
{
	MCAutoStringRef t_value, t_result;

	if (!MCSecureModeCanAccessDisk())
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}

	if (MCsystemFS && MCscreen->hasfeature(PLATFORM_FEATURE_OS_FILE_DIALOGS))
	{
        uint32_t t_options = 0;
        if (p_plural)
            t_options |= MCA_OPTION_PLURAL;
        if (p_sheet)
            t_options |= MCA_OPTION_SHEET;

        MCA_file(p_title, p_prompt, p_filter, p_initial, t_options, &t_value, &t_result);
	}
	else
	{
		MCStringRef t_args[5];
		uindex_t t_arg_count = 5;
		t_args[0] = p_title;
		t_args[1] = p_prompt;
		t_args[2] = p_filter;
		t_args[3] = p_initial;
		t_args[4] = nil;
		MCDialogExecCustomAnswerDialog(ctxt, MCN_file_selector, p_plural ? MCN_files : MCN_file, p_sheet, t_args, t_arg_count, &t_value);
		if (ctxt.HasError())
			return;

		if (MCStringGetLength(*t_value) == 0)
		{
			if (!MCStringCopy(MCNameGetString(MCN_cancel), &t_result))
			{
				ctxt.Throw();
				return;
			}
		}
	}

	if (*t_value != nil)
    {
		ctxt.SetItToValue(*t_value);
        if (*t_result != nil)
            ctxt.SetTheResultToValue(*t_result);
        else
            ctxt.SetTheResultToEmpty();
    }
	else
    {
        // SN-2014-08-11: [[ Bug 13144 ]] it should be set to empty if nothing has been made
        ctxt.SetItToEmpty();
		ctxt.SetTheResultToValue(MCN_cancel);
    }
}

bool MCStringsSplit(MCStringRef p_string, codepoint_t p_separator, MCStringRef*&r_strings, uindex_t& r_count);
void MCDialogExecAnswerFileWithTypes(MCExecContext &ctxt, bool p_plural, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef *p_types, uindex_t p_type_count, MCStringRef p_title, bool p_sheet)
{
	MCAutoStringRef t_value, t_result;
	bool t_success = true;

	MCAutoStringRefArray t_types;
	for (uindex_t i = 0; t_success && i < p_type_count; i++)
	{
		MCAutoStringRefArray t_split;
		t_success = MCStringsSplit(p_types[i], '\n', t_split.PtrRef(), t_split.CountRef());
		if (t_success)
			t_success = t_types.Append(t_split);
	}
	if (t_success)
	{
		if (MCsystemFS && MCscreen->hasfeature(PLATFORM_FEATURE_OS_FILE_DIALOGS))
		{
			uint32_t t_options = 0;
			if (p_plural)
				t_options |= MCA_OPTION_PLURAL;
			if (p_sheet)
				t_options |= MCA_OPTION_SHEET;

			int error;
			error = MCA_file_with_types(p_title, p_prompt, *t_types, t_types.Count(), p_initial, t_options, &t_value, &t_result);
		}
		else
		{
			MCAutoListRef t_type_list;
			MCAutoStringRef t_types_string;
			t_success = MCListCreateMutable('\n', &t_type_list);
			
			for (uindex_t i = 0; t_success && i < t_types.Count(); i++)
				t_success = MCListAppend(*t_type_list, t_types[i]);
			
			if (t_success)
				t_success = MCListCopyAsString(*t_type_list, &t_types_string);

			if (t_success)
			{
				MCStringRef t_args[5];
				uindex_t t_arg_count = 5;
				t_args[0] = p_title;
				t_args[1] = p_prompt;
				t_args[2] = nil;
				t_args[3] = p_initial;
				t_args[4] = *t_types_string;
				MCDialogExecCustomAnswerDialog(ctxt, MCN_file_selector, p_plural ? MCN_files : MCN_file, p_sheet, t_args, t_arg_count, &t_value);
				if (ctxt.HasError())
					return;
			}

			if (t_success && MCStringGetLength(*t_value) == 0)
				t_success = MCStringCopy(MCNameGetString(MCN_cancel), &t_result);
		}
	}
	
	if (!t_success)
	{
		ctxt.Throw();
		return;
	}
	
	if (*t_value != nil)
	{
		ctxt.SetItToValue(*t_value);

		if (*t_result != nil)
			ctxt.SetTheResultToValue(*t_result);
        else
            ctxt.SetTheResultToEmpty();
	}
	else
    {
        // SN-2014-08-11: [[ Bug 13144 ]] it should be set to empty if nothing has been made
        ctxt.SetItToEmpty();
		ctxt.SetTheResultToValue(MCN_cancel);
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCDialogExecAnswerFolder(MCExecContext &ctxt, bool p_plural, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef p_title, bool p_sheet)
{
	MCAutoStringRef t_value, t_result;

	if (!MCSecureModeCanAccessDisk())
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}

	if (MCsystemFS && MCscreen->hasfeature(PLATFORM_FEATURE_OS_FILE_DIALOGS))
	{
        uint32_t t_options = 0;
        if (p_plural)
            t_options |= MCA_OPTION_PLURAL;
        if (p_sheet)
            t_options |= MCA_OPTION_SHEET;

        int t_error;
        t_error = MCA_folder(p_title, p_prompt, p_initial, t_options, &t_value, &t_result);
	}
	else
	{
		MCStringRef t_args[4];
		uindex_t t_arg_count = 4;
		t_args[0] = p_title;
		t_args[1] = p_prompt;
		t_args[2] = nil;
		t_args[3] = p_initial;
		MCDialogExecCustomAnswerDialog(ctxt, MCN_file_selector, p_plural ? MCN_folders : MCN_folder, p_sheet, t_args, t_arg_count, &t_value);
		if (ctxt.HasError())
			return;

		if (MCStringGetLength(*t_value) == 0)
		{
			if (!MCStringCopy(MCNameGetString(MCN_cancel), &t_result))
			{
				ctxt.Throw();
				return;
			}
		}
	}

	if (*t_value != nil)
	{
		ctxt.SetItToValue(*t_value);

		if (*t_result != nil)
			ctxt.SetTheResultToValue(*t_result);
        else
            ctxt.SetTheResultToEmpty();
	}
	else
    {
        // SN-2014-08-11: [[ Bug 13144 ]] it should be set to empty if nothing has been made
        ctxt.SetItToEmpty();
		ctxt.SetTheResultToValue(MCN_cancel);
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCDialogExecAnswerNotify(MCExecContext &ctxt, integer_t p_type, MCStringRef p_prompt, MCStringRef *p_buttons, uindex_t p_button_count, MCStringRef p_title, bool p_sheet)
{
#if !defined(_MOBILE) && !defined(__EMSCRIPTEN__)
	MCAutoStringRef t_value, t_result;
	MCAutoListRef t_button_list;
	MCAutoStringRef t_buttons_string;

	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = MCListCreateMutable('\n', &t_button_list);
	for (uindex_t i = 0; t_success && i < p_button_count; i++)
		t_success = MCListAppend(*t_button_list, p_buttons[i]);
	if (t_success)
		t_success = MCListCopyAsString(*t_button_list, &t_buttons_string);

	if (!t_success)
	{
		ctxt.Throw();
		return;
	}
	
	MCStringRef t_args[4];
	t_args[0] = p_title;
	t_args[1] = p_prompt;
	t_args[2] = *t_buttons_string;

    // AL-2014-05-21: [[ Bug 12074 ]] Pass through directionality of prompt to
    //  dialogData for appropriate dialog layout.
    if (!MCStringResolvesLeftToRight(p_prompt))
        t_args[3] = kMCTrueString;
    else
        t_args[3] = kMCFalseString;
    
    
	MCDialogExecCustomAnswerDialog(ctxt, MCN_answer_dialog, *s_dialog_types[p_type], p_sheet, t_args, 4, &t_value);

	if (ctxt.HasError())
		return;

	if (*t_value != nil && MCStringGetLength(*t_value) != 0)
	{
		ctxt.SetItToValue(*t_value);
		ctxt.SetTheResultToEmpty();
	}
	else
    {
        // SN-2014-08-11: [[ Bug 13144 ]] it should be set to empty if nothing has been made
        ctxt.SetItToEmpty();
		ctxt.SetTheResultToValue(MCN_cancel);
    }
#else
	uint32_t t_type;
	switch(p_type)
	{
	case AT_ERROR: t_type = kMCAnswerDialogTypeError; break;
	case AT_QUESTION: t_type = kMCAnswerDialogTypeQuestion; break;
	case AT_WARNING: t_type = kMCAnswerDialogTypeWarning; break;
	default: t_type = kMCAnswerDialogTypeInformation; break;
	}

	int32_t t_result;
	t_result = MCscreen->popupanswerdialog(p_buttons, p_button_count, t_type, p_title, p_prompt, false);
	if (t_result == -1)
		ctxt.SetTheResultToValue(MCN_cancel);
	else if (p_button_count == 0)
		ctxt.SetItToValue(kMCEmptyString);
	else
		ctxt.SetItToValue(p_buttons[t_result]);
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MCDialogExecCustomAnswerDialog(MCExecContext &ctxt, MCNameRef p_stack, MCNameRef p_type, bool p_sheet, MCStringRef *p_arg_list, uindex_t p_arg_count, MCStringRef &r_result)
{
	bool t_success = true;

	MCAutoStringRef t_arg_string;
	t_success = MCStringCreateMutable(0, &t_arg_string) &&
		MCStringAppendFormat(*t_arg_string, "answer %@", p_type);

	for (uindex_t i = 0; t_success && i < p_arg_count; i++)
		t_success = MCStringAppendNativeChar(*t_arg_string, '\0') &&
			(p_arg_list[i] == nil || MCStringAppend(*t_arg_string, p_arg_list[i]));

	if (t_success)
		t_success = MCdialogdata->setvalueref(*t_arg_string);

	MCStack *t_stack;
	if (t_success)
		t_stack = ctxt.GetObject()->getstack()->findstackname(p_stack);
	
	Boolean t_old_trace = MCtrace;
	MCtrace = False;

	if (t_success && t_stack != nil)
	{
		MCStack *t_parent_stack = nil;

		if (MCdefaultstackptr->getopened() || !MCtopstackptr)
			t_parent_stack = MCdefaultstackptr;
		else
			t_parent_stack = MCtopstackptr;
        
        Boolean added = False;
        if (MCnexecutioncontexts < MAX_CONTEXTS)
        {
            MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
            added = True;
        }
        
		t_success = ES_NORMAL == t_stack->openrect(t_parent_stack->getrect(), p_sheet ? WM_SHEET : WM_MODAL, p_sheet ? t_parent_stack : nil, WP_DEFAULT, OP_NONE);
        
        if (added)
            MCnexecutioncontexts--;
	}

	MCtrace = t_old_trace;

	if (t_success)
		t_success = ctxt.ConvertToString(MCdialogdata->getvalueref(), r_result);

	if (t_success)
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCDialogExecAskQuestion(MCExecContext& ctxt, int p_type, MCStringRef p_prompt, MCStringRef p_answer, bool p_hint_answer, MCStringRef p_title, bool p_as_sheet)
{
#if !defined(_MOBILE) && !defined(__EMSCRIPTEN__)
	MCStringRef t_args[4];
	t_args[0] = p_title;
	t_args[1] = p_prompt;
	t_args[2] = p_answer;
	
    // AL-2014-05-21: [[ Bug 12074 ]] Pass through directionality of prompt to
    //  dialogData for appropriate dialog layout.
    if (!MCStringResolvesLeftToRight(p_prompt))
        t_args[3] = kMCTrueString;
    else
        t_args[3] = kMCFalseString;
    
	bool t_cancelled;
	MCAutoStringRef t_result;
	MCDialogExecCustomAskDialog(ctxt, MCN_ask_dialog, *s_dialog_types[p_type], p_as_sheet, t_args, 4, t_cancelled, &t_result);
	if (ctxt . HasError())
		return;
	
	if (!t_cancelled)
	{
		ctxt . SetItToValue(*t_result);
		ctxt . SetTheResultToEmpty();
	}
	else
	{
		ctxt . SetItToEmpty();
        ctxt . SetTheResultToValue(MCN_cancel);
	}

#else
	MCAutoStringRef t_result;
	if (MCscreen -> popupaskdialog(AT_QUESTION, p_title, p_prompt, p_answer, p_hint_answer, &t_result))
	{
		ctxt . SetItToValue(*t_result);
		ctxt . SetTheResultToEmpty();
	}
	else
	{
		ctxt . SetItToEmpty();
		ctxt . SetTheResultToValue(MCN_cancel);
	}
#endif
}

void MCDialogExecAskPassword(MCExecContext& ctxt, bool p_clear, MCStringRef p_prompt, MCStringRef p_answer, bool p_hint_answer, MCStringRef p_title, bool p_as_sheet)
{
#ifndef _MOBILE
	MCStringRef t_args[3];
	t_args[0] = p_title;
	t_args[1] = p_prompt;
	t_args[2] = p_answer;
	
	bool t_cancelled;
	MCAutoStringRef t_result;
	MCDialogExecCustomAskDialog(ctxt, MCN_ask_dialog, *s_dialog_types[p_clear ? AT_CLEAR : AT_PASSWORD], p_as_sheet, t_args, 3, t_cancelled, &t_result);
	if (ctxt . HasError())
		return;
	
	if (!t_cancelled)
	{
		ctxt . SetItToValue(*t_result);
		ctxt . SetTheResultToEmpty();
	}
	else
	{
		ctxt . SetItToEmpty();
		ctxt . SetTheResultToValue(MCN_cancel);
	}
	
#else
	MCAutoStringRef t_result;
	if (MCscreen -> popupaskdialog(AT_PASSWORD, p_title, p_prompt, p_answer, p_hint_answer, &t_result))
	{
		ctxt . SetItToValue(*t_result);
		ctxt . SetTheResultToEmpty();
	}
	else
	{
		ctxt . SetItToEmpty();
		ctxt . SetTheResultToValue(MCN_cancel);
	}
#endif
}

void MCDialogExecAskFile(MCExecContext& ctxt, MCStringRef prompt, MCStringRef initial, MCStringRef title, bool as_sheet)
{
	MCDialogExecAskFileWithFilter(ctxt, prompt, initial, nil, title, as_sheet);
}

void MCDialogExecAskFileWithFilter(MCExecContext& ctxt, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef p_filter, MCStringRef p_title, bool p_as_sheet)
{
	MCAutoStringRef t_value, t_result;
	if (!MCSecureModeCanAccessDisk())
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}
	
	bool t_cancelled;
	if (MCsystemFS && MCscreen -> hasfeature(PLATFORM_FEATURE_OS_FILE_DIALOGS))
    {
        uint32_t t_options = 0;
        if (p_as_sheet)
            t_options |= MCA_OPTION_SHEET;

        int t_error;
        t_error = MCA_ask_file(p_title, p_prompt, p_filter, p_initial, t_options, &t_value, &t_result);

        t_cancelled = *t_value == nil;
    }
	else
	{
		MCStringRef t_args[5];
		uindex_t t_arg_count = 5;
		t_args[0] = p_title;
		t_args[1] = p_prompt;
		t_args[2] = p_filter;
		t_args[3] = p_initial;
		t_args[4] = nil;
		
		MCDialogExecCustomAskDialog(ctxt, MCN_file_selector, MCN_file, p_as_sheet, t_args, t_arg_count, t_cancelled, &t_value);
		if (ctxt.HasError())
			return;
	}
	
	if (!t_cancelled)
	{
        ctxt . SetItToValue(*t_value);

        if (*t_result != nil)
            ctxt.SetTheResultToValue(*t_result);
        else
            ctxt.SetTheResultToEmpty();
	}
	else
	{
		ctxt . SetItToEmpty();
		ctxt . SetTheResultToValue(MCN_cancel);
	}
}

void MCDialogExecAskFileWithTypes(MCExecContext& ctxt, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef *p_types, uindex_t p_type_count, MCStringRef p_title, bool p_as_sheet)
{
	if (!MCSecureModeCanAccessDisk())
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}
	
	bool t_success = true;
	MCAutoStringRefArray t_types;
	for (uindex_t i = 0; t_success && i < p_type_count; i++)
	{
		MCAutoStringRefArray t_split;
		t_success = MCStringsSplit(p_types[i], '\n', t_split.PtrRef(), t_split.CountRef());
		if (t_success)
			t_success = t_types.Append(t_split);
	}
	
	if (!t_success)
	{
		ctxt.Throw();
		return;
	}
	
	bool t_cancelled;
	MCAutoStringRef t_value, t_result;
	if (MCsystemFS && MCscreen -> hasfeature(PLATFORM_FEATURE_OS_FILE_DIALOGS))
	{
        uint32_t t_options = 0;
        if (p_as_sheet)
            t_options |= MCA_OPTION_SHEET;

        int t_error;
		// t_value contains the filename, t_result the chosen type
        t_error = MCA_ask_file_with_types(p_title, p_prompt, *t_types, t_types.Count(), p_initial, t_options, &t_value, &t_result);

		t_cancelled = *t_value == nil;
	}
	else
	{
		MCAutoListRef t_type_list;
		MCAutoStringRef t_types_string;
		
		t_success = MCListCreateMutable('\n', &t_type_list);
		for (uindex_t i = 0; t_success && i < t_types.Count(); i++)
			t_success = MCListAppend(*t_type_list, t_types[i]);
		if (t_success)
			t_success = MCListCopyAsString(*t_type_list, &t_types_string);
		
		if (!t_success)
		{
			ctxt.Throw();
			return;
		}
		
		MCStringRef t_args[5];
		uindex_t t_arg_count = 5;
		t_args[0] = p_title;
		t_args[1] = p_prompt;
		t_args[2] = nil;
		t_args[3] = p_initial;
		t_args[4] = *t_types_string;
		MCDialogExecCustomAskDialog(ctxt, MCN_file_selector, MCN_file, p_as_sheet, t_args, t_arg_count, t_cancelled, &t_value);
		if (ctxt . HasError())
			return;
	}

	if (!t_cancelled)
	{
		ctxt . SetItToValue(*t_value);
        if (*t_result != nil)
            ctxt . SetTheResultToValue(*t_result);
        else
            ctxt.SetTheResultToEmpty();
	}
	else
	{
		ctxt . SetItToEmpty();
		ctxt . SetTheResultToValue(MCN_cancel);
	}
}

void MCDialogExecCustomAskDialog(MCExecContext& ctxt, MCNameRef p_stack, MCNameRef p_type, bool p_as_sheet, MCStringRef *p_args, uindex_t p_arg_count, bool& r_cancelled, MCStringRef& r_filename)
{
	bool t_success = true;
	
	MCAutoStringRef t_arg_string;
	t_success = MCStringCreateMutable(0, &t_arg_string) &&
	MCStringAppendFormat(*t_arg_string, "ask %@", p_type);
	
    /* Custom ask dialog parameters are '\0' separated. Prior to 7, any arguments
     * containing '\0' would be truncated at the NUL. However, since 7 such strings
     * have caused extra arguments to be passed. Therefore, we revert to the pre-7
     * behavior and truncate. */
	for (uindex_t i = 0; t_success && i < p_arg_count; i++)
    {
        t_success = MCStringAppendNativeChar(*t_arg_string, '\0');
        if (t_success && p_args[i] != nullptr)
        {
            MCRange t_range;
            t_range.offset = 0;
            if (!MCStringFirstIndexOfChar(p_args[i], '\0', 0, kMCStringOptionCompareExact, t_range.length))
            {
                t_range.length = UINDEX_MAX;
            }
            t_success = MCStringAppendSubstring(*t_arg_string, p_args[i], t_range);
        }
    }
    
	if (t_success)
		t_success = MCdialogdata->setvalueref(*t_arg_string);
	
	MCStack *t_stack;
	if (t_success)
		t_stack = ctxt.GetObject()->getstack()->findstackname(p_stack);
	
	Boolean t_old_trace = MCtrace;
	MCtrace = False;
	
	if (t_success && t_stack != nil)
	{
		MCStack *t_parent_stack = nil;
		
		if (MCdefaultstackptr->getopened() || !MCtopstackptr)
			t_parent_stack = MCdefaultstackptr;
		else
			t_parent_stack = MCtopstackptr;
		
        Boolean added = False;
        if (MCnexecutioncontexts < MAX_CONTEXTS)
        {
            MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
            added = True;
        }
        
		t_success = ES_NORMAL == t_stack->openrect(t_parent_stack->getrect(), p_as_sheet ? WM_SHEET : WM_MODAL, p_as_sheet ? t_parent_stack : nil, WP_DEFAULT, OP_NONE);
        
        if (added)
            MCnexecutioncontexts--;
	}
	
	MCtrace = t_old_trace;
	
	if (t_success)
		t_success = ctxt . ConvertToString(MCdialogdata -> getvalueref(), r_filename);
	
	if (t_success)
	{
		if (MCStringGetLength(r_filename) == 1 && MCStringGetCharAtIndex(r_filename, 0) == 0)
			r_cancelled = true;
		else
			r_cancelled = false;
	}
    else
        ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCDialogGetColorDialogColors(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_color_list)
{
    MCColor *t_list;
    uindex_t t_count;
    MCA_getcolordialogcolors(t_list, t_count);
    
    MCAutoArray<MCStringRef> t_colors;
    
    bool t_success;
    t_success = true;
    
    for (uindex_t i = 0; t_success && i < t_count; i++)
    {
		if (t_list[i].red != 0 || t_list[i].green != 0 || t_list[i].blue != 0)
		{
			MCStringRef t_color;
			t_success = MCStringFormat(t_color, "%d,%d,%d", t_list[i] . red, t_list[i] . green, t_list[i] . blue) && t_colors . Push(t_color);
		}
		else
			t_success = t_colors.Push(kMCEmptyString);
    }
    
    t_colors . Take(r_color_list, r_count);
}

void MCDialogSetColorDialogColors(MCExecContext& ctxt, uindex_t p_count, MCStringRef* p_color_list)
{
    MCAutoArray<MCColor> t_list;
    bool t_success;
    t_success = true;
    
    for (uindex_t i = 0; t_success && i < 16; i++)
    {
        MCColor t_color;
        if (i >= p_count || MCStringIsEmpty(p_color_list[i]))
        {
			t_color = MCzerocolor;
            t_success = t_list . Push(t_color);
        }
        else
        {
            t_success = MCscreen -> parsecolor(p_color_list[i], t_color) && t_list . Push(t_color);
        }
    }
    
    MCA_setcolordialogcolors(t_list . Ptr(), p_count);
}
