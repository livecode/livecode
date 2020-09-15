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
#include "util.h"


// Table mapping MCRawClipboardKnownType constants to UTIs
const char* const MCMacRawClipboard::s_clipboard_types[] =
{
    "public.utf8-plain-text",
    "public.utf16-external-plain-text",
    NULL,
    "com.apple.traditional-mac-plain-text",
    NULL,
    
    "public.rtf",
    "public.html",
    
    "public.png",
    "com.compuserve.gif",
    "public.jpeg",
	"public.tiff",
    NULL,
    NULL,
    NULL,                   // "com.microsoft.bmp" but the Mac engine doesn't support this format
    NULL,
    
    "com.runrev.livecode.objects-1",
    "com.runrev.livecode.text-styled-1",
    
    "public.file-url",
    NULL,
    "public.url",
    NULL,
};


MCRawClipboard* MCRawClipboard::CreateSystemClipboard()
{
    return new MCMacRawClipboard([NSPasteboard generalPasteboard]);
}

MCRawClipboard* MCRawClipboard::CreateSystemSelectionClipboard()
{
    // Create a pasteboard internal to LiveCode. Pasteboards created in this
    // manner must be released using 'releaseGlobally'.
    return new MCMacRawClipboard([NSPasteboard pasteboardWithUniqueName], true);
}

MCRawClipboard* MCRawClipboard::CreateSystemDragboard()
{
    return new MCMacRawClipboard([NSPasteboard pasteboardWithName:NSDragPboard]);
}


MCMacRawClipboard::MCMacRawClipboard(NSPasteboard* p_pasteboard,
                                     bool p_release_globally) :
  MCRawClipboard(),
  m_pasteboard(p_pasteboard),
  m_last_changecount(0),
  m_items(nil),
  m_release_globally(p_release_globally),
  m_dirty(false),
  m_external_data(false)
{
    [m_pasteboard retain];
}

MCMacRawClipboard::~MCMacRawClipboard()
{
    [m_items release];
    
    /* If the pasteboard requires global release, then do this before releasing
     * it in the usual way. */
    if (m_release_globally)
    {
        [m_pasteboard releaseGlobally];
    }
    
    [m_pasteboard release];
}

uindex_t MCMacRawClipboard::GetItemCount() const
{
    return [m_items count];
}

const MCMacRawClipboardItem* MCMacRawClipboard::GetItemAtIndex(uindex_t p_index) const
{
#if UINDEX_MAX > NSUIntegerMax
    if (p_index > NSUIntegerMax)
        return NULL;
#endif

    // Retrieve the item at the specified index, if it exists
    if (p_index >= [m_items count])
        return NULL;
    
    id t_item = [m_items objectAtIndex:p_index];
    
    // Index may have been invalid
    if (t_item == nil)
        return NULL;
    
    return new MCMacRawClipboardItem(t_item);
}

MCMacRawClipboardItem* MCMacRawClipboard::GetItemAtIndex(uindex_t p_index)
{
    // Call the const-version of this method and cast away the constness. This
    // is safe as we know the underlying object is non-const.
    return const_cast<MCMacRawClipboardItem*> ((const_cast<const MCMacRawClipboard*> (this))->GetItemAtIndex(p_index));
}

void MCMacRawClipboard::Clear()
{
    // Discard any contents that we might already have
    m_dirty = true;
    m_external_data = false;
    [m_items release];
    m_items = nil;
}

bool MCMacRawClipboard::IsOwned() const
{
    // We own the clipboard if it hasn't been changed since we last asserted
    // ownership.
    return [m_pasteboard changeCount] == m_last_changecount;
}

bool MCMacRawClipboard::IsExternalData() const
{
    return m_external_data;
}

MCMacRawClipboardItem* MCMacRawClipboard::CreateNewItem()
{
    return new MCMacRawClipboardItem;
}

bool MCMacRawClipboard::AddItem(MCRawClipboardItem* p_item)
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
    
    // Clipboard has been modified
    m_dirty = true;
    
    // Push the item onto the array
    [m_items addObject:t_item->m_item];
    return true;
}

bool MCMacRawClipboard::PushUpdates()
{
    // Do nothing if no changes have been made
    if (!m_dirty)
        return true;
    
    // Take ownership of the clipboard
    m_last_changecount = [m_pasteboard clearContents];
    
    // Push all of the clipboard items onto the clipboard
    BOOL t_success = YES;
    if (m_items != nil)
        t_success = [m_pasteboard writeObjects:m_items];
    
    // Update the change count
    m_last_changecount = [m_pasteboard changeCount];
    
    // Clipboard is now clean
    if (t_success)
        m_dirty = false;
    
    return (t_success != NO);
}

bool MCMacRawClipboard::PullUpdates()
{
    // If we're still the owner of the clipboard, do nothing
    if (IsOwned())
        return true;
    
    // Grab the contents of the clipboard
    [m_items release];
    m_items = [[m_pasteboard pasteboardItems] mutableCopy];
    m_external_data = true;
    return (m_items != NULL);
}

