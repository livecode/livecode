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

'module' generate

'use'
    types
    support

'export'
    Generate

--------------------------------------------------------------------------------

'var' ModuleDependencyList : NAMELIST

'var' IgnoredModuleList : NAMELIST

'action' Generate(MODULE)

    'rule' Generate(Module:module(_, Kind, Id, Imports, Definitions)):
        ModuleDependencyList <- nil
        
        QueryModuleId(Id -> Info)
        Id'Name -> ModuleName
        (|
            where(Kind -> module)
            EmitBeginModule(ModuleName -> ModuleIndex)
        ||
            where(Kind -> widget)
            EmitBeginWidgetModule(ModuleName -> ModuleIndex)
            IgnoredModuleList <- nil
        ||
            where(Kind -> library)
            EmitBeginLibraryModule(ModuleName -> ModuleIndex)
        |)
        Info'Index <- ModuleIndex
        
        (|
            ne(Kind, widget)
            (|
                ImportContainsCanvas(Imports)
                MakeNameLiteral("com.livecode.widget" -> WidgetModuleName)
                IgnoredModuleList <- namelist(WidgetModuleName, nil)
            ||
                MakeNameLiteral("com.livecode.widget" -> WidgetModuleName)
                MakeNameLiteral("com.livecode.canvas" -> CanvasModuleName)
                IgnoredModuleList <- namelist(CanvasModuleName, namelist(WidgetModuleName, nil))
            |)
        ||
            IgnoredModuleList <- nil
        |)

        -- Emit all imported declarations and dependent modules.
        GenerateImportedDefinitions(Definitions)

        -- Declare indices for all definitions in this module (so mutally referential
        -- definitions work).
        GenerateDefinitionIndexes(Definitions)
        
        -- Generate all definitions from this module.
        GenerateDefinitions(Definitions)
        
        -- Generate all the exported definitions
        GenerateExportedDefinitions(Definitions)
        
        EmitEndModule()
        
        GenerateManifest(Module)

'condition' ImportContainsCanvas(IMPORT)

    'rule' ImportContainsCanvas(sequence(Left, _)):
        ImportContainsCanvas(Left)
        
    'rule' ImportContainsCanvas(sequence(_, Right)):
        ImportContainsCanvas(Right)
        
    'rule' ImportContainsCanvas(import(_, Id)):
        Id'Name -> Name
        MakeNameLiteral("com.livecode.canvas" -> CanvasModuleName)
        eq(Name, CanvasModuleName)

----------

'action' GenerateManifest(MODULE)

    'rule' GenerateManifest(module(_, Kind, Id, Imports, Definitions)):
        OutputBeginManifest()
        Id'Name -> Name
        OutputWrite("<package version=\"0.0\">\n")
        OutputWriteI("  <name>", Name, "</name>\n")
        [|
            QueryMetadata(Definitions, "title" -> TitleString)
            OutputWriteS("  <title>", TitleString, "</title>\n")
        |]
        [|
            QueryMetadata(Definitions, "author" -> AuthorString)
            OutputWriteS("  <author>", AuthorString, "</author>\n")
        |]
        [|
            QueryMetadata(Definitions, "description" -> DescriptionString)
            OutputWriteS("  <description>", DescriptionString, "</description>\n")
        |]
        [|
            QueryMetadata(Definitions, "version" -> VersionString)
            OutputWriteS("  <version>", VersionString, "</version>\n")
        |]
        OutputWrite("  <license>community</license>\n")
        (|
            where(Kind -> widget)
            OutputWrite("  <type>widget</type>\n")
        ||
            where(Kind -> library)
            OutputWrite("  <type>library</type>\n")
        ||
        |)
        ModuleDependencyList -> Requirements
        GenerateManifestRequires(Requirements)
        GenerateManifestDefinitions(Definitions)
        OutputWrite("</package>")
        OutputEnd()

'condition' QueryMetadata(DEFINITION, STRING -> STRING)

    'rule' QueryMetadata(sequence(Left, _), Key -> Value):
        QueryMetadata(Left, Key -> Value)

    'rule' QueryMetadata(sequence(_, Right), Key -> Value):
        QueryMetadata(Right, Key -> Value)
        
    'rule' QueryMetadata(metadata(_, Key, Value), WantedKey -> Value):
        IsStringEqualToString(Key, WantedKey)

'action' AddModuleToDependencyList(NAME)

    'rule' AddModuleToDependencyList(Name):
        ModuleDependencyList -> List
        IsNameInList(Name, List)
        
    'rule' AddModuleToDependencyList(Name):
        ModuleDependencyList -> List
        ModuleDependencyList <- namelist(Name, List)
        
'action' GenerateManifestRequires(NAMELIST)

    'rule' GenerateManifestRequires(namelist(Head, Tail)):
        GenerateManifestRequires(Tail)
        OutputWriteI("  <requires name=\"", Head, "\"/>\n")
    
    'rule' GenerateManifestRequires(nil):
        -- do nothing

