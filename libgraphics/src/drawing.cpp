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

/******************************************************************************/

inline MCGPoint MCGPointReflect(const MCGPoint &origin, const MCGPoint &point)
{
	return MCGPointMake(origin.x - (point.x - origin.x), origin.y - (point.y - origin.y));
}

/*******************************************************************************
 *
 * DRAWING FORMAT - VERSION 0
 *
 * A drawing is a simple bytecode format for representing graphics expressed
 * using SVG. It consists of a header, followed by a sequence of bytecodes
 * encoded in bytes, and a sequence of scalars encoded as floats.
 *
 * The format is such that it requires no extra data structures at runtime
 * except whilst rendering.
 *
 * The header has the following format:
 *   uint8_t[4] ident = 'L', 'C', 'D', 0
 *   uint32_t flags
 *     has_width : bit 0
 *     has_height : bit 1
 *     has_viewport : bit 2
 *   uint32_t scalar_count
 *   float[scalar_count] scalars
 *   uint32_t opcode_count
 *   uint8_t[opcode_count] opcodes
 *
 * The ident field is used to identify a drawing, allowing simple 'sniffing' of
 * the first four bytes to help determine the format.
 *
 * The last byte of the ident field is the version of the format - currently 0.
 * This will be incremented each time the format changes in a backwards
 * incompatible way.
 *
 * The has_width flag determines whether the drawing has specified an intrinsic
 * width != 100% (i.e. width of container). If set to true, then the width
 * will be available as a size scalar.
 *
 * The has_height flag determines whether the drawing has specified an intrinsic
 * height != 100% (i.e. height of container). If set to true, then the height
 * will be available as a size scalar.
 *
 * The has_viewport flag determines whether the drawing has specified a viewbox
 * and preserve aspect ratio setting. If set to true, then the viewbox will be
 * available as two coordinate scalars, followed by two length scalars, and the
 * aspect ratio setting will be available as a PreserveAspectRatio opcode.
 *
 * There are several kinds of scalar which may be present:
 *
 *   - coordinate : any scalar value
 *
 *   - unit : a value between 0 and 1
 *
 *   - length : a value greater or equal to 0
 *
 *   - positive : a value greater or equal to 1
 *
 *   - size : a value greater or equal to -1; values less than 0 represent
 *     a percentage (divided by -100), values greater or equal to 0 represent
 *     absolute values.
 *
 */

/* MCGSolidColor represents a rgba color value. */
typedef MCGColor4f MCGDrawingSolidColor;

struct MCGDrawingRamp
{
    ssize_t count;
    const MCGDrawingSolidColor *colors;
    const MCGFloat *offsets;
};

struct MCGDrawingGradient
{
    MCGGradientSpreadMethod spread;
    MCGAffineTransform transform;
    MCGDrawingRamp ramp;
};

struct MCGDrawingLinearGradient: public MCGDrawingGradient
{
};

struct MCGDrawingRadialGradient: public MCGDrawingGradient
{
};

struct MCGDrawingConicalGradient: public MCGDrawingGradient
{
    MCGPoint focal_point;
    MCGFloat focal_radius;
};

enum MCGDrawingPaintType
{
    kMCGDrawingPaintTypeNone,
    kMCGDrawingPaintTypeSolidColor,
    kMCGDrawingPaintTypeLinearGradient,
    kMCGDrawingPaintTypeRadialGradient,
    kMCGDrawingPaintTypeConicalGradient,
};

struct MCGDrawingPaint
{
    MCGDrawingPaintType type;
    union
    {
        MCGDrawingSolidColor solid_color;
        MCGDrawingGradient gradient;
        MCGDrawingLinearGradient linear_gradient;
        MCGDrawingRadialGradient radial_gradient;
        MCGDrawingConicalGradient conical_gradient;
    };
    
    MCGDrawingPaint(void)
        : type(kMCGDrawingPaintTypeNone)
    {
    }
};

/* Constants describing the first four bytes of a drawing */
enum : uint8_t
{
    kMCGDrawingIdent0 = 'L',
    kMCGDrawingIdent1 = 'C',
    kMCGDrawingIdent2 = 'D',
    kMCGDrawingVersion = 0,
};

/* MCGDrawingHeaderFlags is a bit set indicating which 'initialization' fields
 * are present in the drawing. */
enum MCGDrawingHeaderFlags : uint32_t
{
    /* The drawing has a width specified which is not 100% */
    kMCGDrawingHeaderFlagHasWidthBit = 1 << 0,
    
    /* The drawing has a height specified which is not 100% */
    kMCGDrawingHeaderFlagHasHeightBit = 1 << 1,
    
    /* The drawing has a viewport specified (viewBox and preserveAspectRatio) */
    kMCGDrawingHeaderFlagHasViewportBit = 1 << 2,
    
    /* The mask to use to check no undefined flags are set. */
    kMCGDrawingHeaderFlagValidMask = 0x07,
};

/* MCGDrawingHeader defines the layout of a drawing's header.
 *
 * Note: The definition is partial as the scalar_count and opcode_count are
 * not fixed. */
struct MCGDrawingHeader
{
    uint8_t ident[4];
    uint32_t flags;
    uint32_t scalar_count;
//  float[scalar_count] scalars;
//  uint32_t opcode_count;
//  uint8_t[opcode_count] opcodes;
};

/* MAIN OPCODES */

/* MCGDrawingOpcode defines the main opcodes which appear in the opcode stream.
 */
enum MCGDrawingOpcode : uint8_t
{
    /* End defines the end of an opcode stream */
    kMCGDrawingOpcodeEnd = 0,
    
    /* Transform defines a transform attribute change. The transform is defined
     * by a following transform opcode stream. */
    kMCGDrawingOpcodeTransform = 1,
    
    /* FillPaint defines a fill-paint attribute change. The paint is defined by
     * a following paint opcode stream. */
    kMCGDrawingOpcodeFillPaint = 2,
    
    /* FillOpacity defines a fill-opacity attribute change. The opacity is
     * defined by a unit scalar. */
    kMCGDrawingOpcodeFillOpacity = 3,
    
    /* FillRule defines a fill-rule attribute change. The fill rule is defined
     * by a FillRule opcode. */
    kMCGDrawingOpcodeFillRule = 4,
    
    /* StrokePaint defines a stroke-paint attribute change. The paint is defined
     * by a following paint opcode stream. */
    kMCGDrawingOpcodeStrokePaint = 5,
    
    /* StrokeOpacity defines a stroke-opacity attribute change. The opacity is
     * defined by a unit length scalar. */
    kMCGDrawingOpcodeStrokeOpacity = 6,
    
    /* StrokeWidth defines a stroke-width attribute change. The stroke-width is
     * defined by a length scalar. */
    kMCGDrawingOpcodeStrokeWidth = 7,
    
    /* StrokeLineJoin defines a stroke-line-join attribute change. The
     * stroke-line-join is defined by a StrokeLineJoin opcode. */
    kMCGDrawingOpcodeStrokeLineJoin = 8,
    
    /* StrokeLineCap defines a stroke-line-cap attribute change. The
     * stroke-line-cap is defined by a StrokeLineCap opcode. */
    kMCGDrawingOpcodeStrokeLineCap = 9,
    
    /* StrokeDashArray defines a stroke-dash-array attribute change. The
     * stroke-dash-array is defined by a length scalar array. */
    kMCGDrawingOpcodeStrokeDashArray = 10,
    
    /* StrokeDashOffset defines a stroke-dash-offset attribute change. The
     * stroke-dash-offset is defined by a length scalar. */
    kMCGDrawingOpcodeStrokeDashOffset = 11,
    
    /* StrokeMiterLimit defines a stroke-miter-limit attribute change. The
     * stroke-miter-limit is defined by a positive length scalar. */
    kMCGDrawingOpcodeStrokeMiterLimit = 12,
    
    /* Rectangle defines a rectangle shape. The rectangle is defined as:
     *   x, y: coordinate scalars
     *   width, height: length scalars
     *   rx, ry: length scalars */
    kMCGDrawingOpcodeRectangle = 13,
    
    /* Circle defines a circle shape. The circle is defined as:
     *   x, y: coordinate scalars
     *   r: length scalar */
    kMCGDrawingOpcodeCircle = 14,
    
    /* Ellipse defines an ellipse shape. The ellipse is defined as:
     *   x, y: coordinate scalars
     *   rx, ry: length scalars */
    kMCGDrawingOpcodeEllipse = 15,
    
    /* Line defines a line shape. The line is defined as:
     *   x1, y1: coordinate scalars
     *   x2, y2: coordinate scalars */
    kMCGDrawingOpcodeLine = 16,
    
    /* Polyline defines a polyline shape. The polyline is defined as an array
     * of coordinate scalar pairs. */
    kMCGDrawingOpcodePolyline = 17,
    
    /* Polyline defines a polygon shape. The polygon is defined as an array
     * of coordinate scalar pairs. */
    kMCGDrawingOpcodePolygon = 18,
    
    /* Path defines a path shape. The path is defined by a path opcode stream.
     */
    kMCGDrawingOpcodePath = 19,
    
    /* _Last is used for range checking on the main opcodes. */
    kMCGDrawingOpcode_Last = kMCGDrawingOpcodePath,
};

/* PRESERVE ASPECT RATIO OPCODES */

/* MCGDrawingPreserveAspectRatioOpcode defines the settings of the preserve-
 * aspect-ratio attribute. */
enum MCGDrawingPreserveAspectRatioOpcode : uint8_t
{
    kMCGDrawingPreserveAspectRatioOpcodeNone = 0,
    kMCGDrawingPreserveAspectRatioOpcodeXMinYMinMeet,
    kMCGDrawingPreserveAspectRatioOpcodeXMidYMinMeet,
    kMCGDrawingPreserveAspectRatioOpcodeXMaxYMinMeet,
    kMCGDrawingPreserveAspectRatioOpcodeXMinYMidMeet,
    kMCGDrawingPreserveAspectRatioOpcodeXMidYMidMeet,
    kMCGDrawingPreserveAspectRatioOpcodeXMaxYMidMeet,
    kMCGDrawingPreserveAspectRatioOpcodeXMinYMaxMeet,
    kMCGDrawingPreserveAspectRatioOpcodeXMidYMaxMeet,
    kMCGDrawingPreserveAspectRatioOpcodeXMaxYMaxMeet,
    kMCGDrawingPreserveAspectRatioOpcodeXMinYMinSlice,
    kMCGDrawingPreserveAspectRatioOpcodeXMidYMinSlice,
    kMCGDrawingPreserveAspectRatioOpcodeXMaxYMinSlice,
    kMCGDrawingPreserveAspectRatioOpcodeXMinYMidSlice,
    kMCGDrawingPreserveAspectRatioOpcodeXMidYMidSlice,
    kMCGDrawingPreserveAspectRatioOpcodeXMaxYMidSlice,
    kMCGDrawingPreserveAspectRatioOpcodeXMinYMaxSlice,
    kMCGDrawingPreserveAspectRatioOpcodeXMidYMaxSlice,
    kMCGDrawingPreserveAspectRatioOpcodeXMaxYMaxSlice,
    
