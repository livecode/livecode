/* Copyright (C) 2003-2014 Runtime Revolution Ltd.
 
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

////////////////////////////////////////////////////////////////////////////////

#include "module-canvas.h"

////////////////////////////////////////////////////////////////////////////////

// Useful stuff

bool MCMemoryAllocateArrayCopy(const void *p_array, uint32_t p_size, uint32_t p_count, void *&r_copy)
{
	return MCMemoryAllocateCopy(p_array, p_size * p_count, r_copy);
}

template <class T>
bool MCMemoryAllocateArrayCopy(const T *p_array, uint32_t p_count, T *&r_copy)
{
	void *t_copy;
	if (!MCMemoryAllocateArrayCopy(p_array, sizeof(T), p_count, t_copy))
		return false;
	
	r_copy = (T*)t_copy;
	return true;
}

bool MCArrayStoreReal(MCArrayRef p_array, MCNameRef p_key, real64_t p_value)
{
	bool t_success;
	t_success = true;
	
	MCNumberRef t_number;
	t_number = nil;
	
	if (t_success)
		t_success = MCNumberCreateWithReal(p_value, t_number);
	
	if (t_success)
		t_success = MCArrayStoreValue(p_array, false, p_key, t_number);
	
	MCValueRelease(t_number);
	
	return t_success;
}

bool MCArrayFetchReal(MCArrayRef p_array, MCNameRef p_key, real64_t &r_value)
{
	bool t_success;
	t_success = true;
	
	MCValueRef t_value;
	t_value = nil;
	
	if (t_success)
		t_success = MCArrayFetchValue(p_array, false, p_key, t_value);
	
	if (t_success)
	{
		t_success = MCValueGetTypeCode(t_value) == kMCValueTypeCodeNumber;
		// TODO - throw error on failure
	}
	
	if (t_success)
		r_value = MCNumberFetchAsReal((MCNumberRef)t_value);
	
	return t_success;
}

bool MCArrayFetchString(MCArrayRef p_array, MCNameRef p_key, MCStringRef &r_value)
{
	bool t_success;
	t_success = true;
	
	MCValueRef t_value;
	t_value = nil;
	
	if (t_success)
		t_success = MCArrayFetchValue(p_array, false, p_key, t_value);
	
	if (t_success)
	{
		if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeName)
			r_value = MCNameGetString((MCNameRef)t_value);
		else if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeString)
			r_value = (MCStringRef)t_value;
		else
			t_success = false;
	}
	
	return t_success;
}

bool MCArrayFetchCanvasColor(MCArrayRef p_array, MCNameRef p_key, MCCanvasColorRef &r_value)
{
	bool t_success;
	t_success = true;
	
	MCValueRef t_value;
	t_value = nil;
	
	if (t_success)
		t_success = MCArrayFetchValue(p_array, false, p_key, t_value);
	
	if (t_success)
		t_success = MCValueGetTypeInfo(t_value) == kMCCanvasColorTypeInfo;
	
	if (t_success)
		r_value = (MCCanvasColorRef)t_value;
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

inline MCGFloat MCGAffineTransformGetEffectiveScale(const MCGAffineTransform &p_transform)
{
	return MCMax(MCAbs(p_transform.a) + MCAbs(p_transform.c), MCAbs(p_transform.d) + MCAbs(p_transform.b));
}

////////////////////////////////////////////////////////////////////////////////

bool MCGPointParse(MCStringRef p_string, MCGPoint &r_point)
{
	bool t_success;
	t_success = nil;
	
	MCProperListRef t_items;
	t_items = nil;
	
	if (t_success)
		t_success = MCStringSplitByDelimiter(p_string, kMCCommaString, kMCStringOptionCompareExact, t_items);
	
	if (t_success)
		t_success = MCProperListGetLength(t_items) == 2;
	
	MCNumberRef t_x_num, t_y_num;
	t_x_num = t_y_num = nil;
	
	if (t_success)
		t_success = MCNumberParse((MCStringRef)MCProperListFetchElementAtIndex(t_items, 0), t_x_num);
	
	if (t_success)
		t_success = MCNumberParse((MCStringRef)MCProperListFetchElementAtIndex(t_items, 1), t_y_num);
	
	if (t_success)
		r_point = MCGPointMake(MCNumberFetchAsReal(t_x_num), MCNumberFetchAsReal(t_y_num));
	
	MCValueRelease(t_x_num);
	MCValueRelease(t_y_num);
	MCValueRelease(t_items);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSolveQuadraticEqn(MCGFloat p_a, MCGFloat p_b, MCGFloat p_c, MCGFloat &r_x_1, MCGFloat &r_x_2)
{
	MCGFloat t_det;
	t_det = p_b * p_b - 4 * p_a * p_c;
	
	if (t_det < 0)
		return false;
	
	MCGFloat t_sqrt;
	t_sqrt = sqrtf(t_det);
	
	r_x_1 = (-p_b + t_sqrt) / (2 * p_a);
	r_x_2 = (-p_b - t_sqrt) / (2 * p_a);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

// Type Definitions

// Rectangle opaque type methods
uinteger_t MCCanvasRectangleType_Measure(void)
{
	return sizeof(MCCanvasRectangle);
}

// Point opaque type methods
uinteger_t MCCanvasPointType_Measure(void)
{
	return sizeof(MCCanvasPoint);
}

// Transform opaque type methods
uinteger_t MCCanvasTransformType_Measure(void)
{
	return sizeof(MCCanvasTransform);
}

// Image opaque type methods
uinteger_t MCCanvasImageType_Measure(void)
{
	return sizeof(MCCanvasImage);
}

void MCCanvasImageType_Finalize(MCCanvasImage *p_image)
{
	MCImageRepRelease(*p_image);
}

void MCCanvasImageType_Copy(MCCanvasImage *p_src_image, MCCanvasImage *p_dst_image)
{
	*p_dst_image = MCImageRepRetain(*p_src_image);
}

// Paint opaque type methods
uinteger_t MCCanvasPaintType_Measure(void)
{
	return sizeof(MCCanvasPaint);
}

// defer to subtype finalize methods
void MCCanvasPaintType_Finalize(MCCanvasPaint *p_paint)
{
	switch ((*p_paint)->type)
	{
		case kMCCanvasPaintTypeSolid:
			MCCanvasSolidPaintType_Finalize((MCCanvasSolidPaint *)p_paint);
			break;
			
		case kMCCanvasPaintTypePattern:
			MCCanvasPatternType_Finalize((MCCanvasPattern *)p_paint);
			break;
			
		case kMCCanvasPaintTypeGradient:
			MCCanvasGradientType_Finalize((MCCanvasGradient *)p_paint);
			break;
			
		default:
			MCAssert(false);
			
	}
}

// Solid Paint opaque type methods
bool MCCanvasSolidPaintType_TypeCheck(MCCanvasPaint *p_paint)
{
	return *p_paint != nil && (*p_paint)->type == kMCCanvasPaintTypeSolid;
}

void MCCanvasSolidPaintDelete(MCCanvasSolidPaint p_paint)
{
	if (p_paint == nil)
		return;
	
	MCCanvasColorDelete(p_paint->color);
	MCMemoryDelete(p_paint);
}

bool MCCanvasSolidPaintCopy(MCCanvasSolidPaint p_paint, MCCanvasSolidPaint &r_copy)
{
	bool t_success;
	t_success = true;
	
	MCCanvasSolidPaint t_solid;
	t_solid = nil;
	
	if (t_success)
		t_success = MCMemoryNew(t_solid);
	
	if (t_success)
	{
		t_solid->type = kMCCanvasPaintTypeSolid;
		t_success = MCCanvasColorCopy(p_paint->color, t_solid->color);
	}
	
	if (t_success)
		r_copy = t_solid;
	else
		MCCanvasSolidPaintDelete(t_solid);
	
	return t_success;
}

void MCCanvasSolidPaintType_Copy(MCCanvasSolidPaint *p_src, MCCanvasSolidPaint *p_dst)
{
	/* UNCHECKED */ MCCanvasSolidPaintCopy(*p_src, *p_dst);
}

void MCCanvasSolidPaintType_Finalize(MCCanvasSolidPaint *p_paint)
{
	MCCanvasSolidPaintDelete(*p_paint);
	*p_paint = nil;
}

// Pattern opaque type methods
bool MCCanvasPatternType_TypeCheck(MCCanvasPaint *p_paint)
{
	return *p_paint != nil && (*p_paint)->type == kMCCanvasPaintTypePattern;
}

void MCCanvasPatternType_Finalize(MCCanvasPattern *p_pattern)
{
	if (*p_pattern == nil)
		return;
	
	MCImageRepRelease((*p_pattern)->image);
	MCMemoryDelete(*p_pattern);
}

void MCCanvasPatternType_Copy(MCCanvasPattern *p_src, MCCanvasPattern *p_dst)
{
	MCCanvasPattern t_pattern;
	t_pattern = nil;
	
	if (!MCMemoryAllocateCopy(*p_dst, sizeof(MCCanvasPatternStruct), *p_dst))
	{
		// TODO - throw error
		return;
	}
	
	MCImageRepRetain(t_pattern->image);
	
	*p_dst = t_pattern;
}

// Gradient stop methods
bool MCCanvasGradientStopCopy(const MCCanvasGradientStop &p_src, MCCanvasGradientStop &p_dst)
{
	if (!MCCanvasColorCopy(p_src.color, p_dst.color))
		return false;
	
	p_dst.offset = p_src.offset;
	return true;
}

void MCCanvasGradientStopClear(MCCanvasGradientStop &x_stop)
{
	MCCanvasColorDelete(x_stop.color);
	x_stop.color = nil;
	x_stop.offset = 0;
}

// Gradient ramp methods
void MCCanvasGradientRampDelete(MCCanvasGradientStop *p_ramp, uint32_t p_length)
{
	if (p_ramp == nil)
		return;
	
	for (uint32_t i = 0; i < p_length; i++)
		MCCanvasGradientStopClear(p_ramp[i]);
	
	MCMemoryDeleteArray(p_ramp);
}
bool MCCanvasGradientRampCopy(const MCCanvasGradientStop *p_ramp, uint32_t p_length, MCCanvasGradientStop *&r_copy)
{
	bool t_success;
	t_success = true;
	
	MCCanvasGradientStop *t_ramp;
	t_ramp = nil;
	
	if (t_success)
		t_success = MCMemoryNewArray(p_length, t_ramp);
	
	for (uint32_t i = 0; t_success && i < p_length; i++)
		t_success = MCCanvasGradientStopCopy(p_ramp[i], t_ramp[i]);
	
	if (t_success)
		r_copy = t_ramp;
	else
		MCCanvasGradientRampDelete(t_ramp, p_length);
	
	return t_success;
}

// Gradient opaque type methods
bool MCCanvasGradientType_TypeCheck(MCCanvasPaint *p_paint)
{
	return *p_paint != nil && (*p_paint)->type == kMCCanvasPaintTypeGradient;
}

void MCCanvasGradientType_Finalize(MCCanvasGradient *p_gradient)
{
	if (*p_gradient == nil)
		return;
	
	MCCanvasGradientRampDelete((*p_gradient)->ramp, (*p_gradient)->ramp_length);
	MCMemoryDelete(*p_gradient);
}

void MCCanvasGradientType_Copy(MCCanvasGradient *p_src, MCCanvasGradient *p_dst)
{
	MCCanvasGradient t_gradient;
	t_gradient = nil;
	
	if (!MCMemoryAllocateCopy(*p_dst, sizeof(MCCanvasGradientStruct), t_gradient))
	{
		// TODO - throw error
		return;
	}
	
	if (!MCCanvasGradientRampCopy((*p_src)->ramp, (*p_src)->ramp_length, t_gradient->ramp))
	{
		// TODO - throw error
		MCMemoryDeallocate(t_gradient);
		return;
	}
	
	*p_dst = t_gradient;
}

void MCCanvasPaintType_Copy(const MCCanvasPaint *p_src, MCCanvasPaint *p_dst)
{
	switch ((*p_src)->type)
	{
		case kMCCanvasPaintTypeSolid:
			MCCanvasSolidPaintType_Copy((MCCanvasSolidPaint*)p_src, (MCCanvasSolidPaint*)p_dst);
			break;
			
		case kMCCanvasPaintTypePattern:
			MCCanvasPatternType_Copy((MCCanvasPattern*)p_src, (MCCanvasPattern*)p_dst);
			break;
			
		case kMCCanvasPaintTypeGradient:
			MCCanvasGradientType_Copy((MCCanvasGradient*)p_src, (MCCanvasGradient*)p_dst);
			break;
			
		default:
			MCAssert(false);
			
	}
}

// Path opaque type methods
uinteger_t MCCanvasPathType_Measure(void)
{
	return sizeof(MCCanvasPath);
}

void MCCanvasPathType_Finalize(MCCanvasPath *p_path)
{
	MCGPathRelease(p_path->path);
}

void MCCanvasPathType_Copy(MCCanvasPath *p_src_path, MCCanvasPath *p_dst_path)
{
	/* UNCHECKED */ MCGPathMutableCopy(p_src_path->path, p_dst_path->path);
}

// Effect opaque type methods
uinteger_t MCCanvasEffectType_Measure(void)
{
	return sizeof(MCCanvasEffect);
}

void MCCanvasEffectType_Finalize(MCCanvasEffect *p_effect)
{
	MCCanvasColorDelete(p_effect->color);
}

void MCCanvasEffectType_Copy(MCCanvasEffect *p_src, MCCanvasEffect *p_dst)
{
	MCMemoryCopy(p_dst, p_src, sizeof(MCCanvasEffect));
	p_dst->color = nil;
	/* UNCHECKED */ MCCanvasColorCopy(p_src->color, p_dst->color);
}

