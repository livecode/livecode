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

#include "sysdefs.h"
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
#include "mblsensor.h"
#include "mblcontrol.h"


////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

static const char *s_orientation_names[] =
{
	"unknown", "portrait", "portrait upside down", "landscape right", "landscape left", "face up", "face down", nil
};

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleCanComposeTextMessage(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_result;
    MCTextMessagingGetCanComposeTextMessage(ctxt, t_result);
    
    if (t_result)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
    if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleComposeTextMessage(void *p_context, MCParameter *p_parameters)
{
    MCAutoStringRef t_recipients, t_body;
    
    bool t_success;
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	t_success = MCParseParameters(p_parameters, "x", &(&t_recipients));
    if (t_success == false)
    {
         ctxt . SetTheResultToValue(kMCFalse);
        return ES_NORMAL;
    }
	t_success = MCParseParameters(p_parameters, "x", &(&t_body));
    
    ep . clear();
  
    
    if (t_success)
        MCTextMessagingExecComposeTextMessage(ctxt, *t_recipients, *t_body);
    
	return ES_NORMAL;
}


///////////////////////////////////////////////////////////////////////////


Exec_stat MCHandleLockIdleTimer(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCIdleTimerExecLockIdleTimer(ctxt);
    
    return ES_NORMAL;
}


Exec_stat MCHandleUnlockIdleTimer(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCIdleTimerExecUnlockIdleTimer(ctxt);
    
    return ES_NORMAL;    
}

Exec_stat MCHandleIdleTimerLocked(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_result;
    
    MCIdleTimerGetIdleTimerLocked(ctxt, t_result);
    
    if (t_result)
        ctxt.SetTheResultToValue(kMCTrue);    
    else
        ctxt.SetTheResultToValue(kMCFalse);
    
    if (ctxt.HasError())
        return ES_ERROR;        
    else
        return ES_NORMAL;
}


///////////////////////////////////////////////////////////////////////////


Exec_stat MCHandleCanMakePurchase(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_result;
    
    MCStoreGetCanMakePurchase(ctxt, t_result);
    
    if (t_result)
        ctxt.SetTheResultToValue(kMCTrue);
    else
        ctxt.SetTheResultToValue(kMCFalse);
    
    if (ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;    
}

Exec_stat MCHandleEnablePurchaseUpdates(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCStoreExecEnablePurchaseUpdates(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleDisablePurchaseUpdates(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCStoreExecDisablePurchaseUpdates(ctxt);
    
    if(!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}
    
Exec_stat MCHandleRestorePurchases(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCStoreExecRestorePurchases(ctxt);
    
    if(!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}


Exec_stat MCHandlePurchaseList(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCAutoStringRef t_list;
    
    MCStoreGetPurchaseList(ctxt, &t_list);
    
    if(!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_list);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandlePurchaseCreate(void* p_context, MCParameter* p_parameters)
{
    bool t_success = true;
    MCAutoStringRef t_product_id;
    uint32_t t_id;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_product_id));
    
    if (t_success)
        MCStoreExecCreatePurchase(ctxt, &t_product_id, t_id);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToNumber(t_id);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}


Exec_stat MCHandlePurchaseState(void* p_context, MCParameter* p_parameters)
{    
	bool t_success = true;
	
	uint32_t t_id;
    MCAutoStringRef t_state;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
    
	if (t_success)
		MCStoreGetPurchaseState(ctxt, t_id, &t_state);
	
	if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_state);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}


Exec_stat MCHandlePurchaseError(void* p_context, MCParameter* p_parameters)
{
	bool t_success = true;
	
    MCStringRef t_error;
	uint32_t t_id;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
	
	if (t_success)
        MCStoreGetPurchaseError(ctxt, t_id, t_error);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(t_error);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandlePurchaseGet(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	MCAutoStringRef t_prop_name;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "ux", &t_id, &(&t_prop_name));
	
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	
	if (t_success)
        MCStoreGetPurchaseProperty(ctxt, t_id, &t_prop_name);
	
	if (!ctxt . HasError())
    {
        MCAutoStringRef t_string;
        /* UNCHECKED */ ep . copyasstringref(&t_string);
		ctxt . SetTheResultToValue(*t_string);
        return ES_NORMAL;
    }
    
    ctxt . SetTheResultToEmpty();
	return ES_ERROR;
}


Exec_stat MCHandlePurchaseSet(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	MCAutoStringRef t_prop_name;
    uint32_t t_quantity;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "uxu", &t_id, &(&t_prop_name), &t_quantity);
		
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	if (t_success)
        MCStoreSetPurchaseProperty(ctxt, t_id, &t_prop_name, t_quantity);
	
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}



Exec_stat MCHandlePurchaseSendRequest(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    if (t_success)
        MCStoreExecSendPurchaseRequest(ctxt, t_id);
    
	if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandlePurchaseConfirmDelivery(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
	
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	if (t_success)
        MCStoreExecConfirmPurchaseDelivery(ctxt, t_id);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}


Exec_stat MCHandleRequestProductDetails(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef t_product;
    bool t_success = true;    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_product));
        
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    if (t_success)
        MCStoreExecRequestProductDetails(ctxt, *t_product);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandlePurchaseVerify(void *context, MCParameter *p_parameters)
{
    bool t_success = true;
    bool t_verified = true;
    
    uint32_t t_id;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "ub", &t_id, &t_verified);
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    if (t_success)
        MCStoreExecPurchaseVerify(ctxt, t_id, t_verified);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleDeviceOrientation(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	intenum_t t_orientation;
	MCOrientationGetDeviceOrientation(ctxt, t_orientation);

	ctxt . SetTheResultToStaticCString(s_orientation_names[(int)t_orientation]);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleOrientation(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	intenum_t t_orientation;
	MCOrientationGetOrientation(ctxt, t_orientation);

	ctxt . SetTheResultToStaticCString(s_orientation_names[(int)t_orientation]);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleAllowedOrientations(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	intset_t t_orientations;
	MCOrientationGetAllowedOrientations(ctxt, t_orientations);

	bool t_success;
	t_success = true;
	
	MCAutoListRef t_orientation_list;

	if (t_success)
		t_success = MCListCreateMutable(EC_COMMA, &t_orientation_list);

	for (uint32_t j = 0; s_orientation_names[j] != nil; j++)
	{
		if ((t_orientations & (1 << j)) != 0)
		{		
			MCAutoStringRef t_orientation;
			if (t_success)
				t_success = MCStringFormat(&t_orientation, "%s", s_orientation_names[j]);

			if (t_success)
				t_success = MCListAppend(*t_orientation_list, *t_orientation);
		}
	}

	MCAutoStringRef t_result;
	if (t_success)
		t_success = MCListCopyAsString(*t_orientation_list, &t_result);

	if (t_success)
	{
		ctxt . SetTheResultToValue(*t_result);
		return ES_NORMAL;
	}

	return ES_ERROR;
}

Exec_stat MCHandleSetAllowedOrientations(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	MCAutoStringRef t_orientations;
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		ep . copyasstringref(&t_orientations);
	}

    bool t_success = true;
	char **t_orientations_array;
	uint32_t t_orientations_count;
	t_orientations_array = nil;
	t_orientations_count = 0;
	if (t_success)
		t_success = MCCStringSplit(MCStringGetCString(*t_orientations), ',', t_orientations_array, t_orientations_count);
	
	intset_t t_orientations_set;
	t_orientations_set = 0;
	if (t_success)
    {
		for(uint32_t i = 0; i < t_orientations_count; i++)
        {
            if (MCCStringEqualCaseless(t_orientations_array[i], "portrait"))
                t_orientations_set |= ORIENTATION_PORTRAIT_BIT;
            else if (MCCStringEqualCaseless(t_orientations_array[i], "portrait upside down"))
                t_orientations_set |= ORIENTATION_PORTRAIT_UPSIDE_DOWN_BIT;
            else if (MCCStringEqualCaseless(t_orientations_array[i], "landscape right"))
                t_orientations_set |= ORIENTATION_LANDSCAPE_RIGHT_BIT;
            else if (MCCStringEqualCaseless(t_orientations_array[i], "landscape left"))
                t_orientations_set |= ORIENTATION_LANDSCAPE_LEFT_BIT;
            else if (MCCStringEqualCaseless(t_orientations_array[i], "face up"))
                t_orientations_set |= ORIENTATION_FACE_UP_BIT;
            else if (MCCStringEqualCaseless(t_orientations_array[i], "face down"))
                t_orientations_set |= ORIENTATION_FACE_DOWN_BIT;
        }
	}
    
	for(uint32_t i = 0; i < t_orientations_count; i++)
		MCCStringFree(t_orientations_array[i]);
	MCMemoryDeleteArray(t_orientations_array);

	MCOrientationSetAllowedOrientations(ctxt, t_orientations_set);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleOrientationLocked(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	bool t_locked;
	MCOrientationGetOrientationLocked(ctxt, t_locked);

    if (t_locked)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleLockOrientation(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	MCOrientationExecLockOrientation(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleUnlockOrientation(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	MCOrientationExecUnlockOrientation(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleRevMail(void *context, MCParameter *p_parameters)
{
	MCAutoStringRef t_address, t_cc_address, t_subject, t_message_body;

	MCExecPoint ep(nil, nil, nil);
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		/* UNCHECKED */ ep . copyasstringref(&t_address);
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		/* UNCHECKED */ ep . copyasstringref(&t_cc_address);
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		/* UNCHECKED */ ep . copyasstringref(&t_subject);
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		/* UNCHECKED */ ep . copyasstringref(&t_message_body);
		p_parameters = p_parameters -> getnext();
	}
	
	MCExecContext ctxt(ep);

	MCMailExecSendEmail(ctxt, *t_address, *t_cc_address, *t_subject, *t_message_body);
	
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleComposeMail(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_to, t_cc, t_bcc, t_subject, t_body;
	MCAutoArrayRef t_attachments;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &(&t_subject), &(&t_to), &(&t_cc), &(&t_bcc), &(&t_body), &(&t_attachments));

	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	if (t_success)
		MCMailExecComposeMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleComposePlainMail(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_to, t_cc, t_bcc, t_subject, t_body;
	MCAutoArrayRef t_attachments;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &(&t_subject), &(&t_to), &(&t_cc), &(&t_bcc), &(&t_body), &(&t_attachments));

	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	if (t_success)
		MCMailExecComposeMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleComposeUnicodeMail(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_to, t_cc, t_bcc, t_subject, t_body;
	MCAutoArrayRef t_attachments;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &(&t_subject), &(&t_to), &(&t_cc), &(&t_bcc), &(&t_body), &(&t_attachments));

	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	if (t_success)
		MCMailExecComposeUnicodeMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleComposeHtmlMail(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_to, t_cc, t_bcc, t_subject, t_body;
	MCAutoArrayRef t_attachments;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &(&t_subject), &(&t_to), &(&t_cc), &(&t_bcc), &(&t_body), &(&t_attachments));

	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	if (t_success)
        MCMailExecComposeHtmlMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCanSendMail(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	bool t_can_send;

	MCMailGetCanSendMail(ctxt, t_can_send);
    
    if (t_can_send)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleStartTrackingSensor(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    bool t_loosely = false;
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_loosely = ep . getsvalue() == MCtruemcstring;
    }
    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    if (t_sensor != kMCSensorTypeUnknown)
    {
        MCSensorExecStartTrackingSensor(ctxt, (intenum_t)t_sensor, t_loosely);
    }
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleStopTrackingSensor(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }
    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();

    if (t_sensor != kMCSensorTypeUnknown)
    {
        MCSensorExecStopTrackingSensor(ctxt, (intenum_t)t_sensor);
    }
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

// MM-2012-02-11: Added support old style sensor syntax (iPhoneEnableAcceleromter etc)
Exec_stat MCHandleAccelerometerEnablement(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(ctxt, kMCSensorTypeAcceleration, false);
    else
        MCSensorExecStopTrackingSensor(ctxt, (intenum_t)kMCSensorTypeAcceleration);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleLocationTrackingState(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(ctxt, kMCSensorTypeLocation, false);
    else
        MCSensorExecStopTrackingSensor(ctxt, (intenum_t)kMCSensorTypeLocation);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleHeadingTrackingState(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(ctxt, kMCSensorTypeHeading, true);
    else
        MCSensorExecStopTrackingSensor(ctxt, (intenum_t)kMCSensorTypeHeading);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleSensorReading(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    bool t_detailed = false;
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_detailed = ep . getsvalue() == MCtruemcstring;
    }
    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_detailed_reading;
    MCAutoStringRef t_reading;

    switch (t_sensor)
    {
        case kMCSensorTypeLocation:
        {
            if (t_detailed)
                MCSensorGetDetailedLocationOfDevice(ctxt, &t_detailed_reading);
            else
                MCSensorGetLocationOfDevice(ctxt, &t_reading);
            break;
        }
        case kMCSensorTypeHeading:
        {
            if (t_detailed)
                MCSensorGetDetailedHeadingOfDevice(ctxt,& t_detailed_reading);
            else
                MCSensorGetHeadingOfDevice(ctxt, &t_reading);
            break;
        }
        case kMCSensorTypeAcceleration:
        {
            if (t_detailed)
                MCSensorGetDetailedAccelerationOfDevice(ctxt, &t_detailed_reading);
            else
                MCSensorGetAccelerationOfDevice(ctxt, &t_reading);
            break;
        }
        case kMCSensorTypeRotationRate:
        {
            if (t_detailed)
                MCSensorGetDetailedRotationRateOfDevice(ctxt, &t_detailed_reading);
            else
                MCSensorGetRotationRateOfDevice(ctxt, &t_reading);
            break;
        }
        default:
            break;
    }
    
    if (t_detailed)
    {
        if (*t_detailed_reading != nil)
            ep.setvalueref(*t_detailed_reading);
    }
    else
    {
        if (*t_reading != nil)
            ep.setvalueref(*t_reading);
    }
    
	MCAutoStringRef t_result;
	ep . copyasstringref(&t_result);
    ctxt . SetTheResultToValue(*t_result);
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

// MM-2012-02-11: Added support old style sensor syntax (iPhoneGetCurrentLocation etc)
Exec_stat MCHandleCurrentLocation(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_detailed_reading;
    MCSensorGetDetailedLocationOfDevice(ctxt, &t_detailed_reading);
    if (*t_detailed_reading != nil)
        ep.setvalueref(*t_detailed_reading);
    
	MCAutoStringRef t_result;
	ep . copyasstringref(&t_result);
    ctxt . SetTheResultToValue(*t_result);
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCurrentHeading(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_detailed_reading;
    MCSensorGetDetailedHeadingOfDevice(ctxt, &t_detailed_reading);
    if (*t_detailed_reading != nil)
        ep.setvalueref(*t_detailed_reading);
    
	MCAutoStringRef t_result;
	ep . copyasstringref(&t_result);
    ctxt . SetTheResultToValue(*t_result);
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleSetHeadingCalibrationTimeout(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    int t_timeout;
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_timeout = atoi(ep.getcstring());
    }
    MCSensorSetLocationCalibrationTimeout(ctxt, t_timeout);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleHeadingCalibrationTimeout(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    int t_timeout;
    MCSensorGetLocationCalibrationTimeout(ctxt, t_timeout);
    MCresult->setnvalue(t_timeout);
    
    ctxt . SetTheResultToEmpty();
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleSensorAvailable(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();

    MCSensorType t_sensor;
    t_sensor = kMCSensorTypeUnknown;    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }    
    
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(ctxt, t_sensor, t_available);
    
    if (t_available)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCanTrackLocation(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
        
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(ctxt, kMCSensorTypeLocation, t_available);
    
    if (t_available)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCanTrackHeading(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(ctxt, kMCSensorTypeHeading, t_available);
    
    if (t_available)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

bool MCContactParseParams(MCParameter *p_params, MCArrayRef &r_contact, char *&r_title, char *&r_message, char *&r_alternate_name)
{
	bool t_success = true;
	
	char *t_title = nil;
	char *t_message = nil;
	char *t_alternate_name = nil;
	
	t_success = MCParseParameters(p_params, "a|sss", r_contact, &t_title, &t_message, &t_alternate_name);
	
	if (t_success)
	{
		r_title = t_title;
		r_message = t_message;
		r_alternate_name = t_alternate_name;
	}
	else
	{
		MCCStringFree(t_title);
		MCCStringFree(t_message);
		MCCStringFree(t_alternate_name);
	}
	
	return t_success;
}

Exec_stat MCHandlePickContact(void *context, MCParameter *p_parameters) // ABPeoplePickerNavigationController
{
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);

    MCAddressBookExecPickContact(ctxt);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleShowContact(void *context, MCParameter *p_parameters) // ABPersonViewController
{
    int32_t t_contact_id = 0;
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);

    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_id = atoi (ep.getsvalue().getstring());
    }

    MCExecContext ctxt(ep);

    MCAddressBookExecShowContact(ctxt, t_contact_id);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCreateContact(void *context, MCParameter *p_parameters) // ABNewPersonViewController
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);

    MCAddressBookExecCreateContact(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleUpdateContact(void *context, MCParameter *p_parameters) // ABUnknownPersonViewController
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);

	MCAutoArrayRef t_contact;
	MCAutoStringRef t_title;
	MCAutoStringRef t_message;
	MCAutoStringRef t_alternate_name;

	if (MCParseParameters(p_parameters, "axxx", &(&t_contact), &(&t_title), &(&t_message), &(&t_alternate_name)))
	    MCAddressBookExecUpdateContact(ctxt, *t_contact, *t_title, *t_message, *t_alternate_name);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleGetContactData(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);

    int32_t t_contact_id = 0;
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_id = atoi (ep.getsvalue().getstring());
    }

    MCExecContext ctxt(ep);

	MCAutoArrayRef t_contact_data;
	MCAddressBookGetContactData(ctxt, t_contact_id, &t_contact_data);

	if (*t_contact_data != nil)
		ctxt . SetTheResultToValue(*t_contact_data);
	else
		ctxt . SetTheResultToEmpty();

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleRemoveContact(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);

    int32_t t_contact_id = 0;
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_id = atoi (ep.getsvalue().getstring());
    }

    MCExecContext ctxt(ep);
    
	MCAddressBookExecRemoveContact(ctxt, t_contact_id);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleAddContact(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    // Handle parameters. We are doing that in a dedicated call
	MCAutoArrayRef t_contact;
	
	/* UNCHECKED */ MCParseParameters(p_parameters, "a", &(&t_contact));

    MCExecContext ctxt(ep);
    // Call the Exec implementation
    MCAddressBookExecAddContact(ctxt, *t_contact);
    // Set return value
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleFindContact(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef t_contact_name;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        /* UNCHECKED */ ep . copyasstringref(&t_contact_name);
    }
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCAddressBookExecFindContact(ctxt, *t_contact_name);
    // Set return value
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}


//////////////////////////////////////////////////////////////////////////////


Exec_stat MCHandleAdRegister(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_key;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_key));
	
	if (t_success)
		MCAdExecRegisterWithInneractive(ctxt, *t_key);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
    return ES_ERROR;
}


Exec_stat MCHandleAdCreate(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;
    MCAutoStringRef t_type;
    
	if (t_success)
		t_success = MCParseParameters(p_parameters, "xx", &(&t_ad), &(&t_type));
    
   MCAdTopLeft t_topleft;

    if (t_success)
        t_success = MCParseParameters(p_parameters, "uu", &t_topleft.x, &t_topleft.y);
    
    MCAutoArrayRef t_metadata;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "a", &(&t_metadata));
    
	if (t_success)
		MCAdExecCreateAd(ctxt, *t_ad, *t_type, t_topleft, *t_metadata);
    
    
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleAdDelete(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_ad));
	
	if (t_success)
		MCAdExecDeleteAd(ctxt, *t_ad);
        
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleAdGetVisible(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	MCAutoStringRef t_ad;
    
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_ad));
	
    bool t_visible;
    t_visible = false;
    
	if (t_success)
		MCAdGetVisibleOfAd(ctxt, *t_ad, t_visible);
    
    
    if (!ctxt . HasError())
    {
        if (t_visible)
            ctxt.SetTheResultToValue(kMCTrueString);
        else
            ctxt.SetTheResultToValue(kMCFalseString);
        
		return ES_NORMAL;
    }

	ctxt . SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleAdSetVisible(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;
    
    bool t_visible;
    t_visible = false;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "xb", &(&t_ad), &t_visible);
	
	if (t_success)
		MCAdSetVisibleOfAd(ctxt, *t_ad, t_visible);
    
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleAdGetTopLeft(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);

	MCAutoStringRef t_ad;
    
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_ad));
	
    MCAdTopLeft t_topleft;
    
	if (t_success)
		MCAdGetTopLeftOfAd(ctxt, *t_ad, t_topleft);
    
    if (!ctxt . HasError())
    {
        MCAutoStringRef t_topleft_string;
        if(MCStringFormat(&t_topleft_string, "%u,%u", t_topleft.x, t_topleft.y))
        {
            ctxt.SetTheResultToValue(*t_topleft_string);
            return ES_NORMAL;
        }
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleAdSetTopLeft(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;
    MCAdTopLeft t_topleft;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "xuu", &(&t_ad), t_topleft.x, t_topleft.y);
    
	if (t_success)
		MCAdSetTopLeftOfAd(ctxt, *t_ad, t_topleft);
    
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleAds(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();

    MCAutoStringRef t_ads;
    MCAdGetAds(ctxt, &t_ads);

    if (!ctxt . HasError())
    {
        ctxt . SetTheResultToValue(*t_ads);
		return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////


Exec_stat MCHandleShowEvent(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef t_id;
    bool t_success;
    t_success = true;
    
    // Handle parameters.
    
    if (t_success)
    {
        t_success = MCParseParameters(p_parameters, "x", &(&t_id));
    }
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    // Call the Exec implementation
    if (t_success)
        MCCalendarExecShowEvent(ctxt, *t_id);
    
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleUpdateEvent(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef t_id;
    bool t_success;
    t_success = true;

    // Handle parameters.    
    if (t_success)
    {
        t_success = MCParseParameters(p_parameters, "x", &(&t_id));
    }
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    // Call the Exec implementation
    if (t_success)
        MCCalendarExecUpdateEvent(ctxt, *t_id);
    
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleCreateEvent(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef t_id;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);

    // Call the Exec implementation
    MCCalendarExecCreateEvent(ctxt);
    // Set return value
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleGetEventData(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef t_id;
    bool t_success;
    t_success = true;
    
    // Handle parameters.
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_id));
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    // Call the Exec implementation
    MCAutoArrayRef t_data;
    if (t_success)
    {
        MCCalendarGetEventData(ctxt, *t_id, &t_data);
    }    
    
    if (!ctxt . HasError())
    {
        ctxt.SetTheResultToValue(*t_data);
		return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleRemoveEvent(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef t_id;
    bool t_reocurring = false;
    bool t_success = true;
    // Handle parameters.
    t_success = MCParseParameters(p_parameters, "x", &(&t_id));
    
    if (t_success)
    {
        t_success = MCParseParameters(p_parameters, "b", &t_reocurring);
    }
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
//    MCCalendarExecRemoveEvent(ctxt, t_reocurring, &t_id);
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleAddEvent(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_success;
    
    // Handle parameters. We are doing that in a dedicated call
    MCAutoArrayRef t_array;
    
    t_success = MCParseParameters(p_parameters, "a", &(&t_array));
    
    if (t_success)
        MCCalendarExecAddEvent(ctxt, *t_array);
    
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleGetCalendarsEvent(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    
    // Call the Exec implementation
    MCCalendarGetCalendars(ctxt);
    
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleFindEvent(void *context, MCParameter *p_parameters)
{
    MCDateTime t_start_date;
    MCDateTime t_end_date;
    bool t_success = true;
    const char *r_result = NULL;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_start_date);
        }
        p_parameters = p_parameters->getnext();
    }
    if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_end_date);
        }
    }
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
//    MCCalendarExecFindEvent(ctxt, t_start_date, t_end_date);
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleCreateLocalNotification (void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
    bool t_success = true;
    MCAutoStringRef t_notification_body;
    MCAutoStringRef t_notification_action;
    MCAutoStringRef t_notification_user_info;
    MCDateTime t_date;
    bool t_play_sound_vibrate = true;
    int32_t t_badge_value = 0;
    
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters (p_parameters, "xxx", &(&t_notification_body), &(&t_notification_action), &(&t_notification_user_info));
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_date);
        }
        p_parameters = p_parameters->getnext();
    }
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_play_sound_vibrate);
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "u", &t_badge_value);
    
	MCNotificationExecCreateLocalNotification (ctxt, *t_notification_body, *t_notification_action, *t_notification_user_info, t_date, t_play_sound_vibrate, t_badge_value);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleGetRegisteredNotifications(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    
    MCNotificationGetRegisteredNotifications(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleGetNotificationDetails(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_success = true;
    
    int32_t t_id = -1;
    MCAutoArrayRef t_details;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "i", &t_id);
    
    if (t_success)
    {
        MCNotificationGetDetails(ctxt, t_id, &t_details);
        if (!ctxt.HasError() && *t_details != nil)
        {
			ctxt . SetTheResultToValue(*t_details);
            return ES_NORMAL;
        }
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleCancelLocalNotification(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    int32_t t_cancel_this;
    bool t_success;
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    if (p_parameters != nil)
		t_success = MCParseParameters (p_parameters, "i", &t_cancel_this);
    
    if (t_success)
    {
        MCNotificationExecCancelLocalNotification (ctxt, t_cancel_this);
    }
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleCancelAllLocalNotifications (void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    
    MCNotificationExecCancelAllLocalNotifications(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleGetNotificationBadgeValue (void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    MCNotificationGetNotificationBadgeValue (ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleSetNotificationBadgeValue (void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    uint32_t t_badge_value;
    bool t_success = true;
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters (p_parameters, "i", &t_badge_value);
    
    if (t_success)
        MCNotificationSetNotificationBadgeValue (ctxt, t_badge_value);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}


////////////////

static MCBusyIndicatorType MCBusyIndicatorTypeFromCString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "in line", kMCCompareCaseless))
        return kMCBusyIndicatorInLine;
    else if (MCStringIsEqualToCString(p_string, "square", kMCCompareCaseless))
        return kMCBusyIndicatorSquare;
    else if (MCStringIsEqualToCString(p_string, "keyboard", kMCCompareCaseless))
        return kMCBusyIndicatorKeyboard;
    
    return kMCBusyIndicatorSquare;
}

static bool MCBusyIndicatorTypeToCString(MCSensorType p_indicator, MCStringRef& r_string)
{
    switch (p_indicator)
    {
        case kMCBusyIndicatorInLine:
            return MCStringCreateWithCString("in line", r_string);
        case kMCBusyIndicatorSquare:
            return MCStringCreateWithCString("square", r_string);
        case kMCBusyIndicatorKeyboard:
            return MCStringCreateWithCString("keyboard", r_string);
        default:
            return MCStringCreateWithCString("unknown", r_string);
    }
    return false;
}


// MM-2013-02-04: [[ Bug 10642 ]] Added new optional opacity parameter to busy indicator.
Exec_stat MCHandleStartBusyIndicator(void *p_context, MCParameter *p_parameters)
{
    bool t_success = true;
    MCAutoStringRef t_indicator_string;
    MCAutoStringRef t_label;    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "xx", &(&t_indicator_string), &(&t_label));
    
    intenum_t t_indicator;
    if(t_success)
        t_success = MCBusyIndicatorTypeFromCString(*t_indicator_string);        
    
    int32_t t_opacity = -1;
    if(t_success)
    {
        t_success = MCParseParameters(p_parameters, "i", &t_opacity);
        if (t_opacity < 0 || t_opacity > 100)
            t_opacity = -1;
    }
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    MCBusyIndicatorExecStartBusyIndicator(ctxt, kMCBusyIndicatorSquare, *t_label, t_opacity);
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;    
}

Exec_stat MCHandleStopBusyIndicator(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	MCBusyIndicatorExecStopBusyIndicator(ctxt);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

///////////////

static MCActivityIndicatorType MCActivityIndicatorTypeFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "white", kMCCompareCaseless))
        return kMCActivityIndicatorWhite;
    else if (MCStringIsEqualToCString(p_string, "large white", kMCCompareCaseless))
        return kMCActivityIndicatorWhiteLarge;
    else if (MCStringIsEqualToCString(p_string, "gray", kMCCompareCaseless))
        return kMCActivityIndicatorGray;
    
    return kMCActivityIndicatorWhite;
}

static bool MCActivityIndicatorTypeToString(MCSensorType p_indicator, MCStringRef& r_string)
{
    switch (p_indicator)
    {
        case kMCActivityIndicatorWhite:
            return MCStringCreateWithCString("white", r_string);
        case kMCActivityIndicatorWhiteLarge:
            return MCStringCreateWithCString("large white", r_string);
        case kMCActivityIndicatorGray:
            return MCStringCreateWithCString("gray", r_string);
        default:
            return MCStringCreateWithCString("unknown", r_string);
    }
    return false;
}

Exec_stat MCHandleStartActivityIndicator(void *p_context, MCParameter *p_parameters)
{
    MCAutoStringRef t_style_string;    
    MCActivityIndicatorType t_style;
    t_style = kMCActivityIndicatorWhite;
    
    bool t_success = true;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_style_string));
    
    if (t_success)
    {
        if (MCStringIsEqualToCString(*t_style_string, "whitelarge", kMCCompareCaseless))
            t_success = MCStringCreateWithCString("large white", &t_style_string);
    }
    
    if (t_success)
        t_style = MCActivityIndicatorTypeFromString(*t_style_string);
        
    
    bool t_location_param = false;
    integer_t* t_location_x_ptr = nil;
    integer_t* t_location_y_ptr = nil;
    integer_t t_location_x;
    integer_t t_location_y;
    
    if (MCParseParameters(p_parameters, "i", &t_location_x))
    {
        t_location_param = true;
        t_location_x_ptr = &t_location_x;
    }

    if (MCParseParameters(p_parameters, "i", &t_location_y))
    {
        t_location_param = true;
        t_location_y_ptr = &t_location_y;
    }
        
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    if (t_success)
        MCBusyIndicatorExecStartActivityIndicator(ctxt, t_style, t_location_x_ptr, t_location_y_ptr);
    
	if (t_success && !ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleStopActivityIndicator(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    MCBusyIndicatorExecStopActivityIndicator(ctxt);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

////////////////


static MCSoundAudioCategory MCSoundAudioCategoryFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "ambient", kMCCompareCaseless))
        return kMCSoundAudioCategoryAmbient;
    else if (MCStringIsEqualToCString(p_string, "solo ambient", kMCCompareCaseless))
        return kMCSoundAudioCategorySoloAmbient;
    else if (MCStringIsEqualToCString(p_string, "playback", kMCCompareCaseless))
        return kMCSoundAudioCategoryPlayback;
    else if (MCStringIsEqualToCString(p_string, "record", kMCCompareCaseless))
        return kMCSoundAudioCategoryRecord;
    else if (MCStringIsEqualToCString(p_string, "play and record", kMCCompareCaseless))
        return kMCSoundAudioCategoryPlayAndRecord;
    else if (MCStringIsEqualToCString(p_string, "audio processing", kMCCompareCaseless))
        return kMCSoundAudioCategoryAudioProcessing;
    
    return kMCSoundAudioCategoryUnknown;
}

static bool MCSoundAudioCategoryTypeToString(MCSoundAudioCategory p_indicator, MCStringRef& r_string)
{
    switch (p_indicator)
    {
        case kMCSoundAudioCategoryAmbient:
            return MCStringCreateWithCString("ambient", r_string);
        case kMCSoundAudioCategorySoloAmbient:
            return MCStringCreateWithCString("solo ambient", r_string);
        case kMCSoundAudioCategoryPlayback:
            return MCStringCreateWithCString("playback", r_string);
        case kMCSoundAudioCategoryRecord:
            return MCStringCreateWithCString("record", r_string);
        case kMCSoundAudioCategoryPlayAndRecord:
            return MCStringCreateWithCString("play and record", r_string);
        case kMCSoundAudioCategoryAudioProcessing:
            return MCStringCreateWithCString("audio processing", r_string);
        default:
            return false;
    }
}

static MCSoundChannelStatus MCSoundChannelStatusFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "playing", kMCCompareCaseless))
        return kMCSoundChannelStatusPlaying;
    else if (MCStringIsEqualToCString(p_string, "paused", kMCCompareCaseless))
        return kMCSoundChannelStatusPaused;
    else //if (MCStringIsEqualToCString(p_string, "stopped", kMCCompareCaseless))
        return kMCSoundChannelStatusStopped;
}

static bool MCSoundChannelStatusTypeToString(MCSoundChannelStatus p_status, MCStringRef& r_string)
{
    switch(p_status)
    {
        case kMCSoundChannelStatusPaused:
            return MCStringCreateWithCString("paused", r_string);
        case kMCSoundChannelStatusStopped:
            return MCStringCreateWithCString("stopped", r_string);
        case kMCSoundChannelStatusPlaying:
            return MCStringCreateWithCString("playing", r_string);
        default:
            return false;
    }
}

static MCSoundChannelPlayType MCSoundChannelPlayTypeFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "next", kMCCompareCaseless))
        return kMCSoundChannelPlayNext;
    else if (MCStringIsEqualToCString(p_string, "looping", kMCCompareCaseless))
        return kMCSoundChannelPlayLooping;
    else
        return kMCSoundChannelPlayNow;
}

static bool MCSoundChannelPlayTypeToString(MCSoundChannelPlayType p_type, MCStringRef& r_string)
{
    switch(p_type)
    {
        case kMCSoundChannelPlayLooping:
            return MCStringCreateWithCString("play looping", r_string);
        case kMCSoundChannelPlayNext:
            return MCStringCreateWithCString("play next", r_string);
        default:
            return MCStringCreateWithCString("play now", r_string);
    }
}


Exec_stat MCHandlePlaySoundOnChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_sound;
    MCAutoStringRef t_channel;
    MCAutoStringRef t_type;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "xxx", &(&t_sound), &(&t_channel), &(&t_type));
	
    MCSoundChannelPlayType t_play_type;
	if (t_success)
        t_play_type = MCSoundChannelPlayTypeFromString(*t_type);
    
    if(t_success)
		MCSoundExecPlaySoundOnChannel(ctxt, *t_channel, *t_sound, (integer_t)t_play_type);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandlePausePlayingOnChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
	if (t_success)
		MCSoundExecPausePlayingOnChannel(ctxt, *t_channel);
	
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleResumePlayingOnChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
    
	MCAutoStringRef t_channel;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
	if (t_success)
		MCSoundExecResumePlayingOnChannel(ctxt, *t_channel);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleStopPlayingOnChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
	if (t_success)
		MCSoundExecStopPlayingOnChannel(ctxt, *t_channel);
	
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleDeleteSoundChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
	if (t_success)
		MCSoundExecDeleteSoundChannel(ctxt, *t_channel);
	
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleSetSoundChannelVolume(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	int32_t t_volume;
	MCAutoStringRef t_channel;

    if (t_success)
		t_success = MCParseParameters(p_parameters, "xu", &(&t_channel), &t_volume);
	
	if (t_success)
		MCSoundSetVolumeOfChannel(ctxt, *t_channel, t_volume);
	    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleSoundChannelVolume(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
	int32_t t_volume;
	if (t_success)
        MCSoundGetVolumeOfChannel(ctxt, *t_channel, t_volume);
		
	if (!ctxt . HasError())
    {
        ctxt.SetTheResultToNumber(t_volume);
		return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleSoundChannelStatus(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
	intenum_t t_status;
	if (t_success)
		MCSoundGetStatusOfChannel(ctxt, *t_channel, t_status);
	
	if (t_success && t_status >= 0 && !ctxt.HasError())
	{
        MCAutoStringRef t_status_string;
        if (MCSoundChannelStatusTypeToString((MCSoundChannelStatus)t_status, &t_status_string))
        {
            ctxt.SetTheResultToValue(*t_status_string);
            return ES_NORMAL;
        }
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleSoundOnChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
    MCAutoStringRef t_sound;
	if (t_success)
		MCSoundGetSoundOfChannel(ctxt, *t_channel, &t_sound);
	
    if (t_success)
        if (*t_sound != nil)
            ep.setvalueref(*t_sound);
    
    if (!ctxt . HasError())
    {
        ctxt . SetTheResultToValue(*t_sound);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleNextSoundOnChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
    
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
    MCAutoStringRef t_sound;
	if (t_success)
		MCSoundGetNextSoundOfChannel(ctxt, *t_channel, &t_sound);
	
    if (t_success)
        if (*t_sound != nil)
            ep.setvalueref(*t_sound);
    
    if (!ctxt . HasError())
    {
        ctxt . SetTheResultToValue(*t_sound);
		return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleSoundChannels(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    bool t_success;
	t_success = true;
    
    MCAutoStringRef t_channels;
	if (t_success)
		MCSoundGetSoundChannels(ctxt, &t_channels);
    
    if (!ctxt . HasError())
    {
        ctxt . SetTheResultToValue(*t_channels);
		return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

// MM-2012-09-07: Added support for setting the category of the current audio session (how mute button is handled etc.
Exec_stat MCHandleSetAudioCategory(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_category_string;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_category_string));
    
    MCSoundAudioCategory t_category;
    t_category = kMCSoundAudioCategoryUnknown;
    if (t_success)
    {
        MCSoundAudioCategoryFromString(*t_category_string);
    }
    
    if (t_success)
        MCSoundSetAudioCategory(ctxt, t_category);
    
    if (t_success)
	{
        MCAutoStringRef t_result;
		ep . copyasstringref(&t_result);
		ctxt . SetTheResultToValue(*t_result);
	}
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

/////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleGetDeviceToken (void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCAutoStringRef t_token;
    
    MCMiscGetDeviceToken(ctxt, &t_token);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_token);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleGetLaunchUrl (void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCAutoStringRef t_url;
    
    MCMiscGetLaunchUrl (ctxt, &t_url);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_url);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleBeep(void *p_context, MCParameter *p_parameters)
{
    int32_t t_number_of_times;
    int32_t* t_number_of_times_ptr = nil;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    if (!MCParseParameters(p_parameters, "i", &t_number_of_times))
        t_number_of_times_ptr = &t_number_of_times;
    
    MCMiscExecBeep(ctxt, t_number_of_times_ptr);
    
    if (!ctxt.HasError())
        return ES_NORMAL;

    return ES_ERROR;
}

Exec_stat MCHandleVibrate(void *p_context, MCParameter *p_parameters)
{
    int32_t t_number_of_times;
    int32_t* t_number_of_times_ptr = nil;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    if (MCParseParameters(p_parameters, "i", &t_number_of_times))
        t_number_of_times_ptr = &t_number_of_times;
    
    MCMiscExecVibrate(ctxt, t_number_of_times_ptr);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleDeviceResolution(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCAutoStringRef t_resolution;
    MCMiscGetDeviceResolution(ctxt, &t_resolution);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_resolution);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleUseDeviceResolution(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_use_device_res;
    bool t_use_control_device_res;
    bool t_success;
    
    t_success = MCParseParameters(p_parameters, "bb", &t_use_device_res, &t_use_control_device_res);
    
    if (t_success)
        MCMiscSetUseDeviceResolution(ctxt, t_use_device_res, t_use_control_device_res);
    
    if (!ctxt.HasError() && t_success)
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleDeviceScale(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    real64_t t_resolution;
    
    MCMiscGetDeviceScale(ctxt, t_resolution);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToNumber(t_resolution);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandlePixelDensity(void* context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    real64_t t_density;
    
    MCMiscGetPixelDensity(ctxt, t_density);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToNumber(t_density);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

static MCMiscStatusBarStyle MCMiscStatusBarStyleFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "translucent", kMCCompareCaseless))
        return kMCMiscStatusBarStyleTranslucent;
    else if (MCStringIsEqualToCString(p_string, "opaque", kMCCompareCaseless))
        return kMCMiscStatusBarStyleOpaque;
    else
        return kMCMiscStatusBarStyleDefault;
}

Exec_stat MCHandleSetStatusBarStyle(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	
    MCAutoStringRef t_status_bar_style_string;
    MCMiscStatusBarStyle t_status_bar_style;
    bool t_success;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_status_bar_style_string));
    
    if (t_success)
    {
        t_status_bar_style = MCMiscStatusBarStyleFromString(*t_status_bar_style_string);
        MCMiscSetStatusBarStyle(ctxt, t_status_bar_style);
    }
	
	if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleShowStatusBar(void* context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    bool t_success = true;
        
    if(t_success)
        MCMiscExecShowStatusBar(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleHideStatusBar(void* context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    bool t_success = true;
    
    if(t_success)
        MCMiscExecHideStatusBar(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

static MCMiscKeyboardType MCMiscKeyboardTypeFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "alphabet", kMCCompareCaseless))
        return kMCMiscKeyboardTypeAlphabet;
    else if (MCStringIsEqualToCString(p_string, "numeric", kMCCompareCaseless))
        return kMCMiscKeyboardTypeNumeric;
    else if (MCStringIsEqualToCString(p_string, "decimal", kMCCompareCaseless))
        return kMCMiscKeyboardTypeDecimal;
    else if (MCStringIsEqualToCString(p_string, "number", kMCCompareCaseless))
        return kMCMiscKeyboardTypeNumber;
    else if (MCStringIsEqualToCString(p_string, "phone", kMCCompareCaseless))
        return kMCMiscKeyboardTypePhone;
    else if (MCStringIsEqualToCString(p_string, "email", kMCCompareCaseless))
        return kMCMiscKeyboardTypeEmail;
    else if (MCStringIsEqualToCString(p_string, "url", kMCCompareCaseless))
        return kMCMiscKeyboardTypeUrl;
    else if (MCStringIsEqualToCString(p_string, "contact", kMCCompareCaseless))
        return kMCMiscKeyboardTypeContact;
    else // default
        return kMCMiscKeyboardTypeDefault;
}

Exec_stat MCHandleSetKeyboardType (void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_success = true;
    
    MCAutoStringRef t_keyboard_type_string;
    MCMiscKeyboardType t_keyboard_type;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_keyboard_type_string));
    
    t_keyboard_type = MCMiscKeyboardTypeFromString(*t_keyboard_type_string);
    
    MCMiscSetKeyboardType(ctxt, t_keyboard_type);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    return ES_ERROR;
}

static MCMiscKeyboardReturnKey MCMiscKeyboardReturnKeyTypeFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "go", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyGo;
    else if (MCStringIsEqualToCString(p_string, "google", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyGoogle;
    else if (MCStringIsEqualToCString(p_string, "join", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyJoin;
    else if (MCStringIsEqualToCString(p_string, "next", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyNext;
    else if (MCStringIsEqualToCString(p_string, "route", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyRoute;
    else if (MCStringIsEqualToCString(p_string, "search", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeySearch;
    else if (MCStringIsEqualToCString(p_string, "send", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeySend;
    else if (MCStringIsEqualToCString(p_string, "yahoo", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyYahoo;
    else if (MCStringIsEqualToCString(p_string, "done", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyDone;
    else if (MCStringIsEqualToCString(p_string, "emergency call", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyEmergencyCall;
    else // default
        return kMCMiscKeyboardReturnKeyDefault;
}

Exec_stat MCHandleSetKeyboardReturnKey (void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCAutoStringRef t_keyboard_return_key_string;
    MCMiscKeyboardReturnKey t_keyboard_return_key;
    bool t_success;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_keyboard_return_key_string));
    
    if (t_success)
    {
        t_keyboard_return_key = MCMiscKeyboardReturnKeyTypeFromString(*t_keyboard_return_key_string);
        MCMiscSetKeyboardReturnKey(ctxt, t_keyboard_return_key);
    }
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandlePreferredLanguages(void *context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    MCAutoStringRef t_preferred_languages;
    
    MCMiscGetPreferredLanguages(ctxt, &t_preferred_languages);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_preferred_languages);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleCurrentLocale(void *context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCAutoStringRef t_current_locale;
    
    MCMiscGetCurrentLocale(ctxt, &t_current_locale);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_current_locale);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleClearTouches(void* context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCMiscExecClearTouches(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleSystemIdentifier(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCAutoStringRef t_identifier;
    
    MCMiscGetSystemIdentifier(ctxt, &t_identifier);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_identifier);
        return ES_NORMAL;
    }

    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleApplicationIdentifier(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCAutoStringRef r_identifier;
    
    MCMiscGetApplicationIdentifier(ctxt, &r_identifier);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleSetReachabilityTarget(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_hostname;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_hostname));
	
	if (t_success)
		MCMiscSetReachabilityTarget(ctxt, *t_hostname);
	
	return !ctxt.HasError() ? ES_NORMAL : ES_ERROR;
}

Exec_stat MCHandleReachabilityTarget(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCAutoStringRef t_hostname;
    
    MCMiscGetReachabilityTarget(ctxt, &t_hostname);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_hostname);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleExportImageToAlbum(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef r_save_result;
    MCAutoStringRef t_file_name;
    
    bool t_file_extension_ok = false;
    
	MCAutoStringRef t_raw_data;
    bool t_success = true;
	MCLog("MCHandleExportImageToAlbum() called", nil);
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_raw_data));
    
    if (t_success)
    {
        if (!MCParseParameters(p_parameters, "x", &(&t_file_name)))
            t_file_name = kMCEmptyString;
    
        MCMiscExecExportImageToAlbum(ctxt, *t_raw_data, *t_file_name);
    }
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleSetRedrawInterval(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	bool t_success;
	t_success = true;
	
	int32_t t_interval;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "i", &t_interval);
	
	if (t_success)
        MCMiscSetRedrawInterval(ctxt, t_interval);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleSetAnimateAutorotation(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	bool t_success;
	t_success = true;

	bool t_enabled;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "b", &t_enabled);
    
    if (t_success)
        MCMiscSetAnimateAutorotation(ctxt, t_enabled);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}


Exec_stat MCHandleFileSetDoNotBackup(void *context, MCParameter *p_parameters)
{    
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	
    MCAutoStringRef t_path;
    
    bool t_success = true;
	bool t_no_backup;
	t_no_backup = true;
    
	if (t_success)
        t_success = MCParseParameters(p_parameters, "xu", &(&t_path), &t_no_backup);
    
    if (t_success)
        MCMiscSetDoNotBackupFile(ctxt, *t_path, t_no_backup);
    
    if (!ctxt . HasError())
        return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleFileGetDoNotBackup(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	MCAutoStringRef t_path;
    bool t_no_backup;
    bool t_success = true;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_path));
    
    if (t_success)
        MCMiscGetDoNotBackupFile(ctxt, *t_path, t_no_backup);
    
	if (!ctxt . HasError())
    {
        if (t_no_backup)
            ctxt.SetTheResultToValue(kMCTrueString);
        else
            ctxt.SetTheResultToValue(kMCFalseString);
        
		return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleFileSetDataProtection(void *context, MCParameter *p_parameters)
{    
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCAutoStringRef t_filename;
    MCAutoStringRef t_protection_string;
    
    if (MCParseParameters(p_parameters, "xx", &(&t_filename), &(&t_protection_string)))
	{
        MCMiscSetFileDataProtection(ctxt, *t_filename, *t_protection_string);
	}
    
	if (!ctxt . HasError())
		return ES_NORMAL;
	
	return ES_ERROR;
}

Exec_stat MCHandleFileGetDataProtection(void *context, MCParameter *p_parameters)
{    
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCAutoStringRef t_path;
    MCAutoStringRef t_protection_string;
    
    if (MCParseParameters(p_parameters, "x", &(&t_path)))
        MCMiscGetFileDataProtection(ctxt, *t_path, &t_protection_string);
    
	if (!ctxt . HasError())
    {
        ctxt.SetTheResultToValue(*t_protection_string);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleLibUrlDownloadToFile(void *context, MCParameter *p_parameters)
{
	MCAutoStringRef t_url, t_filename;
	
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	
    bool t_success;
    
    t_success = MCParseParameters(p_parameters, "xx", &(&t_url), &(&t_filename));
    
    if (t_success)
        MCMiscExecLibUrlDownloadToFile(ctxt, *t_url, *t_filename);
	
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////

static MCMediaType MCMediaTypeFromCString(const char *p_string)
{
    const char *t_ptr = p_string;
    MCMediaType t_media_type = kMCUnknownMediaType;
    
    while (true)
    {
        while(*t_ptr == ' ' || *t_ptr == ',')
            t_ptr += 1;
        if (*t_ptr == '\0')
            break;
    	// HC-2012-02-01: [[ Bug 9983 ]] - This fix is related as the implementation in the new syntax does not produce a result
        if (MCCStringEqualSubstringCaseless(t_ptr, "podcasts", 7))
            t_media_type = t_media_type | kMCMediaTypePodcasts;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "songs", 4))
            t_media_type = t_media_type | kMCMediaTypeSongs;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "audiobooks", 9))
            t_media_type = t_media_type | kMCMediaTypeAudiobooks;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "movies", 5))
            t_media_type = t_media_type | kMCMediaTypeMovies;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "musicvideos", 10))
            t_media_type = t_media_type | kMCMediaTypeMusicVideos;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "tv", 2))
            t_media_type = t_media_type | kMCMediaTypeTv;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "videopodcasts", 12))
            t_media_type = t_media_type | kMCMediaTypeVideoPodcasts;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "anyAudio", 12))
            t_media_type = t_media_type | kMCMediaTypeAnyAudio;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "anyVideo", 12))
            t_media_type = t_media_type | kMCMediaTypeAnyVideo;
        while(*t_ptr != ' ' && *t_ptr != ',' && *t_ptr != '\0')
            t_ptr += 1;
        
    }
    return t_media_type;
}

//iphonePickMedia [multiple] [, music, podCast, audioBook, anyAudio, movie, tv, videoPodcast, musicVideo, videoITunesU, anyVideo]
Exec_stat MCHandleIPhonePickMedia(void *context, MCParameter *p_parameters)
{
	bool t_success, t_allow_multipe_items;
	char *t_option_list;
    const char *r_return_media_types;
	MCMediaType t_media_types;
	
	t_success = true;
	t_allow_multipe_items = false;
	t_media_types = 0;
	
	t_option_list = nil;
	
    MCExecPoint ep(nil, nil, nil);
    
	// Get the options list.
	t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	while (t_success)
	{
		if (MCCStringEqualCaseless(t_option_list, "true"))
			t_allow_multipe_items = true;
		else if (MCCStringEqualCaseless(t_option_list, "music"))
			t_media_types |= kMCMediaTypeSongs;
		else if (MCCStringEqualCaseless(t_option_list, "podcast"))
			t_media_types += kMCMediaTypePodcasts;
		else if (MCCStringEqualCaseless(t_option_list, "audiobook"))
			t_media_types += kMCMediaTypeAudiobooks;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
		{
			if (MCCStringEqualCaseless(t_option_list, "movie"))
				t_media_types += kMCMediaTypeMovies;
			else if (MCCStringEqualCaseless(t_option_list, "tv"))
				t_media_types += kMCMediaTypeTv;
			else if (MCCStringEqualCaseless(t_option_list, "videoPodcast"))
				t_media_types += kMCMediaTypeVideoPodcasts;
			else if (MCCStringEqualCaseless(t_option_list, "musicVideo"))
				t_media_types += kMCMediaTypeMusicVideos;
			else if (MCCStringEqualCaseless(t_option_list, "videoITunesU"))
				t_media_types += kMCMediaTypeMovies;
		}
#endif
		t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	}
	if (t_media_types == 0)
	{
		t_media_types = MCMediaTypeFromCString("podcast, songs, audiobook");;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
			t_media_types += MCMediaTypeFromCString("movies, tv, videoPodcasts, musicVideos, videoITunesU");;
#endif
	}
    MCExecContext ctxt(ep);
    
	// Call MCIPhonePickMedia to process the media pick selection.
    MCPickExecPickMedia(ctxt, (intset_t)t_media_types, t_allow_multipe_items);
	
	return ES_NORMAL;
}

Exec_stat MCHandlePickMedia(void *context, MCParameter *p_parameters)
{
	bool t_success;
    t_success = true;
    char *t_option_list;
    MCMediaType t_media_type;
    t_media_type = kMCUnknownMediaType;
    
	t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	while (t_success)
	{
		if ((MCCStringEqualCaseless(t_option_list, "music")) ||
		    (MCCStringEqualCaseless(t_option_list, "podCast")) ||
		    (MCCStringEqualCaseless(t_option_list, "audioBook")) ||
            (MCCStringEqualCaseless(t_option_list, "anyAudio")))
        {
            t_media_type += kMCMediaTypeAnyAudio;
        }
		if ((MCCStringEqualCaseless(t_option_list, "movie")) ||
			(MCCStringEqualCaseless(t_option_list, "tv")) ||
            (MCCStringEqualCaseless(t_option_list, "videoPodcast")) ||
            (MCCStringEqualCaseless(t_option_list, "musicVideo")) ||
            (MCCStringEqualCaseless(t_option_list, "videoITunesU")) ||
            (MCCStringEqualCaseless(t_option_list, "anyVideo")))
        {
            t_media_type += kMCMediaTypeAnyVideo;
		}
		t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	}
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCPickExecPickMedia(ctxt, (intset_t)t_media_type, false);
    
	return ES_NORMAL;
}

Exec_stat MCHandlePick(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
	bool t_use_cancel, t_use_done, t_use_picker, t_use_checkmark, t_more_optional, t_success;
	t_success = true;
	t_more_optional = true;
	t_use_checkmark = false;
	t_use_done = false;
	t_use_cancel = false;
	t_use_picker = false;
	
    
    MCAutoArray<MCStringRef> t_option_lists;
    MCAutoArray<uindex_t> t_indices;
    
    MCStringRef t_string_param;
   	uint32_t t_initial_index;
    // get the mandatory options list and the initial index
    // HC-30-2011-30 [[ Bug 10036 ]] iPad pick list only returns 0.
	t_success = MCParseParameters(p_parameters, "x", &t_string_param);
    if (t_success)
    {
        t_success = MCParseParameters(p_parameters, "u", &t_initial_index);
        if (!t_success)
        {
            // Degrade gracefully, even if the second mandatory parameter is not supplied.
            t_initial_index = 0;
            t_success = true;
        }
        t_option_lists . Push(t_string_param);
        t_indices . Push(t_initial_index);
    }
    
    // get further options lists if they exist
    while (t_success && t_more_optional)
    {
    	t_success = MCParseParameters(p_parameters, "x", &t_string_param);
        if (t_success)
        {
            if (t_string_param != nil)
            {
                if (MCStringIsEqualToCString(t_string_param, "checkmark", kMCCompareCaseless) ||
                    MCStringIsEqualToCString(t_string_param, "cancel", kMCCompareCaseless) ||
                    MCStringIsEqualToCString(t_string_param, "done", kMCCompareCaseless) ||
                    MCStringIsEqualToCString(t_string_param, "cancelDone", kMCCompareCaseless) ||
                    MCStringIsEqualToCString(t_string_param, "picker", kMCCompareCaseless))
                        t_more_optional = false;
                else
                {
                    t_success = MCParseParameters(p_parameters, "u", &t_initial_index);
                    if (!t_success)
                    {
                        // Degrade gracefully, even if the second mandatory parameter is not supplied.
                        t_initial_index = 0;
                        t_success = true;
                    }
                    t_option_lists . Push(t_string_param);
                    t_indices . Push(t_initial_index);
                }
            }
            else
                t_more_optional = false;
        }
    }
    
    // now process any additional parameters
    
    MCPickButtonType t_type = kMCPickButtonNone;
    
    while (t_success && t_string_param != nil)
    {
        if (MCStringIsEqualToCString(t_string_param, "checkmark", kMCCompareCaseless))
            t_use_checkmark = true;
        else if (MCStringIsEqualToCString(t_string_param, "cancel", kMCCompareCaseless))
            t_type = kMCPickButtonCancel;
        else if (MCStringIsEqualToCString(t_string_param, "done", kMCCompareCaseless))
            t_type = kMCPickButtonDone;
        else if (MCStringIsEqualToCString(t_string_param, "canceldone", kMCCompareCaseless))
            t_type = kMCPickButtonCancelAndDone;
        else if (MCStringIsEqualToCString(t_string_param, "picker", kMCCompareCaseless))
            t_use_picker = true;
        
        MCValueRelease(t_string_param);
        t_success = MCParseParameters(p_parameters, "x", &t_string_param);
    }
    
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    
	// call the Exec method to process the pick wheel
	MCPickExecPickOptionByIndex(ctxt, (int)kMCLines, t_option_lists . Ptr(), t_option_lists . Size(), t_indices . Ptr(), t_indices . Size(),t_use_checkmark, t_use_picker, t_use_cancel, t_use_done, MCtargetptr->getrect());
    
	if (t_success)
    {
        // at the moment, this is the only way to return a value from the function.  pick (date/time/...) should probably
        // set the value of the 'it' variable
		if (MCresult->isempty())
		{
			MCAutoStringRef t_value;
			/* UNCHECKED */ ep . copyasstringref(&t_value);
			ctxt . SetTheResultToValue(*t_value);
		}
    }

    // Free memory
    for (uindex_t i = 0; i < t_option_lists . Size(); i++)
        MCValueRelease(t_option_lists[i]);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

// MM-2012-11-02: Temporarily refactored mobilePickDate to use the old syntax (rather than three separate pick date, pick time, pick date and time).
Exec_stat MCHandlePickDate(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
	bool t_success;
	t_success = true;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    char *t_type;
    t_type = nil;
    
    if (t_success && p_parameters != nil)
  		t_success = MCParseParameters(p_parameters, "s", &t_type);
    
    MCAutoStringRef t_current, t_start, t_end;
    
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_current);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_start);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_end);
        p_parameters = p_parameters->getnext();
    }
	
    int32_t t_step;
    int32_t *t_step_ptr = nil;
    if (t_success && p_parameters != nil)
        if (MCParseParameters(p_parameters, "i", &t_step))
            t_step_ptr = &t_step;
    
    MCPickButtonType t_button_type = kMCPickButtonNone;
    if (t_success && p_parameters != nil)
    {
        char *t_button;
        t_button = nil;
		t_success = MCParseParameters(p_parameters, "s", &t_button);
        if (t_success)
        {
            if (MCCStringEqualCaseless("cancel", t_button))
                t_button_type = kMCPickButtonCancel;
            else if (MCCStringEqualCaseless("done", t_button))
                t_button_type = kMCPickButtonDone;
            else if (MCCStringEqualCaseless("canceldone", t_button))
                t_button_type = kMCPickButtonCancelAndDone;
        }
        MCCStringFree(t_button);
    }
    
    MCExecContext ctxt(ep);
     
	if (t_success)
    {
        // MM-2012-03-15: [[ Bug ]] Make sure we handle no type being passed.
        if (t_type == nil)
            MCPickExecPickDate(ctxt, *t_current, *t_start, *t_end, (intenum_t)t_button_type, MCtargetptr->getrect());
        else if (MCCStringEqualCaseless("time", t_type))
            MCPickExecPickTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr->getrect());
        else if (MCCStringEqualCaseless("datetime", t_type))
            MCPickExecPickDateAndTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr->getrect());
        else
            MCPickExecPickDate(ctxt, *t_current, *t_start, *t_end, (intenum_t)t_button_type, MCtargetptr->getrect());
    }
    
    MCCStringFree(t_type);
    
    // at the moment, this is the only way to return a value from the function.  pick (date/time/...) should probably
    // set the value of the 'it' variable
    if (MCresult->isempty())
	{
		MCAutoStringRef t_value;
		/* UNCHECKED */ ep . copyasstringref(&t_value);
        ctxt . SetTheResultToValue(*t_value);
	}
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandlePickTime(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
	bool t_success, t_use_done, t_use_cancel;
	t_success = true;
	t_use_done = false;
	t_use_cancel = false;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    MCAutoStringRef t_current, t_start, t_end;
    
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_current);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_start);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_end);
        p_parameters = p_parameters->getnext();
    }
	
    int32_t t_step;
    int32_t *t_step_ptr = nil;
    if (t_success && p_parameters != nil)
        if (MCParseParameters(p_parameters, "i", &t_step))
            t_step_ptr = &t_step;
    
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_cancel);
	
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_done);
    
    MCPickButtonType t_button_type = kMCPickButtonNone;
    if (t_success)
    {
        if (t_use_cancel)
        {
            if (t_use_done)
                t_button_type = kMCPickButtonCancelAndDone;
            else
                t_button_type = kMCPickButtonCancel;
        }
        else if (t_use_done)
            t_button_type = kMCPickButtonDone;           
    }

    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    
	if (t_success)
		MCPickExecPickTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr->getrect());
    
    if (MCresult->isempty())
	{
		MCAutoStringRef t_value;
		/* UNCHECKED */ ep . copyasstringref(&t_value);
        ctxt . SetTheResultToValue(*t_value);
	}
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}


