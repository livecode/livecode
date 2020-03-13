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


#include "w32-clipboard.h"
#include "osspec.h"

#include "globdefs.h"
#include "filedefs.h"
#include "mcio.h"
#include "imagebitmap.h"
#include "image.h"

#include <ObjIdl.h>
#include <ShlObj.h>

// Mapping from MCRawClipboardKnownType values to clipboard formats (as atoms 
// and strings). This table will be modifed as clipboard formats are registered.
MCWin32RawClipboardCommon::format_mapping MCWin32RawClipboardCommon::s_formats[] =
{
	{ 0, NULL },										// UTF-8 text
	{ CF_UNICODETEXT, "CF_UNICODETEXT" },				// UTF-16 text
	{ 0, NULL },										// ISO-8859-1 text
	{ 0, NULL },										// MacRoman text
	{ CF_TEXT, "CF_TEXT" },								// Codepage 1252 text

	{ 0, "Rich Text Format" },							// Rich Text
	{ 0, "HTML Format" },								// HTML

	{ 0, "PNG" },										// PNG image
	{ 0, "GIF" },										// GIF image
	{ 0, "JFIF" },										// JPEG image
	{ 6, "CF_TIFF" },									// TIFF image
	{ CF_METAFILEPICT, "CF_METAFILEPICT" },				// Windows Metafile image
	{ CF_ENHMETAFILE, "CF_ENHMETAFILE" },				// Windows Enhanced Metafile image
	{ CF_DIB, "CF_DIB" },								// Windows bitmap image
	{ CF_DIBV5, "CF_DIBV5" },							// Windows bitmap image (V5)

	{ 0, "com.runrev.revolution.objects/1" },			// LiveCode objects
	{ 0, "com.runrev.revolution.text/styled/1" },		// LiveCode styled text

	{ 0, NULL },										// File path
	{ CF_HDROP, "CF_HDROP" },							// File path list
	{ 0, NULL },										// URL
	{ 0, NULL },										// URL list

	// Other built-in clipboard formats. We don't handle them but they also
	// don't have a string returned from GetClipboardFormatString so we have to
	// perform the mapping ourselves if we don't want a numeric type string.

	{ 2, "CF_BITMAP" },									// Handle (HBITMAP)
	{ 5, "CF_DIF" },									// Data Interchange Format
	{ 0x82, "CF_DSPBITMAP" },							// Display bitmap for a private format
	{ 0x8E, "CF_DSPENHMETAFILE" },						// Display EMF for a private format
	{ 0x83, "CF_DSPMETAFILEPICT" },						// Display WMF for a private format
	{ 0x81, "CF_DSPTEXT" },								// Display text for a private format
	{ 16, "CF_LOCALE" },								// Locale for the text data
	{ 7, "CF_OEMTEXT" },								// Text in the OEM encoding
	{ 0x80, "CF_OWNERDISPLAY" },						// Owner-drawn content
	{ 9, "CF_PALETTE" },								// Handle to a colour palette
	{ 10, "CF_PENDATA" },								// Windows 3.1 (!) pen input data
	{ 11, "CF_RIFF" },									// RIFF-encoded audio
	{ 4, "CF_SYLK" },									// Microsoft Symbolic Link format
	{ 12, "CF_WAVE" },									// PCM audio
};

#define kFormatMappingTableLength (sizeof(MCWin32RawClipboardCommon::s_formats) / sizeof(MCWin32RawClipboardCommon::s_formats[0]))


MCRawClipboard* MCRawClipboard::CreateSystemClipboard()
{
	return new MCWin32RawClipboard;
}

MCRawClipboard* MCRawClipboard::CreateSystemSelectionClipboard()
{
	return new MCWin32RawClipboardNull;
}

MCRawClipboard* MCRawClipboard::CreateSystemDragboard()
{
	return new MCWin32RawClipboardNull;
}


MCWin32RawClipboardCommon::MCWin32RawClipboardCommon() :
  MCRawClipboard(),
  m_item(NULL),
  m_dirty(false),
  m_external_data(false)
{
	;
}

MCWin32RawClipboardCommon::~MCWin32RawClipboardCommon()
{
	// Ensure all resources are freed
	Clear();
}


MCWin32RawClipboard::MCWin32RawClipboard() :
  MCWin32RawClipboardCommon()
{
	;
}

MCWin32RawClipboardNull::MCWin32RawClipboardNull() :
  MCWin32RawClipboardCommon()
{
	;
}

uindex_t MCWin32RawClipboardCommon::GetItemCount() const
{
	// The Windows clipboard can only hold a single item
	return m_item == NULL ? 0 : 1;
}

const MCWin32RawClipboardItem* MCWin32RawClipboardCommon::GetItemAtIndex(uindex_t p_index) const
{
	// Only a single item is supported
	if (p_index > 0 || m_item == NULL)
		return NULL;
	m_item->Retain();
	return m_item;
}

MCWin32RawClipboardItem* MCWin32RawClipboardCommon::GetItemAtIndex(uindex_t p_index)
{
	// Only a single item is supported
	if (p_index > 0 || m_item == NULL)
		return NULL;
	m_item->Retain();
	return m_item;
}

void MCWin32RawClipboardCommon::Clear()
{
	// Discard any data we currently have stored
	m_dirty = true;
	m_external_data = false;
	if (m_item)
		m_item->Release();
	m_item = NULL;
}

bool MCWin32RawClipboardCommon::IsExternalData() const
{
	return m_external_data;
}

MCWin32RawClipboardItem* MCWin32RawClipboardCommon::CreateNewItem()
{
	// Fail if there is an existing item on the clipboard
	if (m_item != NULL)
		return NULL;

	// Create a new data item
	m_item = new (nothrow) MCWin32RawClipboardItem(this);
	m_item->Retain();
	
	return m_item;
}

bool MCWin32RawClipboardCommon::AddItem(MCRawClipboardItem* p_item)
{
	// Fail if this is not the single item belonging to this clipboard
	if (m_item != p_item)
		return false;

	// Item was already added at creation time
	m_dirty = true;
	return true;
}

uindex_t MCWin32RawClipboardCommon::GetMaximumItemCount() const
{
	// Only a single item is supported
	return 1;
}

MCStringRef MCWin32RawClipboardCommon::GetKnownTypeString(MCRawClipboardKnownType p_type) const
{	
	// Index into the mapping table
	if (p_type > kMCRawClipboardKnownTypeLast)
		return NULL;
	if (s_formats[p_type].string != NULL)
		return MCSTR(s_formats[p_type].string);

	return NULL;
}

