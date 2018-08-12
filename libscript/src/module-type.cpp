
#include <foundation.h>
#include <foundation-auto.h>

static bool MCTypeValueIsEmpty(MCValueRef p_value)
{
    return p_value != kMCNull &&
    (p_value == kMCEmptyName ||
    (MCValueGetTypeCode(p_value) == kMCValueTypeCodeArray && MCArrayIsEmpty((MCArrayRef)p_value)) ||
    (MCValueGetTypeCode(p_value) == kMCValueTypeCodeString && MCStringIsEmpty((MCStringRef)p_value)) ||
    (MCValueGetTypeCode(p_value) == kMCValueTypeCodeName && MCNameIsEmpty((MCNameRef)p_value)) ||
    (MCValueGetTypeCode(p_value) == kMCValueTypeCodeData && MCDataIsEmpty((MCDataRef)p_value)) ||
    (MCValueGetTypeCode(p_value) == kMCValueTypeCodeProperList && MCProperListIsEmpty((MCProperListRef)p_value)));
}

extern "C" MC_DLLEXPORT_DEF void MCTypeEvalIsEmpty(MCValueRef p_target, bool& r_output)
{
    r_output = MCTypeValueIsEmpty(p_target);
}

extern "C" MC_DLLEXPORT_DEF void MCTypeEvalIsNotEmpty(MCValueRef p_target, bool& r_output)
{
    bool t_empty;
    MCTypeEvalIsEmpty(p_target, t_empty);
    
    r_output = !t_empty;
}

extern "C" MC_DLLEXPORT_DEF void MCTypeEvalIsDefined(MCValueRef *p_target, bool& r_output)
{
    r_output = (p_target != nil && *p_target != kMCNull);
}

extern "C" MC_DLLEXPORT_DEF void MCTypeEvalIsNotDefined(MCValueRef *p_target, bool& r_output)
{
    bool t_defined;
    MCTypeEvalIsDefined(p_target, t_defined);
    
    r_output = !t_defined;
}

extern "C" MC_DLLEXPORT_DEF void MCTypeEvalIsABoolean(MCValueRef p_value, bool& r_output)
{
    if (p_value != nil)
        r_output = MCValueGetTypeCode(p_value) == kMCValueTypeCodeBoolean;
    else
        r_output = false;
}

extern "C" MC_DLLEXPORT_DEF void MCTypeEvalIsANumber(MCValueRef p_value, bool& r_output)
{
    if (p_value != nil)
        r_output = MCValueGetTypeCode(p_value) == kMCValueTypeCodeNumber;
    else
        r_output = false;
}

extern "C" MC_DLLEXPORT_DEF void MCTypeEvalIsAString(MCValueRef p_value, bool& r_output)
{
    if (p_value != nil)
        r_output = MCValueGetTypeCode(p_value) == kMCValueTypeCodeString;
    else
        r_output = false;
}

extern "C" MC_DLLEXPORT_DEF void MCTypeEvalIsAData(MCValueRef p_value, bool& r_output)
{
    if (p_value != nil)
        r_output = MCValueGetTypeCode(p_value) == kMCValueTypeCodeData;
    else
        r_output = false;
}

extern "C" MC_DLLEXPORT_DEF void MCTypeEvalIsAnArray(MCValueRef p_value, bool& r_output)
{
    if (p_value != nil)
        r_output = MCValueGetTypeCode(p_value) == kMCValueTypeCodeArray;
    else
        r_output = false;
}

extern "C" MC_DLLEXPORT_DEF void MCTypeEvalIsAList(MCValueRef p_value, bool& r_output)
{
    if (p_value != nil)
        r_output = MCValueGetTypeCode(p_value) == kMCValueTypeCodeProperList;
    else
        r_output = false;
}

extern "C" MC_DLLEXPORT_DEF void MCNothingEvalIsNothingEqualTo(MCNullRef p_left, MCValueRef p_right, bool& r_output)
{
    r_output = p_right == nil;
}

extern "C" MC_DLLEXPORT_DEF void MCNothingEvalIsEqualToNothing(MCValueRef p_left, MCNullRef p_right, bool& r_output)
{
    r_output = p_left == nil;
}

extern "C" MC_DLLEXPORT_DEF void MCNothingEvalIsNothingNotEqualTo(MCNullRef p_left, MCValueRef p_right, bool& r_output)
{
    r_output = p_right != nil;
}

extern "C" MC_DLLEXPORT_DEF void MCNothingEvalIsNotEqualToNothing(MCValueRef p_left, MCNullRef p_right, bool& r_output)
{
    r_output = p_left != nil;
}

extern "C" MC_DLLEXPORT_DEF void MCNothingEvalIsNothingEqualToNothing(MCValueRef, MCValueRef, bool& r_output)
{
    r_output = true;
}

extern "C" MC_DLLEXPORT_DEF void MCNothingEvalIsNothingNotEqualToNothing(MCValueRef, MCValueRef, bool& r_output)
{
    r_output = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_type_Initialize (void)
{
	return true;
}

extern "C" void com_livecode_type_Finalize (void)
{
	;
}

////////////////////////////////////////////////////////////////

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
