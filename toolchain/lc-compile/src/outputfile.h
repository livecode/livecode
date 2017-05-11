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

#ifndef __OUTPUTFILE__
#define __OUTPUTFILE__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef intptr_t PositionRef;
typedef struct File *FileRef;

enum DependencyModeType
{
    kDependencyModeNone,
    kDependencyModeOrder,
    kDependencyModeChangedOrder,
    kDependencyModeMake
};
    
void AddImportedModuleDir(const char *dir);
void AddImportedModuleName(const char *name);
int AddImportedModuleFile(const char *name);
    
void SetOutputBytecodeFile(const char *filename);
void SetOutputCodeFile(const char *filename);
void SetOutputGrammarFile(const char *filename);
void SetManifestOutputFile(const char *filename);
void SetInterfaceOutputFile(const char *filename);
void SetTemplateFile(const char *filename);
void GetOutputFile(const char **r_filename);
    
FILE *OpenOutputBytecodeFile(const char **r_filename);
FILE *OpenOutputGrammarFile(const char **r_filename);
FILE *OpenOutputCodeFile(const char **r_filename);
FILE *OpenManifestOutputFile(void);
FILE *OpenTemplateFile(void);
FILE *OpenImportedModuleFile(const char *module, char **r_filename);
    void FindImportedModuleFile(const char *p_name, char** r_module_file);

#ifdef __cplusplus
}
#endif

#endif