'action' GenerateManifestDefinitions(DEFINITION)

    'rule' GenerateManifestDefinitions(sequence(Left, Right)):
        GenerateManifestDefinitions(Left)
        GenerateManifestDefinitions(Right)
        
    'rule' GenerateManifestDefinitions(metadata(_, Key, Value)):
        (|
            IsStringEqualToString(Key, "title")
        ||
            IsStringEqualToString(Key, "author")
        ||
            IsStringEqualToString(Key, "description")
        ||
            IsStringEqualToString(Key, "version")
        ||
            OutputWriteS("  <metadata key=\"", Key, "\">")
            OutputWriteS("", Value, "</metadata>\n")
        |)

    'rule' GenerateManifestDefinitions(type(_, public, Name, _)):
    
    'rule' GenerateManifestDefinitions(constant(_, public, Name, _)):
    
    'rule' GenerateManifestDefinitions(variable(_, public, Name, _)):

    'rule' GenerateManifestDefinitions(contextvariable(_, public, Name, _, _)):

    'rule' GenerateManifestDefinitions(handler(_, public, Name, _, Signature, _, _)):
        GenerateManifestHandlerDefinition(Name, Signature)

    'rule' GenerateManifestDefinitions(foreignhandler(_, public, Name, Signature, _)):
        GenerateManifestHandlerDefinition(Name, Signature)

    'rule' GenerateManifestDefinitions(property(_, public, Name, Getter, OptionalSetter)):
        QuerySymbolId(Getter -> GetInfo)
        GetInfo'Type -> GetDefType
        (|
            where(GetDefType -> handler(_, signature(_, GetType)))
        ||
            where(GetDefType -> GetType)
        |)
        (|
            where(OptionalSetter -> id(Setter))
            QuerySymbolId(Setter -> SetInfo)
            SetInfo'Type -> SetDefType
            (|
                where(SetDefType -> handler(_, signature(parameterlist(parameter(_, _, _, SetType), _), _)))
            ||
                where(SetDefType -> SetType)
            |)
        ||
            where(TYPE'nil -> SetType)
        |)
        GenerateManifestPropertyDefinition(Name, GetType, SetType)

    'rule' GenerateManifestDefinitions(event(_, public, Name, Signature)):
        GenerateManifestEventDefinition(Name, Signature)

    'rule' GenerateManifestDefinitions(_):
        -- nothing

'action' GenerateManifestType(TYPE)

    'rule' GenerateManifestType(Type):
        [|
            IsTypeOptional(Type)
            OutputWrite("optional ")
        |]
        GenerateManifestTypeBody(Type)

'condition' IsTypeOptional(TYPE)

    'rule' IsTypeOptional(optional(_, Base)):
        -- do nothing
        
    'rule' IsTypeOptional(named(_, Id))
        QuerySymbolId(Id -> Info)
        Info'Type -> Type
        IsTypeOptional(Type)

'action' GenerateManifestTypeBody(TYPE)

    'rule' GenerateManifestTypeBody(optional(_, Base)):
        GenerateManifestTypeBody(Base)

    'rule' GenerateManifestTypeBody(named(_, Id)):
        QuerySymbolId(Id -> Info)
        Info'Type -> Type
        (|
            where(Type -> optional(_, _))
            GenerateManifestTypeBody(Type)
        ||
            where(Type -> named(_, _))
            GenerateManifestTypeBody(Type)
        ||
            Id'Name -> Name
            OutputWriteI("", Name, "")
        |)

    'rule' GenerateManifestTypeBody(any(_)):
        OutputWrite("any")

    'rule' GenerateManifestTypeBody(undefined(_)):
        OutputWrite("undefined")

    'rule' GenerateManifestTypeBody(boolean(_)):
        OutputWrite("boolean")
    'rule' GenerateManifestTypeBody(integer(_)):
        OutputWrite("integer")
    'rule' GenerateManifestTypeBody(real(_)):
        OutputWrite("real")
    'rule' GenerateManifestTypeBody(number(_)):
        OutputWrite("number")
    'rule' GenerateManifestTypeBody(string(_)):
        OutputWrite("string")
    'rule' GenerateManifestTypeBody(data(_)):
        OutputWrite("data")
    'rule' GenerateManifestTypeBody(array(_)):
        OutputWrite("array")
    'rule' GenerateManifestTypeBody(list(_, _)):
        OutputWrite("list")

    'rule' GenerateManifestTypeBody(Type):
        print(Type)
        Fatal_InternalInconsistency("attempt to generate uncoded type for manifest")

'action' GenerateManifestSignatureParameters(PARAMETERLIST)

    'rule' GenerateManifestSignatureParameters(parameterlist(parameter(_, Mode, _, Type), Tail)):
        (|
            where(Mode -> in)
            OutputWrite("in ")
        ||
            where(Mode -> out)
            OutputWrite("out ")
        ||
            where(Mode -> inout)
            OutputWrite("inout ")
        |)
        GenerateManifestType(Type)
        [|
            ne(Tail, nil)
            OutputWrite(",")
        |]
        GenerateManifestSignatureParameters(Tail)

    'rule' GenerateManifestSignatureParameters(nil):
        -- do nothing

'action' GenerateManifestSignature(SIGNATURE)

    'rule' GenerateManifestSignature(signature(Parameters, ReturnType)):
        OutputWrite("parameters=\"")
        GenerateManifestSignatureParameters(Parameters)
        OutputWrite("\" return=\"")
        GenerateManifestType(ReturnType)
        OutputWrite("\"")

'action' GenerateManifestHandlerDefinition(ID, SIGNATURE)

    'rule' GenerateManifestHandlerDefinition(Id, Signature):
        Id'Name -> Name
        OutputWriteI("  <handler name=\"", Name, "\" ")
        GenerateManifestSignature(Signature)
        OutputWrite("/>\n")

'action' GenerateManifestEventDefinition(ID, SIGNATURE)

    'rule' GenerateManifestEventDefinition(Id, Signature):
        Id'Name -> Name
        OutputWriteI("  <event name=\"", Name, "\" ")
        GenerateManifestSignature(Signature)
        OutputWrite("/>\n")

'action' GenerateManifestPropertyDefinition(ID, TYPE, TYPE)

    'rule' GenerateManifestPropertyDefinition(Id, GetType, SetType):
        Id'Name -> Name
        OutputWriteI("  <property name=\"", Name, "\" get=\"")
        GenerateManifestType(GetType)
        [|
            ne(SetType, nil)
            OutputWrite("\" set=\"")
            GenerateManifestType(SetType)
        |]
        OutputWrite("\"/>\n")

'condition' IsNameInList(NAME, NAMELIST)
    'rule' IsNameInList(Id, namelist(Head, Tail)):
        eq(Id, Head)
    'rule' IsNameInList(Id, namelist(Head, Tail)):
        IsNameInList(Id, Tail)

'condition' IsNameNotInList(NAME, NAMELIST)
    'rule' IsNameNotInList(Id, namelist(Head, Tail)):
        ne(Id, Head)
        IsNameNotInList(Id, Tail)
    'rule' IsNameNotInList(Id, nil):
        -- success

----------

-- Iterate over the tree, generating declarations for all used imported
-- definitions.
'sweep' GenerateImportedDefinitions(ANY)

    'rule' GenerateImportedDefinitions(TYPE'named(Position, Name)):
        QuerySymbolId(Name -> Info)
        Info'Type -> Type
        GenerateImportedDefinitions(Type)
        GenerateImportedDefinition(Name)
        
    'rule' GenerateImportedDefinitions(STATEMENT'call(_, Handler, Arguments)):
        GenerateImportedDefinition(Handler)
        GenerateImportedDefinitions(Arguments)

    --'rule' GenerateImportedDefinitions(STATEMENT'invoke(_, Info, Arguments)):
    --    GenerateImportedInvokeDefinition(Info)
    --    GenerateImportedDefinitions(Arguments)
        
    'rule' GenerateImportedDefinitions(EXPRESSION'slot(_, Name)):
        GenerateImportedDefinition(Name)

    'rule' GenerateImportedDefinitions(EXPRESSION'call(_, Handler, Arguments)):
        GenerateImportedDefinition(Handler)
        GenerateImportedDefinitions(Arguments)

    --'rule' GenerateImportedDefinitions(EXPRESSION'invoke(_, Info, Arguments)):
    --    GenerateImportedInvokeDefinition(Info)
    --    GenerateImportedDefinitions(Arguments)

'action' GenerateImportedDefinition(ID)

    'rule' GenerateImportedDefinition(Id):
        -- Only generate the an imported definition if it comes from another module.
        IsUngeneratedExternalId(Id)
        
        -- Get the module in which the definition is defined
        QueryModuleOfId(Id -> ModuleId)
        
        -- Ensure we have a dependency for the module
        GenerateModuleDependency(ModuleId -> ModuleIndex)
        ModuleId'Name -> ModuleName
        AddModuleToDependencyList(ModuleName)
        
        -- Fetch the info about the symbol.
        QuerySymbolId(Id -> SymbolInfo)
        Id'Name -> SymbolName
        SymbolInfo'Kind -> SymbolKind
        SymbolInfo'Type -> SymbolType
        
        -- Generate a type for the symbol (this is unused for now so just use undefined)
        -- GenerateType(SymbolType -> SymbolTypeIndex)
        EmitUndefinedType(-> SymbolTypeIndex)

        -- Now add the import definition to the module
        (|
            where(SymbolKind -> type)
            EmitImportedType(ModuleIndex, SymbolName, SymbolTypeIndex -> SymbolIndex)
        ||
            where(SymbolKind -> constant)
            EmitImportedConstant(ModuleIndex, SymbolName, SymbolTypeIndex -> SymbolIndex)
        ||
            where(SymbolKind -> variable)
            EmitImportedVariable(ModuleIndex, SymbolName, SymbolTypeIndex -> SymbolIndex)
        ||
            where(SymbolKind -> handler)
            EmitImportedHandler(ModuleIndex, SymbolName, SymbolTypeIndex -> SymbolIndex)
        |)
        
        -- We now have an index of an 'external definition' to use when referencing it.
        SymbolInfo'Index <- SymbolIndex
        
    'rule' GenerateImportedDefinition(Id):
        -- If we get here then either the id isn't imported, or we have previously
        -- generated it.

----

'action' GenerateModuleDependency(ID -> INT)
    
    'rule' GenerateModuleDependency(Id -> ModuleIndex):
        -- Get info about the module
        QueryModuleId(Id -> ModuleInfo)
        
        -- Fetch the module index
        ModuleInfo'Index -> CurrentModuleIndex
        [|
            -- If the module has been depended on yet, it will have index -1
            eq(CurrentModuleIndex, -1)
            Id'Name -> ModuleName
            
            -- Emit a dependency for the module and get its index
            EmitModuleDependency(ModuleName -> NewModuleIndex)
            ModuleInfo'Index <- NewModuleIndex

            AddModuleToDependencyList(ModuleName)
        |]
        
        -- Now return the updated module index
        ModuleInfo'Index -> ModuleIndex

----

'action' GenerateDefinitionIndexes(DEFINITION)

    'rule' GenerateDefinitionIndexes(sequence(Left, Right)):
        GenerateDefinitionIndexes(Left)
        GenerateDefinitionIndexes(Right)
        
    'rule' GenerateDefinitionIndexes(type(_, _, Name, _)):
        GenerateDefinitionIndex(Name)
    
    'rule' GenerateDefinitionIndexes(constant(_, _, Name, _)):
        GenerateDefinitionIndex(Name)
    
    'rule' GenerateDefinitionIndexes(variable(_, _, Name, _)):
        GenerateDefinitionIndex(Name)
    
    'rule' GenerateDefinitionIndexes(contextvariable(_, _, Name, _, _)):
        GenerateDefinitionIndex(Name)
    
    'rule' GenerateDefinitionIndexes(handler(_, _, Name, _, _, _, _)):
        GenerateDefinitionIndex(Name)

    'rule' GenerateDefinitionIndexes(foreignhandler(_, _, Name, _, _)):
        GenerateDefinitionIndex(Name)

    'rule' GenerateDefinitionIndexes(property(_, _, Name, _, _)):
        GenerateDefinitionIndex(Name)

    'rule' GenerateDefinitionIndexes(event(_, _, Name, _)):
        GenerateDefinitionIndex(Name)

    'rule' GenerateDefinitionIndexes(syntax(_, _, Name, _, _, _)):
        GenerateDefinitionIndex(Name)
    
    'rule' GenerateDefinitionIndexes(_):
        -- nothing

'action' GenerateDefinitionIndex(ID)

    'rule' GenerateDefinitionIndex(Id):
        QuerySyntaxId(Id -> Info)
        EmitDefinitionIndex(-> Index)
        Info'Index <- Index

    'rule' GenerateDefinitionIndex(Id):
        QuerySymbolId(Id -> Info)
        EmitDefinitionIndex(-> Index)
        Info'Index <- Index

'condition' IsRValueMethodPresent(SYNTAXMETHODLIST)

    'rule' IsRValueMethodPresent(methodlist(method(_, _, Arguments), Tail)):
        (|
            IsMarkTypePresentInArguments(output, Arguments)
        ||
            IsRValueMethodPresent(Tail)
        |)
        
'condition' IsLValueMethodPresent(SYNTAXMETHODLIST)

    'rule' IsLValueMethodPresent(methodlist(method(_, _, Arguments), Tail)):
        (|
            IsMarkTypePresentInArguments(input, Arguments)
        ||
            IsRValueMethodPresent(Tail)
        |)

'condition' IsMarkTypePresentInArguments(SYNTAXMARKTYPE, SYNTAXCONSTANTLIST)

    'rule' IsMarkTypePresentInArguments(MarkType, constantlist(variable(_, Name), _)):
        QuerySyntaxMarkId(Name -> Info)
        Info'Type -> Type
        eq(MarkType, Type)
        
    'rule' IsMarkTypePresentInArguments(MarkType, constantlist(indexedvariable(_, Name, _), _)):
        QuerySyntaxMarkId(Name -> Info)
        Info'Type -> Type
        eq(MarkType, Type)

    'rule' IsMarkTypePresentInArguments(MarkType, constantlist(_, Tail)):
        IsMarkTypePresentInArguments(MarkType, Tail)

----

'action' GenerateExportedDefinitions(DEFINITION)

    'rule' GenerateExportedDefinitions(sequence(Left, Right)):
        GenerateExportedDefinitions(Left)
        GenerateExportedDefinitions(Right)
        
    'rule' GenerateExportedDefinitions(type(_, public, Id, _)):
        GenerateExportedDefinition(Id)
        
    'rule' GenerateExportedDefinitions(constant(_, public, Id, _)):
        GenerateExportedDefinition(Id)
        
    'rule' GenerateExportedDefinitions(variable(_, public, Id, _)):
        GenerateExportedDefinition(Id)

    'rule' GenerateExportedDefinitions(contextvariable(_, public, Id, _, _)):
        GenerateExportedDefinition(Id)

    'rule' GenerateExportedDefinitions(handler(_, public, Id, _, _, _, _)):
        GenerateExportedDefinition(Id)

    'rule' GenerateExportedDefinitions(foreignhandler(_, public, Id, _, _)):
        GenerateExportedDefinition(Id)

    'rule' GenerateExportedDefinitions(property(_, _, Id, _, _)):
        GenerateExportedDefinition(Id)

    'rule' GenerateExportedDefinitions(event(_, _, Id, _)):
        GenerateExportedDefinition(Id)
        
    'rule' GenerateExportedDefinitions(syntax(_, _, Id, _, _, _)):
        GenerateExportedDefinition(Id)
        
    'rule' GenerateExportedDefinitions(_):
        -- Non-public, non-exportable definition fallthrough.

'action' GenerateExportedDefinition(ID)

    'rule' GenerateExportedDefinition(Id):
        QuerySyntaxId(Id -> Info)
        Info'Index -> Index
        ne(Index, -1)
        EmitExportedDefinition(Index)

    'rule' GenerateExportedDefinition(Id):
        QuerySymbolId(Id -> Info)
        Info'Index -> Index
        EmitExportedDefinition(Index)

----

'action' GenerateDefinitions(DEFINITION)

    'rule' GenerateDefinitions(sequence(Left, Right)):
        GenerateDefinitions(Left)
        GenerateDefinitions(Right)
        
    'rule' GenerateDefinitions(type(Position, _, Id, Type)):
        GenerateType(Type -> TypeIndex)
        
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        EmitTypeDefinition(DefIndex, Position, Name, TypeIndex)
        
    'rule' GenerateDefinitions(constant(Position, _, Id, Value)):
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        EmitConstant(Value -> Index)
        EmitConstantDefinition(DefIndex, Position, Name, Index)
        
    'rule' GenerateDefinitions(variable(Position, _, Id, Type)):
        GenerateType(Type -> TypeIndex)
        
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        EmitVariableDefinition(DefIndex, Position, Name, TypeIndex)

    'rule' GenerateDefinitions(contextvariable(Position, _, Id, Type, Default)):
        GenerateType(Type -> TypeIndex)
        EmitConstant(Default -> ConstIndex)

        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        EmitContextVariableDefinition(DefIndex, Position, Name, TypeIndex, ConstIndex)

    'rule' GenerateDefinitions(handler(Position, _, Id, Scope, Signature:signature(Parameters, _), _, Body)):
        GenerateType(handler(Position, Signature) -> TypeIndex)
        
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        (|
            where(Scope -> normal)
            EmitBeginHandlerDefinition(DefIndex, Position, Name, TypeIndex)
        ||
            where(Scope -> context)
            EmitBeginContextHandlerDefinition(DefIndex, Position, Name, TypeIndex)
        |)
        GenerateParameters(Parameters)
        CreateParameterRegisters(Parameters)
        CreateVariableRegisters(Body)
        EmitCreateRegister(-> ContextReg)
        EmitCreateRegister(-> ResultReg)
        GenerateBody(ResultReg, ContextReg, Body)
        EmitDestroyRegister(ContextReg)
        EmitDestroyRegister(ResultReg)
        EmitReturnNothing()
        DestroyVariableRegisters(Body)
        DestroyParameterRegisters(Parameters)
        EmitEndHandlerDefinition()

    'rule' GenerateDefinitions(foreignhandler(Position, _, Id, Signature, Binding)):
        GenerateType(handler(Position, Signature) -> TypeIndex)
        
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        EmitForeignHandlerDefinition(DefIndex, Position, Name, TypeIndex, Binding)
        
    'rule' GenerateDefinitions(property(Position, _, Id, Getter, OptionalSetter)):
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        QuerySymbolId(Getter -> GetInfo)
        GetInfo'Index -> GetIndex
        (|
            where(OptionalSetter -> id(Setter))
            QuerySymbolId(Setter -> SetInfo)
            SetInfo'Index -> SetIndex
        ||
            where(-1 -> SetIndex)
        |)
        EmitPropertyDefinition(DefIndex, Position, Name, GetIndex, SetIndex)
        
    'rule' GenerateDefinitions(event(Position, _, Id, Signature)):
        GenerateType(handler(Position, Signature) -> TypeIndex)

        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        EmitEventDefinition(DefIndex, Position, Name, TypeIndex)
        
    'rule' GenerateDefinitions(syntax(Position, _, Id, Class, _, _)):
        QuerySyntaxId(Id -> Info)
        Id'Name -> Name
        Info'Methods -> Methods
        Info'Index -> Index
        EmitBeginSyntaxDefinition(Index, Position, Name)
        GenerateSyntaxMethods(Methods)
        EmitEndSyntaxDefinition()
        
    'rule' GenerateDefinitions(metadata(_, _, _)):
        -- do nothing

    'rule' GenerateDefinitions(nil):
        -- nothing

----

'action' GenerateSyntaxMethods(SYNTAXMETHODLIST)

    'rule' GenerateSyntaxMethods(methodlist(method(Position, Id, Arguments), Tail)):
        QuerySymbolId(Id -> Info)
        Info'Index -> HandlerIndex
        EmitBeginSyntaxMethod(HandlerIndex)
        GenerateSyntaxMethodArguments(Arguments)
        EmitEndSyntaxMethod()
        
    'rule' GenerateSyntaxMethods(nil):
        -- do nothing

'action' GenerateSyntaxMethodArguments(SYNTAXCONSTANTLIST)

    'rule' GenerateSyntaxMethodArguments(constantlist(Head, Tail)):
        GenerateSyntaxMethodArgument(Head)
        GenerateSyntaxMethodArguments(Tail)
        
    'rule' GenerateSyntaxMethodArguments(nil):
        -- do nothing

'action' GenerateSyntaxMethodArgument(SYNTAXCONSTANT)

    'rule' GenerateSyntaxMethodArgument(undefined(_)):
        EmitUndefinedSyntaxMethodArgument()

    'rule' GenerateSyntaxMethodArgument(true(_)):
        EmitTrueSyntaxMethodArgument()

    'rule' GenerateSyntaxMethodArgument(false(_)):
        EmitFalseSyntaxMethodArgument()

    'rule' GenerateSyntaxMethodArgument(integer(_, Value)):
        EmitIntegerSyntaxMethodArgument(Value)

    'rule' GenerateSyntaxMethodArgument(real(_, Value)):
        EmitRealSyntaxMethodArgument(Value)

    'rule' GenerateSyntaxMethodArgument(string(_, Value)):
        EmitStringSyntaxMethodArgument(Value)

    'rule' GenerateSyntaxMethodArgument(variable(_, Name)):
        QuerySyntaxMarkId(Name -> Info)
        (|
            Info'Type -> context
            EmitContextSyntaxMethodArgument()
        ||
            Info'Type -> input
            EmitInputSyntaxMethodArgument()
        ||
            Info'Type -> output
            EmitOutputSyntaxMethodArgument()
        ||
            Info'Type -> iterator
            EmitIteratorSyntaxMethodArgument()
        ||
            Info'Type -> container
            EmitContainerSyntaxMethodArgument()
        ||
            Info'Index -> Index
            EmitVariableSyntaxMethodArgument(Index)
        |)

    'rule' GenerateSyntaxMethodArgument(indexedvariable(_, Name, ElementIndex)):
        QuerySyntaxMarkId(Name -> Info)
        Info'Index -> Index
        EmitIndexedVariableSyntaxMethodArgument(Index, ElementIndex)

----

'action' GenerateParameters(PARAMETERLIST)

    'rule' GenerateParameters(parameterlist(parameter(_, _, Id, Type), Rest)):
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        GenerateType(Type -> TypeIndex)
        EmitHandlerParameter(Name, TypeIndex -> Index)
        Info'Index <- Index
        GenerateParameters(Rest)

    'rule' GenerateParameters(nil):
        -- nothing
        
'action' CreateParameterRegisters(PARAMETERLIST)

    'rule' CreateParameterRegisters(parameterlist(parameter(_, _, _, _), Rest)):
        EmitCreateRegister(-> Register)
        CreateParameterRegisters(Rest)

    'rule' CreateParameterRegisters(nil):
        -- nothing

'sweep' CreateVariableRegisters(ANY)

    'rule' CreateVariableRegisters(STATEMENT'variable(_, _, _)):
        EmitCreateRegister(-> Register)

'action' DestroyParameterRegisters(PARAMETERLIST)

    'rule' DestroyParameterRegisters(parameterlist(parameter(_, _, Id, _), Rest)):
        QuerySymbolId(Id -> Info)
        Info'Index -> Index
        EmitDestroyRegister(Index)
        DestroyParameterRegisters(Rest)

    'rule' DestroyParameterRegisters(nil):
        -- nothing

'action' DestroyVariableRegisters(STATEMENT)

    'rule' DestroyVariableRegisters(sequence(Left, Right)):
        DestroyVariableRegisters(Left)
        DestroyVariableRegisters(Right)

    'rule' DestroyVariableRegisters(variable(_, Id, _)):
        QuerySymbolId(Id -> Info)
        Info'Index -> Index
        EmitDestroyRegister(Index)

    'rule' DestroyVariableRegisters(_):
        -- nothing

'action' GenerateBody(INT, INT, STATEMENT)

    'rule' GenerateBody(Result, Context, sequence(Left, Right)):
        GenerateBody(Result, Context, Left)
        GenerateBody(Result, Context, Right)

    'rule' GenerateBody(Result, Context, variable(Position, Id, Type)):
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        GenerateType(Type -> TypeIndex)
        EmitHandlerVariable(Name, TypeIndex -> Index)
        Info'Index <- Index
    
    'rule' GenerateBody(Result, Context, if(Position, Condition, Consequent, Alternate)):
        EmitDeferLabel(-> AlternateLabel)
        EmitDeferLabel(-> EndIfLabel)
        EmitPosition(Position)
        GenerateExpression(Result, Context, Condition -> ResultRegister)
        EmitJumpIfFalse(ResultRegister, AlternateLabel)
        EmitDestroyRegister(ResultRegister)
        GenerateBody(Result, Context, Consequent)
        EmitJump(EndIfLabel)
        EmitResolveLabel(AlternateLabel)
        GenerateBody(Result, Context, Alternate)
        EmitResolveLabel(EndIfLabel)
        
    'rule' GenerateBody(Result, Context, repeatforever(Position, Body)):
        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatTail)
        EmitPosition(Position)
        EmitPushRepeatLabels(RepeatHead, RepeatTail)
        EmitResolveLabel(RepeatHead)
        GenerateBody(Result, Context, Body)
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitPopRepeatLabels()
        
    'rule' GenerateBody(Result, Context, repeatcounted(Position, Count, Body)):
        -- repeat n times uses a builtin invoke:
        -- bool RepeatCountedIterator(inout count)
        --   if count == 0 then return false
        --   count -= 1
        --   return true
        GenerateExpression(Result, Context, Count -> CountRegister)
        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatHead, RepeatTail)

        EmitPosition(Position)
        EmitResolveLabel(RepeatHead)
        EmitCreateRegister(-> ContinueRegister)
        GenerateBeginBuiltinInvoke("RepeatCounted", Context, ContinueRegister)
        EmitContinueInvoke(CountRegister)
        EmitEndInvoke()
        EmitJumpIfFalse(ContinueRegister, RepeatTail)
        EmitDestroyRegister(ContinueRegister)
        GenerateBody(Result, Context, Body)
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitDestroyRegister(CountRegister)
        EmitPopRepeatLabels()
        
    'rule' GenerateBody(Result, Context, repeatwhile(Position, Condition, Body)):
        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatHead, RepeatTail)
        
        EmitPosition(Position)
        EmitResolveLabel(RepeatHead)
        GenerateExpression(Result, Context, Condition -> ContinueRegister)
        EmitJumpIfFalse(ContinueRegister, RepeatTail)
        EmitDestroyRegister(ContinueRegister)
        GenerateBody(Result, Context, Body)
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitPopRepeatLabels()

    'rule' GenerateBody(Result, Context, repeatuntil(Position, Condition, Body)):
        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatHead, RepeatTail)
        
        EmitPosition(Position)
        EmitResolveLabel(RepeatHead)
        GenerateExpression(Result, Context, Condition -> ContinueRegister)
        EmitJumpIfTrue(ContinueRegister, RepeatTail)
        EmitDestroyRegister(ContinueRegister)
        GenerateBody(Result, Context, Body)
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitPopRepeatLabels()
        
    'rule' GenerateBody(Result, Context, repeatupto(Position, Slot, Start, Finish, Step, Body)):
        QuerySymbolId(Slot -> Info)
        Info'Index -> VarIndex
        Info'Kind -> VarKind

        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatNext)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatNext, RepeatTail)
        
        EmitPosition(Position)
        GenerateExpression(Result, Context, Start -> CounterRegister)
        GenerateExpression(Result, Context, Finish -> LimitRegister)
        (|
            where(Step -> nil)
            EmitCreateRegister(-> StepRegister)
            EmitAssignInteger(StepRegister, 1)
        ||
            GenerateExpression(Result, Context, Step -> StepRegister)
        |)

        EmitResolveLabel(RepeatHead)
        EmitCreateRegister(-> ContinueRegister)
        GenerateBeginBuiltinInvoke("RepeatUpToCondition", Context, ContinueRegister)
        EmitContinueInvoke(CounterRegister)
        EmitContinueInvoke(LimitRegister)
        EmitEndInvoke()

        EmitStoreVar(VarKind, CounterRegister, VarIndex)
        EmitJumpIfFalse(ContinueRegister, RepeatTail)
        EmitDestroyRegister(ContinueRegister)

        GenerateBody(Result, Context, Body)

        EmitResolveLabel(RepeatNext)
        EmitFetchVar(VarKind, CounterRegister, VarIndex)
        GenerateBeginBuiltinInvoke("RepeatUpToIterate", Context, CounterRegister)
        EmitContinueInvoke(CounterRegister)
        EmitContinueInvoke(StepRegister)
        EmitEndInvoke()

        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitDestroyRegister(CounterRegister)
        EmitDestroyRegister(LimitRegister)
        EmitDestroyRegister(StepRegister)

    'rule' GenerateBody(Result, Context, repeatdownto(Position, Slot, Start, Finish, Step, Body)):
        QuerySymbolId(Slot -> Info)
        Info'Index -> VarIndex
        Info'Kind -> VarKind

        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatNext)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatNext, RepeatTail)
        
        EmitPosition(Position)
        GenerateExpression(Result, Context, Start -> CounterRegister)
        GenerateExpression(Result, Context, Finish -> LimitRegister)
        (|
            where(Step -> nil)
            EmitCreateRegister(-> StepRegister)
            EmitAssignInteger(StepRegister, -1)
        ||
            GenerateExpression(Result, Context, Step -> StepRegister)
        |)

        EmitResolveLabel(RepeatHead)
        EmitCreateRegister(-> ContinueRegister)
        GenerateBeginBuiltinInvoke("RepeatDownToCondition", Context, ContinueRegister)
        EmitContinueInvoke(CounterRegister)
        EmitContinueInvoke(LimitRegister)
        EmitEndInvoke()

        EmitStoreVar(VarKind, CounterRegister, VarIndex)
        EmitJumpIfFalse(ContinueRegister, RepeatTail)
        EmitDestroyRegister(ContinueRegister)

        GenerateBody(Result, Context, Body)

        EmitResolveLabel(RepeatNext)
        EmitFetchVar(VarKind, CounterRegister, VarIndex)
        GenerateBeginBuiltinInvoke("RepeatDownToIterate", Context, CounterRegister)
        EmitContinueInvoke(CounterRegister)
        EmitContinueInvoke(StepRegister)
        EmitEndInvoke()
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitDestroyRegister(CounterRegister)
        EmitDestroyRegister(LimitRegister)
        EmitDestroyRegister(StepRegister)
        
    'rule' GenerateBody(Result, Context, repeatforeach(Position, Invoke:invoke(_, IteratorInvokes, Arguments), Container, Body)):
        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatHead, RepeatTail)
        
        EmitPosition(Position)
        EmitCreateRegister(-> IteratorReg)
        EmitAssignUndefined(IteratorReg)
        GenerateExpression(Result, Context, Container -> TargetReg)
        
        EmitResolveLabel(RepeatHead)
        GenerateDefinitionGroupForInvokes(IteratorInvokes, iterate, Arguments -> Index, Signature)
        GenerateInvoke_EvaluateArguments(Result, Context, Signature, Arguments)
        EmitCreateRegister(-> ContinueReg)
        EmitBeginInvoke(Index, Context, ContinueReg)
        EmitContinueInvoke(IteratorReg)
        GenerateInvoke_EmitInvokeArguments(Arguments)
        EmitContinueInvoke(TargetReg)
        EmitEndInvoke()

        EmitJumpIfFalse(ContinueReg, RepeatTail)
        
        GenerateInvoke_AssignArguments(Result, Context, Signature, Arguments)
        GenerateInvoke_FreeArguments(Arguments)
        
        EmitDestroyRegister(ContinueReg)
        
        GenerateBody(Result, Context, Body)
        
        EmitJump(RepeatHead)
        
        EmitDestroyRegister(IteratorReg)
        EmitDestroyRegister(TargetReg)

        EmitResolveLabel(RepeatTail)

        
    'rule' GenerateBody(Result, Context, nextrepeat(Position)):
        EmitCurrentRepeatLabels(-> Next, _)
        EmitPosition(Position)
        EmitJump(Next)
        
    'rule' GenerateBody(Result, Context, exitrepeat(Position)):
        EmitCurrentRepeatLabels(-> _, Exit)
        EmitPosition(Position)
        EmitJump(Exit)
        
    'rule' GenerateBody(Result, Context, return(Position, Value)):
        EmitPosition(Position)
        (|
            where(Value -> nil)
            EmitReturnNothing()
        ||
            GenerateExpression(Result, Context, Value -> ReturnReg)
            EmitReturn(ReturnReg)
        |)
        
    'rule' GenerateBody(Result, Context, call(Position, Handler, Arguments)):
        EmitPosition(Position)
        GenerateCallInRegister(Result, Context, Position, Handler, Arguments, Result)

    'rule' GenerateBody(Result, Context, put(Position, Source, slot(_, Id))):
        EmitPosition(Position)
        GenerateInvoke_EvaluateArgumentForIn(Result, Context, Source)
        EmitGetRegisterAttachedToExpression(Source -> SrcReg)
        QuerySymbolId(Id -> Info)
        Info'Kind -> Kind
        Info'Index -> Index
        EmitStoreVar(Kind, SrcReg, Index)
        GenerateInvoke_FreeArgument(Source)
        EmitAssignUndefined(Result)

    'rule' GenerateBody(Result, Context, put(Position, Source, Target)):
        EmitPosition(Position)
        GenerateInvoke_EvaluateArgumentForIn(Result, Context, Source)
        GenerateInvoke_EvaluateArgumentForOut(Result, Context, Target)
        EmitGetRegisterAttachedToExpression(Source -> SrcReg)
        EmitGetRegisterAttachedToExpression(Target -> DstReg)
        EmitAssign(DstReg, SrcReg)
        GenerateInvoke_AssignArgument(Result, Context, Target)
        GenerateInvoke_FreeArgument(Source)
        GenerateInvoke_FreeArgument(Target)
        EmitAssignUndefined(Result)
        
    'rule' GenerateBody(Result, Context, get(Position, Value)):
        GenerateExpression(Result, Context, Value -> Reg)
        EmitAssign(Result, Reg)

    'rule' GenerateBody(Result, Context, invoke(Position, Invokes, Arguments)):
        EmitPosition(Position)
        GenerateDefinitionGroupForInvokes(Invokes, execute, Arguments -> Index, Signature)
        GenerateInvoke_EvaluateArguments(Result, Context, Signature, Arguments)
        EmitBeginInvoke(Index, Context, Result)
        GenerateInvoke_EmitInvokeArguments(Arguments)
        EmitEndInvoke()
        GenerateInvoke_AssignArguments(Result, Context, Signature, Arguments)
        GenerateInvoke_FreeArguments(Arguments)
        
    'rule' GenerateBody(Result, Context, throw(Position, Error)):
        EmitPosition(Position)
        GenerateExpression(Result, Context, Error -> Reg)
        GenerateBeginBuiltinInvoke("Throw", Context, Result)
        EmitContinueInvoke(Reg)
        EmitDestroyRegister(Reg)
        EmitAssignUndefined(Result)
        
    'rule' GenerateBody(Result, Context, postfixinto(Position, Command, slot(_, Id))):
        QuerySymbolId(Id -> Info)
        Info'Kind -> Kind
        Info'Index -> Index
        IsVariableInRegister(Kind)
        GenerateBody(Index, Context, Command)
        EmitAssignUndefined(Result)

    'rule' GenerateBody(Result, Context, postfixinto(Position, Command, Target)):
        EmitPosition(Position)
        GenerateInvoke_EvaluateArgumentForOut(Result, Context, Target)
        EmitGetRegisterAttachedToExpression(Target -> DstReg)
        GenerateBody(DstReg, Context, Command)
        GenerateInvoke_AssignArgument(Result, Context, Target)
        GenerateInvoke_FreeArgument(Target)
        EmitAssignUndefined(Result)

    'rule' GenerateBody(Result, Context, nil):
        -- nothing