    kMCGDrawingPreserveAspectRatioOpcode_Last = kMCGDrawingPreserveAspectRatioOpcodeXMaxYMaxSlice,
};

inline bool MCGDrawingPreserveAspectRatioIsMeet(MCGDrawingPreserveAspectRatioOpcode o)
{
    return o >= kMCGDrawingPreserveAspectRatioOpcodeXMinYMinMeet &&
           o <= kMCGDrawingPreserveAspectRatioOpcodeXMaxYMaxMeet;
}

inline bool MCGDrawingPreserveAspectRatioIsSlice(MCGDrawingPreserveAspectRatioOpcode o)
{
    return o >= kMCGDrawingPreserveAspectRatioOpcodeXMinYMinSlice &&
           o <= kMCGDrawingPreserveAspectRatioOpcodeXMaxYMaxSlice;
}

inline bool MCGDrawingPreserveAspectRatioIsXMid(MCGDrawingPreserveAspectRatioOpcode o)
{
    switch(o)
    {
    case kMCGDrawingPreserveAspectRatioOpcodeXMidYMinMeet:
    case kMCGDrawingPreserveAspectRatioOpcodeXMidYMidMeet:
    case kMCGDrawingPreserveAspectRatioOpcodeXMidYMaxMeet:
    case kMCGDrawingPreserveAspectRatioOpcodeXMidYMinSlice:
    case kMCGDrawingPreserveAspectRatioOpcodeXMidYMidSlice:
    case kMCGDrawingPreserveAspectRatioOpcodeXMidYMaxSlice:
        return true;
    default:
        break;
    }
    
    return false;
}

inline bool MCGDrawingPreserveAspectRatioIsXMax(MCGDrawingPreserveAspectRatioOpcode o)
{
    switch(o)
    {
    case kMCGDrawingPreserveAspectRatioOpcodeXMaxYMinMeet:
    case kMCGDrawingPreserveAspectRatioOpcodeXMaxYMidMeet:
    case kMCGDrawingPreserveAspectRatioOpcodeXMaxYMaxMeet:
    case kMCGDrawingPreserveAspectRatioOpcodeXMaxYMinSlice:
    case kMCGDrawingPreserveAspectRatioOpcodeXMaxYMidSlice:
    case kMCGDrawingPreserveAspectRatioOpcodeXMaxYMaxSlice:
        return true;
    default:
        break;
    }
    
    return false;
}

inline bool MCGDrawingPreserveAspectRatioIsYMid(MCGDrawingPreserveAspectRatioOpcode o)
{
    switch(o)
    {
    case kMCGDrawingPreserveAspectRatioOpcodeXMinYMidMeet:
    case kMCGDrawingPreserveAspectRatioOpcodeXMidYMidMeet:
    case kMCGDrawingPreserveAspectRatioOpcodeXMaxYMidMeet:
    case kMCGDrawingPreserveAspectRatioOpcodeXMinYMidSlice:
    case kMCGDrawingPreserveAspectRatioOpcodeXMidYMidSlice:
    case kMCGDrawingPreserveAspectRatioOpcodeXMaxYMidSlice:
        return true;
    default:
        break;
    }
    
    return false;
}

inline bool MCGDrawingPreserveAspectRatioIsYMax(MCGDrawingPreserveAspectRatioOpcode o)
{
    switch(o)
    {
    case kMCGDrawingPreserveAspectRatioOpcodeXMinYMaxMeet:
    case kMCGDrawingPreserveAspectRatioOpcodeXMidYMaxMeet:
    case kMCGDrawingPreserveAspectRatioOpcodeXMaxYMaxMeet:
    case kMCGDrawingPreserveAspectRatioOpcodeXMinYMaxSlice:
    case kMCGDrawingPreserveAspectRatioOpcodeXMidYMaxSlice:
    case kMCGDrawingPreserveAspectRatioOpcodeXMaxYMaxSlice:
        return true;
    default:
        break;
    }
    
    return false;
}

/* TRANSFORM OPCODES */

/* MCGDrawingTransformOpcode defines the opcodes used to build a transform. The
 * sequence of specified transforms is concatenated with the identity transform
 * to build a transformation matrix. */
enum MCGDrawingTransformOpcode : uint8_t
{
    /* End terminates a sequence of concatenated transforms */
    kMCGDrawingTransformOpcodeEnd = 0,
    
    /* Affine defines a general affine transformation. It is defined by 6
     * scalars - a b c d tx ty. */
    kMCGDrawingTransformOpcodeAffine = 1,
    
    /* _Last is used for range checking on the transform opcodes. */
    kMCGDrawingTransformOpcode_Last = kMCGDrawingTransformOpcodeAffine,
};

/* PAINT OPCODES */

/* MCGDrawingPaintOpcode defines the opcodes used to build a paint. Currently
 * only a single paint is allowed, but in the future multiple paints will be
 * allowed (resulting in the shape being drawn with each paint in turn) to
 * align with that feature in the SVG2 spec. */
enum MCGDrawingPaintOpcode : uint8_t
{
    /* End terminates a sequence of overlaid paints. */
    kMCGDrawingPaintOpcodeEnd = 0,
    
    /* SolidColor defines a solid color paint. It is defined by 4 unit scalars -
     * red green blue alpha. */
    kMCGDrawingPaintOpcodeSolidColor = 1,
    
    /* SolidLinearGradient defines a linear gradient paint. It is defined by:
     *   spread method : spread method opcode
     *   transform : transform opcode stream
     *   count : ramp length
     *   colors : count sets of 4 unit scalars - red green blue alpha
     *   stops : count array of unit scalars
     */
    kMCGDrawingPaintOpcodeLinearGradient = 2,
    
    kMCGDrawingPaintOpcodeRadialGradient = 3,
    
    kMCGDrawingPaintOpcodeConicalGradient = 4,
    
    /* _Last is used for range checking on the paint opcodes. */
    kMCGDrawingPaintOpcode_Last = kMCGDrawingPaintOpcodeConicalGradient,
};

/* SPREAD METHOD OPCODES */

/* MCGDrawingSpreadMethodOpcode defines the opcodes used to specify the spread
 * method of a gradient. */
enum MCGDrawingSpreadMethodOpcode : uint8_t
{
    kMCGDrawingSpreadMethodOpcodePad = 0,
    kMCGDrawingSpreadMethodOpcodeReflect = 1,
    kMCGDrawingSpreadMethodOpcodeRepeat = 2,
    
    kMCGDrawingSpreadMethodOpcode_Last = kMCGDrawingSpreadMethodOpcodeRepeat,
};

/* FILL RULE OPCODES */

/* MCGDrawingFillRuleOpcode defines the opcodes used to specify the fill-rule
 * attribute. */
enum MCGDrawingFillRuleOpcode : uint8_t
{
    /* NonZero indicates that the non-zero winding rule should be used. */
    kMCGDrawingFillRuleOpcodeNonZero = 0,
    
    /* EvenOdd indicates that the even-odd rule should be used. */
    kMCGDrawingFillRuleOpcodeEvenOdd = 1,
    
    /* _Last is used for range checking on the fill rule opcodes. */
    kMCGDrawingFillRuleOpcode_Last = kMCGDrawingFillRuleOpcodeEvenOdd,
};

/* LINE JOIN OPCODES */

/* MCGDrawingStrokeLineJoinOpcode defines the opcodes used to specify the
 * stroke-line-join attribute. */
enum MCGDrawingStrokeLineJoinOpcode : uint8_t
{
    /* Bevel indicates that the bevel line join should be used. A bevel line
     * join uses a single line segment between the outer points of a join. */
    kMCGDrawingStrokeLineJoinOpcodeBevel = 0,
    
    /* Round indicates that the round line join should be used. A round line
     * join uses a circular arc between the outer points of a join. */
    kMCGDrawingStrokeLineJoinOpcodeRound = 1,
    
    /* Miter indicates that the miter line join should be used. A miter line
     * join extends the outer edges of a join so they meet in a point. Miter
     * joins fallback to a bevel join if the ratio of the angle and size between
     * them exceeds the miter-limit. */
    kMCGDrawingStrokeLineJoinOpcodeMiter = 2,
    
    /* _Last is used for range checking on the line join opcodes. */
    kMCGDrawingStrokeLineJoinOpcode_Last = kMCGDrawingStrokeLineJoinOpcodeMiter
};

/* LINE CAP OPCODES */

/* MCGDrawingStrokeLineCapOpcode defines the opcodes used to specify the
 * stroke-line-cap attribute. */
enum MCGDrawingStrokeLineCapOpcode : uint8_t
{
    /* Butt indicates that the butt line cap should be used. A butt line cap
     * adds no cap to the ends of stroked lines. */
    kMCGDrawingStrokeLineCapOpcodeButt = 0,
    
    /* Round indicates that the round line cap should be used. A round line cap
     * adds a half circle of diameter the stroke width to the ends of stroked
     * lines. */
    kMCGDrawingStrokeLineCapOpcodeRound = 1,
    
    /* Square indicates that the square line cap should be used. A square line
     * cap adds a half square of size the stroke width to the ends of stroked
     * lines. */
    kMCGDrawingStrokeLineCapOpcodeSquare = 2,
    
    /* _Last is used for range checking on the line cap opcodes. */
    kMCGDrawingStrokeLineCapOpcode_Last = kMCGDrawingStrokeLineCapOpcodeSquare
};

/* PATH OPCODES */

/* MGDrawingPathOpcodes defines the opcodes used to specify a path shape. */
enum MCGDrawingPathOpcode : uint8_t
{
    /* End terminates a path definition. */
    kMCGDrawingPathOpcodeEnd = 0,
    
