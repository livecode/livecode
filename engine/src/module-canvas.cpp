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

////////////////////////////////////////////////////////////////////////////////

#include "prefix.h"

#include "image.h"
#include "stack.h"
#include "widget.h"

#include "module-canvas.h"
#include "module-canvas-internal.h"
#include "module-engine.h"

//////////

extern MCWidgetRef MCcurrentwidget;

////////////////////////////////////////////////////////////////////////////////

// SVG Path Parsing

enum MCSVGPathCommand
{
	kMCSVGPathMoveTo,
	kMCSVGPathRelativeMoveTo,
	kMCSVGPathLineTo,
	kMCSVGPathRelativeLineTo,
	kMCSVGPathHorizontalLineTo,
	kMCSVGPathRelativeHorizontalLineTo,
	kMCSVGPathVerticalLineTo,
	kMCSVGPathRelativeVerticalLineTo,
	kMCSVGPathCurveTo,
	kMCSVGPathRelativeCurveTo,
	kMCSVGPathShorthandCurveTo,
	kMCSVGPathRelativeShorthandCurveTo,
	kMCSVGPathQuadraticCurveTo,
	kMCSVGPathRelativeQuadraticCurveTo,
	kMCSVGPathShorthandQuadraticCurveTo,
	kMCSVGPathRelativeShorthandQuadraticCurveTo,
	kMCSVGPathEllipticalCurveTo,
	kMCSVGPathRelativeEllipticalCurveTo,
    kMCSVGPathClose,
};

typedef bool (*MCSVGParseCallback)(void *p_context, MCSVGPathCommand p_command, float32_t *p_args, uint32_t p_arg_count);

bool MCSVGParse(MCStringRef p_string, MCSVGParseCallback p_callback, void *p_context);

inline bool MCSVGPathCommandIsCubic(MCSVGPathCommand p_command)
{
	return p_command == kMCSVGPathCurveTo || p_command == kMCSVGPathRelativeCurveTo || p_command == kMCSVGPathShorthandCurveTo || p_command == kMCSVGPathRelativeShorthandCurveTo;
}

inline bool MCSVGPathCommandIsQuadratic(MCSVGPathCommand p_command)
{
	return p_command == kMCSVGPathQuadraticCurveTo || p_command == kMCSVGPathRelativeQuadraticCurveTo || p_command == kMCSVGPathShorthandQuadraticCurveTo || p_command == kMCSVGPathRelativeShorthandQuadraticCurveTo;
}

inline bool MCSVGPathCommandIsRelative(MCSVGPathCommand p_command)
{
	// Odd numbered commands are relative
	return (p_command & 1) != 0;
}

//////////

bool MCGPathGetSVGData(MCGPathRef p_path, MCStringRef &r_string);

////////////////////////////////////////////////////////////////////////////////

// Useful stuff

bool MCMemoryAllocateArray(uint32_t p_size, uint32_t p_count, void *&r_array)
{
	return MCMemoryAllocate(p_size * p_count, r_array);
}

template <class T>
bool MCMemoryAllocateArray(uint32_t p_count, T *&r_array)
{
	void *t_array;
	if (!MCMemoryAllocateArray(sizeof(T), p_count, t_array))
		return false;
	
	r_array = (T*)t_array;
	return true;
}

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

//////////

bool MCProperListGetNumberAtIndex(MCProperListRef p_list, uindex_t p_index, MCNumberRef &r_number)
{
	if (p_index >= MCProperListGetLength(p_list))
		return false;
		
	MCValueRef t_value;
	t_value = MCProperListFetchElementAtIndex(p_list, p_index);
	if (t_value == nil)
		return false;
	
    // Is the value already a (boxed) number?
    MCTypeInfoRef t_typeinfo = MCValueGetTypeInfo(t_value);
	if (t_typeinfo == kMCNumberTypeInfo)
    {
        r_number = MCValueRetain((MCNumberRef)t_value);
        return true;
    }
	
    // Is the value something that can be bridged to a number?
    if (MCTypeInfoConforms(t_typeinfo, kMCNumberTypeInfo))
    {
        // Get the underlying type
        if (MCTypeInfoIsNamed(t_typeinfo))
            t_typeinfo = MCNamedTypeInfoGetBoundTypeInfo(t_typeinfo);
        
        if (MCTypeInfoIsForeign(t_typeinfo))
        {
            const MCForeignTypeDescriptor* t_desc = MCForeignTypeInfoGetDescriptor(t_typeinfo);
            if (t_desc->doimport(t_desc, MCForeignValueGetContentsPtr(t_value), false, (MCValueRef&)r_number))
                return true;
        }
    }
    
    // If we get here, we didn't know how to handle the type
    return false;
}

bool MCProperListFetchRealAtIndex(MCProperListRef p_list, uindex_t p_index, real64_t &r_real)
{
	MCAutoNumberRef t_number;
	if (!MCProperListGetNumberAtIndex(p_list, p_index, &t_number))
		return false;
	
	r_real = MCNumberFetchAsReal(*t_number);
	
	return true;
}

inline bool MCProperListFetchFloatAtIndex(MCProperListRef p_list, uindex_t p_index, float32_t &r_float)
{
	real64_t t_real;
	if (!MCProperListFetchRealAtIndex(p_list, p_index, t_real))
		return false;
	
	r_float = float32_t(t_real);
	return true;
}

bool MCProperListFetchIntegerAtIndex(MCProperListRef p_list, uindex_t p_index, integer_t &r_integer)
{
	MCAutoNumberRef t_number;
	if (!MCProperListGetNumberAtIndex(p_list, p_index, &t_number))
		return false;

	r_integer = MCNumberFetchAsInteger(*t_number);
	
	return true;
}

bool MCProperListFetchAsArrayOfReal(MCProperListRef p_list, uindex_t p_size, real64_t *r_reals)
{
	if (p_size != MCProperListGetLength(p_list))
		return false;
	
	for (uindex_t i = 0; i < p_size; i++)
		if (!MCProperListFetchRealAtIndex(p_list, i, r_reals[i]))
			return false;
	
	return true;
}

bool MCProperListCreateWithArrayOfReal(const real64_t *p_reals, uindex_t p_size, MCProperListRef &r_list)
{
	bool t_success;
	t_success = true;
	
	MCAutoNumberRefArray t_numbers;
	
	if (t_success)
		t_success = t_numbers.New(p_size);
	
	for (uindex_t i = 0; t_success && i < p_size; i++)
		t_success = MCNumberCreateWithReal(p_reals[i], t_numbers[i]);
	
	if (t_success)
		t_success = MCProperListCreate((MCValueRef*)*t_numbers, p_size, r_list);
	
	return t_success;
}

bool MCProperListFetchAsArrayOfFloat(MCProperListRef p_list, uindex_t p_size, float32_t *x_floats)
{
	if (p_size != MCProperListGetLength(p_list))
		return false;
	
	for (uindex_t i = 0; i < p_size; i++)
		if (!MCProperListFetchFloatAtIndex(p_list, i, x_floats[i]))
			return false;
	
	return true;
}

bool MCProperListFetchAsArrayOfInteger(MCProperListRef p_list, uindex_t p_size, integer_t *r_integers)
{
	if (p_size != MCProperListGetLength(p_list))
		return false;
	
	for (uindex_t i = 0; i < p_size; i++)
		if (!MCProperListFetchIntegerAtIndex(p_list, i, r_integers[i]))
			return false;
	
	return true;
}

bool MCProperListToArrayOfFloat(MCProperListRef p_list, uindex_t &r_count, float32_t *&r_values)
{
	bool t_success;
	t_success = true;
	
	uindex_t t_count;
	t_count = MCProperListGetLength(p_list);
	
	float32_t *t_values;
	t_values = nil;
	
	if (t_success)
		t_success = MCMemoryAllocateArray(t_count, t_values);
	
	if (t_success)
		t_success = MCProperListFetchAsArrayOfFloat(p_list, t_count, t_values);
	
	if (t_success)
	{
		r_count = t_count;
		r_values = t_values;
	}
	else
		MCMemoryDeleteArray(t_values);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

inline MCGPoint MCGPointRelativeToAbsolute(const MCGPoint &origin, const MCGPoint &point)
{
	return MCGPointMake(origin.x + point.x, origin.y + point.y);
}

inline MCGPoint MCGPointReflect(const MCGPoint &origin, const MCGPoint &point)
{
	return MCGPointMake(origin.x - (point.x - origin.x), origin.y - (point.y - origin.y));
}

inline MCGFloat MCGAffineTransformGetEffectiveScale(const MCGAffineTransform &p_transform)
{
	return MCMax(MCAbs(p_transform.a) + MCAbs(p_transform.c), MCAbs(p_transform.d) + MCAbs(p_transform.b));
}

////////////////////////////////////////////////////////////////////////////////

bool MCGPointParse(MCStringRef p_string, MCGPoint &r_point)
{
	bool t_success;
	t_success = false;
	
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

bool MCCanvasTypesInitialize();
void MCCanvasTypesFinalize();

MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasRectangleTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasPointTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasColorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasTransformTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasImageTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasPaintTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasSolidPaintTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasPatternTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasGradientTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasGradientStopTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasPathTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasEffectTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasFontTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasTypeInfo;

extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasRectangleTypeInfo(void) { return kMCCanvasRectangleTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasPointTypeInfo(void) { return kMCCanvasPointTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasColorTypeInfo(void) { return kMCCanvasColorTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasTransformTypeInfo(void) { return kMCCanvasTransformTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasImageTypeInfo(void) { return kMCCanvasImageTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasPaintTypeInfo(void) { return kMCCanvasPaintTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasSolidPaintTypeInfo(void) { return kMCCanvasSolidPaintTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasPatternTypeInfo(void) { return kMCCanvasPatternTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasGradientTypeInfo(void) { return kMCCanvasGradientTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasGradientStopTypeInfo(void) { return kMCCanvasGradientStopTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasPathTypeInfo(void) { return kMCCanvasPathTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasEffectTypeInfo(void) { return kMCCanvasEffectTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasFontTypeInfo(void) { return kMCCanvasFontTypeInfo; }
extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCCanvasTypeInfo(void) { return kMCCanvasTypeInfo; }


////////////////////////////////////////////////////////////////////////////////

// Error types

bool MCCanvasErrorsInitialize();
void MCCanvasErrorsFinalize();

MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasRectangleListFormatErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasPointListFormatErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasColorListFormatErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasScaleListFormatErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasTranslationListFormatErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasSkewListFormatErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasRadiiListFormatErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasImageSizeListFormatErrorTypeInfo;

MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasTransformMatrixListFormatErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasTransformDecomposeErrorTypeInfo;

MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasImageRepReferencedErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasImageRepDataErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasImageRepPixelsErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasImageRepGetGeometryErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasImageRepGetMetadataErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasImageRepGetDensityErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasImageRepLockErrorTypeInfo;

MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasGradientInvalidRampErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasGradientStopRangeErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasGradientStopOrderErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasGradientTypeErrorTypeInfo;

MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasEffectInvalidPropertyErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasEffectPropertyNotAvailableErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasEffectPropertyInvalidValueErrorTypeInfo;

MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasPathPointListFormatErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCCanvasSVGPathParseErrorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

// Constant refs

MCCanvasTransformRef kMCCanvasIdentityTransform = nil;
MCCanvasColorRef kMCCanvasColorBlack = nil;
MCCanvasFontRef kMCCanvasFont12PtHelvetica = nil;
MCCanvasPathRef kMCCanvasEmptyPath = nil;

bool MCCanvasConstantsInitialize();
void MCCanvasConstantsFinalize();

////////////////////////////////////////////////////////////////////////////////

bool MCCanvasBlendModeFromString(MCStringRef p_string, MCGBlendMode &r_blend_mode);
bool MCCanvasBlendModeToString(MCGBlendMode p_blend_mode, MCStringRef &r_string);
bool MCCanvasEffectTypeToString(MCCanvasEffectType p_type, MCStringRef &r_string);
bool MCCanvasEffectPropertyToString(MCCanvasEffectProperty p_property, MCStringRef &r_string);
bool MCCanvasEffectPropertyFromString(MCStringRef p_string, MCCanvasEffectProperty &r_property);
bool MCCanvasEffectSourceToString(MCCanvasEffectSource p_source, MCStringRef &r_string);
bool MCCanvasEffectSourceFromString(MCStringRef p_string, MCCanvasEffectSource &r_source);
bool MCCanvasGradientTypeFromString(MCStringRef p_string, MCGGradientFunction &r_type);
bool MCCanvasGradientTypeToString(MCGGradientFunction p_type, MCStringRef &r_string);
bool MCCanvasFillRuleFromString(MCStringRef p_string, MCGFillRule &r_fill_rule);
bool MCCanvasFillRuleToString(MCGFillRule p_fill_rule, MCStringRef &r_string);
bool MCCanvasImageFilterFromString(MCStringRef p_string, MCGImageFilter &r_filter);
bool MCCanvasImageFilterToString(MCGImageFilter p_filter, MCStringRef &r_string);
bool MCCanvasJoinStyleToString(MCGJoinStyle p_style, MCStringRef &r_string);
bool MCCanvasJoinStyleFromString(MCStringRef p_string, MCGJoinStyle &r_style);
bool MCCanvasCapStyleToString(MCGCapStyle p_style, MCStringRef &r_string);
bool MCCanvasCapStyleFromString(MCStringRef p_string, MCGCapStyle &r_style);

MCCanvasFloat MCCanvasColorGetRed(MCCanvasColorRef color);
MCCanvasFloat MCCanvasColorGetGreen(MCCanvasColorRef color);
MCCanvasFloat MCCanvasColorGetBlue(MCCanvasColorRef color);
MCCanvasFloat MCCanvasColorGetAlpha(MCCanvasColorRef color);

////////////////////////////////////////////////////////////////////////////////

static MCNameRef s_blend_mode_map[kMCGBlendModeCount];
static MCNameRef s_transform_matrix_keys[9];
static MCNameRef s_effect_type_map[_MCCanvasEffectTypeCount];
static MCNameRef s_effect_property_map[_MCCanvasEffectPropertyCount];
static MCNameRef s_effect_source_map[_MCCanvasEffectSourceCount];
static MCNameRef s_gradient_type_map[kMCGGradientFunctionCount];
static MCNameRef s_canvas_fillrule_map[kMCGFillRuleCount];
static MCNameRef s_image_filter_map[kMCGImageFilterCount];
static MCNameRef s_join_style_map[kMCGJoinStyleCount];
static MCNameRef s_cap_style_map[kMCGCapStyleCount];

////////////////////////////////////////////////////////////////////////////////

bool MCCanvasStringsInitialize();
void MCCanvasStringsFinalize();

extern "C" bool com_livecode_canvas_Initialize()
{
	if (!MCCanvasErrorsInitialize())
		return false;
	if (!MCCanvasStringsInitialize())
		return false;
	if (!MCCanvasTypesInitialize())
		return false;
	if (!MCCanvasConstantsInitialize())
		return false;
    return true;
}

extern "C" void com_livecode_canvas_Finalize()
{
	MCCanvasConstantsFinalize();
	MCCanvasTypesFinalize();
	MCCanvasStringsFinalize();
	MCCanvasErrorsFinalize();
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
	MCGRectangle t_rectangle;
	MCCanvasRectangleGetMCGRectangle (static_cast<MCCanvasRectangleRef>(p_value), t_rectangle);

	return MCStringFormat (r_desc, "<rectangle (%g, %g) - (%g, %g)>",
	                       double(t_rectangle.origin.x),
	                       double(t_rectangle.origin.y),
	                       double(t_rectangle.origin.x + t_rectangle.size.width),
	                       double(t_rectangle.origin.y + t_rectangle.size.height));
}

bool MCCanvasRectangleCreateWithMCGRectangle(const MCGRectangle &p_rect, MCCanvasRectangleRef &r_rectangle)
{
	bool t_success;
	t_success = true;
	
	MCCanvasRectangleRef t_rectangle;
	t_rectangle = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasRectangleTypeInfo, sizeof(__MCCanvasRectangleImpl), t_rectangle);
	
	if (t_success)
	{
		*(MCCanvasRectangleGet(t_rectangle)) = p_rect;
		t_success = MCValueInter(t_rectangle, r_rectangle);
	}
	
	MCValueRelease(t_rectangle);
	
	return t_success;
}

__MCCanvasRectangleImpl *MCCanvasRectangleGet(MCCanvasRectangleRef p_rect)
{
	return (__MCCanvasRectangleImpl*)MCValueGetExtraBytesPtr(p_rect);
}

void MCCanvasRectangleGetMCGRectangle(MCCanvasRectangleRef p_rect, MCGRectangle &r_rect)
{
	r_rect = *MCCanvasRectangleGet(p_rect);
}

//////////

bool MCProperListToRectangle(MCProperListRef p_list, MCGRectangle &r_rectangle)
{
	bool t_success;
	t_success = true;
	
	real64_t t_rect[4];
	
	if (t_success)
		t_success = MCProperListFetchAsArrayOfReal(p_list, 4, t_rect);
		
	if (t_success)
		r_rectangle = MCGRectangleMake(t_rect[0], t_rect[1], t_rect[2] - t_rect[0], t_rect[3] - t_rect[1]);
	else
		MCCanvasThrowError(kMCCanvasRectangleListFormatErrorTypeInfo);
	
	return t_success;
}

// Constructors

MC_DLLEXPORT_DEF
void MCCanvasRectangleMakeWithLTRB(MCCanvasFloat p_left, MCCanvasFloat p_top, MCCanvasFloat p_right, MCCanvasFloat p_bottom, MCCanvasRectangleRef &r_rect)
{
	/* UNCHECKED */ MCCanvasRectangleCreateWithMCGRectangle(MCGRectangleMake(p_left, p_top, p_right - p_left, p_bottom - p_top), r_rect);
}

MC_DLLEXPORT_DEF
void MCCanvasRectangleMakeWithList(MCProperListRef p_list, MCCanvasRectangleRef &r_rect)
{
	MCGRectangle t_rect;
	if (!MCProperListToRectangle(p_list, t_rect))
		return;
	
	/* UNCHECKED */ MCCanvasRectangleCreateWithMCGRectangle(t_rect, r_rect);
}

// Properties

void MCCanvasRectangleSetMCGRectangle(const MCGRectangle &p_rect, MCCanvasRectangleRef &x_rect)
{
	MCCanvasRectangleRef t_rect;
	if (!MCCanvasRectangleCreateWithMCGRectangle(p_rect, t_rect))
		return;
	
	MCValueAssign(x_rect, t_rect);
	MCValueRelease(t_rect);
}

MC_DLLEXPORT_DEF
void MCCanvasRectangleGetLeft(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_left)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_left = t_rect.origin.x;
}

MC_DLLEXPORT_DEF
void MCCanvasRectangleSetLeft(MCCanvasFloat p_left, MCCanvasRectangleRef &x_rect)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.origin.x = p_left;

	MCCanvasRectangleSetMCGRectangle(t_rect, x_rect);
}

MC_DLLEXPORT_DEF
void MCCanvasRectangleGetTop(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_top)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_top = t_rect.origin.y;
}

MC_DLLEXPORT_DEF
void MCCanvasRectangleSetTop(MCCanvasFloat p_top, MCCanvasRectangleRef &x_rect)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.origin.y = p_top;
	
	MCCanvasRectangleSetMCGRectangle(t_rect, x_rect);
}

MC_DLLEXPORT_DEF
void MCCanvasRectangleGetRight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_right)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_right = t_rect.origin.x + t_rect.size.width;
}

MC_DLLEXPORT_DEF
void MCCanvasRectangleSetRight(MCCanvasFloat p_right, MCCanvasRectangleRef &x_rect)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.origin.x = p_right - t_rect.size.width;
	
	MCCanvasRectangleSetMCGRectangle(t_rect, x_rect);
}

MC_DLLEXPORT_DEF
void MCCanvasRectangleGetBottom(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_bottom)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_bottom = t_rect.origin.y + t_rect.size.height;
}

MC_DLLEXPORT_DEF
void MCCanvasRectangleSetBottom(MCCanvasFloat p_bottom, MCCanvasRectangleRef &x_rect)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.origin.y = p_bottom - t_rect.size.height;
	
	MCCanvasRectangleSetMCGRectangle(t_rect, x_rect);
}

MC_DLLEXPORT_DEF
void MCCanvasRectangleGetWidth(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_width)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_width = t_rect.size.width;
}

MC_DLLEXPORT_DEF
void MCCanvasRectangleSetWidth(MCCanvasFloat p_width, MCCanvasRectangleRef &x_rect)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.size.width = p_width;
	
	MCCanvasRectangleSetMCGRectangle(t_rect, x_rect);
}

MC_DLLEXPORT_DEF
void MCCanvasRectangleGetHeight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_height)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_height = t_rect.size.height;
}

MC_DLLEXPORT_DEF
void MCCanvasRectangleSetHeight(MCCanvasFloat p_height, MCCanvasRectangleRef &x_rect)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.size.height = p_height;
	
	MCCanvasRectangleSetMCGRectangle(t_rect, x_rect);
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
	
	return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_right), sizeof(__MCCanvasPointImpl)) == 0;
}

static hash_t __MCCanvasPointHash(MCValueRef p_value)
{
	return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCCanvasPointImpl));
}

static bool __MCCanvasPointDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint (static_cast<MCCanvasPointRef>(p_value), t_point);

	return MCStringFormat (r_desc, "(%g, %g)",
	                       double(t_point.x), double(t_point.y));
}

bool MCCanvasPointCreateWithMCGPoint(const MCGPoint &p_point, MCCanvasPointRef &r_point)
{
	bool t_success;
	t_success = true;
	
	MCCanvasPointRef t_point;
	t_point = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasPointTypeInfo, sizeof(__MCCanvasPointImpl), t_point);
	
	if (t_success)
	{
		*MCCanvasPointGet(t_point) = p_point;
		t_success = MCValueInter(t_point, r_point);
	}
	
	MCValueRelease(t_point);
	
	return t_success;
}

__MCCanvasPointImpl *MCCanvasPointGet(MCCanvasPointRef p_point)
{
	return (__MCCanvasPointImpl*)MCValueGetExtraBytesPtr(p_point);
}

void MCCanvasPointGetMCGPoint(MCCanvasPointRef p_point, MCGPoint &r_point)
{
	r_point = *MCCanvasPointGet(p_point);
}

//////////

bool MCProperListToPoint(MCProperListRef p_list, MCGPoint &r_point)
{
	real64_t t_point[2];
	if (!MCProperListFetchAsArrayOfReal(p_list, 2, t_point))
	{
		MCCanvasThrowError(kMCCanvasPointListFormatErrorTypeInfo);
		return false;
	}

	r_point = MCGPointMake(t_point[0], t_point[1]);
	
	return true;
}

bool MCProperListFromPoint(const MCGPoint &p_point, MCProperListRef &r_list)
{
	real64_t t_point[2];
	t_point[0] = p_point.x;
	t_point[1] = p_point.y;
	
	return MCProperListCreateWithArrayOfReal(t_point, 2, r_list);
}

// Constructors

MC_DLLEXPORT_DEF
void MCCanvasPointMake(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPointRef &r_point)
{
	/* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(MCGPointMake(p_x, p_y), r_point);
}

MC_DLLEXPORT_DEF
void MCCanvasPointMakeWithList(MCProperListRef p_list, MCCanvasPointRef &r_point)
{
	MCGPoint t_point;
	if (!MCProperListToPoint(p_list, t_point))
		return;
	
	/* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(t_point, r_point);
}

// Properties

void MCCanvasPointSetMCGPoint(const MCGPoint &p_point, MCCanvasPointRef &x_point)
{
	MCCanvasPointRef t_point;
	if (!MCCanvasPointCreateWithMCGPoint(p_point, t_point))
		return;
	MCValueAssign(x_point, t_point);
	MCValueRelease(t_point);
}

MC_DLLEXPORT_DEF
void MCCanvasPointGetX(MCCanvasPointRef p_point, MCCanvasFloat &r_x)
{
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(p_point, t_point);
	r_x = t_point.x;
}

MC_DLLEXPORT_DEF
void MCCanvasPointSetX(MCCanvasFloat p_x, MCCanvasPointRef &x_point)
{
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(x_point, t_point);
	t_point.x = p_x;
	
	MCCanvasPointSetMCGPoint(t_point, x_point);
}

MC_DLLEXPORT_DEF
void MCCanvasPointGetY(MCCanvasPointRef p_point, MCCanvasFloat &r_y)
{
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(p_point, t_point);
	r_y = t_point.y;
}

MC_DLLEXPORT_DEF
void MCCanvasPointSetY(MCCanvasFloat p_y, MCCanvasPointRef &x_point)
{
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(x_point, t_point);
	t_point.y = p_y;
	
	MCCanvasPointSetMCGPoint(t_point, x_point);
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
	MCCanvasColorRef t_color = static_cast<MCCanvasColorRef>(p_value);

	if (1 <= MCCanvasColorGetAlpha (t_color)) /* Opaque case */
		return MCStringFormat (r_desc, "<color: %g, %g, %g>",
		                       double(MCCanvasColorGetRed (t_color)),
		                       double(MCCanvasColorGetGreen (t_color)),
		                       double(MCCanvasColorGetBlue (t_color)));
	else
		return MCStringFormat (r_desc, "<color: %g, %g, %g, %g>",
		                       double(MCCanvasColorGetRed (t_color)),
		                       double(MCCanvasColorGetGreen (t_color)),
		                       double(MCCanvasColorGetBlue (t_color)),
		                       double(MCCanvasColorGetAlpha (t_color)));
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
		t_success = MCValueInterAndRelease(t_color, r_color);
        if (!t_success)
            MCValueRelease(t_color);
	}
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

