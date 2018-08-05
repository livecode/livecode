# Implement filter where clause

A new clause has been added to the filter command to filter where an expression
evaluates to true. For example:

    put "foo,bar,baz" into tList
    filter items of tList where each begins with "b"
    -- tList contains "bar,baz"