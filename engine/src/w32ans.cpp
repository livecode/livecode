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

extern void MCRemoteFileDialog(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char * const p_types[], uint32_t p_type_count, const char *p_initial_folder, const char *p_initial_file, bool p_save, bool p_files);
extern void MCRemoteFolderDialog(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_initial);
extern void MCRemoteColorDialog(MCExecPoint& ep, const char *p_title, uint32_t p_r, uint32_t p_g, uint32_t p_b);

static const char *getfilter(const MCString &s)
{
	uint4 flength = s.getlength();
	if (flength != 0)
	{
		static char *filterstring;
		if (filterstring != NULL)
			delete filterstring;
		filterstring = new char[flength * 2 + 3];
		memcpy(filterstring, s.getstring(), flength);
		filterstring[flength] = '\0';
		if (strchr(filterstring, '\n') == NULL
		        && strchr(filterstring, ',') == NULL)
		{
			memcpy(&filterstring[flength + 1], s.getstring(), flength);
			flength *= 2;
			filterstring[++flength] = '\0';
		}
		filterstring[++flength] = '\0';
		char *str = filterstring;
		while (*str)
		{
			if (*str == '\n' ||  *str == ',')
				*str = '\0'; // replace \n with a \0 to meet the MS requirement
			str++;
		}
		return filterstring;
	}
	else
		return "All Files (*.*)\0*.*\0\0";
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
static void build_path(MCExecPoint& ep, const char *p_folder, const MCString& p_file)
{
	char *t_std_file;
	t_std_file = p_file . clone();
	MCU_path2std(t_std_file);

	bool t_use_folder;
	if (p_folder == NULL || strlen(p_folder) == 0 ||
		(strlen(t_std_file) > 1 && t_std_file[1] == ':') ||
		(strlen(t_std_file) > 2 && t_std_file[0] == '/' && t_std_file[1] == '/'))
		t_use_folder = false;
	else
		t_use_folder = true;

	if (t_use_folder)
	{
		ep.appendcstring(p_folder);
		if (p_folder[strlen(p_folder) - 1] != '/')
			ep.appendchar('/');
	}

	ep . appendcstring(t_std_file);

	delete t_std_file;
}

static void build_paths(MCExecPoint& ep)
{
	ep . clear();

	MCU_path2std(*sg_chosen_folder);
	if (**sg_chosen_files == '"')
	{
		Meta::itemised_string t_items(sg_chosen_files, ' ', true);
		for(unsigned int t_index = 0; t_index < t_items . count(); ++t_index)
		{
			if (t_index != 0)
				ep.appendnewline();
			build_path(ep, *sg_chosen_folder, t_items[t_index]);
		}
	}
	else
		build_path(ep, *sg_chosen_folder, *sg_chosen_files);

	sg_chosen_files . clear();
	sg_chosen_folder . clear();
}

static HRESULT append_shellitem_path_and_release(MCExecPoint& ep, IShellItem *p_item, bool p_first)
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
		char *t_rev_filename;
		t_rev_filename = strdup(AnsiCString(t_filename));
		MCU_path2std(t_rev_filename);
		ep.concatcstring(t_rev_filename, EC_RETURN, p_first);
		delete[] t_rev_filename;
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

static int MCA_do_file_dialog(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options)
{
	int t_result = 0;

	char *t_initial_file;
	t_initial_file = NULL;

	char *t_initial_folder;
	t_initial_folder = NULL;

	ep . clear();

	if (*p_initial != '\0')
	{
		char *t_initial_clone;
		t_initial_clone = strdup(p_initial);
		MCU_w32path2std(t_initial_clone);
		MCU_fix_path(t_initial_clone);

		if (MCS_exists(t_initial_clone, False))
			t_initial_folder = t_initial_clone;
		else if ((p_options & MCA_OPTION_SAVE_DIALOG) != 0)
		{
			t_initial_file = strrchr(t_initial_clone, '/');
			if (t_initial_file == NULL)
			{
				if (strlen(t_initial_clone) != 0)
					t_initial_file = t_initial_clone;
			}
			else
			{
				*t_initial_file = '\0';
				t_initial_file++;
				
				if (t_initial_file[0] == '\0')
					t_initial_file = NULL;

				if (MCS_exists(t_initial_clone, False))
					t_initial_folder = t_initial_clone;
			}
		}
		else
		{
			char *t_leaf;
			t_leaf = strrchr(t_initial_clone, '/');
			if (t_leaf != NULL)
			{
				*t_leaf = '\0';
				if (MCS_exists(t_initial_clone, False))
					t_initial_folder = t_initial_clone;
			}
		}

		t_initial_file = strdup(t_initial_file);
		t_initial_folder = MCS_resolvepath(t_initial_folder);

		delete t_initial_clone;
	}

	if (!MCModeMakeLocalWindows())
	{
		char ** t_filters = NULL;
		uint32_t t_filter_count = 0;

		if (p_filter != NULL)
		{
			const char *t_strptr = p_filter;
			while (t_strptr[0] != '\0')
			{
				t_filter_count++;
				t_filters = (char**)realloc(t_filters, t_filter_count * sizeof(char*));
				t_filters[t_filter_count - 1] = (char *)t_strptr;
				t_strptr += strlen(t_strptr) + 1;
			}
		}

		MCRemoteFileDialog(ep, p_title, p_prompt, t_filters, t_filter_count, t_initial_folder, t_initial_file, (p_options & MCA_OPTION_SAVE_DIALOG) != 0, (p_options & MCA_OPTION_PLURAL) != 0);

		free(t_filters);
		return 0;
	}

	Window t_window;
	t_window = MCModeGetParentWindow();

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

		if (t_succeeded && t_initial_folder != NULL)
		{
			IShellItem *t_initial_folder_shellitem;
			t_initial_folder_shellitem = NULL;
			t_hresult = s_shcreateitemfromparsingname(WideCString(t_initial_folder), NULL, __uuidof(IShellItem), (LPVOID *)&t_initial_folder_shellitem);
			if (SUCCEEDED(t_hresult))
				t_file_dialog -> SetFolder(t_initial_folder_shellitem);
			if (t_initial_folder_shellitem != NULL)
				t_initial_folder_shellitem -> Release();
			t_succeeded = SUCCEEDED(t_hresult);
		}

		if (t_succeeded && t_initial_file != NULL)
		{
			t_hresult = t_file_dialog -> SetFileName(WideCString(t_initial_file));
			t_succeeded = SUCCEEDED(t_hresult);
		}

		if (t_succeeded && p_filter != NULL && (p_options & MCA_OPTION_FOLDER_DIALOG) == 0)
		{
			uint4 t_filter_length, t_filter_count;
			measure_filter(p_filter, t_filter_length, t_filter_count);

			WideCString t_filters(p_filter, t_filter_length);
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
			t_hresult = t_file_dialog -> SetTitle(WideCString(p_prompt));

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
				ep . clear();
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
						t_hresult = append_shellitem_path_and_release(ep, t_file_item, t_index == 0);
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
				ep . clear();
				t_hresult = append_shellitem_path_and_release(ep, t_file_item, true);
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
		if (t_initial_file != NULL)
			strcpy(t_initial_file_buffer, t_initial_file);
		else
			*t_initial_file_buffer = '\0';

		t_open_dialog . lpstrFilter = p_filter;
		t_open_dialog . nFilterIndex = 1;
		t_open_dialog . lpstrFile = t_initial_file_buffer;
		t_open_dialog . nMaxFile = MAX_PATH;
		t_open_dialog . lpstrInitialDir = t_initial_folder;
		t_open_dialog . lpstrTitle = p_prompt;
		t_open_dialog . Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR |
														OFN_LONGNAMES | OFN_PATHMUSTEXIST | OFN_EXPLORER |
														OFN_ENABLEHOOK | OFN_ENABLESIZING;

		if (p_options & MCA_OPTION_PLURAL)
			t_open_dialog . Flags |= OFN_ALLOWMULTISELECT;

		if (p_options & MCA_OPTION_SAVE_DIALOG)
			t_open_dialog . Flags |= OFN_OVERWRITEPROMPT;

		t_open_dialog . lpstrFilter = p_filter;
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
			build_paths(ep);
			t_filter_index = t_open_dialog . nFilterIndex;
		}

		delete t_initial_file_buffer;
	}

	if (t_succeeded)
	{
		if (p_options & MCA_OPTION_RETURN_FILTER)
		{
			const char *t_type = p_filter;
			const char *t_types = p_filter;
			for(int t_index = t_filter_index * 2 - 1; t_index > 1; t_types += 1)
				if (*t_types == '\0')
					t_type = t_types + 1, t_index -= 1;
			MCresult -> copysvalue(t_type);
		}

		t_result = 0;
	}

	waitonbutton();

	if (t_initial_folder != NULL)
		delete t_initial_folder;

	if (t_initial_file != NULL)
		delete t_initial_file;

	return t_result;
}

