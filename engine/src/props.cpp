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

#ifdef OLD_EXEC
typedef struct _PropList
{
	const char *token;
	uint2 value;
}
PropList;

static PropList stackprops[] =
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

static PropList cardprops[] =
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

static PropList groupprops[] =
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

static PropList buttonprops[] =
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
        // AL-2014-07-23: [[ Bug 12894 ]] Add iconGravity to button properties list
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

static PropList fieldprops[] =
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

static PropList imageprops[] =
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

static PropList graphicprops[] =
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

static PropList scrollbarprops[] =
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

static PropList playerprops[] =
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
		{"dontuseqt", P_DONT_USE_QT},
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
        {"mirrored", P_MIRRORED},
        {"name", P_SHORT_NAME},
        {"outerGlow", P_BITMAP_EFFECT_OUTER_GLOW},
        {"opaque", P_OPAQUE},
        {"playRate", P_PLAY_RATE},
        {"playSelection", P_PLAY_SELECTION},
        {"rect", P_RECTANGLE},
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

static PropList epsprops[] =
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

static PropList colorpaletteprops[] =
    {
        {"name", P_SHORT_NAME},
        {"id", P_ID},
        {"selectedColor", P_SELECTED_COLOR},
        {"rect", P_RECTANGLE}
    };

static PropList audioclipprops[] =
    {
        {"altID", P_ALT_ID},
        {"id", P_ID},
        {"name", P_NAME},
        {"playLoudness", P_PLAY_LOUDNESS},
    };

static PropList videoclipprops[] =
    {
        {"altID", P_ALT_ID},
        {"dontRefresh", P_DONT_REFRESH},
        {"frameRate", P_FRAME_RATE},
        {"id", P_ID},
        {"name", P_NAME},
        {"playLoudness", P_PLAY_LOUDNESS},
        {"scale", P_SCALE},
    };

Exec_stat MCObject::getproparray(MCExecPoint &ep, uint4 parid, bool effective)
{
	PropList *table;
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
	default:
		return ES_NORMAL;
	}
	MCAutoArrayRef t_array;
	/* UNCHECKED */ MCArrayCreateMutable(&t_array);
	MCerrorlock++;
	while (tablesize--)
	{
        const char * t_token = table[tablesize].token;
        
        if ((Properties)table[tablesize].value > P_FIRST_ARRAY_PROP)
            getarrayprop(parid, (Properties)table[tablesize].value, ep, kMCEmptyName, effective);
        else
        {
            // MERG-2013-05-07: [[ RevisedPropsProp ]] Special-case the props that could
			//   be either Unicode or native (ensure minimal encoding is used).
			// MERG-2013-06-24: [[ RevisedPropsProp ]] Treat the short name specially to ensure
			//   round-tripping. If the name is empty, then return empty for 'name'.
            switch ((Properties)table[tablesize].value) {
                case P_SHORT_NAME:
                    if (isunnamed())
                        ep.clear();
                    else
                        getprop(parid, P_SHORT_NAME, ep, effective);
                    break;
                case P_LABEL:
                    getprop(parid, P_UNICODE_LABEL, ep, effective);
                    if (!ep.trytoconvertutf16tonative())
                    {
                        if (gettype() == CT_STACK)
                            t_token = "unicodeTitle";
                        else
                            t_token = "unicodeLabel";
                    }
                    break;
                case P_TOOL_TIP:
                    getprop(parid, P_UNICODE_TOOL_TIP, ep, effective);
                    if (!ep.trytoconvertutf16tonative())
                        t_token = "unicodeToolTip";
                    break;
                case P_TEXT:
                    if (gettype() == CT_BUTTON)
                    {
                        getprop(parid, P_UNICODE_TEXT, ep, effective);
                        if (!ep.trytoconvertutf16tonative())
                            t_token = "unicodeText";
                        break;
                    }
                default:
                    getprop(parid, (Properties)table[tablesize].value, ep, effective);
                    break;
            }
        }
        
		ep.storearrayelement_cstring(*t_array, table[tablesize].token);
	}
	MCerrorlock--;
	ep.setvalueref(*t_array);
	return ES_NORMAL;
}
#endif
