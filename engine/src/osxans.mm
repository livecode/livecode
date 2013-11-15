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

//
// MAC plaform specific routines for ask and answer
//

#include "osxprefix.h"

#include "core.h"
#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "execpt.h"
#include "mcerror.h"
#include "util.h"
#include "ans.h"
#include "objdefs.h"
#include "stack.h"
#include "stacklst.h"
#include "osspec.h"

#include "globals.h"
#include "dispatch.h"
#include "card.h"

#include "printer.h"
#include "osxprinter.h"

#include "meta.h"
#include "mode.h"

#include "osxdc.h"

#include <Cocoa/Cocoa.h>

////////////////////////////////////////////////////////////////////////////////

extern void MCRemoteFileDialog(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char * const p_types[], uint32_t p_type_count, const char *p_initial_folder, const char *p_initial_file, bool p_save, bool p_files);
extern void MCRemoteFolderDialog(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_initial);
extern void MCRemoteColorDialog(MCExecPoint& ep, const char *p_title, uint32_t p_r, uint32_t p_g, uint32_t p_b);

////////////////////////////////////////////////////////////////////////////////
// MM-2012-08-30: [[ Bug 10293 ]] Reinstate old file dialog code for tiger

int MCA_file_tiger(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options);
int MCA_file_with_types_tiger(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options);
int MCA_ask_file_tiger(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options);
int MCA_ask_file_with_types_tiger(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options);
int MCA_folder_tiger(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_initial, unsigned int p_options);

////////////////////////////////////////////////////////////////////////////////

struct MCFileFilter
{
	MCFileFilter *next;
	char *tag;
	char **extensions;
	uint32_t extension_count;
	char **filetypes;
	uint32_t filetypes_count;
};


static void MCFileFilterDestroy(MCFileFilter *self)
{
	for(uint32_t i = 0; i < self -> extension_count; i++)
		MCCStringFree(self -> extensions[i]);
	MCMemoryDeleteArray(self -> extensions);
	for(uint32_t i = 0; i < self -> filetypes_count; i++)
		MCCStringFree(self -> filetypes[i]);
	MCMemoryDeleteArray(self -> filetypes);
	MCCStringFree(self -> tag);
	MCMemoryDelete(self);
}

static bool MCFileFilterCreate(const char *p_desc, MCFileFilter*& r_filter)
{
	bool t_success;
	t_success = true;
	
	MCFileFilter *self;
	self = nil;
	if (t_success)
		t_success = MCMemoryNew(self);
	
	char **t_items;
	uint32_t t_item_count;
	t_items = nil;
	t_item_count = 0;
	if (t_success)
		t_success = MCCStringSplit(p_desc, '|', t_items, t_item_count);
	
	// MM-2012-03-09: [[Bug]] Make sure we don't try and copy empty tags (causes a crash on Lion)
	if (t_success)
		if (MCCStringLength(t_items[0]) > 0)
			t_success = MCCStringClone(t_items[0], self -> tag);
	
	if (t_success)
	{
		if (t_item_count < 2)
			t_success =
			MCCStringSplit("*", ',', self -> extensions, self -> extension_count) &&
			MCCStringSplit("*", ',', self -> filetypes, self -> filetypes_count);
		else
		{
			t_success = MCCStringSplit(t_items[1], ',', self -> extensions, self -> extension_count);
			if (t_item_count > 2)
				t_success = MCCStringSplit(t_items[2], ',', self -> filetypes, self -> filetypes_count);
		}
	}
	
	// MM 2012-10-05: [[ Bug 10409 ]] Make sure filters of the form "All|" and "All||" are treated as wildcards
	//   (but not filters of the form "Stacks||RSTK").
	if (t_success)
		if (self -> extension_count == 1 && self -> extensions[0][0] == '\0' && (t_item_count == 2 || (self -> filetypes_count == 1 && self -> filetypes[0][0] == '\0')))
		{
			/* UNCHECKED */ MCCStringFree(self -> extensions[0]);
			/* UNCHECKED */ MCCStringClone("*", self -> extensions[0]);
		}
		
	if (t_success)
		r_filter = self;
	else
		MCFileFilterDestroy(self);
	
	for(uint32_t i = 0; i < t_item_count; i++)
		MCCStringFree(t_items[i]);
	MCMemoryDeleteArray(t_items);
	
	return t_success;
}

