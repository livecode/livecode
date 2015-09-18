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

#include <QuickTime/QuickTime.h>

////////////////////////////////////////////////////////////////////////////////

extern bool MCImageBitmapToCGImage(MCImageBitmap *p_bitmap, bool p_copy, bool p_invert, CGImageRef &r_image);

////////////////////////////////////////////////////////////////////////////////

struct MCPlatformPasteboard
{
	uint32_t references;
	NSPasteboard *ns_pasteboard;
};

////////////////////////////////////////////////////////////////////////////////

#define kMCMacPasteboardObjectsUTString @"com.runrev.livecode.objects-1"
#define kMCMacPasteboardPrivateUTString @"com.runrev.livecode.private"

bool MCMacPasteboardConvertIdentity(const MCString& in_data, MCString& r_out_data);
bool MCMacPasteboardConvertTIFFToPNG(const MCString& in_data, MCString& r_out_data);
bool MCMacPasteboardConvertHTMLToRTF(const MCString& in_data, MCString& r_out_data);
bool MCMacPasteboardConvertFileURLToFiles(const MCString& in_data, MCString& r_out_data);

static struct { NSString *type; MCPlatformPasteboardFlavor flavor; bool (*converter)(const MCString& in_data, MCString& out_data); } s_flavor_mappings[] =
{
	{ (NSString *)kUTTypeUTF8PlainText, kMCPlatformPasteboardFlavorUTF8, MCMacPasteboardConvertIdentity },
	{ (NSString *)kUTTypeRTF, kMCPlatformPasteboardFlavorRTF, MCMacPasteboardConvertIdentity },
	{ (NSString *)kUTTypeHTML, kMCPlatformPasteboardFlavorRTF, MCMacPasteboardConvertHTMLToRTF },
	{ (NSString *)kUTTypeFileURL, kMCPlatformPasteboardFlavorFiles, MCMacPasteboardConvertFileURLToFiles },
	{ (NSString *)kUTTypeJPEG, kMCPlatformPasteboardFlavorJPEG, MCMacPasteboardConvertIdentity },
	{ (NSString *)kUTTypeGIF, kMCPlatformPasteboardFlavorGIF, MCMacPasteboardConvertIdentity },
	{ (NSString *)kUTTypePNG, kMCPlatformPasteboardFlavorPNG, MCMacPasteboardConvertIdentity },
	{ (NSString *)kUTTypeTIFF, kMCPlatformPasteboardFlavorPNG, MCMacPasteboardConvertTIFFToPNG },
	
	// COCOA-TODO: Declare objects UT type so copy/paste objects works between LiveCode instances.
    // PM-2014-04-29: [[Bug 12304]] Updated Revolution-Info.plist to declare objects UT type 
	{ kMCMacPasteboardObjectsUTString, kMCPlatformPasteboardFlavorObjects, MCMacPasteboardConvertIdentity },
	//{ @"com.runrev.livecode.text-styled-1", kMCPlatformPasteboardFlavorStyledText, MCMacPasteboardConvertIdentity },
};

////////////////////////////////////////////////////////////////////////////////

void MCMacPlatformPasteboardCreate(NSPasteboard *p_pasteboard, MCPlatformPasteboardRef& r_pasteboard)
{
	MCPlatformPasteboardRef t_pasteboard;
	/* UNCHECKED */ MCMemoryNew(t_pasteboard);
	
	t_pasteboard -> references = 1;
	t_pasteboard -> ns_pasteboard = p_pasteboard;
	[p_pasteboard retain];
	
	r_pasteboard = t_pasteboard;
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

uindex_t MCPlatformPasteboardGetGeneration(MCPlatformPasteboardRef p_pasteboard)
{
	return [p_pasteboard -> ns_pasteboard changeCount];
}

bool MCPlatformPasteboardQuery(MCPlatformPasteboardRef p_pasteboard, MCPlatformPasteboardFlavor*& r_flavors, uindex_t& r_flavor_count)
{
	// Fetch the array of items.
	NSArray *t_items;
	t_items = [p_pasteboard -> ns_pasteboard pasteboardItems];
	
	// For now we are only interested in the first one.
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
			
			// Loop through the mapping list, adding any flavors which we support for
			// the UT.
			for(uindex_t j = 0; j < sizeof(s_flavor_mappings) / sizeof(s_flavor_mappings[0]); j++)
				if ([t_type isEqualTo: s_flavor_mappings[j] . type])
				{
					bool t_found;
					t_found = false;
					
					for(uindex_t k = 0; k < t_flavor_count; k++)
						if (t_flavors[k] == s_flavor_mappings[j] . flavor)
						{
							t_found = true;
							break;
						}
					
					if (!t_found)
						t_flavors[t_flavor_count++] = s_flavor_mappings[j] . flavor;
				}
		}
	}
	
	r_flavors = t_flavors;
	r_flavor_count = t_flavor_count;
	
	return true;
}

