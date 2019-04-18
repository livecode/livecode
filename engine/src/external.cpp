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


#include "param.h"
#include "handler.h"
#include "license.h"
#include "util.h"
#include "mcerror.h"
#include "osspec.h"
#include "globals.h"

#include "external.h"

////////////////////////////////////////////////////////////////////////////////

MCExternalHandlerList::MCExternalHandlerList(void)
{
}

MCExternalHandlerList::~MCExternalHandlerList(void)
{
	for(uint32_t i = 0; i < m_externals . Count(); i++)
		MCExternal::Unload(m_externals[i]);

	for(uint32_t i = 0; i < m_handlers . Count(); i++)
		MCValueRelease(m_handlers[i] . name);
}

bool MCExternalHandlerList::IsEmpty(void)
{
	return m_handlers . Count() == 0;
}

bool MCExternalHandlerList::ListExternals(MCStringRef& r_list)
{
	bool t_success;
	t_success = true;

	MCAutoListRef t_external_list;

	if (t_success)
		t_success = MCListCreateMutable('\n', &t_external_list);

	for(uindex_t i = 0; i < m_externals . Count(); i++)
	{
		MCAutoStringRef t_name_string;

		if (t_success)
			t_success = MCStringCreateWithCString(m_externals[i] -> GetName(), &t_name_string);
		if (t_success)
			t_success = MCListAppend(*t_external_list, *t_name_string);
	}

	if (t_success)
		t_success = MCListCopyAsString(*t_external_list, r_list);

	return t_success;
}

bool MCExternalHandlerList::ListHandlers(Handler_type p_type, MCStringRef& r_list)
{
	bool t_success;
	t_success = true;

	MCAutoListRef t_handler_list;

	if (t_success)
		t_success = MCListCreateMutable('\n', &t_handler_list);

	for(uindex_t i = 0; i < m_handlers . Count(); i++)
	{
		if (t_success && m_externals[m_handlers[i] . external] -> GetHandlerType(m_handlers[i] . handler) == p_type)
			t_success = MCListAppend(*t_handler_list, m_handlers[i] . name);
	}

	if (t_success)
		t_success = MCListCopyAsString(*t_handler_list, r_list);

	return t_success;
}

bool MCExternalHandlerList::Load(MCStringRef p_external)
{
	bool t_success;
	t_success = true;

	// MW-2012-08-29: [[ Bug 10314 ]] Return immediately if we fail to
	//   load the external otherwise handlers get removed that shouldn't.
	
	MCExternal *t_external;
	t_external = nil;
	if (t_success)
	{
		t_external = MCExternal::Load(p_external);
		if (t_external == nil)
			return false;
	}

	if (t_success)
	{
		t_external -> ListHandlers(AddHandler, this);
		t_success = m_externals . Append(t_external);
	}

	if (!t_success)
	{
		for(uindex_t i = 0; i < m_handlers . Count(); i++)
			if (m_handlers[i] . external == m_externals . Count() - 1)
				m_handlers . Remove(i);
		MCExternal::Unload(t_external);
	}

	return t_success;
}

bool MCExternalHandlerList::Lookup(MCNameRef p_name, uindex_t& r_index)
{
	uintptr_t t_name_key;
	t_name_key = MCNameGetCaselessSearchKey(p_name);

	uint32_t t_low, t_high;
	t_low = 0;
	t_high = m_handlers . Count();
	while(t_low < t_high)
	{
		uint32_t t_mid;
		t_mid = t_low + (t_high - t_low) / 2;

		uintptr_t t_mid_key;
		t_mid_key = MCNameGetCaselessSearchKey(m_handlers[t_mid] . name);

		compare_t d;
		d = MCCompare(t_name_key, t_mid_key);
		if (d < 0)
			t_high = t_mid;
		else if (d > 0)
			t_low = t_mid + 1;
		else
		{
			r_index = t_mid;
			return true;
		}
	}
	r_index = t_low;
	return false;

}

bool MCExternalHandlerList::AddHandler(void *state, Handler_type type, const char *p_name_cstring, uint32_t p_index)
{
	MCExternalHandlerList *self;
	self = (MCExternalHandlerList *)state;

	// Inter the name of the handler.
	MCNewAutoNameRef t_name;
    if (!MCNameCreateWithNativeChars((const char_t*)p_name_cstring, strlen(p_name_cstring), &t_name))
    {
        return false;
    }
	
	// If the handler is already in the table, then do nothing. Note that
	// 't_index' will always be set to the index the handler would be at.
	uindex_t t_index;
	if (self -> Lookup(*t_name, t_index))
		return true;

	// Make the entry.
	MCExternalHandlerListEntry t_entry;
	t_entry . name = *t_name;
	t_entry . external = self -> m_externals . Count();
	t_entry . handler = p_index;
	if (!self -> m_handlers . Insert(t_index, t_entry))
		return false;

	t_name . Take();

	return true;
}

