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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "field.h"
#include "paragraf.h"
#include "image.h"
#include "dispatch.h"
#include "util.h"
#include "exec.h"
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
		MCValueRelease(m_entries[i] . data);
}

bool MCMacOSXTransferData::Publish(ScrapFlavorType p_type, MCDataRef p_data, MCMacOSXConversionCallback p_converter)
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
		t_new_entries[m_entry_count] . data = MCValueRetain(p_data);
		t_new_entries[m_entry_count] . converter = p_converter;

		m_entries = t_new_entries;
		m_entry_count += 1;
	}

	return t_success;
}

bool MCMacOSXTransferData::Publish(MCTransferType p_type, MCDataRef p_data)
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
		if (t_success && MCImageDataIsPNG(p_data))
			t_success = Publish(flavorTypePNG, p_data, NULL);
		if (t_success && MCImageDataIsGIF(p_data))
			t_success = Publish(flavorTypeGIF, p_data, NULL);
		if (t_success && MCImageDataIsJPEG(p_data))
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
			MCAutoDataRef t_data;
			t_success = p_pasteboard -> Fetch(t_types[i], &t_data);
			if (t_success)
				t_success = Publish(t_types[i], *t_data);
		}

	return t_success;
}

bool MCMacOSXTransferData::Subscribe(ScrapFlavorType p_type, MCDataRef& r_data)
{
	for(uint4 i = 0; i < m_entry_count; ++i)
		if (m_entries[i] . type == p_type)
		{
			if (m_entries[i] . converter != NULL)
			{
				MCAutoDataRef t_converted_data;
				if (!m_entries[i] . converter(m_entries[i] . data, &t_converted_data))
					return false;
				
				MCValueRelease(m_entries[i] . data);
                if (*t_converted_data != nil)
                    m_entries[i] . data = MCValueRetain(*t_converted_data);
                else
                    m_entries[i] . data = nil;
                
				m_entries[i] . converter = NULL;
			}

			if (m_entries[i] . data != nil)
			{
				r_data = MCValueRetain(m_entries[i] . data);
				return true;
			}
		}
	return false;
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
		if (m_entries[i] . data != nil)
			MCValueRelease(m_entries[i] . data);

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
bool MCMacOSXPasteboard::Fetch(MCTransferType p_type, MCDataRef& r_data)
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
		r_data = MCValueRetain(m_entries[t_entry] . data);
		return true;
	}

	MCAutoDataRef t_in_data;
	if (!FetchFlavor(m_entries[t_entry] . flavor, &t_in_data))
		return false;
	
	MCAutoDataRef t_out_data;
	switch(m_entries[t_entry] . flavor)
	{
	case kScrapFlavorTypeText:
	{
		MCAutoStringRef t_input_mac;
		/* UNCHECKED */ MCStringDecode(*t_in_data, kMCStringEncodingMacRoman, false, &t_input_mac);
		MCAutoStringRef t_output;
		/* UNCHECKED */ MCStringConvertLineEndingsToLiveCode(*t_input_mac, &t_output);
		/* UNCHECKED */ MCStringEncode(*t_output, kMCStringEncodingMacRoman, false, &t_out_data);
	}
	break;

	case kScrapFlavorTypeUnicode:
	{

		MCAutoStringRef t_utf8;
		/* UNCHECKED */ MCStringDecode(*t_in_data, kMCStringEncodingUTF16, false, &t_utf8);
		MCAutoStringRef t_output;
		/* UNCHECKED */ MCStringConvertLineEndingsToLiveCode(*t_utf8, &t_output);
		/* UNCHECKED */ MCStringEncode(*t_output, kMCStringEncodingUTF16, false, &t_out_data);

	}
	break;

	case kScrapFlavorTypeTextStyle:
	{
		MCAutoDataRef t_text_data;
		if (FetchFlavor(kScrapFlavorTypeText, &t_text_data))
			MCConvertMacStyledToStyledText(*t_text_data, *t_in_data, &t_out_data);
	}
	break;

	case kScrapFlavorTypeUnicodeStyle:
	{
		bool t_is_external;
		t_is_external = false;
		
		// MW-2010-01-08: [[ Bug 8327 ]] Make sure we fetch the correct unicode text!
		MCAutoDataRef t_text_data;
		if (FetchFlavor(kScrapFlavorTypeUTF16External, &t_text_data))
			t_is_external = true;
		if (*t_text_data != nil || FetchFlavor(kScrapFlavorTypeUnicode, &t_text_data))
			MCConvertMacUnicodeStyledToStyledText(*t_text_data, *t_in_data, t_is_external, &t_out_data);
	}
	break;

	// MW-2011-01-17: Add support for fetching RTF encoded text from clipboard.
	case flavorTypeRichText:
	{
		MCAutoDataRef t_text_data;
		if (FetchFlavor(flavorTypeRichText, &t_text_data))
			MCConvertRTFToStyledText(*t_text_data, &t_out_data);
	}
	break;
	
	// MW-2012-11-19: [[ Bug 10542 ]] Add support for processing HTML encoded text from clipboard.
	case flavorTypeHtml:
	{
		MCAutoDataRef t_text_data;
		if (FetchFlavor(flavorTypeHtml, &t_text_data))
		{
            // SN-2013-07-26: [[ Bug 10893 ]] Convert HTML to RTF using Apple's internal class
			MCConvertMacHTMLToStyledText(*t_text_data, &t_out_data);
		}
	}
	break;
							
	case kScrapFlavorTypePicture:
		MCConvertMacPictureToImage(*t_in_data, &t_out_data);	break;

	case flavorTypeTIFF:
		// MW-2010-11-17: [[ Bug 9183 ]] Check the data is actually TIFF, it is actually a PNG then
		//   do nothing
		if (MCDataGetLength(*t_in_data) >= 4 && memcmp(MCDataGetBytePtr(*t_in_data), "\211PNG", 4) == 0)
			&t_out_data = MCValueRetain(*t_in_data);
		else
			MCConvertMacTIFFToImage(*t_in_data, &t_out_data);
	break;

	case flavorTypeHFS:
		MCConvertMacHFSToFiles(*t_in_data, &t_out_data);
	break;

	case flavorTypeRevolutionText:
	case flavorTypeRevolutionObjects:
	case flavorTypePNG:
	case flavorTypeGIF:
	case flavorTypeJPEG:
		t_out_data = *t_in_data;
	break;
	}

	if (*t_out_data == nil)
		return false;

	m_entries[t_entry] . data = MCValueRetain(*t_out_data);
	r_data = MCValueRetain(*t_out_data);

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

