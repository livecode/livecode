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

void MCMathEvalAcos(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	if (p_in < -1.0 || p_in > 1.0)
	{
		ctxt.LegacyThrow(EE_ACOS_DOMAIN);
		return;
	}

	r_result = acos(p_in);
}

void MCMathEvalAsin(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	if (p_in < -1.0 || p_in > 1.0)
	{
		ctxt.LegacyThrow(EE_ASIN_DOMAIN);
		return;
	}

	r_result = asin(p_in);
}

void MCMathEvalAtan(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	r_result = atan(p_in);
}

void MCMathEvalAtan2(MCExecContext& ctxt, real64_t p_y, real64_t p_x, real64_t& r_result)
{
	r_result = atan2(p_y, p_x);
}

void MCMathEvalCos(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	r_result = cos(p_in);
}

void MCMathEvalSin(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	r_result = sin(p_in);
}

void MCMathEvalTan(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	r_result = tan(p_in);
}

//////////

void MCMathEvalExp(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	MCS_seterrno(0);
	r_result = exp(p_in);
	if (MCS_geterrno() != 0 || MCS_isnan(r_result))
	{
		ctxt.LegacyThrow(EE_EXP_DOMAIN);
		MCS_seterrno(0);
	}
}

void MCMathEvalExp1(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	MCS_seterrno(0);
	r_result = exp(p_in) - 1.0;
	if (MCS_geterrno() != 0 || MCS_isnan(r_result))
	{
		ctxt.LegacyThrow(EE_EXP1_DOMAIN);
		MCS_seterrno(0);
	}
}

void MCMathEvalExp2(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	MCS_seterrno(0);
	r_result = pow(2.0, p_in);
	if (MCS_geterrno() != 0 || MCS_isnan(r_result))
	{
		ctxt.LegacyThrow(EE_EXP2_DOMAIN);
		MCS_seterrno(0);
	}
}

void MCMathEvalExp10(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	MCS_seterrno(0);
	r_result = pow(10.0, p_in);
	if (MCS_geterrno() != 0 || MCS_isnan(r_result))
	{
		ctxt.LegacyThrow(EE_EXP10_DOMAIN);
		MCS_seterrno(0);
	}
}

//////////

void MCMathEvalLn(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	MCS_seterrno(0);
	r_result = log(p_in);
	if (MCS_geterrno() != 0 || MCS_isnan(r_result))
	{
		ctxt.LegacyThrow(EE_LN_DOMAIN);
		MCS_seterrno(0);
	}
}

void MCMathEvalLn1(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	MCS_seterrno(0);
	r_result = log(p_in + 1.0);
	if (MCS_geterrno() != 0 || MCS_isnan(r_result))
	{
		ctxt.LegacyThrow(EE_LN1_DOMAIN);
		MCS_seterrno(0);
	}
}

void MCMathEvalLog2(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	MCS_seterrno(0);
	r_result = log(p_in) / log(2.0);
	if (MCS_geterrno() != 0 || MCS_isnan(r_result))
	{
		ctxt.LegacyThrow(EE_LOG2_DOMAIN);
		MCS_seterrno(0);
	}
}

void MCMathEvalLog10(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	MCS_seterrno(0);
	r_result = log10(p_in);
	if (MCS_geterrno() != 0 || MCS_isnan(r_result))
	{
		ctxt.LegacyThrow(EE_LOG10_DOMAIN);
		MCS_seterrno(0);
	}
}

//////////

