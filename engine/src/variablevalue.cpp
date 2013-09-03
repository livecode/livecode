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
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"
#include "mcio.h"

#include "execpt.h"
#include "mcerror.h"
#include "util.h"
#include "globals.h"
#include "objectstream.h"

///////////////////////////////////////////////////////////////////////////////

void MCVariableValue::assign_empty(void)
{
	destroy();

	set_type(VF_STRING);

	strnum . buffer . data = NULL;
	strnum . buffer . size = 0;
	strnum . svalue . string = MCnullstring;
	strnum . svalue . length = 0;
	
	set_dbg_changed(true);
}

void MCVariableValue::assign_new_array(uint32_t p_size)
{
	destroy();
	
	// MW-2009-04-08: Abstract access to 'type' field.
	set_type(VF_ARRAY);
	
	array . presethash(p_size);

	set_dbg_changed(true);
}

void MCVariableValue::assign_constant_string(const MCString& s)
{
	destroy();

	set_type(VF_STRING);

	strnum . buffer . data = NULL;
	strnum . buffer . size = 0;
	strnum . svalue . string = s . getstring();
	strnum . svalue . length = s . getlength();

	set_dbg_changed(true);
}

void MCVariableValue::assign_real(real64_t r)
{
	destroy();

	set_type(VF_NUMBER);
	
	strnum . buffer . data = NULL;
	strnum . buffer . size = 0;
	strnum . nvalue = r;
	
	set_dbg_changed(true);
}

void MCVariableValue::assign_buffer(char *p_buffer, uint32_t p_length)
{
	destroy();
	
	set_type(VF_STRING);

	strnum . buffer . data = p_buffer;
	strnum . buffer . size = p_length;
	strnum . svalue . string = p_buffer;
	strnum . svalue . length = p_length;
	
	set_dbg_changed(true);
}

void MCVariableValue::exchange(MCVariableValue& v)
{
	char t_temp[sizeof(MCVariableValue) - 4];
	memcpy(t_temp, ((char *)&v) + 4, sizeof(MCVariableValue) - 4);
	memcpy(((char *)&v) + 4, ((char *)this) + 4, sizeof(MCVariableValue) - 4);
	memcpy(((char *)this) + 4, t_temp, sizeof(MCVariableValue) - 4);

	Value_format t_temp_type;
	t_temp_type = v . get_type();
	v . set_type(get_type());
	set_type(t_temp_type);

	set_dbg_changed(true);
	v . set_dbg_changed(true);
}

Exec_stat MCVariableValue::fetch(MCExecPoint& ep, bool p_copy)
{
	switch(get_type())
	{
	case VF_UNDEFINED:
		ep . setboth("", 0.0);
	break;

	case VF_STRING:
		assert(is_string());
		ep . setsvalue(get_string());
		if (p_copy)
			ep . grabsvalue();
	break;

	case VF_NUMBER:
		ep . setnvalue(get_real());
	break;

	case VF_BOTH:
		ep . setboth(get_string(), get_real());
		if (p_copy)
			ep . grabsvalue();
	break;

	case VF_ARRAY:
		if (!p_copy)
			ep . setarray(this, False);
		else
			ep . setarray(new MCVariableValue(*this), True);
	break;
	}

	return ES_NORMAL;
}

Exec_stat MCVariableValue::store(MCExecPoint& ep)
{
	switch(ep . getformat())
	{
	case VF_UNDEFINED:
		clear();
	break;

	case VF_STRING:
		assign_string(ep . getsvalue());
	break;

	case VF_NUMBER:
		assign_real(ep . getnvalue());
	break;

	case VF_BOTH:
		assign_both(ep . getsvalue(), ep . getnvalue());
	break;

	case VF_ARRAY:
		// MW-2008-08-30: [[ Bug ]] Doing put tArray[x] into tArray causes badness since 'assign'
		//   first deletes the current value then copies.
		if (!is_array() || ep . getdeletearray())
			assign(*(ep . getarray()));
		else
		{
			MCVariableValue t_temp(*(ep . getarray()));
			exchange(t_temp);
		}
	break;
	}

	return ES_NORMAL;
}

