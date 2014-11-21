Expanded LiveCode Stackfiles File Format Specification
======================================================
Peter Brett, Mark Waddingham & Fraser Gordon

Copyright © 2014 RunRev Ltd.

This work is licensed under a [Creative Commons Attribution 4.0
International License](http://creativecommons.org/licenses/by/4.0/).

Revision History
----------------

Version | Comments
------- | -------------
0.0.1   | Initial draft
0.0.2   | Add arrays; add record & enum subtype specifiers; remove implicit typing; add "`.shared`" subdirectories for per-card data with external storage; do not store non-default property values; SHA-1 hashes of external values; simple case-folding with "`en_US`" locale.
0.0.3   | General rework. Self-describing literals; optional named types for all values; removed enums and records from fundamental types; revised lexical analysis; record each object's parent instead of children.
0.0.4   | Replace `STORAGE_SPEC` separator with "`:`"; change external storage indicator to "`&`" and relocate in property descriptor; simplify grammar slightly; add "`_kind`" file in object directory. Added implementation notes appendix.
0.0.5   | Replace "raw" values by "data" values; require unquoted strings to start with a letter; clarify that the `_parent` file contains a `NAME_STRING`.
0.0.9   | Initial implemented version.
0.0.10  | Remove SHA-1 and explicit filenames for external values; add "`!flag`" syntax.

Summary
-------

The binary LiveCode stackfile format has some disadvantages when doing
collaborative development using text-oriented distributed version
control systems such as git.

This document specifies a scheme for storing stackfile data in the
form of a directory tree containing text files. It is intended to work
well with version control systems like git.

Introduction
------------

### Background ###

A LiveCode stackfile is a type of object database that stores the
state information of a LiveCode project.  It contains both data
(global variables and the transient state of object instances) and
logic (in the form of scripts attached to object instances).

Previously, LiveCode stacks have been stored using a LiveCode-specific
binary encoding. This has provided some important advantages, among
which it facilitates rapid loading and saving, and provides a
convenient and portable way to store and transfer the object database
as a single file.

However, the main drawback of the binary LiveCode stackfiles is that
they cannot be easily managed using modern version control tools,
especially distributed version control systems such as git. In these
systems, every individual working on a project has a complete copy of
the project's history. Change history can be freely exchanged between
all copies of the project.  Most of the work of integrating changes
from different branches is carried out automatically through textual
merge.

Unfortunately, managing the binary LiveCode stackfiles in such a
version control system causes problems, because they cannot be merged
textually. Any two concurrent changes in any part of the stack cause
conflicts for which no merge tools are available.

### Aim of this specification ###

This document provides a specification for a new "expanded" stackfile
format, designed to perform well when managing changes to LiveCode
stacks using a distributed version control system.  All object data is
encoded as text, and divided into multiple files spread through a
directory tree.

### Design objectives ###

1. Minimise the chance of conflicts as far as reasonably practical.

2. In the event that conflicts do occur, maximise the likelihood that
   they can be correctly resolved programmatically.

3. Store only essential information.

### About this specification ###

The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL NOT",
"SHOULD", "SHOULD NOT", "RECOMMENDED", "MAY", and "OPTIONAL" in this
document are to be interpreted as described in
[RFC 2119](https://www.ietf.org/rfc/rfc2119.txt).

This document describes version 1.0.0 of the expanded LiveCode
stackfile specification.

General Comments
----------------

### Text encoding ###

Unless stated otherwise, all references to Unicode shall be considered
to be references to the Unicode 6.3 specification.

### Newline characters ###

When writing expanded stackfiles, end-of-lines in text files MUST be
indicated with a line feed (U+000A) character. The implementation MAY
additionally interpret as line terminations a carriage return
(U+000D), and/or a carriage return followed by a line feed (U+000A,
U+000D).

Text files SHOULD contain a terminal newline character in order to
ensure that line-based merging algorithms always have full lines as
input.

Object identification
---------------------

Objects stored in the object database must have identifiers that will
be expected to be globally unique. If object identifiers are
insufficiently unique, it would be likely that concurrently adding two
objects in different branches of the project could result in a
duplicate object identifier and thus a hard-to-resolve merge conflict.

Each object's identifier MUST be a Universally Unique IDentifier
(UUID), and it is RECOMMENDED to generate the UUID pseudorandomly
using the procedure specified in
[RFC 4122](https://www.ietf.org/rfc/rfc4122.txt), Section 4.4.

When represented as a string, object identifiers SHALL be represented
in lowercase hyphenated hexadecimal form, without brackets, e.g.

    a0118889-e1d1-4028-86fa-3a2e93db67ec

Allowing mixed- or upper-case representations of object identifiers
breaks text-based merging.

Filesystem layout
-----------------

The expanded LiveCode stackfile SHALL consist of a single directory
tree.  The root directory in the tree SHOULD have a name with the
format:

    <STACKFILE>.livecode.d

The directory structure MUST NOT contain any files or data other than
those defined by this specification.

An overview of the directory structure is as follows:

    <STACKFILE>.livecode.d
    ├ _version
    ├ <OBJECT_ID_LSB>
    ┆ ├ <OBJECT_ID>
    ┆ ┆ ├ _kind
    ┆ ┆ ├ _parent
    ┆ ┆ ├ _contents
    ┆ ┆ ├ _shared
    ┆ ┆ ├ _overflow
    ┆ ┆ ├ _propsets
    ┆ ┆ ├ [ … value files for properties w/ external storage ]
    ┆ ┆ ├ [ <CARD_OBJECT_ID>.shared ]
    ┆ ┆ ┆ ├ [ … value files for properties w/ external storage ]
    ┆ ┆ ├ [ <PROPSET_DIRNAME>.propset ]
    ┆ ┆ ┆ ├ _contents
    ┆ ┆ ┆ ├ _overflow
    ┆ ┆ ┆ ├ [ … value files for properties w/ external storage ]
    ┆ ┆ ┆
    ┆ ┆ ├ [ … additional custom property sets ]
    ┆ ┆
    ┆ ├ [ … additional <OBJECT_ID>s ]
    ┆
    ├ [ … additional <OBJECT_ID_LSB>s ]

The contents and interpretation of each of these files is described
later in this specification.

### Filename encoding ###

For maximum portability, all filenames used in the expanded LiveCode
stack MUST conform to the following restrictions:

1. Use only the Unicode characters in the set
   "`abcdefghijklmnopqrstuvwxyz0123456789-_`".

2. Have a length less than or equal to 256 characters.

3. Use the UTF-8 encoding (equivalent to ASCII for the permitted
   character set).

The character "_" (U+005F) is reserved as an escape character.  Two
escape sequences are used when a filename must be generated for an
arbitrary Unicode string:

1. The sequence "`__`" encodes a literal "`_`" character.

2. The sequence "`_xhh`" encodes a Unicode codepoint with hexadecimal
   representation "`00hh`".

3. The sequence "`_uxxxx`" encodes a Unicode codepoint with
   hexadecimal representation "`xxxx`". For example, the Unicode
   string "你好" may be encoded as "`_u4f60_u597d`".  Unicode
   codepoints outside the Basic Multilingual Plane may be encoded
   using a surrogate pair.

All other filenames including the `_` character are reserved for use
by the expanded LiveCode stackfile format.

Arbitrary Unicode strings MUST be pre-processed with Unicode
Normalization (Normalization Form C) and Unicode Case-Folding (Common
plus Simple, C+S) before encoding as filenames. When Case-Folding, the
"`en_US`" locale MUST be used.

In particular, note that generated filenames may begin with the
sequences "`__`", "`_u`" or "`_x`" but with no other sequence "`_c`"
where "`c`" is one of the permitted filename characters. This permits
generated filenames to coexist in a directory with filenames used
internally by the file format without collisions.

### Object directories ###

Some stackfiles contain very large numbers of objects. Some operating
systems and filesystems perform poorly with directories with large
numbers of entries.

The root directory of the expanded stackfile therefore does not
contain the per-object directories as direct children. Each object
directory's path relative to the root directory takes the form:

    <STACKFILE>.livecode.d/<OBJECT_ID_LSB>/<OBJECT_ID>/

The `<OBJECT_ID>` MUST be the hyphenated-string representation of the
object's UUID. The `<OBJECT_ID_LSB>` MUST be the lowest significant
byte of the `<OBJECT_ID>`. For example, the object with UUID
"`a0118889-e1d1-4028-86fa-3a2e93db67ec`" would have an object
directory located at:

    example.livecode.d/ec/a0118889-e1d1-4028-86fa-3a2e93db67ec/

Each `<OBJECT_ID_LSB>` directory MUST NOT contain any entries other
than `<OBJECT_ID>` subdirectories.

Property representation
-----------------------

Each object has a set of properties. Properties are key-value pairs
that describe the state of the object.

Property names are Unicode strings. Property values may be of a
variety of literal types, including:

* Booleans

* Integers (exact)

* Real numbers (inexact)

* Unicode strings

* Data strings (blocks of raw binary data)

* Arrays (name-value dictionaries)

In addition, derived types may be used, identified by name and stored
with a literal representation in one of the formats above.

### Property descriptor lexical analysis ###

Each property descriptor takes the form of a single line of text:

    WS           ::= " "+
    DESCRIPTOR   ::= STORAGE_SPEC [WS VALUE]
    STORAGE_SPEC ::= (NAME_PATH | GENERIC_PATH)
    NAME_PATH    ::= NAME_STRING [":" NAME_PATH]
    GENERIC_PATH ::= (NAME_STRING | DATA) [":" GENERIC_PATH]

The `STORAGE_SPEC` identifies a storage location (e.g. a property or
an array key). It is composed from a sequence of one or more names,
joined with "`:`" characters.  In some contexts, a `STORAGE_SPEC` is
permitted to contain raw data components.

If no `VALUE` is provided, the value is undefined.

    VALUE ::= [TYPE_SPEC WS] [FLAGS] (EXTERNAL | IMMEDIATE)

Values may be provided immediately (i.e. in the descriptor) or may be
located in an external file.

#### Type specifiers ####

    TYPE_SPEC ::= NAME_STRING

Each type is identified by name.

#### Flags ####

    FLAGS     ::= ("!" FLAG_NAME WS)+
    FLAG_NAME ::= NAME_STRING

Types may have any number of optional named flags attached to them.

#### Immediate values ####

    IMMEDIATE ::= LITERAL
    LITERAL   ::= "array" | BOOLEAN | NUMBER | STRING | DATA

Most values can be provided in the descriptor.  They are stored as
literal values with type information, if required. For example, if a
value from an enumerated type is stored in the file, it may be stored
as a `STRING` literal with a `TYPE_SPEC` that identifies the
enumerated type that it was originally stored as.

#### Literals ####

    BOOLEAN_VALUE ::= (“true” | “false”)

Booleans are represented directly.

    NUMBER         ::= [SIGN] (INTEGER_NUMBER | REAL_NUMBER)
    INTEGER_NUMBER ::= NONZERODIGIT DIGIT* | "0"
    REAL_NUMBER    ::= POINT_REAL | EXPONENT_REAL
    POINT_REAL     ::= [INT_PART] FRACTION | INT_PART "."
    EXPONENT_REAL  ::= (INT_PART | POINT_REAL) EXPONENT
    INT_PART       ::= DIGIT+
    FRACTION       ::= "." DIGIT+
    EXPONENT       ::= "e" [SIGN] DIGIT+
    SIGN           ::= "+" | "-"
    DIGIT          ::= "0"..."9"
    NONZERODIGIT   ::= "1"..."9"

Integer values are encoded as a series of digits without any leading
"0"s. Real numbers can be encoded in decimal or exponent form, and can
always be distinguished from integers by the fact that they contain
the character "." and/or the character "e".

The implementation SHOULD use the most compact real number format that
retains precision.

    STRING      ::= '"' STRING_ITEM* '"'
    STRING_ITEM ::= STRING_CHAR | ESCAPE_SEQ
    STRING_CHAR ::= <any source character except "\", '"', or line termination characters>
    ESCAPE_SEQ  ::= "\" <any ASCII character>

The string representation used for literals is a sequence of Unicode
characters between quotation mark (U+0022) delimiters. Only three
types of characters require special handling in a literal string: the
quotation mark (in order to avoid prematurely terminating the string),
line termination characters (in order to avoid prematurely terminating
the property descriptor) and the escape character "\" (U+005C).

Escape sequences MUST be used to encode all characters other than
those in Unicode categories L, M, N, P, and S, and the ASCII space
character, U+0020.

The following escape sequences are available:

Escape sequence | Interpretation
--------------- | --------------
`\\`            | Reverse solidus (U+005C)
`\"`            | Quotation mark (U+0022)
`\n`            | Line feed (U+000A)
`\r`            | Carriage return (U+000D)
`\xhh`          | Unicode codepoint U+00hh (hexadecimal)
`\uxxxx`        | Unicode codepoint U+xxxx (hexadecimal)
`\Uxxxxxxxx`    | Unicode codepoint U+xxxxxxxx (hexadecimal)

Large strings SHOULD be stored in external files.  For example,
property descriptors longer than 80 characters are very difficult to
view in a terminal.

    NAME_STRING      ::= STRING | UNQUOTED_STRING
    UNQUOTED_STRING  ::= UQ_STRING_LETTER UQ_STRING_CHAR*
    UQ_STRING_CHAR   ::= UQ_STRING_LETTER | “0”...“9” | “_” | “-” | “.”
    UQ_STRING_LETTER ::= “a”...“z” | “A”...“Z”

In addition to the representation used for string literals,
`NAME_STRING`s (used in the `STORAGE_SPEC` and `TYPE_SPEC`) may use an
"unquoted" string representation.  Unquoted strings may only contain
ASCII alphanumerics and the characters "-" (U+002D), "." (U+002E) and
"_" (U+005F), and must start with a letter.

During encoding, `NAME_STRING`s MUST be pre-processed with Unicode
Normalization (Normalization Form C) and Unicode Case-Folding (Common
plus Simple, C+S).

The unquoted string representation SHOULD be used wherever possible.

    DATA          ::= “<” (STRING | BASE64_STRING) “>”
    BASE64_STRING ::= <RFC 4648 base 64 encoded data>

Data strings are similar to strings but may contain data that is not
UTF-8 encoded (e.g. text in non-Unicode encodings; compressed data;
audiovisual data; or other binary encoded data).

Often data strings do happen to contain data that can be interpreted
in UTF-8 (e.g. an e-mail message in quoted-printable encoding). The
implementation MAY choose to store such data as a string rather than
as base 64 encoded data.

Other raw data strings should be encoded using the "base64" encoding
described in [RFC 4648](https://www.ietf.org/rfc/rfc4122.txt). The
implementation MUST NOT insert any characters into a `BASE64_STRING`
that are not in the "base64" alphabet.

Large raw data values SHOULD be stored in in external files, since
this is faster to load and save and more compact than using base 64
representation.

#### Storing array values by extending storage specs ####

If a property has an array value, a `STORAGE_SPEC` is constructed for
each key in the array by appending the key to the property's name. A
property descriptor is then created for each `STORAGE_SPEC`.

The base `STORAGE_SPEC` property descriptor for the property that
takes an array value has a `LITERAL` of "array" (with an additional
`TYPE_SPEC` if required).

When constructing array property descriptor `STORAGE_SPECs`, the
`GENERIC_PATH` form MAY be used if necessary to ensure accurate
representation of the key.

#### External values ####

    EXTERNAL  ::= "&" WS FILE_TYPE
    FILE_TYPE ::= "string" | "data" | "array"

Large pieces of data that cannot be reasonably fitted into a single
line may be split out into separate files. String values, data string
values and array values may be marked as being provided externally.

The file name is constructed according to the following procedure:

1. Convert the single `NAME_STRING` of the property's `NAME_PATH` to a
   filename according to the rules described in the “Filename
   encoding” section of this specification.

2. Add a suffix to the depending on the `FILE_TYPE`:

  1. If the `FILE_TYPE` is "`string`", add the suffix "`.txt`"

  2. If the `FILE_TYPE` is "`data`", add the suffix "`.bin`"

  3. If the `FILE_TYPE` is "`array`", add the suffix "`.map`"

If the resulting filename has more than 256 characters, then immediate
storage MUST used for the value. To prevent corruption of the
stackfile using large property values with crafted property names,
generated filenames MUST NOT match any of the regular expressions:

    _[^_ux]
    \.(propset|shared)$

External string values MUST be encoded using UTF-8. External raw
values MUST be stored directly, without any processing or encoding
performed.

External array values MUST be stored as lexicographically-sorted
property descriptor sequences encoded using UTF-8. The `STORAGE_SPEC`s
in the external file MUST exclude the externalized property's
`STORAGE_SPEC`. In order to prevent corruption of the stackfile using
large property values embedded in large property values, external
array values MUST NOT contain property descriptors with external
storage.

#### Derived types ####

There is only one derived type that is RECOMMENDED for all implementations.

    REF_TYPE_SPEC ::= "ref"
    REF_LITERAL   ::= STRING

Some values may be references to other objects.  There are two types
of reference supported:

1. UUID references are used to refer to objects that have UUIDs
   available. They SHOULD be used whenever possible.  The
   `REF_LITERAL` for a UUID reference is a `STRING` containing the UUID
   of the referenced object in hyphenated hexadecimal format.

2. ID (legacy) references are used to refer to objects from stacks in
   older formats that don't support UUIDs.  The `REF_LITERAL` for this
   type of reference is a `STRING` containing an integer ID, a stack
   name, and a parent stack name, separated by whitespace. If the
   object ID is in the same stack, the stack name may be omitted.  If
   the target stack has the same parent stack, or is a top-level
   stack, the parent stack name may be omitted.  If the ID couldn't be
   resolved when the stackfile was written out, the bare ID MAY be
   written out in the hope that it can be resolved at some point in
   the future.

### Property storage ###

In each object directory, there are numerous locations where
properties may be stored:

1. The "`_contents`" file is the default location for the objects
   properties to be stored.

2. The "`_shared`" file contains per-card data for shared
   properties. Some shared properties may be stored in per-card
   subdirectories of the object directory.

3. The "`_overflow`" file contains properties that violate the
   requirements for storage in any other location.

4. Objects can have custom properties set. Each custom property set
   has its own subdirectory of the object directory, and may contain
   "`_contents`" and "`_overflow`" files. Custom property set
   subdirectories with unusual names are listed in the "`_propsets`"
   file in the object directory.

These property files and directories SHOULD NOT be created if they
would otherwise be empty.

In general, it is RECOMMENDED that the implementation should not
create property descriptors for any property with an unmodified
default value.

#### Storage for internal properties ####

An internal property is an object property that is intrinsic to the
property definition. For example, objects defined in C++ have
properties that are stored as members of the object's class.

Internal property names are case-insensitive and can always be
represented using `NAME_STRING`s.

By default, property descriptors for internal properties are stored in
the object directory's "`_contents`" file. Large string and data
string values SHOULD normally use external storage. The implementation
MAY always use external storage for some properties, no matter how
small the value (e.g. an object's "`script`" property may always use
external storage).

If a property would normally be stored using external storage, but has
a name for which a filename would need to be generated, the property
descriptor MAY be moved to the "`_overflow`" file and the large value
encoded as an immediate value.  Property descriptors in the
"`_contents`" and "`_overflow`" files MUST be sorted lexicographically
by `STORAGE_SPEC`.

#### Storage for shared properties ####

A shared property is generated when a control object is part of a
group and is included in multiple cards. The property values for card
0 are taken from the internal properties' files.

The property values for other cards are stored in the object
directory's "`_shared`" file.

The "`_shared`" file is divided into sections for each card. Each
section has a section header.

    SHARED_SECTION ::= “[” OBJECT_UUID “]”
    OBJECT_UUID    ::= <UUID in hyphenated hexadecimal format>

All property descriptors are associated with the card identified by
the OBJECT_UUID specified in the preceding section header. The
"`_shared`" file MUST NOT contain any property descriptors before the
first section header.

Shared property names are case-insensitive (because they all have
corresponding internal properties).

Section headers in the file SHALL shall be ordered lexicographically
by `OBJECT_UUID`.  Property descriptors in each section SHALL be sorted
lexicographically by `STORAGE_SPEC`.

If the shared property's value for a particular card is the same as
the default value, the property descriptor SHALL be omitted. If a
section of the "`_shared`" file would contain no property descriptors,
that section SHALL be omitted.

#### External storage for shared properties ####

If a shared property for a card (other than card 0) requires external
storage the external storage file MUST NOT be stored in the object
directory. Each card's shared data is stored in a subdirectory of the
object directory:

    …/<OBJECT_ID>/<CARD_OBJECT_ID>.shared

Each external storage property descriptor in the "`_shared`" file has
its `FILE_NAME` resolved relative to the “.shared” subdirectory
corresponding to the section in which the property descriptor is
located.

#### Custom property sets ####

It is possible for objects to have custom properties. Custom
properties are organised into property sets.

Each set of custom properties has a name. The default property set is
named "" (the empty string).

Each custom property set's data is stored in a subdirectory of the
object directory:

    …/<OBJECT_ID>/<PROPSET_DIRNAME>.propset

#### Custom property directories ####

The `<PROPSET_DIRNAME>` is derived from the property set name by
converting it to a filename according to the rules described in the
"Filename encoding" section of this specification.

For the default property set, the `<PROPSET_DIRNAME>` MUST be set to
"`_empty`".  If the length of the `<PROPSET_DIRNAME>` with the
"`.propset`" suffix is more than 256 characters, then it cannot be
used. In this case, a suitable `<PROPSET_DIRNAME>` string SHOULD be
generated and stored in the "`_propsets`" file in the object
directory.  The generated `<PROPSET_DIRNAME>` MUST obey the "Filename
encoding" rules.

The "`_propsets`" file contains mappings from custom property set
names to custom property directory filenames.  One mapping is stored
per line.

    PROPSET_MAPPING ::= PROPSET_NAME WS FILE_NAME
    PROPSET_NAME    ::= STRING

If the "`_propsets`" file would otherwise be empty, it SHOULD NOT be
created.  `PROPSET_MAPPING`s SHALL be sorted lexicographically by
`PROPSET_NAME`.

#### Storage for custom properties ####

Each custom property set's subdirectory may contain "`_contents`" and
"`_overflow`" files. These files should store property descriptors in
a similar way to the corresponding files in the object directory. The
subdirectory may also contain value files for properties with external
storage.

Object type information
-----------------------

The "`_kind`" file in the object directory contains a `TYPE_SPEC` that
is used to determine what object class the object has been serialised
from.

Object parent-child relationships
---------------------------------

Objects may have children (e.g. a card may have controls as children).
The "`_parent`" file in the object directory contains the UUID of the
object's parent, stored as a `NAME_STRING` in hyphenated hexadecimal
format.

### Root stack ###

The stackfile MUST contain exactly one stack object without a
"`_parent`" file. This is the root stack.

Version information
-------------------

### _version file ###

The top-level "`_version`" file in the directory tree contains the file
format version.  The "`_version`" file MUST exist and MUST have the
following contents:

    Expanded LiveCode Stackfile <MAJOR>.<MINOR>.<PATCH>

where “<MAJOR>”, “<MINOR>”, and “<PATCH>” are the corresponding
decimal components of the specification version that the expanded
stackfile conforms to.

The implementation is RECOMMENDED to use the existence and contents of
the "`_version`" file during stackfile loading to verify that the
directory tree contains LiveCode stack information and is in a format
that the implementation supports.

### Version interpretation ###

The numeric version SHOULD be interpreted according to the
[Semantic Versioning 2.0.0](http://semver.org/spec/v2.0.0.html)
specification. Future revisions of this specification shall revise the
version information accordingly.

Appendix: Implementation notes
------------------------------

This section contains some notes about how optional or recommended
features of the expanded stackfile format should be implemented.

### Text encodings and line endings ###

git has a special feature to magically modify line endings to match
the platform's expectations.  This behaviour can be disabled using a
"`.gitattributes`" file in the stackfile's root directory.

However, users may (inadvisably) modify some of the stackfile programs
outside LiveCode.  This could cause line-endings to change.  It's
therefore important to be tolerant of line termination characters in
the text-based file formats defined in this specification.

There are exceptions for the external storage string files.  In this
case, ensure that exporting and importing makes no change whatsoever
to the character sequence, even if it's modified outside LiveCode.

### Number formats ###

Always store a real number as a real number (i.e. contains a “.”
and/or a “e” character) and an integer as an integer, even if the real
number is an exact integer.

There may be decimal multiple representations of the same
floating-point value, and the IEEE 754 standard defines which one
should be chosen.  When choosing how to encode a real number, follow
this standard.

### Choosing when to use external storage ###

Ideally, external storage would be used for string and data string
values when an encoded property descriptor would be longer than 80
glyphs. However, following that rule would mean very long `STORAGE_SPEC`
and `TYPE_SPEC` parts could force every string or data string value to
use external storage.

Instead, use external storage for string and data string values if the
encoded property descriptor is longer than 80 glyphs and the encoded
value is longer than 40 glyphs.

Finally, there are some properties for which it makes sense to always
use external storage.  At the moment, the only example is the "`script`"
property.

### Choosing which data string representation to use ###

Raw data string values may be represented as strings
(e.g. "`<"myvalue">`") or as base64 (e.g. "`<bXl2YWx1ZQ==>`".  In general,
base64 representation requires more storage.

When choosing the representation to use, use base64 if 1) the data
string value does not validate as UTF-8 or 2) the correctly-escaped
UTF-8 string representation requires more glyphs than the base64
representation.

The number of glyphs required for base64 encoding is given by
`ceil(len(x)*4/3)`.

### When to use a `GENERIC_PATH` ###

Suppose there is an array-valued property “`foo`”, with keys “`bar`” and
“`baz`”.  It could be represented as follows:

    foo array
    foo:bar 10
    foo:baz "Elementary, my dear Watson"

In this case, the keys are strings, and are valid unquoted strings. So
NAME_PATHs can be used in the array representation. However, LiveCode
arrays allow arbitrary strings as keys:

    foo:"arbitrary\nstring" true

This is still a `NAME_PATH`.

It's also possible to use a data string value as an array key, which
might not validate as UTF-8. In that case, it's possible to use a
`GENERIC_PATH` with data string elements:

    foo:<aW52YWxpZA==> "Hopefully users won't do this"