bool MCConvertTextToMacPlain(MCDataRef p_input, MCDataRef& r_output)
{
	MCAutoStringRef t_input_mac;
	/* UNCHECKED */ MCStringDecode(p_input, kMCStringEncodingMacRoman, false, &t_input_mac);
	MCAutoStringRef t_output;
	/* UNCHECKED */ MCStringConvertLineEndingsFromLiveCode(*t_input_mac, &t_output);
	
	return MCStringEncode(*t_output, kMCStringEncodingMacRoman, false, r_output);
}

bool MCConvertUnicodeToMacUnicode(MCDataRef p_input, MCDataRef& r_output)
{

	MCAutoStringRef t_input_mac_unicode;
	/* UNCHECKED */ MCStringDecode(p_input, kMCStringEncodingUTF16, false, &t_input_mac_unicode);
	MCAutoStringRef t_output;
	/* UNCHECKED */ MCStringConvertLineEndingsFromLiveCode(*t_input_mac_unicode, &t_output);
	
	return MCStringEncode(*t_output, kMCStringEncodingUTF16, false, r_output);
}

bool MCConvertStyledTextToMacUnicode(MCDataRef p_input, MCDataRef& r_output)
{
	MCAutoDataRef t_unicode;
	
	if (!MCConvertStyledTextToUnicode(p_input, &t_unicode))
		return false;
	
	return MCConvertUnicodeToMacUnicode(*t_unicode, r_output);
}

bool MCConvertStyledTextToMacUnicodeStyled(MCDataRef p_input, MCDataRef& r_output)
{
	MCObject *t_object;
	t_object = MCObject::unpickle(p_input, MCtemplatefield -> getstack());
	if (t_object == nil)
		return false;
	
	MCParagraph *t_paragraphs;
	t_paragraphs = ((MCStyledText *)t_object) -> getparagraphs();
	
	/* UNCHECKED */ MCtemplatefield -> getparagraphmacunicodestyles(t_paragraphs, t_paragraphs -> prev(), r_output);
	
	delete t_object;
	
	return true;
}

