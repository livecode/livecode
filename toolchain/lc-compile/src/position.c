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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "report.h"
#include "position.h"

////////////////////////////////////////////////////////////////////////////////

#define COLUMNS_PER_ROW 1000
#define ROWS_PER_FILE 10000
#define COLUMNS_PER_FILE (COLUMNS_PER_ROW * ROWS_PER_FILE)

#define MAXPATHLEN 4096

static long s_current_position;

void InitializePosition(void)
{
    s_current_position = 0;
}

void FinalizePosition(void)
{
}

void AdvanceCurrentPosition(long p_delta)
{
    long t_column;
    GetColumnOfPosition(s_current_position, &t_column);
    t_column += p_delta;
    if (t_column > COLUMNS_PER_ROW)
        t_column = COLUMNS_PER_ROW;
    s_current_position = (s_current_position / COLUMNS_PER_ROW) * COLUMNS_PER_ROW + (t_column - 1);
}

void AdvanceCurrentPositionToNextRow(void)
{
    long t_row;
    GetRowOfPosition(s_current_position, &t_row);
    t_row += 1;
    if (t_row > ROWS_PER_FILE)
        t_row = ROWS_PER_FILE;
    s_current_position = (s_current_position / COLUMNS_PER_FILE) * COLUMNS_PER_FILE + (t_row - 1) * COLUMNS_PER_ROW;
}

void AdvanceCurrentPositionToFile(FileRef p_file)
{
    long t_index;
    GetFileIndex(p_file, &t_index);
    s_current_position = t_index * COLUMNS_PER_FILE;
}

void GetColumnOfPosition(long p_position, long *r_column)
{
    *r_column = (p_position % COLUMNS_PER_ROW) + 1;
}

void GetRowOfPosition(long p_position, long *r_row)
{
    *r_row = ((p_position / COLUMNS_PER_ROW) % ROWS_PER_FILE) + 1;
}

void GetRowTextOfPosition(long p_position, const char **r_text)
{
	FileRef t_file = NULL;
	long t_row = 0;
	GetFileOfPosition(p_position, &t_file);
	GetRowOfPosition(p_position, &t_row);
	*r_text = GetFileLineText(t_file, t_row);
}

void GetFileOfPosition(long p_position, FileRef *r_file)
{
    long t_index;
    t_index = p_position / COLUMNS_PER_FILE;
    if (GetFileWithIndex(t_index, r_file) == 0)
        Fatal_InternalInconsistency("Position encoded with invalid file index");
}

void GetFilenameOfPosition(long p_position, const char **r_filename)
{
    FileRef t_file;
    GetFileOfPosition(p_position, &t_file);
    GetFilePath(t_file, r_filename);
}

void GetCurrentPosition(long *r_result)
{
    *r_result = s_current_position;
}

void GetUndefinedPosition(long *r_result)
{
    *r_result = -1;
}

void yyGetPos(long *r_result)
{
    GetCurrentPosition(r_result);
}

////////////////////////////////////////////////////////////////////////////////

static const char *ImportedModuleDir[8];
static int ImportedModuleDirCount = 0;
static const char *s_interface_output_file = NULL;
static const char *ImportedModuleNames[32];
static int ImportedModuleNameCount = 0;

static int IsImportedModuleName(const char *p_name)
{
	int i;
	for(i = 0; i < ImportedModuleNameCount; i++)
	{
		if (strcmp(p_name, ImportedModuleNames[i]) == 0)
		{
			return 1;
		}
	}
	return 0;
}

void AddImportedModuleDir(const char *p_dir)
{
    if (ImportedModuleDirCount < 8)
        ImportedModuleDir[ImportedModuleDirCount++] = p_dir;
    else
        Fatal_InternalInconsistency("Too many module paths");
}

void AddImportedModuleName(const char *p_name)
{
	if (IsImportedModuleName(p_name))
	{
		return;
	}
	
	if (ImportedModuleNameCount == sizeof(ImportedModuleNames) / sizeof(const char *))
	{
		Fatal_InternalInconsistency("Too many imported module names");
		return;
	}
	
	ImportedModuleNames[ImportedModuleNameCount++] = p_name;
}

