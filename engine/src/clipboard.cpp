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


#include "clipboard.h"

#include "foundation-auto.h"
#include "imagebitmap.h"
#include "paragraf.h"
#include "object.h"
#include "stack.h"
#include "styledtext.h"
#include "globals.h"


class MCClipboard::AutoLock
{
public:
    
    inline AutoLock(const MCClipboard* p_clipboard) :
      m_clipboard(p_clipboard)
    {
        m_clipboard->Lock();
    }
    
    inline ~AutoLock()
    {
        m_clipboard->Unlock();
    }
    
private:
    
    const MCClipboard* m_clipboard;
};


MCClipboard::MCClipboard(MCRawClipboard* p_clipboard) :
  m_clipboard(p_clipboard),
  m_lock_count(0),
  m_dirty(false)
{
    ;
}

bool MCClipboard::IsOwned() const
{
    return m_clipboard->IsOwned();
}

bool MCClipboard::Lock(bool p_skip_pull) const
{
    // Increase the lock count. If it moves from zero to one, update the
    // contents of this clipboard.
    //
    // TODO: atomic operations
    if (m_lock_count++ == 0 && !p_skip_pull)
    {
        return PullUpdates();
    }
    
    return true;
}

bool MCClipboard::Unlock() const
{
    MCAssert(m_lock_count > 0);
    // Decrement the lock count. If it moves from one to zero, push any changes
    // out to the underlying OS clipboard.
    //
    // TODO: atomic operations
    if (--m_lock_count == 0)
    {
        return PushUpdates();
    }
    
    return true;
}

bool MCClipboard::IsLocked() const
{
    return m_lock_count != 0;
}

void MCClipboard::Clear()
{
    // Clear the private data
	ClearPrivateData();
    
    // Pass on the request
    Lock();
    m_clipboard->Clear();
    m_dirty = true;
    Unlock();
}

void MCClipboard::ReleaseData()
{
    // Clear any external data on the clipboard
    if (m_clipboard->IsExternalData())
        m_clipboard->Clear();
}

bool MCClipboard::PullUpdates() const
{
    // If ownership has changed, the private clipboard data needs to be cleared
	if (!this->IsDragboard() && !m_clipboard->IsOwned())
		const_cast<MCClipboard*>(this)->ClearPrivateData();
	
	// Pass on the request
    return m_clipboard->PullUpdates();
}

bool MCClipboard::PushUpdates(bool p_force) const
{
    // Pass on the request
    if (m_dirty || p_force)
    {
        const_cast<MCClipboard*>(this)->m_dirty = false;
        return m_clipboard->PushUpdates();
    }
    
    return true;
}

void MCClipboard::FlushData()
{
    // Pass on the request
    m_clipboard->FlushData();
}

MCRawClipboard* MCClipboard::GetRawClipboard()
{
    return m_clipboard;
}

const MCRawClipboard* MCClipboard::GetRawClipboard() const
{
    return m_clipboard;
}

bool MCClipboard::Rebind(MCRawClipboard* p_clipboard)
{
    // Change the underlying clipboard
    m_clipboard = p_clipboard->Retain();
    return true;
}

bool MCClipboard::IsEmpty() const
{
    return m_clipboard->GetItemCount() == 0 && !HasPrivateData();
}

bool MCClipboard::AddFileList(MCStringRef p_file_names)
{
    AutoLock t_lock(this);
    
    // Clear contents if the clipboard contains external data
    if (m_clipboard->IsExternalData())
        Clear();
    
    // Does the clipboard support multiple items?
    if (m_clipboard->SupportsMultipleItems())
    {
        // Split the file name list into individual paths
        MCAutoArrayRef t_paths;
        if (!MCStringSplit(p_file_names, MCSTR("\n"), NULL, kMCStringOptionCompareExact, &t_paths))
            return false;
        
        // For each path, add a new item to the clipboard
        for (uindex_t i = 0; i < MCArrayGetCount(*t_paths); i++)
        {
            // Get the existing item for this index or create a new one
            MCAutoRefcounted<MCRawClipboardItem> t_item;
            if ((t_item = m_clipboard->GetItemAtIndex(i)) == NULL)
            {
                // No item yet; create a new one and add it to the clipboard
                t_item = m_clipboard->CreateNewItem();
                if (t_item == NULL)
                    return false;
                if (!m_clipboard->AddItem(t_item))
                    return false;
            }
            
            // Get this path
            MCValueRef t_path;
            if (!MCArrayFetchValueAtIndex(*t_paths, index_t(i+1), t_path))
                return false;
            
            // Sanity check - should always be a string!
            if (MCValueGetTypeCode(t_path) != kMCValueTypeCodeString)
                return false;
            
            // Encode it appropriately for this clipboard
            MCAutoDataRef t_encoded_path;
            t_encoded_path.Give(m_clipboard->EncodeFileListForTransfer((MCStringRef)t_path));
			if (*t_encoded_path == NULL)
				return false;
            
            // Add a representation to this item containing the path
            if (!t_item->AddRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeFileURL), *t_encoded_path))
                return false;
            
            // The first item on the list also receives a text representation
            // containing the paths that were added (unless the user has already
            // added a text representation).
            if (i == 0 && !HasText())
                if (!AddTextToItem(t_item, p_file_names))
                    return false;
        }
    }
    else
    {
        // Convert the file list into the correct format
        MCAutoDataRef t_encoded_list;
        t_encoded_list.Give(m_clipboard->EncodeFileListForTransfer(p_file_names));
		if (*t_encoded_list == NULL)
			return false;
        
        // Get the first item on the clipboard
        MCAutoRefcounted<MCRawClipboardItem> t_item = GetItem();
        if (t_item == NULL)
            return false;
        
        // And add the representation to it
        if (!t_item->AddRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeFileURLList), *t_encoded_list))
            return false;
        
        // Add the original list as a text representation as well (unless one
		// already exists)
        if (!HasText() && !AddTextToItem(t_item, p_file_names))
            return false;
    }
    
    // Done!
    return true;
}

