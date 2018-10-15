# LiveCode Builder Standard Library

## Assertions

* Checks for handler preconditions can now be included using the new
  `expect` statement.  For example, the following statement will throw
  an error if the value `pProbability` is out of the valid range for
  probabilities:
  
      expect that (pProbability >= 0 and pProbability <= 1) \
         because "Probabilities must be in the range [0,1]"

  The `that` keyword and the `because <Reason>` clauses are optional.
