script "com.livecode.library.extension-utils"
/*
Copyright (C) 2018 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>. 
*/

/**
Title: Extension Package Handling Utilities

Type: library

Summary: This library provides handlers for managing livecode extension packages

Version: 1.0.0
*/

/**
*/
on extensionInitialize
   if the target is not me then
      pass extensionInitialize
   end if
   
   insert the script of me into back
   
   if the environment contains "development" then
      set the _ideoverride of me to true
   end if
end extensionInitialize

on extensionFinalize
   if the target is not me then
      pass extensionFinalize
   end if
   
   remove the script of me from back
end extensionFinalize

private command log pMsg
   write pMsg & return to stdout
end log

local sDefaultModuleVersion
private function __CompiledModuleVersion pModule
   local tBytes
   put url("binfile:" & pModule) into tBytes
   return byteToNum(byte 4 of tBytes) * 256 + \
         byteToNum(byte 3 of tBytes)
end __CompiledModuleVersion

/**
Check the module bytecode is the same version as a particular lc-compile

Parameters:
pLCCompile: 
The path to the lc-compile program. If empty, this will be computed.

pModule: 
The path to the compiled module bytecode file (.lcm)

Returns: 
true if the version of lc-compile matches that of the bytecode file,
false otherwise.
*/

function extensionCheckModuleVersion pLCCompile, pModule
   return __CompiledModuleVersion(pModule) is \
         extensionLCCompileVersion(pLCCompile)
end extensionCheckModuleVersion

/**
Return the bytecode version of a particular lc-compile

Parameters:
pLCCompile: 
The path to the lc-compile program. If empty, this will be computed.

Returns:
An integer, the version of the bytecode output by <pLCCompile>
*/
function extensionLCCompileVersion pLCCompile
   if pLCCompile is empty then
      if sDefaultModuleVersion is empty then
         put extensionLCCompileVersion(__extensionCompilerPath()) \
               into sDefaultModuleVersion
      end if
      return sDefaultModuleVersion
   end if
   
   local tTempFile, tModule, tFolder
   put the tempname into tTempFile
   set the itemdelimiter to slash
   put item 1 to -2 of tTempFile into tFolder
   put "module com.livecode.dummy" into tModule
   put return & "end module" after tModule
   put tModule into url("file:" & tTempFile)
   
   extensionCompile pLCCompile, tTempFile, "", "", "", tFolder, \
         "module.lcm"
   
   return __CompiledModuleVersion(tFolder & slash & "module.lcm")
end extensionLCCompileVersion

/**
Return the default bytecode output filename

Parameters:
pLCCompile: 
The path to the lc-compile program. If empty, this will be computed.

pUseVersion: 
Whether to use the bytecode version as part of the filename or not.
Defaults to true.

Returns:
Either "module.lcm" or "module.<lc-compile_version>.lcm" depending on 
the value of the <pUseVersion> parameter.
*/
function extensionBytecodeFilename pLCCompile, pUseVersion
   if pUseVersion is empty then 
      put true into pUseVersion
   end if
   
   if pUseVersion then
      return "module." & extensionLCCompileVersion(pLCCompile) & ".lcm"
   else
      return "module.lcm"
   end if
end extensionBytecodeFilename

local sExternalsA
private command __EnsureExternal pExternal
   if sExternalsA[pExternal] then
      exit __EnsureExternal
   end if
   
   local tEnvironment
   put the environment into tEnvironment
   switch tEnvironment
      case "development"
      case "development command line"
      -- If we are running in the IDE we should have externals available
      	exit __EnsureExternal
      case "standalone application"
      case "command line"
         -- In a build standalone externals should have been included
         -- If we are running tests, externals should be loaded.
         exit __EnsureExternal
      case "server"
      default
         break
   end switch
   
   -- If we are running with an 'externals' folder, assume
   -- the externals are loaded
   local tHomeFolder
   put $REV_HOME into tHomeFolder
   if tHomeFolder is empty then
      put $LIVECODE_SERVER_HOME into tHomeFolder
   end if
   if tHomeFolder is empty then
      put specialfolderpath("engine") into tHomeFolder
   end if
   
   local tFileExtension
   switch the platform
      case "MacOS"
         if the environment is "server" then
            put "dylib" into tFileExtension
         else
            put "bundle" into tFileExtension
         end if
         break
      case "Win32"
         put "dll" into tFileExtension
         break
      case "Linux"
         put "so" into tFileExtension
         break
   end switch
   
   local tExternalFilename
   if the environment is "server" then
      put "server-" after tExternalFilename
   end if
   put pExternal after tExternalFilename
   put "." & tFileExtension after tExternalFilename
   
   if there is a file (tHomeFolder & slash & "externals" & slash & \
         tExternalFilename) then
      put true into sExternalsA[pExternal]
      exit __EnsureExternal
   end if
   
   put specialFolderPath("engine") & slash before tExternalFilename
   set the externals of the templateStack to tExternalFilename
   
   create stack pExternal && "External"
   start using it
   if the externalCommands of it is empty then
      throw "cannot load external" && pExternal
   else
      put true into sExternalsA[pExternal]
   end if
end __EnsureExternal 

private on __addToDependencies pDependee, pDependent, @xDependencies
   # AL-2015-04-13: [[ Bug 15216 ]] Use keys of array to remove duplicates
   put true into xDependencies[pDependee][pDependent]
end __addToDependencies

private on __addToList pNode, @xList
   if xList is empty then
      put pNode into xList
   else
      put return & pNode after xList
   end if
end __addToList

private on __removeFromList pNode, @xList
   get lineOffset(pNode, xList)
   if it is not 0 then
      delete line it of xList
   end if
end __removeFromList

private on __visitNode pDependencies, pNode, @xUnmarked, @xMarked, @xTemporaryMarked, @xOrder
   if pNode is among the lines of xTemporaryMarked then
      return "Error"
   end if
   if pNode is among the lines of xMarked then
      # AL-2015-04-13: [[ Bug 15216 ]] Remove from unmarked if this was already marked
      __removeFromList pNode, xUnmarked
      return ""
   end if
   __addToList pNode, xTemporaryMarked
   repeat for each key tNode in pDependencies[pNode]
      __visitNode pDependencies, tNode, xUnmarked, xMarked, xTemporaryMarked, xOrder
      if the result is not empty then
         return "Error"
      end if
   end repeat
   __addToList pNode, xMarked
   __removeFromList pNode, xUnmarked
   __removeFromList pNode, xTemporaryMarked
   __addToList pNode, xOrder
end __visitNode

private function __dependencyOrder pDependencies, pList
   local tUnmarked, tTemporaryMarked, tMarked, tOrder, tToVisit
   put pList into tUnmarked
   repeat while tUnmarked is not empty
      put any line of tUnmarked into tToVisit
      __visitNode pDependencies, tToVisit, tUnmarked, tMarked, tTemporaryMarked, tOrder
      if the result is not empty then
         throw "Circularity in dependencies starting from" && tToVisit
         return empty
      end if
   end repeat
   return tOrder
end __dependencyOrder

