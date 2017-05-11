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

#include <position.h>
#include "report.h"

////////////////////////////////////////////////////////////////////////////////

#define MAXPATHLEN 4096

static const char *ImportedModuleDir[8];
static int ImportedModuleDirCount = 0;
static const char *s_interface_output_file = NULL;
static const char *ImportedModuleNames[4096];
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

static const char *s_template_file = NULL;
static const char *s_output_bytecode_file = NULL;
static const char *s_output_code_file = NULL;
static const char *s_output_grammar_file = NULL;
static const char *s_manifest_output_file = NULL;

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
