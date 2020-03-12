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
#include "sysdefs.h"

#include "globals.h"
#include "object.h"
#include "stack.h"
#include "cdata.h"
#include "objptr.h"
#include "field.h"
#include "object.h"
#include "button.h"
#include "card.h"
#include "exec.h"

#include "stack.h"
#include "card.h"
#include "dispatch.h"
#include "parentscript.h"
#include "hndlrlst.h"
#include "mode.h"
#include "license.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "chunk.h"
#include "util.h"
#include "sellst.h"
#include "redraw.h"
#include "objectpropsets.h"
#include "font.h"

#include "exec-interface.h"

#include "module-engine.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct _PropList
{
	const char *token;
	uint2 value;
}
PropList;

static const PropList stackprops[] =
    {
        {"altId", P_ALT_ID},
        {"alwaysBuffer", P_ALWAYS_BUFFER},
        {"backColor", P_BACK_COLOR},
        {"backPattern", P_BACK_PATTERN},
        {"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderColor", P_BORDER_COLOR},
        {"borderPattern", P_BORDER_PATTERN},
        {"bottomColor", P_BOTTOM_COLOR},
        {"bottomPattern", P_BOTTOM_PATTERN},
        {"cantAbort", P_CANT_ABORT},
        {"cantDelete", P_CANT_DELETE},
        {"cantModify", P_CANT_MODIFY},
        {"decorations", P_DECORATIONS},
        {"destroyStack", P_DESTROY_STACK},
        {"destroyWindow", P_DESTROY_WINDOW},
        {"dynamicPaths", P_DYNAMIC_PATHS},
        {"editMenus", P_EDIT_MENUS},
        {"externals", P_EXTERNALS},
        {"fileName", P_FILE_NAME},
        {"focusColor", P_FOCUS_COLOR},
        {"focusPattern", P_FOCUS_PATTERN},
        {"foreColor", P_FORE_COLOR},
        {"forePattern", P_FORE_PATTERN},
        {"formatForPrinting", P_FORMAT_FOR_PRINTING},
        {"fullscreen", P_FULLSCREEN},
        {"fullscreenmode", P_FULLSCREENMODE},
        {"hcAddressing", P_HC_ADDRESSING},
        {"hiliteColor", P_HILITE_COLOR},
        {"hilitePattern", P_HILITE_PATTERN},
        {"icon", P_ICON},
        {"iconic", P_ICONIC},
        {"id", P_ID},
        {"linkColor", P_LINK_COLOR},
        {"linkHiliteColor", P_LINK_HILITE_COLOR},
        {"linkVisitedColor", P_LINK_VISITED_COLOR},
        {"liveResizing", P_LIVE_RESIZING},
        {"maxHeight", P_MAX_HEIGHT},
        {"maxWidth", P_MAX_WIDTH},
        {"menubar", P_MENU_BAR},
        {"metal", P_METAL},
        {"minHeight", P_MIN_HEIGHT},
        {"minWidth", P_MIN_WIDTH},
        {"name", P_SHORT_NAME},
        {"rect", P_RECTANGLE},
        {"resizable", P_RESIZABLE},
        {"scalefactor", P_SCALE_FACTOR},
        {"shadow", P_SHADOW},
        {"shadowColor", P_SHADOW_COLOR},
        {"shadowPattern", P_SHADOW_PATTERN},
        {"shadowOffset", P_SHADOW_OFFSET},
		{"showInvisibles", P_SHOW_INVISIBLES},
        {"startUpIconic", P_START_UP_ICONIC},
        {"style", P_STYLE},
        {"stackFiles", P_STACK_FILES},
        {"systemWindow", P_SYSTEM_WINDOW},
        {"textFont", P_TEXT_FONT},
        {"textSize", P_TEXT_SIZE},
        {"textStyle", P_TEXT_STYLE},
        {"title", P_LABEL},
        {"topColor", P_TOP_COLOR},
        {"topPattern", P_TOP_PATTERN},
        {"underlineLinks", P_UNDERLINE_LINKS},
        {"visible", P_VISIBLE},
        {"windowManagerPlace", P_WM_PLACE},
        {"windowShape", P_WINDOW_SHAPE}
    };

static const PropList cardprops[] =
    {
        {"altId", P_ALT_ID},
        {"backColor", P_BACK_COLOR},
        {"backPattern", P_BACK_PATTERN},
        {"behavior", P_PARENT_SCRIPT},
        {"borderColor", P_BORDER_COLOR},
        {"borderPattern", P_BORDER_PATTERN},
        {"borderWidth", P_BORDER_WIDTH},
        {"bottomColor", P_BOTTOM_COLOR},
        {"bottomPattern", P_BOTTOM_PATTERN},
        {"blendLevel", P_BLEND_LEVEL},
        {"cantDelete", P_CANT_DELETE},
        {"defaultButton", P_DEFAULT_BUTTON},
        {"dontSearch", P_DONT_SEARCH},
        {"focusColor", P_FOCUS_COLOR},
        {"focusPattern", P_FOCUS_PATTERN},
        {"foreColor", P_FORE_COLOR},
        {"forePattern", P_FORE_PATTERN},
        {"hiliteColor", P_HILITE_COLOR},
        {"hilitePattern", P_HILITE_PATTERN},
        {"id", P_ID},
        {"ink", P_INK},
        {"layer", P_LAYER},
        {"mark", P_MARKED},
        {"name", P_SHORT_NAME},
        {"shadowColor", P_SHADOW_COLOR},
        {"shadowPattern", P_SHADOW_PATTERN},
        {"shadowOffset", P_SHADOW_OFFSET},
        {"showBorder", P_SHOW_BORDER},
        {"textFont", P_TEXT_FONT},
        {"textSize", P_TEXT_SIZE},
        {"textStyle", P_TEXT_STYLE},
        {"threeD", P_3D},
        {"topColor", P_TOP_COLOR},
        {"topPattern", P_TOP_PATTERN}
    };

static const PropList groupprops[] =
    {
        {"altId", P_ALT_ID},
        {"backColor", P_BACK_COLOR},
        {"backPattern", P_BACK_PATTERN},
        {"backgroundBehavior", P_BACKGROUND_BEHAVIOR},
        {"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderColor", P_BORDER_COLOR},
        {"borderPattern", P_BORDER_PATTERN},
        {"borderWidth", P_BORDER_WIDTH},
        {"bottomColor", P_BOTTOM_COLOR},
        {"bottomPattern", P_BOTTOM_PATTERN},
        {"boundingRect", P_BOUNDING_RECT},
        {"cantDelete", P_CANT_DELETE},
        {"cantSelect", P_CANT_SELECT},
        {"clipsToRect", P_CLIPS_TO_RECT},
        {"colorOverlay", P_BITMAP_EFFECT_COLOR_OVERLAY},
        {"disabled", P_DISABLED},
        {"dontSearch", P_DONT_SEARCH},
        {"dropShadow", P_BITMAP_EFFECT_DROP_SHADOW},
        {"focusColor", P_FOCUS_COLOR},
        {"focusPattern", P_FOCUS_PATTERN},
        {"foreColor", P_FORE_COLOR},
        {"forePattern", P_FORE_PATTERN},
        {"hiliteColor", P_HILITE_COLOR},
        {"hilitedButton", P_HILITED_BUTTON},
        {"hilitePattern", P_HILITE_PATTERN},
        {"hScroll", P_HSCROLL},
        {"hScrollbar", P_HSCROLLBAR},
        {"id", P_ID},
        {"ink", P_INK},
        {"innerGlow", P_BITMAP_EFFECT_INNER_GLOW},
        {"innerShadow", P_BITMAP_EFFECT_INNER_SHADOW},
        {"layer", P_LAYER},
        {"layerMode", P_LAYER_MODE},
        {"lockLoc", P_LOCK_LOCATION},
        {"margins", P_MARGINS},
        {"name", P_SHORT_NAME},
        {"outerGlow", P_BITMAP_EFFECT_OUTER_GLOW},
        {"opaque", P_OPAQUE},
        {"radioBehavior", P_RADIO_BEHAVIOR},
        {"rect", P_RECTANGLE},
        // MERG-2013-06-24: [[ RevisedPropsProp ]] Include 'selectGroupedControls' in the group prop-list.
        {"selectGroupedControls", P_SELECT_GROUPED_CONTROLS},
        {"scrollbarWidth", P_SCROLLBAR_WIDTH},
        {"shadowColor", P_SHADOW_COLOR},
        {"shadowPattern", P_SHADOW_PATTERN},
        {"shadowOffset", P_SHADOW_OFFSET},
        {"sharedBehavior", P_SHARED_BEHAVIOR},
        {"showBorder", P_SHOW_BORDER},
        {"showFocusBorder", P_SHOW_FOCUS_BORDER},
        {"showName", P_SHOW_NAME},
        {"tabGroupBehavior", P_TAB_GROUP_BEHAVIOR},
        {"textFont", P_TEXT_FONT},
        {"textSize", P_TEXT_SIZE},
        {"textStyle", P_TEXT_STYLE},
        {"threeD", P_3D},
        {"topColor", P_TOP_COLOR},
        {"topPattern", P_TOP_PATTERN},
        {"traversalOn", P_TRAVERSAL_ON},
        {"unboundedHScroll", P_UNBOUNDED_HSCROLL},
        {"unboundedVScroll", P_UNBOUNDED_VSCROLL},
        {"label", P_LABEL},
        {"toolTip", P_TOOL_TIP},
        {"visible", P_VISIBLE},
        {"vScroll", P_VSCROLL},
        {"vScrollbar", P_VSCROLLBAR}
    };

static const PropList buttonprops[] =
    {
        {"accelKey", P_ACCELERATOR_KEY},
        {"accelMods", P_ACCELERATOR_MODIFIERS},
        {"accelText", P_ACCELERATOR_TEXT},
        {"altId", P_ALT_ID},
        {"armed", P_ARM},
        {"armedIcon", P_ARMED_ICON},
        {"armBorder", P_ARM_BORDER},
        {"armFill", P_ARM_FILL},
        {"autoArm", P_AUTO_ARM},
        {"autoHilite", P_AUTO_HILITE},
        {"backColor", P_BACK_COLOR},
        {"backPattern", P_BACK_PATTERN},
        {"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderColor", P_BORDER_COLOR},
        {"borderPattern", P_BORDER_PATTERN},
        {"borderWidth", P_BORDER_WIDTH},
        {"bottomColor", P_BOTTOM_COLOR},
        {"bottomPattern", P_BOTTOM_PATTERN},
        {"cantSelect", P_CANT_SELECT},
        {"colorOverlay", P_BITMAP_EFFECT_COLOR_OVERLAY},
        {"default", P_DEFAULT},
        {"disabled", P_DISABLED},
        {"disabledIcon", P_DISABLED_ICON},
        {"dropShadow", P_BITMAP_EFFECT_DROP_SHADOW},
        {"family", P_FAMILY},
        {"focusColor", P_FOCUS_COLOR},
        {"focusPattern", P_FOCUS_PATTERN},
        {"foreColor", P_FORE_COLOR},
        {"forePattern", P_FORE_PATTERN},
        {"hilited", P_HILITE},
        {"hiliteBorder", P_HILITE_BORDER},
        {"hiliteColor", P_HILITE_COLOR},
        {"hiliteFill", P_HILITE_FILL},
        {"hiliteIcon", P_HILITED_ICON},
        {"hilitePattern", P_HILITE_PATTERN},
        {"hoverIcon", P_HOVER_ICON},
        {"icon", P_ICON},
        // AL-2014-07-23: [[ Bug 12894 ]] Add iconGravity to image properties list
        {"iconGravity", P_ICON_GRAVITY},
        {"id", P_ID},
        {"ink", P_INK},
        {"innerGlow", P_BITMAP_EFFECT_INNER_GLOW},
        {"innerShadow", P_BITMAP_EFFECT_INNER_SHADOW},
        {"label", P_LABEL},
        {"labelWidth", P_LABEL_WIDTH},
        {"layer", P_LAYER},
        {"layerMode", P_LAYER_MODE},
        {"lockLoc", P_LOCK_LOCATION},
        {"margins", P_MARGINS},
        {"menuHistory", P_MENU_HISTORY},
        {"menuLines", P_MENU_LINES},
        {"menuMouseButton", P_MENU_BUTTON},
        {"menuMode", P_MENU_MODE},
        {"menuName", P_MENU_NAME},
        {"mnemonic", P_MNEMONIC},
        {"name", P_SHORT_NAME},
        {"outerGlow", P_BITMAP_EFFECT_OUTER_GLOW},
        {"opaque", P_OPAQUE},
        {"rect", P_RECTANGLE},
        {"shadow", P_SHADOW},
        {"shadowColor", P_SHADOW_COLOR},
        {"shadowOffset", P_SHADOW_OFFSET},
        {"shadowPattern", P_SHADOW_PATTERN},
        {"sharedHilite", P_SHARED_HILITE},
        {"showBorder", P_SHOW_BORDER},
        {"showFocusBorder", P_SHOW_FOCUS_BORDER},
        {"showHilite", P_SHOW_HILITE},
        {"showIcon", P_SHOW_ICON},
        {"showName", P_SHOW_NAME},
        {"style", P_STYLE},
        {"textAlign", P_TEXT_ALIGN},
        {"textFont", P_TEXT_FONT},
        {"textSize", P_TEXT_SIZE},
        {"textStyle", P_TEXT_STYLE},
        {"threeD", P_3D},
        {"titleWidth", P_LABEL_WIDTH},
        {"topColor", P_TOP_COLOR},
        {"topPattern", P_TOP_PATTERN},
        {"traversalOn", P_TRAVERSAL_ON},
        {"text", P_TEXT},
        {"toolTip", P_TOOL_TIP},
        {"visible", P_VISIBLE},
        {"visitedIcon", P_VISITED_ICON}
    };

static const PropList fieldprops[] =
    {
        {"altId", P_ALT_ID},
        {"autoHilite", P_AUTO_HILITE},
        {"autoTab", P_AUTO_TAB},
        {"backColor", P_BACK_COLOR},
        {"backPattern", P_BACK_PATTERN},
        {"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderColor", P_BORDER_COLOR},
        {"borderPattern", P_BORDER_PATTERN},
        {"borderWidth", P_BORDER_WIDTH},
        {"bottomColor", P_BOTTOM_COLOR},
        {"bottomPattern", P_BOTTOM_PATTERN},
        {"cantSelect", P_CANT_SELECT},
        {"colorOverlay", P_BITMAP_EFFECT_COLOR_OVERLAY},
        {"cursorMovement", P_CURSORMOVEMENT},
        {"disabled", P_DISABLED},
        {"dontSearch", P_DONT_SEARCH},
        {"dontWrap", P_DONT_WRAP},
        {"dropShadow", P_BITMAP_EFFECT_DROP_SHADOW},
        {"firstIndent", P_FIRST_INDENT},
        {"fixedLineHeight", P_FIXED_HEIGHT},
        {"focusColor", P_FOCUS_COLOR},
        {"focusPattern", P_FOCUS_PATTERN},
        {"foreColor", P_FORE_COLOR},
        {"forePattern", P_FORE_PATTERN},
        {"hGrid", P_HGRID},
        {"hiliteColor", P_HILITE_COLOR},
        {"hilitedLines", P_HILITED_LINES},
        {"hilitePattern", P_HILITE_PATTERN},
        {"hScroll", P_HSCROLL},
        {"hScrollbar", P_HSCROLLBAR},
        {"htmlText", P_HTML_TEXT},
        {"id", P_ID},
        {"ink", P_INK},
        {"innerGlow", P_BITMAP_EFFECT_INNER_GLOW},
        {"innerShadow", P_BITMAP_EFFECT_INNER_SHADOW},
        {"layer", P_LAYER},
        {"layerMode", P_LAYER_MODE},
        {"listBehavior", P_LIST_BEHAVIOR},
        {"lockLoc", P_LOCK_LOCATION},
        {"lockText", P_LOCK_TEXT},
        {"margins", P_MARGINS},
        {"multipleHilites", P_MULTIPLE_HILITES},
        {"name", P_SHORT_NAME},
        {"noncontiguousHilites", P_NONCONTIGUOUS_HILITES},
        {"outerGlow", P_BITMAP_EFFECT_OUTER_GLOW},
        {"opaque", P_OPAQUE},
        {"rect", P_RECTANGLE},
        {"scrollbarWidth", P_SCROLLBAR_WIDTH},
        {"shadow", P_SHADOW},
        {"shadowColor", P_SHADOW_COLOR},
        {"shadowOffset", P_SHADOW_OFFSET},
        {"shadowPattern", P_SHADOW_PATTERN},
        {"sharedText", P_SHARED_TEXT},
        {"showBorder", P_SHOW_BORDER},
        {"showFocusBorder", P_SHOW_FOCUS_BORDER},
        {"showLines", P_SHOW_LINES},
        {"style", P_STYLE},
        {"tabStops", P_TAB_STOPS},
        {"textAlign", P_TEXT_ALIGN},
        {"textDirection", P_TEXTDIRECTION},
        {"textFont", P_TEXT_FONT},
        {"textHeight", P_TEXT_HEIGHT},
        {"textSize", P_TEXT_SIZE},
        {"textStyle", P_TEXT_STYLE},
        {"threeD", P_3D},
        {"threeDHilite", P_3D_HILITE},
        {"toggleHilites", P_TOGGLE_HILITE},
        {"topColor", P_TOP_COLOR},
        {"topPattern", P_TOP_PATTERN},
        {"toolTip", P_TOOL_TIP},
        {"traversalOn", P_TRAVERSAL_ON},
        {"vGrid", P_VGRID},
        {"visible", P_VISIBLE},
        {"vScroll", P_VSCROLL},
        {"vScrollbar", P_VSCROLLBAR}
    };

static const PropList imageprops[] =
    {
        {"altId", P_ALT_ID},
        {"angle", P_ANGLE},
        {"alwaysBuffer", P_ALWAYS_BUFFER},
        {"backColor", P_BACK_COLOR},
        {"backPattern", P_BACK_PATTERN},
        {"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderColor", P_BORDER_COLOR},
        {"borderPattern", P_BORDER_PATTERN},
        {"borderWidth", P_BORDER_WIDTH},
        {"bottomColor", P_BOTTOM_COLOR},
        {"bottomPattern", P_BOTTOM_PATTERN},
        {"cantSelect", P_CANT_SELECT},
        // AL-2014-07-23: [[ Bug 12894 ]] Add centerRect to image properties list
        {"centerRect", P_CENTER_RECTANGLE},
        {"colorOverlay", P_BITMAP_EFFECT_COLOR_OVERLAY},
        {"constantMask", P_CONSTANT_MASK},
        {"currentFrame", P_CURRENT_FRAME},
        {"disabled", P_DISABLED},
        {"dontDither", P_DONT_DITHER},
        {"dropShadow", P_BITMAP_EFFECT_DROP_SHADOW},
        {"fileName", P_FILE_NAME},
        {"focusColor", P_FOCUS_COLOR},
        {"focusPattern", P_FOCUS_PATTERN},
        {"foreColor", P_FORE_COLOR},
        {"forePattern", P_FORE_PATTERN},
        {"hiliteColor", P_HILITE_COLOR},
        {"hilitePattern", P_HILITE_PATTERN},
        {"hotSpot", P_HOT_SPOT},
        {"id", P_ID},
        {"ink", P_INK},
        {"innerGlow", P_BITMAP_EFFECT_INNER_GLOW},
        {"innerShadow", P_BITMAP_EFFECT_INNER_SHADOW},
        {"layer", P_LAYER},
        {"layerMode", P_LAYER_MODE},
        {"lockLoc", P_LOCK_LOCATION},
        {"name", P_SHORT_NAME},
        {"outerGlow", P_BITMAP_EFFECT_OUTER_GLOW},
        {"opaque", P_OPAQUE},
        {"palindromeFrames", P_PALINDROME_FRAMES},
        {"rect", P_RECTANGLE},
        {"repeatCount", P_REPEAT_COUNT},
        {"resizeQuality", P_RESIZE_QUALITY},
        {"shadowColor", P_SHADOW_COLOR},
        {"shadowPattern", P_SHADOW_PATTERN},
        {"shadowOffset", P_SHADOW_OFFSET},
        {"showBorder", P_SHOW_BORDER},
        {"showFocusBorder", P_SHOW_FOCUS_BORDER},
        {"text", P_TEXT},
        {"threeD", P_3D},
        {"topColor", P_TOP_COLOR},
        {"topPattern", P_TOP_PATTERN},
        {"traversalOn", P_TRAVERSAL_ON},
        {"toolTip", P_TOOL_TIP},
        {"visible", P_VISIBLE},
        {"xHot", P_XHOT},
        {"yHot", P_YHOT}
    };

