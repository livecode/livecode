# Support for loading multi-module bytecode files (experimental)

The **load extension** command is now able to load LiveCode Builder
bytecode files (`.lcm` files) that contain multiple modules' bytecode.

The first module in each `.lcm` file is treated as the "main module"
of the module (i.e. the library or widget), and other modules are
treated as support modules.

Support modules only remain loaded if they are used by the main
module, and support modules must be submodules of the main module.
For example, if the main module is "com.livecode.newbutton", then all
other modules in the bytecode file must have names like
"com.livecode.newbutton.&lt;something&gt;".
