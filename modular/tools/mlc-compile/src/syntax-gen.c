#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "report.h"
#include "literal.h"

#define MAX_NODE_DEPTH 32

typedef enum SyntaxNodeKind SyntaxNodeKind;
enum SyntaxNodeKind
{
    kSyntaxNodeKindEmpty,
    kSyntaxNodeKindAlternate,
    kSyntaxNodeKindConcatenate,
    kSyntaxNodeKindRepeat,
    kSyntaxNodeKindKeyword,
    kSyntaxNodeKindDescent,
    kSyntaxNodeKindBooleanMark,
    kSyntaxNodeKindIntegerMark,
    kSyntaxNodeKindStringMark,
};

typedef struct SyntaxNode *SyntaxNodeRef;
struct SyntaxNode
{
    SyntaxNodeKind kind;
    union
    {
        struct
        {
            SyntaxNodeRef *operands;
            unsigned int operand_count;
            int is_nullable;
        } alternate, concatenate;
        struct
        {
            SyntaxNodeRef element;
            SyntaxNodeRef delimiter;
        } repeat;
        struct
        {
            NameRef token;
        } keyword;
        struct
        {
            long index;
            NameRef rule;
        } descent;
        struct
        {
            long index;
            long value;
        } boolean_mark, integer_mark;
        struct
        {
            long index;
            NameRef value;
        } string_mark;
    };
};

typedef struct SyntaxMethod *SyntaxMethodRef;
struct SyntaxMethod
{
    NameRef name;
    struct SyntaxArgument *arguments;
    unsigned int argument_count;
};

typedef enum SyntaxRuleKind SyntaxRuleKind;
enum SyntaxRuleKind
{
    kSyntaxRuleKindStatement,
    kSyntaxRuleKindExpression,
    kSyntaxRuleKindLeftUnaryOperator,
    kSyntaxRuleKindRightUnaryOperator,
    kSyntaxRuleKindLeftBinaryOperator,
    kSyntaxRuleKindRightBinaryOperator,
    kSyntaxRuleKindNeutralBinaryOperator,
};

typedef struct SyntaxRule *SyntaxRuleRef;
struct SyntaxRule
{
    NameRef name;
    SyntaxRuleKind kind;
    long precedence;
    SyntaxNodeRef expr;
    SyntaxMethodRef methods;
};

static SyntaxRuleRef s_rule = NULL;
static SyntaxNodeRef s_stack[MAX_NODE_DEPTH];
static unsigned int s_stack_index = 0;

////////////////////////////////////////////////////////////////////////////////

static void *Allocate(size_t p_size)
{
    void *t_ptr;
    t_ptr = malloc(p_size);
    if (t_ptr == NULL)
        Fatal_OutOfMemory();
    return t_ptr;
}

static void *Reallocate(void *p_ptr, size_t p_new_size)
{
    void *t_new_ptr;
    t_new_ptr = realloc(p_ptr, p_new_size);
    if (t_new_ptr == NULL)
        Fatal_OutOfMemory();
    return t_new_ptr;
}

////////////////////////////////////////////////////////////////////////////////

static void BeginSyntaxRule(NameRef p_name, SyntaxRuleKind p_kind, long p_precedence)
{
    assert(s_rule == NULL);
    
    s_rule = (SyntaxRuleRef)Allocate(sizeof(struct SyntaxRule));
    
    s_rule -> name = p_name;
    s_rule -> kind = p_kind;
    s_rule -> precedence = p_precedence;
    s_rule -> expr = NULL;
    s_rule -> methods = NULL;
}

void BeginStatementSyntaxRule(NameRef p_name)
{
    BeginSyntaxRule(p_name, kSyntaxRuleKindStatement, 0);
}

void BeginExpressionSyntaxRule(NameRef p_name)
{
    BeginSyntaxRule(p_name, kSyntaxRuleKindExpression, 0);
}

void BeginLeftUnaryOperatorSyntaxRule(NameRef p_name, long p_precedence)
{
    BeginSyntaxRule(p_name, kSyntaxRuleKindLeftUnaryOperator, p_precedence);
}

void BeginRightUnaryOperatorSyntaxRule(NameRef p_name, long p_precedence)
{
    BeginSyntaxRule(p_name, kSyntaxRuleKindRightUnaryOperator, p_precedence);
}