static const PropList graphicprops[] =
    {
        {"altId", P_ALT_ID},
        {"angle", P_ANGLE},
        {"antialiased", P_ANTI_ALIASED},
        {"arcAngle", P_ARC_ANGLE},
        {"arrowSize", P_ARROW_SIZE},
        {"backColor", P_BACK_COLOR},
        {"backPattern", P_BACK_PATTERN},
        {"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderColor", P_BORDER_COLOR},
        {"borderPattern", P_BORDER_PATTERN},
        {"borderWidth", P_BORDER_WIDTH},
        {"bottomColor", P_BOTTOM_COLOR},
        {"bottomPattern", P_BOTTOM_PATTERN},
        {"cantSelect", P_CANT_SELECT},
        {"colorOverlay", P_BITMAP_EFFECT_COLOR_OVERLAY},
        {"dashes", P_DASHES},
        {"disabled", P_DISABLED},
        {"dontResize", P_DONT_RESIZE},
        {"dropShadow", P_BITMAP_EFFECT_DROP_SHADOW},
        {"endArrow", P_END_ARROW},
        {"filled", P_FILLED},
        {"focusColor", P_FOCUS_COLOR},
        {"focusPattern", P_FOCUS_PATTERN},
        {"foreColor", P_FORE_COLOR},
        {"forePattern", P_FORE_PATTERN},
        {"hiliteColor", P_HILITE_COLOR},
        {"hilitePattern", P_HILITE_PATTERN},
        {"id", P_ID},
        {"ink", P_INK},
        {"innerGlow", P_BITMAP_EFFECT_INNER_GLOW},
        {"innerShadow", P_BITMAP_EFFECT_INNER_SHADOW},
        {"label", P_LABEL},
        {"layer", P_LAYER},
        {"layerMode", P_LAYER_MODE},
        {"lineSize", P_LINE_SIZE},
        {"lockLoc", P_LOCK_LOCATION},
        {"margins", P_MARGINS},
        {"markerDrawn", P_MARKER_DRAWN},
        {"markerFilled", P_MARKER_OPAQUE},
        {"markerPoints", P_MARKER_POINTS},
        {"name", P_SHORT_NAME},
        {"points", P_POINTS},
        {"polySides", P_POLY_SIDES},
        {"outerGlow", P_BITMAP_EFFECT_OUTER_GLOW},
        {"opaque", P_OPAQUE},
        {"rect", P_RECTANGLE},
        {"roundEnds", P_ROUND_ENDS},
        {"roundRadius", P_ROUND_RADIUS},
        {"shadowColor", P_SHADOW_COLOR},
        {"shadowPattern", P_SHADOW_PATTERN},
        {"shadowOffset", P_SHADOW_OFFSET},
        {"showBorder", P_SHOW_BORDER},
        {"showFocusBorder", P_SHOW_FOCUS_BORDER},
        {"showName", P_SHOW_NAME},
        {"startArrow", P_START_ARROW},
        {"style", P_STYLE},
        {"textFont", P_TEXT_FONT},
        {"textSize", P_TEXT_SIZE},
        {"textStyle", P_TEXT_STYLE},
        {"threeD", P_3D},
        {"topColor", P_TOP_COLOR},
        {"topPattern", P_TOP_PATTERN},
        {"traversalOn", P_TRAVERSAL_ON},
        {"toolTip", P_TOOL_TIP},
        {"visible", P_VISIBLE},
        {"markerLineSize", P_MARKER_LSIZE},
        {"startAngle", P_START_ANGLE},
        {"fillRule", P_FILL_RULE}, // PROPERTY - FILL RULE
        {"fillGradient", P_GRADIENT_FILL},
        {"strokeGradient", P_GRADIENT_STROKE},
        {"editMode", P_EDIT_MODE},
        {"capStyle", P_CAP_STYLE},
        {"joinStyle", P_JOIN_STYLE},
        {"miterLimit", P_MITER_LIMIT},
    };

static const PropList scrollbarprops[] =
    {
        {"altId", P_ALT_ID},
        {"backColor", P_BACK_COLOR},
        {"backPattern", P_BACK_PATTERN},
        {"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderColor", P_BORDER_COLOR},
        {"borderPattern", P_BORDER_PATTERN},
        {"borderWidth", P_BORDER_WIDTH},
        {"bottomColor", P_BOTTOM_COLOR},
        {"bottomPattern", P_BOTTOM_PATTERN},
        {"cantSelect", P_CANT_SELECT},
        {"colorOverlay", P_BITMAP_EFFECT_COLOR_OVERLAY},
        {"disabled", P_DISABLED},
        {"dropShadow", P_BITMAP_EFFECT_DROP_SHADOW},
        {"endValue", P_END_VALUE},
        {"focusColor", P_FOCUS_COLOR},
        {"focusPattern", P_FOCUS_PATTERN},
        {"foreColor", P_FORE_COLOR},
        {"forePattern", P_FORE_PATTERN},
        {"hiliteColor", P_HILITE_COLOR},
        {"hilitePattern", P_HILITE_PATTERN},
        {"id", P_ID},
        {"ink", P_INK},
        {"innerGlow", P_BITMAP_EFFECT_INNER_GLOW},
        {"innerShadow", P_BITMAP_EFFECT_INNER_SHADOW},
        {"layer", P_LAYER},
        {"layerMode", P_LAYER_MODE},
        {"lineInc", P_LINE_INC},
        {"lockLoc", P_LOCK_LOCATION},
        {"name", P_SHORT_NAME},
        {"numberFormat", P_NUMBER_FORMAT},
        {"outerGlow", P_BITMAP_EFFECT_OUTER_GLOW},
        {"opaque", P_OPAQUE},
        {"pageInc", P_PAGE_INC},
        {"rect", P_RECTANGLE},
        {"shadowColor", P_SHADOW_COLOR},
        {"shadowPattern", P_SHADOW_PATTERN},
        {"shadowOffset", P_SHADOW_OFFSET},
        {"showBorder", P_SHOW_BORDER},
        {"showFocusBorder", P_SHOW_FOCUS_BORDER},
        {"showValue", P_SHOW_VALUE},
        {"startValue", P_START_VALUE},
        {"style", P_STYLE},
        {"textFont", P_TEXT_FONT},
        {"textSize", P_TEXT_SIZE},
        {"textStyle", P_TEXT_STYLE},
        {"threeD", P_3D},
        {"thumbPosition", P_THUMB_POS},
        {"toolTip", P_TOOL_TIP},
        {"topColor", P_TOP_COLOR},
        {"topPattern", P_TOP_PATTERN},
        {"traversalOn", P_TRAVERSAL_ON},
        {"visible", P_VISIBLE},
        {"thumbSize", P_THUMB_SIZE}
    };

static const PropList playerprops[] =
    {
        {"altId", P_ALT_ID},
        {"alwaysBuffer", P_ALWAYS_BUFFER},
        {"backColor", P_BACK_COLOR},
        {"backPattern", P_BACK_PATTERN},
        {"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderColor", P_BORDER_COLOR},
        {"borderPattern", P_BORDER_PATTERN},
        {"borderWidth", P_BORDER_WIDTH},
        {"bottomColor", P_BOTTOM_COLOR},
        {"bottomPattern", P_BOTTOM_PATTERN},
        {"callbacks", P_CALLBACKS},
        {"cantSelect", P_CANT_SELECT},
        {"colorOverlay", P_BITMAP_EFFECT_COLOR_OVERLAY},
        {"disabled", P_DISABLED},
        {"dropShadow", P_BITMAP_EFFECT_DROP_SHADOW},
        {"endTime", P_END_TIME},
        {"fileName", P_FILE_NAME},
        {"focusColor", P_FOCUS_COLOR},
        {"focusPattern", P_FOCUS_PATTERN},
        {"foreColor", P_FORE_COLOR},
        {"forePattern", P_FORE_PATTERN},
        {"hiliteColor", P_HILITE_COLOR},
        {"hilitePattern", P_HILITE_PATTERN},
        {"id", P_ID},
        {"ink", P_INK},
        {"innerGlow", P_BITMAP_EFFECT_INNER_GLOW},
        {"innerShadow", P_BITMAP_EFFECT_INNER_SHADOW},
        {"layer", P_LAYER},
        {"layerMode", P_LAYER_MODE},
        {"lockLoc", P_LOCK_LOCATION},
        {"looping", P_LOOPING},
        {"name", P_SHORT_NAME},
        {"outerGlow", P_BITMAP_EFFECT_OUTER_GLOW},
        {"opaque", P_OPAQUE},
        {"playRate", P_PLAY_RATE},
        {"playSelection", P_PLAY_SELECTION},
        {"rect", P_RECTANGLE},
        //{"selectedareacolor", P_SELECTED_AREA_COLOR},
        {"shadowColor", P_SHADOW_COLOR},
        {"shadowPattern", P_SHADOW_PATTERN},
        {"shadowOffset", P_SHADOW_OFFSET},
        {"showBadge", P_SHOW_BADGE},
        {"showBorder", P_SHOW_BORDER},
        {"showController", P_SHOW_CONTROLLER},
        {"showFocusBorder", P_SHOW_FOCUS_BORDER},
        {"showSelection", P_SHOW_SELECTION},
        {"startTime", P_START_TIME},
        {"textFont", P_TEXT_FONT},
        {"textSize", P_TEXT_SIZE},
        {"textStyle", P_TEXT_STYLE},
        {"threeD", P_3D},
        {"toolTip", P_TOOL_TIP},
        {"topColor", P_TOP_COLOR},
        {"topPattern", P_TOP_PATTERN},
        {"traversalOn", P_TRAVERSAL_ON},
        {"visible", P_VISIBLE},
    };

static const PropList epsprops[] =
    {
        {"altId", P_ALT_ID},
        {"backColor", P_BACK_COLOR},
        {"backPattern", P_BACK_PATTERN},
        {"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderColor", P_BORDER_COLOR},
        {"borderPattern", P_BORDER_PATTERN},
        {"borderWidth", P_BORDER_WIDTH},
        {"bottomColor", P_BOTTOM_COLOR},
        {"bottomPattern", P_BOTTOM_PATTERN},
        {"boundingBox", P_BOUNDING_RECT},
        {"cantSelect", P_CANT_SELECT},
        {"currentPage", P_CURRENT_PAGE},
        {"disabled", P_DISABLED},
        {"focusColor", P_FOCUS_COLOR},
        {"focusPattern", P_FOCUS_PATTERN},
        {"foreColor", P_FORE_COLOR},
        {"forePattern", P_FORE_PATTERN},
        {"hiliteColor", P_HILITE_COLOR},
        {"hilitePattern", P_HILITE_PATTERN},
        {"id", P_ID},
        {"ink", P_INK},
        {"layer", P_LAYER},
        {"lockLoc", P_LOCK_LOCATION},
        {"name", P_SHORT_NAME},
        {"opaque", P_OPAQUE},
        {"pageCount", P_PAGE_COUNT},
        {"postScript", P_POSTSCRIPT},
        {"prolog", P_PROLOG},
        {"rect", P_RECTANGLE},
        {"retainImage", P_RETAIN_IMAGE},
        {"retainPostScript", P_RETAIN_POSTSCRIPT},
        {"scale", P_SCALE},
        {"scaleIndependently", P_SCALE_INDEPENDENTLY},
        {"shadowColor", P_SHADOW_COLOR},
        {"shadowPattern", P_SHADOW_PATTERN},
        {"showBorder", P_SHOW_BORDER},
        {"showFocusBorder", P_SHOW_FOCUS_BORDER},
        {"threeD", P_3D},
        {"toolTip", P_TOOL_TIP},
        {"topColor", P_TOP_COLOR},
        {"topPattern", P_TOP_PATTERN},
        {"traversalOn", P_TRAVERSAL_ON},
        {"visible", P_VISIBLE},
        {"xExtent", P_X_EXTENT},
        {"xOffset", P_X_OFFSET},
        {"xScale", P_X_SCALE},
        {"yExtent", P_Y_EXTENT},
        {"yOffset", P_Y_OFFSET},
        {"xScale", P_X_SCALE},
        {"yScale", P_Y_SCALE}
    };

static const PropList colorpaletteprops[] =
    {
        {"name", P_SHORT_NAME},
        {"id", P_ID},
        {"selectedColor", P_SELECTED_COLOR},
        {"rect", P_RECTANGLE}
    };

static const PropList audioclipprops[] =
    {
        {"altID", P_ALT_ID},
        {"id", P_ID},
        {"name", P_NAME},
        {"playLoudness", P_PLAY_LOUDNESS},
    };

static const PropList videoclipprops[] =
    {
        {"altID", P_ALT_ID},
        {"dontRefresh", P_DONT_REFRESH},
        {"frameRate", P_FRAME_RATE},
        {"id", P_ID},
        {"name", P_NAME},
        {"playLoudness", P_PLAY_LOUDNESS},
        {"scale", P_SCALE},
    };

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceLayerParse(MCExecContext& ctxt, MCStringRef p_input, MCInterfaceLayer& r_output)
{
	uint4 layer;
	if (MCStringIsEqualToCString(p_input, MCtopstring, kMCCompareCaseless))
		r_output . layer = MAXINT2;
	else if (MCStringIsEqualToCString(p_input, MCbottomstring, kMCCompareCaseless))
		r_output . layer = 1;
	else if (MCU_stoui4(p_input, layer))
		r_output . layer = layer;
	else
		ctxt . LegacyThrow(EE_OBJECT_LAYERNAN);
}

void MCInterfaceLayerFormat(MCExecContext& ctxt, const MCInterfaceLayer& p_input, MCStringRef& r_output)
{
	if (MCStringFormat(r_output, "%d", p_input . layer))
		return;

	ctxt . Throw();
}

void MCInterfaceLayerFree(MCExecContext& ctxt, MCInterfaceLayer& p_input)
{
}

static MCExecCustomTypeInfo _kMCInterfaceLayerTypeInfo =
{
	"Interface.Layer",
	sizeof(MCInterfaceLayer),
	(void *)MCInterfaceLayerParse,
	(void *)MCInterfaceLayerFormat,
	(void *)MCInterfaceLayerFree
};

//////////

static void MCInterfaceTextStyleParse(MCExecContext& ctxt, MCStringRef p_input, MCInterfaceTextStyle& r_output)
{
	uindex_t t_length;
	t_length = MCStringGetLength(p_input);

	if (t_length == 0)
	{
		r_output . style = 0;
		return;
	}

	uint2 style;
	style = FA_DEFAULT_STYLE;

	bool t_success;
	t_success = true;

	uindex_t t_old_offset;
	t_old_offset = 0;
	uindex_t t_new_offset;
	t_new_offset = 0;

	while (t_success && t_old_offset <= t_length)
	{
		MCAutoStringRef t_text_style;
		
		if (!MCStringFirstIndexOfChar(p_input, ',', t_old_offset, kMCCompareExact, t_new_offset))
			t_new_offset = t_length;

		while (MCStringGetNativeCharAtIndex(p_input, t_old_offset) == ' ')
			t_old_offset++;

		t_success = MCStringCopySubstring(p_input, MCRangeMakeMinMax(t_old_offset, t_new_offset), &t_text_style);

		if (t_success)
		{
			t_old_offset = t_new_offset + 1;

			if (MCF_setweightstring(style, *t_text_style))
				continue;
			if (MCF_setexpandstring(style, *t_text_style))
				continue;
			if (MCStringIsEqualToCString(*t_text_style, "oblique", kMCCompareCaseless))
			{
				style |= FA_OBLIQUE;
				continue;
			}
			if (MCStringIsEqualToCString(*t_text_style, "italic", kMCCompareCaseless))
			{
				style |= FA_ITALIC;
				continue;
			}
			if (MCStringIsEqualToCString(*t_text_style, MCplainstring, kMCCompareCaseless))
			{
				style = FA_DEFAULT_STYLE;
				continue;
			}
			if (MCStringIsEqualToCString(*t_text_style, MCmixedstring, kMCCompareCaseless))
			{
				style = FA_DEFAULT_STYLE;
				continue;
			}
			if (MCStringIsEqualToCString(*t_text_style, MCboxstring, kMCCompareCaseless))
			{
				style &= ~FA_3D_BOX;
				style |= FA_BOX;
				continue;
			}
			if (MCStringIsEqualToCString(*t_text_style, MCthreedboxstring, kMCCompareCaseless))
			{
				style &= ~FA_BOX;
				style |= FA_3D_BOX;
				continue;
			}
			if (MCStringIsEqualToCString(*t_text_style, MCunderlinestring, kMCCompareCaseless))
			{
				style |= FA_UNDERLINE;
				continue;
			}
			if (MCStringIsEqualToCString(*t_text_style, MCstrikeoutstring, kMCCompareCaseless))
			{
				style |= FA_STRIKEOUT;
				continue;
			}
			if (MCStringIsEqualToCString(*t_text_style, MCgroupstring, kMCCompareCaseless) || 
				MCStringIsEqualToCString(*t_text_style, MClinkstring, kMCCompareCaseless))
			{
				style |= FA_LINK;
				continue;
			}

			ctxt . LegacyThrow(EE_OBJECT_BADSTYLE);
			return;
		}
	}

	r_output . style = style;
}

static void MCInterfaceTextStyleFormat(MCExecContext& ctxt, const MCInterfaceTextStyle& p_input, MCStringRef& r_output)
{
	if (p_input . style == FA_DEFAULT_STYLE)
	{
		if (!MCStringCreateWithCString(MCplainstring, r_output))
			ctxt . Throw();
		return;
	}

    if (p_input . style == 0)
    {
        r_output = MCValueRetain(kMCEmptyString);
        return;
    }
    
	bool t_success;
	t_success = true;

	MCAutoListRef t_styles;

	if (t_success)
		t_success = MCListCreateMutable(',', &t_styles);

	if (t_success && MCF_getweightint(p_input . style) != MCFW_MEDIUM)
	{
		MCAutoStringRef t_style;
		t_success = (MCStringCreateWithCString(MCF_getweightstring(p_input . style), &t_style) &&
						MCListAppend(*t_styles, *t_style));
	}

	if (t_success && (p_input . style & FA_ITALIC || p_input . style & FA_OBLIQUE))
	{
		MCAutoStringRef t_style;
		t_success = (MCStringCreateWithCString(MCF_getslantlongstring(p_input . style), &t_style) &&
						MCListAppend(*t_styles, *t_style));
	}

	if (t_success && p_input . style & FA_BOX)
	{
		MCAutoStringRef t_style;
		t_success = (MCStringCreateWithCString(MCboxstring, &t_style) &&
						MCListAppend(*t_styles, *t_style));
	}

	if (t_success && p_input . style & FA_3D_BOX)
	{
		MCAutoStringRef t_style;
		t_success = (MCStringCreateWithCString(MCthreedboxstring, &t_style) &&
						MCListAppend(*t_styles, *t_style));
	}

	if (t_success && p_input . style & FA_UNDERLINE)
	{
		MCAutoStringRef t_style;
		t_success = (MCStringCreateWithCString(MCunderlinestring, &t_style) &&
						MCListAppend(*t_styles, *t_style));
	}

	if (t_success && p_input . style & FA_STRIKEOUT)
	{
		MCAutoStringRef t_style;
		t_success = (MCStringCreateWithCString(MCstrikeoutstring, &t_style) &&
						MCListAppend(*t_styles, *t_style));
	}

	if (t_success && p_input . style & FA_LINK)
	{
		MCAutoStringRef t_style;
		t_success = (MCStringCreateWithCString(MClinkstring, &t_style) &&
						MCListAppend(*t_styles, *t_style));
	}

	if (t_success && MCF_getexpandint(p_input . style) != FE_NORMAL)
	{
		MCAutoStringRef t_style;
		t_success = (MCStringCreateWithCString(MCF_getexpandstring(p_input . style), &t_style) &&
						MCListAppend(*t_styles, *t_style));
	}	

	if (t_success)
		t_success = MCListCopyAsString(*t_styles, r_output);

	if (t_success)
		return;

	ctxt . Throw();
}

