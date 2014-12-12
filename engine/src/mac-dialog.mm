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
#include <Carbon/Carbon.h>

#include "core.h"
#include "typedefs.h"
#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

////////////////////////////////////////////////////////////////////////////////

extern Boolean MCS_exists(const char *, Boolean);
extern char *MCS_resolvepath(const char *);

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCOpenSaveDialogDelegate: NSObject

- (void)panelDidEnd:(id)printDialog returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo;

@end

@compatibility_alias MCOpenSaveDialogDelegate com_runrev_livecode_MCOpenSaveDialogDelegate;

////////////////////////////////////////////////////////////////////////////////

struct MCMacPlatformDialogNest
{
	MCMacPlatformDialogNest *next;
	MCPlatformDialogResult result;
	MCPlatformWindowRef owner;
	NSSavePanel *panel;
};

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCOpenSaveDialogDelegate

- (void)panelDidEnd:(id)printDialog returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
	MCMacPlatformDialogNest *t_nest;
	t_nest = (MCMacPlatformDialogNest *)contextInfo;
	
	if (returnCode == NSOKButton)
		t_nest -> result = kMCPlatformDialogResultSuccess;
	else
		t_nest -> result = kMCPlatformDialogResultCancel;
}

@end

////////////////////////////////////////////////////////////////////////////////

static MCMacPlatformDialogNest *s_dialog_nesting = nil;
static MCOpenSaveDialogDelegate *s_dialog_delegate = nil;

static void MCMacPlatformBeginOpenSaveDialog(MCPlatformWindowRef p_owner, NSSavePanel *p_panel, const char *p_folder, const char *p_file)
{
	if (s_dialog_delegate == nil)
		s_dialog_delegate = [[MCOpenSaveDialogDelegate alloc] init];
	
	MCMacPlatformDialogNest *t_nest;
	/* UNCHECKED */ MCMemoryNew(t_nest);
	t_nest -> next = s_dialog_nesting;
	t_nest -> result = kMCPlatformDialogResultContinue;
	t_nest -> panel = p_panel;
	s_dialog_nesting = t_nest;
	
	/////////
	
	NSString *t_initial_folder;
	t_initial_folder = nil;
	if (p_folder != nil)
		t_initial_folder = [NSString stringWithCString: p_folder encoding: NSMacOSRomanStringEncoding];
	
	NSString *t_initial_file;
	t_initial_file = nil;
	if (p_file != nil)
		t_initial_file = [NSString stringWithCString: p_file encoding: NSMacOSRomanStringEncoding];			
	
	
	if (p_owner == nil)
	{
		t_nest -> owner = nil;
		
		// MM-2012-03-16: [[ Bug ]] Use runModalForDirectory:file: rather than setting directory and file - methods only introduced in 10.6
		if ([p_panel runModalForDirectory: t_initial_folder file: t_initial_file] == NSOKButton)
			t_nest -> result = kMCPlatformDialogResultSuccess;
		else
			t_nest -> result = kMCPlatformDialogResultCancel;
	}
	else
	{
		[p_panel beginSheetForDirectory: t_initial_folder
								   file: t_initial_file
						 modalForWindow: ((MCMacPlatformWindow *)p_owner) -> GetHandle()
						  modalDelegate: s_dialog_delegate
						 didEndSelector: @selector(panelDidEnd:returnCode:contextInfo:)
							contextInfo: t_nest];
	}
}

