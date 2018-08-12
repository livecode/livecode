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
#include "debug.h"
#include "handler.h"

#include "mblsyntax.h"
#include "mblcontact.h"

////////////////////////////////////////////////////////////////////////////////

void MCAddressBookExecPickContact(MCExecContext& ctxt)
{
    int32_t r_result = 0;
    MCSystemPickContact(r_result);
    if (r_result > 0)
        ctxt.SetTheResultToNumber(r_result);
    else
        ctxt.SetTheResultToEmpty();
}

void MCAddressBookExecShowContact(MCExecContext& ctxt, int32_t p_contact_id)
{
    int32_t r_result = 0;
    MCSystemShowContact(p_contact_id, r_result);
    if (r_result > 0)
        ctxt.SetTheResultToNumber(r_result);
    else
        ctxt.SetTheResultToEmpty();
}

void MCAddressBookExecCreateContact(MCExecContext& ctxt)
{
    int32_t r_result = 0;
    MCSystemCreateContact(r_result);
    if (r_result > 0)
        ctxt.SetTheResultToNumber(r_result);
    else
        ctxt.SetTheResultToEmpty();
}

void MCAddressBookExecUpdateContact(MCExecContext& ctxt, MCArrayRef p_contact, MCStringRef p_title, MCStringRef p_message, MCStringRef p_alternate_name)
{
    int32_t r_result = 0;
    /* UNCHECKED */ MCSystemUpdateContact(p_contact, p_title, p_message, p_alternate_name, r_result);
    if (r_result > 0)
        ctxt.SetTheResultToNumber(r_result);
    else
        ctxt.SetTheResultToEmpty();
}

void MCAddressBookGetContactData(MCExecContext& ctxt, int32_t p_contact_id, MCArrayRef& r_contact_data)
{
      MCSystemGetContactData(p_contact_id, r_contact_data);
}

void MCAddressBookExecRemoveContact(MCExecContext& ctxt, int32_t p_contact_id)
{
    if (MCSystemRemoveContact(p_contact_id))
		ctxt.SetTheResultToNumber(p_contact_id);
	else
        ctxt.SetTheResultToEmpty();
}

void MCAddressBookExecAddContact(MCExecContext &ctxt, MCArrayRef p_contact)
{
	int32_t t_result = 0;
	/* UNCHECKED */ MCSystemAddContact(p_contact, t_result);
	if (t_result > 0)
		ctxt.SetTheResultToNumber(t_result);
	else
		ctxt.SetTheResultToEmpty();
}

void MCAddressBookExecFindContact(MCExecContext& ctxt, MCStringRef p_contact_name)
{
	MCAutoStringRef t_result;
    MCSystemFindContact(p_contact_name, &t_result);
    if (*t_result != nil)
        ctxt.SetTheResultToValue(*t_result);
    else
        ctxt.SetTheResultToEmpty();
}

////////////////////////////////////////////////////////////////////////////////
