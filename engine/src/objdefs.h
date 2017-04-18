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

//
// Structures used in all display objects
//
#ifndef	OBJDEFS_H
#define	OBJDEFS_H

#define UNLICENSED_SCRIPT_LIMIT 10
#define UNLICENSED_DO_LIMIT 10
#define UNLICENSED_USING_LIMIT 50
#define UNLICENSED_INSERT_LIMIT 10

#if defined(__EMSCRIPTEN__)
#	define DEFAULT_TEXT_FONT "Droid Sans Fallback"
#else
// SN-2015-05-14: [[ Bug 14116 ]] Make the default font a correct PostScript font
#	define DEFAULT_TEXT_FONT "Helvetica"
#endif
#define DEFAULT_TEXT_SIZE 14
#define DEFAULT_TAB_SPACING 8
#define DEFAULT_BORDER 2
#define DEFAULT_SHADOW 4
#define DEFAULT_RADIUS 15
#define STANDARD_RADIUS 6
#define DEFAULT_ARROW_SIZE 3
#define DEFAULT_MENU_LINES 5
#define DEFAULT_SB_WIDTH 20
#define MAC_SB_WIDTH 16
#define MAX_BORDER 255
#define TEXT_Y_OFFSET 6
#define MAX_PLANES 8

#define INTERPOLATION_NEAREST 3
#define INTERPOLATION_BILINEAR 1
#define INTERPOLATION_BICUBIC 2
#define INTERPOLATION_BOX 0

#define STACK_SIZE "10000"
#define FREE_SIZE  "0"
#define DISK_SPACE  "1000000"

#define START_ID 1001

#define OPTION_TIME 250

#define MS_SHIFT               0x01
#define MS_CONTROL             0x02
#define MS_MOD1                0x04
#define MS_ALT                 MS_MOD1
#define MS_MOD2                0x08
#define MS_MOD3                0x10
#define MS_MOD4                0x20
#define MS_MOD5                0x40
#define MS_MODIFIER           (MS_MOD1 | MS_MOD2 | MS_MOD3 | MS_MOD4 | MS_MOD5)
#define MS_CAPS_LOCK           0x80
#ifdef _MACOSX
#define MS_MAC_CONTROL         MS_MOD2
#else
#define MS_MAC_CONTROL         MS_CONTROL
#endif

#define F_CLEAR                0
#define F_STYLE                0x7
// MCScrollbar styles
#define F_VERTICAL             0
#define F_HORIZONTAL           1
// MCButton styles
#define F_STANDARD             0
#define F_ROUNDRECT            1
#define F_CHECK                3
#define F_RADIO                4
#define F_MENU                 5
#define F_OVAL_BUTTON          6
#define F_RECTANGLE            7
// MCGraphic styles
#define F_G_RECTANGLE          0
#define F_ROUNDRECT            1
#define F_POLYGON              2
#define F_CURVE                3
#define F_OVAL                 4
#define F_REGULAR              5
#define F_LINE                 6

