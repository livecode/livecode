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

#include "mcerror.h"

#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"
#include "osspec.h"
#include "exec.h"


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
    kMCInputDataTypeAll = (0x1 | 0x2 | 0x4 | 0x8)
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
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;

public:
    MCAndroidInputControl(void);
    
	virtual MCNativeControlType GetType(void);
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    
    void SetMultiLine(bool p_multiline);
    
    void SetMultiLine(MCExecContext& ctxt, bool p_multiline);
    void SetText(MCExecContext& ctxt, MCStringRef p_string);
    void SetTextColor(MCExecContext& ctxt, const MCNativeControlColor& p_color);
    void SetTextSize(MCExecContext& ctxt, uinteger_t p_size);
    void SetTextAlign(MCExecContext& ctxt, MCNativeControlInputTextAlign p_align);
    void SetVerticalTextAlign(MCExecContext& ctxt, MCNativeControlInputVerticalAlign p_align);
    void SetEnabled(MCExecContext& ctxt, bool p_value);
	void SetEditable(MCExecContext& ctxt, bool p_value);
    void SetAutoCapitalizationType(MCExecContext& ctxt, MCNativeControlInputCapitalizationType p_type);
    void SetAutoCorrectionType(MCExecContext& ctxt, MCNativeControlInputAutocorrectionType p_type);
    void SetKeyboardType(MCExecContext& ctxt, MCNativeControlInputKeyboardType p_type);
    void SetReturnKey(MCExecContext& ctxt, MCNativeControlInputReturnKeyType p_key);
    void SetContentType(MCExecContext& ctxt, MCNativeControlInputContentType p_type);
    void SetScrollingEnabled(MCExecContext& ctxt, bool p_value);
    void SetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType p_type);
    void SetSelectedRange(MCExecContext& ctxt, const MCNativeControlRange& p_range);
    
    void GetMultiLine(MCExecContext& ctxt, bool& r_multiline);
    void GetText(MCExecContext& ctxt, MCStringRef& r_string);
    void GetTextColor(MCExecContext& ctxt, MCNativeControlColor& r_color);
    void GetTextSize(MCExecContext& ctxt, uinteger_t& r_size);
    void GetTextAlign(MCExecContext& ctxt, MCNativeControlInputTextAlign& r_align);
    void GetVerticalTextAlign(MCExecContext& ctxt, MCNativeControlInputVerticalAlign& r_align);
    void GetEnabled(MCExecContext& ctxt, bool& r_value);
	void GetEditable(MCExecContext& ctxt, bool& r_value);
    void GetAutoCapitalizationType(MCExecContext& ctxt, MCNativeControlInputCapitalizationType& r_type);
    void GetAutoCorrectionType(MCExecContext& ctxt, MCNativeControlInputAutocorrectionType& r_type);
    void GetKeyboardType(MCExecContext& ctxt, MCNativeControlInputKeyboardType& r_type);
    void GetReturnKey(MCExecContext& ctxt, MCNativeControlInputReturnKeyType& r_key);
    void GetContentType(MCExecContext& ctxt, MCNativeControlInputContentType& r_type);
    void GetScrollingEnabled(MCExecContext& ctxt, bool& r_value);
    void GetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType& r_type);
    void GetSelectedRange(MCExecContext& ctxt, MCNativeControlRange& r_range);
    
	// Input-specific actions
	void ExecFocus(MCExecContext& ctxt);
    
protected:
    virtual ~MCAndroidInputControl(void);
    virtual jobject CreateView(void);
    virtual void DeleteView(jobject view);
    