static void MCInterfaceTextStyleFree(MCExecContext& ctxt, MCInterfaceTextStyle& p_input)
{
}

static MCExecCustomTypeInfo _kMCInterfaceTextStyleTypeInfo =
{
	"Interface.TextStyle",
	sizeof(MCInterfaceTextStyle),
	(void *)MCInterfaceTextStyleParse,
	(void *)MCInterfaceTextStyleFormat,
	(void *)MCInterfaceTextStyleFree
};

//////////

struct MCInterfaceShadow
{
	bool is_flag;

	union
	{
		bool flag;
		int2 shadow;
	};
};

static void MCInterfaceShadowParse(MCExecContext& ctxt, MCStringRef p_input, MCInterfaceShadow& r_output)
{
	if (MCTypeConvertStringToBool(p_input, r_output . flag))
		r_output . is_flag = true;
	else if (MCU_stoi2(p_input, r_output . shadow))
		r_output . is_flag = false;
	else
		ctxt . LegacyThrow(EE_OBJECT_NAB);
}

static void MCInterfaceShadowFormat(MCExecContext& ctxt, const MCInterfaceShadow& p_input, MCStringRef& r_output)
{
    if (p_input . is_flag)
    {
        if (p_input . flag)
            r_output = MCValueRetain(kMCTrueString);
        else
            r_output = MCValueRetain(kMCFalseString);
    }
    else
        ctxt . FormatInteger(p_input . shadow, r_output);
}

static void MCInterfaceShadowFree(MCExecContext& ctxt, MCInterfaceShadow& p_input)
{
}

