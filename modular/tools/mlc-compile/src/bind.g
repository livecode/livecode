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
'action' Bind(MODULE)

    'rule' Bind(Module:module(Position, Name, Imports, Definitions)):
        --DefineId(Name, module)

        -- Step 1: Ensure all id's referencing definitions point to the definition.
        --         and no duplicate definitions have been attempted.
        EnterScope
        -- Declare the predefined ids
        DeclarePredefinedIds
        -- Assign the defining id to all top-level names.
        Declare(Definitions)
        -- Resolve all references to id's.
        Apply(Definitions)
        LeaveScope
        
        -- Step 2: Ensure all definitions have their appropriate meaning
        Define(Definitions)
        
        --DumpBindings(Module)

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
    
    'rule' Declare(handler(Position, _, Name, _, _, _)):
        DeclareId(Name)
    
    'rule' Declare(foreignhandler(Position, _, Name, _, _)):
        DeclareId(Name)
    
    'rule' Declare(property(Position, _, Name)):
        DeclareId(Name)
    
    'rule' Declare(event(Position, _, Name)):
        DeclareId(Name)

    'rule' Declare(syntax(Position, _, Name, _, _, _)):
        DeclareId(Name)
    
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
'action' Define(DEFINITION)

    'rule' Define(sequence(Left, Right)):
        Define(Left)
        Define(Right)
        
    'rule' Define(type(Position, _, Name, Type)):
        DefineId(Name, type)
        [|
            where(Type -> handler(_, signature(Parameters, _)))
            DefineParameters(Parameters)
        |]
    
    'rule' Define(constant(Position, _, Name, _)):
        DefineId(Name, constant)
    
    'rule' Define(variable(Position, _, Name, _)):
        DefineId(Name, variable)
    
    'rule' Define(handler(Position, _, Name, Signature:signature(Parameters, _), _, _)):
        DefineId(Name, handler(Signature))
        DefineParameters(Parameters)
    
    'rule' Define(foreignhandler(Position, _, Name, Signature:signature(Parameters, _), _)):
        DefineId(Name, handler(Signature))
        DefineParameters(Parameters)

    'rule' Define(property(Position, _, Name)):
        DefineId(Name, property)

    'rule' Define(event(Position, _, Name)):
        DefineId(Name, event)
    
    'rule' Define(syntax(Position, _, Name, Class, Syntax, _)):
        Info::SYNTAXRULEINFO
        Info'Class <- Class
        Info'Syntax <- Syntax
        Info'Prefix <- undefined
        Info'Suffix <- undefined
        DefineId(Name, syntaxrule(Info))
    
    'rule' Define(nil):
        -- do nothing

'action' DefineParameters(PARAMETERLIST)

    'rule' DefineParameters(parameterlist(parameter(_, _, Name, _), Tail)):
        DefineId(Name, parameter)
        DefineParameters(Tail)
        
    'rule' DefineParameters(nil):
        -- do nothing

'action' DefineFields(FIELDLIST)

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
        -- do nothing

--------------------------------------------------------------------------------

'var' LastSyntaxMarkIndexVar : INT

'sweep' Apply(ANY)

    ----------

    'rule' Apply(DEFINITION'handler(_, _, _, signature(Parameters, Type), _, Body)):
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

    ----------

    'rule' Apply(TYPE'named(_, Name)):
        ApplyId(Name)
        
    'rule' Apply(TYPE'opaque(_, BaseType, Fields)):
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
        
    'rule' Apply(TYPE'handler(_, signature(Parameters, Type))):
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
        DefineId(Name, variable)
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

    'rule' Apply(STATEMENT'repeatforeach(_, Iterator, Slot, Container, Body)):
        ApplyId(Slot)
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

    'rule' Apply(DEFINITION'syntax(_, _, _, _, Syntax, Methods)):
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
            where(syntaxmark(MarkInfo) -> Meaning)
            DefineMeaning(Name, Meaning)
            LastSyntaxMarkIndexVar <- Index + 1
        |)
        Id'Meaning <- Meaning
        
    --'rule' ApplySyntaxMarkId(Id):
    --    Id'Position -> Position
    --    Id'Name -> Name
    --    Error_InvalidNameForSyntaxMarkVariable(Position, Name)

'action' DefineId(ID, MEANING)

    'rule' DefineId(Id, Meaning)
        Id'Meaning <- Meaning

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
        MakePredefinedId("output", syntaxoutputmark -> Id1)
        OutputSyntaxMarkIdVar <- Id1
        MakePredefinedId("input", syntaxinputmark -> Id2)
        InputSyntaxMarkIdVar <- Id2
        MakePredefinedId("context", syntaxcontextmark -> Id3)
        ContextSyntaxMarkIdVar <- Id3
        MakePredefinedId("iterator", syntaxiteratormark -> Id4)
        IteratorSyntaxMarkIdVar <- Id4
        MakePredefinedId("container", syntaxcontainermark -> Id5)
        ContainerSyntaxMarkIdVar <- Id5
        MakePredefinedId("Expression", syntaxexpressionrule -> Id6)
        ExpressionSyntaxRuleIdVar <- Id6
        MakePredefinedId("ExpressionList", syntaxexpressionlistrule -> Id7)
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

'action' MakePredefinedId(STRING, MEANING -> ID)

    'rule' MakePredefinedId(String, Meaning -> Id)
        Id::ID
        MakeNameLiteral(String -> Name)
        Id'Name <- Name
        Id'Meaning <- Meaning

--------------------------------------------------------------------------------

'sweep' DumpBindings(ANY)

    'rule' DumpBindings(MODULE'module(_, Name, Imports, Definitions)):
        DumpId("module", Name)
        DumpBindings(Imports)
        DumpBindings(Definitions)
        
    'rule' DumpBindings(IMPORT'import(_, Name)):
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
    'rule' DumpBindings(DEFINITION'handler(_, _, Name, Signature, Definitions, Body)):
        DumpId("handler", Name)
        DumpBindings(Signature)
        DumpBindings(Definitions)
        DumpBindings(Body)
    'rule' DumpBindings(DEFINITION'foreignhandler(_, _, Name, Signature, _)):
        DumpId("foreign handler", Name)
        DumpBindings(Signature)
    'rule' DumpBindings(DEFINITION'property(_, _, Name)):
        DumpId("property", Name)
    'rule' DumpBindings(DEFINITION'event(_, _, Name)):
        DumpId("event", Name)
    'rule' DumpBindings(DEFINITION'syntax(_, _, Name, _, Syntax, Methods)):
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
    'rule' DumpBindings(STATEMENT'repeatforeach(_, Iterator, Slot, Container, Body)):
        DumpId("repeat foreach slot", Slot)
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
