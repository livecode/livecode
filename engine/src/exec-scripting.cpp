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

#include "globals.h"
#include "util.h"

#include "osspec.h"
#include "securemode.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

void MCScriptingEvalAlternateLanguages(MCExecContext& ctxt, MCStringRef& r_list)
{
	if (!MCSecureModeCanAccessDoAlternate())
	{
		r_list = MCValueRetain(kMCEmptyString);
		return;
	}
	
	MCAutoListRef t_list;
	if (MCS_alternatelanguages(&t_list) && MCListCopyAsString(*t_list, r_list))
		return;
	
	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCScriptingExecDoAsAlternateLanguage(MCExecContext& ctxt, MCStringRef p_script, MCStringRef p_language)
{
	if (MCSecureModeCheckDoAlternate())
	{
		MCS_doalternatelanguage(p_script, p_language);
		return;
	}

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCScriptingExecSendToProgram(MCExecContext& ctxt, MCStringRef p_message, MCStringRef p_program, MCStringRef p_event_type, bool p_wait_for_reply)
{
	if (p_event_type != nil &&
		MCStringGetLength(p_event_type) != 8)
	{
		ctxt . LegacyThrow(EE_SEND_BADEXP);
		return;
	}
	
	if (MCsecuremode & MC_SECUREMODE_PROCESS)
	{
		ctxt . LegacyThrow(EE_PROCESS_NOPERM);
		return;
	}
	
	MCS_send(p_message, p_program, p_event_type, p_wait_for_reply);
}

void MCScriptingExecReply(MCExecContext& ctxt, MCStringRef message, MCStringRef keyword)
{
	MCS_reply(message, keyword != nil ? keyword : nil, False);
}

void MCScriptingExecReplyError(MCExecContext& ctxt, MCStringRef message)
{
	MCS_reply(message, nil, True);
}

void MCScriptingExecRequestAppleEvent(MCExecContext& ctxt, int p_type, MCStringRef p_program)
{
	MCAutoStringRef t_result_value;
	if (p_program != nil)
		MCS_request_ae(p_program, p_type, &t_result_value);
	else
		MCS_request_ae(kMCEmptyString, p_type, &t_result_value);
	
	ctxt . SetItToValue(*t_result_value);
}

void MCScriptingExecRequestFromProgram(MCExecContext& ctxt, MCStringRef p_message, MCStringRef p_program)
{
	MCAutoStringRef t_result;
	MCS_request_program(p_message, p_program, &t_result);
	
	ctxt . SetItToValue(*t_result);
}

////////////////////////////////////////////////////////////////////////////////
