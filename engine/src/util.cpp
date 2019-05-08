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


#include "param.h"
#include "util.h"
#include "stack.h"
#include "card.h"
#include "stacklst.h"
#include "sellst.h"
#include "undolst.h"
#include "aclip.h"
#include "image.h"
#include "field.h"
#include "mcerror.h"
#include "osspec.h"
#include "redraw.h"
#include "mcssl.h"
#include "player.h"

#include "globals.h"
#include "exec.h"
#include "system.h"
#include "dispatch.h"
#include "scriptpt.h"

#if defined(_MACOSX)
#include <mach-o/dyld.h>
#endif

#include <algorithm>

// MDW-2014-07-06: [[ oval_points ]]
#define QA_NPOINTS 90

static MCPoint qa_points[QA_NPOINTS + 1];

static void MCU_play_message()
{
	MCAudioClip *acptr = MCacptr;
	MCacptr = nil;
    // PM-2014-12-22: [[ Bug 14269 ]] Nil checks to prevent a crash
	MCStack *sptr = (acptr != NULL ? acptr->getmessagestack() : NULL);
	if (sptr != NULL)
	{
		acptr->setmessagestack(NULL);
		sptr->getcurcard()->message_with_valueref_args(MCM_play_stopped, acptr->getname());
	}
	if (acptr != NULL && acptr->isdisposable())
		delete acptr;
}

void MCU_play()
{
	if (MCacptr && !MCacptr->play())
		MCU_play_message();
}

void MCU_play_stop()
{
	if (MCacptr)
	{
		MCacptr->stop(True);
		MCU_play_message();
	}
}

void MCU_init()
{
	int2 i;
	real8 increment = (M_PI / 2.0) / (real8)QA_NPOINTS;
	real8 angle = 0.0;

	// MDW 2014-07-26: [[ oval_points ]] bumped by one to fix
	for (i = 0 ; i < QA_NPOINTS+1 ; i++)
	{
		qa_points[i].x = (short)(sin(angle) * (real8)MAXINT2);
		qa_points[i].y = MAXINT2 - (short)(cos(angle) * (real8)MAXINT2);
		angle += increment;
	}

    /* Attempt to seed the random number generator using the system entropy
     * source. If that fails, fall back to constructing using some of the
     * entropy available from the properties of the current process. */
    MCAutoDataRef t_seed_data;
    if (MCSRandomData(sizeof(MCrandomseed), &t_seed_data))
    {
        MCMemoryCopy(&MCrandomseed, MCDataGetBytePtr(*t_seed_data),
                     sizeof(MCrandomseed));
    }
    else
    {
        MCLog("Warning: Failed to seed random number generator");
        MCrandomseed = (int4)(intptr_t)&MCdispatcher + MCS_getpid() + (int4)time(NULL);
    }
    MCU_srand();
}

void MCU_watchcursor(MCStack *sptr, Boolean force)
{
	if (!MClockcursor)
	{
		MCwatchcursor = True;
		if (sptr == NULL)
			sptr = MCmousestackptr ? MCmousestackptr : MCdefaultstackptr;
		sptr->resetcursor(force);
	}
}

void MCU_unwatchcursor(MCStack *sptr, Boolean force)
{
	if (!MClockcursor)
	{
		MCwatchcursor = False;
		if (sptr == NULL)
			sptr = MCmousestackptr ? MCmousestackptr : MCdefaultstackptr;
		sptr -> resetcursor(force);
	}
}

void MCU_resetprops(Boolean update)
{
	if (update)
	{
		if (MCwatchcursor || MCcursor != None)
		{
			if (!MClockcursor)
				MCcursor = None;
			MCwatchcursor = False;
			if (MCmousestackptr)
				MCmousestackptr->resetcursor(True);
			else
				MCdefaultstackptr->resetcursor(True);
		}
		
		// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
		MCRedrawForceUnlockScreen();

		if (MClockmenus)
		{
			MCscreen->updatemenubar(True);
			MClockmenus = False;
		}
		if (MCdragging)
		{
			MCdragging = False;
			MCselected->redraw();
		}
	}
	MCerrorlock.Reset();
	MClockerrors = MClockmessages = MClockrecent = False;
	MCscreen->setlockmoves(False);
	MCerrorlockptr = nil;
	MCinterrupt = False;
	MCdragspeed = 0;
	MCdynamiccard = nil;
	MCdynamicpath = False;
	MCexitall = False;
    
    // The clipboard lock is counted and needs to be balanced
    while (MCclipboardlockcount)
    {
        MCclipboardlockcount--;
        MCclipboard->Unlock();
    }
}

void MCU_saveprops(MCSaveprops &sp)
{
	sp.watchcursor = MCwatchcursor;
	MCwatchcursor = False;
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	MCRedrawSaveLockScreen(sp.lockscreen);
	sp.errorlock = MCerrorlock;
	sp.errorlockptr = MCerrorlockptr;
	sp.lockerrors = MClockerrors;
	sp.lockmessages = MClockmessages;
	sp.lockmoves = MCscreen->getlockmoves();
	sp.lockrecent = MClockrecent;
	sp.interrupt = MCinterrupt;
	sp.dragspeed = MCdragspeed;
	sp.dynamiccard = MCdynamiccard;
	sp.errorptr = MCerrorptr;
	sp.exitall = MCexitall;
}

void MCU_restoreprops(MCSaveprops &sp)
{
	MCwatchcursor = sp.watchcursor;
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	MCRedrawRestoreLockScreen(sp.lockscreen);
	// MW-2011-08-17: [[ Redraw ]] This shouldn't be necessary anymore as the
	//   updates will be flushed shortly after as (new) lockscreen == False.
	//if (oldls && !sp.lockscreen)
	//	MCstacks->redrawall(False);
	MCerrorlock = sp.errorlock;
	MCerrorlockptr = sp.errorlockptr;
	MClockerrors = sp.lockerrors;
	MClockmessages = sp.lockmessages;
	MCscreen->setlockmoves(sp.lockmoves);
	MClockrecent = sp.lockrecent;
	MCinterrupt = sp.interrupt;
	MCdragspeed = sp.dragspeed;
	MCdynamiccard = sp.dynamiccard;
	MCerrorptr = sp.errorptr;
	MCexitall = sp.exitall;
}

int4 MCU_any(int4 max)
{
	return (int4)(MCU_drand() * (real8)max);
}

bool MCU_getnumberformat(uint2 fw, uint2 trail, uint2 force, MCStringRef& r_string)
{
	bool t_success;
	t_success = true;

	MCAutoStringRef t_buffer;
	if (t_success)
		t_success = MCStringCreateMutable(0, &t_buffer);

	{
		uint2 i = MCU_max(fw - trail - 1, 0);
		while (t_success && i--)
			t_success = MCStringAppendNativeChar(*t_buffer, '0');	
		if (t_success && trail != 0)
		{
			t_success = MCStringAppendNativeChar(*t_buffer, '.');
			i = force;
			while (t_success && i--)
				t_success = MCStringAppendNativeChar(*t_buffer, '0');
			i = trail - force;
			while (t_success && i--)
				t_success = MCStringAppendNativeChar(*t_buffer, '#');
		}
	}

	if (t_success)
		return MCStringCopy(*t_buffer, r_string);

	return false;
}

void MCU_setnumberformat(MCStringRef d, uint2 &fw,
                         uint2 &trailing, uint2 &force)
{
	fw = MCStringGetLength(d);
    MCAutoPointer<char> temp_d;
    /* UNCHECKED */ MCStringConvertToCString(d, &temp_d);
	const char *sptr = *temp_d;
	const char *eptr = sptr;
	while (eptr - sptr < fw && *eptr != '.')
		eptr++;
	if (eptr - sptr == fw)
	{
		trailing = force = 0;
		return;
	}
	eptr++;
	if (eptr - sptr == fw)
		fw--;
	force = 0;
	while (eptr - sptr < fw && *eptr != '#')
	{
		eptr++;
		force++;
	}
	trailing = force;
	while (eptr - sptr < fw && *eptr == '#')
	{
		eptr++;
		trailing++;
	}
}

#define UtoF(u) (((double)((long)(u - 2147483647L - 1))) + 2147483648.0)

real8 MCU_stoIEEE(const char *bytes)
{
	real8 f = 0;
	int4 expon;
	uint4 hiMant, loMant;

	expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
	hiMant = ((unsigned long)(bytes[2] & 0xFF) << 24)
	         |    ((unsigned long)(bytes[3] & 0xFF) << 16)
	         |    ((unsigned long)(bytes[4] & 0xFF) << 8)
	         |    ((unsigned long)(bytes[5] & 0xFF));
	loMant = ((unsigned long)(bytes[6] & 0xFF) << 24)
	         |    ((unsigned long)(bytes[7] & 0xFF) << 16)
	         |    ((unsigned long)(bytes[8] & 0xFF) << 8)
	         |    ((unsigned long)(bytes[9] & 0xFF));

	if (expon == 0 && hiMant == 0 && loMant == 0)
		f = 0;
	else
	{
		if (expon == 0x7FFF)    /* MCinfinity or NaN */
			f = MCinfinity;
		else
		{
			expon -= 16383;
			f  = ldexp(UtoF(hiMant), expon-=31);
			f += ldexp(UtoF(loMant), expon-=32);
		}
	}
	if (bytes[0] & 0x80)
		return -f;
	else
		return f;
}

real8 MCU_i4tor8(int4 in)
{
	return (real8)in / 65535.0;
}

int4 MCU_r8toi4(real8 in)
{
	return (int4)(in * 65535.0);
}

#define N       16
#define RMASK    ((uint4)(1 << (N - 1)) + (1 << (N - 1)) - 1)
#define LOW(x)  ((uint4)(x) & RMASK)
#define HIGH(x) LOW((x) >> N)
#define MUL(x, y, z)    { uint4 l = (uint4)(x) * (uint4)(y); \
                (z)[0] = LOW(l); (z)[1] = HIGH(l); }
#define CARRY(x, y)     ((uint4)(x) + (uint4)(y) > RMASK)
#define ADDEQU(x, y, z) (z = CARRY(x, (y)), x = LOW(x + (y)))
#define X0      0x330E
#define X1      0xABCD
#define X2      0x1234
#define A0      0xE66D
#define A1      0xDEEC
#define A2      0x5
#define C       0xB
#define SET3(x, x0, x1, x2)     ((x)[0] = (x0), (x)[1] = (x1), (x)[2] = (x2))
#define SEED(x0, x1, x2) (SET3(x, x0, x1, x2), SET3(a, A0, A1, A2), c = C)

static uint4 x[3] = { X0, X1, X2 }, a[3] = { A0, A1, A2 }, c = C;

void MCU_srand()
{
	SEED(X0, LOW(MCrandomseed), HIGH(MCrandomseed));
}

real8 MCU_drand()
{
	static real8 two16m = 1.0 / (1L << N);
	uint4 p[2], q[2], r[2], carry0, carry1;

	MUL(a[0], x[0], p);
	ADDEQU(p[0], c, carry0);
	ADDEQU(p[1], carry0, carry1);
	MUL(a[0], x[1], q);
	ADDEQU(p[1], q[0], carry0);
	MUL(a[1], x[0], r);
	x[2] = LOW(carry0 + carry1 + CARRY(p[1], r[0]) + q[1] + r[1] +
	           a[0] * x[2] + a[1] * x[1] + a[2] * x[0]);
	x[1] = LOW(p[1] + r[0]);
	x[0] = LOW(p[0]);
	return (two16m * (two16m * (two16m * x[0] + x[1]) + x[2]));
}

// custom strtok that only skips single delimiters
static char *olds = NULL;
char *MCU_strtok(char *s, const char *delim)
{
	if (s == NULL)
	{
		if (olds == NULL)
		{
			MCS_seterrno(EINVAL);
			return NULL;
		}
		else
			s = olds;
	}
	if (*s == '\0')
	{
		olds = NULL;
		return NULL;
	}
	char *token = s;
	s = strpbrk(token, delim);
	if (s == NULL)
		olds = NULL;
	else
	{
		*s = '\0';
		olds = s + 1;
	}
	return token;
}

/* WRAPPER */ bool MCU_strtol(MCStringRef p_string, int4& r_l)
{
	Boolean t_converted;
	uint4 l = MCStringGetLength(p_string);
	MCAutoStringRefAsCString t_string;
	if (!t_string.Lock(p_string))
		return false;
    const char *sptr = *t_string;
	r_l = MCU_strtol(sptr, l, '\0', t_converted);
	return True == t_converted;
}

