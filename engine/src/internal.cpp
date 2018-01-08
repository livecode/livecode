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
//
//  Private Source File:
//    internal.cpp
//
//  Description:
//    This file contains the implementation of the internal engine command
//    syntax. Originally this was ide-only and hardcoded. It is now table-driven
//    with the verb table being specified per-mode.
//
//  Todo:
//    2009-06-29 MW Add support for generate_uuid for linux where libuuid is not
//                  available.
//
//  Changes:
//    2009-06-29 MW Added generate_uuid implementation for Linux based using
//                  libuuid.
//    2009-07-19 MW Added support for _internal sign command.
//    2010-05-09 MW Refactored into table-driven approach.
//                  Removed obsolete hooks and methods.
//
////////////////////////////////////////////////////////////////////////////////

#include "prefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "exec.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "express.h"

#include "debug.h"
#include "globals.h"
#include "internal.h"

////////////////////////////////////////////////////////////////////////////////

extern MCInternalVerbInfo MCinternalverbs[];
extern MCInternalVerbInfo MCinternalverbs_base[];

MCInternal::~MCInternal(void)
{
	delete f_statement;
}

Parse_stat MCInternal::parse(MCScriptPoint& sp)
{
	Symbol_type t_type;
	
	initpoint(sp);

	if (sp . next(t_type) != PS_NORMAL || t_type != ST_ID)
	{
		MCperror -> add(PE_INTERNAL_BADVERB, sp);
		return PS_ERROR;
	}

	// Get the first token
    MCAutoStringRef t_first_token;
    t_first_token = sp . gettoken_stringref();

	// Look for a match on the first token.
	for(uint32_t i = 0; MCinternalverbs[i] . first_token != nil; i++)
        if (MCStringIsEqualToCString(*t_first_token, MCinternalverbs[i] . first_token, kMCCompareExact))
		{
			// If the second token is non-nil then check for a match
			if (MCinternalverbs[i] . second_token != nil)
			{
				// No next token means try the next entry
				if (sp . next(t_type) != PS_NORMAL)
					continue;
				
				// If the next token isn't an id, or doesn't match try
				// the next entry
				if (t_type != ST_ID ||
                    !MCStringIsEqualToCString(sp . gettoken_stringref(), MCinternalverbs[i] . second_token, kMCCompareExact))
				{
					sp . backup();
					continue;
				}
			}

			// We've found a match, so construct the statement.
			f_statement = MCinternalverbs[i] . factory();
		}

    if (f_statement == nullptr)
    {
        // Look for a match on the first token.
        for(uint32_t i = 0; MCinternalverbs_base[i] . first_token != nil; i++)
            if (MCStringIsEqualToCString(*t_first_token, MCinternalverbs_base[i] . first_token, kMCCompareExact))
            {
                // If the second token is non-nil then check for a match
                if (MCinternalverbs_base[i] . second_token != nil)
                {
                    // No next token means try the next entry
                    if (sp . next(t_type) != PS_NORMAL)
                        continue;
                    
                    // If the next token isn't an id, or doesn't match try
                    // the next entry
                    if (t_type != ST_ID ||
                        !MCStringIsEqualToCString(sp . gettoken_stringref(), MCinternalverbs_base[i] . second_token, kMCCompareExact))
                    {
                        sp . backup();
                        continue;
                    }
                }

                // We've found a match, so construct the statement.
                f_statement = MCinternalverbs_base[i] . factory();
            }
    }
    
	if (f_statement == nil)
	{
		MCperror -> add(PE_INTERNAL_BADVERB, sp);
		return PS_ERROR;
	}

	if (f_statement -> parse(sp) != PS_NORMAL)
		return PS_ERROR;

	Parse_stat t_stat;
	t_stat = sp . next(t_type);
	if (t_stat != PS_EOF && t_stat != PS_EOL)
	{
		MCperror -> add(PE_INTERNAL_BADEOS, sp);
		return PS_ERROR;
	}

	return PS_NORMAL;
}

