/* Copyright (C) 2018 LiveCode Ltd.
 
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

#include <SkPathOps.h>
#include <SkStrokeRec.h>
#include <SkPathMeasure.h>

/******************************************************************************/

template<typename T, typename ...ArgTs>
bool MCGShapeNew(MCGShapeRef& r_obj, ArgTs... p_args)
{
    T *t_obj = new (nothrow) T(p_args...);
    if (t_obj == nullptr)
    {
        return false;
    }
    r_obj = t_obj;
    return true;
}

class MCGShape: public MCGObject
{
public:
    virtual ~MCGShape(void) {}
    
    virtual bool Resolve(SkPath& r_path) = 0;
};

/**/

class MCGRectangleShape: public MCGShape
{
public:
    MCGRectangleShape(MCGRectangle p_rectangle)
        : m_rectangle(p_rectangle)
    {
    }
    
    bool Resolve(SkPath& r_path)
    {
        r_path.addRect(MCGRectangleToSkRect(m_rectangle));
        return true;
    }
    
private:
    MCGRectangle m_rectangle;
};

bool MCGShapeCreateRectangle(MCGRectangle p_rect, MCGShapeRef& r_shape)
{
    return MCGShapeNew<MCGRectangleShape>(r_shape, p_rect);
}

/**/

class MCGRoundedRectangleShape: public MCGShape
{
public:
    MCGRoundedRectangleShape(MCGRectangle p_rectangle, MCGSize p_radii)
        : m_rectangle(p_rectangle),
          m_radii(p_radii)
    {
    }
    
    bool Resolve(SkPath& r_path)
    {
        r_path.addRoundRect(MCGRectangleToSkRect(m_rectangle), m_radii.width, m_radii.height);
        return true;
    }

private:
    MCGRectangle m_rectangle;
    MCGSize m_radii;
};

bool MCGShapeCreateRoundedRectangle(MCGRectangle p_rect, MCGSize p_radii, MCGShapeRef& r_shape)
{
    return MCGShapeNew<MCGRoundedRectangleShape>(r_shape, p_rect, p_radii);
}

/**/

class MCGOvalShape: public MCGShape
{
public:
    MCGOvalShape(MCGRectangle p_rectangle)
        : m_rectangle(p_rectangle)
    {
    }
    
    bool Resolve(SkPath& r_path)
    {
        r_path.addOval(MCGRectangleToSkRect(m_rectangle));
        return true;
    }
    
private:
    MCGRectangle m_rectangle;
};

bool MCGShapeCreateEllipse(MCGPoint p_center, MCGSize p_radii, MCGShapeRef& r_shape)
{
    return MCGShapeNew<MCGOvalShape>(r_shape, MCGRectangleMake(p_center.x - p_radii.width, p_center.y - p_radii.height, p_radii.width * 2, p_radii.height * 2));
}

/**/

class MCGLineShape: public MCGShape
{
public:
    MCGLineShape(MCGPoint p_from, MCGPoint p_to)
        : m_from(p_from), m_to(p_to)
    {
    }
    
    bool Resolve(SkPath& r_path)
    {
        r_path.moveTo(m_from.x, m_from.y);
        r_path.lineTo(m_to.x, m_to.y);
        return true;
    }
    
private:
    MCGPoint m_from, m_to;
};

bool MCGShapeCreateLine(MCGPoint p_from, MCGPoint p_to, MCGShapeRef& r_shape)
{
    return MCGShapeNew<MCGLineShape>(r_shape, p_from, p_to);
}

/**/

bool MCGShapeCreatePolyline(const MCGPoint* p_points, uindex_t p_arity, MCGShapeRef& r_shape);
bool MCGShapeCreatePolygon(const MCGPoint* p_points, uindex_t p_arity, MCGShapeRef& r_shape);

/**/

class MCGPathShape: public MCGShape
{
public:
    MCGPathShape(MCGPathRef p_path)
    {
        m_path = MCGPathRetain(p_path);
    }
    
