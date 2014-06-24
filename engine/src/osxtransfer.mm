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

#include "osxprefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "field.h"
#include "paragraf.h"
#include "image.h"
#include "dispatch.h"
#include "util.h"
#include "execpt.h"
#include "globals.h"
#include "mctheme.h"
#include "context.h"
#include "transfer.h"
#include "styledtext.h"
#include "stack.h"
#include "mcio.h"

#include "osxtransfer.h"

#import <Foundation/Foundation.h>
#import <AppKit/NSAttributedString.h>

///////////////////////////////////////////////////////////////////////////////

#define flavorTypeRichText 'RTF '
#define flavorTypePNG 'PNGf'
#define flavorTypeJPEG 'JPEG'
#define flavorTypeGIF 'GIFF'
#define flavorTypeTIFF 'TIFF'
#define flavorTypeRevolutionText 'RSTT'
#define flavorTypeRevolutionObjects 'RSTK'
#define flavorTypeHtml 'HTML'

///////////////////////////////////////////////////////////////////////////////

MCMacOSXTransferData::MCMacOSXTransferData(void)
{
	m_entries = NULL;
	m_entry_count = 0;
}

MCMacOSXTransferData::~MCMacOSXTransferData(void)
{
	for(uint4 i = 0; i < m_entry_count; ++i)
		if (m_entries[i] . data != NULL)
			m_entries[i] . data -> Release();
}

bool MCMacOSXTransferData::Publish(ScrapFlavorType p_type, MCSharedString* p_data, MCMacOSXConversionCallback p_converter)
{
	bool t_success;
	t_success = true;

	Entry *t_new_entries;
	t_new_entries = NULL;
	if (t_success)
	{
		t_new_entries = (Entry *)realloc(m_entries, sizeof(Entry) * (m_entry_count + 1));
		if (t_new_entries == NULL)
			t_success = false;
	}

	if (t_success)
	{
		t_new_entries[m_entry_count] . type = p_type;

		p_data -> Retain();
		t_new_entries[m_entry_count] . data = p_data;

		t_new_entries[m_entry_count] . converter = p_converter;

		m_entries = t_new_entries;
		m_entry_count += 1;
	}

	return t_success;
}

bool MCMacOSXTransferData::Publish(MCTransferType p_type, MCSharedString *p_data)
{
	bool t_success;
	t_success = true;

	switch(p_type)
	{
	case TRANSFER_TYPE_TEXT:
		if (t_success)
			t_success = Publish(kScrapFlavorTypeText, p_data, MCConvertTextToMacPlain);
	break;

	case TRANSFER_TYPE_UNICODE_TEXT:
		if (t_success)
			t_success = Publish(kScrapFlavorTypeUnicode, p_data, MCConvertUnicodeToMacUnicode);
	break;

	case TRANSFER_TYPE_STYLED_TEXT:
		// MW-2011-01-17: [[ Bug 9285 ]] Start using RTF as primary format on Mac...
		if (t_success)
			t_success = Publish(flavorTypeRichText, p_data, MCConvertStyledTextToRTF);
			
		if (t_success)
		{
			t_success = Publish(kScrapFlavorTypeUnicodeStyle, p_data, MCConvertStyledTextToMacUnicodeStyled);
			if (t_success)
				t_success = Publish(kScrapFlavorTypeUnicode, p_data, MCConvertStyledTextToMacUnicode);
		}
	break;

	case TRANSFER_TYPE_IMAGE:
		if (t_success && MCFormatImageIsPNG(p_data))
			t_success = Publish(flavorTypePNG, p_data, NULL);
		if (t_success && MCFormatImageIsGIF(p_data))
			t_success = Publish(flavorTypeGIF, p_data, NULL);
		if (t_success && MCFormatImageIsJPEG(p_data))
			t_success = Publish(flavorTypeJPEG, p_data, NULL);
		if (t_success)
			t_success = Publish(kScrapFlavorTypePicture, p_data, MCConvertImageToMacPicture);
	break;

	case TRANSFER_TYPE_FILES:
		if (t_success)
			t_success = Publish(flavorTypeHFS, p_data, MCConvertFilesToMacHFS);
	break;

	case TRANSFER_TYPE_OBJECTS:
		if (t_success)
			t_success = Publish(flavorTypeRevolutionObjects, p_data, NULL);
	break;

	case TRANSFER_TYPE_PRIVATE:
	break;
	}

	return t_success;
}

