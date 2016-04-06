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

#ifndef W32DEFS_H
#define W32DEFS_H

#define Bool int

typedef struct
{
	uint4 pixel;
	uint2 red, green, blue;
	uint1 flags;
	uint1 pad;
}
MCColor;

typedef struct
{
	int2 x1, y1, x2, y2;
}
MCLineSegment;

typedef struct
{
	int2 x, y;
}
MCPoint;

typedef struct
{
	int2 x, y;
	uint2 width, height;
}
MCRectangle;

typedef struct
{
	uint2 width, height;
	uint2 format;
	char *data;
	uint1 byte_order;
	uint1 bitmap_unit;
	uint1 bitmap_bit_order;
	uint1 bitmap_pad;
	uint1 depth;
	uint1 bits_per_pixel;
	uint4 bytes_per_line;
	uint4 red_mask;
	uint4 green_mask;
	uint4 blue_mask;
	HBITMAP bm;
}
MCBitmap;

typedef HFONT              Font;
typedef HRGN               MCSysRegionHandle;

typedef struct
{
	int dummy;
}
XEvent;

typedef struct
{
	Font fid;
	int ascent;	//log extent above baseline for spacing.LONG lMaxAscender
	int descent;  //log. descent below baseline for spacing
	Boolean printer;
	uint1 widths[256];
	uint1 charset;
	Boolean unicode;
}
MCFontStruct;

enum {
    DC_WINDOW,
    DC_BITMAP,
		DC_BITMAP_WITH_DC
};

#define DNULL                ((_Drawable *)0)

#define DoRed                0x1    //to be used in the MCColor.flag
#define DoGreen              0x2    // ..
#define DoBlue               0x4    // ..
#define None                 0
#define Button1              1
#define Button2              2
#define Button3              3
#define HF_STDOUT            1     //0x00000001
#define HF_STDIN             0     //0x00000000
#define HF_STDERR            2     //0x00000002

typedef struct drawable
{
	uint4 type;
	union {
		HWND window;  //is client window when there is a frame window, frame != NULL
		HBITMAP pixmap;
	} handle;
}
_Drawable;

struct _ExtendedDrawable: public _Drawable
{
	HDC hdc;
};

typedef  _Drawable *        XID;
typedef  _Drawable *        Window;
typedef  _Drawable *        Pixmap;
typedef  _Drawable *        Drawable;
typedef unsigned long       KeySym;
typedef unsigned long       Atom;

struct MCCursor;
typedef MCCursor *MCCursorRef;

// X raster opcodes
#define	GXclear			0x0		/* 0 */
#define GXand			0x1		/* src AND dst */
#define GXandReverse		0x2		/* src AND NOT dst */
#define GXcopy			0x3		/* src */
#define GXandInverted		0x4		/* NOT src AND dst */
#define	GXnoop			0x5		/* dst */
#define GXxor			0x6		/* src XOR dst */
#define GXor			0x7		/* src OR dst */
#define GXnor			0x8		/* NOT src AND NOT dst */
#define GXequiv			0x9		/* NOT src XOR dst */
#define GXinvert		0xa		/* NOT dst */
#define GXorReverse		0xb		/* src OR NOT dst */
#define GXcopyInverted		0xc		/* NOT src */
#define GXorInverted		0xd		/* NOT src OR dst */
#define GXnand			0xe		/* NOT src OR NOT dst */
#define GXset			0xf		/* 1 */
#define GXsrcBic		0x10
#define GXnotSrcBic		0x11

#define GXblend			0x12
#define GXaddpin		0x13
#define GXaddOver		0x14
#define GXsubPin		0x15
#define GXtransparent		0x16
#define GXaddMax		0x17
#define GXsubOver		0x18
#define GXaddMin		0x19

#define GXblendClear 0x1a
#define GXblendSrc 0x1b
#define GXblendDst 0x1c
#define GXblendSrcOver 0x1d
#define GXblendDstOver 0x1e
#define GXblendSrcIn 0x1f
#define GXblendDstIn 0x20
#define GXblendSrcOut 0x21
#define GXblendDstOut 0x22
#define GXblendSrcAtop 0x23
#define GXblendDstAtop 0x24
#define GXblendXor 0x25
#define GXblendPlus 0x26
#define GXblendMultiply 0x27
#define GXblendScreen 0x28

#define GXblendOverlay 0x29
#define GXblendDarken 0x2a
#define GXblendLighten 0x2b
#define GXblendDodge 0x2c
#define GXblendBurn 0x2d
#define GXblendHardLight 0x2e
#define GXblendSoftLight 0x2f
#define GXblendDifference 0x30
#define GXblendExclusion 0x31

#define NUM_INKS		0x32

#define LineSolid            1
#define LineOnOffDash        2
#define LineDoubleDash       3

#define MAX_DASHES           8
#define WM_TITLE_HEIGHT      16

#define CapButt				1
#define CapRound			2
#define CapProjecting		3

#define JoinRound            PS_JOIN_ROUND
#define JoinMiter            PS_JOIN_MITER
#define JoinBevel            PS_JOIN_BEVEL