void MCMathEvalSqrt(MCExecContext& ctxt, real64_t p_in, real64_t& r_result)
{
	MCS_seterrno(0);
	r_result = sqrt(p_in);
	if (MCS_geterrno() != 0 || MCS_isnan(r_result))
	{
		ctxt.LegacyThrow(EE_SQRT_DOMAIN);
		MCS_seterrno(0);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCMathEvalAnnuity(MCExecContext& ctxt, real64_t p_rate, real64_t p_periods, real64_t& r_result)
{
	r_result = (1.0 - pow(1.0 + p_rate, -p_periods)) / p_rate;
}

void MCMathEvalCompound(MCExecContext& ctxt, real64_t p_rate, real64_t p_periods, real64_t& r_result)
{
	r_result = pow(1.0 + p_rate, p_periods);
}

////////////////////////////////////////////////////////////////////////////////

void MCMathEvalArithmeticMean(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
	if (p_count == 0)
	{
		r_result = 0;
		return;
	}

	real64_t t_total = 0.0;
	for (uindex_t i = 0; i < p_count; i++)
		t_total += p_values[i];

	r_result = t_total / p_count;
}

int cmp_real64_t(const void* a, const void* b)
{
	real64_t t_a, t_b;
	t_a = *(real64_t*)a;
	t_b = *(real64_t*)b;
	return (t_a > t_b) - (t_a < t_b);
}

void MCMathEvalMedian(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
	if (p_count == 0)
	{
		r_result = 0;
		return;
	}

	qsort((void*)p_values, p_count, sizeof(real64_t), cmp_real64_t);
	uindex_t t_index = p_count / 2;
	if (p_count & 1) // odd
		r_result = p_values[t_index];
	else // even, return avg of two nearest-to-centre values
		r_result = (p_values[t_index - 1] + p_values[t_index]) / 2.0;
}

void MCMathEvalMin(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
	if (p_count == 0)
	{
		r_result = 0.0;
		return;
	}

	r_result = p_values[0];
	for (uindex_t i = 1; i < p_count; i++)
	{
		if (p_values[i] < r_result)
			r_result = p_values[i];
	}
}

void MCMathEvalMax(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
	if (p_count == 0)
	{
		r_result = 0.0;
		return;
	}

	r_result = p_values[0];
	for (uindex_t i = 1; i < p_count; i++)
	{
		if (p_values[i] > r_result)
			r_result = p_values[i];
	}
}

// IM-2012-08-16: Note: this computes the sample standard deviation rather than
// the population standard deviation, hence the division by n - 1 rather than n.
void MCMathEvalSampleStdDev(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvaluateStatsFunction(ctxt, F_SMP_STD_DEV, p_values, p_count, r_result);
}

void MCMathEvalSum(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
	real64_t t_total = 0.0;
	for (uindex_t i = 0; i < p_count; i++)
		t_total += p_values[i];

	r_result = t_total;
}

void MCMathEvalAverageDeviation(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvaluateStatsFunction(ctxt, F_AVG_DEV, p_values, p_count, r_result);
}

void MCMathEvalGeometricMean(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvaluateStatsFunction(ctxt, F_GEO_MEAN, p_values, p_count, r_result);
}

void MCMathEvalHarmonicMean(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvaluateStatsFunction(ctxt, F_HAR_MEAN, p_values, p_count, r_result);
}

void MCMathEvalPopulationStdDev(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvaluateStatsFunction(ctxt, F_POP_STD_DEV, p_values, p_count, r_result);
}

void MCMathEvalPopulationVariance(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvaluateStatsFunction(ctxt, F_POP_VARIANCE, p_values, p_count, r_result);
}

void MCMathEvalSampleVariance(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
    MCMathEvaluateStatsFunction(ctxt, F_SMP_VARIANCE, p_values, p_count, r_result);
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

void MCMathEvalDiv(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
	MCS_seterrno(0);
	if (p_right == 0.0)
	{
		ctxt.LegacyThrow(EE_DIV_ZERO);
		return;
	}

	r_result = p_left / p_right;

	if (r_result == MCinfinity || MCS_geterrno() != 0)
	{
		MCS_seterrno(0);
		ctxt.LegacyThrow(EE_DIV_RANGE);
		return;
	}

	if (r_result < 0.0)
		r_result = ceil(r_result);
	else
		r_result = floor(r_result);
}

void MCMathEvalSubtract(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
	MCS_seterrno(0);
	r_result = p_left - p_right;

	if (MCS_geterrno() != 0)
	{
		MCS_seterrno(0);
		ctxt.LegacyThrow(EE_MINUS_RANGE);
		return;
	}
}

void MCMathEvalMod(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
	if (p_right == 0.0)
	{
		ctxt.LegacyThrow(EE_MOD_ZERO);
		return;
	}

	MCS_seterrno(0);
	r_result = p_left / p_right;

	if (MCS_geterrno() != 0)
	{
		MCS_seterrno(0);
		ctxt.LegacyThrow(EE_MOD_RANGE);
		return;
	}

	r_result = fmod(p_left, p_right);
}

void MCMathEvalWrap(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
	if (p_right == 0.0)
	{
		ctxt.LegacyThrow(EE_WRAP_ZERO);
		return;
	}

	MCS_seterrno(0);
	r_result = p_left / p_right;

	if (MCS_geterrno() != 0)
	{
		MCS_seterrno(0);
		ctxt.LegacyThrow(EE_WRAP_RANGE);
		return;
	}

	r_result = MCU_fwrap(p_left, p_right);
}

void MCMathEvalOver(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
	if (p_right == 0.0)
	{
		ctxt.LegacyThrow(EE_OVER_ZERO);
		return;
	}

	MCS_seterrno(0);
	r_result = p_left / p_right;

	if (MCS_geterrno() != 0)
	{
		MCS_seterrno(0);
		ctxt.LegacyThrow(EE_OVER_RANGE);
		return;
	}
}

void MCMathEvalAdd(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
	MCS_seterrno(0);
	r_result = p_left + p_right;

	if (MCS_geterrno() != 0)
	{
		MCS_seterrno(0);
		ctxt.LegacyThrow(EE_PLUS_RANGE);
		return;
	}
}

void MCMathEvalMultiply(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
	MCS_seterrno(0);
	r_result = p_left * p_right;

	if (r_result == MCinfinity || MCS_geterrno() != 0)
	{
		MCS_seterrno(0);
		ctxt.LegacyThrow(EE_PLUS_RANGE);
		return;
	}
}

void MCMathEvalPower(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result)
{
	MCS_seterrno(0);
	r_result = pow(p_left, p_right);

	if (r_result == MCinfinity || MCS_geterrno() != 0)
	{
		MCS_seterrno(0);
		ctxt.LegacyThrow(EE_POW_RANGE);
		return;
	}
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

void MCMathEvaluateStatsFunction(MCExecContext& ctxt, Functions p_func, real64_t *p_values, uindex_t p_count, real64_t& r_result)
{
	r_result = 0.0;
	if (p_count == 0)
	{
		return;
	}
    
    real64_t t_mean;
	MCMathEvalArithmeticMean(ctxt, p_values, p_count, t_mean);
	if (ctxt.HasError())
		return;
    
    // TODO - move action of MCU_dofunc here
    real64_t t_result = 0.0;
    for (uindex_t i = 0; i < p_count; i++)
        MCU_dofunc(p_func, i, t_result, p_values[i], p_func == F_GEO_MEAN ? p_count : t_mean, nil);
    
    switch (p_func)
	{
            // JS-2013-06-19: [[ StatsFunctions ]] Support for averageDeviation
		case F_AVG_DEV:
			t_result /= p_count;
			break;
            // JS-2013-06-19: [[ StatsFunctions ]] Support for harmonicMean
		case F_HAR_MEAN:
			t_result = p_count/t_result;
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
			break;
    }
    r_result = t_result;
}
