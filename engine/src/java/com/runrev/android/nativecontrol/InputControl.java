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
import android.text.*;
import android.text.method.*;
import android.util.*;
import android.view.*;
import android.view.inputmethod.*;
import android.widget.*;

class InputControl extends NativeControl
{
    public static final String TAG = "revandroid.NativeInputControl";
    
    private InputView m_text_view;
    
    private MovementMethod m_movement_method;
    private TransformationMethod m_transformation_method;
    
    private int m_text_type = 0;
    private int m_text_type_flags = 0;
    
    public InputControl(NativeControlModule p_module)
    {
        super(p_module);
    }
    
    public View createView(Context p_context)
    {
        LayoutInflater t_inflater = LayoutInflater.from(p_context);
        int t_res_id = p_context.getResources().getIdentifier("layout/livecode_inputcontrol", null, p_context.getPackageName());
		//        Log.i(TAG, "resource id: " + t_res_id);
        m_text_view = (InputView) t_inflater.inflate(t_res_id, null);
               
        m_text_view.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus)
            {
				//                Log.i(TAG, String.format("onFocusChange(%s, %b)", String.valueOf(v), hasFocus));
				
                if (hasFocus)
                    doBeginEditing();
                else
                    doEndEditing();
                m_module.getEngine().wakeEngineThread();
            }
        });
        
        m_text_view.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
            {
				//                Log.i(TAG, String.format("onEditorAction(%s, %d, %s)", String.valueOf(v), actionId, String.valueOf(event)));
				
                doReturnKey();
                m_module.getEngine().wakeEngineThread();
                
                return false;
            }
        });
        
        m_text_view.setOnTextChangedListener(new InputView.OnTextChangedListener() {
            @Override
            public void onTextChanged(CharSequence text, int start, int lengthBefore, int lengthAfter)
            {
				//                Log.i(TAG, String.format("onTextChanged(%s, %d, %d, %d)", text.toString(), start, lengthBefore, lengthAfter));
                
                doTextChanged();
                m_module.getEngine().wakeEngineThread();
            }
        });
		
		m_text_view.setGravity(Gravity.LEFT | Gravity.TOP);
		
        return m_text_view;
    }
       
    public void setEnabled(boolean p_enabled)
    {
        m_text_view.setEnabled(p_enabled);
    }
	
	public void setEditable(boolean p_editable)
    {
		m_text_view.setFocusable(p_editable);
    }
    
    public boolean getEnabled()
    {
        return m_text_view.isEnabled();
    }
	
	public boolean getEditable()
    {
        return m_text_view.isFocusable();
    }
    
    public void setText(String p_text)
    {
		//        Log.i(TAG, String.format("setText(%s)", p_text));
        m_text_view.setText(p_text);
    }
    
    public String getText()
    {
        return m_text_view.getText().toString();
    }
    
    public void setTextColor(int red, int green, int blue, int alpha)
    {
        m_text_view.setTextColor(Color.argb(alpha, red, green, blue));
    }
    
    public int getTextColor()
    {
        return m_text_view.getTextColors().getDefaultColor();
    }
    
    public void setTextSize(int p_size)
    {
        // AL-2013-08-02: [[ Bug 11080 ]] Android input field fontSize setting value in incorrect units
        m_text_view.setTextSize(TypedValue.COMPLEX_UNIT_DIP, p_size);
    }
    
    public void setMultiLine(boolean p_multiline)
    {
        int t_type = m_text_view.getInputType();
		
        if (p_multiline)
            t_type |= InputType.TYPE_TEXT_FLAG_MULTI_LINE;
        else
            t_type &= ~InputType.TYPE_TEXT_FLAG_MULTI_LINE;
        
        m_text_view.setInputType(t_type);
    }
    
    public boolean getMultiLine()
    {
        return 0 != (InputType.TYPE_TEXT_FLAG_MULTI_LINE & m_text_view.getInputType());
    }
    
    public int getTextSize()
    {
        return (int) (0.5 + m_text_view.getTextSize() / m_text_view.getContext().getResources().getDisplayMetrics().density);
    }
    
    public void setCapitalization(int p_cap)
    {
        int t_input_type = m_text_view.getInputType();
        t_input_type = t_input_type & ~(InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS | InputType.TYPE_TEXT_FLAG_CAP_WORDS | InputType.TYPE_TEXT_FLAG_CAP_SENTENCES);
        m_text_view.setInputType(t_input_type | p_cap);
    }
    
    public int getCapitalization()
    {
        int t_input_type = m_text_view.getInputType();
        return t_input_type & (InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS | InputType.TYPE_TEXT_FLAG_CAP_WORDS | InputType.TYPE_TEXT_FLAG_CAP_SENTENCES);
    }
    
    public void setAutocorrect(boolean p_autocorrect)
    {
        int t_input_type = m_text_view.getInputType();
        if (p_autocorrect)
            t_input_type |= InputType.TYPE_TEXT_FLAG_AUTO_CORRECT;
        else
            t_input_type &= ~InputType.TYPE_TEXT_FLAG_AUTO_CORRECT;
        m_text_view.setInputType(t_input_type);
    }
    
    public boolean getAutocorrect()
    {
        return 0 != (m_text_view.getInputType() & InputType.TYPE_TEXT_FLAG_AUTO_CORRECT);
    }
    
    public void setKeyboardType(int p_type)
    {
        int t_flags = m_text_view.getInputType() & InputType.TYPE_MASK_FLAGS;
        // unset flags from previous type setting
        if (m_text_type_flags != 0)
            t_flags &= ~m_text_type_flags;
        
        m_text_type = p_type & (InputType.TYPE_MASK_CLASS | InputType.TYPE_MASK_VARIATION);
        m_text_type_flags = p_type & InputType.TYPE_MASK_FLAGS;
        
        m_text_view.setInputType(m_text_type | m_text_type_flags | t_flags);
    }
    
    public int getKeyboardType()
    {
        if (m_text_type == 0)
            return m_text_view.getInputType() & InputType.TYPE_MASK_CLASS;
        
        return m_text_type | m_text_type_flags;
    }
    
    public void setReturnKeyType(int p_type, String p_label)
    {
        int t_ime_options = m_text_view.getImeOptions() & ~EditorInfo.IME_MASK_ACTION;
        m_text_view.setImeOptions(t_ime_options | p_type);
    }
    
    public int getReturnKeyType()
    {
        return m_text_view.getImeOptions() & EditorInfo.IME_MASK_ACTION;
    }
    
    public void setScrollingEnabled(boolean p_enabled)
    {
		//        Log.i(TAG, "old movement method: " + m_text_view.getMovementMethod());
        if (m_movement_method == null)
            m_movement_method = m_text_view.getMovementMethod();
		
        if (p_enabled)
            m_text_view.setMovementMethod(m_movement_method);
        else
            m_text_view.setMovementMethod(null);
		//        Log.i(TAG, "new movement method: " + m_text_view.getMovementMethod());
    }
    
    public boolean getScrollingEnabled()
    {
        return null != m_text_view.getMovementMethod();
    }
    
    public void setIsPassword(boolean p_password)
    {
        TransformationMethod t_transform = m_text_view.getTransformationMethod();
        
        if (p_password && !(t_transform instanceof PasswordTransformationMethod))
        {
            m_transformation_method = m_text_view.getTransformationMethod();
            m_text_view.setTransformationMethod(PasswordTransformationMethod.getInstance());
            m_text_view.setInputType(InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS | InputType.TYPE_TEXT_VARIATION_PASSWORD);
        }
        else if (!p_password && m_transformation_method != null)
        {
            m_text_view.setTransformationMethod(m_transformation_method);
        }
    }
    
    public boolean getIsPassword()
    {
        return m_text_view.getTransformationMethod() instanceof PasswordTransformationMethod;
    }
    
    public void setDataDetectorTypes(int p_types)
    {
        m_text_view.setAutoLinkMask(p_types);
    }
    
    public int getDataDetectorTypes()
    {
        return m_text_view.getAutoLinkMask();
    }
    
    public void setTextAlign(int p_align)
    {
        int t_gravity = m_text_view.getGravity();
        t_gravity &= ~Gravity.HORIZONTAL_GRAVITY_MASK;
        t_gravity |= p_align;
        m_text_view.setGravity(t_gravity);
    }
    
    public int getTextAlign()
    {
        return m_text_view.getGravity() & Gravity.HORIZONTAL_GRAVITY_MASK;
    }
	
	public void setVerticalAlign(int p_align)
	{
		int t_gravity = m_text_view.getGravity();
		t_gravity &= ~Gravity.VERTICAL_GRAVITY_MASK;
		t_gravity |= p_align;
		
		m_text_view.setGravity(t_gravity);
	}
	
	public int getVerticalAlign()
	{
		return m_text_view.getGravity() & Gravity.VERTICAL_GRAVITY_MASK;
	}
    
    public void setSelectedRange(int start, int length)
    {
        Selection.setSelection(m_text_view.getText(), start - 1, start - 1 + length);
    }
    
    public int getSelectedRangeStart()
    {
        return Selection.getSelectionStart(m_text_view.getText()) + 1;
    }
    
    public int getSelectedRangeLength()
    {
        return Selection.getSelectionEnd(m_text_view.getText()) + 1 - Selection.getSelectionStart(m_text_view.getText());
    }
    
    public void focusControl()
    {
        m_text_view.requestFocus();
        
        InputMethodManager imm;
        imm = (InputMethodManager) m_text_view.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        
        if (imm != null)
            imm.restartInput(m_text_view);
        
        imm.showSoftInput(m_text_view, InputMethodManager.SHOW_IMPLICIT);
    }
    
    public native void doBeginEditing();
    public native void doEndEditing();
    public native void doTextChanged();
    public native void doReturnKey();
}
