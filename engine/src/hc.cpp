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

#include "aclip.h"
#include "font.h"
#include "util.h"
#include "cdata.h"
#include "button.h"
#include "field.h"
#include "paragraf.h"
#include "image.h"
#include "scrolbar.h"
#include "group.h"
#include "objptr.h"
#include "card.h"
#include "stack.h"
#include "hndlrlst.h"
#include "hc.h"

#include "exec-interface.h"

#include "globals.h"

static uint4 maxid;
static uint4 version;

static uint2 iconx, icony;
static uint2 cursorx, cursory;

static const uint2 hc_icons[HC_NICONS] =
    {
        24694, 2002, 1000, 1001, 1002, 1003,
        1004, 1005, 1006, 1007, 1008, 1019, 26884, 18814, 27056, 15420,
        16560, 6720, 16692, 3584, 24317, 29903, 1014, 1013, 1012, 29019,
        2730, 30557, 26865, 9301, 27009, 2162, 32488, 2335, 5472, 766, 902,
        26425, 29114, 4895, 6724, 21449, 24830, 17779, 8419, 7417, 26020,
        15279, 19381, 22308, 14953, 6464, 6179, 3835, 29484, 9120, 19162,
        1016, 32650, 1011, 11045, 20098, 21700, 20689, 21847, 1017, 10610,
        20696, 17481, 3430, 11645, 4432, 20965, 17357, 21209, 8961, 22855,
        4263, 15972, 20186, 32670, 26635, 25002, 32462, 21060, 2507, 31685,
        1020, 23078, 19678, 2478, 14767, 1018, 1015, 1009, 8538, 9761, 7012,
        13149, 16344, 17343, 12195, 28023, 28024, 28022, 18222, 18223, 290,
        25309, 24753, 21437, 31885, 29589, 16735, 21711, 17214, 6544, 11260,
        7142, 18607, 21573, 21574, 21575, 21576, 24694, 6560, 6491, 12722,
        24081, 27328, 22978, 15993, 2181, 13744, 13745, 8323, 10935, 17838,
        17896, 17890, 17937, 23613, 8348, 8347, 8979, 8964, 8980, 6044,
        6043, 28811, 28810, 8350, 8349, 23717, 23718, 29654, 27774, 3071,
        3358, 27969, 9104, 10181, 12411, 19638, 17264, 11714, 17169, 3333,
        11216, 16321, 2101, 2102, 2103, 2104, 2105, 2106, 26665, 49012,
        59013, 49014, 49044, 49045, 49046, 49047, 49048, 49049, 49056,
        49124, 65535, 1, 2, 59457 };

static const uint2 mc_icons[HC_NICONS] =
    {
        330, 361, 344, 344, 379, 379, 342, 343,
        359, 335, 336, 359, 379, 339, 356, 320, 321, 396, 325, 324, 319,
        322, 320, 321, 396, 325, 324, 319, 322, 320, 321, 396, 325, 324,
        319, 322, 320, 321, 396, 325, 324, 319, 322, 367, 368, 396, 365,
        366, 367, 368, 396, 601, 602, 601, 602, 601, 602, 601, 602, 351,
        352, 350, 353, 352, 353, 387, 388, 387, 345, 345, 346, 337, 369,
        369, 338, 338, 338, 338, 337, 348, 344, 330, 330, 330, 361, 361,
        361, 361, 330, 362, 363, 364, 370, 328, 354, 378, 370, 358, 330,
        395, 380, 342, 360, 360, 360, 358, 358, 395, 383, 339, 371, 343,
        330, 357, 344, 356, 605, 606, 395, 389, 805, 806, 807, 808, 361,
        384, 389, 338, 341, 350, 350, 350, 353, 352, 352, 352, 352, 387,
        388, 387, 387, 388, 321, 320, 368, 367, 361, 321, 320, 368, 367,
        368, 367, 368, 367, 378, 370, 304, 385, 368, 368, 348, 361, 361,
        330, 330, 330, 330, 330, 330, 372, 373, 374, 375, 376, 377, 394,
        386, 394, 386, 394, 394, 393, 392, 391, 390, 339, 348, 605, 604,
        331, 349 };

static const uint1 hqx[82] =
    {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0, 0, 13,
        14, 15, 16, 17, 18, 19, 0, 20, 21, 0, 0, 0, 0, 0,
        0, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
        34, 35, 36, 0, 37, 38, 39, 40, 41, 42, 43, 0, 44,
        45, 46, 47, 0, 0, 0, 0, 48, 49, 50, 51,52, 53, 54,
        0, 55, 56, 57, 58, 59, 60, 0, 0, 61, 62, 63};

static const uint1 h1[256] =
    {
        0x00, 0x01, 0x03, 0x02, 0x07, 0x06, 0x04, 0x05,
        0x0f, 0x0e, 0x0c, 0x0d, 0x08, 0x09, 0x0b, 0x0a, 0x1f, 0x1e, 0x1c,
        0x1d, 0x18, 0x19, 0x1b, 0x1a, 0x10, 0x11, 0x13, 0x12, 0x17, 0x16,
        0x14, 0x15, 0x3f, 0x3e, 0x3c, 0x3d, 0x38, 0x39, 0x3b, 0x3a, 0x30,
        0x31, 0x33, 0x32, 0x37, 0x36, 0x34, 0x35, 0x20, 0x21, 0x23, 0x22,
        0x27, 0x26, 0x24, 0x25, 0x2f, 0x2e, 0x2c, 0x2d, 0x28, 0x29, 0x2b,
        0x2a, 0x7f, 0x7e, 0x7c, 0x7d, 0x78, 0x79, 0x7b, 0x7a, 0x70, 0x71,
        0x73, 0x72, 0x77, 0x76, 0x74, 0x75, 0x60, 0x61, 0x63, 0x62, 0x67,
        0x66, 0x64, 0x65, 0x6f, 0x6e, 0x6c, 0x6d, 0x68, 0x69, 0x6b, 0x6a,
        0x40, 0x41, 0x43, 0x42, 0x47, 0x46, 0x44, 0x45, 0x4f, 0x4e, 0x4c,
        0x4d, 0x48, 0x49, 0x4b, 0x4a, 0x5f, 0x5e, 0x5c, 0x5d, 0x58, 0x59,
        0x5b, 0x5a, 0x50, 0x51, 0x53, 0x52, 0x57, 0x56, 0x54, 0x55, 0xff,
        0xfe, 0xfc, 0xfd, 0xf8, 0xf9, 0xfb, 0xfa, 0xf0, 0xf1, 0xf3, 0xf2,
        0xf7, 0xf6, 0xf4, 0xf5, 0xe0, 0xe1, 0xe3, 0xe2, 0xe7, 0xe6, 0xe4,
        0xe5, 0xef, 0xee, 0xec, 0xed, 0xe8, 0xe9, 0xeb, 0xea, 0xc0, 0xc1,
        0xc3, 0xc2, 0xc7, 0xc6, 0xc4, 0xc5, 0xcf, 0xce, 0xcc, 0xcd, 0xc8,
        0xc9, 0xcb, 0xca, 0xdf, 0xde, 0xdc, 0xdd, 0xd8, 0xd9, 0xdb, 0xda,
        0xd0, 0xd1, 0xd3, 0xd2, 0xd7, 0xd6, 0xd4, 0xd5, 0x80, 0x81, 0x83,
        0x82, 0x87, 0x86, 0x84, 0x85, 0x8f, 0x8e, 0x8c, 0x8d, 0x88, 0x89,
        0x8b, 0x8a, 0x9f, 0x9e, 0x9c, 0x9d, 0x98, 0x99, 0x9b, 0x9a, 0x90,
        0x91, 0x93, 0x92, 0x97, 0x96, 0x94, 0x95, 0xbf, 0xbe, 0xbc, 0xbd,
        0xb8, 0xb9, 0xbb, 0xba, 0xb0, 0xb1, 0xb3, 0xb2, 0xb7, 0xb6, 0xb4,
        0xb5, 0xa0, 0xa1, 0xa3, 0xa2, 0xa7, 0xa6, 0xa4, 0xa5, 0xaf, 0xae,
        0xac, 0xad, 0xa8, 0xa9, 0xab, 0xaa, };

static uint1 h2[256] =
    {
        0x00, 0x01, 0x02, 0x03, 0x05, 0x04, 0x07, 0x06,
        0x0a, 0x0b, 0x08, 0x09, 0x0f, 0x0e, 0x0d, 0x0c, 0x15, 0x14, 0x17,
        0x16, 0x10, 0x11, 0x12, 0x13, 0x1f, 0x1e, 0x1d, 0x1c, 0x1a, 0x1b,
        0x18, 0x19, 0x2a, 0x2b, 0x28, 0x29, 0x2f, 0x2e, 0x2d, 0x2c, 0x20,
        0x21, 0x22, 0x23, 0x25, 0x24, 0x27, 0x26, 0x3f, 0x3e, 0x3d, 0x3c,
        0x3a, 0x3b, 0x38, 0x39, 0x35, 0x34, 0x37, 0x36, 0x30, 0x31, 0x32,
        0x33, 0x55, 0x54, 0x57, 0x56, 0x50, 0x51, 0x52, 0x53, 0x5f, 0x5e,
        0x5d, 0x5c, 0x5a, 0x5b, 0x58, 0x59, 0x40, 0x41, 0x42, 0x43, 0x45,
        0x44, 0x47, 0x46, 0x4a, 0x4b, 0x48, 0x49, 0x4f, 0x4e, 0x4d, 0x4c,
        0x7f, 0x7e, 0x7d, 0x7c, 0x7a, 0x7b, 0x78, 0x79, 0x75, 0x74, 0x77,
        0x76, 0x70, 0x71, 0x72, 0x73, 0x6a, 0x6b, 0x68, 0x69, 0x6f, 0x6e,
        0x6d, 0x6c, 0x60, 0x61, 0x62, 0x63, 0x65, 0x64, 0x67, 0x66, 0xaa,
        0xab, 0xa8, 0xa9, 0xaf, 0xae, 0xad, 0xac, 0xa0, 0xa1, 0xa2, 0xa3,
        0xa5, 0xa4, 0xa7, 0xa6, 0xbf, 0xbe, 0xbd, 0xbc, 0xba, 0xbb, 0xb8,
        0xb9, 0xb5, 0xb4, 0xb7, 0xb6, 0xb0, 0xb1, 0xb2, 0xb3, 0x80, 0x81,
        0x82, 0x83, 0x85, 0x84, 0x87, 0x86, 0x8a, 0x8b, 0x88, 0x89, 0x8f,
        0x8e, 0x8d, 0x8c, 0x95, 0x94, 0x97, 0x96, 0x90, 0x91, 0x92, 0x93,
        0x9f, 0x9e, 0x9d, 0x9c, 0x9a, 0x9b, 0x98, 0x99, 0xff, 0xfe, 0xfd,
        0xfc, 0xfa, 0xfb, 0xf8, 0xf9, 0xf5, 0xf4, 0xf7, 0xf6, 0xf0, 0xf1,
        0xf2, 0xf3, 0xea, 0xeb, 0xe8, 0xe9, 0xef, 0xee, 0xed, 0xec, 0xe0,
        0xe1, 0xe2, 0xe3, 0xe5, 0xe4, 0xe7, 0xe6, 0xd5, 0xd4, 0xd7, 0xd6,
        0xd0, 0xd1, 0xd2, 0xd3, 0xdf, 0xde, 0xdd, 0xdc, 0xda, 0xdb, 0xd8,
        0xd9, 0xc0, 0xc1, 0xc2, 0xc3, 0xc5, 0xc4, 0xc7, 0xc6, 0xca, 0xcb,
        0xc8, 0xc9, 0xcf, 0xce, 0xcd, 0xcc, };

