# Type Naming in LiveCode Builder
Copyright 2015 LiveCode Ltd.

## Introduction

The Builder standard library introduces a large number of types.
These types require a consistent naming convention, in order to
clearly indicate where the types come from and what they are intended
to be used for.

Some of the types listed in this document will not be available
immediately; however, they may be implemented in the future.

## Mixed-case naming

The current Builder compiler cannot recognise any symbol that is also
used in syntax.  So, for example, it is not possible to define a
variable called `last` because `last` is already used as a keyword in
syntax such as:

    the last offset of <Needle> in <Haystack>

There are a small number of types that are *compiler* built-in types
and can therefore safely have a lower-case name (e.g. `string` and
`list`), but this is an implementation detail and might change.

Types in the Builder standard library should therefore have mixed-case
names.

For consistency with the LiveCode naming conventions used elsewhere,
types should have title-case names.

## Universal types

There are some universal, fundamental types that are independent of
development platform and programming language.  These do not use a
platform prefix.

* `Pointer`: A memory address.

* `UInt<BITS>`: An unsigned integer represented using the specified
  number of bits (e.g. `UInt32`).

* `Int<BITS>`: A signed integer represented using the specified
  number of bits (e.g. `Int8`).

* `IntSize`, `UIntSize`: An (unsigned) integer representing a memory
  extent (e.g. the length of a buffer).  This is used whenever a
  number that's proportional to the size of memory is required.  On
  most platforms, its size will be equal to the size of a `Pointer`.

* `IntPointer`, `UIntPointer`: An (unsigned) integer with the same
  number of bits as a `Pointer`.

* `Float<BITS>`: A IEEE-754 binary floating point value represented
  using the specified number of bits (e.g. `Float32`).

* `Float<BITS>Dec`: A decimal floating point value represented using
  the specified number of bits (e.g. `Float32Dec`).

## LiveCode Core types

The Builder language standard library provides a small number of
"core" types that are central to the programming language and will be
used by *any* Builder program.  Like the universal types, these do not
use a platform prefix.

* `String`

* `List`

* `Number`, `Integer` and `Real`

* `Boolean`

## C types

For foreign function calls into C libraries, some C-specific
fundamental types are required.  These use the `C` platform prefix.

* `CBool`: C's `bool` type (`stdbool.h`).

* `CInt`: C's `int` type.

* `CUInt`: C's `unsigned int` type.

* `CFloat`: C's `float` type.

* `CDouble`: C's `double` type.

## LiveCode library types

For foreign function calls into the LiveCode engine's C/C++ libraries,
some special foreign types are required.

* `LCIndex`, `LCUIndex`: Indices (e.g. into lists or strings)

* `LCInt`, `LCUInt`: Integers

## String buffers

Many applications require nul-terminated string buffers.

* `ZStringNative`: Nul-terminated string buffer containing compact
  character data with 8 bits per character.

* `ZStringUTF8`: Nul-terminated string buffer containing UTF-8
  character data.

* `ZStringUTF16`: Nul-terminated string buffer containing UTF-16
  character data.
