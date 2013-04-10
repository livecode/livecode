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
		{"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"cantAbort", P_CANT_ABORT},
        {"cantDelete", P_CANT_DELETE},
        {"cantModify", P_CANT_MODIFY},
        {"colors", P_COLORS},
        {"decorations", P_DECORATIONS},
        {"destroyStack", P_DESTROY_STACK},
        {"destroyWindow", P_DESTROY_WINDOW},
        {"dynamicPaths", P_DYNAMIC_PATHS},
        {"formatForPrinting", P_FORMAT_FOR_PRINTING},
        {"hcAddressing", P_HC_ADDRESSING},
        {"icon", P_ICON},
        {"iconic", P_ICONIC},
        {"id", P_ID},
        {"linkColor", P_LINK_COLOR},
        {"linkHiliteColor", P_LINK_HILITE_COLOR},
        {"linkVisitedColor", P_LINK_VISITED_COLOR},
        {"liveresizing", P_LIVE_RESIZING},
        {"maxHeight", P_MAX_HEIGHT},
        {"maxWidth", P_MAX_WIDTH},
        {"menubar", P_MENU_BAR},
        {"metal", P_METAL},
        {"minHeight", P_MIN_HEIGHT},
        {"minWidth", P_MIN_WIDTH},
        {"name", P_SHORT_NAME},
        {"passkey", P_KEY},
        {"password", P_PASSWORD},
        {"patterns", P_PATTERNS},
        {"rect", P_RECTANGLE},
        {"resizable", P_RESIZABLE},
        {"scroll", P_VSCROLL},
        {"shadow", P_SHADOW},
        {"startUpIconic", P_START_UP_ICONIC},
        {"style", P_STYLE},
        {"textFont", P_TEXT_FONT},
        {"textSize", P_TEXT_SIZE},
        {"textStyle", P_TEXT_STYLE},
        {"title", P_LABEL},
        {"underlineLinks", P_UNDERLINE_LINKS},
        {"visible", P_VISIBLE},
        {"windowManagerPlace", P_WM_PLACE},
        {"windowShape", P_WINDOW_SHAPE}
    };

static PropList cardprops[] =
    {
        {"altId", P_ALT_ID},
		{"behavior", P_PARENT_SCRIPT},
        {"borderWidth", P_BORDER_WIDTH},
        {"blendLevel", P_BLEND_LEVEL},
        {"cantDelete", P_CANT_DELETE},
        {"colors", P_COLORS},
        {"defaultButton", P_DEFAULT_BUTTON},
        {"dontSearch", P_DONT_SEARCH},
        {"id", P_ID},
        {"ink", P_INK},
        {"layer", P_LAYER},
        {"mark", P_MARKED},
        {"name", P_SHORT_NAME},
        {"patterns", P_PATTERNS},
        {"rect", P_RECTANGLE},
        {"showBorder", P_SHOW_BORDER},
        {"textFont", P_TEXT_FONT},
        {"textSize", P_TEXT_SIZE},
        {"textStyle", P_TEXT_STYLE},
        {"threeD", P_3D}
    };

