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

#include "exec.h"

#include "mblsyntax.h"

////////////////////////////////////////////////////////////////////////////////

bool MCSystemCanAcquirePhoto(MCPhotoSourceType p_source);
bool MCSystemAcquirePhoto(MCPhotoSourceType p_source, int32_t p_max_width, int32_t p_max_height, void*& r_image_data, size_t& r_image_data_size);

MCCameraFeaturesType MCSystemGetCameraFeatures(MCCameraSourceType p_source);

////////////////////////////////////////////////////////////////////////////////

void MCCameraExecAcquirePhotoAndResize(MCExecContext& ctxt, MCPhotoSourceType p_source, int32_t p_max_width, int32_t p_max_height)
{
	if (!MCSystemCanAcquirePhoto(p_source))
	{
		ctxt . SetTheResultToStaticCString("source not available");
		return;
	}
    
#ifdef MOBILE_BROKEN
	MCAutoRawMemoryBlock t_image_data;
	size_t t_image_data_size;
	if (!MCSystemAcquirePhoto(p_source, p_max_width, p_max_height, t_image_data, t_image_data_size))
	{
		ctxt . SetTheResultToStaticCString("error");
		return;
	}
	
	if (t_image_data . Borrow() == nil)
	{
		ctxt . SetTheResultToStaticCString("cancel");
		return;
	}
	
	MCtemplateimage->setparent((MCObject *)MCdefaultstackptr -> getcurcard());
	MCImage *iptr = (MCImage *)MCtemplateimage->clone(False, OP_NONE, false);
	MCtemplateimage->setparent(NULL);
	iptr -> attach(OP_CENTER, false);
	
	MCExecPoint ep(nil, nil, nil);
	ep . setsvalue(MCString((char *)t_image_data . Borrow(), t_image_data_size));
	iptr -> setprop(0, P_TEXT, ep, false);
#endif
}

void MCCameraExecAcquirePhoto(MCExecContext& ctxt, MCPhotoSourceType p_photo)
{
	MCCameraExecAcquirePhotoAndResize(ctxt, p_photo, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////

void MCCameraGetFeatures(MCExecContext& ctxt, MCCameraSourceType p_camera, MCCameraFeaturesType& r_features)
{
	r_features = MCSystemGetCameraFeatures(p_camera);
}

////////////////////////////////////////////////////////////////////////////////
/* moved to mblhandlers.cpp
Exec_stat MCHandleSpecificCameraFeatures(void *p_context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	ep . clear();
	
	MCCameraSourceType t_source;
	p_parameters -> eval_argument(ep);
	if (MCU_strcasecmp(ep . getcstring(), "front"))
		t_source = kMCCameraSourceTypeFront;
	else if (MCU_strcasecmp(ep . getcstring(), "rear"))
		t_source = kMCCameraSourceTypeRear;
	else
		return ES_NORMAL;
	
	////////
	
	MCExecContext t_ctxt(ep);
	
	MCCameraFeaturesType t_features_set;
	MCCameraGetFeatures(t_ctxt, t_source, t_features_set);
	
	////////
	
	if ((t_features_set & kMCCameraFeaturePhoto) != 0)
		ep . concatcstring("photo", EC_COMMA, ep . isempty());
	if ((t_features_set & kMCCameraFeatureVideo) != 0)
		ep . concatcstring("video", EC_COMMA, ep . isempty());
	if ((t_features_set & kMCCameraFeatureFlash) != 0)
		ep . concatcstring("flash", EC_COMMA, ep . isempty());
#ifdef MOBILE_BROKEN
	MCresult -> store(ep, False);
#endif
	
	return ES_NORMAL;
}

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
		t_width_param -> eval_argument(ep);
		t_width = ep . getint4();
	}
	if (t_height_param != nil)
	{
		t_height_param -> eval_argument(ep);
		t_height = ep . getint4();
	}

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
} */

////////////////////////////////////////////////////////////////////////////////