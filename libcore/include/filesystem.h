#ifndef __MC_FILESYSTEM__
#define __MC_FILESYSTEM__

#ifndef __MC_CORE__
#include "core.h"
#endif

////////////////////////////////////////////////////////////////////////////////

enum MCFileSystemEntryType
{
	kMCFileSystemEntryFile,
	kMCFileSystemEntryFolder,
	kMCFileSystemEntryLink,
	kMCFileSystemEntryPackage
};

struct MCFileSystemEntry
{
	MCFileSystemEntryType type;
	const char *filename;
};

typedef bool (*MCFileSystemListCallback)(void *context, const MCFileSystemEntry& entry);

bool MCFileSystemListEntries(const char *folder, uint32_t options, MCFileSystemListCallback callback, void *p_context);

////////////////////////////////////////////////////////////////////////////////

bool MCFileSystemPathExists(const char *path, bool folder, bool& exists);

////////////////////////////////////////////////////////////////////////////////

bool MCFileSystemPathToNative(const char *path, void*& r_native_path);
bool MCFileSystemPathFromNative(const void *native_path, char*& r_path);

bool MCFileSystemPathResolve(const char *path, char*& r_resolved_path);

////////////////////////////////////////////////////////////////////////////////

#endif