static bool hfs_code_to_string(unsigned long p_code, char *r_string)
{
	if (p_code != 0) {
		r_string[4] = '\0';
		r_string[3] = p_code;
		r_string[2] = p_code >> 8;
		r_string[1] = p_code >> 16;
		r_string[0] = p_code >> 24;
		return true;
	}
	r_string[0] =  '\0';
	return false;
}

static void resolve_alias(NSString *p_path, NSString *&r_path_resolved)
{	
	CFURLRef t_url;
	t_url = (CFURLRef) [NSURL fileURLWithPath: p_path];
		
	Boolean t_is_folder;
	t_is_folder = False;	
	Boolean t_is_alias;
	t_is_alias = False;
	FSRef t_fs_ref;
	
	// MM-2012-10-05: [[ Bug 10427 ]] Check to see if path is an alias before resolving. Should solve hangs related to unmounted network drives.
	if (t_url != nil)
		if (CFURLGetFSRef(t_url, &t_fs_ref))
			if (FSIsAliasFile(&t_fs_ref, &t_is_alias, &t_is_folder) != noErr)
				t_is_alias = False;
	
	CFStringRef t_path_resolved;
	t_path_resolved = nil;
	
	if (t_is_alias)
		if (FSResolveAliasFileWithMountFlags(&t_fs_ref, true, &t_is_folder, &t_is_alias, kResolveAliasFileNoUI) == noErr)
		{
			CFURLRef t_url_resolved;
			t_url_resolved = CFURLCreateFromFSRef(nil, &t_fs_ref);
			if (t_url_resolved != nil)
			{
				t_path_resolved = CFURLCopyFileSystemPath(t_url_resolved, kCFURLPOSIXPathStyle);
				CFRelease(t_url_resolved);
			}
		}		
	
	if (t_path_resolved != nil)	
		r_path_resolved = (NSString *) t_path_resolved;
	else
		r_path_resolved = [[NSString alloc] initWithString: p_path];
}

static char *folder_path_from_intial_path(const char *p_path)
{
	char *t_path;
	t_path = nil;
	if (strrchr(p_path, '/') != nil)
	{
		if (*p_path != '/')
			t_path = MCS_resolvepath(p_path);
		else
		/* UNCHECKED */ MCCStringClone(p_path, t_path);
	}
	
	char *t_folder;
	t_folder = nil;
	if (t_path != nil)
	{
		if (MCS_exists(t_path, false))
		/* UNCHECKED */ MCCStringClone(t_path, t_folder);
		else {
			char *t_last_slash;
			t_last_slash = strrchr(t_path, '/');
			if (t_last_slash != nil)
			/* UNCHECKED */ MCCStringCloneSubstring(t_path, t_last_slash - t_path, t_folder);
		}
	}
	
	/* UNCHECKED */ MCCStringFree(t_path);
	return t_folder;	
}

static bool filter_to_type_list(const char *p_filter, char ***r_types, uint32_t &r_type_count)
{
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = p_filter != nil;
	
	uint32_t t_filter_length;
	if (t_success)
	{
		t_filter_length = MCCStringLength(p_filter);
		t_success = t_filter_length >= 4;
	}
	
	char *t_types;
	t_types = nil;
	if (t_success)
		t_success = MCCStringClone("||", t_types);
	
	if (t_success)
	{
		const char *t_current_type;
		t_current_type = p_filter;
		for (uint32_t i = 0; t_success && i < t_filter_length / 4; i++)
		{
			if (i > 0)
				t_success = MCCStringAppend(t_types, ",");
			if (t_success)
			{
				t_success = MCCStringAppendFormat(t_types, "%c%c%c%c", t_current_type[0], t_current_type[1], t_current_type[2], t_current_type[3]);
				t_current_type += 4;
			}
		}
	}
	
	if (t_success)
		t_success = MCMemoryNewArray(1, *r_types);
	
	if (t_success)
	{
		*r_types[0] = t_types;
		r_type_count = 1;
	}
	else
	{
		r_type_count = 0;
		/* UNCHECKED */ MCCStringFree(t_types);
	}		
	
	return t_success;	
}

////////////////////////////////////////////////////////////////////////////////

@interface FileDialogAccessoryView : NSView
{
	NSTextField *m_label;
	NSPopUpButton *m_options;
	