MCDataRef MCWin32RawClipboardCommon::EncodeFileListForTransfer(MCStringRef p_list) const
{
	// Create a mutable copy of the list
	MCAutoStringRef t_copy;
	if (!MCStringMutableCopy(p_list, &t_copy))
		return NULL;

	// Replace all newlines with NULs
	if (!MCStringFindAndReplaceChar(*t_copy, '\n', '\0', kMCStringOptionCompareExact))
		return NULL;

	// Ensure that the string is double-NUL terminated (adding extras here
	// isn't harmful so we just do it unconditionally).
	if (!MCStringAppendChar(*t_copy, 0) || !MCStringAppendChar(*t_copy, 0))
		return NULL;

	// Encode the string as UTF-16 data (we can never safely use the 'native'
	// encoding because it might not be Codepage 1252).
	MCDataRef t_encoded_paths;
	if (!MCStringEncode(*t_copy, kMCStringEncodingUTF16, false, t_encoded_paths))
		return NULL;

	// We need to be able to modify the encoded data
	MCAutoDataRef t_data;
	if (!MCDataMutableCopyAndRelease(t_encoded_paths, &t_data))
	{
		MCValueRelease(t_encoded_paths);
		return NULL;
	}

	// Generate the DROPFILES structure
	DROPFILES t_header;
	memset(&t_header, 0, sizeof(DROPFILES));
	t_header.pFiles = sizeof(DROPFILES);		// Offset to the path list
	t_header.fWide = true;						// We're using UTF-16

	// Prepend this header to the encoded data
	if (!MCDataPrependBytes(*t_data, (const byte_t*)&t_header, sizeof(DROPFILES)))
		return NULL;

	// File list has been encoded
	return MCValueRetain(*t_data);
}

MCStringRef MCWin32RawClipboardCommon::DecodeTransferredFileList(MCDataRef p_data) const
{
	// The encoded data begins with a DROPFILES structure
	const DROPFILES* t_dropfiles = reinterpret_cast<const DROPFILES*> (MCDataGetBytePtr(p_data));

	// Get a pointer to the path data
	byte_t* t_bytes = reinterpret_cast<byte_t*> (uintptr_t(MCDataGetBytePtr(p_data)) + t_dropfiles->pFiles);

	// Calculate the length of the paths.
	uindex_t t_path_char_count = 0;
	bool t_done = false;
	bool t_between_strings = true;
	const byte_t* t_cursor_ansi = t_bytes;
	const unichar_t* t_cursor_wide = reinterpret_cast<const unichar_t*> (t_bytes);
	while (!t_done)
	{
		// UTF-16 encoded or ANSI encoded?
		if (t_dropfiles->fWide && *t_cursor_wide == '\0')
		{
			// If we are between strings, this is the terminator
			if (t_between_strings)
			{
				t_done = true;
				break;
			}

			// We are now between strings
			t_between_strings = true;
		}
		else if (!t_dropfiles->fWide && *t_cursor_ansi == '\0')
		{
			// If we are between strings, this is the terminator
			if (t_between_strings)
			{
				t_done = true;
				break;
			}

			// We are now between strings
			t_between_strings = true;
		}
		else
		{
			// No longer between strings
			t_between_strings = false;
		}

		// Next character
		t_cursor_ansi++;
		t_cursor_wide++;
		t_path_char_count++;
	}

	// Decode the paths into a StringRef
	MCAutoStringRef t_decoded;
	if (t_dropfiles->fWide)
		MCStringCreateWithBytes(t_bytes, t_path_char_count*2, kMCStringEncodingUTF16, false, &t_decoded);
	else 
		MCStringCreateWithBytes(t_bytes, t_path_char_count*1, kMCStringEncodingNative, false, &t_decoded);
    
    if (*t_decoded == nullptr)
        return nullptr;
    
    // Create a mutable list ref.
    MCAutoListRef t_output;
    if (!MCListCreateMutable('\n', &t_output))
        return nullptr;
    
    // Split the file name list into individual paths
    MCAutoArrayRef t_native_paths;
    if (!MCStringSplit(*t_decoded, kMCNulString, NULL, kMCStringOptionCompareExact, &t_native_paths))
        return nullptr;
    
    uindex_t npaths = MCArrayGetCount(*t_native_paths);
    
    for (uindex_t i = 0; i < npaths; i++)
    {
        MCValueRef t_native_path_val = nil;
        if (!MCArrayFetchValueAtIndex(*t_native_paths, i + 1, t_native_path_val))
            return nullptr;
        MCStringRef t_native_path = (MCStringRef)t_native_path_val;
        if (!MCStringIsEmpty(t_native_path))
        {
            MCAutoStringRef t_path;
            if(!MCS_pathfromnative(t_native_path, &t_path) || !MCListAppend(*t_output, *t_path))
                return nullptr;
        }
    }
    
    MCAutoStringRef t_result;
    if (!MCListCopyAsString(*t_output, &t_result))
        return nullptr;

	// Done
    return t_result.Take();
}

MCDataRef MCWin32RawClipboardCommon::EncodeHTMLFragmentForTransfer(MCDataRef p_html) const
{
	const char *t_header_template = "Version:0.9\r\nStartHTML:%010d\r\nEndHTML:%010d\r\nStartFragment:%010d\r\nEndFragment:%010d\r\n";
	const char *t_doc_prefix = "<html><body><!--StartFragment -->";
	const char *t_doc_suffix = "<!--EndFragment --></body></html>";

	uindex_t t_starthtml, t_endhtml, t_startfragment, t_endfragment;
	// length of header will be length of template + difference between placeholder length and inserted string length
	// numbers are padded to 10 digits using '0's
	t_starthtml = strlen(t_header_template) + 4 * (10 - 5);
	t_startfragment = t_starthtml + strlen(t_doc_prefix);
	t_endfragment = t_startfragment + MCDataGetLength(p_html);
	t_endhtml = t_endfragment + strlen(t_doc_suffix);

	MCAutoStringRef t_header;
	MCAutoStringRef t_start_string;
	MCAutoStringRef t_end_string;

	if (!MCStringFormat(&t_header, t_header_template, t_starthtml, t_endhtml, t_startfragment, t_endfragment))
		return nil;

	MCAutoDataRef t_header_data;
	if (!MCStringEncode(*t_header, kMCStringEncodingUTF8, false, &t_header_data))
		return nil;

	MCAutoDataRef t_data;
	if (!MCDataMutableCopy(*t_header_data, &t_data))
		return nil;

	if (!MCDataAppendBytes(*t_data, (const byte_t*)t_doc_prefix, strlen(t_doc_prefix)))
		return nil;

	if (!MCDataAppend(*t_data, p_html))
		return nil;

	if (!MCDataAppendBytes(*t_data, (const byte_t*)t_doc_suffix, strlen(t_doc_suffix)))
		return nil;

	return t_data.Take();
}

