#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "report.h"
#include "literal.h"
#include "position.h"

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
    kSyntaxNodeKindRealMark,
    kSyntaxNodeKindStringMark,
};

typedef struct SyntaxNode *SyntaxNodeRef;
struct SyntaxNodeMark
{
    long index;
    SyntaxNodeKind kind;
    int used;
    SyntaxNodeRef value;
    long lmode;
    long rmode;
};

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
            long lmode;
            long rmode;
        } descent;
        struct
        {
            long index;
            long value;
        } boolean_mark, integer_mark;
        struct
        {
            long index;
            double value;
        } real_mark;
        struct
        {
            long index;
            NameRef value;
        } string_mark;
    };
    
    // Used on output to hold the concrete rule index of this node (for repeat / alternate rules).
    int concrete_rule;
    struct SyntaxNodeMark *marks;
    int mark_count;
    
    int left_trimmed, right_trimmed;
    long left_lmode, left_rmode;
    long right_lmode, right_rmode;
};

typedef enum SyntaxMethodType SyntaxMethodType;
enum SyntaxMethodType
{
    kSyntaxMethodTypeExecute,
    kSyntaxMethodTypeEvaluate,
    kSyntaxMethodTypeAssign,
};

typedef struct SyntaxArgument *SyntaxArgumentRef;
struct SyntaxArgument
{
    SyntaxArgumentRef next;
    long mode;
    long index;
};

typedef struct SyntaxMethod *SyntaxMethodRef;
struct SyntaxMethod
{
    SyntaxMethodRef next;
    NameRef name;
    SyntaxMethodType type;
    struct SyntaxArgument *arguments;
};

typedef enum SyntaxRuleKind SyntaxRuleKind;
enum SyntaxRuleKind
{
    kSyntaxRuleKindFragment,
    kSyntaxRuleKindPhrase,
    kSyntaxRuleKindStatement,
    kSyntaxRuleKindExpression,
    kSyntaxRuleKindPrefixOperator,
    kSyntaxRuleKindPostfixOperator,
    kSyntaxRuleKindLeftBinaryOperator,
    kSyntaxRuleKindRightBinaryOperator,
    kSyntaxRuleKindNeutralBinaryOperator,
};

typedef struct SyntaxRule *SyntaxRuleRef;
struct SyntaxRule
{
    SyntaxRuleRef next;
    NameRef module;
    NameRef name;
    SyntaxRuleKind kind;
    long precedence;
    SyntaxNodeRef expr;
    SyntaxMethodRef methods;
    long *mapping;

    SyntaxMethodRef current_method;
};

typedef struct SyntaxRuleGroup *SyntaxRuleGroupRef;
struct SyntaxRuleGroup
{
    SyntaxRuleGroupRef next;
    int index;
    SyntaxRuleRef rules;
};

static SyntaxRuleGroupRef s_groups = NULL;
static SyntaxRuleRef s_rule = NULL;
static SyntaxNodeRef s_stack[MAX_NODE_DEPTH];
static unsigned int s_stack_index = 0;

static int IsSyntaxNodeEqualTo(SyntaxNodeRef p_left, SyntaxNodeRef p_right);

////////////////////////////////////////////////////////////////////////////////

static void *Allocate(size_t p_size)
{
    void *t_ptr;
    t_ptr = calloc(1, p_size);
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

static void BeginSyntaxRule(NameRef p_module, NameRef p_name, SyntaxRuleKind p_kind, long p_precedence)
{
    assert(s_rule == NULL);
    
    s_rule = (SyntaxRuleRef)Allocate(sizeof(struct SyntaxRule));
    
    s_rule -> next = NULL;
    s_rule -> module = p_module;
    s_rule -> name = p_name;
    s_rule -> kind = p_kind;
    s_rule -> precedence = p_precedence;
    s_rule -> expr = NULL;
    s_rule -> methods = NULL;
    s_rule -> mapping = NULL;
}

void BeginPhraseSyntaxRule(NameRef p_module, NameRef p_name)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindPhrase, 0);
}

void BeginStatementSyntaxRule(NameRef p_module, NameRef p_name)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindStatement, 0);
}

void BeginExpressionSyntaxRule(NameRef p_module, NameRef p_name)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindExpression, 0);
}

void BeginPrefixOperatorSyntaxRule(NameRef p_module, NameRef p_name, long p_precedence)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindPrefixOperator, p_precedence);
}

void BeginPostfixOperatorSyntaxRule(NameRef p_module, NameRef p_name, long p_precedence)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindPostfixOperator, p_precedence);
}

void BeginLeftBinaryOperatorSyntaxRule(NameRef p_module, NameRef p_name, long p_precedence)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindLeftBinaryOperator, p_precedence);
}

void BeginRightBinaryOperatorSyntaxRule(NameRef p_module, NameRef p_name, long p_precedence)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindRightBinaryOperator, p_precedence);
}

void BeginNeutralBinaryOperatorSyntaxRule(NameRef p_module, NameRef p_name, long p_precedence)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindNeutralBinaryOperator, p_precedence);
}

void EndSyntaxRule(void)
{
    assert(s_rule != NULL);
    
    SyntaxRuleGroupRef t_group;
    for(t_group = s_groups; t_group != NULL; t_group = t_group -> next)
    {
        if (s_rule -> kind != kSyntaxRuleKindPhrase &&
            IsSyntaxNodeEqualTo(s_rule -> expr, t_group -> rules -> expr) &&
            s_rule -> kind == t_group -> rules -> kind &&
            s_rule -> precedence == t_group -> rules -> precedence)
            break;
    }
    
    if (t_group == NULL)
    {
        t_group = (SyntaxRuleGroupRef)Allocate(sizeof(struct SyntaxRuleGroup));
        t_group -> rules = NULL;
        
        t_group -> next = s_groups;
        s_groups = t_group;
    }
    
    s_rule -> next = t_group -> rules;
    t_group -> rules = s_rule;
    
    s_rule = NULL;
}

////////////////////////////////////////////////////////////////////////////////

// Syntax node construction is done in such a way to ensure the following
// (canonical?) representation:
//   1) The top-level node is always a concatenate.
//   2) Each operand of an alternate is a concatenate.
//   3) The element and delimiter of a repeat are a concatenate (and thus cannot be nullable)
//   4) All non-descent marks appear at the end of a concatenate.
//   6) An operand of a concatenate is never a concatenate.
//   7) An operand of an alternate is never a alternate.
//   8) No empty nodes appear anyway.
//   9) The children of an alternate are distinct up to marks.

static int IsMarkSyntaxNode(SyntaxNodeRef p_node)
{
    return p_node -> kind >= kSyntaxNodeKindBooleanMark;
}

