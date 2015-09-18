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

#include "platform.h"

#include "globdefs.h"
#include "filedefs.h"
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "objdefs.h"

//#include "execpt.h"
#include "exec.h"

#include "desktop-dc.h"

#include "platform.h"

////////////////////////////////////////////////////////////////////////////////

MCSystemPasteboard::MCSystemPasteboard(MCPlatformPasteboardRef p_pasteboard)
{
	m_references = 1;
	
	m_generation = 0;
	m_pasteboard = p_pasteboard;
	MCPlatformPasteboardRetain(m_pasteboard);
	
	m_types = nil;
	m_entries = nil;
	m_entry_count = 0;
	m_valid = false;
	
	Resolve();
}

MCSystemPasteboard::~MCSystemPasteboard(void)
{
	MCMemoryDeleteArray(m_types);
	
	for(uindex_t i = 0; i < m_entry_count; i++)
		if (m_entries[i] . data != nil)
			MCValueRelease(m_entries[i] . data);
	MCMemoryDeleteArray(m_entries);
	
	MCPlatformPasteboardRelease(m_pasteboard);
}

void MCSystemPasteboard::Retain(void)
{
	m_references += 1;
}

void MCSystemPasteboard::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
		delete this;
}

bool MCSystemPasteboard::IsValid(void)
{
	if (m_valid &&
		m_generation != MCPlatformPasteboardGetGeneration(m_pasteboard))
		m_valid = false;
	
	return m_valid;
}

void MCSystemPasteboard::Resolve(void)
{
	MCPlatformPasteboardFlavor *t_flavors;
	uindex_t t_flavor_count;
	if (!MCPlatformPasteboardQuery(m_pasteboard, t_flavors, t_flavor_count))
		return;
	
	unsigned int t_objects_priority;
	t_objects_priority = 0xFFFFFFFFU;
	
	unsigned int t_image_priority;
	t_image_priority = 0xFFFFFFFFU;
	
	unsigned int t_text_priority;
	t_text_priority = 0xFFFFFFFFU;
	
	unsigned int t_files_priority;
	t_files_priority = 0xFFFFFFFFU;
	
	MCPlatformPasteboardFlavor t_objects_format;
	t_objects_format = kMCPlatformPasteboardFlavorNone;
	
	MCPlatformPasteboardFlavor t_image_format;
	t_image_format = kMCPlatformPasteboardFlavorNone;
	
	MCPlatformPasteboardFlavor t_text_format;
	t_text_format = kMCPlatformPasteboardFlavorNone;
	
	for(uindex_t i = 0; i < t_flavor_count; i++)
	{
		switch(t_flavors[i])
		{
			case kMCPlatformPasteboardFlavorUTF8:
				t_text_priority = MCU_min(i, t_text_priority);
				if (t_text_format == 0)
					t_text_format = kMCPlatformPasteboardFlavorUTF8;
				break;
				
			case kMCPlatformPasteboardFlavorRTF:
				t_text_priority = MCU_min(i, t_text_priority);
				if (t_text_format == 0 || t_text_format == kMCPlatformPasteboardFlavorUTF8)
					t_text_format = kMCPlatformPasteboardFlavorRTF;
				break;
				
			case kMCPlatformPasteboardFlavorHTML:
				t_text_priority = MCU_min(i, t_text_priority);
				if (t_text_format == 0 || t_text_format == kMCPlatformPasteboardFlavorUTF8)
					t_text_format = kMCPlatformPasteboardFlavorHTML;
				break;
				
			case kMCPlatformPasteboardFlavorStyledText:
				t_text_priority = MCU_min(i, t_text_priority);
				t_text_format = kMCPlatformPasteboardFlavorStyledText;
				break;
				
			case kMCPlatformPasteboardFlavorPNG:
				t_image_priority = MCU_min(i, t_image_priority);
				t_image_format = kMCPlatformPasteboardFlavorPNG;
				break;
				
			case kMCPlatformPasteboardFlavorJPEG:
				t_image_priority = MCU_min(i, t_image_priority);
				if (t_image_format == kMCPlatformPasteboardFlavorNone)
					t_image_format = kMCPlatformPasteboardFlavorJPEG;
				break;
				
			case kMCPlatformPasteboardFlavorGIF:
				t_image_priority = MCU_min(i, t_image_priority);
				if (t_image_format == kMCPlatformPasteboardFlavorNone)
					t_image_format = kMCPlatformPasteboardFlavorGIF;
				break;
				
			case kMCPlatformPasteboardFlavorFiles:
				t_files_priority = MCU_min(i, t_files_priority);
				break;
				
			case kMCPlatformPasteboardFlavorObjects:
				t_objects_priority = MCU_min(i, t_objects_priority);
				t_objects_format = kMCPlatformPasteboardFlavorObjects;
				break;
				
			default:
				// Do nothing.
				break;
		}
	}
	
	if (t_objects_priority != 0xFFFFFFFFU)
	{
		AddEntry(TRANSFER_TYPE_OBJECTS, t_objects_format);
		if (t_image_priority != 0xFFFFFFFFU)
			AddEntry(TRANSFER_TYPE_IMAGE, t_image_format);
	}
	else if (t_files_priority != 0xFFFFFFFFU)
		AddEntry(TRANSFER_TYPE_FILES, kMCPlatformPasteboardFlavorFiles);
	else if (t_text_priority != 0xFFFFFFFFU)
	{
		if (t_text_format == kMCPlatformPasteboardFlavorUTF8)
			AddEntry(TRANSFER_TYPE_UNICODE_TEXT, t_text_format);
		else
			AddEntry(TRANSFER_TYPE_STYLED_TEXT, t_text_format);
	}
	else if (t_image_priority != 0xFFFFFFFFU)
		AddEntry(TRANSFER_TYPE_IMAGE, t_image_format);
	
	MCMemoryDeleteArray(t_flavors);
	
	m_generation = MCPlatformPasteboardGetGeneration(m_pasteboard);
	m_valid = true;
}