// Canvas opaque type methods
// TODO - methods for "resource" type ?


////////////////////////////////////////////////////////////////////////////////

// Custom Types

void MCCanvasTypesInitialize();
void MCCanvasTypesFinalize();

MCTypeInfoRef kMCCanvasColorTypeInfo;
MCTypeInfoRef kMCCanvasTypeInfo;

////////////////////////////////////////////////////////////////////////////////

bool MCCanvasBlendModeFromString(MCStringRef p_string, MCGBlendMode &r_blend_mode);
bool MCCanvasBlendModeToString(MCGBlendMode p_blend_mode, MCStringRef &r_string);
bool MCCanvasEffectTypeToString(MCCanvasEffectType p_type, MCStringRef &r_string);
bool MCCanvasGradientTypeFromString(MCStringRef p_string, MCGGradientFunction &r_type);
bool MCCanvasGradientTypeToString(MCGGradientFunction p_type, MCStringRef &r_string);
bool MCCanvasFillRuleFromString(MCStringRef p_string, MCGFillRule &r_fill_rule);
bool MCCanvasFillRuleToString(MCGFillRule p_fill_rule, MCStringRef &r_string);
bool MCCanvasImageFilterFromString(MCStringRef p_string, MCGImageFilter &r_filter);
bool MCCanvasImageFilterToString(MCGImageFilter p_filter, MCStringRef &r_string);
bool MCCanvasPathCommandToString(MCGPathCommand p_command, MCStringRef &r_string);
bool MCCanvasPathCommandFromString(MCStringRef p_string, MCGPathCommand &r_command);

////////////////////////////////////////////////////////////////////////////////

static MCNameRef s_blend_mode_map[kMCGBlendModeCount];
static MCNameRef s_transform_matrix_keys[9];
static MCNameRef s_effect_type_map[_MCCanvasEffectTypeCount];
static MCNameRef s_effect_property_map[_MCCanvasEffectPropertyCount];
static MCNameRef s_gradient_type_map[kMCGGradientFunctionCount];
static MCNameRef s_canvas_fillrule_map[kMCGFillRuleCount];
static MCNameRef s_image_filter_map[kMCGImageFilterCount];
static MCNameRef s_path_command_map[kMCGPathCommandCount];

////////////////////////////////////////////////////////////////////////////////

void MCCanvasStringsInitialize();
void MCCanvasStringsFinalize();

void MCCanvasModuleInitialize()
{
	MCCanvasStringsInitialize();
	MCCanvasTypesInitialize();
}

void MCCanvasModuleFinalize()
{
	MCCanvasStringsFinalize();
	MCCanvasTypesFinalize();
}

////////////////////////////////////////////////////////////////////////////////

MCGColor MCCanvasColorToMCGColor(MCCanvasColorRef p_color)
{
	MCCanvasFloat t_red, t_green, t_blue, t_alpha;
	MCCanvasColorGetRGBA(p_color, t_red, t_green, t_blue, t_alpha);
	return MCGColorMakeRGBA(t_red, t_green, t_blue, t_alpha);
}

////////////////////////////////////////////////////////////////////////////////

// Rectangle

// Constructors

void MCCanvasRectangleMakeWithLTRB(MCCanvasFloat p_left, MCCanvasFloat p_top, MCCanvasFloat p_right, MCCanvasFloat p_bottom, MCCanvasRectangle &r_rect)
{
	r_rect = MCGRectangleMake(p_left, p_top, p_right - p_left, p_bottom - p_top);
}

// Properties

void MCCanvasRectangleGetLeft(const MCCanvasRectangle &p_rect, MCCanvasFloat &r_left)
{
	r_left = p_rect.origin.x;
}

void MCCanvasRectangleSetLeft(MCCanvasRectangle &x_rect, MCCanvasFloat p_left)
{
	x_rect.origin.x = p_left;
}

void MCCanvasRectangleGetTop(const MCCanvasRectangle &p_rect, MCCanvasFloat &r_top)
{
	r_top = p_rect.origin.y;
}

void MCCanvasRectangleSetTop(MCCanvasRectangle &x_rect, MCCanvasFloat p_top)
{
	x_rect.origin.y = p_top;
}

void MCCanvasRectangleGetRight(const MCCanvasRectangle &p_rect, MCCanvasFloat &r_right)
{
	r_right = p_rect.origin.x + p_rect.size.width;
}

void MCCanvasRectangleSetRight(MCCanvasRectangle &x_rect, MCCanvasFloat p_right)
{
	x_rect.origin.x = p_right - x_rect.size.width;
}

void MCCanvasRectangleGetBottom(const MCCanvasRectangle &p_rect, MCCanvasFloat &r_bottom)
{
	r_bottom = p_rect.origin.y + p_rect.size.height;
}

void MCCanvasRectangleSetBottom(MCCanvasRectangle &x_rect, MCCanvasFloat p_bottom)
{
	x_rect.origin.y = p_bottom - x_rect.size.height;
}

void MCCanvasRectangleGetWidth(const MCCanvasRectangle &p_rect, MCCanvasFloat &r_width)
{
	r_width = p_rect.size.width;
}

void MCCanvasRectangleSetWidth(MCCanvasRectangle &x_rect, MCCanvasFloat p_width)
{
	x_rect.size.width = p_width;
}

void MCCanvasRectangleGetHeight(const MCCanvasRectangle &p_rect, MCCanvasFloat &r_height)
{
	r_height = p_rect.size.height;
}

void MCCanvasRectangleSetHeight(MCCanvasRectangle &x_rect, MCCanvasFloat p_height)
{
	x_rect.size.height = p_height;
}

////////////////////////////////////////////////////////////////////////////////

// Point

// Constructors

void MCCanvasPointMake(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPoint &r_point)
{
	r_point = MCGPointMake(p_x, p_y);
}

// Properties

void MCCanvasPointGetX(const MCCanvasPoint &p_point, MCCanvasFloat &r_x)
{
	r_x = p_point.x;
}

void MCCanvasPointSetX(MCCanvasPoint &x_point, MCCanvasFloat p_x)
{
	x_point.x = p_x;
}

void MCCanvasPointGetY(const MCCanvasPoint &p_point, MCCanvasFloat &r_y)
{
	r_y = p_point.y;
}

void MCCanvasPointSetY(MCCanvasPoint &x_point, MCCanvasFloat p_y)
{
	x_point.y = p_y;
}

////////////////////////////////////////////////////////////////////////////////

// Color

// MCCanvasColorRef Type methods

struct __MCCanvasColorImpl
{
	MCCanvasFloat red, green, blue, alpha;
};

static void __MCCanvasColorDestroy(MCValueRef p_value)
{
	// no-op
}

static bool __MCCanvasColorCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	return true;
}

static bool __MCCanvasColorEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	__MCCanvasColorImpl *t_left, *t_right;
	t_left = (__MCCanvasColorImpl*)MCValueGetExtraBytesPtr(p_left);
	t_right = (__MCCanvasColorImpl*)MCValueGetExtraBytesPtr(p_right);
	
	return MCMemoryCompare(t_left, t_right, sizeof(__MCCanvasColorImpl)) == 0;
}

static inline uint64_t MCGColorMakeARGB16(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha)
{
	return (uint64_t)(p_alpha * UINT16_MAX) << 48 |
	(uint64_t)(p_red * UINT16_MAX) << 32 |
	(uint64_t)(p_green * UINT16_MAX) << 16 |
	(uint64_t)(p_blue * UINT16_MAX);
}

static hash_t __MCCanvasColorHash(MCValueRef p_value)
{
	MCCanvasFloat t_red, t_green, t_blue, t_alpha;
	MCCanvasColorGetRGBA((MCCanvasColorRef)p_value, t_red, t_green, t_blue, t_alpha);
	
	// return as ARGB16
	return MCGColorMakeARGB16(t_red, t_green, t_blue, t_alpha);
}

static bool __MCCanvasColorDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

//////////

bool MCCanvasColorCreateRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color)
{
	bool t_success;
	t_success = true;
	
	MCCanvasColorRef t_color;
	t_color = nil;
	
	MCCanvasColorRef t_unique_color;
	t_unique_color = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasColorTypeInfo, sizeof(__MCCanvasColorImpl), r_color);
	
	if (t_success)
	{
		__MCCanvasColorImpl *t_color_impl;
		t_color_impl = (__MCCanvasColorImpl*)MCValueGetExtraBytesPtr(r_color);
		t_color_impl->red = p_red;
		t_color_impl->green = p_green;
		t_color_impl->blue = p_blue;
		t_color_impl->alpha = p_alpha;
		
		t_success = MCValueInter(t_color, t_unique_color);
	}
	
	MCValueRelease(t_color);
	
	if (t_success)
		r_color = t_unique_color;
	
	return t_success;
}

MCCanvasFloat MCCanvasColorGetRed(MCCanvasColorRef color)
{
	__MCCanvasColorImpl *t_color;
	t_color = (__MCCanvasColorImpl*)MCValueGetExtraBytesPtr(color);
	
	return t_color->red;
}

MCCanvasFloat MCCanvasColorGetGreen(MCCanvasColorRef color)
{
	__MCCanvasColorImpl *t_color;
	t_color = (__MCCanvasColorImpl*)MCValueGetExtraBytesPtr(color);
	
	return t_color->green;
}

MCCanvasFloat MCCanvasColorGetBlue(MCCanvasColorRef color)
{
	__MCCanvasColorImpl *t_color;
	t_color = (__MCCanvasColorImpl*)MCValueGetExtraBytesPtr(color);
	
	return t_color->blue;
}

MCCanvasFloat MCCanvasColorGetAlpha(MCCanvasColorRef color)
{
	__MCCanvasColorImpl *t_color;
	t_color = (__MCCanvasColorImpl*)MCValueGetExtraBytesPtr(color);
	
	return t_color->alpha;
}

void MCCanvasColorGetRGBA(MCCanvasColorRef color, MCCanvasFloat &r_red, MCCanvasFloat &r_green, MCCanvasFloat &r_blue, MCCanvasFloat &r_alpha)
{
	r_red = MCCanvasColorGetRed(color);
	r_green = MCCanvasColorGetGreen(color);
	r_blue = MCCanvasColorGetBlue(color);
	r_alpha = MCCanvasColorGetAlpha(color);
}

void MCCanvasColorSetRGBA(MCCanvasColorRef &x_color, MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha)
{
	if (MCValueGetRetainCount(x_color) > 1)
	{
		// multiple references, create new color value
		MCCanvasColorRef t_newcolor;
		/* UNCHECKED */ MCCanvasColorCreateRGBA(p_red, p_green, p_blue, p_alpha, t_newcolor);
		
		MCValueRelease(x_color);
		x_color = t_newcolor;
		
		return;
	}
	
	__MCCanvasColorImpl *t_color;
	t_color = (__MCCanvasColorImpl*)MCValueGetExtraBytesPtr(x_color);
	t_color->red = p_red;
	t_color->green = p_green;
	t_color->blue = p_blue;
	t_color->alpha = p_alpha;
}

bool MCCanvasColorCopy(MCCanvasColorRef p_color, MCCanvasColorRef &r_copy)
{
	return MCValueCopy(p_color, (MCValueRef&)r_copy);
}

void MCCanvasColorDelete(MCCanvasColorRef p_color)
{
	MCValueRelease(p_color);
}

// Constructors

void MCCanvasColorMakeRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color)
{
	// TODO - throw color creation error
	/* UNCHECKED */ MCCanvasColorCreateRGBA(p_red, p_green, p_blue, p_alpha, r_color);
}

//////////

// Properties

void MCCanvasColorGetRed(MCCanvasColorRef p_color, MCCanvasFloat &r_red)
{
	r_red = MCCanvasColorGetRed(p_color);
}

void MCCanvasColorSetRed(MCCanvasColorRef &x_color, MCCanvasFloat p_red)
{
	MCCanvasFloat t_red, t_green, t_blue, t_alpha;
	MCCanvasColorGetRGBA(x_color, t_red, t_green, t_blue, t_alpha);
	if (t_red == p_red)
		return;
	
	MCCanvasColorSetRGBA(x_color, p_red, t_green, t_blue, t_alpha);
}

void MCCanvasColorGetGreen(MCCanvasColorRef p_color, MCCanvasFloat &r_green)
{
	r_green = MCCanvasColorGetGreen(p_color);
}

void MCCanvasColorSetGreen(MCCanvasColorRef &x_color, MCCanvasFloat p_green)
{
	MCCanvasFloat t_red, t_green, t_blue, t_alpha;
	MCCanvasColorGetRGBA(x_color, t_red, t_green, t_blue, t_alpha);
	if (t_green == p_green)
		return;
	
	MCCanvasColorSetRGBA(x_color, t_red, p_green, t_blue, t_alpha);
}

void MCCanvasColorGetBlue(MCCanvasColorRef p_color, MCCanvasFloat &r_blue)
{
	r_blue = MCCanvasColorGetBlue(p_color);
}

void MCCanvasColorSetBlue(MCCanvasColorRef &x_color, MCCanvasFloat p_blue)
{
	MCCanvasFloat t_red, t_green, t_blue, t_alpha;
	MCCanvasColorGetRGBA(x_color, t_red, t_green, t_blue, t_alpha);
	if (t_blue == p_blue)
		return;
	
	MCCanvasColorSetRGBA(x_color, t_red, t_green, p_blue, t_alpha);
}

void MCCanvasColorGetAlpha(MCCanvasColorRef p_color, MCCanvasFloat &r_alpha)
{
	r_alpha = MCCanvasColorGetAlpha(p_color);
}