    ~MCGPathShape(void)
    {
        MCGPathRelease(m_path);
    }
    
    bool Resolve(SkPath& r_path)
    {
        r_path = *m_path->path;
        return true;
    }

private:
    MCGPathRef m_path = nullptr;
};

bool MCGShapeCreatePath(MCGPathRef p_path, MCGShapeRef& r_shape)
{
    return MCGShapeNew<MCGPathShape>(r_shape, p_path);
}
    
/**/

class MCGTransformedShape: public MCGShape
{
public:
    MCGTransformedShape(MCGShapeRef p_shape, MCGAffineTransform p_transform)
        : m_shape(p_shape),
          m_transform(p_transform)
    {
    }
    
    bool Resolve(SkPath& r_path)
    {
        if (!(*m_shape)->Resolve(r_path))
        {
            return false;
        }
        
        SkMatrix t_matrix;
        MCGAffineTransformToSkMatrix(m_transform, t_matrix);
        
        r_path.transform(t_matrix);
        
        return true;
    }

private:
    MCGAuto<MCGShapeRef> m_shape;
    MCGAffineTransform m_transform;
};

bool MCGShapeTransform(MCGShapeRef p_shape, MCGAffineTransform p_transform, MCGShapeRef& r_new_shape)
{
    return MCGShapeNew<MCGTransformedShape>(r_new_shape, p_shape, p_transform);
}

/**/

class MCGFilledShape: public MCGShape
{
public:
    MCGFilledShape(MCGShapeRef p_shape, MCGFillRule p_fill_rule)
        : m_shape(p_shape),
          m_fill_rule(p_fill_rule)
    {
    }
    
    bool Resolve(SkPath& r_path)
    {
        if (!(*m_shape)->Resolve(r_path))
        {
            return false;
        }
        
        r_path.setFillType(m_fill_rule == kMCGFillRuleNonZero ? SkPath::kWinding_FillType : SkPath::kEvenOdd_FillType);
        return Simplify(r_path, &r_path);
    }
    
private:
    MCGAuto<MCGShapeRef> m_shape;
    MCGFillRule m_fill_rule;
};

bool MCGShapeFill(MCGShapeRef p_shape, MCGFillRule p_fill_rule, MCGShapeRef& r_new_shape)
{
    return MCGShapeNew<MCGFilledShape>(r_new_shape, p_shape, p_fill_rule);
}

/**/

bool MCGShapeDash(MCGShapeRef p_shape, const MCGFloat *p_lengths, uindex_t p_arity, MCGFloat p_offset, MCGShapeRef& r_shape);

/**/

class MCGThickenedShape: public MCGShape
{
public:
    MCGThickenedShape(MCGShapeRef p_shape, MCGFloat p_width, MCGCapStyle p_cap, MCGJoinStyle p_join, MCGFloat p_miter_limit)
        : m_shape(p_shape),
          m_width(p_width),
          m_cap(p_cap),
          m_join(p_join),
          m_miter_limit(p_miter_limit)
    {
    }
    
    bool Resolve(SkPath& r_path)
    {
        if (!(*m_shape)->Resolve(r_path))
        {
            return false;
        }
        
        SkStrokeRec t_stroke(SkStrokeRec::kFill_InitStyle);
        t_stroke.setResScale(16.0);
        t_stroke.setStrokeStyle(m_width, false);
        t_stroke.setStrokeParams(MCGCapStyleToSkCapStyle(m_cap),
                                 MCGJoinStyleToSkJoinStyle(m_join),
                                 m_miter_limit);
        return t_stroke.applyToPath(&r_path, r_path);
    }
    
private:
    MCGAuto<MCGShapeRef> m_shape;
    MCGFloat m_width;
    MCGCapStyle m_cap;
    MCGJoinStyle m_join;
    MCGFloat m_miter_limit;
};

