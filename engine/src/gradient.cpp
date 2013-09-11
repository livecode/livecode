/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "execpt.h"

#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"
#include "mcerror.h"
#include "globals.h"
#include "objectstream.h"

#include "path.h"

#include "meta.h"

#include "gradient.h"
#include "paint.h"
#include "packed.h"

#if __BIG_ENDIAN__
#define iman_ 1
#else
#define iman_ 0
#endif
typedef long int32;

#ifdef _LINUX
//IM - fast double -> int code seems to be broken on linux
inline int32 fast_rint(double val) {
   return (int32)(val + 0.5);
}

inline int32 fast_floor(double val) {
   return (int32)val;
}
#else

/* Fast version of (int)rint()
    Works for -2147483648.5 .. 2147483647.49975574019
    Requires IEEE floating point.
*/
inline int32 fast_rint(double val) {
   val = val + 68719476736.0*65536.0*1.5;
   return ((int32*)&val)[iman_];
}

/* Fast version of (int)floor()
    Requires IEEE floating point.
    Rounds numbers greater than n.9999923668 to n+1 rather than n,
    this could be fixed by changing the FP rounding mode and using
    the fast_rint() code.
    Works for -32728 to 32727.99999236688
    The alternative that uses long-long works for -2147483648 to 
2147483647.999923688
*/
inline int32 fast_floor(double val) {
   val = val + (68719476736.0*1.5);
#if 0
   return (int32)((*(long long *)&val)>>16);
#else
   return (((int32*)&val)[iman_]>>16);
#endif
}
#endif

typedef struct _GradientPropList
{
	const char *token;
	MCGradientFillProperty value;
}
GradientPropList;

static GradientPropList gradientprops[] =
{
	{"type", P_GRADIENT_FILL_TYPE},
	{"ramp", P_GRADIENT_FILL_RAMP},
	{"from", P_GRADIENT_FILL_ORIGIN},
	{"to", P_GRADIENT_FILL_PRIMARY_POINT},
	{"via", P_GRADIENT_FILL_SECONDARY_POINT},
	{"quality", P_GRADIENT_FILL_QUALITY},
	{"mirror", P_GRADIENT_FILL_MIRROR},
	{"repeat", P_GRADIENT_FILL_REPEAT},
	{"wrap", P_GRADIENT_FILL_WRAP},
};

static Exec_stat MCGradientFillLookupProperty(MCNameRef p_token, MCGradientFillProperty& r_prop)
{
	uint4 tablesize = ELEMENTS(gradientprops);
	while (tablesize--)
	{
		if (MCNameIsEqualToCString(p_token, gradientprops[tablesize].token, kMCCompareCaseless))
		{
			r_prop = gradientprops[tablesize].value;
			return ES_NORMAL;
		}
	}
	
	MCeerror->add(EE_GRAPHIC_BADGRADIENTKEY, 0, 0, p_token);
	return ES_ERROR;
}

void MCGradientFillInit(MCGradientFill *&r_gradient, MCRectangle p_rect)
{
	r_gradient = new MCGradientFill;
	r_gradient->kind = kMCGradientKindLinear;
	r_gradient->quality = kMCGradientQualityNormal;
	r_gradient->mirror = false;
	r_gradient->wrap = false;
	r_gradient->repeat = 1;
	r_gradient->ramp = new MCGradientFillStop[2];
	r_gradient->ramp[0].offset = 0;
	r_gradient->ramp[0].color = MCGPixelPack(kMCGPixelFormatBGRA, 0, 0, 0, 255); // black
	r_gradient->ramp[0].hw_color = MCGPixelPackNative(0, 0, 0, 255);
	r_gradient->ramp[1].offset = STOP_INT_MAX;
	r_gradient->ramp[0].color = MCGPixelPack(kMCGPixelFormatBGRA, 255, 255, 255, 255); // white
	r_gradient->ramp[0].hw_color = MCGPixelPackNative(255, 255, 255, 255);
	r_gradient->ramp[0].difference = STOP_DIFF_MULT / STOP_INT_MAX;
	r_gradient->ramp_length = 2;
	r_gradient->origin.x = p_rect.x + p_rect.width / 2;
	r_gradient->origin.y = p_rect.y + p_rect.height / 2;
	r_gradient->primary.x = p_rect.x + p_rect.width * 7 / 8;
	r_gradient->primary.y = p_rect.y + p_rect.height / 2;
	r_gradient->secondary.x = p_rect.x + p_rect.width / 2;
	r_gradient->secondary.y = p_rect.y + p_rect.height * 7 / 8;
	r_gradient->old_origin.x = MININT2;
	r_gradient->old_origin.y = MININT2;
}

void MCGradientFillFree(MCGradientFill *p_gradient)
{
	delete p_gradient -> ramp;
	delete p_gradient;
}

MCGradientFill *MCGradientFillCopy(const MCGradientFill *p_gradient)
{
	if (p_gradient == NULL)
		return NULL;

	MCGradientFill *t_gradient = new MCGradientFill;
	t_gradient->kind = p_gradient->kind;
	
	t_gradient->ramp_length = p_gradient->ramp_length;
	t_gradient->ramp = new MCGradientFillStop[t_gradient->ramp_length];

	memcpy(t_gradient->ramp, p_gradient->ramp, sizeof(MCGradientFillStop)*t_gradient->ramp_length);

	t_gradient->origin = p_gradient->origin;
	t_gradient->primary = p_gradient->primary;
	t_gradient->secondary = p_gradient->secondary;
	t_gradient->old_origin.x = MININT2;
	t_gradient->old_origin.y = MININT2;

	t_gradient->quality = p_gradient->quality;
	t_gradient->mirror = p_gradient->mirror;
	t_gradient->repeat = p_gradient->repeat;
	t_gradient->wrap = p_gradient->wrap;

	return t_gradient;
}

