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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "report.h"
#include "position.h"

////////////////////////////////////////////////////////////////////////////////

#define COLUMNS_PER_ROW 1000
#define ROWS_PER_FILE 10000
#define COLUMNS_PER_FILE (COLUMNS_PER_ROW * ROWS_PER_FILE)

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
    long t_column, t_row;
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

void GetFileOfPosition(long p_position, FileRef *r_file)
{
    long t_index;
    t_index = p_position / COLUMNS_PER_FILE;
    if (GetFileWithIndex(t_index, r_file) == 0)
        Fatal_InternalInconsistency("Position encoded with invalid file index");
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

void AddImportedModuleDir(const char *p_dir)
{
    if (ImportedModuleDirCount < 8)
        ImportedModuleDir[ImportedModuleDirCount++] = p_dir;
    else
        Fatal_InternalInconsistency("Too many module paths");
}

int AddImportedModuleFile(const char *p_name)
{
    char t_path[4096];
	FILE *t_file;
    
    t_file = NULL;
    if (ImportedModuleDirCount > 0)
    {
		int i;
        for(i = 0; i < ImportedModuleDirCount; i++)
        {
            sprintf(t_path, "%s/%s.lci", ImportedModuleDir[i], p_name);
            t_file = fopen(t_path, "r");
            if (t_file != NULL)
                break;
        }
    }
    else
    {
        sprintf(t_path, "%s.lci", p_name);
        t_file = fopen(t_path, "r");
    }
    
    if (t_file == NULL)
        return 0;
    
    fclose(t_file);
    
    AddFile(t_path);
    
    return 1;
}

FILE *
OpenImportedModuleFile (const char *p_name,
                        char **r_filename)
{
    char t_path[4096];
    FILE *t_file;

    if (ImportedModuleDirCount == 0)
        return NULL;

    // Use the first modulepath to write the interface file into.
    sprintf(t_path, "%s/%s.lci", ImportedModuleDir[0], p_name);

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
    FILE *stream;
    unsigned int index;
};

static FileRef s_files;
static FileRef s_current_file;
static unsigned int s_next_file_index;
static const char *s_template_file = NULL;
static const char *s_output_file = NULL;
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
    
    t_name = strrchr(p_filename, '/');
    if (t_name == NULL)
        t_name = p_filename;
    else
        t_name += 1;
    t_new_file -> name = strdup(t_name);
    if (t_new_file -> name == NULL)
        Fatal_OutOfMemory();
    
    t_new_file -> index = s_next_file_index++;
    
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

void SetOutputFile(const char *p_output)
{
    s_output_file = p_output;
}

void SetManifestOutputFile(const char *p_output)
{
    s_manifest_output_file = p_output;
}

void SetTemplateFile(const char *p_output)
{
    s_template_file = p_output;
}

FILE *OpenOutputFile(const char **r_filename)
{
    if (s_output_file == NULL)
        return NULL;

	if (NULL != r_filename)
	{
		*r_filename = s_output_file;
	}

    return fopen(s_output_file, "wb");
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
