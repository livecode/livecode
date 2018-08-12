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


#ifndef EMSCRIPTEN_CLIPBOARD_H
#define EMSCRIPTEN_CLIPBOARD_H


#include "raw-clipboard.h"
#include "foundation-auto.h"


class MCEmscriptenRawClipboard :
  public MCRawClipboard
{
public:
    
    // Inherited from MCRawClipboard
    virtual uindex_t GetItemCount() const;
    virtual const MCRawClipboardItem* GetItemAtIndex(uindex_t p_index) const;
    virtual MCRawClipboardItem* GetItemAtIndex(uindex_t p_index);
    virtual void Clear();
    virtual bool IsOwned() const;
    virtual bool IsExternalData() const;
    virtual MCRawClipboardItem* CreateNewItem();
    virtual bool AddItem(MCRawClipboardItem* p_item);
    virtual bool PushUpdates();
    virtual bool PullUpdates();
    virtual bool FlushData();
    virtual uindex_t GetMaximumItemCount() const;
    virtual MCStringRef GetKnownTypeString(MCRawClipboardKnownType p_type) const;
    virtual MCDataRef EncodeFileListForTransfer(MCStringRef p_file_list) const;
    virtual MCStringRef DecodeTransferredFileList(MCDataRef p_data) const;
	virtual MCDataRef EncodeHTMLFragmentForTransfer(MCDataRef p_html) const;
	virtual MCDataRef DecodeTransferredHTML(MCDataRef p_html) const;
};


#endif  /* ifndef EMSCRIPTEN_CLIPBOARD_H */
