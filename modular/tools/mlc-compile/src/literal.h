#ifndef __LITERAL__
#define __LITERAL__

void InitializeLiterals(void);
void FinalizeLiterals(void);

void MakeIntegerLiteral(const char *token, long *r_literal);
void MakeDoubleLiteral(const char *token, long *r_literal);
void MakeStringLiteral(const char *token, long *r_literal);
void MakeNameLiteral(const char *token, long *r_literal);

void GetStringOfNameLiteral(long literal, const char** r_string);

void InitializeScopes(void);
void FinalizeScopes(void);

void EnterScope(void);
void LeaveScope(void);

void DefineMeaning(long name, long meaning);
void UndefineMeaning(long name);
int HasLocalMeaning(long name, long *r_meaning);
int HasMeaning(long name, long *r_meaning);

#endif
