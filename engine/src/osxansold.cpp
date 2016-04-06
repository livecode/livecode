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

//
// MAC plaform specific routines for ask and answer
//

#include "osxprefix.h"

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"
//#include "execpt.h"
#include "exec.h"
#include "mcerror.h"
#include "util.h"
#include "ans.h"
#include "objdefs.h"
#include "stack.h"
#include "stacklst.h"
#include "osspec.h"
#include "variable.h"

#include "globals.h"
#include "dispatch.h"
#include "card.h"

#include "printer.h"
#include "osxprinter.h"

#include "meta.h"
#include "mode.h"

#include "osxdc.h"


////////////////////////////////////////////////////////////////////////////////

extern void MCRemoteFileDialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint32_t p_type_count, MCStringRef p_initial_folder, MCStringRef p_initial_file, bool p_save, bool p_files, MCStringRef &r_value);
extern void MCRemoteFolderDialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef &r_value);
extern void MCRemoteColorDialog(MCStringRef p_title, uint32_t p_red, uint32_t p_green, uint32_t p_blue, bool& r_chosen, MCColor& r_chosen_color);

////////////////////////////////////////////////////////////////////////////////

typedef OSType                          SFTypeList[4];

/*********** Navigation Services routines  ****************/

OSType getAppSignature();
void getFSSpecFromAEDesc(AEDesc &inDesc, FSSpec &outValue);
pascal void navEventProc(NavEventCallbackMessage callBackSelector,
                         NavCBRecPtr callBackParms,
                         NavCallBackUserData callBackUD);
#ifdef LEGACY_EXEC
OSErr navOpenFile(const char *prompt, Boolean hasDefaultPath,
                  const FSSpecPtr defaultDirFspec, short numTypes,
                  SFTypeList filetypes, MCExecPoint &ep, Boolean oneSelection,
                  Boolean sheet);
#endif
OSErr navAnswerFolder(MCStringRef prompt, Boolean hasDefaultPath, const FSRef *p_default_fsref, MCExecContext &ctxt, Boolean sheet);
#ifdef LEGACY_EXEC
OSErr navSaveFile(char *prompt, char *defaultDir,
                  char *proposedName, MCExecPoint &ep, Boolean sheet);
#endif

struct FilterRecord
{
    MCStringRef tag;
    MCArrayRef extensions;
    MCArrayRef file_types;
    
    bool matchesExtension(const char *p_extension, unsigned int t_length)
	{
        uindex_t t_count = MCArrayGetCount(extensions);
		for(unsigned int t_index = 0; t_index < t_count; ++t_index)
        {
    
            MCValueRef t_value;
            /* UNCHECKED */ MCArrayFetchValueAtIndex(extensions, t_index + 1, t_value);
            MCAutoStringRef t_substring;
            /* UNCHECKED */ MCStringCopySubstring((MCStringRef)t_value, MCRangeMake(0, t_length), &t_substring);
            if (MCStringIsEqualToCString((MCStringRef)t_value, "*", kMCCompareExact) || MCStringIsEqualToCString(*t_substring, p_extension, kMCCompareExact))
				return true;
        }
		return false;
	}
    
    bool matchesType(const char *p_type)
	{
        uindex_t t_count = MCArrayGetCount(extensions);
		for(unsigned int t_index = 0; t_index < t_count; ++t_index)
        {
            
            MCValueRef t_value;
            /* UNCHECKED */ MCArrayFetchValueAtIndex(file_types, t_index + 1, t_value);
            MCAutoStringRef t_substring;
            /* UNCHECKED */ MCStringCopySubstring((MCStringRef)t_value, MCRangeMake(0, 4), &t_substring);
            if (MCStringIsEqualToCString((MCStringRef)t_value, "*", kMCCompareExact) || MCStringIsEqualToCString(*t_substring, p_type, kMCCompareExact))
				return true;
        }
		return false;
	}
    
    ~FilterRecord()
    {
        if (tag != nil)
            MCValueRelease(tag);
        if (extensions != nil)
            MCValueRelease(extensions);
        if (file_types != nil)
            MCValueRelease(file_types);
    }
    
#if 0
	Meta::simple_string tag;
	Meta::itemised_string extensions;
	Meta::itemised_string file_types;
	
	bool matchesExtension(const char *p_extension, unsigned int t_length)
	{
		for(unsigned int t_index = 0; t_index < extensions . count(); ++t_index)
			if (strcmp(*(extensions[t_index]), "*") == 0 || strncasecmp(*(extensions[t_index]), p_extension, t_length) == 0)
				return true;
		return false;
	}
	
	bool matchesType(const char *p_type)
	{
		for(unsigned int t_index = 0; t_index < file_types . count(); ++t_index)
			if (strcmp(*(file_types[t_index]), "*") == 0 || strncmp(*(file_types[t_index]), p_type, 4) == 0)
				return true;
		return false;
	}
#endif
};

static bool sg_navigation_dialog_busy = false;
static MCStringRef sg_navigation_initial_path = NULL;
static FilterRecord *sg_navigation_filter_records = NULL;
static unsigned int sg_navigation_filter_record_count = 0;
static unsigned int sg_navigation_filter_record_index = 0;
static Meta::simple_string_buffer sg_navigation_files;

