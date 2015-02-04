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
#include "mcio.h"

#include "globals.h"
#include "scriptpt.h"
#include "mode.h"
#include "handler.h"
#include "osspec.h"
#include "uidc.h"
#include "license.h"
#include "debug.h"
#include "param.h"
#include "property.h"
#include "hndlrlst.h"

#include "stack.h"
#include "card.h"

#include "exec.h"
#include "util.h"

#include "express.h"
#include "variable.h"
#include "chunk.h"
#include "securemode.h"
#include "dispatch.h"

#include "uuid.h"

#include "script.h"

////////////////////////////////////////////////////////////////////////////////

struct MCLoadedExtension
{
    MCLoadedExtension *next;
    MCStringRef filename;
	MCStringRef resource_path;
    MCScriptModuleRef module;
    MCScriptInstanceRef instance;
};

MCLoadedExtension *MCextensions = nil;
bool MCextensionschanged = false;
MCArrayRef MCextensionshandlermap = nil;

////////////////////////////////////////////////////////////////////////////////

static void __rebuild_library_handler_list(void)
{
    if (MCextensionshandlermap != nil)
        MCValueRelease(MCextensionshandlermap);
    
    MCArrayCreateMutable(MCextensionshandlermap);
    
    for(MCLoadedExtension *t_ext = MCextensions; t_ext != nil; t_ext = t_ext -> next)
        if (MCScriptIsModuleALibrary(t_ext -> module))
        {
            MCAutoProperListRef t_handlers;
            MCScriptCopyHandlersOfModule(t_ext -> module, &t_handlers);
            for(uindex_t i = 0; i < MCProperListGetLength(*t_handlers); i++)
            {
                MCNameRef t_name;
                t_name = (MCNameRef)MCProperListFetchElementAtIndex(*t_handlers, i);
                
                MCValueRef t_value;
                if (MCArrayFetchValue(MCextensionshandlermap, false, t_name, t_value))
                    continue;
                
                MCAutoValueRef t_ptr;
                MCForeignValueCreate(kMCPointerTypeInfo, &t_ext, (MCForeignValueRef&)t_ptr);
                MCArrayStoreValue(MCextensionshandlermap, false, t_name, *t_ptr);
            }
        }
}

bool MCEngineAddExtensionFromModule(MCStringRef p_filename, MCScriptModuleRef p_module)
{
    if (!MCScriptEnsureModuleIsUsable(p_module))
    {
        MCresult -> sets("module is not usable");
        return false;
    }
    
    MCScriptInstanceRef t_instance;
    t_instance = nil;
    if (MCScriptIsModuleALibrary(p_module))
    {
        if (!MCScriptCreateInstanceOfModule(p_module, t_instance))
        {
            MCresult -> sets("could not instantiate module");
            return false;
        }
    }
    
    MCLoadedExtension *t_ext;
    /* UNCHECKED */ MCMemoryNew(t_ext);
    
    t_ext -> filename = MCValueRetain(p_filename);
    t_ext -> module = MCScriptRetainModule(p_module);
    t_ext -> instance = t_instance;
    
    t_ext -> next = MCextensions;
    MCextensions = t_ext;
    
    MCextensionschanged = true;
    
    return true;
}

bool MCEngineAddResourcePathForModule(MCScriptModuleRef p_module, MCStringRef p_resource_path)
{
	for (MCLoadedExtension *t_ext = MCextensions; t_ext != nil; t_ext = t_ext->next)
	{
		if (t_ext->module == p_module)
		{
			MCAutoStringRef t_resolved_path;
			if (!MCS_resolvepath(p_resource_path, &t_resolved_path))
				return false;
			
			MCValueAssign(t_ext->resource_path, *t_resolved_path);
			return true;
		}
	}
	
	return false;
}

bool MCEngineLookupResourcePathForModule(MCScriptModuleRef p_module, MCStringRef &r_resource_path)
{
	for (const MCLoadedExtension *t_ext = MCextensions; t_ext != nil; t_ext = t_ext->next)
	{
		if (t_ext->module == p_module)
			return MCStringCopy(t_ext->resource_path != nil ? t_ext->resource_path : kMCEmptyString, r_resource_path);
	}
	
	return false;
}