private function extensionIsBuiltin pID
   switch pID
      case "com.livecode.widget"
      case "com.livecode.engine"
      case "com.livecode.canvas"
      case "com.livecode.foreign"
      case "com.livecode.arithmetic"
      case "com.livecode.array"
      case "com.livecode.assert"
      case "com.livecode.binary"
      case "com.livecode.bitwise"
      case "com.livecode.byte"
      case "com.livecode.char"
      case "com.livecode.codeunit"
      case "com.livecode.date"
      case "com.livecode.file"
      case "com.livecode.list"
      case "com.livecode.logic"
      case "com.livecode.mathfoundation"
      case "com.livecode.math"
      case "com.livecode.sort"
      case "com.livecode.stream"
      case "com.livecode.string"
      case "com.livecode.system"
      case "com.livecode.type"
      case "com.livecode.typeconvert"
      case "com.livecode.extensions.libbrowser"
      case "com.livecode.java"
      case "com.livecode.emscripten"      
      case "com.livecode.objc"
      case "com.livecode.commercial.license"
         return true
      default
         return false
   end switch
end extensionIsBuiltin

/**
Order extensions by dependency using requires info

Parameters:
pExtensions: A list of extensions to order, one per line
pRequiresA: 
An array of dependencies. For each key `tExtension`, 
`pRequiresA[tExtension]` should be a list of extensions on which 
`tExtension` depends

pIncludeBuiltin: 
Include built-in modules in dependency analysis. Defaults to false. 
For most purposes, it is not necessary to include built-in modules.

Returns:
The list of extensions ordered by dependency according to the 
dependency information in <pRequiresA>
*/
function extensionOrderByDependency pExtensions, pRequiresA, \
      pIncludeBuiltin
   # Accumulate an array of dependencies
   local tDependencies, tRequirements, tExtensions
   repeat for each line tExtension in pExtensions
      if not pIncludeBuiltin and extensionIsBuiltin(tExtension) then
         next repeat
      end if
      __addToList tExtension, tExtensions

      put pRequiresA[tExtension] into tRequirements
      repeat for each element tDependent in tRequirements
         if not pIncludeBuiltin and extensionIsBuiltin(tDependent) then
            next repeat  	 
         end if
         __addToDependencies tExtension, tDependent, tDependencies
      end repeat
   end repeat
   
   # Order them
   return __dependencyOrder(tDependencies, tExtensions)
end extensionOrderByDependency

private function __extensionResourcePath
   if the environment begins with "development" then
      if revEnvironmentIsInstalled() is false then
         return revEnvironmentBinariesPath()
      else
         return revIDESpecialFolderPath("Toolchain")
      end if
   else if the environment is "server" or the platform is not "macos" then
      return specialFolderPath("engine")
   else
      set the itemdelimiter to slash
      return item 1 to -4 of specialFolderPath("engine")
   end if
end __extensionResourcePath

private function __extensionCompilerPath
   # The actual compile command
   if the platform is "win32" then
      return __extensionResourcePath() & slash & "lc-compile.exe"
   else
      return __extensionResourcePath() & slash & "lc-compile"
   end if
end __extensionCompilerPath

private function shellFormat pArg, pSwitch
   local tOutput
   
   if pSwitch is not empty then
      put "--" & pSwitch & " " into tOutput
   end if
   
   return tOutput & q(pArg) & " "
end shellFormat

private command pushList @xList, pValue
   put pValue into xList[the number of elements in xList + 1]
end pushList

private function popList @xList
   local tValue
   put xList[the number of elements in xList] into tValue
   delete variable xList[the number of elements in xList]
   return tValue
end popList

local sHideListA
private command __PushHideConsoleWindows pValue
   pushList sHideListA, the hideConsoleWindows
   set the hideConsoleWindows to pValue
end __PushHideConsoleWindows

private command __PopHideConsoleWindows
   set the hideConsoleWindows to popList(sHideListA)
end __PopHideConsoleWindows

private command doShell pShellCmd, @rResult, @rOutput
   __PushHideConsoleWindows true
   
   put shell(pShellCmd) into rOutput
   put the result into rResult
   
   __PopHideConsoleWindows
end doShell

/**
Compile a LiveCode Builder extension file

Parameters:
pLCCompile: 
The path to the lc-compile program. If empty, this will be computed.

pFile: The main source file of the LCB extension
pSupportFiles: 
A list of support files required by the main source file. These must all be named `<main_source_name>-<suffix>.lcb`

pLCIPath: 
The location of the standard library module interface files. If empty, 
this will be computed.

pExtraLCIPaths:
A list of paths to look for extra module interface files.

pTargetFolder: The target output folder
pOutputFilename: The target bytecode output filename

The result: Any error that occurred.
It: The dependency-ordered list.
*/
command extensionCompile pLCCompile, pFile, pSupportFiles, pLCIPath, pExtraLCIPaths, pTargetFolder, pOutputFilename
   __EnsurePath pTargetFolder
   
   # Compute the output filename if empty
   if pOutputFilename is empty then
      put extensionBytecodeFilename(pLCCompile, true) \
            into pOutputFilename
   end if
   
   # The manifest is currently always generated from the source
   if there is a file (pTargetFolder & slash & "manifest.xml") then
      delete file (pTargetFolder & slash & "manifest.xml")
   end if
   
   # Build the shell command
   local tShellCommand
   if pLCCompile is empty then
      put __extensionCompilerPath() into pLCCompile
   end if
   put shellFormat(pLCCompile) into tShellCommand
   
   # The folder to put the .lci file
   put shellFormat(pTargetFolder, "modulepath") after tShellCommand
   
   # The built-in module path
   if pLCIPath is empty then
      put __extensionResourcePath() & slash & "modules/lci" \
            into pLCIPath
   end if
   put shellFormat(pLCIPath, "modulepath") after tShellCommand
   
   # Installed module path
   repeat for each line tInterface in pExtraLCIPaths
      put shellFormat(tInterface, "modulepath") after tShellCommand
   end repeat
   
   # The manifest target
   put shellFormat(pTargetFolder & slash & "manifest.xml", "manifest") after tShellCommand
   
   # The output
   put shellFormat(pTargetFolder & slash & pOutputFilename, "output") after tShellCommand
   
   # Support files must be dependency-ordered
   if pSupportFiles is not empty then
      extensionDepsOrder pLCCompile, pSupportFiles
      if the result is not empty then
         return the result for error
      end if
      put it into pSupportFiles
   end if
   repeat for each line tSupport in pSupportFiles
      put shellFormat(tSupport) after tShellCommand
   end repeat
   
   # The target .lcb file
   put shellFormat(pFile) after tShellCommand
   
   local tShellResult, tShellOutput
   doShell tShellCommand, tShellResult, tShellOutput
   
   if tShellResult is 0 then
      return tShellOutput for value
   end if
   return tShellOutput for error
end extensionCompile

