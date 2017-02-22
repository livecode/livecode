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
		return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason",
                                     MCSTR("could not initialize java"),
                                     nullptr);

	return true;
}

extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCJavaObjectTypeInfo() { return MCJavaGetObjectTypeInfo(); }

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void MCJavaStringFromJString(MCJavaObjectRef p_object, MCStringRef &r_string)
{
	if (!TryToInitializeJava())
		return;
	
    if (!MCJavaConvertJStringToStringRef(p_object, r_string))
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason",
                              MCSTR("couldn't convert java object to string"),
                              nullptr);
}

extern "C" MC_DLLEXPORT_DEF void MCJavaStringToJString(MCStringRef p_string, MCJavaObjectRef &r_object)
{
	if (!TryToInitializeJava())
		return;
	
    if (!MCJavaConvertStringRefToJString(p_string, r_object))
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason",
                              MCSTR("couldn't convert string to java object"),
                              nullptr);
}

extern "C" MC_DLLEXPORT_DEF void MCJavaDataFromJByteArray(MCJavaObjectRef p_object, MCDataRef &r_data)
{
    if (!TryToInitializeJava())
        return;
    
    if (!MCJavaConvertJByteArrayToDataRef(p_object, r_data))
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason",
                              MCSTR("couldn't convert java object to data"),
                              nullptr);
}

extern "C" MC_DLLEXPORT_DEF void MCJavaDataToJByteArray(MCDataRef p_data, MCJavaObjectRef &r_object)
{
    if (!TryToInitializeJava())
        return;
    
    if (!MCJavaConvertDataRefToJByteArray(p_data, r_object))
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason",
                              MCSTR("couldn't convert data to java object"),
                              nullptr);
}

extern "C" MC_DLLEXPORT_DEF void MCJavaGetClassName(MCJavaObjectRef p_obj, MCStringRef &r_string)
{
    if (!TryToInitializeJava())
        return;
    
    if (!MCJavaGetJObjectClassName(p_obj, r_string))
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason",
                              MCSTR("couldn't get java object class name"),
                              nullptr);
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