bool MCMacOSXTransferData::Publish(MCPasteboard *p_pasteboard)
{
	bool t_success;
	t_success = true;

	MCTransferType *t_types;
	uint4 t_type_count;
	if (t_success)
		t_success = p_pasteboard -> Query(t_types, t_type_count);

	if (t_success)
		for(uint4 i = 0; i < t_type_count && t_success; ++i)
		{
			MCSharedString *t_data;
			t_success = p_pasteboard -> Fetch(t_types[i], t_data);
			if (t_success)
				t_success = Publish(t_types[i], t_data);
			if (t_data != NULL)
				t_data -> Release();
		}

	return t_success;
}

MCSharedString *MCMacOSXTransferData::Subscribe(ScrapFlavorType p_type)
{
	for(uint4 i = 0; i < m_entry_count; ++i)
		if (m_entries[i] . type == p_type)
		{
			if (m_entries[i] . converter != NULL)
			{
				MCSharedString *t_converted_data;
				t_converted_data =  m_entries[i] . converter(m_entries[i] . data);
				
				m_entries[i] . data -> Release();
				m_entries[i] . data = t_converted_data;
				m_entries[i] . converter = NULL;
			}

			if (m_entries[i] . data != NULL)
				m_entries[i] . data -> Retain();

			return m_entries[i] . data;
		}

	return NULL;
}

bool MCMacOSXTransferData::ForEachFlavor(bool (*p_callback)(MCMacOSXTransferData *p_data, ScrapFlavorType p_type, void *p_context), void *p_context)
{
	bool t_success;
	t_success = true;
	
	for(uint4 i = 0; i < m_entry_count && t_success; ++i)
		t_success = p_callback(this, m_entries[i] . type, p_context);

	return t_success;
}

///////////////////////////////////////////////////////////////////////////////

MCMacOSXPasteboard::MCMacOSXPasteboard(void)
{
	m_references = 1;
	m_entries = NULL;
	m_entry_count = 0;
	m_types = NULL;
	m_valid = false;
}

MCMacOSXPasteboard::~MCMacOSXPasteboard(void)
{
	for(uint4 i = 0; i < m_entry_count; ++i)
		if (m_entries[i] . data != NULL)
			m_entries[i] . data -> Release();

	delete m_entries;
	delete m_types;
}

void MCMacOSXPasteboard::Retain(void)
{
	m_references += 1;
}

void MCMacOSXPasteboard::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
		delete this;
}

bool MCMacOSXPasteboard::Query(MCTransferType*& r_types, unsigned int& r_type_count)
{
	if (!m_valid)
		return false;

	if (m_types == NULL)
	{
		m_types = new MCTransferType[m_entry_count];
		for(uint4 i = 0; i < m_entry_count; ++i)
			m_types[i] = m_entries[i] . type;
	}

	if (m_types != NULL)
	{
		r_types = m_types;
		r_type_count = m_entry_count;
		return true;
	}

	return false;
}

