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
#include <foundation-system.h>

#include "ffi.h"

#include "libscript/script.h"
#include "script-private.h"
#include "script-bytecode.hpp"

////////////////////////////////////////////////////////////////////////////////

// This is the module of the most recent LCB stack frame on the current thread's
// stack. It is set before and after a foreign handler call so that the native
// code can get some element of context.
static MCScriptModuleRef s_current_module = nil;

static MCScriptWidgetEnterCallback s_widget_enter_callback = nullptr;
static MCScriptWidgetLeaveCallback s_widget_leave_callback = nullptr;

void
MCScriptSetWidgetBarrierCallbacks(MCScriptWidgetEnterCallback p_entry,
                                  MCScriptWidgetLeaveCallback p_leave)
{
    s_widget_enter_callback = p_entry;
    s_widget_leave_callback = p_leave;
}

////////////////////////////////////////////////////////////////////////////////

bool
MCScriptCreateInstanceOfModule(MCScriptModuleRef p_module,
							   MCScriptInstanceRef& r_instance)
{
    bool t_success;
    t_success = true;
    
    MCScriptInstanceRef t_instance;
    t_instance = nil;
    
    // If the module is not usable, then we cannot create an instance.
    if (t_success)
        if (!p_module -> is_usable)
            return false;
    
    // If this is a module which shares a single instance, then return that if we have
    // one.
    if (p_module -> module_kind != kMCScriptModuleKindWidget &&
        p_module -> shared_instance != nil)
    {
        r_instance = MCScriptRetainInstance(p_module -> shared_instance);
        return true;
    }
    
    // Attempt to create a script object.
    if (t_success)
        t_success = MCScriptCreateObject(kMCScriptObjectKindInstance, t_instance);

    // Now associate the script object with the module (so the 'slots' field make sense).
    if (t_success)
    {
        t_instance -> module = MCScriptRetainModule(p_module);
    }
    
    // Now allocate the slots field.
    if (t_success)
        t_success = MCMemoryNewArray(p_module -> slot_count, t_instance -> slots);
    
    if (t_success)
    {
        /* Loop over all of the variables of the module, initialising
         * the corresponding slot to a default value of the
         * appropriate type, if available. */
	    for (uindex_t i = 0; i < p_module->definition_count; ++i)
	    {
		    if (p_module->definitions[i]->kind != kMCScriptDefinitionKindVariable)
			    continue; /* Not a variable */

		    MCScriptVariableDefinition *t_variable =
			    static_cast<MCScriptVariableDefinition *>(p_module->definitions[i]);

		    uindex_t t_slot_index = t_variable->slot_index;
		    /* COMPUTE CHECK */ __MCScriptAssert__(t_slot_index < p_module->slot_count,
		                                           "computed variable slot out of range");

		    MCTypeInfoRef t_type = p_module->types[t_variable->type]->typeinfo;
		    if (nil == t_type)
			    continue; /* Variable is untyped */

		    MCValueRef t_default = MCTypeInfoGetDefault(t_type);
		    if (nil == t_default)
			    continue; /* Type has no default value */

		    t_instance->slots[t_slot_index] = MCValueRetain(t_default);
	    }

        // If this is a module which shares its instance, then add a link to it.
        // (Note this is weak reference - we don't retain, otherwise we would have
        //  a cycle!).
        if (p_module -> module_kind != kMCScriptModuleKindWidget)
            p_module -> shared_instance = t_instance;
        
        r_instance = t_instance;
    }
    else
        MCScriptDestroyObject(t_instance);
    
    return t_success;
}

void
MCScriptDestroyInstance(MCScriptInstanceRef self)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    // If the object had slots initialized, then free them.
    if (self -> slots != nil)
    {
        for(uindex_t i = 0; i < self -> module -> slot_count; i++)
            MCValueRelease(self -> slots[i]);
        MCMemoryDeleteArray(self -> slots);
    }
    
    // If the instance has handler-refs, then free them.
    for(uindex_t i = 0; i < self -> handler_count; i++)
        MCValueRelease(self -> handlers[i] . value);
    MCMemoryDeleteArray(self -> handlers);
    
    // If the instance has a module, and this is the shared instance then set
    // the shared instance field to nil.
    if (self -> module != nil &&
        self -> module -> shared_instance == self)
        self -> module -> shared_instance = nil;
    
    // If the instance was associated with its module, then release it.
    if (self -> module != nil)
        MCScriptReleaseModule(self -> module);
    
    // The rest of the deallocation is handled by MCScriptDestroyObject.
}

////////////////////////////////////////////////////////////////////////////////

MCScriptInstanceRef
MCScriptRetainInstance(MCScriptInstanceRef self)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    return (MCScriptInstanceRef)MCScriptRetainObject(self);
}

