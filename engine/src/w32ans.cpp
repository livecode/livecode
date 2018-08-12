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

#include "prefix.h"

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"
#include "mcio.h"


#include "mcerror.h"
#include "ans.h"
#include "stack.h"
#include "stacklst.h"
#include "dispatch.h"
#include "globals.h"
#include "util.h"
#include "mode.h"
#include "osspec.h"

#include "meta.h"

#include "w32text.h"

#include "malloc.h"

#include <strsafe.h>

// We need the Vista versions of the shell headers
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_VISTA
#include <shobjidl.h>
#include <shlobj.h>
#include <shlwapi.h>

extern void MCRemoteFileDialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint32_t p_type_count, MCStringRef p_initial_folder, MCStringRef p_initial_file, bool p_save, bool p_files, MCStringRef &r_value);
extern void MCRemoteFolderDialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef &r_value);
extern void MCRemoteColorDialog(MCStringRef p_title, uint32_t p_red, uint32_t p_green, uint32_t p_blue, bool& r_chosen, MCColor& r_chosen_color);

extern bool MCStringsSplit(MCStringRef p_string, codepoint_t p_separator, MCStringRef*&r_strings, uindex_t& r_count);

static void getfilter(MCStringRef p_filter, MCStringRef &r_filter)
{
	if (p_filter != nil && !MCStringIsEmpty(p_filter))
	{
		// SN-2014-10-29: [[ Bug 13850 ]] Remove the static variable - t_filterstring::m_value was not set to NULL
		//  and triggered MCAssert(false).
		MCAutoStringRef t_filterstring;

		/* UNCHECKED */ MCStringMutableCopy(p_filter, &t_filterstring);
		
		uindex_t t_offset;

		if (!MCStringFirstIndexOfChar(*t_filterstring, '\n', 0, kMCStringOptionCompareExact, t_offset) &&
				!MCStringFirstIndexOfChar(*t_filterstring, ',', 0, kMCStringOptionCompareExact, t_offset))
		{
			MCStringAppendChar(*t_filterstring, '\0');
			MCStringAppend(*t_filterstring, p_filter);
		}

		/* UNCHECKED */ MCStringAppendChar(*t_filterstring, '\0');
		/* UNCHECKED */ MCStringFindAndReplaceChar(*t_filterstring, '\n', '\0', kMCStringOptionCompareExact);
		/* UNCHECKED */ MCStringFindAndReplaceChar(*t_filterstring, ',', '\0', kMCStringOptionCompareExact);

		MCStringCopy(*t_filterstring, r_filter);
	}
	else
		/* UNCHECKED */ MCStringCreateWithNativeChars((const char_t *)"All Files (*.*)\0*.*\0", 20, r_filter);
}