	MCFileFilter *m_filters;
	MCFileFilter *m_filter;
	uint32_t m_filter_index;
}

- (id)init;
- (void)dealloc;
- (void)finalize;

- (void)setLabel: (NSString *)newLabel;
- (void)setTypes: (char * const *)p_types length: (uint32_t)p_count;

- (void)typeChanged: (id)sender;

- (const char *)currentType;

- (BOOL)panel:(id)sender shouldShowFilename:(NSString *)filename;

@end

@implementation FileDialogAccessoryView

- (id)init
{
	self = [ super init ];
	
	m_label = [ [ NSTextField alloc ] initWithFrame: NSMakeRect(0, 9, 61, 17) ];
	[ m_label setEditable: NO ];
	[ m_label setSelectable: NO ];
	[ m_label setBordered: NO ];
	[ m_label setAlignment: NSRightTextAlignment ];
	[ m_label setDrawsBackground: NO ];
	
	m_options = [ [ NSPopUpButton alloc ] initWithFrame: NSMakeRect(63, 2, 250, 26) ];
	[ m_options setTarget: self ];
	[ m_options setAction: @selector(typeChanged:) ];
	[ m_options setAutoenablesItems: NO ];
	
	[ self addSubview: m_label ];
	[ self addSubview: m_options ];
	
	[ self setFrameSize: NSMakeSize(250 + 63, 30) ];
	
	m_filters = nil;
	m_filter = nil;
	m_filter_index = 0;
	
	return self;
}

- (void)dealloc
{
	[ m_label release ];
	[ m_options release ];
	[ super dealloc ];
}

- (void)finalize
{
	while(m_filters != nil)
		MCFileFilterDestroy(MCListPopFront(m_filters));
}

- (void)setLabel: (NSString *)newLabel
{
	[ m_label setStringValue: newLabel ];
}

- (void)setTypes: (const char **)p_types length: (uint32_t)p_count
{
	for(uint32_t i = 0; i < p_count; i++)
	{
		MCFileFilter *t_filter;
		if (MCFileFilterCreate([[NSString stringWithCString: p_types[i] encoding: NSMacOSRomanStringEncoding] UTF8String], t_filter))
		{
			MCListPushBack(m_filters, t_filter);
			if (t_filter -> tag != nil)
				[ m_options addItemWithTitle: [ NSString stringWithUTF8String: t_filter -> tag ]];
		}
	}
	m_filter = m_filters;
}

- (void)typeChanged: (id)sender
{
	uint32_t t_new_index;
	t_new_index =  [[ m_options objectValue ] intValue ];
	
	m_filter = m_filters;
	for(uint32_t i = 0; i < t_new_index; i++)
		m_filter = m_filter -> next;
	
	[[self window] validateVisibleColumns];
}

- (const char *)currentType
{
	if (m_filter != nil)
		return m_filter -> tag;
	return nil;
}

