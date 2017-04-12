/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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

#include "globdefs.h"
#include "imagebitmap.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

////////////////////////////////////////////////////////////////////////////////

extern bool MCImageBitmapToCGImage(MCImageBitmap *p_bitmap, bool p_copy, bool p_invert, CGImageRef &r_image);

void MCPlatformDoDragDrop(MCPlatformWindowRef p_window, MCPlatformAllowedDragOperations p_allowed_operations, MCImageBitmap *p_image, const MCPoint *p_image_loc, MCPlatformDragOperation& r_operation)
{
	CGImageRef t_cg_image;
	t_cg_image = nil;
	if (p_image != nil &&
		!MCImageBitmapToCGImage(p_image, true, false, t_cg_image))
	{
		r_operation = kMCPlatformDragOperationNone;
		return;
	}
	
	if (t_cg_image == nil)
	{
		uint32_t t_pixel;
		t_pixel = 0;
		
		MCImageBitmap t_bitmap;
		t_bitmap . width = 1;
		t_bitmap . height = 1;
		t_bitmap . stride = 4;
		t_bitmap . data = &t_pixel;
		
		if (!MCImageBitmapToCGImage(&t_bitmap, true, false, t_cg_image))
		{
			r_operation = kMCPlatformDragOperationNone;
			return;
		}
	}
	
	NSImage *t_image;
	t_image = [[NSImage alloc] initWithCGImage: t_cg_image size: NSZeroSize];
	CGImageRelease(t_cg_image);
	
	NSSize t_image_loc;
	t_image_loc . width = t_image_loc . height = 0.0f;
	if (p_image_loc != nil)
	{
        // MW-2014-04-22: [[ Bug 12253 ]] Horizontal image offset obviously goes the other way.
		t_image_loc . width = -p_image_loc -> x;
		t_image_loc . height = p_image_loc -> y;
	}
		
	NSDragOperation t_allowed_operations;
	t_allowed_operations = 0;
	if ((p_allowed_operations & kMCPlatformDragOperationCopy) != 0)
		t_allowed_operations |= NSDragOperationCopy;
	if ((p_allowed_operations & kMCPlatformDragOperationMove) != 0)
		t_allowed_operations |= NSDragOperationMove;
	if ((p_allowed_operations & kMCPlatformDragOperationLink) != 0)
		t_allowed_operations |= NSDragOperationLink;
	
	MCMacPlatformSyncMouseBeforeDragging();
	
	// We create a private pasteboard here if the main one contains no items.
    // This is required as the OSX drag-and-drop loop requires a pasteboard to
    // be specified.
    NSPasteboard *t_pboard;
    t_pboard = [NSPasteboard pasteboardWithName: NSDragPboard];
    if ([[t_pboard pasteboardItems] count] == 0)
    {
        NSPasteboardItem *t_item;
        t_item = [[[NSPasteboardItem alloc] init] autorelease];
        [t_item setString: @"" forType: @"com.runrev.livecode.private"];
        
        [t_pboard clearContents];
        [t_pboard writeObjects: [NSArray arrayWithObject: t_item]];
    }
    
	NSDragOperation t_op;
	t_op = [((MCMacPlatformWindow *)p_window) -> GetView() dragImage: t_image offset: t_image_loc allowing: t_allowed_operations pasteboard: t_pboard];
    
	[t_image release];
	
	//MCMacPlatformSyncMouseAfterTracking();
	
	r_operation = MCMacPlatformMapNSDragOperationToDragOperation(t_op);
}

bool MCMacPasteboardConvertTIFFToPNG(MCDataRef p_in_data, MCDataRef& r_out_data)
{
	// Check the data is actually TIFF, it is actually a PNG then do nothing
	// (some versions of SnagIt! put PNG data masquerading as TIFF).
    if (MCDataGetLength(p_in_data) >= 4 && memcmp(MCDataGetBytePtr(p_in_data), "\211PNG", 4) == 0)
    {
        r_out_data = MCValueRetain(p_in_data);
        return true;
    }

	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	// Now we know it isn't PNG data, do the conversion!
	NSData *t_data;
	t_data = [[NSData alloc] initWithBytesNoCopy: (void *)MCDataGetBytePtr(p_in_data) length: MCDataGetLength(p_in_data) freeWhenDone: NO];
	
	NSBitmapImageRep *t_rep;
	t_rep =[[NSBitmapImageRep alloc] initWithData: t_data];
	[t_data release];
	
	NSData *t_out_data;
	t_out_data = [t_rep representationUsingType: NSPNGFileType properties: nil];

    bool t_success = true;
    
    t_success = MCDataCreateWithBytes((const byte_t*)[t_out_data bytes], [t_out_data length], r_out_data);
    
	[t_pool release];
	
    return t_success;
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

MCPlatformDragOperation MCMacPlatformMapNSDragOperationToDragOperation(NSDragOperation p_operation)
{
	switch(p_operation)
	{
		case NSDragOperationNone:
			return kMCPlatformDragOperationNone;
		case NSDragOperationCopy:
			return kMCPlatformDragOperationCopy;
		case NSDragOperationMove:
			return kMCPlatformDragOperationMove;
		case NSDragOperationLink:
			return kMCPlatformDragOperationLink;
		default:
			assert(false);
			break;
	}
	
	return kMCPlatformDragOperationNone;
}

////////////////////////////////////////////////////////////////////////////////
