/* Copyright (C) 2015 LiveCode Ltd.
 
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

#include <core.h>

#include "libbrowser.h"
#include "libbrowser_internal.h"

////////////////////////////////////////////////////////////////////////////////

void MCBrowserValueClear(MCBrowserValue &p_value)
{
	switch (p_value.type)
	{
		case kMCBrowserValueTypeUTF8String:
			MCCStringFree(p_value.utf8_string);
			break;
		
		case kMCBrowserValueTypeList:
			MCBrowserListRelease(p_value.array);
			break;
		
		case kMCBrowserValueTypeDictionary:
			MCBrowserDictionaryRelease(p_value.dictionary);
			break;
		
		default:
			break;
	}
	
	p_value.type = kMCBrowserValueTypeNone;
}

bool MCBrowserValueCopy(const MCBrowserValue &p_src, MCBrowserValue &r_dst)
{
	switch (p_src.type)
	{
		case kMCBrowserValueTypeBoolean:
			return MCBrowserValueSetBoolean(r_dst, p_src.boolean);
		
		case kMCBrowserValueTypeInteger:
			return MCBrowserValueSetInteger(r_dst, p_src.integer);
		
		case kMCBrowserValueTypeDouble:
			return MCBrowserValueSetDouble(r_dst, p_src.double_val);
		
		case kMCBrowserValueTypeNone:
			MCBrowserValueClear(r_dst);
			return true;
		
		case kMCBrowserValueTypeUTF8String:
			return MCBrowserValueSetUTF8String(r_dst, p_src.utf8_string);
		
		case kMCBrowserValueTypeList:
			return MCBrowserValueSetList(r_dst, p_src.array);
		
		case kMCBrowserValueTypeDictionary:
			return MCBrowserValueSetDictionary(r_dst, p_src.dictionary);
		
		default:
			// unknown type
			break;
	}
	
	return false;
}

bool MCBrowserValueSetBoolean(MCBrowserValue &self, bool p_value)
{
	MCBrowserValueClear(self);
	self.type = kMCBrowserValueTypeBoolean;
	self.boolean = p_value;
	
	return true;
}

bool MCBrowserValueGetBoolean(MCBrowserValue &self, bool &r_value)
{
	if (self.type != kMCBrowserValueTypeBoolean)
		return false;
	
	r_value = self.boolean;
	return true;
}

bool MCBrowserValueSetInteger(MCBrowserValue &self, int32_t p_value)
{
	MCBrowserValueClear(self);
	self.type = kMCBrowserValueTypeInteger;
	self.integer = p_value;
	
	return true;
}

bool MCBrowserValueGetInteger(MCBrowserValue &self, int32_t &r_value)
{
	if (self.type != kMCBrowserValueTypeInteger)
		return false;
	
	r_value = self.integer;
	return true;
}

bool MCBrowserValueSetDouble(MCBrowserValue &self, double p_value)
{
	MCBrowserValueClear(self);
	self.type = kMCBrowserValueTypeDouble;
	self.double_val = p_value;
	
	return true;
}

bool MCBrowserValueGetDouble(MCBrowserValue &self, double &r_value)
{
	if (self.type != kMCBrowserValueTypeDouble)
		return false;
	
	r_value = self.double_val;
	return true;
}

bool MCBrowserValueSetUTF8String(MCBrowserValue &self, const char *p_value)
{
	char *t_string;
	t_string = nil;
	
	if (!MCCStringClone(p_value, t_string))
		return false;
	
	MCBrowserValueClear(self);
	self.type = kMCBrowserValueTypeUTF8String;
	self.utf8_string = t_string;
	
	return true;
}

bool MCBrowserValueGetUTF8String(MCBrowserValue &self, char *&r_value)
{
	if (self.type != kMCBrowserValueTypeUTF8String)
		return false;
	
	return MCCStringClone(self.utf8_string, r_value);
}

bool MCBrowserValueSetList(MCBrowserValue &self, MCBrowserListRef p_value)
{
	MCBrowserListRef t_array;
	t_array = MCBrowserListRetain(p_value);
	MCBrowserValueClear(self);
	self.type = kMCBrowserValueTypeList;
	self.array = t_array;

	return true;
}

bool MCBrowserValueGetList(MCBrowserValue &self, MCBrowserListRef &r_value)
{
	if (self.type != kMCBrowserValueTypeList)
		return false;
	
	r_value = self.array;
	return true;
}

bool MCBrowserValueSetDictionary(MCBrowserValue &self, MCBrowserDictionaryRef p_value)
{
	MCBrowserDictionaryRef t_dict;
	t_dict = MCBrowserDictionaryRetain(p_value);
	MCBrowserValueClear(self);
	self.type = kMCBrowserValueTypeDictionary;
	self.dictionary = t_dict;
	
	return true;
}

bool MCBrowserValueGetDictionary(MCBrowserValue &self, MCBrowserDictionaryRef &r_value)
{
	if (self.type != kMCBrowserValueTypeDictionary)
		return false;
	
	r_value = self.dictionary;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

class MCBrowserList : public MCBrowserRefCounted
{
public:
	MCBrowserList()
	{
		m_elements = nil;
		m_size = 0;
	}
	
	virtual ~MCBrowserList()
	{
		for (uint32_t i = 0; i < m_size; i++)
			MCBrowserValueClear(m_elements[i]);
		MCBrowserMemoryDeleteArray(m_elements);
	}
	
	uint32_t GetSize()
	{
		return m_size;
	}
	
	bool Expand(uint32_t p_size)
	{
		if (p_size <= m_size)
			return true;
		
		return MCBrowserMemoryResizeArray(p_size, m_elements, m_size);
	}
	
	bool GetType(uint32_t p_index, MCBrowserValueType &r_type)
	{
		if (p_index >= m_size)
			return false;
		
		r_type = m_elements[p_index].type;
		return true;
	}
	
	bool SetValue(uint32_t p_index, const MCBrowserValue &p_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueCopy(p_value, m_elements[p_index]);
	}
	
	bool GetValue(uint32_t p_index, MCBrowserValue &r_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueCopy(m_elements[p_index], r_value);
	}
	
	bool SetBoolean(uint32_t p_index, bool p_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueSetBoolean(m_elements[p_index], p_value);
	}
	
	bool GetBoolean(uint32_t p_index, bool &r_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueGetBoolean(m_elements[p_index], r_value);
	}
	
	bool SetInteger(uint32_t p_index, int32_t p_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueSetInteger(m_elements[p_index], p_value);
	}
	
	bool GetInteger(uint32_t p_index, int32_t &r_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueGetInteger(m_elements[p_index], r_value);
	}
	
	bool SetDouble(uint32_t p_index, double p_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueSetDouble(m_elements[p_index], p_value);
	}

	bool GetDouble(uint32_t p_index, double &r_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueGetDouble(m_elements[p_index], r_value);
	}
	
	bool SetUTF8String(uint32_t p_index, const char *p_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueSetUTF8String(m_elements[p_index], p_value);
	}
	
	bool GetUTF8String(uint32_t p_index, char *&r_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueGetUTF8String(m_elements[p_index], r_value);
	}
	
	bool SetList(uint32_t p_index, MCBrowserListRef p_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueSetList(m_elements[p_index], p_value);
	}
	
	bool GetList(uint32_t p_index, MCBrowserListRef &r_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueGetList(m_elements[p_index], r_value);
	}
	
	bool SetDictionary(uint32_t p_index, MCBrowserDictionaryRef p_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueSetDictionary(m_elements[p_index], p_value);
	}
	
	bool GetDictionary(uint32_t p_index, MCBrowserDictionaryRef &r_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueGetDictionary(m_elements[p_index], r_value);
	}
	
	bool AppendValue(const MCBrowserValue &p_value)
	{
		uint32_t t_index;
		t_index = m_size;
		
		return Expand(m_size + 1) && SetValue(t_index, p_value);
	}
	
	bool AppendBoolean(bool p_value)
	{
		uint32_t t_index;
		t_index = m_size;
		
		return Expand(m_size + 1) && SetBoolean(t_index, p_value);
	}
	
	bool AppendInteger(int32_t p_value)
	{
		uint32_t t_index;
		t_index = m_size;
		
		return Expand(m_size + 1) && SetInteger(t_index, p_value);
	}
	
	bool AppendDouble(double p_value)
	{
		uint32_t t_index;
		t_index = m_size;
		
		return Expand(m_size + 1) && SetDouble(t_index, p_value);
	}
	
	bool AppendUTF8String(const char *p_value)
	{
		uint32_t t_index;
		t_index = m_size;
		
		return Expand(m_size + 1) && SetUTF8String(t_index, p_value);
	}
	
	bool AppendList(MCBrowserListRef p_value)
	{
		uint32_t t_index;
		t_index = m_size;
		
		return Expand(m_size + 1) && SetList(t_index, p_value);
	}
	
	bool AppendDictionary(MCBrowserDictionaryRef p_value)
	{
		uint32_t t_index;
		t_index = m_size;
		
		return Expand(m_size + 1) && SetDictionary(t_index, p_value);
	}

private:
	MCBrowserValue *m_elements;
	uint32_t m_size;
};

////////////////////////////////////////////////////////////////////////////////

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListCreate(MCBrowserListRef &r_browser, uint32_t p_size)
{
	MCBrowserList *t_browser;
	t_browser = new (nothrow) MCBrowserList();
	if (t_browser == nil)
		return false;
	
	if (!t_browser->Expand(p_size))
	{
		delete t_browser;
		return false;
	}
	
	r_browser = t_browser;
	return true;
}

MC_BROWSER_DLLEXPORT_DEF
MCBrowserListRef MCBrowserListRetain(MCBrowserListRef p_list)
{
	if (p_list != nil)
		p_list->Retain();
	
	return p_list;
}

MC_BROWSER_DLLEXPORT_DEF
void MCBrowserListRelease(MCBrowserListRef p_list)
{
	if (p_list != nil)
		p_list->Release();
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListGetSize(MCBrowserListRef p_list, uint32_t &r_size)
{
	if (p_list == nil)
		return false;
	
	r_size = p_list->GetSize();
	return true;
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListGetType(MCBrowserListRef p_list, uint32_t p_index, MCBrowserValueType &r_type)
{
	if (p_list == nil)
		return false;
	
	return p_list->GetType(p_index, r_type);
}

bool MCBrowserListSetValue(MCBrowserListRef p_list, uint32_t p_index, const MCBrowserValue &p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->SetValue(p_index, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListSetBoolean(MCBrowserListRef p_list, uint32_t p_index, bool p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->SetBoolean(p_index, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListSetInteger(MCBrowserListRef p_list, uint32_t p_index, int32_t p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->SetInteger(p_index, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListSetDouble(MCBrowserListRef p_list, uint32_t p_index, double p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->SetDouble(p_index, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListSetUTF8String(MCBrowserListRef p_list, uint32_t p_index, const char *p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->SetUTF8String(p_index, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListSetList(MCBrowserListRef p_list, uint32_t p_index, MCBrowserListRef p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->SetList(p_index, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListSetDictionary(MCBrowserListRef p_list, uint32_t p_index, MCBrowserDictionaryRef p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->SetDictionary(p_index, p_value);
}

bool MCBrowserListAppendValue(MCBrowserListRef p_list, const MCBrowserValue &p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->AppendValue(p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListAppendBoolean(MCBrowserListRef p_list, bool p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->AppendBoolean(p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListAppendInteger(MCBrowserListRef p_list, int32_t p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->AppendInteger(p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListAppendDouble(MCBrowserListRef p_list, double p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->AppendDouble(p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListAppendUTF8String(MCBrowserListRef p_list, const char *p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->AppendUTF8String(p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListAppendList(MCBrowserListRef p_list, MCBrowserListRef p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->AppendList(p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListAppendDictionary(MCBrowserListRef p_list, MCBrowserDictionaryRef p_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->AppendDictionary(p_value);
}

bool MCBrowserListGetValue(MCBrowserListRef p_list, uint32_t p_index, MCBrowserValue &r_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->GetValue(p_index, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListGetBoolean(MCBrowserListRef p_list, uint32_t p_index, bool &r_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->GetBoolean(p_index, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListGetInteger(MCBrowserListRef p_list, uint32_t p_index, int32_t &r_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->GetInteger(p_index, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListGetDouble(MCBrowserListRef p_list, uint32_t p_index, double &r_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->GetDouble(p_index, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListGetUTF8String(MCBrowserListRef p_list, uint32_t p_index, char *&r_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->GetUTF8String(p_index, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListGetList(MCBrowserListRef p_list, uint32_t p_index, MCBrowserListRef &r_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->GetList(p_index, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserListGetDictionary(MCBrowserListRef p_list, uint32_t p_index, MCBrowserDictionaryRef &r_value)
{
	if (p_list == nil)
		return false;
	
	return p_list->GetDictionary(p_index, r_value);
}

////////////////////////////////////////////////////////////////////////////////

class MCBrowserDictionary : public MCBrowserRefCounted
{
public:
	MCBrowserDictionary()
	{
		m_elements = nil;
		m_keys = nil;
		m_capacity = 0;
		m_size = 0;
	}
	
	virtual ~MCBrowserDictionary()
	{
		for (uint32_t i = 0; i < m_size; i++)
			MCBrowserValueClear(m_elements[i]);
		for (uint32_t i = 0; i < m_size; i++)
			MCCStringFree(m_keys[i]);
			
		MCBrowserMemoryDeleteArray(m_elements);
		MCBrowserMemoryDeleteArray(m_keys);
	}
	
	bool GetKeys(char **&r_keys, uint32_t &r_count)
	{
		r_keys = m_keys;
		r_count = m_size;
		
		return true;
	}
	
	bool Expand(uint32_t p_size)
	{
		if (p_size <= m_capacity)
			return true;
		
		uindex_t t_element_capacity, t_key_capacity;
		// Need to pass original capacity to BOTH calls to MCBrowserMemoryResizeArray, to ensure memory is appropriately cleared.
		t_element_capacity = t_key_capacity = m_capacity;
		if (MCBrowserMemoryResizeArray(p_size, m_elements, t_element_capacity) && MCBrowserMemoryResizeArray(p_size, m_keys, t_key_capacity))
		{
			m_capacity = t_element_capacity;
			return true;
		}
		
		return false;
	}
	
	bool FindElement(const char *p_key, uint32_t &r_index)
	{
		for (uint32_t i = 0; i < m_size; i++)
		{
			if (MCCStringEqualCaseless(m_keys[i], p_key))
			{
				r_index = i;
				return true;
			}
		}
		
		return false;
	}
	
	bool EnsureElement(const char *p_key, uint32_t &r_index)
	{
		if (FindElement(p_key, r_index))
			return true;
		
		if (!Expand(m_size + 1))
			return false;
		
		if (!MCCStringClone(p_key, m_keys[m_size]))
			return false;
		
		r_index = m_size;
		m_size += 1;
		
		return true;
	}
	
	bool GetType(const char *p_key, MCBrowserValueType &r_type)
	{
		uint32_t t_index;
		if (!FindElement(p_key, t_index))
			return false;
		
		r_type = m_elements[t_index].type;
		return true;
	}
	
	bool SetValue(const char *p_key, const MCBrowserValue &p_value)
	{
		uint32_t t_index;
		if (!EnsureElement(p_key, t_index))
			return false;
		
		return MCBrowserValueCopy(p_value, m_elements[t_index]);
	}
	
	bool GetValue(const char *p_key, MCBrowserValue &r_value)
	{
		uint32_t t_index;
		if (!FindElement(p_key, t_index))
			return false;
		
		return MCBrowserValueCopy(m_elements[t_index], r_value);
	}
	
	bool SetBoolean(const char *p_key, bool p_value)
	{
		uint32_t t_index;
		if (!EnsureElement(p_key, t_index))
			return false;
		
		return MCBrowserValueSetBoolean(m_elements[t_index], p_value);
	}
	
	bool GetBoolean(const char *p_key, bool &r_value)
	{
		uint32_t t_index;
		if (!FindElement(p_key, t_index))
			return false;
		
		return MCBrowserValueGetBoolean(m_elements[t_index], r_value);
	}
	
	bool SetInteger(const char *p_key, int32_t p_value)
	{
		uint32_t t_index;
		if (!EnsureElement(p_key, t_index))
			return false;
		
		return MCBrowserValueSetInteger(m_elements[t_index], p_value);
	}
	
	bool GetInteger(const char *p_key, int32_t &r_value)
	{
		uint32_t t_index;
		if (!FindElement(p_key, t_index))
			return false;
		
		return MCBrowserValueGetInteger(m_elements[t_index], r_value);
	}
	
	bool SetDouble(const char *p_key, double p_value)
	{
		uint32_t t_index;
		if (!EnsureElement(p_key, t_index))
			return false;
		
		return MCBrowserValueSetDouble(m_elements[t_index], p_value);
	}

	bool GetDouble(const char *p_key, double &r_value)
	{
		uint32_t t_index;
		if (!FindElement(p_key, t_index))
			return false;
		
		return MCBrowserValueGetDouble(m_elements[t_index], r_value);
	}
	
	bool SetUTF8String(const char *p_key, const char *p_value)
	{
		uint32_t t_index;
		if (!EnsureElement(p_key, t_index))
			return false;
		
		return MCBrowserValueSetUTF8String(m_elements[t_index], p_value);
	}
	
	bool GetUTF8String(const char *p_key, char *&r_value)
	{
		uint32_t t_index;
		if (!FindElement(p_key, t_index))
			return false;
		
		return MCBrowserValueGetUTF8String(m_elements[t_index], r_value);
	}
	
	bool SetList(const char *p_key, MCBrowserListRef p_value)
	{
		uint32_t t_index;
		if (!EnsureElement(p_key, t_index))
			return false;
		
		return MCBrowserValueSetList(m_elements[t_index], p_value);
	}
	
	bool GetList(const char *p_key, MCBrowserListRef &r_value)
	{
		uint32_t t_index;
		if (!FindElement(p_key, t_index))
			return false;
		
		return MCBrowserValueGetList(m_elements[t_index], r_value);
	}
	
	bool SetDictionary(const char *p_key, MCBrowserDictionaryRef p_value)
	{
		uint32_t t_index;
		if (!EnsureElement(p_key, t_index))
			return false;
		
		return MCBrowserValueSetDictionary(m_elements[t_index], p_value);
	}
	
	bool GetDictionary(const char *p_key, MCBrowserDictionaryRef &r_value)
	{
		uint32_t t_index;
		if (!FindElement(p_key, t_index))
			return false;
		
		return MCBrowserValueGetDictionary(m_elements[t_index], r_value);
	}
	
private:
	MCBrowserValue *m_elements;
	char **m_keys;
	uint32_t m_capacity;
	uint32_t m_size;
};

////////////////////////////////////////////////////////////////////////////////

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionaryCreate(MCBrowserDictionaryRef &r_dict, uint32_t p_size)
{
	MCBrowserDictionary *t_dict;
	t_dict = new (nothrow) MCBrowserDictionary();
	if (t_dict == nil)
		return false;
	
	if (!t_dict->Expand(p_size))
	{
		delete t_dict;
		return false;
	}
	
	r_dict = t_dict;
	return true;
}

MC_BROWSER_DLLEXPORT_DEF
MCBrowserDictionaryRef MCBrowserDictionaryRetain(MCBrowserDictionaryRef p_dictionary)
{
	if (p_dictionary != nil)
		p_dictionary->Retain();
	
	return p_dictionary;
}

MC_BROWSER_DLLEXPORT_DEF
void MCBrowserDictionaryRelease(MCBrowserDictionaryRef p_dictionary)
{
	if (p_dictionary != nil)
		p_dictionary->Release();
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionaryGetKeys(MCBrowserDictionaryRef p_dictionary, char **&r_keys, uint32_t &r_count)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->GetKeys(r_keys, r_count);
}

/* WORKAROUND - Lack of Pointer dereferencing or C Array abstraction in LCB */
MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionaryGetKeyCount(MCBrowserDictionaryRef p_dictionary, uint32_t &r_count)
{
	char **t_keys;
	return MCBrowserDictionaryGetKeys(p_dictionary, t_keys, r_count);
}

