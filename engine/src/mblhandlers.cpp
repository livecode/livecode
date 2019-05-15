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

#include "util.h"

#include "mblsyntax.h"
#include "mblsensor.h"
#include "mblcontrol.h"
#include "mblstore.h"

#include "foundation-chunk.h"


////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

static const char *s_orientation_names[] =
{
	"unknown", "portrait", "portrait upside down", "landscape right", "landscape left", "face up", "face down", nil
};

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleCanComposeTextMessage(void *p_context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
    MCExecContext ctxt(nil, nil, nil);
    
	t_success = MCParseParameters(p_parameters, "x", &(&t_recipients));
    if (t_success == false)
    {
         ctxt . SetTheResultToValue(kMCFalse);
        return ES_NORMAL;
    }
    t_success = MCParseParameters(p_parameters, "x", &(&t_body));
  
    
    if (t_success)
        MCTextMessagingExecComposeTextMessage(ctxt, *t_recipients, *t_body);
    
	return ES_NORMAL;
}


///////////////////////////////////////////////////////////////////////////


Exec_stat MCHandleLockIdleTimer(void* p_context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCIdleTimerExecLockIdleTimer(ctxt);
    
    return ES_NORMAL;
}


Exec_stat MCHandleUnlockIdleTimer(void* p_context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCIdleTimerExecUnlockIdleTimer(ctxt);
    
    return ES_NORMAL;    
}

Exec_stat MCHandleIdleTimerLocked(void* p_context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
    MCExecContext ctxt(nil, nil, nil);
    
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
    MCExecContext ctxt(nil, nil, nil);
    
    MCStoreExecEnablePurchaseUpdates(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleDisablePurchaseUpdates(void* p_context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCStoreExecDisablePurchaseUpdates(ctxt);
    
    if(!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}
    
Exec_stat MCHandleRestorePurchases(void* p_context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCStoreExecRestorePurchases(ctxt);
    
    if(!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}


Exec_stat MCHandlePurchaseList(void* p_context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_product_id));
    
    if (t_success)
        MCStoreExecCreatePurchase(ctxt, *t_product_id, t_id);
    
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
    
    MCExecContext ctxt(nil, nil, nil);
	
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
    
    MCExecContext ctxt(nil, nil, nil);
	
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
    MCAutoValueRef t_value;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "ux", &t_id, &(&t_prop_name));
	
    MCExecContext ctxt(nil, nil, nil);
	
	if (t_success)
        MCStoreExecGet(ctxt, t_id, *t_prop_name, &t_value);
	
	if (!ctxt . HasError())
    {
        ctxt . SetTheResultToValue(*t_value);
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
    MCAutoNumberRef t_quantity_ref;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "uxu", &t_id, &(&t_prop_name), &t_quantity);
		
	MCExecContext ctxt(nil, nil, nil);
    
	if (t_success)
        t_success = MCNumberCreateWithInteger(t_quantity, &t_quantity_ref);

    if (t_success)
        MCStoreExecSet(ctxt, t_id, &t_prop_name, *t_quantity_ref);
	
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
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCStoreExecSendPurchaseRequest(ctxt, t_id);
    
	if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleMakePurchase(void *context, MCParameter *p_parameters)
{
    bool t_success = true;
    
    MCAutoStringRef t_prod_id;
    MCAutoStringRef t_quantity;
    MCAutoStringRef t_payload;
    MCPurchase *t_purchase = nil;
    uint32_t t_id;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "xxx", &(&t_prod_id), &(&t_quantity), &(&t_payload));
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCStoreExecCreatePurchase(ctxt, *t_prod_id, t_id);
   
    if (t_success)
        MCStoreExecMakePurchase(ctxt, *t_prod_id, *t_quantity, *t_payload);
    
    if (!ctxt. HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleConfirmPurchase(void *context, MCParameter *p_parameters)
{
    bool t_success = true;
    
    MCAutoStringRef t_prod_id;
    MCPurchase *t_purchase = nil;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_prod_id));
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCStoreExecConfirmPurchase(ctxt, *t_prod_id);
    
    if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleProductSetType(void *context, MCParameter *p_parameter)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_product_id, t_product_type;
    bool t_success;
    
    t_success = MCParseParameters(p_parameter, "xx", &(&t_product_id), &(&t_product_type));
    
    if (t_success)
        MCStoreExecProductSetType(ctxt, *t_product_id, *t_product_type);
    
    if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}


Exec_stat MCHandlePurchaseConfirmDelivery(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
	
    MCExecContext ctxt(nil, nil, nil);
    
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
        
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCStoreExecRequestProductDetails(ctxt, *t_product);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleReceiveProductDetails(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef t_product;
    bool t_success = true;
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_product));
    
    MCExecContext ctxt(nil, nil, nil);
    MCAutoStringRef t_result;
    
    if (t_success)
        MCStoreExecReceiveProductDetails(ctxt, *t_product, &t_result);
    
    ctxt . SetTheResultToValue(*t_result);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleConsumePurchase(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef t_product;
    bool t_success = true;
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_product));
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCStoreExecConsumePurchase(ctxt, *t_product);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleGetPurchases(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_purchases;
    MCStoreGetPurchaseList(ctxt, &t_purchases);
    
    if (!ctxt . HasError())
    {
        ctxt . SetTheResultToValue(*t_purchases);
        return ES_NORMAL;
    }
    
    ctxt . SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandlePurchaseVerify(void *context, MCParameter *p_parameters)
{
    bool t_success = true;
    bool t_verified = true;
    
    uint32_t t_id;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "ub", &t_id, &t_verified);
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCStoreExecPurchaseVerify(ctxt, t_id, t_verified);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleGetPurchaseProperty(void *context, MCParameter *p_parameters)
{
    bool t_success = true;
    MCAutoStringRef t_product_id, t_prop_name, t_prop_value;
    MCExecContext ctxt(nil,nil,nil);
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "xx", &(&t_product_id), &(&t_prop_name));
    
    if (t_success)
        MCStoreGetPurchaseProperty(ctxt, *t_product_id, *t_prop_name, &t_prop_value);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_prop_value);
        return ES_NORMAL;
    }
    
    return ES_ERROR;
}