/**
Use lc-compile to order LiveCode Builder extensions by dependency.

Parameters:
pLCCompile: 
The path to the lc-compile program. If empty, this will be computed.

pExtensionList: 
A list of LiveCode Builder extensions to put in dependency order

The result: Any error that occurred.
It: The dependency-ordered list.
*/
command extensionDepsOrder pLCCompile, pExtensionList
   # Build the shell command
   local tShellCommand
   if pLCCompile is empty then
      put __extensionCompilerPath() into pLCCompile
   end if
   put shellFormat(pLCCompile) into tShellCommand
   
   # The deps mode
   put shellFormat("order", "deps") after tShellCommand
   
   # The args
   put shellFormat("--") after tShellCommand
   repeat for each line tExtension in pExtensionList
      put shellFormat(tExtension) after tShellCommand
   end repeat
   
   local tShellResult, tShellOutput
   doShell tShellCommand, tShellResult, tShellOutput
   
   if tShellResult is 0 then
      return tShellOutput for value
   end if
   return tShellOutput for error
end extensionDepsOrder

private command appendToStringList @xList, pValue
   if xList is empty then
      put pValue into xList
   else
      put return & pValue after xList
   end if
end appendToStringList

/**
Build, package and extract LiveCode Builder extensions

Parameters:
pDocsParser: The path to the docs parser script.
pTargetFolder: The target folder to extract the extension to
pCommercialExtension: Whether the extension source should be removed
pLCCompile: 
The path to the lc-compile program. If empty, this will be computed.

pLCIPath: 
The location of the standard library module interface files. If empty, 
this will be computed.

pExtraLCIPaths:
A list of paths to look for extra module interface files.

Description:
Any parameters after <pExtraLCIPaths> are the extension sources to
build, package and extract.
*/
command extensionBuildPackageAndExtractLCB pDocsParser, pTargetFolder, \
      pCommercialExtension, pLCCompile, pLCIPath, pExtraLCIPaths
   if pLCCompile is empty then
      put __extensionCompilerPath() into pLCCompile
   end if
   
   # Items 7 onwards in 'the params' are quoted full paths to ext files
   # Construct the list of extensions to build
   local tExtList
   repeat for each item tItem in item 7 to -1 of the params
      appendToStringList tExtList, char 2 to -2 of tItem
   end repeat
   
   extensionDepsOrder pLCCompile, tExtList
   if the result is not empty then
      throw the result
      exit extensionBuildPackageAndExtractLCB
   end if
   
   put word 1 to -1 of it into tExtList
   
   set the itemdelimiter to slash
   repeat for each line tExtPath in tExtList
      local tSourceFileName, tSourceFolder
      put item 1 to -2 of tExtPath into tSourceFolder
      put item -1 of tExtPath into tSourceFileName
      extensionBuildPackageAndExtract "", "", "", \
            pDocsParser, \
            tSourceFolder, tSourceFileName, pTargetFolder, \
            pCommercialExtension, pLCCompile, pLCIPath, pExtraLCIPaths
   end repeat
end extensionBuildPackageAndExtractLCB

/**
Build, package and extract extension

Parameters:
pProtectifyScript: 
The path to the script that protects a livecode stack.

pProtectifyOutput: The target filename for the protected livecode stack
pGitHash: The hash to use when protecting the stack
pDocsParser: The path to the docs parser script.
pSourceFolder: The folder containing the extension
pSourceFileName: The extension file name
pTargetFolder: The target folder to extract the extension to
pCommercialExtension: Whether the extension source should be removed
pLCCompile: 
The path to the lc-compile program. If empty, this will be computed.

pLCIPath: 
The location of the standard library module interface files. If empty, 
this will be computed.

pExtraLCIPaths:
A list of paths to look for extra module interface files.
*/
command extensionBuildPackageAndExtract pProtectifyScript, \
      pProtectifyOutput, pGitHash, pDocsParser, \
      pSourceFolder, pSourceFileName, pTargetFolder, \
      pCommercialExtension, pLCCompile, pLCIPath, pExtraLCIPaths
   
   if pCommercialExtension is true and \
         not (pSourceFileName ends with ".lcb") then
      extensionProtectStack pProtectifyScript, pSourceFolder, \ 
            pSourceFileName, pProtectifyOutput, pGitHash
   end if
   
   extensionExtractDocs pDocsParser, pSourceFolder, \
         pSourceFolder & slash & pSourceFileName
   
   local tPackage
   if not (pSourceFileName ends with ".lcb") then
      extensionGenerateManifestForLCSExtension pDocsParser, pSourceFolder
      extensionPackageScriptExtension pSourceFolder, \
            pTargetFolder, pCommercialExtension
   else
      extensionPackageBuilderExtension pSourceFolder, \
            pTargetFolder, pCommercialExtension, pLCCompile, \
            pLCIPath, pExtraLCIPaths
   end if
   put the result into tPackage
   
   extractExtension tPackage, pSourceFolder, pTargetFolder
   delete file tPackage
end extensionBuildPackageAndExtract

private function q pParam
   return quote & pParam & quote
end q

/**
Protect a livecode stack using the provided script

Parameters:
pProtectifyScript: 
The path to the script that protects a livecode stack.

pSourceFolder: The folder containing the extension
pSourceFileName: The extension file name
pOutput: The target filename for the protected livecode stack
pGitHash: The hash to use when protecting the stack
*/
command extensionProtectStack pProtectifyScript, pSourceFolder, pSourceFileName, pOutput, pGitHash
   if there is not a stack pProtectifyScript then
      throw "Error: could not load script protector:" && pProtectifyScript
   end if
   
   try
      dispatch "protectifyStack" to stack pProtectifyScript with \
            pSourceFolder & slash & pSourceFileName, pOutput, pGitHash
   catch tError
      throw "Error: could not protect stack:" && pSourceFolder & slash & pSourceFileName & \
            return & tError
   end try
end extensionProtectStack

/**
Extract docs from extension files

Parameters:
pDocsParser: 
The path to the docs parser script. If empty, this command will do 
nothing

pOutputDir: The folder to output the extracted docs file

Description:
Use <extensionExtractDocs> to run the docs parser script on all the files which
are parameters subsequent to <pOutputDir>
*/
command extensionExtractDocs pDocsParser, pOutputDir
   if there is not a stack pDocsParser then
      exit extensionExtractDocs
   end if
   
   start using stack pDocsParser
   
   __EnsurePath pOutputDir
   
   if there is not a folder pOutputDir then
      throw "Error: could not create output directory: " & pOutputDir
   end if
   
   # Items 3 onwards in 'the params' are quoted full paths to source files
   local tSourceFiles
   repeat for each item tItem in item 3 to -1 of the params
      appendToStringList tSourceFiles, char 2 to -2 of tItem
   end repeat
   
   repeat for each line tSourceFile in tSourceFiles
      local tName, tSource, tContent, tDocs
      
      if tSourceFile ends with "lcb" then
         put "api" into tName
         put revDocsGenerateDocsFileFromModularFile(tSourceFile) into tDocs
      else
         if there is a stack tSourceFile then
            put "api" into tName
            put the script of stack tSourceFile into tContent
            put the long id of stack tSourceFile into tSource
         else if tSourceFile ends with ".lcidl" then
            if the platform is "Win32" then
               set the itemdelimiter to backslash
            else
               set the itemdelimiter to slash
            end if
            put "com.livecode.external." & tolower(\
                  char 1 to -7 of the last item of tSourceFile) into tName
            put textDecode(url ("binfile:" & tSourceFile), "UTF8") into tContent
            put tSourceFile into tSource
         else
            throw "Error: stack not found: " & tSourceFile
         end if
         put revDocsGenerateDocsFileFromText(tContent, tSource) into tDocs
      end if
      
      put textEncode(tDocs, "UTF-8") into tDocs
      
      replace numToByte(13) with numToByte(10) in tDocs
      
      put tDocs into url ("binfile:" & pOutputDir & "/" & tName & ".lcdoc")
   end repeat
