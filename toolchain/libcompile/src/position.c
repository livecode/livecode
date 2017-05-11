/* Copyright (C) 2016 LiveCode Ltd.
 
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

static intptr_t s_current_position;

void InitializePosition(void)
{
    s_current_position = 0;
}

void FinalizePosition(void)
{
}

void AdvanceCurrentPosition(intptr_t p_delta)
{
    intptr_t t_column;
    GetColumnOfPosition(s_current_position, &t_column);
    t_column += p_delta;
    if (t_column > COLUMNS_PER_ROW)
        t_column = COLUMNS_PER_ROW;
    s_current_position = (s_current_position / COLUMNS_PER_ROW) * COLUMNS_PER_ROW + (t_column - 1);
}

void AdvanceCurrentPositionToNextRow(void)
{
    intptr_t t_row;
    GetRowOfPosition(s_current_position, &t_row);
    t_row += 1;
    if (t_row > ROWS_PER_FILE)
        t_row = ROWS_PER_FILE;
    s_current_position = (s_current_position / COLUMNS_PER_FILE) * COLUMNS_PER_FILE + (t_row - 1) * COLUMNS_PER_ROW;
}

void AdvanceCurrentPositionToFile(FileRef p_file)
{
    intptr_t t_index;
    GetFileIndex(p_file, &t_index);
    s_current_position = t_index * COLUMNS_PER_FILE;
}

void GetColumnOfPosition(intptr_t p_position, intptr_t *r_column)
{
    *r_column = (p_position % COLUMNS_PER_ROW) + 1;
}

void GetRowOfPosition(intptr_t p_position, intptr_t *r_row)
{
    *r_row = ((p_position / COLUMNS_PER_ROW) % ROWS_PER_FILE) + 1;
}

void GetRowTextOfPosition(intptr_t p_position, const char **r_text)
{
	FileRef t_file = NULL;
	intptr_t t_row = 0;
	GetFileOfPosition(p_position, &t_file);
	GetRowOfPosition(p_position, &t_row);
	*r_text = GetFileLineText(t_file, t_row);
}

void GetFileOfPosition(intptr_t p_position, FileRef *r_file)
{
    intptr_t t_index;
    t_index = p_position / COLUMNS_PER_FILE;
    if (GetFileWithIndex(t_index, r_file) == 0)
        Fatal_InternalInconsistency("Position encoded with invalid file index");
}

void GetFilenameOfPosition(intptr_t p_position, const char **r_filename)
{
    FileRef t_file = NULL;
    GetFileOfPosition(p_position, &t_file);
    GetFilePath(t_file, r_filename);
}

void GetCurrentPosition(intptr_t *r_result)
{
    *r_result = s_current_position;
}

void GetUndefinedPosition(intptr_t *r_result)
{
    *r_result = -1;
}

void yyGetPos(intptr_t *r_result)
{
    GetCurrentPosition(r_result);
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
	intptr_t t_file_length = 0;
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

const char *GetFileLineText(FileRef p_file, intptr_t p_row)
{
	__InitializeFileLines(p_file);
	if (p_file->line_text == NULL)
		return NULL;
	if (p_row > p_file->line_count)
		Fatal_InternalInconsistency("Examining line index beyond end of file");
	return p_file->line_text[p_row-1];
}

void GetFileIndex(FileRef p_file, intptr_t *r_index)
{
    *r_index = p_file -> index;
}

int GetFileWithIndex(intptr_t p_index, FileRef *r_file)
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

////////////////////////////////////////////////////////////////////////////////
