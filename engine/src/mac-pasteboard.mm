/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

#include <Cocoa/Cocoa.h>

#include "core.h"
#include "globdefs.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

////////////////////////////////////////////////////////////////////////////////

struct MCPlatformPasteboard
{
	uint32_t references;
	NSPasteboard *ns_pasteboard;
};

////////////////////////////////////////////////////////////////////////////////

void MCMacPlatformPasteboardCreate(NSPasteboard *p_pasteboard, MCPlatformPasteboardRef& r_pasteboard)
{
	MCPlatformPasteboardRef t_pasteboard;
	/* UNCHECKED */ MCMemoryNew(t_pasteboard);
	
	t_pasteboard -> references = 1;
	t_pasteboard -> ns_pasteboard = p_pasteboard;
	[p_pasteboard retain];

#if 0
	NSArray *t_items;
	t_items = [p_pasteboard pasteboardItems];
	
	for(uindex_t i = 0; i < [t_items count]; i++)
	{
		NSArray *t_types;
		t_types = [[t_items objectAtIndex: i] types];
		
		NSLog(@"Item %d", i);
		for(uindex_t j = 0; j < [t_types count]; j++)
		{
			NSLog(@"  [%d] = %@", j, [t_types objectAtIndex: j]);
		}
	}

	r_pasteboard = nil;
#endif
}

void MCPlatformPasteboardRetain(MCPlatformPasteboardRef p_pasteboard)
{
	p_pasteboard -> references += 1;
}

void MCPlatformPasteboardRelease(MCPlatformPasteboardRef p_pasteboard)
{
	p_pasteboard -> references -= 1;
	if (p_pasteboard -> references == 0)
	{
		[p_pasteboard -> ns_pasteboard release];
		MCMemoryDelete(p_pasteboard);
	}
}

void MCPlatformPasteboardGetGeneration(MCPlatformPasteboardRef p_pasteboard, uindex_t& r_generation)
{
	r_generation = [p_pasteboard -> ns_pasteboard changeCount];
}

bool MCPlatformPasteboardQuery(MCPlatformPasteboardRef p_pasteboard, MCPlatformPasteboardFlavor*& r_flavors, uindex_t& r_flavor_count)
{
	// Fetch the array of items.
	NSArray *t_items;
	t_items = [p_pasteboard -> ns_pasteboard pasteboardItems];
	
	// For now we are only interested in the first one.
	// COCOA-TODO: Support multiple files.
	NSPasteboardItem *t_item;
	if ([t_items count] > 0)
		t_item = (NSPasteboardItem *)[t_items objectAtIndex: 0];
	else
		t_item = nil;
		
	// Get the list of types.
	NSArray *t_types;
	if (t_item != nil)
		t_types = [t_item types];
	else
		t_types = nil;
		
	// Now process the types on the pasteboard.
	MCPlatformPasteboardFlavor *t_flavors;
	uindex_t t_flavor_count;
	t_flavors = nil;
	t_flavor_count = 0;
	if (t_item != nil)
	{
		// Allocate the array of flavors, this need be no longer than the number
		// of published types for the item.
		if (!MCMemoryNewArray([t_types count], t_flavors))
			return false;
		
		// Now create a list of types in order.
		for(uindex_t i = 0; i < [t_types count]; i++)
		{
			NSString *t_type;
			t_type = (NSString *)[t_types objectAtIndex: i];
			
			MCPlatformPasteboardFlavor t_flavor;
			if ([t_type isEqualTo: (NSString *)kUTTypeUTF8PlainText])
				t_flavor = kMCPlatformPasteboardFlavorUTF8;
			else if ([t_type isEqualTo: (NSString *)kUTTypeRTF])
				t_flavor = kMCPlatformPasteboardFlavorRTF;
			else if ([t_type isEqualTo: (NSString *)kUTTypeHTML])
				t_flavor = kMCPlatformPasteboardFlavorHTML;
			else if ([t_type isEqualTo: (NSString *)kUTTypeFileURL])
				t_flavor = kMCPlatformPasteboardFlavorFiles;
			else if ([t_type isEqualTo: (NSString *)kUTTypeJPEG])
				t_flavor = kMCPlatformPasteboardFlavorJPEG;
			else if ([t_type isEqualTo: (NSString *)kUTTypeGIF])
				t_flavor = kMCPlatformPasteboardFlavorGIF;
			else if ([t_type isEqualTo: (NSString *)kUTTypePNG])
				t_flavor = kMCPlatformPasteboardFlavorPNG;
			else
			{
				NSLog(@"Unknown pboard type %@", t_type);
				continue;
			}
		
			t_flavors[t_flavor_count++] = t_flavor;
		}
	}
	
	r_flavors = t_flavors;
	r_flavor_count = t_flavor_count;
	
	return true;
}

bool MCPlatformPasteboardFetch(MCPlatformPasteboardRef p_pasteboard, MCPlatformPasteboardFlavor p_flavor, void*& r_bytes, uindex_t& r_byte_count)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////


NSDragOperation MCMacPlatformMapDragOperationToNSDragOperation(MCPlatformDragOperation p_operation)
{
	switch(p_operation)
	{
		case kMCPlatformDragOperationNone:
			return NSDragOperationNone;
		case kMCPlatformDragOperationCopy:
			return NSDragOperationCopy;
		case kMCPlatformDragOperationMove:
			return NSDragOperationMove;
		case kMCPlatformDragOperationLink:
			return NSDragOperationLink;
		default:
			assert(false);
			break;
	}
	
	return NSDragOperationNone;
}

////////////////////////////////////////////////////////////////////////////////