void MCEngineExecLoadExtension(MCExecContext& ctxt, MCStringRef p_filename, MCStringRef p_resource_path)
{
    MCAutoStringRef t_resolved_filename;
    /* UNCHECKED */ MCS_resolvepath(p_filename, &t_resolved_filename);
    
    MCAutoDataRef t_data;
    if (!MCS_loadbinaryfile(*t_resolved_filename, &t_data))
        return;
    
    for(MCLoadedExtension *t_ext = MCextensions; t_ext != nil; t_ext = t_ext -> next)
        if (MCStringIsEqualTo(t_ext -> filename, *t_resolved_filename, kMCStringOptionCompareCaseless))
            return;
    
    MCStreamRef t_stream;
    /* UNCHECKED */ MCMemoryInputStreamCreate(MCDataGetBytePtr(*t_data), MCDataGetLength(*t_data), t_stream);
    
    MCScriptModuleRef t_module;
    if (!MCScriptCreateModuleFromStream(t_stream, t_module))
    {
        ctxt . SetTheResultToStaticCString("failed to load module");
        MCValueRelease(t_stream);
        return;
    }
    
    MCValueRelease(t_stream);
    
    MCEngineAddExtensionFromModule(*t_resolved_filename, t_module);
	if (p_resource_path != nil)
		MCEngineAddResourcePathForModule(t_module, p_resource_path);
    
    MCScriptReleaseModule(t_module);
    
    return;
}

void MCEngineExecUnloadExtension(MCExecContext& ctxt, MCStringRef p_filename)
{
    MCAutoStringRef t_resolved_filename;
    /* UNCHECKED */ MCS_resolvepath(p_filename, &t_resolved_filename);
    
    for(MCLoadedExtension *t_previous = nil, *t_ext = MCextensions; t_ext != nil; t_previous = t_ext, t_ext = t_ext -> next)
        if (MCStringIsEqualTo(t_ext -> filename, *t_resolved_filename, kMCStringOptionCompareCaseless))
        {
            if (t_ext -> instance != nil)
                MCScriptReleaseInstance(t_ext -> instance);
            MCScriptReleaseModule(t_ext -> module);
            MCValueRelease(t_ext -> filename);
			MCValueRelease(t_ext -> resource_path);
            if (t_previous != nil)
                t_previous -> next = t_ext -> next;
            else
                MCextensions = t_ext -> next;
            MCMemoryDelete(t_ext);
            
            MCextensionschanged = true;
            
            break;
        }
}

void MCEngineGetLoadedExtensions(MCExecContext& ctxt, MCProperListRef& r_list)
{
    bool t_success;
    t_success = true;
    
    MCProperListRef t_list;
    t_list = nil;
    if (t_success)
        t_success = MCProperListCreateMutable(t_list);
    
    for(MCLoadedExtension *t_ext = MCextensions; t_success && t_ext != nil; t_ext = t_ext -> next)
        t_success = MCProperListPushElementOntoBack(t_list, MCScriptGetNameOfModule(t_ext -> module));
    
    if (t_success)
        t_success = MCProperListCopyAndRelease(t_list, r_list);
    
    if (!t_success)
    {
        MCValueRelease(t_list);
        ctxt . Throw();
        return;
    }
}

bool MCEngineIterateExtensionFilenames(uintptr_t& x_iterator, MCStringRef& r_filename)
{
    if (x_iterator == 0)
        x_iterator = (uintptr_t)MCextensions;
    else
        x_iterator = (uintptr_t)(((MCLoadedExtension *)x_iterator) -> next);
    
    if (x_iterator == 0)
        return false;
    
    r_filename = ((MCLoadedExtension *)x_iterator) -> filename;
    
    return true;
}

