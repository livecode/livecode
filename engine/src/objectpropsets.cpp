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

#include "object.h"
#include "execpt.h"
#include "objectstream.h"
#include "variable.h"

////////////////////////////////////////////////////////////////////////////////

class MCObjectPropertySet
{
public:
	MCObjectPropertySet(void)
	{
		m_next = nil;
		m_name = nil;
	}

	~MCObjectPropertySet(void)
	{
		MCNameDelete(m_name);
	}

	bool hasname(MCNameRef p_name)
	{
		return MCNameIsEqualTo(m_name, p_name, kMCCompareCaseless);
	}

	MCNameRef getname(void)
	{
		return m_name;
	}

	MCVariableValue *getvalue(void)
	{
		return &m_value;
	}

	MCObjectPropertySet *getnext(void)
	{
		return m_next;
	}

	void setnext(MCObjectPropertySet *p_next)
	{
		m_next = p_next;
	}

	/* CAN FAIL */ bool clone(MCObjectPropertySet*& r_set)
	{
		MCObjectPropertySet *t_new_set;
		if (createwithname(m_name, t_new_set))
		{
			t_new_set -> m_value . assign(m_value);
			r_set = t_new_set;
			return true;
		}

		return false;
	}

	void changename_nocopy(MCNameRef p_name)
	{
		MCNameDelete(m_name);
		m_name = p_name;
	}

	/* CAN FAIL */ static bool createwithname_nocopy(MCNameRef p_name, MCObjectPropertySet*& r_set)
	{
		MCObjectPropertySet *t_new_set;
		t_new_set = new MCObjectPropertySet;
		if (t_new_set == nil)
			return false;

		t_new_set -> m_name = p_name;

		r_set = t_new_set;

		return true;
	}

	/* CAN FAIL */ static bool createwithname(MCNameRef p_name, MCObjectPropertySet*& r_set)
	{
		MCNameRef t_new_name;
		if (!MCNameClone(p_name, t_new_name))
			return false;

		MCObjectPropertySet *t_new_set;
		if (!createwithname_nocopy(t_new_name, t_new_set))
		{
			MCNameDelete(t_new_name);
			return false;
		}

		r_set = t_new_set;

		return true;
	}

private:
	MCObjectPropertySet *m_next;
	MCNameRef m_name;
	MCVariableValue m_value;

};

////////////////////////////////////////////////////////////////////////////////

MCNameRef MCObject::getdefaultpropsetname(void)
{
	return props != nil ? props -> getname() : kMCEmptyName;
}

bool MCObject::findpropset(MCNameRef p_name, bool p_empty_is_default, MCVariableValue*& r_value)
{
	MCObjectPropertySet *t_set;
	t_set = props;
	if (!p_empty_is_default || !MCNameIsEqualTo(p_name, kMCEmptyName, kMCCompareCaseless))
		while(t_set != nil && !t_set -> hasname(p_name))
			t_set = t_set -> getnext();

	if (t_set != nil)
	{
		r_value = t_set -> getvalue();
		return true;
	}

	return false;
}

bool MCObject::ensurepropset(MCNameRef p_name, bool p_empty_is_default, MCVariableValue*& r_value)
{
	if (props == nil)
		if (!MCObjectPropertySet::createwithname(kMCEmptyName, props))
			return false;

	MCObjectPropertySet *t_set;
	t_set = props;
	if (!p_empty_is_default || !MCNameIsEqualTo(p_name, kMCEmptyName, kMCCompareCaseless))
		while(t_set != nil && !t_set -> hasname(p_name))
			t_set = t_set -> getnext();

	if (t_set == nil)
	{
		if (!MCObjectPropertySet::createwithname(p_name, t_set))
			return false;

		t_set -> setnext(props -> getnext());
		props -> setnext(t_set);
	}

	r_value = t_set -> getvalue();

	return true;
}

