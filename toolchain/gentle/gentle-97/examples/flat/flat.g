-- a language with constant definitions and write statements
-- (identifiers may be used before being declared)

'root'
   seq(-> S)
   
   process_declarations(S)
   process_statements(S)

-- Abstract Syntax ------------------------------------------------------------

'type' SEQ
   list(ITEM, SEQ)
   nil

'type' ITEM
   declare(IDENT, INT)
   write(IDENT)

'type' IDENT

-- Concrete Grammar -----------------------------------------------------------

'nonterm' seq(-> SEQ)
   'rule' seq(-> list(H, T)): item(-> H) seq(-> T)
   'rule' seq(-> nil):

'nonterm' item(-> ITEM)
   'rule' item(-> declare(I, N)): "declare" Ident(-> I) "=" Number(-> N)
   'rule' item(-> write(I)): "write" Ident(-> I)

'token' Ident(-> IDENT)
'token' Number(-> INT)

-- Processing Declarations and Statements -------------------------------------

'action' process_declarations(SEQ)
   'rule' process_declarations(list(H, T)):
      process_declaration(H) process_declarations(T)
   'rule' process_declarations(nil)

'action' process_declaration(ITEM)
   'rule' process_declaration(declare(I, N)):
      [| HasMeaning(I -> X) print("multiple declaration") |]
      DefMeaning(I, N)
   'rule' process_declaration(write(I))

'action' process_statements(SEQ)
   'rule' process_statements(list(H, T)):
      process_statement(H) process_statements(T)
   'rule' process_statements(nil)

'action' process_statement(ITEM)
   'rule' process_statement(write(I)):
      (| HasMeaning(I -> N) print(N) || print("undeclared identifier") |)
   'rule' process_statement(declare(I,N))

-- Interface to "idents" Module -----------------------------------------------

'action' DefMeaning (IDENT, INT)
'condition' HasMeaning (IDENT -> INT)
