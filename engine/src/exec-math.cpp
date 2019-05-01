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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "osspec.h"
#include "util.h"

#include "exec.h"

#include "foundation-math.h"

////////////////////////////////////////////////////////////////////////////////

void MCMathEvalBaseConvert(MCExecContext& ctxt, MCStringRef p_source, integer_t p_source_base, integer_t p_dest_base, MCStringRef& r_result)
{
	if (p_source_base < 2 || p_source_base > 36)
	{
		ctxt . LegacyThrow(EE_BASECONVERT_BADSOURCEBASE);
		return;
	}

	if (p_dest_base < 2 || p_dest_base > 36)
	{
		ctxt . LegacyThrow(EE_BASECONVERT_BADDESTBASE);
		return;
	}

    bool t_negative;
    uinteger_t t_digits;
    bool t_error;
    if (!MCMathConvertToBase10(p_source, p_source_base, t_negative, t_digits, t_error))
    {
        if (t_error)
            ctxt . LegacyThrow(EE_BASECONVERT_CANTCONVERT, p_source);
        else
            // Memory error
            ctxt . Throw();
		return;
	}

    if (!MCMathConvertFromBase10(t_digits, t_negative, p_dest_base, r_result))
    {
		ctxt . Throw();
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCMathEvalAbs(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	r_result = fabs(p_in);
}

void MCMathEvalRoundToPrecision(MCExecContext& ctxt, real64_t p_number, real64_t p_precision, real64_t& r_result)
{
	real64_t t_factor;
	// eliminate precision error by using more than exact power of 10,
	// 6.0 is empirically derived on Intel.  Other rounding algorithms
	// don't work with negative numbers and/or are very slow.
	t_factor = pow(10 + DBL_EPSILON * 6.0, p_precision);

	r_result = p_number * t_factor;
	if (r_result < 0.0)
		r_result = ceil(r_result - 0.5);
	else
		r_result = floor(r_result + 0.5);
	r_result /= t_factor;
}

void MCMathEvalRound(MCExecContext& ctxt, real64_t p_number, real64_t& r_result)
{
	MCMathEvalRoundToPrecision(ctxt, p_number, 0.0, r_result);
}

void MCMathEvalStatRoundToPrecision(MCExecContext& ctxt, real64_t p_number, real64_t p_precision, real64_t& r_result)
{
	real64_t t_factor;
	// eliminate precision error by using more than exact power of 10,
	// 6.0 is empirically derived on Intel.  Other rounding algorithms
	// don't work with negative numbers and/or are very slow.
	t_factor = pow(10 + DBL_EPSILON * 6.0, p_precision);

	r_result = p_number * t_factor;
	if (r_result < 0.0)
	{
		r_result -= 0.5;
		if (fmod(r_result, 2.0) == -1.0)
			r_result += 1.0;
		else
			r_result = ceil(r_result);
	}
	else
	{
		r_result += 0.5;
		if (fmod(r_result, 2.0) == 1.0)
			r_result -= 1.0;
		else
			r_result = floor(r_result);
	}
	r_result /= t_factor;
}

void MCMathEvalStatRound(MCExecContext& ctxt, real64_t p_number, real64_t& r_result)
{
	MCMathEvalStatRoundToPrecision(ctxt, p_number, 0.0, r_result);
}

void MCMathEvalTrunc(MCExecContext& ctxt, real64_t p_number, real64_t& r_result)
{
	if (p_number < 0.0)
		r_result = ceil(p_number);
	else
		r_result = floor(p_number);
}

void MCMathEvalFloor(MCExecContext& ctxt, real64_t p_number, real64_t& r_result)
{
	r_result = floor(p_number);
}

void MCMathEvalCeil(MCExecContext& ctxt, real64_t p_number, real64_t& r_result)
{
	r_result = ceil(p_number);
}

//////////

static bool MCMathCheckBinaryResult(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t p_result)
{
    // If the result is finite, we're fine
    if (MCS_isfinite(p_result))
    {
        return true;
    }
    
    // Otherwise, if any of the inputs were not finite, we just allow
    // whatever result was obtained to flow through
    if (!MCS_isfinite(p_left) || !MCS_isfinite(p_right))
    {
        return true;
    }

    return false;
}

static bool MCMathCheckUnaryResult(MCExecContext& ctxt, real64_t p_operand, real64_t p_result)
{
    // If the result is finite, we're fine
    if (MCS_isfinite(p_result))
    {
        return true;
    }
    
    // Otherwise, if the input not finite, we just allow whatever result was
    // obtained to flow through
    if (!MCS_isfinite(p_operand))
    {
        return true;
    }
    
    return false;
}

static bool MCMathCheckNaryResult(MCExecContext& ctxt, real64_t* p_values, uindex_t p_count, real64_t p_result)
{
    // If the result is finite, we're fine
    if (MCS_isfinite(p_result))
    {
        return true;
    }
    
    // Otherwise, if any inputs are not finite, we just allow whatever result was
    // obtained to flow through
    for (uindex_t i = 0; i < p_count; i++)
    {
        if (!MCS_isfinite(p_values[i]))
        {
            return true;
        }
    }
    
    return false;
}

static void MCMathThrowFloatingPointException(MCExecContext& ctxt, real64_t p_result)
{
    if (MCS_isnan(p_result))
    {
        ctxt.LegacyThrow(EE_MATH_DOMAIN);
    }
    else
    {
        ctxt.LegacyThrow(EE_MATH_RANGE);
    }
}

template <real64_t (*Func)(real64_t)>
void MCMathEvalCheckedUnaryFunction(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
    real64_t t_result = Func(p_in);
    if (!MCMathCheckUnaryResult(ctxt, p_in, t_result))
    {
        MCMathThrowFloatingPointException(ctxt, t_result);
        return;
    }
    r_result = t_result;
}

template <real64_t (*Func)(real64_t), bool (*CheckDivideByZero)(real64_t)>
void MCMathEvalCheckedUnaryFunction(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
    real64_t t_result = Func(p_in);
    if (!MCMathCheckUnaryResult(ctxt, p_in, t_result))
    {
        if (CheckDivideByZero(p_in))
        {
            ctxt.LegacyThrow(EE_MATH_ZERO);
        }
        else
        {
            MCMathThrowFloatingPointException(ctxt, t_result);
        }
    }
    r_result = t_result;
}

template <real64_t (*Func)(real64_t, real64_t)>
void MCMathEvalCheckedBinaryFunction(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
    real64_t t_result = Func(p_left, p_right);
    if (!MCMathCheckBinaryResult(ctxt, p_left, p_right, t_result))
    {
        MCMathThrowFloatingPointException(ctxt, t_result);
        return;
    }
    r_result = t_result;
}

template <real64_t (*Func)(real64_t, real64_t), bool (*CheckDivideByZero)(real64_t, real64_t)>
void MCMathEvalCheckedBinaryFunction(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
    real64_t t_result = Func(p_left, p_right);
    if (!MCMathCheckBinaryResult(ctxt, p_left, p_right, t_result))
    {
        if (CheckDivideByZero(p_left, p_right))
        {
            ctxt.LegacyThrow(EE_MATH_ZERO);
        }
        else
        {
            MCMathThrowFloatingPointException(ctxt, t_result);
        }
        return;
    }
    r_result = t_result;
}

template <real64_t (*Func)(real64_t*, uindex_t)>
void MCMathEvalCheckedNaryFunction(MCExecContext& ctxt, real64_t* p_values, uindex_t p_count, real64_t& r_result)
{
    if (p_count == 0)
    {
        r_result = 0.0;
        return;
    }
    
    real64_t t_result = Func(p_values, p_count);
    if (!MCMathCheckNaryResult(ctxt, p_values, p_count, t_result))
    {
        MCMathThrowFloatingPointException(ctxt, t_result);
        return;
    }
    r_result = t_result;
}

void MCMathEvalAcos(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
    MCMathEvalCheckedUnaryFunction<acos>(ctxt, p_in, r_result);
}

void MCMathEvalAsin(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
    MCMathEvalCheckedUnaryFunction<asin>(ctxt, p_in, r_result);
}

void MCMathEvalAtan(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	MCMathEvalCheckedUnaryFunction<atan>(ctxt, p_in, r_result);
}

void MCMathEvalAtan2(MCExecContext& ctxt, real64_t p_y, real64_t p_x, real64_t& r_result)
{
	MCMathEvalCheckedBinaryFunction<atan2>(ctxt, p_y, p_x, r_result);
}

void MCMathEvalCos(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	MCMathEvalCheckedUnaryFunction<cos>(ctxt, p_in, r_result);
}

void MCMathEvalSin(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	MCMathEvalCheckedUnaryFunction<sin>(ctxt, p_in, r_result);
}

void MCMathEvalTan(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	MCMathEvalCheckedUnaryFunction<tan>(ctxt, p_in, r_result);
}

//////////

void MCMathEvalExp(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
    MCMathEvalCheckedUnaryFunction<exp>(ctxt, p_in, r_result);
}

void MCMathEvalExp1(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
    MCMathEvalCheckedUnaryFunction<expm1>(ctxt, p_in, r_result);
}

void MCMathEvalExp2(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
    MCMathEvalCheckedUnaryFunction<exp2>(ctxt, p_in, r_result);
}

void MCMathEvalExp10(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
    MCMathEvalCheckedBinaryFunction<pow>(ctxt, 10.0, p_in, r_result);
}

//////////

static bool mc_log_check_divide_by_zero(real64_t p_in)
{
    return (p_in == 0);
}

static bool mc_log1_check_divide_by_zero(real64_t p_in)
{
    return (p_in == -1);
}

static real64_t mc_log2(real64_t p_in)
{
#ifdef TARGET_SUBPLATFORM_ANDROID
    return (log(p_in) / log(2.0));
#else
    return log2(p_in);
#endif
}


void MCMathEvalLn(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
    MCMathEvalCheckedUnaryFunction<log,mc_log_check_divide_by_zero>(ctxt, p_in, r_result);
}

void MCMathEvalLn1(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
    MCMathEvalCheckedUnaryFunction<log1p,mc_log1_check_divide_by_zero>(ctxt, p_in, r_result);
}

void MCMathEvalLog2(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
    MCMathEvalCheckedUnaryFunction<mc_log2,mc_log_check_divide_by_zero>(ctxt, p_in, r_result);
}

void MCMathEvalLog10(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
    MCMathEvalCheckedUnaryFunction<log10,mc_log_check_divide_by_zero>(ctxt, p_in, r_result);
}

//////////

void MCMathEvalSqrt(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
    MCMathEvalCheckedUnaryFunction<sqrt>(ctxt, p_in, r_result);
}

////////////////////////////////////////////////////////////////////////////////

static real64_t mc_annuity(real64_t p_rate, real64_t p_periods)
{
    if (p_rate == 0)
        return p_periods;
    return (1.0 - pow(1.0 + p_rate, -p_periods)) / p_rate;
}

static bool mc_annuity_check_divide_by_zero(real64_t p_rate, real64_t p_periods)
{
    return (p_rate == -1 && p_periods > 0);
}

static real64_t mc_compound(real64_t p_rate, real64_t p_periods)
{
    return pow(1.0 + p_rate, p_periods);
}

static bool mc_compound_check_divide_by_zero(real64_t p_rate, real64_t p_periods)
{
    return (p_rate == -1 && p_periods < 0);
}

void MCMathEvalAnnuity(MCExecContext& ctxt, real64_t p_rate, real64_t p_periods, real64_t& r_result)
{
	MCMathEvalCheckedBinaryFunction<mc_annuity,mc_annuity_check_divide_by_zero>(ctxt, p_rate, p_periods, r_result);
}

void MCMathEvalCompound(MCExecContext& ctxt, real64_t p_rate, real64_t p_periods, real64_t& r_result)
{
	MCMathEvalCheckedBinaryFunction<mc_compound,mc_compound_check_divide_by_zero>(ctxt, p_rate, p_periods, r_result);
}

////////////////////////////////////////////////////////////////////////////////

static real64_t mc_arithmetic_mean(real64_t *p_values, uindex_t p_count)
{
    real64_t t_total = 0.0;
    for (uindex_t i = 0; i < p_count; i++)
    {
        t_total += p_values[i];
    }
    return t_total / p_count;
}

static real64_t mc_geometric_mean(real64_t *p_values, uindex_t p_count)
{
    real64_t t_result = 1.0;
    real64_t t_exponent = 1.0 / p_count;
    for (uindex_t i = 0; i < p_count; i++)
    {
        t_result *= pow(p_values[i], t_exponent);
    }
    
    return t_result;
}

static real64_t mc_harmonic_mean(real64_t *p_values, uindex_t p_count)
{
    real64_t t_total = 0.0;
    for (uindex_t i = 0; i < p_count; i++)
    {
        t_total += 1 / p_values[i];
    }
    return p_count / t_total;
}

int cmp_real64_t(const void* a, const void* b)
{
    real64_t t_a, t_b;
    t_a = *(real64_t*)a;
    t_b = *(real64_t*)b;
    return (t_a > t_b) - (t_a < t_b);
}

static real64_t mc_median(real64_t *p_values, uindex_t p_count)
{
    qsort((void*)p_values, p_count, sizeof(real64_t), cmp_real64_t);
    uindex_t t_index = p_count / 2;
    if (p_count & 1) // odd
        return p_values[t_index];
    else // even, return avg of two nearest-to-centre values
        return (p_values[t_index - 1] + p_values[t_index]) / 2.0;
}

static real64_t mc_min(real64_t *p_values, uindex_t p_count)
{
    real64_t t_result = p_values[0];
    for (uindex_t i = 1; i < p_count; i++)
    {
        if (p_values[i] < t_result)
            t_result = p_values[i];
    }
    return t_result;
}

static real64_t mc_max(real64_t *p_values, uindex_t p_count)
{
    real64_t t_result = p_values[0];
    for (uindex_t i = 1; i < p_count; i++)
    {
        if (p_values[i] > t_result)
            t_result = p_values[i];
    }
    return t_result;
}

static real64_t mc_sum(real64_t *p_values, uindex_t p_count)
{
    real64_t t_total = 0.0;
    for (uindex_t i = 0; i < p_count; i++)
        t_total += p_values[i];
    
    return t_total;
}

void MCMathEvalArithmeticMean(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvalCheckedNaryFunction<mc_arithmetic_mean>(ctxt, p_values, p_count, r_result);
}

void MCMathEvalGeometricMean(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvalCheckedNaryFunction<mc_geometric_mean>(ctxt, p_values, p_count, r_result);
}

void MCMathEvalHarmonicMean(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvalCheckedNaryFunction<mc_harmonic_mean>(ctxt, p_values, p_count, r_result);
}

void MCMathEvalMedian(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvalCheckedNaryFunction<mc_median>(ctxt, p_values, p_count, r_result);
}

void MCMathEvalMin(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvalCheckedNaryFunction<mc_min>(ctxt, p_values, p_count, r_result);
}

void MCMathEvalMax(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvalCheckedNaryFunction<mc_max>(ctxt, p_values, p_count, r_result);
}

void MCMathEvalSum(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvalCheckedNaryFunction<mc_sum>(ctxt, p_values, p_count, r_result);
}

////////////////////////////////////////////////////////////////////////////////

void MCMathEvalRandom(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	p_in = floor(p_in + 0.5);
	// MW-2007-07-03: [[ Bug 4506 ]] - Large integers result in negative numbers
	// being generated.
	if (p_in < 1.0 || p_in > DBL_INT_MAX)
	{
		ctxt.LegacyThrow(EE_RANDOM_BADSOURCE);
		return;
	}

	r_result = floor(p_in * MCU_drand()) + 1;
}

////////////////////////////////////////////////////////////////////////////////

void MCMathEvalBitwiseAnd(MCExecContext& ctxt, uinteger_t p_left, uinteger_t p_right, uinteger_t& r_result)
{
	r_result = p_left & p_right;
}

void MCMathEvalBitwiseNot(MCExecContext& ctxt, uinteger_t p_right, uinteger_t& r_result)
{
	r_result = ~p_right;
}

void MCMathEvalBitwiseOr(MCExecContext& ctxt, uinteger_t p_left, uinteger_t p_right, uinteger_t& r_result)
{
	r_result = p_left | p_right;
}

void MCMathEvalBitwiseXor(MCExecContext& ctxt, uinteger_t p_left, uinteger_t p_right, uinteger_t& r_result)
{
	r_result = p_left ^ p_right;
}

////////////////////////////////////////////////////////////////////////////////

static real64_t mc_div(real64_t p_left, real64_t p_right)
{
    real64_t t_result = p_left / p_right;
    if (t_result < 0.0)
        return ceil(t_result);
    else
        return floor(t_result);
}

static bool mc_mod_check_divide_by_zero(real64_t p_left, real64_t p_right)
{
    return (p_right == 0);
}

static bool mc_over_check_divide_by_zero(real64_t p_left, real64_t p_right)
{
    return (p_right == 0);
}

void MCMathEvalDiv(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
    MCMathEvalCheckedBinaryFunction<mc_div,mc_over_check_divide_by_zero>(ctxt, p_left, p_right, r_result);
}

void MCMathEvalMod(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
    MCMathEvalCheckedBinaryFunction<fmod,mc_mod_check_divide_by_zero>(ctxt, p_left, p_right, r_result);
}

void MCMathEvalWrap(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
    MCMathEvalCheckedBinaryFunction<MCU_fwrap,mc_mod_check_divide_by_zero>(ctxt, p_left, p_right, r_result);
}

////////////////////////////////////////////////////////////////////////////////

static real64_t mc_minus(real64_t p_left, real64_t p_right)
{
    return p_left - p_right;
}

static real64_t mc_over(real64_t p_left, real64_t p_right)
{
    return p_left / p_right;
}

static real64_t mc_plus(real64_t p_left, real64_t p_right)
{
    return p_left + p_right;
}

static real64_t mc_multiply(real64_t p_left, real64_t p_right)
{
    return p_left * p_right;
}

static bool mc_pow_check_divide_by_zero(real64_t p_left, real64_t p_right)
{
    return (p_left == 0 && p_right < 0);
}

void MCMathEvalSubtract(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
    MCMathEvalCheckedBinaryFunction<mc_minus>(ctxt, p_left, p_right, r_result);
}

void MCMathEvalOver(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
    MCMathEvalCheckedBinaryFunction<mc_over,mc_over_check_divide_by_zero>(ctxt, p_left, p_right, r_result);
}

void MCMathEvalAdd(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
    MCMathEvalCheckedBinaryFunction<mc_plus>(ctxt, p_left, p_right, r_result);
}

void MCMathEvalMultiply(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
    MCMathEvalCheckedBinaryFunction<mc_multiply>(ctxt, p_left, p_right, r_result);
}

void MCMathEvalPower(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
    MCMathEvalCheckedBinaryFunction<pow, mc_pow_check_divide_by_zero>(ctxt, p_left, p_right, r_result);
}

//////////

void MCMathArrayApplyOperationWithNumber(MCExecContext& ctxt, MCArrayRef p_array, Operators p_op, real64_t p_number, MCArrayRef& r_result)
{
	MCAutoArrayRef t_array;
	if (MCArrayGetCount(p_array) == 0)
	{
		r_result = MCValueRetain(kMCEmptyArray);
		return;
	}

	if (!MCArrayCreateMutable(&t_array))
	{
		ctxt.Throw();
		return;
	}

	MCNameRef t_key;
	MCValueRef t_value;
	uintptr_t t_index = 0;

	MCS_seterrno(0);
	while (MCArrayIterate(p_array, t_index, t_key, t_value))
	{
		real64_t t_number;
		if (!ctxt.ConvertToReal(t_value, t_number))
		{
			ctxt.Throw();
			return;
		}

		switch (p_op)
		{
		case O_PLUS:
			MCMathEvalAdd(ctxt, t_number, p_number, t_number);
			break;
		case O_MINUS:
			MCMathEvalSubtract(ctxt, t_number, p_number, t_number);
			break;
		case O_TIMES:
			MCMathEvalMultiply(ctxt, t_number, p_number, t_number);
			break;
		case O_DIV:
			MCMathEvalDiv(ctxt, t_number, p_number, t_number);
			break;
		case O_MOD:
			MCMathEvalMod(ctxt, t_number, p_number, t_number);
			break;
		case O_WRAP:
			MCMathEvalWrap(ctxt, t_number, p_number, t_number);
			break;
		case O_OVER:
			MCMathEvalOver(ctxt, t_number, p_number, t_number);
			break;
		default:
			MCUnreachable();
			break;
		}
		if (ctxt.HasError())
			return;

		MCAutoNumberRef t_numberref;
		if (!MCNumberCreateWithReal(t_number, &t_numberref) ||
			!MCArrayStoreValue(*t_array, ctxt.GetCaseSensitive(), t_key, *t_numberref))
		{
			ctxt.Throw();
			return;
		}
	}

	if (!MCArrayCopy(*t_array, r_result))
		ctxt.Throw();
}


void MCMathArrayApplyOperationWithArray(MCExecContext& ctxt,
										MCArrayRef p_left,
										Operators p_op,
										MCArrayRef p_right,
										Exec_errors p_error,
										MCArrayRef& r_result)
{
	MCAutoArrayRef t_array;
	if (MCArrayGetCount(p_left) == 0)
	{
		r_result = MCValueRetain(kMCEmptyArray);
		return;
	}

	if (!MCArrayMutableCopy(p_left, &t_array))
	{
		ctxt.LegacyThrow(p_error);
		return;
	}

	MCNameRef t_key;
	MCValueRef t_value_left, t_value_right;
	uintptr_t t_index = 0;

	MCS_seterrno(0);
	while (MCArrayIterate(p_right, t_index, t_key, t_value_right))
	{
		real64_t t_double_left, t_double_right;
		if (!MCArrayFetchValue(p_left, ctxt.GetCaseSensitive(), t_key, t_value_left) ||
			!ctxt.ConvertToReal(t_value_left, t_double_left) ||
			!ctxt.ConvertToReal(t_value_right, t_double_right))
		{
			ctxt.LegacyThrow(p_error);
			return;
		}

		switch (p_op)
		{
		case O_PLUS:
			MCMathEvalAdd(ctxt, t_double_left, t_double_right, t_double_left);
			break;
		case O_MINUS:
			MCMathEvalSubtract(ctxt, t_double_left, t_double_right, t_double_left);
			break;
		case O_TIMES:
			MCMathEvalMultiply(ctxt, t_double_left, t_double_right, t_double_left);
			break;
		case O_DIV:
			MCMathEvalDiv(ctxt, t_double_left, t_double_right, t_double_left);
			break;
		case O_MOD:
			MCMathEvalMod(ctxt, t_double_left, t_double_right, t_double_left);
			break;
		case O_WRAP:
			MCMathEvalWrap(ctxt, t_double_left, t_double_right, t_double_left);
			break;
		case O_OVER:
			MCMathEvalOver(ctxt, t_double_left, t_double_right, t_double_left);
			break;
		default:
			MCUnreachable();
			break;
		}
		if (ctxt.HasError())
			return;

		MCAutoNumberRef t_number;
		if (!MCNumberCreateWithReal(t_double_left, &t_number) ||
			!MCArrayStoreValue(*t_array, ctxt.GetCaseSensitive(), t_key, *t_number))
		{
			ctxt.LegacyThrow(p_error);
			return;
		}
	}

	if (!MCArrayCopy(*t_array, r_result))
		ctxt.LegacyThrow(p_error);
}

//////////

void MCMathEvalDivArrayByNumber(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithNumber(ctxt, p_array, O_DIV, p_number, r_result);
}

void MCMathEvalDivArrayByArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithArray(ctxt, p_left, O_DIV, p_right, EE_DIVIDE_BADARRAY, r_result);
}

//////////

void MCMathEvalSubtractNumberFromArray(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithNumber(ctxt, p_array, O_MINUS, p_number, r_result);
}

void MCMathEvalSubtractArrayFromArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithArray(ctxt, p_left, O_MINUS, p_right, EE_MINUS_BADARRAY, r_result);
}