int AddImportedModuleFile(const char *p_name)
{
    char t_path[MAXPATHLEN];
	FILE *t_file;
	
	if (IsImportedModuleName(p_name))
	{
		return 1;
	}
	
    t_file = NULL;
    if (ImportedModuleDirCount > 0)
    {
		int i;
        for(i = 0; i < ImportedModuleDirCount; i++)
        {
            /* OVERFLOW */ sprintf(t_path, "%s/%s.lci", ImportedModuleDir[i], p_name);
            t_file = fopen(t_path, "r");
            if (t_file != NULL)
                break;
        }
    }
    else
    {
        /* OVERFLOW */ sprintf(t_path, "%s.lci", p_name);
        t_file = fopen(t_path, "r");
    }
    
    if (t_file == NULL)
        return 0;
    
    fclose(t_file);
    
    AddFile(t_path);
    
    return 1;
}

void FindImportedModuleFile(const char *p_name, char** r_module_file)
{
    char t_path[MAXPATHLEN];
	FILE *t_file;
    
    t_file = NULL;
    if (ImportedModuleDirCount > 0)
    {
		int i;
        for(i = 0; i < ImportedModuleDirCount; i++)
        {
            /* OVERFLOW */ sprintf(t_path, "%s/%s.lci", ImportedModuleDir[i], p_name);
            t_file = fopen(t_path, "r");
            if (t_file != NULL)
                break;
        }
    }
    else
    {
        /* OVERFLOW */ sprintf(t_path, "%s.lci", p_name);
        t_file = fopen(t_path, "r");
    }
    
    if (t_file == NULL)
    {
        if (ImportedModuleDirCount > 0)
            /* OVERFLOW */ sprintf(t_path, "%s/%s.lci", ImportedModuleDir[0], p_name);
    }
    else
        fclose(t_file);
    
    *r_module_file = strdup(t_path);
}

FILE *
OpenImportedModuleFile (const char *p_name,
                        char **r_filename)
{
    char t_path[MAXPATHLEN];
    FILE *t_file;

    if (ImportedModuleDirCount == 0 &&
        s_interface_output_file == NULL)
        return NULL;

    if (NULL == s_interface_output_file)
    {
        // Use the first modulepath to write the interface file into.
        /* OVERFLOW */ sprintf(t_path, "%s/%s.lci", ImportedModuleDir[0], p_name);
    }
    else
    {
        /* OVERFLOW */ sprintf(t_path, "%s", s_interface_output_file);
    }
    
    if (NULL != r_filename)
	{
		*r_filename = strdup(t_path); /* FIXME should be strndup */
	}

    t_file = fopen(t_path, "w");
    
    return t_file;
}

////////////////////////////////////////////////////////////////////////////////

struct File
{
    FileRef next;
    char *path;
    char *name;
    unsigned int index;

	int line_text_initialized;
	char *line_text_buffer;
	const char **line_text;
	int line_count;
};

static FileRef s_files;
static FileRef s_current_file;
static unsigned int s_next_file_index;
static const char *s_template_file = NULL;
static const char *s_output_bytecode_file = NULL;
static const char *s_output_code_file = NULL;
static const char *s_output_grammar_file = NULL;
static const char *s_manifest_output_file = NULL;

void InitializeFiles(void)
{
    s_files = NULL;
    s_current_file = NULL;
    s_next_file_index = 0;
}

void FinalizeFiles(void)
{
}

int FileAlreadyAdded(const char *p_filename)
{
    FileRef t_file;
    for(t_file = s_files; t_file != NULL; t_file = t_file -> next)
        if (strcmp(t_file -> path, p_filename) == 0)
            return 1;

    return 0;
}

void AddFile(const char *p_filename)
{
	FileRef t_new_file;
	FileRef *t_last_file_ptr;
	const char *t_name;

    if (FileAlreadyAdded(p_filename))
        return;
    
    t_new_file = (FileRef)calloc(sizeof(struct File), 1);
    if (t_new_file == NULL)
        Fatal_OutOfMemory();
    
    t_new_file -> path = strdup(p_filename);
    if (t_new_file -> path == NULL)
        Fatal_OutOfMemory();
    
#ifndef _WIN32
    t_name = strrchr(p_filename, '/');
#else
    t_name = strrchr(p_filename, '\\');
#endif
    if (t_name == NULL)
        t_name = p_filename;
    else
        t_name += 1;
    t_new_file -> name = strdup(t_name);
    if (t_new_file -> name == NULL)
        Fatal_OutOfMemory();
    
    t_new_file -> index = s_next_file_index++;
	
	t_new_file->line_text_initialized = 0;

    for(t_last_file_ptr = &s_files; *t_last_file_ptr != NULL; t_last_file_ptr = &((*t_last_file_ptr) -> next))
        ;
    
    *t_last_file_ptr = t_new_file;
}

