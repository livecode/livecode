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

#ifndef __MC_UNICODE__
#define __MC_UNICODE__

// Returns true if the given codepoint has a non-zero combining class, this
// is not inlined as it requires table lookup and looping.
extern bool MCUnicodeCodepointIsCombiner(uint4 x);

extern uint4 MCUnicodeMapFromNativeSingleton_Windows1252(unsigned char x);
extern uint4 MCUnicodeMapFromNativeSingleton_MacRoman(unsigned char x);
extern uint4 MCUnicodeMapFromNativeSingleton_ISO8859_1(unsigned char x);

extern bool MCUnicodeMapToNativeSingleton_Windows1252(uint4 x, uint1& r_char);
extern bool MCUnicodeMapToNativeSingleton_MacRoman(uint4 x, uint1& r_char);
extern bool MCUnicodeMapToNativeSingleton_ISO8859_1(uint4 x, uint1& r_char);

extern bool MCUnicodeMapToNativePair_Windows1252(uint4 x, uint4 y, uint1& r_char);
extern bool MCUnicodeMapToNativePair_MacRoman(uint4 x, uint4 y, uint1& r_char);
extern bool MCUnicodeMapToNativePair_ISO8859_1(uint4 x, uint4 y, uint1& r_char);

extern bool MCUnicodeCodepointIsIdeographicLookup(uint4 x);

// Returns true if there's a word break between x and y with the given context.
// Note that this is only a crude approximation to the full Unicode rules.
extern bool MCUnicodeCanBreakWordBetween(uint4 xc, uint4 x, uint4 y, uint4 yc);

////////////////////////////////////////////////////////////////////////////////

// Returns true if the given codepoint should be treated as ideographic when
// breaking
inline bool MCUnicodeCodepointIsIdeographic(uint4 x)
{
	if (x < 0x0E01)
		return false;
	return MCUnicodeCodepointIsIdeographicLookup(x);
}

// Returns the break class of the codepoint. If Bit 0 is set, breaking is
// prohibited before, it Bit 1 is set, breaking is prohibited after
extern uint4 MCUnicodeCodepointGetBreakClass(uint4 x);

// Returns true if the given codepoint is in the high surrogate area
inline bool MCUnicodeCodepointIsHighSurrogate(uint4 x)
{
	if (x < 0xD800)
		return false;

	if (x > 0xDBFF)
		return false;

	return true;
}

// Returns true if the given codepoint is in the low surrogate area
inline bool MCUnicodeCodepointIsLowSurrogate(uint4 x)
{
	if (x < 0xDC00)
		return false;
	if (x > 0xDFFF)
		return false;
	return true;
}

// Returns true if the given codepoint is a base character
inline bool MCUnicodeCodepointIsBase(uint4 x)
{
	if (x < 0x0300)
		return true;

	return !MCUnicodeCodepointIsCombiner(x);
}

// Compute and advance the current surrogate pair (used by MCUnicodeCodepointAdvance to
// help the compiler make good choices about inlining - effectively a 'trap' to a very
// rare case).
inline uint4 MCUnicodeCodepointAdvanceSurrogate(const uint2* p_input, uint4 p_length, uint4& x_index)
{
	if (x_index + 1 < p_length && MCUnicodeCodepointIsLowSurrogate(p_input[1]))
	{
		uint4 t_codepoint;
		t_codepoint = ((p_input[x_index] - 0xD800) << 10) | (p_input[x_index + 1] - 0xDC00);
		x_index += 2;
		return t_codepoint;
	}

	uint4 t_codepoint;
	t_codepoint = p_input[x_index];

	x_index += 1;

	return t_codepoint;
}

// Consume and return the next codepoint - this automatically combines surrogates. If the
// surrogate is invalid it is still returned (i.e. the expectation is it will be processed
// as an illegal character).
inline uint4 MCUnicodeCodepointAdvance(const uint2* p_input, uint4 p_length, uint4& x_index)
{
	uint4 t_codepoint;
	t_codepoint = p_input[x_index];

	if (MCUnicodeCodepointIsHighSurrogate(t_codepoint))
		return MCUnicodeCodepointAdvanceSurrogate(p_input, p_length, x_index);

	x_index += 1;

	return t_codepoint;
}