bool MCGShapeThicken(MCGShapeRef p_shape, MCGFloat p_width, MCGCapStyle p_cap, MCGJoinStyle p_join, MCGFloat p_miter_limit, MCGShapeRef& r_new_shape)
{
    return MCGShapeNew<MCGThickenedShape>(r_new_shape, p_shape, p_width, p_cap, p_join, p_miter_limit);
}

/**/

class MCGCombinedShape: public MCGShape
{
public:
    MCGCombinedShape(MCGShapeRef p_left, MCGShapeOperation p_operation, MCGShapeRef p_right)
        : m_left(p_left),
          m_right(p_right),
          m_operation(p_operation)
    {
    }
    
    bool Resolve(SkPath& r_path)
    {
        if (!(*m_left)->Resolve(r_path))
        {
            return false;
        }
        
        SkPath t_right;
        if (!(*m_right)->Resolve(t_right))
        {
            return false;
        
        }
        
        SkPathOp t_op;
        switch(m_operation)
        {
            case kMCGShapeOperationAppend:
                r_path.addPath(t_right, SkPath::kAppend_AddPathMode);
                return true;
            case kMCGShapeOperationExtend:
                r_path.addPath(t_right, SkPath::kExtend_AddPathMode);
                return true;
            case kMCGShapeOperationUnion:
                t_op = kUnion_SkPathOp;
                break;
            case kMCGShapeOperationIntersect:
                t_op = kIntersect_SkPathOp;
                break;
            case kMCGShapeOperationDifference:
                t_op = kDifference_SkPathOp;
                break;
            case kMCGShapeOperationXor:
                t_op = kXOR_SkPathOp;
                break;
        }
        
        return Op(r_path, t_right, t_op, &r_path);
    };
    
private:
    MCGAuto<MCGShapeRef> m_left;
    MCGAuto<MCGShapeRef> m_right;
    MCGShapeOperation m_operation;
};

bool MCGShapeCombine(MCGShapeRef p_shape, MCGShapeOperation p_operation, MCGShapeRef p_other_shape, MCGShapeRef& r_new_shape)
{
    return MCGShapeNew<MCGCombinedShape>(r_new_shape, p_shape, p_operation, p_other_shape);
}

/**/

MCGShapeRef MCGShapeRetain(MCGShapeRef p_shape)
{
    MCGRetain(p_shape);
    return p_shape;
}

void MCGShapeRelease(MCGShapeRef p_shape)
{
    MCGRelease(p_shape);
}

bool MCGShapeMeasure(MCGShapeRef p_shape, MCGFloat& r_length)
{
    SkPath t_path;
    if (!p_shape->Resolve(t_path))
    {
        return false;
    }
    
    SkPathMeasure t_measure(t_path, false);
    
    r_length = t_measure.getLength();
    
    return true;
}

bool MCGShapeHull(MCGShapeRef p_shape, MCGRectangle& r_hull)
{
    SkPath t_path;
    if (!p_shape->Resolve(t_path))
    {
        return false;
    }
    
    r_hull = MCGRectangleFromSkRect(t_path.getBounds());

    return true;
}

bool MCGShapeBounds(MCGShapeRef p_shape, MCGRectangle& r_bounds)
{
    SkPath t_path;
    if (!p_shape->Resolve(t_path))
    {
        return false;
    }
    
    SkRect t_bounds;
    if (!TightBounds(t_path, &t_bounds))
    {
        return false;
    }
    
    r_bounds = MCGRectangleFromSkRect(t_bounds);
    
    return true;
}

bool MCGShapeFlatten(MCGShapeRef p_shape, MCGPathRef& r_path)
{
    SkPath t_sk_path;
    if (!p_shape->Resolve(t_sk_path))
    {
        return false;
    }
    
    MCGPathRef t_path;
    if (!MCMemoryNew(t_path))
    {
        return false;
    }
    
    t_path->is_valid = true;
    t_path->is_mutable = false;
    t_path->references = 1;
    t_path->path = new (nothrow) SkPath(t_sk_path);
    
    r_path = t_path;
    
    return true;
    
}

/******************************************************************************/

