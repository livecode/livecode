
#include "foundation.h"

void MCStringExecPutStringBefore(MCHandlerContext& ctxt, MCStringRef p_source, MCStringRef& x_target)
{
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(x_target, &t_string))
        return;
    
    if (!MCStringPrepend(*t_string, p_source))
        return;
    
    if (!MCStringCopy(*t_string, x_target))
        return;
}

void MCStringExecPutStringAfter(MCHandlerContext& ctxt, MCStringRef p_source, MCStringRef& x_target)
{
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(x_target, &t_string))
        return;
    
    if (!MCStringAppend(*t_string, p_source))
        return;
    
    if (!MCStringCopy(*t_string, x_target))
        return;
}

void MCStringEvalConcatenate(MCHandlerContext& ctxt, MCStringRef p_left, MCStringRef p_right, MCStringRef& r_output)
{
    if (!MCStringFormat(r_output, "%@%@", p_left, p_right))
        return;
}

void MCStringEvalConcatenateWithSpace(MCHandlerContext& ctxt, MCStringRef p_left, MCStringRef p_right, MCStringRef& r_output)
{
    if (!MCStringFormat(r_output, "%@ %@", p_left, p_right))
        return;
}