// General property attributes
#define F_FONT                  (1UL << 3)
#define F_SCRIPT                (1UL << 4)
// General display attributes
#define F_DISPLAY_STYLE         (0x7UL << 5)
#define F_SHOW_BORDER           (1UL << 5)
#define F_OPAQUE                (1UL << 6)
#define F_SHADOW                (1UL << 7)
#define F_3D                    (1UL << 8)
#define F_ALIGNMENT             (3UL << 9)
#define F_ALIGNMENT_SHIFT		9
#define F_ALIGN_LEFT             0
#define F_ALIGN_CENTER          (1UL << 9)
#define F_ALIGN_RIGHT           (1UL << 10)
#define F_ALIGN_JUSTIFY         (3UL << 9)
#define F_VISIBLE               (1UL << 11)
#define F_DISABLED              (1UL << 12)
#define F_LOCK_LOCATION         (1UL << 27)
// General behavior attributes
#define F_TRAVERSAL_ON          (1UL << 13)
// MCDispatch attributes
#define F_WAS_LICENSED          (1UL << 14)
/* #define F_HOME                  (1UL << 15) -- no longer used as of 4.6.3-dp-1 */
#define F_PENDING               (1UL << 16)
// MCButton attributes
#define F_AUTO_ARM              (1UL << 14)
#define F_AUTO_HILITE           (1UL << 15)
#define F_SHARED_HILITE         (1UL << 16)
#define F_SHOW_ICON             (1UL << 17)
#define F_SHOW_NAME             (1UL << 18)
#define F_DEFAULT               (1UL << 19)
#define F_SHOW_HILITE           (1UL << 20)
#define F_LABEL                 (1UL << 21)
#define F_NO_MARGINS            (1UL << 22)
#define F_MENU_STRING           (1UL << 23)
#define F_LABEL_WIDTH           (1UL << 24)
#define F_HAS_ICONS             (1UL << 25) // was F_HILITE_ICON
#define F_MENU_LINES            (1UL << 26)
// MCButton behaviors
#define F_ARM_MODE              (3UL << 28)
#define F_ARM_NONE               0
#define F_ARM_BORDER            (2UL << 28)
#define F_ARM_FILL              (1UL << 28)
#define F_HILITE_MODE           (3UL << 30)
#define F_HILITE_BOTH           (3UL << 30)
#define F_HILITE_NONE            0
#define F_HILITE_BORDER         (2UL << 30)
#define F_HILITE_FILL           (1UL << 30)
// MCField attributes
#define F_VGRID                 (1UL << 0)
#define F_HGRID                 (1UL << 1)
#define F_AUTO_TAB              (1UL << 14)
#define F_F_DONT_SEARCH         (1UL << 15)
#define F_DONT_WRAP             (1UL << 16)
#define F_FIXED_HEIGHT          (1UL << 17)
#define F_LOCK_TEXT             (1UL << 18)
#define F_SHOW_LINES            (1UL << 19)
#define F_SHARED_TEXT           (1UL << 20)
#define F_LIST_BEHAVIOR         (1UL << 21)
#define F_SCROLLBAR             (3UL << 22)
#define F_HSCROLLBAR            (1UL << 22)
#define F_VSCROLLBAR            (1UL << 23)
#define F_MULTIPLE_HILITES      (1UL << 24)
#define F_NONCONTIGUOUS_HILITES (1UL << 25)
#define F_TABS                  (1UL << 26)
#define F_NO_AUTO_HILITE        (1UL << 28)
#define F_TOGGLE_HILITE         (1UL << 29)
#define F_3D_HILITE             (1UL << 30)
#define F_F_AUTO_ARM            (1UL << 31)
// MCImage attributes
#define F_DONT_DITHER           (1UL << 14)
#define F_TRUE_COLOR            (1UL << 15)
#define F_HAS_FILENAME          (1UL << 16)
#define F_COMPRESSION           (3UL << 17 | 1UL << 22)
#define F_RLE                   (0UL << 17)
#define F_GIF                   (1UL << 17)
#define F_JPEG                  (2UL << 17)
#define F_PNG                   (3UL << 17)
#define F_NEED_FIXING           (1UL << 19)
#define F_REPEAT_COUNT          (1UL << 20)
#define F_PALINDROME_FRAMES     (1UL << 21)
#define F_PICT                  (1UL << 22)
#define F_SAVE_SIZE             (1UL << 23)
#define F_I_ALWAYS_BUFFER       (1UL << 24)
#define F_CONSTANT_MASK         (1UL << 25)
#define F_ANGLE                 (1UL << 26)
// MCGraphic attributes
#define F_CAPROUND              (1UL << 14)
#define F_JOINSTYLE             (3UL << 15)
#define F_JOINROUND             (1UL << 15)
#define F_JOINMITER             (2UL << 15)
#define F_JOINBEVEL             (3UL << 15)
#define F_MARKER_DRAWN          (1UL << 17)
#define F_MARKER_OPAQUE         (1UL << 18)

#define F_DONT_RESIZE           (1UL << 19)
#define F_DASHES                (1UL << 20)
#define F_START_ARROW           (1UL << 21)
#define F_END_ARROW             (1UL << 22)
#define F_G_SHOW_NAME           (1UL << 23)
//#define F_G_BOGUS               (1UL << 24) //was label pre 2.1.1
#define F_G_ANTI_ALIASED				(1UL << 24)
#define F_G_LABEL               (1UL << 25)
#define F_CAPSQUARE				(1UL << 26)

#define F_FILLRULE				(3UL << 28)
#define F_FILLRULENONE			(1UL << 28)
#define F_FILLRULENONZERO		(2UL << 28)
#define F_FILLRULEEVENODD		(3UL << 28)