static void navigation_event_callback(NavEventCallbackMessage p_message, NavCBRecPtr p_params, void *p_context)
{
	OSErr t_error = noErr;
	
	switch(p_message)
	{
		case kNavCBStart:
		{
			AEDesc t_descriptor;
			FSRef t_fsref;
			
			// MW-2011-02-07: [[ Bug 7974 ]] Make sure we use FSRefs.
			if (sg_navigation_initial_path != NULL)
			{
				/* UNCHECKED */ MCS_mac_pathtoref(sg_navigation_initial_path, t_fsref);
				AECreateDesc(typeFSRef, &t_fsref, sizeof(FSRef), &t_descriptor);
				NavCustomControl(p_params -> context, kNavCtlSetLocation, &t_descriptor);
				AEDisposeDesc(&t_descriptor);
			}
		}
			break;
			
		case kNavCBTerminate:
			sg_navigation_dialog_busy = false;
			break;
			
		case kNavCBEvent:
			if (p_params -> eventData . eventDataParms . event -> what == updateEvt)
				((MCScreenDC *)MCscreen) -> doredraw(*(p_params -> eventData .eventDataParms . event));
			break;
			
		case kNavCBPopupMenuSelect:
		{
			NavMenuItemSpec *t_item;
			t_item = (NavMenuItemSpec *)p_params -> eventData . eventDataParms . param;
			
            MCAutoStringRef t_item_name;
            /* UNCHECKED */ MCStringCreateWithCString((const char *)&t_item -> menuItemName[1], &t_item_name);
			for(unsigned int t_index = 0; t_index < sg_navigation_filter_record_count; ++t_index)
				if (MCStringIsEqualTo(*t_item_name, sg_navigation_filter_records[t_index] . tag, kMCCompareExact))
					sg_navigation_filter_record_index = t_index;
		}
			break;
			
		case kNavCBUserAction:
		{
			NavReplyRecord t_reply;
			t_error = NavDialogGetReply(p_params -> context, &t_reply);
			if (t_error != noErr || !t_reply . validRecord)
				return;
			
			switch(NavDialogGetUserAction(p_params -> context))
			{
				case kNavUserActionOpen:
				{
					long t_count;
					AEDesc t_descriptor;
					FSRef t_fsref;
					
					t_error = AECountItems(&t_reply . selection, &t_count);
					if (!t_error)
						for(int t_index = 1; t_index <= t_count; t_index += 1)
						{
							t_error = AEGetNthDesc(&t_reply . selection, t_index, typeFSRef, NULL, &t_descriptor);
							if (t_error != noErr)
								continue;
							
							t_error = AEGetDescData(&t_descriptor, &t_fsref, sizeof(FSRef));
							
							if (t_error == noErr)
							{
								char t_utf8_path[1025 + 32];
								FSRefMakePath(&t_fsref, (UInt8 *)t_utf8_path, 1024);
								
								char* t_path;
								uint4 t_path_length;
								MCS_utf8tonative(t_utf8_path, strlen(t_utf8_path), t_path, t_path_length);
								t_path[t_path_length] = '\0';
								
								sg_navigation_files . resize(sg_navigation_files . length() + t_path_length + 2);
								if (t_index > 1)
									strcat(sg_navigation_files . buffer(), "\n");
								strcat(sg_navigation_files . buffer(), t_path);
							}
							
							AEDisposeDesc(&t_descriptor);
						}
				}
					break;
					
				case kNavUserActionSaveAs:
				{
					FSRef t_fsref;
					AEDesc t_descriptor;
					
					t_error = AEGetNthDesc(&t_reply . selection, 1, typeWildCard, NULL, &t_descriptor);
					if (t_error != noErr)
						return;
					
					NavCompleteSave(&t_reply, kNavTranslateInPlace);
					
					t_error = AEGetDescData(&t_descriptor, &t_fsref, sizeof(FSRef));
					
					if (t_error == noErr)
					{
						char t_utf8_path[1025 + 32];
						FSRefMakePath(&t_fsref, (UInt8 *)t_utf8_path, 1024);
						
						char *t_path;
						uint4 t_path_length;
						MCS_utf8tonative(t_utf8_path, strlen(t_utf8_path), t_path, t_path_length);
						t_path[t_path_length] = '\0';
						
						sg_navigation_files . resize(t_path_length + 2 + PATH_MAX);
						strcpy(sg_navigation_files . buffer(), t_path);
						
						char *t_start = sg_navigation_files.buffer() + t_path_length;
						strcpy(t_start, "/");
						
						CFStringRef t_filename;
						t_filename = NavDialogGetSaveFileName(p_params -> context);
						if (t_filename != NULL)
						{
							// MW-2007-11-27: [[ Bug 4076 ]] Make sure that the leaf name that has been chosen
							//   has '/' replaced by ':' (for POSIX path compatibility).
							t_start++;
							CFStringGetCString(t_filename, t_start, PATH_MAX, CFStringGetSystemEncoding());
							while(*t_start != '\0')
							{
								if (*t_start == '/')
									*t_start = ':';
								t_start += 1;
							}
						}
					}
					AEDisposeDesc(&t_descriptor);
				}
					break;
			}
		}
			break;
	}
}

extern int UnicodeToUTF8(const uint2 *p_source_str, int p_source, char *p_dest_str, int p_dest);