void
MCScriptReleaseInstance(MCScriptInstanceRef self)
{
	if (nil == self)
		return;

    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    MCScriptReleaseObject(self);
}

////////////////////////////////////////////////////////////////////////////////

void
MCScriptSetInstanceHostPtr(MCScriptInstanceRef self, void *p_host_ptr)
{
    self->host_ptr = p_host_ptr;
}

void *
MCScriptGetInstanceHostPtr(MCScriptInstanceRef self)
{
    return self->host_ptr;
}

////////////////////////////////////////////////////////////////////////////////

MCScriptModuleRef
MCScriptGetModuleOfInstance(MCScriptInstanceRef self)
{
    return self -> module;
}

////////////////////////////////////////////////////////////////////////////////

static bool
__MCHandlerTypeInfoConformsToPropertyGetter(MCTypeInfoRef p_typeinfo)
{
	return MCHandlerTypeInfoGetParameterCount(p_typeinfo) == 0 &&
			MCHandlerTypeInfoGetReturnType(p_typeinfo) != kMCNullTypeInfo;
}

static bool
__MCHandlerTypeInfoConformsToPropertySetter(MCTypeInfoRef p_typeinfo)
{
	return MCHandlerTypeInfoGetParameterCount(p_typeinfo) == 1 &&
			MCHandlerTypeInfoGetParameterMode(p_typeinfo, 0) == kMCHandlerTypeFieldModeIn;
}

static bool
__MCScriptCallHandlerDefinitionInInstance(MCScriptInstanceRef self,
										  MCScriptHandlerDefinition *p_handler_def,
										  MCValueRef *p_arguments,
										  uindex_t p_argument_count,
										  MCValueRef* r_value)
{
    void *t_cookie = nullptr;
    
    if (self->module->module_kind == kMCScriptModuleKindWidget)
    {
        if (s_widget_enter_callback != nullptr)
        {
            t_cookie = s_widget_enter_callback(self,
                                               self->host_ptr);
        }
    }

	MCScriptExecuteContext t_execute_ctxt;
	t_execute_ctxt.Enter(self,
						 p_handler_def,
	                     MCMakeSpan(p_arguments, p_argument_count),
						 r_value);
	
	while(t_execute_ctxt.Step())
	{
		MCScriptBytecodeExecute(t_execute_ctxt);
	}
	
    if (self->module->module_kind == kMCScriptModuleKindWidget)
    {
        if (s_widget_leave_callback != nullptr)
        {
            s_widget_leave_callback(self,
                                    self->host_ptr,
                                    t_cookie);
        }
    }

	if (!t_execute_ctxt.Leave())
	{
		return false;
	}
	
	return true;
}

static bool
__MCScriptGetVariablePropertyInInstance(MCScriptInstanceRef self,
										MCScriptVariableDefinition *p_variable_def,
										MCValueRef& r_value)
{
	MCValueRef t_value =
		self->slots[p_variable_def->slot_index];
	
	r_value = t_value != nil ? MCValueRetain(t_value)
						     : t_value;
	
	return true;
}

static bool
__MCScriptGetHandlerPropertyInInstance(MCScriptInstanceRef self,
									   MCScriptHandlerDefinition *p_handler_def,
									   MCValueRef& r_value)
{
	/* LOAD CHECK */
	__MCScriptAssert__(__MCHandlerTypeInfoConformsToPropertyGetter(self->module->types[p_handler_def->type]->typeinfo),
					   "incorrect signature for property getter");
	
	return __MCScriptCallHandlerDefinitionInInstance(self,
													 p_handler_def,
													 nil,
													 0,
													 &r_value);
}

static bool
__MCScriptSetVariablePropertyInInstance(MCScriptInstanceRef self,
										MCScriptVariableDefinition *p_variable_def,
										MCValueRef p_value)
{
	// BRIDGE: This should bridge (convert) the type if necessary.
	MCValueAssign(self->slots[p_variable_def->slot_index],
				  p_value);
	
	return true;
}

static bool
__MCScriptSetHandlerPropertyInInstance(MCScriptInstanceRef self,
									   MCScriptHandlerDefinition *p_handler_def,
									   MCValueRef p_value)
{
	/* LOAD CHECK */
	__MCScriptAssert__(__MCHandlerTypeInfoConformsToPropertySetter(self->module->types[p_handler_def->type]->typeinfo),
					   "incorrect signature for property setter");
	
	return __MCScriptCallHandlerDefinitionInInstance(self,
													 p_handler_def,
													 &p_value,
													 1,
													 nil);
}