int MoveToNextFile(void)
{
    for(;;)
    {
        FILE *t_stream;
		
		if (s_current_file == NULL)
            s_current_file = s_files;
        else
            s_current_file = s_current_file -> next;
        
        if (s_current_file == NULL)
            break;

        t_stream = fopen(s_current_file -> path, "r");
        if (t_stream != NULL)
        {
            extern void yynextfile(FILE *stream);
            AdvanceCurrentPositionToFile(s_current_file);
            yynextfile(t_stream);
            return 1;
        }
        else
            Error_CouldNotOpenInputFile(s_current_file -> path);
    }
    
    return 0;
}

void GetFilePath(FileRef p_file, const char **r_path)
{
    *r_path = p_file -> path;
}

void GetFileName(FileRef p_file, const char **r_name)
{
    *r_name = p_file -> name;
}

static int __FindNextSeparator(const char *p_buffer, int p_length,
                               int p_start, int *r_sep)
{
	for (*r_sep = p_start; *r_sep < p_length; ++*r_sep)
	{
		if (p_buffer[*r_sep] == '\n')
			return 1;

		if (p_buffer[*r_sep] == '\r')
		{
			if (p_buffer[1+ *r_sep] == '\n')
				return 2;
			else
				return 1;
		}
	}
	return 0;
}

/* Scan a file and build an array of line texts.  This is expensive,
 * so it's performed lazily and _only_ if it's needed for printing
 * warning or error messages.
 *
 * As per SEPARATOR.t, \r, \n, and \r\n are all considered to be line
 * breaks.
 */
static void __InitializeFileLines(FileRef x_file)
{
	FILE *t_stream = NULL;
	long t_file_length = 0;
	char *t_raw_text = NULL;
	const char **t_lines = NULL;
	int t_line_count = 0;
	int t_offset = 0;
	int t_last_offset = 0;
	int t_eol_length = 0;

	if (0 != x_file->line_text_initialized)
		return;

	x_file->line_text_initialized = 1;
	x_file->line_text_buffer = NULL;
	x_file->line_text = NULL;
	x_file->line_count = 0;

	t_stream = fopen(x_file->path, "rb");

	/* Allocate and fill a buffer with the entire file contents. */
	if (0 > fseek(t_stream, 0, SEEK_END))
		goto cleanup;
	t_file_length = ftell(t_stream);
	if (0 > t_file_length)
		goto cleanup;

	if (0 > fseek(t_stream, 0, SEEK_SET))
		goto cleanup;

	t_raw_text = malloc((t_file_length + 1) * sizeof(*t_raw_text));
	if (NULL == t_raw_text)
		Fatal_OutOfMemory();

	if ((size_t) t_file_length != fread(t_raw_text, 1, t_file_length,
	                                    t_stream))
		goto cleanup;
	t_raw_text[t_file_length] = 0; /* nul-terminate */

	fclose(t_stream);
	t_stream = NULL;

	/* Scan the file contents twice: once to count the number of
	 * lines, and once to fill a pointer array with offset
	 * pointers. */
	t_line_count = 1;
	t_offset = 0;
	while (t_offset < t_file_length)
	{
		t_eol_length = __FindNextSeparator(t_raw_text, t_file_length,
		                                   t_offset, &t_offset);
		++t_line_count;
		t_offset += t_eol_length;
	}

	/* The lines array contains an extra, empty line at the end, which
	 * is necessary because when lc-compile detects an error at the
	 * end of the file (e.g. missing an "end module", it gives a
	 * position *after* the last byte in the file. For consistency,
	 * this "line" is actually a pointer to the additional nul byte
	 * that was added to the end of the raw text buffer above. */
	t_lines = malloc((t_line_count + 1) * sizeof(*t_lines));
	if (NULL == t_lines)
		Fatal_OutOfMemory();

	t_line_count = 0;
	t_offset = 0;
	t_last_offset = 0;
	while (t_offset < t_file_length)
	{
		t_eol_length = __FindNextSeparator(t_raw_text, t_file_length,
		                                   t_offset, &t_offset);
		if (0 < t_eol_length)
			t_raw_text[t_offset] = 0; /* nul-terminate the line */
		t_offset += t_eol_length;

		t_lines[t_line_count++] = (t_raw_text + t_last_offset);
		t_last_offset = t_offset;
	}
	/* Add the extra empty line */
	t_lines[t_line_count++] = (t_raw_text + t_file_length);

	x_file->line_text_buffer = t_raw_text;
	t_raw_text = NULL;
	x_file->line_text = t_lines;
	t_lines = NULL;
	x_file->line_count = t_line_count;

	Debug("Built message context data for %s (%i lines)",
	      x_file->path, x_file->line_count);

 cleanup:
	if (NULL != t_stream)
		fclose(t_stream);
	if (NULL != t_raw_text)
		free(t_raw_text);
	assert(NULL == t_lines);
	return;
}