bool MCClipboard::AddText(MCStringRef p_string)
{
    // [Bug 19206] Converting text to styled text was causing 
    // presentation issues when attempting to paste into other
    // applications due to the HTML format being included.
    // Only add the plain text representations to the clipboard.

    AutoLock t_lock(this);
    
    // Clear contents if the clipboard contains external data
    if (m_clipboard->IsExternalData())
        Clear();
    
    // Clear contents to ensure that the clipboard does not end up
    // containing data from different copy events.
    // i.e. the HTML/RTF representation would be left untouched
    if (HasLiveCodeStyledTextOrCompatible())
    {
        m_clipboard->Clear();
    }
    
    // Get the first item on the clipboard
    MCAutoRefcounted<MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;

    return AddTextToItem(t_item, p_string);
}

bool MCClipboard::AddTextToItem(MCRawClipboardItem* p_item, MCStringRef p_string)
{
    // For each text encoding that the underlying clipboard supports, encode the
    // text and add it.
    bool t_success = true;
    MCStringRef t_type_string;
    
    if (t_success && (t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeUTF8)) != NULL)
    {
        // UTF-8
        MCAutoDataRef t_utf8;
        t_success = MCStringEncode(p_string, kMCStringEncodingUTF8, false, &t_utf8);
        if (t_success)
            t_success = p_item->AddRepresentation(t_type_string, *t_utf8);
    }
    if (t_success && (t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeUTF16)) != NULL)
    {
        // UTF-16
        MCAutoDataRef t_utf16;
        t_success = MCStringEncode(p_string, kMCStringEncodingUTF16, false, &t_utf16);
        if (t_success)
            t_success = p_item->AddRepresentation(t_type_string, *t_utf16);
    }
    if (t_success && (t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeISO8859_1)) != NULL)
    {
        // ISO-8859-1 (aka Latin1)
        MCAutoDataRef t_latin1;
        t_success = MCStringEncode(p_string, kMCStringEncodingISO8859_1, false, &t_latin1);
        if (t_success)
            t_success = p_item->AddRepresentation(t_type_string, *t_latin1);
    }
    if (t_success && (t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeMacRoman)) != NULL)
    {
        // MacRoman
        MCAutoDataRef t_macroman;
        t_success = MCStringEncode(p_string, kMCStringEncodingMacRoman, false, &t_macroman);
        if (t_success)
            t_success = p_item->AddRepresentation(t_type_string, *t_macroman);
    }
    if (t_success && (t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeCP1252)) != NULL)
    {
        // Windows codepage 1252 (Western Europe)
        MCAutoDataRef t_1252;
        t_success = MCStringEncode(p_string, kMCStringEncodingWindows1252, false, &t_1252);
        if (t_success)
            t_success = p_item->AddRepresentation(t_type_string, *t_1252);
    }
    
    return t_success;
}

bool MCClipboard::AddLiveCodeObjects(MCDataRef p_pickled_objects)
{
    AutoLock t_lock(this);
    
    // Clear contents if the clipboard contains external data
    if (m_clipboard->IsExternalData())
        Clear();
    
    // Get the first item on the clipboard
    MCAutoRefcounted<MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    // Add the objects to the clipboard under the correct type
    MCStringRef t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeLiveCodeObjects);
    if (t_type_string == NULL)
        return false;
    
    return t_item->AddRepresentation(t_type_string, p_pickled_objects);
}

bool MCClipboard::AddLiveCodeStyledText(MCDataRef p_pickled_text)
{
    AutoLock t_lock(this);
    
    // Clear contents if the clipboard contains external data
    if (m_clipboard->IsExternalData())
        Clear();
    
    // Get the first item on the clipboard
    MCAutoRefcounted<MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    // Styled text can be presented in various forms; try each of these.
    bool t_success = true;
    MCStringRef t_type_string;
    
    if (t_success && (t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeLiveCodeStyledText)) != NULL)
    {
        t_success = t_item->AddRepresentation(t_type_string, p_pickled_text);
    }
    if (t_success && (t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeRTF)) != NULL)
    {
        // This type is optional as it may not be a faithful representation
        MCAutoDataRef t_rtf;
        t_rtf.Give(ConvertStyledTextToRTF(p_pickled_text));
        if (*t_rtf != NULL)
            t_success = t_item->AddRepresentation(t_type_string, *t_rtf);
    }
    if (t_success && (t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeHTML)) != NULL)
    {
        // This type is optional as it may not be a faithful representation
        MCAutoStringRef t_html;
        t_html.Give(ConvertStyledTextToHTML(p_pickled_text));
        if (*t_html != NULL)
		{
			// All platforms accept UTF-8 as an encoding for HTML on the clipboard
			MCAutoDataRef t_html_utf8;
			t_success = MCStringEncode(*t_html, kMCStringEncodingUTF8, false, &t_html_utf8);
			
			if (t_success)
			{
				MCAutoDataRef t_encoded;
				t_encoded.Give(m_clipboard->EncodeHTMLFragmentForTransfer(*t_html_utf8));
				t_success = *t_encoded != nil;
				if (t_success)
					t_success = t_item->AddRepresentation(t_type_string, *t_encoded);
			}
		}
    }
    
    // Also attempt to add as plain text, so we have a fall-back
    if (t_success)
    {
        // This type is optional as it may not be a faithful representation
        MCAutoStringRef t_text;
        t_text.Give(ConvertStyledTextToText(p_pickled_text));
        if (*t_text != NULL)
            t_success = AddTextToItem(t_item, *t_text);
    }
    
    return t_success;
}

bool MCClipboard::AddLiveCodeStyledTextArray(MCArrayRef p_styled_text)
{
    // Convert the styled text array to serialised styled text and add that way.
    MCAutoDataRef t_pickled_text;
    t_pickled_text.Give(ConvertStyledTextArrayToStyledText(p_styled_text));
    if (*t_pickled_text == NULL)
        return false;
    
    return AddLiveCodeStyledText(*t_pickled_text);
}