bool MCProperListToRGBA(MCProperListRef p_list, MCCanvasFloat &r_red, MCCanvasFloat &r_green, MCCanvasFloat &r_blue, MCCanvasFloat &r_alpha)
{
	bool t_success;
	t_success = true;
	
	uindex_t t_length;
	t_length = MCProperListGetLength(p_list);
	
	real64_t t_rgba[4];
	
	if (t_success)
		t_success = t_length == 3 || t_length == 4;
	
	if (t_success)
		t_success = MCProperListFetchAsArrayOfReal(p_list, t_length, t_rgba);
	
	if (t_success)
	{
		if (t_length == 3)
			t_rgba[3] = 1.0; // set default alpha value of 1.0
	}
	
	if (t_success)
	{
		r_red = t_rgba[0];
		r_green = t_rgba[1];
		r_blue = t_rgba[2];
		r_alpha = t_rgba[3];
	}
	else
		MCCanvasThrowError(kMCCanvasColorListFormatErrorTypeInfo);
	
	return t_success;
}

// Constructors

MC_DLLEXPORT_DEF
void MCCanvasColorMakeRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color)
{
	/* UNCHECKED */ MCCanvasColorCreate(MCCanvasColorImplMake(p_red, p_blue, p_green, p_alpha), r_color);
}

MC_DLLEXPORT_DEF
void MCCanvasColorMakeWithList(MCProperListRef p_color, MCCanvasColorRef &r_color)
{
	MCCanvasFloat t_red, t_green, t_blue, t_alpha;
	if (!MCProperListToRGBA(p_color, t_red, t_green, t_blue, t_alpha))
		return;
	
	/* UNCHECKED */ MCCanvasColorCreateWithRGBA(t_red, t_green, t_blue, t_alpha, r_color);
}

//////////

// Properties

void MCCanvasColorSet(const __MCCanvasColorImpl &p_color, MCCanvasColorRef &x_color)
{
	MCCanvasColorRef t_color;
	if (!MCCanvasColorCreate(p_color, t_color))
		return;
	MCValueAssign(x_color, t_color);
	MCValueRelease(t_color);
}

MC_DLLEXPORT_DEF
void MCCanvasColorGetRed(MCCanvasColorRef p_color, MCCanvasFloat &r_red)
{
	r_red = MCCanvasColorGetRed(p_color);
}

MC_DLLEXPORT_DEF
void MCCanvasColorSetRed(MCCanvasFloat p_red, MCCanvasColorRef &x_color)
{
	__MCCanvasColorImpl t_color;
	t_color = *MCCanvasColorGet(x_color);
	
	if (t_color.red == p_red)
		return;
	
	t_color.red = p_red;
	MCCanvasColorSet(t_color, x_color);
}

MC_DLLEXPORT_DEF
void MCCanvasColorGetGreen(MCCanvasColorRef p_color, MCCanvasFloat &r_green)
{
	r_green = MCCanvasColorGetGreen(p_color);
}

MC_DLLEXPORT_DEF
void MCCanvasColorSetGreen(MCCanvasFloat p_green, MCCanvasColorRef &x_color)
{
	__MCCanvasColorImpl t_color;
	t_color = *MCCanvasColorGet(x_color);
	
	if (t_color.green == p_green)
		return;
	
	t_color.green = p_green;
	MCCanvasColorSet(t_color, x_color);
}

MC_DLLEXPORT_DEF
void MCCanvasColorGetBlue(MCCanvasColorRef p_color, MCCanvasFloat &r_blue)
{
	r_blue = MCCanvasColorGetBlue(p_color);
}

MC_DLLEXPORT_DEF
void MCCanvasColorSetBlue(MCCanvasFloat p_blue, MCCanvasColorRef &x_color)
{
	__MCCanvasColorImpl t_color;
	t_color = *MCCanvasColorGet(x_color);
	
	if (t_color.blue == p_blue)
		return;
	
	t_color.blue = p_blue;
	MCCanvasColorSet(t_color, x_color);
}

MC_DLLEXPORT_DEF
void MCCanvasColorGetAlpha(MCCanvasColorRef p_color, MCCanvasFloat &r_alpha)
{
	r_alpha = MCCanvasColorGetAlpha(p_color);
}

