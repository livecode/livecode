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

'module' bind

'use'
    types
    support
    
'export'
    Bind

--------------------------------------------------------------------------------

-- The purpose of the 'Bind' phase is to ensure every Id is assigned either a
-- reference to the definingid() for its meaning, or the actual meaning if it is
-- the defining id.
'action' Bind(PACKAGE, PACKAGELIST)

    'rule' Bind(Package:package(Position, Name, Definitions), ImportedPackages):
        DefinePackageId(Name)

        -- Make sure all the imported modules are bound
        BindImports(Definitions, ImportedPackages)
        (|
            ErrorsDidOccur()
        ||
            EnterScope

            -- Step 1: Ensure all id's referencing definitions point to the definition.
            --         and no duplicate definitions have been attempted.

            -- Import all the used modules
            DeclareImports(Definitions, ImportedPackages)

            -- Assign the defining id to all top-level names.
            Declare(Definitions)
            
            -- Resolve all references to id's.
            Apply(Definitions)

            LeaveScope

            -- Step 2: Ensure all definitions have their appropriate meaning
            Define(Name, Definitions)
            
            --DumpBindings(Package)
        |)

'action' BindImports(DEFINITION, PACKAGELIST)

    'rule' BindImports(sequence(Left, Right), Imports):
        BindImports(Left, Imports)
        BindImports(Right, Imports)
        
    'rule' BindImports(use(Position, Id), Imports):
        Id'Name -> QualifiedName
        GetPackageIdName(QualifiedName -> Name)
        (|
            FindPackageInList(Name, Imports -> Package)
            Package'Name -> PackageId
            (|
                QueryId(PackageId -> package(Info))
            ||
                DefinePackageId(PackageId)
                Bind(Package, Imports)
            |)
        ||
            GetUnqualifiedIdName(Name -> PackageName)
            Error_UnableToFindImportedPackage(Position, PackageName)
        |)
        
    'rule' BindImports(_, _):
        -- do nothing
--

'action' DeclareImports(DEFINITION, PACKAGELIST)

    'rule' DeclareImports(sequence(Left, Right), Imports):
        DeclareImports(Left, Imports)
        DeclareImports(Right, Imports)
        
    'rule' DeclareImports(use(Position, Id), Imports):
        Id'Name -> QualifiedName
        GetPackageIdName(QualifiedName -> PackageName)
        GetUnqualifiedIdName(PackageName -> FullPackageName)
        GetUnqualifiedIdName(QualifiedName -> Name)
        (|
            FindPackageInList(PackageName, Imports -> Package)
            Package'Definitions -> Definitions
            (|
                FindDefinitionInPackage(Name, Definitions -> Definition)
                DeclareImportedDefinitions(Definition)
            ||
                ConcatenateNameParts(FullPackageName, Name -> QualifiedDefName)
                Error_UnableToFindImportedDefinition(Position, QualifiedDefName)
            |)
        ||
            Error_UnableToFindImportedPackage(Position, FullPackageName)
        |)
        
    'rule' DeclareImports(_, _):
        -- do nothing

'action' DeclareImportedDefinitions(DEFINITION)

    'rule' DeclareImportedDefinitions(sequence(Left, Right)):
        DeclareImportedDefinitions(Left)
        DeclareImportedDefinitions(Right)

    'rule' DeclareImportedDefinitions(constant(Position, Name, _, _)):
        DeclareId(Name)
    
    'rule' DeclareImportedDefinitions(variable(Position, _, Name, _)):
        DeclareId(Name)

    'rule' DeclareImportedDefinitions(method(Position, _, Name, _, _, _)):
        DeclareId(Name)

    'rule' DeclareImportedDefinitions(constructor(Position, _, Name, _, _)):
        DeclareId(Name)
        
    'rule' DeclareImportedDefinitions(class(Position, _, template(_, Name, _), _, _, _)):
        DeclareId(Name)
        
    'rule' DeclareImportedDefinitions(interface(Position, template(_, Name, _), _, _)):
        DeclareId(Name)

    'rule' DeclareImportedDefinitions(use(_, _)):
        -- do nothing

    'rule' DeclareImportedDefinitions(nil):
        -- do nothing