void MCCanvasColorSetAlpha(MCCanvasColorRef &x_color, MCCanvasFloat p_alpha)
{
	MCCanvasFloat t_red, t_green, t_blue, t_alpha;
	MCCanvasColorGetRGBA(x_color, t_red, t_green, t_blue, t_alpha);
	if (t_alpha == p_alpha)
		return;
	
	MCCanvasColorSetRGBA(x_color, t_red, t_green, t_blue, p_alpha);
}

////////////////////////////////////////////////////////////////////////////////

// Transform

// Constructors

void MCCanvasTransformMakeIdentity(MCCanvasTransform &r_transform)
{
	r_transform = MCGAffineTransformMakeIdentity();
}

void MCCanvasTransformMakeScale(MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasTransform &r_transform)
{
	r_transform = MCGAffineTransformMakeScale(p_xscale, p_yscale);
}

void MCCanvasTransformMakeRotation(MCCanvasFloat p_angle, MCCanvasTransform &r_transform)
{
	r_transform = MCGAffineTransformMakeRotation(p_angle);
}

void MCCanvasTransformMakeTranslation(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransform &r_transform)
{
	r_transform = MCGAffineTransformMakeTranslation(p_x, p_y);
}

void MCCanvasTransformMakeSkew(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransform &r_transform)
{
	r_transform = MCGAffineTransformMakeSkew(p_x, p_y);
}

void MCCanvasTransformMakeWithMatrix(MCCanvasFloat p_a, MCCanvasFloat p_b, MCCanvasFloat p_c, MCCanvasFloat p_d, MCCanvasFloat p_tx, MCCanvasFloat p_ty, MCCanvasTransform &r_transform)
{
	r_transform = MCGAffineTransformMake(p_a, p_b, p_c, p_d, p_tx, p_ty);
}

//////////

// Properties

void MCCanvasTransformGetMatrix(const MCCanvasTransform &p_transform, MCArrayRef &r_matrix)
{
	bool t_success;
	t_success = true;
	
	MCArrayRef t_matrix;
	t_matrix = nil;
	
	if (t_success)
		t_success = MCArrayCreateMutable(t_matrix);
	
	if (t_success)
		t_success = MCArrayStoreReal(t_matrix, s_transform_matrix_keys[0], p_transform.a) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[1], p_transform.b) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[2], 0) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[3], p_transform.c) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[4], p_transform.d) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[5], 0) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[6], p_transform.tx) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[7], p_transform.ty) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[8], 1);
		
	if (t_success)
		r_matrix = t_matrix;
	else
		MCValueRelease(t_matrix);
	
	// return t_success;
}

void MCCanvasTransformSetMatrix(MCCanvasTransform &x_transform, MCArrayRef p_matrix)
{
	bool t_success;
	t_success = true;
	
	real64_t a, b, c, d, tx, ty;
	a = b = c = d = tx = ty = 0.0;
	t_success =
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[0], a) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[1], b) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[3], c) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[4], d) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[6], tx) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[7], ty);

	if (t_success)
		x_transform = MCGAffineTransformMake(a, b, c, d, tx, ty);
}

void MCCanvasTransformGetInverse(const MCCanvasTransform &p_transform, MCCanvasTransform &r_transform)
{
	r_transform = MCGAffineTransformInvert(p_transform);
}

// T = Tscale * Trotate * Tskew * Ttranslate

// Ttranslate:
// / 1 0 tx \
// | 0 1 ty |
// \ 0 0  1 /

// Tscale * Trotate * Tskew:
// / a  c \   / ScaleX       0 \   /  Cos(r)  Sin(r) \   / 1  Skew \   /  ScaleX * Cos(r)   ScaleX * Skew * Cos(r) + ScaleX * Sin(r) \
// \ b  d / = \      0  ScaleY / * \ -Sin(r)  Cos(r) / * \ 0     1 / = \ -ScaleY * Sin(r)  -ScaleY * Skew * Sin(r) + ScaleY * Cos(r) /

bool MCCanvasTransformDecompose(const MCCanvasTransform &p_transform, MCGSize &r_scale, MCCanvasFloat &r_rotation, MCGSize &r_skew, MCGSize &r_translation)
{
	MCGFloat t_r, t_skew;
	MCGSize t_scale, t_trans;
	
	t_trans = MCGSizeMake(p_transform.tx, p_transform.ty);
	
	// if a == 0, take r to be pi/2 radians
	if (p_transform.a == 0)
	{
		t_r = M_PI / 2;
		// b == -ScaleY, c == ScaleX
		t_scale = MCGSizeMake(p_transform.c, -p_transform.b);
		// d = -ScaleY * Skew => Skew = d / -ScaleY => Skew = d / b
		if (p_transform.b == 0)
			return false;
		t_skew = p_transform.d / p_transform.b;
	}
	// if b == 0, take r to be 0 radians
	else if (p_transform.b == 0)
	{
		t_r = 0;
		// a == ScaleX, d == ScaleY
		t_scale = MCGSizeMake(p_transform.a, p_transform.d);
		// c == ScaleX * Skew => Skew == c / ScaleX => Skew = c / a
		if (p_transform.a == 0)
			return false;
		t_skew = p_transform.c / p_transform.a;
	}
	else
	{
		// Skew^2 + (-c/a - d/b) * Skew + (c/a * d/b - 1) = 0
		MCGFloat t_c_div_a, t_d_div_b;
		t_c_div_a = p_transform.c / p_transform.a;
		t_d_div_b = p_transform.d / p_transform.b;
		
		MCGFloat t_x1, t_x2;
		if (!MCSolveQuadraticEqn(1, -t_c_div_a -t_d_div_b, t_c_div_a * t_d_div_b - 1, t_x1, t_x2))
			return false;
		
		// choose skew with smallest absolute value
		if (MCAbs(t_x1) < MCAbs(t_x2))
			t_skew = t_x1;
		else
			t_skew = t_x2;
		
		// Tan(r) = c/a -Skew
		t_r = atan(t_c_div_a - t_skew);
		
		t_scale = MCGSizeMake(p_transform.a / cosf(t_r), -p_transform.b / sinf(t_r));
	}
	
	r_scale = t_scale;
	r_rotation = t_r;
	r_skew = MCGSizeMake(t_skew, 0);
	r_translation = t_trans;
	
	return true;
}

bool MCCanvasTransformCompose(const MCGSize &p_scale, MCCanvasFloat p_rotation, const MCGSize &p_skew, const MCGSize &p_translation, MCCanvasTransform &r_transform)
{
	MCCanvasTransform t_transform;
	MCCanvasTransformMakeScale(p_scale.width, p_scale.height, t_transform);
	MCCanvasTransformRotate(t_transform, p_rotation);
	MCCanvasTransformSkew(t_transform, p_skew.width, p_skew.height);
	MCCanvasTransformTranslate(t_transform, p_translation.width, p_translation.height);
	
	r_transform = t_transform;
	return true;
}

void MCCanvasTransformGetScale(const MCCanvasTransform &p_transform, MCGSize &r_scale)
{
	MCGSize t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (MCCanvasTransformDecompose(p_transform, t_scale, t_rotation, t_skew, t_translation))
		r_scale = t_scale;
}

void MCCanvasTransformSetScale(MCCanvasTransform &x_transform, const MCGSize &p_scale)
{
	MCGSize t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	MCCanvasTransform t_transform;
	
	if (MCCanvasTransformDecompose(x_transform, t_scale, t_rotation, t_skew, t_translation) &&
		MCCanvasTransformCompose(p_scale, t_rotation, t_skew, t_translation, t_transform))
		x_transform = t_transform;
}

void MCCanvasTransformGetRotation(const MCCanvasTransform &p_transform, MCCanvasFloat &r_rotation)
{
	MCGSize t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (MCCanvasTransformDecompose(p_transform, t_scale, t_rotation, t_skew, t_translation))
		r_rotation = t_rotation;
}

void MCCanvasTransformSetRotation(MCCanvasTransform &x_transform, MCCanvasFloat p_rotation)
{
	MCGSize t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	MCCanvasTransform t_transform;
	
	if (MCCanvasTransformDecompose(x_transform, t_scale, t_rotation, t_skew, t_translation) &&
		MCCanvasTransformCompose(t_scale, p_rotation, t_skew, t_translation, t_transform))
		x_transform = t_transform;
}

void MCCanvasTransformGetSkew(const MCCanvasTransform &p_transform, MCGSize &r_skew)
{
	MCGSize t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (MCCanvasTransformDecompose(p_transform, t_scale, t_rotation, t_skew, t_translation))
		r_skew = t_skew;
}

void MCCanvasTransformSetSkew(MCCanvasTransform &x_transform, const MCGSize &p_skew)
{
	MCGSize t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	MCCanvasTransform t_transform;
	
	if (MCCanvasTransformDecompose(x_transform, t_scale, t_rotation, t_skew, t_translation) &&
		MCCanvasTransformCompose(t_scale, t_rotation, p_skew, t_translation, t_transform))
		x_transform = t_transform;
}

void MCCanvasTransformGetTranslation(const MCCanvasTransform &p_transform, MCGSize &r_translation)
{
	MCGSize t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (MCCanvasTransformDecompose(p_transform, t_scale, t_rotation, t_skew, t_translation))
		r_translation = t_translation;
}

void MCCanvasTransformSetTranslation(MCCanvasTransform &x_transform, const MCGSize &p_translation)
{
	MCGSize t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	MCCanvasTransform t_transform;
	
	if (MCCanvasTransformDecompose(x_transform, t_scale, t_rotation, t_skew, t_translation) &&
		MCCanvasTransformCompose(t_scale, t_rotation, t_skew, p_translation, t_transform))
		x_transform = t_transform;
}

//////////

// Operations

void MCCanvasTransformConcat(MCCanvasTransform &x_transform_a, const MCCanvasTransform p_transform_b)
{
	x_transform_a = MCGAffineTransformConcat(x_transform_a, p_transform_b);
}

void MCCanvasTransformScale(MCCanvasTransform &x_transform, MCCanvasFloat p_x_scale, MCCanvasFloat p_y_scale)
{
	x_transform = MCGAffineTransformScale(x_transform, p_x_scale, p_y_scale);
}

void MCCanvasTransformRotate(MCCanvasTransform &x_transform, MCCanvasFloat p_rotation)
{
	x_transform = MCGAffineTransformRotate(x_transform, p_rotation);
}

void MCCanvasTransformTranslate(MCCanvasTransform &x_transform, MCCanvasFloat p_dx, MCCanvasFloat p_dy)
{
	x_transform = MCGAffineTransformTranslate(x_transform, p_dx, p_dy);
}

void MCCanvasTransformSkew(MCCanvasTransform &x_transform, MCCanvasFloat p_xskew, MCCanvasFloat p_yskew)
{
	x_transform = MCGAffineTransformSkew(x_transform, p_xskew, p_yskew);
}

////////////////////////////////////////////////////////////////////////////////

// Image

// Constructors

void MCCanvasImageMakeWithPath(MCStringRef p_path, MCCanvasImage &x_image)
{
	/* UNCHECKED */ MCImageRepCreateWithPath(p_path, x_image);
}

void MCCanvasImageMakeWithData(MCDataRef p_data, MCCanvasImage &x_image)
{
	/* UNCHECKED */ MCImageRepCreateWithData(p_data, x_image);
}

void MCCanvasImageMakeWithPixels(integer_t p_width, integer_t p_height, MCDataRef p_pixels, MCCanvasImage &x_image)
{
	// Input should be unpremultiplied ARGB pixels
	/* UNCHECKED */ MCImageRepCreateWithPixels(p_pixels, p_width, p_height, kMCGPixelFormatARGB, false, x_image);
}

// Properties

void MCCanvasImageGetWidth(const MCCanvasImage &p_image, uint32_t &r_width)
{
	uint32_t t_width, t_height;
	/* UNCHECKED */ MCImageRepGetGeometry(p_image, t_width, t_height);
	r_width = t_width;
}

void MCCanvasImageGetHeight(const MCCanvasImage &p_image, uint32_t &r_height)
{
	uint32_t t_width, t_height;
	/* UNCHECKED */ MCImageRepGetGeometry(p_image, t_width, t_height);
	r_height = t_height;
}

void MCCanvasImageGetPixels(const MCCanvasImage &p_image, MCDataRef &r_pixels)
{
	MCImageBitmap *t_raster;
	
	// TODO - handle case of missing normal density image
	// TODO - throw appropriate errors
	if (MCImageRepLockRaster(p_image, 0, 1.0, t_raster))
	{
		uint8_t *t_buffer;
		t_buffer = nil;
		
		uint32_t t_buffer_size;
		t_buffer_size = t_raster->height * t_raster->stride;
		
		/* UNCHECKED */ MCMemoryAllocate(t_buffer_size, t_buffer);
		
		uint8_t *t_pixel_row;
		t_pixel_row = t_buffer;
		
		for (uint32_t y = 0; y < t_raster->height; y++)
		{
			uint32_t *t_pixel_ptr;
			t_pixel_ptr = (uint32_t*)t_pixel_row;
			
			for (uint32_t x = 0; x < t_raster->width; x++)
			{
				*t_pixel_ptr = MCGPixelFromNative(kMCGPixelFormatARGB, *t_pixel_ptr);
				t_pixel_ptr++;
			}
			
			t_pixel_row += t_raster->stride;
		}
		
		/* UNCHECKED */ MCDataCreateWithBytesAndRelease(t_buffer, t_buffer_size, r_pixels);
		
		MCImageRepUnlockRaster(p_image, 0, t_raster);
	}
}