const char *GetFileLineText(FileRef p_file, long p_row)
{
	__InitializeFileLines(p_file);
	if (p_file->line_text == NULL)
		return NULL;
	if (p_row > p_file->line_count)
		Fatal_InternalInconsistency("Examining line index beyond end of file");
	return p_file->line_text[p_row-1];
}

void GetFileIndex(FileRef p_file, long *r_index)
{
    *r_index = p_file -> index;
}

int GetFileWithIndex(long p_index, FileRef *r_file)
{
    FileRef t_file;

	for(t_file = s_files; t_file != NULL; t_file = t_file -> next)
        if (t_file -> index == p_index)
        {
            *r_file = t_file;
            return 1;
        }
    
    return 0;
}

int GetCurrentFile(FileRef *r_file)
{
    if (s_current_file == NULL)
        return 0;
    
    *r_file = s_current_file;

    return 1;
}

void SetOutputBytecodeFile(const char *p_output)
{
    s_output_bytecode_file = p_output;
}

void SetOutputGrammarFile(const char *p_output)
{
    s_output_grammar_file = p_output;
}

void SetOutputCodeFile(const char *p_output)
{
    s_output_code_file = p_output;
}

void SetManifestOutputFile(const char *p_output)
{
    s_manifest_output_file = p_output;
}

void SetInterfaceOutputFile(const char *p_output)
{
    s_interface_output_file = p_output;
}

void SetTemplateFile(const char *p_output)
{
    s_template_file = p_output;
}

void GetOutputFile(const char **r_output)
{
    if (s_output_bytecode_file != NULL)
        *r_output = s_output_bytecode_file;
    else
        *r_output = s_output_code_file;
}

FILE *OpenOutputBytecodeFile(const char **r_filename)
{
    if (s_output_bytecode_file == NULL)
        return NULL;

	if (NULL != r_filename)
	{
		*r_filename = s_output_bytecode_file;
	}

    return fopen(s_output_bytecode_file, "wb");
}

FILE *OpenOutputCodeFile(const char **r_filename)
{
    if (s_output_code_file == NULL)
        return NULL;
    
	if (NULL != r_filename)
	{
		*r_filename = s_output_code_file;
	}
    
    return fopen(s_output_code_file, "w");
}

FILE *OpenOutputGrammarFile(const char **r_filename)
{
    if (s_output_grammar_file == NULL)
        return NULL;

	if (NULL != r_filename)
	{
		*r_filename = s_output_grammar_file;
	}

	if (s_output_grammar_file[0] == '-' &&
	    s_output_grammar_file[1] == '\0')
	{
		return stdout;
	}
    
    return fopen(s_output_grammar_file, "w");
}

FILE *OpenManifestOutputFile(void)
{
    if (s_manifest_output_file == NULL)
        return NULL;
    return fopen(s_manifest_output_file, "w");
}

FILE *OpenTemplateFile(void)
{
    return fopen(s_template_file, "r");
}

////////////////////////////////////////////////////////////////////////////////
