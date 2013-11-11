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

////////////////////////////////////////////////////////////////////////////////
/*
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
*/

bool MCContactAddProperty(MCArrayRef p_contact, MCNameRef p_property, MCStringRef p_value)
{
#ifdef /* MCContactAddProperty */ LEGACY_EXEC
	MCVariableValue *t_element;
	return p_contact->lookup_element(ep, MCNameGetOldString(p_property), t_element) == ES_NORMAL &&
    t_element->assign_string(p_value);
#endif /* MCContactAddProperty */
   
    return MCArrayStoreValue(p_contact, false, p_property, p_value);
}

bool MCContactAddPropertyWithLabel(MCArrayRef p_contact, MCNameRef p_property, MCNameRef p_label, MCValueRef p_value)
{   
	MCValueRef t_element;
	MCValueRef t_array;
	if (!MCArrayFetchValue(p_contact, false, p_property, t_array) ||
		!MCValueIsArray(t_array) ||
		!MCArrayFetchValue((MCArrayRef)t_array, false, p_label, t_array))
		return false;
	
	uindex_t t_index = 1;
	if (!MCValueIsArray(t_array))
		t_index = 1;
	else
		t_index = MCArrayGetCount((MCArrayRef)t_array) + 1;
	
    MCAutoArrayRef t_copied_array;
    if (MCArrayCopy((MCArrayRef) t_array, &t_copied_array))
        return MCArrayStoreValueAtIndex(*t_copied_array, t_index, p_value);
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