static void waitonbutton()
{
	int key = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
	if (GetAsyncKeyState(key) & 0x8000)
	{ // double clicked
		MSG msg;
		while (GetAsyncKeyState(key) & 0x8000)
			PeekMessageW(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE);
		// pull extra mouseUp out of queue
		while (PeekMessageW(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
			;
	}
	// SMR 1880 clear shift/control state
	MCmodifierstate = MCscreen->querymods();
}

static MCAutoArray<unichar_t> s_chosen_folder;
static MCAutoArray<unichar_t> s_chosen_files;

static UINT_PTR CALLBACK open_dialog_hook(HWND p_dialog, UINT p_message, WPARAM p_wparam, LPARAM p_lparam)
{	
	if (p_message != WM_NOTIFY)
		return 0;

	switch(((OFNOTIFYW *)p_lparam) -> hdr . code)
	{
		case CDN_FILEOK:
		case CDN_SELCHANGE:
		{
			int t_length;
			t_length = SendMessageW(GetParent(p_dialog), CDM_GETSPEC, (WPARAM)0, (LPARAM)NULL);
			if (t_length >= 0)
			{
				s_chosen_files.Delete();
                /* UNCHECKED */ s_chosen_files.New(t_length);
				SendMessageW(GetParent(p_dialog), CDM_GETSPEC, (WPARAM)t_length, (LPARAM)s_chosen_files.Ptr());
			}
			t_length = SendMessageW(GetParent(p_dialog), CDM_GETFOLDERPATH, (WPARAM)0, (LPARAM)NULL);
			if (t_length >= 0)
			{
				s_chosen_folder.Delete();
                /* UNCHECKED */ s_chosen_folder.New(t_length);
				SendMessageW(GetParent(p_dialog), CDM_GETFOLDERPATH, (WPARAM)t_length, (LPARAM)s_chosen_folder.Ptr());
			}
		}
		break;

		default:
		break;
	}

	return 0;
}

// MW-2008-03-18: [[ Bug 3300 ]] Make sure that if a file has a full path we dont' prepend
//   the selected folder.
// MW-2008-03-18: [[ Bug 6116 ]] Make sure that we don't add an extra slash if there's already
//   one at the end of the folder path.
static void build_path(MCStringRef p_folder, MCStringRef p_file, MCStringRef x_path)
{
	MCAutoStringRef t_path, t_engine_path;
	MCStringCreateMutable(0, &t_path);

	MCS_pathfromnative(p_file, &t_engine_path);

	// Check for absolute paths
	bool t_use_folder;
	if (MCStringIsEmpty(p_folder)
		|| (MCStringGetLength(*t_engine_path) > 1 && MCStringGetCharAtIndex(*t_engine_path, 1) == ':')
		|| (MCStringGetLength(*t_engine_path) > 2 && MCStringGetCharAtIndex(*t_engine_path, 0) == '/' && MCStringGetCharAtIndex(*t_engine_path, 1) == '/'))
	{
		t_use_folder = false;
	}
	else
	{
		t_use_folder = true;
	}

	if (t_use_folder)
	{
		// Add the folder and a separator, if required
		MCStringAppend(*t_path, p_folder);
		if (MCStringGetCharAtIndex(p_folder, MCStringGetLength(p_folder) - 1) != '/')
			MCStringAppendChar(*t_path, '/');
	}

	MCStringAppend(*t_path, *t_engine_path);

	if (*t_path != nil)
		MCStringAppend(x_path, *t_path);
}

static void build_paths(MCStringRef &r_path)
{
	MCAutoStringRef t_path;
	/* UNCHECKED */ MCStringCreateMutable(0, &t_path);
	MCAutoStringRef t_std_path;
	MCAutoStringRef t_native_path;
	/* UNCHECKED */ MCStringCreateWithChars(s_chosen_folder.Ptr(), s_chosen_folder.Size()-1, &t_native_path);
	/* UNCHECKED */ MCS_pathfromnative(*t_native_path, &t_std_path);

	if (MCStringGetCharAtIndex(*t_std_path, 0) == '"')
	{
		// Does this ever actually receive a quoted path?
		/*Meta::itemised_string t_items(sg_chosen_files, ' ', true);
		for(unsigned int t_index = 0; t_index < t_items . count(); ++t_index)
		{
			if (t_index != 0)
				/* UNCHECKED * / MCStringAppendChar(*t_path, '\n');

			build_path(*t_std_path, t_items[t_index], *t_path);
		}*/
		MCAutoStringRef t_item;
		/* UNCHECKED */ MCStringCreateWithChars(s_chosen_files.Ptr(), s_chosen_files.Size()-1, &t_item);
		build_path(*t_std_path, *t_item, *t_path);
	}
	else
	{
		MCAutoStringRef t_files;
		/* UNCHECKED */ MCStringCreateWithChars(s_chosen_files.Ptr(), s_chosen_files.Size()-1, &t_files);
		build_path(*t_std_path, *t_files, *t_path);
	}

	s_chosen_files.Delete();
	s_chosen_folder.Delete();

	MCStringCopy(*t_path, r_path);
}

static HRESULT append_shellitem_path_and_release(IShellItem *p_item, bool p_first, MCStringRef &x_string)
{
	HRESULT t_hresult;
	t_hresult = S_OK;

	bool t_succeeded;
	t_succeeded = true;

	WCHAR *t_filename;
	t_filename = NULL;
	if (t_succeeded)
	{
		t_hresult = p_item -> GetDisplayName(SIGDN_FILESYSPATH, &t_filename);
		t_succeeded = SUCCEEDED(t_hresult);
	}
	
	if (t_succeeded)
	{
		if (x_string == nil)
			MCStringCreateMutable(0, x_string);
		else if (!MCStringIsMutable(x_string))
		{
			MCStringRef t_clone;
			MCStringMutableCopy(x_string, t_clone);
			MCValueAssign(x_string, t_clone);
		}

		MCAutoStringRef t_rev_filename;
		MCAutoStringRef t_native_filename;
		
		/* UNCHECKED */ MCStringCreateWithChars(t_filename, lstrlenW(t_filename), &t_native_filename);
		/* UNCHECKED */ MCS_pathfromnative(*t_native_filename, &t_rev_filename);
		/* UNCHECKED */ MCStringAppendFormat(x_string, p_first ? "%@" : "\n%@", *t_rev_filename);
	}

	if (t_filename != NULL)
		CoTaskMemFree(t_filename);

	if (p_item != NULL)
		p_item -> Release();

	return t_hresult;
}

static void measure_filter(MCStringRef p_filter, uint4& r_length, uint4& r_count)
{
	uint4 t_count;
	t_count = 0;

	uint4 t_length;
	t_length = 0;

	uindex_t t_offset;
	t_offset = 0;
	do
	{
		t_count += 1;

		while(MCStringGetCharAtIndex(p_filter, t_offset) != '\0')
			t_offset++, t_length++;

		t_offset++;
		t_length++;
	}
	while(MCStringGetCharAtIndex(p_filter, t_offset) != '\0');

	r_length = t_length + 1;
	r_count = (t_count + 1) / 2;
}

static void filter_to_spec(const wchar_t *p_filter, uint4 p_filter_count, COMDLG_FILTERSPEC*& r_types)
{
	r_types = new (nothrow) COMDLG_FILTERSPEC[p_filter_count];
	memset(r_types, 0, sizeof(COMDLG_FILTERSPEC) * p_filter_count);

	uint4 t_count;
	t_count = 0;

	const wchar_t *t_filter;
	t_filter = p_filter;

	do
	{
		if ((t_count % 2) == 0)
			r_types[t_count / 2] . pszName = t_filter;
		else
			r_types[t_count / 2] . pszSpec = t_filter;

		while(*t_filter != L'\0')
			t_filter++;

		t_filter++;
		t_count += 1;
	}
	while(*t_filter != L'\0');
}

typedef HRESULT (WINAPI *SHCreateItemFromParsingNamePtr)(PCWSTR pszPath, IBindCtx *pbc, REFIID riid, void **ppv);

// In the case of a save dialog <p_initial> is one of:
//     <folder>/<file>
//     <folder>[/]
//     <file>
//
// In the case of an open dialog <p_initial> is a folder
//

// SN-2015-03-13: [[ Bug 14611 ]] Update funtion to StringRefs
bool MCU_w32path2std(MCStringRef p_path, MCStringRef &r_std_path)
{
	MCAutoStringRef t_std_path;

	if (!MCStringMutableCopy(p_path, &t_std_path))
		return false;

	if (!MCStringFindAndReplaceChar(*t_std_path, '\\', '/', kMCStringOptionCompareExact))
		return false;

	return MCStringCopy(*t_std_path, r_std_path);
}

static int MCA_do_file_dialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	int t_result = 0;

	MCAutoStringRef t_initial_file;
	MCAutoStringRef t_initial_folder;
    MCAutoStringRef t_initial_native_folder;

	if (p_initial != nil && !MCStringIsEmpty(p_initial))
	{
		MCAutoStringRef t_fixed_path;
		MCAutoStringRef t_std_path;

		// SN-2015-03-13: [[ Bug 14611 ]] Reinstate former behaviour, that is to 
		//  fix backslash-delimited paths
		if (!MCU_w32path2std(p_initial, &t_std_path))
			return ERROR_OUTOFMEMORY;

		/* UNCHECKED */ MCU_fix_path(*t_std_path, &t_fixed_path);

		if (MCS_exists(*t_fixed_path, False))
			t_initial_folder = *t_fixed_path;
		else if ((p_options & MCA_OPTION_SAVE_DIALOG) != 0)
		{
			uindex_t t_last_slash;
			if (!MCStringLastIndexOfChar(*t_fixed_path, '/', UINDEX_MAX, kMCStringOptionCompareExact, t_last_slash))
			{
				if (MCStringGetLength(*t_fixed_path) != 0)
					t_initial_file = *t_fixed_path;
			}
			else
			{
				if (t_last_slash < MCStringGetLength(*t_fixed_path) - 1)
					/* UNCHECKED */ MCStringCopySubstring(*t_fixed_path, MCRangeMakeMinMax(t_last_slash + 1, MCStringGetLength(*t_fixed_path)), &t_initial_file);

				MCAutoStringRef t_folder_split;
				// SN-2014-10-29: [[ Bug 13850 ]] The length is t_last_slash, not t_last_slash - 1
				/* UNCHECKED */ MCStringCopySubstring(*t_fixed_path, MCRangeMake(0, t_last_slash), &t_folder_split);
				if (MCS_exists(*t_folder_split, False))
					t_initial_folder = *t_folder_split;
			}
		}
		else
		{
			uindex_t t_last_slash;

			if (MCStringLastIndexOfChar(*t_fixed_path, '/', UINDEX_MAX, kMCStringOptionCompareExact, t_last_slash))
			{
				MCAutoStringRef t_folder_split;
				// SN-2014-10-29: [[ Bug 13850 ]] The length is t_last_slash, not t_last_slash - 1
				/* UNCHECKED */ MCStringCopySubstring(*t_fixed_path, MCRangeMake(0, t_last_slash), &t_folder_split);
				
				if (MCS_exists(*t_folder_split, False))
					t_initial_folder = *t_folder_split;
			}
		}
        
        MCAutoStringRef t_resolved_folder;
        /* UNCHECKED */ MCS_resolvepath(*t_initial_folder != nil ? *t_initial_folder : kMCEmptyString, &t_resolved_folder);
        /* UNCHECKED */ MCS_pathtonative(*t_resolved_folder, &t_initial_native_folder);
	}
    
	if (!MCModeMakeLocalWindows())
	{
		MCAutoStringRefArray t_filters;

		if (p_filter != NULL)
		{
			/* UNCHECKED */ MCStringsSplit(p_filter, '\0', t_filters.PtrRef(), t_filters.CountRef());
		}

		MCRemoteFileDialog(p_title, p_prompt, *t_filters, t_filters.Count(), *t_initial_native_folder, *t_initial_file, (p_options & MCA_OPTION_SAVE_DIALOG) != 0, (p_options & MCA_OPTION_PLURAL) != 0, r_value);

		return 0;
	}

	Window t_window;
	t_window = MCModeGetParentWindow();

	MCAutoStringRef t_value;
	bool t_succeeded;
	int t_filter_index;

	if (MCmajorosversion >= 0x0600)
	{
		static SHCreateItemFromParsingNamePtr  s_shcreateitemfromparsingname = NULL;
		if (s_shcreateitemfromparsingname == NULL)
		{
			static HMODULE s_shell32_module = NULL;
			s_shell32_module = LoadLibraryW(L"shell32.dll");
			s_shcreateitemfromparsingname = (SHCreateItemFromParsingNamePtr)GetProcAddress(s_shell32_module, "SHCreateItemFromParsingName");
		}

		IFileSaveDialog *t_file_save_dialog;
		IFileOpenDialog *t_file_open_dialog;
		IFileDialog *t_file_dialog;

		t_file_dialog = NULL;

		HRESULT t_hresult;

		if ((p_options & MCA_OPTION_SAVE_DIALOG) == 0)
		{
			t_hresult = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, __uuidof(IFileOpenDialog), (LPVOID *)&t_file_open_dialog);
			t_succeeded = SUCCEEDED(t_hresult);

			t_file_dialog = t_file_open_dialog;
		}
		else
		{
			t_hresult = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, __uuidof(IFileSaveDialog), (LPVOID *)&t_file_save_dialog);
			t_succeeded = SUCCEEDED(t_hresult);

			t_file_dialog = t_file_save_dialog;
		}

		if (t_succeeded)
		{
			DWORD t_options;

			t_options = FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR | FOS_PATHMUSTEXIST;
			if (p_options & MCA_OPTION_PLURAL)
				t_options |= FOS_ALLOWMULTISELECT;
			if (p_options & MCA_OPTION_SAVE_DIALOG)
				t_options |= FOS_OVERWRITEPROMPT;
			if (p_options & MCA_OPTION_FOLDER_DIALOG)
				t_options |= FOS_PICKFOLDERS;
			else
				t_options |= FOS_FILEMUSTEXIST;

			t_hresult = t_file_dialog -> SetOptions(t_options);
			t_succeeded = SUCCEEDED(t_hresult);
		}

		if (t_succeeded && *t_initial_native_folder != NULL)
		{
			IShellItem *t_initial_folder_shellitem;
			t_initial_folder_shellitem = NULL;

			MCAutoStringRefAsWString t_initial_folder_wstr;
			/* UNCHECKED */ t_initial_folder_wstr.Lock(*t_initial_native_folder);

			t_hresult = s_shcreateitemfromparsingname(*t_initial_folder_wstr, NULL, __uuidof(IShellItem), (LPVOID *)&t_initial_folder_shellitem);
			if (SUCCEEDED(t_hresult))
				t_file_dialog -> SetFolder(t_initial_folder_shellitem);
			if (t_initial_folder_shellitem != NULL)
				t_initial_folder_shellitem -> Release();
			t_succeeded = SUCCEEDED(t_hresult);
		}

		if (t_succeeded && *t_initial_file != NULL)
		{
			MCAutoStringRefAsWString t_initial_file_wstr;
			/* UNCHECKED */ t_initial_file_wstr.Lock(*t_initial_file);
			
			t_hresult = t_file_dialog -> SetFileName(*t_initial_file_wstr);
			t_succeeded = SUCCEEDED(t_hresult);
		}

		if (t_succeeded && p_filter != NULL && (p_options & MCA_OPTION_FOLDER_DIALOG) == 0)
		{
			uint4 t_filter_length, t_filter_count;
			measure_filter(p_filter, t_filter_length, t_filter_count);

			MCAutoStringRefAsWString t_filter_wstr;
			/* UNCHECKED */ t_filter_wstr.Lock(p_filter);

			COMDLG_FILTERSPEC *t_filter_spec;

			filter_to_spec(*t_filter_wstr, t_filter_count, t_filter_spec);

			t_hresult = t_file_dialog -> SetFileTypes(t_filter_count, t_filter_spec);
			t_succeeded = SUCCEEDED(t_hresult);

			delete t_filter_spec;
		}

		if (t_succeeded && p_filter != NULL && (p_options & MCA_OPTION_FOLDER_DIALOG) == 0)
		{
			t_hresult = t_file_dialog -> SetFileTypeIndex(1);
			t_succeeded = SUCCEEDED(t_hresult);
		}

		if (t_succeeded)
		{
			MCAutoStringRefAsWString t_prompt_wstr;
			/* UNCHECKED */ t_prompt_wstr.Lock(p_prompt);
			t_hresult = t_file_dialog -> SetTitle(*t_prompt_wstr);
		}

		if (t_succeeded)
		{
			t_hresult = t_file_dialog -> Show(t_window != NULL ? (HWND)t_window -> handle . window : NULL);
			t_succeeded = SUCCEEDED(t_hresult);
		}

		if ((p_options & MCA_OPTION_SAVE_DIALOG) == 0)
		{
			IShellItemArray *t_file_items;
			t_file_items = NULL;
			if (t_succeeded)
			{
				t_hresult = t_file_open_dialog -> GetResults(&t_file_items);
				t_succeeded = SUCCEEDED(t_hresult);
			}

			DWORD t_file_item_count;
			if (t_succeeded)
			{
				t_hresult = t_file_items -> GetCount(&t_file_item_count);
				t_succeeded = SUCCEEDED(t_hresult);
			}

			if (t_succeeded)
			{
				for(uint4 t_index = 0; t_index < t_file_item_count && t_succeeded; ++t_index)
				{
					IShellItem *t_file_item;
					t_file_item = NULL;
					if (t_succeeded)
					{
						t_hresult = t_file_items -> GetItemAt(t_index, &t_file_item);
						t_succeeded = SUCCEEDED(t_hresult);
					}

					if (t_succeeded)
					{
						t_hresult = append_shellitem_path_and_release(t_file_item, t_index == 0, &t_value);
						t_succeeded = SUCCEEDED(t_hresult);
					}
				}
			}

			if (t_file_items != NULL)
				t_file_items -> Release();
		}
		else
		{
			IShellItem *t_file_item;
			t_file_item = NULL;
			if (t_succeeded)
			{
				t_hresult = t_file_dialog -> GetResult(&t_file_item);
				t_succeeded = SUCCEEDED(t_hresult);
			}

			if (t_succeeded)
			{
				t_hresult = append_shellitem_path_and_release(t_file_item, true, &t_value);
				t_succeeded = SUCCEEDED(t_hresult);
			}
		}

		t_filter_index = 0;
		if (t_succeeded && (p_options & MCA_OPTION_FOLDER_DIALOG) == 0)
		{
			UINT t_index;
			t_hresult = t_file_dialog -> GetFileTypeIndex(&t_index);
			t_succeeded = SUCCEEDED(t_hresult);
			if (t_succeeded)
				t_filter_index = (int)t_index;
		}

		if (t_file_dialog != NULL)
			t_file_dialog -> Release();

		if (!t_succeeded)
			t_result = t_hresult;
		else
			t_result = 0;
	}
	else
	{
		OPENFILENAMEW t_open_dialog;
		memset(&t_open_dialog, 0, sizeof(OPENFILENAMEW));
		t_open_dialog . lStructSize = sizeof(OPENFILENAMEW);

		MCAutoStringRefAsWString t_initial_folder_wstr;
		MCAutoStringRefAsWString t_prompt_wstr;
		MCAutoStringRefAsWString t_filter_wstr;
		/* UNCHECKED */ t_filter_wstr.Lock(p_filter);
		/* UNCHECKED */ t_initial_folder_wstr.Lock(*t_initial_native_folder);
		/* UNCHECKED */ t_prompt_wstr.Lock(p_prompt);

		MCAutoArray<unichar_t> t_buffer;
		/* UNCHECKED */ t_buffer.New(MAX_PATH);

		if (!MCStringIsEmpty(*t_initial_file))
			/* UNCHECKED */ MCStringGetChars(*t_initial_file, MCRangeMake(0, t_buffer.Size()), t_buffer.Ptr());
		else
			t_buffer[0] = '\0';

		t_open_dialog . lpstrFilter = *t_filter_wstr;
		t_open_dialog . nFilterIndex = 1;
		t_open_dialog . lpstrFile = t_buffer.Ptr();
		t_open_dialog . nMaxFile = t_buffer.Size();
		t_open_dialog . lpstrInitialDir = *t_initial_folder_wstr;
		t_open_dialog . lpstrTitle = *t_prompt_wstr;
		t_open_dialog . Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR |
														OFN_LONGNAMES | OFN_PATHMUSTEXIST | OFN_EXPLORER |
														OFN_ENABLEHOOK | OFN_ENABLESIZING;

		if (p_options & MCA_OPTION_PLURAL)
			t_open_dialog . Flags |= OFN_ALLOWMULTISELECT;

		if (p_options & MCA_OPTION_SAVE_DIALOG)
			t_open_dialog . Flags |= OFN_OVERWRITEPROMPT;

		t_open_dialog . lpstrFilter = *t_filter_wstr;
		t_open_dialog . lpfnHook = open_dialog_hook;
		t_open_dialog . hwndOwner = t_window != NULL ? (HWND)t_window -> handle . window : NULL;

		if (p_options & MCA_OPTION_SAVE_DIALOG)
			t_succeeded = GetSaveFileNameW(&t_open_dialog) == TRUE;
		else
		{
			*t_open_dialog . lpstrFile = '\0';
			t_succeeded = GetOpenFileNameW(&t_open_dialog) == TRUE;
		}

		if (!t_succeeded)
			t_result = CommDlgExtendedError();

		// MW-2005-07-26: Try again without the specified filename if it was invalid
		if (t_result == FNERR_INVALIDFILENAME)
		{
			*t_open_dialog . lpstrFile = '\0';
			if (p_options & MCA_OPTION_SAVE_DIALOG)
				t_succeeded = GetSaveFileNameW(&t_open_dialog) == TRUE;
			else
				t_succeeded = GetOpenFileNameW(&t_open_dialog) == TRUE;

			if (!t_succeeded)
				t_result = CommDlgExtendedError();	
		}

		if (t_result == FNERR_BUFFERTOOSMALL)
			t_succeeded = true;

		if (t_succeeded)
		{
			build_paths(&t_value);
			t_filter_index = t_open_dialog . nFilterIndex;
		}
	}

	if (t_succeeded)
	{
		if (p_options & MCA_OPTION_RETURN_FILTER)
		{
			// The filter string has the following format:
			// "<description0>\0<extensions0>\0<description1>\0...\0<extensionsN>\0"
			// so the n'th filter comes after the 2(n - 1)'th null character
			uindex_t t_index = 2 * (t_filter_index - 1);
			uindex_t t_offset = 0;
			while (t_index--)
			{
				/* UNCHECKED */ MCStringFirstIndexOfChar(p_filter, '\0', t_offset, kMCStringOptionCompareExact, t_offset);
				t_offset++;
			}
			
			uindex_t t_end;
			t_end = UINDEX_MAX;
			/* UNCHECKED */ MCStringFirstIndexOfChar(p_filter, '\0', t_offset, kMCStringOptionCompareExact, t_end);
            
			/* UNCHECKED */ MCStringCopySubstring(p_filter, MCRangeMakeMinMax(t_offset, t_end), r_result);
		}

		t_result = 0;
		r_value = MCValueRetain(*t_value);
	}
	else
		r_result = MCValueRetain(MCNameGetString(MCN_cancel));

	waitonbutton();

	return t_result;
}