void MCSystemPasteboard::AddEntry(MCTransferType p_type, MCPlatformPasteboardFlavor p_flavor)
{
	/* UNCHECKED */ MCMemoryResizeArray(m_entry_count + 1, m_entries, m_entry_count);
	m_entries[m_entry_count - 1] . type = p_type;
	m_entries[m_entry_count - 1] . flavor = p_flavor;
	m_entries[m_entry_count - 1] . data = nil;
}

bool MCSystemPasteboard::Query(MCTransferType*& r_types, size_t& r_type_count)
{
	if (!IsValid())
		return false;
	
	if (m_types == nil)
	{
		m_types = new MCTransferType[m_entry_count];
		for(uindex_t i = 0; i < m_entry_count; ++i)
			m_types[i] = m_entries[i] . type;
	}
	
	if (m_types != nil)
	{
		r_types = m_types;
		r_type_count = m_entry_count;
		return true;
	}
	
	return false;
}

bool MCSystemPasteboard::Fetch(MCTransferType p_type, MCDataRef& r_data)
{
	if (!IsValid())
		return false;
	
	uindex_t t_entry;
	for(t_entry = 0; t_entry < m_entry_count; ++t_entry)
		if (m_entries[t_entry] . type == p_type)
			break;
	
	if (t_entry == m_entry_count)
		return false;
	
	if (m_entries[t_entry] . data != nil)
	{
		r_data = MCValueRetain(m_entries[t_entry] . data);
		return true;
	}
	
	void *t_in_data_bytes;
	uindex_t t_in_data_byte_count;
	if (!MCPlatformPasteboardFetch(m_pasteboard, m_entries[t_entry] . flavor, t_in_data_bytes, t_in_data_byte_count))
		return false;
	
	MCAutoDataRef t_in_data;
	/* UNCHECKED */ MCDataCreateWithBytesAndRelease((byte_t*)t_in_data_bytes, t_in_data_byte_count, &t_in_data);
	
	MCAutoDataRef t_out_data;
	switch(m_entries[t_entry] . flavor)
	{
		case kMCPlatformPasteboardFlavorUTF8:
		{
            // AL-2014-07-11: [[ Bug 12792 ]] Always convert unicode pasteboard data to UTF16
            MCAutoStringRef t_input_mac;
            /* UNCHECKED */ MCStringDecode(*t_in_data, kMCStringEncodingUTF8, false, &t_input_mac);
            MCAutoStringRef t_output;
            /* UNCHECKED */ MCStringConvertLineEndingsToLiveCode(*t_input_mac, &t_output);
            /* UNCHECKED */ MCStringEncode(*t_output, kMCStringEncodingUTF16, false, &t_out_data);
		}
		break;
			
		case kMCPlatformPasteboardFlavorFiles:
		{
            MCAutoStringRef t_input_mac;
            /* UNCHECKED */ MCStringDecode(*t_in_data, kMCStringEncodingUTF8, false, &t_input_mac);
            MCAutoStringRef t_output;
            /* UNCHECKED */ MCStringConvertLineEndingsToLiveCode(*t_input_mac, &t_output);
            // SN-2014-11-13: [[ Bug 13993 ]] We now get a UTF-16 string to allow Unicode filenames
            /* UNCHECKED */ MCStringEncode(*t_output, kMCStringEncodingUTF16, false, &t_out_data);
		}
		break;
			
		case kMCPlatformPasteboardFlavorRTF:
			MCConvertRTFToStyledText(*t_in_data, &t_out_data);
			break;
			
		case kMCPlatformPasteboardFlavorHTML:
			MCConvertHTMLToStyledText(*t_in_data, &t_out_data);
			break;
			
		case kMCPlatformPasteboardFlavorPNG:
		case kMCPlatformPasteboardFlavorJPEG:
		case kMCPlatformPasteboardFlavorGIF:
		case kMCPlatformPasteboardFlavorObjects:
		case kMCPlatformPasteboardFlavorStyledText:
			t_out_data = *t_in_data;
			break;
			
		default:
			// We only need to handle the flavors resolve adds.
			assert(false);
			break;
	}
	
	if (*t_out_data == nil)
		return false;
	
	m_entries[t_entry] . data = MCValueRetain(*t_out_data);	
	r_data = MCValueRetain(m_entries[t_entry] . data);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
