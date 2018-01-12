lc-compile(1) -- compile LiveCode Builder source code
=====================================================

## SYNOPSIS

**lc-compile** [_OPTION_ ...] --output _OUTFILE_ [--] _LCBFILE_...

**lc-compile** [_OPTION_ ...] --outputc _OUTFILE_ [--] _LCBFILE_...

**lc-compile** [_OPTION_ ...] --deps [_DEPSMODE_] [--] _LCBFILE_...

## DESCRIPTION

**lc-compile** compiles the named input _LCBFILE_ to bytecode, saving the
resulting bytecode to _OUTFILE_.

If one or more `--modulepath` options are provided, and the `--interface` option
is not specified, then **lc-compile** may additionally generate an interface
(`.lci`) file in the first `--modulepath` specified.

## OPTIONS

* --modulepath _PATH_:
  Search for interface (`.lci`) files in _PATH_, which should be a directory.
  If the `--interface` option is not specified, then the first `--modulepath`
  option specified determines the directory in which an interface file may be
  created for _LCBFILE_.

* --output _OUTFILE_:
  Generate LiveCode bytecode in _OUTFILE_, which should be the path to a `.lcm`
  file.  If _OUTFILE_ already exists, it will be overwritten.

* --outputc _OUTFILE_:
  Generate LiveCode bytecode as a static array(s) embedded in C source code in
  _OUTFILE_, which should be the path to a `.c` file.  If _OUTFILE_ already
  exists, it will be overwritten.

* --outputauxc _OUTFILE_:
  Generate LiveCode bytecode as a static array(s) embedded in C source code in
  _OUTFILE_, which should be the path to a `.c` file.  If _OUTFILE_ already
  exists, it will be overwritten.
  This is the same as --outputc mode, except that it does not emit the 'builtin'
  module (so should be used for additional sets of C embedded modules).

* --deps [_DEPSMODE_]:
  Generate dependency information on standard output.  _DEPSMODE_ may
  be `order`, `changed-order`, or `make`.  If _DEPSMODE_ is omitted,
  `make` is assumed.  See also the **DEPENDENCY INFORMATION** section
  below.

* --manifest _MANIFEST_:
  Generate a module manifest in _MANIFEST_.  This is used by the LiveCode IDE.

* --interface _INTERFACE_:
  Generate the module interface file in _INTERFACE_.

* -Werror:
  Turn all warnings into errors.

* -h, --help:
  Print some basic usage information.

* --:
  Stop processing options.  This is useful in case _LCBFILE_ begins with `--`.

The `--output` and `--outputc` / `--outputauxc` options cannot be used together.

## DEPENDENCY INFORMATION

**lc-compile**'s `--deps` mode is used to assist in compiling multiple
`.lcb` source files that depend on each other in the correct order.
In this mode, you should usually specify all of the source files for a
project as command-line parameters; **lc-compile** will output the
dependency information between them on standard output.

If _DEPSMODE_ is `order`, the output is a list of input files in the
order in which they need to be compiled in order to satisfy all
dependencies.  If _DEPSMODE_ is `changed-order`, the output is the
same list, but with all input files that are up-to-date omitted.
Input files are considered up-to-date if the corresponding interface
file is newer than its dependencies.

If _DEPSMODE_ is `make`, or is not specified, then the output is a
Makefile fragment declaring the dependencies between the input files
and any interface files that they depend on.  `--deps make` can also
be used with a single input file and the `--output` option.  In this
mode, **lc-compile** the output is a Makefile fragment that declares
the dependencies between the input file, the output file, and the
corresponding interface file.

## COPYRIGHT

Copyright 2014-2015 LiveCode Ltd.

This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.

## SEE ALSO

lc-run(1), make(1).
