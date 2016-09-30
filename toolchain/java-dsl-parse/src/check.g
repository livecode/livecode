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

'module' check

'export'
    Check

'use'
    support
    types

--------------------------------------------------------------------------------

'action' Check(PACKAGE)

    'rule' Check(Package):
        -- Check that all the ids bind to the correct type of thing. Any ids
        -- which aren't bound correctly get assigned a meaning of 'error'.
        CheckBindings(Package)

--------------------------------------------------------------------------------

-- At this point all identifiers either have a defined meaning, or are defined
-- to be a pointer to the definingid. The next step is to check that bindings
-- are appropriate to each id:
--   BT1) TYPE'named(_, _, Parameters) - reference to a named type must have matching
--   template parameters

'sweep' CheckBindings(ANY)
    --

    'rule' CheckBindings(DEFINITION'constant(Position, _, Type, Value)):
        (|
            where(Value -> nil)
        ||
            IsExpressionSimpleConstant(Value)
        ||
            Error_ConstantsMustBeSimple(Position)
        |)
        CheckBindings(Type)

    'rule' CheckBindings(DEFINITION'class(Position, _, _, Definitions, Inherits, Implements)):
        (|
        /* BD2 */ CheckBindingIsClass(Inherits)
        ||
            Error_ClassesMayOnlyInheritFromClasses(Position)
        |)

        (|
        /* BD2 */ CheckBindingIsInterface(Implements)
        ||
            Error_ClassesMayOnlyImplementInterfaces(Position)
        |)
        CheckBindings(Inherits)
        CheckBindings(Implements)
        CheckBindings(Definitions)

    'rule' CheckBindings(DEFINITION'interface(Position, _, Definitions, Inherits)):
        (|
        /* BD2 */ CheckBindingIsInterface(Inherits)
        ||
            Error_InterfacesMayOnlyInheritFromInterfaces(Position)
        |)
        CheckBindings(Inherits)
        CheckBindings(Definitions)

    'rule' CheckBindings(TYPE'named(Position, Id, Parameters)):
        (|
            QuerySymbolId(Id -> Info)
            Info'Type -> Type
            (|
                where(Type -> nil)
            ||
                CheckParameterMatches(named(Position, Id, Parameters), Type)
            |)
        ||
            Error_GenericTypeMismatch(Position)
        |)

'condition' CheckParametersMatch(TYPELIST, TYPELIST)

    'rule' CheckParametersMatch(nil, nil):

    'rule' CheckParametersMatch(typelist(LeftHead, LeftTail), typelist(RightHead, RightTail)):
        CheckParameterMatches(LeftHead, RightHead)
        CheckParametersMatch(LeftTail, RightTail)

'condition' CheckParameterMatches(TYPE, TYPE)

    'rule' CheckParameterMatches(nil, nil):

    'rule' CheckParameterMatches(named(_, LeftId, LeftParameters), template(_, RightId, RightParameters)):
        QuerySymbolId(LeftId -> LeftInfo)
        LeftInfo'Index -> LeftIndex
        QuerySymbolId(RightId -> RightInfo)
        RightInfo'Index -> RightIndex

        eq(LeftIndex, RightIndex)

        CheckParametersMatch(LeftParameters, RightParameters)

    'rule' CheckParameterMatches(named(_, _, _), placeholder(_, _)):

    'rule' CheckParameterMatches(placeholder(_, _), placeholder(_, _)):

    'rule' CheckParameterMatches(wildcard(_, _), template(_, _, _)):

    'rule' CheckParameterMatches(wildcard(_, _), placeholder(_, _)):

'sweep' CheckBindingsOfConstantExpression(ANY)

    'rule' CheckBindingsOfConstantExpression(EXPRESSION'nil):
        -- TODO

'condition' CheckBindingIsClass(TYPELIST)

    'rule' CheckBindingIsClass(nil):

    'rule' CheckBindingIsClass(typelist(Head, Tail)):
        CheckNamedTypeIsOfKind(Head, class)
        CheckBindingIsClass(Tail)

'condition' CheckBindingIsInterface(TYPELIST)

    'rule' CheckBindingIsInterface(nil):

    'rule' CheckBindingIsInterface(typelist(Head, Tail)):
        CheckNamedTypeIsOfKind(Head, interface)
        CheckBindingIsInterface(Tail)

'condition' CheckNamedTypeIsOfKind(TYPE, SYMBOLKIND)

    'rule' CheckNamedTypeIsOfKind(named(_, Id, _), _):
        -- Do nothing if the meaning is error.
        QueryId(Id -> error)

    'rule' CheckNamedTypeIsOfKind(named(_, Id, _), Symbol):
        QueryKindOfSymbolId(Id -> SymbolKind)
        eq(Symbol, SymbolKind)

'condition' IsExpressionSimpleConstant(EXPRESSION)

    'rule' IsExpressionSimpleConstant(true(_)):

    'rule' IsExpressionSimpleConstant(false(_)):

    'rule' IsExpressionSimpleConstant(integer(_, _)):

    'rule' IsExpressionSimpleConstant(real(_, _)):

    'rule' IsExpressionSimpleConstant(string(_, _)):

--------------------------------------------------------------------------------

'condition' QueryKindOfSymbolId(ID -> SYMBOLKIND)

    'rule' QueryKindOfSymbolId(Id -> Kind):
        QueryId(Id -> Meaning)
        where(Meaning -> symbol(Info))
        Info'Kind -> Kind

'action' QueryId(ID -> MEANING)

    'rule' QueryId(Id -> Meaning):
        Id'Meaning -> definingid(DefId)
        DefId'Meaning -> Meaning
        
    'rule' QueryId(Id -> Meaning):
        Id'Meaning -> Meaning

'action' QueryPackageOfId(ID -> ID)

    'rule' QueryPackageOfId(Id -> PackageId):
        QuerySymbolId(Id -> Info)
        Info'Parent -> PackageId

'condition' QuerySymbolId(ID -> SYMBOLINFO)

    'rule' QuerySymbolId(Id -> Info):
        QueryId(Id -> symbol(Info))

'condition' QueryPackageId(ID -> PACKAGEINFO)

    'rule' QueryPackageId(Id -> Info):
        QueryId(Id -> package(Info))

'action' ResolveIdName(ID -> NAME)

    'rule' ResolveIdName(Id -> Name)
        Id'Name -> QualifiedName
        GetUnqualifiedIdName(QualifiedName -> Name)

'action' ResolveIdQualifiedName(ID -> NAME)

    'rule' ResolveIdQualifiedName(Id -> Name)
        QuerySymbolId(Id -> SymbolInfo)
        Id'Name -> QualifiedName
        SymbolInfo'Parent -> ParentId
        GetQualifiedName(ParentId, QualifiedName -> Name)

    'rule' ResolveIdQualifiedName(Id -> Name)
        Id'Name -> QName
        QName'Name -> Name


'action' GetQualifiedName(ID, QUALIFIEDNAME -> NAME)

    'rule' GetQualifiedName(ParentId, unqualified(UnqName) -> Name)
        ResolveIdName(ParentId -> ParentName)
        ConcatenateNameParts(ParentName, UnqName -> Name)


'action' GetUnqualifiedIdName(QUALIFIEDNAME -> NAME)
'action' GetQualifiedIdName(QUALIFIEDNAME -> NAME)
