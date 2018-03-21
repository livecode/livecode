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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "object.h"

#include "objectstream.h"
#include "variable.h"

#include "objectpropsets.h"

#include "stackfileformat.h"

////////////////////////////////////////////////////////////////////////////////

extern bool MCStringsSplit(MCStringRef p_string, codepoint_t p_delimiter, MCStringRef*& r_array, uindex_t& r_count);

////////////////////////////////////////////////////////////////////////////////

bool MCObjectPropertySet::clone(MCObjectPropertySet*& r_set) const
{
    MCAutoPointer<MCObjectPropertySet> t_new_set;
    if (!createwithname(*m_name, &t_new_set))
        return false;
    if (!t_new_set->store(fetch_nocopy()))
        return false;
    r_set = t_new_set.Release();
    return true;
}

bool MCObjectPropertySet::createwithname_nocopy(MCNameRef p_name, MCObjectPropertySet*& r_set)
{
	MCObjectPropertySet *t_new_set;
	t_new_set = new (nothrow) MCObjectPropertySet;
	if (t_new_set == nil)
		return false;

	t_new_set->m_name.Give(p_name);

	r_set = t_new_set;

	return true;
}

bool MCObjectPropertySet::createwithname(MCNameRef p_name, MCObjectPropertySet*& r_set)
{
	MCNameRef t_new_name = MCValueRetain(p_name);

	MCObjectPropertySet *t_new_set;
	if (!createwithname_nocopy(t_new_name, t_new_set))
	{
		MCValueRelease(t_new_name);
		return false;
	}

	r_set = t_new_set;

	return true;
}

//////////

MCArrayRef MCObjectPropertySet::fetch_nocopy() const
{
    return m_props.IsSet() ? *m_props : kMCEmptyArray;
}

MCAutoArrayRef MCObjectPropertySet::fetch_ensure()
{
    if (!m_props.IsSet())
        MCArrayCreateMutable(&m_props);
    return m_props;
}

bool MCObjectPropertySet::list(MCStringRef& r_keys) const
{
    return MCArrayListKeys(fetch_nocopy(), '\n', r_keys);
}

bool MCObjectPropertySet::clear(void)
{
    m_props.Reset();
    return true;
}

bool MCObjectPropertySet::fetch(MCArrayRef& r_array) const
{
    return MCArrayCopy(fetch_nocopy(), r_array);
}

bool MCObjectPropertySet::store(MCArrayRef p_array)
{
    MCAutoArrayRef t_mutable;
    if (!MCArrayMutableCopy(p_array, &t_mutable))
        return false;
    m_props.Give(t_mutable.Take());
    return true;
}

bool MCObjectPropertySet::fetchelement(MCExecContext& ctxt, MCNameRef p_name, MCValueRef& r_value) const
{
    return MCArrayFetchValue(fetch_nocopy(), ctxt . GetCaseSensitive(),
                             p_name, r_value);
}

bool MCObjectPropertySet::storeelement(MCExecContext& ctxt, MCNameRef p_name, MCValueRef p_value)
{
    MCAutoArrayRef t_props = fetch_ensure();
    if (!t_props.IsSet())
        return false;
    return MCArrayStoreValue(*t_props, ctxt . GetCaseSensitive(), p_name, p_value);
}

bool MCObjectPropertySet::restrict(MCStringRef p_string)
{
    bool t_success;
    t_success = true;
    MCAutoStringRefArray t_split;
    if (!MCStringsSplit(p_string, '\n', t_split . PtrRef(), t_split . CountRef()))
        return false;

    MCAutoArrayRef t_old_props(fetch_nocopy());
    MCAutoArrayRef t_new_props;
    MCArrayCreateMutable(&t_new_props);
    uinteger_t t_size;
    t_size = t_split . Count();
    for (uindex_t i = 0; i < t_size && t_success; i++)
    {
        MCNewAutoNameRef t_key_name;
        if (t_success)
            t_success = MCNameCreate(t_split[i], &t_key_name);
        MCValueRef t_value;
        if (!MCArrayFetchValue(*t_old_props, false, *t_key_name, t_value))
            t_value = kMCEmptyString;
        if (t_success)
            t_success = MCArrayStoreValue(*t_new_props, false, *t_key_name, t_value);
    }
    m_props.Give(t_new_props.Take());
    return t_success;
}

