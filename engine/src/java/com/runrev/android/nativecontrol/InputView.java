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

package com.runrev.android.nativecontrol;

import android.content.*;
import android.graphics.*;
import android.util.*;
import android.view.*;
import android.view.inputmethod.*;
import android.widget.*;

public class InputView extends EditText
{
    public static final String TAG = "revandroid.InputView";
    
    public InputView(Context context)
    {
        super(context);
    }
    
    public InputView(Context context, AttributeSet attrs)
    {
        super(context, attrs);
    }
    
    public InputView(Context context, AttributeSet attrs, int defStyle)
    {
        super(context, attrs, defStyle);
    }
    
    public interface OnTextChangedListener
    {
        public void onTextChanged(CharSequence text, int start, int lengthBefore, int lengthAfter);
    }
    
    private OnTextChangedListener m_text_changed_listener;
    
    public void setOnTextChangedListener(OnTextChangedListener listener)
    {
        m_text_changed_listener = listener;
    }
    
    @Override
    public void onTextChanged(CharSequence text, int start, int lengthBefore, int lengthAfter)
    {
        if (m_text_changed_listener != null)
            m_text_changed_listener.onTextChanged(text, start, lengthBefore, lengthAfter);
        
        super.onTextChanged(text, start, lengthBefore, lengthAfter);
    }
}