static void get_new_filter(MCStringRef *p_types, uint4 p_type_count, MCStringRef &r_filters)
{
	MCAutoStringRef t_filters;
	/* UNCHECKED */ MCStringCreateMutable(0, &t_filters);

	for(uint4 t_type_index = 0; t_type_index < p_type_count; ++t_type_index)
	{
		MCAutoStringRefArray t_split;
		/* UNCHECKED */ MCStringsSplit(p_types[t_type_index], '|', t_split.PtrRef(), t_split.CountRef());

		if (t_split.Count() < 1 || 
			(t_split.Count() == 1 && MCStringIsEmpty(t_split[0])))
			continue;

		if (t_type_index != 0)
			/* UNCHECKED */ MCStringAppendChar(*t_filters, '\0');

		/* UNCHECKED */ MCStringAppend(*t_filters, t_split[0]);

		if (t_split.Count() < 2)
			/* UNCHECKED */ MCStringAppendChars(*t_filters, L"\0*.*", 4);
		else
		{
			MCAutoStringRefArray t_extensions;
			/* UNCHECKED */ MCStringsSplit(t_split[1], ',', t_extensions.PtrRef(), t_extensions.CountRef());
			// SN-2014-07-28: [[ Bug 12972 ]] Filters "Tag|" should be understood as "Tag"
			//  and allow all the file types
			if (t_extensions.Count() == 0 || 
					(t_extensions.Count() == 1 && MCStringIsEmpty(t_extensions[0])))
				/* UNCHECKED */ MCStringAppendChars(*t_filters, L"\0*.*", 4);
			else
			{
				for (unsigned int i = 0; i < t_extensions.Count(); ++i)
				{
					if (i != 0)
						/* UNCHECKED*/ MCStringAppendChar(*t_filters, ';');
					else
						/* UNCHECKED*/ MCStringAppendChar(*t_filters, '\0');

					/* UNCHECKED */ MCStringAppendFormat(*t_filters, "*.%@", t_extensions[i]);
				}
			}
		}
	}

	if (MCStringIsEmpty(*t_filters))
		/* UNCHECKED */ MCStringCreateWithNativeChars((char_t*)"All Files\0*.*\0\0", 15, r_filters);
	else
	{
		/* UNCHECKED */ MCStringAppendChar(*t_filters, '\0');
		MCStringCopy(*t_filters, r_filters);
	}
}