Exec_stat MCGradientFillGetProperty(MCGradientFill* p_gradient, MCGradientFillProperty which, MCExecPoint &ep)
{
	if (p_gradient == NULL)
	{
		ep.setempty();
		return ES_NORMAL;
	}
	switch (which)
	{
	case P_GRADIENT_FILL_TYPE:
	{
		uint1 t_gradient_kind;
		if (p_gradient == NULL)
			t_gradient_kind = kMCGradientKindNone;
		else
			t_gradient_kind = p_gradient->kind;
				
		switch (t_gradient_kind)
		{
		case kMCGradientKindNone:
			ep.setstaticcstring("none");
		break;
		case kMCGradientKindLinear:
			ep.setstaticcstring("linear");
		break;
		case kMCGradientKindRadial:
			ep.setstaticcstring("radial");
		break;
		case kMCGradientKindConical:
			ep.setstaticcstring("conical");
		break;
		case kMCGradientKindDiamond:
			ep.setstaticcstring("diamond");
		break;
		case kMCGradientKindSpiral:
			ep.setstaticcstring("spiral");
		break;
		case kMCGradientKindXY:
			ep.setstaticcstring("xy");
		break;
		case kMCGradientKindSqrtXY:
			ep.setstaticcstring("sqrtxy");
		break;
		}
	}
	break;
	case P_GRADIENT_FILL_RAMP:
		if (p_gradient == NULL)
			ep.setempty();
		else
			MCGradientFillRampUnparse(p_gradient->ramp,p_gradient->ramp_length, ep);
	break;
	case P_GRADIENT_FILL_ORIGIN:
	{
		if (p_gradient == NULL)
			ep.setempty();
		else
			ep.setpoint(p_gradient->origin.x, p_gradient->origin.y);
	}
	break;
	case P_GRADIENT_FILL_PRIMARY_POINT:
	{
		if (p_gradient == NULL)
			ep.setempty();
		else
			ep.setpoint(p_gradient->primary.x, p_gradient->primary.y);
	}
	break;
	case P_GRADIENT_FILL_SECONDARY_POINT:
	{
		if (p_gradient == NULL)
			ep.setempty();
		else
			ep.setpoint(p_gradient->secondary.x, p_gradient->secondary.y);
	}
	break;
	case P_GRADIENT_FILL_QUALITY:
		{
			uint1 t_gradient_quality;
			if (p_gradient == NULL)
				t_gradient_quality = kMCGradientQualityNormal;
			else
				t_gradient_quality = p_gradient->quality;
			switch (t_gradient_quality)
			{
			case kMCGradientQualityNormal:
				ep.setstaticcstring("normal");
				break;
			case kMCGradientQualityGood:
				ep.setstaticcstring("good");
				break;
			}
		}
	break;
	case P_GRADIENT_FILL_MIRROR:
		ep.setboolean(p_gradient != nil ? p_gradient -> mirror : False);
		break;
	case P_GRADIENT_FILL_REPEAT:
		ep.setuint(p_gradient != nil ? p_gradient -> repeat : 0);
		break;
	case P_GRADIENT_FILL_WRAP:
		ep.setboolean(p_gradient != nil ? p_gradient -> wrap : False);
		break;
	}
	return ES_NORMAL;
}

Exec_stat MCGradientFillGetProperty(MCGradientFill* p_gradient, MCExecPoint &ep, MCNameRef prop)
{
	if (p_gradient == NULL)
	{
		ep.setempty();
		return ES_NORMAL;
	}
	uint4 tablesize = ELEMENTS(gradientprops);

	if (!MCNameIsEqualTo(prop, kMCEmptyName, kMCCompareCaseless))
	{
		MCGradientFillProperty t_property;
		if (MCGradientFillLookupProperty(prop, t_property) != ES_NORMAL)
		return ES_ERROR;

		return MCGradientFillGetProperty(p_gradient, t_property, ep);
	}
	else
	{
		MCVariableValue *v = new MCVariableValue;
		v->assign_new_array(tablesize);

		MCerrorlock++;
		while (tablesize--)
		{
			MCGradientFillGetProperty(p_gradient, gradientprops[tablesize].value, ep);
			v->store_element(ep, gradientprops[tablesize].token);
		}
		MCerrorlock--;
		ep.setarray(v, True);
	}
	return ES_NORMAL;
}

Exec_stat MCGradientFillSetProperty(MCGradientFill* &p_gradient, MCGradientFillProperty which, MCExecPoint &ep, Boolean &dirty, MCRectangle rect)
{
	MCGradientFill *t_gradient = NULL;
	MCString data = ep.getsvalue();
	Exec_stat t_stat = ES_NORMAL;
	
	if (p_gradient == NULL)
		MCGradientFillInit(t_gradient, rect);
	else
		t_gradient = p_gradient;

	switch (which)
	{
	case P_GRADIENT_FILL_TYPE:
	{
		MCGradientFillKind t_new_gradient_kind;
		if (ep.getsvalue() == "linear")
			t_new_gradient_kind = kMCGradientKindLinear;
		else if (ep.getsvalue() == "radial")
			t_new_gradient_kind = kMCGradientKindRadial;
		else if (ep.getsvalue() == "conical")
			t_new_gradient_kind = kMCGradientKindConical;
		else if (ep.getsvalue() == "diamond")
			t_new_gradient_kind = kMCGradientKindDiamond;
		else if (ep.getsvalue() == "spiral")
			t_new_gradient_kind = kMCGradientKindSpiral;
		else if (ep.getsvalue() == "xy")
			t_new_gradient_kind = kMCGradientKindXY;
		else if (ep.getsvalue() == "sqrtxy")
			t_new_gradient_kind = kMCGradientKindSqrtXY;
		else if (ep.getsvalue() == "none" || ep.getsvalue() == "0")
			t_new_gradient_kind = kMCGradientKindNone;
		else
		{
			MCeerror->add(EE_GRAPHIC_BADGRADIENTTYPE, 0, 0, data);
			t_stat = ES_ERROR;
			break;
		}

		if (t_new_gradient_kind != t_gradient->kind)
		{
			t_gradient->kind = t_new_gradient_kind;
			dirty = True;
		}
	}
	break;
	case P_GRADIENT_FILL_RAMP:
		if (!MCGradientFillRampParse(t_gradient->ramp, t_gradient->ramp_length, data))
		{
			MCeerror->add(EE_GRAPHIC_BADGRADIENTRAMP, 0, 0, data);
			t_stat = ES_ERROR;
		}
		dirty = true;
	break;
	case P_GRADIENT_FILL_ORIGIN:
		if (!MCU_parsepoint(t_gradient->origin, data))
		{
			MCeerror->add(EE_GRAPHIC_BADGRADIENTPOINT, 0, 0, data);
			t_stat = ES_ERROR;
		}
		else
		{
			t_gradient->old_origin.x = MININT2;
			t_gradient->old_origin.y = MININT2;
			dirty = true;
		}
	break;
	case P_GRADIENT_FILL_PRIMARY_POINT:
		if (!MCU_parsepoint(t_gradient->primary, data))
		{
			MCeerror->add(EE_GRAPHIC_BADGRADIENTPOINT, 0, 0, data);
			t_stat = ES_ERROR;
		}
		else
		{
			t_gradient->old_origin.x = MININT2;
			t_gradient->old_origin.y = MININT2;
			dirty = true;
		}
	break;
	case P_GRADIENT_FILL_SECONDARY_POINT:
		if (!MCU_parsepoint(t_gradient->secondary, data))
		{
			MCeerror->add(EE_GRAPHIC_BADGRADIENTPOINT, 0, 0, data);
			t_stat = ES_ERROR;
		}
		else
		{
			t_gradient->old_origin.x = MININT2;
			t_gradient->old_origin.y = MININT2;
			dirty = true;
		}
	break;
	case P_GRADIENT_FILL_QUALITY:
	{
		MCGradientFillQuality t_new_gradient_quality;
		if (ep.getsvalue() == "normal")
			t_new_gradient_quality = kMCGradientQualityNormal;
		else if (ep.getsvalue() == "good")
			t_new_gradient_quality = kMCGradientQualityGood;
		else
		{
			MCeerror->add(EE_GRAPHIC_BADGRADIENTQUALITY, 0, 0, data);
			t_stat = ES_ERROR;
			break;
		}

		if (t_new_gradient_quality != t_gradient->quality)
		{
			t_gradient->quality = t_new_gradient_quality;
			dirty = True;
		}
	}
	break;
	case P_GRADIENT_FILL_MIRROR:
		bool t_new_mirror;
		if (ep.getsvalue() == MCtruestring)
			t_new_mirror = true;
		else if (ep.getsvalue() == MCfalsestring)
			t_new_mirror = false;
		else
			{
				MCeerror->add(EE_GRAPHIC_NAB, 0, 0, data);
				t_stat = ES_ERROR;
				break;
			}
		if (t_gradient->mirror != t_new_mirror)
		{
			t_gradient->mirror = t_new_mirror;
			dirty = True;
		}
		break;
	case P_GRADIENT_FILL_WRAP:
		bool t_new_wrap;
		if (ep.getsvalue() == MCtruestring)
			t_new_wrap = true;
		else if (ep.getsvalue() == MCfalsestring)
			t_new_wrap = false;
		else
			{
				MCeerror->add(EE_GRAPHIC_NAB, 0, 0, data);
				t_stat = ES_ERROR;
				break;
			}
		if (t_gradient->wrap != t_new_wrap)
		{
			t_gradient->wrap = t_new_wrap;
			dirty = True;
		}
		break;
	case P_GRADIENT_FILL_REPEAT:
		{
			uint4 t_new_repeat;
			if (!MCU_stoui4(data, t_new_repeat))
			{
				MCeerror->add(EE_GRAPHIC_NAN, 0, 0, data);
				t_stat = ES_ERROR;
				break;
			}

			t_new_repeat = MCU_max(1U, MCU_min(t_new_repeat, 255U));
			if (t_gradient->repeat != t_new_repeat)
			{
				t_gradient->repeat = (uint1)t_new_repeat;
				dirty = True;
			}
		}
		break;
	}
	if (t_stat == ES_ERROR)
	{
		if (p_gradient == NULL)
			MCGradientFillFree(t_gradient);
		return ES_ERROR;
	}

	if (dirty)
	{
		if (p_gradient == NULL)
			p_gradient = t_gradient;
	}
	else
	{
		if (p_gradient == NULL)
			MCGradientFillFree(t_gradient);
	}
	return t_stat;
}

