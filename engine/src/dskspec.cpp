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

#include "stack.h"
#include "execpt.h"
#include "object.h"
#include "globals.h"
#include "param.h"
#include "osspec.h"

void MCS_deleteurl(MCObject *p_target, const char *p_url)
{
	Boolean oldlock = MClockmessages;
	MClockmessages = False;
	MCParameter p1(p_url);
	p_target -> message(MCM_delete_url, &p1, False, True);
	MClockmessages = oldlock;
}

void MCS_loadurl(MCObject *p_target, const char *p_url, const char *p_message)
{
	MCParameter p1(p_url);
	MCParameter p2(p_message);
	p1.setnext(&p2);
	// MW-2006-03-03: I've changed this from False, True to True, True to ensure 'target' is returned correctly for libURL.
	p_target -> message(MCM_load_url, &p1, True, True);
}

void MCS_unloadurl(MCObject *p_target, const char *p_url)
{
	MCParameter p1(p_url);
	p_target -> message(MCM_unload_url, &p1, False, True);
}

void MCS_posttourl(MCObject *p_target, const MCString& p_data, const char *p_url)
{
	MCParameter p1(p_data);
	MCParameter p2(p_url);
	p1.setnext(&p2);
	p_target -> message(MCM_post_url, &p1, False, True);
}

void MCS_putintourl(MCObject *p_target, const MCString& p_data, const char *p_url)
{
	Boolean oldlock = MClockmessages;
	MClockmessages = False;
	MCParameter p1(p_data);
	MCParameter p2(p_url);
	p1.setnext(&p2);
	p_target -> message(MCM_put_url, &p1, False, True);
	MClockmessages = oldlock;
}

void MCS_geturl(MCObject *p_target, const char *p_url)
{
	Boolean oldlock = MClockmessages;
	MClockmessages = False;
	MCParameter p1(p_url);
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

bool MCS_put(MCExecPoint& ep, MCSPutKind p_kind, const MCString& p_data)
{
	ep . setsvalue(p_data);

	switch(p_kind)
	{
	case kMCSPutOutput:
	case kMCSPutBeforeMessage:
        return MCmb -> prepend(ep, False) == ES_NORMAL;
	case kMCSPutIntoMessage:
		return MCmb -> store(ep, False) == ES_NORMAL;
	case kMCSPutAfterMessage:
		return MCmb -> append(ep, False) == ES_NORMAL;

	default:
		break;
	}

	// MW-2012-02-23: [[ PutUnicode ]] If we don't understand the king
	//   then return false (caller can then throw an error).
	return false;
}

// SN-2015-01-16: [[ Bug 14295 ]] Added mode-specific way to get the resources folder
void MCS_getresourcesfolder(MCExecPoint &p_context, bool p_standalone)
{
    bool t_path_found;
    t_path_found = true;
    
    if (p_standalone)
    {
        extern char *MCcmd;
        
        p_context . copysvalue(MCcmd, strrchr(MCcmd, '/') - MCcmd);
        
#ifdef TARGET_PLATFORM_MACOS_X
        extern bool MCS_apply_redirect(char*& x_path, bool p_is_file);
        
        char *t_redirected;
        t_redirected = p_context . getsvalue() . clone();

        MCS_apply_redirect(t_redirected, false);
        p_context . copysvalue(t_redirected);
#endif
    }
    else
    {
        // If we are not in a standalone, we return the folder in which sits 'this stack'
        MCExecPoint ep;
        if (MCdefaultstackptr->getprop(0, P_FILE_NAME, ep, true))
        {
            const char *t_filename;
            t_filename = ep . getsvalue() . getstring();
            p_context . copysvalue(t_filename, strrchr(t_filename, '/') - t_filename);
        }
        else
            t_path_found = false;
    }
    
    if (!t_path_found)
    {
        p_context . clear();
        MCresult -> sets("folder not found");
    }
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

bool MCS_set_session_save_path(const char *p_path)
{
	return true;
}

const char *MCS_get_session_save_path(void)
{
	return "";
}

bool MCS_set_session_lifetime(uint32_t p_lifetime)
{
	return true;
}

uint32_t MCS_get_session_lifetime(void)
{
	return 0;
}

bool MCS_set_session_name(const char *p_name)
{
	return true;
}

const char *MCS_get_session_name(void)
{
	return "";
}

bool MCS_set_session_id(const char *p_id)
{
	return true;
}

const char *MCS_get_session_id(void)
{
	return "";
}
