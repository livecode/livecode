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

#include <libscript/script.h>

#include "script-private.h"

class MCScriptValidateContext
{
public:
	// Quering Methods
	//
	// These methods return information about the current bytecode operation
	// being processed.
	
	// Return the address of the current opcode
	uindex_t GetAddress(void) const;
	
	// Return the number of arguments to the current opcode
	uindex_t GetArity(void) const;
	
	// Get the argument at the given index as unsigned
	uindex_t GetArgument(uindex_t index) const;
	
	// Get the argument at the given index as signed
	index_t GetSignedArgument(uindex_t index) const;
	
	// Validating Methods
	//
	// These methods validate various aspects of the bytecode. The context
	// stores whether a previous validation failed, and takes appropriate
	// action. This means there is no need to check return values in a
	// Validate method.
	
	// Check the number of arguments is as specified, returning success or not.
	void CheckArity(uindex_t expected_arity);
	
	// Check the given address is valid - i.e. is within the current handler
	// and targets the start of an opcode.
	void CheckAddress(uindex_t target_address);
	
	// Check the given index is a valid register.
	// Note: At the point of checking, a maximum upper bound is used to check
	// the register index is 'sane'. At the end of validation an extra check
	// is performed to ensure that register indicies form a dense sequence from
	// 0..n where n is the maximum register index specified in the bytecode.
	void CheckRegister(uindex_t register_index);
	
	// Check the given index is a valid constant index
	void CheckConstant(uindex_t constant_index);
	
	// Check the given index is a valid handler index. A valid handler index
	// can be of type Handler, ForeignHandler, or DefinitionGroup (of handlers).
	void CheckHandler(uindex_t handler_index);
	
	// Reporting Methods
	//
	// These report the validation errors which can occur.
	
	void ReportInvalidArity(void);
};