bool MCClipboard::AddHTMLText(MCStringRef p_html_string)
{
    // Adding HTML to the clipboard is done by converting the HTML to LiveCode's
    // internal styled-text format and then adding it to the clipboard in that
    // format.
    //
    // This is a lossy process but preserves legacy behaviour.
    MCAutoDataRef t_pickled_text;
    t_pickled_text.Give(ConvertHTMLToStyledText(p_html_string));
    if (*t_pickled_text == NULL)
        return false;
    
    return AddLiveCodeStyledText(*t_pickled_text);
}

bool MCClipboard::AddRTFText(MCDataRef p_rtf_data)
{
    // Adding RTF to the clipboard is done by converting the RTF to LiveCode's
    // internal styled-text format and then adding it to the clipboard in that
    // format.
    //
    // This is a lossy process but preserves legacy behaviour.
    MCAutoDataRef t_pickled_text;
    t_pickled_text.Give(ConvertRTFToStyledText(p_rtf_data));
    if (*t_pickled_text == NULL)
        return false;
    
    return AddLiveCodeStyledText(*t_pickled_text);
}

bool MCClipboard::AddRTF(MCDataRef p_rtf)
{
    AutoLock t_lock(this);
    
    // Clear the contents if the clipboard contains external data
    if (m_clipboard->IsExternalData())
        Clear();
    
    // Get the first item on the clipboard
    MCAutoRefcounted<MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    // Add the data to the clipboard with the correct type
    MCStringRef t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeRTF);
    if (t_type_string == NULL)
        return false;
    
    return t_item->AddRepresentation(t_type_string, p_rtf);
}

bool MCClipboard::AddHTML(MCStringRef p_html)
{
    AutoLock t_lock(this);
    
	// All platforms support UTF-8 as a text encoding for HTML on the clipboard
	MCAutoDataRef t_html_utf8;
	if (!MCStringEncode(p_html, kMCStringEncodingUTF8, false, &t_html_utf8))
		return false;

    // Clear the contents if the clipboard contains external data
    if (m_clipboard->IsExternalData())
        Clear();
    
    // Get the first item on the clipboard
    MCAutoRefcounted<MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;

	// Encode the HTML in the required format for the clipboard
	MCAutoDataRef t_encoded;
	t_encoded.Give(m_clipboard->EncodeHTMLFragmentForTransfer(*t_html_utf8));
	if (*t_encoded == nil)
		return false;

    // Add the data to the clipboard with the correct type
    MCStringRef t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeHTML);
    if (t_type_string == NULL)
        return false;
    
    return t_item->AddRepresentation(t_type_string, *t_encoded);
}

bool MCClipboard::AddPNG(MCDataRef p_png)
{
    AutoLock t_lock(this);
    
    // Clear contents if the clipboard contains external data
    if (m_clipboard->IsExternalData())
        Clear();
    
    // Get the first item on the clipboard
    MCAutoRefcounted<MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    // Add the objects to the clipboard under the correct type
    MCStringRef t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypePNG);
    if (t_type_string == NULL)
        return false;
    
    return t_item->AddRepresentation(t_type_string, p_png);
}

bool MCClipboard::AddGIF(MCDataRef p_gif)
{
    AutoLock t_lock(this);
    
    // Clear contents if the clipboard contains external data
    if (m_clipboard->IsExternalData())
        Clear();
    
    // Get the first item on the clipboard
    MCAutoRefcounted<MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    // Add the objects to the clipboard under the correct type
    MCStringRef t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeGIF);
    if (t_type_string == NULL)
        return false;
    
    return t_item->AddRepresentation(t_type_string, p_gif);
}

bool MCClipboard::AddJPEG(MCDataRef p_jpeg)
{
    AutoLock t_lock(this);
    
    // Clear contents if the clipboard contains external data
    if (m_clipboard->IsExternalData())
        Clear();
    
    // Get the first item on the clipboard
    MCAutoRefcounted<MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    // Add the objects to the clipboard under the correct type
    MCStringRef t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeJPEG);
    if (t_type_string == NULL)
        return false;
    
    return t_item->AddRepresentation(t_type_string, p_jpeg);
}

bool MCClipboard::AddBMP(MCDataRef p_bmp)
{
	AutoLock t_lock(this);

	// Clear contents if the clipboard contains external data
	if (m_clipboard->IsExternalData())
		Clear();

	// Get the first item on the clipboard
	MCAutoRefcounted<MCRawClipboardItem> t_item = GetItem();
	if (t_item == NULL)
		return false;

	// Add the objects to the clipboard under the correct type
	MCStringRef t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeWinDIBv5);
	if (t_type_string == NULL)
		return false;

	// Encode the BMP for transfer
	MCAutoDataRef t_bmp(m_clipboard->EncodeBMPForTransfer(p_bmp));

	return t_item->AddRepresentation(t_type_string, *t_bmp);
}

bool MCClipboard::AddWinMetafile(MCDataRef p_wmf)
{
	AutoLock t_lock(this);

	// Clear contents if the clipboard contains external data
	if (m_clipboard->IsExternalData())
		Clear();

	// Get the first item on the clipboard
	MCAutoRefcounted<MCRawClipboardItem> t_item = GetItem();
	if (t_item == NULL)
		return false;

	// Add the objects to the clipboard under the correct type
	MCStringRef t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeWinMF);
	if (t_type_string == NULL)
		return false;

	return t_item->AddRepresentation(t_type_string, p_wmf);
}

bool MCClipboard::AddWinEnhMetafile(MCDataRef p_emf)
{
	AutoLock t_lock(this);

	// Clear contents if the clipboard contains external data
	if (m_clipboard->IsExternalData())
		Clear();

	// Get the first item on the clipboard
	MCAutoRefcounted<MCRawClipboardItem> t_item = GetItem();
	if (t_item == NULL)
		return false;

	// Add the objects to the clipboard under the correct type
	MCStringRef t_type_string = m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeWinEMF);
	if (t_type_string == NULL)
		return false;

	return t_item->AddRepresentation(t_type_string, p_emf);
}