//////////

IO_stat MCObjectPropertySet::loadprops_new(IO_handle p_stream)
{
    MCAutoArrayRef t_new_props;

    if (IO_read_valueref_new((MCValueRef&)t_new_props, p_stream) != IO_NORMAL)
		return IO_ERROR;
    if (!t_new_props.MakeMutable())
        return IO_ERROR;
    m_props.Give(t_new_props.Take());

	return IO_NORMAL;
}

IO_stat MCObjectPropertySet::saveprops_new(IO_handle p_stream) const
{
	return IO_write_valueref_new(fetch_nocopy(), p_stream);
}

//////////

uint32_t MCObjectPropertySet::measure_legacy(bool p_nested_only) const
{
	return MCArrayMeasureForStreamLegacy(fetch_nocopy(), p_nested_only);
}

bool MCObjectPropertySet::isnested_legacy(void) const
{
	return MCArrayIsNestedLegacy(fetch_nocopy());
}

IO_stat MCObjectPropertySet::loadprops_legacy(IO_handle p_stream)
{
    MCAutoArrayRef t_props = fetch_ensure();
    if (!t_props.IsSet())
        return IO_ERROR;

    IO_stat t_status = MCArrayLoadFromHandleLegacy(*t_props, p_stream);
    return t_status;
}

IO_stat MCObjectPropertySet::loadarrayprops_legacy(MCObjectInputStream& p_stream)
{
    MCAutoArrayRef t_props = fetch_ensure();
    if (!t_props.IsSet())
        return IO_ERROR;

    IO_stat t_status = MCArrayLoadFromStreamLegacy(*t_props, p_stream);
    return t_status;
}

IO_stat MCObjectPropertySet::saveprops_legacy(IO_handle p_stream) const
{
	return MCArraySaveToHandleLegacy(fetch_nocopy(), p_stream);
}

IO_stat MCObjectPropertySet::savearrayprops_legacy(MCObjectOutputStream& p_stream) const
{
	return MCArraySaveToStreamLegacy(fetch_nocopy(), true, p_stream);
}

////////////////////////////////////////////////////////////////////////////////

MCNameRef MCObject::getdefaultpropsetname(void)
{
	return props != nil ? props -> getname() : kMCEmptyName;
}

bool MCObject::findpropset(MCNameRef p_name, bool p_empty_is_default, MCObjectPropertySet*& r_set)
{
	MCObjectPropertySet *t_set;
	t_set = props;
	if (!p_empty_is_default || !MCNameIsEmpty(p_name))
		while(t_set != nil && !t_set -> hasname(p_name))
			t_set = t_set -> getnext();

	if (t_set != nil)
	{
		r_set = t_set;
		return true;
	}

	return false;
}

bool MCObject::ensurepropset(MCNameRef p_name, bool p_empty_is_default, MCObjectPropertySet*& r_set)
{
	if (props == nil)
		if (!MCObjectPropertySet::createwithname(kMCEmptyName, props))
			return false;

	MCObjectPropertySet *t_set;
	t_set = props;
	if (!p_empty_is_default || !MCNameIsEmpty(p_name))
		while(t_set != nil && !t_set -> hasname(p_name))
			t_set = t_set -> getnext();

	if (t_set == nil)
	{
		if (!MCObjectPropertySet::createwithname(p_name, t_set))
			return false;

		t_set -> setnext(props -> getnext());
		props -> setnext(t_set);
	}

	r_set = t_set;

	return true;
}

bool MCObject::clonepropsets(MCObjectPropertySet*& r_new_props) const
{
	MCObjectPropertySet *t_new_props, *t_set, *t_new_props_tail;
	t_set = props;
	t_new_props = nil;
	t_new_props_tail = nil;
	while(t_set != nil)
	{
		MCObjectPropertySet *t_new_set = nullptr;

		/* UNCHECKED */ t_set -> clone(t_new_set);
		if (t_new_props_tail != nil)
			t_new_props_tail -> setnext(t_new_set);
		else
			t_new_props = t_new_set;

		t_new_props_tail = t_new_set;

		t_set = t_set -> getnext();
	}

	r_new_props = t_new_props;

	return true;
}

