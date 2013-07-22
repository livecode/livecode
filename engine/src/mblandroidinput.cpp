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
public:
    MCAndroidInputControl(void);
    
	virtual MCNativeControlType GetType(void);
#ifdef LEGACY_EXEC
    virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint &ep);
    virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint &ep);
#endif
    virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);

    void SetMultiLine(bool p_multiline);
    
    void SetMultiLine(MCExecContext& ctxt, bool p_multiline);
    void SetText(MCExecContext& ctxt, MCStringRef p_string);
    void SetTextColor(MCExecContext& ctxt, const MCNativeControlColor& p_color);
    void SetTextSize(MCExecContext& ctxt, integer_t p_size);
    void SetTextAlign(MCExecContext& ctxt, intenum_t p_align);
    void SetVerticalTextAlign(MCExecContext& ctxt, intenum_t p_align);
    void SetEnabled(MCExecContext& ctxt, bool p_value);
    void SetAutoCapitalizationType(MCExecContext& ctxt, intenum_t p_type);
    void SetAutoCorrectionType(MCExecContext& ctxt, intenum_t p_type);
    void SetKeyboardType(MCExecContext& ctxt, intenum_t p_type);
    void SetReturnKey(MCExecContext& ctxt, MCNativeControlInputReturnKeyType p_key);
    void SetContentType(MCExecContext& ctxt, MCNativeControlInputContentType p_type);
    void SetScrollingEnabled(MCExecContext& ctxt, bool p_value);
    void SetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType p_type);
    void SetSelectedRange(MCExecContext& ctxt, const MCNativeControlRange& p_range);
    
    void GetMultiLine(MCExecContext& ctxt, bool& r_multiline);
    void GetText(MCExecContext& ctxt, MCStringRef& r_string);
    void GetTextColor(MCExecContext& ctxt, MCNativeControlColor& r_color);
    void GetTextSize(MCExecContext& ctxt, integer_t& r_size);
    void GetTextAlign(MCExecContext& ctxt, intenum_t& r_align);
    void GetVerticalTextAlign(MCExecContext& ctxt, intenum_t& r_align);
    void GetEnabled(MCExecContext& ctxt, bool& r_value);
    void GetAutoCapitalizationType(MCExecContext& ctxt, intenum_t& r_type);
    void GetAutoCorrectionType(MCExecContext& ctxt, intenum_t& r_type);
    void GetKeyboardType(MCExecContext& ctxt, intenum_t& r_type);
    void GetReturnKey(MCExecContext& ctxt, MCNativeControlInputReturnKeyType& r_key);
    void GetContentType(MCExecContext& ctxt, MCNativeControlInputContentType& r_type);
    void GetScrollingEnabled(MCExecContext& ctxt, bool& r_value);
    void GetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType& r_type);
    void GetSelectedRange(MCExecContext& ctxt, MCNativeControlRange& r_range);
    
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
        MCAndroidObjectRemoteCall(t_view, "setText", "vx", nil, *t_string);
}

void MCAndroidInputControl::SetTextColor(MCExecContext& ctxt, const MCNativeControlColor& p_color)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setTextColor", "viiii", nil, p_color . r >> 8, p_color . g >> 8, p_color . b >> 8, p_color . a >> 8);
}

void MCAndroidInputControl::SetTextSize(MCExecContext& ctxt, integer_t p_size)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setTextSize", "vi", nil, p_size);
}

void MCAndroidInputControl::SetTextAlign(MCExecContext& ctxt, intenum_t p_align)
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

void MCAndroidInputControl::SetVerticalTextAlign(MCExecContext& ctxt, intenum_t p_align)
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

void MCAndroidInputControl::SetAutoCapitalizationType(MCExecContext& ctxt, intenum_t p_type)
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

void MCAndroidInputControl::SetAutoCorrectionType(MCExecContext& ctxt, intenum_t p_type)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
    {
        bool t_autocorrect;
        t_autocorrect = p_type != kMCNativeControlInputAutocorrectionNo;
        MCAndroidObjectRemoteCall(t_view, "setAutocorrect", "vb", nil, t_bool);
    }
}

void MCAndroidInputControl::SetKeyboardType(MCExecContext& ctxt, intenum_t p_type)
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
        switch (p_type)
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
        MCAndroidObjectRemoteCall(t_view, "setReturnKeyType", "vix", nil, t_enum, *t_string);
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

void MCAndroidInputControl::SetDataDetectorTypes(MCExecContext& ctxt, intset_t p_type)
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

void MCAndroidInputControl::SetMultiLine(MCExecContext& ctxt, bool p_multiline)
{
    jobject t_view = GetView();
    
    m_is_multiline = p_multiline;
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setMultiLine", "vb", nil, p_multiline);
}