static PropList groupprops[] =
    {
        {"altId", P_ALT_ID},
        {"backgroundBehavior", P_BACKGROUND_BEHAVIOR},
		{"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderWidth", P_BORDER_WIDTH},
        {"boundingRect", P_BOUNDING_RECT},
        {"cantDelete", P_CANT_DELETE},
        {"cantSelect", P_CANT_SELECT},
        {"colors", P_COLORS},
        {"disabled", P_DISABLED},
        {"dontSearch", P_DONT_SEARCH},
        {"hilitedButton", P_HILITED_BUTTON},
        {"hScroll", P_HSCROLL},
        {"hScrollbar", P_HSCROLLBAR},
        {"id", P_ID},
        {"ink", P_INK},
        {"label", P_LABEL},
        {"layer", P_LAYER},
        {"lockLoc", P_LOCK_LOCATION},
        {"margins", P_MARGINS},
        {"name", P_SHORT_NAME},
        {"opaque", P_OPAQUE},
        {"patterns", P_PATTERNS},
        {"radioBehavior", P_RADIO_BEHAVIOR},
        {"rect", P_RECTANGLE},
        {"scrollbarWidth", P_SCROLLBAR_WIDTH},
        {"showBorder", P_SHOW_BORDER},
        {"showName", P_SHOW_NAME},
        {"tabGroupBehavior", P_TAB_GROUP_BEHAVIOR},
        {"textFont", P_TEXT_FONT},
        {"textSize", P_TEXT_SIZE},
        {"textStyle", P_TEXT_STYLE},
        {"threeD", P_3D},
        {"toolTip", P_TOOL_TIP},
        {"traversalOn", P_TRAVERSAL_ON},
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
		{"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderWidth", P_BORDER_WIDTH},
        {"cantSelect", P_CANT_SELECT},
        {"colors", P_COLORS},
        {"default", P_DEFAULT},
        {"disabled", P_DISABLED},
        {"disabledIcon", P_DISABLED_ICON},
        {"family", P_FAMILY},
        {"hilited", P_HILITE},
        {"hiliteBorder", P_HILITE_BORDER},
        {"hiliteFill", P_HILITE_FILL},
        {"hiliteIcon", P_HILITED_ICON},
		{"hoverIcon", P_HOVER_ICON},
        {"icon", P_ICON},
        {"id", P_ID},
        {"ink", P_INK},
        {"label", P_LABEL},
        {"labelWidth", P_LABEL_WIDTH},
        {"layer", P_LAYER},
        {"lockLoc", P_LOCK_LOCATION},
        {"margins", P_MARGINS},
        {"menuHistory", P_MENU_HISTORY},
        {"menuLines", P_MENU_LINES},
        {"menuMouseButton", P_MENU_BUTTON},
        {"menuMode", P_MENU_MODE},
        {"menuName", P_MENU_NAME},
        {"mnemonic", P_MNEMONIC},
        {"name", P_SHORT_NAME},
        {"opaque", P_OPAQUE},
        {"patterns", P_PATTERNS},
        {"rect", P_RECTANGLE},
        {"shadow", P_SHADOW},
        {"shadowOffset", P_SHADOW_OFFSET},
        {"sharedHilite", P_SHARED_HILITE},
        {"showBorder", P_SHOW_BORDER},
        {"showFocusBorder", P_SHOW_FOCUS_BORDER},
        {"showHilite", P_SHOW_HILITE},
        {"showIcon", P_SHOW_ICON},
        {"showName", P_SHOW_NAME},
        {"style", P_STYLE},
        {"text", P_TEXT},
        {"textAlign", P_TEXT_ALIGN},
        {"textFont", P_TEXT_FONT},
        {"textSize", P_TEXT_SIZE},
        {"textStyle", P_TEXT_STYLE},
        {"threeD", P_3D},
        {"toolTip", P_TOOL_TIP},
        {"traversalOn", P_TRAVERSAL_ON},
        {"visible", P_VISIBLE},
        {"visited", P_VISITED},
        {"visitedIcon", P_VISITED_ICON}
    };