// MW-2005-05-15: New answer file with types call
int MCA_file_with_types(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	MCAutoStringRef t_filters;
	get_new_filter(p_types, p_type_count, &t_filters);

	return MCA_do_file_dialog(p_title == NULL ? kMCEmptyString : p_title, p_prompt == NULL ? kMCEmptyString : p_prompt, *t_filters, p_initial, p_options | MCA_OPTION_RETURN_FILTER, r_value, r_result);
}

// MW-2005-05-15: Updated for new answer command restructuring
int MCA_file(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	MCAutoStringRef t_filter;
	getfilter(p_filter, &t_filter);
	return MCA_do_file_dialog(p_title == NULL ? kMCEmptyString : p_title, p_prompt == NULL ? kMCEmptyString : p_prompt, *t_filter, p_initial, p_options, r_value, r_result);
}


INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	char szDir[MAX_PATH];
	switch(uMsg)
	{
	case BFFM_INITIALIZED:
		if (pData)
			// WParam is TRUE since you are passing a path.
			// It would be FALSE if you were passing a pidl.
			SendMessageA(hwnd, BFFM_SETSELECTIONA, TRUE, (LPARAM)pData);
		break;

	case BFFM_SELCHANGED:
		// Set the status window to the currently selected path.
		if (SHGetPathFromIDListA((LPITEMIDLIST) lp ,szDir))
			SendMessageA(hwnd,BFFM_SETSTATUSTEXTA,0,(LPARAM)szDir);
		break;
	}
	return 0;
}

