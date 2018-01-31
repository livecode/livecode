#!
 
send "doPackage" to me
 
on doPackage
   local tArgs
   put the commandArguments into tArgs
   
   local tEngine
   put the commandName into tEngine
   
   local tProtectifyScript, tProtectifyOutput, tGitHash
   put tArgs[2] into tProtectifyScript   
   put tArgs[3] into tProtectifyOutput
   put tArgs[4] into tGitHash

   local tExtractDocsScript, tDocsParser
   put tArgs[5] into tExtractDocsScript
   put tArgs[6] into tDocsParser
   
   local tSourceFolder, tSourceFileName, tTargetFolder
   put tArgs[7] into tSourceFolder
   put tArgs[8] into tSourceFileName
   put tArgs[9] into tTargetFolder
   
   local tCommercialExtension
   put tArgs[10] into tCommercialExtension
   
   local tLCCompile, tInterfaceFolder
   put tArgs[11] into tLCCompile
   put tArgs[12] into tInterfaceFolder
   
   buildPackageAndExtractExtension tEngine, tProtectifyScript, \
      tProtectifyOutput, tGitHash, tExtractDocsScript, tDocsParser, \
      tSourceFolder, tSourceFileName, tTargetFolder, \
      tCommercialExtension, tLCCompile, tInterfaceFolder
end doPackage

command buildPackageAndExtractExtension pEngine, pProtectifyScript, \
   pProtectifyOutput, pGitHash, pExtractDocsScript, pDocsParser, \
   pSourceFolder, pSourceFileName, pTargetFolder, \
   pCommercialExtension, pLCCompile, pInterfaceFolder

   if pCommercialExtension is true and \
      not (pSourceFileName ends with ".lcb") then
      protectStack pEngine, pProtectifyScript, pSourceFolder, \ 
         pSourceFileName, pProtectifyOutput, pGitHash
   end if
   
   extractDocs pEngine, pExtractDocsScript, pDocsParser, \
      pSourceFolder, pSourceFileName
   
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
   
   extractExtension tPackage, pSourceFolder, pTargetFolder, \
      pCommercialExtension
      
   quit 0
end buildPackageAndExtractExtension
 
function q pParam
   return quote & pParam & quote
end q
 
command protectStack pEnginePath, pProtectifyScript, pSourceFolder, pSourceFileName, pOutput, pGitHash
   local tShell
   put merge(q("[[pEnginePath]]") && q("[[pProtectifyScript]]") && \
         q("[[pSourceFolder]]" & slash & "[[pSourceFileName]]") && \
         q("[[pOutput]]") && q("[[pGitHash]]")) into tShell
   
   get shell(tShell)
   if the result is not 0 then
      write "subprocess error when protecting stack" && pSourceFileName & ":" && it to stderr
      quit 1
   end if
end protectStack

command extractDocs pEnginePath, pExtractScript, pDocsParser, pSourceFolder, pSourceFileName
   write the params to stdout
   local tShell, tSource
   put pSourceFolder & slash & pSourceFileName into tSource
   put merge(q("[[pEnginePath]]") && q("[[pExtractScript]]") && \
         q("[[pDocsParser]]") && q("[[pSourceFolder]]") && q("[[tSource]]")) \
         into tShell
   
   get shell(tShell)
   if the result is not 0 then
      write "subprocess error when extracting docs from" && pSourceFileName & ":" && it to stderr
      quit 2
   end if
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
   write "packaging extension" && pSourceFileName & return to stdout
   local tTargetDir, tBuildTarget, tZip
   put pExtensionName into tTargetDir
   put pTargetFolder & slash & tTargetDir into tBuildTarget
   put pSourceFolder & slash & tTargetDir & ".lce" into tZip
   
   local tArchive, tError
   revZipOpenArchive tZip, "write"
   if the result begins with "ziperr" then
      put the result into tError
   end if
   addFolderToArchive pSourceFolder, tZip, pRemoveSource, pSourceFileName, pSupportFiles
   if the result begins with "ziperr" then
      put the result into tError
   end if
   revZipCloseArchive tZip
   if the result begins with "ziperr" then
      put the result into tError
   end if
   if tError is not empty then
      write "zip error when packaging extension" && pSourceFileName & ":" && tError to stderr
      quit 3
   end if
   return tZip
end packageExtension

private command __EnsurePath pPath
   if there is a folder pPath then exit __EnsurePath
   
   local tPath
   set the itemdel to "/"
   repeat for each item tFolder in pPath
      
      if tFolder contains ":" then
         put tFolder after tPath
      else
         put slash & tFolder after tPath
      end if
      
      if tPath is empty then next repeat
      if there is a folder (quote & tPath & quote) then next repeat
      
      create folder tPath
   end repeat
end __EnsurePath

private command extractExtension pZip, pExtensionName, pSourceFolder, pTargetFolder
   local tTargetDir, tBuildTarget, tError
   put pExtensionName into tTargetDir
   put pTargetFolder & slash & tTargetDir into tBuildTarget
   
   create folder tBuildTarget
   # Extract the icon,docs and executable module into the folder
   revZipOpenArchive pZip, "read"
   if the result begins with "ziperr" then
      put the result into tError
   end if
   
   # Work out the root of the zip
   local tZipItems, tZipRoot
   put revZipEnumerateItems(pZip) into tZipItems
   if the result begins with "ziperr" then
      put the result into tError
   end if
   if the last char of line 1 of tZipItems is "/" then
      put line 1 of tZipItems into tZipRoot
   else
      put empty into tZipRoot
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
   
   revZipCloseArchive pZip
   if the result begins with "ziperr" then
      put the result into tError
   end if
   if tError is not empty then
      write "zip error when extracting extension" && pExtensionName & ":" && tError to stderr
      quit 3
   end if
end extractExtension

command packageScriptExtension pSourceFolder, pSourceFileName, pTargetFolder, pRemoveSource   
   local tExtShortName
   set the itemdelimiter to "."
   put item -2 of pSourceFileName into tExtShortName
   
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
			write "subprocess error when finding support deps order for" && pSourceFileName & ":" && tSupportDepsOrder to stderr
			quit 3
		end if  
		replace return with space in tSupportDepsOrder
		put " " & tSupportDepsOrder after tCompileShell
	end if
	
    put " " & q(pSourceFolder & slash & pSourceFileName) after tCompileShell

    get shell(tCompileShell)
    if the result is not 0 then
        write "subprocess error when compiling extension" && pSourceFileName & ":" && it to stderr
        quit 3
    end if    
    
    # Extract an e.g. "module foo.bar.baz" line from an LCB source file
    local tExtractNameShell, tExtensionName
    put "sed -nEe 's,^([[:space:]]*<name>(.*)</name>[[:space:]]*)$,\2,p' <" \
       && q(pSourceFolder & slash & "manifest.xml") into tExtractNameShell

    write "extracting extension name from manifest" & return to stdout    
    put word 1 to -1 of shell(tExtractNameShell) into tExtensionName
    if the result is not 0 then
        write "subprocess error when extracting name from" && pSourceFileName & ":" && it to stderr
        quit 3
    end if   	
    
   packageExtension tExtensionName, pSourceFolder, pSourceFileName, \
         pTargetFolder, pRemoveSource, tSupportFiles
   return the result
end packageBuilderExtension

on errorDialog tError
   write "Error:" && tError to stderr
   quit 10
end errorDialog