bool MCMacOSXPasteboard::Fetch(MCTransferType p_type, MCSharedString*& r_data)
{
	if (!m_valid)
		return false;

	unsigned int t_entry;
	for(t_entry = 0; t_entry < m_entry_count; ++t_entry)
		if (m_entries[t_entry] . type == p_type)
			break;

	if (t_entry == m_entry_count)
		return false;

	if (m_entries[t_entry] . data != NULL)
	{
		m_entries[t_entry] . data -> Retain();
		r_data = m_entries[t_entry] . data;
		return true;
	}

	MCSharedString *t_in_data;
	t_in_data = NULL;
	if (!FetchFlavor(m_entries[t_entry] . flavor, t_in_data))
		return false;

	MCSharedString *t_out_data;
	t_out_data = NULL;

	switch(m_entries[t_entry] . flavor)
	{
	case kScrapFlavorTypeText:
	{
		MCExecPoint ep;
		ep . setsvalue(t_in_data -> Get());
		ep . texttobinary();
		t_out_data = MCSharedString::Create(ep . getsvalue());
	}
	break;

	case kScrapFlavorTypeUnicode:
	{
		MCExecPoint ep;
		ep . setsvalue(t_in_data -> Get());
		ep . utf16toutf8();
		ep . texttobinary();
		ep . utf8toutf16();
		t_out_data = MCSharedString::Create(ep . getsvalue());
	}
	break;

	case kScrapFlavorTypeTextStyle:
	{
		MCSharedString *t_text_data;
		if (FetchFlavor(kScrapFlavorTypeText, t_text_data))
		{	
			t_out_data = MCConvertMacStyledToStyledText(t_text_data, t_in_data);
			t_text_data -> Release();
		}
	}
	break;

	case kScrapFlavorTypeUnicodeStyle:
	{
		bool t_is_external;
		t_is_external = false;
		
		// MW-2010-01-08: [[ Bug 8327 ]] Make sure we fetch the correct unicode text!
		MCSharedString *t_text_data;
		t_text_data = nil;
		if (FetchFlavor(kScrapFlavorTypeUTF16External, t_text_data))
			t_is_external = true;
		if (t_text_data != nil ||
			FetchFlavor(kScrapFlavorTypeUnicode, t_text_data))
		{
			t_out_data = MCConvertMacUnicodeStyledToStyledText(t_text_data, t_in_data, t_is_external);
			t_text_data -> Release();
		}
	}
	break;

	// MW-2011-01-17: Add support for fetching RTF encoded text from clipboard.
	case flavorTypeRichText:
	{
		MCSharedString *t_text_data;
		if (FetchFlavor(flavorTypeRichText, t_text_data))
		{
			t_out_data = MCConvertRTFToStyledText(t_text_data);
			t_text_data -> Release();
		}
	}
	break;
	
	// MW-2012-11-19: [[ Bug 10542 ]] Add support for processing HTML encoded text from clipboard.
	case flavorTypeHtml:
	{
		MCSharedString *t_text_data;
		if (FetchFlavor(flavorTypeHtml, t_text_data))
		{
            // SN-2013-07-26: [[ Bug 10893 ]] Convert HTML to RTF using Apple's internal class
			t_out_data = MCConvertMacHTMLToStyledText(t_text_data);
			t_text_data -> Release();
		}
	}
	break;
							
	case kScrapFlavorTypePicture:
		t_out_data = MCConvertMacPictureToImage(t_in_data);
	break;

	case flavorTypeTIFF:
		// MW-2010-11-17: [[ Bug 9183 ]] Check the data is actually TIFF, it is actually a PNG then
		//   do nothing
		if (t_in_data -> GetLength() >= 4 && memcmp(t_in_data -> GetBuffer(), "\211PNG", 4) == 0)
		{
			t_in_data -> Retain();
			t_out_data = t_in_data;
		}
		else
			t_out_data = MCConvertMacTIFFToImage(t_in_data);
	break;

	case flavorTypeHFS:
		t_out_data = MCConvertMacHFSToFiles(t_in_data);
	break;

	case flavorTypeRevolutionText:
	case flavorTypeRevolutionObjects:
	case flavorTypePNG:
	case flavorTypeGIF:
	case flavorTypeJPEG:
		t_in_data -> Retain();
		t_out_data = t_in_data;
	break;
	}

	t_in_data -> Release();

	if (t_out_data == NULL)
		return false;

	m_entries[t_entry] . data = t_out_data;
	t_out_data -> Retain();

	r_data = t_out_data;

	return true;
}