static uint1 patbytes[8] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};

void hcstat_append(const char *msg, ...)
{
	// Build the new message string (msg is printf format string)
	va_list t_args;
	va_start(t_args, msg);
	MCAutoStringRef t_new_line;
	/* UNCHECKED */ MCStringFormatV(&t_new_line, msg, t_args);
	va_end(t_args);
	
	// Turn the current MChcstat stringref into a mutable stringref
	MCAutoStringRef t_mutable_hcstat;
	/* UNCHECKED */ MCStringMutableCopyAndRelease(MChcstat, &t_mutable_hcstat);

	// If the string isn't empty, it needs a return char
	if (!MCStringIsEmpty(*t_mutable_hcstat))
		/* UNCHECKED */ MCStringAppendChar(*t_mutable_hcstat, '\n');

	// Append the new line
	/* UNCHECKED */ MCStringAppend(*t_mutable_hcstat, *t_new_line);

	// We've released the previous MChcstat var above so just copy as immutable into MChcstat.
	/* UNCHECKED */ MCStringCopy(*t_mutable_hcstat, MChcstat);
}

static uint32_t MCHCBitmapStride(uint32_t p_width)
{
	// rows are padded to a multiple of 4 bytes
	return ((p_width + 31) & ~0x1F ) / 8;
}

// IM-2014-04-08: [[ Bug 12101 ]] Modify to return decoded bitmap data directly 
static bool convert_hcbitmap_data(uint1 *sptr, uint2 width, uint2 height, uint8_t *&r_bitmapdata)
{
	uint8_t *t_data;
	uint32_t t_stride;

	t_stride = MCHCBitmapStride(width);

	if (!MCMemoryNewArray(t_stride * (height + 64), t_data))
		return false;

	r_bitmapdata = t_data;

	uint1 xorcode = 0x89;
	uint2 patindex = 0;

	uint2 bpl = t_stride;
	uint1 *dptr = t_data;
	uint1 *deptr = t_data + height * bpl;
	uint1 *startptr = sptr;
	uint2 repcount = 1;
	uint2 line = 0;
	while (dptr < deptr)
	{
		uint2 hrepcount = bpl;
		line = (dptr - t_data) / bpl;
		uint1 opcode = *sptr++;
		if (opcode == 0)
			opcode = *sptr++;
		switch(opcode & 0xF0)
		{
		case 0x00:
		case 0x10:
		case 0x20:
		case 0x30:
		case 0x40:
		case 0x50:
		case 0x60:
		case 0x70:
			hrepcount = opcode & 0xF;
			if (hrepcount == 0)
			{
				hrepcount = (opcode & 0xF0) >> 4;
				while (hrepcount--)
					*dptr++ = *sptr++;
			}
			else
			{
				while (hrepcount--)
					*dptr++ = 0;
				hrepcount = (opcode & 0xF0) >> 4;
				while (hrepcount--)
				{
					*dptr++ = *sptr++;
				}
			}
			break;
		case 0x80:
			switch (opcode & 0xF)
			{
			case 0x0:
				while (hrepcount--)
					*dptr++ = *sptr++;
				break;
			case 0x1:
				while (repcount--)
				{
					while (hrepcount--)
						*dptr++ = 0;
					hrepcount = bpl;
				}
				break;
			case 0x2:
				while (repcount--)
				{
					while (hrepcount--)
						*dptr++ = 0xFF;
					hrepcount = bpl;
				}
				break;
			case 0x3:
				patbytes[patindex] = *sptr++;
			case 0x4:
				while (repcount--)
				{
					while (hrepcount--)
						*dptr++ = patbytes[patindex];
					hrepcount = bpl;
					patindex = (patindex + 1) & 0x7;
				}
				break;
			case 0x5:
				while (repcount--)
				{
					while (hrepcount--)
					{
						*dptr = *(dptr - bpl);
						dptr++;
					}
					hrepcount = bpl;
				}
				break;
			case 0x6:
				while (repcount--)
				{
					while (hrepcount--)
					{
						*dptr = *(dptr - (bpl << 1));
						dptr++;
					}
					hrepcount = bpl;
				}
				break;
			case 0x8:
			case 0x9:
			case 0xA:
			case 0xB:
			case 0xC:
			case 0xD:
			case 0xE:
			case 0xF:
				xorcode = opcode;
				break;
			case 0x7:

				hcstat_append("Unknown BMAP opcode %x at offset %d, line %d",
				        opcode, (int)(sptr - startptr), line);

				return true;
			}
			repcount = 1;
			continue;
		case 0x90:
			hcstat_append("Unknown BMAP opcode %x at offset %d, line %d",
			        opcode, (int)(sptr - startptr), line);

			return true;
		case 0xA0:
		case 0xB0:
			repcount = opcode & 0x1F;
			continue;
		case 0xC0:
		case 0xD0:
			hrepcount = (opcode & 0x1F) << 3;
			while (hrepcount--)
				*dptr++ = *sptr++;
			break;
		case 0xE0:
		case 0xF0:
			hrepcount = (opcode & 0x1F) << 4;
			while (hrepcount--)
				*dptr++ = 0;
			break;
		}
		if ((dptr - t_data) % bpl == 0)
		{
			line = (dptr - t_data) / bpl - 1;
			uint1 remainder = 0;
			hrepcount = bpl;
			uint1 *t1ptr = dptr - bpl;
			uint1 *t2ptr = dptr - bpl;
			switch (xorcode)
			{
			case 0x88:
				t2ptr += 2;
				hrepcount -= 2;
				break;
			case 0x89:
				continue;
			case 0x8A:
				t1ptr = dptr - bpl * 2;
				break;
			case 0x8B:
				t1ptr = dptr - bpl * 3;
				break;
			case 0x8C:
				while (hrepcount--)
				{
					*t2ptr = h1[*t2ptr ^ remainder];
					remainder = *t2ptr++ << 7;
				}
				continue;
			case 0x8D:
				while (hrepcount--)
				{
					*t2ptr = h1[*t2ptr ^ remainder];
					remainder = *t2ptr++ << 7;
				}
				hrepcount = bpl;
				t1ptr = dptr - bpl * 2;
				t2ptr = dptr - bpl;
				break;
			case 0x8E:
				while (hrepcount--)
				{
					*t2ptr = h2[*t2ptr ^ remainder];
					remainder = *t2ptr++ << 6;
				}
				hrepcount = bpl;
				t1ptr = dptr - bpl * 3;
				t2ptr = dptr - bpl;
				break;
			case 0x8F:
				t2ptr++;
				hrepcount--;
				break;
			}
			if (line != 0)
				while (hrepcount--)
					*t2ptr++ ^= *t1ptr++;
		}
	}
	if (dptr > deptr)
	{
		hcstat_append("Error: ran off end of image at offset %d line %d",
		        (int)(sptr - startptr), line);
	}
	return true;
}

static const char *convert_font(char *sptr)
{
	return sptr;  // pass font names directly through: will be ugly on Windows systems!
}

static uint2 convert_style(uint2 istyle)
{
	uint2 ostyle = FA_DEFAULT_STYLE;
	if (istyle != 0xFFFF)
	{
		if (istyle & HC_TSTYLE_BOLD)
			ostyle = (FE_NORMAL << 4) + MCFW_BOLD;
		if (istyle & HC_TSTYLE_ITALIC)
			ostyle |= FA_ITALIC;
		if (istyle & HC_TSTYLE_UNDERLINE)
			ostyle |= FA_UNDERLINE;
		if (istyle & HC_TSTYLE_GROUP)
			ostyle |= FA_LINK;
	}
	return ostyle;
}

static char *convert_string(const char *string)
{
	const uint1 *sptr = (const uint1 *)string;
	char *newstring = new (nothrow) char[strlen(string) + 1];
	uint1 *dptr = (uint1 *)newstring;
	while (*sptr)
	{
		if (*sptr == 0x0D)
			*dptr = '\n';
		else
#ifdef _MACOSX
			*dptr = *sptr;
#else
			*dptr = MCisotranslations[*sptr];
#endif

		dptr++;
		sptr++;
	}
	*dptr = *sptr;
	return newstring;
}

static char *convert_script(const char *string)
{
	if (!*string)
		return NULL;

	const uint1 *sptr = (const uint1 *)string;
	uint2 conversions = 0;
	uint2 length = 0;

	while (*sptr)
	{
		switch (*sptr)
		{
		case 0xB2:
		case 0xB3:
		case 0xAD:
			conversions++;
			break;
		default:
			break;
		}
		length++;
		sptr++;
	}
	sptr = (const uint1 *)string;
	char *newstring = new (nothrow) char[length + conversions + 1];
	uint1 *dptr = (uint1 *)newstring;
	while (*sptr)
	{
		switch (*sptr)
		{
		case 0x0D:
			*dptr = '\n';
			break;
		case 0xC2:
			*dptr = '\\';
			break;
		case 0xB2:
			*dptr++ = '<';
			*dptr = '=';
			break;
		case 0xB3:
			*dptr++ = '>';
			*dptr = '=';
			break;
		case 0xAD:
			*dptr++ = '<';
			*dptr = '>';
			break;
		default:
#ifdef _MACOSX
			*dptr = *sptr;
#else
			*dptr = MCisotranslations[*sptr];
#endif

			break;
		}
		dptr++;
		sptr++;
	}
	*dptr = *sptr;
	return newstring;
}

static MCHcfield *findfield(MCHcfield *flist, uint2 fid)
{
	if (flist != NULL)
	{
		MCHcfield *fptr = flist;
		do
		{
			if (fptr->id == fid)
				return fptr;
			fptr = (MCHcfield *)fptr->next();
		}
		while (fptr != flist);
	}
	return NULL;
}

static MCHcbutton *findbutton(MCHcbutton *blist, uint2 bid)
{
	if (blist != NULL)
	{
		MCHcbutton *bptr = blist;
		do
		{
			if (bptr->id == bid)
				return bptr;
			bptr = (MCHcbutton *)bptr->next();
		}
		while (bptr != blist);
	}
	return NULL;
}

MCHcsnd::MCHcsnd()
{
    m_name = MCValueRetain(kMCEmptyName);
	data = NULL;
}

MCHcsnd::~MCHcsnd()
{
    MCValueRelease(m_name);
	if (data != NULL)
		delete data;
}

