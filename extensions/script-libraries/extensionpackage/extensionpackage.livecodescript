script "com.livecode.library.extension-package"
/*
This library provides handlers for managing livecode extension packages
*/

private command log pMsg
   write pMsg & return to stdout
end log

local sExternalsA
private command __EnsureExternal pExternal
   if sExternalsA[pExternal] then
      exit __EnsureExternal
   end if
   
   -- If we are running in the IDE we should have externals available
   if the environment begins with "development" then
      exit __EnsureExternal
   end if
   
   -- If we are running on server with an 'externals' folder, assume
   -- the externals are loaded
   if the environment is "server" then
      local tHomeFolder
      put $REV_HOME into tHomeFolder
      if tHomeFolder is empty then
         put $LIVECODE_SERVER_HOME into tHomeFolder
      end if
      if tHomeFolder is empty then
         put specialfolderpath("engine") into tHomeFolder
      end if
      if there is a folder (tHomeFolder & slash & "externals") then
         put true into sExternalsA[pExternal]
         exit __EnsureExternal
      end if
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
   put specialFolderPath("engine") & slash into tExternalFilename
   if the environment is "server" then
      put "server-" after tExternalFilename
   end if
   put pExternal after tExternalFilename
   put "." & tFileExtension after tExternalFilename
   set the externals of the templateStack to tExternalFilename
   
   create stack pExternal && "External"
   start using it
   if the externalCommands of it is empty then
      throw "cannot load external" && pExternal
   else
      put true into sExternalsA[pExternal]
   end if
end __EnsureExternal 

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

command extensionCompile pFile, pSupportFiles, pExtraModulePaths, pTargetFolder, pOutputFilename
   # The manifest is currently always generated from the source
   if there is a file (pTargetFolder & slash & "manifest.xml") then
      delete file (pTargetFolder & slash & "manifest.xml")
   end if
   
   # Build the shell command
   local tShellCommand
   put shellFormat(__extensionCompilerPath()) into tShellCommand
   
   # The folder to put the .lci file
   put shellFormat(pTargetFolder, "modulepath") after tShellCommand
   
   # The built-in module path
   put shellFormat(__extensionResourcePath() & slash & "modules/lci", "modulepath") after tShellCommand
   
   # Installed module path
   repeat for each line tInterface in pExtraModulePaths
      put shellFormat(tInterface, "modulepath") after tShellCommand
   end repeat
   
   # The manifest target
   put shellFormat(pTargetFolder & slash & "manifest.xml", "manifest") after tShellCommand
   
   # The output
   put shellFormat(pTargetFolder & slash & pOutputFilename, "output") after tShellCommand
   
   # Support files must be dependency-ordered
   repeat for each line tSupport in revIDEExtensionsOrderByDependency(pSupportFiles)
      put shellFormat(pTargetFolder & slash &tSupport) after tShellCommand
   end repeat
   
   # The target .lcb file
   put shellFormat(pFile) after tShellCommand
   
   # AL-2015-06-15: [[ Bug 15001 ]] Don't show console window when executing shell command
   local tHideConsoleWindows
   put the hideConsoleWindows into tHideConsoleWindows
   set the hideConsoleWindows to true
   
   local tShellOutput, tShellResult
   put shell(tShellCommand) into tShellOutput
   put the result into tShellResult
   set the hideConsoleWindows to tHideConsoleWindows
   if tShellResult is 0 then
      return tShellOutput for value
   end if
   return tShellOutput for error
end extensionCompile

command extensionsDepsOrder pExtensionList
   # Build the shell command
   local tShellCommand
   put shellFormat(__extensionCompilerPath()) into tShellCommand
   
   # The built-in module path
   put shellFormat(__extensionResourcePath() & slash & "modules/lci", "modulepath") after tShellCommand
   
   # The deps mode
   put shellFormat("order", "deps") after tShellCommand
   
   # The args
   put shellFormat("--") after tShellCommand
   repeat for each line tExtension in pExtensionList
      put shellFormat(tExtension) after tShellCommand
   end repeat
   
   # AL-2015-06-15: [[ Bug 15001 ]] Don't show console window when executing shell command
   local tHideConsoleWindows
   put the hideConsoleWindows into tHideConsoleWindows
   set the hideConsoleWindows to true
   
   local tShellOutput, tShellResult
   put shell(tShellCommand) into tShellOutput
   put the result into tShellResult
   set the hideConsoleWindows to tHideConsoleWindows
   if tShellResult is 0 then
      return tShellOutput for value
   end if
   return tShellOutput for error
end extensionsDepsOrder

private command appendToStringList @xList, pValue
   if xList is empty then
      put pValue into xList
   else
      put return & pValue after xList
   end if
end appendToStringList

