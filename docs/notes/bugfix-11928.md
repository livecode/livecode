# Inconsistencies in behavior when doing 'delete the selectedChunk'.
The following should all operate the same way after selecting a line in a field by doing 'triple-click', or just selected the whole line without the paragraph break:
* pressing backspace
* executing 'delete the selectedChunk'
* executing 'get the selectedChunk; delete it'
Previously, 'delete the selectedChunk' would cause paragraph styles not to be set correctly on the resulting paragraph; or the paragraph break to be included when it should not be - this is no longer the case.
Previously, 'get / delete it' would only work correctly the first time the command was executed - this is no longer the case.
