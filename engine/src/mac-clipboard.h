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
    virtual MCStringRef getTypeString() const;
    virtual MCDataRef getData() const;
    
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
    virtual uindex_t getRepresentationCount() const;
    virtual const MCMacRawClipboardItemRep* fetchRepresentationAtIndex(uindex_t p_index) const;
    virtual bool addRepresentation(MCStringRef p_type, MCDataRef p_bytes);
    virtual bool addRepresentation(MCStringRef p_type, render_callback_t, void* p_context);
    
    
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
    virtual uindex_t getItemCount() const;
    virtual const MCMacRawClipboardItem* getItemAtIndex(uindex_t p_index) const;
    virtual MCMacRawClipboardItem* getItemAtIndex(uindex_t p_index);
    virtual void clear();
    virtual bool isOwned() const;
    virtual MCMacRawClipboardItem* createNewItem();
    virtual bool pushItem(MCRawClipboardItem* p_item);
    virtual bool PushUpdates();
    virtual bool PullUpdates();
    virtual bool flushData();
    virtual uindex_t getMaximumItemCount() const;
    virtual MCStringRef getKnownTypeString(MCRawClipboardKnownType p_type) const;
    
    // Constructor. The NSPasteboard being wrapped is required.
    MCMacRawClipboard(NSPasteboard* p_pasteboard);
    
private:
    
    // The NSPasteboard being wrapped
    NSPasteboard* m_pasteboard;
    
    // Change count used to track ownership of the clipboard
    NSInteger m_last_changecount;
    
    // The array that we are pushing onto the clipboard
    NSMutableArray* m_items;
    
    
    // Table used to map from MCRawClipboardKnownType constants to Apple's UTIs
    static const char* const s_clipboard_types[];
    
    
    // Destrutor
    virtual ~MCMacRawClipboard();
};


#endif  /* ifndef MAC_CLIPBOARD_H */
