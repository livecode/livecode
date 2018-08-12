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

#ifndef __MC_SYSDEFS__
#define __MC_SYSDEFS__


#include "globdefs.h"


//////////////////////////////////////////////////////////////////////
//
//  MODE AND FEATURE DEFINITIONS
//

#ifdef MODE_DEVELOPMENT
#define FEATURE_PROPERTY_LISTENER
#endif

#if defined(_WINDOWS_DESKTOP)

#define PLATFORM_STRING "Win32"

#define MCSSL
#define FEATURE_TASKBAR_ICON
#define FEATURE_PLATFORM_PLAYER
#define FEATURE_RELAUNCH_SUPPORT
#define FEATURE_NOTIFY 1

#elif defined(_MAC_DESKTOP)

#define PLATFORM_STRING "MacOS"

#define MCSSL
#define FEATURE_TASKBAR_ICON
#define FEATURE_PLATFORM_APPLICATION
#define FEATURE_PLATFORM_PLAYER
#define FEATURE_PLATFORM_WINDOW
#define FEATURE_PLATFORM_RECORDER
#define FEATURE_PLATFORM_AUDIO
#define FEATURE_NOTIFY 1

// QuickTime is not supported in 64-bit OSX applications as it has been
// deprecated by Apple.
#ifndef __LP64__
#define FEATURE_QUICKTIME
#define FEATURE_QUICKTIME_EFFECTS
#endif

#elif defined(_LINUX_DESKTOP)

#define PLATFORM_STRING "Linux"

#define MCSSL
#define FEATURE_MPLAYER
#define FEATURE_NOTIFY 1

#elif defined(_WINDOWS_SERVER)

#define PLATFORM_STRING "Win32"

#define MCSSL
#define FEATURE_NOTIFY 1

#elif defined(_MAC_SERVER)

#define PLATFORM_STRING "MacOS"

#define MCSSL
#define FEATURE_NOTIFY 1

#elif defined(_LINUX_SERVER) || defined(_DARWIN_SERVER)

#define PLATFORM_STRING "Linux"

#define MCSSL
#define FEATURE_NOTIFY 1

#elif defined(_IOS_MOBILE)

#define MCSSL
#define __MACROMAN__
#define __LF__
#define PLATFORM_STRING "iphone"

#define FEATURE_PLATFORM_URL 1
#define FEATURE_NOTIFY 1

#elif defined(_ANDROID_MOBILE)

#define MCSSL
#define __ISO_8859_1__
#define __LF__
#define PLATFORM_STRING "android"

#define FEATURE_PLATFORM_URL 1
#define FEATURE_NOTIFY 1

#elif defined(__EMSCRIPTEN__)

#define PLATFORM_STRING "HTML5"

#define FEATURE_PLATFORM_URL 1

#endif

//////////////////////////////////////////////////////////////////////
//
//  FOUNDATION TYPES
//

#include <foundation.h>
#include <foundation-auto.h>
#include <foundation-unicode.h>
#include <foundation-bidi.h>

#ifdef __OBJC__
#include <foundation-objc.h>
#endif

//////////////////////////////////////////////////////////////////////
//
//  FOUNDATION SYSTEM LIBRARY
//

#include <foundation-system.h>

//////////////////////////////////////////////////////////////////////
//
//  COMPILER AND CODE GENERATION DEFINES
//

#if defined(_MSC_VER)
#define _HAS_VSCPRINTF
#define _HAS_QSORT_S
#elif defined(_LINUX_DESKTOP) || defined(_LINUX_SERVER) || defined(__EMSCRIPTEN__)
#define _HAS_VSNPRINTF
#undef _HAS_QSORT_R
#elif defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(_DARWIN_SERVER) || defined(_IOS_MOBILE)
#define _HAS_VSNPRINTF
#define _HAS_QSORT_R
#elif defined(_ANDROID_MOBILE)
#define _HAS_VSNPRINTF
#undef _HAS_QSORT_R
#else
#error Unknown compiler being used.
#endif

// The engine is implemented assuming that 'bool' is at most one byte in size
static_assert(sizeof(bool) <= 1, "Bool size is not at most 1 byte");

//////////////////////////////////////////////////////////////////////
//
//  LEGACY INCLUDES AND DEFINES
//

class MCString;
#include "typedefs.h"
#include "foundation-legacy.h"
#include "rawarray.h"