'action' GenerateBeginBuiltinInvoke(STRING, INT, INT)

    'rule' GenerateBeginBuiltinInvoke(NameString, ContextReg, ResultReg):
        MakeNameLiteral("__builtin__" -> ModuleName)
        EmitModuleDependency(ModuleName -> ModuleIndex)
        MakeNameLiteral(NameString -> InvokeName)
        EmitUndefinedType(-> SymbolTypeIndex)
        EmitImportedHandler(ModuleIndex, InvokeName, SymbolTypeIndex -> SymbolIndex)
        EmitBeginInvoke(SymbolIndex, ContextReg, ResultReg)

----

-- Evaluate the arguments for the invoke with the given signature. This attaches
-- a register to each expression which is used to perform the invoke.
-- For 'in' arguments the expression is evaluated as normal as no assign-invoke
-- is required.
-- For 'inout' arguments the expression is evaluated as normal but if the expr is
-- an invoke, its argument registers are retained for use in the corresponding
-- assign-invoke.
-- For 'out' arguments the expression is not evaluated, but if the expr is an invoke
-- then its arguments are evaluated ready for the corresponding assign-invoke.
--
'action' GenerateInvoke_EvaluateArguments(INT, INT, INVOKESIGNATURE, EXPRESSIONLIST)

    'rule' GenerateInvoke_EvaluateArguments(ResultReg, ContextReg, invokesignature(in, Index, SigRest), Args):
        GetExpressionAtIndex(Args, Index -> Expr)
        GenerateInvoke_EvaluateArgumentForIn(ResultReg, ContextReg, Expr)
        GenerateInvoke_EvaluateArguments(ResultReg, ContextReg, SigRest, Args)
        
    'rule' GenerateInvoke_EvaluateArguments(ResultReg, ContextReg, invokesignature(inout, Index, SigRest), Args):
        GetExpressionAtIndex(Args, Index -> Expr)
        GenerateInvoke_EvaluateArgumentForInOut(ResultReg, ContextReg, Expr)
        GenerateInvoke_EvaluateArguments(ResultReg, ContextReg, SigRest, Args)

    'rule' GenerateInvoke_EvaluateArguments(ResultReg, ContextReg, invokesignature(out, Index, SigRest), Args):
        GetExpressionAtIndex(Args, Index -> Expr)
        GenerateInvoke_EvaluateArgumentForOut(ResultReg, ContextReg, Expr)
        GenerateInvoke_EvaluateArguments(ResultReg, ContextReg, SigRest, Args)
        
    'rule' GenerateInvoke_EvaluateArguments(_, _, nil, _):
        -- do nothing.

