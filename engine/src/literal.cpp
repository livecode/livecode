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

#include "prefix.h"

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "globals.h"

#include "literal.h"
#include "scriptpt.h"
#include "mcerror.h"

MCExpressionAttrs MCLiteral::getattrs(void) const
{
    return MCExpressionAttrs().SetIsConstant();
}

Parse_stat MCLiteral::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	return PS_NORMAL;
}

void MCLiteral::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
	r_value . type = kMCExecValueTypeValueRef;
	r_value . valueref_value = MCValueRetain(value);
}

/**/

MCSequenceLiteral::~MCSequenceLiteral(void)
{
    /* Iteratively advance to the next link, then delete the previous one. */
    while(m_dynamic_values != nullptr)
    {
        DynamicValue *t_current = m_dynamic_values;
        m_dynamic_values = m_dynamic_values->next;
        delete t_current;
    }
}

MCExpressionAttrs MCSequenceLiteral::getattrs(void) const
{
    /* If there are dynamic values, then the expression cannot be constant. */
    if (m_dynamic_values != nullptr)
    {
        return {};
    }
    
    /* If there are no dynamic values, then the expression is constant and its
     * value is m_base_value. */
    return MCExpressionAttrs().SetIsConstant();
}

Parse_stat MCSequenceLiteral::parse(MCScriptPoint& sp, Boolean the)
{
    initpoint(sp);
    
    /* The base value is created as a mutable array and will hold all constant
     * values. */
    MCAutoArrayRef t_base_value;
    if (!MCArrayCreateMutable(&t_base_value))
    {
        MCperror->add(PE_OUTOFMEMORY, sp);
        return PS_ERROR;
    }
    
    /* The indicies start at 1 */
    uindex_t t_index = 1;
    
    /* We keep track of the pointer to the slot holding the current head of
     * chain - which begins with the head link in the MCSequenceLiteral ast. */
    DynamicValue** t_values_head = &m_dynamic_values;
    
    /* Loop to match '[' { <expr> , ',' } ']' */
    for(;;)
    {
        /* First match an expression - we want more than a single factor (first
         * False), but we need to match ',' as part of the value list (second
         * False). */
        MCAutoPointer<MCExpression> t_value_exp;
        if (sp.parseexp(False, False, &(&t_value_exp)) != PS_NORMAL)
        {
            MCperror->add(PE_SEQLITERAL_EXPEXPECTED, sp);
            return PS_ERROR;
        }
        
        /* If the node is constant, *and* it successfully evaluates then we
         * can make it part of the base value. Otherwise we add it to the list
         * of dynamic values to be evaluated at runtime. */
        MCAutoValueRef t_constant_value;
        if (t_value_exp->getattrs().IsConstant() &&
            t_value_exp->constant_eval(&t_constant_value))
        {
            /* We must unique the constant value we get back as some constant
             * exprs (e.g. MCConstant) use unboxed values if they can. */
            if (!t_constant_value.MakeUnique() ||
                !MCArrayStoreValueAtIndex(*t_base_value,
                                          t_index,
                                          *t_constant_value))
            {
                MCperror->add(PE_OUTOFMEMORY, sp);
                return PS_ERROR;
            }
        }
        else
        {
            /* Add a new DynamicValue link on to the head of the chain. */
            *t_values_head = new(nothrow) DynamicValue(*t_values_head,
                                                       t_index,
                                                       t_value_exp.Release());
            if (*t_values_head == nullptr)
            {
                MCperror->add(PE_OUTOFMEMORY, sp);
                return PS_ERROR;
            }

            /* Update the head link to be the 'next' field of the newly appended
             * link */
            t_values_head = &(*t_values_head)->next;
        }
        
        /* Check for either ',' or ']' */
        Symbol_type t_sep_type;
        if (sp.next(t_sep_type) != PS_NORMAL ||
            (t_sep_type != ST_SEP && t_sep_type != ST_RB))
        {
            MCperror->add(PE_SEQLITERAL_SEPEXPECTED, sp);
            return PS_ERROR;
        }
        
        /* As soon as we encounter ']', we are done. */
        if (t_sep_type == ST_RB)
        {
            break;
        }

        /* Move to the next index */
        t_index += 1;
    }
    
    /* If the base value array has at least one element we make it immutable
     * and then unique it. This means that sequence literals which have the same
     * base value, will share that value in memory. If the base value array has
     * no elements, then we use the empty array. */
    if (MCArrayGetCount(*t_base_value) != 0)
    {
        if (!t_base_value.MakeImmutable() ||
            !t_base_value.MakeUnique())
        {
            MCperror->add(PE_OUTOFMEMORY, sp);
            return PS_ERROR;
        }
        
        /* We can just use assignment here as we are assigning auto pointer to
         * auto pointer. A suitably clever C++ compiler will use the moving
         * constructor. */
        m_base_value.Reset(*t_base_value);
    }
    else
    {
        m_base_value = kMCEmptyArray;
    }

    return PS_NORMAL;
}