    kMCGDrawingPathOpcodeMoveTo = 1,
    kMCGDrawingPathOpcodeRelativeMoveTo = 2,
    kMCGDrawingPathOpcodeLineTo = 3,
    kMCGDrawingPathOpcodeRelativeLineTo = 4,
    kMCGDrawingPathOpcodeHorizontalTo = 5,
    kMCGDrawingPathOpcodeRelativeHorizontalTo = 6,
    kMCGDrawingPathOpcodeVerticalTo = 7,
    kMCGDrawingPathOpcodeRelativeVerticalTo = 8,
    kMCGDrawingPathOpcodeCubicTo = 9,
    kMCGDrawingPathOpcodeRelativeCubicTo = 10,
    kMCGDrawingPathOpcodeSmoothCubicTo = 11,
    kMCGDrawingPathOpcodeRelativeSmoothCubicTo = 12,
    kMCGDrawingPathOpcodeQuadraticTo= 13,
    kMCGDrawingPathOpcodeRelativeQuadraticTo = 14,
    kMCGDrawingPathOpcodeSmoothQuadraticTo = 15,
    kMCGDrawingPathOpcodeRelativeSmoothQuadraticTo = 16,
    kMCGDrawingPathOpcodeArcTo = 17,
    kMCGDrawingPathOpcodeRelativeArcTo = 18,
    kMCGDrawingPathOpcodeReflexArcTo = 19,
    kMCGDrawingPathOpcodeRelativeReflexArcTo = 20,
    kMCGDrawingPathOpcodeReverseArcTo = 21,
    kMCGDrawingPathOpcodeRelativeReverseArcTo = 22,
    kMCGDrawingPathOpcodeReverseReflexArcTo = 23,
    kMCGDrawingPathOpcodeRelativeReverseReflexArcTo = 24,
    kMCGDrawingPathOpcodeCloseSubpath = 25,
    
    kMCGDrawingPathOpcode_Last = kMCGDrawingPathOpcodeCloseSubpath,
};

inline bool MCGDrawingPathOpcodeIsRelativeArcTo(MCGDrawingPathOpcode p_opcode)
{
    switch(p_opcode)
    {
    case kMCGDrawingPathOpcodeRelativeArcTo:
    case kMCGDrawingPathOpcodeRelativeReflexArcTo:
    case kMCGDrawingPathOpcodeRelativeReverseArcTo:
    case kMCGDrawingPathOpcodeRelativeReverseReflexArcTo:
        return true;
    default:
        break;
    }
    return false;
}

inline bool MCGDrawingPathOpcodeIsReflexArcTo(MCGDrawingPathOpcode p_opcode)
{
    switch(p_opcode)
    {
    case kMCGDrawingPathOpcodeReflexArcTo:
    case kMCGDrawingPathOpcodeRelativeReflexArcTo:
    case kMCGDrawingPathOpcodeReverseReflexArcTo:
    case kMCGDrawingPathOpcodeRelativeReverseReflexArcTo:
        return true;
    default:
        break;
    }
    return false;
}

inline bool MCGDrawingPathOpcodeIsReverseArcTo(MCGDrawingPathOpcode p_opcode)
{
    switch(p_opcode)
    {
    case kMCGDrawingPathOpcodeReverseArcTo:
    case kMCGDrawingPathOpcodeRelativeReverseArcTo:
    case kMCGDrawingPathOpcodeReverseReflexArcTo:
    case kMCGDrawingPathOpcodeRelativeReverseReflexArcTo:
        return true;
    default:
        break;
    }
    return false;
}

/**/

/* MCGDrawingVisitor is the interface expected from the Visitor type used by
 * the Execute methods in MCGDrawingContext. */
struct MCGDrawingVisitor
{
    /* Path Operations */
    
    void PathBegin(void);
    void PathMoveTo(bool is_relative, MCGPoint p_point);
    void PathLineTo(bool is_relative, MCGPoint p_point);
    void PathHorizontalTo(bool is_relative, MCGFloat x);
    void PathVerticalTo(bool is_relative, MCGFloat y);
    void PathCubicTo(bool is_relative, MCGPoint p_a, MCGPoint p_b, MCGPoint p_point);
    void PathSmoothCubicTo(bool is_relative, MCGPoint p_b, MCGPoint p_point);
    void PathQuadraticTo(bool is_relative, MCGPoint p_a, MCGPoint p_point);
    void PathSmoothQuadraticTo(bool is_relative, MCGPoint p_point);
    void PathArcTo(bool is_relative, bool is_reflex, bool is_reverse, MCGPoint p_center, MCGFloat rotation, MCGPoint p_point);
    void PathCloseSubpath(void);
    void PathEnd(void);
    
    /* General Attribute Operations */
    
    void Transform(MCGAffineTransform transform);
    
    /* Fill Attribute Operations */
    
    void FillPaint(const MCGDrawingPaint& paint);
    void FillOpacity(MCGFloat opacity);
    void FillRule(MCGFillRule fill_rule);
   
    /* Stroke Attribute Operations */
    
    void StrokePaint(const MCGDrawingPaint& paint);
    void StrokeOpacity(MCGFloat opacity);
    void StrokeWidth(MCGFloat width);
    void StrokeLineJoin(MCGJoinStyle join_style);
    void StrokeLineCap(MCGCapStyle cap_style);
    void StrokeDashArray(MCSpan<const MCGFloat> lengths);
    void StrokeDashOffset(MCGFloat offset);
    void StrokeMiterLimit(MCGFloat miter_limit);
    
    /* Shape Operations */
    
    void Rectangle(MCGRectangle p_bounds, MCGPoint p_radii);
    void Circle(MCGPoint p_center, MCGFloat r);
    void Ellipse(MCGPoint p_center, MCGSize p_radii);
    void Line(MCGPoint p_from, MCGPoint p_to);
    void Polyline(MCSpan<const MCGPoint> p_points);
    void Polygon(MCSpan<const MCGPoint> p_points);
    
    /* Lifecycle Operations */

    void Start(void);
    void Finish(bool p_error);
};

/* DRAWING CONTEXT */

/* MCGDrawingStatus is used to indicate the current status of playback of a
 * drawing. */
enum MCGDrawingStatus
{
    /* None indicates that there are no problems with the drawing so far. */
    kMCGDrawingStatusNone,
    
    /* InvalidDrawing indicates that the drawing data is malformed and cannot
     * be unpacked. */
    kMCGDrawingStatusInvalidDrawing,
    
    /* InvalidIdent indicates that the 3 main ident bytes do not match 'LCD'. */
    kMCGDrawingStatusInvalidIdent,
    
    /* InvalidVersion indicates that the version is not recognised. */
    kMCGDrawingStatusInvalidVersion,
    
    /* InvalidFlags indicates that flags which are not defined have been set. */
    kMCGDrawingStatusInvalidFlags,
    
    /* InvalidWidth indicates that the width scalar could not be unpacked. */
    kMCGDrawingStatusInvalidWidth,
    
    /* InvalidHeight indicates that the height scalar could not be unpacked. */
    kMCGDrawingStatusInvalidHeight,
    
    /* InvalidViewport indicates that the viewport (viewBox and preserveAspectRatio)
     * could not be unpacked. */
    kMCGDrawingStatusInvalidViewport,
    
    /* ScalarOverflow indicates that more scalars are required than are present.
     */
    kMCGDrawingStatusScalarOverflow,
    
    /* OpcodeOverflow indicates that more opcodes are required than are present.
     */
    kMCGDrawingStatusOpcodeOverflow,
    
    /* InvalidOpcode indicates that an undefined main opcode has been
     * encountered. */
    kMCGDrawingStatusInvalidOpcode,
    
    /* InvalidPathOpcode indicates that an undefined path opcode has been
     * encountered. */
    kMCGDrawingStatusInvalidPathOpcode
    ,
    /* InvalidPaintOpcode indicates that an undefined paint opcode has been
     * encountered. */
    kMCGDrawingStatusInvalidPaintOpcode,
    
    /* InvalidTransformOpcode indicates that an undefined transform opcode has
     * been encountered. */
    kMCGDrawingStatusInvalidTransformOpcode,
    
    /* InvalidFillRuleOpcode indicates that an undefined fill rule opcode has
     * been encountered. */
    kMCGDrawingStatusInvalidFillRuleOpcode,
    
    /* InvalidStrokeLineJoinOpcode indicates that an undefined line join opcode
     * has been encountered. */
    kMCGDrawingStatusInvalidStrokeLineJoinOpcode,
    
    /* InvalidStrokeLineCapOpcode indicates that an undefined line cap opcode
     * has been encountered. */
    kMCGDrawingStatusInvalidStrokeLineCapOpcode,
    
    /* InvalidPreserveAspectRatioOpcode indicates that an undefined preserve
     * aspect ratio opcode has been encountered. */
    kMCGDrawingStatusInvalidPreserveAspectRatioOpcode,
    
    /* InvalidSpreadMethodOpcode indicates that an undefined spread method
     * opcode has been encountered. */
    kMCGDrawingStatusInvalidSpreadMethodOpcode,
    
    /* InvalidPointArray indicates that a point array with an odd number of
     * scalars has been encountered. */
    kMCGDrawingStatusInvalidPointArray,
};

/* MCGDrawingByteStream is a simple class which wraps a byte array, allowing
 * decoding of individual and sequences of specific types. It is used to unpack
 * the drawing header. */
class MCGDrawingByteStream
{
public:
    MCGDrawingByteStream(MCSpan<const byte_t> p_bytes)
        : m_bytes(p_bytes)
    {
    }

    /* Returns true if there are no more bytes to process. */
    bool IsFinished(void) const
    {
        return m_bytes.empty();
    }

    /* Singleton attempts to unpack a single field of type T. It returns true
     * if successful, or false if there are not enough bytes (sizeof(T)). */
    template<typename T>
    bool Singleton(T& r_singleton)
    {
        ssize_t t_singleton_size = sizeof(T);
        
        if (m_bytes.size() < t_singleton_size)
        {
            return false;
        }
        
        auto t_singleton_ptr = reinterpret_cast<const T *>(m_bytes.data());
        r_singleton = *t_singleton_ptr;
        
        m_bytes = m_bytes.subspan(t_singleton_size);
        
        return true;
    }
    
    /* Array attempts to unpack p_count fields of type T. It returns true if
     * successful, or false if there are not enough bytes (sizeof(T) * p_count).
     */
    template<typename T>
    bool Sequence(MCSpan<const T>& r_sequence)
    {
        uint32_t t_size;
        if (!Singleton(t_size))
        {
            return false;
        }
        
        ssize_t t_byte_size = t_size * sizeof(T);
        
        if (m_bytes.size() < t_byte_size)
        {
            return false;
        }
        
        auto t_seq_ptr = reinterpret_cast<const T *>(m_bytes.data());
        
        r_sequence = MCMakeSpan(t_seq_ptr, t_size);
        
        m_bytes = m_bytes.subspan(t_byte_size);
        
        return true;
    }

private:
    MCSpan<const byte_t> m_bytes;
};

/* MCGDrawingContext is a class which implements a visitor pattern over a
 * drawing. It unpacks the header on construction, and then allows execution
 * of the drawing by calling 'Execute' and passing a suitably defined Visitor
 * type. */
