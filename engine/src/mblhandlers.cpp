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

	t_success = MCParseParameters(p_parameters, "x", &t_recipients);
    if (t_success == false)
    {
        MCresult -> sets(MCfalsestring);
        return ES_NORMAL;
    }
	t_success = MCParseParameters(p_parameters, "x", &t_body);
    
    ep . clear();
    MCExecContext ctxt(ep);
    
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
    
    if(ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;
}
    
Exec_stat MCHandleRestorePurchases(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCStoreExecRestorePurchases(ctxt);
    
    if(ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;
}


Exec_stat MCHandlePurchaseList(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCAutoStringRef t_list;
    
    MCStoreGetPurchaseList(ctxt, &t_list);
    
    
    if(ctxt.HasError())
        return ES_ERROR;
    else
    {
        ctxt.SetTheResultToValue(*t_list);
        return ES_NORMAL;
    }
}

Exec_stat MCHandlePurchaseCreate(void* p_context, MCParameter* p_parameters)
{
    bool t_success = true;
    MCAutoStringRef t_product_id;
    uint32_t t_id;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &t_product_id);
    
    if (t_success)
        MCStoreExecCreatePurchase(ctxt, &t_product_id, t_id);
    
    if (ctxt.HasError())
    {
        ctxt.SetTheResultToEmpty();
        return ES_ERROR;
    }
    else
    {
        ctxt.SetTheResultToNumber(t_id);
        return ES_NORMAL;
    }
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
	
	if (ctxt.HasError())
    {
        ctxt.SetTheResultToEmpty();
        return ES_ERROR;}
    else
    {
        ctxt.SetTheResultToValue(*t_state);
        return ES_NORMAL;
    }
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
    
    if (ctxt.HasError())
    {
        ctxt.SetTheResultToEmpty();
        return ES_ERROR;
    }
    else
    {
        ctxt.SetTheResultToValue(t_error);
        return ES_NORMAL;
    }
}

Exec_stat MCHandlePurchaseGet(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	MCAutoStringRef t_prop_name;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "us", &t_id, &t_prop_name);
	
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
		t_success = MCParseParameters(p_parameters, "uxu", &t_id, &t_prop_name, &t_quantity);
		
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	if (t_success)
        MCStoreSetPurchaseProperty(ctxt, t_id, &t_prop_name, t_quantity);
	
    if (ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;
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
    
	if (ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;
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
    
    if (ctxt.HasError())
        return ES_ERROR;
	else
        return ES_NORMAL;
}


Exec_stat MCHandleRequestProductDetails(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef t_product;
    bool t_success = true;    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &t_product);
        
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
/*	MCExecPoint ep(nil, nil, nil);
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
		for(uint32_t i = 0; i < t_orientations_count; i++)
			for(uint32_t j = 0; get_orientation_name(j) != nil; j++)
				if (MCCStringEqualCaseless(t_orientations_array[i], s_orientation_names[j]))
					t_orientations_set |= (1 << j);
	
	for(uint32_t i = 0; i < t_orientations_count; i++)
		MCCStringFree(t_orientations_array[i]);
	MCMemoryDeleteArray(t_orientations_array);

	MCOrientationSetAllowedOrientations(ctxt, t_orientations_set);

	if (!ctxt . HasError())
		return ES_NORMAL;
*/
	return ES_ERROR;
}

Exec_stat MCHandleOrientationLocked(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	bool t_locked;
	MCOrientationGetOrientationLocked(ctxt, t_locked);

	//ctxt . SetTheResultToValue(t_locked);

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

	//MCMailExecSendMail(t_address, t_cc_address, t_subject, t_message_body);
	
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
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &t_subject, &t_to, &t_cc, &t_bcc, &t_body, &t_attachments);

	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	if (t_success)
//		MCMailExecComposeMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

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
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &t_subject, &t_to, &t_cc, &t_bcc, &t_body, &t_attachments);

	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	if (t_success)
//		MCMailExecComposeMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

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
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &t_subject, &t_to, &t_cc, &t_bcc, &t_body, &t_attachments);

	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	if (t_success)
//		MCMailExecComposeUnicodeMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

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
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &t_subject, &t_to, &t_cc, &t_bcc, &t_body, &t_attachments);

	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	if (t_success)
//		MCMailExecComposeHtmlMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

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

//	ctxt . SetTheResultToValue(t_can_send);
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
        MCSensorExecStartTrackingSensor(ctxt, t_sensor, t_loosely);
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
        MCSensorExecStopTrackingSensor(ctxt, t_sensor);
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
        MCSensorExecStopTrackingSensor(ctxt, kMCSensorTypeAcceleration);
    
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
        MCSensorExecStopTrackingSensor(ctxt, kMCSensorTypeLocation);
    
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
        MCSensorExecStopTrackingSensor(ctxt, kMCSensorTypeHeading);
    
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

 /*   switch (t_sensor)
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
    } */
    
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

// MM-2012-02-11: Added support old style senseor syntax (iPhoneGetCurrentLocation etc)
Exec_stat MCHandleCurrentLocation(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_detailed_reading;
    //MCSensorGetDetailedLocationOfDevice(ctxt, &t_detailed_reading);
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
    //MCSensorGetDetailedHeadingOfDevice(ctxt, &t_detailed_reading);
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
    MCSensorSetLocationCalibration(ctxt, t_timeout);
    
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
    MCSensorGetLocationCalibration(ctxt, t_timeout);
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
    //MCSensorGetSensorAvailable(ctxt, t_sensor, t_available);
    
    MCresult->sets(MCU_btos(t_available));
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
    //MCSensorGetSensorAvailable(ctxt, kMCSensorTypeLocation, t_available);
    
    MCresult->sets(MCU_btos(t_available));
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
    //MCSensorGetSensorAvailable(ctxt, kMCSensorTypeHeading, t_available);
    
    MCresult->sets(MCU_btos(t_available));
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

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

////////////////////////////////////////////////////////////////////////////////

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

	if (MCParseParameters(p_parameters, "axxx", &t_contact, &t_title, &t_message, &t_alternate_name))
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
	
	/* UNCHECKED */ MCParseParameters(p_parameters, "a", &t_contact);

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
    const char *t_contact_name = NULL;
    const char *r_result = NULL;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_name = ep.getcstring();
    }
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    //MCAddressBookExecFindContact(ctxt, t_contact_name);
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
		t_success = MCParseParameters(p_parameters, "x", &t_key);
	
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
		t_success = MCParseParameters(p_parameters, "xx", &t_ad, &t_type);
    
    uint32_t t_topleft_x;
    uint32_t t_topleft_y;

    if (t_success)
        t_success = MCParseParameters(p_parameters, "uu", &t_topleft_x, &t_topleft_y);
    
    MCAutoArrayRef t_metadata;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "a", &t_metadata);
    
	if (t_success)
		MCAdExecCreateAd(ctxt, *t_ad, *t_type, t_topleft_x, t_topleft_y, *t_metadata);
    
    
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
		t_success = MCParseParameters(p_parameters, "x", &t_ad);
	
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
		t_success = MCParseParameters(p_parameters, "x", &t_ad);
	
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
		t_success = MCParseParameters(p_parameters, "xb", &t_ad, &t_visible);
	
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
		t_success = MCParseParameters(p_parameters, "x", &t_ad);
	
    uint32_t t_topleft_x;
    uint32_t t_topleft_y;
    
	if (t_success)
		MCAdGetTopLeftOfAd(ctxt, *t_ad, t_topleft_x, t_topleft_y);
    
    if (!ctxt . HasError())
    {
        MCAutoStringRef t_topleft_string;
        if(MCStringFormat(&t_topleft_string, "uu", t_topleft_x, t_topleft_y))
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
    uint32_t t_topleft_x;
    uint32_t t_topleft_y;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "xuu", &t_ad, t_topleft_x, t_topleft_y);
    
	if (t_success)
		MCAdSetTopLeftOfAd(ctxt, *t_ad, t_topleft_x, t_topleft_y);
    
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
        t_success = MCParseParameters(p_parameters, "x", &t_id);
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
        t_success = MCParseParameters(p_parameters, "x", &t_id);
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
    MCcalendarExecCreateEvent(ctxt);
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
        t_success = MCParseParameters(p_parameters, "x", &t_id);
    
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
    t_success = MCParseParameters(p_parameters, "s", &t_id);
    
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
    // Handle parameters. We are doing that in a dedicated call