private:
    bool m_is_multiline;
};

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCAndroidInputControl::kProperties[] =
{
    DEFINE_RW_CTRL_PROPERTY(P_MULTI_LINE, Bool, MCAndroidInputControl, MultiLine)
    DEFINE_RW_CTRL_PROPERTY(P_TEXT, String, MCAndroidInputControl, Text)
    DEFINE_RW_CTRL_PROPERTY(P_UNICODE_TEXT, String, MCAndroidInputControl, Text)
    DEFINE_RW_CTRL_CUSTOM_PROPERTY(P_TEXT_COLOR, NativeControlColor, MCAndroidInputControl, TextColor)
    DEFINE_RW_CTRL_PROPERTY(P_FONT_SIZE, UInt32, MCAndroidInputControl, TextSize)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_TEXT_ALIGN, NativeControlInputTextAlign, MCAndroidInputControl, TextAlign)
    DEFINE_RW_CTRL_PROPERTY(P_ENABLED, Bool, MCAndroidInputControl, Enabled)
    DEFINE_RW_CTRL_PROPERTY(P_EDITABLE, Bool, MCAndroidInputControl, Editable)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_AUTO_CAPITALIZATION_TYPE, NativeControlInputCapitalizationType, MCAndroidInputControl, AutoCapitalizationType)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_AUTOCORRECTION_TYPE, NativeControlInputAutocorrectionType, MCAndroidInputControl, AutoCorrectionType)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_KEYBOARD_TYPE, NativeControlInputKeyboardType, MCAndroidInputControl, KeyboardType)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_RETURN_KEY_TYPE, NativeControlInputReturnKeyType, MCAndroidInputControl, ReturnKey)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_CONTENT_TYPE, NativeControlInputContentType, MCAndroidInputControl, ContentType)
    DEFINE_RW_CTRL_PROPERTY(P_SCROLLING_ENABLED, Bool, MCAndroidInputControl, ScrollingEnabled)
    DEFINE_RW_CTRL_SET_PROPERTY(P_DATA_DETECTOR_TYPES, NativeControlInputDataDetectorType, MCAndroidInputControl, DataDetectorTypes)
    DEFINE_RW_CTRL_CUSTOM_PROPERTY(P_SELECTED_RANGE, NativeControlRange, MCAndroidInputControl, SelectedRange)
};

MCObjectPropertyTable MCAndroidInputControl::kPropertyTable =
{
	&MCAndroidControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCAndroidInputControl::kActions[] =
{
    DEFINE_CTRL_EXEC_METHOD(Focus, Void, MCAndroidInputControl, Focus)
};

MCNativeControlActionTable MCAndroidInputControl::kActionTable =
{
    &MCAndroidControl::kActionTable,
    sizeof(kActions) / sizeof(kActions[0]),
    &kActions[0],
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

void MCAndroidInputControl::SetMultiLine(MCExecContext& ctxt, bool p_multiline)
{
    jobject t_view = GetView();
    
    m_is_multiline = p_multiline;
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setMultiLine", "vb", nil, p_multiline);
}

void MCAndroidInputControl::SetText(MCExecContext& ctxt, MCStringRef p_string)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setText", "vx", nil, p_string);
}

void MCAndroidInputControl::SetTextColor(MCExecContext& ctxt, const MCNativeControlColor& p_color)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setTextColor", "viiii", nil, p_color . r >> 8, p_color . g >> 8, p_color . b >> 8, p_color . a >> 8);
}

void MCAndroidInputControl::SetTextSize(MCExecContext& ctxt, uinteger_t p_size)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setTextSize", "vi", nil, p_size);
}

void MCAndroidInputControl::SetTextAlign(MCExecContext& ctxt, MCNativeControlInputTextAlign p_align)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
    {
        MCInputTextAlign t_align;
        
        switch (p_align)
        {
            case kMCNativeControlInputTextAlignCenter:
                t_align = kMCInputTextAlignCenter;
                break;
            case kMCNativeControlInputTextAlignLeft:
                t_align = kMCInputTextAlignLeft;
                break;
            case kMCNativeControlInputTextAlignRight:
                t_align = kMCInputTextAlignRight;
                break;
        }
        
        MCAndroidObjectRemoteCall(t_view, "setTextAlign", "vi", nil, t_align);
    }
}