int4 MCU_strtol(const char *&sptr, uint4 &l, int1 p_delim, Boolean &done,
                Boolean reals, Boolean octals)
{
	done = False;
	MCU_skip_spaces(sptr, l);
	if (!l)
		return 0;
	Boolean negative = False;
	int4 value = 0;
	if (*sptr == '-' || *sptr == '+')
	{
		negative = *sptr == '-';
		sptr++;
		l--;
	}
	if (!l)
		return 0;
	uint4 startlength = l;
	uint2 base = 10;
	if (l && *sptr == '0')
    {
		if (l > 2 && MCS_tolower(*(sptr + 1)) == 'x')
		{
			base = 16;
			sptr += 2;
			l -= 2;
		}
		else if (octals)
        {
            base = 8;
            sptr++;
            l--;
        }
    }
	while (l)
	{
		if (isdigit((uint1)*sptr))
		{
			int4 v = *sptr - '0';
			if (base < 16 && value > MAXINT4 / base - v)  // prevent overflow
				return 0;
			value *= base;
			value += v;
		}
		else
			if (isspace((uint1)*sptr))
			{
				MCU_skip_spaces(sptr, l);
				if (l && *sptr == p_delim)
				{
					sptr++;
					l--;
				}
				break;
			}
			else
				if (l && p_delim && *sptr == p_delim)
				{
					sptr++;
					l--;
					break;
				}
				else
					if (*sptr == '.')
					{
						if (startlength > 1)
						{
							if (reals)
							{
								// MDW-2013-06-09: [[ Bug 10964 ]] Round integral values to nearest
								//   (consistent with getuint4() and round()).
								if (*(sptr+1) > '4')
								{
									value++;
								}
								do
								{
									sptr++;
									l--;
								}
								while (l && isdigit((uint1)*sptr));
							}
							else
								do
								{
									sptr++;
									l--;
								}
								while (l && *sptr == '0');
							if (l == 0)
								break;
							if (*sptr == p_delim || isspace((uint1)*sptr))
							{
								sptr++;
								l--;
								break;
							}
						}
						return 0;
					}
					else
					{
						char t_char = MCS_tolower(*sptr);
						if (base == 16 && t_char >= 'a' && t_char <= 'f')
						{
							value *= base;
							value += t_char - 'a' + 10;
						}
						else
							return 0;
					}
		sptr++;
		l--;
	}
	if (negative)
		value = -value;
	MCU_skip_spaces(sptr, l);
	done = True;
	return value;
}

void MCU_strip(char *sptr, uint2 trailing, uint2 force)
{
	if (trailing == 0)
		return;
	char *eptr = &sptr[strlen(sptr)];
	sptr = strchr(sptr, '.');
	uint2 count = force;
	while (count--)
		sptr++;
	while (--eptr > sptr)
		if (*eptr == '0')
		{
			*eptr = '\0';
			if (*(eptr - 1) == '.')
			{
				*(eptr - 1) = '\0';
				break;
			}
		}
		else
			break;
}

/*
This function is defined as follows:
Let x >= 0 and y > 0.
	x wrap y  := ((x - 1) mod y) + 1
	x wrap -y := x wrap y
	-x wrap y := -(x wrap y)
*/
real8 MCU_fwrap(real8 p_x, real8 p_y)
{
	real8 t_y;
	t_y = p_y > 0 ? p_y : -p_y;
	if (p_x >= 0)
		return (fmod(p_x - 1, t_y) + 1);
	else
		return -(fmod(-p_x - 1, t_y) + 1);
		
}

bool MCU_r8tos(real8 n, uint2 fw, uint2 trailing, uint2 force, MCStringRef &r_string)
{
	bool t_success = true;
	char *t_str = nil;
	uint4 t_s = 0;
	if (t_success)
		t_success = (0 != MCU_r8tos(t_str, t_s, n, fw, trailing, force));
	
	MCAutoStringRef t_string;
	if (t_success)
		t_success = MCStringCreateWithCStringAndRelease(t_str, &t_string);

	if (t_success)
		t_success = MCStringSetNumericValue(*t_string, n);

	if (t_success)
		t_success = MCStringCopy(*t_string, r_string);

	if (!t_success)
		delete[] t_str;

	return t_success;
}

uint4 MCU_r8tos(char *&d, uint4 &s, real8 n,
                uint2 fw, uint2 trailing, uint2 force)
{
	if (d == NULL || s <  R8L)
	{
		delete d;
		d = new (nothrow) char[R8L];
		s = R8L;
	}
	if (n < 0.0 && n >= -MC_EPSILON)
		n = 0.0;
    
    if (MCS_isfinite(n))
    {
        sprintf(d, "%0*.*f", fw, trailing, n);
        MCU_strip(d, trailing, force);
    }
    else
    {
        sprintf(d, "%f", n);
    }
	
	// 2007-09-11: [[ Bug 5321 ]] If the first character is '-', we must check
	//   to see if the value is actually '0', and if it is remove the '-'.
	if (*d == '-')
	{
		bool t_is_zero;
		t_is_zero = true;
		for(char *dptr = d + 1; *dptr != '\0'; ++dptr)
			if (*dptr == 'e')
				break;
			else if (*dptr != '.' && *dptr != '0')
			{
				t_is_zero = false;
				break;
			}

		if (t_is_zero)
			memmove(d, d + 1, strlen(d));
	}

	return strlen(d);
}

/* WRAPPER */
bool MCU_stor8(MCStringRef p_string, real8 &r_d, bool p_convert_octals)
{
    MCAutoStringRefAsCString t_cstring;
    t_cstring . Lock(p_string);
	return True == MCU_stor8(MCString(*t_cstring, strlen(*t_cstring)), r_d, p_convert_octals);
}

Boolean MCU_stor8(const MCString &s, real8 &d, Boolean convertoctals)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	int4 i = MCU_strtol(sptr, l, '\0', done, False, convertoctals);
	if (done)
	{
		d = i;
		return l == 0;
	}
	sptr = s.getstring();
	l = MCU_min(R8L - 1U, s.getlength());
	MCU_skip_spaces(sptr, l);
	// bugs in MSL means we need to check these things
	// MW-2006-04-21: [[ Purify ]] This was incorrect - we need to ensure l > 1 before running most
	//   of these tests.
	if (l == 0 || (l > 1 && (((MCS_tolower((uint1)sptr[1]) == 'x' && (l == 2 || !isxdigit((uint1)sptr[2])))
	        || (sptr[1] == '+' || sptr[1] == '-')))))
		return False;
	char buff[R8L];
	memcpy(buff, sptr, l);
	buff[l] = '\0';
	const char *newptr;
	d = strtod((char *)buff, (char **)&newptr);
	if (newptr == buff)
		return False;
	l = buff + l - newptr;
	MCU_skip_spaces(newptr, l);
	if (l != 0)
		return False;
	return True;
}

real8 MCU_strtor8(const char *&r_str, uint4 &r_len, int1 p_delim, Boolean &r_done, Boolean convertoctals)
{
	const char *sptr = r_str;
	uint4 l = r_len;
	Boolean done;
	real8 d;
	int4 i = MCU_strtol(sptr, l, p_delim, done, False, convertoctals);
	if (done)
	{
		r_done = True;
		r_str = sptr;
		r_len = l;
		return i;
	}
	MCU_skip_spaces(r_str, r_len);
	// bugs in MSL means we need to check these things
	// MW-2006-04-21: [[ Purify ]] This was incorrect - we need to ensure l > 1 before running most
	//   of these tests.
	if (r_len == 0 || (r_len > 1 && (((MCS_tolower((uint1)r_str[1]) == 'x' && (r_len == 2 || !isxdigit((uint1)r_str[2])))
	        || (r_str[1] == '+' || r_str[1] == '-')))))
	{
		r_done = False;
		return i;
	}
	char buff[R8L];
	uint4 buff_len = MCU_min(R8L - 1U, r_len);
	memcpy(buff, r_str, buff_len);
	buff[buff_len] = '\0';
	const char *newptr;
	d = strtod((char *)buff, (char **)&newptr);
	if (newptr == buff)
	{
		r_done = False;
		return d;
	}
	uint4 t_diff = newptr - buff;
	r_len = r_len - t_diff;
	r_str += t_diff;
	MCU_skip_spaces(r_str, r_len);
	if (r_len && *r_str == p_delim)
	{
			r_str++;
			r_len--;
	}
	r_done = True;

	return d;
}

/* WRAPPER */ bool MCU_stoi2(MCStringRef p_string, int2 &r_d)
{
    MCAutoStringRefAsCString t_cstring;
    t_cstring . Lock(p_string);
	return True == MCU_stoi2(MCString(*t_cstring, strlen(*t_cstring)), r_d);
}

Boolean MCU_stoi2(const MCString &s, int2 &d)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d = MCU_strtol(sptr, l, '\0', done, True, False);
	if (!done || l != 0)
		return False;
	return True;
}

/* WRAPPER */ bool MCU_stoui2(MCStringRef p_string, uint2 &r_d)
{
    MCAutoStringRefAsCString t_cstring;
    t_cstring . Lock(p_string);
	return True == MCU_stoui2(MCString(*t_cstring, strlen(*t_cstring)), r_d);
}

Boolean MCU_stoui2(const MCString &s, uint2 &d)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d = MCU_strtol(sptr, l, '\0', done, True, False);
	if (!done || l != 0)
		return False;
	return True;
}

Boolean MCU_stoi2x2(const MCString &s, int2 &d1, int2 &d2)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d1 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return False;
	d2 = MCU_strtol(sptr, l, '\0', done, True, False);
	if (!done || l != 0)
		return False;
	return True;
}

/* WRAPPER */ bool MCU_stoi2x2(MCStringRef p_string, int16_t& r_d1, int16_t& r_d2)
{
    MCAutoStringRefAsCString t_cstring;
    t_cstring . Lock(p_string);
	return True == MCU_stoi2x2(MCString(*t_cstring, strlen(*t_cstring)), r_d1, r_d2);
}

Boolean MCU_stoi2x4(const MCString &s, int2 &d1, int2 &d2, int2 &d3, int2 &d4)
{
	int32_t t_d1, t_d2, t_d3, t_d4;
	if (!MCU_stoi4x4(s, t_d1, t_d2, t_d3, t_d4))
		return False;
	
	d1 = t_d1;
	d2 = t_d2;
	d3 = t_d3;
	d4 = t_d4;
	
	return True;
}

/* WRAPPER */ bool MCU_stoi2x4(MCStringRef p_string, int16_t& r_d1, int16_t& r_d2, int16_t& r_d3, int16_t& r_d4)
{
    MCAutoStringRefAsCString t_cstring;
    t_cstring . Lock(p_string);
	return True == MCU_stoi2x4(MCString(*t_cstring, strlen(*t_cstring)), r_d1, r_d2, r_d3, r_d4);
}

/* WRAPPER */ bool MCU_stoi4x4(MCStringRef p_string, int32_t& r_d1, int32_t& r_d2, int32_t& r_d3, int32_t& r_d4)
{
    MCAutoStringRefAsCString t_cstring;
    t_cstring . Lock(p_string);
	return True == MCU_stoi4x4(MCString(*t_cstring, strlen(*t_cstring)), r_d1, r_d2, r_d3, r_d4);
}

Boolean MCU_stoi4x4(const MCString &s, int32_t &d1, int32_t &d2, int32_t &d3, int32_t &d4)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d1 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return False;
	d2 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return False;
	d3 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return False;
	d4 = MCU_strtol(sptr, l, '\0', done, True, False);
	if (!done || l != 0)
		return False;
	return True;
}

Boolean MCU_stoi4x2(const MCString &s, int32_t &d1, int32_t &d2)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d1 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return False;
	d2 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return False;
	return True;
}

/* WRAPPER */ bool MCU_stoi4(MCStringRef p_string, int4& r_d)
{
    MCAutoStringRefAsCString t_cstring;
    t_cstring . Lock(p_string);
	return True == MCU_stoi4(MCString(*t_cstring, strlen(*t_cstring)), r_d);
}

Boolean MCU_stoi4(const MCString &s, int4 &d)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d = MCU_strtol(sptr, l, '\0', done);
	if (!done || l != 0)
		return False;
	return True;
}
/* WRAPPER */ bool MCU_stoui4(MCStringRef p_string, uint4 &r_d)
{
    MCAutoStringRefAsCString t_cstring;
    t_cstring . Lock(p_string);
	return True == MCU_stoui4(MCString(*t_cstring, strlen(*t_cstring)), r_d);
}

Boolean MCU_stoui4(const MCString &s, uint4 &d)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d = MCU_strtol(sptr, l, '\0', done);
	if (!done || l != 0)
		return False;
	return True;
}

bool MCU_stoui4x2(MCStringRef p_string, uint4 &r_d1, uint4 &r_d2)
{
    MCAutoStringRefAsCString t_string;
    if (!t_string.Lock(p_string))
        return false;
    const char *sptr = *t_string;
    uint4 l = t_string.Size();
	Boolean done;
	r_d1 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return false;
	r_d2 = MCU_strtol(sptr, l, '\0', done, True, False);
	if (!done || l != 0)
		return false;
	return true;
}

