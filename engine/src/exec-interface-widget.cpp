/* Copyright (C) 2015 LiveCode Ltd.
 
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

#include "widget.h"
#include "widget-ref.h"

#include "exec.h"
#include "param.h"
#include "stack.h"
#include "globals.h"

#include "exec-interface.h"

void MCInterfaceExecGoBackInWidget(MCExecContext& ctxt, MCWidget *p_widget)
{
	MCWidgetPost(p_widget->getwidget(), MCNAME("OnGoBack"), kMCEmptyProperList);
}

void MCInterfaceExecGoForwardInWidget(MCExecContext& ctxt, MCWidget *p_widget)
{
	MCWidgetPost(p_widget->getwidget(), MCNAME("OnGoForward"), kMCEmptyProperList);
}

void MCInterfaceExecLaunchUrlInWidget(MCExecContext& ctxt, MCStringRef p_url, MCWidget *p_widget)
{
	MCAutoProperListRef t_list;
	
	if (!MCProperListCreate((MCValueRef*)&p_url, 1, &t_list))
		return;
	
	MCWidgetPost(p_widget->getwidget(), MCNAME("OnLaunchUrl"), *t_list);
}

void MCInterfaceExecDoInWidget(MCExecContext& ctxt, MCStringRef p_script, MCWidget *p_widget)
{
	MCAutoProperListRef t_list;
	
	if (!MCProperListCreate((MCValueRef*)&p_script, 1, &t_list))
		return;
	
	MCWidgetPost(p_widget->getwidget(), MCNAME("OnDo"), *t_list);
}

Exec_stat MCInterfaceExecCallWidget(MCExecContext& ctxt, MCNameRef p_message, MCWidget *p_widget, MCParameter *p_parameters, bool p_is_send)
{

    if (p_widget->getwidget() == nullptr)
    {
        ctxt.LegacyThrow(EE_INVOKE_EXTENSIONNOTFOUND);
        return ES_ERROR;
    }
    
    MCStackHandle t_old_default_stack;
    if (p_is_send)
    {
        t_old_default_stack = MCdefaultstackptr;
        MCdefaultstackptr = p_widget->getstack();
    }

    Exec_stat t_stat = MCWidgetCall(ctxt, p_widget->getwidget(), p_message, p_parameters);
    
    if (p_is_send && t_old_default_stack.IsValid())
    {
        MCdefaultstackptr = t_old_default_stack;
    }
    
    return t_stat;
}

