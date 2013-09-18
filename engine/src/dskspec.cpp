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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "exec.h"
#include "object.h"
#include "globals.h"
#include "param.h"
#include "osspec.h"

void MCS_deleteurl(MCObject *p_target, MCStringRef p_url)
{
	Boolean oldlock = MClockmessages;
	MClockmessages = False;
	MCParameter p1;
	p1 . setvalueref_argument(p_url);
	p_target -> message(MCM_delete_url, &p1, False, True);
	MClockmessages = oldlock;
}

void MCS_loadurl(MCObject *p_target, MCStringRef p_url, MCNameRef p_message)
{
	MCParameter p1;
	p1 . setvalueref_argument(p_url);
	MCParameter p2;
	p2 . setvalueref_argument(p_message);
	p1.setnext(&p2);
	// MW-2006-03-03: I've changed this from False, True to True, True to ensure 'target' is returned correctly for libURL.
	p_target -> message(MCM_load_url, &p1, True, True);
}

void MCS_unloadurl(MCObject *p_target, MCStringRef p_url)
{
	MCParameter p1;
	p1 . setvalueref_argument(p_url);
	p_target -> message(MCM_unload_url, &p1, False, True);
}

void MCS_posttourl(MCObject *p_target, MCDataRef p_data, MCStringRef p_url)
{
	MCParameter p1;
	p1 . setvalueref_argument(p_data);
	MCParameter p2;
	p2 . setvalueref_argument(p_url);
	p1.setnext(&p2);
	p_target -> message(MCM_post_url, &p1, False, True);
}

void MCS_putintourl(MCObject *p_target, MCDataRef p_data, MCStringRef p_url)
{
	Boolean oldlock = MClockmessages;
	MClockmessages = False;
	MCParameter p1;
	p1 . setvalueref_argument(p_data);
	MCParameter p2;
	p2 . setvalueref_argument(p_url);
	p1.setnext(&p2);
	p_target -> message(MCM_put_url, &p1, False, True);
	MClockmessages = oldlock;
}

void MCS_geturl(MCObject *p_target, MCStringRef p_url)
{
	Boolean oldlock = MClockmessages;
	MClockmessages = False;
	MCParameter p1;
	p1 . setvalueref_argument(p_url);
	p_target -> message(MCM_get_url, &p1, True, True);
	MClockmessages = oldlock;
}

void MCS_set_errormode(MCSErrorMode mode)
{
}

MCSErrorMode MCS_get_errormode(void)
{
	return kMCSErrorModeNone;
}

bool MCS_put(MCExecContext &ctxt, MCSPutKind p_kind, MCStringRef p_data)
{
	ctxt . SetTheResultToValue(p_data);
	
	switch (p_kind)
	{
	case kMCSPutOutput:	
	case kMCSPutUnicodeOutput:
	case kMCSPutBeforeMessage:
	case kMCSPutIntoMessage:
		return MCmb -> set(ctxt, p_data);
	case kMCSPutAfterMessage:
		{
			MCAutoStringRef t_existing;
			MCAutoStringRef t_new;
			MCValueRef t_val = MCmb -> getvalueref();
			if (ctxt . ConvertToString(t_val, &t_existing))
				if (MCStringFormat(&t_new, "%@%@", *t_existing, p_data))
					return MCmb -> set(ctxt, *t_new);
			break;
		}
	default:
		break;
	}
	
	// MW-2012-02-23: [[ PutUnicode ]] If we don't understand the kind
	//   then return false (caller can then throw an error).
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
