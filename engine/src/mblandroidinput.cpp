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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"
#include "execpt.h"
#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"
#include "osspec.h"


#include <jni.h>
#include "mblandroidjava.h"
#include "mblandroidcontrol.h"
#include "mblandroidutil.h"

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    kMCInputCapitalizeNone = 0,
    kMCInputCapitalizeCharacters = 0x1000,
    kMCInputCapitalizeWords = 0x2000,
    kMCInputCapitalizeSentences = 0x4000,
} MCInputCapitalizationType;

static MCNativeControlEnumEntry s_autocapitalizationtype_enum[] = 
{
    {"none", kMCInputCapitalizeNone},
    {"words", kMCInputCapitalizeWords},
    {"sentences", kMCInputCapitalizeSentences},
    {"all characters", kMCInputCapitalizeCharacters},
    {nil, 0},
};

typedef enum
{
    kMCInputAutocorrectionNo,
    kMCInputAutocorrectionYes,
    kMCInputAutocorrectionDefault = kMCInputAutocorrectionYes,
} MCInputAutocorrectionType;

static MCNativeControlEnumEntry s_autocorrectiontype_enum[] = 
{
    {"default", kMCInputAutocorrectionDefault},
    {"no", kMCInputAutocorrectionNo},
    {"yes", kMCInputAutocorrectionYes},
    {nil, 0},
};

typedef enum
{
    kMCInputKeyboardTypeAlphabet = 0x1, // TYPE_CLASS_TEXT
    kMCInputKeyboardTypeNumeric = (0x2 | 0x1000 | 0x2000), // TYPE_CLASS_NUMBER | TYPE_NUMBER_FLAG_SIGNED | TYPE_NUMBER_FLAG_DECIMAL
    kMCInputKeyboardTypeURL = (0x1 | 0x10), // TYPE_CLASS_TEXT | TYPE_TEXT_VARIATION_URI
    kMCInputKeyboardTypeNumber = 0x2, // TYPE_CLASS_NUMBER
    kMCInputKeyboardTypePhone = 0x3, // TYPE_CLASS_PHONE
    kMCInputKeyboardTypeContact = (0x1 | 0x60), // TYPE_CLASS_TEXT | TYPE_TEXT_VARIATION_PERSON_NAME
    kMCInputKeyboardTypeEmail = (0x1 | 0x20), // TYPE_CLASS_TEXT | TYPE_TEXT_VARIATION_EMAIL_ADDRESS
    kMCInputKeyboardTypeDecimal = kMCInputKeyboardTypeNumeric,
    kMCInputKeyboardTypeDefault = kMCInputKeyboardTypeAlphabet,
} MCInputKeyboardType;

static MCNativeControlEnumEntry s_keyboard_type_enum[] =
{
    { "default", kMCInputKeyboardTypeDefault},
    { "alphabet", kMCInputKeyboardTypeAlphabet},
    { "numeric", kMCInputKeyboardTypeNumeric},
    { "url", kMCInputKeyboardTypeURL},
    { "number", kMCInputKeyboardTypeNumber},
    { "phone", kMCInputKeyboardTypePhone},
    { "contact", kMCInputKeyboardTypeContact},
    { "email", kMCInputKeyboardTypeEmail},
    { "decimal", kMCInputKeyboardTypeDecimal},
    {nil, 0},
};

typedef enum
{
    kMCInputReturnKeyTypeDefault = 0x0, // IME_ACTION_UNSPECIFIED
    kMCInputReturnKeyTypeGo = 0x2, // IME_ACTION_GO
    kMCInputReturnKeyTypeNext = 0x5, // IME_ACTION_NEXT
    kMCInputReturnKeyTypeSearch = 0x3, // IME_ACTION_SEARCH
    kMCInputReturnKeyTypeSend = 0x4, // IME_ACTION_SEND
    kMCInputReturnKeyTypeDone = 0x6, // IME_ACTION_DONE
} MCInputReturnKeyType;