static MCExecCustomTypeInfo _kMCInterfaceShadowTypeInfo =
{
	"Interface.Shadow",
	sizeof(MCInterfaceShadow),
	(void *)MCInterfaceShadowParse,
	(void *)MCInterfaceShadowFormat,
	(void *)MCInterfaceShadowFree
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceTextAlignElementInfo[] =
{	
	{ MCleftstring, F_ALIGN_LEFT, false },
	{ "", F_ALIGN_LEFT, false },
	{ MCcenterstring, F_ALIGN_CENTER, false },
	{ MCrightstring, F_ALIGN_RIGHT, false },
	{ MCjustifystring, F_ALIGN_JUSTIFY, false },
};

static MCExecEnumTypeInfo _kMCInterfaceTextAlignTypeInfo =
{
	"Interface.TextAlign",
	sizeof(_kMCInterfaceTextAlignElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceTextAlignElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceInkNamesElementInfo[] =
{	
	{ "clear", GXclear, false },
	{ "srcAnd", GXand, false },
	{ "srcAndReverse", GXandReverse, false },
	{ "srcCopy", GXcopy, false },
	{ "notSrcAnd", GXandInverted, false },
	{ "noop", GXnoop, false },
	{ "srcXor", GXxor, false },
	{ "srcOr", GXor, false },
	{ "notSrcAndReverse", GXnor, false },
	{ "notSrcXor", GXequiv, false },
	{ "reverse", GXinvert, false },
	{ "srcOrReverse", GXorReverse, false },
	{ "notSrcCopy", GXcopyInverted, false },
	{ "notSrcOr", GXorInverted, false },
	{ "notSrcOrReverse", GXnand, false },
	{ "set", GXset, false },
	{ "srcBic", GXsrcBic, false },
	{ "notSrcBic", GXnotSrcBic, false },
	{ "blend", GXblend, false },
	{ "addpin", GXaddpin, false },
	{ "addOver", GXaddOver, false },
	{ "subPin", GXsubPin, false },
	{ "transparent", GXtransparent, false },
	{ "addMax", GXaddMax, false },
	{ "subOver", GXsubOver, false },
	{ "adMin", GXaddMin, false },
	{ "blendClear", GXblendClear, false },
	{ "blendSrc", GXblendSrc, false },
	{ "blendDst", GXblendDst, false },
	{ "blendSrcOver", GXblendSrcOver, false },
	{ "blendDstOver", GXblendDstOver, false },
	{ "blendSrcIn", GXblendSrcIn, false },
	{ "blendDstIn", GXblendDstIn, false },
	{ "blendSrcOut", GXblendSrcOut, false },
	{ "blendDstOut", GXblendDstOut, false },
	{ "blendSrcAtop", GXblendSrcAtop, false },
	{ "blendDstAtop", GXblendDstAtop, false },
	{ "blendXor", GXblendXor, false },
	{ "blendPlus", GXblendPlus, false },
	{ "blendMultiply", GXblendMultiply, false },
	{ "blendScreen", GXblendScreen, false },
	{ "blendOverlay", GXblendOverlay, false },
	{ "blendDarken", GXblendDarken, false },
	{ "blendLighten", GXblendLighten, false },
	{ "blendDodge", GXblendDodge, false },
	{ "blendBurn", GXblendBurn, false },
	{ "blendHardLight", GXblendHardLight, false },
	{ "blendSoftLight", GXblendSoftLight, false },
	{ "blendDifference", GXblendDifference, false },
	{ "blendExclusion", GXblendExclusion, false },
};

static MCExecEnumTypeInfo _kMCInterfaceInkNamesTypeInfo =
{
	"Interface.InkNames",
	sizeof(_kMCInterfaceInkNamesElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceInkNamesElementInfo
};

//////////

void MCInterfaceTriStateParse(MCExecContext& ctxt, MCStringRef p_input, MCInterfaceTriState& r_output)
{
    if (MCStringIsEqualToCString(p_input, "mixed", kMCCompareCaseless))
    {
        r_output . value = kMCTristateMixed;
        return;
    }

    bool t_bool = false;
    if (MCTypeConvertStringToBool(p_input, t_bool))
    {
        r_output . value = t_bool;
        return;
    }
    
    ctxt . LegacyThrow(EE_OBJECT_NAB);
}

void MCInterfaceTriStateFormat(MCExecContext& ctxt, const MCInterfaceTriState& p_input, MCStringRef& r_output)
{
    if (p_input.value.isMixed())
    {
        if (!MCStringCreateWithCString("mixed", r_output))
            ctxt.Throw();
        return;
    }

    r_output = MCValueRetain(p_input.value.isFalse() ? kMCFalseString : kMCTrueString);
}

void MCInterfaceTriStateFree(MCExecContext& ctxt, MCInterfaceTriState& p_input)
{
}

static MCExecCustomTypeInfo _kMCInterfaceTriStateTypeInfo =
{
	"Interface.TriState",
	sizeof(MCInterfaceTriState),
	(void *)MCInterfaceTriStateParse,
	(void *)MCInterfaceTriStateFormat,
	(void *)MCInterfaceTriStateFree
};

//////////

MCExecEnumTypeElementInfo _kMCInterfaceEncodingElementInfo[] =
{
	{ MCnativestring, 0, true },
	{ MCunicodestring, 1, true },
};

MCExecEnumTypeInfo _kMCInterfaceEncodingTypeInfo =
{
	"Interface.Encoding",
	sizeof(_kMCInterfaceEncodingElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceEncodingElementInfo
};

//////////

MCExecEnumTypeElementInfo _kMCInterfaceListStyleElementInfo[] =
{
	{ "", 0, false },
	{ "disc", 1, false },
	{ "circle", 2, false },
	{ "square", 3, false },
	{ "decimal", 4, false },
	{ "lower latin", 5, false },
	{ "upper latin", 6, false },
	{ "lower roman", 7, false },
	{ "upper roman", 8, false },
	{ "skip", 9, false },
};

MCExecEnumTypeInfo _kMCInterfaceListStyleTypeInfo =
{
	"Interface.ListStyle",
	sizeof(_kMCInterfaceListStyleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceListStyleElementInfo
};

//////////

MCExecEnumTypeElementInfo _kMCInterfaceThemeElementInfo[] =
{
    { "", kMCInterfaceThemeEmpty, false },
    { "native", kMCInterfaceThemeNative, false },
    { "legacy", kMCInterfaceThemeLegacy, false },
};

MCExecEnumTypeInfo _kMCInterfaceThemeTypeInfo =
{
    "Interface.Theme",
    sizeof (_kMCInterfaceThemeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCInterfaceThemeElementInfo
};

//////////

MCExecEnumTypeElementInfo _kMCInterfaceThemeControlTypeElementInfo[] =
{
    { "", kMCPlatformControlTypeGeneric, false },
    { "button", kMCPlatformControlTypeButton, false },
    { "checkbox", kMCPlatformControlTypeCheckbox, false },
    { "radiobutton", kMCPlatformControlTypeRadioButton, false },
    { "tabbutton", kMCPlatformControlTypeTabButton, false },
    { "tabpane", kMCPlatformControlTypeTabPane, false },
    { "label", kMCPlatformControlTypeLabel, false },
    { "inputfield", kMCPlatformControlTypeInputField, false },
    { "list", kMCPlatformControlTypeList, false },
    { "menu", kMCPlatformControlTypeMenu, false },
    { "menuitem", kMCPlatformControlTypeMenuItem, false },
    { "optionmenu", kMCPlatformControlTypeOptionMenu, false },
    { "pulldownmenu", kMCPlatformControlTypePulldownMenu, false },
    { "combobox", kMCPlatformControlTypeComboBox, false },
    { "popupmenu", kMCPlatformControlTypePopupMenu, false },
    { "progressbar", kMCPlatformControlTypeProgressBar, false },
    { "scrollbar", kMCPlatformControlTypeScrollBar, false },
    { "slider", kMCPlatformControlTypeSlider, false },
    { "spinarrows", kMCPlatformControlTypeSpinArrows, false },
    { "window", kMCPlatformControlTypeWindow, false },
    { "messagebox", kMCPlatformControlTypeMessageBox, false },
    { "richtext", kMCPlatformControlTypeRichText, false },
    { "tooltip", kMCPlatformControlTypeTooltip, false },
};

MCExecEnumTypeInfo _kMCInterfaceThemeControlTypeTypeInfo =
{
    "Interface.ControlType",
    sizeof (_kMCInterfaceThemeControlTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCInterfaceThemeControlTypeElementInfo
};

//////////

MCExecEnumTypeElementInfo _kMCInterfaceScriptStatusElementInfo[] =
{
    { "compiled", kMCInterfaceScriptStatusCompiled, false },
    { "uncompiled", kMCInterfaceScriptStatusUncompiled, false },
    { "warning", kMCInterfaceScriptStatusWarning, false },
    { "error", kMCInterfaceScriptStatusError, false },
};

MCExecEnumTypeInfo _kMCInterfaceScriptStatusTypeInfo =
{
    "Interface.ScriptStatus",
    sizeof(_kMCInterfaceScriptStatusElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCInterfaceScriptStatusElementInfo
};


////////////////////////////////////////////////////////////////////////////////

MCExecCustomTypeInfo *kMCInterfaceLayerTypeInfo = &_kMCInterfaceLayerTypeInfo;
MCExecCustomTypeInfo *kMCInterfaceShadowTypeInfo = &_kMCInterfaceShadowTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceTextAlignTypeInfo = &_kMCInterfaceTextAlignTypeInfo;
MCExecCustomTypeInfo *kMCInterfaceTextStyleTypeInfo = &_kMCInterfaceTextStyleTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceInkNamesTypeInfo = &_kMCInterfaceInkNamesTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceEncodingTypeInfo = &_kMCInterfaceEncodingTypeInfo;
MCExecCustomTypeInfo *kMCInterfaceTriStateTypeInfo = &_kMCInterfaceTriStateTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceListStyleTypeInfo = &_kMCInterfaceListStyleTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceThemeTypeInfo = &_kMCInterfaceThemeTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceThemeControlTypeTypeInfo = &_kMCInterfaceThemeControlTypeTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceScriptStatusTypeInfo = &_kMCInterfaceScriptStatusTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCObject::Redraw(void)
{
	if (!opened)
		return;
	
	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	if (gettype() >= CT_GROUP)
		static_cast<MCControl *>(this) -> layer_redrawall();
}

////////////////////////////////////////////////////////////////////////////////

struct MCObjectChangeIdVisitor: public MCObjectVisitor
{
	uint32_t old_card_id;
	uint32_t new_card_id;

	void Process(MCCdata *p_cdata)
	{
		if (p_cdata == nil)
			return;

		MCCdata *t_ptr;
		t_ptr = p_cdata;
		do
		{
			if (t_ptr -> getid() == old_card_id)
			{
				t_ptr -> setid(new_card_id);
				return;
			}

			t_ptr = t_ptr -> next();
		}
		while(t_ptr != p_cdata);
	}

	bool OnField(MCField *p_field)
	{
		Process(p_field -> getcdata());
		return true;
	}

	bool OnButton(MCButton *p_button)
	{
		Process(p_button -> getcdata());
		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

void MCObject::GetId(MCExecContext& ctxt, uint32_t& r_id)
{
	r_id = obj_id;
}

void MCObject::SetId(MCExecContext& ctxt, uint32_t p_new_id)
{
	if (obj_id == p_new_id)
		return;

	// MW-2010-05-18: (Silently) don't allow id == 0 - this prevents people working around the
	//   script limits, which don't come into effect on objects with 0 id.
	if (p_new_id == 0)
		return;
	
	// MW-2011-02-08: Don't allow id change if the parent is nil as this means its a template
	//   object which doesn't really have an id.
	if (!parent)
		return;
	
	MCStack *t_stack;
	t_stack = getstack();

	if (t_stack -> isediting())
	{
		ctxt . LegacyThrow(EE_OBJECT_NOTWHILEEDITING);
		return;
	}

	// If the stack's id is less than the requested id then we are fine
	// since the stack id is always greater or equal to the highest numbered
	// control/card id. Otherwise, check the whole list of controls and cards.
	if (p_new_id <= t_stack -> getid())
	{
		if (t_stack -> getcontrolid(CT_LAYER, p_new_id) != NULL ||
			t_stack -> findcardbyid(p_new_id) != NULL)
		{
			ctxt . LegacyThrow(EE_OBJECT_IDINUSE, p_new_id);
			return;
		}
	}
	else
		t_stack -> obj_id = p_new_id;

	// If the object is a card, we have to reset all the control's data
	// id's.
	// If the object is not a card, but has a card as parent, we need to
	// reset the card's objptr id for it.
	if (gettype() == CT_CARD)
	{
		MCObjectChangeIdVisitor t_visitor;
		t_visitor . old_card_id = obj_id;
		t_visitor . new_card_id = p_new_id;
		t_stack -> visit(VISIT_STYLE_DEPTH_FIRST, 0, &t_visitor);
	}
	else if (parent -> gettype() == CT_CARD)
		parent.GetAs<MCCard>()->resetid(obj_id, p_new_id);

	// MW-2012-10-10: [[ IdCache ]] If the object is in the cache, then remove
	//   it since its id is changing.
	if (m_in_id_cache)
		t_stack -> uncacheobjectbyid(this);

	uint4 oldid = obj_id;
	obj_id = p_new_id;
	message_with_args(MCM_id_changed, oldid, obj_id);
}

void MCObject::GetAbbrevId(MCExecContext& ctxt, MCStringRef& r_abbrev_id)
{
	MCAutoValueRef t_abbrev_id;
	if (names(P_ABBREV_ID, &t_abbrev_id))
		if (ctxt.ConvertToString(*t_abbrev_id, r_abbrev_id))
			return;
	
	ctxt . Throw();
}

void MCObject::GetLongName(MCExecContext& ctxt, uint32_t p_part_id, MCStringRef& r_long_name)
{
	MCAutoValueRef t_long_name;
	if (getnameproperty(P_LONG_NAME, p_part_id, &t_long_name))
		if (ctxt.ConvertToString(*t_long_name, r_long_name))
			return;
	
	ctxt . Throw();
}

void MCObject::GetLongId(MCExecContext& ctxt, uint32_t p_part_id, MCStringRef& r_long_id)
{
	MCAutoValueRef t_long_id;
	if (getnameproperty(P_LONG_ID, p_part_id, &t_long_id))
		if (ctxt.ConvertToString(*t_long_id, r_long_id))
			return;
	
	ctxt . Throw();
}

void MCObject::GetName(MCExecContext& ctxt, MCStringRef& r_name)
{
	MCAutoValueRef t_name;
	if (names(P_NAME, &t_name))
		if (ctxt.ConvertToString(*t_name, r_name))
			return;
	
	ctxt . Throw();
}

void MCObject::SetName(MCExecContext& ctxt, MCStringRef p_name)
{
	bool t_success;
	t_success = true;

	// Cannot have return characters in object names.
	MCAutoStringRef t_new_string;
	if (t_success)
		t_success = MCStringMutableCopy(p_name, &t_new_string);
	if (t_success)
		t_success = MCStringFindAndReplaceChar(*t_new_string, '\n', '_', kMCStringOptionCompareExact);
		
	MCNewAutoNameRef t_new_name;
	if (t_success)
		t_success = MCNameCreate(*t_new_string, &t_new_name);
		
	// MW-2012-09-12; [[ Bug ]] Make sure we compare literally, otherwise can't
	//   change case of names of objects.
	if (t_success && getname() != *t_new_name)
	{
		MCNewAutoNameRef t_old_name = getname();
		setname(*t_new_name);
		message_with_valueref_args(MCM_name_changed, *t_old_name, getname());
	}
	
	if (t_success)
	{
		Redraw();
		return;
	}

	ctxt . Throw();
}

void MCObject::GetAbbrevName(MCExecContext& ctxt, MCStringRef& r_abbrev_name)
{
	MCAutoValueRef t_abbrev_name;
	if (names(P_ABBREV_NAME, &t_abbrev_name))
		if (ctxt.ConvertToString(*t_abbrev_name, r_abbrev_name))
			return;
	
	ctxt . Throw();
}

void MCObject::GetShortName(MCExecContext& ctxt, MCStringRef& r_short_name)
{
	MCAutoValueRef t_short_name;
	if (names(P_SHORT_NAME, &t_short_name))
		if (ctxt.ConvertToString(*t_short_name, r_short_name))
			return;
	
	ctxt . Throw();
}

void MCObject::GetAltId(MCExecContext& ctxt, uint32_t& r_alt_id)
{
	r_alt_id = altid;
}

void MCObject::SetAltId(MCExecContext& ctxt, uint32_t p_new_alt_id)
{
	altid = p_new_alt_id;
}

////////////////////////////////////////////////////////////////////////////////

void MCObject::GetLayer(MCExecContext& ctxt, uint32_t part, MCInterfaceLayer& r_layer)
{
	// OK-2009-03-12: [[Bug 8049]] - Fix relayering of grouped objects broken by 
	// previous fix for crash when attempting to get the layer of an object outside
	// the group being edited in edit group mode.
	
	uint2 num = 0;
	if (parent)
	{
		MCCard *t_card;
		t_card = getcard(part);
		
		if(part != 0 && t_card == NULL)
			t_card = getstack() -> findcardbyid(part);
		
		if (t_card == NULL)
		{
			// This shouldn't happen, but rather than a crash, throw a random execution error..
			ctxt . LegacyThrow(EE_CHUNK_NOCARD);
			return;
		}
		
		t_card -> count(CT_LAYER, CT_UNDEFINED, this, num, True);
	}
	
	r_layer . layer = num;
}

void MCObject::SetLayer(MCExecContext& ctxt, uint32_t part, const MCInterfaceLayer& p_layer)
{
	if (!parent || getcard(part)->relayer((MCControl *)this, p_layer . layer) != ES_NORMAL)
		ctxt . LegacyThrow(EE_OBJECT_BADRELAYER);
}

void MCObject::GetScript(MCExecContext& ctxt, MCStringRef& r_script)
{
	if (!MCdispatcher -> cut(True))
	{
		ctxt . LegacyThrow(EE_OBJECT_NOHOME);
		return;
	}

	if (!getstack() -> iskeyed())
	{
		ctxt . LegacyThrow(EE_STACK_NOKEY);
		return;
	}

	getstack() -> unsecurescript(this);
	r_script = MCValueRetain(_script);
	getstack() -> securescript(this);
}

void MCObject::SetScript(MCExecContext& ctxt, MCStringRef new_script)
{
	if (!MCdispatcher->cut(True))
	{
		ctxt . LegacyThrow(EE_OBJECT_NOHOME);
		return;
	}
	if (!getstack()->iskeyed())
	{
		ctxt . LegacyThrow(EE_STACK_NOKEY);
		return;
	}
	if (scriptdepth != 0)
	{
		ctxt . LegacyThrow(EE_OBJECT_SCRIPTEXECUTING);
		return;
	}

	bool t_success;
	t_success = true;

	uint4 length;
	length = MCStringGetLength(new_script);

	if (MCStringIsEmpty(new_script))
	{
		delete hlist;
		hlist = NULL;
		MCValueRelease(_script);
		_script = MCValueRetain(kMCEmptyString);
		hashandlers = 0;
	}
	else
	{
		MCAutoStringRef t_old_script;
		t_old_script = _script;
		
        bool t_old_script_encrypted = m_script_encrypted;
        
		MCAutoStringRef t_new_script;
		if (MCStringGetNativeCharAtIndex(new_script, length - 1) != '\n')
		{
			MCAutoStringRef t_script;
			if (t_success)
				t_success = MCStringMutableCopy(new_script, &t_script);
			if (t_success)
				t_success = MCStringAppendChar(*t_script, '\n');
			if (t_success)
				/* UNCHECKED */ MCStringCopy(*t_script, &t_new_script);
		}
		else
			t_new_script = new_script;
		
		MCValueAssign(_script, *t_new_script);

        // IM-2013-05-29: [[ BZ 10916 ]] flag new script as unencrypted
		m_script_encrypted = false;
		getstack() -> securescript(this);
		
		if (t_success)
		{
			if (MCModeCanSetObjectScript(obj_id))
			{ // not template object
				hashandlers = 0;
				parsescript(False, True);
				if (hlist != NULL && MClicenseparameters . script_limit > 0 && hlist -> linecount() >= MClicenseparameters . script_limit)
				{
					delete hlist;
					hlist = NULL;
					MCValueAssign(_script, *t_old_script);
                    m_script_encrypted = t_old_script_encrypted;
					MCperror->add(PE_OBJECT_NOTLICENSED, 0, 0);
				}
				if (!MCperror->isempty())
				{
                    MCAutoStringRef t_error;
                    MCperror -> copyasstringref(&t_error);

                    ctxt . SetTheResultToValue(*t_error);
					MCperror->clear();
				}
				else
					ctxt . SetTheResultToEmpty();
			}
		}
	}

	if (t_success)
		return;

	ctxt . Throw();
}

void MCObject::GetParentScript(MCExecContext& ctxt, MCStringRef& r_parent_script)
{
	// MW-2008-10-25: Handle the parentScript property when getting
	// If there is a parent script we return a reference string
	if (parent_script != nil)
	{
		MCParentScript *t_parent;
		t_parent = parent_script -> GetParent();
		
        if (t_parent -> GetObjectId() != 0)
        {
            if (MCStringFormat(r_parent_script, "button id %d of stack \"%@\"", t_parent -> GetObjectId(),
                               t_parent -> GetObjectStack()))
            
                return;
        }
        else
        {
            if (MCStringFormat(r_parent_script, "stack \"%@\"",
                               t_parent -> GetObjectStack()))
                
                return;
        }

		ctxt . Throw();
	}
}

void MCObject::SetParentScript(MCExecContext& ctxt, MCStringRef new_parent_script)
{
    MCObject *t_current_parent = nil;
    if (parent_script != nil)
    {
        t_current_parent = parent_script -> GetParent() -> GetObject();
    }
    
    // MW-2008-10-25: Add the setting logic for parent scripts. This code is a
	//   modified version of what goes on in MCChunk::getobj when the final
	//   target for a chunk is an expression. We first parse the string as a
	//   chunk expression, then attempt to get the object of it. If the object
	//   doesn't exist, the set fails.
	
	// MW-2008-11-02: [[ Bug ]] Setting the parentScript of an object to
	//   empty should unset the parent script property and not throw an
	//   error.
	if (MCStringIsEmpty(new_parent_script))
	{
        if (t_current_parent != nil &&
            t_current_parent -> getscriptdepth() > 0)
        {
            ctxt . LegacyThrow(EE_PARENTSCRIPT_EXECUTING);
            return;
        }
        
		if (parent_script != NULL)
			parent_script -> Release();
		parent_script = NULL;
		return;
	}

	// Create a script point with the value are setting the property to
	// as source text.
	MCScriptPoint sp(new_parent_script);

	// Create a new chunk object to parse the reference into
	/* UNCHECKED */ MCAutoPointer<MCChunk> t_chunk = new (nothrow) MCChunk(False);

	// Attempt to parse a chunk. We also check that there is no 'junk' at
	// the end of the string - if there is, its an error. Note the errorlock
	// here - it stops parse errors being pushed onto MCperror.
	Symbol_type t_next_type;
	MCerrorlock++;
	bool t_success = (t_chunk -> parse(sp, False) == PS_NORMAL &&
	                  sp.next(t_next_type) == PS_EOF);
	MCerrorlock--;

	// Now attempt to evaluate the object reference - this will only succeed
    // if the object exists.
	MCObject *t_object = nil;
	uint32_t t_part_id;
	if (t_success)
		t_success = t_chunk -> getobj(ctxt, t_object, t_part_id, False);
	
	// Check to see if we are already parent-linked to t_object and if so
	// do nothing.
	if (t_current_parent == t_object)
		return;
	
	if (t_current_parent != nil &&
	    t_current_parent -> getscriptdepth() > 0)
	{
		ctxt . LegacyThrow(EE_PARENTSCRIPT_EXECUTING);
		return;
	}
	
	// Check that the object is a button or a stack.
	if (t_success &&
		t_object -> gettype() != CT_BUTTON &&
		t_object -> gettype() != CT_STACK)
		t_success = false;
	
	// MW-2013-07-18: [[ Bug 11037 ]] Make sure the object isn't in the hierarchy
	//   of the parentScript.
	if (t_success)
	{
		MCObject *t_parent_object;
		t_parent_object = t_object;
		while(t_parent_object != nil)
		{
			if (t_parent_object == this)
			{
				ctxt . LegacyThrow(EE_PARENTSCRIPT_CYCLICOBJECT);
				return;
			}
			
			MCParentScript *t_super_parent_script;
			t_super_parent_script = t_parent_object -> getparentscript();
			if (t_super_parent_script != nil)
				t_parent_object = t_super_parent_script -> GetObject();
			else
				t_parent_object = nil;
		}
	}

	if (!t_success)
	{
		ctxt . LegacyThrow(EE_PARENTSCRIPT_BADOBJECT);
		return;
	}
		
	// We have the target object, so extract its rugged id. That is the
	// (id, stack, mainstack) triple. Note that mainstack is NULL if the
	// object lies on a mainstack.
	//
	uint32_t t_id;
	t_id = t_object -> getid();

	// If the object is a stack, then it has an id of zero.
	if (t_object -> gettype() == CT_STACK)
		t_id = 0;
	
	MCNameRef t_stack;
	t_stack = t_object -> getstack() -> getname();

	// Now attempt to acquire a parent script use object. This can only
	// fail if memory is exhausted, so in this case just return an error
	// stat.
	MCParentScriptUse *t_use;
	t_use = MCParentScript::Acquire(this, t_id, t_stack);
	t_success = t_use != nil;

	// MW-2013-05-30: [[ InheritedPscripts ]] Make sure we resolve the the
	//   parent script as pointing to the object (so Inherit works correctly).
	if (t_success)
		t_use -> GetParent() -> Resolve(t_object);
	
	// MW-2013-05-30: [[ InheritedPscripts ]] Next we have to ensure the
	//   inheritence hierarchy is in place (The inherit call will create
	//   super-uses, and will return false if there is not enough memory).
	if (t_success)
		t_success = t_use -> Inherit();

	// We have succeeded in creating a new use of an object as a parent
	// script, so now release the old parent script this object points
	// to (if any) and install the new one.
	if (parent_script != NULL)
		parent_script -> Release();

	parent_script = t_use;

	// MW-2013-05-30: [[ InheritedPscripts ]] Make sure we update all the
	//   uses of this object if it is being used as a parentScript. This
	//   is because the inheritence hierarchy has been updated and so the
	//   super_use chains need to be remade.
	MCParentScript *t_this_parent;
	if (getisparentscript())
	{
		t_this_parent = MCParentScript::Lookup(this);
		if (t_success && t_this_parent != nil)
			t_success = t_this_parent -> Reinherit();
	}

	if (t_success)
		return;

	ctxt . Throw();
}

void MCObject::GetNumber(MCExecContext& ctxt, uint32_t part, uinteger_t& r_number)
{
	uint2 num;
	if (getstack() -> hcaddress())
	{
		if (parent -> gettype() == CT_CARD)
			getcard(part) -> count(gettype(), CT_CARD, this, num, True);
		else
			getcard(part) -> count(gettype(), CT_BACKGROUND, this, num, True);
	}
	else
		getcard(part) -> count(gettype(), CT_UNDEFINED, this, num, True);

	r_number = num;
}

bool MCObject::GetPixel(MCExecContext& ctxt, Properties which, bool effective, uinteger_t& r_pixel)
{
    MCInterfaceNamedColor t_color;
    MCInterfaceNamedColorInit(ctxt, t_color);
    
    // Change the property name from *Pixel to *Color
    Properties t_which;
    t_which = Properties(which - P_FORE_PIXEL + P_FORE_COLOR);
    
    if (GetColor(ctxt, t_which, effective, t_color))
    {
        r_pixel = MCColorGetPixel(t_color.color) & 0x00FFFFFF;
        
        MCInterfaceNamedColorFree(ctxt, t_color);
        return true;
    }
    
    return false;
}

void MCObject::SetPixel(MCExecContext& ctxt, Properties which, uinteger_t pixel)
{
	uint2 i;
	if (!getcindex(which - P_FORE_PIXEL, i))
		i = createcindex(which - P_FORE_PIXEL);

	MCColorSetPixel(colors[i], pixel);
	if (colornames[i] != nil)
	{
		MCValueRelease(colornames[i]);
		colornames[i] = nil;
	}
	
	Redraw();
}

void MCObject::GetEffectiveForePixel(MCExecContext& ctxt, uinteger_t& r_pixel)
{
	GetPixel(ctxt, P_FORE_PIXEL, true, r_pixel);
}

void MCObject::GetForePixel(MCExecContext& ctxt, uinteger_t*& r_pixel)
{
	if (GetPixel(ctxt, P_FORE_PIXEL, false, *r_pixel))
		return;

	r_pixel = nil;
}

void MCObject::SetForePixel(MCExecContext& ctxt, uinteger_t* pixel)
{
	if (pixel != nil)
		SetPixel(ctxt, P_FORE_PIXEL, *pixel);
}

void MCObject::GetEffectiveBackPixel(MCExecContext& ctxt, uinteger_t& r_pixel)
{
	GetPixel(ctxt, P_BACK_PIXEL, true, r_pixel);
}

void MCObject::GetBackPixel(MCExecContext& ctxt, uinteger_t*& r_pixel)
{
	if (GetPixel(ctxt, P_BACK_PIXEL, false, *r_pixel))
		return;

	r_pixel = nil;
}

void MCObject::SetBackPixel(MCExecContext& ctxt, uinteger_t* pixel)
{
	if (pixel != nil)
		SetPixel(ctxt, P_BACK_PIXEL, *pixel);
}

void MCObject::GetEffectiveHilitePixel(MCExecContext& ctxt, uinteger_t& r_pixel)
{
	GetPixel(ctxt, P_HILITE_PIXEL, true, r_pixel);
}

void MCObject::GetHilitePixel(MCExecContext& ctxt, uinteger_t*& r_pixel)
{
	if (GetPixel(ctxt, P_HILITE_PIXEL, false, *r_pixel))
		return;

	r_pixel = nil;
}

void MCObject::SetHilitePixel(MCExecContext& ctxt, uinteger_t* pixel)
{
	if (pixel != nil)
		SetPixel(ctxt, P_HILITE_PIXEL, *pixel);
}

void MCObject::GetEffectiveBorderPixel(MCExecContext& ctxt, uinteger_t& r_pixel)
{
	GetPixel(ctxt, P_BORDER_PIXEL, true, r_pixel);
}

void MCObject::GetBorderPixel(MCExecContext& ctxt, uinteger_t*& r_pixel)
{
	if (GetPixel(ctxt, P_BORDER_PIXEL, false, *r_pixel))
		return;

	r_pixel = nil;
}

void MCObject::SetBorderPixel(MCExecContext& ctxt, uinteger_t* pixel)
{
	if (pixel != nil)
		SetPixel(ctxt, P_BORDER_PIXEL, *pixel);
}

void MCObject::GetEffectiveTopPixel(MCExecContext& ctxt, uinteger_t& r_pixel)
{
	GetPixel(ctxt, P_TOP_PIXEL, true, r_pixel);
}

void MCObject::GetTopPixel(MCExecContext& ctxt, uinteger_t*& r_pixel)
{
	if (GetPixel(ctxt, P_TOP_PIXEL, false, *r_pixel))
		return;

	r_pixel = nil;
}

void MCObject::SetTopPixel(MCExecContext& ctxt, uinteger_t* pixel)
{
	if (pixel != nil)
		SetPixel(ctxt, P_TOP_PIXEL, *pixel);
}

void MCObject::GetEffectiveBottomPixel(MCExecContext& ctxt, uinteger_t& r_pixel)
{
	GetPixel(ctxt, P_BOTTOM_PIXEL, true, r_pixel);
}

void MCObject::GetBottomPixel(MCExecContext& ctxt, uinteger_t*& r_pixel)
{
	if (GetPixel(ctxt, P_BOTTOM_PIXEL, false, *r_pixel))
		return;

	r_pixel = nil;
}

void MCObject::SetBottomPixel(MCExecContext& ctxt, uinteger_t* pixel)
{
	if (pixel != nil)
		SetPixel(ctxt, P_BOTTOM_PIXEL, *pixel);
}

void MCObject::GetEffectiveShadowPixel(MCExecContext& ctxt, uinteger_t& r_pixel)
{
	GetPixel(ctxt, P_SHADOW_PIXEL, true, r_pixel);
}

void MCObject::GetShadowPixel(MCExecContext& ctxt, uinteger_t*& r_pixel)
{
	if (GetPixel(ctxt, P_SHADOW_PIXEL, false, *r_pixel))
		return;

	r_pixel = nil;
}

void MCObject::SetShadowPixel(MCExecContext& ctxt, uinteger_t* pixel)
{
	if (pixel != nil)
		SetPixel(ctxt, P_SHADOW_PIXEL, *pixel);
}

void MCObject::GetEffectiveFocusPixel(MCExecContext& ctxt, uinteger_t& r_pixel)
{
	GetPixel(ctxt, P_FOCUS_PIXEL, true, r_pixel);
}

void MCObject::GetFocusPixel(MCExecContext& ctxt, uinteger_t*& r_pixel)
{
	if (GetPixel(ctxt, P_FOCUS_PIXEL, false, *r_pixel))
		return;

	r_pixel = nil;
}

void MCObject::SetFocusPixel(MCExecContext& ctxt, uinteger_t* pixel)
{
	if (pixel != nil)
		SetPixel(ctxt, P_FOCUS_PIXEL, *pixel);
}
////////////////////////////////////////////////////////////////////////////////

void MCObject::GetPenBackColor(MCExecContext& ctxt, MCValueRef& r_value)
{
	 // NO OP
}

void MCObject::SetPenBackColor(MCExecContext& ctxt, MCValueRef r_value)
{
	// NO OP
}

void MCObject::GetBrushBackColor(MCExecContext& ctxt, MCValueRef& r_value)
{
	// NO OP
}

void MCObject::SetBrushBackColor(MCExecContext& ctxt, MCValueRef r_value)
{
	// NO OP
}

void MCObject::SetColor(MCExecContext& ctxt, int index, const MCInterfaceNamedColor& p_color)
{
	uint2 i, j;
	if (p_color . name != nil && MCStringIsEmpty(p_color . name))
	{
		if (getcindex(index, i))
			destroycindex(index, i);
	}
	else
	{
		if (!getcindex(index, i))
		{
			i = createcindex(index);
			colors[i].red = colors[i].green = colors[i].blue = 0;
		}
		set_interface_color(colors[i], colornames[i], p_color);

		j = i;
		if (getpindex(index, j))
		{
			if (opened)
				MCpatternlist->freepat(patterns[j].pattern);
			destroypindex(index, j);
		}
	}
}

bool MCObject::GetColor(MCExecContext& ctxt, Properties which, bool effective, MCInterfaceNamedColor& r_color, bool recursive)
{
	uint2 i;
	if (getcindex(which - P_FORE_COLOR, i))
	{
		get_interface_color(colors[i], colornames[i], r_color);
        
		return true;
	}
	else if (effective)
    {
        bool t_found;
        t_found = false;
        
        if (parent)
            t_found = parent -> GetColor(ctxt, which, effective, r_color, true);
        
        if (!t_found && !recursive)
        {
            // Look up the colour using the theming API
            MCPlatformControlType t_control_type;
            MCPlatformControlPart t_control_part;
            MCPlatformControlState t_control_state;
            MCPlatformThemeProperty t_control_prop;
            MCPlatformThemePropertyType t_control_prop_type;
            if (getthemeselectorsforprop(which, t_control_type, t_control_part, t_control_state, t_control_prop, t_control_prop_type))
            {
                MCColor t_color;
                if (MCPlatformGetControlThemePropColor(t_control_type, t_control_part, t_control_state, t_control_prop, t_color))
                {
                    t_found = true;
                    r_color.color = t_color;
                    r_color.name = nil;
                }

            }
            
            // Only fall back to the dispatcher's default colours if theming failed
            if (!t_found)
                t_found = MCdispatcher -> GetColor(ctxt, which, effective, r_color);
        }
        
        return t_found;
    }
	else
	{
		r_color . name = MCValueRetain(kMCEmptyString);
		return true;
	}

	return false;
}

void MCObject::GetForeColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	if (GetColor(ctxt, P_FORE_COLOR, false, r_color))
		return;

	ctxt . Throw();
}

void MCObject::SetForeColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color)
{
	SetColor(ctxt, DI_FORE, color);
	Redraw();
}

void MCObject::GetEffectiveForeColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	GetColor(ctxt, P_FORE_COLOR, true, r_color);
}

void MCObject::GetBackColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	if (GetColor(ctxt, P_BACK_COLOR, false, r_color))
		return;

	ctxt . Throw();
}

void MCObject::SetBackColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color)
{
	SetColor(ctxt, DI_BACK, color);
	Redraw();
}

void MCObject::GetEffectiveBackColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	GetColor(ctxt, P_BACK_COLOR, true, r_color);
}

void MCObject::GetHiliteColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	if (GetColor(ctxt, P_HILITE_COLOR, false, r_color))
		return;

	ctxt . Throw();
}

void MCObject::SetHiliteColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color)
{
	SetColor(ctxt, P_HILITE_COLOR - P_FORE_COLOR, color);
	Redraw();
}

void MCObject::GetEffectiveHiliteColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	GetColor(ctxt, P_HILITE_COLOR, true, r_color);
}

void MCObject::GetBorderColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	if (GetColor(ctxt, P_BORDER_COLOR, false, r_color))
		return;

	ctxt . Throw();
}

void MCObject::SetBorderColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color)
{
	SetColor(ctxt, P_BORDER_COLOR - P_FORE_COLOR, color);
	Redraw();
}