static Boolean navigation_filter_callback(AEDesc *p_item, void *p_info, void *p_context, NavFilterModes p_mode)
{
	if (p_mode != kNavFilteringBrowserList)
		return true;
	
	if (sg_navigation_filter_record_count == 0)
		return true;
	
	if (p_item -> descriptorType != typeFSRef)
		return true;
	
	NavFileOrFolderInfo *t_item_info;
	t_item_info = (NavFileOrFolderInfo *)p_info;
	
	if (t_item_info -> isFolder && t_item_info -> fileAndFolder . folderInfo . folderType == '\?\?\?\?')
		return true;
	
	FSRef t_fsref;
	OSErr t_error;
	
	t_error = AEGetDescData(p_item, &t_fsref, sizeof(FSRef));
	if (t_error != noErr)
		return true;
	
	OSType t_type;
	if (t_item_info -> isFolder)
		t_type = t_item_info -> fileAndFolder . folderInfo . folderType;
	else
		t_type = t_item_info -> fileAndFolder . fileInfo . finderInfo . fdType;
	
	HFSUniStr255 t_unicode_filename;
	t_error = FSGetCatalogInfo(&t_fsref, kFSCatInfoNone, NULL, &t_unicode_filename, NULL, NULL);
	if (t_error != noErr)
		return true;
	
	int t_unicode_extension_index;
	for(t_unicode_extension_index = t_unicode_filename . length - 1; t_unicode_extension_index >= 0; t_unicode_extension_index -= 1)
		if (t_unicode_filename . unicode[t_unicode_extension_index] == '.')
			break;
	
	unsigned int t_unicode_extension_length;
	t_unicode_extension_length = (t_unicode_extension_index == -1 ? 0 : t_unicode_filename . length - t_unicode_extension_index - 1);
	
	char t_extension[t_unicode_extension_length * 4];
	int t_extension_length;
	t_extension_length = UnicodeToUTF8(t_unicode_filename . unicode + t_unicode_extension_index + 1, t_unicode_extension_length * 2, t_extension, t_unicode_extension_length * 4);
	
	if (t_extension_length > 0 && sg_navigation_filter_records[sg_navigation_filter_record_index] . matchesExtension(t_extension, t_extension_length))
		return true;
	
	// MW-2006-07-26: Endian issue - we need to byte-swap t_type...
	t_type = MCSwapInt32HostToNetwork(t_type);
	if (sg_navigation_filter_records[sg_navigation_filter_record_index] . matchesType((char *)&t_type))
		return true;
	
	return false;
}

static void build_types_from_filter_records(FilterRecord* p_filter_records, unsigned int p_filter_record_count, MCStringRef* &p_types, uint4 &r_type_count);