Exec_stat MCGradientFillSetProperty(MCGradientFill* &p_gradient, MCExecPoint &ep, MCNameRef prop, Boolean &dirty, MCRectangle rect)
{
	MCGradientFill *t_gradient = NULL;
	MCString data = ep.getsvalue();
	Exec_stat t_stat = ES_NORMAL;
	
	if (p_gradient == NULL)
	{
		MCGradientFillInit(t_gradient, rect);
		dirty = true;
	}
	else
		t_gradient = p_gradient;

	uint4 tablesize = ELEMENTS(gradientprops);

	if (!MCNameIsEqualTo(prop, kMCEmptyName, kMCCompareCaseless))
	{
		MCGradientFillProperty t_property;
		t_stat = MCGradientFillLookupProperty(prop, t_property);
		
		if (t_stat == ES_NORMAL)
			t_stat = MCGradientFillSetProperty(t_gradient, t_property, ep, dirty, rect);
			}
	else
	{
		MCVariableValue *v = ep.getarray();
		if (v == NULL)
		{
			delete t_gradient;
			t_gradient = NULL;
			p_gradient = NULL;
			dirty = True;
		}
		else if (!v->is_array())
		{
			MCeerror->add(EE_GRAPHIC_BADGRADIENTKEY, 0, 0, data);
			t_stat = ES_ERROR;
		}
		else
		{
			MCerrorlock++;
			while (tablesize--)
			{
				v->fetch_element(ep, gradientprops[tablesize].token);
				MCGradientFillSetProperty(t_gradient, gradientprops[tablesize].value, ep, dirty, rect);
			}
			MCerrorlock--;
		}
	}

	if (t_stat == ES_ERROR)
	{
		if (p_gradient == NULL)
			MCGradientFillFree(t_gradient);
		return ES_ERROR;
	}

	if (dirty)
	{
		if (p_gradient == NULL)
			p_gradient = t_gradient;
	}
	else
	{
		if (p_gradient == NULL)
			MCGradientFillFree(t_gradient);
	}
	return t_stat;
}

int cmp_gradient_fill_stops(const void *a, const void *b)
{
	return ((MCGradientFillStop*)a)->offset - ((MCGradientFillStop*)b)->offset;
}

Boolean MCGradientFillRampParse(MCGradientFillStop* &r_stops, uint1 &r_stop_count, const MCString &r_data)
{
	Boolean allvalid = True;
	bool ordered = true;
	uint4 t_nstops = 0;
	uint4 l = r_data.getlength();
	const char *sptr = r_data.getstring();
	// avoid overflow in the case of extremely long ramps
	while(t_nstops < 255 && (l != 0))
	{
		Boolean done1, done2;
		uint4 offset;
		offset = (uint4) (STOP_INT_MAX * MCU_fmin(1.0,MCU_fmax(0.0,MCU_strtor8(sptr, l, ',', done1))));
		uint1 r, g, b, a;
		r = MCU_max(0, MCU_min(255, MCU_strtol(sptr, l, ',', done2)));
		done1 &= done2;
		g = MCU_max(0, MCU_min(255, MCU_strtol(sptr, l, ',', done2)));
		done1 &= done2;
		b = MCU_max(0, MCU_min(255, MCU_strtol(sptr, l, ',', done2)));
		done1 &= done2;

		if (l == 0 || *(sptr-1) == '\n')
			a = 255;
		else
			a = MCU_max(0, MCU_min(255, MCU_strtol(sptr, l, ',', done2)));
		
		while (l && !isdigit((uint1)*sptr) && *sptr != '-' && *sptr != '+')
		{
			l--;
			sptr++;
		}
		if (!done1 || !done2)
			allvalid = False;
		if (t_nstops > 0 && offset < r_stops[t_nstops-1].offset)
			ordered = false;
		
		if (t_nstops+1 > r_stop_count)
			MCU_realloc((char **)&r_stops, t_nstops, t_nstops + 1, sizeof(MCGradientFillStop));
		r_stops[t_nstops].offset = offset;
		if (t_nstops > 0 && offset != r_stops[t_nstops-1].offset)
			r_stops[t_nstops-1].difference = STOP_DIFF_MULT / (offset - r_stops[t_nstops-1].offset);
		r_stops[t_nstops].color =  MCGPixelPack(kMCGPixelFormatBGRA, r, g, b, a);
		r_stops[t_nstops++].hw_color = MCGPixelPackNative(r, g, b, a);
	}
	r_stop_count = t_nstops;
	if (!ordered)
	{
		qsort(r_stops, r_stop_count, sizeof(MCGradientFillStop), cmp_gradient_fill_stops);
		for (uint4 i = 1; i < r_stop_count; i++)
			if (r_stops[i].offset != r_stops[i - 1].offset)
				r_stops[i - 1].difference = STOP_DIFF_MULT / (r_stops[i].offset - r_stops[i - 1].offset);
	}
	return allvalid;
}