Exec_stat MCHandlePickDateAndTime(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
	bool t_success, t_use_done, t_use_cancel;
	t_success = true;
	t_use_done = false;
	t_use_cancel = false;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    MCAutoStringRef t_current, t_start, t_end;
    
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_current);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_start);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_end);
        p_parameters = p_parameters->getnext();
    }
	
    int32_t t_step;
    int32_t *t_step_ptr = nil;
    if (t_success && p_parameters != nil)
        if (MCParseParameters(p_parameters, "i", &t_step))
            t_step_ptr = &t_step;
    
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_cancel);
	
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_done);
    
    MCPickButtonType t_button_type = kMCPickButtonNone;
    if (t_success)
    {
        if (t_use_cancel)
        {
            if (t_use_done)
                t_button_type = kMCPickButtonCancelAndDone;
            else
                t_button_type = kMCPickButtonCancel;
        }
        else if (t_use_done)
            t_button_type = kMCPickButtonDone;
    }
    
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
       
	if (t_success)
		MCPickExecPickDateAndTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr->getrect());
    
    if (MCresult->isempty())
	{
		MCAutoStringRef t_value;
		/* UNCHECKED */ ep . copyasstringref(&t_value);
        ctxt . SetTheResultToValue(*t_value);
	}
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

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
	
    MCExecContext ctxt(ep);
    intset_t t_result;
    
    MCPickGetSpecificCameraFeatures(ctxt, (intenum_t)t_source, t_result);
	
	MCCameraFeaturesType t_features_set;
    t_features_set = (MCCameraFeaturesType)t_result;
    
	if ((t_features_set & kMCCameraFeaturePhoto) != 0)
		ctxt . GetEP() . concatcstring("photo", EC_COMMA, ctxt . GetEP() . isempty());
	if ((t_features_set & kMCCameraFeatureVideo) != 0)
		ctxt . GetEP() . concatcstring("video", EC_COMMA, ctxt . GetEP() . isempty());
	if ((t_features_set & kMCCameraFeatureFlash) != 0)
		ctxt . GetEP() . concatcstring("flash", EC_COMMA, ctxt . GetEP() . isempty());
	
    MCAutoStringRef t_features;
    /* UNCHECKED */ ep . copyasstringref(&t_features);
    ctxt . SetTheResultToValue(&t_result);
    
	return ES_NORMAL;
}

