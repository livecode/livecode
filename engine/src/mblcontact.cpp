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
#include "mcio.h"

#include "globals.h"
#include "stack.h"
#include "image.h"
#include "param.h"
#include "exec.h"

#include "mblsyntax.h"

#include "mblcontact.h"

void MCPickContactExec(MCExecContext& p_ctxt)
{
#ifdef /* MCPickContactExec */ LEGACY_EXEC
    int32_t r_result = 0;
    MCSystemPickContact(r_result);
    if (r_result > 0)
        p_ctxt.SetTheResultToNumber(r_result);
    else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCPickContactExec */
}

void MCShowContactExec(MCExecContext& p_ctxt, int32_t p_contact_id)
{
#ifdef /* MCShowContactExec */ LEGACY_EXEC
    int32_t r_result = 0;
    MCSystemShowContact(p_contact_id, r_result);
    if (r_result > 0)
        p_ctxt.SetTheResultToNumber(r_result);
    else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCShowContactExec */
}

void MCCreateContactExec(MCExecContext& p_ctxt)
{
#ifdef /* MCCreateContactExec */ LEGACY_EXEC
    int32_t r_result = 0;
    MCSystemCreateContact(r_result);
    if (r_result > 0)
        p_ctxt.SetTheResultToNumber(r_result);
    else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCCreateContactExec */
}

void MCUpdateContactExec(MCExecContext& p_ctxt, MCVariableValue *p_contact, const char *p_title, const char *p_message, const char *p_alternate_name)
{
#ifdef /* MCUpdateContactExec */ LEGACY_EXEC
    int32_t r_result = 0;
    /* UNCHECKED */ MCSystemUpdateContact(p_contact, p_title, p_message, p_alternate_name, r_result);
    if (r_result > 0)
        p_ctxt.SetTheResultToNumber(r_result);
    else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCUpdateContactExec */
}

void MCGetContactDataExec(MCExecContext& p_ctxt, int32_t p_contact_id)
{
#ifdef /* MCGetContactDataExec */ LEGACY_EXEC
    MCVariableValue *r_contact_data = nil;
    MCSystemGetContactData(p_ctxt, p_contact_id, r_contact_data);
    if (r_contact_data == nil)
        p_ctxt.SetTheResultToEmpty();
    else
        p_ctxt.GetEP().setarray(r_contact_data, True);
#endif /* MCGetContactDataExec */
}

void MCRemoveContactExec(MCExecContext& p_ctxt, int32_t p_contact_id)
{
#ifdef /* MCRemoveContactExec */ LEGACY_EXEC
    if (MCSystemRemoveContact(p_contact_id))
		p_ctxt.SetTheResultToNumber(p_contact_id);
	else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCRemoveContactExec */
}

void MCAddContactExec(MCExecContext &ctxt, MCVariableValue *p_contact)
{
#ifdef /* MCAddContactExec */ LEGACY_EXEC
	int32_t t_result = 0;
	/* UNCHECKED */ MCSystemAddContact(p_contact, t_result);
	if (t_result > 0)
		ctxt.SetTheResultToNumber(t_result);
	else
		ctxt.SetTheResultToEmpty();
#endif /* MCAddContactExec */
}

void MCFindContactExec(MCExecContext& p_ctxt, const char* p_contact_name)
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
}

////////////////////////////////////////////////////////////////////////////////

bool MCContactAddProperty(MCExecPoint& ep, MCVariableValue *p_contact, MCNameRef p_property, MCString p_value)
{
#ifdef /* MCContactAddProperty */ LEGACY_EXEC
	MCVariableValue *t_element;
	return p_contact->lookup_element(ep, MCNameGetOldString(p_property), t_element) == ES_NORMAL &&
		t_element->assign_string(p_value);
#endif /* MCContactAddProperty */
}

bool MCContactAddPropertyWithLabel(MCExecPoint& ep, MCVariableValue *p_contact, MCNameRef p_property, MCNameRef p_label, MCVariableValue *p_value)
{
	MCVariableValue *t_element;
	MCVariableValue *t_array;
	if (p_contact->lookup_element(ep, MCNameGetOldString(p_property), t_array) != ES_NORMAL ||
		t_array->lookup_element(ep, MCNameGetOldString(p_label), t_array) != ES_NORMAL)
		return false;
	
	uindex_t t_index = 1;
	if (!t_array->is_array())
		t_index = 1;
	else
		t_index = t_array->get_array()->getnfilled() + 1;
	
	return t_array->lookup_index(ep, t_index, t_element) == ES_NORMAL &&
		t_element->assign(*p_value);
}