#define STOP_LINE_LENGTH ((8 + 4*4 + 1))
void MCGradientFillRampUnparse(MCGradientFillStop* p_stops, uint1 p_stop_count, MCExecPoint &p_ep)
{
	uint4 i;

	p_ep.getbuffer(STOP_LINE_LENGTH*p_stop_count + 1);
	p_ep.clear();
	for (i=0; i<p_stop_count; i++)
	{
		char buf[STOP_LINE_LENGTH];
		uint1 t_strlen;
		uint1 r, g, b, a;
		
		a = (p_stops[i].color >> 24) & 0xFF;
		r = (p_stops[i].color >> 16) & 0xFF;
		g = (p_stops[i].color >> 8) & 0xFF;
		b = (p_stops[i].color) & 0xFF;
		
		// MW-2008-08-07: Adjusting the precision of the stop value since we have at most 65535 values
		//   which therefore need no more than 100000 decimal values.
		t_strlen = sprintf(buf, "%.5f,%d,%d,%d", p_stops[i].offset * (1.0 / STOP_INT_MAX), r, g, b);
		if (a!=255)
			sprintf(buf+t_strlen, ",%d", a);
		p_ep.concatcstring(buf, EC_RETURN, i==0);
	}

}

static void gradient_combiner_begin(MCCombiner *_self, int4 y)
{
	MCSolidCombiner *self = (MCSolidCombiner *)_self;
	self -> bits += y * self -> stride;
}

void gradient_combiner_end(MCCombiner *_self)
{
}

static void gradient_affine_combiner_advance(MCCombiner *_self, int4 dy)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner *)_self;
	self -> bits += dy * self -> stride;
	self->x_inc += self->x_coef_b * dy;
	self->y_inc += self->y_coef_b * dy;
}

#define FP_2PI ((int4)(2 * M_PI * (1<<8)))
#define FP_INV_2PI ((STOP_INT_MAX << 8) / FP_2PI)

template<MCGradientFillKind x_type> static inline int4 compute_index(int4 p_x, int4 p_y, bool p_mirror, uint4 p_repeat, bool p_wrap)
{
	int4 t_index;
	switch(x_type)
	{
		// Per gradient ramp index calculation
	case kMCGradientKindLinear:
		t_index = p_x;
		break;
	case kMCGradientKindConical:
		{
			int4 t_angle = fast_rint((atan2((double)p_y, p_x) * (1<<8)));
			if (t_angle < 0)
				t_angle += FP_2PI;
			t_index = (t_angle * FP_INV_2PI) >> 8;
		}
		break;
	case kMCGradientKindRadial:
		{
			real8 t_dist = ((real8)(p_x)*p_x + (real8)(p_y)*p_y);
			t_index = !p_wrap && t_dist > ((real8)STOP_INT_MAX * STOP_INT_MAX) ? STOP_INT_MAX + 1 : fast_rint(sqrt(t_dist));
		}
		break;
	case kMCGradientKindDiamond:
		t_index = MCU_max(MCU_abs(p_x), MCU_abs(p_y));
		break;
	case kMCGradientKindSpiral:
		{
			int4 t_angle = fast_rint((atan2((double)p_y, p_x) * (1<<8)));
			real8 t_dist = sqrt((real8)(p_x)*p_x + (real8)(p_y)*p_y);
			t_index = fast_rint(t_dist);
			if (t_angle > 0)
				t_angle -= FP_2PI;
			t_index -= (t_angle * FP_INV_2PI) >> 8;
			t_index %= STOP_INT_MAX;
		}
		break;
	case kMCGradientKindXY:
		{
			uint4 t_x = MCU_abs(p_x);  uint4 t_y = MCU_abs(p_y);
			t_index = (int4) ((int64_t)t_x * t_y / STOP_INT_MAX);
		}
		break;
	case kMCGradientKindSqrtXY:
		{
			real8 t_x = MCU_abs(p_x);  real8 t_y = MCU_abs(p_y);
			t_index = fast_rint(sqrt(t_x * t_y));
		}
		break;
	default:
		assert (false);
		return NULL;
	}
	if (p_mirror)
	{
		if (p_wrap)
		{
			if (p_repeat > 1)
				t_index = (t_index * p_repeat);
			t_index &= STOP_INT_MIRROR_MAX;
			if (t_index > STOP_INT_MAX)
			{
				t_index = STOP_INT_MAX - (t_index & STOP_INT_MAX);
			}
		}
		else
		{
			if (t_index >= STOP_INT_MAX)
			{
				if ((p_repeat & 1) == 0)
					t_index = -t_index;
			}
			else if (p_repeat > 1 && t_index > 0)
			{
				t_index = (t_index * p_repeat);
				t_index &= STOP_INT_MIRROR_MAX;
				if (t_index > STOP_INT_MAX)
				{
					t_index = STOP_INT_MAX - (t_index & STOP_INT_MAX);
				}
			}
		}
	}
	else
	{
		if (p_wrap)
			t_index &= STOP_INT_MAX;
		if (p_repeat > 1 && t_index > 0 && t_index < STOP_INT_MAX)
		{
			t_index = (t_index * p_repeat);
			t_index &= 0xFFFF;
		}
	}
	return t_index;
}

