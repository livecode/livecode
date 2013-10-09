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

#include "w32prefix.h"

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"
#include "mcio.h"

#include "execpt.h"
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

extern void MCRemoteFileDialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint32_t p_type_count, MCStringRef p_initial_folder, MCStringRef p_initial_file, bool p_save, bool p_files, MCStringRef &r_value);
extern void MCRemoteFolderDialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef &r_value);
extern void MCRemoteColorDialog(MCStringRef p_title, uint32_t p_red, uint32_t p_green, uint32_t p_blue, bool& r_chosen, MCColor& r_chosen_color);

extern bool MCStringsSplit(MCStringRef p_string, codepoint_t p_separator, MCStringRef*&r_strings, uindex_t& r_count);

static void getfilter(MCStringRef p_filter, MCStringRef &r_filter)
{
	if (p_filter != nil && MCStringGetLength(p_filter) != 0)
	{
		static MCAutoStringRef t_filterstring;

		if (*t_filterstring != nil)
			MCValueRelease(*t_filterstring);

		/* UNCHECKED */ MCStringMutableCopy(p_filter, &t_filterstring);
		
		uindex_t t_offset;

		if (!MCStringFirstIndexOfChar(*t_filterstring, '\n', 0, kMCStringOptionCompareCaseless, t_offset) &&
				!MCStringFirstIndexOfChar(*t_filterstring, ',', 0, kMCStringOptionCompareCaseless, t_offset))
		{
			MCStringAppendNativeChar(*t_filterstring, '\0');
			MCStringAppend(*t_filterstring, p_filter);
		}

		/* UNCHECKED */ MCStringAppendNativeChar(*t_filterstring, '\0');
		/* UNCHECKED */ MCStringFindAndReplaceChar(*t_filterstring, '\n', '\0', kMCStringOptionCompareCaseless);
		/* UNCHECKED */ MCStringFindAndReplaceChar(*t_filterstring, ',', '\0', kMCStringOptionCompareCaseless);

		/* UNCHECKED */ MCStringCopy(*t_filterstring, r_filter);
	}
	else
		/* UNCHECKED */ MCStringCreateWithNativeChars((char_t*)"All Files (*.*)\0*.*\0\0", 21, r_filter);
}