void MCObject::GetEffectiveBorderColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	GetColor(ctxt, P_BORDER_COLOR, true, r_color);
}

void MCObject::GetTopColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	if (GetColor(ctxt, P_TOP_COLOR, false, r_color))
		return;

	ctxt . Throw();
}

void MCObject::SetTopColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color)
{
	SetColor(ctxt, P_TOP_COLOR - P_FORE_COLOR, color);
	Redraw();
}

void MCObject::GetEffectiveTopColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	GetColor(ctxt, P_TOP_COLOR, true, r_color);
}

void MCObject::GetBottomColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	if (GetColor(ctxt, P_BOTTOM_COLOR, false, r_color))
		return;

	ctxt . Throw();
}

void MCObject::SetBottomColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color)
{
	SetColor(ctxt, P_BOTTOM_COLOR - P_FORE_COLOR, color);
	Redraw();
}

void MCObject::GetEffectiveBottomColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	GetColor(ctxt, P_BOTTOM_COLOR, true, r_color);
}

void MCObject::GetShadowColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	if (GetColor(ctxt, P_SHADOW_COLOR, false, r_color))
		return;

	ctxt . Throw();
}

void MCObject::SetShadowColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color)
{
	SetColor(ctxt, P_SHADOW_COLOR - P_FORE_COLOR, color);
	Redraw();
}

void MCObject::GetEffectiveShadowColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	GetColor(ctxt, P_SHADOW_COLOR, true, r_color);
}

void MCObject::GetFocusColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	if (GetColor(ctxt, P_FOCUS_COLOR, false, r_color))
		return;

	ctxt . Throw();
}

void MCObject::SetFocusColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color)
{
	SetColor(ctxt, P_FOCUS_COLOR - P_FORE_COLOR, color);
	Redraw();
}

void MCObject::GetEffectiveFocusColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	GetColor(ctxt, P_FOCUS_COLOR, true, r_color);
}

bool MCObject::GetColors(MCExecContext& ctxt, bool effective, MCStringRef& r_colors)
{
	bool t_success;
	t_success = true;

	MCAutoListRef t_color_list;
	if (t_success)
		t_success = MCListCreateMutable('\n', &t_color_list);

	if (t_success)
	{
		for (uint2 p = P_FORE_COLOR; p <= P_FOCUS_COLOR; p++)
		{
			MCInterfaceNamedColor t_color;
			MCAutoStringRef t_color_string;
			t_success = GetColor(ctxt, (Properties)p, effective, t_color);
			if (t_success)
			{
				MCInterfaceNamedColorFormat(ctxt, t_color, &t_color_string);
				MCInterfaceNamedColorFree(ctxt, t_color);
				t_success = !ctxt . HasError();
			}
			if (t_success)
				t_success = MCListAppend(*t_color_list, *t_color_string);

			if (!t_success)
				break;
		}
	}

	if (t_success)
		t_success = MCListCopyAsString(*t_color_list, r_colors);

	return t_success;
}

void MCObject::GetColors(MCExecContext& ctxt, MCStringRef& r_colors)
{
	if (GetColors(ctxt, false, r_colors))
		return;

	ctxt. Throw();
}

void MCObject::SetColors(MCExecContext& ctxt, MCStringRef p_input)
{
	bool t_success;
	t_success = true;

	uindex_t t_old_offset = 0;
	uindex_t t_new_offset = 0;
	uindex_t t_length;
	t_length = MCStringGetLength(p_input);

	for (uint2 index = DI_FORE; index <= DI_FOCUS; index++)
	{
		MCAutoStringRef t_color_string;
		if (!MCStringFirstIndexOfChar(p_input, '\n', t_old_offset, kMCCompareExact, t_new_offset))
			t_new_offset = t_length;
		
		// PM-2015-12-02: [[ Bug 16524 ]] Make sure empty lines reset color props
		if (t_new_offset == t_old_offset)
		{
			uint2 i;
			if (getcindex(index, i))
				destroycindex(index, i);
		}
		else if (t_new_offset > t_old_offset)
		{
			MCInterfaceNamedColor t_color;
			t_success = MCStringCopySubstring(p_input, MCRangeMakeMinMax(t_old_offset, t_new_offset), &t_color_string);
			if (t_success)
			{
				MCInterfaceNamedColorParse(ctxt, *t_color_string, t_color);
				t_success = !ctxt . HasError();
			}
			if (t_success)
			{
				uint2 i, j;
				if (getpindex(index, j))
				{
					if (opened)
						MCpatternlist->freepat(patterns[j].pattern);
					destroypindex(index, j);
				}
				if (!getcindex(index, i))
				{
					i = createcindex(index);
					colors[i] = t_color . color;
					colornames[i] = t_color . name == nil ? nil : MCValueRetain(t_color . name);
				}
				else
				{
					if (colornames[i] != nil)
					{
						MCValueRelease(colornames[i]);
						colornames[i] = nil;
					}
					if (opened)
					{
						colors[i] = t_color . color;
					}
					colornames[i] = t_color . name == nil ? nil : MCValueRetain(t_color . name);
				}
				MCInterfaceNamedColorFree(ctxt, t_color);
			}
		}
		t_old_offset = t_new_offset + 1;
		if (t_old_offset > t_length)
			break;
	}

	if (t_success)
	{
		Redraw();
		return;
	}

	ctxt . Throw();	
}

void MCObject::GetEffectiveColors(MCExecContext& ctxt, MCStringRef& r_colors)
{
	if (GetColors(ctxt, true, r_colors))
		return;

	ctxt. Throw();
}

////////////////////////////////////////////////////////////////////////////////

bool MCObject::GetPattern(MCExecContext& ctxt, Properties which, bool effective, uinteger_t*& r_pattern)
{
    uint2 i;
    if (getpindex(which - P_FORE_PATTERN, i))
    {
        if (patterns[i].id <= PI_END && patterns[i].id >= PI_PATTERNS)
            *r_pattern = patterns[i].id - PI_PATTERNS;
        else
            *r_pattern = patterns[i].id;
        return true;
    }
    else
    {
        if (effective)
        {
            if (parent)
                return parent->GetPattern(ctxt, which, effective, r_pattern);
            else
            {
                // AL-2014-11-18: [[ Bug 14055 ]] Effective pattern needs to be optional as
                //  exisiting behavior is to return empty for no pattern
                MCdispatcher -> GetDefaultPattern(ctxt, r_pattern);
                return true;
            }
        }
    }
    
    return false;
}

void MCObject::SetPattern(MCExecContext& ctxt, uint2 p_new_pixmap, uint4* p_new_id)
{
    uint2 i;
    bool t_isopened;
    t_isopened = (opened != 0) || (gettype() == CT_STACK && static_cast<MCStack*>(this)->getextendedstate(ECS_ISEXTRAOPENED));
    if (p_new_id == nil || *p_new_id == 0)
    {
        if (getpindex(p_new_pixmap, i))
        {
            if (t_isopened)
                MCpatternlist->freepat(patterns[i].pattern);
            destroypindex(p_new_pixmap, i);
        }
    }
    else
    {
        if (!getpindex(p_new_pixmap, i))
            i = createpindex(p_new_pixmap);
        else
            if (t_isopened)
                MCpatternlist->freepat(patterns[i].pattern);
        if (*p_new_id <= PI_END - PI_PATTERNS)
            *p_new_id += PI_PATTERNS;
        patterns[i].id = *p_new_id;
        if (t_isopened)
            patterns[i].pattern = MCpatternlist->allocpat(patterns[i].id, this);
        if (getcindex(p_new_pixmap, i))
            destroycindex(p_new_pixmap, i);
    }
}

void MCObject::GetPenPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    r_pattern = nil;
}

void MCObject::SetPenPattern(MCExecContext& ctxt, uinteger_t* pattern)
{
    SetPattern(ctxt, DI_FORE, pattern);
    Redraw();
}

void MCObject::GetBrushPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    r_pattern = nil;
}

void MCObject::SetBrushPattern(MCExecContext& ctxt, uinteger_t* pattern)
{
    SetPattern(ctxt, DI_BACK, pattern);
    Redraw();
}

void MCObject::GetForePattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    if (GetPattern(ctxt, P_FORE_PATTERN, false, r_pattern))
        return;
    
    r_pattern = nil;
}

void MCObject::SetForePattern(MCExecContext& ctxt, uinteger_t* pattern)
{
    SetPattern(ctxt, DI_FORE, pattern);
    Redraw();
}

void MCObject::GetEffectiveForePattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    GetPattern(ctxt, P_FORE_PATTERN, true, r_pattern);
}

void MCObject::GetBackPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    if (GetPattern(ctxt, P_BACK_PATTERN, false, r_pattern))
        return;
    
    r_pattern = nil;
}

void MCObject::SetBackPattern(MCExecContext& ctxt, uinteger_t* pattern)
{
    SetPattern(ctxt, DI_BACK, pattern);
    Redraw();
}

void MCObject::GetEffectiveBackPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    GetPattern(ctxt, P_BACK_PATTERN, true, r_pattern);
}

void MCObject::GetHilitePattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    if (GetPattern(ctxt, P_HILITE_PATTERN, false, r_pattern))
        return;
    
    r_pattern = nil;
}

void MCObject::SetHilitePattern(MCExecContext& ctxt, uinteger_t* pattern)
{
    SetPattern(ctxt, P_HILITE_PATTERN - P_FORE_PATTERN, pattern);
    Redraw();
}

void MCObject::GetEffectiveHilitePattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    GetPattern(ctxt, P_HILITE_PATTERN, true, r_pattern);
}

void MCObject::GetBorderPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    if (GetPattern(ctxt, P_BORDER_PATTERN, false, r_pattern))
        return;
    
    r_pattern = nil;
}

void MCObject::SetBorderPattern(MCExecContext& ctxt, uinteger_t* pattern)
{
    SetPattern(ctxt, P_BORDER_PATTERN - P_FORE_PATTERN, pattern);
    Redraw();
}

void MCObject::GetEffectiveBorderPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    GetPattern(ctxt, P_BORDER_PATTERN, true, r_pattern);
}

void MCObject::GetTopPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    if (GetPattern(ctxt, P_TOP_PATTERN, false, r_pattern))
        return;
    
    r_pattern = nil;
}

void MCObject::SetTopPattern(MCExecContext& ctxt, uinteger_t* pattern)
{
    SetPattern(ctxt, P_TOP_PATTERN - P_FORE_PATTERN, pattern);
    Redraw();
}

void MCObject::GetEffectiveTopPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    GetPattern(ctxt, P_TOP_PATTERN, true, r_pattern);
}

void MCObject::GetBottomPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    if (GetPattern(ctxt, P_BOTTOM_PATTERN, false, r_pattern))
        return;
    
    r_pattern = nil;
}

void MCObject::SetBottomPattern(MCExecContext& ctxt, uinteger_t* pattern)
{
    SetPattern(ctxt, P_BOTTOM_PATTERN - P_FORE_PATTERN, pattern);
    Redraw();
}

void MCObject::GetEffectiveBottomPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    GetPattern(ctxt, P_BOTTOM_PATTERN, true, r_pattern);
}

void MCObject::GetShadowPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    if (GetPattern(ctxt, P_SHADOW_PATTERN, false, r_pattern))
        return;
    
    r_pattern = nil;
}

void MCObject::SetShadowPattern(MCExecContext& ctxt, uinteger_t* pattern)
{
    SetPattern(ctxt, P_SHADOW_PATTERN - P_FORE_PATTERN, pattern);
    Redraw();
}

void MCObject::GetEffectiveShadowPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    GetPattern(ctxt, P_SHADOW_PATTERN, true, r_pattern);
}

void MCObject::GetFocusPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    if (GetPattern(ctxt, P_FOCUS_PATTERN, false, r_pattern))
        return;
    
    r_pattern = nil;
}

void MCObject::SetFocusPattern(MCExecContext& ctxt, uinteger_t* pattern)
{
    SetPattern(ctxt, P_FOCUS_PATTERN - P_FORE_PATTERN, pattern);
    Redraw();
}

void MCObject::GetEffectiveFocusPattern(MCExecContext& ctxt, uinteger_t*& r_pattern)
{
    GetPattern(ctxt, P_FOCUS_PATTERN, true, r_pattern);
}

bool MCObject::GetPatterns(MCExecContext& ctxt, bool effective, MCStringRef& r_patterns)
{
    bool t_success;
    t_success = true;
    
    MCAutoListRef t_pattern_list;
    if (t_success)
        t_success = MCListCreateMutable('\n', &t_pattern_list);
    
    if (t_success)
    {
        uinteger_t *t_id_ptr;
        for (uint2 p = P_FORE_PATTERN; p <= P_FOCUS_PATTERN; p++)
        {
            uinteger_t t_id;
            t_id_ptr = &t_id;
            
            MCAutoStringRef t_pattern;
            if (GetPattern(ctxt, (Properties)p, effective, t_id_ptr) && t_id_ptr != nil)
                t_success = MCStringFormat(&t_pattern, "%d", t_id) && MCListAppend(*t_pattern_list, *t_pattern);
            else
                t_success = MCListAppend(*t_pattern_list, kMCEmptyString);
        }
    }
    
    if (t_success)
        t_success = MCListCopyAsString(*t_pattern_list, r_patterns);
    
    return t_success;
}

void MCObject::GetPatterns(MCExecContext& ctxt, MCStringRef& r_patterns)
{
	if (GetPatterns(ctxt, false, r_patterns))
		return;

	ctxt . Throw();
}

void MCObject::SetPatterns(MCExecContext& ctxt, MCStringRef p_patterns)
{
	bool t_success;
	t_success = true;

	uindex_t t_old_offset = 0;
	uindex_t t_new_offset = 0;
	uindex_t t_length;
	t_length = MCStringGetLength(p_patterns);

	for (uint2 p = P_FORE_PATTERN; p <= P_FOCUS_PATTERN; p++)
	{
		MCAutoStringRef t_pattern;
		uint4 t_id;
		if (!MCStringFirstIndexOfChar(p_patterns, '\n', t_old_offset, kMCCompareExact, t_new_offset))
			t_new_offset = t_length;
		
		if (t_new_offset > t_old_offset)
		{
			t_success = MCStringCopySubstring(p_patterns, MCRangeMakeMinMax(t_old_offset, t_new_offset), &t_pattern);
			if (t_success)
				t_success = MCU_stoui4(*t_pattern, t_id);
			if (t_success)
				SetPattern(ctxt, (Properties)p - P_FORE_PATTERN, &t_id);
		}
		t_old_offset = t_new_offset + 1;
		if (t_old_offset > t_length)
			break;
	}

	if (t_success)
	{
		Redraw();
		return;
	}

	ctxt . Throw();
}

void MCObject::GetEffectivePatterns(MCExecContext& ctxt, MCStringRef& r_patterns)
{
	if (GetPatterns(ctxt, true, r_patterns))
		return;

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCObject::GetLockLocation(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_LOCK_LOCATION);
}

void MCObject::SetLockLocation(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_LOCK_LOCATION))
		Redraw();
}

////////////////////////////////////////////////////////////////////////////////

bool MCObject::TextPropertyMapFont()
{
	// MW-2013-03-06: [[ Bug 10698 ]] When a text property is set on a stack we
	//   must recomputefonts() due to substacks. However, substacks can be open
	//   independently of their mainstacks which causes a problem for font mapping.
	//   If we map the font after setting attributes, we won't be able to tell if
	//   the concrete font has changed. Therefore, we map here if not mapped and
	//   then unmap afterwards (only stacks need this - for all other objects
	//   child open => parent open).
	// MW-2013-03-21: [[ Bug ]] The templateStack has no parent, so probably best
	//   *not* to attempt to mapfonts on it!
	// MW-2013-03-28: [[ Bug 10791 ]] Exceptions to every rule - the home stack
	//   can be open but with no font...
	if ((opened == 0 || m_font == nil) && gettype() == CT_STACK && parent)
	{
		mapfont();
		return true;
	}
	
	return false;
}

void MCObject::TextPropertyUnmapFont(bool p_unmap)
{
	if (p_unmap)
		unmapfont();
}

void MCObject::GetTextHeight(MCExecContext& ctxt, uinteger_t*& r_height)
{
	if (fontheight != 0)
	{
		uinteger_t height;
		height = (uinteger_t)gettextheight();
		*r_height = height;
	}
	else
		r_height = nil;
}

void MCObject::SetTextHeight(MCExecContext& ctxt, uinteger_t* height)
{
	bool t_unmap_font = TextPropertyMapFont();
	
	fontheight = height == nil ? 0 : *height;
	recompute();
	
	TextPropertyUnmapFont(t_unmap_font);
	
	Redraw();
}

void MCObject::GetEffectiveTextHeight(MCExecContext& ctxt, uinteger_t& r_height)
{
	r_height = gettextheight();
}

void MCObject::GetTextAlign(MCExecContext& ctxt, intenum_t*& r_align)
{
	intenum_t align;
	align = (intenum_t)(flags & F_ALIGNMENT);
	*r_align = align;
}

void MCObject::SetTextAlign(MCExecContext& ctxt, intenum_t* align)
{
	flags &= ~F_ALIGNMENT;
	if (align == nil)
		flags |= F_ALIGN_LEFT;
	else
		flags |= *align;

	Redraw();
}

void MCObject::GetEffectiveTextAlign(MCExecContext& ctxt, intenum_t& r_align)
{
	r_align = (flags & F_ALIGNMENT);
}

void MCObject::GetTextFont(MCExecContext& ctxt, MCStringRef& r_font)
{
	if ((m_font_flags & FF_HAS_TEXTFONT) == 0)
		return;

    uint2 fontsize, fontstyle;
    MCNameRef fontname;
    getfontattsnew(fontname, fontsize, fontstyle);
    r_font = MCValueRetain(MCNameGetString(fontname));
}

void MCObject::SetTextFont(MCExecContext& ctxt, MCStringRef font)
{
	bool t_redraw = false;
	
	bool t_unmap_font = TextPropertyMapFont();

	MCNewAutoNameRef t_font_name;

	bool t_success;
	t_success = true;
	
	if (font != nil)
	{
		uindex_t t_comma;
        MCAutoStringRef t_newfont;
        if (MCStringFirstIndexOfChar(font, ',', 0, kMCCompareExact, t_comma))
        {
            t_success = MCStringCopySubstring(font, MCRangeMake(0, t_comma), &t_newfont) && MCNameCreate(*t_newfont, &t_font_name);
             
        }
        else
            t_success = MCNameCreate(font, &t_font_name);
	}

	if (t_success)
	{
		uint32_t t_font_flags;
		t_font_flags = 0;
		t_font_flags |= FF_HAS_TEXTFONT;

		setfontattrs(t_font_flags, *t_font_name, 0, 0);
		
		// MW-2012-12-14: [[ Bug ]] If this object is a stack, always recompute fonts
		//   to ensure substacks update properly.
		// MW-2013-03-21: [[ Bug ]] Unless its the templateStack (parent == nil) in which
		//   case we don't want to do any font recomputation.
		if ((gettype() == CT_STACK && parent) || opened)
		{
			if (recomputefonts(parent -> getfontref()))
			{
				recompute();
				t_redraw = true;
			}
		}
	}

	TextPropertyUnmapFont(t_unmap_font);
	
	if (t_redraw)
		Redraw();
	
	if (t_success)
		return;
	
	ctxt . Throw();
}

