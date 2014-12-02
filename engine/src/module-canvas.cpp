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
#include "module-canvas-internal.h"

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

// Custom Types

void MCCanvasTypesInitialize();
void MCCanvasTypesFinalize();

MCTypeInfoRef kMCCanvasRectangleTypeInfo;
MCTypeInfoRef kMCCanvasPointTypeInfo;
MCTypeInfoRef kMCCanvasColorTypeInfo;
MCTypeInfoRef kMCCanvasTransformTypeInfo;
MCTypeInfoRef kMCCanvasImageTypeInfo;
MCTypeInfoRef kMCCanvasPaintTypeInfo;
MCTypeInfoRef kMCCanvasGradientStopTypeInfo;
MCTypeInfoRef kMCCanvasPathTypeInfo;
MCTypeInfoRef kMCCanvasEffectTypeInfo;
MCTypeInfoRef kMCCanvasTypeInfo;

////////////////////////////////////////////////////////////////////////////////

// Constant refs

MCCanvasTransformRef kMCCanvasIdentityTransform = nil;

void MCCanvasConstantsInitialize();
void MCCanvasConstantsFinalize();

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
	MCCanvasConstantsInitialize();
}

void MCCanvasModuleFinalize()
{
	MCCanvasStringsFinalize();
	MCCanvasTypesFinalize();
	MCCanvasConstantsFinalize();
}

////////////////////////////////////////////////////////////////////////////////

MCGColor MCCanvasColorToMCGColor(MCCanvasColorRef p_color)
{
	__MCCanvasColorImpl *t_color;
	t_color = MCCanvasColorGet(p_color);
	return MCGColorMakeRGBA(t_color->red, t_color->green, t_color->blue, t_color->alpha);
}

////////////////////////////////////////////////////////////////////////////////

// Rectangle

static void __MCCanvasRectangleDestroy(MCValueRef p_value)
{
	// no-op
}

static bool __MCCanvasRectangleCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	return true;
}

static bool __MCCanvasRectangleEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_right), sizeof(__MCCanvasRectangleImpl)) == 0;
}

static hash_t __MCCanvasRectangleHash(MCValueRef p_value)
{
	return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCCanvasRectangleImpl));
}

static bool __MCCanvasRectangleDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe rectangle
	return false;
}

bool MCCanvasRectangleCreateWithMCGRectangle(const MCGRectangle &p_rect, MCCanvasRectangleRef &r_rectangle)
{
	if (!MCValueCreateCustom(kMCCanvasRectangleTypeInfo, sizeof(__MCCanvasRectangleImpl), r_rectangle))
		return false;
	
	*(MCCanvasRectangleGet(r_rectangle)) = p_rect;
	return true;
	
	// TODO - make rectangles unique?
}

__MCCanvasRectangleImpl *MCCanvasRectangleGet(MCCanvasRectangleRef p_rect)
{
	return (__MCCanvasRectangleImpl*)MCValueGetExtraBytesPtr(p_rect);
}

void MCCanvasRectangleGetMCGRectangle(MCCanvasRectangleRef p_rect, MCGRectangle &r_rect)
{
	r_rect = *MCCanvasRectangleGet(p_rect);
}

// Constructors

static inline void MCCanvasRectangleMakeWithMCGRectangle(const MCGRectangle &p_rect, MCCanvasRectangleRef &r_rect)
{
	MCCanvasRectangleRef t_rect;
	if (!MCCanvasRectangleCreateWithMCGRectangle(p_rect, t_rect))
	{
		// TODO - throw memory error
		return;
	}
	
	MCValueAssign(r_rect, t_rect);
	MCValueRelease(t_rect);
}

void MCCanvasRectangleMakeWithLTRB(MCCanvasFloat p_left, MCCanvasFloat p_top, MCCanvasFloat p_right, MCCanvasFloat p_bottom, MCCanvasRectangleRef &r_rect)
{
	MCCanvasRectangleMakeWithMCGRectangle(MCGRectangleMake(p_left, p_top, p_right - p_left, p_bottom - p_top), r_rect);
}

// Properties

void MCCanvasRectangleGetLeft(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_left)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_left = t_rect.origin.x;
}

void MCCanvasRectangleSetLeft(MCCanvasRectangleRef &x_rect, MCCanvasFloat p_left)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.origin.x = p_left;

	MCCanvasRectangleMakeWithMCGRectangle(t_rect, x_rect);
}

void MCCanvasRectangleGetTop(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_top)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_top = t_rect.origin.y;
}

void MCCanvasRectangleSetTop(MCCanvasRectangleRef &x_rect, MCCanvasFloat p_top)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.origin.y = p_top;
	
	MCCanvasRectangleMakeWithMCGRectangle(t_rect, x_rect);
}

void MCCanvasRectangleGetRight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_right)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_right = t_rect.origin.x + t_rect.size.width;
}

void MCCanvasRectangleSetRight(MCCanvasRectangleRef &x_rect, MCCanvasFloat p_right)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.origin.x = p_right - t_rect.size.width;
	
	MCCanvasRectangleMakeWithMCGRectangle(t_rect, x_rect);
}

void MCCanvasRectangleGetBottom(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_bottom)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_bottom = t_rect.origin.y + t_rect.size.height;
}

void MCCanvasRectangleSetBottom(MCCanvasRectangleRef &x_rect, MCCanvasFloat p_bottom)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.origin.y = p_bottom - t_rect.size.height;
	
	MCCanvasRectangleMakeWithMCGRectangle(t_rect, x_rect);
}

void MCCanvasRectangleGetWidth(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_width)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_width = t_rect.size.width;
}

void MCCanvasRectangleSetWidth(MCCanvasRectangleRef &x_rect, MCCanvasFloat p_width)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.size.width = p_width;
	
	MCCanvasRectangleMakeWithMCGRectangle(t_rect, x_rect);
}

void MCCanvasRectangleGetHeight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_height)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_height = t_rect.size.height;
}

void MCCanvasRectangleSetHeight(MCCanvasRectangleRef &x_rect, MCCanvasFloat p_height)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.size.height = p_height;
	
	MCCanvasRectangleMakeWithMCGRectangle(t_rect, x_rect);
}

////////////////////////////////////////////////////////////////////////////////

// Point

static void __MCCanvasPointDestroy(MCValueRef p_value)
{
	// no-op
}

static bool __MCCanvasPointCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	return true;
}

static bool __MCCanvasPointEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_left), sizeof(__MCCanvasPointImpl)) == 0;
}

static hash_t __MCCanvasPointHash(MCValueRef p_value)
{
	return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCCanvasPointImpl));
}

static bool __MCCanvasPointDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe point
	return false;
}

bool MCCanvasPointCreateWithMCGPoint(const MCGPoint &p_point, MCCanvasPointRef &r_point)
{
	if (!MCValueCreateCustom(kMCCanvasPointTypeInfo, sizeof(__MCCanvasPointImpl), r_point))
		return false;
	
	*MCCanvasPointGet(r_point) = p_point;
	return true;

	// TODO - make points unique?
}

__MCCanvasPointImpl *MCCanvasPointGet(MCCanvasPointRef p_point)
{
	return (__MCCanvasPointImpl*)MCValueGetExtraBytesPtr(p_point);
}

void MCCanvasPointGetMCGPoint(MCCanvasPointRef p_point, MCGPoint &r_point)
{
	r_point = *MCCanvasPointGet(p_point);
}

// Constructors

static inline void MCCanvasPointMakeWithMCGPoint(const MCGPoint &p_point, MCCanvasPointRef &r_point)
{
	MCCanvasPointRef t_point;
	t_point = nil;
	if (!MCCanvasPointCreateWithMCGPoint(p_point, t_point))
	{
		// TODO - throw memory error
		return;
	}
	
	MCValueAssign(r_point, t_point);
	MCValueRelease(t_point);
}

void MCCanvasPointMake(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPointRef &r_point)
{
	MCCanvasPointMakeWithMCGPoint(MCGPointMake(p_x, p_y), r_point);
}

// Properties

void MCCanvasPointGetX(MCCanvasPointRef p_point, MCCanvasFloat &r_x)
{
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(p_point, t_point);
	r_x = t_point.x;
}

void MCCanvasPointSetX(MCCanvasPointRef &x_point, MCCanvasFloat p_x)
{
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(x_point, t_point);
	t_point.x = p_x;
	
	MCCanvasPointMakeWithMCGPoint(t_point, x_point);
}

void MCCanvasPointGetY(MCCanvasPointRef p_point, MCCanvasFloat &r_y)
{
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(p_point, t_point);
	r_y = t_point.y;
}

void MCCanvasPointSetY(MCCanvasPointRef &x_point, MCCanvasFloat p_y)
{
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(x_point, t_point);
	t_point.y = p_y;
	
	MCCanvasPointMakeWithMCGPoint(t_point, x_point);
}

////////////////////////////////////////////////////////////////////////////////

// Color

// MCCanvasColorRef Type methods

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
	
	return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_right), sizeof(__MCCanvasColorImpl)) == 0;
}

static hash_t __MCCanvasColorHash(MCValueRef p_value)
{
	return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCCanvasColorImpl));
}

static bool __MCCanvasColorDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

//////////

bool MCCanvasColorCreate(const __MCCanvasColorImpl &p_color, MCCanvasColorRef &r_color)
{
	bool t_success;
	t_success = true;
	
	MCCanvasColorRef t_color;
	t_color = nil;
	
	t_success = MCValueCreateCustom(kMCCanvasColorTypeInfo, sizeof(__MCCanvasColorImpl), t_color);
	
	if (t_success)
	{
		*MCCanvasColorGet(t_color) = p_color;
		t_success = MCValueInter(t_color, r_color);
	}
	
	MCValueRelease(t_color);
	
	return t_success;
}

__MCCanvasColorImpl *MCCanvasColorGet(MCCanvasColorRef p_color)
{
	return (__MCCanvasColorImpl*)MCValueGetExtraBytesPtr(p_color);
}

static inline __MCCanvasColorImpl MCCanvasColorImplMake(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha)
{
	__MCCanvasColorImpl t_color;
	t_color.red = p_red;
	t_color.green = p_green;
	t_color.blue = p_blue;
	t_color.alpha = p_alpha;
	
	return t_color;
}

bool MCCanvasColorCreateWithRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color)
{
	return MCCanvasColorCreate(MCCanvasColorImplMake(p_red, p_green, p_blue, p_alpha), r_color);
}

MCCanvasFloat MCCanvasColorGetRed(MCCanvasColorRef color)
{
	return MCCanvasColorGet(color)->red;
}

MCCanvasFloat MCCanvasColorGetGreen(MCCanvasColorRef color)
{
	return MCCanvasColorGet(color)->green;
}

MCCanvasFloat MCCanvasColorGetBlue(MCCanvasColorRef color)
{
	return MCCanvasColorGet(color)->blue;
}

MCCanvasFloat MCCanvasColorGetAlpha(MCCanvasColorRef color)
{
	return MCCanvasColorGet(color)->alpha;
}

// Constructors

void MCCanvasColorMake(const __MCCanvasColorImpl &p_color, MCCanvasColorRef &r_color)
{
	MCCanvasColorRef t_color;
	t_color = nil;
	if (!MCCanvasColorCreate(p_color, t_color))
	{
		// TODO - throw memory error
		return;
	}
	
	MCValueAssign(r_color, t_color);
	MCValueRelease(t_color);
}

void MCCanvasColorMakeRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color)
{
	MCCanvasColorMake(MCCanvasColorImplMake(p_red, p_blue, p_green, p_alpha), r_color);
}

//////////

// Properties

void MCCanvasColorGetRed(MCCanvasColorRef p_color, MCCanvasFloat &r_red)
{
	r_red = MCCanvasColorGetRed(p_color);
}

void MCCanvasColorSetRed(MCCanvasColorRef &x_color, MCCanvasFloat p_red)
{
	__MCCanvasColorImpl *t_color;
	t_color = MCCanvasColorGet(x_color);
	
	if (t_color->red == p_red)
		return;
	
	MCCanvasColorMake(*t_color, x_color);
}

void MCCanvasColorGetGreen(MCCanvasColorRef p_color, MCCanvasFloat &r_green)
{
	r_green = MCCanvasColorGetGreen(p_color);
}

void MCCanvasColorSetGreen(MCCanvasColorRef &x_color, MCCanvasFloat p_green)
{
	__MCCanvasColorImpl *t_color;
	t_color = MCCanvasColorGet(x_color);
	
	if (t_color->green == p_green)
		return;
	
	MCCanvasColorMake(*t_color, x_color);
}