void MCSequenceLiteral::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    /* If there are no dynamic values then we are done - the value of the
     * literal is the base value. */
    if (m_dynamic_values == nullptr)
    {
        r_value . type = kMCExecValueTypeValueRef;
        r_value . valueref_value = MCValueRetain(*m_base_value);
        return;
    }
    
    /* There are dynamic values, so we need to add them to a copy of the base
     * value. */
    MCAutoArrayRef t_literal;
    if (!MCArrayMutableCopy(*m_base_value,
                            &t_literal))
    {
        ctxt.LegacyThrow(EE_NO_MEMORY);
        return;
    }
    
    /* Loop through the singly-linked dynamic value list. */
    for(DynamicValue *t_dynamic_value = m_dynamic_values;
        t_dynamic_value != nullptr;
        t_dynamic_value = t_dynamic_value->next)
    {
        /* First try to evaluate the expression. */
        MCAutoValueRef t_value;
        if (!ctxt.EvalExprAsValueRef(*t_dynamic_value->expr,
                                     EE_SEQLITERAL_BADEXPR,
                                     &t_value))
        {
            return;
        }
        
        /* If evaluation succeeds, try to store the value */
        if (!MCArrayStoreValueAtIndex(*t_literal,
                                      t_dynamic_value->index,
                                      *t_value))
        {
            ctxt.Throw();
            return;
        }
    }
    
    /* Make sure the value is immutable */
    if (!t_literal.MakeImmutable())
    {
        ctxt.Throw();
        return;
    }
    
    /* We are done - return the constructed literal as an ExecValue. */
	r_value . type = kMCExecValueTypeValueRef;
	r_value . valueref_value = t_literal.Take();
}

/**/

MCArrayLiteral::~MCArrayLiteral(void)
{
    /* Iteratively advance to the next link, then delete the previous one. */
    while(m_dynamic_values != nullptr)
    {
        DynamicValue *t_current = m_dynamic_values;
        m_dynamic_values = m_dynamic_values->next;
        delete t_current;
    }
}

MCExpressionAttrs MCArrayLiteral::getattrs(void) const
{
    /* If there are dynamic values, then the expression cannot be constant. */
    if (m_dynamic_values != nullptr)
    {
        return {};
    }
    
    /* If there are no dynamic values, then the expression is constant and its
     * value is m_base_value. */
    return MCExpressionAttrs().SetIsConstant();
}

#ifndef __ALLOW_DYNAMIC_ARRAY_LITERAL_KEYS__