void MCObject::deletepropsets(void)
{
	while(props != NULL)
	{
		MCObjectPropertySet *t_next;
		t_next = props -> getnext();
		delete props;
		props = t_next;
	}
}

IO_stat MCObject::savepropsets(IO_handle stream, uint32_t p_version)
{
	if (p_version < kMCStackFileFormatVersion_7_0)
		return savepropsets_legacy(stream);
	
	// MW-2013-12-05: [[ UnicodeFileFormat ]] Emit all the propsets in
	//   OT_CUSTOM - name - array
	// format.
	IO_stat stat;
	MCObjectPropertySet *p = props;
	while (p != NULL)
	{
		if ((stat = IO_write_uint1(OT_CUSTOM, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_nameref_new(p->getname(), stream, true)) != IO_NORMAL)
			return stat;
		if ((stat = p->saveprops_new(stream)) != IO_NORMAL)
			return stat;
		p = p->getnext();
	}
	return IO_NORMAL;
}

IO_stat MCObject::loadpropsets(IO_handle stream, uint32_t version)
{
	if (version < kMCStackFileFormatVersion_7_0)
		return loadpropsets_legacy(stream);
	
	// MW-2013-12-05: [[ UnicodeFileFormat ]] Read all the propsets in
	//   OT_CUSTOM - name - array
	// format.
	MCObjectPropertySet *p = props;
	IO_stat stat;
	while (True)
	{
		uint1 type;
		if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if (type == OT_CUSTOM)
		{
			MCNameRef pname;
			if ((stat = IO_read_nameref_new(pname, stream, true)) != IO_NORMAL)
				return checkloadstat(stat);
			
			MCObjectPropertySet *v = nullptr;
			/* UNCHECKED */ MCObjectPropertySet::createwithname_nocopy(pname, v);
			if (p != nil)
			{
				p->setnext(v);
				p = p->getnext();
			}
			else
				props = p = v;

			if ((stat = p->loadprops_new(stream)) != IO_NORMAL)
				return checkloadstat(stat);
		}
		else
		{
			MCS_seek_cur(stream, -1);
			break;
		}
	}
	return IO_NORMAL;
}

bool MCObject::hasarraypropsets_legacy(void)
{
	MCObjectPropertySet *t_prop;
	t_prop = props;
	while(t_prop != NULL)
	{
		if (t_prop -> isnested_legacy())
			return true;
		t_prop = t_prop -> getnext();
	}
	return false;
}

uint32_t MCObject::measurearraypropsets_legacy(void)
{
	uint32_t t_prop_size;
	t_prop_size = 0;

	MCObjectPropertySet *t_prop;
	t_prop = props;
	while(t_prop != NULL)
	{
		// Although we only want nested arrays, measure returns 0 in size for these
		// if we pass true for p_nested_array.
		uint32_t t_size;
		t_size = t_prop -> measure_legacy(true);
		if (t_size != 0)
			t_prop_size += t_size + 4;

		t_prop = t_prop -> getnext();
	}

	return t_prop_size;
}

IO_stat MCObject::loadunnamedpropset_legacy(IO_handle stream)
{
	IO_stat stat;
	if (props == NULL)
		/* UNCHECKED */ MCObjectPropertySet::createwithname(kMCEmptyName, props);
	if ((stat = props->loadprops_legacy(stream)) != IO_NORMAL)
		return stat;
	return stat;
}

IO_stat MCObject::loadpropsets_legacy(IO_handle stream)
{
	MCObjectPropertySet *p = props;
	IO_stat stat;

	while (True)
	{
		uint1 type;
		if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if (type == OT_CUSTOM)
		{
			MCNameRef pname;
			if ((stat = IO_read_nameref_new(pname, stream, false)) != IO_NORMAL)
				return checkloadstat(stat);

			// If there is already a next pset, then it means its had array values loaded.
			// Thus we just advance and update the name.
			if (p -> getnext() != NULL)
			{
				p = p -> getnext();
				/* UNCHECKED */ p -> changename_nocopy(pname);
			}
			else
			{
				MCObjectPropertySet *v = nullptr;
				/* UNCHECKED */ MCObjectPropertySet::createwithname_nocopy(pname, v);
				p->setnext(v);
				p = p->getnext();
			}

			if ((stat = p->loadprops_legacy(stream)) != IO_NORMAL)
				return checkloadstat(stat);
		}
		else
		{
			MCS_seek_cur(stream, -1);
			break;
		}
	}
	return IO_NORMAL;
}

IO_stat MCObject::loadarraypropsets_legacy(MCObjectInputStream& p_stream)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	// Note that props is always non-empty if we get here since we will have already loaded the
	// root custom properties.
	MCObjectPropertySet *t_prop;

	MCAssert (nil != props);
	t_prop = props;

	uint32_t t_index;
	t_index = 1;
	while(t_stat == IO_NORMAL)
	{
		// First compute the index of the first prop we want to load into.
		// 
		uint32_t t_new_index;
		t_stat = p_stream . ReadU32(t_new_index);
		if (t_new_index == 0)
			break;

		if (t_stat == IO_NORMAL)
		{
			// Iterate forward in the props list creating them as we go.
			while(t_index < t_new_index)
			{
				if (t_prop != NULL && t_prop -> getnext() != NULL)
				{
					t_prop = t_prop -> getnext();
					continue;
				}

				MCObjectPropertySet *t_new_prop = nullptr;
				/* UNCHEKED */ MCObjectPropertySet::createwithname(kMCEmptyName, t_new_prop);
				t_prop -> setnext(t_new_prop);

				t_prop = t_new_prop;
				t_index += 1;
			}

			// We now have our prop to load into - so we do so :o)
			t_stat = t_prop -> loadarrayprops_legacy(p_stream);
		}
	}

	return checkloadstat(t_stat);
}

