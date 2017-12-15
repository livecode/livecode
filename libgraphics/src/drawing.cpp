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
    
    /* SolidColor defines a solid color paint. It is defined by 3 scalars -
     * red green blue. */
    kMCGDrawingPaintOpcodeSolidColor = 1,
    
    /* _Last is used for range checking on the paint opcodes. */
    kMCGDrawingPaintOpcode_Last = kMCGDrawingPaintOpcodeSolidColor,
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
    kMCGDrawingPathOpcodeBearing = 26,
    
    kMCGDrawingPathOpcode_Last = kMCGDrawingPathOpcodeBearing,
};

inline bool MCGDrawingPathOpcodeIsRelativeArc(MCGDrawingPathOpcode p_opcode)
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

inline bool MCGDrawingPathOpcodeIsReflexArc(MCGDrawingPathOpcode p_opcode)
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

inline bool MCGDrawingPathOpcodeIsReverseArc(MCGDrawingPathOpcode p_opcode)
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

struct MCGDrawingVisitor
{
    void TransformBegin(void);
    void TransformAffine(MCGFloat a, MCGFloat b, MCGFloat c, MCGFloat d, MCGFloat tx, MCGFloat ty);
    void TransformViewport(MCGFloat min_x, MCGFloat min_y, MCGFloat width, MCGFloat height, MCGDrawingPreserveAspectRatioOpcode par);
    void TransformEnd(void);
    
    void PaintBegin(bool is_stroke);
    void PaintNone(void);
    void PaintSolidColor(MCGFloat red, MCGFloat green, MCGFloat blue);
    void PaintEnd(void);
    
    void PathBegin(void);
    void PathMoveTo(bool is_relative, MCGFloat x, MCGFloat y);
    void PathLineTo(bool is_relative, MCGFloat x, MCGFloat y);
    void PathHorizontalTo(bool is_relative, MCGFloat x);
    void PathVerticalTo(bool is_relative, MCGFloat y);
    void PathCubicTo(bool is_relative, MCGFloat ax, MCGFloat ay, MCGFloat bx, MCGFloat by, MCGFloat x, MCGFloat y);
    void PathSmoothCubicTo(bool is_relative, MCGFloat bx, MCGFloat by, MCGFloat x, MCGFloat y);
    void PathQuadraticTo(bool is_relative, MCGFloat ax, MCGFloat ay, MCGFloat x, MCGFloat y);
    void PathSmoothQuadraticTo(bool is_relative, MCGFloat x, MCGFloat y);
    void PathArcTo(bool is_relative, bool is_reflex, bool is_reverse, MCGFloat cx, MCGFloat cy, MCGFloat rotation, MCGFloat x, MCGFloat y);
    void PathCloseSubpath(void);
    void PathBearing(MCGFloat angle);
    void PathEnd(void);
    
    void FillOpacity(MCGFloat opacity);
    void FillRule(MCGFillRule fill_rule);
    
    void StrokeOpacity(MCGFloat opacity);
    void StrokeWidth(MCGFloat width);
    void StrokeLineJoin(MCGJoinStyle join_style);
    void StrokeLineCap(MCGCapStyle cap_style);
    void StrokeDashArray(size_t count, const MCGFloat* lengths);
    void StrokeDashOffset(MCGFloat offset);
    void StrokeMiterLimit(MCGFloat miter_limit);
    
    void Rectangle(MCGFloat x, MCGFloat y, MCGFloat width, MCGFloat height, MCGFloat rx, MCGFloat ry);
    void Circle(MCGFloat cx, MCGFloat cy, MCGFloat r);
    void Ellipse(MCGFloat cx, MCGFloat cy, MCGFloat rx, MCGFloat ry);
    void Line(MCGFloat x1, MCGFloat y1, MCGFloat x2, MCGFloat y2);
    void Polyline(size_t count, const MCGFloat* coordinates);
    void Polygon(size_t count, const MCGFloat* coordinates);
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
    MCGDrawingByteStream(const void *p_bytes, size_t p_byte_count)
        : m_bytes(p_bytes),
          m_byte_count(p_byte_count)
    {
    }