void MCCanvasImageGetMetadata(const MCCanvasImage &p_image, MCArrayRef &r_metadata)
{
	// TODO - implement image metadata
}

////////////////////////////////////////////////////////////////////////////////

// Solid Paint

// Constructor

void MCCanvasSolidPaintCreateWithColor(MCCanvasColorRef p_color, MCCanvasSolidPaint &x_paint)
{
	/* UNCHECKED */ MCMemoryNew(x_paint);
	x_paint->type = kMCCanvasPaintTypeSolid;
	/* UNCHECKED */ MCCanvasColorCopy(p_color, x_paint->color);
}

// Properties

void MCCanvasSolidPaintGetColor(const MCCanvasSolidPaint &p_paint, MCCanvasColorRef &r_color)
{
	/* UNCHECKED */ MCCanvasColorCopy(p_paint->color, r_color);
	r_color = p_paint->color;
}

void MCCanvasSolidPaintSetColor(MCCanvasSolidPaint &x_paint, MCCanvasColorRef p_color)
{
	MCCanvasColorRef t_color;
	/* UNCHECKED */ MCCanvasColorCopy(p_color, t_color);
	MCCanvasColorDelete(x_paint->color);
	x_paint->color = t_color;
}

////////////////////////////////////////////////////////////////////////////////

// Pattern

// Constructor

void MCCanvasPatternMakeWithTransformedImage(const MCCanvasImage &p_image, const MCCanvasTransform &p_transform, MCCanvasPattern &r_pattern)
{
	/* UNCHECKED */ MCMemoryNew(r_pattern);
	r_pattern->type = kMCCanvasPaintTypePattern;
	r_pattern->image = MCImageRepRetain(p_image);
	r_pattern->transform = p_transform;
}

void MCCanvasPatternMakeWithImage(const MCCanvasImage &p_image, MCCanvasPattern &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeIdentity(), r_pattern);
}

void MCCanvasPatternMakeWithScaledImage(const MCCanvasImage &p_image, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasPattern &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeScale(p_xscale, p_yscale), r_pattern);
}

void MCCanvasPatternMakeWithRotatedImage(const MCCanvasImage &p_image, MCCanvasFloat p_angle, MCCanvasPattern &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeRotation(p_angle), r_pattern);
}

void MCCanvasPatternMakeWithTranslatedImage(const MCCanvasImage &p_image, MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPattern &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeTranslation(p_x, p_y), r_pattern);
}

// Properties

void MCCanvasPatternGetImage(const MCCanvasPattern &p_pattern, MCCanvasImage &r_image)
{
	r_image = MCImageRepRetain(p_pattern->image);
}

void MCCanvasPatternSetImage(MCCanvasPattern &x_pattern, const MCCanvasImage &p_image)
{
	MCCanvasImage t_rep;
	t_rep = MCImageRepRetain(p_image);
	MCImageRepRelease(x_pattern->image);
	x_pattern->image = t_rep;
}

void MCCanvasPatternGetTransform(const MCCanvasPattern &p_pattern, MCCanvasTransform &r_transform)
{
	r_transform = p_pattern->transform;
}

void MCCanvasPatternSetTransform(MCCanvasPattern &x_pattern, const MCCanvasTransform &p_transform)
{
	x_pattern->transform = p_transform;
}

// Operators

void MCCanvasPatternTransform(MCCanvasPattern &x_pattern, const MCCanvasTransform &p_transform)
{
	MCCanvasTransformConcat(x_pattern->transform, p_transform);
}

void MCCanvasPatternScale(MCCanvasPattern &x_pattern, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

void MCCanvasPatternRotate(MCCanvasPattern &x_pattern, MCCanvasFloat p_angle)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeRotation(p_angle));
}

void MCCanvasPatternTranslate(MCCanvasPattern &x_pattern, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeTranslation(p_x, p_y));
}

////////////////////////////////////////////////////////////////////////////////

// Gradient

bool MCCanvasGradientCheckStopOrder(MCCanvasGradientStop *p_ramp, uint32_t p_length)
{
	for (uint32_t i = 1; i < p_length; i++)
	{
		if (p_ramp[i].offset < p_ramp[i - 1].offset)
		{
			// TODO - throw stop offset order error
			return false;
		}
	}
	
	return true;
}

// Constructor

void MCCanvasGradientMakeWithRamp(integer_t p_type, MCCArray<MCCanvasGradientStop> &p_ramp, MCCanvasGradient &r_gradient)
{
	bool t_success;
	t_success = true;
	
	MCCanvasGradientStop *t_stops;
	t_stops = nil;
	
	uint32_t t_stop_count;
	t_stop_count = 0;
	
	if (t_success)
		t_success = MCCanvasGradientCheckStopOrder(p_ramp.data, p_ramp.length);
	
	if (t_success)
	{
		t_stop_count = p_ramp.length;
		t_success = MCCanvasGradientRampCopy(p_ramp.data, p_ramp.length, t_stops);
	}
	
	if (t_success)
		t_success = MCMemoryNew(r_gradient);
	
	if (t_success)
	{
		r_gradient->type = kMCCanvasPaintTypeGradient;

		r_gradient->function = (MCGGradientFunction)p_type;
		r_gradient->ramp = t_stops;
		r_gradient->ramp_length = t_stop_count;
		r_gradient->filter = kMCGImageFilterNone;
		r_gradient->mirror = false;
		r_gradient->repeats = 1;
		r_gradient->wrap = false;
		r_gradient->transform = MCGAffineTransformMakeIdentity();
	}
	else
	{
		// TODO - throw memory error
		MCCanvasGradientRampDelete(t_stops, t_stop_count);
	}
}

// Properties

void MCCanvasGradientGetRamp(const MCCanvasGradient &p_gradient, MCCArray<MCCanvasGradientStop> &r_ramp)
{
	if (!MCCanvasGradientRampCopy(p_gradient->ramp, p_gradient->ramp_length, r_ramp.data))
	{
		// TODO - throw memory error
		return;
	}
	
	r_ramp.length = p_gradient->ramp_length;
}
void MCCanvasGradientSetRamp(MCCanvasGradient &x_gradient, const MCCArray<MCCanvasGradientStop> &p_ramp)
{
	if (!MCCanvasGradientCheckStopOrder(p_ramp.data, p_ramp.length))
		return;
	
	MCCanvasGradientStop *t_ramp;
	t_ramp = nil;
	
	if (!MCCanvasGradientRampCopy(p_ramp.data, p_ramp.length, t_ramp))
	{
		// TODO - throw memory error
		return;
	}
	
	MCCanvasGradientRampDelete(x_gradient->ramp, x_gradient->ramp_length);
	x_gradient->ramp = t_ramp;
	x_gradient->ramp_length = p_ramp.length;
}

void MCCanvasGradientGetTypeAsString(const MCCanvasGradient &p_gradient, MCStringRef &r_string)
{
	/* UNCHECKED */ MCCanvasGradientTypeToString(p_gradient->function, r_string);
}

void MCCanvasGradientSetTypeAsString(MCCanvasGradient &x_gradient, MCStringRef p_string)
{
	/* UNCHECKED */ MCCanvasGradientTypeFromString(p_string, x_gradient->function);
}

void MCCanvasGradientGetRepeat(const MCCanvasGradient &p_gradient, integer_t &r_repeat)
{
	r_repeat = p_gradient->repeats;
}

void MCCanvasGradientSetRepeat(MCCanvasGradient &x_gradient, integer_t p_repeat)
{
	x_gradient->repeats = p_repeat;
}

void MCCanvasGradientGetWrap(const MCCanvasGradient &p_gradient, bool &r_wrap)
{
	r_wrap = p_gradient->wrap;
}

void MCCanvasGradientSetWrap(MCCanvasGradient &x_gradient, bool p_wrap)
{
	x_gradient->wrap = p_wrap;
}

void MCCanvasGradientGetMirror(const MCCanvasGradient &p_gradient, bool &r_mirror)
{
	r_mirror = p_gradient->mirror;
}

void MCCanvasGradientSetMirror(MCCanvasGradient &x_gradient, bool p_mirror)
{
	x_gradient->mirror = p_mirror;
}

void MCCanvasGradientTransformToPoints(const MCCanvasTransform &p_transform, MCCanvasPoint &r_from, MCCanvasPoint &r_to, MCCanvasPoint &r_via)
{
	r_from = MCGPointApplyAffineTransform(MCGPointMake(0, 0), p_transform);
	r_to = MCGPointApplyAffineTransform(MCGPointMake(0, 1), p_transform);
	r_via = MCGPointApplyAffineTransform(MCGPointMake(1, 0), p_transform);
}

bool MCCanvasGradientTransformFromPoints(const MCCanvasPoint &p_from, const MCCanvasPoint &p_to, const MCCanvasPoint &p_via, MCCanvasTransform &r_transform)
{
	MCGPoint t_src[3], t_dst[3];
	t_src[0] = MCGPointMake(0, 0);
	t_src[1] = MCGPointMake(0, 1);
	t_src[2] = MCGPointMake(1, 0);
	
	t_dst[0] = p_from;
	t_dst[1] = p_to;
	t_dst[2] = p_via;
	
	return MCGAffineTransformFromPoints(t_src, t_dst, r_transform);
}

void MCCanvasGradientGetPoints(const MCCanvasGradient &p_gradient, MCCanvasPoint &r_from, MCCanvasPoint &r_to, MCCanvasPoint &r_via)
{
	MCCanvasGradientTransformToPoints(p_gradient->transform, r_from, r_to, r_via);
}
void MCCanvasGradientSetPoints(MCCanvasGradient &x_gradient, const MCCanvasPoint &p_from, const MCCanvasPoint &p_to, const MCCanvasPoint &p_via)
{
	if (!MCCanvasGradientTransformFromPoints(p_from, p_to, p_via, x_gradient->transform))
	{
		// TODO - throw error
	}
}

void MCCanvasGradientGetFrom(const MCCanvasGradient &p_gradient, MCCanvasPoint &r_from)
{
	MCCanvasPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	r_from = t_from;
}

void MCCanvasGradientGetTo(const MCCanvasGradient &p_gradient, MCCanvasPoint &r_to)
{
	MCCanvasPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	r_to = t_to;
}

void MCCanvasGradientGetVia(const MCCanvasGradient &p_gradient, MCCanvasPoint &r_via)
{
	MCCanvasPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	r_via = t_via;
}

void MCCanvasGradientSetFrom(MCCanvasGradient &x_gradient, const MCCanvasPoint &p_from)
{
	MCCanvasPoint t_from, t_to, t_via;
	MCCanvasGradientTransformToPoints(x_gradient->transform, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, p_from, t_to, t_via);
}

void MCCanvasGradientSetTo(MCCanvasGradient &x_gradient, const MCCanvasPoint &p_to)
{
	MCCanvasPoint t_from, t_to, t_via;
	MCCanvasGradientTransformToPoints(x_gradient->transform, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, t_from, p_to, t_via);
}

void MCCanvasGradientSetVia(MCCanvasGradient &x_gradient, const MCCanvasPoint &p_via)
{
	MCCanvasPoint t_from, t_to, t_via;
	MCCanvasGradientTransformToPoints(x_gradient->transform, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, t_from, t_to, p_via);
}

void MCCanvasGradientGetTransform(const MCCanvasGradient &p_gradient, MCCanvasTransform &r_transform)
{
	r_transform = p_gradient->transform;
}

void MCCanvasGradientSetTransform(MCCanvasGradient &x_gradient, const MCCanvasTransform &p_transform)
{
	x_gradient->transform = p_transform;
}

// Operators

void MCCanvasGradientAddStop(MCCanvasGradient &x_gradient, const MCCanvasGradientStop &p_stop)
{
	if (p_stop.offset < 0 || p_stop.offset > 1)
	{
		// TODO - throw offset range error
		return;
	}
	
	if (x_gradient->ramp_length > 0 && p_stop.offset < x_gradient->ramp[x_gradient->ramp_length - 1].offset)
	{
		// TODO - throw stop order error
		return;
	}
	
	if (!MCMemoryResizeArray(x_gradient->ramp_length + 1, x_gradient->ramp, x_gradient->ramp_length))
	{
		// TODO - throw memory error
		return;
	}
	
	if (!MCCanvasGradientStopCopy(p_stop, x_gradient->ramp[x_gradient->ramp_length - 1]))
	{
		// TODO - throw gradient stop copy error
		x_gradient->ramp_length--;
		return;
	}
}

void MCCanvasGradientTransform(MCCanvasGradient &x_gradient, const MCCanvasTransform &p_transform)
{
	MCCanvasTransformConcat(x_gradient->transform, p_transform);
}

void MCCanvasGradientScale(MCCanvasGradient &x_gradient, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

void MCCanvasGradientRotate(MCCanvasGradient &x_gradient, MCCanvasFloat p_angle)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeRotation(p_angle));
}

void MCCanvasGradientTranslate(MCCanvasGradient &x_gradient, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeTranslation(p_x, p_y));
}

////////////////////////////////////////////////////////////////////////////////

// Path

bool MCCanvasPathParseInstructions(MCStringRef p_instructions, MCGPathIterateCallback p_callback, void *p_context);
bool MCCanvasPathUnparseInstructions(const MCCanvasPath &p_path, MCStringRef &r_string);

// Constructors