void MCCanvasColorGetBlue(MCCanvasColorRef p_color, MCCanvasFloat &r_blue)
{
	r_blue = MCCanvasColorGetBlue(p_color);
}

void MCCanvasColorSetBlue(MCCanvasColorRef &x_color, MCCanvasFloat p_blue)
{
	__MCCanvasColorImpl *t_color;
	t_color = MCCanvasColorGet(x_color);
	
	if (t_color->blue == p_blue)
		return;
	
	MCCanvasColorMake(*t_color, x_color);
}

void MCCanvasColorGetAlpha(MCCanvasColorRef p_color, MCCanvasFloat &r_alpha)
{
	r_alpha = MCCanvasColorGetAlpha(p_color);
}

void MCCanvasColorSetAlpha(MCCanvasColorRef &x_color, MCCanvasFloat p_alpha)
{
	__MCCanvasColorImpl *t_color;
	t_color = MCCanvasColorGet(x_color);
	
	if (t_color->alpha == p_alpha)
		return;
	
	MCCanvasColorMake(*t_color, x_color);
}

////////////////////////////////////////////////////////////////////////////////

// Transform

// MCCanvasColorRef Type methods

static void __MCCanvasTransformDestroy(MCValueRef p_value)
{
	// no-op
}

static bool __MCCanvasTransformCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	return true;
}

static bool __MCCanvasTransformEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_right), sizeof(__MCCanvasTransformImpl)) == 0;
}

static hash_t __MCCanvasTransformHash(MCValueRef p_value)
{
	return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCCanvasTransformImpl));
}

static bool __MCCanvasTransformDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

//////////

bool MCCanvasTransformCreateWithMCGAffineTransform(const MCGAffineTransform &p_transform, MCCanvasTransformRef &r_transform)
{
	bool t_success;
	t_success = true;
	
	MCCanvasTransformRef t_transform;
	t_transform = nil;
	
	t_success = MCValueCreateCustom(kMCCanvasTransformTypeInfo, sizeof(__MCCanvasTransformImpl), t_transform);
	
	if (t_success)
	{
		*MCCanvasTransformGet(t_transform) = p_transform;
		t_success = MCValueInter(t_transform, r_transform);
	}
	
	MCValueRelease(t_transform);
	
	return t_success;
}

__MCCanvasTransformImpl *MCCanvasTransformGet(MCCanvasTransformRef p_transform)
{
	return (__MCCanvasTransformImpl*)MCValueGetExtraBytesPtr(p_transform);
}

void MCCanvasTransformGetMCGAffineTransform(MCCanvasTransformRef p_transform, MCGAffineTransform &r_transform)
{
	r_transform = *MCCanvasTransformGet(p_transform);
}

// Constructors

void MCCanvasTransformMake(const MCGAffineTransform &p_transform, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformRef t_transform;
	t_transform = nil;
	
	if (!MCCanvasTransformCreateWithMCGAffineTransform(p_transform, t_transform))
	{
		// TODO - throw memory error
		return;
	}
	
	MCValueAssign(r_transform, t_transform);
	MCValueRelease(t_transform);
}

void MCCanvasTransformMakeIdentity(MCCanvasTransformRef &r_transform)
{
	r_transform = MCValueRetain(kMCCanvasIdentityTransform);
}

void MCCanvasTransformMakeScale(MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMakeScale(p_xscale, p_yscale), r_transform);
}

void MCCanvasTransformMakeRotation(MCCanvasFloat p_angle, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMakeRotation(p_angle), r_transform);
}

void MCCanvasTransformMakeTranslation(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMakeTranslation(p_x, p_y), r_transform);
}

void MCCanvasTransformMakeSkew(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMakeSkew(p_x, p_y), r_transform);
}

void MCCanvasTransformMakeWithMatrix(MCCanvasFloat p_a, MCCanvasFloat p_b, MCCanvasFloat p_c, MCCanvasFloat p_d, MCCanvasFloat p_tx, MCCanvasFloat p_ty, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMake(p_a, p_b, p_c, p_d, p_tx, p_ty), r_transform);
}

//////////

// Properties

void MCCanvasTransformGetMatrix(MCCanvasTransformRef p_transform, MCArrayRef &r_matrix)
{
	bool t_success;
	t_success = true;
	
	MCGAffineTransform *t_transform;
	t_transform = MCCanvasTransformGet(p_transform);
	
	MCArrayRef t_matrix;
	t_matrix = nil;
	
	if (t_success)
		t_success = MCArrayCreateMutable(t_matrix);
	
	if (t_success)
		t_success = MCArrayStoreReal(t_matrix, s_transform_matrix_keys[0], t_transform->a) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[1], t_transform->b) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[2], 0) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[3], t_transform->c) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[4], t_transform->d) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[5], 0) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[6], t_transform->tx) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[7], t_transform->ty) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[8], 1);
		
	if (t_success)
		r_matrix = t_matrix;
	else
	{
		// TODO - throw memory error
		MCValueRelease(t_matrix);
	}
}

void MCCanvasTransformSetMatrix(MCCanvasTransformRef &x_transform, MCArrayRef p_matrix)
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

	if (!t_success)
	{
		// TODO - throw matrix array keys error
		return;
	}
	
	MCCanvasTransformMakeWithMatrix(a, b, c, d, tx, ty, x_transform);
}

void MCCanvasTransformGetInverse(MCCanvasTransformRef p_transform, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformInvert(*MCCanvasTransformGet(p_transform)), r_transform);
}

// T = Tscale * Trotate * Tskew * Ttranslate

// Ttranslate:
// / 1 0 tx \
// | 0 1 ty |
// \ 0 0  1 /

// Tscale * Trotate * Tskew:
// / a  c \   / ScaleX       0 \   /  Cos(r)  Sin(r) \   / 1  Skew \   /  ScaleX * Cos(r)   ScaleX * Skew * Cos(r) + ScaleX * Sin(r) \
// \ b  d / = \      0  ScaleY / * \ -Sin(r)  Cos(r) / * \ 0     1 / = \ -ScaleY * Sin(r)  -ScaleY * Skew * Sin(r) + ScaleY * Cos(r) /

bool MCCanvasTransformDecompose(const MCGAffineTransform &p_transform, MCGPoint &r_scale, MCCanvasFloat &r_rotation, MCGPoint &r_skew, MCGPoint &r_translation)
{
	MCGFloat t_r, t_skew;
	MCGPoint t_scale, t_trans;
	
	t_trans = MCGPointMake(p_transform.tx, p_transform.ty);
	
	// if a == 0, take r to be pi/2 radians
	if (p_transform.a == 0)
	{
		t_r = M_PI / 2;
		// b == -ScaleY, c == ScaleX
		t_scale = MCGPointMake(p_transform.c, -p_transform.b);
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
		t_scale = MCGPointMake(p_transform.a, p_transform.d);
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
		
		t_scale = MCGPointMake(p_transform.a / cosf(t_r), -p_transform.b / sinf(t_r));
	}
	
	r_scale = t_scale;
	r_rotation = t_r;
	r_skew = MCGPointMake(t_skew, 0);
	r_translation = t_trans;
	
	return true;
}

MCGAffineTransform MCCanvasTransformCompose(const MCGPoint &p_scale, MCCanvasFloat p_rotation, const MCGPoint &p_skew, const MCGPoint &p_translation)
{
	MCGAffineTransform t_transform;
	t_transform = MCGAffineTransformMakeScale(p_scale.x, p_scale.y);
	t_transform = MCGAffineTransformConcat(t_transform, MCGAffineTransformMakeRotation(p_rotation));
	t_transform = MCGAffineTransformConcat(t_transform, MCGAffineTransformMakeSkew(p_skew.x, p_skew.y));
	t_transform = MCGAffineTransformConcat(t_transform, MCGAffineTransformMakeTranslation(p_translation.x, p_translation.y));
	
	return t_transform;
}

void MCCanvasTransformGetScale(MCCanvasTransformRef p_transform, MCCanvasPointRef &r_scale)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(p_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	MCCanvasPointMakeWithMCGPoint(t_scale, r_scale);
}

void MCCanvasTransformSetScale(MCCanvasTransformRef &x_transform, MCCanvasPointRef p_scale)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(x_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	MCCanvasTransformMake(MCCanvasTransformCompose(*MCCanvasPointGet(p_scale), t_rotation, t_skew, t_translation), x_transform);
}

void MCCanvasTransformGetRotation(MCCanvasTransformRef p_transform, MCCanvasFloat &r_rotation)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(p_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	r_rotation = t_rotation;
}

void MCCanvasTransformSetRotation(MCCanvasTransformRef &x_transform, MCCanvasFloat p_rotation)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(x_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	MCCanvasTransformMake(MCCanvasTransformCompose(t_scale, p_rotation, t_skew, t_translation), x_transform);
}

void MCCanvasTransformGetSkew(MCCanvasTransformRef p_transform, MCCanvasPointRef &r_skew)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(p_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	MCCanvasPointMakeWithMCGPoint(t_skew, r_skew);
}

void MCCanvasTransformSetSkew(MCCanvasTransformRef &x_transform, MCCanvasPointRef p_skew)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(x_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	MCCanvasTransformMake(MCCanvasTransformCompose(t_scale, t_rotation, *MCCanvasPointGet(p_skew), t_translation), x_transform);
}

void MCCanvasTransformGetTranslation(MCCanvasTransformRef p_transform, MCCanvasPointRef &r_translation)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(p_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	MCCanvasPointMakeWithMCGPoint(t_translation, r_translation);
}

void MCCanvasTransformSetTranslation(MCCanvasTransformRef &x_transform, MCCanvasPointRef p_translation)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(x_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	MCCanvasTransformMake(MCCanvasTransformCompose(t_scale, t_rotation, t_skew, *MCCanvasPointGet(p_translation)), x_transform);
}

//////////

// Operations

void MCCanvasTransformConcat(MCCanvasTransformRef &x_transform, const MCGAffineTransform &p_transform)
{
	MCCanvasTransformMake(MCGAffineTransformConcat(*MCCanvasTransformGet(x_transform), p_transform), x_transform);
}

void MCCanvasTransformConcat(MCCanvasTransformRef &x_transform, MCCanvasTransformRef p_transform)
{
	MCCanvasTransformConcat(x_transform, *MCCanvasTransformGet(p_transform));
}

void MCCanvasTransformScale(MCCanvasTransformRef &x_transform, MCCanvasFloat p_x_scale, MCCanvasFloat p_y_scale)
{
	MCCanvasTransformConcat(x_transform, MCGAffineTransformMakeScale(p_x_scale, p_y_scale));
}

void MCCanvasTransformRotate(MCCanvasTransformRef &x_transform, MCCanvasFloat p_rotation)
{
	MCCanvasTransformConcat(x_transform, MCGAffineTransformMakeRotation(p_rotation));
}

void MCCanvasTransformTranslate(MCCanvasTransformRef &x_transform, MCCanvasFloat p_dx, MCCanvasFloat p_dy)
{
	MCCanvasTransformConcat(x_transform, MCGAffineTransformMakeTranslation(p_dx, p_dy));
}

void MCCanvasTransformSkew(MCCanvasTransformRef &x_transform, MCCanvasFloat p_xskew, MCCanvasFloat p_yskew)
{
	MCCanvasTransformConcat(x_transform, MCGAffineTransformMakeSkew(p_xskew, p_yskew));
}

////////////////////////////////////////////////////////////////////////////////

// Image

static void __MCCanvasImageDestroy(MCValueRef p_image)
{
	MCImageRepRelease(MCCanvasImageGetImageRep((MCCanvasImageRef) p_image));
}

static bool __MCCanvasImageCopy(MCValueRef p_image, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_image;
	else
		r_copy = MCValueRetain(p_image);
	
	return true;
}

static bool __MCCanvasImageEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_right), sizeof(__MCCanvasImageImpl)) == 0;
}

static hash_t __MCCanvasImageHash(MCValueRef p_value)
{
	return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCCanvasImageImpl));
}

static bool __MCCanvasImageDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

bool MCCanvasImageCreateWithImageRep(MCImageRep *p_image, MCCanvasImageRef &r_image)
{
	bool t_success;
	t_success = true;
	
	MCCanvasImageRef t_image;
	t_image = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasImageTypeInfo, sizeof(__MCCanvasImageImpl), t_image);
	
	if (t_success)
	{
		*MCCanvasImageGet(t_image) = MCImageRepRetain(p_image);
		t_success = MCValueInter(t_image, r_image);
	}
	
	MCValueRelease(t_image);
	
	return t_success;
}

__MCCanvasImageImpl *MCCanvasImageGet(MCCanvasImageRef p_image)
{
	return (__MCCanvasImageImpl*)MCValueGetExtraBytesPtr(p_image);
}