--

-- These simply extract the relevant data from
-- a qualified name if it is of the correct type

'action' GetPackageIdName(QUALIFIEDNAME -> QUALIFIEDNAME)

    'rule' GetPackageIdName(qualified(Name, Qualifier) -> Qualifier):
        
'action' GetUnqualifiedIdName(QUALIFIEDNAME -> NAME)

    'rule' GetUnqualifiedIdName(qualified(Name, Qualifier) -> Name):

    'rule' GetUnqualifiedIdName(unqualified(Name) -> Name):
    
    'rule' GetUnqualifiedIdName(package(Name, Tail) -> PackageName):
        GetUnqualifiedIdName(Tail -> TailName)
        ConcatenateNameParts(Name, TailName -> PackageName)

'action' GetQualifiedIdName(QUALIFIEDNAME -> NAME)

    'rule' GetQualifiedIdName(qualified(Name, Qualifier) -> Name):
        GetQualifiedIdName(Qualifier -> HeadName)
        ConcatenateNameParts(HeadName, Name -> QualifiedName)

    'rule' GetQualifiedIdName(package(Name, Tail) -> PackageName):
        GetUnqualifiedIdName(Tail -> TailName)
        ConcatenateNameParts(Name, TailName -> PackageName)

    'rule' GetQualifiedIdName(unqualified(Name) -> Name):
        -- todo - don't do this
--

'condition' FindPackageInList(QUALIFIEDNAME, PACKAGELIST -> PACKAGE)

    'rule' FindPackageInList(Name, packagelist(Head, Rest) -> Head):
        Head'Name -> Id
        Id'Name -> PkgName
        IsQualifiedNameEqualToName(Name, PkgName)
        
    'rule' FindPackageInList(Name, packagelist(_, Rest) -> Found):
        FindPackageInList(Name, Rest -> Found)

'condition' IsQualifiedNameEqualToName(QUALIFIEDNAME, QUALIFIEDNAME)

    'rule' IsQualifiedNameEqualToName(package(LeftName, LeftTail), package(RightName, RightTail)):
        IsNameEqualToName(LeftName, RightName)
        IsQualifiedNameEqualToName(LeftTail, RightTail)

    'rule' IsQualifiedNameEqualToName(qualified(LeftName, LeftTail), qualified(RightName, RightTail)):
        IsNameEqualToName(LeftName, RightName)
        IsQualifiedNameEqualToName(LeftTail, RightTail)

    'rule' IsQualifiedNameEqualToName(unqualified(LeftName), unqualified(RightName)):
        IsNameEqualToName(LeftName, RightName)

'condition' FindDefinitionInPackage(NAME, DEFINITION -> DEFINITION)

    'rule' FindDefinitionInPackage(Name, sequence(Head, Tail) -> Head):
        IsNameOfDefinition(Name, Head)
        
    'rule' FindDefinitionInPackage(Name, sequence(_, Tail) -> Found):
        FindDefinitionInPackage(Name, Tail -> Found)