bool MCConvertStyledTextToMacPlain(MCDataRef p_input, MCDataRef& r_output)
{	
	MCAutoDataRef t_text;
	if (!MCConvertStyledTextToText(p_input, &t_text))
		return false;
	
	return MCConvertTextToMacPlain(*t_text, r_output);
}

bool MCConvertImageToMacPicture(MCDataRef p_input, MCDataRef &r_output)
{
	bool t_success = true;
	
	MCMacSysPictHandle t_handle = nil;
	MCImageFrame *t_frames = nil;
	uindex_t t_frame_count = 0;
	
	t_success = MCImageDecode((const uint8_t*)MCDataGetBytePtr(p_input), MCDataGetLength(p_input), t_frames, t_frame_count) &&
	MCImageBitmapToPICT(t_frames[0].image, t_handle);
	
	MCImageFreeFrames(t_frames, t_frame_count);
	
	if (t_success)
	{
		HLock((Handle)t_handle);
		t_success = MCDataCreateWithBytes((const char_t *)t_handle, GetHandleSize((Handle)t_handle), r_output);
		HUnlock((Handle)t_handle);
		DisposeHandle((Handle)t_handle);
	}
	
	return t_success;
}

bool MCConvertFilesToMacHFS(MCDataRef p_input, MCDataRef& r_output)
{
	uint4 t_length = MCDataGetLength(p_input);
	const char *t_buffer = (const char *)MCDataGetBytePtr(p_input);
	
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
        MCAutoStringRef t_path;
        /* UNCHECKED */ MCStringCreateWithNativeChars((char_t*) t_start, t_buffer - t_start, &t_path);
		/* UNCHECKED */ MCS_mac_pathtoref(*t_path, t_ref);
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

	if (MCDataCreateWithBytesAndRelease((char_t *)t_items, sizeof(HFSFlavor) * t_count, r_output))
		return true;
	
	free(t_items);
	
	return false;
}

//

bool MCConvertMacStyledToStyledText(MCDataRef p_text_data, MCDataRef p_style_data, MCDataRef& r_output)
{
	MCParagraph *t_paragraphs;
	t_paragraphs = MCtemplatefield -> macstyletexttoparagraphs(MCDataGetOldString(p_text_data), MCDataGetOldString(p_style_data), False);
	
	MCStyledText t_styled_text;
	t_styled_text . setparent(MCtemplatefield -> getparent());
	t_styled_text . setparagraphs(t_paragraphs);
	
	/* UNCHECKED */ MCObject::pickle(&t_styled_text, 0, r_output);
	
	return true;
}

bool MCConvertMacUnicodeStyledToStyledText(MCDataRef p_text_data, MCDataRef p_style_data, bool p_is_external, MCDataRef& r_output)
{
	MCParagraph *t_paragraphs;
	
	// MW-2010-01-08: [[ Bug 8327 ]] If the text is 'external' representation, skip the BOM.
	if (p_is_external && MCDataGetLength(p_text_data) >= 2 &&
		(*(uint2 *)MCDataGetBytePtr(p_text_data) == 0xfffe ||
		*(uint2 *)MCDataGetBytePtr(p_text_data) == 0xfeff))
	{
        /* UNCHECKED */ MCDataRemove(p_text_data, MCRangeMake(0,2));
	}
	
	// MW-2009-12-01: If the unicode styled text has an empty style data, then make
	//   sure we just convert it as plain unicode text.
	if (!MCDataIsEmpty(p_style_data))
		t_paragraphs = MCtemplatefield -> macunicodestyletexttoparagraphs(p_text_data, p_style_data);
	else
	{
        MCAutoStringRef t_input;
        /* UNCHECKED */ MCStringDecode(p_text_data, kMCStringEncodingNative, false, &t_input);
		MCAutoStringRef t_output;
		/* UNCHECKED */ MCStringConvertLineEndingsToLiveCode(*t_input, &t_output);
		MCAutoDataRef t_data;
		/* UNCHECKED */ MCStringEncode(*t_output, kMCStringEncodingUTF16, false, &t_data);
		t_paragraphs = MCtemplatefield -> texttoparagraphs(MCDataGetOldString(*t_data), true);
	}
	
	MCStyledText t_styled_text;
	t_styled_text . setparent(MCtemplatefield -> getparent());
	t_styled_text . setparagraphs(t_paragraphs);
	
	/* UNCHECKED */ MCObject::pickle(&t_styled_text, 0, r_output);
	
	return true;
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

bool MCConvertMacPictureToImage(MCDataRef p_data, MCDataRef &r_output)
{
	const uint8_t *t_data_ptr = MCDataGetBytePtr(p_data);
	uint32_t t_data_len = MCDataGetLength(p_data);
	
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
		t_pict = (PicHandle)NewHandle(t_data_len);
		if (t_pict == NULL)
			t_success = false;
	}
	
	if (t_success)
	{
		HLock((Handle)t_pict);
		memcpy(*t_pict, t_data_ptr, t_data_len);
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
		/* UNCHECKED */ MCS_closetakingbuffer(t_stream, reinterpret_cast<void*&>(t_bytes), reinterpret_cast<size_t&>(t_byte_count));

		t_success = MCDataCreateWithBytes((const char_t*)t_bytes, t_byte_count, r_output);
		MCMemoryDeallocate(t_bytes);
	}
	
	if (t_black_world != NULL)
		DisposeGWorld(t_black_world);
	
	if (t_white_world != NULL)
		DisposeGWorld(t_white_world);
	
	if (t_pict != NULL)
		DisposeHandle((Handle)t_pict);

	SetGWorld(t_old_port, t_old_gdevice);

	return t_success;
}