// MM-2012-02-28: [[BUG 10003]] Make sure we filter out files appropriately
- (BOOL)panel:(id)sender shouldShowFilename:(NSString *)filename
{
	// No filters to filter against
	if (m_filters == nil || (m_filter->extension_count == 0 && m_filter->filetypes_count == 0))
		return YES;
	
	// If any of the filters are * (ext or file type) then we allow all files
	for (uint32_t i = 0; i < m_filter->extension_count; i++)
		if (MCCStringEqual("*", m_filter->extensions[i]))
			return YES;
	for (uint32_t i = 0; i < m_filter->filetypes_count; i++)
		if (MCCStringEqual("*", m_filter->filetypes[i]))
			return YES;				
	
	// MM-2012-09-25: [[ Bug 10407 ]] Filter on the attirbutes of the target of the alias, rather than the alias.
	NSString *t_filename_resolved;
	resolve_alias(filename, t_filename_resolved);
	
	NSDictionary *t_attr;
	t_attr = [[NSFileManager defaultManager] fileAttributesAtPath: t_filename_resolved traverseLink: YES];	
	if (t_attr == nil)
	{
		[t_filename_resolved release];
		return NO;
	}
	
	BOOL t_should_show;
	t_should_show = NO;
	
	// We always display folders so we can navigate into them but not packages (apps, bundles etc)
	if (([NSFileTypeDirectory isEqualTo:[t_attr objectForKey:NSFileType]] 
		&& ![[NSWorkspace sharedWorkspace] isFilePackageAtPath: filename]))
		t_should_show = YES;
	
	// Check to see if the extension of the file matches any of those in the extension list
	if (!t_should_show && m_filter->extension_count > 0)
	{
		const char *t_filename;
		t_filename = [filename cStringUsingEncoding: NSMacOSRomanStringEncoding];
		if (t_filename != nil)
		{
			char *t_ext;
			t_ext = strrchr(t_filename, '.');
			if (t_ext != nil && MCCStringLength(t_ext) > 0)
				for (uint32_t i = 0; i < m_filter->extension_count && !t_should_show; i++)
					if (MCCStringLength(m_filter->extensions[i]) > 0 && MCCStringEqualCaseless(t_ext + 1, m_filter->extensions[i]))
						t_should_show = YES;
		}
	}
	
	// Check the various HFS codes to see if they match any of the types in the filter
	if (!t_should_show && m_filters->filetypes_count > 0)
	{
		// For regular files, extract any type and creator codes - these are four character codes e.g. REVO
		char t_type[5];
		hfs_code_to_string([[t_attr objectForKey: NSFileHFSTypeCode] unsignedLongLongValue], t_type);
		char t_creator[5];
		hfs_code_to_string([[t_attr objectForKey: NSFileHFSCreatorCode] unsignedLongLongValue], t_creator);
		
		// For bundles, extract any type codes form the plist
		// MW-2012-03-08: [[ Bug ]] Seems the keys can be empty, so if they are revert to 'unknown'
		//   sig / type.
		NSString *t_bundle_sig;
		t_bundle_sig = [[[NSBundle bundleWithPath: filename] infoDictionary] objectForKey: @"CFBundleSignature"];
		if (t_bundle_sig == nil)
			t_bundle_sig = @"????";
		NSString *t_bundle_type;
		t_bundle_type = [[[NSBundle bundleWithPath: filename] infoDictionary] objectForKey: @"CFBundlePackageType"];
		if (t_bundle_type == nil)
			t_bundle_type = @"????";
		
		for (uint32_t i = 0; i < m_filter->filetypes_count && !t_should_show; i++)
			if (MCCStringLength(m_filter->filetypes[i]) > 0 &&
				(MCCStringEqualCaseless(t_type, m_filter->filetypes[i]) || 
				 MCCStringEqualCaseless(t_creator, m_filter->filetypes[i]) ||
				 MCCStringEqualCaseless([t_bundle_sig cStringUsingEncoding: NSMacOSRomanStringEncoding], m_filter->filetypes[i]) ||
				 MCCStringEqualCaseless([t_bundle_type cStringUsingEncoding: NSMacOSRomanStringEncoding], m_filter->filetypes[i])))				
				t_should_show = YES;
	}
	
	[t_filename_resolved release];	
	return t_should_show;
}

@end

////////////////////////////////////////////////////////////////////////////////

static bool s_dialog_dismissed = false;
static int s_dialog_result = 0;

@interface DialogDelegate : NSObject
- (void) panelDidEnd:(NSSavePanel *)savePanel returnCode:(int)returnCode conextInfo:(void *)contextInfo;
@end

@implementation DialogDelegate
- (void) panelDidEnd:(NSSavePanel *)savePanel returnCode:(int)returnCode conextInfo:(void *)contextInfo
{
	s_dialog_result = returnCode;
	s_dialog_dismissed = true;
}
@end

void *MCCreateNSWindow(void *);