void MCObject::GetEffectiveTextFont(MCExecContext& ctxt, MCStringRef& r_font)
{
    uint2 fontsize, fontstyle;
    MCNameRef fontname;
    getfontattsnew(fontname, fontsize, fontstyle);
    r_font = MCValueRetain(MCNameGetString(fontname));
}

void MCObject::GetTextSize(MCExecContext& ctxt, uinteger_t*& r_size)
{
	if ((m_font_flags & FF_HAS_TEXTSIZE) == 0)
    {
        r_size = nil;
		return;
    }

	uint2 fontsize, fontstyle;
	MCNameRef fontname;
	getfontattsnew(fontname, fontsize, fontstyle);
	uinteger_t size;
	size = (uinteger_t)fontsize;
	*r_size = size;
}

void MCObject::SetTextSize(MCExecContext& ctxt, uinteger_t* size)
{
	bool t_redraw = false;
	
	bool t_unmap_font = TextPropertyMapFont();

	uint2 fontsize;
	fontsize = size != nil ? *size : 0;

	uint32_t t_font_flags;
	t_font_flags = 0;
	t_font_flags |= FF_HAS_TEXTSIZE;
	setfontattrs(t_font_flags, nil, fontsize, 0);

	fontheight = 0;

	// MW-2012-12-14: [[ Bug ]] If this object is a stack, always recompute fonts
	//   to ensure substacks update properly.
	// MW-2013-03-21: [[ Bug ]] Unless its the templateStack (parent == nil) in which
	//   case we don't want to do any font recomputation.
	if ((gettype() == CT_STACK && parent) || opened)
	{
		if (recomputefonts(parent -> getfontref()))
		{
			recompute();
			t_redraw = true;
		}
	}

	TextPropertyUnmapFont(t_unmap_font);
	
	if (t_redraw)
		Redraw();
}

void MCObject::GetEffectiveTextSize(MCExecContext& ctxt, uinteger_t& r_size)
{
    uint2 fontsize, fontstyle;
    MCNameRef fontname;
    getfontattsnew(fontname, fontsize, fontstyle);
    r_size = fontsize;
}

void MCObject::GetTextStyle(MCExecContext& ctxt, MCInterfaceTextStyle& r_style)
{
	if ((m_font_flags & FF_HAS_TEXTSTYLE) == 0)
    {
        r_style . style = 0;
		return;
    }

	uint2 fontsize, fontstyle;
	MCNameRef fontname;
	getfontattsnew(fontname, fontsize, fontstyle);
	r_style . style = fontstyle;
}

void MCObject::SetTextStyle(MCExecContext& ctxt, const MCInterfaceTextStyle& p_style)
{
	bool t_redraw = false;
	
	bool t_unmap_font = TextPropertyMapFont();

	uint2 fontstyle;
	fontstyle = p_style . style;

	uint32_t t_font_flags;
	t_font_flags = 0;
	t_font_flags |= FF_HAS_TEXTSTYLE;
	setfontattrs(t_font_flags, nil, 0, fontstyle);

	// MW-2012-12-14: [[ Bug ]] If this object is a stack, always recompute fonts
	//   to ensure substacks update properly.
	// MW-2013-03-21: [[ Bug ]] Unless its the templateStack (parent == nil) in which
	//   case we don't want to do any font recomputation.
	if ((gettype() == CT_STACK && parent) || opened)
	{
		if (recomputefonts(parent -> getfontref()))
		{
			recompute();
			t_redraw = true;
		}
	}

	TextPropertyUnmapFont(t_unmap_font);
	
	if (t_redraw)
		Redraw();
}

void MCObject::GetEffectiveTextStyle(MCExecContext& ctxt, MCInterfaceTextStyle& r_style)
{
	if ((m_font_flags & FF_HAS_TEXTSIZE) == 0)
	{
		if (parent)
			parent -> GetEffectiveTextStyle(ctxt, r_style);
		else
			MCdispatcher -> GetDefaultTextStyle(ctxt, r_style);
	}
	else
	{
		uint2 fontsize, fontstyle;
		MCNameRef fontname;
		getfontattsnew(fontname, fontsize, fontstyle);
		r_style . style = fontstyle;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCObject::GetShowBorder(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_SHOW_BORDER);
}

void MCObject::SetShowBorder(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_SHOW_BORDER))
		Redraw();
}

void MCObject::GetShowFocusBorder(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = (extraflags & EF_NO_FOCUS_BORDER) == False;
}

void MCObject::SetShowFocusBorder(MCExecContext& ctxt, bool setting)
{
	// Fetch the current transient rect of the control
	uint2 t_old_trans;
	t_old_trans = gettransient();

	if (setting)
		extraflags &= ~EF_NO_FOCUS_BORDER;
	else
		extraflags |= EF_NO_FOCUS_BORDER;

	// Redraw the control if the parent doesn't and we are open
	if (opened)
	{
		// MW-2011-08-18: [[ Layers ]] Take note of transient change and invalidate.
		if (gettype() >= CT_GROUP)
			static_cast<MCControl *>(this) -> layer_transientchangedandredrawall(t_old_trans);
	}
}

void MCObject::GetBorderWidth(MCExecContext& ctxt, uinteger_t& r_width)
{
	r_width = (uinteger_t)borderwidth;
}

void MCObject::SetBorderWidth(MCExecContext& ctxt, uinteger_t width)
{
	borderwidth = (uint1)width;
	Redraw();
}

void MCObject::GetOpaque(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_OPAQUE);
}
void MCObject::SetOpaque(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_OPAQUE))
		Redraw();
}
void MCObject::GetShadow(MCExecContext& ctxt, MCInterfaceShadow& r_shadow)
{
    r_shadow . is_flag = true;
	r_shadow . flag = getflag(F_SHADOW);
}

void MCObject::SetShadow(MCExecContext& ctxt, const MCInterfaceShadow& p_shadow)
{
	if (p_shadow . is_flag)
	{
		if (changeflag(p_shadow . flag, F_SHADOW))
			Redraw();
	}
	else
	{
		shadowoffset = (int1)p_shadow . shadow;
		flags |= F_SHADOW;
	}	
}

void MCObject::GetShadowOffset(MCExecContext& ctxt, integer_t& r_offset)
{
	r_offset = (integer_t)shadowoffset;
}

void MCObject::SetShadowOffset(MCExecContext& ctxt, integer_t offset)
{
	shadowoffset = (int1)offset;
	Redraw();
}

void MCObject::Get3D(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_3D);
}

void MCObject::Set3D(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_3D))
		Redraw();
}

////////////////////////////////////////////////////////////////////////////////

void MCObject::SetVisible(MCExecContext& ctxt, uint32_t part, bool setting)
{
	bool dirty;
	dirty = changeflag(setting, F_VISIBLE);

	// MW-2011-10-17: [[ Bug 9813 ]] Record the current effective rect of the object.
	MCRectangle t_old_effective_rect;
	if (dirty && opened && gettype() >= CT_GROUP)
		t_old_effective_rect = static_cast<MCControl *>(this) -> geteffectiverect();
	
    if (dirty)
    {
        signallisteners(P_VISIBLE);
        // AL-2015-09-23: [[ Bug 15197 ]] Hook up widget OnVisibilityChanged.
        visibilitychanged((flags & F_VISIBLE) != 0);
    }
    
    if (dirty && opened)
    {
        // MW-2011-08-18: [[ Layers ]] Take note of the change in visibility.
        if (gettype() >= CT_GROUP)
		{
            static_cast<MCControl *>(this) -> layer_visibilitychanged(t_old_effective_rect);

			// IM-2016-10-05: [[ Bug 17008 ]] Dirty selection handles when object shown / hidden
			if (getselected())
				getcard()->dirtyselection(rect);
		}
    }
    
	if (dirty)
        // AL-2015-06-30: [[ Bug 15556 ]] Use refactored function to sync mouse focus
        sync_mfocus(true, true);
}

void MCObject::GetVisible(MCExecContext& ctxt, uint32_t part, bool& r_setting)
{
	r_setting = isvisible(false);
}

void MCObject::GetEffectiveVisible(MCExecContext& ctxt, uint32_t part, bool& r_setting)
{
	r_setting = isvisible(true);
}

void MCObject::GetInvisible(MCExecContext& ctxt, uint32_t part, bool& r_setting)
{
	r_setting = !isvisible(false);
}

void MCObject::SetInvisible(MCExecContext& ctxt, uint32_t part, bool setting)
{
	SetVisible(ctxt, part, !setting);
}

void MCObject::GetEffectiveInvisible(MCExecContext& ctxt, uint32_t part, bool& r_setting)
{
    r_setting = !isvisible(true);
}

void MCObject::GetEnabled(MCExecContext& ctxt, uint32_t part, bool& r_setting)
{
	r_setting = (flags & F_DISABLED) == False;
}

void MCObject::SetEnabled(MCExecContext& ctxt, uint32_t part, bool setting)
{
    SetDisabled(ctxt, part, !setting);
}

void MCObject::GetDisabled(MCExecContext& ctxt, uint32_t part, bool& r_setting)
{
	r_setting = getflag(F_DISABLED);
}

void MCObject::SetDisabled(MCExecContext& ctxt, uint32_t part, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, F_DISABLED);
		
	if (flags & F_DISABLED && state & CS_KFOCUSED)
		getcard(part)->kunfocus();

	if (t_dirty)
		Redraw();
}

void MCObject::GetSelected(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getstate(CS_SELECTED);
}

void MCObject::SetSelected(MCExecContext& ctxt, bool setting)
{
	if (setting != ((state & CS_SELECTED) != 0))
	{
		if (setting)
			MCselected->add(this);
		else
			MCselected->remove(this);
	}
}

void MCObject::GetTraversalOn(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_TRAVERSAL_ON);
}

void MCObject::SetTraversalOn(MCExecContext& ctxt, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, F_TRAVERSAL_ON);
		
	if (state & CS_KFOCUSED && !(flags & F_TRAVERSAL_ON))
		state &= ~CS_KFOCUSED;

	if (t_dirty)
		Redraw();
}

////////////////////////////////////////////////////////////////////////////////

void MCObject::GetOwner(MCExecContext& ctxt, MCStringRef& r_owner)
{
	if (parent)
		parent -> GetName(ctxt, r_owner);
}

void MCObject::GetShortOwner(MCExecContext& ctxt, MCStringRef& r_owner)
{
	if (parent)
		parent -> GetShortName(ctxt, r_owner);
}

void MCObject::GetAbbrevOwner(MCExecContext& ctxt, MCStringRef& r_owner)
{
	if (parent)
		parent -> GetAbbrevName(ctxt, r_owner);
}

void MCObject::GetLongOwner(MCExecContext& ctxt, uint32_t p_part_id, MCStringRef& r_owner)
{
	if (parent)
        parent -> GetLongName(ctxt, p_part_id, r_owner);
}

void MCObject::DoGetProperties(MCExecContext& ctxt, uint32_t part, bool p_effective, MCArrayRef& r_props)
{
	const PropList *table;
	uint2 tablesize;

	switch (gettype())
	{
	case CT_STACK:
		table = stackprops;
		tablesize = ELEMENTS(stackprops);
		break;
	case CT_CARD:
		table = cardprops;
		tablesize = ELEMENTS(cardprops);
		break;
	case CT_GROUP:
		table = groupprops;
		tablesize = ELEMENTS(groupprops);
		break;
	case CT_BUTTON:
		table = buttonprops;
		tablesize = ELEMENTS(buttonprops);
		break;
	case CT_FIELD:
		table = fieldprops;
		tablesize = ELEMENTS(fieldprops);
		break;
	case CT_IMAGE:
		table = imageprops;
		tablesize = ELEMENTS(imageprops);
		break;
	case CT_GRAPHIC:
		table = graphicprops;
		tablesize = ELEMENTS(graphicprops);
		break;
	case CT_SCROLLBAR:
		table = scrollbarprops;
		tablesize = ELEMENTS(scrollbarprops);
		break;
	case CT_PLAYER:
		table = playerprops;
		tablesize = ELEMENTS(playerprops);
		break;
	case CT_EPS:
		table = epsprops;
		tablesize = ELEMENTS(epsprops);
		break;
	case CT_COLOR_PALETTE:
		table = colorpaletteprops;
		tablesize = ELEMENTS(colorpaletteprops);
		break;
	case CT_AUDIO_CLIP:
		table = audioclipprops;
		tablesize = ELEMENTS(audioclipprops);
		break;
	case CT_VIDEO_CLIP:
		table = videoclipprops;
		tablesize = ELEMENTS(videoclipprops);
		break;
    case CT_WIDGET:
		table = NULL;
		tablesize = 0;
        // WIDGET-TODO: Implement properties
        break;
	default:
		return;
	}
	bool t_success;
	t_success = true;

	MCAutoArrayRef t_array;
	if (t_success)
		t_success = MCArrayCreateMutable(&t_array);
	if (t_success)
    {
		MCerrorlock++;
		while (tablesize--)
        {
            MCAutoValueRef t_value;
            const char* t_token = table[tablesize].token;

            // MERG-2013-06-24: [[ RevisedPropsProp ]] Treat the short name specially to ensure
            //   round-tripping. If the name is empty, then return empty for 'name'.
            if ((Properties)table[tablesize].value == P_SHORT_NAME)
            {
                if (isunnamed())
                    t_value = MCValueRetain(kMCEmptyString);
                else
                    getstringprop(ctxt, part, P_SHORT_NAME, p_effective, (MCStringRef&)&t_value);
            }
            else
                getvariantprop(ctxt, part, (Properties)table[tablesize].value, p_effective, &t_value);

            if (!ctxt . HasError())
                MCArrayStoreValue(*t_array, false, MCNAME(t_token), *t_value);
		}
		MCerrorlock--;
		t_success = MCArrayCopy(*t_array, r_props);
	}
	
	if (t_success)
		return;

	ctxt . Throw();
}

void MCObject::GetProperties(MCExecContext& ctxt, uint32_t part, MCArrayRef& r_props)
{
    DoGetProperties(ctxt, part, false, r_props);
}

// MERG-2013-05-07: [[ RevisedPropsProp ]] Array of object props that must be set first
//   to ensure other properties don't set them differently.
static struct { Properties prop; const char *tag; } s_preprocess_props[] =
{
    // MERG-2013-08-30: [[ RevisedPropsProp ]] Ensure lockLocation of groups is set before rectangle
    { P_LOCK_LOCATION, "lockLocation" },
    { P_LOCK_LOCATION, "lockLoc" },
    { P_RECTANGLE, "rectangle" },// gradients will be wrong if this isn't set first
    { P_RECTANGLE, "rect" },     // synonym
    { P_WIDTH, "width" },        // incase left,right are in the array
    { P_HEIGHT, "height" },      // incase top,bottom are in the array
    { P_STYLE, "style" },        // changes numerous properties including text alignment
    { P_TEXT_SIZE, "textSize" }, // changes textHeight
	// MERG-2013-06-24: [[ RevisedPropsProp ]] Ensure filename takes precedence over text.
    { P_FILE_NAME, "fileName" }, // setting image filenames to empty after setting the text will clear them
    // MERG-2013-07-20: [[ Bug 11060 ]] hilitedLines being lost.
    { P_LIST_BEHAVIOR, "listBehavior" }, // setting hilitedLines before listBehavior will lose the hilited lines
    { P_HTML_TEXT, "htmlText" }, // setting hilitedLines before htmlText will lose the hilited lines
    // MERG-2013-08-30: [[ RevisedPropsProp ]] Ensure button text has precedence over label and menuHistory
    { P_TEXT, "text" },
    { P_MENU_HISTORY, "menuHistory" },
    { P_FORE_PATTERN, "forePattern" },
    { P_FORE_PATTERN, "foregroundPattern" },
    { P_FORE_PATTERN, "textPattern" },
    { P_FORE_PATTERN, "thumbPattern" },
    { P_BACK_PATTERN, "backPattern" },
    { P_BACK_PATTERN, "backgroundPattern" },
    { P_BACK_PATTERN, "fillPat" },
    { P_HILITE_PATTERN, "hilitePattern" },
    { P_HILITE_PATTERN, "markerPattern" },
    { P_HILITE_PATTERN, "thirdPattern" },
    { P_BORDER_PATTERN, "borderPattern" },
    { P_TOP_PATTERN, "topPattern" },
    { P_BOTTOM_PATTERN, "bottomPattern" },
    { P_SHADOW_PATTERN, "shadowPattern" },
    { P_FOCUS_PATTERN, "focusPattern" },
    { P_PATTERNS, "patterns" },
};

void MCObject::SetProperties(MCExecContext& ctxt, uint32_t part, MCArrayRef p_props)
{
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	MCRedrawLockScreen();
	MCerrorlock++;
    
	MCValueRef t_value;
    MCExecValue t_exec_value;
    uindex_t j;
    // MERG-2013-05-07: [[ RevisedPropsProp ]] pre-process to ensure properties
	//   that impact others are set first.
    uindex_t t_preprocess_size = sizeof(s_preprocess_props) / sizeof(s_preprocess_props[0]);
    for (j=0; j<t_preprocess_size; j++)
    {
		// MERG-2013-06-24: [[ RevisedPropsProp ]] Make sure we do a case-insensitive search
		//   for the property name.
        if (!MCArrayFetchValue(p_props, false, MCNAME(s_preprocess_props[j].tag), t_value))
            continue;

        // MW-2013-06-24: [[ RevisedPropsProp ]] Workaround Bug 10977 - only set the
        //   'filename' of an image if it is non-empty or the image has a filename.
        if (s_preprocess_props[j].prop == P_FILE_NAME && gettype() == CT_IMAGE &&
            MCValueIsEmpty(t_value) && !getflag(F_HAS_FILENAME))
            continue;
        
        t_exec_value . valueref_value = MCValueRetain(t_value);
        t_exec_value . type = kMCExecValueTypeValueRef;
        setprop(ctxt, part, (Properties)s_preprocess_props[j].prop, nil, False, t_exec_value);
        
        ctxt . IgnoreLastError();
    }
    
	uintptr_t t_iterator;
	t_iterator = 0;
    MCNameRef t_key;
	while(MCArrayIterate(p_props, t_iterator, t_key, t_value))
	{
		MCScriptPoint sp(MCNameGetString(t_key));
		Symbol_type type;
		const LT *te;
		if (sp.next(type) && sp.lookup(SP_FACTOR, te) == PS_NORMAL
		        && te->type == TT_PROPERTY && te->which != P_ID)
		{
            // MERG-2013-05-07: [[ RevisedPropsProp ]] check if the key was
            //   in the pre-processed.
            // MW-2013-06-24: [[ RevisedPropsProp ]] set a boolean if the prop has already been
            //   set.
            bool t_been_preprocessed;
            t_been_preprocessed = false;
            for (j=0; j<t_preprocess_size; j++)
                if (te->which == s_preprocess_props[j].prop)
                {
                    t_been_preprocessed = true;
                    break;
                }
            
            // MW-2013-06-24: [[ RevisedPropsProp ]] Only attempt to set the prop if it hasn't
            //   already been processed.
            if (t_been_preprocessed)
                continue;
    
            // MW-2013-06-24: [[ RevisedPropsProp ]] Workaround Bug 10977 - only set the
            //   'filename' of an image if it is non-empty or the image has a filename.
            if (te->which == P_FILE_NAME && gettype() == CT_IMAGE &&
                MCValueIsEmpty(t_value) && !getflag(F_HAS_FILENAME))
                continue;
            
            t_exec_value . valueref_value = MCValueRetain(t_value);
            t_exec_value . type = kMCExecValueTypeValueRef;
            setprop(ctxt, part, (Properties)te->which, nil, False, t_exec_value);
            
            ctxt . IgnoreLastError();
		}
	}
	MCerrorlock--;
	MCRedrawUnlockScreen();
}