end extensionExtractDocs

constant kFileKey = "file"
constant kSourceFileKey = "source_file"
constant kSupportFilesKey = "support_files"
constant kSourceTypeKey = "source_type"
constant kNameKey = "name"
constant kVersionKey = "version"
constant kAuthorKey = "author"
constant kTitleKey = "title"
constant kPlatformsKey = "platforms"
constant kOsKey = "os"
constant kTypeKey = "type"
constant kIdeKey = "ide"
constant kModuleFilesKey = "module_files"
constant kIconKey = "icon"
constant kRetinaIconKey = "retina_icon"
constant kGuideFileKey = "guide_file"
constant kGuideContentsKey = "guide_contents"
constant kApiFileKey = "api_file"
constant kResourcesKey = "resources"
constant kCodeKey = "code"
constant kSamplesKey = "samples"
constant kInterfaceKey = "interface"
constant kDefaultScriptFileKey = "defaultscript_file"
constant kEditorsKey = "editors"

private command __extensionBuildPackage pDetailsA, pFolder, pTargetFolder, pRemoveSource, @rBuiltLocation
   local tFullPath
   put pFolder & slash & pDetailsA[kFileKey] into tFullPath
   
   if there is not a file tFullPath then
      return "Error: missing file" && tFullPath
   end if
   
   local tManifestFile
   put pFolder & slash & "manifest.xml" into tManifestFile
   
   if there is no file tManifestFile then
      return "Error: missing manifest" && tManifestFile
   end if
   
   local tTargetFolder
   if pTargetFolder is empty then
      put pFolder into tTargetFolder
   else
      put pTargetFolder into tTargetFolder
   end if
   
   __EnsurePath tTargetFolder
   
   local tTargetName
   if pDetailsA[kNameKey] is empty then
      return "no id found in " && tFullpath
   end if
   put pDetailsA[kNameKey] into tTargetName
   
   local tTargetVersion
   if pDetailsA[kVersionKey] is empty then
      return "no version found in " && tFullpath
   end if
   put pDetailsA[kVersionKey] into tTargetVersion
   
   local tArchive
   put tTargetFolder & slash & tTargetName & "." & \
         tTargetVersion & ".lce" into tArchive
   
   if there is a file tArchive then
      delete file tArchive
   end if
   revZipOpenArchive tArchive, "write"
   
   if the result begins with "ziperr" then
      return "couldn't open zip archive" && tArchive
   end if
   
   local tHaveSvgIcon
   put ("svgicon" is among the keys of pDetailsA) into tHaveSvgIcon
   
   # Don't include source if not required
   local tSourceFile, tSupportFiles
   if not pRemoveSource then
      put tFullPath into tSourceFile
      put pDetailsA[kSupportFilesKey] into tSupportFiles
   end if
   
   local tError
   put __extensionAddFilesToPackage(\
         tSourceFile, tSupportFiles, pFolder, tArchive, \
         pDetailsA) into tError
   
   revZipCloseArchive tArchive
   
   if tError is empty then
      if the result begins with "ziperr" then
         put "Error: couldn't close zip archive" into tError
      end if
   end if
   
   if tError is not empty then
      return tError
   end if
   
   put tArchive into rBuiltLocation
   return empty
end __extensionBuildPackage

private command __extensionAddFolder pArchive, pBaseFolder, pFolderName, pResourcesList
   repeat for each line tLine in pResourcesList
      revZipAddItemWithFile pArchive, pFolderName& slash & tLine, \
            pBaseFolder & slash & pFolderName & slash & tLine
      
      if the result begins with "ziperr" then
         return "couldn't add" && pFolderName & ":" && tLine
      end if
   end repeat
   return empty
end __extensionAddFolder

private function __extensionAddFilesToPackage \
      pSource, pSupportFiles, pFolder, pArchive, pDetailsA
   
   set the itemdelimiter to slash
   # Add source into package, if present
   if pSource is not empty then
      revZipAddItemWithFile pArchive, item -1 of pSource, pSource
      
      if the result begins with "ziperr" then
         return "couldn't add source"
      end if
   end if
   
   # Add support files into package
   repeat for each line tSupport in pSupportFiles
      put pFolder & slash & tSupport into tSupport
      revZipAddItemWithFile pArchive, item -1 of tSupport, tSupport
      
      if the result begins with "ziperr" then
         return "couldn't add support file" && tSupport
      end if
   end repeat
   
   # Add modules into package
   repeat for each line tModuleFile in pDetailsA[kModuleFilesKey]
      put pFolder & slash & tModuleFile into tModuleFile
      revZipAddItemWithFile pArchive, item -1 of tModuleFile, tModuleFile
      if the result begins with "ziperr" then
         return "couldn't add module" && tModuleFile
      end if
   end repeat
   
   # Add manifest into package
   revZipAddItemWithFile pArchive, "manifest.xml", pFolder & slash & "manifest.xml"
   
   if the result begins with "ziperr" then
      return "couldn't add manifest"
   end if
   
   # Add user guide
   if pDetailsA[kGuideContentsKey] is not empty then
      __extensionAddFolder pArchive, pFolder, "docs/guide", pDetailsA[kGuideContentsKey]
   else if pDetailsA[kGuideFileKey] is not empty then
      revZipAddItemWithFile pArchive, "docs/guide/guide.md", pDetailsA[kGuideFileKey]
   end if
   if the result begins with "ziperr" then
      return "couldn't add guide"
   end if
   
   # Add icons
   if pDetailsA[kIconKey] is not empty then
      revZipAddItemWithFile pArchive, "support/icon.png", pDetailsA[kIconKey]
      
      if the result begins with "ziperr" then
         return "couldn't add icon.png"
      end if
   end if
   
   if pDetailsA[kRetinaIconKey] is not empty then
      revZipAddItemWithFile pArchive, "support/icon@extra-high.png", pDetailsA[kRetinaIconKey]
      
      if the result begins with "ziperr" then
         return "couldn't add icon@extra-high.png"
      end if
   end if
   
   # Add docs
   if pDetailsA[kApiFileKey] is not empty then
      revZipAddItemWithFile pArchive, "api.lcdoc", pDetailsA[kApiFileKey]
   end if
   if the result begins with "ziperr" then
      return "couldn't add doc" && pDetailsA[kApiFileKey]
   end if
   
   # Add resources
   __extensionAddFolder pArchive, pFolder, "resources", pDetailsA[kResourcesKey]
   if the result is not empty then
      return the result
   end if
   
   # Add code
   __extensionAddFolder pArchive, pFolder, "code", pDetailsA[kCodeKey]
   if the result is not empty then
      return the result
   end if
   
   # Add sample stacks
   __extensionAddFolder pArchive, pFolder, "samples", pDetailsA[kSamplesKey]
   if the result is not empty then
      return the result
   end if
   
   # AL-2015-03-18: [[ Bug 15007 ]] Include interface file if present
   if pDetailsA[kInterfaceKey] is not empty then
      revZipAddItemWithFile pArchive, item -1 of pDetailsA[kInterfaceKey], pDetailsA[kInterfaceKey]
      
      if the result begins with "ziperr" then
         return "couldn't add interface file" && pDetailsA[kInterfaceKey]
      end if
   end if
   
   # Add default script
   if pDetailsA[kDefaultScriptFileKey] is not empty then
      revZipAddItemWithFile pArchive, "support/defaultscript.livecodescript", pDetailsA[kDefaultScriptFileKey]
      
      if the result begins with "ziperr" then
         return "couldn't add default script" && pDetailsA[kDefaultScriptFileKey]
      end if
   end if
   
   # Add editors
   __extensionAddFolder pArchive, pFolder, "editors", pDetailsA[kEditorsKey]
   if the result is not empty then
      return the result
   end if
   
   return empty