/* WORKAROUND - Lack of Pointer dereferencing or C Array abstraction in LCB */
MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionaryGetKey(MCBrowserDictionaryRef p_dictionary, uint32_t p_index, char *&r_key)
{
	char **t_keys;
	uint32_t t_count;
	
	if (!MCBrowserDictionaryGetKeys(p_dictionary, t_keys, t_count))
		return false;
	
	if (p_index >= t_count)
		return false;
	
	return MCCStringClone(t_keys[p_index], r_key);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionaryGetType(MCBrowserDictionaryRef p_dictionary, const char *p_key, MCBrowserValueType &r_type)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->GetType(p_key, r_type);
}

bool MCBrowserDictionarySetValue(MCBrowserDictionaryRef p_dictionary, const char *p_key, const MCBrowserValue &p_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->SetValue(p_key, p_value);
}

bool MCBrowserDictionaryGetValue(MCBrowserDictionaryRef p_dictionary, const char *p_key, MCBrowserValue &r_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->GetValue(p_key, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionarySetBoolean(MCBrowserDictionaryRef p_dictionary, const char *p_key, bool p_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->SetBoolean(p_key, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionarySetInteger(MCBrowserDictionaryRef p_dictionary, const char *p_key, int32_t p_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->SetInteger(p_key, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionarySetDouble(MCBrowserDictionaryRef p_dictionary, const char *p_key, double p_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->SetDouble(p_key, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionarySetUTF8String(MCBrowserDictionaryRef p_dictionary, const char *p_key, const char *p_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->SetUTF8String(p_key, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionarySetList(MCBrowserDictionaryRef p_dictionary, const char *p_key, MCBrowserListRef p_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->SetList(p_key, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionarySetDictionary(MCBrowserDictionaryRef p_dictionary, const char *p_key, MCBrowserDictionaryRef p_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->SetDictionary(p_key, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionaryGetBoolean(MCBrowserDictionaryRef p_dictionary, const char *p_key, bool &r_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->GetBoolean(p_key, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionaryGetInteger(MCBrowserDictionaryRef p_dictionary, const char *p_key, int32_t &r_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->GetInteger(p_key, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionaryGetDouble(MCBrowserDictionaryRef p_dictionary, const char *p_key, double &r_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->GetDouble(p_key, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionaryGetUTF8String(MCBrowserDictionaryRef p_dictionary, const char *p_key, char *&r_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->GetUTF8String(p_key, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionaryGetList(MCBrowserDictionaryRef p_dictionary, const char *p_key, MCBrowserListRef &r_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->GetList(p_key, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserDictionaryGetDictionary(MCBrowserDictionaryRef p_dictionary, const char *p_key, MCBrowserDictionaryRef &r_value)
{
	if (p_dictionary == nil)
		return false;
	
	return p_dictionary->GetDictionary(p_key, r_value);
}

////////////////////////////////////////////////////////////////////////////////