bool MCWin32RawClipboardGetHTMLDataHeader(MCDataRef p_data, uindex_t &x_index, MCStringRef &r_key, MCStringRef &r_value)
{
	MCAutoStringRef t_key;
	MCAutoStringRef t_value;

	uindex_t t_length;
	t_length = MCDataGetLength(p_data);

	const byte_t *t_data_ptr;
	t_data_ptr = MCDataGetBytePtr(p_data);

	uindex_t t_index = x_index;

	for (uint32_t i = t_index; i < t_length; i++)
	{
		if (char(t_data_ptr[i]) == ':')
		{
			if (!MCStringCreateWithBytes(t_data_ptr + t_index, i - t_index, kMCStringEncodingUTF8, false, &t_key))
				return false;
			t_index = i + 1;
			break;
		}

		// end of line with no ':' char - not a header
		if (char(t_data_ptr[i]) == '\r' || char(t_data_ptr[i]) == '\n')
			return false;

		// start of html data - no header
		if (char(t_data_ptr[i]) == '<')
			return false;
	}

	// reached end without finding key
	if (*t_key == nil)
		return false;

	// look for end of line - may be cr, lf, or crlf
	for (uindex_t i = t_index; i < t_length; i++)
	{
		// end of line
		if (char(t_data_ptr[i]) == '\r' || char(t_data_ptr[i]) == '\n')
		{
			if (!MCStringCreateWithBytes(t_data_ptr + t_index, i - t_index, kMCStringEncodingUTF8, false, &t_value))
				return false;
			t_index = i + 1;
			// check for crlf
			if (char(t_data_ptr[i]) == '\r' && t_index < t_length && char(t_data_ptr[t_index]) == '\n')
				t_index++;

			break;
		}
	}

	// reached end without finding line ending
	if (*t_value == nil)
		return false;

	r_key = t_key.Take();
	r_value = t_value.Take();

	x_index = t_index;

	return true;
}

MCDataRef MCWin32RawClipboardCommon::DecodeTransferredHTML(MCDataRef p_data) const 
{
	bool t_success = true;

	uindex_t t_index = 0;

	index_t t_starthtml = -1;
	index_t t_endhtml = -1;
	index_t t_startfragment = -1;
	index_t t_endfragment = -1;
	index_t t_startselection = -1;
	index_t t_endselection = -1;

	MCStringRef t_headerkey = nil;
	MCStringRef t_headervalue = nil;

	while (MCWin32RawClipboardGetHTMLDataHeader(p_data, t_index, t_headerkey, t_headervalue))
	{
		MCAutoNumberRef t_number;
		if (MCStringIsEqualToCString(t_headerkey, "starthtml", kMCStringOptionCompareCaseless))
		{
			if (MCNumberParse(t_headervalue, &t_number))
				t_starthtml = MCNumberFetchAsInteger(*t_number);
		}
		else if (MCStringIsEqualToCString(t_headerkey, "endhtml", kMCStringOptionCompareCaseless))
		{
			if (MCNumberParse(t_headervalue, &t_number))
				t_endhtml = MCNumberFetchAsInteger(*t_number);
		}
		else if (MCStringIsEqualToCString(t_headerkey, "startfragment", kMCStringOptionCompareCaseless))
		{
			if (MCNumberParse(t_headervalue, &t_number))
				t_startfragment = MCNumberFetchAsInteger(*t_number);
		}
		else if (MCStringIsEqualToCString(t_headerkey, "endfragment", kMCStringOptionCompareCaseless))
		{
			if (MCNumberParse(t_headervalue, &t_number))
				t_endfragment = MCNumberFetchAsInteger(*t_number);
		}
		else if (MCStringIsEqualToCString(t_headerkey, "startselection", kMCStringOptionCompareCaseless))
		{
			if (MCNumberParse(t_headervalue, &t_number))
				t_startselection = MCNumberFetchAsInteger(*t_number);
		}
		else if (MCStringIsEqualToCString(t_headerkey, "endselection", kMCStringOptionCompareCaseless))
		{
			if (MCNumberParse(t_headervalue, &t_number))
				t_endselection = MCNumberFetchAsInteger(*t_number);
		}

		MCValueRelease(t_headerkey);
		MCValueRelease(t_headervalue);
		t_headerkey = nil;
		t_headervalue = nil;
	}

	uindex_t t_start = -1;
	uindex_t t_end = -1;

	// Strip off the HTML fragment headers but leave the context elements intact;
	// the legacy clipboard round-trips through a field object so these will get
	// removed then while they will be retained for the fullClipboardData.
	if (t_starthtml != -1 && t_endhtml != -1)
	{
		t_start = t_starthtml;
		t_end = t_endhtml;
	}

	t_end = MCClamp(t_end, 0, MCDataGetLength(p_data));
	t_start = MCClamp(t_start, 0, t_end);

	MCDataRef t_decoded;
	if (MCDataCopyRange(p_data, MCRangeMakeMinMax(t_start, t_end), t_decoded))
		return t_decoded;

	return nil;
}

MCDataRef MCWin32RawClipboardCommon::EncodeBMPForTransfer(MCDataRef p_bmp) const
{
	// Strip the BITMAPFILEHEADER structure from the BMP and pass the BMP in its in-memory form
	MCAutoDataRef t_data;
	size_t t_offset = sizeof(BITMAPFILEHEADER);
	if (!MCDataCopyRange(p_bmp, MCRangeMakeMinMax(t_offset, MCDataGetLength(p_bmp)), &t_data))
		return nil;
	return *t_data;
}