template<MCGradientFillKind x_type> static void MCGradientFillBlend(MCCombiner *_self, int4 fx, int4 tx, uint1 alpha)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner*)_self;
	uint4 *d;
	uint4 s;

	d = self -> bits;

	int4 t_index;
	int4 t_x = self->x_inc + self->x_coef_a * ((int4)fx);
	int4 t_y = self->y_inc + self->y_coef_a * ((int4)fx);

	int4 t_min = (int4)self->ramp[0].offset;
	int4 t_max = (int4)self->ramp[self->ramp_length - 1].offset;

	if (fx == tx) return;

	bool t_mirror = self->mirror;
	uint4 t_repeat = self->repeat;
	
	if (alpha == 255)
	{
		uint4 t_stop_pos = 0;

		t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat);
		while (fx < tx)
		{
			if (t_index <= t_min)
			{
				s = self->ramp[0].hw_color;
				s = packed_scale_bounded(s | 0xFF000000, s >> 24);
				while (t_index <= t_min)
				{
					d[fx] = packed_scale_bounded(d[fx], 255 - (s >> 24)) + s;
					fx += 1;
					if (fx == tx)
						return;
					t_x += self->x_coef_a;
					t_y += self->y_coef_a;
					t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat);
				}
			}

			if (t_index >= t_max)
			{
				s = self->ramp[self->ramp_length - 1].hw_color;
				s = packed_scale_bounded(s | 0xFF000000, s >> 24);
				while (t_index >= t_max)
				{
					d[fx] = packed_scale_bounded(d[fx], 255 - (s >> 24)) + s;
					fx += 1;
					if (fx == tx)
						return;
					t_x += self->x_coef_a;
					t_y += self->y_coef_a;
					t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat);
				}
			}

			while (t_index >= t_min && t_index <= t_max)
			{
				MCGradientFillStop *t_current_stop = &self->ramp[t_stop_pos];
				int4 t_current_offset = t_current_stop->offset;
				int4 t_current_difference = t_current_stop->difference;
				uint4 t_current_color = t_current_stop->hw_color;
				MCGradientFillStop *t_next_stop = &self->ramp[t_stop_pos+1];
				int4 t_next_offset = t_next_stop->offset;
				uint4 t_next_color = t_next_stop->hw_color;

				while (t_next_offset >= t_index && t_current_offset <= t_index)
				{
					uint1 b = ((t_index - t_current_offset) * t_current_difference) >> STOP_DIFF_PRECISION ;
					uint1 a = 255 - b;

					s = packed_bilinear_bounded(t_current_color, a, t_next_color, b);
					d[fx] = packed_bilinear_bounded(d[fx], 255 - (s >> 24), s | 0xFF000000, s >> 24);
					fx += 1;
					if (fx == tx)
						return;
					t_x += self->x_coef_a;
					t_y += self->y_coef_a;
					t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat);
				}
				if (t_current_offset > t_index && t_stop_pos > 0)
					t_stop_pos -= 1;
				else if (t_next_offset < t_index && t_stop_pos < (self->ramp_length - 1))
					t_stop_pos += 1;
			}
		}
	}
}

template<MCGradientFillKind x_type> static void MCGradientFillCombine(MCCombiner *_self, int4 fx, int4 tx, uint1 *mask)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner*)_self;
	uint4 *d;
	uint4 s;

	d = self -> bits;

	int4 t_index;
	int4 t_x = self->x_inc + self->x_coef_a * ((int4)fx);
	int4 t_y = self->y_inc + self->y_coef_a * ((int4)fx);

	int4 t_min = (int4)self->ramp[0].offset;
	int4 t_max = (int4)self->ramp[self->ramp_length - 1].offset;

	if (fx == tx) return;

	bool t_mirror = self->mirror;
	uint4 t_repeat = self->repeat;
	bool t_wrap = self->wrap;
	
	uint4 t_stop_pos = 0;

	t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
	while (fx < tx)
	{
		if (t_index <= t_min)
		{
			s = self->ramp[0].hw_color;
			while (t_index <= t_min)
			{
				uint4 sa = packed_scale_bounded(s | 0xFF000000, ((s >> 24) * *mask++) / 255);
				d[fx] = packed_scale_bounded(d[fx], 255 - (sa >> 24)) + sa;
				fx += 1;
				if (fx == tx)
					return;
				t_x += self->x_coef_a;
				t_y += self->y_coef_a;
				t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
			}
		}

		if (t_index >= t_max)
		{
			s = self->ramp[self->ramp_length - 1].hw_color;
			while (t_index >= t_max)
			{
				uint4 sa = packed_scale_bounded(s | 0xFF000000, ((s >> 24) * *mask++) / 255);
				d[fx] = packed_scale_bounded(d[fx], 255 - (sa >> 24)) + sa;
				fx += 1;
				if (fx == tx)
					return;
				t_x += self->x_coef_a;
				t_y += self->y_coef_a;
				t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
			}
		}

		while (t_index >= t_min && t_index <= t_max)
		{
			MCGradientFillStop *t_current_stop = &self->ramp[t_stop_pos];
			int4 t_current_offset = t_current_stop->offset;
			int4 t_current_difference = t_current_stop->difference;
			uint4 t_current_color = t_current_stop->hw_color;
			MCGradientFillStop *t_next_stop = &self->ramp[t_stop_pos+1];
			int4 t_next_offset = t_next_stop->offset;
			uint4 t_next_color = t_next_stop->hw_color;

			while (t_next_offset >= t_index && t_current_offset <= t_index)
			{
				uint1 b = ((t_index - t_current_offset) * t_current_difference) >> STOP_DIFF_PRECISION ;
				uint1 a = 255 - b;

				s = packed_bilinear_bounded(t_current_color, a, t_next_color, b);
				uint4 sa = packed_scale_bounded(s | 0xFF000000, ((s >> 24) * *mask++) / 255);
				d[fx] = packed_scale_bounded(d[fx], 255 - (sa >> 24)) + sa;
				fx += 1;
				if (fx == tx)
					return;
				t_x += self->x_coef_a;
				t_y += self->y_coef_a;
				t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
			}
			if (t_current_offset > t_index && t_stop_pos > 0)
				t_stop_pos -= 1;
			else if (t_next_offset < t_index && t_stop_pos < (self->ramp_length - 1))
				t_stop_pos += 1;
		}
	}
}

