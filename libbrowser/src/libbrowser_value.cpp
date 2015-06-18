/* Copyright (C) 2003-2015 Runtime Revolution Ltd.
 
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

#include "libbrowser.h"

////////////////////////////////////////////////////////////////////////////////

struct MCBrowserValue
{
	MCBrowserValueType type;
	union
	{
		bool boolean;
		int32_t integer;
		double_t double_val;
		char *utf8_string;
		MCBrowserListRef array;
	};
};

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
		
		default:
			break;
	}
	
	p_value.type = kMCBrowserValueTypeNone;
}

bool MCBrowserValueSetBoolean(MCBrowserValue &self, bool p_value)
{
	MCBrowserValueClear(self);
	self.type = kMCBrowserValueTypeInteger;
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

bool MCBrowserValueSetDouble(MCBrowserValue &self, double_t p_value)
{
	MCBrowserValueClear(self);
	self.type = kMCBrowserValueTypeDouble;
	self.double_val = p_value;
	
	return true;
}

bool MCBrowserValueGetDouble(MCBrowserValue &self, double_t &r_value)
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
	
	r_value = self.utf8_string;
	return true;
}

bool MCBrowserValueSetList(MCBrowserValue &self, MCBrowserListRef p_value)
{
	MCBrowserListRef t_array;
	t_array = MCBrowserListRetain(p_value);
	MCBrowserValueClear(self);
	self.type = kMCBrowserValueTypeList;
	self.array = t_array;
}

bool MCBrowserValueGetList(MCBrowserValue &self, MCBrowserListRef &r_value)
{
	if (self.type != kMCBrowserValueTypeList)
		return false;
	
	r_value = self.array;
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
		MCMemoryDeleteArray(m_elements);
	}
	
	uint32_t GetSize()
	{
		return m_size;
	}
	
	bool Expand(uint32_t p_size)
	{
		if (p_size <= m_size)
			return true;
		
		return MCMemoryResizeArray(p_size, m_elements, m_size);
	}
	
	bool GetType(uint32_t p_index, MCBrowserValueType &r_type)
	{
		if (p_index >= m_size)
			return false;
		
		r_type = m_elements[p_index].type;
		return true;
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
	
	bool SetDouble(uint32_t p_index, double_t p_value)
	{
		if (p_index >= m_size)
			return false;
		
		MCBrowserValueSetDouble(m_elements[p_index], p_value);
	}

	bool GetDouble(uint32_t p_index, double_t &r_value)
	{
		if (p_index >= m_size)
			return false;
		
		return MCBrowserValueGetDouble(m_elements[p_index], r_value);
	}
	
	bool SetUTF8String(uint32_t p_index, const char *p_value)
	{
		if (p_index >= m_size)
			return false;
		
		MCBrowserValueSetUTF8String(m_elements[p_index], p_value);
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
		
		MCBrowserValueSetList(m_elements[p_index], p_value);
	}
	
	bool GetList(uint32_t p_index, MCBrowserListRef &r_value)
	{
		if (p_index >= m_size)
		return false;
		
		return MCBrowserValueGetList(m_elements[p_index], r_value);
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
	
	bool AppendDouble(double_t p_value)
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

private:
	MCBrowserValue *m_elements;
	uint32_t m_size;
};

////////////////////////////////////////////////////////////////////////////////

bool MCBrowserListCreate(MCBrowserListRef &r_browser, uint32_t p_size)
{
	MCBrowserList *t_browser;
	t_browser = new MCBrowserList();
	if (t_browser == nil)
		return false;
	
	if (!t_browser->Expand(p_size))
	{
		delete t_browser;
		return false;
	}
	
	r_browser = (MCBrowserListRef)t_browser;
	return true;
}

MCBrowserListRef MCBrowserListRetain(MCBrowserListRef p_list)
{
	if (p_list != nil)
		((MCBrowserList*)p_list)->Retain();
	
	return p_list;
}

void MCBrowserListRelease(MCBrowserListRef p_list)
{
	if (p_list != nil)
		((MCBrowserList*)p_list)->Release();
}

bool MCBrowserListGetSize(MCBrowserListRef p_list, uint32_t &r_size)
{
	if (p_list == nil)
		return false;
	
	r_size = ((MCBrowserList*)p_list)->GetSize();
	return true;
}

bool MCBrowserListGetType(MCBrowserListRef p_list, uint32_t p_index, MCBrowserValueType &r_type)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->GetType(p_index, r_type);
}

bool MCBrowserListSetBoolean(MCBrowserListRef p_list, uint32_t p_index, bool p_value)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->SetBoolean(p_index, p_value);
}

bool MCBrowserListSetInteger(MCBrowserListRef p_list, uint32_t p_index, int32_t p_value)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->SetInteger(p_index, p_value);
}

bool MCBrowserListSetDouble(MCBrowserListRef p_list, uint32_t p_index, double_t p_value)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->SetDouble(p_index, p_value);
}

bool MCBrowserListSetUTF8String(MCBrowserListRef p_list, uint32_t p_index, const char *p_value)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->SetUTF8String(p_index, p_value);
}

bool MCBrowserListSetList(MCBrowserListRef p_list, uint32_t p_index, MCBrowserListRef p_value)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->SetList(p_index, p_value);
}

bool MCBrowserListAppendBoolean(MCBrowserListRef p_list, bool p_value)
{
	if (p_list == nil)
	return false;
	
	return ((MCBrowserList*)p_list)->AppendBoolean(p_value);
}

bool MCBrowserListAppendInteger(MCBrowserListRef p_list, int32_t p_value)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->AppendInteger(p_value);
}

bool MCBrowserListAppendDouble(MCBrowserListRef p_list, double_t p_value)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->AppendDouble(p_value);
}

bool MCBrowserListAppendUTF8String(MCBrowserListRef p_list, const char *p_value)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->AppendUTF8String(p_value);
}

bool MCBrowserListAppendList(MCBrowserListRef p_list, MCBrowserListRef p_value)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->AppendList(p_value);
}

bool MCBrowserListGetBoolean(MCBrowserListRef p_list, uint32_t p_index, bool &r_value)
{
	if (p_list == nil)
	return false;
	
	return ((MCBrowserList*)p_list)->GetBoolean(p_index, r_value);
}

bool MCBrowserListGetInteger(MCBrowserListRef p_list, uint32_t p_index, int32_t &r_value)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->GetInteger(p_index, r_value);
}

bool MCBrowserListGetDouble(MCBrowserListRef p_list, uint32_t p_index, double_t &r_value)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->GetDouble(p_index, r_value);
}

bool MCBrowserListGetUTF8String(MCBrowserListRef p_list, uint32_t p_index, char *&r_value)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->GetUTF8String(p_index, r_value);
}

bool MCBrowserListGetList(MCBrowserListRef p_list, uint32_t p_index, MCBrowserListRef &r_value)
{
	if (p_list == nil)
		return false;
	
	return ((MCBrowserList*)p_list)->GetList(p_index, r_value);
}


////////////////////////////////////////////////////////////////////////////////
