# Faster Unicode text processing

Text processing operations involving Unicode strings are now about 25%
faster.

The engine now has its own implementation of the Unicode 'grapheme
cluster breaking' algorithm.  This means that it no longer needs to
incur the penalty of initialising an ICU grapheme break iterator,
which involves making a complete copy of the target string.
