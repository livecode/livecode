# Non-executable file redirection on Mac
Mac AppStore rules require that only executables (including bundles and apps) are present within the Contents/MacOS folder in the application bundle.
However, historically (for cross-platform purposes), LiveCode applications traditional place resources relative to the engine executable, resulting in non-executable files to be present in the Contents/MacOS folder which violates AppStore signing policy.
To remedy this situation without requiring users to change scripts, a simple redirection facility has been implemented in the engine:
If an attempt is made to open a file for read which falls within Contents/MacOS and does not exist, the engine will attempt to open the same path but under Contents/Resources/_MacOS instead.
If an attempt is made to list files in a folder which falls within Contents/MacOS, the engine will list files in that folder and concatenate them will files within the same folder under Contents/Resources/_MacOS.
Additionally the standalone builder has had an extra processing step added on Mac:
After the Mac bundle has been built, the S/B recurses through Contents/MacOS and creates an identical folder structure based at Contents/Resources/_MacOS. All non-executable files in any folders under Contents/MacOS are moved to the same folder under Contents/Resources/_MacOS whereas any Mach-O executable files are left where they are.
The result of this is that after building a standalone, from a script's point of view nothing has changed; but the app bundle will conform to the rules required for signing for the Mac AppStore.
