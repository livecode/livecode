/******************************************************************************/
/*                                                                            */
/*   A COMPILER  WITH A BACKEND FOR THE MC68K MICROPROCESSOR                  */
/*                                                                            */
/******************************************************************************/

'ROOT'
   Program (-> P)
   Trafo (P -> C)
   Encode (C)


/******************************************************************************/
/*                                                                            */
/*   PARSER                                                                   */
/*                                                                            */
/******************************************************************************/

/* Abstract Syntax */

'TYPE' PROG
   prog (DECLLIST, STMT)

'TYPE' DECL
   dcl (IDENT, TP, POS)

'TYPE' DECLLIST
   decllist (DECL, DECLLIST)
   nil

'TYPE' TP
   integer
   array (INT, INT, TP)

'TYPE' STMT
   assign (DESIG, EXPR, POS)
   seq (STMT, STMT)

'TYPE' DESIG
   id (IDENT, POS)
   subscr (DESIG, EXPR, POS)

'TYPE' EXPR
   binary (OP, EXPR, EXPR)
   int (INT)
   desig (DESIG, POS)

'TYPE' OP
   plus
   mult

'TYPE' IDENT


/* Concrete Syntax */

'NONTERM' Program (-> PROG)

   Program (-> prog(Ds, S)) :
      "VAR" DeclList (-> Ds)
      "BEGIN" StatSeq (-> S) "END" "." .

'NONTERM' Type (-> TP)

   Type (-> integer) :
      "INTEGER" .

   Type (-> array(Lwb, Upb, T)) :
      "ARRAY" "["  Number (-> Lwb) ".." Number (-> Upb) "]"
      "OF" Type (-> T) .

'NONTERM' Decl (-> DECL)

   Decl (-> dcl(I, T, Pos)) :
      Ident (-> I) ":" @(-> Pos) Type (-> T).

'NONTERM' Designator (-> DESIG)

   Designator (-> id(I, Pos)) :
      Ident (-> I) @(-> Pos).

   Designator (-> subscr(V, E, Pos)) :
      Designator (-> V) "[" @(-> Pos) Expr (-> E) "]" .

'NONTERM' Expr (-> EXPR)

   Expr (-> binary(plus, X, Y)) :
      Expr (-> X) "+" Expr2 (-> Y) .

   Expr (-> X) :
      Expr2 (-> X) .

'NONTERM' Expr2 (-> EXPR)

   Expr2 (-> binary(mult, X, Y)) :
      Expr2 (-> X) "*" Expr3 (-> Y) .

   Expr2 (-> X) :
      Expr3 (-> X) .

'NONTERM' Expr3 (-> EXPR)

   Expr3 (-> X) :
      "(" Expr (-> X) ")" .

   Expr3 (-> desig(V, Pos)) :
      Designator (-> V) @(-> Pos) .

   Expr3 (-> int(N)) :
      Number (-> N) .

'NONTERM' StatSeq (-> STMT)

   StatSeq (-> S) :
      Stat (-> S) .

   StatSeq (-> seq(S1, S2)) :
      StatSeq (-> S1) ";" Stat (-> S2) .

'NONTERM' Stat (-> STMT)

   Stat (-> assign(V, E, Pos)) :
      Designator (-> V) ":=" @(-> Pos) Expr (-> E) .

'NONTERM' DeclList (-> DECLLIST)

   DeclList (-> decllist(D, Ds)) :
      Decl (-> D) ";" DeclList (-> Ds) .

   DeclList (-> decllist(D, nil)) :
      Decl (-> D) .

'TOKEN' Ident (-> IDENT)
'TOKEN' Number (-> INT)

/******************************************************************************/
/*                                                                            */
/*   TRAFO                                                                    */
/*                                                                            */
/******************************************************************************/

/* Low Level Intermediate Representation */

'TYPE' COMMAND
   assign (ARG, ARG)
   codeseq (COMMAND, COMMAND)

'TYPE' ARG
   intplus (ARG, ARG)
   addrplus (ARG, ARG)
   intminus (ARG, ARG)
   intmult (ARG, ARG)
   intconst (INT)
   blockbase
   cont (ARG)

/* Symbol Table Entries */

'TYPE' DESCR
   varobj (INT, TP)


/* Transformation */

'ACTION' Trafo (PROG -> COMMAND)

   Trafo (prog(Locals, Body) -> Code) :
      DeclareList (Locals, 0 -> Offset)
      Statement (Body -> Code) .

'ACTION' Statement (STMT -> COMMAND)

   Statement (assign(V, E, Pos) -> assign(X1, X2)) :
      Variable (V -> TV, X1)
      Expression (E -> X2)
      CheckInt(TV, Pos) .

   Statement (seq(S1, S2) -> codeseq(C1,C2)) :
      Statement (S1 -> C1)
      Statement (S2 -> C2) .

