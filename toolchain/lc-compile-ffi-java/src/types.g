/* Copyright (C) 2016 LiveCode Ltd.
 
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

'module' types

'use'

'export'
	PACKAGE PACKAGELIST
	DEFINITION
    MODIFIER
    SIGNATURE
    TYPE TYPELIST
    BOUNDS
    EXPRESSION
    PARAMETER PARAMETERLIST
    ID QUALIFIEDNAME OPTIONALID
    SYMBOLKIND SYMBOLINFO
    PACKAGEINFO
    MEANING NAMELIST
    NAME DOUBLE

--------------------------------------------------------------------------------

'type' PACKAGE
	package(Position: POS, Name: ID, Definitions: DEFINITION)

'type' PACKAGELIST
	packagelist(Head: PACKAGE, Tail: PACKAGELIST)
	nil

'type' DEFINITION
	sequence(Head: DEFINITION, Tail: DEFINITION)
    use(Position: POS, Name: ID)
    class(Position: POS, Modifiers: MODIFIER, Type: TYPE, Definitions: DEFINITION, Inherits: TYPELIST, Implements: TYPELIST)
    interface(Position: POS, Type: TYPE, Definitions: DEFINITION, Inherits: TYPELIST)
    constructor(Position: POS, Modifiers: MODIFIER, Name: ID, Signature: SIGNATURE, Alias: OPTIONALID)
    method(Position: POS, Modifiers: MODIFIER, Name: ID, Signature: SIGNATURE, Alias: OPTIONALID, Throws: OPTIONALID)
    variable(Position: POS, Modifiers: MODIFIER, Name: ID, Type: TYPE)
    constant(Position: POS, Name: ID, Type: TYPE, Value: EXPRESSION)
    nil

'type' MODIFIER
    classmodifiers(Access: MODIFIER, StrictFP: MODIFIER, Inherit: MODIFIER, Modify: MODIFIER, Instance: MODIFIER)
	constructormodifiers(Access: MODIFIER)
	methodmodifiers(Access: MODIFIER, Sync: MODIFIER, Native: MODIFIER, StrictFP: MODIFIER, Inherit: MODIFIER, Instance: MODIFIER)
	variablemodifiers(Access: MODIFIER, Transient: MODIFIER, Modify: MODIFIER, Instance: MODIFIER)
	interfacemethodmodifiers(Modifier: MODIFIER)
	public
	protected
	synchronized
	native
	strictfp
	abstract
	final
	class
	volatile
	transient
	default
	inferred

'type' SIGNATURE
    signature(Parameters: PARAMETERLIST, ReturnType: TYPE)

'type' TYPE
    byte(Position: POS)
    short(Position: POS)
    int(Position: POS)
    long(Position: POS)
    float(Position: POS)
    double(Position: POS)
    boolean(Position: POS)
    char(Position: POS)
    string(Position: POS)
    jarray(Position: POS, Type: TYPE, Dimension: INT)
    named(Position: POS, Name: ID, Parameters: TYPELIST)
    template(Position: POS, Name: ID, Parameters: TYPELIST)
    placeholder(Position: POS, Name: ID)
    wildcard(Position: POS, Bounds: BOUNDS)
    nil

'type' TYPELIST
    typelist(Head: TYPE, Tail: TYPELIST)
    nil

'type' PARAMETERLIST
    parameterlist(Head: PARAMETER, Tail: PARAMETERLIST)
    nil
    
'type' PARAMETER
    parameter(Position: POS, Name: ID, Type: TYPE)
    variadic(Position: POS)

'type' EXPRESSION
    true(Position: POS)
    false(Position: POS)
    integer(Position: POS, Value: INT)
    real(Position: POS, Value: DOUBLE)
    string(Position: POS, Value: STRING)
    nil

'type' MEANING
    definingid(Id: ID)
    package(Info: PACKAGEINFO)
    symbol(Info: SYMBOLINFO)    
    error
    nil

'type' QUALIFIEDNAME
    package(Name: NAME, Tail: QUALIFIEDNAME)
    qualified(Name: NAME, Qualifier: QUALIFIEDNAME)
    unqualified(Name: NAME)

'type' OPTIONALID
    id(Id: ID)
    nil

'type' NAMELIST
    namelist(Name: NAME, Rest: NAMELIST)
    nil

'type' SYMBOLKIND
    package
    constant
    variable
    parameter
	method
	constructor
	class
    interface
    placeholder

'type' BOUNDS
    unbounded
    upper(Type: TYPE)
    lower(Type: TYPE)

'table' PACKAGEINFO(Index: INT, Generator: INT)
'table' SYMBOLINFO(Index: INT, Generator: INT, Parent: ID, Modifiers: MODIFIER, Kind: SYMBOLKIND, Type: TYPE)

'table' ID(Position: POS, Name: QUALIFIEDNAME, Meaning: MEANING)

'type' NAME
'type' DOUBLE

--------------------------------------------------------------------------------