bool MCPlatformPasteboardFetch(MCPlatformPasteboardRef p_pasteboard, MCPlatformPasteboardFlavor p_flavor, void*& r_bytes, uindex_t& r_byte_count)
{
	// Fetch the array of items.
	NSArray *t_items;
	t_items = [p_pasteboard -> ns_pasteboard pasteboardItems];
	
	// If we are requesting files, then handle it specially.
	if (p_flavor == kMCPlatformPasteboardFlavorFiles)
	{
		NSArray *t_urls;
		t_urls = [p_pasteboard -> ns_pasteboard readObjectsForClasses: [NSArray arrayWithObject: [NSURL class]]
															  options: [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES] 
																								   forKey: NSPasteboardURLReadingFileURLsOnlyKey]];
		
		NSMutableString *t_files;
		t_files = [[NSMutableString alloc] init];
		for(uindex_t i = 0; i < [t_urls count]; i++)
			[t_files appendFormat: @"%s%@", i != 0 ? "\n" : "", [(NSURL *)[t_urls objectAtIndex: i] path]];

		r_bytes = strdup([t_files UTF8String]);
		r_byte_count = strlen((char *)r_bytes);

		[t_files release];
	
		return true;
	}
	else
	{
		// For now we are only interested in the first one.
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
		
		// Now loop through our mapping list, trying to find a format that is appropriate.
		for(uindex_t i = 0; i < [t_types count]; i++)
		{
			NSString *t_type;
			t_type = (NSString *)[t_types objectAtIndex: i];

			for(uindex_t j = 0; j < sizeof(s_flavor_mappings) / sizeof(s_flavor_mappings[0]); j++)
				if ([t_type isEqualTo: s_flavor_mappings[j] . type] &&
					p_flavor == s_flavor_mappings[j] . flavor)
				{
					NSData *t_data;
					t_data = [t_item dataForType: t_type];
					
					MCString t_in_data;
					t_in_data . set((const char *)[t_data bytes], [t_data length]);
					
					MCString t_out_data;
					if (!s_flavor_mappings[j] . converter(t_in_data, t_out_data))
						return false;
					
					r_bytes = (void *)t_out_data . getstring();
					r_byte_count = t_out_data . getlength();
					
					return true;
				}
		}
	}
	
	return false;
}

//////////

@interface com_runrev_livecode_MCPasteboardProvider: NSObject<NSPasteboardItemDataProvider>
{
	MCPlatformPasteboardRef m_pasteboard;
	void *m_handle;
}

- (id)initWithPasteboard:(MCPlatformPasteboardRef)pasteboard handle:(void *)handle;
- (void)dealloc;

- (void)pasteboard:(NSPasteboard *)pasteboard item:(NSPasteboardItem *)item provideDataForType:(NSString *)type;
- (void)pasteboardFinishedWithDataProvider:(NSPasteboard *)pasteboard;

@end

@implementation com_runrev_livecode_MCPasteboardProvider

- (id)initWithPasteboard:(MCPlatformPasteboardRef)pasteboard handle:(void *)handle
{
	self = [super init];
	if (self == nil)
		return nil;
		
	m_pasteboard = pasteboard;
	m_handle = handle;
	
	return self;
}

- (void)dealloc
{
	[super dealloc];
}

