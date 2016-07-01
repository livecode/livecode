---
version: 8.1.0-dp-2
---
# Improved return command
The 'return' command has had two new forms added:

    return <value> for value
    return <value> for error

When running in a command handler, the 'return ... for value' form will cause
execution of the handler to halt, and control to return to the calling handler.
At this point the 'it' variable in the calling handler will be set to 'value'
and 'the result' will be set to empty. In contrast, the 'return ... for error'
form will cause the 'it' variable in the calling handler to be set to empty and
'the result' to be set to 'value'.

When running in a function handler, the 'return ... for value' form will cause
execution of the handler to halt, and control to return to the calling handler.
At this point the return value of the function call will be 'value', and
'the result' will be set to empty. In contrast, the 'return ... for error' form
will cause the return value of the function call to be empty, and 'the result'
will be set to 'value'.

These forms of return are designed to be used by script library functions to
allow them to have the same ability as built-in engine functions and commands -
namely the ability to return a value (in it for commands, or return value for
functions) *or* return a status (in the result).
