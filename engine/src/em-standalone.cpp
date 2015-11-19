/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

#include <foundation.h>

#include "em-standalone.h"

#include "globdefs.h"
#include "parsedef.h"
#include "scriptpt.h"
#include "globals.h"
#include "variable.h"
#include "mcio.h"
#include "osspec.h"
#include "minizip.h"

/* ----------------------------------------------------------------
 * Functions implemented in em-standalone.js
 * ---------------------------------------------------------------- */

extern "C" int MCEmscriptenStandaloneGetDataJS(void **r_buffer, int *r_length);

/* ---------------------------------------------------------------- */

static bool
__MCEmscriptenStandaloneUnpackWrite(void *context,
                                    const void *p_data,
                                    uint32_t p_data_length,
                                    uint32_t p_data_offset,
                                    uint32_t p_data_total)
{
	IO_handle t_handle = static_cast<IO_handle>(context);
	const char *t_buffer = static_cast<const char *>(p_data);

	/* Write the extracted data */
	return (IO_NORMAL == MCS_write(t_buffer + p_data_offset,
	                               p_data_length, 1, t_handle));
}

static bool
__MCEmscriptenStandaloneUnpackMkdirs(MCStringRef p_name)
{
	/* If p_name doesn't contain a "/", do nothing. If p_name doesn't end
	 * with "/", truncate to the last "/", and recurse. */
	uindex_t t_sep_index;
	if (!MCStringLastIndexOfChar(p_name, '/', UINDEX_MAX,
	                             kMCStringOptionCompareExact, t_sep_index))
	{
		return true; /* No directory part */
	}
	if (t_sep_index != MCStringGetLength(p_name) - 1)
	{
		/* Doesn't end with "/" */
		MCAutoStringRef t_dirname;
		if (!MCStringCopySubstring (p_name, MCRangeMake(0, t_sep_index + 1),
		                            &t_dirname))
		{
			return false;
		}

		return __MCEmscriptenStandaloneUnpackMkdirs(*t_dirname);
	}

	/* If the directory already exists, we're done! */
	if (MCS_exists(p_name, false))
	{
		return true;
	}

	/* Make sure the parent directories have been created.  If
	 * possible, truncate the path by one element and recurse. */
	if (MCStringLastIndexOfChar(p_name, '/', t_sep_index,
	                            kMCStringOptionCompareExact, t_sep_index))
	{
		MCAutoStringRef t_dirname;
		if (!MCStringCopySubstring (p_name, MCRangeMake(0, t_sep_index + 1),
		                            &t_dirname))
		{
			return false;
		}

		if (!__MCEmscriptenStandaloneUnpackMkdirs(*t_dirname))
		{
			return false;
		}
	}

	/* Create the directory */
	MCLog("zip: %@", p_name);
	if (!MCS_mkdir(p_name) && !MCS_exists(p_name, false))
	{
		return false;
	}

	return true;
}

static bool
__MCEmscriptenStandaloneUnpackExtract(void *context,
                                      MCStringRef p_name)
{
	bool t_success = true;

	MCMiniZipRef t_zip = static_cast<MCMiniZipRef>(context);

	/* Create the directory for the entry */
	if (!__MCEmscriptenStandaloneUnpackMkdirs(p_name))
	{
		return false;
	}

	/* If the zip file item is a directory, we're done */
	if (MCStringEndsWithCString(p_name, (const char_t *) "/",
	                            kMCStringOptionCompareExact))
	{
		return true;
	}
	else
	{
		MCLog("zip: %@", p_name);

		/* Otherwise, extract it to file */
		IO_handle t_handle = NULL;
		if (t_success)
		{
			t_handle = MCS_open(p_name, kMCOpenFileModeWrite, false, false, 0);
			if (t_handle == NULL)
			{
				t_success = 0;
			}
		}

		if (t_success)
		{
			t_success = MCMiniZipExtractItem(t_zip, p_name,
			                                 __MCEmscriptenStandaloneUnpackWrite,
			                                 t_handle);
		}

		if (t_handle != NULL)
		{
			MCS_close(t_handle);
		}
	}

	return t_success;
}

bool
MCEmscriptenStandaloneUnpack()
{
	/* Note that because this function is called before X_open(), it's
	 * not possible to store diagnostic information in MCresult on
	 * failure. */
	bool t_success = true;

	MCLog("Unpacking standalone...");

	/* Fetch downloaded standalone data */
	void *t_buffer = NULL;
	int t_buffer_len = -1;

	if (t_success)
	{
		if (!MCEmscriptenStandaloneGetDataJS(&t_buffer, &t_buffer_len))
		{
			MCLog("failed to download standalone data");
			t_success = false;
		}
	}

	/* Unpack the VFS image to the filesystem root */
	MCMiniZipRef t_zip = NULL;

	if (t_success)
	{
		t_success = MCS_setcurdir(MCSTR("/"));
	}

	if (t_success)
	{
		MCAssert(0 < t_buffer_len);
		MCAssert(NULL != t_buffer);
		if (!MCMiniZipOpen(t_buffer, t_buffer_len, t_zip))
		{
			MCLog("failed to open standalone data as zip archive");
			t_success = false;
		}
	}

	if (t_success)
	{
		if (!MCMiniZipListItems(t_zip,
		                        __MCEmscriptenStandaloneUnpackExtract,
		                        t_zip))
		{
			MCLog("failed to extract standalone files");
			t_success = false;
		}
	}

	/* ---------- 4. Cleanup */
	if (NULL != t_zip)
	{
		MCMiniZipClose(t_zip);
	}
	if (NULL != t_buffer)
	{
		free(t_buffer);
	}

	return t_success;
}