void MCObject::GetEffectiveProperties(MCExecContext& ctxt, uint32_t part, MCArrayRef& r_props)
{
    DoGetProperties(ctxt, part, true, r_props);
}

void MCObject::GetCustomPropertySet(MCExecContext& ctxt, MCStringRef& r_propset)
{
	r_propset = (MCStringRef)MCValueRetain(MCNameGetString(getdefaultpropsetname()));
}

void MCObject::SetCustomPropertySet(MCExecContext& ctxt, MCStringRef propset)
{
	MCNewAutoNameRef t_propset_name;
	if (MCNameCreate(propset, &t_propset_name))
	{
		if (props == nil)
			if (!MCObjectPropertySet::createwithname(kMCEmptyName, props))
				return;

		if (props -> hasname(*t_propset_name))
			return;

		MCObjectPropertySet *t_set;
		t_set = props;
		while(t_set -> getnext() != nil && !t_set -> getnext() -> hasname(*t_propset_name))
			t_set = t_set -> getnext();

		if (t_set -> getnext() == nil)
		{
			if (!MCObjectPropertySet::createwithname(*t_propset_name, t_set))
				return;

			t_set -> setnext(props);
			props = t_set;
		}
		else
		{
			MCObjectPropertySet *t_next_set;
			t_next_set = t_set -> getnext();
			t_set -> setnext(t_next_set -> getnext());
			t_next_set -> setnext(props);
			props = t_next_set;
		}
		return;
	}

	ctxt . Throw();
}

void MCObject::GetCustomPropertySets(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_propsets)
{
	bool t_success;
	t_success = true;

	MCAutoListRef t_propsets;
	MCObjectPropertySet *p = props;
	
	MCAutoArray<MCStringRef> t_list;

	while (t_success && p != NULL)
	{
		if (!p -> hasname(kMCEmptyName))
			t_success = t_list . Push(MCValueRetain(MCNameGetString(p -> getname())));
		p = p -> getnext();
	}

	if (t_success)
	{
		t_list . Take(r_propsets, r_count);
		return;
	}
	
	ctxt . Throw();
}

void MCObject::SetCustomPropertySets(MCExecContext& ctxt, uindex_t p_count, MCStringRef* p_propsets)
{
	MCObjectPropertySet *newprops = nil;
	MCObjectPropertySet *newp = nil;
	bool t_success;
	t_success = true;

	for (uindex_t i = 0; i < p_count && t_success; i++)
	{		
		MCNewAutoNameRef t_name;
		t_success = MCNameCreate(p_propsets[i], &t_name);
		
		if (t_success)
		{
			MCObjectPropertySet *lp = nil;
			MCObjectPropertySet *p = props;
			while (p != nil && !p->hasname(*t_name))
			{
				lp = p;
				p = p->getnext();
			}
			if (p == nil)
				/* UNCHECKED */ MCObjectPropertySet::createwithname(*t_name, p);
			else
			{
				if (p == props)
					props = props->getnext();
				else
					lp->setnext(p->getnext());
				p->setnext(NULL);
			}
			if (newprops == NULL)
				newprops = p;
			else
				newp->setnext(p);
			newp = p;
		}
	}

	bool gotdefault = false;
	while (props != nil)
	{
		MCObjectPropertySet *sp = props->getnext();
		if (props->hasname(kMCEmptyName))
		{
			props->setnext(newprops);
			newprops = props;
			gotdefault = true;
		}
		else
			delete props;
		props = sp;
	}
	if (!gotdefault && newprops != NULL)
	{
		/* UNCHECKED */ MCObjectPropertySet::createwithname(kMCEmptyName, props);
		props->setnext(newprops);
	}
	else
		props = newprops;
}

////////////////////////////////////////////////////////////////////////////////

void MCObject::GetInk(MCExecContext& ctxt, intenum_t& r_ink)
{
	r_ink = (intenum_t)ink;
}

void MCObject::SetInk(MCExecContext& ctxt, intenum_t newink)
{
	if (ink != (uint1)newink)
	{
		ink = (uint1)newink;
		Redraw();
	}
}

void MCObject::GetCantSelect(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = !(isselectable(true));
}

void MCObject::SetCantSelect(MCExecContext& ctxt, bool setting)
{
	if (setting)
		extraflags |= EF_CANT_SELECT;
	else
		extraflags &= ~EF_CANT_SELECT;
	Redraw();
}

void MCObject::GetEffectiveCantSelect(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = !(isselectable(false));
}

void MCObject::GetBlendLevel(MCExecContext& ctxt, uinteger_t& r_level)
{
	r_level = (uinteger_t)(100 - blendlevel);
}

void MCObject::SetBlendLevel(MCExecContext& ctxt, uinteger_t level)
{
	level = level > 100 ? 0 : 100 - level;
	if ((uint1)level != blendlevel)
	{
		blendlevel = (uint1)level;

		// MW-2012-04-11: [[ Bug ]] Special case for when a dynamic layer has its
		//   blend level changed - all we need do is invalidate the card.
		if (gettype() < CT_GROUP || !static_cast<MCControl *>(this) -> layer_issprite())
			Redraw();
		else
			parent.GetAs<MCCard>()->layer_dirtyrect(static_cast<MCControl *>(this) -> geteffectiverect());
	}
}

void MCObject::SetRectProp(MCExecContext& ctxt, bool p_effective, MCRectangle p_rect)
{
	MCRectangle t_rect;
	t_rect = p_rect;
	// MW-2012-10-26: Adjust the rectangle appropriately based on any effective margins.
	if (p_effective)
	{
		MCRectangle t_outer_rect;
		t_outer_rect = getrectangle(true);

		MCRectangle t_inner_rect;
		t_inner_rect = getrectangle(false);

		t_rect . x += t_inner_rect . x - t_outer_rect . x;
		t_rect . y += t_inner_rect . y - t_outer_rect . y;
		t_rect . width -= (t_inner_rect . x - t_outer_rect . x) + (t_outer_rect . x + t_outer_rect . width - (t_inner_rect . x + t_inner_rect . width));
		t_rect . height -= (t_inner_rect . y - t_outer_rect . y) + (t_outer_rect . y + t_outer_rect . height - (t_inner_rect . y + t_inner_rect . height));
	}

	if (!MCU_equal_rect(t_rect, rect))
	{
		bool t_sync_mouse = false;
		if (MCU_point_in_rect(rect, MCmousex, MCmousey))
		{
			if (!MCU_point_in_rect(t_rect, MCmousex, MCmousey))
				t_sync_mouse = true;
		}
		else
		{
			if (MCU_point_in_rect(t_rect, MCmousex, MCmousey))
				t_sync_mouse = true;
		}

		if (gettype() >= CT_GROUP)
		{
			// MW-2011-08-18: [[ Layers ]] Notify of change of rect.
			static_cast<MCControl *>(this) -> layer_setrect(t_rect, false);
			// Notify the parent of the resize.
			resizeparent();
		}
		else
			setrect(t_rect);

		if (t_sync_mouse)
			sync_mfocus(false, false);
	}
}

void MCObject::GetRectPoint(MCExecContext& ctxt, bool effective, Properties which, MCPoint &r_point)
{
	MCRectangle t_rect;
	t_rect = getrectangle(effective);

	switch (which)
	{
	case P_LOCATION:
		r_point . x = t_rect . x + (t_rect . width >> 1);
		r_point . y = t_rect . y + (t_rect . height >> 1);
		break;
	case P_TOP_LEFT:
		r_point . x = t_rect . x;
		r_point . y = t_rect . y;
		break;
	case P_TOP_RIGHT:
		r_point . x = t_rect . x + t_rect . width;
		r_point . y = t_rect . y;
		break;
	case P_BOTTOM_LEFT:
		r_point . x = t_rect . x;
		r_point . y = t_rect . y + t_rect . height;
		break;
	case P_BOTTOM_RIGHT:
		r_point . x = t_rect . x + t_rect . width;
		r_point . y = t_rect . y + t_rect . height;
		break;
	default:
		break;
	}
}

void MCObject::SetRectPoint(MCExecContext& ctxt, bool effective, Properties which, MCPoint point)
{
	MCRectangle t_rect;
	t_rect = getrectangle(effective);

	switch (which)
	{
	case P_LOCATION:
		point . x -= t_rect . width >> 1;
		point . y -= t_rect . height >> 1;
		break;
	case P_BOTTOM_LEFT:
		point . y -= t_rect . height;
		break;
	case P_BOTTOM_RIGHT:
		point . x -= t_rect . width;
		point . y -= t_rect . height;
		break;
	case P_TOP_LEFT:
		break;
	case P_TOP_RIGHT:
		point . x -= t_rect . width;
		break;
	default:
		MCUnreachable();
		break;
	}

	t_rect . x = point . x;
	t_rect . y = point . y;

	SetRectProp(ctxt, effective, t_rect);
}

void MCObject::GetRectValue(MCExecContext& ctxt, bool effective, Properties which, integer_t& r_value)
{
	MCRectangle t_rect;
	t_rect = getrectangle(effective);

	switch (which)
	{
	case P_LEFT:
		r_value = t_rect . x;
		break;
	case P_TOP:
		r_value = t_rect . y;
		break;
	case P_RIGHT:
		r_value = t_rect . x + t_rect . width;
		break;
	case P_BOTTOM:
		r_value = t_rect . y + t_rect . height;
		break;
	case P_WIDTH:
		r_value = t_rect . width;
		break;
	case P_HEIGHT:
		r_value = t_rect . height;
		break;
	default:
		break;
	}
}

void MCObject::SetRectValue(MCExecContext& ctxt, bool effective, Properties which, integer_t value)
{
	MCRectangle t_rect;
	t_rect = getrectangle(effective);

	switch (which)
	{
	case P_LEFT:
		t_rect . x = value;
		break;
	case P_RIGHT:
		t_rect . x = value - t_rect . width;
		break;
	case P_TOP:
		t_rect . y = value;
		break;
	case P_BOTTOM:
		t_rect . y = value - t_rect . height;
		break;
	case P_WIDTH:
		if (!getflag(F_LOCK_LOCATION))
			t_rect . x += (t_rect . width - value) >> 1;
		t_rect . width = MCU_max(value, 1);
		break;
	case P_HEIGHT:
		if (!getflag(F_LOCK_LOCATION))
			t_rect . y += (t_rect . height - value) >> 1;
		t_rect . height = MCU_max(value, 1);
		break;
	default:
		MCUnreachable();
		break;
	}

	SetRectProp(ctxt, effective, t_rect);
}

void MCObject::GetLocation(MCExecContext& ctxt, MCPoint& r_location)
{
	GetRectPoint(ctxt, false, P_LOCATION, r_location);
}

void MCObject::SetLocation(MCExecContext& ctxt, MCPoint location)
{
	SetRectPoint(ctxt, false, P_LOCATION, location);
}

void MCObject::GetEffectiveLocation(MCExecContext& ctxt, MCPoint& r_location)
{
	GetRectPoint(ctxt, true, P_LOCATION, r_location);
}

void MCObject::SetEffectiveLocation(MCExecContext& ctxt, MCPoint location)
{
	SetRectPoint(ctxt, true, P_LOCATION, location);
}

void MCObject::GetLeft(MCExecContext& ctxt, integer_t& r_value)
{
	GetRectValue(ctxt, false, P_LEFT, r_value);
}

void MCObject::SetLeft(MCExecContext& ctxt, integer_t value)
{
	SetRectValue(ctxt, false, P_LEFT, value);
}

void MCObject::GetEffectiveLeft(MCExecContext& ctxt, integer_t& r_value)
{
	GetRectValue(ctxt, true, P_LEFT, r_value);
}

void MCObject::SetEffectiveLeft(MCExecContext& ctxt, integer_t value)
{
	SetRectValue(ctxt, true, P_LEFT, value);
}

void MCObject::GetTop(MCExecContext& ctxt, integer_t& r_value)
{
	GetRectValue(ctxt, false, P_TOP, r_value);
}
void MCObject::SetTop(MCExecContext& ctxt, integer_t value)
{
	SetRectValue(ctxt, false, P_TOP, value);
}

void MCObject::GetEffectiveTop(MCExecContext& ctxt, integer_t& r_value)
{
	GetRectValue(ctxt, true, P_TOP, r_value);
}

void MCObject::SetEffectiveTop(MCExecContext& ctxt, integer_t value)
{
	SetRectValue(ctxt, true, P_TOP, value);
}

void MCObject::GetRight(MCExecContext& ctxt, integer_t& r_value)
{
	GetRectValue(ctxt, false, P_RIGHT, r_value);
}
void MCObject::SetRight(MCExecContext& ctxt, integer_t value)
{
	SetRectValue(ctxt, false, P_RIGHT, value);
}

void MCObject::GetEffectiveRight(MCExecContext& ctxt, integer_t& r_value)
{
	GetRectValue(ctxt, true, P_RIGHT, r_value);
}

void MCObject::SetEffectiveRight(MCExecContext& ctxt, integer_t value)
{
	SetRectValue(ctxt, true, P_RIGHT, value);
}

void MCObject::GetBottom(MCExecContext& ctxt, integer_t& r_value)
{
	GetRectValue(ctxt, false, P_BOTTOM, r_value);
}
void MCObject::SetBottom(MCExecContext& ctxt, integer_t value)
{
	SetRectValue(ctxt, false, P_BOTTOM, value);
}

void MCObject::GetEffectiveBottom(MCExecContext& ctxt, integer_t& r_value)
{
	GetRectValue(ctxt, true, P_BOTTOM, r_value);
}

void MCObject::SetEffectiveBottom(MCExecContext& ctxt, integer_t value)
{
	SetRectValue(ctxt, true, P_BOTTOM, value);
}

void MCObject::GetWidth(MCExecContext& ctxt, uinteger_t& r_value)
{
	integer_t t_value;
	GetRectValue(ctxt, false, P_WIDTH, t_value);
	r_value = (uinteger_t)t_value;
}
void MCObject::SetWidth(MCExecContext& ctxt, uinteger_t value)
{
	SetRectValue(ctxt, false, P_WIDTH, value);
}

void MCObject::GetEffectiveWidth(MCExecContext& ctxt, uinteger_t& r_value)
{
	integer_t t_value;
	GetRectValue(ctxt, true, P_WIDTH, t_value);
	r_value = (uinteger_t)t_value;
}

void MCObject::SetEffectiveWidth(MCExecContext& ctxt, uinteger_t value)
{
	SetRectValue(ctxt, true, P_WIDTH, value);
}

void MCObject::GetHeight(MCExecContext& ctxt, uinteger_t& r_value)
{
	integer_t t_value;
	GetRectValue(ctxt, false, P_HEIGHT, t_value);
	r_value = (uinteger_t)t_value;
}
void MCObject::SetHeight(MCExecContext& ctxt, uinteger_t value)
{
	SetRectValue(ctxt, false, P_HEIGHT, value);
}

void MCObject::GetEffectiveHeight(MCExecContext& ctxt, uinteger_t& r_value)
{
	integer_t t_value;
	GetRectValue(ctxt, true, P_HEIGHT, t_value);
	r_value = (uinteger_t)t_value;
}

void MCObject::SetEffectiveHeight(MCExecContext& ctxt, uinteger_t value)
{
	SetRectValue(ctxt, true, P_HEIGHT, value);
}

void MCObject::GetTopLeft(MCExecContext& ctxt, MCPoint& r_location)
{
	GetRectPoint(ctxt, false, P_TOP_LEFT, r_location);
}

void MCObject::SetTopLeft(MCExecContext& ctxt, MCPoint location)
{
	SetRectPoint(ctxt, false, P_TOP_LEFT, location);
}

void MCObject::GetEffectiveTopLeft(MCExecContext& ctxt, MCPoint& r_location)
{
	GetRectPoint(ctxt, true, P_TOP_LEFT, r_location);
}

void MCObject::SetEffectiveTopLeft(MCExecContext& ctxt, MCPoint location)
{
	SetRectPoint(ctxt, true, P_TOP_LEFT, location);
}

void MCObject::GetTopRight(MCExecContext& ctxt, MCPoint& r_location)
{
	GetRectPoint(ctxt, false, P_TOP_RIGHT, r_location);
}

void MCObject::SetTopRight(MCExecContext& ctxt, MCPoint location)
{
	SetRectPoint(ctxt, false, P_TOP_RIGHT, location);
}

void MCObject::GetEffectiveTopRight(MCExecContext& ctxt, MCPoint& r_location)
{
	GetRectPoint(ctxt, true, P_TOP_RIGHT, r_location);
}

void MCObject::SetEffectiveTopRight(MCExecContext& ctxt, MCPoint location)
{
	SetRectPoint(ctxt, true, P_TOP_RIGHT, location);
}

void MCObject::GetBottomLeft(MCExecContext& ctxt, MCPoint& r_location)
{
	GetRectPoint(ctxt, false, P_BOTTOM_LEFT, r_location);
}

void MCObject::SetBottomLeft(MCExecContext& ctxt, MCPoint location)
{
	SetRectPoint(ctxt, false, P_BOTTOM_LEFT, location);
}

void MCObject::GetEffectiveBottomLeft(MCExecContext& ctxt, MCPoint& r_location)
{
	GetRectPoint(ctxt, true, P_BOTTOM_LEFT, r_location);
}

void MCObject::SetEffectiveBottomLeft(MCExecContext& ctxt, MCPoint location)
{
	SetRectPoint(ctxt, true, P_BOTTOM_LEFT, location);
}

void MCObject::GetBottomRight(MCExecContext& ctxt, MCPoint& r_location)
{
	GetRectPoint(ctxt, false, P_BOTTOM_RIGHT, r_location);
}

void MCObject::SetBottomRight(MCExecContext& ctxt, MCPoint location)
{
	SetRectPoint(ctxt, false, P_BOTTOM_RIGHT, location);
}

void MCObject::GetEffectiveBottomRight(MCExecContext& ctxt, MCPoint& r_location)
{
	GetRectPoint(ctxt, true, P_BOTTOM_RIGHT, r_location);
}

void MCObject::SetEffectiveBottomRight(MCExecContext& ctxt, MCPoint location)
{
	SetRectPoint(ctxt, true, P_BOTTOM_RIGHT, location);
}

void MCObject::GetRectangle(MCExecContext& ctxt, MCRectangle& r_rect)
{
	r_rect = getrectangle(false);
}

void MCObject::SetRectangle(MCExecContext& ctxt, MCRectangle p_rect)
{
	SetRectProp(ctxt, false, p_rect);
}

void MCObject::GetEffectiveRectangle(MCExecContext& ctxt, MCRectangle& r_rect)
{
	r_rect = getrectangle(true);
}

void MCObject::SetEffectiveRectangle(MCExecContext& ctxt, MCRectangle p_rect)
{
	SetRectProp(ctxt, true, p_rect);
}

////////////////////////////////////////////////////////////////////////////////

