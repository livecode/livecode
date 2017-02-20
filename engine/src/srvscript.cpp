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


#include "exec.h"
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
#include "variable.h"


#include "system.h"
#include "srvscript.h"

////////////////////////////////////////////////////////////////////////////////

MCServerScript::MCServerScript(void)
{
	m_files = NULL;
	m_ctxt = NULL;
	m_include_depth = 0;
	m_current_file = nil;
	
	// MW-2013-11-08: [[ RefactorIt ]] This varref is created when hlist is.
	m_it = nil;
}

MCServerScript::~MCServerScript(void)
{
	while(m_files != NULL)
	{
		File *t_file;
		t_file = m_files;
		m_files = m_files -> next;

        // Closing a MCMemoryMappedFileHandle calls unmap()
        // and thus deallocates the memory mapped - which is what's stored in t_file -> script
        if (t_file -> handle != NULL)
            t_file -> handle -> Close();
        else
            delete t_file -> script;

		delete t_file;
	}
	
	// MW-2013-11-08: [[ RefactorIt ]] Dispose of the it varref.
	delete m_it;
}

////////////////////////////////////////////////////////////////////////////////

void MCServerScript::ListFiles(MCStringRef &r_string)
{
	MCListRef t_list;
	/* UNCHECKED */ MCListCreateMutable('\n', t_list);
	for(File *t_file = m_files; t_file != NULL; t_file = t_file -> next)
		/* UNCHECKED */ MCListAppend(t_list, *t_file->filename);
	
	/* UNCHECKED */ MCListCopyAsStringAndRelease(t_list, r_string);
}

uint4 MCServerScript::GetFileIndexForContext(MCExecContext &ctxt)
{
	uint32_t t_file_index;
	if (ctxt.GetHandler() != NULL)
		t_file_index = ctxt.GetHandler() -> getfileindex();
	else
		t_file_index = m_current_file == nil ? 0 : m_current_file -> index;
	
	return t_file_index;
}

bool MCServerScript::GetFileForContext(MCExecContext &ctxt, MCStringRef &r_file)
{
	uint32_t t_file_index;
	if (ctxt.GetHandler() != NULL)
		t_file_index = ctxt.GetHandler() -> getfileindex();
	else
		t_file_index = m_current_file == nil ? 0 : m_current_file -> index;
	
	for(File *t_file = m_files; t_file != NULL; t_file = t_file -> next)
		if (t_file -> index == t_file_index)
            return MCStringCopy(*t_file -> filename, r_file);
	
    return false;
}

uint4 MCServerScript::FindFileIndex(MCStringRef p_filename, bool p_add)
{
	File *t_file;
	t_file = FindFile(p_filename, p_add);

	if (t_file == NULL)
		return 0;

    /* If the file was newly-created, link it into the MCServerScript
     * instance's list of files so that it doesn't get leaked. */
    /* TODO[2017-02-06] This is fragile; FindFile() should be
     * refactored so that it's not necessary to guess whether the
     * caller owns the returned pointer or not. */
    if (t_file->next == m_files)
        m_files = t_file;

	return t_file -> index;
}

// SN-2014-09-05: [[ Bug 13378 ]] Added forgotten function GetIt
MCVarref* MCServerScript::GetIt()
{
    return m_it;
}

