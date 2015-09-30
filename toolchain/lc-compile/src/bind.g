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

'module' bind

'use'
    types
    support
    
'export'
    InitializeBind
    Bind

--------------------------------------------------------------------------------

-- The purpose of the 'Bind' phase is to ensure every Id is assigned either a
-- reference to the definingid() for its meaning, or the actual meaning if it is
-- the defining id.
'action' Bind(MODULE, MODULELIST)

    'rule' Bind(Module:module(Position, Kind, Name, Definitions), ImportedModules):
        DefineModuleId(Name)

        -- Make sure all the imported modules are bound
        BindImports(Definitions, ImportedModules)
        (|
            ErrorsDidOccur()
        ||
            -- Step 1: Ensure all id's referencing definitions point to the definition.
            --         and no duplicate definitions have been attempted.
            EnterScope

            -- Import all the used modules
            DeclareImports(Definitions, ImportedModules)
            
            EnterScope

            -- Declare the predefined ids
            DeclarePredefinedIds
            -- Assign the defining id to all top-level names.
            Declare(Definitions)
            -- Resolve all references to id's.
            Apply(Definitions)
            
            LeaveScope

            LeaveScope
            
            -- Step 2: Ensure all definitions have their appropriate meaning
            Define(Name, Definitions)
            
            --DumpBindings(Module)
        |)

'action' BindImports(DEFINITION, MODULELIST)

    'rule' BindImports(sequence(Left, Right), Imports):
        BindImports(Left, Imports)
        BindImports(Right, Imports)
        
    'rule' BindImports(import(Position, Id), Imports):
        Id'Name -> Name
        (|
            FindModuleInList(Name, Imports -> Module)
            Module'Name -> ModuleId
            (|
                QueryId(ModuleId -> module(Info))
            ||
                DefineModuleId(ModuleId)
                Bind(Module, Imports)
            |)
        ||
            (|
                IsBootstrapCompile()
                Error_DependentModuleNotIncludedWithInputs(Position, Name)
            ||
                Error_InterfaceFileNameMismatch(Position, Name)
            |)
        |)
        
    'rule' BindImports(_, _):
        -- do nothing
--

'action' DeclareImports(DEFINITION, MODULELIST)

    'rule' DeclareImports(sequence(Left, Right), Imports):
        DeclareImports(Left, Imports)
        DeclareImports(Right, Imports)
        
    'rule' DeclareImports(import(Position, Id), Imports):
        Id'Name -> Name
        (|
            FindModuleInList(Name, Imports -> Module)
            Module'Definitions -> Definitions
            DeclareImportedDefinitions(Definitions)
        ||
            (|
                IsBootstrapCompile()
                Error_DependentModuleNotIncludedWithInputs(Position, Name)
            ||
                Error_InterfaceFileNameMismatch(Position, Name)
            |)
        |)
        
    'rule' DeclareImports(_, _):
        -- do nothing
        
'action' DeclareImportedDefinitions(DEFINITION)

    'rule' DeclareImportedDefinitions(sequence(Left, Right)):
        DeclareImportedDefinitions(Left)
        DeclareImportedDefinitions(Right)
        
    'rule' DeclareImportedDefinitions(type(Position, _, Name, _)):
        DeclareId(Name)

    'rule' DeclareImportedDefinitions(constant(Position, _, Name, _)):
        DeclareId(Name)
    
    'rule' DeclareImportedDefinitions(variable(Position, _, Name, _)):
        DeclareId(Name)
    
    'rule' DeclareImportedDefinitions(contextvariable(Position, _, Name, _, _)):
        DeclareId(Name)

    'rule' DeclareImportedDefinitions(handler(Position, _, Name, _, _, _, _)):
        DeclareId(Name)
    
    'rule' DeclareImportedDefinitions(foreignhandler(Position, _, Name, _, _)):
        DeclareId(Name)
    
    'rule' DeclareImportedDefinitions(property(Position, _, Name, _, _)):
        DeclareId(Name)
    
    'rule' DeclareImportedDefinitions(event(Position, _, Name, _)):
        DeclareId(Name)

    'rule' DeclareImportedDefinitions(syntax(Position, _, Name, _, _, _, _)):
        DeclareId(Name)

    'rule' DeclareImportedDefinitions(metadata(_, _, _)):
        -- do nothing

    'rule' DeclareImportedDefinitions(import(_, _)):
        -- do nothing

    'rule' DeclareImportedDefinitions(nil):
        -- do nothing