template<MCGradientFillKind x_type> static void blend_row(MCCombiner *_self, uint4 fx, uint4 tx, uint4 *p_buff)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner*)_self;
	uint4 s;

	int4 t_index;
	int4 t_x = self->x_inc + self->x_coef_a * ((int4)fx);
	int4 t_y = self->y_inc + self->y_coef_a * ((int4)fx);

	int4 t_min = (int4)self->ramp[0].offset;
	int4 t_max = (int4)self->ramp[self->ramp_length - 1].offset;

	uint4 t_stop_pos = 0;

	bool t_mirror = self->mirror;
	uint4 t_repeat = self->repeat;
	bool t_wrap = self->wrap;

	t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
	while (fx < tx)
	{
		if (t_index <= t_min)
		{
			s = self->ramp[0].hw_color;
			while (t_index <= t_min)
			{
				*p_buff = s;
				fx += 1;
				if (fx == tx)
					return;
				p_buff++;
				t_x += self->x_coef_a;
				t_y += self->y_coef_a;
				t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
			}
		}

		if (t_index >= t_max)
		{
			s = self->ramp[self->ramp_length - 1].hw_color;
			while (t_index >= t_max)
			{
				*p_buff = s;
				fx += 1;
				if (fx == tx)
					return;
				p_buff++;
				t_x += self->x_coef_a;
				t_y += self->y_coef_a;
				t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
			}
		}

		while (t_index >= t_min && t_index <= t_max)
		{
			MCGradientFillStop *t_current_stop = &self->ramp[t_stop_pos];
			int4 t_current_offset = t_current_stop->offset;
			int4 t_current_difference = t_current_stop->difference;
			uint4 t_current_color = t_current_stop->hw_color;
			MCGradientFillStop *t_next_stop = &self->ramp[t_stop_pos+1];
			int4 t_next_offset = t_next_stop->offset;
			uint4 t_next_color = t_next_stop->hw_color;

			while (t_next_offset >= t_index && t_current_offset <= t_index)
			{
				uint1 b = ((t_index - t_current_offset) * t_current_difference) >> STOP_DIFF_PRECISION ;
				uint1 a = 255 - b;

				s = packed_bilinear_bounded(t_current_color, a, t_next_color, b);
				*p_buff = s;
				fx += 1;
				if (fx == tx)
					return;
				p_buff++;
				t_x += self->x_coef_a;
				t_y += self->y_coef_a;
				t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
			}
			if (t_current_offset > t_index && t_stop_pos > 0)
				t_stop_pos -= 1;
			else if (t_next_offset < t_index && t_stop_pos < (self->ramp_length - 1))
				t_stop_pos += 1;
		}
	}
}

static void gradient_bilinear_affine_combiner_end(MCCombiner *_self)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner*)_self;
	delete self->buffer;
}

template<MCGradientFillKind x_type> static void MCGradientFillBilinearBlend(MCCombiner *_self, uint4 fx, uint4 tx, uint1 alpha)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner*)_self;
	uint4 *d;
	uint4 s;

	d = self -> bits;

	if (fx == tx) return;

	uint4 *t_buffer = self->buffer;
	uint4 t_bufflen = self->buffer_width;

	int4 x_a, x_b, x_inc;
	int4 y_a, y_b, y_inc;
	x_a = self->x_coef_a; x_b = self->x_coef_b; x_inc = self->x_inc;
	y_a = self->y_coef_a; y_b = self->y_coef_b; y_inc = self->y_inc;
	self->x_coef_a /= GRADIENT_AA_SCALE; self->x_coef_b /= GRADIENT_AA_SCALE;
	self->y_coef_a /= GRADIENT_AA_SCALE; self->y_coef_b /= GRADIENT_AA_SCALE;

	uint4 dx = (tx - fx) * GRADIENT_AA_SCALE;
	uint4 t_fx = fx * GRADIENT_AA_SCALE;
	for (int i = 0; i < GRADIENT_AA_SCALE; i++)
	{
		blend_row<x_type>(self, t_fx, t_fx + dx, t_buffer);
		t_buffer += t_bufflen;
		self->x_inc += self->x_coef_b;
		self->y_inc += self->y_coef_b;
	}

	self->x_coef_a = x_a; self->x_coef_b = x_b; self->x_inc = x_inc;
	self->y_coef_a = y_a; self->y_coef_b = y_b; self->y_inc = y_inc;

	uint4 i = 0;
	if (alpha == 255)
	{
		for (; fx < tx; fx++)
		{
			uint4 u;
			uint4 v;

#if GRADIENT_AA_SCALE == 2
			// unroll for GRADIENT_AA_SCALE == 2
			u = (self->buffer[i*2] & 0xFF00FF) + (self->buffer[i*2 + 1] & 0xFF00FF) + \
				(self->buffer[t_bufflen + i*2] & 0xFF00FF) + (self->buffer[t_bufflen + i*2 + 1] & 0xFF00FF);
			v = ((self->buffer[i*2] >> 8) & 0xFF00FF) + ((self->buffer[i*2 + 1] >> 8) & 0xFF00FF) + \
				((self->buffer[t_bufflen + i*2] >> 8) & 0xFF00FF) + ((self->buffer[t_bufflen + i*2 + 1] >> 8) & 0xFF00FF);
			u = (u >> 2) & 0xFF00FF;
			v = (v << 6) & 0xFF00FF00;
#endif
			i++;

			s = u | v;
			d[fx] = packed_bilinear_bounded(d[fx], 255 - (s >> 24), s | 0xFF000000, s >> 24);
		}
	}
}

template<MCGradientFillKind x_type> static void MCGradientFillBilinearCombine(MCCombiner *_self, int4 fx, int4 tx, uint1* mask)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner*)_self;
	uint4 *d;
	uint4 s;

	d = self -> bits;

	if (fx == tx) return;

	uint4 *t_buffer = self->buffer;
	uint4 t_bufflen = self->buffer_width;

	int4 x_a, x_b, x_inc;
	int4 y_a, y_b, y_inc;
	x_a = self->x_coef_a; x_b = self->x_coef_b; x_inc = self->x_inc;
	y_a = self->y_coef_a; y_b = self->y_coef_b; y_inc = self->y_inc;
	self->x_coef_a /= GRADIENT_AA_SCALE; self->x_coef_b /= GRADIENT_AA_SCALE;
	self->y_coef_a /= GRADIENT_AA_SCALE; self->y_coef_b /= GRADIENT_AA_SCALE;

	uint4 dx = (tx - fx) * GRADIENT_AA_SCALE;
	uint4 t_fx = fx * GRADIENT_AA_SCALE;
	for (int i = 0; i < GRADIENT_AA_SCALE; i++)
	{
		blend_row<x_type>(self, t_fx, t_fx + dx, t_buffer);
		t_buffer += t_bufflen;
		self->x_inc += self->x_coef_b;
		self->y_inc += self->y_coef_b;
	}

	self->x_coef_a = x_a; self->x_coef_b = x_b; self->x_inc = x_inc;
	self->y_coef_a = y_a; self->y_coef_b = y_b; self->y_inc = y_inc;

	uint4 i = 0;
	for (; fx < tx; fx++)
	{
		uint4 u = (self->buffer[i*2] & 0xFF00FF) + (self->buffer[i*2 + 1] & 0xFF00FF) + \
			(self->buffer[t_bufflen + i*2] & 0xFF00FF) + (self->buffer[t_bufflen + i*2 + 1] & 0xFF00FF);
		uint4 v = ((self->buffer[i*2] >> 8) & 0xFF00FF) + ((self->buffer[i*2 + 1] >> 8) & 0xFF00FF) + \
			((self->buffer[t_bufflen + i*2] >> 8) & 0xFF00FF) + ((self->buffer[t_bufflen + i*2 + 1] >> 8) & 0xFF00FF);
		u = (u >> 2) & 0xFF00FF;
		v = (v << 6) & 0xFF00FF00;

		i++;

		s = u | v;
		uint1 alpha = (s >> 24) * (*mask++) / 255;
		d[fx] = packed_bilinear_bounded(d[fx], 255 - alpha, s | 0xFF000000, alpha);
	}
}


