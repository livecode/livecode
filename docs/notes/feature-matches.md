# Matches Operator

A new operator `matches` has been implemented to allow checking a value against
a regular expression or wildcard pattern. For example:

    if "foobar" matches wildcard pattern "*b*" then
        --
    end if

    if "foobar" matches regex pattern "^f.*r$" then
        --
    end if