-- In arguments are simple, just evaluate the expr into a register attached to the
-- node.
'action' GenerateInvoke_EvaluateArgumentForIn(INT, INT, EXPRESSION)

    'rule' GenerateInvoke_EvaluateArgumentForIn(ResultReg, ContextReg, nil):
        -- do nothing for nil arguments.

    'rule' GenerateInvoke_EvaluateArgumentForIn(ResultReg, ContextReg, Expr:slot(_, Id)):
        QuerySymbolId(Id -> Info)
        (|
            Info'Kind -> parameter
        ||
            Info'Kind -> local
        |)
        Info'Index -> Register
        EmitAttachRegisterToExpression(Register, Expr)

    'rule' GenerateInvoke_EvaluateArgumentForIn(ResultReg, ContextReg, Expr):
        EmitCreateRegister(-> OutputReg)
        EmitAttachRegisterToExpression(OutputReg, Expr)
        GenerateExpressionInRegister(ResultReg, ContextReg, Expr, OutputReg)

-- Out arguments are a little bit trickier. If the expr is an invoke then we must
-- evaluate its arguments, but do no more. Otherwise, it has to be something we
-- can store in (i.e. slot). If it is neither of these things then our compiler has
-- not checked things properly!
'action' GenerateInvoke_EvaluateArgumentForOut(INT, INT, EXPRESSION)

    'rule' GenerateInvoke_EvaluateArgumentForOut(ResultReg, ContextReg, Invoke:invoke(_, Invokes, Arguments)):
        EmitCreateRegister(-> OutputReg)
        EmitAttachRegisterToExpression(OutputReg, Invoke)
        GenerateDefinitionGroupForInvokes(Invokes, assign, Arguments -> Index, Signature)
        GenerateInvoke_EvaluateArguments(ResultReg, ContextReg, Signature, Arguments)

    'rule' GenerateInvoke_EvaluateArgumentForOut(ResultReg, ContextReg, Expr:slot(_, Id)):
        QuerySymbolId(Id -> Info)
        (|
            Info'Kind -> parameter
        ||
            Info'Kind -> local
        |)
        Info'Index -> Register
        EmitAttachRegisterToExpression(Register, Expr)

    'rule' GenerateInvoke_EvaluateArgumentForOut(ResultReg, ContextReg, Slot:slot(_, _)):
        EmitCreateRegister(-> OutputReg)
        EmitAttachRegisterToExpression(OutputReg, Slot)
        
    'rule' GenerateInvoke_EvaluateArgumentForOut(ResultReg, ContextReg, nil):
        -- do nothing for nil arguments

    'rule' GenerateInvoke_EvaluateArgumentForOut(ResultReg, ContextReg, _):
        Fatal_InternalInconsistency("Invalid expression for out argument not checked properly!")

