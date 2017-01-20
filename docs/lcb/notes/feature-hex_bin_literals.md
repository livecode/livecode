# LiveCode Builder Tools
## lc-compile
### Errors

* Parsing of numeric literals, in general, has been tightened up. In
  particular, the compiler will detect invalid suffixes on numeric
  literals meaning you cannot accidentally elide a number with an
  identifier.

      1.344foo -- ERROR
      0xabcdefgh -- ERROR
      0b010432 -- ERROR

# LiveCode Builder Language
## Literals

* Base 2 (binary) integer literals can now be specified by using a
  "0b" prefix, e.g.

      0b0000
      0b1010

* Base 16 (hexadecimal) integer literals can now be specified by using
  a "0x" prefix.  e.g.

      0xdeadbeef
      0x0123fedc