bool MCClipboard::AddImage(MCDataRef p_image_data)
{
    // Examine the data to see if it matches any of the formats that we handle
    if (MCImageDataIsPNG(p_image_data))
        return AddPNG(p_image_data);
    if (MCImageDataIsGIF(p_image_data))
        return AddGIF(p_image_data);
    if (MCImageDataIsJPEG(p_image_data))
        return AddJPEG(p_image_data);
	if (MCImageDataIsBMP(p_image_data))
		return AddBMP(p_image_data);
    
    return false;
}

bool MCClipboard::AddPrivateData(MCDataRef p_private_data)
{
    // Clear contents if the clipboard contains external data
    if (m_clipboard->IsExternalData())
        Clear();
    
    // Update the stored private data
    m_private_data.Reset(p_private_data);
    
    // Ensure that an item is always on the clipboard, even if it is empty
    if (m_clipboard->GetItemCount() == 0)
    {
        MCAutoRefcounted<MCRawClipboardItem> t_item = m_clipboard->CreateNewItem();
        m_clipboard->AddItem(t_item);
    }
    
    return true;
}

bool MCClipboard::HasFileList() const
{
    // A list of files can be returned as either multiple file URL items or a
    // single file URL list item.
    MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    if (t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeFileURL)))
        return true;
    if (t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeFileURLList)))
        return true;
    
    return false;
}

bool MCClipboard::HasText() const
{
    AutoLock t_lock(this);
    
    // The clipboard has text if any of the known formats are present
    MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    if (t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeUTF8)))
        return true;
    if (t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeUTF16)))
        return true;
    if (t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeISO8859_1)))
        return true;
    if (t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeMacRoman)))
        return true;
    if (t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeCP1252)))
        return true;
    
    return false;
}

bool MCClipboard::HasLiveCodeObjects() const
{
    AutoLock t_lock(this);
    
    // Check for the corresponding type
    MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    return t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeLiveCodeObjects));
}

bool MCClipboard::HasLiveCodeStyledText() const
{
    AutoLock t_lock(this);
    
    // Check for the corresponding type
    MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    return t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeLiveCodeStyledText));
}

bool MCClipboard::HasRTF() const
{
    AutoLock t_lock(this);
    
    // Check for the corresponding type
    MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    return t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeRTF));
}

bool MCClipboard::HasHTML() const
{
    AutoLock t_lock(this);
    
    // Check for the corresponding type
    MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    return t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeHTML));
}

bool MCClipboard::HasPNG() const
{
    AutoLock t_lock(this);
    
    // Check for the corresponding type
    MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
	
#if defined(__MAC__) && !defined(MODE_SERVER)
	// Many Mac apps (like Preview) present things as TIFF on the clipboard. Since
	// the engine doesn't currently understand TIFF generally we make TIFF data
	// masquerade as PNG.
	if (t_item -> HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeTIFF)))
		return true;
#endif
	
    return t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypePNG));
}

bool MCClipboard::HasGIF() const
{
    AutoLock t_lock(this);
    
    // Check for the corresponding type
    MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    return t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeGIF));
}

bool MCClipboard::HasJPEG() const
{
    AutoLock t_lock(this);
    
    // Check for the corresponding type
    MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    return t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeJPEG));
}

bool MCClipboard::HasBMP() const
{
	AutoLock t_lock(this);

	// Check for the corresponding type
	MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
	if (t_item == NULL)
		return false;

	return t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeWinDIB))
		|| t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeWinDIBv5));
}

bool MCClipboard::HasWinMetafile() const
{
	AutoLock t_lock(this);

	// Check for the corresponding type
	MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
	if (t_item == NULL)
		return false;

	return t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeWinMF));
}

bool MCClipboard::HasWinEnhMetafile() const
{
	AutoLock t_lock(this);

	// Check for the corresponding type
	MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
	if (t_item == NULL)
		return false;

	return t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeWinEMF));
}

bool MCClipboard::HasImage() const
{
    // Images are any of PNG, GIF or JPEG
    AutoLock t_lock(this);
	return HasPNG() || HasGIF() || HasJPEG() || HasBMP();
}

bool MCClipboard::HasTextOrCompatible() const
{
    // Styled text is compatible with plain text. A list of file paths can also
    // be auto-converted into text.
    AutoLock t_lock(this);
    return HasText() || HasLiveCodeStyledTextOrCompatible() || HasFileList();
}

bool MCClipboard::HasLiveCodeStyledTextOrCompatible() const
{
    // Note that plain text is *not* up-converted to styled text
    AutoLock t_lock(this);
    return HasLiveCodeStyledText() || HasRTF() || HasHTML();
}

bool MCClipboard::HasPrivateData() const
{
    return m_private_data.IsSet();
}

bool MCClipboard::CopyAsFileList(MCStringRef& r_file_list) const
{
    AutoLock t_lock(this);
    
    // String containing a newline-separated list of file paths
    MCStringRef t_output = NULL;
    
    // If the clipboard supports multiple items, look for multiple "file URL"
    // representations.
    if (m_clipboard->SupportsMultipleItems())
    {
        // Accumulate the items into a list, separated by newlines
        MCAutoListRef t_list;
        if (!MCListCreateMutable('\n', &t_list))
            return false;
        
        for (uindex_t i = 0; i < m_clipboard->GetItemCount(); i++)
        {
            // Get this item and extract the file URL representation from it
            const MCRawClipboardItemRep* t_rep = NULL;
            MCAutoRefcounted<const MCRawClipboardItem> t_item = m_clipboard->GetItemAtIndex(i);
            if (t_item == NULL)
                return false;
            
            t_rep = t_item->FetchRepresentationByType(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeFileURL));
            if (t_rep == NULL)
                return false;
            
            // Get the data for this representation and decode it
            MCAutoStringRef t_url;
            MCAutoDataRef t_encoded_url;
            t_encoded_url.Give(t_rep->CopyData());
			if (*t_encoded_url == NULL)
				return false;
            t_url.Give(m_clipboard->DecodeTransferredFileList(*t_encoded_url));
			if (*t_url == NULL)
				return false;
            
            // Append it to the list
            if (!MCListAppend(*t_list, *t_url))
                return false;
        }
        
        // Convert the list to a string
        if (!MCListCopyAsString(*t_list, t_output))
            return false;
    }
    else
    {
        // Clipboard only supports a single item. Grab the contents of the file
        // URL list representation and massage slightly.
        MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
        if (t_item == NULL)
            return false;
        
        const MCRawClipboardItemRep* t_rep = t_item->FetchRepresentationByType(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeFileURLList));
        if (t_rep == NULL)
            return false;
        
		// Decode the list of files
        MCAutoDataRef t_data;
        t_data.Give(t_rep->CopyData());
		if (*t_data == NULL)
			return false;
		t_output = m_clipboard->DecodeTransferredFileList(*t_data);
    }
    
    // All done
    if (t_output != NULL)
        r_file_list = t_output;
    
    return t_output != NULL;
}

