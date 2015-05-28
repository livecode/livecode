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

#include "param.h"
#include "util.h"
#include "stack.h"

#include "exec.h"

#include "platform.h"

////////////////////////////////////////////////////////////////////////////////

static MCExecSetTypeElementInfo _kMCPlatformCameraDeviceElementInfo[] =
{
	{ "default", kMCPlatformCameraDeviceDefaultBit },
	{ "front", kMCPlatformCameraDeviceFrontBit },
	{ "back", kMCPlatformCameraDeviceBackBit },
};

static MCExecSetTypeInfo _kMCPlatformCameraDeviceTypeInfo =
{
	"Platform.CameraDevice",
	sizeof(_kMCPlatformCameraDeviceElementInfo) / sizeof(MCExecSetTypeElementInfo),
	_kMCPlatformCameraDeviceElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCPlatformCameraFlashModeElementInfo[] =
{
	{"off", kMCPlatformCameraFlashModeOff},
	{"on", kMCPlatformCameraFlashModeOn},
	{"auto", kMCPlatformCameraFlashModeAuto},
};

static MCExecEnumTypeInfo _kMCPlatformCameraFlashModeTypeInfo =
{
	"Platform.CameraFlashMode",
	sizeof(_kMCPlatformCameraFlashModeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCPlatformCameraFlashModeElementInfo
};

//////////

static MCExecSetTypeElementInfo _kMCPlatformCameraFeatureElementInfo[] =
{
	{ "flash", kMCPlatformCameraFeatureFlashBit },
	{ "flashmode", kMCPlatformCameraFeatureFlashModeBit },
};

static MCExecSetTypeInfo _kMCPlatformCameraFeatureTypeInfo =
{
	"Platform.CameraFeature",
	sizeof(_kMCPlatformCameraFeatureElementInfo) / sizeof(MCExecSetTypeElementInfo),
	_kMCPlatformCameraFeatureElementInfo
};

////////////////////////////////////////////////////////////////////////////////

MCExecSetTypeInfo *kMCPlatformCameraDeviceTypeInfo = &_kMCPlatformCameraDeviceTypeInfo;
MCExecEnumTypeInfo *kMCPlatformCameraFlashModeTypeInfo = &_kMCPlatformCameraFlashModeTypeInfo;
MCExecSetTypeInfo *kMCPlatformCameraFeatureTypeInfo = &_kMCPlatformCameraFeatureTypeInfo;

////////////////////////////////////////////////////////////////////////////////

struct MCCameraControl
{
    MCCameraControl *next;
    MCPlatformCameraRef camera;
    MCNameRef name;
    MCStack *owner;
};

struct MCCameraControlProp
{
    const char *name;
    MCPlatformCameraProperty property;
    MCPlatformPropertyType type;
};

union MCCameraControlPropValue
{
    MCRectangle rectangle;
    bool cbool;
	intset_t set;
	intenum_t enumeration;
};

static MCCameraControlProp s_camera_control_props[] =
{
    { "rectangle", kMCPlatformCameraPropertyRectangle, kMCPlatformPropertyTypeRectangle },
    { "rect", kMCPlatformCameraPropertyRectangle, kMCPlatformPropertyTypeRectangle },
    { "visible", kMCPlatformCameraPropertyVisible, kMCPlatformPropertyTypeBool },
	
	{ "devices", kMCPlatformCameraPropertyDevices, kMCPlatformPropertyTypeCameraDevice },
	{ "device", kMCPlatformCameraPropertyDevice, kMCPlatformPropertyTypeCameraDevice },
	{ "features", kMCPlatformCameraPropertyFeatures, kMCPlatformPropertyTypeCameraFeature },
	{ "flashmode", kMCPlatformCameraPropertyFlashMode, kMCPlatformPropertyTypeCameraFlashMode },
	{ "isflashactive", kMCPlatformCameraPropertyIsFlashActive, kMCPlatformPropertyTypeBool },
	{ "isflashavailable", kMCPlatformCameraPropertyIsFlashAvailable, kMCPlatformPropertyTypeBool },
    { nil },
};

struct MCCameraControlAction
{
	const char *name;
	MCPlatformCameraAction action;
};

static MCCameraControlAction s_camera_control_actions[] =
{
	{ "takepicture", kMCPlatformCameraActionTakePicture },
	{ "startrecording", kMCPlatformCameraActionStartRecording },
	{ "stoprecording", kMCPlatformCameraActionStopRecording },
	{ nil },
};

static MCCameraControl *s_camera_controls = nil;

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...)
{
    MCExecContext ctxt(nil, nil, nil);
	
	bool t_success;
	t_success = true;
	
	bool t_now_optional;
	t_now_optional = false;
	
	va_list t_args;
	va_start(t_args, p_format);
	
	while(*p_format != '\0' && t_success)
	{
		if (*p_format == '|')
		{
			t_now_optional = true;
			p_format++;
			continue;
		}
		
        MCAutoValueRef t_value;
		if (p_parameters != nil)
        {
            // AL-2014-05-28: [[ Bug 12477 ]] Use eval_argument here otherwise variable references do not get resolved
			t_success = p_parameters -> eval_argument(ctxt, &t_value);
        }
		else if (t_now_optional)
			break;
		else
			t_success = false;
		
		switch(*p_format)
		{
			case 'b':
				if (t_success)
                {
                    MCAutoStringRef t_string;
                    /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
					*(va_arg(t_args, bool *)) = MCStringIsEqualTo(*t_string, kMCTrueString, kMCCompareCaseless);
                }
				break;
				
			case 's':
				if (t_success)
                {
                    MCAutoStringRef t_string;
                    /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
                    char *temp;
                    /* UNCHECKED */ MCStringConvertToCString(*t_string, temp);
					*(va_arg(t_args, char **)) = temp;
                }
				else
					*(va_arg(t_args, char **)) = nil;
				break;
                
			case 'd':
				if (t_success)
                {
                    MCAutoStringRef t_string;
                    /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
                    char *temp;
                    /* UNCHECKED */ MCStringConvertToCString(*t_string, temp);
					(va_arg(t_args, MCString *)) -> set(temp, strlen(temp));
                }
				else
					(va_arg(t_args, MCString *)) -> set(nil, 0);
				break;
                
            case 'x':
            {
				if (t_success)
                {
                    /* UNCHECKED */ ctxt . ConvertToString(*t_value, *(va_arg(t_args, MCStringRef *)));
                }
				else
					t_success = false;
				break;
            }
                
            case 'n':
            {
				if (t_success)
                {
                    /* UNCHECKED */ ctxt . ConvertToName(*t_value, *(va_arg(t_args, MCNameRef *)));
                }
				else
					t_success = false;
				break;
            }
			
            case 'a':
            {
				if (t_success)
                /* UNCHECKED */ ctxt . ConvertToArray(*t_value, *(va_arg(t_args, MCArrayRef *)));
				else
					t_success = false;
				break;
            }
				
			case 'r':
			{
				int2 i1, i2, i3, i4;
				if (t_success)
                {
                    MCAutoStringRef t_string;
                    ctxt . ConvertToString(*t_value, &t_string);
					t_success = MCU_stoi2x4(*t_string, i1, i2, i3, i4) == True;
                }
				if (t_success)
					MCU_set_rect(*(va_arg(t_args, MCRectangle *)), i1, i2, i3 - i1, i4 - i2);
			}
				break;
				
			case 'i':
				if (t_success)
				{
                    integer_t t_int;
					if (ctxt . ConvertToInteger(*t_value, t_int))
						*(va_arg(t_args, integer_t *)) = t_int;
					else
						t_success = false;
				}
				break;
				
			case 'u':
				if (t_success)
				{
                    uinteger_t t_uint;
					if (ctxt . ConvertToUnsignedInteger(*t_value, t_uint))
						*(va_arg(t_args, uinteger_t *)) = t_uint;
					else
						t_success = false;
				}
				break;
                
            case 'v':
                if (t_success)
                {
                    *(va_arg(t_args, MCValueRef *)) = MCValueRetain(*t_value);
                }
                break;
		};
		
		p_format += 1;
		
		if (p_parameters != nil)
			p_parameters = p_parameters -> getnext();
	}
	
	va_end(t_args);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static MCCameraControl *find_camera_control(MCNameRef p_name, MCCameraControl **r_previous = nil)
{
    MCCameraControl *t_previous, *t_control;
    for(t_previous = nil, t_control = s_camera_controls; t_control != nil; t_control = t_control -> next)
    {
        if (MCNameIsEqualTo(p_name, t_control -> name))
            break;
        t_previous = t_control;
    }
    
    if (r_previous != nil && t_control != nil)
        *r_previous = t_previous;
    
    return t_control;
}

static MCCameraControlProp *find_camera_control_prop(MCNameRef p_prop_name)
{
    for(uindex_t i = 0; s_camera_control_props[i] . name != nil; i++)
        if (MCNameIsEqualToCString(p_prop_name, s_camera_control_props[i] . name, kMCStringOptionCompareCaseless))
            return &s_camera_control_props[i];
    
    return nil;
}

static MCCameraControlAction *find_camera_control_action(MCNameRef p_action_name)
{
	for (uindex_t i = 0; s_camera_control_actions[i].name != nil; i++)
		if (MCNameIsEqualToCString(p_action_name, s_camera_control_actions[i].name, kMCStringOptionCompareCaseless))
			return &s_camera_control_actions[i];
	
	return nil;
}

static bool convert_to_platform_prop_type(MCExecContext& ctxt, MCValueRef p_value, MCPlatformPropertyType p_type, MCCameraControlPropValue& r_prop_value)
{
    switch(p_type)
    {
        case kMCPlatformPropertyTypeBool:
            return ctxt . ConvertToBool(p_value, r_prop_value . cbool);
        case kMCPlatformPropertyTypeRectangle:
            return ctxt . ConvertToLegacyRectangle(p_value, r_prop_value . rectangle);
		case kMCPlatformPropertyTypeCameraDevice:
		{
			MCExecValue t_val;
			MCExecTypeSetValueRef(t_val, MCValueRetain(p_value));
			
			/* UNCHECKED */
			MCExecParseSet(ctxt, kMCPlatformCameraDeviceTypeInfo, t_val, r_prop_value.set);
			
			return true;
		}
		case kMCPlatformPropertyTypeCameraFlashMode:
		{
			MCExecValue t_val;
			MCExecTypeSetValueRef(t_val, MCValueRetain(p_value));
			
			/* UNCHECKED */
			MCExecParseEnum(ctxt, kMCPlatformCameraFlashModeTypeInfo, t_val, r_prop_value.enumeration);
			
			return true;
		}
		case kMCPlatformPropertyTypeCameraFeature:
		{
			MCExecValue t_val;
			MCExecTypeSetValueRef(t_val, MCValueRetain(p_value));
			
			/* UNCHECKED */
			MCExecParseSet(ctxt, kMCPlatformCameraFeatureTypeInfo, t_val, r_prop_value.set);
			
			return true;
		}
        default:
            break;
    }
                
    return false;
}

static bool convert_from_platform_prop_type(MCExecContext& ctxt, MCCameraControlPropValue& p_prop_value, MCPlatformPropertyType p_type, MCValueRef& r_value)
{
    switch(p_type)
    {
        case kMCPlatformPropertyTypeBool:
            r_value = MCValueRetain(p_prop_value . cbool ? kMCTrue : kMCFalse);
            return true;
        case kMCPlatformPropertyTypeRectangle:
            return MCStringFormat((MCStringRef&)r_value, "%d,%d,%d,%d", p_prop_value . rectangle . x, p_prop_value . rectangle . y, p_prop_value . rectangle . x + p_prop_value . rectangle . width, p_prop_value . rectangle . y + p_prop_value . rectangle . height);
		case kMCPlatformPropertyTypeCameraDevice:
		{
			MCExecValue t_val;
			/* UNCHECKED */
			MCExecFormatSet(ctxt, kMCPlatformCameraDeviceTypeInfo, p_prop_value.set, t_val);
			MCExecTypeConvertToValueRefAndReleaseAlways(ctxt, t_val.type, &t_val, r_value);
			/* UNCHECKED */
			return true;
		}
		case kMCPlatformPropertyTypeCameraFlashMode:
		{
			MCExecValue t_val;
			/* UNCHECKED */
			MCExecFormatEnum(ctxt, kMCPlatformCameraFlashModeTypeInfo, p_prop_value.enumeration, t_val);
			MCExecTypeConvertToValueRefAndReleaseAlways(ctxt, t_val.type, &t_val, r_value);
			/* UNCHECKED */
			return true;
		}
		case kMCPlatformPropertyTypeCameraFeature:
		{
			MCExecValue t_val;
			/* UNCHECKED */
			MCExecFormatSet(ctxt, kMCPlatformCameraFeatureTypeInfo, p_prop_value.set, t_val);
			MCExecTypeConvertToValueRefAndReleaseAlways(ctxt, t_val.type, &t_val, r_value);
			/* UNCHECKED */
			return true;
		}
        default:
            break;
    }
    
    return false;
}

static void camera_control_attachment_callback(void *p_context, MCStack *p_stack, MCStackAttachmentEvent p_event)
{
    MCCameraControl *t_control;
    t_control = (MCCameraControl *)p_context;
    
    switch(p_event)
    {
        case kMCStackAttachmentEventDeleting:
            MCPlatformCameraDetach(t_control -> camera);
            t_control -> owner = nil;
            break;
            
        case kMCStackAttachmentEventRealizing:
            MCPlatformCameraAttach(t_control -> camera, p_stack -> getwindowalways());
            break;
            
        case kMCStackAttachmentEventUnrealizing:
            MCPlatformCameraDetach(t_control -> camera);
            break;
            
        case kMCStackAttachmentEventToolChanged:
        {
            bool t_active;
            t_active = p_stack -> gettool(p_stack) == T_BROWSE;
            MCPlatformCameraSetProperty(t_control -> camera, kMCPlatformCameraPropertyActive, kMCPlatformPropertyTypeBool, &t_active);
        }
        break;
            
        default:
            break;
    }
}

Exec_stat MCHandleCameraControlCreate(void *p_context, MCParameter *p_parameters)
{
    bool t_success;
    t_success = true;
    
	MCNewAutoNameRef t_control_name;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "n", &(&t_control_name));
    
    if (*t_control_name == nil)
        t_control_name = kMCEmptyName;
    
    // If a control with the given name exists, do nothing.
    if (find_camera_control(*t_control_name) != nil)
        return ES_NORMAL;
    
    MCCameraControl *t_control;
    if (!MCMemoryNew(t_control))
        return ES_ERROR;
    
    MCPlatformCameraRef t_camera;
    t_control -> next = s_camera_controls;
    t_control -> name = MCValueRetain(*t_control_name);
    t_control -> owner = MCdefaultstackptr;
    MCPlatformCameraCreate(t_control -> camera);
    t_control -> owner -> attach(t_control, camera_control_attachment_callback);
    s_camera_controls = t_control;
    
    bool t_active;
    t_active = t_control -> owner -> gettool(t_control -> owner) == T_BROWSE;
    MCPlatformCameraSetProperty(t_control -> camera, kMCPlatformCameraPropertyActive, kMCPlatformPropertyTypeBool, &t_active);
    
    MCPlatformCameraOpen(t_control -> camera);
    
    return ES_NORMAL;
}

Exec_stat MCHandleCameraControlDelete(void *p_context, MCParameter *p_parameters)
{
    bool t_success;
    t_success = true;
    
	MCNewAutoNameRef t_control_name;
	if (t_success)
        t_success = MCParseParameters(p_parameters, "n", &(&t_control_name));
    
    MCCameraControl *t_control, *t_previous;
    t_control = find_camera_control(*t_control_name, &t_previous);
    
    // Do nothing if control not found.
    if (t_control == nil)
        return ES_NORMAL;
    
    MCPlatformCameraClose(t_control -> camera);
    
    if (t_previous != nil)
        t_previous -> next = t_control -> next;
    else
        s_camera_controls = t_control -> next;
    
    if (t_control -> owner != nil)
    {
        t_control -> owner -> detach(t_control, camera_control_attachment_callback);
        MCPlatformCameraDetach(t_control -> camera);
    }
    
    MCPlatformCameraRelease(t_control -> camera);
    MCValueRelease(t_control -> name);
    MCMemoryDelete(t_control);
    
    return ES_NORMAL;
}

Exec_stat MCHandleCameraControlSet(void *p_context, MCParameter *p_parameters)
{
    MCNewAutoNameRef t_control_name;
    MCNewAutoNameRef t_property;
    MCAutoValueRef t_value;
    if (!MCParseParameters(p_parameters, "nnv", &(&t_control_name), &(&t_property), &(&t_value)))
        return ES_ERROR;
    
    MCCameraControl *t_control;
    t_control = find_camera_control(*t_control_name);
    
    // Do nothing if control not found.
    if (t_control == nil)
        return ES_NORMAL;
    
    MCCameraControlProp *t_prop;
    t_prop = find_camera_control_prop(*t_property);
    
    // Do nothing if property not found
    if (t_prop == nil)
        return ES_NORMAL;
    
    MCExecContext ctxt(nil, nil, nil);
    MCCameraControlPropValue t_prop_value;
    if (!convert_to_platform_prop_type(ctxt, *t_value, t_prop -> type, t_prop_value))
        return ES_ERROR;
    
    if (!MCPlatformCameraSetProperty(t_control -> camera, t_prop -> property, t_prop -> type, &t_prop_value))
        return ES_ERROR;
    
    return ES_NORMAL;
}

Exec_stat MCHandleCameraControlGet(void *p_context, MCParameter *p_parameters)
{
    MCNewAutoNameRef t_control_name;
    MCNewAutoNameRef t_property;
    if (!MCParseParameters(p_parameters, "nn", &(&t_control_name), &(&t_property)))
        return ES_ERROR;
    
    MCCameraControl *t_control;
    t_control = find_camera_control(*t_control_name);
    
    // Do nothing if control not found.
    if (t_control == nil)
        return ES_NORMAL;
    
    MCCameraControlProp *t_prop;
    t_prop = find_camera_control_prop(*t_property);
    
    // Do nothing if property not found
    if (t_prop == nil)
        return ES_NORMAL;
    
    MCCameraControlPropValue t_prop_value;
    if (!MCPlatformCameraGetProperty(t_control -> camera, t_prop -> property, t_prop -> type, &t_prop_value))
        return ES_ERROR;
    
    MCExecContext ctxt(nil, nil, nil);
    MCAutoValueRef t_value;
    if (!convert_from_platform_prop_type(ctxt, t_prop_value, t_prop -> type, &t_value))
        return ES_ERROR;
    
    ctxt . SetTheResultToValue(*t_value);
    
    return ES_NORMAL;
}

Exec_stat MCHandleCameraControlDo(void *p_context, MCParameter *p_parameters)
{
    MCNewAutoNameRef t_control_name;
    MCNewAutoNameRef t_action_name;
    if (!MCParseParameters(p_parameters, "nn", &(&t_control_name), &(&t_action_name)))
        return ES_ERROR;
    
    MCExecContext ctxt(nil, nil, nil);
    MCAutoValueRefArray t_params;
    while(p_parameters != nil)
    {
        MCAutoValueRef t_value;
        
        if (p_parameters -> eval(ctxt, &t_value) != ES_NORMAL)
            return ES_ERROR;
        
        if (!t_params . Push(*t_value))
            return ES_ERROR;
        
        p_parameters = p_parameters -> getnext();
    }
    
	MCCameraControl *t_control;
	t_control = find_camera_control(*t_control_name);
	
	// Do nothing if control not found.
	if (t_control == nil)
		return ES_NORMAL;
	
	MCCameraControlAction *t_action;
	t_action = find_camera_control_action(*t_action_name);
	
	if (t_action == nil)
		return ES_NORMAL; // TODO - throw error?
	
	// Do it.
	switch (t_action->action)
	{
		case kMCPlatformCameraActionTakePicture:
		{
			MCAutoDataRef t_data;
			if (!MCPlatformCameraTakePicture(t_control->camera, &t_data))
				return ES_ERROR; // TODO - throw error
			
			ctxt.SetTheResultToValue(*t_data);
			return ES_NORMAL;
		}
		
		case kMCPlatformCameraActionStartRecording:
		{
			MCAutoStringRef t_string;
			if (t_params.Count() < 1 || !ctxt.ConvertToString(t_params[0], &t_string))
				return ES_ERROR; // TODO - throw error
			
			if (!MCPlatformCameraStartRecording(t_control->camera, *t_string))
				return ES_ERROR; // TODO - throw error
			
			return ES_NORMAL;
		}
		
		case kMCPlatformCameraActionStopRecording:
		{
			if (!MCPlatformCameraStopRecording(t_control->camera))
				return ES_ERROR; // TODO - throw error
			
			return ES_NORMAL;
		}
	}
	
    return ES_NORMAL;
}

Exec_stat MCHandleCameraControlTarget(void *p_context, MCParameter *p_parameters)
{
    return ES_NORMAL;
}

Exec_stat MCHandleCameraControls(void *p_context, MCParameter *p_parameters)
{
	MCAutoListRef t_list;
    
    if (!MCListCreateMutable('\n', &t_list))
        return ES_ERROR;
    
    for(MCCameraControl *t_control = s_camera_controls; t_control != nil; t_control = t_control -> next)
        if (!MCListAppend(*t_list, t_control -> name))
            return ES_ERROR;
    
    MCAutoStringRef t_result;
    if (!MCListCopyAsString(*t_list, &t_result))
        return ES_ERROR;
    
	MCExecContext ctxt(nil, nil, nil);
    ctxt . SetTheResultToValue(*t_result);
    
    return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

typedef Exec_stat (*MCPlatformMessageHandler)(void *context, MCParameter *parameters);

struct MCPlatformMessageSpec
{
	const char *message;
	MCPlatformMessageHandler handler;
	void *context;
};

static MCPlatformMessageSpec s_platform_messages[] =
{
    {"cameraControlCreate", MCHandleCameraControlCreate, nil},
    {"cameraControlDelete", MCHandleCameraControlDelete, nil},
    {"cameraControlSet", MCHandleCameraControlSet, nil},
    {"cameraControlGet", MCHandleCameraControlGet, nil},
    {"cameraControlDo", MCHandleCameraControlDo, nil},
    {"cameraControlTarget", MCHandleCameraControlTarget, nil},
    {"cameraControls", MCHandleCameraControls, nil},
	{nil, nil, nil}
};

bool MCIsPlatformMessage(MCNameRef handler_name)
{
    bool found = false;
    
    for(uint32_t i = 0; s_platform_messages[i] . message != nil; i++)
    {
        const char* t_message = s_platform_messages[i].message;
		if (MCNameIsEqualToCString(handler_name, s_platform_messages[i].message, kMCCompareCaseless))
			found = true;
    }
    
    return found;
}

Exec_stat MCHandlePlatformMessage(MCNameRef p_message, MCParameter *p_parameters)
{
	for(uint32_t i = 0; s_platform_messages[i] . message != nil; i++)
		if (MCNameIsEqualToCString(p_message, s_platform_messages[i] . message, kMCCompareCaseless))
			return s_platform_messages[i] . handler(s_platform_messages[i] . context, p_parameters);
    return ES_NOT_HANDLED;
}

////////////////////////////////////////////////////////////////////////////////
