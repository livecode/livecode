script "BuilderUtilities"
////////////////////////////////////////////////////////////////////////////////

command builderLog  pType, pMessage
   if there is a stack "Builder" then
      send "__builderLog pType, pMessage" to stack "Builder"
   end if
end builderLog

////////////////////////////////////////////////////////////////////////////////

command builderInitialize
   start using stack (builderSystemFolder() & slash & "package_compiler.livecodescript")
end builderInitialize

command builderFinalize
   stop using stack (builderSystemFolder() & slash & "package_compiler.livecodescript")
end builderFinalize

////////////////////////////////////////////////////////////////////////////////

on builderBuild pWhich, pPlatforms, pEdition, pType
   builderInitialize  
   
   local tVersion
   put builderGetVersion() into tVersion
   
   switch pWhich
      case "Tools"
         repeat for each line tPlatform in pPlatforms
            # Only one linux target
            if tPlatform begins with "linux" and tPlatform is not "linux" then next repeat
            dispatch "toolsBuilderRun" to stack builderToolsBuilderStack() with tPlatform, pEdition, tVersion
         end repeat
         break
      case "Engine"
         dispatch "engineBuilderRun" to stack builderEngineBuilderStack() with "environment", pEdition, tVersion
         repeat for each line tPlatform in pPlatforms
            # Only one linux target
            if tPlatform begins with "linux" and tPlatform is not "linux" then next repeat
            dispatch "engineBuilderRun" to stack builderEngineBuilderStack() with tPlatform, pEdition, tVersion
         end repeat
         break
      case "Server"
         repeat for each line tPlatform in pPlatforms
            dispatch "serverBuilderRun" to stack builderToolsBuilderStack() with tPlatform, pEdition, tVersion
         end repeat
         break
      case "Notes"
         dispatch "releaseNotesBuilderRun" to stack builderNotesBuilderStack() with pEdition, tVersion
         break
      case "Docs"
         dispatch "docsBuilderRun" to stack builderDocsBuilderStack() with pEdition, tVersion
         break
      case "Archive"
         local tBuildNumber
         put builderGetBuildNumber() into tBuildNumber
         dispatch "archiveBuilderRun" to stack builderArchiveBuilderStack() with pEdition, tVersion, tBuildNumber, pType
         break
   end switch
   	
   builderFinalize	
end builderBuild

command builderFetchEngine pVersion, pPlatform
   local tPlatform
   switch pPlatform
      case "windows"
         get "windows/release"
         break
      case "linux"
      case "linux-x64"
      case "linux-armv6hf"
         get "linux"
         break
      case "macosx"
         get "mac/Release"
         break
      case "ios"
         get "ios"
         break
      case "android"
         get "android/release"
         break
   end switch
   return builderRepoFolder() & "/_build/" & it
end builderFetchEngine

////////////////////////////////////////////////////////////////////////////////

-- This command copies a file to folder, or to a new name in a folder
private command copyFile pSrc, pDst
   if there is a folder pDst then
      set the itemDelimiter to slash
      put slash & item -1 of pSrc after pDst
      set the itemDelimiter to comma
   end if
   
   put url ("binfile:" & pSrc) into url ("binfile:" & pDst)
end copyFile

-- This command copies a folder to another location, building a simple 'Manifest' as it
-- goes.
private command copyFolder @xManifest, pPrefix, pSrcFolder, pDstFolder
   local tOldFolder
   put the folder into tOldFolder
   set the folder to pSrcFolder
   if the result is not empty then
      exit copyFolder
   end if
   repeat for each line tFile in the files
      put url ("binfile:" & pSrcFolder & slash & tFile) into url ("binfile:" & pDstFolder & slash & tFile)
      if pPrefix ends with "Contents/MacOS" then
         put "executable," & pPrefix & slash & tFile & return after xManifest
      else
         put "file," & pPrefix & slash & tFile & return after xManifest
      end if
   end repeat
   repeat for each line tFolder in the folders
      if tFolder is "." or tFolder is ".." then
         next repeat
      end if
      create folder pDstFolder & slash & tFolder
      put "folder," & pPrefix & slash & tFolder & return after xManifest
      copyFolder xManifest, pPrefix & slash & tFolder, pSrcFolder & slash & tFolder, pDstFolder & slash & tFolder
   end repeat
   set the folder to tOldFolder