Exec_stat MCHandleCameraFeatures(void *context, MCParameter *p_parameters)
{
    if (p_parameters != nil)
		return MCHandleSpecificCameraFeatures(context, p_parameters);
    
    MCExecPoint ep(nil, nil, nil);
	ep.clear();
    
    MCExecContext ctxt(ep);
    
    intset_t t_features;
    
    MCPickGetCameraFeatures(ctxt, t_features);
    
    MCCamerasFeaturesType t_features_set;
    t_features_set = (MCCamerasFeaturesType)t_features;
    
	if ((t_features_set & kMCCamerasFeatureFrontPhoto) != 0)
		ctxt . GetEP() . concatcstring("front photo", EC_COMMA, ctxt . GetEP() . isempty());
	if ((t_features_set & kMCCamerasFeatureFrontVideo) != 0)
		ctxt . GetEP() . concatcstring("front video", EC_COMMA, ctxt . GetEP() . isempty());
	if ((t_features_set & kMCCamerasFeatureFrontFlash) != 0)
		ctxt . GetEP() . concatcstring("front flash", EC_COMMA, ctxt . GetEP() . isempty());
   	if ((t_features_set & kMCCamerasFeatureRearPhoto) != 0)
		ctxt . GetEP() . concatcstring("rear photo", EC_COMMA, ctxt . GetEP() . isempty());
	if ((t_features_set & kMCCamerasFeatureRearVideo) != 0)
		ctxt . GetEP() . concatcstring("rear video", EC_COMMA, ctxt . GetEP() . isempty());
	if ((t_features_set & kMCCamerasFeatureRearFlash) != 0)
		ctxt . GetEP() . concatcstring("rear flash", EC_COMMA, ctxt . GetEP() . isempty());
    
    MCAutoStringRef t_features_string;
    /* UNCHECKED */ ep . copyasstringref(&t_features_string);
    ctxt . SetTheResultToValue(*t_features_string);
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
	else if (MCU_strcasecmp(t_source, "camera") == 0)
        t_photo_source = kMCPhotoSourceTypeCamera;
    else if (MCU_strcasecmp(t_source, "rear camera") == 0)
		t_photo_source = kMCPhotoSourceTypeRearCamera;
	else if (MCU_strcasecmp(t_source, "front camera") == 0)
		t_photo_source = kMCPhotoSourceTypeFrontCamera;
	else
	{
		MCresult -> sets("unknown source");
		return ES_NORMAL;
	}
	
	/////
	
	MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
	
	if (t_width != 0 && t_height != 0)
		MCPickExecPickPhotoAndResize(ctxt, t_photo_source, t_width, t_height);
	else
		MCPickExecPickPhoto(ctxt, t_photo_source);
    
	if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleControlCreate(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_type_name;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_type_name));
	
	MCAutoStringRef t_control_name;
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "x", &(&t_control_name));
	
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCNativeControlExecCreateControl(ctxt, *t_type_name, *t_control_name);
    
	if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleControlDelete(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	MCAutoStringRef t_control_name;
	if (MCParseParameters(p_parameters, "x", &(&t_control_name)))
        MCNativeControlExecDeleteControl(ctxt, *t_control_name);

	return ES_NORMAL;
}

