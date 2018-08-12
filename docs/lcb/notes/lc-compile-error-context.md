# LiveCode Builder Tools
## lc-compile
### Messages

* Errors, warnings and informational messages now display the affected
  line of code and visually indicate the position where the problem
  was found.  For example, the output might look like:

      foo.lcb:2:26: error: Identifier 'InvalidExpression' not declared
        constant kBadConstant is InvalidExpression
                                 ^

# [18463] Show correct error position when source line includes tabs