Boolean MCHcsnd::import(uint4 inid, MCNameRef inname, char *sptr)
{
	id = inid;
	maxid = MCU_max((uint4)id, maxid);
    MCValueAssign(m_name, inname);
	uint2 type = get_uint2(sptr);
	uint2 hsize;
	real8 baserate;
	if (type == 1)
	{
		if (sptr[40]) // compressed sound
			return False;
		hsize = get_uint4(&sptr[16]);
		rate = get_uint2(&sptr[28]);
		baserate = (real8)(sptr[41] - 0x3C);
		size = get_uint4(&sptr[32]);
	}
	else
	{
		hsize = get_uint4(&sptr[10]);
		if (sptr[hsize + 20]) // compressed sound
			return False;
		rate = get_uint2(&sptr[hsize + 8]);
		baserate = (real8)(sptr[hsize + 21] - 0x3C);
		size = get_uint4(&sptr[hsize + 4]);
	}
	rate = (uint2)((real8)rate / pow(1.05946309434, (real8)baserate));
	data = new (nothrow) int1[size];
	memcpy(data, sptr + hsize + 22, size);
	uint1 *dptr = (uint1 *)data;
	uint4 i = size;
	while (i--)
		*dptr++ -= 128;
	return True;
}

MCAudioClip *MCHcsnd::build()
{
	MCAudioClip *aptr = new (nothrow) MCAudioClip;
	aptr->size = size;
    aptr->setname(m_name);
	aptr->samples = (int1 *)data;
	data = NULL;
	aptr->nchannels = 1;
	aptr->format = AF_SLINEAR;
	aptr->rate = rate;
	aptr->swidth = 1;
	return aptr;
}

MCHctext::MCHctext()
{
	card = False;
	atts = NULL;
	string = NULL;
}

MCHctext::~MCHctext()
{
	delete atts;
	delete string;
}

IO_stat MCHctext::parse(char *sptr)
{
	uint1 *uint1ptr = (uint1 *)sptr;
	int2 fid = (*uint1ptr << 8) | *(uint1ptr + 1);
	if (fid < 0)
	{
		card = True;
		id = -fid;
	}
	else
		id = fid;
	if (version == 1)
		string = convert_string(&sptr[2]);
	else
	{
		uint2 offset;
		uint2 *uint2ptr = (uint2 *)sptr;
		uint2 tsize = swap_uint2(&uint2ptr[1]);
		if (uint1ptr[4] & 0x80)
		{
			uint2 natts = swap_uint2(&uint2ptr[2]) & 0x7FFF;
			tsize -= natts;
			offset = natts + 4;
			natts = (natts >> 1) - 1;
			atts = new (nothrow) uint2[natts + 1];
			uint2 i;
			for (i = 0 ; i < natts ; i++)
				atts[i] = swap_uint2(&uint2ptr[i + 3]);
			atts[natts] = tsize;
		}
		else
		{
			tsize--;
			offset = 5;
		}
		char *newstring = new (nothrow) char[tsize + 1];
		strncpy(newstring, &sptr[offset], tsize);
		newstring[tsize] = '\0';
		string = convert_string(newstring);
		delete[] newstring;
	}
	return IO_NORMAL;
}

MCCdata *MCHctext::buildf(MCHcstak *hcsptr, MCField *parent)
{
	MCCdata *fptr = new (nothrow) MCCdata(cid);
	if (string == NULL)
		string = MCU_empty();
	char *eptr = string;
	MCParagraph *paragraphs = NULL;
	const char *tname = nullptr;
	uint2 tsize = 0;
	uint2 tstyle = 0;
	uint2 aindex = 2;
	uint2 aoffset = 0;
	uint2 alength = 0;

    MCExecContext ctxt(nil, nil, nil);
	if (atts != NULL)
	{
		hcsptr->getatts(atts[1], tname, tsize, tstyle);
		alength = atts[aindex];
	}
	do
	{
		char *sptr = eptr;
		eptr = strchr(sptr, '\n');
		if (eptr != NULL)
			*eptr++ = '\0';
		uint2 length = strlen(sptr);
		MCParagraph *pgptr = new (nothrow) MCParagraph;
		pgptr->setparent(parent);
		MCAutoStringRef t_string;
		/* UNCHECKED */ MCStringCreateWithNativeChars((const char_t*)sptr, length, &t_string);
		pgptr->settext(*t_string);
		pgptr->appendto(paragraphs);
		if (atts != NULL)
		{
			uint2 cindex = 0;
			do
			{
                if (tname != NULL)
                {
                    MCAutoStringRef t_fontname;
                    MCStringCreateWithCString(tname, &t_fontname);
                    pgptr -> SetTextFontOfCharChunk(ctxt, (integer_t)cindex, (integer_t)MCU_min(length, cindex + alength), *t_fontname);
                }
				if (tsize != 0xFFFF)
                {
                    uinteger_t t_size;
                    t_size = tsize;
                    pgptr -> SetTextSizeOfCharChunk(ctxt, (integer_t)cindex, (integer_t)MCU_min(length, cindex + alength), &t_size);
                }
				if (tstyle != FA_DEFAULT_STYLE)
                {
                    MCInterfaceTextStyle t_style;
                    t_style . style = tstyle;
                    pgptr -> SetTextStyleOfCharChunk(ctxt, (integer_t)cindex, (integer_t)MCU_min(length, cindex + alength), t_style);
                }
				uint2 tlength = alength;
				if (alength <= length - cindex && eptr != NULL)
				{
					Boolean skip = alength == length - cindex;
					hcsptr->getatts(atts[aindex + 1], tname, tsize, tstyle);
					aoffset = atts[aindex];
					aindex += 2;
					alength = atts[aindex] - aoffset;
					if (skip)
					{
						cindex++;
						alength--;
					}
				}
				else
					alength -= length + 1 - cindex;
				cindex += tlength;
			}
			while (cindex < length);
		}
	}
	while (eptr != NULL);
	fptr->setparagraphs(paragraphs);
	return fptr;
}

MCCdata *MCHctext::buildb()
{
	MCCdata *bptr = new (nothrow) MCCdata(cid);
	bptr->setset(string[0] == '1');
	return bptr;
}

char *MCHctext::buildm()
{
	char *tstring = string;
	string = NULL;
	return tstring;
}

MCHcfield::MCHcfield()
{
	name = NULL;
	script = NULL;
	text = NULL;
}

MCHcfield::~MCHcfield()
{
	delete name;
	delete script;
	while (text != NULL)
	{
		MCHctext *tptr = text->remove
		                 (text);
		delete tptr;
	}
}

IO_stat MCHcfield::parse(char *sptr)
{
	int2 *int2ptr = (int2 *)sptr;
	uint2 *uint2ptr = (uint2 *)sptr;

	id = swap_uint2(&uint2ptr[1]);
	rect.x = swap_int2(&int2ptr[4]);
	rect.y = swap_int2(&int2ptr[3]);
	rect.width = swap_int2(&int2ptr[6]) - rect.x + 1;
	rect.height = swap_int2(&int2ptr[5]) - rect.y + 1;
	atts = (sptr[5] << 8) | (sptr[14] & 0xFF);
	style = sptr[15];
	tfont = swap_uint2(&uint2ptr[11]);
	tsize = swap_uint2(&uint2ptr[12]);
	hctstyle = swap_uint2(&uint2ptr[13]);
	tstyle = convert_style(hctstyle);
	talign = (sptr[21] & 0xFF);
	theight = swap_uint2(&uint2ptr[14]);
	uint2 offset = 15;
	if (sptr[offset * 2])
		name = strclone(&sptr[offset * 2]);
	script = convert_script(&sptr[offset * 2 + strlen(&sptr[offset * 2]) + 2]);
	return IO_NORMAL;
}

MCControl *MCHcfield::build(MCHcstak *hcsptr, MCStack *sptr)
{
	MCField *fptr = (MCField *)MCtemplatefield->clone(False, OP_NONE, false);
	fptr->parent = sptr;
	fptr -> setname_cstring(name);
	delete name;
	fptr->altid = id;
	fptr->obj_id = ++maxid;
	if (script != NULL)
	{
		fptr -> setscript_cstring(script);
		delete script;
	}
	name = script = NULL;
	fptr->rect = rect;
	fptr->fontheight = theight;
	const char *fontname = hcsptr->getfont(tfont);
	if (fontname != NULL || tsize != HC_DEFAULT_TEXT_SIZE
	        || tstyle != FA_DEFAULT_STYLE)
	{
		if (fontname == NULL)
			fontname = HC_DEFAULT_TEXT_FONT;

		// MW-2012-02-17: [[ LogFonts ]] Set the font attributes of the object.
        MCAutoStringRef t_fontname;
        /* UNCHECKED */ MCStringCreateWithCString(fontname, &t_fontname);
		fptr -> setfontattrs(*t_fontname, tsize, tstyle);
	}
	fptr->flags &= ~(F_STYLE | F_DISPLAY_STYLE | F_SHOW_LINES | F_LOCK_TEXT
	                 | F_AUTO_TAB | F_SHARED_TEXT
	                 | F_F_DONT_SEARCH | F_DONT_WRAP);
	if (atts & HC_F_MARGINS)
	{
		fptr->leftmargin = fptr->rightmargin =
		                       fptr->topmargin = fptr->bottommargin = 14;
	}
	else
	{
		fptr->leftmargin = fptr->rightmargin =
		                       fptr->topmargin = fptr->bottommargin = 8;
	}

	if (!(fptr->flags & F_LOCK_TEXT))
		rect = MCU_reduce_rect(rect, -MCfocuswidth);
	else
		fptr->flags &= ~F_TRAVERSAL_ON;

	if (atts & HC_F_MULTIPLE)
		fptr->flags |= F_MULTIPLE_HILITES;
	if (atts & HC_F_AUTO_SELECT)
		fptr->flags |= F_LIST_BEHAVIOR | F_TRAVERSAL_ON;
	if (atts & HC_F_SHOW_LINES)
		fptr->flags |= F_SHOW_LINES;
	if (atts & HC_F_LOCK_TEXT)
	{
		fptr->flags |= F_LOCK_TEXT;
		if (!(atts & HC_F_AUTO_SELECT))
			fptr->flags &= ~F_TRAVERSAL_ON;
	}
	if (atts & HC_F_AUTO_TAB)
		fptr->flags |= F_AUTO_TAB;
	if (atts & HC_F_UNFIXED_HEIGHT)
		fptr->flags &= ~F_FIXED_HEIGHT;
	if (atts & HC_F_SHARED_TEXT)
		fptr->flags |= F_SHARED_TEXT | F_LOCK_TEXT;
	if (atts & HC_F_DONT_SEARCH)
		fptr->flags |= F_F_DONT_SEARCH;
	if (atts & HC_F_DONT_WRAP)
		fptr->flags |= F_DONT_WRAP;
	if (atts & HC_F_INVISIBLE)
		fptr->flags &= ~F_VISIBLE;

	switch (talign)
	{
	case HC_TALIGN_LEFT:
		fptr->flags |= F_ALIGN_LEFT;
		break;
	case HC_TALIGN_CENTER:
		fptr->flags |= F_ALIGN_CENTER;
		break;
	case HC_TALIGN_RIGHT:
		fptr->flags |= F_ALIGN_RIGHT;
		break;
	}
	switch (style)
	{
	case HC_FSTYLE_TRANS:
		fptr->flags |= F_3D;
		break;
	case HC_FSTYLE_OPAQUE:
		fptr->flags |= F_OPAQUE | F_3D;
		break;
	case HC_FSTYLE_RECT:
		fptr->flags |= F_SHOW_BORDER | F_OPAQUE | F_3D;
		break;
	case HC_FSTYLE_SHADOW:
		fptr->flags |= F_SHOW_BORDER | F_OPAQUE | F_SHADOW;
		break;
	case HC_FSTYLE_SCROLL:
		fptr->flags |= F_SHOW_BORDER | F_OPAQUE | F_3D;
		fptr->flags &= ~F_SHOW_LINES;
		if (!(fptr->flags & F_VSCROLLBAR))
		{
			fptr->flags |= F_VSCROLLBAR;
			fptr->vscrollbar = new (nothrow) MCScrollbar(*MCtemplatescrollbar);
			fptr->vscrollbar->setparent(fptr);
			fptr->vscrollbar->allowmessages(False);
			fptr->vscrollbar->setflag(False, F_TRAVERSAL_ON);
			fptr->setsbrects();
		}
		break;
	}
	if (hctstyle & HC_TSTYLE_OUTLINE)
	{
		fptr->ncolors = 1;
		fptr->colors = new (nothrow) MCColor;
		fptr->colors[0].red = fptr->colors[0].green
		                      = fptr->colors[0].blue = MAXUINT2;
		fptr->colornames = new (nothrow) MCStringRef[1];
		fptr->colornames[0] = nil;
		fptr->dflags |= DF_FORE_COLOR;
	}
	while (text != NULL)
	{
		MCHctext *tptr = text->remove
		                 (text);
		MCCdata *newfdata = tptr->buildf(hcsptr, fptr);
		newfdata->appendto(fptr->fdata);
		delete tptr;
	}
	return fptr;
}