static MCNativeControlEnumEntry s_return_key_type_enum[] =
{
    { "default", kMCInputReturnKeyTypeDefault },
    { "done", kMCInputReturnKeyTypeDone },
    { "go", kMCInputReturnKeyTypeGo },
    { "next", kMCInputReturnKeyTypeNext },
    { "search", kMCInputReturnKeyTypeSearch },
    { "send", kMCInputReturnKeyTypeSend },
    { nil, 0 },
};

typedef enum
{
    kMCInputContentTypePlain,
    kMCInputContentTypePassword,
} MCInputContentType;

static MCNativeControlEnumEntry s_content_type_enum[] =
{
    { "plain", kMCInputContentTypePlain },
    { "password", kMCInputContentTypePassword },
    { nil, 0 },
};

typedef enum
{
    kMCInputDataTypeWebUrl = 0x1,
    kMCInputDataTypeEmailAddress = 0x2,
    kMCInputDataTypePhoneNumber = 0x4,
    kMCInputDataTypeMapAddress = 0x8,
} MCInputDataType;

static MCNativeControlEnumEntry s_datadetectortype_enum[] =
{
    { "link", kMCInputDataTypeWebUrl },
    { "address", kMCInputDataTypeMapAddress },
    { "phone number", kMCInputDataTypePhoneNumber },
    { "email", kMCInputDataTypeEmailAddress },
    { nil, 0 },
};

typedef enum
{
    kMCInputTextAlignCenter = 0x1,
    kMCInputTextAlignLeft = 0x3,
    kMCInputTextAlignRight = 0x5,
} MCInputTextAlign;

static MCNativeControlEnumEntry s_textalign_enum[] =
{
    { "center", kMCInputTextAlignCenter },
    { "left", kMCInputTextAlignLeft },
    { "right", kMCInputTextAlignRight },
    { nil, 0 },
};

typedef enum
{
	kMCInputVerticalAlignCenter = 0x10,
	kMCInputVerticalAlignTop = 0x30,
	kMCInputVerticalAlignBottom = 0x50,
} MCInputVerticalAlign;

static MCNativeControlEnumEntry s_verticalalign_enum[] =
{
	{ "center", kMCInputVerticalAlignCenter },
	{ "top", kMCInputVerticalAlignTop },
	{ "bottom", kMCInputVerticalAlignBottom },
	{ nil, 0 },
};

////////////////////////////////////////////////////////////////////////////////

class MCAndroidInputControl: public MCAndroidControl
{
public:
    MCAndroidInputControl(void);
    
	virtual MCNativeControlType GetType(void);
    
    virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint &ep);
    virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint &ep);
    virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);
    
    void SetMultiLine(bool p_multiline);
    
protected:
    virtual ~MCAndroidInputControl(void);
    virtual jobject CreateView(void);
    virtual void DeleteView(jobject view);
    
private:
    bool m_is_multiline;
};


////////////////////////////////////////////////////////////////////////////////

MCAndroidInputControl::MCAndroidInputControl(void)
{
    m_is_multiline = true;
}

MCAndroidInputControl::~MCAndroidInputControl(void)
{
    
}

MCNativeControlType MCAndroidInputControl::GetType(void)
{
    return m_is_multiline ? kMCNativeControlTypeMultiLineInput : kMCNativeControlTypeInput;
}

void MCAndroidInputControl::SetMultiLine(bool p_multiline)
{
    jobject t_view = GetView();
    
    m_is_multiline = p_multiline;
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setMultiLine", "vb", nil, p_multiline);
}

