# Ensure non-BMP characters roundtrip through htmlText

Previously, unicode characters outside of the basic multilingual plane (i.e. those
with codepoint < 65536) would fail to roundtrip through the htmlText property of
fields. This has now been fixed.

In addition, fixing this issue also means that unicode characters (of any
codepoint) can now appear in the metadata attribute of 'p' and 'span' tags.

Finally, the imageSource property can now span multiple characters. This is
required to allow it to apply to surrogate pairs (i.e. characters with codepoint
> 65535) and unicode character sequences which are considered a single 'char'
(i.e. human readable character / grapheme).