class MCGDrawingContext
{
public:
    /* MCGDrawingContext is the main constructor. It attempts to unpack the
     * drawing header, and sets up for main execution. If an error occurs
     * with setting up, the status field is set appropriately. */
    MCGDrawingContext(MCSpan<const byte_t> p_bytes)
    {
        MCGDrawingByteStream t_stream(p_bytes);
        
        /* Unpack the header. If the size of the provided data does not match
         * what is expected by unpacking the data, then it is an
         * 'InvalidDrawing' */
        uint8_t t_ident_0, t_ident_1, t_ident_2, t_ident_version;
        uint32_t t_flags;
        if (!t_stream.Singleton(t_ident_0) ||
            !t_stream.Singleton(t_ident_1) ||
            !t_stream.Singleton(t_ident_2) ||
            !t_stream.Singleton(t_ident_version) ||
            !t_stream.Singleton(t_flags) ||
            !t_stream.Sequence(m_scalars) ||
            !t_stream.Sequence(m_opcodes) ||
            !t_stream.IsFinished())
        {
            m_status = kMCGDrawingStatusInvalidDrawing;
            return;
        }
        
        /* Check the ident field is correct. */
        if (t_ident_0 != kMCGDrawingIdent0 ||
            t_ident_1 != kMCGDrawingIdent1 ||
            t_ident_2 != kMCGDrawingIdent2)
        {
            m_status = kMCGDrawingStatusInvalidIdent;
            return;
        }
        
        /* Check the version field is correct. */
        if (t_ident_version != kMCGDrawingVersion)
        {
            m_status = kMCGDrawingStatusInvalidVersion;
            return;
        }
        
        /* Check the flags field is correct. */
        if ((t_flags & ~kMCGDrawingHeaderFlagValidMask) != 0)
        {
            m_status = kMCGDrawingStatusInvalidFlags;
            return;
        }
        
        /* Process the width, if present. If the width is not present, then
         * it is taken to be '100%' (-1). */
        if ((t_flags & kMCGDrawingHeaderFlagHasWidthBit) != 0)
        {
            if (!Scalar(m_width))
            {
                m_status = kMCGDrawingStatusInvalidWidth;
                return;
            }
        }
        
        /* Process the height, if present. If the height is not present, then
         * it is taken to be '100%' (-1). */
        if ((t_flags & kMCGDrawingHeaderFlagHasHeightBit) != 0)
        {
            if (!Scalar(m_height))
            {
                m_status = kMCGDrawingStatusInvalidHeight;
                return;
            }
        }
        
        /* Process the viewport, if present. */
        if ((t_flags & kMCGDrawingHeaderFlagHasViewportBit) != 0)
        {
            if (!Rectangle(m_viewbox) ||
                !PreserveAspectRatioOpcode(m_preserve_aspect_ratio))
            {
                m_status = kMCGDrawingStatusInvalidViewport;
                return;
            }
            
            m_has_viewport = true;
        }
    }
    
    /* IsRunning() returns true if the execution has encountered no problems.
     */
    bool IsRunning(void) const
    {
        return m_status == kMCGDrawingStatusNone;
    }
    
    /* Opcode attempts to read the next opcode as a main opcode. */
    bool Opcode(MCGDrawingOpcode& r_opcode)
    {
        return GeneralOpcode<MCGDrawingOpcode,
                             kMCGDrawingOpcode_Last,
                             kMCGDrawingStatusInvalidOpcode>(r_opcode);
    }
    
    /* TransformOpcode attempts to read the next opcode as a transform opcode. */
    bool TransformOpcode(MCGDrawingTransformOpcode& r_opcode)
    {
        return GeneralOpcode<MCGDrawingTransformOpcode,
                             kMCGDrawingTransformOpcode_Last,
                             kMCGDrawingStatusInvalidTransformOpcode>(r_opcode);
    }
    
    /* PaintOpcode attempts to read the next opcode as a paint opcode. */
    bool PaintOpcode(MCGDrawingPaintOpcode& r_opcode)
    {
        return GeneralOpcode<MCGDrawingPaintOpcode,
                             kMCGDrawingPaintOpcode_Last,
                             kMCGDrawingStatusInvalidPaintOpcode>(r_opcode);
    }
    
    /* PathOpcode attempts to read the next opcode as a path opcode. */
    bool PathOpcode(MCGDrawingPathOpcode& r_opcode)
    {
        return GeneralOpcode<MCGDrawingPathOpcode,
                             kMCGDrawingPathOpcode_Last,
                             kMCGDrawingStatusInvalidPathOpcode>(r_opcode);
    }
    
    /* FillRuleOpcode attempts to read the next opcode as a fill rule opcode. */
    bool FillRuleOpcode(MCGDrawingFillRuleOpcode& r_opcode)
    {
        return GeneralOpcode<MCGDrawingFillRuleOpcode,
                             kMCGDrawingFillRuleOpcode_Last,
                             kMCGDrawingStatusInvalidFillRuleOpcode>(r_opcode);
    }
    
    /* StrokeLineJoin attempts to read the next opcode as a line join opcode. */
    bool StrokeLineJoinOpcode(MCGDrawingStrokeLineJoinOpcode& r_opcode)
    {
        return GeneralOpcode<MCGDrawingStrokeLineJoinOpcode,
                             kMCGDrawingStrokeLineJoinOpcode_Last,
                             kMCGDrawingStatusInvalidStrokeLineJoinOpcode>(r_opcode);
    }
    
    /* StrokeLineCape attempts to read the next opcode as a line cap opcode. */
    bool StrokeLineCapOpcode(MCGDrawingStrokeLineCapOpcode& r_opcode)
    {
        return GeneralOpcode<MCGDrawingStrokeLineCapOpcode,
                             kMCGDrawingStrokeLineCapOpcode_Last,
                             kMCGDrawingStatusInvalidStrokeLineCapOpcode>(r_opcode);
    }
    
    /* PreserveAspectRatioOpcode attempts to read the next opcode as a 
     * preserve aspect ratio opcode. */
    bool PreserveAspectRatioOpcode(MCGDrawingPreserveAspectRatioOpcode& r_opcode)
    {
        return GeneralOpcode<MCGDrawingPreserveAspectRatioOpcode,
                             kMCGDrawingPreserveAspectRatioOpcode_Last,
                             kMCGDrawingStatusInvalidPreserveAspectRatioOpcode>(r_opcode);
    }
    
    /* SpreadMethodOpcode attempts to read the next opcode as a spread method
     * opcode. */
    bool SpreadMethodOpcode(MCGDrawingSpreadMethodOpcode& r_opcode)
    {
        return GeneralOpcode<MCGDrawingSpreadMethodOpcode,
                             kMCGDrawingSpreadMethodOpcode_Last,
                             kMCGDrawingStatusInvalidSpreadMethodOpcode>(r_opcode);
    }
    
    /* Count attempts to read an non-negative integer from the opcode stream.
     * Non-negative integers are encoded as a sequence of bytes, most
     * significant byte first. The top bit of each byte is used to indicate
     * whether there is another following. */
    bool Count(ssize_t& r_count)
    {
        /* Start the read count at zero, each byte will be accumulated into it
         */
        ssize_t t_count = 0;
    
        /* Opcode bytes are processed until we reach a termination condition. */
        for(;;)
        {
            /* If the end of the opcode stream is reached, then indicate 
             * overflow. */
            if (m_pc == m_opcodes.size())
            {
                m_status = kMCGDrawingStatusOpcodeOverflow;
                return false;
            }
            
            /* Fetch the next limb of the integer and increment the pc. */
            uint8_t t_next_limb = m_opcodes[m_pc++];
            
            /* Accumulate the limb into the current count, making sure to mask
             * out the top bit of the limb. */
            t_count = (t_count << 8) | (t_next_limb & 0x7f);
            
            /* If the top bit of the limb is zero, then the integer is complete.
             */
            if ((t_next_limb & 0x40) == 0)
            {
                break;
            }
        }
    
        /* Return the accumulated integer. */
        r_count = t_count;
    
        return true;
    }
    
    /* Scalar attempts to read the next scalar from the scalar stream. If the
     * end of the scalar stream has been reached, ScalarOverflow is indicated
     * and false is returned. Otherwise, the next scalar is placed in r_scalar
     * and true is returned. */
    bool Scalar(MCGFloat& r_scalar)
    {
        /* If the scalar counter is at the end of the scalar stream, then
         * indicate ScalarOverflow and return false. */
        if (m_sc == m_scalars.size())
        {
            m_status = kMCGDrawingStatusScalarOverflow;
            return false;
        }
        
        /* Place the next scalar in r_scalar and increment the scalar counter. */
        r_scalar = m_scalars[m_sc++];
        
        return true;
    }
    
    /* Scalars attempts to read p_count scalars from the scalar stream. If there
     * are not enough scalars left then ScalarOverflow is indicated and false is
     * returned. Otherwise, the pointer to the base of the scalars is returned
     * in r_scalars and true is returned. */
    bool Scalars(ssize_t p_count, MCSpan<const MCGFloat>& r_scalars)
    {
        /* If there are not enough scalars left to satisfy the request then
         * indicate ScalarOverflow and return false. */
        if (m_sc + p_count > m_scalars.size())
        {
            m_status = kMCGDrawingStatusScalarOverflow;
            return false;
        }
        
        /* Place the base of the scalar array in r_scalars. */
        r_scalars = m_scalars.subspan(m_sc, p_count);
        
        /* Increment the scalar counter */
        m_sc += p_count;
        
        return true;
    }
    
    bool UnitScalars(ssize_t p_count, MCSpan<const MCGFloat>& r_scalars)
    {
        return Scalars(p_count, r_scalars);
    }
    
    /* LengthScalars attempts to read p_count length scalars - a length scalar
     * is non-negative scalar. */
    bool LengthScalars(ssize_t p_count, MCSpan<const MCGFloat>& r_scalars)
    {
        return Scalars(p_count, r_scalars);
    }
    
    /* CoordinateScalars attempts to read p_count coordinate scalars. */
    bool CoordinateScalars(ssize_t p_count, MCSpan<const MCGFloat>& r_scalars)
    {
        return Scalars(p_count, r_scalars);
    }
    
    /* Coordinate scalar attempte to read a single coordinate scalar. */
    bool CoordinateScalar(MCGFloat& r_coord_scalar)
    {
        return Scalar(r_coord_scalar);
    }
    
    /* UnitScalar attempts to read a single unit scalar - a unit scalar is a
     * scalar in the range [0, 1]. */
    bool UnitScalar(MCGFloat& r_unit_scalar)
    {
        return Scalar(r_unit_scalar);
    }
    
    /* LengthScalar attempts to read a single length scalar. */
    bool LengthScalar(MCGFloat& r_length_scalar)
    {
        return Scalar(r_length_scalar);
    }
    
    /* PositiveScalar attempts to read a single positive scalar - a positive
     * scalar is a scalar in the range [1, +inf). */
    bool PositiveScalar(MCGFloat& r_positive_scalar)
    {
        return Scalar(r_positive_scalar);
    }
    
