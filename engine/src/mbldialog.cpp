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

void FreeIndexArray (const_int32_array_t *p_array)
{
    if (p_array == nil)
		return;
	MCMemoryDeleteArray(p_array->elements);
	MCMemoryDeallocate(p_array);
}

void FreeStringArray (const_cstring_array_t *p_array)
{
    if (p_array == nil)
		return;
    
	for(uint32_t i = 0; i < p_array->length; i++)
		MCCStringFree(p_array->elements[i]);
	MCMemoryDeleteArray(p_array->elements);
	MCMemoryDeallocate(p_array);
}

void FreeStringArray (const_cstring_array_t **p_array, uint32_t p_entry_count)
{
    if (p_array == nil)
		return;
    
	for(uint32_t i = 0; i < p_entry_count; i++)
		FreeStringArray(p_array[i]);
	MCMemoryDeleteArray(p_array);
}

bool IndexesArrayToStringArray (const_cstring_array_t **p_expression, const_int32_array_t *p_indexes, const_cstring_array_t *&r_result)
{
    bool t_success = true;
    uint32_t t_element_count;
    t_success = MCMemoryNew(r_result);
	if (t_success)
		t_success = MCMemoryNewArray(p_indexes->length, r_result->elements);
    if (t_success)
    {
        r_result->length = p_indexes->length;
        for (t_element_count = 0; t_element_count < p_indexes->length; t_element_count++)
        {
            if (t_success)
                t_success = MCCStringClone(p_expression[t_element_count]->elements[p_indexes->elements[t_element_count]], r_result->elements[t_element_count]);
            else
                break;
        }
    }
    if (!t_success)
    {
        FreeStringArray (r_result);
        r_result = nil;
    }
    return t_success; 
}

// HC-2012-02-15 [[ BUG 9999 ]] Date Picker should return 0 if cancel was selected.
bool IndexesArrayToString (const_int32_array_t *p_indexes, char *&r_result, bool p_all_zero = false)
{
    bool t_success;
    char t_index[32];
    uint32_t t_string_length = 0;
    for (int i = 0; i < p_indexes->length; i++)
    {
        sprintf (t_index, "%i", p_indexes->elements[i]);
        t_string_length += strlen(t_index) + 1;
    }
    if (t_string_length)
    {
      t_success = MCMemoryAllocate(sizeof (char) * t_string_length, r_result);
      if (t_success)
      {
          // HC-2012-02-15 [[ BUG 9999 ]] Date Picker should return 0 if cancel was selected.
          p_all_zero ? sprintf (r_result, "%i", 0) : sprintf (r_result, "%i", p_indexes->elements[0]);
          for (int i = 1; i < p_indexes->length; i++)
          {
              strcat (r_result, ",");
              // HC-2012-02-15 [[ BUG 9999 ]] Date Picker should return 0 if cancel was selected.
              p_all_zero ? sprintf (t_index, "%i", 0) : sprintf (t_index, "%i", p_indexes->elements[i]);
              strcat (r_result, t_index);
          }
      }
    }
    return t_success;
}

bool StringArrayToIndexesArray (const_cstring_array_t **p_expression, const_cstring_array_t *p_indexes, const_int32_array_t *&r_result)
{
    bool t_success = true;
    uint32_t t_element_count, t_item_count;
    t_success = MCMemoryNew(r_result);
    if (t_success)
        t_success = MCMemoryResizeArray(p_indexes->length, r_result->elements, r_result->length);
    if (t_success)
    {
        for (t_element_count = 0; t_element_count < p_indexes->length; t_element_count++)
        {
            for (t_item_count = 0; t_item_count < p_expression[t_element_count]->length; t_item_count++);
            {
                if (strcmp (p_indexes->elements[t_element_count], p_expression[t_element_count]->elements[t_item_count]) == 0)
                {
                    r_result->elements[t_element_count] = t_item_count;
                    break;
                }
            }
        }
    }
    if (!t_success)
    {
        FreeIndexArray (r_result);
        r_result = nil;
    }
    return t_success;
}