//    MCCalendar t_new_event_data;
//    t_new_event_data = MCParameterDataToCalendar(p_parameters, t_new_event_data);
//    MCExecContext ctxt(ep);
//    ctxt.SetTheResultToEmpty();
//    // Call the Exec implementation
//    MCAddEventExec(ctxt, t_new_event_data);
//    // Set return value
//    if (!ctxt . HasError())
//		return ES_NORMAL;
//    
//	return ES_ERROR;
}

Exec_stat MCHandleGetCalendarsEvent(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
//    MCGetCalendarsEvent(ctxt);
//    // Set return value
//    if (!ctxt . HasError())
//		return ES_NORMAL;
//    
//	return ES_ERROR;
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
		t_success = MCParseParameters (p_parameters, "xxx", &t_notification_body, &t_notification_action, &t_notification_user_info);
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
        t_success = MCParseParameters(p_parameters, "xx", &t_indicator_string, &t_label);
    
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
    
    t_success = MCParseParameters(p_parameters, "x", &t_style_string);
    
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
	t_sound = nil;
	t_channel = nil;
	t_type = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "xxx", &t_sound, &t_channel, &t_type);
	
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
		t_success = MCParseParameters(p_parameters, "x", &t_channel);
	
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
		t_success = MCParseParameters(p_parameters, "x", &t_channel);
	
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
		t_success = MCParseParameters(p_parameters, "x", &t_channel);
	
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
		t_success = MCParseParameters(p_parameters, "x", &t_channel);
	
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
		t_success = MCParseParameters(p_parameters, "xu", &t_channel, &t_volume);
	
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
		t_success = MCParseParameters(p_parameters, "x", &t_channel);
	
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
		t_success = MCParseParameters(p_parameters, "x", &t_channel);
	
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
		t_success = MCParseParameters(p_parameters, "x", &t_channel);
	
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
		t_success = MCParseParameters(p_parameters, "x", &t_channel);
	
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
	
    if (t_success)
        if (*t_channels != nil)
            ep.setvalueref(*t_channels);
    
    if (!ctxt . HasError() && t_success)
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
		t_success = MCParseParameters(p_parameters, "x", &t_category_string);
    
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
    MCMiscGetiphoneDeviceResolution(ctxt, &t_resolution);
    
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
        MCMiscSetiphoneUseDeviceResolution(ctxt, t_use_device_res, t_use_control_device_res);
    
    if (!ctxt.HasError() && t_success)
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleDeviceScale(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    real64_t t_resolution;
    
    MCMiscGetiphoneDeviceScale(ctxt, t_resolution);
    
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
    
    t_success = MCParseParameters(p_parameters, "x", &t_status_bar_style_string);
    
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
    
    t_success = MCParseParameters(p_parameters, "x", &t_keyboard_type_string);
    
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
    
    t_success = MCParseParameters(p_parameters, "x", &t_keyboard_return_key_string);
    
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
		t_success = MCParseParameters(p_parameters, "x", &t_hostname);
	
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
    
    t_success = MCParseParameters(p_parameters, "x", &t_raw_data);
    
    if (t_success)
    {
        if (!MCParseParameters(p_parameters, "x", &t_file_name))
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
        t_success = MCParseParameters(p_parameters, "xu", &t_path, &t_no_backup);
    
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
        t_success = MCParseParameters(p_parameters, "x", &t_path);
    
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
    
    if (MCParseParameters(p_parameters, "xx", &t_filename, &t_protection_string))
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
    
    if (MCParseParameters(p_parameters, "x", &t_path))
        MCMiscGetFileDataProtection(ctxt, *t_path, &t_protection_string);
    
	if (!ctxt . HasError())
    {
        ctxt.SetTheResultToValue(*t_protection_string);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

