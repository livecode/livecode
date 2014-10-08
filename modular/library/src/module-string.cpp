
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

void MCStringEvalBeginsWith(MCHandlerContext& ctxt, MCStringRef p_source, MCStringRef p_prefix, bool& r_result)
{
    r_result = MCStringBeginsWith(p_source, p_prefix, ctxt . GetStringComparisonOptions());
}

void MCStringEvalEndsWith(MCHandlerContext& ctxt, MCStringRef p_source, MCStringRef p_suffix, bool& r_result)
{
    r_result = MCStringBeginsWith(p_source, p_prefix, ctxt . GetStringComparisonOptions());
}

void MCStringEvalLowercaseOf(MCHandlerContext& ctxt, MCStringRef p_source, MCStringRef& r_output)
{
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(p_source, &t_string))
        return;
    
    if (!MCStringLowercase(*t_string, kMCBasicLocale))
        return;
    
    if (!MCStringCopy(*t_string, r_output))
        return;
}

void MCStringEvalUppercaseOf(MCHandlerContext& ctxt, MCStringRef p_source, MCStringRef& r_output)
{
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(p_source, &t_string))
        return;
    
    if (!MCStringUppercase(*t_string, kMCBasicLocale))
        return;
    
    if (!MCStringCopy(*t_string, r_output))
        return;
}

void MCStringEvalOffset(MCHandlerContext& ctxt, MCStringRef p_needle, MCStringRef p_source, uindex_t& r_output)
{
    MCStringEvalOffsetAfter(ctxt, p_needle, p_after, p_source, r_output);
}

void MCStringEvalOffsetAfter(MCHandlerContext& ctxt, MCStringRef p_needle, uindex_t p_after, MCStringRef p_source, uindex_t& r_output)
{
    uindex_t t_output = 0;
    if (!MCStringIsEmpty(p_needle))
        MCStringFirstIndexOf(p_source, p_needle, p_after, ctxt . GetStringComparisonOptions(), t_output);
    r_output = t_output - p_after + 1;
}

void MCStringEvalLastOffset(MCHandlerContext& ctxt, MCStringRef p_needle, MCStringRef p_source, uindex_t& r_output)
{
    MCStringEvalLastOffsetBefore(ctxt, p_needle, UINDEX_MAX, p_source, r_output);
}

void MCStringEvalLastOffsetBefore(MCHandlerContext& ctxt, MCStringRef p_needle, uindex_t p_before, MCStringRef p_source, uindex_t& r_output)
{
    uindex_t t_output = 0;
    if (!MCStringIsEmpty(p_needle))
        MCStringFirstIndexOf(p_source, p_needle, p_before, ctxt . GetStringComparisonOptions(), t_output);
    r_output = t_output + 1;
}
