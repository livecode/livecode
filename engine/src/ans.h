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

#ifndef __MC_ANS__
#define __MC_ANS__

// MW-2005-05-15: Have expanded these to take extra parameters
#define MCA_OPTION_SHEET (1 << 0)
#define MCA_OPTION_PLURAL (1 << 1)
#define MCA_OPTION_RETURN_FILTER (1 << 2)
#define MCA_OPTION_SAVE_DIALOG (1 << 3)
#define MCA_OPTION_FOLDER_DIALOG (1 << 4)

// Display a system file open dialog.
//   p_title - this string should appear in the titlebar
//   p_prompt - this string should appear as a description of what the user should do
//     (if there is no suitable place for this, p_prompt override p_title in the titlebar)
//   p_filter - this string is the filter that should be applied to the files this
//     string is platform specific:
//       UNIX - single wildcard expression
//       Mac OS - list of filetype codes
//       Windows - list of file extensions and names
//   p_initial - the path to start with
//   p_options - options controlling display
//     MCA_OPTION_SHEET - (Mac OS X only) display the dialog as a sheet
//     MCA_OPTION_PLURAL - allow multiple files to be selected (not ASK)
//
// On exit:
//   ep should be a return-delimited list of LiveCode paths to the selected files
//   or empty if the dialog was cancelled
//
// The LiveCode syntax that uses this call is deprecated.
//
extern int MCA_file(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result);
extern int MCA_ask_file(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result); //const char *prompt, char *fn, MCExecPoint& ep, Boolean sheet);

// Display a system file open dialog with a list of file types.
//   p_title - this string should appear in the titlebar
//   p_prompt - this string should appear as a description of what the user should do
//     (if there is no suitable place for this, p_prompt override p_title in the titlebar)
//   p_types - this is an array of file type strings. Each string has the format:
//     <label>|<comma delimited list of extension>|<comma delimited list of Mac OS filetypes>
//     e.g. "All Images|jpg,gif,png|JPEG,GIFf,PNGf"
//   p_type_count - the number of types in the p_types array
//   p_initial - the path to start with
//   p_options - options controlling display
//     MCA_OPTION_SHEET - (Mac OS X only) display the dialog as a sheet
//     MCA_OPTION_PLURAL - allow multiple files to be selected (not ASK)
//     MCA_OPTION_RETURN_FILTER - return the label of the type selected in MCresult
//
// On exit:
//   ep should be a return-delimited list of LiveCode paths to the selected files
//   or empty if the dialog was cancelled
//   If MCA_OPTION_RETURN_FILTER is specified MCresult should contain the label of the
//   filetype in effect when the dialog was closed (but not cancelled).
//
extern int MCA_file_with_types(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result);
extern int MCA_ask_file_with_types(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result);

// Display a system folder selection dialog.
//   p_title - this string should appear in the titlebar
//   p_prompt - this string should appear as a description of what the user should do
//     (if there is no suitable place for this, p_prompt override p_title in the titlebar)
//   p_initial - the path to start with
//   p_options - options controlling display
//     MCA_OPTION_SHEET (Mac OS X only) - display the dialog as a sheet
//
// On exit:
//   ep should contain the LiveCode path of the folder selected, or empty if the
//   dialog was cancelled.
//
extern int MCA_folder(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result);

// Display a system color selection dialog.
//   p_title - this string should appear in the titlebar
//   p_initial - the color to start with
//   sheet (OS X only) - display the dialog as a sheet
//
// On exit:
//   ep should contain the selected color as "red,green,blue" or empty if the dialog
//   was cancelled
//
// Note:
//   if p_initial is empty, take the value of MCpencolor.
//   otherwise parse p_initial using MCscreen -> parsecolor
//
extern bool MCA_color(MCStringRef title, MCColor initial_color, bool as_sheet, bool& r_chosen, MCColor& r_chosen_color);

extern void MCA_getcolordialogcolors(MCColor*& r_list, uindex_t& r_count);

extern void MCA_setcolordialogcolors(MCColor* p_list, uindex_t p_count);

extern void MCA_record(MCExecPoint &ep);

#endif
