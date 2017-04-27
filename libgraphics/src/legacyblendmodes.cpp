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

#include "graphics.h"
#include "graphics-internal.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef __VISUALC__
#pragma optimize("agt", on)
#pragma optimize("y", off)
#define INLINE __forceinline
#else
#define INLINE inline
#endif

////////////////////////////////////////////////////////////////////////////////

enum BitwiseOperation
{
	// Bitwise
	OPERATION_CLEAR,
	OPERATION_AND,
	OPERATION_AND_REVERSE,
	OPERATION_COPY,
	OPERATION_AND_INVERTED,
	OPERATION_NOOP,
	OPERATION_XOR,
	OPERATION_OR,
	OPERATION_NOR,
	OPERATION_EQUIV,
	OPERATION_INVERT,
	OPERATION_OR_REVERSE,
	OPERATION_COPY_INVERTED,
	OPERATION_OR_INVERTED,
	OPERATION_NAND,
	OPERATION_SET,
    
    LAST_BITWISE_OPERATION = OPERATION_SET,
};

enum ArithmeticOperation
{
	// Arithmetic
	OPERATION_BLEND = LAST_BITWISE_OPERATION + 1,
	OPERATION_ADD_PIN,
	OPERATION_ADD_OVER,
	OPERATION_SUB_PIN,
	OPERATION_TRANSPARENT,
	OPERATION_AD_MAX,
	OPERATION_SUB_OVER,
	OPERATION_AD_MIN,
    
    LAST_ARITHMETIC_OPERATION = OPERATION_AD_MIN,
};

enum BasicImagingOperation
{
	// Basic Imaging Blends
	OPERATION_BLEND_CLEAR = LAST_ARITHMETIC_OPERATION + 1,
	OPERATION_BLEND_SRC,
	OPERATION_BLEND_DST,
	OPERATION_BLEND_SRC_OVER,
	OPERATION_BLEND_DST_OVER,
	OPERATION_BLEND_SRC_IN,
	OPERATION_BLEND_DST_IN,
	OPERATION_BLEND_SRC_OUT,
	OPERATION_BLEND_DST_OUT,
	OPERATION_BLEND_SRC_ATOP,
	OPERATION_BLEND_DST_ATOP,
	OPERATION_BLEND_XOR,
	OPERATION_BLEND_PLUS,
	OPERATION_BLEND_MULTIPLY,
	OPERATION_BLEND_SCREEN,
    
    LAST_BASIC_IMAGING_OPERATION = OPERATION_BLEND_SCREEN,
};

enum AdvancedImagingOperation
{
	// Advanced Imaging Blends
	OPERATION_BLEND_OVERLAY = LAST_BASIC_IMAGING_OPERATION + 1,
	OPERATION_BLEND_DARKEN,
	OPERATION_BLEND_LIGHTEN,
	OPERATION_BLEND_DODGE,
	OPERATION_BLEND_BURN,
	OPERATION_BLEND_HARD_LIGHT,
	OPERATION_BLEND_SOFT_LIGHT,
	OPERATION_BLEND_DIFFERENCE,
	OPERATION_BLEND_EXCLUSION,
    
    LAST_ADVANCED_IMAGING_OPERATION = OPERATION_BLEND_EXCLUSION,
};

#ifdef __VISUALC__
static uint16_t s_sqrt_table[1024] =
{
    0,
    2048,2896,3547,4096,4579,5017,5418,5793,6144,6476,6792,7094,7384,7663,7932,8192,8444,
    8689,8927,9159,9385,9606,9822,10033,10240,10443,10642,10837,11029,11217,11403,11585,
    11765,11942,12116,12288,12457,12625,12790,12953,13114,13273,13430,13585,13738,13890,
    14040,14189,14336,14482,14626,14768,14910,15050,15188,15326,15462,15597,15731,15864,
    15995,16126,16255,16384,16512,16638,16764,16888,17012,17135,17257,17378,17498,17618,
    17736,17854,17971,18087,18203,18318,18432,18545,18658,18770,18882,18992,19102,19212,
    19321,19429,19537,19644,19750,19856,19961,20066,20170,20274,20377,20480,20582,20684,
    20785,20886,20986,21085,21185,21283,21382,21480,21577,21674,21771,21867,21962,22058,
    22153,22247,22341,22435,22528,22621,22713,22806,22897,22989,23080,23170,23261,23351,
    23440,23530,23619,23707,23796,23884,23971,24059,24146,24232,24319,24405,24491,24576,
    24661,24746,24831,24915,24999,25083,25166,25249,25332,25415,25497,25580,25661,25743,
    25824,25905,25986,26067,26147,26227,26307,26387,26466,26545,26624,26703,26781,26859,
    26937,27015,27092,27170,27247,27324,27400,27477,27553,27629,27705,27780,27856,27931,
    28006,28081,28155,28230,28304,28378,28452,28525,28599,28672,28745,28818,28891,28963,
    29035,29108,29180,29251,29323,29394,29466,29537,29608,29678,29749,29819,29890,29960,
    30030,30099,30169,30238,30308,30377,30446,30515,30583,30652,30720,30788,30856,30924,
    30992,31059,31127,31194,31261,31328,31395,31462,31529,31595,31661,31727,31794,31859,
    31925,31991,32056,32122,32187,32252,32317,32382,32446,32511,32575,32640,32704,32768,
    32832,32896,32959,33023,33086,33150,33213,33276,33339,33402,33465,33527,33590,33652,
    33714,33776,33839,33900,33962,34024,34086,34147,34208,34270,34331,34392,34453,34514,
    34574,34635,34695,34756,34816,34876,34936,34996,35056,35116,35176,35235,35295,35354,
    35413,35472,35531,35590,35649,35708,35767,35825,35884,35942,36001,36059,36117,36175,
    36233,36291,36348,36406,36464,36521,36578,36636,36693,36750,36807,36864,36921,36978,
    37034,37091,37147,37204,37260,37316,37372,37429,37485,37540,37596,37652,37708,37763,
    37819,37874,37929,37985,38040,38095,38150,38205,38260,38315,38369,38424,38478,38533,
    38587,38642,38696,38750,38804,38858,38912,38966,39020,39073,39127,39181,39234,39287,
    39341,39394,39447,39500,39553,39606,39659,39712,39765,39818,39870,39923,39975,40028,
    40080,40132,40185,40237,40289,40341,40393,40445,40497,40548,40600,40652,40703,40755,
    40806,40857,40909,40960,41011,41062,41113,41164,41215,41266,41317,41368,41418,41469,
    41519,41570,41620,41671,41721,41771,41821,41871,41922,41972,42021,42071,42121,42171,
    42221,42270,42320,42369,42419,42468,42518,42567,42616,42665,42714,42763,42813,42861,
    42910,42959,43008,43057,43105,43154,43203,43251,43300,43348,43396,43445,43493,43541,
    43589,43637,43685,43733,43781,43829,43877,43925,43972,44020,44068,44115,44163,44210,
    44258,44305,44352,44400,44447,44494,44541,44588,44635,44682,44729,44776,44823,44869,
    44916,44963,45009,45056,45103,45149,45195,45242,45288,45334,45381,45427,45473,45519,
    45565,45611,45657,45703,45749,45795,45840,45886,45932,45977,46023,46069,46114,46160,
    46205,46250,46296,46341,46386,46431,46477,46522,46567,46612,46657,46702,46746,46791,
    46836,46881,46926,46970,47015,47059,47104,47149,47193,47237,47282,47326,47370,47415,
    47459,47503,47547,47591,47635,47679,47723,47767,47811,47855,47899,47942,47986,48030,
    48074,48117,48161,48204,48248,48291,48335,48378,48421,48465,48508,48551,48594,48637,
    48680,48723,48766,48809,48852,48895,48938,48981,49024,49067,49109,49152,49195,49237,
    49280,49322,49365,49407,49450,49492,49535,49577,49619,49661,49704,49746,49788,49830,
    49872,49914,49956,49998,50040,50082,50124,50166,50207,50249,50291,50332,50374,50416,
    50457,50499,50540,50582,50623,50665,50706,50747,50789,50830,50871,50912,50954,50995,
    51036,51077,51118,51159,51200,51241,51282,51323,51364,51404,51445,51486,51527,51567,
    51608,51649,51689,51730,51770,51811,51851,51892,51932,51972,52013,52053,52093,52134,
    52174,52214,52254,52294,52334,52374,52414,52454,52494,52534,52574,52614,52654,52694,
    52734,52773,52813,52853,52892,52932,52972,53011,53051,53090,53130,53169,53209,53248,
    53287,53327,53366,53405,53445,53484,53523,53562,53601,53640,53679,53719,53758,53797,
    53836,53874,53913,53952,53991,54030,54069,54108,54146,54185,54224,54262,54301,54340,
    54378,54417,54455,54494,54532,54571,54609,54647,54686,54724,54762,54801,54839,54877,
    54915,54954,54992,55030,55068,55106,55144,55182,55220,55258,55296,55334,55372,55410,
    55447,55485,55523,55561,55599,55636,55674,55712,55749,55787,55824,55862,55900,55937,
    55975,56012,56049,56087,56124,56162,56199,56236,56273,56311,56348,56385,56422,56459,
    56497,56534,56571,56608,56645,56682,56719,56756,56793,56830,56867,56903,56940,56977,
    57014,57051,57087,57124,57161,57198,57234,57271,57307,57344,57381,57417,57454,57490,
    57527,57563,57599,57636,57672,57709,57745,57781,57817,57854,57890,57926,57962,57999,
    58035,58071,58107,58143,58179,58215,58251,58287,58323,58359,58395,58431,58467,58503,
    58538,58574,58610,58646,58682,58717,58753,58789,58824,58860,58896,58931,58967,59002,
    59038,59073,59109,59144,59180,59215,59251,59286,59321,59357,59392,59427,59463,59498,
    59533,59568,59603,59639,59674,59709,59744,59779,59814,59849,59884,59919,59954,59989,
    60024,60059,60094,60129,60164,60199,60233,60268,60303,60338,60373,60407,60442,60477,
    60511,60546,60581,60615,60650,60684,60719,60753,60788,60822,60857,60891,60926,60960,
    60995,61029,61063,61098,61132,61166,61201,61235,61269,61303,61338,61372,61406,61440,
    61474,61508,61542,61576,61610,61644,61678,61712,61746,61780,61814,61848,61882,61916,
    61950,61984,62018,62051,62085,62119,62153,62186,62220,62254,62287,62321,62355,62388,
    62422,62456,62489,62523,62556,62590,62623,62657,62690,62724,62757,62790,62824,62857,
    62891,62924,62957,62991,63024,63057,63090,63124,63157,63190,63223,63256,63289,63323,
    63356,63389,63422,63455,63488,63521,63554,63587,63620,63653,63686,63719,63752,63785,
    63817,63850,63883,63916,63949,63982,64014,64047,64080,64113,64145,64178,64211,64243,
    64276,64309,64341,64374,64406,64439,64471,64504,64536,64569,64601,64634,64666,64699,
    64731,64763,64796,64828,64861,64893,64925,64957,64990,65022,65054,65086,65119,65151,
    65183,65215,65247,65279,65312,65344,65376,65408,65440,65472,65504
};
#endif
////////////////////////////////////////////////////////////////////////////////

