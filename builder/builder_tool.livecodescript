script "Command Line Builder"
local sWarnAsError
local sDebug -- True if the builder should run in debugging mode

on startup
   set the itemdelimiter to slash
   start using stack (item 1 to -2 of the filename of me & slash & "builder_utilities.livecodescript")
   start using stack (item 1 to -2 of the filename of me & slash & "edition_utilities.livecodescript")
   set the itemDelimiter to comma
   
   __loadExternal "revzip"
   __loadExternal "revxml"
   __loadExternal "revdb"
   
   put false into sWarnAsError
   put ($BUILDER_DEBUG is not empty) into sDebug
   
   local tArgs, tArgIndex
   repeat with tArgIndex = 1 to $# - 1
      put value("$" & tArgIndex) into tArgs[tArgIndex]
   end repeat
   
   local tPlatforms, tStage, tEdition, tBuild
   put empty into tPlatforms
   put "tools" into tStage
   put "community" into tEdition
   put "stable" into tBuild
   
   local tEngineDir, tPrivateDir, tOutputDir, tWorkDir, tDocsDir, tBuiltDocsDir, tArch
   
   put 1 into tArgIndex
   repeat while tArgIndex is among the keys of tArgs
      if tArgs[tArgIndex] is "--platform" then
         get tArgs[tArgIndex + 1]
         if it is not among the items of "windows,win,win-x86,win-x86_64,macosx,mac,linux,linux-x86,linux-x86_64,universal" then
            __builderLog "error", "Unknown platform '" & tArgs[tArgIndex + 1] & "'"
         end if
         -- Hack the arch from the platform. This should really use
         -- triples.
         set the itemdelimiter to "-"
         put item -1 of it into tArch
         set the itemdelimiter to comma
		 
         if it is "linux" then
            put true into tPlatforms["linux-x86"]
            put true into tPlatforms["linux-x86_64"]
         else if it is "win" then
            put true into tPlatforms["win-x86"]
            put true into tPlatforms["win-x86_64"]
         else
            if it is "mac" then get "macosx"
            put true into tPlatforms[it]
         end if
         add 1 to tArgIndex
      else if tArgs[tArgIndex] is "--stage" then
         if tArgs[tArgIndex + 1] is not among the items of "environment,tools,bundle,server,notes,docs,disk,guide,fmpackage" then
            __builderLog "error", "Unknown stage '" & tArgs[tArgIndex + 1] & "'"
         end if
         put tArgs[tArgIndex + 1] into tStage
         add 1 to tArgIndex
      else if tArgs[tArgIndex] is "--edition" then
         if tArgs[tArgIndex + 1] is not among the items of editionNames() then
            __builderLog "error", "Unknown edition '" & tArgs[tArgIndex + 1] & "'"
         end if
         put tArgs[tArgIndex + 1] into tEdition
         add 1 to tArgIndex
      else if tArgs[tArgIndex] is "--build" then
         if tArgs[tArgIndex + 1] is not among the items of "stable,maintenance,development,beta" then
            __builderLog "error", "Unknown build '" & tArgs[tArgIndex + 1] & "'"
         end if
         put tArgs[tArgIndex + 1] into tBuild
         add 1 to tArgIndex
      else if tArgs[tArgIndex] is "--engine-dir" or tArgs[tArgIndex] is "--input-dir" then
         if tArgs[tArgIndex + 1] is empty then
            __builderLog "error", "No path specified for --engine-dir argument"
         end if
         put tArgs[tArgIndex + 1] into tEngineDir
         add 1 to tArgIndex
      else if tArgs[tArgIndex] is "--private-dir" then
         if tArgs[tArgIndex + 1] is empty then
            __builderLog "error", "No path specified for --private-dir argument"
         end if
         put tArgs[tArgIndex + 1] into tPrivateDir
         add 1 to tArgIndex
      else if tArgs[tArgIndex] is "--output-dir" then
         if tArgs[tArgIndex + 1] is empty then
            __builderLog "error", "No path specified for --output-dir argument"
         end if
         put tArgs[tArgIndex + 1] into tOutputDir
         add 1 to tArgIndex
      else if tArgs[tArgIndex] is "--work-dir" then
         if tArgs[tArgIndex + 1] is empty then
            __builderLog "error", "No path specified for --work-dir argument"
         end if
         put tArgs[tArgIndex + 1] into tWorkDir
         add 1 to tArgIndex
      else if tArgs[tArgIndex] is "--docs-dir" then
         if tArgs[tArgIndex + 1] is empty then
            __builderLog "error", "No path specified for --docs-dir argument"
         end if
         put tArgs[tArgIndex + 1] into tDocsDir
         add 1 to tArgIndex
      else if tArgs[tArgIndex] is "--built-docs-dir" then
         if tArgs[tArgIndex + 1] is empty then
            __builderLog "error", "No path specified for --built-docs-dir argument"
         end if
         put tArgs[tArgIndex + 1] into tBuiltDocsDir
         add 1 to tArgIndex
      else if tArgs[tArgIndex] is "--warn-as-error" then
         put true into sWarnAsError
      else if tArgs[tArgIndex] is "--debug" then
         put true into sDebug
      end if
      add 1 to tArgIndex
   end repeat
   
   if the keys of tPlatforms is empty and tStage is "tools" then
      __builderLog "error", "No platforms specified"
   end if
   
   if tEdition is "commercial" and tStage is not "server" then
      __builderLog "error", "Commercial is only a valid edition for server packages"
   end if
   
   if tStage is "environment" then
      put empty into tPlatforms
      put true into tPlatforms["environment"]
      put "engine" into tStage
   end if
   
   set the name of this stack to "Builder"
   
   if tArch is not empty then
      builderSetTargetArch tArch
   end if
   
   if tEngineDir is not empty then
      builderSetEngineDir tEngineDir
   end if

   if tPrivateDir is not empty then
      builderSetPrivateDir tPrivateDir
   end if
   
   if tOutputDir is not empty then
      builderSetOutputDir tOutputDir
   end if
   
   if tWorkDir is not empty then
      builderSetWorkDir tWorkDir
   end if
   
   if tDocsDir is not empty then
      builderSetDocsDir tDocsDir
   end if
   
   if tBuiltDocsDir is not empty then
      builderSetBuiltDocsDir tBuiltDocsDir
   end if
   
   builderBuild tStage, the keys of tPlatforms, tEdition, tBuild
   
   quit 0
end startup

command __loadExternal pExternal
   set the itemdelimiter to slash
   
   if the platform is "MacOS" then
      set the externals of the templateStack to item 1 to -4 of specialFolderPath("engine") & slash & pExternal & ".bundle"
   else if the platform is "linux" then
      set the externals of the templateStack to specialFolderPath("engine") & slash & pExternal & ".so"
   end if
   
   create stack pExternal && "External"
   start using it
   if the externalCommands of it is empty then
      __builderLog "error", "Cannot load external" && pExternal
   end if
end __loadExternal 

command __builderLog pType, pMessage
   local tLine

   if pType is "debug" and not sDebug then
      exit __builderLog
   end if

   -- Promote builder warnings to errors, if requested
   if pType is "warning" and sWarnAsError then
      put "error" into pType
   end if
   
   put toUpper(char 1 of pType) into char 1 of pType
   
   repeat for each line tLine in pMessage
      write ("[" && the internet date && "]" && ":" && pType && ":" && tLine & return) to stderr
   end repeat
   if pType is "error" then
      quit 1
   end if
end __builderLog

on errorDialog pBacktrace
   write pBacktrace & return to stderr
   quit 1
end errorDialog
