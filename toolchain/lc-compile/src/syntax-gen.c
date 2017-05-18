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
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <allocate.h>
#include <position.h>
#include <report.h>
#include <literal.h>
#include "outputfile.h"

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
    intptr_t index;
    SyntaxNodeKind kind;
    int used;
    SyntaxNodeRef value;
    intptr_t lmode;
    intptr_t rmode;
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
            int reserved;
        } keyword;
        struct
        {
            intptr_t index;
            NameRef rule;
            intptr_t lmode;
            intptr_t rmode;
        } descent;
        struct
        {
            intptr_t index;
            intptr_t value;
        } boolean_mark, integer_mark;
        struct
        {
            intptr_t index;
            double value;
        } real_mark;
        struct
        {
            intptr_t index;
            NameRef value;
        } string_mark;
    };
    
    // Used on output to hold the concrete rule index of this node (for repeat / alternate rules).
    int concrete_rule;
    struct SyntaxNodeMark *marks;
    int mark_count;
    
    int left_trimmed, right_trimmed;
    intptr_t left_lmode, left_rmode;
    intptr_t right_lmode, right_rmode;
};

typedef enum SyntaxMethodType SyntaxMethodType;
enum SyntaxMethodType
{
    kSyntaxMethodTypeExecute,
    kSyntaxMethodTypeEvaluate,
    kSyntaxMethodTypeAssign,
    kSyntaxMethodTypeIterate,
};

typedef struct SyntaxArgument *SyntaxArgumentRef;
struct SyntaxArgument
{
    SyntaxArgumentRef next;
    intptr_t mode;
    intptr_t index;
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
    kSyntaxRuleKindIterator,
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
    intptr_t precedence;
    SyntaxNodeRef expr;
    SyntaxMethodRef methods;
    intptr_t *mapping;
    const char *deprecation_message;
    
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

static intptr_t *s_invoke_lists = NULL;
static intptr_t s_invoke_list_size = 0;

static NameRef *s_unreserved_keywords = NULL;
static unsigned int s_unreserved_keyword_count = 0;

static int IsSyntaxNodeEqualTo(SyntaxNodeRef p_left, SyntaxNodeRef p_right, int p_with_mark_values);

static FILE* s_output;

////////////////////////////////////////////////////////////////////////////////

static void BeginSyntaxRule(NameRef p_module, NameRef p_name, SyntaxRuleKind p_kind, intptr_t p_precedence)
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

void BeginIteratorSyntaxRule(NameRef p_module, NameRef p_name)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindIterator, 0);
}

void BeginExpressionSyntaxRule(NameRef p_module, NameRef p_name)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindExpression, 0);
}

void BeginPrefixOperatorSyntaxRule(NameRef p_module, NameRef p_name, intptr_t p_precedence)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindPrefixOperator, p_precedence);
}

void BeginPostfixOperatorSyntaxRule(NameRef p_module, NameRef p_name, intptr_t p_precedence)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindPostfixOperator, p_precedence);
}

void BeginLeftBinaryOperatorSyntaxRule(NameRef p_module, NameRef p_name, intptr_t p_precedence)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindLeftBinaryOperator, p_precedence);
}

void BeginRightBinaryOperatorSyntaxRule(NameRef p_module, NameRef p_name, intptr_t p_precedence)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindRightBinaryOperator, p_precedence);
}

void BeginNeutralBinaryOperatorSyntaxRule(NameRef p_module, NameRef p_name, intptr_t p_precedence)
{
    BeginSyntaxRule(p_module, p_name, kSyntaxRuleKindNeutralBinaryOperator, p_precedence);
}