void MCObject::GetEncoding(MCExecContext& ctxt, intenum_t& r_encoding)
{
	r_encoding = hasunicode() ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////

void MCObject::GetTextStyleElement(MCExecContext& ctxt, MCNameRef p_index, bool& r_setting)
{
    uint2 t_style_set;
    if ((m_font_flags & FF_HAS_TEXTSTYLE) == 0)
        t_style_set = FA_DEFAULT_STYLE;
    else
        t_style_set = gettextstyle();
    
    Font_textstyle t_style;
    if (MCF_parsetextstyle(MCNameGetString(p_index), t_style) == ES_NORMAL)
    {
        r_setting = MCF_istextstyleset(t_style_set, t_style);
        return;
    }
    
    ctxt . Throw();
}

void MCObject::SetTextStyleElement(MCExecContext& ctxt, MCNameRef p_index, bool p_setting)
{
    Font_textstyle t_style;
    if (MCF_parsetextstyle(MCNameGetString(p_index), t_style) == ES_NORMAL)
    {
        uint2 t_style_set;
		if ((m_font_flags & FF_HAS_TEXTSTYLE) == 0)
			t_style_set = FA_DEFAULT_STYLE;
		else
			t_style_set = gettextstyle();
        
        MCF_changetextstyle(t_style_set, t_style, p_setting);
        
        MCInterfaceTextStyle t_interface_style;
        t_interface_style . style = t_style_set;
        SetTextStyle(ctxt, t_interface_style);
        return;
    }
    
    ctxt . Throw();
}

void MCObject::GetCustomKeysElement(MCExecContext& ctxt, MCNameRef p_index, MCStringRef& r_string)
{
    MCObjectPropertySet *t_propset;
    if (findpropset(p_index, true, t_propset))
    {
        if (t_propset -> list(r_string))
            return;
    }
    else
    {
        r_string = MCValueRetain(kMCEmptyString);
        return;
    }
    
    ctxt . Throw();
}

void MCObject::GetCustomProperties(MCExecContext& ctxt, MCValueRef& r_array)
{
    GetCustomPropertiesElement(ctxt, kMCEmptyName, r_array);
}

void MCObject::GetCustomPropertiesElement(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_array)
{
    MCObjectPropertySet *t_propset;
    if (findpropset(p_index, true, t_propset))
    {
        MCAutoArrayRef t_array;
        if (t_propset -> fetch(&t_array))
        {
            r_array = MCValueRetain(*t_array);
            return;
        }
    }
    else
    {
        r_array = MCValueRetain(kMCEmptyString);
        return;
    }
    
    ctxt . Throw();
}

void MCObject::SetCustomKeysElement(MCExecContext& ctxt, MCNameRef p_index, MCStringRef p_string)
{
    MCObjectPropertySet *t_propset;
    if (MCStringIsEmpty(p_string))
    {
        if (findpropset(p_index, true, t_propset))
            t_propset -> clear();
        return;
    }

    /* UNCHECKED */ ensurepropset(p_index, true, t_propset);
    /* UNCHECKED */ t_propset -> restrict(p_string);
}

void MCObject::SetCustomProperties(MCExecContext& ctxt, MCValueRef p_array)
{
    SetCustomPropertiesElement(ctxt, kMCEmptyName, p_array);
}

void MCObject::SetCustomPropertiesElement(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_array)
{
    MCObjectPropertySet *t_propset;
    if  (!MCValueIsArray(p_array))
    {
        if (findpropset(p_index, true, t_propset))
            t_propset -> clear();
        return;
    }
    
    /* UNCHECKED */ ensurepropset(p_index, true, t_propset);
    t_propset -> store((MCArrayRef)p_array);
}

void MCObject::GetCustomKeys(MCExecContext& ctxt, MCStringRef& r_string)
{
    GetCustomKeysElement(ctxt, kMCEmptyName, r_string);
}

void MCObject::SetCustomKeys(MCExecContext& ctxt, MCStringRef p_string)
{
    SetCustomKeysElement(ctxt, kMCEmptyName, p_string);
}

void MCObject::GetCardIds(MCExecContext& ctxt, MCCard *p_cards, bool p_all, uint32_t p_id, uindex_t& r_count, uinteger_t*& r_ids)
{
	MCAutoArray<uinteger_t> t_ids;
    bool t_success;
    
    t_success = true;
    if (p_cards != nil)
	{
		MCCard *cptr = p_cards;
		do
		{
            if (p_all || cptr -> countme(p_id, False))
            {
                uint32_t t_id;
                t_id = cptr -> getid();
                t_success = t_ids . Push(t_id);
            }
            cptr = cptr -> next();
		}
		while (cptr != p_cards && t_success);
	}
    
    t_ids . Take(r_ids, r_count);
}

void MCObject::GetCardNames(MCExecContext& ctxt, MCCard *p_cards, bool p_all, uint32_t p_id, uindex_t& r_count, MCStringRef*& r_names)
{
	MCAutoArray<MCStringRef> t_names;
    bool t_success;
    
    t_success = true;
    if (p_cards != nil)
	{
		MCCard *cptr = p_cards;
		do
		{
            if (p_all || cptr -> countme(p_id, False))
            {
                MCStringRef t_name;
                cptr -> GetShortName(ctxt, t_name);
                t_success = !ctxt . HasError();
                if (t_success)
                    t_success = t_names . Push(t_name);
            }
            cptr = cptr -> next();
		}
		while (cptr != p_cards && t_success);
	}
    
    t_names . Take(r_names, r_count);
}

void MCObject::GetTheme(MCExecContext& ctxt, intenum_t& r_theme)
{
    r_theme = m_theme;
}

void MCObject::SetTheme(MCExecContext& ctxt, intenum_t p_theme)
{
    m_theme = MCInterfaceTheme(p_theme);
    
    // Changing the theme probably changed the font
    if (recomputefonts(parent ? parent->m_font : nil, true))
        recompute();
    
    Redraw();
}

void MCObject::GetEffectiveTheme(MCExecContext& ctxt, intenum_t& r_theme)
{
    r_theme = gettheme();
}

void MCObject::GetThemeControlType(MCExecContext& ctxt, intenum_t& r_theme)
{
    r_theme = m_theme_type;
}

void MCObject::SetThemeControlType(MCExecContext& ctxt, intenum_t p_theme)
{
    m_theme_type = MCPlatformControlType(p_theme);
    
    // Changing the theming type probably changed the font
    if (recomputefonts(parent ? parent->m_font : nil, true))
        recompute();
    
    Redraw();
}

void MCObject::GetEffectiveThemeControlType(MCExecContext& ctxt, intenum_t& r_theme)
{
    r_theme = getcontroltype();
}

void MCObject::GetScriptStatus(MCExecContext& ctxt, intenum_t& r_status)
{
    if (hashandlers & HH_DEAD_SCRIPT)
        r_status = kMCInterfaceScriptStatusError;
    else if (hlist == nil && flags & F_SCRIPT)
        r_status = kMCInterfaceScriptStatusUncompiled;
    else
        r_status = kMCInterfaceScriptStatusCompiled;
}

///////////////////////////////////////////////////////////////////////////////
//
//  REFLECTIVE PROPERTIES - MOVED FROM DEVELOPMENT ENGINE 8.1.5-rc-1 ONWARDS
//

/* This functions are designed for, and should only be used for
 * revAvailableHandlers as they are tied to its requirements at present. */
static bool MCObjectListAppendObject(MCObjectList *&x_list, MCObject *p_object)
{
	if (x_list != nil)
	{
		MCObjectList *t_object;
		t_object = x_list;
		
		do
		{
			if (t_object->getobject() == p_object)
				return true;
			t_object = t_object->next();
		}
		while (t_object != x_list);
	}

	MCObjectList *t_newobject;
	t_newobject = nil;
	
    /* Make sure the object's script is compiled so that we can enumerate the
     * handlers, assuming it compiles. */
    p_object -> parsescript(False);
    
	t_newobject = new (nothrow) MCObjectList(p_object);
	
	if (t_newobject == nil)
		return false;
	
	if (x_list == nil)
		x_list = t_newobject;
	else
		x_list->append(t_newobject);
	
	return true;
}

static bool MCObjectListAppendObjectAndBehaviors(MCObjectList *&x_list, MCObject *p_object)
{
    if (!MCObjectListAppendObject(x_list, p_object))
    {
        return false;
    }
    
    MCParentScript *t_parent_script = p_object->getparentscript();
    while(t_parent_script != nullptr)
    {
        // If the behavior is blocked, then there are no more behaviors in the
        // chain.
        if (t_parent_script->IsBlocked())
        {
            break;
        }
        
        MCObject *t_parent_object = t_parent_script->GetObject();
        
        // If the behavior object has been deleted, then it won't currently be
        // blocked, but will have a null object.
        if (t_parent_object == nullptr)
        {
            break;
        }
        
        // We have a parent object, so add it.
        if (!MCObjectListAppendObject(x_list, t_parent_object))
        {
            return false;
        }
        
        t_parent_script = t_parent_object->getparentscript();
    }
    
    return true;
}

static bool MCObjectListAppendObjectList(MCObjectList *&x_list, MCObjectList *p_list)
{
	bool t_success;
	t_success = true;
	
	if (p_list != nil)
	{
		MCObjectList *t_object;
		t_object = p_list;
		
		do
		{
			if (!t_object->getremoved())
				t_success = MCObjectListAppendObjectAndBehaviors(x_list, t_object->getobject());
			t_object = t_object->next();
		}
		while (t_success && t_object != p_list);
	}
	
	return t_success;
}

static void MCObjectListFree(MCObjectList *p_list)
{
	if (p_list == nil)
		return;
	
	while (p_list->next() != p_list)
		delete p_list->next();
	
	delete p_list;
}

void MCObject::GetRevAvailableHandlers(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_handlers)
{
    // MW-2010-07-09: [[ Bug 8848 ]] Previously scripts were being compiled into
    //   separate hlists causing script local variable loss in the behavior
    //   case. Instead we just use parsescript in non-reporting mode.
    
    parsescript(False);
    if (hlist == nil)
    {
        r_count = 0;
        return;
    }
    
    hlist -> enumerate(ctxt, true, true, r_count, r_handlers);
}

static bool get_message_path_object_list_for_object(MCObject* p_object, MCObjectList*& r_object_list)
{
    
    bool t_success;
    t_success = true;
    
    MCObjectList *t_object_list;
    t_object_list = nil;
    
    t_success = MCObjectListAppendObjectList(t_object_list, MCfrontscripts);
    
    for (MCObject *t_object = p_object; t_success && t_object != NULL; t_object = t_object -> getparent())
    {
        t_success = MCObjectListAppendObjectAndBehaviors(t_object_list, t_object);
    }
    
    if (t_success)
        t_success = MCObjectListAppendObjectList(t_object_list, MCbackscripts);
    
    for (uint32_t i = 0; t_success && i < MCnusing; i++)
    {
        if (MCusing[i] == p_object)
            continue;
        t_success = MCObjectListAppendObjectAndBehaviors(t_object_list, MCusing[i]);
    }
    
    if (!t_success && t_object_list)
    {
        MCObjectListFree(t_object_list);
        t_object_list = nil;
    }
    
    r_object_list = t_object_list;
    
    return t_success;
}

void MCObject::GetEffectiveRevAvailableHandlers(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_handlers)
{
    bool t_first;
    t_first = true;
    
    MCAutoArray<MCStringRef> t_handlers;
    
    bool t_success;
    t_success = true;
    
    // IM-2014-02-25: [[ Bug 11841 ]] Collect non-repeating objects in the message path
    MCObjectList *t_object_list;
    if (t_success)
        t_success = get_message_path_object_list_for_object(this, t_object_list);
    
    // IM-2014-02-25: [[ Bug 11841 ]] Enumerate the handlers for each object
    if (t_success)
    {
        MCObjectList *t_object_ref;
        t_object_ref = t_object_list;
        do
        {
            // AL-2014-05-23: [[ Bug 12491 ]] The object list checks for uniqueness,
            //  so no need to check if the object is itself a frontscript.
            
            t_first = true;
            MCHandlerlist *t_handler_list;
            
            if (!t_object_ref -> getremoved() && t_object_ref -> getobject() -> getstack() -> iskeyed())
                t_handler_list = t_object_ref -> getobject() -> hlist;
            else
                t_handler_list = NULL;
            
            if (t_handler_list != NULL)
            {
                MCStringRef *t_handler_array;
                t_handler_array = nil;
                uindex_t t_count;
                
                t_first = t_handler_list -> enumerate(ctxt, t_object_ref->getobject() == this, t_first, t_count, t_handler_array);
            
                for (uindex_t i = 0; i < t_count; i++)
                    t_handlers . Push(t_handler_array[i]);
                
                MCMemoryDeleteArray(t_handler_array);
            }
            
            t_object_ref = t_object_ref -> next();
        }
        while(t_object_ref != t_object_list);
    }
    
    MCObjectListFree(t_object_list);
     
    t_handlers . Take(r_handlers, r_count);
}

void MCObject::GetRevAvailableVariablesNonArray(MCExecContext& ctxt, MCStringRef& r_variables)
{
    GetRevAvailableVariables(ctxt, nil, r_variables);
}

void MCObject::GetRevAvailableVariables(MCExecContext& ctxt, MCNameRef p_key, MCStringRef& r_variables)
{
    // OK-2008-04-23 : Added for script editor
    if (hlist == NULL)
    {
        r_variables = MCValueRetain(kMCEmptyString);
        return;
    }
    // A handler can be specified using array notation in the form <handler_type>,<handler_name>.
    // Where handler type is a single letter using the same conventation as the revAvailableHandlers.
    //
    // If a handler is specified, the list of variables for that handler is returned in the same format
    // as the variableNames property.
    //
    // If no handler is specified, the property returns the list of script locals for the object followed
    // by the list of script-declared globals.
    //
    // At the moment, no errors are thrown, just returns empty if it doesn't like something.
    if (p_key == nil)
    {
        MCAutoListRef t_list;
        if (!MCListCreateMutable('\n', &t_list))
            return;
        
        MCAutoListRef t_global_list, t_local_list;
        
        if (!(hlist->getlocalnames(&t_local_list) &&
              MCListAppend(*t_list, *t_local_list)))
        {
            ctxt . Throw();
            return;
        }
        
        if (!(hlist->getglobalnames(&t_global_list) &&
                MCListAppend(*t_list, *t_global_list)))
        {
            ctxt . Throw();
            return;
        }
        
        MCListCopyAsString(*t_list, r_variables);
        return;
    }

    
    MCStringRef t_key;
    t_key = MCNameGetString(p_key);
    
    // The handler name begins after the comma character
    MCAutoStringRef t_handler_name;
    uindex_t t_comma_offset;
    if (!MCStringFirstIndexOfChar(t_key, ',', 0, kMCCompareExact, t_comma_offset))
    {
        r_variables = MCValueRetain(kMCEmptyString);
        return;
    }
    
    if (!MCStringCopySubstring(t_key, MCRangeMake(t_comma_offset + 1, MCStringGetLength(t_key) - t_comma_offset - 1), &t_handler_name))
    {
        ctxt . Throw();
        return;
    }
    
    // The handler code must be the first char of the string
    const char_t t_handler_code = MCStringGetNativeCharAtIndex(t_key, 0);
    
    Handler_type t_handler_type;
    switch (t_handler_code)
    {
        case 'M':
            t_handler_type = HT_MESSAGE;
            break;
        case 'C':
            t_handler_type = HT_MESSAGE;
            break;
        case 'F':
            t_handler_type = HT_FUNCTION;
            break;
        case 'G':
            t_handler_type = HT_GETPROP;
            break;
        case 'S':
            t_handler_type = HT_SETPROP;
            break;
        case 'A':
            t_handler_type = HT_AFTER;
            break;
        case 'B':
            t_handler_type = HT_BEFORE;
            break;
        default:
            t_handler_type = HT_MESSAGE;
            break;
    }
    
    Exec_stat t_status;
    MCNewAutoNameRef t_name;
    MCNameCreate(*t_handler_name, &t_name);
    // The handler list class allows us to locate the handler, just return empty if it can't be found.
    MCHandler *t_handler;
    t_status = hlist -> findhandler(t_handler_type, *t_name, t_handler);

    if (t_status == ES_NORMAL)
    {
        MCAutoListRef t_list;
        t_handler -> getvarnames(true, &t_list);
        MCListCopyAsString(*t_list, r_variables);
    }
    else
        r_variables = nil;
}

void MCObject::GetRevScriptDescription(MCExecContext& ctxt, MCValueRef& r_desc)
{
    if (!getstack() -> iskeyed())
    {
        ctxt . LegacyThrow(EE_STACK_NOKEY);
        return;
    }
    
    MCAutoValueRefBase<MCScriptObjectRef> t_object_ref;
    if (!MCEngineScriptObjectCreate(this, 0, &t_object_ref))
    {
        ctxt.Throw();
        return;
    }
    
    MCValueRef t_desc = MCEngineExecDescribeScriptOfScriptObject(*t_object_ref);
    if (t_desc == nil)
    {
        ctxt.Throw();
        return;
    }
    
    if (!MCExtensionConvertToScriptType(ctxt, t_desc))
    {
        MCValueRelease(t_desc);
        ctxt.Throw();
        return;
    }

    r_desc = t_desc;
}

void MCObject::GetEffectiveRevScriptDescription(MCExecContext& ctxt, MCValueRef& r_descriptions)
{
    bool t_success;
    t_success = true;
    
    MCAutoArrayRef t_descriptions;
    t_success = MCArrayCreateMutable(&t_descriptions);
    
    MCObjectList *t_object_list;
    if (t_success)
        t_success = get_message_path_object_list_for_object(this, t_object_list);
    
    if (t_success)
    {
        MCObjectList *t_object_ref;
        t_object_ref = t_object_list;
        
        index_t t_index = 1;
        do
        {
            if (!t_object_ref -> getremoved() && t_object_ref -> getobject() -> getstack() -> iskeyed())
            {
                MCAutoValueRefBase<MCScriptObjectRef> t_script_object_ref;
                if (t_success)
                    t_success = MCEngineScriptObjectCreate(t_object_ref -> getobject(), 0, &t_script_object_ref);
                
                MCAutoArrayRef t_description;
                if (t_success)
                {
                    t_description.Give(MCEngineExecDescribeScriptOfScriptObject(*t_script_object_ref,
                                                                                t_object_ref -> getobject() == this));
                    t_success = t_description.IsSet();
                }
                    
                MCAutoArrayRef t_object_description;
                if (t_success)
                {
                    MCAutoStringRef t_object_id;
                    t_object_ref -> getobject() -> GetLongId(ctxt, 0, &t_object_id);
                    
                    MCNameRef t_keys[2];
                    t_keys[0] = MCNAME("object");
                    t_keys[1] = MCNAME("description");
                    
                    MCValueRef t_values[2];
                    t_values[0] = *t_object_id;
                    t_values[1] = *t_description;
                    
                    t_success = MCArrayCreate(false,
                                              t_keys,
                                              t_values,
                                              sizeof(t_keys) / sizeof(t_keys[0]),
                                              &t_object_description);
                }
                
                if (t_success)
                    t_success = MCArrayStoreValueAtIndex(*t_descriptions,
                                                         t_index,
                                                         *t_object_description);
                
                ++t_index;
                
            }
            
            t_object_ref = t_object_ref -> prev();
        }
        while(t_success && t_object_ref != t_object_list);
    }
    
    MCObjectListFree(t_object_list);
    
    MCValueRef t_value;
    if (t_success)
    {
        t_value = t_descriptions.Take();
        t_success = MCExtensionConvertToScriptType(ctxt, t_value);
        if (!t_success)
        {
            MCValueRelease(t_value);
        }
    }
    
    if (!t_success)
        ctxt . Throw();
    else
        r_descriptions = t_value;
}

void MCObject::GetRevBehaviorUses(MCExecContext& ctxt, MCArrayRef& r_objects)
{
    MCParentScript *t_parent;
    t_parent = MCParentScript::Lookup(this);
    
    if (t_parent == nullptr)
    {
        r_objects = MCValueRetain(kMCEmptyArray);
        return;
    }
    
    if (!t_parent->CopyUses(r_objects))
    {
        ctxt.Throw();
        return;
    }
}

