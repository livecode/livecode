#include <stdio.h>

#include "report.h"

typedef unsigned int SetRef;

#define MAX_SET_DEPTH 256
SetRef s_sets[MAX_SET_DEPTH];
unsigned int s_set_count = 0;

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

static void EnsureSetIndexInRange(long p_index)
{
    if (p_index < 0 || p_index > 31)
        Fatal_InternalInconsistency("Index out of range for set operation");
}

void PushEmptySet(void)
{
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

int IsIndexInSet(long p_index)
{
    EnsureSetStackDepth(1);
    EnsureSetIndexInRange(p_index);
    
    if ((s_sets[s_set_count - 1] & (1 << p_index)) != 0)
        return 1;
    
    return 0;
}

void IncludeIndexInSet(long p_index)
{
    EnsureSetStackDepth(1);
    EnsureSetIndexInRange(p_index);
    
    s_sets[s_set_count - 1] |= (1 << p_index);
}

void ExcludeIndexFromSet(long p_index)
{
    EnsureSetStackDepth(1);
    EnsureSetIndexInRange(p_index);
    
    s_sets[s_set_count - 1] &= ~(1 << p_index);
}