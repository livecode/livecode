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

#ifndef __MC_OBJECT_PROP_SETS__
#define __MC_OBJECT_PROP_SETS__

////////////////////////////////////////////////////////////////////////////////

class MCObjectPropertySet
{
public:
	MCObjectPropertySet(void)
	{
		m_next = nil;
		m_name = nil;
        /* UNCHECKED */ MCArrayCreateMutable(m_props);
	}

	~MCObjectPropertySet(void)
	{
		MCNameDelete(m_name);
		MCValueRelease(m_props);
	}

	bool hasname(MCNameRef p_name)
	{
		return MCNameIsEqualTo(m_name, p_name, kMCCompareCaseless);
	}

	MCNameRef getname(void)
	{
		return m_name;
	}

	MCObjectPropertySet *getnext(void)
	{
		return m_next;
	}

	void setnext(MCObjectPropertySet *p_next)
	{
		m_next = p_next;
	}

	void changename_nocopy(MCNameRef p_name)
	{
		MCNameDelete(m_name);
		m_name = p_name;
	}

	/* CAN FAIL */ bool clone(MCObjectPropertySet*& r_set);
	/* CAN FAIL */ static bool createwithname_nocopy(MCNameRef p_name, MCObjectPropertySet*& r_set);
	/* CAN FAIL */ static bool createwithname(MCNameRef p_name, MCObjectPropertySet*& r_set);

	//////////

	// List the props in the property set into the ep.
#ifdef LEGACY_EXEC
    bool list(MCExecPoint& ep);
#endif
    bool list(MCStringRef& r_keys);

	// Clear the contents of the propset.
	bool clear(void);

	// Remove any props not in the list in ep.
#ifdef LEGACY_EXEC
//	bool restrict(MCExecPoint& ep);
#endif
    /* WRAPPER */ bool restrict(MCStringRef p_string);
    
	// Copy the prop set into the ep.
#ifdef LEGACY_EXEC
    bool fetch(MCExecPoint& ep);
#endif
    bool fetch(MCArrayRef& r_array);
    
	// Store the contents of the ep as the prop set.
#ifdef LEGACY_EXEC
    bool store(MCExecPoint& ep);
#endif
    bool store(MCArrayRef p_array);

#ifdef LEGACY_EXEC
    // Fetch the given element of the property set into the ep.
    bool fetchelement(MCExecPoint& ep, MCNameRef name);
    // Store the contents of the ep as the given property.
    bool storeelement(MCExecPoint& ep, MCNameRef name);
#endif

    bool fetchelement(MCExecContext& ctxt, MCNameRef p_name, MCValueRef& r_value);
    bool storeelement(MCExecContext& ctxt, MCNameRef p_name, MCValueRef p_value);
    
	//////////
	
	// MW-2013-12-05: [[ UnicodeFileFormat ]] These are the non-unicode propset
	//   pickle routines.
	IO_stat loadprops_new(IO_handle stream);
	IO_stat saveprops_new(IO_handle stream);
	
	// MW-2013-12-05: [[ UnicodeFileFormat ]] These are the non-unicode propset
	//   pickle routines.

	// Returns the (old-style) serialized size of the array. If 'nested_only'
	// is true, then it only counts the size of the props containing arrays.
	uint32_t measure_legacy(bool p_nested_only);
	// Returns true if the prop set has nested arrays.
	bool isnested_legacy(void) const;
	// Load the props from the given stream - this merges the existing array
	// with any we find (since nested and non-nested props are stored separately).
	IO_stat loadprops_legacy(IO_handle stream);
	// Load the array props from the given stream - this merges the existing
	// array with any we find.
	IO_stat loadarrayprops_legacy(MCObjectInputStream& stream);
	// Save the non-array props to the given stream.
	IO_stat saveprops_legacy(IO_handle stream);
	// Save the array props to the given stream.
	IO_stat savearrayprops_legacy(MCObjectOutputStream& stream);

private:
	MCObjectPropertySet *m_next;
	MCNameRef m_name;
	MCArrayRef m_props;
};

////////////////////////////////////////////////////////////////////////////////

#endif