IO_stat MCObject::saveunnamedpropset_legacy(IO_handle stream)
{
	MCObjectPropertySet *p = props;
	while (!p->hasname(kMCEmptyName))
		p = p->getnext();
	return p->saveprops_legacy(stream);
}

IO_stat MCObject::savepropsets_legacy(IO_handle stream)
{
	IO_stat stat;
	MCObjectPropertySet *p = props;
	while (p != NULL)
	{
		if (!p -> hasname(kMCEmptyName))
		{
			if ((stat = IO_write_uint1(OT_CUSTOM, stream)) != IO_NORMAL)
				return stat;
			if ((stat = IO_write_nameref_new(p->getname(), stream, false)) != IO_NORMAL)
				return stat;
			if ((stat = p->saveprops_legacy(stream)) != IO_NORMAL)
				return stat;
		}
		p = p->getnext();
	}
	return IO_NORMAL;
}

IO_stat MCObject::savearraypropsets_legacy(MCObjectOutputStream& p_stream)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	// Start the prop index at one - this is because the first custom property set
	// is the no-name one and it is always created.
	MCObjectPropertySet *t_prop;
	t_prop = props;
	uint32_t t_prop_index;
	t_prop_index = 1;

	// Store the no-name custom property first
	for(t_prop = props; t_prop != NULL; t_prop = t_prop -> getnext())
		if (t_prop -> hasname(kMCEmptyName))
		{
			if (t_prop -> isnested_legacy())
			{
				t_stat = p_stream . WriteU32(t_prop_index);
				if (t_stat == IO_NORMAL)
					t_stat = t_prop -> savearrayprops_legacy(p_stream);
			}
			break;
		}
	
	// Now, iterate through the rest, ignoring the no-name custom prop.
	t_prop = props;
	while(t_prop != NULL && t_stat == IO_NORMAL)
	{
		if (!t_prop -> hasname(kMCEmptyName) != 0)
		{
			t_prop_index += 1;
			if (t_prop -> isnested_legacy())
			{
				t_stat = p_stream . WriteU32(t_prop_index);
				if (t_stat == IO_NORMAL)
					t_stat = t_prop -> savearrayprops_legacy(p_stream);
			}
		}
		
		t_prop = t_prop -> getnext();
	}

	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU32(0);

	return t_stat;
}

////////////////////////////////////////////////////////////////////////////////
