
#include <foundation.h>

void MCTypeEvalBoolAsString(bool p_target, MCStringRef& r_output)
{
    r_output = MCValueRetain(p_target ? kMCTrueString : kMCFalseString);
}

void MCTypeEvalRealAsString(double p_target, MCStringRef& r_output)
{
    MCStringFormat(r_output, "%f", p_target);
}

void MCTypeEvalIntAsString(integer_t p_target, MCStringRef& r_output)
{
    MCStringFormat(r_output, "%d", p_target);
}

void MCTypeEvalStringAsBool(MCStringRef p_target, bool& r_output)
{
    if (!MCTypeConvertStringToBool(p_target, r_output))
        return;
}

void MCTypeEvalStringAsReal(MCStringRef p_target, real64_t r_output)
{
    if (!MCTypeConvertStringToReal(p_target, r_output))
        return;
}

void MCTypeEvalStringAsInt(MCStringRef p_target, integer_t r_output)
{
    if (!MCTypeConvertStringToLongInteger(p_target, r_output))
        return;
}

void MCTypeEvalIntAsReal(integer_t p_target, real64_t r_output)
{
    r_output = p_target;
}

void MCTypeEvalIsEmpty(MCValueRef p_target, bool& r_output)
{
    r_output = MCValueIsEmpty(p_target);
}

void MCTypeEvalIsNotEmpty(MCValueRef p_target, bool& r_output)
{
    bool t_empty;
    MCTypeEvalIsEmpty(p_target, t_empty);
    
    r_output = !t_empty;
}

void MCTypeEvalIsDefined(MCValueRef p_target, bool& r_output)
{
    r_output = p_target == kMCNull;
}

void MCTypeEvalIsNotDefined(MCValueRef p_target, bool p_is_not, bool& r_output)
{
    bool t_defined;
    MCTypeEvalIsDefined(p_target, t_defined);
    
    r_output = !t_defined;
}

void MCTypeEvalIsEqual(MCValueRef p_left, MCValueRef p_right, bool& r_output)
{
    r_output = MCValueIsEqualTo(p_left, p_right);
}

void MCTypeEvalIsNotEqual(MCValueRef p_left, MCValueRef p_right, bool& r_output)
{
    bool t_equal;
    MCTypeEvalIsEqual(p_left, p_right, t_equal);
    
    r_output = !t_equal;
}