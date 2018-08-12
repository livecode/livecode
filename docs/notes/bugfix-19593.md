# Type should work with accented characters

The `type` command now handles Unicode characters in a manner
consistent with normal keyboard entry. If a Unicode character
is typed and it has a native mapping, then it is propagated as
a keypress with the keycode being the code of the character. If
it has no native mapping, it is propagated with keycode equal
to the Unicode codepoint with bit 22 set to 1. In either case
the string value of the keypress is the Unicode codepoint.
