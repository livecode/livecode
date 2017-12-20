---
version: 9.0.0-dp-6
---
# LiveCode Builder Language

## Foreign bindings to Java

It is now possible to bind to Java class methods and fields in LCB via 
the Java Native Interface (JNI). Foreign handlers bind to Java class 
using the existing foreign handler syntax, but with an appropriate
binding string.

The Java binding string has the following form:

    "java:[className>][functionType.]function[!calling]"
    
Where *className* is the qualified name of the Java class to bind to, 
*functionType* is either empty, or 'get' or 'set', which are 
currently used for getting and setting member fields of a Java class, 
and *function* specifies the name of the method or field to bind to. The
function 'new' may be used to call a class constructor. *function* also
includes the specification of function signature, according to the 
[standard rules for forming these](http://journals.ecs.soton.ac.uk/java/tutorial/native1.1/implementing/method.html) 
when calling the JNI.

*calling* specifies the calling convention which can be one of:

 - `instance`
 - `static`
 - `nonvirtual`
 
Instance and nonvirtual calling conventions require instances of the given
Java class, so the foreign handler declaration will always require a Java
object parameter.

### Examples

- Binding to a class constructor with no parameters:

	foreign handler CreateJavaObject() returns JObject binds to "java:java.lang.Object>new()"

- Binding to a class constructor with parameters:

	foreign handler CreateJavaString(in pBytes as JByteArray) returns JString binds to "java:java.lang.String>new([B)"

- Binding to a class instance method

	foreign handler JavaStringIsEmpty(in pString as JString) returns CBool binds to "java:java.lang.String>isEmpty()Z"
	
- Binding to a class static method

	foreign handler CallJavaAdd(in pLeft as CInt, in pRight as CInt) returns CInt binds to "java:java.lang.Math>addExact(JJ)J!static"

- Binding to a class field

	foreign handler JavaCalendarSetTime(in pObj as JObject) returns nothing binds to "java:java.util.Calendar>set.time(J)"
	foreign handler JavaCalendarGetTime(in pObj as JObject) returns CInt binds to "java:java.util.Calendar>get.time()J"

- Binding to a class constant

	foreign handler GetJavaPi() returns CDouble binds to "java:java.lang.Math>get.PI()D!static"

> **Note:** This feature is still highly experimental. All aspects of the
> syntax are subject to change. Incorrect binding strings may cause your
> application to crash, so please ensure you back up your stacks before
> experimenting.

> **Important:** This feature is currently supported on Android, Mac and
> Linux. Binding to java classes requires the availability of a Java 
> runtime and access to the appropriate libraries. On Mac, 
> the `JAVA_HOME` environment variable must be set to the path to your 
> Java installation (usually at 
> `/Library/Java/JavaVirtualMachines/jdk1.7.0_55.jdk/Contents/Home`). 
> On Linux, your `LD_LIBRARY_PATH` must be set to the folder containing 
> the `libjvm.so` library (usually at ${JAVA_HOME}/jre/lib/amd64/server)
> on 64-bit Linux.