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