void MCAndroidInputControl::GetText(MCExecContext& ctxt, MCStringRef& r_string)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getText", "x", &t_string);
    else
        r_string = MCValueRetain(kMCEmptyString);
}

void MCAndroidInputControl::GetTextColor(MCExecContext& ctxt, const MCNativeControlColor& p_color)
{
    jobject t_view;
    t_view = GetView();
    int32_t t_color = 0;

    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getTextColor", "i", &t_color);
    
    MCJavaColorToComponents(t_color, p_color . r, p_color . g, p_color . b, p_color . a);
}

void MCAndroidInputControl::GetTextSize(MCExecContext& ctxt, integer_t& r_size)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getTextSize", "i", &r_size);
    else
        r_size = 0;
}

void MCAndroidInputControl::GetTextAlign(MCExecContext& ctxt, intenum_t& r_align)
{
    jobject t_view;
    t_view = GetView();
    
    MCInputTextAlign t_align;
    
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

void MCAndroidInputControl::GetVerticalTextAlign(MCExecContext& ctxt, intenum_t& r_align)
{
    jobject t_view;
    t_view = GetView();
    
    MCInputVerticalAlign t_align;
    
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

void MCAndroidInputControl::GetAutoCapitalizationType(MCExecContext& ctxt, intenum_t& r_type)
{
    jobject t_view;
    t_view = GetView();
    
    MCInputCapitalizationType t_type;
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getCapitalization", "i", &t_type);

    switch (p_type)
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

void MCAndroidInputControl::GetAutoCorrectionType(MCExecContext& ctxt, intenum_t& r_type)
{
    jobject t_view;
    t_view = GetView();

    bool t_autocorrect = true;
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getAutocorrect", "b", &t_autocorrect);
    
    r_type = t_autocorrect ? kMCNativeControlInputAutocorrectionYes : kMCNativeControlInputAutocorrectionNo;
}

void MCAndroidInputControl::GetKeyboardType(MCExecContext& ctxt, intenum_t& r_type)
{
    jobject t_view;
    t_view = GetView();

    MCInputKeyboardType t_type;
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getKeyboardType", "v", &t_type);

    switch (t_type)
    {
        case kMCInputKeyboardTypeNumeric:
            t_type = kMCNativeControlInputKeyboardTypeNumeric;
            return;
        case kMCInputKeyboardTypeURL:
            t_type = kMCNativeControlInputKeyboardTypeURL;
            return;
        case kMCInputKeyboardTypeNumber:
            t_type = kMCNativeControlInputKeyboardTypeNumber;
            return;
        case kMCInputKeyboardTypePhone:
            t_type = kMCNativeControlInputKeyboardTypePhone;
            return;
        case kMCInputKeyboardTypeContact:
            t_type = kMCNativeControlInputKeyboardTypeContact;
            return;
        case kMCInputKeyboardTypeEmail:
            t_type = kMCNativeControlInputKeyboardTypeEmail;
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
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getReturnKeyType", "i", &t_type);
    
    switch (t_type)
    {
        case kMCInputReturnKeyTypeGo:
            t_type = kMCNativeControlInputReturnKeyTypeGo;
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

void MCAndroidInputControl::GetDataDetectorTypes(MCExecContext& ctxt, intset_t& r_type)
{
    jobject t_view;
    t_view = GetView();
    
    uint32_t t_type;
    t_type = 0;
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getDataDetectorTypes", "i", &t_type);

    r_type = kMCNativeControlInputDataDetectorTypeNone;
    
    if (t_type & kMCInputDataTypeWebUrl)
        r_type |= kMCNativeControlInputDataDetectorTypeWebUrl;
    if (t_type & kMCInputDataTypeMapAddress)
        r_type |= kMCNativeControlInputDataDetectorTypeMapAddress;
    if (t_type & kMCInputDataTypePhoneNumber)
        r_type |= kMCNativeControlInputDataDetectorTypePhoneNumber;
    if (t_type & kMCInputDataTypeEmailAddress)
        r_type |= kMCNativeControlInputDataDetectorTypeEmailAddress;
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

#ifdef /* MCAndroidInputControl::Set */ LEGACY_EXEC
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
		case kMCNativeControlPropertyUnicodeText:
		{
			MCAutoStringRef t_string;
			UNCHECKED ep . copyasstringref(&t_string);
            MCAndroidObjectRemoteCall(t_view, "setText", "vx", nil, *t_string);
            return ES_NORMAL;
		}

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
			MCAutoStringRef t_string;
			/* UNCHECKED */ ep . copyasstringref(&t_string);
            MCAndroidObjectRemoteCall(t_view, "setReturnKeyType", "vix", nil, t_enum, *t_string);
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
#endif /* MCAndroidInputControl::Set */

void MCAndroidInputControl::Set(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
    MCExecPoint& ep = ctxt . GetEP();
    
	switch(p_property)
	{
		case kMCNativeControlPropertyText:
        case kMCNativeControlPropertyUnicodeText:
        {
            MCAutoStringRef t_text;
            /* UNCHECKED */ ep . copyasstringref(&t_text);
            SetText(ctxt, *t_text);
            return;
        }
            
		case kMCNativeControlPropertyTextColor:
        {
            MCAutoStringRef t_string;
            MCNativeControlColor t_color;
            /* UNCHECKED */ ep . copyasstringref(&t_string);
            MCNativeControlColorParse(ctxt, *t_string, t_color);
            if (!ctxt . HasError())
                SetTextColor(ctxt, t_color);
            return;
        }
			
		case kMCNativeControlPropertyFontSize:
        {
			int32_t t_size;
            if (!ep . copyasint(t_size))
                ctxt . LegacyThrow(EE_OBJECT_NAN);
            else
                SetFontSize(ctxt, t_size);
            return;
        }
            
        case kMCNativeControlPropertyTextAlign:
        {
            intenum_t t_align;
            ctxt_to_enum(ctxt, kMCNativeControlInputTextAlignTypeInfo, t_align);
            SetTextAlign(ctxt, (MCNativeControlInputTextAlign)t_align);
            return;
        }
    
        case kMCNativeControlPropertyVerticalTextAlign:
		{
            intenum_t t_align;
            ctxt_to_enum(ctxt, kMCNativeControlInputVerticalAlignTypeInfo, t_align);
            SetVerticalTextAlign(ctxt, (MCNativeControlInputVerticalAlign)t_align);
            return;
        }
            
        case kMCNativeControlPropertyEditable:
		case kMCNativeControlPropertyEnabled:
        {
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetEnabled(ctxt, t_value);
            return;
        }
			
		case kMCNativeControlPropertyAutoCapitalizationType:
        {
            intenum_t t_type;
            ctxt_to_enum(ctxt, kMCNativeControlInputCapitalizationTypeTypeInfo, t_type);
            SetAutoCapitalizationType(ctxt, (MCNativeControlInputCapitalizationType)t_type);
            return;
        }
		case kMCNativeControlPropertyAutoCorrectionType:
        {
            intenum_t t_type;
            ctxt_to_enum(ctxt, kMCNativeControlInputAutocorrectiontionTypeTypeInfo, t_type);
            SetAutoCorrectionType(ctxt, (MCNativeControlInputAutocorrectionType)t_type);
            return;
		}

		case kMCNativeControlPropertyKeyboardStyle:
        {
            intenum_t t_style;
            ctxt_to_enum(ctxt, kMCNativeControlInputKeyboardStyleTypeInfo, t_style);
            SetKeyboardStyle(ctxt, (MCNativeControlInputKeyboardStyle)t_style);
            return;
		}

		case kMCNativeControlPropertyReturnKeyType:
        {
            intenum_t t_type;
            ctxt_to_enum(ctxt, kMCNativeControlInputReturnKeyTypeTypeInfo, t_type);
            SetReturnKey(ctxt, (MCNativeControlInputReturnKeyType)t_type);
            return;
		}
            
		case kMCNativeControlPropertyContentType:
        {
            intenum_t t_type;
            ctxt_to_enum(ctxt, kMCNativeControlInputContentTypeTypeInfo, t_type);
            SetContentType(ctxt, (MCNativeControlInputContentType)t_type);
            return;
        }
            
        case kMCNativeControlPropertyScrollingEnabled:
		{
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetScrollingEnabled(ctxt, t_value);
            return;
        }
        
        case kMCNativeControlPropertyDataDetectorTypes:
        {
            intset_t t_types;
            ctxt_to_set(ctxt, kMCNativeControlInputDataDetectorTypeTypeInfo, t_types);
            SetDataDetectorTypes(ctxt, (MCNativeControlInputDataDetectorType)t_types);
            return;
        }
            
        case kMCNativeControlPropertyMultiLine:
        {
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetMultiLine(ctxt, t_value);
            return;
        }
            
        case kMCNativeControlPropertySelectedRange:
        {
            MCNativeControlRange t_range;
            MCAutoStringRef t_string;
            /* UNCHECKED */ ep . copyasstringref(&t_string);
            MCNativeControlRangeParse(ctxt, *t_string, t_range);
            if (!ctxt . HasError())
                SetSelectedRange(ctxt, t_range);
        }
            
		default:
			break;
	}
	
	MCAndroidControl::Set(ctxt, p_property);
}

#ifdef /* MCAndroidInputControl::Get */ LEGACY_EXEC
Exec_stat MCAndroidInputControl::Get(MCNativeControlProperty p_property, MCExecPoint &ep)
{
    jobject t_view;
    t_view = GetView();
    if (t_view == nil)
        return MCAndroidControl::Get(p_property, ep);
    
    switch (p_property)
    {
        case kMCNativeControlPropertyText:
		case kMCNativeControlPropertyUnicodeText:
        {
            MCAutoStringRef t_text;
            MCAndroidObjectRemoteCall(t_view, "getText", "x", &t_text);
            /* UNCHECKED */ ep.setvalueref(*t_text);
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
#endif /* MCAndroidInputControl::Get */

void MCAndroidInputControl::Get(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
    MCExecPoint& ep = ctxt . GetEP();
    
	switch(p_property)
	{
        case kMCNativeControlPropertyText:
        case kMCNativeControlPropertyUnicodeText:
        {
            MCAutoStringRef t_text;
            GetText(ctxt, &t_text);
            if (*t_text != nil)
                ep . setvalueref(*t_text);
            return;
        }
            
        case kMCNativeControlPropertyTextColor:
        {
            MCNativeControlColor t_color;
            MCAutoStringRef t_string;
            GetTextColor(ctxt, t_color);
            MCNativeControlColorFormat(ctxt, t_color, &t_string);
            if (*t_string != nil)
                ep . setvalueref(*t_string);
            return;
        }

        case kMCNativeControlPropertyFontSize:
        {
            int32_t t_size;
            GetFontSize(ctxt, t_size);
            ep . setnvalue(t_size);
            return;
        }
            
        case kMCNativeControlPropertyTextAlign:
        {
            MCNativeControlInputTextAlign t_align;
            GetTextAlign(ctxt, t_align);
            enum_to_ctxt(ctxt, kMCNativeControlInputTextAlignTypeInfo, t_align);
            return;
        }
            
        case kMCNativeControlPropertyVerticalTextAlign:
		{
            MCNativeControlInputVerticalAlign t_align;
            GetVerticalTextAlign(ctxt, t_align);
            enum_to_ctxt(ctxt, kMCNativeControlInputVerticalAlignTypeInfo, t_align);
            return;
        }
            
        case kMCNativeControlPropertyEditable:
        case kMCNativeControlPropertyEnabled:
		{
            bool t_value;
            GetEnabled(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
            
        case kMCNativeControlPropertyAutoCapitalizationType:
        {
            MCNativeControlInputCapitalizationType t_type;
            GetAutoCapitalizationType(ctxt, t_type);
            enum_to_ctxt(ctxt, kMCNativeControlInputCapitalizationTypeTypeInfo, t_type);
            return;
        }
		case kMCNativeControlPropertyAutoCorrectionType:
        {
            MCNativeControlInputAutocorrectionType t_type;
            GetAutoCorrectionType(ctxt, t_type);
            enum_to_ctxt(ctxt, kMCNativeControlInputAutocorrectiontionTypeTypeInfo, t_type);
            return;
        }

		case kMCNativeControlPropertyKeyboardType:
        {
            MCNativeControlInputKeyboardType t_type;
            GetKeyboardType(ctxt, t_type);
            enum_to_ctxt(ctxt, kMCNativeControlInputKeyboardTypeTypeInfo, t_type);
            return;
        }
		case kMCNativeControlPropertyReturnKeyType:
        {
            MCNativeControlInputReturnKeyType t_type;
            GetReturnKey(ctxt, t_type);
            enum_to_ctxt(ctxt, kMCNativeControlInputReturnKeyTypeTypeInfo, t_type);
            return;
        }
		case kMCNativeControlPropertyContentType:
        {
            MCNativeControlInputContentType t_type;
            GetContentType(ctxt, t_type);
            enum_to_ctxt(ctxt, kMCNativeControlInputContentTypeTypeInfo, t_type);
            return;
        }
        
        case kMCNativeControlPropertyScrollingEnabled:
		{
            bool t_value;
            GetScrollingEnabled(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
            
        case kMCNativeControlPropertySelectedRange:
		{
            MCNativeControlRange t_range;
            GetSelectedRange(ctxt, t_range);
            MCAutoStringRef t_string;
            MCNativeControlRangeFormat(ctxt, t_range, &t_string);
            if (*t_string != nil)
                ep . setvalueref(*t_string);
            return;
        }
		case kMCNativeControlPropertyDataDetectorTypes:
		{
            MCNativeControlInputDataDetectorType t_types;
            GetDataDetectorTypes(ctxt, t_types);
            set_to_ctxt(ctxt, kMCNativeControlInputDataDetectorTypeTypeInfo, t_types);
            return;
		}
            
        case kMCNativeControlPropertyMultiLine:
        {
            bool t_value;
            GetMultiLine(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
        
        default:
            break;
            
    }
    MCAndroidControl::Get(ctxt, p_property);
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