Parse_stat MCArrayLiteral::parse(MCScriptPoint& sp, Boolean the)
{
    initpoint(sp);
    
    /* The base value is created as a mutable array and will hold all constant
     * values - up to the first non-dynamic value encountered. */
    MCAutoArrayRef t_base_value;
    if (!MCArrayCreateMutable(&t_base_value))
    {
        MCperror->add(PE_OUTOFMEMORY, sp);
        return PS_ERROR;
    }
    
    /* We keep track of the pointer to the slot holding the current head of
     * chain - which begins with the head link in the MCArrayLiteral ast. */
    DynamicValue** t_values_head = &m_dynamic_values;
    
    /* Loop to match '{' { <arrlit> ':' <expr> , ',' } '}' */
    for(;;)
    {
        /* A key must be either a (string) literal or a number token. */
        Symbol_type t_key_type;
        if (sp.next(t_key_type) != PS_NORMAL ||
            (t_key_type != ST_LIT && t_key_type != ST_NUM))
        {
            MCperror->add(PE_ARRLITERAL_KEYEXPECTED, sp);
            return PS_ERROR;
        }
        
        /* If the key is a number token, then it must be a strict integer. */
        if (t_key_type == ST_NUM &&
            !MCStringIsInteger(MCNameGetString(sp.gettoken_nameref())))
        {
            MCperror->add(PE_ARRLITERAL_BADKEY, sp);
            return PS_ERROR;
        }
        
        /* The key is just the nameref of the token. */
        MCNewAutoNameRef t_constant_key = sp.gettoken_nameref();
        
        /* Match the key separator ':' */
        Symbol_type t_col_type;
        if (sp.next(t_col_type) != PS_NORMAL ||
            t_col_type != ST_COL)
        {
            MCperror->add(PE_ARRLITERAL_COLEXPECTED, sp);
            return PS_ERROR;
        }
        
        /* Next match the value expression - we want more than a single factor
         * (first False), but we need to match ',' as part of the key-value list
         * (second False). */
        MCAutoPointer<MCExpression> t_value_exp;
        if (sp.parseexp(False, False, &(&t_value_exp)) != PS_NORMAL)
        {
            MCperror->add(PE_ARRLITERAL_VALEXPEXPECTED, sp);
            return PS_ERROR;
        }
        
        /* Keys must be caselessly unique. */
        MCValueRef t_unused_value;
        if (MCArrayFetchValue(*t_base_value, false, *t_constant_key, t_unused_value))
        {
            MCperror->add(PE_ARRLITERAL_NOTUNIQUE, sp);
            return PS_ERROR;
        }
        
        /* Evaluate the value as a constant, if possible. */
        MCAutoValueRef t_constant_value;
        if (t_value_exp->getattrs().IsConstant() &&
            t_value_exp->constant_eval(&t_constant_value))
        {
            /* We must unique the constant value we get back as some constant
             * exprs (e.g. MCConstant) use unboxed values if they can. Notice
             * that we store as if keys are case-insensitive - this is to ensure
             * consistent behavior is generated at runtime regardless of the
             * (runtime) setting of the case-sensitive property. */
            if (!t_constant_value.MakeUnique() ||
                !MCArrayStoreValue(*t_base_value,
                                   false,
                                   *t_constant_key,
                                   *t_constant_value))
            {
                MCperror->add(PE_OUTOFMEMORY, sp);
                return PS_ERROR;
            }
        }
        else
        {
            *t_values_head = new(nothrow) DynamicValue(*t_values_head,
                                                       t_constant_key.Take(),
                                                       t_value_exp.Release());
            if (*t_values_head == nullptr)
            {
                MCperror->add(PE_OUTOFMEMORY, sp);
                return PS_ERROR;
            }
            
            /* Update the head link to be the 'next' field of the newly appended
             * link */
            t_values_head = &(*t_values_head)->next;
        }
        
        /* Check for either ',' or '}' */
        Symbol_type t_sep_type;
        if (sp.next(t_sep_type) != PS_NORMAL ||
            (t_sep_type != ST_SEP && t_sep_type != ST_RC))
        {
            MCperror->add(PE_ARRLITERAL_SEPEXPECTED, sp);
            return PS_ERROR;
        }
        
        /* As soon as we encounter '}', we are done. */
        if (t_sep_type == ST_RC)
        {
            break;
        }
    }
    
    /* If the base value array has at least one element we make it immutable
     * and then unique it. This means that array literals which have the same
     * base value, will share that value in memory. If the base value array has
     * no elements, then we use the empty array. */
    if (MCArrayGetCount(*t_base_value) != 0)
    {
        if (!t_base_value.MakeImmutable() ||
            !t_base_value.MakeUnique())
        {
            MCperror->add(PE_OUTOFMEMORY, sp);
            return PS_ERROR;
        }
        
        /* We can just use assignment here as we are assigning auto pointer to
         * auto pointer. A suitably clever C++ compiler will use the moving
         * constructor. */
        m_base_value.Reset(*t_base_value);
    }
    else
    {
        m_base_value = kMCEmptyArray;
    }
    
    return PS_NORMAL;
}

void MCArrayLiteral::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    /* If there are no dynamic values then we are done - the value of the
     * literal is the base value. */
    if (m_dynamic_values == nullptr)
    {
        r_value . type = kMCExecValueTypeValueRef;
        r_value . valueref_value = MCValueRetain(*m_base_value);
        return;
    }
    
    /* There are dynamic values, so we need to add them to a copy of the base
     * value. */
    MCAutoArrayRef t_literal;
    if (!MCArrayMutableCopy(*m_base_value,
                            &t_literal))
    {
        ctxt.LegacyThrow(EE_NO_MEMORY);
        return;
    }
    
    /* Loop through the singly-linked dynamic value list. */
    for(DynamicValue *t_dynamic_value = m_dynamic_values;
        t_dynamic_value != nullptr;
        t_dynamic_value = t_dynamic_value->next)
    {
        /* Evaluate the key's value */
        MCAutoValueRef t_value;
        if (!ctxt.EvalExprAsValueRef(t_dynamic_value->value,
                                     EE_ARRLITERAL_BADVALEXPR,
                                     &t_value))
        {
            return;
        }
        
        /* If evaluation succeeds, try to store the value */
        if (!MCArrayStoreValue(*t_literal,
                               ctxt.GetCaseSensitive(),
                               t_dynamic_value->key,
                               *t_value))
        {
            ctxt.Throw();
            return;
        }
    }
    
    /* Make sure the value is immutable */
    if (!t_literal.MakeImmutable())
    {
        ctxt.Throw();
        return;
    }
    
    /* We are done - return the constructed literal as an ExecValue. */
    r_value . type = kMCExecValueTypeValueRef;
    r_value . valueref_value = t_literal.Take();
}

