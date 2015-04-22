script "ServerBuilder"
////////////////////////////////////////////////////////////////////////////////

-- Building the LiveCode Server consists of
--   1) Fetch native code components for the target platform
--   2) Build the output zip

-- The layout of the zip is as follows:
--   <zip>/externals/<externals>
--   <zip>/drivers/<db drivers>
--   <zip>/livecode-server

////////////////////////////////////////////////////////////////////////////////

command serverBuilderRun pPlatform, pEdition
   builderLog "report", "Building" && pEdition && "server for" && pPlatform
   
   local tVersionFile, tVersion
   put url ("file:" & builderRepoFolder() & slash & "version") into tVersionFile
   replace space with empty in tVersionFile
   split tVersionFile by return and "="
   put tVersionFile["BUILD_SHORT_VERSION"] into tVersion
   
   local tEngineFolder
   builderFetchEngine tVersion, pPlatform
   put the result into tEngineFolder
   if pPlatform is "linux" then
      put "/i386/release" after tEngineFolder
   else if pPlatform is "linux-x64" then
      put "/x86_64/release" after tEngineFolder
   else if pPlatform is "linux-armv6hf" then
      put "/armv6-hf/release" after tEngineFolder
   end if
   
   local tOutputFile
   put builderWorkspaceFolder() & "/deploy/" & getZipFilenameStub(tVersion, pPlatform, pEdition) into tOutputFile
   delete file tOutputFile
   
   if pPlatform is "windows" then
      local tExternalsFolder
      put tEngineFolder into tExternalsFolder
      set the itemDel to slash
      put "releaseserver" into the last item of tExternalsFolder
      set the itemDel to comma
      
      repeat for each word tExternal in "revdb revzip revxml dbsqlite dbmysql dbpostgresql dbodbc"
         get "server-" & tExternal & ".dll"
         if there is a file (tExternalsFolder & slash & it) then
            builderLog "message", "Copying windows external '" & tExternal & "' from release server folder"
            put URL ("binfile:" & tExternalsFolder & slash & it) into URL ("binfile:" & tEngineFolder & slash & it)
         else
            get tExternal & ".dll"
            if there is a file (tEngineFolder & slash & it) then
               builderLog "message", "Copying windows external '" & tExternal & "' from desktop external"
               put URL ("binfile:" & tEngineFolder & slash & it) into URL ("binfile:" & tEngineFolder & slash & "server-" & it)
            end if
         end if
      end repeat
      
      if pEdition is "community" then
         builderLog "message", "Renaming windows engine for community edition"
         rename tEngineFolder & slash & "server.exe" to tEngineFolder & slash & "server-community.exe"
      end if   
   end if
   
   try
      revZipOpenArchive tOutputFile, "write"
      if the result is not empty then
         throw the result
      end if
      
      local tExeExtension, tOutExeExtension
      if pPlatform is "windows" then
         put ".exe" into tExeExtension
         put ".exe" into tOutExeExtension
         --else if pPlatform is "linux" then
         --put ".x86" into tExeExtension
         --put empty into tOutExeExtension
      else
         put empty into tExeExtension
         put empty into tOutExeExtension
      end if
      
      local tLibExtension
      if pPlatform is "windows" then
         put ".dll" into tLibExtension
      else if pPlatform is "macosx" then
         put ".dylib" into tLibExtension
      else if pPlatform is "linux" then
         put ".so" into tLibExtension
      end if
      
      repeat for each word tExternal in "revdb revzip revxml"
         get tEngineFolder & slash & "server-" & tExternal & tLibExtension
         if there is a file it then
            builderLog "message", "Adding external '" & tExternal & "'"
            revZipAddItemWithFile tOutputFile, "externals/" & tExternal & tLibExtension, it
            if the result is not empty then
               throw the result
            end if
         else
            builderLog "warning", "Could not find external '" & tExternal & "'"
         end if
      end repeat
      
      repeat for each word tDriver in "dbsqlite dbmysql dbpostgresql dbodbc dboracle"
         get tEngineFolder & slash & "server-" & tDriver & tLibExtension
         if there is a file it then
            builderLog "message", "Adding driver '" & tDriver & "'"
            revZipAddItemWithFile tOutputFile, "drivers/" & tDriver & tLibExtension, it
            if the result is not empty then
               throw the result
            end if
         else
            builderLog "message", "Warning - could not find driver '" & tDriver & "'"
         end if
      end repeat
      
      if pEdition is "Community" then
         get tEngineFolder & slash & "server-community" & tExeExtension
      else
         get tEngineFolder & slash & "server-commercial" & tExeExtension
      end if
      if there is a file it then
         builderLog "message", "Adding livecode-server engine"
         if pEdition is "Community" then
            revZipAddItemWithFile tOutputFile, "livecode-community-server" & tOutExeExtension, it
         else
            revZipAddItemWithFile tOutputFile, "livecode-server" & tOutExeExtension, it
         end if
         if the result is not empty then
            throw the result
         end if
      else
         builderLog "message", "Warning - could not find server engine"
      end if
      
      local tVersionClean
      put tVersion into tVersionClean
      if tVersionClean contains "gm" then 
         set the itemDel to "-"
         put item 1 of tVersionClean into tVersionClean
         set the itemDel to comma
      end if
      replace "." with "_" in tVersion
      replace "-" with "_" in tVersion
      replace "." with "_" in tVersionClean
      replace "-" with "_" in tVersionClean      
      get $HOME & "/Builds/Installers/LiveCode/" & tVersion & "/LiveCodeNotes-" & tVersionClean & "-Server.pdf"
      if there is a file it then
         builderLog "message", "Adding server release notes"
         revZipAddItemWithFile tOutputFile, "LiveCodeNotes-" & tVersionClean & "-Server.pdf", it
         if the result is not empty then
            throw the result
         end if
      else
         builderLog "message", "Warning - could not find release notes"
      end if
      
      
      revZipCloseArchive tOutputFile
      if the result is not empty then
         throw the result
      end if
   catch tError
      builderLog "error", "Server archive building failed - " & tError
      throw "failure"
   end try
end serverBuilderRun

////////////////////////////////////////////////////////////////////////////////

function getZipFilenameStub pVersion, pPlatform, pEdition
   if pVersion contains "gm" then 
      set the itemDel to "-"
      put item 1 of pVersion into pVersion
      set the itemDel to comma
   end if   
   replace "-" with "_" in pVersion
   replace "." with "_" in pVersion
   if pPlatform is "macosx" then
      put "Mac" into pPlatform
   else if pPlatform is "windows" then
      put "Windows" into pPlatform
   else if pPlatform is "linux" then
      put "Linux" into pPlatform
   end if
   if pEdition is "Community" then
      return "LiveCodeCommunityServer-" & pVersion & "-" & pPlatform & ".zip"
   else
      return "LiveCodeServer-" & pVersion & "-" & pPlatform & ".zip"
   end if
end getZipFilenameStub
