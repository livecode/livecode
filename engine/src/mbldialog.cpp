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

#include "string.h"
#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "stack.h"
#include "image.h"
#include "param.h"

#include "exec.h"
#include "date.h"

#include "mblsyntax.h"

//////////////////////////////////////// Item Pickers ////////////////////////////////////////

#ifdef /* MCDialogExecPickOption */ LEGACY_EXEC
void MCDialogExecPickOption(MCExecContext &p_ctxt, MCChunkType p_chunk_type, const_cstring_array_t *p_option_lists, const char *p_initial_choice, bool p_use_hilite_type, bool p_use_picker, bool p_use_cancel, bool p_use_done, char *&r_picked_options, MCRectangle p_button_rect)
{
    bool t_success = true;
    bool t_cancelled = false;
    
    const_cstring_array_t **t_option_lists = nil;
    const_cstring_array_t *t_initial_choices = nil;
    const_cstring_array_t *t_result_choices_array = nil;
    
    const_int32_array_t *t_initial_indices = nil;
    const_int32_array_t *t_option_result = nil;
    uint32_t t_option_lists_count, t_initial_choices_array_count;

    char *t_return_string;
        
    // Convert the initil choices string into a const_cstring_array_t
    t_success = MCMemoryNew(t_initial_choices);
    if (t_success)
        t_success = SplitStringByChunk(p_chunk_type, p_initial_choice, t_initial_choices->elements, t_initial_choices->length);

    // Create the multi dimensional option lists
    if (t_success)
        t_success = SplitOptionListsByChunk(p_chunk_type, p_option_lists, t_option_lists, t_option_lists_count);

    // Create the indices from the list of entries
    if (t_success)
        t_success = StringArrayToIndexesArray (t_option_lists, t_initial_choices, t_initial_indices);

    // Open the picker and allow the user to select the options
    if (t_success)
        t_success = MCSystemPickOption(t_option_lists, t_initial_indices, t_option_lists_count, t_option_result, p_use_hilite_type, p_use_picker, p_use_cancel, p_use_done, t_cancelled, p_button_rect);
    
    if (t_cancelled)
    {
        //p_ctxt.SetTheResultToStaticCString("cancel");
        // TODO - the standard way to indicate cancelation of a dialog would be to set the result to cancel,
        //        but the way mobile functions work doesn't really fit with that so for now this will return the initial choice 
        p_ctxt.GetEP().copysvalue(p_initial_choice);
    }
    else
    {
         // Convert the indices returned into entries
        if (t_success)
            t_success = IndexesArrayToStringArray (t_option_lists, t_option_result, t_result_choices_array);
                
        if (t_success)
            t_success = StringArrayToString (t_result_choices_array, t_return_string, p_chunk_type);

        p_ctxt.SetTheResultToEmpty();

        if (t_success)
        {
            p_ctxt.GetEP().setcstring(t_return_string);
            // make execpoint take ownership of result string
            p_ctxt.GetEP().grabsvalue();
        }
    }
    
    // Free memory
    FreeStringArray (t_option_lists, t_option_lists_count);
    FreeStringArray (t_initial_choices); 
    FreeStringArray (t_result_choices_array);
    FreeIndexArray (t_initial_indices);
    FreeIndexArray (t_option_result);
}
#endif /* MCDialogExecPickOption */