'condition' IsNameOfDefinition(NAME, DEFINITION)

    'rule' IsNameOfDefinition(Name, constant(_, Id, _, _)):
        Id'Name -> QualifiedName
        GetUnqualifiedIdName(QualifiedName -> DefName)
        IsNameEqualToName(Name, DefName)
    
    'rule' IsNameOfDefinition(Name, variable(_, _, Id, _)):
        Id'Name -> QualifiedName
        GetUnqualifiedIdName(QualifiedName -> DefName)
        IsNameEqualToName(Name, DefName)

    'rule' IsNameOfDefinition(Name, method(_, _, Id, _, Alias, _)):
        (|
            where(Alias -> id(AliasId))
            AliasId'Name -> QualifiedName
        ||
            Id'Name -> QualifiedName
        |)

        GetUnqualifiedIdName(QualifiedName -> DefName)
        IsNameEqualToName(Name, DefName)

    'rule' IsNameOfDefinition(Name, constructor(_, _, Id, _, Alias)):
        (|
            where(Alias -> id(AliasId))
            AliasId'Name -> QualifiedName
        ||
            Id'Name -> QualifiedName
        |)
        GetUnqualifiedIdName(QualifiedName -> DefName)
        IsNameEqualToName(Name, DefName)
        
    'rule' IsNameOfDefinition(Name, class(_, _, template(_, Id, _), _, _, _)):
        Id'Name -> QualifiedName
        GetUnqualifiedIdName(QualifiedName -> DefName)
        IsNameEqualToName(Name, DefName)

    'rule' IsNameOfDefinition(Name, interface(_, template(_, Id, _), _, _)):
        Id'Name -> QualifiedName
        GetUnqualifiedIdName(QualifiedName -> DefName)
        IsNameEqualToName(Name, DefName)

'action' QueryId(ID -> MEANING)

--------------------------------------------------------------------------------

-- The 'Declare' phase associates the meanings at the current scope with the
-- identifier which defines them.
--
-- It catches multiple definition errors and shadow warnings.
--
'action' Declare(DEFINITION)

    'rule' Declare(sequence(Left, Right)):
        Declare(Left)
        Declare(Right)

    'rule' Declare(constant(Position, Name, _, _)):
        DeclareId(Name)
    
    'rule' Declare(variable(Position, _, Name, _)):
        DeclareId(Name)

    'rule' Declare(method(Position, _, Name, _, Alias, _)):
        (|
            where(Alias -> id(AliasName))
            DeclareId(AliasName)
        ||
            DeclareId(Name)
        |)

    'rule' Declare(constructor(Position, _, Name, _, Alias)):
        (|
            where(Alias -> id(AliasName))
            DeclareId(AliasName)
        ||
            DeclareId(Name)
        |)
        
    'rule' Declare(class(Position, _, template(_, Name, _), Definitions, _, _)):
        DeclareId(Name)
        
    'rule' Declare(interface(Position, template(_, Name, _), Definitions, _)):
        DeclareId(Name)

    'rule' Declare(use(_, _)):
        -- do nothing

    'rule' Declare(nil):
        -- do nothing

'action' DeclareParameters(PARAMETERLIST)

    'rule' DeclareParameters(parameterlist(Head, Tail)):
        DeclareParameter(Head)
        DeclareParameters(Tail)
        
    'rule' DeclareParameters(nil):
        -- do nothing

'action' DeclareParameter(PARAMETER)

    'rule' DeclareParameter(parameter(_, Id, _)):
        DeclareId(Id)

    'rule' DeclareParameter(variadic(_)):

'action' DeclareTemplateParameters(TYPELIST)

    'rule' DeclareTemplateParameters(typelist(Head, Tail)):
        DeclareTemplateParameter(Head)
        DeclareTemplateParameters(Tail)

    'rule' DeclareTemplateParameters(nil):
        -- do nothing

'action' DeclareTemplateParameter(TYPE)

    'rule' DeclareTemplateParameter(template(_, Id, Parameters)):
        DeclareTemplateParameters(Parameters)

    'rule' DeclareTemplateParameter(placeholder(_, Id)):
        DeclareId(Id)

--------------------------------------------------------------------------------