/* WRAPPER */ bool MCU_stob(MCStringRef p_string, bool& r_condition)
{
	Boolean t_condition;
	bool t_success;
    
    MCAutoStringRefAsCString t_cstring;
    t_cstring . Lock(p_string);
	t_success = True == MCU_stob(MCString(*t_cstring, strlen(*t_cstring)), t_condition);
    
	if (t_success)
	{
		r_condition = t_condition == True;
		return true;
	}

	return false;
}

Boolean MCU_stob(const MCString &s, Boolean &condition)
{
	if (s.getlength() == 4 && (s.getstring() == MCtruestring
	                           || !MCU_strncasecmp(s.getstring(),
	                                               MCtruestring, 4)))
	{
		condition = True;
		return True;
	}
	if (s.getlength() == 5 && (s.getstring() == MCfalsestring
	                           || !MCU_strncasecmp(s.getstring(),
	                                               MCfalsestring, 5)))
	{
		condition = False;
		return True;
	}
	return False;
}

void MCU_lower(char *dptr, const MCString &s)
{
	uint4 length = s.getlength();
	const uint1 *sptr = (uint1 *)s.getstring();
	uint4 i;
	for (i = 0 ; i < length ; i++)
		*dptr++ = MCS_tolower(*sptr++);
}

Boolean MCU_offset(const MCString &part, const MCString &whole,
                   uint4 &offset, Boolean casesensitive)
{
	uint4 tl = part.getlength();
	uint4 sl = whole.getlength();
	offset = 0;

	if (tl > sl || tl == 0 || sl == 0)
		return False;
	uint4 length = sl - tl;
	uint4 i;
	const uint1 *pptr = (uint1 *)part.getstring();
	const uint1 *wptr = (uint1 *)whole.getstring();
	if (casesensitive)
		for (i = 0 ; i <= length ; i++)
		{
			const uint1 *sptr = pptr;
			const uint1 *dptr = wptr + i;
			if (*sptr != *dptr)
				continue;
			int4 diff = 0;
			uint4 n = tl;
			while (n--)
				if (*dptr++ != *sptr++)
				{
					diff = 1;
					break;
				}
			if (diff == 0)
			{
				offset = i;
				return True;
			}
		}
	else
		for (i = 0 ; i <= length ; i++)
		{
			int4 diff = 0;

			char t_p, t_w;
			t_p = *pptr;
			t_w = *(wptr + i);

			t_p = MCS_tolower(t_p);
			t_w = MCS_tolower(t_w);

			if (t_p != t_w)
				continue;
			else
				diff = MCU_strncasecmp((char *)pptr, (char *)wptr + i, tl);

			if (diff == 0)
			{
				offset = i;
				return True;
			}
		}
	return False;
}

void MCU_additem(char *&dptr, const char *sptr, Boolean first)
{
	uint4 dlength = strlen(dptr);
	uint4 slength;
	if (sptr == NULL)
		slength = 0;
	else
		slength = strlen(sptr);
	MCU_realloc((char **)&dptr, dlength, dlength + slength + 2, sizeof(char));
	if (!first)
		dptr[dlength++] = ',';
	if (sptr == NULL)
		dptr[dlength++] = '\0';
	else
		strcpy(&dptr[dlength], sptr);
}

void MCU_addline(char *&dptr, const char *sptr, Boolean first)
{
	uint4 dlength = strlen(dptr);
	uint4 slength;
	if (sptr == NULL)
		slength = 0;
	else
		slength = strlen(sptr);
	MCU_realloc((char **)&dptr, dlength, dlength + slength + 2, sizeof(char));
	if (!first)
		dptr[dlength++] = '\n';
	if (sptr == NULL)
		dptr[dlength++] = '\0';
	else
		strcpy(&dptr[dlength], sptr);
}

void MCU_break_string(const MCString &s, MCString *&ptrs, uint2 &nptrs,
                      Boolean isunicode)
{
	delete ptrs;
	ptrs = NULL;
	nptrs = 0;
	uint4 len = s.getlength();
	if (isunicode && (long)len & 1)
		len--;
	const char *string = s.getstring();
	if (string == NULL)
		return;
	const char *sptr = string;
	nptrs = 1;
	uint4 tlen = len;
	while (tlen)
	{
		tlen -= MCU_charsize(isunicode);
		if (MCU_comparechar(sptr, '\n', isunicode) && tlen)
			nptrs++;
		sptr += MCU_charsize(isunicode);
	}
	ptrs = (MCString *)new char[nptrs * sizeof(MCString)]; // GCC bug
	uint2 curptr = 0;
	sptr = string;
	const char *eptr =  sptr;
	Boolean wasfound = False;
	uint4 l = len;
	ptrs[0].set(sptr, len);
	while (l)
	{
		if (((wasfound = MCU_strchr(eptr, l, '\n', isunicode)) == True && l)
		        || sptr != NULL)
		{
			uint2 length = wasfound ? eptr - sptr : (string + len) - sptr ;
			ptrs[curptr++].set(sptr, length);
			if (!wasfound)
				break;
			eptr += MCU_charsize(isunicode);
			l -= MCU_charsize(isunicode);
			sptr = eptr;
		}
	}
}

#if !defined(_DEBUG_MEMORY)
void MCU_realloc(char **data, uint4 osize, uint4 nsize, uint4 csize)
{
	char *ndata = new (nothrow) char[nsize * csize];
	if (data != NULL)
	{
		if (nsize > osize)
			memcpy(ndata, *data, osize * csize);
		else
			memcpy(ndata, *data, nsize * csize);
		delete[] *data;
	}
	*data = ndata;
}
#else
void _dbg_MCU_realloc(char **data, uint4 osize, uint4 nsize, uint4 csize, const char *f, int l)
{
	char *ndata = (char *)_malloc_dbg(nsize * csize, _NORMAL_BLOCK, f, l); //_old_new(f, l) char[nsize * csize];
	if (data != NULL)
	{
		if (nsize > osize)
			memcpy(ndata, *data, osize * csize);
		else
			memcpy(ndata, *data, nsize * csize);
		delete *data;
	}
	*data = ndata;
}
#endif

// MM-2014-08-01: [[ Bug ]] Pulled name table initialisation out of MCU_matchname to prevent crah on Linux.
// IM-2014-08-20: [[ Bug ]] Cannot guarantee that the globals have been initialized before nametable,
// so use pointers to the globals rather than their value and dereference later.

// This *must* be updated if any chunk terms are added between CT_STACK
// and CT_LAST_CONTROL in the Chunk_term enum
static const char * const * const nametable[] =
{
	&MCstackstring,			/* CT_STACK */
	&MCnullstring,			/* CT_TOOLTIP */
	&MCaudiostring,			/* CT_AUDIO_CLIP */
	&MCvideostring,			/* CT_VIDEO_CLIP */
	&MCbackgroundstring,	/* CT_BACKGROUND */
	&MCcardstring,			/* CT_CARD */
	&MCnullstring,			/* CT_MARKED */
	&MCgroupstring,			/* CT_GROUP */
	&MCnullstring,			/* CT_LAYER */
	&MCbuttonstring,		/* CT_BUTTON */
	&MCnullstring,			/* CT_MENU */
	&MCnullstring,			/* CT_SCROLLBAR */
	&MCscrollbarstring,		/* CT_PLAYER */
	&MCimagestring,			/* CT_IMAGE */
	&MCgraphicstring,		/* CT_GRAPHIC */
	&MCepsstring,			/* CT_EPS */
	&MCmagnifierstring,		/* CT_MAGNIFY */
	&MCcolorstring,			/* CT_COLOR_PALETTE */
	&MCwidgetstring,		/* CT_WIDGET */
	&MCfieldstring			/* CT_FIELD */
};

bool MCU_matchname(MCNameRef test, Chunk_term type, MCNameRef name)
{
	if (name == nil || MCNameIsEmpty(name) || MCNameIsEmpty(test))
		return false;
    
	if (MCNameIsEqualToCaseless(name, test))
		return true;

	MCAssert(type - CT_STACK < (sizeof(nametable) / sizeof(nametable[0])));
	
    bool match = false;

    MCStringRef t_name, t_test;
    t_name = MCNameGetString(name);
    t_test = MCNameGetString(test);
    uindex_t t_offset, t_name_length;
    t_name_length = MCStringGetLength(t_name);
    
	if (MCStringFirstIndexOfChar(t_test, '"', 0, kMCCompareExact, t_offset) &&
        MCStringGetLength(t_test) - t_offset > t_name_length + 1 &&
        MCStringGetNativeCharAtIndex(t_test, t_offset + t_name_length + 1) == '"' &&
        MCStringSubstringIsEqualTo(t_test, MCRangeMake(t_offset + 1, t_name_length), t_name, kMCCompareCaseless) &&
        t_offset >= (int)strlen(*nametable[type - CT_STACK]) &&
        MCStringSubstringIsEqualTo(t_test, MCRangeMake(0, strlen(*nametable[type - CT_STACK])), MCSTR(*nametable[type - CT_STACK]), kMCCompareCaseless))
            match = True;

	return match;
}

void MCU_snap(int2 &p)
{
	if (!MCgrid)
		return;
	p = (p + (MCgridsize >> 1)) / MCgridsize * MCgridsize;
}

// MDW-2014-07-09: [[ oval_points ]] need to factor in startAngle and arcAngle
// this is now used for both roundrects and ovals
bool MCU_roundrect(MCPoint *&r_points, uindex_t &r_point_count,
                   const MCRectangle &rect, uint2 radius, uint2 startAngle, uint2 arcAngle, uint2 flags)
{
	uint2 i, j, k, count;
	uint2 t_x, t_y;
	bool ignore = false;

	MCAutoArray<MCPoint> t_points;
	if (!t_points.New(4 * QA_NPOINTS + 1))
		return false;
	
	MCRectangle tr = rect;
	tr . width--;
	tr . height--;

	uint2 rr_width, rr_height;
	if (radius < tr.width >> 1)
		rr_width = radius;
	else
		rr_width = tr.width >> 1;
	if (radius < tr.height >> 1)
		rr_height = radius;
	else
		rr_height = tr.height >> 1;
	
	uint2 origin_horiz, origin_vert;
	int2 arc, arclength;
    arc = 0;
    arclength = 0;
	origin_horiz = tr.x + rr_width;
	origin_vert = tr.y + rr_height;

	// pre-compute for speed if we're dealing with an oval
	if (arcAngle > 0)
	{
		arc = 360 - arcAngle;	// length of arc in degrees
		arclength = startAngle - arc;
	}

	j = QA_NPOINTS; // iterator for quadrants 1 and 3
	k = 1;			// iterator for quadrants 2 and 4
	i = 0;
	// each time through the loop: the quadrant table is prebuilt.
	// check for startAngle/arcAngle interaction
	for (count = 0; count < (QA_NPOINTS*4); count++)
	{
		ignore = false;
		// open wedge segment
		if ((count < startAngle && arclength > 0 && count > arclength) || 
			(arclength < 0 && count < startAngle) ||
			(arclength < 0 && count > arcAngle+startAngle) )
		{
			if (flags & F_OPAQUE)
			{
				t_x = origin_horiz;
				t_y = origin_vert;
			}
			else
			{
				ignore = true;
			}
		}
		else if (count < 90) // quadrant 1
		{
			t_x = tr . x + tr . width - rr_width + (qa_points[j] . x * rr_width / MAXINT2);
			t_y = tr . y                         + (qa_points[j] . y * rr_height / MAXINT2);
		}
		else if (count < 180) // quadrant 2
		{
			t_x = origin_horiz - (qa_points[k] . x * rr_width / MAXINT2);
			t_y = tr . y       + (qa_points[k] . y * rr_height / MAXINT2);
		}
		else if (count < 270) // quadrant 3
		{
			t_x = origin_horiz         - (qa_points[j] . x * rr_width / MAXINT2);
			t_y = tr . y + tr . height - (qa_points[j] . y * rr_height / MAXINT2);
		}
		else // quadrant 4
		{
			t_x = tr . x + tr . width - rr_width + (qa_points[k] . x * rr_width / MAXINT2);
			t_y = tr . y + tr . height           - (qa_points[k] . y * rr_height / MAXINT2);
		}

		if (ignore == false)
		{
			if (i == 0 || t_x != t_points[i-1] . x || t_y != t_points[i-1] . y)
			{
				t_points[i] . x = t_x;
				t_points[i] . y = t_y;
				i++;
			}
		}

		j--;
		if (j == 0)
			j = QA_NPOINTS;
		k++;
		if (k > QA_NPOINTS)
			k = 1;
	}
	
	t_points.Shrink(i);
	t_points.Take(r_points, r_point_count);
	return true;
}

