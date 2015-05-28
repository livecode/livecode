script "ArchiveBuilder"
command archiveBuilderRun pEdition, pVersion, pBuildNumber, pReleaseType
   --builderLog "report", "Archiving symbols..."
   --get shell("sh" && builderCommercialFolder() & "/build-archive.osx")
   --builderLog "message", it
   --builderLog "report", "Archiving complete."
   
   builderLog "report", "Adding release to store..."
   
   local tUpdaterText, tReleaseNotesBuilder
   put  builderSystemFolder() & slash & "release_notes_builder.livecodescript" into tReleaseNotesBuilder  
   dispatch "releaseNotesBuilderRun" to stack tReleaseNotesBuilder with pEdition, pVersion, pReleaseType
   put the result into tUpdaterText
   
   set the itemDel to "-"
   if the number of items in pVersion is 1 then
      put "-gm-1" after pVersion
   end if
   set the itemDel to comma
   
   local tPostDataArray
   put pVersion into tPostDataArray["version"]
   put pBuildNumber into tPostDataArray["build_number"]
   put pReleaseTYpe into tPostDataArray["type"]
   put tUpdaterText into tPostDataArray["description"]
   put "jobbies123!" into tPostDataArray["password"]
   
   local tPostData
   repeat for each line tKey in the keys of tPostDataArray
      put "&" & tKey & "=" & urlencode(tPostDataArray[tKey]) after tPostData
   end repeat
   delete the first char of tPostData
   
   local tResult
   post tPostData to URL "http://livecode.com/livecode/building/release_create"
   put the result into tResult
   if the result is not empty then
      builderLog "report", "Error adding release" && pVersion & " to store:" && tResult
      throw "failure"
   end if
   if it is not 1 then
      builderLog "report", it
      throw "failure"
   end if
   
   builderLog "report", "Adding release complete."   
end archiveBuilderRun
