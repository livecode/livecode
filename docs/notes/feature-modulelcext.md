# Static linked code libraries for iOS device builds

The standalone builder now supports `.lcext` compiled objects that link static
libraries used by a LCB module to the module compiled as C++ using lc-compile's
`--forcebuiltins --outputauxc` options. Additionaly, a new section named 
Foreign Code Libraries has been added to the LiveCode Builder Language Reference
describing the creation of `.lcext` objects and use of code libraries in general.