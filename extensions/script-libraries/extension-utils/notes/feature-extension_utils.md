# Extension Package Utilities library
An extension package utilities library has been added. It contains 
handlers for managing LiveCode extension packages.

## API
The following handlers are implemented:

- `extensionFetchSourceFromFolder pFolder, rSource, rSupportFiles, rType`: Fetch basic information about extension source from a folder
- `extensionPackageScriptExtension pSourceFolder, pTargetFolder, pRemoveSource`: Package a LiveCode Script extension
- `extensionPackage pSourceFolder, pTargetFolder, pRemoveSource`: Package an extension
- `extensionFindInFolder pFolder, pIsUserFolder, pRecursive, xDataA`: Find any extensions in the given folder
- `extensionDepsOrder pLCCompile, pExtensionList`: Use lc-compile to order LiveCode Builder extensions by dependency.
- `extensionCheckModuleVersion pLCCompile, pModule`: Check the module bytecode is the same version as a particular lc-compile
- `extensionFetchMetadata pManifestPath`: Fetch metadata from an extension manifest
- `extensionExtractDocs pDocsParser, pOutputDir`: Extract docs from extension files
- `extensionBuildPackageAndExtractLCB pDocsParser, pTargetFolder, pCommercialExtension, pLCCompile, pLCIPath, pExtraLCIPaths`: Build, package and extract LiveCode Builder extensions
- `extensionOrderByDependency pExtensions, pRequiresA, pIncludeBuiltin`: Order extensions by dependency using requires info
- `extensionLCCompileVersion pLCCompile`: Return the bytecode version of a particular lc-compile
- `extensionCompile pLCCompile, pFile, pSupportFiles, pLCIPath, pExtraLCIPaths, pTargetFolder, pOutputFilename`: Compile a LiveCode Builder extension file
- `extensionBytecodeFilename pLCCompile, pUseVersion`: Return the default bytecode output filename
- `extensionProtectStack pProtectifyScript, pSourceFolder, pSourceFileName, pOutput, pGitHash`:  Protect a livecode stack using the provided script