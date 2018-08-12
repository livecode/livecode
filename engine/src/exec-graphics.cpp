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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "uidc.h"
#include "util.h"
#include "image.h"
#include "image_rep.h"

#include "osspec.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

void MCGraphicsEvalIsAColor(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCColor t_color;
	if (ctxt . TryToConvertToLegacyColor(p_value, r_result, t_color))
		return;
		
	ctxt . Throw();
	
	/*MCAutoStringRef t_string;
	MCColor t_color;
	r_result = ctxt.ConvertToString(p_value, &t_string) && MCStringGetLength(*t_string) > 0 &&
		MCscreen->parsecolor(*t_string, t_color);*/
}

void MCGraphicsEvalIsNotAColor(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCGraphicsEvalIsAColor(ctxt, p_value, r_result);
	r_result = !r_result;
}

void MCGraphicsEvalIsAPoint(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCPoint t_point;
	if (ctxt . TryToConvertToLegacyPoint(p_value, r_result, t_point))
		return;
		
	ctxt . Throw();
	
	/*MCAutoStringRef t_string;
	int16_t t_int16;
	r_result = ctxt.ConvertToString(p_value, &t_string) && MCU_stoi2x2(*t_string, t_int16, t_int16);*/
}

void MCGraphicsEvalIsNotAPoint(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCGraphicsEvalIsAPoint(ctxt, p_value, r_result);
	r_result = !r_result;
}

void MCGraphicsEvalIsARectangle(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCRectangle t_rectangle;
	if (ctxt . TryToConvertToLegacyRectangle(p_value, r_result, t_rectangle))
		return;
		
	ctxt . Throw();

	/*MCAutoStringRef t_string;
	int16_t t_int16;
	r_result = ctxt.ConvertToString(p_value, &t_string) && MCU_stoi2x4(*t_string, t_int16, t_int16, t_int16, t_int16);*/
}

void MCGraphicsEvalIsNotARectangle(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCGraphicsEvalIsARectangle(ctxt, p_value, r_result);
	r_result = !r_result;
}

////////////////////////////////////////////////////////////////////////////////

void MCGraphicsEvalIsWithin(MCExecContext& ctxt, MCPoint p_point, MCRectangle p_rect, bool& r_result)
{
	r_result = p_point . x >= p_rect . x && p_point . x < p_rect . x + p_rect . width &&
		p_point . y >= p_rect . y && p_point . y < p_rect . y + p_rect . height;
}

void MCGraphicsEvalIsNotWithin(MCExecContext& ctxt, MCPoint p_point, MCRectangle p_rect, bool& r_result)
{
	MCGraphicsEvalIsWithin(ctxt, p_point, p_rect, r_result);
	r_result = !r_result;
}

////////////////////////////////////////////////////////////////////////////////

void MCGraphicsExecFlipSelection(MCExecContext& ctxt, bool p_horizontal)
{
    // MW-2013-07-01: [[ Bug 10999 ]] Throw an error if the image is not editable.
    if (MCactiveimage->getflag(F_HAS_FILENAME))
    {
        ctxt . LegacyThrow(EE_FLIP_NOTIMAGE);
        return;
    }
    
	if (MCactiveimage)
		MCactiveimage->flipsel(p_horizontal);
}

void MCGraphicsExecFlipImage(MCExecContext& ctxt, MCImage *p_image, bool p_horizontal)
{
	MColdtool = MCcurtool;

    // SN-2014-07-23: [[ Bug 12918 ]] Ported bugfix 11300 to 7.0
    // MW-2013-10-25: [[ Bug 11300 ]] If this is a reference image, then flip using
    //   transient flags in the image object.
    if (p_image -> getflag(F_HAS_FILENAME))
    {
        p_image -> flip(p_horizontal);
        return;
    }

	p_image->selimage();

	MCGraphicsExecFlipSelection(ctxt, p_horizontal);

	MCcurtool = MColdtool;
    
    // IM-2013-06-28: [[ Bug 10999 ]] ensure MCactiveimage is not null when calling endsel() method
    if (MCactiveimage)
        MCactiveimage -> endsel();
}

////////////////////////////////////////////////////////////////////////////////

void MCGraphicsRotateImage(MCImage *image, integer_t angle)
{
	angle %= 360;
	if (angle < 0)
		angle += 360;
	if (angle != 0 && image != NULL)
		image->rotatesel(angle);
}

void MCGraphicsExecRotateSelection(MCExecContext& ctxt, integer_t angle)
{
	MCGraphicsRotateImage(MCactiveimage, angle);
}

void MCGraphicsExecRotateImage(MCExecContext& ctxt, MCImage *image, integer_t angle)
{
	if (!image->iseditable())
	{
		ctxt . LegacyThrow(EE_ROTATE_NOTIMAGE);
		return;
	}

	MCGraphicsRotateImage(image, angle);
}

////////////////////////////////////////////////////////////////////////////////

void MCGraphicsExecCropImage(MCExecContext& ctxt, MCImage *p_image, MCRectangle p_bounds)
{
	if (p_image != nil)
	{
		if (!p_image -> iseditable())
		{
			ctxt . LegacyThrow(EE_CROP_NOTIMAGE);
			return;
		}
		p_image->crop(&p_bounds);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCGraphicsExecResetPaint(MCExecContext& ctxt) 
{
    MCeditingimage = nil;
    
		// MDW-2016-05-06 [[ bugfix_17553 ]] set brush defaults using validators
    MCInterfaceSetBrush(ctxt, 8);
    MCInterfaceSetSpray(ctxt, 34);
    MCInterfaceSetEraser(ctxt, 2);
    MCcentered = False;
    MCfilled = False;
    MCgrid = False;
    MCgridsize = 8;
    MClinesize = 1;
    MCmultiple = False;
    MCmultispace = 1;
    MCpolysides = 4;
    MCroundends = False;
    MCslices = 16;
    MCmagnification = 8;
    MCpatternlist->freepat(MCpenpattern);
    MCpencolor.red = MCpencolor.green = MCpencolor.blue = 0x0;
	
    MCpatternlist->freepat(MCbrushpattern);
    MCbrushcolor.red = MCbrushcolor.green = MCbrushcolor.blue = 0xFFFF;
}

////////////////////////////////////////////////////////////////////////////////

void MCGraphicsExecPrepareImage(MCExecContext& ctxt, MCImage *image)
{
	image -> prepareimage();
}

void MCGraphicsExecPrepareImageFile(MCExecContext& ctxt, MCStringRef p_filename)
{
	MCAutoStringRef t_resolved;
//	MCAutoStringRef t_fixed;
	/* UNCHECKED */ MCS_resolvepath(p_filename, &t_resolved);
	//		MCU_fix_path(t_filename);
	
	MCImageRep *t_rep;
	MCImageRepGetReferenced(*t_resolved, t_rep);
	
	if (t_rep != nil)
	{
		uindex_t t_width, t_height;
		t_rep -> GetGeometry(t_width, t_height);
		
		t_rep -> Release();
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCGraphicsGetImageCacheLimit(MCExecContext &ctxt, uinteger_t &r_value)
{
	r_value = MCCachedImageRep::GetCacheLimit();
}

void MCGraphicsSetImageCacheLimit(MCExecContext &ctxt, uinteger_t p_value)
{
	MCCachedImageRep::SetCacheLimit(p_value);
}

void MCGraphicsGetImageCacheUsage(MCExecContext &ctxt, uinteger_t &r_value)
{
	r_value = MCCachedImageRep::GetCacheUsage();
}

////////////////////////////////////////////////////////////////////////////////