-- The 'Define' phase associates meanings with the definining ids.
--
'action' Define(ID, DEFINITION)

    'rule' Define(PackageId, sequence(Left, Right)):
        Define(PackageId, Left)
        Define(PackageId, Right)
    
    'rule' Define(PackageId, constant(Position, Name, Type, Value)):
        DefineSymbolId(Name, inferred, PackageId, constant, Type)
    
    'rule' Define(PackageId, variable(Position, Modifiers, Name, Type)):
        DefineSymbolId(Name, Modifiers, PackageId, variable, Type)

    'rule' Define(PackageId, method(Position, Modifiers, Name, signature(Params, Returns), Alias, Throws)):
        (|
            where(Alias -> id(AliasName))
            DefineSymbolId(AliasName, Modifiers, PackageId, method, Returns)
        ||
            DefineSymbolId(Name, Modifiers, PackageId, method, Returns)
        |)
        DefineParameters(PackageId, Params)

    'rule' Define(PackageId, constructor(Position, Modifiers, Name, signature(Params, _), Alias)):
        (|
            where(Alias -> id(AliasName))
            DefineSymbolId(AliasName, Modifiers, PackageId, constructor, nil)
        ||
            DefineSymbolId(Name, Modifiers, PackageId, constructor, nil)
        |)
        DefineParameters(PackageId, Params)
        
    'rule' Define(PackageId, class(Position, Modifiers, template(TypePosition, Name, Parameters), Definitions, _, _)):
        DefineSymbolId(Name, Modifiers, PackageId, class, template(TypePosition, Name, Parameters))
        Define(PackageId, Definitions)
        (|
            where(Parameters -> nil)
        ||
            DefineTemplateParameters(PackageId, Parameters)
        |)

    'rule' Define(PackageId, interface(Position, template(TypePosition, Name, Parameters), Definitions, _)):
        DefineSymbolId(Name, inferred, PackageId, interface, template(TypePosition, Name, Parameters))
        Define(PackageId, Definitions)
        (|
            where(Parameters -> nil)
        ||
            DefineTemplateParameters(PackageId, Parameters)
        |)


    'rule' Define(_, use(_, _)):
        -- do nothing
        
    'rule' Define(_, nil):
        -- do nothing

'action' DefineTemplateParameters(ID, TYPELIST)

    'rule' DefineTemplateParameters(PackageId, typelist(Head, Tail)):
        DefineTemplateParameter(PackageId, Head)
        DefineTemplateParameters(PackageId, Tail)

    'rule' DefineTemplateParameters(PackageId, nil):

'action' DefineTemplateParameter(ID, TYPE)

    'rule' DefineTemplateParameter(PackageId, template(_, Id, Parameters)):
        DefineTemplateParameters(PackageId, Parameters)

    'rule' DefineTemplateParameter(PackageId, placeholder(_, Id)):
        DefineSymbolId(Id, inferred, PackageId, placeholder, nil)

    'rule' DefineTemplateParameter(PackageId, nil):

'action' DefineParameters(ID, PARAMETERLIST)

    'rule' DefineParameters(PackageId, parameterlist(Parameter, Tail)):
        DefineParameter(PackageId, Parameter)
        DefineParameters(PackageId, Tail)

    'rule' DefineParameters(PackageId, nil):
        -- do nothing

'action' DefineParameter(ID, PARAMETER)

    'rule' DefineParameter(PackageId, parameter(_, Name, Type)):
        DefineSymbolId(Name, inferred, PackageId, parameter, Type)

    'rule' DefineParameter(PackageId, variadic(_)):
        -- do nothing

--------------------------------------------------------------------------------