MCImageRep *MCCanvasImageGetImageRep(MCCanvasImageRef p_image)
{
	return *MCCanvasImageGet(p_image);
}

// Constructors

void MCCanvasImageMake(MCImageRep *p_image, MCCanvasImageRef &r_image)
{
	MCCanvasImageRef t_image;
	t_image = nil;
	
	if (!MCCanvasImageCreateWithImageRep(p_image, t_image))
	{
		// TODO - throw memory error
		return;
	}
	
	MCValueAssign(r_image, t_image);
	MCValueRelease(t_image);
}

void MCCanvasImageMakeWithPath(MCStringRef p_path, MCCanvasImageRef &r_image)
{
	MCImageRep *t_image_rep;
	t_image_rep = nil;
	
	if (!MCImageRepCreateWithPath(p_path, t_image_rep))
	{
		// TODO - throw image rep error
		return;
	}
	
	MCCanvasImageMake(t_image_rep, r_image);
	MCImageRepRelease(t_image_rep);
}

void MCCanvasImageMakeWithData(MCDataRef p_data, MCCanvasImageRef &r_image)
{
	MCImageRep *t_image_rep;
	t_image_rep = nil;
	
	if (!MCImageRepCreateWithData(p_data, t_image_rep))
	{
		// TODO - throw image rep error
		return;
	}
	
	MCCanvasImageMake(t_image_rep, r_image);
	MCImageRepRelease(t_image_rep);
}

// Input should be unpremultiplied ARGB pixels
void MCCanvasImageMakeWithPixels(integer_t p_width, integer_t p_height, MCDataRef p_pixels, MCCanvasImageRef &r_image)
{
	MCImageRep *t_image_rep;
	t_image_rep = nil;
	
	if (!MCImageRepCreateWithPixels(p_pixels, p_width, p_height, kMCGPixelFormatARGB, false, t_image_rep))
	{
		// TODO - throw image rep error
		return;
	}
	
	MCCanvasImageMake(t_image_rep, r_image);
	MCImageRepRelease(t_image_rep);
}

// Properties

void MCCanvasImageGetWidth(MCCanvasImageRef p_image, uint32_t &r_width)
{
	uint32_t t_width, t_height;
	if (!MCImageRepGetGeometry(MCCanvasImageGetImageRep(p_image), t_width, t_height))
	{
		// TODO - throw error
		return;
	}
	r_width = t_width;
}

void MCCanvasImageGetHeight(MCCanvasImageRef p_image, uint32_t &r_height)
{
	uint32_t t_width, t_height;
	if (!MCImageRepGetGeometry(MCCanvasImageGetImageRep(p_image), t_width, t_height))
	{
		// TODO - throw error
		return;
	}
	r_height = t_height;
}

void MCCanvasImageGetPixels(MCCanvasImageRef p_image, MCDataRef &r_pixels)
{
	bool t_success;
	t_success = true;
	
	MCImageRep *t_image_rep;
	t_image_rep = MCCanvasImageGetImageRep(p_image);
	
	MCImageBitmap *t_raster;
	
	// TODO - handle case of missing normal density image
	// TODO - throw appropriate errors
	if (MCImageRepLockRaster(t_image_rep, 0, 1.0, t_raster))
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
		
		MCImageRepUnlockRaster(t_image_rep, 0, t_raster);
	}
}

void MCCanvasImageGetMetadata(MCCanvasImageRef p_image, MCArrayRef &r_metadata)
{
	// TODO - implement image metadata
}

////////////////////////////////////////////////////////////////////////////////

// Paint

// defer to subtype finalize methods
static void __MCCanvasPaintDestroy(MCValueRef p_value)
{
	__MCCanvasPaintImpl *t_paint;
	t_paint = (__MCCanvasPaintImpl*)MCValueGetExtraBytesPtr(p_value);
	
	switch (t_paint->type)
	{
		case kMCCanvasPaintTypeSolid:
			MCCanvasSolidPaintDelete((__MCCanvasSolidPaintImpl *)t_paint);
			break;
			
		case kMCCanvasPaintTypePattern:
			MCCanvasPatternDelete((__MCCanvasPatternImpl *)t_paint);
			break;
			
		case kMCCanvasPaintTypeGradient:
			MCCanvasGradientDelete((__MCCanvasGradientImpl *)t_paint);
			break;
			
		default:
			MCAssert(false);
			
	}
}

static bool __MCCanvasPaintCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasPaintEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	__MCCanvasPaintImpl *t_left, *t_right;
	t_left = MCCanvasPaintGet((MCCanvasPaintRef)p_left);
	t_right = MCCanvasPaintGet((MCCanvasPaintRef)p_right);
	
	if (t_left->type != t_right->type)
		return false;
	
	switch (t_left->type)
	{
		case kMCCanvasPaintTypeSolid:
			return MCCanvasSolidPaintEqual((__MCCanvasSolidPaintImpl*)t_left, (__MCCanvasSolidPaintImpl*)t_right);
			
		case kMCCanvasPaintTypePattern:
			return MCCanvasPatternEqual((__MCCanvasPatternImpl*)t_left, (__MCCanvasPatternImpl*)t_right);
			
		case kMCCanvasPaintTypeGradient:
			return MCCanvasGradientEqual((__MCCanvasGradientImpl*)t_left, (__MCCanvasGradientImpl*)t_right);
	}
	
	MCAssert(false);
	return false;
}

static hash_t __MCCanvasPaintHash(MCValueRef p_value)
{
	__MCCanvasPaintImpl *t_paint;
	t_paint = (__MCCanvasSolidPaintImpl*)MCValueGetExtraBytesPtr(p_value);
	
	switch (t_paint->type)
	{
		case kMCCanvasPaintTypeSolid:
			return MCCanvasSolidPaintHash((__MCCanvasSolidPaintImpl*)t_paint);
			
		case kMCCanvasPaintTypePattern:
			return MCCanvasPatternHash((__MCCanvasPatternImpl*)t_paint);
			
		case kMCCanvasPaintTypeGradient:
			return MCCanvasGradientHash((__MCCanvasGradientImpl*)t_paint);
	}
	
	MCAssert(false);
	return 0;
}

static bool __MCCanvasPaintDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

bool MCCanvasPaintCreate(const __MCCanvasPaintImpl &p_paint, MCCanvasPaintRef &r_paint)
{
	bool t_success;
	t_success = true;
	
	MCCanvasPaintRef t_paint;
	t_paint = nil;
	
	size_t t_size;
	switch (p_paint.type)
	{
		case kMCCanvasPaintTypeSolid:
			t_size = sizeof(__MCCanvasSolidPaintImpl);
			break;
			
		case kMCCanvasPaintTypePattern:
			t_size = sizeof(__MCCanvasPatternImpl);
			break;
			
		case kMCCanvasPaintTypeGradient:
			t_size = sizeof(__MCCanvasGradientImpl);
			break;
			
		default:
			MCAssert(false);
			t_success = false;
	}
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasImageTypeInfo, t_size, t_paint);
	
	if (t_success)
	{
		*MCCanvasPaintGet(t_paint) = p_paint;
		t_success = MCValueInter(t_paint, r_paint);
	}
	
	MCValueRelease(t_paint);
	
	return t_success;
}

__MCCanvasPaintImpl *MCCanvasPaintGet(MCCanvasPaintRef p_paint)
{
	return (__MCCanvasPaintImpl*)MCValueGetExtraBytesPtr(p_paint);
}

//////////

// Solid Paint

bool MCCanvasPaintIsSolid(MCCanvasPaintRef p_paint)
{
	return MCCanvasPaintGet(p_paint)->type == kMCCanvasPaintTypeSolid;
}

void MCCanvasSolidPaintDelete(__MCCanvasSolidPaintImpl *p_paint)
{
	MCValueRelease(p_paint->color);
}

bool MCCanvasSolidPaintEqual(__MCCanvasSolidPaintImpl *p_left, __MCCanvasSolidPaintImpl *p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCValueIsEqualTo(p_left->color, p_right->color);
}

hash_t MCCanvasSolidPaintHash(__MCCanvasSolidPaintImpl *p_paint)
{
	return MCValueHash(p_paint->color);
}

bool MCCanvasSolidPaintCreateWithColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &r_paint)
{
	bool t_success;
	t_success = true;
	
	MCCanvasSolidPaintRef t_paint;
	t_paint = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasPaintTypeInfo, sizeof(__MCCanvasSolidPaintImpl), t_paint);
	
	if (t_success)
	{
		__MCCanvasSolidPaintImpl *t_impl;
		t_impl = MCCanvasSolidPaintGet(t_paint);
		
		t_impl->type = kMCCanvasPaintTypeSolid;
		t_impl->color = MCValueRetain(p_color);

		t_success = MCValueInter(t_paint, r_paint);
	}
	
	MCValueRelease(t_paint);
	
	return t_success;
}

__MCCanvasSolidPaintImpl *MCCanvasSolidPaintGet(MCCanvasSolidPaintRef p_paint)
{
	return (__MCCanvasSolidPaintImpl*)MCValueGetExtraBytesPtr(p_paint);
}

// Constructor

void MCCanvasSolidPaintMakeWithColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &r_paint)
{
	MCCanvasSolidPaintRef t_paint;
	t_paint = nil;
	
	if (!MCCanvasSolidPaintCreateWithColor(p_color, t_paint))
	{
		// TODO - throw memory error
		return;
	}
	
	MCValueAssign(r_paint, t_paint);
	MCValueRelease(t_paint);
}

// Properties

void MCCanvasSolidPaintGetColor(MCCanvasSolidPaintRef p_paint, MCCanvasColorRef &r_color)
{
	if (!MCCanvasPaintIsSolid(p_paint))
	{
		// TODO - throw paint type error
		return;
	}
	
	r_color = MCValueRetain(MCCanvasSolidPaintGet(p_paint)->color);
}

void MCCanvasSolidPaintSetColor(MCCanvasSolidPaintRef &x_paint, MCCanvasColorRef p_color)
{
	if (!MCCanvasPaintIsSolid(x_paint))
	{
		// TODO - throw paint type error
		return;
	}
	
	MCCanvasSolidPaintMakeWithColor(p_color, x_paint);
}

////////////////////////////////////////////////////////////////////////////////

// Pattern

bool MCCanvasPaintIsPattern(MCCanvasPaintRef p_paint)
{
	return MCCanvasPaintGet(p_paint)->type == kMCCanvasPaintTypePattern;
}

void MCCanvasPatternDelete(__MCCanvasPatternImpl *p_pattern)
{
	MCValueRelease(p_pattern->image);
}

bool MCCanvasPatternEqual(__MCCanvasPatternImpl *p_left, __MCCanvasPatternImpl *p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCValueIsEqualTo(p_left->image, p_right->image);
}

hash_t MCCanvasPatternHash(__MCCanvasPatternImpl *p_paint)
{
	// TODO - ask Mark how to combine hash values
	return MCValueHash(p_paint->image) ^ MCValueHash(p_paint->transform);
}

bool MCCanvasPatternCreateWithImage(MCCanvasImageRef p_image, MCCanvasTransformRef p_transform, MCCanvasPatternRef &r_paint)
{
	bool t_success;
	t_success = true;
	
	MCCanvasPatternRef t_paint;
	t_paint = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasPaintTypeInfo, sizeof(__MCCanvasPatternImpl), t_paint);
	
	if (t_success)
	{
		__MCCanvasPatternImpl *t_impl;
		t_impl = MCCanvasPatternGet(t_paint);
		
		t_impl->type = kMCCanvasPaintTypePattern;
		t_impl->image = MCValueRetain(p_image);
		t_impl->transform = MCValueRetain(p_transform);
		
		t_success = MCValueInter(t_paint, r_paint);
	}
	
	MCValueRelease(t_paint);
	
	return t_success;
}

__MCCanvasPatternImpl *MCCanvasPatternGet(MCCanvasPatternRef p_paint)
{
	return (__MCCanvasPatternImpl*)MCValueGetExtraBytesPtr(p_paint);
}

// Constructor

void MCCanvasPatternMakeWithTransformedImage(MCCanvasImageRef p_image, MCCanvasTransformRef p_transform, MCCanvasPatternRef &r_pattern)
{
	MCCanvasPatternRef t_pattern;
	t_pattern = nil;
	
	if (!MCCanvasPatternCreateWithImage(p_image, p_transform, t_pattern))
	{
		// TODO - throw memory error
		return;
	}
	
	MCValueAssign(r_pattern, t_pattern);
	MCValueRelease(t_pattern);
}