MCHcbutton::MCHcbutton()
{
	name = NULL;
	script = NULL;
	text = NULL;
}

MCHcbutton::~MCHcbutton()
{
	delete name;
	delete script;
	while (text != NULL)
	{
		MCHctext *tptr = text->remove
		                 (text);
		delete tptr;
	}
}

IO_stat MCHcbutton::parse(char *sptr)
{
	int2 *int2ptr = (int2 *)sptr;
	uint2 *uint2ptr = (uint2 *)sptr;

	id = swap_uint2(&uint2ptr[1]);
	rect.x = swap_int2(&int2ptr[4]);
	rect.y = swap_int2(&int2ptr[3]);
	rect.width = swap_int2(&int2ptr[6]) - rect.x + 1;
	rect.height = swap_int2(&int2ptr[5]) - rect.y + 1;
	atts = (sptr[5] << 8) | (sptr[14] & 0xFF);
	style = sptr[15];
	titlewidth = swap_uint2(&uint2ptr[8]);
	icon = swap_uint2(&uint2ptr[9]);
	tfont = swap_uint2(&uint2ptr[11]);
	tsize = swap_uint2(&uint2ptr[12]);
	hctstyle = swap_uint2(&uint2ptr[13]);
	tstyle = convert_style(hctstyle);
	talign = (sptr[21] & 0xFF);
	uint2 offset = 15;
	if (sptr[offset * 2])
		name = strclone(&sptr[offset * 2]);
	script = convert_script(&sptr[offset * 2 + strlen(&sptr[offset * 2]) + 2]);
	return IO_NORMAL;
}

MCControl *MCHcbutton::build(MCHcstak *hcsptr, MCStack *sptr)
{
	MCButton *bptr = (MCButton *)MCtemplatebutton->clone(False, OP_NONE, false);
	bptr->parent = sptr;
	bptr->setname_cstring(name);
	delete name;
	bptr->altid = id;
	bptr->obj_id = ++maxid;
	bptr->rect = rect;
	if (script != NULL)
	{
		bptr -> setscript_cstring(script);
		delete script;
	}
	name = script = NULL;
	bptr->flags &= ~(F_STYLE | F_DISPLAY_STYLE | F_ALIGNMENT | F_SHOW_ICON
	                 | F_TRAVERSAL_ON | F_SHOW_NAME | F_AUTO_HILITE
	                 | F_SHARED_HILITE | F_HILITE_MODE | F_ARM_MODE);
	bptr->family = atts & 0x0F;
	if (!(atts & HC_B_NOT_SHARED))
		bptr->flags |= F_SHARED_HILITE;
	if (atts & HC_B_SHOW_NAME)
		bptr->flags |= F_SHOW_NAME;
	if (atts & HC_B_AUTO_HILITE || bptr->family)
		bptr->flags |= F_AUTO_HILITE;
	if (atts & HC_B_INVISIBLE)
		bptr->flags &= ~F_VISIBLE;
	if (atts & HC_B_DISABLED)
		bptr->flags |= F_DISABLED;
	while (text != NULL)
	{
		MCHctext *tptr = text->remove
		                 (text);
		if (!(bptr->flags & F_SHARED_HILITE) && tptr->cid != 0)
		{
			MCCdata *newbdata = tptr->buildb();
			newbdata->appendto(bptr->bdata);
		}
		else
		{
			MCStringRef t_menustring = nil;
			/* UNCHECKED */ MCStringCreateWithCString(tptr->buildm(), t_menustring);
			MCValueAssign(bptr->menustring, t_menustring);
			MCValueRelease(t_menustring);
		}
		delete tptr;
	}
	switch (style)
	{
	case HC_BSTYLE_TRANS:
		bptr->flags |= F_RECTANGLE | F_3D;
		break;
	case HC_BSTYLE_OPAQUE:
		bptr->flags |= F_RECTANGLE | F_OPAQUE | F_3D | F_HILITE_FILL;
		break;
	case HC_BSTYLE_DEFAULT:
		bptr->flags |= F_DEFAULT;
	case HC_BSTYLE_STANDARD:
		bptr->flags |= F_STANDARD | F_SHOW_BORDER | F_OPAQUE
		               | F_3D | F_HILITE_BOTH | F_ARM_BORDER;
		break;
	case HC_BSTYLE_RECT:
		bptr->flags |= F_RECTANGLE | F_SHOW_BORDER | F_OPAQUE
		               | F_3D | F_HILITE_BOTH | F_ARM_BORDER;
		break;
	case HC_BSTYLE_SHADOW:
		bptr->flags |= F_RECTANGLE | F_SHOW_BORDER | F_OPAQUE
		               | F_SHADOW | F_HILITE_BOTH;
		break;
	case HC_BSTYLE_ROUND:
		bptr->flags |=  F_ROUNDRECT | F_SHOW_BORDER | F_OPAQUE | F_HILITE_FILL;
		break;
	case HC_BSTYLE_CHECK:
		bptr->flags |= F_CHECK | F_3D;
		talign = HC_TALIGN_LEFT;
		icon = 0;
		break;
	case HC_BSTYLE_RADIO:
		bptr->flags |= F_RADIO | F_3D;
		talign = HC_TALIGN_LEFT;
		icon = 0;
		break;
	case HC_BSTYLE_OVAL:
		bptr->flags |= F_OVAL_BUTTON | F_HILITE_FILL;
		break;
	case HC_BSTYLE_POPUP:
		bptr->flags |= F_MENU | F_3D | F_SHOW_BORDER | F_OPAQUE
		               | F_ALIGN_CENTER | F_ARM_BORDER;
		bptr->menumode = WM_OPTION;
		talign = HC_TALIGN_LEFT;
		bptr->menuhistory = icon;
		bptr->labelwidth = titlewidth;
		icon = 0;
		bptr->resetlabel();
		break;
	}
	switch (talign)
	{
	case HC_TALIGN_LEFT:
		bptr->flags |= F_ALIGN_LEFT;
		break;
	case HC_TALIGN_CENTER:
		bptr->flags |= F_ALIGN_CENTER;
		break;
	case HC_TALIGN_RIGHT:
		bptr->flags |= F_ALIGN_RIGHT;
		break;
	}
	uint4 iid = hcsptr->geticon(icon);
	if (iid != 0)
	{
		bptr->icons = new (nothrow) iconlist;
		memset(bptr->icons, 0, sizeof(iconlist));
		bptr->icons->iconids[CI_DEFAULT] = iid;	
		bptr->flags |= F_SHOW_ICON;

		// MW-2012-02-17: [[ LogFonts ]] Set the font attributes of the object.
		bptr -> setfontattrs(MCSTR("textfont"), 9, FA_DEFAULT_STYLE);
		bptr -> fontheight = 12;
	}
	else
	{
		bptr->fontheight = heightfromsize(tsize);
		const char *fontname = hcsptr->getfont(tfont);
		if (fontname != NULL || tsize != HC_DEFAULT_TEXT_SIZE
		        || tstyle != FA_DEFAULT_STYLE)
		{
			if (fontname == NULL)
				fontname = HC_DEFAULT_TEXT_FONT;

			// MW-2012-02-17: [[ LogFonts ]] Set the font attributes of the object.
            MCAutoStringRef t_fontname;
            /* UNCHECKED */ MCStringCreateWithCString(fontname, &t_fontname);
			bptr -> setfontattrs(*t_fontname, tsize, tstyle);
		}
	}
	if (hctstyle & HC_TSTYLE_OUTLINE)
	{
		bptr->ncolors = 1;
		bptr->colors = new (nothrow) MCColor;
		bptr->colors[0].red = bptr->colors[0].green = bptr->colors[0].blue = MAXUINT2;
		bptr->colornames = new (nothrow) MCStringRef[1];
		bptr->colornames[0] = nil;
		bptr->dflags |= DF_FORE_COLOR;
	}
	if (bptr->flags & F_SHARED_HILITE)
	{
		MCCdata *newbdata = new (nothrow) MCCdata(0);
		newbdata->setset(atts & HC_B_HILITED);
		newbdata->appendto(bptr->bdata);
	}
	return bptr;
}

MCHcbmap::MCHcbmap()
{
    m_name = MCValueRetain(kMCEmptyName);
	mask = nil;
	data = nil;
	visible = True;
	xhot = yhot = 1;
}

MCHcbmap::~MCHcbmap()
{
    MCValueRelease(m_name);
	if (data != nil)
		MCMemoryDeleteArray(data);
	if (mask != nil)
		MCMemoryDeleteArray(mask);
}

void MCHcbmap::setvisible(Boolean newvis)
{
	visible = newvis;
}

void MCHcbmap::icon(uint4 inid, MCNameRef inname, char *sptr)
{
	id = inid;
    MCValueAssign(m_name, inname);
	rect.x = iconx;
	rect.y = icony;
	iconx += 32;
	if (iconx >= 512)
	{
		iconx = 0;
		icony += 32;
	}
	rect.width = rect.height = 32;
	// IM-2014-04-08: [[ Bug 12101 ]] Copy raw bitmap to data
	/* UNCHECKED */ MCMemoryAllocateCopy((uint8_t*)sptr, 128, data);
}