// The 'CHARSET' define is used to determine the direction of char
// translation when reading in stacks. If the stack's charset byte
// matches CHARSET no translation is done, otherwise a swap between
// latin. and mac is done.
//
#if defined(__MACROMAN__)
#define CHARSET 1
#else
#define CHARSET 0
#endif

//////////////////////////////////////////////////////////////////////
//
//  HANDLE DEFINITIONS
//

// Any system handles that are present in non-platform specific data
// structures use one of these types. They have to be manually cast
// to the actual type when used.
//
typedef struct __MCSysModuleHandle *MCSysModuleHandle;
typedef struct __MCSysBitmapHandle *MCSysBitmapHandle;
typedef struct __MCSysWindowHandle *MCSysWindowHandle;
typedef struct __MCSysFontHandle *MCSysFontHandle;
typedef struct __MCSysContextHandle *MCSysContextHandle;

typedef class MCPlatformWindow *MCPlatformWindowRef;
typedef class MCPlatformSurface *MCPlatformSurfaceRef;
typedef class MCPlatformCursor *MCPlatformCursorRef;
typedef class MCPlatformPasteboard *MCPlatformPasteboardRef;
typedef class MCPlatformMenu *MCPlatformMenuRef;
typedef class MCPlatformPlayer *MCPlatformPlayerRef;

typedef void *MCColorTransformRef;

#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER)
typedef MCPlatformCursorRef MCCursorRef;
#else
typedef struct MCCursor *MCCursorRef;
#endif

//////////////////////////////////////////////////////////////////////
//
//  PLATFORM SPECIFIC DEFINES AND INCLUDES
//

#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)

// Ensure we don't get deprecate warnings for things.
#undef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#undef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#undef _CRT_DISABLE_PERFCRIT_LOCKS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <assert.h>

// Replace tolower() and toupper() with versions that by-pass any thread
// issues - not doing this has a serious performance impact on string
// comparisons.
inline uint1 MCS_tolower(uint1 p_char) {return _isupper_l(p_char, NULL) ? _tolower_l(p_char, NULL) : p_char;}
inline uint1 MCS_toupper(uint1 p_char) {return _toupper_l(p_char, NULL);}

// Although less of an issue than tolower/toupper, redefining these
// is for the best, as it ensures best performance.
#define isalpha(x) _isalpha_l(x, NULL)
#define isupper(x) _isupper_l(x, NULL)
#define islower(x) _islower_l(x, NULL)
#define isdigit(x) _isdigit_l(x, NULL)
#define isspace(x) _isspace_l(x, NULL)
#define isxdigit(x) _isxdigit_l(x, NULL)
#define ispunct(x) _ispunct_l(x, NULL)
#define isalnum(x) _isalnum_l(x, NULL)
#define isprint(x) _isprint_l(x, NULL)
#define isgraph(x) _isgraph_l(x, NULL)
#define iscntrl(x) _iscntrl_l(x, NULL)

#define EINVAL          22
#define ERANGE          34

#define SIGKILL 9
#define SIGTERM 15

class CDropTarget;

#define fixmaskrop(a) ((a == GXand || a == GXor)?(a == GXand?GXor:GXand):(a == GXandInverted?GXorInverted:GXandInverted))//DEBUG
#define fixmaskcolor(a) (MCColorGetPixel(a) == 0 ? MConecolor:MCzerocolor)//DEBUG

typedef uintptr_t MCSocketHandle;

typedef struct __MCWinSysHandle *MCWinSysHandle;
typedef struct __MCWinSysIconHandle *MCWinSysIconHandle;
typedef struct __MCWinSysMetafileHandle *MCWinSysMetafileHandle;
typedef struct __MCWinSysEnhMetafileHandle *MCWinSysEnhMetafileHandle;

#if defined(_DEBUG)

#include <crtdbg.h>

#define _DEBUG_MEMORY

inline void *operator new(size_t size, std::nothrow_t, const char *fnm, int line) throw () {return _malloc_dbg(size, _NORMAL_BLOCK, fnm, line);}
inline void *operator new[](size_t size, std::nothrow_t, const char *fnm, int line) throw () {return _malloc_dbg(size, _NORMAL_BLOCK, fnm, line);}

inline void *operator new(size_t, void *p, const char *, long)
{
	return p;
}

#define new(...) new(__VA_ARGS__, __FILE__, __LINE__ )
#define delete delete

