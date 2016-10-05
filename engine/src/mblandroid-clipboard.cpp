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



#include "mblandroid-clipboard.h"


MCRawClipboard* MCRawClipboard::CreateSystemClipboard()
{
    return new MCAndroidRawClipboard;
}

MCRawClipboard* MCRawClipboard::CreateSystemSelectionClipboard()
{
    return new MCAndroidRawClipboard;
}

MCRawClipboard* MCRawClipboard::CreateSystemDragboard()
{
    return new MCAndroidRawClipboard;
}


uindex_t MCAndroidRawClipboard::GetItemCount() const
{
    return 0;
}

const MCRawClipboardItem* MCAndroidRawClipboard::GetItemAtIndex(uindex_t p_index) const
{
    return NULL;
}

MCRawClipboardItem* MCAndroidRawClipboard::GetItemAtIndex(uindex_t p_index)
{
    return NULL;
}

void MCAndroidRawClipboard::Clear()
{
    ;
}

bool MCAndroidRawClipboard::IsOwned() const
{
    return false;
}

bool MCAndroidRawClipboard::IsExternalData() const
{
    return false;
}

MCRawClipboardItem* MCAndroidRawClipboard::CreateNewItem()
{
    return NULL;
}

bool MCAndroidRawClipboard::AddItem(MCRawClipboardItem* p_item)
{
    return false;
}

bool MCAndroidRawClipboard::PushUpdates()
{
    return false;
}

bool MCAndroidRawClipboard::PullUpdates()
{
    return false;
}

bool MCAndroidRawClipboard::FlushData()
{
    return false;
}

uindex_t MCAndroidRawClipboard::GetMaximumItemCount() const
{
    return 1;
}

MCStringRef MCAndroidRawClipboard::GetKnownTypeString(MCRawClipboardKnownType p_type) const
{
    return NULL;
}

MCDataRef MCAndroidRawClipboard::EncodeFileListForTransfer(MCStringRef p_file_list) const
{
    return NULL;
}

MCStringRef MCAndroidRawClipboard::DecodeTransferredFileList(MCDataRef p_data) const
{
    return NULL;
}

MCDataRef MCAndroidRawClipboard::EncodeHTMLFragmentForTransfer(MCDataRef p_html) const
{
	return NULL;
}

MCDataRef MCAndroidRawClipboard::DecodeTransferredHTML(MCDataRef p_html) const
{
	return NULL;
}