static PropList fieldprops[] =
    {
        {"altId", P_ALT_ID},
        {"autoHilite", P_AUTO_HILITE},
        {"autoTab", P_AUTO_TAB},
		{"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderWidth", P_BORDER_WIDTH},
        {"cantSelect", P_CANT_SELECT},
        {"colors", P_COLORS},
        {"disabled", P_DISABLED},
        {"dontSearch", P_DONT_SEARCH},
        {"dontWrap", P_DONT_WRAP},
        {"firstIndent", P_FIRST_INDENT},
        {"fixedLineHeight", P_FIXED_HEIGHT},
        {"hGrid", P_HGRID},
        {"hilitedLines", P_HILITED_LINES},
        {"hScroll", P_HSCROLL},
        {"hScrollbar", P_HSCROLLBAR},
        {"htmlText", P_HTML_TEXT},
        {"id", P_ID},
        {"ink", P_INK},
        {"layer", P_LAYER},
        {"listBehavior", P_LIST_BEHAVIOR},
        {"lockLoc", P_LOCK_LOCATION},
        {"lockText", P_LOCK_TEXT},
        {"margins", P_MARGINS},
        {"multipleHilites", P_MULTIPLE_HILITES},
        {"name", P_SHORT_NAME},
        {"noncontiguousHilites", P_NONCONTIGUOUS_HILITES},
        {"opaque", P_OPAQUE},
        {"patterns", P_PATTERNS},
        {"rect", P_RECTANGLE},
        {"scroll", P_VSCROLL},
        {"scrollbarWidth", P_SCROLLBAR_WIDTH},
        {"shadow", P_SHADOW},
        {"shadowOffset", P_SHADOW_OFFSET},
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
		{"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderWidth", P_BORDER_WIDTH},
        {"cantSelect", P_CANT_SELECT},
        {"colors", P_COLORS},
        {"constantMask", P_CONSTANT_MASK},
        {"currentFrame", P_CURRENT_FRAME},
        {"disabled", P_DISABLED},
        {"dontDither", P_DONT_DITHER},
        {"fileName", P_FILE_NAME},
        {"frameCount", P_FRAME_COUNT},
        {"hotSpot", P_HOT_SPOT},
        {"id", P_ID},
        {"ink", P_INK},
        {"layer", P_LAYER},
        {"lockLoc", P_LOCK_LOCATION},
        {"name", P_SHORT_NAME},
        {"opaque", P_OPAQUE},
        {"palindromeFrames", P_PALINDROME_FRAMES},
        {"rect", P_RECTANGLE},
        {"repeatCount", P_REPEAT_COUNT},
        {"showBorder", P_SHOW_BORDER},
        {"showFocusBorder", P_SHOW_FOCUS_BORDER},
        {"size", P_SIZE},
        {"threeD", P_3D},
        {"toolTip", P_TOOL_TIP},
        {"traversalOn", P_TRAVERSAL_ON},
        {"visible", P_VISIBLE},
        {"xHot", P_XHOT},
        {"yHot", P_YHOT}
    };

static PropList graphicprops[] =
    {
        {"altId", P_ALT_ID},
        {"angle", P_ANGLE},
		{"antiAliased", P_ANTI_ALIASED},
        {"arcAngle", P_ARC_ANGLE},
        {"arrowSize", P_ARROW_SIZE},
		{"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderWidth", P_BORDER_WIDTH},
        {"cantSelect", P_CANT_SELECT},
        {"colors", P_COLORS},
        {"dashes", P_DASHES},
        {"disabled", P_DISABLED},
        {"dontResize", P_DONT_RESIZE},
        {"endArrow", P_END_ARROW},
        {"filled", P_FILLED},
        {"id", P_ID},
        {"ink", P_INK},
        {"label", P_LABEL},
        {"layer", P_LAYER},
        {"lineSize", P_LINE_SIZE},
        {"lockLoc", P_LOCK_LOCATION},
        {"margins", P_MARGINS},
        {"markerDrawn", P_MARKER_DRAWN},
        {"markerFilled", P_MARKER_OPAQUE},
        {"markerPoints", P_MARKER_POINTS},
        {"name", P_SHORT_NAME},
        {"points", P_POINTS},
        {"polySides", P_POLY_SIDES},
        {"opaque", P_OPAQUE},
        {"patterns", P_PATTERNS},
        {"rect", P_RECTANGLE},
        {"roundEnds", P_ROUND_ENDS},
        {"roundRadius", P_ROUND_RADIUS},
        {"showBorder", P_SHOW_BORDER},
        {"showFocusBorder", P_SHOW_FOCUS_BORDER},
        {"showName", P_SHOW_NAME},
        {"startArrow", P_START_ARROW},
        {"style", P_STYLE},
        {"textFont", P_TEXT_FONT},
        {"textSize", P_TEXT_SIZE},
        {"textStyle", P_TEXT_STYLE},
        {"threeD", P_3D},
        {"toolTip", P_TOOL_TIP},
        {"traversalOn", P_TRAVERSAL_ON},
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
		{"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderWidth", P_BORDER_WIDTH},
        {"cantSelect", P_CANT_SELECT},
        {"colors", P_COLORS},
        {"disabled", P_DISABLED},
        {"endValue", P_END_VALUE},
        {"id", P_ID},
        {"ink", P_INK},
        {"layer", P_LAYER},
        {"lineInc", P_LINE_INC},
        {"lockLoc", P_LOCK_LOCATION},
        {"name", P_SHORT_NAME},
        {"numberFormat", P_NUMBER_FORMAT},
        {"opaque", P_OPAQUE},
        {"orientation", P_ORIENTATION},
        {"pageInc", P_PAGE_INC},
        {"patterns", P_PATTERNS},
        {"rect", P_RECTANGLE},
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
        {"traversalOn", P_TRAVERSAL_ON},
        {"visible", P_VISIBLE},
        {"thumbSize", P_THUMB_SIZE}
    };

