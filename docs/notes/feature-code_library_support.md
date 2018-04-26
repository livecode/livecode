# Code Library Support

Extensions can now include compiled libraries on which they depend. The
libraries must be compiled for each platform and architecture they are
required on and placed folders named with a platform ID in the extension
code folder. The platform ID folder names are in the form:

    <architecture>-<platform>[-<options>]

See the [platform ID](https://github.com/livecode/livecode/blob/develop/docs/development/platform-id.md)
specification for more details.

On all platforms with the exception of iOS only dynamically linked
libraries are supported. 

On iOS 8+ dynamically linked frameworks (`.framework`) are supported and
on all versions of iOS statically linked frameworks and libraries (`.a`)
are supported. Static linking is not yet supported in iOS simulator
builds. 

If the iOS library requires linker dependencies a text file 
(`.txt`) may be included to list them in the form:

    {library | [weak-]framework} <name>

Additionally, on iOS the `.lcext` extension is used to identify code
resources that conform to the exported symbols and sectors of externals.
Specifically they have a `__deps` sector that contains the content of the
dependencies file mentioned above and they export a `LibInfo` struct
named `__libinfoptr_<libraryname>`. Examples of generating `.lcext` files
are available in the LiveCode source repository. This is a more
efficient means of inclusion as it allows the compiler to strip unused
symbols.