static int display_modal_dialog(MCExecPoint &ep, NSSavePanel *p_panel, const char *p_initial_folder, const char *p_initial_file, bool p_as_sheet)
{	
	NSString *t_initial_folder;
	t_initial_folder = nil;
	if (p_initial_folder != nil)
		t_initial_folder = [NSString stringWithCString: p_initial_folder encoding: NSMacOSRomanStringEncoding];
	
	NSString *t_initial_file;
	t_initial_file = nil;
	if (p_initial_file != nil)
		t_initial_file = [NSString stringWithCString: p_initial_file encoding: NSMacOSRomanStringEncoding];			
	
	if (p_as_sheet)
	{
        // MM-2012-04-02: [[ Bug 10136 ]] Cocoa/carbon window incompatibility.
		NSWindow *t_window;
		t_window = (NSWindow *) MCCreateNSWindow(MCtopstackptr->getwindow()->handle.window);
		
		DialogDelegate *t_delegate;
		t_delegate = [[DialogDelegate alloc] init];
				
		[p_panel beginSheetForDirectory: t_initial_folder
									file: t_initial_file
						  modalForWindow: t_window
						   modalDelegate: t_delegate
						  didEndSelector: @selector(panelDidEnd:returnCode:conextInfo:)
							 contextInfo: nil];
		
		s_dialog_dismissed = false;
		while(!s_dialog_dismissed)
			MCscreen -> wait(1.0, True, True);
		[t_delegate release];
		return s_dialog_result;
	}
	else
		// MM-2012-03-16: [[ Bug ]] Use runModalForDirectory:file: rather than setting directory and file - methods only introduced in 10.6
		return [p_panel runModalForDirectory: t_initial_folder file: t_initial_file];
}

////////////////////////////////////////////////////////////////////////////////

// MM-2012-02-13: Updated to use Cocoa APIs.
int MCA_do_file_dialog(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options)
{
    bool t_is_save;
    t_is_save = (p_options & MCA_OPTION_SAVE_DIALOG) != 0;
    bool t_is_plural;
    t_is_plural = (p_options & MCA_OPTION_PLURAL) != 0;
				
    if (!MCModeMakeLocalWindows())
	{
		char *t_resolved_initial;
		t_resolved_initial = MCS_resolvepath(p_initial);		
        MCRemoteFileDialog(ep, p_title, p_prompt, p_types, p_type_count, nil, t_resolved_initial, t_is_save, t_is_plural);
		/* UNCHECKED */ MCCStringFree(t_resolved_initial);
	}
    else
    {
		char *t_initial_folder;
		t_initial_folder = nil;
		if (p_initial != nil)
			t_initial_folder = folder_path_from_intial_path(p_initial);
		
		const char *t_initial_file;
		t_initial_file = nil;	
		if (t_is_save && p_initial != nil && !MCS_exists(p_initial, false))
		{
			char *t_last_slash;
			t_last_slash = strrchr(p_initial, '/');
			if (t_last_slash != nil)
				t_initial_file = strrchr(p_initial, '/') + 1;
			else
				t_initial_file = p_initial;
		}		
		
        // MW-2007-12-21: [[ Bug 4548 ]] Make sure the cursor doesn't stay hidden on showing a system dialog!
        Cursor c;
        SetCursor(GetQDGlobalsArrow(&c));
        ShowCursor();
		
        FileDialogAccessoryView *t_accessory;
        t_accessory = [[FileDialogAccessoryView alloc] init];
        [t_accessory setTypes: p_types length: p_type_count];
		
        char *t_filename;
        t_filename = nil;
        char *t_type;
        t_type = nil;
		
		NSSavePanel *t_panel;
		t_panel = (t_is_save) ? [NSSavePanel savePanel] : [NSOpenPanel openPanel] ;
		
		if (p_title != nil && MCCStringLength(p_title) != 0)
		{
			[t_panel setTitle: [NSString stringWithCString: p_title encoding: NSMacOSRomanStringEncoding]];
			[t_panel setMessage: [NSString stringWithCString: p_prompt encoding: NSMacOSRomanStringEncoding]];
		}
		else
			[t_panel setTitle: [NSString stringWithCString: p_prompt encoding: NSMacOSRomanStringEncoding]];
		[t_panel setDelegate: t_accessory];
		
		if (p_type_count > 1)
		{
			[t_accessory setLabel: @"Format:"];
			[t_panel setAccessoryView: t_accessory];
		}
		
		if (!t_is_save)
		{
			[t_panel setCanChooseFiles: YES];
			[t_panel setCanChooseDirectories: NO];
			[t_panel setAllowsMultipleSelection: t_is_plural ? YES : NO];
		}
		// MM-2012-03-01: [[ BUG 10046]] Make sure the "new folder" button is enabled for save dialogs
		else 
			[t_panel setCanCreateDirectories: YES];
		
		if (display_modal_dialog(ep, t_panel, t_initial_folder, t_initial_file, (p_options & MCA_OPTION_SHEET) != 0) == NSOKButton)
		{
			if (t_is_save)
			{
				MCCStringToNative([[t_panel filename] UTF8String], t_filename);
				if ([t_accessory currentType] != nil)
					/* UNCHECKED */ MCCStringToNative([t_accessory currentType], t_type);
			}
			else
			{
				for(uint32_t i = 0; i < [[t_panel filenames] count ]; i++)
				{
					// MM-2012-09-25: [[ Bug 10407 ]] Resolve alias (if any) of the returned files.
					NSString *t_alias;
					resolve_alias([[t_panel filenames] objectAtIndex: i], t_alias);					
					char *t_conv_filename;
					t_conv_filename = nil;					
					if (MCCStringToNative([t_alias UTF8String], t_conv_filename))
						/* UNCHECKED */ MCCStringAppendFormat(t_filename, "%s%s", i > 0 ? "\n" : "", t_conv_filename);
					/* UNCHECKED */ MCCStringFree(t_conv_filename);
					[t_alias release];
				}
				if ([t_accessory currentType] != nil)
					/* UNCHECKED */ MCCStringToNative([t_accessory currentType], t_type);				
			}
		}
		
        if (MCCStringLength(t_filename) == 0)
            ep.clear();
        else
        {
            ep.copysvalue(t_filename, MCCStringLength(t_filename));
            if (p_options & MCA_OPTION_RETURN_FILTER && p_type_count > 0)
                MCresult->copysvalue(t_type);
        }
		
		/* UNCHECKED */ MCCStringFree(t_initial_folder);
        /* UNCHECKED */ MCCStringFree(t_filename);
        /* UNCHECKED */ MCCStringFree(t_type);
        [t_accessory release];
    }
		
    return noErr;
}

