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

#include "report.h"

typedef enum NodeKind NodeKind;
enum NodeKind
{
    kNodeKindOperand,
    kNodeKindPrefixOperator,
    kNodeKindPostfixOperator,
    kNodeKindLeftBinaryOperator,
    kNodeKindRightBinaryOperator,
    kNodeKindNeutralBinaryOperator,
};

typedef struct Node Node;
struct Node
{
    NodeKind kind;
    Node *next;
    union
    {
        struct
        {
            intptr_t precedence;
			intptr_t position;
			intptr_t method;
			intptr_t arity;
            Node *arguments;
        } operator;
        struct
        {
			intptr_t value;
        } operand;
    };
};

/*typedef struct Nest Nest;
struct Nest
{
    Nest *next;
    Node *stack;
    Node *clause;
};

static Nest *s_nests = NULL;*/

// The stack is used to rebuild the AST after reordering.
static Node *s_stack = NULL;

// The clause is built up from parsing.
static Node *s_clause = NULL;

//////////

static Node *MakeNode(NodeKind p_kind)
{
    Node *t_node;
    t_node = (Node *)malloc(sizeof(Node));
    if (t_node == NULL)
        Fatal_OutOfMemory();
    
    t_node -> kind = p_kind;
    t_node -> next = NULL;
    
    return t_node;
}

static Node *LastNodeInList(Node *p_list)
{
    Node *t_node;
	
	for(t_node = p_list; t_node != NULL; t_node = t_node -> next)
        if (t_node -> next == NULL)
            return t_node;
    return NULL;
}

static void AppendToNodeList(Node **x_list, Node *p_node)
{
    Node *t_last;
	
	if (*x_list == NULL)
    {
        *x_list = p_node;
        return;
    }
    
    t_last = LastNodeInList(*x_list);
    t_last -> next = p_node;
}

static intptr_t CountNodeList(Node *p_list)
{
    intptr_t t_count = 0;

    while(p_list != NULL)
    {
        t_count += 1;
        p_list = p_list -> next;
    }
    return t_count;
}

Node *DivideNodeListAt(Node **x_list, intptr_t p_index)
{
    Node *t_return;

    if (p_index == 0)
    {
        t_return = *x_list;
        *x_list = NULL;
    }
    else
    {
        Node *t_previous;
        t_previous = *x_list;
        while(p_index > 1)
        {
            t_previous = t_previous -> next;
            p_index -= 1;
        }
        t_return = t_previous -> next;
        t_previous -> next = NULL;
    }
    return t_return;
}

//////////

static int CompareNodePrecedence(Node *p_left, Node *p_right)
{
	NodeKind t_left_operator, t_right_operator;
	
	// A nil pointer indicates $ - the beginning or end of input.
	if (p_left == NULL)
		return -1;
	if (p_right == NULL)
		return 1;
    
	// Now fetch the operators for both syntax trees.
    t_left_operator = p_left -> kind;
    t_right_operator = p_right -> kind;
    
	// A nil pointer indicates a non-operator.
	if (t_left_operator == kNodeKindOperand)
		return 1;
	if (t_right_operator == kNodeKindOperand)
		return -1;
    
    if (t_left_operator == kNodeKindPrefixOperator &&
        t_right_operator == kNodeKindPrefixOperator)
        return -1;
    
    if (t_left_operator == kNodeKindPostfixOperator &&
        t_right_operator == kNodeKindPostfixOperator)
        return 1;
    
	// Handle the prefix rules:
	//   T <- L for all T
	//   L -> T if p(L) > p(T)
	//   L <- T if p(L) <= p(T)
	if (t_left_operator == kNodeKindPrefixOperator)
		return p_left -> operator . precedence < p_right -> operator . precedence ? 1 : -1;
	if (t_right_operator == kNodeKindPrefixOperator)
		return -1;
    
	// Handle the postfix rules:
	//  R <- T for all T
	//  T -> R if p(T) > p(R)
	//  T <- R if p(T) <= p(R)
	if (t_right_operator == kNodeKindPrefixOperator)
		return p_left -> operator . precedence > p_right -> operator . precedence ? 1 : -1;
	if (t_left_operator == kNodeKindPrefixOperator)
		return -1;
    
	// Handle the binary rules:
	//   p(L) > p(R) => L -> R and R <- L
	//   for left-assoc: p(L) == p(R) => L -> R and R -> L
	//   for right-assoc: p(L) == p(R) => L <- R and R <- L
	if (p_left -> operator . precedence < p_right -> operator . precedence)
		return 1;
	if (p_left -> operator . precedence > p_right -> operator . precedence)
		return -1;
    
	if (t_left_operator == kNodeKindLeftBinaryOperator && t_right_operator == kNodeKindLeftBinaryOperator)
		return 1;
    
	if (t_left_operator == kNodeKindRightBinaryOperator && t_right_operator == kNodeKindRightBinaryOperator)
		return -1;
    
	return 0;
}