bool MCConvertMacTIFFToImage(MCDataRef p_data, MCDataRef& r_output)
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
		t_dataref . data = (void *)MCDataGetBytePtr(p_data);
		t_dataref . dataLength = MCDataGetLength(p_data);
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
	
	if (t_success)
	{
		HLock(t_output_handle);
		t_success = MCDataCreateWithBytes((const char_t *)*t_output_handle, GetHandleSize(t_output_handle), r_output);
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
		
	return t_success;
}

bool MCConvertMacHFSToFiles(MCDataRef p_data, MCDataRef& r_output)
{
	HFSFlavor *t_items;
	t_items = (HFSFlavor *)MCDataGetBytePtr(p_data);
	
	uint32_t t_count;
	t_count = MCDataGetLength(p_data) / sizeof(HFSFlavor);

	// Create a mutable list ref.
	MCListRef t_output_files;
	/* UNCHECKED */ MCListCreateMutable('\n', t_output_files);
	
	for(uint32_t i = 0; i < t_count; ++i)
	{
		FSRef t_fs_ref;
		if (FSpMakeFSRef(&t_items[i] . fileSpec, &t_fs_ref) != noErr)
			continue;
			
		MCAutoStringRef t_filename;
		
		if (!MCS_mac_fsref_to_path(t_fs_ref, &t_filename))
			continue;
		
		/* UNCHECKED */ MCListAppend(t_output_files, *t_filename);
	}

	// Build the output stringref.
	MCStringRef t_output_files_string;
	/* UNCHECKED */ MCListCopyAsStringAndRelease(t_output_files, t_output_files_string);

	// Finally encode as native string and encapsulate in a dataref.
	/* UNCHECKED */ MCStringEncodeAndRelease(t_output_files_string, kMCStringEncodingNative, false, r_output);

	return true;
}

bool MCConvertMacHTMLToStyledText(MCDataRef p_input, MCDataRef& r_output)
{
    NSData *t_html_data;
    t_html_data = [NSData dataWithMCDataRef: p_input];
    
    NSAttributedString *t_html_string;
    t_html_string = [[NSAttributedString alloc] initWithHTML: t_html_data documentAttributes: nil];
    
    NSData *t_rtf_data;
    t_rtf_data = [t_html_string RTFFromRange: NSMakeRange(0, [t_html_string length]) documentAttributes: nil];

    [t_html_string release];
    
    MCParagraph *t_paragraphs;
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringCreateWithCString((const char *)[t_rtf_data bytes], &t_string);
	t_paragraphs = MCtemplatefield -> rtftoparagraphs(*t_string);
    
	MCStyledText t_styled_text;
	t_styled_text . setparent(MCdefaultstackptr);
	t_styled_text . setparagraphs(t_paragraphs);
	/* UNCHECKED */ MCObject::pickle(&t_styled_text, 0, r_output);
    return true;
}