bool MCObject::setpropset(MCNameRef p_name)
{
	if (props == nil)
		if (!MCObjectPropertySet::createwithname(kMCEmptyName, props))
			return false;

	if (props -> hasname(p_name))
		return true;

	MCObjectPropertySet *t_set;
	t_set = props;
	while(t_set -> getnext() != nil && !t_set -> getnext() -> hasname(p_name))
		t_set = t_set -> getnext();

	if (t_set -> getnext() == nil)
	{
		if (!MCObjectPropertySet::createwithname(p_name, t_set))
			return false;

		t_set -> setnext(props);
		props = t_set;
	}
	else
	{
		MCObjectPropertySet *t_next_set;
		t_next_set = t_set -> getnext();
		t_set -> setnext(t_next_set -> getnext());
		t_next_set -> setnext(props);
		props = t_next_set;
	}

	return true;
}

void MCObject::listpropsets(MCExecPoint& ep)
{
	ep.clear();

	MCObjectPropertySet *p = props;
	uint2 j = 0;
	while (p != NULL)
	{
		if (!p->hasname(kMCEmptyName))
			ep.concatnameref(p->getname(), EC_RETURN, j++ == 0);
		p = p->getnext();
	}
}

bool MCObject::changepropsets(MCExecPoint& ep)
{
	if (ep.getsvalue().getlength() && ep.getsvalue().getstring()[ep.getsvalue().getlength() - 1] != '\n')
		ep.appendnewline();
	ep.appendnewline();
	char *string = ep.getsvalue().clone();
	char *eptr = string;
	MCObjectPropertySet *newprops = NULL;
	MCObjectPropertySet *newp = NULL;
	while ((eptr = strtok(eptr, "\n")) != NULL)
	{
		MCAutoNameRef t_name;
		/* UNCHECKED */ t_name . CreateWithCString(eptr);

		MCObjectPropertySet *lp = NULL;
		MCObjectPropertySet *p = props;
		while (p != NULL && !p->hasname(t_name))
		{
			lp = p;
			p = p->getnext();
		}
		if (p == NULL)
			/* UNCHECKED */ MCObjectPropertySet::createwithname(t_name, p);
		else
		{
			if (p == props)
				props = props->getnext();
			else
				lp->setnext(p->getnext());
			p->setnext(NULL);
		}
		if (newprops == NULL)
			newprops = p;
		else
			newp->setnext(p);
		newp = p;
		eptr = NULL;
	}
	Boolean gotdefault = False;
	while (props != NULL)
	{
		MCObjectPropertySet *sp = props->getnext();
		if (props->hasname(kMCEmptyName))
		{
			props->setnext(newprops);
			newprops = props;
			gotdefault = True;
		}
		else
			delete props;
		props = sp;
	}
	if (!gotdefault && newprops != NULL)
	{
		/* UNCHECKED */ MCObjectPropertySet::createwithname(kMCEmptyName, props);
		props->setnext(newprops);
	}
	else
		props = newprops;
	delete string;

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
		MCObjectPropertySet *t_new_set;

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

bool MCObject::hasarraypropsets(void)
{
	MCObjectPropertySet *t_prop;
	t_prop = props;
	while(t_prop != NULL)
	{
		if (t_prop -> getvalue() -> is_array() && t_prop -> getvalue() -> get_array() -> isnested())
			return true;
		t_prop = t_prop -> getnext();
	}
	return false;
}

uint32_t MCObject::measurearraypropsets(void)
{
	uint32_t t_prop_size;
	t_prop_size = 0;

	MCObjectPropertySet *t_prop;
	t_prop = props;
	while(t_prop != NULL)
	{
		// Although we only want nested arrays, measure returns 0 in size for these
		// if we pass true for p_nested_array.
		if (t_prop -> getvalue() -> is_array())
		{
			uint32_t t_size;
			t_size = t_prop -> getvalue() -> get_array() -> measure(true);
			if (t_size != 0)
				t_prop_size += t_size + 4;
		}

		t_prop = t_prop -> getnext();
	}

	return t_prop_size;
}

IO_stat MCObject::loadunnamedpropset(IO_handle stream)
{
	IO_stat stat;
	if (props == NULL)
		/* UNCHECKED */ MCObjectPropertySet::createwithname(kMCEmptyName, props);
	if ((stat = props->getvalue()->loadkeys(stream, props -> getvalue() -> is_array())) != IO_NORMAL)
		return stat;
	return stat;
}

IO_stat MCObject::loadpropsets(IO_handle stream)
{
	MCObjectPropertySet *p = props;
	IO_stat stat;

	while (True)
	{
		uint1 type;
		if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
			return stat;
		if (type == OT_CUSTOM)
		{
			MCNameRef pname;
			if ((stat = IO_read_nameref(pname, stream)) != IO_NORMAL)
				return stat;

			// If there is already a next pset, then it means its had array values loaded.
			// Thus we just advance and update the name.
			if (p -> getnext() != NULL)
			{
				p = p -> getnext();
				/* UNCHECKED */ p -> changename_nocopy(pname);
			}
			else
			{
				MCObjectPropertySet *v;
				/* UNCHECKED */ MCObjectPropertySet::createwithname_nocopy(pname, v);
				p->setnext(v);
				p = p->getnext();
			}

			if ((stat = p->getvalue()->loadkeys(stream, p -> getvalue() -> is_array())) != IO_NORMAL)
				return stat;
		}
		else
		{
			MCS_seek_cur(stream, -1);
			break;
		}
	}
	return IO_NORMAL;
}

IO_stat MCObject::loadarraypropsets(MCObjectInputStream& p_stream)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	// Note that props is always non-empty if we get here since we will have already loaded the
	// root custom properties.
	MCObjectPropertySet *t_prop;
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

				MCObjectPropertySet *t_new_prop;
				/* UNCHEKED */ MCObjectPropertySet::createwithname(kMCEmptyName, t_new_prop);
				if (t_prop != NULL)
					t_prop -> setnext(t_new_prop);

				t_prop = t_new_prop;
				t_index += 1;
			}

			// We now have our prop to load into - so we do so :o)
			t_stat = t_prop -> getvalue() -> loadarray(p_stream, t_prop -> getvalue() -> is_array());
		}
	}

	return t_stat;
}