void MCCanvasPatternMakeWithTransformedImage(MCCanvasImageRef p_image, const MCGAffineTransform &p_transform, MCCanvasPatternRef &r_pattern)
{
	MCCanvasTransformRef t_transform;
	t_transform = nil;
	
	MCCanvasTransformMake(p_transform, t_transform);
	if (!MCErrorIsPending())
		MCCanvasPatternMakeWithTransformedImage(p_image, t_transform, r_pattern);
	MCValueRelease(t_transform);
}

void MCCanvasPatternMakeWithImage(MCCanvasImageRef p_image, MCCanvasPatternRef &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, kMCCanvasIdentityTransform, r_pattern);
}

void MCCanvasPatternMakeWithScaledImage(MCCanvasImageRef p_image, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasPatternRef &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeScale(p_xscale, p_yscale), r_pattern);
}

void MCCanvasPatternMakeWithRotatedImage(MCCanvasImageRef p_image, MCCanvasFloat p_angle, MCCanvasPatternRef &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeRotation(p_angle), r_pattern);
}

void MCCanvasPatternMakeWithTranslatedImage(MCCanvasImageRef p_image, MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPatternRef &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeTranslation(p_x, p_y), r_pattern);
}

// Properties

void MCCanvasPatternGetImage(MCCanvasPatternRef p_pattern, MCCanvasImageRef &r_image)
{
	if (!MCCanvasPaintIsPattern(p_pattern))
	{
		// TODO - throw paint type error
		return;
	}
	
	r_image = MCValueRetain(MCCanvasPatternGet(p_pattern)->image);
}

void MCCanvasPatternSetImage(MCCanvasPatternRef &x_pattern, MCCanvasImageRef p_image)
{
	if (!MCCanvasPaintIsPattern(x_pattern))
	{
		// TODO - throw paint type error
		return;
	}
	
	MCCanvasPatternMakeWithTransformedImage(p_image, MCCanvasPatternGet(x_pattern)->transform, x_pattern);
}

void MCCanvasPatternGetTransform(MCCanvasPatternRef p_pattern, MCCanvasTransformRef &r_transform)
{
	if (!MCCanvasPaintIsPattern(p_pattern))
	{
		// TODO - throw paint type error
		return;
	}

	r_transform = MCCanvasPatternGet(p_pattern)->transform;
}

void MCCanvasPatternSetTransform(MCCanvasPatternRef &x_pattern, MCCanvasTransformRef p_transform)
{
	if (!MCCanvasPaintIsPattern(x_pattern))
	{
		// TODO - throw paint type error
		return;
	}

	MCCanvasPatternMakeWithTransformedImage(MCCanvasPatternGet(x_pattern)->image, p_transform, x_pattern);
}

// Operators

void MCCanvasPatternTransform(MCCanvasPatternRef &x_pattern, const MCGAffineTransform &p_transform)
{
	if (!MCCanvasPaintIsPattern(x_pattern))
	{
		// TODO - throw paint type error
		return;
	}
	
	MCCanvasTransformRef t_transform;
	t_transform = MCValueRetain(MCCanvasPatternGet(x_pattern)->transform);
	
	MCCanvasTransformConcat(t_transform, p_transform);
	
	if (!MCErrorIsPending())
		MCCanvasPatternSetTransform(x_pattern, t_transform);
	
	MCValueRelease(t_transform);
}

void MCCanvasPatternTransform(MCCanvasPatternRef &x_pattern, MCCanvasTransformRef p_transform)
{
	MCCanvasPatternTransform(x_pattern, *MCCanvasTransformGet(p_transform));
}

void MCCanvasPatternScale(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

void MCCanvasPatternRotate(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_angle)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeRotation(p_angle));
}

void MCCanvasPatternTranslate(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeTranslation(p_x, p_y));
}

////////////////////////////////////////////////////////////////////////////////

// Gradient Stop

static void __MCCanvasGradientStopDestroy(MCValueRef p_stop)
{
	MCValueRelease(MCCanvasGradientStopGet((MCCanvasGradientStopRef)p_stop)->color);
}

static bool __MCCanvasGradientStopCopy(MCValueRef p_image, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_image;
	else
		r_copy = MCValueRetain(p_image);
	
	return true;
}

bool __MCCanvasGradientStopEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	__MCCanvasGradientStopImpl *t_left, *t_right;
	t_left = (__MCCanvasGradientStopImpl*)MCValueGetExtraBytesPtr(p_left);
	t_right = (__MCCanvasGradientStopImpl*)MCValueGetExtraBytesPtr(p_right);
	
	return t_left->offset == t_right->offset && MCValueIsEqualTo(t_left->color, t_right->color);
}

static hash_t __MCCanvasGradientStopHash(MCValueRef p_value)
{
	__MCCanvasGradientStopImpl *t_stop;
	t_stop = (__MCCanvasGradientStopImpl*)MCValueGetExtraBytesPtr(p_value);

	return MCValueHash(t_stop->color) ^ MCHashDouble(t_stop->offset);
}

static bool __MCCanvasGradientStopDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

bool MCCanvasGradientStopCreate(MCCanvasFloat p_offset, MCCanvasColorRef p_color, MCCanvasGradientStopRef &r_stop)
{
	bool t_success;
	t_success = true;
	
	MCCanvasGradientStopRef t_stop;
	t_stop = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasGradientStopTypeInfo, sizeof(__MCCanvasGradientStopImpl), t_stop);
	
	if (t_success)
	{
		__MCCanvasGradientStopImpl *t_impl;
		t_impl = MCCanvasGradientStopGet(t_stop);
		t_impl->offset = p_offset;
		t_impl->color = MCValueRetain(p_color);
		t_success = MCValueInter(t_stop, r_stop);
	}
	
	MCValueRelease(t_stop);
	
	return t_success;
}

__MCCanvasGradientStopImpl *MCCanvasGradientStopGet(MCCanvasGradientStopRef p_stop)
{
	return (__MCCanvasGradientStopImpl*)MCValueGetExtraBytesPtr(p_stop);
}

// Constructors

void MCCanvasGradientStopMake(MCCanvasFloat p_offset, MCCanvasColorRef p_color, MCCanvasGradientStopRef &r_stop)
{
	MCCanvasGradientStopRef t_stop;
	t_stop = nil;
	
	if (!MCCanvasGradientStopCreate(p_offset, p_color, t_stop))
	{
		// TODO - throw memory error
		return;
	}
	
	MCValueAssign(r_stop, t_stop);
	MCValueRelease(t_stop);
}

// Gradient

bool MCCanvasPaintIsGradient(MCCanvasPaintRef p_paint)
{
	return MCCanvasPaintGet(p_paint)->type == kMCCanvasPaintTypeGradient;
}

void MCCanvasGradientDelete(__MCCanvasGradientImpl *p_gradient)
{
	MCValueRelease(p_gradient->ramp);
	MCValueRelease(p_gradient->transform);
}

bool MCCanvasGradientEqual(__MCCanvasGradientImpl *p_left, __MCCanvasGradientImpl *p_right)
{
	if (p_left == p_right)
		return true;
	
	return p_left->function == p_right->function &&
	MCValueIsEqualTo(p_left->ramp, p_right->ramp) &&
	p_left->mirror == p_right->mirror &&
	p_left->wrap == p_right->wrap &&
	p_left->repeats == p_right->repeats &&
	MCValueIsEqualTo(p_left->transform, p_right->transform) &&
	p_left->filter == p_right->filter;
}

hash_t MCCanvasGradientHash(__MCCanvasGradientImpl *p_paint)
{
	// TODO - ask Mark how to combine hash values
	return MCHashInteger(p_paint->mirror | (p_paint->wrap < 1)) ^ MCHashInteger(p_paint->filter) ^ MCValueHash(p_paint->ramp) ^ MCHashInteger(p_paint->repeats) ^ MCValueHash(p_paint->transform) ^ MCHashInteger(p_paint->filter);
}

bool MCCanvasGradientCreate(const __MCCanvasGradientImpl &p_gradient, MCCanvasGradientRef &r_paint)
{
	bool t_success;
	t_success = true;
	
	MCCanvasGradientRef t_paint;
	t_paint = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasPaintTypeInfo, sizeof(__MCCanvasGradientImpl), t_paint);
	
	if (t_success)
	{
		__MCCanvasGradientImpl *t_impl;
		t_impl = MCCanvasGradientGet(t_paint);
		
		*t_impl = p_gradient;
		MCValueRetain(t_impl->ramp);
		MCValueRetain(t_impl->transform);
		
		t_success = MCValueInter(t_paint, r_paint);
	}
	
	MCValueRelease(t_paint);
	
	return t_success;
}

__MCCanvasGradientImpl *MCCanvasGradientGet(MCCanvasGradientRef p_paint)
{
	return (__MCCanvasGradientImpl*)MCValueGetExtraBytesPtr(p_paint);
}

// Gradient

bool MCProperListFetchGradientStopAt(MCProperListRef p_list, uindex_t p_index, MCCanvasGradientStopRef &r_stop)
{
	if (p_index >= MCProperListGetLength(p_list))
		return false;
	
	MCValueRef t_value;
	t_value = MCProperListFetchElementAtIndex(p_list, p_index);
	
	if (MCValueGetTypeInfo(t_value) != kMCCanvasGradientStopTypeInfo)
		return false;
	
	r_stop = (MCCanvasGradientStopRef)t_value;
	
	return true;
}

bool MCCanvasGradientCheckStopOrder(MCProperListRef p_ramp)
{
	uindex_t t_length;
	t_length = MCProperListGetLength(p_ramp);

	if (t_length == 0)
		return true;
	
	MCCanvasGradientStopRef t_prev_stop;
	if (!MCProperListFetchGradientStopAt(p_ramp, 0, t_prev_stop))
		return false;
	
	for (uint32_t i = 1; i < t_length; i++)
	{
		MCCanvasGradientStopRef t_stop;
		if (!MCProperListFetchGradientStopAt(p_ramp, i, t_stop))
			return false;
		
		if (MCCanvasGradientStopGet(t_stop)->offset < MCCanvasGradientStopGet(t_prev_stop)->offset)
		{
			// TODO - throw stop offset order error
			return false;
		}
	}
	
	return true;
}

// Constructor

void MCCanvasGradientMake(const __MCCanvasGradientImpl &p_gradient, MCCanvasGradientRef &r_gradient)
{
	MCCanvasGradientRef t_gradient;
	if (!MCCanvasGradientCreate(p_gradient, t_gradient))
	{
		// TODO - throw memory error
		return;
	}
	
	MCValueAssign(r_gradient, t_gradient);
	MCValueRelease(t_gradient);
}

void MCCanvasGradientMakeWithRamp(integer_t p_type, MCProperListRef p_ramp, MCCanvasGradientRef &r_gradient)
{
	MCCanvasGradientRef t_gradient;
	
	if (!MCCanvasGradientCheckStopOrder(p_ramp))
	{
		return;
	}
	
	__MCCanvasGradientImpl t_gradient_impl;
	MCMemoryClear(&t_gradient_impl, sizeof(__MCCanvasGradientImpl));
	t_gradient_impl.type = kMCCanvasPaintTypeGradient;
	
	t_gradient_impl.function = (MCGGradientFunction)p_type;
	t_gradient_impl.mirror = false;
	t_gradient_impl.wrap = false;
	t_gradient_impl.repeats = 1; // TODO - check default
	t_gradient_impl.transform = kMCCanvasIdentityTransform;
	t_gradient_impl.filter = kMCGImageFilterNone;
	
	MCCanvasGradientMake(t_gradient_impl, r_gradient);
}

// Properties

