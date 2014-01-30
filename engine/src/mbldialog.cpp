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

#include "core.h"

#include "mblsyntax.h"


bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

bool MCSystemPickDate(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect);
bool MCSystemPickTime(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, int32_t p_step, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect);
bool MCSystemPickDateAndTime(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, int32_t p_step, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect);

bool MCSystemPickOption(const_cstring_array_t **p_expression, const_int32_array_t *p_indexes, uint32_t p_expression_cnt, const_int32_array_t *&r_result, bool p_use_hilited, bool p_use_picker, bool p_use_cancel, bool p_use_done, bool &r_canceled, MCRectangle p_button_rect);

bool MCSystemPickMedia(MCMediaType *p_media_type, const char *&r_result, bool p_multiple);

//////////////////////////////////////// Media Pickers ////////////////////////////////////////

static MCMediaType MCMediaTypeFromCString(const char *p_string)
{
#ifdef /* MCMediaTypeFromCString */ LEGACY_EXEC
    const char *t_ptr = p_string;
    MCMediaType t_media_type = kMCunknownMediaType;
    
    while (true)
    {
        while(*t_ptr == ' ' || *t_ptr == ',')
            t_ptr += 1;
        if (*t_ptr == '\0')
            break;
    	// HC-2012-02-01: [[ Bug 9983 ]] - This fix is related as the implementation in the new syntax does not produce a result
        if (MCCStringEqualSubstringCaseless(t_ptr, "podcasts", 7))
            t_media_type = t_media_type | kMCpodcasts;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "songs", 4))
            t_media_type = t_media_type | kMCsongs;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "audiobooks", 9))
            t_media_type = t_media_type | kMCaudiobooks;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "movies", 5))
            t_media_type = t_media_type | kMCmovies;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "musicvideos", 10))
            t_media_type = t_media_type | kMCmusicvideos;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "tv", 2))
            t_media_type = t_media_type | kMCtv;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "videopodcasts", 12))
            t_media_type = t_media_type | kMCvideopodcasts;

        while(*t_ptr != ' ' && *t_ptr != ',' && *t_ptr != '\0')
            t_ptr += 1;

    }
    return t_media_type;
#endif /* MCMediaTypeFromCString */
}

static MCMediaScope MCMediaScopeFromCString(const char *p_string)
{
    if (MCCStringEqualCaseless(p_string, "audio"))
        return kMCaudio;
    else if (MCCStringEqualCaseless(p_string, "media"))
        return kMCmedia;
    
    return kMCunknownMediaType;
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
}

//iphonePickMedia [multiple] [, music, podCast, audioBook, anyAudio, movie, tv, videoPodcast, musicVideo, videoITunesU, anyVideo]
Exec_stat MCHandleIPhonePickMedia(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleIPhonePickMedia */ LEGACY_EXEC
	bool t_success, t_allow_multipe_items;
	char *t_option_list;
    const char *r_return_media_types;
	MCMediaType t_media_types;
	
	t_success = true;
	t_allow_multipe_items = false;
	t_media_types = 0;
	
	t_option_list = nil;
	
    MCExecPoint ep(nil, nil, nil);
    
	// Get the options list.
	t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	while (t_success)
	{
		if (MCCStringEqualCaseless(t_option_list, "true"))
			t_allow_multipe_items = true;
		else if (MCCStringEqualCaseless(t_option_list, "music"))
			t_media_types += kMCsongs;
		else if (MCCStringEqualCaseless(t_option_list, "podCast"))
			t_media_types += kMCpodcasts;
		else if (MCCStringEqualCaseless(t_option_list, "audioBook"))
			t_media_types += kMCaudiobooks;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
		{
			if (MCCStringEqualCaseless(t_option_list, "movie"))
				t_media_types += kMCmovies;
			else if (MCCStringEqualCaseless(t_option_list, "tv"))
				t_media_types += kMCtv;
			else if (MCCStringEqualCaseless(t_option_list, "videoPodcast"))
				t_media_types += kMCvideopodcasts;
			else if (MCCStringEqualCaseless(t_option_list, "musicVideo"))
				t_media_types += kMCmusicvideos;
			else if (MCCStringEqualCaseless(t_option_list, "videoITunesU"))
				t_media_types += kMCmovies;
		}
#endif
	t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	}
	if (t_media_types == 0)
	{
		t_media_types = MCMediaTypeFromCString("podcast, songs, audiobook");;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
			t_media_types += MCMediaTypeFromCString("movies, tv, videoPodcasts, musicVideos, videoITunesU");;
#endif		
	}
    MCExecContext t_ctxt(ep);
    
	// Call MCIPhonePickMedia to process the media pick selection. 
    MCDialogExecPickMedia(t_ctxt, &t_media_types, t_allow_multipe_items, r_return_media_types);
	
	return ES_NORMAL;
#endif /* MCHandleIPhonePickMedia */
}