Exec_stat MCEngineHandleLibraryMessage(MCNameRef p_message, MCParameter *p_parameters)
{
    if (MCextensionschanged)
        __rebuild_library_handler_list();
    
    if (MCextensionshandlermap == nil)
        return ES_NOT_HANDLED;
    
    MCForeignValueRef t_ptr;
    if (!MCArrayFetchValue(MCextensionshandlermap, false, p_message, (MCValueRef&)t_ptr))
        return ES_NOT_HANDLED;
    
    MCLoadedExtension *t_ext;
    t_ext = *(MCLoadedExtension **)MCForeignValueGetContentsPtr(t_ptr);
    
    MCTypeInfoRef t_signature;
    MCScriptQueryHandlerOfModule(t_ext -> module, p_message, t_signature);
    
    uindex_t t_arg_count;
    t_arg_count = MCHandlerTypeInfoGetParameterCount(t_signature);
    
    MCAutoArray<MCValueRef> t_arguments;
    
    bool t_success;
    t_success = true;
    
    MCParameter *t_param;
    t_param = p_parameters;
    for(uindex_t i = 0; i < t_arg_count && t_success; i++)
    {
        // Wrong number of parameters error.
        if (t_param == nil)
        {
            t_success = false;
            break;
        }
        
        if (MCHandlerTypeInfoGetParameterMode(t_signature, i) != kMCHandlerTypeFieldModeOut)
        {
            MCValueRef t_value;
            if (!t_param -> eval(*MCECptr, t_value))
            {
                t_success = false;
                break;
            }
            
            MCTypeInfoRef t_arg_type;
            t_arg_type = MCHandlerTypeInfoGetParameterType(t_signature, i);
            
            if (!MCExtensionConvertFromScriptType(*MCECptr, t_arg_type, t_value))
            {
                MCValueRelease(t_value);
                t_success = false;
                break;
            }
            
            if (!t_arguments . Push(t_value))
            {
                t_success = false;
                break;
            }
        }
        else if (!t_arguments . Push(nil))
        {
            t_success = false;
            break;
        }
        
        t_param = t_param -> getnext();
    }
    
    MCValueRef t_result;
    t_result = nil;
    if (MCScriptCallHandlerOfInstance(t_ext -> instance, p_message, t_arguments . Ptr(), t_arguments . Size(), t_result))
    {
        MCParameter *t_param;
        t_param = p_parameters;
        for(uindex_t i = 0; i < t_arg_count && t_success; i++)
        {
            MCHandlerTypeFieldMode t_mode;
            t_mode = MCHandlerTypeInfoGetParameterMode(t_signature, i);
            
            if (t_mode != kMCHandlerTypeFieldModeIn)
            {
                MCContainer *t_container;
                if (t_param -> evalcontainer(*MCECptr, t_container))
                {
                    if (!MCExtensionConvertToScriptType(*MCECptr, t_arguments[i]) ||
                        !t_container -> set(*MCECptr, t_arguments[i]))
                        t_success = false;
                    delete t_container;
                }
                else
                    t_param -> set_argument(*MCECptr, t_arguments[i]);
            }
            
            t_param = t_param -> getnext();
        }
        
        if (t_success &&
            !MCExtensionConvertToScriptType(*MCECptr, t_result))
        {
            t_success = false;
        }
        
        if (t_success)
            MCresult -> setvalueref(t_result);
        
        if (t_result != nil)
            MCValueRelease(t_result);
    }
    else
        t_success = false;
    
    for(uindex_t i = 0; i < t_arguments.Size(); i++)
        if (t_arguments[i] != nil)
            MCValueRelease(t_arguments[i]);
    
    // If we failed, then catch the error and create a suitable MCerror unwinding.
    if (t_success)
        return ES_NORMAL;
    
    return MCExtensionCatchError(*MCECptr);
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCExtensionCatchError(MCExecContext& ctxt)
{
    MCAutoErrorRef t_error;
    if (!MCErrorCatch(&t_error))
    {
        MCLog("Error state indicated with no error having been thrown", 0);
        return ES_ERROR;
    }
    
    ctxt . LegacyThrow(EE_EXTENSION_ERROR_DOMAIN, MCErrorGetDomain(*t_error));
    ctxt . LegacyThrow(EE_EXTENSION_ERROR_DESCRIPTION, MCErrorGetMessage(*t_error));
    if (MCErrorGetDepth(*t_error) > 0)
    {
        ctxt . LegacyThrow(EE_EXTENSION_ERROR_FILE, MCErrorGetTargetAtLevel(*t_error, 0));
        ctxt . LegacyThrow(EE_EXTENSION_ERROR_LINE, MCErrorGetRowAtLevel(*t_error, 0));
    }
    
    return ES_ERROR;
}

static bool __script_try_to_convert_to_boolean(MCExecContext& ctxt, bool p_optional, MCValueRef& x_value, bool& r_converted);
static bool __script_try_to_convert_to_number(MCExecContext& ctxt, bool p_optional, MCValueRef& x_value, bool& r_converted);
static bool __script_try_to_convert_to_string(MCExecContext& ctxt, MCValueRef& x_value, bool& r_converted);
static bool __script_try_to_convert_to_data(MCExecContext& ctxt, MCValueRef& x_value, bool& r_converted);
static bool __script_try_to_convert_to_array(MCExecContext& ctxt, MCValueRef& x_value, bool& r_converted);
static bool __script_try_to_convert_to_list(MCExecContext& ctxt, MCValueRef& x_value, bool& r_converted);
static bool __script_try_to_convert_to_record(MCExecContext& ctxt, MCTypeInfoRef p_type, MCValueRef& x_value, bool& r_converted);
static bool __script_try_to_convert_to_foreign(MCExecContext& ctxt, MCTypeInfoRef p_type, MCValueRef& x_value, bool& r_converted);

static bool MCExtensionThrowUnrepresentableValueError(MCValueRef p_value)
{
    return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("unrepresentable value"), nil);
}