void MCHcbmap::cursor(uint4 inid, MCNameRef inname, char *sptr)
{
	id = inid;
    MCValueAssign(m_name, inname);
	rect.x = cursorx;
	rect.y = cursory;
	cursorx += 16;
	if (cursorx >= 512)
	{
		cursorx = 0;
		cursory += 16;
	}
	rect.width = rect.height = 16;
	mrect = rect;
	// IM-2014-04-08: [[ Bug 12101 ]] Copy raw bitmaps to data and mask
	/* UNCHECKED */ MCMemoryAllocateCopy((uint8_t*)sptr, 32, data);
	/* UNCHECKED */ MCMemoryAllocateCopy((uint8_t*)sptr + 32, 32, mask);
	xhot = get_uint2(&sptr[64]);
	yhot = get_uint2(&sptr[66]);
}

IO_stat MCHcbmap::parse(char *sptr)
{
	int2 *int2ptr = (int2 *)sptr;
	uint4 *uint4ptr = (uint4 *)sptr;
	id = swap_uint4(&uint4ptr[2]);
	maxid = MCU_max(id, maxid);
	uint4 offset;
	uint4 masksize;
	if (version == 1)
	{
		mrect.x = swap_int2(&int2ptr[15]) & 0xFFE0;
		mrect.y = swap_int2(&int2ptr[14]);
		mrect.width = (swap_int2(&int2ptr[17]) - mrect.x + 31) & 0xFFE0;
		mrect.height = swap_int2(&int2ptr[16]) - mrect.y;
		rect.x = swap_int2(&int2ptr[19]) & 0xFFE0;
		rect.y = swap_int2(&int2ptr[18]);
		rect.width = (swap_int2(&int2ptr[21]) - rect.x + 31) & 0xFFE0;
		rect.height = swap_int2(&int2ptr[20]) - rect.y;
		masksize = swap_uint4(&uint4ptr[13]);
		offset = 60;
	}
	else
	{
		mrect.x = swap_int2(&int2ptr[17]) & 0xFFE0;
		mrect.y = swap_int2(&int2ptr[16]);
		mrect.width = (swap_int2(&int2ptr[19]) - mrect.x + 31) & 0xFFE0;
		mrect.height = swap_int2(&int2ptr[18]) - mrect.y;
		rect.x = swap_int2(&int2ptr[21]) & 0xFFE0;
		rect.y = swap_int2(&int2ptr[20]);
		rect.width = (swap_int2(&int2ptr[23]) - rect.x + 31) & 0xFFE0;
		rect.height = swap_int2(&int2ptr[22]) - rect.y;
		masksize = swap_uint4(&uint4ptr[14]);
		offset = 64;
	}
	// IM-2014-04-08: [[ Bug 12101 ]] Decode bitmap to mask
	if (masksize != 0)
		/* UNCHECKED */ convert_hcbitmap_data((uint8_t*)&sptr[offset], mrect.width, mrect.height, mask);
	offset += masksize;
	// IM-2014-04-08: [[ Bug 12101 ]] Decode bitmap to data
	if (rect.width != 0 && rect.height != 0)
		/* UNCHECKED */ convert_hcbitmap_data((uint8_t*)&sptr[offset], rect.width, rect.height, data);
	return IO_NORMAL;
}

void MCImageBitmapApplyPlane(MCImageBitmap *p_dst, uint8_t *p_src, uindex_t p_src_stride, uint32_t p_value);
void surface_merge_with_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height);
MCControl *MCHcbmap::build()
{
	if (data == nil)
		return nil;

	uint8_t *data2;
	data2 = nil;

	if (mask == nil)
	{
		// IM-2014-04-08: [[ Bug 12101 ]] Use data bitmap as mask if not given
		/* UNCHECKED */ MCMemoryAllocateCopy(data, rect.height * MCHCBitmapStride(rect.width), mask);
        mrect = rect;
	}
	else
	{
		MCRectangle trect = MCU_union_rect(mrect, rect);

		uint32_t t_stride;
		t_stride = MCHCBitmapStride(trect.width);

		uint32_t t_bytes;
		t_bytes = t_stride * trect.height;

		if (!MCU_equal_rect(trect, mrect))
		{
			// IM-2014-04-08: [[ Bug 12101 ]] Resize mask bitmap to new size
			uint8_t *t_newmask;
			t_newmask = nil;

			/* UNCHECKED */ MCMemoryNewArray(t_bytes, t_newmask);

			uint32_t t_mask_stride;
			t_mask_stride = MCHCBitmapStride(mrect.width);

			uint4 sbpl = t_mask_stride;
			uint4 offset = (mrect.y - trect.y) * t_stride + (mrect.x - trect.x) / 8;
			for (uint32_t i = 0 ; i < mrect.height ; i++)
				MCMemoryCopy(&t_newmask[i * t_stride + offset], &mask[i * sbpl], sbpl);
			MCMemoryDeleteArray(mask);
			mask = t_newmask;
			mrect = trect;
		}

		if (!MCU_equal_rect(trect, rect))
		{
			// IM-2014-04-08: [[ Bug 12101 ]] Resize data bitmap to new size
			uint8_t* t_newdata;
			t_newdata = nil;

			/* UNCHECKED */ MCMemoryNewArray(t_bytes, t_newdata);

			uint4 sbpl = MCHCBitmapStride(rect.width);
			uint4 offset = (rect.y - trect.y) * t_stride + (rect.x - trect.x) / 8;
			for (uint32_t i = 0 ; i < rect.height ; i++)
				MCMemoryCopy(&t_newdata[i * t_stride + offset], &data[i * sbpl], sbpl);
			MCMemoryDeleteArray(data);
			data = t_newdata;
			rect = trect;
		}

		// IM-2014-04-08: [[ Bug 12101 ]] Make bits set in data opaque in mask
		uint4 bytes = t_bytes;
		uint1 *sptr = data;
		uint1 *dptr = mask;
		while (bytes--)
			*dptr++ |= *sptr++;

		// IM-2014-04-08: [[ Bug 12101 ]] Extract bits set in mask but not in data - these will be opaque white in final image
		/* UNCHECKED */ MCMemoryAllocateCopy(mask, t_bytes, data2);
		bytes = t_bytes;
		sptr = data;
		dptr = data2;
		while (bytes--)
			*dptr++ ^= *sptr++;
	}

	MCImage *iptr = (MCImage *)MCtemplateimage->clone(False, OP_NONE, false);

	iptr->obj_id = id;
	iptr->rect = rect;
	if (!visible)
		iptr->flags &= ~F_VISIBLE;
	iptr->flags |= MCscreen->getpad() >> 3;
	iptr->flags |= F_RLE;

	uint32_t t_bytes, t_stride;
	t_stride = MCHCBitmapStride(rect.width);
	t_bytes = t_stride * rect.height;

	MCImageBitmap *t_bitmap = nil;
	MCImageCompressedBitmap *t_compressed = nil;
	/* UNCHECKED */ MCImageBitmapCreate(rect.width, rect.height, t_bitmap);
	// set first plane to opaque black
	MCImageBitmapApplyPlane(t_bitmap, data, t_stride, 0xFF000000);
	// set second plane to opaque white
	if (data2 != nil)
		MCImageBitmapApplyPlane(t_bitmap, data2, t_stride, 0xFFFFFFFF);
	if (mask != nil)
	{
		surface_merge_with_mask(t_bitmap->data, t_bitmap->stride, mask, t_stride, 0, rect.width, rect.height);
		MCImageBitmapCheckTransparency(t_bitmap);
	}
	MCImageCompress(t_bitmap, false, t_compressed);
	iptr->setcompressedbitmap(t_compressed);
	MCImageFreeBitmap(t_bitmap);
	MCImageFreeCompressedBitmap(t_compressed);

	if (mask != nil)
		MCMemoryDeleteArray(mask);
	if (data != nil)
		MCMemoryDeleteArray(data);
	if (data2 != nil)
		MCMemoryDeleteArray(data2);

	mask = data = data2 = nil;

	iptr->setname(m_name);
	iptr->xhot = xhot;
	iptr->yhot = yhot;
	return iptr;
}

MCHccard::MCHccard()
{
	name = NULL;
	script = NULL;
	objects = NULL;
	hcbuttons = NULL;
	hcfields = NULL;
	hctexts = NULL;
}

MCHccard::~MCHccard()
{
	delete name;
	delete script;
	delete objects;
	while (hcbuttons != NULL)
	{
		MCHcbutton *bptr = hcbuttons->remove
		                   (hcbuttons);
		delete bptr;
	}
	while (hcfields != NULL)
	{
		MCHcfield *fptr = hcfields->remove
		                  (hcfields);
		delete fptr;
	}
	while (hctexts != NULL)
	{
		MCHctext *fptr = hctexts->remove
		                 (hctexts);
		delete fptr;
	}
}

IO_stat MCHccard::parse(char *sptr)
{
	uint2 *uint2ptr = (uint2 *)sptr;
	uint4 *uint4ptr = (uint4 *)sptr;
	id = swap_uint4(&uint4ptr[2]);
	maxid = MCU_max(id, maxid);
	uint2 ntext;
	uint4 offset;
	if (version == 1)
	{
		bmapid = swap_uint4(&uint4ptr[3]);
		bkgdid = swap_uint4(&uint4ptr[8]);
		nobjects = swap_uint2(&uint2ptr[18]);
		ntext = swap_uint2(&uint2ptr[22]);
		atts = sptr[18];
		offset = 25;
	}
	else
	{
		bmapid = swap_uint4(&uint4ptr[4]);
		bkgdid = swap_uint4(&uint4ptr[9]);
		nobjects = swap_uint2(&uint2ptr[20]);
		ntext = swap_uint2(&uint2ptr[24]);
		atts = sptr[20];
		offset = 27;
	}
	if (nobjects)
		objects = new (nothrow) uint2[nobjects];
	uint2 i;
	for (i = 0 ; i < nobjects ; i++)
	{
		objects[i] = sptr[offset * 2 + 4];
		switch (objects[i])
		{
		case HC_OTYPE_BUTTON:
			{
				MCHcbutton *newbutton = new (nothrow) MCHcbutton;
				newbutton->appendto(hcbuttons);
				if (newbutton->parse(&sptr[offset * 2]) != IO_NORMAL)
					return IO_ERROR;
			}
			break;
		case HC_OTYPE_FIELD:
			{
				MCHcfield *newfield = new (nothrow) MCHcfield;
				newfield->appendto(hcfields);
				if (newfield->parse(&sptr[offset * 2]) != IO_NORMAL)
					return IO_ERROR;
			}
			break;
		default:
			hcstat_append("Error: Unknown object type %x",
			        swap_uint2(&uint2ptr[offset]));
			break;
		}
		offset += swap_uint2(&uint2ptr[offset]) >> 1;
	}
	offset <<= 1;
	while (ntext--)
	{
		MCHctext *newtext = new (nothrow) MCHctext;
		newtext->cid = id;
		newtext->appendto(hctexts);
		if (newtext->parse(&sptr[offset]) != IO_NORMAL)
			return IO_ERROR;
		if (version == 1)
			if (newtext->string != NULL)
				offset += strlen(newtext->string) + 3;
			else
				offset += 3;
		else
			offset += ((uint2ptr[(offset >> 1) + 1] + 1) & ~0x01) + 4;
	}

	if (sptr[offset])
		name = strclone(&sptr[offset]);
	script = convert_script(&sptr[offset + strlen(&sptr[offset]) + 1]);
	
	return IO_NORMAL;
}

