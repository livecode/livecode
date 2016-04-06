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

#ifndef __MC_VARIABLE_IMPLEMENTATION__
#define __MC_VARIABLE_IMPLEMENTATION__

//

extern const char *MCnullstring;
extern uint4 MCU_r8tos(char *&sptr, uint4 &s, real8 n,uint2 fw, uint2 trailing, uint2 force);
extern Boolean MCU_stor8(const MCString&, real8& d, Boolean co);

//

inline Value_format MCVariableValue::get_type(void) const
{
	return (Value_format)_type;
}

inline void MCVariableValue::set_type(Value_format p_new_type)
{
	_type = (uint8_t)p_new_type;
}

inline bool MCVariableValue::get_external(void) const
{
	return (_flags & kExternalBit) != 0;
}

inline void MCVariableValue::set_external(void)
{
	_flags |= kExternalBit;
}

inline bool MCVariableValue::get_temporary(void) const
{
	return (_flags & kTemporaryBit) != 0;
}

inline void MCVariableValue::set_temporary(void)
{
	_flags |= kTemporaryBit;
}

//

#ifdef _IREVIAM
inline void MCVariableValue::set_dbg_notify(bool p_state)
{
	if (p_state)
		_flags = (_flags & ~(kDebugChangedBit | kDebugMutatedBit)) | kDebugNotifyBit;
	else
		_flags &= ~(kDebugNotifyBit | kDebugChangedBit | kDebugMutatedBit);
}

inline bool MCVariableValue::get_dbg_notify(void) const
{
	return (_flags & kDebugNotifyBit) != 0;
}

inline void MCVariableValue::set_dbg_changed(bool p_state)
{
	if (p_state)
		_flags |= kDebugChangedBit;
	else
		_flags &= ~kDebugChangedBit;
}

inline bool MCVariableValue::get_dbg_changed(void) const
{
	return (_flags & kDebugChangedBit) != 0;
}

inline void MCVariableValue::set_dbg_mutated(bool p_state)
{
	if (p_state)
		_flags |= kDebugMutatedBit;
	else
		_flags &= ~kDebugMutatedBit;
}

inline bool MCVariableValue::get_dbg_mutated(void) const
{
	return (_flags & kDebugMutatedBit) != 0;
}
#else
inline void MCVariableValue::set_dbg_notify(bool p_state)
{
}

inline bool MCVariableValue::get_dbg_notify(void) const
{
	return false;
}

inline void MCVariableValue::set_dbg_changed(bool p_state)
{
}

inline bool MCVariableValue::get_dbg_changed(void) const
{
	return false;
}

inline void MCVariableValue::set_dbg_mutated(bool p_state)
{
}

inline bool MCVariableValue::get_dbg_mutated(void) const
{
	return false;
}
#endif

//

inline MCVariableValue::MCVariableValue(void)
{
	set_type(VF_UNDEFINED);
	_flags = 0;

	strnum . buffer . data = NULL;
	strnum . buffer . size = 0;
}

inline MCVariableValue::MCVariableValue(const MCVariableValue& p_other)
{
	copy(p_other);
	_flags = 0;
}

inline MCVariableValue::~MCVariableValue(void)
{
	destroy();

#ifdef _IREVIAM
	extern void MCDebugNotifyValueDeleted(MCVariableValue *);
	if (get_dbg_notify())
		MCDebugNotifyValueDeleted(this);
#endif
}

//

inline bool MCVariableValue::is_clear(void) const
{
	return get_type() == VF_UNDEFINED;
}

inline bool MCVariableValue::is_undefined(void) const
{
	return is_clear();
}

// MW-2014-04-10: [[ Bug 12170 ]] A 'both' var with empty string is empty.
inline bool MCVariableValue::is_empty(void) const
{
	Value_format t_type;
	t_type = get_type();
	return t_type == VF_UNDEFINED || ((t_type == VF_STRING || t_type == VF_BOTH) && strnum . svalue . length == 0);
}

inline bool MCVariableValue::is_string(void) const
{
	Value_format t_type;
	t_type = get_type();
	return t_type == VF_STRING || t_type == VF_BOTH;
}

inline bool MCVariableValue::is_real(void) const
{
	// MW-2009-04-08: Abstract access to 'type' field.
	Value_format t_type;
	t_type = get_type();
	return t_type == VF_NUMBER || t_type == VF_BOTH;
}

inline bool MCVariableValue::is_number(void) const
{
	return is_real();
}

inline bool MCVariableValue::is_array(void) const
{
	return get_type() == VF_ARRAY;
}

//

inline Value_format MCVariableValue::get_format(void) const
{
	return get_type();
}

//

inline MCString MCVariableValue::get_string(void) const
{
	assert(is_string());
	return MCString(strnum . svalue . string, strnum . svalue . length);
}

inline real64_t MCVariableValue::get_real(void) const
{
	assert(is_real());
	return strnum . nvalue;
}

inline MCVariableArray* MCVariableValue::get_array(void)
{
	assert(is_array());
	return &array;
}

// Special method for use with externals api to fetch the string value of the
// variable. Shoudl only be used afer 'assign_custom_both'.
inline MCString MCVariableValue::get_custom_string(void) const
{
	return MCString(strnum . svalue . string, strnum . svalue . length);
}

//

inline void MCVariableValue::clear(void)
{
	destroy();

	set_type(VF_UNDEFINED);

	strnum . buffer . data = NULL;
	strnum . buffer . size = 0;
	
	set_dbg_changed(true);
}

inline bool MCVariableValue::assign(const MCVariableValue& v)
{
	destroy();
	return copy(v);
}

//

inline bool MCVariableValue::has_element(MCExecPoint& ep, const MCString& key)
{
	assert(is_array());
	return array . lookuphash(key, ep . getcasesensitive(), False) != NULL;
}

inline bool MCVariableValue::ensure_real(MCExecPoint& ep)
{
	if (is_number())
		return true;

	return coerce_to_real(ep);
}

inline bool MCVariableValue::ensure_string(MCExecPoint& ep)
{
	if (is_string())
		return true;

	return coerce_to_string(ep);
}

//

inline void MCVariableValue::getextents(MCExecPoint& ep)
{
	if (!is_array())
	{
		ep . clear();
		return;
	}

	array . getextents(ep);
}

inline void MCVariableValue::getkeys(MCExecPoint& ep)
{
	if (!is_array())
	{
		ep . clear();
		return;
	}

	array . getkeys(ep);
}

inline void MCVariableValue::getkeys(char **r_keys, uint32_t p_count)
{
	if (!is_array())
		return;

	array . getkeys(r_keys, p_count);
}

//

inline void MCVariableValue::destroy(void)
{
	if (get_type() != VF_ARRAY)
		free(strnum . buffer . data);
	else
		array . freehash();
}

//

inline Boolean MCVariableArray::issequence(void)
{
	return isnumeric() && dimensions == 1 && extents[0] . minimum == 1;
}

#endif