    /* AngleScalar attempts to read a single angle scalar. */
    bool AngleScalar(MCGFloat& r_angle_scalar)
    {
        return Scalar(r_angle_scalar);
    }
    
    /* Point attempts to read a point defined as two coordinate scalars - x y. */
    bool Point(MCGPoint& r_point)
    {
        if (!CoordinateScalar(r_point.x) ||
            !CoordinateScalar(r_point.y))
        {
            return false;
        }
        return true;
    }
    
    /* Size attempts to read a size defined as two length scalars - rx ry. */
    bool Size(MCGSize& r_size)
    {
        if (!LengthScalar(r_size.width) ||
            !LengthScalar(r_size.height))
        {
            return false;
        }
        return true;
    }
    
    /* AffineTransform attempts to read an transform defined as size scalars. */
    bool AffineTransform(MCGAffineTransform& r_transform)
    {
        if (!Scalar(r_transform.a) ||
            !Scalar(r_transform.b) ||
            !Scalar(r_transform.c) ||
            !Scalar(r_transform.d) ||
            !Scalar(r_transform.tx) ||
            !Scalar(r_transform.ty))
        {
            return false;
        }
        return true;
    }
    
    /* Rectangle attempts to read a rectangle defined as four scalars - x, y
     * width, height. */
    bool Rectangle(MCGRectangle& r_rect)
    {
        if (!CoordinateScalar(r_rect.origin.x) ||
            !CoordinateScalar(r_rect.origin.y) ||
            !LengthScalar(r_rect.size.width) ||
            !LengthScalar(r_rect.size.height))
        {
            return false;
        }
        return true;
    }
    
    /* SolidColor attempts to read a solid color defined as three unit scalars -
     * red, green, blue. */
    bool SolidColor(MCGDrawingSolidColor& r_color)
    {
        if (!UnitScalar(r_color.red) ||
            !UnitScalar(r_color.green) ||
            !UnitScalar(r_color.blue) ||
            !UnitScalar(r_color.alpha))
        {
            return false;
        }
        return true;
    }
    
    /* LengthArray attempts to read an array of scalars, using a count fetched
     * from the opcode stream. */
    bool LengthArray(MCSpan<const MCGFloat>& r_lengths)
    {
        ssize_t t_count;
        if (!Count(t_count))
        {
            return false;
        }
        
        if (!Scalars(t_count, r_lengths))
        {
            return false;
        }
        
        return true;
    }
    
    /* PointArray attempts to read an array of coordinate scalar pairs, using
     * a count fetched from the opcode stream. Currently the count encoded in
     * the opcode stream indicates the number of scalars, not points. If the
     * fetched count is odd, then InvalidPointArray is indicated and false is
     * returned. */
    bool PointArray(MCSpan<const MCGPoint>& r_points)
    {
        /* Read the number of scalars to expect from the opcode stream. */
        ssize_t t_scalar_count;
        if (!Count(t_scalar_count))
        {
            return false;
        }
        
        /* Check that the count is even. */
        if (t_scalar_count % 2 != 0)
        {
            m_status = kMCGDrawingStatusInvalidPointArray;
            return false;
        }
        
        /* Fetch t_scalar_count scalars from the scalar stream. */
        MCSpan<const MCGFloat> t_scalars;
        if (!CoordinateScalars(t_scalar_count, t_scalars))
        {
            return false;
        }
        
        r_points = MCMakeSpan(reinterpret_cast<const MCGPoint*>(t_scalars.data()),
                              t_scalars.size() / 2);
        
        return true;
    }
    
    bool Ramp(MCGDrawingRamp& r_ramp)
    {
        if (!Count(r_ramp.count))
        {
            return false;
        }
        
        MCSpan<const MCGFloat> t_color_scalars;
        if (!UnitScalars(r_ramp.count * 4, t_color_scalars))
        {
            return false;
        }
        r_ramp.colors = reinterpret_cast<const MCGDrawingSolidColor*>(t_color_scalars.data());
        
        MCSpan<const MCGFloat> t_offset_scalars;
        if (!UnitScalars(r_ramp.count, t_offset_scalars))
        {
            return false;
        }
        r_ramp.offsets = t_offset_scalars.data();
        
        return true;
    }
    
    bool Gradient(MCGDrawingGradient& r_gradient)
    {
        MCGDrawingSpreadMethodOpcode t_spread_opcode;
        if (!SpreadMethodOpcode(t_spread_opcode))
        {
            return false;
        }
        switch(t_spread_opcode)
        {
            case kMCGDrawingSpreadMethodOpcodePad:
                r_gradient.spread = kMCGGradientSpreadMethodPad;
                break;
            case kMCGDrawingSpreadMethodOpcodeReflect:
                r_gradient.spread = kMCGGradientSpreadMethodReflect;
                break;
            case kMCGDrawingSpreadMethodOpcodeRepeat:
                r_gradient.spread = kMCGGradientSpreadMethodRepeat;
                break;
        }

        if (!ExecuteTransform(MCGAffineTransformMakeIdentity(),
                              r_gradient.transform))
        {
            return false;
        }
        if (!Ramp(r_gradient.ramp))
        {
            return false;
        }
        
        return true;
    }
    
    /* Execute runs the drawing program using VisitorT to perform the
     * operations. The rect into which the drawing should be 'rendered' is
     * indicated in p_dst_rect. */
    template<typename VisitorT>
    void Execute(VisitorT& visitor, MCGRectangle p_dst_rect);
    
private:
     /* GeneralOpcode reads the next opcode from the opcode stream. If there is
     * no opcode then OpcodeOverflow is flagged and false is returned. The
     * retrieved opcode is checked to ensure it is defined (that is bounded by
     * LastV) and if it is not, then StatusV is flagged and false is returned.
     * Otherwise, the opcode is placed in r_opcode and true is returned. */
    template<typename T, T LastV, MCGDrawingStatus StatusV>
    bool GeneralOpcode(T& r_opcode)
    {
        /* If the program counter is at the end of the opcode stream, then there
         * is no opcode to fetch, so flag Overflow. */
        if (m_pc == m_opcodes.size())
        {
            m_status = kMCGDrawingStatusOpcodeOverflow;
            return false;
        }
        
        /* Fetch the next opcode as opcode type T and advance the program
         * counter by one. */
        T t_opcode = T(m_opcodes[m_pc++]);
        
        /* Check that the opcode is in bounds for the opcode type. */
        if (t_opcode > LastV)
        {
            m_status = StatusV;
            return false;
        }
        
        r_opcode = t_opcode;
        
        return true;
    }

    /* ExecuteTransform runs a transform opcode stream to generate an affine
     * transform. */
    bool ExecuteTransform(MCGAffineTransform p_base_transform, MCGAffineTransform& r_transform);
    
    /* ExecutePaint runs a paint opcode stream to generate a paint struct. */
    bool ExecutePaint(MCGDrawingPaint& r_paint);
    
    /* ExecutePath runs a path opcode stream into the provided
     * visitor. */
    template<typename VisitorT>
    void ExecutePath(VisitorT& visitor);
     
    /**/
    
    /* m_status indicates the current state of the context. If this is not
     * None, then an error has occured. */
    MCGDrawingStatus m_status = kMCGDrawingStatusNone;
    
    /* m_pc indicates the current position in the opcode stream. */
    ssize_t m_pc = 0;
    
    /* m_sc indicates the current position in the scalar stream. */
    ssize_t m_sc = 0;
    
    /* m_opcodes / m_opcode_count define the (const) array of opcodes which are
     * defined by the drawing. */
    MCSpan<const byte_t> m_opcodes;
    
    /* m_scalars / m_scalar_count define the (const) array of scalars which are
     * defined by the drawing. */
    MCSpan<const MCGFloat> m_scalars;
    
    /* m_width is the unpacked width field from the drawing. This is initialized
     * on construction of the context. */
    MCGFloat m_width = -1.0f;
    
    /* m_height is the unpacked height field from the drawing. This is initialized
     * on construction of the context. */
    MCGFloat m_height = -1.0f;
    
    /* m_has_viewport is true if the drawing defined a viewport in the header.
     * This is initialized on construction of the context. */
    bool m_has_viewport = false;
    
    /* m_viewbox is the unpacked viewbox field from the drawing. This is
     * initialized on construction of the context. */
    MCGRectangle m_viewbox = {};
    
    /* m_preserve_aspect_ratio is the unpacked preserve aspect ratio field from
     * the drawing. This is initialized on construction of the context. */
    MCGDrawingPreserveAspectRatioOpcode m_preserve_aspect_ratio = kMCGDrawingPreserveAspectRatioOpcodeNone;
};

