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


#include "mac-clipboard.h"

#include "foundation-auto.h"


// Table mapping MCRawClipboardKnownType constants to UTIs
const char* const MCMacRawClipboard::s_clipboard_types[] =
{
    "public.utf8-plain-text",
    "public.utf16-plain-text",
    NULL,
    "com.apple.traditional-mac-plain-text",
    NULL,
    
    "public.rtf",
    "public.html",
    
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    
    "com.runrev.livecode.objects-1",
    
    NULL,
    NULL,
    NULL,
    NULL,
};


MCRawClipboard* MCRawClipboard::Create()
{
    return new MCMacRawClipboard([NSPasteboard generalPasteboard]);
}


MCMacRawClipboard::MCMacRawClipboard(NSPasteboard* p_pasteboard) :
  MCRawClipboard(),
  m_pasteboard(p_pasteboard),
  m_last_changecount(0),
  m_items(nil)
{
    [m_pasteboard retain];
}

MCMacRawClipboard::~MCMacRawClipboard()
{
    [m_items release];
    [m_pasteboard release];
}

uindex_t MCMacRawClipboard::getItemCount() const
{
    NSArray* t_items = [m_pasteboard pasteboardItems];
    
    // If an error occurred retrieving the clipboard items, we will have
    // received nil.
    if (t_items == nil)
        return 0;
    
    return [t_items count];
}

const MCMacRawClipboardItem* MCMacRawClipboard::getItemAtIndex(uindex_t p_index) const
{
    // Range check
    if (p_index > NSUIntegerMax)
        return NULL;
    
    // Retrieve the item at the specified index, if it exists
    if (p_index >= [m_items count])
        return NULL;
    
    id t_item = [m_items objectAtIndex:p_index];
    
    // Index may have been invalid
    if (t_item == nil)
        return NULL;
    
    return new MCMacRawClipboardItem(t_item);
}

MCMacRawClipboardItem* MCMacRawClipboard::getItemAtIndex(uindex_t p_index)
{
    // Call the const-version of this method and cast away the constness. This
    // is safe as we know the underlying object is non-const.
    return const_cast<MCMacRawClipboardItem*> ((const_cast<const MCMacRawClipboard*> (this))->getItemAtIndex(p_index));
}

void MCMacRawClipboard::clear()
{
    // Discard any contents that we might already have
    [m_items release];
    m_items = nil;
}

bool MCMacRawClipboard::isOwned() const
{
    // We own the clipboard if it hasn't been changed since we last asserted
    // ownership.
    return [m_pasteboard changeCount] == m_last_changecount;
}

MCMacRawClipboardItem* MCMacRawClipboard::createNewItem()
{
    return new MCMacRawClipboardItem;
}

bool MCMacRawClipboard::pushItem(MCRawClipboardItem* p_item)
{
    // Assume that the user of this class has read the documentation and is only
    // passing MCMacRawClipboardItem instances to us.
    MCMacRawClipboardItem* t_item = static_cast<MCMacRawClipboardItem*>(p_item);
    
    // If we haven't already done so, create the items array
    if (m_items == nil)
        m_items = [[NSMutableArray alloc] init];
    
    // Check for allocation success
    if (m_items == nil)
        return false;
    
    // Push the item onto the array
    [m_items addObject:t_item->m_item];
    return true;
}

bool MCMacRawClipboard::PushUpdates()
{
    // Take ownership of the clipboard
    m_last_changecount = [m_pasteboard clearContents];
    
    // Push all of the clipboard items onto the clipboard
    BOOL t_success = YES;
    if (m_items != nil)
        t_success = [m_pasteboard writeObjects:m_items];
    
    // Update the change count
    m_last_changecount = [m_pasteboard changeCount];
    
    return (t_success != NO);
}

bool MCMacRawClipboard::PullUpdates()
{
    // Grab the contents of the clipboard
    [m_items release];
    m_items = [[m_pasteboard pasteboardItems] mutableCopy];
    return (m_items != NULL);
}

bool MCMacRawClipboard::flushData()
{
    // Automatically handled by the system.
    return true;
}

uindex_t MCMacRawClipboard::getMaximumItemCount() const
{
    // As many items as an NSArray can handle
    return NSUIntegerMax;
}

MCStringRef MCMacRawClipboard::getKnownTypeString(MCRawClipboardKnownType p_type) const
{
    // TODO: implement
    return NULL;
}