MCCard *MCHccard::build(MCHcstak *hcsptr, MCStack *sptr)
{
	MCObjptr *newoptr;
	MCCard *cptr = MCtemplatecard->clone(False, False);
	cptr->obj_id = id;
	cptr->setname_cstring(name);
	delete name;
	if (script != NULL)
	{
		cptr -> setscript_cstring(script);
		delete script;
	}
	name = script = NULL;
	if (atts & HC_BC_DONT_SEARCH)
		cptr->flags |= F_G_DONT_SEARCH;
	if (atts & HC_BC_CANT_DELETE)
		cptr->flags |= F_G_CANT_DELETE;
	newoptr = new (nothrow) MCObjptr;
	newoptr->setparent(cptr);
	newoptr->setid(bkgdid);
	newoptr->appendto(cptr->objptrs);
	if (bmapid != 0)
	{
		newoptr = new (nothrow) MCObjptr;
		newoptr->setparent(cptr);
		newoptr->setid(bmapid);
		newoptr->appendto(cptr->objptrs);
		MCHcbmap *bmptr = hcsptr->getbmap(bmapid);
		bmptr->setvisible((atts & HC_BC_HIDE_BMAP) == 0);
	}
	MCHcbkgd *bgptr = hcsptr->getbkgd(bkgdid);
	while (hctexts != NULL)
	{
		MCHctext *tptr = hctexts->remove
		                 (hctexts);
		if (tptr->card)
		{
			MCHcfield *fptr = findfield(hcfields, tptr->id);
			if (fptr != NULL)
				tptr->appendto(fptr->text);
			else
			{
				MCHcbutton *bptr = findbutton(hcbuttons, tptr->id);
				if (bptr != NULL)
					tptr->appendto(bptr->text);
				else
					delete tptr;
			}
		}
		else
		{
			MCHcfield *fptr = findfield(bgptr->hcfields, tptr->id);
			if (fptr != NULL)
				tptr->appendto(fptr->text);
			else
			{
				MCHcbutton *bptr = findbutton(bgptr->hcbuttons, tptr->id);
				if (bptr != NULL)
					tptr->appendto(bptr->text);
				else
					delete tptr;
			}
		}
	}
	uint2 i;
	for (i = 0 ; i < nobjects ; i++)
		switch (objects[i])
		{
		case HC_OTYPE_BUTTON:
			{
				MCHcbutton *bptr = hcbuttons->remove
				                   (hcbuttons);
				MCControl *newbutton = bptr->build(hcsptr, sptr);
				newbutton->setparent(sptr);
				newbutton->appendto(sptr->controls);
				newoptr = new (nothrow) MCObjptr;
				newoptr->setparent(cptr);
				newoptr->setid(newbutton->getid());
				newoptr->appendto(cptr->objptrs);
				delete bptr;
			}
			break;
		case HC_OTYPE_FIELD:
			{
				MCHcfield *fptr = hcfields->remove
				                  (hcfields);
				MCControl *newfield = fptr->build(hcsptr, sptr);
				newfield->setparent(sptr);
				newfield->appendto(sptr->controls);
				newoptr = new (nothrow) MCObjptr;
				newoptr->setparent(cptr);
				newoptr->setid(newfield->getid());
				newoptr->appendto(cptr->objptrs);
				delete fptr;
			}
			break;
		}
	return cptr;
}

MCHcbkgd::MCHcbkgd()
{
	name = NULL;
	script = NULL;
	objects = NULL;
	hcbuttons = NULL;
	hcfields = NULL;
	hctexts = NULL;
}

MCHcbkgd::~MCHcbkgd()
{
	delete name;
	delete script;
	delete objects;
	while (hcbuttons != NULL)
	{
		MCHcbutton *bptr = hcbuttons->remove
		                   (hcbuttons);
		delete bptr;
	}
	while (hcfields != NULL)
	{
		MCHcfield *fptr = hcfields->remove
		                  (hcfields);
		delete fptr;
	}
	while (hctexts != NULL)
	{
		MCHctext *fptr = hctexts->remove
		                 (hctexts);
		delete fptr;
	}
}

IO_stat MCHcbkgd::parse(char *sptr)
{
	uint2 *uint2ptr = (uint2 *)sptr;
	uint4 *uint4ptr = (uint4 *)sptr;
	id = swap_uint4(&uint4ptr[2]);
	maxid = MCU_max(id, maxid);
	uint4 offset;
	uint2 ntext;
	if (version == 1)
	{
		bmapid = swap_uint4(&uint4ptr[3]);
		nobjects = swap_uint2(&uint2ptr[16]);
		ntext = swap_uint2(&uint2ptr[20]);
		atts = sptr[18];
		offset = 23;
	}
	else
	{
		bmapid = swap_uint4(&uint4ptr[4]);
		nobjects = swap_uint2(&uint2ptr[18]);
		ntext = swap_uint2(&uint2ptr[22]);
		atts = sptr[20];
		offset = 25;
	}
	if (nobjects)
		objects = new (nothrow) uint2[nobjects];
	uint2 i;
	for (i = 0 ; i < nobjects ; i++)
	{
		objects[i] = sptr[offset * 2 + 4];
		switch (objects[i])
		{
		case HC_OTYPE_BUTTON:
			{
				MCHcbutton *newbutton = new (nothrow) MCHcbutton;
				newbutton->appendto(hcbuttons);
				if (newbutton->parse(&sptr[offset * 2]) != IO_NORMAL)
					return IO_ERROR;
			}
			break;
		case HC_OTYPE_FIELD:
			{
				MCHcfield *newfield = new (nothrow) MCHcfield;
				newfield->appendto(hcfields);
				if (newfield->parse(&sptr[offset * 2]) != IO_NORMAL)
					return IO_ERROR;
			}
			break;
		default:
			hcstat_append("Error: bad object type %x",
			        swap_uint2(&uint2ptr[offset]));
			break;
		}
		offset += swap_uint2(&uint2ptr[offset]) >> 1;
	}
	offset <<= 1;
	while (ntext--)
	{
		MCHctext *newtext = new (nothrow) MCHctext;
		newtext->cid = 0;
		newtext->appendto(hctexts);
		if (newtext->parse(&sptr[offset]) != IO_NORMAL)
			return IO_ERROR;
		if (version == 1)
			if (newtext->string != NULL)
				offset += strlen(newtext->string) + 3;
			else
				offset += 3;
		else
			offset += ((uint2ptr[(offset >> 1) + 1] + 1) & ~0x01) + 4;
	}

	if (sptr[offset])
		name = strclone(&sptr[offset]);
	script = convert_script(&sptr[offset + strlen(&sptr[offset]) + 1]);
	return IO_NORMAL;
}

MCGroup *MCHcbkgd::build(MCHcstak *hcsptr, MCStack *sptr)
{
	MCGroup *gptr = (MCGroup *)MCtemplategroup->clone(False, OP_NONE, false);
	// MW-2011-08-09: [[ Groups ]] HC backgrounds are shared.
	gptr->flags = (gptr -> flags & ~F_GROUP_ONLY) | F_GROUP_SHARED;
	gptr->obj_id = id;
	gptr->setname_cstring(name);
	delete name;
	gptr->rect = sptr->getrect();
	gptr->rect.x = gptr->rect.y = 0;
	if (script != NULL)
	{
		gptr -> setscript_cstring(script);
		delete script;
	}
	name = script = NULL;
	if (atts & HC_BC_DONT_SEARCH)
		gptr->flags |= F_G_DONT_SEARCH;
	if (atts & HC_BC_CANT_DELETE)
		gptr->flags |= F_G_CANT_DELETE;
	if (bmapid != 0)
	{
		MCHcbmap *bmptr = hcsptr->removebmap(bmapid);
		bmptr->setvisible((atts & HC_BC_HIDE_BMAP) == 0);
		MCControl *iptr = bmptr->build();
		delete bmptr;
		if (iptr != NULL)
		{
			iptr->setparent(gptr);
			iptr->appendto(gptr->controls);
		}
	}
	while (hctexts != NULL)
	{
		MCHctext *tptr = hctexts->remove
		                 (hctexts);
		MCHcfield *fptr = findfield(hcfields, tptr->id);
		if (fptr != NULL)
			tptr->appendto(fptr->text);
		else
		{
			MCHcbutton *bptr = findbutton(hcbuttons, tptr->id);
			if (bptr != NULL)
				tptr->appendto(bptr->text);
			else
				delete tptr;
		}
	}
	uint2 i;
	for (i = 0 ; i < nobjects ; i++)
		switch (objects[i])
		{
		case HC_OTYPE_BUTTON:
			{
				MCHcbutton *bptr = hcbuttons->remove
				                   (hcbuttons);
				MCControl *newbutton = bptr->build(hcsptr, sptr);
				newbutton->setparent(gptr);
				newbutton->appendto(gptr->controls);
				delete bptr;
			}
			break;
		case HC_OTYPE_FIELD:
			{
				MCHcfield *fptr = hcfields->remove
				                  (hcfields);
				MCControl *newfield = fptr->build(hcsptr, sptr);
				newfield->setparent(gptr);
				newfield->appendto(gptr->controls);
				delete fptr;
			}
			break;
		}
	return gptr;
}

MCHcstak::MCHcstak(char *inname)
	: rect(kMCEmptyRectangle),
	  name(inname),
	  script(nullptr),
	  fullbuffer(nullptr),
	  buffer(nullptr),
	  pages(nullptr),
	  marks(nullptr),
	  npages(0),
	  fonts(nullptr),
	  atts(nullptr),
	  nfonts(0),
	  natts(0),
	  pagesize(0),
	  npbuffers(0),
	  pbuffersizes(nullptr),
	  pbuffers(nullptr),
	  hcbkgds(nullptr),
	  hccards(nullptr),
	  hcbmaps(nullptr),
	  icons(nullptr),
	  cursors(nullptr),
	  snds(nullptr)
{
}

MCHcstak::~MCHcstak()
{
	delete name;
	delete script;
	delete fullbuffer;
	delete pages;
	delete marks;
	delete fonts;
	delete atts;
	uint4 i;
	for (i = 0 ; i < npbuffers ; i++)
		delete pbuffers[i];
	if (pbuffers != NULL)
	{
		delete pbuffers;
		delete pbuffersizes;
	}
	while (hcbkgds != NULL)
	{
		MCHcbkgd *bgptr = hcbkgds->remove
		                  (hcbkgds);
		delete bgptr;
	}
	while (hcbmaps != NULL)
	{
		MCHcbmap *bmptr = hcbmaps->remove
		                  (hcbmaps);
		delete bmptr;
	}
	while (hccards != NULL)
	{
		MCHccard *cptr = hccards->remove
		                 (hccards);
		delete cptr;
	}
	while (icons != NULL)
	{
		MCHcbmap *bmptr = icons->remove
		                  (icons);
		delete bmptr;
	}
	while (cursors != NULL)
	{
		MCHcbmap *bmptr = cursors->remove
		                  (cursors);
		delete bmptr;
	}
	while (snds != NULL)
	{
		MCHcsnd *sptr = snds->remove
		                (snds);
		delete sptr;
	}
}