end __extensionAddFilesToPackage

private function __extensionListResourcesRecursive pFolder, pPrefix
   local tResources, tResource, tPrefix
   if pPrefix is not empty then
      put pPrefix & slash into tPrefix
   end if
   
   # Add files to the list of resources
   repeat for each line tResource in files(pFolder)
      if tResource begins with "." then next repeat
      if tResources is empty then
         put tPrefix & tResource into tResources
      else
         put return & tPrefix & tResource after tResources
      end if
   end repeat
   
   # Add folders to the list of resources
   repeat for each line tResourceFolder in folders(pFolder)
      if tResourceFolder begins with "." then next repeat
      local tRecursive
      put __extensionListResourcesRecursive(pFolder & slash & tResourceFolder, tPrefix & tResourceFolder) into tRecursive
      if tRecursive is not empty then
         if tResources is empty then
            put tRecursive into tResources
         else
            put return & tRecursive after tResources
         end if
      end if
   end repeat
   return tResources
end __extensionListResourcesRecursive

/**
Fetch all the information about an extension in the given folder

Parameters:
pFolder: The folder to search for an extension
pParseManifest: 
Whether to parse any existing manifest file for information about the
extension.

Returns:
An array of details about any extension found, and its assets, or empty
if none was found.
*/
function extensionFetchFolderDetails pFolder, pParseManifest
   local tMainFile, tType, tSupportFiles
   extensionFetchSourceFromFolder pFolder, tMainFile, tSupportFiles, tType
   
   if tType is "none" then
      return empty
   end if
   
   local tDetailsA
   put tMainFile into tDetailsA[kFileKey]
   put tSupportFiles into tDetailsA[kSupportFilesKey]
   put tType into tDetailsA[kSourceTypeKey]
   
   local tAPIFolder
   if there is a folder (pFolder & slash & "docs" & slash & "api") then
      put pFolder & slash & "docs" & slash & "api" into tAPIFolder
   else
      put pFolder into tAPIFolder
   end if
   
   put tAPIFolder & slash & "api.lcdoc" into tDetailsA[kApiFileKey]
   
   if there is a file (pFolder & slash & "support" & slash & "icon.png") then
      put (pFolder & slash & "support" & slash & "icon.png") into tDetailsA[kIconKey]
   else if there is a file  (pFolder & slash & "icon.png") then
      put (pFolder & slash & "icon.png") into tDetailsA[kIconKey]
   else
      put empty into tDetailsA[kIconKey]
   end if
   
   if there is a file (pFolder & slash & "support" & slash & "icon@extra-high.png") then
      put (pFolder & slash & "support" & slash & "icon@extra-high.png") into tDetailsA[kRetinaIconKey]
   else if there is a file (pFolder & slash & "icon@extra-high.png") then
      put (pFolder & slash & "icon@extra-high.png") into tDetailsA[kRetinaIconKey]
   else
      put empty into tDetailsA[kRetinaIconKey]
   end if
   
   local tGuide, tGuideFile, tGuideFolder
   put pFolder & slash & "docs" & slash & "guide" into tGuideFolder
   if there is a file (tGuideFolder & slash & "guide.md") then
      put __extensionListResourcesRecursive(tGuideFolder, "") \
            into tDetailsA[kGuideContentsKey]
      put tGuideFolder & slash & "guide.md" into tDetailsA[kGuideFileKey]
   else if there is a file (pFolder & slash & "guide.md") then
      put pFolder & slash & "guide.md" into tDetailsA[kGuideFileKey]
   end if
   
   // AL-2015-02-23: [[ Bug 14552 ]] Find resource folder and return details
   local tResourceFolder, tResources
   put pFolder & slash & "resources" into tResourceFolder
   if there is a folder tResourceFolder then
      put __extensionListResourcesRecursive(tResourceFolder, "") into tResources
      put tResources into tDetailsA[kResourcesKey]
   end if
   
   local tCodeFolder, tCode
   put pFolder & slash & "code" into tCodeFolder
   if there is a folder tCodeFolder then
      put __extensionListResourcesRecursive(tCodeFolder, "") into tCode
      put tCode into tDetailsA[kCodeKey]
   end if
   
   local tSamplesFolder, tSamples
   put pFolder & slash & "samples" into tSamplesFolder
   if there is a folder tSamplesFolder then
      put __extensionListResourcesRecursive(tSamplesFolder, "") into tSamples
      put tSamples into tDetailsA[kSamplesKey]
   end if
   
   # Default script
   local tDefaultScriptPath
   put pFolder & "/support/defaultscript.livecodescript" into tDefaultScriptPath
   if there is a stack tDefaultScriptPath then
      put pFolder & "/support/defaultscript.livecodescript" into tDetailsA[kDefaultScriptFileKey]
   end if
   
   # Custom PI editors
   local tEditorsFolder, tEditors
   put pFolder & slash & "editors" into tEditorsFolder
   if there is a folder tEditorsFolder then
      put __extensionListResourcesRecursive(tEditorsFolder, "") into tEditors
      put tEditors into tDetailsA[kEditorsKey]
   end if
   
   # Compiled module files
   local tModuleFiles
   if tDetailsA[kSourceTypeKey] is "lcb" then
      put files(pFolder) into tModuleFiles
      filter tModuleFiles with regex pattern "^module(\.[0-9]+)?\.lcm$"
      put tModuleFiles into tDetailsA[kModuleFilesKey]
   end if
   
   # Fetch data from manifest
   local tManifest
   put pFolder & slash & "manifest.xml" into tManifest
   if pParseManifest is not false and \
         there is a file tManifest then
      union tDetailsA with extensionFetchMetadata(tManifest)
   end if
   
   # Now we have the name, add the interface file
   local tInterfaceFile
   put pFolder & slash & tDetailsA[kNameKey] & ".lci" into tInterfaceFile
   if there is a file tInterfaceFile then
      put tInterfaceFile into tDetailsA[kInterfaceKey]
   end if
   
   return tDetailsA 
end extensionFetchFolderDetails