MCDataRef MCWin32RawClipboardCommon::DecodeTransferredBMP(MCDataRef p_bmp) const
{
	// Add a BITMAPFILEHEADER structure to the front of the in-memory form
	BITMAPFILEHEADER t_header = { 0 };
	t_header.bfType = 'MB';
	t_header.bfSize = sizeof(BITMAPFILEHEADER) + MCDataGetLength(p_bmp);
	t_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPV5HEADER);

	MCAutoDataRef t_bmp;
	if (!MCDataCreateMutable(0, &t_bmp))
		return nil;

	// Add the header
	if (!MCDataAppendBytes(*t_bmp, (const byte_t*)&t_header, sizeof(BITMAPFILEHEADER)))
		return nil;

	// Add the bitmap data
	if (!MCDataAppend(*t_bmp, p_bmp))
		return nil;

	return t_bmp.Take();
}

void MCWin32RawClipboardCommon::SetDirty()
{
	m_dirty = true;
}

bool MCWin32RawClipboardCommon::SetToIDataObject(IDataObject* p_object)
{
	// Do nothing if this is our own object
	if (m_item && p_object == m_item->GetIDataObject())
		return true;

	// Also do nothing if this is a request to clear a non-external object
	if (!m_external_data && p_object == NULL)
		return true;
	
	// Clear the clipboard
	Clear();

	// Create a wrapper for this object
	m_external_data = true;
	m_item = new (nothrow) MCWin32RawClipboardItem(this, p_object);
	return true;
}

UINT MCWin32RawClipboardCommon::CopyAtomForType(MCStringRef p_type)
{
	// Check for a valid type
	if (p_type == NULL || MCStringIsEmpty(p_type))
		return 0;

	// Search the known-types table for a match
	for (uindex_t i = 0; i < kFormatMappingTableLength; i++)
	{
		// Does it match the type in this slot of the table?
		if (s_formats[i].string != NULL && MCStringIsEqualToCString(p_type, s_formats[i].string, kMCStringOptionCompareCaseless))
		{
			// Is there an atom assigned for this type?
			if (s_formats[i].atom == 0)
			{
				// No atom has been assigned yet; do that now
				MCAutoStringRefAsWString t_type;
				if (!t_type.Lock(p_type))
					return 0;

				// If this fails, it returns 0 (our invalid-atom constant)
				UINT t_atom = RegisterClipboardFormatW(*t_type);
				s_formats[i].atom = t_atom;
			}

			// Return the atom
			return s_formats[i].atom;
		}
	}

	// Register a new atom for this type
	MCAutoStringRefAsWString t_type;
	if (!t_type.Lock(p_type))
		return 0;

	// If this fails, it returns 0 (our invalid-atom constant)
	UINT t_atom = RegisterClipboardFormatW(*t_type);
	return t_atom;
}

MCStringRef MCWin32RawClipboardCommon::CopyTypeForAtom(UINT p_atom)
{
	// Reject an atom of type CF_NULL
	if (p_atom == 0)
		return NULL;

	// Search the known-types table for a match
	for (uindex_t i = 0; i < kFormatMappingTableLength; i++)
	{
		// Does it match the atom in this slot of the table?
		if (p_atom == s_formats[i].atom)
			return MCSTR(s_formats[i].string);
	}

	// No match. Ask the system for the type's string. This needs to
	// be done in a loop as the length of the name is unknown.
	bool t_done = false;
	uindex_t t_length = 0;
	MCAutoArray<WCHAR> t_chars;
	if (!t_chars.Extend(256))
		return NULL;
	while (!t_done)
	{
		// This function returns 0 on error or the number of characters copied
		// on success. If the number of characters equals the length of the
		// buffer, we have to assume that the name got truncated.
		int t_result = GetClipboardFormatNameW(p_atom, t_chars.Ptr(), t_chars.Size());
		if (t_result <= 0)
			break;
		if (uindex_t(t_result) < t_chars.Size())
		{
			t_done = true;
			t_length = t_result;
			break;
		}
		
		// Try to expand the buffer
		if (!t_chars.Extend(2 * t_chars.Size()))
			return NULL;
	}

	// Turn the result into a StringRef. If we failed to get a string, just
	// convert the number into a string.
	MCAutoStringRef t_type;
	if (t_done)
	{
		// Convert the UTF-16 codeunits into a StringRef
		if (!MCStringCreateWithBytes((const byte_t*)t_chars.Ptr(), t_length*2, kMCStringEncodingUTF16, false, &t_type))
			return NULL;
	}
	else
	{
		// Format the number into a string
		if (!MCStringFormat(&t_type, "%u", p_atom))
			return NULL;
	}

	// Done
	return MCValueRetain(*t_type);
}


bool MCWin32RawClipboard::IsOwned() const
{
	// Get the IDataObject that we are placing onto the clipboard
	IDataObject* t_data_object = NULL;
	if (m_item != NULL)
		t_data_object = m_item->GetIDataObject();

	// Check if our data object is the current clipboard data object
	if (t_data_object == NULL)
		return true;
	return S_OK == OleIsCurrentClipboard(t_data_object);
}


bool MCWin32RawClipboard::PushUpdates()
{
	// Do nothing if there are no changes to push
	if (!m_dirty)
		return true;

	// Get the IDataObject that we are placing onto the clipboard
	IDataObject* t_data_object = NULL;
	if (m_item != NULL)
		t_data_object = m_item->GetIDataObject();

	// Push the item onto the clipboard. If it is NULL, the clipboard will be
	// cleared (which is what we want).
	HRESULT t_result = OleSetClipboard(t_data_object);

	// Clipboard is now clean
	if (t_result == S_OK)
	{
		m_dirty = false;
	}

	return (t_result == S_OK);
}

bool MCWin32RawClipboard::PullUpdates()
{
	// If we're still the owner of the clipboard, do nothing
	if (m_item != NULL && IsOwned())
		return true;

	// Release the current clipboard contents
	if (m_item)
		m_item->Release();
	m_item = NULL;

	// Fetch the current clipboard IDataObject
	IDataObject* t_contents;
	HRESULT t_result = OleGetClipboard(&t_contents);
	if (t_result != S_OK)
		return false;

	// Create a new item to wrap this data object
	m_external_data = true;
	m_item = new (nothrow) MCWin32RawClipboardItem(this, t_contents);

	if (t_contents != NULL)
		t_contents->Release();

	return (m_item != NULL);
}

bool MCWin32RawClipboard::FlushData()
{
	// Flush the clipboard
	return OleFlushClipboard() == S_OK;
}

MCWin32RawClipboardItem* MCWin32RawClipboard::CreateNewItem()
{
	MCWin32RawClipboardItem *t_item;
	t_item = MCWin32RawClipboardCommon::CreateNewItem();

	if (t_item == NULL)
		return NULL;

	// fetch data object and push it to the clipboard
	IDataObject * t_contents = t_item->GetDataObject();
	OleSetClipboard(t_contents);

	return t_item;
}

