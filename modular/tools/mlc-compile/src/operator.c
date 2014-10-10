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
            long precedence;
            long position;
            long method;
            long arity;
            Node *arguments;
        } operator;
        struct
        {
            long value;
        } operand;
    };
};

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
    for(Node *t_node = p_list; t_node != NULL; t_node = t_node -> next)
        if (t_node -> next == NULL)
            return t_node;
    return NULL;
}

static void AppendToNodeList(Node **x_list, Node *p_node)
{
    if (*x_list == NULL)
    {
        *x_list = p_node;
        return;
    }
    
    Node *t_last;
    t_last = LastNodeInList(*x_list);
    t_last -> next = p_node;
}

//////////

static int CompareNodePrecedence(Node *p_left, Node *p_right)
{
	// A nil pointer indicates $ - the beginning or end of input.
	if (p_left == NULL)
		return -1;
	if (p_right == NULL)
		return 1;
    
	// Now fetch the operators for both syntax trees.
	NodeKind t_left_operator, t_right_operator;
    t_left_operator = p_left -> kind;
    t_right_operator = p_right -> kind;
    
	// A nil pointer indicates a non-operator.
	if (t_left_operator == kNodeKindOperand)
		return 1;
	if (t_right_operator == kNodeKindOperand)
		return -1;
    
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

static long GetNodeArity(Node *p_node)
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

void ReorderOperatorExpression(void)
{
    assert(s_clause != NULL);
    
    Node *t_input_stack, *t_output_stack;
    t_input_stack = NULL;
    t_output_stack = NULL;
    
	for(;;)
	{
		// Fetch the top element of the stack and input.
		Node *t_top, *t_input;
		t_top = t_input_stack;
		t_input = s_clause;
        
		// If we are at end of input and top of stack, we are done.
		if (t_top == NULL && t_input == NULL)
			break;
        
		// Now compare the top-most symbol on the stack (if any) and the next piece of syntax.
		int t_relation;
		t_relation = CompareNodePrecedence(t_top, t_input);
        
		// If top <- input or top =- input then push input and advance.
		if (t_relation <= 0)
		{
			// Advance the input.
			s_clause = s_clause -> next;
            
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
				long t_popped_arity;
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

int PopOperatorExpression(long *r_position, long *r_method, long *r_arity)
{
    assert(s_stack != NULL);
    
    if (s_stack -> kind == kNodeKindOperand)
        return 0;
    
    Node *t_node;
    t_node = s_stack;
    s_stack = s_stack -> next;
    
    *r_position = t_node -> operator . position;
    *r_method = t_node -> operator . method;
    *r_arity = t_node -> operator . arity;
    
    Node *t_arguments;
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

void PopOperatorExpressionArgument(long *r_argument)
{
    assert(s_stack -> kind == kNodeKindOperand);
    *r_argument = s_stack -> operand . value;
    
    Node *t_node;
    t_node = s_stack;
    s_stack = s_stack -> next;
    
    free(t_node);
}

//////////

static void PushOperatorExpressionOperator(NodeKind p_kind, long p_position, long p_precedence, long p_method)
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
}

void PushOperatorExpressionPrefix(long p_position, long p_precedence, long p_method)
{
    PushOperatorExpressionOperator(kNodeKindPrefixOperator, p_position, p_precedence, p_method);
}

void PushOperatorExpressionPostfix(long p_position, long p_precedence, long p_method)
{
    PushOperatorExpressionOperator(kNodeKindPostfixOperator, p_position, p_precedence, p_method);
}

void PushOperatorExpressionLeftBinary(long p_position, long p_precedence, long p_method)
{
    PushOperatorExpressionOperator(kNodeKindLeftBinaryOperator, p_position, p_precedence, p_method);
}

void PushOperatorExpressionRightBinary(long p_position, long p_precedence, long p_method)
{
    PushOperatorExpressionOperator(kNodeKindRightBinaryOperator, p_position, p_precedence, p_method);
}

void PushOperatorExpressionNeutralBinary(long p_position, long p_precedence, long p_method)
{
    PushOperatorExpressionOperator(kNodeKindNeutralBinaryOperator, p_position, p_precedence, p_method);
}

void PushOperatorExpressionArgument(long p_argument)
{
    assert(s_clause != NULL);
    assert(s_clause -> kind != kNodeKindOperand);
    
    Node *t_node;
    t_node = MakeNode(kNodeKindOperand);
    t_node -> operand . value = p_argument;
    
    Node *t_last;
    t_last = LastNodeInList(s_clause);
    t_last -> operator . arity += 1;
    AppendToNodeList(&t_last -> operator . arguments, t_node);
}

void PushOperatorExpressionOperand(long p_operand)
{
    Node *t_node;
    t_node = MakeNode(kNodeKindOperand);
    t_node -> operand . value = p_operand;
    AppendToNodeList(&s_clause, t_node);
}