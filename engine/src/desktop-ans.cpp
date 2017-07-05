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

#include "platform.h"
#include "platform-legacy.h"

#include "globdefs.h"
#include "filedefs.h"
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "objdefs.h"


#include "globals.h"
#include "exec.h"
#include "stack.h"
#include "ans.h"

#include "variable.h"

////////////////////////////////////////////////////////////////////////////////

// MW-2014-06-25: [[ Bug 12686 ]] Make sure we first try the defaultStack, then
//   the topStack.
static MCPlatformWindowRef compute_sheet_owner(unsigned int p_options)
{
    if ((p_options & MCA_OPTION_SHEET) == 0)
        return nil;
    
    MCPlatformWindowRef t_window;
    t_window = MCdefaultstackptr -> getwindow();
    if (t_window != nil && MCPlatformIsWindowVisible(t_window))
        return t_window;
    
    t_window = MCtopstackptr -> getwindow();
    if (t_window != nil && MCPlatformIsWindowVisible(t_window))
        return t_window;
    
    return nil;
}

static bool folder_path_from_initial_path(MCStringRef p_path, MCStringRef &r_folderpath)
{
    MCAutoStringRef t_path;
    uindex_t t_offset;
    bool t_success;
    
    t_success = false;
    
    if (MCStringFirstIndexOfChar(p_path, '/', 0, kMCStringOptionCompareExact, t_offset))
    {
        if (t_offset != 0)
            t_success = MCS_resolvepath(p_path, &t_path);
        else
            t_success = MCStringCopy(p_path, &t_path);
    }
	
    if (t_success)
	{
        if (MCS_exists(*t_path, False))
            t_success = MCStringCopy(*t_path, r_folderpath);
        else if (MCStringLastIndexOfChar(*t_path, '/', UINT32_MAX, kMCStringOptionCompareExact, t_offset))
            t_success = MCStringCopySubstring(*t_path, MCRangeMake(0, t_offset), r_folderpath);
	}
	
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// MM-2012-02-13: Updated to use Cocoa APIs.  Code mostly cribbed from plugin dialog stuff
int MCA_folder(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	MCPlatformWindowRef t_owner;
    t_owner = compute_sheet_owner(p_options);
	
    MCAutoStringRef t_initial_folder;
    if (p_initial != nil)
    /* UNCHECKED */ folder_path_from_initial_path(p_initial, &t_initial_folder);
    
	MCPlatformBeginFolderDialog(t_owner, p_title, p_prompt, *t_initial_folder);
	
	MCPlatformDialogResult t_result;
	MCAutoStringRef t_folder;
	for(;;)
	{
		t_result = MCPlatformEndFolderDialog(&t_folder);
		if (t_result != kMCPlatformDialogResultContinue)
			break;
		
		MCscreen -> wait(REFRESH_INTERVAL, True, True);
	}
	
	if (t_result == kMCPlatformDialogResultSuccess)
		r_value = MCValueRetain(*t_folder);
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

static bool filter_to_type_list(MCStringRef p_filter, MCStringRef *&r_types, uint32_t &r_type_count)
{
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = p_filter != nil;
	
	uint32_t t_filter_length;
	if (t_success)
	{
        t_filter_length = MCStringGetLength(p_filter);
		t_success = t_filter_length >= 4;
	}
    
    if (!t_success)
    {
        r_type_count = 0;
        return true;
    }
	
    MCStringRef t_types;
    if (t_success)
        t_success = MCStringCreateMutable(0, t_types);
    
	if (t_success)
        t_success = MCStringAppendFormat(t_types, "||", strlen("||"));
	
	if (t_success)
    {
        uint32_t t_current_pos;
        t_current_pos = 0;
		for (uint32_t i = 0; t_success && i < t_filter_length / 4; i++)
		{
			if (i > 0)
                t_success = MCStringAppendNativeChar(t_types, ',');
			if (t_success)
			{
                t_success = MCStringAppendSubstring(t_types, p_filter, MCRangeMake(t_current_pos, 4));
                t_current_pos += 4;
			}
		}
	}
	
	if (t_success)
		t_success = MCMemoryNewArray(1, r_types);
	
	if (t_success)
	{
		r_types[0] = t_types;
		r_type_count = 1;
	}
	else
	{
		r_type_count = 0;
        MCValueRelease(t_types);
    }
	
	return t_success;
}

static void split_initial(MCPlatformFileDialogKind p_kind, MCStringRef p_initial, MCStringRef& r_folder, MCStringRef& r_file)
{
	if (p_initial != nil)
		/* UNCHECKED */ folder_path_from_initial_path(p_initial, r_folder);
    
	if ((p_kind == kMCPlatformFileDialogKindSave) && p_initial != nil && !MCS_exists(p_initial, false))
	{
		uindex_t t_last_slash;
        if (MCStringLastIndexOfChar(p_initial, '/', UINT32_MAX, kMCStringOptionCompareExact, t_last_slash))
            // SN-2014-08-11: [[ Bug 13143 ]] Take the right part: after the last slash, not before
            MCStringCopySubstring(p_initial, MCRangeMake(t_last_slash + 1, MCStringGetLength(p_initial) - t_last_slash - 1), r_file);
        else
            r_file = MCValueRetain(p_initial);
	}
}

int MCA_file(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
    MCAutoStringRefArray t_types;
    
    filter_to_type_list(p_filter, t_types.PtrRef(), t_types.CountRef());
	
	MCPlatformWindowRef t_owner;
    t_owner = compute_sheet_owner(p_options);
	
	MCPlatformFileDialogKind t_kind;
	if ((p_options & MCA_OPTION_PLURAL) != 0)
		t_kind = kMCPlatformFileDialogKindOpenMultiple;
	else
		t_kind = kMCPlatformFileDialogKindOpen;
	
	MCAutoStringRef t_initial_folder;
	MCAutoStringRef t_initial_file;
    split_initial(t_kind, p_initial, &t_initial_folder, &t_initial_file);
    
	MCPlatformBeginFileDialog(t_kind, t_owner, p_title, p_prompt, *t_types, t_types . Count(), *t_initial_folder, *t_initial_file);
	
	MCPlatformDialogResult t_result;
	MCAutoStringRef t_file, t_type;
    
	for(;;)
	{
        MCValueRelease(*t_file);
        MCValueRelease(*t_type);
		t_result = MCPlatformEndFileDialog(t_kind, &t_file, &t_type);
		if (t_result != kMCPlatformDialogResultContinue)
			break;
		
		MCscreen -> wait(REFRESH_INTERVAL, True, True);
	}
	
	if (t_result == kMCPlatformDialogResultSuccess)
		r_value = MCValueRetain(*t_file);
	
	return 0;
}

int MCA_file_with_types(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	MCPlatformWindowRef t_owner;
    t_owner = compute_sheet_owner(p_options);
	
	MCPlatformFileDialogKind t_kind;
	if ((p_options & MCA_OPTION_PLURAL) != 0)
		t_kind = kMCPlatformFileDialogKindOpenMultiple;
	else
		t_kind = kMCPlatformFileDialogKindOpen;
	
	MCAutoStringRef t_initial_folder;
	MCAutoStringRef t_initial_file;
    split_initial(t_kind, p_initial, &t_initial_folder, &t_initial_file);
    
	MCPlatformBeginFileDialog(t_kind, t_owner, p_title, p_prompt, p_types, p_type_count, *t_initial_folder, *t_initial_file);
	
	MCPlatformDialogResult t_result;
	MCAutoStringRef t_file, t_type;
    
	for(;;)
	{
        MCValueRelease(*t_file);
        MCValueRelease(*t_type);
		t_result = MCPlatformEndFileDialog(t_kind, &t_file, &t_type);
		if (t_result != kMCPlatformDialogResultContinue)
			break;
		
		MCscreen -> wait(REFRESH_INTERVAL, True, True);
	}
	
	if (t_result == kMCPlatformDialogResultSuccess)
    {        
        r_value = MCValueRetain(*t_file);
        if (*t_type != nil)
            r_result = MCValueRetain(*t_file);
    }
	
	return 0;
	
}

int MCA_ask_file(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
    MCAutoStringRefArray t_types;
    
    filter_to_type_list(p_filter, t_types.PtrRef(), t_types.CountRef());
	
	MCPlatformWindowRef t_owner;
    t_owner = compute_sheet_owner(p_options);
	
	MCAutoStringRef t_initial_folder;
	MCAutoStringRef t_initial_file;
    split_initial(kMCPlatformFileDialogKindSave, p_initial, &t_initial_folder, &t_initial_file);
    
	MCPlatformBeginFileDialog(kMCPlatformFileDialogKindSave, t_owner, p_title, p_prompt, *t_types, t_types . Count(), *t_initial_folder, *t_initial_file);
	
	MCPlatformDialogResult t_result;
	MCAutoStringRef t_file, t_type;
    
	for(;;)
	{
        MCValueRelease(*t_file);
        MCValueRelease(*t_type);
		t_result = MCPlatformEndFileDialog(kMCPlatformFileDialogKindSave, &t_file, &t_type);
		if (t_result != kMCPlatformDialogResultContinue)
			break;
		
		MCscreen -> wait(REFRESH_INTERVAL, True, True);
	}
	
	if (t_result == kMCPlatformDialogResultSuccess)
		r_value = MCValueRetain(*t_file);
	
	return 0;
}

int MCA_ask_file_with_types(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	MCPlatformWindowRef t_owner;
    t_owner = compute_sheet_owner(p_options);
	
	MCAutoStringRef t_initial_folder;
	MCAutoStringRef t_initial_file;
    split_initial(kMCPlatformFileDialogKindSave, p_initial, &t_initial_folder, &t_initial_file);
    
	MCPlatformBeginFileDialog(kMCPlatformFileDialogKindSave, t_owner, p_title, p_prompt, p_types, p_type_count, *t_initial_folder, *t_initial_file);
	
	MCPlatformDialogResult t_result;
	MCAutoStringRef t_file, t_type;
    
	for(;;)
	{
        MCValueRelease(*t_file);
        MCValueRelease(*t_type);
		t_result = MCPlatformEndFileDialog(kMCPlatformFileDialogKindSave, &t_file, &t_type);
		if (t_result != kMCPlatformDialogResultContinue)
			break;
		
		MCscreen -> wait(REFRESH_INTERVAL, True, True);
	}
	
	if (t_result == kMCPlatformDialogResultSuccess)
	{
        r_value = MCValueRetain(*t_file);
        // SN-2014-10-31: [[ Bug 13893 ]] MCPlatformEndFileDialog might return a nil value
        if (*t_type != nil)
            r_result = MCValueRetain(*t_type);
        else
            r_result = nil;
	}
	
	return 0;
	
}

////////////////////////////////////////////////////////////////////////////////

bool MCA_color(MCStringRef p_title, MCColor p_initial, bool as_sheet, bool& r_chosen, MCColor& r_chosen_color)
{	
	MCPlatformBeginColorDialog(p_title, p_initial);
	
	MCPlatformDialogResult t_result;
	MCColor t_new_color;
	for(;;)
	{
		t_result = MCPlatformEndColorDialog(t_new_color);
		if (t_result != kMCPlatformDialogResultContinue)
			break;
		
		MCscreen -> wait(REFRESH_INTERVAL, True, True);
	}
	
	if (t_result == kMCPlatformDialogResultSuccess)
	{
        r_chosen_color = t_new_color;
        r_chosen = true;
		MCresult->clear(False);
	}
	else
	{
		MCresult->sets(MCcancelstring);
        r_chosen = false;
	}

    // SN-2014-07-23: [[ Bug 12901 ]] Object colors not selectable in inspector
    //  A bool is expected from MCS_color, not an int defaulting to 0.
	return true;
}

// MERG-2013-08-18: Stubs for colorDialogColors. Possibly implement when color dialog moves to Cocoa
void MCA_getcolordialogcolors(MCColor*& r_list, uindex_t& r_count)
{
    r_count = 0;
}

void MCA_setcolordialogcolors(MCColor* p_list, uindex_t p_count)
{
    
}

////////////////////////////////////////////////////////////////////////////////
