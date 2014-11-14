#ifndef __LITERAL__
#define __LITERAL__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Name *NameRef;

void InitializeLiterals(void);
void FinalizeLiterals(void);

void MakeIntegerLiteral(const char *token, long *r_literal);
void MakeDoubleLiteral(const char *token, long *r_literal);
void MakeStringLiteral(const char *token, long *r_literal);
void MakeNameLiteral(const char *token, NameRef *r_literal);

void GetStringOfNameLiteral(NameRef literal, const char** r_string);

void InitializeScopes(void);
void FinalizeScopes(void);

void EnterScope(void);
void LeaveScope(void);

void DefineMeaning(NameRef name, long meaning);
void UndefineMeaning(NameRef name);
int HasLocalMeaning(NameRef name, long *r_meaning);
int HasMeaning(NameRef name, long *r_meaning);

#ifdef __cplusplus
}
#endif

#endif