bool
MCScriptGetPropertyInInstance(MCScriptInstanceRef self,
							  MCNameRef p_property,
							  MCValueRef& r_value)
{
	__MCScriptValidateObjectAndKind__(self,
									  kMCScriptObjectKindInstance);
	
	// Lookup the definition (throws if not found).
	MCScriptPropertyDefinition *t_property_def;
	if (!MCScriptLookupPropertyDefinitionInModule(self->module,
												  p_property,
												  t_property_def))
		
	{
		return MCScriptThrowPropertyNotFoundError(self,
												  p_property);
	}
	
	MCScriptDefinition *t_getter;
	t_getter = t_property_def->getter != 0 ? self->module->definitions[t_property_def->getter - 1] : nil;
	
	/* LOAD CHECK */
	__MCScriptAssert__(t_getter != nil,
					   "property has no getter");
	
	switch(t_getter->kind)
	{
		case kMCScriptDefinitionKindVariable:
		{
			MCValueRef t_value;
			if (!__MCScriptGetVariablePropertyInInstance(self,
														 static_cast<MCScriptVariableDefinition *>(t_getter),
														 t_value))
			{
				return false;
			}
			
			if (t_value == nil)
			{
				return MCScriptThrowPropertyUsedBeforeAssignedError(self,
																	t_property_def);
			}
			
			r_value = t_value;
		}
		break;
				
		case kMCScriptDefinitionKindHandler:
		{
			if (!__MCScriptGetHandlerPropertyInInstance(self,
														static_cast<MCScriptHandlerDefinition *>(t_getter),
														r_value))
			{
				return false;
			}
		}
		break;
				
		default:
			/* LOAD CHECK */
			__MCScriptUnreachable__("property getter is not a variable or handler");
			break;
	}
				
	return true;
}

bool
MCScriptSetPropertyInInstance(MCScriptInstanceRef self,
							  MCNameRef p_property,
							  MCValueRef p_value)
{
	__MCScriptValidateObjectAndKind__(self,
									  kMCScriptObjectKindInstance);
	
	// Lookup the definition (throws if not found).
	MCScriptPropertyDefinition *t_property_def;
	if (!MCScriptLookupPropertyDefinitionInModule(self->module,
												  p_property,
												  t_property_def))
		
	{
		return MCScriptThrowPropertyNotFoundError(self,
												  p_property);
	}
	
	MCScriptDefinition *t_setter;
	t_setter = t_property_def->setter != 0 ? self->module->definitions[t_property_def->setter - 1] : nil;
	
	/* LOAD CHECK */
	__MCScriptAssert__(t_setter != nil,
					   "property has no setter");
	/* LOAD CHECK */
	__MCScriptAssert__(t_setter->kind == kMCScriptDefinitionKindVariable ||
					   t_setter->kind == kMCScriptDefinitionKindHandler,
					   "property setter is not a variable or handler");
	
	MCTypeInfoRef t_property_type;
	switch(t_setter->kind)
	{
		case kMCScriptDefinitionKindVariable:
		{
			t_property_type = self->module->types[static_cast<MCScriptVariableDefinition*>(t_setter)->type]->typeinfo;
		}
		break;
			
		case kMCScriptDefinitionKindHandler:
		{
			MCTypeInfoRef t_setter_signature =
				self->module->types[static_cast<MCScriptHandlerDefinition*>(t_setter)->type]->typeinfo;
			
			/* LOAD CHECK */
			__MCScriptAssert__(__MCHandlerTypeInfoConformsToPropertySetter(t_setter_signature),
							   "incorrect signature for property setter");
			
			t_property_type = MCHandlerTypeInfoGetParameterType(t_setter_signature,
																0);
		}
		break;
			
		default:
		{
			__MCScriptUnreachable__("inconsistency with definition kind in property setting");
			return false;
		}
		break;
	}
	
	// Check the type passed in conforms to the required type of the
	// property here so that it generates a more useful error message.
	if (!MCTypeInfoConforms(MCValueGetTypeInfo(p_value),
							t_property_type))
	{
		return MCScriptThrowInvalidValueForPropertyError(self,
														 t_property_def,
		                                                 t_property_type,
														 p_value);
	}
	
	switch(t_setter->kind)
	{
		case kMCScriptDefinitionKindVariable:
		{
			if (!__MCScriptSetVariablePropertyInInstance(self,
														 static_cast<MCScriptVariableDefinition *>(t_setter),
														 p_value))
			{
				return false;
			}
		}
		break;
			
		case kMCScriptDefinitionKindHandler:
		{
			if (!__MCScriptSetHandlerPropertyInInstance(self,
														static_cast<MCScriptHandlerDefinition *>(t_setter),
														p_value))
			{
				return false;
			}
		}
		break;
			
		default:
		{
			__MCScriptUnreachable__("inconsistency with definition kind in property setting");
			return false;
		}
		break;
	}
	
	return true;

}