void MCMacOSXPasteboard::Resolve(void)
{
	ScrapFlavorType *t_types;
	uint4 t_type_count;
	t_types = NULL;
	t_type_count = 0;
	if (!QueryFlavors(t_types, t_type_count))
		return;

	unsigned int t_objects_priority;
	t_objects_priority = 0xFFFFFFFFU;
	
	unsigned int t_image_priority;
	t_image_priority = 0xFFFFFFFFU;

	unsigned int t_text_priority;
	t_text_priority = 0xFFFFFFFFU;

	unsigned int t_files_priority;
	t_files_priority = 0xFFFFFFFFU;

	ScrapFlavorType t_objects_format;
	t_objects_format = 0;

	ScrapFlavorType t_image_format;
	t_image_format = 0;

	ScrapFlavorType t_text_format;
	t_text_format = 0;

	for(uint4 i = 0; i < t_type_count; ++i)
	{
		ScrapFlavorType t_type;
		t_type = t_types[i];
		
		switch(t_type)
		{
		case kScrapFlavorTypePicture:
			t_image_priority = MCU_min(i, t_image_priority);
			if (t_image_format == 0)
				t_image_format = kScrapFlavorTypePicture;
		break;

		case flavorTypePNG:
			t_image_priority = MCU_min(i, t_image_priority);
			t_image_format = flavorTypePNG;
		break;

		case flavorTypeGIF:
			t_image_priority = MCU_min(i, t_image_priority);
			if (t_image_format == 0)
				t_image_format = flavorTypeGIF;
		break;

		case flavorTypeJPEG:
			t_image_priority = MCU_min(i, t_image_priority);
			if (t_image_format == 0)
				t_image_format = flavorTypeJPEG;
		break;
		
		case flavorTypeTIFF:
			t_image_priority = MCU_min(i, t_image_priority);
			if (t_image_format == 0)
				t_image_format = flavorTypeTIFF;
		break;

		case kScrapFlavorTypeText:
			t_text_priority = MCU_min(i, t_text_priority);
			if (t_text_format == 0)
				t_text_format = kScrapFlavorTypeText;
		break;

		case kScrapFlavorTypeUTF16External:
		case kScrapFlavorTypeUnicode:
			t_text_priority = MCU_min(i, t_text_priority);
			if (t_text_format == 0 || t_text_format == kScrapFlavorTypeText)
				t_text_format = kScrapFlavorTypeUnicode;
		break;

		case kScrapFlavorTypeTextStyle:
			t_text_priority = MCU_min(i, t_text_priority);
			if (t_text_format == 0 || t_text_format == kScrapFlavorTypeText || t_text_format == kScrapFlavorTypeUnicode)
				t_text_format = kScrapFlavorTypeTextStyle;
		break;

		case kScrapFlavorTypeUnicodeStyle:
			t_text_priority = MCU_min(i, t_text_priority);
			if (t_text_format == 0 || t_text_format == kScrapFlavorTypeText || t_text_format == kScrapFlavorTypeUnicode || t_text_format == kScrapFlavorTypeTextStyle)
				t_text_format = kScrapFlavorTypeUnicodeStyle;
		break;
		
		// MW-2011-01-17: Add support for fetching RTF encoded text from clipboard.
		case flavorTypeRichText:
			t_text_priority = MCU_min(i, t_text_priority);
			if (t_text_format == 0 || t_text_format == kScrapFlavorTypeText || t_text_format == kScrapFlavorTypeUnicode || t_text_format == kScrapFlavorTypeTextStyle || t_text_format == kScrapFlavorTypeUnicodeStyle)
				t_text_format = flavorTypeRichText;
		break;
		
		// MW-2012-11-19: [[ Bug 10542 ]] Copying from firefox loses formatting.
		case flavorTypeHtml:
			t_text_priority = MCU_min(i, t_text_priority);
			if (t_text_format == 0 || t_text_format == kScrapFlavorTypeText || t_text_format == kScrapFlavorTypeUnicode || t_text_format == kScrapFlavorTypeTextStyle || t_text_format == kScrapFlavorTypeUnicodeStyle)
				t_text_format = flavorTypeHtml;
		break;

		case flavorTypeRevolutionText:
			t_text_priority = MCU_min(i, t_text_priority);
			t_text_format = flavorTypeRevolutionText;
		break;

		case flavorTypeRevolutionObjects:
			t_objects_priority = MCU_min(i, t_objects_priority);
			t_objects_format = flavorTypeRevolutionObjects;
		break;

		case flavorTypeHFS:
			t_files_priority = MCU_min(i, t_files_priority);
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
		AddEntry(TRANSFER_TYPE_FILES, flavorTypeHFS);
	else if (t_text_priority != 0xFFFFFFFFU)
	{
		if (t_text_format == kScrapFlavorTypeText)
			AddEntry(TRANSFER_TYPE_TEXT, t_text_format);
		if (t_text_format == kScrapFlavorTypeUnicode)
			AddEntry(TRANSFER_TYPE_UNICODE_TEXT, t_text_format);
		else
			AddEntry(TRANSFER_TYPE_STYLED_TEXT, t_text_format);
	}
	else if (t_image_priority != 0xFFFFFFFFU)
		AddEntry(TRANSFER_TYPE_IMAGE, t_image_format);
	
	m_valid = true;

	delete t_types;
}

bool MCMacOSXPasteboard::AddEntry(MCTransferType p_type, ScrapFlavorType p_flavor)
{
	Entry *t_new_entries;
	t_new_entries = (Entry *)realloc(m_entries, sizeof(Entry) * (m_entry_count + 1));
	if (t_new_entries == NULL)
		return false;

	m_entries = t_new_entries;
	m_entries[m_entry_count] . type = p_type;
	m_entries[m_entry_count] . flavor = p_flavor;
	m_entries[m_entry_count] . data = NULL;
	m_entry_count += 1;

	return true;
}

///////////////////////////////////////////////////////////////////////////////

MCSharedString *MCConvertTextToMacPlain(MCSharedString *p_data)
{
	MCExecPoint ep(NULL, NULL, NULL);
	ep . setsvalue(p_data -> Get());
	ep . binarytotext();
	return MCSharedString::Create(ep . getsvalue());
}

MCSharedString *MCConvertUnicodeToMacUnicode(MCSharedString *p_data)
{
	MCExecPoint ep(NULL, NULL, NULL);
	ep . setsvalue(p_data -> Get());
	ep . utf16toutf8();
	ep . binarytotext();
	ep . utf8toutf16();
	return MCSharedString::Create(ep . getsvalue());
}

MCSharedString *MCConvertStyledTextToMacUnicode(MCSharedString *p_data)
{
	MCSharedString *t_result;
	t_result = NULL;
	
	MCSharedString *t_unicode;
	t_unicode = MCConvertStyledTextToUnicode(p_data);
	if (t_unicode != NULL)
	{
		t_result = MCConvertUnicodeToMacUnicode(t_unicode);
		t_unicode -> Release();
	}
	
	return t_result;
}

MCSharedString *MCConvertStyledTextToMacUnicodeStyled(MCSharedString *p_in)
{
	MCObject *t_object;
	t_object = MCObject::unpickle(p_in, MCtemplatefield -> getstack());
	if (t_object != NULL)
	{
		MCParagraph *t_paragraphs;
		t_paragraphs = ((MCStyledText *)t_object) -> getparagraphs();

		MCExecPoint ep(NULL, NULL, NULL);
		MCtemplatefield -> getparagraphmacunicodestyles(ep, t_paragraphs, t_paragraphs -> prev());

		delete t_object;

		return MCSharedString::Create(ep . getsvalue());
	}
	return NULL;
}

MCSharedString *MCConvertStyledTextToMacPlain(MCSharedString *p_data)
{
	MCSharedString *t_result;
	t_result = NULL;
	
	MCSharedString *t_text;
	t_text = MCConvertStyledTextToText(p_data);
	if (t_text != NULL)
	{
		t_result = MCConvertTextToMacPlain(t_text);
		t_text -> Release();
	}
	
	return t_result;
}

MCSharedString *MCConvertImageToMacPicture(MCSharedString* p_data)
{
	bool t_success = true;
	
	MCMacSysPictHandle t_handle = nil;
	MCSharedString *t_result = nil;
	MCImageFrame *t_frames = nil;
	uindex_t t_frame_count = 0;
	
	t_success = MCImageDecode((const uint8_t*)p_data->GetBuffer(), p_data->GetLength(), t_frames, t_frame_count) &&
		MCImageBitmapToPICT(t_frames[0].image, t_handle);
	
	MCImageFreeFrames(t_frames, t_frame_count);
	
	if (t_success)
	{
		HLock((Handle)t_handle);
		t_success = nil != (t_result = MCSharedString::Create(*(Handle)t_handle, GetHandleSize((Handle)t_handle)));
		HUnlock((Handle)t_handle);
		DisposeHandle((Handle)t_handle);
	}
	
	return t_success ? t_result : nil;
}

MCSharedString *MCConvertFilesToMacHFS(MCSharedString *p_data)
{
	uint4 t_length;
	t_length = p_data -> GetLength();
	
	const char *t_buffer;
	t_buffer = (const char *)p_data -> GetBuffer();
	
	HFSFlavor *t_items;
	t_items = NULL;
	
	uint32_t t_count;
	t_count = 0;
	
	do
	{
		const char *t_start;
		t_start = t_buffer;
		if (!MCU_strchr(t_buffer, t_length, '\n', False))
		{
			t_buffer += t_length;
			t_length = 0;
		}
		
		FSRef t_ref;
		FSSpec t_spec;
		FSCatalogInfo t_info;
		
		OSErr t_err;
		t_err = MCS_pathtoref(MCString(t_start, t_buffer - t_start), &t_ref);
		if (t_err == noErr)
			t_err = FSGetCatalogInfo(&t_ref, kFSCatInfoFinderInfo | kFSCatInfoNodeFlags, &t_info, NULL, &t_spec, NULL);
		
		if (t_err == noErr)
		{
			HFSFlavor *t_new_items;
			t_new_items = (HFSFlavor *)realloc(t_items, sizeof(HFSFlavor) * (t_count + 1));
			if (t_new_items != NULL)
			{
				if ((t_info . nodeFlags & kFSNodeIsDirectoryBit) == 0)
				{
					t_new_items[t_count] . fileType = ((FileInfo *)&t_info . finderInfo) -> fileType;
					t_new_items[t_count] . fileCreator = ((FileInfo *)&t_info . finderInfo) -> fileCreator;
					t_new_items[t_count] . fdFlags = ((FileInfo *)&t_info . finderInfo) -> finderFlags;
				}
				else
				{
					t_new_items[t_count] . fileType = t_spec . parID == fsRtParID ? 'disk' : 'fold';
					t_new_items[t_count] . fileCreator = 'MACS';
					t_new_items[t_count] . fdFlags = ((FolderInfo *)&t_info . finderInfo) -> finderFlags;
				}
				t_new_items[t_count] . fileSpec = t_spec;
				
				t_items = t_new_items;
				t_count += 1;
			}
		}
		
		if (t_length > 0)
		{
			t_buffer += 1;
			t_length -= 1;
		}
	}
	while(t_length > 0);

	if (t_items == NULL)
		return NULL;

	return MCSharedString::CreateNoCopy(t_items, sizeof(HFSFlavor) * t_count);
}

//

MCSharedString *MCConvertMacStyledToStyledText(MCSharedString *p_text_data, MCSharedString *p_style_data)
{
	MCParagraph *t_paragraphs;
	t_paragraphs = MCtemplatefield -> macstyletexttoparagraphs(p_text_data -> Get(), p_style_data -> Get(), False);
	
	MCStyledText t_styled_text;
	t_styled_text . setparent(MCtemplatefield -> getparent());
	t_styled_text . setparagraphs(t_paragraphs);
	
	return MCObject::pickle(&t_styled_text, 0);
}

MCSharedString *MCConvertMacUnicodeStyledToStyledText(MCSharedString *p_text_data, MCSharedString *p_style_data, bool t_is_external)
{
	MCParagraph *t_paragraphs;
	
	MCString t_text;
	t_text = p_text_data -> Get();
	
	// MW-2010-01-08: [[ Bug 8327 ]] If the text is 'external' representation, skip the BOM.
	if (t_is_external && t_text . getlength() >= 2 &&
		(*(uint2 *)t_text . getstring() == 0xfffe || 
		*(uint2 *)t_text . getstring() == 0xfeff))
		t_text . set(t_text . getstring() + 2, t_text . getlength() - 1);
	
	// MW-2009-12-01: If the unicode styled text has an empty style data, then make
	//   sure we just convert it as plain unicode text.
	if (p_style_data -> Get() . getlength() != 0)
		t_paragraphs = MCtemplatefield -> macunicodestyletexttoparagraphs(t_text, p_style_data -> Get());
	else
	{
		MCExecPoint ep;
		ep . setsvalue(t_text);
		ep . utf16toutf8();
		ep . texttobinary();
		ep . utf8toutf16();
		t_paragraphs = MCtemplatefield -> texttoparagraphs(ep . getsvalue(), true);
	}
	
	MCStyledText t_styled_text;
	t_styled_text . setparent(MCtemplatefield -> getparent());
	t_styled_text . setparagraphs(t_paragraphs);
	
	return MCObject::pickle(&t_styled_text, 0);
}

//

static inline uint32_t packed_scale_bounded(uint32_t x, uint8_t a)
{
	uint32_t u, v;

	u = ((x & 0xff00ff) * a) + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = (((x >> 8) & 0xff00ff) * a) + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u + v;
}

MCSharedString *MCConvertMacPictureToImage(MCSharedString *p_data)
{
	CGrafPtr t_old_port;
	GDHandle t_old_gdevice;
	GetGWorld(&t_old_port, &t_old_gdevice);

	bool t_success;
	t_success = true;

	// First copy the data into a system memory handle
	PicHandle t_pict;
	t_pict = NULL;
	if (t_success)
	{
		t_pict = (PicHandle)NewHandle(p_data -> GetLength());
		if (t_pict == NULL)
			t_success = false;
	}
	
	if (t_success)
	{
		HLock((Handle)t_pict);
		memcpy(*t_pict, p_data -> GetBuffer(), p_data -> GetLength());
		HUnlock((Handle)t_pict);
	}
		
	// Fetch information about the pict and use it to calculate the optimal width
	// and height.
	PictInfo t_pict_info;
	if (t_success)
	{
		if (GetPictInfo(t_pict, &t_pict_info, 0, 0, 0, 0) != noErr)
			t_success = false;
	}
	
	// Calculate the width and height assuming a target resolution of 72 dpi
	Rect t_rect;
	if (t_success)
	{
		uint4 t_width, t_height;
		t_width = (double)t_pict_info . sourceRect . right * t_pict_info . hRes / (72 << 16);
		t_height = (double)t_pict_info . sourceRect . bottom * t_pict_info . vRes / (72 << 16);
		SetRect(&t_rect, 0, 0, t_width, t_height);
	}
	
	// Create black and white worlds to do the alpha divination...
	GWorldPtr t_black_world;
	t_black_world = NULL;
	if (t_success)
	{
		if (NewGWorld(&t_black_world, 32, &t_rect, NULL, NULL, MCmajorosversion >= 0x1040 ? kNativeEndianPixMap : 0) != noErr)
			t_success = false;
	}
	
	GWorldPtr t_white_world;
	t_white_world = NULL;
	if (t_success)
	{
		if (NewGWorld(&t_white_world, 32, &t_rect, NULL, NULL, MCmajorosversion >= 0x1040 ? kNativeEndianPixMap : 0) != noErr)
			t_success = false;
	}
	
	// We have our worlds so render the picture to both and divine the alpha channel
	MCSharedString *t_out_data;
	t_out_data = NULL;
	if (t_success)
	{
		Pattern temp;
	
		SetGWorld(t_black_world, NULL);
		PenPat(GetQDGlobalsBlack(&temp));
		PaintRect(&t_rect);
		DrawPicture(t_pict, &t_rect);
		
		SetGWorld(t_white_world, NULL);
		PenPat(GetQDGlobalsWhite(&temp));
		PaintRect(&t_rect);
		DrawPicture(t_pict, &t_rect);
		
		PixMapHandle t_black_pixmap;
		t_black_pixmap = GetGWorldPixMap(t_black_world);
		
		void *t_black_ptr;
		t_black_ptr = GetPixBaseAddr(t_black_pixmap);
		
		uint4 t_black_stride;
		t_black_stride = GetPixRowBytes(t_black_pixmap);
		
		PixMapHandle t_white_pixmap;
		t_white_pixmap = GetGWorldPixMap(t_white_world);
		
		void *t_white_ptr;
		t_white_ptr = GetPixBaseAddr(t_white_pixmap);
		
		uint4 t_white_stride;
		t_white_stride = GetPixRowBytes(t_white_pixmap);
		
		for(uint4 y = 0; y < t_rect . bottom; ++y)
			for(uint4 x = 0; x < t_rect . right; ++x)
			{
#ifdef __BIG_ENDIAN__
				uint1 rw;
				rw = ((uint1 *)t_white_ptr)[y * t_white_stride + x * 4 + 3];

				uint1 rb;
				rb = ((uint1 *)t_black_ptr)[y * t_black_stride + x * 4 + 3];
#else
				uint1 rw;
				rw = ((uint1 *)t_white_ptr)[y * t_white_stride + x * 4 + 0];

				uint1 rb;
				rb = ((uint1 *)t_black_ptr)[y * t_black_stride + x * 4 + 0];
#endif

				uint1 a;
				a = 255 - rw + rb;

				((uint4 *)t_black_ptr)[y * t_black_stride / 4 + x] = packed_scale_bounded(((uint4 *)t_black_ptr)[y * t_black_stride / 4 + x] & 0xFFFFFF, a) | (a << 24);
			}
	
		MCImageBitmap t_bitmap;
		t_bitmap . width = t_rect.right;
		t_bitmap . height = t_rect . bottom;
		t_bitmap . data = (uint32_t*)t_black_ptr;
		t_bitmap . stride = t_black_stride;

		IO_handle t_stream = MCS_fakeopenwrite();

		char *t_bytes = nil;
		uindex_t t_byte_count = 0;

		/* UNCHECKED */ MCImageEncodePNG(&t_bitmap, t_stream, t_byte_count);

		MCS_fakeclosewrite(t_stream, t_bytes, t_byte_count);

		t_out_data = MCSharedString::Create(t_bytes, t_byte_count);
		MCMemoryDeallocate(t_bytes);
	}
	
	if (t_black_world != NULL)
		DisposeGWorld(t_black_world);
	
	if (t_white_world != NULL)
		DisposeGWorld(t_white_world);
	
	if (t_pict != NULL)
		DisposeHandle((Handle)t_pict);

	SetGWorld(t_old_port, t_old_gdevice);

	return t_out_data;
}

MCSharedString *MCConvertMacTIFFToImage(MCSharedString *p_data)
{
	bool t_success;
	t_success = true;

	GraphicsImportComponent t_importer;
	t_importer = 0;
	if (t_success)
	{
		if (OpenADefaultComponent(GraphicsImporterComponentType, kQTFileTypeTIFF, &t_importer) != noErr)
			t_success = false;
	}
	
	GraphicsExportComponent t_exporter;
	t_exporter = 0;
	if (t_success)
	{
		if (OpenADefaultComponent(GraphicsExporterComponentType, kQTFileTypePNG, &t_exporter) != noErr)
			t_success = false;
	}
	
	Handle t_input_dataref_handle;
	t_input_dataref_handle = NULL;
	if (t_success)
	{
		PointerDataRefRecord t_dataref;
		t_dataref . data = (void *)p_data -> GetBuffer();
		t_dataref . dataLength = p_data -> GetLength();
		if (PtrToHand(&t_dataref, &t_input_dataref_handle, sizeof(PointerDataRefRecord)) != noErr)
			t_success = false;
	}
		
	if (t_success)
	{
		if (GraphicsImportSetDataReference(t_importer, t_input_dataref_handle, 'ptr ') != noErr)
			t_success = false;
	}
	
	Handle t_output_handle;
	t_output_handle = NULL;
	if (t_success)
	{
		t_output_handle = NewHandle(0);
		if (t_output_handle == NULL)
			t_success = false;
	}
	
	if (t_success)
	{
		if (GraphicsExportSetInputGraphicsImporter(t_exporter, t_importer) != noErr)
			t_success = false;
	}
	
	if (t_success)
	{
		if (GraphicsExportSetOutputHandle(t_exporter, t_output_handle) != noErr)
			t_success = false;
	}
	
	if (t_success)
	{
		if (GraphicsExportDoExport(t_exporter, NULL) != noErr)
			t_success = false;
	}
	
	MCSharedString *t_result;
	t_result = NULL;
	if (t_success)
	{
		HLock(t_output_handle);
		t_result = MCSharedString::Create(*t_output_handle, GetHandleSize(t_output_handle));
		HUnlock(t_output_handle);
	}
	
	if (t_output_handle != NULL)
		DisposeHandle(t_output_handle);
		
	if (t_input_dataref_handle != NULL)
		DisposeHandle(t_input_dataref_handle);
	
	if (t_exporter != 0)
		CloseComponent(t_exporter);
		
	if (t_importer != 0)
		CloseComponent(t_importer);
		
	return t_result;
	
}

MCSharedString *MCConvertMacHFSToFiles(MCSharedString *p_data)
{
	HFSFlavor *t_items;
	t_items = (HFSFlavor *)(p_data -> GetBuffer());
	
	uint32_t t_count;
	t_count = p_data -> GetLength() / sizeof(HFSFlavor);
	
	MCExecPoint ep(NULL, NULL, NULL);
	for(uint32_t i = 0; i < t_count; ++i)
	{
		FSRef t_fs_ref;
		if (FSpMakeFSRef(&t_items[i] . fileSpec, &t_fs_ref) != noErr)
			continue;
			
		char *t_filename;
		t_filename = MCS_fsref_to_path(t_fs_ref);
		if (t_filename == NULL)
			continue;
			
		ep . concatcstring(t_filename, EC_RETURN, i == 0);
		
		delete t_filename;
	}
	
	return MCSharedString::Create(ep . getsvalue());
}


MCSharedString *MCConvertMacHTMLToStyledText(MCSharedString *p_data)
{
    NSData *t_html_data;
    t_html_data = [[NSData alloc] initWithBytes: p_data->GetBuffer() length: p_data->GetLength()];
    
    NSAttributedString *t_html_string;
    t_html_string = [[NSAttributedString alloc] initWithHTML: t_html_data documentAttributes: nil];
    
    NSData *t_rtf_data;
    t_rtf_data = [t_html_string RTFFromRange: NSMakeRange(0, [t_html_string length]) documentAttributes: nil];

    [t_html_string release];
    [t_html_data release];
    
    MCParagraph *t_paragraphs;
	t_paragraphs = MCtemplatefield -> rtftoparagraphs(MCString((const char *)[t_rtf_data bytes], [t_rtf_data length]));
    
	MCStyledText t_styled_text;
	t_styled_text . setparent(MCdefaultstackptr);
	t_styled_text . setparagraphs(t_paragraphs);
	return MCObject::pickle(&t_styled_text, 0);
}
