# Operator Precedence in LiveCode builder
Copyright © LiveCode Ltd.

## Introduction

The Builder standard library provides a large variety of syntax definitions.
Syntax operators can be combined in a huge number of ways, and it's important
that the interaction is sensible and consistent.

Choosing appropriate precedence levels fulfils some important objectives:

- The ordering of precedence is consistent with LiveCode Script
- Complicated expressions can easily be written without needing many
  parentheses `(…)`
- The ordering of precedence is reasonably consistent with other programming
  languages

The precedence levels are named.  This makes it much clearer, when reading an
operator syntax definition, why its assigned precedence level is appropriate.

Multiple named precedence levels may share the same numeric priority.

## Precedence levels

Lower precedence numbers are more tightly-binding.

| Level | Name              | Examples                      |
|-------|-------------------|-------------------------------|
| 1     | scope resolution  | `.`                           |
| 2     | function call     | `MyHandler()`                 |
|       | subscript         | `tList[1]`, `tArray["key"]`   |
| 3     | property          | `the paint of this canvas`    |
|       | subscript chunk   | `char 2 of tString`           |
| 4     | conversion        | `tString parsed as number`    |
|       | function chunk    | `the offset of "o" in "foo"`  |
| 5     | modifier          | `-tNum`, `bitwise not`        |
| 6     | exponentiation    | `^`                           |
| 7     | multiplication    | `/`,`*`,`mod`, `div`          |
| 8     | addition          | `+`, `-`                      |
|       | concatenation     | `&`, `&&`                     |
| 9     | bitwise shift     | `1 shifted left by 3 bitwise` |
| 10    | bitwise and       | `7 bitwise and 1`             |
| 11    | bitwise xor       | `7 bitwise xor 5`             |
| 12    | bitwise or        | `2 bitwise or 4`              |
| 13    | constructor       | `rectangle tList`             |
| 14    | comparison        | `<=`, `<`, `is`               |
|       | classification    | `is a`                        |
| 15    | logical not       | `not tBoolean`                |
| 16    | logical and       | `Foo() and Bar()`             |
| 17    | logical or        | `Foo() or Bar()`              |
| 18    | sequence          | `,`                           |