MC_DLLEXPORT_DEF
void MCCanvasColorSetAlpha(MCCanvasFloat p_alpha, MCCanvasColorRef &x_color)
{
	__MCCanvasColorImpl t_color;
	t_color = *MCCanvasColorGet(x_color);
	
	if (t_color.alpha == p_alpha)
		return;
	
	t_color.alpha = p_alpha;
	MCCanvasColorSet(t_color, x_color);
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

//////////

// special case for scale parameters, which may have one or two values
bool MCProperListToScale(MCProperListRef p_list, MCGPoint &r_scale)
{
	bool t_success;
	t_success = true;
	
	uindex_t t_length;
	t_length = MCProperListGetLength(p_list);
	
	real64_t t_scale[2];
	
	if (t_success)
		t_success = t_length == 1 || t_length == 2;
	
	if (t_success)
		t_success = MCProperListFetchAsArrayOfReal(p_list, t_length, t_scale);
	
	if (t_success)
	{
		if (t_length == 1)
			t_scale[1] = t_scale[0];
		r_scale = MCGPointMake(t_scale[0], t_scale[1]);
	}
	else
		MCCanvasThrowError(kMCCanvasScaleListFormatErrorTypeInfo);
	
	return t_success;
}

bool MCProperListToSkew(MCProperListRef p_list, MCGPoint &r_skew)
{
	bool t_success;
	t_success = true;
	
	real64_t t_skew[2];
	
	t_success = MCProperListFetchAsArrayOfReal(p_list, 2, t_skew);
	
	if (t_success)
		r_skew = MCGPointMake(t_skew[0], t_skew[1]);
	else
		MCCanvasThrowError(kMCCanvasSkewListFormatErrorTypeInfo);
	
	return t_success;
}

bool MCProperListToTranslation(MCProperListRef p_list, MCGPoint &r_translation)
{
	bool t_success;
	t_success = true;
	
	real64_t t_translation[2];
	
	t_success = MCProperListFetchAsArrayOfReal(p_list, 2, t_translation);
	
	if (t_success)
		r_translation = MCGPointMake(t_translation[0], t_translation[1]);
	else
		MCCanvasThrowError(kMCCanvasTranslationListFormatErrorTypeInfo);
	
	return t_success;
}

bool MCProperListToTransform(MCProperListRef p_list, MCGAffineTransform &r_transform)
{
	bool t_success;
	t_success = true;
	
	real64_t t_matrix[6];
	
	t_success = MCProperListFetchAsArrayOfReal(p_list, 6, t_matrix);
	
	if (t_success)
		r_transform = MCGAffineTransformMake(t_matrix[0], t_matrix[1], t_matrix[2], t_matrix[3], t_matrix[4], t_matrix[5]);
	else
		MCCanvasThrowError(kMCCanvasTransformMatrixListFormatErrorTypeInfo);
	
	return t_success;
}

bool MCProperListFromTransform(const MCGAffineTransform &p_transform, MCProperListRef &r_list)
{
	real64_t t_matrix[6];
	t_matrix[0] = p_transform.a;
	t_matrix[1] = p_transform.b;
	t_matrix[2] = p_transform.c;
	t_matrix[3] = p_transform.d;
	t_matrix[4] = p_transform.tx;
	t_matrix[5] = p_transform.ty;
	
	return MCProperListCreateWithArrayOfReal(t_matrix, 6, r_list);
}

// Constructors

void MCCanvasTransformMake(const MCGAffineTransform &p_transform, MCCanvasTransformRef &r_transform)
{
	/* UNCHECKED */ MCCanvasTransformCreateWithMCGAffineTransform(p_transform, r_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformMakeIdentity(MCCanvasTransformRef &r_transform)
{
	r_transform = MCValueRetain(kMCCanvasIdentityTransform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformMakeScale(MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMakeScale(p_xscale, p_yscale), r_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformMakeScaleWithList(MCProperListRef p_scale, MCCanvasTransformRef &r_transform)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasTransformMakeScale(t_scale.x, t_scale.y, r_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformMakeRotation(MCCanvasFloat p_angle, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_angle)), r_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformMakeTranslation(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMakeTranslation(p_x, p_y), r_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformMakeTranslationWithList(MCProperListRef p_translation, MCCanvasTransformRef &r_transform)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasTransformMakeTranslation(t_translation.x, t_translation.y, r_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformMakeSkew(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMakeSkew(p_x, p_y), r_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformMakeSkewWithList(MCProperListRef p_skew, MCCanvasTransformRef &r_transform)
{
	MCGPoint t_skew;
	if (!MCProperListToSkew(p_skew, t_skew))
		return;
	
	MCCanvasTransformMakeSkew(t_skew.x, t_skew.y, r_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformMakeWithMatrixValues(MCCanvasFloat p_a, MCCanvasFloat p_b, MCCanvasFloat p_c, MCCanvasFloat p_d, MCCanvasFloat p_tx, MCCanvasFloat p_ty, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMake(p_a, p_b, p_c, p_d, p_tx, p_ty), r_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformMakeWithMatrixAsList(MCProperListRef p_matrix, MCCanvasTransformRef &r_transform)
{
	MCGAffineTransform t_transform;
	if (!MCProperListToTransform(p_matrix, t_transform))
		return;
	
	MCCanvasTransformMake(t_transform, r_transform);
}

//////////

// Properties

void MCCanvasTransformSetMCGAffineTransform(const MCGAffineTransform &p_transform, MCCanvasTransformRef &x_transform)
{
	MCCanvasTransformRef t_transform;
	if (!MCCanvasTransformCreateWithMCGAffineTransform(p_transform, t_transform))
		return;
	MCValueAssign(x_transform, t_transform);
	MCValueRelease(t_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformGetMatrixAsList(MCCanvasTransformRef p_transform, MCProperListRef &r_matrix)
{
	/* UNCHECKED */ MCProperListFromTransform(*MCCanvasTransformGet(p_transform), r_matrix);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformSetMatrixAsList(MCProperListRef p_matrix, MCCanvasTransformRef &x_transform)
{
	bool t_success;
	t_success = true;
	
	MCGAffineTransform t_transform;
	if (!MCProperListToTransform(p_matrix, t_transform))
		return;
	
	MCCanvasTransformSetMCGAffineTransform(t_transform, x_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformGetInverse(MCCanvasTransformRef p_transform, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformInvert(*MCCanvasTransformGet(p_transform)), r_transform);
}

// T = Ttranslate * Trotate * Tskew * Tscale

MCGAffineTransform MCCanvasTransformCompose(const MCGPoint &p_scale, MCCanvasFloat p_rotation, const MCGPoint &p_skew, const MCGPoint &p_translation)
{
	MCGAffineTransform t_transform;
	t_transform = MCGAffineTransformMakeScale(p_scale.x, p_scale.y);
	t_transform = MCGAffineTransformPreSkew(t_transform, p_skew.x, p_skew.y);
	t_transform = MCGAffineTransformPreRotate(t_transform, MCCanvasAngleFromRadians(p_rotation));
	t_transform = MCGAffineTransformPreTranslate(t_transform, p_translation.x, p_translation.y);
	
	return t_transform;
}

bool MCCanvasTransformDecompose(const MCGAffineTransform &p_transform, MCGPoint &r_scale, MCCanvasFloat &r_rotation, MCGPoint &r_skew, MCGPoint &r_translation)
{
	MCGAffineTransform t_transform;
	t_transform = p_transform;
	
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;

	// Remove translation component.
	t_translation = MCGPointMake(t_transform.tx, t_transform.ty);
	t_transform.tx = t_transform.ty = 0;
	
	// Calculate rotation of transformed unit vector
	MCGPoint t_point;
	t_point = MCGPointApplyAffineTransform(MCGPointMake(1, 0), t_transform);
	
	t_rotation = atan2f(t_point.y, t_point.x);
	
	// remove rotation component from transform by applying rotation in the opposite direction
	t_transform = MCGAffineTransformPreRotate(t_transform, MCCanvasAngleFromRadians(-t_rotation));
	
	if (t_transform.a == 0 || t_transform.d == 0)
		return false;
	
	// scale and skew can now be obtained directly from the transform
	t_scale = MCGPointMake(t_transform.a, t_transform.d);
	t_skew = MCGPointMake(t_transform.c / t_transform.d, t_transform.b / t_transform.a);
	
	r_scale = t_scale;
	r_rotation = t_rotation;
	r_skew = t_skew;
	r_translation = t_translation;
	
	return true;
}

MC_DLLEXPORT_DEF
void MCCanvasTransformGetScaleAsList(MCCanvasTransformRef p_transform, MCProperListRef &r_scale)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(p_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		MCCanvasThrowError(kMCCanvasTransformDecomposeErrorTypeInfo);
		return;
	}
	
	/* UNCHECKED */ MCProperListFromPoint(t_scale, r_scale);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformSetScaleAsList(MCProperListRef p_scale, MCCanvasTransformRef &x_transform)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(x_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		MCCanvasThrowError(kMCCanvasTransformDecomposeErrorTypeInfo);
		return;
	}
	
	if (!MCProperListToScale(p_scale, t_scale))
		return;
		
	MCCanvasTransformSetMCGAffineTransform(MCCanvasTransformCompose(t_scale, t_rotation, t_skew, t_translation), x_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformGetRotation(MCCanvasTransformRef p_transform, MCCanvasFloat &r_rotation)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(p_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		MCCanvasThrowError(kMCCanvasTransformDecomposeErrorTypeInfo);
		return;
	}
	r_rotation = MCCanvasAngleFromRadians(t_rotation);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformSetRotation(MCCanvasFloat p_rotation, MCCanvasTransformRef &x_transform)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(x_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		MCCanvasThrowError(kMCCanvasTransformDecomposeErrorTypeInfo);
		return;
	}
	
	MCCanvasTransformSetMCGAffineTransform(MCCanvasTransformCompose(t_scale, MCCanvasAngleToRadians(p_rotation), t_skew, t_translation), x_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformGetSkewAsList(MCCanvasTransformRef p_transform, MCProperListRef &r_skew)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(p_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		MCCanvasThrowError(kMCCanvasTransformDecomposeErrorTypeInfo);
		return;
	}
	
	/* UNCHECKED */ MCProperListFromPoint(t_skew, r_skew);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformSetSkewAsList(MCProperListRef p_skew, MCCanvasTransformRef &x_transform)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(x_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		MCCanvasThrowError(kMCCanvasTransformDecomposeErrorTypeInfo);
		return;
	}
	
	if (!MCProperListToSkew(p_skew, t_skew))
		return;
	
	MCCanvasTransformSetMCGAffineTransform(MCCanvasTransformCompose(t_scale, t_rotation, t_skew, t_translation), x_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformGetTranslationAsList(MCCanvasTransformRef p_transform, MCProperListRef &r_translation)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(p_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		MCCanvasThrowError(kMCCanvasTransformDecomposeErrorTypeInfo);
		return;
	}
	
	/* UNCHECKED */ MCProperListFromPoint(t_translation, r_translation);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformSetTranslationAsList(MCProperListRef p_translation, MCCanvasTransformRef &x_transform)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(x_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		MCCanvasThrowError(kMCCanvasTransformDecomposeErrorTypeInfo);
		return;
	}
	
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasTransformSetMCGAffineTransform(MCCanvasTransformCompose(t_scale, t_rotation, t_skew, t_translation), x_transform);
}

//////////

// Operations

void MCCanvasTransformConcat(MCCanvasTransformRef &x_transform, const MCGAffineTransform &p_transform)
{
	MCCanvasTransformSetMCGAffineTransform(MCGAffineTransformConcat(*MCCanvasTransformGet(x_transform), p_transform), x_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformConcat(MCCanvasTransformRef &x_transform, MCCanvasTransformRef p_transform)
{
	MCCanvasTransformConcat(x_transform, *MCCanvasTransformGet(p_transform));
}

MC_DLLEXPORT_DEF
void MCCanvasTransformScale(MCCanvasTransformRef &x_transform, MCCanvasFloat p_x_scale, MCCanvasFloat p_y_scale)
{
	MCCanvasTransformConcat(x_transform, MCGAffineTransformMakeScale(p_x_scale, p_y_scale));
}

MC_DLLEXPORT_DEF
void MCCanvasTransformScaleWithList(MCCanvasTransformRef &x_transform, MCProperListRef p_scale)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasTransformScale(x_transform, t_scale.x, t_scale.y);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformRotate(MCCanvasTransformRef &x_transform, MCCanvasFloat p_rotation)
{
	MCCanvasTransformConcat(x_transform, MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_rotation)));
}

MC_DLLEXPORT_DEF
void MCCanvasTransformTranslate(MCCanvasTransformRef &x_transform, MCCanvasFloat p_dx, MCCanvasFloat p_dy)
{
	MCCanvasTransformConcat(x_transform, MCGAffineTransformMakeTranslation(p_dx, p_dy));
}

MC_DLLEXPORT_DEF
void MCCanvasTransformTranslateWithList(MCCanvasTransformRef &x_transform, MCProperListRef p_translation)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasTransformTranslate(x_transform, t_translation.x, t_translation.y);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformSkew(MCCanvasTransformRef &x_transform, MCCanvasFloat p_xskew, MCCanvasFloat p_yskew)
{
	MCCanvasTransformConcat(x_transform, MCGAffineTransformMakeSkew(p_xskew, p_yskew));
}

MC_DLLEXPORT_DEF
void MCCanvasTransformSkewWithList(MCCanvasTransformRef &x_transform, MCProperListRef p_skew)
{
	MCGPoint t_skew;
	if (!MCProperListToSkew(p_skew, t_skew))
		return;
	
	MCCanvasTransformSkew(x_transform, t_skew.x, t_skew.y);
}

MC_DLLEXPORT_DEF
void MCCanvasTransformMultiply(MCCanvasTransformRef p_left, MCCanvasTransformRef p_right, MCCanvasTransformRef &r_transform)
{
	MCGAffineTransform t_transform;
	t_transform = MCGAffineTransformConcat(*MCCanvasTransformGet(p_left), *MCCanvasTransformGet(p_right));
	
	MCCanvasTransformMake(t_transform, r_transform);
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
	MCCanvasImageRef t_image = static_cast<MCCanvasImageRef>(p_value);

	uint32_t t_width, t_height;
	if (!MCImageRepGetGeometry(MCCanvasImageGetImageRep (t_image),
	                           t_width, t_height))
		return MCStringCopy (MCSTR("<image>"), r_desc);

	return MCStringFormat(r_desc, "<image %ux%u>", t_width, t_height);
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
	/* UNCHECKED */ MCCanvasImageCreateWithImageRep(p_image, r_image);
}

MC_DLLEXPORT_DEF
void MCCanvasImageMakeWithPath(MCStringRef p_path, MCCanvasImageRef &r_image)
{
	MCImageRep *t_image_rep;
	t_image_rep = nil;
    
    MCObject *t_object = MCEngineCurrentContextObject();
    if (t_object == nullptr)
    {
        return;
    }
    
	if (!MCImageGetRepForFileWithStackContext(p_path, t_object->getstack(), t_image_rep))
	{
		MCCanvasThrowError(kMCCanvasImageRepReferencedErrorTypeInfo);
		return;
	}
	
	MCCanvasImageMake(t_image_rep, r_image);
	MCImageRepRelease(t_image_rep);
}

MC_DLLEXPORT_DEF
void MCCanvasImageMakeWithResourceFile(MCStringRef p_resource, MCCanvasImageRef &r_image)
{
	MCImageRep *t_image_rep;
	t_image_rep = nil;
	
	if (!MCImageGetRepForResource(p_resource, t_image_rep))
	{
		MCCanvasThrowError(kMCCanvasImageRepReferencedErrorTypeInfo);
		return;
	}
	
	MCCanvasImageMake(t_image_rep, r_image);
	MCImageRepRelease(t_image_rep);
}

MC_DLLEXPORT_DEF
void MCCanvasImageMakeWithData(MCDataRef p_data, MCCanvasImageRef &r_image)
{
	MCImageRep *t_image_rep;
	t_image_rep = nil;
	
	if (!MCImageRepCreateWithData(p_data, t_image_rep))
	{
		MCCanvasThrowError(kMCCanvasImageRepDataErrorTypeInfo);
		return;
	}
	
	MCCanvasImageMake(t_image_rep, r_image);
	MCImageRepRelease(t_image_rep);
}

// Input should be unpremultiplied ARGB pixels

MC_DLLEXPORT_DEF
void MCCanvasImageMakeWithPixels(integer_t p_width, integer_t p_height, MCDataRef p_pixels, MCCanvasImageRef &r_image)
{
    MCCanvasImageMakeWithPixelsInFormat(p_width, p_height, p_pixels, kMCGPixelFormatARGB, r_image);
}

MC_DLLEXPORT_DEF
void MCCanvasImageMakeWithPixelsInFormat(integer_t p_width, integer_t p_height, MCDataRef p_pixels, MCGPixelFormat p_format, MCCanvasImageRef &r_image)
{
	MCImageRep *t_image_rep;
	t_image_rep = nil;
	
	if (!MCImageRepCreateWithPixels(p_pixels, p_width, p_height, p_format, false, t_image_rep))
	{
		MCCanvasThrowError(kMCCanvasImageRepPixelsErrorTypeInfo);
		return;
	}
	
	MCCanvasImageMake(t_image_rep, r_image);
	MCImageRepRelease(t_image_rep);
}

MC_DLLEXPORT_DEF
void MCCanvasImageMakeWithPixelsWithSizeAsList(MCProperListRef p_size, MCDataRef p_pixels, MCCanvasImageRef &r_image)
{
	integer_t t_size[2];
	if (!MCProperListFetchAsArrayOfInteger(p_size, 2, t_size))
	{
		MCCanvasThrowError(kMCCanvasImageSizeListFormatErrorTypeInfo);
		return;
	}
	
	MCCanvasImageMakeWithPixels(t_size[0], t_size[1], p_pixels, r_image);
}

// Properties

MC_DLLEXPORT_DEF
void MCCanvasImageGetWidth(MCCanvasImageRef p_image, uint32_t &r_width)
{
	uint32_t t_width, t_height;
	if (!MCImageRepGetGeometry(MCCanvasImageGetImageRep(p_image), t_width, t_height))
	{
		MCCanvasThrowError(kMCCanvasImageRepGetGeometryErrorTypeInfo);
		return;
	}
	r_width = t_width;
}

MC_DLLEXPORT_DEF
void MCCanvasImageGetHeight(MCCanvasImageRef p_image, uint32_t &r_height)
{
	uint32_t t_width, t_height;
	if (!MCImageRepGetGeometry(MCCanvasImageGetImageRep(p_image), t_width, t_height))
	{
		MCCanvasThrowError(kMCCanvasImageRepGetGeometryErrorTypeInfo);
		return;
	}
	r_height = t_height;
}

MC_DLLEXPORT_DEF
void MCCanvasImageGetMetadata(MCCanvasImageRef p_image, MCArrayRef &r_metadata)
{
	if (!MCImageRepGetMetadata(MCCanvasImageGetImageRep(p_image), r_metadata))
		MCCanvasThrowError(kMCCanvasImageRepGetMetadataErrorTypeInfo);
}

MC_DLLEXPORT_DEF
void MCCanvasImageGetDensity(MCCanvasImageRef p_image, double &r_density)
{
	if (!MCImageRepGetDensity(MCCanvasImageGetImageRep(p_image), r_density))
		MCCanvasThrowError(kMCCanvasImageRepGetDensityErrorTypeInfo);
}

MC_DLLEXPORT_DEF
void MCCanvasImageGetPixels(MCCanvasImageRef p_image, MCDataRef &r_pixels)
{
	MCImageRep *t_image_rep;
	t_image_rep = MCCanvasImageGetImageRep(p_image);
	
	MCImageBitmap *t_raster;
	
	// TODO - handle case of missing normal density image
	
	if (!MCImageRepLockRaster(t_image_rep, 0, 1.0, t_raster))
	{
		MCCanvasThrowError(kMCCanvasImageRepLockErrorTypeInfo);
		return;
	}
	
	uint8_t *t_buffer;
	t_buffer = nil;
	
	uint32_t t_buffer_size;
	t_buffer_size = t_raster->height * t_raster->stride;
	
	/* UNCHECKED */ MCMemoryAllocate(t_buffer_size, t_buffer);
    uint32_t *t_buffer_ptr = (uint32_t *)t_buffer;
	uint8_t *t_pixel_row;
	t_pixel_row = (uint8_t*)t_raster->data;
	
	for (uint32_t y = 0; y < t_raster->height; y++)
	{
		uint32_t *t_pixel_ptr;
		t_pixel_ptr = (uint32_t*)t_pixel_row;
		
		for (uint32_t x = 0; x < t_raster->width; x++)
		{
			*t_buffer_ptr++ = MCGPixelFromNative(kMCGPixelFormatARGB, *t_pixel_ptr);
			t_pixel_ptr++;
		}
		
		t_pixel_row += t_raster->stride;
	}
	
	/* UNCHECKED */ MCDataCreateWithBytesAndRelease(t_buffer, t_buffer_size, r_pixels);
	
	MCImageRepUnlockRaster(t_image_rep, 0, t_raster);
}

////////////////////////////////////////////////////////////////////////////////

// Solid Paint

static void __MCCanvasSolidPaintDestroy(MCValueRef p_value)
{
	MCValueRelease(MCCanvasSolidPaintGet((MCCanvasSolidPaintRef)p_value)->color);
}

static bool __MCCanvasSolidPaintCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasSolidPaintEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCValueIsEqualTo(MCCanvasSolidPaintGet((MCCanvasSolidPaintRef)p_left)->color, MCCanvasSolidPaintGet((MCCanvasSolidPaintRef)p_right)->color);
}

static hash_t __MCCanvasSolidPaintHash(MCValueRef p_value)
{
	return MCValueHash(MCCanvasSolidPaintGet((MCCanvasSolidPaintRef)p_value)->color);
}

static bool __MCCanvasSolidPaintDescribe(MCValueRef p_value, MCStringRef &r_string)
{
	return false;
}

bool MCCanvasSolidPaintCreateWithColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &r_paint)
{
	bool t_success;
	t_success = true;
	
	MCCanvasSolidPaintRef t_paint;
	t_paint = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasSolidPaintTypeInfo, sizeof(__MCCanvasSolidPaintImpl), t_paint);
	
	if (t_success)
	{
		__MCCanvasSolidPaintImpl *t_impl;
		t_impl = MCCanvasSolidPaintGet(t_paint);
		
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

bool MCCanvasPaintIsSolidPaint(MCCanvasPaintRef p_paint)
{
	return MCValueGetTypeInfo(p_paint) == kMCCanvasSolidPaintTypeInfo;
}

// Constructor

MC_DLLEXPORT_DEF
void MCCanvasSolidPaintMakeWithColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &r_paint)
{
	/* UNCHECKED */ MCCanvasSolidPaintCreateWithColor(p_color, r_paint);
}

// Properties

MC_DLLEXPORT_DEF
void MCCanvasSolidPaintGetColor(MCCanvasSolidPaintRef p_paint, MCCanvasColorRef &r_color)
{
	r_color = MCValueRetain(MCCanvasSolidPaintGet(p_paint)->color);
}

MC_DLLEXPORT_DEF
void MCCanvasSolidPaintSetColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &x_paint)
{
	MCCanvasSolidPaintRef t_paint;
	t_paint = nil;
	
	if (!MCCanvasSolidPaintCreateWithColor(p_color, t_paint))
		return;
	
	MCValueAssign(x_paint, t_paint);
	MCValueRelease(t_paint);
}

////////////////////////////////////////////////////////////////////////////////

// Pattern

static void __MCCanvasPatternDestroy(MCValueRef p_value)
{
	MCValueRelease(MCCanvasPatternGet((MCCanvasPatternRef)p_value)->image);
}

static bool __MCCanvasPatternCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasPatternEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCValueIsEqualTo(MCCanvasPatternGet((MCCanvasPatternRef)p_left)->image, MCCanvasPatternGet((MCCanvasPatternRef)p_right)->image);
}

static hash_t __MCCanvasPatternHash(MCValueRef p_value)
{
	__MCCanvasPatternImpl *t_pattern;
	t_pattern = MCCanvasPatternGet((MCCanvasPatternRef)p_value);
	
	// TODO - ask Mark how to combine hash values
	return MCValueHash(t_pattern->image) ^ MCValueHash(t_pattern->transform);
}

static bool __MCCanvasPatternDescribe(MCValueRef p_value, MCStringRef &r_string)
{
	return false;
}

bool MCCanvasPatternCreateWithImage(MCCanvasImageRef p_image, MCCanvasTransformRef p_transform, MCCanvasPatternRef &r_paint)
{
	bool t_success;
	t_success = true;
	
	MCCanvasPatternRef t_paint;
	t_paint = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasPatternTypeInfo, sizeof(__MCCanvasPatternImpl), t_paint);
	
	if (t_success)
	{
		__MCCanvasPatternImpl *t_impl;
		t_impl = MCCanvasPatternGet(t_paint);
		
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

bool MCCanvasPaintIsPattern(MCCanvasPaintRef p_paint)
{
	return MCValueGetTypeInfo(p_paint) == kMCCanvasPatternTypeInfo;
}

// Constructor

void MCCanvasPatternMakeWithTransformedImage(MCCanvasImageRef p_image, MCCanvasTransformRef p_transform, MCCanvasPatternRef &r_pattern)
{
	/* UNCHECKED */ MCCanvasPatternCreateWithImage(p_image, p_transform, r_pattern);
}

MC_DLLEXPORT_DEF
void MCCanvasPatternMakeWithTransformedImage(MCCanvasImageRef p_image, const MCGAffineTransform &p_transform, MCCanvasPatternRef &r_pattern)
{
	MCCanvasTransformRef t_transform;
	t_transform = nil;
	
	MCCanvasTransformMake(p_transform, t_transform);
	if (!MCErrorIsPending())
		MCCanvasPatternMakeWithTransformedImage(p_image, t_transform, r_pattern);
	MCValueRelease(t_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasPatternMakeWithImage(MCCanvasImageRef p_image, MCCanvasPatternRef &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, kMCCanvasIdentityTransform, r_pattern);
}

MC_DLLEXPORT_DEF
void MCCanvasPatternMakeWithScaledImage(MCCanvasImageRef p_image, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasPatternRef &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeScale(p_xscale, p_yscale), r_pattern);
}

MC_DLLEXPORT_DEF
void MCCanvasPatternMakeWithImageScaledWithList(MCCanvasImageRef p_image, MCProperListRef p_scale, MCCanvasPatternRef &r_pattern)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasPatternMakeWithScaledImage(p_image, t_scale.x, t_scale.y, r_pattern);
}

MC_DLLEXPORT_DEF
void MCCanvasPatternMakeWithRotatedImage(MCCanvasImageRef p_image, MCCanvasFloat p_angle, MCCanvasPatternRef &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_angle)), r_pattern);
}

MC_DLLEXPORT_DEF
void MCCanvasPatternMakeWithTranslatedImage(MCCanvasImageRef p_image, MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPatternRef &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeTranslation(p_x, p_y), r_pattern);
}

MC_DLLEXPORT_DEF
void MCCanvasPatternMakeWithImageTranslatedWithList(MCCanvasImageRef p_image, MCProperListRef p_translation, MCCanvasPatternRef &r_pattern)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasPatternMakeWithTranslatedImage(p_image, t_translation.x, t_translation.y, r_pattern);
}

// Properties

void MCCanvasPatternSet(MCCanvasImageRef p_image, MCCanvasTransformRef p_transform, MCCanvasPatternRef &x_pattern)
{
	MCCanvasPatternRef t_pattern;
	if (!MCCanvasPatternCreateWithImage(p_image, p_transform, t_pattern))
		return;
	
	MCValueAssign(x_pattern, t_pattern);
	MCValueRelease(t_pattern);
}

MC_DLLEXPORT_DEF
void MCCanvasPatternGetImage(MCCanvasPatternRef p_pattern, MCCanvasImageRef &r_image)
{
	r_image = MCValueRetain(MCCanvasPatternGet(p_pattern)->image);
}

MC_DLLEXPORT_DEF
void MCCanvasPatternSetImage(MCCanvasImageRef p_image, MCCanvasPatternRef &x_pattern)
{
	MCCanvasPatternSet(p_image, MCCanvasPatternGet(x_pattern)->transform, x_pattern);
}

MC_DLLEXPORT_DEF
void MCCanvasPatternGetTransform(MCCanvasPatternRef p_pattern, MCCanvasTransformRef &r_transform)
{
	r_transform = MCValueRetain(MCCanvasPatternGet(p_pattern)->transform);
}

MC_DLLEXPORT_DEF
void MCCanvasPatternSetTransform(MCCanvasTransformRef p_transform, MCCanvasPatternRef &x_pattern)
{
	MCCanvasPatternSet(MCCanvasPatternGet(x_pattern)->image, p_transform, x_pattern);
}

// Operators

void MCCanvasPatternTransform(MCCanvasPatternRef &x_pattern, const MCGAffineTransform &p_transform)
{
	MCCanvasTransformRef t_transform;
	t_transform = MCValueRetain(MCCanvasPatternGet(x_pattern)->transform);
	
	MCCanvasTransformConcat(t_transform, p_transform);
	
	if (!MCErrorIsPending())
		MCCanvasPatternSetTransform(t_transform, x_pattern);
	
	MCValueRelease(t_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasPatternTransform(MCCanvasPatternRef &x_pattern, MCCanvasTransformRef p_transform)
{
	MCCanvasPatternTransform(x_pattern, *MCCanvasTransformGet(p_transform));
}

MC_DLLEXPORT_DEF
void MCCanvasPatternScale(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

MC_DLLEXPORT_DEF
void MCCanvasPatternScaleWithList(MCCanvasPatternRef &x_pattern, MCProperListRef p_scale)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasPatternScale(x_pattern, t_scale.x, t_scale.y);
}

MC_DLLEXPORT_DEF
void MCCanvasPatternRotate(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_angle)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_angle)));
}

MC_DLLEXPORT_DEF
void MCCanvasPatternTranslate(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeTranslation(p_x, p_y));
}

MC_DLLEXPORT_DEF
void MCCanvasPatternTranslateWithList(MCCanvasPatternRef &x_pattern, MCProperListRef p_translation)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasPatternTranslate(x_pattern, t_translation.x, t_translation.y);
}

////////////////////////////////////////////////////////////////////////////////

// Gradient Stop

static void __MCCanvasGradientStopDestroy(MCValueRef p_stop)
{
	MCValueRelease(MCCanvasGradientStopGet((MCCanvasGradientStopRef)p_stop)->color);
}

static bool __MCCanvasGradientStopCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasGradientStopEqual(MCValueRef p_left, MCValueRef p_right)
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

MC_DLLEXPORT_DEF
void MCCanvasGradientStopMake(MCCanvasFloat p_offset, MCCanvasColorRef p_color, MCCanvasGradientStopRef &r_stop)
{
	/* UNCHECKED */ MCCanvasGradientStopCreate(p_offset, p_color, r_stop);
}

//	Properties

void MCCanvasGradientStopSet(MCCanvasFloat p_offset, MCCanvasColorRef p_color, MCCanvasGradientStopRef &x_stop)
{
	MCCanvasGradientStopRef t_stop;
	if (!MCCanvasGradientStopCreate(p_offset, p_color, t_stop))
		return;
	MCValueAssign(x_stop, t_stop);
	MCValueRelease(t_stop);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientStopGetOffset(MCCanvasGradientStopRef p_stop, MCCanvasFloat &r_offset)
{
	r_offset = MCCanvasGradientStopGet(p_stop)->offset;
}

MC_DLLEXPORT_DEF
void MCCanvasGradientStopSetOffset(MCCanvasFloat p_offset, MCCanvasGradientStopRef &x_stop)
{
	__MCCanvasGradientStopImpl *t_stop;
	t_stop = MCCanvasGradientStopGet(x_stop);
	
	MCCanvasGradientStopSet(p_offset, t_stop->color, x_stop);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientStopGetColor(MCCanvasGradientStopRef p_stop, MCCanvasColorRef &r_color)
{
	r_color = MCValueRetain(MCCanvasGradientStopGet(p_stop)->color);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientStopSetColor(MCCanvasColorRef p_color, MCCanvasGradientStopRef &x_stop)
{
	__MCCanvasGradientStopImpl *t_stop;
	t_stop = MCCanvasGradientStopGet(x_stop);
	
	MCCanvasGradientStopSet(t_stop->offset, p_color, x_stop);
}

// Gradient

static void __MCCanvasGradientDestroy(MCValueRef p_value)
{
	__MCCanvasGradientImpl *t_gradient;
	t_gradient = MCCanvasGradientGet((MCCanvasGradientRef)p_value);
	
	MCValueRelease(t_gradient->ramp);
	MCValueRelease(t_gradient->transform);
}

static bool __MCCanvasGradientCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasGradientEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	__MCCanvasGradientImpl *t_left, *t_right;
	t_left = MCCanvasGradientGet((MCCanvasGradientRef)p_left);
	t_right = MCCanvasGradientGet((MCCanvasGradientRef)p_right);
	
	return t_left->function == t_right->function &&
	MCValueIsEqualTo(t_left->ramp, t_right->ramp) &&
	t_left->mirror == t_right->mirror &&
	t_left->wrap == t_right->wrap &&
	t_left->repeats == t_right->repeats &&
	MCValueIsEqualTo(t_left->transform, t_right->transform) &&
	t_left->filter == t_right->filter;
}

static hash_t __MCCanvasGradientHash(MCValueRef p_value)
{
	__MCCanvasGradientImpl *t_gradient;
	t_gradient = MCCanvasGradientGet((MCCanvasGradientRef)p_value);
	// TODO - ask Mark how to combine hash values
	return MCHashInteger(t_gradient->mirror | (t_gradient->wrap < 1)) ^ MCHashInteger(t_gradient->filter) ^ MCValueHash(t_gradient->ramp) ^ MCHashInteger(t_gradient->repeats) ^ MCValueHash(t_gradient->transform) ^ MCHashInteger(t_gradient->filter);
}

static bool __MCCanvasGradientDescribe(MCValueRef p_value, MCStringRef &r_string)
{
	return false;
}

bool MCCanvasGradientCreate(const __MCCanvasGradientImpl &p_gradient, MCCanvasGradientRef &r_paint)
{
	bool t_success;
	t_success = true;
	
	MCCanvasGradientRef t_paint;
	t_paint = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasGradientTypeInfo, sizeof(__MCCanvasGradientImpl), t_paint);
	
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

bool MCCanvasPaintIsGradient(MCCanvasPaintRef p_paint)
{
	return MCValueGetTypeInfo(p_paint) == kMCCanvasGradientTypeInfo;
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
		
		MCCanvasFloat t_stop_offset;
		MCCanvasGradientStopGetOffset(t_stop, t_stop_offset);
		
		if (t_stop_offset < 0 || t_stop_offset > 1)
		{
			MCCanvasThrowError(kMCCanvasGradientStopRangeErrorTypeInfo);
			return false;
		}
		
		if (t_stop_offset < MCCanvasGradientStopGet(t_prev_stop)->offset)
		{
			MCCanvasThrowError(kMCCanvasGradientStopOrderErrorTypeInfo);
			return false;
		}
	}
	
	return true;
}

//////////

MC_DLLEXPORT_DEF
void MCCanvasGradientEvaluateType(integer_t p_type, integer_t& r_type)
{
    r_type = p_type;
}

// Constructor

MC_DLLEXPORT_DEF
void MCCanvasGradientMakeWithRamp(integer_t p_type, MCProperListRef p_ramp, MCCanvasGradientRef &r_gradient)
{
	if (MCProperListGetLength(p_ramp) == 0)
	{
		MCCanvasThrowError(kMCCanvasGradientInvalidRampErrorTypeInfo);
		return;
	}
	
	if (!MCCanvasGradientCheckStopOrder(p_ramp))
	{
		return;
	}
	
	__MCCanvasGradientImpl t_gradient_impl;
	MCMemoryClear(&t_gradient_impl, sizeof(__MCCanvasGradientImpl));
	t_gradient_impl.function = (MCGGradientFunction)p_type;
	t_gradient_impl.mirror = false;
	t_gradient_impl.wrap = false;
	t_gradient_impl.repeats = 1;
	t_gradient_impl.transform = kMCCanvasIdentityTransform;
	t_gradient_impl.filter = kMCGImageFilterNone;
    t_gradient_impl.ramp = p_ramp;
	
	/* UNCHECKED */ MCCanvasGradientCreate(t_gradient_impl, r_gradient);
}

// Properties

void MCCanvasGradientSet(const __MCCanvasGradientImpl &p_gradient, MCCanvasGradientRef &x_gradient)
{
	MCCanvasGradientRef t_gradient;
	if (!MCCanvasGradientCreate(p_gradient, t_gradient))
		return;
	
	MCValueAssign(x_gradient, t_gradient);
	MCValueRelease(t_gradient);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientGetRamp(MCCanvasGradientRef p_gradient, MCProperListRef &r_ramp)
{
	r_ramp = MCValueRetain(MCCanvasGradientGet(p_gradient)->ramp);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientSetRamp(MCProperListRef p_ramp, MCCanvasGradientRef &x_gradient)
{
	if (!MCCanvasGradientCheckStopOrder(p_ramp))
		return;
	
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	t_gradient.ramp = p_ramp;
	
	MCCanvasGradientSet(t_gradient, x_gradient);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientGetTypeAsString(MCCanvasGradientRef p_gradient, MCStringRef &r_string)
{
	/* UNCHECKED */ MCCanvasGradientTypeToString(MCCanvasGradientGet(p_gradient)->function, r_string);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientSetTypeAsString(MCStringRef p_string, MCCanvasGradientRef &x_gradient)
{
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	if (!MCCanvasGradientTypeFromString(p_string, t_gradient.function))
	{
		MCCanvasThrowError(kMCCanvasGradientTypeErrorTypeInfo);
		return;
	}
	
	MCCanvasGradientSet(t_gradient, x_gradient);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientGetRepeat(MCCanvasGradientRef p_gradient, integer_t &r_repeat)
{
	r_repeat = MCCanvasGradientGet(p_gradient)->repeats;
}

MC_DLLEXPORT_DEF
void MCCanvasGradientSetRepeat(integer_t p_repeat, MCCanvasGradientRef &x_gradient)
{
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	t_gradient.repeats = p_repeat;
	
	MCCanvasGradientSet(t_gradient, x_gradient);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientGetWrap(MCCanvasGradientRef p_gradient, bool &r_wrap)
{
	r_wrap = MCCanvasGradientGet(p_gradient)->wrap;
}

MC_DLLEXPORT_DEF
void MCCanvasGradientSetWrap(bool p_wrap, MCCanvasGradientRef &x_gradient)
{
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	t_gradient.wrap = p_wrap;
	
	MCCanvasGradientSet(t_gradient, x_gradient);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientGetMirror(MCCanvasGradientRef p_gradient, bool &r_mirror)
{
	r_mirror = MCCanvasGradientGet(p_gradient)->mirror;
}

MC_DLLEXPORT_DEF
void MCCanvasGradientSetMirror(bool p_mirror, MCCanvasGradientRef &x_gradient)
{
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	t_gradient.mirror = p_mirror;
	
	MCCanvasGradientSet(t_gradient, x_gradient);
}

void MCCanvasGradientTransformToPoints(const MCGAffineTransform &p_transform, MCGPoint &r_from, MCGPoint &r_to, MCGPoint &r_via)
{
	r_from = MCGPointApplyAffineTransform(MCGPointMake(0, 0), p_transform);
	r_to = MCGPointApplyAffineTransform(MCGPointMake(1, 0), p_transform);
	r_via = MCGPointApplyAffineTransform(MCGPointMake(0, 1), p_transform);
}

void MCCanvasGradientTransformFromPoints(const MCGPoint &p_from, const MCGPoint &p_to, const MCGPoint &p_via, MCGAffineTransform &r_transform)
{
	MCGAffineTransform t_transform;
	t_transform . a = p_to . x - p_from . x;
	t_transform . b = p_to . y - p_from . y;
	t_transform . c = p_via . x - p_from . x;
	t_transform . d = p_via . y - p_from . y;
	t_transform . tx = p_from . x;
	t_transform . ty = p_from . y;
	
	r_transform = t_transform;
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
		MCCanvasGradientSetTransform(t_transform, x_gradient);
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
	MCCanvasGradientTransformFromPoints(p_from, p_to, p_via, t_transform);
	MCCanvasGradientSetTransform(x_gradient, t_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientGetFrom(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_from)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	
	/* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(t_from, r_from);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientGetTo(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_to)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	
	/* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(t_to, r_to);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientGetVia(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_via)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	
	/* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(t_via, r_via);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientSetFrom(MCCanvasPointRef p_from, MCCanvasGradientRef &x_gradient)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(x_gradient, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, *MCCanvasPointGet(p_from), t_to, t_via);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientSetTo(MCCanvasPointRef p_to, MCCanvasGradientRef &x_gradient)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(x_gradient, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, t_from, *MCCanvasPointGet(p_to), t_via);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientSetVia(MCCanvasPointRef p_via, MCCanvasGradientRef &x_gradient)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(x_gradient, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, t_from, t_to, *MCCanvasPointGet(p_via));
}

MC_DLLEXPORT_DEF
void MCCanvasGradientGetTransform(MCCanvasGradientRef p_gradient, MCCanvasTransformRef &r_transform)
{
	r_transform = MCValueRetain(MCCanvasGradientGet(p_gradient)->transform);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientSetTransform(MCCanvasTransformRef p_transform, MCCanvasGradientRef &x_gradient)
{
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	t_gradient.transform = p_transform;
	MCCanvasGradientSet(t_gradient, x_gradient);
}

// Operators

// TODO - replace this with a binary search :)
bool MCProperListGetGradientStopInsertionPoint(MCProperListRef p_list, MCCanvasGradientStopRef p_stop, uindex_t &r_index)
{
	uindex_t t_length;
	t_length = MCProperListGetLength(p_list);
	
	MCCanvasFloat t_offset;
	t_offset = MCCanvasGradientStopGet(p_stop)->offset;
	
	for (uindex_t i = 0; i < t_length; i++)
	{
		MCCanvasGradientStopRef t_stop;
		if (!MCProperListFetchGradientStopAt(p_list, i, t_stop))
			return false;
		if (t_offset < MCCanvasGradientStopGet(p_stop)->offset)
		{
			r_index = i;
			return true;
		}
	}
	
	r_index = t_length;
	
	return true;
}

MC_DLLEXPORT_DEF
void MCCanvasGradientAddStop(MCCanvasGradientStopRef p_stop, MCCanvasGradientRef &x_gradient)
{
	bool t_success;
	t_success = true;
	
	__MCCanvasGradientStopImpl *t_new_stop;
	t_new_stop = MCCanvasGradientStopGet(p_stop);
	
	if (t_new_stop->offset < 0 || t_new_stop->offset > 1)
	{
		MCCanvasThrowError(kMCCanvasGradientStopRangeErrorTypeInfo);
		return;
	}
	
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	MCProperListRef t_mutable_ramp;
	t_mutable_ramp = nil;
	
	if (t_success)
		t_success = MCProperListMutableCopy(t_gradient.ramp, t_mutable_ramp);
	
	uindex_t t_index;
	
	if (t_success)
		t_success = MCProperListGetGradientStopInsertionPoint(t_mutable_ramp, p_stop, t_index);
	
	if (t_success)
		t_success = MCProperListInsertElement(t_mutable_ramp, p_stop, t_index);
	
	MCProperListRef t_new_ramp;
	t_new_ramp = nil;
	
	if (t_success)
		t_success = MCProperListCopyAndRelease(t_mutable_ramp, t_new_ramp);
	
	if (t_success)
	{
		t_gradient.ramp = t_new_ramp;
		MCCanvasGradientSet(t_gradient, x_gradient);
		MCValueRelease(t_new_ramp);
	}
	else
		MCValueRelease(t_mutable_ramp);
}

void MCCanvasGradientTransform(MCCanvasGradientRef &x_gradient, const MCGAffineTransform &p_transform)
{
	MCCanvasTransformRef t_transform;
	t_transform = MCValueRetain(MCCanvasGradientGet(x_gradient)->transform);
	
	MCCanvasTransformConcat(t_transform, p_transform);
	
	if (!MCErrorIsPending())
		MCCanvasGradientSetTransform(t_transform, x_gradient);
	
	MCValueRelease(t_transform);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientTransform(MCCanvasGradientRef &x_gradient, MCCanvasTransformRef p_transform)
{
	MCCanvasGradientTransform(x_gradient, *MCCanvasTransformGet(p_transform));
}

MC_DLLEXPORT_DEF
void MCCanvasGradientScale(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

MC_DLLEXPORT_DEF
void MCCanvasGradientScaleWithList(MCCanvasGradientRef &x_gradient, MCProperListRef p_scale)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasGradientScale(x_gradient, t_scale.x, t_scale.y);
}

MC_DLLEXPORT_DEF
void MCCanvasGradientRotate(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_angle)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_angle)));
}

MC_DLLEXPORT_DEF
void MCCanvasGradientTranslate(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeTranslation(p_x, p_y));
}

MC_DLLEXPORT_DEF
void MCCanvasGradientTranslateWithList(MCCanvasGradientRef &x_gradient, MCProperListRef p_translation)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasGradientTranslate(x_gradient, t_translation.x, t_translation.y);
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
	
	return MCGPathIsEqualTo(MCCanvasPathGetMCGPath((MCCanvasPathRef)p_left), MCCanvasPathGetMCGPath((MCCanvasPathRef)p_right));
}

static bool __MCCanvasPathHashCallback(void *p_context, MCGPathCommand p_command, MCGPoint *p_points, uint32_t p_point_count)
{
	hash_t *t_hash;
	t_hash = static_cast<hash_t*>(p_context);
	
	*t_hash ^= MCHashInteger(p_command);
	for (uint32_t i = 0; i < p_point_count; i++)
	{
		*t_hash ^= MCHashDouble(p_points[i].x);
		*t_hash ^= MCHashDouble(p_points[i].y);
	}
	
	return true;
}

static hash_t __MCCanvasPathHash(MCValueRef p_value)
{
	hash_t t_hash;
	t_hash = 0;
	
	/* UNCHECKED */ MCGPathIterate(MCCanvasPathGetMCGPath((MCCanvasPathRef)p_value), __MCCanvasPathHashCallback, &t_hash);
	
	return t_hash;
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
		MCGPathCopy(p_path, *MCCanvasPathGet(t_path));
		t_success = MCGPathIsValid(*MCCanvasPathGet(t_path));
	}
	
	if (t_success)
		t_success = MCValueInter(t_path, r_path);
	
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

bool MCCanvasPathCreateEmpty(MCCanvasPathRef &r_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_gpath;
	t_gpath = nil;
	
	MCCanvasPathRef t_path;
	t_path = nil;
	
	if (t_success)
		t_success = MCGPathCreateMutable(t_gpath);
	
	if (t_success)
		t_success = MCCanvasPathCreateWithMCGPath(t_gpath, t_path);
	
	MCGPathRelease(t_gpath);
	
	if (t_success)
		r_path = t_path;
	
	return t_success;
}

//////////

bool MCProperListToRadii(MCProperListRef p_list, MCGPoint &r_radii)
{
	bool t_success;
	t_success = true;
	
	real64_t t_radii[2];
	
	if (t_success)
		t_success = MCProperListFetchAsArrayOfReal(p_list, 2, t_radii);
	
	if (t_success)
		r_radii = MCGPointMake(t_radii[0], t_radii[1]);
	else
		MCCanvasThrowError(kMCCanvasRadiiListFormatErrorTypeInfo);
	
	return t_success;
}

// Constructors

MC_DLLEXPORT_DEF
void MCCanvasPathMakeEmpty(MCCanvasPathRef &r_path)
{
	r_path = MCValueRetain(kMCCanvasEmptyPath);
}

struct MCCanvasPathSVGParseContext
{
	MCGPathRef path;
	MCGPoint first_point;
	MCGPoint last_point;
	MCGPoint last_control;
	MCSVGPathCommand last_command;
};

bool MCCanvasPathSVGParseCallback(void *p_context, MCSVGPathCommand p_command, float32_t *p_params, uindex_t p_param_count)
{
	MCCanvasPathSVGParseContext *t_context;
	t_context = static_cast<MCCanvasPathSVGParseContext*>(p_context);
	
	MCGPoint t_origin;
	if (MCSVGPathCommandIsRelative(p_command))
		t_origin = t_context->last_point;
	else
		t_origin = MCGPointMake(0, 0);
	
	switch (p_command)
	{
		case kMCSVGPathMoveTo:
		case kMCSVGPathRelativeMoveTo:
		{
			MCGPoint t_point;
			t_point = MCGPointRelativeToAbsolute(t_origin, MCGPointMake(p_params[0], p_params[1]));
			MCGPathMoveTo(t_context->path, t_point);
			t_context->last_point = t_context->first_point = t_point;
			break;
		}
			
		case kMCSVGPathClose:
			MCGPathCloseSubpath(t_context->path);
			t_context->last_point = t_context->first_point;
			break;
			
		case kMCSVGPathLineTo:
		case kMCSVGPathRelativeLineTo:
		{
			MCGPoint t_point;
			t_point = MCGPointRelativeToAbsolute(t_origin, MCGPointMake(p_params[0], p_params[1]));
			MCGPathLineTo(t_context->path, t_point);
			t_context->last_point = t_point;
			break;
		}
			
		case kMCSVGPathHorizontalLineTo:
		case kMCSVGPathRelativeHorizontalLineTo:
		{
			MCGPoint t_point;
			t_point = t_context->last_point;
			t_point.x = t_origin.x + p_params[0];
			MCGPathLineTo(t_context->path, t_point);
			t_context->last_point = t_point;
			break;
		}
			
		case kMCSVGPathVerticalLineTo:
		case kMCSVGPathRelativeVerticalLineTo:
		{
			MCGPoint t_point;
			t_point = t_context->last_point;
			t_point.y = t_origin.y + p_params[0];
			MCGPathLineTo(t_context->path, t_point);
			t_context->last_point = t_point;
			break;
		}
			
		case kMCSVGPathCurveTo:
		case kMCSVGPathRelativeCurveTo:
		{
			MCGPoint t_point[3];
			t_point[0] = MCGPointRelativeToAbsolute(t_origin, MCGPointMake(p_params[0], p_params[1]));
			t_point[1] = MCGPointRelativeToAbsolute(t_origin, MCGPointMake(p_params[2], p_params[3]));
			t_point[2] = MCGPointRelativeToAbsolute(t_origin, MCGPointMake(p_params[4], p_params[5]));
			
			MCGPathCubicTo(t_context->path, t_point[0], t_point[1], t_point[2]);
			t_context->last_point = t_point[2];
			t_context->last_control = t_point[1];
			break;
		}
			
		case kMCSVGPathShorthandCurveTo:
		case kMCSVGPathRelativeShorthandCurveTo:
		{
			MCGPoint t_point[3];
			t_point[1] = MCGPointRelativeToAbsolute(t_origin, MCGPointMake(p_params[0], p_params[1]));
			t_point[2] = MCGPointRelativeToAbsolute(t_origin, MCGPointMake(p_params[2], p_params[3]));
			if (MCSVGPathCommandIsCubic(t_context->last_command))
				t_point[0] = MCGPointReflect(t_context->last_point, t_context->last_control);
			else
				t_point[0] = t_point[1];
			
			MCGPathCubicTo(t_context->path, t_point[0], t_point[1], t_point[2]);
			t_context->last_point = t_point[2];
			t_context->last_control = t_point[1];
			break;
		}
			
		case kMCSVGPathQuadraticCurveTo:
		case kMCSVGPathRelativeQuadraticCurveTo:
		{
			MCGPoint t_point[2];
			t_point[0] = MCGPointRelativeToAbsolute(t_origin, MCGPointMake(p_params[0], p_params[1]));
			t_point[1] = MCGPointRelativeToAbsolute(t_origin, MCGPointMake(p_params[2], p_params[3]));

			MCGPathQuadraticTo(t_context->path, t_point[0], t_point[1]);
			t_context->last_point = t_point[1];
			t_context->last_control = t_point[0];
			break;
		}
			
		case kMCSVGPathShorthandQuadraticCurveTo:
		case kMCSVGPathRelativeShorthandQuadraticCurveTo:
		{
			MCGPoint t_point[2];
			t_point[1] = MCGPointRelativeToAbsolute(t_origin, MCGPointMake(p_params[0], p_params[1]));
			if (MCSVGPathCommandIsQuadratic(t_context->last_command))
				t_point[0] = MCGPointReflect(t_context->last_point, t_context->last_control);
			else
				t_point[0] = t_point[1];
			
			MCGPathQuadraticTo(t_context->path, t_point[0], t_point[1]);
			t_context->last_point = t_point[1];
			t_context->last_control = t_point[0];
			break;
		}
			
		case kMCSVGPathEllipticalCurveTo:
		case kMCSVGPathRelativeEllipticalCurveTo:
		{
			MCCanvasFloat t_rx, t_ry;
			MCCanvasFloat t_xrotation;
			bool t_large_arc, t_sweep;
			MCGPoint t_point;
			t_rx = p_params[0]; t_ry = p_params[1];
			t_xrotation = p_params[2];
			t_large_arc = p_params[3] != 0;
			t_sweep = p_params[4] != 0;
			t_point = MCGPointRelativeToAbsolute(t_origin, MCGPointMake(p_params[5], p_params[6]));
			
			MCGPathArcTo(t_context->path, MCGSizeMake(t_rx, t_ry), t_xrotation, t_large_arc, t_sweep, t_point);
			t_context->last_point = t_point;
			break;
		}
			
		default:
			MCUnreachable();
			break;
	}
	
	t_context->last_command = p_command;
	
	return MCGPathIsValid(t_context->path);
}

void MCCanvasPathMakeWithMCGPath(MCGPathRef p_path, MCCanvasPathRef &r_path)
{
	/* UNCHECKED */ MCCanvasPathCreateWithMCGPath(p_path, r_path);
}

// TODO - investigate error handling in libgraphics, libskia - don't think skia mem errors are tested for
MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithInstructionsAsString(MCStringRef p_instructions, MCCanvasPathRef &r_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
		t_success = MCGPathCreateMutable(t_path);
	
	if (t_success)
	{
		MCCanvasPathSVGParseContext t_context;
		t_context.path = t_path;
		t_context.first_point = t_context.last_point = MCGPointMake(0, 0);
		t_success = MCSVGParse(p_instructions, MCCanvasPathSVGParseCallback, &t_context);
	}
	
	if (t_success)
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithRoundedRectangleWithRadii(MCCanvasRectangleRef p_rect, MCCanvasFloat p_x_radius, MCCanvasFloat p_y_radius, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
		return;
	
	MCGPathAddRoundedRectangle(t_path, *MCCanvasRectangleGet(p_rect), MCGSizeMake(p_x_radius, p_y_radius));
	if (MCGPathIsValid(t_path))
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithRoundedRectangle(MCCanvasRectangleRef p_rect, MCCanvasFloat p_radius, MCCanvasPathRef &r_path)
{
	MCCanvasPathMakeWithRoundedRectangleWithRadii(p_rect, p_radius, p_radius, r_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithRoundedRectangleWithRadiiAsList(MCCanvasRectangleRef p_rect, MCProperListRef p_radii, MCCanvasPathRef &r_path)
{
	MCGPoint t_radii;
	if (!MCProperListToRadii(p_radii, t_radii))
		return;
	
	MCCanvasPathMakeWithRoundedRectangleWithRadii(p_rect, t_radii.x, t_radii.y, r_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithRectangle(MCCanvasRectangleRef p_rect, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
		return;
	
	MCGPathAddRectangle(t_path, *MCCanvasRectangleGet(p_rect));
	if (MCGPathIsValid(t_path))
		MCCanvasPathMakeWithMCGPath(t_path, r_path);

	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithEllipse(MCCanvasPointRef p_center, MCCanvasFloat p_radius_x, MCCanvasFloat p_radius_y, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
		return;
	
	MCGPathAddEllipse(t_path, *MCCanvasPointGet(p_center), MCGSizeMake(p_radius_x, p_radius_y), 0);
	if (MCGPathIsValid(t_path))
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithEllipseWithRadiiAsList(MCCanvasPointRef p_center, MCProperListRef p_radii, MCCanvasPathRef &r_path)
{
	MCGPoint t_radii;
	if (!MCProperListToRadii(p_radii, t_radii))
		return;
	
	MCCanvasPathMakeWithEllipse(p_center, t_radii.x, t_radii.y, r_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithCircle(MCCanvasPointRef p_center, MCCanvasFloat p_radius, MCCanvasPathRef &r_path)
{
	MCCanvasPathMakeWithEllipse(p_center, p_radius, p_radius, r_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithLine(MCCanvasPointRef p_start, MCCanvasPointRef p_end, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
		return;
	
	MCGPathAddLine(t_path, *MCCanvasPointGet(p_start), *MCCanvasPointGet(p_end));
	if (MCGPathIsValid(t_path))
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
}

bool MCCanvasPointsListToMCGPoints(MCProperListRef p_points, MCGPoint *&r_points)
{
	bool t_success;
	t_success = true;
	
	MCGPoint *t_points;
	t_points = nil;
	
	uint32_t t_point_count;
	t_point_count = MCProperListGetLength(p_points);
	
	if (t_success)
		t_success = MCMemoryNewArray(t_point_count, t_points);
	
	for (uint32_t i = 0; t_success && i < t_point_count; i++)
	{
		MCValueRef t_value;
		t_value = MCProperListFetchElementAtIndex(p_points, i);
		
		if (MCValueGetTypeInfo(t_value) == kMCCanvasPointTypeInfo)
		{
			MCCanvasPointGetMCGPoint((MCCanvasPointRef)t_value, t_points[i]);
		}
		else
		{
			MCCanvasThrowError(kMCCanvasPathPointListFormatErrorTypeInfo);
			t_success = false;
		}
	}
	
	if (t_success)
		r_points = t_points;
	else
		MCMemoryDeleteArray(t_points);
	
	return t_success;
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithPoints(bool p_close, MCProperListRef p_points, MCCanvasPathRef &r_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
		t_success = MCGPathCreateMutable(t_path);
	
	MCGPoint *t_points;
	t_points = nil;
	
	if (t_success)
		t_success = MCCanvasPointsListToMCGPoints(p_points, t_points);
	
	if (t_success)
	{
		if (p_close)
			MCGPathAddPolygon(t_path, t_points, MCProperListGetLength(p_points));
		else
			MCGPathAddPolyline(t_path, t_points, MCProperListGetLength(p_points));
		
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
	MCMemoryDeleteArray(t_points);
}

static void MCCanvasPathMakeWithArcWithRadii(const MCGPoint &p_center, MCGFloat p_radius_x, MCGFloat p_radius_y, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
		return;
	
	MCGPathAddArc(t_path, p_center, MCGSizeMake(p_radius_x, p_radius_y), 0, p_start_angle, p_end_angle);
	if (MCGPathIsValid(t_path))
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithArcWithRadius(MCCanvasPointRef p_center, MCCanvasFloat p_radius, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path)
{
	MCCanvasPathMakeWithArcWithRadii(*MCCanvasPointGet(p_center), p_radius, p_radius, p_start_angle, p_end_angle, r_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithArcWithRadiiAsList(MCCanvasPointRef p_center, MCProperListRef p_radii, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path)
{
	MCGPoint t_radii;
	if (!MCProperListToRadii(p_radii, t_radii))
		return;
	
	MCCanvasPathMakeWithArcWithRadii(*MCCanvasPointGet(p_center), t_radii.x, t_radii.y, p_start_angle, p_end_angle, r_path);
}

static void MCCanvasPathMakeWithSectorWithRadii(const MCGPoint &p_center, MCGFloat p_radius_x, MCGFloat p_radius_y, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
		return;
	
	MCGPathAddArc(t_path, p_center, MCGSizeMake(p_radius_x, p_radius_y), 0, p_start_angle, p_end_angle);
	MCGPathLineTo(t_path, p_center);
	MCGPathCloseSubpath(t_path);
	if (MCGPathIsValid(t_path))
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithSectorWithRadius(MCCanvasPointRef p_center, MCCanvasFloat p_radius, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path)
{
	MCCanvasPathMakeWithSectorWithRadii(*MCCanvasPointGet(p_center), p_radius, p_radius, p_start_angle, p_end_angle, r_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithSectorWithRadiiAsList(MCCanvasPointRef p_center, MCProperListRef p_radii, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path)
{
	MCGPoint t_radii;
	if (!MCProperListToRadii(p_radii, t_radii))
		return;
	
	MCCanvasPathMakeWithSectorWithRadii(*MCCanvasPointGet(p_center), t_radii.x, t_radii.y, p_start_angle, p_end_angle, r_path);
}

static void MCCanvasPathMakeWithSegmentWithRadii(const MCGPoint &p_center, MCGFloat p_radius_x, MCGFloat p_radius_y, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
		return;
	
	MCGPathAddArc(t_path, p_center, MCGSizeMake(p_radius_x, p_radius_y), 0, p_start_angle, p_end_angle);
	MCGPathCloseSubpath(t_path);
	if (MCGPathIsValid(t_path))
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithSegmentWithRadius(MCCanvasPointRef p_center, MCCanvasFloat p_radius, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path)
{
	MCCanvasPathMakeWithSegmentWithRadii(*MCCanvasPointGet(p_center), p_radius, p_radius, p_start_angle, p_end_angle, r_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMakeWithSegmentWithRadiiAsList(MCCanvasPointRef p_center, MCProperListRef p_radii, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path)
{
	MCGPoint t_radii;
	if (!MCProperListToRadii(p_radii, t_radii))
		return;
	
	MCCanvasPathMakeWithSegmentWithRadii(*MCCanvasPointGet(p_center), t_radii.x, t_radii.y, p_start_angle, p_end_angle, r_path);
}

// Properties

void MCCanvasPathSetMCGPath(MCGPathRef p_path, MCCanvasPathRef &x_path)
{
	MCCanvasPathRef t_path;
	if (!MCCanvasPathCreateWithMCGPath(p_path, t_path))
		return;
	MCValueAssign(x_path, t_path);
	MCValueRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathGetSubpaths(integer_t p_start, integer_t p_end, MCCanvasPathRef p_path, MCCanvasPathRef &r_subpaths)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathMutableCopySubpaths(*MCCanvasPathGet(p_path), p_start, p_end, t_path))
		return;
	
	MCCanvasPathMakeWithMCGPath(t_path, r_subpaths);
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathGetSubpath(integer_t p_index, MCCanvasPathRef p_path, MCCanvasPathRef &r_subpath)
{
	MCCanvasPathGetSubpaths(p_index, p_index, p_path, r_subpath);
}

MC_DLLEXPORT_DEF
void MCCanvasPathGetBoundingBox(MCCanvasPathRef p_path, MCCanvasRectangleRef &r_bounds)
{
	MCGRectangle t_rect;
	MCGPathGetBoundingBox(*MCCanvasPathGet(p_path), t_rect);
	
	/* UNCHECKED */ MCCanvasRectangleCreateWithMCGRectangle(t_rect, r_bounds);
}

MC_DLLEXPORT_DEF
void MCCanvasPathGetInstructionsAsString(MCCanvasPathRef p_path, MCStringRef &r_instruction_string)
{
	MCAutoStringRef t_instruction_string;
	if (MCGPathGetSVGData(MCCanvasPathGetMCGPath(p_path), &t_instruction_string))
		r_instruction_string = MCValueRetain(*t_instruction_string);
}

// Operations

void MCCanvasPathTransform(MCCanvasPathRef &x_path, const MCGAffineTransform &p_transform)
{
	// Path transformations are applied immediately
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;

	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		t_success = MCGPathTransform(t_path, p_transform);
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathTransform(MCCanvasPathRef &x_path, MCCanvasTransformRef p_transform)
{
	MCCanvasPathTransform(x_path, *MCCanvasTransformGet(p_transform));
}

MC_DLLEXPORT_DEF
void MCCanvasPathScale(MCCanvasPathRef &x_path, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

MC_DLLEXPORT_DEF
void MCCanvasPathScaleWithList(MCCanvasPathRef &x_path, MCProperListRef p_scale)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasPathScale(x_path, t_scale.x, t_scale.y);
}

MC_DLLEXPORT_DEF
void MCCanvasPathRotate(MCCanvasPathRef &x_path, MCCanvasFloat p_angle)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_angle)));
}

MC_DLLEXPORT_DEF
void MCCanvasPathTranslate(MCCanvasPathRef &x_path, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeTranslation(p_x, p_y));
}

MC_DLLEXPORT_DEF
void MCCanvasPathTranslateWithList(MCCanvasPathRef &x_path, MCProperListRef p_translation)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasPathTranslate(x_path, t_translation.x, t_translation.y);
}

MC_DLLEXPORT_DEF
void MCCanvasPathAddPath(MCCanvasPathRef p_source, MCCanvasPathRef &x_dest)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_dest), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
	{
		MCGPathAddPath(t_path, *MCCanvasPathGet(p_source));
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_dest);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathMoveTo(MCCanvasPointRef p_point, MCCanvasPathRef &x_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
	{
		MCGPathMoveTo(t_path, *MCCanvasPointGet(p_point));
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathLineTo(MCCanvasPointRef p_point, MCCanvasPathRef &x_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
	{
		MCGPathLineTo(t_path, *MCCanvasPointGet(p_point));
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathCurveThroughPoint(MCCanvasPointRef p_through, MCCanvasPointRef p_to, MCCanvasPathRef &x_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
	{
		MCGPathQuadraticTo(t_path, *MCCanvasPointGet(p_through), *MCCanvasPointGet(p_to));
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathCurveThroughPoints(MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to, MCCanvasPathRef &x_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
	{
		MCGPathCubicTo(t_path, *MCCanvasPointGet(p_through_a), *MCCanvasPointGet(p_through_b), *MCCanvasPointGet(p_to));
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathClosePath(MCCanvasPathRef &x_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
	{
		MCGPathCloseSubpath(t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathArcTo(MCCanvasPointRef p_tangent, MCCanvasPointRef p_to, MCCanvasFloat p_radius, MCCanvasPathRef &x_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
	{
		MCGPathArcToTangent(t_path, *MCCanvasPointGet(p_tangent), *MCCanvasPointGet(p_to), p_radius);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathEllipticArcToWithFlagsWithRadiiAsList(MCCanvasPointRef p_to, MCProperListRef p_radii, MCCanvasFloat p_rotation, bool p_largest, bool p_clockwise, MCCanvasPathRef &x_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	MCGPoint t_radii;
	if (t_success)
		t_success = MCProperListToRadii(p_radii, t_radii);
	
	if (t_success)
	{
		MCGPathArcTo(t_path, MCGSizeMake(t_radii.x, t_radii.y), p_rotation, p_largest, p_clockwise, *MCCanvasPointGet(p_to));
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

MC_DLLEXPORT_DEF
void MCCanvasPathEllipticArcToWithRadiiAsList(MCCanvasPointRef p_to, MCProperListRef p_radii, MCCanvasFloat p_rotation, MCCanvasPathRef &x_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	MCGPoint t_radii;
	if (t_success)
		t_success = MCProperListToRadii(p_radii, t_radii);
	
	if (t_success)
	{
		MCGPathArcToBestFit(t_path, MCGSizeMake(t_radii.x, t_radii.y), p_rotation, *MCCanvasPointGet(p_to));
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}



////////////////////////////////////////////////////////////////////////////////

// Effect

bool MCCanvasEffectHasSizeAndSpread(MCCanvasEffectType p_type)
{
	return p_type == kMCCanvasEffectTypeInnerShadow || p_type == kMCCanvasEffectTypeOuterShadow || p_type == kMCCanvasEffectTypeInnerGlow || p_type == kMCCanvasEffectTypeOuterGlow;
}

bool MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectType p_type)
{
	return p_type == kMCCanvasEffectTypeInnerShadow || p_type == kMCCanvasEffectTypeOuterShadow;
}

bool MCCanvasEffectHasKnockOut(MCCanvasEffectType p_type)
{
	return p_type == kMCCanvasEffectTypeOuterShadow;
}

bool MCCanvasEffectHasSource(MCCanvasEffectType p_type)
{
	return p_type == kMCCanvasEffectTypeInnerGlow;
}

//////////

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
	
	if (!MCValueIsEqualTo(t_left->color, t_right->color) || t_left->blend_mode != t_right->blend_mode)
		return false;
	
	if (MCCanvasEffectHasSizeAndSpread(t_left->type) &&
		(t_left->size != t_right->size || t_left->spread != t_right->spread))
		return false;

	if (MCCanvasEffectHasDistanceAndAngle(t_left->type) &&
		(t_left->distance != t_right->distance || t_left->angle != t_right->angle))
		return false;
	
	if (MCCanvasEffectHasKnockOut(t_left->type) &&
		(t_left->knockout != t_right->knockout))
		return false;
	
	if (MCCanvasEffectHasSource(t_left->type) &&
		(t_left->source != t_right->source))
		return false;
	
	return true;
}

static hash_t __MCCanvasEffectHash(MCValueRef p_value)
{
	__MCCanvasEffectImpl *t_effect;
	t_effect = MCCanvasEffectGet((MCCanvasEffectRef)p_value);
	
	hash_t t_hash;
	t_hash = MCHashInteger(t_effect->type) ^ MCValueHash(t_effect->color) ^ MCHashInteger(t_effect->blend_mode);
	
	if (MCCanvasEffectHasSizeAndSpread(t_effect->type))
		t_hash ^= MCHashDouble(t_effect->size) ^ MCHashDouble(t_effect->spread);

	if (MCCanvasEffectHasDistanceAndAngle(t_effect->type))
		t_hash ^= MCHashDouble(t_effect->distance) ^ MCHashDouble(t_effect->angle);

	if (MCCanvasEffectHasKnockOut(t_effect->type))
		t_hash ^= MCHashInteger(t_effect->knockout);

	if (MCCanvasEffectHasSource(t_effect->type))
		t_hash ^= MCHashInteger(t_effect->source);
	
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

//////////

MC_DLLEXPORT_DEF
void MCCanvasEffectEvaluateType(integer_t p_type, integer_t& r_type)
{
	r_type = p_type;
}

// Constructors

void MCCanvasEffectDefault(MCCanvasEffectType p_type, __MCCanvasEffectImpl &x_effect)
{
	x_effect.color = kMCCanvasColorBlack;
	x_effect.blend_mode = kMCGBlendModeSourceOver;
	
	if (MCCanvasEffectHasSizeAndSpread(p_type))
	{
		x_effect.size = 5;
		x_effect.spread = 0;
	}
	
	if (MCCanvasEffectHasDistanceAndAngle(p_type))
	{
		x_effect.distance = 5;
		x_effect.angle = 60;
	}
	
	if (MCCanvasEffectHasKnockOut(p_type))
	{
		x_effect.knockout = true;
	}
	
	if (MCCanvasEffectHasSource(p_type))
	{
		x_effect.source = kMCCanvasEffectSourceEdge;
	}
}

MC_DLLEXPORT_DEF
void MCCanvasEffectMake(integer_t p_type, MCCanvasEffectRef &r_effect)
{
	__MCCanvasEffectImpl t_effect;
	t_effect.type = (MCCanvasEffectType)p_type;
	
	// Set default values for the effect type
	MCCanvasEffectDefault(t_effect.type, t_effect);
	
	MCCanvasEffectCreate(t_effect, r_effect);
}

//////////

bool MCCanvasEffectThrowPropertyInvalidValueError(MCCanvasEffectProperty p_property, MCValueRef p_value)
{
	MCStringRef t_prop_name;
	if (!MCCanvasEffectPropertyToString(p_property, t_prop_name))
	{
		return false;
	}
	
	return MCErrorCreateAndThrow(kMCCanvasEffectPropertyInvalidValueErrorTypeInfo,
								 "property", t_prop_name,
								 "value", p_value,
								 nil);
}

bool MCCanvasEffectThrowPropertyNotAvailableError(MCCanvasEffectProperty p_property, MCCanvasEffectType p_type)
{
	MCStringRef t_prop_name;
	if (!MCCanvasEffectPropertyToString(p_property, t_prop_name))
	{
		return false;
	}
	
	MCStringRef t_type_name;
	if (!MCCanvasEffectTypeToString(p_type, t_type_name))
	{
		return false;
	}
	
	return MCErrorCreateAndThrow(kMCCanvasEffectPropertyNotAvailableErrorTypeInfo,
								 "property", t_prop_name,
								 "type", t_type_name,
								 nil);
}

bool MCCanvasEffectSetBlendModeProperty(__MCCanvasEffectImpl &x_effect, MCStringRef p_blend_mode)
{
	MCGBlendMode t_mode;
	if (!MCCanvasBlendModeFromString(p_blend_mode, t_mode))
		return MCCanvasEffectThrowPropertyInvalidValueError(kMCCanvasEffectPropertyBlendMode, p_blend_mode);
	
	x_effect.blend_mode = t_mode;
	
	return true;
}

bool MCCanvasEffectSetSizeProperty(__MCCanvasEffectImpl &x_effect, MCCanvasFloat p_size)
{
	if (!MCCanvasEffectHasSizeAndSpread(x_effect.type))
		return MCCanvasEffectThrowPropertyNotAvailableError(kMCCanvasEffectPropertySize, x_effect.type);
	
	x_effect.size = p_size;
	
	return true;
}

bool MCCanvasEffectSetSpreadProperty(__MCCanvasEffectImpl &x_effect, MCCanvasFloat p_spread)
{
	if (!MCCanvasEffectHasSizeAndSpread(x_effect.type))
		return MCCanvasEffectThrowPropertyNotAvailableError(kMCCanvasEffectPropertySpread, x_effect.type);
	
	x_effect.spread = p_spread;
	
	return true;
}

bool MCCanvasEffectSetDistanceProperty(__MCCanvasEffectImpl &x_effect, MCCanvasFloat p_distance)
{
	if (!MCCanvasEffectHasDistanceAndAngle(x_effect.type))
		return MCCanvasEffectThrowPropertyNotAvailableError(kMCCanvasEffectPropertyDistance, x_effect.type);
	
	x_effect.distance = p_distance;
	
	return true;
}

bool MCCanvasEffectSetAngleProperty(__MCCanvasEffectImpl &x_effect, MCCanvasFloat p_angle)
{
	if (!MCCanvasEffectHasDistanceAndAngle(x_effect.type))
		return MCCanvasEffectThrowPropertyNotAvailableError(kMCCanvasEffectPropertyAngle, x_effect.type);
	
	x_effect.angle = p_angle;
	
	return true;
}

bool MCCanvasEffectSetKnockOutProperty(__MCCanvasEffectImpl &x_effect, bool p_knockout)
{
	if (!MCCanvasEffectHasKnockOut(x_effect.type))
		return MCCanvasEffectThrowPropertyNotAvailableError(kMCCanvasEffectPropertyKnockOut, x_effect.type);
	
	x_effect.knockout = p_knockout;
	
	return true;
}

bool MCCanvasEffectSetSourceProperty(__MCCanvasEffectImpl &x_effect, MCStringRef p_source)
{
	if (!MCCanvasEffectHasSource(x_effect.type))
		return MCCanvasEffectThrowPropertyNotAvailableError(kMCCanvasEffectPropertySource, x_effect.type);
	
	MCCanvasEffectSource t_source;
	if (!MCCanvasEffectSourceFromString(p_source, t_source))
		return MCCanvasEffectThrowPropertyInvalidValueError(kMCCanvasEffectPropertySource, p_source);
	
	x_effect.source = t_source;
	
	return true;
}

MC_DLLEXPORT_DEF
void MCCanvasEffectMakeWithPropertyArray(integer_t p_type, MCArrayRef p_properties, MCCanvasEffectRef &r_effect)
{
	bool t_success;
	t_success = true;
	
	__MCCanvasEffectImpl t_effect;
	t_effect.type = (MCCanvasEffectType)p_type;
	
	// Set default values for the effect type
	MCCanvasEffectDefault(t_effect.type, t_effect);
	
	uintptr_t t_iter;
	t_iter = 0;
	
	MCNameRef t_key;
	MCValueRef t_value;
	
	// Set effect properties from the array
	while (t_success && MCArrayIterate(p_properties, t_iter, t_key, t_value))
	{
		MCCanvasEffectProperty t_property;
		if (!MCCanvasEffectPropertyFromString(MCNameGetString(t_key), t_property))
		{
			t_success = MCErrorCreateAndThrow(kMCCanvasEffectInvalidPropertyErrorTypeInfo,
											  "property", t_key,
											  nil);
			break;
		}
		
		switch (t_property)
		{
			case kMCCanvasEffectPropertyColor:
				if (MCValueGetTypeInfo(t_value) != kMCCanvasColorTypeInfo)
					t_success = MCCanvasEffectThrowPropertyInvalidValueError(t_property, t_value);
				else
					t_effect.color = (MCCanvasColorRef)t_value;
				break;
				
			case kMCCanvasEffectPropertyBlendMode:
				if (MCValueGetTypeCode(t_value) != kMCValueTypeCodeString)
					t_success = MCCanvasEffectThrowPropertyInvalidValueError(t_property, t_value);
				else
					t_success = MCCanvasEffectSetBlendModeProperty(t_effect, (MCStringRef)t_value);
				break;
				
			case kMCCanvasEffectPropertySize:
				if (MCValueGetTypeCode(t_value) != kMCValueTypeCodeNumber)
					t_success = MCCanvasEffectThrowPropertyInvalidValueError(t_property, t_value);
				else
					t_success = MCCanvasEffectSetSizeProperty(t_effect, MCNumberFetchAsReal((MCNumberRef)t_value));
				break;
				
			case kMCCanvasEffectPropertySpread:
				if (MCValueGetTypeCode(t_value) != kMCValueTypeCodeNumber)
					t_success = MCCanvasEffectThrowPropertyInvalidValueError(t_property, t_value);
				else
					t_success = MCCanvasEffectSetSpreadProperty(t_effect, MCNumberFetchAsReal((MCNumberRef)t_value));
				break;
				
			case kMCCanvasEffectPropertyDistance:
				if (MCValueGetTypeCode(t_value) != kMCValueTypeCodeNumber)
					t_success = MCCanvasEffectThrowPropertyInvalidValueError(t_property, t_value);
				else
					t_success = MCCanvasEffectSetDistanceProperty(t_effect, MCNumberFetchAsReal((MCNumberRef)t_value));
				break;
				
			case kMCCanvasEffectPropertyAngle:
				if (MCValueGetTypeCode(t_value) != kMCValueTypeCodeNumber)
					t_success = MCCanvasEffectThrowPropertyInvalidValueError(t_property, t_value);
				else
					t_success = MCCanvasEffectSetAngleProperty(t_effect, MCNumberFetchAsReal((MCNumberRef)t_value));
				break;
				
			case kMCCanvasEffectPropertyKnockOut:
				if (MCValueGetTypeCode(t_value) != kMCValueTypeCodeBoolean)
					t_success = MCCanvasEffectThrowPropertyInvalidValueError(t_property, t_value);
				else
					t_success = MCCanvasEffectSetKnockOutProperty(t_effect, kMCTrue == t_value);
				break;
				
			case kMCCanvasEffectPropertySource:
				if (MCValueGetTypeCode(t_value) != kMCValueTypeCodeString)
					t_success = MCCanvasEffectThrowPropertyInvalidValueError(t_property, t_value);
				else
					t_success = MCCanvasEffectSetSourceProperty(t_effect, (MCStringRef)t_value);
				break;
		}
	}
	
	if (t_success)
		t_success = MCCanvasEffectCreate(t_effect, r_effect);
}

//////////

// Properties

void MCCanvasEffectSet(const __MCCanvasEffectImpl &p_effect, MCCanvasEffectRef &x_effect)
{
	MCCanvasEffectRef t_effect;
	t_effect = nil;
	
	if (!MCCanvasEffectCreate(p_effect, t_effect))
		return;
	
	MCValueAssign(x_effect, t_effect);
	MCValueRelease(t_effect);
}

MC_DLLEXPORT_DEF
void MCCanvasEffectGetTypeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_type)
{
	/* UNCHECKED */ MCCanvasEffectTypeToString(MCCanvasEffectGet(p_effect)->type, r_type);
}

MC_DLLEXPORT_DEF
void MCCanvasEffectGetColor(MCCanvasEffectRef p_effect, MCCanvasColorRef &r_color)
{
	r_color = MCValueRetain(MCCanvasEffectGet(p_effect)->color);
}

MC_DLLEXPORT_DEF
void MCCanvasEffectSetColor(MCCanvasColorRef p_color, MCCanvasEffectRef &x_effect)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	t_effect.color = p_color;
	MCCanvasEffectSet(t_effect, x_effect);
}

MC_DLLEXPORT_DEF
void MCCanvasEffectGetBlendModeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_blend_mode)
{
	/* UNCHECKED */ MCCanvasBlendModeToString(MCCanvasEffectGet(p_effect)->blend_mode, r_blend_mode);
}

MC_DLLEXPORT_DEF
void MCCanvasEffectSetBlendModeAsString(MCStringRef p_blend_mode, MCCanvasEffectRef &x_effect)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	
	if (!MCCanvasEffectSetBlendModeProperty(t_effect, p_blend_mode))
		return;

	MCCanvasEffectSet(t_effect, x_effect);
}

MC_DLLEXPORT_DEF
void MCCanvasEffectGetSize(MCCanvasEffectRef p_effect, MCCanvasFloat &r_size)
{
	if (!MCCanvasEffectHasSizeAndSpread(MCCanvasEffectGet(p_effect)->type))
	{
		MCCanvasEffectThrowPropertyNotAvailableError(kMCCanvasEffectPropertySize, MCCanvasEffectGet(p_effect)->type);
		return;
	}
	
	r_size = MCCanvasEffectGet(p_effect)->size;
}

MC_DLLEXPORT_DEF
void MCCanvasEffectSetSize(MCCanvasFloat p_size, MCCanvasEffectRef &x_effect)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	
	if (!MCCanvasEffectSetSizeProperty(t_effect, p_size))
		return;

	MCCanvasEffectSet(t_effect, x_effect);
}

MC_DLLEXPORT_DEF
void MCCanvasEffectGetSpread(MCCanvasEffectRef p_effect, MCCanvasFloat &r_spread)
{
	if (!MCCanvasEffectHasSizeAndSpread(MCCanvasEffectGet(p_effect)->type))
	{
		MCCanvasEffectThrowPropertyNotAvailableError(kMCCanvasEffectPropertySpread, MCCanvasEffectGet(p_effect)->type);
		return;
	}
	
	r_spread = MCCanvasEffectGet(p_effect)->spread;
}

MC_DLLEXPORT_DEF
void MCCanvasEffectSetSpread(MCCanvasFloat p_spread, MCCanvasEffectRef &x_effect)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);

	if (!MCCanvasEffectSetSpreadProperty(t_effect, p_spread))
		return;

	MCCanvasEffectSet(t_effect, x_effect);
}

MC_DLLEXPORT_DEF
void MCCanvasEffectGetDistance(MCCanvasEffectRef p_effect, MCCanvasFloat &r_distance)
{
	if (!MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectGet(p_effect)->type))
	{
		MCCanvasEffectThrowPropertyNotAvailableError(kMCCanvasEffectPropertyDistance, MCCanvasEffectGet(p_effect)->type);
		return;
	}
	
	r_distance = MCCanvasEffectGet(p_effect)->distance;
}

MC_DLLEXPORT_DEF
void MCCanvasEffectSetDistance(MCCanvasFloat p_distance, MCCanvasEffectRef &x_effect)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	
	if (!MCCanvasEffectSetDistanceProperty(t_effect, p_distance))
		return;

	MCCanvasEffectSet(t_effect, x_effect);
}

MC_DLLEXPORT_DEF
void MCCanvasEffectGetAngle(MCCanvasEffectRef p_effect, MCCanvasFloat &r_angle)
{
	if (!MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectGet(p_effect)->type))
	{
		MCCanvasEffectThrowPropertyNotAvailableError(kMCCanvasEffectPropertyAngle, MCCanvasEffectGet(p_effect)->type);
		return;
	}
	
	r_angle = MCCanvasEffectGet(p_effect)->angle;
}

MC_DLLEXPORT_DEF
void MCCanvasEffectSetAngle(MCCanvasFloat p_angle, MCCanvasEffectRef &x_effect)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	
	if (!MCCanvasEffectSetAngleProperty(t_effect, p_angle))
		return;
	
	MCCanvasEffectSet(t_effect, x_effect);
}

MC_DLLEXPORT_DEF
void MCCanvasEffectGetKnockOut(MCCanvasEffectRef p_effect, bool &r_knockout)
{
	if (!MCCanvasEffectHasKnockOut(MCCanvasEffectGet(p_effect)->type))
	{
		MCCanvasEffectThrowPropertyNotAvailableError(kMCCanvasEffectPropertyKnockOut, MCCanvasEffectGet(p_effect)->type);
		return;
	}
	
	r_knockout = MCCanvasEffectGet(p_effect)->knockout;
}

MC_DLLEXPORT_DEF
void MCCanvasEffectSetKnockOut(bool p_knockout, MCCanvasEffectRef &x_effect)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	
	if (!MCCanvasEffectSetKnockOutProperty(t_effect, p_knockout))
		return;
	
	MCCanvasEffectSet(t_effect, x_effect);
}

MC_DLLEXPORT_DEF
void MCCanvasEffectGetSourceAsString(MCCanvasEffectRef p_effect, MCStringRef &r_source)
{
	if (!MCCanvasEffectHasSource(MCCanvasEffectGet(p_effect)->type))
	{
		MCCanvasEffectThrowPropertyNotAvailableError(kMCCanvasEffectPropertySource, MCCanvasEffectGet(p_effect)->type);
		return;
	}
	
	/* UNCHECKED */
	MCCanvasEffectSourceToString(MCCanvasEffectGet(p_effect)->source, r_source);
}

MC_DLLEXPORT_DEF
void MCCanvasEffectSetSourceAsString(MCStringRef p_source, MCCanvasEffectRef &x_effect)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	
	if (!MCCanvasEffectSetSourceProperty(t_effect, p_source))
		return;
	
	MCCanvasEffectSet(t_effect, x_effect);
}

////////////////////////////////////////////////////////////////////////////////

// Font

static void __MCCanvasFontDestroy(MCValueRef p_value)
{
	MCFontRelease(MCCanvasFontGet((MCCanvasFontRef)p_value)->font);
}

static bool __MCCanvasFontCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasFontEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	// TODO - compare fonts
	return false;
}

static hash_t __MCCanvasFontHash(MCValueRef p_value)
{
	// TODO - compute font hash
	return MCHashPointer(MCCanvasFontGetMCFont((MCCanvasFontRef)p_value));
}

static bool __MCCanvasFontDescribe(MCValueRef p_value, MCStringRef &r_string)
{
	return false;
}

bool MCCanvasFontCreateWithMCFont(MCFontRef p_font, MCCanvasFontRef &r_font)
{
	bool t_success;
	t_success = true;
	
	MCCanvasFontRef t_font;
	t_font = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasFontTypeInfo, sizeof(__MCCanvasFontImpl), t_font);
	
	if (t_success)
	{
		__MCCanvasFontImpl *t_impl;
		t_impl = MCCanvasFontGet(t_font);
		
		t_impl->font = MCFontRetain(p_font);
		
		t_success = MCValueInter(t_font, r_font);
	}
	
	MCValueRelease(t_font);
	
	return t_success;
}

bool MCCanvasFontCreate(MCStringRef p_name, MCFontStyle p_style, int32_t p_size, MCCanvasFontRef &r_font)
{
	MCNewAutoNameRef t_name;
	if (!MCNameCreate(p_name, &t_name))
		return false;
	
	MCFontRef t_font;
	if (!MCFontCreate(*t_name, p_style, p_size, t_font))
	{
		// TODO - throw font creation error
		return false;
	}
	
	bool t_success;
	t_success = MCCanvasFontCreateWithMCFont(t_font, r_font);
	
	MCFontRelease(t_font);
	
	return t_success;
}

__MCCanvasFontImpl *MCCanvasFontGet(MCCanvasFontRef p_font)
{
	return (__MCCanvasFontImpl*)MCValueGetExtraBytesPtr(p_font);
}

MCFontRef MCCanvasFontGetMCFont(MCCanvasFontRef p_font)
{
	return MCCanvasFontGet(p_font)->font;
}

static inline MCFontStyle MCFontStyleMake(bool p_bold, bool p_italic)
{
	return (p_bold ? kMCFontStyleBold : 0) | (p_italic ? kMCFontStyleItalic : 0);
}

static inline bool MCFontStyleIsBold(MCFontStyle p_style)
{
	return p_style & kMCFontStyleBold;
}

static inline bool MCFontStyleIsItalic(MCFontStyle p_style)
{
	return p_style & kMCFontStyleItalic;
}

static inline MCFontStyle MCFontStyleSetFlag(MCFontStyle p_style, MCFontStyle p_flag, bool p_set)
{
	return (p_style & ~p_flag) | (p_set ? p_flag : 0);
}

//////////

bool MCCanvasFontGetDefault(MCCanvasFontRef &r_font)
{
	if (kMCCanvasFont12PtHelvetica == nil)
	{
		if (!MCCanvasFontCreate(MCSTR(DEFAULT_TEXT_FONT), 0, 12, kMCCanvasFont12PtHelvetica))
			return false;
	}
	
	r_font = MCValueRetain(kMCCanvasFont12PtHelvetica);
	
	return true;
}

// Constructors
MC_DLLEXPORT_DEF
void MCCanvasFontMakeWithSize(MCStringRef p_name, bool p_bold, bool p_italic, integer_t p_size, MCCanvasFontRef &r_font)
{
	/* UNCHECKED */ MCCanvasFontCreate(p_name, MCFontStyleMake(p_bold, p_italic), p_size, r_font);
}

MC_DLLEXPORT_DEF
void MCCanvasFontMakeWithStyle(MCStringRef p_name, bool p_bold, bool p_italic, MCCanvasFontRef &r_font)
{
	// TODO - confirm default font size - make configurable?
	/* UNCHECKED */ MCCanvasFontCreate(p_name, MCFontStyleMake(p_bold, p_italic), 12, r_font);
}

MC_DLLEXPORT_DEF
void MCCanvasFontMake(MCStringRef p_name, MCCanvasFontRef &r_font)
{
	MCCanvasFontMakeWithStyle(p_name, false, false, r_font);
}

// Properties

void MCCanvasFontGetProps(MCCanvasFontRef p_font, MCStringRef &r_name, MCFontStyle &r_style, int32_t &r_size)
{
	MCFontRef t_font;
	t_font = MCCanvasFontGetMCFont(p_font);
	
	r_name = MCNameGetString(MCFontGetName(t_font));
	r_style = MCFontGetStyle(t_font);
	r_size = MCFontGetSize(t_font);
}

void MCCanvasFontSetProps(MCCanvasFontRef &x_font, MCStringRef p_name, MCFontStyle p_style, int32_t p_size)
{
	MCCanvasFontRef t_font;
	if (!MCCanvasFontCreate(p_name, p_style, p_size, t_font))
		return;
	
	MCValueAssign(x_font, t_font);
	MCValueRelease(t_font);
}

MC_DLLEXPORT_DEF
void MCCanvasFontGetName(MCCanvasFontRef p_font, MCStringRef &r_name)
{
	r_name = MCValueRetain(MCNameGetString(MCFontGetName(MCCanvasFontGetMCFont(p_font))));
}

MC_DLLEXPORT_DEF
void MCCanvasFontSetName(MCStringRef p_name, MCCanvasFontRef &x_font)
{
	MCStringRef t_name;
	MCFontStyle t_style;
	int32_t t_size;
	
	MCCanvasFontGetProps(x_font, t_name, t_style, t_size);
	MCCanvasFontSetProps(x_font, p_name, t_style, t_size);
}

MC_DLLEXPORT_DEF
void MCCanvasFontGetBold(MCCanvasFontRef p_font, bool &r_bold)
{
	r_bold = MCFontStyleIsBold(MCFontGetStyle(MCCanvasFontGetMCFont(p_font)));
}

MC_DLLEXPORT_DEF
void MCCanvasFontSetBold(bool p_bold, MCCanvasFontRef &x_font)
{
	MCStringRef t_name;
	MCFontStyle t_style;
	int32_t t_size;
	
	MCCanvasFontGetProps(x_font, t_name, t_style, t_size);
	MCCanvasFontSetProps(x_font, t_name, MCFontStyleSetFlag(t_style, kMCFontStyleBold, p_bold), t_size);
}

MC_DLLEXPORT_DEF
void MCCanvasFontGetItalic(MCCanvasFontRef p_font, bool &r_italic)
{
	r_italic = MCFontStyleIsItalic(MCFontGetStyle(MCCanvasFontGetMCFont(p_font)));
}

MC_DLLEXPORT_DEF
void MCCanvasFontSetItalic(bool p_italic, MCCanvasFontRef &x_font)
{
	MCStringRef t_name;
	MCFontStyle t_style;
	int32_t t_size;
	
	MCCanvasFontGetProps(x_font, t_name, t_style, t_size);
	MCCanvasFontSetProps(x_font, t_name, MCFontStyleSetFlag(t_style, kMCFontStyleItalic, p_italic), t_size);
}

MC_DLLEXPORT_DEF
void MCCanvasFontGetSize(MCCanvasFontRef p_font, uinteger_t &r_size)
{
	r_size = MCFontGetSize(MCCanvasFontGetMCFont(p_font));
}

MC_DLLEXPORT_DEF
void MCCanvasFontSetSize(uinteger_t p_size, MCCanvasFontRef &x_font)
{
	MCStringRef t_name;
	MCFontStyle t_style;
	int32_t t_size;
	
	MCCanvasFontGetProps(x_font, t_name, t_style, t_size);
	MCCanvasFontSetProps(x_font, t_name, t_style, p_size);
}

MC_DLLEXPORT_DEF
void MCCanvasFontGetHandle(MCCanvasFontRef p_font, void*& r_handle)
{
    r_handle = MCFontGetHandle(MCCanvasFontGetMCFont(p_font));
}

// Operations

MCCanvasRectangleRef MCCanvasFontMeasureTextTypographicBoundsWithTransform(MCStringRef p_text, MCCanvasFontRef p_font, const MCGAffineTransform &p_transform)
{
	MCFontRef t_font;
	t_font = MCCanvasFontGetMCFont(p_font);
	
	MCGRectangle t_bounds;
	t_bounds.origin.x = 0;
	t_bounds.size.width = MCFontMeasureTextFloat(t_font, p_text, p_transform);
	t_bounds.origin.y = -MCFontGetAscent(t_font);
	t_bounds.size.height = MCFontGetDescent(t_font) + MCFontGetAscent(t_font);
	
	MCCanvasRectangleRef t_rect;
	if (!MCCanvasRectangleCreateWithMCGRectangle(t_bounds, t_rect))
		return nil;
	
	return t_rect;
}

MC_DLLEXPORT_DEF
void MCCanvasFontMeasureTextTypographicBounds(MCStringRef p_text, MCCanvasFontRef p_font, MCCanvasRectangleRef& r_rect)
{
	r_rect = MCCanvasFontMeasureTextTypographicBoundsWithTransform(p_text, p_font, MCGAffineTransformMakeIdentity());
}

MC_DLLEXPORT_DEF
void MCCanvasFontMeasureTextTypographicBoundsOnCanvas(MCStringRef p_text, MCCanvasRef p_canvas, MCCanvasRectangleRef& r_rect)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	r_rect = MCCanvasFontMeasureTextTypographicBoundsWithTransform(p_text, t_canvas->props().font, MCGContextGetDeviceTransform(t_canvas->context));
}

MCCanvasRectangleRef MCCanvasFontMeasureTextImageBoundsWithTransform(MCStringRef p_text, MCCanvasFontRef p_font, const MCGAffineTransform &p_transform)
{
	MCFontRef t_font;
	t_font = MCCanvasFontGetMCFont(p_font);
	
	MCGRectangle t_bounds;
	if (!MCFontMeasureTextImageBounds(t_font, p_text, p_transform, t_bounds))
	{
		// TODO - throw text measure error
		return nil;
	}
	
	MCCanvasRectangleRef t_rect;
	if (!MCCanvasRectangleCreateWithMCGRectangle(t_bounds, t_rect))
		return nil;
	
	return t_rect;
}

MC_DLLEXPORT_DEF
void MCCanvasFontMeasureTextImageBounds(MCStringRef p_text, MCCanvasFontRef p_font, MCCanvasRectangleRef& r_rect)
{
	r_rect = MCCanvasFontMeasureTextImageBoundsWithTransform(p_text, p_font, MCGAffineTransformMakeIdentity());
}

MC_DLLEXPORT_DEF
void MCCanvasFontMeasureTextImageBoundsOnCanvas(MCStringRef p_text, MCCanvasRef p_canvas, MCCanvasRectangleRef& r_rect)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	r_rect = MCCanvasFontMeasureTextImageBoundsWithTransform(p_text, t_canvas->props().font, MCGContextGetDeviceTransform(t_canvas->context));
}

////////////////////////////////////////////////////////////////////////////////

// Canvas

void MCCanvasDirtyProperties(__MCCanvasImpl &p_canvas)
{
	p_canvas.antialias_changed = p_canvas.blend_mode_changed = p_canvas.fill_rule_changed = p_canvas.opacity_changed = p_canvas.paint_changed = true;
	p_canvas.stroke_width_changed = p_canvas.join_style_changed = p_canvas.cap_style_changed = p_canvas.miter_limit_changed = p_canvas.dashes_changed = true;
}

bool MCCanvasPropertiesInit(MCCanvasProperties &p_properties)
{
	bool t_success;
	t_success = true;
	
	MCCanvasSolidPaintRef t_black_paint;
	t_black_paint = nil;
	
	MCCanvasFontRef t_default_font;
	t_default_font = nil;
	
	if (t_success)
		t_success = MCCanvasFontGetDefault(t_default_font);
	
	if (t_success)
		t_success = MCCanvasSolidPaintCreateWithColor(kMCCanvasColorBlack, t_black_paint);
	
	if (t_success)
	{
		p_properties.antialias = true;
		p_properties.blend_mode = kMCGBlendModeSourceOver;
		p_properties.fill_rule = kMCGFillRuleNonZero;
		p_properties.opacity = 1.0;
		p_properties.paint = nil;
		p_properties.stippled = false;
		p_properties.image_filter = kMCGImageFilterMedium;
		p_properties.font = t_default_font;
		
		p_properties.stroke_width = 0;
		p_properties.join_style = kMCGJoinStyleBevel;
		p_properties.cap_style = kMCGCapStyleButt;
		p_properties.miter_limit = 0;
		p_properties.dash_lengths = MCValueRetain(kMCEmptyProperList);
		p_properties.dash_phase = 0;

		// TODO - check this cast to supertype?
		p_properties.paint = (MCCanvasPaintRef)t_black_paint;
	}
	else
	{
		// TODO - throw error
		MCValueRelease(t_black_paint);
		MCValueRelease(t_default_font);
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
	t_properties.font = MCValueRetain(p_src.font);
	
	t_properties.stroke_width = p_src.stroke_width;
	t_properties.join_style = p_src.join_style;
	t_properties.cap_style = p_src.cap_style;
	t_properties.miter_limit = p_src.miter_limit;
	t_properties.dash_lengths = MCValueRetain(p_src.dash_lengths);
	t_properties.dash_phase = p_src.dash_phase;
	
	p_dst = t_properties;
	
	return true;
}

void MCCanvasPropertiesClear(MCCanvasProperties &p_properties)
{
	MCValueRelease(p_properties.paint);
	MCValueRelease(p_properties.font);
	MCValueRelease(p_properties.dash_lengths);
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
	{
		// TODO - throw canvas pop error
		return false;
	}
	
	MCCanvasPropertiesClear(x_canvas.prop_stack[x_canvas.prop_index]);
	x_canvas.prop_index--;
	
	MCCanvasDirtyProperties(x_canvas);

	return true;
}

MC_DLLEXPORT_DEF
bool MCCanvasCreate(MCGContextRef p_context, MCCanvasRef &r_canvas)
{
	bool t_success;
	t_success = true;
	
	MCCanvasRef t_canvas;
	t_canvas = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasTypeInfo, sizeof(__MCCanvasImpl), t_canvas);
	
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
		t_canvas_impl->context = MCGContextRetain(p_context);
		MCCanvasDirtyProperties(*t_canvas_impl);
		
		r_canvas = t_canvas;
	}
	else
		MCValueRelease(t_canvas);
	
	return t_success;
}

__MCCanvasImpl *MCCanvasGet(MCCanvasRef p_canvas)
{
	return (__MCCanvasImpl*)MCValueGetExtraBytesPtr(p_canvas);
}

// MCCanvasRef Type Methods

static void __MCCanvasDestroy(MCValueRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(p_canvas);
	
	if (t_canvas->prop_stack != nil)
	{
		for (uint32_t i = 0; i <= t_canvas->prop_index; i++)
			MCCanvasPropertiesClear(t_canvas->prop_stack[i]);
		MCMemoryDeleteArray(t_canvas->prop_stack);
	}
	
    MCGPaintRelease(t_canvas->last_paint);
    
	MCGContextRelease(t_canvas->context);
}

static bool __MCCanvasCopy(MCValueRef p_canvas, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_canvas;
	else
		r_copy = MCValueRetain(p_canvas);
	
	return true;
}

static bool __MCCanvasEqual(MCValueRef p_left, MCValueRef p_right)
{
	return p_left == p_right;
}

static hash_t __MCCanvasHash(MCValueRef p_canvas)
{
	return MCHashPointer (p_canvas);
}

static bool __MCCanvasDescribe(MCValueRef p_canvas, MCStringRef &r_description)
{
	// TODO - provide canvas description?
	return false;
}

// Properties

static inline MCCanvasProperties &MCCanvasGetProps(MCCanvasRef p_canvas)
{
	return MCCanvasGet(p_canvas)->props();
}

MC_DLLEXPORT_DEF
void MCCanvasGetPaint(MCCanvasRef p_canvas, MCCanvasPaintRef &r_paint)
{
	r_paint = MCValueRetain(MCCanvasGetProps(p_canvas).paint);
}

MC_DLLEXPORT_DEF
void MCCanvasSetPaint(MCCanvasPaintRef p_paint, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	MCValueAssign(t_canvas->props().paint, p_paint);
	t_canvas->paint_changed = true;
}

MC_DLLEXPORT_DEF
void MCCanvasGetFillRuleAsString(MCCanvasRef p_canvas, MCStringRef &r_string)
{
	if (!MCCanvasFillRuleToString(MCCanvasGetProps(p_canvas).fill_rule, r_string))
	{
		// TODO - throw error
	}
}

MC_DLLEXPORT_DEF
void MCCanvasSetFillRuleAsString(MCStringRef p_string, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	if (!MCCanvasFillRuleFromString(p_string, t_canvas->props().fill_rule))
	{
		// TODO - throw error
		return;
	}
	
	t_canvas->fill_rule_changed = true;
}

MC_DLLEXPORT_DEF
void MCCanvasGetAntialias(MCCanvasRef p_canvas, bool &r_antialias)
{
	r_antialias = MCCanvasGetProps(p_canvas).antialias;
}

MC_DLLEXPORT_DEF
void MCCanvasSetAntialias(bool p_antialias, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	t_canvas->props().antialias = p_antialias;
	t_canvas->antialias_changed = true;
}

MC_DLLEXPORT_DEF
void MCCanvasGetOpacity(MCCanvasRef p_canvas, MCCanvasFloat &r_opacity)
{
	r_opacity = MCCanvasGetProps(p_canvas).opacity;
}

MC_DLLEXPORT_DEF
void MCCanvasSetOpacity(MCCanvasFloat p_opacity, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	t_canvas->props().opacity = p_opacity;
	t_canvas->opacity_changed = true;
}

MC_DLLEXPORT_DEF
void MCCanvasGetBlendModeAsString(MCCanvasRef p_canvas, MCStringRef &r_blend_mode)
{
	/* UNCHECKED */ MCCanvasBlendModeToString(MCCanvasGetProps(p_canvas).blend_mode, r_blend_mode);
}

MC_DLLEXPORT_DEF
void MCCanvasSetBlendModeAsString(MCStringRef p_blend_mode, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	/* UNCHECKED */ MCCanvasBlendModeFromString(p_blend_mode, t_canvas->props().blend_mode);
	t_canvas->blend_mode_changed = true;
}

MC_DLLEXPORT_DEF
void MCCanvasGetStippled(MCCanvasRef p_canvas, bool &r_stippled)
{
	r_stippled = MCCanvasGetProps(p_canvas).stippled;
}

MC_DLLEXPORT_DEF
void MCCanvasSetStippled(bool p_stippled, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	t_canvas->props().stippled = p_stippled;
	// Stippled property only applies to solid paint
	// TODO - make stippled a property of solid paint instead of canvas
	if (MCCanvasPaintIsSolidPaint(t_canvas->props().paint))
		t_canvas->paint_changed = true;
}

MC_DLLEXPORT_DEF
void MCCanvasGetImageResizeQualityAsString(MCCanvasRef p_canvas, MCStringRef &r_quality)
{
	/* UNCHECKED */ MCCanvasImageFilterToString(MCCanvasGetProps(p_canvas).image_filter, r_quality);
}

MC_DLLEXPORT_DEF
void MCCanvasSetImageResizeQualityAsString(MCStringRef p_quality, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	/* UNCHECKED */ MCCanvasImageFilterFromString(p_quality, t_canvas->props().image_filter);
	// need to re-apply pattern paint if quality changes
	if (MCCanvasPaintIsPattern(t_canvas->props().paint))
		t_canvas->paint_changed = true;
}

MC_DLLEXPORT_DEF
void MCCanvasGetStrokeWidth(MCCanvasRef p_canvas, MCGFloat& r_stroke_width)
{
	r_stroke_width = MCCanvasGetProps(p_canvas).stroke_width;
}

MC_DLLEXPORT_DEF
void MCCanvasSetStrokeWidth(MCGFloat p_stroke_width, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	t_canvas->props().stroke_width = p_stroke_width;
	t_canvas->stroke_width_changed = true;
}

MC_DLLEXPORT_DEF
void MCCanvasGetFont(MCCanvasRef p_canvas, MCCanvasFontRef &r_font)
{
	r_font = MCValueRetain(MCCanvasGetProps(p_canvas).font);
}

MC_DLLEXPORT_DEF
void MCCanvasSetFont(MCCanvasFontRef p_font, MCCanvasRef p_canvas)
{
	MCValueAssign(MCCanvasGetProps(p_canvas).font, p_font);
}

MC_DLLEXPORT_DEF
void MCCanvasGetJoinStyleAsString(MCCanvasRef p_canvas, MCStringRef &r_join_style)
{
	/* UNCHECKED */ MCCanvasJoinStyleToString(MCCanvasGetProps(p_canvas).join_style, r_join_style);
}

MC_DLLEXPORT_DEF
void MCCanvasSetJoinStyleAsString(MCStringRef p_join_style, MCCanvasRef p_canvas)
{
	if (!MCCanvasJoinStyleFromString(p_join_style, MCCanvasGetProps(p_canvas).join_style))
	{
		// TODO - throw join style error
		return;
	}
	
	MCCanvasGet(p_canvas)->join_style_changed = true;
}

MC_DLLEXPORT_DEF
void MCCanvasGetCapStyleAsString(MCCanvasRef p_canvas, MCStringRef &r_cap_style)
{
	/* UNCHECKED */ MCCanvasCapStyleToString(MCCanvasGetProps(p_canvas).cap_style, r_cap_style);
}

MC_DLLEXPORT_DEF
void MCCanvasSetCapStyleAsString(MCStringRef p_cap_style, MCCanvasRef p_canvas)
{
	if (!MCCanvasCapStyleFromString(p_cap_style, MCCanvasGetProps(p_canvas).cap_style))
	{
		// TODO - throw cap style error
		return;
	}
	
	MCCanvasGet(p_canvas)->cap_style_changed = true;
}

MC_DLLEXPORT_DEF
void MCCanvasGetMiterLimit(MCCanvasRef p_canvas, MCCanvasFloat &r_limit)
{
	r_limit = MCCanvasGetProps(p_canvas).miter_limit;
}

MC_DLLEXPORT_DEF
void MCCanvasSetMiterLimit(MCCanvasFloat p_limit, MCCanvasRef p_canvas)
{
	MCCanvasGetProps(p_canvas).miter_limit = p_limit;
	MCCanvasGet(p_canvas)->miter_limit_changed = true;
}

MC_DLLEXPORT_DEF
void MCCanvasGetDashes(MCCanvasRef p_canvas, MCProperListRef &r_dashes)
{
	r_dashes = MCValueRetain(MCCanvasGetProps(p_canvas).dash_lengths);
}

bool MCCanvasDashesCheckList(MCProperListRef p_list)
{
	uindex_t t_length;
	t_length = MCProperListGetLength(p_list);
	
	for (uindex_t i = 0; i < t_length; i++)
	{
		if (MCValueGetTypeInfo(MCProperListFetchElementAtIndex(p_list, i)) != kMCNumberTypeInfo)
			return false;
	}
	
	return true;
}

MC_DLLEXPORT_DEF
void MCCanvasSetDashes(MCProperListRef p_dashes, MCCanvasRef p_canvas)
{
	if (!MCCanvasDashesCheckList(p_dashes))
	{
		// TODO - throw dashes list type error
		return;
	}
	
	MCValueAssign(MCCanvasGetProps(p_canvas).dash_lengths, p_dashes);
	MCCanvasGet(p_canvas)->dashes_changed = true;
}

MC_DLLEXPORT_DEF
void MCCanvasGetDashPhase(MCCanvasRef p_canvas, MCCanvasFloat &r_phase)
{
	r_phase = MCCanvasGetProps(p_canvas).dash_phase;
}

MC_DLLEXPORT_DEF
void MCCanvasSetDashPhase(MCCanvasFloat p_phase, MCCanvasRef p_canvas)
{
	MCCanvasGetProps(p_canvas).dash_phase = p_phase;
	MCCanvasGet(p_canvas)->dashes_changed = true;
}

//////////

MC_DLLEXPORT_DEF
void MCCanvasGetClipBounds(MCCanvasRef p_canvas, MCCanvasRectangleRef &r_bounds)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGRectangle t_bounds;
	t_bounds = MCGContextGetClipBounds(t_canvas->context);
	
	/* UNCHECKED */ MCCanvasRectangleCreateWithMCGRectangle(t_bounds, r_bounds);
}

//////////

void MCCanvasCreateSolidPaint(__MCCanvasImpl &x_canvas, MCCanvasSolidPaintRef p_paint, MCGPaintRef& r_paint)
{
    __MCCanvasColorImpl *t_color;
    t_color = MCCanvasColorGet(MCCanvasSolidPaintGet(p_paint)->color);
    
    MCGPaintCreateWithSolidColor(t_color->red, t_color->green, t_color->blue, t_color->alpha, r_paint);
}

void MCCanvasCreatePatternPaint(__MCCanvasImpl &x_canvas, MCCanvasPatternRef p_pattern, MCGPaintRef& r_paint)
{
	__MCCanvasPatternImpl *t_pattern;
	t_pattern = MCCanvasPatternGet(p_pattern);
	
	MCGImageFrame t_frame;
	
	MCGAffineTransform t_pattern_transform;
	t_pattern_transform = *MCCanvasTransformGet(t_pattern->transform);
	
	MCImageRep *t_pattern_image;
	t_pattern_image = MCCanvasImageGetImageRep(t_pattern->image);
	
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
		
        MCGPaintCreateWithPattern(t_frame.image, t_transform, x_canvas.props().image_filter, r_paint);
        
		MCImageRepUnlock(t_pattern_image, 0, t_frame);
	}
}

void MCCanvasCreateGradientPaint(__MCCanvasImpl &x_canvas, MCCanvasGradientRef p_gradient, MCGPaintRef& r_paint)
{
	bool t_success;
	t_success = true;
	
	__MCCanvasGradientImpl *t_gradient;
	t_gradient = MCCanvasGradientGet(p_gradient);
	
	MCGFloat *t_offsets;
	t_offsets = nil;
	
	MCGColor *t_colors;
	t_colors = nil;
	
	MCGAffineTransform t_gradient_transform;
	t_gradient_transform = *MCCanvasTransformGet(t_gradient->transform);
	
	uint32_t t_ramp_length;
	t_ramp_length = MCProperListGetLength(t_gradient->ramp);
	
	if (t_success)
		t_success = MCMemoryNewArray(t_ramp_length, t_offsets) && MCMemoryNewArray(t_ramp_length, t_colors);
	
	if (t_success)
	{
		for (uint32_t i = 0; i < t_ramp_length; i++)
		{
			MCCanvasGradientStopRef t_stop;
			if (MCProperListFetchGradientStopAt(t_gradient->ramp, i, t_stop))
			{
					t_offsets[i] = MCCanvasGradientStopGet(t_stop)->offset;
					t_colors[i] = MCCanvasColorToMCGColor(MCCanvasGradientStopGet(t_stop)->color);
			}
		}
        
        MCGPaintCreateWithGradient(t_gradient->function, t_offsets, t_colors, t_ramp_length, t_gradient->mirror, t_gradient->wrap, t_gradient->repeats, t_gradient_transform, t_gradient->filter, r_paint);
	}

	MCMemoryDeleteArray(t_offsets);
	MCMemoryDeleteArray(t_colors);
}

void MCCanvasCreatePaint(__MCCanvasImpl &x_canvas, MCCanvasPaintRef& p_paint, MCGPaintRef& r_paint)
{
    if (MCCanvasPaintIsSolidPaint(p_paint))
        MCCanvasCreateSolidPaint(x_canvas, (MCCanvasSolidPaintRef)p_paint, r_paint);
    else if (MCCanvasPaintIsPattern(p_paint))
        MCCanvasCreatePatternPaint(x_canvas, (MCCanvasPatternRef)p_paint, r_paint);
    else if (MCCanvasPaintIsGradient(p_paint))
        MCCanvasCreateGradientPaint(x_canvas, (MCCanvasGradientRef)p_paint, r_paint);
    else
        MCAssert(false);
}

void MCCanvasApplyPaint(__MCCanvasImpl &x_canvas, MCCanvasPaintRef &p_paint)
{
    MCGPaintRelease(x_canvas.last_paint);
    
    MCCanvasCreatePaint(x_canvas, p_paint, x_canvas.last_paint);
    MCGContextSetFillPaint(x_canvas.context, x_canvas.last_paint);
    MCGContextSetStrokePaint(x_canvas.context, x_canvas.last_paint);
    
    MCGPaintStyle t_style;
    t_style = x_canvas.props().stippled ? kMCGPaintStyleStippled : kMCGPaintStyleOpaque;
    
    MCGContextSetFillPaintStyle(x_canvas.context, t_style);
    MCGContextSetStrokePaintStyle(x_canvas.context, t_style);
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
	
    if (x_canvas . stroke_width_changed)
    {
        MCGContextSetStrokeWidth(x_canvas.context, x_canvas.props().stroke_width);
        x_canvas.stroke_width_changed = false;
    }
	
	if (x_canvas.join_style_changed)
	{
		MCGContextSetStrokeJoinStyle(x_canvas.context, x_canvas.props().join_style);
		x_canvas.join_style_changed = false;
	}
	
	if (x_canvas.cap_style_changed)
	{
		MCGContextSetStrokeCapStyle(x_canvas.context, x_canvas.props().cap_style);
		x_canvas.cap_style_changed = false;
	}
	
	if (x_canvas.miter_limit_changed)
	{
		MCGContextSetStrokeMiterLimit(x_canvas.context, x_canvas.props().miter_limit);
		x_canvas.miter_limit_changed = false;
	}
	
	if (x_canvas.dashes_changed)
	{
		MCCanvasFloat *t_lengths;
		t_lengths = nil;
		uindex_t t_count;
		t_count = 0;
		
		/* UNCHECKED */ MCProperListToArrayOfFloat(x_canvas.props().dash_lengths, t_count, t_lengths);
		MCGContextSetStrokeDashes(x_canvas.context, x_canvas.props().dash_phase, t_lengths, t_count);
		x_canvas.dashes_changed = false;
		
		MCMemoryDeleteArray(t_lengths);
	}
}

//////////

void MCCanvasTransform(MCCanvasRef p_canvas, const MCGAffineTransform &p_transform)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextConcatCTM(t_canvas->context, p_transform);
	// Need to re-apply pattern paint when transform changes
	if (MCCanvasPaintIsPattern(t_canvas->props().paint))
		t_canvas->paint_changed = true;
}

MC_DLLEXPORT_DEF
void MCCanvasTransform(MCCanvasRef p_canvas, MCCanvasTransformRef p_transform)
{
	MCCanvasTransform(p_canvas, *MCCanvasTransformGet(p_transform));
}

MC_DLLEXPORT_DEF
void MCCanvasScale(MCCanvasRef p_canvas, MCCanvasFloat p_scale_x, MCCanvasFloat p_scale_y)
{
	MCCanvasTransform(p_canvas, MCGAffineTransformMakeScale(p_scale_x, p_scale_y));
}

MC_DLLEXPORT_DEF
void MCCanvasScaleWithList(MCCanvasRef p_canvas, MCProperListRef p_scale)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasScale(p_canvas, t_scale.x, t_scale.y);
}

MC_DLLEXPORT_DEF
void MCCanvasRotate(MCCanvasRef p_canvas, MCCanvasFloat p_angle)
{
	MCCanvasTransform(p_canvas, MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_angle)));
}

MC_DLLEXPORT_DEF
void MCCanvasTranslate(MCCanvasRef p_canvas, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasTransform(p_canvas, MCGAffineTransformMakeTranslation(p_x, p_y));
}

MC_DLLEXPORT_DEF
void MCCanvasTranslateWithList(MCCanvasRef p_canvas, MCProperListRef p_translation)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasTranslate(p_canvas, t_translation.x, t_translation.y);
}

//////////

MC_DLLEXPORT_DEF
void MCCanvasSaveState(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	if (!MCCanvasPropertiesPush(*t_canvas))
		return;

	MCGContextSave(t_canvas->context);
}

MC_DLLEXPORT_DEF
void MCCanvasRestoreState(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	if (!MCCanvasPropertiesPop(*t_canvas))
		return;

	MCGContextRestore(t_canvas->context);
}

MC_DLLEXPORT_DEF
void MCCanvasBeginLayer(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	if (!MCCanvasPropertiesPush(*t_canvas))
		return;

	MCGContextBegin(t_canvas->context, true);
}

static void MCPolarCoordsToCartesian(MCGFloat p_distance, MCGFloat p_angle, MCGFloat &r_x, MCGFloat &r_y)
{
	r_x = p_distance * cos(p_angle);
	r_y = p_distance * sin(p_angle);
}

void _MCCanvasBeginLayerWithEffect(MCCanvasEffectRef p_effect, MCCanvasRef p_canvas, bool p_isolated);

MC_DLLEXPORT_DEF
void MCCanvasBeginLayerWithEffect(MCCanvasEffectRef p_effect, MCCanvasRef p_canvas)
{
	_MCCanvasBeginLayerWithEffect(p_effect, p_canvas, false);
}

MC_DLLEXPORT_DEF
void MCCanvasBeginEffectOnlyLayerWithEffect(MCCanvasEffectRef p_effect, MCCanvasRef p_canvas)
{
	_MCCanvasBeginLayerWithEffect(p_effect, p_canvas, true);
}

void _MCCanvasBeginLayerWithEffect(MCCanvasEffectRef p_effect, MCCanvasRef p_canvas, bool p_isolated)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	if (!MCCanvasPropertiesPush(*t_canvas))
		return;

	MCGBitmapEffects t_effects = MCGBitmapEffects();

	__MCCanvasEffectImpl *t_effect_impl;
	t_effect_impl = MCCanvasEffectGet(p_effect);
	
	MCCanvasFloat t_spread;
	t_spread = MCClamp(t_effect_impl->spread, 0.0, 1.0);
	
	t_effects.isolated = p_isolated;
	
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
			t_effects.inner_glow.inverted = t_effect_impl->source == kMCCanvasEffectSourceEdge;
			t_effects.inner_glow.size = t_effect_impl->size;
			t_effects.inner_glow.spread = t_spread;
			break;
		}
			
		case kMCCanvasEffectTypeInnerShadow:
		{
			t_effects.has_inner_shadow = true;
			t_effects.inner_shadow.blend_mode = t_effect_impl->blend_mode;
			t_effects.inner_shadow.color = MCCanvasColorToMCGColor(t_effect_impl->color);
			t_effects.inner_shadow.size = t_effect_impl->size;
			t_effects.inner_shadow.spread = t_spread;
			MCPolarCoordsToCartesian(t_effect_impl->distance, MCCanvasAngleToRadians(t_effect_impl->angle), t_effects.inner_shadow.x_offset, t_effects.inner_shadow.y_offset);
			break;
		}
			
		case kMCCanvasEffectTypeOuterGlow:
		{
			t_effects.has_outer_glow = true;
			t_effects.outer_glow.blend_mode = t_effect_impl->blend_mode;
			t_effects.outer_glow.color = MCCanvasColorToMCGColor(t_effect_impl->color);
			t_effects.outer_glow.size = t_effect_impl->size;
			t_effects.outer_glow.spread = t_spread;
			break;
		}
			
		case kMCCanvasEffectTypeOuterShadow:
		{
			t_effects.has_drop_shadow = true;
			t_effects.drop_shadow.blend_mode = t_effect_impl->blend_mode;
			t_effects.drop_shadow.color = MCCanvasColorToMCGColor(t_effect_impl->color);
			t_effects.drop_shadow.knockout = t_effect_impl->knockout;
			t_effects.drop_shadow.size = t_effect_impl->size;
			t_effects.drop_shadow.spread = t_spread;
			MCPolarCoordsToCartesian(t_effect_impl->distance, MCCanvasAngleToRadians(t_effect_impl->angle), t_effects.drop_shadow.x_offset, t_effects.drop_shadow.y_offset);
			break;
		}
			
		default:
			MCAssert(false);
	}
	
	MCGRectangle t_rect;
	t_rect = MCGContextGetClipBounds(t_canvas->context);
	MCGContextBeginWithEffects(t_canvas->context, t_rect, t_effects);
}

MC_DLLEXPORT_DEF
void MCCanvasEndLayer(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	if (!MCCanvasPropertiesPop(*t_canvas))
		return;

	MCGContextEnd(t_canvas->context);
}

MC_DLLEXPORT_DEF
void MCCanvasFill(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	MCGContextBegin(t_canvas->context, false);
	MCGContextFill(t_canvas->context);
	MCGContextEnd(t_canvas->context);
}

MC_DLLEXPORT_DEF
void MCCanvasStroke(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	MCGContextBegin(t_canvas->context, false);
	MCGContextStroke(t_canvas->context);
	MCGContextEnd(t_canvas->context);
}

MC_DLLEXPORT_DEF
void MCCanvasClipToRect(MCCanvasRectangleRef p_rect, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextClipToRect(t_canvas->context, *MCCanvasRectangleGet(p_rect));
}

MC_DLLEXPORT_DEF
void MCCanvasClipToPath(MCCanvasPathRef p_path, MCCanvasRef p_canvas)
{
    __MCCanvasImpl *t_canvas;
    t_canvas = MCCanvasGet(p_canvas);
    
    MCGContextClipToPath(t_canvas->context, *MCCanvasPathGet(p_path));
}

MC_DLLEXPORT_DEF
void MCCanvasAddPath(MCCanvasPathRef p_path, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextAddPath(t_canvas->context, *MCCanvasPathGet(p_path));
}

MC_DLLEXPORT_DEF
void MCCanvasFillPath(MCCanvasPathRef p_path, MCCanvasRef p_canvas)
{
	MCCanvasAddPath(p_path, p_canvas);
	MCCanvasFill(p_canvas);
}

MC_DLLEXPORT_DEF
void MCCanvasStrokePath(MCCanvasPathRef p_path, MCCanvasRef p_canvas)
{
	MCCanvasAddPath(p_path, p_canvas);
	MCCanvasStroke(p_canvas);
}

void MCCanvasDrawRectOfImage(MCCanvasRef p_canvas, MCCanvasImageRef p_image, const MCGRectangle &p_src_rect, const MCGRectangle &p_dst_rect)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCImageRep *t_image;
	t_image = MCCanvasImageGetImageRep(p_image);
	
	MCCanvasApplyChanges(*t_canvas);

    MCImageRepRender(t_image, t_canvas->context, 0, p_src_rect, p_dst_rect, t_canvas->props().image_filter, t_canvas->last_paint);
}

MC_DLLEXPORT_DEF
void MCCanvasDrawRectOfImage(MCCanvasRectangleRef p_src_rect, MCCanvasImageRef p_image, MCCanvasRectangleRef p_dst_rect, MCCanvasRef p_canvas)
{
	MCCanvasDrawRectOfImage(p_canvas, p_image, *MCCanvasRectangleGet(p_src_rect), *MCCanvasRectangleGet(p_dst_rect));
}

MC_DLLEXPORT_DEF
void MCCanvasDrawImage(MCCanvasImageRef p_image, MCCanvasRectangleRef p_dst_rect, MCCanvasRef p_canvas)
{
	MCGRectangle t_src_rect;
	uint32_t t_width = 0;
	uint32_t t_height = 0;
	MCCanvasImageGetWidth(p_image, t_width);
	MCCanvasImageGetHeight(p_image, t_height);
	t_src_rect = MCGRectangleMake(0, 0, t_width, t_height);
	MCCanvasDrawRectOfImage(p_canvas, p_image, t_src_rect, *MCCanvasRectangleGet(p_dst_rect));
}

MC_DLLEXPORT_DEF
void MCCanvasMoveTo(MCCanvasPointRef p_point, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextMoveTo(t_canvas->context, *MCCanvasPointGet(p_point));
}

MC_DLLEXPORT_DEF
void MCCanvasLineTo(MCCanvasPointRef p_point, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextLineTo(t_canvas->context, *MCCanvasPointGet(p_point));
}

MC_DLLEXPORT_DEF
void MCCanvasCurveThroughPoint(MCCanvasPointRef p_through, MCCanvasPointRef p_to, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextQuadraticTo(t_canvas->context, *MCCanvasPointGet(p_through), *MCCanvasPointGet(p_to));
}

MC_DLLEXPORT_DEF
void MCCanvasCurveThroughPoints(MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextCubicTo(t_canvas->context, *MCCanvasPointGet(p_through_a), *MCCanvasPointGet(p_through_b), *MCCanvasPointGet(p_to));
}

MC_DLLEXPORT_DEF
void MCCanvasClosePath(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextCloseSubpath(t_canvas->context);
}

MC_DLLEXPORT_DEF
void MCCanvasFillText(MCStringRef p_text, MCCanvasPointRef p_point, MCCanvasRef p_canvas)
{
	MCGPoint t_point;
	t_point = *MCCanvasPointGet(p_point);
	
	MCGContextRef t_context;
	t_context = MCCanvasGet(p_canvas)->context;
	
	MCFontRef t_font;
	t_font = MCCanvasFontGetMCFont(MCCanvasGetProps(p_canvas).font);

	MCCanvasApplyChanges(*MCCanvasGet(p_canvas));
	MCFontDrawText(t_context, t_point.x, t_point.y, p_text, t_font, false, false);
}

void MCCanvasFillTextAligned(MCStringRef p_text, integer_t p_halign, integer_t p_valign, MCCanvasRectangleRef p_rect, MCCanvasRef p_canvas)
{
	MCFontRef t_font;
	t_font = MCCanvasFontGetMCFont(MCCanvasGetProps(p_canvas).font);
	
	MCGContextRef t_context;
	t_context = MCCanvasGet(p_canvas)->context;
	
	MCGRectangle t_rect;
	t_rect = *MCCanvasRectangleGet(p_rect);
	
	int32_t t_text_width;
	t_text_width = MCFontMeasureText(t_font, p_text, MCGContextGetDeviceTransform(t_context));
	
	int32_t t_x;
	switch(p_halign)
	{
		case -1:
			t_x = 0;
			break;
		case 0:
			t_x = (t_rect . size . width - t_text_width) / 2;
			break;
		case 1:
			t_x = t_rect . size . width - t_text_width;
			break;
		default:
			MCUnreachableReturn()
	}
	
	int32_t t_y;
	switch(p_valign)
	{
		case -1:
			t_y = MCFontGetAscent(t_font);
			break;
		case 0:
			t_y = (t_rect . size . height - (MCFontGetAscent(t_font) + MCFontGetDescent(t_font))) / 2 + MCFontGetAscent(t_font);
			break;
		case 1:
			t_y = t_rect . size . height - MCFontGetDescent(t_font);
			break;
		default:
			MCUnreachableReturn()
	}
	
	MCCanvasApplyChanges(*MCCanvasGet(p_canvas));
	MCFontDrawText(t_context, t_rect.origin.x + t_x, t_rect.origin.y + t_y, p_text, t_font, false, false);
}

MC_DLLEXPORT_DEF
void MCCanvasAlignmentEvaluate(integer_t p_h_align, integer_t p_v_align, integer_t &r_align)
{
	// range of h/v align is -1, 0, 1 so shift to 0,1,2 and combine
	r_align = (p_h_align + 1) | ((p_v_align + 1) << 2);
}

void MCCanvasAlignmentSplit(integer_t p_align, integer_t &r_h_align, integer_t &r_v_align)
{
	r_h_align = (0x3 & p_align) - 1;
	r_v_align = (0x3 & (p_align >> 2)) - 1;
}

MC_DLLEXPORT_DEF
void MCCanvasFillTextAligned(MCStringRef p_text, integer_t p_align, MCCanvasRectangleRef p_rect, MCCanvasRef p_canvas)
{
	integer_t t_h_aligh, t_v_align;
	MCCanvasAlignmentSplit(p_align, t_h_aligh, t_v_align);
	MCCanvasFillTextAligned(p_text, t_h_aligh, t_v_align, p_rect, p_canvas);
}

MC_DLLEXPORT_DEF
MCCanvasRectangleRef MCCanvasMeasureText(MCStringRef p_text, MCCanvasRef p_canvas)
{
	MCCanvasRectangleRef t_rect;
	t_rect = nil;
	
	MCCanvasFontMeasureTextTypographicBoundsOnCanvas(p_text, p_canvas, t_rect);
	
	return t_rect;
}

////////////////////////////////////////////////////////////////////////////////

static MCCanvasRef s_current_canvas = nil;

void MCCanvasPush(MCGContextRef p_gcontext, uintptr_t& r_cookie)
{
    MCCanvasRef t_new_canvas;
    MCCanvasCreate(p_gcontext, t_new_canvas);
    r_cookie = (uintptr_t)s_current_canvas;
    s_current_canvas = t_new_canvas;
}

void MCCanvasPop(uintptr_t p_cookie)
{
    MCCanvasRef t_canvas;
    t_canvas = s_current_canvas;
    s_current_canvas = (MCCanvasRef)p_cookie;
    MCValueRelease(t_canvas);
}

extern "C" MC_DLLEXPORT_DEF void MCCanvasThisCanvas(MCCanvasRef& r_canvas)
{
    if (s_current_canvas == nil)
    {
        MCErrorThrowGeneric(MCSTR("no current canvas"));
        return;
    }
    
    r_canvas = MCValueRetain(s_current_canvas);
}

extern "C" MC_DLLEXPORT_DEF void MCCanvasPretendToAssignToThisCanvas(MCCanvasRef p_canvas)
{
}

extern "C" MC_DLLEXPORT_DEF void MCCanvasNewCanvasWithSize(MCProperListRef p_list, MCCanvasRef& r_canvas)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_list, t_scale))
		return;

    MCGContextRef t_gcontext;
    if (!MCGContextCreate(ceil(t_scale . x), ceil(t_scale . y), false, t_gcontext))
    {
        MCErrorThrowGeneric(MCSTR("could not create gcontext"));
        return;
    }
    
    MCCanvasRef t_canvas;
    if (!MCCanvasCreate(t_gcontext, t_canvas))
    {
        MCGContextRelease(t_gcontext);
        return;
    }
    
    MCGContextRelease(t_gcontext);
    
    r_canvas = t_canvas;
}

extern "C" MC_DLLEXPORT_DEF void MCCanvasGetPixelDataOfCanvas(MCCanvasRef p_canvas, MCDataRef& r_data)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
    
    uint32_t t_width, t_height;
    t_width = MCGContextGetWidth(t_canvas -> context);
    t_height = MCGContextGetHeight(t_canvas -> context);
    
    void *t_pixels;
    t_pixels = MCGContextGetPixelPtr(t_canvas -> context);
    
    uint32_t t_pixel_count;
    t_pixel_count = t_width * t_height;
    
    uint32_t *t_my_pixels, *t_data_ptr;
    t_my_pixels = new (nothrow) uint32_t[t_pixel_count];
    memcpy(t_my_pixels, t_pixels, t_pixel_count * sizeof(uint32_t));
    
    t_data_ptr = t_my_pixels;
#if (kMCGPixelFormatNative != kMCGPixelFormatARGB)
    while (t_pixel_count--)
    {
        uint8_t t_r, t_g, t_b, t_a;
        MCGPixelUnpackNative(*t_data_ptr, t_r, t_g, t_b, t_a);
        *t_data_ptr++ = MCGPixelPack(kMCGPixelFormatARGB, t_r, t_g, t_b, t_a);
    }
#endif
    
    if (!MCDataCreateWithBytesAndRelease((byte_t *)t_my_pixels, t_width * t_height * sizeof(uint32_t), r_data))
        return;
}

extern "C" MC_DLLEXPORT_DEF void MCCanvasGetPixelWidthOfCanvas(MCCanvasRef p_canvas, uinteger_t& r_width)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
    r_width = MCGContextGetWidth(t_canvas -> context);
}

extern "C" MC_DLLEXPORT_DEF void MCCanvasGetPixelHeightOfCanvas(MCCanvasRef p_canvas, uinteger_t& r_height)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
    r_height = MCGContextGetHeight(t_canvas -> context);
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
    nil,
    nil,
};

static MCValueCustomCallbacks kMCCanvasPointCustomValueCallbacks =
{
	false,
	__MCCanvasPointDestroy,
	__MCCanvasPointCopy,
	__MCCanvasPointEqual,
	__MCCanvasPointHash,
	__MCCanvasPointDescribe,
    nil,
    nil,
};

static MCValueCustomCallbacks kMCCanvasColorCustomValueCallbacks =
{
	false,
	__MCCanvasColorDestroy,
	__MCCanvasColorCopy,
	__MCCanvasColorEqual,
	__MCCanvasColorHash,
	__MCCanvasColorDescribe,
    nil,
    nil,
};

static MCValueCustomCallbacks kMCCanvasTransformCustomValueCallbacks =
{
	false,
	__MCCanvasTransformDestroy,
	__MCCanvasTransformCopy,
	__MCCanvasTransformEqual,
	__MCCanvasTransformHash,
	__MCCanvasTransformDescribe,
    nil,
    nil,
};

static MCValueCustomCallbacks kMCCanvasImageCustomValueCallbacks =
{
	false,
	__MCCanvasImageDestroy,
	__MCCanvasImageCopy,
	__MCCanvasImageEqual,
	__MCCanvasImageHash,
	__MCCanvasImageDescribe,
    nil,
    nil,
};

static MCValueCustomCallbacks kMCCanvasPaintCustomValueCallbacks =
{
	false,
	nil,
	nil,
	nil,
	nil,
	nil,
    nil,
    nil,
};

static MCValueCustomCallbacks kMCCanvasSolidPaintCustomValueCallbacks =
{
	false,
	__MCCanvasSolidPaintDestroy,
	__MCCanvasSolidPaintCopy,
	__MCCanvasSolidPaintEqual,
	__MCCanvasSolidPaintHash,
	__MCCanvasSolidPaintDescribe,
    nil,
    nil,
};

static MCValueCustomCallbacks kMCCanvasPatternCustomValueCallbacks =
{
	false,
	__MCCanvasPatternDestroy,
	__MCCanvasPatternCopy,
	__MCCanvasPatternEqual,
	__MCCanvasPatternHash,
	__MCCanvasPatternDescribe,
    nil,
    nil,
};

static MCValueCustomCallbacks kMCCanvasGradientCustomValueCallbacks =
{
	false,
	__MCCanvasGradientDestroy,
	__MCCanvasGradientCopy,
	__MCCanvasGradientEqual,
	__MCCanvasGradientHash,
	__MCCanvasGradientDescribe,
    nil,
    nil,
};

static MCValueCustomCallbacks kMCCanvasGradientStopCustomValueCallbacks =
{
	false,
	__MCCanvasGradientStopDestroy,
	__MCCanvasGradientStopCopy,
	__MCCanvasGradientStopEqual,
	__MCCanvasGradientStopHash,
	__MCCanvasGradientStopDescribe,
    nil,
    nil,
};

static MCValueCustomCallbacks kMCCanvasPathCustomValueCallbacks =
{
	false,
	__MCCanvasPathDestroy,
	__MCCanvasPathCopy,
	__MCCanvasPathEqual,
	__MCCanvasPathHash,
	__MCCanvasPathDescribe,
    nil,
    nil,
};

static MCValueCustomCallbacks kMCCanvasEffectCustomValueCallbacks =
{
	false,
	__MCCanvasEffectDestroy,
	__MCCanvasEffectCopy,
	__MCCanvasEffectEqual,
	__MCCanvasEffectHash,
	__MCCanvasEffectDescribe,
    nil,
    nil,
};

// 2019-11-05 mdw [[ font_callback ]] fixed typo
static MCValueCustomCallbacks kMCCanvasFontCustomValueCallbacks =
{
	false,
	__MCCanvasFontDestroy,
	__MCCanvasFontCopy,
	__MCCanvasFontEqual,
	__MCCanvasFontHash,
	__MCCanvasFontDescribe,
    nil,
    nil,
};

static MCValueCustomCallbacks kMCCanvasCustomValueCallbacks =
{
	true,
	__MCCanvasDestroy,
	__MCCanvasCopy,
	__MCCanvasEqual,
	__MCCanvasHash,
	__MCCanvasDescribe,
    nil,
    nil,
};

bool MCCanvasTypesInitialize()
{
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.Rectangle"), kMCNullTypeInfo, &kMCCanvasRectangleCustomValueCallbacks, kMCCanvasRectangleTypeInfo))
		return false;
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.Point"), kMCNullTypeInfo, &kMCCanvasPointCustomValueCallbacks, kMCCanvasPointTypeInfo))
		return false;
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.Color"), kMCNullTypeInfo, &kMCCanvasColorCustomValueCallbacks, kMCCanvasColorTypeInfo))
		return false;
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.Transform"), kMCNullTypeInfo, &kMCCanvasTransformCustomValueCallbacks, kMCCanvasTransformTypeInfo))
		return false;
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.Image"), kMCNullTypeInfo, &kMCCanvasImageCustomValueCallbacks, kMCCanvasImageTypeInfo))
		return false;
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.Paint"), kMCNullTypeInfo, &kMCCanvasPaintCustomValueCallbacks, kMCCanvasPaintTypeInfo))
		return false;
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.SolidPaint"), kMCCanvasPaintTypeInfo, &kMCCanvasSolidPaintCustomValueCallbacks, kMCCanvasSolidPaintTypeInfo))
		return false;
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.Pattern"), kMCCanvasPaintTypeInfo, &kMCCanvasPatternCustomValueCallbacks, kMCCanvasPatternTypeInfo))
		return false;
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.Gradient"), kMCCanvasPaintTypeInfo, &kMCCanvasGradientCustomValueCallbacks, kMCCanvasGradientTypeInfo))
		return false;
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.GradientStop"), kMCNullTypeInfo, &kMCCanvasGradientStopCustomValueCallbacks, kMCCanvasGradientStopTypeInfo))
		return false;
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.Path"), kMCNullTypeInfo, &kMCCanvasPathCustomValueCallbacks, kMCCanvasPathTypeInfo))
		return false;
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.Effect"), kMCNullTypeInfo, &kMCCanvasEffectCustomValueCallbacks, kMCCanvasEffectTypeInfo))
		return false;
// 2019-11-05 mdw [[ font_callback ]] fixed typo
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.Font"), kMCNullTypeInfo, &kMCCanvasFontCustomValueCallbacks, kMCCanvasFontTypeInfo))
		return false;
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.canvas.Canvas"), kMCNullTypeInfo, &kMCCanvasCustomValueCallbacks, kMCCanvasTypeInfo))
		return false;
	return true;
}

void MCCanvasTypesFinalize()
{
	MCValueRelease(kMCCanvasRectangleTypeInfo);
	MCValueRelease(kMCCanvasPointTypeInfo);
	MCValueRelease(kMCCanvasColorTypeInfo);
	MCValueRelease(kMCCanvasTransformTypeInfo);
	MCValueRelease(kMCCanvasImageTypeInfo);
	MCValueRelease(kMCCanvasPaintTypeInfo);
	MCValueRelease(kMCCanvasSolidPaintTypeInfo);
	MCValueRelease(kMCCanvasPatternTypeInfo);
	MCValueRelease(kMCCanvasGradientTypeInfo);
	MCValueRelease(kMCCanvasGradientStopTypeInfo);
	MCValueRelease(kMCCanvasPathTypeInfo);
	MCValueRelease(kMCCanvasEffectTypeInfo);
	MCValueRelease(kMCCanvasFontTypeInfo);
	MCValueRelease(kMCCanvasTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////

bool MCCanvasThrowError(MCTypeInfoRef p_error_type)
{
	MCAutoErrorRef t_error;
	if (!MCErrorCreate(p_error_type, nil, &t_error))
		return false;
	
	return MCErrorThrow(*t_error);
}

bool MCCanvasErrorsInitialize()
{
	kMCCanvasRectangleListFormatErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.RectangleListFormatError"), MCNAME("canvas"), MCSTR("Rectangle parameter must be a list of 4 numbers."), kMCCanvasRectangleListFormatErrorTypeInfo))
		return false;
	
	kMCCanvasPointListFormatErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.PointListFormatError"), MCNAME("canvas"), MCSTR("Point parameter must be a list of 2 numbers."), kMCCanvasPointListFormatErrorTypeInfo))
		return false;
	
	kMCCanvasColorListFormatErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.ColorListFormatError"), MCNAME("canvas"), MCSTR("Color parameter must be a list of 3 or 4 numbers between 0 and 1."), kMCCanvasColorListFormatErrorTypeInfo))
		return false;
	
	kMCCanvasScaleListFormatErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.ScaleListFormatError"), MCNAME("canvas"), MCSTR("Scale parameter must be a list of 1 or 2 numbers."), kMCCanvasScaleListFormatErrorTypeInfo))
		return false;

	kMCCanvasTranslationListFormatErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.TranslationListFormatError"), MCNAME("canvas"), MCSTR("Translation parameter must be a list of 2 numbers."), kMCCanvasTranslationListFormatErrorTypeInfo))
		return false;

	kMCCanvasSkewListFormatErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.SkewListFormatError"), MCNAME("canvas"), MCSTR("Skew parameter must be a list of 2 numbers."), kMCCanvasSkewListFormatErrorTypeInfo))
		return false;

	kMCCanvasRadiiListFormatErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.RadiiListFormatError"), MCNAME("canvas"), MCSTR("Radii parameter must be a list of 2 numbers."), kMCCanvasRadiiListFormatErrorTypeInfo))
		return false;
	
	kMCCanvasImageSizeListFormatErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.ImageSizeListFormatError"), MCNAME("canvas"), MCSTR("image size parameter must be a list of 2 integers greater than 0."), kMCCanvasImageSizeListFormatErrorTypeInfo))
		return false;
	
	kMCCanvasTransformMatrixListFormatErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.TransformMatrixListFormatError"), MCNAME("canvas"), MCSTR("transform matrix parameter must be a list of 6 numbers."), kMCCanvasTransformMatrixListFormatErrorTypeInfo))
		return false;
	
	kMCCanvasTransformDecomposeErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.TransformDecomposeError"), MCNAME("canvas"), MCSTR("Unable to decompose transform matrix."), kMCCanvasTransformDecomposeErrorTypeInfo))
		return false;
	
	kMCCanvasImageRepReferencedErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.ImageRepReferencedError"), MCNAME("canvas"), MCSTR("Unable to create image from reference."), kMCCanvasImageRepReferencedErrorTypeInfo))
		return false;
	
	kMCCanvasImageRepDataErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.ImageRepDataError"), MCNAME("canvas"), MCSTR("Unable to create image from data."), kMCCanvasImageRepDataErrorTypeInfo))
		return false;
	
	kMCCanvasImageRepPixelsErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.ImageRepPixelsError"), MCNAME("canvas"), MCSTR("Unable to create image with pixels."), kMCCanvasImageRepPixelsErrorTypeInfo))
		return false;
	
	kMCCanvasImageRepGetGeometryErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.ImageRepGetGeometryError"), MCNAME("canvas"), MCSTR("Unable to get image geometry."), kMCCanvasImageRepGetGeometryErrorTypeInfo))
		return false;
	
	kMCCanvasImageRepGetMetadataErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.ImageRepGetMetadataError"), MCNAME("canvas"), MCSTR("Unable to get image metadata."), kMCCanvasImageRepGetMetadataErrorTypeInfo))
		return false;
	
	kMCCanvasImageRepGetDensityErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.ImageRepGetDensityError"), MCNAME("canvas"), MCSTR("Unable to get image density."), kMCCanvasImageRepGetDensityErrorTypeInfo))
		return false;
	
	kMCCanvasImageRepLockErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.ImageRepLockError"), MCNAME("canvas"), MCSTR("Unable to lock image pixels."), kMCCanvasImageRepLockErrorTypeInfo))
		return false;
	
	kMCCanvasGradientInvalidRampErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.GradientInvalidRampError"),
									MCNAME("canvas"),
									MCSTR("Gradient ramps must have at least one stop."),
									kMCCanvasGradientInvalidRampErrorTypeInfo))
		return false;
	
	kMCCanvasGradientStopRangeErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.GradientStopRangeError"), MCNAME("canvas"), MCSTR("Gradient stop offset must be between 0 and 1."), kMCCanvasGradientStopRangeErrorTypeInfo))
		return false;
	
	kMCCanvasGradientStopOrderErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.GradientStopOrderError"), MCNAME("canvas"), MCSTR("Gradient stops must be provided in order of increasing offset."), kMCCanvasGradientStopOrderErrorTypeInfo))
		return false;
	
	kMCCanvasGradientTypeErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.GradientTypeError"), MCNAME("canvas"), MCSTR("Unrecognised gradient type."), kMCCanvasGradientTypeErrorTypeInfo))
		return false;
	
	kMCCanvasEffectInvalidPropertyErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.EffectInvalidPropertyError"), MCNAME("canvas"), MCSTR("Unrecognised effect property \"%{property}\"."), kMCCanvasEffectInvalidPropertyErrorTypeInfo))
		return false;
	
	kMCCanvasEffectPropertyNotAvailableErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.EffectPropertyNotAvailableError"), MCNAME("canvas"), MCSTR("Property \"%{property}\" not valid for effect type %{type}"), kMCCanvasEffectPropertyNotAvailableErrorTypeInfo))
		return false;
	
	kMCCanvasEffectPropertyInvalidValueErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.EffectPropertyInvalidValueError"), MCNAME("canvas"), MCSTR("Invalid value for effect property \"%{property}\" - %{value}"), kMCCanvasEffectPropertyInvalidValueErrorTypeInfo))
		return false;
	
	kMCCanvasPathPointListFormatErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.PathPointListFormatError"), MCNAME("canvas"), MCSTR("Invalid value in list of points."), kMCCanvasPathPointListFormatErrorTypeInfo))
		return false;
	
	kMCCanvasSVGPathParseErrorTypeInfo = nil;
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.canvas.SVGPathParseError"), MCNAME("canvas"), MCSTR("Unable to parse path data: \"%{reason}\" at position %{position}"), kMCCanvasSVGPathParseErrorTypeInfo))
		return false;
	
	return true;
}