Exec_stat MCHandleSetPurchaseProperty(void *context, MCParameter *p_parameters)
{
    bool t_success = true;
    MCAutoStringRef t_product_id, t_prop_name, t_prop_value;
    MCExecContext ctxt(nil,nil,nil);
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "xxx", &(&t_product_id), &(&t_prop_name), &(&t_prop_value));
    if (t_success)
        MCStoreSetPurchaseProperty(ctxt, *t_product_id, *t_prop_name, *t_prop_value);
    
    if (ctxt.HasError())
        return ES_ERROR;
    
    return ES_NORMAL;
    
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleDeviceOrientation(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	intenum_t t_orientation;
	MCOrientationGetDeviceOrientation(ctxt, t_orientation);

	ctxt . SetTheResultToStaticCString(s_orientation_names[(int)t_orientation]);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleOrientation(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	intenum_t t_orientation;
	MCOrientationGetOrientation(ctxt, t_orientation);

	ctxt . SetTheResultToStaticCString(s_orientation_names[(int)t_orientation]);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleAllowedOrientations(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	intset_t t_orientations;
	MCOrientationGetAllowedOrientations(ctxt, t_orientations);

	bool t_success;
	t_success = true;
	
	MCAutoListRef t_orientation_list;

	if (t_success)
		t_success = MCListCreateMutable(',', &t_orientation_list);

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

Exec_stat MCHandleSetFullScreenRectForOrientations(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_orientations;
    
    bool t_success = p_parameters != nil;
    
    if (t_success)
    {
        MCAutoValueRef t_value;
        if (p_parameters -> eval_argument(ctxt, &t_value))
        {
            ctxt . ConvertToString(*t_value, &t_orientations);
        }
        t_success = t_orientations.IsSet();
        p_parameters = p_parameters -> getnext();
    }
    
    MCRectangle *t_rect_ptr = nullptr;
    MCRectangle t_rect;
    
    if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        if (p_parameters -> eval_argument(ctxt, &t_value))
        {
            bool t_have_rect = false;
            ctxt.TryToConvertToLegacyRectangle(*t_value, t_have_rect, t_rect);
            if (t_have_rect)
            {
                t_rect_ptr = &t_rect;
            }
        }
    }
    
    MCAutoArrayRef t_orientations_array;
    if (t_success)
    {
        t_success = MCStringSplit(*t_orientations, MCSTR(","), nil, kMCCompareExact, &t_orientations_array);
    }
    
    uint32_t t_orientations_count;
    if (t_success)
    {
        t_orientations_count = MCArrayGetCount(*t_orientations_array);
    }
    
    intset_t t_orientations_set = 0;
    if (t_success)
    {
        for(uint32_t i = 0; i < t_orientations_count; i++)
        {
            // Note: 't_orientations_array' is an array of strings
            MCValueRef t_orien_value = nil;
            if (MCArrayFetchValueAtIndex(*t_orientations_array, i + 1, t_orien_value))
            {
                MCStringRef t_orientation = (MCStringRef)(t_orien_value);
                if (MCStringIsEqualToCString(t_orientation, "portrait", kMCCompareCaseless))
                    t_orientations_set |= ORIENTATION_PORTRAIT;
                else if (MCStringIsEqualToCString(t_orientation, "portrait upside down", kMCCompareCaseless))
                    t_orientations_set |= ORIENTATION_PORTRAIT_UPSIDE_DOWN;
                else if (MCStringIsEqualToCString(t_orientation, "landscape right", kMCCompareCaseless))
                    t_orientations_set |= ORIENTATION_LANDSCAPE_RIGHT;
                else if (MCStringIsEqualToCString(t_orientation, "landscape left", kMCCompareCaseless))
                    t_orientations_set |= ORIENTATION_LANDSCAPE_LEFT;
                else if (MCStringIsEqualToCString(t_orientation, "face up", kMCCompareCaseless))
                    t_orientations_set |= ORIENTATION_FACE_UP;
                else if (MCStringIsEqualToCString(t_orientation, "face down", kMCCompareCaseless))
                    t_orientations_set |= ORIENTATION_FACE_DOWN;
            }
        }
    }
    
    if (t_success)
    {
        MCOrientationSetRectForOrientations(ctxt, t_orientations_set, t_rect_ptr);
    }
    
    if (t_success && !ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleSetAllowedOrientations(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	MCAutoStringRef t_orientations;
	
	if (p_parameters != nil)
	{
        MCAutoValueRef t_value;
		p_parameters -> eval_argument(ctxt, &t_value);
        ctxt . ConvertToString(*t_value, &t_orientations);
	}

    bool t_success = true;
    MCAutoArrayRef t_orientations_array;
    if (t_success)
        t_success = MCStringSplit(*t_orientations, MCSTR(","), nil, kMCCompareExact, &t_orientations_array);
    
    uint32_t t_orientations_count = MCArrayGetCount(*t_orientations_array);
    intset_t t_orientations_set;
	t_orientations_set = 0;
	if (t_success)
    {
		for(uint32_t i = 0; i < t_orientations_count; i++)
        {
            // Note: 't_orientations_array' is an array of strings
			MCValueRef t_orien_value = nil;
            /* UNCHECKED */ MCArrayFetchValueAtIndex(*t_orientations_array, i + 1, t_orien_value);
			MCStringRef t_orientation = (MCStringRef)(t_orien_value);
            if (MCStringIsEqualToCString(t_orientation, "portrait", kMCCompareCaseless))
                t_orientations_set |= ORIENTATION_PORTRAIT;
            else if (MCStringIsEqualToCString(t_orientation, "portrait upside down", kMCCompareCaseless))
                t_orientations_set |= ORIENTATION_PORTRAIT_UPSIDE_DOWN;
            else if (MCStringIsEqualToCString(t_orientation, "landscape right", kMCCompareCaseless))
                t_orientations_set |= ORIENTATION_LANDSCAPE_RIGHT;
            else if (MCStringIsEqualToCString(t_orientation, "landscape left", kMCCompareCaseless))
                t_orientations_set |= ORIENTATION_LANDSCAPE_LEFT;
            else if (MCStringIsEqualToCString(t_orientation, "face up", kMCCompareCaseless))
                t_orientations_set |= ORIENTATION_FACE_UP;
            else if (MCStringIsEqualToCString(t_orientation, "face down", kMCCompareCaseless))
                t_orientations_set |= ORIENTATION_FACE_DOWN;
            
        }
	}

	MCOrientationSetAllowedOrientations(ctxt, t_orientations_set);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleOrientationLocked(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

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
	MCExecContext ctxt(nil, nil, nil);

	MCOrientationExecLockOrientation(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleUnlockOrientation(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	MCOrientationExecUnlockOrientation(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleRotateInterface(void *context, MCParameter *p_parameters)
{
	return ES_NORMAL;
}


////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleRevMail(void *context, MCParameter *p_parameters)
{
	MCAutoStringRef t_address, t_cc_address, t_subject, t_message_body;

	MCExecContext ctxt(nil, nil, nil);
	
	if (p_parameters != nil)
	{
        MCAutoValueRef t_value;
		p_parameters -> eval_argument(ctxt, &t_value);
        ctxt . ConvertToString(*t_value, &t_address);
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
        MCAutoValueRef t_value;
		p_parameters -> eval_argument(ctxt, &t_value);
        ctxt . ConvertToString(*t_value, &t_cc_address);
        p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
        MCAutoValueRef t_value;
		p_parameters -> eval_argument(ctxt, &t_value);
        ctxt . ConvertToString(*t_value, &t_subject);
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
        MCAutoValueRef t_value;
		p_parameters -> eval_argument(ctxt, &t_value);
        ctxt . ConvertToString(*t_value, &t_message_body);
		p_parameters = p_parameters -> getnext();
	}

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

	MCExecContext ctxt(nil, nil, nil);

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

	MCExecContext ctxt(nil, nil, nil);

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

	MCExecContext ctxt(nil, nil, nil);

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

	MCExecContext ctxt(nil, nil, nil);

	if (t_success)
        MCMailExecComposeHtmlMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCanSendMail(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	bool t_can_send;

	MCMailGetCanSendMail(ctxt, t_can_send);
    
    if (t_can_send)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
    return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleStartTrackingSensor(void *p_context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    bool t_loosely = false;
    
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoStringRef t_string;
        p_parameters->eval_argument(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
        t_sensor = MCSensorTypeFromString(*t_string);
        p_parameters = p_parameters->getnext();
    }
    
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoBooleanRef t_bool;
        p_parameters->eval_argument(ctxt, &t_value);
        // PM-2015-03-11: [[ Bug 14855 ]] Evaluate correctly the second param
        if (ctxt . ConvertToBoolean(*t_value, &t_bool))
            t_loosely = MCValueIsEqualTo(*t_bool, kMCTrue);
        // if conversion fails, keep the same behaviour as in LC 6.7
        else
            t_loosely = false;
    }
    
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
    MCExecContext ctxt(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoStringRef t_string;
        p_parameters->eval_argument(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
        t_sensor = MCSensorTypeFromString(*t_string);
        p_parameters = p_parameters->getnext();
    }

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
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    bool t_detailed = false;
    
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoStringRef t_string;
        p_parameters->eval_argument(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
        t_sensor = MCSensorTypeFromString(*t_string);
        p_parameters = p_parameters->getnext();
    }
    
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoBooleanRef t_bool;
        p_parameters->eval_argument(ctxt, &t_value);
        // PM-2015-03-11: [[ Bug 14855 ]] Evaluate correctly the second param
        if(ctxt . ConvertToBoolean(*t_value, &t_bool))
            t_detailed = MCValueIsEqualTo(*t_bool, kMCTrue);
        // if conversion fails, keep the same behaviour as in LC 6.7
        else
            t_detailed = false;
    }
    
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
            ctxt . SetTheResultToValue(*t_detailed_reading);
    }
    else
    {
        if (*t_reading != nil)
            ctxt . SetTheResultToValue(*t_reading);
    }

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

// SN-2014-12-11: [[ Merge-6.7.1-rc-4 ]]
// PM-2014-12-08: [[ Bug 13659 ]] New function to detect if Voice Over is turned on (iOS only)
Exec_stat MCHandleIsVoiceOverRunning(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);

    bool t_is_vo_running;
    MCMiscGetIsVoiceOverRunning(ctxt, t_is_vo_running);

    if (!ctxt . HasError())
    {
        ctxt . SetTheResultToBool(t_is_vo_running);
        return ES_NORMAL;
    }

    return ES_ERROR;
}

// MM-2012-02-11: Added support old style sensor syntax (iPhoneGetCurrentLocation etc)
Exec_stat MCHandleCurrentLocation(void *p_context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_detailed_reading;
    MCSensorGetDetailedLocationOfDevice(ctxt, &t_detailed_reading);
    
    if (*t_detailed_reading != nil)    
        ctxt . SetTheResultToValue(*t_detailed_reading);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCurrentHeading(void *p_context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_detailed_reading;
    MCSensorGetDetailedHeadingOfDevice(ctxt, &t_detailed_reading);
    if (*t_detailed_reading != nil)
        ctxt . SetTheResultToValue(*t_detailed_reading);
    
	if (!ctxt . HasError())
        return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleSetHeadingCalibrationTimeout(void *p_context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    int t_timeout;
    t_timeout = 0;
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoNumberRef t_number;
        p_parameters->eval_argument(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToNumber(*t_value, &t_number);
        t_timeout = MCNumberFetchAsInteger(*t_number);
    }
    else
    {
        // We need a parameter
        ctxt . Throw();
    }
    
    if (!ctxt . HasError())
        MCSensorSetLocationCalibrationTimeout(ctxt, t_timeout);
    
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleHeadingCalibrationTimeout(void *p_context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    int t_timeout;
    MCSensorGetLocationCalibrationTimeout(ctxt, t_timeout);
    MCresult->setnvalue(t_timeout);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleSensorAvailable(void *p_context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();

    MCSensorType t_sensor;
    t_sensor = kMCSensorTypeUnknown;    
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoStringRef t_string;
        p_parameters->eval_argument(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
        t_sensor = MCSensorTypeFromString(*t_string);
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
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
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

Exec_stat MCHandleSetLocationHistoryLimit(void *p_context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    if (p_parameters == nil)
        return ES_NORMAL;
    
    MCAutoValueRef t_value;
    p_parameters->eval_argument(ctxt, &t_value);
        
    uinteger_t t_limit = 0;
    /* UNCHECKED */ ctxt . ConvertToUnsignedInteger(*t_value, t_limit);
    
    MCSensorSetLocationSampleLimit(t_limit);
    return ES_NORMAL;
}

Exec_stat MCHandleGetLocationHistoryLimit(void *p_context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    ctxt.SetTheResultToNumber(MCSensorGetLocationSampleLimit());
    return ES_NORMAL;
}

Exec_stat MCHandleGetLocationHistory(void *p_context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    MCAutoArrayRef t_array;
    MCSensorGetLocationHistoryOfDevice(ctxt, &t_array);
    ctxt.SetTheResultToValue(*t_array);
    return ES_NORMAL;
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
    MCExecContext ctxt(nil, nil, nil);

    MCAddressBookExecPickContact(ctxt);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleShowContact(void *context, MCParameter *p_parameters) // ABPersonViewController
{
    int32_t t_contact_id = 0;
    int32_t r_result;
    MCExecContext ctxt(nil, nil, nil);

    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoNumberRef t_number;
        p_parameters->eval_argument(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToNumber(*t_value, &t_number);
        t_contact_id = MCNumberFetchAsInteger(*t_number);
    }

    MCAddressBookExecShowContact(ctxt, t_contact_id);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCreateContact(void *context, MCParameter *p_parameters) // ABNewPersonViewController
{
    MCExecContext ctxt(nil, nil, nil);

    MCAddressBookExecCreateContact(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleUpdateContact(void *context, MCParameter *p_parameters) // ABUnknownPersonViewController
{
    MCExecContext ctxt(nil, nil, nil);

	MCAutoArrayRef t_contact;
	MCAutoStringRef t_title;
	MCAutoStringRef t_message;
	MCAutoStringRef t_alternate_name;

    // PM-2015-05-21: [[ Bug 14792 ]] Make sure params are parsed properly
	if (MCParseParameters(p_parameters, "a|xxx", &(&t_contact), &(&t_title), &(&t_message), &(&t_alternate_name)))
	    MCAddressBookExecUpdateContact(ctxt, *t_contact, *t_title, *t_message, *t_alternate_name);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleGetContactData(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);

    int32_t t_contact_id = 0;
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoNumberRef t_number;
        p_parameters->eval_argument(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToNumber(*t_value, &t_number);
        t_contact_id = MCNumberFetchAsInteger(*t_number);
    }
    
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
    MCExecContext ctxt(nil, nil, nil);

    int32_t t_contact_id = 0;
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoNumberRef t_number;
        p_parameters->eval_argument(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToNumber(*t_value, &t_number);
        t_contact_id = MCNumberFetchAsInteger(*t_number);
    }
    
	MCAddressBookExecRemoveContact(ctxt, t_contact_id);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleAddContact(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    // Handle parameters. We are doing that in a dedicated call
	MCAutoArrayRef t_contact;
	
	/* UNCHECKED */ MCParseParameters(p_parameters, "a", &(&t_contact));

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
    MCExecContext ctxt(nil, nil, nil);
    // Handle parameters.
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        p_parameters->eval_argument(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_contact_name);
    }
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
    
    MCExecContext ctxt(nil, nil, nil);
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
    
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;
    MCAutoStringRef t_type;
    
	if (t_success)
		t_success = MCParseParameters(p_parameters, "xx", &(&t_ad), &(&t_type));
    
    MCAdTopLeft t_topleft = {0,0};
    MCAutoStringRef t_topleft_string;
    
    if (t_success)
    {
        if (MCParseParameters(p_parameters, "x", &(&t_topleft_string)))
            /* UNCHECKED */ sscanf(MCStringGetCString(*t_topleft_string), "%u,%u", &t_topleft.x, &t_topleft.y);
    }
    
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
    
    MCExecContext ctxt(nil, nil, nil);
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
    
    MCExecContext ctxt(nil, nil, nil);
    
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
    
    MCExecContext ctxt(nil, nil, nil);
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
    
    MCExecContext ctxt(nil, nil, nil);
    
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
    
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
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
    
    MCExecContext ctxt(nil, nil, nil);
    
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
    
    MCExecContext ctxt(nil, nil, nil);
    
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
    
    MCExecContext ctxt(nil, nil, nil);

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
    
    MCExecContext ctxt(nil, nil, nil);
    
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
    
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
    
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
    MCExecContext ctxt(nil, nil, nil);
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

    MCExecContext ctxt(nil, nil, nil);
    // Handle parameters.
    if (p_parameters)
    {
        MCAutoValueRef t_value;

        if (p_parameters->eval_argument(ctxt, &t_value)
                && !MCValueIsEmpty(*t_value))
        {
			t_success = MCD_convert_to_datetime(ctxt, *t_value, CF_UNDEFINED, CF_UNDEFINED, t_start_date);
        }
        p_parameters = p_parameters->getnext();
    }
    if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;

        if (p_parameters->eval_argument(ctxt, &t_value)
                && !MCValueIsEmpty(*t_value))
        {
			t_success = MCD_convert_to_datetime(ctxt, *t_value, CF_UNDEFINED, CF_UNDEFINED, t_end_date);
        }
    }

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
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_success = true;
    MCAutoStringRef t_notification_body;
    MCAutoStringRef t_notification_action;
    MCAutoStringRef t_notification_user_info;
    MCDateTime t_date;
    bool t_play_sound_vibrate = true;
    int32_t t_badge_value = 0;
    
    ctxt.SetTheResultToEmpty();
    
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters (p_parameters, "xxx", &(&t_notification_body), &(&t_notification_action), &(&t_notification_user_info));
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        if (p_parameters->eval_argument(ctxt, &t_value)
                && !MCValueIsEmpty(*t_value))
        {
            t_success = MCD_convert_to_datetime(ctxt, *t_value, CF_UNDEFINED, CF_UNDEFINED, t_date);
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
    MCExecContext ctxt(nil, nil, nil);
    ctxt.SetTheResultToEmpty();
    
    MCNotificationGetRegisteredNotifications(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleGetNotificationDetails(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
    MCExecContext ctxt(nil, nil, nil);
    int32_t t_cancel_this;
    bool t_success;
    
    ctxt.SetTheResultToEmpty();
    if (p_parameters != nil)
    {
		t_success = MCParseParameters (p_parameters, "i", &t_cancel_this);
    
        if (t_success)
            MCNotificationExecCancelLocalNotification (ctxt, t_cancel_this);
    }
    else
        t_success = false;
    
    
    if (t_success && ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleCancelAllLocalNotifications (void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    ctxt.SetTheResultToEmpty();
    
    MCNotificationExecCancelAllLocalNotifications(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleGetNotificationBadgeValue (void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    ctxt.SetTheResultToEmpty();
    MCNotificationGetNotificationBadgeValue (ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleSetNotificationBadgeValue (void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    uint32_t t_badge_value;
    bool t_success = true;
    
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

static MCBusyIndicatorType MCBusyIndicatorTypeFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "in line", kMCCompareCaseless))
        return kMCBusyIndicatorInLine;
    else if (MCStringIsEqualToCString(p_string, "square", kMCCompareCaseless))
        return kMCBusyIndicatorSquare;
    else if (MCStringIsEqualToCString(p_string, "keyboard", kMCCompareCaseless))
        return kMCBusyIndicatorKeyboard;
    
    return kMCBusyIndicatorSquare;
}

static bool MCBusyIndicatorTypeToString(MCSensorType p_indicator, MCStringRef& r_string)
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

    if (t_success && p_parameters)
        t_success = MCParseParameters(p_parameters, "x", &(&t_indicator_string));
                                      
    if (t_success && p_parameters)
        t_success = MCParseParameters(p_parameters, "x", &(&t_label));
    
    intenum_t t_indicator;
    // PM-2014-11-21: [[ Bug 14068 ]] Nil check to prevent a crash
    if (t_success && p_parameters)
        t_success = MCBusyIndicatorTypeFromString(*t_indicator_string);
    
    int32_t t_opacity = -1;
    if (t_success && p_parameters)
    {
        t_success = MCParseParameters(p_parameters, "i", &t_opacity);
        if (t_opacity < 0 || t_opacity > 100)
            t_opacity = -1;
    }
    
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    MCBusyIndicatorExecStartBusyIndicator(ctxt, kMCBusyIndicatorSquare, *t_label, t_opacity);
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;    
}

Exec_stat MCHandleStopBusyIndicator(void *p_context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
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
            t_style = MCActivityIndicatorTypeFromString(MCSTR("large white"));
        else
            t_style = MCActivityIndicatorTypeFromString(*t_style_string);
    }
    
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
        
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    if (t_success)
        MCBusyIndicatorExecStartActivityIndicator(ctxt, t_style, t_location_x_ptr, t_location_y_ptr);
    
	if (t_success && !ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleStopActivityIndicator(void *p_context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
    MCAutoStringRef t_sound;
	if (t_success)
		MCSoundGetSoundOfChannel(ctxt, *t_channel, &t_sound);
	
    
    if (t_success &&
        !ctxt . HasError() &&
        *t_sound != nil)
    {
        ctxt . SetTheResultToValue(*t_sound);
        return ES_NORMAL;
    }
    
	return ES_ERROR;
}

Exec_stat MCHandleNextSoundOnChannel(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
    
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
    MCAutoStringRef t_sound;
	if (t_success)
		MCSoundGetNextSoundOfChannel(ctxt, *t_channel, &t_sound);
	
    
    if (t_success &&
        !ctxt . HasError() &&
        *t_sound != nil)
    {
        ctxt . SetTheResultToValue(*t_sound);
		return ES_NORMAL;
    }
    
    return ES_ERROR;
}

Exec_stat MCHandleSoundChannels(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
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
        t_category = MCSoundAudioCategoryFromString(*t_category_string);
    }
    
    if (t_success)
        MCSoundSetAudioCategory(ctxt, t_category);
        
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

/////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleGetDeviceToken (void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
    MCExecContext ctxt(nil, nil, nil);
    
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

Exec_stat MCHandleGetLaunchData(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);
	
	MCAutoArrayRef t_data;
	
	MCMiscGetLaunchData(ctxt, &t_data);
	
	if (!ctxt.HasError())
	{
		ctxt.SetTheResultToValue(*t_data);
		return ES_NORMAL;
	}
	
	ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleBeep(void *p_context, MCParameter *p_parameters)
{
    int32_t t_number_of_times;
    int32_t* t_number_of_times_ptr = nil;
    
    MCExecContext ctxt(nil, nil, nil);
    
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
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (MCParseParameters(p_parameters, "i", &t_number_of_times))
        t_number_of_times_ptr = &t_number_of_times;
    
    MCMiscExecVibrate(ctxt, t_number_of_times_ptr);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleDeviceResolution(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_use_device_res;
    bool t_use_control_device_res;
    bool t_success;
    
    t_success = MCParseParameters(p_parameters, "bb", &t_use_device_res, &t_use_control_device_res);
    
    if (t_success)
        MCMiscSetUseDeviceResolution(ctxt, t_use_device_res, t_use_control_device_res);
    
    if (t_success && !ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleDeviceScale(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
    MCExecContext ctxt(nil, nil, nil);
    
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

// SN-2014-10-15: [[ Merge-6.7.0-rc-3 ]]
static Exec_stat MCHandleLocationAuthorizationStatus(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef t_status;
    MCExecContext ctxt(nil, nil,nil);

    MCSensorGetLocationAuthorizationStatus(ctxt, &t_status);

    if (!ctxt . HasError())
    {
        ctxt . SetTheResultToValue(*t_status);
        return ES_NORMAL;
    }

    ctxt . SetTheResultToEmpty();
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
	MCExecContext ctxt(nil, nil, nil);
	
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
    MCExecContext ctxt(nil, nil, nil);
    bool t_success = true;
        
    if(t_success)
        MCMiscExecShowStatusBar(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleHideStatusBar(void* context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    bool t_success = true;
    
    if(t_success)
        MCMiscExecHideStatusBar(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

static MCInterfaceKeyboardType MCInterfaceKeyboardTypeFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "alphabet", kMCCompareCaseless))
        return kMCInterfaceKeyboardTypeAlphabet;
    else if (MCStringIsEqualToCString(p_string, "numeric", kMCCompareCaseless))
        return kMCInterfaceKeyboardTypeNumeric;
    else if (MCStringIsEqualToCString(p_string, "decimal", kMCCompareCaseless))
        return kMCInterfaceKeyboardTypeDecimal;
    else if (MCStringIsEqualToCString(p_string, "number", kMCCompareCaseless))
        return kMCInterfaceKeyboardTypeNumber;
    else if (MCStringIsEqualToCString(p_string, "phone", kMCCompareCaseless))
        return kMCInterfaceKeyboardTypePhone;
    else if (MCStringIsEqualToCString(p_string, "email", kMCCompareCaseless))
        return kMCInterfaceKeyboardTypeEmail;
    else if (MCStringIsEqualToCString(p_string, "url", kMCCompareCaseless))
        return kMCInterfaceKeyboardTypeUrl;
    else if (MCStringIsEqualToCString(p_string, "contact", kMCCompareCaseless))
        return kMCInterfaceKeyboardTypeContact;
    else // default
        return kMCInterfaceKeyboardTypeDefault;
}

Exec_stat MCHandleSetKeyboardType (void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);
    
    bool t_success = true;
    
    MCAutoStringRef t_keyboard_type_string;
    MCInterfaceKeyboardType t_keyboard_type;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_keyboard_type_string));
    
    t_keyboard_type = MCInterfaceKeyboardTypeFromString(*t_keyboard_type_string);
    
    MCMiscSetKeyboardType(ctxt, t_keyboard_type);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    return ES_ERROR;
}

static MCInterfaceReturnKeyType MCInterfaceReturnKeyTypeTypeFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "go", kMCCompareCaseless))
        return kMCInterfaceReturnKeyTypeGo;
    else if (MCStringIsEqualToCString(p_string, "google", kMCCompareCaseless))
        return kMCInterfaceReturnKeyTypeGoogle;
    else if (MCStringIsEqualToCString(p_string, "join", kMCCompareCaseless))
        return kMCInterfaceReturnKeyTypeJoin;
    else if (MCStringIsEqualToCString(p_string, "next", kMCCompareCaseless))
        return kMCInterfaceReturnKeyTypeNext;
    else if (MCStringIsEqualToCString(p_string, "route", kMCCompareCaseless))
        return kMCInterfaceReturnKeyTypeRoute;
    else if (MCStringIsEqualToCString(p_string, "search", kMCCompareCaseless))
        return kMCInterfaceReturnKeyTypeSearch;
    else if (MCStringIsEqualToCString(p_string, "send", kMCCompareCaseless))
        return kMCInterfaceReturnKeyTypeSend;
    else if (MCStringIsEqualToCString(p_string, "yahoo", kMCCompareCaseless))
        return kMCInterfaceReturnKeyTypeYahoo;
    else if (MCStringIsEqualToCString(p_string, "done", kMCCompareCaseless))
        return kMCInterfaceReturnKeyTypeDone;
    else if (MCStringIsEqualToCString(p_string, "emergency call", kMCCompareCaseless))
        return kMCInterfaceReturnKeyTypeEmergencyCall;
    else // default
        return kMCInterfaceReturnKeyTypeDefault;
}

Exec_stat MCHandleSetKeyboardReturnKey (void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_keyboard_return_key_string;
    MCInterfaceReturnKeyType t_keyboard_return_key;
    bool t_success;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_keyboard_return_key_string));
    
    if (t_success)
    {
        t_keyboard_return_key = MCInterfaceReturnKeyTypeTypeFromString(*t_keyboard_return_key_string);
        MCMiscSetKeyboardReturnKey(ctxt, t_keyboard_return_key);
    }
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

static const char *s_keyboard_display_names[] =
{
    "over", "pan", nil
};


Exec_stat MCHandleSetKeyboardDisplay(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    ctxt.SetTheResultToEmpty();
    
    MCAutoStringRef t_mode_string;
    if (!MCParseParameters(p_parameters, "x", &(&t_mode_string)))
    {
        return ES_ERROR;
    }
    
    bool t_success = true;
    
    intenum_t t_mode = 0;
    for (uint32_t i = 0; s_keyboard_display_names[i] != nil; i++)
    {
        if (MCStringIsEqualToCString(*t_mode_string, s_keyboard_display_names[i], kMCCompareCaseless))
        {
            t_mode = i;
            break;
        }
    }
    
    MCMiscExecSetKeyboardDisplay(ctxt, t_mode);
    
    if (!ctxt.HasError())
    {
        return ES_NORMAL;
    }
    
    return ES_ERROR;
}

Exec_stat MCHandleGetKeyboardDisplay(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    ctxt.SetTheResultToEmpty();
    
    intenum_t t_mode;
    MCMiscExecGetKeyboardDisplay(ctxt, t_mode);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToStaticCString(s_keyboard_display_names[t_mode]);
        return ES_NORMAL;
    }
    
    return ES_ERROR;
}

Exec_stat MCHandlePreferredLanguages(void *context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
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
    MCExecContext ctxt(nil, nil, nil);
    
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
    MCExecContext ctxt(nil, nil, nil);
    
    MCMiscExecClearTouches(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleSystemIdentifier(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_identifier;
    
    MCMiscGetApplicationIdentifier(ctxt, &t_identifier);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_identifier);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleSetReachabilityTarget(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
    MCExecContext ctxt(nil, nil, nil);
    
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
    
	MCAutoStringRef t_data_or_id;
    bool t_success = true;
	MCLog("MCHandleExportImageToAlbum() called", nil);
    
    MCExecContext ctxt(nil, nil, nil);
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_data_or_id));    
    
    MCLog("%@", *t_data_or_id);
    
    if (t_success)
    {
        if (!MCParseParameters(p_parameters, "x", &(&t_file_name)))
            t_file_name = kMCEmptyString;
    
        MCMiscExecExportImageToAlbum(ctxt, *t_data_or_id, *t_file_name);
    }
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleSetRedrawInterval(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
    MCExecContext ctxt(nil, nil, nil);
    
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
	MCExecContext ctxt(nil, nil, nil);
	
    MCAutoStringRef t_path;
    
    bool t_success = true;
	bool t_no_backup;
	t_no_backup = true;
    
	if (t_success)
        t_success = MCParseParameters(p_parameters, "xb", &(&t_path), &t_no_backup);
    
    if (t_success)
        MCMiscSetDoNotBackupFile(ctxt, *t_path, t_no_backup);
    
    if (!ctxt . HasError())
        return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleFileGetDoNotBackup(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);
    
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
	MCExecContext ctxt(nil, nil, nil);
    
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
	MCExecContext ctxt(nil, nil, nil);
    
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
	
	MCExecContext ctxt(nil, nil, nil);
	
    bool t_success;
    
    t_success = MCParseParameters(p_parameters, "xx", &(&t_url), &(&t_filename));
    
    if (t_success)
        MCMiscExecLibUrlDownloadToFile(ctxt, *t_url, *t_filename);
	
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleBuildInfo(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_value, t_key;
    bool t_success;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_key));
    
    if(t_success)
        MCMiscGetBuildInfo(ctxt, *t_key, &t_value);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_value);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

/////////////////// Android 6.0 runtime permissions /////////////////////////
Exec_stat MCHandleRequestPermission(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_permission;
    bool t_success, t_granted;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_permission));
    
    bool t_permission_exists;
    MCMiscExecPermissionExists(ctxt, *t_permission, t_permission_exists);
    
    if (!t_permission_exists)
    {
        ctxt.LegacyThrow(EE_BAD_PERMISSION_NAME);
        t_success = false;
    }
    
    if (t_success)
        MCMiscExecRequestPermission(ctxt, *t_permission, t_granted);
    
    Exec_stat t_stat;
    if (!ctxt . HasError())
        t_stat = ES_NORMAL;
    else
        t_stat = ES_ERROR;
    
    ctxt.SetTheResultToEmpty();
    return t_stat;
}

Exec_stat MCHandlePermissionExists(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_permission;
    bool t_success, t_exists;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_permission));
    
    if (t_success)
        MCMiscExecPermissionExists(ctxt, *t_permission, t_exists);
    
    if (!ctxt . HasError())
    {
        if (t_exists)
            ctxt.SetTheResultToValue(kMCTrueString);
        else
            ctxt.SetTheResultToValue(kMCFalseString);
        
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleHasPermission(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_permission;
    bool t_success, t_permission_granted;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_permission));
    
    bool t_permission_exists;
    MCMiscExecPermissionExists(ctxt, *t_permission, t_permission_exists);
    
    if (!t_permission_exists)
    {
        ctxt.LegacyThrow(EE_BAD_PERMISSION_NAME);
        t_success = false;
    }
    
    if (t_success)
        MCMiscExecHasPermission(ctxt, *t_permission, t_permission_granted);
    
    if (!ctxt . HasError())
    {
        if (t_permission_granted)
            ctxt.SetTheResultToValue(kMCTrueString);
        else
            ctxt.SetTheResultToValue(kMCFalseString);
        
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////

static MCMediaType MCMediaTypeFromString(MCStringRef p_string)
{
    MCMediaType t_media_type = kMCUnknownMediaType;
    
    uindex_t t_pos;
    t_pos = 0;
    while (true)
    {
        while (MCStringGetNativeCharAtIndex(p_string, t_pos) == ' ' || MCStringGetNativeCharAtIndex(p_string, t_pos) == ',')
            t_pos += 1;
        if (MCStringGetLength(p_string) == t_pos)
            break;
    	// HC-2012-02-01: [[ Bug 9983 ]] - This fix is related as the implementation in the new syntax does not produce a result
        
        if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 7), MCSTR("podcasts"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypePodcasts;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 4), MCSTR("songs"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeSongs;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 9), MCSTR("audiobooks"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeAudiobooks;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 5), MCSTR("movies"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeMovies;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 10), MCSTR("musicvideos"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeMusicVideos;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 2), MCSTR("tv"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeTv;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 12), MCSTR("videopodcasts"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeVideoPodcasts;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 12), MCSTR("anyAudio"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeAnyAudio;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 12), MCSTR("anyVideo"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeAnyVideo;
        while (MCStringGetNativeCharAtIndex(p_string, t_pos) != ' ' && MCStringGetNativeCharAtIndex(p_string, t_pos) != ',' && MCStringGetNativeCharAtIndex(p_string, t_pos) != '\0')
            t_pos += 1;
        
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
	
    MCExecContext ctxt(nil, nil, nil);
    
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
		t_media_types = MCMediaTypeFromString(MCSTR("podcast, songs, audiobook"));;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
			t_media_types += MCMediaTypeFromString(MCSTR("movies, tv, videoPodcasts, musicVideos, videoITunesU"));;
#endif
	}
    
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
    MCExecContext ctxt(nil, nil, nil);
    
    MCPickExecPickMedia(ctxt, (intset_t)t_media_type, false);
    
	return ES_NORMAL;
}

Exec_stat MCHandlePick(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
	bool t_use_cancel, t_use_done, t_use_picker, t_use_checkmark, t_use_hilite, t_success, t_has_buttons;
	t_success = true;
	t_use_checkmark = false;
	t_use_hilite = true;
	t_use_done = false;
	t_use_cancel = false;
	t_use_picker = false;
    t_has_buttons = false;
	
    
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
    while (t_success && p_parameters != nil)
    {
    	t_success = MCParseParameters(p_parameters, "x", &t_string_param);
        if (t_success)
        {
            if (MCStringIsEqualToCString(t_string_param, "checkmark", kMCCompareCaseless) ||
                MCStringIsEqualToCString(t_string_param, "cancel", kMCCompareCaseless) ||
                MCStringIsEqualToCString(t_string_param, "done", kMCCompareCaseless) ||
                MCStringIsEqualToCString(t_string_param, "cancelDone", kMCCompareCaseless) ||
                MCStringIsEqualToCString(t_string_param, "picker", kMCCompareCaseless))
            {
                t_has_buttons = true;
                break;
            }
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
    }
	
    // PM-2015-09-01: [[ Bug 15816 ]] Process any additional parameters correctly
    while (t_success && t_has_buttons && t_string_param != nil)
    {
        if (MCStringIsEqualToCString(t_string_param, "checkmark", kMCCompareCaseless))
            t_use_checkmark = true;
        else if (MCStringIsEqualToCString(t_string_param, "cancel", kMCCompareCaseless))
			t_use_cancel = true;
        else if (MCStringIsEqualToCString(t_string_param, "done", kMCCompareCaseless))
			t_use_done = true;
        else if (MCStringIsEqualToCString(t_string_param, "canceldone", kMCCompareCaseless))
		{
			t_use_cancel = true;
			t_use_done = true;
		}
        else if (MCStringIsEqualToCString(t_string_param, "picker", kMCCompareCaseless))
            t_use_picker = true;
        
        MCValueRelease(t_string_param);
		t_string_param = nil;
		
		if (p_parameters != nil)
			t_success = MCParseParameters(p_parameters, "x", &t_string_param);
		
    }
    
    ctxt.SetTheResultToEmpty();
    
    // PM-2016-02-19: [[ Bug 16945 ]] Make sure the use of checkmark is taken into account
	t_use_hilite = !t_use_checkmark;

	// call the Exec method to process the pick wheel
    // The function sets the result itself.
	if (t_success && !MCtargetptr)
	{
		ctxt.LegacyThrow(EE_NOTARGET);
		t_success = false;
	}
	if (t_success)
		MCPickExecPickOptionByIndex(ctxt, kMCChunkTypeLine, t_option_lists . Ptr(), t_option_lists . Size(), t_indices . Ptr(), t_indices . Size(), t_use_hilite, t_use_picker, t_use_cancel, t_use_done, MCtargetptr -> getrect());
    
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
    MCExecContext ctxt(nil, nil, nil);
    
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
        MCAutoValueRef t_value;
        p_parameters->eval_argument(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_current);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval_argument(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_start);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval_argument(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_end);
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
    if (t_success && !MCtargetptr)
    {
        ctxt.LegacyThrow(EE_NOTARGET);
        t_success = false;
    }
    if (t_success)
    {
        // MM-2012-03-15: [[ Bug ]] Make sure we handle no type being passed.
        if (t_type == nil)
            MCPickExecPickDate(ctxt, *t_current, *t_start, *t_end, (intenum_t)t_button_type, MCtargetptr -> getrect());
        else if (MCCStringEqualCaseless("time", t_type))
            MCPickExecPickTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr -> getrect());
        else if (MCCStringEqualCaseless("datetime", t_type))
            MCPickExecPickDateAndTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr -> getrect());
        else
            MCPickExecPickDate(ctxt, *t_current, *t_start, *t_end, (intenum_t)t_button_type, MCtargetptr -> getrect());
    }
    
    MCCStringFree(t_type);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandlePickTime(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
        MCAutoValueRef t_value;
        p_parameters->eval_argument(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_current);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval_argument(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_start);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval_argument(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_end);
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

    ctxt.SetTheResultToEmpty();

    if (!MCtargetptr)
    {
        ctxt.LegacyThrow(EE_NOTARGET);
        t_success = false;
    }

	if (t_success)
		MCPickExecPickTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr -> getrect());
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}


Exec_stat MCHandlePickDateAndTime(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
        MCAutoValueRef t_value;
        p_parameters->eval_argument(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_current);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval_argument(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_start);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval_argument(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_end);
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
    
    ctxt.SetTheResultToEmpty();

    if (!MCtargetptr)
    {
        ctxt.LegacyThrow(EE_NOTARGET);
        t_success = false;
    }
	if (t_success)
		MCPickExecPickDateAndTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr -> getrect());
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleSpecificCameraFeatures(void *p_context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);
	
	MCCameraSourceType t_source;
    MCAutoValueRef t_value;
    MCAutoStringRef t_string;
	p_parameters -> eval_argument(ctxt, &t_value);
    /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
    if (MCStringIsEqualToCString(*t_string, "front", kMCCompareCaseless))
		t_source = kMCCameraSourceTypeFront;
	else if (MCStringIsEqualToCString(*t_string, "rear", kMCCompareCaseless))
		t_source = kMCCameraSourceTypeRear;
	else
		return ES_NORMAL;
	
    intset_t t_result;
    
    MCPickGetSpecificCameraFeatures(ctxt, (intenum_t)t_source, t_result);
	
	MCCameraFeaturesType t_features_set;
    t_features_set = (MCCameraFeaturesType)t_result;
    MCAutoListRef t_list;
    /* UNCHECKED */ MCListCreateMutable(',', &t_list);
    
	if ((t_features_set & kMCCameraFeaturePhoto) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "photo");
	if ((t_features_set & kMCCameraFeatureVideo) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "video");
	if ((t_features_set & kMCCameraFeatureFlash) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "flash");
	
    MCAutoStringRef t_features;
    /* UNCHECKED */ MCListCopyAsString(*t_list, &t_features);
    ctxt . SetTheResultToValue(*t_features);
    
	return ES_NORMAL;
}

Exec_stat MCHandleCameraFeatures(void *context, MCParameter *p_parameters)
{
    if (p_parameters != nil)
		return MCHandleSpecificCameraFeatures(context, p_parameters);
    
    MCExecContext ctxt(nil, nil, nil);
    
    intset_t t_features;
    
    MCPickGetCameraFeatures(ctxt, t_features);
    
    MCCamerasFeaturesType t_features_set;
    t_features_set = (MCCamerasFeaturesType)t_features;
    
    MCAutoListRef t_list;
    /* UNCHECKED */ MCListCreateMutable(',', &t_list);
    
	if ((t_features_set & kMCCamerasFeatureFrontPhoto) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "front photo");
	if ((t_features_set & kMCCamerasFeatureFrontVideo) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "front video");
	if ((t_features_set & kMCCamerasFeatureFrontFlash) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "front flash");
   	if ((t_features_set & kMCCamerasFeatureRearPhoto) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "rear photo");
	if ((t_features_set & kMCCamerasFeatureRearVideo) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "rear video");
	if ((t_features_set & kMCCamerasFeatureRearFlash) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "rear flash");
    
    MCAutoStringRef t_features_string;

    if (!MCListCopyAsString(*t_list, &t_features_string))
        return ES_ERROR;

    ctxt . SetTheResultToValue(*t_features_string);

    return ES_NORMAL;
}

Exec_stat MCHandlePickPhoto(void *p_context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);
	
	MCParameter *t_source_param, *t_width_param, *t_height_param;
	t_source_param = p_parameters;
	t_width_param = t_source_param != nil ? t_source_param -> getnext() : nil;
	t_height_param = t_width_param != nil ? t_width_param -> getnext() : nil;
	
	int32_t t_width, t_height;
	t_width = t_height = 0;
	if (t_width_param != nil)
	{
		// MW-2013-07-01: [[ Bug 10989 ]] Make sure we force conversion to a number.
        MCAutoValueRef t_value;
        if (t_width_param -> eval_argument(ctxt, &t_value))
            /* UNCHECKED */ ctxt . ConvertToInteger(*t_value, t_width);
	}
	if (t_height_param != nil)
	{
		// MW-2013-07-01: [[ Bug 10989 ]] Make sure we force conversion to a number.
        MCAutoValueRef t_value;
        if (t_height_param -> eval_argument(ctxt, &t_value))
            /* UNCHECKED */ ctxt . ConvertToInteger(*t_value, t_height);
	}
    
    MCLog("%d, %d", t_width, t_height);
    
	MCAutoStringRef t_source;
	if (p_parameters != nil)
	{
        MCAutoValueRef t_value;
		p_parameters -> eval_argument(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_source);
	}
    
    if (!t_source.IsSet())
    {
        return ES_ERROR;
    }
	
	MCPhotoSourceType t_photo_source;
	bool t_is_take;
	t_is_take = false;
	
	if (MCStringIsEqualToCString(*t_source, "library", kMCCompareCaseless))
		t_photo_source = kMCPhotoSourceTypeLibrary;
	else if (MCStringIsEqualToCString(*t_source, "album", kMCCompareCaseless))
		t_photo_source = kMCPhotoSourceTypeAlbum;
	else if (MCStringIsEqualToCString(*t_source, "camera", kMCCompareCaseless))
        t_photo_source = kMCPhotoSourceTypeCamera;
    else if (MCStringIsEqualToCString(*t_source, "rear camera", kMCCompareCaseless))
		t_photo_source = kMCPhotoSourceTypeRearCamera;
	else if (MCStringIsEqualToCString(*t_source, "front camera", kMCCompareCaseless))
		t_photo_source = kMCPhotoSourceTypeFrontCamera;
	else
	{
		MCresult -> sets("unknown source");
		return ES_NORMAL;
	}
	
	/////
	
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

    MCExecContext ctxt(nil, nil, nil);
    
    MCNativeControlExecCreateControl(ctxt, *t_type_name, *t_control_name);
    
	if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleControlDelete(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
	MCAutoStringRef t_control_name;
	if (MCParseParameters(p_parameters, "x", &(&t_control_name)))
        MCNativeControlExecDeleteControl(ctxt, *t_control_name);

	return ES_NORMAL;
}

Exec_stat MCHandleControlSet(void *context, MCParameter *p_parameters)
{
    
    MCAutoStringRef t_control_name;
    MCAutoStringRef t_property;
    
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_success;
	t_success = true;
    
    if (t_success)
		t_success = MCParseParameters(p_parameters, "xx", &(&t_control_name), &(&t_property));

    MCAutoValueRef t_value;
    if (t_success && p_parameters != nil)
        t_success = p_parameters -> eval_argument(ctxt, &t_value);
    
    if (t_success)
        MCNativeControlExecSet(ctxt, *t_control_name, *t_property, *t_value);
    
    return ES_NORMAL;
}

Exec_stat MCHandleControlGet(void *context, MCParameter *p_parameters)
{
    
    MCAutoStringRef t_control_name;
    MCAutoStringRef t_property;
        
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_success;
	t_success = true;
    
    if (t_success)
		t_success = MCParseParameters(p_parameters, "xx", &(&t_control_name), &(&t_property));
    
    MCAutoValueRef t_value;
    if (t_success)
        MCNativeControlExecGet(ctxt, *t_control_name, *t_property, &t_value);
    
    if (*t_value != nil)
        ctxt . SetTheResultToValue(*t_value);
    else
        ctxt . SetTheResultToEmpty();
    
    return ES_NORMAL;
}

Exec_stat MCHandleControlDo(void *context, MCParameter *p_parameters)
{
    
    MCAutoStringRef t_control_name;
    MCAutoStringRef t_property;
    
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_success;
	t_success = true;
    
    if (t_success)
		t_success = MCParseParameters(p_parameters, "xx", &(&t_control_name), &(&t_property));
	
    MCAutoArray<MCValueRef> t_params;
    
    MCValueRef t_value;
    while (t_success && p_parameters != nil)
    {
        p_parameters -> eval_argument(ctxt, t_value);
        t_success = t_params . Push(t_value);
        p_parameters = p_parameters -> getnext();
    }

	if (t_success)
		MCNativeControlExecDo(ctxt, *t_control_name, *t_property, t_params . Ptr(), t_params . Size());

    
    // SN-2014-11-20: [[ Bug 14062 ]] Cleanup the memory
    for (uint32_t i = 0; i < t_params . Size(); ++i)
        MCValueRelease(t_params[i]);
	
	return ES_NORMAL;
}

Exec_stat MCHandleControlTarget(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    MCNativeControlIdentifier t_identifier;
    MCNativeControlGetTarget(ctxt, t_identifier);
    MCAutoStringRef t_string;
    MCNativeControlIdentifierFormat(ctxt, t_identifier, &t_string);
    MCNativeControlIdentifierFree(ctxt, t_identifier);
    if (*t_string != nil)
        ctxt . SetTheResultToValue(*t_string);
    
    return ES_NORMAL;
}

bool list_native_controls(void *context, MCNativeControl* p_control)
{
	MCListRef t_list;
	t_list = (MCListRef )context;
    MCAutoStringRef t_name;
	p_control -> GetName(&t_name);
	if (!MCStringIsEmpty(*t_name))
		MCListAppend(t_list, *t_name);
	else
		MCListAppendFormat(t_list, "%u", p_control -> GetId());
	
	return true;
}

Exec_stat MCHandleControlList(void *context, MCParameter *p_parameters)
{
    MCAutoListRef t_list;
    MCListCreateMutable('\n', &t_list);
	MCNativeControl::List(list_native_controls, *t_list);
    
	MCExecContext ctxt(nil, nil, nil);
	MCAutoStringRef t_value;
	/* UNCHECKED */ MCListCopyAsString(*t_list, &t_value);
    ctxt . SetTheResultToValue(*t_value);
    
	return ES_NORMAL;
}

// MW-2013-10-02: [[ MobileSSLVerify ]] Handle libUrlSetSSLVerification
Exec_stat MCHandleLibUrlSetSSLVerification(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	bool t_enabled;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "b", &t_enabled);
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCMiscExecLibUrlSetSSLVerification(ctxt, t_enabled);
    
    if (t_success && !ctxt . HasError())
        return ES_NORMAL;
	
	return ES_ERROR;
}

// MM-2013-05-21: [[ Bug 10895 ]] Added iphoneIdentifierForVendor as an initial replacement for iphoneSystemIdentifier.
//  identifierForVendor was only added to UIDevice in iOS 6.1 so make sure we weakly link.
Exec_stat MCHandleIdentifierForVendor(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_id;
    MCMiscGetIdentifierForVendor(ctxt, &t_id);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_id);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleEnableRemoteControl(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCMiscExecEnableRemoteControl(ctxt);

    if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleDisableRemoteControl(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCMiscExecDisableRemoteControl(ctxt);

    if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleRemoteControlEnabled(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_enabled;
    MCMiscGetRemoteControlEnabled(ctxt, t_enabled);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToBool(t_enabled);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleSetRemoteControlDisplay(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_success;
	t_success = true;
    
    MCAutoArrayRef t_props;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "a", &(&t_props));
    
    if (t_success)
        MCMiscSetRemoteControlDisplayProperties(ctxt, *t_props);
    
    if (t_success)
        return ES_NORMAL;
    else
        return ES_ERROR;
}

Exec_stat MCHandleIsNFCAvailable(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);
	ctxt.SetTheResultToEmpty();
	
	MCNFCGetIsNFCAvailable(ctxt);
	
	if (!ctxt.HasError())
		return ES_NORMAL;
	
	return ES_ERROR;
}

Exec_stat MCHandleIsNFCEnabled(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);
	ctxt.SetTheResultToEmpty();
	
	MCNFCGetIsNFCEnabled(ctxt);
	
	if (!ctxt.HasError())
		return ES_NORMAL;
	
	return ES_ERROR;
}

Exec_stat MCHandleEnableNFCDispatch(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);
	ctxt.SetTheResultToEmpty();
	
	MCNFCExecEnableNFCDispatch(ctxt);
	
	if (!ctxt.HasError())
		return ES_NORMAL;
	
	return ES_ERROR;
}

Exec_stat MCHandleDisableNFCDispatch(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);
	ctxt.SetTheResultToEmpty();
	
	MCNFCExecDisableNFCDispatch(ctxt);
	
	if (!ctxt.HasError())
		return ES_NORMAL;
	
	return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////


typedef Exec_stat (*MCPlatformMessageHandler)(void *context, MCParameter *parameters);

// MW-2012-08-06: [[ Fibers ]] If 'waitable' is true it means the handler must
//   be run on the script fiber. Otherwise it is run on the system fiber (making
//   implementation easier).
struct MCPlatformMessageSpec
{
	bool waitable;
	const char *message;
	MCPlatformMessageHandler handler;
	void *context;
};

static const MCPlatformMessageSpec s_platform_messages[] =
{
    // MM-2012-02-22: Added support for ad management
    {false, "mobileAdRegister", MCHandleAdRegister, nil},
    {false, "mobileAdCreate", MCHandleAdCreate, nil},
    {false, "mobileAdDelete", MCHandleAdDelete, nil},
    {false, "mobileAdGetVisible", MCHandleAdGetVisible, nil},
    {false, "mobileAdSetVisible", MCHandleAdSetVisible, nil},
    {false, "mobileAdGetTopLeft", MCHandleAdGetTopLeft, nil},
    {false, "mobileAdSetTopLeft", MCHandleAdSetTopLeft, nil},
    {false, "mobileAds", MCHandleAds, nil},
    {false, "iphoneAdRegister", MCHandleAdRegister, nil},
    {false, "iphoneAdCreate", MCHandleAdCreate, nil},
    {false, "iphoneAdDelete", MCHandleAdDelete, nil},
    {false, "iphoneAdGetVisible", MCHandleAdGetVisible, nil},
    {false, "iphoneAdSetVisible", MCHandleAdSetVisible, nil},
    {false, "iphoneAdGetTopLeft", MCHandleAdGetTopLeft, nil},
    {false, "iphoneAdSetTopLeft", MCHandleAdSetTopLeft, nil},
    {false, "iphoneAds", MCHandleAds, nil},
	
	{true, "libUrlDownloadToFile", MCHandleLibUrlDownloadToFile, nil},
    
    // MW-2013-10-02: [[ MobileSSLVerify ]] Added support for libUrlSetSSLVerification.
    {true, "libUrlSetSSLVerification", MCHandleLibUrlSetSSLVerification, nil},

    // PM-2014-10-07: [[ Bug 13590 ]] StartTrackingSensor and StopTrackingSensor must run on the script thread
    {true, "mobileStartTrackingSensor", MCHandleStartTrackingSensor, nil},
    {true, "mobileStopTrackingSensor", MCHandleStopTrackingSensor, nil},
    {false, "mobileSensorReading", MCHandleSensorReading, nil},
    {false, "mobileSensorAvailable", MCHandleSensorAvailable, nil},
    
    // MM-2012-02-11: Added support old style senseor syntax (iPhoneEnableAcceleromter etc)
	/* DEPRECATED */ {false, "iphoneCanTrackLocation", MCHandleCanTrackLocation, nil},

    // PM-2014-10-07: [[ Bug 13590 ]] StartTrackingLocation and StopTrackingLocation must run on the script thread
    /* DEPRECATED */ {true, "iphoneStartTrackingLocation", MCHandleLocationTrackingState, (void *)true},
    /* DEPRECATED */ {true, "iphoneStopTrackingLocation", MCHandleLocationTrackingState, (void *)false},

	/* DEPRECATED */ {false, "iphoneCurrentLocation", MCHandleCurrentLocation, nil},
    /* DEPRECATED */ {false, "mobileCanTrackLocation", MCHandleCanTrackLocation, nil},

    // PM-2014-10-07: [[ Bug 13590 ]] StartTrackingLocation and StopTrackingLocation must run on the script thread
    /* DEPRECATED */ {true, "mobileStartTrackingLocation", MCHandleLocationTrackingState, (void *)true},
    /* DEPRECATED */ {true, "mobileStopTrackingLocation", MCHandleLocationTrackingState, (void *)false},

	/* DEPRECATED */ {false, "mobileCurrentLocation", MCHandleCurrentLocation, nil},
	
	/* DEPRECATED */ {false, "iphoneCanTrackHeading", MCHandleCanTrackHeading, nil},
	/* DEPRECATED */ {false, "iphoneStartTrackingHeading", MCHandleHeadingTrackingState, (void *)true},
	/* DEPRECATED */ {false, "iphoneStopTrackingHeading", MCHandleHeadingTrackingState, (void *)false},
	/* DEPRECATED */ {false, "iphoneCurrentHeading", MCHandleCurrentHeading, nil},
	{false, "iphoneSetHeadingCalibrationTimeout", MCHandleSetHeadingCalibrationTimeout, nil},
	{false, "iphoneHeadingCalibrationTimeout", MCHandleHeadingCalibrationTimeout, nil},
    /* DEPRECATED */ {false, "mobileCanTrackHeading", MCHandleCanTrackHeading, nil},
    /* DEPRECATED */ {false, "mobileStartTrackingHeading", MCHandleHeadingTrackingState, (void *)true},
	/* DEPRECATED */ {false, "mobileStopTrackingHeading", MCHandleHeadingTrackingState, (void *)false},
	/* DEPRECATED */ {false, "mobileCurrentHeading", MCHandleCurrentHeading, nil},
    
    /* DEPRECATED */ {false, "iphoneEnableAccelerometer", MCHandleAccelerometerEnablement, (void *)true},
	/* DEPRECATED */ {false, "iphoneDisableAccelerometer", MCHandleAccelerometerEnablement, (void *)false},
	/* DEPRECATED */ {false, "mobileEnableAccelerometer", MCHandleAccelerometerEnablement, (void *)true},
	/* DEPRECATED */ {false, "mobileDisableAccelerometer", MCHandleAccelerometerEnablement, (void *)false},
    
    {false, "mobileSetLocationHistoryLimit", MCHandleSetLocationHistoryLimit, nil},
    {false, "mobileGetLocationHistoryLimit", MCHandleGetLocationHistoryLimit, nil},
    {false, "mobileGetLocationHistory", MCHandleGetLocationHistory, nil},
    
    {false, "mobileBusyIndicatorStart", MCHandleStartBusyIndicator, nil},
    {false, "mobileBusyIndicatorStop", MCHandleStopBusyIndicator, nil},
    {false, "iphoneBusyIndicatorStart", MCHandleStartBusyIndicator, nil},
    {false, "iphoneBusyIndicatorStop", MCHandleStopBusyIndicator, nil},
    
    {false, "mobileBeep", MCHandleBeep, nil},
    {true, "mobileVibrate", MCHandleVibrate, nil},
    {false, "iphoneBeep", MCHandleBeep, nil},
    {true, "iphoneVibrate", MCHandleVibrate, nil},
	
    {true, "iphoneComposeTextMessage", MCHandleComposeTextMessage, nil},
    {false, "iphoneCanComposeTextMessage", MCHandleCanComposeTextMessage, nil},
    {true, "mobileComposeTextMessage", MCHandleComposeTextMessage, nil},
    {false, "mobileCanComposeTextMessage", MCHandleCanComposeTextMessage, nil},
    
    {false, "iphoneCameraFeatures", MCHandleCameraFeatures, nil},
    {false, "mobileCameraFeatures", MCHandleCameraFeatures, nil},
	{true, "iphonePickPhoto", MCHandlePickPhoto, nil},
	{true, "mobilePickPhoto", MCHandlePickPhoto, nil},
    
	{true, "iphonePickDate", MCHandlePickDate, nil},
    {true, "mobilePickDate", MCHandlePickDate, nil},
    {true, "mobilePickTime", MCHandlePickTime, nil},
    {true, "mobilePickDateAndTime", MCHandlePickDateAndTime, nil},
    
	{true, "iphonePick", MCHandlePick, nil},
    {true, "mobilePick", MCHandlePick, nil},
    
    {true, "mobilePickMedia", MCHandlePickMedia, nil},
    {true, "iphonePickMedia", MCHandleIPhonePickMedia, nil},
    
	{true, "revMail", MCHandleRevMail, nil},
	{false, "iphoneCanSendMail", MCHandleCanSendMail, nil},
	{true, "iphoneComposeMail", MCHandleComposePlainMail, nil},
	{true, "iphoneComposeUnicodeMail", MCHandleComposeUnicodeMail, nil},
	{true, "iphoneComposeHtmlMail", MCHandleComposeHtmlMail, nil},
    {false, "mobileCanSendMail", MCHandleCanSendMail, nil},
	{true, "mobileComposeMail", MCHandleComposePlainMail, nil},
	{true, "mobileComposeUnicodeMail", MCHandleComposeUnicodeMail, nil},
	{true, "mobileComposeHtmlMail", MCHandleComposeHtmlMail, nil},
    
	{false, "iphoneDeviceOrientation", MCHandleDeviceOrientation, nil},
	{false, "iphoneOrientation", MCHandleOrientation, nil},
	{false, "iphoneAllowedOrientations", MCHandleAllowedOrientations, nil},
	{false, "iphoneSetAllowedOrientations", MCHandleSetAllowedOrientations, nil},
	{false, "iphoneOrientationLocked", MCHandleOrientationLocked, nil},
	{false, "iphoneLockOrientation", MCHandleLockOrientation, nil},
	{false, "iphoneUnlockOrientation", MCHandleUnlockOrientation, nil},
	{false, "mobileDeviceOrientation", MCHandleDeviceOrientation, nil},
	{false, "mobileOrientation", MCHandleOrientation, nil},
	{false, "mobileAllowedOrientations", MCHandleAllowedOrientations, nil},
	{false, "mobileSetAllowedOrientations", MCHandleSetAllowedOrientations, nil},
    {false, "mobileSetFullScreenRectForOrientations", MCHandleSetFullScreenRectForOrientations, nil},
    {false, "mobileLockOrientation", MCHandleLockOrientation, nil},
	{false, "mobileUnlockOrientation", MCHandleUnlockOrientation, nil},
	{false, "mobileOrientationLocked", MCHandleOrientationLocked, nil},
    
    {false, "mobileGetDeviceToken", MCHandleGetDeviceToken, nil},
    {false, "mobileGetLaunchUrl", MCHandleGetLaunchUrl, nil},
    {false, "iphoneGetDeviceToken", MCHandleGetDeviceToken, nil},
    {false, "iphoneGetLaunchUrl", MCHandleGetLaunchUrl, nil},
	
	{false, "mobileGetLaunchData", MCHandleGetLaunchData, nil},
	
	{false, "iphoneSetStatusBarStyle", MCHandleSetStatusBarStyle, nil},
	{false, "iphoneShowStatusBar", MCHandleShowStatusBar, nil},
	{false, "iphoneHideStatusBar", MCHandleHideStatusBar, nil},
    {false, "mobileSetStatusBarStyle", MCHandleSetStatusBarStyle, nil},
	{false, "mobileShowStatusBar", MCHandleShowStatusBar, nil},
	{false, "mobileHideStatusBar", MCHandleHideStatusBar, nil},
    
	{false, "iphoneSetKeyboardType", MCHandleSetKeyboardType, nil},
	{false, "iphoneSetKeyboardReturnKey", MCHandleSetKeyboardReturnKey, nil},
	{false, "mobileSetKeyboardType", MCHandleSetKeyboardType, nil},
    {false, "mobileSetKeyboardReturnKey", MCHandleSetKeyboardReturnKey, nil}, // Added from androidmisc.cpp
	
	{false, "iphoneDeviceResolution", MCHandleDeviceResolution, nil},
	{false, "iphoneUseDeviceResolution", MCHandleUseDeviceResolution, nil},
	{false, "iphoneDeviceScale", MCHandleDeviceScale, nil},
    {false, "mobileDeviceResolution", MCHandleDeviceResolution, nil},
    {false, "mobileUseDeviceResolution", MCHandleUseDeviceResolution, nil},
    {false, "mobileDeviceScale", MCHandleDeviceScale, nil},
    {false, "mobilePixelDensity", MCHandlePixelDensity, nil},

    // SN-2014-10-15: [[ Merge-6.7.0-rc-3 ]]
    {false, "iphoneLocationAuthorizationStatus", MCHandleLocationAuthorizationStatus, nil},
    {false, "mobileLocationAuthorizationStatus", MCHandleLocationAuthorizationStatus, nil},
    
	{false, "mobileBuildInfo", MCHandleBuildInfo, nil},
    {false, "androidRequestPermission", MCHandleRequestPermission, nil},
    {false, "androidPermissionExists", MCHandlePermissionExists, nil},
    {false, "androidHasPermission", MCHandleHasPermission, nil},
	
	{false, "mobileCanMakePurchase", MCHandleCanMakePurchase, nil},
	{false, "mobileEnablePurchaseUpdates", MCHandleEnablePurchaseUpdates, nil},
	{false, "mobileDisablePurchaseUpdates", MCHandleDisablePurchaseUpdates, nil},
	{false, "mobileRestorePurchases", MCHandleRestorePurchases, nil},
	{false, "mobilePurchases", MCHandlePurchaseList, nil},
	{false, "mobilePurchaseCreate", MCHandlePurchaseCreate, nil},
	{false, "mobilePurchaseState", MCHandlePurchaseState, nil},
	{false, "mobilePurchaseError", MCHandlePurchaseError, nil},
	{false, "mobilePurchaseGet", MCHandlePurchaseGet, nil},
	{false, "mobilePurchaseSet", MCHandlePurchaseSet, nil},
	{false, "mobilePurchaseSendRequest", MCHandlePurchaseSendRequest, nil},
	{false, "mobilePurchaseConfirmDelivery", MCHandlePurchaseConfirmDelivery, nil},
    {false, "mobilePurchaseVerify", MCHandlePurchaseVerify, nil},
    {false, "iphoneRequestProductDetails", MCHandleRequestProductDetails, nil},
    
    {false, "mobileStoreCanMakePurchase", MCHandleCanMakePurchase, nil},
    {false, "mobileStoreEnablePurchaseUpdates", MCHandleEnablePurchaseUpdates, nil},
    {false, "mobileStoreDisablePurchaseUpdates", MCHandleDisablePurchaseUpdates, nil},
    {false, "mobileStoreRestorePurchases", MCHandleRestorePurchases, nil},
    {false, "mobileStoreMakePurchase", MCHandleMakePurchase, nil},
    {false, "mobileStoreConfirmPurchase", MCHandleConfirmPurchase, nil},
    {false, "mobileStoreProductProperty", MCHandleGetPurchaseProperty, nil},
    {false, "mobileStoreSetProductType", MCHandleProductSetType, nil},
    {false, "mobileStoreRequestProductDetails", MCHandleRequestProductDetails, nil},
    {false, "mobileStoreConsumePurchase", MCHandleConsumePurchase, nil},
    {false, "mobileStorePurchasedProducts", MCHandleGetPurchases, nil},
    {false, "mobileStorePurchaseError", MCHandlePurchaseError, nil},	
    {false, "mobileStoreVerifyPurchase", MCHandlePurchaseVerify, nil},
	
	{false, "iphoneControlCreate", MCHandleControlCreate, nil},
	{false, "iphoneControlDelete", MCHandleControlDelete, nil},
	{false, "iphoneControlSet", MCHandleControlSet, nil},
	{false, "iphoneControlGet", MCHandleControlGet, nil},
	{true, "iphoneControlDo", MCHandleControlDo, nil},
	{false, "iphoneControlTarget", MCHandleControlTarget, nil},
	{false, "iphoneControls", MCHandleControlList, nil},
	{false, "mobileControlCreate", MCHandleControlCreate, nil},
	{false, "mobileControlDelete", MCHandleControlDelete, nil},
	{false, "mobileControlSet", MCHandleControlSet, nil},
	{false, "mobileControlGet", MCHandleControlGet, nil},
	{true, "mobileControlDo", MCHandleControlDo, nil},
	{false, "mobileControlTarget", MCHandleControlTarget, nil},
	{false, "mobileControls", MCHandleControlList, nil},
	
	{false, "iphonePreferredLanguages", MCHandlePreferredLanguages, nil},
	{false, "mobilePreferredLanguages", MCHandlePreferredLanguages, nil},
	{false, "iphoneCurrentLocale", MCHandleCurrentLocale, nil},
	{false, "mobileCurrentLocale", MCHandleCurrentLocale, nil},
	
	{false, "iphoneApplicationIdentifier", MCHandleApplicationIdentifier, nil},
	{false, "iphoneSystemIdentifier", MCHandleSystemIdentifier, nil},

    // MM-2013-05-21: [[ Bug 10895 ]] Added iphoneIdentifierForVendor as an initial replacement for iphoneSystemIdentifier.
    {false, "mobileIdentifierForVendor", MCHandleIdentifierForVendor, nil},
    {false, "iphoneIdentifierForVendor", MCHandleIdentifierForVendor, nil},
    
	{false, "iphoneSetReachabilityTarget", MCHandleSetReachabilityTarget, nil},
	{false, "iphoneReachabilityTarget", MCHandleReachabilityTarget, nil},
    
    // MM-2012-09-02: Add support for mobile* multi channel sound syntax
    {false, "mobilePlaySoundOnChannel", MCHandlePlaySoundOnChannel, nil},
	{false, "mobilePausePlayingOnChannel", MCHandlePausePlayingOnChannel},
	{false, "mobileResumePlayingOnChannel", MCHandleResumePlayingOnChannel},
	{false, "mobileStopPlayingOnChannel", MCHandleStopPlayingOnChannel, nil},
	{false, "mobileDeleteSoundChannel", MCHandleDeleteSoundChannel, nil},
	{false, "mobileSetSoundChannelVolume", MCHandleSetSoundChannelVolume, nil},
	{false, "mobileSoundChannelVolume", MCHandleSoundChannelVolume, nil},
	{false, "mobileSoundChannelStatus", MCHandleSoundChannelStatus, nil},
	{false, "mobileSoundOnChannel", MCHandleSoundOnChannel, nil},
	{false, "mobileNextSoundOnChannel", MCHandleNextSoundOnChannel, nil},
	{false, "mobileSoundChannels", MCHandleSoundChannels, nil},
	{false, "iphonePlaySoundOnChannel", MCHandlePlaySoundOnChannel, nil},
	{false, "iphonePausePlayingOnChannel", MCHandlePausePlayingOnChannel},
	{false, "iphoneResumePlayingOnChannel", MCHandleResumePlayingOnChannel},
	{false, "iphoneStopPlayingOnChannel", MCHandleStopPlayingOnChannel, nil},
	{false, "iphoneDeleteSoundChannel", MCHandleDeleteSoundChannel, nil},
	{false, "iphoneSetSoundChannelVolume", MCHandleSetSoundChannelVolume, nil},
	{false, "iphoneSoundChannelVolume", MCHandleSoundChannelVolume, nil},
	{false, "iphoneSoundChannelStatus", MCHandleSoundChannelStatus, nil},
	{false, "iphoneSoundOnChannel", MCHandleSoundOnChannel, nil},
	{false, "iphoneNextSoundOnChannel", MCHandleNextSoundOnChannel, nil},
	{false, "iphoneSoundChannels", MCHandleSoundChannels, nil},
    // MM-2012-09-07: Added support for setting the category of the current audio session (how mute button is handled etc.
    {false, "iphoneSetAudioCategory", MCHandleSetAudioCategory, nil},
    {false, "mobileSetAudioCategory", MCHandleSetAudioCategory, nil},
    
    {false, "iphoneSetDoNotBackupFile", MCHandleFileSetDoNotBackup, nil},
    {false, "iphoneDoNotBackupFile", MCHandleFileGetDoNotBackup, nil},
    {false, "iphoneSetFileDataProtection", MCHandleFileSetDataProtection, nil},
    {false, "iphoneFileDataProtection", MCHandleFileGetDataProtection, nil},
	
	{false, "iphoneLockIdleTimer", MCHandleLockIdleTimer, nil},
	{false, "mobileLockIdleTimer", MCHandleLockIdleTimer, nil},
	{false, "iphoneUnlockIdleTimer", MCHandleUnlockIdleTimer, nil},
	{false, "mobileUnlockIdleTimer", MCHandleUnlockIdleTimer, nil},
	{false, "iphoneIdleTimerLocked", MCHandleIdleTimerLocked, nil},
	{false, "mobileIdleTimerLocked", MCHandleIdleTimerLocked, nil},
    
    {false, "mobileCreateLocalNotification", MCHandleCreateLocalNotification, nil},
    {false, "mobileGetRegisteredNotifications", MCHandleGetRegisteredNotifications, nil},
    {false, "mobileGetNotificationDetails", MCHandleGetNotificationDetails, nil},
    {false, "mobileCancelLocalNotification", MCHandleCancelLocalNotification, nil},
    {false, "mobileCancelAllLocalNotifications", MCHandleCancelAllLocalNotifications, nil},
    {false, "iphoneCreateLocalNotification", MCHandleCreateLocalNotification, nil},
    {false, "iphoneGetRegisteredNotifications", MCHandleGetRegisteredNotifications, nil},
    {false, "iphoneCancelLocalNotification", MCHandleCancelLocalNotification, nil},
    {false, "iphoneCancelAllLocalNotifications", MCHandleCancelAllLocalNotifications, nil},
    
    {false, "iphoneGetNotificationBadgeValue", MCHandleGetNotificationBadgeValue, nil},
    {false, "iphoneSetNotificationBadgeValue", MCHandleSetNotificationBadgeValue, nil},
    
	{false, "iphoneActivityIndicatorStart", MCHandleStartActivityIndicator, nil},
	{false, "iphoneActivityIndicatorStop", MCHandleStopActivityIndicator, nil},
	
	{true, "iphoneExportImageToAlbum", MCHandleExportImageToAlbum, nil},
	{true, "mobileExportImageToAlbum", MCHandleExportImageToAlbum, nil},	
    
	{false, "iphoneSetRedrawInterval", MCHandleSetRedrawInterval, nil},
	
    // MW-2012-02-15: [[ Bug 9985 ]] Control whether the autorotation animation happens
    //   or not.
	{false, "iphoneSetAnimateAutorotation", MCHandleSetAnimateAutorotation, nil}, 
    
    {true, "mobilePickContact", MCHandlePickContact, nil},       // ABPeoplePickerNavigationController
    {true, "mobileShowContact", MCHandleShowContact, nil},       // ABPersonViewController
    {true, "mobileGetContactData", MCHandleGetContactData, nil}, // ABNewPersonViewController
    {true, "mobileUpdateContact", MCHandleUpdateContact, nil},   // ABUnknownPersonViewController
    {true, "mobileCreateContact", MCHandleCreateContact, nil},

    // PM-2014-10-08: [[ Bug 13621 ]] Add/Find/Remove contact must run on the script thread
    {true, "mobileAddContact", MCHandleAddContact, nil},
    {true, "mobileFindContact", MCHandleFindContact, nil},
    {true, "mobileRemoveContact", MCHandleRemoveContact, nil},
    
    {true, "mobileShowEvent", MCHandleShowEvent, nil},                     // ???                      // UI
    {false, "mobileGetEventData", MCHandleGetEventData, nil},               // get calendar data for
    {true, "mobileCreateEvent", MCHandleCreateEvent, nil},                 // create event in calendar // UI
    {true, "mobileUpdateEvent", MCHandleUpdateEvent, nil},                 // edit calendar event      // UI
    {false, "mobileAddEvent", MCHandleAddEvent, nil},                       // create calendar entry
    {false, "mobileGetCalendarsEvent", MCHandleGetCalendarsEvent, nil}, // create reoccurring calendar entry
    {false, "mobileFindEvent", MCHandleFindEvent, nil},                     // get calendar entry
    {false, "mobileRemoveEvent", MCHandleRemoveEvent, nil},
	
	{false, "iphoneClearTouches", MCHandleClearTouches, nil},
	{false, "mobileClearTouches", MCHandleClearTouches, nil},
    
    // MW-2013-05-30: [[ RemoteControl ]] Support for iOS 'remote controls' and metadata display.
    {false, "iphoneEnableRemoteControl", MCHandleEnableRemoteControl, nil},
    {false, "iphoneDisableRemoteControl", MCHandleDisableRemoteControl, nil},
    {false, "iphoneRemoteControlEnabled", MCHandleRemoteControlEnabled, nil},
    {false, "iphoneSetRemoteControlDisplay", MCHandleSetRemoteControlDisplay, nil},
	
	{false, "mobileIsNFCAvailable", MCHandleIsNFCAvailable, nil},
	{false, "mobileIsNFCEnabled", MCHandleIsNFCEnabled, nil},
	{false, "mobileEnableNFCDispatch", MCHandleEnableNFCDispatch, nil},
	{false, "mobileDisableNFCDispatch", MCHandleDisableNFCDispatch, nil},
    
    {false, "mobileSetKeyboardDisplay", MCHandleSetKeyboardDisplay, nil},
    {false, "mobileGetKeyboardDisplay", MCHandleGetKeyboardDisplay, nil},
    
	{nil, nil, nil}    
};

bool MCIsPlatformMessage(MCNameRef handler_name)
{
    bool found = false;
    
    for(uint32_t i = 0; s_platform_messages[i] . message != nil; i++)
    {
		if (MCStringIsEqualToCString(MCNameGetString(handler_name), s_platform_messages[i].message, kMCCompareCaseless))
			found = true;
    }
    
    return found;
}

#ifdef TARGET_SUBPLATFORM_IPHONE
struct handle_context_t
{
	MCPlatformMessageHandler handler;
	void *context;
	MCParameter *parameters;
	Exec_stat result;
};

static void invoke_platform(void *p_context)
{
	handle_context_t *ctxt;
	ctxt = (handle_context_t *)p_context;
	ctxt -> result = ctxt -> handler(ctxt -> context, ctxt -> parameters);
}

extern void MCIPhoneCallOnMainFiber(void (*)(void *), void *);

bool MCDoHandlePlatformMessage(bool p_waitable, MCPlatformMessageHandler p_handler, void *p_context, MCParameter *p_parameters, Exec_stat& r_result)
{
    // MW-2012-07-31: [[ Fibers ]] If the method doesn't need script / wait, then
    //   jump to the main fiber for it.
    if (!p_waitable)
    {
        handle_context_t ctxt;
        ctxt . handler = p_handler;
        ctxt . context = p_context;
        ctxt . parameters = p_parameters;
        MCIPhoneCallOnMainFiber(invoke_platform, &ctxt);
        r_result = ctxt . result;
        return true;
    }
    
    // Execute the method as normal, in this case the method will have to jump
    // to the main fiber to do any system stuff.
    r_result = p_handler(p_context, p_parameters);
    return true;
    
}
#else // Android
bool MCDoHandlePlatformMessage(bool p_waitable, MCPlatformMessageHandler p_handler, void *p_context, MCParameter *p_parameters, Exec_stat& r_result)
{
    r_result = p_handler(p_context, p_parameters);
    return true;
}
#endif

bool MCHandlePlatformMessage(MCNameRef p_message, MCParameter *p_parameters, Exec_stat& r_result)
{
	for(uint32_t i = 0; s_platform_messages[i] . message != nil; i++)
		if (MCStringIsEqualToCString(MCNameGetString(p_message), s_platform_messages[i] . message, kMCCompareCaseless))
		{
            return MCDoHandlePlatformMessage(s_platform_messages[i] . waitable, s_platform_messages[i] . handler, s_platform_messages[i] . context, p_parameters, r_result);
		}
	
    r_result = ES_NOT_HANDLED;
	return false;
}