Exec_stat MCVariableValue::fetch_element(MCExecPoint& ep, const MCString& key, bool p_copy)
{
	if (is_array())
	{
		MCHashentry *e;
		e = array . lookuphash(key, ep . getcasesensitive(), False);
		if (e != NULL)
			return e -> value . fetch(ep, p_copy);
	}

	// MW-2008-06-30: [[ Bug ]] If an element is not found, then it should set the ep to the
	//   'undefined' value (which is BOTH empty and 0.0).
	ep . setboth(MCnullmcstring, 0.0);

	return ES_NORMAL;
}

bool MCVariableValue::fetch_element_if_exists(MCExecPoint& ep, const MCString& key, bool p_copy)
{
	if (is_array())
	{
		MCHashentry *e;
		e = array . lookuphash(key, ep . getcasesensitive(), False);
		if (e != NULL)
			return e -> value . fetch(ep, p_copy) == ES_NORMAL;
	}

	return false;
}

Exec_stat MCVariableValue::lookup_index(MCExecPoint& ep, uint32_t index, bool p_add, MCVariableValue*& r_value)
{
	if (is_array())
		;
	else
	{
		if (!p_add)
		{
			r_value = nil;
			return ES_NORMAL;
		}
		assign_new_array(TABLE_SIZE);
	}
	
	MCHashentry *t_hashentry = array . lookupindex(index, p_add ? True : False);
	if (t_hashentry != nil)
		r_value = &(t_hashentry->value);
	else
		r_value = nil;

	if (p_add)
		set_dbg_mutated(true);
	
	return ES_NORMAL;
}

Exec_stat MCVariableValue::lookup_index(MCExecPoint& ep, uint32_t index, MCVariableValue*& r_value)
{
	return lookup_index(ep, index, true, r_value);
}

Exec_stat MCVariableValue::lookup_element(MCExecPoint& ep, const MCString& key, MCVariableValue*& r_value)
{
	if (is_array())
		;
	else
		assign_new_array(TABLE_SIZE);
	
	// MW-2012-09-04: [[ Bug 10284 ]] Use common 'lookup_hash' method.
	MCHashentry *t_value;
	Exec_stat t_stat;
	t_stat = lookup_hash(ep, key, True, t_value);
	if (t_stat == ES_ERROR)
		return t_stat;
	
	r_value = &(t_value -> value);

	set_dbg_mutated(true);
	
	return ES_NORMAL;
}

// MW-2012-09-04: [[ Bug 10284 ]] If the key shares the string buffer with the
//   variable then make a temporary copy of it, otherwise data gets overwritten
//   causing keys to be created with corrupted names.
Exec_stat MCVariableValue::lookup_hash(MCExecPoint& ep, const MCString& p_key, bool p_add, MCHashentry*& r_value)
{
	MCString t_key;
	bool t_free;
	t_free = false;
	if (is_array())
	{
		t_key = p_key;
		t_free = false;
	}
	else
	{
		if (!p_add)
		{
			r_value = NULL;
			return ES_NORMAL;
		}

		if (is_string() && (p_key . getstring() < strnum . buffer . data + strnum . buffer . size && p_key . getstring() + p_key . getlength() >= strnum . buffer . data))
		{
			t_key . set((char *)malloc(p_key . getlength()), p_key . getlength());
			memcpy((char *)t_key . getstring(), p_key . getstring(), p_key . getlength());
		}
		else
		{
			t_free = false;
			t_key = p_key;
		}

		assign_new_array(TABLE_SIZE);
	}
	
	r_value = array . lookuphash(t_key, ep . getcasesensitive(), p_add);
	
	if (t_free)
		free((void *)t_key . getstring());
	
	if (p_add)
		set_dbg_mutated(true);

	return ES_NORMAL;
}