//////////

void MCMathEvalModArrayByNumber(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithNumber(ctxt, p_array, O_MOD, p_number, r_result);
}

void MCMathEvalModArrayByArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithArray(ctxt, p_left, O_MOD, p_right, EE_TIMES_BADARRAY, r_result);
}

//////////

void MCMathEvalWrapArrayByNumber(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithNumber(ctxt, p_array, O_WRAP, p_number, r_result);
}

void MCMathEvalWrapArrayByArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithArray(ctxt, p_left, O_WRAP, p_right, EE_WRAP_BADARRAY, r_result);
}

//////////

void MCMathEvalOverArrayByNumber(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithNumber(ctxt, p_array, O_OVER, p_number, r_result);
}

void MCMathEvalOverArrayByArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithArray(ctxt, p_left, O_OVER, p_right, EE_OVER_BADARRAY, r_result);
}

//////////

void MCMathEvalAddNumberToArray(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithNumber(ctxt, p_array, O_PLUS, p_number, r_result);
}

void MCMathEvalAddArrayToArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithArray(ctxt, p_left, O_PLUS, p_right, EE_ADD_BADARRAY, r_result);
}

//////////

void MCMathEvalMultiplyArrayByNumber(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithNumber(ctxt, p_array, O_TIMES, p_number, r_result);
}

void MCMathEvalMultiplyArrayByArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result)
{
	MCMathArrayApplyOperationWithArray(ctxt, p_left, O_TIMES, p_right, EE_MULTIPLY_BADARRAY, r_result);
}

////////////////////////////////////////////////////////////////////////////////

void MCMathEvalIsAnInteger(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCAutoNumberRef t_number;
	if (!MCValueIsEmpty(p_value) && ctxt.ConvertToNumber(p_value, &t_number))
	{
		if (MCNumberIsInteger(*t_number))
			r_result = true;
		else
		{
			real64_t t_double;
			t_double = MCNumberFetchAsReal(*t_number);
			r_result = t_double == floor(t_double);
		}
	}
	else
		r_result = false;
}

void MCMathEvalIsNotAnInteger(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCMathEvalIsAnInteger(ctxt, p_value, r_result);
	r_result = !r_result;
}

void MCMathEvalIsANumber(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCAutoNumberRef t_number;
	r_result = !MCValueIsEmpty(p_value) && ctxt.ConvertToNumber(p_value, &t_number);
}

void MCMathEvalIsNotANumber(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCMathEvalIsANumber(ctxt, p_value, r_result);
	r_result = !r_result;
}

////////////////////////////////////////////////////////////////////////////////

void MCMathGetRandomSeed(MCExecContext& ctxt, integer_t& r_value)
{
	r_value = MCrandomseed;
}