MCMacRawClipboardItem::MCMacRawClipboardItem() :
  MCRawClipboardItem(),
  m_item([[NSPasteboardItem alloc] init]),
  m_rep_cache()
{
    ;
}


MCMacRawClipboardItem::MCMacRawClipboardItem(id p_item) :
  MCRawClipboardItem(),
  m_item(p_item),
  m_rep_cache()
{
    [m_item retain];
    
    // Count the number of representations
    NSUInteger t_rep_count;
    
    // Is this an NSPasteboardItem? If not, it only has one representation
    if (![m_item isKindOfClass:[NSPasteboardItem class]])
    {
        t_rep_count = 1;
    }
    else
    {
        // Get the type array for this item and count the entries
        NSArray* t_types = [m_item types];
        t_rep_count = [t_types count];
    }
    
    // Initialise the representation cache to be empty
    //
    // This result isn't checked as a failure to allocate the memory for the
    // array will leave it with a zero size, making the item look as if it has
    // no representations. But that is probably fine, as if we don't have enough
    // heap available for this small array, we won't have enough for actually
    // copying data from the clipboard...
    //
    /* UNCHECKED */ m_rep_cache.Resize(t_rep_count);
}

MCMacRawClipboardItem::~MCMacRawClipboardItem()
{
    [m_item release];
    
    // Clear the representation cache
    for (uindex_t i = 0; i < m_rep_cache.Size(); i++)
    {
        delete m_rep_cache[i];
    }
}

uindex_t MCMacRawClipboardItem::getRepresentationCount() const
{
    // The number of representations was grabbed in the constructor and is equal
    // to the number of items in the representation cache array.
    return m_rep_cache.Size();
}

const MCMacRawClipboardItemRep* MCMacRawClipboardItem::fetchRepresentationAtIndex(uindex_t p_index) const
{
    // Is the index valid?
    if (p_index >= getRepresentationCount())
        return NULL;
    
    // If there isn't a representation object for this index yet, create it now
    if (m_rep_cache[p_index] == NULL)
    {
        m_rep_cache[p_index] = new MCMacRawClipboardItemRep(m_item, p_index);
    }
    
    return m_rep_cache[p_index];
}

bool MCMacRawClipboardItem::addRepresentation(MCStringRef p_type, MCDataRef p_bytes)
{
    // Extend the representation array to hold this new representation
    uindex_t t_index = m_rep_cache.Size();
    if (!m_rep_cache.Extend(t_index + 1))
        return false;
    
    // Allocate a new representation object.
    // If this fails to allocate a new item, we ignore the failure. This is safe
    // as the cache can be re-generated on demand from the data stored in the
    // NSPasteboardItem itself.
    m_rep_cache[t_index] = new MCMacRawClipboardItemRep(m_item, t_index, p_type, p_bytes);
    
    // Turn the type string and data into their NS equivalents.
    // Note that the NSData is auto-released when we get it.
    NSString* t_type;
    NSData* t_data;
    if (!MCStringConvertToCFStringRef(p_type, (CFStringRef&)t_type))
        return false;
    t_data = [NSData dataWithBytes:MCDataGetBytePtr(p_bytes) length:MCDataGetLength(p_bytes)];
    if (t_data == nil)
    {
        [t_type release];
        return false;
    }
    
    // Add a new representation to the NSPasteboardItem object
    BOOL t_result = [m_item setData:t_data forType:t_type];
    [t_type release];
    return t_result;
}

bool MCMacRawClipboardItem::addRepresentation(MCStringRef p_type, render_callback_t p_render, void* p_context)
{
    return false;
}


MCMacRawClipboardItemRep::MCMacRawClipboardItemRep(id p_item, NSUInteger p_index) :
  MCRawClipboardItemRep(),
  m_item(p_item),
  m_index(p_index),
  m_type(),
  m_data()
{
    // m_item is not retained as the parent MCMacRawClipboardItem holds the
    // reference for us.
}

MCMacRawClipboardItemRep::MCMacRawClipboardItemRep(id p_item, NSUInteger p_index, MCStringRef p_type, MCDataRef p_data) :
  MCRawClipboardItemRep(),
  m_item(p_item),
  m_index(p_index),
  m_type(),
  m_data()
{
    // Retain references to the type string and data
    m_type = p_type;
    m_data = p_data;
    
    // m_item is not retained as the parent MCMacRawClipboardItem holds the
    // reference for us.
}