-- Inout arguments are a combination of 'in' and 'out'. If the expr is an invoke then
-- we must evaluate its arguments and call its evaluate side. Otherwise the expr has to
-- be a slot which we must also evaluate. Anything else is an inconsistency.
'action' GenerateInvoke_EvaluateArgumentForInOut(INT, INT, EXPRESSION)

    'rule' GenerateInvoke_EvaluateArgumentForInOut(ResultReg, ContextReg, Invoke:invoke(_, Invokes, Arguments)):
        EmitCreateRegister(-> OutputReg)
        EmitAttachRegisterToExpression(OutputReg, Invoke)
        GenerateDefinitionGroupForInvokes(Invokes, evaluate, Arguments -> Index, Signature)
        GenerateInvoke_EvaluateArguments(ResultReg, ContextReg, Signature, Arguments)
        EmitCreateRegister(-> IgnoredResultReg)
        EmitBeginInvoke(Index, ContextReg, IgnoredResultReg)
        GenerateInvoke_EmitInvokeArguments(Arguments)
        EmitContinueInvoke(OutputReg)
        EmitEndInvoke()
        EmitDestroyRegister(IgnoredResultReg)
        GenerateInvoke_AssignArguments(ResultReg, ContextReg, Signature, Arguments)

    'rule' GenerateInvoke_EvaluateArgumentForInOut(ResultReg, ContextReg, Expr:slot(_, Id)):
        QuerySymbolId(Id -> Info)
        (|
            Info'Kind -> parameter
        ||
            Info'Kind -> local
        |)
        Info'Index -> Register
        EmitAttachRegisterToExpression(Register, Expr)

    'rule' GenerateInvoke_EvaluateArgumentForInOut(ResultReg, ContextReg, Slot:slot(_, _)):
        EmitCreateRegister(-> OutputReg)
        EmitAttachRegisterToExpression(OutputReg, Slot)
        GenerateExpressionInRegister(ResultReg, ContextReg, Slot, OutputReg)
        
    'rule' GenerateInvoke_EvaluateArgumentForInOut(ResultReg, ContextReg, nil):
        -- do nothing for nil arguments

    'rule' GenerateInvoke_EvaluateArgumentForInOut(ResultReg, ContextReg, _):
        Fatal_InternalInconsistency("Invalid expression for inout argument not checked properly!")

