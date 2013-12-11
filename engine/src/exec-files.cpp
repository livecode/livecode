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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "osspec.h"

#include "securemode.h"
#include "exec.h"
#include "util.h"
#include "uidc.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EVAL_METHOD(Files, Directories, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Files, Files, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Files, DiskSpace, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Files, DriverNames, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Files, Drives, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Files, OpenFiles, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Files, TempName, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Files, SpecialFolderPath, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, LongFilePath, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, ShortFilePath, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, OpenProcesses, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Files, OpenProcessesIds, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Files, ProcessId, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Files, DeleteRegistry, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, ListRegistry, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, QueryRegistry, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, QueryRegistryWithType, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Files, SetRegistry, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Files, SetRegistryWithType, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Files, CopyResourceWithNewId, 6)
MC_EXEC_DEFINE_EVAL_METHOD(Files, CopyResource, 5)
MC_EXEC_DEFINE_EVAL_METHOD(Files, DeleteResource, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Files, GetResource, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Files, GetResourcesWithType, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Files, GetResources, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, SetResource, 7)
MC_EXEC_DEFINE_EVAL_METHOD(Files, AliasReference, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, ThereIsAFile, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, ThereIsNotAFile, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, ThereIsAFolder, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, ThereIsNotAFolder, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, ThereIsAProcess, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, ThereIsNotAProcess, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Files, Shell, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Files, DeleteFile, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Files, CloseFile, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Files, CloseDriver, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Files, CloseProcess, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Files, LaunchUrl, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Files, LaunchDocument, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Files, LaunchApp, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Files, OpenFile, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Files, OpenDriver, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Files, OpenProcess, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Files, OpenElevatedProcess, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Files, Rename, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Files, ReadFromStdinFor, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Files, ReadFromStdinUntil, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Files, ReadFromFileOrDriverFor, 6)
MC_EXEC_DEFINE_EXEC_METHOD(Files, ReadFromFileOrDriverUntil, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Files, ReadFromFileOrDriverAtFor, 7)
MC_EXEC_DEFINE_EXEC_METHOD(Files, ReadFromFileOrDriverAtUntil, 6)
MC_EXEC_DEFINE_EXEC_METHOD(Files, ReadFromFileOrDriverAtEndFor, 7)
MC_EXEC_DEFINE_EXEC_METHOD(Files, ReadFromFileOrDriverAtEndUntil, 6)
MC_EXEC_DEFINE_EXEC_METHOD(Files, ReadFromFileOrDriverAtEndForLegacy, 7)
MC_EXEC_DEFINE_EXEC_METHOD(Files, ReadFromFileOrDriverAtEndUntilLegacy, 6)
MC_EXEC_DEFINE_EXEC_METHOD(Files, ReadFromProcessFor, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Files, ReadFromProcessUntil, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Files, WriteToStdout, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Files, WriteToStderr, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Files, WriteToFileOrDriver, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Files, WriteToFileOrDriverAt, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Files, WriteToFileOrDriverAtEnd, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Files, WriteToFileOrDriverAtEndLegacy, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Files, WriteToProcess, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Files, SeekToEofInFile, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Files, SeekToEofInFileLegacy, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Files, SeekAbsoluteInFile, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Files, SeekRelativeInFile, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Files, CreateFolder, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Files, CreateAlias, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Files, KillProcess, 2)
MC_EXEC_DEFINE_GET_METHOD(Files, UMask, 1)
MC_EXEC_DEFINE_SET_METHOD(Files, UMask, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, FileType, 1)
MC_EXEC_DEFINE_SET_METHOD(Files, FileType, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, SerialControlString, 1)
MC_EXEC_DEFINE_SET_METHOD(Files, SerialControlString, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, HideConsoleWindows, 1)
MC_EXEC_DEFINE_SET_METHOD(Files, HideConsoleWindows, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, ShellCommand, 1)
MC_EXEC_DEFINE_SET_METHOD(Files, ShellCommand, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, CurrentFolder, 1)
MC_EXEC_DEFINE_SET_METHOD(Files, CurrentFolder, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, EngineFolder, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, HomeFolder, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, DocumentsFolder, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, DesktopFolder, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, TemporaryFolder, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, Files, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, DetailedFiles, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, Folders, 1)
MC_EXEC_DEFINE_GET_METHOD(Files, DetailedFolders, 1)

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