bool MCContactAddPropertyWithLabel(MCExecPoint& ep, MCVariableValue *p_contact, MCNameRef p_property, MCNameRef p_label, MCString p_value)
{
	bool t_success = true;
	MCVariableValue *t_element;
	t_element = new MCVariableValue();
	
	t_success = t_element != nil &&
		t_element->assign_string(p_value) &&
		MCContactAddPropertyWithLabel(ep, p_contact, p_property, p_label, t_element);
	delete t_element;
	return t_success;
}

bool MCContactParseParams(MCParameter *p_params, MCVariableValue *&r_contact, char *&r_title, char *&r_message, char *&r_alternate_name)
{
	bool t_success = true;
	
	MCVariableValue *t_contact = nil;
	char *t_title = nil;
	char *t_message = nil;
	char *t_alternate_name = nil;
	
	t_success = MCParseParameters(p_params, "a|sss", &t_contact, &t_title, &t_message, &t_alternate_name);
	
	if (t_success)
	{
		r_contact = t_contact;
		r_title = t_title;
		r_message = t_message;
		r_alternate_name = t_alternate_name;
	}
	else
	{
		MCCStringFree(t_title);
		MCCStringFree(t_message);
		MCCStringFree(t_alternate_name);
	}
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandlePickContact(void *context, MCParameter *p_parameters) // ABPeoplePickerNavigationController
{
#ifdef /* MCHandlePickContact */ LEGACY_EXEC
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCPickContactExec(t_ctxt);
    // Set return value
	return t_ctxt.GetStat();
#endif /* MCHandlePickContact */
}

Exec_stat MCHandleShowContact(void *context, MCParameter *p_parameters) // ABPersonViewController
{
#ifdef /* MCHandleShowContact */ LEGACY_EXEC
    int32_t t_contact_id = 0;
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_id = atoi (ep.getsvalue().getstring());
    }
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCShowContactExec(t_ctxt, t_contact_id);
    // Set return value
	return t_ctxt.GetStat();
#endif /* MCHandleShowContact */
}

Exec_stat MCHandleCreateContact(void *context, MCParameter *p_parameters) // ABNewPersonViewController
{
#ifdef /* MCHandleCreateContact */ LEGACY_EXEC
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCCreateContactExec(t_ctxt);
    // Set return value
	return t_ctxt.GetStat();
#endif /* MCHandleCreateContact */
}

Exec_stat MCHandleUpdateContact(void *context, MCParameter *p_parameters) // ABUnknownPersonViewController
{
#ifdef /* MCHandleUpdateContact */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    // Handle parameters. We are doing that in a dedicated call
	MCVariableValue *t_contact = nil;
	char *t_title = nil;
	char *t_message = nil;
	char *t_alternate_name = nil;
	/* UNCHECKED */ MCContactParseParams(p_parameters, t_contact, t_title, t_message, t_alternate_name);
    // Call the Exec implementation
    MCUpdateContactExec(ctxt, t_contact, t_title, t_message, t_alternate_name);
    // Set return value
	return ctxt.GetStat();
#endif /* MCHandleUpdateContact */
}

Exec_stat MCHandleGetContactData(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetContactData */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    int32_t t_contact_id = 0;
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_id = atoi (ep.getsvalue().getstring());
    }
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCGetContactDataExec(t_ctxt, t_contact_id);
    if (MCresult->isempty())
        MCresult->store(ep, True);
	return t_ctxt.GetStat();
#endif /* MCHandleGetContactData */
}

Exec_stat MCHandleRemoveContact(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleRemoveContact */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    int32_t t_contact_id = 0;
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_id = atoi (ep.getsvalue().getstring());
    }
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCRemoveContactExec(t_ctxt, t_contact_id);
    // Set return value
    return t_ctxt.GetStat();
#endif /* MCHandleRemoveContact */
}

Exec_stat MCHandleAddContact(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleAddContact */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    // Handle parameters. We are doing that in a dedicated call
	MCVariableValue *t_contact;
	
	/* UNCHECKED */ MCParseParameters(p_parameters, "a", &t_contact);

    MCExecContext t_ctxt(ep);
    // Call the Exec implementation
    MCAddContactExec(t_ctxt, t_contact);
    // Set return value
    return t_ctxt.GetStat();
#endif /* MCHandleAddContact */
}

Exec_stat MCHandleFindContact(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleFindContact */ LEGACY_EXEC
    const char *t_contact_name = NULL;
    const char *r_result = NULL;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_name = ep.getcstring();
    }
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCFindContactExec(t_ctxt, t_contact_name);
    // Set return value
    return t_ctxt.GetStat();
#endif /* MCHandleFindContact */
}
