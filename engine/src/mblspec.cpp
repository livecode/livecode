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

#include "uidc.h"
#include "param.h"
#include "mcerror.h"

#include "exec.h"
#include "util.h"
#include "object.h"
#include "socket.h"
#include "globals.h"
#include "text.h"
#include "system.h"
#include "eventqueue.h"
#include "osspec.h"

////////////////////////////////////////////////////////////////////////////////


bool MCS_put(MCExecContext &ctxt, MCSPutKind p_kind, MCStringRef p_string)
{
	bool t_success;
	switch (p_kind)
	{
		case kMCSPutBeforeMessage:
			t_success = MCmb -> set(ctxt, p_string, kMCVariableSetBefore);
			break;
		case kMCSPutOutput:
		case kMCSPutIntoMessage:
			t_success = MCmb -> set(ctxt, p_string);
			break;
		case kMCSPutAfterMessage:
			t_success = MCmb -> set(ctxt, p_string, kMCVariableSetAfter);
			break;
		default:
			t_success = false;
			break;
	}
	
	return t_success;
}

// Missing implementation. What to write here? Panos.
bool MCS_put_binary(MCExecContext& ctxt, MCSPutKind p_kind, MCDataRef p_data)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void MCS_set_errormode(MCSErrorMode mode)
{
}

MCSErrorMode MCS_get_errormode(void)
{
	return kMCSErrorModeNone;
}

////////////////////////////////////////////////////////////////////////////////

void MCS_set_outputtextencoding(MCSOutputTextEncoding encoding)
{
}

MCSOutputTextEncoding MCS_get_outputtextencoding(void)
{
	return kMCSOutputTextEncodingNative;
}

void MCS_set_outputlineendings(MCSOutputLineEndings ending)
{
}

MCSOutputLineEndings MCS_get_outputlineendings(void)
{
	return kMCSOutputLineEndingsNative;
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_set_session_save_path(MCStringRef p_path)
{
	return true;
}

bool MCS_get_session_save_path(MCStringRef& r_path)
{
	r_path = MCValueRetain(kMCEmptyString);
	return true;
}

bool MCS_set_session_lifetime(uint32_t p_lifetime)
{
	return true;
}

uint32_t MCS_get_session_lifetime(void)
{
	return 0;
}

bool MCS_set_session_name(MCStringRef p_name)
{
	return true;
}

bool MCS_get_session_name(MCStringRef &r_name)
{
	r_name = MCValueRetain(kMCEmptyString);
	return true;
}

bool MCS_set_session_id(MCStringRef p_id)
{
	return true;
}

bool MCS_get_session_id(MCStringRef &r_id)
{
	r_id = MCValueRetain(kMCEmptyString);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

int MCA_file(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	return 0;
}

int MCA_ask_file(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	return 0;
}

int MCA_file_with_types(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	return 0;
}

int MCA_ask_file_with_types(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	return 0;
}

int MCA_folder(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	return 0;
}

bool MCA_color(MCStringRef title, MCColor initial_color, bool as_sheet, bool& r_chosen, MCColor& r_chosen_color)
{
	return true;
}

// MERG-2013-08-18: Stubs for colorDialogColors.
void MCA_setcolordialogcolors(MCColor* p_colors, uindex_t p_count)
{
    
}

void MCA_getcolordialogcolors(MCColor*& r_colors, uindex_t& r_count)
{
	r_count = 0;
}


////////////////////////////////////////////////////////////////////////////////

