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

#include "graphics.h"
#include "graphics-internal.h"

////////////////////////////////////////////////////////////////////////////////////////

static void __MCGSvgCreate(MCGSvgRef self, const void *data, size_t data_size);
static void __MCGSvgDestroy(MCGSvgRef self);
static bool __MCGSvgIsValid(MCGSvgRef self);
static void __MCGSvgInvalidate(MCGSvgRef self);
static void __MCGSvgRender(MCGSvgRef self, MCGContextRef target);

////////////////////////////////////////////////////////////////////////////////////////

bool MCGSvgCreate(const void *p_data,
                  size_t p_data_size,
                  MCGSvgRef& r_svg)
{
    bool t_success;
    t_success = true;
    
    __MCGSvg *t_svg;
    t_svg = NULL;
    
    if (!MCMemoryNew(t_svg))
        return false;
    
    __MCGSvgCreate(t_svg, p_data, p_data_size);
    
    r_svg = t_svg;
    
    return true;
}

MCGSvgRef MCGSvgRetain(MCGSvgRef self)
{
    if (self == NULL)
        return NULL;
    
    self -> references += 1;
    
    return self;
}

void MCGSvgRelease(MCGSvgRef self)
{
    if (self == NULL)
        return;
    
    self -> references -= 1;
    
    if (self -> references == 0)
        __MCGSvgDestroy(self);
}

bool MCGSvgIsValid(MCGSvgRef self)
{
    return __MCGSvgIsValid(self);
}

////////////////////////////////////////////////////////////////////////////////////////

MCGRectangle MCGSvgGetViewBox(MCGSvgRef self)
{
    if (!__MCGSvgIsValid(self))
        return MCGRectangleMake(0.0f, 0.0f, 0.0f, 0.0f);
    
    return self -> view_box;
}

MCGRectangle MCGSvgGetBoundingBox(MCGSvgRef self)
{
    if (!__MCGSvgIsValid(self))
        return MCGRectangleMake(0.0f, 0.0f, 0.0f, 0.0f);
    
    return self -> bounding_box;
}

////////////////////////////////////////////////////////////////////////////////////////

void MCGSvgRender(MCGSvgRef self, MCGContextRef target)
{
    if (!__MCGSvgIsValid(self))
        return;
    
    __MCGSvgRender(self, target);
}

////////////////////////////////////////////////////////////////////////////////////////

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"

static void __nsvgUnionBounds(float x_bounds[4],
                              float p_new_bounds[4])
{
    if (p_new_bounds[0] < x_bounds[0])
        x_bounds[0] = p_new_bounds[0];
    if (p_new_bounds[1] < x_bounds[1])
        x_bounds[1] = p_new_bounds[1];
    if (p_new_bounds[2] > x_bounds[2])
        x_bounds[2] = p_new_bounds[2];
    if (p_new_bounds[3] > x_bounds[3])
        x_bounds[3] = p_new_bounds[3];
}

static void __nsvgMeasure(NSVGimage *p_image,
                          MCGRectangle& r_bounds)
{
    float t_bounds[4];
    t_bounds[0] = FLT_MAX;
    t_bounds[1] = FLT_MAX;
    t_bounds[2] = -FLT_MAX;
    t_bounds[3] = -FLT_MAX;
    for(NSVGshape *t_shape = p_image -> shapes; t_shape != NULL; t_shape = t_shape -> next)
        __nsvgUnionBounds(t_bounds, t_shape -> bounds);
    
    r_bounds . origin . x = t_bounds[0];
    r_bounds . origin . y = t_bounds[1];
    r_bounds . size . width = t_bounds[2] - t_bounds[0];
    r_bounds . size . height = t_bounds[3] - t_bounds[1];
}

static void __MCGSvgCreate(MCGSvgRef self,
                           const void *p_data,
                           size_t p_data_size)
{
    // NSVG expects a NUL terminated string and then mutates the string you give it so
    // we allocate a copy of the input data one byte bigger and explicitly NUL
    // terminate.
    
    MCAutoBlock<char> t_data_cstring;
    if (!t_data_cstring . Allocate(p_data_size + 1))
        return;
    
    MCMemoryCopy(*t_data_cstring, p_data, p_data_size);
    (*t_data_cstring)[p_data_size] = '\0';
    
    // NSVG will take the NUL terminated string and parse the content, hopefully
    // producing an NSVGimage.
    
    self -> image = nsvgParse(*t_data_cstring, "", 1);
    if (self -> image == NULL)
        return;
    
    __nsvgMeasure(self -> image, self -> bounding_box);
    
    self -> is_valid = true;
}

