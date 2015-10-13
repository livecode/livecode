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

#include "stack.h"
#include "widget.h"
#include "widget-ref.h"
#include "osspec.h"

#include "module-resources.h"
#include "libscript/script.h"

#include "filepath.h"
#include "foundation-auto.h"

extern bool MCEngineLookupResourcePathForModule(MCScriptModuleRef p_module, MCStringRef &r_resource_path);

bool MCResourceResolvePath(MCStringRef p_name, MCStringRef &r_path)
{
	MCScriptModuleRef t_module;
	t_module = MCScriptGetCurrentModule();
	if (t_module == nil)
		return false;
	
	MCAutoStringRef t_path;
	if (!MCEngineLookupResourcePathForModule(t_module, &t_path))
		return false;
	
	return MCPathAppend(*t_path, p_name, r_path);
}

bool MCResourceResolveStackRelativePath(MCStringRef p_name, MCStringRef& r_path)
{
	if (MCPathIsAbsolute(p_name))
    {
        r_path = MCValueRetain(p_name);
        return true;
    }
    
    MCStack *t_stack;
    t_stack = nil;
    if (MCcurrentwidget != nil)
        t_stack = MCWidgetGetHost(MCcurrentwidget) -> getstack();
    
    if (t_stack != nil &&
        t_stack -> resolve_relative_path(p_name, r_path))
        return true;
    
    if (MCS_resolvepath(p_name, r_path))
        return true;
        
    return false;
}
