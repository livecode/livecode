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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"
#include "mode.h"

#include "globals.h"
#include "osspec.h"

#include "securemode.h"
#include "exec.h"
#include "util.h"
#include "uidc.h"
#include "mcerror.h"

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCFilesEofEnumElementInfo[] =
{
	{ "\004", 0, false },
	{ "eof", 1, false },
};

static MCExecEnumTypeInfo _kMCFilesEofEnumTypeInfo =
{
	"Files.EofEnum",
	sizeof(_kMCFilesEofEnumElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCFilesEofEnumElementInfo
};

//////////

MCExecEnumTypeInfo *kMCFilesEofEnumTypeInfo = &_kMCFilesEofEnumTypeInfo;

//////////

void
MCFilesEvalFileItemsOfDirectory(MCExecContext& ctxt,
                                MCStringRef p_directory,
                                bool p_files,
                                bool p_detailed,
                                bool p_utf8,
                                MCStringRef& r_string)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt . LegacyThrow(EE_DISK_NOPERM);
		return;
	}
	MCAutoListRef t_list;
	if (MCS_getentries(p_directory, p_files, p_detailed, p_utf8, &t_list))
	{
		MCListCopyAsString(*t_list, r_string);
	}
	else
	{
		MCStringCopy(kMCEmptyString, r_string);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesEvalDiskSpace(MCExecContext& ctxt, real64_t& r_result)
{
	r_result = MCS_getfreediskspace();
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesEvalDriverNames(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt . LegacyThrow(EE_DISK_NOPERM);
		return;
	}

	if (MCS_getdevices(r_string) == True)
		return;

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesEvalDrives(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt . LegacyThrow(EE_DISK_NOPERM);
		return;
	}
    
	if (MCS_getdrives(r_string) == True)
		return;

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

bool MCFilesOpenFilesList(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	for (uinteger_t i = 0; i < MCnfiles; i++)
		if (!MCListAppend(*t_list, MCfiles[i].name))
			return false;
	return MCListCopy(*t_list, r_list);
}

void MCFilesEvalOpenFiles(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCFilesOpenFilesList(&t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesEvalTempName(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCS_tmpnam(r_string);
}

void MCFilesEvalSpecialFolderPath(MCExecContext& ctxt, MCStringRef p_folder, MCStringRef& r_path)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}

    bool t_error;
    MCNewAutoNameRef t_path;
    t_error = false;
    MCNameCreate(p_folder, &t_path);
    // We have a special, mode-specific resource folder
    if (MCNameIsEqualToCaseless(*t_path, MCN_resources))
        MCModeGetResourcesFolder(r_path);
    else if (!MCS_getspecialfolder(*t_path, r_path))
        t_error = true;

    // MCModeGetResourcesFolder won't fail, but can return an empty path
    if (!t_error)
	{
		if (MCStringIsEmpty(r_path))
			ctxt.SetTheResultToCString("folder not found");
		else
			ctxt.SetTheResultToEmpty();

		return;
	}

	ctxt.Throw();
}

void MCFilesEvalLongFilePath(MCExecContext& ctxt, MCStringRef p_path, MCStringRef& r_long_path)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}

	// No failure for this function, only a specific "can't get" result - and
	// an empty string as a return value
	if (!MCS_longfilepath(p_path, r_long_path))
	{
		r_long_path = MCValueRetain(kMCEmptyString);
		ctxt.SetTheResultToCString("can't get");
	}
	else
		ctxt.SetTheResultToEmpty();
}

void MCFilesEvalShortFilePath(MCExecContext& ctxt, MCStringRef p_path, MCStringRef& r_short_path)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}

    // Failure of this function isn't really an error
    r_short_path = nil;
	if (!MCS_shortfilepath(p_path, r_short_path))
	{
		r_short_path = MCValueRetain(kMCEmptyString);
		ctxt.SetTheResultToCString("can't get");
	}
	else
		ctxt.SetTheResultToEmpty();
}


////////////////////////////////////////////////////////////////////////////////

bool MCFilesOpenProcessesNamesList(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

    IO_cleanprocesses();
	for (uinteger_t i = 0; i < MCnprocesses; i++)
		if (!MCListAppend(*t_list, MCprocesses[i].name))
			return false;

	return MCListCopy(*t_list, r_list);
}

void MCFilesEvalOpenProcesses(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCFilesOpenProcessesNamesList(&t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

bool MCFilesOpenProcessesIdsList(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

    IO_cleanprocesses();
	for (uinteger_t i = 0; i < MCnprocesses; i++)
		if (!MCListAppendInteger(*t_list, MCprocesses[i].pid))
			return false;

	return MCListCopy(*t_list, r_list);
}

void MCFilesEvalOpenProcessesIds(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCFilesOpenProcessesIdsList(&t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

void MCFilesEvalProcessId(MCExecContext& ctxt, integer_t& r_pid)
{
    r_pid = MCS_getpid();
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesEvalDeleteRegistry(MCExecContext& ctxt, MCStringRef p_key, bool& r_deleted)
{
	if (MCsecuremode & MC_SECUREMODE_REGISTRY_WRITE)
	{
		ctxt.LegacyThrow(EE_REGISTRY_NOPERM);
		return;
	}

	MCAutoStringRef t_error;
	if (MCS_delete_registry(p_key, &t_error))
	{
		r_deleted = *t_error == nil;
		if (!r_deleted)
			ctxt.SetTheResultToValue(*t_error);
		else
			ctxt.SetTheResultToEmpty();

		return;
	}

	ctxt.Throw();
}

void MCFilesEvalListRegistry(MCExecContext& ctxt, MCStringRef p_key, MCStringRef& r_list)
{
	if (MCsecuremode & MC_SECUREMODE_REGISTRY_READ)
	{
		ctxt.LegacyThrow(EE_REGISTRY_NOPERM);
		return;
	}

	MCAutoListRef t_list;
	MCAutoStringRef t_error;
	if (MCS_list_registry(p_key, &t_list, &t_error))
	{
		if (*t_error != nil)
		{
			ctxt.SetTheResultToValue(*t_error);
			r_list = MCValueRetain(kMCEmptyString);
			return;
		}
		else
		{
			ctxt.SetTheResultToEmpty();
			if (MCListCopyAsString(*t_list, r_list))
				return;
		}
	}

	ctxt.Throw();
}

void MCFilesEvalQueryRegistry(MCExecContext& ctxt, MCStringRef p_key, MCValueRef& r_string)
{
	if (MCsecuremode & MC_SECUREMODE_REGISTRY_READ)
	{
		ctxt.LegacyThrow(EE_REGISTRY_NOPERM);
		return;
	}

	MCAutoStringRef t_type, t_error;
	if (MCS_query_registry(p_key, r_string, &t_type, &t_error))
	{
		if (*t_error != nil)
		{
			ctxt.SetTheResultToValue(*t_error);
			r_string = MCValueRetain(kMCEmptyString);
		}
		else
			ctxt.SetTheResultToEmpty();

		return;
	}

	ctxt.Throw();
}

void MCFilesEvalQueryRegistryWithType(MCExecContext& ctxt, MCStringRef p_key, MCStringRef& r_type, MCValueRef& r_string)
{
	if (MCsecuremode & MC_SECUREMODE_REGISTRY_READ)
	{
		ctxt.LegacyThrow(EE_REGISTRY_NOPERM);
		return;
	}

	MCAutoStringRef t_error;
	if (MCS_query_registry(p_key, r_string, r_type, &t_error))
	{
		if (*t_error != nil)
		{
			ctxt.SetTheResultToValue(*t_error);
			r_string = MCValueRetain(kMCEmptyString);
            // SN-2014-11-18: [[ Bug 14052 ]] Assign the empty string to the type as well.
            r_type = MCValueRetain(kMCEmptyString);
		}
		else
			ctxt.SetTheResultToEmpty();
        return;
	}

	ctxt.Throw();
}

void MCFilesEvalSetRegistry(MCExecContext& ctxt, MCStringRef p_key, MCValueRef p_value, bool& r_set)
{
	MCFilesEvalSetRegistryWithType(ctxt, p_key, p_value, kMCEmptyString, r_set);
}

void MCFilesEvalSetRegistryWithType(MCExecContext& ctxt, MCStringRef p_key, MCValueRef p_value, MCStringRef p_type, bool& r_set)
{
	if (MCsecuremode & MC_SECUREMODE_REGISTRY_WRITE)
	{
		ctxt.LegacyThrow(EE_REGISTRY_NOPERM);
		return;
	}

	// Convert the type string to a registry type
	MCSRegistryValueType t_type;
	t_type = MCS_registry_type_from_string(p_type);

	// Ensure that the supplied value is of the correct type
	MCAutoValueRef t_converted;
	if (t_type == kMCSRegistryValueTypeSz || t_type == kMCSRegistryValueTypeMultiSz
		|| t_type == kMCSRegistryValueTypeExpandSz || t_type == kMCSRegistryValueTypeLink)
	{
		/* UNCHECKED */ ctxt.ConvertToString(p_value, (MCStringRef&)&t_converted);
	}
    else if (t_type == kMCSRegistryValueTypeNone)
    {
        // Nothing needs to be converted
    }
	else    // Includes REG_BINARY, REG_DWORD (and variants), REG_QWORD
	{
		/* UNCHECKED */ ctxt.ConvertToData(p_value, (MCDataRef&)&t_converted);
	}

	MCAutoStringRef t_error;
	if (MCS_set_registry(p_key, *t_converted, t_type, &t_error))
	{
		r_set = *t_error == nil;
		if (!r_set)
			ctxt.SetTheResultToValue(*t_error);
		else
			ctxt.SetTheResultToEmpty();

		return;
	}

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesEvalCopyResourceWithNewId(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_dest, MCStringRef p_type, MCStringRef p_name, MCStringRef p_newid, MCStringRef& r_value)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}
	
	MCAutoStringRef t_error;
	if (MCS_copyresource(p_source, p_dest, p_type, p_name, p_newid, &t_error))
	{
		if (*t_error != nil)
			ctxt.SetTheResultToValue(*t_error);
		else
			ctxt.SetTheResultToEmpty();
		r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
		return;
	}
	
	ctxt.Throw();
}

void MCFilesEvalCopyResource(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_dest, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_value)
{
	MCFilesEvalCopyResourceWithNewId(ctxt, p_source, p_dest, p_type, p_name, nil, r_value);
}

void MCFilesEvalDeleteResource(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_value)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}

	MCAutoStringRef t_error;
	if (MCS_deleteresource(p_source, p_type, p_name, &t_error))
	{
		if (*t_error != nil)
			ctxt.SetTheResultToValue(*t_error);
		else
			ctxt.SetTheResultToEmpty();
		r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
		return;
	}
	
	ctxt.Throw();
}

void MCFilesEvalGetResource(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_value)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}

	MCAutoStringRef t_error;
	if (MCS_getresource(p_source, p_type, p_name, r_value, &t_error))
	{
		if (*t_error != nil)
		{
			r_value = MCValueRetain(kMCEmptyString);
			ctxt.SetTheResultToValue(*t_error);
		}
		else
			ctxt.SetTheResultToEmpty();
		return;
	}
	
	ctxt.Throw();
}