typedef HRESULT (CALLBACK *dll_get_version_t)(DLLVERSIONINFO *p_info);
static unsigned int get_dll_version(const wchar_t *p_dll)
{
	HMODULE t_module;
	t_module = LoadLibraryW(p_dll);
	if (t_module == NULL)
		return 0;

	dll_get_version_t t_get_version;
	t_get_version = (dll_get_version_t)GetProcAddress(t_module, "DllGetVersion");
	if (t_get_version == NULL)
		return 0;

	DLLVERSIONINFO p_info;
	p_info . cbSize = sizeof(DLLVERSIONINFO);
	if (t_get_version(&p_info) != NOERROR)
		return 0;

	FreeLibrary(t_module);

	return p_info . dwMajorVersion * 100 + p_info . dwMinorVersion;
}

// MW-2005-05-15: Updated for new answer command restructuring
int MCA_folder(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	if (MCmajorosversion >= 0x0600 && MCModeMakeLocalWindows())
		return MCA_file(p_title, p_prompt, nil, p_initial, p_options | MCA_OPTION_FOLDER_DIALOG, r_value, r_result);

// MW-2005-05-27: We'll use a static (I know bad me) to store the version
//   of the shell dll.
	static int s_shell_version = -1;
	static MCStringRef s_last_folder = MCValueRetain(kMCEmptyString);

	MCAutoStringRef t_native_filename;

	if (p_initial != NULL)
	{
		MCAutoStringRef t_std_path;

		/* UNCHECKED */ MCS_pathfromnative(p_initial, &t_std_path);
		t_native_filename = *t_std_path;
	}
	else
		t_native_filename = MCValueRetain(s_last_folder);

	if (!MCModeMakeLocalWindows())
    {
		MCAutoStringRef t_answer_path;
		MCRemoteFolderDialog(p_title, p_prompt, *t_native_filename, &t_answer_path);
        if (*t_answer_path != nil)
		{
			MCAutoStringRef t_std_path;

			/* UNCHECKED */ MCS_pathfromnative(*t_answer_path, &t_std_path);
			MCValueAssign(s_last_folder, *t_std_path);
		}
		r_value = MCValueRetain(*t_answer_path);
		return 0;
	}

	if (s_shell_version == -1)
		s_shell_version = get_dll_version(L"shell32.dll");

	bool sheet = (p_options & MCA_OPTION_SHEET) != 0;

	BROWSEINFOW bi;
	memset(&bi, 0, sizeof(BROWSEINFOW));

	Window pw;
	pw = MCModeGetParentWindow();

	if (pw != DNULL)
		bi.hwndOwner = (HWND)pw->handle.window;

	MCAutoStringRefAsWString t_prompt_wstr;
	MCAutoStringRefAsWString t_native_filename_wstr;
	/* UNCHECKED */ t_prompt_wstr.Lock(p_prompt);

	bi.pidlRoot = NULL;
	bi.lpszTitle = *t_prompt_wstr;
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	if (s_shell_version >= 500)
		bi.ulFlags |= BIF_NEWDIALOGSTYLE;
	if (*t_native_filename != nil && !MCStringIsEmpty(*t_native_filename))
	{
		t_native_filename_wstr.Lock(*t_native_filename);
		
		bi . lpfn = BrowseCallbackProc;
		bi . lParam = (LPARAM)*t_native_filename_wstr;
	}
	else
	{
		bi.lpfn = NULL;
		bi.lParam = NULL;
	}
	LPITEMIDLIST lpiil;
	LPMALLOC lpm;
	SHGetMalloc(&lpm);

	DWORD t_error;
	lpiil = SHBrowseForFolderW(&bi);
	if (lpiil == NULL)
	{
		t_error = GetLastError();
	}
	
	MCAutoArray<unichar_t> t_buffer;
	/* UNCHECKED */ t_buffer.New(MAX_PATH);

	if (lpiil != NULL && SHGetPathFromIDListW(lpiil, t_buffer.Ptr()))
	{
		if (s_last_folder != NULL)
			MCValueRelease(s_last_folder);

		size_t t_length;
		/* UNCHECKED */ StringCchLengthW(t_buffer.Ptr(), t_buffer.Size(), &t_length);
		/* UNCHECKED */ MCStringCreateWithChars(t_buffer.Ptr(), t_length, s_last_folder);

		MCAutoStringRef t_std_path;
		/* UNCHECKED */ MCS_pathfromnative(s_last_folder, &t_std_path);

		r_value = MCValueRetain(*t_std_path);
	}
	else
		r_result = MCSTR(MCcancelstring);

	//  SMR 1880 clear shift and button state
	waitonbutton();

	lpm->Free(lpiil);
	lpm->Release();

	return 0;
}