static void __MCGSvgDestroy(MCGSvgRef self)
{
    nsvgDelete(self -> image);
    MCMemoryDelete(self);
}

static bool __MCGSvgIsValid(MCGSvgRef self)
{
    if (self == NULL)
        return false;
    
    return self -> is_valid;
}

static void __MCGSvgInvalidate(MCGSvgRef self)
{
    self -> is_valid = false;
}

////////////////////////////////////////////////////////////////////////////////////////

typedef void (*MCGContextSetGradientProc)(MCGContextRef self,
                                          MCGGradientFunction p_function,
                                          const MCGFloat* p_stops,
                                          const MCGColor* p_colors,
                                          uindex_t p_ramp_length,
                                          bool p_mirror,
                                          bool p_wrap,
                                          uint32_t p_repeats,
                                          MCGAffineTransform p_transform,
                                          MCGImageFilter p_filter);

static bool __MCGSvgApplyGradient(char p_gradient_type,
                                  NSVGgradient *p_gradient,
                                  MCGContextRef p_context,
                                  MCGContextSetGradientProc p_setter)
{
    MCGFloat *t_stops;
    if (!MCMemoryNewArray((unsigned)p_gradient -> nstops, t_stops))
        return false;
    
    MCGColor *t_colors;
    if (!MCMemoryNewArray((unsigned)p_gradient -> nstops, t_colors))
    {
        MCMemoryDeleteArray(t_stops);
        return false;
    }
    
    for(int i = 0; i < p_gradient -> nstops; i++)
    {
        t_stops[i] = p_gradient -> stops[i] . offset;
        t_colors[i] = MCGColorMakeRGBA((p_gradient -> stops[i] . color & 0xFF) / 1.0f,
                                       ((p_gradient -> stops[i] . color >> 8) & 0xFF) / 1.0f,
                                       ((p_gradient -> stops[i] . color >> 16) & 0xFF) / 1.0f,
                                       ((p_gradient -> stops[i] . color >> 24) & 0xFF) / 1.0f);
    }
    
    // NanoSVG gives us the inverse gradient transform for a vertically dominant
    // orientation. Thus we must invert the transform and rotate by 90 degrees
    // anti-clockwise.
    MCGAffineTransform t_transform;
    t_transform = MCGAffineTransformMake(p_gradient -> xform[0], p_gradient -> xform[1],
                                         p_gradient -> xform[2], p_gradient -> xform[3],
                                         p_gradient -> xform[4], p_gradient -> xform[5]);
    t_transform = MCGAffineTransformInvert(t_transform);
    t_transform = MCGAffineTransformPostRotate(t_transform, 90);
    
    p_setter(p_context,
             p_gradient_type == NSVG_PAINT_LINEAR_GRADIENT ?
             kMCGGradientFunctionLinear :
             kMCGGradientFunctionRadial,
             t_stops,
             t_colors,
             (unsigned)p_gradient -> nstops,
             p_gradient -> spread == NSVG_SPREAD_REFLECT ? true : false,
             p_gradient -> spread != NSVG_SPREAD_PAD ? true : false,
             1,
             t_transform,
             kMCGImageFilterNone);
    
    MCMemoryDeleteArray(t_colors);
    MCMemoryDeleteArray(t_stops);
    
    return true;
}