Boolean MCU_parsepoints(MCPoint *&points, uindex_t &noldpoints, MCStringRef data)
{
    // This method will parse as much as it can from the string, so we need to
    // nativize first.
    
	Boolean allvalid = True;
	uindex_t npoints = 0;
	uint4 l = MCStringGetLength(data);
    MCAutoPointer<char> t_data;
    /* UNCHECKED */ MCStringConvertToCString(data, &t_data);
	const char *sptr = *t_data;
	while (l)
	{
		Boolean done1, done2;
		// MDW-2013-06-09: [[ Bug 11041 ]] Round non-integer values to nearest.
		int2 i1= (int2)MCU_strtol(sptr, l, ',', done1, True);
		int2 i2 = (int2)MCU_strtol(sptr, l, ',', done2, True);
		while (l && !isdigit((uint1)*sptr) && *sptr != '-' && *sptr != '+')
		{
			l--;
			sptr++;
		}
		if (!done1 || !done2)
		{
			i1 = i2 = MININT2;
			allvalid = False;
		}
		if (npoints + 1 > noldpoints)
			MCU_realloc((char **)&points, npoints, npoints + 1, sizeof(MCPoint));
		points[npoints].x = i1;
		points[npoints++].y = i2;
        // At this point we have skipped any CRs, so if the previous two chars
        // are CR (and there is room for two previous CRs) then we append a
        // 'non-point' to indicate a break in path. This ensures we preserve
        // a trailing 'non-point'.
		if (sptr - *t_data >= 2 && *(sptr - 1) == '\n' && *(sptr - 2) == '\n')
		{
			if (npoints + 1 > noldpoints)
				MCU_realloc((char **)&points, npoints, npoints + 1, sizeof(MCPoint));
			points[npoints].x = MININT2;
			points[npoints++].y = MININT2;
			allvalid = False;
		}
	}
	noldpoints = npoints;
	return allvalid;
}

Boolean MCU_parsepoint(MCPoint &point, MCStringRef data)
{
    // This method returns False if it can't parse the point - which will happen
    // if the string isn't native.
    if (!MCStringCanBeNative(data))
        return false;
    
    MCAutoPointer<char> t_data;
    /* UNCHECKED */ MCStringConvertToCString(data, &t_data);
    const char *sptr = *t_data;
    uint4 l = MCStringGetLength(data);

	Boolean done1, done2;
	// MDW-2013-06-09: [[ Bug 11041 ]] Round non-integer values to nearest.
	int2 i1= (int2)(MCU_strtol(sptr, l, ',', done1, True));
	int2 i2 = (int2)(MCU_strtol(sptr, l, ',', done2, True));
	if (!done1 || !done2)
	{
		i1 = i2 = MININT2;
		return False;
	}
	point.x = i1;
	point.y = i2;
	return True;
}

void MCU_set_rect(MCRectangle &rect, int2 p_x, int2 p_y, uint2 p_w, uint2 p_h)
{
	rect.x = p_x;
	rect.y = p_y;
	rect.width = p_w;
	rect.height = p_h;
}

void MCU_set_rect(MCRectangle32 &rect, int32_t p_x, int32_t p_y, int32_t p_w, int32_t p_h)
{
	rect.x = p_x;
	rect.y = p_y;
	rect.width = p_w;
	rect.height = p_h;
}

Boolean MCU_point_in_rect(const MCRectangle &srect, int2 p_x, int2 p_y)
{
	if (p_x >= srect.x && p_x < srect.x + srect.width
	        && p_y >= srect.y && p_y < srect.y + srect.height)
		return True;
	return False;
}

Boolean MCU_rect_in_rect(const MCRectangle &p, const MCRectangle &w)
{
	if (p.x >= w.x && p.x + p.width <= w.x + w.width
	        && p.y >= w.y && p.y + p.height <= w.y + w.height)
		return True;
	return False;
}

// AL-2015-10-07:: [[ External Handles ]] Check if possible zero-width line
// 'intersects' with rect.
bool MCU_line_intersect_rect(const MCRectangle& srect, const MCRectangle& line)
{
    MCRectangle t_test_rect;
    t_test_rect = line;
    
    
    // If the line is zero-width or zero-height, adjust the test rect
    //  so that we can just use MCU_intersect_rect.
    if (t_test_rect . width == 0)
    {
        t_test_rect . width++;
        if (srect . x > t_test_rect . x)
            t_test_rect . x--;
    }
    
    if (t_test_rect . height == 0)
    {
        t_test_rect . height++;
        if (srect . y > t_test_rect . y)
            t_test_rect . y--;
    }

    MCRectangle t_intersect;
    t_intersect = MCU_intersect_rect(srect, t_test_rect);

    return t_intersect . width != 0 && t_intersect . height != 0;
}


static inline double distance_to_point(int4 p_x, int4 p_y, int4 p_px, int4 p_py)
{
	double dx, dy;

	dx = p_px - p_x;
	dy = p_py - p_y;

	return dx * dx + dy * dy;
}

double MCU_squared_distance_from_line(int4 p_sx, int4 p_sy, int4 p_ex, int4 p_ey, int4 p_x, int4 p_y)
{
	double dx, dy;
	double d;

	dx = p_ex - p_sx;
	dy = p_ey - p_sy;

	if (dx == 0 && dy == 0)
		d = distance_to_point(p_x, p_y, p_sx, p_sy);
	else
	{
		double pdx, pdy;
		double u;

		pdx = p_x - p_sx;
		pdy = p_y - p_sy;

		u = (pdx * dx + pdy * dy) / (dx * dx + dy * dy);

		if (u <= 0)
			d = distance_to_point(p_x, p_y, p_sx, p_sy);
		else if (u >= 1)
			d = distance_to_point(p_x, p_y, p_ex, p_ey);
		else
			d = distance_to_point((int4)(p_sx + u * dx), (int4)(p_sy + u * dy), p_x, p_y);
	}

	return d;
}


Boolean MCU_point_on_line(MCPoint *p_points, uint2 p_npoints,
                          int2 p_x, int2 p_y, uint2 p_linesize)
{
	// OK-2008-12-04: [[Bug 7292]] - Old code replaced with stuff copied from pathprocess.cpp
	uint2 i;
	for (i = 0 ; i < p_npoints -  1 ; i++)
	{
		// SMR 1913 expand radius for hit testing lines
		p_linesize >>= 1;
		p_linesize *= p_linesize;

		double t_distance;
		t_distance = MCU_squared_distance_from_line(p_points[i]. x, p_points[i] . y, p_points[i + 1] . x, p_points[i + 1] . y, p_x, p_y);
		if (t_distance < p_linesize + (4 * 4))
			return True;
	}
	return False;
}

Boolean MCU_point_in_polygon(MCPoint *p_points, uint2 p_npoints, int2 p_x, int2 p_y)
{
	// SMR 1958 don't do check if no points
	if (p_npoints <= 1)
		return False;
	MCU_offset_points(p_points, p_npoints, -p_x, -p_y);

	MCPoint *endLp = &p_points[p_npoints];
	MCPoint *lp = p_points;

	uint2 ncross = 0;
	int2 sign = lp->y < 0 ? -1 : 1;
	int2 nextSign;
	lp++;
	for(; lp < endLp ; lp++, sign = nextSign)
	{
		nextSign = lp->y < 0 ? -1 : 1;
		if (sign != nextSign)
		{
			if (lp[-1].x > 0)
			{
				if (lp->x > 0)
				{
					ncross++;
					continue;
				}
			}
			else
			{
				if (lp->x < 0)
					continue;
			}
			if (lp[-1].x - lp[-1].y * (lp->x - lp[-1].x) / (lp->y - lp[-1].y) > 0)
				ncross++;
		}
	}
	MCU_offset_points(p_points, p_npoints, p_x, p_y);
	if (ncross & 1)
		return True;
	return False;
}

void MCU_offset_points(MCPoint *points, uint2 npoints, int2 xoff, int2 yoff)
{
	uint2 i;
	for (i = 0 ; i < npoints ; i++)
	{
		if (points[i].x != MININT2)
		{
			points[i].x += xoff;
			points[i].y += yoff;
		}
	}
}

MCRectangle MCU_compute_rect(int2 x1, int2 y1, int2 x2, int2 y2)
{
	MCRectangle drect;

	if (x1 < x2)
	{
		drect.x = x1;
		drect.width = x2 - x1 + 1;
	}
	else
	{
		drect.x = x2;
		drect.width = x1 - x2 + 1;
	}
	if (y1 < y2)
	{
		drect.y = y1;
		drect.height = y2 - y1 + 1;
	}
	else
	{
		drect.y = y2;
		drect.height = y1 - y2 + 1;
	}
	return drect;
}

MCRectangle MCU_center_rect(const MCRectangle &one, const MCRectangle &two)
{
	MCRectangle drect;
	drect.x = one.x + ((one.width - two.width) >> 1);
	drect.y = one.y + ((one.height - two.height) >> 1);
	drect.width = two.width;
	drect.height = two.height;
	return drect;
}

MCRectangle MCU_bound_rect(const MCRectangle &p_srect,
                           int2 p_x, int2 p_y, uint2 p_width, uint2 p_height)
{
	MCRectangle drect = p_srect;
	if (drect.x + drect.width > p_x + p_width)
		drect.x = p_x + p_width - drect.width;
	if (drect.x < p_x)
		drect.x = p_x;
	if (drect.y + drect.height > p_y + p_height)
		drect.y = p_y + p_height - drect.height;
	if (drect.y < p_y)
		drect.y = p_y;
	return drect;
}

MCRectangle MCU_clip_rect(const MCRectangle &p_srect,
                          int2 p_x, int2 p_y, uint2 p_width, uint2 p_height)
{
	MCRectangle drect = p_srect;
	if (p_srect.x < p_x)
	{
		drect.x = p_x;
		if (p_x - p_srect.x > p_srect.width)
			drect.width = 0;
		else
			drect.width -= p_x - p_srect.x;
	}
	if (p_srect.x + p_srect.width > p_x + p_width)
	{
		if (p_srect.x > p_x + p_width)
			drect.width = 0;
		else
			drect.width = p_x + p_width - drect.x;
	}
	if (p_srect.y < p_y)
	{
		drect.y = p_y;
		if (p_y - p_srect.y > p_srect.height)
			drect.height = 0;
		else
			drect.height -= p_y - p_srect.y;
	}
	if (p_srect.y + p_srect.height > p_y + p_height)
	{
		if (p_srect.y > p_y + p_height)
			drect.height = 0;
		else
			drect.height = p_y + p_height - drect.y;
	}
	return drect;
}

MCRectangle MCU_intersect_rect(const MCRectangle &one, const MCRectangle &two)
{
	MCRectangle drect;

	if (one . width == 0 || one . height == 0)
		return one;

	if (two . width == 0 || two . height == 0)
		return two;

	if (one.x > two.x)
		drect.x = one.x;
	else
		drect.x = two.x;
	if (one.y > two.y)
		drect.y = one.y;
	else
		drect.y = two.y;
	if (one.x + one.width > two.x + two.width)
		if (two.x + two.width < drect.x)
			drect.width = 0;
		else
			drect.width = two.x + two.width - drect.x;
	else
		if (one.x + one.width < drect.x)
			drect.width = 0;
		else
			drect.width = one.x + one.width - drect.x;
	if (one.y + one.height > two.y + two.height)
		if (two.y + two.height < drect.y)
			drect.height = 0;
		else
			drect.height = two.y + two.height - drect.y;
	else
		if (one.y + one.height < drect.y)
			drect.height = 0;
		else
			drect.height = one.y + one.height - drect.y;
	return drect;
}

MCRectangle MCU_union_rect(const MCRectangle &one, const MCRectangle &two)
{
	MCRectangle drect;
	int2 lrx, lry;

	if (one . width == 0 || one . height == 0)
		return two;
	else if (two . width == 0 || two . height == 0)
		return one;

	if (one.x + one.width > two.x + two.width)
		lrx = one.x + one.width;
	else
		lrx = two.x + two.width;
	if (one.y + one.height > two.y + two.height)
		lry = one.y + one.height;
	else
		lry = two.y + two.height;
	if (one.x < two.x)
		drect.x = one.x;
	else
		drect.x = two.x;
	if (one.y < two.y)
		drect.y = one.y;
	else
		drect.y = two.y;
	drect.width = lrx - drect.x;
	drect.height = lry - drect.y;
	return drect;
}