static void get_new_filter(MCExecPoint& ep, char * const p_types[], uint4 p_type_count)
{
	ep . clear();
	for(uint4 t_type_index = 0; t_type_index < p_type_count; ++t_type_index)
	{
		Meta::itemised_string t_type(p_types[t_type_index], '|');
		if (t_type . count() < 1)
			continue;
		ep . concatmcstring(t_type[0], EC_NULL, t_type_index == 0);
		if (t_type . count() < 2)
			ep . concatcstring("*.*", EC_NULL, false);
		else
		{
			Meta::itemised_string t_extensions(t_type[1], ',');
			if (t_extensions . count() == 0)
				ep . concatcstring("*.*", EC_NULL, false);
			else
				for(unsigned int t_extension = 0; t_extension < t_extensions . count(); ++t_extension)
				{
					ep . appendchar(t_extension != 0 ? ';' : '\0');
					ep . appendcstring("*.");
					ep . appendmcstring(t_extensions[t_extension]);
				}
		}
	}
	if (ep . getsvalue() == MCnullmcstring)
		ep . appendchars("All Files\0*.*\0", 14);
	ep . appendchar('\0');
}

// MW-2005-05-15: New answer file with types call
int MCA_file_with_types(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options)
{
	get_new_filter(ep, p_types, p_type_count);

	Meta::cstring_value t_filters;
	t_filters = ep;

	return MCA_do_file_dialog(ep, p_title == NULL ? "" : p_title, p_prompt == NULL ? "" : p_prompt, *t_filters, p_initial == NULL ? "" : p_initial, p_options | MCA_OPTION_RETURN_FILTER);
}