#else

Parse_stat MCArrayLiteral::parse(MCScriptPoint& sp, Boolean the)
{
    initpoint(sp);
    
    /* The base value is created as a mutable array and will hold all constant
     * values - up to the first non-dynamic value encountered. */
    MCAutoArrayRef t_base_value;
    if (!MCArrayCreateMutable(&t_base_value))
    {
        MCperror->add(PE_OUTOFMEMORY, sp);
        return PS_ERROR;
    }
    
    /* Constant values are only accumulated until one of the following:
     *   - a non-constant value
     *   - a non-constant key
     *   - a constant key which matches case-insensitively with a previous
     *     constant key
     * If t_still_constant is false, it means the key-value pair must be 
     * accumulated as dynamic. */
    bool t_still_constant = true;
    
    /* We keep track of the pointer to the slot holding the current head of
     * chain - which begins with the head link in the MCArrayLiteral ast. */
    DynamicValue** t_values_head = &m_dynamic_values;
    
    /* Loop to match '{' { <arrlit> ':' <expr> , ',' } '}' */
    for(;;)
    {
        /* First match the key expression - we want more than a single factor
         * (first False), and although the next token in an array literal is ':',
         * for consistency we don't allow ',' operators in keys (second False). */
        MCAutoPointer<MCExpression> t_key_exp;
        if (sp.parseexp(False, False, &(&t_key_exp)) != PS_NORMAL)
        {
            MCperror->add(PE_ARRLITERAL_KEYEXPEXPECTED, sp);
            return PS_ERROR;
        }
        
        /* Match the key separator ':' */
        Symbol_type t_col_type;
        if (sp.next(t_col_type) != PS_NORMAL ||
            t_col_type != ST_COL)
        {
            MCperror->add(PE_ARRLITERAL_COLEXPECTED, sp);
            return PS_ERROR;
        }
    
        /* Next match the value expression - similar to the first expression we
         * want more than a single factor (first False), but we need to match ','
         * as part of the key-value list (second False). */
        MCAutoPointer<MCExpression> t_value_exp;
        if (sp.parseexp(False, False, &(&t_value_exp)) != PS_NORMAL)
        {
            MCperror->add(PE_ARRLITERAL_VALEXPEXPECTED, sp);
            return PS_ERROR;
        }
  
        /* Keys must be constant. */
        MCNewAutoNameRef t_constant_key;
        if (!t_key_exp->getattrs().IsConstant() ||
            !t_key_exp->constant_eval(&t_constant_key))
        {
            MCperror->add(PE_ARRLITERAL_KEYNOTCONSTANT, sp);
            return PS_ERROR;
        }
        
        /* Evaluate the value as a constant, if possible. */
        MCAutoValueRef t_constant_value;
        if (!t_value_exp->getattrs().IsConstant() ||
            !t_value_exp->constant_eval(&t_constant_value))
        {
            t_still_constant = false;
        }
        
        /* If the array is still constant, then check to see if the key is
         * already in the base values when compared case-insensitively. */
        if (t_still_constant)
        {
            MCValueRef t_unused_value;
            if (MCArrayFetchValue(*t_base_value, false, *t_constant_key, t_unused_value))
            {
                t_still_constant = false;
            }
        }
        
        /* If still constant, then store the key-value pair in the base value.
         * Otherwise we create a dynamic value link. */
        if (t_still_constant)
        {
            /* We must unique the constant value we get back as some constant
             * exprs (e.g. MCConstant) use unboxed values if they can. Notice
             * that we store as if keys are case-insensitive - this is to ensure
             * consistent behavior is generated at runtime regardless of the
             * (runtime) setting of the case-sensitive property. */
            if (!t_constant_value.MakeUnique() ||
                !MCArrayStoreValue(*t_base_value,
                                   false,
                                   *t_constant_key,
                                   *t_constant_value))
            {
                MCperror->add(PE_OUTOFMEMORY, sp);
                return PS_ERROR;
            }
        }
        else
        {
            *t_values_head = new(nothrow) DynamicValue(*t_values_head,
                                                       !t_constant_key.IsSet(),
                                                       t_constant_key.IsSet() ? (void*)t_constant_key.Take() : (void*)t_key_exp.Release(),
                                                       !t_constant_value.IsSet(),
                                                       t_constant_value.IsSet() ? (void*)t_constant_value.Take() : (void*)t_value_exp.Release());
            if (*t_values_head == nullptr)
            {
                MCperror->add(PE_OUTOFMEMORY, sp);
                return PS_ERROR;
            }
            
            /* Update the head link to be the 'next' field of the newly appended
             * link */
            t_values_head = &(*t_values_head)->next;
        }
        
        /* Check for either ',' or '}' */
        Symbol_type t_sep_type;
        if (sp.next(t_sep_type) != PS_NORMAL ||
            (t_sep_type != ST_SEP && t_sep_type != ST_RC))
        {
            MCperror->add(PE_ARRLITERAL_SEPEXPECTED, sp);
            return PS_ERROR;
        }
        
        /* As soon as we encounter '}', we are done. */
        if (t_sep_type == ST_RC)
        {
            break;
        }
    }
    
    /* If the base value array has at least one element we make it immutable
     * and then unique it. This means that array literals which have the same
     * base value, will share that value in memory. If the base value array has
     * no elements, then we use the empty array. */
    if (MCArrayGetCount(*t_base_value) != 0)
    {
        if (!t_base_value.MakeImmutable() ||
            !t_base_value.MakeUnique())
        {
            MCperror->add(PE_OUTOFMEMORY, sp);
            return PS_ERROR;
        }
        
        /* We can just use assignment here as we are assigning auto pointer to
         * auto pointer. A suitably clever C++ compiler will use the moving
         * constructor. */
        m_base_value.Reset(*t_base_value);
    }
    else
    {
        m_base_value = kMCEmptyArray;
    }

    return PS_NORMAL;
}