--

'condition' FindModuleInList(NAME, MODULELIST -> MODULE)

    'rule' FindModuleInList(Name, modulelist(Head, Rest) -> Head):
        Head'Name -> Id
        Id'Name -> ModName
        IsNameEqualToName(Name, ModName)
        
    'rule' FindModuleInList(Name, modulelist(_, Rest) -> Found):
        FindModuleInList(Name, Rest -> Found)

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
        
    'rule' Declare(type(Position, _, Name, _)):
        DeclareId(Name)

    'rule' Declare(constant(Position, _, Name, _)):
        DeclareId(Name)
    
    'rule' Declare(variable(Position, _, Name, _)):
        DeclareId(Name)
    
    'rule' Declare(contextvariable(Position, _, Name, _, _)):
        DeclareId(Name)
    
    'rule' Declare(handler(Position, _, Name, _, _, _, _)):
        DeclareId(Name)
    
    'rule' Declare(foreignhandler(Position, _, Name, _, _)):
        DeclareId(Name)
    
    'rule' Declare(property(Position, _, Name, _, _)):
        DeclareId(Name)
    
    'rule' Declare(event(Position, _, Name, _)):
        DeclareId(Name)

    'rule' Declare(syntax(Position, _, Name, _, _, _, _)):
        DeclareId(Name)
    
    'rule' Declare(metadata(_, _, _)):
        -- do nothing

    'rule' Declare(import(_, _)):
        -- do nothing
        
    'rule' Declare(nil):
        -- do nothing

'action' DeclareParameters(PARAMETERLIST)

    'rule' DeclareParameters(parameterlist(parameter(_, _, Name, _), Tail)):
        DeclareId(Name)
        DeclareParameters(Tail)
        
    'rule' DeclareParameters(nil):
        -- do nothing

'action' DeclareFields(FIELDLIST)

    'rule' DeclareFields(fieldlist(Field, Tail)):
        (|
            where(Field -> action(_, Name, _))
        ||
            where(Field -> slot(_, Name, _))
        ||
            where(Field -> element(_, Name))
        |)
        DeclareId(Name)
        DeclareFields(Tail)
        
    'rule' DeclareFields(nil):
        -- do nothing

--------------------------------------------------------------------------------

-- The 'Define' phase associates meanings with the definining ids.
--
'action' Define(ID, DEFINITION)

    'rule' Define(ModuleId, sequence(Left, Right)):
        Define(ModuleId, Left)
        Define(ModuleId, Right)
        
    'rule' Define(ModuleId, type(Position, Access, Name, Type)):
        DefineSymbolId(Name, ModuleId, Access, type, Type)
        [|
            where(Type -> handler(_, _, signature(Parameters, _)))
            DefineParameters(Name, Parameters)
        |]
    
    'rule' Define(ModuleId, constant(Position, Access, Name, Value)):
        --ComputeTypeOfConstantTermExpression(Value -> Type)
        DefineSymbolId(Name, ModuleId, Access, constant, undefined(Position))
    
    'rule' Define(ModuleId, variable(Position, Access, Name, Type)):
        DefineSymbolId(Name, ModuleId, Access, variable, Type)

    'rule' Define(ModuleId, contextvariable(Position, Access, Name, Type, _)):
        DefineSymbolId(Name, ModuleId, Access, context, Type)
    
    'rule' Define(ModuleId, handler(Position, Access, Name, _, Signature:signature(Parameters, _), _, _)):
        DefineSymbolId(Name, ModuleId, Access, handler, handler(Position, normal, Signature))
        DefineParameters(Name, Parameters)
    
    'rule' Define(ModuleId, foreignhandler(Position, Access, Name, Signature:signature(Parameters, _), _)):
        DefineSymbolId(Name, ModuleId, Access, handler, handler(Position, foreign, Signature))
        DefineParameters(Name, Parameters)

    'rule' Define(ModuleId, property(Position, Access, Name, Getter, Setter)):
        DefineSymbolId(Name, ModuleId, Access, property, nil)

    'rule' Define(ModuleId, event(Position, Access, Name, Signature:signature(Parameters, _))):
        DefineSymbolId(Name, ModuleId, Access, event, nil)
        DefineParameters(Name, Parameters)
    
    'rule' Define(ModuleId, syntax(Position, Access, Name, Class, Warnings, Syntax, Methods)):
        DefineSyntaxId(Name, ModuleId, Class, Syntax, Methods)
    
    'rule' Define(_, metadata(_, _, _)):
        -- do nothing

    'rule' Define(_, import(_, _)):
        -- do nothing
        
    'rule' Define(_, nil):
        -- do nothing