// MCPlayer attributes
#define F_FRAME_RATE            (1UL << 14)
//#define F_ALWAYS_BUFFER         (1UL << 15)
#define F_DONT_REFRESH          (1UL << 16)
#define F_SHOW_CONTROLLER       (1UL << 17)
#define F_SHOW_BADGE            (1UL << 18)
#define F_LOOPING               (1UL << 19)
#define F_PLAY_SELECTION        (1UL << 20)
#define F_SHOW_SELECTION        (1UL << 21)
#define F_SHOW_VOLUME           (1UL << 22)
#define F_MIRRORED              (1UL << 23)
// MCVideoClip attributes, scale is for backward compatibility
#define F_SCALE_FACTOR          (1UL << 15)
// MCEPS attributes
#define F_RETAIN_IMAGE          (1UL << 14)
#define F_RETAIN_POSTSCRIPT     (1UL << 15)
#define F_SCALE_INDEPENDENTLY (  1UL << 16)
// MCAudioClip attributes
#define F_LOUDNESS              (1UL << 14)
#define F_EXTERNAL              (1UL << 15)
#define F_UNSIGNED              (1UL << 16)
// MCScrollbar attributes
#define F_SAVE_ATTS             (1UL << 14)
#define F_SCALE                 (1UL << 15)
#define F_HAS_VALUES            (1UL << 16)
#define F_SHOW_VALUE            (1UL << 17)
#define F_PROGRESS              (1UL << 18)
#define F_SB_STYLE              (F_SCALE | F_PROGRESS)
// MCCard attributes
#define F_C_CANT_DELETE         (1UL << 14)
#define F_MARKED                (1UL << 15)
#define F_C_DONT_SEARCH         (1UL << 16)
// MCGroup attributes
#define F_RADIO_BEHAVIOR        (1UL << 14)
#define F_TAB_GROUP_BEHAVIOR    (1UL << 15)
#define F_G_CANT_DELETE         (1UL << 16)
#define F_G_DONT_SEARCH         (1UL << 17)
#define F_SHOW_NAME             (1UL << 18)
#define F_MARGINS               (1UL << 19)
//#define F_LABEL                 (1UL << 21)
//#define F_HSCROLLBAR            (1UL << 22)
//#define F_VSCROLLBAR            (1UL << 23)
#define F_GROUP_ONLY            (1UL << 24)
#define F_BOUNDING_RECT         (1UL << 25)

// MW-2008-10-28: [[ SelectGroupedControls ]] This flag indicates whether
//   it should be group or its children that are selectable. If true,
//   the children will not be selectable.
#define F_SELECT_GROUP			(1UL << 26)

//#defined F_LOCK_LOCATION		(1UL << 27)

// MW-2010-12-19: [[ Unbounded Scrolling ]] These flags indicate whether
//   the group should clamp its scroll values.
#define F_UNBOUNDED_HSCROLL		(1UL << 28)
#define F_UNBOUNDED_VSCROLL		(1UL << 29)

// MW-2011-08-09: [[ Groups ]] This flag indicates that the group is placed on
//   multiple cards, or is allowed to be placed on multiple cards.
#define F_GROUP_SHARED			(1UL << 30)

// MCStack attributes
#define F_LINK_ATTS             (1UL << 5)
#define F_S_CANT_DELETE         (1UL << 14)
#define F_ALWAYS_BUFFER         (1UL << 15)
#define F_CANT_ABORT            (1UL << 16)
#define F_MENU_BAR              (1UL << 17)
#define F_CANT_MODIFY           (1UL << 18)
#define F_DYNAMIC_PATHS         (1UL << 19)
#define F_WM_PLACE              (1UL << 20)
#define F_DECORATIONS           (1UL << 21)
#define F_HC_ADDRESSING         (1UL << 22)
#define F_HC_STACK              (1UL << 23)
#define F_RESIZABLE             (1UL << 24)
#define F_TITLE                 (1UL << 25) // same as label
#define F_START_UP_ICONIC       (1UL << 26)
#define F_FORMAT_FOR_PRINTING   (1UL << 27)
#define F_DESTROY_WINDOW        (1UL << 28)
#define F_DESTROY_STACK         (1UL << 29)

// MW-2013-03-28: This flag is no longer used, but we retain it so we can ensure
//   it isn't set on any stacks that are saved.
#define F_CANT_STANDALONE		(1UL << 30)