#define malloc(len) _malloc_dbg(len, _NORMAL_BLOCK, __FILE__,__LINE__)
#define free(b) _free_dbg(b, _NORMAL_BLOCK)
#define realloc(b, size) _realloc_dbg(b, size, _NORMAL_BLOCK, __FILE__, __LINE__)

extern void _dbg_MCU_realloc(char **data, uint4 osize, uint4 nsize, uint4 csize, const char *f, int l);
#define MCU_realloc(a, b, c, d) _dbg_MCU_realloc(a, b, c, d, __FILE__, __LINE__)

#endif

// VS before 2013 doesn't provide this function
inline float roundf(float f) { return f >= 0.0f ? floorf(f + 0.5f) : ceilf(f - 0.5f); }

// MW-2010-10-14: This constant is the amount of 'extra' stack space ensured to be present
//   after a recursionlimit check has failed.
#define MC_UNCHECKED_STACKSIZE 65536U

struct MCFontStruct
{
	MCSysFontHandle fid;
	uint16_t size;
	Boolean printer;
    
    coord_t m_ascent;
    coord_t m_descent;
    coord_t m_leading;
    coord_t m_xheight;
};

#define SECONDS_MIN 0.0
#define SECONDS_MAX 32535244799.0

#elif defined(_MAC_DESKTOP)

/*#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <assert.h>*/

typedef int MCSocketHandle;

typedef struct __MCMacSysHandle *MCMacSysHandle;
typedef struct __MCMacSysMenuHandle *MCMacSysMenuHandle;
typedef struct __MCMacSysPictHandle *MCMacSysPictHandle;
typedef struct __MCMacSysPixMapHandle *MCMacSysPixMapHandle;
typedef struct __MCMacSysBitMapHandle *MCMacSysBitMapHandle;
typedef struct __MCMacSysBitMapHandle *MCMacSysPortHandle;

typedef struct CGImage *CGImageRef;
typedef struct __CFSocket * CFSocketRef;
typedef struct __CFRunLoopSource * CFRunLoopSourceRef;

typedef struct Rect MCMacSysRect;

struct MCMacProcessSerialNumber
{
	uint32_t highLongOfPSN;
	uint32_t lowLongOfPSN;
};

extern uint1 *MClowercasingtable;
inline uint1 MCS_tolower(uint1 p_char)
{
	return MClowercasingtable[p_char];
}

extern uint1 *MCuppercasingtable;
inline uint1 MCS_toupper(uint1 p_char)
{
	return MCuppercasingtable[p_char];
}

struct MCFontStruct
{
	MCSysFontHandle fid;
	uint2 size;
	uint2 style;
    
    coord_t m_ascent;
    coord_t m_descent;
    coord_t m_em;
    coord_t m_xheight;
    coord_t m_capheight;
    coord_t m_leading;
};

#define fixmaskrop(a) (a)
#define fixmaskcolor(a) (a)

#define SECONDS_MIN -32535244799.0
#define SECONDS_MAX 32535244799.0

#elif defined(_LINUX_DESKTOP) || defined(_LINUX_SERVER) || defined(__EMSCRIPTEN__)

#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <assert.h>
#ifndef _CTYPE_H
#define _CTYPE_H
#endif

#define SIGBOGUS 100

typedef int MCSocketHandle;

extern uint1 MClowercasingtable[];
inline uint1 MCS_tolower(uint1 p_char)
{
	return MClowercasingtable[p_char];
}

extern uint1 MCuppercasingtable[];
inline uint1 MCS_toupper(uint1 p_char)
{
	return MCuppercasingtable[p_char];
}

extern uint2 MCctypetable[];
#define _ctype(x, y) ((MCctypetable[(uindex_t) (x)] & (1 << (y))) != 0)
#define isalpha(x) (_ctype(x, 0))
#define isupper(x) (_ctype(x, 1))
#define islower(x) (_ctype(x, 2))
#define isdigit(x) (_ctype(x, 3))
#define isspace(x) (_ctype(x, 4))
#define isxdigit(x) (_ctype(x, 5))
#define ispunct(x) (_ctype(x, 6))
#define isalnum(x) (_ctype(x, 7))
#define isprint(x) (_ctype(x, 8))
#define isgraph(x) (_ctype(x, 9))
#define iscntrl(x) (_ctype(x, 10))

struct MCFontStruct
{
    MCSysFontHandle fid;
    uint16_t size;
    
    coord_t m_ascent;
    coord_t m_descent;
    coord_t m_leading;
    coord_t m_xheight;
};