bool MCClipboard::CopyAsText(MCStringRef& r_text) const
{
    AutoLock t_lock(this);
    
    // Try to fetch the data using any of the supported text types
    MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
	// IM-2016-11-21: [[ Bug 18652 ]] If we should be able to fetch using a text encoding then return
	//    false if conversion fails instead of falling through to other encodings.
	if (t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeUTF8)))
		return CopyAsEncodedText(t_item, kMCRawClipboardKnownTypeUTF8, kMCStringEncodingUTF8, r_text);
	if (t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeUTF16)))
		return CopyAsEncodedText(t_item, kMCRawClipboardKnownTypeUTF16, kMCStringEncodingUTF16, r_text);
	if (t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeISO8859_1)))
		return CopyAsEncodedText(t_item, kMCRawClipboardKnownTypeISO8859_1, kMCStringEncodingISO8859_1, r_text);
	if (t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeMacRoman)))
		return CopyAsEncodedText(t_item, kMCRawClipboardKnownTypeMacRoman, kMCStringEncodingMacRoman, r_text);
	if (t_item->HasRepresentation(m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeCP1252)))
		return CopyAsEncodedText(t_item, kMCRawClipboardKnownTypeCP1252, kMCStringEncodingWindows1252, r_text);
    
    // As a fallback, try to convert a list of file paths into text
    if (CopyAsFileList(r_text))
        return true;
    
    // None of the text representations existed
    return false;
}

bool MCClipboard::CopyAsLiveCodeObjects(MCDataRef& r_objects) const
{
    return CopyAsData(kMCRawClipboardKnownTypeLiveCodeObjects, r_objects);
}

bool MCClipboard::CopyAsLiveCodeStyledText(MCDataRef& r_pickled_text) const
{
    AutoLock t_lock(this);
    
    // First, try to get the data directly as styled text
    if (CopyAsData(kMCRawClipboardKnownTypeLiveCodeStyledText, r_pickled_text))
        return true;
    
    // No LiveCode styled text on the clipboard; try RTF
    MCAutoDataRef t_rtf;
    if (CopyAsData(kMCRawClipboardKnownTypeRTF, &t_rtf))
    {
        // Convert to LiveCode styled text
        MCDataRef t_pickled_text = ConvertRTFToStyledText(*t_rtf);
        if (t_pickled_text != NULL)
        {
            r_pickled_text = t_pickled_text;
            return true;
        }
    }
    
    // Could not grab as either styled text or RTF. Try with HTML.
    MCAutoStringRef t_html;
    if (CopyAsHTML(&t_html))
    {
		// Convert to LiveCode styled text
		MCDataRef t_pickled_text = ConvertHTMLToStyledText(*t_html);
		if (t_pickled_text != NULL)
		{
			r_pickled_text = t_pickled_text;
			return true;
		}
    }
    
    // Finally, try plain text.
    MCAutoStringRef t_plain_text;
    if (CopyAsText(&t_plain_text))
    {
        // Convert to LiveCode styled text
        MCDataRef t_pickled_text = ConvertTextToStyledText(*t_plain_text);
        if (t_pickled_text != NULL)
        {
            r_pickled_text = t_pickled_text;
            return true;
        }
    }
    
    // There was nothing on the clipboard that could be converted to LiveCode
    // styled text.
    return false;
}

bool MCClipboard::CopyAsLiveCodeStyledTextArray(MCArrayRef& r_styles) const
{
    // Get the clipboard contents as serialised LiveCode styled text
    MCAutoDataRef t_serialised;
    if (!CopyAsLiveCodeStyledText(&t_serialised))
        return false;
    
    // Convert the serialised form to array form
    MCArrayRef t_styles = ConvertStyledTextToStyledTextArray(*t_serialised);
    if (t_styles == NULL)
        return false;
    
    r_styles = t_styles;
    return true;
}

bool MCClipboard::CopyAsRTFText(MCDataRef& r_rtf) const
{
    AutoLock t_lock(this);
    
    // Grab the data as LiveCode styled text then convert to RTF. This ensures
    // that the returned RTF is usable by the LiveCode field object.
    MCAutoDataRef t_pickled_text;
    if (!CopyAsLiveCodeStyledText(&t_pickled_text))
        return false;
    
    MCDataRef t_rtf = ConvertStyledTextToRTF(*t_pickled_text);
    if (t_rtf == NULL)
        return false;
    
    r_rtf = t_rtf;
    return true;
}

bool MCClipboard::CopyAsHTMLText(MCStringRef& r_html) const
{
    AutoLock t_lock(this);
    
    // Grab the data as LiveCode styled text then convert to HTML. This ensures
    // that the returned HTML is usable by the LiveCode field object.
    MCAutoDataRef t_pickled_text;
    if (!CopyAsLiveCodeStyledText(&t_pickled_text))
        return false;
    
	MCStringRef t_html = ConvertStyledTextToHTML(*t_pickled_text);
    if (t_html == nil)
        return false;
    
    r_html = t_html;
    return true;
}

