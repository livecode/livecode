/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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

package com.runrev.android;

import java.lang.*;
import java.lang.reflect.*;

public class LCBInvocationHandler extends Object implements InvocationHandler
{
	long m_handler_ptr;
	
	public LCBInvocationHandler(long p_handler)
	{
		m_handler_ptr = p_handler;
	}
	
	public static Object getProxy(Class<?> p_interface, long p_handler)
	{
		InvocationHandler t_inv_handler = new LCBInvocationHandler(p_handler);

		Object t_proxy =
			Proxy.newProxyInstance(p_interface.getClassLoader(),
								   new Class[] { p_interface },
								   t_inv_handler);						   
								   
		return t_proxy;
	}
	
	public Object invoke(Object proxy, Method method, Object[] args)
	{
		return doNativeListenerCallback(m_handler_ptr, method.getName(), args);
	}
	
	public static native Object doNativeListenerCallback(long handler, String method_name, Object[] args);
}