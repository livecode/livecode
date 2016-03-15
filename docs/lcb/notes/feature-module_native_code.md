---
version: 8.0.0-dp-3
---
# LiveCode Builder Host Library
## Native code in extensions

LiveCode Builder extensions can now contain native code libraries
which LCB will use to resolve foreign handler references.

The foreign handler binding string should be of the form
`libname>function` to use this feature. In this case, the engine will
look for a library `libname` on a per-platform basis when the foreign
handler needs to be resolved.

Native code libraries should be present inside the `resources` folder
inside the extension archive. The engine derives the appropriate path
from the requested library name and current platform. The structure is
as follows:

    <extension>/
      resources/
        code/
          mac/
            <library>.dylib
          linux-x86/
            <library>.so
          linux-x86_64/
            <library>.so
          win-x86/
            <library>.dll

**Note:** At present, only the desktop platforms are supported.

**Note:** The above structure is likely to change in a future release.
In particular the `code` folder will sit at the same level as
resources rather than within it.
