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



#include "em-clipboard.h"


MCRawClipboard* MCRawClipboard::CreateSystemClipboard()
{
    return new MCEmscriptenRawClipboard;
}

MCRawClipboard* MCRawClipboard::CreateSystemSelectionClipboard()
{
    return new MCEmscriptenRawClipboard;
}

MCRawClipboard* MCRawClipboard::CreateSystemDragboard()
{
    return new MCEmscriptenRawClipboard;
}


uindex_t MCEmscriptenRawClipboard::GetItemCount() const
{
    return 0;
}

const MCRawClipboardItem* MCEmscriptenRawClipboard::GetItemAtIndex(uindex_t p_index) const
{
    return NULL;
}

MCRawClipboardItem* MCEmscriptenRawClipboard::GetItemAtIndex(uindex_t p_index)
{
    return NULL;
}

void MCEmscriptenRawClipboard::Clear()
{
    ;
}

bool MCEmscriptenRawClipboard::IsOwned() const
{
    return false;
}

bool MCEmscriptenRawClipboard::IsExternalData() const
{
    return false;
}

MCRawClipboardItem* MCEmscriptenRawClipboard::CreateNewItem()
{
    return NULL;
}

bool MCEmscriptenRawClipboard::AddItem(MCRawClipboardItem* p_item)
{
    return false;
}

bool MCEmscriptenRawClipboard::PushUpdates()
{
    return false;
}

bool MCEmscriptenRawClipboard::PullUpdates()
{
    return false;
}

bool MCEmscriptenRawClipboard::FlushData()
{
    return false;
}

uindex_t MCEmscriptenRawClipboard::GetMaximumItemCount() const
{
    return 1;
}

MCStringRef MCEmscriptenRawClipboard::GetKnownTypeString(MCRawClipboardKnownType p_type) const
{
    return NULL;
}

MCDataRef MCEmscriptenRawClipboard::EncodeFileListForTransfer(MCStringRef p_file_list) const
{
    return NULL;
}

MCStringRef MCEmscriptenRawClipboard::DecodeTransferredFileList(MCDataRef p_data) const
{
    return NULL;
}

MCDataRef MCEmscriptenRawClipboard::EncodeHTMLFragmentForTransfer(MCDataRef p_html) const
{
	return NULL;
}

MCDataRef MCEmscriptenRawClipboard::DecodeTransferredHTML(MCDataRef p_html) const
{
	return NULL;
}