#define F_STACK_FILES           (1UL << 31)
#define F_TAKE_FLAGS            (F_STYLE | F_RESIZABLE | F_DECORATIONS \
				 | F_CANT_MODIFY | F_ALWAYS_BUFFER)
// MCBlock attributes
#define F_HAS_COLOR             (1UL << 14)
// MW-2012-01-06: [[ Block Metadata ]] Flag that indicates the metadata field is used.
#define F_HAS_METADATA          (1UL << 15)
#define F_HAS_SHIFT             (1UL << 16)
#define F_HAS_BACK_COLOR        (1UL << 17)
#define F_HAS_COLOR_NAME        (1UL << 18)
// SN-2014-12-04: [[ Bug 14149 ]] Flag required for saving in legacy format.
#define F_HAS_TAB               (1UL << 19) // [[ TabAlignments ]] No longer required
#define F_HAS_BACK_COLOR_NAME   (1UL << 20)
#define F_HAS_LINK              (1UL << 21)
#define F_HAS_IMAGE             (1UL << 22)
// MW-2012-01-26: [[ FlaggedField ]] This flag (not persistent) indicates the block is
//   'flagged'.
#define F_FLAGGED				(1UL << 23)
// MW-2012-02-27: [[ Bug ]] F_HAS_METADATA should be included here otherwise it causes
//   premature delete of Atts struct!
#define F_HAS_ATTS              (F_HAS_FNAME | F_HAS_FSIZE | F_HAS_FSTYLE | F_HAS_COLOR | F_HAS_SHIFT | F_HAS_BACK_COLOR | F_HAS_LINK | F_HAS_IMAGE | F_HAS_METADATA)

// MW-2012-02-17: [[ SplitTextAttrs ]] When in memory, these flags indicate whether the
//   given font styles are present. When on-disk they indicate whether they are inherited.
#define F_FATTR_MASK			(F_HAS_FNAME | F_HAS_FSIZE | F_HAS_FSTYLE)
#define F_HAS_ALL_FATTR			F_FATTR_MASK
#define F_HAS_FNAME		        (1UL << 24)
#define F_HAS_FSIZE		        (1UL << 25)
#define F_HAS_FSTYLE            (1UL << 26)
#define F_INHERIT_FNAME	        (1UL << 24)
#define F_INHERIT_FSIZE	        (1UL << 25)
#define F_INHERIT_FSTYLE        (1UL << 26)

#define F_HILITED               (1UL << 27)
#define F_VISITED               (1UL << 28)
#define F_HAS_UNICODE           (1UL << 29)

// Block flags
#define BF_NAME                0x1
#define BF_SIZE                0x2
#define BF_STYLE               0x4

// MW-2012-02-27: [[ Bug 2939 ]] These flags indicate whether a block should include the
//   left/right edge of any box.
#define DBF_DRAW_LEFT_EDGE		0x1
#define DBF_DRAW_RIGHT_EDGE		0x2

// Additional properties, piggybacked on pixmaps (12 bits)
#define AF_CUSTOM_PROPS         (1UL << 15)
#define AF_BORDER_WIDTH         (1UL << 14)
#define AF_SHADOW_OFFSET        (1UL << 13)
#define AF_TOOL_TIP             (1UL << 12)
#define AF_ALT_ID               (1UL << 11)
#define AF_INK                  (1UL << 10)
#define AF_CANT_SELECT          (1UL << 9)
#define AF_LONG_SCRIPT          (1UL << 8)
#define AF_LINK_COLORS          (1UL << 7)
#define AF_NO_FOCUS_BORDER      (1UL << 6)
#define AF_BLEND_LEVEL					(1UL << 5) // New in Version 2.7
#define AF_EXTENDED							(1UL << 4)
// Corresponding additional properties in extraflags
#define EF_CANT_SELECT          (1UL << 0)
#define EF_LINK_COLORS          (1UL << 1)
#define EF_NO_FOCUS_BORDER      (1UL << 2)

// MW-2012-02-16: [[ LogFonts ]] The logical font flags - recorded in MCObject::m_font_flags.

// If 'unicode-tag' is set then it means the object's textFont had a ,unicode
// tag when it was loaded. (This is used in individual objects to set the unicode
// flag - if applicable and is not persistent).
#define FF_HAS_UNICODE_TAG		(1UL << 7)

// If 'unicode' is set then it means that the object's text contents are
// encoded in unicode. (This is not persistent)
#define FF_HAS_UNICODE			(1UL << 6)