#define fixmaskrop(a) (a)
#define fixmaskcolor(a) (a)

#define SECONDS_MIN 0.0
#define SECONDS_MAX 32535244799.0

#elif defined(_MAC_SERVER) || defined(_DARWIN_SERVER)

#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <assert.h>

struct MCMacProcessSerialNumber
{
	uint32_t highLongOfPSN;
	uint32_t lowLongOfPSN;
};

typedef int MCSocketHandle;

extern uint1 *MClowercasingtable;
inline uint1 MCS_tolower(uint1 p_char)
{
	return MClowercasingtable[p_char];
}

extern uint1 *MCuppercasingtable;
inline uint1 MCS_toupper(uint1 p_char)
{
	return MCuppercasingtable[p_char];
}

// MM-2013-09-13: [[ RefactorGraphics ]] Updated for server font support.
struct MCFontStruct
{
	MCSysFontHandle fid;
	uint2 size;
	uint2 style;
    
    coord_t m_ascent;
    coord_t m_descent;
    coord_t m_leading;
    coord_t m_xheight;
};

#define fixmaskrop(a) (a)
#define fixmaskcolor(a) (a)

#define SECONDS_MIN 0.0
#define SECONDS_MAX 32535244799.0

#elif defined(_MOBILE)

#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <assert.h>

typedef int MCSocketHandle;

extern uint1 *MClowercasingtable;
inline uint1 MCS_tolower(uint1 p_char)
{
	return MClowercasingtable[p_char];
}

extern uint1 *MCuppercasingtable;
inline uint1 MCS_toupper(uint1 p_char)
{
	return MCuppercasingtable[p_char];
}

struct MCFontStruct
{
	uint16_t size;
	MCSysFontHandle fid;
    
    coord_t m_ascent;
    coord_t m_descent;
    coord_t m_leading;
    coord_t m_xheight;
};

#define fixmaskrop(a) (a)
#define fixmaskcolor(a) (a)

#define SECONDS_MIN 0.0
#define SECONDS_MAX 32535244799.0

#endif

// SN-2015-04-17: [[ Bug 15187 ]] Needed to know whether we are compiling for
//  iOS Device or iOS Simulator
#if defined TARGET_SUBPLATFORM_IPHONE
#include <TargetConditionals.h>
#endif

//////////////////////////////////////////////////////////////////////
//
//  INTERVAL DEFINITIONS
//

// These intervals are parameters to 'wait' in various places
// throughout the engine.
//
#define BEEP_INTERVAL 0.500
#define WAIT_INTERVAL 0.250
#define CLICK_INTERVAL 0.200
#define DRAG_INTERVAL 0.001
#define MIN_INTERVAL 0.0
#define SHELL_INTERVAL 0.050
#define SHELL_COUNT 20
#define MENU_INTERVAL 0.033
#define REFRESH_INTERVAL 10.0
#define HILITE_INTERVAL 0.200
#define READ_INTERVAL 0.250
#define CHECK_INTERVAL 0.500
#define SOCKET_INTERVAL 0.050

//////////////////////////////////////////////////////////////////////
//
//  MISCELLANEOUS DEFINITIONS
//

#ifndef M_PI
#define M_PI 3.141592653589793238462643
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#define BAD_NUMERIC DBL_MAX
#define MC_EPSILON  (DBL_EPSILON * 10.0)

//////////////////////////////////////////////////////////////////////

#define DoRed                0x1
#define DoGreen              0x2
#define DoBlue               0x4

struct MCColor
{
	uint2 red, green, blue;
};

//////////////////////////////////////////////////////////////////////
//
//  GRAPHICS STRUCTURES AND DEFINITIONS
//

struct MCLineSegment
{
	int2 x1, y1, x2, y2;
};

struct MCPoint
{
	int2 x, y;
};

struct MCRectangle
{
	int2 x, y;
	uint2 width, height;
};

const MCRectangle kMCEmptyRectangle = {0, 0, 0, 0};

struct MCPoint32
{
	int32_t x, y;
};

struct MCRectangle32
{
	int32_t x, y;
	int32_t width, height;
};

////////////////////////////////////////

// MW-2011-09-13: [[ Masks ]] Used to be 'MCAlphaData' but slightly adjusting to
//   make more specific.
struct MCWindowShape
{
	// The width and height of the shape.
	int32_t width;
	int32_t height;

