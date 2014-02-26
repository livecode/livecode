# Crash of standalone -ui mode and server engine on some Linux distributions
The engine (both server and standalone) were not previously checking properly for the correct initialization of the optional text libraries it needs to support graphics. This has been fixed by disabling text rendering if the libraries are not available (other graphics operations will still work, however).
