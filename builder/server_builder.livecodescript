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

   -- Make sure that the output directory exists
   local tOutputDir
   put builderOutputFolder() into tOutputDir
   builderEnsureFolder tOutputDir

   local tOutputFile
   put tOutputDir & slash & getZipFilenameStub(tVersion, pPlatform, pEdition) into tOutputFile

   -- Clean up any existing output file
   delete file tOutputFile
   
   if pPlatform begins with "win-" then
      repeat for each word tExternal in "revdb revzip revxml dbsqlite dbmysql dbpostgresql dbodbc"
         get "server-" & tExternal & ".dll"
         if there is not a file (tEngineFolder & slash & it) then
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
      if pPlatform begins with "win-" then
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
      if pPlatform begins with "win-" then
         put ".dll" into tLibExtension
      else if pPlatform is "macosx" then
         put ".dylib" into tLibExtension
      else if pPlatform contains "linux" then
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
      
      // SN-2015-06-25: [[ ServerBuilder ]] DBoracle has never been in the Server zip.
      //  Removed to suppress the builder warning
      repeat for each word tDriver in "dbsqlite dbmysql dbpostgresql dbodbc"
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
      
      local tServerPath, tServerZipPath
      put tEngineFolder & slash & "server-" & toLower(pEdition) & tExeExtension into tServerPath
      if pEdition is "Community" then
         put "livecode-community-server" & tOutExeExtension into tServerZipPath
      else
         put "livecode-server" & tOutExeExtension into tServerZipPath
      end if
      
      if there is a file tServerPath then
         builderLog "message", "Adding livecode-server engine"
         revZipAddItemWithFile tOutputFile, tServerZipPath, tServerPath
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
      get builderBuiltNotesFolder() & "/LiveCodeNotes-" & tVersionClean & ".pdf"

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
   else if pPlatform is "win-x86" then
      put "Windows-x86" into pPlatform
   else if pPlatform is "win-x86_64" then
      put "Windows-x86_64" into pPlatform
   else if pPlatform is "linux-x86" then
      put "Linux" into pPlatform
   else if pPlatform is "linux-x86_64" then
      put "Linux-x86_64" into pPlatform
   else if pPlatform is "linux-armv6hf" then
      put "Linux-armv6hf" into pPlatform
   end if
   return "LiveCode" & editionTitleCase(pEdition) & "Server-" & pVersion & "-" & pPlatform & ".zip"
end getZipFilenameStub