end copyFolder

////////////////////////////////////////////////////////////////////////////////

-- Returns the folder of the repository
function builderRepoFolder
   set the itemDelimiter to slash
   return item 1 to -3 of the filename of me
end builderRepoFolder

function builderIDEFolder
   set the itemDelimiter to slash
   return (item 1 to -3 of the filename of me & slash & "ide")
end builderIDEFolder

-- Returns the folder of the builder system
function builderSystemFolder
   set the itemDelimiter to slash
   return item 1 to -2 of the filename of me
end builderSystemFolder

-- Returns the folder where all the work should be done
function builderWorkspaceFolder
   return builderRepoFolder() & slash & "_build/final"
   --return specialFolderPath("Desktop") & slash & "BuilderWorkspace"
end builderWorkspaceFolder

function builderIDEFolder
   return builderRepoFolder() & slash & "ide"
end builderIDEFolder

function builderIDEDocsFolder
   return builderIDEFolder() & slash & "Documentation"
end builderIDEDocsFolder

# Location for api js file in > LC8
function builderAPIFolder
   return builderIDEDocsFolder() & slash & "html_viewer/resources/data/api"
end builderAPIFolder

# Location for guide js file in > LC8
function builderGuideFolder
   return builderIDEDocsFolder() & slash & "html_viewer/resources/data/guide"
end builderGuideFolder

function builderDocsFolder
   return builderRepoFolder() & slash & "docs"
end builderDocsFolder

function builderBuiltDocsFolder
   return builderRepoFolder() & slash & "_build/docs"
end builderBuiltDocsFolder

function builderCommercialFolder
   set the itemDel to slash
   return item 1 to -4 of the filename of me & "/livecode-commercial"
end builderCommercialFolder

function builderCommercialResourceFolder
   return builderCommercialFolder() & "/rsrc"
end builderCommercialResourceFolder

function builderBuildsFolder
   set the itemDelimiter to slash
   return item 1 to -4 of the filename of me
end builderBuildsFolder

function builderResourceStack
   return builderSystemFolder() & slash & "tools_installer_resources.rev"
end builderResourceStack

function builderToolsBuilderStack
   return builderSystemFolder() & slash & "tools_builder.livecodescript"
end builderToolsBuilderStack

function builderEngineBuilderStack
   return builderSystemFolder() & slash & "engine_builder.livecodescript"
end builderEngineBuilderStack

function builderDocsBuilderStack
   return builderSystemFolder() & slash & "docs_builder.livecodescript"
end builderDocsBuilderStack

function builderNotesBuilderStack
   return builderSystemFolder() & slash & "release_notes_builder.livecodescript"
end builderNotesBuilderStack

function builderArchiveBuilderStack
   return builderSystemFolder() & slash & "archive_builder.livecodescript"
end builderArchiveBuilderStack

function builderModuleInterfaceFolder
   builderFetchEngine "", "macosx"
   return the result & slash & "modules"
end builderModuleInterfaceFolder

-- Returns the engine that should be used to build the installer
function builderInstallerEngine pPlatform
   local tEngineFolder
   --put builderWorkspaceFolder() & slash & "engine" & slash & pPlatform & "-" & item 1 of field "Version" of card 1 of me into tEngineFolder
   builderFetchEngine empty, pPlatform
   put the result into tEngineFolder
   
   switch pPlatform
      case "windows"
         return tEngineFolder & slash & "installer.exe"
      case "linux"
         return tEngineFolder & slash & "i386/release/installer"
      case "linux-x64"
         return tEngineFolder & slash & "x86_64/release/installer"
      case "linux-armv6hf"
         return tEngineFolder & slash & "armv6-hf/release/installer"
      case "macosx"
         return tEngineFolder & slash & "installer.app"
   end switch