'action' DefineParameters(ID, PARAMETERLIST)

    'rule' DefineParameters(ModuleId, parameterlist(parameter(_, _, Name, Type), Tail)):
        DefineSymbolId(Name, ModuleId, inferred, parameter, Type)
        DefineParameters(ModuleId, Tail)
        
    'rule' DefineParameters(ModuleId, nil):
        -- do nothing

'action' ComputeTypeOfConstantTermExpression(EXPRESSION -> TYPE)
    'rule' ComputeTypeOfConstantTermExpression(undefined(Position) -> undefined(Position)):
    'rule' ComputeTypeOfConstantTermExpression(true(Position) -> boolean(Position)):
    'rule' ComputeTypeOfConstantTermExpression(false(Position) -> boolean(Position)):
    'rule' ComputeTypeOfConstantTermExpression(unsignedinteger(Position, _) -> integer(Position)):
    'rule' ComputeTypeOfConstantTermExpression(integer(Position, _) -> integer(Position)):
    'rule' ComputeTypeOfConstantTermExpression(real(Position, _) -> real(Position)):
    'rule' ComputeTypeOfConstantTermExpression(string(Position, _) -> string(Position)):

/*'action' DefineFields(FIELDLIST)

    'rule' DefineFields(fieldlist(Field, Tail)):
        (|
            where(Field -> action(_, Name, _))
            --DefineId(Name, fieldaction)
        ||
            where(Field -> slot(_, Name, _))
            --DefineId(Name, fieldslot)
        ||
            where(Field -> element(_, Name))
            --DefineId(Name, fieldelement)
        |)
        
    'rule' DefineFields(nil):
        -- do nothing*/

--------------------------------------------------------------------------------

'var' LastSyntaxMarkIndexVar : INT

-- We need to define variables at the point they are present, rather than at the
-- start. So we store the handler id for the parent link in the info rather than
-- thread the parent through.
'var' CurrentHandlerId : ID