'ACTION' Expression (EXPR -> ARG)

   Expression (binary(plus, X, Y) -> intplus(CX, CY)) :
      Expression (X -> CX)
      Expression (Y -> CY) .

   Expression (binary(mult, X, Y) -> intmult(CX, CY)) :
      Expression (X -> CX)
      Expression (Y -> CY) .

   Expression (int(N) -> intconst(N)) : .

   Expression (desig(V, Pos) -> cont(Arg)) :
      Variable (V -> T, Arg)
      CheckInt (T, Pos) .

'ACTION' Variable (DESIG -> TP, ARG)

   Variable (id(Ident, Pos) -> Type, addrplus(blockbase, intconst(0-Offset))) :
      Apply (Ident, Pos -> varobj(Offset, Type)) .

   Variable ( subscr(V, E, Pos) -> T,
      addrplus(CV, intmult(intminus(CE, intconst(Lwb)), intconst(Size)))) :

      Variable (V -> TV, CV)
      Expression (E -> CE)
      CheckArrayType (TV, Pos -> Lwb, Upb, T)
      TypeSize (T -> Size) .

'ACTION' TypeSize (TP -> INT)

   TypeSize (integer -> 4) : .

   TypeSize (array(Lwb, Upb, ElemType) -> ((Upb-Lwb)+1)*ElemSize) :
      TypeSize (ElemType -> ElemSize) .

'ACTION' CheckInt (TP, POS)

   CheckInt (integer, Pos) : .

   CheckInt (T, Pos) :
      Error ("Integer expected", Pos) .

'ACTION' CheckArrayType (TP, POS -> INT, INT, TP)

   CheckArrayType (array(Lwb, Upb, ElemType), Pos -> Lwb, Upb, ElemType) : .

   CheckArrayType (T, Pos -> 0, 0, integer) :
      Error ("Array expected", Pos) .

'ACTION' Error (STRING, POS)

'ACTION' ErrorI (STRING, IDENT, STRING, POS)

/* Declarations */

'ACTION' DeclareList (DECLLIST, INT -> INT)

   DeclareList (decllist(D, Ds), Offset1 -> Offset3) :
      Declare (D, Offset1 -> Offset2)
      DeclareList (Ds, Offset2 -> Offset3) .

   DeclareList (nil, Offset -> Offset) : .

'ACTION' Declare (DECL, INT -> INT)

   Declare (dcl(Ident, D, Pos), Offset -> Offset) :
      HasMeaning (Ident -> Descr)
      ErrorI ("Identifier '", Ident, "' already declared", Pos) .

   Declare (dcl(Ident, T, Pos), Offset -> Offset+Size) :
      TypeSize (T -> Size)
      DefMeaning (Ident, varobj(Offset, T)) .

'ACTION' Apply (IDENT, POS -> DESCR)

   Apply (Ident, Pos -> Descr) :
      HasMeaning (Ident -> Descr) .

   Apply (Ident, Pos -> varobj(0, integer)) :
      ErrorI ("Identifier '", Ident, "' not declared", Pos) .

'ACTION' DefMeaning (IDENT, DESCR)

'CONDITION' HasMeaning (IDENT -> DESCR)

/******************************************************************************/
/*                                                                            */
/*   CODE GENERATOR                                                           */
/*                                                                            */
/******************************************************************************/

/* Addressing Modes */

'TYPE' OPND
   d (INT)
   a (INT)
   ad (Areg: INT, Displ: INT)
   ax (AReg: INT, Displ: INT, IndexReg: INT, Scale: INT)
   int (INT)


'ACTION' Encode (COMMAND)

   Encode (C):
      OpenOutput ("out.s")
      CODE(C)
      CloseOutput.

'ACTION' CODE (COMMAND)

   CODE (assign(X1, X2)) :
      EA (X1 -> Y1)
      SRC (X2 -> Y2)
      INSTR ("movl", Y2, Y1)
      Dispose (Y1)
      Dispose (Y2) .

   CODE (codeseq(C1, C2)):
      CODE(C1)
      CODE(C2).

/* Data Register */

'CHOICE' DR (ARG -> INT)

   DR (cont(Arg) -> Y2) :
      EA (Arg -> Y1)
      FetchDReg (-> Y2)
      INSTR ("movl", Y1, d(Y2))
      Dispose (Y1)
      $ 4.

   DR (intplus(X1, X2) -> Y2) :
      SRC (X1 -> Y1)
      DR (X2 -> Y2)
      INSTR ("addl", Y1, d(Y2))
      Dispose (Y1)
      $ 6.

   DR (intplus(X1, X2) -> Y1) :
      SRC (X2 -> Y2)
      DR (X1 -> Y1)
      INSTR ("addl", Y2, d(Y1))
      Dispose (Y2)
      $ 6.

   DR (intminus(X1, X2) -> Y1) :
      DR (X1 -> Y1)
      SRC (X2 -> Y2)
      INSTR ("subl", Y2, d(Y1))
      Dispose (Y2)
      $ 6.

   DR (intmult(X1, X2) -> Y2) :
      SRC (X1 -> Y1)
      DR (X2 -> Y2)
      INSTR ("mulsl", Y1, d(Y2))
      Dispose (Y1)
      $ 42.

   DR (intmult(X1, X2) -> Y1) :
      SRC (X2 -> Y2)
      DR (X1 -> Y1)
      INSTR ("mulsl", Y2, d(Y1))
      Dispose (Y2)
      $ 42.

   DR (intconst(N) -> Opnd) :
      FetchDReg (-> Opnd)
      INSTR ("movl", int(N), d(Opnd))
      $ 12.

