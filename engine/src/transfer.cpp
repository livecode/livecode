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

//#include "execpt.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "field.h"
#include "paragraf.h"
#include "image.h"
#include "mcerror.h"
#include "undolst.h"
#include "util.h"
#include "transfer.h"
#include "styledtext.h"

#include "globals.h"

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    MCLocalPasteboard
//

MCLocalPasteboard::MCLocalPasteboard(void)
{
	m_references = 1;
	m_count = 0;
	m_types = NULL;
	m_datas = NULL;
}

MCLocalPasteboard::~MCLocalPasteboard(void)
{
	delete m_types;
	for(uint4 i = 0; i < m_count; ++i)
		if (m_datas[i] != NULL)
			MCValueRelease(m_datas[i]);

	delete m_datas;
}

void MCLocalPasteboard::Retain(void)
{
	m_references += 1;
}

void MCLocalPasteboard::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
		delete this;
}


bool MCLocalPasteboard::Find(MCTransferType p_type, uint4& r_index)
{
	for(uint4 i = 0; i < m_count; ++i)
		if (m_types[i] == p_type)
		{
			r_index = i;
			return true;
		}
	return false;
}


bool MCLocalPasteboard::Normalize(MCTransferType p_type, MCValueRef p_data, MCTransferType& r_normal_type, MCDataRef &r_normal_data)
{
	switch (p_type)
	{
		case TRANSFER_TYPE_RTF_TEXT:
			r_normal_type = TRANSFER_TYPE_STYLED_TEXT;
			return MCConvertRTFToStyledText((MCDataRef)p_data, r_normal_data);
			
		case TRANSFER_TYPE_HTML_TEXT:
			r_normal_type = TRANSFER_TYPE_STYLED_TEXT;
			return MCConvertHTMLToStyledText((MCDataRef)p_data, r_normal_data);
			
		case TRANSFER_TYPE_STYLED_TEXT_ARRAY:
			r_normal_type = TRANSFER_TYPE_STYLED_TEXT;
			return MCConvertStyledTextArrayToStyledText((MCArrayRef)p_data, r_normal_data);
			
		case TRANSFER_TYPE_TEXT:
			r_normal_type = TRANSFER_TYPE_UNICODE_TEXT;
			return MCStringEncode((MCStringRef)p_data, kMCStringEncodingUTF16, false, r_normal_data);
			
		case TRANSFER_TYPE_FILES:
			r_normal_type = TRANSFER_TYPE_FILES;
			return MCStringEncode((MCStringRef)p_data, kMCStringEncodingUTF16, false, r_normal_data);
			
		default:
			r_normal_type = p_type;
			r_normal_data = MCValueRetain((MCDataRef)p_data);
			return true;
	}
}

bool MCLocalPasteboard::Query(MCTransferType*& r_types, size_t& r_type_count)
{
	r_types = m_types;
	r_type_count = m_count;
	return true;
}

bool MCLocalPasteboard::Fetch(MCTransferType p_type, MCDataRef& r_data)
{
	uint4 t_index;
	if (!Find(p_type, t_index))
		return false;

	r_data = MCValueRetain(m_datas[t_index]);

	return true;
}