Exec_stat MCExternalHandlerList::Handle(MCObject *p_object, Handler_type p_type, MCNameRef p_message, MCParameter *p_parameters)
{
	// Search for the handler in the handler table, and return not handled
	// if not found.
	uindex_t t_index;
	if (!Lookup(p_message, t_index))
		return ES_NOT_HANDLED;

	// Otherwise, dispatch to the external.
	return m_externals[m_handlers[t_index] . external] -> Handle(p_object, p_type, m_handlers[t_index] . handler, p_parameters);
}

bool MCExternalHandlerList::HasHandler(MCNameRef p_name, Handler_type p_type)
{
	uindex_t t_index;
	if (!Lookup(p_name, t_index))
		return false;
	
	// MW-2012-10-31: [[ Bug ]] Check that the handler type is what we expect.
	return m_externals[m_handlers[t_index] . external] -> GetHandlerType(m_handlers[t_index] . handler) == p_type;
}

////////////////////////////////////////////////////////////////////////////////

extern MCExternal *MCExternalCreateV0(void);
extern MCExternal *MCExternalCreateV1(void);

///////////////////

MCExternal *MCExternal::s_externals = nil;

///////////////////

MCExternal::MCExternal(void)
{
	m_next = nil;
	m_references = 0;
	m_name = nil;
}

MCExternal::~MCExternal(void)
{
}

MCExternal *MCExternal::Load(MCStringRef p_filename)
{
	bool t_success;
	t_success = true;
	
	// Load the referenced module.
	MCSAutoLibraryRef t_module;
	if (t_success)
	{
        &t_module = MCU_library_load(p_filename);
        if (!t_module.IsSet())
        {
            // try a relative path
            MCAutoStringRef t_relative_filename;
            if (MCStringFormat(&t_relative_filename, "./%@", p_filename))
            {
                &t_module = MCU_library_load(*t_relative_filename);
            }
        }
        
        if (!t_module.IsSet())
            t_success = false;
	}

	// Now loop through the loaded externals to see if we are already loaded.
	MCExternal *t_external;
	t_external = nil;
	if (t_success)
		for(t_external = s_externals; t_external != nil; t_external = t_external -> m_next)
			if (MCValueIsEqualTo(*t_module,
                                 *(t_external->m_module)))
				break;

	// If we failed to find the external, then we must try and prepare it.
	if (t_success && t_external == nil)
	{
		// First try and load it as a new style external.
        
		if (MCU_library_lookup(*t_module, MCSTR("MCExternalDescribe")) != nil)
			t_external = MCExternalCreateV1();
		else if (MCU_library_lookup(*t_module, MCSTR("getXtable")) != nil)
			t_external = MCExternalCreateV0();
		
		if (t_external != nil)
		{
			t_external -> m_next = s_externals;
			s_externals = t_external;

			t_external -> m_references = 0;
            t_external -> m_module.Give(t_module.Take());
			t_external -> m_name = nil;
			
			t_success = t_external -> Prepare();
		}
		else
			t_success = false;
	}

	// Now we attempt to initialize the external - if it isn't already initialized.
	// (i.e. if the reference count > 0).
	if (t_success && t_external -> m_references == 0)
		t_success = t_external -> Initialize();

	// Finally, increment the reference count and we are done.
	if (t_success)
		t_external -> m_references += 1;

	return t_success ? t_external : nil;
}

void MCExternal::Unload(MCExternal *p_external)
{
	if (p_external == nil)
		return;
	
	// Decrement the reference count.
	p_external -> m_references -= 1;

	// If the reference count reaches 0, then finalize.
	if (p_external -> m_references == 0)
		p_external -> Finalize();
}

///////////////////

void MCExternal::Cleanup(void)
{
	while(s_externals != NULL)
	{
		MCExternal *t_external;
		t_external = s_externals;
		s_externals = s_externals -> m_next;

		// Delete the external - note that there is no need to finalize/unload
		// as that will already have happened.
		delete t_external;
	}
}

////////////////////////////////////////////////////////////////////////////////

