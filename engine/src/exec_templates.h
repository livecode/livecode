//
//  exec_templates.hpp
//  kernel
//
//  Created by Livecode on 10/9/17.
//

#ifndef exec_templates_h
#define exec_templates_h

#include "executionerrors.h"

enum EVAL_RESULT
{
    NON_NULL_SUCC,
    NON_NULL_FAIL,
    IS_NULL
};

typedef enum Exec_errors ErrorT;

template<typename To, ErrorT error>
struct StandardArg
{
    typedef To DestType;
    typedef MCExpression * SourceType;
    static EVAL_RESULT Eval(MCExecContext& ctxt, MCExpression* p_source, To& r_dest)
    {
        if (MCExecValueTraits<DestType>::eval(ctxt, p_source, error, r_dest))
        {
            return NON_NULL_SUCC;
        }
        return NON_NULL_FAIL;
    }
};

template<typename To, To def_value, ErrorT error>
struct DefaultedArg
{
    typedef To DestType;
    typedef MCExpression * SourceType;
    static EVAL_RESULT Eval(MCExecContext& ctxt, MCExpression* p_source, To& r_dest)
    {
        if (p_source == nullptr)
        {
            r_dest = def_value;
            return NON_NULL_SUCC;
        }
        if (MCExecValueTraits<DestType>::eval(ctxt, p_source, error, r_dest))
        {
            return NON_NULL_SUCC;
        }
        return NON_NULL_FAIL;
    }
};

template<typename To, ErrorT error>
struct NullableArg
{
    typedef To DestType;
    typedef MCExpression * SourceType;
    
    static EVAL_RESULT Eval(MCExecContext &ctxt, SourceType p_source, DestType &r_dest)
    {
        if (p_source == NULL)
        {
            return IS_NULL;
        }
        if (MCExecValueTraits<DestType>::eval(ctxt, p_source, error, r_dest))
        {
            return NON_NULL_SUCC;
        }
        return NON_NULL_FAIL;
    }
};

template<typename ExecMethodDescT>
void Dispatch(MCExecContext &ctxt, typename ExecMethodDescT::Arg1::SourceType arg_1)
{
    typename ExecMethodDescT::Arg1::DestType r_value;
    
    EVAL_RESULT arg_eval = ExecMethodDescT::Arg1::Eval(ctxt, arg_1, r_value);
    
    switch(arg_eval)
    {
        case IS_NULL:
            ExecMethodDescT::NullFunc(ctxt);
        case NON_NULL_SUCC:
            ExecMethodDescT::Func(ctxt, r_value);
        case NON_NULL_FAIL:
            break;
    }
}

template<typename ExecMethodDescT, typename FuncName>
void Dispatch(MCExecContext &ctxt, typename ExecMethodDescT::Arg1::SourceType arg_1, typename ExecMethodDescT::Arg2::SourceType arg_2, FuncName &&interimFunction, typename ExecMethodDescT::StateType state_var)
{
    typename ExecMethodDescT::Arg1::DestType r_value_1;
    
    EVAL_RESULT arg_1_eval = ExecMethodDescT::Arg1::Eval(ctxt, arg_1, r_value_1);
    
    switch(arg_1_eval)
    {
        case IS_NULL:
            break;
        case NON_NULL_SUCC:
            break;
        case NON_NULL_FAIL:
            break;
    }
    
    interimFunction(r_value_1);
    
    typename ExecMethodDescT::Arg2::DestType r_value_2;
    
    EVAL_RESULT arg_2_eval = ExecMethodDescT::Arg2::Eval(ctxt, arg_2, r_value_2);
    
    switch(arg_2_eval)
    {
        case IS_NULL:
            ExecMethodDescT::NullFunc(ctxt);
        case NON_NULL_SUCC:
            ExecMethodDescT::Func(ctxt, r_value_1, r_value_2, state_var);
        case NON_NULL_FAIL:
            break;
    }
    
}

#endif /* exec_templates_hpp */