void MCAndroidInputControl::SetVerticalTextAlign(MCExecContext& ctxt, MCNativeControlInputVerticalAlign p_align)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
    {
        MCInputVerticalAlign t_align;
        
        switch (p_align)
        {
            case kMCNativeControlInputVerticalAlignCenter:
                t_align = kMCInputVerticalAlignCenter;
                break;
            case kMCNativeControlInputVerticalAlignTop:
                t_align = kMCInputVerticalAlignTop;
                break;
            case kMCNativeControlInputVerticalAlignBottom:
                t_align = kMCInputVerticalAlignBottom;
                break;
        }
        
        MCAndroidObjectRemoteCall(t_view, "setVerticalAlign", "vi", nil, t_align);
    }
}

void MCAndroidInputControl::SetEnabled(MCExecContext& ctxt, bool p_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setEnabled", "vb", nil, p_value);
}

// PM-2015-01-14: [[ Bug 16704 ]] Allow a non-editable multiline fld to be scrolled
void MCAndroidInputControl::SetEditable(MCExecContext& ctxt, bool p_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setEditable", "vb", nil, p_value);
}


void MCAndroidInputControl::SetAutoCapitalizationType(MCExecContext& ctxt, MCNativeControlInputCapitalizationType p_type)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
    {
        MCInputCapitalizationType t_type;
        switch (p_type)
        {
            case kMCNativeControlInputCapitalizeNone:
                t_type = kMCInputCapitalizeNone;
                break;
            case kMCNativeControlInputCapitalizeWords:
                t_type = kMCInputCapitalizeWords;
                break;
            case kMCNativeControlInputCapitalizeSentences:
                t_type = kMCInputCapitalizeSentences;
                break;
            case kMCNativeControlInputCapitalizeCharacters:
                t_type = kMCInputCapitalizeCharacters;
                break;
        }
        MCAndroidObjectRemoteCall(t_view, "setCapitalization", "vi", nil, t_type);
    }
}

void MCAndroidInputControl::SetAutoCorrectionType(MCExecContext& ctxt, MCNativeControlInputAutocorrectionType p_type)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
    {
        bool t_autocorrect;
        t_autocorrect = p_type != kMCNativeControlInputAutocorrectionNo;
        MCAndroidObjectRemoteCall(t_view, "setAutocorrect", "vb", nil, t_autocorrect);
    }
}

void MCAndroidInputControl::SetKeyboardType(MCExecContext& ctxt, MCNativeControlInputKeyboardType p_type)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
    {
        MCInputKeyboardType t_type;
        switch (p_type)
        {
            case kMCNativeControlInputKeyboardTypeDefault:
            case kMCNativeControlInputKeyboardTypeAlphabet:
                t_type = kMCInputKeyboardTypeAlphabet;
                break;
            case kMCNativeControlInputKeyboardTypeDecimal:
            case kMCNativeControlInputKeyboardTypeNumeric:
                t_type = kMCInputKeyboardTypeNumeric;
                break;
            case kMCNativeControlInputKeyboardTypeURL:
                t_type = kMCInputKeyboardTypeURL;
                break;
            case kMCNativeControlInputKeyboardTypeNumber:
                t_type = kMCInputKeyboardTypeNumber;
                break;
            case kMCNativeControlInputKeyboardTypePhone:
                t_type = kMCInputKeyboardTypePhone;
                break;
            case kMCNativeControlInputKeyboardTypeContact:
                t_type = kMCInputKeyboardTypeContact;
                break;
            case kMCNativeControlInputKeyboardTypeEmail:
                t_type = kMCInputKeyboardTypeEmail;
                break;
        }
        MCAndroidObjectRemoteCall(t_view, "setKeyboardType", "vi", nil, t_type);
    }
}