MCMacRawClipboardItemRep::~MCMacRawClipboardItemRep()
{
    // m_item is not released as the parent MCMacRawClipboardItem holds the
    // refernce for us.
}

MCStringRef MCMacRawClipboardItemRep::getTypeString() const
{
    // If the type string has already been fetched, just return it
    if (*m_type != NULL)
    {
        return MCValueRetain(*m_type);
    }
    
    // The type string
    NSString* t_type = nil;
    
    // Is this item an NSPasteboardItem or something else?
    if ([m_item isKindOfClass:[NSPasteboardItem class]])
    {
        // Is the index valid for this item?
        NSArray* t_types = [m_item types];
        if (m_index >= [t_types count])
            return NULL;
        
        // Get the type string
        t_type = (NSString*)[t_types objectAtIndex:m_index];
        
        // If the type string is dynamic, try to turn in into some other useful
        // type. The order of these is completely arbitrary... the goal here is
        // to turn it into something consistent.
        //
        // A domain is prefixed so that the transformation is reversible (the
        // colon character is prohibited in UTIs).
        if ([t_type hasPrefix:@"dyn."])
        {
            CFStringRef t_new_type = nil;
            CFStringRef t_new_type_domain = nil;
            
            if (t_new_type == nil)
            {
                t_new_type = UTTypeCopyPreferredTagWithClass((CFStringRef)t_type, kUTTagClassMIMEType);
                t_new_type_domain = kUTTagClassMIMEType;
            }
            
            if (t_new_type == nil)
            {
                t_new_type = UTTypeCopyPreferredTagWithClass((CFStringRef)t_type, kUTTagClassNSPboardType);
                t_new_type_domain = kUTTagClassNSPboardType;
            }
            
            if (t_new_type == nil)
            {
                t_new_type = UTTypeCopyPreferredTagWithClass((CFStringRef)t_type, kUTTagClassOSType);
                t_new_type_domain = kUTTagClassOSType;
            }
            
            if (t_new_type == nil)
            {
                t_new_type = UTTypeCopyPreferredTagWithClass((CFStringRef)t_type, kUTTagClassFilenameExtension);
                t_new_type_domain = kUTTagClassFilenameExtension;
            }
            
            if (t_new_type != nil)
            {
                NSMutableString* t_prefixed_type = [[NSMutableString alloc] init];
                [t_prefixed_type autorelease];
                [t_prefixed_type appendFormat:@"%@:%@", t_new_type_domain, t_new_type];
                CFRelease(t_new_type);
                
                t_type = t_prefixed_type;
            }
        }
    }
    else
    {
        // Not an NSPasteboardItem - only index 0 is valid
        if (m_index > 0)
            return NULL;
        
        // Use the class name as the type string
        t_type = NSStringFromClass([m_item class]);
    }
    
    // Convert the NSString into a StringRef
    MCStringRef t_type_string;
    if (!MCStringCreateWithCFString((CFStringRef)t_type, t_type_string))
        return NULL;
    
    m_type = t_type_string;
    return t_type_string;
}

MCDataRef MCMacRawClipboardItemRep::getData() const
{
    // If the data has already been fetched, just return it
    if (*m_data != NULL)
    {
        return MCValueRetain(*m_data);
    }
    
    // Is this item an NSPasteboardItem or something else?
    if ([m_item isKindOfClass:[NSPasteboardItem class]])
    {
        // Get the type string for this representation (as lookup is by type)
        MCAutoStringRef t_type_string;
        t_type_string = getTypeString();
        if (*t_type_string == NULL)
            return NULL;
        
        // Convert from a StringRef to a CFString
        CFStringRef t_type;
        if (!MCStringConvertToCFStringRef(*t_type_string, t_type))
            return NULL;
        
        // Get the data for this type
        NSData* t_bytes = [m_item dataForType:(NSString*)t_type];
        CFRelease(t_type);
        
        // Convert the data to a DataRef
        MCDataRef t_data;
        if (!MCDataCreateWithBytes((const byte_t*)[t_bytes bytes], [t_bytes length], t_data))
            return NULL;
        
        m_data = t_data;
        return t_data;
    }
    else
    {
        // The item is an arbitrary Objective-C object so we can't really do
        // anything with it. Return an empty data ref.
        m_data = kMCEmptyData;
        return kMCEmptyData;
    }
}
