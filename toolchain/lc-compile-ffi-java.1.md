lc-compile-ffi-java(1) -- convert java package spec files into lcb code
===================================================

## SYNOPSIS

**lc-compile-ffi-java** [_OPTION_ ...] [--] _DSLFILE_...

## DESCRIPTION

**lc-compile-ffi-java** parses a specification of a java package into an 
abstract form, and outputs as specified.

## OPTIONS

* --output _LCBFILE_:
  Output an LiveCode Builder source file containing a usable interface 
  between LiveCode Builder and the Java Native Interface. Compiling and
  loading _LCBFILE_ using **lc-compile** allows the functionality 
  described by the _DSLFILE_ to be used by LCB modules.

* --modulename _NAME_:
  Use _NAME_ as the name of the LiveCode Builder module specified by the
  --output option.

* --check _CHECKFILE_:
  Output the parsed data to _CHECKFILE_. This is used for checking
  consistency of output - the result of running **lc-compile-ffi-java** on 
  _CHECKFILE_ should be identical to _CHECKFILE_.
  
* -h, --help: Print some basic usage information.

* --: Stop processing options.  This is useful in case _DSLFILE_ begins with `-`
  or `--`.

## COPYRIGHT

Copyright 2016 LiveCode Ltd.

This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

## SEE ALSO

**lc-compile**(1).
