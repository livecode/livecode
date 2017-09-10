# Java CLASSPATH support

Limited support is available for loading custom Java classes 
and .jar files in the IDE on Mac and Linux. If the CLASSPATH 
environment variable is set before the Java virtual machine 
is initialised (i.e. before Java FFI is used), then any paths
specified are added to the locations searched by the default
class loader.