bool MCWin32RawClipboardNull::IsOwned() const
{
	// This clipboard is non-functional
	return false;
}

bool MCWin32RawClipboardNull::PullUpdates()
{
	// This clipboard is non-functional
	return true;
}

bool MCWin32RawClipboardNull::PushUpdates()
{
	// This clipboard is non-functional
	return true;
}

bool MCWin32RawClipboardNull::FlushData()
{
	// This clipboard is non-functional
	return true;
}

MCWin32RawClipboardItem::MCWin32RawClipboardItem(MCWin32RawClipboardCommon* p_clipboard, IDataObject* p_object) :
  MCRawClipboardItem(),
  m_clipboard(p_clipboard),
  m_object_is_external(true),
  m_object(p_object),
  m_reps()
{
	if (m_object != nil)
		m_object->AddRef();
}

MCWin32RawClipboardItem::MCWin32RawClipboardItem(MCWin32RawClipboardCommon* p_clipboard) :
	MCRawClipboardItem(),
	m_clipboard(p_clipboard),
	m_object_is_external(false),
	m_object(nullptr),
	m_reps()
{
}

MCWin32RawClipboardItem::~MCWin32RawClipboardItem()
{
	// Release the object
	if (m_object)
		m_object->Release();
	
	// Delete all of the cached representations
	for (uindex_t i = 0; i < m_reps.Size(); i++)
		delete m_reps[i];
}


uindex_t MCWin32RawClipboardItem::GetRepresentationCount() const
{
	// Ensure all representations have been fetched
	GenerateRepresentations();

	return m_reps.Size();
}

const MCWin32RawClipboardItemRep* MCWin32RawClipboardItem::FetchRepresentationAtIndex(uindex_t p_index) const
{
	// Ensure all representations have been fetched and cached
	GenerateRepresentations();
	
	// Is the index valid?
	if (p_index >= GetRepresentationCount())
		return NULL;

	// All representations are created up front
	return m_reps[p_index];
}

bool MCWin32RawClipboardItem::AddRepresentation(MCStringRef p_type, MCDataRef p_bytes)
{
	// Get the data object for this item. This will fail if the object is from
	// an external source.
	MCWin32DataObject* t_object = GetDataObject();
	if (t_object == NULL)
		return false;

	// Mark the clipboard as dirty
	m_clipboard->SetDirty();

	// Look for an existing representation with this type
	MCWin32RawClipboardItemRep* t_rep = NULL;
	for (uindex_t i = 0; i < GetRepresentationCount(); i++)
	{
        MCAutoStringRef t_type;
        t_type.Give(m_reps[i]->CopyTypeString());
		if (t_type.IsSet() &&
            MCStringIsEqualTo(*t_type, p_type, kMCStringOptionCompareCaseless))
		{
			// This is the rep we're looking for. Update it.
			t_rep = m_reps[i];
			t_rep->m_bytes.Reset(p_bytes);
			break;
		}
	}

	// If there wasn't an existing representation, create one now
	if (t_rep == NULL)
	{
		// Extend the representation array to hold this new representation
		uindex_t t_index = m_reps.Size();
		if (!m_reps.Extend(t_index + 1))
			return false;

		// Allocate a new representation object.
		m_reps[t_index] = t_rep = new (nothrow) MCWin32RawClipboardItemRep(this, p_type, p_bytes);
	}

	// If we still have no rep, something went wrong
	if (t_rep == NULL)
		return false;

	// Ensure that the IDataObject is up-to-date too
	if (!t_object->AddRepresentation(p_type, p_bytes))
		return false;

	// If we are adding CF_TEXT, we should also set CF_LOCALE to specify that
	// we are using the Codepage 1252 text encoding (this is done by setting
	// a locale that uses this encoding; 1033 is en-US).
	if (MCWin32RawClipboardCommon::CopyAtomForType(p_type) == CF_TEXT)
	{
		// If CF_LOCALE has already been set, let it take priority
		if (!HasRepresentation(MCSTR("CF_LOCALE")))
		{
			uint32_t t_locale = 1033;
			MCAutoDataRef t_locale_bytes;
			if (!MCDataCreateWithBytes((const byte_t*)&t_locale, 4, &t_locale_bytes))
				return false;
			if (!AddRepresentation(MCSTR("CF_LOCALE"), *t_locale_bytes))
				return false;
		}
	}

	// If we are using CF_HDROP, we need to set another property to specify
	// that the file paths are to be copied rather than moved or linked.
	if (MCWin32RawClipboardCommon::CopyAtomForType(p_type) == CF_HDROP)
	{
		// If "Preferred DropEffect" has already been set, let it take priority
		if (!HasRepresentation(MCSTR("Preferred DropEffect")))
		{
			DWORD t_effect = DROPEFFECT_COPY;
			MCAutoDataRef t_effect_bytes;
			if (!MCDataCreateWithBytes((const byte_t*)&t_effect, sizeof(DWORD), &t_effect_bytes))
				return false;
			if (!AddRepresentation(MCSTR("Preferred DropEffect"), *t_effect_bytes))
				return false;
		}
	}

    // If we are adding a PNG, JPG or GIF image, then make sure we add a DIB
    // too. Windows automatically synthesizes DIB from DIBV5.
    if (MCWin32RawClipboardCommon::CopyAtomForType(p_type) == MCWin32RawClipboardCommon::CopyAtomForType(MCSTR("PNG")) ||
        MCWin32RawClipboardCommon::CopyAtomForType(p_type) == MCWin32RawClipboardCommon::CopyAtomForType(MCSTR("GIF")) ||
        MCWin32RawClipboardCommon::CopyAtomForType(p_type) == MCWin32RawClipboardCommon::CopyAtomForType(MCSTR("JFIF")))
    {
        MCAutoDataRef t_data;

	    IO_handle t_stream = 
                MCS_fakeopen((const char *)MCDataGetBytePtr(p_bytes), MCDataGetLength(p_bytes));
        if (t_stream == nullptr)
        {
            return false;
        }

        MCBitmapFrame *t_frames = nullptr;
        uindex_t t_frame_count = 0;
        if (!MCImageDecode(t_stream, t_frames, t_frame_count))
        {
            MCS_close(t_stream);
            return false;
        }

        MCS_close(t_stream);

        MCImageBitmap *t_bitmap = t_frames[0].image;
        size_t t_stride = t_bitmap->width * 4;

        size_t t_dib_size = sizeof(BITMAPV5HEADER) + t_bitmap->height * t_stride;
        void *t_dib = malloc(t_dib_size);
        if (t_dib == nullptr)
        {
            MCImageFreeFrames(t_frames, t_frame_count);
            return false;
        }
            
	    BITMAPV5HEADER *t_header = (BITMAPV5HEADER*)t_dib;
	    MCMemoryClear(t_header, sizeof(BITMAPV5HEADER));
	    t_header -> bV5Size = sizeof(BITMAPV5HEADER);
	    t_header -> bV5Width = t_bitmap->width;
	    t_header -> bV5Height = t_bitmap->height;
	    t_header -> bV5Planes = 1;
	    t_header -> bV5BitCount = 32;
	    t_header -> bV5Compression = BI_RGB;
	    t_header -> bV5SizeImage = 0;
	    t_header -> bV5XPelsPerMeter = 0;
	    t_header -> bV5YPelsPerMeter = 0;
	    t_header -> bV5ClrUsed = 0;
	    t_header -> bV5ClrImportant = 0;
	    t_header -> bV5AlphaMask = 0xFF000000;
	    t_header -> bV5RedMask =   0x00FF0000;
	    t_header -> bV5GreenMask = 0x0000FF00;
	    t_header -> bV5BlueMask =  0x000000FF;
	    t_header -> bV5CSType = LCS_WINDOWS_COLOR_SPACE;
            
	    uint8_t *t_dst_ptr = (uint8_t*)t_dib + sizeof(BITMAPV5HEADER);
	    uint8_t *t_src_ptr = (uint8_t*)t_bitmap->data + (t_bitmap->height - 1) * t_bitmap->stride;

	    for (uindex_t y = 0; y < t_bitmap->height; y++)
	    {
		    MCMemoryCopy(t_dst_ptr, t_src_ptr, t_stride);
		    t_dst_ptr += t_stride;
		    t_src_ptr -= t_bitmap->stride;
	    }
            
        MCImageFreeFrames(t_frames, t_frame_count);

        if (!MCDataCreateWithBytesAndRelease(static_cast<byte_t *>(t_dib), t_dib_size, &t_data))
        {
            free(t_dib);
            return false;
        }

        if (!AddRepresentation(MCSTR("CF_DIBV5"), *t_data))
        {
            return false;
        }
    }

    return true;
}

