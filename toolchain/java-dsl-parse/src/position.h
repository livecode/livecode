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

#ifndef __POSITION__
#define __POSITION__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef long PositionRef;
typedef struct File *FileRef;
    
void InitializePosition(void);
void FinalizePosition(void);

void AdvanceCurrentPosition(long delta);
void AdvanceCurrentPositionToNextRow(void);
void AdvanceCurrentPositionToFile(FileRef file);

void GetColumnOfPosition(PositionRef position, long *r_column);
void GetRowOfPosition(PositionRef position, long *r_row);
void GetFileOfPosition(PositionRef position, FileRef *r_file);
void GetFilenameOfPosition(PositionRef position, const char **r_filename);

void GetCurrentPosition(PositionRef *r_result);
void yyGetPos(PositionRef *r_result);

void InitializeFiles(void);
void FinalizeFiles(void);

void AddFile(const char *filename);
int MoveToNextFile(void);
void GetFilePath(FileRef file, const char **r_path);
void GetFileName(FileRef file, const char **r_name);
void GetFileIndex(FileRef file, long *r_index);
int GetFileWithIndex(long index, FileRef *r_file);
int GetCurrentFile(FileRef *r_file);

void SetOutputFile(const char *filename);
void GetOutputFile(const char **r_filename);
    
void SetOutputLCBFile(const char *filename);
void GetOutputLCBFile(const char **r_filename);
    
FILE *OpenOutputFile(void);
FILE *OpenLCBOutputFile(void);

#ifdef __cplusplus
}
#endif

#endif
