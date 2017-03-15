# LiveCode Builder Standard Library

## Java utilities

There is now a utility library for manipulating java objects. It contains
a type `JObject` which wraps a Java object, some type conversion operations:

* `StringFromJString` - converts a Java string to an LCB String
* `StringToJString` - converts an LCB String to a Java string
* `DataFromJByteArray` - converts a Java byte array to LCB Data
* `DataToJByteArray` - converts LCB Data to a Java byte array

and a utility for determining the class of a given java object:

* `GetJavaClassName` - return an LCB String containing the class name of the given `JObject`
