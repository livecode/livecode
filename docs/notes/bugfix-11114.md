# secureMode restricts access to network
The securityPermissions and securityCategories properties have been reinstated in the main engine. This means that the exact set of restrictions in secureMode is now configurable. The secureMode property turns off access to all security categories, use the securityPermissions property for more fine-grained control.
In particular, to run in secure mode with network access use 'set the securityPermissions to network'.