void MCFilesEvalGetResourcesWithType(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_type, MCStringRef& r_resources)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}

	MCAutoListRef t_resources;
	MCAutoStringRef t_error;
	if (MCS_getresources(p_source, p_type, &t_resources, &t_error))
	{
		if (*t_error != nil)
		{
			r_resources = MCValueRetain(kMCEmptyString);
			ctxt.SetTheResultToValue(*t_error);
			return;
		}
		else
		{
			ctxt.SetTheResultToEmpty();
			if  (MCListCopyAsString(*t_resources, r_resources))
				return;
		}
	}
	
	ctxt.Throw();
}

void MCFilesEvalGetResources(MCExecContext& ctxt, MCStringRef p_source, MCStringRef& r_resources)
{
	MCFilesEvalGetResourcesWithType(ctxt, p_source, nil, r_resources);
}

void MCFilesEvalSetResource(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_type, MCStringRef p_id, MCStringRef p_name, MCStringRef p_flags, MCStringRef p_value, MCStringRef& r_value)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}
	
	MCAutoStringRef t_error;
	if (MCS_setresource(p_source, p_type, p_id, p_name, p_flags, p_value, &t_error))
	{
		if (*t_error != nil)
			ctxt.SetTheResultToValue(*t_error);
		else
			ctxt.SetTheResultToEmpty();
		r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
		return;
	}

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesEvalAliasReference(MCExecContext& ctxt, MCStringRef p_path, MCStringRef& r_reference)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}
	
	if (MCS_resolvealias(p_path, r_reference))
	{
        ctxt.SetTheResultToEmpty();
		return;
	}
    
    r_reference = MCValueRetain(kMCEmptyString);
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesEvalThereIsAFile(MCExecContext& ctxt, MCStringRef p_path, bool& r_result)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}

	r_result = MCS_exists(p_path, true);
}

void MCFilesEvalThereIsNotAFile(MCExecContext& ctxt, MCStringRef p_path, bool& r_result)
{
	MCFilesEvalThereIsAFile(ctxt, p_path, r_result);
	r_result = !r_result;
}

void MCFilesEvalThereIsAFolder(MCExecContext& ctxt, MCStringRef p_path, bool& r_result)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}

	r_result = MCS_exists(p_path, false);
}

void MCFilesEvalThereIsNotAFolder(MCExecContext& ctxt, MCStringRef p_path, bool& r_result)
{
	MCFilesEvalThereIsAFolder(ctxt, p_path, r_result);
	r_result = !r_result;
}

void MCFilesEvalThereIsAProcess(MCExecContext& ctxt, MCStringRef p_process, bool& r_result)
{
	if (MCsecuremode & MC_SECUREMODE_PROCESS)
	{
		ctxt.LegacyThrow(EE_PROCESS_NOPERM);
		return;
	}

	MCNewAutoNameRef t_name;
	uindex_t t_index;
	if (MCNameCreate(p_process, &t_name))
	{
		r_result = IO_findprocess(*t_name, t_index);
		return;
	}

	ctxt.Throw();
}

