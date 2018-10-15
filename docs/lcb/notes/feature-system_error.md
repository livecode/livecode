# LiveCode Builder Standard Library
## System error information

* Two new expressions have been added for accessing platform-specific
  system error status:

  * `the system error code` evaluates to the current numerical system
    error code

  * `the system error message` evaluates to a string describing the
    current system error

* The new `reset system error` statement clears the current system
  error.
  