void MCMathSetRandomSeed(MCExecContext& ctxt, integer_t p_value)
{
	MCrandomseed = p_value;
	MCU_srand();
}

////////////////////////////////////////////////////////////////////////////////

static real64_t MCMathEvaluateStatsFunction(Functions p_func, real64_t *p_values, uindex_t p_count)
{
    if ((p_func == F_SMP_STD_DEV || p_func == F_SMP_VARIANCE)
        && p_count == 1)
    {
        return 0;
    }
    
    real64_t t_mean = mc_arithmetic_mean(p_values, p_count);
    
    real64_t t_result = 0.0;
    for (uindex_t i = 0; i < p_count; i++)
    {
        real64_t t_value = p_values[i] - t_mean;
        switch (p_func)
        {
                // JS-2013-06-19: [[ StatsFunctions ]] Support for 'averageDeviation'
            case F_AVG_DEV:
                // IM-2014-02-28: [[ Bug 11778 ]] Make sure we're using the floating-point version of 'abs'
                t_result += fabs(t_value);
                break;
                // JS-2013-06-19: [[ StatsFunctions ]] Support for 'populationStdDev', 'populationVariance', 'sampleStdDev' (was stdDev), 'sampleVariance'
            case F_POP_STD_DEV:
            case F_POP_VARIANCE:
            case F_SMP_STD_DEV:
            case F_SMP_VARIANCE:
                t_result += t_value * t_value;
                break;
            default:
                MCUnreachableReturn(0.0);
                break;
        }
    }
    
    switch (p_func)
    {
            // JS-2013-06-19: [[ StatsFunctions ]] Support for averageDeviation
        case F_AVG_DEV:
            t_result /= p_count;
            break;
            // JS-2013-06-19: [[ StatsFunctions ]] Support for populationStandardDeviation
        case F_POP_STD_DEV:
            t_result = sqrt(t_result/p_count);
            break;
            // JS-2013-06-19: [[ StatsFunctions ]] Support for populationVariance
        case F_POP_VARIANCE:
            t_result /= p_count;
            break;
            // JS-2013-06-19: [[ StatsFunctions ]] Support for sampleStandardDeviation (was stdDev)
        case F_SMP_STD_DEV:
            t_result = sqrt(t_result/(p_count - 1));
            break;
            // JS-2013-06-19: [[ StatsFunctions ]] Support for sampleVariance
        case F_SMP_VARIANCE:
            t_result /= p_count - 1;
            break;
        default:
            MCUnreachableReturn(0.0);
            break;
    }
    return t_result;
}

