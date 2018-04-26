# Make the encoding property of field char chunks more useful

The 'encoding' char-level field property will now return native
if all chars in the chunk can be encoded in the native encoding,
and unicode otherwise.

This means that the property will now return the identical value
as it did in 6.7 and before, assuming that the field text hadn't
had its encoding changed by script (via the textFont ',unicode'
flag).