bool MCClipboard::CopyAsRTF(MCDataRef& r_rtf_data) const
{
    return CopyAsData(kMCRawClipboardKnownTypeRTF, r_rtf_data);
}

bool MCClipboard::CopyAsHTML(MCStringRef& r_html) const
{
	MCAutoDataRef t_data;
	if (!CopyAsData(kMCRawClipboardKnownTypeHTML, &t_data))
		return false;

	MCAutoDataRef t_decoded;
	t_decoded.Give(m_clipboard->DecodeTransferredHTML(*t_data));
	if (*t_decoded == nil)
		return false;

	// Convert the decoded HTML data into a string
	MCStringEncoding t_encoding = GuessHTMLEncoding(*t_decoded);
	MCAutoStringRef t_html_string;
	if (!MCStringDecode(*t_decoded, t_encoding, false, &t_html_string))
		return false;

	r_html = t_html_string.Take();
	return true;
}

bool MCClipboard::CopyAsPNG(MCDataRef& r_png) const
{
    if (CopyAsData(kMCRawClipboardKnownTypePNG, r_png))
		return true;
	
#if defined(__MAC__) && !defined(MODE_SERVER)
	MCAutoDataRef t_tiff_data;
	if (!CopyAsData(kMCRawClipboardKnownTypeTIFF, &t_tiff_data))
		return false;
	
	extern bool MCMacPasteboardConvertTIFFToPNG(MCDataRef p_in_data, MCDataRef& r_out_data);
	if (!MCMacPasteboardConvertTIFFToPNG(*t_tiff_data, r_png))
		return false;
	
	return true;
#else
	return false;
#endif
}

bool MCClipboard::CopyAsGIF(MCDataRef& r_gif) const
{
    return CopyAsData(kMCRawClipboardKnownTypeGIF, r_gif);
}

bool MCClipboard::CopyAsJPEG(MCDataRef& r_jpeg) const
{
    return CopyAsData(kMCRawClipboardKnownTypeJPEG, r_jpeg);
}

bool MCClipboard::CopyAsBMP(MCDataRef& r_bmp) const
{
	// Copy and decode the BMP file
	MCAutoDataRef t_bmp;
	if (!CopyAsData(kMCRawClipboardKnownTypeWinDIBv5, &t_bmp))
    {
        if (!CopyAsData(kMCRawClipboardKnownTypeWinDIB, &t_bmp))
        {
		    return false;
	    }
    }

	MCDataRef t_decoded = m_clipboard->DecodeTransferredBMP(*t_bmp);
	if (t_decoded == nil)
		return false;

	r_bmp = t_decoded;
	return true;
}

bool MCClipboard::CopyAsWinMetafile(MCDataRef& r_wmf) const
{
	return CopyAsData(kMCRawClipboardKnownTypeWinMF, r_wmf);
}

bool MCClipboard::CopyAsWinEnhMetafile(MCDataRef& r_emf) const
{
	return CopyAsData(kMCRawClipboardKnownTypeWinEMF, r_emf);
}

bool MCClipboard::CopyAsImage(MCDataRef& r_image) const
{
    // The order here is fairly arbitrary
    AutoLock t_lock(this);
    return CopyAsPNG(r_image) || CopyAsJPEG(r_image) || CopyAsGIF(r_image) || CopyAsBMP(r_image);
}

bool MCClipboard::CopyAsPrivateData(MCDataRef& r_data) const
{
    if (!m_private_data.IsSet())
        return false;
    
    r_data = MCValueRetain(*m_private_data);
    return true;
}

void MCClipboard::ClearPrivateData()
{
    m_private_data.Reset();
}

// Wrapper for MCStringIsEqualTo that handles NULL parameters
static bool MCTypeMatches(MCStringRef p_one, MCStringRef p_two)
{
    if (p_one == NULL || p_two == NULL)
        return false;
    return MCStringIsEqualTo(p_one, p_two, kMCStringOptionCompareExact);
}

int MCClipboard::GetLegacyOrdering() const
{
    AutoLock t_lock(this);
    
    // Grab the first item on the clipboard - for backwards compatibility, only
    // this first item is examined. No item means neither type is present.
    MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return 0;
    
    // Scan through the list of representations
    for (uindex_t i = 0; i < t_item->GetRepresentationCount(); i++)
    {
        // Get the representation
        const MCRawClipboardItemRep* t_rep = t_item->FetchRepresentationAtIndex(i);
        
        // Is this an image representation?
        MCAutoStringRef t_type;
        t_type.Give(t_rep->CopyTypeString());
        if (MCTypeMatches(*t_type, m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypePNG))
            || MCTypeMatches(*t_type, m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeGIF))
            || MCTypeMatches(*t_type, m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeJPEG)))
            return 1;
        
        // Is this a text (or text-like) representation?
        if (MCTypeMatches(*t_type, m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeUTF8))
            || MCTypeMatches(*t_type, m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeUTF16))
            || MCTypeMatches(*t_type, m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeISO8859_1))
            || MCTypeMatches(*t_type, m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeMacRoman))
            || MCTypeMatches(*t_type, m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeISO8859_1))
            || MCTypeMatches(*t_type, m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeCP1252))
            || MCTypeMatches(*t_type, m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeHTML))
            || MCTypeMatches(*t_type, m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeRTF))
            || MCTypeMatches(*t_type, m_clipboard->GetKnownTypeString(kMCRawClipboardKnownTypeLiveCodeStyledText)))
            return -1;
    }
    
    // Neither an image nor a text type was found
    return 0;
}

MCParagraph* MCClipboard::CopyAsParagraphs(MCField* p_via_field) const
{
    AutoLock t_lock(this);
    
    // Copy the data as styled text, first
    MCAutoDataRef t_pickled_text;
    if (CopyAsLiveCodeStyledText(&t_pickled_text))
    {
        // Turn the pickled text into a StyledText object
	    MCAutoPointer<MCObject> t_object = MCObject::unpickle(*t_pickled_text, p_via_field -> getstack());
        if (!t_object)
            return NULL;
        
        // And from that, get the paragraph structures that the field can deal with
        MCParagraph *t_paragraphs;
        t_paragraphs = (static_cast<MCStyledText*>(t_object.Get()))->grabparagraphs(p_via_field);

        return t_paragraphs;
    }
    
    // There was no styled text - use plain text instead
    MCAutoStringRef t_text;
    if (CopyAsText(&t_text))
    {
        return p_via_field->texttoparagraphs(*t_text);
    }
    
    // There was nothing that could convert to paragraphs
    return NULL;
}