static bool MCExtensionThrowTypeConversionError(MCTypeInfoRef p_type, MCValueRef p_value)
{
    return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("cannot convert value"), nil);
}

// This methods translates a value from the extension world to one which is
// usable in the scripting world.
//
// The mapping is as follows:
//   Null -> Null
//   Boolean -> Boolean
//   Number -> Number
//   String -> String
//   Name -> Name
//   Data -> Data
//   ProperList -> Array with numeric keys starting at 1
//   Record -> Array with keys the same as the record's fields
//   ForeignValue -> error
//   Handler -> error
//   Error -> error
//   Custom -> error
//
// Any conversion errors are thrown using MCErrorThrow.
//
bool MCExtensionConvertToScriptType(MCExecContext& ctxt, MCValueRef& x_value)
{
    switch(MCValueGetTypeCode(x_value))
    {
        // These map across with no conversion.
        case kMCValueTypeCodeNull:
        case kMCValueTypeCodeBoolean:
        case kMCValueTypeCodeNumber:
        case kMCValueTypeCodeString:
        case kMCValueTypeCodeName:
        case kMCValueTypeCodeData:
            return true;
            
        // ProperLists map to sequences (arrays with numeric keys)
        case kMCValueTypeCodeProperList:
        {
            MCArrayRef t_array;
            if (!MCArrayCreateMutable(t_array))
                return false;
            for(uindex_t i = 0; i < MCProperListGetLength((MCProperListRef)x_value); i++)
                if (!MCArrayStoreValueAtIndex(t_array, i + 1, MCProperListFetchElementAtIndex((MCProperListRef)x_value, i)))
                {
                    MCValueRelease(t_array);
                    return false;
                }
            if (!MCArrayCopyAndRelease(t_array, t_array))
            {
                MCValueRelease(t_array);
                return false;
            }
            MCValueRelease(x_value);
            x_value = t_array;
        }
        return true;
            
        // Records map to arrays
        case kMCValueTypeCodeRecord:
        {
            MCArrayRef t_array;
            if (!MCArrayCreateMutable(t_array))
                return false;
            
            MCNameRef t_field_name;
            MCValueRef t_field_value;
            uintptr_t t_iterator;
            t_iterator = 0;
            while(MCRecordIterate((MCRecordRef)x_value, t_iterator, t_field_name, t_field_value))
            {
                if (!MCExtensionConvertToScriptType(ctxt, t_field_value))
                {
                    // TODO: Augment error
                    MCValueRelease(t_array);
                    return false;
                }
                
                if (!MCArrayStoreValue(t_array, false, t_field_name, t_field_value))
                {
                    MCValueRelease(t_array);
                    return false;
                }
            }
            
            if (!MCArrayCopyAndRelease(t_array, t_array))
            {
                MCValueRelease(t_array);
                return false;
            }
            
            MCValueRelease(x_value);
            x_value = t_array;
        }
        return true;
            
        // The rest have no conversion
        default:
            break;
    }
    
    return MCExtensionThrowUnrepresentableValueError(x_value);
}

