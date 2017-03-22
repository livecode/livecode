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

#ifndef __ALLOCATE__
#define __ALLOCATE__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
	
void *Allocate(size_t p_size);
void *Reallocate(void *p_ptr, size_t p_new_size);
	
#ifdef __cplusplus
}
#endif

#endif