Exec_stat MCVariableValue::store_element(MCExecPoint& ep, const MCString& key)
{
	Exec_stat t_stat;
	MCVariableValue *t_value;
	t_stat = lookup_element(ep, key, t_value);
	if (t_stat == ES_NORMAL)
		t_value -> store(ep);
	return t_stat;
}

Exec_stat MCVariableValue::append_element(MCExecPoint& ep, const MCString& key)
{
	Exec_stat t_stat;
	MCVariableValue *t_value;
	t_stat = lookup_element(ep, key, t_value);
	if (t_stat == ES_NORMAL)
		t_value -> append(ep);
	return t_stat;
}

Exec_stat MCVariableValue::remove_element(MCExecPoint& ep, const MCString& key)
{
	if (is_array())
	{
		array . removehash(key, ep . getcasesensitive());
		if (array . getnfilled() == 0)
			assign_empty();
		else
			set_dbg_mutated(true);
	}

	return ES_NORMAL;
}

void MCVariableValue::remove_hash(MCHashentry *p_entry)
{
	if (is_array())
	{
		array . removehash(p_entry);
		if (array . getnfilled() == 0)
			assign_empty();
		else
			set_dbg_mutated(true);
	}
}

//

bool MCVariableValue::get_as_real(MCExecPoint& ep, real64_t& r_value)
{
	if (is_number())
	{
		r_value = strnum . nvalue;
		return true;
	}

	if (is_empty())
	{
		r_value = 0.0;
		return true;
	}

	if (is_string())
		return MCU_stor8(MCString(strnum . svalue . string, strnum . svalue . length), r_value, ep . getconvertoctals()) == True;

	assert(is_array());

	r_value = 0.0;
	return true;
}

bool MCVariableValue::copy(const MCVariableValue& v)
{
	set_type(v . get_type());
	set_dbg_changed(true);

	if (!v . is_array())
	{
		strnum . buffer . data = NULL;
		strnum . buffer . size = 0;

		if (v . is_number())
			strnum . nvalue = v . strnum . nvalue;

		if (v . is_string())
		{
			if (v . strnum . svalue . length > 0)
			{
				// MW-2008-08-26: [[ Bug 6981 ]] Make sure we don't do anything if memory is exhausted
				char *t_new_buffer;
				t_new_buffer = (char *)malloc(v . strnum . svalue . length);
				if (t_new_buffer != NULL)
				{
					strnum . buffer . data = t_new_buffer;
					strnum . buffer . size = v . strnum . svalue . length;

					memcpy(strnum . buffer . data, v . strnum . svalue . string, v . strnum . svalue . length);

					strnum . svalue . string = strnum . buffer . data;
					strnum . svalue . length = strnum . buffer . size;
				}
				else
				{
					strnum . svalue . string = MCnullstring;
					strnum . svalue . length = 0;
					return false;
				}
			}
			else
			{
				strnum . svalue . string = MCnullstring;
				strnum . svalue . length = 0;
			}
		}

		return true;
	}
	
	if (array . copytable(v . array))
	{
		set_dbg_mutated(true);
		return true;
	}
	
	clear();

	return false;
}