IO_stat MCObject::saveunnamedpropset(IO_handle stream)
{
	MCObjectPropertySet *p = props;
	while (!p->hasname(kMCEmptyName))
		p = p->getnext();
	return p->getvalue()->savekeys(stream);
}

IO_stat MCObject::savepropsets(IO_handle stream)
{
	IO_stat stat;
	MCObjectPropertySet *p = props;
	while (p != NULL)
	{
		if (!p -> hasname(kMCEmptyName))
		{
			if ((stat = IO_write_uint1(OT_CUSTOM, stream)) != IO_NORMAL)
				return stat;
			if ((stat = IO_write_nameref(p->getname(), stream)) != IO_NORMAL)
				return stat;
			if ((stat = p->getvalue()->savekeys(stream)) != IO_NORMAL)
				return stat;
		}
		p = p->getnext();
	}
	return IO_NORMAL;
}

IO_stat MCObject::savearraypropsets(MCObjectOutputStream& p_stream)
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
			if (t_prop -> getvalue() -> is_array() && t_prop -> getvalue() -> get_array() -> isnested())
			{
				t_stat = p_stream . WriteU32(t_prop_index);
				if (t_stat == IO_NORMAL)
					t_stat = t_prop -> getvalue() -> get_array() -> save(p_stream, true);
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
			if (t_prop -> getvalue() -> is_array() && t_prop -> getvalue() -> get_array() -> isnested())
			{
				t_stat = p_stream . WriteU32(t_prop_index);
				if (t_stat == IO_NORMAL)
					t_stat = t_prop -> getvalue() -> get_array() -> save(p_stream, true);
			}
		}
		
		t_prop = t_prop -> getnext();
	}

	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU32(0);

	return t_stat;
}

////////////////////////////////////////////////////////////////////////////////
