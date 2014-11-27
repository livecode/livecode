
#include <foundation.h>
#include <foundation-auto.h>

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

void MCTypeEvalStringAsReal(MCStringRef p_target, real64_t& r_output)
{
    if (!MCTypeConvertStringToReal(p_target, r_output))
        return;
}

void MCTypeEvalStringAsInt(MCStringRef p_target, integer_t& r_output)
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
    //r_output = MCValueIsEmpty(p_target);
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

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _TEST
extern void log(const char *module, const char *test, bool result);
#define log_result(test, result) log("TYPE MODULE", test, result)
void MCTypeRunTests()
{
/*
     void MCTypeEvalBoolAsString(bool p_target, MCStringRef& r_output)
     void MCTypeEvalRealAsString(double p_target, MCStringRef& r_output)
     void MCTypeEvalIntAsString(integer_t p_target, MCStringRef& r_output)
     void MCTypeEvalStringAsBool(MCStringRef p_target, bool& r_output)
     void MCTypeEvalStringAsReal(MCStringRef p_target, real64_t r_output)
     void MCTypeEvalStringAsInt(MCStringRef p_target, integer_t r_output)
     void MCTypeEvalIntAsReal(integer_t p_target, real64_t r_output)
     void MCTypeEvalIsEmpty(MCValueRef p_target, bool& r_output)
     void MCTypeEvalIsNotEmpty(MCValueRef p_target, bool& r_output)
     void MCTypeEvalIsDefined(MCValueRef p_target, bool& r_output)
     void MCTypeEvalIsNotDefined(MCValueRef p_target, bool p_is_not, bool& r_output)
     void MCTypeEvalIsEqual(MCValueRef p_left, MCValueRef p_right, bool& r_output)
     void MCTypeEvalIsNotEqual(MCValueRef p_left, MCValueRef p_right, bool& r_output)
*/
    
    MCAutoStringRef t_int_string, t_real_string;
    MCAutoStringRef t_string_int, t_string_real;
    integer_t t_int, t_int2;
    real64_t t_real, t_real2;
    
    t_int = 1234567890;
    MCTypeEvalIntAsString(t_int, &t_int_string);
    MCTypeEvalStringAsInt(*t_int_string, t_int2);
    bool t_result;
    t_result = (t_int == t_int2);
    
    log_result("round trip integer -> string -> integer", t_result);
    
    t_real = 1.23545467e25;
    MCTypeEvalRealAsString(t_real, &t_real_string);
    MCTypeEvalStringAsReal(*t_real_string, t_real2);
    if (t_result)
        t_result = (t_real == t_real2);
    
    log_result("round trip real -> string -> real", t_result);
    
}
#endif