// MW-2005-06-03: New version of answer-file code because old stuff is so bad...
int MCA_do_file_dialog_tiger(MCStringRef p_title, MCStringRef p_prompt, FilterRecord *p_filters, unsigned int p_filter_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	if (!MCModeMakeLocalWindows())
	{
		bool t_save = (p_options & MCA_OPTION_SAVE_DIALOG) != 0;
		bool t_plural = (p_options & MCA_OPTION_PLURAL) != 0;
		uint32_t t_type_count;
		MCAutoStringRefArray t_types;
		build_types_from_filter_records(p_filters, p_filter_count, t_types.PtrRef(), t_types.CountRef());
        
        MCAutoStringRef t_resolved_path_str;
        MCS_resolvepath(p_initial, &t_resolved_path_str);
        
		MCRemoteFileDialog(p_title, p_prompt, *t_types, t_types.Count(), NULL, *t_resolved_path_str, t_save, t_plural, r_value);
	
		return noErr;
	}
	// MW-2007-12-21: [[ Bug 4548 ]] Make sure the cursor doesn't stay hidden on showing a system dialog!
	Cursor c;
	SetCursor(GetQDGlobalsArrow(&c));
	ShowCursor();
	
	NavDialogCreationOptions t_dialog_options;
	
	CFStringRef t_title;
	if (p_title == nil || MCStringGetLength(p_title) == 0)
		t_title = NULL;
	else
        MCStringConvertToCFStringRef(p_title, t_title);
	
	CFStringRef t_prompt;
	if (p_prompt == nil || MCStringGetLength(p_prompt) == 0)
		t_prompt = NULL;
	else
		MCStringConvertToCFStringRef(p_prompt, t_prompt);
	
	// Initialise the dialog options
	t_dialog_options . version = kNavDialogCreationOptionsVersion;
	t_dialog_options . optionFlags = kNavDontAddTranslateItems | kNavSupportPackages;
	
	if (p_filter_count <= 1)
		t_dialog_options . optionFlags |= kNavNoTypePopup;
	
	if ((p_options & MCA_OPTION_PLURAL))
		t_dialog_options . optionFlags |= kNavAllowMultipleFiles;
	
	t_dialog_options . location . v = -1;
	t_dialog_options . location . h = -1;
	t_dialog_options . clientName = NULL;
	t_dialog_options . actionButtonLabel = NULL;
	t_dialog_options . cancelButtonLabel = NULL;
	t_dialog_options . windowTitle = NULL;
	t_dialog_options . message = NULL;
	
	// Determine wheter the initial location is a folder
	bool t_initial_is_folder;
	if (p_initial != NULL && MCS_exists(p_initial, False))
		t_initial_is_folder = true;
	else
		t_initial_is_folder = false;
	
	// Only add a 'save' filename if saving, and initial isn't a folder
	if (p_initial != NULL && (p_options & MCA_OPTION_SAVE_DIALOG) != 0)
	{
		if (!t_initial_is_folder)
        {
            MCAutoStringRef t_filename;
            uindex_t t_last_slash;
            if (MCStringLastIndexOfChar(p_initial, '/', UINDEX_MAX, kMCStringOptionCompareExact, t_last_slash))
                /* UNCHECKED */ MCStringCopySubstring(p_initial, MCRangeMake(t_last_slash + 1, MCStringGetLength(p_initial) - (t_last_slash + 1)), &t_filename);
            else
                MCStringCopy(p_initial, &t_filename);
            
			MCStringConvertToCFStringRef(*t_filename, t_dialog_options . saveFileName);
        }
		else
			t_dialog_options . saveFileName = CFSTR("");
	}
	else
		t_dialog_options . saveFileName = NULL;
	t_dialog_options . preferenceKey = 0;
	
	// MW-2008-02-27: [[ Bug 5976 ]] Make sure we set the parentWindow pointer here all the time, otherwise
	//   we get errors logged to the console on Leopard.
	MCStack *t_parent_stack = NULL;
	if ((p_options & MCA_OPTION_SHEET) != 0)
	{
		Window t_parent_window;
		t_dialog_options . modality = kWindowModalityWindowModal;
		t_parent_window = MCdefaultstackptr -> getwindow();
		if (t_parent_window == NULL && MCtopstackptr != NULL)
			t_parent_window = MCtopstackptr -> getwindow();
		if (t_parent_window != NULL)
		{
			if (GetWRefCon((WindowPtr)t_parent_window -> handle . window) == WM_SHEET || GetWRefCon((WindowPtr)t_parent_window -> handle . window) == WM_MODAL)
			{
				t_dialog_options . parentWindow = NULL;
				t_dialog_options . modality = kWindowModalityAppModal;
			}
			else
			{
				t_dialog_options . parentWindow = (WindowPtr)t_parent_window -> handle . window;
				t_parent_stack = MCdispatcher -> findstackd(t_parent_window);
			}
		}
	}
	else
	{
		t_dialog_options . parentWindow = NULL;
		t_dialog_options . modality = kWindowModalityAppModal;
	}
	
	// MW-2005-06-06: Choose sensible placement of title and prompt based on which are non-empty
	if (t_title != NULL)
	{
		t_dialog_options . windowTitle = t_title;
		t_dialog_options . message = t_prompt;
	}
	else
		t_dialog_options . windowTitle = t_prompt;
	
	CFStringRef t_tags[p_filter_count];
	for(unsigned int t_index = 0; t_index < p_filter_count; ++t_index)
        /* UNCHECKED */ MCStringConvertToCFStringRef(p_filters[t_index] . tag, t_tags[t_index]);
	
	t_dialog_options . popupExtension = CFArrayCreate(kCFAllocatorDefault, (const void **)t_tags, p_filter_count, NULL);
	
	OSErr t_error = 0;
	NavEventUPP t_navigation_event_callback_ref = NULL;
	NavObjectFilterUPP t_navigation_filter_callback_ref = NULL;
	NavDialogRef t_dialog_ref = NULL;
	
	t_navigation_event_callback_ref = NewNavEventUPP(navigation_event_callback);
	t_navigation_filter_callback_ref = NewNavObjectFilterUPP(navigation_filter_callback);
	
	if ((p_options & MCA_OPTION_SAVE_DIALOG) != 0)
		t_error = NavCreatePutFileDialog(&t_dialog_options, kNavGenericSignature, kNavGenericSignature, t_navigation_event_callback_ref, NULL, &t_dialog_ref);
	else
		t_error = NavCreateGetFileDialog(&t_dialog_options, NULL, t_navigation_event_callback_ref, NULL, t_navigation_filter_callback_ref, NULL, &t_dialog_ref);
	
	if (t_error == noErr && t_dialog_ref != NULL)
	{
		sg_navigation_dialog_busy = true;
		sg_navigation_filter_records = p_filters;
		sg_navigation_filter_record_count = p_filter_count;
		sg_navigation_filter_record_index = 0;
		
		// MW-2005-06-09: By not setting the initial path to NULL when there isn't one we are
		//   losing the OS saving of last path.
		if (p_initial != NULL)
		{
            MCAutoStringRef t_folder_name;
            uindex_t t_last_slash;
            
            if (MCStringLastIndexOfChar(p_initial, '/', UINDEX_MAX, kMCStringOptionCompareExact, t_last_slash) &&
                (p_options & MCA_OPTION_SAVE_DIALOG) != 0 &&
                !t_initial_is_folder)
                /* UNCHECKED */ MCStringCopySubstring(p_initial, MCRangeMake(0, t_last_slash - 1), &t_folder_name);
            else
                /* UNCHECKED */ MCStringCopy(p_initial, &t_folder_name);
						
			MCValueAssign(sg_navigation_initial_path, *t_folder_name);
		}
		else
			sg_navigation_initial_path = NULL;
		
		sg_navigation_files . resize(1);
		*(sg_navigation_files . buffer()) = '\0';
		
		t_error = NavDialogRun(t_dialog_ref);
		
		
		while(sg_navigation_dialog_busy)
			MCscreen -> wait(REFRESH_INTERVAL, True, True);
		
		NavDialogDispose(t_dialog_ref);
		
		if (strlen(*sg_navigation_files) != 0)\
		{
            /* UNCHECKED */ MCStringCreateWithCString(*sg_navigation_files, r_value);
			
			if (p_options & MCA_OPTION_RETURN_FILTER && p_filter_count > 0)
                r_result = MCValueRetain(p_filters[sg_navigation_filter_record_index] . tag);
		}
	}
	
	// MW-2004-06-04: [[Deep Masks]] CFRelease mustn't be passed a NULL...
	if (t_dialog_options . windowTitle != NULL)
		CFRelease(t_dialog_options . windowTitle);
	if (t_dialog_options . saveFileName != NULL)
		CFRelease(t_dialog_options . saveFileName);
	if (t_dialog_options . message != NULL)
		CFRelease(t_dialog_options . message);
	if (t_dialog_options . popupExtension != NULL)
		CFRelease(t_dialog_options . popupExtension);
	
	for(unsigned int t_index = 0; t_index < p_filter_count; ++t_index)
		if (t_tags[t_index] != NULL)
			CFRelease(t_tags[t_index]);
	
	DisposeNavEventUPP(t_navigation_event_callback_ref);
	DisposeNavObjectFilterUPP(t_navigation_filter_callback_ref);
	
	sg_navigation_filter_records = NULL;
	
	// MW-2009-01-22: [[ Bug 3509 ]] Make sure we force an update to the menubar, just in case
	//   the dialog has messed with our menu item enabled state!
	if (t_dialog_options . modality == kWindowModalityWindowModal)
		MCscreen -> updatemenubar(True);
	
	return t_error;
}