bool
MCScriptCallHandlerInInstanceInternal(MCScriptInstanceRef self,
									  MCScriptHandlerDefinition *p_handler_def,
									  MCValueRef *p_arguments,
									  uindex_t p_argument_count,
									  MCValueRef& r_value)
{
	return __MCScriptCallHandlerDefinitionInInstance(self,
													 p_handler_def,
													 p_arguments,
													 p_argument_count,
													 &r_value);
}

bool
MCScriptCallHandlerInInstance(MCScriptInstanceRef self,
							  MCNameRef p_handler,
							  MCValueRef *p_arguments,
							  uindex_t p_argument_count,
							  MCValueRef& r_value)
{
	__MCScriptValidateObjectAndKind__(self,
									  kMCScriptObjectKindInstance);
	
	MCScriptHandlerDefinition *t_handler_def;
	if (!MCScriptLookupHandlerDefinitionInModule(self->module,
												 p_handler,
												 t_handler_def))
	{
		return MCScriptThrowHandlerNotFoundError(self,
												 p_handler);
	}
	
	return MCScriptCallHandlerInInstanceInternal(self,
												 t_handler_def,
												 p_arguments,
												 p_argument_count,
												 r_value);
}

bool
MCScriptCallHandlerInInstanceIfFound(MCScriptInstanceRef self,
									 MCNameRef p_handler,
									 MCValueRef *p_arguments,
									 uindex_t p_argument_count,
									 MCValueRef& r_value)
{
	__MCScriptValidateObjectAndKind__(self,
									  kMCScriptObjectKindInstance);
	
	MCScriptHandlerDefinition *t_handler_def;
	if (!MCScriptLookupHandlerDefinitionInModule(self->module,
												 p_handler,
												 t_handler_def))
	{
		r_value = MCValueRetain(kMCNull);
		return true;
	}

	return MCScriptCallHandlerInInstanceInternal(self,
												 t_handler_def,
												 p_arguments,
												 p_argument_count,
												 r_value);
}

////////////////////////////////////////////////////////////////////////////////

// This method resolves the binding string in the foreign function. The format is:
//   [lang:][library>][class.]function[!calling]
//
// lang - one of c, cpp, objc or java. If not present, it is taken to be c.
// library - the library to load the symbol from. If not present, it is taken to be the
//   main executable.
// class - only valid for cpp, objc, or java. If not present, it is taken to be no class.
// function - the name of the function or method.
// calling - the calling convention, if not present it is taken to be the platform default.
//
// The calling conventions are:
//   default (sysv on 32-bit unix intel, unix64 on 64-bit unix intel, win64 on 64-bit windows, cdelc on 32-bit windows)
//   stdcall (windows 32-bit)
//   thiscall (windows 32-bit)
//   fastcall (windows 32-bit)
//   cdecl (windows 32-bit)
//   pascal (windows 32-bit)
//   register (windows 32-bit)
// If a windows calling convention is specified, it is taken to be 'default' on
// other platforms.
//

// Split x_string at p_char, returning the portion up to the char, or the end
// of the string in r_field. The source string x_string is replaced with the
// remainder of the string.
// i.e. Split("foo.bar", '.'):
//        x_string = "bar", r_field = "foo"
//
static bool
__MCScriptSplitForeignBindingString(MCStringRef& x_string,
									codepoint_t p_char,
									MCStringRef& r_field)
{
	MCStringRef t_head, t_tail;
	if (!MCStringDivideAtChar(x_string,
							  p_char,
							  kMCStringOptionCompareExact,
							  t_head,
							  t_tail))
	{
		return false;
	}
	
	if (!MCStringIsEmpty(t_tail))
	{
		r_field = t_head;
		MCValueRelease(x_string);
		x_string = t_tail;
	}
	else
	{
		MCValueRelease(x_string);
		MCValueRelease(t_tail);
		x_string = t_head;
	}
	
	return true;
}

static bool __split_function_signature(MCStringRef p_string, MCStringRef& r_function, MCStringRef& r_arguments, MCStringRef& r_return)
{
	MCAutoStringRef t_head, t_args, t_return;
	uindex_t t_open_bracket_offset, t_close_bracket_offset;
	if (!MCStringFirstIndexOfChar(p_string, '(', 0, kMCStringOptionCompareExact, t_open_bracket_offset) ||
		!MCStringFirstIndexOfChar(p_string, ')', 0, kMCStringOptionCompareExact, t_close_bracket_offset))
	{
		r_function = MCValueRetain(p_string);
		r_arguments = MCValueRetain(kMCEmptyString);
		r_return = MCValueRetain(kMCEmptyString);
		return true;
	}
	
	if (!MCStringCopySubstring(p_string, MCRangeMakeMinMax(t_open_bracket_offset, t_close_bracket_offset + 1), &t_args))
		return false;
	
	if (!MCStringCopySubstring(p_string, MCRangeMake(t_close_bracket_offset + 1, UINDEX_MAX), &t_return))
		return false;
	
	if (!MCStringCopySubstring(p_string, MCRangeMake(0, t_open_bracket_offset), &t_head))
		return false;
	
	r_arguments = MCValueRetain(*t_args);
	r_return = MCValueRetain(*t_return);
	r_function = MCValueRetain(*t_head);
	return true;
}