MCRawClipboardItem* MCClipboard::GetItem()
{
    // Can't do anything without an underlying clipboard
    if (m_clipboard == NULL)
        return NULL;
    
    // Attempt to get a pre-existing item from the clipboard
    MCAutoRefcounted<MCRawClipboardItem> t_item = m_clipboard->GetItemAtIndex(0);
    
    // If there is not already an item, create it now
    if (t_item == NULL)
    {
        t_item = m_clipboard->CreateNewItem();
        if (t_item == NULL || !m_clipboard->AddItem(t_item))
            return NULL;
    }
    
    // The item we returned is going to be modified so mark the clipboard as
    // being dirty.
    m_dirty = true;
    
    return t_item->Retain();
}

const MCRawClipboardItem* MCClipboard::GetItem() const
{
    // Can't do anything without an underlying clipboard
    if (m_clipboard == NULL)
        return NULL;
    
    // Only pre-existing clipboard items will be returned
    MCAutoRefcounted<const MCRawClipboardItem> t_item = m_clipboard->GetItemAtIndex(0);
    if (t_item == NULL)
        return NULL;
    
    return t_item->Retain();
}

MCStringRef MCClipboard::ConvertStyledTextToText(MCDataRef p_pickled_text)
{
    // Turn the pickled text into a StyledText object
	MCAutoPointer<MCObject> t_object = MCObject::unpickle(p_pickled_text, MCtemplatefield -> getstack());
    if (!t_object)
        return NULL;
    
    // And from that, get the paragraph structures that the field can deal with
    MCParagraph *t_paragraphs;
    t_paragraphs = (static_cast<MCStyledText*>(t_object.Get()))->getparagraphs();
    if (t_paragraphs == NULL)
        return NULL;
    
    // Export the field contents as plain text
    MCAutoStringRef t_text;
    if (!MCtemplatefield->exportasplaintext(t_paragraphs, 0, INT32_MAX,
                                            &t_text))
        return NULL;

    return t_text.Take();
}

MCStringRef MCClipboard::ConvertStyledTextToHTML(MCDataRef p_pickled_text)
{
    // Turn the pickled text into a StyledText object
    MCAutoPointer<MCObject> t_object =
        MCObject::unpickle(p_pickled_text, MCtemplatefield -> getstack());
    if (!t_object)
        return NULL;
    
    // And from that, get the paragraph structures that the field can deal with
    MCParagraph *t_paragraphs;
    t_paragraphs = (static_cast<MCStyledText*>(t_object.Get()))->getparagraphs();
    if (t_paragraphs == NULL)
        return NULL;
    
    // Export the field contents as HTML
    MCAutoDataRef t_html;
    if (!MCtemplatefield->exportashtmltext(t_paragraphs, 0, INT32_MAX,
                                           false, &t_html))
        return NULL;

	// Convert the HTML into a string
	// exportashtmltext always returns native-encoded data (where non-ASCII chars are entity-encoded)
	MCAutoStringRef t_html_string;
	if (!MCStringDecode(*t_html, kMCStringEncodingNative, false, &t_html_string))
		return nil;

	return t_html_string.Take();
}

MCDataRef MCClipboard::ConvertStyledTextToRTF(MCDataRef p_pickled_text)
{
    // Turn the pickled text into a StyledText object
    MCAutoPointer<MCObject> t_object =
        MCObject::unpickle(p_pickled_text, MCtemplatefield -> getstack());
    if (!t_object)
        return NULL;
    
    // And from that, get the paragraph structures that the field can deal with
    MCParagraph *t_paragraphs;
    t_paragraphs = (static_cast<MCStyledText*>(t_object.Get()))->getparagraphs();
    if (t_paragraphs == NULL)
        return NULL;
    
    // Export the field contents as RTF
    MCAutoStringRef t_text;
    if (!MCtemplatefield->exportasrtftext(t_paragraphs, 0, INT32_MAX, &t_text))
        return NULL;
    
    // The RTF format description specifies that only 7-bit ASCII characters are
    // valid. However, it can contain bytes with the high bit set if it contains
    // binary data inline. Therefore, the platform's native encoding needs to
    // be used here, to preserve those bytes.
    MCAutoDataRef t_rtf;
    if (!MCStringEncode(*t_text, kMCStringEncodingNative, false, &t_rtf))
        return NULL;
    
    return t_rtf.Take();
}

MCDataRef MCClipboard::ConvertRTFToStyledText(MCDataRef p_rtf_data)
{
    // Treat the RTF as a string in the native encoding (it really should be
    // 7-bit ASCII but it may include inline binary data with the high bit set).
    MCAutoStringRef t_rtf_string;
    if (!MCStringDecode(p_rtf_data, kMCStringEncodingNative, false, &t_rtf_string))
        return NULL;
    
    // Turn the RTF into paragraphs
    MCParagraph *t_paragraphs = MCtemplatefield->rtftoparagraphs(*t_rtf_string);
    if (t_paragraphs == NULL)
        return NULL;
    
    // Then, turn these paragraphs into a styled text object
    MCStyledText t_styled_text;
    t_styled_text.setparent(MCtemplatefield -> getparent());
    t_styled_text.setparagraphs(t_paragraphs);
    
    // Finally, serialise the styled text
    MCDataRef t_pickled_text;
    MCObject::pickle(&t_styled_text, 0, t_pickled_text);
    
    return t_pickled_text;
}