----

'action' GenerateInvoke_AssignArguments(INT, INT, INVOKESIGNATURE, EXPRESSIONLIST)

    'rule' GenerateInvoke_AssignArguments(ResultReg, ContextReg, invokesignature(in, Index, SigRest), Args):
        -- nothing to do for in arguments
        GetExpressionAtIndex(Args, Index -> Expr)
        GenerateInvoke_AssignArguments(ResultReg, ContextReg, SigRest, Args)
        
    'rule' GenerateInvoke_AssignArguments(ResultReg, ContextReg, invokesignature(_, Index, SigRest), Args):
        -- out and inout are the same
        GetExpressionAtIndex(Args, Index -> Expr)
        GenerateInvoke_AssignArgument(ResultReg, ContextReg, Expr)
        GenerateInvoke_AssignArguments(ResultReg, ContextReg, SigRest, Args)

    'rule' GenerateInvoke_AssignArguments(_, _, nil, _):
        -- do nothing.
        
'action' GenerateInvoke_AssignArgument(INT, INT, EXPRESSION)

    'rule' GenerateInvoke_AssignArgument(ResultReg, ContextReg, Invoke:invoke(_, Invokes, Arguments)):
        EmitGetRegisterAttachedToExpression(Invoke -> InputReg)
        GenerateDefinitionGroupForInvokes(Invokes, assign, Arguments -> Index, Signature)
        [|
            ne(Signature, nil)
            EmitCreateRegister(-> IgnoredResultReg)
            EmitBeginInvoke(Index, ContextReg, IgnoredResultReg)
            EmitContinueInvoke(InputReg)
            GenerateInvoke_EmitInvokeArguments(Arguments)
            EmitEndInvoke()
            EmitDestroyRegister(IgnoredResultReg)
        |]
        GenerateInvoke_AssignArguments(ResultReg, ContextReg, Signature, Arguments)
        
    'rule' GenerateInvoke_AssignArgument(ResultReg, ContextReg, Slot:slot(_, Id)):
        QuerySymbolId(Id -> Info)
        (|
            Info'Kind -> parameter
        ||
            Info'Kind -> local
        |)
        -- Nothing more to do as the invoke will have done the assign.

    'rule' GenerateInvoke_AssignArgument(ResultReg, ContextReg, Slot:slot(_, Id)):
        EmitGetRegisterAttachedToExpression(Slot -> InputReg)
        QuerySymbolId(Id -> Info)
        Info'Kind -> Kind
        Info'Index -> Index
        EmitStoreVar(Kind, InputReg, Index)

----

'action' GenerateInvoke_FreeArguments(EXPRESSIONLIST)

    'rule' GenerateInvoke_FreeArguments(expressionlist(Expr, Rest)):
        GenerateInvoke_FreeArgument(Expr)
        GenerateInvoke_FreeArguments(Rest)
        
    'rule' GenerateInvoke_FreeArguments(nil):
        -- do nothing
        
'action' GenerateInvoke_FreeArgument(EXPRESSION)

    'rule' GenerateInvoke_FreeArgument(Expr:invoke(_, _, Arguments)):
        [|
            EmitGetRegisterAttachedToExpression(Expr -> Reg)
            EmitDestroyRegister(Reg)
            EmitDetachRegisterFromExpression(Expr)
        |]
        GenerateInvoke_FreeArguments(Arguments)
    
    'rule' GenerateInvoke_FreeArgument(Expr:slot(_, Id)):
        QuerySymbolId(Id -> Info)
        (|
            Info'Kind -> parameter
        ||
            Info'Kind -> local
        |)
        EmitDetachRegisterFromExpression(Expr)

    'rule' GenerateInvoke_FreeArgument(Expr):
        [|
            EmitGetRegisterAttachedToExpression(Expr -> Reg)
            EmitDestroyRegister(Reg)
            EmitDetachRegisterFromExpression(Expr)
        |]

    'rule' GenerateInvoke_FreeArgument(_):
        -- nothing to do

----

'action' GenerateInvoke_EmitInvokeArguments(EXPRESSIONLIST)

    'rule' GenerateInvoke_EmitInvokeArguments(expressionlist(Head, Tail)):
        EmitGetRegisterAttachedToExpression(Head -> Reg)
        EmitContinueInvoke(Reg)
        GenerateInvoke_EmitInvokeArguments(Tail)

    'rule' GenerateInvoke_EmitInvokeArguments(expressionlist(nil, Tail)):
        GenerateInvoke_EmitInvokeArguments(Tail)

    'rule' GenerateInvoke_EmitInvokeArguments(nil):
        -- do nothing

----

'action' GenerateDefinitionGroupForInvokes(INVOKELIST, INVOKEMETHODTYPE, EXPRESSIONLIST -> INT, INVOKESIGNATURE)

    'rule' GenerateDefinitionGroupForInvokes(InvokeList, Type, Arguments -> Index, Signature)
        EmitBeginDefinitionGroup()
        GenerateDefinitionGroupForInvokeList(InvokeList, Type, Arguments -> Signature)
        EmitEndDefinitionGroup(-> Index)

'action' GenerateDefinitionGroupForInvokeList(INVOKELIST, INVOKEMETHODTYPE, EXPRESSIONLIST -> INVOKESIGNATURE)

    'rule' GenerateDefinitionGroupForInvokeList(invokelist(Head, Tail), Type, Arguments -> Signature):
        Head'ModuleName -> ModuleName
        Head'Methods -> Methods
        GenerateDefinitionGroupForInvokeMethodList(ModuleName, Type, Arguments, Methods -> HeadSig)
        GenerateDefinitionGroupForInvokeList(Tail, Type, Arguments -> TailSig)
        (|
            eq(HeadSig, nil)
            where(TailSig -> Signature)
        ||
            where(HeadSig -> Signature)
        |)
        
    'rule' GenerateDefinitionGroupForInvokeList(nil, _, _ -> nil):
        -- do nothing