- (void)pasteboard:(NSPasteboard *)pasteboard item:(NSPasteboardItem *)item provideDataForType:(NSString *)type
{
	MCPlatformPasteboardFlavor t_flavor;
	if ([type isEqualTo: (NSString *)kUTTypeUTF8PlainText])
		t_flavor = kMCPlatformPasteboardFlavorUTF8;
	else if ([type isEqualTo: (NSString *)kUTTypeRTF])
		t_flavor = kMCPlatformPasteboardFlavorRTF;
	else if ([type isEqualTo: (NSString *)kUTTypeHTML])
		t_flavor = kMCPlatformPasteboardFlavorHTML;
	else if ([type isEqualTo: (NSString *)kUTTypeFileURL])
		t_flavor = kMCPlatformPasteboardFlavorFiles;
	else if ([type isEqualTo: (NSString *)kUTTypeJPEG])
		t_flavor = kMCPlatformPasteboardFlavorJPEG;
	else if ([type isEqualTo: (NSString *)kUTTypeGIF])
		t_flavor = kMCPlatformPasteboardFlavorGIF;
	else if ([type isEqualTo: (NSString *)kUTTypePNG])
		t_flavor = kMCPlatformPasteboardFlavorPNG;
	else if ([type isEqualTo: kMCMacPasteboardObjectsUTString])
		t_flavor = kMCPlatformPasteboardFlavorObjects;
	else
	{
		NSLog(@"Unknown pboard type %@", type);
		return;
	}

	void *t_data;
	size_t t_data_size;
	MCPlatformCallbackSendPasteboardResolve(m_pasteboard, t_flavor, m_handle, t_data, t_data_size);
	
	if (t_data != nil)
	{
		NSData *t_ns_data;
		t_ns_data = [[NSData alloc] initWithBytes: t_data length: t_data_size];
		[item setData: t_ns_data forType: type];
		[t_ns_data release];
		
		MCMemoryDeallocate(t_data);
	}
}

- (void)pasteboardFinishedWithDataProvider:(NSPasteboard *)pasteboard
{
}

@end

void MCPlatformPasteboardClear(MCPlatformPasteboardRef p_pasteboard)
{
	[p_pasteboard -> ns_pasteboard clearContents];
}

