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



#include "mbliphone-clipboard.h"


MCRawClipboard* MCRawClipboard::CreateSystemClipboard()
{
    return new MCIPhoneRawClipboard;
}

MCRawClipboard* MCRawClipboard::CreateSystemSelectionClipboard()
{
    return new MCIPhoneRawClipboard;
}

MCRawClipboard* MCRawClipboard::CreateSystemDragboard()
{
    return new MCIPhoneRawClipboard;
}


uindex_t MCIPhoneRawClipboard::GetItemCount() const
{
    return 0;
}

const MCRawClipboardItem* MCIPhoneRawClipboard::GetItemAtIndex(uindex_t p_index) const
{
    return NULL;
}

MCRawClipboardItem* MCIPhoneRawClipboard::GetItemAtIndex(uindex_t p_index)
{
    return NULL;
}

void MCIPhoneRawClipboard::Clear()
{
    ;
}

bool MCIPhoneRawClipboard::IsOwned() const
{
    return false;
}

bool MCIPhoneRawClipboard::IsExternalData() const
{
    return false;
}

MCRawClipboardItem* MCIPhoneRawClipboard::CreateNewItem()
{
    return NULL;
}

bool MCIPhoneRawClipboard::AddItem(MCRawClipboardItem* p_item)
{
    return false;
}

bool MCIPhoneRawClipboard::PushUpdates()
{
    return false;
}

bool MCIPhoneRawClipboard::PullUpdates()
{
    return false;
}

bool MCIPhoneRawClipboard::FlushData()
{
    return false;
}

uindex_t MCIPhoneRawClipboard::GetMaximumItemCount() const
{
    return 1;
}

MCStringRef MCIPhoneRawClipboard::GetKnownTypeString(MCRawClipboardKnownType p_type) const
{
    return NULL;
}

MCDataRef MCIPhoneRawClipboard::EncodeFileListForTransfer(MCStringRef p_file_list) const
{
    return NULL;
}

MCStringRef MCIPhoneRawClipboard::DecodeTransferredFileList(MCDataRef p_data) const
{
    return NULL;
}

MCDataRef MCIPhoneRawClipboard::EncodeHTMLFragmentForTransfer(MCDataRef p_html) const
{
	return NULL;
}

MCDataRef MCIPhoneRawClipboard::DecodeTransferredHTML(MCDataRef p_html) const
{
	return NULL;
}

