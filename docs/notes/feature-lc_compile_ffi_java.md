# LiveCode Builder Tools
## lc-compile-ffi-java

* **lc-compile-ffi-java** is a tool to create LiveCode Builder code which 
  provides an interface between LCB modules and the Java Native 
  Interface (JNI). It does this by taking a specification of a java
  package written in a domain-specific language and creating an LCB 
  module which wraps the necessary foreign handlers which call the 
  appropriate JNI methods. See the [documentation for the tool](https://github.com/livecode/livecode/blob/develop/toolchain/lc-compile-ffi-java.1.md) 
  and the [description of the DSL](https://github.com/livecode/livecode/tree/develop/docs/specs/java-ffi-dsl.md) 
  for more details
  