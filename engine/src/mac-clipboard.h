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



#ifndef MAC_CLIPBOARD_H
#define MAC_CLIPBOARD_H


#include "raw-clipboard.h"

#import <AppKit/AppKit.h>

#include "foundation-auto.h"


class MCMacRawClipboardItemRep :
public MCRawClipboardItemRep
{
public:
    
    // Methods inherited from MCRawClipboardItem
    virtual MCStringRef CopyTypeString() const;
    virtual MCDataRef CopyData() const;
    
private:
    
    // The representation is identified by an (item,index) tuple
    id m_item;
    NSUInteger m_index;
    
    // Cache of the StringRef identifying the data type and the DataRef holding
    // the data itself for this representation. These are only initialised when
    // the information is requested.
    mutable MCAutoStringRef m_type;
    mutable MCAutoDataRef m_data;
    
    
    // Lifetime is managed by the parent MCMacRawClipboardItem
    friend class MCMacRawClipboardItem;
    MCMacRawClipboardItemRep(id p_item, NSUInteger p_index);
    MCMacRawClipboardItemRep(id p_item, NSUInteger p_index, MCStringRef p_type, MCDataRef p_data);
    virtual ~MCMacRawClipboardItemRep();
};


class MCMacRawClipboardItem :
  public MCRawClipboardItem
{
public:
    
    // Methods inherited from MCRawClipboardItem
    virtual uindex_t GetRepresentationCount() const;
    virtual const MCMacRawClipboardItemRep* FetchRepresentationAtIndex(uindex_t p_index) const;
    virtual bool AddRepresentation(MCStringRef p_type, MCDataRef p_bytes);
    virtual bool AddRepresentation(MCStringRef p_type, render_callback_t, void* p_context);
    
    
private:
    
    friend class MCMacRawClipboard;
    
    // The object being wrapped
    id m_item;
    
    // Cache of the representation objects. Marked as mutable as the various
    // representation objects are only allocated when they are requested.
    mutable MCAutoArray<MCMacRawClipboardItemRep*> m_rep_cache;
    
    // Constructors. If no item is supplied, a new NSPasteboardItem will be
    // automatically created.
    MCMacRawClipboardItem();
    MCMacRawClipboardItem(id p_item);
    
    // Destructor
    virtual ~MCMacRawClipboardItem();
};


class MCMacRawClipboard :
  public MCRawClipboard
{
public:
    
    // Methods inherited from MCRawClipboard
    virtual uindex_t GetItemCount() const;
    virtual const MCMacRawClipboardItem* GetItemAtIndex(uindex_t p_index) const;
    virtual MCMacRawClipboardItem* GetItemAtIndex(uindex_t p_index);
    virtual void Clear();
    virtual bool IsOwned() const;
    virtual bool IsExternalData() const;
    virtual MCMacRawClipboardItem* CreateNewItem();
    virtual bool AddItem(MCRawClipboardItem* p_item);
    virtual bool PushUpdates();
    virtual bool PullUpdates();
    virtual bool FlushData();
    virtual uindex_t GetMaximumItemCount() const;
    virtual MCStringRef GetKnownTypeString(MCRawClipboardKnownType p_type) const;
    virtual MCDataRef EncodeFileListForTransfer(MCStringRef p_file_path) const;
    virtual MCStringRef DecodeTransferredFileList(MCDataRef p_encoded_path) const;
	virtual MCDataRef EncodeHTMLFragmentForTransfer(MCDataRef p_html) const;
	virtual MCDataRef DecodeTransferredHTML(MCDataRef p_html) const;
    
    // Constructor. The NSPasteboard being wrapped is required.
    MCMacRawClipboard(NSPasteboard* p_pasteboard, bool p_release_globally = false);
    
    // Converts a LiveCode-style RawClipboardData key into an OSX UTI
    static MCStringRef CopyAsUTI(MCStringRef p_key);
    
private:
    
    // The NSPasteboard being wrapped
    NSPasteboard* m_pasteboard;
    
    // Change count used to track ownership of the clipboard
    NSInteger m_last_changecount;
    
    // The array that we are pushing onto the clipboard
    NSMutableArray* m_items;
    
    // If true, the NSPasteboard must be released with releaseGlobally rather
    // than release.
    bool m_release_globally;
    
    // Indicates whether any changes have been made to the clipboard
    bool m_dirty;
    
    // Indicates that the data on the clipboard is from outside LiveCode
    bool m_external_data;
    
    
    // Table used to map from MCRawClipboardKnownType constants to Apple's UTIs
    static const char* const s_clipboard_types[];
    
    
    // Destrutor
    virtual ~MCMacRawClipboard();
};


#endif  /* ifndef MAC_CLIPBOARD_H */