static PropList playerprops[] =
    {
        {"altId", P_ALT_ID},
        {"alwaysBuffer", P_ALWAYS_BUFFER},
		{"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderWidth", P_BORDER_WIDTH},
        {"callbacks", P_CALLBACKS},
        {"cantSelect", P_CANT_SELECT},
        {"colors", P_COLORS},
        {"disabled", P_DISABLED},
        {"endTime", P_END_TIME},
        {"fileName", P_FILE_NAME},
        {"id", P_ID},
        {"ink", P_INK},
        {"layer", P_LAYER},
        {"lockLoc", P_LOCK_LOCATION},
        {"looping", P_LOOPING},
        {"name", P_SHORT_NAME},
        {"opaque", P_OPAQUE},
        {"patterns", P_PATTERNS},
        {"playRate", P_PLAY_RATE},
        {"playSelection", P_PLAY_SELECTION},
        {"rect", P_RECTANGLE},
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
        {"traversalOn", P_TRAVERSAL_ON},
        {"visible", P_VISIBLE},
    };

static PropList epsprops[] =
    {
        {"altId", P_ALT_ID},
		{"behavior", P_PARENT_SCRIPT},
        {"blendLevel", P_BLEND_LEVEL},
        {"borderWidth", P_BORDER_WIDTH},
        {"boundingBox", P_BOUNDING_RECT},
        {"cantSelect", P_CANT_SELECT},
        {"colors", P_COLORS},
        {"currentPage", P_CURRENT_PAGE},
        {"disabled", P_DISABLED},
        {"id", P_ID},
        {"ink", P_INK},
        {"layer", P_LAYER},
        {"lockLoc", P_LOCK_LOCATION},
        {"name", P_SHORT_NAME},
        {"opaque", P_OPAQUE},
        {"pageCount", P_PAGE_COUNT},
        {"patterns", P_PATTERNS},
        {"postScript", P_POSTSCRIPT},
        {"prolog", P_PROLOG},
        {"rect", P_RECTANGLE},
        {"retainImage", P_RETAIN_IMAGE},
        {"retainPostScript", P_RETAIN_POSTSCRIPT},
        {"scale", P_SCALE},
        {"scaleIndependently", P_SCALE_INDEPENDENTLY},
        {"showBorder", P_SHOW_BORDER},
        {"showFocusBorder", P_SHOW_FOCUS_BORDER},
        {"threeD", P_3D},
        {"toolTip", P_TOOL_TIP},
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
        {"size", P_SIZE}
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
        {"size", P_SIZE}
    };

Exec_stat MCObject::getproparray(MCExecPoint &ep, uint4 parid)
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
	MCVariableValue *v = new MCVariableValue;
	v->assign_new_array(64);
	MCerrorlock++;
	while (tablesize--)
	{
		getprop(parid, (Properties)table[tablesize].value, ep, False);
		v->store_element(ep, table[tablesize].token);
	}
	MCerrorlock--;
	ep.setarray(v, True);
	return ES_NORMAL;
}