	// A 'sharp' mask is a region and is non-alpha-blended.
	bool is_sharp : 1;
	
	// The alpha-data of the mask (if any).
	uint32_t stride;
	char *data;
	
	// A generic handle used on the different platforms for purposes relating
	// to using the window mask.
	void *handle;
};

////////////////////////////////////////

#if !defined(_LINUX_DESKTOP) && !defined(_LINUX_SERVER)
struct MCBitmap
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
	MCSysBitmapHandle bm;
};
#else
// FG-2014-05-15: [[ GDK ]] We no longer use an XImage for bitmaps
typedef struct _GdkPixbuf MCBitmap;
//struct MCBitmap
//{
//    int width, height;          /* size of image */
//    int xoffset;                /* number of pixels offset in X direction */
//    int format;                 /* XYBitmap, XYPixmap, ZPixmap */
//    char *data;                 /* pointer to image data */
//    int byte_order;             /* data byte order, LSBFirst, MSBFirst */
//    int bitmap_unit;            /* quant. of scanline 8, 16, 32 */
//    int bitmap_bit_order;       /* LSBFirst, MSBFirst */
//    int bitmap_pad;             /* 8, 16, 32 either XY or ZPixmap */
//    int depth;                  /* depth of image */
//    int bytes_per_line;         /* accelarator to next line */
//    int bits_per_pixel;         /* bits per pixel (ZPixmap) */
//    unsigned long red_mask;     /* bits in z arrangment */
//    unsigned long green_mask;
//    unsigned long blue_mask;
//    void *obdata;            /* hook for the object routines to hang on */
//    struct
//	{
//		void *create_image;
//		void *destroy_image;
//		void *get_pixel;
//		void *put_pixel;
//		void *sub_image;
//		void *add_pixel;
//	} f;
//};
#endif

////////////////////////////////////////

#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER)

typedef MCPlatformWindowRef Window;
typedef MCSysWindowHandle Drawable;

#elif defined(_LINUX_DESKTOP) || defined(_LINUX_SERVER)

// MDW-2013-04-15: [[ x64 ]] added 64-bit-safe typedefs
/*#ifndef __LP64__
#   if !defined(Window)
        typedef unsigned long Window;
#   endif
#   if !defined(Pixmap)
        typedef unsigned long Pixmap;
#   endif
#   if !defined(Drawable)
        typedef unsigned long Drawable;
#   endif
#else
#   if !defined(Window)
        typedef unsigned long int Window;
#   endif
#   if !defined(Pixmap)
        typedef unsigned long int Pixmap;
#   endif
#   if !defined(Drawable)
        typedef unsigned long int Drawable;
#   endif
#endif*/

#include <gdk/gdk.h>

typedef GdkWindow*      Window;
typedef GdkPixmap*      Pixmap;
typedef GdkDrawable*    Drawable;

#else

enum
{
    DC_WINDOW,
    DC_BITMAP,
	DC_BITMAP_WITH_DC
};

typedef struct drawable
{
	uint4 type;
	union
	{
		MCSysWindowHandle window;
		MCSysBitmapHandle pixmap;
	} handle;
}
_Drawable;

struct _ExtendedDrawable: public _Drawable
{
	MCSysContextHandle hdc;
};

typedef  _Drawable *        Window;
typedef  _Drawable *        Pixmap;
typedef  _Drawable *        Drawable;


#endif

#define DNULL ((Drawable)0)

////////////////////////////////////////

#define None                 0
#define Button1              1
#define Button2              2
#define Button3              3

typedef unsigned long       KeySym;

#if defined(_LINUX_DESKTOP) || defined(_LINUX_SERVER)
typedef GdkAtom             Atom;
#else
typedef unsigned long       Atom;
#endif

////////////////////////////////////////

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

////////////////////////////////////////

#define LineSolid            0
#define LineOnOffDash        1
#define LineDoubleDash       2

#define CapMask				(CapButt | CapRound | CapProjecting)
#define CapButt				1
#define CapRound			2
#define CapProjecting		3

#define NoStartCap		0x4
#define NoEndCap		0x8

#define JoinRound			0
#define JoinMiter			1
#define JoinBevel			2

#define FillSolid			0
#define FillTiled			1
#define FillStippled		2
#define FillOpaqueStippled	3

#define StaticGray		0
#define GrayScale		1
#define StaticColor		2
#define PseudoColor		3
#define TrueColor		4
#define DirectColor		5

