---
version: 8.0.0-dp-9
---
# LiveCode Builder Language

## Syntax

* The precedence of LCB syntax operators has been reworked to use a
  defined set of 23 named precedence classes.  This is to ensure
  predictable and consistent interactions when multiple LCB operators
  are used in a single expression.

  There only change that is likely to significantly affect
  pre-existing LCB code is the reduction in precedence of the logical
  `not` operator to below that of comparison operators.  This may
  allow removal of parentheses in the condition clauses of `if`
  statements.

# [15435] Fix precedence problem with array subscripts