Exec_stat MCHandleControlSet(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	char *t_control_name;
	char *t_prop_name;
	t_control_name = nil;
	t_prop_name = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "ss", &t_control_name, &t_prop_name);
	
	MCNativeControl *t_control;
	MCNativeControlProperty t_property;
	if (t_success)
		t_success =
		MCNativeControl::FindByNameOrId(t_control_name, t_control) &&
		MCNativeControl::LookupProperty(t_prop_name, t_property);
	
	MCExecPoint ep(nil, nil, nil);
	if (t_success && p_parameters != nil)
		t_success = p_parameters -> eval(ep);
	
	if (t_success)
		t_success = t_control -> Set(t_property, ep) == ES_NORMAL;
	
	delete t_prop_name;
	delete t_control_name;
	
	return ES_NORMAL;
}

Exec_stat MCHandleControlGet(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	char *t_control_name;
	char *t_prop_name;
	t_control_name = nil;
	t_prop_name = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "ss", &t_control_name, &t_prop_name);
	
	MCNativeControl *t_control;
	MCNativeControlProperty t_property;
	if (t_success)
		t_success =
		MCNativeControl::FindByNameOrId(t_control_name, t_control) &&
		MCNativeControl::LookupProperty(t_prop_name, t_property);
	
	MCExecPoint ep(nil, nil, nil);
	if (t_success)
		t_success = t_control -> Get(t_property, ep) == ES_NORMAL;
	
	MCExecContext ctxt(ep);
	if (t_success)
	{
		MCAutoStringRef t_value;
        ep . copyasstringref(&t_value);
        ctxt . SetTheResultToValue(*t_value);
	}
	else
		ctxt . SetTheResultToEmpty();
    
	delete t_prop_name;
	delete t_control_name;
	
	return ES_NORMAL;
	
}

