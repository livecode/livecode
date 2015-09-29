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

#include "execpt.h"
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
			m_datas[i] -> Release();

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

bool MCLocalPasteboard::Normalize(MCTransferType p_type, MCSharedString *p_data, MCTransferType& r_normal_type, MCSharedString*& r_normal_data)
{
	MCTransferType t_normal_type;
	MCSharedString *t_normal_data;

	if (p_type == TRANSFER_TYPE_RTF_TEXT)
	{
		t_normal_type = TRANSFER_TYPE_STYLED_TEXT;
		t_normal_data = MCConvertRTFToStyledText(p_data);
	}
	else if (p_type == TRANSFER_TYPE_HTML_TEXT)
	{
		t_normal_type = TRANSFER_TYPE_STYLED_TEXT;
		t_normal_data = MCConvertHTMLToStyledText(p_data);
	}
	else
	{
		t_normal_type = p_type;
		t_normal_data = p_data;
		t_normal_data -> Retain();
	}

	if (t_normal_data == NULL)
		return false;

	r_normal_type = t_normal_type;
	r_normal_data = t_normal_data;

	return true;
}

bool MCLocalPasteboard::Query(MCTransferType*& r_types, unsigned int& r_type_count)
{
	r_types = m_types;
	r_type_count = m_count;
	return true;
}

bool MCLocalPasteboard::Fetch(MCTransferType p_type, MCSharedString*& r_data)
{
	uint4 t_index;
	if (!Find(p_type, t_index))
		return false;

	r_data = m_datas[t_index];
	r_data -> Retain();

	return true;
}