static void waitonbutton()
{
	int key = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
	if (GetAsyncKeyState(key) & 0x8000)
	{ // double clicked
		MSG msg;
		while (GetAsyncKeyState(key) & 0x8000)
			PeekMessageA(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE);
		// pull extra mouseUp out of queue
		while (PeekMessageA(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
			;
	}
	// SMR 1880 clear shift/control state
	MCmodifierstate = MCscreen->querymods();
}

static Meta::simple_string_buffer sg_chosen_folder;
static Meta::simple_string_buffer sg_chosen_files;

static UINT_PTR CALLBACK open_dialog_hook(HWND p_dialog, UINT p_message, WPARAM p_wparam, LPARAM p_lparam)
{
	if (p_message != WM_NOTIFY)
		return 0;

	switch(((OFNOTIFY *)p_lparam) -> hdr . code)
	{
		case CDN_FILEOK:
		case CDN_SELCHANGE:
		{
			int t_length;
			t_length = SendMessageA(GetParent(p_dialog), CDM_GETSPEC, (WPARAM)0, (LPARAM)NULL);
			if (t_length >= 0)
			{
				sg_chosen_files . resize(t_length == 0 ? 0 : t_length - 1);
				SendMessageA(GetParent(p_dialog), CDM_GETSPEC, (WPARAM)t_length, (LPARAM)*sg_chosen_files);
			}
			t_length = SendMessageA(GetParent(p_dialog), CDM_GETFOLDERPATH, (WPARAM)0, (LPARAM)NULL);
			if (t_length >= 0)
			{
				sg_chosen_folder . resize(t_length == 0 ? 0 : t_length - 1);
				SendMessageA(GetParent(p_dialog), CDM_GETFOLDERPATH, (WPARAM)t_length, (LPARAM)*sg_chosen_folder);
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
static void build_path(const char *p_folder, const MCString& p_file, MCStringRef x_path)
{
	MCAutoStringRef t_path;
	MCStringCreateMutable(0, &t_path);

	MCAutoStringRef t_std_path, t_native_path;
	MCStringCreateWithCString(p_file.getstring(), &t_native_path);
	MCS_pathfromnative(*t_native_path, &t_std_path);

	bool t_use_folder;
	if (p_folder == NULL || strlen(p_folder) == 0 ||
		(MCStringGetLength(*t_std_path) > 1 && MCStringGetNativeCharAtIndex(*t_std_path, 1) == ':') ||
		(MCStringGetLength(*t_std_path) > 2 && MCStringGetNativeCharAtIndex(*t_std_path, 0) == '/' && MCStringGetNativeCharAtIndex(*t_std_path, 1) == '/'))
		t_use_folder = false;
	else
		t_use_folder = true;

	if (t_use_folder)
	{
		MCStringAppendFormat(*t_path, "%s", p_folder);
		if (p_folder[strlen(p_folder) - 1] != '/')
			MCStringAppendChar(*t_path, '/');
	}

	MCStringAppend(*t_path, *t_std_path);

	if (*t_path != nil)
		MCStringAppend(*t_path, x_path);
}

static void build_paths(MCStringRef &r_path)
{
	MCAutoStringRef t_path;
	/* UNCHECKED */ MCStringCreateMutable(0, &t_path);
	MCAutoStringRef t_std_path;
	MCAutoStringRef t_native_path;
	/* UNCHECKED */ MCStringCreateWithCString(*sg_chosen_folder, &t_native_path);
	/* UNCHECKED */ MCS_pathfromnative(*t_native_path, &t_std_path);

	sg_chosen_folder.resize(MCStringGetLength(*t_std_path));
	strcpy(*sg_chosen_folder, MCStringGetCString(*t_std_path));

	if (**sg_chosen_files == '"')
	{
		Meta::itemised_string t_items(sg_chosen_files, ' ', true);
		for(unsigned int t_index = 0; t_index < t_items . count(); ++t_index)
		{
			if (t_index != 0)
				/* UNCHECKED */ MCStringAppendChar(*t_path, '\n');

			build_path(*sg_chosen_folder, t_items[t_index], *t_path);
		}
	}
	else
		build_path(*sg_chosen_folder, *sg_chosen_files, *t_path);

	sg_chosen_files . clear();
	sg_chosen_folder . clear();

	/* UNCHECKED */ MCStringCopy(*t_path, r_path);
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

		/* UNCHECKED */ MCStringCreateWithCString(AnsiCString(t_filename), &t_native_filename);
		/* UNCHECKED */ MCS_pathfromnative(*t_native_filename, &t_rev_filename);
		/* UNCHECKED */ MCStringAppendFormat(x_string, p_first ? "%@" : "\n%@", *t_rev_filename);
	}

	if (t_filename != NULL)
		CoTaskMemFree(t_filename);

	if (p_item != NULL)
		p_item -> Release();

	return t_hresult;
}

static void measure_filter(const char *p_filter, uint4& r_length, uint4& r_count)
{
	uint4 t_count;
	t_count = 0;

	uint4 t_length;
	t_length = 0;

	const char *t_filter;
	t_filter = p_filter;
	do
	{
		t_count += 1;

		while(*t_filter != '\0')
			t_filter++, t_length++;

		t_filter++;
		t_length++;
	}
	while(*t_filter != '\0');

	r_length = t_length + 1;
	r_count = (t_count + 1) / 2;
}

static void filter_to_spec(const wchar_t *p_filter, uint4 p_filter_count, COMDLG_FILTERSPEC*& r_types)
{
	r_types = new COMDLG_FILTERSPEC[p_filter_count];
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

void MCU_w32path2std(char *p_path)
{
	if (p_path == NULL || !*p_path)
		return;

	do 
	{
		if (*p_path == '\\')
			*p_path = '/';
	} while (*++p_path);
}

static int MCA_do_file_dialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	int t_result = 0;

	MCAutoStringRef t_initial_file;
	MCAutoStringRef t_initial_folder;

	if (p_initial != nil && MCStringGetLength(p_initial) != 0)
	{
		MCAutoStringRef t_std_path;
		MCAutoStringRef t_fixed_path;

		/* UNCHECKED */ MCS_pathfromnative(p_initial, &t_std_path);
		/* UNCHECKED */ MCU_fix_path(*t_std_path, &t_fixed_path);

		if (MCS_exists(*t_fixed_path, False))
			t_initial_folder = *t_fixed_path;
		else if ((p_options & MCA_OPTION_SAVE_DIALOG) != 0)
		{
			uindex_t t_last_slash;
			if (!MCStringLastIndexOfChar(*t_fixed_path, '/', UINDEX_MAX, kMCStringOptionCompareCaseless, t_last_slash))
			{
				if (MCStringGetLength(*t_fixed_path) != 0)
					t_initial_folder = *t_fixed_path;
			}
			else
			{
				if (t_last_slash < MCStringGetLength(*t_fixed_path) - 1)
					/* UNCHECKED */ MCStringCopySubstring(*t_fixed_path, MCRangeMake(t_last_slash + 1, MCStringGetLength(*t_fixed_path) - (t_last_slash + 1)), &t_initial_file);

				MCAutoStringRef t_folder_split;
				/* UNCHECKED */ MCStringCopySubstring(*t_fixed_path, MCRangeMake(0, t_last_slash - 1), &t_folder_split);
				if (MCS_exists(*t_folder_split, False))
					t_initial_folder = *t_folder_split;
			}
		}
		else
		{
			uindex_t t_last_slash;

			if (MCStringLastIndexOfChar(*t_fixed_path, '/', UINDEX_MAX, kMCStringOptionCompareCaseless, t_last_slash))
			{
				MCAutoStringRef t_folder_split;
				/* UNCHECKED */ MCStringCopySubstring(*t_fixed_path, MCRangeMake(0, t_last_slash - 1), &t_folder_split);
				
				if (MCS_exists(*t_folder_split, False))
					t_initial_folder = *t_folder_split;
			}
		}
	}

	if (!MCModeMakeLocalWindows())
	{
		MCAutoStringRefArray t_filters;

		if (p_filter != NULL)
		{
			/* UNCHECKED */ MCStringsSplit(p_filter, '\0', t_filters.PtrRef(), t_filters.CountRef());
		}

		MCRemoteFileDialog(p_title, p_prompt, *t_filters, t_filters.Count(), *t_initial_folder, *t_initial_file, (p_options & MCA_OPTION_SAVE_DIALOG) != 0, (p_options & MCA_OPTION_PLURAL) != 0, r_value);

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
			s_shell32_module = LoadLibraryA("shell32.dll");
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

		if (t_succeeded && *t_initial_folder != NULL)
		{
			IShellItem *t_initial_folder_shellitem;
			t_initial_folder_shellitem = NULL;
			t_hresult = s_shcreateitemfromparsingname(WideCString(MCStringGetCString(*t_initial_folder)), NULL, __uuidof(IShellItem), (LPVOID *)&t_initial_folder_shellitem);
			if (SUCCEEDED(t_hresult))
				t_file_dialog -> SetFolder(t_initial_folder_shellitem);
			if (t_initial_folder_shellitem != NULL)
				t_initial_folder_shellitem -> Release();
			t_succeeded = SUCCEEDED(t_hresult);
		}

		if (t_succeeded && *t_initial_file != NULL)
		{
			t_hresult = t_file_dialog -> SetFileName(WideCString(MCStringGetCString(*t_initial_file)));
			t_succeeded = SUCCEEDED(t_hresult);
		}

		if (t_succeeded && p_filter != NULL && (p_options & MCA_OPTION_FOLDER_DIALOG) == 0)
		{
			uint4 t_filter_length, t_filter_count;
			measure_filter(MCStringGetCString(p_filter), t_filter_length, t_filter_count);

			WideCString t_filters(MCStringGetCString(p_filter), t_filter_length);
			COMDLG_FILTERSPEC *t_filter_spec;

			filter_to_spec(t_filters, t_filter_count, t_filter_spec);

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
			t_hresult = t_file_dialog -> SetTitle(WideCString(MCStringGetCString(p_prompt)));

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
		OPENFILENAMEA t_open_dialog;
		memset(&t_open_dialog, 0, sizeof(OPENFILENAMEA));
		t_open_dialog . lStructSize = sizeof(OPENFILENAMEA);

		char *t_initial_file_buffer = new char[MAX_PATH];
		if (*t_initial_file != NULL)
			strcpy(t_initial_file_buffer, MCStringGetCString(*t_initial_file));
		else
			*t_initial_file_buffer = '\0';

		t_open_dialog . lpstrFilter = MCStringGetCString(p_filter);
		t_open_dialog . nFilterIndex = 1;
		t_open_dialog . lpstrFile = t_initial_file_buffer;
		t_open_dialog . nMaxFile = MAX_PATH;
		t_open_dialog . lpstrInitialDir = MCStringGetCString(*t_initial_folder);
		t_open_dialog . lpstrTitle = MCStringGetCString(p_prompt);
		t_open_dialog . Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR |
														OFN_LONGNAMES | OFN_PATHMUSTEXIST | OFN_EXPLORER |
														OFN_ENABLEHOOK | OFN_ENABLESIZING;

		if (p_options & MCA_OPTION_PLURAL)
			t_open_dialog . Flags |= OFN_ALLOWMULTISELECT;

		if (p_options & MCA_OPTION_SAVE_DIALOG)
			t_open_dialog . Flags |= OFN_OVERWRITEPROMPT;

		t_open_dialog . lpstrFilter = MCStringGetCString(p_filter);
		t_open_dialog . lpfnHook = open_dialog_hook;
		t_open_dialog . hwndOwner = t_window != NULL ? (HWND)t_window -> handle . window : NULL;

		if (p_options & MCA_OPTION_SAVE_DIALOG)
			t_succeeded = GetSaveFileNameA((LPOPENFILENAMEA)&t_open_dialog) == TRUE;
		else
		{
			*t_open_dialog . lpstrFile = '\0';
			t_succeeded = GetOpenFileNameA((LPOPENFILENAMEA)&t_open_dialog) == TRUE;
		}

		if (!t_succeeded)
			t_result = CommDlgExtendedError();

		// MW-2005-07-26: Try again without the specified filename if it was invalid
		if (t_result == FNERR_INVALIDFILENAME)
		{
			*t_open_dialog . lpstrFile = '\0';
			if (p_options & MCA_OPTION_SAVE_DIALOG)
				t_succeeded = GetSaveFileNameA((LPOPENFILENAMEA)&t_open_dialog) == TRUE;
			else
				t_succeeded = GetOpenFileNameA((LPOPENFILENAMEA)&t_open_dialog) == TRUE;

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

		delete t_initial_file_buffer;
	}

	if (t_succeeded)
	{
		if (p_options & MCA_OPTION_RETURN_FILTER)
		{
			const char *t_type = MCStringGetCString(p_filter);
			const char *t_types = MCStringGetCString(p_filter);
			for(int t_index = t_filter_index * 2 - 1; t_index > 1; t_types += 1)
				if (*t_types == '\0')
					t_type = t_types + 1, t_index -= 1;

			/* UNCHECKED */ MCStringCreateWithCString(t_type, r_result);
		}

		t_result = 0;
		r_value = MCValueRetain(*t_value);
	}
	else
		MCStringCopy(MCNameGetString(MCN_cancel), r_result);

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
			/* UNCHECKED */ MCStringAppendNativeChar(*t_filters, '\0');

		/* UNCHECKED */ MCStringAppend(*t_filters, t_split[0]);

		if (t_split.Count() < 2)
			/* UNCHECKED */ MCStringAppendNativeChars(*t_filters, (char_t*)"\0*.*", 4);
		else
		{
			MCAutoStringRefArray t_extensions;
			/* UNCHECKED */ MCStringsSplit(t_split[1], ',', t_extensions.PtrRef(), t_extensions.CountRef());
			if (t_extensions.Count() == 0)
				/* UNCHECKED */ MCStringAppendNativeChars(*t_filters, (char_t*)"\0*.*", 4);
			else
			{
				for (unsigned int i = 0; i < t_extensions.Count(); ++i)
				{
					if (i != 0)
						/* UNCHECKED*/ MCStringAppendNativeChar(*t_filters, ';');
					else
						/* UNCHECKED*/ MCStringAppendNativeChar(*t_filters, '\0');

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
		/* UNCHECKED */ MCStringCopy(*t_filters, r_filters);
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
static unsigned int get_dll_version(const char *p_dll)
{
	HMODULE t_module;
	t_module = LoadLibraryA(p_dll);
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
	static MCStringRef s_last_folder = NULL;

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
		s_shell_version = get_dll_version("shell32.dll");

	bool sheet = (p_options & MCA_OPTION_SHEET) != 0;
	char *prompt = (char *)p_prompt;

	BROWSEINFOA bi;
	memset(&bi, 0, sizeof(BROWSEINFO));

	Window pw;
	pw = MCModeGetParentWindow();

	if (pw != DNULL)
		bi.hwndOwner = (HWND)pw->handle.window;

	bi.pidlRoot = NULL;
	bi.lpszTitle = prompt;
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	if (s_shell_version >= 500)
		bi.ulFlags |= BIF_NEWDIALOGSTYLE;
	if (*t_native_filename != NULL)
	{
		bi . lpfn = BrowseCallbackProc;
		bi . lParam = (LPARAM)MCStringGetCString(*t_native_filename);
	}
	else
	{
		bi.lpfn = NULL;
		bi.lParam = NULL;
	}
	LPITEMIDLIST lpiil;
	LPMALLOC lpm;
	char *tdir = NULL;
	SHGetMalloc(&lpm);

	DWORD t_error;
	lpiil = SHBrowseForFolderA(&bi);
	if (lpiil == NULL)
	{
		t_error = GetLastError();
	}
	
	char *buf;
	if (lpiil != NULL && SHGetPathFromIDListA(lpiil, buf))
	{
		if (s_last_folder != NULL)
			MCValueRelease(s_last_folder);
		/* UNCHECKED */ MCStringCreateWithCString(buf, s_last_folder);

		MCAutoStringRef t_std_path;
		/* UNCHECKED */ MCS_pathfromnative(s_last_folder, &t_std_path);
		buf = strclone(MCStringGetCString(*t_std_path));

		r_result = MCValueRetain(*t_std_path);
	}
	else
		/* UNCHECKED */ MCStringCreateWithCString(MCcancelstring, r_result);

	//  SMR 1880 clear shift and button state
	waitonbutton();

	lpm->Free(lpiil);
	lpm->Release();

	return 0;
}

/* WRAPPER */
/*bool MCA_folder(bool p_plural, MCStringRef p_prompt, MCStringRef p_initial,
				MCStringRef p_title, bool p_sheet,
				MCStringRef &r_value, MCStringRef &r_result)
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
	CHOOSECOLORA chooseclr ;
	static COLORREF custclr[16]; //save custom colors
	memset(&chooseclr, 0, sizeof(CHOOSECOLORA));
	chooseclr.lStructSize = sizeof (CHOOSECOLORA);
	chooseclr.lpCustColors = (LPDWORD)custclr;

	Window t_parent_window;
	t_parent_window = MCModeGetParentWindow();
	chooseclr.hwndOwner = t_parent_window != NULL ? (HWND)t_parent_window -> handle . window : NULL;

	chooseclr.Flags = CC_RGBINIT;
	chooseclr.rgbResult = RGB(p_initial_color.red >> 8, p_initial_color.green >> 8,
	                          p_initial_color.blue >> 8);

	bool t_success = true;
	if (!ChooseColorA(&chooseclr))
	{
		DWORD err = CommDlgExtendedError();
		r_chosen = false;
	}
	else
	{
		r_chosen = true;
		p_initial_color.red = GetRValue(chooseclr.rgbResult);
		p_initial_color.red |= p_initial_color.red << 8;
		p_initial_color.green = GetGValue(chooseclr.rgbResult);
		p_initial_color.green |= p_initial_color.green << 8;
		p_initial_color.blue = GetBValue(chooseclr.rgbResult);
		p_initial_color.blue |= p_initial_color.blue << 8;
	}

	//  SMR 1880 clear shift and button state
	waitonbutton();

	return t_success;
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