// This method translates a value in the script world to one of a specified type
// in the extension world. The translation follows the normal script type-conversion
// rules.
//
// We use our own versions of conversion to ensure error handling is appropriate.
// (In particular, we must distinguish between not converting because of a type
// error and not converting because of some other error - i.e. oom).
bool MCExtensionTryToConvertFromScriptType(MCExecContext& ctxt, MCTypeInfoRef p_as_type, MCValueRef& x_value, bool& r_converted)
{
    MCResolvedTypeInfo t_resolved_type;
    MCTypeInfoResolve(p_as_type, t_resolved_type);
    
    bool t_converted;
    if (t_resolved_type . named_type == kMCBooleanTypeInfo)
    {
        if (!__script_try_to_convert_to_boolean(ctxt, t_resolved_type . is_optional, x_value, r_converted))
            return false;
    }
    else if (t_resolved_type . named_type == kMCNumberTypeInfo)
    {
        if (!__script_try_to_convert_to_number(ctxt, t_resolved_type . is_optional, x_value, r_converted))
            return false;
    }
    else if (t_resolved_type . named_type == kMCStringTypeInfo)
    {
        if (!__script_try_to_convert_to_string(ctxt, x_value, r_converted))
            return false;
    }
    else if (t_resolved_type . named_type == kMCDataTypeInfo)
    {
        if (!__script_try_to_convert_to_data(ctxt, x_value, r_converted))
            return false;
    }
    else if (t_resolved_type . named_type == kMCArrayTypeInfo)
    {
        if (!__script_try_to_convert_to_array(ctxt, x_value, r_converted))
            return false;
    }
    else if (t_resolved_type . named_type == kMCProperListTypeInfo)
    {
        if (!__script_try_to_convert_to_list(ctxt, x_value, r_converted))
            return false;
    }
    else if (MCTypeInfoIsRecord(t_resolved_type . named_type))
    {
        if (!__script_try_to_convert_to_record(ctxt, t_resolved_type . named_type, x_value, r_converted))
            return false;
    }
    else if (MCTypeInfoIsForeign(t_resolved_type . named_type))
    {
        if (!__script_try_to_convert_to_foreign(ctxt, t_resolved_type . named_type, x_value, r_converted))
            return false;
    }
    
    return true;
}

bool MCExtensionConvertFromScriptType(MCExecContext& ctxt, MCTypeInfoRef p_as_type, MCValueRef& x_value)
{
    bool t_converted;
    if (!MCExtensionTryToConvertFromScriptType(ctxt, p_as_type, x_value, t_converted))
        return false;
    
    if (t_converted)
        return true;
    
    return MCExtensionThrowTypeConversionError(p_as_type, x_value);
}

// Convert a script value to a boolean following standard lax-scripting rules.
static bool __script_try_to_convert_to_boolean(MCExecContext& ctxt, bool p_optional, MCValueRef& x_value, bool& r_converted)
{
    // If it is already a boolean, we are done.
    if (MCValueGetTypeCode(x_value) == kMCValueTypeCodeBoolean)
    {
        r_converted = true;
        return true;
    }
    
    // Try to convert to a string.
    bool t_is_string;
    if (!__script_try_to_convert_to_string(ctxt, x_value, t_is_string))
        return false;
    
    // If we succeeded in getting a string, see if it is suitable.
    if (t_is_string)
    {
        // If the value is the empty string and we are optional, then assign null.
        if (MCStringIsEmpty((MCStringRef)x_value) && p_optional)
        {
            MCValueAssign((MCNullRef&)x_value, kMCNull);
            return true;
        }
     
        // If the string is 'true', then assign true.
        if (MCStringIsEqualTo((MCStringRef)x_value, kMCTrueString, kMCStringOptionCompareCaseless))
        {
            MCValueAssign((MCBooleanRef&)x_value, kMCTrue);
            return true;
        }
        
        // If the string is 'false', then assign false.
        if (MCStringIsEqualTo((MCStringRef)x_value, kMCFalseString, kMCStringOptionCompareCaseless))
        {
            MCValueAssign((MCBooleanRef&)x_value, kMCFalse);
            return true;
        }
    }
    
    // We failed to convert.
    r_converted = false;
    return true;
}