MCDataRef MCClipboard::ConvertHTMLToStyledText(MCStringRef p_html_string)
{
    // Turn the HTML into paragraphs
    MCParagraph *t_paragraphs = MCtemplatefield->importhtmltext(p_html_string);
    if (t_paragraphs == NULL)
        return NULL;
    
    // Then, turn these paragraphs into a styled text object
    MCStyledText t_styled_text;
    t_styled_text.setparent(MCtemplatefield -> getparent());
    t_styled_text.setparagraphs(t_paragraphs);
    
    // Finally, serialise the styled text
    MCDataRef t_pickled_text;
    MCObject::pickle(&t_styled_text, 0, t_pickled_text);
    
    return t_pickled_text;
}

MCArrayRef MCClipboard::ConvertStyledTextToStyledTextArray(MCDataRef p_pickled_text)
{
    // Turn the pickled text into a StyledText object
    MCAutoPointer<MCObject> t_object =
        MCObject::unpickle(p_pickled_text, MCtemplatefield -> getstack());
    if (!t_object)
        return NULL;
    
    // Grab the paragraphs from the object and turn them into a text styles
    // array.
    MCParagraph* t_paragraphs;
    MCAutoArrayRef t_style_array;
    t_paragraphs = (static_cast<MCStyledText*>(t_object.Get()))->getparagraphs();
    if (t_paragraphs != NULL)
        MCtemplatefield->exportasstyledtext(t_paragraphs, 0, INT32_MAX, false, false, &t_style_array);
    
    // If generating the array failed, return NULL
    if (*t_style_array == NULL)
        return NULL;
    
    // Return the styled text array
    return t_style_array.Take();
}

MCDataRef MCClipboard::ConvertStyledTextArrayToStyledText(MCArrayRef p_styles)
{
    // Convert the styles array to paragraphs
    MCParagraph *t_paragraphs;
    t_paragraphs = MCtemplatefield -> styledtexttoparagraphs(p_styles);
    if (t_paragraphs == NULL)
        return NULL;
    
    // Create a new styled text object and give it these paragraphs
    MCStyledText t_styled_text;
    t_styled_text . setparent(MCdefaultstackptr);
    t_styled_text . setparagraphs(t_paragraphs);
    
    // Serialise the styled text
    MCDataRef t_pickled_text;
    MCObject::pickle(&t_styled_text, 0, t_pickled_text);
 
    return t_pickled_text;
}

MCDataRef MCClipboard::ConvertTextToStyledText(MCStringRef p_text)
{
    // Convert the plain text to a series of paragraph structures
    MCParagraph *t_paragraphs;
    t_paragraphs = MCtemplatefield -> texttoparagraphs(p_text);
    
    // Create a StyledText object and give it the paragraphs
    MCStyledText t_styled_text;
    t_styled_text . setparagraphs(t_paragraphs);
    
    // Serialise the styled text
    MCDataRef t_serialised = NULL;
    MCObject::pickle(&t_styled_text, 0, t_serialised);
    
    return t_serialised;
}

bool MCClipboard::CopyAsEncodedText(const MCRawClipboardItem* p_item, MCRawClipboardKnownType p_type, MCStringEncoding p_encoding, MCStringRef& r_text) const
{
    AutoLock t_lock(this);
    
    // Find the representation for this type
    const MCRawClipboardItemRep* t_rep = p_item->FetchRepresentationByType(m_clipboard->GetKnownTypeString(p_type));
    if (t_rep == NULL)
        return false;
    
    // Get the data for this representation and decode it
    MCAutoDataRef t_encoded;
    t_encoded.Give(t_rep->CopyData());

	// IM-2016-11-21: [[ Bug 18652 ]] Check for null return from CopyData()
	if (!t_encoded.IsSet())
		return false;
    
    MCAutoStringRef t_string;
    if (!MCStringDecode(*t_encoded, p_encoding, false, &t_string))
        return false;
    
    return MCStringNormalizeLineEndings(*t_string, 
                                        kMCStringLineEndingStyleLF, 
                                        kMCStringLineEndingOptionNormalizePSToLineEnding |
                                        kMCStringLineEndingOptionNormalizeLSToVT, 
                                        r_text, 
                                        nullptr);
}

bool MCClipboard::CopyAsData(MCRawClipboardKnownType p_type, MCDataRef& r_data) const
{
    AutoLock t_lock(this);
    
    // Find the representation for this type
    MCAutoRefcounted<const MCRawClipboardItem> t_item = GetItem();
    if (t_item == NULL)
        return false;
    
    // Find the representation for this type
    const MCRawClipboardItemRep* t_rep = t_item->FetchRepresentationByType(m_clipboard->GetKnownTypeString(p_type));
    if (t_rep == NULL)
        return false;
    
    // Finally, get the data for this representation
    MCDataRef t_data = t_rep->CopyData();
    if (t_data == NULL)
        return false;
    
    r_data = t_data;
    return true;
}

MCClipboard* MCClipboard::CreateSystemClipboard()
{
    return new MCClipboard(MCRawClipboard::CreateSystemClipboard());
}

MCClipboard* MCClipboard::CreateSystemSelectionClipboard()
{
    return new MCClipboard(MCRawClipboard::CreateSystemSelectionClipboard());
}

MCClipboard* MCClipboard::CreateSystemDragboard()
{
    // Drag boards are created in a locked state
    MCClipboard* t_dragboard = new (nothrow) MCClipboard(MCRawClipboard::CreateSystemDragboard());
    t_dragboard->Lock(true);
    t_dragboard->Clear();
    return t_dragboard;
}

MCStringEncoding MCClipboard::GuessHTMLEncoding(MCDataRef p_html_data)
{
	// All of our platforms (seem to) use UTF-8 as the text encoding for HTML
	// data transferred via the clipboard. Windows guarantees this; for Mac and
	// Linux the situation is unclear (but various places in the engine seem to
	// assume this anyway).
	//
	// If it turns out that non-UTF-8 HTML gets passed via the clipboard, this
	// function should be updated to detect it if possible.

	return kMCStringEncodingUTF8;
}

bool MCClipboard::IsDragboard() const
{
	return this == MCdragboard;
}
