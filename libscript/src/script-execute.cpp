/* Copyright (C) 2003-2016 LiveCode Ltd.
 
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

#include "libscript/script.h"
#include "script-private.h"

#include "ffi.h"

#include "foundation-auto.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "script-execute.hpp"

//////////

bool
MCScriptExecuteContext::TryToBindForeignHandler(MCScriptInstanceRef p_instance,
												MCScriptForeignHandlerDefinition* p_handler_def,
												bool& r_bound)
{
	return false;
}


void
MCScriptExecuteContext::InvokeForeign(MCScriptInstanceRef p_instance,
									  MCScriptForeignHandlerDefinition *p_definition,
									  uindex_t p_result_reg,
									  const uindex_t *p_argument_regs,
									  uindex_t p_argument_count)
{
}

//////////

// Hander values are stored in a table in the instance, and are owned
// by the instance - this is so that the lifetime of the handler value
// is known, and so if a C function ptr (closure) is generated for one
// it will last as long as the instance to which it points (the
// maximum length it can).
bool
MCScriptExecuteContext::EvaluateHandler(MCScriptInstanceRef p_instance,
										MCScriptCommonHandlerDefinition *p_handler_def,
										MCHandlerRef& r_handler)
{
	// Compute the index in the handler table of p_handler; then, if it is the
	// definition we are looking for, return its previously computed value.
	uindex_t t_index =
		ComputeHandlerIndexInInstance(p_instance,
									  p_handler_def);
	if (t_index < p_instance->handler_count &&
		p_instance->handlers[t_index].definition == p_handler_def)
	{
		r_handler = p_instance->handlers[t_index].value;
	}
	return false;
}

//////////

void
MCScriptExecuteContext::Unwind(void)
{
	// If there is no error on the current thread, then something is
	// failing without throwing; thus we throw an error to represent
	// this situation so it can be easily traced (in LCB, at least!).
	if (!MCErrorIsPending())
	{
		ThrowErrorExpected();
	}
	
	// Now we should definitely have an error, so we catch it.
	// Note: This call will only fail if there is no error.
	MCAutoErrorRef t_error;
	if (!MCErrorCatch(&t_error))
	{
		__MCScriptUnreachable__("catch failed to return error");
		return;
	}
	
	// Now we loop back through the frames, adding the backtrace
	// information as we go.

	// Position information is stored in modules as a list of triples
	// (address, file, line) sorted by address. The address is that
	// of the first instruction generated for a given source line. Each
	// frame stores the return_address which is the address of the
	// instruction after the invoke which caused the new frame. Therefore
	// to find the position corresponding to each frame, we look
	// for the last position whose address is strictly less than the
	// return_address.
	
	// As the address of the current frame is stored in the context,
	// we fetch it here, and replace by the return_address as we step
	// back up the frames.
	uindex_t t_address;
	t_address = GetNextAddress();
	while(m_frame != nil)
	{
		const MCScriptModuleRef t_module =
			m_frame->instance->module;
		
		// If the module of the frame has debug info then we add the
		// source info, otherwise we just skip the frame.
		if (t_module->position_count > 0)
		{
			uindex_t t_index;
			for(t_index = 0; t_index < t_module->position_count - 1; t_index++)
			{
				if (t_module->positions[t_index + 1].address >= t_address)
				{
					break;
				}
			}
			
			const MCScriptPosition *t_position =
				&t_module->positions[t_index];
			
			// If unwinding fails (due to oom), then we cease processing
			// and drop the error, allowing the new error to be propagate.
			if (!MCErrorUnwind(*t_error,
							   t_module->source_files[t_position->file],
							   t_position->line,
							   1))
			{
				t_error.Reset();
				break;
			}
		}
		
		// Move to the calling frame.
		Frame *t_current_frame = m_frame;
		m_frame = m_frame->caller;
		t_address = t_current_frame->return_address;
		delete t_current_frame;
	}
	
	// If an error occurred whilst unwinding, another error will have been
	// thrown, so we just return.
	if (*t_error == nil)
	{
		return;
	}
	
	// Rethrow the error that we have unwound.
	MCErrorThrow(*t_error);
}

//////////

const MCHandlerCallbacks MCScriptExecuteContext::kInternalHandlerCallbacks =
{
	sizeof(MCScriptExecuteContext::InternalHandlerContext),
	MCScriptExecuteContext::InternalHandlerRelease,
	MCScriptExecuteContext::InternalHandlerInvoke,
	MCScriptExecuteContext::InternalHandlerDescribe,
	
};

bool
MCScriptExecuteContext::InternalHandlerInvoke(void *p_context,
											  MCValueRef *p_arguments,
											  uindex_t p_argument_count,
											  MCValueRef& r_result)
{
	InternalHandlerContext *context =
		(InternalHandlerContext *)p_context;
	
	if (context -> definition -> kind != kMCScriptDefinitionKindHandler)
		return MCErrorThrowGeneric(MCSTR("out-of-frame indirect foreign handler calls not yet supported"));
	
	return MCScriptCallHandlerOfInstanceDirect(context -> instance,
											   static_cast<MCScriptHandlerDefinition *>(context -> definition),
											   p_arguments,
											   p_argument_count,
											   r_result);
}

void
MCScriptExecuteContext::InternalHandlerRelease(void *p_context)
{
	InternalHandlerContext *context =
		(InternalHandlerContext *)p_context;
	MCScriptReleaseInstance(context -> instance);
}

bool
MCScriptExecuteContext::InternalHandlerDescribe(void *p_context,
												MCStringRef & r_desc)
{
	InternalHandlerContext *context =
		(InternalHandlerContext *)p_context;
	
	MCScriptModuleRef t_module;
	t_module = MCScriptGetModuleOfInstance (context->instance);
	
	MCNameRef t_module_name;
	t_module_name = MCScriptGetNameOfModule (t_module);
	
	MCNameRef t_handler_name;
	t_handler_name = MCScriptGetNameOfDefinitionInModule(t_module,
														 context->definition);
	
	return MCStringFormat(r_desc, "%@.%@()", t_module_name, t_handler_name);
}
