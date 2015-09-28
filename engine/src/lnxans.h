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

extern int MCA_file(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options);
extern int MCA_ask_file(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options); //const char *prompt, char *fn, MCExecPoint& ep, Boolean sheet);

extern int MCA_file_with_types(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options);
extern int MCA_ask_file_with_types(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options);

extern int MCA_folder(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_initial, unsigned int p_options);

extern MCPrinterDialogResult MCA_gtk_page_setup (PSPrinterSettings &p_settings) ;
extern MCPrinterDialogResult MCA_gtk_printer_setup ( PSPrinterSettings &p_settings ) ;

#endif