bool MCLocalPasteboard::Store(MCTransferType p_type, MCValueRef p_data)
{
	MCTransferType t_normal_type;
    MCAutoDataRef t_normal_data;

	if (!Normalize(p_type, p_data, t_normal_type, &t_normal_data))
		return false;

	// Here the reference count of p_data hasn't changed (it is still owned by
	// the caller).
	// We own t_normal_data (which may be the same as p_data, but if it is its
	// reference count will have already been incremented).

	uint4 t_index;
	if (!Find(t_normal_type, t_index))
	{
		MCTransferType *t_new_types;
		t_new_types = (MCTransferType *)realloc(m_types, sizeof(MCTransferType) * (m_count + 1));
		if (t_new_types == NULL)
			return false;

		MCDataRef *t_new_datas;
		t_new_datas = (MCDataRef *)realloc(m_datas, sizeof(MCDataRef) * (m_count + 1));

		if (t_new_datas == NULL)
		{
			m_types = t_new_types;
			return false;
		}

		m_types = t_new_types;
		m_datas = t_new_datas;
		m_datas[m_count] = nil;
		
		t_index = m_count;

		m_count += 1;
	}
	
	m_types[t_index] = t_normal_type;

	if (m_datas[t_index] != nil)
		MCValueRelease(m_datas[t_index]);

	m_datas[t_index] = MCValueRetain(*t_normal_data);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Abstract Class:
//    MCTransferData
//

MCTransferData::MCTransferData(void)
{
	m_lock_count = 0;
	m_pasteboard = NULL;

	m_open_count = 0;
	m_open_pasteboard = NULL;
}

MCTransferData::~MCTransferData(void)
{
	if (m_pasteboard != NULL)
		m_pasteboard -> Release();

	if (m_open_pasteboard != NULL)
		m_open_pasteboard -> Release();
}

//

bool MCTransferData::HasTypeConversion(MCTransferType p_source, MCTransferType p_target)
{
	bool t_source_is_text;
	t_source_is_text = p_source >= TRANSFER_TYPE_TEXT__FIRST && p_source <= TRANSFER_TYPE_TEXT__LAST;

	bool t_target_is_text;
	t_target_is_text = p_target >= TRANSFER_TYPE_TEXT__FIRST && p_target <= TRANSFER_TYPE_TEXT__LAST;

	if (t_source_is_text && t_target_is_text)
		return true;

	if (t_target_is_text && p_source == TRANSFER_TYPE_FILES)
		return true;

	if (t_source_is_text && p_target == TRANSFER_TYPE_FILES)
		return true;

	return p_source == p_target;
}

//

bool MCTransferData::HasText(void)
{
	return Contains(TRANSFER_TYPE_TEXT, true);
}

bool MCTransferData::HasImage(void)
{
	return Contains(TRANSFER_TYPE_IMAGE, true);
}

bool MCTransferData::HasFiles(void)
{
	return Contains(TRANSFER_TYPE_FILES, true);
}

bool MCTransferData::HasPrivate(void)
{
	return Contains(TRANSFER_TYPE_PRIVATE, true);
}

bool MCTransferData::HasObjects(void)
{
	return Contains(TRANSFER_TYPE_OBJECTS, true);
}

//

MCParagraph *MCTransferData::FetchParagraphs(MCField *p_field)
{
	MCParagraph *t_paragraphs;
	t_paragraphs = NULL;

	if (!Lock())
		return NULL;

	if (Contains(TRANSFER_TYPE_STYLED_TEXT, false))
	{
		MCAutoValueRef t_data;
		Fetch(TRANSFER_TYPE_STYLED_TEXT, &t_data);
		if (*t_data != nil)
		{
			MCObject *t_object;
			t_object = MCObject::unpickle((MCDataRef)*t_data, MCactivefield -> getstack());
			if (t_object != NULL)
			{
				// TODO: Do a proper type check
				t_paragraphs = ((MCStyledText *)t_object) -> grabparagraphs(MCactivefield);
				delete t_object;
			}
		}
	}
	else if (Contains(TRANSFER_TYPE_UNICODE_TEXT, false))
	{
		MCAutoValueRef t_data;
		if (Fetch(TRANSFER_TYPE_UNICODE_TEXT, &t_data))
			t_paragraphs = p_field -> texttoparagraphs(MCDataGetOldString((MCDataRef)*t_data), true);
	}
	else if (Contains(TRANSFER_TYPE_TEXT, true))
	{
		MCAutoValueRef t_string;
		if (Fetch(TRANSFER_TYPE_TEXT, &t_string))
        {
            MCAutoDataRef t_data;
            /* UNCHECKED */ MCStringEncode((MCStringRef)*t_string, kMCStringEncodingUTF16, false, &t_data);
			t_paragraphs = p_field -> texttoparagraphs(MCDataGetOldString(*t_data), true);
        }
	}

	Unlock();

	return t_paragraphs;
}

//

bool MCTransferData::Lock(void)
{
	if (m_lock_count == 0)
	{
		if (m_open_count != 0 && m_open_pasteboard != NULL)
		{
			m_pasteboard = m_open_pasteboard;
			m_pasteboard -> Retain();
		}
		else
			m_pasteboard = Get();

		if (m_pasteboard == NULL)
			return false;
	}
	
	m_lock_count += 1;

	return true;
}

void MCTransferData::Unlock(void)
{
	m_lock_count -= 1;

	if (m_lock_count == 0)
	{
		m_pasteboard -> Release();
		m_pasteboard = NULL;
	}
}

bool MCTransferData::Query(MCTransferType*& r_types, size_t& r_type_count)
{
	if (m_lock_count == 0)
		return false;

	return m_pasteboard -> Query(r_types, r_type_count);
}

bool MCTransferData::Contains(MCTransferType p_type, bool p_with_conversion)
{
	if (!Lock())
		return false;

	MCTransferType *t_types;
	size_t t_type_count;
	if (!m_pasteboard -> Query(t_types, t_type_count))
	{
		Unlock();
		return false;
	}

	bool t_contains;
	t_contains = false;

	for(uint4 i = 0; i < t_type_count && !t_contains; ++i)
	{
		if (p_type == t_types[i])
			t_contains = true;
		else if (p_with_conversion)
			t_contains = HasTypeConversion(t_types[i], p_type);
	}

	Unlock();

	return t_contains;
}

bool MCTransferData::Fetch(MCTransferType p_type, MCValueRef &r_data)
{
	if (!Lock())
		return false;

	MCTransferType *t_types;
	size_t t_type_count;
	if (!m_pasteboard -> Query(t_types, t_type_count))
	{
		Unlock();
		return false;
	}

	MCTransferType t_current_type;
	t_current_type = TRANSFER_TYPE_NULL;
	for(uint4 i = 0; i < t_type_count; ++i)
		if (HasTypeConversion(t_types[i], p_type))
		{
			t_current_type = t_types[i];
			break;
		}

	MCAutoDataRef t_current_data;
	
	if (t_current_type == TRANSFER_TYPE_NULL || !m_pasteboard -> Fetch(t_current_type, &t_current_data))
	{
		Unlock();
		return false;
	}

	Unlock();
    
    // SN-2014-11-13: [[ Bug 13993 ]] The clipboard for files now contains a UTF-16 string
	if (p_type == t_current_type && p_type != TRANSFER_TYPE_FILES)
    {
        r_data = MCValueRetain(*t_current_data);
        return true;
    }
    
    // SN-2014-11-13: [[ Bug 13993 ]] The files may return unicode chars; the data should be UTF-16
    if (p_type == TRANSFER_TYPE_FILES)
    {
        return MCStringDecode(*t_current_data, kMCStringEncodingUTF16, false, (MCStringRef&)r_data);
    }
    
    // AL-2014-06-26: [[ Bug 12540 ]] If text is requested, return a (not necessarily native) string
    if (p_type == TRANSFER_TYPE_TEXT && t_current_type == TRANSFER_TYPE_UNICODE_TEXT)
	{
		return MCStringDecode(*t_current_data, kMCStringEncodingUTF16, false, (MCStringRef&)r_data);
	}

	if (p_type == TRANSFER_TYPE_TEXT && t_current_type == TRANSFER_TYPE_FILES)
	{
		r_data = MCValueRetain(*t_current_data);
		return true;
	}
	
	if (p_type == TRANSFER_TYPE_UNICODE_TEXT && (t_current_type == TRANSFER_TYPE_TEXT || t_current_type == TRANSFER_TYPE_FILES))
	{
		MCAutoStringRef t_text;
		if (!MCStringDecode(*t_current_data, kMCStringEncodingNative, false, &t_text) ||
			!MCStringEncode(*t_text, kMCStringEncodingUTF16, false, (MCDataRef &)r_data))
			return false;
		return true;
	}


	bool t_success;
	t_success = true;

	if (p_type >= TRANSFER_TYPE_TEXT__FIRST && p_type <= TRANSFER_TYPE_TEXT__LAST)
	{
		MCAutoDataRef t_styled_text;
		switch(t_current_type)
		{
			case TRANSFER_TYPE_FILES:
			case TRANSFER_TYPE_TEXT:
				t_success = MCConvertTextToStyledText(*t_current_data, &t_styled_text);
			break;

			case TRANSFER_TYPE_STYLED_TEXT:
				t_styled_text = *t_current_data;
			break;

			case TRANSFER_TYPE_UNICODE_TEXT:
				t_success = MCConvertUnicodeToStyledText(*t_current_data, &t_styled_text);
			break;

			case TRANSFER_TYPE_RTF_TEXT:
				t_success = MCConvertRTFToStyledText(*t_current_data, &t_styled_text);
			break;

			case TRANSFER_TYPE_HTML_TEXT:
				 t_success = MCConvertHTMLToStyledText(*t_current_data, &t_styled_text);
			break;

			default:
			break;
		}

		if (t_success)
		{
			switch(p_type)
			{
			case TRANSFER_TYPE_TEXT:
            {
                // AL-2014-06-26: [[ Bug 12540 ]] If text is requested, return a (not necessarily native) string
                MCAutoDataRef t_data;
				t_success = (MCConvertStyledTextToUnicode(*t_styled_text, &t_data) &&
                             MCStringDecode(*t_data, kMCStringEncodingUTF16, false, (MCStringRef &)r_data));
            }
			break;

			case TRANSFER_TYPE_UNICODE_TEXT:
				t_success = MCConvertStyledTextToUnicode(*t_styled_text, (MCDataRef &)r_data);
			break;

			case TRANSFER_TYPE_RTF_TEXT:
				t_success = MCConvertStyledTextToRTF(*t_styled_text, (MCDataRef &)r_data);
			break;

			case TRANSFER_TYPE_HTML_TEXT:
				t_success = MCConvertStyledTextToHTML(*t_styled_text, (MCDataRef &)r_data);
			break;

			case TRANSFER_TYPE_STYLED_TEXT:
				r_data = MCValueRetain(*t_styled_text);
			break;
			}
		}
		return t_success;
	}
	return false;
}

//

void MCTransferData::Open(void)
{
	m_open_count += 1;
	if (m_open_count == 1)
		m_open_pasteboard = new MCLocalPasteboard;
}

bool MCTransferData::Store(MCTransferType p_type, MCValueRef p_data)
{
	Open();

	if (m_open_pasteboard == NULL)
		return false;

	bool t_success;
	t_success = m_open_pasteboard -> Store(p_type, p_data);

	if (!Close())
		t_success = false;

	return t_success;
}

bool MCTransferData::Close(void)
{
	m_open_count -= 1;
	if (m_open_count == 0)
	{
		if (m_open_pasteboard == NULL)
			return false;

		bool t_success;
		t_success = Set(m_open_pasteboard);

		m_open_pasteboard -> Release();
		m_open_pasteboard = NULL;

		return t_success;
	}

	return true;
}

//

MCTransferType MCTransferData::StringToType(MCStringRef p_string)
{
	if (MCStringIsEqualToCString(p_string, "text", kMCCompareCaseless))
		return TRANSFER_TYPE_TEXT;

	if (MCStringIsEqualToCString(p_string, "unicode", kMCCompareCaseless))
		return TRANSFER_TYPE_UNICODE_TEXT;
    
	if (MCStringIsEqualToCString(p_string, "styledText", kMCCompareCaseless))
        return TRANSFER_TYPE_STYLED_TEXT_ARRAY;
        
	if (MCStringIsEqualToCString(p_string, "styles", kMCCompareCaseless))
		return TRANSFER_TYPE_STYLED_TEXT;

	if (MCStringIsEqualToCString(p_string, "rtf", kMCCompareCaseless))
		return TRANSFER_TYPE_RTF_TEXT;

	if (MCStringIsEqualToCString(p_string, "html", kMCCompareCaseless))
		return TRANSFER_TYPE_HTML_TEXT;

	if (MCStringIsEqualToCString(p_string, "files", kMCCompareCaseless))
		return TRANSFER_TYPE_FILES;

	if (MCStringIsEqualToCString(p_string, "private", kMCCompareCaseless))
		return TRANSFER_TYPE_PRIVATE;

	if (MCStringIsEqualToCString(p_string, "image", kMCCompareCaseless))
		return TRANSFER_TYPE_IMAGE;

	if (MCStringIsEqualToCString(p_string, "objects", kMCCompareCaseless))
		return TRANSFER_TYPE_OBJECTS;

	return TRANSFER_TYPE_NULL;
}

const char *MCTransferData::TypeToString(MCTransferType p_type)
{
	switch(p_type)
	{
	case TRANSFER_TYPE_TEXT:
		return "text";
	case TRANSFER_TYPE_UNICODE_TEXT:
		return "unicode";
	case TRANSFER_TYPE_STYLED_TEXT:
		return "styles";
    case TRANSFER_TYPE_STYLED_TEXT_ARRAY:
        return "styledText";
	case TRANSFER_TYPE_RTF_TEXT:
		return "rtf";
	case TRANSFER_TYPE_HTML_TEXT:
		return "html";
	case TRANSFER_TYPE_IMAGE:
		return "image";
	case TRANSFER_TYPE_FILES:
		return "files";
	case TRANSFER_TYPE_PRIVATE:
		return "private";
	case TRANSFER_TYPE_OBJECTS:
		return "objects";
	}

	return "";
}

///////////////////////////////////////////////////////////////////////////////

MCSelectionData::MCSelectionData(void)
{
	m_local_pasteboard = NULL;
}

MCSelectionData::~MCSelectionData(void)
{
	if (m_local_pasteboard != NULL)
		m_local_pasteboard -> Release();
}

MCPasteboard *MCSelectionData::Get(void)
{
	if (m_local_pasteboard != NULL && MCscreen -> ownsselection())
	{
		m_local_pasteboard -> Retain();
		return m_local_pasteboard;
	}

	return MCscreen -> getselection();
}

bool MCSelectionData::Set(MCPasteboard *p_pasteboard)
{
	// IM-2011-12-05: Make sure m_local_pasteboard is set to NULL after Release() as MCscreen->setselection() could fail
	if (m_local_pasteboard != NULL)
    {
		m_local_pasteboard -> Release();
        m_local_pasteboard = NULL;
    }
	
	if (MCscreen -> setselection(p_pasteboard))
	{
		m_local_pasteboard = p_pasteboard;
		m_local_pasteboard -> Retain();
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

MCClipboardData::MCClipboardData(void)
{
	m_local_pasteboard = NULL;
}

MCClipboardData::~MCClipboardData(void)
{
	if (m_local_pasteboard != NULL)
		m_local_pasteboard -> Release();
}

MCPasteboard *MCClipboardData::Get(void)
{
	if (m_local_pasteboard != NULL && MCscreen -> ownsclipboard())
	{
		m_local_pasteboard -> Retain();
		return m_local_pasteboard;
	}

	return MCscreen -> getclipboard();
}

bool MCClipboardData::Set(MCPasteboard *p_pasteboard)
{
	// IM-2011-12-05: Make sure m_local_pasteboard is set to NULL after Release() as MCscreen->setclipboard() could fail
	if (m_local_pasteboard != NULL)
    {
		m_local_pasteboard -> Release();
        m_local_pasteboard = NULL;
    }
	
	if (MCscreen -> setclipboard(p_pasteboard))
	{
		m_local_pasteboard = p_pasteboard;
		m_local_pasteboard -> Retain();
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

MCDragData::MCDragData(void)
{
	m_target_pasteboard = NULL;
	m_source_pasteboard = NULL;
}

MCDragData::~MCDragData(void)
{
	if (m_target_pasteboard != NULL)
		m_target_pasteboard -> Release();

	if (m_source_pasteboard != NULL)
		m_source_pasteboard -> Release();
}

void MCDragData::SetTarget(MCPasteboard *p_pasteboard)
{
	if (m_target_pasteboard != NULL)
	{
		m_target_pasteboard -> Release();
		m_target_pasteboard = NULL;
	}

	if (p_pasteboard != NULL)
	{
		m_target_pasteboard  = p_pasteboard;
		m_target_pasteboard -> Retain();
	}
}

void MCDragData::ResetTarget(void)
{
	SetTarget(NULL);
}

MCPasteboard *MCDragData::GetSource(void)
{
	return m_source_pasteboard;
}

void MCDragData::ResetSource(void)
{
	if (m_source_pasteboard != NULL)
	{
		m_source_pasteboard -> Release();
		m_source_pasteboard = NULL;
	}
}

MCPasteboard *MCDragData::Get(void)
{
	if (MCdispatcher -> isdragtarget())
	{
		if (m_target_pasteboard != NULL)
			m_target_pasteboard -> Retain();
		return m_target_pasteboard;
	}

	if (m_source_pasteboard != NULL)
		m_source_pasteboard -> Retain();

	return m_source_pasteboard;
}

bool MCDragData::Set(MCPasteboard *p_pasteboard)
{
	if (m_source_pasteboard != NULL)
		m_source_pasteboard -> Release();

	m_source_pasteboard = p_pasteboard;
	m_source_pasteboard -> Retain();

	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool MCFormatStyledTextIsUnicode(MCDataRef p_input)
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool MCConvertTextToStyledText(MCDataRef p_input, MCDataRef& r_output)
{
	MCParagraph *t_paragraphs;
	t_paragraphs = MCtemplatefield -> texttoparagraphs(MCDataGetOldString(p_input), false);

	MCStyledText t_styled_text;
	t_styled_text . setparagraphs(t_paragraphs);

	/* UNCHECKED */MCObject::pickle(&t_styled_text, 0, r_output);

	return true;
}

bool MCConvertUnicodeToStyledText(MCDataRef p_input, MCDataRef& r_output)
{
	MCParagraph *t_paragraphs;
	t_paragraphs = MCtemplatefield -> texttoparagraphs(MCDataGetOldString(p_input), true);

	MCStyledText t_styled_text;
	t_styled_text . setparagraphs(t_paragraphs);

	/* UNCHECKED */ MCObject::pickle(&t_styled_text, 0, r_output);

	return true;
}

bool MCConvertHTMLToStyledText(MCDataRef p_input, MCDataRef& r_output)
{
	MCParagraph *t_paragraphs;
	// MW-2012-03-08: [[ FieldImport ]] Use the new htmlText importer.
	t_paragraphs = MCtemplatefield -> importhtmltext(p_input);

	MCStyledText t_styled_text;
	t_styled_text . setparent(MCtemplatefield -> getparent());
	t_styled_text . setparagraphs(t_paragraphs);

	/* UNCHECKED */ MCObject::pickle(&t_styled_text, 0, r_output);

	return true;
}

bool MCConvertRTFToStyledText(MCDataRef p_input, MCDataRef& r_output)
{
	MCParagraph *t_paragraphs;
    MCAutoStringRef t_input;
    /* UNCHECKED */ MCStringDecode(p_input, kMCStringEncodingNative, false, &t_input);
	t_paragraphs = MCtemplatefield -> rtftoparagraphs(*t_input);

	MCStyledText t_styled_text;
	t_styled_text . setparent(MCdefaultstackptr);
	t_styled_text . setparagraphs(t_paragraphs);

	/* UNCHECKED */ MCObject::pickle(&t_styled_text, 0, r_output);

	return true;
}

bool MCConvertStyledTextToText(MCDataRef p_input, MCDataRef& r_output)
{
	MCObject *t_object;
	t_object = MCObject::unpickle(p_input, MCtemplatefield -> getstack());
	if (t_object == nil)
		return false;

	MCParagraph *t_paragraphs;
	t_paragraphs = ((MCStyledText *)t_object) -> getparagraphs();

	bool t_success;
	t_success = t_paragraphs != nil;
	
	// MW-2012-02-21: [[ FieldExport ]] Use the new plain text export method.
	MCAutoStringRef t_text;
	if (t_success)
		t_success = MCtemplatefield -> exportasplaintext(t_paragraphs, 0, INT32_MAX, &t_text);
	
	if (t_success)
		t_success = MCStringEncode(*t_text, kMCStringEncodingNative, false, r_output);

	delete t_object;

	return t_success;
}


bool MCConvertStyledTextToUnicode(MCDataRef p_input, MCDataRef& r_output)
{
	MCObject *t_object;
	t_object = MCObject::unpickle(p_input, MCtemplatefield -> getstack());
	if (t_object == nil)
		return false;

	MCParagraph *t_paragraphs;
	t_paragraphs = ((MCStyledText *)t_object) -> getparagraphs();

	bool t_success;
	t_success = t_paragraphs != nil;
	
	// MW-2012-02-21: [[ FieldExport ]] Use the new plain text export method.
	MCAutoStringRef t_text;
	if (t_success)
		t_success = MCtemplatefield -> exportasplaintext(t_paragraphs, 0, INT32_MAX, &t_text);
	
	if (t_success)
		t_success = MCStringEncode(*t_text, kMCStringEncodingUTF16, false, r_output);
	
	delete t_object;

	return t_success;
}

bool MCConvertStyledTextToHTML(MCDataRef p_input, MCDataRef& r_output)
{
	MCObject *t_object;
	t_object = MCObject::unpickle(p_input, MCtemplatefield -> getstack());
	if (t_object == nil)
		return false;

	MCParagraph *t_paragraphs;
	t_paragraphs = ((MCStyledText *)t_object) -> getparagraphs();

	bool t_success;
	t_success = t_paragraphs != nil;

	// MW-2012-02-21: [[ FieldExport ]] Use the new plain text export method.
	if (t_success)
		t_success = MCtemplatefield -> exportashtmltext(t_paragraphs, 0, INT32_MAX, false, r_output);
	
	delete t_object;
	
	return t_success;
}

bool MCConvertStyledTextToRTF(MCDataRef p_input, MCDataRef& r_output)
{
	MCObject *t_object;
	t_object = MCObject::unpickle(p_input, MCtemplatefield -> getstack());
	if (t_object == nil)
		return false;

	MCParagraph *t_paragraphs;
	t_paragraphs = ((MCStyledText *)t_object) -> getparagraphs();

	bool t_success;
	t_success = t_paragraphs != nil;

	// MW-2012-02-21: [[ FieldExport ]] Use the new plain text export method.
	MCAutoStringRef t_text;
	if (t_success)
		t_success = MCtemplatefield -> exportasrtftext(t_paragraphs, 0, INT32_MAX, &t_text);
	
	if (t_success)
		t_success = MCStringEncode(*t_text, kMCStringEncodingNative, false, r_output);
    
	delete t_object;
	
	return t_success;
}

// MW-2014-03-12: [[ ClipboardStyledText ]] Convert data stored as a 'styles' pickle to a styledText array.
bool MCConvertStyledTextToStyledTextArray(MCDataRef p_string, MCArrayRef &r_array)
{
	MCObject *t_object;
	t_object = MCObject::unpickle(p_string, MCtemplatefield -> getstack());
	if (t_object != NULL)
	{
        bool t_success;
        t_success = true;
		MCParagraph *t_paragraphs;
        MCAutoArrayRef t_array;
		t_paragraphs = ((MCStyledText *)t_object) -> getparagraphs();
		
		if (t_paragraphs != NULL)
        {
            // SN-2015-01-19: [[ Bug 14378 ]] Actually sets the return value.
			if (MCtemplatefield -> exportasstyledtext(t_paragraphs, 0, INT32_MAX, false, false, &t_array))
                t_success = MCArrayCopy(*t_array, r_array);
            else
                t_success = false;
        }
        else
            t_success = false;
		
		delete t_object;
        return t_success;
	}
    
	return false;
}

// MW-2014-03-12: [[ ClipboardStyledText ]] Convert a styledText array to a 'styles' pickle.
bool MCConvertStyledTextArrayToStyledText(MCArrayRef p_array, MCDataRef &r_output)
{	
	MCParagraph *t_paragraphs;
	t_paragraphs = MCtemplatefield -> styledtexttoparagraphs(p_array);
    
	MCStyledText t_styled_text;
	t_styled_text . setparent(MCdefaultstackptr);
	t_styled_text . setparagraphs(t_paragraphs);
	/* UNCHECKED */ MCObject::pickle(&t_styled_text, 0, r_output);
    
    return true;
}