bool MCWin32RawClipboardItem::AddRepresentation(MCStringRef p_type, render_callback_t p_render, void* p_context)
{
	return false;
}

IDataObject* MCWin32RawClipboardItem::GetIDataObject() const
{
	return m_object;
}

MCWin32DataObject* MCWin32RawClipboardItem::GetDataObject()
{
	// Only possible if the data object belongs to LiveCode
	if (m_object_is_external)
		return NULL;

	// If the data object doesn't exist yet, create it now
	if (m_object == NULL)
		m_object = new (nothrow) MCWin32DataObject;

	return static_cast<MCWin32DataObject*> (m_object);
}

void MCWin32RawClipboardItem::GenerateRepresentations() const
{
	// This method is only necessary for externally-supplied data
	if (!m_object_is_external)
		return;

	// Do nothing if this has already been done
	if (m_reps.Size() > 0)
		return;

	// Request an enumerator for the data object
	IEnumFORMATETC* t_enum;
	if (m_object->EnumFormatEtc(DATADIR_GET, &t_enum) != S_OK)
	{
		// Failed to enumerate... pretend the clipboard is empty
		m_clipboard->Clear();
		return;
	}

	FORMATETC t_format;
	while (t_enum->Next(1, &t_format, NULL) == S_OK)
	{
		// Extend the representation array
		uindex_t t_index = m_reps.Size();
		if (!m_reps.Extend(t_index + 1))
			break;

		// Create a new representation object
		m_reps[t_index] = new (nothrow) MCWin32RawClipboardItemRep(const_cast<MCWin32RawClipboardItem*> (this), t_format);
	}

	// Release the enumerator object
	t_enum->Release();
}


MCWin32RawClipboardItemRep::MCWin32RawClipboardItemRep(MCWin32RawClipboardItem* p_parent, const FORMATETC& p_format) :
  MCRawClipboardItemRep(),
  m_item(p_parent),
  m_format(p_format),
  m_type(),
  m_bytes()
{
	;
}

MCWin32RawClipboardItemRep::MCWin32RawClipboardItemRep(MCWin32RawClipboardItem* p_parent, MCStringRef p_type, MCDataRef p_bytes) :
  MCRawClipboardItemRep(),
  m_item(p_parent),
  m_format(),
  m_type(),
  m_bytes()
{
	// Clear the format structure
	memset(&m_format, 0, sizeof(m_format));
	
	// Retain a reference to the type and data
	m_type = p_type;
	m_bytes = p_bytes;
}

MCWin32RawClipboardItemRep::~MCWin32RawClipboardItemRep()
{
	;
}


MCStringRef MCWin32RawClipboardItemRep::CopyTypeString() const
{
	// If we already have a type string, just return it
	if (*m_type != NULL)
		return MCValueRetain(*m_type);

	// Otherwise, examine the FORMATETC to generate the type string
	return MCWin32RawClipboardCommon::CopyTypeForAtom(m_format.cfFormat);
}