// These flags determine which of the font attrs are actually set. These
// will be saved into the stackfile object extended area.
#define FF_HAS_TEXTFONT			(1UL << 0)
#define FF_HAS_TEXTSTYLE		(1UL << 1)
#define FF_HAS_TEXTSIZE			(1UL << 2)
#define FF_HAS_ALL_FATTR		(FF_HAS_TEXTFONT | FF_HAS_TEXTSTYLE | FF_HAS_TEXTSIZE)

// MCControl state
#define CS_CLEAR                0
#define CS_NO_MESSAGES          (1UL << 0)
#define CS_NO_FILE              (1UL << 1)
#define CS_CREATE               (1UL << 2)
#define CS_SELECTED             (1UL << 3)
#define CS_KFOCUSED             (1UL << 4)
#define CS_MFOCUSED             (1UL << 5)
#define CS_HILITED              (1UL << 6)
#define CS_MOVE                 (1UL << 7)
#define CS_SIZEL                (1UL << 8)
#define CS_SIZER                (1UL << 9)
#define CS_SIZET                (1UL << 10)
#define CS_SIZEB                (1UL << 11)
#define CS_SIZE                 (CS_SIZEL | CS_SIZER | CS_SIZET | CS_SIZEB)

// MW-2011-01-10: [[ Bug 9777 ]] The image object uses this flag to prevent
//   the layer attributes being ditched when it 'reopens'.
#define CS_KEEP_LAYER           (1UL << 29) // was CS_DONT_DRAW

#define CS_GRAB                 (1UL << 30)
#define CS_MENU_ATTACHED        (1UL << 31)
// MCButton state
#define CS_ARMED                (1UL << 13)
#define CS_SUBMENU              (1UL << 14)
//#define CS_SUBMENU_CLOSED       (1UL << 15)
#define CS_SHOW_DEFAULT         (1UL << 16)
#define CS_FIELD_GRAB           (1UL << 17)
#define CS_SCROLLBAR            (1UL << 18)
#define CS_IGNORE_MENU          (1UL << 19)
#define CS_MIXED                (1UL << 20)
#define CS_MOUSE_UP_MENU        (1UL << 21)
#define CS_VISITED              (1UL << 22)

// MCImage state
#define CS_BEEN_MOVED           (1UL << 13)

#define CS_MAGNIFY              (1UL << 14)
#define CS_DRAG                 (1UL << 15)
#define CS_MAG_DRAG             (1UL << 16)
#define CS_DRAW                 (1UL << 17)
#define CS_OWN_SELECTION        (1UL << 18)
#define CS_REVERSED             (1UL << 19)
#define CS_MASK_PM              (1UL << 20)
#define CS_IMAGE_PM             (1UL << 21)
#define CS_EDITED               (1UL << 22)
//#define CS_IS_TRUE              (1UL << 23) /* OBSOLETE */
#define CS_REVERSE              (1UL << 24)
#define CS_DO_START             (1UL << 25)
//#define CS_LOADING				(1UL << 26) /* OBSOLETE */
// MCScrollbar state
#define CS_SCROLL               (1UL << 13)
// MCField state
#define CS_SELECTING            (1UL << 13)
#define CS_HSCROLL              (1UL << 14)
#define CS_VSCROLL              (1UL << 15)
#define CS_IBEAM                (1UL << 16)
#define CS_CHANGED              (1UL << 17)
#define CS_DELETING             (1UL << 18)
#define CS_PARTIAL              (1UL << 19)
#define CS_DRAG_TEXT            (1UL << 20)
#define CS_SOURCE_TEXT          (1UL << 21)
#define CS_FOREIGN_VSCROLLBAR		(1UL << 22)
#define CS_FOREIGN_HSCROLLBAR		(1UL << 23)
#define CS_FOREIGN_SCROLLBAR		(3UL << 22)
#define CS_MENUFIELD			(1UL << 24)
#define CS_MOUSEDOWN			(1UL << 25)
#define CS_IN_TEXTCHANGED		(1UL << 26)
// MCGraphic state
#define CS_CREATE_POINTS        (1UL << 13)
// MCPlayer state
#define CS_PREPARED             (1UL << 13)
#define CS_PLAYING              (1UL << 14)
#define CS_PAUSED               (1UL << 15)
#define CS_CLOSING              (1UL << 16)
#define CS_EXTERNAL_CONTROLLER	(1UL << 17)
#define CS_CTC_PENDING          (1UL << 18)
// MCGroup state
#define CS_NEED_UPDATE          (1UL << 13)
// Uses CS_HSCROLL 14
// Uses CS_VSCROLL 15
#define CS_SENDING_RESIZE		(1UL << 16)
// MCCard state
#define CS_INSIDE               (1UL << 15)
// MCStack state
#define CS_BEEN_MOVED           (1UL << 13)
#define CS_LOCK_SCREEN			(1UL << 14)
#define CS_NO_FOCUS             (1UL << 15)
#define CS_SUSPENDED            (1UL << 16)
#define CS_MARKED               (1UL << 17)
#define CS_ICONIC               (1UL << 18)
#define CS_NEED_REDRAW          (1UL << 19)
//#define CS_KEYED                (1UL << 20) /* OBSOLETE */
#define CS_FOREIGN_WINDOW       (1UL << 21)
#define CS_IGNORE_CLOSE         (1UL << 22)
#define CS_NEED_RESIZE          (1UL << 23)
#define CS_DELETE_STACK         (1UL << 24)
#define CS_NO_CONFIG            (1UL << 25)
#define CS_TITLE_CHANGED        (1UL << 26)
#define CS_EDIT_MENUS           (1UL << 27)
#define CS_TRANSLATED           (1UL << 28)
#define CS_EFFECT               (1UL << 29)
#define CS_ISOPENING            (1UL << 30)