'sweep' Apply(ANY)

    ----------

    'rule' Apply(DEFINITION'use(_, Id)):
        ApplyId(Id)

    'rule' Apply(DEFINITION'class(_, _, template(_, Id, Parameters), Definitions, Inherits, Implements)):
        ApplyId(Id)

        Apply(Inherits)
        Apply(Implements)

        EnterScope

        -- Apply template parameters in scope of class

        DeclareTemplateParameters(Parameters)

        Apply(Parameters)

        Declare(Definitions)

        Apply(Definitions)

        LeaveScope

    'rule' Apply(DEFINITION'interface(_, template(_, Id, Parameters), Definitions, Inherits)):
        ApplyId(Id)

        Apply(Inherits)

        EnterScope

        -- Apply template parameters in scope of class

        DeclareTemplateParameters(Parameters)

        Apply(Parameters)

        Declare(Definitions)

        Apply(Definitions)

        LeaveScope

    'rule' Apply(DEFINITION'constant(_, Id, Type, _)):
        ApplyId(Id)
        Apply(Type)

    'rule' Apply(DEFINITION'variable(_, _, Id, Type)):
        ApplyId(Id)
        Apply(Type)

    'rule' Apply(DEFINITION'method(_, _, Id, signature(Params, Returns), Alias, Throws)):
        (|
            where(Alias -> id(AliasName))
            ApplyId(AliasName)
        ||
            ApplyId(Id)
        |)

        EnterScope

        DeclareParameters(Params)

        Apply(Params)

        LeaveScope

        ApplyParamTypes(Params)

        Apply(Returns)

        Apply(Throws)

    'rule' Apply(DEFINITION'constructor(_, _, Id, signature(Params, _), Alias)):
        (|
            where(Alias -> id(AliasName))
            ApplyId(AliasName)
        ||
            ApplyId(Id)
        |)

        EnterScope

        DeclareParameters(Params)

        Apply(Params)

        LeaveScope

        ApplyParamTypes(Params)

    'rule' Apply(TYPE'named(_, Name, Parameter)):
        ApplyId(Name)
        Apply(Parameter)

    'rule' Apply(TYPE'jarray(_, named(_, Name, Parameter), _)):
        ApplyId(Name)
        Apply(Parameter)

    'rule' Apply(TYPE'template(_, Name, Parameters)):
        ApplyId(Name)
        Apply(Parameters)

    'rule' Apply(TYPE'placeholder(_, Name)):
        ApplyId(Name)

    'rule' Apply(PARAMETER'parameter(_, Id, _)):
        ApplyId(Id)

    'rule' Apply(OPTIONALID'id(Id)):
        ApplyId(Id)

    'rule' Apply(PARAMETERLIST'parameterlist(Head, Tail)):
        Apply(Head)
        Apply(Tail)

'action' ApplyParamTypes(PARAMETERLIST)

    'rule' ApplyParamTypes(parameterlist(Head, Tail)):
        ApplyParamType(Head)
        ApplyParamTypes(Tail)

    'rule' ApplyParamTypes(nil):

'action' ApplyParamType(PARAMETER)

    'rule' ApplyParamType(parameter(_, _, Type)):
        Apply(Type)

    'rule' ApplyParamType(variadic(_)):



--------------------------------------------------------------------------------

'action' DeclareId(ID)

    'rule' DeclareId(Id):
        Id'Name -> QualifiedName
        GetUnqualifiedIdName(QualifiedName -> Name)
        HasLocalMeaning(Name -> definingid(DefiningId))
        Id'Position -> Position
        DefiningId'Position -> DefiningPosition
        Error_IdentifierPreviouslyDeclared(Position, Name, DefiningPosition)
        
    'rule' DeclareId(Id):
        Id'Name -> QualifiedName
        GetUnqualifiedIdName(QualifiedName -> Name)
        DefineUnqualifiedMeaning(Name, definingid(Id))

'action' ApplyId(ID)

    'rule' ApplyId(Id):
        Id'Name -> QualifiedName
        GetUnqualifiedIdName(QualifiedName -> Name)
        HasUnqualifiedMeaning(Name -> Meaning)
        Id'Meaning <- Meaning
        
    'rule' ApplyId(Id):
        Id'Name -> QualifiedName
        GetUnqualifiedIdName(QualifiedName -> Name)
        Id'Position -> Position
        Error_IdentifierNotDeclared(Position, Name)
        Id'Meaning <- error

'action' ApplyLocalId(ID)

    'rule' ApplyLocalId(Id):
        Id'Name -> QualifiedName
        GetUnqualifiedIdName(QualifiedName -> Name)
        HasLocalMeaning(Name -> Meaning)
        Id'Meaning <- Meaning

    'rule' ApplyLocalId(Id):
        Id'Name -> QualifiedName
        GetUnqualifiedIdName(QualifiedName -> Name)
        Id'Position -> Position
        Error_IdentifierNotDeclared(Position, Name)
        Id'Meaning <- error

--------------------------------------------------------------------------------

'action' DefinePackageId(ID)

    'rule' DefinePackageId(Id):
        Info::PACKAGEINFO
        Info'Index <- -1
        Info'Generator <- -1
        Id'Meaning <- package(Info)

'action' DefineSymbolId(ID, MODIFIER, ID, SYMBOLKIND, TYPE)

    'rule' DefineSymbolId(Id, Modifiers, ParentId, Kind, Type)
        Info::SYMBOLINFO
        Info'Index <- -1
        Info'Generator <- -1
        Info'Parent <- ParentId
        Info'Kind <- Kind
        Info'Type <- Type
        Info'Modifiers <- Modifiers
        Id'Meaning <- symbol(Info)

--------------------------------------------------------------------------------

'sweep' DumpBindings(ANY)

    'rule' DumpBindings(PACKAGE'package(_, Name, Definitions)):
        DumpId("package", Name)
        DumpBindings(Definitions)

    'rule' DumpBindings(DEFINITION'use(_, Name)):
        DumpId("use", Name)
        
    'rule' DumpBindings(DEFINITION'class(_, _, template(_, Name, Parameters), Definitions, Inherits, Implements)):
        DumpId("class", Name)
        DumpBindings(Parameters)
        DumpBindings(Definitions)
        DumpBindings(Inherits)
        DumpBindings(Implements)

    'rule' DumpBindings(DEFINITION'interface(_, template(_, Name, Parameters), Definitions, Inherits)):
        DumpId("interface", Name)
        DumpBindings(Parameters)
        DumpBindings(Definitions)
        DumpBindings(Inherits)

    'rule' DumpBindings(DEFINITION'constant(_, Name, Type, Value)):
        DumpId("constant", Name)
        DumpBindings(Type)
        DumpBindings(Value)
    'rule' DumpBindings(DEFINITION'variable(_, _, Name, Type)):
        DumpId("variable", Name)
        DumpBindings(Type)
    'rule' DumpBindings(DEFINITION'method(_, _, Name, signature(_, Type), _, _)):
        DumpId("method", Name)
        DumpBindings(Type)
    'rule' DumpBindings(DEFINITION'constructor(_, _, Name, _, _)):
        DumpId("constructor", Name)

    'rule' DumpBindings(TYPE'named(_, Name, Parameters)):
        DumpId("named type", Name)
        DumpBindings(Parameters)

    'rule' DumpBindings(TYPE'template(_, Name, Parameters)):
        DumpId("template type", Name)
        DumpBindings(Parameters)

    'rule' DumpBindings(TYPE'placeholder(_, Name)):
        DumpId("template placeholder", Name)
        
    'rule' DumpBindings(PARAMETER'parameter(_, Name, Type)):
        DumpId("parameter", Name)
        DumpBindings(Type)

'action' DumpId(STRING, ID)

    'rule' DumpId(Tag, Id):
        Id'Position -> Position
        Id'Name -> QualifiedName
        GetUnqualifiedIdName(QualifiedName -> Name)
        print(Tag)
        GetStringOfNameLiteral(Name -> NameString)
        print(NameString)
        Id'Meaning -> Meaning
        DumpMeaning(Meaning)

'action' DumpMeaning(MEANING)

    'rule' DumpMeaning(definingid(Id)):
        print("points to")
        QueryId(Id -> Info)
        print(Info)

    'rule' DumpMeaning(Meaning):
        print(Meaning)


--------------------------------------------------------------------------------