MCRectangle MCU_subtract_rect(const MCRectangle &one, const MCRectangle &two)
{
	MCRectangle drect = one;
	if (one.x == two.x && one.width == two.width)
		if (one.y > two.y)
		{
			uint2 overlap = two.y + two.height - one.y;
			drect.height -= overlap;
			drect.y += overlap;
		}
		else
			drect.height -= one.y + one.height - two.y;
	else
		if (one.y == two.y && one.height == two.height)
        {
			if (one.x > two.x)
			{
				uint2 overlap = two.x + two.width - one.x;
				drect.width -= overlap;
				drect.x += overlap;
			}
			else
				drect.width -= one.x + one.width - two.x;
        }
	return drect;
}

MCRectangle MCU_reduce_rect(const MCRectangle &srect, int2 amount)
{
	MCRectangle drect;
	drect.x = srect.x + amount;
	drect.y = srect.y + amount;
	if (amount << 1 > srect.width)
		drect.width = 0;
	else
		drect.width = srect.width - (amount << 1);
	if (amount << 1 > srect.height)
		drect.height = 0;
	else
		drect.height = srect.height - (amount << 1);
	return drect;
}

MCRectangle MCU_scale_rect(const MCRectangle &srect, int2 factor)
{
	MCRectangle drect;
	if (factor < 0)
	{
		factor = -factor;
		drect.x = srect.x / factor;
		drect.y = srect.y / factor;
		drect.width = srect.width / factor;
		drect.height = srect.height / factor;
	}
	else
	{
		drect.x = srect.x * factor;
		drect.y = srect.y * factor;
		drect.width = srect.width * factor;
		drect.height = srect.height * factor;
	}
	return drect;
}

MCRectangle MCU_offset_rect(const MCRectangle& r, int2 ox, int2 oy)
{
	MCRectangle nr;
	nr . x = r . x + ox;
	nr . y = r . y + oy;
	nr . width = r . width;
	nr . height = r . height;
	return nr;
}

MCRectangle MCU_recttoroot(MCStack *sptr, const MCRectangle &orect)
{
	return sptr -> recttoroot(orect);
}

void MCU_getshift(uint4 mask, uint2 &shift, uint2 &outmask)
{
	shift = 0;
	while (shift < 32 && !(mask & 1))
	{
		mask >>= 1;
		shift++;
	}
	uint2 i = shift;
	uint2 j = 0;
	while (i < 32 && mask & 1)
	{
		mask >>= 1;
		i++;
		j++;
	}
	outmask = j;
}

static bool _MCStackNotifyToolChange(MCStack *p_stack, void *p_context)
{
    p_stack -> notifyattachments(kMCStackAttachmentEventToolChanged);
    return true;
}

void MCU_choose_tool(MCExecContext& ctxt, MCStringRef p_input, Tool p_tool)
{
	Tool t_new_tool;
	MColdtool = MCcurtool;
	MCAutoStringRef t_tool_name;
	if (p_tool != T_UNDEFINED)
	{
		t_new_tool = p_tool;
		MCStringCreateWithCString(MCtoolnames[t_new_tool], &t_tool_name);
	}
	else
	{
		t_tool_name = p_input;
		if (MCStringGetLength(*t_tool_name) < 3)
		{
			ctxt . LegacyThrow(EE_CHOOSE_BADTOOL);
			return;
		}
		uint2 i;
        MCRange t_range = MCRangeMake(0, 3);
		for (i = 0 ; i <= T_TEXT ; i++)
            if (MCStringSubstringIsEqualToSubstring(*t_tool_name, t_range, MCSTR(MCtoolnames[i]), t_range, kMCCompareExact))
            {
				t_new_tool = (Tool)i;
				break;
			}
		if (i > T_TEXT)
		{
			ctxt . LegacyThrow(EE_CHOOSE_BADTOOL);
			return;
		}
	}
	if (t_new_tool == MCcurtool)
		return;

	if (MCeditingimage)
		MCeditingimage -> canceldraw();

	MCcurtool = t_new_tool;

	MCundos->freestate();
	if (MCcurtool != T_POINTER)
		MCselected->clear(True);
	if (MCactiveimage && MCcurtool != T_SELECT)
		MCactiveimage->endsel();
	MCeditingimage = nil;
	if (MCactivefield
	        && MCactivefield->getstack()->gettool(MCactivefield) != T_BROWSE)
		MCactivefield->getstack()->kunfocus();
	ctxt . GetObject()->getstack()->resetcursor(True);
	if (MCcurtool == T_BROWSE)
		MCstacks->restartidle();
	if (MCtopstackptr)
		MCtopstackptr->updatemenubar();
    
    MCStacknode *t_node, *t_first_node;
    t_node = t_first_node = MCstacks->topnode();
    while (t_node)
    {
        t_node->getstack()->toolchanged(MCcurtool);
        
        if (t_node->next() == t_first_node)
            t_node = nil;
        else
            t_node = t_node->next();
    }
    
    // MW-2014-04-24: [[ Bug 12249 ]] Prod each player to make sure its buffered correctly for the new tool.
    MCPlayer::SyncPlayers(nil, nil);
    
    MCdispatcher -> foreachstack(_MCStackNotifyToolChange, nil);
    
	ctxt . GetObject()->message_with_valueref_args(MCM_new_tool, *t_tool_name);
}

Exec_stat MCU_dofrontscripts(Handler_type htype, MCNameRef mess, MCParameter *params)
{
	Exec_stat stat = ES_NOT_HANDLED;

	if (MCfrontscripts != NULL)
	{
		MCObjectList *optr = MCfrontscripts;
		do
		{
			if (!optr->getremoved())
			{
				// MW-2011-01-05: Make sure dynamicpath global is sensible.
				Boolean olddynamic = MCdynamicpath;
				MCdynamicpath = MCdynamiccard.IsValid();

				// PASS STATE FIX
				Exec_stat oldstat = stat;
				stat = optr->getobject()->handle(htype, mess, params, nil);
				
				MCdynamicpath = olddynamic;
				
				if (stat != ES_NOT_HANDLED && stat != ES_PASS)
					break;

				if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
					stat = ES_PASS;
			}
			optr = optr->next();
		}
		while (optr != MCfrontscripts);
	}

	return stat;
}
//
//bool MCU_path2std(MCStringRef p_path, MCStringRef& r_stdpath)
//{
//	uindex_t t_length = MCStringGetLength(p_path);
//	if (t_length == 0)
//		return MCStringCopy(p_path, r_stdpath);
//
//	MCAutoNativeCharArray t_path;
//	if (!t_path.New(t_length))
//		return false;
//
//	const char_t *t_src = MCStringGetNativeCharPtr(p_path);
//	char_t *t_dst = t_path.Chars();
//
//	for (uindex_t i = 0; i < t_length; i++)
//	{
//#ifdef _MACOSX
//		if (t_src[i] == '/')
//			t_dst[i] = ':';
//		else if (t_src[i] == ':')
//			t_dst[i] = '/';
//		else
//			t_dst[i] = t_src[i];
//#else
//		if (t_src[i] == '/')
//			t_dst[i] = '\\';
//		else if (t_src[i] == '\\')
//			t_dst[i] = '/';
//		else
//			t_dst[i] = t_src[i];
//#endif
//	}
//
//	return t_path.CreateStringAndRelease(r_stdpath);
//}
//
//void MCU_path2std(char *dptr)
//{
//	if (dptr == NULL || !*dptr)
//		return;
//	do
//	{
//#ifdef _MACOSX
//		if (*dptr == '/')
//			*dptr = ':';
//		else if (*dptr == ':')
//			*dptr = '/';
//#else
//		if (*dptr == '/')
//			*dptr = '\\';
//		else if (*dptr == '\\')
//			*dptr = '/';
//#endif
//	}
//	while (*++dptr);
//}

bool MCU_path2native(MCStringRef p_path, MCStringRef& r_native_path)
{
#ifdef _WIN32
	if (MCStringIsEmpty(p_path))
		return MCStringCopy(p_path, r_native_path);

	unichar_t *t_dst;
	uindex_t t_length;
    t_dst = new (nothrow) unichar_t[t_length + 1];
	t_length = MCStringGetChars(p_path, MCRangeMake(0, t_length), t_dst);

	for (uindex_t i = 0; i < t_length; i++)
	{
		if (t_dst[i] == '/')
			t_dst[i] = '\\';
		else if (t_dst[i] == '\\')
			t_dst[i] = '/';
	}

    return MCStringCreateWithCharsAndRelease(t_dst, t_length, r_native_path);
#else
	return MCStringCopy(p_path, r_native_path);
#endif
}

void MCU_path2native(char *dptr)
{
	if (dptr == NULL || !*dptr)
		return;
#if defined _WIN32
	do
	{
		if (*dptr == '/')
			*dptr = '\\';
		else if (*dptr == '\\')
			*dptr = '/';
	}
	while (*++dptr);
#endif
}

// MW-2004-11-26: Copy null-terminated string at p_src to p_dest, the strings
//   are allowed to overlap.
inline void strmove(char *p_dest, const char *p_src)
{
	while(*p_src != 0)
		*p_dest++ = *p_src++;
	*p_dest = 0;
}

// SN-2014-01-09: Same as above, handling unicode chars
// Returns the characters suppressed in case the string is the same
inline index_t strmove(unichar_t *p_dest, const unichar_t *p_src, bool p_same_string)
{
	while(*p_src != 0)
		*p_dest++ = *p_src++;
	*p_dest = 0;
    
    if (p_same_string)
        return p_src - p_dest;
    else
        return 0;
}

// MW-2004-11-26: Replace strcpy with strmov - overalapping regions (VG)
void MCU_fix_path(MCStringRef in, MCStringRef& r_out)
{
    unichar_t *t_unicode_str;
    uindex_t t_length;
    t_length = MCStringGetLength(in);
    
    t_unicode_str = new (nothrow) unichar_t[t_length + 1];
    t_length = MCStringGetChars(in, MCRangeMake(0, t_length), t_unicode_str);
    t_unicode_str[t_length] = 0;

    unichar_t *fptr = t_unicode_str; //pointer to search forward in curdir
    while (*fptr)
	{
		if (*fptr == '/' && *(fptr + 1) == '.'
		        && *(fptr + 2) == '.' && *(fptr + 3) == '/')
		{//look for "/../" pattern
            if (fptr == t_unicode_str)
				/* Delete "/.." component */
				t_length -= strmove(fptr, fptr + 3, true);
			else
			{
				unichar_t *bptr = fptr - 1;
				while (True)
				{ //search backword for '/'
					if (*bptr == '/')
					{
                        // Leave "/../.." unchanged
                        if (fptr-bptr == 3 && bptr[1] == '.' && bptr[2] == '.')
                        {
                            // Ignore this "/../" sequence and move to next component
                            fptr += 3;
                            break;
                        }
                        
                        /* Delete "/xxx/.." component */
                        t_length -= strmove(bptr, fptr + 3, true);
                        fptr = bptr;
						break;
					}
					else if (bptr == t_unicode_str)
					{
                        // Leave "../../" unchanged
                        if (fptr-bptr == 2 && bptr[0] == '.' && bptr[1] == '.')
                        {
                            // Ignore this "/../" sequence and move to next component
                            fptr += 3;
                            break;
                        }
                        
                        /* Delete "xxx/../" component */
						t_length -= strmove (bptr, fptr + 4, true);
						fptr = bptr;
						break;
					}
					else
						bptr--;
				}
			}
		}
		else
			if (*fptr == '/' && *(fptr + 1) == '.' && *(fptr + 2) == '/')
				t_length -= strmove(fptr, fptr + 2, true); //erase the '/./'
			else
#ifdef _MACOSX
				if (*fptr == '/' && *(fptr + 1) == '/')
#else
                if (fptr != t_unicode_str && *fptr == '/' && *(fptr + 1) == '/')
#endif

					t_length -= strmove(fptr, fptr + 1, true); //erase the extra '/'
				else
					fptr++;
	}
    
    /* UNCHECKED */ MCStringCreateWithChars(t_unicode_str, t_length, r_out);
    delete[] t_unicode_str;
}

bool MCFiltersBase64Encode(MCDataRef p_src, MCStringRef& r_dst);

void MCU_base64encode(MCDataRef in, MCStringRef &out)
{
	/* UNCHECKED */ MCFiltersBase64Encode(in, out);
}

bool MCFiltersBase64Decode(MCStringRef p_src, MCDataRef& r_dst);

void MCU_base64decode(MCStringRef in, MCDataRef &out)
{
	/* UNCHECKED */ MCFiltersBase64Decode(in, out);
}

// SN-2014-12-02": [[ Bug 14015 ]] The fix should only affect the URLs explicitely encoded as UTF-8
bool MCFiltersUrlEncode(MCStringRef p_source, bool p_use_utf8, MCStringRef& r_result);

