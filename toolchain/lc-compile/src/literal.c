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
    for(BindingRef t_binding = p_scope -> bindings; t_binding != NULL; t_binding = t_binding -> next)
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
    if (s_scopes == NULL)
        Fatal_InternalInconsistency("Scope stack underflow");
    
    ScopeRef t_scope;
    t_scope = s_scopes;
    s_scopes = s_scopes -> outer;
    
    FreeScope(t_scope);
}

void DefineMeaning(NameRef p_name, long p_meaning)
{
    if (s_scopes == NULL)
        Fatal_InternalInconsistency("No scope when manipulating meaning");
    
    BindingRef t_binding;
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
    if (s_scopes == NULL)
        Fatal_InternalInconsistency("No scope when manipulating meaning");
    
    BindingRef t_binding;
    if (FindNameInScope(s_scopes, (NameRef)p_name, &t_binding) == 1)
    {
        t_binding -> meaning = 0;
        t_binding -> has_meaning = 0;
    }
}

int HasLocalMeaning(NameRef p_name, long *r_meaning)
{
    if (s_scopes == NULL)
        Fatal_InternalInconsistency("No scope when manipulating meaning");
    
    BindingRef t_binding;
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
    if (s_scopes == NULL)
        Fatal_InternalInconsistency("No scope when checking for meaning");
    
    for(ScopeRef t_scope = s_scopes; t_scope != NULL; t_scope = t_scope -> outer)
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
    t_depth = 0;
    for(ScopeRef t_scope = s_scopes; t_scope != NULL; t_scope = t_scope -> outer)
    {
        for(BindingRef t_binding = t_scope -> bindings; t_binding != NULL; t_binding = t_binding -> next)
            if (t_binding -> has_meaning)
                fprintf(stderr, "[%d] %s = %ld\n", t_depth, t_binding -> name -> token, t_binding -> meaning);
        t_depth += 1;
    }
}

////////////////////////////////////////////////////////////////////////////////
