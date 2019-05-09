# The `is strictly` and `is not strictly` operators have been enhanced to allow comparison to another value

Use `<value> is [not] strictly <otherValue>` to determine if two values have
strict value equivalence. Previously `is strictly` could be used to only test if
the expression evaluated to either a particular type or `nothing`. This
enhancement allows `is strictly` to also compare exact value equivalence.

When comparing to another value the values are considered to be equal if they
are the same type and have exactly the same value:

* strings and binary strings must have the same byte values
* unicode strings are not normalized prior to the comparison
* numeric strings are compared as strings

For example:

The following evaluates to `true` because strict type of a numeric literal is
string and the string values differ

    1 is not strictly 1.0

The following evaluates to `true` because the strict type resulting from a
numeric expression is a number and the number is the same

    1 + 0 is strictly 1.0 + 0