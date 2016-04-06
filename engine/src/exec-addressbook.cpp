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

MC_EXEC_DEFINE_EXEC_METHOD(AddressBook, PickContact, 0)
MC_EXEC_DEFINE_EXEC_METHOD(AddressBook, ShowContact, 1)
MC_EXEC_DEFINE_EXEC_METHOD(AddressBook, CreateContact, 0)
MC_EXEC_DEFINE_EXEC_METHOD(AddressBook, UpdateContact, 4)
MC_EXEC_DEFINE_GET_METHOD(AddressBook, ContactData, 2)
MC_EXEC_DEFINE_EXEC_METHOD(AddressBook, RemoveContact, 1)
MC_EXEC_DEFINE_EXEC_METHOD(AddressBook, AddContact, 0)
MC_EXEC_DEFINE_EXEC_METHOD(AddressBook, FindContact, 4)

////////////////////////////////////////////////////////////////////////////////

void MCAddressBookExecPickContact(MCExecContext& ctxt)
{
#ifdef /* MCPickContactExec */ LEGACY_EXEC
    int32_t r_result = 0;
    MCSystemPickContact(r_result);
    if (r_result > 0)
        p_ctxt.SetTheResultToNumber(r_result);
    else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCPickContactExec */
    int32_t r_result = 0;
    MCSystemPickContact(r_result);
    if (r_result > 0)
        ctxt.SetTheResultToNumber(r_result);
    else
        ctxt.SetTheResultToEmpty();
}

void MCAddressBookExecShowContact(MCExecContext& ctxt, int32_t p_contact_id)
{
#ifdef /* MCShowContactExec */ LEGACY_EXEC
    int32_t r_result = 0;
    MCSystemShowContact(p_contact_id, r_result);
    if (r_result > 0)
        p_ctxt.SetTheResultToNumber(r_result);
    else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCShowContactExec */
    int32_t r_result = 0;
    MCSystemShowContact(p_contact_id, r_result);
    if (r_result > 0)
        ctxt.SetTheResultToNumber(r_result);
    else
        ctxt.SetTheResultToEmpty();
}

void MCAddressBookExecCreateContact(MCExecContext& ctxt)
{
#ifdef /* MCCreateContactExec */ LEGACY_EXEC
    int32_t r_result = 0;
    MCSystemCreateContact(r_result);
    if (r_result > 0)
        p_ctxt.SetTheResultToNumber(r_result);
    else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCCreateContactExec */
    int32_t r_result = 0;
    MCSystemCreateContact(r_result);
    if (r_result > 0)
        ctxt.SetTheResultToNumber(r_result);
    else
        ctxt.SetTheResultToEmpty();
}

void MCAddressBookExecUpdateContact(MCExecContext& ctxt, MCArrayRef p_contact, MCStringRef p_title, MCStringRef p_message, MCStringRef p_alternate_name)
{
#ifdef /* MCUpdateContactExec */ LEGACY_EXEC
    int32_t r_result = 0;
    /* UNCHECKED */ MCSystemUpdateContact(p_contact, p_title, p_message, p_alternate_name, r_result);
    if (r_result > 0)
        p_ctxt.SetTheResultToNumber(r_result);
    else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCUpdateContactExec */
    int32_t r_result = 0;
    /* UNCHECKED */ MCSystemUpdateContact(p_contact, p_title, p_message, p_alternate_name, r_result);
    if (r_result > 0)
        ctxt.SetTheResultToNumber(r_result);
    else
        ctxt.SetTheResultToEmpty();
}

void MCAddressBookGetContactData(MCExecContext& ctxt, int32_t p_contact_id, MCArrayRef& r_contact_data)
{
#ifdef /* MCGetContactDataExec */ LEGACY_EXEC
    MCVariableValue *r_contact_data = nil;
    MCSystemGetContactData(p_ctxt, p_contact_id, r_contact_data);
    if (r_contact_data == nil)
        p_ctxt.SetTheResultToEmpty();
    else
        p_ctxt.GetEP().setarray(r_contact_data, True);
#endif /* MCGetContactDataExec */
      MCSystemGetContactData(p_contact_id, r_contact_data);
}

void MCAddressBookExecRemoveContact(MCExecContext& ctxt, int32_t p_contact_id)
{
#ifdef /* MCRemoveContactExec */ LEGACY_EXEC
    if (MCSystemRemoveContact(p_contact_id))
		p_ctxt.SetTheResultToNumber(p_contact_id);
	else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCRemoveContactExec */
    if (MCSystemRemoveContact(p_contact_id))
		ctxt.SetTheResultToNumber(p_contact_id);
	else
        ctxt.SetTheResultToEmpty();
}

void MCAddressBookExecAddContact(MCExecContext &ctxt, MCArrayRef p_contact)
{
#ifdef /* MCAddContactExec */ LEGACY_EXEC
	int32_t t_result = 0;
	/* UNCHECKED */ MCSystemAddContact(p_contact, t_result);
	if (t_result > 0)
		ctxt.SetTheResultToNumber(t_result);
	else
		ctxt.SetTheResultToEmpty();
#endif /* MCAddContactExec */
	int32_t t_result = 0;
	/* UNCHECKED */ MCSystemAddContact(p_contact, t_result);
	if (t_result > 0)
		ctxt.SetTheResultToNumber(t_result);
	else
		ctxt.SetTheResultToEmpty();
}

void MCAddressBookExecFindContact(MCExecContext& ctxt, MCStringRef p_contact_name)
{
#ifdef /* MCFindContactExec */ LEGACY_EXEC
    char *t_result;
    t_result = nil;
    MCSystemFindContact(p_contact_name, t_result);
    if (t_result != nil)
        p_ctxt.SetTheResultToCString(t_result);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
#endif /* MCFindContactExec */
	MCAutoStringRef t_result;
    MCSystemFindContact(p_contact_name, &t_result);
    if (*t_result != nil)
        ctxt.SetTheResultToValue(*t_result);
    else
        ctxt.SetTheResultToEmpty();
}

////////////////////////////////////////////////////////////////////////////////
