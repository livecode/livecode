/* Copyright (C) 2003-2017 LiveCode Ltd.

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
	}

	bool hasname(MCNameRef p_name) const
	{
        return m_name.IsSet() &&
            MCNameIsEqualToCaseless(*m_name, p_name);
	}

	MCNameRef getname(void) const
	{
		return *m_name;
	}

	MCObjectPropertySet *getnext(void) const
	{
		return m_next;
	}

	void setnext(MCObjectPropertySet *p_next)
	{
		m_next = p_next;
	}

	void changename_nocopy(MCNameRef p_name)
	{
        m_name.Give(p_name);
	}

	/* CAN FAIL */ bool clone(MCObjectPropertySet*& r_set) const;
	/* CAN FAIL */ static bool createwithname_nocopy(MCNameRef p_name, MCObjectPropertySet*& r_set);
	/* CAN FAIL */ static bool createwithname(MCNameRef p_name, MCObjectPropertySet*& r_set);

	//////////

	// List the props in the property set into the ep.
    bool list(MCStringRef& r_keys) const;

	// Clear the contents of the propset.
	bool clear(void);

	// Remove any props not in the list in ep.
    /* WRAPPER */ bool restrict(MCStringRef p_string);
    
	// Copy the prop set into the ep.
    bool fetch(MCArrayRef& r_array) const;
    
	// Store the contents of the ep as the prop set.
    bool store(MCArrayRef p_array);

    bool fetchelement(MCExecContext& ctxt, MCNameRef p_name, MCValueRef& r_value) const;
    bool storeelement(MCExecContext& ctxt, MCNameRef p_name, MCValueRef p_value);
    
	//////////
	
	// MW-2013-12-05: [[ UnicodeFileFormat ]] These are the non-unicode propset
	//   pickle routines.
	IO_stat loadprops_new(IO_handle stream);
	IO_stat saveprops_new(IO_handle stream) const;
	
	// MW-2013-12-05: [[ UnicodeFileFormat ]] These are the non-unicode propset
	//   pickle routines.

	// Returns the (old-style) serialized size of the array. If 'nested_only'
	// is true, then it only counts the size of the props containing arrays.
	uint32_t measure_legacy(bool p_nested_only) const;
	// Returns true if the prop set has nested arrays.
	bool isnested_legacy(void) const;
	// Load the props from the given stream - this merges the existing array
	// with any we find (since nested and non-nested props are stored separately).
	IO_stat loadprops_legacy(IO_handle stream);
	// Load the array props from the given stream - this merges the existing
	// array with any we find.
	IO_stat loadarrayprops_legacy(MCObjectInputStream& stream);
	// Save the non-array props to the given stream.
	IO_stat saveprops_legacy(IO_handle stream) const;
	// Save the array props to the given stream.
	IO_stat savearrayprops_legacy(MCObjectOutputStream& stream) const;

private:
    /* Always returns a valid array, even if m_props isn't set.  The
     * returned array is _not_ owned by the caller */
    MCArrayRef fetch_nocopy() const;
    /* Returns the contents array, creating it if necessary.  If
     * creation fails, returns an unset array reference. */
    MCAutoArrayRef fetch_ensure();

	MCObjectPropertySet *m_next;
	MCNewAutoNameRef m_name;
	MCAutoArrayRef m_props;
};

////////////////////////////////////////////////////////////////////////////////

#endif