void BeginLeftBinaryOperatorSyntaxRule(NameRef p_name, long p_precedence)
{
    BeginSyntaxRule(p_name, kSyntaxRuleKindLeftBinaryOperator, p_precedence);
}

void BeginRightBinaryOperatorSyntaxRule(NameRef p_name, long p_precedence)
{
    BeginSyntaxRule(p_name, kSyntaxRuleKindRightBinaryOperator, p_precedence);
}

void BeginNeutralBinaryOperatorSyntaxRule(NameRef p_name, long p_precedence)
{
    BeginSyntaxRule(p_name, kSyntaxRuleKindNeutralBinaryOperator, p_precedence);
}

void EndSyntaxRule(void)
{
    assert(s_rule != NULL);
    s_rule = NULL;
}

////////////////////////////////////////////////////////////////////////////////

static void MakeSyntaxNode(SyntaxNodeRef *r_node)
{
    SyntaxNodeRef t_node;
    t_node = (SyntaxNodeRef)Allocate(sizeof(struct SyntaxNode));
    *r_node = t_node;
}

static void PushSyntaxNode(SyntaxNodeRef p_node)
{
    assert(s_rule != NULL);
    assert(s_stack_index < MAX_NODE_DEPTH);
    
    s_stack[s_stack_index++] = p_node;
}

static void FreeSyntaxNode(SyntaxNodeRef p_node)
{
    switch(p_node -> kind)
    {
        case kSyntaxNodeKindEmpty:
        case kSyntaxNodeKindKeyword:
        case kSyntaxNodeKindDescent:
        case kSyntaxNodeKindBooleanMark:
        case kSyntaxNodeKindIntegerMark:
        case kSyntaxNodeKindStringMark:
            break;
        case kSyntaxNodeKindConcatenate:
        case kSyntaxNodeKindAlternate:
            for(int i = 0; i < p_node -> concatenate . operand_count; i++)
                FreeSyntaxNode(p_node -> concatenate . operands[i]);
            free(p_node -> concatenate . operands);
            break;
        case kSyntaxNodeKindRepeat:
            FreeSyntaxNode(p_node -> repeat . element);
            FreeSyntaxNode(p_node -> repeat . delimiter);
            break;
    }
    free(p_node);
}

static void AppendSyntaxNode(SyntaxNodeRef p_target, SyntaxNodeRef p_node)
{
    assert(p_target -> kind == kSyntaxNodeKindConcatenate || p_target -> kind == kSyntaxNodeKindAlternate);
    assert(p_node -> kind != p_target -> kind);
    
    if (p_target -> kind == kSyntaxNodeKindConcatenate && p_node == kSyntaxNodeKindEmpty)
        return;
    
    if (p_target -> kind == kSyntaxNodeKindAlternate && p_node == kSyntaxNodeKindEmpty)
    {
        p_target -> alternate . is_nullable = 1;
        return;
    }
    
    int t_new_count;
    t_new_count = p_target -> concatenate . operand_count + 1;
    p_target -> concatenate . operands = (SyntaxNodeRef *)Reallocate(p_target -> concatenate . operands, t_new_count * sizeof(SyntaxNodeRef));
    p_target -> concatenate . operands[p_target -> concatenate . operand_count++] = p_node;
}

static void PrependSyntaxNode(SyntaxNodeRef p_target, SyntaxNodeRef p_node)
{
    assert(p_target -> kind == kSyntaxNodeKindConcatenate || p_target -> kind == kSyntaxNodeKindAlternate);
    assert(p_node -> kind != p_target -> kind);
    
    if (p_target -> kind == kSyntaxNodeKindConcatenate && p_node == kSyntaxNodeKindEmpty)
        return;
    
    if (p_target -> kind == kSyntaxNodeKindAlternate && p_node == kSyntaxNodeKindEmpty)
    {
        p_target -> alternate . is_nullable = 1;
        return;
    }
    
    int t_new_count;
    t_new_count = p_target -> concatenate . operand_count + 1;
    p_target -> concatenate . operands = (SyntaxNodeRef *)Reallocate(p_target -> concatenate . operands, t_new_count * sizeof(SyntaxNodeRef));
    for(int i = t_new_count; i > 1; i--)
        p_target -> concatenate . operands[i - 1] = p_target -> concatenate . operands[i - 2];
    p_target -> concatenate . operands[0] = p_node;
    p_target -> concatenate . operand_count++;
}