/* Address Register */

'CHOICE' AR (ARG -> INT)

   AR (addrplus(X1, X2) -> Y1) :
      Mutable(X1)
      AR (X1 -> Y1)
      SRC (X2 -> Y2)
      INSTR ("adda", Y2, a(Y1))
      Dispose (Y2)
      $ 6.

   AR (addrplus(blockbase, X2) -> AR) :
      SRC (X2 -> Y2)
      FetchAReg (-> AR)
      INSTR ("lea", a(6), a(AR))
      INSTR ("adda", Y2, a(AR))
      Dispose (Y2)
      $ 6.

   AR (X -> AR) :
      EA (X -> Y)
      FetchAReg (-> AR)
      INSTR ("lea", Y, a(AR))
      Dispose (Y)
      $ 4.

/* Effective Address */

'CHOICE' EA (ARG -> OPND)

   EA (X -> a(R)):
      AR (X -> R)
      $ 0.

   EA (addrplus(addrplus(A, intconst(Displ)), intmult(I, intconst(Size)))
       -> ax(AR, Displ, DR, Size)) :
      eq (Size, 4)
      d8 (Displ)
      AR (A -> AR)
      DR (I -> DR)
      $ 14.

   EA (addrplus(A, intmult(I, intconst(Size))) -> ax(AR, 0, DR, Size)) :
      eq (Size, 4)
      AR (A -> AR)
      DR (I -> DR)
      $ 14.

   EA (addrplus(addrplus(A, intconst(Displ)), I) -> ax(AR, Displ, DR, 1)) :
      d8 (Displ)
      AR (A -> AR)
      DR (I -> DR)
      $ 14.

   EA (addrplus(Base, intconst(Offset)) -> ad(N, Offset)) :
      d16(Offset)
      AR (Base -> N)
      $ 12.

   EA (addrplus(blockbase, intconst(Offset)) -> ad(6, Offset)) :
      d16(Offset)
      $ 12.
   
/* Source Operand */

'CHOICE' SRC (ARG -> OPND)

   SRC (Arg -> d(Opnd)) :
      DR (Arg -> Opnd)
      $ 0.

   SRC (cont(Arg) -> Opnd) :
      EA (Arg -> Opnd)
      $ 0.

   SRC (intconst(N) -> int(N)) :
      $ 8.

/* Conditions */

'CONDITION' Mutable (ARG)

   Mutable (X) : ne (X, blockbase) .

'CONDITION' d8(INT)

   d8 (N): ge(N, -128) le(N, +127) .

'CONDITION' d16(INT)

   d16 (N): ge(N, -32768) le(N, +32767) .

/* Register Management */

'ACTION' Dispose (OPND)

   Dispose (a(N)) : ReleaseAReg (N) .
   Dispose (d(N)) : ReleaseDReg (N) .
   Dispose (ad(N, Offset)) : ReleaseAReg (N) .
   Dispose (ax(AR, N, DR, S)) : ReleaseAReg (AR) ReleaseDReg (DR) .
   Dispose (int(Int)) : .

'ACTION' FetchAReg (-> INT)
'ACTION' FetchDReg (-> INT)
'ACTION' ReleaseDReg (INT)
'ACTION' ReleaseAReg (INT)

/* Assembler Representation */

'ACTION' INSTR (STRING, OPND, OPND) .

   INSTR (Name, X, Y) :
      Put (Name) Put (" ") Opnd (X) Put (",") Opnd (Y) Nl .

'ACTION' Opnd (OPND)

   Opnd (d(N)) :
      Put ("d") PutI (N) .
   Opnd (a(N)) :
      Put ("a") PutI (N) .
   Opnd (ad(R, Offset)) :
      Put ("a") PutI (R) Put ("@(") PutI (Offset) Put (")") .
   Opnd (ax(AR, N, DR, S)) :
      Put ("a") PutI (AR) Put ("@(") PutI (N) Put (",d") PutI (DR)
      Put (":l:") PutI (S) Put (")") .
   Opnd (int(N)) :
      Put ("#") PutI (N) .

'ACTION' OpenOutput (STRING)
'ACTION' CloseOutput
'ACTION' Put (STRING)
'ACTION' Nl
'ACTION' PutI (INT)