MCDataRef MCWin32RawClipboardItemRep::CopyData() const
{
	// If we've already fetched the data, just return it
	if (*m_bytes != NULL)
		return MCValueRetain(*m_bytes);

	// Otherwise, it will have to be fetched from the data object
	IDataObject* t_object = m_item->GetIDataObject();
	if (t_object == NULL)
		return NULL;

	// Get information on how to retrieve the data from the object
	STGMEDIUM p_medium;
	if (t_object->GetData(const_cast<FORMATETC*>(&m_format), &p_medium) != S_OK)
		return NULL;

	// What kind of storage medium is being used?
	MCDataRef t_data = NULL;
	switch (p_medium.tymed)
	{
		case TYMED_HGLOBAL:
		{
			// The data is passed via an HGLOBAL handle
			HGLOBAL t_handle = p_medium.hGlobal;

			// Lock the memory buffer
			void* t_memory = GlobalLock(t_handle);
			if (t_memory != NULL)
			{
				// Copy the data out of the buffer and release it
				MCDataCreateWithBytes((const byte_t*)t_memory, GlobalSize(t_handle), t_data);
				GlobalUnlock(t_handle);
			}

			break;
		}

		case TYMED_ISTREAM:
		{
			// The data is available via an IStream interface
			IStream* t_stream = p_medium.pstm;

			// Find the size of the data represented by the stream
			STATSTG t_statstg;
			if (t_stream->Stat(&t_statstg, STATFLAG_NONAME) == S_OK)
			{
				// Allocate memory for the data
				MCAutoArray<byte_t> t_bytes;
				if (t_statstg.cbSize.QuadPart <= UINDEX_MAX && t_bytes.Extend(t_statstg.cbSize.QuadPart))
				{
					// Loop until all the data has been read
					uindex_t t_cursor = 0;
					while (t_cursor < t_bytes.Size())
					{
						// Read the next block of data
						ULONG t_read = 0;
						HRESULT t_result = t_stream->Read(t_bytes.Ptr(), t_bytes.Size()-t_cursor, &t_read);

						// If the result is S_OK, then all data was sucessfully
						// read. If it was S_FALSE, we only had a partial read.
						// Otherwise, an error occurred.
						if (t_result == S_OK)
							break;
						else if (t_result == S_FALSE)
							t_cursor += t_read;
						else
							break;
					}
				}

				// If all data was read successfully, turn the bytes into a
				// DataRef.
				if (t_bytes.Size() == t_statstg.cbSize.QuadPart)
				{
					// Take the storage from the autoarray and hand it directly
					// to the dataref to save a reallocation of (what is likely
					// to be) a large chunk of memory.
					byte_t* t_pointer;
					uindex_t t_length;
					t_bytes.Take(t_pointer, t_length);
					if (!MCDataCreateWithBytesAndRelease(t_pointer, t_length, t_data))
						MCMemoryDelete(t_pointer);
				}
			}
			
			break;
		}

		case TYMED_FILE:
		case TYMED_ISTORAGE:
		{
			// TODO: implement
			break;
		}

		case TYMED_GDI:
		case TYMED_MFPICT:
		case TYMED_ENHMF:
		case TYMED_NULL:
		{
			// None of these types are currently supported
			break;
		}

		default:
		{
			// Unknown storage type; can't do anything with it
			break;
		}
	}

	// Release the data associated with the storage medium
	ReleaseStgMedium(&p_medium);

    // If the type was one of CF_TEXT, CF_OEMTEXT or CF_UNICODETEXT, it has
    // probably been NUL-terminated. Because we automatically add this on the
    // outgoing side, we should also remove it on the incoming side.
    if (m_format.cfFormat == CF_TEXT || m_format.cfFormat == CF_OEMTEXT || m_format.cfFormat == CF_UNICODETEXT)
    {
        // Make the data mutable
        MCDataMutableCopyAndRelease(t_data, t_data);
        
        // Trim the terminating NULL, if it exists
        uindex_t t_length = MCDataGetLength(t_data);
        if (m_format.cfFormat == CF_UNICODETEXT)
        {
            // 16-bit NUL
            if (MCDataGetByteAtIndex(t_data, t_length-1) == 0
                && MCDataGetByteAtIndex(t_data, t_length-2) == 0)
            {
                MCDataRemove(t_data, MCRangeMake(t_length-2, 2));
            }
        }
        else
        {
            // 8-bit NUL
            if (MCDataGetByteAtIndex(t_data, t_length-1) == 0)
            {
                MCDataRemove(t_data, MCRangeMake(t_length-1, 1));
            }
        }
        
        // Make the data immutable again
        MCDataCopyAndRelease(t_data, t_data);
    }
    
	// Update the data cache and return it
	m_bytes = t_data;
	return t_data;
}


MCWin32DataObject::MCWin32DataObject() :
  IDataObject(),
  MCMixinRefcounted(),
  m_types(),
  m_bytes()
{
	;
}

MCWin32DataObject::~MCWin32DataObject()
{
	;
}


bool MCWin32DataObject::AddRepresentation(MCStringRef p_type, MCDataRef p_bytes)
{
	// Is this representation already present?
	for (uindex_t i = 0; i < m_types.Count(); i++)
	{
		if (MCStringIsEqualTo(p_type, m_types[i], kMCStringOptionCompareCaseless))
		{
			// Just update the data for this representation
			MCValueRelease(m_bytes[i]);
			m_bytes[i] = MCValueRetain(p_bytes);
			return true;
		}
	}
	
	// Increase the size of the type and data arrays
	uindex_t t_index = m_types.Count();
	if (!m_types.Extend(t_index + 1) || !m_bytes.Extend(t_index + 1))
		return false;

	// Add the type and representation to the end of the array
	m_types[t_index] = MCValueRetain(p_type);
	m_bytes[t_index] = MCValueRetain(p_bytes);

	return true;
}

ULONG MCWin32DataObject::AddRef()
{
	MCMixinRefcounted::Retain();
	return 0;
}

ULONG MCWin32DataObject::Release()
{
	// Forward to the MCMixinRefcounted method
	MCMixinRefcounted::Release();
	return 0;
}

HRESULT MCWin32DataObject::QueryInterface(REFIID riid, void** ppvObject)
{
	// We support the IUnknown and IDataObject interfaces
	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDataObject))
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}

	// Interface not supported
	*ppvObject = NULL;
	return E_NOINTERFACE;
}