Exec_stat MCHandleControlDo(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	char *t_control_name;
	char *t_action_name;
	t_control_name = nil;
	t_action_name = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "ss", &t_control_name, &t_action_name);
	
	MCNativeControl *t_control;
	MCNativeControlAction t_action;
	if (t_success)
		t_success =
		MCNativeControl::FindByNameOrId(t_control_name, t_control) &&
		MCNativeControl::LookupAction(t_action_name, t_action);
	
	if (t_success)
		t_success = t_control -> Do(t_action, p_parameters) == ES_NORMAL;
	
	delete t_action_name;
	delete t_control_name;
	
	return ES_NORMAL;
}

Exec_stat MCHandleControlTarget(void *context, MCParameter *p_parameters)
{
	MCNativeControl *t_target;
	t_target = MCNativeControl::CurrentTarget();
	if (t_target != nil)
	{
		if (t_target -> GetName() != nil)
			MCresult -> copysvalue(t_target -> GetName());
		else
			MCresult -> setnvalue(t_target -> GetId());
	}
	else
		MCresult -> clear();
	
	return ES_NORMAL;
}

bool list_native_controls(void *context, MCNativeControl* p_control)
{
	MCExecPoint *ep;
	ep = (MCExecPoint *)context;
	
	if (p_control -> GetName() != nil)
		ep -> concatcstring(p_control -> GetName(), EC_RETURN, ep -> isempty());
	else
		ep -> concatuint(p_control -> GetId(), EC_RETURN, ep -> isempty());
	
	return true;
}

Exec_stat MCHandleControlList(void *context, MCParameter *p_parameters)
{
    
	MCExecPoint ep(nil, nil, nil);
	MCNativeControl::List(list_native_controls, &ep);
    
	MCExecContext ctxt(ep);
	MCAutoStringRef t_value;
    ep . copyasstringref(&t_value);
    ctxt . SetTheResultToValue(*t_value);
    
	return ES_NORMAL;
}