//////////////////////////////////////// Item Pickers ////////////////////////////////////////

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

void MCDialogExecPickOptionByIndex(MCExecContext &p_ctxt, MCChunkType p_chunk_type, const_cstring_array_t *p_option_lists, const_int32_array_t *p_initial_indices, bool p_use_hilite_type, bool p_use_picker, bool p_use_cancel, bool p_use_done, const_int32_array_t &r_picked_options, MCRectangle p_button_rect)
{
#ifdef /* MCDialogExecPickOptionByIndex */ LEGACY_EXEC
    bool t_success, t_cancelled;
    const_cstring_array_t **t_option_lists = nil;
    const_int32_array_t *t_option_result = nil;
    uint32_t t_option_lists_count;

    char t_delimiter[2] = {'\0','\0'};
    char *t_return_string = nil;
    
    // Create the multi dimensional option lists
	t_success = SplitOptionListsByChunk(p_chunk_type, p_option_lists, t_option_lists, t_option_lists_count);

    // Open the picker and allow the user to select the options
    if (t_success)
        t_success = MCSystemPickOption(t_option_lists, p_initial_indices, t_option_lists_count, t_option_result, p_use_hilite_type, p_use_picker, p_use_cancel, p_use_done, t_cancelled, p_button_rect);

    p_ctxt.SetTheResultToEmpty();
    
    if (t_success)
    {
        if (t_cancelled)
        {
            // HC-2012-02-15 [[ BUG 9999 ]] Picker should return 0 if cancel was selected.
            p_ctxt.GetEP().setnvalue(0);
        }
        else
        {
            if (t_success)
                t_success = IndexesArrayToString (t_option_result, t_return_string, t_cancelled);
            
            p_ctxt.GetEP().setcstring(t_return_string);
            // make execpoint take ownership of result string
            p_ctxt.GetEP().grabsvalue();
        }
    }
    
    // Free memory
    FreeStringArray (t_option_lists, t_option_lists_count);
    FreeIndexArray(t_option_result);
#endif /* MCDialogExecPickOptionByIndex */
}