static void MergeSyntaxNodes(SyntaxNodeRef p_left, SyntaxNodeRef p_right)
{
    assert(p_left -> kind == kSyntaxNodeKindConcatenate || p_left -> kind == kSyntaxNodeKindAlternate);
    assert(p_left -> kind == p_right -> kind);
    
    for(int i = 0; i < p_right -> concatenate . operand_count; i++)
        AppendSyntaxNode(p_left, p_right -> concatenate . operands[i]);
    p_right -> concatenate . operand_count = 0;
    
    if (p_right -> concatenate . is_nullable == 1)
        p_left -> concatenate . is_nullable = 1;
    
    /*int t_new_count;
     t_new_count = p_left -> concatenate . operand_count + p_right -> concatenate . operand_count;
     p_left -> concatenate . operands = (SyntaxNodeRef *)Reallocate(p_left -> concatenate . operands, t_new_count * sizeof(SyntaxNodeRef));
     for(int i = 0; i < p_right -> concatenate . operand_count; i++)
     p_left -> concatenate . operands[p_left -> concatenate . operand_count++] = p_right -> concatenate . operands[i];
     p_right -> concatenate . operand_count = 0;*/
    FreeSyntaxNode(p_right);
}

static void PrintSyntaxNode(SyntaxNodeRef p_node)
{
    switch(p_node -> kind)
    {
        case kSyntaxNodeKindEmpty:
            printf("_");
            break;
        case kSyntaxNodeKindKeyword:
        {
            const char *t_string;
            GetStringOfNameLiteral(p_node -> keyword . token, &t_string);
            printf("'%s'", t_string);
        }
        break;
        case kSyntaxNodeKindDescent:
        {
            const char *t_string;
            GetStringOfNameLiteral(p_node -> descent . rule, &t_string);
            printf("<%ld:%s>", p_node -> descent . index, t_string);
        }
        break;
        case kSyntaxNodeKindBooleanMark:
        {
            printf("<%ld=%s>", p_node -> boolean_mark . index, p_node -> boolean_mark . value ? "true" : "false");
        }
        break;
        case kSyntaxNodeKindIntegerMark:
        {
            printf("<%ld=%ld>", p_node -> boolean_mark . index, p_node -> integer_mark . value);
        }
        break;
        case kSyntaxNodeKindStringMark:
        {
            const char *t_string;
            GetStringOfNameLiteral(p_node -> string_mark . value, &t_string);
            printf("<%ld=%s>", p_node -> boolean_mark . index, t_string);
        }
        break;
        case kSyntaxNodeKindConcatenate:
            printf("(");
            for(int i = 0; i < p_node -> concatenate . operand_count; i++)
            {
                if (i > 0)
                    printf(".");
                PrintSyntaxNode(p_node -> concatenate . operands[i]);
            }
            printf(")");
        break;
        case kSyntaxNodeKindAlternate:
            printf(p_node -> alternate . is_nullable ? "[" : "(");
            for(int i = 0; i < p_node -> concatenate . operand_count; i++)
            {
                if (i > 0)
                    printf("|");
                PrintSyntaxNode(p_node -> concatenate . operands[i]);
            }
            printf(p_node -> alternate . is_nullable ? "]" : ")");
            break;
        case kSyntaxNodeKindRepeat:
            printf("{");
            PrintSyntaxNode(p_node -> repeat . element);
            printf(",");
            PrintSyntaxNode(p_node -> repeat . delimiter);
            printf("}");
            break;
    }
}

void BeginSyntaxGrammar(void)
{
    assert(s_rule != NULL);
    assert(s_stack_index == 0);
}

void EndSyntaxGrammar(void)
{
    assert(s_rule != NULL);
    assert(s_stack_index == 1);
    
    PrintSyntaxNode(s_stack[0]);
    printf("\n");
    
    FreeSyntaxNode(s_stack[0]);
    s_stack_index = 0;
}