bool StringArrayToString (const_cstring_array_t *p_indexes, char *&r_result, MCChunkType p_chunk_type = kMCWords)
{
    bool t_success = true;
	char t_delimiter[2] = {'\0','\0'};

    if (p_chunk_type == kMCWords)
        sprintf (t_delimiter, "%s", ",");
    else if (p_chunk_type == kMCLines)
        sprintf (t_delimiter, "%s", "\n");

    uint32_t t_string_length = 0;
    for (int i = 0; i < p_indexes->length; i++)
        t_string_length += strlen(p_indexes->elements[i]) + 1;
    if (t_string_length)
    {
        t_success = MCMemoryAllocate(sizeof (char) * t_string_length, r_result);
        if (t_success)
        {
            strcpy (r_result, p_indexes->elements[0]);
            for (int i = 1; i < p_indexes->length; i++)
            {
                strcat (r_result, t_delimiter);
                strcat (r_result, p_indexes->elements[i]);
            }
        }
    }
    if (!t_success)
    {
        free (r_result);
        r_result = nil;
    }
    return t_success;
}

static bool contains_char(const char *p_string, char p_char)
{
	return p_char != '\0' && strchr(p_string, p_char) != nil;
}

bool StringTokenize(const char *p_string, const char *p_delimiters, char**& r_elements, uint32_t& r_element_count)
{
	bool t_success;
	t_success = true;
    
	char **t_elements;
	t_elements = nil;
	
	uint32_t t_element_count;
	t_element_count = 0;
	
	if (p_string != nil)
	{
		const char *t_ptr;
		t_ptr = p_string;
		
		const char *t_token;
		t_token = nil;
		
		while(t_success)
		{
			// Skip spaces
			while(contains_char(p_delimiters, *t_ptr))
				t_ptr += 1;
			
			t_token = t_ptr;
			
			// If we find a quote, start a quoted token, else loop until whitespace
			if (*t_ptr == '"')
			{
				t_ptr += 1;
				while(*t_ptr != '\0' && *t_ptr != '"')
					t_ptr += 1;
				
				if (*t_ptr == '"')
					t_ptr += 1;
			}
			else
			{
				while(*t_ptr != '\0' && !contains_char(p_delimiters, *t_ptr))
					t_ptr += 1;
			}
			
			// If ptr and token are the same, we have exhausted the string
			if (t_ptr == t_token)
				break;
			
			// Add a new element to the array, and clone the substring
			t_success = MCMemoryResizeArray(t_element_count + 1, t_elements, t_element_count);
			if (t_success)
				t_success = MCCStringCloneSubstring(t_token, t_ptr - t_token, t_elements[t_element_count - 1]);
		}
	}
	
	if (t_success)
	{
		r_elements = t_elements;
		r_element_count = t_element_count;
	}
	else
	{
		for(uint32_t i = 0; i < t_element_count; i++)
			MCCStringFree(t_elements[i]);
		MCMemoryDeleteArray(t_elements);
	}
	
	return t_success;
}

bool SplitStringByChunk(MCChunkType p_chunk_type, const char *p_string, char **&r_chunks, uint32_t &r_chunk_count, char p_itemdelimiter = ',')
{
    const char *t_delimiters = " \t\n";
    if (p_chunk_type == kMCItems)
    {
        t_delimiters = ",";
    }
    else if (p_chunk_type == kMCLines)
    {
        t_delimiters = "\n";
    }
    return StringTokenize(p_string, t_delimiters, r_chunks, r_chunk_count);
}

bool SplitOptionListsByChunk(MCChunkType p_chunk_type, const_cstring_array_t *p_option_lists, const_cstring_array_t **&r_option_lists, uint32_t &r_option_list_count, char p_itemdelimter = ',')
{
	bool t_success;
	t_success = true;

	uint32_t t_element_count;
	
	r_option_list_count = 0;
	r_option_lists = nil;
	
    t_success = MCMemoryNewArray(p_option_lists->length, r_option_lists);
    if (t_success)
        r_option_list_count = p_option_lists->length;
    
	// Loop through all of the option lists
	for (t_element_count = 0; t_success && t_element_count < p_option_lists->length; t_element_count++)
	{
        t_success = MCMemoryNew(r_option_lists[t_element_count]);
        
		if (p_option_lists->elements[t_element_count] != nil && t_success)
		{
            t_success = SplitStringByChunk(p_chunk_type, p_option_lists->elements[t_element_count], r_option_lists[t_element_count]->elements, r_option_lists[t_element_count]->length, p_itemdelimter);
		}
	}

    if (!t_success)
    {
        FreeStringArray (r_option_lists, r_option_list_count);
        r_option_lists = nil;
    }

	return t_success;
}

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