Exec_stat MCHandlePick(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePick */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);

	bool t_use_cancel, t_use_done, t_use_picker, t_use_checkmark, t_more_optional, t_success;
	t_success = true;
	t_more_optional = true;
	t_use_checkmark = false;
	t_use_done = false;
	t_use_cancel = false;
	t_use_picker = false;
	
    char *t_options_list = nil;
    const_cstring_array_t *t_option_list_array = nil;
    const_int32_array_t r_picked_options = {nil, 0};	

	uint32_t t_initial_index;
    const_int32_array_t *t_initial_index_array = nil;
	
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    
	t_options_list = nil;
	// get the mandatory options list and the initial index
    // HC-30-2011-30 [[ Bug 10036 ]] iPad pick list only returns 0.
	t_success = MCParseParameters(p_parameters, "s", &t_options_list);
    if (t_success)
    {
        t_success = MCParseParameters(p_parameters, "u", &t_initial_index);
        if (!t_success)
        {
            // Degrade gracefully, even if the second mandatory parameter is not supplied.
            t_initial_index = 0;
            t_success = true;
        }
    }
    if (t_success)

    if (t_success)
        t_success = MCMemoryNew(t_option_list_array);
    if (t_success)
    {
        t_option_list_array->length = 0;
        t_option_list_array->elements = nil;
    }
    if (t_success)
        t_success = MCMemoryNew(t_initial_index_array);
    if (t_success)
    {
        t_initial_index_array->length = 0;
        t_initial_index_array->elements = nil;
    }
   
	// get the optional option lists, initial indexes and the style
	while (t_more_optional && t_success)
	{
        if (t_success)
            t_success = MCMemoryResizeArray(t_option_list_array->length + 1, t_option_list_array->elements, t_option_list_array->length);
        if (t_success)
            t_option_list_array->elements[t_option_list_array->length - 1] = t_options_list;

		// convert the initial index for each component into an array entry
        if (t_success)
            t_success = MCMemoryResizeArray(t_initial_index_array->length + 1, t_initial_index_array->elements, t_initial_index_array->length);

        if (t_success)
            t_initial_index_array->elements[t_initial_index_array->length - 1] = t_initial_index;

		t_success = MCParseParameters(p_parameters, "s", &t_options_list);
        // HC-2011-09-28 [[ Picker Buttons ]] Updated parameter parsing so we do not skip more than one paramter
		if (t_success)
		{			
			if (t_options_list != nil && (MCCStringEqualCaseless(t_options_list, "checkmark")) ||
				(MCCStringEqualCaseless(t_options_list, "cancel")) || 
				(MCCStringEqualCaseless(t_options_list, "done")) ||
				(MCCStringEqualCaseless(t_options_list, "cancelDone")) ||
				(MCCStringEqualCaseless(t_options_list, "picker")))
			{
				t_more_optional = false;
				// HC-2011-09-28 [[ Picker Buttons ]] Get the button values that are to be displayed.
				while (t_options_list != nil)
				{
					t_success = true;
					if (t_options_list != nil && MCCStringEqualCaseless(t_options_list, "checkmark"))
						t_use_checkmark = true;
					else if (t_options_list != nil && MCCStringEqualCaseless(t_options_list, "cancel"))
						t_use_cancel = true;
					else if (t_options_list != nil && MCCStringEqualCaseless(t_options_list, "done"))
						t_use_done = true;
					else if (t_options_list != nil && MCCStringEqualCaseless(t_options_list, "cancelDone"))
					{
						t_use_cancel = true;
						t_use_done = true;
					}
					else if (t_options_list != nil && MCCStringEqualCaseless(t_options_list, "picker"))
						t_use_picker = true;
					else
						t_success = false;
					if (!MCParseParameters(p_parameters, "s", &t_options_list))
						t_options_list = nil;
				}
			}
			else
				t_success = MCParseParameters(p_parameters, "u", &t_initial_index);
		}
		else
		{
			t_success = true;
			t_more_optional = false;
		}
	}

	// call MCSystemPick to process the pick wheel
	MCDialogExecPickOptionByIndex(t_ctxt, kMCLines, t_option_list_array, t_initial_index_array, t_use_checkmark, t_use_picker, t_use_cancel, t_use_done, r_picked_options, MCtargetptr->getrect());

	
	if (t_success)
    {
        // at the moment, this is the only way to return a value from the function.  pick (date/time/...) should probably
        // set the value of the 'it' variable
        if (MCresult->isempty())
            MCresult->store(ep, True);
    }
	
	return t_ctxt.GetStat();
#endif /* MCHandlePick */
}

////////////////////////////////////////////////////////////////////////////////

void MCDialogExecPickDate(MCExecContext &p_ctxt, MCDateTime *p_current, MCDateTime *p_start, MCDateTime *p_end, bool p_cancel_button, bool p_done_button, MCRectangle p_button_rect)
{
#ifdef /* MCDialogExecPickDate */ LEGACY_EXEC
    bool t_success = true;
    bool t_canceled = false;
    MCDateTime t_result;
    
    t_success = MCSystemPickDate(p_current, p_start, p_end, p_cancel_button, p_done_button, &t_result, t_canceled, p_button_rect);
    
    if (t_success)
    {
        if (t_canceled)
            p_ctxt.SetTheResultToStaticCString("cancel");
        else
        {
            p_ctxt.SetTheResultToEmpty();
            MCExecPoint &ep = p_ctxt.GetEP();
            MCD_convert_from_datetime(ep, CF_DATE, CF_UNDEFINED, t_result);
        }
    }
#endif /* MCDialogExecPickDate */
}