#define ECS_MASK_CHANGED		(1UL << 31)
#define ECS_UNSAVEABLE			(1UL << 29)
#define ECS_MEDIA_ORIGIN		(1UL << 28)
#define ECS_EXTERNALS_RESOLVED	(1UL << 27)
#define ECS_DURING_STARTUP		(1UL << 26)
#define ECS_FULLSCREEN			(1UL << 24)

// MW-2008-10-28: [[ ParentScripts ]] If this extended state flag is set it
//   means that there is likely to be an object being used as a parentScript
//   on this stack.
#define ECS_HAS_PARENTSCRIPTS	(1UL << 23)
#define ECS_ISEXTRAOPENED		(1UL << 22)

// MW-2011-01-05: Add a state flag for whether the window should display
//   the modified mark.
#define ECS_MODIFIED_MARK		(1UL << 21)

// MW-2011-03-11: Stack is currently disabled due to modality
#define ECS_DISABLED_FOR_MODAL	(1UL << 20)

// If this is set then no updates should be rendered.
#define ECS_DONTDRAW			(1UL << 19)

// If this is set then this stack needs parentScripts resolve
#define ECS_USES_PARENTSCRIPTS	(1UL << 18)

// MERG-2014-06-02: [[ IgnoreMouseEvents ]] If this is set then the stack is transparent to mouse events
#define ECS_IGNORE_MOUSE_EVENTS (1UL << 17)

// Has handlers
#define HH_IDLE                 (1UL << 0)
#define HH_MOUSE_WITHIN         (1UL << 1)
#define HH_MOUSE_STILL_DOWN     (1UL << 2)
#define HH_DEAD_SCRIPT          (1UL << 3)
#define HH_OPEN_CONTROL			(1UL << 4)
#define HH_PREOPEN_CONTROL		(1UL << 5)
#define HH_RESIZE_CONTROL		(1UL << 6)
#define HH_CLOSE_CONTROL		(1UL << 7)

// MCParagraph state
#define PS_FRONT                (1UL << 0)
#define PS_BACK                 (1UL << 1)
#define PS_HILITED              (PS_FRONT | PS_BACK)
#define PS_LINES_NOT_SYNCHED		(1UL << 2)

// MCStack decorations
#define DECORATION_LENGTH     64

#define DECORATION_MINIMIZE_WIDTH 96



#define WD_CLEAR              0
#define WD_TITLE              (1UL << 0)
#define WD_MENU               (1UL << 1)
#define WD_MINIMIZE           (1UL << 2)
#define WD_MAXIMIZE           (1UL << 3)
#define WD_CLOSE              (1UL << 4)
#define WD_SHAPE              (1UL << 5)
//SHOULD BE ACCESSIBLE USING A NEW WINDOWATTRIBUTE PROPERTY which is
//not mutually exclusive.  In the future we should also make this a
//int.
#define WD_METAL              (1UL << 6)
#define WD_NOSHADOW           (1UL << 7)
#define WD_UTILITY            (1UL << 8)
#define WD_LIVERESIZING        (1UL << 9)
#define WD_FORCETASKBAR		  (1UL << 10)
#define WD_WDEF               (1UL << 15) // use Mac WDEF resource

