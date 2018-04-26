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

#ifndef __MC_SERVER_SCRIPT__
#define __MC_SERVER_SCRIPT__

#ifndef __MC_STACK__
#include "stack.h"
#endif

#ifndef __MC_EXTERNAL__
#include "external.h"
#endif

class MCStatement;

class MCServerScript: public MCStack
{
public:
	MCServerScript(void);
	virtual ~MCServerScript(void);
	
	void ListFiles(MCStringRef &r_string);
	
	uint32_t GetIncludeDepth(void);
	bool Include(MCExecContext& context, MCStringRef p_filename, bool p_require);

	uint4 GetFileIndexForContext(MCExecContext &ctxt);
	
    bool GetFileForContext(MCExecContext &ctxt, MCStringRef &r_file);
	
	// Lookup the file index for the given filename. If <p_add> is true then
	// add new entry and return its index.
	uint4 FindFileIndex(MCStringRef p_filename, bool p_add);
    
    // SN-2014-09-05: [[ Bug 13378 ]] Added forgotten function GetIt
    MCVarref* GetIt();
	
private:
	// A File record stores information about an included file.
	struct File
	{
		// The list linkage (single is good enough because Files are never
		// removed from the list).
		File *next;
		
		// The absolute filename of the file it refers to.
        MCAutoStringRef filename;
		
		// The buffer containing the file's contents. This should be treated
		// as read-only as it could be mmapped. We need to keep this around
		// because the engine keeps direct pointers to substrings of it.
		char *script;
		
		// An (instance) unique index used to map handlers to files (via
		// MCHandler::fileindex)
		uint4 index;
		
		// The underlying system file-handle for the file - this will be nil
		// if we had to load the entire file, non-nil if mmapped.
		MCSystemFileHandle *handle;
	};
	
	// Locate the given file in the list of files, adding it if not present and
	// 'add' is true.
	File *FindFile(MCStringRef p_filename, bool p_add);

	// Return the next statement in the script point, processing any definitions
	// that occur before it.
	Parse_stat ParseNextStatement(MCScriptPoint& sp, MCStatement*& r_statement);

	// The linked list of files that have been included
	File *m_files;
	
	// The file currently being executed.
	File *m_current_file;

	// The current include depth.
	uint32_t m_include_depth;

	// The execpoint in which global code is executed.
	MCExecContext *m_ctxt;
	
	// MW-2013-11-08: [[ RefactorIt ]] The 'it' var at global scope.
	MCVarref *m_it;
};

#endif