/**
Package an extension

Parameters:
pSourceFolder: The folder containing the extension
pTargetFolder: The output location of the package
pRemoveSource: Whether to remove the source code from the package or not

The result: Any error that occurred when packaging
It: The location of the build package, or empty if there was an error.
*/
command extensionPackage pSourceFolder, pTargetFolder, pRemoveSource
   __EnsureExternal "revzip"
   local tDetailsA
   put extensionFetchFolderDetails(pSourceFolder) into tDetailsA
   
   local tTargetDir, tBuildTarget, tZip
   put tDetailsA[kNameKey] into tTargetDir
   put pTargetFolder & slash & tTargetDir into tBuildTarget
   
   __extensionBuildPackage tDetailsA, pSourceFolder, pTargetFolder, \
         pRemoveSource, tZip
   if the result is not empty then
      return the result for error
   end if
   
   return tZip for value
end extensionPackage

private command __EnsurePath pPath
   if there is a folder pPath then exit __EnsurePath
   
   set the itemdelimiter to slash
   
   local tPathPart
   repeat for each item tPart in pPath
      put tPart after tPathPart
      if there is not a folder tPathPart then
         create folder tPathPart
      end if
      put slash after tPathPart
   end repeat
end __EnsurePath

private command extractExtension pZip, pSourceFolder, pTargetFolder
   __EnsureExternal "revzip"
   
   local tDetailsA
   put extensionFetchFolderDetails(pSourceFolder) into tDetailsA
   
   local tTargetDir, tBuildTarget, tError, tName
   put tDetailsA[kNameKey] into tName
   put pTargetFolder & slash & tName into tBuildTarget
   
   __EnsurePath tBuildTarget
   
   # Extract the icon,docs and executable module into the folder
   revZipOpenArchive pZip, "read"
   if the result begins with "ziperr" then
      put the result into tError
   end if
   
   local tZipItems
   if tError is empty then
      put revZipEnumerateItems(pZip) into tZipItems
      if the result begins with "ziperr" then
         put the result into tError
      end if
   end if
   
   # Extract all the files
   local tDirectory
   set the itemdelimiter to slash
   repeat for each line tFile in tZipItems
      put tBuildTarget & slash & item 1 to -2 of tFile into tDirectory
      __EnsurePath tDirectory, slash
      revZipExtractItemToFile pZip, tFile, tBuildTarget & slash & tFile
      if the result begins with "ziperr" then
         put the result into tError
      end if
   end repeat 
   
   if tError is empty then
      revZipCloseArchive pZip
      if the result begins with "ziperr" then
         put the result into tError
      end if
   end if
   
   if tError is not empty then
      throw "zip error when extracting extension" && tName & ":" && tError
   end if
end extractExtension

/**
Package a LiveCode Script extension

Parameters:
pSourceFolder: The folder containing the extension
pTargetFolder: The folder to output the extension package
pRemoveSource: Whether to remove the source files or not
*/
command extensionPackageScriptExtension pSourceFolder, \
      pTargetFolder, pRemoveSource   
   extensionPackage pSourceFolder, pTargetFolder, pRemoveSource
   if the result is not empty then
      throw the result
   end if
   return it
end extensionPackageScriptExtension

/**
Package a LiveCode Builder extension

Parameters:
pSourceFolder: The folder containing the extension
pTargetFolder: The folder to output the extension package
pRemoveSource: Whether to remove the source files or not
pLCCompile: 
The path to the lc-compile program. If empty, this will be computed.

pLCIPath: 
The location of the standard library module interface files. If empty, 
this will be computed.

pExtraLCIPaths:
A list of paths to look for extra module interface files.
*/
command extensionPackageBuilderExtension pSourceFolder, \
      pTargetFolder, pRemoveSource, pLCCompile, \
      pLCIPath, pExtraLCIPaths
   if pLCCompile is empty then
      put __extensionCompilerPath() into pLCCompile
   end if
   
   # The built-in module path
   if pLCIPath is empty then
      put __extensionResourcePath() & slash & "modules/lci" \
            into pLCIPath
   end if
   
   local tCompileShell
   put shellFormat(pLCCompile) into tCompileShell
   put shellFormat("-Werror") after tCompileShell
   put shellFormat(pLCIPath, "modulepath") after tCompileShell
   
   # Installed module path
   repeat for each line tInterface in pExtraLCIPaths
      put shellFormat(tInterface, "modulepath") after tCompileShell
   end repeat
   
   put shellFormat(pSourceFolder & slash & "manifest.xml", "manifest") \
         after tCompileShell
   put shellFormat(pSourceFolder & slash & "module.lcm", "output") \
         after tCompileShell
   
   local tSource, tSupportFiles, tType
   extensionFetchSourceFromFolder pSourceFolder, tSource, tSupportFiles, tType
   
   -- Multi-module assemblies must be in reverse dependency order
   local tDepsShell
   	
   if tSupportFiles is not empty then
      put shellFormat(pLCCompile) into tDepsShell
      put shellFormat("order", "deps") after tDepsShell
      repeat for each line tSupportModule in tSupportFiles
         put shellFormat(pSourceFolder & slash & tSupportModule, "") \ 
               after tDepsShell
      end repeat

      local tDepsResult, tDepsOutput
      doShell tDepsShell, tDepsResult, tDepsOutput
      
      if tDepsResult is not 0 then
         throw "subprocess error when finding support deps order for" && tSource & ":" && tDepsOutput
      end if  

      repeat for each line tSupportModule in tDepsOutput
         put shellFormat(tSupportModule, "") after tCompileShell
      end repeat
   end if
   	
   put shellFormat(pSourceFolder & slash & tSource, "") \
         after tCompileShell

   local tShellResult, tShellOutput
   doShell tCompileShell, tShellResult, tShellOutput
   if tShellResult is not 0 then
      throw "subprocess error when compiling extension" && tSource & ":" && tShellOutput
   end if    
   
   extensionPackage pSourceFolder, pTargetFolder, pRemoveSource
   if the result is not empty then
      throw the result
   end if
   return it
end extensionPackageBuilderExtension

private command addXML @xXML, pDepth, pTag, pContent, pAttributesA
   repeat 2*pDepth
      put space after xXML
   end repeat
   put "<" & pTag after xXML
   repeat for each key tKey in pAttributesA
      put " " & tKey & "=" & quote & pAttributesA[tKey] & quote \
            after xXML
   end repeat
   if pContent is empty then
      put " />" after xXML
   else
      put ">" & pContent & "</" & pTag & ">" after xXML
   end if
   put return after xXML
end addXML