bool MCVariableValue::assign_string(const MCString& p_string)
{
	set_dbg_changed(true);

	if (!is_array())
	{
		// Note that it could be that p_string overlaps buffer, but in which case the new size will
		// always be less than the current size so no realloc occurs.

		if (p_string . getlength() >  strnum . buffer . size)
		{
			// MW-2008-08-26: [[ Bug 6981 ]] Make sure we assign the empty string if we don't manage
			//   to reallocate.
			char *t_new_buffer;
			t_new_buffer = (char *)realloc(strnum . buffer . data, p_string . getlength());
			if (t_new_buffer != NULL)
			{
				strnum . buffer . data = t_new_buffer;
				strnum . buffer . size = p_string . getlength();
			}
			else
			{
				strnum . svalue . string = MCnullstring;
				strnum . svalue . length = 0;
				set_type(VF_STRING);
				return false;
			}
		}
		else if (strnum . buffer . data == NULL)
		{
			strnum . svalue . string = MCnullstring;
			strnum . svalue . length = 0;
			set_type(VF_STRING);
			return true;
		}

		assert(p_string . getlength() <= strnum . buffer . size);
		memmove(strnum . buffer . data, p_string . getstring(), p_string . getlength());
		
		strnum . svalue . string = strnum . buffer . data;
		strnum . svalue . length = p_string . getlength();

		set_type(VF_STRING);

		return true;
	}

	// We are an array, so first we allocate the new buffer *then* we destroy ourselves.
	uint32_t t_new_size;
	t_new_size = p_string . getlength();

	if (t_new_size == 0)
	{
		assign_empty();
		return true;
	}

	char *t_new_buffer;
	t_new_buffer = (char *)malloc(t_new_size);
	if (t_new_buffer == NULL)
	{
		assign_empty();
		return false;
	}

	memcpy(t_new_buffer, p_string . getstring(), t_new_size);

	array . freehash();
	
	strnum . buffer . data = t_new_buffer;
	strnum . buffer . size = t_new_size;

	strnum . svalue . string = t_new_buffer;
	strnum . svalue . length = t_new_size;

	set_type(VF_STRING);

	return true;
}

bool MCVariableValue::assign_both(const MCString& s, real64_t n)
{
	if (!assign_string(s))
		return false;

	set_type(VF_BOTH);
	strnum . nvalue = n;

	return true;
}

// This is a support method for the V1 externals API. It allows the buffer of
// a value to hold a temporary stringified value with custom number formatting.
// Note that the stored string has a NUL included, to help with passing around.
bool MCVariableValue::assign_custom_both(const char *s, real64_t n)
{
	// Assign the string, but with the NUL included.
	if (!assign_string(MCString(s, strlen(s) + 1)))
		return false;

	// Make sure the extra NUL isn't included in the string value's length
	strnum . svalue . length -= 1;

	set_type(VF_NUMBER);
	strnum . nvalue = n;

	return true;
}

// This method assumes the value is already a string.
bool MCVariableValue::reserve(uint32_t p_required_length, void*& r_buffer, uint32_t& r_length)
{
	if (strnum . buffer . size == 0)
	{
		// If the buffer is 0 size it means we are either empty or we have a constant
		// string assigned.

		// Work out how much space we need.
		uint32_t t_new_size;
		t_new_size = MCU_max(p_required_length, strnum . svalue . length);

		// Attempt to allocate it.
		strnum . buffer . data = (char *)malloc(t_new_size);
		if (strnum . buffer . data == NULL)
			return false;

		// Assign the buffer, and move the svalue over.
		strnum . buffer . size = t_new_size;
		memcpy(strnum . buffer . data, strnum . svalue . string, strnum . svalue . length);
		strnum . svalue . string = strnum . buffer . data;
	}
	else if (p_required_length > strnum . buffer . size)
	{
		// We need more space, so reallocate.
		char *t_new_buffer;
		t_new_buffer = (char *)realloc(strnum . buffer . data, p_required_length);
		if (t_new_buffer == NULL)
			return false;

		// Update the buffer
		strnum . buffer . size = p_required_length;
		
		// And make sure the svalue points to the right place
		strnum . svalue . string = strnum . buffer . data;
	}

	// Return the buffer and existing string length.
	r_buffer = strnum . buffer . data;
	r_length = strnum . svalue . length;

	return true;
}

bool MCVariableValue::commit(uint32_t p_actual_length)
{
	if (strnum . buffer . size < p_actual_length)
		return false;

	strnum . buffer . data = (char *)realloc(strnum . buffer . data, p_actual_length);
	strnum . buffer . size = p_actual_length;

	strnum . svalue . string = strnum . buffer . data;
	strnum . svalue . length = p_actual_length;

	set_dbg_changed(true);

	return true;
}

void MCVariableValue::assign_constant_both(const MCString& s, real64_t n)
{
	assign_constant_string(s);

	set_type(VF_BOTH);
	strnum . nvalue = n;
}

