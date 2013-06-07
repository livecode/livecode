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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "stack.h"
#include "image.h"
#include "param.h"
#include "card.h"

#include "exec.h"
#include "mblsyntax.h"

////////////////////////////////////////////////////////////////////////////////

//bool MCSystemActivityIndicatorStart (MCActivityIndicatorType p_indicator, MCLocation p_location);
//bool MCSystemActivityIndicatorStop ();

////////////////////////////////////////////////////////////////////////////////

//// MOVED TO exec-activityindicator.cpp
//void MCActivityIndicatorExecStart (MCExecContext& ctxt, MCActivityIndicatorType p_indicator, MCLocation p_location)
//{
//    MCSystemActivityIndicatorStart (p_indicator, p_location);
//}
//
//void MCActivityIndicatorExecStop (MCExecContext& ctxt)
//{
//    MCSystemActivityIndicatorStop ();
//}


//// MOVED TO mblhanders.cpp
//static MCActivityIndicatorType MCActivityIndicatorTypeFromCString(const char *p_string)
//{
//    if (MCCStringEqualCaseless(p_string, "white"))
//        return kMCActivityIndicatorWhite;
//    else if (MCCStringEqualCaseless(p_string, "large white"))
//        return kMCActivityIndicatorWhiteLarge;
//    else if (MCCStringEqualCaseless(p_string, "gray"))
//        return kMCActivityIndicatorGray;
//    
//    return kMCActivityIndicatorWhite;
//}
//
//static bool MCActivityIndicatorTypeToCString(MCSensorType p_indicator, char *&r_string)
//{
//    switch (p_indicator)
//    {
//        case kMCActivityIndicatorWhite:
//            return MCCStringClone("white", r_string);
//        case kMCActivityIndicatorWhiteLarge:
//            return MCCStringClone("large white", r_string);
//        case kMCActivityIndicatorGray:
//            return MCCStringClone("gray", r_string);
//        default:
//            return MCCStringClone("unknown", r_string);
//    }
//    return false;
//}
//
//Exec_stat MCHandleStartActivityIndicator(void *p_context, MCParameter *p_parameters)
//{
//    MCActivityIndicatorType t_indicator_type;
//    t_indicator_type = kMCActivityIndicatorWhite;
//    
//    char *t_style;
//    t_style = nil;
//    
//    MCLocation t_location;
//    t_location.x = -1;
//    t_location.y = -1;
//    
//    MCExecPoint ep(nil, nil, nil);
//    if (p_parameters != nil)
//    {
//        p_parameters->eval(ep);
//        // Provide backwards compatibility here for "whiteLarge" rather than "large white".
//        if (MCCStringEqualCaseless (ep.getsvalue().getstring(), "whiteLarge"))
//            MCCStringClone("large white", t_style);
//        else
//            t_style = ep.getsvalue().clone();
//        if (t_style != nil)
//            p_parameters = p_parameters->getnext();
//    }
//    
//#ifdef MOBILE_BROKEN
//    if (p_parameters != nil)
//    {
//        p_parameters->eval(ep);
//        if (ep.getformat() != VF_STRING || ep.ston() == ES_NORMAL)
//        {
//            t_location.x = ep.getint4();
//            p_parameters = p_parameters->getnext();
//            if (p_parameters != nil)
//            {
//                p_parameters->eval(ep);
//                if (ep.getformat() != VF_STRING || ep.ston() == ES_NORMAL)
//                {
//                    t_location.y = ep.getint4();
//                    p_parameters = p_parameters->getnext();
//                }
//            }
//        }
//        if (t_location.y == -1)
//            t_location.x = -1;
//    }
//#endif
//
//    MCExecContext ctxt(ep);
//	ctxt . SetTheResultToEmpty();
//
//	if (t_style != nil)
//		t_indicator_type = MCActivityIndicatorTypeFromCString(t_style);
//    MCActivityIndicatorExecStart(ctxt, t_indicator_type, t_location);
//
//	if (!ctxt . HasError())
//		return ES_NORMAL;
//
//	return ES_ERROR;
//}
//
//Exec_stat MCHandleStopActivityIndicator(void *p_context, MCParameter *p_parameters)
//{
//    MCExecPoint ep(nil, nil, nil);
//    
//    MCExecContext ctxt(ep);
//	ctxt . SetTheResultToEmpty();
//    
//    MCActivityIndicatorExecStop(ctxt);
//    
//	if (!ctxt . HasError())
//		return ES_NORMAL;
//
//	return ES_ERROR;
//
//}