void MCCanvasGradientGetRamp(MCCanvasGradientRef p_gradient, MCProperListRef &r_ramp)
{
	if (!MCCanvasPaintIsGradient(p_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	r_ramp = MCValueRetain(MCCanvasGradientGet(p_gradient)->ramp);
}

void MCCanvasGradientSetRamp(MCCanvasGradientRef &x_gradient, MCProperListRef p_ramp)
{
	if (!MCCanvasPaintIsGradient(x_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	if (!MCCanvasGradientCheckStopOrder(p_ramp))
		return;
	
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	t_gradient.ramp = p_ramp;
	
	MCCanvasGradientMake(t_gradient, x_gradient);
}

void MCCanvasGradientGetTypeAsString(MCCanvasGradientRef p_gradient, MCStringRef &r_string)
{
	if (!MCCanvasPaintIsGradient(p_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	/* UNCHECKED */ MCCanvasGradientTypeToString(MCCanvasGradientGet(p_gradient)->function, r_string);
}

void MCCanvasGradientSetTypeAsString(MCCanvasGradientRef &x_gradient, MCStringRef p_string)
{
	if (!MCCanvasPaintIsGradient(x_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	if (!MCCanvasGradientTypeFromString(p_string, t_gradient.function))
	{
		// TODO - throw gradient type error
		return;
	}
	
	MCCanvasGradientMake(t_gradient, x_gradient);
}

void MCCanvasGradientGetRepeat(MCCanvasGradientRef p_gradient, integer_t &r_repeat)
{
	if (!MCCanvasPaintIsGradient(p_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	r_repeat = MCCanvasGradientGet(p_gradient)->repeats;
}

void MCCanvasGradientSetRepeat(MCCanvasGradientRef &x_gradient, integer_t p_repeat)
{
	if (!MCCanvasPaintIsGradient(x_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	t_gradient.repeats = p_repeat;
	
	MCCanvasGradientMake(t_gradient, x_gradient);
}

void MCCanvasGradientGetWrap(MCCanvasGradientRef p_gradient, bool &r_wrap)
{
	if (!MCCanvasPaintIsGradient(p_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	r_wrap = MCCanvasGradientGet(p_gradient)->wrap;
}

void MCCanvasGradientSetWrap(MCCanvasGradientRef &x_gradient, bool p_wrap)
{
	if (!MCCanvasPaintIsGradient(x_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	t_gradient.wrap = p_wrap;
	
	MCCanvasGradientMake(t_gradient, x_gradient);
}

void MCCanvasGradientGetMirror(MCCanvasGradientRef p_gradient, bool &r_mirror)
{
	if (!MCCanvasPaintIsGradient(p_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	r_mirror = MCCanvasGradientGet(p_gradient)->mirror;
}

void MCCanvasGradientSetMirror(MCCanvasGradientRef &x_gradient, bool p_mirror)
{
	if (!MCCanvasPaintIsGradient(x_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	t_gradient.mirror = p_mirror;
	
	MCCanvasGradientMake(t_gradient, x_gradient);
}

void MCCanvasGradientTransformToPoints(const MCGAffineTransform &p_transform, MCGPoint &r_from, MCGPoint &r_to, MCGPoint &r_via)
{
	r_from = MCGPointApplyAffineTransform(MCGPointMake(0, 0), p_transform);
	r_to = MCGPointApplyAffineTransform(MCGPointMake(0, 1), p_transform);
	r_via = MCGPointApplyAffineTransform(MCGPointMake(1, 0), p_transform);
}

bool MCCanvasGradientTransformFromPoints(const MCGPoint &p_from, const MCGPoint &p_to, const MCGPoint &p_via, MCGAffineTransform &r_transform)
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

void MCCanvasGradientGetTransform(MCCanvasGradientRef p_gradient, MCGAffineTransform &r_transform)
{
	r_transform = *MCCanvasTransformGet(MCCanvasGradientGet(p_gradient)->transform);
}

void MCCanvasGradientSetTransform(MCCanvasGradientRef &x_gradient, const MCGAffineTransform &p_transform)
{
	MCCanvasTransformRef t_transform;
	t_transform = nil;
	
	MCCanvasTransformMake(p_transform, t_transform);
	if (!MCErrorIsPending())
		MCCanvasGradientSetTransform(x_gradient, t_transform);
	MCValueRelease(t_transform);
}

void MCCanvasGradientGetPoints(MCCanvasGradientRef p_gradient, MCGPoint &r_from, MCGPoint &r_to, MCGPoint &r_via)
{
	MCGAffineTransform t_transform;
	MCCanvasGradientGetTransform(p_gradient, t_transform);
	MCCanvasGradientTransformToPoints(t_transform, r_from, r_to, r_via);
}

void MCCanvasGradientSetPoints(MCCanvasGradientRef &x_gradient, const MCGPoint &p_from, const MCGPoint &p_to, const MCGPoint &p_via)
{
	MCGAffineTransform t_transform;
	if (!MCCanvasGradientTransformFromPoints(p_from, p_to, p_via, t_transform))
	{
		// TODO - throw error
	}
	MCCanvasGradientSetTransform(x_gradient, t_transform);
}

void MCCanvasGradientGetFrom(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_from)
{
	if (!MCCanvasPaintIsGradient(p_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	MCCanvasPointMakeWithMCGPoint(t_from, r_from);
}

void MCCanvasGradientGetTo(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_to)
{
	if (!MCCanvasPaintIsGradient(p_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	MCCanvasPointMakeWithMCGPoint(t_to, r_to);
}

void MCCanvasGradientGetVia(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_via)
{
	if (!MCCanvasPaintIsGradient(p_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	MCCanvasPointMakeWithMCGPoint(t_via, r_via);
}

void MCCanvasGradientSetFrom(MCCanvasGradientRef &x_gradient, MCCanvasPointRef p_from)
{
	if (!MCCanvasPaintIsGradient(x_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(x_gradient, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, *MCCanvasPointGet(p_from), t_to, t_via);
}

void MCCanvasGradientSetTo(MCCanvasGradientRef &x_gradient, MCCanvasPointRef p_to)
{
	if (!MCCanvasPaintIsGradient(x_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(x_gradient, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, t_from, *MCCanvasPointGet(p_to), t_via);
}

void MCCanvasGradientSetVia(MCCanvasGradientRef &x_gradient, MCCanvasPointRef p_via)
{
	if (!MCCanvasPaintIsGradient(x_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(x_gradient, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, t_from, t_to, *MCCanvasPointGet(p_via));
}

void MCCanvasGradientGetTransform(MCCanvasGradientRef p_gradient, MCCanvasTransformRef &r_transform)
{
	r_transform = MCValueRetain(MCCanvasGradientGet(p_gradient)->transform);
}

void MCCanvasGradientSetTransform(MCCanvasGradientRef &x_gradient, MCCanvasTransformRef p_transform)
{
	if (!MCCanvasPaintIsGradient(x_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	t_gradient.transform = p_transform;
	MCCanvasGradientMake(t_gradient, x_gradient);
}

// Operators

void MCCanvasGradientAddStop(MCCanvasGradientRef &x_gradient, MCCanvasGradientStopRef p_stop)
{
	if (!MCCanvasPaintIsGradient(x_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	bool t_success;
	t_success = true;
	
	__MCCanvasGradientStopImpl *t_new_stop;
	t_new_stop = MCCanvasGradientStopGet(p_stop);
	
	if (t_new_stop->offset < 0 || t_new_stop->offset > 1)
	{
		// TODO - throw offset range error
		return;
	}
	
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	if (MCProperListGetLength(t_gradient.ramp) > 0)
	{
		MCCanvasGradientStopRef t_last_stop;
		/* UNCHECKED */ MCProperListFetchGradientStopAt(t_gradient.ramp, MCProperListGetLength(t_gradient.ramp) - 1, t_last_stop);
		
		if (t_new_stop->offset < MCCanvasGradientStopGet(t_last_stop)->offset)
		{
			// TODO - throw stop order error
			return;
		}
	}
	
	MCProperListRef t_mutable_ramp;
	t_mutable_ramp = nil;
	
	if (!MCProperListMutableCopy(t_gradient.ramp, t_mutable_ramp))
	{
		// TODO - throw memory error
		return;
	}
	
	if (!MCProperListPushElementOntoBack(t_mutable_ramp, p_stop))
	{
		// TODO - throw memory error
		MCValueRelease(t_mutable_ramp);
		return;
	}
	
	MCProperListRef t_new_ramp;
	t_new_ramp = nil;
	
	if (!MCProperListCopyAndRelease(t_mutable_ramp, t_new_ramp))
	{
		// TODO - throw memory error
		MCValueRelease(t_mutable_ramp);
		return;
	}
	
	t_gradient.ramp = t_new_ramp;
	
	MCCanvasGradientMake(t_gradient, x_gradient);
	
	MCValueRelease(t_new_ramp);
}

void MCCanvasGradientTransform(MCCanvasGradientRef &x_gradient, const MCGAffineTransform &p_transform)
{
	if (!MCCanvasPaintIsGradient(x_gradient))
	{
		// TODO - throw paint type error
		return;
	}
	
	MCCanvasTransformRef t_transform;
	t_transform = MCValueRetain(MCCanvasGradientGet(x_gradient)->transform);
	
	MCCanvasTransformConcat(t_transform, p_transform);
	
	if (!MCErrorIsPending())
		MCCanvasGradientSetTransform(x_gradient, t_transform);
	
	MCValueRelease(t_transform);
}

void MCCanvasGradientTransform(MCCanvasGradientRef &x_gradient, MCCanvasTransformRef &p_transform)
{
	MCCanvasGradientTransform(x_gradient, *MCCanvasTransformGet(p_transform));
}

void MCCanvasGradientScale(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

void MCCanvasGradientRotate(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_angle)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeRotation(p_angle));
}

void MCCanvasGradientTranslate(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeTranslation(p_x, p_y));
}

////////////////////////////////////////////////////////////////////////////////

// Path

static void __MCCanvasPathDestroy(MCValueRef p_value)
{
	MCGPathRelease(MCCanvasPathGetMCGPath((MCCanvasPathRef) p_value));
}

static bool __MCCanvasPathCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasPathEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	// TODO - implememt MCGPath comparison
	return MCCanvasPathGetMCGPath((MCCanvasPathRef)p_left) == MCCanvasPathGetMCGPath((MCCanvasPathRef)p_right);
}

static hash_t __MCCanvasPathHash(MCValueRef p_value)
{
	// TODO - implement MCGPath hash
	return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCCanvasPathImpl));
}

static bool __MCCanvasPathDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

bool MCCanvasPathCreateWithMCGPath(MCGPathRef p_path, MCCanvasPathRef &r_path)
{
	bool t_success;
	t_success = true;
	
	MCCanvasPathRef t_path;
	t_path = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasPathTypeInfo, sizeof(__MCCanvasPathImpl), t_path);
	
	if (t_success)
	{
		*MCCanvasPathGet(t_path) = MCGPathRetain(p_path);
		t_success = MCValueInter(t_path, r_path);
	}
	
	MCValueRelease(t_path);
	
	return t_success;
}

__MCCanvasPathImpl *MCCanvasPathGet(MCCanvasPathRef p_path)
{
	return (__MCCanvasPathImpl*)MCValueGetExtraBytesPtr(p_path);
}

MCGPathRef MCCanvasPathGetMCGPath(MCCanvasPathRef p_path)
{
	return *MCCanvasPathGet(p_path);
}

//////////

bool MCCanvasPathParseInstructions(MCStringRef p_instructions, MCGPathIterateCallback p_callback, void *p_context);
bool MCCanvasPathUnparseInstructions(MCCanvasPathRef &p_path, MCStringRef &r_string);

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

void MCCanvasPathMakeWithMCGPath(MCGPathRef p_path, MCCanvasPathRef &r_path)
{
	MCCanvasPathRef t_path;
	t_path = nil;
	
	if (!MCCanvasPathCreateWithMCGPath(p_path, t_path))
	{
		// TODO - throw memory error;
		return;
	}
	
	MCValueAssign(r_path, t_path);
	MCValueRelease(t_path);
}

void MCCanvasPathMakeWithInstructionsAsString(MCStringRef p_instructions, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
	{
		// TODO - throw memory error
		return;
	}
	
	if (!MCCanvasPathParseInstructions(p_instructions, MCCanvasPathMakeWithInstructionsCallback, t_path))
		return;
	
	MCCanvasPathMakeWithMCGPath(t_path, r_path);
	MCGPathRelease(t_path);
}

void MCCanvasPathMakeWithRoundedRectangle(MCCanvasRectangleRef p_rect, MCCanvasFloat p_x_radius, MCCanvasFloat p_y_radius, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
	{
		// TODO - throw memory error
		return;
	}
	
	MCGPathAddRoundedRectangle(t_path, *MCCanvasRectangleGet(p_rect), MCGSizeMake(p_x_radius, p_y_radius));
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		MCGPathRelease(t_path);
		return;
	}
	
	MCCanvasPathMakeWithMCGPath(t_path, r_path);
	MCGPathRelease(t_path);
}

void MCCanvasPathMakeWithRectangle(MCCanvasRectangleRef p_rect, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
	{
		// TODO - throw memory error
		return;
	}
	
	/* UNCHECKED */ MCGPathAddRectangle(t_path, *MCCanvasRectangleGet(p_rect));
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		MCGPathRelease(t_path);
		return;
	}
	
	MCCanvasPathMakeWithMCGPath(t_path, r_path);
	MCGPathRelease(t_path);
}

void MCCanvasPathMakeWithEllipse(MCCanvasPointRef p_center, MCCanvasFloat p_radius_x, MCCanvasFloat p_radius_y, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
	{
		// TODO - throw memory error
		return;
	}
	
	/* UNCHECKED */ MCGPathAddEllipse(t_path, *MCCanvasPointGet(p_center), MCGSizeMake(p_radius_x, p_radius_y), 0);
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		MCGPathRelease(t_path);
		return;
	}
	
	MCCanvasPathMakeWithMCGPath(t_path, r_path);
	MCGPathRelease(t_path);
}

void MCCanvasPathMakeWithLine(MCCanvasPointRef p_start, MCCanvasPointRef p_end, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
	{
		// TODO - throw memory error
		return;
	}
	
	/* UNCHECKED */ MCGPathAddLine(t_path, *MCCanvasPointGet(p_start), *MCCanvasPointGet(p_end));
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		MCGPathRelease(t_path);
		return;
	}
	
	MCCanvasPathMakeWithMCGPath(t_path, r_path);
	MCGPathRelease(t_path);
}

bool MCCanvasPointsListToMCGPoints(MCProperListRef p_points, MCGPoint *r_points)
{
	bool t_success;
	t_success = true;
	
	MCGPoint *t_points;
	t_points = nil;
	
	uint32_t t_point_count;
	t_point_count = MCProperListGetLength(p_points);
	
	if (t_success)
	{
		if (!MCMemoryNewArray(t_point_count, t_points))
		{
			// TODO - throw memory error
			t_success = false;
		}
	}
	
	for (uint32_t i = 0; t_success && i < t_point_count; i++)
	{
		MCValueRef t_value;
		t_value = MCProperListFetchElementAtIndex(p_points, t_point_count);
		
		if (MCValueGetTypeInfo(t_value) == kMCCanvasPointTypeInfo)
		{
			MCCanvasPointGetMCGPoint((MCCanvasPointRef)t_value, t_points[i]);
		}
		else
		{
			// TODO - throw point type error
			t_success = false;
		}
	}
	
	if (t_success)
		r_points = t_points;
	else
		MCMemoryDeleteArray(t_points);
	
	return t_success;
}

void MCCanvasPathMakeWithPoints(MCProperListRef p_points, bool p_close, MCCanvasPathRef &r_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		if (!MCGPathCreateMutable(t_path))
		{
			// TODO - throw memory error
			t_success = false;
		}
	}
	
	MCGPoint *t_points;
	t_points = nil;
	
	if (t_success)
		t_success = MCCanvasPointsListToMCGPoints(p_points, t_points);
	{
		if (!MCMemoryNewArray(MCProperListGetLength(p_points), t_points))
		{
			// TODO - throw memory error
			t_success = false;
		}
	}
	
	if (t_success)
	{
		if (p_close)
			MCGPathAddPolygon(t_path, t_points, MCProperListGetLength(p_points));
		else
			MCGPathAddPolyline(t_path, t_points, MCProperListGetLength(p_points));
		
		if (!MCGPathIsValid(t_path))
		{
			// TODO - throw memory error
			t_success = false;
		}
	}
	
	if (t_success)
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
	MCMemoryDeleteArray(t_points);
}

// Properties

void MCCanvasPathGetSubpaths(MCCanvasPathRef p_path, integer_t p_start, integer_t p_end, MCCanvasPathRef &r_subpaths)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathMutableCopySubpaths(*MCCanvasPathGet(p_path), p_start, p_end, t_path))
	{
		// TODO - throw error
		return;
	}
	
	MCCanvasPathMakeWithMCGPath(t_path, r_subpaths);
	MCGPathRelease(t_path);
}

void MCCanvasPathGetBoundingBox(MCCanvasPathRef p_path, MCCanvasRectangleRef &r_bounds)
{
	MCGRectangle t_rect;
	MCGPathGetBoundingBox(*MCCanvasPathGet(p_path), t_rect);
	MCCanvasRectangleMakeWithMCGRectangle(t_rect, r_bounds);
}

void MCCanvasPathGetInstructionsAsString(MCCanvasPathRef p_path, MCStringRef &r_instruction_string)
{
	/* UNCHECKED */ MCCanvasPathUnparseInstructions(p_path, r_instruction_string);
}

// Operations

void MCCanvasPathTransform(MCCanvasPathRef &x_path, const MCGAffineTransform &p_transform)
{
	// Path transformations are applied immediately
	MCGPathRef t_path;
	t_path = nil;

	MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		return;
	}
	
	if (!MCGPathTransform(t_path, p_transform))
	{
		// TODO - throw memory error
		MCGPathRelease(t_path);
	}
	
	MCCanvasPathMakeWithMCGPath(t_path, x_path);
	MCGPathRelease(t_path);
}

void MCCanvasPathTransform(MCCanvasPathRef &x_path, MCCanvasTransformRef p_transform)
{
	MCCanvasPathTransform(x_path, *MCCanvasTransformGet(p_transform));
}

void MCCanvasPathScale(MCCanvasPathRef &x_path, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

void MCCanvasPathRotate(MCCanvasPathRef &x_path, MCCanvasFloat p_angle)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeRotation(p_angle));
}

void MCCanvasPathTranslate(MCCanvasPathRef &x_path, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeTranslation(p_x, p_y));
}

void MCCanvasPathAddPath(MCCanvasPathRef &x_path, const MCCanvasPathRef p_to_add)
{
	MCGPathRef t_path;
	t_path = nil;
	
	MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		return;
	}
	
	MCGPathAddPath(t_path, *MCCanvasPathGet(p_to_add));
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		MCGPathRelease(t_path);
		return;
	}
	
	MCCanvasPathMakeWithMCGPath(t_path, x_path);
	MCGPathRelease(t_path);
}

void MCCanvasPathMoveTo(MCCanvasPathRef &x_path, MCCanvasPointRef p_point)
{
	MCGPathRef t_path;
	t_path = nil;
	
	MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		return;
	}
	
	MCGPathMoveTo(t_path, *MCCanvasPointGet(p_point));
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		MCGPathRelease(t_path);
		return;
	}
	
	MCCanvasPathMakeWithMCGPath(t_path, x_path);
	MCGPathRelease(t_path);
}

void MCCanvasPathLineTo(MCCanvasPathRef &x_path, MCCanvasPointRef p_point)
{
	MCGPathRef t_path;
	t_path = nil;
	
	MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		return;
	}
	
	MCGPathLineTo(t_path, *MCCanvasPointGet(p_point));
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		MCGPathRelease(t_path);
		return;
	}
	
	MCCanvasPathMakeWithMCGPath(t_path, x_path);
	MCGPathRelease(t_path);
}

void MCCanvasPathCurveThroughPoint(MCCanvasPathRef &x_path, MCCanvasPointRef p_through, MCCanvasPointRef p_to)
{
	MCGPathRef t_path;
	t_path = nil;
	
	MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		return;
	}
	
	MCGPathQuadraticTo(t_path, *MCCanvasPointGet(p_through), *MCCanvasPointGet(p_to));
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		MCGPathRelease(t_path);
		return;
	}
	
	MCCanvasPathMakeWithMCGPath(t_path, x_path);
	MCGPathRelease(t_path);
}

void MCCanvasPathCurveThroughPoints(MCCanvasPathRef &x_path, MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to)
{
	MCGPathRef t_path;
	t_path = nil;
	
	MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		return;
	}
	
	MCGPathCubicTo(t_path, *MCCanvasPointGet(p_through_a), *MCCanvasPointGet(p_through_b), *MCCanvasPointGet(p_to));
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		MCGPathRelease(t_path);
		return;
	}
	
	MCCanvasPathMakeWithMCGPath(t_path, x_path);
	MCGPathRelease(t_path);
}

void MCCanvasPathClosePath(MCCanvasPathRef &x_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		return;
	}
	
	/* UNCHECKED */ MCGPathCloseSubpath(t_path);
	if (!MCGPathIsValid(t_path))
	{
		// TODO - throw memory error
		MCGPathRelease(t_path);
		return;
	}
	
	MCCanvasPathMakeWithMCGPath(t_path, x_path);
	MCGPathRelease(t_path);
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

bool MCCanvasPathUnparseInstructions(MCCanvasPathRef &p_path, MCStringRef &r_string)
{
	MCGPathRef t_path;
	t_path = MCCanvasPathGetMCGPath(p_path);
	
	if (MCGPathIsEmpty(t_path))
		return MCStringCopy(kMCEmptyString, r_string);
	
	bool t_success;
	t_success = true;
	
	MCStringRef t_string;
	t_string = nil;
	
	if (t_success)
		t_success = MCStringCreateMutable(0, t_string);
	
	if (t_success)
		t_success = MCGPathIterate(t_path, MCCanvasPathUnparseInstructionsCallback, t_string);
	
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
			// TODO - throw path parse error
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

static void __MCCanvasEffectDestroy(MCValueRef p_value)
{
	MCValueRelease(MCCanvasEffectGet((MCCanvasEffectRef)p_value)->color);
}

static bool __MCCanvasEffectCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasEffectEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	__MCCanvasEffectImpl *t_left, *t_right;
	t_left = MCCanvasEffectGet((MCCanvasEffectRef)p_left);
	t_right = MCCanvasEffectGet((MCCanvasEffectRef)p_right);
	
	if (t_left->type != t_right->type)
		return false;
	
	if (!MCValueIsEqualTo(t_left->color, t_right->color) || t_left->opacity != t_right->opacity || t_left->blend_mode != t_right->blend_mode)
		return false;
	
	switch (t_left->type)
	{
		case kMCCanvasEffectTypeColorOverlay:
			break;
			
		case kMCCanvasEffectTypeInnerGlow:
		case kMCCanvasEffectTypeOuterGlow:
			if (t_left->size != t_right->size || t_left->spread != t_right->spread)
				return false;
			break;
			
		case kMCCanvasEffectTypeInnerShadow:
		case kMCCanvasEffectTypeOuterShadow:
			if (t_left->size != t_right->size || t_left->spread != t_right->spread)
				return false;
			if (t_left->distance != t_right->distance || t_left->angle != t_right->angle)
				return false;
			break;
	}
	
	return true;
}

static hash_t __MCCanvasEffectHash(MCValueRef p_value)
{
	__MCCanvasEffectImpl *t_effect;
	t_effect = MCCanvasEffectGet((MCCanvasEffectRef)p_value);
	
	hash_t t_hash;
	t_hash = MCValueHash(t_effect->color) ^ MCHashInteger(t_effect->blend_mode) ^ MCHashDouble(t_effect->opacity);
	
	switch (t_effect->type)
	{
		case kMCCanvasEffectTypeColorOverlay:
			break;
			
		case kMCCanvasEffectTypeInnerGlow:
		case kMCCanvasEffectTypeOuterGlow:
			t_hash ^= MCHashDouble(t_effect->size) ^ MCHashDouble(t_effect->spread);
			break;
			
		case kMCCanvasEffectTypeInnerShadow:
		case kMCCanvasEffectTypeOuterShadow:
			t_hash ^= MCHashDouble(t_effect->size) ^ MCHashDouble(t_effect->spread);
			t_hash ^= MCHashDouble(t_effect->distance) ^ MCHashDouble(t_effect->angle);
			break;
	}
	
	return t_hash;
}

static bool __MCCanvasEffectDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

bool MCCanvasEffectCreate(const __MCCanvasEffectImpl &p_effect, MCCanvasEffectRef &r_effect)
{
	bool t_success;
	t_success = true;
	
	MCCanvasEffectRef t_effect;
	t_effect = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasEffectTypeInfo, sizeof(__MCCanvasEffectImpl), t_effect);
	
	if (t_success)
	{
		*MCCanvasEffectGet(t_effect) = p_effect;
		MCValueRetain(MCCanvasEffectGet(t_effect)->color);
		t_success = MCValueInter(t_effect, r_effect);
	}
	
	MCValueRelease(t_effect);
	
	return t_success;
}

__MCCanvasEffectImpl *MCCanvasEffectGet(MCCanvasEffectRef p_effect)
{
	return (__MCCanvasEffectImpl*)MCValueGetExtraBytesPtr(p_effect);
}

// Constructors

void MCCanvasEffectMake(const __MCCanvasEffectImpl &p_effect, MCCanvasEffectRef &r_effect)
{
	MCCanvasEffectRef t_effect;
	t_effect = nil;
	
	if (!MCCanvasEffectCreate(p_effect, t_effect))
	{
		// TODO - throw memory error
		return;
	}
	
	MCValueAssign(r_effect, t_effect);
	MCValueRelease(t_effect);
}

void MCCanvasGraphicEffectMakeWithPropertyArray(integer_t p_type, MCArrayRef p_properties, MCCanvasEffectRef &r_effect)
{
	// TODO - defaults for missing properties?
	bool t_success;
	t_success = true;
	
	__MCCanvasEffectImpl t_effect;
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
	
	if (t_success)
		t_success = MCArrayFetchCanvasColor(p_properties, s_effect_property_map[kMCCanvasEffectPropertyColor], t_effect.color);
	
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
		MCCanvasEffectMake(t_effect, r_effect);
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

void MCCanvasGraphicEffectGetTypeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_type)
{
	/* UNCHECKED */ MCCanvasEffectTypeToString(MCCanvasEffectGet(p_effect)->type, r_type);
}

void MCCanvasGraphicEffectGetColor(MCCanvasEffectRef p_effect, MCCanvasColorRef &r_color)
{
	r_color = MCValueRetain(MCCanvasEffectGet(p_effect)->color);
}

void MCCanvasGraphicEffectSetColor(MCCanvasEffectRef &x_effect, MCCanvasColorRef p_color)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	t_effect.color = p_color;
	MCCanvasEffectMake(t_effect, x_effect);
}

void MCCanvasEffectGetBlendModeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_blend_mode)
{
	/* UNCHECKED */ MCCanvasBlendModeToString(MCCanvasEffectGet(p_effect)->blend_mode, r_blend_mode);
}

void MCCanvasEffectSetBlendModeAsString(MCCanvasEffectRef &x_effect, MCStringRef p_blend_mode)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	if (!MCCanvasBlendModeFromString(p_blend_mode, t_effect.blend_mode))
	{
		// TODO - throw blend mode error
		return;
	}
	MCCanvasEffectMake(t_effect, x_effect);
}

void MCCanvasEffectGetOpacity(MCCanvasEffectRef p_effect, MCCanvasFloat &r_opacity)
{
	r_opacity = MCCanvasEffectGet(p_effect)->opacity;
}

void MCCanvasEffectSetOpacity(MCCanvasEffectRef &x_effect, MCCanvasFloat p_opacity)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	t_effect.opacity = p_opacity;
	MCCanvasEffectMake(t_effect, x_effect);
}

void MCCanvasEffectGetSize(MCCanvasEffectRef p_effect, MCCanvasFloat &r_size)
{
	if (!MCCanvasEffectHasSizeAndSpread(MCCanvasEffectGet(p_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	r_size = MCCanvasEffectGet(p_effect)->size;
}

void MCCanvasEffectSetSize(MCCanvasEffectRef &x_effect, MCCanvasFloat p_size)
{
	if (!MCCanvasEffectHasSizeAndSpread(MCCanvasEffectGet(x_effect)->type))
	{
		// TODO - throw error
		return;
	}

	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	t_effect.size = p_size;
	MCCanvasEffectMake(t_effect, x_effect);
}

void MCCanvasEffectGetSpread(MCCanvasEffectRef p_effect, MCCanvasFloat &r_spread)
{
	if (!MCCanvasEffectHasSizeAndSpread(MCCanvasEffectGet(p_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	r_spread = MCCanvasEffectGet(p_effect)->spread;
}

void MCCanvasEffectSetSpread(MCCanvasEffectRef &x_effect, MCCanvasFloat p_spread)
{
	if (!MCCanvasEffectHasSizeAndSpread(MCCanvasEffectGet(x_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	t_effect.spread = p_spread;
	MCCanvasEffectMake(t_effect, x_effect);
}

void MCCanvasEffectGetDistance(MCCanvasEffectRef p_effect, MCCanvasFloat &r_distance)
{
	if (!MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectGet(p_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	r_distance = MCCanvasEffectGet(p_effect)->distance;
}

void MCCanvasEffectSetDistance(MCCanvasEffectRef &x_effect, MCCanvasFloat p_distance)
{
	if (!MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectGet(x_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	t_effect.distance = p_distance;
	MCCanvasEffectMake(t_effect, x_effect);
}

void MCCanvasEffectGetAngle(MCCanvasEffectRef p_effect, MCCanvasFloat &r_angle)
{
	if (!MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectGet(p_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	r_angle = MCCanvasEffectGet(p_effect)->angle;
}

void MCCanvasEffectSetAngle(MCCanvasEffectRef &x_effect, MCCanvasFloat p_angle)
{
	if (!MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectGet(x_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	t_effect.angle = p_angle;
	MCCanvasEffectMake(t_effect, x_effect);
}

////////////////////////////////////////////////////////////////////////////////

// Canvas

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
	
	MCCanvasPaintRef t_black_paint;
	t_black_paint = nil;
	
	if (t_success)
		t_success = MCCanvasSolidPaintCreateWithColor(kMCCanvasColorBlack, t_black_paint);
	
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
	t_properties.paint = MCValueRetain(p_src.paint);
	
	p_dst = t_properties;
	
	return true;
}

void MCCanvasPropertiesClear(MCCanvasProperties &p_properties)
{
	MCValueRelease(p_properties.paint);
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
		t_canvas_impl = MCCanvasGet(t_canvas);
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
	return MCCanvasGet(p_canvas)->props();
}

void MCCanvasGetPaint(MCCanvasRef p_canvas, MCCanvasPaintRef &r_paint)
{
	r_paint = MCValueRetain(MCCanvasGetProps(p_canvas).paint);
}

void MCCanvasSetPaint(MCCanvasRef &x_canvas, MCCanvasPaintRef p_paint)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(x_canvas);
	MCValueAssign(t_canvas->props().paint, p_paint);
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
	t_canvas = MCCanvasGet(x_canvas);
	
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
	t_canvas = MCCanvasGet(x_canvas);
	
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
	t_canvas = MCCanvasGet(x_canvas);
	
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
	t_canvas = MCCanvasGet(x_canvas);
	
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
	t_canvas = MCCanvasGet(x_canvas);
	
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
	t_canvas = MCCanvasGet(x_canvas);
	
	/* UNCHECKED */ MCCanvasImageFilterFromString(p_quality, t_canvas->props().image_filter);
	// need to re-apply pattern paint if quality changes
	if (MCCanvasPaintIsPattern(t_canvas->props().paint))
		t_canvas->paint_changed = true;
}

//////////

void MCCanvasApplySolidPaint(__MCCanvasImpl &x_canvas, const __MCCanvasSolidPaintImpl &p_paint)
{
	__MCCanvasColorImpl *t_color;
	MCCanvasFloat t_red, t_green, t_blue, t_alpha;
	t_color = MCCanvasColorGet(p_paint.color);
	
	MCGContextSetFillRGBAColor(x_canvas.context, t_color->red, t_color->green, t_color->blue, t_color->alpha);
	MCGContextSetStrokeRGBAColor(x_canvas.context, t_color->red, t_color->green, t_color->blue, t_color->alpha);
}

void MCCanvasApplyPatternPaint(__MCCanvasImpl &x_canvas, const __MCCanvasPatternImpl &p_pattern)
{
	MCGImageFrame t_frame;
	
	MCGAffineTransform t_pattern_transform;
	t_pattern_transform = *MCCanvasTransformGet(p_pattern.transform);
	
	MCImageRep *t_pattern_image;
	t_pattern_image = MCCanvasImageGetImageRep(p_pattern.image);
	
	MCGAffineTransform t_transform;
	
	MCGAffineTransform t_combined;
	t_combined = MCGAffineTransformConcat(t_pattern_transform, MCGContextGetDeviceTransform(x_canvas.context));
	
	MCGFloat t_scale;
	t_scale = MCGAffineTransformGetEffectiveScale(t_combined);
	
	if (MCImageRepLock(t_pattern_image, 0, t_scale, t_frame))
	{
		t_transform = MCGAffineTransformMakeScale(1.0 / t_frame.x_scale, 1.0 / t_frame.y_scale);
		
		// return image & transform scaled for image density
		t_transform = MCGAffineTransformConcat(t_pattern_transform, t_transform);
		

		MCGContextSetFillPattern(x_canvas.context, t_frame.image, t_transform, x_canvas.props().image_filter);
		MCGContextSetStrokePattern(x_canvas.context, t_frame.image, t_transform, x_canvas.props().image_filter);
		
		MCImageRepUnlock(t_pattern_image, 0, t_frame);
	}
	
}

bool MCCanvasApplyGradientPaint(__MCCanvasImpl &x_canvas, const __MCCanvasGradientImpl &p_gradient)
{
	bool t_success;
	t_success = true;
	
	MCGFloat *t_offsets;
	t_offsets = nil;
	
	MCGColor *t_colors;
	t_colors = nil;
	
	MCGAffineTransform t_gradient_transform;
	t_gradient_transform = *MCCanvasTransformGet(p_gradient.transform);
	
	uint32_t t_ramp_length;
	t_ramp_length = MCProperListGetLength(p_gradient.ramp);
	
	if (t_success)
		t_success = MCMemoryNewArray(t_ramp_length, t_offsets) && MCMemoryNewArray(t_ramp_length, t_colors);
	
	if (t_success)
	{
		for (uint32_t i = 0; i < t_ramp_length; i++)
		{
			MCCanvasGradientStopRef t_stop;
			/* UNCHECKED */ MCProperListFetchGradientStopAt(p_gradient.ramp, i, t_stop);
			t_offsets[i] = MCCanvasGradientStopGet(t_stop)->offset;
			t_colors[i] = MCCanvasColorToMCGColor(MCCanvasGradientStopGet(t_stop)->color);
		}
		
		MCGContextSetFillGradient(x_canvas.context, p_gradient.function, t_offsets, t_colors, t_ramp_length, p_gradient.mirror, p_gradient.wrap, p_gradient.repeats, t_gradient_transform, p_gradient.filter);
		MCGContextSetStrokeGradient(x_canvas.context, p_gradient.function, t_offsets, t_colors, t_ramp_length, p_gradient.mirror, p_gradient.wrap, p_gradient.repeats, t_gradient_transform, p_gradient.filter);
	}

	MCMemoryDeleteArray(t_offsets);
	MCMemoryDeleteArray(t_colors);
	
	return t_success;
}

void MCCanvasApplyPaint(__MCCanvasImpl &x_canvas, MCCanvasPaintRef &p_paint)
{
	__MCCanvasPaintImpl *t_paint;
	t_paint = MCCanvasPaintGet(p_paint);
	
	switch (t_paint->type)
	{
		case kMCCanvasPaintTypeSolid:
			MCCanvasApplySolidPaint(x_canvas, *(__MCCanvasSolidPaintImpl*)p_paint);
			break;
			
		case kMCCanvasPaintTypePattern:
			MCCanvasApplyPatternPaint(x_canvas, *(__MCCanvasPatternImpl*)p_paint);
			break;
			
		case kMCCanvasPaintTypeGradient:
			MCCanvasApplyGradientPaint(x_canvas, *(__MCCanvasGradientImpl*)p_paint);
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

void MCCanvasCanvasTransform(MCCanvasRef &x_canvas, const MCGAffineTransform &p_transform)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(x_canvas);
	
	MCGContextConcatCTM(t_canvas->context, p_transform);
	// Need to re-apply pattern paint when transform changes
	if (MCCanvasPaintIsPattern(t_canvas->props().paint))
		t_canvas->paint_changed = true;
}

void MCCanvasCanvasTransform(MCCanvasRef &x_canvas, MCCanvasTransformRef p_transform)
{
	MCCanvasCanvasTransform(x_canvas, *MCCanvasTransformGet(p_transform));
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
	t_canvas = MCCanvasGet(x_canvas);
	
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
	t_canvas = MCCanvasGet(x_canvas);
	
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
	t_canvas = MCCanvasGet(x_canvas);
	
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


void MCCanvasCanvasBeginLayerWithEffect(MCCanvasRef &x_canvas, MCCanvasEffectRef p_effect, MCCanvasRectangleRef p_rect)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(x_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	if (!MCCanvasPropertiesPush(*t_canvas))
	{
		// TODO - throw error
		return;
	}

	MCGBitmapEffects t_effects;
	t_effects.has_color_overlay = t_effects.has_drop_shadow = t_effects.has_inner_glow = t_effects.has_inner_shadow = t_effects.has_outer_glow = false;
	
	__MCCanvasEffectImpl *t_effect_impl;
	t_effect_impl = MCCanvasEffectGet(p_effect);
	
	switch (t_effect_impl->type)
	{
		case kMCCanvasEffectTypeColorOverlay:
		{
			t_effects.has_color_overlay = true;
			t_effects.color_overlay.blend_mode = t_effect_impl->blend_mode;
			t_effects.color_overlay.color = MCCanvasColorToMCGColor(t_effect_impl->color);
			break;
		}
			
		case kMCCanvasEffectTypeInnerGlow:
		{
			t_effects.has_inner_glow = true;
			t_effects.inner_glow.blend_mode = t_effect_impl->blend_mode;
			t_effects.inner_glow.color = MCCanvasColorToMCGColor(t_effect_impl->color);
//			t_effects.inner_glow.inverted = // TODO - inverted property?
			t_effects.inner_glow.size = t_effect_impl->size;
			t_effects.inner_glow.spread = t_effect_impl->spread;
			break;
		}
			
		case kMCCanvasEffectTypeInnerShadow:
		{
			t_effects.has_inner_shadow = true;
			t_effects.inner_shadow.blend_mode = t_effect_impl->blend_mode;
			t_effects.inner_shadow.color = MCCanvasColorToMCGColor(t_effect_impl->color);
//			t_effects.inner_shadow.knockout = // TODO - knockout property?
			t_effects.inner_shadow.size = t_effect_impl->size;
			t_effects.inner_shadow.spread = t_effect_impl->spread;
			MCPolarCoordsToCartesian(t_effect_impl->distance, t_effect_impl->angle, t_effects.inner_shadow.x_offset, t_effects.inner_shadow.y_offset);
			break;
		}
			
		case kMCCanvasEffectTypeOuterGlow:
		{
			t_effects.has_outer_glow = true;
			t_effects.outer_glow.blend_mode = t_effect_impl->blend_mode;
			t_effects.outer_glow.color = MCCanvasColorToMCGColor(t_effect_impl->color);
			t_effects.outer_glow.size = t_effect_impl->size;
			t_effects.outer_glow.spread = t_effect_impl->spread;
			break;
		}
			
		case kMCCanvasEffectTypeOuterShadow:
		{
			t_effects.has_drop_shadow = true;
			t_effects.drop_shadow.blend_mode = t_effect_impl->blend_mode;
			t_effects.drop_shadow.color = MCCanvasColorToMCGColor(t_effect_impl->color);
			t_effects.drop_shadow.size = t_effect_impl->size;
			t_effects.drop_shadow.spread = t_effect_impl->spread;
			MCPolarCoordsToCartesian(t_effect_impl->distance, t_effect_impl->angle, t_effects.drop_shadow.x_offset, t_effects.drop_shadow.y_offset);
			break;
		}
			
		default:
			MCAssert(false);
	}
	
	MCGContextBeginWithEffects(t_canvas->context, *MCCanvasRectangleGet(p_rect), t_effects);
}

void MCCanvasCanvasEndLayer(MCCanvasRef &x_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(x_canvas);
	
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
	t_canvas = MCCanvasGet(x_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	MCGContextFill(t_canvas->context);
}

void MCCanvasCanvasStroke(MCCanvasRef &x_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(x_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	MCGContextStroke(t_canvas->context);
}

void MCCanvasCanvasClipToRect(MCCanvasRef &x_canvas, MCCanvasRectangleRef &p_rect)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(x_canvas);
	
	MCGContextClipToRect(t_canvas->context, *MCCanvasRectangleGet(p_rect));
}

void MCCanvasCanvasAddPath(MCCanvasRef &x_canvas, MCCanvasPathRef p_path)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(x_canvas);
	
	MCGContextAddPath(t_canvas->context, *MCCanvasPathGet(p_path));
}

void MCCanvasCanvasFillPath(MCCanvasRef &x_canvas, MCCanvasPathRef p_path)
{
	MCCanvasCanvasAddPath(x_canvas, p_path);
	MCCanvasCanvasFill(x_canvas);
}

void MCCanvasCanvasStrokePath(MCCanvasRef &x_canvas, MCCanvasPathRef p_path)
{
	MCCanvasCanvasAddPath(x_canvas, p_path);
	MCCanvasCanvasStroke(x_canvas);
}

void MCCanvasCanvasDrawRectOfImage(MCCanvasRef &x_canvas, MCCanvasImageRef p_image, const MCGRectangle &p_src_rect, const MCGRectangle &p_dst_rect)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(x_canvas);
	
	MCImageRep *t_image;
	t_image = MCCanvasImageGetImageRep(p_image);
	
	MCCanvasApplyChanges(*t_canvas);

	MCGImageFrame t_frame;
	
	MCGFloat t_scale;
	t_scale = MCGAffineTransformGetEffectiveScale(MCGContextGetDeviceTransform(t_canvas->context));
	
	if (MCImageRepLock(t_image, 0, t_scale, t_frame))
	{
		MCGAffineTransform t_transform;
		t_transform = MCGAffineTransformMakeScale(1.0 / t_frame.x_scale, 1.0 / t_frame.y_scale);
		
		MCGRectangle t_src_rect;
		t_src_rect = MCGRectangleScale(p_src_rect, t_frame.x_scale, t_frame.y_scale);
		
		MCGContextDrawRectOfImage(t_canvas->context, t_frame.image, t_src_rect, p_dst_rect, t_canvas->props().image_filter);
		
		MCImageRepUnlock(t_image, 0, t_frame);
	}
}

void MCCanvasCanvasDrawRectOfImage(MCCanvasRef &x_canvas, MCCanvasImageRef p_image, MCCanvasRectangleRef p_src_rect, MCCanvasRectangleRef p_dst_rect)
{
	MCCanvasCanvasDrawRectOfImage(x_canvas, p_image, *MCCanvasRectangleGet(p_src_rect), *MCCanvasRectangleGet(p_dst_rect));
}

void MCCanvasCanvasDrawImage(MCCanvasRef &x_canvas, MCCanvasImageRef p_image, MCCanvasRectangleRef p_dst_rect)
{
	MCGRectangle t_src_rect;
	uint32_t t_width, t_height;
	MCCanvasImageGetWidth(p_image, t_width);
	MCCanvasImageGetHeight(p_image, t_height);
	t_src_rect = MCGRectangleMake(0, 0, t_width, t_height);
	MCCanvasCanvasDrawRectOfImage(x_canvas, p_image, t_src_rect, *MCCanvasRectangleGet(p_dst_rect));
}

void MCCanvasCanvasMoveTo(MCCanvasRef &x_canvas, MCCanvasPointRef p_point)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(x_canvas);
	
	MCGContextMoveTo(t_canvas->context, *MCCanvasPointGet(p_point));
}

void MCCanvasCanvasLineTo(MCCanvasRef &x_canvas, MCCanvasPointRef p_point)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(x_canvas);
	
	MCGContextLineTo(t_canvas->context, *MCCanvasPointGet(p_point));
}

void MCCanvasCanvasCurveThroughPoint(MCCanvasRef &x_canvas, MCCanvasPointRef p_through, MCCanvasPointRef p_to)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(x_canvas);
	
	MCGContextQuadraticTo(t_canvas->context, *MCCanvasPointGet(p_through), *MCCanvasPointGet(p_to));
}

void MCCanvasCanvasCurveThroughPoints(MCCanvasRef &x_canvas, MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(x_canvas);
	
	MCGContextCubicTo(t_canvas->context, *MCCanvasPointGet(p_through_a), *MCCanvasPointGet(p_through_b), *MCCanvasPointGet(p_to));
}

void MCCanvasCanvasClosePath(MCCanvasRef &x_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(x_canvas);
	
	MCGContextCloseSubpath(t_canvas->context);
}

////////////////////////////////////////////////////////////////////////////////

static MCValueCustomCallbacks kMCCanvasRectangleCustomValueCallbacks =
{
	false,
	__MCCanvasRectangleDestroy,
	__MCCanvasRectangleCopy,
	__MCCanvasRectangleEqual,
	__MCCanvasRectangleHash,
	__MCCanvasRectangleDescribe,
};

static MCValueCustomCallbacks kMCCanvasPointCustomValueCallbacks =
{
	false,
	__MCCanvasPointDestroy,
	__MCCanvasPointCopy,
	__MCCanvasPointEqual,
	__MCCanvasPointHash,
	__MCCanvasPointDescribe,
};

static MCValueCustomCallbacks kMCCanvasColorCustomValueCallbacks =
{
	false,
	__MCCanvasColorDestroy,
	__MCCanvasColorCopy,
	__MCCanvasColorEqual,
	__MCCanvasColorHash,
	__MCCanvasColorDescribe,
};

static MCValueCustomCallbacks kMCCanvasTransformCustomValueCallbacks =
{
	false,
	__MCCanvasTransformDestroy,
	__MCCanvasTransformCopy,
	__MCCanvasTransformEqual,
	__MCCanvasTransformHash,
	__MCCanvasTransformDescribe,
};

static MCValueCustomCallbacks kMCCanvasImageCustomValueCallbacks =
{
	false,
	__MCCanvasImageDestroy,
	__MCCanvasImageCopy,
	__MCCanvasImageEqual,
	__MCCanvasImageHash,
	__MCCanvasImageDescribe,
};

static MCValueCustomCallbacks kMCCanvasPaintCustomValueCallbacks =
{
	false,
	__MCCanvasPaintDestroy,
	__MCCanvasPaintCopy,
	__MCCanvasPaintEqual,
	__MCCanvasPaintHash,
	__MCCanvasPaintDescribe,
};

static MCValueCustomCallbacks kMCCanvasGradientStopCustomValueCallbacks =
{
	false,
	__MCCanvasGradientStopDestroy,
	__MCCanvasGradientStopCopy,
	__MCCanvasGradientStopEqual,
	__MCCanvasGradientStopHash,
	__MCCanvasGradientStopDescribe,
};

static MCValueCustomCallbacks kMCCanvasPathCustomValueCallbacks =
{
	false,
	__MCCanvasPathDestroy,
	__MCCanvasPathCopy,
	__MCCanvasPathEqual,
	__MCCanvasPathHash,
	__MCCanvasPathDescribe,
};

static MCValueCustomCallbacks kMCCanvasEffectCustomValueCallbacks =
{
	false,
	__MCCanvasEffectDestroy,
	__MCCanvasEffectCopy,
	__MCCanvasEffectEqual,
	__MCCanvasEffectHash,
	__MCCanvasEffectDescribe,
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
	/* UNCHECKED */ MCCustomTypeInfoCreate(&kMCCanvasRectangleCustomValueCallbacks, kMCCanvasRectangleTypeInfo);
	/* UNCHECKED */ MCCustomTypeInfoCreate(&kMCCanvasPointCustomValueCallbacks, kMCCanvasPointTypeInfo);
	/* UNCHECKED */ MCCustomTypeInfoCreate(&kMCCanvasColorCustomValueCallbacks, kMCCanvasColorTypeInfo);
	/* UNCHECKED */ MCCustomTypeInfoCreate(&kMCCanvasTransformCustomValueCallbacks, kMCCanvasTransformTypeInfo);
	/* UNCHECKED */ MCCustomTypeInfoCreate(&kMCCanvasImageCustomValueCallbacks, kMCCanvasImageTypeInfo);
	/* UNCHECKED */ MCCustomTypeInfoCreate(&kMCCanvasPaintCustomValueCallbacks, kMCCanvasPaintTypeInfo);
	/* UNCHECKED */ MCCustomTypeInfoCreate(&kMCCanvasGradientStopCustomValueCallbacks, kMCCanvasGradientStopTypeInfo);
	/* UNCHECKED */ MCCustomTypeInfoCreate(&kMCCanvasPathCustomValueCallbacks, kMCCanvasPathTypeInfo);
	/* UNCHECKED */ MCCustomTypeInfoCreate(&kMCCanvasEffectCustomValueCallbacks, kMCCanvasEffectTypeInfo);
	/* UNCHECKED */ MCCustomTypeInfoCreate(&kMCCanvasCustomValueCallbacks, kMCCanvasTypeInfo);
}

void MCCanvasTypesFinalize()
{
	MCValueRelease(kMCCanvasRectangleTypeInfo);
	MCValueRelease(kMCCanvasPointTypeInfo);
	MCValueRelease(kMCCanvasColorTypeInfo);
	MCValueRelease(kMCCanvasTransformTypeInfo);
	MCValueRelease(kMCCanvasImageTypeInfo);
	MCValueRelease(kMCCanvasPaintTypeInfo);
	MCValueRelease(kMCCanvasGradientStopTypeInfo);
	MCValueRelease(kMCCanvasPathTypeInfo);
	MCValueRelease(kMCCanvasEffectTypeInfo);
	MCValueRelease(kMCCanvasTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////

void MCCanvasConstantsInitialize()
{
	/* UNCHECKED */ MCCanvasTransformCreateWithMCGAffineTransform(MCGAffineTransformMakeIdentity(), kMCCanvasIdentityTransform);
	/* UNCHECKED */ MCCanvasColorCreateWithRGBA(0, 0, 0, 1, kMCCanvasColorBlack);
}

void MCCanvasConstantsFinalize()
{
	MCValueRelease(kMCCanvasIdentityTransform);
	MCValueRelease(kMCCanvasColorBlack);
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