// MERG-2013-08-18: Updated to allow script access to colorDialogColors
static COLORREF s_colordialogcolors[16];

/* WRAPPER */
/*bool MCA_folder(bool p_plural, MCStringRef p_prompt, MCStringRef p_initial,
 MCStringRef p_title, bool p_sheet, MCStringRef &r_value, MCStringRef &r_result)
{
	const char *t_title = p_title == nil ? "" : MCStringGetCString(p_title);
	const char *t_prompt = p_prompt == nil ? "" : MCStringGetCString(p_prompt);
	const char *t_initial = p_initial == nil ? "" : MCStringGetCString(p_initial);

	uint32_t t_options = 0;
	if (p_plural)
		t_options |= MCA_OPTION_PLURAL;
	if (p_sheet)
		t_options |= MCA_OPTION_SHEET;

	MCExecPoint ep(nil, nil, nil);
	int t_error;
	t_error = MCA_folder(ep, t_title, t_prompt, t_initial, t_options);
	if (ep.isempty())
		return MCStringCreateWithCString(MCcancelstring, r_result);
	else
		return ep.copyasstringref(r_value);
}*/

// MW-2005-05-15: Updated for new answer command restructuring
bool MCA_color(MCStringRef p_title, MCColor p_initial_color, bool p_as_sheet, bool& r_chosen, MCColor& r_chosen_color)
{
	CHOOSECOLORW chooseclr ;

	memset(&chooseclr, 0, sizeof(CHOOSECOLORW));
	chooseclr.lStructSize = sizeof (CHOOSECOLORW);
	chooseclr.lpCustColors = (LPDWORD)s_colordialogcolors;

	Window t_parent_window;
	t_parent_window = MCModeGetParentWindow();
	chooseclr.hwndOwner = t_parent_window != NULL ? (HWND)t_parent_window -> handle . window : NULL;

	chooseclr.Flags = CC_RGBINIT;
	chooseclr.rgbResult = RGB(p_initial_color.red >> 8, p_initial_color.green >> 8,
	                          p_initial_color.blue >> 8);

	bool t_success = true;
	if (!ChooseColorW(&chooseclr))
	{
		DWORD err = CommDlgExtendedError();
		r_chosen = false;
	}
	else
	{
		r_chosen = true;
		r_chosen_color.red = GetRValue(chooseclr.rgbResult);
		r_chosen_color.red |= r_chosen_color.red << 8;
		r_chosen_color.green = GetGValue(chooseclr.rgbResult);
		r_chosen_color.green |= r_chosen_color.green << 8;
		r_chosen_color.blue = GetBValue(chooseclr.rgbResult);
		r_chosen_color.blue |= r_chosen_color.blue << 8;
	}

	//  SMR 1880 clear shift and button state
	waitonbutton();

	return t_success;
}