// Resolve the call type
static MCJavaCallType __MCScriptGetJavaCallType(MCStringRef p_class, MCStringRef p_function, MCStringRef p_calling)
{
    if (MCStringIsEqualToCString(p_function, "new", kMCStringOptionCompareExact))
        return MCJavaCallTypeConstructor;
    
    if (MCStringIsEqualToCString(p_function, "interface", kMCStringOptionCompareExact))
        return MCJavaCallTypeInterfaceProxy;
    
    bool t_is_static =
        MCStringIsEqualToCString(p_calling, "static", kMCStringOptionCompareCaseless);
    
    // If the 'class' is get or set, then the binding is for a field
    if (p_class != nullptr &&
        MCStringIsEqualToCString(p_class, "get", kMCStringOptionCompareExact))
    {
        if (t_is_static)
            return MCJavaCallTypeStaticGetter;
        
        return MCJavaCallTypeGetter;
    }

    if (p_class != nullptr &&
        MCStringIsEqualToCString(p_class, "set", kMCStringOptionCompareExact))
    {
        if (t_is_static)
            return MCJavaCallTypeStaticSetter;
        
        return MCJavaCallTypeSetter;
    }
    
    if (t_is_static)
        return MCJavaCallTypeStatic;

    if (MCStringIsEqualToCString(p_calling, "nonvirtual", kMCStringOptionCompareCaseless))
        return MCJavaCallTypeNonVirtual;
    
    return MCJavaCallTypeInstance;
}