/* The (template based) implementation of the Execute method. */
template<typename VisitorT>
void MCGDrawingContext::Execute(VisitorT& p_visitor, MCGRectangle p_viewport)
{
    /* If the drawing's width is negative, then it means the width of the
     * drawing was specified as a percentage (represented as a scalar in the
     * range [-1, 0)) of the requested destination rectangle width. Otherwise
     * the width is absolute. */
    p_viewport.size.width = m_width >= 0 ? m_width : p_viewport.size.width * -m_width;
    
    /* If the drawing's height is negative, then it means the height of the
     * drawing was specified as a percentage (represented as a scalar in the
     * range [-1, 0)) of the requested destination rectangle height. Otherwise
     * the height is absolute. */
    p_viewport.size.height = m_height >= 0 ? m_height : p_viewport.size.height * -m_height;
    
    /* Compute the viewport transform. If there is no viewbox then it is just
     * a translation by the origin of the dst rect. Otherwise it is a translation
     * followed by a scale defined by the viewbox and preserve-aspect-ratio
     * attribute. */
    MCGAffineTransform t_viewport_transform;
    if (!m_has_viewport)
    {
        t_viewport_transform = MCGAffineTransformMakeTranslation(p_viewport.origin.x, p_viewport.origin.y);
    }
    else
    {
        float t_scale_x, t_scale_y;
        if (m_viewbox.size.width != 0)
        {
            t_scale_x = p_viewport.size.width / m_viewbox.size.width;
        }
        else
        {
            t_scale_x = 0;
        }
        
        if (m_viewbox.size.height != 0)
        {
            t_scale_y = p_viewport.size.height / m_viewbox.size.height;
        }
        else
        {
            t_scale_y = 0;
        }
        
        if (MCGDrawingPreserveAspectRatioIsMeet(m_preserve_aspect_ratio))
        {
            t_scale_x = t_scale_y = MCMin(t_scale_x, t_scale_y);
        }
        else if (MCGDrawingPreserveAspectRatioIsSlice(m_preserve_aspect_ratio))
        {
            t_scale_x = t_scale_y = MCMax(t_scale_x, t_scale_y);
        }
        
        float t_translate_x, t_translate_y;
        t_translate_x = p_viewport.origin.x - (m_viewbox.origin.x * t_scale_x);
        t_translate_y = p_viewport.origin.y - (m_viewbox.origin.y * t_scale_y);
        
        if (MCGDrawingPreserveAspectRatioIsXMid(m_preserve_aspect_ratio))
        {
            t_translate_x += (p_viewport.size.width - m_viewbox.size.width * t_scale_x) / 2;
        }
        else if (MCGDrawingPreserveAspectRatioIsXMax(m_preserve_aspect_ratio))
        {
            t_translate_x += (p_viewport.size.width - m_viewbox.size.width * t_scale_x);
        }
        
        if (MCGDrawingPreserveAspectRatioIsYMid(m_preserve_aspect_ratio))
        {
            t_translate_y += (p_viewport.size.height - m_viewbox.size.height * t_scale_y) / 2;
        }
        else if (MCGDrawingPreserveAspectRatioIsYMax(m_preserve_aspect_ratio))
        {
            t_translate_y += (p_viewport.size.height - m_viewbox.size.height * t_scale_y);
        }
        
        t_viewport_transform = MCGAffineTransformMakeTranslation(t_translate_x, t_translate_y);
        t_viewport_transform = MCGAffineTransformConcat(t_viewport_transform,
                                                        MCGAffineTransformMakeScale(t_scale_x, t_scale_y));
    }
    
    /* Start the visitor, passing in the destination rectangle and viewport
     * details so it can configure an initial transform. */
    p_visitor.Start();
    
    p_visitor.Transform(t_viewport_transform);
    
    /* Loop until the status ceases to be None. */
    while(m_status == kMCGDrawingStatusNone)
    {
        /* Fetch the next (main) opcode. */
        MCGDrawingOpcode t_opcode;
        if (!Opcode(t_opcode))
        {
            break;
        }
        
        /* If End is encountered, then the drawing is finished so break. */
        if (t_opcode == kMCGDrawingOpcodeEnd)
        {
            break;
        }
        
        /* Dispatch to the appropriate method for each opcode. */
        switch(t_opcode)
        {
            /* End has already been handled, so this shouldn't happen. */
            case kMCGDrawingOpcodeEnd:
            {
                MCUnreachable();
            }
            break;
               
            /* Transform opcodes require executing a transform opcode stream. */
            case kMCGDrawingOpcodeTransform:
            {
                MCGAffineTransform t_transform;
                if (!ExecuteTransform(t_viewport_transform, t_transform))
                {
                    break;
                }
                p_visitor.Transform(t_transform);
            }
            break;
            
            /* FillPaint / StrokePaint require executing a paint opcode stream.
             * The attribute to apply is passed to the visitor after the paint
             * has been executed. */
            case kMCGDrawingOpcodeFillPaint:
            case kMCGDrawingOpcodeStrokePaint:
            {
                MCGDrawingPaint t_paint;
                if (!ExecutePaint(t_paint))
                {
                    break;
                }
                if (t_opcode == kMCGDrawingOpcodeFillPaint)
                {
                    p_visitor.FillPaint(t_paint);
                }
                else
                {
                    p_visitor.StrokePaint(t_paint);
                }
            }
            break;
                
            /* FillOpacity / StrokeOpacity set the corresponding attribute. */
            case kMCGDrawingOpcodeFillOpacity:
            case kMCGDrawingOpcodeStrokeOpacity:
            {
                /* Fetch the requested opacity as a unit scalar. */
                MCGFloat t_opacity;
                if (!UnitScalar(t_opacity))
                {
                    break;
                }
                
                /* Visit the appropriate method depending on which opcode was
                 * encountered. */
                if (t_opcode == kMCGDrawingOpcodeFillOpacity)
                {
                    p_visitor.FillOpacity(t_opacity);
                }
                else
                {
                    p_visitor.StrokeOpacity(t_opacity);
                }
            }
            break;
            
            /* FillRule visits the FillRule method of the visitor. */
            case kMCGDrawingOpcodeFillRule:
            {
                /* Fetch the fill rule opcode to apply. */
                MCGDrawingFillRuleOpcode t_fill_rule_opcode;
                if (!FillRuleOpcode(t_fill_rule_opcode))
                {
                    break;
                }
                
                /* Determine the fill rule type from the opcode. */
                MCGFillRule t_fill_rule;
                switch(t_fill_rule_opcode)
                {
                    case kMCGDrawingFillRuleOpcodeNonZero:
                        t_fill_rule = kMCGFillRuleNonZero;
                        break;
                    case kMCGDrawingFillRuleOpcodeEvenOdd:
                        t_fill_rule = kMCGFillRuleEvenOdd;
                        break;
                }
                
                /* Visit the visitor method. */
                p_visitor.FillRule(t_fill_rule);
            }    
            break;
            
            /* StrokeWidth visits the StrokeWidth method of the visitor. */
            case kMCGDrawingOpcodeStrokeWidth:
            {
                /* Fetch the requested width as a length scalar. */
                MCGFloat t_width;
                if (!LengthScalar(t_width))
                {
                    break;
                }
                
                /* Visit the visitor method. */
                p_visitor.StrokeWidth(t_width);
            }
            break;
            
            /* StrokeLineJoin visits the StrokeLineJoin method of the visitor. */
            case kMCGDrawingOpcodeStrokeLineJoin:
            {
                /* Fetch the requested line join as a LineJoin opcode. */
                MCGDrawingStrokeLineJoinOpcode t_line_join_opcode;
                if (!StrokeLineJoinOpcode(t_line_join_opcode))
                {
                    break;
                }
                
                /* Determine the join style from the opcode. */
                MCGJoinStyle t_join_style;
                switch(t_line_join_opcode)
                {
                    case kMCGDrawingStrokeLineJoinOpcodeBevel:
                        t_join_style = kMCGJoinStyleBevel;
                        break;
                    case kMCGDrawingStrokeLineJoinOpcodeRound:
                        t_join_style = kMCGJoinStyleRound;
                        break;
                    case kMCGDrawingStrokeLineJoinOpcodeMiter:
                        t_join_style = kMCGJoinStyleMiter;
                        break;
                }
                
                /* Visit the visitor method. */
                p_visitor.StrokeLineJoin(t_join_style);
            }    
            break;
                
            /* StrokeLineCap visits the StrokeLineCap method of the visitor. */
            case kMCGDrawingOpcodeStrokeLineCap:
            {
                /* Fetch the requested line cap as a LineCapOpcode. */
                MCGDrawingStrokeLineCapOpcode t_line_cap_opcode;
                if (!StrokeLineCapOpcode(t_line_cap_opcode))
                {
                    break;
                }
                
                /* Determine the cap style from the opcode. */
                MCGCapStyle t_cap_style;
                switch(t_line_cap_opcode)
                {
                    case kMCGDrawingStrokeLineCapOpcodeButt:
                        t_cap_style = kMCGCapStyleButt;
                        break;
                    case kMCGDrawingStrokeLineCapOpcodeRound:
                        t_cap_style = kMCGCapStyleRound;
                        break;
                    case kMCGDrawingStrokeLineCapOpcodeSquare:
                        t_cap_style = kMCGCapStyleSquare;
                        break;
                }
                
                /* Visit the visitor method. */
                p_visitor.StrokeLineCap(t_cap_style);
            }    
            break;
                
            /* StrokeDashArray visits the StrokeDashArray method of the visitor.
             */
            case kMCGDrawingOpcodeStrokeDashArray:
            {
                /* Fetch the requested dash array as a length array (the count
                 * is encoded in the opcode stream). */
                MCSpan<const MCGFloat> t_lengths;
                if (!LengthArray(t_lengths))
                {
                    break;
                }
                
                /* Visit the visitor method. */
                p_visitor.StrokeDashArray(t_lengths);
            }
            break;
            
            /* StrokeDashOffset visits the StrokeDashOffset method of the
             * visitor. */
            case kMCGDrawingOpcodeStrokeDashOffset:
            {
                /* Fetch the requested dash offset as a length scalar. */
                MCGFloat t_offset;
                if (!LengthScalar(t_offset))
                {
                    break;
                }
                
                /* Visit the visitor method. */
                p_visitor.StrokeDashOffset(t_offset);
            }
            break;
                
            /* StrokeMiterLimit visits the StrokeMiterLimit method of the
             * visitor. */
            case kMCGDrawingOpcodeStrokeMiterLimit:
            {
                /* Fetch the requested miter limit as a positive scalar. */
                MCGFloat t_miter_limit;
                if (!PositiveScalar(t_miter_limit))
                {
                    break;
                }
                
                /* Visit the visitor method. */
                p_visitor.StrokeMiterLimit(t_miter_limit);
            }
            break;
            
            /* Rectangle visits the Rectangle method of the visitor. */
            case kMCGDrawingOpcodeRectangle:
            {
                /* Fetch the requested (rounded) rectangle's geometry. The
                 * geometry is encoded as scalars:
                 *   x, y : coordinate scalars
                 *   width, height : length scalars
                 *   rx, ry : length scalars
                 */
                MCGRectangle t_bounds;
                MCGSize t_radii;
                if (!Rectangle(t_bounds) ||
                    !Size(t_radii))
                {
                    break;
                }
                
                /* Visit the visitor method. */
                p_visitor.Rectangle(t_bounds, t_radii);
            }
            break;
                
            /* Circle visits the Circle method of the visitor. */
            case kMCGDrawingOpcodeCircle:
            {
                /* Fetch the requested circle's geometry. The geometry is
                 * encoded as scalars:
                 *   cx, cy : coordinate scalars
                 *   r : length scalar
                 */
                MCGPoint t_center;
                MCGFloat t_radius;
                if (!Point(t_center) ||
                    !LengthScalar(t_radius))
                {
                    break;
                }
                
                /* Visit the visitor method. */
                p_visitor.Circle(t_center, t_radius);
            }
            break;
            
            /* Ellipse visits the Ellipse method of the visitor. */
            case kMCGDrawingOpcodeEllipse:
            {
                /* Fetch the requested ellipse's geometry. The geometry is
                 * encoded as scalars:
                 *   cx, cy : coordinate scalars
                 *   rx, ry : length scalars
                 */
                MCGPoint t_center;
                MCGSize t_radii;
                if (!Point(t_center) ||
                    !Size(t_radii))
                {
                    break;
                }
                
                /* Visit the visitor method. */
                p_visitor.Ellipse(t_center, t_radii);
            }
            break;
                
            /* Line visits the Line method of the visitor. */
            case kMCGDrawingOpcodeLine:
            {
                /* Fetch the requested line's geometry. The geometry is
                 * encoded as scalars:
                 *   x1, y1 : coordinate scalars
                 *   x2, y2 : coordinate scalars
                 */
                MCGPoint t_from, t_to;
                if (!Point(t_from) ||
                    !Point(t_to))
                {
                    break;
                }
                
                /* Visit the visitor method. */
                p_visitor.Line(t_from, t_to);
            }
            break;
                
            /* Polygon/Polyline visit the Polygon/Polyline methods of the
             * visitor. */
            case kMCGDrawingOpcodePolygon:
            case kMCGDrawingOpcodePolyline:
            {
                /* Fetch the requested shape's geometry. The geometry is 
                 * encoded as a point array - the number of points (reflected
                 * as scalar count) in the opcode stream. */
                MCSpan<const MCGPoint> t_points;
                if (!PointArray(t_points))
                {
                    break;
                }
                
                /* Visit the appropriate visitor method depending on the opcode.
                 */
                if (t_opcode == kMCGDrawingOpcodePolygon)
                {
                    p_visitor.Polygon(t_points);
                }
                else
                {
                    p_visitor.Polyline(t_points);
                }
            }
            break;
            
            /* Path executes a path opcode stream, causing appropriate Path
             * methods to be visited in the visitor. The path visit is bounded
             * by a visit of PathBegin and PathEnd. */
            case kMCGDrawingOpcodePath:
            {
                ExecutePath(p_visitor);
            }
            break;
        }
    }
    
    /* Tell the visitor that visiting has terminated - whether an error occurred
     * or not is flagged by the bool parameter. (None means no error, everything
     * else means error). */
    p_visitor.Finish(m_status != kMCGDrawingStatusNone);
}