Exec_stat MCAndroidInputControl::Set(MCNativeControlProperty p_property, MCExecPoint &ep)
{
    jobject t_view;
    t_view = GetView();
    if (t_view == nil)
        return MCAndroidControl::Set(p_property, ep);
    
    int32_t t_integer;
    int32_t t_enum;
    bool t_bool;
    
    switch (p_property)
    {
        case kMCNativeControlPropertyText:
            MCAndroidObjectRemoteCall(t_view, "setText", "vS", nil, &(ep.getsvalue()));
            return ES_NORMAL;
            
        case kMCNativeControlPropertyUnicodeText:
            MCAndroidObjectRemoteCall(t_view, "setText", "vU", nil, &(ep.getsvalue()));
            return ES_NORMAL;

        case kMCNativeControlPropertyTextColor:
        {
            uint16_t t_r, t_g, t_b, t_a;
            if (MCNativeControl::ParseColor(ep, t_r, t_g, t_b, t_a))
                MCAndroidObjectRemoteCall(t_view, "setTextColor", "viiii", nil, t_r >> 8, t_g >> 8, t_b >> 8, t_a >> 8);
            else
            {
                MCeerror->add(EE_OBJECT_BADCOLOR, 0, 0, ep.getsvalue());
                return ES_ERROR;
            }
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyFontSize:
        {
            if (!ParseInteger(ep, t_integer))
                return ES_ERROR;
            MCAndroidObjectRemoteCall(t_view, "setTextSize", "vi", nil, t_integer);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyTextAlign:
        {
            if (!ParseEnum(ep, s_textalign_enum, t_enum))
                return ES_ERROR;
            MCAndroidObjectRemoteCall(t_view, "setTextAlign", "vi", nil, t_enum);
            return ES_NORMAL;
        }
            
		case kMCNativeControlPropertyVerticalTextAlign:
		{
			if (!ParseEnum(ep, s_verticalalign_enum, t_enum))
				return ES_ERROR;
			MCAndroidObjectRemoteCall(t_view, "setVerticalAlign", "vi", nil, t_enum);
			return ES_NORMAL;
		}
			
        case kMCNativeControlPropertyEnabled:
        case kMCNativeControlPropertyEditable:
        {
            if (!ParseBoolean(ep, t_bool))
                return ES_ERROR;
            MCAndroidObjectRemoteCall(t_view, "setEnabled", "vb", nil, t_bool);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyAutoCapitalizationType:
        {
            if (!ParseEnum(ep, s_autocapitalizationtype_enum, t_enum))
                return ES_ERROR;
            MCAndroidObjectRemoteCall(t_view, "setCapitalization", "vi", nil, t_enum);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyAutoCorrectionType:
        {
            if (!ParseEnum(ep, s_autocorrectiontype_enum, t_enum))
                return ES_ERROR;
            t_bool = t_enum == kMCInputAutocorrectionYes;
            MCAndroidObjectRemoteCall(t_view, "setAutocorrect", "vb", nil, t_bool);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyKeyboardType:
        {
            if (!ParseEnum(ep, s_keyboard_type_enum, t_enum))
                return ES_ERROR;
            MCAndroidObjectRemoteCall(t_view, "setKeyboardType", "vi", nil, t_enum);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyReturnKeyType:
        {
            if (!ParseEnum(ep, s_return_key_type_enum, t_enum))
                return ES_ERROR;
            MCAndroidObjectRemoteCall(t_view, "setReturnKeyType", "viS", nil, t_enum, &(ep.getsvalue()));
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyContentType:
        {
            if (!ParseEnum(ep, s_content_type_enum, t_enum))
                return ES_ERROR;
            t_bool = t_enum == kMCInputContentTypePassword;
            MCAndroidObjectRemoteCall(t_view, "setIsPassword", "vb", nil, t_bool);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyScrollingEnabled:
        {
            if (!ParseBoolean(ep, t_bool))
                return ES_ERROR;
            MCAndroidObjectRemoteCall(t_view, "setScrollingEnabled", "vb", nil, t_bool);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyDataDetectorTypes:
        {
            if (!ParseSet(ep, s_datadetectortype_enum, t_enum))
                return ES_ERROR;
            MCAndroidObjectRemoteCall(t_view, "setDataDetectorTypes", "vi", nil, t_enum);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyMultiLine:
        {
            if (!ParseBoolean(ep, t_bool))
                return ES_ERROR;
            SetMultiLine(t_bool);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertySelectedRange:
        {
            uint32_t t_start, t_length;
            if (!ParseRange(ep, t_start, t_length))
                return ES_ERROR;
            MCAndroidObjectRemoteCall(t_view, "setSelectedRange", "vii", nil, t_start, t_length);
            return ES_NORMAL;
        }
            
        default:
            break;
    }
    
    return MCAndroidControl::Set(p_property, ep);
}

Exec_stat MCAndroidInputControl::Get(MCNativeControlProperty p_property, MCExecPoint &ep)
{
    jobject t_view;
    t_view = GetView();
    if (t_view == nil)
        return MCAndroidControl::Get(p_property, ep);
    
    switch (p_property)
    {
        case kMCNativeControlPropertyText:
        {
            MCString t_text;
            MCAndroidObjectRemoteCall(t_view, "getText", "S", &t_text);
            ep.setsvalue(t_text);
            ep.grabsvalue();
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyUnicodeText:
        {
            MCString t_text;
            MCAndroidObjectRemoteCall(t_view, "getText", "U", &t_text);
            ep.setsvalue(t_text);
            ep.grabsvalue();
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyTextColor:
        {
            int32_t t_color;
            uint16_t t_r, t_g, t_b, t_a;
            MCAndroidObjectRemoteCall(t_view, "getTextColor", "i", &t_color);
            MCJavaColorToComponents(t_color, t_r, t_g, t_b, t_a);
            FormatColor(ep, t_r, t_g, t_b, t_a);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyFontSize:
        {
            int32_t t_size;
            MCAndroidObjectRemoteCall(t_view, "getTextSize", "i", &t_size);
            FormatInteger(ep, t_size);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyTextAlign:
        {
            int32_t t_enum;
            MCAndroidObjectRemoteCall(t_view, "getTextAlign", "i", &t_enum);
            FormatEnum(ep, s_textalign_enum, t_enum);
            return ES_NORMAL;
        }
			
		case kMCNativeControlPropertyVerticalTextAlign:
		{
			int32_t t_enum;
			MCAndroidObjectRemoteCall(t_view, "getVerticalAlign", "i", &t_enum);
			FormatEnum(ep, s_verticalalign_enum, t_enum);
			return ES_NORMAL;
		}
            
        case kMCNativeControlPropertyEnabled:
        case kMCNativeControlPropertyEditable:
        {
            bool t_editable;
            MCAndroidObjectRemoteCall(t_view, "getEnabled", "b", &t_editable);
            FormatBoolean(ep, t_editable);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyAutoCapitalizationType:
        {
            int32_t t_enum;
            MCAndroidObjectRemoteCall(t_view, "getCapitalization", "i", &t_enum);
            FormatEnum(ep, s_autocapitalizationtype_enum, t_enum);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyAutoCorrectionType:
        {
            bool t_autocorrect;
            MCAndroidObjectRemoteCall(t_view, "getAutocorrect", "b", &t_autocorrect);
            FormatEnum(ep, s_autocorrectiontype_enum, t_autocorrect ? kMCInputAutocorrectionYes : kMCInputAutocorrectionNo);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyKeyboardType:
        {
            int32_t t_enum;
            MCAndroidObjectRemoteCall(t_view, "getKeyboardType", "i", &t_enum);
            FormatEnum(ep, s_keyboard_type_enum, t_enum);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyReturnKeyType:
        {
            int32_t t_enum;
            MCAndroidObjectRemoteCall(t_view, "getReturnKeyType", "i", &t_enum);
            FormatEnum(ep, s_return_key_type_enum, t_enum);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyContentType:
        {
            bool t_password;
            MCAndroidObjectRemoteCall(t_view, "getIsPassword", "b", &t_password);
            FormatEnum(ep, s_content_type_enum, t_password ? kMCInputContentTypePassword : kMCInputContentTypePlain);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyScrollingEnabled:
        {
            bool t_enabled;
            MCAndroidObjectRemoteCall(t_view, "getScrollingEnabled", "b", &t_enabled);
            FormatBoolean(ep, t_enabled);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyDataDetectorTypes:
        {
            int32_t t_enum;
            MCAndroidObjectRemoteCall(t_view, "getDataDetectorTypes", "i", &t_enum);
            FormatSet(ep, s_datadetectortype_enum, t_enum);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyMultiLine:
        {
            bool t_bool;
            MCAndroidObjectRemoteCall(t_view, "getMultiLine", "b", &t_bool);
            FormatBoolean(ep, t_bool);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertySelectedRange:
        {
            uint32_t t_start, t_length;
            MCAndroidObjectRemoteCall(t_view, "getSelectedRangeStart", "i", &t_start);
            MCAndroidObjectRemoteCall(t_view, "getSelectedRangeLength", "i", &t_length);
            FormatRange(ep, t_start, t_length);
            return ES_NORMAL;
        }
            
        default:
            break;
    }
    
    return MCAndroidControl::Get(p_property, ep);
}

Exec_stat MCAndroidInputControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
{
    jobject t_view;
    t_view = GetView();
    
    switch (p_action)
    {
        case kMCNativeControlActionFocus:
            MCAndroidObjectRemoteCall(t_view, "focusControl", "v", nil);
            return ES_NORMAL;
            
        default:
            break;
    }
    
    return MCAndroidControl::Do(p_action, p_parameters);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_InputControl_doBeginEditing(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_InputControl_doBeginEditing(JNIEnv *env, jobject object)
{
    MCCustomEvent *t_event;
    MCAndroidControl *t_control = nil;
    
    if (MCAndroidControl::FindByView(object, t_control))
        t_control->PostNotifyEvent(MCM_input_begin_editing);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_InputControl_doEndEditing(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_InputControl_doEndEditing(JNIEnv *env, jobject object)
{
    MCCustomEvent *t_event;
    MCAndroidControl *t_control = nil;
    
    if (MCAndroidControl::FindByView(object, t_control))
        t_control->PostNotifyEvent(MCM_input_end_editing);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_InputControl_doTextChanged(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_InputControl_doTextChanged(JNIEnv *env, jobject object)
{
    MCCustomEvent *t_event;
    MCAndroidControl *t_control = nil;
    
    if (MCAndroidControl::FindByView(object, t_control))
        t_control->PostNotifyEvent(MCM_input_text_changed);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_InputControl_doReturnKey(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_InputControl_doReturnKey(JNIEnv *env, jobject object)
{
    MCCustomEvent *t_event;
    MCAndroidControl *t_control = nil;
    
    if (MCAndroidControl::FindByView(object, t_control))
        t_control->PostNotifyEvent(MCM_input_return_key);
}

////////////////////////////////////////////////////////////////////////////////

jobject MCAndroidInputControl::CreateView(void)
{
    jobject t_view;
    MCAndroidEngineRemoteCall("createInputControl", "o", &t_view);
    MCAndroidObjectRemoteCall(t_view, "setMultiLine", "vb", nil, m_is_multiline);
    return t_view;
}

void MCAndroidInputControl::DeleteView(jobject p_view)
{
    JNIEnv *env;
    env = MCJavaGetThreadEnv();
    
    env->DeleteGlobalRef(p_view);
}

////////////////////////////////////////////////////////////////////////////////

bool MCNativeInputControlCreate(MCNativeControl *&r_control)
{
    MCAndroidInputControl *t_control = new MCAndroidInputControl();
    // configure as single-line input control
    t_control->SetMultiLine(false);
    
    r_control = t_control;
    return true;
}

bool MCNativeMultiLineInputControlCreate(MCNativeControl *&r_control)
{
    // configure as multi-line input control
    MCAndroidInputControl *t_control = new MCAndroidInputControl();
    t_control->SetMultiLine(true);
    
    r_control = t_control;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