void MCInternal::exec_ctxt(MCExecContext &ctxt)
{
    f_statement -> exec_ctxt(ctxt);
    if (ctxt . GetExecStat() != ES_NORMAL)
        ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

#include "module-canvas.h"
#include "module-canvas-internal.h"

class MCInternalVectorPathGetBBox: public MCStatement
{
public:
	MCInternalVectorPathGetBBox(void)
	{
		m_path_string = nil;
	}

	~MCInternalVectorPathGetBBox(void)
	{
		delete m_path_string;
	}

	Parse_stat parse(MCScriptPoint& sp)
	{
		if (sp . parseexp(False, True, &m_path_string) != PS_NORMAL)
		{
			MCperror -> add(PE_PUT_BADEXP, sp);
			return PS_ERROR;
		}

		return PS_NORMAL;
	}

    void exec_ctxt(MCExecContext &ctxt)
    {
        MCAutoStringRef t_path_string;
        if (!ctxt.EvalExprAsStringRef(m_path_string, EE_PROPERTY_NAS, &t_path_string))
            return;
        
        MCCanvasPathRef t_canvas_path = nullptr;
        MCCanvasPathMakeWithInstructionsAsString(*t_path_string, t_canvas_path);
        if (MCErrorIsPending())
        {
            ctxt.Throw();
            return;
        }
        
        MCGPathRef t_path = MCCanvasPathGetMCGPath(t_canvas_path);
        MCGRectangle t_bbox;
        if (!MCGPathGetBoundingBox(t_path, t_bbox))
        {
            ctxt.Throw();
            MCValueRelease(t_canvas_path);
            return;
        }
        
        MCValueRelease(t_canvas_path);
        
        MCAutoStringRef t_bbox_string;
        if (!MCStringFormat(&t_bbox_string, "%.10lf,%.10lf,%.10lf,%.10lf", t_bbox.origin.x, t_bbox.origin.y, t_bbox.origin.x+t_bbox.size.width, t_bbox.origin.y + t_bbox.size.height))
        {
            ctxt.Throw();
            return;
        }
        
        ctxt.SetTheResultToValue(*t_bbox_string);
    }
    
private:
    MCExpression *m_path_string;
};

template<bool(*Op)(MCExecContext& ctxt, MCGShapeRef shape)>
class MCInternalShape: public MCStatement
{
public:
    MCInternalShape(void)
    {
        m_shape = nil;
    }
    
    ~MCInternalShape(void)
    {
        delete m_shape;
    }
    
    Parse_stat parse(MCScriptPoint& sp)
    {
        if (sp.parseexp(False, True, &m_shape) != PS_NORMAL)
        {
            MCperror -> add(PE_PUT_BADEXP, sp);
            return PS_ERROR;
        }
        
        return PS_NORMAL;
    }
    
    void exec_ctxt(MCExecContext &ctxt)
    {
        MCAutoArrayRef t_array;
        if (!ctxt.EvalExprAsArrayRef(m_shape, EE_PROPERTY_NOTANARRAY, &t_array))
        {
            ctxt.Throw();
            return;
        }
        
        MCGShapeRef t_shape = nullptr;
        if (!execute(ctxt, *t_array, t_shape))
        {
            ctxt.Throw();
            return;
        }
        
        Op(ctxt, t_shape);
        MCGShapeRelease(t_shape);
    }
    
    bool execute(MCExecContext& ctxt, MCArrayRef p_array, MCGShapeRef& r_shape)
    {
        MCAutoStringRef t_type;
        if (!ctxt.CopyElementAsString(p_array, MCNAME("type"), false, &t_type))
        {
            return false;
        }
        
        MCGShapeRef t_shape = nullptr;
        if (MCStringIsEqualToCString(*t_type, "rectangle", kMCStringOptionCompareCaseless))
        {
            MCGFloat t_x, t_y, t_width, t_height;
            if (!parse_array(ctxt, p_array, "fx", &t_x, "fy", &t_y, "fwidth", &t_width, "fheight", &t_height, nullptr))
            {
                return false;
            }
            MCGShapeCreateRectangle(MCGRectangleMake(t_x, t_y, t_width, t_height), t_shape);
        }
        else if (MCStringIsEqualToCString(*t_type, "rounded-rectangle", kMCStringOptionCompareCaseless))
        {
            MCGFloat t_x, t_y, t_width, t_height, t_rx, t_ry;
            if (!parse_array(ctxt, p_array, "fx", &t_x, "fy", &t_y, "fwidth", &t_width, "fheight", &t_height, "frx", &t_rx, "fry", &t_ry, nullptr))
            {
                return false;
            }
            MCGShapeCreateRoundedRectangle(MCGRectangleMake(t_x, t_y, t_width, t_height), MCGSizeMake(t_rx, t_ry), t_shape);
        }
        else if (MCStringIsEqualToCString(*t_type, "ellipse", kMCStringOptionCompareCaseless))
        {
            MCGFloat t_cx, t_cy, t_rx, t_ry;
            if (!parse_array(ctxt, p_array, "fcx", &t_cx, "fcy", &t_cy, "frx", &t_rx, "fry", &t_ry, nullptr))
            {
                return false;
            }
            MCGShapeCreateEllipse(MCGPointMake(t_cx, t_cy), MCGSizeMake(t_rx, t_ry), t_shape);
        }
        else if (MCStringIsEqualToCString(*t_type, "line", kMCStringOptionCompareCaseless))
        {
            MCGFloat t_x1, t_y1, t_x2, t_y2;
            if (!parse_array(ctxt, p_array, "fx1", &t_x1, "fy1", &t_y1, "fx2", &t_x2, "fy2", &t_y2, nullptr))
            {
                return false;
            }
            MCGShapeCreateLine(MCGPointMake(t_x1, t_y1), MCGPointMake(t_x2, t_y2), t_shape);
        }
        else if (MCStringIsEqualToCString(*t_type, "polyline", kMCStringOptionCompareCaseless))
        {
        }
        else if (MCStringIsEqualToCString(*t_type, "polygon", kMCStringOptionCompareCaseless))
        {
        }
        else if (MCStringIsEqualToCString(*t_type, "path", kMCStringOptionCompareCaseless))
        {
            MCAutoStringRef t_path_string;
            if (!parse_array(ctxt, p_array, "sd", &(&t_path_string), nullptr))
            {
                return false;
            }
            
            MCAutoValueRefBase<MCCanvasPathRef> t_canvas_path;
            MCCanvasPathMakeWithInstructionsAsString(*t_path_string, &t_canvas_path);
            if (MCErrorIsPending())
            {
                ctxt.Throw();
                return false;
            }
            
            MCGPathRef t_path = MCCanvasPathGetMCGPath(*t_canvas_path);
            
            MCGShapeCreatePath(t_path, t_shape);
        }
        else if (MCStringIsEqualToCString(*t_type, "transform", kMCStringOptionCompareCaseless))
        {
            MCAutoArrayRef t_shape_array;
            MCGAffineTransform t_t;
            if (!parse_array(ctxt, p_array, "ashape", &(&t_shape_array), "fa", &t_t.a, "fb", &t_t.b, "fc", &t_t.c, "fd", &t_t.d, "ftx", &t_t.tx, "fty", &t_t.ty, nullptr))
            {
                return false;
            }
            MCGShapeRef t_base_shape;
            if (!execute(ctxt, *t_shape_array, t_base_shape))
            {
                return false;
            }
            MCGShapeTransform(t_base_shape, t_t, t_shape);
            MCGShapeRelease(t_base_shape);
        }
        else if (MCStringIsEqualToCString(*t_type, "fill", kMCStringOptionCompareCaseless))
        {
            MCAutoArrayRef t_shape_array;
            MCAutoStringRef t_rule_string;
            if (!parse_array(ctxt, p_array, "ashape", &(&t_shape_array), "srule", &(&t_rule_string), nullptr))
            {
                return false;
            }
            MCGFillRule t_rule;
            if (MCStringIsEqualToCString(*t_rule_string, "non-zero", kMCStringOptionCompareCaseless))
            {
                t_rule = kMCGFillRuleNonZero;
            }
            else if (MCStringIsEqualToCString(*t_rule_string, "even-odd", kMCStringOptionCompareCaseless))
            {
                t_rule = kMCGFillRuleEvenOdd;
            }
            else
            {
                return false;
            }
            MCGShapeRef t_base_shape;
            if (!execute(ctxt, *t_shape_array, t_base_shape))
            {
                return false;
            }
            MCGShapeFill(t_base_shape, t_rule, t_shape);
            MCGShapeRelease(t_base_shape);
        }
        else if (MCStringIsEqualToCString(*t_type, "dash", kMCStringOptionCompareCaseless))
        {
        }
        else if (MCStringIsEqualToCString(*t_type, "thicken", kMCStringOptionCompareCaseless))
        {
            MCAutoArrayRef t_shape_array;
            MCGFloat t_width, t_miter_limit;
            MCAutoStringRef t_cap_string, t_join_string;
            if (!parse_array(ctxt, p_array, "ashape", &(&t_shape_array), "fwidth", &t_width, "scap", &(&t_cap_string), "sjoin", &(&t_join_string), "fmiter-limit", &t_miter_limit, nullptr))
            {
                return false;
            }
            
            MCGCapStyle t_cap;
            if (MCStringIsEqualToCString(*t_cap_string, "butt", kMCStringOptionCompareCaseless))
            {
                t_cap = kMCGCapStyleButt;
            }
            else if (MCStringIsEqualToCString(*t_cap_string, "round", kMCStringOptionCompareCaseless))
            {
                t_cap = kMCGCapStyleRound;
            }
            else if (MCStringIsEqualToCString(*t_cap_string, "square", kMCStringOptionCompareCaseless))
            {
                t_cap = kMCGCapStyleSquare;
            }
            else
            {
                return false;
            }
            
            MCGJoinStyle t_join;
            if (MCStringIsEqualToCString(*t_join_string, "bevel", kMCStringOptionCompareCaseless))
            {
                t_join = kMCGJoinStyleBevel;
            }
            else if (MCStringIsEqualToCString(*t_join_string, "round", kMCStringOptionCompareCaseless))
            {
                t_join = kMCGJoinStyleRound;
            }
            else if (MCStringIsEqualToCString(*t_join_string, "miter", kMCStringOptionCompareCaseless))
            {
                t_join = kMCGJoinStyleMiter;
            }
            else
            {
                return false;
            }
            
            MCGShapeRef t_base_shape;
            if (!execute(ctxt, *t_shape_array, t_base_shape))
            {
                return false;
            }
            MCGShapeThicken(t_base_shape, t_width, t_cap, t_join, t_miter_limit, t_shape);
            MCGShapeRelease(t_base_shape);
        }
        else
        {
            MCGShapeOperation t_op;
            if (MCStringIsEqualToCString(*t_type, "union", kMCStringOptionCompareCaseless))
            {
                t_op = kMCGShapeOperationUnion;
            }
            else if (MCStringIsEqualToCString(*t_type, "intersect", kMCStringOptionCompareCaseless))
            {
                t_op = kMCGShapeOperationIntersect;
            }
            else if (MCStringIsEqualToCString(*t_type, "difference", kMCStringOptionCompareCaseless))
            {
                t_op = kMCGShapeOperationDifference;
            }
            else if (MCStringIsEqualToCString(*t_type, "xor", kMCStringOptionCompareCaseless))
            {
                t_op = kMCGShapeOperationXor;
            }
            else if (MCStringIsEqualToCString(*t_type, "append", kMCStringOptionCompareCaseless))
            {
                t_op = kMCGShapeOperationAppend;
            }
            else if (MCStringIsEqualToCString(*t_type, "extend", kMCStringOptionCompareCaseless))
            {
                t_op = kMCGShapeOperationExtend;
            }
            else
            {
                return false;
            }
            
            MCAutoArrayRef t_left_array, t_right_array;
            if (!parse_array(ctxt, p_array, "aleft", &(&t_left_array), "aright", &(&t_right_array), nullptr))
            {
                return false;
            }
            
            MCGShapeRef t_left;
            if (!execute(ctxt, *t_left_array, t_left))
            {
                return false;
            }
            
            MCGShapeRef t_right;
            if (!execute(ctxt, *t_right_array, t_right))
            {
                MCGShapeRelease(t_left);
                return false;
            }
            
            MCGShapeCombine(t_left, t_op, t_right, t_shape);
            MCGShapeRelease(t_left);
            MCGShapeRelease(t_right);
        }
        
        if (t_shape == nullptr)
        {
            return false;
        }
        
        r_shape = t_shape;
        
        return true;
    }
    
    bool parse_array(MCExecContext& ctxt, MCArrayRef p_array, ...)
    {
        bool t_success = true;
        
        va_list t_args;
        va_start(t_args, p_array);
        while(t_success)
        {
            const char *t_tag = va_arg(t_args, const char *);
            if (t_tag == nullptr)
            {
                break;
            }

            char t_type = t_tag[0];
            const char *t_name = t_tag + 1;
            
            MCValueRef t_value;
            if (!MCArrayFetchValue(p_array, false, MCNAME(t_name), t_value))
            {
                t_success = false;
                break;
            }
            
            switch(t_type)
            {
                case 'a':
                    MCArrayRef *t_array_ptr;
                    t_array_ptr = va_arg(t_args, MCArrayRef*);
                    MCArrayRef t_array_value;
                    if (!ctxt.ConvertToArray(t_value, t_array_value))
                    {
                        t_success = false;
                        break;
                    }
                    *t_array_ptr = t_array_value;
                    break;
                case 'f':
                    float *t_float_ptr;
                    t_float_ptr = va_arg(t_args, float*);
                    double t_float_value;
                    if (!ctxt.ConvertToReal(t_value, t_float_value))
                    {
                        t_success = false;
                        break;
                    }
                    *t_float_ptr = (float)t_float_value;
                    break;
                case 's':
                    MCStringRef *t_string_ptr;
                    t_string_ptr = va_arg(t_args, MCStringRef*);
                    MCStringRef t_string_value;
                    if (!ctxt.ConvertToString(t_value, t_string_value))
                    {
                        t_success = false;
                        break;
                    }
                    *(MCStringRef *)t_string_ptr = t_string_value;
                    break;
                default:
                    t_success = false;
                    break;
            }
        }
            
        va_end(t_args);
            
        return t_success;
    }
    
private:
    MCExpression *m_shape;
};

bool MCGPathGetSVGData(MCGPathRef p_path, MCStringRef &r_string);
static bool MCInternalShapeAsString(MCExecContext& ctxt, MCGShapeRef p_shape)
{
    MCGPathRef t_path;
    if (!MCGShapeFlatten(p_shape, t_path))
    {
        return false;
    }
    
    MCAutoStringRef t_path_string;
    bool t_success = MCGPathGetSVGData(t_path, &t_path_string);
    MCGPathRelease(t_path);
    
    if (!t_success)
    {
        return false;
    }

    ctxt.SetTheResultToValue(*t_path_string);
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class T> inline MCStatement *class_factory(void)
{
	return new T;
}

MCInternalVerbInfo MCinternalverbs_base[] =
{
	{ "vectorpath", "getbbox", class_factory<MCInternalVectorPathGetBBox> },
    { "shape", "as_string", class_factory<MCInternalShape<MCInternalShapeAsString>> },
    { nullptr, nullptr, nullptr},
};

////////////////////////////////////////////////////////////////////////////////
