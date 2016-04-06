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
#include "stack.h"
#include "image.h"
#include "param.h"

#include "exec.h"

#include "mblsyntax.h"

////////////////////////////////////////////////////////////////////////////////
/*
bool MCSystemCanAcquirePhoto(MCPhotoSourceType p_source);
bool MCSystemAcquirePhoto(MCPhotoSourceType p_source, int32_t p_max_width, int32_t p_max_height, void*& r_image_data, size_t& r_image_data_size);
*/

////////////////////////////////////////////////////////////////////////////////

#ifdef /* MCHandlePickPhotoIphone */ LEGACY_EXEC
Exec_stat MCHandlePickPhoto(void *p_context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	ep . clear();
	
	MCParameter *t_source_param, *t_width_param, *t_height_param;
	t_source_param = p_parameters;
	t_width_param = t_source_param != nil ? t_source_param -> getnext() : nil;
	t_height_param = t_width_param != nil ? t_width_param -> getnext() : nil;
	
	int32_t t_width, t_height;
	t_width = t_height = 0;
	if (t_width_param != nil)
	{
		// MW-2013-07-01: [[ Bug 10989 ]] Make sure we force conversion to a number.
		if (t_width_param -> eval_argument(ep) == ES_NORMAL &&
			ep . ton() == ES_NORMAL)
			t_width = ep . getint4();
	}
	if (t_height_param != nil)
	{
		// MW-2013-07-01: [[ Bug 10989 ]] Make sure we force conversion to a number.
		if (t_height_param -> eval_argument(ep) == ES_NORMAL &&
			ep . ton() == ES_NORMAL)
			t_height = ep . getint4();
	}

	MCLog("%d, %d", t_width, t_height);
	
	const char *t_source;
	t_source = nil;
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		t_source = ep . getcstring();
	}
	
	MCPhotoSourceType t_photo_source;
	bool t_is_take;
	t_is_take = false;
	
	if (MCU_strcasecmp(t_source, "library") == 0)
		t_photo_source = kMCPhotoSourceTypeLibrary;
	else if (MCU_strcasecmp(t_source, "album") == 0)
		t_photo_source = kMCPhotoSourceTypeAlbum;
	else if (MCU_strcasecmp(t_source, "camera") == 0 || MCU_strcasecmp(t_source, "rear camera") == 0)
		t_photo_source = kMCPhotoSourceTypeRearCamera;
	else if (MCU_strcasecmp(t_source, "front camera") == 0)
		t_photo_source = kMCPhotoSourceTypeFrontCamera;
	else
	{
		MCresult -> sets("unknown source");
		return ES_NORMAL;
	}
	
	/////
	
	MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
	
	if (t_width != 0 && t_height != 0)
		MCCameraExecAcquirePhotoAndResize(t_ctxt, t_photo_source, t_width, t_height);
	else
		MCCameraExecAcquirePhoto(t_ctxt, t_photo_source);

	return t_ctxt . GetExecStat();
}
#endif /* MCHandlePickPhotoIphone */

////////////////////////////////////////////////////////////////////////////////