#define LSBFirst		0
#define MSBFirst		1

#define AllPlanes               0xFFFFFF
#define XYPixmap                0
#define ZPixmap                 1

#define CapMask				(CapButt | CapRound | CapProjecting)

#define NoStartCap		0x4
#define NoEndCap		0x8


//////////////////////////////////////////////////////////////////////
//
//  KEYSYM DEFINITIONS
//

#define XK_BackSpace		0xFF08	/* back space, back char */
#define XK_space                0x020
#define XK_Tab			0xFF09
#define XK_ISO_Left_Tab 0xFE20
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

#define XK_WheelUp		0xFF1C
#define XK_WheelDown	0xFF1D
#define XK_WheelLeft	0xFF1E
#define XK_WheelRight	0xFF1F

#define XK_Class_mask		0xFF000000		/* Key classes */
#define XK_Class_compat		0x00000000		/* Ordinary (X11) keycodes */
#define XK_Class_codepoint	0x01000000		/* The low 21 bits contain a Unicode codepoint */
#define XK_Class_vendor		0x10000000		/* OS vendor specific */
#define XK_Codepoint_mask	0x001FFFFF		/* Mask for extracting codepoint from XK_Class_codepoint */

//////////////////////////////////////////////////////////////////////
//
//  FORWARD REFERENCES
//

struct ssl_st;
typedef struct ssl_st SSL;
struct ssl_ctx_st;
typedef struct ssl_ctx_st SSL_CTX;

class MCContext;
typedef class MCContext MCDC;
struct MCPattern;
typedef MCPattern *MCPatternRef;

struct MCPickleContext;

class MCUIDC;
class MCTransferlist;
class MCUndolist;
class MCSellist;
class MCIdlelist;
class MCStacklist;
class MCCardlist;
class MCDispatch;
class MCStack;
class MCTooltip;
class MCAudioClip;
class MCVideoClip;
class MCGroup;
class MCCard;
class MCButton;
class MCGraphic;
class MCEPS;
class MCScrollbar;
class MCPlayer;
class MCImage;
class MCField;
class MCObject;
class MCObjectList;
class MCMagnify;
class MCPrinter;
class MCPrinterDevice;
class MCPrinterSetupDialog;
class MCPageSetupDialog;
class MCSocket;
class MCTheme;
class MCWidget;
class MCScriptEnvironment;
class MCDragData;
class MCClipboardData;
class MCSelectionData;
class MCPasteboard;
class MCFontlist;
struct MCWidgetInfo;
class MCExecPoint;
class MCParameter;
class MCStack;
class MCExecContext;

typedef uint32_t MCDragAction;
typedef uint32_t MCDragActionSet;

typedef struct _Streamnode Streamnode;
typedef struct _Linkatts Linkatts;

struct MCThemeDrawInfo;

struct MCDateTime;
struct MCDateTimeLocale;

class MPlayer;

class MCObjectInputStream;
class MCObjectOutputStream;
class MCStatement;
class MCScriptPoint;
class MCVarref;

class MCBlock;
struct Ustruct;
class MCControl;
class MCParagraph;
class MCHandlerlist;
class MCHandler;
class MCParentScript;
class MCParentScriptUse;
class MCVariable;
class MCExpression;
class MCContainer;
struct MCPickleContext;

class MCExternal;
class MCExternalHandlerList;

class MCObjptr;

typedef struct __MCRegion *MCRegionRef;
typedef struct MCTileCache *MCTileCacheRef;
class MCStackSurface;

class MCObjectPropertySet;
class MCGo;
class MCVisualEffect;
class MCCRef;
class MCProperty;
class MCChunk;
struct MCObjectRef;

typedef struct MCBitmapEffects *MCBitmapEffectsRef;
class MCCdata;
class MCLine;
struct MCTextBlock;
struct MCTextParagraph;

class MCEffectList;
class MCError;

class MCStyledText;

typedef struct MCFont *MCFontRef;

typedef struct MCSyntaxFactory *MCSyntaxFactoryRef;

//////////////////////////////////////////////////////////////////////