bool MCMacRawClipboard::FlushData()
{
    // Automatically handled by the system.
    return true;
}

uindex_t MCMacRawClipboard::GetMaximumItemCount() const
{
    // As many items as an NSArray can handle
    return NSUIntegerMax;
}

MCStringRef MCMacRawClipboard::GetKnownTypeString(MCRawClipboardKnownType p_type) const
{
    // Index into the mapping table
    if (s_clipboard_types[p_type] != NULL)
        return MCSTR(s_clipboard_types[p_type]);
    
    return NULL;
}

MCDataRef MCMacRawClipboard::EncodeFileListForTransfer(MCStringRef p_file_path) const
{
    // Encode as UTF-8 and then as a URL
    MCAutoStringRef t_encoded;
    MCU_urlencode(p_file_path, true, &t_encoded);
    
    // Undo the transformation of slashes to '%2F'
    MCAutoStringRef t_modified;
    if (!MCStringMutableCopy(*t_encoded, &t_modified))
        return NULL;
    if (!MCStringFindAndReplace(*t_modified, MCSTR("%2F"), MCSTR("/"), kMCStringOptionCompareExact))
        return NULL;
    // Undo the transformation of spaces to '+'
    if (!MCStringFindAndReplace(*t_modified, MCSTR("+"), MCSTR(" "), kMCStringOptionCompareExact))
        return NULL;
    // Properly encode spaces
    if (!MCStringFindAndReplace(*t_modified, MCSTR(" "), MCSTR("%20"), kMCStringOptionCompareExact))
        return NULL;
    
    // Add the required "file://" prefix to the path
    if (!MCStringPrepend(*t_modified, MCSTR("file://")))
        return NULL;
    
    // Convert the encoded path to bytes
    MCDataRef t_bytes;
    if (!MCStringEncode(*t_modified, kMCStringEncodingNative, false, t_bytes))
        return NULL;
    return t_bytes;
}

MCStringRef MCMacRawClipboard::DecodeTransferredFileList(MCDataRef p_encoded_path) const
{
    // Convert the bytes into its string equivalent
    MCAutoStringRef t_bytes;
    if (!MCStringDecode(p_encoded_path, kMCStringEncodingNative, false, &t_bytes))
        return NULL;
    
    // URL-decode and then decode the resulting UTF-8 bytes
    MCAutoStringRef t_decoded;
    MCU_urldecode(*t_bytes, true, &t_decoded);
    
    // Remove the file prefix, if it exists
    MCStringRef t_path = NULL;
    if (MCStringBeginsWithCString(*t_decoded, (const char_t*)"file://", kMCStringOptionCompareExact))
    {
        MCStringCopySubstring(*t_decoded, MCRangeMake(7, MCStringGetLength(*t_decoded)-7), t_path);
    }
    else
    {
        t_path = MCValueRetain(*t_decoded);
    }
    
    // All done
    return t_path;
}

MCDataRef MCMacRawClipboard::EncodeHTMLFragmentForTransfer(MCDataRef p_html) const
{
	return MCValueRetain(p_html);
}

MCDataRef MCMacRawClipboard::DecodeTransferredHTML(MCDataRef p_html) const
{
	return MCValueRetain(p_html);
}

