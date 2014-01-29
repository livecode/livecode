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

#include <QuickTime/QuickTime.h>

////////////////////////////////////////////////////////////////////////////////

struct MCPlatformPasteboard
{
	uint32_t references;
	NSPasteboard *ns_pasteboard;
};

////////////////////////////////////////////////////////////////////////////////

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
	{ @"com.runrev.livecode.objects-1", kMCPlatformPasteboardFlavorObjects, MCMacPasteboardConvertIdentity },
	{ @"com.runrev.livecode.text-styled-1", kMCPlatformPasteboardFlavorStyledText, MCMacPasteboardConvertIdentity },
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
	
#if 0
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
#endif
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
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCPlatformOwnsClipboard(void)
{
	return false;
}

void MCPlatformFlushClipboard(void)
{
}

void MCPlatformGetClipboard(MCPlatformPasteboardRef& r_pasteboard)
{
	MCMacPlatformPasteboardCreate([NSPasteboard generalPasteboard], r_pasteboard);
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
	
	return false;
}

// COCOA-TODO: Use Cocoa stuff for TIFF conversion (if there is any!)
bool MCMacPasteboardConvertTIFFToPNG(const MCString& p_in_data, MCString& r_out_data)
{
	// MW-2010-11-17: [[ Bug 9183 ]] Check the data is actually TIFF, it is actually a PNG then
	//   do nothing (some versions of SnagIt! put PNG data masquerading as TIFF).
	if (p_in_data . getlength() >= 4 && memcmp(p_in_data . getstring(), "\211PNG", 4) == 0)
		return MCMacPasteboardConvertIdentity(p_in_data, r_out_data);

	// Now we know it isn't PNG data, do the conversion!
	
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

////////////////////////////////////////////////////////////////////////////////
