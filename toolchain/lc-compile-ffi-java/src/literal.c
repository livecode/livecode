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

#include "literal.h"
#include "report.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////

void JavaQualifiedNameToClassPath(NameRef p_input, NameRef *r_output)
{
    const char* t_input = NULL;
    char *t_output = NULL;
    unsigned long i = 0;
    
    GetStringOfNameLiteral(p_input, &t_input);
    
    t_output = malloc(strlen(t_input) + 1);
    if (t_output == NULL)
        Fatal_OutOfMemory();
    
    for (i = 0; i < strlen(t_input); i++) {
        if (t_input[i] == '.')
            t_output[i] = '/';
        else
            t_output[i] = t_input[i];
    }
    
    MakeNameLiteral(t_output, r_output);
    
    free(t_output);
}

////////////////////////////////////////////////////////////////////////////////
