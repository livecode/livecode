lc-run(1) -- run LiveCode Builder bytecode programs
===================================================

## SYNOPSIS

**lc-run** [_OPTION_ ...] [--handler _NAME_] [--] _LCMFILE_ [_ARG_ ...]

**lc-run** [_OPTION_ ...] --list-handlers [--] _LCMFILE_

## DESCRIPTION

**lc-run** runs a LiveCode Builder program from the bytecode file _LCMFILE_.
When running an LCB program with **lc-run**, some standard libraries are
unavailable (e.g. the `canvas` and `engine` libraries).

**lc-run** can use any public, top-level handler with no arguments as an entry
point.  If the `--handler` option is not used to specify an entry point
_NAME_, then **lc-run** looks for a handler named `main()`.

The `--list-handlers` option can be used to obtain a list of valid entry points
for a particular _LCMFILE_.

Any arguments _ARG_... specified after _LCMFILE_ are available to the program as
the value of `the command arguments`.

## OPTIONS

* -l, --load _LCMLIB_:
  In addition to loading the _LCMFILE_, load a module from the
  bytecode file _LCMLIB_.  Any number of `--load` options may be
  provided.  _LCMFILE_ is loaded first, followed by each _LCMLIB_ in
  reverse order.

* -H, --handler _NAME_:
  Call the handler with the specified _NAME_ as the entry point of the program.
  _NAME_ must have public visibility and accept no arguments.  The default value
  of _NAME_ is `main`.

* --list-handlers: Don't run the program.  Instead, print a list of valid entry
  point handlers in _LCMFILE_ to standard output.

* -h, --help: Print some basic usage information.

* --: Stop processing options.  This is useful in case _LCMFILE_ begins with `-`
  or `--`.

## COPYRIGHT

Copyright 2015 LiveCode Ltd.

This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

## SEE ALSO

lc-compile(1).
