#!
 
send "doPackage" to me
 
on doPackage
   local tArgs
   put the commandArguments into tArgs

   if tArgs[11] is true and not (tArgs[8] ends with ".lcb") then
       protectStack the commandName, tArgs[2], tArgs[7], tArgs[8], tArgs[3], tArgs[4]
   end if
   
   extractDocs the commandName, tArgs[5], tArgs[6], tArgs[7], tArgs[8]

   if not (tArgs[8] ends with ".lcb") then   
       packageScriptExtension tArgs[7], tArgs[8], tArgs[9], tArgs[10]
   else
       packageBuilderExtension tArgs[7], tArgs[8], tArgs[9], tArgs[10], tArgs[11], tArgs[12]
   end if
   quit 0
end doPackage
 
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
       write "subprocess error when protecting stack" && pSource & ":" && it to stderr
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
 
command packageScriptExtension pSourceFolder, pSourceFileName, pTargetFolder, pRemoveSource   
   local tExtShortName
   set the itemdelimiter to "."
   put item -2 of pSourceFileName into tExtShortName

   local tTargetDir, tBuildTarget, tZip
   put "com.livecode.library." & tExtShortName into tTargetDir
   put pTargetFolder & slash & tTargetDir into tBuildTarget
   put pSourceFolder & slash & tTargetDir & ".lce" into tZip
   
   local tShell
   put "pushd" && q(pSourceFolder) && "1>/dev/null" into tShell
   put return & "zip -q -r " & tTargetDir & ".lce *" after tShell
   put return & "popd 1>/dev/null" after tShell
   put return & "mkdir -p " & q(tBuildTarget) after tShell
   put return & "unzip -q -o " & q(tZip) && \
         "-d " & q(tBuildTarget) after tShell
   put return & "rm" && q(tZip) after tShell
   
   if pRemoveSource is "true" then
      put return & "rm" && quote & tBuildTarget & slash & \ 
         pSourceFileName & quote after tShell
   end if
   
   get shell(tShell)
   if the result is not 0 then
       write "subprocess error when packaging extension" && pSourceFileName & ":" && it to stderr
       quit 3
   end if
end packageScriptExtension

command packageBuilderExtension pSourceFolder, pSourceFileName, pTargetFolder, pRemoveSource, pLCCompile, pInterfaceFolder
    local tCompileShell
    put q(pLCCompile) into tCompileShell
    put " -Werror" after tCompileShell
    put " --modulepath" && q(pInterfaceFolder) after tCompileShell
    put " --manifest" && q(pSourceFolder & slash & "manifest.xml") after tCompileShell
    put " --output" && q(pSourceFolder & slash & "module.lcm") after tCompileShell
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
    
   local tTargetDir, tBuildTarget, tZip
   put tExtensionName into tTargetDir
   put pTargetFolder & slash & tTargetDir into tBuildTarget
   put pSourceFolder & slash & tTargetDir & ".lce" into tZip
   
   local tShell
   put "pushd" && quote & pSourceFolder & quote && "1>/dev/null" into tShell
   put return & "zip -q -r " & tTargetDir & ".lce *" after tShell
   put return & "popd 1>/dev/null" after tShell
   put return & "mkdir -p " & quote & tBuildTarget & quote after tShell
   put return & "unzip -q -o " & quote & tZip & quote && \
         "-d " & quote & tBuildTarget & quote after tShell
   put return & "rm" && quote & tZip & quote after tShell
   
   if pRemoveSource is "true" then
      put return & "rm" && quote & tBuildTarget & slash & \ 
         pSourceFileName & quote after tShell
   end if

   write "packaging extension" && pSourceFileName & return to stdout      
   get shell(tShell)
   if the result is not 0 then
       write "subprocess error when packaging extension" && pSourceFileName & ":" && it to stderr
       quit 3
   end if

end packageBuilderExtension

on errorDialog tError
   write "Error:" && tError to stderr
   quit 10
end errorDialog