int MCA_file(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options)
{
	if (MCmajorosversion > 0x1040)
	{
		int t_result;	
		
		// MW-2012-03-27: [[ Bug ]] Make sure we initialize t_types to nil.
		char **t_types;
		uint32_t t_type_count;	
		t_types = nil;
		filter_to_type_list(p_filter, &t_types, t_type_count);	
		t_result = MCA_do_file_dialog(ep, p_title == NULL ? "" : p_title, p_prompt == NULL ? "" : p_prompt, t_types, t_type_count, p_initial, p_options);
		/* UNCHECKED */ MCMemoryDeleteArray(t_types);
		return t_result;
		
	}
	else
		return MCA_file_tiger(ep, p_title, p_prompt, p_filter, p_initial, p_options);
}

int MCA_file_with_types(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options)
{
	if (MCmajorosversion > 0x1040)
	{
		int t_result;
		t_result = MCA_do_file_dialog(ep, p_title == NULL ? "" : p_title, p_prompt == NULL ? "" : p_prompt, p_types, p_type_count, p_initial, p_options | MCA_OPTION_RETURN_FILTER);
		return t_result;
	}
	else
		return MCA_file_with_types_tiger(ep, p_title, p_prompt, p_types, p_type_count, p_initial, p_options);
}

int MCA_ask_file(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options)
{
	if (MCmajorosversion > 0x1040)
	{
		int t_result;
		char **t_types;
		uint32_t t_type_count;
		t_types = nil;
		filter_to_type_list(p_filter, &t_types, t_type_count);
		t_result = MCA_do_file_dialog(ep, p_prompt == NULL ? "" : p_prompt, "", t_types, t_type_count, p_initial == NULL ? "" : p_initial, p_options | MCA_OPTION_SAVE_DIALOG);
		/* UNCHECKED */ MCMemoryDeleteArray(t_types);
		return t_result;
	}
	else
		return MCA_ask_file_tiger(ep, p_title, p_prompt, p_filter, p_initial, p_options);
}

int MCA_ask_file_with_types(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options)
{
	if (MCmajorosversion > 0x1040)
	{
		int t_result;
		t_result = MCA_do_file_dialog(ep, p_title == NULL ? "" : p_title, p_prompt == NULL ? "" : p_prompt, p_types, p_type_count, p_initial == NULL ? "" : p_initial, p_options | MCA_OPTION_RETURN_FILTER | MCA_OPTION_SAVE_DIALOG);
		return t_result;
	}
	else
		return MCA_ask_file_with_types_tiger(ep, p_title, p_prompt, p_types, p_type_count, p_initial, p_options);
}

