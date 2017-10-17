//
//  exec_templates.hpp
//  kernel
//
//  Created by Livecode on 10/9/17.
//

#ifndef exec_templates_h
#define exec_templates_h

#include "executionerrors.h"


typedef enum Exec_errors ErrorT;

template<typename To, ErrorT error>
struct StandardArg
{
    typedef To DestType;
    typedef MCExpression * SourceType;
    static bool Eval(MCExecContext& ctxt, MCExpression* p_source, To& r_dest)
    {
        if (MCExecValueTraits<DestType>::eval(ctxt, p_source, error, r_dest))
        {
            return true;
        }
        return false;
    }
};

template<typename To, To def_value, ErrorT error>
struct DefaultedArg
{
    typedef To DestType;
    typedef MCExpression * SourceType;
    static bool Eval(MCExecContext& ctxt, MCExpression* p_source, To& r_dest)
    {
        if (p_source == nullptr)
        {
            r_dest = def_value;
            return true;
        }
        if (MCExecValueTraits<DestType>::eval(ctxt, p_source, error, r_dest))
        {
            return true;
        }
        return false;
    }
};

template<typename ExecMethodDescT>
void Dispatch(MCExecContext &ctxt)
{
    ExecMethodDescT::Func(ctxt);
}

template<typename ExecMethodDescT>
void Dispatch(MCExecContext &ctxt, typename ExecMethodDescT::Arg1::SourceType arg_1)
{
    typename ExecMethodDescT::Arg1::DestType r_value;
    
    if (ExecMethodDescT::Arg1::Eval(ctxt, arg_1, r_value))
    {
        ExecMethodDescT::Func(ctxt, r_value);
    }
}

template<typename ExecMethodDescT, typename FuncName>
void Dispatch(MCExecContext &ctxt, typename ExecMethodDescT::Arg1::SourceType arg_1, typename ExecMethodDescT::Arg2::SourceType arg_2, FuncName &&interimFunction, typename ExecMethodDescT::StateType state_var)
{
    typename ExecMethodDescT::Arg1::DestType r_value_1;
    
    if (!ExecMethodDescT::Arg1::Eval(ctxt, arg_1, r_value_1))
    {
        return;
    }
    
    interimFunction(r_value_1);
    
    typename ExecMethodDescT::Arg2::DestType r_value_2;
    
    if (!ExecMethodDescT::Arg2::Eval(ctxt, arg_2, r_value_2))
    {
        return;
    }
    
    ExecMethodDescT::Func(ctxt, r_value_1, r_value_2);
}

#endif /* exec_templates_hpp */