MCGradientCombiner *MCGradientFillCreateCombiner(MCGradientFill *p_gradient, MCRectangle &r_clip)
{
	static bool s_gradient_affine_combiner_initialised = false;
	static MCGradientAffineCombiner s_gradient_affine_combiner;

	if (!s_gradient_affine_combiner_initialised)
	{
		s_gradient_affine_combiner.begin = gradient_combiner_begin;
		s_gradient_affine_combiner.advance = gradient_affine_combiner_advance;
		s_gradient_affine_combiner.combine = NULL;
		s_gradient_affine_combiner_initialised = true;
	}

	uint1 t_kind = p_gradient->kind;

	int4 vx = p_gradient->primary.x - p_gradient->origin.x;
	int4 vy = p_gradient->primary.y - p_gradient->origin.y;
	int4 wx = p_gradient->secondary.x - p_gradient->origin.x;
	int4 wy = p_gradient->secondary.y - p_gradient->origin.y;

	int4 d = vy * wx - vx *wy;

	if (d != 0)
	{
		s_gradient_affine_combiner.x_coef_a = STOP_INT_MAX * -wy / d;
		s_gradient_affine_combiner.x_coef_b = STOP_INT_MAX * wx / d;
		s_gradient_affine_combiner.x_inc = (uint4) (STOP_INT_MAX * (int64_t)(p_gradient->origin.x * wy + (r_clip.y - p_gradient->origin.y) * wx) / d);

		s_gradient_affine_combiner.y_coef_a = STOP_INT_MAX * vy / d;
		s_gradient_affine_combiner.y_coef_b = STOP_INT_MAX * -vx / d;
		s_gradient_affine_combiner.y_inc = (uint4) (STOP_INT_MAX * -(int64_t)(p_gradient->origin.x * vy + (r_clip.y - p_gradient->origin.y) * vx) / d);
	}
	s_gradient_affine_combiner.origin = p_gradient->origin;
	s_gradient_affine_combiner.ramp = p_gradient->ramp;
	s_gradient_affine_combiner.ramp_length = p_gradient->ramp_length;
	s_gradient_affine_combiner.mirror = p_gradient->mirror;
	s_gradient_affine_combiner.repeat = p_gradient->repeat;
	s_gradient_affine_combiner.wrap = p_gradient->wrap;

	switch (p_gradient->quality)
	{
	case kMCGradientQualityNormal:
		{
			s_gradient_affine_combiner.end = gradient_combiner_end;
			s_gradient_affine_combiner.x_inc += (s_gradient_affine_combiner.x_coef_a + s_gradient_affine_combiner.x_coef_b) >> 1;
			s_gradient_affine_combiner.y_inc += (s_gradient_affine_combiner.y_coef_a + s_gradient_affine_combiner.y_coef_b) >> 1;
			switch (t_kind)
			{
			case kMCGradientKindConical:
				s_gradient_affine_combiner.combine = MCGradientFillCombine<kMCGradientKindConical>;
				return &s_gradient_affine_combiner;
			case kMCGradientKindLinear:
				s_gradient_affine_combiner.combine = MCGradientFillCombine<kMCGradientKindLinear>;
				return &s_gradient_affine_combiner;
			case kMCGradientKindRadial:
				s_gradient_affine_combiner.combine = MCGradientFillCombine<kMCGradientKindRadial>;
				return &s_gradient_affine_combiner;
			case kMCGradientKindDiamond:
				s_gradient_affine_combiner.combine = MCGradientFillCombine<kMCGradientKindDiamond>;
				return &s_gradient_affine_combiner;
			case kMCGradientKindSpiral:
				s_gradient_affine_combiner.combine = MCGradientFillCombine<kMCGradientKindSpiral>;
				return &s_gradient_affine_combiner;
			case kMCGradientKindXY:
				s_gradient_affine_combiner.combine = MCGradientFillCombine<kMCGradientKindXY>;
				return &s_gradient_affine_combiner;
			case kMCGradientKindSqrtXY:
				s_gradient_affine_combiner.combine = MCGradientFillCombine<kMCGradientKindSqrtXY>;
				return &s_gradient_affine_combiner;
			}
		}
	case kMCGradientQualityGood:
		{
			s_gradient_affine_combiner.end = gradient_bilinear_affine_combiner_end;
			s_gradient_affine_combiner.buffer_width = GRADIENT_AA_SCALE * r_clip.width;
			s_gradient_affine_combiner.buffer = new uint4[GRADIENT_AA_SCALE * s_gradient_affine_combiner.buffer_width];

			s_gradient_affine_combiner.x_inc += (s_gradient_affine_combiner.x_coef_a + s_gradient_affine_combiner.x_coef_b) >> 2;
			s_gradient_affine_combiner.y_inc += (s_gradient_affine_combiner.y_coef_a + s_gradient_affine_combiner.y_coef_b) >> 2;
			switch (t_kind)
			{
			case kMCGradientKindConical:
				s_gradient_affine_combiner.combine = MCGradientFillBilinearCombine<kMCGradientKindConical>;
				return &s_gradient_affine_combiner;
			case kMCGradientKindLinear:
				s_gradient_affine_combiner.combine = MCGradientFillBilinearCombine<kMCGradientKindLinear>;
				return &s_gradient_affine_combiner;
			case kMCGradientKindRadial:
				s_gradient_affine_combiner.combine = MCGradientFillBilinearCombine<kMCGradientKindRadial>;
				return &s_gradient_affine_combiner;
			case kMCGradientKindDiamond:
				s_gradient_affine_combiner.combine = MCGradientFillBilinearCombine<kMCGradientKindDiamond>;
				return &s_gradient_affine_combiner;
			case kMCGradientKindSpiral:
				s_gradient_affine_combiner.combine = MCGradientFillBilinearCombine<kMCGradientKindSpiral>;
				return &s_gradient_affine_combiner;
			case kMCGradientKindXY:
				s_gradient_affine_combiner.combine = MCGradientFillBilinearCombine<kMCGradientKindXY>;
				return &s_gradient_affine_combiner;
			case kMCGradientKindSqrtXY:
				s_gradient_affine_combiner.combine = MCGradientFillBilinearCombine<kMCGradientKindSqrtXY>;
				return &s_gradient_affine_combiner;
			default:
				delete s_gradient_affine_combiner.buffer;
				return NULL;
			}
		}
	}

	return NULL;
}

