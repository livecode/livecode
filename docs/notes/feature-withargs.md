# Send and call with parameters

The `send` and `call` commands have been enhanced to use the `with parameterList`
syntax similar to the `dispatch` command. If parameters are included in
the parameter list then the message string is evaluated directly as a
handler name. As a result an execution error will throw if parameters
are specified in both the parameter list and the message string.

The new syntax is in the following forms:

    send <message> [ to <object> [in <time> [{seconds | ticks | milliseconds}] ] ] [ with param1 [... ,paramN]]
    call <message> [ of <object> [ with param1 [... ,paramN]]
