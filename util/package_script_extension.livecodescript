#!
 
send "doPackage" to me
 
on doPackage
   local tArgs
   put the commandArguments into tArgs

   if tArgs[11] is true then
       protectStack the commandName, tArgs[2], tArgs[7], tArgs[3], tArgs[4]
   end if
   extractDocs the commandName, tArgs[5], tArgs[6], tArgs[8], tArgs[7]
   packageScriptExtension tArgs[8], tArgs[9], tArgs[10], tArgs[11]

   quit 0
end doPackage
 
function q pParam
    return quote & pParam & quote
end q
 
command protectStack pEnginePath, pProtectifyScript, pSource, pOutput, pGitHash
    local tShell
    put merge(q("[[pEnginePath]]") && q("[[pProtectifyScript]]") && \
        q("[[pSource]]") && q("[[pOutput]]") && q("[[pGitHash]]")) into tShell
        
    get shell(tShell)
   if the result is not 0 then
       write "subprocess error:" && it to stderr
       quit 1
   end if
end protectStack

command extractDocs pEnginePath, pExtractScript, pDocsParser, pSourceDir, pSource
    local tShell
    put merge(q("[[pEnginePath]]") && q("[[pExtractScript]]") && \
        q("[[pDocsParser]]") && q("[[pSourceDir]]") && q("[[pSource]]")) \
        into tShell

    get shell(tShell)
   if the result is not 0 then
       write "subprocess error:" && it to stderr
       quit 2
   end if
end extractDocs
 
command packageScriptExtension pSourceFolder, pSourceFileName, pTargetFolder, pRemoveSource   

   local tTargetDir, tBuildTarget, tZip
   put "com.livecode.library." & pSourceFileName into tTargetDir
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
         pSourceFileName & ".livecodescript" & quote after tShell
        write tShell to stdout
   end if
   
   get shell(tShell)
   if the result is not 0 then
       write "subprocess error:" && it to stderr
       quit 3
   end if
end packageScriptExtension

on errorDialog tError
   write "Error:" && tError to stderr
   quit 10
end errorDialog
