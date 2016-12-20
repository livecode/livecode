# LiveCode Builder Language
## Identifiers

* Unqualified identifiers are now expected to match `[A-Z0-9_]`. The `.`
  symbol is interpreted as a namespace operator. 

## Namespaces
* Identifiers in LCB can now be qualified, to disambiguate between 
  symbols defined in different namespaces.

# LiveCode Builder Tools
## lc-compile
### Errors

* A new error has been added for identifiers in declaration context that
  contain `.` - identifiers should always be declared without 
  qualification.

# [18107] Do not permit namespace operator in unqualified identifiers.