////////////////////////////////////////////////////////////////////////////////

// MM-2012-02-13: Updated to use Cocoa APIs.  Code mostly cribbed from plugin dialog stuff
int MCA_folder(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_initial, unsigned int p_options)
{
	if (MCmajorosversion > 0x1040)
	{
		if (!MCModeMakeLocalWindows())
		{
			char *t_resolved_initial_path;
			t_resolved_initial_path = MCS_resolvepath(p_initial);
			MCRemoteFolderDialog(ep, p_title, p_prompt, t_resolved_initial_path);
			/* UNCHECKED */ MCCStringFree(t_resolved_initial_path);
		}
		else
		{
			char *t_initial_folder;
			t_initial_folder = nil;
			if (p_initial != nil)
				t_initial_folder = folder_path_from_intial_path(p_initial);
			
			char *t_folder;
			t_folder = nil;
			
			NSOpenPanel *t_choose;
			t_choose = [NSOpenPanel openPanel];
			if (p_title != nil && MCCStringLength(p_title) != 0)
			{
				[t_choose setTitle: [NSString stringWithCString: p_title encoding: NSMacOSRomanStringEncoding]];
				[t_choose setMessage: [NSString stringWithCString:p_prompt encoding: NSMacOSRomanStringEncoding]];
			}
			else
				[t_choose setTitle: [NSString stringWithCString: p_prompt encoding: NSMacOSRomanStringEncoding]];
			[t_choose setPrompt: @"Choose"];
			[t_choose setCanChooseFiles: NO];
			[t_choose setCanChooseDirectories: YES];
			[t_choose setAllowsMultipleSelection: NO];
			
			// MM-2012-03-01: [[ BUG 10046]] Make sure the "new folder" button is enabled for folder dialogs
			[t_choose setCanCreateDirectories: YES];
			
			if (display_modal_dialog(ep, t_choose, t_initial_folder, nil, (p_options & MCA_OPTION_SHEET) != 0) == NSOKButton)
			{
				// MM-2012-09-25: [[ Bug 10407 ]] Resolve alias (if any) of the returned folder
				NSString *t_alias;
				resolve_alias([t_choose filename], t_alias);									
				/* UNCHECKED */ MCCStringToNative([t_alias UTF8String], t_folder);
				[t_alias release];
			}
			
			// Send results back
			if (MCCStringLength(t_folder) == 0)
				ep.clear();
			else
				ep.copysvalue(t_folder, MCCStringLength(t_folder));
			
			// Free the folder
			/* UNCHECKED */ MCCStringFree(t_folder);
			/* UNCHECKED */ MCCStringFree(t_initial_folder);		
		}
		
		return noErr;
	}
	else
		return MCA_folder_tiger(ep, p_title, p_prompt, p_initial, p_options);
}

////////////////////////////////////////////////////////////////////////////////

// MW-2005-05-23: Update to new API
// MW-2008-03-28: [[ Bug 6248 ]] Updated to use Generic RGB Profile and new NPickColor API to
//   make sure we don't get unpleasant color drift.
int MCA_color(MCExecPoint& ep, const char *p_title, const char *p_initial, Boolean p_as_sheet)
{
	uint32_t t_red, t_green, t_blue;
	if (p_initial == NULL)
	{
		t_red = MCpencolor.red;
		t_green = MCpencolor.green;
		t_blue = MCpencolor.blue;
	}
	else
	{
		char *cname = NULL;
		MCColor c;
		MCscreen->parsecolor(p_initial, &c, &cname);
		delete cname;
		t_red = c.red;
		t_green = c.green;
		t_blue = c.blue;
	}
	
	if (!MCModeMakeLocalWindows())
	{
		MCRemoteColorDialog(ep, p_title, t_red >> 8, t_green >> 8, t_blue >> 8);
		return 0;
	}

	Cursor c;
	SetCursor(GetQDGlobalsArrow(&c)); // bug in OS X doesn't reset this
	ShowCursor();
	MCscreen->expose();
	NColorPickerInfo theColorInfo;
	memset(&theColorInfo, 0, sizeof(theColorInfo));
	theColorInfo.placeWhere = kCenterOnMainScreen;
	pStrcpy(theColorInfo.prompt, "\pChoose a color:");

	CMProfileLocation t_location;
	t_location . locType = cmPathBasedProfile;
	strcpy(t_location . u . pathLoc . path, "/System/Library/ColorSync/Profiles/Generic RGB Profile.icc");
	
	OSErr t_err;
	CMProfileRef t_icc_profile;
	t_err = CMOpenProfile(&t_icc_profile, &t_location);
	if (t_err != noErr)
		t_icc_profile = NULL;

	theColorInfo.theColor.color.rgb.red = t_red;
	theColorInfo.theColor.color.rgb.green = t_green;
	theColorInfo.theColor.color.rgb.blue = t_blue;

	theColorInfo . theColor . profile = t_icc_profile;
	
	if (NPickColor(&theColorInfo) == noErr && theColorInfo.newColorChosen)
	{
		ep.setcolor(theColorInfo.theColor.color.rgb.red  >> 8, theColorInfo.theColor.color.rgb.green >> 8, theColorInfo.theColor.color.rgb.blue >> 8);
		MCresult->clear(False);
	}
	else
	{
		ep.clear();
		MCresult->sets(MCcancelstring);
	}
	
	if (t_icc_profile != NULL)
		CMCloseProfile(t_icc_profile);
	
	return 0;
}