end builderInstallerEngine

////////////////////////////////////////////////////////////////////////////////

command builderEnsureFolder pFolder
   set the itemDelimiter to "/"
   if not (pFolder begins with (builderWorkspaceFolder() & slash)) then
      --builderLog "error", "Attempt to create folder outside of root (" & pFolder & ")"
      --throw "failed"
   end if
   
   set the itemDelimiter to slash
   repeat with i = 2 to the number of items in pFolder
      if there is no folder (item 1 to i of pFolder) then
         create folder item 1 to i of pFolder
      end if
   end repeat
   
   if there is no folder pFolder then
      builderLog "error", "Folder creation failed (" & pFolder & ")"
      throw "failed"
   end if
end builderEnsureFolder

command builderEnsureFolderForFile pFile
   set the itemDelimiter to slash
   builderEnsureFolder item 1 to -2 of pFile
end builderEnsureFolderForFile

////////////////////////////////////////////////////////////////////////////////

function builderGetVersion
   local tVersionFile, tVersion
   put url ("file:" & builderRepoFolder() & slash & "version") into tVersionFile
   replace space with empty in tVersionFile
   split tVersionFile by return and "="
   put tVersionFile["BUILD_SHORT_VERSION"] into tVersion   
   return tVersion
end builderGetVersion

function builderGetBuildNumber
   local tVersionFile, tVersion
   put url ("file:" & builderRepoFolder() & slash & "version") into tVersionFile
   replace space with empty in tVersionFile
   split tVersionFile by return and "="
   put tVersionFile["BUILD_REVISION"] into tVersion   
   return tVersion
end builderGetBuildNumber

////////////////////////////////////////////////////////////////////////////////

function getExecutableArchs pInput
   get shell("lipo -info" && quote & pInput & quote)
   set the itemDelimiter to ":"
   get word 1 to -1 of the last item of it
   replace space with comma in it
   return it
end getExecutableArchs

command dietAndStrip pArchs, pInput, pOutput
   local tCurrentArchs
   put getExecutableArchs(pInput) into tCurrentArchs
   
   local tActualArchs
   repeat for each item tArch in pArchs
      if tArch is "x86" then
         put "i386" into tArch
      end if
      if tArch is "x86-64" then
         put "x86_64" into tArch
      end if
      if tArch is "ppc" then
         repeat for each item tCurrentArch in tCurrentArchs
            if tCurrentArch contains "ppc" then
               put tCurrentArch into tArch
               next repeat
            end if
         end repeat
      end if
      if tArch is among the items of tCurrentArchs then
         put tArch & comma after tActualArchs
      end if
   end repeat
   delete the last char of tActualArchs
   
   if the number of items of tCurrentArchs is not the number of items of tActualArchs then
      if  the number of items of tActualArchs is 1 then
         get shell("lipo -thin " & tActualArchs && "-output" && quote & pOutput & quote && quote & pInput & quote)
         if it is not empty then
            return it
         end if
         get shell("strip -S" && quote & pOutput & quote)
      else
         local tFiles
         repeat for each item tArch in tActualArchs
            put quote & pOutput & "." & tArch & quote & space after tFiles
            get shell("lipo -thin" && tArch && "-output" && quote & pOutput & "." & tArch & quote && quote & pInput & quote)
            if it is not empty then
               return it
            end if
         end repeat
         get shell("lipo -create" && tFiles && "-output" && quote & pOutput & quote)
         if it is not empty then
            return it
         end if
         get shell("strip -S" && quote & pOutput & quote)
      end if
   else
      get shell("strip -S -o" && quote & pOutput & quote && quote & pInput & quote)
   end if
   if it is not empty then
      return it
   end if
   return empty
end dietAndStrip

function windowsManifest
   return url ("file:" & builderSystemFolder() & slash & "windows_manifest.xml")
end windowsManifest

function macPList
   return url ("file:" & builderSystemFolder() & slash & "mac_info.plist")
end macPList
