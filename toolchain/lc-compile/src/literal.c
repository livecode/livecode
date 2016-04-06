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

#include "literal.h"
#include "report.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
#  define strcasecmp _stricmp
#endif

////////////////////////////////////////////////////////////////////////////////

// typedef struct Name *NameRef;
struct Name
{
    NameRef next;
    NameRef key;
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

// Integer literals are actually unsigned, even though they pass through as
// longs. Indeed, a -ve integer literal is not possible since it is prevented
// by the token's regex.
int MakeIntegerLiteral(const char *p_token, long *r_literal)
{
    unsigned long t_value;
	errno = 0;

    t_value = strtoul(p_token, NULL, 10);
    
    if (errno == ERANGE || t_value > 0xFFFFFFFFU)
        return 0;
    
    *r_literal = (long)t_value;
    
    return 1;
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

static int char_to_nibble(char p_char, unsigned int *r_nibble)
{
    if (isdigit(p_char))
        *r_nibble = p_char - '0';
    else if (p_char >= 'a' && p_char <= 'f')
        *r_nibble = (p_char - 'a') + 10;
    else if (p_char >= 'A' && p_char <= 'F')
        *r_nibble = (p_char - 'A') + 10;
    else
        return 0;
    
    return 1;
}

void append_utf8_char(char *p_string, int *x_index, int p_char)
{
	// If the char is NUL (i.e. U+0000) we can't represent it directly
	// in a nul-terminated string.  However, we *can* use a 2-byte
	// overlong encoding to represent it safely (i.e. "Modified
	// UTF-8")
	if (p_char == 0)
	{
		p_string[*x_index]     = 0xc0;
		p_string[*x_index + 1] = 0x80;
		(*x_index) += 2;
	}
	else if (p_char < 128)
    {
        p_string[*x_index] = p_char;
        (*x_index) += 1;
    }
    else if (p_char >= 0x10000)
    {
        p_string[*x_index] = 0xf0 | (p_char >> 18);
        p_string[*x_index + 1] = 0x80 | ((p_char >> 12) & 0x3f);
        p_string[*x_index + 2] = 0x80 | ((p_char >> 6) & 0x3f);
        p_string[*x_index + 3] = 0x80 | ((p_char >> 0) & 0x3f);
        (*x_index) += 4;
    }
    else if (p_char >= 0x0800)
    {
        p_string[*x_index] = 0xe0 | (p_char >> 12);
        p_string[*x_index + 1] = 0x80 | ((p_char >> 6) & 0x3f);
        p_string[*x_index + 2] = 0x80 | ((p_char >> 0) & 0x3f);
        (*x_index) += 3;
    }
    else
    {
        p_string[*x_index] = 0xc0 | (p_char >> 6);
        p_string[*x_index + 1] = 0x80 | ((p_char >> 0) & 0x3f);
        (*x_index) += 2;
    }
}

int UnescapeStringLiteral(long p_position, const char *p_string, long *r_unescaped_string)
{
    // Allocate enough room for the length of the string including a NUL char.
    // This is more than enough to handle any escapes as escaped chars are always
    // greater in length than their unescaped versions.
    char *t_value;
    const char *t_limit;
    int t_length;
    const char *t_ptr;
    
    t_value = (char *)malloc(strlen(p_string) + 1);
    
    t_limit = p_string + strlen(p_string);
    t_length = 0;
    
    for(t_ptr = p_string; t_ptr < t_limit; t_ptr++)
    {
        if (*t_ptr == '\\')
        {
            // Record the start of the escape.
            const char *t_escape;
            t_escape = t_ptr;
            
            if (t_ptr + 1 < t_limit)
            {
                t_ptr += 1;
                
                if (*t_ptr == 'q')
                    t_value[t_length++] = '"';
                else if (*t_ptr == 'n')
                    t_value[t_length++] = '\n';
                else if (*t_ptr == 'r')
                    t_value[t_length++] = '\r';
                else if (*t_ptr == 't')
                    t_value[t_length++] = '\t';
                else if (*t_ptr == '\\')
                    t_value[t_length++] = '\\';
                else if (*t_ptr == 'u')
                {
                    // We expect the form:
                    //   \u{ABCDEF}
                    // With BCDEF all optional.
                    t_ptr += 1;
                    if (t_ptr < t_limit && *t_ptr == '{')
                    {
                        int t_overflow;
                        unsigned int t_char;
                        t_char = 0;
                        t_overflow = 0;
                        for(;;)
                        {
                            unsigned int t_nibble;
							
							// Advance the input ptr - if we are at the end here
                            // it is an error.
                            t_ptr += 1;
                            if (t_ptr >= t_limit)
                                goto error_exit;
                            
                            // If we get a } we are done, unless we haven't seen
                            // a nibble yet, in which case it is an error.
                            if (*t_ptr == '}')
                            {
                                if (t_ptr == t_escape + 3)
                                    Warning_EmptyUnicodeEscape(p_position + (t_escape - p_string));
                                break;
                            }
                            
                            // Parse the next nibble, shift and add it.
                            if (!char_to_nibble(*t_ptr, &t_nibble))
                                goto error_exit;
                            
                            t_char = t_char << 4;
                            t_char |= t_nibble;
                            
                            if (t_char > 0x10FFFF)
                                t_overflow = 1;
                        }
                        
                        // If we get here and we are not looking at } then it
                        // is an error.
                        if (*t_ptr != '}')
                            goto error_exit;
                        
                        if (t_overflow)
                        {
                            Warning_UnicodeEscapeTooBig(p_position + (t_escape - p_string));
                            t_char = 0xFFFD;
                        }
                        
                        append_utf8_char(t_value, &t_length, t_char);
                    }
                    else
                        goto error_exit;
                }
                else
                    goto error_exit;
            }
        }
        else
            t_value[t_length++] = *t_ptr;
    }
    
    t_value[t_length++] = '\0';
    
    *r_unescaped_string = (long)t_value;

    return 1;
    
error_exit:
    free(t_value);
    return 0;
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

void MakeNameLiteralN(const char *p_token, int p_token_length, NameRef *r_literal)
{
    NameRef t_name;
    NameRef t_key;
    t_name = NULL;
    t_key = NULL;
    for(t_name = s_names; t_name != NULL; t_name = t_name -> next)
    {
        if (strcmp(p_token, t_name -> token) == 0)
            break;
        
        // We want the last name in the list which matches case-insensitively.
        if (strcasecmp(p_token, t_name -> token) == 0)
            t_key = t_name;
    }
    
    if (t_name == NULL)
    {
        t_name = (NameRef)malloc(sizeof(struct Name));
        if (t_name == NULL)
            Fatal_OutOfMemory();
        
        t_name -> token = malloc(p_token_length + 1);
        if (t_name -> token == NULL)
            Fatal_OutOfMemory();
        memcpy(t_name -> token, p_token, p_token_length);
        t_name -> token[p_token_length] = '\0';
        
        // If we found a key (one which matches this case-insensitively) then
        // that is the key of this name. Otherwise the name is its own key.
        if (t_key != NULL)
            t_name -> key = t_key;
        else
            t_name -> key = t_name;
        
        t_name -> next = s_names;
        s_names = t_name;
    }
    
    *r_literal = t_name;
}

void MakeNameLiteral(const char *p_token, NameRef *r_literal)
{
    MakeNameLiteralN(p_token, strlen(p_token), r_literal);
}

void GetStringOfNameLiteral(NameRef p_literal, const char **r_string)
{
    *r_string = ((NameRef)p_literal) -> token;
}

int IsNameEqualToName(NameRef p_left, NameRef p_right)
{
    return p_left == p_right || p_left -> key == p_right -> key;
}

int IsNameNotEqualToName(NameRef p_left, NameRef p_right)
{
    return IsNameEqualToName(p_left, p_right) == 0 ? 1 : 0;
}

int IsNameEqualToString(NameRef p_name, const char *p_string)
{
    return strcasecmp(p_name -> token, p_string) == 0;
}

int IsStringEqualToString(const char *p_left, const char *p_right)
{
    return strcmp(p_left, p_right) == 0;
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
        if (IsNameEqualToName(t_binding -> name, p_name))
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

/* Check that module names contain a '.' character. */
int
IsNameSuitableForModule (NameRef p_id)
{
	const char *t_id;
	size_t i;

	GetStringOfNameLiteral (p_id, &t_id);

	for (i = 0; '\0' != t_id[i]; ++i)
	{
		if ('.' == t_id[i])
			return 1;
	}

	return 0;
}

/* Check that defined names *don't* match [a-z]+ (because these names
 * are reserved for syntax keywords) */
int
IsNameSuitableForDefinition (NameRef p_id)
{
	const char *t_id;
	size_t i;

	GetStringOfNameLiteral (p_id, &t_id);

	for (i = 0; '\0' != t_id[i]; ++i)
	{
		/* If char is not in [a-z] then string is okay */
		if (t_id[i] < 'a' || t_id[i] > 'z')
			return 1;
	}

	return 0;
}

/* Keywords mustn't conflict with valid identifiers.  Ensure this by
 * forbidding keywords from containing any of the characters
 * [A-Z0-9_.] */
int
IsStringSuitableForKeyword (const char *p_keyword)
{
	size_t i;

	for (i = 0; p_keyword[i] != '\0'; ++i)
	{
		if ((p_keyword[i] == '.') ||
		    (p_keyword[i] == '_') ||
		    (p_keyword[i] >= 'A' && p_keyword[i] <= 'Z') ||
		    (p_keyword[i] >= '0' && p_keyword[i] <= '9'))
			return 0;
	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////