MCServerScript::File *MCServerScript::FindFile(MCStringRef p_filename, bool p_add)
{
	// First resolve the filename.

	MCAutoStringRef t_resolved_filename;
	MCsystem -> ResolvePath(p_filename, &t_resolved_filename);
	
	// Look through the file list...
	File *t_file;
	for(t_file = m_files; t_file != NULL; t_file = t_file -> next)
        if (MCStringIsEqualTo(*t_file -> filename, *t_resolved_filename, kMCStringOptionCompareExact))
			break;
	
	// If we are here the file doesn't exist (yet). If we aren't in
	// adding mode, then just return nil.
	if (t_file != NULL || !p_add)
	{
		return t_file;
	}

	// Create a new entry.
	t_file = new (nothrow) File;
	t_file -> next = m_files;
    t_file -> filename = MCValueRetain(*t_resolved_filename);
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
				t_statement = new (nothrow) MCEcho;
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
						t_new_handler = new (nothrow) MCHandler((uint1)t_symbol -> which, t_is_private);
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
							t_statement = new (nothrow) MCGlobal;
							break;
						case S_LOCAL:
							t_statement = new (nothrow) MCLocalVariable;
							break;
						case S_CONSTANT:
							t_statement = new (nothrow) MCLocalConstant;
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
				t_statement = new (nothrow) MCComref(sp . gettoken_nameref());
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
bool MCServerScript::Include(MCExecContext& ctxt, MCStringRef p_filename, bool p_require)
{
	if (MCStringIsEmpty(p_filename))
	{
		MCeerror->add(EE_INCLUDE_BADFILENAME, 0, 0, p_filename);
		return false;
	}

	if (hlist == NULL)
	{
		hlist = new (nothrow) MCHandlerlist;
	}
	
	if (m_ctxt == NULL)
	{
		m_ctxt = new (nothrow) MCExecContext(this, hlist, NULL);
		// MW-2013-11-08: [[ RefactorIt ]] Make sure we have an 'it' var in global context.
		/* UNCHECKED */ hlist -> newvar(MCN_it, nil, &m_it, False);
	}

	// Save the old default folder
	MCAutoStringRef t_old_folder;
	MCsystem->GetCurrentFolder(&t_old_folder);

	if (m_current_file != nil)
    {
		// Set the default folder to the folder containing the current script
		MCAutoStringRef t_full_path;
        /* UNCHECKED */ MCsystem->LongFilePath(*m_current_file -> filename, &t_full_path);
		
		uindex_t t_last_separator;
		if (MCStringLastIndexOfChar(*t_full_path, '/', UINDEX_MAX, kMCStringOptionCompareExact, t_last_separator))
		{
			MCAutoStringRef t_folder;
			/* UNCHECKED */ MCStringCopySubstring(*t_full_path, MCRangeMake(0, t_last_separator), &t_folder);
			MCsystem->SetCurrentFolder(*t_folder);
		}
	}

	// Look for the file
	File *t_file;
	t_file = FindFile(p_filename, true);
	if (t_file -> index == 1)
	{
		setfilename(*t_file -> filename);
	}

	// Set back the old default folder
	MCsystem->SetCurrentFolder(*t_old_folder);

	// If we are 'requiring' and the script is already loaded, we are done.
	if (t_file -> script != NULL && p_require)
	{
		delete t_file;
		return true;
	}
	
	// If the file isn't open yet, open it
	if (t_file -> script == NULL)
	{
		MCAutoDataRef t_file_contents;

		if (!MCS_loadbinaryfile (*t_file->filename,
								 &t_file_contents))
		{
			MCeerror -> add(EE_INCLUDE_FILENOTFOUND, 0, 0, *t_file -> filename);
			delete t_file;
			return false;
		}

		uindex_t t_length;
		t_length = MCDataGetLength (*t_file_contents);
		t_file -> script = new (nothrow) char[t_length + 1];

		MCMemoryCopy (t_file -> script,
					  MCDataGetBytePtr (*t_file_contents),
					  t_length);
		/* Ensure trailing nul */
		t_file -> script[t_length] = 0;

		m_files = t_file;
	}
	
	// Save the old file index
	File *t_old_file;
	t_old_file = m_current_file;
	
	// Set the current one.
	m_current_file = t_file;
	
    // MERG 2013-12-24: [[ Shebang ]] Don't use tagged mode in script files
    bool t_is_script_file;
    t_is_script_file = false;
    if (t_file -> script[0] == '#' && t_file -> script[1] == '!')
        t_is_script_file = true;
    
    // MW-2014-10-24: [[ Bug 13730 ]] When in script file mode, we check the second
    //   line for a match to the RE "coding[=:]\s*([-\w.]+)" and take this to be the
    //   source encoding.
    MCStringEncoding t_encoding;
    t_encoding = kMCStringEncodingNative;
    if (t_is_script_file)
    {
        char *t_end_of_first_line;
        t_end_of_first_line = strchr(t_file -> script, '\n');
        if (t_end_of_first_line != NULL)
        {
            t_end_of_first_line += 1;
            if (t_end_of_first_line[0] == '\r')
                t_end_of_first_line += 1;
            if (t_end_of_first_line[0] == '#')
            {
                const char *t_end_of_second_line;
                t_end_of_second_line = strchr(t_end_of_first_line, '\n');
                if (t_end_of_second_line == NULL)
                    t_end_of_second_line = t_end_of_first_line + strlen(t_end_of_first_line);
                
                MCAutoStringRef t_line;
                /* UNCHECKED */ MCStringCreateWithNativeChars((const char_t *)t_end_of_first_line, t_end_of_second_line - t_end_of_first_line, &t_line);

                MCAutoStringRef t_encoding_str;
                
                regexp *t_regexp;
                t_regexp = MCR_compile(MCSTR("coding[=:]\\s*([-\\w.]+)"), false);
                if (t_regexp != NULL)
                {
                    if (MCR_exec(t_regexp, *t_line, MCRangeMake(0, MCStringGetLength(*t_line))) != 0 &&
                        t_regexp -> matchinfo[1] . rm_so != -1)
                    {
                        uindex_t t_start, t_length;
                        t_start = t_regexp->matchinfo[1].rm_so;
                        t_length = t_regexp->matchinfo[1].rm_eo - t_start;
                        /* UNCHECKED */ MCStringCopySubstring(*t_line, MCRangeMake(t_start, t_length), &t_encoding_str);
                    }
                }
                
                if (*t_encoding_str != NULL)
                    MCStringsEvalTextEncoding(*t_encoding_str, t_encoding);
            }
        }
    }
    
    MCAutoStringRef t_file_script;
    /* UNCHECKED */ MCStringCreateWithBytes((const byte_t *)t_file -> script, strlen(t_file -> script), t_encoding, false, &t_file_script);
	MCScriptPoint sp(this, hlist, *t_file_script);

    if (!t_is_script_file)
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
				MCB_trace(*m_ctxt, t_statement -> getline(), t_statement -> getpos());
			
			if (!MCexitall)
			{
				m_ctxt -> SetLineAndPos(t_statement -> getline(), t_statement -> getpos());
				
				Exec_stat t_exec_stat;
				t_statement -> exec_ctxt(*m_ctxt);
				t_exec_stat = m_ctxt -> GetExecStat();
				m_ctxt -> IgnoreLastError();
				
				if (t_exec_stat != ES_NORMAL)
				{
					// Throw an error in the debugger
					if ((MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
						do
						{
							if (!MCB_error(*m_ctxt, t_statement->getline(), t_statement->getpos(), EE_HANDLER_BADSTATEMENT))
								break;
							m_ctxt -> IgnoreLastError();
							t_statement -> exec_ctxt(*m_ctxt);
						}
						while (MCtrace && (t_exec_stat = m_ctxt -> GetExecStat()) != ES_NORMAL);

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
			MCB_error(*m_ctxt, 0, 0, EE_SCRIPT_SYNTAXERROR);
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