void MCDialogExecPickTime(MCExecContext &p_ctxt, MCDateTime *p_current, MCDateTime *p_start, MCDateTime *p_end, int32_t p_step, bool p_cancel_button, bool p_done_button, MCRectangle p_button_rect)
{
#ifdef /* MCDialogExecPickTime */ LEGACY_EXEC
    bool t_success = true;
    bool t_canceled = false;
    MCDateTime t_result;
    
    t_success = MCSystemPickTime(p_current, p_start, p_end, p_step, p_cancel_button, p_done_button, &t_result, t_canceled, p_button_rect);
    
    if (t_success)
    {
        if (t_canceled)
            p_ctxt.SetTheResultToStaticCString("cancel");
        else
        {
            p_ctxt.SetTheResultToEmpty();
            MCExecPoint &ep = p_ctxt.GetEP();
            MCD_convert_from_datetime(ep, CF_TIME, CF_UNDEFINED, t_result);
        }
    }
#endif /* MCDialogExecPickTime */
}

void MCDialogExecPickDateAndTime(MCExecContext &p_ctxt, MCDateTime *p_current, MCDateTime *p_start, MCDateTime *p_end, int32_t p_step, bool p_cancel_button, bool p_done_button, MCRectangle p_button_rect)
{
#ifdef /* MCDialogExecPickDateAndTime */ LEGACY_EXEC
    bool t_success = true;
    bool t_canceled = false;
    MCDateTime t_result;
    
    t_success = MCSystemPickDateAndTime(p_current, p_start, p_end, p_step, p_cancel_button, p_done_button, &t_result, t_canceled, p_button_rect);
    
    if (t_success)
    {
        if (t_canceled)
            p_ctxt.SetTheResultToStaticCString("cancel");
        else
        {
            p_ctxt.SetTheResultToEmpty();
            MCExecPoint &ep = p_ctxt.GetEP();
            MCD_convert_from_datetime(ep, CF_DATE, CF_TIME, t_result);
        }
    }
#endif /* MCDialogExecPickDateAndTime */
}

////////////////////////////////////////////////////////////////////////////////

// MM-2012-11-02: Temporarily refactored mobilePickDate to use the old syntax (rather than three separate pick date, pick time, pick date and time).
Exec_stat MCHandlePickDate(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePickDate */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    char *t_type;    
    t_type = nil;
    
	bool t_success, t_use_done, t_use_cancel;
	t_success = true;
	t_use_done = false;
	t_use_cancel = false;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    MCDateTime t_current_date;
    MCDateTime t_start_date;
    MCDateTime t_end_date;
    
    uint32_t t_step = 1;
    
    if (t_success && p_parameters != nil)
  		t_success = MCParseParameters(p_parameters, "s", &t_type);
    
    if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_current = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_current_date);
        }
        p_parameters = p_parameters->getnext();
    }

    if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_start = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_start_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_end = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_end_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
    if (t_success && p_parameters != nil)
        t_success = MCParseParameters(p_parameters, "u", &t_step);
    
    if (t_success && p_parameters != nil)
    {
        char *t_button;
        t_button = nil;
		t_success = MCParseParameters(p_parameters, "s", &t_button);
        if (t_success)
        {
            if (MCCStringEqualCaseless("cancel", t_button))
                t_use_cancel = true;
            else if (MCCStringEqualCaseless("done", t_button))
                t_use_done = true;
            else if (MCCStringEqualCaseless("canceldone", t_button))
                t_use_cancel = t_use_done = true;
        }
        MCCStringFree(t_button);
    }

    
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    
    MCDateTime *t_current_date_ptr = nil;
    MCDateTime *t_start_date_ptr = nil;
    MCDateTime *t_end_date_ptr = nil;
    
    if (t_use_current)
        t_current_date_ptr = &t_current_date;
    if (t_use_start)
        t_start_date_ptr = &t_start_date;
    if (t_use_end)
        t_end_date_ptr = &t_end_date;
    
	if (t_success)
    {
        // MM-2012-03-15: [[ Bug ]] Make sure we handle no type being passed.
        if (t_type == nil)
            MCDialogExecPickDate(t_ctxt, t_current_date_ptr, t_start_date_ptr, t_end_date_ptr, t_use_cancel, t_use_done, MCtargetptr->getrect());
        else if (MCCStringEqualCaseless("time", t_type))
            MCDialogExecPickTime(t_ctxt, t_current_date_ptr, t_start_date_ptr, t_end_date_ptr, t_step, t_use_cancel, t_use_done, MCtargetptr->getrect());
        else if (MCCStringEqualCaseless("datetime", t_type))
            MCDialogExecPickDateAndTime(t_ctxt, t_current_date_ptr, t_start_date_ptr, t_end_date_ptr, t_step, t_use_cancel, t_use_done, MCtargetptr->getrect());
        else
            MCDialogExecPickDate(t_ctxt, t_current_date_ptr, t_start_date_ptr, t_end_date_ptr, t_use_cancel, t_use_done, MCtargetptr->getrect());
    }
    
    MCCStringFree(t_type);
    
    // at the moment, this is the only way to return a value from the function.  pick (date/time/...) should probably
    // set the value of the 'it' variable
    if (MCresult->isempty())
        MCresult->store(ep, True);
    
	return t_ctxt.GetStat();
