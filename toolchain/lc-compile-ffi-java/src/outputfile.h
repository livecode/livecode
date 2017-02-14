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

#ifndef __OUTPUTFILE__
#define __OUTPUTFILE__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void SetOutputFile(const char *filename);
void GetOutputFile(const char **r_filename);
    
void SetOutputLCBFile(const char *filename);
void GetOutputLCBFile(const char **r_filename);
    
void SetOutputLCBModuleName(const char* p_module_name);
void GetOutputLCBModuleName(const char* *r_module_name);
    
FILE *OpenOutputFile(void);
FILE *OpenLCBOutputFile(void);

#ifdef __cplusplus
}
#endif

#endif