    /* Returns true if there are no more bytes to process. */
    bool IsFinished(void) const
    {
        return m_byte_count == 0;
    }

    /* Singleton attempts to unpack a single field of type T. It returns true
     * if successful, or false if there are not enough bytes (sizeof(T)). */
    template<typename T>
    bool Singleton(T& r_singleton)
    {
        size_t t_singleton_size = sizeof(T);
        
        if (m_byte_count < t_singleton_size)
        {
            return false;
        }
        
        auto t_singleton_ptr = static_cast<const T *>(m_bytes);
        r_singleton = *t_singleton_ptr;
        
        m_bytes = t_singleton_ptr + 1;
        m_byte_count -= t_singleton_size;
        
        return true;
    }
    
    /* Array attempts to unpack p_count fields of type T. It returns true if
     * successful, or false if there are not enough bytes (sizeof(T) * p_count).
     */
    template<typename T>
    bool Sequence(size_t p_count, T*& r_seq)
    {
        size_t t_seq_size = p_count * sizeof(T);
        
        if (m_byte_count < t_seq_size)
        {
            return false;
        }
        
        auto t_seq_ptr = static_cast<const T *>(m_bytes);
        r_seq = t_seq_ptr;
        
        m_bytes = t_seq_ptr + p_count;
        m_byte_count -= t_seq_size;
        
        return true;
    }

private:
    const void *m_bytes;
    size_t m_byte_count;
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
    MCGDrawingContext(const void *p_drawing_bytes, size_t p_drawing_byte_count)
    {
        MCGDrawingByteStream t_stream(p_drawing_bytes, p_drawing_byte_count);
        
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
            !t_stream.Singleton(m_scalar_count) ||
            !t_stream.Sequence(m_scalar_count, m_scalars) ||
            !t_stream.Singleton(m_opcode_count) ||
            !t_stream.Sequence(m_opcode_count, m_opcodes) ||
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
    
    /* Count attempts to read an non-negative integer from the opcode stream.
     * Non-negative integers are encoded as a sequence of bytes, most
     * significant byte first. The top bit of each byte is used to indicate
     * whether there is another following. */
    bool Count(size_t& r_count)
    {
        /* Start the read count at zero, each byte will be accumulated into it
         */
        size_t t_count = 0;
    
        /* Opcode bytes are processed until we reach a termination condition. */
        for(;;)
        {
            /* If the end of the opcode stream is reached, then indicate 
             * overflow. */
            if (m_pc == m_opcode_count)
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
        if (m_sc == m_scalar_count)
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
    bool Scalars(size_t p_count, const MCGFloat*& r_scalars)
    {
        /* If there are not enough scalars left to satisfy the request then
         * indicate ScalarOverflow and return false. */
        if (m_sc + p_count > m_scalar_count)
        {
            m_status = kMCGDrawingStatusScalarOverflow;
            return false;
        }
        
        /* Place the base of the scalar array in r_scalars. */
        r_scalars = m_scalars + m_sc;
        
        /* Increment the scalar counter */
        m_sc += p_count;
        
        return true;
    }
    
    /* LengthScalars attempts to read p_count length scalars - a length scalar
     * is non-negative scalar. */
    bool LengthScalars(size_t p_count, const MCGFloat*& r_scalars)
    {
        return Scalars(p_count, r_scalars);
    }
    
    /* CoordinateScalars attempts to read p_count coordinate scalars. */
    bool CoordinateScalars(size_t p_count, const MCGFloat*& r_scalars)
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
    
    /* LengthArray attempts to read an array of scalars, using a count fetched
     * from the opcode stream. */
    bool LengthArray(size_t& r_count, const MCGFloat*& r_lengths)
    {
        if (!Count(r_count))
        {
            return false;
        }
        
        if (!Scalars(r_count, r_lengths))
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
    bool PointArray(size_t& r_point_count, const MCGPoint*& r_points)
    {
        /* Read the number of scalars to expect from the opcode stream. */
        size_t t_scalar_count;
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
        const MCGFloat* t_scalars;
        if (!CoordinateScalars(t_scalar_count, t_scalars))
        {
            return false;
        }
        
        /* The number of points is half the number of scalars. */
        r_point_count = t_scalar_count / 2;
        
        /* MCGPoint is a pair of scalars, so use a reinterpret cast to change
         * the view of the scalars to MCGPoints. */
        r_points = reinterpret_cast<const MCGPoint*>(t_scalars);
        
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
        if (m_pc == m_opcode_count)
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

    /* ExecuteTransform runs a transform opcode stream into the provided
     * visitor. */
    template<typename VisitorT>
    void ExecuteTransform(VisitorT& visitor);
    
    /* ExecutePaint runs a paint opcode stream into the provided
     * visitor. */
    template<typename VisitorT>
    void ExecutePaint(VisitorT& visitor, bool is_stroke);
    
    /* ExecutePath runs a path opcode stream into the provided
     * visitor. */
    template<typename VisitorT>
    void ExecutePath(VisitorT& visitor);
     
    /**/
    
    /* m_status indicates the current state of the context. If this is not
     * None, then an error has occured. */
    MCGDrawingStatus m_status = kMCGDrawingStatusNone;
    
    /* m_pc indicates the current position in the opcode stream. */
    uindex_t m_pc = 0;
    
    /* m_sc indicates the current position in the scalar stream. */
    uindex_t m_sc = 0;
    
    /* m_opcodes / m_opcode_count define the (const) array of opcodes which are
     * defined by the drawing. */
    const byte_t *m_opcodes = nullptr;
    uindex_t m_opcode_count = 0;
    
    /* m_scalars / m_scalar_count define the (const) array of scalars which are
     * defined by the drawing. */
    const MCGFloat *m_scalars = nullptr;
    uindex_t m_scalar_count = 0;
    
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
void MCGDrawingContext::Execute(VisitorT& p_visitor, MCGRectangle p_dst_rect)
{
    /* If the drawing's width is negative, then it means the width of the
     * drawing was specified as a percentage (represented as a scalar in the
     * range [-1, 0)) of the requested destination rectangle width. Otherwise
     * the width is absolute. */
    if (m_width < 0)
    {
        p_dst_rect.size.width *= -m_width;
    }
    else
    {
        p_dst_rect.size.width = m_width;
    }
    
    /* If the drawing's height is negative, then it means the height of the
     * drawing was specified as a percentage (represented as a scalar in the
     * range [-1, 0)) of the requested destination rectangle height. Otherwise
     * the height is absolute. */
    if (m_height < 0)
    {
        p_dst_rect.size.height *= -m_height;
    }
    else
    {
        p_dst_rect.size.height = m_height;
    }
    
    /* Start the visitor, passing in the destination rectangle and viewport
     * details so it can configure an initial transform. */
    p_visitor.Start(p_dst_rect, m_has_viewport ? &m_viewbox : nullptr, m_preserve_aspect_ratio);
    
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
                ExecuteTransform(p_visitor);
            }
            break;
            
            /* FillPaint / StrokePaint require executing a paint opcode stream.
             * The attribute to apply is passed to the visitor after the paint
             * has been executed. */
            case kMCGDrawingOpcodeFillPaint:
            case kMCGDrawingOpcodeStrokePaint:
            {
                ExecutePaint(p_visitor, t_opcode == kMCGDrawingOpcodeStrokePaint);
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
                size_t t_length_count;
                const MCGFloat* t_lengths;
                if (!LengthArray(t_length_count, t_lengths))
                {
                    break;
                }
                
                /* Visit the visitor method. */
                p_visitor.StrokeDashArray(t_length_count, t_lengths);
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
                MCGFloat t_x, t_y, t_width, t_height, t_rx, t_ry;
                if (!CoordinateScalar(t_x) || !CoordinateScalar(t_y) ||
                    !LengthScalar(t_width) || !LengthScalar(t_height) ||
                    !LengthScalar(t_rx) || !LengthScalar(t_ry))
                {
                    break;
                }
                
                /* Visit the visitor method. */
                p_visitor.Rectangle(t_x, t_y, t_width, t_height, t_rx, t_ry);
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
                MCGFloat t_cx, t_cy, t_r;
                if (!CoordinateScalar(t_cx) || !CoordinateScalar(t_cy) ||
                    !LengthScalar(t_r))
                {
                    break;
                }
                
                /* Visit the visitor method. */
                p_visitor.Circle(t_cx, t_cy, t_r);
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
                MCGFloat t_cx, t_cy, t_rx, t_ry;
                if (!CoordinateScalar(t_cx) || !CoordinateScalar(t_cy) ||
                    !LengthScalar(t_rx) || !LengthScalar(t_ry))
                {
                    break;
                }
                
                /* Visit the visitor method. */
                p_visitor.Ellipse(t_cx, t_cy, t_rx, t_ry);
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
                MCGFloat t_x1, t_y1, t_x2, t_y2;
                if (!CoordinateScalar(t_x1) || !CoordinateScalar(t_y1) ||
                    !CoordinateScalar(t_x2) || !CoordinateScalar(t_y2))
                {
                    break;
                }
                
                /* Visit the visitor method. */
                p_visitor.Line(t_x1, t_y1, t_x2, t_y2);
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
                size_t t_point_count;
                const MCGPoint* t_points;
                if (!PointArray(t_point_count, t_points))
                {
                    break;
                }
                
                /* Visit the appropriate visitor method depending on the opcode.
                 */
                if (t_opcode == kMCGDrawingOpcodePolygon)
                {
                    p_visitor.Polygon(t_point_count, t_points);
                }
                else
                {
                    p_visitor.Polyline(t_point_count, t_points);
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
    p_visitor.Finish(m_status == kMCGDrawingStatusNone);
}

/* The (template based) implementation of the ExecuteTransform method. */
template<typename VisitorT>
void MCGDrawingContext::ExecuteTransform(VisitorT& p_visitor)
{
    p_visitor.TransformBegin();
    
    while(IsRunning())
    {
        MCGDrawingTransformOpcode t_opcode;
        if (!TransformOpcode(t_opcode))
        {
            break;
        }
        
        if (t_opcode == kMCGDrawingTransformOpcodeEnd)
        {
            p_visitor.TransformEnd();
            break;
        }
        
        switch(t_opcode)
        {
            case kMCGDrawingTransformOpcodeEnd:
                MCUnreachable();
                break;
                
            case kMCGDrawingTransformOpcodeAffine:
            {
                MCGFloat t_a, t_b, t_c, t_d, t_tx, t_ty;
                if (!Scalar(t_a) ||
                    !Scalar(t_b) ||
                    !Scalar(t_c) ||
                    !Scalar(t_d) ||
                    !Scalar(t_tx) ||
                    !Scalar(t_ty))
                {
                    break;
                }
                p_visitor.TransformAffine(t_a, t_b, t_c, t_d, t_tx, t_ty);
            }
            break;
        }
    }
}

/* The (template based) implementation of the ExecutePaint method. */
template<typename VisitorT>
void MCGDrawingContext::ExecutePaint(VisitorT& p_visitor, bool p_is_stroke)
{
    p_visitor.PaintBegin(p_is_stroke);
    
    while(IsRunning())
    {
        MCGDrawingPaintOpcode t_opcode;
        if (!PaintOpcode(t_opcode))
        {
            break;
        }
        
        if (t_opcode == kMCGDrawingPaintOpcodeEnd)
        {
            p_visitor.PaintEnd();
            break;
        }
        
        switch(t_opcode)
        {
            case kMCGDrawingPaintOpcodeEnd:
                MCUnreachable();
                break;
                
            case kMCGDrawingPaintOpcodeSolidColor:
                MCGFloat t_red, t_green, t_blue;
                if (!UnitScalar(t_red) ||
                    !UnitScalar(t_green) ||
                    !UnitScalar(t_blue))
                {
                    break;
                }
                p_visitor.PaintSolidColor(t_red, t_green, t_blue);
                break;
        }
    }
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
                MCGFloat t_x, t_y;
                if (!CoordinateScalar(t_x) ||
                    !CoordinateScalar(t_y))
                {
                    break;
                }
                p_visitor.PathMoveTo(t_opcode == kMCGDrawingPathOpcodeRelativeMoveTo, t_x, t_y);
            }
            break;
                
            case kMCGDrawingPathOpcodeLineTo:
            case kMCGDrawingPathOpcodeRelativeLineTo:
            {
                MCGFloat t_x, t_y;
                if (!CoordinateScalar(t_x) ||
                    !CoordinateScalar(t_y))
                {
                    break;
                }
                p_visitor.PathLineTo(t_opcode == kMCGDrawingPathOpcodeRelativeLineTo, t_x, t_y);
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
                p_visitor.PathHorizontalTo(t_opcode == kMCGDrawingPathOpcodeRelativeHorizontalTo, t_x);
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
                p_visitor.PathVerticalTo(t_opcode == kMCGDrawingPathOpcodeRelativeVerticalTo, t_y);
            }
            break;
                
            case kMCGDrawingPathOpcodeCubicTo:
            case kMCGDrawingPathOpcodeRelativeCubicTo:
            {
                MCGFloat t_ax, t_ay, t_bx, t_by, t_x, t_y;
                if (!CoordinateScalar(t_ax) || !CoordinateScalar(t_ay) ||
                    !CoordinateScalar(t_bx) || !CoordinateScalar(t_by) ||
                    !CoordinateScalar(t_x) || !CoordinateScalar(t_y))
                {
                    break;
                }
                p_visitor.PathCubicTo(t_opcode == kMCGDrawingPathOpcodeRelativeCubicTo, t_ax, t_ay, t_bx, t_by, t_x, t_y);
            }
            break;
                
            case kMCGDrawingPathOpcodeSmoothCubicTo:
            case kMCGDrawingPathOpcodeRelativeSmoothCubicTo:
            {
                MCGFloat t_bx, t_by, t_x, t_y;
                if (!CoordinateScalar(t_bx) || !CoordinateScalar(t_by) ||
                    !CoordinateScalar(t_x) || !CoordinateScalar(t_y))
                {
                    break;
                }
                p_visitor.PathSmoothCubicTo(t_opcode == kMCGDrawingPathOpcodeRelativeSmoothCubicTo, t_bx, t_by, t_x, t_y);
            }
            break;
                
            case kMCGDrawingPathOpcodeQuadraticTo:
            case kMCGDrawingPathOpcodeRelativeQuadraticTo:
            {
                MCGFloat t_ax, t_ay, t_x, t_y;
                if (!CoordinateScalar(t_ax) || !CoordinateScalar(t_ay) ||
                    !CoordinateScalar(t_x) || !CoordinateScalar(t_y))
                {
                    break;
                }
                p_visitor.PathQuadraticTo(t_opcode == kMCGDrawingPathOpcodeRelativeQuadraticTo, t_ax, t_ay, t_x, t_y);
            }
            break;
                
            case kMCGDrawingPathOpcodeSmoothQuadraticTo:
            case kMCGDrawingPathOpcodeRelativeSmoothQuadraticTo:
            {
                MCGFloat t_x, t_y;
                if (!CoordinateScalar(t_x) || !CoordinateScalar(t_y))
                {
                    break;
                }
                p_visitor.PathSmoothQuadraticTo(t_opcode == kMCGDrawingPathOpcodeRelativeSmoothQuadraticTo, t_x, t_y);
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
                MCGFloat t_rx, t_ry, t_rotation, t_x, t_y;
                if (!LengthScalar(t_rx) || !LengthScalar(t_ry) ||
                    !AngleScalar(t_rotation) ||
                    !CoordinateScalar(t_x) || !CoordinateScalar(t_y))
                {
                    break;
                }
                
                bool t_is_relative =
                        MCGDrawingPathOpcodeIsRelativeArc(t_opcode);
                bool t_is_reflex =
                        MCGDrawingPathOpcodeIsReflexArc(t_opcode);
                bool t_is_reverse =
                        MCGDrawingPathOpcodeIsReverseArc(t_opcode);
                p_visitor.PathArcTo(t_is_relative, t_is_reflex, t_is_reverse, t_rx, t_ry, t_rotation, t_x, t_y);
            }
            break;
                
            case kMCGDrawingPathOpcodeBearing:
            {
                MCGFloat t_angle;
                if (!AngleScalar(t_angle))
                {
                    break;
                }
                p_visitor.PathBearing(t_angle);
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
    
    bool paint_is_for_stroke = false;
    bool paint_set = false;
    MCGAffineTransform current_transform;
    
    MCGPoint first_point = {0, 0};
    MCGPoint last_point = {0, 0};
    MCGPoint last_control_point = {0, 0};
    MCGDrawingPathOpcode last_curve_opcode = kMCGDrawingPathOpcodeEnd;
    
    /**/
    
    MCGPoint Point(bool p_is_relative, MCGFloat p_x, MCGFloat p_y)
    {
        if (p_is_relative)
        {
            return MCGPointMake(last_point.x + p_x, last_point.y + p_y);
        }
        return MCGPointMake(p_x, p_y);
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
    
    void TransformBegin(void)
    {
        current_transform = MCGAffineTransformMakeIdentity();
    }
    
    void TransformAffine(MCGFloat a, MCGFloat b, MCGFloat c, MCGFloat d, MCGFloat tx, MCGFloat ty)
    {
        current_transform = MCGAffineTransformConcat(current_transform,
                                                     MCGAffineTransformMake(a, b, c, d, tx, ty));
    }

    void TransformEnd(void)
    {
        MCGContextSetTransform(gcontext, current_transform);
    }    
    
    /**/
    
    void PaintBegin(bool p_is_stroke)
    {
        paint_is_for_stroke = p_is_stroke;
        paint_set = false;
    }
    
    void PaintNone(void)
    {
        if (paint_set)
        {
            return;
        }
        
        if (!paint_is_for_stroke)
        {
            MCGContextSetFillNone(gcontext);
        }
        else
        {
            MCGContextSetStrokeNone(gcontext);
        }
        
        paint_set = true;
    }
    
    void PaintSolidColor(MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue)
    {
        if (paint_set)
        {
            return;
        }
        
        if (!paint_is_for_stroke)
        {
            MCGContextSetFillRGBAColor(gcontext, p_red, p_green, p_blue, 1.0);
        }
        else
        {
            MCGContextSetStrokeRGBAColor(gcontext, p_red, p_green, p_blue, 1.0);
        }
        
        paint_set = true;
    }
    
    void PaintEnd(void)
    {
        if (!paint_set)
        {
            PaintNone();
        }
    }
    
    /**/
    
    void PathBegin(void)
    {
        MCGContextBeginPath(gcontext);
        
        last_curve_opcode = kMCGDrawingPathOpcodeEnd;
        first_point = last_point = MCGPointMake(0.0, 0.0);
    }
    
    void PathMoveTo(bool p_is_relative, MCGFloat p_x, MCGFloat p_y)
    {
        MCGPoint t_point = Point(p_is_relative, p_x, p_y);
        MCGContextMoveTo(gcontext, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeEnd;
        first_point = last_point = t_point;
    }
    
    void PathLineTo(bool p_is_relative, MCGFloat p_x, MCGFloat p_y)
    {
        MCGPoint t_point = Point(p_is_relative, p_x, p_y);
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
    
    void PathCubicTo(bool p_is_relative, MCGFloat p_ax, MCGFloat p_ay, MCGFloat p_bx, MCGFloat p_by, MCGFloat p_x, MCGFloat p_y)
    {
        MCGPoint t_a = Point(p_is_relative, p_ax, p_ay);
        MCGPoint t_b = Point(p_is_relative, p_bx, p_by);
        MCGPoint t_point = Point(p_is_relative, p_x, p_y);
        MCGContextCubicTo(gcontext, t_a, t_b, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeCubicTo;
        last_control_point = t_b;
        last_point = t_point;
    }
    
    void PathSmoothCubicTo(bool p_is_relative, MCGFloat p_bx, MCGFloat p_by, MCGFloat p_x, MCGFloat p_y)
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
        
        MCGPoint t_b = Point(p_is_relative, p_bx, p_by);
        MCGPoint t_point = Point(p_is_relative, p_x, p_y);
        MCGContextCubicTo(gcontext, t_a, t_b, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeCubicTo;
        last_control_point = t_b;
        last_point = t_point;
    }
    
    void PathQuadraticTo(bool p_is_relative, MCGFloat p_ax, MCGFloat p_ay, MCGFloat p_x, MCGFloat p_y)
    {
        MCGPoint t_a = Point(p_is_relative, p_ax, p_ay);
        MCGPoint t_point = Point(p_is_relative, p_x, p_y);
        MCGContextQuadraticTo(gcontext, t_a, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeQuadraticTo;
        last_control_point = t_a;
        last_point = t_point;
    }
    
    void PathSmoothQuadraticTo(bool p_is_relative, MCGFloat p_x, MCGFloat p_y)
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
        
        MCGPoint t_point = Point(p_is_relative, p_x, p_y);
        MCGContextQuadraticTo(gcontext, t_a, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeCubicTo;
        last_control_point = t_a;
        last_point = t_point;
    }
    
    void PathArcTo(bool p_is_relative, bool p_is_reflex, bool p_is_reverse, MCGFloat p_rx, MCGFloat p_ry, MCGFloat p_rotation, MCGFloat p_x, MCGFloat p_y)
    {
        MCGSize t_radii = MCGSizeMake(p_rx, p_ry);
        MCGPoint t_point = Point(p_is_relative, p_x, p_y);
        MCGContextArcTo(gcontext, t_radii, p_rotation, p_is_reflex, p_is_reverse, t_point);
        
        last_curve_opcode = kMCGDrawingPathOpcodeEnd;
        last_point = t_point;
    }
    
    void PathCloseSubpath(void)
    {
        MCGContextCloseSubpath(gcontext);
        last_point = first_point;
    }
    
    void PathBearing(MCGFloat p_angle)
    {
    }
    
    void PathEnd(void)
    {
        MCGContextFillAndStroke(gcontext);
    }
    
    /**/
    
    void Start(MCGRectangle p_viewport, const MCGRectangle* p_viewbox, MCGDrawingPreserveAspectRatioOpcode p_par)
    {
        MCGContextSave(gcontext);
        MCGContextSetShouldAntialias(gcontext, true);
        
        if (p_viewbox != nullptr)
        {
            float t_scale_x, t_scale_y;
            if (p_viewbox->size.width != 0)
            {
                t_scale_x = p_viewport.size.width / p_viewbox->size.width;
            }
            else
            {
                t_scale_x = 0;
            }
            
            if (p_viewbox->size.height != 0)
            {
                t_scale_y = p_viewport.size.height / p_viewbox->size.height;
            }
            else
            {
                t_scale_y = 0;
            }
            
            if (MCGDrawingPreserveAspectRatioIsMeet(p_par))
            {
                t_scale_x = t_scale_y = MCMin(t_scale_x, t_scale_y);
            }
            else if (MCGDrawingPreserveAspectRatioIsSlice(p_par))
            {
                t_scale_x = t_scale_y = MCMax(t_scale_x, t_scale_y);
            }
            
            float t_translate_x, t_translate_y;
            t_translate_x = p_viewport.origin.x - (p_viewbox->origin.x * t_scale_x);
            t_translate_y = p_viewport.origin.y - (p_viewbox->origin.y * t_scale_y);
            
            if (MCGDrawingPreserveAspectRatioIsXMid(p_par))
            {
                t_translate_x += (p_viewport.size.width - p_viewbox->size.width * t_scale_x) / 2;
            }
            else if (MCGDrawingPreserveAspectRatioIsXMax(p_par))
            {
                t_translate_x += (p_viewport.size.width - p_viewbox->size.width * t_scale_x);
            }
            
            if (MCGDrawingPreserveAspectRatioIsYMid(p_par))
            {
                t_translate_y += (p_viewport.size.height - p_viewbox->size.height * t_scale_y) / 2;
            }
            else if (MCGDrawingPreserveAspectRatioIsYMax(p_par))
            {
                t_translate_y += (p_viewport.size.height - p_viewbox->size.height * t_scale_y);
            }
            
            MCGContextConcatCTM(gcontext,
                                MCGAffineTransformPostScale(MCGAffineTransformMakeTranslation(t_translate_x, t_translate_y),
                                                            t_scale_x, t_scale_y));
        }
        else
        {
            MCGContextTranslateCTM(gcontext, p_viewport.origin.x, p_viewport.origin.y);
        }
        
        MCGContextSave(gcontext);
    }
    
    void Finish(bool p_error)
    {
        MCGContextRestore(gcontext);
        MCGContextRestore(gcontext);
    }
    
    void FillOpacity(MCGFloat p_opacity)
    {
        MCGContextSetFillOpacity(gcontext, p_opacity);
    }
    
    void FillRule(MCGFillRule p_fill_rule)
    {
        MCGContextSetFillRule(gcontext, p_fill_rule);
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
    
    void StrokeDashArray(size_t p_count, const MCGFloat* p_lengths)
    {
        MCGContextSetStrokeDashArray(gcontext, p_lengths, p_count);
    }
    
    void StrokeDashOffset(MCGFloat p_offset)
    {
        MCGContextSetStrokeDashOffset(gcontext, p_offset);
    }
    
    void StrokeMiterLimit(MCGFloat p_miter_limit)
    {
        MCGContextSetStrokeMiterLimit(gcontext, p_miter_limit);
    }
    
    void Rectangle(MCGFloat p_x, MCGFloat p_y, MCGFloat p_width, MCGFloat p_height, MCGFloat p_rx, MCGFloat p_ry)
    {
        if (p_rx == 0.0 && p_ry == 0.0)
        {
            MCGContextAddRectangle(gcontext, MCGRectangleMake(p_x, p_y, p_width, p_height));
        }
        else
        {
            MCGContextAddRoundedRectangle(gcontext, MCGRectangleMake(p_x, p_y, p_width, p_height), MCGSizeMake(p_rx, p_ry));
        }
        MCGContextFillAndStroke(gcontext);
    }
    
    void Circle(MCGFloat p_cx, MCGFloat p_cy, MCGFloat p_r)
    {
        MCGContextAddEllipse(gcontext, MCGPointMake(p_cx, p_cy), MCGSizeMake(p_r, p_r), 0.0);
        MCGContextFillAndStroke(gcontext);
    }
    
    void Ellipse(MCGFloat p_cx, MCGFloat p_cy, MCGFloat p_rx, MCGFloat p_ry)
    {
        MCGContextAddEllipse(gcontext, MCGPointMake(p_cx, p_cy), MCGSizeMake(p_rx, p_ry), 0.0);
        MCGContextFillAndStroke(gcontext);
    }
    
    void Line(MCGFloat p_x1, MCGFloat p_y1, MCGFloat p_x2, MCGFloat p_y2)
    {
        MCGContextAddLine(gcontext, MCGPointMake(p_x1, p_y1), MCGPointMake(p_x2, p_y2));
        MCGContextFillAndStroke(gcontext);
    }
    
    void Polyline(size_t p_count, const MCGPoint* p_points)
    {
        MCGContextAddPolyline(gcontext, p_points, uindex_t(p_count));
        MCGContextFillAndStroke(gcontext);
    }
    
    void Polygon(size_t p_count, const MCGPoint* p_points)
    {
        MCGContextAddPolygon(gcontext, p_points, uindex_t(p_count));
        MCGContextFillAndStroke(gcontext);
    }
};

/* MCGContextPlayback renders the specified drawing into p_dst_rect of p_gcontext. */
void MCGContextPlayback(MCGContextRef p_gcontext, MCGRectangle p_dst_rect, const void *p_drawing, size_t p_drawing_byte_size)
{
    MCGDrawingRenderVisitor t_render_visitor;
    t_render_visitor.gcontext = p_gcontext;
    
    MCGDrawingContext t_context(p_drawing, p_drawing_byte_size);
    t_context.Execute(t_render_visitor, p_dst_rect);
}

/******************************************************************************/