static void build_filter_records_from_types(MCStringRef *p_types, uint4 p_type_count, FilterRecord*& r_filter_records, unsigned int& r_filter_record_count)
{
    r_filter_records = new FilterRecord[p_type_count];
	r_filter_record_count = p_type_count;
	
	for(uint4 t_type_index = 0; t_type_index < p_type_count; ++t_type_index)
	{
        MCAutoArrayRef t_type;
        /* UNCHECKED */ MCStringSplit(p_types[t_type_index], MCSTR("|"), nil, kMCCompareExact, &t_type);
        uindex_t t_index;
        t_index = MCArrayGetCount(*t_type);
        if (t_index < 1)
			continue;
        
        MCValueRef t_first;
        /* UNCHECKED */ MCArrayFetchValueAtIndex(*t_type, 1, t_first);
        r_filter_records[t_type_index] . tag = MCValueRetain((MCStringRef)t_first);
        
        if (t_index < 2)
		{
            /* UNCHECKED */ MCArrayCreateMutable(r_filter_records[t_type_index] . extensions);
            /* UNCHECKED */ MCArrayCreateMutable(r_filter_records[t_type_index] . file_types);
            
            /* UNCHECKED */ MCArrayStoreValueAtIndex(r_filter_records[t_type_index] . extensions, 1, MCSTR("*"));
            /* UNCHECKED */ MCArrayStoreValueAtIndex(r_filter_records[t_type_index] . file_types, 1, MCSTR("*"));
        }
        else
		{
            MCValueRef t_second;
            MCAutoArrayRef t_extensions;
            /* UNCHECKED */ MCArrayFetchValueAtIndex(*t_type, 1, t_second);
            /* UNCHECKED */ MCStringSplit((MCStringRef)t_second, MCSTR(","), nil, kMCCompareExact, &t_extensions);
            MCArrayCopy(*t_extensions, r_filter_records[t_type_index] . extensions);
			if (t_index > 2)
            {
                MCValueRef t_third;
                MCAutoArrayRef t_file_types;
                /* UNCHECKED */ MCArrayFetchValueAtIndex(*t_type, 2, t_third);
                /* UNCHECKED */ MCStringSplit((MCStringRef)t_third, MCSTR(","), nil, kMCCompareExact, &t_file_types);
                MCArrayCopy(*t_file_types, r_filter_records[t_type_index] . file_types);
            }
		}
        
	}
    
#if 0
	r_filter_records = new FilterRecord[p_type_count];
	r_filter_record_count = p_type_count;
	
	for(uint4 t_type_index = 0; t_type_index < p_type_count; ++t_type_index)
	{
        char *temp;
        /* UNCHECKED */ MCStringConvertToCString(p_types[t_type_index], temp);
		Meta::itemised_string t_type(temp, '|');
        delete temp;
		if (t_type . count() < 1)
			continue;
		
		r_filter_records[t_type_index] . tag = t_type[0];
		
		if (t_type . count() < 2)
		{
			r_filter_records[t_type_index] . extensions . assign("*", 1, ',');
			r_filter_records[t_type_index] . file_types . assign("*", 1, ',');
		}
		else
		{
			r_filter_records[t_type_index] . extensions . assign(t_type[1], ',');
			if (t_type . count() > 2)
				r_filter_records[t_type_index] . file_types . assign(t_type[2], ',');
		}
	}
#endif
}