void MCAndroidInputControl::SetReturnKey(MCExecContext& ctxt, MCNativeControlInputReturnKeyType p_key)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
    {
        MCInputReturnKeyType t_type;
        switch (p_key)
        {
            case kMCNativeControlInputReturnKeyTypeDefault:
                t_type = kMCInputReturnKeyTypeDefault;
                break;
            case kMCNativeControlInputReturnKeyTypeGo:
                t_type = kMCInputReturnKeyTypeGo;
                break;
            case kMCNativeControlInputReturnKeyTypeNext:
                t_type = kMCInputReturnKeyTypeNext;
                break;
            case kMCNativeControlInputReturnKeyTypeSearch:
                t_type = kMCInputReturnKeyTypeSearch;
                break;
            case kMCNativeControlInputReturnKeyTypeSend:
                t_type = kMCInputReturnKeyTypeSend;
                break;
            case kMCNativeControlInputReturnKeyTypeDone:
                t_type = kMCInputReturnKeyTypeDone;
                break;
        }
        MCAndroidObjectRemoteCall(t_view, "setReturnKeyType", "vix", nil, t_type, kMCEmptyString);
    }
}

void MCAndroidInputControl::SetContentType(MCExecContext& ctxt, MCNativeControlInputContentType p_type)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
    {
        bool t_is_password;
        t_is_password = p_type == kMCNativeControlInputContentTypePassword;
     
        MCAndroidObjectRemoteCall(t_view, "setIsPassword", "vb", nil, t_is_password);
    }
}

void MCAndroidInputControl::SetScrollingEnabled(MCExecContext& ctxt, bool p_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setScrollingEnabled", "vb", nil, p_value);
}

void MCAndroidInputControl::SetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType p_type)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
    {
        uint32_t t_type;
        t_type = 0;
        
        if (p_type & kMCNativeControlInputDataDetectorTypeAll)
            t_type |= kMCInputDataTypeAll;
        if (p_type & kMCNativeControlInputDataDetectorTypeWebUrl)
            t_type |= kMCInputDataTypeWebUrl;
        if (p_type & kMCNativeControlInputDataDetectorTypeMapAddress)
            t_type |= kMCInputDataTypeMapAddress;
        if (p_type & kMCNativeControlInputDataDetectorTypePhoneNumber)
            t_type |= kMCInputDataTypePhoneNumber;
        if (p_type & kMCNativeControlInputDataDetectorTypeEmailAddress)
            t_type |= kMCInputDataTypeEmailAddress;

        MCAndroidObjectRemoteCall(t_view, "setDataDetectorTypes", "vi", nil, t_type);
    }
}

void MCAndroidInputControl::SetSelectedRange(MCExecContext& ctxt, const MCNativeControlRange& p_range)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setSelectedRange", "vii", nil, p_range . start, p_range . length);
}

void MCAndroidInputControl::GetMultiLine(MCExecContext& ctxt, bool& r_multiline)
{
    jobject t_view = GetView();

    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getMultiLine", "b", &r_multiline);
}

void MCAndroidInputControl::GetText(MCExecContext& ctxt, MCStringRef& r_string)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getText", "x", &r_string);
    else
        r_string = MCValueRetain(kMCEmptyString);
}

void MCAndroidInputControl::GetTextColor(MCExecContext& ctxt, MCNativeControlColor& r_color)
{
    jobject t_view;
    t_view = GetView();
    int32_t t_color = 0;

    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getTextColor", "i", &t_color);
    
    MCJavaColorToComponents(t_color, r_color . r, r_color . g, r_color . b, r_color . a);
}

void MCAndroidInputControl::GetTextSize(MCExecContext& ctxt, uinteger_t& r_size)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getTextSize", "i", &r_size);
    else
        r_size = 0;
}

void MCAndroidInputControl::GetTextAlign(MCExecContext& ctxt, MCNativeControlInputTextAlign& r_align)
{
    jobject t_view;
    t_view = GetView();
    
    MCInputTextAlign t_align;
    // Default to the switch's default
    t_align = kMCInputTextAlignLeft;
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getTextAlign", "v", &t_align);
        
    switch (t_align)
    {
        case kMCInputTextAlignCenter:
            r_align = kMCNativeControlInputTextAlignCenter;
            return;
        case kMCInputTextAlignRight:
            r_align = kMCNativeControlInputTextAlignRight;
            return;
        case kMCInputTextAlignLeft:
        default:
            r_align = kMCNativeControlInputTextAlignLeft;
            return;
    }
}