static void __MCGSvgRender(MCGSvgRef self, MCGContextRef p_context)
{
    MCGContextSave(p_context);
    
    MCGContextSetShouldAntialias(p_context, true);
    
    for(NSVGshape *t_shape = self -> image -> shapes; t_shape != NULL; t_shape = t_shape -> next)
    {
        if (t_shape -> opacity == 0.0f)
            continue;
        
        bool t_need_fill;
        t_need_fill = false;
        if (t_shape -> fill . type == NSVG_PAINT_COLOR)
        {
            MCGContextSetFillRGBAColor(p_context,
                                       (t_shape -> fill . color & 0xFF) / 255.0f,
                                       ((t_shape -> fill . color >> 8) & 0xFF) / 255.0f,
                                       ((t_shape -> fill . color >> 16) & 0xFF) / 255.0f,
                                       ((t_shape -> fill . color >> 24) & 0xFF) / 255.0f);
            
            t_need_fill = true;
        }
        else if (t_shape -> fill . type == NSVG_PAINT_LINEAR_GRADIENT ||
                 t_shape -> fill . type == NSVG_PAINT_RADIAL_GRADIENT)
        {
            if (!__MCGSvgApplyGradient(t_shape -> fill . type,
                                       t_shape -> fill . gradient,
                                       p_context,
                                       MCGContextSetFillGradient))
                continue;
            
            t_need_fill = true;
        }
        
        if (t_need_fill)
        {
            MCGContextSetFillRule(p_context,
                                  t_shape -> fillRule == NSVG_FILLRULE_NONZERO ? 
                                  kMCGFillRuleNonZero :
                                  kMCGFillRuleEvenOdd);
        }
        
        bool t_need_stroke;
        t_need_stroke = false;
        if (t_shape -> strokeWidth > 0.0f)
        {
            if (t_shape -> stroke . type == NSVG_PAINT_COLOR)
            {
                MCGContextSetStrokeRGBAColor(p_context,
                                             (t_shape -> stroke . color & 0xFF) / 255.0f,
                                             ((t_shape -> stroke . color >> 8) & 0xFF) / 255.0f,
                                             ((t_shape -> stroke . color >> 16) & 0xFF) / 255.0f,
                                             ((t_shape -> stroke . color >> 24) & 0xFF) / 255.0f);
                
                t_need_stroke = true;
            }
            else if (t_shape -> stroke . type == NSVG_PAINT_LINEAR_GRADIENT ||
                     t_shape -> stroke . type == NSVG_PAINT_RADIAL_GRADIENT)
            {
                if (!__MCGSvgApplyGradient(t_shape -> stroke . type,
                                           t_shape -> stroke . gradient,
                                           p_context,
                                           MCGContextSetStrokeGradient))
                    continue;
                
                t_need_stroke = true;
            }
            
            if (t_need_stroke)
            {
                MCGContextSetStrokeWidth(p_context,
                                         t_shape -> strokeWidth);
                MCGContextSetStrokeJoinStyle(p_context,
                                             t_shape -> strokeLineJoin == NSVG_JOIN_MITER ?
                                             kMCGJoinStyleMiter :
                                             t_shape -> strokeLineJoin == NSVG_JOIN_ROUND ?
                                             kMCGJoinStyleRound :
                                             kMCGJoinStyleBevel);
                MCGContextSetStrokeCapStyle(p_context,
                                            t_shape -> strokeLineCap == NSVG_CAP_BUTT ?
                                            kMCGCapStyleButt :
                                            t_shape -> strokeLineCap == NSVG_CAP_ROUND ?
                                            kMCGCapStyleRound :
                                            kMCGCapStyleSquare);
            }
        }
        
        if (!t_need_fill && !t_need_stroke)
            continue;
        
        MCGContextSetOpacity(p_context, t_shape -> opacity);
        
        // First build a path (as it might get reused).
        MCGContextBeginPath(p_context);
        for(NSVGpath *t_subpath = t_shape -> paths; t_subpath != NULL; t_subpath = t_subpath -> next)
        {
            float *t_ords;
            t_ords = t_subpath -> pts;
            
            MCGContextMoveTo(p_context,
                             MCGPointMake(t_ords[0], t_ords[1]));
            t_ords += 2;
            
            for(int i = 0; i < t_subpath -> npts - 1; i += 3)
            {
                MCGContextCubicTo(p_context,
                                  MCGPointMake(t_ords[0], t_ords[1]),
                                  MCGPointMake(t_ords[2], t_ords[3]),
                                  MCGPointMake(t_ords[4], t_ords[5]));
                t_ords += 6;
            }
            
            if (t_subpath -> closed)
                MCGContextCloseSubpath(p_context);
        }
        
        if (t_need_fill && t_need_stroke)
            MCGContextFillAndStroke(p_context);
        else if (t_need_stroke)
            MCGContextStroke(p_context);
        else
            MCGContextFill(p_context);
    }
    
    MCGContextRestore(p_context);
}

////////////////////////////////////////////////////////////////////////////////////////