bool MCU_urlencode(MCStringRef p_url, bool p_use_utf8, MCStringRef &r_encoded)
{
	return MCFiltersUrlEncode(p_url, p_use_utf8, r_encoded);
}

bool MCFiltersUrlDecode(MCStringRef p_source, bool p_use_utf8, MCStringRef& r_result);

void MCU_urldecode(MCStringRef p_source, bool p_use_utf8, MCStringRef& r_result)
{
	/* UNCHECKED */ MCFiltersUrlDecode(p_source, p_use_utf8, r_result);
}

Boolean MCU_freeinserted(MCObjectList *&l)
{
	if (l != NULL)
	{
		MCObjectList *optr = l;
		do
		{
			if (optr->getremoved())
			{
				optr->remove
				(l);
				delete optr;
				return True;
			}
			optr = optr->next();
		}
		while (optr != l);
	}
	return False;
}

void MCU_cleaninserted()
{
	while (MCU_freeinserted(MCbackscripts))
		;
	while (MCU_freeinserted(MCfrontscripts))
		;
}

// MW-2013-06-25: [[ Bug 10983 ]] This function returns true if the given string
//   could be a url. It checks for strings of the form:
//     <letter> (<letter> | <digit> | '+' | '.' | '-')+ ':' <char>+
// MW-2013-07-01: [[ Bug 10975 ]] Update to a MCU_* utility function.
bool MCU_couldbeurl(MCStringRef p_potential_url)
{
    MCAutoStringRefAsCString t_potential_url;
    if (!t_potential_url.Lock(p_potential_url))
        return false;
    const auto&& t_url = t_potential_url.Span();

	// If the first char isn't a letter, then we are done.
	if (t_url.empty() || !isalpha(t_url[0]))
		return false;

    auto t_is_scheme_char = [](const char p_char) {
        return (isalpha(p_char) || isdigit(p_char) ||
                p_char == '+' || p_char == '.' || p_char == '-');
    };
    auto t_colon =
        std::find_if_not(t_url.begin(), t_url.end(), t_is_scheme_char);

    /* If no scheme name or non-scheme characters were found, this
     * could still be a URL. */
    if (t_colon == t_url.end())
        return true;
    /* If there was a non-scheme character, this can't be a URL */
    if (*t_colon != ':')
        return false;
    /* If the scheme name is < 2 characters, or there is nothing after
       the scheme, this can't be a URL. */
    if (t_url.end() - t_colon <= 1 || t_colon - t_url.begin() <= 1)
        return false;

    return true;
}

void MCU_geturl(MCExecContext& ctxt, MCStringRef p_url, MCValueRef &r_output)
// SJT-2014-09-10: [[ URLMessages ]] Send "getURL" messages on all platforms.
{
	MCAutoStringRef t_filename;
	if (MCStringGetLength(p_url) > 5 && MCStringBeginsWithCString(p_url, (const char_t*)"file:", kMCCompareCaseless))
	{
		MCStringCopySubstring(p_url, MCRangeMakeMinMax(5, MCStringGetLength(p_url)), &t_filename);
		MCS_loadtextfile(*t_filename, (MCStringRef&)r_output);
	}
	else if (MCStringGetLength(p_url) > 8 && MCStringBeginsWithCString(p_url, (const char_t*)"binfile:", kMCCompareCaseless))
	{
		MCStringCopySubstring(p_url, MCRangeMakeMinMax(8, MCStringGetLength(p_url)), &t_filename);
		MCS_loadbinaryfile(*t_filename, (MCDataRef&)r_output);
	}
	else if (MCStringGetLength(p_url) > 8 && MCStringBeginsWithCString(p_url, (const char_t*)"resfile:", kMCCompareCaseless))
	{
		MCStringCopySubstring(p_url, MCRangeMakeMinMax(8, MCStringGetLength(p_url)), &t_filename);
		MCS_loadresfile(*t_filename, (MCStringRef&)r_output);
	}
	else if (MCU_couldbeurl(p_url))
	{
		// Send a "getURL" message
		Boolean oldlock = MClockmessages;
		MClockmessages = False;
		MCParameter p1;
		p1 . setvalueref_argument(p_url);
		Exec_stat t_stat = ctxt . GetObject() -> message(MCM_get_url, &p1, True, True);
		MClockmessages = oldlock;

		switch (t_stat) 
		{
		case ES_NOT_HANDLED:
		case ES_PASS:
			// Either there was no message handler, or the handler passed the message,
			// so process the URL in the engine.
			MCS_geturl(ctxt . GetObject(), p_url);
			break;

		case ES_ERROR:
			ctxt . Throw();
			break;

		default:
			break;
		}

		MCurlresult->copyasvalueref((MCValueRef&)r_output);
	}
	else
	{
		// MM-2014-08-12: [[ Bug 2902 ]] Make sure we set the result accordingly if the URL is invalid.
		MCAutoStringRef t_err;
		MCStringFormat(&t_err, "invalid URL: %@", p_url);
		MCresult -> setvalueref(*t_err);
	}

	if (r_output == nil)
		r_output = MCValueRetain(kMCEmptyString);
}

void MCU_puturl(MCExecContext &ctxt, MCStringRef p_url, MCValueRef p_data)
// SJT-2014-09-10: [[ URLMessages ]] Send "putURL" messages on all platforms.
{
	if (MCStringBeginsWithCString(p_url, (const char_t*)"file:", kMCCompareCaseless))
	{
		MCAutoStringRef t_path, t_data;
		/* UNCHECKED */ ctxt . ConvertToString(p_data, &t_data);
		/* UNCHECKED */ MCStringCopySubstring(p_url, MCRangeMakeMinMax(5, MCStringGetLength(p_url)), &t_path);
		MCS_savetextfile(*t_path, *t_data);
	}
	else if (MCStringBeginsWithCString(p_url, (const char_t*)"binfile:", kMCCompareCaseless))
	{
		MCAutoStringRef t_path;
		MCAutoDataRef t_data;
		/* UNCHECKED */ MCStringCopySubstring(p_url, MCRangeMakeMinMax(8, MCStringGetLength(p_url)), &t_path);
		/* UNCHECKED */ ctxt.ConvertToData(p_data, &t_data);
		MCS_savebinaryfile(*t_path, *t_data);
	}
	else if (MCStringBeginsWithCString(p_url, (const char_t*)"resfile:", kMCCompareCaseless))
	{
		MCAutoStringRef t_path;
		MCAutoDataRef t_data;
		/* UNCHECKED */ MCStringCopySubstring(p_url, MCRangeMakeMinMax(8, MCStringGetLength(p_url)), &t_path);
		/* UNCHECKED */ ctxt.ConvertToData(p_data, &t_data);
		MCS_saveresfile(*t_path, *t_data);
	}
	else if (MCU_couldbeurl(p_url))
	{
		MCAutoDataRef t_data;

		// Send "putURL" message
		Boolean oldlock = MClockmessages;
		MClockmessages = False;
		MCParameter p1;
		p1 . setvalueref_argument(p_data);
		MCParameter p2;
		p2 . setvalueref_argument(p_url);
		p1.setnext(&p2);
		Exec_stat t_stat = ctxt . GetObject() -> message(MCM_put_url, &p1, False, True);
		MClockmessages = oldlock;

		switch (t_stat)
		{
		case ES_NOT_HANDLED:
		case ES_PASS:
			// Either there was no message handler, or the handler passed the message,
			// so process the URL in the engine.
			/* UNCHECKED */ ctxt.ConvertToData(p_data, &t_data);
			MCS_putintourl(ctxt.GetObject(), *t_data, p_url);
			break;

		case ES_ERROR:
			ctxt . Throw();
			break;

		default:
			break;
		}
	}
	else
	{
		MCAutoStringRef t_err;
		MCStringFormat(&t_err, "invalid URL: %@", p_url);
		MCresult -> setvalueref(*t_err);
	}
}

////////////////////////////////////////////////////////////////////////////////

struct Language2Charset
{
	MCNameRef* langname;
	Lang_charset charset;
};

static Language2Charset langtocharsets[] = {
	{ &MCN_ansi, LCH_ENGLISH},
	{ &MCN_arabic, LCH_ARABIC },
	{ &MCN_bulgarian, LCH_BULGARIAN },
	{ &MCN_chinese, LCH_CHINESE},
	{ &MCN_english, LCH_ENGLISH},
	{ &MCN_greek, LCH_GREEK },
	{ &MCN_hebrew, LCH_HEBREW},
	{ &MCN_japanese, LCH_JAPANESE },
	{ &MCN_korean, LCH_KOREAN},
	{ &MCN_lithuanian, LCH_LITHUANIAN },
	{ &MCN_polish, LCH_POLISH},
	{ &MCN_roman, LCH_ROMAN },
	{ &MCN_russian, LCH_RUSSIAN },
	{ &MCN_simple_chinese, LCH_SIMPLE_CHINESE},
	{ &MCN_thai, LCH_THAI},
	{ &MCN_turkish, LCH_TURKISH },
	{ &MCN_ukrainian, LCH_UKRAINIAN},
	{ &MCN_unicode, LCH_UNICODE},
	{ &MCN_utf8, LCH_UTF8},
	{ &MCN_vietnamese, LCH_VIETNAMESE },
	{ &MCN_w_char, LCH_UNICODE},
	{ &MCN_asterisk_char, LCH_ENGLISH }
};

MCNameRef MCU_charsettolanguage(uint1 charset)
{
	uint2 i;
	for (i = 0; i < ELEMENTS(langtocharsets); i++)
		if (langtocharsets[i].charset == charset)
			return *langtocharsets[i].langname;
	return kMCEmptyName;
}

uint1 MCU_languagetocharset(MCNameRef p_language)
{
	for (uinteger_t i = 0; i < ELEMENTS(langtocharsets); i++)
		if (MCNameIsEqualToCaseless(p_language, *langtocharsets[i].langname))
			return langtocharsets[i].charset;

	return 0;
}

/* LEGACY */ uint1 MCU_languagetocharset(MCStringRef langname)
{
	MCNewAutoNameRef t_langname;
	/* UNCHECKED */  MCNameCreate(langname, &t_langname);
	return MCU_languagetocharset(*t_langname);
}

//////////

struct  CharSet2WinCharset
{
	int1 charset;
	uint1 wincharset;
};

static CharSet2WinCharset charset2wincharsets[] = {
            { LCH_ENGLISH, 0},
            { LCH_ENGLISH, 1},
            { LCH_JAPANESE, 128 },
            { LCH_CHINESE, 136},
            { LCH_ARABIC, 178},
            { LCH_ARABIC, 179},
            { LCH_ARABIC, 180},
            { LCH_HEBREW, 177},
            { LCH_HEBREW, 181},
            { LCH_GREEK, 161},
            { LCH_KOREAN, 129},
            { LCH_THAI,	222},
            {LCH_RUSSIAN, 204},
            { LCH_SIMPLE_CHINESE, 134}
        };


uint1 MCU_wincharsettocharset(uint2 wincharset)
{
	uint2 i;
	for (i = 0; i < ELEMENTS(charset2wincharsets); i++)
		if (charset2wincharsets[i].wincharset == wincharset)
			return charset2wincharsets[i].charset;
	return 0;
}

uint1 MCU_charsettowincharset(uint1 charset)
{
	uint2 i;
	for (i = 0; i < ELEMENTS(charset2wincharsets); i++)
		if (charset2wincharsets[i].charset == charset)
			return charset2wincharsets[i].wincharset;
	return 0;
}

uint1 MCU_unicodetocharset(uint2 uchar)
{
	if (uchar <= MAXUINT1)
		return LCH_ENGLISH;
	if (uchar >= 0x0080 && uchar <= 0x024F)
		return LCH_GREEK;
	if (uchar >= 0x0600 && uchar <= 0x06FF)
		return LCH_ARABIC;
	if (uchar >= 0x0400 && uchar <= 0x04FF)
		return LCH_RUSSIAN;
	if (uchar >= 0x0370 && uchar <= 0x03FF)
		return LCH_GREEK;
	if (uchar >= 0x0590 && uchar <= 0x05FF)
		return LCH_HEBREW;
	if (uchar >= 0x0E00 && uchar <= 0x0E7F)
		return LCH_THAI;
	if (uchar >= 0x1F00 && uchar <= 0x1FFF)
		return LCH_GREEK;
	if (uchar >= 0xAC00 && uchar <= 0xD7A3)
		return LCH_KOREAN;
	return LCH_JAPANESE;
}

// MW-2005-02-08: New implementation of multibytetounicode
// MW-2006-04-13: Bad me - sign extending p_mbstring in an inappropriate manner caused a little problem!
void MCU_multibytetounicode(const char *p_mbstring, uint4 p_mblength,
                            char *p_buffer, uint4 p_capacity, uint4& r_used, uint1 p_mbcharset)
{
	if (p_mbcharset == LCH_UTF8)
		r_used = UTF8ToUnicode(p_mbstring, p_mblength, (uint2 *)p_buffer, p_capacity);
	else
        r_used = MCsystem ->TextConvert((const void*)p_mbstring, p_mblength, (void*)p_buffer, p_capacity, p_mbcharset, LCH_UNICODE);
}

