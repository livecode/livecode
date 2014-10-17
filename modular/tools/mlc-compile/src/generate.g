'module' generate

'use'
    types
    support

'export'
    Generate

--------------------------------------------------------------------------------

'action' Generate(MODULE)

    'rule' Generate(module(_, Id, Imports, Definitions)):
        QueryModuleId(Id -> Info)
        Id'Name -> ModuleName
        EmitBeginModule(ModuleName -> ModuleIndex)
        Info'Index <- ModuleIndex

        -- Emit all imported declarations and dependent modules.
        GenerateImportedDeclarations(Definitions)

        -- Declare indices for all definitions in this module (so mutally referential
        -- definitions work).
        GenerateDeclarations(Definitions)
        
        -- Generate all definitions from this module.
        GenerateDefinitions(Definitions)
        
        EmitEndModule()

----------

-- Iterate over the tree, generating declarations for all used imported
-- definitions.
'sweep' GenerateImportedDeclarations(ANY)

    'rule' GenerateImportedDeclarations(TYPE'named(Position, Name)):
        QuerySymbolId(Name -> Info)
        Info'Type -> Type
        GenerateImportedDeclarations(Type)
        GenerateImportedDeclaration(Name)
        
    'rule' GenerateImportedDeclarations(STATEMENT'call(_, Handler, Arguments)):
        GenerateImportedDeclaration(Handler)
        GenerateImportedDeclarations(Arguments)
        
    'rule' GenerateImportedDeclarations(EXPRESSION'slot(_, Name)):
        GenerateImportedDeclaration(Name)

    'rule' GenerateImportedDeclarations(EXPRESSION'call(_, Handler, Arguments)):
        GenerateImportedDeclaration(Handler)

'action' GenerateImportedDeclaration(ID)

    'rule' GenerateImportedDeclaration(Id):
        IsUngeneratedExternalId(Id)
        QueryModuleOfId(Id -> ModuleId)
        GenerateModuleDependency(ModuleId)
        QuerySymbolId(Id -> SymbolInfo)
        Id'Name -> SymbolName
        SymbolInfo'Kind -> SymbolKind
        SymbolInfo'Type -> SymbolType
        GenerateImportedType(SymbolType -> SymbolTypeIndex)
        (|
            where(SymbolKind -> type)
            EmitImportedType(SymbolName, SymbolTypeIndex -> SymbolIndex)
        ||
            where(SymbolKind -> constant)
            EmitImportedConstant(SymbolName, SymbolTypeIndex -> SymbolIndex)
        ||
            where(SymbolKind -> variable)
            EmitImportedVariable(SymbolName, SymbolTypeIndex -> SymbolIndex)
        ||
            where(SymbolKind -> handler)
            EmitImportedHandler(SymbolName, SymbolTypeIndex -> SymbolIndex)
        |)
        SymbolInfo'Index <- SymbolIndex
        
    'rule' GenerateImportedDeclaration(Id):
        --

----

'action' GenerateModuleDependency(ID)
    
    'rule' GenerateModuleDependency(Id):
        QueryModuleId(Id -> ModuleInfo)
        ModuleInfo'Index -> ModuleIndex
        [|
            eq(ModuleIndex, -1)
            Id'Name -> ModuleName
            EmitModuleDependency(ModuleName -> NewModuleIndex)
            ModuleInfo'Index <- NewModuleIndex
        |]

----

'action' GenerateDeclarations(DEFINITION)

    'rule' GenerateDeclarations(sequence(Left, Right)):
        GenerateDeclarations(Left)
        GenerateDeclarations(Right)
        
    'rule' GenerateDeclarations(type(_, _, Name, _)):
        GenerateDeclaration(Name)
    
    'rule' GenerateDeclarations(constant(_, _, Name, _)):
        GenerateDeclaration(Name)
    
    'rule' GenerateDeclarations(variable(_, _, Name, _)):
        GenerateDeclaration(Name)
    
    'rule' GenerateDeclarations(handler(_, _, Name, _, _, _)):
        GenerateDeclaration(Name)
    
    'rule' GenerateDeclarations(foreignhandler(_, _, Name, _, _)):
        GenerateDeclaration(Name)
    
    'rule' GenerateDeclarations(_):
        -- nothing

'action' GenerateDeclaration(ID)

    'rule' GenerateDeclaration(Id):
        QuerySymbolId(Id -> Info)
        EmitDeclaration(-> Index)
        Info'Index <- Index

----

'action' GenerateDefinitions(DEFINITION)

    'rule' GenerateDefinitions(sequence(Left, Right)):
        GenerateDefinitions(Left)
        GenerateDefinitions(Right)
        
    'rule' GenerateDefinitions(type(Position, Access, Id, Type)):
        GenerateType(Type -> TypeIndex)
        
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        --GenerateAccess(Access)
        EmitTypeDefinition(DefIndex, Position, Name, TypeIndex)
        
    'rule' GenerateDefinitions(constant(Position, Access, Id, Value)):
        QuerySymbolId(Id -> Info)
        -- Do something
        
    'rule' GenerateDefinitions(variable(Position, Access, Id, Type)):
        GenerateType(Type -> TypeIndex)
        
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        --GenerateAccess(Access)
        EmitVariableDefinition(DefIndex, Position, Name, TypeIndex)

    'rule' GenerateDefinitions(handler(Position, Access, Id, Signature, _, Body)):
        GenerateType(handler(Position, Signature) -> TypeIndex)
        
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        --GenerateAccess(Access)
        EmitBeginHandlerDefinition(DefIndex, Position, Name, TypeIndex)
        GenerateBody(Body)
        EmitEndHandlerDefinition()

    'rule' GenerateDefinitions(foreignhandler(Position, Access, Id, Signature, Binding)):
        GenerateType(handler(Position, Signature) -> TypeIndex)
        
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        --GenerateAccess(Access)
        EmitForeignHandlerDefinition(DefIndex, Position, Name, TypeIndex, Binding)

----

'action' GenerateBody(STATEMENT)

    'rule' GenerateBody(Statement):
        --

--------------------------------------------------------------------------------

'action' GenerateImportedType(TYPE -> INT)

    'rule' GenerateImportedType(opaque(_, Base, Fields) -> Index):
        GenerateImportedType(Base -> BaseIndex)
        EmitImportedOpaqueType(BaseIndex -> Index)


'action' GenerateType(TYPE -> INT)

    'rule' GenerateType(named(_, Id) -> Index):
        QuerySymbolId(Id -> Info)
        Info'Index -> OtherTypeIndex
        EmitNamedType(OtherTypeIndex -> Index)
    
    'rule' GenerateType(opaque(_, Base, Fields) -> ):

    'rule' GenerateType(record(_, Base, Fields) -> ):
        GenerateTypeFields(Fields)
    
    'rule' GenerateType(enum(_, Base, Fields) -> ):

    'rule' GenerateType(handler(_, Signature) -> ):

    'rule' GenerateType(pointer(_) -> Index):
        EmitPointerType(-> Index)
    'rule' GenerateType(bool(_) -> Index):
        EmitBoolType(-> Index)
    'rule' GenerateType(int(_) -> Index):
        EmitIntType(-> Index)
    'rule' GenerateType(uint(_) -> Index):
        EmitUIntType(-> Index)
    'rule' GenerateType(float(_) -> Index):
        EmitFloatType(-> Index)
    'rule' GenerateType(double(_) -> Index):
        EmitDoubleType(-> Index)

    'rule' GenerateType(any(_) -> Index):
        EmitAnyType(-> Index)
    'rule' GenerateType(boolean(_) -> Index):
        EmitBooleanType(-> Index)
    'rule' GenerateType(integer(_) -> Index):
        EmitIntegerType(-> Index)
    'rule' GenerateType(real(_) -> Index):
        EmitRealType(-> Index)
    'rule' GenerateType(number(_) -> Index):
        EmitNumberType(-> Index)
    'rule' GenerateType(string(_) -> Index):
        EmitStringType(-> Index)
    'rule' GenerateType(data(_) -> Index):
        EmitDataType(-> Index)
    'rule' GenerateType(array(_) -> Index):
        EmitArrayType(-> Index)
    'rule' GenerateType(list(_) -> Index):
        EmitListType(-> Index)
    'rule' GenerateType(undefined(_) -> Index):
        EmitUndefinedType(-> Index)

--------------------------------------------------------------------------------

'condition' IsUngeneratedExternalId(ID)

    'rule' IsUngeneratedExternalId(Id):
        -- Ungenerated if index is -1
        QuerySymbolId(Id -> Info)
        Info'Index -> Index
        eq(Index, -1)

        -- Extenal if module index is not 0
        Info'Module -> ModuleId
        QueryModuleId(ModuleId -> ModuleInfo)
        ModuleInfo'Index -> ModuleIndex
        ne(ModuleIndex, 0)

'condition' IsExternalId(ID)

    'rule' IsExternalId(Id):
        -- Extenal if module index is not 0
        QuerySymbolId(Id -> Info)
        Info'Module -> ModuleId
        QueryModuleId(ModuleId -> ModuleInfo)
        ModuleInfo'Index -> ModuleIndex
        ne(ModuleIndex, 0)

'action' QueryModuleOfId(ID -> ID)

    'rule' QueryModuleOfId(Id -> ModuleId):
        QuerySymbolId(Id -> Info)
        Info'Module -> ModuleId

'action' QuerySymbolId(ID -> SYMBOLINFO)

    'rule' QuerySymbolId(Id -> Info):
        QueryId(Id -> symbol(Info))

'action' QueryModuleId(ID -> MODULEINFO)

    'rule' QueryModuleId(Id -> Info):
        QueryId(Id -> module(Info))
        
'action' QueryId(ID -> MEANING)
