# LiveCode Builder Tools
## lc-compile
### Command-line interface

* The new `--forcebuiltins` command line flag can be used in combination with
  the `--outputauxc OUTFILE` option to generate shims for C foreign bindings.
  Use the output file to link with any static libraries used by modules on iOS
  to create a lcext binary and the shims will ensure that required objects from
  the static libraries are included in the resulting binary.