static INLINE uint32_t _combine(uint32_t u, uint32_t v)
{
	u += 0x800080;
	v += 0x800080;
	return (((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff) + (((v + ((v >> 8) & 0xff00ff))) & 0xff00ff00);
}

static INLINE uint32_t _low(uint32_t x)
{
	return x & 0xff00ff;
}

static INLINE uint32_t _high(uint32_t x)
{
	return (x >> 8) & 0xff00ff;
}

static INLINE uint32_t _scale(uint32_t x, uint8_t a)
{
	return x * a;
}

static INLINE uint32_t _scale_low(uint32_t x, uint8_t a)
{
	return _scale(_low(x), a);
}

static INLINE uint32_t _scale_high(uint32_t x, uint32_t a)
{
	return _scale(_high(x), a);
}

static INLINE uint32_t _multiply_low(uint32_t x, uint32_t y)
{
	return ((x & 0xff) * (y & 0xff)) | ((x & 0xff0000) * ((y >> 16) & 0xff));
}

static INLINE uint32_t _multiply_high(uint32_t x, uint32_t y)
{
	x = x >> 8;
	return ((x & 0xff) * ((y >> 8) & 0xff)) | ((x & 0xff0000) * (y >> 24));
}

// r_i = (x_i * a) / 255
static INLINE uint32_t packed_scale_bounded(uint32_t x, uint8_t a)
{
	uint32_t u, v;
	
	u = ((x & 0xff00ff) * a) + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
	
	v = (((x >> 8) & 0xff00ff) * a) + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
	
	return u + v;
}

// r_i = x_i + a
static INLINE uint32_t packed_translate_bounded(uint32_t x, uint8_t a)
{
	uint32_t y;
	y = a | a << 8;
	y |= y << 16;
	return x + y;
}

// r_i = x_i + y_i
static INLINE uint32_t packed_add_bounded(uint32_t x, uint32_t y)
{
	return x + y;
}

// r_i = x_i * y_i / 255;
static INLINE uint32_t packed_multiply_bounded(uint32_t x, uint32_t y)
{
	return _combine(_multiply_low(x, y), _multiply_high(x, y));
}

// r_i = (x_i * a) / 255 + y_i
static INLINE uint32_t packed_scale_add_bounded(uint32_t x, uint8_t a, uint32_t y)
{
	return packed_add_bounded(packed_scale_bounded(x, a), y);
}

// r_i = (x_i * a + y_i * b) / 255
static INLINE uint32_t packed_bilinear_bounded(uint32_t x, uint8_t a, uint32_t y, uint8_t b)
{
	uint32_t u, v;
	
	u = (x & 0xff00ff) * a + (y & 0xff00ff) * b + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
	
	v = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
	
	return u | v;
}

static INLINE uint32_t packed_add_saturated(uint32_t x, uint32_t y)
{
	uint32_t u, v;
	
	u = _low(x) + _low(y);
	u = (u | (0x10000100 - ((u >> 8) & 0xff00ff))) & 0xff00ff;
	
	v = _high(x) + _high(y);
	v = (v | (0x10000100 - ((v >> 8) & 0xff00ff))) & 0xff00ff;
	
	return u | (v << 8);
}

static INLINE uint32_t packed_subtract_saturated(uint32_t x, uint32_t y)
{
	uint32_t u, v;
	
	u = _low(x) - _low(y);
	u = (u & ~((u >> 8) & 0xff00ff)) & 0xff00ff;
	
	v = _high(x) - _high(y);
	v = (v & ~((v >> 8) & 0xff00ff)) & 0xff00ff;
	
	return u | (v << 8);
}

static INLINE uint32_t packed_divide_bounded(uint32_t x, uint8_t a)
{
	uint32_t u, v, w;
	u = ((((x & 0xff0000) << 8) - (x & 0xff0000)) / a) & 0xff0000;
	v = ((((x & 0x00ff00) << 8) - (x & 0x00ff00)) / a) & 0x00ff00;
	w = ((((x & 0x0000ff) << 8) - (x & 0x0000ff)) / a) & 0x0000ff;
	return u | v | w;
}

static INLINE uint32_t packed_inverse(uint32_t x)
{
	return ~x;
}

static INLINE uint8_t packed_alpha(uint32_t x)
{
	return x >> 24;
}

static INLINE uint8_t packed_inverse_alpha(uint32_t x)
{
	return (~x) >> 24;
}

static INLINE uint8_t saturated_add(uint8_t a, uint8_t b)
{
	if (a + b > 255)
		return 255;
	
	return a + b;
}

static INLINE uint8_t inverse(uint8_t a)
{
	return 255 - a;
}

static INLINE uint16_t upscale(uint8_t a)
{
	return (a << 8) - a;
}

static INLINE uint32_t upscale(uint16_t a)
{
	return (a << 8) - a;
}

static INLINE uint8_t downscale(uint16_t a)
{
	uint32_t r = ((a + 0x80) + ((a + 0x80) >> 8)) >> 8;
	return uint8_t(r);
}

static INLINE uint16_t scaled_divide(uint16_t n, uint8_t s, uint8_t d)
{
	return d != 0 ? uint16_t((n * s) / d) : 0;
}

static INLINE uint16_t long_scaled_divide(uint32_t n, uint8_t s, uint16_t d)
{
	return d != 0 ? uint16_t((n * s) / d) : 0;
}

static INLINE uint8_t sqrt(uint16_t n)
{
	return 1;
}

template<typename Type> INLINE Type fastmin(Type a, Type b)
{
	return a > b ? b : a;
}

template<typename Type> INLINE Type fastmax(Type a, Type b)
{
	return a < b ? b : a;
}

////////////////////////////////////////////////////////////////////////////////

template<BitwiseOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t bitwise_combiner(uint32_t dst, uint32_t src)
{
	uint8_t sa, da;
	uint32_t r, s, d;
	
	if (x_dst_alpha)
		da = packed_alpha(dst);
	else
		da = 255;
	
	if (x_src_alpha)
		sa = packed_alpha(src);
	else
		sa = 255;
	
	if (sa == 0)
		return dst;
	
	if (da == 0)
		return src;
	
	if (sa != 255)
		s = packed_divide_bounded(src, sa);
	else
		s = src;
	
	if (da != 255)
		d = packed_divide_bounded(dst, da);
	else
		d = dst;
	
	switch(x_combiner)
	{
		case OPERATION_CLEAR:
			r = 0x00000000;
			break;
		case OPERATION_AND:
			r = s & d;
			break;
		case OPERATION_AND_REVERSE:
			r = (s & ~d) & 0xffffff;
			break;
		case OPERATION_COPY:
			r = s;
			break;
		case OPERATION_AND_INVERTED:
			r = (~s & d) & 0xffffff;
			break;
		case OPERATION_NOOP:
			r = d;
			break;
		case OPERATION_XOR:
			r = s ^ d;
			break;
		case OPERATION_OR:
			r = s | d;
			break;
		case OPERATION_NOR:
			r = (~(s | d)) & 0xffffff;
			break;
		case OPERATION_EQUIV:
			r = (~s ^ d) & 0xffffff;
			break;
		case OPERATION_INVERT:
			r = ~d & 0xffffff;
			break;
		case OPERATION_OR_REVERSE:
			r = (s | ~d) & 0xffffff;
			break;
		case OPERATION_COPY_INVERTED:
			r = (~s) & 0xffffff;
			break;
		case OPERATION_OR_INVERTED:
			r = (~s | d) & 0xffffff;
			break;
		case OPERATION_NAND:
			r = (~(s & d)) & 0xffffff;
			break;
		case OPERATION_SET:
			r = 0x00ffffff;
			break;
        default:
            MCUnreachableReturn(0);
	}
	
	if (x_src_alpha && x_dst_alpha)
	{
		uint8_t ra;
		ra = downscale(sa * da);
		r = packed_bilinear_bounded(src, 255 - da, dst, 255 - sa) + packed_scale_bounded(r | 0xff000000, ra);
	}
	else if (!x_src_alpha && x_dst_alpha)
		r = packed_bilinear_bounded(src, 255 - da, r | 0xff000000, da);
	else if (!x_dst_alpha && x_src_alpha)
		r = packed_bilinear_bounded(dst, 255 - sa, r | 0xff000000, sa);
	
	return r;
}

template<ArithmeticOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t arithmetic_combiner(uint32_t dst, uint32_t src)
{
	uint8_t sa, da;
	uint32_t s, d;
	
	if (x_dst_alpha)
		da = packed_alpha(dst);
	else
		da = 255;
	
	if (x_src_alpha)
		sa = packed_alpha(src);
	else
		sa = 255;
	
	if (sa == 0)
		return dst;
	
	if (da == 0)
		return src;
	
	if (sa != 255)
		s = packed_divide_bounded(src, sa);
	else
		s = src;
	
	if (da != 255)
		d = packed_divide_bounded(dst, da);
	else
		d = dst;
	
	uint32_t r;
	switch(x_combiner)
	{
		case OPERATION_BLEND:
		{
			uint32_t u, v;
			u = (((d & 0xff00ff) + (s & 0xff00ff)) >> 1) & 0xff00ff;
			v = (((d & 0x00ff00) + (s & 0x00ff00)) >> 1) & 0x00ff00;
			r = u | v;
		}
			break;
		case OPERATION_ADD_PIN:
		{
			uint32_t u, v;
			u = (d & 0xff00ff) + (s & 0xff00ff);
			u = (u | (0x1000100 - ((u >> 8) & 0xff00ff))) & 0xff00ff;
			v = (d & 0x00ff00) + (s & 0x00ff00);
			v = (v | (0x0010000 - ((v >> 8) & 0x00ff00))) & 0x00ff00;
			r = u | v;
		}
			break;
		case OPERATION_ADD_OVER:
		{
			uint32_t u, v;
			u = ((d & 0xff00ff) + (s & 0xff00ff)) & 0xff00ff;
			v = ((d & 0x00ff00) + (s & 0x00ff00)) & 0x00ff00;
			r = u | v;
		}
			break;
		case OPERATION_SUB_PIN:
		{
			uint32_t u, v;
			u = (s & 0xff00ff) - (d & 0xff00ff);
			u = (u & (((u >> 8) & 0xff00ff) - 0x100101)) & 0xff00ff;
			v = (s & 0x00ff00) - (d & 0x00ff00);
			v = (v & (((v >> 8) & 0x00ff00) - 0x0010100)) & 0x00ff00;
			r = u | v;
		}
			break;
		case OPERATION_TRANSPARENT:
			r = (s == 0) ? d : s;
			break;
		case OPERATION_AD_MAX:
		{
			uint32_t sr, sg, sb;
			sr = s & 0x0000ff; sg = s & 0x00ff00; sb = s & 0xff0000;
			
			uint32_t dr, dg, db;
			dr = d & 0x0000ff; dg = d & 0x00ff00; db = d & 0xff0000;
			
			uint32_t rr, rg, rb;
			rr = sr > dr ? sr : dr;
			rg = sg > dg ? sg : dg;
			rb = sb > db ? sb : db;
			
			r = rr | rg | rb;
		}
			break;
		case OPERATION_SUB_OVER:
		{
			uint32_t u, v;
			u = ((s & 0xff00ff) - (d & 0xff00ff)) & 0xff00ff;
			v = ((s & 0x00ff00) - (d & 0x00ff00)) & 0x00ff00;
			r = u | v;
		}
			break;
		case OPERATION_AD_MIN:
		{
			uint32_t sr, sg, sb;
			sr = s & 0x0000ff; sg = s & 0x00ff00; sb = s & 0xff0000;
			
			uint32_t dr, dg, db;
			dr = d & 0x0000ff; dg = d & 0x00ff00; db = d & 0xff0000;
			
			uint32_t rr, rg, rb;
			rr = sr < dr ? sr : dr;
			rg = sg < dg ? sg : dg;
			rb = sb < db ? sb : db;
			
			r = rr | rg | rb;
		}
			break;
        default:
            MCUnreachableReturn(0);
	}
	
	if (x_src_alpha && x_dst_alpha)
	{
		uint8_t ra;
		ra = downscale(sa * da);
		r = packed_bilinear_bounded(src, 255 - da, dst, 255 - sa) + packed_scale_bounded(r | 0xff000000, ra);
	}
	else if (!x_src_alpha && x_dst_alpha)
		r = packed_bilinear_bounded(src, 255 - da, r | 0xff000000, da);
	else if (!x_dst_alpha && x_src_alpha)
		r = packed_bilinear_bounded(dst, 255 - sa, r | 0xff000000, sa);
	
	return r;
}

template<BasicImagingOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t basic_imaging_combiner(uint32_t dst, uint32_t src)
{
	uint32_t r;
	switch(x_combiner)
	{
		case OPERATION_BLEND_CLEAR:
			r = 0;
			break;
		case OPERATION_BLEND_SRC:
			r = src;
			break;
		case OPERATION_BLEND_DST:
			r = dst;
			break;
		case OPERATION_BLEND_SRC_OVER:
			if (x_src_alpha)
				r = packed_scale_add_bounded(dst, packed_inverse_alpha(src), src);
			else
				r = src;
			break;
		case OPERATION_BLEND_DST_OVER:
			if (x_dst_alpha)
				r = packed_scale_add_bounded(src, packed_inverse_alpha(dst), dst);
			else
				r = dst;
			break;
		case OPERATION_BLEND_SRC_IN:
			if (x_dst_alpha)
				r = packed_scale_bounded(src, packed_alpha(dst));
			else
				r = src;
			break;
		case OPERATION_BLEND_DST_IN:
			if (x_src_alpha)
				r = packed_scale_bounded(dst, packed_alpha(src));
			else
				r = dst;
			break;
		case OPERATION_BLEND_SRC_OUT:
			if (x_dst_alpha)
				r = packed_scale_bounded(src, packed_inverse_alpha(dst));
			else
				r = 0;
			break;
		case OPERATION_BLEND_DST_OUT:
			if (x_src_alpha)
				r = packed_scale_bounded(dst, packed_inverse_alpha(src));
			else
				r = 0;
			break;
		case OPERATION_BLEND_SRC_ATOP:
			if (x_dst_alpha && x_src_alpha)
				r = packed_bilinear_bounded(src, packed_alpha(dst), dst, packed_inverse_alpha(src));
			else if (!x_dst_alpha && x_src_alpha)
				r = packed_scale_add_bounded(dst, packed_inverse_alpha(src), src);
			else if (!x_src_alpha && x_dst_alpha)
				r = packed_scale_bounded(src, packed_alpha(dst));
			else
				r = src;
			break;
		case OPERATION_BLEND_DST_ATOP:
			if (x_dst_alpha && x_src_alpha)
				r = packed_bilinear_bounded(dst, packed_alpha(src), src, packed_inverse_alpha(dst));
			else if (!x_src_alpha && x_dst_alpha)
				r = packed_scale_add_bounded(src, packed_inverse_alpha(dst), dst);
			else if (!x_dst_alpha && x_src_alpha)
				r = packed_scale_bounded(dst, packed_alpha(src));
			else
				r = dst;
			break;
		case OPERATION_BLEND_XOR:
			if (x_dst_alpha && x_src_alpha)
				r = packed_bilinear_bounded(src, packed_inverse_alpha(dst), dst, packed_inverse_alpha(src));
			else if (!x_dst_alpha && x_src_alpha)
				r = packed_scale_bounded(dst, packed_inverse_alpha(src));
			else if (!x_src_alpha && x_dst_alpha)
				r = packed_scale_bounded(src, packed_inverse_alpha(dst));
			else
				r = 0;
			break;
		case OPERATION_BLEND_PLUS:
			r = packed_add_saturated(src, dst);
			break;
		case OPERATION_BLEND_MULTIPLY:
			if (x_dst_alpha && x_src_alpha)
				r = packed_multiply_bounded(src, dst) + packed_bilinear_bounded(src, packed_inverse_alpha(dst), dst, packed_inverse_alpha(src));
			else if (!x_dst_alpha && x_src_alpha)
				r = packed_multiply_bounded(src, dst) + packed_scale_bounded(dst, packed_inverse_alpha(src));
			else if (!x_src_alpha && x_dst_alpha)
				r = packed_multiply_bounded(src, dst) + packed_scale_bounded(src, packed_inverse_alpha(dst));
			else
				r = packed_multiply_bounded(src, dst);
			break;
		case OPERATION_BLEND_SCREEN:
			r = packed_multiply_bounded(src, packed_inverse(dst)) + dst;
			break;
        default:
            MCUnreachableReturn(0);
	}
	
	return r;
}

template<AdvancedImagingOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t advanced_imaging_combiner(uint32_t dst, uint32_t src)
{
	uint8_t t_src_red, t_src_green, t_src_blue, t_src_alpha;
	uint8_t t_dst_red, t_dst_green, t_dst_blue, t_dst_alpha;
	uint16_t t_dst_alpha_src_red, t_dst_alpha_src_green, t_dst_alpha_src_blue, t_dst_alpha_src_alpha, t_dst_alpha_dst_alpha;
	uint16_t t_dst_alpha_dst_red, t_dst_alpha_dst_green, t_dst_alpha_dst_blue;
	uint16_t t_src_alpha_dst_red, t_src_alpha_dst_green, t_src_alpha_dst_blue, t_src_alpha_dst_alpha;
	uint16_t t_inv_dst_alpha_src_red, t_inv_dst_alpha_src_green, t_inv_dst_alpha_src_blue, t_inv_dst_alpha_src_alpha;
	uint16_t t_inv_src_alpha_dst_red, t_inv_src_alpha_dst_green, t_inv_src_alpha_dst_blue, t_inv_src_alpha_dst_alpha;
	uint32_t t_dst_alpha_dst_alpha_src_alpha;
	uint8_t t_red, t_green, t_blue, t_alpha;
	
	t_src_red = src & 0xff;
	t_src_green = (src >> 8) & 0xff;
	t_src_blue = (src >> 16) & 0xff;
	if (x_src_alpha)
		t_src_alpha = src >> 24;
	else
		t_src_alpha = 255;
	
	t_dst_red = dst & 0xff;
	t_dst_green = (dst >> 8) & 0xff;
	t_dst_blue = (dst >>16) & 0xff;
	if (x_dst_alpha)
		t_dst_alpha = dst >> 24;
	else
		t_dst_alpha = 255;
	
	if (t_src_alpha == 0)
	{
		t_src_alpha_dst_red = 0;
		t_src_alpha_dst_green = 0;
		t_src_alpha_dst_blue = 0;
		t_src_alpha_dst_alpha = 0;
		
		t_inv_src_alpha_dst_red = upscale(t_dst_red);
		t_inv_src_alpha_dst_green = upscale(t_dst_green);
		t_inv_src_alpha_dst_blue = upscale(t_dst_blue);
		t_inv_src_alpha_dst_alpha = upscale(t_dst_alpha);
	}
	else if (t_src_alpha == 255)
	{
		t_src_alpha_dst_red = upscale(t_dst_red);
		t_src_alpha_dst_green = upscale(t_dst_green);
		t_src_alpha_dst_blue = upscale(t_dst_blue);
		t_src_alpha_dst_alpha = upscale(t_dst_alpha);
		
		t_inv_src_alpha_dst_red = 0;
		t_inv_src_alpha_dst_green = 0;
		t_inv_src_alpha_dst_blue = 0;
		t_inv_src_alpha_dst_alpha = 0;
	}
	else
	{
		t_src_alpha_dst_red = t_src_alpha * t_dst_red;
		t_src_alpha_dst_green = t_src_alpha * t_dst_green;
		t_src_alpha_dst_blue = t_src_alpha * t_dst_blue;
		t_src_alpha_dst_alpha = t_src_alpha * t_dst_alpha;
		
		t_inv_src_alpha_dst_red = (255 - t_src_alpha) * t_dst_red;
		t_inv_src_alpha_dst_green = (255 - t_src_alpha) * t_dst_green;
		t_inv_src_alpha_dst_blue = (255 - t_src_alpha) * t_dst_blue;
		t_inv_src_alpha_dst_alpha = (255 - t_src_alpha) * t_dst_alpha;
	}
	
	if (t_dst_alpha == 0)
	{
		t_dst_alpha_src_red = 0;
		t_dst_alpha_src_green = 0;
		t_dst_alpha_src_blue = 0;
		t_dst_alpha_src_alpha = 0;
		
		t_dst_alpha_dst_red = 0;
		t_dst_alpha_dst_blue = 0;
		t_dst_alpha_dst_green = 0;
		t_dst_alpha_dst_alpha = 0;
		
		t_dst_alpha_dst_alpha_src_alpha = 0;
		
		t_inv_dst_alpha_src_red = upscale(t_src_red);
		t_inv_dst_alpha_src_green = upscale(t_src_green);
		t_inv_dst_alpha_src_blue = upscale(t_src_blue);
		t_inv_dst_alpha_src_alpha = upscale(t_src_alpha);
	}
	else if (t_dst_alpha == 255)
	{
		t_dst_alpha_src_red = upscale(t_src_red);
		t_dst_alpha_src_green = upscale(t_src_green);
		t_dst_alpha_src_blue = upscale(t_src_blue);
		t_dst_alpha_src_alpha = upscale(t_src_alpha);
		
		t_dst_alpha_dst_red = upscale(t_dst_red);
		t_dst_alpha_dst_green = upscale(t_dst_green);
		t_dst_alpha_dst_blue = upscale(t_dst_blue);
		t_dst_alpha_dst_alpha = upscale((uint8_t)255);
		t_dst_alpha_dst_alpha_src_alpha = t_src_alpha == 255 ? upscale(upscale(uint8_t(255))) : upscale(uint16_t(255 * t_src_alpha));
		
		t_inv_dst_alpha_src_red = 0;
		t_inv_dst_alpha_src_green = 0;
		t_inv_dst_alpha_src_blue = 0;
		t_inv_dst_alpha_src_alpha = 0;
	}
	else
	{
		t_dst_alpha_src_red = t_dst_alpha * t_src_red;
		t_dst_alpha_src_green = t_dst_alpha * t_src_green;
		t_dst_alpha_src_blue = t_dst_alpha * t_src_blue;
		t_dst_alpha_src_alpha = t_dst_alpha * t_src_alpha;
		
		t_dst_alpha_dst_red = t_dst_alpha * t_dst_red;
		t_dst_alpha_dst_green = t_dst_alpha * t_dst_green;
		t_dst_alpha_dst_blue = t_dst_alpha * t_dst_blue;
		t_dst_alpha_dst_alpha = t_dst_alpha * t_dst_alpha;
		t_dst_alpha_dst_alpha_src_alpha = t_dst_alpha_dst_alpha * t_src_alpha;
		
		t_inv_dst_alpha_src_red = (255 - t_dst_alpha) * t_src_red;
		t_inv_dst_alpha_src_green = (255 - t_dst_alpha) * t_src_green;
		t_inv_dst_alpha_src_blue = (255 - t_dst_alpha) * t_src_blue;
		t_inv_dst_alpha_src_alpha = (255 - t_dst_alpha) * t_src_alpha;
	}
	
	switch(x_combiner)
	{
		case OPERATION_BLEND_OVERLAY:
			t_red = 2 * t_dst_red < t_dst_alpha ?
			downscale(2 * t_src_red * t_dst_red + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red) :
			downscale(t_src_alpha_dst_alpha - 2 * (t_dst_alpha - t_dst_red) * (t_src_alpha - t_src_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
			t_green = 2 * t_dst_green < t_dst_alpha ?
			downscale(2 * t_src_green * t_dst_green + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green) :
			downscale(t_src_alpha_dst_alpha - 2 * (t_dst_alpha - t_dst_green) * (t_src_alpha - t_src_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
			t_blue = 2 * t_dst_blue < t_dst_alpha ?
			downscale(2 * t_src_blue * t_dst_blue + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue) :
			downscale(t_src_alpha_dst_alpha - 2 * (t_dst_alpha - t_dst_blue) * (t_src_alpha - t_src_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
			t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
			break;
		case OPERATION_BLEND_DARKEN:
			t_red = downscale(fastmin(t_dst_alpha_src_red, t_src_alpha_dst_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
			t_green = downscale(fastmin(t_dst_alpha_src_green, t_src_alpha_dst_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
			t_blue = downscale(fastmin(t_dst_alpha_src_blue, t_src_alpha_dst_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
			t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
			break;
		case OPERATION_BLEND_LIGHTEN:
			t_red = downscale(fastmax(t_dst_alpha_src_red, t_src_alpha_dst_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
			t_green = downscale(fastmax(t_dst_alpha_src_green, t_src_alpha_dst_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
			t_blue = downscale(fastmax(t_dst_alpha_src_blue, t_src_alpha_dst_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
			t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
			break;
		case OPERATION_BLEND_DODGE:
			t_red = t_dst_alpha_src_red + t_src_alpha_dst_red >= t_src_alpha_dst_alpha ?
			downscale(t_dst_alpha_src_alpha + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red) :
			downscale(scaled_divide(t_src_alpha_dst_red, t_src_alpha, t_src_alpha - t_src_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
			t_green = t_dst_alpha_src_green + t_src_alpha_dst_green >= t_src_alpha_dst_alpha ?
			downscale(t_dst_alpha_src_alpha + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green) :
			downscale(scaled_divide(t_src_alpha_dst_green, t_src_alpha, t_src_alpha - t_src_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
			t_blue = t_dst_alpha_src_blue + t_src_alpha_dst_blue >= t_src_alpha_dst_alpha ?
			downscale(t_dst_alpha_src_alpha + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue) :
			downscale(scaled_divide(t_src_alpha_dst_blue, t_src_alpha, t_src_alpha - t_src_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
			t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
			break;
		case OPERATION_BLEND_BURN:
			t_red = t_dst_alpha_src_red + t_src_alpha_dst_red <= t_src_alpha_dst_alpha ?
			downscale(t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red) :
			downscale(scaled_divide(t_dst_alpha_src_red + t_src_alpha_dst_red - t_src_alpha_dst_alpha, t_src_alpha, t_src_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
			t_green = t_dst_alpha_src_green + t_src_alpha_dst_green <= t_src_alpha_dst_alpha ?
			downscale(t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green) :
			downscale(scaled_divide(t_dst_alpha_src_green + t_src_alpha_dst_green - t_src_alpha_dst_alpha, t_src_alpha, t_src_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
			t_blue = t_dst_alpha_src_blue + t_src_alpha_dst_blue <= t_src_alpha_dst_alpha ?
			downscale(t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue) :
			downscale(scaled_divide(t_dst_alpha_src_blue + t_src_alpha_dst_blue - t_src_alpha_dst_alpha, t_src_alpha, t_src_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
			t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
			break;
		case OPERATION_BLEND_HARD_LIGHT:
			t_red = 2 * t_src_red < t_src_alpha ?
			downscale(2 * t_src_red * t_dst_red + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red) :
			downscale(t_src_alpha_dst_alpha - 2 * (t_dst_alpha - t_dst_red) * (t_src_alpha - t_src_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
			t_green = 2 * t_src_green < t_src_alpha ?
			downscale(2 * t_src_green * t_dst_green + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green) :
			downscale(t_src_alpha_dst_alpha - 2 * (t_dst_alpha - t_dst_green) * (t_src_alpha - t_src_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
			t_blue = 2 * t_src_blue < t_src_alpha ?
			downscale(2 * t_src_blue * t_dst_blue + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue) :
			downscale(t_src_alpha_dst_alpha - 2 * (t_dst_alpha - t_dst_blue) * (t_src_alpha - t_src_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
			t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
			break;
		case OPERATION_BLEND_SOFT_LIGHT:
			if (2 * t_src_red < t_src_alpha)
				t_red = downscale(scaled_divide(t_src_alpha_dst_alpha + (t_dst_alpha - t_dst_red) * (t_src_alpha - 2 * t_src_red), t_dst_red, t_dst_alpha) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
			else if (8 * t_dst_red < t_dst_alpha)
				t_red = downscale(long_scaled_divide(t_dst_alpha_dst_alpha_src_alpha - (t_dst_alpha - t_dst_red) * (2 * t_src_red - t_src_alpha) * (3 * t_dst_alpha - 8 * t_dst_red), t_dst_red, t_dst_alpha_dst_alpha) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
			else
				t_red = downscale(t_src_alpha_dst_red + (sqrt(t_dst_alpha_dst_red) - t_dst_red) * (2 * t_src_red - t_src_alpha) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
			
			if (2 * t_src_green < t_src_alpha)
				t_green = downscale(scaled_divide(t_src_alpha_dst_alpha + (t_dst_alpha - t_dst_green) * (t_src_alpha - 2 * t_src_green), t_dst_green, t_dst_alpha) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
			else if (8 * t_dst_green < t_dst_alpha)
				t_green = downscale(long_scaled_divide(t_dst_alpha_dst_alpha_src_alpha - (t_dst_alpha - t_dst_green) * (2 * t_src_green - t_src_alpha) * (3 * t_dst_alpha - 8 * t_dst_green), t_dst_green, t_dst_alpha_dst_alpha) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
			else
				t_green = downscale(t_src_alpha_dst_green + (sqrt(t_dst_alpha_dst_green) - t_dst_green) * (2 * t_src_green - t_src_alpha) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
			
			if (2 * t_src_blue < t_src_alpha)
				t_blue = downscale(scaled_divide(t_src_alpha_dst_alpha + (t_dst_alpha - t_dst_blue) * (t_src_alpha - 2 * t_src_blue), t_dst_blue, t_dst_alpha) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
			else if (8 * t_dst_blue < t_dst_alpha)
				t_blue = downscale(long_scaled_divide(t_dst_alpha_dst_alpha_src_alpha - (t_dst_alpha - t_dst_blue) * (2 * t_src_blue - t_src_alpha) * (3 * t_dst_alpha - 8 * t_dst_blue), t_dst_blue, t_dst_alpha_dst_alpha) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
			else
				t_blue = downscale(t_src_alpha_dst_blue + (sqrt(t_dst_alpha_dst_blue) - t_dst_blue) * (2 * t_src_blue - t_src_alpha) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
			
			t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
			break;
		case OPERATION_BLEND_DIFFERENCE:
			t_red = t_src_red + t_dst_red - 2 * downscale(fastmin(t_dst_alpha_src_red, t_src_alpha_dst_red));
			t_green = t_src_green + t_dst_green - 2 * downscale(fastmin(t_dst_alpha_src_green, t_src_alpha_dst_green));
			t_blue = t_src_blue + t_dst_blue - 2 * downscale(fastmin(t_dst_alpha_src_blue, t_src_alpha_dst_blue));
			t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
			break;
		case OPERATION_BLEND_EXCLUSION:
			t_red = downscale(t_src_red * (t_dst_alpha - t_dst_red) + t_dst_red * (t_src_alpha - t_src_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
			t_green = downscale(t_src_green * (t_dst_alpha - t_dst_green) + t_dst_green * (t_src_alpha - t_src_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
			t_blue = downscale(t_src_blue * (t_dst_alpha - t_dst_blue) + t_dst_blue * (t_src_alpha - t_src_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
			t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
			break;
        default:
            MCUnreachableReturn(0);
	}
	
	if (x_dst_alpha)
		return t_red | (t_green << 8) | (t_blue << 16) | (t_alpha << 24);
	
	return t_red | (t_green << 8) | (t_blue << 16);
}

// These are commented out until a particular bug in Visual Studio (described
// below) is fixed.
//
/*template<BitwiseOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t pixel_combine(uint32_t dst, uint32_t src)
{
	return bitwise_combiner<x_combiner, x_dst_alpha, x_src_alpha>(dst, src);
}

template<ArithmeticOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t pixel_combine(uint32_t dst, uint32_t src)
{
	return arithmetic_combiner<x_combiner, x_dst_alpha, x_src_alpha>(dst, src);
}

template<BasicImagingOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t pixel_combine(uint32_t dst, uint32_t src)
{
	return basic_imaging_combiner<x_combiner, x_dst_alpha, x_src_alpha>(dst, src);
}

template<AdvancedImagingOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t pixel_combine(uint32_t dst, uint32_t src)
{
	return advanced_imaging_combiner<x_combiner, x_dst_alpha, x_src_alpha>(dst, src);
}*/

// Nasty shim to work around a Visual Studio compiler bug (it doesn't handle
// templates whose parameters are overloaded on enums properly so it always
// tries to use the AdvancedImagingOperation form of the template)
template<int x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t pixel_combine(uint32_t dst, uint32_t src)
{
    // These should get collapsed at compile-time so no need to worry about a
    // performance hit (assuming the compiler is half-way sensible...)
    if (x_combiner <= LAST_BITWISE_OPERATION)
        return bitwise_combiner<BitwiseOperation(x_combiner), x_dst_alpha, x_src_alpha>(dst, src);
    else if (x_combiner <= LAST_ARITHMETIC_OPERATION)
        return arithmetic_combiner<ArithmeticOperation(x_combiner), x_dst_alpha, x_src_alpha>(dst, src);
    else if (x_combiner <= LAST_BASIC_IMAGING_OPERATION)
        return basic_imaging_combiner<BasicImagingOperation(x_combiner), x_dst_alpha, x_src_alpha>(dst, src);
    else if (x_combiner <= LAST_ADVANCED_IMAGING_OPERATION)
        return advanced_imaging_combiner<AdvancedImagingOperation(x_combiner), x_dst_alpha, x_src_alpha>(dst, src);
    else
        MCUnreachable();
}

template<int x_combiner, bool x_dst_alpha, bool x_src_alpha> void surface_combine(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity)
{
	if (p_opacity == 0)
		return;
	
	uint32_t *t_dst_ptr;
	uint32_t t_dst_stride;
	
	t_dst_ptr = (uint32_t *)p_dst;
	t_dst_stride = (p_dst_stride >> 2) - p_width;
	
	uint32_t *t_src_ptr;
	uint32_t t_src_stride;
	
	t_src_ptr = (uint32_t *)p_src;
	t_src_stride = (p_src_stride >> 2) - p_width;
	
	for(; p_height > 0; --p_height, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
	{
		for(uint32_t t_width = p_width; t_width > 0; --t_width)
		{
			uint32_t t_src, t_dst;
			uint32_t t_pixel;
			
			t_src = *t_src_ptr++;
			t_dst = *t_dst_ptr;
			t_pixel = pixel_combine<x_combiner, x_dst_alpha, x_src_alpha>(t_dst, t_src);
			
			if (p_opacity != 255)
				t_pixel = packed_bilinear_bounded(t_pixel, p_opacity, t_dst, 255 - p_opacity);
			
			*t_dst_ptr++ = t_pixel;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

// MW-2009-02-09: This is the most important combiner so we optimize it.
//   This optimization is based on the observation that:
//     (1 - e) * dst + e * (src over dst)
//   Is equiavlent to:
//     (e * src) over dst
//  In addition, we inline the fundamental operations ourselves since it
//  seems this inlining *isn't* done by GCC on PowerPC.
//
static void surface_combine_blendSrcOver(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity)
{
	if (p_opacity == 0)
		return;
	
	uint32_t *t_dst_ptr;
	uint32_t t_dst_stride;
	
	t_dst_ptr = (uint32_t *)p_dst;
	t_dst_stride = (p_dst_stride >> 2) - p_width;
	
	uint32_t *t_src_ptr;
	uint32_t t_src_stride;
	
	t_src_ptr = (uint32_t *)p_src;
	t_src_stride = (p_src_stride >> 2) - p_width;
	
	for(; p_height > 0; --p_height, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
	{
		for(uint32_t t_width = p_width; t_width > 0; --t_width)
		{
			uint32_t t_src;
			uint32_t t_pixel;
			
			t_src = *t_src_ptr++;
			
			if (t_src != 0)
			{
				// Compute [ opacity * src ]
				if (p_opacity != 255)
				{
					uint32_t u, v;
					
					u = ((t_src & 0xff00ff) * p_opacity) + 0x800080;
					u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
					
					v = (((t_src >> 8) & 0xff00ff) * p_opacity) + 0x800080;
					v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
					
					t_src = u + v;
				}
				
				// Compute [ dst * (1 - src_alpha) + src ]
				{
					uint32_t x;
					uint8_t a;
					x = *t_dst_ptr;
					a = (~t_src) >> 24;
					
					uint32_t u, v;
					u = ((x & 0xff00ff) * a) + 0x800080;
					u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
					
					v = (((x >> 8) & 0xff00ff) * a) + 0x800080;
					v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
					
					t_pixel = u + v + t_src;
				}
				
				*t_dst_ptr++ = t_pixel;
				
				continue;
			}
			
			t_dst_ptr += 1;
		}
	}
}

// MW-2011-09-22: Optimized variant of blendSrcOver for alpha-masked image. The
//    assumption here is that 'src' is not pre-multipled, its alpha mask being
//    given by the mask array.
static void surface_combine_blendSrcOver_masked(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity)
{
	if (p_opacity == 0)
		return;
	
	uint32_t *t_dst_ptr;
	uint32_t t_dst_stride;
	t_dst_ptr = (uint32_t *)p_dst;
	t_dst_stride = (p_dst_stride >> 2) - p_width;
	
	uint32_t *t_src_ptr;
	uint32_t t_src_stride;
	t_src_ptr = (uint32_t *)p_src;
	t_src_stride = (p_src_stride >> 2) - p_width;
	
	if (p_opacity == 255)
	{
		// If opacity is 255 then we only need do:
		//   dst = dst * (1 - msk) + msk * src.
		for(; p_height > 0; --p_height, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
		{
			for(uint32_t t_width = p_width; t_width > 0; --t_width)
			{
				uint32_t t_src;
				t_src = *t_src_ptr++;
				
				uint8_t t_mask;
				t_mask = t_src >> 24;
				
				if (t_mask != 0)
				{
					uint32_t t_dst;
					t_src |= 0xff000000;
					t_dst = *t_dst_ptr;
					
					uint8_t sa, da;
					sa = t_mask;
					da = (~t_mask) & 0xff;
					
					// Compute [ dst * (1 - msk) + msk * src ]
					uint32_t u, v;
					
					u = (t_dst & 0xff00ff) * da + (t_src & 0xff00ff) * sa + 0x800080;
					u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
					
					v = ((t_dst >> 8) & 0xff00ff) * da + ((t_src >> 8) & 0xff00ff) * sa + 0x800080;
					v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
					
					*t_dst_ptr++ = u | v;
					
					continue;
				}
				
				t_dst_ptr += 1;
			}
		}
	}
	else
	{
		// If opacity is not 255 then we need to do:
		//   dst = dst * (1 - (msk * opc)) + (msk * opc) * src.
		for(; p_height > 0; --p_height, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
		{
			for(uint32_t t_width = p_width; t_width > 0; --t_width)
			{
				uint32_t t_src;
				t_src = *t_src_ptr++;
				
				uint8_t t_mask;
				t_mask = t_src >> 24;
				
				if (t_mask != 0)
				{
					uint32_t t_dst;
					t_src |= 0xff000000;
					t_dst = *t_dst_ptr;
					
					// Compute [ mask * opacity ]
					uint32_t a;
					a = p_opacity * t_mask;
					
					uint8_t sa, da;
					sa = ((a + 0x80) + ((a + 0x80) >> 8)) >> 8;
					da = (~sa) & 0xff;
					
					// Compute [ dst * (1 - msk) + msk * src ]
					uint32_t u, v;
					
					u = (t_dst & 0xff00ff) * da + (t_src & 0xff00ff) * sa + 0x800080;
					u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
					
					v = ((t_dst >> 8) & 0xff00ff) * da + ((t_src >> 8) & 0xff00ff) * sa + 0x800080;
					v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
					
					*t_dst_ptr++ = u | v;
					
					continue;
				}
				
				t_dst_ptr += 1;
			}
		}
	}	
}

// MW-2011-09-22: Optimized variant of blendSrcOver for a solid image. The
//   assumption here is that src's alpha is 255.
static void surface_combine_blendSrcOver_solid(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity)
{
	if (p_opacity == 0)
		return;
	
	uint32_t *t_dst_ptr;
	uint32_t t_dst_stride;
	t_dst_ptr = (uint32_t *)p_dst;
	t_dst_stride = (p_dst_stride >> 2) - p_width;
	
	uint32_t *t_src_ptr;
	uint32_t t_src_stride;
	t_src_ptr = (uint32_t *)p_src;
	t_src_stride = (p_src_stride >> 2) - p_width;
	
	// Special-case opacity == 255, as this is just a copy operation.
	if (p_opacity == 255)
	{
		for(; p_height > 0; --p_height, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
		{
			for(uint32_t t_width = p_width; t_width > 0; --t_width)
			{
				// MW-2011-10-03: [[ Bug ]] Make sure the source is opaque.
				*t_dst_ptr++ = *t_src_ptr++ | 0xff000000;
			}
		}
		
		return;
	}
	
	// Otherwise we must do:
	//   dst = (1 - opc) * dst + opc * src
	uint8_t t_inv_opacity;
	t_inv_opacity = 255 - p_opacity;
	
	for(; p_height > 0; --p_height, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
	{
		for(uint32_t t_width = p_width; t_width > 0; --t_width)
		{
			uint32_t t_src;
			uint32_t t_pixel;
			
			// MW-2011-10-03: [[ Bug ]] Make sure the source is opaque.
			t_src = *t_src_ptr++ | 0xff000000;
			
			// Compute [ opacity * src ]
			{
				uint32_t u, v;
				
				u = ((t_src & 0xff00ff) * p_opacity) + 0x800080;
				u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
				
				v = (((t_src >> 8) & 0xff00ff) * p_opacity) + 0x800080;
				v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
				
				t_src = u + v;
			}
			
			// Compute [ dst * (1 - src_alpha) + src ]
			{
				uint32_t x;
				x = *t_dst_ptr;
				
				uint32_t u, v;
				u = ((x & 0xff00ff) * t_inv_opacity) + 0x800080;
				u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
				
				v = (((x >> 8) & 0xff00ff) * t_inv_opacity) + 0x800080;
				v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
				
				t_pixel = u + v + t_src;
			}
			
			*t_dst_ptr++ = t_pixel;
		}
	}	
}

////////////////////////////////////////////////////////////////////////////////

typedef void (*surface_combiner_t)(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity);

// We have the following equivalences:
//   GXsrcBic == GXandReverse
//   GXnotSrcBic == GXand
//   GXcopy == GXblendSrc

/*static surface_combiner_t s_surface_combiners[] =
{
	surface_combine<OPERATION_CLEAR, true, true>,
	surface_combine<OPERATION_AND, true, true>,
	surface_combine<OPERATION_AND_REVERSE, true, true>,
	surface_combine_blendSrcOver, // Was OPERATION_COPY
	surface_combine<OPERATION_AND_INVERTED, true, true>,
	surface_combine<OPERATION_NOOP, true, true>,
	surface_combine<OPERATION_XOR, true, true>,
	surface_combine<OPERATION_OR, true, true>,
	surface_combine<OPERATION_NOR, true, true>,
	surface_combine<OPERATION_EQUIV, true, true>,
	surface_combine<OPERATION_INVERT, true, true>,
	surface_combine<OPERATION_OR_REVERSE, true, true>,
	surface_combine<OPERATION_COPY_INVERTED, true, true>,
	surface_combine<OPERATION_OR_INVERTED, true, true>,
	surface_combine<OPERATION_NAND, true, true>,
	surface_combine<OPERATION_SET, true, true>,
	
	//surface_combine<OPERATION_SRC_BIC, true, true>,
	//surface_combine<OPERATION_NOT_SRC_BIC, true, true>,
	
	surface_combine<OPERATION_BLEND, true, true>,
	surface_combine<OPERATION_ADD_PIN, true, true>,
	surface_combine<OPERATION_ADD_OVER, true, true>,
	surface_combine<OPERATION_SUB_PIN, true, true>,
	surface_combine<OPERATION_TRANSPARENT, true, true>,
	surface_combine<OPERATION_AD_MAX, true, true>,
	surface_combine<OPERATION_SUB_OVER, true, true>,
	surface_combine<OPERATION_AD_MIN, true, true>,
	
	surface_combine<OPERATION_BLEND_CLEAR, true, true>,
	surface_combine<OPERATION_BLEND_SRC, true, true>,
	surface_combine<OPERATION_BLEND_DST, true, true>,
	surface_combine_blendSrcOver,
	surface_combine<OPERATION_BLEND_DST_OVER, true, true>,
	surface_combine<OPERATION_BLEND_SRC_IN, true, true>,
	surface_combine<OPERATION_BLEND_DST_IN, true, true>,
	surface_combine<OPERATION_BLEND_SRC_OUT, true, true>,
	surface_combine<OPERATION_BLEND_DST_OUT, true, true>,
	surface_combine<OPERATION_BLEND_SRC_ATOP, true, true>,
	surface_combine<OPERATION_BLEND_DST_ATOP, true, true>,
	surface_combine<OPERATION_BLEND_XOR, true, true>,
	surface_combine<OPERATION_BLEND_PLUS, true, true>,
	surface_combine<OPERATION_BLEND_MULTIPLY, true, true>,
	surface_combine<OPERATION_BLEND_SCREEN, true, true>,
	
	surface_combine<OPERATION_BLEND_OVERLAY, true, true>,
	surface_combine<OPERATION_BLEND_DARKEN, true, true>,
	surface_combine<OPERATION_BLEND_LIGHTEN, true, true>,
	surface_combine<OPERATION_BLEND_DODGE, true, true>,
	surface_combine<OPERATION_BLEND_BURN, true, true>,
	surface_combine<OPERATION_BLEND_HARD_LIGHT, true, true>,
	surface_combine<OPERATION_BLEND_SOFT_LIGHT, true, true>,
	surface_combine<OPERATION_BLEND_DIFFERENCE, true, true>,
	surface_combine<OPERATION_BLEND_EXCLUSION, true, true>,
};*/

typedef uint32_t (*pixel_combiner_t)(uint32_t dst, uint32_t src);

static pixel_combiner_t s_pixel_combiners[] =
{
	pixel_combine<OPERATION_CLEAR, true, true>,
	pixel_combine<OPERATION_AND, true, true>,
	pixel_combine<OPERATION_AND_REVERSE, true, true>,
	pixel_combine<OPERATION_BLEND_SRC_OVER, true, true>,
	pixel_combine<OPERATION_AND_INVERTED, true, true>,
	pixel_combine<OPERATION_NOOP, true, true>,
	pixel_combine<OPERATION_XOR, true, true>,
	pixel_combine<OPERATION_OR, true, true>,
	pixel_combine<OPERATION_NOR, true, true>,
	pixel_combine<OPERATION_EQUIV, true, true>,
	pixel_combine<OPERATION_INVERT, true, true>,
	pixel_combine<OPERATION_OR_REVERSE, true, true>,
	pixel_combine<OPERATION_COPY_INVERTED, true, true>,
	pixel_combine<OPERATION_OR_INVERTED, true, true>,
	pixel_combine<OPERATION_NAND, true, true>,
	pixel_combine<OPERATION_SET, true, true>,
		
	pixel_combine<OPERATION_BLEND, true, true>,
	pixel_combine<OPERATION_ADD_PIN, true, true>,
	pixel_combine<OPERATION_ADD_OVER, true, true>,
	pixel_combine<OPERATION_SUB_PIN, true, true>,
	pixel_combine<OPERATION_TRANSPARENT, true, true>,
	pixel_combine<OPERATION_AD_MAX, true, true>,
	pixel_combine<OPERATION_SUB_OVER, true, true>,
	pixel_combine<OPERATION_AD_MIN, true, true>,
	
	pixel_combine<OPERATION_BLEND_CLEAR, true, true>,
	pixel_combine<OPERATION_BLEND_SRC, true, true>,
	pixel_combine<OPERATION_BLEND_DST, true, true>,
	pixel_combine<OPERATION_BLEND_SRC_OVER, true, true>,
	pixel_combine<OPERATION_BLEND_DST_OVER, true, true>,
	pixel_combine<OPERATION_BLEND_SRC_IN, true, true>,
	pixel_combine<OPERATION_BLEND_DST_IN, true, true>,
	pixel_combine<OPERATION_BLEND_SRC_OUT, true, true>,
	pixel_combine<OPERATION_BLEND_DST_OUT, true, true>,
	pixel_combine<OPERATION_BLEND_SRC_ATOP, true, true>,
	pixel_combine<OPERATION_BLEND_DST_ATOP, true, true>,
	pixel_combine<OPERATION_BLEND_XOR, true, true>,
	pixel_combine<OPERATION_BLEND_PLUS, true, true>,
	pixel_combine<OPERATION_BLEND_MULTIPLY, true, true>,
	pixel_combine<OPERATION_BLEND_SCREEN, true, true>,
	
	pixel_combine<OPERATION_BLEND_OVERLAY, true, true>,
	pixel_combine<OPERATION_BLEND_DARKEN, true, true>,
	pixel_combine<OPERATION_BLEND_LIGHTEN, true, true>,
	pixel_combine<OPERATION_BLEND_DODGE, true, true>,
	pixel_combine<OPERATION_BLEND_BURN, true, true>,
	pixel_combine<OPERATION_BLEND_HARD_LIGHT, true, true>,
	pixel_combine<OPERATION_BLEND_SOFT_LIGHT, true, true>,
	pixel_combine<OPERATION_BLEND_DIFFERENCE, true, true>,
	pixel_combine<OPERATION_BLEND_EXCLUSION, true, true>,
};

////////////////////////////////////////////////////////////////////////////////

static inline uint32_t MCGBlendModeToFunction(MCGBlendMode p_blend_mode)
{
	switch (p_blend_mode)
	{
		case kMCGBlendModeClear:
			return OPERATION_BLEND_CLEAR;
		case kMCGBlendModeCopy:
			return OPERATION_BLEND_SRC_OVER;
		case kMCGBlendModeSourceOver:
			return OPERATION_BLEND_SRC_OVER;
		case kMCGBlendModeSourceIn:
			return OPERATION_BLEND_SRC_IN;
		case kMCGBlendModeSourceOut:
			return OPERATION_BLEND_SRC_OUT;
		case kMCGBlendModeSourceAtop:
			return OPERATION_BLEND_SRC_ATOP;
		case kMCGBlendModeDestinationOver:
			return OPERATION_BLEND_DST_OVER;
		case kMCGBlendModeDestinationIn:
			return OPERATION_BLEND_DST_IN;
		case kMCGBlendModeDestinationOut:
			return OPERATION_BLEND_DST_OUT;
		case kMCGBlendModeDestinationAtop:
			return OPERATION_BLEND_DST_ATOP;
		case kMCGBlendModeXor:
			return OPERATION_BLEND_XOR;
		case kMCGBlendModePlusLighter:
			return OPERATION_BLEND_PLUS;
		case kMCGBlendModeMultiply:
			return OPERATION_BLEND_MULTIPLY;
		case kMCGBlendModeScreen:
			return OPERATION_BLEND_SCREEN;
		case kMCGBlendModeOverlay:
			return OPERATION_BLEND_OVERLAY;
		case kMCGBlendModeDarken:
			return OPERATION_BLEND_DARKEN;
		case kMCGBlendModeLighten:
			return OPERATION_BLEND_LIGHTEN;
		case kMCGBlendModeColorDodge:
			return OPERATION_BLEND_DODGE;
		case kMCGBlendModeColorBurn:
			return OPERATION_BLEND_BURN;
		case kMCGBlendModeSoftLight:
			return OPERATION_BLEND_SOFT_LIGHT;
		case kMCGBlendModeHardLight:
			return OPERATION_BLEND_HARD_LIGHT;
		case kMCGBlendModeDifference:
			return OPERATION_BLEND_DIFFERENCE;
		case kMCGBlendModeExclusion:
			return OPERATION_BLEND_EXCLUSION;
			
		case kMCGBlendModeLegacyClear:
			return OPERATION_CLEAR;
		case kMCGBlendModeLegacyAnd:
			return OPERATION_AND;
		case kMCGBlendModeLegacyAndReverse:
			return OPERATION_AND_REVERSE;
		case kMCGBlendModeLegacyCopy:
			return OPERATION_COPY;
		case kMCGBlendModeLegacyInverted:
			return OPERATION_AND_INVERTED;
		case kMCGBlendModeLegacyNoop:
			return OPERATION_NOOP;
		case kMCGBlendModeLegacyXor:
			return OPERATION_XOR;
		case kMCGBlendModeLegacyOr:
			return OPERATION_OR;
		case kMCGBlendModeLegacyNor:
			return OPERATION_NOR;
		case kMCGBlendModeLegacyEquiv:
			return OPERATION_EQUIV;
		case kMCGBlendModeLegacyInvert:
			return OPERATION_INVERT;
		case kMCGBlendModeLegacyOrReverse:
			return OPERATION_OR_REVERSE;
		case kMCGBlendModeLegacyCopyInverted:
			return OPERATION_COPY_INVERTED;
		case kMCGBlendModeLegacyOrInverted:
			return OPERATION_OR_INVERTED;
		case kMCGBlendModeLegacyNand:
			return OPERATION_NAND;
		case kMCGBlendModeLegacySet:
			return OPERATION_SET;
		case kMCGBlendModeLegacyBlend:
			return OPERATION_BLEND;
		case kMCGBlendModeLegacyAddPin:
			return OPERATION_ADD_PIN;
		case kMCGBlendModeLegacyAddOver:
			return OPERATION_ADD_OVER;
		case kMCGBlendModeLegacySubPin:
			return OPERATION_SUB_PIN;
		case kMCGBlendModeLegacyTransparent:
			return OPERATION_TRANSPARENT;
		case kMCGBlendModeLegacyAdMax:
			return OPERATION_AD_MAX;
		case kMCGBlendModeLegacySubOver:
			return OPERATION_SUB_OVER;
		case kMCGBlendModeLegacyAdMin:
			return OPERATION_AD_MIN;
		case kMCGBlendModeLegacyBlendSource:
			return OPERATION_BLEND_SRC;
		case kMCGBlendModeLegacyBlendDestination:
			return OPERATION_BLEND_DST;
			
		default:
			return OPERATION_BLEND_SRC_OVER;
	}
}

////////////////////////////////////////////////////////////////////////////////

MCGLegacyBlendMode::MCGLegacyBlendMode(MCGBlendMode p_blend_mode)
{
	m_function = MCGBlendModeToFunction(p_blend_mode);
}

bool MCGLegacyBlendMode::asCoeff(SkXfermode::Coeff *src, SkXfermode::Coeff *dst) const
{
	return false;
}

bool MCGLegacyBlendMode::asMode(Mode* mode) const 
{
    return false;
}

/*void MCGLegacyBlendMode::xfer32(SkPMColor dst[], const SkPMColor src[], int count, const SkAlpha aa[]) const
{
	if (aa == NULL)
		s_surface_combiners[m_function](dst, count * 4, src, count * 4, count, 1, 0xFF);
	else
		s_surface_combiners[m_function](dst, count * 4, src, count * 4, count, 1, aa[0]);
}

void MCGLegacyBlendMode::xfer4444(uint16_t dst[], const SkPMColor src[], int count, const SkAlpha aa[]) const
{
}

void MCGLegacyBlendMode::xfer16(uint16_t dst[], const SkPMColor src[], int count, const SkAlpha aa[]) const
{
}

void MCGLegacyBlendMode::xferA8(SkAlpha dst[], const SkPMColor src[], int count, const SkAlpha aa[]) const
{
}*/

// MM-2013-12-05: [[ Bug 11505 ]] Implement xferColor rather than xfer32. This way we don't ignore the alpha component of the mask.
SkPMColor MCGLegacyBlendMode::xferColor(SkPMColor src, SkPMColor dst) const
{
	return s_pixel_combiners[m_function](dst, src);
}

////////////////////////////////////////////////////////////////////////////////
