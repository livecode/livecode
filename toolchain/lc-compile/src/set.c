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

#include <stdio.h>

#include "report.h"

typedef unsigned int SetRef;

#define MAX_SET_DEPTH 256
SetRef s_sets[MAX_SET_DEPTH];
int s_set_count = 0;

void InitializeSets(void)
{
}

void FinalizeSets(void)
{
}

static void EnsureRoomInSetStack(void)
{
    if (s_set_count == MAX_SET_DEPTH)
        Fatal_InternalInconsistency("Maximum depth of set stack reached");
}

static void EnsureSetStackDepth(int p_depth)
{
    if (s_set_count < p_depth)
        Fatal_InternalInconsistency("Incorrect depth of set stack for operation");
}

static void EnsureSetIndexInRange(intptr_t p_index)
{
    if (p_index < 0 || p_index > 31)
        Fatal_InternalInconsistency("Index out of range for set operation");
}

void PushEmptySet(void)
{
    // Pushing the empty set starts processing, so we reset the stack.
    s_set_count = 0;
    
    EnsureRoomInSetStack();
    
    s_sets[s_set_count++] = 0;
}

void DuplicateSet(void)
{
    EnsureSetStackDepth(1);
    
    s_sets[s_set_count] = s_sets[s_set_count - 1];
    s_set_count++;
}

void ExchangeSet(void)
{
    SetRef t_set;
	
	EnsureSetStackDepth(2);
    
    t_set = s_sets[s_set_count - 1];
    s_sets[s_set_count - 1] = s_sets[s_set_count - 2];
    s_sets[s_set_count - 2] = t_set;
}

void UnionSet(void)
{
    EnsureSetStackDepth(2);
 
    s_sets[s_set_count - 2] |= s_sets[s_set_count - 1];
    s_set_count -= 1;
}

int IsIndexInSet(intptr_t p_index)
{
    EnsureSetStackDepth(1);
    EnsureSetIndexInRange(p_index);
    
    if ((s_sets[s_set_count - 1] & (1 << p_index)) != 0)
        return 1;
    
    return 0;
}

void IncludeIndexInSet(intptr_t p_index)
{
    EnsureSetStackDepth(1);
    EnsureSetIndexInRange(p_index);
    
    s_sets[s_set_count - 1] |= (1 << p_index);
}

void ExcludeIndexFromSet(intptr_t p_index)
{
    EnsureSetStackDepth(1);
    EnsureSetIndexInRange(p_index);
    
    s_sets[s_set_count - 1] &= ~(1 << p_index);
}
