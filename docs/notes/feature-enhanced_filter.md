# Enhanced 'filter' command
The **filter** command was enhanced to support:
- filtering items in addition to lines
- matching a regular expression in addition to wildcard patterns
- storing the output in another container using an optional 'into' clause
- as well as the adoption of 'convert' semantics.

The new syntax is:
`filter [ { lines | items } of ] <source_container_or_expr> { with | without | [ not ] matching } [ { wildcard | regex } [ pattern ] ] <pattern> [ into <target_container> ]`

Note that the implementation is backward compatible as:
- the default chunk type is 'lines'
- and the default pattern type is 'wildcard'.

## Filtering items
In previous versions, the 'filter' command only supported the filtering of lines. Now you can also filter items in a source.
For example, for a variable 'theList' containing a comma-separated list of strings:
`foo,bar,baz,qux,quux,corge,grault,garply,waldo,fred,plugh,xyzzy,thud`
The script line:
`filter items of theList with "b*"`
Would result in the variable 'theList' containing:
`bar,baz`

## Matching regular expressions
In previous versions, the 'filter' command only supported matching a 'wildcard' pattern. Now you can use regular expression pattern matching as well.
For example, for a variable 'theList' containing a comma-separated list of strings:
`foo,bar,baz,qux,quux,corge,grault,garply,waldo,fred,plugh,xyzzy,thud`
The script line:
`filter items of theList with regex pattern "b.*"`
Would result in the variable 'theList' containing:
`bar,baz`
Note that the keyword 'pattern' is optional syntactic sugar to clarify the intent of the script.

## Storing the output in another container
In previous versions, the 'filter' command always replaced the contents of the original container. Now you can opt to store the output in a separate container, allowing you to easily retain both the original and filtered data in separate variables.
For example, for a variable 'theList' containing a comma-separated list of strings:
`foo,bar,baz,qux,quux,corge,grault,garply,waldo,fred,plugh,xyzzy,thud`
The script line:
`filter items of theList with "b*" into theFilteredList`
Would result in the variable 'theFilteredList' containing:
`bar,baz`
While the variable 'theList' still contains the original unfiltered data.

## Adoption of 'convert' semantics
In previous versions, the 'filter' command only supported containers as input, not expressions. Now you can use any expression as input, and the output is stored in a separate container (if the 'into' clause is used) or in the special 'it' variable.
For example, for a variable 'theFirstList' containing a comma-separated list of strings:
`foo,bar,baz,qux,quux,corge,grault`
And a second variable 'theSecondList' containing another comma-separated list of strings:
`garply,waldo,fred,plugh,xyzzy,thud`
The script line:
`filter items of theFirstList & comma & theSecondList with "b*" into theFilteredList`
Would result in the variable 'theFilteredList' containing:
`bar,baz`
On the other hand, the script line:
`filter items of theFirstList & comma & theSecondList with "b*"`
Would result in the variable 'it' containing:
`bar,baz`

## Backward compatibility
As stated above, the implementation is backward compatible.
This means that the script line:
`filter theList with "b*"`
Is equivalent to the following, more explicit variants:
`filter lines of theList with wildcard "b*"`
`filter lines of theList matching wildcard pattern "b*"`
Likewise, the script line:
`filter theList without "b*"`
Is equivalent to the following, more explicit variants:
`filter lines of theList without wildcard "b*"`
`filter lines of theList not matching wildcard pattern "b*"`