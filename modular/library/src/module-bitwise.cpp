
#include <foundation.h>

void MCBitwiseEvalBitwiseAnd(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    r_output = p_left & p_right;
}

void MCBitwiseEvalBitwiseOr(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    r_output = p_left | p_right;
}

void MCBitwiseEvalBitwiseXor(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    r_output = p_left ^ p_right;
}

void MCBitwiseEvalBitwiseNot(integer_t p_operand, integer_t& r_output)
{
    r_output = ~p_operand;
}

void MCBitwiseEvalBitwiseShift(integer_t p_operand, uinteger_t p_shift, bool p_is_right, integer_t& r_output)
{
    r_output = p_is_right ? p_operand >> p_shift : p_operand << p_shift;
}