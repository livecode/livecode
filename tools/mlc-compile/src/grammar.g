'module' grammar

'use'
    types

--------------------------------------------------------------------------------

'root'
    Parse(-> Module)

--------------------------------------------------------------------------------

'nonterm' Parse(-> MODULE)

    'rule' Parse(-> Module):
        Module(-> Module)

--------------------------------------------------------------------------------
-- Module Syntax
--------------------------------------------------------------------------------

'nonterm' Module(-> MODULE)

    'rule' Module(-> module(Position, Name, Imports, Definitions)):
        "module" @(-> Position) Identifier(-> Name) SEPARATOR
        Imports(-> Imports)
        Definitions(-> Definitions)

--------------------------------------------------------------------------------
-- Import Syntax
--------------------------------------------------------------------------------

'nonterm' Imports(-> IMPORT)

    'rule' Imports(-> sequence(Head, Tail)):
        Import(-> Head) SEPARATOR
        Imports(-> Tail)
        
    'rule' Imports(-> nil):
        -- empty
        
'nonterm' Import(-> IMPORT)

    'rule' Import(-> ImportList):
        "use" @(-> Position) IdentifierList(-> Identifiers)
        ExpandImports(Position, Identifiers -> ImportList)

'action' ExpandImports(POS, IDLIST -> IMPORT)

    'rule' ExpandImports(Position, idlist(Id, nil) -> import(Position, Id)):
        -- empty
        
    'rule' ExpandImports(Position, idlist(Id, Tail) -> sequence(import(Position, Id), ExpandedTail)):
        ExpandImports(Position, Tail -> ExpandedTail)

--------------------------------------------------------------------------------
-- Definitions Syntax
--------------------------------------------------------------------------------

'nonterm' Definitions(-> DEFINITION)

    'rule' Definitions(-> nil):
        -- empty

--------------------------------------------------------------------------------
-- Identifier Syntax
--------------------------------------------------------------------------------

'nonterm' Identifier(-> ID)

    'rule' Identifier(-> Id):
        NAME_LITERAL(-> Identifier) @(-> Position)
        Id::ID
        Id'Position <- Position
        Id'Name <- Identifier

'nonterm' IdentifierList(-> IDLIST)

    'rule' IdentifierList(-> idlist(Head, Tail)):
        Identifier(-> Head) "," IdentifierList(-> Tail)
        
    'rule' IdentifierList(-> idlist(Id, nil)):
        Identifier(-> Id)

--------------------------------------------------------------------------------
-- Tokens
--------------------------------------------------------------------------------

'token' NAME_LITERAL (-> NAME)
'token' INTEGER_LITERAL (-> INT)
'token' DOUBLE_LITERAL (-> DOUBLE)
'token' STRING_LITERAL (-> STRING)

'token' SEPARATOR