// Convert a script value to a number following standard lax-scripting rules.
static bool __script_try_to_convert_to_number(MCExecContext& ctxt, bool p_optional, MCValueRef& x_value, bool& r_converted)
{
    // If it is already a number, we are done.
    if (MCValueGetTypeCode(x_value) == kMCValueTypeCodeNumber)
        return true;
    
    // Otherwise convert to a string and attempt to parse.
    bool t_is_string;
    if (!__script_try_to_convert_to_string(ctxt, x_value, t_is_string))
        return false;
    
    // If converted is true then we have a string.
    if (t_is_string)
    {
        // If the value is the empty string and we are optional, then assign null.
        if (MCStringIsEmpty((MCStringRef)x_value) && p_optional)
        {
            MCValueAssign((MCNullRef&)x_value, kMCNull);
            return true;
        }
        
        // Now try to convert the string to a number.
        double t_numeric_value;
        if (MCTypeConvertStringToReal((MCStringRef)x_value, t_numeric_value, ctxt . GetConvertOctals()))
        {
            MCValueRef t_number;
            if (!MCNumberCreateWithReal(t_numeric_value, (MCNumberRef&)t_number))
                return false;
            
            MCValueRelease(x_value);
            x_value = t_number;
            return true;
        }
    }
    
    // We failed to convert.
    r_converted = false;
    return true;
}

static bool __script_try_to_convert_to_string(MCExecContext& ctxt, MCValueRef& x_value, bool& r_converted)
{
    // If it is already a string, we are done.
    if (MCValueGetTypeCode(x_value) == kMCValueTypeCodeString)
    {
        r_converted = true;
        return true;
    }
    
    // If it is a name, then assign the name's string.
    if (MCValueGetTypeCode(x_value) == kMCValueTypeCodeName)
    {
        MCValueAssign((MCStringRef&)x_value, MCNameGetString((MCNameRef)x_value));
        r_converted = true;
        return true;
    }
    
    // If it is a binary string, then decode as native.
    if (MCValueGetTypeCode(x_value) == kMCValueTypeCodeData)
    {
        if (!MCStringDecodeAndRelease((MCDataRef)x_value, kMCStringEncodingNative, false, (MCStringRef&)x_value))
            return false;
        r_converted = true;
        return true;
    }

    // If it is a boolean, then return the appropriate string.
    if (MCValueGetTypeCode(x_value) == kMCValueTypeCodeBoolean)
    {
        MCValueAssign((MCStringRef&)x_value, x_value == kMCTrue ? kMCTrueString : kMCFalseString);
        r_converted = true;
        return true;
    }
    
    // If it is a number then, format it as a string.
    if (MCValueGetTypeCode(x_value) == kMCValueTypeCodeNumber)
    {
        char t_backing_buffer[R8L];
        char *t_buffer;
        uint4 t_length;
        t_buffer = t_backing_buffer;
        t_length = R8L;
        t_length = MCU_r8tos(t_buffer, t_length, MCNumberFetchAsReal((MCNumberRef)x_value), ctxt . GetNumberFormatWidth(), ctxt . GetNumberFormatTrailing(), ctxt . GetNumberFormatForce());
        
        MCStringRef t_string;
        if (!MCStringCreateWithNativeChars((const char_t *)t_buffer, t_length, t_string))
            return false;
        
        MCValueRelease(x_value);
        x_value = t_string;
        r_converted = true;
        return true;
    }
    
    // If it is an array, then decode as the empty string.
    if (MCValueGetTypeCode(x_value) == kMCValueTypeCodeArray)
    {
        MCValueAssign((MCStringRef&)x_value, kMCEmptyString);
        r_converted = true;
        return true;
    }
    
    // If it is null, then decode as the empty string.
    if (MCValueGetTypeCode(x_value) == kMCValueTypeCodeNull)
    {
        MCValueAssign((MCStringRef&)x_value, kMCEmptyString);
        r_converted = true;
        return true;
    }
    
    // We failed to convert.
    r_converted = false;
    return true;
}