void MCFilesEvalDirectories(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt . LegacyThrow(EE_DISK_NOPERM);
		return;
	}
	MCAutoListRef t_list;
	if (MCS_getentries(false, false, &t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt . Throw();
}

void MCFilesEvalFiles(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt . LegacyThrow(EE_DISK_NOPERM);
		return;
	}
	MCAutoListRef t_list;
	if (MCS_getentries(true, false, &t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt . Throw();
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

    MCNewAutoNameRef t_path;
    MCNameCreate(p_folder, &t_path);
	if (MCS_getspecialfolder(*t_path, r_path))
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

	if (MCS_longfilepath(p_path, r_long_path))
	{
		if (MCStringGetLength(r_long_path) == 0)
			ctxt.SetTheResultToCString("can't get");
		else
			ctxt.SetTheResultToEmpty();

		return;
	}

	ctxt.Throw();
}

void MCFilesEvalShortFilePath(MCExecContext& ctxt, MCStringRef p_path, MCStringRef& r_short_path)
{
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		ctxt.LegacyThrow(EE_DISK_NOPERM);
		return;
	}

	if (MCS_shortfilepath(p_path, r_short_path))
	{
		if (MCStringGetLength(r_short_path) == 0)
			ctxt.SetTheResultToCString("can't get");
		else
			ctxt.SetTheResultToEmpty();

		return;
	}

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

bool MCFilesOpenProcessesNamesList(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

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
		/* UNCHECKED */ ctxt.ConvertToData(p_value, (MCDataRef&)&t_converted);
	}
	else
	{
		/* UNCHECKED */ ctxt.ConvertToString(p_value, (MCStringRef&)&t_converted);
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
	
	ctxt.Throw();
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
		ctxt . LegacyThrow(EE_SHELL_NOPERM);
		return;
	}

	if (MCS_runcmd(p_command, r_output) != IO_NORMAL)
	{
		ctxt . LegacyThrow(EE_SHELL_BADCOMMAND);
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

	MCStringRef t_new_url;
	t_new_url = MCValueRetain(*t_mutable_string);

	if (ctxt . EnsureProcessIsAllowed())
	{
		MCS_launch_url(t_new_url);
		MCValueRelease(t_new_url);
		return;
	}

	MCValueRelease(t_new_url);
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

void MCFilesExecPerformOpen(MCExecContext& ctxt, MCNameRef p_name, int p_mode, bool p_text_mode, bool p_is_driver)
{
	if (!ctxt . EnsureDiskAccessIsAllowed())
		return;

	// REVIEW AND CHANGE : open file should probably not close the file
	//   first - this should be down to user script.
	IO_closefile(p_name);

	IO_handle istream = NULL;
	IO_handle ostream = NULL;
	
	switch (p_mode)
	{
	case OM_APPEND:
		ostream = MCS_open(MCNameGetString(p_name), kMCSOpenFileModeAppend, False, p_is_driver, 0);
		break;
	case OM_NEITHER:
		break;
	case OM_READ:
		istream = MCS_open(MCNameGetString(p_name), kMCSOpenFileModeRead, True, p_is_driver, 0);
		break;
	case OM_WRITE:
		ostream = MCS_open(MCNameGetString(p_name), kMCSOpenFileModeWrite, False, p_is_driver, 0);
		break;
	case OM_UPDATE:
		istream = ostream = MCS_open(MCNameGetString(p_name), kMCSOpenFileModeUpdate, False, p_is_driver, 0);
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
	MCfiles[MCnfiles].textmode = p_text_mode ? True : False;
	MCfiles[MCnfiles].ihandle = istream;
	MCfiles[MCnfiles++].ohandle = ostream;
}

void MCFilesExecOpenFile(MCExecContext& ctxt, MCNameRef p_filename, int p_mode, bool p_text_mode)
{
	MCFilesExecPerformOpen(ctxt, p_filename, p_mode, p_text_mode, false);
}

void MCFilesExecOpenDriver(MCExecContext& ctxt, MCNameRef p_device, int p_mode, bool p_text_mode)
{
	MCFilesExecPerformOpen(ctxt, p_device, p_mode, p_text_mode, true);
}

void MCFilesExecPerformOpenProcess(MCExecContext& ctxt, MCNameRef p_process, int p_mode, bool p_text_mode, bool p_elevated)
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
		MCprocesses[index].textmode = p_text_mode ? True : False;
}

void MCFilesExecOpenProcess(MCExecContext& ctxt, MCNameRef p_process, int p_mode, bool p_text_mode)
{
	MCFilesExecPerformOpenProcess(ctxt, p_process, p_mode, p_text_mode, false);
}

void MCFilesExecOpenElevatedProcess(MCExecContext& ctxt, MCNameRef p_process, int p_mode, bool p_text_mode)
{
	MCFilesExecPerformOpenProcess(ctxt, p_process, p_mode, p_text_mode, true);
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

void MCFilesExecPerformReadFor(MCExecContext& ctxt, IO_handle p_stream, int4 p_index, int p_unit_type, uint4 p_count, double p_max_wait, int p_time_units, MCStringRef &r_output, IO_stat &r_stat)
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
	default:
		size = p_count;
		break;
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

	MCStringRef t_buffer;
	MCStringCreateMutable(0, t_buffer);
	uindex_t t_num_chars;
	
	switch (p_unit_type) 
	{
	case FU_INT1:
		{
			char buffer[I1L];
			int1 *i1ptr = (int1 *)t_current.Chars();
			for (uint4 i = 0 ; i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", i1ptr[i]);
				if (i != 0)
					MCStringAppendNativeChar(t_buffer, ',');
				MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_INT2:
		{
			char buffer[I2L];
			int2 *i2ptr = (int2 *)t_current.Chars();
			for (uint4 i = 0 ; i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", i2ptr[i]);
				if (i != 0)
					MCStringAppendNativeChar(t_buffer, ',');
				MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_INT4:
		{
			char buffer[I4L];
			int4 *i4ptr = (int4 *)t_current.Chars();
			for (uint4 i = 0 ; i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", i4ptr[i]);
				if (i != 0)
					MCStringAppendNativeChar(t_buffer, ',');
				MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_INT8:
		{
			char buffer[I4L];
			int4 *i8ptr = (int4 *)t_current.Chars();
			for (uint4 i = 0 ; i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", i8ptr[i]);
				if (i != 0)
					MCStringAppendNativeChar(t_buffer, ',');
				MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_REAL4:
		{
			char buffer[R4L];
			real4 *r4ptr = (real4 *)t_current.Chars();
			for (uint4 i = 0 ; i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%f", r4ptr[i]);
				if (i != 0)
					MCStringAppendNativeChar(t_buffer, ',');
				MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_REAL8:
		{
			char buffer[R8L];
			real8 *r8ptr = (real8 *)t_current.Chars();
			for (uint4 i = 0 ; i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%lf", r8ptr[i]);
				if (i != 0)
					MCStringAppendNativeChar(t_buffer, ',');
				MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_UINT1:
		{
			char buffer[U1L];
			uint1 *u1ptr = (uint1 *)t_current.Chars();
			for (uint4 i = 0 ; i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", u1ptr[i]);
				if (i != 0)
					MCStringAppendNativeChar(t_buffer, ',');
				MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_UINT2:
		{
			char buffer[U2L];
			uint2 *u2ptr = (uint2 *)t_current.Chars();
			for (uint4 i = 0 ; i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", u2ptr[i]);
				if (i != 0)
					MCStringAppendNativeChar(t_buffer, ',');
				MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_UINT4:
		{
			char buffer[U4L];
			uint4 *u4ptr = (uint4 *)t_current.Chars();
			for (uint4 i = 0 ; i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", u4ptr[i]);
				if (i != 0)
					MCStringAppendNativeChar(t_buffer, ',');
				MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break;
	case FU_UINT8:
		{
			char buffer[U8L];
			uint4 *u8ptr = (uint4 *)t_current.Chars();
			for (uint4 i = 0 ; i < p_count ; i++)
			{
				t_num_chars = sprintf(buffer, "%d", u8ptr[i]);
				if (i != 0)
					MCStringAppendNativeChar(t_buffer, ',');
				MCStringAppendNativeChars(t_buffer, (char_t *)buffer, t_num_chars);
			}
		}
		break; 
	default:
		t_current.Shrink(size);
		/* UNCHECKED */ t_current.CreateStringAndRelease(r_output);
		return;
	}
	/* UNCHECKED */ MCStringCopyAndRelease(t_buffer, r_output);
}

void MCFilesExecPerformReadUntil(MCExecContext& ctxt, IO_handle p_stream, int4 p_index, uint4 p_count, const MCStringRef p_sentinel, Boolean words, double p_max_wait, int p_time_units, MCStringRef &r_output, IO_stat &r_stat)
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
	if (MCStringGetNativeCharAtIndex(p_sentinel, 0) == '\004')
		fullsize = BUFSIZ;
	else
		fullsize = 1;
	uint4 endp_count = MCStringGetLength(p_sentinel) - 1;

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
		r_stat = MCS_readall(t_buffer.Chars() + size, rsize, p_stream, rsize);
		size += rsize;
		if (rsize < fullsize)
		{
			// MW-2010-10-25: [[ Bug 4022 ]] If we are reading from a process and we didn't
			//   get as much data as we wanted then do a sweep.
			if (p_index != -1)
				MCS_checkprocesses();

			if (MCStringIsEmpty(p_sentinel) || ((r_stat == IO_ERROR || r_stat == IO_EOF) && (p_index == -1 || MCprocesses[p_index].pid == 0)))
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
				if (MCStringGetNativeCharAtIndex(p_sentinel, 0) && MCStringGetNativeCharAtIndex(p_sentinel, 0) != '\004')
				{
					uint4 i = endp_count;
					uint4 j = size - 1;
					while (i && j && t_buffer.Chars()[j] == MCStringGetNativeCharAtIndex(p_sentinel, i))
					{
						i--;
						j--;
					}
					if (i == 0 && (t_buffer.Chars()[j] == MCStringGetNativeCharAtIndex(p_sentinel, 0)
					               || (MCStringGetNativeCharAtIndex(p_sentinel, 0) == '\n' && t_buffer.Chars()[j] == '\r')))
					{
						// MW-2008-08-15: [[ Bug 6580 ]] This clause looks ahead for CR LF sequences
						//   if we have just enp_countered CR. However, it was previousy using MCS_seek_cur
						//   to retreat, which *doesn't* work for process p_streams.
						if (MCStringGetNativeCharAtIndex(p_sentinel, 0) == '\n' && endp_count == 0
						        && t_buffer.Chars()[j] == '\r')
						{
							uint1 term;
							uint4 nread = 1;
							if (MCS_readall(&term, nread, p_stream, nread) == IO_NORMAL)
                            {
								if (term != '\n')
									MCS_putback(term, p_stream);
								else
									t_buffer.Chars()[j] = '\n';
                            }
						}
						if (--p_count == 0)
							break;
					}
				}
			}
		}
	}
	t_buffer.Shrink(size);
	/* UNCHECKED */ t_buffer.CreateStringAndRelease(r_output);
}

void MCFilesExecPerformReadUntilBinary(MCExecContext& ctxt, IO_handle stream, int4 p_index, uint4 p_count, const MCStringRef p_sentinel, Boolean words, double p_max_wait, int p_time_units, MCStringRef &r_output, IO_stat &r_stat)
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
					while (i && j && t_buffer.Chars()[j] == MCStringGetNativeCharAtIndex(p_sentinel, 0))
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
	/* UNCHECKED */ t_buffer.CreateStringAndRelease(r_output);
}

void MCFilesExecReadComplete(MCExecContext& ctxt, MCStringRef p_output, IO_stat p_stat, Boolean t_textmode)
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
	if (t_textmode)
	{
		MCAutoStringRef t_output;
		/* UNCHECKED*/ MCStringConvertLineEndingsToLiveCode(p_output, &t_output);
		MCValueAssign(p_output, *t_output);
	}
	
	ctxt . SetItToValue(p_output);
}

void MCFilesExecReadUntil(MCExecContext& ctxt, IO_handle p_stream, index_t p_index, MCStringRef p_sentinel, double p_max_wait, int p_time_units, bool p_is_text, MCStringRef &r_output, IO_stat &r_stat)
{
	MCAutoStringRef t_sentinel;
	if (p_sentinel != nil)
		MCStringCopy(p_sentinel, &t_sentinel);
	else
		/* UNCHECKED */ MCStringCreateWithCString("end", &t_sentinel);

	// MW-2009-11-03: [[ Bug 8402 ]] Use a different stream array, depending on what
	//   type of stream we are reading from.
	if (!p_is_text)
		MCFilesExecPerformReadUntilBinary(ctxt, p_stream, p_index, 1, *t_sentinel, False, p_max_wait, p_time_units, r_output, r_stat);
	else
		MCFilesExecPerformReadUntil(ctxt, p_stream, p_index, 1, *t_sentinel, False, p_max_wait, p_time_units, r_output, r_stat);
}

void MCFilesExecReadFor(MCExecContext& ctxt, IO_handle p_stream, index_t p_index, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units, MCStringRef &r_output, IO_stat &r_stat) 
{
	MCAutoStringRef t_sentinel;

	switch (p_unit_type)
	{
	case FU_LINE:
		MCStringCreateWithCString("\n", &t_sentinel);
		MCFilesExecPerformReadUntil(ctxt, p_stream, p_index, p_count, *t_sentinel, False, p_max_wait, p_time_units, r_output, r_stat);
		break;
	case FU_ITEM:
		MCStringCreateWithCString(",", &t_sentinel);
		MCFilesExecPerformReadUntil(ctxt, p_stream, p_index, p_count, *t_sentinel, False, p_max_wait, p_time_units, r_output, r_stat);
		break;
	case FU_WORD:
		MCStringCreateWithCString(" ", &t_sentinel);
		MCFilesExecPerformReadUntil(ctxt, p_stream, p_index, p_count, *t_sentinel, True, p_max_wait, p_time_units, r_output, r_stat);
		break;
	default:
		MCFilesExecPerformReadFor(ctxt, p_stream, p_index, p_unit_type, p_count, p_max_wait, p_time_units, r_output, r_stat);
		break;
	}
}

void MCFilesExecReadFromStdin(MCExecContext& ctxt, MCStringRef p_sentinel, uint4 p_count, int p_unit_type, uint2 p_repeat_form)
{
	IO_stat t_stat = IO_NORMAL;
	MCAutoStringRef t_output;

#ifndef _SERVER
	if (!MCnoui && MCS_isinteractiveconsole(0))
	{
		ctxt . SetTheResultToStaticCString("eof");
		ctxt . SetItToValue(kMCNull);
		return;
	}
	else
#endif

	switch (p_repeat_form)
	{
	case RF_FOR:
		MCFilesExecReadFor(ctxt, IO_stdin, -1, p_count, p_unit_type, MAXUINT4, 0, &t_output, t_stat);
		break;
	case RF_UNTIL:
		MCFilesExecReadUntil(ctxt, IO_stdin, -1, p_sentinel, MAXUINT4, 0, true, &t_output, t_stat);
		break;
	default:
		break;
	}
	MCFilesExecReadComplete(ctxt, *t_output, t_stat, True);
}

void MCFilesExecReadFromStdinFor(MCExecContext& ctxt, uint4 p_count, int p_unit_type)
{
	MCFilesExecReadFromStdin(ctxt, nil, p_count, p_unit_type, RF_FOR);
}

void MCFilesExecReadFromStdinUntil(MCExecContext& ctxt, MCStringRef p_sentinel)
{
	MCFilesExecReadFromStdin(ctxt, p_sentinel, 0, 0, RF_UNTIL);
}

void MCFilesExecReadGetStream(MCExecContext& ctxt, MCNameRef p_name, bool p_is_end, int64_t p_at, IO_handle &r_stream, Boolean &r_textmode, IO_stat &r_stat)
{
	uindex_t t_index;
	if (!IO_findfile(p_name, t_index) || MCfiles[t_index].mode == OM_APPEND
	        || MCfiles[t_index].mode == OM_WRITE)
	{
		ctxt . SetTheResultToStaticCString("file is not open for read");
		return;
	}
	r_stream = MCfiles[t_index].ihandle;
	r_textmode = MCfiles[t_index].textmode;

	if (p_at == nil)
	{
#ifdef OLD_IO_HANDLE
		if (r_stream->flags & IO_WRITTEN)
		{
			r_stream->flags &= ~IO_WRITTEN;
			MCS_sync(r_stream);
		}
#endif
	}
	else
	{
		if (p_is_end)
			r_stat = MCS_seek_end(r_stream, p_at);
		else
			r_stat = MCS_seek_set(r_stream, p_at);
	}
}

void MCFilesExecReadFromFileOrDriverFor(MCExecContext& ctxt, bool p_driver, bool p_is_end, MCNameRef p_file, int64_t p_at, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units)
{
	IO_handle t_stream = NULL;
	Boolean t_textmode = False;
	IO_stat t_stat = IO_NORMAL;
	
	MCFilesExecReadGetStream(ctxt, p_file, p_is_end, p_at, t_stream, t_textmode, t_stat);	
	
	if (t_stream == NULL)
		return;

	if (t_stat != IO_NORMAL)
	{	
		ctxt . SetTheResultToStaticCString("error seeking in file");
		return;
	}

	MCAutoStringRef t_output;

	MCFilesExecReadFor(ctxt, t_stream, -1, p_count, p_unit_type, p_max_wait, p_time_units, &t_output, t_stat);
 	MCFilesExecReadComplete(ctxt, *t_output, t_stat, t_textmode);

#if !defined _WIN32 && !defined _MACOSX
	MCS_sync(t_stream);
#endif
}

void MCFilesExecReadFromFileOrDriverUntil(MCExecContext& ctxt, bool p_driver, bool p_is_end, MCNameRef p_file, MCStringRef p_sentinel, int64_t p_at, double p_max_wait, int p_time_units)
{
	IO_handle t_stream = NULL;
	Boolean t_textmode = False;
	IO_stat t_stat = IO_NORMAL;
	
	MCFilesExecReadGetStream(ctxt, p_file, p_is_end, p_at, t_stream, t_textmode, t_stat);		
	
	if (t_stream == NULL)
		return;

	if (t_stat != IO_NORMAL)
	{	
		ctxt . SetTheResultToStaticCString("error seeking in file");
		return;
	}
	
	MCAutoStringRef t_output;
	bool t_is_text = true;

	if (MCStringGetLength(p_sentinel) == 1 && MCStringGetNativeCharAtIndex(p_sentinel, 0) == '\004')
	{
		MCAutoDataRef t_data;
		IO_read_to_eof(t_stream, &t_data);
		ctxt . SetTheResultToStaticCString("eof");
	}
		
	if (!p_driver)
		t_is_text = t_textmode != 0;
		
	MCFilesExecReadUntil(ctxt, t_stream, -1, p_sentinel, p_max_wait, p_time_units, t_is_text, &t_output, t_stat);
	MCFilesExecReadComplete(ctxt, *t_output, t_stat, t_textmode);

#if !defined _WIN32 && !defined _MACOSX
	MCS_sync(t_stream);
#endif
}

void MCFilesExecReadFromFileOrDriverFor(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverFor(ctxt, p_driver, false, p_file, nil, p_count, p_unit_type, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverUntil(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, MCStringRef p_sentinel, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverUntil(ctxt, p_driver, false, p_file, p_sentinel, nil, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverAtFor(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, int64_t p_at, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverFor(ctxt, p_driver, false, p_file, p_at, p_count, p_unit_type, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverAtUntil(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, int64_t p_at, MCStringRef p_sentinel, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverUntil(ctxt, p_driver, false, p_file, p_sentinel, p_at, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverAtEndFor(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, int64_t p_at, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverFor(ctxt, p_driver, true, p_file, p_at, p_count, p_unit_type, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverAtEndForLegacy(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, intenum_t p_eof, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverFor(ctxt, p_driver, true, p_file, 0, p_count, p_unit_type, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverAtEndUntil(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, int64_t p_at, MCStringRef p_sentinel, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverUntil(ctxt, p_driver, true, p_file, p_sentinel, p_at, p_max_wait, p_time_units);
}

void MCFilesExecReadFromFileOrDriverAtEndUntilLegacy(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, intenum_t p_eof, MCStringRef p_sentinel, double p_max_wait, int p_time_units)
{
	MCFilesExecReadFromFileOrDriverUntil(ctxt, p_driver, true, p_file, p_sentinel, 0, p_max_wait, p_time_units);
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
	Boolean t_textmode = False;
	IO_stat t_stat = IO_NORMAL;
	t_stream = MCprocesses[t_index].ihandle;
#ifdef OLD_IO_HANDLE
	MCshellfd = t_stream->gefd();
#endif // OLD_IO_HANDLE
	t_textmode = MCprocesses[t_index].textmode;
	MCAutoStringRef t_output;

	switch (p_repeat_form)
	{
	case RF_FOR:
		MCFilesExecReadFor(ctxt, t_stream, t_index, p_count, p_unit_type, p_max_wait, p_time_units, &t_output, t_stat);
		break;
	case RF_UNTIL:
	{
		bool t_is_text = MCprocesses[t_index] . textmode != 0;
		MCFilesExecReadUntil(ctxt, t_stream, t_index, p_sentinel, p_max_wait, p_time_units, t_is_text, &t_output, t_stat);
		break;
	}
	default:  
		break;
	}
	MCFilesExecReadComplete(ctxt, *t_output, t_stat, t_textmode);
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

void MCFilesExecWriteToStream(MCExecContext& ctxt, IO_handle p_stream, MCStringRef p_data, int p_unit_type, IO_stat &r_stat)
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
			MCAutoStringRefAsCString t_output;
			/* UNCHECKED */ t_output . Lock(p_data);
			r_stat = MCS_write(*t_output, sizeof(char), strlen(*t_output), p_stream);
		}
		break;
	default:
		{
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
				/* UNCHECKED */ MCStringCopySubstring(p_data, MCRangeMake(t_start_pos, t_data_pos - t_start_pos), &s); 
				real8 n;
				if (!MCU_stor8(*s, n))
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

void MCFilesExecWriteGetStream(MCExecContext& ctxt, MCNameRef p_name, bool p_is_end, int64_t p_at, IO_handle &r_stream, Boolean &r_textmode, IO_stat &r_stat)
{
	uindex_t t_index;
	if (!IO_findfile(p_name, t_index) || MCfiles[t_index].mode == OM_NEITHER || 
		MCfiles[t_index].mode == OM_READ)
	{
		ctxt . SetTheResultToStaticCString("file is not open for write");
		return;
	}
	r_stream = MCfiles[t_index].ohandle;
	r_textmode = MCfiles[t_index].textmode;

	if (p_at != nil)
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
	MCFilesExecWriteToStream(ctxt, IO_stdout, p_data, p_unit_type, t_stat);

	if (t_stat != IO_NORMAL)
		ctxt . SetTheResultToStaticCString("error writing file");
	else
		ctxt . SetTheResultToEmpty();
}
void MCFilesExecWriteToStderr(MCExecContext& ctxt, MCStringRef p_data, int p_unit_type)
{
	IO_stat t_stat;
	MCFilesExecWriteToStream(ctxt, IO_stderr, p_data, p_unit_type, t_stat);

	if (t_stat != IO_NORMAL)
		ctxt . SetTheResultToStaticCString("error writing file");
	else
		ctxt . SetTheResultToEmpty();
}


void MCFilesExecWriteToFileOrDriver(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, bool p_is_end, int p_unit_type, int64_t p_at)
{
	
	IO_handle t_stream = NULL;
	Boolean t_textmode = False;
	IO_stat t_stat = IO_NORMAL;
	
	MCFilesExecWriteGetStream(ctxt, p_file, p_is_end, p_at, t_stream, t_textmode, t_stat);		
	
	if (t_stream == NULL)
		return;

	if (t_stat != IO_NORMAL)
	{
		ctxt . SetTheResultToStaticCString("error seeking in file");
		return;
	}

	if (t_textmode)
	{
		MCAutoStringRef t_text_data;
		/* UNCHECKED */ MCStringConvertLineEndingsFromLiveCode(p_data, &t_text_data);
		MCFilesExecWriteToStream(ctxt, t_stream, *t_text_data, p_unit_type, t_stat);
	}
	else
		MCFilesExecWriteToStream(ctxt, t_stream, p_data, p_unit_type, t_stat);

	if (t_stat != IO_NORMAL)
	{
		ctxt . SetTheResultToStaticCString("error writing file");
		return;
	}
	ctxt . SetTheResultToEmpty();
#ifdef OLD_IO_HANDLE
	t_stream->flags |= IO_WRITTEN;
#endif

#if !defined _WIN32 && !defined _MACOSX
	MCS_flush(t_stream);
#endif
	
}

void MCFilesExecWriteToFileOrDriver(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, int p_unit_type)
{
	MCFilesExecWriteToFileOrDriver(ctxt, p_file, p_data, false, p_unit_type, nil);
}

void MCFilesExecWriteToFileOrDriverAt(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, int p_unit_type, int64_t p_at)
{
	MCFilesExecWriteToFileOrDriver(ctxt, p_file, p_data, false, p_unit_type, p_at);
}

void MCFilesExecWriteToFileOrDriverAtEnd(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, int p_unit_type)
{
	MCFilesExecWriteToFileOrDriver(ctxt, p_file, p_data, true, p_unit_type, nil);
}

void MCFilesExecWriteToFileOrDriverAtEndLegacy(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, int p_unit_type, intenum_t p_eof)
{
	MCFilesExecWriteToFileOrDriver(ctxt, p_file, p_data, true, p_unit_type, nil);
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
	Boolean t_textmode = MCprocesses[t_index].textmode;
	uint4 t_offset;
	Boolean haseof = False;
	IO_stat t_stat;
    t_stat = IO_NORMAL;

	if (t_textmode)
	{
		MCStringRef t_text_data;
		/* UNCHECKED */ MCStringConvertLineEndingsFromLiveCode(p_data, t_text_data);
		// MW-2004-11-17: EOD should only happen when writing to processes in text-mode
		if (MCStringFirstIndexOfChar(t_text_data, '\004', 0, kMCCompareExact, t_offset))
		{
			MCAutoStringRef t_substring;
			MCStringCopySubstring(t_text_data, MCRangeMake(0, t_offset), &t_substring);
			MCValueAssign(t_text_data, *t_substring);
			haseof = True;
		}
		MCFilesExecWriteToStream(ctxt, t_stream, t_text_data, p_unit_type, t_stat);
		MCValueRelease(t_text_data);
	}
	else
		MCFilesExecWriteToStream(ctxt, t_stream, p_data, p_unit_type, t_stat);

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

#ifdef OLD_IO_HANDLE
	t_stream->flags |= IO_SEEKED;
#endif

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
}

void MCFilesGetHomeFolder(MCExecContext& ctxt, MCStringRef& r_value)
{
	/* UNCHECKED */ MCS_getspecialfolder(MCN_home, r_value);
}

void MCFilesGetDocumentsFolder(MCExecContext& ctxt, MCStringRef& r_value)
{
	/* UNCHECKED */ MCS_getspecialfolder(MCN_documents, r_value);
}

void MCFilesGetDesktopFolder(MCExecContext& ctxt, MCStringRef& r_value)
{
	/* UNCHECKED */ MCS_getspecialfolder(MCN_desktop, r_value);
}

void MCFilesGetTemporaryFolder(MCExecContext& ctxt, MCStringRef& r_value)
{
	/* UNCHECKED */ MCS_getspecialfolder(MCN_temporary, r_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCFilesGetFiles(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (ctxt . EnsureDiskAccessIsAllowed())
	{
		MCAutoListRef t_list;
		if (MCS_getentries(true, false, &t_list) && MCListCopyAsString(*t_list, r_value))
			return;

		ctxt . Throw();
	}
}

void MCFilesGetDetailedFiles(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (ctxt . EnsureDiskAccessIsAllowed())
	{
		MCAutoListRef t_list;
		if (MCS_getentries(true, true, &t_list) && MCListCopyAsString(*t_list, r_value))
			return;

		ctxt . Throw();
	}
}

void MCFilesGetFolders(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (ctxt . EnsureDiskAccessIsAllowed())
	{
		MCAutoListRef t_list;
		if (MCS_getentries(false, false, &t_list) && MCListCopyAsString(*t_list, r_value))
			return;

		ctxt . Throw();
	}
}

void MCFilesGetDetailedFolders(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (ctxt . EnsureDiskAccessIsAllowed())
	{
		MCAutoListRef t_list;
		if (MCS_getentries(false, true, &t_list) && MCListCopyAsString(*t_list, r_value))
			return;

		ctxt . Throw();
	}
}