static bool
__MCScriptResolveForeignFunctionBinding(MCScriptInstanceRef p_instance,
										MCScriptForeignHandlerDefinition *p_handler,
										ffi_abi& r_abi,
										bool* r_bound)
{
    integer_t t_ordinal = 0;
    if (p_instance->module->builtins != nullptr &&
        MCTypeConvertStringToLongInteger(p_handler->binding, t_ordinal))
    {
        p_handler->native.function = p_instance->module->builtins[t_ordinal];
        p_handler->is_builtin = true;
        r_abi = FFI_DEFAULT_ABI;
        if (r_bound != nullptr)
        {
            *r_bound = true;
        }
        return true;
    }

	MCStringRef t_rest;
	t_rest = MCValueRetain(p_handler -> binding);
	
	MCAutoStringRef t_language;
	MCAutoStringRef t_library;
	MCAutoStringRef t_class;
	MCAutoStringRef t_function_string;
	MCAutoStringRef t_calling;
	if (!__MCScriptSplitForeignBindingString(t_rest,
											 ':',
											 &t_language) ||
		!__MCScriptSplitForeignBindingString(t_rest,
											 '>',
											 &t_library) ||
		!__MCScriptSplitForeignBindingString(t_rest,
											 '.',
											 &t_class) ||
		!MCStringDivideAtChar(t_rest,
							  '!',
							  kMCStringOptionCompareExact,
							  &t_function_string,
							  &t_calling))
	{
		MCValueRelease(t_rest);
		return false;
	}
	
	MCValueRelease(t_rest);
	
	MCAutoStringRef t_arguments, t_return, t_function;
	if (!__split_function_signature(*t_function_string, &t_function, &t_arguments, &t_return))
		return false;
	
	int t_cc;
	if (!MCStringIsEmpty(*t_calling))
	{
		static const char *s_callconvs[] =
		{
			"default",
			"sysv",
			"stdcall",
			"thiscall",
			"fastcall",
			"cdecl",
			"pascal",
			"register",
			"instance",
			"static",
			"nonvirtual",
			nil
		};
		for(t_cc = 0; s_callconvs[t_cc] != nil; t_cc++)
		{
			if (MCStringIsEqualToCString(*t_calling,
										 s_callconvs[t_cc],
										 kMCStringOptionCompareCaseless))
				break;
		}
		
		if (s_callconvs[t_cc] == nil)
		{
			return MCScriptThrowUnknownForeignCallingConventionError();
		}
	}
	else
		t_cc = 0;
	
	if (MCStringIsEmpty(*t_function))
	{
		return MCScriptThrowMissingFunctionInForeignBindingError();
	}
	
	if (MCStringIsEmpty(*t_language) ||
		MCStringIsEqualToCString(*t_language,
								 "c",
								 kMCStringOptionCompareExact))
	{
		if (!MCStringIsEmpty(*t_class))
		{
			return MCScriptThrowClassNotAllowedInCBindingError();
		}
		
        /* TODO: This leaks a module handle if library is not empty (builtin) */
        MCSLibraryRef t_module;
        if (MCStringIsEmpty(*t_library))
        {
            t_module = MCScriptGetLibrary();
        }
        else
        {
		    if (!MCScriptLoadLibrary(MCScriptGetModuleOfInstance(p_instance),
                                     *t_library,
                                     t_module))
		    {
			    if (r_bound == nil)
			    {
				    return MCScriptThrowUnableToLoadForiegnLibraryError();
			    }
			
			    *r_bound = false;
			
			    return true;
		    }
        }
		
		void *t_pointer =
                MCSLibraryLookupSymbol(t_module,
                                       *t_function);
		if (t_pointer == nullptr)
		{
            if (r_bound == nullptr)
            {
                return MCScriptThrowUnableToResolveForeignHandlerError(p_instance,
                                                                       p_handler);
            }
            
            *r_bound = false;
            
			return true;
		}
		
		p_handler -> native . function = t_pointer;
        p_handler->is_builtin = false;
	}
	else if (MCStringIsEqualToCString(*t_language,
									  "cpp",
									  kMCStringOptionCompareExact))
	{
		return MCScriptThrowCXXBindingNotImplemented();
	}
	else if (MCStringIsEqualToCString(*t_language,
									  "objc",
									  kMCStringOptionCompareExact))
	{
#if !defined(_MACOSX) && !defined(TARGET_SUBPLATFORM_IPHONE)
		if (r_bound == nil)
		{
			return MCScriptThrowObjCBindingNotSupported();
		}
		
		*r_bound = false;
		
		return true;
#else
		return MCScriptThrowObjCBindingNotImplemented();
#endif
	}
	else if (MCStringIsEqualToCString(*t_language, "java", kMCStringOptionCompareExact) ||
             MCStringIsEqualToCString(*t_language, "javaui", kMCStringOptionCompareExact))
    {
        p_handler->is_ui_bound = MCStringIsEqualToCString(*t_language, "javaui", kMCStringOptionCompareExact);
    
		p_handler -> is_java = true;
        p_handler->is_builtin = false;
	
        p_handler -> java . call_type = __MCScriptGetJavaCallType(*t_class,
                                                                  *t_function,
                                                                  *t_calling);
        
		MCNewAutoNameRef t_class_name;
		if (!MCNameCreate(*t_library, &t_class_name))
			return false;
		
		p_handler -> java . class_name = MCValueRetain(*t_class_name);
		
		MCTypeInfoRef t_signature = p_instance -> module -> types[p_handler -> type] -> typeinfo;
        
        // Check that the java signature in the binding string is
        // compatible with the types of the foreign handler
		if (!MCJavaCheckSignature(t_signature,
		                          *t_arguments,
		                          *t_return,
		                          p_handler -> java . call_type) ||
            !MCJavaVMInitialize())
        {
            if (r_bound == nullptr)
            {
                return false;
            }
            
            MCErrorReset();
        
            *r_bound = false;
        
            return true;
        }
		
        void *t_method_id = MCJavaGetMethodId(*t_class_name, *t_function, *t_arguments, *t_return, p_handler -> java . call_type);
        
		if (t_method_id != nullptr)
		{
			p_handler -> java . method_id = t_method_id;
		}
		else
		{
			if (r_bound == nullptr)
			{
                return MCScriptThrowUnableToResolveForeignHandlerError(p_instance,
                                                                       p_handler);
            }
            
            MCErrorReset();
            
			*r_bound = false;
			
			return true;
		}
	}

#ifdef _WIN32
	r_abi = t_cc == 0 ? FFI_DEFAULT_ABI : (ffi_abi)t_cc;
#else
	r_abi = FFI_DEFAULT_ABI;
#endif

	if (r_bound != nullptr)
	{
		*r_bound = true;
	}

	return true;
}