//Map X's Fill style
#define FillSolid                       1
#define FillTiled                       2
#define FillStippled                    3
#define FillOpaqueStippled              4

//X Screen color type
#define StaticGray		0
#define GrayScale		1
#define StaticColor		2
#define PseudoColor		3
#define TrueColor		4
#define DirectColor		5

/* Byte order  used in imageByteOrder and bitmapBitOrder */
#define LSBFirst		0
#define MSBFirst		1

#define AllPlanes               0xFFFFFF
#define XYPixmap                0
#define ZPixmap                 1

#define FULL_CIRCLE      23040    //360 degrees *64

//PM resources ID defines
#define ID_METACARD    256

//Keysym defines in X window
#define XK_BackSpace		0xFF08	/* back space, back char */
#define XK_space                0x020
#define XK_Tab			0xFF09
#define XK_Linefeed		0xFF0A	/* Linefeed, LF */
#define XK_Clear		0xFF0B
#define XK_Return		0xFF0D	/* Return, enter */
#define XK_Pause		0xFF13	/* Pause, hold */
#define XK_Scroll_Lock		0xFF14
#define XK_Sys_Req		0xFF15
#define XK_Escape		0xFF1B
#define XK_Delete		0xFFFF	/* Delete, rubout */

#define XK_Home			0xFF50
#define XK_Left			0xFF51	/* Move left, left arrow */
#define XK_Up			0xFF52	/* Move up, up arrow */
#define XK_Right		0xFF53	/* Move right, right arrow */
#define XK_Down			0xFF54	/* Move down, down arrow */
#define XK_Prior		0xFF55	/* Prior, previous */
#define XK_Next			0xFF56	/* Next */
#define XK_End			0xFF57	/* EOL */
#define XK_Begin		0xFF58	/* BOL */

#define XK_Select		0xFF60	/* Select, mark */
#define XK_Print		0xFF61
#define XK_Execute		0xFF62	/* Execute, run, do */
#define XK_Insert		0xFF63	/* Insert, insert here */
#define XK_Undo			0xFF65	/* Undo, oops */
#define XK_Redo			0xFF66	/* redo, again */
#define XK_Menu			0xFF67
#define XK_Find			0xFF68	/* Find, search */
#define XK_Cancel		0xFF69	/* Cancel, stop, abort, exit */
#define XK_Help			0xFF6A	/* Help */
#define XK_Break		0xFF6B
#define XK_Mode_switch		0xFF7E	/* Character set switch */
#define XK_script_switch        0xFF7E  /* Alias for mode_switch */
#define XK_Num_Lock		0xFF7F

#define XK_KP_Space		0xFF80	/* space */
#define XK_KP_Tab		0xFF89
#define XK_KP_Enter		0xFF8D	/* enter */
#define XK_KP_F1		0xFF91	/* PF1, KP_A, ... */
#define XK_KP_F2		0xFF92
#define XK_KP_F3		0xFF93
#define XK_KP_F4		0xFF94
#define XK_KP_Begin		0xFF95
#define XK_KP_Home		0xFF96
#define XK_KP_Left		0xFF97
#define XK_KP_Up		0xFF98
#define XK_KP_Right		0xFF99
#define XK_KP_Down		0xFF9A
#define XK_KP_Prior		0xFF9B
#define XK_KP_Next		0xFF9C
#define XK_KP_End		0xFF9D
#define XK_KP_Insert		0xFF9E
#define XK_KP_Delete		0xFF9F

#define XK_KP_Equal		0xFFBD	/* equals */
#define XK_KP_Multiply		0xFFAA
#define XK_KP_Add		0xFFAB
#define XK_KP_Separator		0xFFAC	/* separator, often comma */
#define XK_KP_Subtract		0xFFAD
#define XK_KP_Decimal		0xFFAE
#define XK_KP_Divide		0xFFAF

#define XK_KP_0			0xFFB0
#define XK_KP_1			0xFFB1
#define XK_KP_2			0xFFB2
#define XK_KP_3			0xFFB3
#define XK_KP_4			0xFFB4
#define XK_KP_5			0xFFB5
#define XK_KP_6			0xFFB6
#define XK_KP_7			0xFFB7
#define XK_KP_8			0xFFB8
#define XK_KP_9			0xFFB9

#define XK_F1			0xFFBE
#define XK_F2			0xFFBF
#define XK_F3			0xFFC0
#define XK_F4			0xFFC1
#define XK_F5			0xFFC2
#define XK_F6			0xFFC3
#define XK_F7			0xFFC4
#define XK_F8			0xFFC5
#define XK_F9			0xFFC6
#define XK_F10			0xFFC7
#define XK_F11			0xFFC8
#define XK_F12			0xFFC9
#define XK_F13			0xFFCA
#define XK_F14			0xFFCB
#define XK_F15			0xFFCC
#define XK_F16			0xFFCD
#define XK_F17			0xFFCE
#define XK_F18			0xFFCF
#define XK_F19			0xFFD0
#define XK_F20			0xFFD1
#define XK_F21			0xFFD2
#define XK_F22			0xFFD3
#define XK_F23			0xFFD4
#define XK_F24			0xFFD5
#define XK_F25			0xFFD6
#define XK_F26			0xFFD7
#define XK_F27			0xFFD8
#define XK_F28			0xFFD9
#define XK_F29			0xFFDA
#define XK_F30			0xFFDB
#define XK_F31			0xFFDC
#define XK_F32			0xFFDD
#define XK_F33			0xFFDE
#define XK_F34			0xFFDF
#define XK_F35			0xFFE0