// Chunks, containers and ordinals (and dest for Go command)
enum Chunk_term {
    CT_UNDEFINED,
    CT_START,
    CT_BACKWARD,
    CT_FORWARD,
    CT_FINISH,
    CT_HOME,
	// MW-2009-03-03: The chunk type of the invisible 'script' object that
	//   holds the SERVER mode state.
	CT_SERVER_SCRIPT,
    CT_HELP,
    CT_DIRECT,
    CT_RECENT,
    CT_THIS,
    CT_FIRST,
    CT_SECOND,
    CT_THIRD,
    CT_FOURTH,
    CT_FIFTH,
    CT_SIXTH,
    CT_SEVENTH,
    CT_EIGHTH,
    CT_NINTH,
    CT_TENTH,
    CT_LAST,
    CT_NEXT,
    CT_PREV,
    CT_MIDDLE,
    CT_ANY,
    CT_ORDINAL,
    CT_ID,
    CT_EXPRESSION,
    CT_RANGE,
    CT_URL,
    CT_URL_HEADER,
    CT_ALIAS,
	CT_DOCUMENT,
    CT_TOP_LEVEL,
    CT_MODELESS,
    CT_PALETTE,
    CT_MODAL,
    CT_PULLDOWN,
    CT_POPUP,
    CT_OPTION,

	// The name table used for MCU_matchname *must* be updated if any
	// chunk terms are added between CT_STACK and CT_LAST_CONTROL here
    CT_STACK,
    CT_TOOLTIP,
    CT_AUDIO_CLIP,
    CT_VIDEO_CLIP,
    CT_BACKGROUND,
    CT_CARD,
    CT_MARKED,
    CT_GROUP,
	CT_FIRST_CONTROL = CT_GROUP,
	CT_LAYER,
    CT_BUTTON,
    CT_MENU,
    CT_SCROLLBAR,
    CT_PLAYER,
    CT_IMAGE,
    CT_GRAPHIC,
    CT_EPS,
    CT_MAGNIFY,
    CT_COLOR_PALETTE,
    CT_WIDGET,
    CT_FIELD,
	CT_LAST_CONTROL = CT_FIELD,
	
    CT_FIRST_TEXT_CHUNK = CT_FIELD,
    CT_LINE,
    CT_PARAGRAPH,
    CT_SENTENCE,
    CT_ITEM,
    CT_WORD,
    CT_TRUEWORD,
    CT_TOKEN,
    CT_CHARACTER,
    // AL-2013-01-08 [[ CharChunks ]] Add 'codepoint, codeunit and byte' to chunk types
    CT_CODEPOINT,
    CT_CODEUNIT,
    CT_BYTE,
    // SN-2014-04-15 [[ ByteChunk ]] CT_ELEMENT should be put after the char chunks, as the value won't be evaluated as a string
	CT_ELEMENT,
    CT_TYPES,
	CT_KEY
};

struct MCObjectPtr
{
    /* TODO[C++11] MCObject *object = nullptr; */
    /* TODO[C++11] uint32_t part_id = 0; */
	MCObject *object;
	uint32_t part_id;

    /* TODO[C++11] constexpr MCObjectPtr() = default; */
    MCObjectPtr() : object(nullptr), part_id(0) {}
    /* TODO[C++11] constexpr */
    MCObjectPtr(MCObject *p_object, uint32_t p_part_id)
        : object(p_object), part_id(p_part_id)
    {}

    MCObjectPtr& operator = (const MCObjectPtr& p_obj_ptr)
    {
        object = p_obj_ptr . object;
        part_id = p_obj_ptr . part_id;
        return *this;
    }
};

// NOTE: the indices in this structure are UTF-16 code unit indices if the value is a stringref,
//  and byte indices if it is a dataref.
struct MCMarkedText
{
    MCValueRef text;
    uint32_t start, finish;
    // SN-2014-09-03: [[ Bug 13314 ]] MCMarkedText::changed updated to store the number of chars appended
    uindex_t changed;
};

struct MCObjectChunkPtr
{
	MCObject *object;
	uint32_t part_id;
	Chunk_term chunk;
    MCMarkedText mark;
};

struct MCVariableChunkPtr
{
	MCVarref *variable;
	Chunk_term chunk;
    MCMarkedText mark;
};

struct MCUrlChunkPtr
{
	MCStringRef url;
	Chunk_term chunk;
    MCMarkedText mark;
};

struct MCObjectIndexPtr
{
    MCObject *object;
    uint32_t part_id;
    MCNameRef index;
};

struct MCObjectChunkIndexPtr
{
	MCObject *object;
	uint32_t part_id;
	Chunk_term chunk;
	MCMarkedText mark;
    MCNameRef index;
};

//////////////////////////////////////////////////////////////////////

#endif
