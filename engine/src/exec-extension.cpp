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

#include "libscript/script.h"
#include "libscript/script-auto.h"

////////////////////////////////////////////////////////////////////////////////

struct MCLoadedExtension
{
    MCLoadedExtension *next;
    MCNameRef module_name;
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
            MCScriptListHandlerNamesOfModule(t_ext -> module, &t_handlers);
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

bool MCEngineAddExtensionFromModule(MCScriptModuleRef p_module)
{
    if (!MCScriptEnsureModuleIsUsable(p_module))
    {
        MCAutoErrorRef t_error;
        if (MCErrorCatch(&t_error))
        {
            MCresult -> setvalueref(MCErrorGetMessage(*t_error));
        }
        else
        {
            MCresult -> sets("module is not usable");
        }
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
    if (!MCMemoryNew(t_ext))
    {
      return false;
    }
    
    t_ext -> module_name = MCValueRetain(MCScriptGetNameOfModule(p_module));
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

static bool
MCEngineCheckModulesHaveNamePrefix(MCNameRef p_prefix,
                                   MCSpan<MCScriptModuleRef> p_modules)
{
    MCAutoStringRef t_prefix;
    if (!(MCStringMutableCopy(MCNameGetString(p_prefix), &t_prefix) &&
          MCStringAppendChar(*t_prefix, '.')))
        return false;

    for (MCScriptModuleRef t_iter : p_modules)
    {
        if (!MCStringBeginsWith(MCNameGetString(MCScriptGetNameOfModule(t_iter)),
                                *t_prefix, kMCStringOptionCompareCaseless))
            return false;
    }
    return true;
}

void MCEngineAddExtensionsFromModulesArray(MCAutoScriptModuleRefArray& p_modules, MCStringRef p_resource_path, MCStringRef& r_error)
{
    MCScriptModuleRef t_main = p_modules[0];
    
    /* Check that the 2nd to Nth modules have names that have the
     * appropriate prefix */
    if (!MCEngineCheckModulesHaveNamePrefix(MCScriptGetNameOfModule(t_main),
                                            p_modules.Span().subspan(1)))
    {
        MCAutoStringRef t_message;
        /* It's probably okay not to check this; failures will be
         * dealt with by the following MCErrorCatch(). */
        /*UNCHECKED*/ MCStringFormat(&t_message,
                                     "failed to load modules: support modules' names did not begin with '%@'",
                                     MCScriptGetNameOfModule(t_main));
        
        MCAutoErrorRef t_error;
        if (MCErrorCatch(&t_error))
        {
            r_error = MCValueRetain(MCErrorGetMessage(*t_error));
        }
        else
        {
            r_error = MCValueRetain(*t_message);
        }
        return;
    }
    
    /* Only the head module is registered as an extension */
    MCEngineAddExtensionFromModule(t_main);
    
    if (p_resource_path != nullptr)
        MCEngineAddResourcePathForModule(t_main, p_resource_path);
}

void MCEngineLoadExtensionFromData(MCExecContext& ctxt, MCDataRef p_extension_data, MCStringRef p_resource_path)
{
    if (!MCSecureModeCanAccessExtension())
    {
        ctxt . SetTheResultToStaticCString("no permission to load module");
        return;
    }
    
    MCAutoScriptModuleRefArray t_modules;
    if (!MCScriptCreateModulesFromData(p_extension_data, t_modules))
    {
        MCAutoErrorRef t_error;
        if (MCErrorCatch(Out(t_error)))
            ctxt . SetTheResultToValue(MCErrorGetMessage(In(t_error)));
        else
            ctxt . SetTheResultToStaticCString("failed to load module");
        return;
    }

    MCAutoStringRef t_error;
    MCEngineAddExtensionsFromModulesArray(t_modules, p_resource_path, &t_error);
    if (*t_error != nullptr)
    {
        ctxt.SetTheResultToValue(*t_error);
    }
}

// This is the callback given to libscript so that it can resolve the absolute
// path of native code libraries used by foreign handlers in the module.

static bool MCEngineLoadLibrary(MCScriptModuleRef p_module, MCStringRef p_name, MCSLibraryRef& r_library)
{
    // Extension libraries should be mapped by the IDE or deploy params
    // We first check <module_name>/<lib_name> so that two modules
    // can use distinct code resources with the same name
    MCNameRef t_module_name = MCScriptGetNameOfModule(p_module);
    MCAutoStringRef t_module_lib_name;
    if (!MCStringFormat(&t_module_lib_name, "%@/%@", t_module_name, p_name))
        return false;
    
    MCAutoStringRef t_map_name;
    bool t_has_mapping = false;
    if (MCdispatcher->haslibrarymapping(*t_module_lib_name))
    {
        t_has_mapping = true;
        if (!MCStringFormat(&t_map_name, "./%@", *t_module_lib_name))
            return false;
    }
    else if (MCdispatcher->haslibrarymapping(p_name))
    {
        t_has_mapping = true;
        if (!MCStringFormat(&t_map_name, "./%@", p_name))
            return false;
    }
    else
    {
        t_map_name = p_name;
    }
    
    MCSLibraryRef t_library = MCU_library_load(*t_map_name);
        
    // there was a mapping and it failed to load
    if (t_has_mapping && t_library == nullptr)
    {
        return false;
    }
    
#if defined(__IOS__)
    // On iOS we fallback to the engine because with the exception of
    // dynamic frameworks libraries will be static linked
    if (t_library == nullptr)
    {
        t_library = MCValueRetain(MCScriptGetLibrary());
    }
#endif

    if (t_library == nullptr)
    {
        return false;
    }
    
    r_library = t_library;
    
    return true;
}

void MCEngineExecLoadExtension(MCExecContext& ctxt, MCStringRef p_filename, MCStringRef p_resource_path)
{
    ctxt . SetTheResultToEmpty();
    
    MCAutoStringRef t_resolved_filename;
    if (!MCS_resolvepath(p_filename, &t_resolved_filename))
        return;
    
    MCAutoDataRef t_data;
    if (!MCS_loadbinaryfile(*t_resolved_filename, &t_data))
        return;
    
    MCEngineLoadExtensionFromData(ctxt, *t_data, p_resource_path);
 }

/* This function frees the given loaded extension. It must have already been
 * removed from the global MCextensions lists. */
static void
__MCEngineFreeExtension(MCLoadedExtension *p_extension)
{
    if (p_extension -> instance != nil)
        MCScriptReleaseInstance(p_extension -> instance);
    MCScriptReleaseModule(p_extension -> module);
    MCValueRelease(p_extension -> module_name);
    MCValueRelease(p_extension -> resource_path);
    MCMemoryDelete(p_extension);
}

void MCEngineExecUnloadExtension(MCExecContext& ctxt, MCStringRef p_module_name)
{
    MCNewAutoNameRef t_name;
    if (!MCNameCreate(p_module_name, &t_name))
    {
        ctxt.Throw();
        return;
    }
    
    for(MCLoadedExtension *t_previous = nil, *t_ext = MCextensions; t_ext != nil; t_previous = t_ext, t_ext = t_ext -> next)
        if (MCNameIsEqualToCaseless(t_ext -> module_name, *t_name))
        {
            bool t_in_use = false;
            
            if (MCScriptIsModuleALibrary(t_ext -> module))
            {
                // If the module is a library module, then if it is not in
                // use by another module it will have a reference count of
                // 2 - one for the module ref in the loaded extensions list
                // and one for the singleton instance.
                if (2 != MCScriptGetRetainCountOfModule(t_ext -> module))
                {
                    t_in_use = true;
                }
            }
            else
            {
                // If the module is a widget or non-public module then it
                // can only be unloaded if it is not in use in a widget
                // instance, or used by an instance.
                if (1 != MCScriptGetRetainCountOfModule(t_ext -> module))
                {
                    t_in_use = true;
                }
            }
                
            if (t_in_use)
			{
                
				ctxt . SetTheResultToCString("module in use");
				return;
			}
            
            /* Unlink the extension from the global linked-list */
            if (t_previous != nil)
                t_previous -> next = t_ext -> next;
            else
                MCextensions = t_ext -> next;
            
            /* Makes sure the global handler list is refreshed on next use */
            MCextensionschanged = true;
            
            /* Free the extension struct and things it owns */
            __MCEngineFreeExtension(t_ext);
            
			return;
        }
	
	// If we get here the module was not found.
	ctxt . SetTheResultToCString("module not loaded");
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
        t_success = MCProperListPushElementOntoBack(t_list, t_ext -> module_name);
    
    if (t_success)
        t_success = MCProperListCopyAndRelease(t_list, r_list);
    
    if (!t_success)
    {
        MCValueRelease(t_list);
        ctxt . Throw();
        return;
    }
}

Exec_stat MCEngineHandleLibraryMessage(MCNameRef p_message, MCParameter *p_parameters)
{
    if (MCextensionschanged)
	{
        __rebuild_library_handler_list();
		MCextensionschanged = false;
	}
	
    if (MCextensionshandlermap == nil)
        return ES_NOT_HANDLED;
    
    MCForeignValueRef t_ptr;
    if (!MCArrayFetchValue(MCextensionshandlermap, false, p_message, (MCValueRef&)t_ptr))
        return ES_NOT_HANDLED;
    
    MCLoadedExtension *t_ext;
    t_ext = *(MCLoadedExtension **)MCForeignValueGetContentsPtr(t_ptr);
    
    MCTypeInfoRef t_signature;
    MCScriptQueryHandlerSignatureOfModule(t_ext -> module, p_message, t_signature);
    
    uindex_t t_arg_count;
    t_arg_count = MCHandlerTypeInfoGetParameterCount(t_signature);
    
    MCAutoArray<MCValueRef> t_arguments;
    
    bool t_success;
    t_success = true;
    
    MCParameter *t_param;
    t_param = p_parameters;
    for(uindex_t i = 0; i < t_arg_count && t_success; i++)
    {
        // Too few parameters error.
        if (t_param == nil)
        {
			MCECptr -> LegacyThrow(EE_INVOKE_TOOFEWARGS);
            t_success = false;
            break;
        }
        
        if (MCHandlerTypeInfoGetParameterMode(t_signature, i) != kMCHandlerTypeFieldModeOut)
        {
            MCValueRef t_value;
            if (!t_param -> eval_argument(*MCECptr, t_value))
            {
                t_success = false;
                break;
            }
            
            MCTypeInfoRef t_arg_type;
            t_arg_type = MCHandlerTypeInfoGetParameterType(t_signature, i);
            
            if (!MCExtensionConvertFromScriptType(*MCECptr, t_arg_type, t_value))
            {
                MCECptr->LegacyThrow(EE_INVOKE_TYPEERROR);
                MCValueRelease(t_value);
                t_success = false;
                break;
            }
            
            if (!t_arguments . Push(t_value))
            {
                MCValueRelease(t_value);
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
    
	// Too many parameters error.
	if (t_success &&
        t_param != nil)
	{
		MCECptr -> LegacyThrow(EE_INVOKE_TOOMANYARGS);
		t_success = false;
	}
	
    MCValueRef t_result;
    t_result = nil;
    if (t_success &&
        MCScriptCallHandlerInInstance(t_ext -> instance, p_message, t_arguments . Ptr(), t_arguments . Size(), t_result))
    {
        t_param = p_parameters;
        for(uindex_t i = 0; i < t_arg_count && t_success; i++)
        {
            MCHandlerTypeFieldMode t_mode;
            t_mode = MCHandlerTypeInfoGetParameterMode(t_signature, i);
            
            if (t_mode != kMCHandlerTypeFieldModeIn)
            {
                MCContainer t_container;
                if (t_param -> evalcontainer(*MCECptr, t_container))
                {
                    if (!MCExtensionConvertToScriptType(*MCECptr, t_arguments[i]) ||
                        !t_container.set(*MCECptr, t_arguments[i]))
                        t_success = false;
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
	
	// If the exec context is already in error, use that.
	if (MCECptr -> HasError())
		return ES_ERROR;
	
    return MCExtensionCatchError(*MCECptr);
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCExtensionCatchError(MCExecContext& ctxt)
{
    MCAutoErrorRef t_error;
    if (!MCErrorCatch(&t_error))
    {
        MCLog("Error state indicated with no error having been thrown");
        return ES_ERROR;
    }

	uindex_t t_num_frames = MCErrorGetDepth(*t_error);
	for (uindex_t t_depth = 0; t_depth < t_num_frames; ++t_depth)
	{
		if (t_depth == 0)
		{
			ctxt . LegacyThrow(EE_EXTENSION_ERROR_DOMAIN, MCErrorGetDomain(*t_error));
			ctxt . LegacyThrow(EE_EXTENSION_ERROR_DESCRIPTION, MCErrorGetMessage(*t_error));
		}
		ctxt . LegacyThrow(EE_EXTENSION_ERROR_FILE, MCErrorGetTargetAtLevel(*t_error, t_depth));
		ctxt . LegacyThrow(EE_EXTENSION_ERROR_LINE, MCErrorGetRowAtLevel(*t_error, t_depth));
		ctxt . LegacyThrow(EE_EXTENSION_ERROR_COLUMN, MCErrorGetColumnAtLevel(*t_error, t_depth));
	}
    
    return ES_ERROR;
}

static bool __script_ensure_names_are_strings(MCValueRef p_input, MCValueRef& r_output);
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
        
        case kMCValueTypeCodeArray:
        {
            // We start off with no new value - we only create a new array if one
            // of the elements changes.
            MCAutoArrayRef t_mutated_value;
            
            MCNameRef t_key;
            MCValueRef t_element;
            uintptr_t t_iterator;
            t_iterator = 0;
            while(MCArrayIterate((MCArrayRef)x_value, t_iterator, t_key, t_element))
            {
                // 'Copy' the value (as we don't own it).
                MCAutoValueRef t_new_element;
                t_new_element = t_element;
                
                // Attempt to convert it to a script type.
                if (!MCExtensionConvertToScriptType(ctxt, InOut(t_new_element)))
                    return false;
                
                // If the value has changed then we must do a store.
                if (*t_new_element != t_element)
                {
                    // If we haven't had to change anything yet, we must copy the original
                    // array.
                    if (*t_mutated_value == nil)
                    {
                        if (!MCArrayMutableCopy((MCArrayRef)x_value, Out(t_mutated_value)))
                            return false;
                    }
                    
                    // Store the element in the array.
                    if (!MCArrayStoreValue(In(t_mutated_value), true, t_key, In(t_new_element)))
                        return false;
                    
                    continue;
                }
    
                // If we get here then we haven't had to mutate the input array yet
                // and this element has not changed through conversion so there is
                // nothing to do!
                
                continue;
            }
            
            // If we get here then all is well. If we had to mutate the input value
            // then return it in x_value (after releasing the original value since
            // it is an inout).
            if (*t_mutated_value != nil)
                MCValueAssignAndRelease(x_value, (MCValueRef)t_mutated_value . Take());
        }
        return true;
            
        // ProperLists map to sequences (arrays with numeric keys)
        case kMCValueTypeCodeProperList:
        {
            MCArrayRef t_array;
            if (!MCArrayCreateMutable(t_array))
                return false;
            for(uindex_t i = 0; i < MCProperListGetLength((MCProperListRef)x_value); i++)
            {
                // 'Copy' the value (as we don't own it).
                MCAutoValueRef t_new_element;
                t_new_element = MCProperListFetchElementAtIndex((MCProperListRef)x_value, i);
                
                // Attempt to convert it to a script type.
                if (!MCExtensionConvertToScriptType(ctxt, InOut(t_new_element)))
                {
                    MCValueRelease(t_array);
                    return false;
                }
                
                if (!MCArrayStoreValueAtIndex(t_array, i + 1, *t_new_element))
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
            
        case kMCValueTypeCodeForeignValue:
        {
            // Get the type info for the foreign value so we can find its bridge
            // type
            MCTypeInfoRef t_foreign_type = MCValueGetTypeInfo(x_value);
            const MCForeignTypeDescriptor* t_desc = MCForeignTypeInfoGetDescriptor(t_foreign_type);
            if (t_desc->bridgetype == nil)
                break;

            // Import the type
            MCValueRef t_imported;
            if (!t_desc->doimport(t_desc, MCForeignValueGetContentsPtr(x_value), false, t_imported))
                return false;

            // Recursively convert to a script type
            if (!MCExtensionConvertToScriptType(ctxt, t_imported))
            {
                MCValueRelease(t_imported);
                return false;
            }
            
            MCValueAssignAndRelease(x_value, t_imported);
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
    if (!MCTypeInfoResolve(p_as_type, t_resolved_type))
		return false;
    
    if (t_resolved_type . named_type == kMCAnyTypeInfo)
    {
        MCValueRef t_revised_element;
        if (!__script_ensure_names_are_strings(x_value, t_revised_element))
            return false;
        
        if (t_revised_element != nil)
            MCValueAssignAndRelease(x_value, t_revised_element);
        
        r_converted = true;
    }
    else if (t_resolved_type . named_type == kMCBooleanTypeInfo)
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
    else if (MCTypeInfoIsRecord(t_resolved_type . type))
    {
        if (!__script_try_to_convert_to_record(ctxt, t_resolved_type . named_type, x_value, r_converted))
            return false;
    }
    else if (MCTypeInfoIsForeign(t_resolved_type . type))
    {
        if (!__script_try_to_convert_to_foreign(ctxt, t_resolved_type . named_type, x_value, r_converted))
            return false;
    }
    else
    {
        // If we don't recognise the type - we cannot convert it!
        r_converted = false;
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

// Ensure that if the input value is a name it comes out as a string.
// Ensure that if the input value is an array, all elements that are names (recursed)
// come out as strings.
// If r_output is nil, then no mutation occurred; otherwise it is the mutated value.
static bool __script_ensure_names_are_strings(MCValueRef p_input, MCValueRef& r_output)
{
    if (MCValueGetTypeCode(p_input) == kMCValueTypeCodeName)
    {
        r_output = MCValueRetain(MCNameGetString((MCNameRef)p_input));
        return true;
    }
    
    if (MCValueGetTypeCode(p_input) == kMCValueTypeCodeArray)
    {
        MCAutoArrayRef t_mutated_input;
        
        MCNameRef t_key;
        MCValueRef t_element;
        uintptr_t t_iterator;
        t_iterator = 0;
        while(MCArrayIterate((MCArrayRef)p_input, t_iterator, t_key, t_element))
        {
            // Ensure the element's names are all strings.
            MCAutoValueRef t_mutated_element;
            if (!__script_ensure_names_are_strings(t_element, Out(t_mutated_element)))
                return false;
            
            // If something changed then we must replace the existing value.
            if (*t_mutated_element != nil)
            {
                // First make sure we have a copy of the original to mutate.
                if (*t_mutated_input == nil)
                {
                    if (!MCArrayMutableCopy((MCArrayRef)p_input, Out(t_mutated_input)))
                        return false;
                }
                
                // Now store the value.
                if (!MCArrayStoreValue(In(t_mutated_input), true, t_key, In(t_mutated_element)))
                    return false;
                
                // We are done for this element.
                continue;
            }
            
            // If we get here then nothing has changed, so there's nothing to do.
        }
        
        // Either return the original or the mutated version depending on whether
        // any of its elements mutated.
        if (*t_mutated_input == nil)
            r_output = nil;
        else
            r_output = t_mutated_input . Take();
        
        return true;
    }
    
    // If no changes are made, return nil for the output.
    r_output = nil;
    
    return true;
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
            
            r_converted = true;
            
            return true;
        }
     
        // If the string is 'true', then assign true.
        if (MCStringIsEqualTo((MCStringRef)x_value, kMCTrueString, kMCStringOptionCompareCaseless))
        {
            MCValueAssign((MCBooleanRef&)x_value, kMCTrue);
            
            r_converted = true;
            
            return true;
        }
        
        // If the string is 'false', then assign false.
        if (MCStringIsEqualTo((MCStringRef)x_value, kMCFalseString, kMCStringOptionCompareCaseless))
        {
            MCValueAssign((MCBooleanRef&)x_value, kMCFalse);
            
            r_converted = true;
            
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
    {
        r_converted = true;
        
        return true;
    }
    
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
            
            r_converted = true;
            
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
            
            r_converted = true;
            
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
		MCNumberRef t_number = static_cast<MCNumberRef>(x_value);
		MCAutoStringRef t_string;
		if (MCNumberIsInteger(t_number))
		{
			if (!MCStringFormat(&t_string, "%d", MCNumberFetchAsInteger(t_number)))
				return false;
			if (!MCStringSetNumericValue(*t_string, MCNumberFetchAsReal(t_number)))
				return false;
		}
		else
		{
			if (!MCU_r8tos(MCNumberFetchAsReal(t_number),
			               ctxt.GetNumberFormatWidth(),
			               ctxt.GetNumberFormatTrailing(),
			               ctxt.GetNumberFormatForce(),
			               &t_string))
				return false;
		}
		MCValueRelease(x_value);
		x_value = MCValueRetain(*t_string);
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
        MCValueRef t_mutated_value;
        if (!__script_ensure_names_are_strings(x_value, t_mutated_value))
            return false;
        
        if (t_mutated_value != nil)
            MCValueAssignAndRelease(x_value, t_mutated_value);
        
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
	
	// If we managed to convert to an array, and the array is empty then we
	// can convert.
	if (t_is_array &&
		MCArrayIsEmpty((MCArrayRef)x_value))
	{
		MCValueAssign(x_value, (MCValueRef)kMCEmptyProperList);
		r_converted = true;
		return true;
	}
	
    // If we managed to convert to an array, and the array is a sequence
    // we can convert.
    if (t_is_array && MCArrayIsSequence((MCArrayRef)x_value))
    {
        MCAutoProperListRef t_proper_list;
        if (!MCProperListCreateMutable(Out(t_proper_list)))
            return false;
        
        for(uindex_t i = 0; i < MCArrayGetCount((MCArrayRef)x_value); i++)
        {
            // We know this will succeed as we have a sequence.
            MCValueRef t_element;
            MCArrayFetchValueAtIndex((MCArrayRef)x_value, i + 1, t_element);
            
            // Deal with the name/string issue.
            MCAutoValueRef t_revised_element;
            if (!__script_ensure_names_are_strings(t_element, &t_revised_element))
                return false;
                
            if (!MCProperListPushElementOntoBack(In(t_proper_list), *t_revised_element == nil ? t_element : In(t_revised_element)))
                return false;
        }
        
        if (!t_proper_list . MakeImmutable())
            return false;
        
        MCValueAssignAndRelease(x_value, (MCValueRef)t_proper_list . Take());
        
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
    
    // If the bridging type is null, then there is no bridge.
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

bool
MCExtensionInitialize(void)
{
	// Initialize static variables
	MCextensions = nil;
	MCextensionschanged = false;
	MCextensionshandlermap = nil;

    MCScriptSetLoadLibraryCallback(MCEngineLoadLibrary);
    
    return MCScriptForEachBuiltinModule([](void *p_context, MCScriptModuleRef p_module) {
        if (MCScriptIsModuleALibrary(p_module) ||
            MCScriptIsModuleAWidget(p_module))
        {
            return MCEngineAddExtensionFromModule(p_module);
        }

        return true;
    }, nullptr);
}

void
MCExtensionFinalize(void)
{
    while(MCextensions != nullptr)
    {
        /* Unlink the extension from the global list */
        MCLoadedExtension *t_ext = MCextensions;
        MCextensions = MCextensions->next;
        
        /* Free the extensions */
        __MCEngineFreeExtension(t_ext);
    }
    
    if (MCextensionshandlermap != nil)
		MCValueRelease(MCextensionshandlermap);
	MCextensionshandlermap = nil;
}


////////////////////////////////////////////////////////////////////////////////