static bool __script_try_to_convert_to_data(MCExecContext& ctxt, MCValueRef& x_value, bool& r_converted)
{
    // If it is already a data, we are done.
    if (MCValueGetTypeCode(x_value) == kMCValueTypeCodeData)
    {
        r_converted = true;
        return true;
    }
    
    // Otherwise try to convert to a string.
    bool t_is_string;
    if (!__script_try_to_convert_to_string(ctxt, x_value, t_is_string))
        return false;
    
    // If we now have a string, then we can make it data.
    if (t_is_string)
    {
        MCDataRef t_data;
        if (!MCDataConvertStringToData((MCStringRef)x_value, t_data))
            return false;
        
        MCValueRelease(x_value);
        x_value = t_data;
        r_converted = true;
        return true;
    }
    
    // We failed to convert.
    r_converted = false;
    return true;
}

static bool __script_try_to_convert_to_array(MCExecContext& ctxt, MCValueRef& x_value, bool& r_converted)
{
    // If it is already an array, we are done.
    if (MCValueGetTypeCode(x_value) == kMCValueTypeCodeArray)
    {
        r_converted = true;
        return true;
    }
    
    // Otherwise try to convert to a string.
    bool t_is_string;
    if (!__script_try_to_convert_to_string(ctxt, x_value, t_is_string))
        return false;
    
    // If we managed to convert to a string, then it becomes the empty
    // array.
    if (t_is_string)
    {
        MCValueAssign((MCArrayRef&)x_value, kMCEmptyArray);
        r_converted = true;
        return true;
    }
    
    // We failed to convert.
    r_converted = false;
    return true;
}

static bool __script_try_to_convert_to_list(MCExecContext& ctxt, MCValueRef& x_value, bool& r_converted)
{
    // If we are already a proper list then we are done.
    // (This case should never be hit at the moment as script world doesn't
    // do properlists!).
    if (MCValueGetTypeCode(x_value) == kMCValueTypeCodeProperList)
    {
        r_converted = true;
        return true;
    }
    
    // Otherwise we try to convert to an array.
    bool t_is_array;
    if (!__script_try_to_convert_to_array(ctxt, x_value, t_is_array))
        return false;
    
    // If we managed to convert to an array, and the array is a sequence
    // we can convert.
    if (t_is_array && MCArrayIsSequence((MCArrayRef)x_value))
    {
        MCProperListRef t_proper_list;
        if (!MCProperListCreateMutable(t_proper_list))
            return false;
        for(uindex_t i = 0; i < MCArrayGetCount((MCArrayRef)x_value); i++)
        {
            // We know this will succeed as we have a sequence.
            MCValueRef t_element;
            MCArrayFetchValueAtIndex((MCArrayRef)x_value, i + 1, t_element);
            if (!MCProperListPushElementOntoBack(t_proper_list, t_element))
            {
                MCValueRelease(t_proper_list);
                return false;
            }
        }
        if (!MCProperListCopyAndRelease(t_proper_list, t_proper_list))
        {
            MCValueRelease(t_proper_list);
            return false;
        }
        MCValueRelease(x_value);
        x_value = t_proper_list;
        r_converted = true;
        return true;
    }
    
    // We failed to convert.
    r_converted = false;
    return true;
}