command buildPackageAndExtractLCBExtensions pDocsParser, pTargetFolder, \
      pCommercialExtension, pLCCompile, pInterfaceFolder
   # Items 6 onwards in 'the params' are quoted full paths to ext files
   # Construct the list of extensions to build
   local tExtList
   repeat for each item tItem in item 6 to -1 of the params
      appendToStringList tExtList, char 2 to -2 of tItem
   end repeat
   
   extensionsDepsOrder tExtList
   if the result is not empty then
      throw the result
      exit buildPackageAndExtractLCBExtensions
   end if
   
   put word 1 to -1 of it into tExtList
   
   set the itemdelimiter to slash
   repeat for each line tExtPath in tExtList
      local tSourceFileName, tSourceFolder
      put item 1 to -2 of tExtPath into tSourceFolder
      put item -1 of tExtPath into tSourceFileName
      buildPackageAndExtractExtension "", "", "", \
            pDocsParser, \
            tSourceFolder, tSourceFileName, pTargetFolder, \
            pCommercialExtension, pLCCompile, pInterfaceFolder
   end repeat
end buildPackageAndExtractLCBExtensions

command buildPackageAndExtractExtension pProtectifyScript, \
      pProtectifyOutput, pGitHash, pDocsParser, \
      pSourceFolder, pSourceFileName, pTargetFolder, \
      pCommercialExtension, pLCCompile, pInterfaceFolder
   
   if pCommercialExtension is true and \
         not (pSourceFileName ends with ".lcb") then
      protectStack pProtectifyScript, pSourceFolder, \ 
            pSourceFileName, pProtectifyOutput, pGitHash
   end if
   
   extractDocs pDocsParser, pSourceFolder, \
         pSourceFolder & slash & pSourceFileName
   
   local tPackage
   if not (pSourceFileName ends with ".lcb") then   
      packageScriptExtension pSourceFolder, pSourceFileName, \
            pTargetFolder, pCommercialExtension
   else
      packageBuilderExtension pSourceFolder, pSourceFileName, \
            pTargetFolder, pCommercialExtension, pLCCompile, \
            pInterfaceFolder
   end if
   put the result into tPackage
   
   extractExtension tPackage, pSourceFolder, pTargetFolder
   delete file tPackage
end buildPackageAndExtractExtension
 
private function q pParam
   return quote & pParam & quote
end q
 
command protectStack pProtectifyScript, pSourceFolder, pSourceFileName, pOutput, pGitHash
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
end protectStack

command extractDocs pDocsParser, pOutputDir
   try
      start using stack pDocsParser
   catch tError
      throw "Error: could not load docs parser:" && pDocsParser & return & tError
   end try
   
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
               set the itemDelimiter to backslash
            else
               set the itemDelimiter to slash
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
end extractDocs
 
private command addFolderToArchive pBaseFolder, pArchive, pRemoveSource, pSourceFileName, pSupportFiles 
   # First add the files, then recusively add any sub folders
   local tFiles
   put files(pBaseFolder) into tFiles
   filter tFiles without "."
   filter tFiles without ".."
   
   repeat for each line tFile in tFiles
      # This must be tweaked if the files to remove are ever not
      # top-level source files
      if pRemoveSource is "true" then
         if tFile is pSourceFileName then
            next repeat
         end if
         if tFile is among the lines of pSupportFiles then
            next repeat
         end if
      end if
      revZipAddItemWithFile pArchive, tFile, pBaseFolder & slash & tFile
      if the result begins with "ziperr" then
         return the result
      end if
   end repeat
   
   local tFolders
   put folders(pBaseFolder) into tFolders
   filter tFolders without "."
   filter tFolders without ".."
   
   repeat for each line tFolder in tFolders
      addFolderToArchiveRecursive pBaseFolder, tFolder, pArchive
      if the result begins with "ziperr" then
         return the result
      end if
   end repeat
   return empty
end addFolderToArchive

private command addFolderToArchiveRecursive pBaseFolder, pFolder, pArchive
   # First add the files, then recusively add any sub folders
   local tFiles
   put files(pBaseFolder & slash & pFolder) into tFiles
   filter tFiles without "."
   filter tFiles without ".."
   
   repeat for each line tFile in tFiles
      revZipAddItemWithFile pArchive, pFolder & slash & tFile, (pBaseFolder & slash & pFolder & slash & tFile)
      if the result begins with "ziperr" then
         return the result
      end if
   end repeat
   
   local tFolders
   put folders(pBaseFolder & slash & pFolder) into tFolders
   filter tFolders without "."
   filter tFolders without ".."
   
   repeat for each line tFolder in tFolders
      addFolderToArchiveRecursive pBaseFolder, pFolder & slash & tFolder, pArchive
      if the result begins with "ziperr" then
         return the result
      end if
   end repeat
   return empty
end addFolderToArchiveRecursive