void EndSyntaxRule(void)
{
    SyntaxRuleGroupRef t_group;
	assert(s_rule != NULL);
    
    for(t_group = s_groups; t_group != NULL; t_group = t_group -> next)
    {
        if (s_rule -> kind != kSyntaxRuleKindPhrase &&
            IsSyntaxNodeEqualTo(s_rule -> expr, t_group -> rules -> expr, 0) &&
            s_rule -> kind == t_group -> rules -> kind)
        {
            const char *t_rule_name, *t_other_rule_name;
            GetStringOfNameLiteral(s_rule -> name, &t_rule_name);
            GetStringOfNameLiteral(t_group -> rules -> name, &t_other_rule_name);
            if (s_rule -> precedence != t_group -> rules -> precedence)
            {
                Error_Bootstrap("Rule '%s' and '%s' have conflicting precedence", t_rule_name, t_other_rule_name);
                break;
            }
            
            if (!IsSyntaxNodeEqualTo(s_rule -> expr, t_group -> rules -> expr, 1))
            {
                Error_Bootstrap("Rule '%s' and '%s' have conflicting methods", t_rule_name, t_other_rule_name);
                break;
            }
            
            break;
        }
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

void DeprecateSyntaxRule(const char *p_message)
{
	assert(s_rule != NULL);
    s_rule -> deprecation_message = p_message;
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

static int IsMarkSyntaxNodeEqualTo(SyntaxNodeRef p_left, SyntaxNodeRef p_right)
{
    if (p_left -> kind != p_right -> kind)
        return 0;
    
    switch(p_left -> kind)
    {
        case kSyntaxNodeKindBooleanMark:
            if (p_left -> boolean_mark  . value != p_right -> boolean_mark . value)
                return 0;
            break;
            
        case kSyntaxNodeKindIntegerMark:
            if (p_left -> integer_mark  . value != p_right -> integer_mark . value)
                return 0;
            break;
            
        case kSyntaxNodeKindRealMark:
            if (p_left -> real_mark  . value != p_right -> real_mark . value)
                return 0;
            break;
            
        case kSyntaxNodeKindStringMark:
            if (p_left -> string_mark . value != p_right -> string_mark . value)
                return 0;
            break;
            
        default:
            return 0;
    }
    
    return 1;
}

static int IsSyntaxNodeEqualTo(SyntaxNodeRef p_left, SyntaxNodeRef p_right, int p_with_mark_values)
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
            if (p_left -> boolean_mark . index != p_right -> boolean_mark . index)
                return 0;
            if (p_with_mark_values == 0)
                return 1;
            if (!IsMarkSyntaxNodeEqualTo(p_left, p_right))
                return 0;
            break;
            
        case kSyntaxNodeKindConcatenate:
		{
            unsigned int i;
			for(i = 0; 1; i++)
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
                    if (!IsSyntaxNodeEqualTo(t_left_child, t_right_child, p_with_mark_values))
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
		}
        case kSyntaxNodeKindAlternate:
		{
			unsigned int i, t_found;
            if (p_left -> alternate . operand_count != p_right -> alternate . operand_count)
                return 0;
            if (p_left -> alternate . is_nullable != p_right -> alternate . is_nullable)
                return 0;
            t_found = 0;
            for(i = 0; i < p_left -> alternate . operand_count; i++)
            {
                unsigned int j;
                for(j = 0; j < p_right -> alternate . operand_count; j++)
                    if (IsSyntaxNodeEqualTo(p_left -> alternate . operands[i], p_right -> alternate . operands[j], p_with_mark_values))
                    {
                        t_found += 1;
                        break;
                    }
            }
            if (t_found != p_left -> alternate . operand_count)
                return 0;
            break;
		}
        case kSyntaxNodeKindRepeat:
            if (!IsSyntaxNodeEqualTo(p_left -> repeat . element, p_right -> repeat . element, p_with_mark_values))
                return 0;
            if (!IsSyntaxNodeEqualTo(p_left -> repeat . delimiter, p_right -> repeat . delimiter, p_with_mark_values))
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
		{
            unsigned int i;
			for(i = 0; i < p_node -> concatenate . operand_count; i++)
                FreeSyntaxNode(p_node -> concatenate . operands[i]);
            free(p_node -> concatenate . operands);
            break;
		}
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
    int t_new_count;
	
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
    
    t_new_count = p_target -> concatenate . operand_count + 1;
    p_target -> concatenate . operands = (SyntaxNodeRef *)Reallocate(p_target -> concatenate . operands, t_new_count * sizeof(SyntaxNodeRef));
    p_target -> concatenate . operands[p_target -> concatenate . operand_count++] = p_node;
}

static void PrependSyntaxNode(SyntaxNodeRef p_target, SyntaxNodeRef p_node)
{
    int i, t_new_count;
	
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

    t_new_count = p_target -> concatenate . operand_count + 1;
    p_target -> concatenate . operands = (SyntaxNodeRef *)Reallocate(p_target -> concatenate . operands, t_new_count * sizeof(SyntaxNodeRef));
    for(i = t_new_count; i > 1; i--)
        p_target -> concatenate . operands[i - 1] = p_target -> concatenate . operands[i - 2];
    p_target -> concatenate . operands[0] = p_node;
    p_target -> concatenate . operand_count++;
}

static void JoinSyntaxNodes(SyntaxNodeRef p_left, SyntaxNodeRef p_right)
{
    unsigned int i;

	assert(p_left -> kind == kSyntaxNodeKindConcatenate || p_left -> kind == kSyntaxNodeKindAlternate);
    assert(p_left -> kind == p_right -> kind);
    
    for(i = 0; i < p_right -> concatenate . operand_count; i++)
        AppendSyntaxNode(p_left, p_right -> concatenate . operands[i]);
    
    p_right -> concatenate . operand_count = 0;
    
    if (p_right -> concatenate . is_nullable == 1)
        p_left -> concatenate . is_nullable = 1;

    FreeSyntaxNode(p_right);
}

static intptr_t CountSyntaxNodeMarks(SyntaxNodeRef p_node)
{
    intptr_t t_index = 0;
    switch(p_node -> kind)
    {
        case kSyntaxNodeKindEmpty:
            t_index = 0;
            break;
        case kSyntaxNodeKindKeyword:
            t_index = 0;
            break;
        case kSyntaxNodeKindDescent:
            // We are counting the number of marks - we want index + 1 (since index can be 0)
            t_index = p_node -> descent . index == -1 ? 0 : p_node -> descent . index + 1;
            break;
        case kSyntaxNodeKindBooleanMark:
        case kSyntaxNodeKindIntegerMark:
        case kSyntaxNodeKindRealMark:
        case kSyntaxNodeKindStringMark:
            // We are counting the number of marks - we want index + 1 (since index can be 0)
            t_index = p_node -> boolean_mark . index + 1;
            break;
        case kSyntaxNodeKindConcatenate:
        case kSyntaxNodeKindAlternate:
		{
            unsigned int i;
			t_index = 0;
            for(i = 0; i < p_node -> concatenate . operand_count; i++)
            {
                intptr_t t_node_index;
                t_node_index = CountSyntaxNodeMarks(p_node -> concatenate . operands[i]);
                if (t_node_index > t_index)
                    t_index = t_node_index;
            }
            break;
		}
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
		{
            unsigned int i;
			printf("(");
            for(i = 0; i < p_node -> concatenate . operand_count; i++)
            {
                if (i > 0)
                    printf(".");
				if (NULL != p_node -> concatenate . operands[i])
					PrintSyntaxNode(p_node -> concatenate . operands[i]);
				else
					printf("<removed>");
            }
            printf(")");
			break;
		}
        case kSyntaxNodeKindAlternate:
		{
			unsigned int i;
            printf(p_node -> alternate . is_nullable ? "[" : "(");
            for(i = 0; i < p_node -> concatenate . operand_count; i++)
            {
                if (i > 0)
                    printf("|");
                PrintSyntaxNode(p_node -> concatenate . operands[i]);
            }
            printf(p_node -> alternate . is_nullable ? "]" : ")");
            break;
		}
        case kSyntaxNodeKindRepeat:
            printf("{");
            PrintSyntaxNode(p_node -> repeat . element);
            printf(",");
            PrintSyntaxNode(p_node -> repeat . delimiter);
            printf("}");
            break;
    }
}

static void RemoveFirstTermFromSyntaxNode(SyntaxNodeRef p_node, intptr_t *r_lmode, intptr_t *r_rmode)
{
    if (p_node -> kind == kSyntaxNodeKindAlternate)
    {
        unsigned int i;
		for(i = 0; i < p_node -> alternate . operand_count; i++)
            RemoveFirstTermFromSyntaxNode(p_node -> alternate . operands[i], r_lmode, r_rmode);
    }
    else if (p_node -> kind == kSyntaxNodeKindConcatenate)
    {
        unsigned int i;
        if (p_node -> concatenate . operands[0] -> kind != kSyntaxNodeKindAlternate)
        {
            p_node -> left_trimmed = 1;
            *r_lmode = p_node -> concatenate . operands[0] -> descent . lmode;
            *r_rmode = p_node -> concatenate . operands[0] -> descent . rmode;
            FreeSyntaxNode(p_node -> concatenate . operands[0]);
            for(i = 1; i < p_node -> concatenate . operand_count; i++)
                p_node -> concatenate . operands[i - 1] = p_node -> concatenate . operands[i];
            p_node -> concatenate . operand_count -= 1;
        }
        else
            RemoveFirstTermFromSyntaxNode(p_node -> concatenate . operands[0], r_lmode, r_rmode);
    }
    else
        assert(0);
}

static void RemoveLastTermFromSyntaxNode(SyntaxNodeRef p_node, intptr_t *r_lmode, intptr_t *r_rmode)
{
    if (p_node -> kind == kSyntaxNodeKindAlternate)
    {
        unsigned int i;
		for(i = 0; i < p_node -> alternate . operand_count; i++)
            RemoveLastTermFromSyntaxNode(p_node -> alternate . operands[i], r_lmode, r_rmode);
    }
    else if (p_node -> kind == kSyntaxNodeKindConcatenate)
    {
        p_node -> right_trimmed = 1;
        if (p_node -> concatenate . operands[p_node -> concatenate . operand_count - 1] -> kind != kSyntaxNodeKindAlternate)
        {
            *r_lmode = p_node -> concatenate . operands[p_node -> concatenate . operand_count - 1] -> descent . lmode;
            *r_rmode = p_node -> concatenate . operands[p_node -> concatenate . operand_count - 1] -> descent . rmode;
            FreeSyntaxNode(p_node -> concatenate . operands[p_node -> concatenate . operand_count - 1]);
            p_node -> concatenate . operand_count -= 1;
        }
        else
            RemoveLastTermFromSyntaxNode(p_node -> concatenate . operands[p_node -> concatenate . operand_count - 1], r_lmode, r_rmode);
    }
    else
        assert(0);
}

static void TrimSyntaxNodeForOperator(SyntaxNodeRef p_node, SyntaxRuleKind p_kind)
{
    int t_remove_left;
	int t_remove_right;
    t_remove_left = 0;
	t_remove_right = 0;

    if (p_kind == kSyntaxRuleKindLeftBinaryOperator ||
        p_kind == kSyntaxRuleKindRightBinaryOperator ||
        p_kind == kSyntaxRuleKindNeutralBinaryOperator ||
        p_kind == kSyntaxRuleKindPostfixOperator)
        t_remove_left = 1;
    
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
    SyntaxNodeRef t_left, t_right;
	SyntaxNodeRef t_result;

	assert(s_rule != NULL);
    assert(s_stack_index >= 2);
    
    t_left = s_stack[s_stack_index - 2];
    t_right = s_stack[s_stack_index - 1];
    
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
    SyntaxNodeRef t_node;

	assert(s_rule != NULL);
    assert(s_stack_index >= 2);
    
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
	SyntaxNodeRef t_node;

    MakeNameLiteral(p_keyword, &t_name);
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindKeyword;
    t_node -> keyword . token = t_name;
    t_node -> keyword . reserved = 1;
    PushSyntaxNode(t_node);
}

void PushUnreservedKeywordSyntaxGrammar(const char *p_keyword)
{
    NameRef t_name;
	SyntaxNodeRef t_node;
    
    MakeNameLiteral(p_keyword, &t_name);
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindKeyword;
    t_node -> keyword . token = t_name;
    t_node -> keyword . reserved = 0;
    PushSyntaxNode(t_node);
}

void PushMarkedDescentSyntaxGrammar(intptr_t p_mark, NameRef p_rule, intptr_t p_lmode, intptr_t p_rmode)
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

void PushMarkedTrueSyntaxGrammar(intptr_t p_mark)
{
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindBooleanMark;
    t_node -> boolean_mark . index = p_mark;
    t_node -> boolean_mark . value = 1;
    PushSyntaxNode(t_node);
}

void PushMarkedFalseSyntaxGrammar(intptr_t p_mark)
{
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindBooleanMark;
    t_node -> boolean_mark . index = p_mark;
    t_node -> boolean_mark . value = 0;
    PushSyntaxNode(t_node);
}

void PushMarkedIntegerSyntaxGrammar(intptr_t p_mark, intptr_t p_value)
{
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindIntegerMark;
    t_node -> integer_mark . index = p_mark;
    t_node -> integer_mark . value = p_value;
    PushSyntaxNode(t_node);
}

void PushMarkedRealSyntaxGrammar(intptr_t p_mark, intptr_t p_value)
{
    SyntaxNodeRef t_node;
    MakeSyntaxNode(&t_node);
    t_node -> kind = kSyntaxNodeKindRealMark;
    t_node -> real_mark . index = p_mark;
    t_node -> real_mark . value = *(double *)p_value;
    PushSyntaxNode(t_node);
}

void PushMarkedStringSyntaxGrammar(intptr_t p_mark, const char *p_value)
{
    NameRef t_name;
	SyntaxNodeRef t_node;

    MakeNameLiteral(p_value, &t_name);
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

void BeginIterateMethodSyntaxMapping(NameRef p_name)
{
    BeginMethodSyntaxMapping(kSyntaxMethodTypeIterate, p_name);
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

void PushIntegerArgumentSyntaxMapping(intptr_t p_value)
{
}

void PushRealArgumentSyntaxMapping(double *p_value)
{
}

void PushStringArgumentSyntaxMapping(const char *p_value)
{
}

static void PushMarkArgumentSyntaxMapping(intptr_t p_mode, intptr_t p_index)
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

void PushInMarkArgumentSyntaxMapping(intptr_t p_index)
{
    PushMarkArgumentSyntaxMapping(0, p_index);
}

void PushOutMarkArgumentSyntaxMapping(intptr_t p_index)
{
    PushMarkArgumentSyntaxMapping(1, p_index);
}

void PushInOutMarkArgumentSyntaxMapping(intptr_t p_index)
{
    PushMarkArgumentSyntaxMapping(2, p_index);
}

////////////////////////////////////////////////////////////////////////////////

void AddUnreservedSyntaxKeyword(intptr_t p_name)
{
    s_unreserved_keywords = Reallocate(s_unreserved_keywords, sizeof(NameRef) * (s_unreserved_keyword_count + 1));
    s_unreserved_keywords[s_unreserved_keyword_count++] = (NameRef)p_name;
}

static void ListSyntaxNodeUnreservedKeywords(SyntaxNodeRef p_node, NameRef** x_tokens, intptr_t *x_token_count)
{
    switch(p_node -> kind)
    {
        case kSyntaxNodeKindEmpty:
            break;
        case kSyntaxNodeKindKeyword:
        {
            if (!p_node -> keyword . reserved)
            {
                *x_tokens = Reallocate(*x_tokens, sizeof((*x_tokens)[0]) * (*x_token_count + 1));
                (*x_tokens)[*x_token_count] = p_node -> keyword . token;
                (*x_token_count) += 1;
            }
        }
        break;
        case kSyntaxNodeKindDescent:
        break;
        case kSyntaxNodeKindBooleanMark:
        case kSyntaxNodeKindIntegerMark:
        case kSyntaxNodeKindStringMark:
        case kSyntaxNodeKindRealMark:
            break;
        case kSyntaxNodeKindConcatenate:
        case kSyntaxNodeKindAlternate:
		{
            unsigned int i;
            for(i = 0; i < p_node -> alternate . operand_count; i++)
                ListSyntaxNodeUnreservedKeywords(p_node -> alternate . operands[i], x_tokens, x_token_count);
            break;
		}
        case kSyntaxNodeKindRepeat:
            if (p_node -> repeat . delimiter != NULL)
                ListSyntaxNodeUnreservedKeywords(p_node -> repeat . delimiter, x_tokens, x_token_count);
            ListSyntaxNodeUnreservedKeywords(p_node -> repeat . element, x_tokens, x_token_count);
            break;
    }
}

static void MergeSyntaxNodes(SyntaxNodeRef p_node, SyntaxNodeRef p_other_node, intptr_t *x_next_mark, intptr_t *x_mapping)
{
    if (p_node -> kind == kSyntaxNodeKindAlternate)
    {
        unsigned int i;
		for(i = 0; i < p_node -> alternate . operand_count; i++)
        {
            unsigned int j;
			for(j = 0; j < p_other_node -> alternate . operand_count; j++)
                if (IsSyntaxNodeEqualTo(p_node -> alternate . operands[i], p_other_node -> alternate . operands[j], 0))
                    MergeSyntaxNodes(p_node -> alternate . operands[i], p_other_node -> alternate . operands[j], x_next_mark, x_mapping);
        }
    }
    else if (p_node -> kind == kSyntaxNodeKindConcatenate)
    {
        unsigned int i;
		for(i = 0; i < p_other_node -> concatenate . operand_count; i++)
        {
            SyntaxNodeRef t_child, t_other_child;
            t_child = p_node -> concatenate . operands[i];
            t_other_child = p_other_node -> concatenate . operands[i];
            if (!IsMarkSyntaxNode(t_other_child))
            {
                MergeSyntaxNodes(t_child, t_other_child, x_next_mark, x_mapping);
                continue;
            }
            
            assert(IsMarkSyntaxNode(t_child));
            assert(t_child -> boolean_mark . index == t_other_child -> boolean_mark . index);
        }
    }
    else if (p_node -> kind == kSyntaxNodeKindRepeat)
    {
        MergeSyntaxNodes(p_node -> repeat . element, p_other_node -> repeat . element, x_next_mark, x_mapping);
    }
    else if (p_node -> kind == kSyntaxNodeKindDescent)
    {
        assert(p_other_node -> descent . index != -1);
        assert(p_other_node -> descent . index == p_node -> descent . index);
    }
}

static void MergeSyntaxRule(SyntaxRuleRef p_rule, SyntaxRuleRef p_other_rule)
{
    intptr_t t_rule_mark_count, t_other_rule_mark_count;
	intptr_t *t_mark_mapping;

    t_rule_mark_count = CountSyntaxNodeMarks(p_rule -> expr);
    t_other_rule_mark_count = CountSyntaxNodeMarks(p_other_rule -> expr);
    
    t_mark_mapping = (intptr_t *)Allocate(t_other_rule_mark_count * sizeof(intptr_t));
    
    MergeSyntaxNodes(p_rule -> expr, p_other_rule -> expr, &t_rule_mark_count, t_mark_mapping);
    
    p_other_rule -> mapping = t_mark_mapping;
}

static void SetSyntaxNodeMarkAsUsed(struct SyntaxNodeMark *p_marks, int p_mark_count, intptr_t p_index, SyntaxNodeRef p_value)
{
    int i;
	for(i = 0; i < p_mark_count; i++)
        if (p_marks[i] . index == p_index)
        {
            p_marks[i] . used = 1;
            p_marks[i] . value = p_value;
            break;
        }
}

static void SetAllSyntaxNodeMarksAsUnused(struct SyntaxNodeMark *p_marks, int p_mark_count)
{
    int i;
	for(i = 0; i < p_mark_count; i++)
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
                fprintf(s_output, "\"%s\"", t_string);
            }
            break;
        case kSyntaxNodeKindDescent:
            {
                const char *t_string;
                GetStringOfNameLiteral(p_node -> descent . rule, &t_string);
                if (strcmp(t_string, "Expression") != 0 &&
                    strcmp(t_string, "ExpressionList") != 0)
                    fprintf(s_output, "Custom_");
                if (strcmp(t_string, "ExpressionList") == 0)
                    fprintf(s_output, "ExpressionListAsExpression");
                else
                    fprintf(s_output, "%s", t_string);
                if (p_node -> descent . index != -1)
                {
                    fprintf(s_output, "(-> Mark%ld)", p_node -> descent . index);
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
		{
            int i;
			fprintf(s_output, "CustomRule%d(-> ", p_node -> concrete_rule);
            for(i = 0; i < p_node -> mark_count; i++)
            {
                if (i != 0)
                    fprintf(s_output, ", ");
                fprintf(s_output, "Mark%ld", p_node -> marks[i] . index);
                SetSyntaxNodeMarkAsUsed(p_marks, p_mark_count, p_node -> marks[i] . index, NULL);
            }
            fprintf(s_output, ")");
            break;
		}
    }
}

static void AddSyntaxNodeMark(struct SyntaxNodeMark **x_marks, int *x_mark_count, intptr_t p_index, SyntaxNodeKind p_kind, intptr_t p_lmode, intptr_t p_rmode)
{
    int i;
	for(i = 0; i < *x_mark_count; i++)
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
		{
            unsigned int i;
			for(i = 0; i < p_node -> concatenate . operand_count; i++)
                GenerateSyntaxRuleMarks(p_node -> concatenate . operands[i], x_marks, x_mark_count);
            break;
		}
        case kSyntaxNodeKindRepeat:
            break;
    }
}

static void GenerateSyntaxRuleSubHeader(SyntaxNodeRef p_node, SyntaxRuleKind p_kind)
{
    if (p_kind >= kSyntaxRuleKindPrefixOperator)
        fprintf(s_output, "  'rule' CustomRule%d(-> Sentinal):\n", p_node -> concrete_rule);
    else
    {
        fprintf(s_output, "  'rule' CustomRule%d(-> ", p_node -> concrete_rule);
        if (p_kind == kSyntaxRuleKindFragment)
        {
            int i;
			for(i = 0; i < p_node -> mark_count; i++)
            {
                if (i != 0)
                    fprintf(s_output, ", ");
                fprintf(s_output, "Mark%ld", p_node -> marks[i] . index);
            }
        }
        else
        {
            int i;
			fprintf(s_output, "invoke(Position, Invoke, ");
            for(i = 0; i < p_node -> mark_count; i++)
                fprintf(s_output, "expressionlist(Mark%ld, ", p_node -> marks[i] . index);
            fprintf(s_output, "nil");
            for(i = 0; i < p_node -> mark_count; i++)
                fprintf(s_output, ")");
            fprintf(s_output, ")");
        }
        fprintf(s_output, "):\n");
    }
}

static void GenerateSyntaxRuleExplicitAndUnusedMarks(SyntaxNodeRef p_node)
{
    int i;
	int t_has_pos;
    t_has_pos = 0;
    for(i = 0; i < p_node -> mark_count; i++)
    {
        if (p_node -> marks[i] . used == 0)
            fprintf(s_output, "    where(EXPRESSION'nil -> Mark%ld)\n", p_node -> marks[i] . index);
        else if (p_node -> marks[i] . value != NULL)
        {
            if (t_has_pos == 0)
            {
                fprintf(s_output, "    GetUndefinedPosition(-> UndefinedPosition)\n");
                t_has_pos = 1;
            }
            if (p_node -> marks[i] . value -> kind == kSyntaxNodeKindRealMark)
            {
                fprintf(s_output, "    MakeDoubleLiteral(\"%.18lf\" -> Mark%ldValue)\n", p_node -> marks[i] . value -> real_mark . value, p_node -> marks[i] . index);
            }
            fprintf(s_output, "    where(");
            switch(p_node -> marks[i] . value -> kind)
            {
                case kSyntaxNodeKindBooleanMark:
                    fprintf(s_output, "EXPRESSION'%s(UndefinedPosition)", p_node -> marks[i] . value -> boolean_mark . value == 0 ? "false" : "true");
                    break;
                case kSyntaxNodeKindIntegerMark:
                    if (p_node -> marks[i] . value -> integer_mark . value < 0)
                        fprintf(s_output, "EXPRESSION'integer(UndefinedPosition, %ld)", p_node -> marks[i] . value -> integer_mark . value);
                    else
                        fprintf(s_output, "EXPRESSION'unsignedinteger(UndefinedPosition, %lu)", (uintptr_t)p_node -> marks[i] . value -> integer_mark . value);
                    break;
                case kSyntaxNodeKindRealMark:
                    fprintf(s_output, "EXPRESSION'real(UndefinedPosition, Mark%ldValue)", p_node -> marks[i] . index);
                    break;
                case kSyntaxNodeKindStringMark:
                    {
                        const char *t_string;
                        GetStringOfNameLiteral(p_node -> marks[i] . value -> string_mark . value, &t_string);
                        fprintf(s_output, "EXPRESSION'string(UndefinedPosition, \"%s\")", t_string);
                    }
                    break;
                default:
                    assert(0);
                    break;
            }
            fprintf(s_output, " -> Mark%ld)\n", p_node -> marks[i] . index);
        }
            
    }
}

static void GenerateSyntaxRuleConstructor(SyntaxNodeRef p_node, SyntaxRuleRef p_rule)
{
    if (p_rule != NULL && p_rule -> kind >= kSyntaxRuleKindPrefixOperator)
    {
        int i;
		static const char *s_calls[] = { "PushOperatorExpressionPrefix", "PushOperatorExpressionPostfix", "PushOperatorExpressionLeftBinary", "PushOperatorExpressionRightBinary", "PushOperatorExpressionNeutralBinary" };
        fprintf(s_output, "    %s(Position, %ld, %d -> Sentinal)\n", s_calls[p_rule -> kind - kSyntaxRuleKindPrefixOperator],
                p_rule -> precedence, p_node -> concrete_rule);
        for(i = 0; i < p_node -> mark_count; i++)
            fprintf(s_output, "    PushOperatorExpressionArgument(Mark%ld -> _)\n", p_node -> marks[i] . index);
    }
    else if (p_rule != NULL)
    {
        fprintf(s_output, "    CustomInvokeLists(%d -> Invoke)\n", p_node -> concrete_rule);
    }
}

static void GenerateSyntaxRule(int *x_index, SyntaxNodeRef p_node, SyntaxRuleKind p_kind, SyntaxRuleRef p_rule)
{
    int t_index;
	
	assert(p_node -> kind == kSyntaxNodeKindConcatenate ||
           p_node -> kind == kSyntaxNodeKindAlternate ||
           p_node -> kind == kSyntaxNodeKindRepeat);
    
    t_index = *x_index;
    *x_index += 1;
    
    if (p_node -> kind == kSyntaxNodeKindConcatenate)
    {
        unsigned int i;
		for(i = 0; i < p_node -> alternate . operand_count; i++)
            if (p_node -> alternate . operands[i] -> kind == kSyntaxNodeKindAlternate ||
                p_node -> alternate . operands[i] -> kind == kSyntaxNodeKindRepeat)
                GenerateSyntaxRule(x_index, p_node -> alternate . operands[i], kSyntaxRuleKindFragment, NULL);
    }
    else if (p_node -> kind == kSyntaxNodeKindAlternate)
    {
        unsigned int i;
		for(i = 0; i < p_node -> alternate . operand_count; i++)
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
        int i;
		fprintf(s_output, "'nonterm' CustomRule%d(-> ", t_index);
        for(i = 0; i < p_node -> mark_count; i++)
        {
            if (i != 0)
                fprintf(s_output, ", ");
            fprintf(s_output, "EXPRESSION");
        }
        fprintf(s_output, ")\n");
    }
    else if (p_kind >= kSyntaxRuleKindPrefixOperator)
        fprintf(s_output, "'nonterm' CustomRule%d(-> INT)\n", t_index);
    else
        fprintf(s_output, "'nonterm' CustomRule%d(-> %s)\n", t_index, p_kind == kSyntaxRuleKindStatement ? "STATEMENT" : "EXPRESSION");
    
    if (p_node -> kind == kSyntaxNodeKindConcatenate)
    {
        unsigned int i;
		
		SetAllSyntaxNodeMarksAsUnused(p_node -> marks, p_node -> mark_count);
        
        GenerateSyntaxRuleSubHeader(p_node, p_kind);
        fprintf(s_output, "    ");
        for(i = 0; i < p_node -> concatenate . operand_count; i++)
        {
            if (i != 0)
                fprintf(s_output, " ");
            GenerateSyntaxRuleTerm(p_node -> concatenate . operands[i], p_node -> marks, p_node -> mark_count);
            if (i == 0)
                fprintf(s_output, " @(-> Position)");
        }
        fprintf(s_output, "\n");
        GenerateSyntaxRuleExplicitAndUnusedMarks(p_node);
        if (p_rule != NULL)
            GenerateSyntaxRuleConstructor(p_node, p_rule);
    }
    else if (p_node -> kind == kSyntaxNodeKindAlternate)
    {
        unsigned int i;
		for(i = 0; i < p_node -> alternate . operand_count; i++)
        {
            SetAllSyntaxNodeMarksAsUnused(p_node -> marks, p_node -> mark_count);
            
            GenerateSyntaxRuleSubHeader(p_node, p_kind);
            fprintf(s_output, "    ");
            if (p_node -> alternate . operands[i] -> kind != kSyntaxNodeKindConcatenate)
                GenerateSyntaxRuleTerm(p_node -> alternate . operands[i], p_node -> marks, p_node -> mark_count);
            else
            {
                unsigned int j;
				for(j = 0; j < p_node -> alternate . operands[i] -> concatenate . operand_count; j++)
                {
                    if (j != 0)
                        fprintf(s_output, " ");
                    GenerateSyntaxRuleTerm(p_node -> alternate . operands[i] -> concatenate . operands[j], p_node -> marks, p_node -> mark_count);
                    if (j == 0)
                        fprintf(s_output, " @(-> Position)");
                }
            }
            fprintf(s_output, "\n");
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
        SyntaxRuleGroupRef t_group;
		
		fprintf(s_output, "'nonterm' %s(-> %s)\n", p_name, p_first == kSyntaxRuleKindStatement ? "STATEMENT" : "EXPRESSION");
        
        for(t_group = s_groups; t_group != NULL; t_group = t_group -> next)
        {
            SyntaxRuleRef t_rule;
            t_rule = t_group -> rules;

            if (t_rule -> kind >= p_first && t_rule -> kind <= p_last)
            {
                fprintf(s_output, "  'rule' %s(-> Node):\n", p_name);
                fprintf(s_output, "    CustomRule%d(-> Node)\n", t_rule -> expr -> concrete_rule);
                t_made = 1;
            }
        }
        if (t_made == 0)
            fprintf(s_output, "  'rule' %s(-> nil):\n    \"__%s\"\n", p_name, p_name);
    }
    else
    {
        SyntaxRuleGroupRef t_group;
		
		fprintf(s_output, "'nonterm' %s(-> INT)\n", p_name);
        
        for (t_group = s_groups; t_group != NULL; t_group = t_group -> next)
        {
            SyntaxRuleRef t_rule;
            t_rule = t_group -> rules;
            
            if (t_rule -> kind >= p_first && t_rule -> kind <= p_last)
            {
                fprintf(s_output, "  'rule' %s(-> Sentinal):\n", p_name);
                fprintf(s_output, "    CustomRule%d(-> Sentinal)\n", t_rule -> expr -> concrete_rule);
                t_made = 1;
            }
        }
        if (t_made == 0)
            fprintf(s_output, "  'rule' %s(-> 10000):\n    \"__%s\"\n", p_name, p_name);
    }
}

static const char *s_invoke_modes[] = { "In_Mode", "Out_Mode", "InOut_Mode" };
static void GenerateInvokeMethodArg(SyntaxArgumentRef p_arg)
{
	if (p_arg == NULL)
    {
        fprintf(s_output, "Signature_Nil");
        return;
    }
    
    fprintf(s_output, "invokesignature(%s, %ld, ", s_invoke_modes[p_arg -> mode], p_arg -> index);
    GenerateInvokeMethodArg(p_arg -> next);
    fprintf(s_output, ")");
}

static void GenerateInvokeMethodList(int p_index, int p_method_index, SyntaxMethodRef p_method)
{
    static const char *s_method_types[] = { "Execute_Type", "Evaluate_Type", "Assign_Type", "Iterate_Type" };
    const char *t_name;
	int t_arity;
    SyntaxArgumentRef t_arg;
    
	if (p_method == NULL)
    {
        fprintf(s_output, "    where(MethodList_Nil -> MethodList_%d_%d)\n", p_index, p_method_index);
        return;
    }
    
    if (p_method -> next != NULL)
        GenerateInvokeMethodList(p_index, p_method_index + 1, p_method -> next);
    
    for(t_arity = 0, t_arg = p_method -> arguments; t_arg != NULL; t_arity += 1, t_arg = t_arg -> next)
        ;
    
    if (t_arity > 0 && t_arity <= 3)
    {
        fprintf(s_output, "    MakeCustomInvokeMethodArgs%d(", t_arity);
        for(t_arg = p_method -> arguments; t_arg != NULL; t_arg = t_arg -> next)
        {
            fprintf(s_output, "%s, %ld", s_invoke_modes[t_arg -> mode], t_arg -> index);
            if (t_arg -> next != NULL)
                fprintf(s_output, ", ");
        }
        fprintf(s_output, " -> MethodListArgs_%d_%d)\n", p_index, p_method_index);
    }
    
    GetStringOfNameLiteral(p_method -> name, &t_name);
    fprintf(s_output, "    MakeCustomInvokeMethodList(\"%s\", %s, ", t_name, s_method_types[p_method -> type]);
    if (t_arity == 0)
    {
        fprintf(s_output, " Signature_Nil");
    }
    else if (t_arity < 3)
        fprintf(s_output, "  MethodListArgs_%d_%d", p_index, p_method_index);
    else
        GenerateInvokeMethodArg(p_method -> arguments);
    if (p_method -> next != NULL)
        fprintf(s_output, ", MethodList_%d_%d -> MethodList_%d_%d)\n", p_index, p_method_index + 1, p_index, p_method_index);
    else
        fprintf(s_output, ", MethodList_Nil -> MethodList_%d_%d)\n", p_index, p_method_index);
}

static void GenerateInvokeLists(void)
{
    int t_method_index = 1;
	SyntaxRuleGroupRef t_group;
    
    int t_entry_count;
    t_entry_count = 0;
    
    fprintf(s_output, "'action' InitializeCustomInvokeLists()\n");
    fprintf(s_output, "  'rule' InitializeCustomInvokeLists():\n");
    for (t_group = s_groups; t_group != NULL; t_group = t_group -> next)
    {
        int t_index = 1;
		SyntaxRuleRef t_rule;
        
        if ((t_entry_count % 10) == 0)
        {
            fprintf(s_output, "    InitializeCustomInvokeLists_%d\n\n", t_entry_count);
            fprintf(s_output, "'action' InitializeCustomInvokeLists_%d\n", t_entry_count);
            fprintf(s_output, "  'rule' InitializeCustomInvokeLists_%d\n", t_entry_count);
            fprintf(s_output, "    where(INVOKELIST'nil -> List_Nil)\n");
            fprintf(s_output, "    where(INVOKEMETHODLIST'nil -> MethodList_Nil)\n");
            fprintf(s_output, "    where(INVOKESIGNATURE'nil -> Signature_Nil)\n");
            fprintf(s_output, "    where(MODE'in -> In_Mode)\n");
            fprintf(s_output, "    where(MODE'out -> Out_Mode)\n");
            fprintf(s_output, "    where(MODE'inout -> InOut_Mode)\n");
            fprintf(s_output, "    where(INVOKEMETHODTYPE'execute -> Execute_Type)\n");
            fprintf(s_output, "    where(INVOKEMETHODTYPE'evaluate -> Evaluate_Type)\n");
            fprintf(s_output, "    where(INVOKEMETHODTYPE'assign -> Assign_Type)\n");
            fprintf(s_output, "    where(INVOKEMETHODTYPE'iterate -> Iterate_Type)\n");
        }
        
        for (t_rule = t_group -> rules; t_rule != NULL; t_rule = t_rule -> next)
        {
            const char *t_module, *t_name;
			
			GenerateInvokeMethodList(t_method_index, 0, t_rule -> methods);
            
            GetStringOfNameLiteral(t_rule -> module, &t_module);
            GetStringOfNameLiteral(t_rule -> name, &t_name);
            if (t_index - 1 > 0)
                fprintf(s_output, "    MakeCustomInvokeList(\"%s\", \"%s\", MethodList_%d_%d, List_%d_%d -> List_%d_%d)\n",
                    t_name, t_module, t_method_index, 0, t_group -> index, t_index - 1, t_group -> index, t_index);
            else
                fprintf(s_output, "    MakeCustomInvokeList(\"%s\", \"%s\", MethodList_%d_%d, List_Nil -> List_%d_%d)\n",
                        t_name, t_module, t_method_index, 0, t_group -> index, t_index);
            
            t_index += 1;
            t_method_index += 1;
        }
        fprintf(s_output, "    DefineCustomInvokeList(%d, List_%d_%d)\n", t_group -> index, t_group -> index, t_index - 1);
        
        t_entry_count += 1;
    }
}

static void GenerateTokenList(void)
{
    NameRef *t_tokens;
    intptr_t t_token_count;
	SyntaxRuleGroupRef t_group;
	intptr_t i;
	unsigned int j;
	const char *t_string;

    t_tokens = NULL;
    t_token_count = 0;
    for(t_group = s_groups; t_group != NULL; t_group = t_group -> next)
        ListSyntaxNodeUnreservedKeywords(t_group -> rules -> expr, &t_tokens, &t_token_count);
    
    fprintf(s_output, "'nonterm' CustomKeywords(-> STRING)\n");
    if (t_token_count != 0 || s_unreserved_keyword_count != 0)
    {
        for(i = 0; i < t_token_count; i++)
        {
            GetStringOfNameLiteral(t_tokens[i], &t_string);
            fprintf(s_output, "  'rule' CustomKeywords(-> String):\n");
            fprintf(s_output, "    \"%s\"\n", t_string);
            fprintf(s_output, "    where(\"%s\" -> String)\n", t_string);
        }
        for(j = 0; j < s_unreserved_keyword_count; j++)
        {
            GetStringOfNameLiteral(s_unreserved_keywords[j], &t_string);
            fprintf(s_output, "  'rule' CustomKeywords(-> String):\n");
            fprintf(s_output, "    \"%s\"\n", t_string);
            fprintf(s_output, "    where(\"%s\" -> String)\n", t_string);
        }
    }
    else
    {
        fprintf(s_output, "  'rule' CustomKeywords(-> String):\n");
        fprintf(s_output, "    \"THISCANNEVERHAPPENORATLEASTWEHOPESO\"\n");
        fprintf(s_output, "    where(\"THISCANNEVERHAPPENORATLEASTWEHOPESO\" -> String)\n");
    }
}

void DefineCustomInvokeList(intptr_t index, intptr_t ptr)
{
    if (index >= s_invoke_list_size)
    {
        while(index >= s_invoke_list_size)
        {
            if (s_invoke_list_size == 0)
                s_invoke_list_size = 1;
            s_invoke_list_size *= 2;
        }
    
        s_invoke_lists = (intptr_t *)Reallocate(s_invoke_lists, s_invoke_list_size * sizeof(intptr_t));
    }
    s_invoke_lists[index] = ptr;
}

void LookupCustomInvokeList(intptr_t index, intptr_t *r_ptr)
{
    *r_ptr = s_invoke_lists[index];
}

extern void DumpSyntaxRules(void);
extern void DumpSyntaxMethods(void);
void GenerateSyntaxRules(void)
{
    FILE *t_template;
	FILE *t_output;
	int t_index;
	SyntaxRuleGroupRef t_group;

    t_output = OpenOutputGrammarFile(NULL);
    
    if (t_output != NULL)
        s_output = t_output;
    else
	    return;
    
    t_template = OpenTemplateFile();
    if (t_template != NULL)
    {
        char t_line[128];
        
        while(!feof(t_template))
        {
            size_t t_length = 0;
            fgets(t_line, 128, t_template);
            t_length = strlen(t_line);
            
            if (t_length > 8 && strncmp(t_line, "'module'", 8) == 0)
                fprintf(t_output, "'module' grammar_full\n");
            else
                fprintf(t_output, "%.*s", (int)t_length, t_line);
            
            if (t_length > 5 && strncmp(t_line, "--*--", 5) == 0)
                break;
        }
        
        fclose(t_template);
    }

    t_index = 0;
    
    for(t_group = s_groups; t_group != NULL; t_group = t_group -> next)
    {
        SyntaxRuleRef t_rule, t_other_rule;

        t_rule = t_group -> rules;
        for(t_other_rule = t_group -> rules -> next; t_other_rule != NULL; t_other_rule = t_other_rule -> next)
            MergeSyntaxRule(t_rule, t_other_rule);
        
        if (t_rule -> kind == kSyntaxRuleKindPhrase)
            t_rule = t_rule;
        
        t_group -> index = t_index;
        GenerateSyntaxRule(&t_index, t_rule -> expr, t_rule -> kind, t_rule);
        if (t_rule -> deprecation_message != NULL)
            fprintf(s_output, "    Warning_DeprecatedSyntax(Position, \"%s\")\n", t_rule -> deprecation_message);
        
        if (t_rule -> kind == kSyntaxRuleKindPhrase)
        {
            const char *t_name_string;
            GetStringOfNameLiteral(t_rule -> name, &t_name_string);
            fprintf(s_output, "'nonterm' Custom_%s(-> EXPRESSION)\n", t_name_string);
            fprintf(s_output, "  'rule' Custom_%s(-> Expr):\n", t_name_string);
            fprintf(s_output, "    CustomRule%d(-> Expr)\n", t_rule -> expr -> concrete_rule);
        }
    }
    
    GenerateUmbrellaSyntaxRule("CustomStatements", kSyntaxRuleKindStatement, kSyntaxRuleKindStatement);
    GenerateUmbrellaSyntaxRule("CustomIterators", kSyntaxRuleKindIterator, kSyntaxRuleKindIterator);
    GenerateUmbrellaSyntaxRule("CustomTerms", kSyntaxRuleKindExpression, kSyntaxRuleKindExpression);
    GenerateUmbrellaSyntaxRule("CustomBinaryOperators", kSyntaxRuleKindLeftBinaryOperator, kSyntaxRuleKindNeutralBinaryOperator);
    GenerateUmbrellaSyntaxRule("CustomPrefixOperators", kSyntaxRuleKindPrefixOperator, kSyntaxRuleKindPrefixOperator);
    GenerateUmbrellaSyntaxRule("CustomPostfixOperators", kSyntaxRuleKindPostfixOperator, kSyntaxRuleKindPostfixOperator);
    
    GenerateInvokeLists();
    GenerateTokenList();
}

void DumpSyntaxRules(void)
{
    int t_gindex;
	SyntaxRuleGroupRef t_group;

    t_gindex = 0;
    for (t_group = s_groups; t_group != NULL; t_group = t_group -> next)
    {
        SyntaxRuleRef t_rule;
		
		for (t_rule = t_group -> rules; t_rule != NULL; t_rule = t_rule -> next)
        {
            const char *t_name;
            GetStringOfNameLiteral(t_rule -> name, &t_name);
            printf("[%d] ", t_gindex);
            PrintSyntaxNode(t_rule -> expr);
            printf("\n");
        }
        t_gindex += 1;
    }
}

void PrintSyntaxMethod(SyntaxMethodRef self)
{
    const char *t_name;
    struct SyntaxArgument *t_arg;
    GetStringOfNameLiteral(self -> name, &t_name);
    printf("%s(", t_name);
    for(t_arg = self -> arguments; t_arg != NULL; t_arg = t_arg -> next)
    {
        if (t_arg != self -> arguments)
            printf(", ");
        printf("%s %ld", t_arg -> mode == 0 ? "in" : t_arg -> mode == 1 ? "out" : "inout", t_arg -> index);
        
        {
            printf(" {");
        
            printf("}");
        }
    }
    printf(")");
}

void DumpSyntaxMethods(void)
{
    int t_gindex;
	SyntaxRuleGroupRef t_group;
    
    t_gindex = 0;
    for (t_group = s_groups; t_group != NULL; t_group = t_group -> next)
    {
        SyntaxRuleRef t_rule;
		
		for (t_rule = t_group -> rules; t_rule != NULL; t_rule = t_rule -> next)
        {
            const char *t_name;
            SyntaxMethodRef t_method;
            
            GetStringOfNameLiteral(t_rule -> name, &t_name);
            for(t_method = t_rule -> methods; t_method != NULL; t_method = t_method -> next)
            {
                printf("[%d] [%s] ", t_gindex, t_name);
                PrintSyntaxMethod(t_method);
                printf("\n");
            }
        }
        t_gindex += 1;
    }
}

////////////////////////////////////////////////////////////////////////////////