static bool __script_try_to_convert_to_record(MCExecContext& ctxt, MCTypeInfoRef p_type, MCValueRef& x_value, bool& r_converted)
{
    // If the value is already of the given type, then we are done.
    // (This case should never be hit at the moment as script world doesn't
    // do records!).
    if (MCValueGetTypeInfo(x_value) == p_type)
    {
        r_converted = true;
        return true;
    }
    
    // Otherwise we try to convert to an array.
    bool t_is_array;
    if (!__script_try_to_convert_to_array(ctxt, x_value, t_is_array))
        return false;
    
    // If we managed to convert to an array, then we attempt to convert to
    // a record.
    if (t_is_array)
    {
        uindex_t t_field_count;
        t_field_count = MCRecordTypeInfoGetFieldCount(p_type);
        
        // If the number of fields in the array is the same as that in the record
        // then we can continue.
        if (MCArrayGetCount((MCArrayRef)x_value) == t_field_count)
        {
            MCRecordRef t_record;
            if (!MCRecordCreateMutable(p_type, t_record))
                return false;
            
            for(uindex_t i = 0; i < t_field_count; i++)
            {
                MCNameRef t_field_name;
                t_field_name = MCRecordTypeInfoGetFieldName(p_type, i);
                
                // If the given field isn't in the array, we can't convert.
                MCValueRef t_field_value;
                if (!MCArrayFetchValue((MCArrayRef)x_value, false, t_field_name, t_field_value))
                {
                    MCValueRelease(t_record);
                    r_converted = false;
                    return true;
                }
                
                MCTypeInfoRef t_field_type;
                t_field_type = MCRecordTypeInfoGetFieldType(p_type, i);
                
                // If the given field's value can't be converted to the target type,
                // we can't convert. (Note that we retain the field value as TryToConvert
                // will convert in place if it can).
                MCValueRetain(t_field_value);
                
                // If conversion throws an error, propagate.
                bool t_is_field_type;
                if (!MCExtensionTryToConvertFromScriptType(ctxt, t_field_type, t_field_value, t_is_field_type))
                {
                    MCValueRelease(t_field_value);
                    MCValueRelease(t_record);
                    return false;
                }
                
                // If it is not the field type, conversion failed.
                if (!t_is_field_type)
                {
                    MCValueRelease(t_field_value);
                    MCValueRelease(t_record);
                    r_converted = false;
                    return true;
                }
                
                // Finally, try to store the value in the record.
                if (!MCRecordStoreValue(t_record, t_field_name, t_field_value))
                {
                    MCValueRelease(t_field_value);
                    MCValueRelease(t_record);
                    return false;
                }
                
                MCValueRelease(t_field_value);
            }
            
            if (!MCRecordCopyAndRelease(t_record, t_record))
            {
                MCValueRelease(t_record);
                return false;
            }
            
            MCValueRelease(x_value);
            x_value = t_record;
            
            r_converted = true;
            return true;
        }
    }
    
    r_converted = false;
    return true;
}

static bool __script_try_to_convert_to_foreign(MCExecContext& ctxt, MCTypeInfoRef p_type, MCValueRef& x_value, bool& r_converted)
{
    // If the value is already of the given type, then we are done.
    // (This case should never be hit at the moment as script world doesn't
    // do foreign values!).
    if (MCValueGetTypeInfo(x_value) == p_type)
    {
        r_converted = true;
        return true;
    }
    
    // Fetch the type descriptor so we can check for a bridging type.
    const MCForeignTypeDescriptor *t_descriptor;
    t_descriptor = MCForeignTypeInfoGetDescriptor(p_type);
    
    // If the briding type is null, then there is no bridge.
    if (t_descriptor -> bridgetype == kMCNullTypeInfo)
    {
        r_converted = false;
        return true;
    }
    
    // Otherwise, attempt to conver to the bridging type.
    bool t_is_bridge;
    if (!MCExtensionTryToConvertFromScriptType(ctxt, t_descriptor -> bridgetype, x_value, t_is_bridge))
        return false;
    
    // If we don't have the bridging type we can't convert.
    if (!t_is_bridge)
    {
        r_converted = false;
        return true;
    }
    
    // Otherwise we can now try to export.
    MCValueRef t_foreign_value;
    if (!MCForeignValueExport(p_type, x_value, (MCForeignValueRef&)t_foreign_value))
        return false;
    
    // If we got this far, we succeeded!
    MCValueRelease(x_value);
    x_value = t_foreign_value;
    
    r_converted = true;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