template <Functions Function>
void MCMathEvalCheckedStatsFunction(MCExecContext& ctxt, real64_t* p_values, uindex_t p_count, real64_t& r_result)
{
    if (p_count == 0)
    {
        r_result = 0.0;
        return;
    }
    
    real64_t t_result = MCMathEvaluateStatsFunction(Function, p_values, p_count);
    if (!MCMathCheckNaryResult(ctxt, p_values, p_count, t_result))
    {
        MCMathThrowFloatingPointException(ctxt, t_result);
        return;
    }
    r_result = t_result;
}

// IM-2012-08-16: Note: this computes the sample standard deviation rather than
// the population standard deviation, hence the division by n - 1 rather than n.
void MCMathEvalSampleStdDev(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvalCheckedStatsFunction<F_SMP_STD_DEV>(ctxt, p_values, p_count, r_result);
}

void MCMathEvalAverageDeviation(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvalCheckedStatsFunction<F_AVG_DEV>(ctxt, p_values, p_count, r_result);
}

void MCMathEvalPopulationStdDev(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvalCheckedStatsFunction<F_POP_STD_DEV>(ctxt, p_values, p_count, r_result);
}

void MCMathEvalPopulationVariance(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvalCheckedStatsFunction<F_POP_VARIANCE>(ctxt, p_values, p_count, r_result);
}

void MCMathEvalSampleVariance(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvalCheckedStatsFunction<F_SMP_VARIANCE>(ctxt, p_values, p_count, r_result);
}
