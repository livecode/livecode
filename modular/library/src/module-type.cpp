
#include "foundation.h"

void MCTypeEvalBoolAsString(bool p_target, MCStringRef& r_output)
{
    r_output = MCValueRetain(p_target ? kMCTrueString : kMCFalseString);
}