void MCFilesEvalThereIsNotAProcess(MCExecContext& ctxt, MCStringRef p_path, bool& r_result)
{
	MCFilesEvalThereIsAProcess(ctxt, p_path, r_result);
	r_result = !r_result;
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesEvalShell(MCExecContext& ctxt, MCStringRef p_command, MCStringRef& r_output)
{
	if (MCsecuremode & MC_SECUREMODE_PROCESS)
	{
		MCeerror->add(EE_SHELL_NOPERM, 0, 0, p_command);
		ctxt . Throw();
		return;
	}

	if (MCS_runcmd(p_command, r_output) != IO_NORMAL)
	{
		MCeerror->add(EE_SHELL_BADCOMMAND, 0, 0, p_command);
		ctxt . Throw();
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesExecRename(MCExecContext& ctxt, MCStringRef p_from, MCStringRef p_to)
{
	if (!MCS_rename(p_from, p_to))
		ctxt . SetTheResultToStaticCString("can't rename file");
	else
		ctxt . SetTheResultToEmpty();
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesExecLaunchUrl(MCExecContext& ctxt, MCStringRef p_url)
{
    MCAutoStringRef t_new_url;
    // Ensure we aren't asked for a file
    if (!MCStringBeginsWithCString(p_url, (const char_t*)"file:", kMCStringOptionCompareCaseless))
    {
        // MW-2008-04-02: [[ Bug 6306 ]] Make sure we escape invalid URL characters to save
        //   the user having to do so.
        MCAutoStringRef t_mutable_string;
        /* UNCHECKED */ MCStringCreateMutable(0, &t_mutable_string);
        uindex_t t_index = 0;
        while (t_index != MCStringGetLength((p_url)))
        {
            // MW-2008-08-14: [[ Bug 6898 ]] Interpreting this as a signed char causes sprintf
            //   to produce bad results.

            unsigned char t_char;
            t_char = MCStringGetNativeCharAtIndex(p_url, t_index);

            // MW-2008-06-12: [[ Bug 6446 ]] We must not escape '#' because this breaks URL
            //   anchors.
            if (t_char < 128 && (isalnum(t_char) || strchr("$-_.+!*'%(),;/?:@&=#", t_char) != NULL))
                /* UNCHECKED */ MCStringAppendNativeChar(*t_mutable_string, t_char);
            else
                MCStringAppendFormat(*t_mutable_string, "%%%02X", t_char);
            //t_new_ep . appendstringf("%%%02X", t_char);

            t_index += 1;
        }

        t_new_url = *t_mutable_string;
    }
    else
        t_new_url = p_url;

    if (ctxt . EnsureProcessIsAllowed())
	{
		// MCS_launch_url will set the result on failure, so we clear here to
		// make sure it is empty if it succeeds.
		ctxt.SetTheResultToEmpty();
		
        MCS_launch_url(*t_new_url);
	}
    else
        ctxt . LegacyThrow(EE_PROCESS_NOPERM);
}

void MCFilesExecLaunchDocument(MCExecContext& ctxt, MCStringRef p_document)
{
	if (ctxt . EnsureProcessIsAllowed())
	{
		MCS_launch_document(p_document);		
		return;
	}

	ctxt . LegacyThrow(EE_PROCESS_NOPERM);
}

void MCFilesExecLaunchApp(MCExecContext& ctxt, MCNameRef p_app, MCStringRef p_document)
{
	if (!ctxt . EnsureProcessIsAllowed())
	{
		ctxt . LegacyThrow(EE_PROCESS_NOPERM);
		return;
	}

	uindex_t index;

	if (!IO_findprocess(p_app, index))
		MCS_startprocess(p_app, 
							p_document == NULL ? kMCEmptyString : p_document, 
							OM_NEITHER, False);
	else
		ctxt . SetTheResultToStaticCString("process is already open");
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesExecDeleteFile(MCExecContext& ctxt, MCStringRef p_target)
{
	if (!ctxt . EnsureDiskAccessIsAllowed())
		return;
	Boolean done = False;
	if (MCS_exists(p_target, true))
	{
		// REVIEW AND CHANGE : delete file should probably not close the file
		//   first - this should be down to user script.
		{
			MCNewAutoNameRef t_target_name;
			if (!MCNameCreate(p_target, &t_target_name))
			{
				ctxt . Throw();
				return;
			}
			IO_closefile(*t_target_name);
		}

		done = MCS_unlink(p_target);
	}
	else if (MCS_exists(p_target, false))
		done = MCS_rmdir(p_target);

	if (!done)
		ctxt . SetTheResultToStaticCString("can't delete that file");
	else
		ctxt . SetTheResultToEmpty();
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesExecPerformOpen(MCExecContext& ctxt, MCNameRef p_name, int p_mode, intenum_t p_encoding, bool p_is_driver)
{
	if (!ctxt . EnsureDiskAccessIsAllowed())
		return;

	// REVIEW AND CHANGE : open file should probably not close the file
	//   first - this should be down to user script.
	IO_closefile(p_name);

	IO_handle istream = NULL;
	IO_handle ostream = NULL;

	// Attempt to open the file to find out its current encoding
    // FG-2014-05-21: [[ Bugfix 12246 ]] Don't open devices for BOM read
	Encoding_type t_encoding = (Encoding_type)p_encoding;
    if (p_encoding == kMCFileEncodingText && !p_is_driver)
    {
        IO_handle t_BOM_stream = MCS_open(MCNameGetString(p_name), kMCOpenFileModeRead, True, p_is_driver, 0);
		if (t_BOM_stream != NULL)
		{
            uint32_t t_bom_size;
            t_encoding = (Encoding_type)MCS_resolve_BOM(t_BOM_stream, t_bom_size);
			MCS_close(t_BOM_stream);
		}
		else
		{
			t_encoding = (Encoding_type)kMCFileEncodingNative;
		}
    }
    // FG-2014-09-23: [[ Bugfix 12545 ]] "text" is not valid when performing I/O
    else if (p_encoding == kMCFileEncodingText && p_is_driver)
        t_encoding = (Encoding_type)kMCFileEncodingNative;
    else
        t_encoding = (Encoding_type)p_encoding;

	switch (p_mode)
	{
	case OM_APPEND:
        ostream = MCS_open(MCNameGetString(p_name), kMCOpenFileModeAppend, False, p_is_driver, 0);
		break;
	case OM_NEITHER:
		break;
	case OM_READ:
        istream = MCS_open(MCNameGetString(p_name), kMCOpenFileModeRead, True, p_is_driver, 0);
		break;
	case OM_WRITE:
        ostream = MCS_open(MCNameGetString(p_name), kMCOpenFileModeWrite, False, p_is_driver, 0);
		break;
	case OM_UPDATE:
        istream = ostream = MCS_open(MCNameGetString(p_name), kMCOpenFileModeUpdate, False, p_is_driver, 0);
		break;
	default:
		break;
	}
	if (istream == NULL && ostream == NULL)
	{
		ctxt . SetTheResultToStaticCString("can't open that file");
		return;
    }

	MCU_realloc((char **)&MCfiles, MCnfiles, MCnfiles + 1, sizeof(Streamnode));
	MCfiles[MCnfiles].name = (MCNameRef)MCValueRetain(p_name);
    MCfiles[MCnfiles].mode = (Open_mode)p_mode;
	MCfiles[MCnfiles].encoding = t_encoding;

	MCfiles[MCnfiles].ihandle = istream;
    MCfiles[MCnfiles++].ohandle = ostream;
}

void MCFilesExecOpenFile(MCExecContext& ctxt, MCNameRef p_filename, int p_mode, intenum_t p_encoding)
{
    MCFilesExecPerformOpen(ctxt, p_filename, p_mode, p_encoding, false);
}

void MCFilesExecOpenDriver(MCExecContext& ctxt, MCNameRef p_device, int p_mode, intenum_t p_encoding)
{
    MCFilesExecPerformOpen(ctxt, p_device, p_mode, p_encoding, true);
}

void MCFilesExecPerformOpenProcess(MCExecContext& ctxt, MCNameRef p_process, int p_mode, intenum_t p_encoding, bool p_elevated)
{
	if (!ctxt . EnsureProcessIsAllowed())
		return;

	uindex_t index;
	if (IO_findprocess(p_process, index))
	{
		ctxt . SetTheResultToStaticCString("process is already open");
		return;
	}
    MCS_startprocess(p_process, NULL, (Open_mode)p_mode, p_elevated);
	if (IO_findprocess(p_process, index))
    {
        // ENCODING what default encoding for the process?
        if (p_encoding == kMCFileEncodingText)
            MCprocesses[index].encoding = (Encoding_type)kMCFileEncodingNative;
        else
            MCprocesses[index].encoding = (Encoding_type)p_encoding;
    }
}

void MCFilesExecOpenProcess(MCExecContext& ctxt, MCNameRef p_process, int p_mode, intenum_t p_encoding)
{
    MCFilesExecPerformOpenProcess(ctxt, p_process, p_mode, p_encoding, false);
}

void MCFilesExecOpenElevatedProcess(MCExecContext& ctxt, MCNameRef p_process, int p_mode, intenum_t p_encoding)
{
    MCFilesExecPerformOpenProcess(ctxt, p_process, p_mode, p_encoding, true);
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesExecPerformClose(MCExecContext& ctxt, MCNameRef p_name)
{
	if (IO_closefile(p_name))	
		ctxt . SetTheResultToEmpty();
	else		
		ctxt . SetTheResultToStaticCString("file is not open");
}

void MCFilesExecCloseFile(MCExecContext& ctxt, MCNameRef p_filename)
{
	MCFilesExecPerformClose(ctxt, p_filename);
}

void MCFilesExecCloseDriver(MCExecContext& ctxt, MCNameRef p_device)
{
	MCFilesExecPerformClose(ctxt, p_device);
}

void MCFilesExecCloseProcess(MCExecContext& ctxt, MCNameRef p_process)
{
	uindex_t t_index;
	if (IO_findprocess(p_process, t_index))	
		MCS_closeprocess(t_index);	
	else	
		ctxt . SetTheResultToStaticCString("process is not open");
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesExecPerformReadFixedFor(MCExecContext& ctxt, IO_handle p_stream, int4 p_index, int p_unit_type, uint4 p_count, double p_max_wait, int p_time_units, intenum_t p_encoding, MCValueRef &r_output, IO_stat &r_stat)
{
	real8 t_duration = p_max_wait;
	switch (p_time_units)
	{
	case F_MILLISECS:
		t_duration /= 1000.0;
		break;
	case F_TICKS:
		t_duration /= 60.0;
		break;
	default:
		break;
	}

	uint4 size; 

    if (p_encoding != kMCFileEncodingNative && p_encoding != kMCFileEncodingBinary)
    {
        r_stat = IO_ERROR;
        return;
    }

	switch (p_unit_type)
	{
	case FU_INT1:
	case FU_UINT1:
		size = p_count;
		break;
	case FU_INT2:
	case FU_UINT2:
		size = p_count * 2;
		break;
	case FU_INT4:
	case FU_UINT4:
	case FU_REAL4:
		size = p_count * 4;
		break;
	case FU_INT8:
	case FU_UINT8:
		size = p_count * 4;
		break;
	case FU_REAL8:
		size = p_count * 8;
        break;
    case FU_CHARACTER:
    case FU_CODEPOINT:
    case FU_CODEUNIT:
    // AL-2014-25-06: [[ Bug 12650 ]] Unit type can be FU_BYTE
    case FU_BYTE:
        size = p_count;
        break;
    default:
        r_stat = IO_ERROR;
        return;
    }

    MCAutoNativeCharArray t_current;
	/* UNCHECKED */ t_current.New(size);

    uint4 tsize = 0;

	do 
	{
		uint4 rsize = size - tsize;
		uint4 fullsize = rsize;
        r_stat = MCS_readall(t_current.Chars() + tsize, rsize, p_stream, rsize);
		tsize += rsize;

		if (rsize < fullsize)
		{
			// MW-2010-10-25: [[ Bug 4022 ]] If we are reading from a process and we didn't
			//   get as much data as we wanted then do a sweep.
			if (p_index != -1)
				MCS_checkprocesses();

			if (((r_stat == IO_ERROR || r_stat == IO_EOF)
			        && (p_index == -1 || MCprocesses[p_index].pid == 0)))
			{
				r_stat = IO_EOF;
				break;
			}
			t_duration -= READ_INTERVAL;
			if (t_duration < 0.0)
			{
				r_stat = IO_TIMEOUT;
				break;
			}
			else
			{
				MCU_play();
				// MH-2007-05-18 [[Bug 4021]]: read from process times out too soon.
				// Originally the arguments to wait were READ_INTERVAL, False, True
				if (MCscreen->wait(READ_INTERVAL, False, False))
				{
					ctxt . LegacyThrow(EE_READ_ABORT);
					r_stat = IO_ERROR;
					return;
				}
			}
		}
	}
	while (tsize < size);

    bool t_success;
    t_success = true;
    
	MCStringRef t_buffer;
    t_buffer = nil;
    if (t_success)
        t_success = MCStringCreateMutable(0, t_buffer);
    
    bool t_used_buffer;
    t_used_buffer = true;
    
	uindex_t t_num_chars;
	switch (p_unit_type) 
	{
	case FU_INT1:
		{
			char buffer[I1L];
			int1 *i1ptr = (int1 *)t_current.Chars();
			for (uint4 i = 0 ; t_success && i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", i1ptr[i]);
				if (i != 0)
					t_success = MCStringAppendNativeChar(t_buffer, ',');
                if (t_success)
                    t_success = MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_INT2:
		{
			char buffer[I2L];
			int2 *i2ptr = (int2 *)t_current.Chars();
			for (uint4 i = 0 ; t_success && i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", i2ptr[i]);
				if (i != 0)
					t_success = MCStringAppendNativeChar(t_buffer, ',');
                if (t_success)
                    t_success = MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_INT4:
		{
			char buffer[I4L];
			int4 *i4ptr = (int4 *)t_current.Chars();
			for (uint4 i = 0 ; t_success && i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", i4ptr[i]);
				if (i != 0)
					t_success = MCStringAppendNativeChar(t_buffer, ',');
                if (t_success)
                    t_success = MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_INT8:
		{
			char buffer[I4L];
			int4 *i8ptr = (int4 *)t_current.Chars();
			for (uint4 i = 0 ; t_success && i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", i8ptr[i]);
				if (i != 0)
					t_success = MCStringAppendNativeChar(t_buffer, ',');
                if (t_success)
                    t_success = MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_REAL4:
		{
			char buffer[R4L];
			real4 *r4ptr = (real4 *)t_current.Chars();
			for (uint4 i = 0 ; t_success && i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%f", r4ptr[i]);
				if (i != 0)
					t_success = MCStringAppendNativeChar(t_buffer, ',');
                if (t_success)
                    t_success = MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_REAL8:
		{
			char buffer[R8L];
			real8 *r8ptr = (real8 *)t_current.Chars();
			for (uint4 i = 0 ; t_success && i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%lf", r8ptr[i]);
				if (i != 0)
					t_success = MCStringAppendNativeChar(t_buffer, ',');
                if (t_success)
                    t_success = MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_UINT1:
		{
			char buffer[U1L];
			uint1 *u1ptr = (uint1 *)t_current.Chars();
			for (uint4 i = 0 ; t_success && i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", u1ptr[i]);
				if (i != 0)
					t_success = MCStringAppendNativeChar(t_buffer, ',');
                if (t_success)
                    t_success = MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_UINT2:
		{
			char buffer[U2L];
			uint2 *u2ptr = (uint2 *)t_current.Chars();
			for (uint4 i = 0 ; t_success && i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", u2ptr[i]);
				if (i != 0)
					t_success && MCStringAppendNativeChar(t_buffer, ',');
				t_success && MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_UINT4:
		{
			char buffer[U4L];
			uint4 *u4ptr = (uint4 *)t_current.Chars();
			for (uint4 i = 0 ; t_success && i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", u4ptr[i]);
				if (i != 0)
					t_success && MCStringAppendNativeChar(t_buffer, ',');
                if (t_success)
                    t_success && MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_UINT8:
		{
			char buffer[U8L];
			uint4 *u8ptr = (uint4 *)t_current.Chars();
			for (uint4 i = 0 ; t_success && i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", u8ptr[i]);
				if (i != 0)
					t_success && MCStringAppendNativeChar(t_buffer, ',');
                if (t_success)
                    t_success && MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
        break;
    default:
        // AL-2014-06-12: [[ Bug 12195 ]] If the encoding is binary, return the bytes read as data
        if (p_encoding == kMCFileEncodingBinary)
            t_success = MCDataCreateWithBytes((byte_t*)t_current . Chars(), tsize, (MCDataRef&)r_output);
        else
            t_success = MCStringCreateWithBytes((byte_t*)t_current . Chars(), tsize, kMCStringEncodingNative, false, (MCStringRef&)r_output);
        
        // AL_2015-03-27: [[ Bug 15056 ]] Don't overwrite the output value with the buffer in this case.
        t_used_buffer = false;
        break;
	}
    if (t_success && t_used_buffer)
        t_success = MCStringCopyAndRelease(t_buffer, (MCStringRef&)r_output);
    else
        MCValueRelease(t_buffer);
	
	// If creating the buffer from the read data failed, then treat it as an IO error.
	// Otherwise leave 'stat' as it is - as it could be EOF (which isn't stricly a
	// failure).
    if (!t_success)
        r_stat = IO_ERROR;
}

// Refactoring of the waiting block used in MCFilesExecPerformRead*
// Returns false if an error occurred, true if the waiting was done properly
void MCFilesExecPerformWait(MCExecContext &ctxt, int4 p_index, real8 &x_duration, IO_stat &r_stat)
{
    // MW-2010-10-25: [[ Bug 4022 ]] If we are reading from a process and we didn't
    //   get as much data as we wanted then do a sweep.
    if (p_index != -1)
        MCS_checkprocesses();

    if (((r_stat == IO_ERROR || r_stat == IO_EOF)
            && (p_index == -1 || MCprocesses[p_index].pid == 0)))
    {
        r_stat = IO_EOF;
        return;
    }
    x_duration -= READ_INTERVAL;
    if (x_duration < 0.0)
    {
        r_stat = IO_TIMEOUT;
        return;
    }
    else
    {
        MCU_play();
        // MH-2007-05-18 [[Bug 4021]]: read from process times out too soon.
        // Originally the arguments to wait were READ_INTERVAL, False, True
        if (MCscreen->wait(READ_INTERVAL, False, False))
        {
            ctxt . LegacyThrow(EE_READ_ABORT);
            r_stat = IO_ERROR;
            return;
        }
    }
}

// Reads from the stream a codeunit and put it back in the end of the mutable buffer x_buffer
// For a UTF-8 character, it might read more than one codepoint, the number of codeunit read is returned
// SN-2014-06-18 [[ Bug 12538 ]] Read from process until empty
// Added p_empty_allowed, to allow a read to fail in case we read 'until empty'
uint4 MCFilesExecPerformReadCodeUnit(MCExecContext& ctxt, int4 p_index, intenum_t p_encoding, bool p_empty_allowed, real8 &x_duration, IO_handle p_stream, MCStringRef x_buffer, IO_stat &r_stat)
{
    uint4 t_codeunit_added = 0;

	do
	{
		MCAutoByteArray t_bytes;
		uint4 t_bytes_read = 0;
		switch(p_encoding)
		{
			case kMCFileEncodingNative:
			t_bytes . New(1);
			r_stat = MCS_readall(t_bytes.Bytes(), 1, p_stream, t_bytes_read);

			if (t_bytes_read == 1)
			{
				MCStringAppendNativeChar(x_buffer, (char)t_bytes.Bytes()[0]);
				t_codeunit_added = 1;
			}
            // SN-2014-12-02: [[ Bug 14135 ]] Do not wait if reading empty may occur
			else if (!p_empty_allowed)
				MCFilesExecPerformWait(ctxt, p_index, x_duration, r_stat);
			break;

		case kMCFileEncodingUTF16:
		case kMCFileEncodingUTF16LE:
		case kMCFileEncodingUTF16BE:
			t_bytes . New(2);
			r_stat = MCS_readall(t_bytes.Bytes(), 2, p_stream, t_bytes_read);

			if (t_bytes_read == 2 ||
					(t_bytes_read == 1 && r_stat == IO_EOF))
			{
				unichar_t t_codeunit;

				// Reverse the bytes in case it's needed
				if (p_encoding == kMCFileEncodingUTF16BE)
					t_codeunit = MCSwapInt16HostToBig(((unichar_t*)t_bytes . Bytes())[0]);
				else
					t_codeunit = *(unichar_t*)t_bytes . Bytes();

				MCStringAppendChar(x_buffer, t_codeunit);
				t_codeunit_added = 1;
            }
            // SN-2014-12-02: [[ Bug 14135 ]] Do not wait if reading empty may occur
			else if (!p_empty_allowed)
				MCFilesExecPerformWait(ctxt, p_index, x_duration, r_stat);
            break;
                
        case kMCFileEncodingUTF32:
        case kMCFileEncodingUTF32LE:
        case kMCFileEncodingUTF32BE:
            t_bytes . New(4);
            r_stat = MCS_readall(t_bytes.Bytes(), 4, p_stream, t_bytes_read);
            
            if (t_bytes_read == 4 || r_stat == IO_EOF)
            {
                MCAutoStringRef t_string;
                
                /* UNCHECKED */ MCStringCreateWithBytes(t_bytes . Bytes(), t_bytes_read, MCS_file_to_string_encoding((MCFileEncodingType)p_encoding), false, &t_string);
                /* UNCHECKED */ MCStringAppend(x_buffer, *t_string);
                
                t_codeunit_added = MCStringGetLength(*t_string);
            }
            // SN-2014-12-02: [[ Bug 14135 ]] Do not wait if reading empty may occur
            else if (!p_empty_allowed)
                MCFilesExecPerformWait(ctxt, p_index, x_duration, r_stat);
            break;
                
		case kMCFileEncodingUTF8:
			t_bytes . New(1);
			r_stat = MCS_readall(t_bytes . Bytes(), 1, p_stream, t_bytes_read);

			if (t_bytes_read == 1)
			{
				byte_t t_byte = t_bytes . Bytes()[0];
				uint4 t_to_read;
				bool t_sequence_correct;

				t_sequence_correct = true;

				if (t_byte < 0x80)
					t_to_read = 0;
				else if (t_byte < 0xC0) // invalid 10xxxxxx pattern
					break;
				else if (t_byte < 0xE0) // 2-byte long
					t_to_read = 1;
				else if (t_byte < 0xF0) // 3-byte long
					t_to_read = 2;
				else if (t_byte < 0xF8) // 4-byte long
					t_to_read = 3;
				else if (t_byte < 0xFC) // 5-byte long
					t_to_read = 4;
				else if (t_byte < 0xFE) // 6-byte long
					t_to_read = 5;
                else
                    break; // invalid 1111111x pattern

				// We need to read more bytes
				if (t_to_read)
				{
					uint4 t_rsize;

					// Read all the expected bytes from the lead one
					for (uint4 i = 1; i < t_to_read + 1 && t_sequence_correct; )
					{
						t_bytes . Extend(1);
						r_stat = MCS_readall(t_bytes . Bytes() + i, 1, p_stream, t_rsize);
						if (t_rsize != 1)
						{
							// If we couldn't read the byte, let's wait first
							MCFilesExecPerformWait(ctxt, p_index, x_duration, r_stat);

							// If there is a stream error, the sequence is incorrect
							t_sequence_correct = r_stat == IO_NORMAL;
						}
						else
						{
							// The sequence is correct if the byte starts with 110xxxxx
							t_sequence_correct = *((byte_t*)t_bytes . Bytes() + i) < 0xC0;
							++i;

							t_bytes_read += t_rsize;
						}
					}
				}

				if (t_sequence_correct)
				{
					MCAutoStringRef t_codepoints;
					MCStringCreateWithBytes(t_bytes . Bytes(), t_bytes_read, kMCStringEncodingUTF8, false, &t_codepoints);
					t_codeunit_added = MCStringGetLength(*t_codepoints);
					MCStringAppend(x_buffer, *t_codepoints);
				}
				else
				{
					// Append the <?> character ('?' in a diamond) instead of the sequence
					MCStringAppendChar(x_buffer, 0xFFFD);
					t_codeunit_added = 1;

					// Put back the last char if there is no stream error
					if (r_stat == IO_NORMAL)
						MCS_putback(t_bytes . Bytes()[t_bytes_read - 1], p_stream);
				}
            }
            // SN-2014-12-02: [[ Bug 14135 ]] Do not wait if reading empty may occur
			else if (!p_empty_allowed)
				MCFilesExecPerformWait(ctxt, p_index, x_duration, r_stat);

			break;
		default:
			r_stat = IO_ERROR;
		}
	}
    // SN-2014-06-18 [[ Bug 12538 ]] Read from process until empty
    // We only read once in case the reading is allowed to return empty
    // This avoid to get stuck in an infinite loop if the stream read always generates text
	while (!t_codeunit_added && !p_empty_allowed && r_stat == IO_NORMAL);

    return t_codeunit_added;
}

/*
 * Read the appropriate chunk from the stream.
 * p_last_char_boundary is the starting index of the next char - it's the length of the string if the last char full
 * Returns
 *      - on CODEUNIT, 0
 *      - on success, the end boundary of the last char loaded
 *      - on error, the one passed as parameter
 */
// SN-2014-06-18 [[ Bug 12538 ]] Read from process until empty
// Added p_empty_allowed, to allow a read to fail in case we read 'until empty'
bool MCFilesExecPerformReadChunk(MCExecContext &ctxt, int4 p_index, intenum_t p_encoding, bool p_empty_allowed, intenum_t p_file_unit, uint4 p_last_boundary, real8 &x_duration, IO_handle x_stream, MCStringRef x_buffer, uint4 &r_new_boundary, IO_stat &r_stat)
{
    switch (p_file_unit)
    {
    case FU_CODEUNIT:
        if (!MCFilesExecPerformReadCodeUnit(ctxt, p_index, p_encoding, p_empty_allowed, x_duration, x_stream, x_buffer, r_stat))
            return false;

        r_new_boundary = MCStringGetLength(x_buffer);
        break;

    case FU_CODEPOINT:
        if (MCStringGetLength(x_buffer) == p_last_boundary
                || MCStringIsEmpty(x_buffer))
        {
            // We are at the beginning of a char
            if (!MCFilesExecPerformReadCodeUnit(ctxt, p_index, p_encoding, p_empty_allowed, x_duration, x_stream, x_buffer, r_stat))
                return false;
        }
        if (MCUnicodeCodepointIsHighSurrogate(MCStringGetCharAtIndex(x_buffer, p_last_boundary)))
        {
            if (MCStringGetLength(x_buffer) - p_last_boundary == 1)
            {
                // Having a lead surrogate in the end, we need to read the next codeunit of the codepoint
                if (MCFilesExecPerformReadCodeUnit(ctxt, p_index, p_encoding, p_empty_allowed, x_duration, x_stream, x_buffer, r_stat))
                    // We failed at reading, so we just add a single codeunit codepoint.
                    r_new_boundary = p_last_boundary + 1;
                else
                    // We have successfully got the trail surrogate
                    r_new_boundary = p_last_boundary + 2;

                break;
            }
        }
        else
            // No lead/trail surrogate pair loaded - but up to 2 codeunit from UTF-8
            r_new_boundary = MCStringGetLength(x_buffer);

        break;
    case FU_CHARACTER:
    {
        // In order to make the reading of 1 char faster, we use a temporary,
        // small buffer.
        // We copy the codeunit(s) we read over the last character boundary at
        //  the previous reading, and put it into our reading buffer.
        MCAutoStringRef t_read_buffer;
        uindex_t t_initial_length;
        
        if (!MCStringMutableCopySubstring(x_buffer, MCRangeMake(p_last_boundary, UINDEX_MAX), &t_read_buffer))
            return false;
        
        // We keep track of the number of codeunit that we have copied for the
        //  main buffer.
        t_initial_length = MCStringGetLength(*t_read_buffer);
        
        while(true)
        {
            uint4 t_codeunit_read = MCFilesExecPerformReadCodeUnit(ctxt, p_index, p_encoding, p_empty_allowed, x_duration, x_stream, *t_read_buffer, r_stat);

            if (!t_codeunit_read)
            {
                r_new_boundary = MCStringGetLength(x_buffer);

                // In case we came across EOF before the char is finished, we want to had the codepoints that have been read
                if (r_new_boundary != p_last_boundary)
                {
                    // EOF is actually not encountered.
                    if (r_stat == IO_EOF)
                        r_stat = IO_NORMAL;
                    break;
                }
                else
                    return false;
            }

            MCRange t_cu_range, t_char_range;
            t_cu_range = MCRangeMake(0, MCStringGetLength(*t_read_buffer));
            MCStringUnmapIndices(*t_read_buffer, kMCCharChunkTypeGrapheme, t_cu_range, t_char_range);

            if (t_char_range . length > 1)
            {
                // We append the reading buffer MINUS the initial codeunits
                MCAutoStringRef t_new_codeunits;
                if (!MCStringCopySubstring(*t_read_buffer, MCRangeMake(t_initial_length, UINDEX_MAX), &t_new_codeunits)
                        || !MCStringAppend(x_buffer, *t_new_codeunits))
                    return false;
                
                // The last codeunit is now on part of a new character:
                //  we can end here the loop, and appendmust end now the loop and mark the position
                r_new_boundary = MCStringGetLength(x_buffer) - t_codeunit_read;
                break;
            }
        }
    }
        break;

    default:
        r_stat = IO_ERROR;
        return false;
    }

    return true;
}

/*
 *  This function comes to read an unpredictable amount of bytes for the number of units asked,
 *  depending of the encoding in use and of the way the characters are encoded
 */
void MCFilesExecPerformReadUnicodeFor(MCExecContext& ctxt, IO_handle p_stream, int4 p_index, int p_unit_type, uint4 p_count, double p_max_wait, int p_time_units, intenum_t p_encoding, MCStringRef &r_output, IO_stat &r_stat)
{
    real8 t_duration = p_max_wait;
    switch (p_time_units)
    {
    case F_MILLISECS:
        t_duration /= 1000.0;
        break;
    case F_TICKS:
        t_duration /= 60.0;
        break;
    default:
        break;
    }

    if (p_unit_type < FU_CHARACTER
            || p_unit_type > FU_CODEPOINT)
    {
        r_stat = IO_ERROR;
        return;
    }

    MCAutoStringRef t_output;
    MCStringCreateMutable(0, &t_output);

    uint4 t_last_char_boundary = 0;
    uint4 t_progress = 0;
    IO_stat t_stat;

    while (t_progress < p_count)
    {
        uint4 t_new_boundary = 0;
        // SN-2014-06-18 [[ Bug 12538 ]] Read from process until empty
        // We need to allow a reading to return nothing, without being stuck in a waiting loop for data
        if (!MCFilesExecPerformReadChunk(ctxt, p_index, p_encoding, false, p_unit_type, t_last_char_boundary, t_duration, p_stream, *t_output, t_new_boundary,t_stat))
            // An error occurred during the reading
            break;

        // Update the position of the last char boundary
        t_last_char_boundary = t_new_boundary;
        ++t_progress;
    }

    if (!MCStringCopySubstring(*t_output, MCRangeMake(0, t_last_char_boundary), r_output))
        r_stat = IO_ERROR;
    else
        r_stat = t_stat;
}

void MCFilesExecPerformReadTextUntil(MCExecContext& ctxt, IO_handle p_stream, int4 p_index, uint4 p_count, const MCStringRef p_sentinel, Boolean words, double p_max_wait, int p_time_units, intenum_t p_encoding, MCStringRef &r_output, IO_stat &r_stat)
{
	real8 t_duration = p_max_wait;
	switch (p_time_units)
	{
	case F_MILLISECS:
		t_duration /= 1000.0;
		break;
	case F_TICKS:
		t_duration /= 60.0;
		break;
	default:
		break;
	}

    MCAutoStringRef t_output;
    /* UNCHECKED */ MCStringCreateMutable(0, &t_output);

    // We don't accept binary or badly processed files
    if (p_encoding == kMCFileEncodingText
            || p_encoding == kMCFileEncodingBinary)
    {
        r_stat = IO_ERROR;
        return;
    }

    uint4 t_last_char_boundary = 0;
    Boolean doingspace = True;
    IO_stat t_stat = IO_NORMAL;

    MCAutoStringRef t_norm_sent;
    MCStringNormalizedCopyNFC(p_sentinel, &t_norm_sent);

    MCAutoArray<unichar_t> t_norm_buf;
    
    // SN-2014-06-18 [[ Bug 12538 ]] Read from process until empty
    // We want to shortcut the waiting loop in case we accept that the reading returns empty
    bool t_empty_allowed;
    t_empty_allowed = MCStringIsEmpty(p_sentinel);

    while (p_count)
    {
        uint4 t_new_char_boundary = 0;
        if (!MCFilesExecPerformReadChunk(ctxt, p_index, p_encoding, t_empty_allowed, FU_CODEPOINT, t_last_char_boundary, t_duration, p_stream, *t_output, t_new_char_boundary, t_stat))
            // error occurred while reading a codepoint
            break;

        if (words)
        {
            if (doingspace)
            {
                if (!MCUnicodeIsWhitespace(MCStringGetCharAtIndex(*t_output, t_last_char_boundary)))
                    doingspace = False;
            }
            else
                if (MCUnicodeIsWhitespace(MCStringGetCharAtIndex(*t_output, t_last_char_boundary)))
                {
                    --p_count;
                    doingspace = True;
                }
        }
        else if (!MCStringIsEmpty(*t_norm_sent))
        {
            // Normalise the character read and append it to the normalised buffer
            unichar_t *t_norm_chunk;
            uint4 t_norm_chunk_size;

            MCUnicodeNormaliseNFC(MCStringGetCharPtr(*t_output) + t_last_char_boundary, t_new_char_boundary - t_last_char_boundary, t_norm_chunk, t_norm_chunk_size);

            // Append the normalised chunk read to the normalised buffer
            uint4 t_previous_size = t_norm_buf . Size();
            /* UNCHECKED */ t_norm_buf . Extend(t_previous_size + t_norm_chunk_size);
            memcpy(t_norm_buf . Ptr() + t_previous_size, t_norm_chunk, t_norm_chunk_size * sizeof(unichar_t));
            MCMemoryDelete(t_norm_chunk);

            uint4 i = MCStringGetLength(*t_norm_sent) - 1;
            uint4 j = t_norm_buf . Size() - 1;

            while (i && j && t_norm_buf[j] == MCStringGetCharAtIndex(*t_norm_sent, i))
            {
                i--;
                j--;
            }
            if (i == 0 && t_norm_buf[j] == MCStringGetCharAtIndex(*t_norm_sent, 0))
                --p_count;
            else if (MCStringGetCharAtIndex(*t_norm_sent, 0) == '\n' && t_norm_buf[j] == '\r')
            {
                // MW-2008-08-15: [[ Bug 6580 ]] This clause looks ahead for CR LF sequences
                //   if we have just enp_countered CR. However, it was previousy using MCS_seek_cur
                //   to retreat, which *doesn't* work for process p_streams.
                if (MCStringGetLength(*t_norm_sent) == 1)
                {
                    if (t_new_char_boundary != MCStringGetLength(*t_output))
                    {
                        // We already have the next char loaded
                        // If it's a '\n', we remove it
                        if (MCStringGetCharAtIndex(*t_output, t_new_char_boundary) == '\n')
                            /* UNCHECKED */ MCStringRemove(*t_output, MCRangeMake(t_new_char_boundary, 1));
                    }
                    else
                    {
                        // We need to read the next char
                        uint1 term;
                        uint4 nread = 1;
                        if (MCS_readall(&term, nread, p_stream, nread) == IO_NORMAL)
                        {
                            if (term != '\n')
                                MCS_putback(term, p_stream);
                            else
                                // Reaching that point, we want to change the last char of the buffer (being a lone, byte-wide <CR>) to an LF
                                /* UNCHECKED */ MCStringReplace(*t_output, MCRangeMake(t_last_char_boundary, 1), MCSTR("\n"));
                        }
                    }
                }
                --p_count;
            }
        }
        // Update the position of the last char boundary
        t_last_char_boundary = t_new_char_boundary;
    }

    // We need to discard any char read over the actual amount needed
    MCStringCopySubstring(*t_output, MCRangeMake(0, t_last_char_boundary), r_output);
    r_stat = t_stat;
}

void MCFilesExecPerformReadBinaryUntil(MCExecContext& ctxt, IO_handle stream, int4 p_index, uint4 p_count, const MCStringRef p_sentinel, Boolean words, double p_max_wait, int p_time_units, MCDataRef &r_output, IO_stat &r_stat)
{
	real8 t_duration = p_max_wait;
	switch (p_time_units)
	{
	case F_MILLISECS:
		t_duration /= 1000.0;
		break;
	case F_TICKS:
		t_duration /= 60.0;
		break;
	default:
		break;
	}

	uint4 tsize;
	tsize = BUFSIZ;
	MCAutoNativeCharArray t_buffer;
	/* UNCHECKED */ t_buffer.New(BUFSIZ);

	uint4 fullsize;
	if (MCStringGetLength(p_sentinel) == 0)
		fullsize = BUFSIZ;
	else
		fullsize = 1;

	uint4 endcount = MCStringGetLength(p_sentinel) - 1;
	
	uint4 size = 0;
    Boolean doingspace = True;
	while (True)
	{
		uint4 rsize = fullsize;
		if (size + rsize > tsize)
		{
			/* UNCHECKED */ t_buffer.Extend(tsize + BUFSIZ);
			tsize += BUFSIZ;
		}
		r_stat = MCS_readall(t_buffer.Chars() + size, rsize, stream, rsize);
		size += rsize;
		if (rsize < fullsize)
		{
			// MW-2010-10-25: [[ Bug 4022 ]] If we are reading from a process and we didn't
			//   get as much data as we wanted then do a sweep.
			if (p_index != -1)
				MCS_checkprocesses();

			if (MCStringGetLength(p_sentinel) == 0
			        || ((r_stat == IO_ERROR || r_stat == IO_EOF)
			            && (p_index == -1 || MCprocesses[p_index].pid == 0)))
			{
				r_stat = IO_EOF;
				break;
			}
			t_duration -= READ_INTERVAL;
			if (t_duration <= 0)
			{
				r_stat = IO_TIMEOUT;
				break;
			}
			else
				if (MCscreen->wait(READ_INTERVAL, False, True))
				{
					ctxt . LegacyThrow(EE_READ_ABORT);
					r_stat = IO_ERROR;
					return;
				}
		}
		else
		{
			if (words)
			{
				if (doingspace)
				{
					if (!isspace(t_buffer.Chars()[size - 1]))
						doingspace = False;
				}
				else
					if (isspace(t_buffer.Chars()[size - 1]))
                    {
						if (--p_count == 0)
							break;
						else
							doingspace = True;
                    }
			}
			else
			{
				if (MCStringGetLength(p_sentinel) != 0)
				{
					uint4 i = endcount;
					uint4 j = size - 1;
                    while (i && j && t_buffer.Chars()[j] == MCStringGetNativeCharAtIndex(p_sentinel, i))
					{
						i--;
						j--;
					}
					if (i == 0 && (t_buffer.Chars()[j] == MCStringGetNativeCharAtIndex(p_sentinel, 0)))
					{
						if (--p_count == 0)
							break;
					}
				}
			}
		}
	}
	t_buffer.Shrink(size);
    // AL-2014-06-12: [[ Bug 12195 ]] If the encoding is binary, return the bytes read as data
    if (!MCDataCreateWithBytes((byte_t*)t_buffer . Chars(), size, (MCDataRef&)r_output))
        r_stat = IO_ERROR;
}

static void MCFilesReadComplete(MCExecContext& ctxt, MCValueRef p_output, IO_stat p_stat, Boolean t_textmode)
{
	MCshellfd = -1;
	switch (p_stat)
	{
	case IO_ERROR:
		ctxt . SetTheResultToStaticCString("error reading file");
		break;
	case IO_EOF:
		ctxt . SetTheResultToStaticCString("eof");
		break;
	case IO_TIMEOUT:
		ctxt . SetTheResultToStaticCString("timed out");
		break;
	default:
		ctxt . SetTheResultToEmpty();
		break;
    }

    if (p_output != nil)
    {
        if (t_textmode)
        {
            MCAutoStringRef t_output;
            if (!MCStringNormalizeLineEndings((MCStringRef)p_output, 
                                              kMCStringLineEndingStyleLF, 
                                              kMCStringLineEndingOptionNormalizePSToLineEnding |
                                              kMCStringLineEndingOptionNormalizeLSToVT, 
                                              &t_output, 
                                              nullptr))
            {
                ctxt . SetItToEmpty();
                ctxt . SetTheResultToStaticCString("error normalizing line endings");
            }
            else
            {
                ctxt . SetItToValue(*t_output);
            }
        }
        else
        {
            ctxt . SetItToValue((MCDataRef)p_output);
        }
    }
    else
        ctxt . SetItToEmpty();
}

void MCFilesExecReadUntil(MCExecContext& ctxt, IO_handle p_stream, index_t p_index, MCStringRef p_sentinel, double p_max_wait, int p_time_units, intenum_t p_encoding, MCValueRef &r_output, IO_stat &r_stat)
{
	MCAutoStringRef t_sentinel;
	if (p_sentinel != nil)
		MCStringCopy(p_sentinel, &t_sentinel);
	else
		/* UNCHECKED */ MCStringCreateWithCString("end", &t_sentinel);

	// MW-2009-11-03: [[ Bug 8402 ]] Use a different stream array, depending on what
	//   type of stream we are reading from.
    if (p_encoding == kMCFileEncodingBinary)
        MCFilesExecPerformReadBinaryUntil(ctxt, p_stream, p_index, 1, *t_sentinel, False, p_max_wait, p_time_units, (MCDataRef&)r_output, r_stat);
	else
        MCFilesExecPerformReadTextUntil(ctxt, p_stream, p_index, 1, *t_sentinel, False, p_max_wait, p_time_units, p_encoding, (MCStringRef &)r_output, r_stat);
}

void MCFilesExecReadFor(MCExecContext& ctxt, IO_handle p_stream, index_t p_index, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units, intenum_t p_encoding, MCValueRef &r_output, IO_stat &r_stat)
{
	MCAutoStringRef t_sentinel;
    Boolean t_words;

	switch (p_unit_type)
	{
	case FU_LINE:
		MCStringCreateWithCString("\n", &t_sentinel);
        t_words = False;
		break;
	case FU_ITEM:
		MCStringCreateWithCString(",", &t_sentinel);
        t_words = False;
		break;
	case FU_WORD:
		MCStringCreateWithCString(" ", &t_sentinel);
        t_words = True;
        break;
    default:
        if (p_encoding == kMCFileEncodingNative || p_encoding == kMCFileEncodingBinary)
            MCFilesExecPerformReadFixedFor(ctxt, p_stream, p_index, p_unit_type, p_count, p_max_wait, p_time_units, p_encoding, r_output, r_stat);
        else
            MCFilesExecPerformReadUnicodeFor(ctxt, p_stream, p_index, p_unit_type, p_count, p_max_wait, p_time_units, p_encoding, (MCStringRef&)r_output, r_stat);

        return;
	}

    if (p_encoding == kMCFileEncodingBinary)
        MCFilesExecPerformReadBinaryUntil(ctxt, p_stream, p_index, p_count, *t_sentinel, t_words, p_max_wait, p_time_units, (MCDataRef&)r_output, r_stat);
    else
        MCFilesExecPerformReadTextUntil(ctxt, p_stream, p_index, p_count, *t_sentinel, t_words, p_max_wait, p_time_units, p_encoding, (MCStringRef&)r_output, r_stat);
}

void MCFilesExecReadFromStdin(MCExecContext& ctxt, MCStringRef p_sentinel, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units, uint2 p_repeat_form)
{
	IO_stat t_stat = IO_NORMAL;
	MCAutoValueRef t_output;

#ifndef _SERVER
	if (!MCnoui && MCS_isinteractiveconsole(0))
	{
		ctxt . SetTheResultToStaticCString("eof");
		ctxt . SetItToValue(kMCEmptyString);
		return;
	}
	else
#endif

	switch (p_repeat_form)
	{
	case RF_FOR:
        MCFilesExecReadFor(ctxt, IO_stdin, -1, p_count, p_unit_type, p_max_wait, p_time_units, kMCFileEncodingNative, &t_output, t_stat);
		break;
	case RF_UNTIL:
        MCFilesExecReadUntil(ctxt, IO_stdin, -1, p_sentinel, p_max_wait, p_time_units, kMCFileEncodingNative, &t_output, t_stat);
		break;
	default:
		break;
	}
    MCFilesReadComplete(ctxt, *t_output, t_stat, True);
}

void MCFilesExecReadFromStdinFor(MCExecContext& ctxt, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromStdin(ctxt, nil, p_count, p_unit_type, p_max_wait, p_time_units, RF_FOR);
}

void MCFilesExecReadFromStdinUntil(MCExecContext& ctxt, MCStringRef p_sentinel, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromStdin(ctxt, p_sentinel, 0, 0, p_max_wait, p_time_units, RF_UNTIL);
}

void MCFilesExecReadGetStream(MCExecContext& ctxt, MCNameRef p_name, bool p_is_end, int64_t p_at, bool p_has_at, IO_handle &r_stream, intenum_t &r_encoding, IO_stat &r_stat)
{
	uindex_t t_index;
	if (!IO_findfile(p_name, t_index) || MCfiles[t_index].mode == OM_APPEND
	        || MCfiles[t_index].mode == OM_WRITE)
	{
		ctxt . SetTheResultToStaticCString("file is not open for read");
		return;
	}
	r_stream = MCfiles[t_index].ihandle;
    r_encoding = MCfiles[t_index].encoding;

	if (p_has_at)
    {
		if (p_is_end)
			r_stat = MCS_seek_end(r_stream, p_at);
		else
			r_stat = MCS_seek_set(r_stream, p_at);
	}
    else
        MCS_sync(r_stream);
}

void MCFilesExecReadFromFileOrDriverFor(MCExecContext& ctxt, bool p_driver, bool p_is_end, MCNameRef p_file, int64_t p_at, bool p_has_at, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units)
{
	IO_handle t_stream = NULL;
    MCFileEncodingType t_encoding;
	IO_stat t_stat = IO_NORMAL;
	
    MCFilesExecReadGetStream(ctxt, p_file, p_is_end, p_at, p_has_at, t_stream, (intenum_t&)t_encoding, t_stat);
	
	if (t_stream == NULL)
		return;

	if (t_stat != IO_NORMAL)
	{	
		ctxt . SetTheResultToStaticCString("error seeking in file");
		return;
	}

	MCAutoValueRef t_output;

    MCFilesExecReadFor(ctxt, t_stream, -1, p_count, p_unit_type, p_max_wait, p_time_units, t_encoding, &t_output, t_stat);
    MCFilesReadComplete(ctxt, *t_output, t_stat, t_encoding != kMCFileEncodingBinary);

#if !defined _WIN32 && !defined _MACOSX
	MCS_sync(t_stream);
#endif
}

void MCFilesExecReadFromFileOrDriverUntil(MCExecContext& ctxt, bool p_driver, bool p_is_end, MCNameRef p_file, MCStringRef p_sentinel, int64_t p_at, bool p_has_at, double p_max_wait, int p_time_units)
{
	IO_handle t_stream = NULL;
    MCFileEncodingType t_encoding;
	IO_stat t_stat = IO_NORMAL;
	
    MCFilesExecReadGetStream(ctxt, p_file, p_is_end, p_at, p_has_at, t_stream, (intenum_t&)t_encoding, t_stat);
	
	if (t_stream == NULL)
		return;

	if (t_stat != IO_NORMAL)
	{	
		ctxt . SetTheResultToStaticCString("error seeking in file");
		return;
	}
	
    MCValueRef t_output;
    t_output = nil;
	if (MCStringGetLength(p_sentinel) == 1 && MCStringGetNativeCharAtIndex(p_sentinel, 0) == '\004')
	{
		MCAutoDataRef t_data;
        t_stat = IO_read_to_eof(t_stream, &t_data);

        if (t_stat == IO_NORMAL)
        {
            t_stat = IO_EOF;
            
            if (t_encoding != kMCFileEncodingBinary)
            {
                if (!MCStringCreateWithBytes(MCDataGetBytePtr(*t_data), MCDataGetLength(*t_data), MCS_file_to_string_encoding(t_encoding), false, (MCStringRef&)t_output))
                    t_stat = IO_ERROR;
            }
            else
                t_output = MCValueRetain(*t_data);
        }
	}
    else
    {
        MCFilesExecReadUntil(ctxt, t_stream, -1, p_sentinel, p_max_wait, p_time_units, t_encoding, t_output, t_stat);
    }
    
    MCFilesReadComplete(ctxt, t_output, t_stat, t_encoding != kMCFileEncodingBinary);
    MCValueRelease(t_output);

#if !defined _WIN32 && !defined _MACOSX
	MCS_sync(t_stream);
#endif
}

void MCFilesExecReadFromFileOrDriverFor(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverFor(ctxt, p_driver, false, p_file, 0, false, p_count, p_unit_type, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverUntil(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, MCStringRef p_sentinel, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverUntil(ctxt, p_driver, false, p_file, p_sentinel, 0, false, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverAtFor(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, int64_t p_at, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverFor(ctxt, p_driver, false, p_file, p_at, true, p_count, p_unit_type, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverAtUntil(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, int64_t p_at, MCStringRef p_sentinel, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverUntil(ctxt, p_driver, false, p_file, p_sentinel, p_at, true, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverAtEndFor(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, int64_t p_at, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverFor(ctxt, p_driver, true, p_file, p_at, true, p_count, p_unit_type, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverAtEndForLegacy(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, intenum_t p_eof, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverFor(ctxt, p_driver, true, p_file, 0, true, p_count, p_unit_type, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverAtEndUntil(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, int64_t p_at, MCStringRef p_sentinel, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverUntil(ctxt, p_driver, true, p_file, p_sentinel, p_at, true, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverAtEndUntilLegacy(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, intenum_t p_eof, MCStringRef p_sentinel, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverUntil(ctxt, p_driver, true, p_file, p_sentinel, 0, true, p_max_wait, p_time_units);
}


void MCFilesExecReadFromProcess(MCExecContext& ctxt, MCNameRef p_process, MCStringRef p_sentinel, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units, uint2 p_repeat_form)
{
	uindex_t t_index;
	if (!IO_findprocess(p_process, t_index) || MCprocesses[t_index].mode == OM_APPEND ||
		MCprocesses[t_index].mode == OM_WRITE || MCprocesses[t_index].mode == OM_NEITHER)
	{
		ctxt . SetTheResultToStaticCString("process is not open for read");
		// MH-2007-05-18 [[Bug 4020]]: A read from an unopened process would throw an error, which is inconsistent with general behaviour through "the result".
		return;
	}

	IO_handle t_stream = NULL;
    intenum_t t_encoding;
	IO_stat t_stat = IO_NORMAL;
	t_stream = MCprocesses[t_index].ihandle;
    t_encoding = MCprocesses[t_index].encoding;
	MCAutoValueRef t_output;

	switch (p_repeat_form)
	{
	case RF_FOR:
        MCFilesExecReadFor(ctxt, t_stream, t_index, p_count, p_unit_type, p_max_wait, p_time_units, t_encoding, &t_output, t_stat);
		break;
	case RF_UNTIL:
    {
        // MW-2014-10-23: [[ Bug ]] Only prod the 'sentinal' if its the until case (otherwise
        //   it is nil).
        // SN-2014-10-14: [[ Bug 13658 ]] In case we want to read everything (EOF, end, empty) from a binary process,
        //  the sentinel must be empty, not Ctrl-D (0x04, which might appear in a binary data output.
        MCAutoStringRef t_sentinel;
        if (MCprocesses[t_index] . encoding == (Encoding_type) kMCFileEncodingBinary &&
            MCStringGetLength(p_sentinel) == 1 && MCStringGetCharAtIndex(p_sentinel, 0) == 0x4)
            t_sentinel = kMCEmptyString;
        else
            t_sentinel = p_sentinel;

        MCFilesExecReadUntil(ctxt, t_stream, t_index, *t_sentinel, p_max_wait, p_time_units, t_encoding, &t_output, t_stat);
		break;
	}
	default:  
		break;
	}

    MCFilesReadComplete(ctxt, *t_output, t_stat, t_encoding != kMCFileEncodingBinary);
}

void MCFilesExecReadFromProcessFor(MCExecContext& ctxt, MCNameRef p_process, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromProcess(ctxt, p_process, nil, p_count, p_unit_type, p_max_wait, p_time_units, RF_FOR);
}

void MCFilesExecReadFromProcessUntil(MCExecContext& ctxt, MCNameRef p_process, MCStringRef p_sentinel, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromProcess(ctxt, p_process, p_sentinel, 0, 0, p_max_wait, p_time_units, RF_UNTIL);
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesExecWriteToStream(MCExecContext& ctxt, IO_handle p_stream, MCStringRef p_data, int p_unit_type, intenum_t p_encoding, IO_stat &r_stat)
{
	uint4 len = MCStringGetLength(p_data);
	uindex_t t_data_pos;
	t_data_pos = 0;

	switch (p_unit_type)
	{
	case FU_CHARACTER:
	case FU_ITEM:
	case FU_LINE:
	case FU_WORD:
        {
            switch(p_encoding)
            {
            // MCWrite always has FU_CHARACTER as file unit when the file is opened for 'binary write'
            case kMCFileEncodingBinary:
            case kMCFileEncodingNative:
                {
                    MCAutoStringRefAsCString t_output;
                    /* UNCHECKED */ t_output . Lock(p_data);
                    r_stat = MCS_write(*t_output, sizeof(char), len, p_stream);
                    break;
                }
            case kMCFileEncodingUTF8:
                {
                    MCAutoStringRefAsUTF8String t_output;
                    /* UNCHECKED */ t_output . Lock(p_data);
                    r_stat = MCS_write(*t_output, sizeof(char), t_output . Size(), p_stream);
                    break;
                }
            case kMCFileEncodingUTF16:
                {
                    r_stat = MCS_write(MCStringGetCharPtr(p_data), sizeof(unichar_t), len, p_stream);
                    break;
                }
            case kMCFileEncodingUTF16LE:
            case kMCFileEncodingUTF16BE:
            case kMCFileEncodingUTF32:
            case kMCFileEncodingUTF32BE:
            case kMCFileEncodingUTF32LE:
                {
                    MCAutoDataRef t_output;
                    if (MCStringEncode(p_data, MCS_file_to_string_encoding((MCFileEncodingType)p_encoding), false, &t_output))
                        r_stat = MCS_write(MCDataGetBytePtr(*t_output), 1, MCDataGetLength(*t_output), p_stream);
                    else
                        r_stat = IO_ERROR;
                    break;
                }
            default:
                r_stat = IO_ERROR;
                break;
            }
        }
		break;
	default:
		{
            r_stat = IO_NORMAL;
			while (len)
			{
				uindex_t t_start_pos;
				t_start_pos = t_data_pos;
				if (!MCStringFirstIndexOfChar(p_data, ',', 0, kMCCompareExact, len))
				{
					t_data_pos += len;
					len = 0;
				}
				MCAutoStringRef s;
				/* UNCHECKED */ MCStringCopySubstring(p_data, MCRangeMakeMinMax(t_start_pos, t_data_pos), &s); 
				real8 n;
				if (!MCTypeConvertStringToReal(*s, n))
				{
					ctxt . LegacyThrow(EE_FUNCTION_NAN);
					r_stat = IO_ERROR;
					break;
				}
				t_data_pos++;
				switch (p_unit_type)
				{
				case FU_INT1:
					r_stat = IO_write_int1((int1)n, p_stream);
					break;
				case FU_INT2:
					r_stat = IO_write_int2((int2)n, p_stream);
					break;
				case FU_INT4:
					r_stat = IO_write_int4((int4)n, p_stream);
					break;
				case FU_INT8:
					if (n < 0)
						r_stat = IO_write_int4(-1, p_stream);
					else
						r_stat = IO_write_int4(0, p_stream);
					if (r_stat == IO_NORMAL)
						r_stat = IO_write_int4((int4)n, p_stream);
					break;
				case FU_REAL4:
					r_stat = IO_write_real4((real4)n, p_stream);
					break;
				case FU_REAL8:
					r_stat = IO_write_real8(n, p_stream);
					break;
				case FU_UINT1:
					r_stat = IO_write_uint1((uint1)n, p_stream);
					break;
				case FU_UINT2:
					r_stat = IO_write_uint2((uint2)n, p_stream);
					break;
				case FU_UINT4:
					r_stat = IO_write_uint4((uint4)n, p_stream);
					break;
				case FU_UINT8:
					r_stat = IO_write_uint4(0, p_stream);
					if (r_stat == IO_NORMAL)
						r_stat = IO_write_uint4((uint4)n, p_stream);
					break;
				default:
                    r_stat = IO_ERROR;
					break;
				}
			}
			if (r_stat != IO_NORMAL)
				break;
		}
		break;
	}
}

void MCFilesExecWriteGetStream(MCExecContext& ctxt, MCNameRef p_name, bool p_is_end, int64_t p_at, bool p_has_at, IO_handle &r_stream, intenum_t &r_encoding, IO_stat &r_stat)
{
	uindex_t t_index;
	if (!IO_findfile(p_name, t_index) || MCfiles[t_index].mode == OM_NEITHER || 
		MCfiles[t_index].mode == OM_READ)
	{
		ctxt . SetTheResultToStaticCString("file is not open for write");
		return;
	}
    r_stream = MCfiles[t_index].ohandle;
    r_encoding = MCfiles[t_index].encoding;

	if (p_has_at)
		if (p_at < 0)
			MCS_seek_end(r_stream, p_at);
		else
			MCS_seek_set(r_stream, p_at);
	else if (p_is_end)
		r_stat = MCS_seek_end(r_stream, 0);
}

void MCFilesExecWriteToStdout(MCExecContext& ctxt, MCStringRef p_data, int p_unit_type)
{
	IO_stat t_stat;
    // ENCODING what encoding for STD?
    MCFilesExecWriteToStream(ctxt, IO_stdout, p_data, p_unit_type, kMCFileEncodingNative, t_stat);

	if (t_stat != IO_NORMAL)
		ctxt . SetTheResultToStaticCString("error writing file");
	else
		ctxt . SetTheResultToEmpty();
}
void MCFilesExecWriteToStderr(MCExecContext& ctxt, MCStringRef p_data, int p_unit_type)
{
	IO_stat t_stat;
    MCFilesExecWriteToStream(ctxt, IO_stderr, p_data, p_unit_type, kMCFileEncodingNative, t_stat);

	if (t_stat != IO_NORMAL)
		ctxt . SetTheResultToStaticCString("error writing file");
	else
		ctxt . SetTheResultToEmpty();
}


void MCFilesExecWriteToFileOrDriver(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, bool p_is_end, int p_unit_type, int64_t p_at, bool p_has_at)
{
	
	IO_handle t_stream = NULL;
    MCFileEncodingType t_encoding;
	IO_stat t_stat = IO_NORMAL;
	
    MCFilesExecWriteGetStream(ctxt, p_file, p_is_end, p_at, p_has_at, t_stream, (intenum_t&)t_encoding, t_stat);
	
	if (t_stream == NULL)
		return;

	if (t_stat != IO_NORMAL)
	{
		ctxt . SetTheResultToStaticCString("error seeking in file");
		return;
	}

    if (t_encoding != kMCFileEncodingBinary)
    {
        MCAutoStringRef t_text_data;
        if (!MCStringNormalizeLineEndings(p_data, 
                                          kMCStringLineEndingStyleLegacyNative, 
                                          false, 
                                          &t_text_data, 
                                          nullptr))
        {
            return;
        }
        MCFilesExecWriteToStream(ctxt, t_stream, *t_text_data, p_unit_type, t_encoding, t_stat);
    }
    else
        MCFilesExecWriteToStream(ctxt, t_stream, p_data, p_unit_type, t_encoding, t_stat);

	if (t_stat != IO_NORMAL)
	{
		ctxt . SetTheResultToStaticCString("error writing file");
		return;
	}
	ctxt . SetTheResultToEmpty();

#if !defined _WIN32 && !defined _MACOSX
	MCS_flush(t_stream);
#endif
	
}

void MCFilesExecWriteToFileOrDriver(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, int p_unit_type)
{
	MCFilesExecWriteToFileOrDriver(ctxt, p_file, p_data, false, p_unit_type, 0, false);
}

void MCFilesExecWriteToFileOrDriverAt(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, int p_unit_type, int64_t p_at)
{
	MCFilesExecWriteToFileOrDriver(ctxt, p_file, p_data, false, p_unit_type, p_at, true);
}

void MCFilesExecWriteToFileOrDriverAtEnd(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, int p_unit_type)
{
	MCFilesExecWriteToFileOrDriver(ctxt, p_file, p_data, true, p_unit_type, 0, false);
}

void MCFilesExecWriteToFileOrDriverAtEndLegacy(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, int p_unit_type, intenum_t p_eof)
{
	MCFilesExecWriteToFileOrDriver(ctxt, p_file, p_data, true, p_unit_type, 0, false);
}

void MCFilesExecWriteToProcess(MCExecContext& ctxt, MCNameRef p_process, MCStringRef p_data, int p_unit_type)
{
	uindex_t t_index;
	if (!IO_findprocess(p_process, t_index)  || MCprocesses[t_index].mode == OM_NEITHER || 
		MCprocesses[t_index].mode == OM_READ)
	{
		ctxt . SetTheResultToStaticCString("process is not open for write");
		return;
	}

    IO_handle t_stream = MCprocesses[t_index].ohandle;
	uint4 t_offset;
	Boolean haseof = False;
	IO_stat t_stat;
    t_stat = IO_NORMAL;

    if (MCprocesses[t_index].encoding != EN_BINARY)
	{
		MCStringRef t_text_data;
        if (!MCStringNormalizeLineEndings(p_data, 
                                          kMCStringLineEndingStyleLegacyNative,
                                          false, 
                                          t_text_data, 
                                          nullptr))
        {
            return;
        }
		// MW-2004-11-17: EOD should only happen when writing to processes in text-mode
		if (MCStringFirstIndexOfChar(t_text_data, '\004', 0, kMCCompareExact, t_offset))
		{
			MCAutoStringRef t_substring;
			MCStringCopySubstring(t_text_data, MCRangeMake(0, t_offset), &t_substring);
			MCValueAssign(t_text_data, *t_substring);
			haseof = True;
		}
        MCFilesExecWriteToStream(ctxt, t_stream, t_text_data, p_unit_type, MCS_file_to_string_encoding((MCFileEncodingType)MCprocesses[t_index].encoding), t_stat);
		MCValueRelease(t_text_data);
	}
	else
        MCFilesExecWriteToStream(ctxt, t_stream, p_data, p_unit_type, MCS_file_to_string_encoding((MCFileEncodingType)MCprocesses[t_index].encoding), t_stat);

    if (haseof)
	{
		MCS_close(MCprocesses[t_index].ohandle);
		MCprocesses[t_index].ohandle = NULL;
		if (MCprocesses[t_index].mode == OM_UPDATE)
			MCprocesses[t_index].mode = OM_READ;
		else
			MCprocesses[t_index].mode = OM_NEITHER;
	}
	
	if (t_stat != IO_NORMAL)
	{
		ctxt .SetTheResultToStaticCString("error writing to process");
		return;
	}

#if !defined _WIN32 && !defined _MACOSX
	if (!haseof)
		MCS_flush(t_stream);
#endif

	ctxt . SetTheResultToEmpty();
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesExecSeekInFile(MCExecContext& ctxt, MCNameRef p_file, bool is_end, bool is_by, int64_t p_at)
{
	uindex_t t_index;
	if (!IO_findfile(p_file, t_index)
        || (MCfiles[t_index].ihandle == NULL && MCfiles[t_index].ohandle == NULL))
	{
		ctxt . LegacyThrow(EE_SEEK_NOFILE);
		return;
	}

	IO_handle t_stream = MCfiles[t_index].ihandle;
	if (t_stream == NULL)
		t_stream = MCfiles[t_index].ohandle;
	IO_stat t_stat = IO_ERROR;
	
	if (is_end)
		t_stat = MCS_seek_end(t_stream, p_at);
	else if (is_by)
		t_stat = MCS_seek_cur(t_stream, p_at);
	else
		t_stat = MCS_seek_set(t_stream, p_at);

	if (t_stat != IO_NORMAL)
		ctxt . LegacyThrow(EE_SEEK_BADWHERE);
}

void MCFilesExecSeekToEofInFile(MCExecContext& ctxt, MCNameRef p_file)
{
	MCFilesExecSeekInFile(ctxt, p_file, true, false, 0);
}

void MCFilesExecSeekToEofInFileLegacy(MCExecContext& ctxt, intenum_t p_eof, MCNameRef p_file)
{
	MCFilesExecSeekInFile(ctxt, p_file, true, false, 0);
}

void MCFilesExecSeekAbsoluteInFile(MCExecContext& ctxt, int64_t p_to, MCNameRef p_file)
{
	if (p_to < 0)
		MCFilesExecSeekInFile(ctxt, p_file, true, false, p_to);
	else
		MCFilesExecSeekInFile(ctxt, p_file, false, false, p_to);
}

void MCFilesExecSeekRelativeInFile(MCExecContext& ctxt, int64_t p_by, MCNameRef p_file)
{
	MCFilesExecSeekInFile(ctxt, p_file, false, true, p_by);
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesExecCreateFolder(MCExecContext& ctxt, MCStringRef p_filename)
{
	if (!ctxt . EnsureDiskAccessIsAllowed())
		return;
	
	if (!MCS_mkdir(p_filename))
		ctxt . SetTheResultToStaticCString("can't create that directory");
	else
		ctxt . SetTheResultToEmpty();
}

void MCFilesExecCreateAlias(MCExecContext& ctxt, MCStringRef p_target_filename, MCStringRef p_alias_filename)
{
	if (!ctxt . EnsureDiskAccessIsAllowed())
		return;

	if (!MCS_createalias(p_target_filename, p_alias_filename))
		ctxt . SetTheResultToStaticCString("can't create that alias");
	else
		ctxt . SetTheResultToEmpty();
}

////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	const char *token;
	int4 which;
}
ST;

static ST signal_table[] = {
#if defined(TARGET_PLATFORM_LINUX)
                               {"abrt", SIGABRT}, {"alrm", SIGALRM}, {"bus", SIGBUS}, {"chld", SIGCHLD},
                               {"cld", SIGCLD}, {"cont", SIGCONT}, {"emt", SIGBOGUS}, {"fpe", SIGFPE},
                               {"hup", SIGHUP}, {"ill", SIGILL}, {"info", SIGBOGUS}, {"int", SIGINT},
                               {"io", SIGIO}, {"iot", SIGIOT}, {"kill", SIGKILL}, {"lwp", SIGBOGUS},
                               {"phone", SIGBOGUS}, {"pipe", SIGPIPE}, {"poll", SIGPOLL}, {"prof", SIGPROF},
                               {"pwr", SIGPWR}, {"quit", SIGQUIT}, {"segv", SIGSEGV}, {"stop", SIGSTOP},
                               {"sys", SIGSYS}, {"term", SIGTERM}, {"trap", SIGTRAP}, {"tstp", SIGTSTP},
                               {"ttin", SIGTTIN}, {"ttou", SIGTTOU}, {"urg", SIGURG}, {"usr1", SIGUSR1},
                               {"usr2", SIGUSR2}, {"vtalrm", SIGVTALRM}, {"waiting", SIGBOGUS},
                               {"winch", SIGWINCH}, {"xcpu", SIGXCPU}, {"xfsz", SIGXFSZ}
#else
                               {"xfsz", 1}
#endif
                           };

int4 MCFilesExecKillLookup(MCStringRef p_string)
{
	uint2 size = ELEMENTS(signal_table);
	while(size--)
		if (MCStringIsEqualToCString(p_string, signal_table[size].token, kMCCompareCaseless))
			return signal_table[size].which;
	return SIGTERM;
}

void MCFilesExecKillProcess(MCExecContext& ctxt, MCStringRef p_process, MCStringRef p_signal)
{
	if (!ctxt . EnsureProcessIsAllowed())
		return;

	int4 t_sig = SIGTERM;
	if (p_signal != nil)
	{
		if (ctxt . ConvertToInteger(p_signal, t_sig))
			t_sig = MCU_abs(t_sig);
		else
			t_sig = MCFilesExecKillLookup(p_signal);
	}

	uindex_t t_index;
	MCNewAutoNameRef t_process_name;
	/* UNCHECKED */ MCNameCreate(p_process, &t_process_name);
	if (IO_findprocess(*t_process_name, t_index))
	{
		if (t_sig == SIGTERM || t_sig == SIGKILL)
			MCS_closeprocess(t_index);
		uint2 t_count = SHELL_COUNT;
		MCS_kill(MCprocesses[t_index].pid, t_sig);
		if (t_sig == SIGTERM || t_sig == SIGKILL)
		{
			while (--t_count)
			{
				if (MCprocesses[t_index].pid == 0)
					break;
				if (MCscreen->wait(SHELL_INTERVAL, False, False))
				{
					ctxt . LegacyThrow(EE_SHELL_ABORT);
					return;
				}
			}
			if (!t_count)
			{
				ctxt . SetTheResultToStaticCString("process didn't die");
				MCprocesses[t_index].pid = 0;
			}
			IO_cleanprocesses();
		}
	}
	else
	{
		uint4 t_pid;
		if (MCU_stoui4(p_process, t_pid) && t_pid != 0 && t_pid != MCS_getpid())
			MCS_kill(t_pid, t_sig);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesGetUMask(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = (uinteger_t)MCS_getumask();
}
void MCFilesSetUMask(MCExecContext& ctxt, uinteger_t p_value)
{
	MCS_setumask(p_value);
}
void MCFilesGetFileType(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = MCValueRetain(MCfiletype);
}
void MCFilesSetFileType(MCExecContext& ctxt, MCStringRef p_value)
{
    MCValueRelease(MCfiletype);
	MCfiletype = MCValueRetain(p_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesGetSerialControlString(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = MCValueRetain(MCserialcontrolsettings);
}

void MCFilesSetSerialControlString(MCExecContext& ctxt, MCStringRef p_value)
{
	MCValueAssign(MCserialcontrolsettings, p_value);
}

void MCFilesGetHideConsoleWindows(MCExecContext& ctxt, bool& r_value)
{
	r_value = MChidewindows == True;
}

void MCFilesSetHideConsoleWindows(MCExecContext& ctxt, bool p_value)
{
	MChidewindows = p_value ? True : False;
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesGetShellCommand(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = MCValueRetain(MCshellcmd);
}

void MCFilesSetShellCommand(MCExecContext& ctxt, MCStringRef p_value)
{
	MCValueAssign(MCshellcmd, p_value);	
}

void MCFilesGetCurrentFolder(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (!ctxt . EnsureDiskAccessIsAllowed())
		return;

	MCS_getcurdir(r_value);
    
    if (MCStringGetLength(r_value) != 0)
		return;

	ctxt . Throw();
}

void MCFilesSetCurrentFolder(MCExecContext& ctxt, MCStringRef p_value)
{
	if (!ctxt . EnsureDiskAccessIsAllowed())
		return;
	
	if (!MCS_setcurdir(p_value))
		ctxt . SetTheResultToStaticCString("can't open directory");
}

// MW-2011-11-24: [[ Nice Folders ]] Handle fetching of the special folder types.
void MCFilesGetEngineFolder(MCExecContext& ctxt, MCStringRef& r_value)
{
	/* UNCHECKED */ MCS_getspecialfolder(MCN_engine, r_value);
	ctxt . SetTheResultToEmpty();
}

void MCFilesGetHomeFolder(MCExecContext& ctxt, MCStringRef& r_value)
{
	/* UNCHECKED */ MCS_getspecialfolder(MCN_home, r_value);
	ctxt . SetTheResultToEmpty();
}

void MCFilesGetDocumentsFolder(MCExecContext& ctxt, MCStringRef& r_value)
{
	/* UNCHECKED */ MCS_getspecialfolder(MCN_documents, r_value);
	ctxt . SetTheResultToEmpty();
}

void MCFilesGetDesktopFolder(MCExecContext& ctxt, MCStringRef& r_value)
{
	/* UNCHECKED */ MCS_getspecialfolder(MCN_desktop, r_value);
	ctxt . SetTheResultToEmpty();
}

void MCFilesGetTemporaryFolder(MCExecContext& ctxt, MCStringRef& r_value)
{
	/* UNCHECKED */ MCS_getspecialfolder(MCN_temporary, r_value);
	ctxt . SetTheResultToEmpty();
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesGetFiles(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (ctxt . EnsureDiskAccessIsAllowed())
	{
		MCAutoListRef t_list;
		if (MCS_getentries(nil, true, false, false, &t_list) && MCListCopyAsString(*t_list, r_value))
			return;

		ctxt . Throw();
	}
}

void MCFilesGetDetailedFiles(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (ctxt . EnsureDiskAccessIsAllowed())
	{
		MCAutoListRef t_list;
		if (MCS_getentries(nil, true, true, false, &t_list) && MCListCopyAsString(*t_list, r_value))
			return;

		ctxt . Throw();
	}
}

void MCFilesGetFolders(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (ctxt . EnsureDiskAccessIsAllowed())
	{
		MCAutoListRef t_list;
		if (MCS_getentries(nil, false, false, false, &t_list) && MCListCopyAsString(*t_list, r_value))
			return;

		ctxt . Throw();
	}
}

void MCFilesGetDetailedFolders(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (ctxt . EnsureDiskAccessIsAllowed())
	{
		MCAutoListRef t_list;
		if (MCS_getentries(nil, false, true, false, &t_list) && MCListCopyAsString(*t_list, r_value))
			return;

		ctxt . Throw();
	}
}
