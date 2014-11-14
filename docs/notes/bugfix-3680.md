# Initializers for constants and locals too strict
Initializers used for constants and locals can now be of one of the following forms:
* number
* +number
* -number
* quoted literal
* if explicitVars is false then an unquoted literal
* if explicitVars is true then any unquoted literal that would evaluate to the same string
The last rule means that when explicitVars is true you can use constants such as true and false without quotes.


