lc-compile(1) -- compile LiveCode Builder source code
=====================================================

## SYNOPSIS

**lc-compile** [_OPTION_ ...] --output _OUTFILE_ [--] _LCBFILE_

**lc-compile** [_OPTION_ ...] --outputc _OUTFILE_ [--] _LCBFILE_

**lc-compile** [_OPTION_ ...] --deps [DEPKIND] [--] _LCBFILE_ ... _LCBFILE_...

## DESCRIPTION

**lc-compile** compiles the named input _LCBFILE_ to bytecode, saving the
resulting bytecode to _OUTFILE_.

If one or more `--modulepath` options are provided, **lc-compile** may
additionally generate an interface (`.lci`) file in the first `--modulepath`
specified.

## OPTIONS

* --modulepath _PATH_:
  Search for interface (`.lci`) files in _PATH_, which should be a directory.
  The first `--modulepath` option specified determines the directory in which
  an interface file may be created for _LCBFILE_.

* --output _OUTFILE_:
  Generate LiveCode bytecode in _OUTFILE_, which should be the path to a `.lcm`
  file.  If _OUTFILE_ already exists, it will be overwritten.

* --outputc _OUTFILE_:
  Generate LiveCode bytecode as a static array embedded in C source code in
  _OUTFILE_, which should be the path to a `.c` file.  If _OUTFILE_ already
  exists, it will be overwritten.
  
* --deps [make]:
  Generate lci file dependencies in make format for the input source files.

* --deps order:
  Output the input source files in dependency order, the one that needs to be
  compiled first being first.

* --deps changed-order:
  Output the input source files in dependency order, the one that needs to be
  compiled first being first. Any source files which don't need to be recompiled
  (based on timestamp comparisons with the interface files) will be omitted.

* --manifest _MANIFEST_:
  Generate a module manifest in _MANIFEST_.  This is used by the LiveCode IDE.

* -h, --help:
  Print some basic usage information.

* --:
  Stop processing options.  This is useful in case _LCBFILE_ begins with `--`.

The `--output` and `--outputc` options cannot be used together.

## COPYRIGHT

Copyright 2014-2015 LiveCode Ltd.

This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.

## SEE ALSO

lc-run(1).
