---
version: 8.0.0-dp-5
---
# LiveCode Builder Language

## Type definitions

* It is now possible define foreign handler types:

  ```
  foreign handler type MyCallback(in pContext as optional pointer, in pValue as any) as CBool
  ```

  When used in the context of a foreign handler definition, a foreign
  handler type will cause automatic bridging of the LCB handler to a C
  function pointer which can be called directly by the native code.

  The function pointers created in this fashion have lifetime
  equivalent to that of the calling context. In particular, for
  widgets they will last as long as the widget does, for all other
  module types they will last as long as the module is loaded.