void MCArrayLiteral::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    /* If there are no dynamic values then we are done - the value of the
     * literal is the base value. */
    if (m_dynamic_values == nullptr)
    {
        r_value . type = kMCExecValueTypeValueRef;
        r_value . valueref_value = MCValueRetain(*m_base_value);
        return;
    }
    
    /* There are dynamic values, so we need to add them to a copy of the base
     * value. */
    MCAutoArrayRef t_literal;
    if (!MCArrayMutableCopy(*m_base_value,
                            &t_literal))
    {
        ctxt.LegacyThrow(EE_NO_MEMORY);
        return;
    }
    
    /* Loop through the singly-linked dynamic value list. */
    for(DynamicValue *t_dynamic_value = m_dynamic_values;
        t_dynamic_value != nullptr;
        t_dynamic_value = t_dynamic_value->next)
    {
        /* First try to evaluate the key expression (or use the constant value
         * if constant). */
        MCNewAutoNameRef t_key;
        if (t_dynamic_value->key_is_dynamic)
        {
            if (!ctxt.EvalExprAsNameRef(t_dynamic_value->dynamic_key,
                                        EE_ARRLITERAL_BADKEYEXPR,
                                        &t_key))
            {
                return;
            }
        }
        else
        {
            t_key = t_dynamic_value->constant_key;
        }
        
        /* Next try to evaluate the value expression (or use the constant value
         * if constant). */
        MCAutoValueRef t_value;
        if (t_dynamic_value->value_is_dynamic)
        {
            if (!ctxt.EvalExprAsValueRef(t_dynamic_value->dynamic_value,
                                         EE_ARRLITERAL_BADVALEXPR,
                                         &t_value))
            {
                return;
            }
        }
        else
        {
            t_value = t_dynamic_value->constant_value;
        }
        
        /* If evaluation succeeds, try to store the value */
        if (!MCArrayStoreValue(*t_literal,
                               ctxt.GetCaseSensitive(),
                               *t_key,
                               *t_value))
        {
            ctxt.Throw();
            return;
        }
    }
    
    /* Make sure the value is immutable */
    if (!t_literal.MakeImmutable())
    {
        ctxt.Throw();
        return;
    }
    
    /* We are done - return the constructed literal as an ExecValue. */
	r_value . type = kMCExecValueTypeValueRef;
	r_value . valueref_value = t_literal.Take();
}

#endif