// MW-2005-02-08: New implementation of unicodetomultibyte
void MCU_unicodetomultibyte(const char *p_ucstring, uint4 p_uclength,
                            char *p_buffer, uint4 p_capacity, uint4& r_used, uint1 p_mbcharset)
{
	if (p_mbcharset == LCH_UTF8)
		r_used = UnicodeToUTF8((uint2 *)p_ucstring, p_uclength, p_buffer, p_capacity);
	else
        r_used = MCsystem ->TextConvert((const void*)p_ucstring, p_uclength, (void*)p_buffer, p_capacity, LCH_UNICODE, p_mbcharset);
}

//////////

bool MCU_multibytetounicode(MCDataRef p_input, uinteger_t p_charset, MCDataRef &r_output)
{
	MCAutoArray<byte_t> t_buffer;
	uint4 t_mb_length, t_uc_length;
	const char *t_mb = (const char*)MCDataGetBytePtr(p_input);
	t_mb_length = MCDataGetLength(p_input);
	
	// How much storage is required for this conversion?
	MCU_multibytetounicode(t_mb, t_mb_length, NULL, 0, t_uc_length, p_charset);
	t_buffer.Resize(t_uc_length);
	
	// Convert the data
	MCU_multibytetounicode(t_mb, t_mb_length, (char*)t_buffer.Ptr(), t_uc_length, t_uc_length, p_charset);
	
	return MCDataCreateWithBytes(t_buffer.Ptr(), t_uc_length, r_output);
}

bool MCU_unicodetomultibyte(MCDataRef p_input, uinteger_t p_charset, MCDataRef &r_output)
{
	MCAutoArray<byte_t> t_buffer;
	uint4 t_mb_length, t_uc_length;
	const char *t_uc = (const char*)MCDataGetBytePtr(p_input);
	t_uc_length = MCDataGetLength(p_input);
	
	// How much storage is required for this conversion?
	MCU_unicodetomultibyte(t_uc, t_uc_length, NULL, 0, t_mb_length, p_charset);
	t_buffer.Resize(t_mb_length);
	
	// Convert the data
	MCU_unicodetomultibyte(t_uc, t_uc_length, (char*)t_buffer.Ptr(), t_mb_length, t_mb_length, p_charset);
	
	return MCDataCreateWithBytes(t_buffer.Ptr(), t_mb_length, r_output);
}

///////////////////////////////////////////////////////////////////////////////

bool MCU_disjointrangeinclude(MCInterval*& x_ranges, int& x_count, int p_from, int p_to)
{
	MCInterval *t_new_ranges;
	t_new_ranges = (MCInterval *)malloc(sizeof(MCInterval) * (x_count + 1));
	if (t_new_ranges == NULL)
		return false;
	
	int t_new_count;
	t_new_count = 0;

	int t_range;
	t_range = 0;

	// Copy all source ranges completely before the new range
	while(t_range < x_count && x_ranges[t_range] . to + 1 < p_from)
		t_new_ranges[t_new_count++] = x_ranges[t_range++];

	int t_new_from;
	t_new_from = t_range < x_count ? MCU_min(x_ranges[t_range] . from, p_from) : p_from;

	// MW-2010-10-18: [[ Bug 9102 ]] Make sure the range after the new one is included if they touch.
	// Iterate though the input while it overlaps the new range
	while(t_range < x_count && x_ranges[t_range] . from - 1 <= p_to)
		t_range++;

	int t_new_to;
	t_new_to = t_range > 0 ? MCU_max(x_ranges[t_range - 1] . to, p_to) : p_to;

	t_new_ranges[t_new_count] . from = t_new_from;
	t_new_ranges[t_new_count] . to = t_new_to;
	t_new_count++;

	while(t_range < x_count)
		t_new_ranges[t_new_count++] = x_ranges[t_range++];

	if (x_ranges != NULL)
		free(x_ranges);

	x_ranges = t_new_ranges;
	x_count = t_new_count;

	return true;
}