// This method attempts to bind a foreign function so it can be used. If
// binding is required (as the call is about to be executed), r_bound should be
// nil. Otherwise, it is assumed the binding is to be checked afterwards, in
// which case r_bound should be non-nil, and whether or not the binding is
// present / usable will be indicated via a true return value in that out
// parameter.
static bool
__MCScriptPrepareForeignFunction(MCScriptInstanceRef p_instance,
								 MCScriptForeignHandlerDefinition *p_handler,
								 bool *r_bound)
{
    // If the handler is already bound, then do nothing.
    if (p_handler->is_bound)
    {
        if (r_bound != nullptr)
        {
            *r_bound = true;
        }
        
        return true;
    }
    
    // Attempt to resolve the foreign function binding. If r_bound == nullptr,
    // then this will fail if binding fails. If r_bound != nullptr, then *r_bound
    // will indicate whether the function was bound or not.
	ffi_abi t_abi;
    if (!__MCScriptResolveForeignFunctionBinding(p_instance,
                                                 p_handler,
                                                 t_abi,
                                                 r_bound))
    {
        return false;
    }
    
    // If we are 'try to bind' and the foreign binding failed, then
	// return the bound status to the caller and return.
	if (r_bound != nullptr &&
		!*r_bound)
	{
		return true;
	}

    // If we get here then it means that the binding succeeded.
	MCTypeInfoRef t_signature;
	t_signature = p_instance->module->types[p_handler->type]->typeinfo;
	
	// Ask the handler typeinfo to construct its ffi 'cif'. If this fails
	// then it will already have thrown (either OOM, or there was a problem
	// with libffi!).
	if (!p_handler -> is_java &&
        !MCHandlerTypeInfoGetLayoutType(t_signature,
										t_abi,
										p_handler->native.function_cif))
	{
		return false;
	}
	
	// The function can only be considered bound, if it gets to this point.
    p_handler->is_bound = true;
    
	return true;
}

bool
MCScriptTryToBindForeignHandlerInInstanceInternal(MCScriptInstanceRef p_instance,
												  MCScriptForeignHandlerDefinition *p_handler,
												  bool& r_bound)
{
	return __MCScriptPrepareForeignFunction(p_instance,
											p_handler,
											&r_bound);
}

bool
MCScriptBindForeignHandlerInInstanceInternal(MCScriptInstanceRef p_instance,
											 MCScriptForeignHandlerDefinition *p_handler)
{
	return __MCScriptPrepareForeignFunction(p_instance,
											p_handler,
											nil);
}

////////////////////////////////////////////////////////////////////////////////

// This context struct, callbacks and associated functions provide an implementation
// of MCHandler for handlers coming from libscript. It enables an MCHandlerRef
// created from within LCB to be used 'out-of-frame' - i.e. not just from direct
// invocation within the LCB VM.

struct __MCScriptInternalHandlerContext
{
	MCScriptInstanceRef instance;
	MCScriptCommonHandlerDefinition *definition;
};

static void
__MCScriptInternalHandlerRelease(void *p_context)
{
	__MCScriptInternalHandlerContext *context =
		(__MCScriptInternalHandlerContext *)p_context;
	
	MCScriptReleaseInstance(context->instance);
}

static bool
__MCScriptInternalHandlerInvoke(void *p_context,
								MCValueRef *p_arguments,
								uindex_t p_argument_count,
								MCValueRef& r_return_value)
{
	__MCScriptInternalHandlerContext *context =
		(__MCScriptInternalHandlerContext *)p_context;
	
	if (context->definition->kind != kMCScriptDefinitionKindHandler)
		return MCErrorThrowGeneric(MCSTR("out-of-frame indirect foreign handler calls not yet supported"));
	
	return MCScriptCallHandlerInInstanceInternal(context->instance,
												 static_cast<MCScriptHandlerDefinition *>(context->definition),
												 p_arguments,
												 p_argument_count,
												 r_return_value);
}

static bool
__MCScriptInternalHandlerDescribe(void *p_context,
								  MCStringRef& r_description)
{
	__MCScriptInternalHandlerContext *context =
		(__MCScriptInternalHandlerContext *)p_context;
	
	MCScriptModuleRef t_module;
	t_module = MCScriptGetModuleOfInstance (context->instance);
	
	MCNameRef t_module_name;
	t_module_name = MCScriptGetNameOfModule (t_module);
	
	MCNameRef t_handler_name;
	t_handler_name = MCScriptGetNameOfDefinitionInModule(t_module,
														 context->definition);
	
	return MCStringFormat(r_description, "%@.%@()", t_module_name, t_handler_name);
}

static MCHandlerCallbacks __kMCScriptInternalHandlerCallbacks =
{
	sizeof(__MCScriptInternalHandlerContext),
	__MCScriptInternalHandlerRelease,
	__MCScriptInternalHandlerInvoke,
	__MCScriptInternalHandlerDescribe
};

