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
#include <Carbon/Carbon.h>

#include "foundation.h"
#include "typedefs.h"
#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

////////////////////////////////////////////////////////////////////////////////

extern bool MCS_exists(MCStringRef p_path, bool p_is_file);
extern bool MCS_resolvepath(MCStringRef p_path, MCStringRef &r_folderpath);

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

static bool folder_path_from_initial_path(MCStringRef p_path, MCStringRef &r_folderpath)
{
    MCAutoStringRef t_path;
    uindex_t t_offset;
    bool t_success;
    
    t_success = false;
    
    if (MCStringFirstIndexOfChar(p_path, '/', 0, kMCStringOptionCompareExact, t_offset))
    {
        if (t_offset != 0)
            t_success = MCS_resolvepath(p_path, &t_path);
        else
            t_success = MCStringCopy(p_path, &t_path);
    }
	
    if (t_success)
	{
        if (MCS_exists(*t_path, False))
            t_success = MCStringCopy(*t_path, r_folderpath);
        else if (MCStringLastIndexOfChar(*t_path, '/', UINT32_MAX, kMCStringOptionCompareExact, t_offset))
            t_success = MCStringCopySubstring(*t_path, MCRangeMake(0, t_offset), r_folderpath);
	}
	
    return t_success;
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

static void MCMacPlatformBeginOpenSaveDialog(MCPlatformWindowRef p_owner, NSSavePanel *p_panel, MCStringRef p_folder, MCStringRef p_file)
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
		t_initial_folder = MCStringConvertToAutoreleasedNSString(p_folder);
	
	NSString *t_initial_file;
	t_initial_file = nil;
	if (p_file != nil)
		t_initial_file = MCStringConvertToAutoreleasedNSString(p_file);
	
	
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

MCPlatformDialogResult MCPlatformEndFolderDialog(MCStringRef& r_selected_folder)
{	
	if (s_dialog_nesting -> result == kMCPlatformDialogResultContinue)
		return kMCPlatformDialogResultContinue;
	
	if (s_dialog_nesting -> result == kMCPlatformDialogResultSuccess)
	{
		NSString *t_alias;
		resolve_alias([s_dialog_nesting -> panel filename], t_alias);
        /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)t_alias, r_selected_folder);
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
	MCStringRef tag;
	MCStringRef *extensions;
	uint32_t extension_count;
	MCStringRef *filetypes;
	uint32_t filetypes_count;
};


static void MCFileFilterDestroy(MCFileFilter *self)
{
	for(uint32_t i = 0; i < self -> extension_count; i++)
		MCValueRelease(self -> extensions[i]);
	MCMemoryDeleteArray(self -> extensions);
	for(uint32_t i = 0; i < self -> filetypes_count; i++)
		MCValueRelease(self -> filetypes[i]);
	MCMemoryDeleteArray(self -> filetypes);
	MCValueRelease(self -> tag);
	MCMemoryDelete(self);
}

