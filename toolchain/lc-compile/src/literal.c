/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

// typedef struct Name *NameRef;
struct Name
{
    NameRef next;
    char *token;
};

typedef struct Binding *BindingRef;
struct Binding
{
    BindingRef next;
    NameRef name;
    unsigned int has_meaning : 1;
    long meaning;
};

typedef struct Scope *ScopeRef;
struct Scope
{
    ScopeRef outer;
    BindingRef bindings;
};

static NameRef s_names;
static ScopeRef s_scopes;

////////////////////////////////////////////////////////////////////////////////

void InitializeLiterals(void)
{
    s_names = NULL;
}

void FinalizeLiterals(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MakeIntegerLiteral(const char *p_token, long *r_literal)
{
    *r_literal = atoi(p_token);
}

void MakeDoubleLiteral(const char *p_token, long *r_literal)
{
    double *t_value;
    t_value = (double *)malloc(sizeof(double));
    if (t_value == NULL)
        Fatal_OutOfMemory();
    *t_value = atof(p_token);
    *r_literal = (long)t_value;
}

void MakeStringLiteral(const char *p_token, long *r_literal)
{
    char *t_value;
    t_value = strdup(p_token + 1);
    if (t_value == NULL)
        Fatal_OutOfMemory();
    t_value[strlen(t_value) - 1] = '\0';
    *r_literal = (long)t_value;
}

void MakeNameLiteral(const char *p_token, NameRef *r_literal)
{
    NameRef t_name;
    for(t_name = s_names; t_name != NULL; t_name = t_name -> next)
        if (strcmp(p_token, t_name -> token) == 0)
            break;
    
    if (t_name == NULL)
    {
        t_name = (NameRef)malloc(sizeof(struct Name));
        if (t_name == NULL)
            Fatal_OutOfMemory();
        
        t_name -> token = strdup(p_token);
        if (t_name -> token == NULL)
            Fatal_OutOfMemory();
        
        t_name -> next = s_names;
        s_names = t_name;
    }
    
    *r_literal = t_name;
}

void GetStringOfNameLiteral(NameRef p_literal, const char **r_string)
{
    *r_string = ((NameRef)p_literal) -> token;
}

int IsNameEqualToString(NameRef p_name, const char *p_string)
{
    return strcmp(p_name -> token, p_string) == 0;
}

void NegateReal(long p_real, long *r_real)
{
    double *t_value;
    t_value = (double *)malloc(sizeof(double));
    if (t_value == NULL)
        Fatal_OutOfMemory();
    *t_value = -*(double *)p_real;
    *r_real = (long)t_value;
}

////////////////////////////////////////////////////////////////////////////////

static void FreeScope(ScopeRef p_scope)
{
    while(p_scope -> bindings != NULL)
    {
        BindingRef t_binding;
        t_binding = p_scope -> bindings;
        p_scope -> bindings = p_scope -> bindings -> next;
        
        free(t_binding);
    }
    
    free(p_scope);
}

static int FindNameInScope(ScopeRef p_scope, NameRef p_name, BindingRef *r_binding)
{
    BindingRef t_binding;

	for (t_binding = p_scope -> bindings; t_binding != NULL; t_binding = t_binding -> next)
        if (t_binding -> name == p_name)
        {
            *r_binding = t_binding;
            return 1;
        }
    return 0;
}

void InitializeScopes(void)
{
    s_scopes = NULL;
}

void FinalizeScopes(void)
{
}

void EnterScope(void)
{
    ScopeRef t_new_scope;
    t_new_scope = (ScopeRef)malloc(sizeof(struct Scope));
    if (t_new_scope == NULL)
        Fatal_OutOfMemory();
    
    t_new_scope -> outer = s_scopes;
    t_new_scope -> bindings = NULL;
    s_scopes = t_new_scope;
}

void LeaveScope(void)
{
    ScopeRef t_scope;

	if (s_scopes == NULL)
        Fatal_InternalInconsistency("Scope stack underflow");
    
    t_scope = s_scopes;
    s_scopes = s_scopes -> outer;
    
    FreeScope(t_scope);
}

void DefineMeaning(NameRef p_name, long p_meaning)
{
    BindingRef t_binding;
	
	if (s_scopes == NULL)
        Fatal_InternalInconsistency("No scope when manipulating meaning");
    
    if (FindNameInScope(s_scopes, (NameRef)p_name, &t_binding) == 0)
    {
        t_binding = (BindingRef)malloc(sizeof(struct Binding));
        if (t_binding == NULL)
            Fatal_OutOfMemory();
        
        t_binding -> next = s_scopes -> bindings;
        t_binding -> name = (NameRef)p_name;
        
        s_scopes -> bindings = t_binding;
    }
    
    t_binding -> meaning = p_meaning;
    t_binding -> has_meaning = 1;
}

void UndefineMeaning(NameRef p_name)
{
    BindingRef t_binding;
	
	if (s_scopes == NULL)
        Fatal_InternalInconsistency("No scope when manipulating meaning");
    
    if (FindNameInScope(s_scopes, (NameRef)p_name, &t_binding) == 1)
    {
        t_binding -> meaning = 0;
        t_binding -> has_meaning = 0;
    }
}

int HasLocalMeaning(NameRef p_name, long *r_meaning)
{
    BindingRef t_binding;
	
	if (s_scopes == NULL)
        Fatal_InternalInconsistency("No scope when manipulating meaning");

    if (FindNameInScope(s_scopes, (NameRef)p_name, &t_binding) == 1 &&
        t_binding -> has_meaning == 1)
    {
        *r_meaning = t_binding -> meaning;
        return 1;
    }
    
    return 0;
}

int HasMeaning(NameRef p_name, long *r_meaning)
{
    ScopeRef t_scope;
	
	if (s_scopes == NULL)
        Fatal_InternalInconsistency("No scope when checking for meaning");
    
    for(t_scope = s_scopes; t_scope != NULL; t_scope = t_scope -> outer)
    {
        BindingRef t_binding;
        if (FindNameInScope(t_scope, (NameRef)p_name, &t_binding) == 1 &&
            t_binding -> has_meaning == 1)
        {
            *r_meaning = t_binding -> meaning;
            return 1;
        }
    }
    
    return 0;
}

void DumpScopes(void)
{
    int t_depth;
	ScopeRef t_scope;

    t_depth = 0;
    for (t_scope = s_scopes; t_scope != NULL; t_scope = t_scope -> outer)
    {
        BindingRef t_binding;

		for (t_binding = t_scope -> bindings; t_binding != NULL; t_binding = t_binding -> next)
            if (t_binding -> has_meaning)
                fprintf(stderr, "[%d] %s = %ld\n", t_depth, t_binding -> name -> token, t_binding -> meaning);
        t_depth += 1;
    }
}

////////////////////////////////////////////////////////////////////////////////