static bool
__MCScriptInternalHandlerCreate(MCScriptInstanceRef p_instance,
								MCScriptCommonHandlerDefinition *p_handler_def,
								MCHandlerRef& r_handler)
{
	MCTypeInfoRef t_signature;
	t_signature = p_instance->module->types[p_handler_def->type]->typeinfo;
	
	// The context struct is 'moved' into the handlerref.
	__MCScriptInternalHandlerContext t_context;
	t_context . instance = p_instance;
	t_context . definition = p_handler_def;
	
	return MCHandlerCreate(t_signature,
						   &__kMCScriptInternalHandlerCallbacks,
						   &t_context,
						   r_handler);
}

bool
MCScriptHandlerIsInternal(MCHandlerRef p_handler)
{
	return MCHandlerGetCallbacks(p_handler) == &__kMCScriptInternalHandlerCallbacks;
}

void
MCScriptInternalHandlerQuery(MCHandlerRef p_handler,
							 MCScriptInstanceRef& r_instance,
							 MCScriptCommonHandlerDefinition*& r_handler_def)
{
	__MCScriptAssert__(MCScriptHandlerIsInternal(p_handler),
					   "passed handlerref is not an script internal handler ref");
	
	__MCScriptInternalHandlerContext *t_context =
		static_cast<__MCScriptInternalHandlerContext *>(MCHandlerGetContext(p_handler));
	
	r_instance = t_context->instance;
	r_handler_def = t_context->definition;
}

static uindex_t
__MCScriptComputeHandlerIndexInInstance(MCScriptInstanceRef p_instance,
										MCScriptCommonHandlerDefinition *p_handler)
{
	uindex_t t_min, t_max;
	t_min = 0;
	t_max = p_instance->handler_count;
	while(t_min < t_max)
	{
		uindex_t t_mid;
		t_mid = (t_min + t_max) / 2;
		
		if (p_instance->handlers[t_mid].definition < p_handler)
			t_min = t_mid + 1;
		else
			t_max = t_mid;
	}
	
	return t_min;
}

bool
MCScriptEvaluateHandlerInInstanceInternal(MCScriptInstanceRef p_instance,
										  MCScriptCommonHandlerDefinition *p_handler_def,
										  MCHandlerRef& r_handler)
{
	// Compute the index in the handler table of p_handler; then, if it is the
	// definition we are looking for, return its previously computed value.
	uindex_t t_index;
	t_index = __MCScriptComputeHandlerIndexInInstance(p_instance,
													  p_handler_def);
	if (t_index < p_instance->handler_count &&
		p_instance->handlers[t_index].definition == p_handler_def)
	{
		r_handler = p_instance->handlers[t_index] . value;
		return true;
	}
	
	MCTypeInfoRef t_signature;
	t_signature = p_instance->module->types[p_handler_def->type]->typeinfo;
	
	MCAutoValueRefBase<MCHandlerRef> t_handler;
	if (!__MCScriptInternalHandlerCreate(p_instance,
										 p_handler_def,
										 &t_handler))
	{
		return false;
	}
	
	// Now put the handler value into the instance's handler list. First we make
	// space in the array, then insert the value at the index we computed at the
	// start.
	if (!MCMemoryResizeArray(p_instance->handler_count + 1,
							 p_instance->handlers,
							 p_instance->handler_count))
	{
		return false;
	}
	
	MCMemoryMove(p_instance->handlers + t_index + 1,
				 p_instance->handlers + t_index,
				 (p_instance->handler_count - t_index - 1) * sizeof(MCScriptHandlerValue));
	
	p_instance->handlers[t_index].definition = p_handler_def;
	p_instance->handlers[t_index].value = t_handler.Take();
	
	r_handler = p_instance->handlers[t_index].value;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCScriptModuleRef MCScriptGetCurrentModule(void)
{
	return s_current_module;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" bool MC_DLLEXPORT_DEF MCScriptBuiltinRepeatCounted(uinteger_t *x_count)
{
    if (*x_count == 0)
        return false;
    
    *x_count -= 1;
    return true;
}

extern "C" bool MC_DLLEXPORT_DEF MCScriptBuiltinRepeatUpToCondition(double p_counter, double p_limit)
{
    return p_counter <= p_limit;
}

extern "C" double MC_DLLEXPORT_DEF MCScriptBuiltinRepeatUpToIterate(double p_counter, double p_step)
{
    return p_counter + p_step;
}

extern "C" bool MC_DLLEXPORT_DEF MCScriptBuiltinRepeatDownToCondition(double p_counter, double p_limit)
{
    return p_counter >= p_limit;
}

extern "C" double MC_DLLEXPORT_DEF MCScriptBuiltinRepeatDownToIterate(double p_counter, double p_step)
{
    return p_counter + p_step;
}

extern "C" void MC_DLLEXPORT_DEF MCScriptBuiltinThrow(MCStringRef p_reason)
{
    MCErrorThrowGeneric(p_reason);
}

////////////////////////////////////////////////////////////////////////////////