static bool MCFileFilterCreate(MCStringRef p_desc, MCFileFilter*& r_filter)
{    
	bool t_success;
	t_success = true;
	
	MCFileFilter *self;
	self = nil;
	if (t_success)
		t_success = MCMemoryNew(self);
    
    extern bool MCStringsSplit(MCStringRef p_string, codepoint_t p_separator, MCStringRef*&r_strings, uindex_t& r_count);
	
	MCAutoStringRefArray t_items;
	if (t_success)
		t_success = MCStringsSplit(p_desc, '|', t_items . PtrRef(), t_items . CountRef());
	
	// MM-2012-03-09: [[Bug]] Make sure we don't try and copy empty tags (causes a crash on Lion)
	if (t_success)
    {
        if (!MCStringIsEmpty(t_items[0]))
            self -> tag = MCValueRetain(t_items[0]);
    }
    
	if (t_success)
	{
        if (t_items . Count() < 2)
        {
            self -> filetypes = new MCStringRef(MCSTR("*"));
            self -> extensions = new MCStringRef(MCSTR("*"));
            self -> extension_count = 1;
            self -> filetypes_count = 1;
        }
        else
        {
            t_success = MCStringsSplit(t_items[1], ',', self -> extensions, self -> extension_count);
            if (t_items . Count() > 2)
            {
                t_success = MCStringsSplit(t_items[2], ',', self -> filetypes, self -> filetypes_count);
            }
        }
	}
	
	// MM 2012-10-05: [[ Bug 10409 ]] Make sure filters of the form "All|" and "All||" are treated as wildcards
	//   (but not filters of the form "Stacks||RSTK").
	if (t_success)
    {
		if (self -> extension_count == 1 && MCStringIsEmpty(self -> extensions[0]) && (t_items . Count() == 2 || (self -> filetypes_count == 1 && MCStringIsEmpty(self -> filetypes[0]))))
		{
			MCValueRelease(self -> extensions[0]);
            self -> extensions[0] = MCSTR("*");
		}
	}
    
	if (t_success)
		r_filter = self;
	else
		MCFileFilterDestroy(self);
	
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
- (void)setTypes: (MCStringRef *)p_types length: (uint32_t)p_count;

- (void)typeChanged: (id)sender;

- (MCStringRef)currentType;

- (BOOL)panel:(id)sender shouldShowFilename:(NSString *)filename;

@end

@compatibility_alias MCFileDialogAccessoryView com_runrev_livecode_MCFileDialogAccessoryView;

@implementation com_runrev_livecode_MCFileDialogAccessoryView

- (id)initWithPanel: (NSSavePanel *)panel
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

- (void)setTypes: (MCStringRef *)p_types length: (uint32_t)p_count
{
	for(uint32_t i = 0; i < p_count; i++)
	{
		MCFileFilter *t_filter;
		if (MCFileFilterCreate(p_types[i], t_filter))
		{
			MCListPushBack(m_filters, t_filter);
			if (t_filter -> tag != nil)
				[ m_options addItemWithTitle: MCStringConvertToAutoreleasedNSString(t_filter -> tag)];
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

- (MCStringRef)currentType
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
		if (MCStringIsEqualTo(MCSTR("*"), m_filter->extensions[i], kMCCompareExact))
			return YES;
	for (uint32_t i = 0; i < m_filter->filetypes_count; i++)
		if (MCStringIsEqualTo(MCSTR("*"), m_filter->filetypes[i], kMCCompareExact))
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
        MCAutoStringRef t_filename;
		if (MCStringCreateWithCFStringRef((CFStringRef)t_filename_resolved, &t_filename) && *t_filename != nil)
		{
			uindex_t t_dot;
            // AL-2014-04-01: [[ Bug 12081 ]] Find last occurrence of '.' rather than first, for file extension detection.
            if (MCStringLastIndexOfChar(*t_filename, '.', UINDEX_MAX, kMCCompareExact, t_dot))
            {
                MCRange t_range = MCRangeMake(t_dot + 1, UINDEX_MAX);
                for (uint32_t i = 0; i < m_filter->extension_count && !t_should_show; i++)
                    if (MCStringSubstringIsEqualTo(*t_filename, t_range, m_filter->extensions[i], kMCCompareCaseless))
                        t_should_show = YES;
            }
		}
	}
	
	// Check the various HFS codes to see if they match any of the typesin the filter
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
            // AL-2014-10-21: [[ Bug 13745 ]] Empty filter is not wild, so only do comparisons for non-empty
			if (!MCStringIsEmpty(m_filter->filetypes[i]) &&
                (MCStringIsEqualToCString(m_filter->filetypes[i], t_type, kMCCompareCaseless) ||
                MCStringIsEqualToCString(m_filter->filetypes[i], t_creator, kMCCompareCaseless) ||
                MCStringIsEqualToCString(m_filter->filetypes[i], [t_bundle_sig cStringUsingEncoding: NSMacOSRomanStringEncoding], kMCCompareCaseless) ||
                MCStringIsEqualToCString(m_filter->filetypes[i], [t_bundle_type cStringUsingEncoding: NSMacOSRomanStringEncoding], kMCCompareCaseless)))
				t_should_show = YES;
	}
	
	[t_filename_resolved release];
	return t_should_show;
}

@end

////////////////////////////////////////////////////////////////////////////////

void MCPlatformBeginFolderOrFileDialog(MCPlatformFileDialogKind p_kind, MCPlatformWindowRef p_owner, MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef *p_types, uint4 p_type_count)
{
    MCAutoStringRef t_initial_folder;
    if (p_initial != nil)
    /* UNCHECKED */ folder_path_from_initial_path(p_initial, &t_initial_folder);
    
    MCAutoStringRef t_initial_file;
    if ((p_kind == kMCPlatformFileDialogKindSave) && p_initial != nil && !MCS_exists(p_initial, false))
    {
        uindex_t t_last_slash;
        if (MCStringLastIndexOfChar(p_initial, '/', UINT32_MAX, kMCStringOptionCompareExact, t_last_slash))
            // SN-2014-08-11: [[ Bug 13143 ]] Take the right part: after the last slash, not before
            MCStringCopySubstring(p_initial, MCRangeMake(t_last_slash + 1, MCStringGetLength(p_initial) - t_last_slash - 1), &t_initial_file);
        else
            t_initial_file = p_initial;
    }
    
    NSSavePanel *t_panel;
    t_panel = (p_kind == kMCPlatformFileDialogKindSave) ? [NSSavePanel savePanel] : [NSOpenPanel openPanel] ;
    
    // If we have both a title and a message, then 'title' is used on setTitle, and 'prompt' is used for message.
    // If there is no title, then on 10.11+ we must use the message field of the dialog; on <10.11 we must use the title field
    if (p_title != nil && !MCStringIsEmpty(p_title))
    {
        [t_panel setTitle: MCStringConvertToAutoreleasedNSString(p_title)];
        [t_panel setMessage:MCStringConvertToAutoreleasedNSString(p_prompt)];
    }
    else
    {
        extern uint4 MCmajorosversion;
        if (MCmajorosversion >= 0x10B0 && p_kind != kMCPlatformFileDialogKindSave)
            [t_panel setMessage:MCStringConvertToAutoreleasedNSString(p_prompt)];
        else
            [t_panel setTitle: MCStringConvertToAutoreleasedNSString(p_prompt)];
    }
    
    if (p_kind != kMCPlatformFileDialogKindFolder)
    {
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
    }
    else
    {
        [t_panel setPrompt: @"Choose"];
        [(NSOpenPanel *)t_panel setCanChooseFiles: NO];
        [(NSOpenPanel *)t_panel setCanChooseDirectories: YES];
        [(NSOpenPanel *)t_panel setAllowsMultipleSelection: NO];
        
        // MM-2012-03-01: [[ BUG 10046]] Make sure the "new folder" button is enabled for folder dialogs
        [t_panel setCanCreateDirectories: YES];
    }
    
    MCMacPlatformBeginOpenSaveDialog(p_owner, t_panel, *t_initial_folder, p_kind != kMCPlatformFileDialogKindFolder ? *t_initial_file : nil);
}

MCPlatformDialogResult MCPlatformEndFileDialog(MCPlatformFileDialogKind p_kind, MCStringRef &r_paths, MCStringRef &r_type)
{
	if (s_dialog_nesting -> result == kMCPlatformDialogResultContinue)
		return kMCPlatformDialogResultContinue;
	
	if (s_dialog_nesting -> result == kMCPlatformDialogResultSuccess)
	{
		MCFileDialogAccessoryView *t_accessory;
		t_accessory = (MCFileDialogAccessoryView *)[s_dialog_nesting -> panel accessoryView];
		
		if (p_kind == kMCPlatformFileDialogKindSave)
		{
            /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)[s_dialog_nesting -> panel filename], r_paths);
			if (t_accessory != nil && [t_accessory currentType] != nil)
				r_type = MCValueRetain([t_accessory currentType]);
			else
				r_type = nil;
		}
		else
		{
			MCStringCreateMutable(0, r_paths);
            
			for(uint32_t i = 0; i < [[(NSOpenPanel *)s_dialog_nesting -> panel filenames] count ]; i++)
			{
				// MM-2012-09-25: [[ Bug 10407 ]] Resolve alias (if any) of the returned files.
				NSString *t_alias;
				resolve_alias([[(NSOpenPanel *)s_dialog_nesting -> panel filenames] objectAtIndex: i], t_alias);
                
				MCAutoStringRef t_conv_filename;			
				if (MCStringCreateWithCFStringRef((CFStringRef)t_alias, &t_conv_filename))
					/* UNCHECKED */ MCStringAppendFormat(r_paths, "%s%@", i > 0 ? "\n" : "", *t_conv_filename);

				[t_alias release];
			}
			if (t_accessory != nil && [t_accessory currentType] != nil)
				r_type = MCValueRetain([t_accessory currentType]);
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
    
    // Get the colour picker's view and store it
    mColorPanel = [p_panel retain];
    
    mColorPickerView = [p_view retain];
    
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
    
    [mColorPanel release];
    [mColorPickerView release];
    
    
    [mOkButton release];
    [mCancelButton release];
    [mUpdatedView release];
    
    [super dealloc];
}

-(void)closePanel
{
    [mColorPanel close];
    [mColorPanel setDelegate: nil];
    
    // Reset the color's picker view
    [mColorPickerView removeFromSuperview];
    [mColorPanel setContentView: mColorPickerView];
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
        t_color =  [mColorPanel color];
        
        // Some NSColor's will not have a colorspace (e.g. named ones from the developer
        // pane). Since trying to get a colorSpace of such a thing throws an exception
        // we wrap the colorSpace access call.
        NSColorSpace *t_colorspace;
        @try {
            t_colorspace = [t_color colorSpace];
        }
        @catch (NSException *exception) {
            t_colorspace = nil;
        }
        
        // If we have no colorspace, or the colorspace is not already RGB convert.
        if (t_colorspace == nil ||
            [t_colorspace colorSpaceModel] != NSRGBColorSpaceModel)
            t_color = [t_color colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
    
        // Convert the value from to a colour component value.
        s_color_dialog_color . red   = (uint2) ([t_color redComponent] * UINT16_MAX);
        s_color_dialog_color . green = (uint2) ([t_color greenComponent] * UINT16_MAX);
        s_color_dialog_color . blue  = (uint2) ([t_color blueComponent] * UINT16_MAX);
    }
}

//////////
// NSWindow delegate's method

// PM-2015-07-10: [[ Bug 15096 ]] Escape key should dismiss the 'answer color' dialog
- (void) windowDidBecomeKey:(NSNotification *)notification
{
	NSEvent* (^handler)(NSEvent*) = ^(NSEvent *theEvent) {
		
		NSEvent *result = theEvent;
		// Check if the esc key is pressed
		if (theEvent.keyCode == 53)
		{
			[self processEscKeyDown];
			result = nil;
		}
		
		return result;
	};
	
	eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSKeyDownMask handler:handler];
}