static int IsSyntaxNodeEqualTo(SyntaxNodeRef p_left, SyntaxNodeRef p_right)
{
    if (p_left -> kind != p_right -> kind)
        return 0;
    
    switch(p_left -> kind)
    {
        case kSyntaxNodeKindEmpty:
            break;
            
        case kSyntaxNodeKindKeyword:
            if (p_left -> keyword . token != p_right -> keyword . token)
                return 0;
            break;
            
        case kSyntaxNodeKindDescent:
            if (p_left -> descent . rule != p_right -> descent . rule)
                return 0;
            break;
            
        case kSyntaxNodeKindBooleanMark:
        case kSyntaxNodeKindIntegerMark:
        case kSyntaxNodeKindRealMark:
        case kSyntaxNodeKindStringMark:
            break;
        case kSyntaxNodeKindConcatenate:
            for(int i = 0; 1; i++)
            {
                SyntaxNodeRef t_left_child, t_right_child;
                if (i < p_left -> concatenate . operand_count)
                    t_left_child = p_left -> concatenate . operands[i];
                else
                    t_left_child = NULL;
                if (i < p_right -> concatenate . operand_count)
                    t_right_child = p_right -> concatenate . operands[i];
                else
                    t_right_child = NULL;
                
                if (t_left_child != NULL && t_right_child != NULL)
                {
                    if (!IsSyntaxNodeEqualTo(t_left_child, t_right_child))
                        return 0;
                }
                else if (t_left_child != NULL)
                {
                    if (!IsMarkSyntaxNode(t_left_child))
                        return 0;
                }
                else if (t_right_child != NULL)
                {
                    if (!IsMarkSyntaxNode(t_right_child))
                        return 0;
                }
                else
                    break;
            }
            break;
        case kSyntaxNodeKindAlternate:
            if (p_left -> alternate . operand_count != p_right -> alternate . operand_count)
                return 0;
            if (p_left -> alternate . is_nullable != p_right -> alternate . is_nullable)
                return 0;
            for(int i = 0; i < p_left -> alternate . operand_count; i++)
            {
                int t_found;
                t_found = 0;
                for(int j = 0; j < p_right -> alternate . operand_count; j++)
                    if (IsSyntaxNodeEqualTo(p_left -> alternate . operands[i], p_right -> alternate . operands[j]))
                    {
                        t_found = 1;
                        break;
                    }
                if (!t_found)
                    return 0;
            }
            break;
        case kSyntaxNodeKindRepeat:
            if (!IsSyntaxNodeEqualTo(p_left -> repeat . element, p_right -> repeat . element))
                return 0;
            if (!IsSyntaxNodeEqualTo(p_left -> repeat . delimiter, p_right -> repeat . delimiter))
                return 0;
            break;
    }
    
    return 1;
}

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
    if (p_node == NULL)
        return;
    
    switch(p_node -> kind)
    {
        case kSyntaxNodeKindEmpty:
        case kSyntaxNodeKindKeyword:
        case kSyntaxNodeKindDescent:
        case kSyntaxNodeKindBooleanMark:
        case kSyntaxNodeKindIntegerMark:
        case kSyntaxNodeKindRealMark:
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
    free(p_node -> marks);
    free(p_node);
}

