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
#include "mcio.h"

#include "globals.h"
#include "debug.h"
#include "handler.h"

#include "mblsyntax.h"
#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EXEC_METHOD(Pick, PickDate, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Pick, PickTime, 6)
MC_EXEC_DEFINE_EXEC_METHOD(Pick, PickDateAndTime, 6)

////////////////////////////////////////////////////////////////////////////////

enum 
{
    kMCPickDate,
    kMCPickTime,
    kMCPickDateAndTime
};

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCPickButtonTypeElementInfo[] =
{
	{ "cancel", kMCPickButtonCancel },
	{ "done", kMCPickButtonDone },
	{ "canceldone", kMCPickButtonCancelAndDone },
};

static MCExecEnumTypeInfo _kMCPickButtonTypeTypeInfo =
{
	"Pick.ButtonType",
	sizeof(_kMCPickButtonTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCPickButtonTypeElementInfo
};

//////////

MCExecEnumTypeInfo *kMCPickButtonTypeTypeInfo = &_kMCPickButtonTypeTypeInfo;

//////////

void MCPickDoPickDateTime(MCExecContext& ctxt, MCStringRef p_current, MCStringRef p_start, MCStringRef p_end, int32_t *p_step, MCPickButtonType p_buttons, MCRectangle p_button_rect, int p_which)
{
    MCDateTime t_current;
    MCDateTime *t_current_ptr;
    t_current_ptr = nil;
    
    if (p_current != nil)
    {
        if (!MCD_convert_to_datetime(ctxt, p_current, CF_UNDEFINED, CF_UNDEFINED, t_current))
            return;
        t_current_ptr = &t_current;
    }
    
    MCDateTime t_start;
    MCDateTime *t_start_ptr;
    t_start_ptr = nil;
    
    if (p_start != nil)
    {
        if (!MCD_convert_to_datetime(ctxt, p_start, CF_UNDEFINED, CF_UNDEFINED, t_start))
            return;
        t_start_ptr = &t_start;
    }
    
    MCDateTime t_end;
    MCDateTime *t_end_ptr;
    t_end_ptr = nil;
    
    if (p_end != nil)
    {
        if (!MCD_convert_to_datetime(ctxt, p_end, CF_UNDEFINED, CF_UNDEFINED, t_end))
            return;
        t_end_ptr = &t_end;
    }
    
    int32_t t_step;
    if (p_step != nil)
        t_step = *p_step;
    else
        t_step = 1;
    
    bool t_cancel_button, t_done_button;
    
    t_cancel_button = false;
    t_done_button = false;
    
    switch (p_buttons)
    {
        case kMCPickButtonCancel:
            t_cancel_button = true;
            break;
        case kMCPickButtonDone:
            t_done_button = true;
            break;
        case kMCPickButtonCancelAndDone:
            t_cancel_button = true;
            t_done_button = true;
            break;
        case kMCPickButtonNone:
        default:
            break;
    }
    
    MCDateTime t_result;
    bool t_cancelled;
    t_cancelled = false;
    
    bool t_success;
    MCAutoStringRef t_result_string;
    
    switch (p_which)
    {
    case kMCPickDate:
            t_success = (MCSystemPickDate(t_current_ptr, t_start_ptr, t_end_ptr, t_cancel_button, t_done_button, &t_result, t_cancelled, p_button_rect) && MCD_convert_from_datetime(ctxt, t_result, CF_DATE, CF_UNDEFINED, &t_result_string));
        break;
    case kMCPickTime:
            t_success = (MCSystemPickTime(t_current_ptr, t_start_ptr, t_end_ptr, t_step, t_cancel_button, t_done_button, &t_result, t_cancelled, p_button_rect) && MCD_convert_from_datetime(ctxt, t_result, CF_TIME, CF_UNDEFINED, &t_result_string));
        break;
    case kMCPickDateAndTime:
            t_success = (MCSystemPickDateAndTime(t_current_ptr, t_start_ptr, t_end_ptr, t_step, t_cancel_button, t_done_button, &t_result, t_cancelled, p_button_rect) && MCD_convert_from_datetime(ctxt, t_result, CF_DATE, CF_TIME, &t_result_string));
        break;
    }
    
    if (t_success)
    {
        if (t_cancelled)
            ctxt.SetTheResultToStaticCString("cancel");
        else
            ctxt . SetTheResultToValue(&t_result_string);
        return;
    }
    
    ctxt . SetTheResultToEmpty();
}

void MCPickExecPickDate(MCExecContext& ctxt, MCStringRef p_current, MCStringRef p_start, MCStringRef p_end, intenum_t p_buttons, MCRectangle p_button_rect)
{
    MCPickDoPickDateTime(ctxt, p_current, p_start, p_end, nil, (MCPickButtonType)p_buttons, p_button_rect, kMCPickDate);
}

void MCPickExecPickTime(MCExecContext &ctxt, MCStringRef p_current, MCStringRef p_start, MCStringRef p_end, int32_t *p_step, intenum_t p_buttons, MCRectangle p_button_rect)
{
    MCPickDoPickDateTime(ctxt, p_current, p_start, p_end, p_step, (MCPickButtonType)p_buttons, p_button_rect, kMCPickTime);
}

void MCPickExecPickDateAndTime(MCExecContext &ctxt, MCStringRef p_current, MCStringRef p_start, MCStringRef p_end, int32_t *p_step, intenum_t p_buttons, MCRectangle p_button_rect)
{
    MCPickDoPickDateTime(ctxt, p_current, p_start, p_end, p_step, (MCPickButtonType)p_buttons, p_button_rect, kMCPickDateAndTime);
}

void MCPickExecPickOptionByIndex(MCExecContext &ctxt, int p_chunk_type, MCStringRef *p_option_lists, uindex_t p_option_list_count, uindex_t *p_initial_indices, uindex_t p_indices_count, bool p_use_hilite_type, bool p_use_picker, bool p_use_cancel, bool p_use_done, MCRectangle p_button_rect)
{
    
    MCAutoArray<MCPickList> t_pick_lists;
    
    char_t t_delimiter;
    switch ((MCChunkType)p_chunk_type)
    {
        case kMCItems:
            t_delimiter = ctxt . GetItemDelimiter();
        default:
        case kMCLines:
            t_delimiter = '\n';
    }
    uindex_t t_old_offset = 0;
    uindex_t t_new_offset = 0;
    
    bool t_success;
    t_success = true;
    
    for (uindex_t i = 0; i < p_option_list_count; i++)
    {
        MCStringRef t_option;
        MCPickList t_pick_list;
        MCAutoArray<MCStringRef> t_options;
        while (t_success && MCStringFirstIndexOfChar(p_option_lists[i], t_delimiter, t_old_offset, kMCCompareCaseless, t_new_offset))
        {
            t_success = MCStringCopySubstring(p_option_lists[i], MCRangeMake(t_old_offset, t_new_offset - 1), t_option);
            if (t_success)
                t_options . Push(t_option);
            t_old_offset = t_new_offset;
        }
        t_pick_list . options = t_options . Ptr();
        t_pick_list . option_count = t_options . Size();
        t_pick_list . initial = p_initial_indices[i];
        t_pick_lists . Push(t_pick_list);
    }
    
    bool t_cancelled;
    
    uindex_t *t_result;
    uindex_t t_result_count = 0;
    
    // Open the picker and allow the user to select the options
    if (t_success)
        t_success = MCSystemPickOption(t_pick_lists . Ptr(), t_pick_lists . Size(), t_result, t_result_count, p_use_hilite_type, p_use_picker, p_use_cancel, p_use_done, t_cancelled, p_button_rect);
    
    ctxt.SetTheResultToEmpty();
    
    if (t_success)
    {
        if (t_cancelled)
        {
            // HC-2012-02-15 [[ BUG 9999 ]] Picker should return 0 if cancel was selected.
            ctxt.GetEP().setnvalue(0);
        }
        else
        {
            MCAutoListRef t_indices;
            t_success = MCListCreateMutable(',', &t_indices);
            for (uindex_t i = 0; i < t_result_count && t_success; i++)
            {
                MCAutoStringRef t_index;
                t_success = MCStringFormat(&t_index, "%u", t_result[i]);
                if (t_success)
                    t_success = MCListAppend(*t_indices, *t_index);
            }
			MCAutoStringRef t_string;
			MCListCopyAsString(*t_indices, &t_string);
            ctxt . GetEP() . setvalueref(*t_string);
        }
    }
    
    // Free memory
    for (uindex_t i = 0; i < t_pick_lists . Size(); i++)
        for (uindex_t j = 0; j < t_pick_lists[i] . option_count; j++)
            MCValueRelease(t_pick_lists[i] . options[j]);
}

// pick [ "multiple" ] (podcast(s), song(s), audiobook(s), movie(s), musicvideo(s)) from library
void MCDialogExecPickMedia(MCExecContext &p_ctxt, MCMediaType *p_media_type, bool p_multiple, const char *&r_result)
{
    MCSystemPickMedia(p_media_type, r_result, p_multiple);
    p_ctxt.SetTheResultToStaticCString(r_result);
}

// pick [ "multiple" ] items from [(audio|media) allowing expression
void MCDialogExecPickMedia(MCExecContext &p_ctxt, char *p_media_expression, MCMediaScope *p_media_scope, bool p_multiple, const char *&r_result)
{
    /*
    MCMediaType t_media_type, t_allowed_media_type;
    t_media_type = MCMediaTypeFromCString(p_media_expression);
    t_allowed_media_type = MCMediaTypeFromCString("podcast, songs, audiobook");
    
    if (*p_media_scope == kMCmedia)
        MCSystemPickMedia(&t_media_type, r_result, p_multiple);
    else if (*p_media_scope == kMCaudio)
    {
        t_media_type = t_media_type & t_allowed_media_type;
        MCSystemPickMedia(&t_media_type, r_result, p_multiple);
    }
    p_ctxt.SetTheResultToStaticCString(r_result);
     */
}

////////////////////////////////////////////////////////////////////////////////