uint4 MCGradientFillMeasure(MCGradientFill *p_gradient)
{
	return 3 + 4 * 3 + p_gradient -> ramp_length * 6;
}

IO_stat MCGradientFillSerialize(MCGradientFill *p_gradient, MCObjectOutputStream& p_stream)
{
	IO_stat t_stat;

	t_stat = p_stream . WriteU8((p_gradient->kind << 4) | (p_gradient->quality << 2) | (p_gradient->mirror << 1) | (p_gradient->wrap));
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU8(p_gradient -> repeat);
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU8(p_gradient -> ramp_length);
	
	MCPoint *t_point;
	t_point = &p_gradient -> origin;
	for(int i = 0; i < 3 && t_stat == IO_NORMAL; ++i)
	{
		t_stat = p_stream . WriteS16(t_point[i] . x);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . WriteS16(t_point[i] . y);
	}

	for(int i = 0; i < p_gradient -> ramp_length; ++i)
	{
		t_stat = p_stream . WriteU16((uint16_t)p_gradient -> ramp[i] . offset);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . WriteU32(p_gradient -> ramp[i] . color);
	}

	return t_stat;
}

IO_stat MCGradientFillUnserialize(MCGradientFill *p_gradient, MCObjectInputStream& p_stream)
{
	IO_stat t_stat;

	uint1 t_packed;
	t_stat = p_stream . ReadU8(t_packed);
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . ReadU8(p_gradient -> repeat);
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . ReadU8(p_gradient -> ramp_length);

	MCPoint *t_points;
	t_points = &p_gradient -> origin;
	for(int i = 0; i < 3 && t_stat == IO_NORMAL; ++i)
	{
		t_stat = p_stream . ReadS16(t_points[i] . x);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . ReadS16(t_points[i] . y);
	}

	if (t_stat == IO_NORMAL)
	{
		if (p_gradient != NULL)
			delete p_gradient -> ramp;
		p_gradient -> ramp = new MCGradientFillStop[p_gradient -> ramp_length];
		for(int i = 0; i < p_gradient -> ramp_length && t_stat == IO_NORMAL; ++i)
		{
			uint16_t t_offset;
			t_stat = p_stream . ReadU16(t_offset);
			if (t_stat == IO_NORMAL)
			{
				p_gradient -> ramp[i] . offset = t_offset;
				
				uint32_t t_color;
				t_stat = p_stream . ReadU32(t_color);
				if (t_stat == IO_NORMAL)
				{
					p_gradient -> ramp[i] . color = t_color;
					
					uint8_t r, g, b, a;
					MCGPixelUnpack(kMCGPixelFormatBGRA, t_color, r, g, b, a);
					p_gradient -> ramp[i] . hw_color = MCGPixelPackNative(r, g, b, a);
				}
			}
		}
	}

	if (t_stat == IO_NORMAL)
	{
		p_gradient->kind = t_packed >> 4;
		p_gradient->quality = (t_packed >> 2) & 0x03;
		p_gradient->mirror = (t_packed >> 1) & 0x01;
		p_gradient->wrap = t_packed & 0x01;

		for (uint4 i = 1; i < p_gradient->ramp_length; i++)
			if (p_gradient->ramp[i].offset != p_gradient->ramp[i - 1].offset)
				p_gradient->ramp[i - 1].difference = STOP_DIFF_MULT / (p_gradient->ramp[i].offset - p_gradient->ramp[i - 1].offset);
	}

	return t_stat;
}

uint1 *MCGradientFillSerialize(MCGradientFill *p_gradient, uint4 &r_length)
{
	uint1 *t_data = (uint1*)malloc(GRADIENT_HEADER_SIZE + p_gradient->ramp_length * (2 + 4));
	uint1 *t_ptr = t_data;

	*t_ptr++ = ((p_gradient->kind << 4) | (p_gradient->quality << 2) | 
		(p_gradient->mirror << 1) | (p_gradient->wrap));
	*t_ptr++ = p_gradient->repeat;
	*t_ptr++ = p_gradient->ramp_length;

	MCPoint *t_point = &p_gradient->origin;

	//write out 3 byte-swapped pairs of coordinates
	for (int i = 0; i < 3; i++)
	{
		*((uint2*)t_ptr) = MCSwapInt16HostToNetwork((uint2) t_point[i].x);
		t_ptr += 2;
		*((uint2*)t_ptr) = MCSwapInt16HostToNetwork((uint2) t_point[i].y);
		t_ptr += 2;
	}

	//write out byte-swapped ramp
	for (int i = 0; i < p_gradient->ramp_length; i++)
	{
		*((uint2*)t_ptr) = MCSwapInt16HostToNetwork((uint2)p_gradient->ramp[i].offset);
		t_ptr += 2;
		*((uint4*)t_ptr) = MCSwapInt32HostToNetwork(p_gradient->ramp[i].color);
		t_ptr += 4;
	}

	r_length = t_ptr - t_data;
	return t_data;
}

void MCGradientFillUnserialize(MCGradientFill *p_gradient, uint1 *p_data, uint4 &r_length)
{
	uint1 *t_ptr = p_data;

	uint1 t_packed = *t_ptr++;
	p_gradient->kind = t_packed >> 4;
	p_gradient->quality = (t_packed >> 2) & 0x03;
	p_gradient->mirror = (t_packed >> 1) & 0x01;
	p_gradient->wrap = t_packed & 0x01;

	p_gradient->repeat = *t_ptr++;
	p_gradient->ramp_length = *t_ptr++;
	MCPoint *t_points = &p_gradient->origin;

	for (int i=0; i < 3; i++)
	{
		t_points[i].x = MCSwapInt16NetworkToHost(*((uint2*)t_ptr));
		t_ptr += 2;
		t_points[i].y = MCSwapInt16NetworkToHost(*((uint2*)t_ptr));
		t_ptr += 2;
	}

	if (p_gradient->ramp != NULL)
		delete p_gradient->ramp;
	p_gradient->ramp = new MCGradientFillStop[p_gradient->ramp_length];

	for (uint4 i = 0; i < p_gradient->ramp_length; i++)
	{
		p_gradient->ramp[i].offset = MCSwapInt16NetworkToHost(*((uint2*)t_ptr));
		t_ptr += 2;

		p_gradient->ramp[i].color = MCSwapInt32NetworkToHost(*((uint4*)t_ptr));
		t_ptr += 4;
	}
	for (uint4 i = 1; i < p_gradient->ramp_length; i++)
		if (p_gradient->ramp[i].offset != p_gradient->ramp[i - 1].offset)
			p_gradient->ramp[i - 1].difference = STOP_DIFF_MULT / (p_gradient->ramp[i].offset - p_gradient->ramp[i - 1].offset);

	r_length = t_ptr - p_data;
}