static void AppendSyntaxNode(SyntaxNodeRef p_target, SyntaxNodeRef p_node)
{
    assert(p_target -> kind == kSyntaxNodeKindConcatenate || p_target -> kind == kSyntaxNodeKindAlternate);
    assert(p_node -> kind != p_target -> kind);
    
    if (p_target -> kind == kSyntaxNodeKindConcatenate && p_node -> kind == kSyntaxNodeKindEmpty)
        return;
    
    if (p_target -> kind == kSyntaxNodeKindAlternate && p_node -> kind == kSyntaxNodeKindEmpty)
    {
        p_target -> alternate . is_nullable = 1;
        return;
    }
    
    if (p_target -> kind == kSyntaxNodeKindAlternate)
    {
        // TODO: Check whether node is already a child of target.
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
    
    if (p_target -> kind == kSyntaxNodeKindConcatenate && p_node -> kind == kSyntaxNodeKindEmpty)
        return;
    
    if (p_target -> kind == kSyntaxNodeKindConcatenate && p_node -> kind >= kSyntaxNodeKindBooleanMark)
    {
        AppendSyntaxNode(p_target, p_node);
        return;
    }
    
    if (p_target -> kind == kSyntaxNodeKindAlternate)
    {
        AppendSyntaxNode(p_target, p_node);
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

static void JoinSyntaxNodes(SyntaxNodeRef p_left, SyntaxNodeRef p_right)
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

static long CountSyntaxNodeMarks(SyntaxNodeRef p_node)
{
    long t_index;
    switch(p_node -> kind)
    {
        case kSyntaxNodeKindEmpty:
            t_index = 0;
            break;
        case kSyntaxNodeKindKeyword:
            t_index = 0;
            break;
        case kSyntaxNodeKindDescent:
            t_index = p_node -> descent . index == -1 ? 0 : p_node -> descent . index;
            break;
        case kSyntaxNodeKindBooleanMark:
        case kSyntaxNodeKindIntegerMark:
        case kSyntaxNodeKindRealMark:
        case kSyntaxNodeKindStringMark:
            t_index = p_node -> boolean_mark . index;
            break;
        case kSyntaxNodeKindConcatenate:
        case kSyntaxNodeKindAlternate:
            t_index = 0;
            for(int i = 0; i < p_node -> concatenate . operand_count; i++)
            {
                long t_node_index;
                t_node_index = CountSyntaxNodeMarks(p_node -> concatenate . operands[i]);
                if (t_node_index > t_index)
                    t_index = t_node_index;
            }
            break;
        case kSyntaxNodeKindRepeat:
            t_index = CountSyntaxNodeMarks(p_node -> repeat . element);
            break;
    }
    
    return t_index;
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
        case kSyntaxNodeKindRealMark:
        {
            printf("<%ld=%lf>", p_node -> boolean_mark . index, p_node -> real_mark . value);
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

static void RemoveFirstTermFromSyntaxNode(SyntaxNodeRef p_node, long *r_lmode, long *r_rmode)
{
    if (p_node -> kind == kSyntaxNodeKindAlternate)
    {
        for(int i = 0; i < p_node -> alternate . operand_count; i++)
            RemoveFirstTermFromSyntaxNode(p_node -> alternate . operands[i], r_lmode, r_rmode);
    }
    else if (p_node -> kind == kSyntaxNodeKindConcatenate)
    {
        p_node -> left_trimmed = 1;
        *r_lmode = p_node -> concatenate . operands[0] -> descent . lmode;
        *r_rmode = p_node -> concatenate . operands[0] -> descent . rmode;
        FreeSyntaxNode(p_node -> concatenate . operands[0]);
        for(int i = 1; i < p_node -> concatenate . operand_count; i++)
            p_node -> concatenate . operands[i - 1] = p_node -> concatenate . operands[i];
        p_node -> concatenate . operand_count -= 1;
    }
    else
        assert(0);
}

static void RemoveLastTermFromSyntaxNode(SyntaxNodeRef p_node, long *r_lmode, long *r_rmode)
{
    if (p_node -> kind == kSyntaxNodeKindAlternate)
    {
        for(int i = 0; i < p_node -> alternate . operand_count; i++)
            RemoveLastTermFromSyntaxNode(p_node -> alternate . operands[i], r_lmode, r_rmode);
    }
    else if (p_node -> kind == kSyntaxNodeKindConcatenate)
    {
        p_node -> right_trimmed = 1;
        *r_lmode = p_node -> concatenate . operands[p_node -> concatenate . operand_count - 1] -> descent . lmode;
        *r_rmode = p_node -> concatenate . operands[p_node -> concatenate . operand_count - 1] -> descent . rmode;
        FreeSyntaxNode(p_node -> concatenate . operands[p_node -> concatenate . operand_count - 1]);
        p_node -> concatenate . operand_count -= 1;
    }
    else
        assert(0);
}

static void TrimSyntaxNodeForOperator(SyntaxNodeRef p_node, SyntaxRuleKind p_kind)
{
    int t_remove_left;
    t_remove_left = 0;
    if (p_kind == kSyntaxRuleKindLeftBinaryOperator ||
        p_kind == kSyntaxRuleKindRightBinaryOperator ||
        p_kind == kSyntaxRuleKindNeutralBinaryOperator ||
        p_kind == kSyntaxRuleKindPostfixOperator)
        t_remove_left = 1;
    
    int t_remove_right;
    t_remove_right = 0;
    if (p_kind == kSyntaxRuleKindLeftBinaryOperator ||
        p_kind == kSyntaxRuleKindRightBinaryOperator ||
        p_kind == kSyntaxRuleKindNeutralBinaryOperator ||
        p_kind == kSyntaxRuleKindPrefixOperator)
        t_remove_right = 1;
    
    if (t_remove_left)
        RemoveFirstTermFromSyntaxNode(p_node, &p_node -> left_lmode, &p_node -> left_rmode);
    if (t_remove_right)
        RemoveLastTermFromSyntaxNode(p_node, &p_node -> right_lmode, &p_node -> right_rmode);
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

    if (s_rule -> kind >= kSyntaxRuleKindPrefixOperator)
        TrimSyntaxNodeForOperator(s_stack[0], s_rule -> kind);

    s_rule -> expr = s_stack[0];
    s_stack_index = 0;
    
    if (s_rule -> expr -> kind != kSyntaxNodeKindAlternate &&
        s_rule -> expr -> kind != kSyntaxNodeKindConcatenate &&
        s_rule -> expr -> kind != kSyntaxNodeKindRepeat)
    {
        SyntaxNodeRef t_node;
        MakeSyntaxNode(&t_node);
        t_node -> kind = kSyntaxNodeKindConcatenate;
        t_node -> concatenate . operands = NULL;
        t_node -> concatenate . operand_count = 0;
        t_node -> concatenate . is_nullable = 0;
        AppendSyntaxNode(t_node, s_rule -> expr);
        s_rule -> expr = t_node;
    }
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
        JoinSyntaxNodes(t_left, t_right);
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

void PushMarkedDescentSyntaxGrammar(long p_mark, NameRef p_rule, long p_lmode, long p_rmode)
{
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindDescent;
    t_node -> descent . index = p_mark;
    t_node -> descent . rule = p_rule;
    t_node -> descent . lmode = p_lmode;
    t_node -> descent . rmode = p_rmode;
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

void PushMarkedRealSyntaxGrammar(long p_mark, long p_value)
{
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindRealMark;
    t_node -> real_mark . index = p_mark;
    t_node -> real_mark . value = *(double *)p_value;
    PushSyntaxNode(t_node);
}

void PushMarkedStringSyntaxGrammar(long p_mark, const char *p_value)
{
    NameRef t_name;
    MakeNameLiteral(p_value, &t_name);
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindStringMark;
    t_node -> string_mark . index = p_mark;
    t_node -> string_mark . value = t_name;
    PushSyntaxNode(t_node);
}

////////////////////////////////////////////////////////////////////////////////

void BeginSyntaxMappings(void)
{
    assert(s_rule != NULL);
    s_rule -> current_method = NULL;
}

void EndSyntaxMappings(void)
{
}

static void BeginMethodSyntaxMapping(SyntaxMethodType p_type, NameRef p_name)
{
    SyntaxMethodRef t_method;
    t_method = (SyntaxMethodRef)Allocate(sizeof(struct SyntaxMethod));
    t_method -> type = p_type;
    t_method -> name = p_name;
    t_method -> next = NULL;
    s_rule -> current_method = t_method;
}

void BeginExecuteMethodSyntaxMapping(NameRef p_name)
{
    BeginMethodSyntaxMapping(kSyntaxMethodTypeExecute, p_name);
}

void BeginEvaluateMethodSyntaxMapping(NameRef p_name)
{
    BeginMethodSyntaxMapping(kSyntaxMethodTypeEvaluate, p_name);
}

void BeginAssignMethodSyntaxMapping(NameRef p_name)
{
    BeginMethodSyntaxMapping(kSyntaxMethodTypeAssign, p_name);
}

void EndMethodSyntaxMapping(void)
{
    if (s_rule -> methods == NULL)
        s_rule -> methods = s_rule -> current_method;
    else
    {
        SyntaxMethodRef t_last;
        for(t_last = s_rule -> methods; t_last -> next != NULL; t_last = t_last -> next)
            ;
        t_last -> next = s_rule -> current_method;
    }
    s_rule -> current_method = NULL;
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

void PushRealArgumentSyntaxMapping(double *p_value)
{
}

void PushStringArgumentSyntaxMapping(const char *p_value)
{
}

static void PushMarkArgumentSyntaxMapping(long p_mode, long p_index)
{
    SyntaxArgumentRef t_arg;
    t_arg = (SyntaxArgumentRef)Allocate(sizeof(struct SyntaxArgument));
    t_arg -> mode = p_mode;
    t_arg -> index = p_index;
    t_arg -> next = NULL;
    if (s_rule -> current_method -> arguments == NULL)
        s_rule -> current_method -> arguments = t_arg;
    else
    {
        SyntaxArgumentRef t_last;
        for(t_last = s_rule -> current_method -> arguments; t_last -> next != NULL; t_last = t_last -> next)
            ;
        t_last -> next = t_arg;
    }
}

void PushInMarkArgumentSyntaxMapping(long p_index)
{
    PushMarkArgumentSyntaxMapping(0, p_index);
}

void PushOutMarkArgumentSyntaxMapping(long p_index)
{
    PushMarkArgumentSyntaxMapping(1, p_index);
}

void PushInOutMarkArgumentSyntaxMapping(long p_index)
{
    PushMarkArgumentSyntaxMapping(2, p_index);
}

////////////////////////////////////////////////////////////////////////////////

static void MergeSyntaxNodes(SyntaxNodeRef p_node, SyntaxNodeRef p_other_node, long *x_next_mark, long *x_mapping)
{
    if (p_node -> kind == kSyntaxNodeKindAlternate)
    {
        for(int i = 0; i < p_node -> alternate . operand_count; i++)
        {
            for(int j = 0; j < p_other_node -> alternate . operand_count; j++)
                if (IsSyntaxNodeEqualTo(p_node -> alternate . operands[i], p_other_node -> alternate . operands[j]))
                    MergeSyntaxNodes(p_node -> alternate . operands[i], p_other_node -> alternate . operands[j], x_next_mark, x_mapping);
        }
    }
    else if (p_node -> kind == kSyntaxNodeKindConcatenate)
    {
        for(int i = 0; i < p_other_node -> concatenate . operand_count; i++)
        {
            SyntaxNodeRef t_child, t_other_child;
            t_child = p_node -> concatenate . operands[i];
            t_other_child = p_other_node -> concatenate . operands[i];
            if (!IsMarkSyntaxNode(t_other_child))
            {
                MergeSyntaxNodes(t_child, t_other_child, x_next_mark, x_mapping);
                continue;
            }
            
            x_mapping[t_other_child -> boolean_mark . index] = *x_next_mark;
            t_other_child -> boolean_mark . index = *x_next_mark;
            AppendSyntaxNode(p_node, t_other_child);
            *x_next_mark += 1;
            
            p_other_node -> concatenate . operands[i] = NULL;
        }
    }
    else if (p_node -> kind == kSyntaxNodeKindRepeat)
    {
        MergeSyntaxNodes(p_node -> repeat . element, p_other_node -> repeat . element, x_next_mark, x_mapping);
    }
    else if (p_node -> kind == kSyntaxNodeKindDescent)
    {
        if (p_other_node -> descent . index != -1)
        {
            if (p_node -> descent . index == -1)
            {
                p_node -> descent . index = *x_next_mark;
                x_mapping[p_other_node -> descent . index] = *x_next_mark;
                x_next_mark++;
            }
            else
                x_mapping[p_other_node -> descent . index] = p_node -> descent . index;
        }
    }
}

static void MergeSyntaxRule(SyntaxRuleRef p_rule, SyntaxRuleRef p_other_rule)
{
    long t_rule_mark_count, t_other_rule_mark_count;
    t_rule_mark_count = CountSyntaxNodeMarks(p_rule -> expr);
    t_other_rule_mark_count = CountSyntaxNodeMarks(p_other_rule -> expr);
    
    long *t_mark_mapping;
    t_mark_mapping = (long *)Allocate(t_other_rule_mark_count * sizeof(long));
    
    MergeSyntaxNodes(p_rule -> expr, p_other_rule -> expr, &t_rule_mark_count, t_mark_mapping);
    
    // Need to process this against the method mappings.
    p_other_rule -> mapping = t_mark_mapping;
}

static void SetSyntaxNodeMarkAsUsed(struct SyntaxNodeMark *p_marks, int p_mark_count, long p_index, SyntaxNodeRef p_value)
{
    for(int i = 0; i < p_mark_count; i++)
        if (p_marks[i] . index == p_index)
        {
            p_marks[i] . used = 1;
            p_marks[i] . value = p_value;
            break;
        }
}

static void SetAllSyntaxNodeMarksAsUnused(struct SyntaxNodeMark *p_marks, int p_mark_count)
{
    for(int i = 0; i < p_mark_count; i++)
        p_marks[i] . used = 0;
}

static void GenerateSyntaxRuleTerm(SyntaxNodeRef p_node, struct SyntaxNodeMark *p_marks, int p_mark_count)
{
    switch(p_node -> kind)
    {
        case kSyntaxNodeKindEmpty:
            break;
        case kSyntaxNodeKindKeyword:
            {
                const char *t_string;
                GetStringOfNameLiteral(p_node -> keyword . token, &t_string);
                fprintf(stderr, "\"%s\"", t_string);
            }
            break;
        case kSyntaxNodeKindDescent:
            {
                const char *t_string;
                GetStringOfNameLiteral(p_node -> descent . rule, &t_string);
                if (strcmp(t_string, "Expression") != 0 &&
                    strcmp(t_string, "ExpressionList") != 0)
                    fprintf(stderr, "Custom_");
                if (strcmp(t_string, "ExpressionList") == 0)
                    fprintf(stderr, "ExpressionListAsExpression");
                else
                    fprintf(stderr, "%s", t_string);
                if (p_node -> descent . index != -1)
                {
                    fprintf(stderr, "(-> Mark%ld)", p_node -> descent . index);
                    SetSyntaxNodeMarkAsUsed(p_marks, p_mark_count, p_node -> descent . index, NULL);
                }
            }
            break;
        case kSyntaxNodeKindBooleanMark:
        case kSyntaxNodeKindIntegerMark:
        case kSyntaxNodeKindStringMark:
        case kSyntaxNodeKindRealMark:
            SetSyntaxNodeMarkAsUsed(p_marks, p_mark_count, p_node -> descent . index, p_node);
            break;
        case kSyntaxNodeKindConcatenate:
        case kSyntaxNodeKindAlternate:
        case kSyntaxNodeKindRepeat:
            fprintf(stderr, "CustomRule%d(-> ", p_node -> concrete_rule);
            for(int i = 0; i < p_node -> mark_count; i++)
            {
                if (i != 0)
                    fprintf(stderr, ", ");
                fprintf(stderr, "Mark%ld", p_node -> marks[i] . index);
                SetSyntaxNodeMarkAsUsed(p_marks, p_mark_count, p_node -> marks[i] . index, NULL);
            }
            fprintf(stderr, ")");
            break;
    }
}

static void AddSyntaxNodeMark(struct SyntaxNodeMark **x_marks, int *x_mark_count, long p_index, SyntaxNodeKind p_kind, long p_lmode, long p_rmode)
{
    for(int i = 0; i < *x_mark_count; i++)
        if ((*x_marks)[i] . index == p_index)
            return;
    
    *x_marks = (struct SyntaxNodeMark *)Reallocate(*x_marks, (*x_mark_count + 1) * sizeof(struct SyntaxNodeMark));
    (*x_marks)[*x_mark_count] . index = p_index;
    (*x_marks)[*x_mark_count] . kind = p_kind;
    (*x_marks)[*x_mark_count] . lmode = p_lmode;
    (*x_marks)[*x_mark_count] . rmode = p_rmode;
    *x_mark_count += 1;
}

static void GenerateSyntaxRuleMarks(SyntaxNodeRef p_node, struct SyntaxNodeMark **x_marks, int *x_mark_count)
{
    switch(p_node -> kind)
    {
        case kSyntaxNodeKindEmpty:
            break;
        case kSyntaxNodeKindKeyword:
            break;
        case kSyntaxNodeKindDescent:
            if (p_node -> descent . index != -1)
                AddSyntaxNodeMark(x_marks, x_mark_count, p_node -> descent . index, kSyntaxNodeKindDescent, p_node -> descent . lmode, p_node -> descent . rmode);
            break;
        case kSyntaxNodeKindBooleanMark:
            AddSyntaxNodeMark(x_marks, x_mark_count, p_node -> boolean_mark . index, kSyntaxNodeKindBooleanMark, 0, 0);
            break;
        case kSyntaxNodeKindIntegerMark:
            AddSyntaxNodeMark(x_marks, x_mark_count, p_node -> integer_mark . index, kSyntaxNodeKindIntegerMark, 0, 0);
            break;
        case kSyntaxNodeKindRealMark:
            AddSyntaxNodeMark(x_marks, x_mark_count, p_node -> real_mark . index, kSyntaxNodeKindRealMark, 0, 0);
            break;
        case kSyntaxNodeKindStringMark:
            AddSyntaxNodeMark(x_marks, x_mark_count, p_node -> string_mark . index, kSyntaxNodeKindStringMark, 0, 0);
            break;
        case kSyntaxNodeKindConcatenate:
        case kSyntaxNodeKindAlternate:
            for(int i = 0; i < p_node -> concatenate . operand_count; i++)
                GenerateSyntaxRuleMarks(p_node -> concatenate . operands[i], x_marks, x_mark_count);
            break;
        case kSyntaxNodeKindRepeat:
            break;
    }
}

static void GenerateSyntaxRuleSubHeader(SyntaxNodeRef p_node, SyntaxRuleKind p_kind)
{
    if (p_kind >= kSyntaxRuleKindPrefixOperator)
        fprintf(stderr, "  'rule' CustomRule%d(-> Sentinal):\n", p_node -> concrete_rule);
    else
    {
        fprintf(stderr, "  'rule' CustomRule%d(-> ", p_node -> concrete_rule);
        if (p_kind == kSyntaxRuleKindFragment)
        {
            for(int i = 0; i < p_node -> mark_count; i++)
            {
                if (i != 0)
                    fprintf(stderr, ", ");
                fprintf(stderr, "Mark%ld", p_node -> marks[i] . index);
            }
        }
        else
        {
            fprintf(stderr, "invoke(Position, Invoke, ");
            for(int i = 0; i < p_node -> mark_count; i++)
                fprintf(stderr, "expressionlist(Mark%ld, ", p_node -> marks[i] . index);
            fprintf(stderr, "nil");
            for(int i = 0; i < p_node -> mark_count; i++)
                fprintf(stderr, ")");
            fprintf(stderr, ")");
        }
        fprintf(stderr, "):\n");
    }
}

static void GenerateSyntaxRuleExplicitAndUnusedMarks(SyntaxNodeRef p_node)
{
    int t_has_pos;
    t_has_pos = 0;
    for(int i = 0; i < p_node -> mark_count; i++)
    {
        if (p_node -> marks[i] . used == 0)
            fprintf(stderr, "    where(EXPRESSION'nil -> Mark%ld)\n", p_node -> marks[i] . index);
        else if (p_node -> marks[i] . value != NULL)
        {
            if (t_has_pos == 0)
            {
                fprintf(stderr, "    GetUndefinedPosition(-> UndefinedPosition)\n");
                t_has_pos = 1;
            }
            if (p_node -> marks[i] . value -> kind == kSyntaxNodeKindRealMark)
            {
                fprintf(stderr, "    MakeDoubleLiteral(\"%.18lf\" -> Mark%ldValue)\n", p_node -> marks[i] . value -> real_mark . value, p_node -> marks[i] . index);
            }
            fprintf(stderr, "    where(");
            switch(p_node -> marks[i] . value -> kind)
            {
                case kSyntaxNodeKindBooleanMark:
                    fprintf(stderr, "EXPRESSION'%s(UndefinedPosition)", p_node -> marks[i] . value -> boolean_mark . value == 0 ? "false" : "true");
                    break;
                case kSyntaxNodeKindIntegerMark:
                    fprintf(stderr, "EXPRESSION'integer(UndefinedPosition, %ld)", p_node -> marks[i] . value -> integer_mark . value);
                    break;
                case kSyntaxNodeKindRealMark:
                    fprintf(stderr, "EXPRESSION'real(UndefinedPosition, Mark%ldValue)", p_node -> marks[i] . index);
                    break;
                case kSyntaxNodeKindStringMark:
                    {
                        const char *t_string;
                        GetStringOfNameLiteral(p_node -> marks[i] . value -> string_mark . value, &t_string);
                        fprintf(stderr, "EXPRESSION'string(UndefinedPosition, \"%s\")", t_string);
                    }
                    break;
                default:
                    assert(0);
                    break;
            }
            fprintf(stderr, " -> Mark%ld)\n", p_node -> marks[i] . index);
        }
            
    }
}

static void GenerateSyntaxRuleConstructor(SyntaxNodeRef p_node, SyntaxRuleRef p_rule)
{
    if (p_rule != NULL && p_rule -> kind >= kSyntaxRuleKindPrefixOperator)
    {
        static const char *s_calls[] = { "PushOperatorExpressionPrefix", "PushOperatorExpressionPostfix", "PushOperatorExpressionLeftBinary", "PushOperatorExpressionRightBinary", "PushOperatorExpressionNeutralBinary" };
        fprintf(stderr, "    %s(Position, %ld, %d -> Sentinal)\n", s_calls[p_rule -> kind - kSyntaxRuleKindPrefixOperator],
                p_rule -> precedence, p_node -> concrete_rule);
        for(int i = 0; i < p_node -> mark_count; i++)
            fprintf(stderr, "    PushOperatorExpressionArgument(Mark%ld -> _)\n", p_node -> marks[i] . index);
    }
    else if (p_rule != NULL)
    {
        fprintf(stderr, "    CustomInvokeLists(%d -> Invoke)\n", p_node -> concrete_rule);
    }
}

static void GenerateSyntaxRule(int *x_index, SyntaxNodeRef p_node, SyntaxRuleKind p_kind, SyntaxRuleRef p_rule)
{
    assert(p_node -> kind == kSyntaxNodeKindConcatenate ||
           p_node -> kind == kSyntaxNodeKindAlternate ||
           p_node -> kind == kSyntaxNodeKindRepeat);
    
    int t_index;
    t_index = *x_index;
    *x_index += 1;
    
    if (p_node -> kind == kSyntaxNodeKindConcatenate)
    {
        for(int i = 0; i < p_node -> alternate . operand_count; i++)
            if (p_node -> alternate . operands[i] -> kind == kSyntaxNodeKindAlternate ||
                p_node -> alternate . operands[i] -> kind == kSyntaxNodeKindRepeat)
                GenerateSyntaxRule(x_index, p_node -> alternate . operands[i], kSyntaxRuleKindFragment, NULL);
    }
    else if (p_node -> kind == kSyntaxNodeKindAlternate)
    {
        for(int i = 0; i < p_node -> alternate . operand_count; i++)
            if (p_node -> alternate . operands[i] -> kind == kSyntaxNodeKindRepeat)
                GenerateSyntaxRule(x_index, p_node -> alternate . operands[i], kSyntaxRuleKindFragment, NULL);
    }
    else if (p_node -> kind == kSyntaxNodeKindRepeat)
    {
        GenerateSyntaxRule(x_index, p_node -> repeat . element, kSyntaxRuleKindFragment, NULL);
        if (p_node -> repeat . delimiter != NULL)
            GenerateSyntaxRule(x_index, p_node -> repeat . delimiter, kSyntaxRuleKindFragment, NULL);
    }
    
    // Now generate this rule.
    
    p_node -> marks = NULL;
    p_node -> mark_count = 0;
    GenerateSyntaxRuleMarks(p_node, &p_node -> marks, &p_node -> mark_count);
    
    p_node -> concrete_rule = t_index;
    
    if (p_kind == kSyntaxRuleKindFragment)
    {
        fprintf(stderr, "'nonterm' CustomRule%d(-> ", t_index);
        for(int i = 0; i < p_node -> mark_count; i++)
        {
            if (i != 0)
                fprintf(stderr, ", ");
            fprintf(stderr, "EXPRESSION");
        }
        fprintf(stderr, ")\n");
    }
    else if (p_kind >= kSyntaxRuleKindPrefixOperator)
        fprintf(stderr, "'nonterm' CustomRule%d(-> INT)\n", t_index);
    else
        fprintf(stderr, "'nonterm' CustomRule%d(-> %s)\n", t_index, p_kind == kSyntaxRuleKindStatement ? "STATEMENT" : "EXPRESSION");
    
    if (p_node -> kind == kSyntaxNodeKindConcatenate)
    {
        SetAllSyntaxNodeMarksAsUnused(p_node -> marks, p_node -> mark_count);
        
        GenerateSyntaxRuleSubHeader(p_node, p_kind);
        fprintf(stderr, "    ");
        for(int i = 0; i < p_node -> concatenate . operand_count; i++)
        {
            if (i != 0)
                fprintf(stderr, " ");
            GenerateSyntaxRuleTerm(p_node -> concatenate . operands[i], p_node -> marks, p_node -> mark_count);
            if (i == 0)
                fprintf(stderr, " @(-> Position)");
        }
        fprintf(stderr, "\n");
        GenerateSyntaxRuleExplicitAndUnusedMarks(p_node);
        if (p_rule != NULL)
            GenerateSyntaxRuleConstructor(p_node, p_rule);
    }
    else if (p_node -> kind == kSyntaxNodeKindAlternate)
    {
        for(int i = 0; i < p_node -> alternate . operand_count; i++)
        {
            SetAllSyntaxNodeMarksAsUnused(p_node -> marks, p_node -> mark_count);
            
            GenerateSyntaxRuleSubHeader(p_node, p_kind);
            fprintf(stderr, "    ");
            if (p_node -> alternate . operands[i] -> kind != kSyntaxNodeKindConcatenate)
                GenerateSyntaxRuleTerm(p_node -> alternate . operands[i], p_node -> marks, p_node -> mark_count);
            else
            {
                for(int j = 0; j < p_node -> alternate . operands[i] -> concatenate . operand_count; j++)
                {
                    if (j != 0)
                        fprintf(stderr, " ");
                    GenerateSyntaxRuleTerm(p_node -> alternate . operands[i] -> concatenate . operands[j], p_node -> marks, p_node -> mark_count);
                    if (j == 0)
                        fprintf(stderr, " @(-> Position)");
                }
            }
            fprintf(stderr, "\n");
            GenerateSyntaxRuleExplicitAndUnusedMarks(p_node);
            if (p_rule != NULL)
                GenerateSyntaxRuleConstructor(p_node, p_rule);
        }
        if (p_node -> alternate . is_nullable)
        {
            SetAllSyntaxNodeMarksAsUnused(p_node -> marks, p_node -> mark_count);
            GenerateSyntaxRuleSubHeader(p_node, p_kind);
            GenerateSyntaxRuleExplicitAndUnusedMarks(p_node);
            if (p_rule != NULL)
                GenerateSyntaxRuleConstructor(p_node, p_rule);
        }
    }
    else if (p_node -> kind == kSyntaxNodeKindRepeat)
    {
    }
}

static void GenerateUmbrellaSyntaxRule(const char *p_name, SyntaxRuleKind p_first, SyntaxRuleKind p_last)
{
    int t_made;
    t_made = 0;
    if (p_first <= kSyntaxRuleKindExpression)
    {
        fprintf(stderr, "'nonterm' %s(-> %s)\n", p_name, p_first == kSyntaxRuleKindStatement ? "STATEMENT" : "EXPRESSION");
        
        for(SyntaxRuleGroupRef t_group = s_groups; t_group != NULL; t_group = t_group -> next)
        {
            SyntaxRuleRef t_rule;
            t_rule = t_group -> rules;

            if (t_rule -> kind >= p_first && t_rule -> kind <= p_last)
            {
                fprintf(stderr, "  'rule' %s(-> Node):\n", p_name);
                fprintf(stderr, "    CustomRule%d(-> Node)\n", t_rule -> expr -> concrete_rule);
                t_made = 1;
            }
        }
        if (t_made == 0)
            fprintf(stderr, "  'rule' %s(-> nil):\n    \"__%s\"\n", p_name, p_name);
    }
    else
    {
        fprintf(stderr, "'nonterm' %s(-> INT)\n", p_name);
        
        for(SyntaxRuleGroupRef t_group = s_groups; t_group != NULL; t_group = t_group -> next)
        {
            SyntaxRuleRef t_rule;
            t_rule = t_group -> rules;
            
            if (t_rule -> kind >= p_first && t_rule -> kind <= p_last)
            {
                fprintf(stderr, "  'rule' %s(-> Sentinal):\n", p_name);
                fprintf(stderr, "    CustomRule%d(-> Sentinal)\n", t_rule -> expr -> concrete_rule);
                t_made = 1;
            }
        }
        if (t_made == 0)
            fprintf(stderr, "  'rule' %s(-> 10000):\n    \"__%s\"\n", p_name, p_name);
    }
}

static void GenerateInvokeMethodArg(SyntaxArgumentRef p_arg)
{
    if (p_arg == NULL)
    {
        fprintf(stderr, "nil");
        return;
    }
    
    static const char *s_modes[] = { "in", "out", "inout" };
    fprintf(stderr, "invokesignature(%s, %ld, ", s_modes[p_arg -> mode], p_arg -> index);
    GenerateInvokeMethodArg(p_arg -> next);
    fprintf(stderr, ")");
}

static void GenerateInvokeMethodList(int p_index, int p_method_index, SyntaxMethodRef p_method)
{
    if (p_method == NULL)
    {
        fprintf(stderr, "    where(INVOKEMETHODLIST'nil -> MethodList_%d_%d)\n", p_index, p_method_index);
        return;
    }

    GenerateInvokeMethodList(p_index, p_method_index + 1, p_method -> next);
    
    static const char *s_method_types[] = { "execute", "evaluate", "assign" };
    
    const char *t_name;
    GetStringOfNameLiteral(p_method -> name, &t_name);
    fprintf(stderr, "    where(INVOKEMETHODLIST'methodlist(\"%s\", %s, ", t_name, s_method_types[p_method -> type]);
    
    GenerateInvokeMethodArg(p_method -> arguments);
    
    fprintf(stderr, ", MethodList_%d_%d) -> MethodList_%d_%d)\n", p_index, p_method_index + 1, p_index, p_method_index);
}

static void GenerateInvokeLists(void)
{
    for(SyntaxRuleGroupRef t_group = s_groups; t_group != NULL; t_group = t_group -> next)
        fprintf(stderr, "'var' CustomInvokeList%d: INVOKELIST\n", t_group -> index);
    
    int t_method_index;
    t_method_index = 1;
    
    fprintf(stderr, "'action' InitializeCustomInvokeLists()\n");
    fprintf(stderr, "  'rule' InitializeCustomInvokeLists():\n");
    for(SyntaxRuleGroupRef t_group = s_groups; t_group != NULL; t_group = t_group -> next)
    {
        int t_index;
        t_index = 1;
        
        fprintf(stderr, "    where(INVOKELIST'nil -> List_%d_0)\n", t_group -> index);
        for(SyntaxRuleRef t_rule = t_group -> rules; t_rule != NULL; t_rule = t_rule -> next)
        {
            GenerateInvokeMethodList(t_method_index, 0, t_rule -> methods);
            
            const char *t_module, *t_name;
            GetStringOfNameLiteral(t_rule -> module, &t_module);
            GetStringOfNameLiteral(t_rule -> name, &t_name);
            fprintf(stderr, "    Info_%d_%d::INVOKEINFO\n", t_group -> index, t_index);
            fprintf(stderr, "    Info_%d_%d'Index <- -1\n", t_group -> index, t_index);
            fprintf(stderr, "    Info_%d_%d'ModuleIndex <- -1\n", t_group -> index, t_index);
            fprintf(stderr, "    Info_%d_%d'Name <- \"%s\"\n", t_group -> index, t_index, t_name);
            fprintf(stderr, "    Info_%d_%d'ModuleName <- \"%s\"\n", t_group -> index, t_index, t_module);
            fprintf(stderr, "    Info_%d_%d'Methods <- MethodList_%d_%d\n", t_group -> index, t_index, t_method_index, 0);
            
            fprintf(stderr, "    where(invokelist(Info_%d_%d, List_%d_%d) -> List_%d_%d)\n", t_group -> index, t_index, t_group -> index, t_index - 1, t_group -> index, t_index);
            t_index += 1;
            t_method_index += 1;
        }
        fprintf(stderr, "    CustomInvokeList%d <- List_%d_%d\n", t_group -> index, t_group -> index, t_index - 1);
    }
    
    fprintf(stderr, "'action' CustomInvokeLists(INT -> INVOKELIST)\n");
    for(SyntaxRuleGroupRef t_group = s_groups; t_group != NULL; t_group = t_group -> next)
    {
        fprintf(stderr, "  'rule' CustomInvokeLists(Index -> List):\n");
        fprintf(stderr, "    eq(Index, %d)\n", t_group -> index);
        fprintf(stderr, "    CustomInvokeList%d -> List\n", t_group -> index);
    }
}

#if 0
static void GenerateInvokeLists(void)
{
    for(SyntaxRuleGroupRef t_group = s_groups; t_group != NULL; t_group = t_group -> next)
        fprintf(stderr, "'var' CustomInvokeList%d: INVOKELIST\n", t_group -> index);
    
    fprintf(stderr, "'action' InitializeCustomInvokeLists()\n");
    fprintf(stderr, "  'rule' InitializeCustomInvokeLists():\n");
    for(SyntaxRuleGroupRef t_group = s_groups; t_group != NULL; t_group = t_group -> next)
    {
        static const char *s_modes[] = { "in", "out", "inout" };
        
        int t_index;
        t_index = 1;
        fprintf(stderr, "    where(INVOKESIGNATURE'nil -> LSig_%d_%d)\n", t_group -> index, 0);
        fprintf(stderr, "    where(INVOKESIGNATURE'nil -> RSig_%d_%d)\n", t_group -> index, 0);
        
        SyntaxNodeRef t_node;
        t_node = t_group -> rules -> expr;
        
        if (t_node -> right_trimmed)
        {
            fprintf(stderr, "    where(invokesignature(%s, LSig_%d_%d) -> LSig_%d_%d)\n", s_modes[t_node -> right_lmode], t_group -> index, t_index - 1, t_group -> index, t_index);
            fprintf(stderr, "    where(invokesignature(%s, RSig_%d_%d) -> RSig_%d_%d)\n", s_modes[t_node -> right_rmode], t_group -> index, t_index - 1, t_group -> index, t_index);
            t_index += 1;
        }
        for(int i = t_group -> rules -> expr -> mark_count; i > 0; i--)
        {
            fprintf(stderr, "    where(invokesignature(%s, LSig_%d_%d) -> LSig_%d_%d)\n", s_modes[t_group -> rules -> expr -> marks[i - 1] . lmode], t_group -> index, t_index - 1, t_group -> index, t_index);
            fprintf(stderr, "    where(invokesignature(%s, RSig_%d_%d) -> RSig_%d_%d)\n", s_modes[t_group -> rules -> expr -> marks[i - 1] . rmode], t_group -> index, t_index - 1, t_group -> index, t_index);
            t_index += 1;
        }
        if (t_node -> left_trimmed)
        {
            fprintf(stderr, "    where(invokesignature(%s, LSig_%d_%d) -> LSig_%d_%d)\n", s_modes[t_node -> left_lmode], t_group -> index, t_index - 1, t_group -> index, t_index);
            fprintf(stderr, "    where(invokesignature(%s, RSig_%d_%d) -> RSig_%d_%d)\n", s_modes[t_node -> left_rmode], t_group -> index, t_index - 1, t_group -> index, t_index);
            t_index += 1;
        }
        fprintf(stderr, "    where(LSig_%d_%d -> LSig_%d)\n", t_group -> index, t_index - 1, t_group -> index);
        fprintf(stderr, "    where(RSig_%d_%d -> RSig_%d)\n", t_group -> index, t_index - 1, t_group -> index);
        
        t_index = 1;
        fprintf(stderr, "    where(INVOKELIST'nil -> List_%d_0)\n", t_group -> index);
        for(SyntaxRuleRef t_rule = t_group -> rules; t_rule != NULL; t_rule = t_rule -> next)
        {
            const char *t_module, *t_name;
            GetStringOfNameLiteral(t_rule -> module, &t_module);
            GetStringOfNameLiteral(t_rule -> name, &t_name);
            fprintf(stderr, "    Info_%d_%d::INVOKEINFO\n", t_group -> index, t_index);
            fprintf(stderr, "    Info_%d_%d'Index <- -1\n", t_group -> index, t_index);
            fprintf(stderr, "    Info_%d_%d'ModuleIndex <- -1\n", t_group -> index, t_index);
            fprintf(stderr, "    Info_%d_%d'Name <- \"%s\"\n", t_group -> index, t_index, t_name);
            fprintf(stderr, "    Info_%d_%d'ModuleName <- \"%s\"\n", t_group -> index, t_index, t_module);
            fprintf(stderr, "    Info_%d_%d'RSignature <- RSig_%d\n", t_group -> index, t_index, t_group -> index);
            fprintf(stderr, "    Info_%d_%d'LSignature <- LSig_%d\n", t_group -> index, t_index, t_group -> index);
            
            fprintf(stderr, "    where(invokelist(Info_%d_%d, List_%d_%d) -> List_%d_%d)\n", t_group -> index, t_index, t_group -> index, t_index - 1, t_group -> index, t_index);
            t_index += 1;
        }
        fprintf(stderr, "    CustomInvokeList%d <- List_%d_%d\n", t_group -> index, t_group -> index, t_index - 1);
    }
    
    fprintf(stderr, "'action' CustomInvokeLists(INT -> INVOKELIST)\n");
    for(SyntaxRuleGroupRef t_group = s_groups; t_group != NULL; t_group = t_group -> next)
    {
        fprintf(stderr, "  'rule' CustomInvokeLists(Index -> List):\n");
        fprintf(stderr, "    eq(Index, %d)\n", t_group -> index);
        fprintf(stderr, "    CustomInvokeList%d -> List\n", t_group -> index);
    }
}
#endif

void GenerateSyntaxRules(void)
{
    FILE *t_template;
    t_template = OpenTemplateFile();
    
    FILE *t_output;
    t_output = OpenOutputFile();
    
    FILE *t_old_stderr;
    t_old_stderr = stderr;
    
    if (t_output != NULL)
        stderr = t_output;
    else
        t_output = stderr;
    
    if (t_template != NULL)
    {
        while(!feof(t_template))
        {
            char *t_line;
            size_t t_length;
            t_line = fgetln(t_template, &t_length);
            
            if (t_length > 8 && strncmp(t_line, "'module'", 8) == 0)
                fprintf(t_output, "'module' grammar_full\n");
            else
                fprintf(t_output, "%.*s", (int)t_length, t_line);
            
            if (t_length > 5 && strncmp(t_line, "--*--", 5) == 0)
                break;
        }
    }

    int t_index;
    t_index = 0;
    
    for(SyntaxRuleGroupRef t_group = s_groups; t_group != NULL; t_group = t_group -> next)
    {
        SyntaxRuleRef t_rule;
        t_rule = t_group -> rules;
        for(SyntaxRuleRef t_other_rule = t_group -> rules -> next; t_other_rule != NULL; t_other_rule = t_other_rule -> next)
            MergeSyntaxRule(t_rule, t_other_rule);
        
        if (t_rule -> kind == kSyntaxRuleKindPhrase)
            t_rule = t_rule;
        
        t_group -> index = t_index;
        GenerateSyntaxRule(&t_index, t_rule -> expr, t_rule -> kind, t_rule);
        
        if (t_rule -> kind == kSyntaxRuleKindPhrase)
        {
            const char *t_name_string;
            GetStringOfNameLiteral(t_rule -> name, &t_name_string);
            fprintf(stderr, "'nonterm' Custom_%s(-> EXPRESSION)\n", t_name_string);
            fprintf(stderr, "  'rule' Custom_%s(-> Expr):\n", t_name_string);
            fprintf(stderr, "    CustomRule%d(-> Expr)\n", t_rule -> expr -> concrete_rule);
        }
    }
    
    GenerateUmbrellaSyntaxRule("CustomStatements", kSyntaxRuleKindStatement, kSyntaxRuleKindStatement);
    GenerateUmbrellaSyntaxRule("CustomTerms", kSyntaxRuleKindExpression, kSyntaxRuleKindExpression);
    GenerateUmbrellaSyntaxRule("CustomBinaryOperators", kSyntaxRuleKindLeftBinaryOperator, kSyntaxRuleKindNeutralBinaryOperator);
    GenerateUmbrellaSyntaxRule("CustomPrefixOperators", kSyntaxRuleKindPrefixOperator, kSyntaxRuleKindPrefixOperator);
    GenerateUmbrellaSyntaxRule("CustomPostfixOperators", kSyntaxRuleKindPostfixOperator, kSyntaxRuleKindPostfixOperator);
    
    GenerateInvokeLists();
    
    stderr = t_old_stderr;
    
    extern void DumpSyntaxRules(void);
    DumpSyntaxRules();
}

void DumpSyntaxRules(void)
{
    int t_gindex;
    t_gindex = 0;
    for(SyntaxRuleGroupRef t_group = s_groups; t_group != NULL; t_group = t_group -> next)
    {
        for(SyntaxRuleRef t_rule = t_group -> rules; t_rule != NULL; t_rule = t_rule -> next)
        {
            printf("[%d] ", t_gindex);
            PrintSyntaxNode(t_rule -> expr);
            printf("\n");
        }
        t_gindex += 1;
    }
}

////////////////////////////////////////////////////////////////////////////////