static intptr_t GetNodeArity(Node *p_node)
{
    if (p_node -> kind == kNodeKindOperand)
        return 0;
    
    if (p_node -> kind >= kNodeKindLeftBinaryOperator)
        return 2;
    
    return 1;
}

static void PrintNodeList(Node *p_node)
{
    while(p_node != NULL)
    {
        if (p_node -> kind == kNodeKindOperand)
            fprintf(stderr, "<%ld>", p_node -> operand . value);
        else
        {
            fprintf(stderr, "{%d(%ld):", p_node -> kind, p_node -> operator . arity);
            PrintNodeList(p_node -> operator . arguments);
            fprintf(stderr, "}");
        }
        p_node = p_node -> next;
    }
}

/*void BeginOperatorExpression(void)
{
    Nest *t_nest;
    t_nest = (Nest *)malloc(sizeof(Nest));
    t_nest -> stack = s_stack;
    t_nest -> clause = s_clause;
    t_nest -> next = s_nests;
    s_nests = t_nest;
    
    s_stack = NULL;
    s_clause = NULL;
}

void EndOperatorExpression(void)
{
    if (s_nests == NULL)
        return;
    
    s_stack = s_nests -> stack;
    s_clause = s_nests -> clause;
    
    Nest *t_remove;
    t_remove = s_nests;
    s_nests = s_nests -> next;
    free(t_remove);
}*/

void ReorderOperatorExpression(intptr_t p_sentinal)
{
    Node *t_clause;
	Node *t_input_stack, *t_output_stack;
	
	assert(s_clause != NULL);
    
    t_clause = DivideNodeListAt(&s_clause, p_sentinal);
    
    t_input_stack = NULL;
    t_output_stack = NULL;
    
	for(;;)
	{
		Node *t_top, *t_input;
		int t_relation;
		
		// Fetch the top element of the stack and input.
		t_top = t_input_stack;
		t_input = t_clause;
        
		// If we are at end of input and top of stack, we are done.
		if (t_top == NULL && t_input == NULL)
			break;
        
		// Now compare the top-most symbol on the stack (if any) and the next piece of syntax.
		t_relation = CompareNodePrecedence(t_top, t_input);
        
		// If top <- input or top =- input then push input and advance.
		if (t_relation <= 0)
		{
			// Advance the input.
			t_clause = t_clause -> next;
            
			// Put the input at the head of the input stack.
			t_input -> next = t_input_stack;
			t_input_stack = t_input;
		}
		else
		{
			// Otherwise we must pop until we yield.
			for(;;)
			{
                Node *t_popped;
				t_popped = t_top;
                
				// Pop off the input stack,
				t_input_stack = t_input_stack -> next;
                
				// Fetch the top operator arity.
				/*ng t_popped_arity;
				t_popped_arity = GetNodeArity(t_popped);
				if (t_popped_arity != 0)
				{
					while(t_popped_arity > 0)
					{
						Node *t_argument;
						t_argument = t_output_stack;
                        
						t_output_stack = t_output_stack -> next;
                        
						t_argument -> next = t_popped -> operator . arguments;
						t_popped -> operator . arguments = t_argument;
                        t_popped -> operator . arity += 1;
                        
						t_popped_arity -= 1;
					}
				}*/
                if (t_popped -> kind == kNodeKindPostfixOperator)
                {
                    Node *t_argument;
                    t_argument = t_output_stack;
                    t_output_stack = t_output_stack -> next;
                    t_argument -> next = NULL;
                    
                    t_argument -> next = t_popped -> operator . arguments;
                    t_popped -> operator . arguments = t_argument;
                    t_popped -> operator . arity += 1;
                }
                else if (t_popped -> kind == kNodeKindPrefixOperator)
                {
                    Node *t_argument;
                    t_argument = t_output_stack;
                    t_output_stack = t_output_stack -> next;
                    t_argument -> next = NULL;
                    
                    AppendToNodeList(&t_popped -> operator . arguments, t_argument);
                    t_popped -> operator . arity += 1;
                }
                else if (t_popped -> kind != kNodeKindOperand)
                {
                    Node *t_argument;
                    t_argument = t_output_stack;
                    t_output_stack = t_output_stack -> next;
                    t_argument -> next = NULL;
                    
                    AppendToNodeList(&t_popped -> operator . arguments, t_argument);
                    t_popped -> operator . arity += 1;
                    
                    t_argument = t_output_stack;
                    t_output_stack = t_output_stack -> next;
                    t_argument -> next = NULL;
                    
                    t_argument -> next = t_popped -> operator . arguments;
                    t_popped -> operator . arguments = t_argument;
                    t_popped -> operator . arity += 1;
                }
                
				// Push the popped element on the output stafck.
				t_popped -> next = t_output_stack;
				t_output_stack = t_popped;
                
				// Fetch the new top of the input stack.
				t_top = t_input_stack;
                
				// We only continue while top yields to popped.
				t_relation = CompareNodePrecedence(t_top, t_popped);
				if (t_relation < 0)
					break;
			}
		}
	}
    
    s_stack = t_output_stack;
}

