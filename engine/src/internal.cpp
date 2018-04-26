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

////////////////////////////////////////////////////////////////////////////////

template<class T> inline MCStatement *class_factory(void)
{
	return new T;
}

MCInternalVerbInfo MCinternalverbs_base[] =
{
	{ "vectorpath", "getbbox", class_factory<MCInternalVectorPathGetBBox> },
    { nullptr, nullptr, nullptr},
};

////////////////////////////////////////////////////////////////////////////////