#endif /* MCHandlePickDate */
}

Exec_stat MCHandlePickTime(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePickTime */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
	bool t_success, t_use_done, t_use_cancel;
	t_success = true;
	t_use_done = false;
	t_use_cancel = false;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    MCDateTime t_current_date;
    MCDateTime t_start_date;
    MCDateTime t_end_date;
    
    uint32_t t_step = 1;
    
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_current = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_current_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_start = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_start_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_end = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_end_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
    if (t_success && p_parameters != nil)
        t_success = MCParseParameters(p_parameters, "u", &t_step);
    
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_cancel);
	
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_done);
    
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    
    MCDateTime *t_current_date_ptr = nil;
    MCDateTime *t_start_date_ptr = nil;
    MCDateTime *t_end_date_ptr = nil;
    
    if (t_use_current)
        t_current_date_ptr = &t_current_date;
    if (t_use_start)
        t_start_date_ptr = &t_start_date;
    if (t_use_end)
        t_end_date_ptr = &t_end_date;
    
	if (t_success)
		MCDialogExecPickTime(t_ctxt, t_current_date_ptr, t_start_date_ptr, t_end_date_ptr, t_step, t_use_cancel, t_use_done, MCtargetptr->getrect());
    
    if (MCresult->isempty())
        MCresult->store(ep, True);
    
	return t_ctxt.GetStat();
#endif /* MCHandlePickTime */
}


Exec_stat MCHandlePickDateAndTime(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePickDateAndTime */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
	bool t_success, t_use_done, t_use_cancel;
	t_success = true;
	t_use_done = false;
	t_use_cancel = false;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    MCDateTime t_current_date;
    MCDateTime t_start_date;
    MCDateTime t_end_date;
    
    uint32_t t_step = 1;
    
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_current = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_current_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_start = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_start_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_end = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_end_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
    if (t_success && p_parameters != nil)
        t_success = MCParseParameters(p_parameters, "u", &t_step);
    
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_cancel);
	
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_done);
    
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    
    MCDateTime *t_current_date_ptr = nil;
    MCDateTime *t_start_date_ptr = nil;
    MCDateTime *t_end_date_ptr = nil;
    
    if (t_use_current)
        t_current_date_ptr = &t_current_date;
    if (t_use_start)
        t_start_date_ptr = &t_start_date;
    if (t_use_end)
        t_end_date_ptr = &t_end_date;
    
	if (t_success)
		MCDialogExecPickDateAndTime(t_ctxt, t_current_date_ptr, t_start_date_ptr, t_end_date_ptr, t_step, t_use_cancel, t_use_done, MCtargetptr->getrect());
   
    if (MCresult->isempty())
        MCresult->store(ep, True);
    
	return t_ctxt.GetStat();
#endif /* MCHandlePickDateAndTime */
}