bool MCCanvasPathMakeWithInstructionsCallback(void *p_context, MCGPathCommand p_command, MCGPoint *p_points, uint32_t p_point_count)
{
	MCGPathRef t_path;
	t_path = static_cast<MCGPathRef>(p_context);
	
	switch (p_command)
	{
		case kMCGPathCommandMoveTo:
			MCGPathMoveTo(t_path, p_points[0]);
			break;
			
		case kMCGPathCommandLineTo:
			MCGPathLineTo(t_path, p_points[0]);
			break;
			
		case kMCGPathCommandQuadCurveTo:
			MCGPathQuadraticTo(t_path, p_points[0], p_points[1]);
			break;
			
		case kMCGPathCommandCubicCurveTo:
			MCGPathCubicTo(t_path, p_points[0], p_points[1], p_points[2]);
			break;
			
		case kMCGPathCommandCloseSubpath:
			MCGPathCloseSubpath(t_path);
			break;
			
		case kMCGPathCommandEnd:
			break;
			
		default:
			MCAssert(false);
	}
	
	return MCGPathIsValid(t_path);
}

void MCCanvasPathMakeWithInstructionsAsString(MCStringRef p_instructions, MCCanvasPath &r_path)
{
	/* UNCHECKED */ MCGPathCreateMutable(r_path.path);
	/* UNCHECKED */ MCCanvasPathParseInstructions(p_instructions, MCCanvasPathMakeWithInstructionsCallback, r_path.path);
}

void MCCanvasPathMakeWithRoundedRectangle(const MCCanvasRectangle &p_rect, MCCanvasFloat p_x_radius, MCCanvasFloat p_y_radius, MCCanvasPath &r_path)
{
	/* UNCHECKED */ MCGPathCreateMutable(r_path.path);
	/* UNCHECKED */ MCGPathAddRoundedRectangle(r_path.path, p_rect, MCGSizeMake(p_x_radius, p_y_radius));
}

void MCCanvasPathMakeWithRectangle(const MCCanvasRectangle &p_rect, MCCanvasPath &r_path)
{
	/* UNCHECKED */ MCGPathCreateMutable(r_path.path);
	/* UNCHECKED */ MCGPathAddRectangle(r_path.path, p_rect);
}

void MCCanvasPathMakeWithEllipse(const MCCanvasPoint &p_center, MCCanvasFloat p_radius_x, MCCanvasFloat p_radius_y, MCCanvasPath &r_path)
{
	/* UNCHECKED */ MCGPathCreateMutable(r_path.path);
	/* UNCHECKED */ MCGPathAddEllipse(r_path.path, p_center, MCGSizeMake(p_radius_x, p_radius_y), 0);
}

void MCCanvasPathMakeWithLine(const MCCanvasPoint &p_start, const MCCanvasPoint &p_end, MCCanvasPath &r_path)
{
	/* UNCHECKED */ MCGPathCreateMutable(r_path.path);
	/* UNCHECKED */ MCGPathAddLine(r_path.path, p_start, p_end);
}

void MCCanvasPathMakeWithPoints(const MCCArray<MCCanvasPoint> &p_points, bool p_close, MCCanvasPath &r_path)
{
	/* UNCHECKED */ MCGPathCreateMutable(r_path.path);
	if (p_close)
		/* UNCHECKED */ MCGPathAddPolygon(r_path.path, p_points.data, p_points.length);
	else
		/* UNCHECKED */ MCGPathAddPolyline(r_path.path, p_points.data, p_points.length);
}

// Properties

void MCCanvasPathGetSubpaths(MCCanvasPath &p_path, integer_t p_start, integer_t p_end, MCCanvasPath &r_subpaths)
{
	MCGPathRef t_path;
	t_path = nil;
	if (!MCGPathMutableCopySubpaths(p_path.path, p_start, p_end, t_path))
	{
		// TODO - throw error
		return;
	}
	
	r_subpaths.path = t_path;
}

void MCCanvasPathGetBoundingBox(MCCanvasPath &p_path, MCCanvasRectangle &r_bounds)
{
	/* UNCHECKED */ MCGPathGetBoundingBox(p_path.path, r_bounds);
}

void MCCanvasPathGetInstructionsAsString(const MCCanvasPath &p_path, MCStringRef &r_instruction_string)
{
	/* UNCHECKED */ MCCanvasPathUnparseInstructions(p_path, r_instruction_string);
}

// Operations

void MCCanvasPathTransform(MCCanvasPath &x_path, const MCCanvasTransform &p_transform)
{
	// Path transformations are applied immediately
	/* UNCHECKED */ MCGPathTransform(x_path.path, p_transform);
}

void MCCanvasPathScale(MCCanvasPath &x_path, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

void MCCanvasPathRotate(MCCanvasPath &x_path, MCCanvasFloat p_angle)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeRotation(p_angle));
}

void MCCanvasPathTranslate(MCCanvasPath &x_path, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeTranslation(p_x, p_y));
}

void MCCanvasPathAddPath(MCCanvasPath &x_path, const MCCanvasPath &p_to_add)
{
	/* UNCHECKED */ MCGPathAddPath(x_path.path, p_to_add.path);
}

void MCCanvasPathMoveTo(MCCanvasPath &x_path, const MCCanvasPoint &p_point)
{
	/* UNCHECKED */ MCGPathMoveTo(x_path.path, p_point);
}

void MCCanvasPathLineTo(MCCanvasPath &x_path, const MCCanvasPoint &p_point)
{
	/* UNCHECKED */ MCGPathLineTo(x_path.path, p_point);
}

void MCCanvasPathCurveThroughPoint(MCCanvasPath &x_path, const MCCanvasPoint &p_through, const MCCanvasPoint &p_to)
{
	/* UNCHECKED */ MCGPathQuadraticTo(x_path.path, p_through, p_to);
}

void MCCanvasPathCurveThroughPoints(MCCanvasPath &x_path, const MCCanvasPoint &p_through_a, const MCCanvasPoint &p_through_b, const MCCanvasPoint &p_to)
{
	/* UNCHECKED */ MCGPathCubicTo(x_path.path, p_through_a, p_through_b, p_to);
}

void MCCanvasPathClosePath(MCCanvasPath &x_path)
{
	/* UNCHECKED */ MCGPathCloseSubpath(x_path.path);
}

//////////

bool MCCanvasPathUnparseInstructionsCallback(void *p_context, MCGPathCommand p_command, MCGPoint *p_points, uint32_t p_point_count)
{
	MCStringRef t_string;
	t_string = static_cast<MCStringRef>(p_context);
	
	MCStringRef t_command_string;
	t_command_string = nil;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = MCCanvasPathCommandToString(p_command, t_command_string);
	
	if (t_success)
		t_success = MCStringAppend(t_string, t_command_string);
	
	for (uint32_t i = 0; t_success && i < p_point_count; i++)
		t_success = MCStringAppendFormat(t_string, " %f,%f", p_points[i].x, p_points[i].y);
	
	if (t_success)
		t_success = MCStringAppendChar(t_string, '\n');
	
	return t_success;
}

bool MCCanvasPathUnparseInstructions(const MCCanvasPath &p_path, MCStringRef &r_string)
{
	if (MCGPathIsEmpty(p_path.path))
		return MCStringCopy(kMCEmptyString, r_string);
	
	bool t_success;
	t_success = true;
	
	MCStringRef t_string;
	t_string = nil;
	
	if (t_success)
		t_success = MCStringCreateMutable(0, t_string);
	
	if (t_success)
		t_success = MCGPathIterate(p_path.path, MCCanvasPathUnparseInstructionsCallback, t_string);
	
	if (t_success)
		t_success = MCStringCopyAndRelease(t_string, r_string);
	
	if (!t_success)
		MCValueRelease(t_string);
	
	return t_success;
}

bool MCCanvasPathParseInstructions(MCStringRef p_instructions, MCGPathIterateCallback p_callback, void *p_context)
{
	bool t_success;
	t_success = true;
	
	if (MCStringIsEmpty(p_instructions))
	{
		t_success = p_callback(p_context, kMCGPathCommandEnd, nil, 0);
		return t_success;
	}
	
	MCProperListRef t_lines;
	t_lines = nil;

	MCGPathCommand t_command;
	t_command = kMCGPathCommandEnd;
	
	MCGPoint t_points[3];
	uint32_t t_point_count;
	t_point_count = 0;
	
	bool t_ended;
	t_ended = false;

	if (t_success)
		t_success = MCStringSplitByDelimiter(p_instructions, kMCLineEndString, kMCStringOptionCompareExact, t_lines);
	
	for (uint32_t i = 0; t_success && i < MCProperListGetLength(t_lines); i++)
	{
		MCStringRef t_line;
		t_line = (MCStringRef)MCProperListFetchElementAtIndex(t_lines, i);
		
		if (MCStringIsEmpty(t_line))
			continue;
		
		if (t_ended)
			// TODO - throw error - instruction after end
			t_success = false;
		
		MCProperListRef t_components;
		t_components = nil;
		
		if (t_success)
			t_success = MCStringSplitByDelimiter(t_line, MCSTR(" "), kMCStringOptionCompareExact, t_components);
		
		if (t_success)
		{
			MCStringRef t_command_string;
			t_command_string = (MCStringRef)MCProperListFetchElementAtIndex(t_components, 0);
			t_success = MCCanvasPathCommandFromString(t_command_string, t_command);
		}
		
		if (t_success)
		{
			switch (t_command)
			{
				case kMCGPathCommandMoveTo:
					t_point_count = 1;
					t_success = MCProperListGetLength(t_components) == 2 &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 1), t_points[0]);
					// TODO - throw parse error
					break;
					
				case kMCGPathCommandLineTo:
					t_point_count = 1;
					t_success = MCProperListGetLength(t_components) == 2 &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 1), t_points[0]);
					// TODO - throw parse error
					break;
					
				case kMCGPathCommandQuadCurveTo:
					t_point_count = 2;
					t_success = MCProperListGetLength(t_components) == 3 &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 1), t_points[0]) &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 2), t_points[1]);
					// TODO - throw parse error
					break;
					
				case kMCGPathCommandCubicCurveTo:
					t_point_count = 3;
					t_success = MCProperListGetLength(t_components) == 4 &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 1), t_points[0]) &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 2), t_points[1]) &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 3), t_points[2]);
					// TODO - throw parse error
					break;
					
				case kMCGPathCommandCloseSubpath:
					t_point_count = 0;
					t_success = MCProperListGetLength(t_components) == 1;
					// TODO - throw parse error
					break;
					
				case kMCGPathCommandEnd:
					t_point_count = 0;
					t_success = MCProperListGetLength(t_components) == 1;
					t_ended = true;
					// TODO - throw parse error
					break;
					
				default:
					MCAssert(false);
			}
		}
		
		MCValueRelease(t_components);
		
		if (t_success)
			t_success = p_callback(p_context, t_command, t_points, t_point_count);
	}
	
	MCValueRelease(t_lines);

	if (t_success && !t_ended)
		t_success = p_callback(p_context, kMCGPathCommandEnd, nil, 0);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// Effect

// Constructors

void MCCanvasGraphicEffectMakeWithPropertyArray(integer_t p_type, MCArrayRef p_properties, MCCanvasEffect &r_effect)
{
	// TODO - defaults for missing properties?
	bool t_success;
	t_success = true;
	
	MCCanvasEffect t_effect;
	t_effect.type = (MCCanvasEffectType)p_type;
	
	real64_t t_opacity;
	
	if (t_success)
		t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertyOpacity], t_opacity);
	
	MCStringRef t_blend_mode;
	t_blend_mode = nil;
	
	// Blend Mode
	if (t_success)
		t_success = MCArrayFetchString(p_properties, s_effect_property_map[kMCCanvasEffectPropertyBlendMode], t_blend_mode) &&
		MCCanvasBlendModeFromString(t_blend_mode, t_effect.blend_mode);
	
	MCCanvasColorRef t_color;
	t_color = nil;
	
	if (t_success)
		t_success = MCArrayFetchCanvasColor(p_properties, s_effect_property_map[kMCCanvasEffectPropertyColor], t_color) &&
		MCCanvasColorCopy(t_color, t_effect.color);
	
	// read properties for each effect type
	switch (t_effect.type)
	{
		case kMCCanvasEffectTypeColorOverlay:
			// no other properties
			break;
			
		case kMCCanvasEffectTypeInnerShadow:
		case kMCCanvasEffectTypeOuterShadow:
		{
			real64_t t_distance, t_angle;
			real64_t t_size, t_spread;
			// distance
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertyDistance], t_distance);
			// angle
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertyAngle], t_angle);
			// size
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertySize], t_size);
			// spread
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertySpread], t_spread);
			
			t_effect.distance = t_distance;
			t_effect.angle = t_angle;
			t_effect.size = t_size;
			t_effect.spread = t_spread;
			
			break;
		}
			
		case kMCCanvasEffectTypeInnerGlow:
		case kMCCanvasEffectTypeOuterGlow:
		{
			real64_t t_size, t_spread;
			// size
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertySize], t_size);
			// spread
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertySpread], t_spread);
			
			t_effect.size = t_size;
			t_effect.spread = t_spread;
			
			break;
		}
			
		default:
			t_success = false;
	}

	// TODO - throw exception on error
	if (t_success)
		r_effect = t_effect;
}

//////////

// Properties

bool MCCanvasEffectHasSizeAndSpread(MCCanvasEffectType p_type)
{
	return p_type == kMCCanvasEffectTypeInnerShadow || p_type == kMCCanvasEffectTypeOuterShadow || p_type == kMCCanvasEffectTypeInnerGlow || p_type == kMCCanvasEffectTypeOuterGlow;
}

bool MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectType p_type)
{
	return p_type == kMCCanvasEffectTypeInnerShadow || p_type == kMCCanvasEffectTypeOuterShadow;
}

void MCCanvasGraphicEffectGetTypeAsString(const MCCanvasEffect &p_effect, MCStringRef &r_type)
{
	/* UNCHECKED */ MCCanvasEffectTypeToString(p_effect.type, r_type);
}

