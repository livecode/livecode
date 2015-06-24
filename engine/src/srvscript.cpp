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

#include "execpt.h"
#include "scriptpt.h"
#include "newobj.h"
#include "mcerror.h"
#include "globals.h"
#include "hndlrlst.h"
#include "handler.h"
#include "keywords.h"
#include "debug.h"
#include "stack.h"
#include "cmds.h"

#include "core.h"

#include "system.h"
#include "srvscript.h"

////////////////////////////////////////////////////////////////////////////////

MCServerScript::MCServerScript(void)
{
	m_files = NULL;
	m_ep = NULL;
	m_include_depth = 0;
	m_current_file = nil;
}

MCServerScript::~MCServerScript(void)
{
	while(m_files != NULL)
	{
		File *t_file;
		t_file = m_files;
		m_files = m_files -> next;
		
		if (t_file -> handle != NULL)
			t_file -> handle -> Close();
		delete t_file -> filename;
		delete t_file -> script;
		delete t_file;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCServerScript::ListFiles(MCExecPoint& ep)
{
	ep . clear();
	for(File *t_file = m_files; t_file != NULL; t_file = t_file -> next)
		ep . concatcstring(t_file -> filename, EC_RETURN, t_file == m_files);
}

uint4 MCServerScript::GetFileIndexForContext(MCExecPoint& ep)
{
	uint32_t t_file_index;
	if (ep . gethandler() != NULL)
		t_file_index = ep . gethandler() -> getfileindex();
	else
		t_file_index = m_current_file == nil ? 0 : m_current_file -> index;
	
	return t_file_index;
}

const char *MCServerScript::GetFileForContext(MCExecPoint& ep)
{
	uint32_t t_file_index;
	if (ep . gethandler() != NULL)
		t_file_index = ep . gethandler() -> getfileindex();
	else
		t_file_index = m_current_file == nil ? 0 : m_current_file -> index;
	
	for(File *t_file = m_files; t_file != NULL; t_file = t_file -> next)
		if (t_file -> index == t_file_index)
			return t_file -> filename;
	
	return NULL;
}

uint4 MCServerScript::FindFileIndex(const char *p_filename, bool p_add)
{
	File *t_file;
	t_file = FindFile(p_filename, p_add);

	if (t_file == NULL)
		return 0;
	
	return t_file -> index;
}

MCServerScript::File *MCServerScript::FindFile(const char *p_filename, bool p_add)
{
	// First resolve the filename.
	char *t_filename;
	t_filename = MCsystem -> ResolvePath(p_filename);
	
	// Look through the file list...
	File *t_file;
	for(t_file = m_files; t_file != NULL; t_file = t_file -> next)
		if (strcmp(t_file -> filename, t_filename) == 0)
			break;
	
	// If we are here the file doesn't exist (yet). If we aren't in
	// adding mode, then just return nil.
	if (t_file != NULL || !p_add)
	{
		delete t_filename;
		return t_file;
	}

	// Create a new entry.
	t_file = new File;
	t_file -> next = m_files;
	t_file -> filename = t_filename;
	t_file -> index = m_files == NULL ? 1 : m_files -> index + 1;
	t_file -> script = NULL;
	t_file -> handle = NULL;
	
	return t_file;
}

Parse_stat MCServerScript::ParseNextStatement(MCScriptPoint& sp, MCStatement*& r_statement)
{
	Parse_stat t_stat;
	t_stat = PS_NORMAL;

	// The next token type/symbol.
	Symbol_type t_type;
	const LT *t_symbol;
	
	// If we end up parsing a statement, it will be stored here.
	MCStatement *t_statement;
	t_statement = NULL;

	// Loop until we either encounter a non-normal parse state, or have a statement
	// to return.
	while(t_stat == PS_NORMAL && t_statement == NULL)
	{
		t_stat = sp . next(t_type);
		if (t_stat == PS_NORMAL)
		{
			if (t_type == ST_DATA)
			{
				// A data token turns into an Echo command.
				t_statement = new MCEcho;
				if (t_statement != NULL && t_statement -> parse(sp) != PS_NORMAL)
				{
					MCperror->add(PE_SCRIPT_BADECHO, sp);
					t_stat = PS_ERROR;
				}
			}
			else if (sp . lookup(SP_HANDLER, t_symbol) == PS_NORMAL)
			{
				// Its a handler or variable definition
				if (t_symbol -> type == TT_HANDLER)
				{
					bool t_is_private;
					t_is_private = false;
					
					if (t_symbol -> which == HT_PRIVATE)
					{
						t_is_private = true;
						
						sp . next(t_type);
						if (sp . lookup(SP_HANDLER, t_symbol) != PS_NORMAL)
							t_symbol = nil;
							
					}
					if (t_symbol != nil && (t_symbol -> which == HT_MESSAGE || t_symbol -> which == HT_FUNCTION))
					{
						MCHandler *t_new_handler;
						t_new_handler = new MCHandler((uint1)t_symbol -> which, t_is_private);
						t_new_handler -> setfileindex(m_current_file -> index);
						if (t_new_handler -> parse(sp, false) == PS_NORMAL && !hlist -> hashandler((Handler_type)t_symbol -> which, t_new_handler -> getname()))
						{
							sp . sethandler(NULL);
							hlist -> addhandler((Handler_type)t_symbol -> which, t_new_handler);
						}
						else
						{
							sp . sethandler(NULL);
							delete t_new_handler;
							MCperror -> add(PE_SCRIPT_BADHANDLER, sp);
							t_stat = PS_ERROR;
						}
					}
					else
					{
						MCperror -> add(PE_SCRIPT_BADHANDLERTYPE, sp);
						t_stat = PS_ERROR;
					}
				}
				else if (t_symbol -> type == TT_VARIABLE)
				{
					sp.sethandler(NULL);
					switch (t_symbol -> which)
					{
						case S_GLOBAL:
							t_statement = new MCGlobal;
							break;
						case S_LOCAL:
							t_statement = new MCLocalVariable;
							break;
						case S_CONSTANT:
							t_statement = new MCLocalConstant;
							break;
						default:
							t_statement = NULL;
							break;
					}
					
					if (t_statement != NULL && t_statement->parse(sp) != PS_NORMAL)
					{
						MCperror->add(PE_SCRIPT_BADVAR, sp);
						t_stat = PS_ERROR;
					}
				}
			}
			else if (sp . lookup(SP_COMMAND, t_symbol) == PS_NORMAL)
			{
				// It's a statement
				if (t_symbol -> type == TT_STATEMENT)
				{
					t_statement = MCN_new_statement(t_symbol -> which);
					
					if (t_statement -> parse(sp) != PS_NORMAL)
					{
						MCperror -> add(PE_SCRIPT_BADSTATEMENT, sp);
						t_stat = PS_ERROR;
					}
				}
				else
				{
					MCperror -> add(PE_SCRIPT_NOTSTATEMENT, sp);
					t_stat = PS_ERROR;
				}
			}
			else if (t_type == ST_ID)
			{
				// Treat it as a call
				t_statement = new MCComref(sp . gettoken_nameref());
				if (t_statement -> parse(sp) != PS_NORMAL)
				{
					MCperror -> add(PE_SCRIPT_BADCOMMAND, sp);
					t_stat = PS_ERROR;
				}
			}
			else
			{
				MCperror -> add(PE_SCRIPT_NOTCOMMAND, sp);
				t_stat = PS_ERROR;
			}
		}
		else if (t_stat == PS_EOF)
		{
			// Nothing to do - end of input.
		}
		else if (t_stat == PS_EOL)
		{
			t_stat = sp . skip_eol();
			if (t_stat != PS_NORMAL)
			{
				MCperror -> add(PE_SCRIPT_BADEOL, sp);
				t_stat = PS_ERROR;
			}
		}
		else
		{
			MCperror -> add(PE_SCRIPT_BADCHAR, sp);
			t_stat = PS_ERROR;
		}
	}

	if (t_stat == PS_NORMAL)
		r_statement = t_statement;
	else
		delete t_statement;

	return t_stat;
}

// MW-2009-06-02: Add support for 'require' style includes.
bool MCServerScript::Include(MCExecPoint& outer_ep, const char *p_filename, bool p_require)
{
	if (p_filename == NULL || p_filename[0] == '\0')
	{
		MCeerror->add(EE_INCLUDE_BADFILENAME, 0, 0, p_filename);
		return false;
	}

	if (hlist == NULL)
		hlist = new MCHandlerlist;
	
	if (m_ep == NULL)
		m_ep = new MCExecPoint(this, hlist, NULL);

	// Save the old default folder
	char *t_old_folder;
	t_old_folder = MCsystem->GetCurrentFolder();

	if (m_current_file != nil)
	{
		// Set the default folder to the folder containing the current script
		char *t_full_path;
		t_full_path = MCsystem->LongFilePath(m_current_file->filename);
		uindex_t t_last_separator;
		if (MCCStringLastIndexOf(t_full_path, '/', t_last_separator))
		{
			t_full_path[t_last_separator] = '\0';
			MCsystem->SetCurrentFolder(t_full_path);
		}
		MCCStringFree(t_full_path);
	}

	// Look for the file
	File *t_file;
	t_file = FindFile(p_filename, true);
	if (t_file -> index == 1)
	{
		setfilename(t_file -> filename);
	}

	// Set back the old default folder
	MCsystem->SetCurrentFolder(t_old_folder);
	MCCStringFree(t_old_folder);

	// If we are 'requiring' and the script is already loaded, we are done.
	if (t_file -> script != NULL && p_require)
		return true;
	
	// If the file isn't open yet, open it
	if (t_file -> script == NULL)
	{
		// Attempt to open the file
		MCSystemFileHandle *t_handle;
		t_handle = MCsystem -> OpenFile(t_file -> filename, kMCSystemFileModeRead | kMCSystemFileModeNulTerminate, true);
		if (t_handle == NULL)
		{
			MCeerror -> add(EE_INCLUDE_FILENOTFOUND, 0, 0, t_file -> filename);
			return false;
		}
		
		// If the file was successfully memory-mapped, then use the direct pointer,
		// otherwise just load it all into memory.
		t_file -> script = (char *)t_handle -> GetFilePointer();
		if (t_file -> script != NULL)
			t_file -> handle = t_handle;
		else
		{
			int32_t t_length;
			t_length = (int32_t)t_handle -> GetFileSize();
			t_file -> script = new char[t_length + 1];
			
			uint32_t t_read;
			t_handle -> Read(t_file -> script, t_length, t_read);
			
			t_file -> script[t_length] = '\0';
			
			t_handle -> Close();
			
			t_file -> handle = NULL;
		}
		
		m_files = t_file;
	}
	
	// Save the old file index
	File *t_old_file;
	t_old_file = m_current_file;
	
	// Set the current one.
	m_current_file = t_file;
	
	// Note that script point does not copy 'script' and requires it to be NUL-
	// terminated. Indeed, this string *has* to persist until termination as
	// constants, handler names and variable names use substrings of it directly.
	MCScriptPoint sp(this, hlist, t_file -> script);
    
    // MERG 2013-12-24: [[ Shebang ]] Don't use tagged mode in script files
    if (!(t_file -> script[0] == '#' && t_file -> script[1] == '!'))
        sp . allowtags(True);
	
	// The statement chain that will executed.
	MCStatement *t_statements, *t_last_statement;
	t_statements = t_last_statement = nil;

	// Clear any parse errors
	MCperror -> clear();

	// Parse the statements
	Parse_stat t_stat;
	t_stat = PS_NORMAL;
	for(;;)
	{	
		// If we end up parsing a statement, it will be stored here.
		MCStatement *t_statement;
		t_statement = NULL;

		// Fetch the next statement (if any).
		t_stat = ParseNextStatement(sp, t_statement);
	
		// If we got a statement, append it to the chain.
		if (t_statement != nil)
		{
			if (t_last_statement != nil)
				t_last_statement -> setnext(t_statement);
			else
				t_statements = t_statement;

			t_last_statement = t_statement;
		}
		else if (t_stat == PS_EOF)
		{
			t_stat = PS_NORMAL;
			break;
		}
		else
			break;
	}

	////
	
	// We are about to start execution from a new file so increase the include
	// depth.
	m_include_depth += 1;	
	
	// Execute any statements
	if (t_stat == PS_NORMAL && t_statements != nil)
	{
		MCStatement *t_statement;
		t_statement = t_statements;
		while(t_stat == PS_NORMAL && !MCexitall && t_statement != nil)
		{
			if (MCtrace || MCnbreakpoints)
				MCB_trace(*m_ep, t_statement -> getline(), t_statement -> getpos());
			
			if (!MCexitall)
			{
				Exec_stat t_exec_stat;
				t_exec_stat = t_statement -> exec(*m_ep);
				if (t_exec_stat != ES_NORMAL)
				{
					// Throw an error in the debugger
					if ((MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
						do
						{
							if (!MCB_error(*m_ep, t_statement->getline(), t_statement->getpos(), EE_HANDLER_BADSTATEMENT))
								break;
						}
						while (MCtrace && (t_exec_stat = t_statement->exec(*m_ep)) != ES_NORMAL);

					// Flag an error.
					t_stat = PS_ERROR;
					
					break;
				}
			}

			t_statement = t_statement -> getnext();
		}

		t_statements -> deletestatements(t_statements);
	}
	
	// Reduce the include depth.
	m_include_depth -= 1;
	
	////

	// Report a parse error, if any. Otherwise append a file index.
	if (t_stat == PS_ERROR && !MCperror -> isempty())
	{
		char t_buffer[U4L];
		sprintf(t_buffer, "%u", t_file -> index);
		MCeerror -> add(EE_SCRIPT_SYNTAXERROR, 0, 0, t_buffer);
		MCeerror -> append(*MCperror);
		MCeerror -> add(EE_SCRIPT_SYNTAXERROR, 0, 0);
		MCperror -> clear();
		
		// Throw an error in the debugger
		if ((MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
			MCB_error(*m_ep, 0, 0, EE_SCRIPT_SYNTAXERROR);
	}
	else
	{
		char t_buffer[U4L];
		sprintf(t_buffer, "%u", t_file -> index);
		MCeerror -> add(EE_SCRIPT_FILEINDEX, 0, 0, t_buffer);
	}

	////


	// Set back the old file index.
	m_current_file = t_old_file;
	
	return t_stat == PS_NORMAL;
}

uint32_t MCServerScript::GetIncludeDepth(void)
{
	return m_include_depth;
}

////////////////////////////////////////////////////////////////////////////////
