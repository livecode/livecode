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

//
// MCLiteral class declarations
//
#ifndef	LITERAL_H
#define	LITERAL_H

#include "express.h"

class MCLiteral : public MCExpression
{
	MCValueRef value;
public:
	MCLiteral(MCValueRef v)
	{
		/* UNCHECKED */ value = MCValueRetain(v);
	}
	~MCLiteral(void)
	{
		MCValueRelease(value);
	}

    virtual MCExpressionAttrs getattrs(void) const;
    
    virtual Parse_stat parse(MCScriptPoint &, Boolean the);
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
	virtual void compile(MCSyntaxFactoryRef ctxt);
};

/********/

/* The MCSequenceLiteral class is the AST expression node for a numerically
 * keyed array literal. e.g. [ 1, 2, "foo", "bar", myFunc(3) ]. */
class MCSequenceLiteral: public MCExpression
{
    /* The DynamicValue struct represents a link in a singly-linked list of
     * values that need to be evaluated at runtime, rather than compile time. */
    struct DynamicValue
    {
        DynamicValue *next;
        uindex_t index;
        MCAutoPointer<MCExpression> expr;
        
        DynamicValue(DynamicValue* p_next, uindex_t p_index, MCExpression* p_expr)
            : next(p_next),
              index(p_index),
              expr(p_expr)
        {
        }
    };
    
    /* The m_base_value member holds all the compile-time evaluatable (constant)
     * elements of the sequence literal. */
    MCAutoArrayRef m_base_value;
    
    /* The m_dynamic_values member is the head ptr for the list of dynamic
     * values. */
    DynamicValue *m_dynamic_values = nullptr;
    
public:
    /* We need an explicit constructor, as the m_dynamic_values list needs
     * to be freed. */
    ~MCSequenceLiteral(void);
    
    /* Sequence literals can be constant, so we override the 'getattrs' method
     * so that we can indicate this. A sequence literal is constant if there are
     * no dynamic values. */
    virtual MCExpressionAttrs getattrs(void) const;
    
    /* The syntax of a sequence literal is '[' { <expr> , ',' } ']' */
    virtual Parse_stat parse(MCScriptPoint& sp, Boolean the);
    
    /* The evaluation can return a constant literal directly, or for ones with
     * dynamic values will need to evaluate each in turn and build a new one. */
    virtual void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
};

/********/

class MCArrayLiteral: public MCExpression
{
    struct DynamicValue
    {
        DynamicValue *next;
        bool key_is_dynamic : 1;
        bool value_is_dynamic : 1;
        union
        {
            MCExpression *dynamic_key;
            MCNameRef constant_key;
            void *key_ptr;
        };
        union
        {
            MCExpression *dynamic_value;
            MCValueRef constant_value;
            void *value_ptr;
        };
        
        DynamicValue(DynamicValue *p_next, bool p_key_is_dynamic, void *p_key_ptr, bool p_value_is_dynamic, void *p_value_ptr)
            : next(p_next),
              key_is_dynamic(p_key_is_dynamic),
              value_is_dynamic(p_value_is_dynamic),
              key_ptr(p_key_ptr),
              value_ptr(p_value_ptr)
        {
        }
        
        ~DynamicValue(void)
        {
            if (key_ptr != nullptr)
            {
                if (key_is_dynamic)
                    delete dynamic_key;
                else
                    MCValueRelease(constant_key);
            }
            
            if (value_ptr != nullptr)
            {
                if (value_is_dynamic)
                    delete dynamic_value;
                else
                    MCValueRelease(constant_key);
            }
        }
    };
    
    MCAutoArrayRef m_base_value;
    DynamicValue *m_dynamic_values = nullptr;
    
public:
    ~MCArrayLiteral(void);

    virtual MCExpressionAttrs getattrs(void) const;
    
    virtual Parse_stat parse(MCScriptPoint& sp, Boolean the);
    virtual void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
};

#endif