MCStringRef MCMacRawClipboard::CopyAsUTI(MCStringRef p_key)
{
    // If the key is already in UTI form, just pass it out
    uindex_t t_index;
    if (!MCStringFirstIndexOfChar(p_key, ':', 0, kMCStringOptionCompareExact, t_index))
        return MCValueRetain(p_key);
    
    // Split the string into a prefix and a suffix
    MCAutoStringRef t_prefix, t_suffix;
    if (!MCStringCopySubstring(p_key, MCRangeMake(0, t_index), &t_prefix))
        return NULL;
    if (!MCStringCopySubstring(p_key, MCRangeMake(t_index + 1, MCStringGetLength(p_key) - t_index - 1), &t_suffix))
        return NULL;
    
    // Create the UTI for this class and type
    CFStringRef t_class, t_tag;
    if (!MCStringConvertToCFStringRef(*t_prefix, t_class))
        return NULL;
    if (!MCStringConvertToCFStringRef(*t_suffix, t_tag))
    {
        CFRelease(t_class);
        return NULL;
    }
    CFStringRef t_uti = UTTypeCreatePreferredIdentifierForTag(t_class, t_tag, kUTTypeData);
    CFRelease(t_class);
    CFRelease(t_tag);
    if (t_uti == NULL)
        return NULL;
    
    // Convert the UTI to a StringRef
    MCStringRef t_return = NULL;
    MCStringCreateWithCFStringRef(t_uti, t_return);
    CFRelease(t_uti);
    return t_return;
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

uindex_t MCMacRawClipboardItem::GetRepresentationCount() const
{
    // The number of representations was grabbed in the constructor and is equal
    // to the number of items in the representation cache array.
    return m_rep_cache.Size();
}

const MCMacRawClipboardItemRep* MCMacRawClipboardItem::FetchRepresentationAtIndex(uindex_t p_index) const
{
    // Is the index valid?
    if (p_index >= GetRepresentationCount())
        return NULL;
    
    // If there isn't a representation object for this index yet, create it now
    if (m_rep_cache[p_index] == NULL)
    {
        m_rep_cache[p_index] = new MCMacRawClipboardItemRep(m_item, p_index);
    }
    
    return m_rep_cache[p_index];
}

bool MCMacRawClipboardItem::AddRepresentation(MCStringRef p_type, MCDataRef p_bytes)
{
    // Look for an existing representation with this type
    MCMacRawClipboardItemRep* t_rep = NULL;
    for (uindex_t i = 0; i < GetRepresentationCount(); i++)
    {
        // Calling this method forces the rep object to be generated
        if (!FetchRepresentationAtIndex(i))
            return false;
        
        MCAutoStringRef t_type;
        t_type.Give(m_rep_cache[i]->CopyTypeString());
        if (t_type.IsSet() &&
            MCStringIsEqualTo(*t_type, p_type, kMCStringOptionCompareExact))
        {
            // This is the rep we're looking for
            t_rep = m_rep_cache[i];
            
            // Update the cached data for this representation
            t_rep->m_data.Reset(p_bytes);
            
            break;
        }
    }
    
    // If there wasn't an existing representation, create one now
    if (t_rep == NULL)
    {
        // Extend the representation array to hold this new representation
        uindex_t t_index = m_rep_cache.Size();
        if (!m_rep_cache.Extend(t_index + 1))
            return false;
        
        // Allocate a new representation object.
        // If this fails to allocate a new item, we ignore the failure. This is safe
        // as the cache can be re-generated on demand from the data stored in the
        // NSPasteboardItem itself.
        m_rep_cache[t_index] = t_rep = new MCMacRawClipboardItemRep(m_item, t_index, p_type, p_bytes);
    }
    
    // Convert the RawClipboardData-style key into a UTI
    MCAutoStringRef t_uti(MCMacRawClipboard::CopyAsUTI(p_type));
    
    // Turn the type string and data into their NS equivalents.
    // Note that the NSData is auto-released when we get it.
    NSString* t_type;
    NSData* t_data;
    if (!MCStringConvertToCFStringRef(*t_uti, (CFStringRef&)t_type))
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

bool MCMacRawClipboardItem::AddRepresentation(MCStringRef p_type, render_callback_t p_render, void* p_context)
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

MCStringRef MCMacRawClipboardItemRep::CopyTypeString() const
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
    if (!MCStringCreateWithCFStringRef((CFStringRef)t_type, t_type_string))
        return NULL;
    
    m_type = t_type_string;
    return t_type_string;
}

MCDataRef MCMacRawClipboardItemRep::CopyData() const
{
    // If the data has already been fetched, just return it
    if (*m_data != NULL)
    {
        return MCValueRetain(*m_data);
    }
    
    // Objects of type NSURL need special handling when attempting to get their
    // URL so that it is returned as a path rather than an inode number.
    if (MCStringIsEqualTo(*m_type, MCSTR("public.file-url"), kMCStringOptionCompareExact))
    {
        // Turn the data into an NSURL object
        NSData* t_bytes = [m_item dataForType:@"public.file-url"];
        if (t_bytes == nil)
            return NULL;
        NSString* t_url_string = [[NSString alloc] initWithData:t_bytes encoding:NSUTF8StringEncoding];
        if (t_url_string == nil)
            return NULL;
        NSURL* t_url = [NSURL URLWithString:t_url_string];
        [t_url_string release];
        if (t_url == nil)
            return NULL;
        
        // Get the current path of the file referenced by the URL
        NSString* t_path = [t_url path];
        if (t_path == nil)
            return NULL;
        
        // Turn this path into a LiveCode string
        MCAutoStringRef t_path_string;
        if (!MCStringCreateWithCFStringRef((CFStringRef)t_path, &t_path_string))
            return NULL;
        
        // Because this needs to return data, UTF-8 encode the result
        if (!MCStringEncode(*t_path_string, kMCStringEncodingUTF8, false, &m_data))
            return NULL;
        
        return MCValueRetain(*m_data);
    }
    
    // Is this item an NSPasteboardItem or something else?
    else if ([m_item isKindOfClass:[NSPasteboardItem class]])
    {
        // Get the type string for this representation (as lookup is by type)
        MCAutoStringRef t_type_string;
        t_type_string.Give(CopyTypeString());
        if (*t_type_string == NULL)
            return NULL;
        
        // Convert the type string to a UTI
        MCAutoStringRef t_uti;
        t_uti.Give(MCMacRawClipboard::CopyAsUTI(*t_type_string));
        
        // Convert from a StringRef to a CFString
        CFStringRef t_type;
        if (!MCStringConvertToCFStringRef(*t_uti, t_type))
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
