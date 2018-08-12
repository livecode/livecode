# Implement [effective] revScriptDescription property

A new property has been implemented object reflection. The property is
currently undocumented and subject to change without notice as it is
intended largely for internal use.

The `revScriptDescription` of an object returns a multi-dimensional
array description detailing the constants, variables and handlers of
and object. The `effective revScriptDescription` of an object returns
a sequence of arrays with keys `object` and `description` in the order
that messages are passed through an object.