// This method is wrapped by other methods. It is used by ::append, and also the
// externals V1 interface. It assumes that the value within the value is already
// a string.
bool MCVariableValue::append_string(const MCString& s)
{
	const char *t_new_string;
	t_new_string = s . getstring();

	uint32_t t_new_length;
	t_new_length = s . getlength();

	if (t_new_length > 0)
	{
		uint32_t t_new_size;
		t_new_size = t_new_length + strnum . svalue . length;

		if (t_new_size > strnum . buffer . size)
		{
			t_new_size = (t_new_size + VAR_PAD) & VAR_MASK;
			if (strnum . buffer . data != NULL)
				t_new_size += MCU_min(t_new_size, VAR_APPEND_MAX);

			char *t_new_buffer;
			t_new_buffer = (char *)malloc(t_new_size);
			if (t_new_buffer != NULL)
			{
				memcpy(t_new_buffer, strnum . svalue . string, strnum . svalue . length);
				memmove(t_new_buffer + strnum . svalue . length, t_new_string, t_new_length);
				
				free(strnum . buffer . data);

				strnum . buffer . data = t_new_buffer;
				strnum . buffer . size = t_new_size;

				strnum . svalue . string = t_new_buffer;
				strnum . svalue . length += t_new_length;
			}
			else
				return false;
		}
		else
		{
			memmove(strnum . buffer . data + strnum . svalue . length, t_new_string, t_new_length);
			strnum . svalue . string = strnum . buffer . data;
			strnum . svalue . length += t_new_length;
		}

		set_type(VF_STRING);
	}

	set_dbg_changed(true);

	return true;
}

Exec_stat MCVariableValue::append(MCExecPoint& ep)
{
	if (!ensure_string(ep))
		return ES_ERROR;

	// If we attempt to append an array, the array gets converted to empty,
	// thus resulting in a no-op.
	if (ep . getformat() == VF_ARRAY)
		return ES_NORMAL;

	// Attempt to append the string value of the ep.
	if (!append_string(ep . getsvalue()))
	{
		if (!MCabortscript)
		{
			MCeerror -> add(EE_NO_MEMORY, 0, 0);
			MCabortscript = True;
		}

		return ES_ERROR;
	}

	return ES_NORMAL;
}

// This is a helper method for the V1 externals interface. It ensures that the
// string is terminated by a NUL, but without affecting the value's size. It
// must only be called on value's which are already strings.
bool MCVariableValue::ensure_cstring(void)
{
	if (strnum . buffer . size != 0 &&
		strnum . svalue . length < strnum . buffer . size)
	{
		strnum . buffer . data[strnum . svalue . length] = '\0';
		return true;
	}
	
	if (!append_string(MCString("\0", 1)))
		return false;

	strnum . svalue . length -= 1;

	return true;
}

