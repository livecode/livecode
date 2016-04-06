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

#ifndef _XANSWER_H
#define _XANSWER_H

extern int MCA_file(MCExecContext& ctxt, MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options);
extern int MCA_ask_file(MCExecContext& ctxt, MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options); //const char *prompt, char *fn, MCExecPoint& ep, Boolean sheet);

extern int MCA_file_with_types(MCExecContext& ctxt, MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options);
extern int MCA_ask_file_with_types(MCExecContext& ctxt, MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options);

extern int MCA_folder(MCExecContext& ctxt, MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, unsigned int p_options);

extern MCPrinterDialogResult MCA_gtk_page_setup (PSPrinterSettings &p_settings) ;
extern MCPrinterDialogResult MCA_gtk_printer_setup ( PSPrinterSettings &p_settings ) ;

#endif