- (void)windowDidResize:(NSNotification *)notification
{
    [self relayout];
}

-(void) windowWillClose:(NSNotification *)notification
{
    if (mResult != kMCPlatformDialogResultSuccess)
        mResult = kMCPlatformDialogResultCancel;
		
	// Detach the event monitor when window closes.
	[NSEvent removeMonitor:eventMonitor];
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

- (void) processEscKeyDown
{
	[self pickerCancelClicked];
}

//////////

-(MCPlatformDialogResult) result
{
    return mResult;
}

@end

////////////////////////////////////////////////////////////////////////////////

void MCPlatformBeginColorDialog(MCStringRef p_title, const MCColor& p_color)
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
    
    NSView* t_pickerView = [t_colorPicker contentView];
    
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
	MCMacPlatformApplicationBecomePseudoModalFor(t_colorPicker);
}

MCPlatformDialogResult MCPlatformEndColorDialog(MCColor& r_color)
{
    // SN-2014-10-20: [[ Bug 13628 ]] Deallocate the delegate in case we don't continue
    if (s_color_dialog_result != kMCPlatformDialogResultContinue)
    {
        if (s_color_dialog_result == kMCPlatformDialogResultSuccess)
            r_color = s_color_dialog_color;

		MCMacPlatformApplicationBecomePseudoModalFor(nil);
    
        [s_color_dialog_delegate closePanel];
        [s_color_dialog_delegate release];

        s_color_dialog_delegate = NULL;
    }
    
	return s_color_dialog_result;
}

////////////////////////////////////////////////////////////////////////////////
