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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "execpt.h"
#include "stack.h"
#include "ans.h"

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

////////////////////////////////////////////////////////////////////////////////

// MM-2012-02-13: Updated to use Cocoa APIs.  Code mostly cribbed from plugin dialog stuff
int MCA_folder(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_initial, unsigned int p_options)
{
	MCPlatformWindowRef t_owner;
    t_owner = compute_sheet_owner(p_options);
	
	MCPlatformBeginFolderDialog(t_owner, p_title, p_prompt, p_initial);
	
	MCPlatformDialogResult t_result;
	char *t_folder;
	t_folder = nil;
	for(;;)
	{
		t_result = MCPlatformEndFolderDialog(t_folder);
		if (t_result != kMCPlatformDialogResultContinue)
			break;
		
		MCscreen -> wait(REFRESH_INTERVAL, True, True);
	}
	
	if (t_result == kMCPlatformDialogResultSuccess)
		ep . copysvalue(t_folder);
	else
		ep . clear();
	
	free(t_folder);
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

static bool filter_to_type_list(const char *p_filter, char**&r_types, uint32_t &r_type_count)
{
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = p_filter != nil;
	
	uint32_t t_filter_length;
	if (t_success)
	{
		t_filter_length = MCCStringLength(p_filter);
		t_success = t_filter_length >= 4;
	}
	
	char *t_types;
	t_types = nil;
	if (t_success)
		t_success = MCCStringClone("||", t_types);
	
	if (t_success)
	{
		const char *t_current_type;
		t_current_type = p_filter;
		for (uint32_t i = 0; t_success && i < t_filter_length / 4; i++)
		{
			if (i > 0)
				t_success = MCCStringAppend(t_types, ",");
			if (t_success)
			{
				t_success = MCCStringAppendFormat(t_types, "%c%c%c%c", t_current_type[0], t_current_type[1], t_current_type[2], t_current_type[3]);
				t_current_type += 4;
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
		/* UNCHECKED */ MCCStringFree(t_types);
	}		
	
	return t_success;	
}

int MCA_file(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options)
{
	char **t_types;
	uint32_t t_type_count;
	t_types = nil;
	filter_to_type_list(p_filter, t_types, t_type_count);
	
	MCPlatformWindowRef t_owner;
    t_owner = compute_sheet_owner(p_options);
	
	MCPlatformFileDialogKind t_kind;
	if ((p_options & MCA_OPTION_PLURAL) != 0)
		t_kind = kMCPlatformFileDialogKindOpenMultiple;
	else
		t_kind = kMCPlatformFileDialogKindOpen;
	
	MCPlatformBeginFileDialog(t_kind, t_owner, p_title, p_prompt, t_types, t_type_count, p_initial);
	
	MCPlatformDialogResult t_result;
	char *t_file, *t_type;
	t_file = nil;
	t_type = nil;
	for(;;)
	{
		t_result = MCPlatformEndFileDialog(t_kind, t_file, t_type);
		if (t_result != kMCPlatformDialogResultContinue)
			break;
		
		MCscreen -> wait(REFRESH_INTERVAL, True, True);
	}
	
	if (t_result == kMCPlatformDialogResultSuccess)
		ep . copysvalue(t_file);
	else
		ep . clear();
	
	free(t_file);
	free(t_type);
	/* UNCHECKED */ MCCStringArrayFree(t_types, t_type_count);
	
	return 0;
}

int MCA_file_with_types(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options)
{
	MCPlatformWindowRef t_owner;
    t_owner = compute_sheet_owner(p_options);
	
	MCPlatformFileDialogKind t_kind;
	if ((p_options & MCA_OPTION_PLURAL) != 0)
		t_kind = kMCPlatformFileDialogKindOpenMultiple;
	else
		t_kind = kMCPlatformFileDialogKindOpen;
	
	MCPlatformBeginFileDialog(t_kind, t_owner, p_title, p_prompt, p_types, p_type_count, p_initial);
	
	MCPlatformDialogResult t_result;
	char *t_file, *t_type;
	t_file = nil;
	t_type = nil;
	for(;;)
	{
		t_result = MCPlatformEndFileDialog(t_kind, t_file, t_type);
		if (t_result != kMCPlatformDialogResultContinue)
			break;
		
		MCscreen -> wait(REFRESH_INTERVAL, True, True);
	}
	
	if (t_result == kMCPlatformDialogResultSuccess)
	{
		ep . copysvalue(t_file);
		if (t_type != nil)
			MCresult -> copysvalue(t_type);
	}
	else
		ep . clear();
	
	free(t_file);
	free(t_type);
	
	return 0;
	
}
int MCA_ask_file(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options)
{
	char **t_types;
	uint32_t t_type_count;
	t_types = nil;
	filter_to_type_list(p_filter, t_types, t_type_count);
	
	MCPlatformWindowRef t_owner;
    t_owner = compute_sheet_owner(p_options);
	
	MCPlatformBeginFileDialog(kMCPlatformFileDialogKindSave, t_owner, p_title, p_prompt, t_types, t_type_count, p_initial);
	
	MCPlatformDialogResult t_result;
	char *t_file, *t_type;
	t_file = nil;
	t_type = nil;
	for(;;)
	{
		t_result = MCPlatformEndFileDialog(kMCPlatformFileDialogKindSave, t_file, t_type);
		if (t_result != kMCPlatformDialogResultContinue)
			break;
		
		MCscreen -> wait(REFRESH_INTERVAL, True, True);
	}
	
	if (t_result == kMCPlatformDialogResultSuccess)
		ep . copysvalue(t_file);
	else
		ep . clear();
	
	free(t_file);
	free(t_type);
	/* UNCHECKED */ MCCStringArrayFree(t_types, t_type_count);
	
	return 0;
}

int MCA_ask_file_with_types(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options)
{
	MCPlatformWindowRef t_owner;
    t_owner = compute_sheet_owner(p_options);
	
	MCPlatformBeginFileDialog(kMCPlatformFileDialogKindSave, t_owner, p_title, p_prompt, p_types, p_type_count, p_initial);
	
	MCPlatformDialogResult t_result;
	char *t_file, *t_type;
	t_file = nil;
	t_type = nil;
	for(;;)
	{
		t_result = MCPlatformEndFileDialog(kMCPlatformFileDialogKindSave, t_file, t_type);
		if (t_result != kMCPlatformDialogResultContinue)
			break;
		
		MCscreen -> wait(REFRESH_INTERVAL, True, True);
	}
	
	if (t_result == kMCPlatformDialogResultSuccess)
	{
		ep . copysvalue(t_file);
		if (t_type != nil)
			MCresult -> copysvalue(t_type);
	}
	else
		ep . clear();
	
	free(t_file);
	free(t_type);
	
	return 0;
	
}

////////////////////////////////////////////////////////////////////////////////

int MCA_color(MCExecPoint& ep, const char *p_title, const char *p_initial, Boolean p_as_sheet)
{
	MCColor t_color;
	if (p_initial == NULL)
		t_color = MCpencolor;
	else
	{
		char *cname = NULL;
		MCscreen->parsecolor(p_initial, &t_color, &cname);
		delete cname;
	}
	
	MCPlatformBeginColorDialog(p_title, t_color);
	
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
		ep.setcolor(t_new_color);
		MCresult->clear(False);
	}
	else
	{
		ep.clear();
		MCresult->sets(MCcancelstring);
	}

	return 0;
}

// MERG-2013-08-18: Stubs for colorDialogColors. Possibly implement when color dialog moves to Cocoa
void MCA_setcolordialogcolors(MCExecPoint& p_ep)
{
    
}

void MCA_getcolordialogcolors(MCExecPoint& p_ep)
{
	p_ep.clear();
}

////////////////////////////////////////////////////////////////////////////////
