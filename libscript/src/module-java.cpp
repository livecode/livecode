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
	if (!MCJavaVMInitialize())
		return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason",
                                     MCSTR("could not initialize java"),
                                     nullptr);

	return true;
}

extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCJavaObjectTypeInfo() { return MCJavaGetObjectTypeInfo(); }

bool MCJavaErrorThrow(MCTypeInfoRef p_error_type)
{
    MCAutoErrorRef t_error;
    if (!MCErrorCreate(p_error_type, nil, &t_error))
        return false;
    
    return MCErrorThrow(*t_error);
}

MC_DLLEXPORT MCTypeInfoRef kMCJavaCouldNotConvertStringToJStringErrorTypeInfo;
MC_DLLEXPORT MCTypeInfoRef kMCJavaCouldNotConvertJStringToStringErrorTypeInfo;
MC_DLLEXPORT MCTypeInfoRef kMCJavaCouldNotConvertDataToJByteArrayErrorTypeInfo;
MC_DLLEXPORT MCTypeInfoRef kMCJavaCouldNotConvertJByteArrayToDataErrorTypeInfo;
MC_DLLEXPORT MCTypeInfoRef kMCJavaCouldNotGetObjectClassNameErrorTypeInfo;
MC_DLLEXPORT MCTypeInfoRef kMCJavaCouldNotCreateJObjectErrorTypeInfo;

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void MCJavaStringFromJString(MCJavaObjectRef p_object, MCStringRef &r_string)
{
	if (!TryToInitializeJava())
		return;
	
    if (!MCJavaConvertJStringToStringRef(p_object, r_string))
        MCJavaErrorThrow(kMCJavaCouldNotConvertJStringToStringErrorTypeInfo);
}

extern "C" MC_DLLEXPORT_DEF void MCJavaStringToJString(MCStringRef p_string, MCJavaObjectRef &r_object)
{
	if (!TryToInitializeJava())
		return;
	
    if (!MCJavaConvertStringRefToJString(p_string, r_object))
        MCJavaErrorThrow(kMCJavaCouldNotConvertStringToJStringErrorTypeInfo);
}

extern "C" MC_DLLEXPORT_DEF void MCJavaDataFromJByteArray(MCJavaObjectRef p_object, MCDataRef &r_data)
{
    if (!TryToInitializeJava())
        return;
    
    if (!MCJavaConvertJByteArrayToDataRef(p_object, r_data))
        MCJavaErrorThrow(kMCJavaCouldNotConvertJByteArrayToDataErrorTypeInfo);
}

extern "C" MC_DLLEXPORT_DEF void MCJavaDataToJByteArray(MCDataRef p_data, MCJavaObjectRef &r_object)
{
    if (!TryToInitializeJava())
        return;
    
    if (!MCJavaConvertDataRefToJByteArray(p_data, r_object))
        MCJavaErrorThrow(kMCJavaCouldNotConvertDataToJByteArrayErrorTypeInfo);
}

extern "C" MC_DLLEXPORT_DEF void MCJavaGetClassName(MCJavaObjectRef p_obj, MCStringRef &r_string)
{
    if (!TryToInitializeJava())
        return;
    
    if (!MCJavaGetJObjectClassName(p_obj, r_string))
        MCJavaErrorThrow(kMCJavaCouldNotGetObjectClassNameErrorTypeInfo);
}

extern "C" MC_DLLEXPORT_DEF void MCJavaUnwrapJObject(MCJavaObjectRef p_obj, void*&r_pointer)
{
    r_pointer = MCJavaObjectGetObject(p_obj);
}

extern "C" MC_DLLEXPORT_DEF void MCJavaWrapJObject(void *p_pointer, MCJavaObjectRef& r_obj)
{
    if (!MCJavaObjectCreate(p_pointer, r_obj))
        MCJavaErrorThrow(kMCJavaCouldNotCreateJObjectErrorTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MCJavaErrorsInitialize()
{
    if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.java.ConvertFromStringError"), MCNAME("java"), MCSTR("Could not convert String to Java string"), kMCJavaCouldNotConvertStringToJStringErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.java.ConvertToStringError"), MCNAME("java"), MCSTR("Could not convert Java byte array to Data"), kMCJavaCouldNotConvertJStringToStringErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.java.ConvertFromDataError"), MCNAME("java"), MCSTR("Could not convert Java byte array to Data"), kMCJavaCouldNotConvertDataToJByteArrayErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.java.ConvertToDataError"), MCNAME("java"), MCSTR("Could not convert Java byte array to Data"), kMCJavaCouldNotConvertJByteArrayToDataErrorTypeInfo))
        return false;
        
    if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.java.FetchJavaClassNameError"), MCNAME("java"), MCSTR("Could not get Java object class name"), kMCJavaCouldNotGetObjectClassNameErrorTypeInfo))
        return false;

    if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.java.CreateJObjectError"), MCNAME("java"), MCSTR("Could not create JObject from Pointer"), kMCJavaCouldNotCreateJObjectErrorTypeInfo))
        return false;
    
    return true;
}

void MCJavaErrorsFinalize()
{
    MCValueRelease(kMCJavaCouldNotConvertStringToJStringErrorTypeInfo);
    MCValueRelease(kMCJavaCouldNotConvertJStringToStringErrorTypeInfo);
    MCValueRelease(kMCJavaCouldNotConvertDataToJByteArrayErrorTypeInfo);
    MCValueRelease(kMCJavaCouldNotConvertJByteArrayToDataErrorTypeInfo);
    MCValueRelease(kMCJavaCouldNotGetObjectClassNameErrorTypeInfo);
    MCValueRelease(kMCJavaCouldNotCreateJObjectErrorTypeInfo);
}

extern "C" bool com_livecode_java_Initialize(void)
{
    if (!MCJavaErrorsInitialize())
        return false;
    
    if (!MCJavaCreateJavaObjectTypeInfo())
        return false;
    
    return true;
}

extern "C" void com_livecode_java_Finalize(void)
{
    MCJavaErrorsFinalize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

