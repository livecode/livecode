# Static linked code libraries for iOS device builds

The standalone builder now supports `.lcext` compiled objects that link static
libraries used by a LCB module to the module compiled as C++ using lc-compile's
`--forcebuiltins --outputauxc` options. Additionally, the `Using compiled libraries`
section of the `Extending LiveCode` guide has been updated to describe the
creation of `.lcext` objects.