void MCCanvasGraphicEffectGetColor(const MCCanvasEffect &p_effect, MCCanvasColorRef &r_color)
{
	/* UNCHECKED */ MCCanvasColorCopy(p_effect.color, r_color);
}

void MCCanvasGraphicEffectSetColor(MCCanvasEffect &x_effect, MCCanvasColorRef p_color)
{
	MCCanvasColorRef t_color;
	/* UNCHECKED */ MCCanvasColorCopy(p_color, t_color);
	MCCanvasColorDelete(x_effect.color);
	x_effect.color = p_color;
}

void MCCanvasEffectGetBlendModeAsString(const MCCanvasEffect &p_effect, MCStringRef &r_blend_mode)
{
	/* UNCHECKED */ MCCanvasBlendModeToString(p_effect.blend_mode, r_blend_mode);
}

void MCCanvasEffectSetBlendModeAsString(MCCanvasEffect &x_effect, MCStringRef p_blend_mode)
{
	MCGBlendMode t_blend_mode;
	if (MCCanvasBlendModeFromString(p_blend_mode, t_blend_mode))
		x_effect.blend_mode = t_blend_mode;
}

void MCCanvasEffectGetOpacity(const MCCanvasEffect &p_effect, MCCanvasFloat &r_opacity)
{
	r_opacity = p_effect.opacity;
}

void MCCanvasEffectSetOpacity(MCCanvasEffect &x_effect, MCCanvasFloat p_opacity)
{
	x_effect.opacity = p_opacity;
}

