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

    'rule' Bind(module(Position, Name, Imports, Definitions)):
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

--------------------------------------------------------------------------------

-- The 'Define' phase associates meanings with the definining ids.
--
'action' Define(DEFINITION)

    'rule' Define(sequence(Left, Right)):
        Define(Left)
        Define(Right)
        
    'rule' Define(type(Position, _, Name, _)):
        DefineId(Name, type)
    
    'rule' Define(constant(Position, _, Name, _)):
        DefineId(Name, constant)
    
    'rule' Define(variable(Position, _, Name, _)):
        DefineId(Name, variable)
    
    'rule' Define(handler(Position, _, Name, signature(Parameters, _), _, _)):
        DefineId(Name, handler)
        DefineParameters(Parameters)
    
    'rule' Define(foreignhandler(Position, _, Name, _, _)):
        DefineId(Name, handler)

    'rule' Define(property(Position, _, Name)):
        DefineId(Name, property)

    'rule' Define(event(Position, _, Name)):
        DefineId(Name, event)
    
    'rule' Define(syntax(Position, _, Name, Class, _, _)):
        DefineId(Name, syntaxrule(Class))
    
    'rule' Define(nil):
        -- do nothing

'action' DefineParameters(PARAMETERLIST)

    'rule' DefineParameters(parameterlist(parameter(_, _, Name, _), Tail)):
        DefineId(Name, parameter)
        DefineParameters(Tail)
        
    'rule' DefineParameters(nil):
        -- do nothing

--------------------------------------------------------------------------------

'var' LastSyntaxMarkIndexVar : INT

'sweep' Apply(ANY)

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

    'rule' Apply(TYPE'named(_, Name)):
        ApplyId(Name)
        
    'rule' Apply(STATEMENT'call(_, Handler, Arguments)):
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
        
    'rule' Apply(SYNTAXCONSTANT'name(_, Value)):
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

'action' ApplyLocalId(ID)

    'rule' ApplyLocalId(Id):
        Id'Name -> Name
        HasLocalMeaning(Name -> Meaning)
        Id'Meaning <- Meaning

    'rule' ApplyLocalId(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_IdentifierNotDeclared(Position, Name)

'action' ApplySyntaxMarkId(ID)

    'rule' ApplySyntaxMarkId(Id):
        Id'Name -> Name
        (|
            HasLocalMeaning(Name -> Meaning)
            where(Meaning -> syntaxmark(_))
        ||
            LastSyntaxMarkIndexVar -> Index
            where(syntaxmark(Index) -> Meaning)
            DefineMeaning(Name, Meaning)
            LastSyntaxMarkIndexVar <- Index + 1
        |)
        Id'Meaning <- Meaning
        
    'rule' ApplySyntaxMarkId(Id):
        Id'Position -> Position
        Id'Name -> Name
        Error_InvalidNameForSyntaxMarkVariable(Position, Name)

'action' DefineId(ID, MEANING)

    'rule' DefineId(Id, Meaning)
        Id'Meaning <- Meaning

--------------------------------------------------------------------------------

'var' OutputSyntaxMarkIdVar : ID
'var' InputSyntaxMarkIdVar : ID
'var' ContextSyntaxMarkIdVar : ID

'var' ExpressionSyntaxRuleIdVar : ID

'action' InitializeBind

    'rule' InitializeBind:
        MakePredefinedId("output", syntaxoutputmark -> Id1)
        OutputSyntaxMarkIdVar <- Id1
        MakePredefinedId("input", syntaxinputmark -> Id2)
        InputSyntaxMarkIdVar <- Id2
        MakePredefinedId("context", syntaxcontextmark -> Id3)
        ContextSyntaxMarkIdVar <- Id3
        MakePredefinedId("Expression", syntaxrule(expression) -> Id4)
        ExpressionSyntaxRuleIdVar <- Id4

'action' DeclarePredefinedIds

    'rule' DeclarePredefinedIds:
        ExpressionSyntaxRuleIdVar -> Id1
        DeclareId(Id1)

'action' DeclarePredefinedMarkVariables

    'rule' DeclarePredefinedMarkVariables:
        OutputSyntaxMarkIdVar -> Id1
        DeclareId(Id1)
        InputSyntaxMarkIdVar -> Id2
        DeclareId(Id2)
        ContextSyntaxMarkIdVar -> Id3
        DeclareId(Id3)

'action' MakePredefinedId(STRING, MEANING -> ID)

    'rule' MakePredefinedId(String, Meaning -> Id)
        Id::ID
        MakeNameLiteral(String -> Name)
        Id'Name <- Name
        Id'Meaning <- Meaning

--------------------------------------------------------------------------------