uint4 MCHcstak::geticon(uint2 iid)
{
	if (iid == 0)
		return 0;
	if (icons != NULL)
	{
		MCHcbmap *bptr = icons;
		do
		{
			if (bptr->id == iid)
				return iid;
			bptr = bptr->next();
		}
		while (bptr != icons);
	}
	uint2 i;
	for (i = 0 ; i < HC_NICONS ; i++)
		if (iid == hc_icons[i])
			return mc_icons[i];
	return HC_DEFAULT_ICON;
}

MCHcbmap *MCHcstak::getbmap(uint4 bid)
{
	MCHcbmap *bptr = hcbmaps;
	do
	{
		if (bptr->id == bid)
			return bptr;
		bptr = bptr->next();
	}
	while (bptr != hcbmaps);
	return NULL;
}

MCHcbmap *MCHcstak::removebmap(uint4 bid)
{
	MCHcbmap *bptr = getbmap(bid);
	return bptr->remove
	       (hcbmaps);
}

MCHcbkgd *MCHcstak::getbkgd(uint4 bid)
{
	MCHcbkgd *bptr = hcbkgds;
	do
	{
		if (bptr->id == bid)
			return bptr;
		bptr = bptr->next();
	}
	while (bptr != hcbkgds);
	return NULL;
}

const char *MCHcstak::getfont(uint2 fid)
{
	uint2 i;
	for (i = 0 ; i < nfonts ; i++)
		if (fonts[i].id == fid)
			return fonts[i].name;
	return NULL;
}

void MCHcstak::getatts(uint2 aid, const char *&font,
                       uint2 &size, uint2 &style)
{
	uint2 i;
	for (i = 0 ; i < natts ; i++)
		if (atts[i].id == aid)
		{
			if (atts[i].fid != 0xFFFF)
				font = getfont(atts[i].fid);
			else
				font = NULL;
			size = atts[i].size;
			style = atts[i].style;
			return;
		}
	font = NULL;
	size = 0xFFFF;
	style = FA_DEFAULT_STYLE;
}

IO_stat MCHcstak::read(IO_handle stream)
{
	char header[HC_HEADER_SIZE];
	uint4 boffset = 0;
	uint4 roffset = 0;
	uint4 rsize = 0;
	HC_File_type filetype = HC_RAW;
	uint4 *uint4buff = (uint4 *)header;
	uint2 *uint2buff;
	uint4 size;
	if (IO_read(header, HC_HEADER_SIZE, stream) != IO_NORMAL)
		return IO_ERROR;
	swap_uint4(&uint4buff[1]);
	if (uint4buff[1] == HC_STAK)
	{
		MCS_seek_set(stream, 0);
		filetype = HC_RAW;
	}
	else
	{
		swap_uint4(&uint4buff[1]);
		if (header[0] == '\0' && header[74] == '\0' && header[82] == '\0')
		{
			if (!strnequal(&header[65], "STAK", 4))
				return IO_ERROR;
			filetype = HC_MACBIN;
			delete name;
			name = new (nothrow) char[strlen(&header[2]) + 1];
			strcpy(name, &header[2]);
			uint1 *uint1ptr = (uint1 *)header;
			roffset = uint1ptr[83] << 24 | uint1ptr[84] << 16
			          | uint1ptr[85] << 8 | uint1ptr[86];
			roffset += 128 + 127;
			roffset &= 0xFFFFFF80;
			rsize = uint1ptr[87] << 24 | uint1ptr[88] << 16
			        | uint1ptr[89] << 8 | uint1ptr[90];
			MCS_seek_set(stream, 128);
		}
		else
		{
			header[255] = '\0';
			uint4 foffset;
			if (MCU_offset("BinHex", header, foffset))
			{
				filetype = HC_BINHEX;
				while(header[foffset] != ':')
				{
					foffset++;
					if (header[foffset] == '\0')
						return IO_ERROR;
				}
				MCS_seek_set(stream, ++foffset);
				uint4 fsize = (uint4)MCS_fsize(stream) - foffset;
				char *tbuffer = new (nothrow) char[fsize * 3 / 4];
				uint1 *uint1ptr = (uint1 *)tbuffer;
				uint1 byte;
				uint2 tcount = 0;
				uint1 tbuf[4];
				uint4 tsize = fsize;
				while (tsize--)
				{
					if (IO_read_uint1(&byte, stream) != IO_NORMAL)
					{
						delete[] tbuffer;
						return IO_ERROR;
					}
					while ((byte == '\n' || byte == '\r') && tsize--)
						if (IO_read_uint1(&byte, stream) != IO_NORMAL)
						{
							delete[] tbuffer;
							return IO_ERROR;
						}
					if (byte == ':')
						break;
					tbuf[tcount++] = hqx[byte - '!'];
					if (tcount == 4)
					{
						uint1ptr[boffset++] = uint1(tbuf[0] << 2 | tbuf[1] >> 4);
						uint1ptr[boffset++] = uint1(tbuf[1] << 4 | tbuf[2] >> 2);
						uint1ptr[boffset++] = uint1(tbuf[2] << 6 | tbuf[3]);
						tcount = 0;
					}
				}
				uint4 fullsize = fsize;
				fullbuffer = new (nothrow) char[fullsize];
				uint4 doffset = 0;
				uint1 *eptr = uint1ptr + boffset;
				while (uint1ptr < eptr)
				{
					uint1 t_byte = *uint1ptr++;
					uint2 count = 1;
					if (t_byte == 0x90 && *uint1ptr == 0)
						uint1ptr++;
					else
						if (*uint1ptr == 0x90)
						{
							uint1ptr++;
							count = *uint1ptr++;
							if (count == 0)
							{
								fullbuffer[doffset++] = byte;
								t_byte = 0x90;
								if (*uint1ptr == 0x90)
								{
									uint1ptr++;
									count = *uint1ptr++;
									if (count < 2)
										count = 2;
								}
								else
									count = 1;
							}
						}
					while (count--)
						fullbuffer[doffset++] = t_byte;
					if (doffset > fullsize - 256)
					{
						MCU_realloc((char **)&fullbuffer, fullsize,
						            fullsize + fsize, sizeof(uint1));
						fullsize += fsize;
					}
				}
				uint1ptr = (uint1 *)fullbuffer;
				delete name;
				name = new (nothrow) char[uint1ptr[0] + 1];
				strcpy(name, &tbuffer[1]);
				delete[] tbuffer;

				uint2 toffset = uint1ptr[0] + 2;
				if (!strnequal(&fullbuffer[toffset], "STAK", 4))
					return IO_ERROR;

				toffset += 10;
				roffset = uint1ptr[toffset] << 24 | uint1ptr[toffset + 1] << 16
					| uint1ptr[toffset + 2] << 8 | (uint1ptr[toffset + 3] + 2);
				toffset += 4;
				rsize = uint1ptr[toffset] << 24 | uint1ptr[toffset + 1] << 16
				        | uint1ptr[toffset + 2] << 8 | uint1ptr[toffset + 3];
				memcpy(fullbuffer, &fullbuffer[uint1ptr[0] + 22], roffset + rsize);
				boffset = 0;
			}
			else
				return IO_ERROR;
		}
	}
	uint2 i;
	uint4 type = 0;
	char *t_buffer;
	while (type != HC_TAIL)
	{
		if (filetype == HC_BINHEX)
		{
			t_buffer = &fullbuffer[boffset];
			uint2buff = (uint2 *)t_buffer;
			uint4buff = (uint4 *)t_buffer;
			type = swap_uint4(&uint4buff[1]);
			size = swap_uint4(&uint4buff[0]);
			boffset += size;
		}
		else
		{
			if (IO_read_uint4(&size, stream) != IO_NORMAL
			        || IO_read_uint4(&type, stream) != IO_NORMAL)
				return IO_ERROR;
			if (type == HC_BUGS)
				size = 512;
			fullbuffer = new (nothrow) char[size];
			if (IO_read(&fullbuffer[8], size - 8, stream) != IO_NORMAL)
				return IO_ERROR;
			t_buffer = fullbuffer;
			uint2buff = (uint2 *)t_buffer;
			uint4buff = (uint4 *)t_buffer;
		}
		uint2 offset;
		switch (type)
		{
		case HC_STAK:
			version = t_buffer[108];
			if (version > 2)
				return IO_ERROR;
			if (version == 0)
				version = 1;
			rect.width = swap_uint2(&uint2buff[221]);
			rect.height = swap_uint2(&uint2buff[220]);
			if (version == 1 || rect.width == 0 || rect.height == 0)
			{
				rect.width = 512;
				rect.height = 342;
			}
			script = convert_script(&t_buffer[1536]);
			break;
		case HC_LIST:
			if (version == 1)
				pagesize = swap_uint2(&uint2buff[12]);
			else
				pagesize = swap_uint2(&uint2buff[14]);
			break;
		case HC_PAGE:
			MCU_realloc((char **)&pbuffers, npbuffers,
			            npbuffers + 1, sizeof(char *));
			MCU_realloc((char **)&pbuffersizes, npbuffers,
			            npbuffers + 1, sizeof(uint2));
			pbuffersizes[npbuffers] = size;
	        pbuffers[npbuffers] = new (nothrow) char[size];
			memcpy(pbuffers[npbuffers++], t_buffer, size);
			break;
		case HC_BKGD:
			{
				MCHcbkgd *newbkgd = new (nothrow) MCHcbkgd;
				newbkgd->appendto(hcbkgds);
				if (newbkgd->parse(buffer) != IO_NORMAL)
					return IO_ERROR;
			}
			break;
		case HC_CARD:
			{
				MCHccard *newcard = new (nothrow) MCHccard;
				newcard->appendto(hccards);
				if (newcard->parse(t_buffer) != IO_NORMAL)
					return IO_ERROR;
			}
			break;
		case HC_BMAP:
			{
				MCHcbmap *newbmap = new (nothrow) MCHcbmap;
				newbmap->appendto(hcbmaps);
				if (newbmap->parse(buffer) != IO_NORMAL)
					return IO_ERROR;
			}
			break;
		case HC_STBL:
			if ((natts = swap_uint2(&uint2buff[9])) != 0)
			{
				atts = new (nothrow) Hcatts[natts];
				offset = 13;
				for (i = 0 ; i < natts ; i++)
				{
					atts[i].id = swap_uint2(&uint2buff[offset]);
					atts[i].fid = swap_uint2(&uint2buff[offset + 5]);
					atts[i].style = convert_style(swap_uint2(&uint2buff[offset + 6]));
					atts[i].size = swap_uint2(&uint2buff[offset + 7]);
					offset += 12;
				}
			}
			break;
		case HC_FTBL:
			if ((nfonts = swap_uint2(&uint2buff[9])) != 0)
			{
				fonts = new (nothrow) Hcfont[nfonts];
				offset = 12;
				for (i = 0 ; i < nfonts ; i++)
				{
					fonts[i].id = swap_uint2(&uint2buff[offset]);
					fonts[i].name = convert_font(&t_buffer[(offset + 1) * 2]);
					offset += (strlen(&t_buffer[(offset + 1) * 2]) + 4) >> 1;
				}
			}
			break;
		case HC_FREE:
		case HC_MAST:
		case HC_PRNT:
		case HC_PRFT:
		case HC_PRST:
		case HC_TAIL:
			break;
		default:
			hcstat_append("Error: unknown section type -> %x", type);

			if (filetype == HC_BINHEX)
				return IO_ERROR;
			break;
		}
		if (filetype != HC_BINHEX)
		{
			delete fullbuffer;
			fullbuffer = NULL;
		}
	}
	if (filetype == HC_RAW || rsize == 0)
	{
#ifdef _MAC_DESKTOP
		return macreadresources();
#else
			return IO_NORMAL;
#endif
	}
	if (filetype == HC_MACBIN)
	{
		fullbuffer = new (nothrow) char[rsize];
		MCS_seek_set(stream, roffset);
		if (MCS_readfixed(fullbuffer, rsize, stream) != IO_NORMAL)
			return IO_ERROR;
	}
	else
		memcpy(fullbuffer, &fullbuffer[roffset], rsize);
	iconx = icony = cursorx = cursory = 0;
	t_buffer = fullbuffer;
	uint4buff = (uint4 *)t_buffer;
	char *rdata = &t_buffer[swap_uint4(&uint4buff[0])];
	char *rheader = rdata + swap_uint4(&uint4buff[2]);
	uint2 typecount = get_uint2(&rheader[28]) + 1;
	char *strings = rheader + get_uint2(&rheader[26]);
	char *objects = &rheader[typecount * 8 + 30];
	for (i = 0 ; i < typecount ; i++)
	{
		uint4 t_type = get_uint4(&rheader[i * 8 + 30]);
		uint2 count = get_uint2(&rheader[i * 8 + 34]) + 1;
		while (count--)
		{
			uint2 id = get_uint2(objects);
			uint2 soffset = get_uint2(&objects[2]);
            
            MCNewAutoNameRef t_name;
            if (soffset == UINT16_MAX)
                t_name = kMCEmptyName;
            else
            {
                MCAutoStringRef t_name_string;
                /* UNCHECKED */ MCStringCreateWithPascalString((unsigned char*)&strings[soffset], &t_name_string);
                /* UNCHECKED */ MCNameCreate(*t_name_string, &t_name);
            }
            
			uint4 offset = get_uint4(&objects[4]);
			offset &= 0xFFFFFF;
			switch (t_type)
			{
			case HC_ICON:
				{
					MCHcbmap *newicon = new (nothrow) MCHcbmap;
					newicon->appendto(icons);
					newicon->icon(id, *t_name, &rdata[offset + 4]);
				}
				break;
			case HC_CURS:
				{
					MCHcbmap *newcurs = new (nothrow) MCHcbmap;
					newcurs->appendto(cursors);
					newcurs->cursor(id, *t_name, &rdata[offset + 4]);
				}
				break;
			case HC_SND:
				{
					MCHcsnd *newsnd = new (nothrow) MCHcsnd;
					if (newsnd->import(id, *t_name, &rdata[offset + 4]))
						newsnd->appendto(snds);
					else
						delete newsnd;
				}
				break;
			default:
                {
                    MCAutoStringRefAsCString t_name_str;
                    /* UNCHECKED */ t_name_str.Lock(MCNameGetString(*t_name));
                    hcstat_append("Not converting %4.4s id %5d \"%s\"",
                                  (char *)&t_type, id, *t_name_str);
                }
				break;
			}
			objects += 12;
		}
	}
	return IO_NORMAL;
}