static void JoinSyntaxGrammar(SyntaxNodeKind p_kind)
{
    assert(s_rule != NULL);
    assert(s_stack_index >= 2);
    
    SyntaxNodeRef t_left, t_right;
    t_left = s_stack[s_stack_index - 2];
    t_right = s_stack[s_stack_index - 1];
    
    SyntaxNodeRef t_result;
    if (t_left -> kind == p_kind && t_right -> kind == p_kind)
    {
        MergeSyntaxNodes(t_left, t_right);
        t_result = t_left;
    }
    else if (t_left -> kind == p_kind)
    {
        AppendSyntaxNode(t_left, t_right);
        t_result = t_left;
    }
    else if (t_right -> kind == p_kind)
    {
        PrependSyntaxNode(t_right, t_left);
        t_result = t_right;
    }
    else
    {
        MakeSyntaxNode(&t_result);
        t_result -> kind = p_kind;
        t_result -> concatenate . operands = NULL;
        t_result -> concatenate . operand_count = 0;
        t_result -> concatenate . is_nullable = 0;
        AppendSyntaxNode(t_result, t_left);
        AppendSyntaxNode(t_result, t_right);
    }
    
    s_stack[s_stack_index - 2] = t_result;
    s_stack_index -= 1;
}

void ConcatenateSyntaxGrammar(void)
{
    JoinSyntaxGrammar(kSyntaxNodeKindConcatenate);
}

void AlternateSyntaxGrammar(void)
{
    JoinSyntaxGrammar(kSyntaxNodeKindAlternate);
}

void RepeatSyntaxGrammar(void)
{
    assert(s_rule != NULL);
    assert(s_stack_index >= 2);
    
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindRepeat;
    t_node -> repeat . element = s_stack[s_stack_index - 2];
    t_node -> repeat . delimiter = s_stack[s_stack_index - 1];
    s_stack[s_stack_index - 2] = t_node;
    s_stack_index -= 1;
}

void PushEmptySyntaxGrammar(void)
{
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindEmpty;
    PushSyntaxNode(t_node);
}

void PushKeywordSyntaxGrammar(const char *p_keyword)
{
    NameRef t_name;
    MakeNameLiteral(p_keyword, &t_name);
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindKeyword;
    t_node -> keyword . token = t_name;
    PushSyntaxNode(t_node);
}

void PushMarkedDescentSyntaxGrammar(long p_mark, NameRef p_rule)
{
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindDescent;
    t_node -> descent . index = p_mark;
    t_node -> descent . rule = p_rule;
    PushSyntaxNode(t_node);
}

void PushDescentSyntaxGrammar(NameRef p_rule)
{
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindDescent;
    t_node -> descent . index = -1;
    t_node -> descent . rule = p_rule;
    PushSyntaxNode(t_node);
}

void PushMarkedTrueSyntaxGrammar(long p_mark)
{
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindBooleanMark;
    t_node -> boolean_mark . index = p_mark;
    t_node -> boolean_mark . value = 1;
    PushSyntaxNode(t_node);
}

void PushMarkedFalseSyntaxGrammar(long p_mark)
{
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindBooleanMark;
    t_node -> boolean_mark . index = p_mark;
    t_node -> boolean_mark . value = 0;
    PushSyntaxNode(t_node);
}

void PushMarkedIntegerSyntaxGrammar(long p_mark, long p_value)
{
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindIntegerMark;
    t_node -> integer_mark . index = p_mark;
    t_node -> integer_mark . value = p_value;
    PushSyntaxNode(t_node);
}

void PushMarkedStringSyntaxGrammar(long p_mark, const char *p_value)
{
    NameRef t_name;
    MakeNameLiteral(p_value, &t_name);
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindIntegerMark;
    t_node -> string_mark . index = p_mark;
    t_node -> string_mark . value = t_name;
    PushSyntaxNode(t_node);
}

////////////////////////////////////////////////////////////////////////////////

void BeginSyntaxMappings(void)
{
    assert(s_rule != NULL);
}

void EndSyntaxMappings(void)
{
}

void BeginMethodSyntaxMapping(NameRef p_name)
{
}

void EndMethodSyntaxMapping(void)
{
}

void PushUndefinedArgumentSyntaxMapping(void)
{
}

void PushTrueArgumentSyntaxMapping(void)
{
}

void PushFalseArgumentSyntaxMapping(void)
{
}

void PushIntegerArgumentSyntaxMapping(long p_value)
{
}

void PushStringArgumentSyntaxMapping(const char *p_value)
{
}

void PushMarkArgumentSyntaxMapping(long p_index)
{
}

////////////////////////////////////////////////////////////////////////////////