void MCCanvasErrorsFinalize()
{
	MCValueRelease(kMCCanvasRectangleListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasPointListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasColorListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasScaleListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasTranslationListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasSkewListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasRadiiListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasImageSizeListFormatErrorTypeInfo);
	
	MCValueRelease(kMCCanvasTransformMatrixListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasTransformDecomposeErrorTypeInfo);
	
	MCValueRelease(kMCCanvasImageRepReferencedErrorTypeInfo);
	MCValueRelease(kMCCanvasImageRepDataErrorTypeInfo);
	MCValueRelease(kMCCanvasImageRepPixelsErrorTypeInfo);
	MCValueRelease(kMCCanvasImageRepGetGeometryErrorTypeInfo);
	MCValueRelease(kMCCanvasImageRepGetMetadataErrorTypeInfo);
	MCValueRelease(kMCCanvasImageRepGetDensityErrorTypeInfo);
	MCValueRelease(kMCCanvasImageRepLockErrorTypeInfo);
	
	MCValueRelease(kMCCanvasGradientStopRangeErrorTypeInfo);
	MCValueRelease(kMCCanvasGradientStopOrderErrorTypeInfo);
	MCValueRelease(kMCCanvasGradientTypeErrorTypeInfo);
	
	MCValueRelease(kMCCanvasEffectInvalidPropertyErrorTypeInfo);
	MCValueRelease(kMCCanvasEffectPropertyNotAvailableErrorTypeInfo);
	MCValueRelease(kMCCanvasEffectPropertyInvalidValueErrorTypeInfo);

	MCValueRelease(kMCCanvasPathPointListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasSVGPathParseErrorTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////

bool MCCanvasConstantsInitialize()
{
	if (!MCCanvasTransformCreateWithMCGAffineTransform(MCGAffineTransformMakeIdentity(), kMCCanvasIdentityTransform))
		return false;
	if (!MCCanvasColorCreateWithRGBA(0, 0, 0, 1, kMCCanvasColorBlack))
		return false;
	// Defer creation until after initialize
	kMCCanvasFont12PtHelvetica = nil;
//	if (!MCCanvasFontCreate(MCNAME("Helvetica"), 0, 12, kMCCanvasFont12PtHelvetica))
//		return false;
	if (!MCCanvasPathCreateEmpty(kMCCanvasEmptyPath))
		return false;
	
	return true;
}

void MCCanvasConstantsFinalize()
{
	MCValueRelease(kMCCanvasIdentityTransform);
	MCValueRelease(kMCCanvasColorBlack);
	MCValueRelease(kMCCanvasEmptyPath);
}

////////////////////////////////////////////////////////////////////////////////

bool MCCanvasStringsInitialize()
{
	MCMemoryClear(s_blend_mode_map, sizeof(s_blend_mode_map));
	MCMemoryClear(s_transform_matrix_keys, sizeof(s_transform_matrix_keys));
	MCMemoryClear(s_effect_type_map, sizeof(s_effect_type_map));
	MCMemoryClear(s_effect_property_map, sizeof(s_effect_property_map));
	MCMemoryClear(s_effect_source_map, sizeof(s_effect_source_map));
	MCMemoryClear(s_gradient_type_map, sizeof(s_gradient_type_map));
	MCMemoryClear(s_canvas_fillrule_map, sizeof(s_canvas_fillrule_map));
	MCMemoryClear(s_image_filter_map, sizeof(s_image_filter_map));
	MCMemoryClear(s_join_style_map, sizeof(s_join_style_map));
	MCMemoryClear(s_cap_style_map, sizeof(s_cap_style_map));
	
	/* UNCHECKED */
	s_blend_mode_map[kMCGBlendModeClear] = MCNAME("clear");
	s_blend_mode_map[kMCGBlendModeCopy] = MCNAME("copy");
	s_blend_mode_map[kMCGBlendModeSourceOver] = MCNAME("source over");
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
	s_effect_property_map[kMCCanvasEffectPropertySize] = MCNAME("size");
	s_effect_property_map[kMCCanvasEffectPropertySpread] = MCNAME("spread");
	s_effect_property_map[kMCCanvasEffectPropertyDistance] = MCNAME("distance");
	s_effect_property_map[kMCCanvasEffectPropertyAngle] = MCNAME("angle");
	s_effect_property_map[kMCCanvasEffectPropertyKnockOut] = MCNAME("knockout");
	s_effect_property_map[kMCCanvasEffectPropertySource] = MCNAME("source");
	
	s_effect_source_map[kMCCanvasEffectSourceCenter] = MCNAME("center");
	s_effect_source_map[kMCCanvasEffectSourceEdge] = MCNAME("edge");
	
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
	
	s_join_style_map[kMCGJoinStyleBevel] = MCNAME("bevel");
	s_join_style_map[kMCGJoinStyleMiter] = MCNAME("miter");
	s_join_style_map[kMCGJoinStyleRound] = MCNAME("round");
	
	s_cap_style_map[kMCGCapStyleButt] = MCNAME("butt");
	s_cap_style_map[kMCGCapStyleRound] = MCNAME("round");
	s_cap_style_map[kMCGCapStyleSquare] = MCNAME("square");
	
/* UNCHECKED */
	return true;
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
	
	for (uint32_t i = 0; i < _MCCanvasEffectSourceCount; i++)
		MCValueRelease(s_effect_source_map[i]);
	
	for (uint32_t i = 0; i < kMCGGradientFunctionCount; i++)
		MCValueRelease(s_gradient_type_map[i]);
	
	for (uint32_t i = 0; i < kMCGFillRuleCount; i++)
		MCValueRelease(s_canvas_fillrule_map[i]);
		
	for (uint32_t i = 0; i < kMCGImageFilterCount; i++)
		MCValueRelease(s_image_filter_map[i]);
	
	for (uint32_t i = 0; i < kMCGJoinStyleCount; i++)
		MCValueRelease(s_join_style_map);
	
	for (uint32_t i = 0; i < kMCGCapStyleSquare; i++)
		MCValueRelease(s_cap_style_map);
}

template <typename T, int C>
bool _mcenumfromstring(MCNameRef *N, MCStringRef p_string, T &r_value)
{
	for (uint32_t i = 0; i < C; i++)
	{
		if (N[i] != nil && MCStringIsEqualTo(p_string, MCNameGetString(N[i]), kMCStringOptionCompareCaseless))
		{
			r_value = (T)i;
			return true;
		}
	}
	
	return false;
}

template <typename T, int C>
bool _mcenumtostring(MCNameRef *N, T p_value, MCStringRef &r_string)
{
	if (p_value >= C)
		return false;
	
	if (N[p_value] == nil)
		return false;
	
	r_string = MCValueRetain(MCNameGetString(N[p_value]));
    return true;
}

bool MCCanvasBlendModeFromString(MCStringRef p_string, MCGBlendMode &r_blend_mode)
{
	return _mcenumfromstring<MCGBlendMode, kMCGBlendModeCount>(s_blend_mode_map, p_string, r_blend_mode);
}

bool MCCanvasBlendModeToString(MCGBlendMode p_blend_mode, MCStringRef &r_string)
{
	return _mcenumtostring<MCGBlendMode, kMCGBlendModeCount>(s_blend_mode_map, p_blend_mode, r_string);
}

bool MCCanvasGradientTypeFromString(MCStringRef p_string, MCGGradientFunction &r_type)
{
	return _mcenumfromstring<MCGGradientFunction, kMCGGradientFunctionCount>(s_gradient_type_map, p_string, r_type);
}

bool MCCanvasGradientTypeToString(MCGGradientFunction p_type, MCStringRef &r_string)
{
	return _mcenumtostring<MCGGradientFunction, kMCGGradientFunctionCount>(s_gradient_type_map, p_type, r_string);
}

bool MCCanvasEffectTypeToString(MCCanvasEffectType p_type, MCStringRef &r_string)
{
	return _mcenumtostring<MCCanvasEffectType, _MCCanvasEffectTypeCount>(s_effect_type_map, p_type, r_string);
}

bool MCCanvasEffectTypeFromString(MCStringRef p_string, MCCanvasEffectType &r_type)
{
	return _mcenumfromstring<MCCanvasEffectType, _MCCanvasEffectTypeCount>(s_effect_type_map, p_string, r_type);
}

bool MCCanvasEffectPropertyToString(MCCanvasEffectProperty p_property, MCStringRef &r_string)
{
	return _mcenumtostring<MCCanvasEffectProperty, _MCCanvasEffectPropertyCount>(s_effect_property_map, p_property, r_string);
}

bool MCCanvasEffectPropertyFromString(MCStringRef p_string, MCCanvasEffectProperty &r_property)
{
	return _mcenumfromstring<MCCanvasEffectProperty, _MCCanvasEffectPropertyCount>(s_effect_property_map, p_string, r_property);
}

bool MCCanvasEffectSourceToString(MCCanvasEffectSource p_source, MCStringRef &r_string)
{
	return _mcenumtostring<MCCanvasEffectSource, _MCCanvasEffectSourceCount>(s_effect_source_map, p_source, r_string);
}

bool MCCanvasEffectSourceFromString(MCStringRef p_string, MCCanvasEffectSource &r_source)
{
	return _mcenumfromstring<MCCanvasEffectSource, _MCCanvasEffectSourceCount>(s_effect_source_map, p_string, r_source);
}

bool MCCanvasFillRuleToString(MCGFillRule p_fill_rule, MCStringRef &r_string)
{
	return _mcenumtostring<MCGFillRule, kMCGFillRuleCount>(s_canvas_fillrule_map, p_fill_rule, r_string);
}

bool MCCanvasFillRuleFromString(MCStringRef p_string, MCGFillRule &r_fill_rule)
{
	return _mcenumfromstring<MCGFillRule, kMCGFillRuleCount>(s_canvas_fillrule_map, p_string, r_fill_rule);
}

bool MCCanvasImageFilterToString(MCGImageFilter p_filter, MCStringRef &r_string)
{
	return _mcenumtostring<MCGImageFilter, kMCGImageFilterCount>(s_image_filter_map, p_filter, r_string);
}

bool MCCanvasImageFilterFromString(MCStringRef p_string, MCGImageFilter &r_filter)
{
	return _mcenumfromstring<MCGImageFilter, kMCGImageFilterCount>(s_image_filter_map, p_string, r_filter);
}

bool MCCanvasJoinStyleToString(MCGJoinStyle p_style, MCStringRef &r_string)
{
	return _mcenumtostring<MCGJoinStyle, kMCGJoinStyleCount>(s_join_style_map, p_style, r_string);
}

bool MCCanvasJoinStyleFromString(MCStringRef p_string, MCGJoinStyle &r_style)
{
	return _mcenumfromstring<MCGJoinStyle, kMCGJoinStyleCount>(s_join_style_map, p_string, r_style);
}

bool MCCanvasCapStyleToString(MCGCapStyle p_style, MCStringRef &r_string)
{
	return _mcenumtostring<MCGCapStyle, kMCGCapStyleCount>(s_cap_style_map, p_style, r_string);
}

bool MCCanvasCapStyleFromString(MCStringRef p_string, MCGCapStyle &r_style)
{
	return _mcenumfromstring<MCGCapStyle, kMCGCapStyleCount>(s_cap_style_map, p_string, r_style);
}

////////////////////////////////////////////////////////////////////////////////

bool MCSVGThrowPathParseError(uint32_t p_char_position, MCStringRef p_error)
{
	MCAutoNumberRef t_number;
	if (!MCNumberCreateWithUnsignedInteger(p_char_position + 1, &t_number))
		return false;
	
	return MCErrorCreateAndThrow(kMCCanvasSVGPathParseErrorTypeInfo,
								 "position", *t_number,
								 "reason", p_error,
								 nil);
}

struct MCSVGPathCommandMap
{
	char letter;
	MCSVGPathCommand command;
};

static MCSVGPathCommandMap s_svg_command_map[] = {
	{'M', kMCSVGPathMoveTo},
	{'m', kMCSVGPathRelativeMoveTo},
	{'Z', kMCSVGPathClose},
	{'z', kMCSVGPathClose},
	{'L', kMCSVGPathLineTo},
	{'l', kMCSVGPathRelativeLineTo},
	{'H', kMCSVGPathHorizontalLineTo},
	{'h', kMCSVGPathRelativeHorizontalLineTo},
	{'V', kMCSVGPathVerticalLineTo},
	{'v', kMCSVGPathRelativeVerticalLineTo},
	{'C', kMCSVGPathCurveTo},
	{'c', kMCSVGPathRelativeCurveTo},
	{'S', kMCSVGPathShorthandCurveTo},
	{'s', kMCSVGPathRelativeShorthandCurveTo},
	{'Q', kMCSVGPathQuadraticCurveTo},
	{'q', kMCSVGPathRelativeQuadraticCurveTo},
	{'T', kMCSVGPathShorthandQuadraticCurveTo},
	{'t', kMCSVGPathRelativeShorthandQuadraticCurveTo},
	{'A', kMCSVGPathEllipticalCurveTo},
	{'a', kMCSVGPathRelativeEllipticalCurveTo},
};

bool MCSVGLookupPathCommand(char p_char, MCSVGPathCommand &r_command)
{
	for (uint32_t i = 0; i < sizeof(s_svg_command_map) / sizeof(s_svg_command_map[0]); i++)
		if (p_char == s_svg_command_map[i].letter)
		{
			r_command = s_svg_command_map[i].command;
			return true;
		}
	
	return false;
}

bool MCSVGParsePathCommand(const char *p_string, MCRange &x_range, MCSVGPathCommand &r_command)
{
	if (MCRangeIsEmpty(x_range))
		return false;
	
	if (!MCSVGLookupPathCommand(p_string[x_range.offset], r_command))
		return false;
	
	x_range = MCRangeIncrementOffset(x_range, 1);
	return true;
}

bool MCSVGTryToParseRangeAsReal(const char *p_string, const MCRange &p_range, MCRange *r_out_range, real64_t &r_real)
{
	const char *t_start;
	t_start = p_string + p_range.offset;
	
	char *t_end;
	t_end = nil;
	
	// IM-2015-08-04: [[ Bug 15681 ]] clear errno to avoid spurious ERANGE error.
	errno = 0;
	
	real64_t t_real;
	t_real = strtod(t_start, &t_end);
	
	if ((errno == ERANGE) || (r_out_range == nil && t_end - t_start != p_range.length) || t_end == (p_string + p_range.offset))
		return false;
	
	r_real = t_real;
	if (r_out_range != nil)
		*r_out_range = MCRangeMake(p_range.offset, t_end - t_start);
	
	return true;
}

bool MCSVGParseReal(const char *p_string, MCRange &x_range, real64_t &r_real)
{
	MCRange t_used;
	
	if (!MCSVGTryToParseRangeAsReal(p_string, x_range, &t_used, r_real))
		return false;
	
	uindex_t t_next;
	t_next = t_used.offset + t_used.length;
	
	x_range = MCRangeSetMinimum(x_range, t_used.offset + t_used.length);
	
	return true;
}

bool MCSVGIsWhiteSpace(char p_char)
{
	return p_char == ' ' || p_char == '\t' || p_char == '\r' || p_char == '\n';
}

void MCSVGSkipWhitespace(const char *p_string, MCRange &x_range)
{
	while (!MCRangeIsEmpty(x_range) && MCSVGIsWhiteSpace(p_string[x_range.offset]))
		x_range = MCRangeIncrementOffset(x_range, 1);
}

void MCSVGSkipComma(const char *p_string, MCRange &x_range)
{
	if (!MCRangeIsEmpty(x_range) && p_string[x_range.offset] == ',')
		x_range = MCRangeIncrementOffset(x_range, 1);
}

bool MCSVGConsumeWSPCommaWSP(const char *p_string, MCRange &x_range)
{
	MCRange t_range;
	t_range = x_range;
	
	MCSVGSkipWhitespace(p_string, x_range);
	MCSVGSkipComma(p_string, x_range);
	MCSVGSkipWhitespace(p_string, x_range);
	
	return !MCRangeIsEqual(x_range, t_range);
}

bool MCSVGParseParams(const char *p_string, MCRange &x_range, MCSVGPathCommand p_command, float32_t r_params[7], uindex_t &r_param_count)
{
	uindex_t t_param_count;
	MCRange t_range;
	t_range = x_range;
	
	switch (p_command)
	{
		case kMCSVGPathMoveTo:
		case kMCSVGPathRelativeMoveTo:
			t_param_count = 2;
			break;
			
		case kMCSVGPathClose:
			t_param_count = 0;
			break;
			
		case kMCSVGPathLineTo:
		case kMCSVGPathRelativeLineTo:
			t_param_count = 2;
			break;
			
		case kMCSVGPathHorizontalLineTo:
		case kMCSVGPathRelativeHorizontalLineTo:
			t_param_count = 1;
			break;
			
		case kMCSVGPathVerticalLineTo:
		case kMCSVGPathRelativeVerticalLineTo:
			t_param_count = 1;
			break;
			
		case kMCSVGPathCurveTo:
		case kMCSVGPathRelativeCurveTo:
			t_param_count = 6;
			break;
			
		case kMCSVGPathShorthandCurveTo:
		case kMCSVGPathRelativeShorthandCurveTo:
			t_param_count = 4;
			break;
			
		case kMCSVGPathQuadraticCurveTo:
		case kMCSVGPathRelativeQuadraticCurveTo:
			t_param_count = 4;
			break;
			
		case kMCSVGPathShorthandQuadraticCurveTo:
		case kMCSVGPathRelativeShorthandQuadraticCurveTo:
			t_param_count = 2;
			break;
			
		case kMCSVGPathEllipticalCurveTo:
		case kMCSVGPathRelativeEllipticalCurveTo:
			t_param_count = 7;
			break;
			
		default:
			MCUnreachable();
			break;
	}
	
	for (uint32_t i = 0; i < t_param_count; i++)
	{
		real64_t t_real;
		MCAutoNumberRef t_number;
		if (!MCSVGParseReal(p_string, t_range, t_real))
			return MCSVGThrowPathParseError(t_range.offset, MCSTR("Expected number value"));
		
		MCSVGConsumeWSPCommaWSP(p_string, t_range);
		
		r_params[i] = t_real;
	}
	
	r_param_count = t_param_count;
	x_range = t_range;
	
	return true;
}

bool MCSVGParse(MCStringRef p_string, MCSVGParseCallback p_callback, void *p_context)
{
	
	// Lock the string as native - only ASCII characters are valid in SVG path data.
	// The resulting string will be the same length as the input, but with ? where
	// any non-native chars are. Since an error will be thrown at the point of the
	// first non-ASCII char, we don't have to worry about char index mapping.
	MCAutoStringRefAsCString t_native_string;
	if (!t_native_string.Lock(p_string))
		return false;
	
	// Compute the initial range of the string.
	MCRange t_range;
	t_range = MCRangeMake(0, strlen(*t_native_string));
	
	bool t_first_command;
	t_first_command = true;
	MCSVGPathCommand t_command;
	while (!MCRangeIsEmpty(t_range))
	{
		// First skip any whitespace in the string. After this we know that we should
		// be expecting a path command, anything else is an error.
		MCSVGSkipWhitespace(*t_native_string, t_range);
		
		// If we don't manage to parse a path command then we assume it is a sequence
		// of the previous command.
		bool t_have_command;
		t_have_command = MCSVGParsePathCommand(*t_native_string, t_range, t_command);
		
		if (t_first_command)
		{
			// If this is the first command then it must exist and it must be a moveto.
			if (!t_have_command ||
				(t_command != kMCSVGPathMoveTo &&
				 t_command != kMCSVGPathRelativeMoveTo))
				return MCSVGThrowPathParseError(t_range.offset, MCSTR("Path must begin with moveto command"));
			
			t_first_command = false;
		}
		else
		{
			// If this is subsequent command and we did not parse a command then we
			// must map a move to to the corresponding line to. If we previously had
			// a close, then it is an error (since you can't have multiple closes -
			// they have no params!).
			if (!t_have_command)
			{
				if (t_command == kMCSVGPathMoveTo)
					t_command = kMCSVGPathLineTo;
				else if (t_command == kMCSVGPathRelativeMoveTo)
					t_command = kMCSVGPathRelativeLineTo;
				else if (t_command == kMCSVGPathClose)
					return MCSVGThrowPathParseError(t_range.offset, MCSTR("Path command character expected"));
			}
		}
		
		// Attempt to parse the parameters.
		float32_t t_params[7];
		uindex_t t_param_count;
		if (!MCSVGParseParams(*t_native_string, t_range, t_command, t_params, t_param_count))
			return false;
		
		if (!p_callback(p_context, t_command, t_params, t_param_count))
			return false;
        
        // Skip any trailing whitespace in the string
        MCSVGSkipWhitespace(*t_native_string, t_range);
        
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSVGAppendValueToString(MCStringRef p_string, bool p_need_separator, float32_t p_value)
{
	// negative values don't need a separator due to the leading minus sign.
	if (p_need_separator && p_value >= 0)
		return MCStringAppendFormat(p_string, " %f", p_value);
	else
		return MCStringAppendFormat(p_string, "%f", p_value);
}

struct MCGPathToSVGDataContext
{
	MCStringRef string;
	MCGPathCommand last_command;
};

bool MCGPathToSVGDataCallback(void *p_context, MCGPathCommand p_command, MCGPoint *p_points, uint32_t p_point_count)
{
	MCGPathToSVGDataContext *t_context;
	t_context = static_cast<MCGPathToSVGDataContext*>(p_context);
	
	bool t_success;
	t_success = true;
	
	bool t_repeat_command;
	t_repeat_command = p_command == t_context->last_command;
	
	if (!t_repeat_command)
	{
		char t_command;
		switch (p_command)
		{
			case kMCGPathCommandMoveTo:
				t_command = 'M';
				break;
				
			case kMCGPathCommandLineTo:
				t_command = 'L';
				break;
				
			case kMCGPathCommandQuadCurveTo:
				t_command = 'Q';
				break;
				
			case kMCGPathCommandCubicCurveTo:
				t_command = 'C';
				break;
				
			case kMCGPathCommandCloseSubpath:
				t_command = 'Z';
				break;
				
			case kMCGPathCommandEnd:
				return true;
				
			default:
				MCUnreachable();
				break;
		}
		
		t_success = MCStringAppendNativeChar(t_context->string, t_command);
		t_context->last_command = p_command;
	}
	
	for (uint32_t i = 0; t_success && i < p_point_count; i++)
	{
		t_success = MCSVGAppendValueToString(t_context->string, t_repeat_command || i != 0, p_points[i].x) &&
			MCSVGAppendValueToString(t_context->string, true, p_points[i].y);
	}
	
	return t_success;
}

bool MCGPathGetSVGData(MCGPathRef p_path, MCStringRef &r_string)
{
	if (MCGPathIsEmpty(p_path))
		return MCStringCopy(kMCEmptyString, r_string);
	
	bool t_success;
	t_success = true;
	
	MCStringRef t_string;
	t_string = nil;
	
	if (t_success)
		t_success = MCStringCreateMutable(0, t_string);
	
	if (t_success)
	{
		MCGPathToSVGDataContext t_context;
		t_context.string = t_string;
		t_context.last_command = kMCGPathCommandEnd;
		
		t_success = MCGPathIterate(p_path, MCGPathToSVGDataCallback, &t_context);
	}
	
	if (t_success)
		t_success = MCStringCopyAndRelease(t_string, r_string);
	
	if (!t_success)
		MCValueRelease(t_string);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