void MCAndroidInputControl::GetVerticalTextAlign(MCExecContext& ctxt, MCNativeControlInputVerticalAlign& r_align)
{
    jobject t_view;
    t_view = GetView();
    
    MCInputVerticalAlign t_align;
    // Default to the switch's default
    t_align = kMCInputVerticalAlignCenter;
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getVerticalAlign", "i", &t_align);
        
    switch (t_align)
    {
        case kMCInputVerticalAlignTop:
            r_align = kMCNativeControlInputVerticalAlignTop;
            return;
        case kMCInputVerticalAlignBottom:
            r_align = kMCNativeControlInputVerticalAlignBottom;
            return;
        case kMCInputVerticalAlignCenter:
        default:
            r_align = kMCNativeControlInputVerticalAlignCenter;
            return;
    }
}

void MCAndroidInputControl::GetEnabled(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getEnabled", "b", &r_value);
    else
        r_value = false;
}

void MCAndroidInputControl::GetEditable(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getEditable", "b", &r_value);
    else
        r_value = false;
}

void MCAndroidInputControl::GetAutoCapitalizationType(MCExecContext& ctxt, MCNativeControlInputCapitalizationType& r_type)
{
    jobject t_view;
    t_view = GetView();
    
    MCInputCapitalizationType t_type;
    // Default to the switch default
    t_type = kMCInputCapitalizeNone;
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getCapitalization", "i", &t_type);

    switch (t_type)
    {
        case kMCInputCapitalizeWords:
            r_type = kMCNativeControlInputCapitalizeWords;
            return;
        case kMCInputCapitalizeSentences:
            r_type = kMCNativeControlInputCapitalizeSentences;
            return;
        case kMCInputCapitalizeCharacters:
            r_type = kMCNativeControlInputCapitalizeCharacters;
            return;
        case kMCInputCapitalizeNone:
        default:
            r_type = kMCNativeControlInputCapitalizeNone;
            return;
    }
}

void MCAndroidInputControl::GetAutoCorrectionType(MCExecContext& ctxt, MCNativeControlInputAutocorrectionType& r_type)
{
    jobject t_view;
    t_view = GetView();

    bool t_autocorrect = true;
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getAutocorrect", "b", &t_autocorrect);
    
    r_type = t_autocorrect ? kMCNativeControlInputAutocorrectionYes : kMCNativeControlInputAutocorrectionNo;
}

void MCAndroidInputControl::GetKeyboardType(MCExecContext& ctxt, MCNativeControlInputKeyboardType& r_type)
{
    jobject t_view;
    t_view = GetView();

    MCInputKeyboardType t_type;
    // Default to the switch's default
    t_type = kMCInputKeyboardTypeDefault;
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getKeyboardType", "v", &t_type);

    switch (t_type)
    {
        case kMCInputKeyboardTypeNumeric:
            r_type = kMCNativeControlInputKeyboardTypeNumeric;
            return;
        case kMCInputKeyboardTypeURL:
            r_type = kMCNativeControlInputKeyboardTypeURL;
            return;
        case kMCInputKeyboardTypeNumber:
            r_type = kMCNativeControlInputKeyboardTypeNumber;
            return;
        case kMCInputKeyboardTypePhone:
            r_type = kMCNativeControlInputKeyboardTypePhone;
            return;
        case kMCInputKeyboardTypeContact:
            r_type = kMCNativeControlInputKeyboardTypeContact;
            return;
        case kMCInputKeyboardTypeEmail:
            r_type = kMCNativeControlInputKeyboardTypeEmail;
            return;
        case kMCInputKeyboardTypeAlphabet:
        default:
            r_type = kMCNativeControlInputKeyboardTypeAlphabet;
            return;
    }
}

