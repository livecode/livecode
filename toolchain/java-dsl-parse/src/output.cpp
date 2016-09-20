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

#include "report.h"
#include "literal.h"
#include "position.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#ifndef WIN32
#include <unistd.h>
#endif


extern "C" void OutputBegin(void);
extern "C" void OutputLCBBegin(void);
extern "C" void OutputEnd(void);
extern "C" void OutputWrite(const char *msg);
extern "C" void OutputWriteI(const char *left, NameRef name, const char *right);
extern "C" void OutputWriteS(const char *left, const char *string, const char *right);
extern "C" void OutputWriteD(const char *left, double number, const char *right);
extern "C" void OutputWriteN(const char *left, int number, const char *right);

//////////

void *Allocate(size_t p_size)
{
    void *t_ptr;
    t_ptr = calloc(1, p_size);
    if (t_ptr == NULL)
        Fatal_OutOfMemory();
    return t_ptr;
}

void *Reallocate(void *p_ptr, size_t p_new_size)
{
    void *t_new_ptr;
    t_new_ptr = realloc(p_ptr, p_new_size);
    if (t_new_ptr == NULL)
        Fatal_OutOfMemory();
    return t_new_ptr;
}

static FILE *s_output = NULL;

void OutputBegin(void)
{
    s_output = OpenOutputFile();
}

void OutputLCBBegin(void)
{
    s_output = OpenLCBOutputFile();
}

void OutputWrite(const char *p_string)
{
    if (s_output == NULL)
        return;
    
    fprintf(s_output, "%s", p_string);
}

void OutputWriteS(const char *p_left, const char *p_string, const char *p_right)
{
    if (s_output == NULL)
        return;
    
    fprintf(s_output, "%s%s%s", p_left, p_string, p_right);
}

void OutputWriteI(const char *p_left, NameRef p_name, const char *p_right)
{
    if (s_output == NULL)
        return;
    
    const char *t_name_string;
    GetStringOfNameLiteral(p_name, &t_name_string);
    OutputWriteS(p_left, t_name_string, p_right);
}

void OutputWriteD(const char *p_left, double p_number, const char *p_right)
{
    if (s_output == NULL)
        return;
    
    fprintf(s_output, "%s%f%s", p_left, p_number, p_right);
}

void OutputWriteN(const char *p_left, int p_number, const char *p_right)
{
    if (s_output == NULL)
        return;
    
    fprintf(s_output, "%s%d%s", p_left, p_number, p_right);
}

void OutputEnd(void)
{
    if (s_output == NULL)
        return;
    
    fclose(s_output);
}

//////////