/* The implementation of the ExecuteTransform method. */
bool MCGDrawingContext::ExecuteTransform(MCGAffineTransform p_base_transform,
                                         MCGAffineTransform& r_transform)
{
    MCGAffineTransform t_transform = p_base_transform;

    while(IsRunning())
    {
        MCGDrawingTransformOpcode t_opcode;
        if (!TransformOpcode(t_opcode))
        {
            return false;
        }
        
        if (t_opcode == kMCGDrawingTransformOpcodeEnd)
        {
            break;
        }
        
        switch(t_opcode)
        {
            case kMCGDrawingTransformOpcodeEnd:
                MCUnreachable();
                break;
                
            case kMCGDrawingTransformOpcodeAffine:
            {
                MCGAffineTransform t_affine_transform;
                if (!AffineTransform(t_affine_transform))
                {
                    return false;
                }
                t_transform = MCGAffineTransformConcat(t_transform, t_affine_transform);
            }
            break;
        }
    }
    
    r_transform = t_transform;
    
    return true;
}

/* The implementation of the ExecutePaint method. */
bool MCGDrawingContext::ExecutePaint(MCGDrawingPaint& r_paint)
{
    MCGDrawingPaint t_paint;
    while(IsRunning())
    {
        MCGDrawingPaintOpcode t_opcode;
        if (!PaintOpcode(t_opcode))
        {
            break;
        }
        
        if (t_opcode == kMCGDrawingPaintOpcodeEnd)
        {
            break;
        }
        
        switch(t_opcode)
        {
            case kMCGDrawingPaintOpcodeEnd:
                MCUnreachable();
                break;
                
            case kMCGDrawingPaintOpcodeSolidColor:
                t_paint.type = kMCGDrawingPaintTypeSolidColor;
                if (!SolidColor(t_paint.solid_color))
                {
                    return false;
                }
                break;
            
            case kMCGDrawingPaintOpcodeLinearGradient:
                t_paint.type = kMCGDrawingPaintTypeLinearGradient;
                if (!Gradient(t_paint.gradient))
                {
                    return false;
                }
                break;
                
            case kMCGDrawingPaintOpcodeRadialGradient:
                t_paint.type = kMCGDrawingPaintTypeRadialGradient;
                if (!Gradient(t_paint.gradient))
                {
                    return false;
                }
                break;
                
            case kMCGDrawingPaintOpcodeConicalGradient:
                t_paint.type = kMCGDrawingPaintTypeConicalGradient;
                if (!Gradient(t_paint.gradient))
                {
                    return false;
                }
                if (!Point(t_paint.conical_gradient.focal_point))
                {
                    return false;
                }
                if (!LengthScalar(t_paint.conical_gradient.focal_radius))
                {
                    return false;
                }
                break;
        }
    }
    
    r_paint = t_paint;
    
    return true;
}

/* The (template based) implementation of the ExecutePath method. */
template<typename VisitorT>
void MCGDrawingContext::ExecutePath(VisitorT& p_visitor)
{
    p_visitor.PathBegin();
    
    while(IsRunning())
    {
        MCGDrawingPathOpcode t_opcode;
        if (!PathOpcode(t_opcode))
        {
            break;
        }
        
        if (t_opcode == kMCGDrawingPathOpcodeEnd)
        {
            p_visitor.PathEnd();
            break;
        }
        
        switch(t_opcode)
        {
            case kMCGDrawingPathOpcodeEnd:
            {
                MCUnreachable();
            }
            break;
                
            case kMCGDrawingPathOpcodeMoveTo:
            case kMCGDrawingPathOpcodeRelativeMoveTo:
            {
                MCGPoint t_point;
                if (!Point(t_point))
                {
                    break;
                }
                p_visitor.PathMoveTo(t_opcode == kMCGDrawingPathOpcodeRelativeMoveTo,
                                     t_point);
            }
            break;
                
            case kMCGDrawingPathOpcodeLineTo:
            case kMCGDrawingPathOpcodeRelativeLineTo:
            {
                MCGPoint t_point;
                if (!Point(t_point))
                {
                    break;
                }
                p_visitor.PathLineTo(t_opcode == kMCGDrawingPathOpcodeRelativeLineTo,
                                     t_point);
            }
            break; 
            
            case kMCGDrawingPathOpcodeHorizontalTo:
            case kMCGDrawingPathOpcodeRelativeHorizontalTo:
            {
                MCGFloat t_x;
                if (!CoordinateScalar(t_x))
                {
                    break;
                }
                p_visitor.PathHorizontalTo(t_opcode == kMCGDrawingPathOpcodeRelativeHorizontalTo,
                                           t_x);
            }
            break;
                
            case kMCGDrawingPathOpcodeVerticalTo:
            case kMCGDrawingPathOpcodeRelativeVerticalTo:
            {
                MCGFloat t_y;
                if (!CoordinateScalar(t_y))
                {
                    break;
                }
                p_visitor.PathVerticalTo(t_opcode == kMCGDrawingPathOpcodeRelativeVerticalTo,
                                         t_y);
            }
            break;
                
            case kMCGDrawingPathOpcodeCubicTo:
            case kMCGDrawingPathOpcodeRelativeCubicTo:
            {
                MCGPoint t_a, t_b, t_point;
                if (!Point(t_a) ||
                    !Point(t_b) ||
                    !Point(t_point))
                {
                    break;
                }
                p_visitor.PathCubicTo(t_opcode == kMCGDrawingPathOpcodeRelativeCubicTo,
                                      t_a,
                                      t_b,
                                      t_point);
            }
            break;
                
            case kMCGDrawingPathOpcodeSmoothCubicTo:
            case kMCGDrawingPathOpcodeRelativeSmoothCubicTo:
            {
                MCGPoint t_a, t_b, t_point;
                if (!Point(t_b) ||
                    !Point(t_point))
                {
                    break;
                }
                p_visitor.PathSmoothCubicTo(t_opcode == kMCGDrawingPathOpcodeRelativeSmoothCubicTo,
                                            t_b,
                                            t_point);
            }
            break;
                
            case kMCGDrawingPathOpcodeQuadraticTo:
            case kMCGDrawingPathOpcodeRelativeQuadraticTo:
            {
                MCGPoint t_a, t_point;
                if (!Point(t_a) ||
                    !Point(t_point))
                {
                    break;
                }
                p_visitor.PathQuadraticTo(t_opcode == kMCGDrawingPathOpcodeRelativeQuadraticTo,
                                          t_a,
                                          t_point);
            }
            break;
                
            case kMCGDrawingPathOpcodeSmoothQuadraticTo:
            case kMCGDrawingPathOpcodeRelativeSmoothQuadraticTo:
            {
                MCGPoint t_point;
                if (!Point(t_point))
                {
                    break;
                }
                p_visitor.PathSmoothQuadraticTo(t_opcode == kMCGDrawingPathOpcodeRelativeSmoothQuadraticTo,
                                                t_point);
            }
            break;
                
            case kMCGDrawingPathOpcodeArcTo:
            case kMCGDrawingPathOpcodeRelativeArcTo:
            case kMCGDrawingPathOpcodeReflexArcTo:
            case kMCGDrawingPathOpcodeRelativeReflexArcTo:
            case kMCGDrawingPathOpcodeReverseArcTo:
            case kMCGDrawingPathOpcodeRelativeReverseArcTo:
            case kMCGDrawingPathOpcodeReverseReflexArcTo:
            case kMCGDrawingPathOpcodeRelativeReverseReflexArcTo:
            {
                MCGSize t_radii;
                MCGFloat t_rotation;
                MCGPoint t_point;
                if (!Size(t_radii) ||
                    !AngleScalar(t_rotation) ||
                    !Point(t_point))
                {
                    break;
                }
                
                p_visitor.PathArcTo(MCGDrawingPathOpcodeIsRelativeArcTo(t_opcode),
                                    MCGDrawingPathOpcodeIsReflexArcTo(t_opcode),
                                    MCGDrawingPathOpcodeIsReverseArcTo(t_opcode),
                                    t_radii,
                                    t_rotation,
                                    t_point);
            }
            break;
            
            case kMCGDrawingPathOpcodeCloseSubpath:
            {
                p_visitor.PathCloseSubpath();
            }
            break;
        }
    }
}

/******************************************************************************/

/* MCGDrawingRenderVisitor implements a visitor for the Execute method(s)
 * required by MCGDrawingContext. It performs rendering of a drawing using
 * a MCGContext. */
struct MCGDrawingRenderVisitor
{
    MCGContextRef gcontext = nullptr;

    /**/
    
    MCGPoint first_point = {0, 0};
    MCGPoint last_point = {0, 0};
    MCGPoint last_control_point = {0, 0};
    MCGDrawingPathOpcode last_curve_opcode = kMCGDrawingPathOpcodeEnd;
    
    /**/
    
    MCGPoint Point(bool p_is_relative, MCGPoint p_point)
    {
        if (p_is_relative)
        {
            return MCGPointMake(last_point.x + p_point.x, last_point.y + p_point.y);
        }
        return p_point;
    }
    
    MCGPoint HorizontalPoint(bool p_is_relative, MCGFloat p_x)
    {
        if (p_is_relative)
        {
            return MCGPointMake(last_point.x + p_x, last_point.y);
        }
        return MCGPointMake(p_x, last_point.y);
    }
    