bool MCU_disjointrangecontains(MCInterval* p_ranges, int p_count, int p_element)
{
	if (p_count == 0)
		return false;

	unsigned int t_low;
	t_low = 0;

	unsigned int t_high;
	t_high = p_count;

	while(t_low < t_high)
	{
		unsigned int t_mid;
		t_mid = t_low + (t_high - t_low) / 2;

		if (p_element < p_ranges[t_mid] . from)
			t_high = t_mid;
		else if (p_element > p_ranges[t_mid] . to)
			t_low = t_mid + 1;
		else
			return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

// MW-2013-05-02: [[ x64 ]] The 'x_length' parameter is always IO_header::len
//   which is now size_t, so match it.
IO_stat MCU_dofakewrite(char*& x_buffer, size_t& x_length, const void *p_data, uint4 p_size, uint4 p_count)
{
	uint4 t_capacity;
	if (x_length > 65536)
		t_capacity = (x_length + 65535) & ~65535;
	else
		t_capacity = (x_length + 4095) & ~4095;

	uint4 t_new_capacity;
	t_new_capacity = x_length + p_size * p_count;
	if (t_new_capacity > t_capacity)
	{
		if (t_new_capacity >= 65536)
			t_new_capacity = (t_new_capacity + 65535) & ~65535;
		else
			t_new_capacity = (t_new_capacity + 4095) & ~4095;

		char *t_new_buffer;
		t_new_buffer = (char *)realloc(x_buffer, t_new_capacity);
		if (t_new_buffer == NULL)
			return IO_ERROR;

		x_buffer = t_new_buffer;
	}

	memcpy(x_buffer + x_length, p_data, p_size * p_count);
	x_length += p_size * p_count;

	return IO_NORMAL;
}

///////////////////////////////////////////////////////////////////////////////

bool MCString::split(char p_char, MCString& r_head, MCString& r_tail)
{
	const char *t_string;
	uint4 t_length;

	t_string = sptr;
	t_length = length;

	if (!MCU_strchr(t_string, t_length, p_char, False))
	{		
		r_head = *this;
		r_tail . set(NULL, 0);
	}

	r_head . set(sptr, t_string - sptr);
	r_tail . set(t_string + 1, t_length);
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////

MCDictionary::MCDictionary(void)
{
	m_nodes = NULL;
}

MCDictionary::~MCDictionary(void)
{
	while(m_nodes != NULL)
	{
		Node *t_next;
		t_next = m_nodes -> next;
		delete (char *)(m_nodes -> buffer);
		delete m_nodes;
		m_nodes = t_next;
	}
}

void MCDictionary::Set(uint4 p_id, MCString p_value)
{
	Node *t_node;
	t_node = Find(p_id);
	if (t_node == NULL)
	{
		t_node = new (nothrow) Node;
		t_node -> next = m_nodes;
		t_node -> key = p_id;
		m_nodes = t_node;
	}
	else
		free(t_node -> buffer);

	t_node -> buffer = memdup(p_value . getstring(), p_value . getlength());
	t_node -> length = p_value . getlength();
}

bool MCDictionary::Get(uint4 p_id, MCString& r_value)
{
	Node *t_node;
	t_node = Find(p_id);
	if (t_node == NULL)
		return false;

	r_value . set((const char *)t_node -> buffer, t_node -> length);
	return true;
}

void MCDictionary::Pickle(void*& r_buffer, uint4& r_length)
{
	uint4 t_size;
	t_size = 4 + 4 + 4;
	for(Node *t_node = m_nodes; t_node != NULL; t_node = t_node -> next)
		t_size += ((t_node -> length + 3) & ~3) + 8;

	char *t_buffer;
	t_buffer = new (nothrow) char[t_size];
	
	char *t_buffer_ptr;
	t_buffer_ptr = t_buffer;
	((uint4 *)t_buffer_ptr)[0] = MCSwapInt32HostToNetwork('QDCT');
	((uint4 *)t_buffer_ptr)[1] = MCSwapInt32HostToNetwork(t_size);
	((uint4 *)t_buffer_ptr)[2] = 0;
	t_buffer_ptr += 12;

	for(Node *t_node = m_nodes; t_node != NULL; t_node = t_node -> next)
	{
		((uint4 *)t_buffer_ptr)[0] = MCSwapInt32HostToNetwork(t_node -> key);
		((uint4 *)t_buffer_ptr)[1] = MCSwapInt32HostToNetwork(t_node -> length);
		memcpy(t_buffer_ptr + 8, t_node -> buffer, t_node -> length);
		t_buffer_ptr += 8 + (t_node -> length + 3) & ~3;
	}

	((uint4 *)t_buffer)[2] = MCSwapInt32HostToNetwork(Checksum(t_buffer + 12, t_size - 12));

	r_buffer = t_buffer;
	r_length = t_size;
}

bool MCDictionary::Unpickle(const void* p_buffer, uint4 p_length)
{
	if (p_length < 12)
		return false;

	const char *t_buffer;
	t_buffer = (const char *)p_buffer;

	uint4 t_header;
	t_header = MCSwapInt32NetworkToHost(((uint4 *)t_buffer)[0]);

	uint4 t_size;
	t_size = MCSwapInt32NetworkToHost(((uint4 *)t_buffer)[1]);

	uint4 t_checksum;
	t_checksum = MCSwapInt32NetworkToHost(((uint4 *)t_buffer)[2]);

	if (t_header != 'QDCT')
		return false;

	if (t_size != p_length)
		return false;

	t_buffer += 12;
	t_size -= 12;

	if (t_checksum != Checksum(t_buffer, t_size))
		return false;

	while(t_size != 0)
	{
		uint4 t_node_key;
		t_node_key = MCSwapInt32NetworkToHost(((uint4 *)t_buffer)[0]);

		uint4 t_node_size;
		t_node_size = MCSwapInt32NetworkToHost(((uint4 *)t_buffer)[1]);

		t_buffer += 8;
		t_size -= 8;

		if (t_size < t_node_size)
			return false;
		
		Set(t_node_key, MCString(t_buffer, t_node_size));

		t_buffer += (t_node_size + 3) & ~3;
		t_size -= (t_node_size + 3) & ~3;
	}

	return true;
}

MCDictionary::Node *MCDictionary::Find(uint4 p_id)
{
	for(Node *t_node = m_nodes; t_node != NULL; t_node = t_node -> next)
		if (t_node -> key == p_id)
			return t_node;
	return NULL;
}

uint32_t MCDictionary::Checksum(const void *p_data, uint32_t p_length)
{
	uint32_t t_a, t_b;
	t_a = 1;
	t_b = 0;
	
	uint8_t *t_data;
	t_data = (uint8_t *)p_data;
	
	for(uint32_t t_i = 0; t_i < p_length; t_i++, t_data++)
	{
		t_a += *t_data;
		t_b += t_a;
		
		if (t_a > 0xFFFFFF00)
			t_a %= 65521;
		
		if (t_b > 0xFFFFFF00)
			t_b %= 65521;
	}

	return (((t_b % 65521) << 16) | (t_a % 65521));
}

///////////////////////////////////////////////////////////////////////////////

static bool
MCU_path_is_absolute(MCStringRef p_path)
{
#ifdef __WINDOWS__
    if ((MCStringGetLength(p_path) > 1 && MCStringGetCharAtIndex(p_path, 1) == ':') ||
        (MCStringGetLength(p_path) > 2 && MCStringGetCharAtIndex(p_path, 0) == '/' && MCStringGetCharAtIndex(p_path, 1) == '/'))
    {
        return true;
    }
#endif
    
    return MCStringGetCharAtIndex(p_path,
                                  0) == '/';
}

static bool
MCU_path_has_extension(MCStringRef p_path)
{
    uindex_t t_sep;
    if (!MCStringLastIndexOfChar(p_path,
                                 '/',
                                 UINDEX_MAX,
                                 kMCStringOptionCompareExact,
                                 t_sep))
    {
        t_sep = 0;
    }
    
    uindex_t t_ext;
    return MCStringFirstIndexOfChar(p_path,
                                    '.',
                                    t_sep,
                                    kMCStringOptionCompareExact,
                                    t_ext);
}

static void
MCU_path_compute_split_unix(MCStringRef p_path,
                            uindex_t p_split_at,
                            uindex_t& x_dir_end,
                            uindex_t& x_base_start)

{
    if (MCStringGetLength(p_path) > 0 &&
        MCStringGetCharAtIndex(p_path, 0) == '/')
    {
        if (p_split_at == 0)
        {
            /* Make sure dir is / and base is everything after */
            x_dir_end = 1;
        }
    }
    else
    {
        if (p_split_at == 0)
        {
            x_dir_end = 0;
            x_base_start = 0;
        }
    }

    /* Trim any trailing slashes, down to one in the first position */
    while(x_dir_end > 1 &&
          MCStringGetCharAtIndex(p_path, x_dir_end - 1) == '/')
    {
        x_dir_end -= 1;
    }
}

static void
MCU_path_compute_split_win32(MCStringRef p_path,
                             uindex_t p_split_at,
                             uindex_t& x_dir_end,
                             uindex_t& x_base_start)
{
   if (MCStringBeginsWithCString(p_path,
                                 (const char_t *)"//",
                                 kMCStringOptionCompareExact))
    {
        /* UNC */
        
        uindex_t t_end_of_share = 0;
        uindex_t t_end_of_folder = 0;
        if (MCStringFirstIndexOfChar(p_path,
                                     '/',
                                     2,
                                     kMCStringOptionCompareExact,
                                     t_end_of_share))
        {
            if (!MCStringFirstIndexOfChar(p_path,
                                          '/',
                                          t_end_of_share + 1,
                                          kMCStringOptionCompareExact,
                                          t_end_of_folder))
            {
                t_end_of_folder = UINDEX_MAX;
            }
        }
        else
        {
            t_end_of_folder = UINDEX_MAX;
        }
        
        if (t_end_of_folder >= p_split_at)
        {
            x_dir_end = t_end_of_folder;
            if (t_end_of_folder != UINDEX_MAX)
            {
                x_base_start = t_end_of_folder + 1;
            }
            else
            {
                x_base_start = UINDEX_MAX;
            }
        }
        
        /* Trim any slashes down to the end of the UNC folder component */
        while(x_dir_end > t_end_of_folder &&
              MCStringGetCharAtIndex(p_path, x_dir_end - 1) == '/')
        {
            x_dir_end -= 1;
        }
    }
    else if (MCStringGetLength(p_path) > 1 &&
             MCStringGetCharAtIndex(p_path, 1) == ':')
    {
        /* DRIVE */
    
        if (MCStringGetLength(p_path) > 2 &&
            MCStringGetCharAtIndex(p_path, 2) == '/')
        {
            /* DRIVE:/ (absolute) */
        
            if (p_split_at == 2)
            {
                /* Make sure we include the / after the drive in the dir */
                x_dir_end = p_split_at + 1;
            }
        }
        else
        {
            /* DRIVE: (DRIVE relative) */
        
            if (p_split_at == 0)
            {
                /* Make sure dir is DRIVE: and base is everything after */
                x_dir_end = 2;
                x_base_start = 2;
            }
        }
        
        /* Trim any trailing slashes, down to one in the third position */
        while(x_dir_end > 3 &&
              MCStringGetCharAtIndex(p_path, x_dir_end - 1) == '/')
        {
            x_dir_end -= 1;
        }
    }
    else
    {
        MCU_path_compute_split_unix(p_path,
                                    p_split_at,
                                    x_dir_end,
                                    x_base_start);
    }
}

static bool
MCU_path_split(MCStringRef p_path,
               MCStringRef* r_dir,
               MCStringRef* r_base,
               bool p_win32)
{
    uindex_t t_split_at = 0;
    if (!MCStringLastIndexOfChar(p_path,
                                 '/',
                                 UINDEX_MAX,
                                 kMCStringOptionCompareExact,
                                 t_split_at))
    {
        t_split_at = 0;
    }
    
    uindex_t t_dir_end = t_split_at;
    uindex_t t_base_start = t_split_at + 1;

    if (p_win32)
    {
        MCU_path_compute_split_win32(p_path,
                                     t_split_at,
                                     t_dir_end,
                                     t_base_start);
    }
    else
    {
        MCU_path_compute_split_unix(p_path,
                                    t_split_at,
                                    t_dir_end,
                                    t_base_start);
    }

    if (r_dir != nullptr)
    {
        if (!MCStringCopySubstring(p_path,
                                   MCRangeMakeMinMax(0, t_dir_end),
                                   *r_dir))
        {
            return false;
        }
    }
    
    if (r_base != nullptr)
    {
        if (!MCStringCopySubstring(p_path,
                                   MCRangeMakeMinMax(t_base_start, UINDEX_MAX),
                                   *r_base))
        {
            return false;
        }
    }
    
    return true;
}

bool MCU_path_split(MCStringRef p_path,
                    MCStringRef* r_dir,
                    MCStringRef* r_base)
{
#ifdef __WINDOWS__
    return MCU_path_split(p_path, r_dir, r_base, true);
#else
    return MCU_path_split(p_path, r_dir, r_base, false);
#endif
}

bool MCU_path_split_unix(MCStringRef p_path,
                         MCStringRef* r_dir,
                         MCStringRef* r_base)
{
    return MCU_path_split(p_path, r_dir, r_base, false);
}

bool MCU_path_split_win32(MCStringRef p_path,
                          MCStringRef* r_dir,
                          MCStringRef* r_base)
{
    return MCU_path_split(p_path, r_dir, r_base, true);
}

///////////////////////////////////////////////////////////////////////////////

static bool
__MCU_library_load_verbatim(MCStringRef p_path,
                            MCSLibraryRef& r_library)
{
    if (!MCSLibraryCreateWithPath(p_path,
                                  r_library))
    {
        MCAutoErrorRef t_error;
        MCErrorCatch(&t_error);
        MCLog("MCU_library_load failed for %@", p_path);
        return false;
    }
    
    return true;
}

static bool
__MCU_library_load_adding_extension(MCStringRef p_path,
                                    const char *p_extension,
                                    MCSLibraryRef& r_library)
{
    MCAutoStringRef t_library_path;
    if (!MCStringFormat(&t_library_path,
                        "%@.%s",
                        p_path,
                        p_extension))
    {
        return false;
    }
    return __MCU_library_load_verbatim(*t_library_path,
                                       r_library);
}

static bool
__MCU_library_map_path(MCStringRef p_path,
                       MCStringRef& r_mapped_library_path)
{
    // If the path does not begin with './', then we are done.
    if (!MCStringBeginsWithCString(p_path,
                                   reinterpret_cast<const char_t *>("./"),
                                   kMCStringOptionCompareExact))
    {
        r_mapped_library_path = MCValueRetain(p_path);
        return true;
    }
    
    // Extract the base path (i.e. remove the ./)
    MCAutoStringRef t_base_path;
    if (!MCStringCopySubstring(p_path,
                               MCRangeMake(2, UINDEX_MAX),
                               &t_base_path))
    {
        return false;
    }
    
    // Apply any mappings which are present.
    MCAutoStringRef t_mapped_path;
    if (MCdispatcher == nullptr ||
        !MCdispatcher->fetchlibrarymapping(*t_base_path,
                                           &t_mapped_path))
    {
        // If there is no mapping, restore the original path including ./
        // if it was there.
        t_mapped_path = p_path;
    }
    
    // If the mapped path does not begin with './', then we just resolve.
    MCAutoStringRef t_unresolved_library_path;
    if (MCStringBeginsWithCString(*t_mapped_path,
                                  reinterpret_cast<const char_t *>("./"),
                                  kMCStringOptionCompareExact))
    {
        
        // Concatenate the mapped path onto the app code path
        if (!MCStringFormat(&t_unresolved_library_path,
                            "%@/%@",
                            MCappcodepath,
                            *t_mapped_path))
        {
            return false;
        }
    }
    else
    {
        t_unresolved_library_path = *t_mapped_path;
    }
    
    // resolve the oath to ensure that all '.' and '..' type components are
    // removed. (otherwise things might go awry if we have a //?/ type path on
    // Windows).
    if (!MCS_resolvepath(*t_unresolved_library_path,
                         r_mapped_library_path))
    {
        return false;
    }
    
    return true;
}

MCSLibraryRef MCU_library_load(MCStringRef p_path)
{
    // If the path is not absolute, apply the internal mapping to the name.
    // This uses any mapping section in the standalone capsule, and ensures
    // that revsecurity and revpdfprinter map correctly.
    MCAutoStringRef t_library_path;
    if (!__MCU_library_map_path(p_path,
                                &t_library_path))
    {
        MCAutoErrorRef t_error;
        MCErrorCatch(&t_error);
        return nullptr;
    }

    MCLog("MCU_library_load %@ -> %@",
        p_path,
        *t_library_path);
    
    // If the path already has an extension, we don't need to add one. Otherwise
    // we try the various appropriate extensions per-platform.
    MCSAutoLibraryRef t_library;
    if (MCU_path_has_extension(*t_library_path))
    {
        __MCU_library_load_verbatim(*t_library_path,
                                    &t_library);
    }
    else
    {
#if defined(__MAC__) || defined(__IOS__)
        __MCU_library_load_adding_extension(*t_library_path,
                                            "framework",
                                            &t_library);
        if (!t_library.IsSet())
            __MCU_library_load_adding_extension(*t_library_path,
                                                "bundle",
                                                &t_library);
        if (!t_library.IsSet())
            __MCU_library_load_adding_extension(*t_library_path,
                                                "dylib",
                                                &t_library);
#elif defined(__WINDOWS__)
        __MCU_library_load_adding_extension(*t_library_path,
                                            "dll",
                                            &t_library);
#elif defined(__LINUX__) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
        __MCU_library_load_adding_extension(*t_library_path,
                                            "so",
                                            &t_library);
#else
#       error MCU_library_load not implemented for this platform
#endif
    }
    
    return t_library.Take();
}

void MCU_library_unload(MCSLibraryRef p_module)
{
    if (p_module != nullptr)
        MCValueRelease(p_module);
}

void *MCU_library_lookup(MCSLibraryRef p_module,
                         MCStringRef p_symbol)
{
    return MCSLibraryLookupSymbol(p_module,
                                  p_symbol);
}

//////////

void *MCSupportLibraryLoad(const char *p_name_cstr)
{
    MCAutoStringRef t_name;
    if (!MCStringCreateWithBytes(reinterpret_cast<const byte_t *>(p_name_cstr),
                                 strlen(p_name_cstr),
                                 kMCStringEncodingUTF8,
                                 false,
                                 &t_name))
    {
        return nullptr;
    }

    MCSAutoLibraryRef t_module;
    &t_module = MCU_library_load(*t_name);
    if (!t_module.IsSet())
    {
        // try a relative path
        MCAutoStringRef t_relative_filename;
        if (MCStringFormat(&t_relative_filename, "./%@", *t_name))
        {
            &t_module = MCU_library_load(*t_relative_filename);
        }
    }
    
    if (!t_module.IsSet())
    {
        return nullptr;
    }
    
    return t_module.Take();
}

void MCSupportLibraryUnload(void *p_handle)
{
    MCU_library_unload(static_cast<MCSLibraryRef>(p_handle));
}

char *MCSupportLibraryCopyNativePath(void *p_handle)
{
    MCAutoStringRef t_path;
    if (!MCSLibraryCopyPath(static_cast<MCSLibraryRef>(p_handle),
                            &t_path))
    {
        return nullptr;
    }

    MCAutoStringRef t_native_path;
    if (!MCS_pathtonative(*t_path,
                          &t_native_path))
    {
        return nullptr;
    }

    char *t_path_str;
    if (!MCStringConvertToUTF8String(*t_native_path,
                                     t_path_str))
    {
        return nullptr;
    }

    return t_path_str;
}

void *MCSupportLibraryLookupSymbol(void *p_handle,
                                   const char *p_symbol_cstr)
{
    MCAutoStringRef t_symbol;
    if (!MCStringCreateWithBytes(reinterpret_cast<const byte_t *>(p_symbol_cstr),
                                 strlen(p_symbol_cstr),
                                 kMCStringEncodingUTF8,
                                 false,
                                 &t_symbol))
    {
        return nullptr;
    }
 
    return MCU_library_lookup(static_cast<MCSLibraryRef>(p_handle),
                              *t_symbol);
}

///////////////////////////////////////////////////////////////////////////////

bool
MCU_is_token(MCStringRef p_string)
{
	MCScriptPoint sp(p_string);

	++MCerrorlock;

	Parse_stat ps = sp.nexttoken();

	--MCerrorlock;

	if (ps == PS_ERROR || ps == PS_EOF)
	{
		return false;
	}

	/* Check that token is located at start of query string */
	if (sp.getindex() != 0)
	{
		return false;
	}

	/* Check that token spans full length of query string */
	if (MCStringGetLength(p_string) != MCStringGetLength(sp.gettoken_stringref()))
	{
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

// Color conversion utilities

bool MCU_format_color(const MCColor p_color, MCStringRef& r_string)
{
    return MCStringFormat(r_string, "%d,%d,%d", p_color.red >> 8, p_color.green >> 8, p_color.blue >> 8);
}