// Font styles
#define FA_DEFAULT_STYLE       0x55
#define FA_WEIGHT              0x0f
#define FA_EXPAND              0xf0
#define FA_CONDENSED           0x30
#define FA_EXPANDED            0x70

#define FA_BOLD                0x57
#define FA_ITALIC             (1UL << 8)
#define FA_OBLIQUE            (1UL << 9)
#define FA_BOX                (1UL << 10)
#define FA_3D_BOX             (1UL << 11)
#define FA_UNDERLINE          (1UL << 12)
#define FA_STRIKEOUT          (1UL << 13)
#define FA_LINK               (1UL << 14)
#define FA_FONT               (1UL << 15)  // to mark a font change in paragraph HTML

// Font is special and should be ignored for font lookups, etc
#define FA_SYSTEM_FONT        (1UL << 31)

#define STYLE_LENGTH           256

typedef struct
{
	uint4 keysym;
	uint1 functions[4];
}
Keytranslations;

#define MODIFIER_LENGTH        64

typedef struct
{
	uint4 keysym;
	const char *name;
}
Keynames;

typedef struct _Linkatts
{
	MCColor color;
	MCStringRef colorname;
	MCColor hilitecolor;
	MCStringRef hilitecolorname;
	MCColor visitedcolor;
	MCStringRef visitedcolorname;
	Boolean underline;
}
Linkatts;

enum Draw_index {
    DI_FORE,
    DI_BACK,
    DI_HILITE,
    DI_BORDER,
    DI_TOP,
    DI_BOTTOM,
    DI_SHADOW,
    DI_FOCUS,
    
    // Pseudo-DIs used for theming
    DI_PSEUDO_TEXT_COLOR,           // Text colour for non-selected text
    DI_PSEUDO_TEXT_BACKGROUND,      // Text background colour
    DI_PSEUDO_TEXT_COLOR_SEL_FORE,  // Text colour for selected text, use DI_FORE
    DI_PSEUDO_TEXT_COLOR_SEL_BACK,  // Text colour for selected text, use DI_BACK
    DI_PSEUDO_TEXT_BACKGROUND_SEL,  // Text selection colour
    DI_PSEUDO_BUTTON_TEXT,          // Text colour for button text
    DI_PSEUDO_BUTTON_TEXT_SEL      // Text colour for selected menu items
};


#define DF_FORE_COLOR         (1UL << 0)
#define DF_BACK_COLOR         (1UL << 1)
#define DF_HILITE_COLOR       (1UL << 2)
#define DF_BORDER_COLOR       (1UL << 3)
#define DF_TOP_COLOR          (1UL << 4)
#define DF_BOTTOM_COLOR       (1UL << 5)
#define DF_SHADOW_COLOR       (1UL << 6)
#define DF_FOCUS_COLOR        (1UL << 7)
#define DF_FORE_PATTERN       (1UL << 8)
#define DF_BACK_PATTERN       (1UL << 9)
#define DF_HILITE_PATTERN     (1UL << 10)
#define DF_BORDER_PATTERN     (1UL << 11)
#define DF_TOP_PATTERN        (1UL << 12)
#define DF_BOTTOM_PATTERN     (1UL << 13)
#define DF_SHADOW_PATTERN     (1UL << 14)
#define DF_FOCUS_PATTERN      (1UL << 15)

enum Scrollbar_mode {
    SM_CLEARED,
    SM_BEGINNING,
    SM_END,
    SM_LINEDEC,
    SM_LINEINC,
    SM_PAGEDEC,
    SM_PAGEINC
};

enum Object_pos {
    OP_NONE,
    OP_LEFT,
    OP_ALIGN_LEFT,
    OP_CENTER,
    OP_ALIGN_RIGHT,
    OP_RIGHT,
    OP_TOP,
    OP_ALIGN_TOP,
    OP_MIDDLE,
    OP_ALIGN_BOTTOM,
    OP_BOTTOM,
    OP_OFFSET
};

enum Arrow_direction {
    AD_UP,
    AD_DOWN,
    AD_LEFT,
    AD_RIGHT
};