void MCCanvasEffectGetSize(const MCCanvasEffect &p_effect, MCCanvasFloat &r_size)
{
	if (!MCCanvasEffectHasSizeAndSpread(p_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	r_size = p_effect.size;
}

void MCCanvasEffectSetSize(MCCanvasEffect &x_effect, MCCanvasFloat p_size)
{
	if (!MCCanvasEffectHasSizeAndSpread(x_effect.type))
	{
		// TODO - throw error
		return;
	}

	x_effect.size = p_size;
}

void MCCanvasEffectGetSpread(const MCCanvasEffect &p_effect, MCCanvasFloat &r_spread)
{
	if (!MCCanvasEffectHasSizeAndSpread(p_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	r_spread = p_effect.spread;
}

void MCCanvasEffectSetSpread(MCCanvasEffect &x_effect, MCCanvasFloat p_spread)
{
	if (!MCCanvasEffectHasSizeAndSpread(x_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	x_effect.spread = p_spread;
}

void MCCanvasEffectGetDistance(const MCCanvasEffect &p_effect, MCCanvasFloat &r_distance)
{
	if (!MCCanvasEffectHasDistanceAndAngle(p_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	r_distance = p_effect.distance;
}

void MCCanvasEffectSetDistance(MCCanvasEffect &x_effect, MCCanvasFloat p_distance)
{
	if (!MCCanvasEffectHasDistanceAndAngle(x_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	x_effect.distance = p_distance;
}

void MCCanvasEffectGetAngle(const MCCanvasEffect &p_effect, MCCanvasFloat &r_angle)
{
	if (!MCCanvasEffectHasDistanceAndAngle(p_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	r_angle = p_effect.angle;
}

void MCCanvasEffectSetAngle(MCCanvasEffect &x_effect, MCCanvasFloat p_angle)
{
	if (!MCCanvasEffectHasDistanceAndAngle(x_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	x_effect.angle = p_angle;
}

////////////////////////////////////////////////////////////////////////////////

// Canvas

struct MCCanvasProperties
{
	MCCanvasPaint paint;
	MCGFillRule fill_rule;
	bool antialias;
	MCCanvasFloat opacity;
	MCGBlendMode blend_mode;
	bool stippled;
	MCGImageFilter image_filter;
};

struct __MCCanvasImpl
{
	bool paint_changed : 1;
	bool fill_rule_changed : 1;
	bool antialias_changed : 1;
	bool opacity_changed : 1;
	bool blend_mode_changed : 1;
	bool stippled_changed : 1;
	
	MCCanvasProperties *prop_stack;
	uint32_t prop_max;
	uint32_t prop_index;
	
	MCCanvasProperties &props() { return prop_stack[prop_index]; }
	const MCCanvasProperties &props() const { return prop_stack[prop_index]; }
	
	MCGContextRef context;
};

void MCCanvasDirtyProperties(__MCCanvasImpl &p_canvas)
{
	p_canvas.antialias_changed = p_canvas.blend_mode_changed = p_canvas.fill_rule_changed = p_canvas.opacity_changed = p_canvas.paint_changed = p_canvas.stippled_changed = true;
}

bool MCCanvasPropertiesInit(MCCanvasProperties &p_properties)
{
	bool t_success;
	t_success = true;
	
	MCCanvasProperties t_properties;
	t_properties.antialias = true;
	t_properties.blend_mode = kMCGBlendModeSourceOver;
	t_properties.fill_rule = kMCGFillRuleEvenOdd;
	t_properties.opacity = 1.0;
	t_properties.paint = nil;
	t_properties.stippled = false;
	t_properties.image_filter = kMCGImageFilterMedium;
	
	MCCanvasColorRef t_black;
	t_black = nil;
	
	if (t_success)
		t_success = MCCanvasColorCreateRGBA(0, 0, 0, 1, t_black);
	
	MCCanvasSolidPaint t_black_paint;
	t_black_paint = nil;
	
	if (t_success)
	{
		MCCanvasSolidPaintCreateWithColor(t_black, t_black_paint);
		t_success = t_black_paint != nil;
	}
	
	MCCanvasColorDelete(t_black);
	
	if (t_success)
	{
		t_properties.paint = t_black_paint;
		
		p_properties = t_properties;
	}
	else
	{
		// TODO - throw error
	}
	
	return t_success;
}

bool MCCanvasPropertiesCopy(MCCanvasProperties &p_src, MCCanvasProperties &p_dst)
{
	MCCanvasProperties t_properties;
	t_properties.antialias = p_src.antialias;
	t_properties.blend_mode = p_src.blend_mode;
	t_properties.fill_rule = p_src.fill_rule;
	t_properties.opacity = p_src.opacity;
	t_properties.stippled = p_src.stippled;
	t_properties.image_filter = p_src.image_filter;
	
	MCCanvasPaintType_Copy(&(p_src.paint), &(t_properties.paint));
	if (t_properties.paint == nil)
		return false;
	
	p_dst = t_properties;
	
	return true;
}

void MCCanvasPropertiesClear(MCCanvasProperties &p_properties)
{
	MCCanvasPaintType_Finalize(&p_properties.paint);
	MCMemoryClear(&p_properties, sizeof(MCCanvasProperties));
}

bool MCCanvasPropertiesPush(__MCCanvasImpl &x_canvas)
{
	if (x_canvas.prop_index <= x_canvas.prop_max)
	{
		if (!MCMemoryResizeArray(x_canvas.prop_max + 1, x_canvas.prop_stack, x_canvas.prop_max))
			return false;
	}
	
	if (!MCCanvasPropertiesCopy(x_canvas.prop_stack[x_canvas.prop_index], x_canvas.prop_stack[x_canvas.prop_index + 1]))
		return false;
	
	x_canvas.prop_index++;
	
	return true;
}

bool MCCanvasPropertiesPop(__MCCanvasImpl &x_canvas)
{
	if (x_canvas.prop_index == 0)
		return false;
	
	MCCanvasPropertiesClear(x_canvas.prop_stack[x_canvas.prop_index]);
	x_canvas.prop_index--;
	
	MCCanvasDirtyProperties(x_canvas);

	return true;
}

bool MCCanvasCreate(MCGContextRef p_context, MCCanvasRef &r_canvas)
{
	bool t_success;
	t_success = true;
	
	MCCanvasRef t_canvas;
	t_canvas = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasTypeInfo, sizeof(__MCCanvasImpl), r_canvas);
	
	__MCCanvasImpl *t_canvas_impl;
	t_canvas_impl = nil;

	if (t_success)
	{
		// init property stack with 5 levels
		t_canvas_impl = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(t_canvas);
		t_success = MCMemoryNewArray(5, t_canvas_impl->prop_stack);
	}
	
	if (t_success)
	{
		t_canvas_impl->prop_max = 5;
		t_success = MCCanvasPropertiesInit(t_canvas_impl->prop_stack[0]);
	}
	
	if (t_success)
	{
		t_canvas_impl->prop_index = 0;
		t_canvas_impl->context = p_context;
		MCCanvasDirtyProperties(*t_canvas_impl);
		
		r_canvas = t_canvas;
	}
	else
		MCValueRelease(t_canvas);
	
	return t_success;
}

// MCCanvasRef Type Methods

void __MCCanvasDestroy(MCValueRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(p_canvas);
	
	if (t_canvas->prop_stack != nil)
	{
		for (uint32_t i = 0; i <= t_canvas->prop_index; i++)
			MCCanvasPropertiesClear(t_canvas->prop_stack[i]);
		MCMemoryDeleteArray(t_canvas->prop_stack);
	}
}

bool __MCCanvasCopy(MCValueRef p_canvas, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_canvas;
	else
		r_copy = MCValueRetain(p_canvas);
	
	return true;
}

bool __MCCanvasEqual(MCValueRef p_left, MCValueRef p_right)
{
	return p_left == p_right;
}

hash_t __MCCanvasHash(MCValueRef p_canvas)
{
	return (hash_t)p_canvas;
}

bool __MCCanvasDescribe(MCValueRef p_canvas, MCStringRef &r_description)
{
	// TODO - provide canvas description?
	return false;
}

// Properties

static inline MCCanvasProperties &MCCanvasGetProps(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(p_canvas);
	
	return t_canvas->props();
}

void MCCanvasGetPaint(MCCanvasRef p_canvas, MCCanvasPaint &r_paint)
{
	MCCanvasPaintType_Copy(&MCCanvasGetProps(p_canvas).paint, &r_paint);
}

void MCCanvasSetPaint(MCCanvasRef &x_canvas, const MCCanvasPaint &p_paint)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCCanvasPaint t_paint;
	/* UNCHECKED */ MCCanvasPaintType_Copy(&p_paint, &t_paint);
	
	MCCanvasPaintType_Finalize(&t_canvas->props().paint);
	t_canvas->props().paint = t_paint;
	t_canvas->paint_changed = true;
}

void MCCanvasGetFillRuleAsString(MCCanvasRef p_canvas, MCStringRef &r_string)
{
	if (!MCCanvasFillRuleToString(MCCanvasGetProps(p_canvas).fill_rule, r_string))
	{
		// TODO - throw error
	}
}

void MCCanvasSetFillRuleAsString(MCCanvasRef &x_canvas, MCStringRef p_string)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	if (!MCCanvasFillRuleFromString(p_string, t_canvas->props().fill_rule))
	{
		// TODO - throw error
		return;
	}
	
	t_canvas->fill_rule_changed = true;
}

void MCCanvasGetAntialias(MCCanvasRef p_canvas, bool &r_antialias)
{
	r_antialias = MCCanvasGetProps(p_canvas).antialias;
}

void MCCanvasSetAntialias(MCCanvasRef &x_canvas, bool p_antialias)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	t_canvas->props().antialias = p_antialias;
	t_canvas->antialias_changed = true;
}

void MCCanvasGetOpacity(MCCanvasRef p_canvas, MCCanvasFloat &r_opacity)
{
	r_opacity = MCCanvasGetProps(p_canvas).opacity;
}

void MCCanvasSetOpacity(MCCanvasRef &x_canvas, MCCanvasFloat p_opacity)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	t_canvas->props().opacity = p_opacity;
	t_canvas->opacity_changed = true;
}

void MCCanvasGetBlendModeAsString(MCCanvasRef p_canvas, MCStringRef &r_blend_mode)
{
	/* UNCHECKED */ MCCanvasBlendModeToString(MCCanvasGetProps(p_canvas).blend_mode, r_blend_mode);
}

void MCCanvasSetBlendModeAsString(MCCanvasRef &x_canvas, MCStringRef p_blend_mode)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	/* UNCHECKED */ MCCanvasBlendModeFromString(p_blend_mode, t_canvas->props().blend_mode);
	t_canvas->blend_mode_changed = true;
}

void MCCanvasGetStippled(MCCanvasRef p_canvas, bool &r_stippled)
{
	r_stippled = MCCanvasGetProps(p_canvas).stippled;
}

void MCCanvasSetStippled(MCCanvasRef &x_canvas, bool p_stippled)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	t_canvas->props().stippled = p_stippled;
	t_canvas->stippled_changed = true;
}

void MCCanvasGetImageResizeQualityAsString(MCCanvasRef p_canvas, MCStringRef &r_quality)
{
	/* UNCHECKED */ MCCanvasImageFilterToString(MCCanvasGetProps(p_canvas).image_filter, r_quality);
}

void MCCanvasSetImageResizeQualityAsString(MCCanvasRef &x_canvas, MCStringRef p_quality)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	/* UNCHECKED */ MCCanvasImageFilterFromString(p_quality, t_canvas->props().image_filter);
	// need to re-apply pattern paint if quality changes
	if (t_canvas->props().paint->type == kMCCanvasPaintTypePattern)
		t_canvas->paint_changed = true;
}

//////////

void MCCanvasApplySolidPaint(__MCCanvasImpl &x_canvas, const MCCanvasSolidPaint &p_paint)
{
	MCCanvasFloat t_red, t_green, t_blue, t_alpha;
	MCCanvasColorGetRGBA(p_paint->color, t_red, t_green, t_blue, t_alpha);
	
	MCGContextSetFillRGBAColor(x_canvas.context, t_red, t_green, t_blue, t_alpha);
	MCGContextSetStrokeRGBAColor(x_canvas.context, t_red, t_green, t_blue, t_alpha);
}

void MCCanvasApplyPatternPaint(__MCCanvasImpl &x_canvas, const MCCanvasPattern &p_pattern)
{
	MCGImageFrame t_frame;
	
	MCGAffineTransform t_transform;
	
	MCGAffineTransform t_combined;
	t_combined = MCGAffineTransformConcat(p_pattern->transform, MCGContextGetDeviceTransform(x_canvas.context));
	
	MCGFloat t_scale;
	t_scale = MCGAffineTransformGetEffectiveScale(t_combined);
	
	if (MCImageRepLock(p_pattern->image, 0, t_scale, t_frame))
	{
		t_transform = MCGAffineTransformMakeScale(1.0 / t_frame.x_scale, 1.0 / t_frame.y_scale);
		
		// return image & transform scaled for image density
		t_transform = MCGAffineTransformConcat(p_pattern->transform, t_transform);
		

		MCGContextSetFillPattern(x_canvas.context, t_frame.image, t_transform, x_canvas.props().image_filter);
		MCGContextSetStrokePattern(x_canvas.context, t_frame.image, t_transform, x_canvas.props().image_filter);
		
		MCImageRepUnlock(p_pattern->image, 0, t_frame);
	}
	
}

bool MCCanvasApplyGradientPaint(__MCCanvasImpl &x_canvas, const MCCanvasGradient &p_gradient)
{
	bool t_success;
	t_success = true;
	
	MCGFloat *t_offsets;
	t_offsets = nil;
	
	MCGColor *t_colors;
	t_colors = nil;
	
	if (t_success)
		t_success = MCMemoryNewArray(p_gradient->ramp_length, t_offsets) && MCMemoryNewArray(p_gradient->ramp_length, t_colors);
	
	if (t_success)
	{
		for (uint32_t i = 0; i < p_gradient->ramp_length; i++)
		{
			t_offsets[i] = p_gradient->ramp[i].offset;
			t_colors[i] = MCCanvasColorToMCGColor(p_gradient->ramp[i].color);
		}
		
		MCGContextSetFillGradient(x_canvas.context, p_gradient->function, t_offsets, t_colors, p_gradient->ramp_length, p_gradient->mirror, p_gradient->wrap, p_gradient->repeats, p_gradient->transform, p_gradient->filter);
		MCGContextSetStrokeGradient(x_canvas.context, p_gradient->function, t_offsets, t_colors, p_gradient->ramp_length, p_gradient->mirror, p_gradient->wrap, p_gradient->repeats, p_gradient->transform, p_gradient->filter);
	}

	MCMemoryDeleteArray(t_offsets);
	MCMemoryDeleteArray(t_colors);
	
	return t_success;
}

void MCCanvasApplyPaint(__MCCanvasImpl &x_canvas, MCCanvasPaint &p_paint)
{
	switch (p_paint->type)
	{
		case kMCCanvasPaintTypeSolid:
			MCCanvasApplySolidPaint(x_canvas, (const MCCanvasSolidPaint)p_paint);
			MCCanvasApplySolidPaint(x_canvas, (const MCCanvasSolidPaint)p_paint);
			
			break;
			
		case kMCCanvasPaintTypePattern:
			MCCanvasApplyPatternPaint(x_canvas, (const MCCanvasPattern)p_paint);
			MCCanvasApplyPatternPaint(x_canvas, (const MCCanvasPattern)p_paint);
			
			break;
			
		case kMCCanvasPaintTypeGradient:
			MCCanvasApplyGradientPaint(x_canvas, (const MCCanvasGradient)p_paint);
			MCCanvasApplyGradientPaint(x_canvas, (const MCCanvasGradient)p_paint);
			
			break;
	}
}

void MCCanvasApplyChanges(__MCCanvasImpl &x_canvas)
{
	if (x_canvas.paint_changed)
	{
		MCCanvasApplyPaint(x_canvas, x_canvas.props().paint);
		x_canvas.paint_changed = false;
	}
	
	if (x_canvas.fill_rule_changed)
	{
		MCGContextSetFillRule(x_canvas.context, x_canvas.props().fill_rule);
		x_canvas.fill_rule_changed = false;
	}
	
	if (x_canvas.antialias_changed)
	{
		MCGContextSetShouldAntialias(x_canvas.context, x_canvas.props().antialias);
		x_canvas.antialias_changed = false;
	}
	
	if (x_canvas.opacity_changed)
	{
		MCGContextSetOpacity(x_canvas.context, x_canvas.props().opacity);
		x_canvas.opacity_changed = false;
	}
	
	if (x_canvas.blend_mode_changed)
	{
		MCGContextSetBlendMode(x_canvas.context, x_canvas.props().blend_mode);
		x_canvas.blend_mode_changed = false;
	}
	
	if (x_canvas.stippled_changed)
	{
		MCGPaintStyle t_style;
		t_style = x_canvas.props().stippled ? kMCGPaintStyleStippled : kMCGPaintStyleOpaque;
		MCGContextSetFillPaintStyle(x_canvas.context, t_style);
		MCGContextSetStrokePaintStyle(x_canvas.context, t_style);
		x_canvas.stippled_changed = false;
	}
}

//////////

void MCCanvasCanvasTransform(MCCanvasRef &x_canvas, const MCCanvasTransform &p_transform)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCGContextConcatCTM(t_canvas->context, p_transform);
	// Need to re-apply pattern paint when transform changes
	if (t_canvas->props().paint->type == kMCCanvasPaintTypePattern)
		t_canvas->paint_changed = true;
}

void MCCanvasCanvasScale(MCCanvasRef &x_canvas, MCCanvasFloat p_scale_x, MCCanvasFloat p_scale_y)
{
	MCCanvasCanvasTransform(x_canvas, MCGAffineTransformMakeScale(p_scale_x, p_scale_y));
}

void MCCanvasCanvasRotate(MCCanvasRef &x_canvas, MCCanvasFloat p_angle)
{
	MCCanvasCanvasTransform(x_canvas, MCGAffineTransformMakeRotation(p_angle));
}

void MCCanvasCanvasTranslate(MCCanvasRef &x_canvas, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasCanvasTransform(x_canvas, MCGAffineTransformMakeTranslation(p_x, p_y));
}

//////////

void MCCanvasCanvasSaveState(MCCanvasRef &x_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	if (!MCCanvasPropertiesPush(*t_canvas))
	{
		// TODO - throw error
		return;
	}
	MCGContextSave(t_canvas->context);
}

void MCCanvasCanvasRestore(MCCanvasRef &x_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	if (!MCCanvasPropertiesPop(*t_canvas))
	{
		// TODO - throw error
		return;
	}
	MCGContextRestore(t_canvas->context);
}

void MCCanvasCanvasBeginLayer(MCCanvasRef &x_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	if (!MCCanvasPropertiesPush(*t_canvas))
	{
		// TODO - throw error
		return;
	}
	MCGContextBegin(t_canvas->context, true);
}

static void MCPolarCoordsToCartesian(MCGFloat p_distance, MCGFloat p_angle, MCGFloat &r_x, MCGFloat &r_y)
{
	r_x = p_distance * cos(p_angle);
	r_y = p_distance * sin(p_angle);
}


void MCCanvasCanvasBeginLayerWithEffect(MCCanvasRef &x_canvas, const MCCanvasEffect &p_effect, const MCCanvasRectangle &p_rect)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	if (!MCCanvasPropertiesPush(*t_canvas))
	{
		// TODO - throw error
		return;
	}

	MCGBitmapEffects t_effects;
	t_effects.has_color_overlay = t_effects.has_drop_shadow = t_effects.has_inner_glow = t_effects.has_inner_shadow = t_effects.has_outer_glow = false;
	
	switch (p_effect.type)
	{
		case kMCCanvasEffectTypeColorOverlay:
		{
			t_effects.has_color_overlay = true;
			t_effects.color_overlay.blend_mode = p_effect.blend_mode;
			t_effects.color_overlay.color = MCCanvasColorToMCGColor(p_effect.color);
			break;
		}
			
		case kMCCanvasEffectTypeInnerGlow:
		{
			t_effects.has_inner_glow = true;
			t_effects.inner_glow.blend_mode = p_effect.blend_mode;
			t_effects.inner_glow.color = MCCanvasColorToMCGColor(p_effect.color);
//			t_effects.inner_glow.inverted = // TODO - inverted property?
			t_effects.inner_glow.size = p_effect.size;
			t_effects.inner_glow.spread = p_effect.spread;
			break;
		}
			
		case kMCCanvasEffectTypeInnerShadow:
		{
			t_effects.has_inner_shadow = true;
			t_effects.inner_shadow.blend_mode = p_effect.blend_mode;
			t_effects.inner_shadow.color = MCCanvasColorToMCGColor(p_effect.color);
//			t_effects.inner_shadow.knockout = // TODO - knockout property?
			t_effects.inner_shadow.size = p_effect.size;
			t_effects.inner_shadow.spread = p_effect.spread;
			MCPolarCoordsToCartesian(p_effect.distance, p_effect.angle, t_effects.inner_shadow.x_offset, t_effects.inner_shadow.y_offset);
			break;
		}
			
		case kMCCanvasEffectTypeOuterGlow:
		{
			t_effects.has_outer_glow = true;
			t_effects.outer_glow.blend_mode = p_effect.blend_mode;
			t_effects.outer_glow.color = MCCanvasColorToMCGColor(p_effect.color);
			t_effects.outer_glow.size = p_effect.size;
			t_effects.outer_glow.spread = p_effect.spread;
			break;
		}
			
		case kMCCanvasEffectTypeOuterShadow:
		{
			t_effects.has_drop_shadow = true;
			t_effects.drop_shadow.blend_mode = p_effect.blend_mode;
			t_effects.drop_shadow.color = MCCanvasColorToMCGColor(p_effect.color);
			t_effects.drop_shadow.size = p_effect.size;
			t_effects.drop_shadow.spread = p_effect.spread;
			MCPolarCoordsToCartesian(p_effect.distance, p_effect.angle, t_effects.drop_shadow.x_offset, t_effects.drop_shadow.y_offset);
			break;
		}
			
		default:
			MCAssert(false);
	}
	
	MCGContextBeginWithEffects(t_canvas->context, p_rect, t_effects);
}

void MCCanvasCanvasEndLayer(MCCanvasRef &x_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	if (!MCCanvasPropertiesPop(*t_canvas))
	{
		// TODO - throw error
		return;
	}
	MCGContextRestore(t_canvas->context);
}

void MCCanvasCanvasFill(MCCanvasRef &x_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	MCGContextFill(t_canvas->context);
}

void MCCanvasCanvasStroke(MCCanvasRef &x_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	MCGContextStroke(t_canvas->context);
}

void MCCanvasCanvasClipToRect(MCCanvasRef &x_canvas, const MCCanvasRectangle &p_rect)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCGContextClipToRect(t_canvas->context, p_rect);
}

void MCCanvasCanvasAddPath(MCCanvasRef &x_canvas, const MCCanvasPath &p_path)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCGContextAddPath(t_canvas->context, p_path.path);
}

void MCCanvasCanvasFillPath(MCCanvasRef &x_canvas, const MCCanvasPath &p_path)
{
	MCCanvasCanvasAddPath(x_canvas, p_path);
	MCCanvasCanvasFill(x_canvas);
}

void MCCanvasCanvasStrokePath(MCCanvasRef &x_canvas, const MCCanvasPath &p_path)
{
	MCCanvasCanvasAddPath(x_canvas, p_path);
	MCCanvasCanvasStroke(x_canvas);
}

void MCCanvasCanvasDrawRectOfImage(MCCanvasRef &x_canvas, const MCCanvasImage &p_image, const MCCanvasRectangle &p_src_rect, const MCCanvasRectangle &p_dst_rect)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCCanvasApplyChanges(*t_canvas);

	MCGImageFrame t_frame;
	
	MCGFloat t_scale;
	t_scale = MCGAffineTransformGetEffectiveScale(MCGContextGetDeviceTransform(t_canvas->context));
	
	if (MCImageRepLock(p_image, 0, t_scale, t_frame))
	{
		MCGAffineTransform t_transform;
		t_transform = MCGAffineTransformMakeScale(1.0 / t_frame.x_scale, 1.0 / t_frame.y_scale);
		
		MCGRectangle t_src_rect;
		t_src_rect = MCGRectangleScale(p_src_rect, t_frame.x_scale, t_frame.y_scale);
		
		MCGContextDrawRectOfImage(t_canvas->context, t_frame.image, t_src_rect, p_dst_rect, t_canvas->props().image_filter);
		
		MCImageRepUnlock(p_image, 0, t_frame);
	}
}

void MCCanvasCanvasDrawImage(MCCanvasRef &x_canvas, const MCCanvasImage &p_image, const MCCanvasRectangle p_dst_rect)
{
	MCCanvasRectangle t_src_rect;
	uint32_t t_width, t_height;
	MCCanvasImageGetWidth(p_image, t_width);
	MCCanvasImageGetHeight(p_image, t_height);
	t_src_rect = MCGRectangleMake(0, 0, t_width, t_height);
	MCCanvasCanvasDrawRectOfImage(x_canvas, p_image, t_src_rect, p_dst_rect);
}

void MCCanvasCanvasMoveTo(MCCanvasRef &x_canvas, const MCCanvasPoint &p_point)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCGContextMoveTo(t_canvas->context, p_point);
}