int PopOperatorExpression(intptr_t *r_position, intptr_t *r_method, intptr_t *r_arity)
{
    Node *t_node;
	Node *t_arguments;
	
	assert(s_stack != NULL);
    
    if (s_stack -> kind == kNodeKindOperand)
        return 0;
    
    t_node = s_stack;
    s_stack = s_stack -> next;
    
    *r_position = t_node -> operator . position;
    *r_method = t_node -> operator . method;
    *r_arity = t_node -> operator . arity;
    
    t_arguments = t_node -> operator . arguments;
    
    if (t_arguments != NULL)
    {
        Node *t_last_arg;
        t_last_arg = LastNodeInList(t_arguments);
        t_last_arg -> next = s_stack;
        s_stack = t_arguments;
    }
    
    free(t_node);
    
    return 1;
}

void PopOperatorExpressionArgument(intptr_t *r_argument)
{
    Node *t_node;
	
	assert(s_stack -> kind == kNodeKindOperand);
    *r_argument = s_stack -> operand . value;
    
    t_node = s_stack;
    s_stack = s_stack -> next;
    
    free(t_node);
}

//////////

static void PushOperatorExpressionOperator(NodeKind p_kind, intptr_t p_position, intptr_t p_precedence, intptr_t p_method, intptr_t *r_sentinal)
{
    Node *t_node;
    t_node = MakeNode(p_kind);
    t_node -> next = NULL;
    t_node -> operator . position = p_position;
    t_node -> operator . precedence = p_precedence;
    t_node -> operator . method = p_method;
    t_node -> operator . arity = 0;
    t_node -> operator . arguments = NULL;
    AppendToNodeList(&s_clause, t_node);
    *r_sentinal = CountNodeList(s_clause) - 1;
}

void PushOperatorExpressionPrefix(intptr_t p_position, intptr_t p_precedence, intptr_t p_method, intptr_t *r_sentinal)
{
    PushOperatorExpressionOperator(kNodeKindPrefixOperator, p_position, p_precedence, p_method, r_sentinal);
}

void PushOperatorExpressionPostfix(intptr_t p_position, intptr_t p_precedence, intptr_t p_method, intptr_t *r_sentinal)
{
    PushOperatorExpressionOperator(kNodeKindPostfixOperator, p_position, p_precedence, p_method, r_sentinal);
}

void PushOperatorExpressionLeftBinary(intptr_t p_position, intptr_t p_precedence, intptr_t p_method, intptr_t *r_sentinal)
{
    PushOperatorExpressionOperator(kNodeKindLeftBinaryOperator, p_position, p_precedence, p_method, r_sentinal);
}

void PushOperatorExpressionRightBinary(intptr_t p_position, intptr_t p_precedence, intptr_t p_method, intptr_t *r_sentinal)
{
    PushOperatorExpressionOperator(kNodeKindRightBinaryOperator, p_position, p_precedence, p_method, r_sentinal);
}

void PushOperatorExpressionNeutralBinary(intptr_t p_position, intptr_t p_precedence, intptr_t p_method, intptr_t *r_sentinal)
{
    PushOperatorExpressionOperator(kNodeKindNeutralBinaryOperator, p_position, p_precedence, p_method, r_sentinal);
}

void PushOperatorExpressionArgument(intptr_t p_argument, intptr_t *r_sentinal)
{
    Node *t_node;
	Node *t_last;
	
	assert(s_clause != NULL);
    assert(LastNodeInList(s_clause) -> kind != kNodeKindOperand);
    
    t_node = MakeNode(kNodeKindOperand);
    t_node -> operand . value = p_argument;
    
    t_last = LastNodeInList(s_clause);
    t_last -> operator . arity += 1;
    AppendToNodeList(&t_last -> operator . arguments, t_node);
    *r_sentinal = CountNodeList(s_clause) - 1;
}

void PushOperatorExpressionOperand(intptr_t p_operand, intptr_t *r_sentinal)
{
    Node *t_node;
    t_node = MakeNode(kNodeKindOperand);
    t_node -> operand . value = p_operand;
    AppendToNodeList(&s_clause, t_node);
    *r_sentinal = CountNodeList(s_clause) - 1;
}