bool MCLocalPasteboard::Store(MCTransferType p_type, MCSharedString *p_data)
{
	MCTransferType t_normal_type;
	MCSharedString *t_normal_data;

	if (!Normalize(p_type, p_data, t_normal_type, t_normal_data))
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

		// FG-2013-09-20 [[ Bugfix 11191 ]]
		// Wrong type used in sizeof calculation (was MCTransferType)
		MCSharedString **t_new_datas;
		t_new_datas = (MCSharedString **)realloc(m_datas, sizeof(MCSharedString*) * (m_count + 1));
		if (t_new_datas == NULL)
		{
			m_types = t_new_types;
			return false;
		}

		m_types = t_new_types;
		m_datas = t_new_datas;
		m_datas[m_count] = NULL;
		
		t_index = m_count;

		m_count += 1;
	}
	
	m_types[t_index] = t_normal_type;

	if (m_datas[t_index] != NULL)
		m_datas[t_index] -> Release();

	m_datas[t_index] = t_normal_data;

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
		return false;

	if (Contains(TRANSFER_TYPE_STYLED_TEXT, false))
	{
		MCSharedString *t_data;
		t_data = Fetch(TRANSFER_TYPE_STYLED_TEXT);
		if (t_data != NULL)
		{
			MCObject *t_object;
			t_object = MCObject::unpickle(t_data, MCactivefield -> getstack());
			if (t_object != NULL)
			{
				t_paragraphs = ((MCStyledText *)t_object) -> grabparagraphs(MCactivefield);
				delete t_object;
			}
			t_data -> Release();
		}
	}
	else if (Contains(TRANSFER_TYPE_UNICODE_TEXT, false))
	{
		MCSharedString *t_data;
		t_data = Fetch(TRANSFER_TYPE_UNICODE_TEXT);
		if (t_data != NULL)
		{
			t_paragraphs = p_field -> texttoparagraphs(t_data -> Get(), true);
			t_data -> Release();
		}
	}
	else if (Contains(TRANSFER_TYPE_TEXT, true))
	{
		MCSharedString *t_data;
		t_data = Fetch(TRANSFER_TYPE_TEXT);
		if (t_data != NULL)
		{
			t_paragraphs = p_field -> texttoparagraphs(t_data -> Get(), false);
			t_data -> Release();
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

bool MCTransferData::Query(MCTransferType*& r_types, uint4& r_type_count)
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
	uint4 t_type_count;
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

MCSharedString *MCTransferData::Fetch(MCTransferType p_type)
{
	if (!Lock())
		return false;

	MCTransferType *t_types;
	uint4 t_type_count;
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

	MCSharedString *t_current_data;
	t_current_data = NULL;
	if (t_current_type != TRANSFER_TYPE_NULL)
		if (!m_pasteboard -> Fetch(t_current_type, t_current_data))
			t_current_data = NULL;

	Unlock();
	
	if (t_current_type == TRANSFER_TYPE_NULL || t_current_data == NULL)
		return NULL;

	if (p_type == t_current_type)
		return t_current_data;

	if (p_type == TRANSFER_TYPE_TEXT && t_current_type == TRANSFER_TYPE_FILES)
		return t_current_data;

	if (p_type == TRANSFER_TYPE_TEXT && t_current_type == TRANSFER_TYPE_UNICODE_TEXT)
	{
		MCSharedString *t_target_data;
		t_target_data = MCConvertUnicodeToText(t_current_data);
		t_current_data -> Release();
		return t_target_data;
	}

	if (p_type == TRANSFER_TYPE_UNICODE_TEXT && (t_current_type == TRANSFER_TYPE_TEXT || t_current_type == TRANSFER_TYPE_FILES))
	{
		MCSharedString *t_target_data;
		t_target_data = MCConvertTextToUnicode(t_current_data);
		t_current_data -> Release();
		return t_target_data;
	}

	if (p_type >= TRANSFER_TYPE_TEXT__FIRST && p_type <= TRANSFER_TYPE_TEXT__LAST)
	{
		MCSharedString *t_styled_text;
		switch(t_current_type)
		{
			case TRANSFER_TYPE_FILES:
			case TRANSFER_TYPE_TEXT:
				t_styled_text = MCConvertTextToStyledText(t_current_data);
			break;

			case TRANSFER_TYPE_STYLED_TEXT:
				t_styled_text = t_current_data;
				t_styled_text -> Retain();
			break;

			case TRANSFER_TYPE_UNICODE_TEXT:
				t_styled_text = MCConvertUnicodeToStyledText(t_current_data);
			break;

			case TRANSFER_TYPE_RTF_TEXT:
				t_styled_text = MCConvertRTFToStyledText(t_current_data);
			break;

			case TRANSFER_TYPE_HTML_TEXT:
				t_styled_text = MCConvertHTMLToStyledText(t_current_data);
			break;

			default:
				t_styled_text = NULL;
			break;
		}

		t_current_data -> Release();

		if (t_styled_text == NULL)
			return NULL;

		MCSharedString *t_output_data;
		switch(p_type)
		{
		case TRANSFER_TYPE_TEXT:
			t_output_data = MCConvertStyledTextToText(t_styled_text);
		break;

		case TRANSFER_TYPE_UNICODE_TEXT:
			t_output_data = MCConvertStyledTextToUnicode(t_styled_text);
		break;

		case TRANSFER_TYPE_RTF_TEXT:
			t_output_data = MCConvertStyledTextToRTF(t_styled_text);
		break;

		case TRANSFER_TYPE_HTML_TEXT:
			t_output_data = MCConvertStyledTextToHTML(t_styled_text);
		break;

		case TRANSFER_TYPE_STYLED_TEXT:
			t_output_data = t_styled_text;
			t_output_data -> Retain();
		break;
		}

		t_styled_text -> Release();
		return t_output_data;
	}
	else
		t_current_data -> Release();

	return NULL;
}

//

void MCTransferData::Open(void)
{
	m_open_count += 1;
	if (m_open_count == 1)
		m_open_pasteboard = new MCLocalPasteboard;
}

bool MCTransferData::Store(MCTransferType p_type, MCSharedString* p_data)
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

MCTransferType MCTransferData::StringToType(const MCString& p_string)
{
	if (p_string == "text")
		return TRANSFER_TYPE_TEXT;

	if (p_string == "unicode")
		return TRANSFER_TYPE_UNICODE_TEXT;

	if (p_string == "styledText")
		return TRANSFER_TYPE_STYLED_TEXT_ARRAY;
	
	if (p_string == "styles")
		return TRANSFER_TYPE_STYLED_TEXT;

	if (p_string == "rtf")
		return TRANSFER_TYPE_RTF_TEXT;

	if (p_string == "html")
		return TRANSFER_TYPE_HTML_TEXT;

	if (p_string == "files")
		return TRANSFER_TYPE_FILES;

	if (p_string == "private")
		return TRANSFER_TYPE_PRIVATE;

	if (p_string == "image")
		return TRANSFER_TYPE_IMAGE;

	if (p_string == "objects")
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

bool MCFormatImageIsJPEG(MCSharedString *p_in)
{
    return MCImageDataIsJPEG(p_in -> Get());
}

bool MCFormatImageIsPNG(MCSharedString *p_in)
{
    return MCImageDataIsPNG(p_in -> Get());
}

bool MCFormatImageIsGIF(MCSharedString *p_in)
{
    return MCImageDataIsGIF(p_in -> Get());
}

bool MCFormatStyledTextIsUnicode(MCSharedString *p_in)
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////

MCSharedString *MCConvertTextToStyledText(MCSharedString *p_in)
{
	MCParagraph *t_paragraphs;
	t_paragraphs = MCtemplatefield -> texttoparagraphs(p_in -> Get(), false);
	MCStyledText t_styled_text;
	t_styled_text . setparagraphs(t_paragraphs);
	return MCObject::pickle(&t_styled_text, 0);
}

MCSharedString *MCConvertUnicodeToStyledText(MCSharedString *p_in)
{
	return MCConvertUnicodeToStyledText(p_in -> Get());
}

MCSharedString *MCConvertUnicodeToStyledText(const MCString& p_in)
{
	MCParagraph *t_paragraphs;
	t_paragraphs = MCtemplatefield -> texttoparagraphs(p_in, true);
	MCStyledText t_styled_text;
	t_styled_text . setparagraphs(t_paragraphs);
	return MCObject::pickle(&t_styled_text, 0);
}

MCSharedString *MCConvertHTMLToStyledText(MCSharedString *p_in)
{
	MCParagraph *t_paragraphs;
	// MW-2012-03-08: [[ FieldImport ]] Use the new htmlText importer.
	t_paragraphs = MCtemplatefield -> importhtmltext(p_in -> Get());
	MCStyledText t_styled_text;
	t_styled_text . setparent(MCtemplatefield -> getparent());
	t_styled_text . setparagraphs(t_paragraphs);
	return MCObject::pickle(&t_styled_text, 0);
}

MCSharedString *MCConvertRTFToStyledText(MCSharedString *p_in)
{
	return MCConvertRTFToStyledText(p_in -> Get());
}

MCSharedString *MCConvertRTFToStyledText(const MCString& p_in)
{
	MCParagraph *t_paragraphs;
	t_paragraphs = MCtemplatefield -> rtftoparagraphs(p_in);
	MCStyledText t_styled_text;
	t_styled_text . setparent(MCdefaultstackptr);
	t_styled_text . setparagraphs(t_paragraphs);
	return MCObject::pickle(&t_styled_text, 0);
}

MCSharedString *MCConvertStyledTextToText(MCSharedString *p_in)
{
	MCObject *t_object;
	t_object = MCObject::unpickle(p_in, MCtemplatefield -> getstack());
	if (t_object != NULL)
	{
		MCParagraph *t_paragraphs;
		t_paragraphs = ((MCStyledText *)t_object) -> getparagraphs();

		MCExecPoint ep(NULL, NULL, NULL);

		// MW-2012-02-21: [[ FieldExport ]] Use the new plain text export method.
		if (t_paragraphs != NULL)
			MCtemplatefield -> exportasplaintext(ep, t_paragraphs, 0, INT32_MAX, false);

		delete t_object;

		return MCSharedString::Create(ep . getsvalue());
	}
	return NULL;
}

MCSharedString *MCConvertStyledTextToUnicode(MCSharedString *p_in)
{
	MCObject *t_object;
	t_object = MCObject::unpickle(p_in, MCtemplatefield -> getstack());
	if (t_object != NULL)
	{
		MCParagraph *t_paragraphs;
		t_paragraphs = ((MCStyledText *)t_object) -> getparagraphs();

		MCExecPoint ep(NULL, NULL, NULL);

		// MW-2012-02-21: [[ FieldExport ]] Use the new plain text export method.
		if (t_paragraphs != NULL)
			MCtemplatefield -> exportasplaintext(ep, t_paragraphs, 0, INT32_MAX, true);

		delete t_object;

		return MCSharedString::Create(ep . getsvalue());
	}
	return NULL;
}

MCSharedString *MCConvertStyledTextToHTML(MCSharedString *p_in)
{
	MCObject *t_object;
	t_object = MCObject::unpickle(p_in, MCtemplatefield -> getstack());
	if (t_object != NULL)
	{
		MCParagraph *t_paragraphs;
		t_paragraphs = ((MCStyledText *)t_object) -> getparagraphs();

		MCExecPoint ep(NULL, NULL, NULL);
		
		// OK-2009-03-13: [[Bug 7742]] - Not reproduced but we think this is the cause of the crash.
		// MW-2012-03-05: [[ FieldExport ]] Use the new html text export method.
		if (t_paragraphs != NULL)
			MCtemplatefield -> exportashtmltext(ep, t_paragraphs, 0, INT32_MAX, false);

		delete t_object;

		return MCSharedString::Create(ep . getsvalue());
	}
	return NULL;
}

MCSharedString *MCConvertStyledTextToRTF(MCSharedString *p_in)
{
	MCObject *t_object;
	t_object = MCObject::unpickle(p_in, MCtemplatefield -> getstack());
	if (t_object != NULL)
	{
		MCParagraph *t_paragraphs;
		t_paragraphs = ((MCStyledText *)t_object) -> getparagraphs();

		MCExecPoint ep(NULL, NULL, NULL);
		// OK-2009-03-13: [[Bug 7742]] - Not reproduced but we think this is the cause of the crash.
		// MW-2012-02-29: [[ FieldExport ]] Use the new field export rtf method.
		if (t_paragraphs != NULL)
			MCtemplatefield -> exportasrtftext(ep, t_paragraphs, 0, INT32_MAX);

		delete t_object;

		return MCSharedString::Create(ep . getsvalue());
	}
	return NULL;
}

// MW-2014-03-12: [[ ClipboardStyledText ]] Convert data stored as a 'styles' pickle to a styledText array.
MCVariableValue *MCConvertStyledTextToStyledTextArray(MCSharedString *p_string)
{
	MCObject *t_object;
	t_object = MCObject::unpickle(p_string, MCtemplatefield -> getstack());
	if (t_object != NULL)
	{
		MCParagraph *t_paragraphs;
		t_paragraphs = ((MCStyledText *)t_object) -> getparagraphs();
		
		MCExecPoint ep(NULL, NULL, NULL);
		if (t_paragraphs != NULL)
			MCtemplatefield -> exportasstyledtext(ep, t_paragraphs, 0, INT32_MAX, false, false);
		
		delete t_object;
		
		ep . grabarray();
		
		MCVariableValue *t_array;
		Boolean t_delete_array;
		ep . takearray(t_array, t_delete_array);
		
		return t_array;
	}
	return NULL;
}

// MW-2014-03-12: [[ ClipboardStyledText ]] Convert a styledText array to a 'styles' pickle.
MCSharedString *MCConvertStyledTextArrayToStyledText(MCVariableValue *p_array)
{
	MCExecPoint ep(NULL, NULL, NULL);
	ep . setarray(p_array, False);
	
	MCParagraph *t_paragraphs;
	t_paragraphs = MCtemplatefield -> styledtexttoparagraphs(ep);
	MCStyledText t_styled_text;
	t_styled_text . setparent(MCdefaultstackptr);
	t_styled_text . setparagraphs(t_paragraphs);
	return MCObject::pickle(&t_styled_text, 0);
}

MCSharedString *MCConvertTextToUnicode(MCSharedString *p_string)
{
	MCExecPoint ep(NULL, NULL, NULL);
	ep . setsvalue(p_string -> Get());
	ep . nativetoutf16();
	return MCSharedString::Create(ep . getsvalue());
}

MCSharedString *MCConvertUnicodeToText(MCSharedString *p_string)
{
	MCExecPoint ep(NULL, NULL, NULL);
	ep . setsvalue(p_string -> Get());
	ep . utf16tonative();
	return MCSharedString::Create(ep . getsvalue());
}