void MCCanvasCanvasLineTo(MCCanvasRef &x_canvas, const MCCanvasPoint &p_point)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCGContextLineTo(t_canvas->context, p_point);
}

void MCCanvasCanvasCurveThroughPoint(MCCanvasRef &x_canvas, const MCCanvasPoint &p_through, const MCCanvasPoint &p_to)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCGContextQuadraticTo(t_canvas->context, p_through, p_to);
}

void MCCanvasCanvasCurveThroughPoints(MCCanvasRef &x_canvas, const MCCanvasPoint &p_through_a, const MCCanvasPoint &p_through_b, const MCCanvasPoint &p_to)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCGContextCubicTo(t_canvas->context, p_through_a, p_through_b, p_to);
}

void MCCanvasCanvasClosePath(MCCanvasRef &x_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(x_canvas);
	
	MCGContextCloseSubpath(t_canvas->context);
}

////////////////////////////////////////////////////////////////////////////////

static MCValueCustomCallbacks kMCCanvasColorCustomValueCallbacks =
{
	false,
	__MCCanvasColorDestroy,
	__MCCanvasColorCopy,
	__MCCanvasColorEqual,
	__MCCanvasColorHash,
	__MCCanvasColorDescribe,
};

static MCValueCustomCallbacks kMCCanvasCustomValueCallbacks =
{
	true,
	__MCCanvasDestroy,
	__MCCanvasCopy,
	__MCCanvasEqual,
	__MCCanvasHash,
	__MCCanvasDescribe,
};

void MCCanvasTypesInitialize()
{
	/* UNCHECKED */ MCCustomTypeInfoCreate(&kMCCanvasColorCustomValueCallbacks, kMCCanvasColorTypeInfo);
	/* UNCHECKED */ MCCustomTypeInfoCreate(&kMCCanvasCustomValueCallbacks, kMCCanvasTypeInfo);
}

void MCCanvasTypesFinalize()
{
	MCValueRelease(kMCCanvasColorTypeInfo);
	MCValueRelease(kMCCanvasTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////

void MCCanvasStringsInitialize()
{
	MCMemoryClear(s_blend_mode_map, sizeof(s_blend_mode_map));
	MCMemoryClear(s_transform_matrix_keys, sizeof(s_transform_matrix_keys));
	MCMemoryClear(s_effect_type_map, sizeof(s_effect_type_map));
	MCMemoryClear(s_effect_property_map, sizeof(s_effect_property_map));
	MCMemoryClear(s_gradient_type_map, sizeof(s_gradient_type_map));
	MCMemoryClear(s_canvas_fillrule_map, sizeof(s_gradient_type_map));
	MCMemoryClear(s_image_filter_map, sizeof(s_image_filter_map));
	
	/* UNCHECKED */
	s_blend_mode_map[kMCGBlendModeClear] = MCNAME("clear");
	s_blend_mode_map[kMCGBlendModeCopy] = MCNAME("copy");
	s_blend_mode_map[kMCGBlendModeSourceOut] = MCNAME("source over");
	s_blend_mode_map[kMCGBlendModeSourceIn] = MCNAME("source in");
	s_blend_mode_map[kMCGBlendModeSourceOut] = MCNAME("source out");
	s_blend_mode_map[kMCGBlendModeSourceAtop] = MCNAME("source atop");
	s_blend_mode_map[kMCGBlendModeDestinationOver] = MCNAME("destination over");
	s_blend_mode_map[kMCGBlendModeDestinationIn] = MCNAME("destination in");
	s_blend_mode_map[kMCGBlendModeDestinationOut] = MCNAME("destination out");
	s_blend_mode_map[kMCGBlendModeDestinationAtop] = MCNAME("destination atop");
	s_blend_mode_map[kMCGBlendModeXor] = MCNAME("xor");
	s_blend_mode_map[kMCGBlendModePlusDarker] = MCNAME("plus darker");
	s_blend_mode_map[kMCGBlendModePlusLighter] = MCNAME("plus lighter");
	s_blend_mode_map[kMCGBlendModeMultiply] = MCNAME("multiply");
	s_blend_mode_map[kMCGBlendModeScreen] = MCNAME("screen");
	s_blend_mode_map[kMCGBlendModeOverlay] = MCNAME("overlay");
	s_blend_mode_map[kMCGBlendModeDarken] = MCNAME("darken");
	s_blend_mode_map[kMCGBlendModeLighten] = MCNAME("lighten");
	s_blend_mode_map[kMCGBlendModeColorDodge] = MCNAME("color dodge");
	s_blend_mode_map[kMCGBlendModeColorBurn] = MCNAME("color burn");
	s_blend_mode_map[kMCGBlendModeSoftLight] = MCNAME("soft light");
	s_blend_mode_map[kMCGBlendModeHardLight] = MCNAME("hard light");
	s_blend_mode_map[kMCGBlendModeDifference] = MCNAME("difference");
	s_blend_mode_map[kMCGBlendModeExclusion] = MCNAME("exclusion");
	s_blend_mode_map[kMCGBlendModeHue] = MCNAME("hue");
	s_blend_mode_map[kMCGBlendModeSaturation] = MCNAME("saturation");
	s_blend_mode_map[kMCGBlendModeColor] = MCNAME("color");
	s_blend_mode_map[kMCGBlendModeLuminosity] = MCNAME("luminosity");
	
	s_transform_matrix_keys[0] = MCNAME("0,0");
	s_transform_matrix_keys[1] = MCNAME("1,0");
	s_transform_matrix_keys[2] = MCNAME("2,0");
	s_transform_matrix_keys[3] = MCNAME("0,1");
	s_transform_matrix_keys[4] = MCNAME("1,1");
	s_transform_matrix_keys[5] = MCNAME("2,1");
	s_transform_matrix_keys[6] = MCNAME("0,2");
	s_transform_matrix_keys[7] = MCNAME("1,2");
	s_transform_matrix_keys[8] = MCNAME("2,2");

	s_effect_type_map[kMCCanvasEffectTypeColorOverlay] = MCNAME("color overlay");
	s_effect_type_map[kMCCanvasEffectTypeInnerShadow] = MCNAME("inner shadow");
	s_effect_type_map[kMCCanvasEffectTypeOuterShadow] = MCNAME("outer shadow");
	s_effect_type_map[kMCCanvasEffectTypeInnerGlow] = MCNAME("inner glow");
	s_effect_type_map[kMCCanvasEffectTypeOuterGlow] = MCNAME("outer glow");
	
	s_effect_property_map[kMCCanvasEffectPropertyColor] = MCNAME("color");
	s_effect_property_map[kMCCanvasEffectPropertyBlendMode] = MCNAME("blend mode");
	s_effect_property_map[kMCCanvasEffectPropertyOpacity] = MCNAME("opacity");
	s_effect_property_map[kMCCanvasEffectPropertySize] = MCNAME("size");
	s_effect_property_map[kMCCanvasEffectPropertySpread] = MCNAME("spread");
	s_effect_property_map[kMCCanvasEffectPropertyDistance] = MCNAME("distance");
	s_effect_property_map[kMCCanvasEffectPropertyAngle] = MCNAME("angle");
	
	s_gradient_type_map[kMCGGradientFunctionLinear] = MCNAME("linear");
	s_gradient_type_map[kMCGGradientFunctionRadial] = MCNAME("radial");
	s_gradient_type_map[kMCGGradientFunctionSweep] = MCNAME("conical");
	s_gradient_type_map[kMCGLegacyGradientDiamond] = MCNAME("diamond");
	s_gradient_type_map[kMCGLegacyGradientSpiral] = MCNAME("spiral");
	s_gradient_type_map[kMCGLegacyGradientXY] = MCNAME("xy");
	s_gradient_type_map[kMCGLegacyGradientSqrtXY] = MCNAME("sqrtxy");
	
	s_canvas_fillrule_map[kMCGFillRuleEvenOdd] = MCNAME("even odd");
	s_canvas_fillrule_map[kMCGFillRuleNonZero] = MCNAME("non zero");
	
	s_image_filter_map[kMCGImageFilterNone] = MCNAME("none");
	s_image_filter_map[kMCGImageFilterLow] = MCNAME("low");
	s_image_filter_map[kMCGImageFilterMedium] = MCNAME("medium");
	s_image_filter_map[kMCGImageFilterHigh] = MCNAME("high");
	
	s_path_command_map[kMCGPathCommandMoveTo] = MCNAME("move");
	s_path_command_map[kMCGPathCommandLineTo] = MCNAME("line");
	s_path_command_map[kMCGPathCommandQuadCurveTo] = MCNAME("quad");
	s_path_command_map[kMCGPathCommandCubicCurveTo] = MCNAME("cubic");
	s_path_command_map[kMCGPathCommandCloseSubpath] = MCNAME("close");
	s_path_command_map[kMCGPathCommandEnd] = MCNAME("end");
	
/* UNCHECKED */
}

void MCCanvasStringsFinalize()
{
	for (uint32_t i = 0; i < kMCGBlendModeCount; i++)
		MCValueRelease(s_blend_mode_map[i]);
	
	for (uint32_t i = 0; i < 9; i++)
		MCValueRelease(s_transform_matrix_keys[i]);

	for (uint32_t i = 0; i < _MCCanvasEffectTypeCount; i++)
		MCValueRelease(s_effect_type_map[i]);
	
	for (uint32_t i = 0; i < _MCCanvasEffectPropertyCount; i++)
		MCValueRelease(s_effect_property_map[i]);
	
	for (uint32_t i = 0; i < kMCGGradientFunctionCount; i++)
		MCValueRelease(s_gradient_type_map[i]);
	
	for (uint32_t i = 0; i < kMCGFillRuleCount; i++)
		MCValueRelease(s_canvas_fillrule_map[i]);
		
	for (uint32_t i = 0; i < kMCGImageFilterCount; i++)
		MCValueRelease(s_image_filter_map[i]);
	
	for (uint32_t i = 0; i < kMCGPathCommandCount; i++)
		MCValueRelease(s_path_command_map[i]);
}

template <typename T, MCNameRef *N, int C>
bool _mcenumfromstring(MCStringRef p_string, T &r_value)
{
	for (uint32_t i = 0; i < C; i++)
	{
		if (MCStringIsEqualTo(p_string, MCNameGetString(N[i]), kMCStringOptionCompareCaseless))
		{
			r_value = (T)i;
			return true;
		}
	}
	
	return false;
}

template <typename T, MCNameRef *N, int C>
bool _mcenumtostring(T p_value, MCStringRef &r_string)
{
	if (p_value >= C)
		return false;
	
	if (N[p_value] == nil)
		return false;
	
	return MCStringCopy(MCNameGetString(N[p_value]), r_string);
}

bool MCCanvasBlendModeFromString(MCStringRef p_string, MCGBlendMode &r_blend_mode)
{
	return _mcenumfromstring<MCGBlendMode, s_blend_mode_map, kMCGBlendModeCount>(p_string, r_blend_mode);
}

bool MCCanvasBlendModeToString(MCGBlendMode p_blend_mode, MCStringRef &r_string)
{
	return _mcenumtostring<MCGBlendMode, s_blend_mode_map, kMCGBlendModeCount>(p_blend_mode, r_string);
}

bool MCCanvasGradientTypeFromString(MCStringRef p_string, MCGGradientFunction &r_type)
{
	return _mcenumfromstring<MCGGradientFunction, s_gradient_type_map, kMCGGradientFunctionCount>(p_string, r_type);
}

bool MCCanvasGradientTypeToString(MCGGradientFunction p_type, MCStringRef &r_string)
{
	return _mcenumtostring<MCGGradientFunction, s_gradient_type_map, kMCGGradientFunctionCount>(p_type, r_string);
}

bool MCCanvasEffectTypeToString(MCCanvasEffectType p_type, MCStringRef &r_string)
{
	return _mcenumtostring<MCCanvasEffectType, s_effect_type_map, _MCCanvasEffectTypeCount>(p_type, r_string);
}

bool MCCanvasEffectTypeFromString(MCStringRef p_string, MCCanvasEffectType &r_type)
{
	return _mcenumfromstring<MCCanvasEffectType, s_effect_type_map, kMCGFillRuleCount>(p_string, r_type);
}

bool MCCanvasFillRuleToString(MCGFillRule p_fill_rule, MCStringRef &r_string)
{
	return _mcenumtostring<MCGFillRule, s_canvas_fillrule_map, kMCGFillRuleCount>(p_fill_rule, r_string);
}

bool MCCanvasFillRuleFromString(MCStringRef p_string, MCGFillRule &r_fill_rule)
{
	return _mcenumfromstring<MCGFillRule, s_canvas_fillrule_map, kMCGFillRuleCount>(p_string, r_fill_rule);
}

bool MCCanvasFillRuleToString(MCGImageFilter p_filter, MCStringRef &r_string)
{
	return _mcenumtostring<MCGImageFilter, s_image_filter_map, kMCGImageFilterCount>(p_filter, r_string);
}

bool MCCanvasFillRuleFromString(MCStringRef p_string, MCGImageFilter &r_filter)
{
	return _mcenumfromstring<MCGImageFilter, s_image_filter_map, kMCGImageFilterCount>(p_string, r_filter);
}

bool MCCanvasPathCommandToString(MCGPathCommand p_command, MCStringRef &r_string)
{
	return _mcenumtostring<MCGPathCommand, s_path_command_map, kMCGPathCommandCount>(p_command, r_string);
}

bool MCCanvasPathCommandFromString(MCStringRef p_string, MCGPathCommand &r_command)
{
	return _mcenumfromstring<MCGPathCommand, s_path_command_map, kMCGPathCommandCount>(p_string, r_command);
}

////////////////////////////////////////////////////////////////////////////////