// MW-2005-05-15: Updated for new answer command restructuring
int MCA_file(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options)
{
	return MCA_do_file_dialog(ep, p_title == NULL ? "" : p_title, p_prompt == NULL ? "" : p_prompt, getfilter(p_filter == NULL ? "" : p_filter), p_initial == NULL ? "" : p_initial, p_options);
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
int MCA_folder(MCExecPoint &ep, const char *p_title, const char *p_prompt, const char *p_initial, unsigned int p_options)
{
	if (MCmajorosversion >= 0x0600 && MCModeMakeLocalWindows())
		return MCA_file(ep, p_title, p_prompt, nil, p_initial, p_options | MCA_OPTION_FOLDER_DIALOG);

// MW-2005-05-27: We'll use a static (I know bad me) to store the version
//   of the shell dll.
	static int s_shell_version = -1;
	static char *s_last_folder = NULL;

	char *t_native_filename;
	unsigned int t_native_filename_length;

	if (p_initial != NULL)
	{
		t_native_filename_length = strlen(p_initial);
		t_native_filename = (char *)_alloca(t_native_filename_length + 2);
		strcpy(t_native_filename, p_initial);
		MCU_path2native(t_native_filename);
	}
	else
	{
		t_native_filename = s_last_folder;
		t_native_filename_length = 0;
	}

	if (!MCModeMakeLocalWindows())
	{
		MCRemoteFolderDialog(ep, p_title, p_prompt, t_native_filename);
		if (!ep.isempty())
		{
			if (s_last_folder != NULL)
				delete s_last_folder;
			s_last_folder = ep.getsvalue().clone();
			MCU_path2native(s_last_folder);
		}
		return 0;
	}

	if (s_shell_version == -1)
		s_shell_version = get_dll_version("shell32.dll");

	bool sheet = (p_options & MCA_OPTION_SHEET) != 0;
	char *prompt = (char *)p_prompt;
	
	ep . clear();

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
	if (t_native_filename != NULL)
	{
		bi . lpfn = BrowseCallbackProc;
		bi . lParam = (LPARAM)t_native_filename;
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
	
	if (lpiil != NULL && SHGetPathFromIDListA(lpiil, ep.getbuffer(PATH_MAX)))
	{
		if (s_last_folder != NULL)
			delete s_last_folder;
		s_last_folder = strclone(ep . getbuffer(0));
		MCU_path2std(ep.getbuffer(0));
		ep.setstrlen();
	}
	else
	{
		ep.clear();
		MCresult->sets(MCcancelstring);
	}
	//  SMR 1880 clear shift and button state
	waitonbutton();

	lpm->Free(lpiil);
	lpm->Release();

	return 0;
}

// MERG-2013-08-18: Updated to allow script access to colorDialogColors
static COLORREF s_colordialogcolors[16];

// MW-2005-05-15: Updated for new answer command restructuring
int MCA_color(MCExecPoint &ep, const char *p_title, const char *p_initial, Boolean sheet)
{
	ep . setsvalue(p_initial);

	MCColor oldcolor;
	if (ep.getsvalue().getlength() == 0)
	{
		oldcolor.red = MCpencolor.red;
		oldcolor.green = MCpencolor.green;
		oldcolor.blue = MCpencolor.blue;
	}
	else
	{
		char *cname = NULL;
		MCscreen->parsecolor(ep.getsvalue(), &oldcolor, &cname);
		delete cname;
	}

	if (!MCModeMakeLocalWindows())
	{
		MCRemoteColorDialog(ep, p_title, oldcolor.red >> 8, oldcolor.green >> 8, oldcolor.blue >> 8);
		return 0;
	}

	CHOOSECOLORA chooseclr ;

	memset(&chooseclr, 0, sizeof(CHOOSECOLORA));
	chooseclr.lStructSize = sizeof (CHOOSECOLORA);
	chooseclr.lpCustColors = (LPDWORD)s_colordialogcolors;

	Window t_parent_window;
	t_parent_window = MCModeGetParentWindow();
	chooseclr.hwndOwner = t_parent_window != NULL ? (HWND)t_parent_window -> handle . window : NULL;

	chooseclr.Flags = CC_RGBINIT;
	chooseclr.rgbResult = RGB(oldcolor.red >> 8, oldcolor.green >> 8,
	                          oldcolor.blue >> 8);
	if (!ChooseColorA(&chooseclr))
	{
		DWORD err = CommDlgExtendedError();
		if (err == 0)
			MCresult->sets(MCcancelstring);
		else
		{
			char buffer[22 + U4L];
			sprintf(buffer, "Error %d opening dialog", err);
			MCresult->copysvalue(buffer);
		}
		ep.clear();
	}
	else
	{
		ep.setcolor(GetRValue(chooseclr.rgbResult), GetGValue(chooseclr.rgbResult), GetBValue(chooseclr.rgbResult));
		
	} 

	//  SMR 1880 clear shift and button state
	waitonbutton();

	return 0;
}

void MCA_setcolordialogcolors(MCExecPoint& p_ep)
{
	const char * t_color_list;
	t_color_list = p_ep.getcstring();
		
	if (t_color_list != NULL)
	{
		MCColor t_colors[16];
		char *t_colornames[16];
		int i;
        
		for (i = 0 ; i < 16 ; i++)
			t_colornames[i] = NULL;
        
		MCscreen->parsecolors(t_color_list, t_colors, t_colornames, 16);
        
		for(i=0;i < 16;i++)
		{
			if (t_colors[i] . flags != 0)
				s_colordialogcolors[i] = RGB(t_colors[i].red >> 8, t_colors[i].green >> 8,
	                          t_colors[i].blue >> 8);
			else
				s_colordialogcolors[i] = NULL;
            
            delete t_colornames[i];
		}

	}
}

void MCA_getcolordialogcolors(MCExecPoint& p_ep)
{
	p_ep.clear();
	MCExecPoint t_ep(p_ep);

	for(int i=0;i < 16;i++)
	{
		if (s_colordialogcolors[i] != 0)
			t_ep.setcolor(GetRValue(s_colordialogcolors[i]), GetGValue(s_colordialogcolors[i]), GetBValue(s_colordialogcolors[i]));
		else
			t_ep.clear();

		p_ep.concatmcstring(t_ep.getsvalue(), EC_RETURN, i==0);
	}
}

int MCA_ask_file_with_types(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options)
{
	get_new_filter(ep, p_types, p_type_count);

	Meta::cstring_value t_filters;
	t_filters = ep;

	return MCA_do_file_dialog(ep, p_title == NULL ? "" : p_title, p_prompt == NULL ? "" : p_prompt, *t_filters, p_initial == NULL ? "" : p_initial, p_options | MCA_OPTION_RETURN_FILTER | MCA_OPTION_SAVE_DIALOG);
}

// Mw-2005-06-02: Updated to use new answer file prototype
int MCA_ask_file(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options)
{
	return MCA_do_file_dialog(ep, p_title == NULL ? "" : p_title, p_prompt == NULL ? "" : p_prompt, getfilter(p_filter == NULL ? "" : p_filter), p_initial == NULL ? "" : p_initial, p_options | MCA_OPTION_SAVE_DIALOG);
}