/**
Generate manifest for LiveCode Script extension

Parameters:
pDocsParser: The path to the docs parser script.
pSourceFolder: The folder in which to find the lcs extension
*/
command extensionGenerateManifestForLCSExtension pDocsParser, pSourceFolder
   __EnsureExternal "revxml"

   # Get library metadata from the docs, in lieu of a manifest
   local tDocA, tDataA
   dispatch function "revDocsParseDocFileToLibraryArray" to stack pDocsParser \
         with (pSourceFolder & slash & "api.lcdoc")
   put the result into tDocA 
   
   set the itemdelimiter to comma
   repeat for each item tKey in "title,author,version,name,type,display name,requires,uses,svgicon"
      put tDocA[tKey] into tDataA[tKey]
   end repeat
   if tDataA[kTitleKey] is empty then
      put tDataA["display name"] into tDataA["title"]
   end if
   
   local tPackageXML, tAttrA
   repeat for each item tTag in "name,title,author,version,type"
      addXML tPackageXML, 1, tTag, tDataA[tTag]
   end repeat
   
   addXML tPackageXML, 1, "license", "community"
   
   repeat for each element tRequirement in tDataA["requires"]
      put tRequirement into tAttrA["name"]
      addXML tPackageXML, 1, "requires", "", tAttrA
      put empty into tAttrA
   end repeat
   
   repeat for each item tMetadata in "svgicon"
      local tValue
      put tDataA[tMetadata] into tValue
      if tValue is empty then
         next repeat
      end if
      put tMetadata into tAttrA["key"]
      addXML tPackageXML, 1, "metadata", tValue, tAttrA
      put empty into tAttrA
   end repeat
   
   local tXML
   put "0.0" into tAttrA["version"]
   addXML tXML, 0, "package", return & tPackageXML, tAttrA
   
   log "writing manifest to" && pSourceFolder & slash & "manifest.xml"
   put textEncode(tXML, "utf-8") into url("binfile:" & pSourceFolder & slash & "manifest.xml")
end extensionGenerateManifestForLCSExtension

private command __SetMetadata pKey, pValue, @xArray
   split pKey by "."
   put pValue into xArray[pKey]
end __SetMetadata

/**
Fetch metadata from an extension manifest

Parameters:
pManifestPath: The path to the manifest XML file
*/
function extensionFetchMetadata pManifestPath
   __EnsureExternal "revxml"

   local tDataA
   
   local tManifestContents, tTreeID
   put url ("file:" & pManifestPath) into tManifestContents
   put revXMLCreateTree(tManifestContents,true,true,false) into tTreeID
   
   if tTreeID begins with "xmlerr" then
      return "Error: invalid xml in manifest" for error
   end if
   
   local tTargetType
   put textDecode(revXMLNodeContents(tTreeID, "/package/type"), "utf-8") into tTargetType
   
   if tTargetType begins with "xmlerr" then
      return "Error: no type found in manifest" for error
   end if
   put tTargetType into tDataA[kTypeKey]
   
   local tTargetName
   put textDecode(revXMLNodeContents(tTreeID, "/package/name"), "utf-8") into tTargetName
   if tTargetName begins with "xmlerr" then
      return "Error: couldn't retrieve extension name from manifest" for error
   end if
   put tTargetName into tDataA[kNameKey]
   
   local tTargetTitle
   put textDecode(revXMLNodeContents(tTreeID, "/package/title"), "utf-8") into tTargetTitle
   
   if tTargetTitle is empty or tTargetTitle begins with "xmlerr" then
      put empty into tTargetTitle
   end if
   put tTargetTitle into tDataA[kTitleKey]
   
   local tTargetVersion
   put textDecode(revXMLNodeContents(tTreeID, "/package/version"), "utf-8") into tTargetVersion
   
   if tTargetVersion is empty or tTargetVersion begins with "xmlerr" then
      put empty into tTargetVersion
   end if
   put tTargetVersion into tDataA[kVersionKey]
   
   local tTargetAuthor
   put textDecode(revXMLNodeContents(tTreeID, "/package/author"), "utf-8") into tTargetAuthor
   if tTargetAuthor is empty or tTargetAuthor begins with "xmlerr" then
      put empty into tTargetAuthor
   end if
   put tTargetAuthor into tDataA[kAuthorKey]
   
   local tTargetPlatforms
   put textDecode(revXMLNodeContents(tTreeID, "/package/platforms"), "utf-8") into tTargetPlatforms
   if tTargetPlatforms is empty or tTargetPlatforms begins with "xmlerr" then
      put empty into tTargetPlatforms
   end if
   put tTargetPlatforms into tDataA[kPlatformsKey]
   
   local tTargetOs
   put textDecode(revXMLNodeContents(tTreeID, "/package/os"), "utf-8") into tTargetOs
   if tTargetOs is empty or tTargetOs begins with "xmlerr" then
      put empty into tTargetOs
   end if
   put tTargetOs into tDataA[kOsKey]
   
   # Fetch property metadata
   local tMetadataValue, tMetadataKey
   local tMetadataNodes, tMetadataA, tKeys, tValue
   put revXMLChildNames(tTreeID, "package",return,"metadata",true) into tMetadataNodes
   repeat for each line tMetadata in tMetadataNodes
      put revXMLAttribute(tTreeID,"package" & "/" & tMetadata,"key") into tMetadataKey
      put revXMLNodeContents(tTreeID,"package" & "/" & tMetadata) into tMetadataValue
      if tMetadataValue is empty or tMetadataValue begins with "xmlerr" then
         put empty into tMetadataValue
      end if
      __SetMetadata tMetadataKey, tMetadataValue, tDataA
   end repeat
   
   # Fetch extension dependencies
   local tRequires, tCount
   put 0 into tCount
   put revXMLChildNames(tTreeID,"package",return,"requires",true) into tRequires
   repeat for each line tDependency in tRequires
      add 1 to tCount
      local tName
      put revXMLAttribute(tTreeID,"package" & "/" & tDependency,"name") into tName
      put tName into tDataA["requires"][tCount]
   end repeat
   
   # Fetch library handlers
   if tTargetType is "library" then
      local tHandlerList, tHandlerNodes
      put revXMLChildNames(tTreeID, "package",return,"handler",true) into tHandlerNodes
      repeat for each line tHandler in tHandlerNodes
         local tHandlerName
         put revXMLAttribute(tTreeID,"package" & "/" & tHandler,"name") into tHandlerName
         
         if tHandlerName is empty or tHandlerName begins with "xmlerr" then
            next repeat
         end if
         
         if tHandlerList is empty then
            put tHandlerName into tHandlerList
         else
            put return & tHandlerName after tHandlerList
         end if
      end repeat
      put tHandlerList into tDataA["handlers"]
   end if
   
   # User visible should default to true
   if tDataA["uservisible"] is empty then
      put true into tDataA["uservisible"]
   end if
   
   return tDataA for value
end extensionFetchMetadata

private function __fetchExtensionManifestData pFolder, pExtFile
   local tDataA
   put extensionFetchMetadata(pFolder & slash & "manifest.xml") into tDataA
   
   # If we couldn't fetch a type id, or there was no manifest then 
   # the result will not be empty
   if the result is not empty then
      put "Invalid manifest" into tDataA["error"]
      put "error" into tDataA["status"]
   end if
   return tDataA
end __fetchExtensionManifestData