static void build_types_from_filter_records(FilterRecord* p_filter_records, unsigned int p_filter_record_count, MCStringRef* &r_types, uint4 &r_type_count)
{
    MCAutoStringRefArray t_types;
    t_types.New(p_filter_record_count);
    
    bool t_success;
    t_success = true;
    
    for (uint32_t t_filter_index = 0 ; t_filter_index < p_filter_record_count && t_success; ++t_filter_index)
        t_success = MCStringCopy(p_filter_records[t_filter_index].tag, t_types[t_filter_index]);
    
    if (!t_success)
        r_type_count = 0;
    else
        t_types.Take(r_types, r_type_count);
}

int MCA_file_tiger(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef r_result)
{
	int t_result;
	FilterRecord *t_filters = NULL;
	unsigned int t_filter_count = 0;
    if (p_filter != NULL && MCStringGetLength(p_filter) >= 4)
	{
		unsigned int t_filetype_count;
        t_filetype_count = MCStringGetLength(p_filter) / 4;
        
        t_filters = new FilterRecord[1];
		t_filters[0] . tag = MCValueRetain(kMCEmptyString);
        /* UNCHECKED */ MCArrayCreateMutable(t_filters[0] . file_types);
        
        for(unsigned int t_index = 0; t_index < t_filetype_count; ++t_index)
        {
            MCAutoStringRef t_file_type;
            /* UNCHECKED */ MCStringCopySubstring(p_filter, MCRangeMake(t_index * 4, 4), &t_file_type);
            MCArrayStoreValueAtIndex(t_filters[0] . file_types, t_index + 1, *t_file_type);
        }

		t_filter_count = 1;        
	}
    t_result = MCA_do_file_dialog_tiger(p_title, p_prompt, t_filters, t_filter_count, p_initial, p_options, r_value, r_result);
	delete[] t_filters;
	return t_result;
}

int MCA_file_with_types_tiger(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	int t_result;
	FilterRecord *t_filters;
	unsigned int t_filter_count;
	build_filter_records_from_types(p_types, p_type_count, t_filters, t_filter_count);
	t_result = MCA_do_file_dialog_tiger(p_title, p_prompt, t_filters, t_filter_count, p_initial, p_options | MCA_OPTION_RETURN_FILTER, r_value, r_result);
	delete[] t_filters;
	return t_result;
}

int MCA_ask_file_tiger(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	int t_result;
	FilterRecord *t_filters = NULL;
	unsigned int t_filter_count = 0;

	if (p_filter != NULL && MCStringGetLength(p_filter) >= 4)
	{
		unsigned int t_filetype_count;
		t_filetype_count = MCStringGetLength(p_filter) / 4;
        
        t_filters = new FilterRecord[1];
		t_filters[0] . tag = MCValueRetain(kMCEmptyString);
        /* UNCHECKED */ MCArrayCreateMutable(t_filters[0] . file_types);
        
        for(unsigned int t_index = 0; t_index < t_filetype_count; ++t_index)
        {
            MCAutoStringRef t_file_type;
            /* UNCHECKED */ MCStringCopySubstring(p_filter, MCRangeMake(t_index * 4, 4), &t_file_type);
            MCArrayStoreValueAtIndex(t_filters[0] . file_types, t_index + 1, *t_file_type);
        }
        
        t_filter_count = 1;
    }
	t_result = MCA_do_file_dialog_tiger(nil, p_prompt, t_filters, t_filter_count, p_initial, p_options | MCA_OPTION_SAVE_DIALOG, r_value, r_result);

	delete[] t_filters;
	return t_result;
}

int MCA_ask_file_with_types_tiger(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	int t_result;
	FilterRecord *t_filters;
	unsigned int t_filter_count;
	build_filter_records_from_types(p_types, p_type_count, t_filters, t_filter_count);
	t_result = MCA_do_file_dialog_tiger(p_title, p_prompt, t_filters, t_filter_count, p_initial, p_options | MCA_OPTION_RETURN_FILTER | MCA_OPTION_SAVE_DIALOG, r_value, r_result);
	delete[] t_filters;
	return t_result;
}

/****************** Navigation Services routines *************************/
OSType getAppSignature()//returns an application's 4 letter character type code
{
	ProcessSerialNumber PSN;
	ProcessInfoRec info;
	Str31 processName;
	FSSpec FileSpec;
	info.processInfoLength = sizeof(ProcessInfoRec);
	info.processName = processName;
	info.processAppSpec = &FileSpec;
	GetCurrentProcess(&PSN);
	GetProcessInformation(&PSN, &info);
	return info.processSignature;
}

void getFSSpecFromAEDesc(AEDesc &inDesc, FSSpec &outValue)
{
	Handle	dataH;
	AEDesc	coerceDesc = {typeNull, nil};
	if (inDesc.descriptorType == typeFSS)
		dataH = (Handle)inDesc.dataHandle; 	//Descriptor is the type we want
	else
	{				//Try to coerce to the desired type
		if (AECoerceDesc(&inDesc, typeFSS, &coerceDesc) == noErr)// Coercion succeeded
			dataH = (Handle)coerceDesc.dataHandle;
	}
	outValue = **(FSSpec**)dataH;	//Extract value from Handle
	if (coerceDesc.dataHandle != NULL)
		AEDisposeDesc(&coerceDesc);
}