'sweep' Apply(ANY)

    ----------

    'rule' Apply(DEFINITION'handler(_, _, Id, _, signature(Parameters, Type), _, Body)):
        -- The type of the handler is resolved in the current scope
        Apply(Type)
        
        -- Now enter a new scope for parameters and local variables.
        EnterScope
        
        -- Declare the parameters first
        DeclareParameters(Parameters)
        
        -- Apply all ids in the parameters (covers types)
        Apply(Parameters)

        -- Now apply all ids in the body. This will also declare local variables as
        -- it goes along
        CurrentHandlerId <- Id
        Apply(Body)

        LeaveScope
        
    'rule' Apply(DEFINITION'foreignhandler(_, _, _, signature(Parameters, Type), _)):
        -- The type of the foreign handler is resolved in the current scope.
        Apply(Type)
        
        -- Enter a new scope to check parameters.
        EnterScope
        DeclareParameters(Parameters)
        Apply(Parameters)
        LeaveScope

    'rule' Apply(DEFINITION'property(_, _, _, Getter, OptSetter)):
        -- Resolve the getter and setter ids
        ApplyId(Getter)
        [|
            where(OptSetter -> id(Setter))
            ApplyId(Setter)
        |]
        
    'rule' Apply(DEFINITION'event(_, _, _, signature(Parameters, Type))):
        Apply(Type)
        
        -- Enter a new scope to check parameters.
        EnterScope
        DeclareParameters(Parameters)
        Apply(Parameters)
        LeaveScope


    ----------

    'rule' Apply(TYPE'named(_, Name)):
        ApplyId(Name)
    
    'rule' Apply(TYPE'record(_, BaseType, Fields)):
        -- Apply the base type
        Apply(BaseType)
        
        -- Enter a new scope for fields
        EnterScope
        
        -- Declare the fields first
        DeclareFields(Fields)
        
        -- Now apply all id's in the fields
        Apply(Fields)
        
        -- Leave the fields scope
        LeaveScope
        
    'rule' Apply(TYPE'enum(_, BaseType, Fields)):
        -- Apply the base type
        Apply(BaseType)
        
        -- Enter a new scope for fields
        EnterScope
        
        -- Declare the fields first
        DeclareFields(Fields)
        
        -- Now apply all id's in the fields
        Apply(Fields)
        
        -- Leave the fields scope
        LeaveScope
        
    'rule' Apply(TYPE'handler(_, _, signature(Parameters, Type))):
        -- The return type of the handler is resolved in the current scope.
        Apply(Type)
        
        -- Enter a new scope to check parameters.
        EnterScope
        DeclareParameters(Parameters)
        Apply(Parameters)
        LeaveScope
        
    ----------

    'rule' Apply(FIELD'action(_, Id, Handler)):
        ApplyId(Id)
        ApplyId(Handler)

    'rule' Apply(FIELD'slot(_, Id, Type)):
        ApplyId(Id)
        Apply(Type)

    'rule' Apply(FIELD'element(_, Id)):
        ApplyId(Id)

    ----------

    'rule' Apply(STATEMENT'variable(_, Name, Type)):
        DeclareId(Name)
        CurrentHandlerId -> ParentId
        DefineSymbolId(Name, ParentId, inferred, local, Type)
        Apply(Type)
        
    'rule' Apply(STATEMENT'repeatupto(_, Slot, Start, Finish, Step, Body)):
        ApplyId(Slot)
        Apply(Start)
        Apply(Finish)
        Apply(Step)
        Apply(Body)

    'rule' Apply(STATEMENT'repeatdownto(_, Slot, Start, Finish, Step, Body)):
        ApplyId(Slot)
        Apply(Start)
        Apply(Finish)
        Apply(Step)
        Apply(Body)

    'rule' Apply(STATEMENT'repeatforeach(_, Iterator, Container, Body)):
        Apply(Iterator)
        Apply(Container)
        Apply(Body)

    'rule' Apply(STATEMENT'call(_, Handler, Arguments)):
        ApplyId(Handler)
        Apply(Arguments)

    ---------
    
    'rule' Apply(EXPRESSION'slot(_, Name)):
        ApplyId(Name)

    'rule' Apply(EXPRESSION'call(_, Handler, Arguments)):
        ApplyId(Handler)
        Apply(Arguments)

    ---------

    'rule' Apply(DEFINITION'syntax(_, _, _, _, _, Syntax, Methods)):
        -- We index all mark variabless starting at 0.
        LastSyntaxMarkIndexVar <- 0

        -- Mark variables have their own local scope
        EnterScope
        
        -- Define appropriate meaning for 'input' and 'output' before we
        -- bind the use of the ids in the methods.
        DeclarePredefinedMarkVariables

        -- Process the mark variables in the syntax (definiton side).
        -- all variables with the same name get the same index.
        Apply(Syntax)

        -- Now bind the mark variables to the arguments in the methods.
        Apply(Methods)

        LeaveScope

    'rule' Apply(SYNTAX'markedrule(_, Variable, Rule)):
        ApplySyntaxMarkId(Variable)
        ApplyId(Rule)

    'rule' Apply(SYNTAX'mark(_, Variable, _)):
        ApplySyntaxMarkId(Variable)
        
    'rule' Apply(SYNTAX'rule(_, Rule)):
        ApplyId(Rule)

    'rule' Apply(SYNTAXMETHOD'method(_, Name, Arguments)):
        ApplyId(Name)
        Apply(Arguments)
        
    'rule' Apply(SYNTAXCONSTANT'variable(_, Value)):
        ApplyId(Value)

    'rule' Apply(SYNTAXCONSTANT'indexedvariable(_, Value, _)):
        ApplyId(Value)

--------------------------------------------------------------------------------

'action' DeclareId(ID)

    'rule' DeclareId(Id):
        Id'Name -> Name
        HasLocalMeaning(Name -> definingid(DefiningId))
        Id'Position -> Position
        DefiningId'Position -> DefiningPosition
        Error_IdentifierPreviouslyDeclared(Position, Name, DefiningPosition)
        
    'rule' DeclareId(Id):
        Id'Name -> Name
        DefineMeaning(Name, definingid(Id))

'action' ApplyId(ID)

    'rule' ApplyId(Id):
        Id'Name -> Name
        HasMeaning(Name -> Meaning)
        Id'Meaning <- Meaning
        
    'rule' ApplyId(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_IdentifierNotDeclared(Position, Name)
        Id'Meaning <- error

'action' ApplyLocalId(ID)

    'rule' ApplyLocalId(Id):
        Id'Name -> Name
        HasLocalMeaning(Name -> Meaning)
        Id'Meaning <- Meaning

    'rule' ApplyLocalId(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_IdentifierNotDeclared(Position, Name)
        Id'Meaning <- error

'action' ApplySyntaxMarkId(ID)

    'rule' ApplySyntaxMarkId(Id):
        Id'Name -> Name
        (|
            HasLocalMeaning(Name -> Meaning)
            where(Meaning -> syntaxmark(_))
        ||
            LastSyntaxMarkIndexVar -> Index
            MarkInfo::SYNTAXMARKINFO
            MarkInfo'Index <- Index
            MarkInfo'Type <- uncomputed
            MarkInfo'LMode <- uncomputed
            MarkInfo'RMode <- uncomputed
            where(syntaxmark(MarkInfo) -> Meaning)
            DefineMeaning(Name, Meaning)
            LastSyntaxMarkIndexVar <- Index + 1
        |)
        Id'Meaning <- Meaning
        
    --'rule' ApplySyntaxMarkId(Id):
    --    Id'Position -> Position
    --    Id'Name -> Name
    --    Error_InvalidNameForSyntaxMarkVariable(Position, Name)

--------------------------------------------------------------------------------

'action' DefineModuleId(ID)

    'rule' DefineModuleId(Id):
        Info::MODULEINFO
        Info'Index <- -1
        Info'Generator <- -1
        Id'Meaning <- module(Info)

'action' DefineSymbolId(ID, ID, ACCESS, SYMBOLKIND, TYPE)

    'rule' DefineSymbolId(Id, ParentId, Access, Kind, Type)
        Info::SYMBOLINFO
        Info'Index <- -1
        Info'Generator <- -1
        Info'Parent <- ParentId
        Info'Kind <- Kind
        Info'Type <- Type
        Info'Access <- Access
        Id'Meaning <- symbol(Info)

'action' DefineSyntaxId(ID, ID, SYNTAXCLASS, SYNTAX, SYNTAXMETHODLIST)

    'rule' DefineSyntaxId(Id, ModuleId, Class, Syntax, Methods):
        Info::SYNTAXINFO
        Info'Class <- Class
        Info'Parent <- ModuleId
        Info'Syntax <- Syntax
        Info'Methods <- Methods
        Info'Prefix <- undefined
        Info'Suffix <- undefined
        Id'Meaning <- syntax(Info)

--------------------------------------------------------------------------------

'var' OutputSyntaxMarkIdVar : ID
'var' InputSyntaxMarkIdVar : ID
'var' ContextSyntaxMarkIdVar : ID
'var' IteratorSyntaxMarkIdVar : ID
'var' ContainerSyntaxMarkIdVar : ID

'var' ExpressionSyntaxRuleIdVar : ID
'var' ExpressionListSyntaxRuleIdVar : ID

'action' InitializeBind

    'rule' InitializeBind:
        MakePredefinedSyntaxMarkId("output", output -> Id1)
        OutputSyntaxMarkIdVar <- Id1
        MakePredefinedSyntaxMarkId("input", input -> Id2)
        InputSyntaxMarkIdVar <- Id2
        MakePredefinedSyntaxMarkId("context", context -> Id3)
        ContextSyntaxMarkIdVar <- Id3
        MakePredefinedSyntaxMarkId("iterator", iterator -> Id4)
        IteratorSyntaxMarkIdVar <- Id4
        MakePredefinedSyntaxMarkId("container", container -> Id5)
        ContainerSyntaxMarkIdVar <- Id5
        MakePredefinedSyntaxId("Expression", expressionphrase, expression, expression -> Id6)
        ExpressionSyntaxRuleIdVar <- Id6
        MakePredefinedSyntaxId("ExpressionList", expressionlistphrase, expression, expression -> Id7)
        ExpressionListSyntaxRuleIdVar <- Id7

'action' DeclarePredefinedIds

    'rule' DeclarePredefinedIds:
        ExpressionSyntaxRuleIdVar -> Id1
        DeclareId(Id1)
        ExpressionListSyntaxRuleIdVar -> Id2
        DeclareId(Id2)

'action' DeclarePredefinedMarkVariables

    'rule' DeclarePredefinedMarkVariables:
        OutputSyntaxMarkIdVar -> Id1
        DeclareId(Id1)
        InputSyntaxMarkIdVar -> Id2
        DeclareId(Id2)
        ContextSyntaxMarkIdVar -> Id3
        DeclareId(Id3)
        IteratorSyntaxMarkIdVar -> Id4
        DeclareId(Id4)
        ContainerSyntaxMarkIdVar -> Id5
        DeclareId(Id5)

'action' MakePredefinedSyntaxId(STRING, SYNTAXCLASS, SYNTAXTERM, SYNTAXTERM -> ID)

    'rule' MakePredefinedSyntaxId(String, Class, Prefix, Suffix -> Id):
        Id::ID
        MakeNameLiteral(String -> Name)
        Id'Name <- Name
        Info::SYNTAXINFO
        Info'Index <- -1
        Info'Class <- Class
        Info'Prefix <- Prefix
        Info'Suffix <- Suffix
        Id'Meaning <- syntax(Info)

'action' MakePredefinedSyntaxMarkId(STRING, SYNTAXMARKTYPE -> ID)

    'rule' MakePredefinedSyntaxMarkId(String, Type -> Id)
        Id::ID
        MakeNameLiteral(String -> Name)
        Id'Name <- Name
        Info::SYNTAXMARKINFO
        Info'Index <- -1
        Info'Type <- Type
        Id'Meaning <- syntaxmark(Info)

--------------------------------------------------------------------------------

'sweep' DumpBindings(ANY)

    'rule' DumpBindings(MODULE'module(_, Kind, Name, Definitions)):
        DumpId("module", Name)
        DumpBindings(Definitions)
        
    'rule' DumpBindings(DEFINITION'import(_, Name)):
        DumpId("import", Name)
        
    'rule' DumpBindings(DEFINITION'type(_, _, Name, Type)):
        DumpId("type", Name)
        DumpBindings(Type)
    'rule' DumpBindings(DEFINITION'constant(_, _, Name, Value)):
        DumpId("constant", Name)
        DumpBindings(Value)
    'rule' DumpBindings(DEFINITION'variable(_, _, Name, Type)):
        DumpId("variable", Name)
        DumpBindings(Type)
    'rule' DumpBindings(DEFINITION'contextvariable(_, _, Name, Type, _)):
        DumpId("variable", Name)
        DumpBindings(Type)
    'rule' DumpBindings(DEFINITION'handler(_, _, Name, _, Signature, Definitions, Body)):
        DumpId("handler", Name)
        DumpBindings(Signature)
        DumpBindings(Definitions)
        DumpBindings(Body)
    'rule' DumpBindings(DEFINITION'foreignhandler(_, _, Name, Signature, _)):
        DumpId("foreign handler", Name)
        DumpBindings(Signature)
    'rule' DumpBindings(DEFINITION'property(_, _, Name, Getter, OptionalSetter)):
        DumpId("property", Name)
        DumpId("property getter", Getter)
        [|
            where(OptionalSetter -> id(Setter))
            DumpId("property setter", Setter)
        |]
    'rule' DumpBindings(DEFINITION'event(_, _, Name, Signature)):
        DumpId("event", Name)
        DumpBindings(Signature)
    'rule' DumpBindings(DEFINITION'syntax(_, _, Name, _, _, Syntax, Methods)):
        DumpId("syntax", Name)
        DumpBindings(Syntax)
        DumpBindings(Methods)
    
    'rule' DumpBindings(TYPE'named(_, Name)):
        DumpId("named type", Name)
        
    'rule' DumpBindings(FIELD'action(_, Name, Handler)):
        DumpId("action field", Name)
        DumpId("action field handler", Handler)
    'rule' DumpBindings(FIELD'slot(_, Name, Type)):
        DumpId("slot field", Name)
        DumpBindings(Type)
    'rule' DumpBindings(FIELD'element(_, Name)):
        DumpId("element field", Name)
        
    'rule' DumpBindings(PARAMETER'parameter(_, _, Name, Type)):
        DumpId("parameter", Name)
        DumpBindings(Type)
        
    'rule' DumpBindings(STATEMENT'variable(_, Name, Type)):
        DumpId("local variable", Name)
        DumpBindings(Type)
    'rule' DumpBindings(STATEMENT'repeatupto(_, Slot, Start, Finish, Step, Body)):
        DumpId("repeat upto slot", Slot)
        DumpBindings(Start)
        DumpBindings(Finish)
        DumpBindings(Step)
        DumpBindings(Body)
    'rule' DumpBindings(STATEMENT'repeatdownto(_, Slot, Start, Finish, Step, Body)):
        DumpId("repeat downto slot", Slot)
        DumpBindings(Start)
        DumpBindings(Finish)
        DumpBindings(Step)
        DumpBindings(Body)
    'rule' DumpBindings(STATEMENT'repeatforeach(_, Iterator, Container, Body)):
        --DumpId("repeat foreach")
        DumpBindings(Iterator)
        DumpBindings(Container)
        DumpBindings(Body)
    'rule' DumpBindings(STATEMENT'call(_, Handler, Arguments)):
        DumpId("statement call handler", Handler)
        DumpBindings(Arguments)

    'rule' DumpBindings(EXPRESSION'slot(_, Name)):
        DumpId("slot", Name)
    'rule' DumpBindings(EXPRESSION'call(_, Handler, Arguments)):
        DumpId("expression call handler", Handler)
        DumpBindings(Arguments)

    'rule' DumpBindings(SYNTAXMETHOD'method(_, Name, Arguments)):
        DumpId("syntax method", Name)
        DumpBindings(Arguments)
        
    'rule' DumpBindings(SYNTAX'markedrule(_, Id, Name)):
        DumpId("syntax mark", Id)
        DumpId("syntax rule", Name)
    'rule' DumpBindings(SYNTAX'mark(_, Id, Value)):
        DumpId("syntax mark", Id)
        DumpBindings(Value)
        
    'rule' DumpBindings(SYNTAXCONSTANT'variable(_, Name)):
        DumpId("syntax slot", Name)
    'rule' DumpBindings(SYNTAXCONSTANT'indexedvariable(_, Name, _)):
        DumpId("indexed syntax slot", Name)


'action' DumpId(STRING, ID)

    'rule' DumpId(Tag, Id):
        Id'Position -> Position
        Id'Name -> Name
        print(Tag)
        GetStringOfNameLiteral(Name -> NameString)
        print(NameString)
        Id'Meaning -> Meaning
        print(Meaning)

--------------------------------------------------------------------------------