private command packageExtension pExtensionName, pSourceFolder, pSourceFileName, pTargetFolder, pRemoveSource, pSupportFiles
   __EnsureExternal "revzip"
   
   local tTargetDir, tBuildTarget, tZip
   put pExtensionName into tTargetDir
   put pTargetFolder & slash & tTargetDir into tBuildTarget
   put pSourceFolder & slash & tTargetDir & ".lce" into tZip

   write "packaging extension" && pSourceFileName && \
      "to" && tZip & return to stdout
   
   if there is a file tZip then
      delete file tZip
   end if
   
   local tArchive, tError
   revZipOpenArchive tZip, "write"
   if the result begins with "ziperr" then
      put the result into tError
   end if
   
   if tError is empty then
      addFolderToArchive pSourceFolder, tZip, pRemoveSource, \
         pSourceFileName, pSupportFiles
    
      if the result begins with "ziperr" then
         put the result into tError
      end if
   end if
   
   if tError is empty then
      revZipCloseArchive tZip
      if the result begins with "ziperr" then
         put the result into tError
      end if
   end if
   
   if tError is not empty then
      throw "zip error when packaging extension" && pSourceFileName & ":" && tError
   end if
   return tZip
end packageExtension

private command __EnsurePath pPath
   if there is a folder pPath then exit __EnsurePath
   
   set the itemdel to slash
   
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
   
   local tTargetDir, tBuildTarget, tError, tName
   set the itemdelimiter to slash
   put item -1 of pZip into tName
   set the itemdelimiter to "."
   put item 1 to -2 of tName into tName
   
   put pTargetFolder & slash & tName into tBuildTarget
   
   write "extracting extension" && pZip && \
         "to" && tBuildTarget & return to stdout
   
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
   set the itemdel to slash
   repeat for each line tFile in tZipItems
      put tBuildTarget & slash & item 1 to -2 of tFile into tDirectory
      __EnsurePath tDirectory
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

command packageScriptExtension pSourceFolder, pSourceFileName, pTargetFolder, pRemoveSource   
   local tExtShortName
   set the itemdelimiter to "."
   put item -2 of pSourceFileName into tExtShortName
   log "PACKAGE SCRIPT EXTENSION" && the params
   packageExtension "com.livecode.library." & tExtShortName, \
         pSourceFolder, pSourceFileName, pTargetFolder, pRemoveSource
   return the result
end packageScriptExtension

private function getSupportFiles pSourceFolder, pSourceFileName, pExtension
   local tSupportFiles, tBase
   set the itemdelimiter to "."
   put item 1 to -2 of pSourceFileName into tBase
   put files(pSourceFolder) into tSupportFiles
   filter tSupportFiles with tBase & "-*." & pExtension
   return tSupportFiles
end getSupportFiles

command packageBuilderExtension pSourceFolder, pSourceFileName, pTargetFolder, pRemoveSource, pLCCompile, pInterfaceFolder
   write "compiling" && pSourceFolder & return to stdout
   local tCompileShell
   put q(pLCCompile) into tCompileShell
   put " -Werror" after tCompileShell
   put " --modulepath" && q(pInterfaceFolder) after tCompileShell
   put " --manifest" && q(pSourceFolder & slash & "manifest.xml") after tCompileShell
   put " --output" && q(pSourceFolder & slash & "module.lcm") after tCompileShell
   
   -- Multi-module assemblies must be in reverse dependency order
   local tSupportFiles, tSupportDepsOrder, tDepsShell
   put getSupportFiles(pSourceFolder, pSourceFileName, "lcb") into tSupportFiles
   	
   if tSupportFiles is not empty then
      put q(pLCCompile) into tDepsShell
      put " --deps order" after tDepsShell
      repeat for each line tSupportModule in tSupportFiles
         put " " & q(pSourceFolder & slash & tSupportModule) after tDepsShell
      end repeat
      	
      put shell(tDepsShell) into tSupportDepsOrder
      if the result is not 0 then
         throw "subprocess error when finding support deps order for" && pSourceFileName & ":" && tSupportDepsOrder
      end if  
      replace return with space in tSupportDepsOrder
      put " " & tSupportDepsOrder after tCompileShell
   end if
   	
   put " " & q(pSourceFolder & slash & pSourceFileName) after tCompileShell
   
   get shell(tCompileShell)
   if the result is not 0 then
      throw "subprocess error when compiling extension" && pSourceFileName & ":" && it
   end if    
   
   # Extract an e.g. "module foo.bar.baz" line from an LCB source file
   write "extracting extension name from manifest" & return to stdout      
   local tManifest, tName
   put url("binfile:" & pSourceFolder & slash & "manifest.xml") \
         into tManifest
   
   put textDecode(tManifest, "utf-8") into tManifest
   if not matchText(tManifest, " *<name>(.*)</name> *", tName) then
      throw "could not find name in manifest" && \
            pSourceFolder & slash & "manifest.xml"
   end if
   
   packageExtension tName, pSourceFolder, pSourceFileName, \
         pTargetFolder, pRemoveSource, tSupportFiles
   return the result
end packageBuilderExtension