MCStack *MCHcstak::build()
{
	MCStack *sptr = MCtemplatestack->clone();
	sptr->flags |= F_HC_STACK | F_HC_ADDRESSING | F_DYNAMIC_PATHS;
	const char *tname = strrchr(name, PATH_SEPARATOR);
	if (tname != NULL)
		tname += 1;
	else
		tname = name;
	sptr->setname_cstring(tname);
	delete name;
	if (script != NULL)
	{
		sptr -> setscript_cstring(script);
		delete script;
		if (sptr->hlist != NULL)
		{
			delete sptr->hlist;
			sptr->hlist = NULL;
		}
	}
	name = script = NULL;
	sptr->rect = rect;
	// MW-2012-02-17: [[ LogFonts ]] Set the font attributes of the object.
	sptr -> setfontattrs(MCSTR(HC_DEFAULT_TEXT_FONT), HC_DEFAULT_TEXT_SIZE, FA_DEFAULT_STYLE);
	sptr->fontheight = heightfromsize(HC_DEFAULT_TEXT_SIZE);
	uint4 i;
	for (i = 0 ; i < npbuffers ; i++)
	{
		buffer = pbuffers[i];
		uint4 *uint4buff = (uint4 *)buffer;
		MCU_realloc((char **)&pages, npages,
		            npages + pbuffersizes[i] / pagesize + 1, sizeof(uint4));
		MCU_realloc((char **)&marks, npages,
		            npages + pbuffersizes[i] / pagesize + 1, sizeof(uint1));
		uint2 offset = 0;
		do
		{
			// MW-2008-02-12: [[ Bug 5232 ]] Crash when importing certain HC stack - just added
			//   bounds checks.
			if ((offset * (pagesize / 4) + 6) * 4 + 4 > pbuffersizes[i])
				break;

			// MW-2008-02-27: [[ Bug 5977 ]] Originally adjusted for shorts.

			if ((offset * pagesize + 28) + 1 > pbuffersizes[i])
				break;
			pages[npages] = swap_uint4(&uint4buff[offset * (pagesize / 4) + 6]);
			marks[npages] = buffer[offset * pagesize + 28] & 0x10;
			offset++;
		}
		while (pages[npages++] != 0
		        && (offset - 1) * pagesize + 28 < pbuffersizes[i]);
		npages--;
		delete pbuffers[i];
	}
	if (pbuffers != NULL)
	{
		npbuffers = 0;
		delete pbuffers;
		pbuffers = NULL;
		delete pbuffersizes;
		pbuffersizes = NULL;
	}
	if (pages == NULL)
		while (hccards != NULL)
		{
			MCHccard *cptr = hccards->remove
			                 (hccards);
			MCCard *newcard = cptr->build(this, sptr);
			newcard->setparent(sptr);
			newcard->appendto(sptr->cards);
			delete cptr;
		}
	else
	{
		for (i = 0 ; i < npages ; i++)
		{
			MCHccard *cptr = hccards;
			do
			{
				if (cptr->id == pages[i])
				{
					cptr->remove
					(hccards);
					MCCard *newcard = cptr->build(this, sptr);
					newcard->setmark(marks[i]);
					newcard->setparent(sptr);
					newcard->appendto(sptr->cards);
					delete cptr;
					break;
				}
				cptr = cptr->next();
			}
			while (cptr != hccards);
		}
	}
	while (hcbkgds != NULL)
	{
		MCHcbkgd *bgptr = hcbkgds->remove
		                  (hcbkgds);
		MCControl *newobj = bgptr->build(this, sptr);
		newobj->setparent(sptr);
		newobj->appendto(sptr->controls);
		delete bgptr;
	}
	while (hcbmaps != NULL)
	{
		MCHcbmap *bmptr = hcbmaps->remove
		                  (hcbmaps);
		MCControl *newobj = bmptr->build();
		if (newobj != NULL)
		{
			newobj->setparent(sptr);
			newobj->appendto(sptr->controls);
		}
		delete bmptr;
	}
	if (icons != NULL)
	{
		MCGroup *gptr = (MCGroup *)MCtemplategroup->clone(False, OP_NONE, false);
		// MW-2011-08-09: [[ Groups ]] HC backgrounds are shared.
		gptr->flags = (gptr -> flags & ~F_GROUP_ONLY) | F_GROUP_SHARED;
		gptr->obj_id = ++maxid;
		gptr->setname_cstring("HC Icons");
		gptr->setparent(sptr);
		MCControl *cptr = (MCControl *)gptr;
		cptr->appendto(sptr->controls);
		while (icons != NULL)
		{
			MCHcbmap *bmptr = icons->remove
			                  (icons);
			MCControl *newobj = bmptr->build();
			if (newobj != NULL)
			{
				newobj->setparent(gptr);
				newobj->appendto(gptr->controls);
			}
			delete bmptr;
		}
	}
	if (cursors != NULL)
	{
		MCGroup *gptr = (MCGroup *)MCtemplategroup->clone(False, OP_NONE, false);
		gptr->obj_id = ++maxid;
		// MW-2011-08-09: [[ Groups ]] HC backgrounds are shared.
		gptr->flags = (gptr -> flags & ~F_GROUP_ONLY) | F_GROUP_SHARED;
		gptr->setname_cstring("HC cursors");
		gptr->setparent(sptr);
		MCControl *cptr = (MCControl *)gptr;
		cptr->appendto(sptr->controls);
		while (cursors != NULL)
		{
			MCHcbmap *bmptr = cursors->remove
			                  (cursors);
			MCControl *newobj = bmptr->build();
			if (newobj != NULL)
			{
				newobj->setparent(gptr);
				newobj->appendto(gptr->controls);
			}
			delete bmptr;
		}
	}
	while (snds != NULL)
	{
		MCHcsnd *sndptr = snds->remove
		                  (snds);
		MCAudioClip *aptr = sndptr->build();
		aptr->setparent(sptr);
		aptr->appendto(sptr->aclips);
		delete sndptr;
	}
	sptr->obj_id = maxid;
	return sptr;
}


IO_stat hc_import(MCStringRef name, IO_handle stream, MCStack *&sptr)
{
	maxid = 0;
	MCValueAssign(MChcstat, kMCEmptyString);

    char* t_name;
    if(!MCStringConvertToCString(name, t_name))
		return IO_ERROR;
	
    MCHcstak *hcstak = new (nothrow) MCHcstak(t_name);
	hcstat_append("Loading stack %s...", t_name);
	uindex_t startlen = MCStringGetLength(MChcstat);
	IO_stat stat;
	if ((stat = hcstak->read(stream)) == IO_NORMAL)
		sptr = hcstak->build();
	
	delete hcstak;
	if (!MClockerrors && MCStringGetLength(MChcstat) != startlen)
	{
		MCStack *tptr = MCdefaultstackptr->findstackname(MCN_hcstat);
		if (tptr != NULL)
		{
			sptr->open();
			tptr->openrect(MCdefaultstackptr->getrect(), WM_MODELESS,
			               NULL, WP_DEFAULT, OP_NONE);
		}
	}
	return stat;
}