// MERG-2013-08-18: Stubs for colorDialogColors. Possibly implement when color dialog moves to Cocoa
void MCA_setcolordialogcolors(MCExecPoint& p_ep)
{
    
}

void MCA_getcolordialogcolors(MCExecPoint& p_ep)
{
	p_ep.clear();
}

////////////////////////////////////////////////////////////////////////////////
// MM-2012-04-02: Added new MC*Window functions which wrap the corresponding cocoa/carbon window calls.
//  If a NSWindow has been fetched for the given WinowdRef, the use the appropriate cocoa call.
//  Otherwise use the carbon call (as before).  Fixes bug associated with sheet (cocoa) file dialogs [[ 10136 ]].

static bool get_ns_window_from_carbon_window(void *p_window, NSWindow *&r_nswindow)
{
	void *t_nswindow;
	t_nswindow = nil;
	if (GetWindowProperty((WindowPtr)p_window, 'REVO', 'nswn', sizeof(void *), nil, &t_nswindow) == noErr && t_nswindow != nil)
	{
		r_nswindow = (NSWindow *)t_nswindow;
		return true;
	}
	return false;	
}

void *MCCreateNSWindow(void *p_window)
{
	NSWindow *t_nswindow;
	t_nswindow = nil;
	if (!get_ns_window_from_carbon_window(p_window, t_nswindow))
	{
		t_nswindow = [[NSWindow alloc] initWithWindowRef: (WindowPtr)p_window];
		[t_nswindow setCanHide: YES];
		SetWindowProperty((WindowRef)p_window, 'REVO', 'nswn', sizeof(NSWindow *), &t_nswindow);
	}
	return t_nswindow;
}

void MCDisposeWindow(void *p_window)
{
	NSWindow *t_nswindow;
	if (get_ns_window_from_carbon_window(p_window, t_nswindow))
		[t_nswindow release];
	DisposeWindow((WindowPtr)p_window);
}

void MCShowHideWindow(void *p_window, bool p_visible)
{
	NSWindow *t_nswindow;
	if (get_ns_window_from_carbon_window(p_window, t_nswindow))
	{
		if (p_visible)
			[t_nswindow orderFront: nil];
		else
			[t_nswindow orderOut: nil];
	}
	else
		ShowHide((WindowPtr)p_window, p_visible);
}

void MCShowWindow(void *p_window)
{
	NSWindow *t_nswindow;
	if (get_ns_window_from_carbon_window(p_window, t_nswindow))	
		[t_nswindow orderFront: nil];
	else
		ShowWindow((WindowPtr)p_window);
}

void MCHideWindow(void *p_window)
{
	NSWindow *t_nswindow;
	if (get_ns_window_from_carbon_window(p_window, t_nswindow))
		[t_nswindow orderOut: nil];
	else
		HideWindow((WindowPtr)p_window);
}

void MCBringWindowToFront(void *p_window)
{
	NSWindow *t_nswindow;
	if (get_ns_window_from_carbon_window(p_window, t_nswindow))
		[t_nswindow orderFront: nil];
	else
		BringToFront((WindowPtr)p_window);		
}