static MCPlatformDialogResult MCPlatformEndOpenSaveDialog(void)
{
	MCMacPlatformDialogNest *t_nest;
	t_nest = s_dialog_nesting;
	
	s_dialog_nesting = s_dialog_nesting -> next;
	
	MCPlatformDialogResult t_result;
	t_result = t_nest -> result;
	
	if (t_nest -> owner != nil)
		MCPlatformReleaseWindow(t_nest -> owner);
	
	MCMemoryDelete(t_nest);
	
	return t_result;	
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformBeginFolderDialog(MCPlatformWindowRef p_owner, const char *p_title, const char *p_prompt, const char *p_initial)
{
	char *t_initial_folder;
	t_initial_folder = nil;
	if (p_initial != nil)
		t_initial_folder = folder_path_from_intial_path(p_initial);
	
	NSOpenPanel *t_panel;
	t_panel = [NSOpenPanel openPanel];
	if (p_title != nil && strlen(p_title) != 0)
	{
		[t_panel setTitle: [NSString stringWithCString: p_title encoding: NSMacOSRomanStringEncoding]];
		[t_panel setMessage: [NSString stringWithCString:p_prompt encoding: NSMacOSRomanStringEncoding]];
	}
	else
		[t_panel setTitle: [NSString stringWithCString: p_prompt encoding: NSMacOSRomanStringEncoding]];
	[t_panel setPrompt: @"Choose"];
	[t_panel setCanChooseFiles: NO];
	[t_panel setCanChooseDirectories: YES];
	[t_panel setAllowsMultipleSelection: NO];
	
	// MM-2012-03-01: [[ BUG 10046 ]] Make sure the "new folder" button is enabled for folder dialogs
	[t_panel setCanCreateDirectories: YES];
	
	MCMacPlatformBeginOpenSaveDialog(p_owner, t_panel, t_initial_folder, nil);
	
	MCCStringFree(t_initial_folder);
}

MCPlatformDialogResult MCPlatformEndFolderDialog(char*& r_selected_folder)
{	
	if (s_dialog_nesting -> result == kMCPlatformDialogResultContinue)
		return kMCPlatformDialogResultContinue;
	
	if (s_dialog_nesting -> result == kMCPlatformDialogResultSuccess)
	{
		NSString *t_alias;
		resolve_alias([s_dialog_nesting -> panel filename], t_alias);									
		/* UNCHECKED */ MCCStringToNative([t_alias UTF8String], r_selected_folder);
		[t_alias release];
	}
	else
		r_selected_folder = nil;
	
	return MCPlatformEndOpenSaveDialog();
}

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

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCFileDialogAccessoryView : NSView<NSOpenSavePanelDelegate>
{
	NSTextField *m_label;
	NSPopUpButton *m_options;
	
	MCFileFilter *m_filters;
	MCFileFilter *m_filter;
	uint32_t m_filter_index;
    
    NSSavePanel *m_panel;
    
    id m_panel_view_hack;
}

- (id)initWithPanel: (NSSavePanel *)panel;
- (void)dealloc;

- (void)setLabel: (NSString *)newLabel;
- (void)setTypes: (char * const *)p_types length: (uint32_t)p_count;

- (void)typeChanged: (id)sender;

- (const char *)currentType;

- (BOOL)panel:(id)sender shouldShowFilename:(NSString *)filename;

@end

@compatibility_alias MCFileDialogAccessoryView com_runrev_livecode_MCFileDialogAccessoryView;

@implementation com_runrev_livecode_MCFileDialogAccessoryView

- (id)initWithPanel: (NSSavePanel *)panel;
{
	self = [ super init ];
    if (self == nil)
        return nil;
	
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
    
    m_panel = panel;
	
	return self;
}

- (void)dealloc
{
	while(m_filters != nil)
		MCFileFilterDestroy(MCListPopFront(m_filters));
	
	[ m_label release ];
	[ m_options release ];
	[ super dealloc ];
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

// MW-2014-07-25: [[ Bug 12250 ]] Hack to find tableview inside the savepanel so we can force
//   an update.
- (id)findPanelViewHack: (NSArray *)subviews
{
    id t_table = nil;
    for(id t_view in subviews)
    {
        if ([[t_view className] isEqualToString: @"FI_TListView"])
        {
            t_table = t_view;
            break;
        }
        
        t_table = [self findPanelViewHack: [t_view subviews]];
        if (t_table != nil)
            break;
    }
    
    return t_table;
}

// MW-2014-07-25: [[ Bug 12250 ]] Use the hack from:
//     http://stackoverflow.com/questions/18192986/nsopenpanel-doesnt-validatevisiblecolumns
//   To make sure the file list updates - it appears to be an apple bug and it might
//   be that 'setAllowedFileTypes' would work (although my testing appears to indicate otherwise)
//   however using that would require a change to how the answer command is used in this case
//   so we need to make sure the current method works anyway.
- (void)typeChanged: (id)sender
{
	uint32_t t_new_index;
	t_new_index =  [[ m_options objectValue ] intValue ];
	
	m_filter = m_filters;
	for(uint32_t i = 0; i < t_new_index; i++)
		m_filter = m_filter -> next;
	
    if (m_panel_view_hack == nil)
        m_panel_view_hack = [self findPanelViewHack: [[m_panel contentView] subviews]];
    
    if (m_panel_view_hack != nil)
        [m_panel_view_hack reloadData];
        
	[m_panel validateVisibleColumns];
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

void MCPlatformBeginFileDialog(MCPlatformFileDialogKind p_kind, MCPlatformWindowRef p_owner, const char *p_title, const char *p_prompt,  char * const p_types[], uint4 p_type_count, const char *p_initial)
{
	char *t_initial_folder;
	t_initial_folder = nil;
	if (p_initial != nil)
		t_initial_folder = folder_path_from_intial_path(p_initial);
	
	const char *t_initial_file;
	t_initial_file = nil;	
	if ((p_kind == kMCPlatformFileDialogKindSave) && p_initial != nil && !MCS_exists(p_initial, false))
	{
		char *t_last_slash;
		t_last_slash = strrchr(p_initial, '/');
		if (t_last_slash != nil)
			t_initial_file = strrchr(p_initial, '/') + 1;
		else
			t_initial_file = p_initial;
	}
	
	char *t_filename;
	t_filename = nil;
	
	char *t_type;
	t_type = nil;
	
	NSSavePanel *t_panel;
	t_panel = (p_kind == kMCPlatformFileDialogKindSave) ? [NSSavePanel savePanel] : [NSOpenPanel openPanel] ;
	
	if (p_title != nil && MCCStringLength(p_title) != 0)
	{
		[t_panel setTitle: [NSString stringWithCString: p_title encoding: NSMacOSRomanStringEncoding]];
		[t_panel setMessage: [NSString stringWithCString: p_prompt encoding: NSMacOSRomanStringEncoding]];
	}
	else
		[t_panel setTitle: [NSString stringWithCString: p_prompt encoding: NSMacOSRomanStringEncoding]];
	
    // MW-2014-07-17: [[ Bug 12826 ]] If we have at least one type, add a delegate. Only add as
    //   an accessory view if more than one type.
    MCFileDialogAccessoryView *t_accessory;
    if (p_type_count > 0)
    {
        t_accessory = [[MCFileDialogAccessoryView alloc] initWithPanel: t_panel];
        [t_accessory setTypes: p_types length: p_type_count];
        [t_accessory setLabel: @"Format:"];
        if (p_type_count > 1)
            [t_panel setAccessoryView: t_accessory];
        [t_panel setDelegate: t_accessory];
	}
    
	if (p_kind != kMCPlatformFileDialogKindSave)
	{
		[(NSOpenPanel *)t_panel setCanChooseFiles: YES];
		[(NSOpenPanel *)t_panel setCanChooseDirectories: NO];
		[(NSOpenPanel *)t_panel setAllowsMultipleSelection: p_kind == kMCPlatformFileDialogKindOpenMultiple ? YES : NO];
	}
	// MM-2012-03-01: [[ BUG 10046]] Make sure the "new folder" button is enabled for save dialogs
	else 
		[t_panel setCanCreateDirectories: YES];
	
	MCMacPlatformBeginOpenSaveDialog(p_owner, t_panel, t_initial_folder, t_initial_file);
	
	MCCStringFree(t_initial_folder);
}

MCPlatformDialogResult MCPlatformEndFileDialog(MCPlatformFileDialogKind p_kind, char*& r_paths, char*& r_type)
{
	if (s_dialog_nesting -> result == kMCPlatformDialogResultContinue)
		return kMCPlatformDialogResultContinue;
	
	if (s_dialog_nesting -> result == kMCPlatformDialogResultSuccess)
	{
		MCFileDialogAccessoryView *t_accessory;
		t_accessory = (MCFileDialogAccessoryView *)[s_dialog_nesting -> panel accessoryView];
		
		if (p_kind == kMCPlatformFileDialogKindSave)
		{
			r_paths = nil;
			MCCStringToNative([[s_dialog_nesting -> panel filename] UTF8String], r_paths);
			if (t_accessory != nil && [t_accessory currentType] != nil)
				/* UNCHECKED */ MCCStringToNative([t_accessory currentType], r_type);
			else
				r_type = nil;
		}
		else
		{
			r_paths = nil;
			for(uint32_t i = 0; i < [[(NSOpenPanel *)s_dialog_nesting -> panel filenames] count ]; i++)
			{
				// MM-2012-09-25: [[ Bug 10407 ]] Resolve alias (if any) of the returned files.
				NSString *t_alias;
				resolve_alias([[(NSOpenPanel *)s_dialog_nesting -> panel filenames] objectAtIndex: i], t_alias);					
				char *t_conv_filename;
				t_conv_filename = nil;					
				if (MCCStringToNative([t_alias UTF8String], t_conv_filename))
					/* UNCHECKED */ MCCStringAppendFormat(r_paths, "%s%s", i > 0 ? "\n" : "", t_conv_filename);
				/* UNCHECKED */ MCCStringFree(t_conv_filename);
				[t_alias release];
			}
			if (t_accessory != nil && [t_accessory currentType] != nil)
				/* UNCHECKED */ MCCStringToNative([t_accessory currentType], r_type);
			else
				r_type = nil;
		}
	}
	else
		r_paths = nil, r_type = nil;
    
    // MW-2014-07-17: [[ Bug 12826 ]] Make sure we release the delegate (might be nil, but no
    //   problem here with that).
    id t_delegate;
    t_delegate = [s_dialog_nesting -> panel delegate];
    [s_dialog_nesting -> panel setDelegate: nil];
    [t_delegate release];
	
	return MCPlatformEndOpenSaveDialog();
}

////////////////////////////////////////////////////////////////////////////////

static MCPlatformDialogResult s_color_dialog_result = kMCPlatformDialogResultContinue;
static MCColor s_color_dialog_color;
// SN-2014-10-20 [[ Bub 13628 ]] Added a static delegate for the colour picker
static MCColorPanelDelegate* s_color_dialog_delegate;

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCColorPanelDelegate

-(id) initWithColorPanel: (NSColorPanel*) p_panel
             contentView: (NSView*) p_view
{
    self = [super init];
    
    NSButton *t_ok_button;
    NSButton *t_cancel_button;
    
    // Get the colour picker's view and store it
    mColorPanel = p_panel;
    
    mColorPickerView = p_view;
    
    // Remove the colour picker's view
    [mColorPanel setContentView:0];
    
    // Create the 'OK' and 'Cancel' buttons
    mOkButton = [[NSButton alloc] init];
    mCancelButton = [[NSButton alloc] init];
    
    mOkButton.bezelStyle = NSRoundedBezelStyle;
    mOkButton.imagePosition = NSNoImage;
    [mOkButton setTitle: @"OK"];
    [mOkButton setAction:@selector(pickerOkClicked)];
    [mOkButton setTarget:self];
    
    mCancelButton.bezelStyle = NSRoundedBezelStyle;
    mCancelButton.imagePosition = NSNoImage;
    [mCancelButton setTitle: @"Cancel"];
    [mCancelButton setAction:@selector(pickerCancelClicked)];
    [mCancelButton setTarget:self];
    
    mResult = kMCPlatformDialogResultContinue;
    
    // Add all the views (colour picker panel + buttons)
    NSRect frameRect = { { 0.0, 0.0 }, { 0.0, 0.0 } };
    mUpdatedView = [[NSView alloc] initWithFrame:frameRect];
    [mUpdatedView addSubview:mColorPickerView];
    [mUpdatedView addSubview: mOkButton];
    [mUpdatedView addSubview: mCancelButton];
    
    [mColorPanel setContentView: mUpdatedView];
    [mColorPanel setDefaultButtonCell:[mOkButton cell]];
    
    [self relayout];
    
    return self;
}

-(void)dealloc
{
    NSColorPanel *t_color_picker;
    t_color_picker = [NSColorPanel sharedColorPanel];
    
    [[mColorPickerView window] close];
    
    // Reset the color's picker view
    [mColorPickerView removeFromSuperview];
    [t_color_picker setContentView: mColorPickerView];
    
    [mOkButton release];
    [mCancelButton release];
    [mUpdatedView release];
    
    [super dealloc];
}

// Redrawing method - adapts the size of the buttons to the size of the picker
-(void)relayout
{
    // Get the colorpicker's initial size
    NSRect rect = [[mColorPickerView superview] frame];
    
    const CGFloat ButtonMinWidth = 78.0; // 84.0 for Carbon
    const CGFloat ButtonMinHeight = 28.0;
    const CGFloat ButtonMaxWidth = 200.0;
    const CGFloat ButtonSpacing = 5.0;
    const CGFloat ButtonTopMargin = 0.0;
    const CGFloat ButtonBottomMargin = 7.0;
    const CGFloat ButtonSideMargin = 9.0;
    
    // Compute the desired width
    const CGFloat ButtonWidth = MCU_max(ButtonMinWidth,
                                        MCU_min(ButtonMaxWidth,
                                                CGFloat((rect.size.width - 2.0 * ButtonSideMargin - ButtonSpacing) * 0.5)));
    
    const CGFloat ButtonHeight = ButtonMinHeight;
    
    // SN-2014-11-28: [[ Bug 14098 ]] OK and Cancel buttons were inverted.
    // Update frame for the Cancel button
    NSRect cancelRect = { { ButtonSideMargin,
        ButtonBottomMargin },
        { ButtonWidth, ButtonHeight } };
    [mCancelButton setButtonType: NSMomentaryLightButton];
    [mCancelButton setFrame:cancelRect];
    [mCancelButton setNeedsDisplay:YES];
    
    // Update frame for the OK button
    NSRect okRect = { { cancelRect.origin.x + ButtonWidth + ButtonSpacing,
        ButtonBottomMargin },
        { ButtonWidth, ButtonHeight } };
    [mOkButton setButtonType: NSMomentaryLightButton];
    [mOkButton setFrame:okRect];
    [mOkButton setNeedsDisplay:YES];
    
    const CGFloat Y = ButtonBottomMargin + ButtonHeight + ButtonTopMargin;
    NSRect pickerCVRect = { { 0.0, Y },
        { rect.size.width, rect.size.height - Y } };
    
    [mColorPickerView setFrame:pickerCVRect];
    [mColorPickerView setNeedsDisplay:YES];
    
    [[mColorPickerView superview] setNeedsDisplay:YES];
}

// Sets the static MCColor to the value available from the color picker
-(void) getColor
{
    s_color_dialog_result = mResult;
    
    // In case of a successful event, set the color selected
    if (s_color_dialog_result == kMCPlatformDialogResultSuccess)
    {
        NSColor *t_color;
        
        // Do not calibrate the colour. As stated in the doc for -colorUsingColorSpace
        //   "The new NSColor object. This method converts the receiver's color to an equivalent
        //    one in the new color space. Although the new color might have different component
        //    values, it looks the same as the original."
        t_color = [mColorPanel color];
        
        // Convert the value from to a colour component value.
        s_color_dialog_color . red   = (uint2) ([t_color redComponent] * UINT16_MAX);
        s_color_dialog_color . green = (uint2) ([t_color greenComponent] * UINT16_MAX);
        s_color_dialog_color . blue  = (uint2) ([t_color blueComponent] * UINT16_MAX);
    }
}

//////////
// NSWindow delegate's method
- (void)windowDidResize:(NSNotification *)notification
{
    [self relayout];
}

-(void) windowWillClose:(NSNotification *)notification
{
    if (mResult != kMCPlatformDialogResultSuccess)
        mResult = kMCPlatformDialogResultCancel;
}

//////////
// Selectors called when the according button is pressed.
-(void) pickerCancelClicked
{
    mResult = kMCPlatformDialogResultCancel;
    [self getColor];
}

-(void) pickerOkClicked
{
    mResult = kMCPlatformDialogResultSuccess;
    [self getColor];
}

//////////

-(MCPlatformDialogResult) result
{
    return mResult;
}

@end

////////////////////////////////////////////////////////////////////////////////

void MCPlatformBeginColorDialog(const char *p_title, const MCColor& p_color)
{
    // SN-2014-10-20: [[ Bug 13628 ]] Update to use the Cocoa picker
    NSColorPanel *t_colorPicker;
    
    // Set the display type of the singleton colour panel.
    [NSColorPanel setPickerMask: NSColorPanelAllModesMask];
    
    t_colorPicker = [NSColorPanel sharedColorPanel];
    
    // SN-2014-11-28: [[ Bug 14098 ]] Make use of the initial colour
    CGFloat t_divider = UINT16_MAX;
    NSColor* t_initial_color = [NSColor colorWithCalibratedRed:(CGFloat)p_color.red / t_divider
                                                         green:(CGFloat)p_color.green / t_divider
                                                          blue:(CGFloat)p_color.blue / t_divider
                                                         alpha:1];
    [t_colorPicker setColor:t_initial_color];
    
    NSView* t_pickerView;
    t_pickerView = [t_colorPicker contentView];
    [t_pickerView retain];
    
    s_color_dialog_result = kMCPlatformDialogResultContinue;
    s_color_dialog_delegate = [[com_runrev_livecode_MCColorPanelDelegate alloc] initWithColorPanel:t_colorPicker
                                                                                       contentView:t_pickerView];
    
    // Set the color picker attributes
    [t_colorPicker setStyleMask:[t_colorPicker styleMask] & ~NSClosableWindowMask];
    [t_colorPicker setDelegate: s_color_dialog_delegate];
    
    // Make the colour picker the first window.
    // as modal mode breaks the color picker
    //[NSApp runModalForWindow: t_colorPicker];
    [t_colorPicker makeKeyAndOrderFront:t_colorPicker];
    [NSApp becomePseudoModalFor: t_colorPicker];
}

MCPlatformDialogResult MCPlatformEndColorDialog(MCColor& r_color)
{
    // SN-2014-10-20: [[ Bug 13628 ]] Deallocate the delegate in case we don't continue
    if (s_color_dialog_result != kMCPlatformDialogResultContinue)
    {
        if (s_color_dialog_result == kMCPlatformDialogResultSuccess)
            r_color = s_color_dialog_color;
        
        [NSApp becomePseudoModalFor: nil];
        [s_color_dialog_delegate dealloc];
        s_color_dialog_delegate = NULL;
    }
    
	return s_color_dialog_result;
}

////////////////////////////////////////////////////////////////////////////////