#define XK_L1                    0xFFC8
#define XK_L2                    0xFFC9
#define XK_L3                    0xFFCA
#define XK_L4                    0xFFCB
#define XK_L5                    0xFFCC
#define XK_L6                    0xFFCD
#define XK_L7                    0xFFCE
#define XK_L8                    0xFFCF
#define XK_L9                    0xFFD0
#define XK_L10                   0xFFD1

#define XK_R1			0xFFD2
#define XK_R2			0xFFD3
#define XK_R3			0xFFD4
#define XK_R4			0xFFD5
#define XK_R5			0xFFD6
#define XK_R6			0xFFD7
#define XK_R7			0xFFD8
#define XK_R8			0xFFD9
#define XK_R9			0xFFDA
#define XK_R10			0xFFDB
#define XK_R11			0xFFDC
#define XK_R12			0xFFDD
#define XK_R13			0xFFDE
#define XK_R14			0xFFDF
#define XK_R15			0xFFE0

#define XK_Shift_L		0xFFE1	/* Left shift */
#define XK_Shift_R		0xFFE2	/* Right shift */
#define XK_Control_L		0xFFE3	/* Left control */
#define XK_Control_R		0xFFE4	/* Right control */
#define XK_Caps_Lock		0xFFE5	/* Caps lock */
#define XK_Shift_Lock		0xFFE6	/* Shift lock */

#define XK_Meta_L		0xFFE7	/* Left meta */
#define XK_Meta_R		0xFFE8	/* Right meta */
#define XK_Alt_L		0xFFE9	/* Left alt */
#define XK_Alt_R		0xFFEA	/* Right alt */
#define XK_Super_L		0xFFEB	/* Left super */
#define XK_Super_R		0xFFEC	/* Right super */
#define XK_Hyper_L		0xFFED	/* Left hyper */
#define XK_Hyper_R		0xFFEE	/* Right hyper */

#define XK_apCut        0x1000FF03
#define XK_osfCut       0x1004FF03
#define XK_SunCut       0x1005FF75
#define XK_apCopy       0x1000FF02
#define XK_osfCopy      0x1004FF02
#define XK_SunCopy      0x1005FF72
#define XK_apPaste      0x1000FF04
#define XK_osfPaste     0x1004FF04
#define XK_SunPaste     0x1005FF74
#define XK_osfUndo      0x1004FF65
#define XK_osfHelp      0x1004FF6A
#define XK_hpInsertChar 0x1000FF72
#define XK_osfInsert    0x1004FF63
#define XK_hpDeleteChar 0x1000FF73
#define XK_osfDelete    0x1004FFFF

#define XK_A                   0x041
#define XK_B                   0x042
#define XK_C                   0x043
#define XK_D                   0x044
#define XK_E                   0x045
#define XK_F                   0x046
#define XK_G                   0x047
#define XK_H                   0x048
#define XK_I                   0x049
#define XK_J                   0x04a
#define XK_K                   0x04b
#define XK_L                   0x04c
#define XK_M                   0x04d
#define XK_N                   0x04e
#define XK_O                   0x04f
#define XK_P                   0x050
#define XK_Q                   0x051
#define XK_R                   0x052
#define XK_S                   0x053
#define XK_T                   0x054
#define XK_U                   0x055
#define XK_V                   0x056
#define XK_W                   0x057
#define XK_X                   0x058
#define XK_Y                   0x059
#define XK_Z                   0x05a
#define XK_bracketleft         0x05b
#define XK_backslash           0x05c
#define XK_bracketright        0x05d
#define XK_asciicircum         0x05e
#define XK_underscore          0x05f
#define XK_grave               0x060
#define XK_quoteleft           0x060	/* deprecated */
#define XK_a                   0x061
#define XK_b                   0x062
#define XK_c                   0x063
#define XK_d                   0x064
#define XK_e                   0x065
#define XK_f                   0x066
#define XK_g                   0x067
#define XK_h                   0x068
#define XK_i                   0x069
#define XK_j                   0x06a
#define XK_k                   0x06b
#define XK_l                   0x06c
#define XK_m                   0x06d
#define XK_n                   0x06e
#define XK_o                   0x06f
#define XK_p                   0x070
#define XK_q                   0x071
#define XK_r                   0x072
#define XK_s                   0x073
#define XK_t                   0x074
#define XK_u                   0x075
#define XK_v                   0x076
#define XK_w                   0x077
#define XK_x                   0x078
#define XK_y                   0x079
#define XK_z                   0x07a

#endif