bool MCVariableValue::encode(void*& r_buffer, uint32_t& r_length)
{
	IO_handle t_stream_handle;
	t_stream_handle = MCS_fakeopenwrite();
	if (t_stream_handle == NULL)
		return false;
	
	MCObjectOutputStream *t_stream;
	t_stream = new MCObjectOutputStream(t_stream_handle);
	if (t_stream == NULL)
	{
		char *t_buffer;
		uint32_t t_length;
		MCS_fakeclosewrite(t_stream_handle, t_buffer, t_length);
		delete t_buffer;
		return false;
	}

	IO_stat t_stat;
	t_stat = IO_NORMAL;
	
	switch(get_format())
	{
	case VF_UNDEFINED:
		t_stat = t_stream -> WriteU8(kMCEncodedValueTypeUndefined);
	break;
	case VF_STRING:
		if (strnum . svalue . length == 0)
			t_stat = t_stream -> WriteU8(kMCEncodedValueTypeEmpty);
		else
		{
			t_stat = t_stream -> WriteU8(kMCEncodedValueTypeString);
			if (t_stat == IO_NORMAL)
				t_stat = t_stream -> WriteU32(strnum . svalue . length);
			if (t_stat == IO_NORMAL)
				t_stat = t_stream -> Write(strnum . svalue . string, strnum . svalue . length);
		}
	break;
	case VF_BOTH:
		// MW-2008-10-29: [[ Bug ]] If the length of the string is 0 and the
		//   numeric value is 0.0, then we are actually 'UNDEFINED'.
		if (strnum . svalue . length == 0 && strnum . nvalue == 0.0)
			t_stat = t_stream -> WriteU8(kMCEncodedValueTypeUndefined);
		else
		{
			t_stat = t_stream -> WriteU8(kMCEncodedValueTypeNumber);
			if (t_stat == IO_NORMAL)
				t_stat = t_stream -> WriteFloat64(strnum . nvalue);
		}
	break;
	case VF_NUMBER:
		t_stat = t_stream -> WriteU8(kMCEncodedValueTypeNumber);
		if (t_stat == IO_NORMAL)
			t_stat = t_stream -> WriteFloat64(strnum . nvalue);
	break;
	case VF_ARRAY:
		t_stat = t_stream -> WriteU8(kMCEncodedValueTypeArray);
		if (t_stat == IO_NORMAL)
			t_stat = array . save(*t_stream, false);
	break;
	}
	
	if (t_stat == IO_NORMAL)
		t_stream -> Flush(true);

	delete t_stream;
	
	char *t_buffer;
	uint32_t t_length;
	MCS_fakeclosewrite(t_stream_handle, t_buffer, t_length);
	
	if (t_stat == IO_NORMAL)
	{
		r_buffer = t_buffer;
		r_length = t_length;
	}
	else
		delete t_buffer;
	
	return t_stat == IO_NORMAL;
}

bool MCVariableValue::decode(const MCString& p_value)
{
	IO_handle t_stream_handle;
	t_stream_handle = MCS_fakeopen(p_value);
	if (t_stream_handle == NULL)
		return false;
		
	MCObjectInputStream *t_stream = nil;
	t_stream = new MCObjectInputStream(t_stream_handle, p_value . getlength());
	if (t_stream == NULL)
	{
		MCS_close(t_stream_handle);
		return false;
	}
	
	IO_stat t_stat;
	t_stat = IO_NORMAL;
	
	uint8_t t_type;
	t_stat = t_stream -> ReadU8(t_type);
	if (t_stat == IO_NORMAL)
	{
		switch(t_type)
		{
		case kMCEncodedValueTypeUndefined:
			clear();
		break;
		case kMCEncodedValueTypeEmpty:
			assign_empty();
		break;
		case kMCEncodedValueTypeString:
		{
			uint32_t t_length;
			t_stat = t_stream -> ReadU32(t_length);
			if (t_stat == IO_NORMAL)
			{
				char *t_value;
				t_value = new char[t_length];
				if (t_value != NULL)
					assign_buffer(t_value, t_length);
				else
					t_stat = IO_ERROR;
			}
		}
		break;
		case kMCEncodedValueTypeNumber:
		{
			double t_value;
			t_stat = t_stream -> ReadFloat64(t_value);
			if (t_stat == IO_NORMAL)
				assign_real(t_value);
		}
		break;
		case kMCEncodedValueTypeArray:
			t_stat = loadarray(*t_stream, false);
		break;
		default:
			t_stat = IO_ERROR;
		break;
		}
	}
	
	delete t_stream;
	MCS_close(t_stream_handle);
	
	set_dbg_changed(true);

	return t_stat == IO_NORMAL;
}

///////////////////////////////////////////////////////////////////////////////

bool MCVariableValue::coerce_to_real(MCExecPoint& ep)
{
	assert(!is_real());

	if (is_empty())
	{
		assign_real(0.0);
		return true;
	}

	if (is_string())
	{
		if (MCU_stor8(MCString(strnum . svalue . string, strnum . svalue . length), strnum . nvalue, ep . getconvertoctals()) != True)
			return false;

		set_type(VF_BOTH);
		return true;
	}

	assert(is_array());

	assign_real(0.0);
	return true;
}