/**
Find any extensions in the given folder

Parameters:
pFolder: The folder to search
pIsUserFolder: Whether this is a user folder or not
pRecursive: Whether to search subfolders
xDataA: 
An array. Details of any extensions found will be placed in the 
array under xDataA[<type_id>][<folder>]
*/
command extensionFindInFolder pFolder, pIsUserFolder, pRecursive, @xDataA
   local tExtSource, tArray, tFiles, tMainSource, tSupportFiles, tType
   extensionFetchSourceFromFolder pFolder, tMainSource, tSupportFiles, tType
   
   -- If we found no extensions, recurse
   if the result is not empty or tType is "none" then
      if pRecursive then
         repeat for each line tSubFolder in folders(pFolder)
            if tSubFolder begins with "." then next repeat
            extensionFindInFolder pFolder & slash & tSubFolder, \
               pIsUserFolder, true, xDataA
         end repeat
      end if
      exit extensionFindInFolder
   end if
   
   put __fetchExtensionManifestData(pFolder, tMainSource) into tArray
   
   put tMainSource into tArray[kSourceFileKey]
   put tSupportFiles into tArray[kSupportFilesKey]
   put tType into tArray[kSourceTypeKey]
   put not pIsUserFolder into tArray[kIdeKey]
   
   local tTypeID
   if tArray[kNameKey] is empty then
      put pFolder into tTypeID
   else
      put tArray[kNameKey] into tTypeID
   end if
   put tArray into xDataA[tTypeID][pFolder]
end extensionFindInFolder

private function __RegexPatternForSource pType
   switch pType
      case "lcb"
         return "^.*\.lcb$"
         break
      case "lcs"
         return "^.*\.livecode(script)?$"
         break
   end switch
end __RegexPatternForSource

private command __FilterFilesForExtension pFiles, @rResult, @rType
   local tExtFiles, tType
   filter pFiles with regex pattern \
         __RegexPatternForSource("lcb") into tExtFiles
   if tExtFiles is not empty \
         or extensionBytecodeFilename("", true) is among the lines of pFiles \
         or extensionBytecodeFilename("", false) is among the lines of pFiles \
         then
      put "lcb" into tType
   end if
   
   if tType is empty then
      filter pFiles with regex pattern \
            __RegexPatternForSource("lcs") into tExtFiles
      if tExtFiles is not empty then
         put "lcs" into tType
      end if
   end if
   put tExtFiles into rResult
   put tType into rType
end __FilterFilesForExtension

/**
Fetch basic information about extension source from a folder

Parameters:
pFolder: The folder to search
rSource: The main source file found
rSupportFiles: A list of supporting source files found
rType (enum): The type of extension. 
- "lcb": A LiveCode Builder extension
- "lcs": A LiveCode Script extension
*/
command extensionFetchSourceFromFolder pFolder, @rSource, @rSupportFiles, @rType
   local tFiles
   put files(pFolder) into tFiles
   
   local tExtFiles, tType
   __FilterFilesForExtension tFiles, tExtFiles, tType
   
   -- Try and separate main source from support
   local tFoundCount, tSupport, tMainFile
   put the number of lines in tExtFiles into tFoundCount
   if tFoundCount > 1 then
      set the itemdelimiter to "."
      repeat for each line tTry in tExtFiles
         filter tExtFiles with item 1 to -2 of tTry & "-*" into tSupport
         if the number of lines in tSupport is tFoundCount - 1 then
            put tTry into tMainFile
            exit repeat
         end if
         put empty into tSupport
      end repeat
      if tSupport is empty then
         return "Invalid extension folder" && pFolder & return & \
               "Must have one main module, with all support modules named" && \
               "<main-name>-<support-name>."
      end if
   else
      put tExtFiles into tMainFile 
   end if
   
   if tType is empty then
      put "none" into tType
   end if 
   
   put tMainFile into rSource
   put tSupport into rSupportFiles
   put tType into rType
   return empty
end extensionFetchSourceFromFolder

/**
Validate an extension package

Parameters:
pPackage: The .lce package to validate
rManifestDataA: The basic manifest data extracted from the package

The result: 
If the package was deemed invalid, the result contains the reason why 
the package failed validation.

Description:
Use the <extensionValidateLCEPackage> command to naively validate the 
contents of an .lce package. <extensionValidateLCEPackage> checks if 
there is a file that looks like the main extension (either a compiled 
LiveCode Builder module file or a LiveCode stack), that it contains a
manifest XML file and that the manifest contains the minimum required
information, namely the name, version, author, type, and title.

The manifest information is returned in the <rManifestDataA> out 
parameter.
*/
command extensionValidateLCEPackage pPackage, @rManifestDataA
   # Open the zip archive
   revZipOpenArchive pPackage, "read"
   if the result is not empty then
      return pPackage && "is not a valid archive"
   end if
   
   # If we have a root folder in the archive, reject it 
   local tZipItems, tError
   put revZipEnumerateItems(pPackage) into tZipItems
   
   if the last char of line 1 of tZipItems is "/" then
      put "invalid archive structure - files must be at root" into tError
   end if
   
   # Ensure the zip contains all the required files
   if tError is empty then
      local tExtFiles, tType
      __FilterFilesForExtension tZipItems, tExtFiles, tType
      if tType is empty then
         put "invalid package - no stack or module found" into tError
      end if
   end if
   
   if tError is empty then
      if "manifest.xml" is not among the lines of tZipItems then
         put "invalid package - manifest not found" into tError
      end if
   end if
   
   # Get initial manifest data needed for loading
   local tManifestText, tManifestData, tManifestXMLTree
   
   # Extract the package manfiest to a variable to read key data
   if tError is empty then
      revZipExtractItemToVariable pPackage, "manifest.xml", \
            "tManifestText"
      if the result is not empty then
         put "unable to extract manifest data" into tError
      end if
   end if
   
   revZipCloseArchive pPackage
   
   if tError is not empty then
      return tError
   end if
   
   put revXMLCreateTree(tManifestText,true,true,false) into tManifestXMLTree
   
   if tManifestXMLTree is not a number then
      return "invalid manifest"
   end if
   
   # Check the manifest contains the required xml elements
   repeat for each item tItem in "name,version,author,type,title"
      put __extensionManifestValueFromTree(tManifestXMLTree, tItem) \
            into tManifestData[tItem]
      if the result is not empty then
         put "no" && tItem && "element found in package manifest" into tError
         exit repeat
      end if
   end repeat
   
   # Fetch the manifest requirements
   local tRequires
   put __extensionManifestValueFromTree(tManifestXMLTree, "requires") \
         into tRequires
   if tRequires is an array then
      put tRequires into tManifestData["requires"]
   end if
   
   revXMLDeleteTree tManifestXMLTree
   
   if tError is not empty then
      return tError
   end if
   
   put tManifestData into rManifestDataA
   return empty
end extensionValidateLCEPackage

private function __extensionManifestValueFromTree pTreeID, pProperty
   local tValue
   switch pProperty
      case "requires"
         # Fetch extension dependencies
         local tRequires, tCount
         put 0 into tCount
         put revXMLChildNames(pTreeID,"package",return,"requires",true) into tRequires
         if tRequires begins with "xmlerr" then
            put tRequires into tValue
            break
         end if
         repeat for each line tDependency in tRequires
            add 1 to tCount
            local tName
            put revXMLAttribute(pTreeID,"package" & "/" & tDependency,"name") into tName
            if tName begins with "xmlerr" then
               put tName into tValue
               break
            end if
            put tName into tValue[tCount]
         end repeat
         break
      default
         put revXMLNodeContents(pTreeID,"/package/" & pProperty) \
            into tValue
         break
   end switch
   
   if tValue begins with "xmlerr" then
      return tValue for error
   end if
   return tValue for value
end __extensionManifestValueFromTree