HRESULT MCWin32DataObject::DAdvise(FORMATETC* pformatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection)
{
	// Not supported
	return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT MCWin32DataObject::DUnadvise(DWORD dwConnection)
{
	// Not supported
	return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT MCWin32DataObject::EnumDAdvise(IEnumSTATDATA** ppenumAdvise)
{
	// Not supported
	if (ppenumAdvise != NULL)
		*ppenumAdvise = NULL;
	return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT MCWin32DataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc)
{
	// Only the 'get' direction is supported
	if (dwDirection != DATADIR_GET)
	{
		*ppenumFormatEtc = NULL;
		return E_NOTIMPL;
	}

	// Create a new enumerator object
	*ppenumFormatEtc = new (nothrow) MCWin32DataObjectFormatEnum(this, 0);
	return S_OK;
}

HRESULT MCWin32DataObject::GetCanonicalFormatEtc(FORMATETC* pformatetcIn, FORMATETC* pformatetcOut)
{
	// We don't provide device-specific data renderings
	memcpy(pformatetcOut, pformatetcIn, sizeof(FORMATETC));
	pformatetcOut->ptd = NULL;
	return DATA_S_SAMEFORMATETC;
}

HRESULT MCWin32DataObject::GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium)
{
	// Find the data ref for this format
	MCDataRef t_bytes;
	HRESULT t_result = FindDataForFormat(pformatetcIn, t_bytes);
	if (t_result != S_OK)
		return t_result;

	// Automatically add NUL-termination to text properties
	// (as required by the formats)
	UINT t_gmem_flags = GMEM_MOVEABLE;
	UINT t_extra_size = 0;
	if (pformatetcIn->cfFormat == CF_TEXT || pformatetcIn->cfFormat == CF_OEMTEXT)
	{
		t_gmem_flags |= GMEM_ZEROINIT;
		t_extra_size = 1;
	}
	else if (pformatetcIn->cfFormat == CF_UNICODETEXT)
	{
		t_gmem_flags |= GMEM_ZEROINIT;
		t_extra_size = 2;
	}

	// Allocate an HGLOBAL for this data
	HGLOBAL t_handle = GlobalAlloc(t_gmem_flags, MCDataGetLength(t_bytes) + t_extra_size);
	if (t_handle == NULL)
		return E_OUTOFMEMORY;

	// Copy the data to the memory block
	void* t_buffer = GlobalLock(t_handle);
	if (t_buffer == NULL)
	{
		GlobalFree(t_handle);
		return E_OUTOFMEMORY;
	}
	memcpy(t_buffer, MCDataGetBytePtr(t_bytes), MCDataGetLength(t_bytes));
	GlobalUnlock(t_handle);

	// Fill in the output storage medium structure.
	memset(pmedium, 0, sizeof(STGMEDIUM));
	pmedium->tymed = TYMED_HGLOBAL;
	pmedium->hGlobal = t_handle;

	return S_OK;
}

HRESULT MCWin32DataObject::GetDataHere(FORMATETC* pformatetcIn, STGMEDIUM* pmedium)
{
	// Not supported
	return DV_E_TYMED;
}

HRESULT MCWin32DataObject::QueryGetData(FORMATETC* pformatetcIn)
{
	// Can we find some data for this format?
	MCDataRef t_ignored;
	return FindDataForFormat(pformatetcIn, t_ignored);
}

HRESULT MCWin32DataObject::SetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium, BOOL fRelease)
{
	// Not supported
	return E_NOTIMPL;
}

HRESULT MCWin32DataObject::FindDataForFormat(FORMATETC* pformatetcIn, MCDataRef& r_data)
{
	// Pointer checks
	if (pformatetcIn == NULL)
		return E_INVALIDARG;
	
	// We only support HGLOBALs as a method of passing data
	if (!(pformatetcIn->tymed & TYMED_HGLOBAL))
		return DV_E_TYMED;

	// Check the other format settings
	if (pformatetcIn->lindex != -1)
		return DV_E_LINDEX;

	// Turn the requested clipboard format into a type string
	MCAutoStringRef t_type(MCWin32RawClipboardCommon::CopyTypeForAtom(pformatetcIn->cfFormat));
	if (*t_type == NULL)
		return DV_E_FORMATETC;

	// Do we have a representation with that type?
	MCDataRef t_bytes = NULL;
	for (uindex_t i = 0; i < m_types.Count(); i++)
	{
		// Is this the type we're looking for?
		if (MCStringIsEqualTo(m_types[i], *t_type, kMCStringOptionCompareCaseless))
		{
			t_bytes = m_bytes[i];
			break;
		}
	}

	// If we didn't find a representation, the requested format is invalid
	if (t_bytes == NULL)
		return DV_E_FORMATETC;

	// Found it!
	r_data = t_bytes;
	return S_OK;
}


MCWin32DataObjectFormatEnum::MCWin32DataObjectFormatEnum(MCWin32DataObject* p_object, uindex_t p_index) :
  MCMixinRefcounted(),
  m_object(p_object),
  m_index(p_index)
{
	m_object->AddRef();
}

MCWin32DataObjectFormatEnum::~MCWin32DataObjectFormatEnum()
{
	m_object->Release();
}


ULONG MCWin32DataObjectFormatEnum::AddRef()
{
	MCMixinRefcounted::Retain();
	return 0;
}

ULONG MCWin32DataObjectFormatEnum::Release()
{
	MCMixinRefcounted::Release();
	return 0;
}

HRESULT MCWin32DataObjectFormatEnum::QueryInterface(REFIID riid, void** ppvObject)
{
	// We support the IUnknown and IEnumFORMATETC interfaces
	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IEnumFORMATETC))
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}

	// Interface not supported
	*ppvObject = NULL;
	return E_NOINTERFACE;
}

HRESULT MCWin32DataObjectFormatEnum::Clone(IEnumFORMATETC** ppenum)
{
	// Check the pointer is invalid
	if (ppenum == NULL)
		return E_INVALIDARG;

	// Create a copy of this object
	*ppenum = new (nothrow) MCWin32DataObjectFormatEnum(m_object, m_index);
	return S_OK;
}

HRESULT MCWin32DataObjectFormatEnum::Next(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched)
{
	// Check the parameters for validity
	if (rgelt == NULL || (celt > 1 && pceltFetched == NULL))
		return S_FALSE;

	// Have we run out of items?
	if (m_index >= m_object->m_types.Count())
		return S_FALSE;

	// How many items can we actually copy?
	uindex_t t_remaining = m_object->m_types.Count() - m_index;
	if (t_remaining < celt)
		celt = t_remaining;

	// Copy the items
	for (uindex_t i = 0; i < celt; i++)
	{
		// Synthesize the FORMATETC structure for this representation
		FORMATETC* t_entry = &rgelt[i];
		memset(t_entry, 0, sizeof(FORMATETC));
		t_entry->cfFormat = MCWin32RawClipboardCommon::CopyAtomForType(m_object->m_types[i + m_index]);
		t_entry->dwAspect = DVASPECT_CONTENT;
		t_entry->tymed = TYMED_HGLOBAL;
	}

	// Say how many entries we copied
	if (pceltFetched != NULL)
		*pceltFetched = celt;

	// Update the index
	m_index += celt;

	// Done
	return S_OK;
}

HRESULT MCWin32DataObjectFormatEnum::Reset()
{
	// Reset to the first item
	m_index = 0;
	return S_OK;
}

HRESULT MCWin32DataObjectFormatEnum::Skip(ULONG celt)
{
	// Check how many items remain
	if (celt > m_object->m_types.Count() - m_index)
		return S_FALSE;

	// Update the index
	m_index += celt;
	return S_OK;
}
