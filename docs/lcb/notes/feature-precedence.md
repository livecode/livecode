# LiveCode Builder Language

## Operator Precedence

The precedence of LCB syntax operators has been reworked to use a
defined set of 23 named precedence classes.  This is to ensure
predictable and consistent interactions when multiple LCB operators
are used in a single expression.

There are two changes which are likely to significantly affect
pre-existing LCB code:

* "property", "subscript chunk", "function chunk" and "constructor
  chunk" syntax classes now always have higher precedence than
  arithmetic operators.  In particular, this change is likely to
  require additional parentheses `(...)` around arithmetic operations
  when using the LCB canvas API.

* the precedence of the logical `not` operator has been reduced to
  below that of comparison operators.  This may allow removal of
  parentheses in the condition clauses of `if` statements.