void MCA_setcolordialogcolors(MCColor* p_colors, uindex_t p_count)
{
    for(int i = 0; i < 16; i++)
		s_colordialogcolors[i] = RGB(p_colors[i] . red >> 8, p_colors[i] . green >> 8, p_colors[i] . blue >> 8);
}

void MCA_getcolordialogcolors(MCColor*& r_colors, uindex_t& r_count)
{
    MCAutoArray<MCColor> t_list;
    
	for(int i = 0; i < 16; i++)
	{
		MCColor t_color;
		t_color . red = GetRValue(s_colordialogcolors[i]);
		t_color . green = GetGValue(s_colordialogcolors[i]);
		t_color . blue = GetBValue(s_colordialogcolors[i]);
		t_list . Push(t_color);
	}
    
    t_list . Take(r_colors, r_count);
}

int MCA_ask_file_with_types(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	MCAutoStringRef t_filters;
	get_new_filter(p_types, p_type_count, &t_filters);

	return MCA_do_file_dialog(p_title == NULL ? kMCEmptyString : p_title, p_prompt == NULL ? kMCEmptyString : p_prompt, *t_filters, p_initial, p_options | MCA_OPTION_RETURN_FILTER | MCA_OPTION_SAVE_DIALOG, r_value, r_result);
}

// Mw-2005-06-02: Updated to use new answer file prototype
int MCA_ask_file(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	MCAutoStringRef t_filters;
	getfilter(p_filter, &t_filters);

	return MCA_do_file_dialog(p_title == NULL ? kMCEmptyString: p_title, p_prompt == NULL ? kMCEmptyString : p_prompt, *t_filters, p_initial, p_options | MCA_OPTION_SAVE_DIALOG, r_value, r_result);
}
