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

#ifndef __LITERAL__
#define __LITERAL__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Name *NameRef;

void InitializeLiterals(void);
void FinalizeLiterals(void);

int IsDoubleLiteral(const char *token);
int IsIntegerLiteral(const char *token);
int MakeIntegerLiteral(const char *token, intptr_t *r_literal);
int MakeDoubleLiteral(const char *token, intptr_t *r_literal);
void MakeStringLiteral(const char *token, intptr_t *r_literal);
void MakeNameLiteral(const char *token, NameRef *r_literal);
void MakeNameLiteralN(const char *p_token, intptr_t p_token_length, NameRef *r_literal);
int IsNameEqualToName(NameRef p_left, NameRef p_right);
    
void GetStringOfNameLiteral(NameRef literal, const char** r_string);

void InitializeScopes(void);
void FinalizeScopes(void);

void EnterScope(void);
void LeaveScope(void);

void DefineMeaning(NameRef name, NameRef p_namespace, intptr_t meaning);
void DefineUnqualifiedMeaning(NameRef name, intptr_t meaning);
void UndefineMeaning(NameRef name, NameRef p_namespace);
int HasLocalMeaning(NameRef name, intptr_t *r_meaning);
int HasMeaning(NameRef name, NameRef p_namespace, intptr_t *r_meaning);
int HasUnqualifiedMeaning(NameRef name, intptr_t *r_meaning);

#ifdef __cplusplus
}
#endif

#endif