bool MCVariableValue::coerce_to_string(MCExecPoint& ep)
{
	assert(!is_string());

	if (is_undefined())
	{
		assign_empty();
		return true;
	}

	if (is_number())
	{
		uint32_t t_length;
		t_length = MCU_r8tos(strnum . buffer . data, strnum . buffer . size, strnum . nvalue, ep . getnffw(), ep . getnftrailing(), ep . getnfforce());

		strnum . svalue . string = strnum . buffer . data;
		strnum . svalue . length = t_length;

		set_type(VF_BOTH);
		return true;
	}

	assign_empty();
	return true;
}

//

Exec_stat MCVariableValue::combine(char e, char k, MCExecPoint& ep)
{
	if (!is_array())
	{
		assign_empty();
		return ES_NORMAL;
	}

	char *t_buffer;
	uint32_t t_length;
	array . combine(ep, e, k, t_buffer, t_length);
	assign_buffer(t_buffer, t_length);
	
	set_dbg_changed(true);

	return ES_NORMAL;
}

Exec_stat MCVariableValue::split(char e, char k, MCExecPoint& ep)
{
	if (!ensure_string(ep))
		return ES_ERROR;

	if (is_empty())
		return ES_NORMAL;

	MCVariableArray v;
	v . split(get_string(), e, k);

	destroy();

	set_type(VF_ARRAY);
	array . taketable(&v);
	
	set_dbg_changed(true);

	return ES_NORMAL;
}

Exec_stat MCVariableValue::combine_column(char p_column_delimiter, char p_row_delimiter, MCExecPoint& ep)
{
	if (!is_array())
	{
		assign_empty();
		return ES_NORMAL;
	}

	// OK-2009-02-24: [[Bug 7617]] - To prevent crashes, just clear the the array if it's non-numeric.
	if (!array . isnumeric())
	{
		assign_empty();
		return ES_NORMAL;
	}

	char *t_buffer;
	uint32_t t_length;
	array . combine_column(ep, p_row_delimiter, p_column_delimiter, t_buffer, t_length);
	assign_buffer(t_buffer, t_length);
	
	set_dbg_changed(true);

	return ES_NORMAL;
}

Exec_stat MCVariableValue::split_column(char p_column_delimiter, char p_row_delimiter, MCExecPoint& ep)
{
	if (!ensure_string(ep))
		return ES_ERROR;

	if (is_empty())
		return ES_NORMAL;
	
	MCVariableArray v;
	v . split_column(get_string(), p_row_delimiter, p_column_delimiter);

	destroy();

	set_type(VF_ARRAY);
	array . taketable(&v);

	set_dbg_changed(true);
	
	return ES_NORMAL;
}

Exec_stat MCVariableValue::combine_as_set(char e, MCExecPoint& ep)
{
	if (!is_array())
	{
		assign_empty();
		return ES_NORMAL;
	}

	char *t_buffer;
	uint32_t t_length;
	array . combine_as_set(ep, e, t_buffer, t_length);
	assign_buffer(t_buffer, t_length);
	
	set_dbg_changed(true);

	return ES_NORMAL;
}

Exec_stat MCVariableValue::split_as_set(char e, MCExecPoint& ep)
{
	if (!ensure_string(ep))
		return ES_ERROR;

	if (is_empty())
		return ES_NORMAL;

	MCVariableArray v;
	v . split_as_set(get_string(), e);

	destroy();

	set_type(VF_ARRAY);
	array . taketable(&v);
	
	set_dbg_changed(true);

	return ES_NORMAL;
}

Exec_stat MCVariableValue::factorarray(MCExecPoint& ep, Operators op)
{
	if (!is_array())
		assign_empty();

	if (is_empty())
		return ES_NORMAL;

	return array . factorarray(ep, op);
}