void MCAndroidInputControl::GetReturnKey(MCExecContext& ctxt, MCNativeControlInputReturnKeyType& r_type)
{
    jobject t_view;
    t_view = GetView();

    MCInputReturnKeyType t_type;
    // Default to the switch's default
    t_type = kMCInputReturnKeyTypeDefault;
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getReturnKeyType", "i", &t_type);
    
    switch (t_type)
    {
        case kMCInputReturnKeyTypeGo:
            r_type = kMCNativeControlInputReturnKeyTypeGo;
            break;
        case kMCInputReturnKeyTypeNext:
            r_type = kMCNativeControlInputReturnKeyTypeNext;
            break;
        case kMCInputReturnKeyTypeSearch:
            r_type = kMCNativeControlInputReturnKeyTypeSearch;
            break;
        case kMCInputReturnKeyTypeSend:
            r_type = kMCNativeControlInputReturnKeyTypeSend;
            break;
        case kMCInputReturnKeyTypeDone:
            r_type = kMCNativeControlInputReturnKeyTypeDone;
            break;
        case kMCInputReturnKeyTypeDefault:
        default:
            r_type = kMCNativeControlInputReturnKeyTypeDefault;
            break;
    }
}

void MCAndroidInputControl::GetContentType(MCExecContext& ctxt, MCNativeControlInputContentType& r_type)
{
    jobject t_view;
    t_view = GetView();
    
    bool t_is_password;
    t_is_password = false;
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getIsPassword", "b", &t_is_password);
    
    r_type = t_is_password ? kMCNativeControlInputContentTypePassword : kMCNativeControlInputContentTypePlain;
}

void MCAndroidInputControl::GetScrollingEnabled(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getScrollingEnabled", "b", nil, &r_value);
}

void MCAndroidInputControl::GetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType& r_type)
{
    jobject t_view;
    t_view = GetView();
    
    uint32_t t_type;
    t_type = 0;
    
    uint32_t t_detector_types;
    t_detector_types = 0;
    
    if (t_view != nil)
    {
        MCAndroidObjectRemoteCall(t_view, "getDataDetectorTypes", "i", &t_type);
    
        if (t_type & kMCInputDataTypeWebUrl)
            t_detector_types |= kMCNativeControlInputDataDetectorTypeWebUrl;
        if (t_type & kMCInputDataTypeMapAddress)
            t_detector_types |= kMCNativeControlInputDataDetectorTypeMapAddress;
        if (t_type & kMCInputDataTypePhoneNumber)
            t_type |= kMCNativeControlInputDataDetectorTypePhoneNumber;
        if (t_detector_types & kMCInputDataTypeEmailAddress)
            t_detector_types |= kMCNativeControlInputDataDetectorTypeEmailAddress;
    }
    
    r_type = (MCNativeControlInputDataDetectorType)t_detector_types;
}

void MCAndroidInputControl::GetSelectedRange(MCExecContext& ctxt, MCNativeControlRange& r_range)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
    {
        MCAndroidObjectRemoteCall(t_view, "getSelectedRangeStart", "i", &r_range . start);
        MCAndroidObjectRemoteCall(t_view, "getSelectedRangeLength", "i", &r_range . length);
    }
}

void MCAndroidInputControl::ExecFocus(MCExecContext& ctxt)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view == nil)
        return;
        
    MCAndroidObjectRemoteCall(t_view, "focusControl", "v", nil);
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
    MCAndroidInputControl *t_control = new (nothrow) MCAndroidInputControl();
    // configure as single-line input control
    t_control->SetMultiLine(false);
    
    r_control = t_control;
    return true;
}

bool MCNativeMultiLineInputControlCreate(MCNativeControl *&r_control)
{
    // configure as multi-line input control
    MCAndroidInputControl *t_control = new (nothrow) MCAndroidInputControl();
    t_control->SetMultiLine(true);
    
    r_control = t_control;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