////////////////////////////////////////////////////////////////////////////////

inline bool MCUnicodeMapToNative_Windows1252(const uint2 *p_codepoints, uint4 p_length, uint1& r_char)
{
	if (p_length == 1)
	{
		if (p_codepoints[0] <= 0x7F || (p_codepoints[0] >= 0xA0 && p_codepoints[0] <= 0xFF))
		{
			r_char = (uint1)p_codepoints[0];
			return true;
		}
		return MCUnicodeMapToNativeSingleton_Windows1252(p_codepoints[0], r_char);
	}

	if (p_length == 2)
	{
		if (p_codepoints[0] < 'A' || p_codepoints[0] > 'z')
			return false;

		if (p_codepoints[1] < 0x0300 || p_codepoints[1] > 0x0327)
			return false;

		return MCUnicodeMapToNativePair_Windows1252(p_codepoints[0], p_codepoints[1], r_char);
	}

	return false;
}

inline bool MCUnicodeMapToNative_MacRoman(const uint2 *p_codepoints, uint4 p_length, uint1& r_char)
{
	if (p_length == 1)
	{
		if (p_codepoints[0] <= 0x7f)
		{
			r_char = (uint1)p_codepoints[0];
			return true;
		}
		
		return MCUnicodeMapToNativeSingleton_MacRoman(p_codepoints[0], r_char);
	}
	
	if (p_length == 2)
	{
		if (p_codepoints[0] < 'A' || p_codepoints[0] > 'z')
			return false;

		if (p_codepoints[1] < 0x0300 || p_codepoints[1] > 0x0327)
			return false;

		return MCUnicodeMapToNativePair_MacRoman(p_codepoints[0], p_codepoints[1], r_char);
	}

	return false;
}

inline bool MCUnicodeMapToNative_ISO8859_1(const uint2 *p_codepoints, uint4 p_length, uint1& r_char)
{
	if (p_length == 1)
	{
		if (p_codepoints[0] <= 0xff)
		{
			r_char = (uint1)p_codepoints[0];
			return true;
		}
		
		return false;
	}

	if (p_length == 2)
	{
		if (p_codepoints[0] < 'A' || p_codepoints[0] > 'y')
			return false;

		if (p_codepoints[1] < 0x0300 || p_codepoints[1] > 0x0327)
			return false;

		return MCUnicodeMapToNativePair_ISO8859_1(p_codepoints[0], p_codepoints[1], r_char);
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

inline uint4 MCUnicodeMapFromNative_Windows1252(uint1 p_native)
{
	if (p_native <= 0x7F)
		return p_native;
	if (p_native >= 0xA0)
		return p_native;
	return MCUnicodeMapFromNativeSingleton_Windows1252(p_native);
}

inline uint4 MCUnicodeMapFromNative_MacRoman(uint1 p_native)
{
	if (p_native <= 0x7F)
		return p_native;
	return MCUnicodeMapFromNativeSingleton_MacRoman(p_native);
}

inline uint4 MCUnicodeMapFromNative_ISO8859_1(uint1 p_native)
{
	return p_native;
}

////////////////////////////////////////////////////////////////////////////////

#if defined(__WINDOWS_1252__)
#define MCUnicodeMapFromNative(x) MCUnicodeMapFromNative_Windows1252(x)
#define MCUnicodeMapToNative(x, y, z) MCUnicodeMapToNative_Windows1252(x, y, z)
#elif defined(__MACROMAN__)
#define MCUnicodeMapFromNative(x) MCUnicodeMapFromNative_MacRoman(x)
#define MCUnicodeMapToNative(x, y, z) MCUnicodeMapToNative_MacRoman(x, y, z)
#elif defined(__ISO_8859_1__)
#define MCUnicodeMapFromNative(x) MCUnicodeMapFromNative_ISO8859_1(x)
#define MCUnicodeMapToNative(x, y, z) MCUnicodeMapToNative_ISO8859_1(x, y, z)
#else
#error Unknown native text encoding.
#endif

////////////////////////////////////////////////////////////////////////////////

#endif