void getFSRefFromAEDesc(AEDesc &p_aedesc, FSRef *r_fsref)
{
	Handle t_handle;
	AEDesc	coerceDesc = {typeNull, nil};
	if (p_aedesc . descriptorType == typeFSRef)
		t_handle = (Handle)p_aedesc . dataHandle; 	//Descriptor is the type we want
	else if (AECoerceDesc(&p_aedesc, typeFSRef, &coerceDesc) == noErr)// Coercion succeeded
		t_handle = (Handle)coerceDesc . dataHandle;
	
	*r_fsref = **(FSRef**)t_handle;	//Extract value from Handle
	if (coerceDesc . dataHandle != NULL)
		AEDisposeDesc(&coerceDesc);
}

static char *navfilepath = NULL;
static Boolean navdialogbusy = False;
AEDesc *defaultlocationptr = NULL;

void navEventProc(NavEventCallbackMessage callBackSelector,
                  NavCBRecPtr callBackParms, NavCallBackUserData callBackUD)
{
	OSErr osError = noErr;
	NavReplyRecord reply;
	NavUserAction navUserAction;
	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	switch (callBackSelector)
	{
		case kNavCBUserAction:
			osError = NavDialogGetReply(callBackParms->context, &reply);
			if(osError == noErr && reply.validRecord)
			{
				navUserAction = NavDialogGetUserAction(callBackParms->context);
				switch(navUserAction)
				{
					case kNavUserActionOpen:
						if (reply.validRecord)
						{
							long count;
							if ((osError = AECountItems(&(reply.selection), &count)) == noErr)
							{
								long index;    //Set up index for file list
								AEDesc desc;
								FSRef  fsRef;
								for (index = 1; index <= count; index++)
								{//handle multiple files selection
									osError = AEGetNthDesc(&(reply.selection), index, typeFSRef,
														   NULL, &desc);
									if (osError == noErr)
									{
										
										osError = AEGetDescData(&desc, &fsRef, sizeof(fsRef));
										//  fprintf(stderr,"AEGetDescData failed %d\n", err );
										if (osError == noErr)
										{
											char *tnavfilepath = new char[1025 + 32];
											osError = FSRefMakePath(&fsRef, (UInt8 *)tnavfilepath, 1024);
											uint4 destlen;
											MCS_utf8tonative(tnavfilepath, strlen(tnavfilepath), navfilepath, destlen);
											navfilepath[destlen] = '\0';
											delete tnavfilepath;
										}
									}
								}
							}
						}
						else
							navfilepath = NULL;
						break;
					case kNavUserActionSaveAs:
						if (osError == noErr && reply.validRecord)
						{
							AEDesc desc;
							osError = AEGetNthDesc(&(reply.selection), 1, typeWildCard,
												   NULL, &desc);
							NavCompleteSave(&reply, kNavTranslateInPlace);
							if (osError == noErr)
							{ 
								//process single file for now, may be necessary to add support for multiple files
								FSSpec theSpec;
								getFSSpecFromAEDesc(desc, theSpec);	  //put reply in FSSpec
                                MCAutoStringRef t_path;
                                
								if (MCS_mac_FSSpec2path(&theSpec, &t_path))
								{
									uint2 tpathsize = MCStringGetLength(*t_path);
									navfilepath = new char[tpathsize + 32 + PATH_MAX];
									navfilepath[0] = 0;
                                    char *t_path_cstring;
                                    /* UNCHECKED */ MCStringConvertToCString(*t_path, t_path_cstring);
									strcpy(navfilepath, t_path_cstring);
									strcat(navfilepath, "/");
                                    delete t_path_cstring;
									CFStringRef fileName
									= NavDialogGetSaveFileName(callBackParms->context);
									if (fileName != NULL)
										osError = CFStringGetCString(fileName,
																	 &navfilepath[tpathsize + 1],
																	 tpathsize + 1 + PATH_MAX,
																	 CFStringGetSystemEncoding());
								}
							}
						}
						break;
					case kNavUserActionChoose:
						if (reply.validRecord)
						{
							AEDesc desc;
							osError = AEGetNthDesc(&(reply.selection), 1, typeWildCard,
												   NULL, &desc);
							if (osError == noErr)
							{
								// MH-2007-05-28 [[Bug 4984]] Answer folder would not return long file paths correctly.
								FSRef t_fsref;
								getFSRefFromAEDesc(desc, &t_fsref);
                                MCAutoStringRef t_navfile_path;
                                
								/* UNCHECKED */ MCS_mac_fsref_to_path(t_fsref, &t_navfile_path);
                                char *t_navfile_path_cstring;
                                /* UNCHECKED */ MCStringConvertToCString(*t_navfile_path, t_navfile_path_cstring);
                                navfilepath = t_navfile_path_cstring;
							}
						}
						else
							navfilepath = NULL;
						break;
				}
			}
			osError = NavDisposeReply(&reply);
			break;
		case kNavCBStart:
			if (defaultlocationptr)
				NavCustomControl(callBackParms->context, kNavCtlSetLocation,
								 defaultlocationptr);
			break;
		case kNavCBEvent:
			switch (((callBackParms->eventData).eventDataParms).event->what)
		{
			case updateEvt:
				pms->doredraw(*((callBackParms->eventData).eventDataParms).event);
				break;
		}
			break;
		case kNavCBTerminate:
			if (callBackParms->context != NULL)
				NavDialogDispose(callBackParms->context);
			navdialogbusy = False;
			break;
	}
}

