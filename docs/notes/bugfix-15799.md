# Some arrays encoded in 6.7 format from 7.0 won't load into 6.7.

It was possible for an array in 7.0 to have a key that contained the empty array. When encoded in 6.7 format using arrayEncode,
the resulting data would not decode correctly in 6.7 - producing a truncated result.

This has been fixed - 6.7 will now successfully load such arrays when generated from 7.0.
