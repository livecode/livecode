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
#include "stack.h"
#include "image.h"
#include "param.h"
#include "exec.h"

#include "mblsyntax.h"

#include "mblcontact.h"

////////////////////////////////////////////////////////////////////////////////

bool MCContactAddProperty(MCArrayRef p_contact, MCNameRef p_property, MCStringRef p_value)
{   
    return MCArrayStoreValue(p_contact, false, p_property, p_value);
}

bool MCContactAddPropertyWithLabel(MCArrayRef p_contact, MCNameRef p_property, MCNameRef p_label, MCValueRef p_value)
{
    // AL-2015-05-14: [[ Bug 15371 ]] mobileGetContactData fails when there are multiple labels of the same property
    bool t_success;
    t_success = true;
    
	MCValueRef t_element;
	MCValueRef t_array;
    // SN-2014-11-17: [[ Bug 14016 ]] Creates the properties array, if it does not exist for this contact.
	if (!MCArrayFetchValue(p_contact, false, p_property, t_array) || !MCValueIsArray(t_array) || !MCArrayIsMutable((MCArrayRef)t_array))
    {
        MCAutoArrayRef t_property_array;
       
        t_success = MCArrayCreateMutable(&t_property_array);
        
        if (t_success)
            t_success = MCArrayStoreValue(p_contact, false, p_property, *t_property_array);
        
        // Fetch the array (no need to release).
        if (t_success)
            t_success = MCArrayFetchValue(p_contact, false, p_property, t_array);
    }
    
    // SN-2014-11-17:[[ Bug 14016 ]] Create the labels array of this property, if it does not exist already.
    if (t_success && (!MCArrayFetchValue((MCArrayRef)t_array, false, p_label, t_element) || !MCValueIsArray(t_element) || !MCArrayIsMutable((MCArrayRef)t_element)))
    {
        MCAutoArrayRef t_label_array;
        
        if (t_success)
            t_success = MCArrayCreateMutable(&t_label_array);
        
        if (t_success)
            t_success = MCArrayStoreValue((MCArrayRef)t_array, false, p_label, *t_label_array);
        
        // Fetch the array (no need to release).
        if (t_success)
            t_success = MCArrayFetchValue((MCArrayRef)t_array, false, p_label, t_element);
    }
	
    // Stop here if we don't have the element where to store the value.
    if (!t_success)
        return false;
    
	uindex_t t_index = 1;
	if (!MCValueIsArray(t_element))
		t_index = 1;
	else
		t_index = MCArrayGetCount((MCArrayRef)t_element) + 1;
	
    
    return MCArrayStoreValueAtIndex((MCArrayRef)t_element, t_index, p_value);
}

////////////////////////////////////////////////////////////////////////////////