'action' GenerateDefinitionGroupForInvokeMethodList(STRING, INVOKEMETHODTYPE, EXPRESSIONLIST, INVOKEMETHODLIST -> INVOKESIGNATURE)
    
    'rule' GenerateDefinitionGroupForInvokeMethodList(ModuleNameString, WantType, Arguments, methodlist(SymbolNameString, IsType, Signature, Tail) -> OutSig):
        (|
            eq(WantType, IsType)
            AreAllArgumentsDefinedForInvokeMethod(Arguments, Signature)

            CountDefinedArguments(Arguments -> ArgCount)
            CountInvokeParameters(Signature -> ParamCount)
            eq(ArgCount, ParamCount)

            MakeNameLiteral(ModuleNameString -> ModuleName)
            
            IgnoredModuleList -> IgnoredModules
            IsNameNotInList(ModuleName, IgnoredModules)
            
            EmitModuleDependency(ModuleName -> ModuleIndex)
            AddModuleToDependencyList(ModuleName)
            
            MakeNameLiteral(SymbolNameString -> SymbolName)
            EmitUndefinedType(-> SymbolTypeIndex)
            EmitImportedHandler(ModuleIndex, SymbolName, SymbolTypeIndex -> SymbolIndex)
            
            EmitContinueDefinitionGroup(SymbolIndex)
            
            where(Signature -> HeadSig)
        ||
            where(INVOKESIGNATURE'nil -> HeadSig)
        |)
        GenerateDefinitionGroupForInvokeMethodList(ModuleNameString, WantType, Arguments, Tail -> TailSig)
        (|
            eq(HeadSig, nil)
            where(TailSig -> OutSig)
        ||
            where(HeadSig -> OutSig)
        |)

        
     'rule' GenerateDefinitionGroupForInvokeMethodList(_, _, _, nil -> nil):
        -- do nothing

'action' CountDefinedArguments(EXPRESSIONLIST -> INT)
    'rule' CountDefinedArguments(expressionlist(nil, Rest) -> Count):
        CountDefinedArguments(Rest -> Count)
    'rule' CountDefinedArguments(expressionlist(Head, Rest) -> Count + 1):
        CountDefinedArguments(Rest -> Count)
    'rule' CountDefinedArguments(nil -> 0):
        -- do nothing

'action' CountInvokeParameters(INVOKESIGNATURE -> INT)
    'rule' CountInvokeParameters(invokesignature(_, Index, Rest) -> Count)
        eq(Index, -1)
        CountInvokeParameters(Rest -> Count)
    'rule' CountInvokeParameters(invokesignature(_, _, Rest) -> Count + 1)
        CountInvokeParameters(Rest -> Count)
    'rule' CountInvokeParameters(nil -> 0):
        -- do nothing

'condition' AreAllArgumentsDefinedForInvokeMethod(EXPRESSIONLIST, INVOKESIGNATURE)

    'rule' AreAllArgumentsDefinedForInvokeMethod(Arguments, invokesignature(_, Index, Tail)):
        eq(Index, -1)
        AreAllArgumentsDefinedForInvokeMethod(Arguments, Tail)
        
    'rule' AreAllArgumentsDefinedForInvokeMethod(Arguments, invokesignature(_, Index, Tail)):
        GetExpressionAtIndex(Arguments, Index -> Arg)
        ne(Arg, nil)
        AreAllArgumentsDefinedForInvokeMethod(Arguments, Tail)

    'rule' AreAllArgumentsDefinedForInvokeMethod(Arguments, nil):
        -- do nothing

'action' GetExpressionAtIndex(EXPRESSIONLIST, INT -> EXPRESSION)

    'rule' GetExpressionAtIndex(expressionlist(Head, Tail), Index -> Head):
        eq(Index, 0)
        
    'rule' GetExpressionAtIndex(expressionlist(_, Tail), Index -> Head):
        GetExpressionAtIndex(Tail, Index - 1 -> Head)
        
    'rule' GetExpressionAtIndex(nil, Index -> nil):
        -- off the end!

----

'action' FullyResolveType(TYPE -> TYPE)

'action' GenerateCallInRegister(INT, INT, POS, ID, EXPRESSIONLIST, INT)

    'rule' GenerateCallInRegister(Result, Context, Position, Handler, Arguments, Output):
        QuerySymbolId(Handler -> Info)
        Info'Index -> Index
        Info'Kind -> Kind
        Info'Type -> Type
        FullyResolveType(Type -> handler(_, signature(HandlerSig, _)))
        GenerateCall_GetInvokeSignature(HandlerSig, 0 -> InvokeSig)

        (|
            -- If the Id is not a handler it must be a variable
            ne(Kind, handler)
            EmitCreateRegister(-> HandlerReg)
            EmitFetchVar(Kind, HandlerReg, Index)
        ||
            where(-1 -> HandlerReg)
        |)
        
        GenerateInvoke_EvaluateArguments(Result, Context, InvokeSig, Arguments)
        
        (|
            ne(Kind, handler)
            EmitBeginIndirectInvoke(HandlerReg, Context, Output)
        ||
            EmitBeginInvoke(Index, Context, Output)
        |)
        
        GenerateInvoke_EmitInvokeArguments(Arguments)
        EmitEndInvoke
        GenerateInvoke_AssignArguments(Result, Context, InvokeSig, Arguments)
        GenerateInvoke_FreeArguments(Arguments)
        
        [|
            ne(Kind, handler)
            EmitDestroyRegister(HandlerReg)
        |]

'action' GenerateCall_GetInvokeSignature(PARAMETERLIST, INT -> INVOKESIGNATURE)

    'rule' GenerateCall_GetInvokeSignature(parameterlist(parameter(_, Mode, _, _), ParamRest), Index -> invokesignature(Mode, Index, InvokeRest)):
        GenerateCall_GetInvokeSignature(ParamRest, Index + 1 -> InvokeRest)
    
    'rule' GenerateCall_GetInvokeSignature(nil, _ -> nil):
        -- do nothing

----

'condition' IsExpressionSimpleConstant(EXPRESSION)

'action' GenerateExpression(INT, INT, EXPRESSION -> INT)

    'rule' GenerateExpression(Result, Context, Expr -> Output):
        EmitCreateRegister(-> Output)
        GenerateExpressionInRegister(Result, Context, Expr, Output)

'action' GenerateExpressionInRegister(INT, INT, EXPRESSION, INT)

    'rule' GenerateExpressionInRegister(Result, Context, undefined(_), Output):
        EmitAssignUndefined(Output)

    'rule' GenerateExpressionInRegister(Result, Context, true(_), Output):
        EmitAssignTrue(Output)
        
    'rule' GenerateExpressionInRegister(Result, Context, false(_), Output):
        EmitAssignFalse(Output)
        
    'rule' GenerateExpressionInRegister(Result, Context, integer(_, Value), Output):
        EmitAssignInteger(Output, Value)
        
    'rule' GenerateExpressionInRegister(Result, Context, real(_, Value), Output):
        EmitAssignReal(Output, Value)
        
    'rule' GenerateExpressionInRegister(Result, Context, string(_, Value), Output):
        EmitAssignString(Output, Value)
        
    'rule' GenerateExpressionInRegister(Result, Context, slot(_, Id), Output):
        QuerySymbolId(Id -> Info)
        Info'Kind -> Kind
        Info'Index -> Index
        EmitFetchVar(Kind, Output, Index)
        
    'rule' GenerateExpressionInRegister(Result, Context, result(_), Output):
        EmitAssign(Output, Result)
        
    'rule' GenerateExpressionInRegister(Result, Context, logicalor(_, Left, Right), Output):
        EmitDeferLabel(-> ShortLabel)
        GenerateExpressionInRegister(Result, Context, Left, Output)
        EmitJumpIfTrue(Output, ShortLabel)
        GenerateExpressionInRegister(Result, Context, Right, Output)
        EmitResolveLabel(ShortLabel)

    'rule' GenerateExpressionInRegister(Result, Context, logicaland(_, Left, Right), Output):
        EmitDeferLabel(-> ShortLabel)
        GenerateExpressionInRegister(Result, Context, Left, Output)
        EmitJumpIfFalse(Output, ShortLabel)
        GenerateExpressionInRegister(Result, Context, Right, Output)
        EmitResolveLabel(ShortLabel)

    'rule' GenerateExpressionInRegister(Result, Context, as(_, _, _), Output):
        -- TODO
    
    'rule' GenerateExpressionInRegister(Result, Context, List:list(Position, Elements), Output):
        (|
            IsExpressionSimpleConstant(List)
            EmitConstant(List -> Index)
            EmitAssignConstant(Output, Index)
        ||
            GenerateExpressionList(Result, Context, Elements -> ListRegs)
            EmitBeginAssignList(Output)
            GenerateAssignList(ListRegs)
            EmitEndAssignList()
            EmitDestroyRegisterList(ListRegs)
        |)
    
    'rule' GenerateExpressionInRegister(Result, Context, call(Position, Handler, Arguments), Output):
        GenerateCallInRegister(Result, Context, Position, Handler, Arguments, Output)
    
    'rule' GenerateExpressionInRegister(Result, Context, invoke(_, Invokes, Arguments), Output):
        GenerateDefinitionGroupForInvokes(Invokes, evaluate, Arguments -> Index, Signature)
        GenerateInvoke_EvaluateArguments(Result, Context, Signature, Arguments)
        EmitCreateRegister(-> IgnoredReg)
        EmitBeginInvoke(Index, Context, IgnoredReg)
        GenerateInvoke_EmitInvokeArguments(Arguments)
        EmitContinueInvoke(Output)
        EmitEndInvoke()
        EmitDestroyRegister(IgnoredReg)
        GenerateInvoke_AssignArguments(Result, Context, Signature, Arguments)
        GenerateInvoke_FreeArguments(Arguments)
        
    'rule' GenerateExpressionInRegister(_, _, Expr, _):
        print(Expr)
        eq(0, 1)

----

'action' GenerateAssignList(INTLIST)

    'rule' GenerateAssignList(intlist(Head, Tail)):
        EmitContinueAssignList(Head)
        GenerateAssignList(Tail)

    'rule' GenerateAssignList(nil):
        -- finished

'action' EmitAssignUndefined(INT)

    'rule' EmitAssignUndefined(Reg):
        EmitUndefinedConstant(-> Index)
        EmitAssignConstant(Reg, Index)

'action' EmitAssignTrue(INT)

    'rule' EmitAssignTrue(Reg):
        EmitTrueConstant(-> Index)
        EmitAssignConstant(Reg, Index)

'action' EmitAssignFalse(INT)

    'rule' EmitAssignFalse(Reg):
        EmitFalseConstant(-> Index)
        EmitAssignConstant(Reg, Index)

'action' EmitAssignInteger(INT, INT)

    'rule' EmitAssignInteger(Reg, Value):
        EmitIntegerConstant(Value -> Index)
        EmitAssignConstant(Reg, Index)

'action' EmitAssignReal(INT, DOUBLE)

    'rule' EmitAssignReal(Reg, Value):
        EmitRealConstant(Value -> Index)
        EmitAssignConstant(Reg, Index)
        
'action' EmitAssignString(INT, STRING)

    'rule' EmitAssignString(Reg, Value):
        EmitStringConstant(Value -> Index)
        EmitAssignConstant(Reg, Index)

'action' EmitConstant(EXPRESSION -> INT)

    'rule' EmitConstant(undefined(_) -> Index):
        EmitUndefinedConstant(-> Index)

    'rule' EmitConstant(true(_) -> Index):
        EmitTrueConstant(-> Index)

    'rule' EmitConstant(false(_) -> Index):
        EmitFalseConstant(-> Index)

    'rule' EmitConstant(integer(_, IntValue) -> Index):
        EmitIntegerConstant(IntValue -> Index)

    'rule' EmitConstant(real(_, RealValue) -> Index):
        EmitRealConstant(RealValue -> Index)

    'rule' EmitConstant(string(_, StringValue) -> Index):
        EmitStringConstant(StringValue -> Index)

    'rule' EmitConstant(list(_, Elements) -> Index):
        EmitListConstantElements(Elements -> Indicies)
        EmitBeginListConstant()
        EmitListConstant(Indicies)
        EmitEndListConstant(-> Index)

'action' EmitListConstantElements(EXPRESSIONLIST -> INTLIST)

    'rule' EmitListConstantElements(expressionlist(Head, Tail) -> intlist(Index, Rest)):
        EmitConstant(Head -> Index)
        EmitListConstantElements(Tail -> Rest)
        
    'rule' EmitListConstantElements(nil -> nil):
        -- nothing

'action' EmitListConstant(INTLIST)

    'rule' EmitListConstant(intlist(Head, Tail)):
        EmitContinueListConstant(Head)
        EmitListConstant(Tail)
        
    'rule' EmitListConstant(nil):
        -- nothing

'condition' IsVariableInRegister(SYMBOLKIND)

    'rule' IsVariableInRegister(local):
    'rule' IsVariableInRegister(parameter):


'action' EmitStoreVar(SYMBOLKIND, INT, INT)
        
    'rule' EmitStoreVar(Kind, Reg, Var):
        IsVariableInRegister(Kind)
        EmitAssign(Var, Reg)

    -- This catches all module-scope things, including variables and handler references.
    'rule' EmitStoreVar(_, Reg, Var):
        EmitStore(Reg, Var, 0)

'action' EmitFetchVar(SYMBOLKIND, INT, INT)

    'rule' EmitFetchVar(Kind, Reg, Var):
        IsVariableInRegister(Kind)
        EmitAssign(Reg, Var)

    -- This catches all module-scope things, including variables and handler references.
    'rule' EmitFetchVar(_, Reg, Var):
        EmitFetch(Reg, Var, 0)
        
'action' EmitInvokeRegisterList(INTLIST)

    'rule' EmitInvokeRegisterList(intlist(Head, Tail)):
        EmitContinueInvoke(Head)
        EmitInvokeRegisterList(Tail)
        
    'rule' EmitInvokeRegisterList(nil):
        -- nothing

'action' EmitDestroyRegisterList(INTLIST)

    'rule' EmitDestroyRegisterList(intlist(Head, Tail)):
        EmitDestroyRegister(Head)
        EmitDestroyRegisterList(Tail)
        
    'rule' EmitDestroyRegisterList(nil):
        -- nothing

'action' GenerateExpressionList(INT, INT, EXPRESSIONLIST -> INTLIST)

    'rule' GenerateExpressionList(Result, Context, expressionlist(Head, Tail) -> intlist(HeadReg, TailRegs)):
        GenerateExpression(Result, Context, Head -> HeadReg)
        GenerateExpressionList(Result, Context, Tail -> TailRegs)
        
    'rule' GenerateExpressionList(_, _, nil -> nil)
        -- nothing

--------------------------------------------------------------------------------

'condition' IsNamedTypeId(ID)

    'rule' IsNamedTypeId(Id):
        -- If the id is defined in another module then it is a named type.
        -- If the id is defined in this module and is public then it is a named type.
        -- Otherwise it is an alias to an 'unnamed' type.
        QuerySymbolId(Id -> Info)
        Info'Parent -> ModuleId
        QueryModuleId(ModuleId -> ModuleInfo)
        ModuleInfo'Index -> Index
        (|
            ne(Index, 0)
        ||
            Info'Access -> public
        |)


'action' ResolveType(TYPE -> TYPE)

    'rule' ResolveType(optional(_, Base) -> Base):
    'rule' ResolveType(ThisType:named(_, Id) -> Base):
        QuerySymbolId(Id -> Info)
        Info'Type -> Type
        (|
            where(Type -> named(_, _))
            ResolveType(Type -> Base)
        ||
            where(Type -> optional(_, _))
            ResolveType(Type -> Base)
        ||
            where(Type -> foreign(_, _))
            where(ThisType -> Base)
        ||
            where(Type -> handler(_, _))
            where(ThisType -> Base)
        ||
            where(Type -> record(_, _, _))
            where(ThisType -> Base)
        ||
            where(Type -> Base)
        |)
    'rule' ResolveType(Type -> Type):
        -- do nothing

'action' GenerateType(TYPE -> INT)

    'rule' GenerateType(Type -> Index):
        ResolveType(Type -> ActualType)
        GenerateBaseType(ActualType -> BaseTypeIndex)
        (|
            IsTypeOptional(Type)
            EmitOptionalType(BaseTypeIndex -> Index)
        ||
            where(BaseTypeIndex -> Index)
        |)

'action' GenerateBaseType(TYPE -> INT)

    'rule' GenerateBaseType(optional(_, Base) -> Index):
        GenerateBaseType(Base -> BaseTypeIndex)
        EmitOptionalType(BaseTypeIndex -> Index)

    'rule' GenerateBaseType(Type:named(_, Id) -> Index):
        ResolveType(Type -> DefinedType)
        QuerySymbolId(Id -> Info)
        Info'Index -> DefinedIndex
        EmitDefinedType(DefinedIndex -> Index)
        
    'rule' GenerateBaseType(foreign(_, Binding) -> Index):
        EmitForeignType(Binding -> Index)

    'rule' GenerateBaseType(record(_, Base, Fields) -> Index):
        GenerateType(Base -> BaseTypeIndex)
        EmitBeginRecordType(BaseTypeIndex)
        GenerateRecordTypeFields(Fields)
        EmitEndRecordType(-> Index)

    'rule' GenerateBaseType(handler(_, signature(Parameters, ReturnType)) -> Index):
        GenerateType(ReturnType -> ReturnTypeIndex)
        EmitBeginHandlerType(ReturnTypeIndex)
        GenerateHandlerTypeParameters(Parameters)
        EmitEndHandlerType(-> Index)

    'rule' GenerateBaseType(any(_) -> Index):
        EmitAnyType(-> Index)
    'rule' GenerateBaseType(undefined(_) -> Index):
        EmitUndefinedType(-> Index)

    'rule' GenerateBaseType(boolean(_) -> Index):
        EmitBooleanType(-> Index)
    'rule' GenerateBaseType(integer(_) -> Index):
        EmitIntegerType(-> Index)
    'rule' GenerateBaseType(real(_) -> Index):
        EmitRealType(-> Index)
    'rule' GenerateBaseType(number(_) -> Index):
        EmitNumberType(-> Index)
    'rule' GenerateBaseType(string(_) -> Index):
        EmitStringType(-> Index)
    'rule' GenerateBaseType(data(_) -> Index):
        EmitDataType(-> Index)
    'rule' GenerateBaseType(array(_) -> Index):
        EmitArrayType(-> Index)
    'rule' GenerateBaseType(list(_, _) -> Index):
        EmitListType(-> Index)

    'rule' GenerateBaseType(Type -> 0):
        print(Type)
        Fatal_InternalInconsistency("attempt to generate uncoded type")

'action' GenerateRecordTypeFields(FIELDLIST)

    'rule' GenerateRecordTypeFields(fieldlist(slot(_, Id, Type), Tail)):
        GenerateType(Type -> TypeIndex)
        Id'Name -> Name
        EmitRecordTypeField(Name, TypeIndex)
        GenerateRecordTypeFields(Tail)
        
    'rule' GenerateRecordTypeFields(nil):
        -- nothing

'action' GenerateHandlerTypeParameters(PARAMETERLIST)

    'rule' GenerateHandlerTypeParameters(parameterlist(parameter(_, Mode, Id, Type), Rest)):
        GenerateType(Type -> TypeIndex)
        Id'Name -> Name
        (|
            where(Mode -> in)
            EmitHandlerTypeInParameter(Name, TypeIndex)
        ||
            where(Mode -> out)
            EmitHandlerTypeOutParameter(Name, TypeIndex)
        ||
            where(Mode -> inout)
            EmitHandlerTypeInOutParameter(Name, TypeIndex)
        |)
        GenerateHandlerTypeParameters(Rest)
        
    'rule' GenerateHandlerTypeParameters(nil):
        -- nothing

--------------------------------------------------------------------------------

'condition' IsUngeneratedExternalId(ID)

    'rule' IsUngeneratedExternalId(Id):
        -- Ungenerated if index is -1
        QuerySymbolId(Id -> Info)
        Info'Index -> Index
        eq(Index, -1)

        -- Extenal if module index is not 0
        Info'Parent -> ModuleId
        QueryModuleId(ModuleId -> ModuleInfo)
        ModuleInfo'Index -> ModuleIndex

        ne(ModuleIndex, 0)

'condition' IsExternalId(ID)

    'rule' IsExternalId(Id):
        -- Extenal if module index is not 0
        QuerySymbolId(Id -> Info)
        Info'Parent -> ModuleId
        QueryModuleId(ModuleId -> ModuleInfo)
        ModuleInfo'Index -> ModuleIndex
        ne(ModuleIndex, 0)

'action' QueryModuleOfId(ID -> ID)

    'rule' QueryModuleOfId(Id -> ModuleId):
        QuerySymbolId(Id -> Info)
        Info'Parent -> ModuleId

'condition' QuerySymbolId(ID -> SYMBOLINFO)

    'rule' QuerySymbolId(Id -> Info):
        QueryId(Id -> symbol(Info))

'condition' QueryModuleId(ID -> MODULEINFO)

    'rule' QueryModuleId(Id -> Info):
        QueryId(Id -> module(Info))
        
-- Defined in check.g
'action' QueryId(ID -> MEANING)
'condition' QuerySyntaxId(ID -> SYNTAXINFO)
'condition' QuerySyntaxMarkId(ID -> SYNTAXMARKINFO)

--------------------------------------------------------------------------------