bool MCPlatformPasteboardStore(MCPlatformPasteboardRef p_pasteboard, MCPlatformPasteboardFlavor *p_flavors, uindex_t p_flavor_count, void *p_handle)
{
	// We handle files specially.
	if (p_flavor_count > 0 && p_flavors[0] == kMCPlatformPasteboardFlavorFiles)
	{
		void *t_data;
		size_t t_data_size;
		MCPlatformCallbackSendPasteboardResolve(p_pasteboard, kMCPlatformPasteboardFlavorFiles, p_handle, t_data, t_data_size);
		
		NSString *t_string;
		t_string = [[NSString alloc] initWithBytes: t_data length: t_data_size encoding: NSUTF8StringEncoding];
		
		NSArray *t_files;
		t_files = [t_string componentsSeparatedByString: @"\n"];
		
		NSMutableArray *t_urls;
		t_urls = [[NSMutableArray alloc] init];
		for(uindex_t i = 0; i < [t_files count]; i++)
		{
			NSURL *t_url;
			t_url = [[NSURL alloc] initFileURLWithPath: (NSString *)[t_files objectAtIndex: i]];
			if (t_url != nil)
				[t_urls addObject: t_url];
			[t_url release];
		}
		
		[p_pasteboard -> ns_pasteboard writeObjects: t_urls];
		[t_urls release];
		
		[t_string release];
	}
	else
	{
		NSPasteboardItem *t_item;
		t_item = [[NSPasteboardItem alloc] init];
		
		com_runrev_livecode_MCPasteboardProvider *t_provider;
		t_provider = [[com_runrev_livecode_MCPasteboardProvider alloc] initWithPasteboard: p_pasteboard handle: p_handle];
		
		NSMutableArray *t_flavor_strings;
		t_flavor_strings = [[NSMutableArray alloc] init];
		for(uindex_t i = 0; i < p_flavor_count; i++)
		{
			NSString *t_flavor_string;
			switch(p_flavors[i])
			{
				case kMCPlatformPasteboardFlavorUTF8:
					t_flavor_string = (NSString *)kUTTypeUTF8PlainText;
					break;
				case kMCPlatformPasteboardFlavorRTF:
					t_flavor_string = (NSString *)kUTTypeRTF;
					break;
				case kMCPlatformPasteboardFlavorHTML:
					t_flavor_string = (NSString *)kUTTypeHTML;
					break;
				case kMCPlatformPasteboardFlavorPNG:
					t_flavor_string = (NSString *)kUTTypePNG;
					break;
				case kMCPlatformPasteboardFlavorJPEG:
					t_flavor_string = (NSString *)kUTTypeJPEG;
					break;
				case kMCPlatformPasteboardFlavorGIF:
					t_flavor_string = (NSString *)kUTTypeGIF;
					break;
				case kMCPlatformPasteboardFlavorObjects:
					t_flavor_string = kMCMacPasteboardObjectsUTString;
					break;
	//			case kMCPlatformPasteboardFlavorStyledText:
	//				t_flavor_string = @"";
	//				break;
				default:
					assert(false);
					break;
			}
			[t_flavor_strings addObject: t_flavor_string];
		}
			
		[t_item setDataProvider: t_provider forTypes: t_flavor_strings];
		[t_flavor_strings release];
		
		[p_pasteboard -> ns_pasteboard writeObjects: [NSArray arrayWithObject: t_item]];
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformGetClipboard(MCPlatformPasteboardRef& r_pasteboard)
{
	MCMacPlatformPasteboardCreate([NSPasteboard generalPasteboard], r_pasteboard);
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformGetDragboard(MCPlatformPasteboardRef& r_dragboard)
{
	MCMacPlatformPasteboardCreate([NSPasteboard pasteboardWithName: NSDragPboard], r_dragboard);
}

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
	
	// MW-2014-06-10: [[ Bug 12388 ]] If the main pboard is empty, then create a private
    //   one and put the empty string as our private UTType. (we store the actual private
    //   data locally elsewhere).
    NSPasteboard *t_pboard;
    bool t_is_private;
    t_is_private = false;
    t_pboard = [NSPasteboard pasteboardWithName: NSDragPboard];
    if ([[t_pboard pasteboardItems] count] == 0)
    {
        t_is_private = true;
        t_pboard = [NSPasteboard pasteboardWithUniqueName];
        
        NSPasteboardItem *t_item;
        t_item = [[[NSPasteboardItem alloc] init] autorelease];
        [t_item setString: @"" forType: kMCMacPasteboardPrivateUTString];
        
        [t_pboard clearContents];
        [t_pboard writeObjects: [NSArray arrayWithObject: t_item]];
    }
    
	NSDragOperation t_op;
	t_op = [((MCMacPlatformWindow *)p_window) -> GetView() dragImage: t_image offset: t_image_loc allowing: t_allowed_operations pasteboard: t_pboard];
	
    if (t_is_private)
        [t_pboard releaseGlobally];
    
	[t_image release];
	
	//MCMacPlatformSyncMouseAfterTracking();
	
	r_operation = MCMacPlatformMapNSDragOperationToDragOperation(t_op);
}

////////////////////////////////////////////////////////////////////////////////

bool MCMacPasteboardConvertIdentity(const MCString& p_in_data, MCString& r_out_data)
{
	r_out_data . set((const char *)memdup(p_in_data . getstring(), p_in_data . getlength()), p_in_data . getlength());
	return true;
}

bool MCMacPasteboardConvertHTMLToRTF(const MCString& p_in_data, MCString& r_out_data)
{
	// SN-2013-07-26: [[ Bug 10893 ]] Convert HTML to RTF using Apple's internal class
    NSData *t_html_data;
    t_html_data = [[NSData alloc] initWithBytes: p_in_data . getstring() length: p_in_data . getlength()];
    
    NSAttributedString *t_html_string;
    t_html_string = [[NSAttributedString alloc] initWithHTML: t_html_data documentAttributes: nil];
    
    NSData *t_rtf_data;
    t_rtf_data = [t_html_string RTFFromRange: NSMakeRange(0, [t_html_string length]) documentAttributes: nil];
	
    [t_html_string release];
    [t_html_data release];
	
	r_out_data . set((const char *)memdup([t_rtf_data bytes], [t_rtf_data length]), [t_rtf_data length]);
	
	return true;
}

bool MCMacPasteboardConvertTIFFToPNG(const MCString& p_in_data, MCString& r_out_data)
{
	// MW-2010-11-17: [[ Bug 9183 ]] Check the data is actually TIFF, it is actually a PNG then
	//   do nothing (some versions of SnagIt! put PNG data masquerading as TIFF).
	if (p_in_data . getlength() >= 4 && memcmp(p_in_data . getstring(), "\211PNG", 4) == 0)
		return MCMacPasteboardConvertIdentity(p_in_data, r_out_data);

	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	// Now we know it isn't PNG data, do the conversion!
	NSData *t_data;
	t_data = [[NSData alloc] initWithBytesNoCopy: (void *)p_in_data . getstring() length: p_in_data . getlength() freeWhenDone: NO];
	
	NSBitmapImageRep *t_rep;
	t_rep =[[NSBitmapImageRep alloc] initWithData: t_data];
	[t_data release];
	
	NSData *t_out_data;
	t_out_data = [t_rep representationUsingType: NSPNGFileType properties: nil];

	r_out_data . set((const char *)memdup([t_out_data bytes], [t_out_data length]), [t_out_data length]);
	
	[t_pool release];
	
	return true;
	
#ifdef QUICKTIME
	bool t_success;
	t_success = true;
	
	GraphicsImportComponent t_importer;
	t_importer = 0;
	if (t_success)
	{
		if (OpenADefaultComponent(GraphicsImporterComponentType, kQTFileTypeTIFF, &t_importer) != noErr)
			t_success = false;
	}
	
	GraphicsExportComponent t_exporter;
	t_exporter = 0;
	if (t_success)
	{
		if (OpenADefaultComponent(GraphicsExporterComponentType, kQTFileTypePNG, &t_exporter) != noErr)
			t_success = false;
	}
	
	Handle t_input_dataref_handle;
	t_input_dataref_handle = NULL;
	if (t_success)
	{
		PointerDataRefRecord t_dataref;
		t_dataref . data = (void *)p_in_data . getstring();
		t_dataref . dataLength = p_in_data . getlength();
		if (PtrToHand(&t_dataref, &t_input_dataref_handle, sizeof(PointerDataRefRecord)) != noErr)
			t_success = false;
	}
	
	if (t_success)
	{
		if (GraphicsImportSetDataReference(t_importer, t_input_dataref_handle, 'ptr ') != noErr)
			t_success = false;
	}
	
	Handle t_output_handle;
	t_output_handle = NULL;
	if (t_success)
	{
		t_output_handle = NewHandle(0);
		if (t_output_handle == NULL)
			t_success = false;
	}
	
	if (t_success)
	{
		if (GraphicsExportSetInputGraphicsImporter(t_exporter, t_importer) != noErr)
			t_success = false;
	}
	
	if (t_success)
	{
		if (GraphicsExportSetOutputHandle(t_exporter, t_output_handle) != noErr)
			t_success = false;
	}
	
	if (t_success)
	{
		if (GraphicsExportDoExport(t_exporter, NULL) != noErr)
			t_success = false;
	}
	
	MCSharedString *t_result;
	t_result = NULL;
	if (t_success)
	{
		HLock(t_output_handle);
		r_out_data . set((const char *)memdup(*t_output_handle, GetHandleSize(t_output_handle)), GetHandleSize(t_output_handle));
		HUnlock(t_output_handle);
	}
	
	if (t_output_handle != NULL)
		DisposeHandle(t_output_handle);
	
	if (t_input_dataref_handle != NULL)
		DisposeHandle(t_input_dataref_handle);
	
	if (t_exporter != 0)
		CloseComponent(t_exporter);
	
	if (t_importer != 0)
		CloseComponent(t_importer);
	
	return t_result;
#endif
}

bool MCMacPasteboardConvertFileURLToFiles(const MCString& p_in_data, MCString& r_out_data)
{
	return MCMacPasteboardConvertIdentity(p_in_data, r_out_data);
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
