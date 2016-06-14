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


#include "exec.h"
#include "stack.h"
#include "object.h"
#include "globals.h"
#include "param.h"
#include "osspec.h"

void MCS_deleteurl(MCObject *p_target, MCStringRef p_url)
// SJT-2014-09-10: [[ URLMessages ]] Send "deleteURL" messages on all platforms.
{
  // Should have been processed via the "deleteURL" message.
}

void MCS_loadurl(MCObject *p_target, MCStringRef p_url, MCNameRef p_message)
// SJT-2014-09-11: [[ URLMessages ]] Send "loadURL" messages on all platforms.
{
  // Should have been processed via the "loadURL" message.
}

void MCS_unloadurl(MCObject *p_target, MCStringRef p_url)
// SJT-2014-09-11: [[ URLMessages ]] Send "unloadURL" messages on all platforms.
{
  // Should have been processed via the "unloadURL" message.
}

void MCS_posttourl(MCObject *p_target, MCDataRef p_data, MCStringRef p_url)
// SJT-2014-09-11: [[ URLMessages ]] Send "postURL" messages on all platforms.
{
  // Should have been processed via the "postURL" message.
}

void MCS_putintourl(MCObject *p_target, MCDataRef p_data, MCStringRef p_url)
// SJT-2014-09-10: [[ URLMessages ]] Send "putURL" messages on all platforms.
{
  // Should have been processed via the "putURL" message.
}

void MCS_geturl(MCObject *p_target, MCStringRef p_url)
// SJT-2014-09-10: [[ URLMessages ]] Send "getURL" messages on all platforms.
{
  // Should have been processed via the "getURL" message.
}

void MCS_set_errormode(MCSErrorMode mode)
{
}

MCSErrorMode MCS_get_errormode(void)
{
	return kMCSErrorModeNone;
}

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
        // SN-2014-04-11 [[ FasterVariable ]] parameter updated to use the new 'set' operation on variables
        t_success = MCmb -> set(ctxt, p_string, kMCVariableSetAfter);
        break;
	default:
        t_success = false;
		break;
	}
    
	// MW-2012-02-23: [[ PutUnicode ]] If we don't understand the kind
	//   then return false (caller can then throw an error).
	return t_success;
}

// Missing implementation. What to write here? Panos.
bool MCS_put_binary(MCExecContext& ctxt, MCSPutKind p_kind, MCDataRef p_data)
{
    return false;
}

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