//display Open File dialog through Navigation Services, called by Answer::exec()
OSErr navAnswerFolder(MCStringRef prompt, Boolean hasDefaultPath, const FSRef *p_default_dir_fsref, Boolean sheet, MCStringRef &r_value, MCStringRef &r_result)
{
	NavEventUPP eventProc = NewNavEventUPP(navEventProc);
	NavDialogCreationOptions dOptions; //dialog options structure
	AEDesc defaultLoc;
	OSErr	anErr = noErr;
	NavGetDefaultDialogCreationOptions(&dOptions);
	if (prompt != NULL)
        /* UNCHECKED */ MCStringConvertToCFStringRef(prompt, dOptions.windowTitle);
    
	MCStack *parentwindowstack = NULL;
	if (sheet)
	{
		dOptions.modality = kWindowModalityWindowModal;
		Window pw = MCdefaultstackptr->getwindow();
		if (pw == DNULL && MCtopstackptr != NULL)
			pw = MCtopstackptr->getwindow();
		if (pw != DNULL)
		{
			if (GetWRefCon((WindowPtr)pw->handle.window) == WM_SHEET
				|| GetWRefCon((WindowPtr)pw->handle.window) == WM_MODAL)
				dOptions.modality = kWindowModalityAppModal;
			else
			{
				dOptions.parentWindow = (WindowPtr)pw->handle.window;
				parentwindowstack = MCdispatcher->findstackd(pw);
			}
		}
	}
	else
		dOptions.modality = kWindowModalityAppModal;
	dOptions.optionFlags &= ~kNavAllowMultipleFiles;
	dOptions.optionFlags |= kNavNoTypePopup | kNavSupportPackages;
	dOptions.optionFlags &= ~kNavAllowPreviews; //Clear preview option
	
	//make descriptor for default location
	// MH-2007-29-05: Deprecating the use of FSSpec in favour of FSRef
	if (hasDefaultPath)
		anErr = AECreateDesc(typeFSRef, p_default_dir_fsref, sizeof(FSRef), &defaultLoc);
	
	NavDialogRef navdialogref;
	defaultlocationptr = NULL;
	if (hasDefaultPath)
		defaultlocationptr =  &defaultLoc;
	anErr = NavCreateChooseFolderDialog(&dOptions, eventProc, NULL,
	                                    NULL, &navdialogref);
	
	if (anErr == noErr && navdialogref != NULL)
	{
		navfilepath = NULL;
		navdialogbusy = True;
		anErr = NavDialogRun(navdialogref);
		
		while (navdialogbusy)
			MCscreen->wait(REFRESH_INTERVAL, True, True);
		
		if (anErr != noErr)
		{
			NavDialogDispose(navdialogref);
			navdialogref = NULL;
		}
		
		if (navfilepath)
            /* UNCHECKED */ MCStringCreateWithCStringAndRelease((char_t*)navfilepath, r_value);
		else
            MCStringCreateWithCString(MCcancelstring, r_result);
	}
	if (hasDefaultPath)
		AEDisposeDesc(&defaultLoc);
	if (dOptions.clientName)
		CFRelease(dOptions.clientName);
	DisposeNavEventUPP(eventProc);
	
	// MW-2009-01-22: [[ Bug 3509 ]] Make sure we force an update to the menubar, just in case
	//   the dialog has messed with our menu item enabled state!
	if (dOptions.modality == kWindowModalityWindowModal)
		MCscreen -> updatemenubar(True);
	
	return anErr;
}


static void get_default_path(const char *p_initial, Boolean& hasDefaultPath, FSSpec& openDirFSpec)
{
	const char *t_leaf = p_initial != NULL ? strrchr(p_initial, '/') : NULL;
	if (t_leaf != NULL)
	{
		char *newfile = new char[t_leaf - p_initial + 1];
		strncpy(newfile, t_leaf, t_leaf - p_initial);
		newfile[t_leaf - p_initial] = '\0';
		MCS_path2FSSpec(newfile, &openDirFSpec);
		delete newfile;
		hasDefaultPath = True;
	}
}

// MW-2005-05-23: Update to new API
int MCA_folder_tiger(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	Cursor c;
	
	if (!MCModeMakeLocalWindows())
    {
        MCAutoStringRef t_resolved_initial_path_str;        
        MCS_resolvepath(p_initial, &t_resolved_initial_path_str);
		
		MCRemoteFolderDialog(p_title, p_prompt, *t_resolved_initial_path_str, r_value);
        
		return 0;
	}
	
	SetCursor(GetQDGlobalsArrow(&c)); // bug in OS X doesn't reset this
	ShowCursor();
	MCscreen->expose();
	FSRef t_defaultfolder;
	Boolean hasDefaultPath = False;
	Boolean usens = !MCdontuseNS && NavServicesAvailable();
	
	// MW-2005-09-27: Fix to ensure p_initial is treated as a folder path, rather than a file path
	if (p_initial != NULL && MCStringGetLength(p_initial) != 0)
	{
        /* UNCHECKED */ MCS_mac_pathtoref(p_initial, t_defaultfolder);
		hasDefaultPath = True;
	}
	
	if (usens)
	{
		navAnswerFolder(p_prompt, hasDefaultPath, &t_defaultfolder, (p_options & MCA_OPTION_SHEET) != 0, r_value, r_result);
		MCscreen->expose();
		EventRecord e;
		WaitNextEvent(mUpMask, &e, 0, NULL);
	}
	
	return 0;
}
