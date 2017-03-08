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

#include <stdlib.h>
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////

static const char *s_output_file = NULL;
static const char *s_output_lcb_file = NULL;
static const char *s_output_lcb_module_name = NULL;

void SetOutputFile(const char *p_output)
{
    s_output_file = p_output;
}

void GetOutputFile(const char **r_output)
{
    *r_output = s_output_file;
}

void SetOutputLCBFile(const char *p_output)
{
    s_output_lcb_file = p_output;
}

void GetOutputLCBFile(const char **r_output)
{
    *r_output = s_output_lcb_file;
}

void SetOutputLCBModuleName(const char* p_module_name)
{
    s_output_lcb_module_name = p_module_name;
}

void GetOutputLCBModuleName(const char* *r_module_name)
{
    *r_module_name = s_output_lcb_module_name;
}

FILE *OpenOutputFile(void)
{
    if (s_output_file == NULL)
        return NULL;
    return fopen(s_output_file, "w");
}

FILE *OpenLCBOutputFile(void)
{
    if (s_output_lcb_file == NULL)
        return NULL;
    return fopen(s_output_lcb_file, "w");
}

////////////////////////////////////////////////////////////////////////////////