enum Etch {
    ETCH_RAISED,
    ETCH_RAISED_SMALL,
    ETCH_SUNKEN,
    ETCH_SUNKEN_BUTTON,
    ETCH_HILITE,
    ETCH_GROOVE,
    ETCH_RIDGE
};

enum Field_translations {
    FT_UNDEFINED,
    FT_DELBCHAR,
	FT_DELBSUBCHAR,
    FT_DELBWORD,
    FT_DELFCHAR,
    FT_DELFWORD,
	FT_DELBOL,
	FT_DELEOL,
	FT_DELBOP,
	FT_DELEOP,
    FT_HELP,
    FT_UNDO,
    FT_CUT,
    FT_CUTLINE,
    FT_COPY,
    FT_PASTE,
    FT_TAB,
    FT_FOCUSFIRST,
    FT_FOCUSLAST,
    FT_FOCUSNEXT,
    FT_FOCUSPREV,
    FT_PARAGRAPH,
	FT_PARAGRAPHAFTER,
    FT_LEFTCHAR,
	FT_BACKCHAR,
    FT_LEFTWORD,
	FT_BACKWORD,
	FT_LEFTPARA,
	FT_BACKPARA = FT_LEFTPARA,
    FT_RIGHTCHAR,
	FT_FORWARDCHAR,
    FT_RIGHTWORD,
	FT_FORWARDWORD,
	FT_RIGHTPARA,
	FT_FORWARDPARA = FT_RIGHTPARA,
    FT_UP,
    FT_DOWN,
    FT_PAGEUP,
    FT_PAGEDOWN,
    FT_HOME,
    FT_END,
    FT_BOL,
    FT_BOS,
    FT_BOP,
    FT_BOF,
    FT_EOL,
    FT_EOS,
    FT_EOP,
    FT_EOF,
    FT_CENTER,
	FT_TRANSPOSE,
    FT_SCROLLDOWN,
    FT_SCROLLUP,
	FT_SCROLLLEFT,
	FT_SCROLLRIGHT,
	FT_SCROLLTOP,
	FT_SCROLLPAGEUP,
	FT_SCROLLBOTTOM,
	FT_SCROLLPAGEDOWN,
    FT_IMEINSERT,
	FT_SELECTALL,
};

inline uint4 getstyleint(uint4 flags)
{
	return flags & F_STYLE;
}

inline uint2 heightfromsize(uint2 size)
{
	return (size * 4 / 3);
}

enum Text_encoding {
    TE_ANSI,
    TE_UNICODE,
    TE_MULTIBYTE
};

enum Lang_charset
{
    LCH_ENGLISH = 0,
    LCH_ROMAN, //maps to macroman on macos
    LCH_JAPANESE,
    LCH_CHINESE,
    LCH_KOREAN,
    LCH_ARABIC,
    LCH_HEBREW,
    LCH_RUSSIAN,
    LCH_TURKISH,
    LCH_BULGARIAN,
    LCH_UKRAINIAN,
    LCH_POLISH,
    LCH_GREEK,
    LCH_SIMPLE_CHINESE,
    LCH_THAI,
	LCH_VIETNAMESE,
	LCH_LITHUANIAN,
    LCH_UNICODE,
    LCH_UTF8,
    // SN-2015-06-18: [[ Bug 11803 ]] Added for Android TextEncode
    LCH_WINDOWS_NATIVE,
	LCH_DEFAULT = 255
};

enum MCImagePaletteType
{
	kMCImagePaletteTypeEmpty,
	kMCImagePaletteTypeWebSafe,
	kMCImagePaletteTypeOptimal,
	kMCImagePaletteTypeCustom,
};

typedef struct _MCImagePaletteSettings
{
	MCImagePaletteType type;
	MCColor *colors;
	uint32_t ncolors;
} MCImagePaletteSettings;

// MW-2014-07-17: [[ ImageMetadata ]] Struct representing image metadata contents - at
//   some point (post-7.0) this would be better as an opaque type, but this will do for now.
struct MCImageMetadata
{
    bool has_density : 1;
    double density;
};

enum MCGravity
{
    kMCGravityNone,
    kMCGravityLeft,
    kMCGravityTop,
    kMCGravityRight,
    kMCGravityBottom,
    kMCGravityTopLeft,
    kMCGravityTopRight,
    kMCGravityBottomLeft,
    kMCGravityBottomRight,
    kMCGravityCenter,
    kMCGravityResize,
    kMCGravityResizeAspect,
    kMCGravityResizeAspectFill,
};

#endif
