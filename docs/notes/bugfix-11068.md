# Resolving object chunks does not throw an error for long chunk references.
When using named object chunks such as 'control <name>' or 'field <name>', the engine only accepts <name> of the form:
* The short name of the object (e.g. 'foo')
* The name of the object (e.g. 'field "foo"')
Previously, in the case of matching a name, the engine would only match the prefix ignoring everything after it.
This resulted in things like 'field the long id of field "foo"' potentially resolving incorrectly.
This issue has now been fixed - when using named object chunks, <name>, must either be just a name, or a string of the form '<type> "<name>"'.
To resolve long id/long name references, there is no need for a chunk and you should just use the variable directly:
    put the long id of field "foo" into tFooRef
    answer the short name of tFooRef