// MERG-2013-08-26: [[ RecursiveArrayOp ]] Support nested arrays in union and intersect
Exec_stat MCVariableValue::unionarray(MCVariableValue& v, bool p_recursive)
{
	if (!is_array())
		assign_empty();

	if (!v . is_array())
		return ES_NORMAL;

	if (is_empty())
	{
		assign(v);
		return ES_NORMAL;
	}

	uint4 t_nfilled;
	t_nfilled = array . getnfilled();
	
	if (array . unionarray(v . array, p_recursive) != ES_NORMAL)
		return ES_ERROR;
	
	set_dbg_mutated(t_nfilled != array . getnfilled());
	
	return ES_NORMAL;
}

// MERG-2013-08-26: [[ RecursiveArrayOp ]] Support nested arrays in union and intersect
Exec_stat MCVariableValue::intersectarray(MCVariableValue& v, bool p_recursive)
{
	if (!is_array() || !v . is_array())
	{
		assign_empty();
		return ES_NORMAL;
	}
	
	uint4 t_nfilled;
	t_nfilled = array . getnfilled();
	
	if (array . intersectarray(v . array, p_recursive) != ES_NORMAL)
		return ES_ERROR;
	
	set_dbg_mutated(t_nfilled != array . getnfilled());
	
	return ES_NORMAL;
}

Exec_stat MCVariableValue::transpose(MCVariableValue& v)
{
	if (!v . is_array())
	{
		assign_empty();
		return ES_NORMAL;
	}

	// MW-2009-01-22: [[ Bug 7240 ]] Probably a good idea to attempt to transpose 'v'
	//   rather than this's array (which is undefined!).
	MCVariableArray vt;
	Exec_stat stat;
	stat = vt . transpose(v . array);
	if (stat != ES_NORMAL)
		return stat;

	destroy();

	set_type(VF_ARRAY);
	array . taketable(&vt);
	
	set_dbg_mutated(true);

	return ES_NORMAL;
}

Exec_stat MCVariableValue::matrixmultiply(MCExecPoint& ep, MCVariableValue& va, MCVariableValue& vb)
{
	if (!va . is_array() || !vb . is_array())
	{
		assign_empty();
		return ES_NORMAL;
	}

	MCVariableArray vc;
	Exec_stat stat;
	stat = vc . matrixmultiply(ep, va . array, vb . array);
	if (stat != ES_NORMAL)
		return stat;

	destroy();

	set_type(VF_ARRAY);
	array . taketable(&vc);
	
	set_dbg_mutated(true);

	return ES_NORMAL;
}

Exec_stat MCVariableValue::setprops(uint4 parid, MCObject *optr)
{
	if (!is_array())
		return ES_NORMAL;

	return array . setprops(parid, optr);
}

IO_stat MCVariableValue::loadkeys(IO_header *stream, bool p_merge)
{
	if (!p_merge)
		destroy();

	IO_stat t_stat;

	set_type(VF_ARRAY);

	t_stat = array . loadkeys(stream, p_merge);
	if (!p_merge && array . getnfilled() == 0)
	{
		set_type(VF_UNDEFINED);
		strnum . buffer . data = NULL;
		strnum . buffer . size = 0;
	}

	set_dbg_mutated(true);
	
	return t_stat;
}

IO_stat MCVariableValue::savekeys(IO_header *stream)
{
	if (is_array())
		return array . savekeys(stream);

	return IO_write_uint4(0, stream);
}

IO_stat MCVariableValue::loadarray(MCObjectInputStream& p_stream, bool p_merge)
{
	if (!p_merge)
		destroy();
	
	set_type(VF_ARRAY);

	IO_stat t_stat;
	t_stat = array . load(p_stream, p_merge);
	if (!p_merge && array . getnfilled() == 0)
	{
		set_type(VF_UNDEFINED);

		strnum . buffer . data = NULL;
		strnum . buffer . size = 0;
	}
	else
		set_dbg_mutated(true);

	return t_stat;
}

MCHashentry *MCVariableArray::lookupindex(uint32_t p_index, Boolean add)
{
	char t_buffer[U4L];
	sprintf(t_buffer, "%u", p_index);
	return lookuphash(t_buffer, True, add);
}

///////////////////////////////////////////////////////////////////////////////