    MCGPoint VerticalPoint(bool p_is_relative, MCGFloat p_y)
    {
        if (p_is_relative)
        {
            return MCGPointMake(last_point.x, last_point.y + p_y);
        }
        return MCGPointMake(last_point.x, p_y);
    }
    
    /**/
    
    void PathBegin(void)
    {
        MCGContextBeginPath(gcontext);
        
        last_curve_opcode = kMCGDrawingPathOpcodeEnd;
        first_point = last_point = MCGPointMake(0.0, 0.0);
    }
    
    void PathMoveTo(bool p_is_relative, MCGPoint p_point)
    {
        MCGPoint t_point = Point(p_is_relative, p_point);
        MCGContextMoveTo(gcontext, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeEnd;
        first_point = last_point = t_point;
    }
    
    void PathLineTo(bool p_is_relative, MCGPoint p_point)
    {
        MCGPoint t_point = Point(p_is_relative, p_point);
        MCGContextLineTo(gcontext, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeEnd;
        last_point = t_point;
    }
    
    void PathHorizontalTo(bool p_is_relative, MCGFloat p_x)
    {
        MCGPoint t_point = HorizontalPoint(p_is_relative, p_x);
        MCGContextLineTo(gcontext, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeEnd;
        last_point = t_point;
    }
    
    void PathVerticalTo(bool p_is_relative, MCGFloat p_y)
    {
        MCGPoint t_point = VerticalPoint(p_is_relative, p_y);
        MCGContextLineTo(gcontext, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeEnd;
        last_point = t_point;
    }
    
    void PathCubicTo(bool p_is_relative, MCGPoint p_a, MCGPoint p_b, MCGPoint p_point)
    {
        MCGPoint t_a = Point(p_is_relative, p_a);
        MCGPoint t_b = Point(p_is_relative, p_b);
        MCGPoint t_point = Point(p_is_relative, p_point);
        MCGContextCubicTo(gcontext, t_a, t_b, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeCubicTo;
        last_control_point = t_b;
        last_point = t_point;
    }
    
    void PathSmoothCubicTo(bool p_is_relative,  MCGPoint p_b, MCGPoint p_point)
    {
        MCGPoint t_a;
        if (last_curve_opcode == kMCGDrawingPathOpcodeCubicTo)
        {
            t_a = MCGPointReflect(last_point, last_control_point);
        }
        else
        {
            t_a = last_point;
        }
        
        MCGPoint t_b = Point(p_is_relative, p_b);
        MCGPoint t_point = Point(p_is_relative, p_point);
        MCGContextCubicTo(gcontext, t_a, t_b, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeCubicTo;
        last_control_point = t_b;
        last_point = t_point;
    }
    
    void PathQuadraticTo(bool p_is_relative, MCGPoint p_a, MCGPoint p_point)
    {
        MCGPoint t_a = Point(p_is_relative, p_a);
        MCGPoint t_point = Point(p_is_relative, p_point);
        MCGContextQuadraticTo(gcontext, t_a, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeQuadraticTo;
        last_control_point = t_a;
        last_point = t_point;
    }
    
    void PathSmoothQuadraticTo(bool p_is_relative, MCGPoint p_point)
    {
        MCGPoint t_a;
        if (last_curve_opcode == kMCGDrawingPathOpcodeQuadraticTo)
        {
            t_a = MCGPointReflect(last_point, last_control_point);
        }
        else
        {
            t_a = last_point;
        }
        
        MCGPoint t_point = Point(p_is_relative, p_point);
        MCGContextQuadraticTo(gcontext, t_a, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeCubicTo;
        last_control_point = t_a;
        last_point = t_point;
    }
    
    void PathArcTo(bool p_is_relative, bool p_is_reflex, bool p_is_reverse, MCGSize p_radii, MCGFloat p_rotation, MCGPoint p_point)
    {
        MCGPoint t_point = Point(p_is_relative, p_point);
        MCGContextArcTo(gcontext, p_radii, p_rotation, p_is_reflex, p_is_reverse, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeEnd;
        last_point = t_point;
    }
    
    void PathCloseSubpath(void)
    {
        MCGContextCloseSubpath(gcontext);
        last_point = first_point;
    }
    
    void PathEnd(void)
    {
        MCGContextFillAndStroke(gcontext);
    }
    
    /**/
    
    void Start(void)
    {
        MCGContextSave(gcontext);
        MCGContextSetShouldAntialias(gcontext, true);
    }
    
    void Finish(bool p_error)
    {
        MCGContextRestore(gcontext);
    }
    
    void Transform(MCGAffineTransform p_transform)
    {
        MCGContextSetTransform(gcontext, p_transform);
    }
    
    void FillPaint(const MCGDrawingPaint& p_paint)
    {
        ApplyPaint(false, p_paint);
    }
    
    void FillOpacity(MCGFloat p_opacity)
    {
        MCGContextSetFillOpacity(gcontext, p_opacity);
    }
    
    void FillRule(MCGFillRule p_fill_rule)
    {
        MCGContextSetFillRule(gcontext, p_fill_rule);
    }
    
    void StrokePaint(const MCGDrawingPaint& p_paint)
    {
        ApplyPaint(true, p_paint);
    }
    
    void StrokeOpacity(MCGFloat p_opacity)
    {
        MCGContextSetStrokeOpacity(gcontext, p_opacity);
    }
    
    void StrokeWidth(MCGFloat p_width)
    {
        MCGContextSetStrokeWidth(gcontext, p_width);
    }
    
    void StrokeLineJoin(MCGJoinStyle p_join_style)
    {
        MCGContextSetStrokeJoinStyle(gcontext, p_join_style);
    }
    
    void StrokeLineCap(MCGCapStyle p_cap_style)
    {
        MCGContextSetStrokeCapStyle(gcontext, p_cap_style);
    }
    
    void StrokeDashArray(MCSpan<const MCGFloat> p_lengths)
    {
        MCGContextSetStrokeDashArray(gcontext, p_lengths.data(), p_lengths.size());
    }
    
    void StrokeDashOffset(MCGFloat p_offset)
    {
        MCGContextSetStrokeDashOffset(gcontext, p_offset);
    }
    
    void StrokeMiterLimit(MCGFloat p_miter_limit)
    {
        MCGContextSetStrokeMiterLimit(gcontext, p_miter_limit);
    }
    
    void Rectangle(MCGRectangle p_rect, MCGSize p_radii)
    {
        if (p_radii.width == 0 && p_radii.height == 0)
        {
            MCGContextAddRectangle(gcontext, p_rect);
        }
        else
        {
            MCGContextAddRoundedRectangle(gcontext, p_rect, p_radii);
        }
        MCGContextFillAndStroke(gcontext);
    }
    
    void Circle(MCGPoint p_center, MCGFloat p_r)
    {
        MCGContextAddEllipse(gcontext, p_center, MCGSizeMake(p_r, p_r), 0.0);
        MCGContextFillAndStroke(gcontext);
    }
    
    void Ellipse(MCGPoint p_center, MCGSize p_radii)
    {
        MCGContextAddEllipse(gcontext, p_center, p_radii, 0.0);
        MCGContextFillAndStroke(gcontext);
    }
    
    void Line(MCGPoint p_from, MCGPoint p_to)
    {
        MCGContextAddLine(gcontext, p_from, p_to);
        MCGContextFillAndStroke(gcontext);
    }
    
    void Polyline(MCSpan<const MCGPoint> p_points)
    {
        MCGContextAddPolyline(gcontext, p_points.data(), uindex_t(p_points.size()));
        MCGContextFillAndStroke(gcontext);
    }
    
    void Polygon(MCSpan<const MCGPoint> p_points)
    {
        MCGContextAddPolygon(gcontext, p_points.data(), uindex_t(p_points.size()));
        MCGContextFillAndStroke(gcontext);
    }
    
private:
    void ApplyPaint(bool p_is_stroke, const MCGDrawingPaint& p_paint)
    {
        switch(p_paint.type)
        {
            case kMCGDrawingPaintTypeNone:
                (!p_is_stroke ? MCGContextSetFillNone
                              : MCGContextSetStrokeNone)(gcontext);
            break;
            
            case kMCGDrawingPaintTypeSolidColor:
                (!p_is_stroke ? MCGContextSetFillRGBAColor
                              : MCGContextSetStrokeRGBAColor)(gcontext,
                                                              p_paint.solid_color.red,
                                                              p_paint.solid_color.green,
                                                              p_paint.solid_color.blue,
                                                              p_paint.solid_color.alpha);
            break;
            
            case kMCGDrawingPaintTypeLinearGradient:
            case kMCGDrawingPaintTypeRadialGradient:
            case kMCGDrawingPaintTypeConicalGradient:
            {
                MCGRampRef t_ramp = nullptr;
                if (!MCGRamp::Create4f(p_paint.gradient.ramp.offsets, p_paint.gradient.ramp.colors, p_paint.gradient.ramp.count, t_ramp))
                {
                    break;
                }
                
                MCGGradientRef t_gradient = nullptr;
                if (p_paint.type == kMCGDrawingPaintTypeLinearGradient)
                {
                    if (!MCGLinearGradient::Create({0, 0}, {1, 0}, t_ramp, p_paint.gradient.spread, p_paint.gradient.transform, t_gradient))
                    {
                        MCGRelease(t_ramp);
                        break;
                    }
                }
                else if (p_paint.type == kMCGDrawingPaintTypeRadialGradient)
                {
                    if (!MCGRadialGradient::Create({0, 0}, 1, t_ramp, p_paint.gradient.spread, p_paint.gradient.transform, t_gradient))
                    {
                        MCGRelease(t_ramp);
                        break;
                    }
                }
                else if (p_paint.type == kMCGDrawingPaintTypeConicalGradient)
                {
                    if (!MCGConicalGradient::Create({0, 0}, 1,
                                                            p_paint.conical_gradient.focal_point,
                                                            p_paint.conical_gradient.focal_radius,
                                                            t_ramp,
                                                            p_paint.gradient.spread,
                                                            p_paint.gradient.transform,
                                                            t_gradient))
                    {
                        MCGRelease(t_ramp);
                        break;
                    }
                }
                
                MCGRelease(t_ramp);
                
                (!p_is_stroke ? MCGContextSetFillPaint
                              : MCGContextSetStrokePaint)(gcontext, t_gradient);
                
                MCGRelease(t_gradient);
            }
            break;
        }
    }
};

/* MCGContextPlayback renders the specified drawing into p_dst_rect of p_gcontext. */
void MCGContextPlayback(MCGContextRef p_gcontext, MCGRectangle p_dst_rect, MCSpan<const byte_t> p_drawing)
{
    MCGDrawingRenderVisitor t_render_visitor;
    t_render_visitor.gcontext = p_gcontext;
    
    MCGDrawingContext t_context(p_drawing);
    t_context.Execute(t_render_visitor, p_dst_rect);
}

/******************************************************************************/
