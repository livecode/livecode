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

#include <foundation.h>
#include <foundation-auto.h>

static bool TryToInitializeJava()
{
	if (!MCJavaInitialize())
	{
		MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason",
							  MCSTR("could not initialize java"), nil);
		return false;
	}
	
	return true;
}

extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCJavaObjectTypeInfo() { return MCJavaGetObjectTypeInfo(); }

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF bool MCJavaNewObject(MCStringRef p_class_name, MCListRef p_args, MCJavaObjectRef& r_object)
{
	if (!TryToInitializeJava())
		return false;
	
    MCNewAutoNameRef t_name;
    MCNameCreate(p_class_name, &t_name);
    return MCJavaCallConstructor(*t_name, p_args, r_object);
}

extern "C" MC_DLLEXPORT_DEF void MCJavaStringFromJString(MCJavaObjectRef p_object, MCStringRef &r_string)
{
	if (!TryToInitializeJava())
		return;
	
    MCJavaConvertJStringToStringRef(p_object, r_string);
}

extern "C" MC_DLLEXPORT_DEF void MCJavaStringToJString(MCStringRef p_string, MCJavaObjectRef &r_object)
{
	if (!TryToInitializeJava())
		return;
	
    MCJavaConvertStringRefToJString(p_string, r_object);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_java_Initialize(void)
{
    if (!MCJavaCreateJavaObjectTypeInfo())
        return false;
    
    return true;
}

extern "C" void com_livecode_java_Finalize(void)
{
    MCJavaFinalize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

