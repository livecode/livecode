script "BuilderUtilities"

constant kMergExtVersion = "2017-4-6"
constant kTSNetVersion = "1.2.10"

local sEngineDir
local sWorkDir
local sOutputDir
local sPrivateDir
local sDocsDir
local sBuiltDocsDir

////////////////////////////////////////////////////////////////////////////////

command builderLog  pType, pMessage, pContext
   if there is a stack "Builder" then
      dispatch "__builderLog" to stack "Builder" with  pType, pMessage, pContext
   else
      write pType & ":" & pMessage & return to stdout
      write pContext & return to stdout
   end if
end builderLog

////////////////////////////////////////////////////////////////////////////////

command builderInitialize
   start using stack (builderSystemFolder() & slash & "package_compiler.livecodescript")
end builderInitialize

command builderFinalize
   stop using stack (builderSystemFolder() & slash & "package_compiler.livecodescript")
end builderFinalize

command builderSetEngineDir pDir
   put pDir into sEngineDir
end builderSetEngineDir

command builderSetPrivateDir pDir
   put pDir into sPrivateDir
end builderSetPrivateDir

command builderSetWorkDir pDir
   put pDir into sWorkDir
end builderSetWorkDir

command builderSetDocsDir pDir
   put pDir into sDocsDir
end builderSetDocsDir

command builderSetBuiltDocsDir pDir
   put pDir into sBuiltDocsDir
end builderSetBuiltDocsDir

command builderSetOutputDir pDir
   put pDir into sOutputDir
end builderSetOutputDir

////////////////////////////////////////////////////////////////////////////////

on builderBuild pWhich, pPlatforms, pEdition, pType
   builderInitialize  
   
   local tVersion
   put builderGetVersion() into tVersion
   
   switch pWhich
      case "Tools"
         repeat for each line tPlatform in pPlatforms
            dispatch "toolsBuilderRun" to stack builderToolsBuilderStack() with tPlatform, pEdition, tVersion
         end repeat
         break
      case "Bundle"
         repeat for each line tPlatform in pPlatforms
            dispatch "toolsBuilderRunBundle" to stack builderToolsBuilderStack() with tPlatform, pEdition, tVersion
         end repeat
         break
      case "Engine"
         dispatch "engineBuilderRun" to stack builderEngineBuilderStack() with "environment", pEdition, tVersion
         repeat for each line tPlatform in pPlatforms
            dispatch "engineBuilderRun" to stack builderEngineBuilderStack() with tPlatform, pEdition, tVersion
         end repeat
         break
      case "Server"
         repeat for each line tPlatform in pPlatforms
            dispatch "serverBuilderRun" to stack builderServerBuilderStack() with tPlatform, pEdition, tVersion
         end repeat
         break
      case "Notes"
         dispatch "releaseNotesBuilderRun" to stack builderNotesBuilderStack() with pEdition, tVersion, pType, builderOutputFolder()
         break
      case "Docs"
         dispatch "docsBuilderRun" to stack builderDocsBuilderStack() with pEdition, tVersion
         break
      case "Guide"
         dispatch "guideBuilderRun" to stack builderGuideBuilderStack() with pEdition, tVersion, pType, builderRepoFolder()
         break
      case "Disk"
         repeat for each line tPlatform in pPlatforms
            dispatch "toolsBuilderRunDisk" to stack builderToolsBuilderStack() with tPlatform, pEdition, tVersion
         end repeat
         break
      case "Archive"
         local tBuildNumber
         put builderGetBuildNumber() into tBuildNumber
         dispatch "archiveBuilderRun" to stack builderArchiveBuilderStack() with tVersion, tBuildNumber, pType
         break
      case "Embedded"
         dispatch "embeddedBuilderBuildPlugin" to stack builderEmbeddedBuilderStack()
         break
   end switch
   	
   builderFinalize	
end builderBuild

-- Fetch the platform string for the platform on which the builder is
-- currently running
function builderPlatform
   switch the platform
      case "linux"
         if the processor ends with "86" then
            return "linux-x86"
         end if
         if the processor ends with "64" then
            return "linux-x86_64"
         end if
         throw "Unknown Linux architecture:" && the processor
      case "win32"
         return "windows"
      case "macos"
         return "macosx"
      case "iphone"
         return "ios"
      case "html5"
         return "emscripten"
      case "android"
         return "android"
      default
         return the platform
   end switch
end builderPlatform

command builderFetchEngine pVersion, pPlatform, pMode
   if pMode is empty then
      put "Release" into pMode
   end if
   if pPlatform is empty then
      put builderPlatform() into pPlatform
   end if
   if sEngineDir is not empty then
      switch pPlatform
         case "windows"
            return sEngineDir & slash & "win-x86-bin"
         case "linux-x86"
            return sEngineDir & slash & "linux-x86-bin"
         case "linux-x86_64"
            return sEngineDir & slash & "linux-x86_64-bin"
         case "linux-armv6hf"
            return sEngineDir & slash & "linux-armv6hf-bin"
         case "macosx"
            return sEngineDir & slash & "mac-bin"
         case "ios"
            return sEngineDir & slash & "ios-bin"
         case "android"
            return sEngineDir & slash & "android-armv6-bin"
         case "emscripten"
            return sEngineDir & slash & "emscripten-bin"
         default
            write the executionContexts to stderr
            builderLog "error", "Unknown platform '" & pPlatform & "'"
      end switch
      return empty
   end if
   local tPlatform
   switch pPlatform
      case "windows"
         get "windows/" & tolower(pMode)
         break
      case "linux-x86"
      case "linux-x86_64"
      case "linux-armv6hf"
         get "linux"
         break
      case "macosx"
         get "mac/" & pMode
         break
      case "ios"
         get "ios"
         break
      case "android"
         get "android/" & tolower(pMode)
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

-- Returns the folder of the private repository
function builderPrivateRepoFolder
   if sPrivateDir is not empty then
      return sPrivateDir
   end if
   return builderRepoFolder() & slash & ".."
end builderPrivateRepoFolder

-- Returns the folder of the IDE repository
function builderIdeRepoFolder
   return builderRepoFolder() & slash & "ide"
end builderIdeRepoFolder

-- Returns the folder of the builder system
function builderSystemFolder
   set the itemDelimiter to slash
   return item 1 to -2 of the filename of me
end builderSystemFolder

-- Return the folder of the private builder system
function builderSystemPrivateFolder
   return builderPrivateRepoFolder() & slash & "builder" 
end builderSystemPrivateFolder

-- Returns the folder where intermediate products should be placed
function builderWorkFolder
   if sWorkDir is not empty then
      return sWorkDir
   end if
   return builderRepoFolder() & slash & "_build/final/work"
end builderWorkFolder

-- Returns the folder where input products are placed
function builderInputFolder
   return sEngineDir
end builderInputFolder

-- Returns the folder where output products should be placed
function builderOutputFolder
   if sOutputDir is not empty then
      return sOutputDir
   end if
   return builderRepoFolder() & slash & "_build/final/output"
end builderOutputFolder

-- Returns the folder where all the work should be done
function builderWorkspaceFolder
   return builderRepoFolder() & slash & "_build/final"
end builderWorkspaceFolder

function builderIDEFolder
   return builderIdeRepoFolder()
end builderIDEFolder

function builderIDEDocsFolder
   return builderIDEFolder() & slash & "Documentation"
end builderIDEDocsFolder

# Location for api js file in > LC8
function builderAPIFolder pEdition
   return builderIDEDocsFolder() & slash & "html_viewer/resources/data/" & pEdition & "/api"
end builderAPIFolder

# Location for guide js file in > LC8
function builderGuideFolder pEdition
   return builderIDEDocsFolder() & slash & "html_viewer/resources/data/" & pEdition & "/guide"
end builderGuideFolder

function builderDocsFolder
   if sDocsDir is not empty then
      return sDocsDir
   end if
   return builderRepoFolder() & slash & "docs"
end builderDocsFolder

function builderBuiltDocsFolder
   if sBuiltDocsDir is not empty then
      return sBuiltDocsDir
   end if
   return builderRepoFolder() & slash & "_build/docs"
end builderBuiltDocsFolder

function builderBuiltNotesFolder
   return builderRepoFolder()
end builderBuiltNotesFolder

function builderMergExtVersion
   return kMergExtVersion
end builderMergExtVersion

function builderUnpackFolder
   return sWorkDir & "/ext/unpacked"
end builderUnpackFolder

function builderTSNetVersion
   return kTSNetVersion
end builderTSNetVersion

-- Folder for storing guide Markdown files that are generated as part
-- of the build process.
function builderBuiltGuidesFolder
   return builderBuiltDocsFolder() & slash & "guides"
end builderBuiltGuidesFolder

function builderCommercialFolder
   set the itemDel to slash
   return item 1 to -4 of the filename of me
end builderCommercialFolder

function builderCommercialResourceFolder
   return builderCommercialFolder() & "/engine/rsrc"
end builderCommercialResourceFolder

function builderCommunityFolder
   set the itemDel to slash
   return item 1 to -3 of the filename of me
end builderCommunityFolder

function builderCommunityResourceFolder
   return builderCommunityFolder() & "/engine/rsrc"
end builderCommunityResourceFolder

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

function builderServerBuilderStack
   return builderSystemFolder() & slash & "server_builder.livecodescript"
end builderServerBuilderStack

function builderEngineBuilderStack
   return builderSystemFolder() & slash & "engine_builder.livecodescript"
end builderEngineBuilderStack

function builderDocsBuilderStack
   return builderSystemFolder() & slash & "docs_builder.livecodescript"
end builderDocsBuilderStack

function builderGuideBuilderStack
   return builderSystemFolder() & slash & "user_guide_builder.livecodescript"
end builderGuideBuilderStack

function builderNotesBuilderStack
   return builderSystemFolder() & slash & "release_notes_builder.livecodescript"
end builderNotesBuilderStack

function builderArchiveBuilderStack
   return builderSystemPrivateFolder() & slash & "archive_builder.livecodescript"
end builderArchiveBuilderStack

function builderEmbeddedBuilderStack
   return builderSystemPrivateFolder() & slash & "embedded_builder_builder.livecodescript"
end builderEmbeddedBuilderStack

function builderModuleInterfaceFolder
   builderFetchEngine "", "macosx"
   return the result & slash & "modules" & slash & "lci"
end builderModuleInterfaceFolder

function builderPackagedExtensionsFolder
   builderFetchEngine "", "macosx"
   return the result & slash & "packaged_extensions"
end builderPackagedExtensionsFolder

-- Returns the engine that should be used to build the installer
function builderInstallerEngine pPlatform
   local tEngineFolder
   builderFetchEngine empty, pPlatform
   put the result into tEngineFolder
   
   switch pPlatform
      case "windows"
         return tEngineFolder & slash & "installer.exe"
      case "linux-x86"
         return tEngineFolder & slash & "installer"
      case "linux-x86_64"
         return tEngineFolder & slash & "installer"
      case "linux-armv6hf"
         return tEngineFolder & slash & "installer"
      case "macosx"
         return tEngineFolder & slash & "Installer.app"
   end switch
end builderInstallerEngine

function builderMakeTemporaryFile pTempFolder, pTag
   local tIndex
   put 1 into tIndex
   repeat while there is a file (pTempFolder & slash & pTag & "-" & tIndex)
      add 1 to tIndex
   end repeat
   
   return pTempFolder & slash & pTag & "-" & tIndex
end builderMakeTemporaryFile

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

////////////////////////////////////////////////////////////////////////////////

command builderEnsureZip pZipfile, pURL
   if there is not a file pZipfile then
      builderEnsureFolderForFile pZipfile
      
      builderLog "message", "Downloading zip file:" && pZipfile
      get shell(merge("curl -o '[[pZipfile]]' '[[pUrl]]'"))
      if the result is not zero then
         builderLog "error", "Failed to download zip file:" && it
         throw "failure"
      end if
   end if
end builderEnsureZip

command builderUnpackZip pZipfile, pFolder
   builderLog "message", "Unpacking zip file:" && pZipfile
   
   builderEnsureFolder pFolder
   
   get shell(merge("cd '[[pFolder]]' && unzip '[[pZipfile]]'"))
   if the result is not zero then
      builderLog "error", "Failed to unzip file:" && it
      throw "failure"
   end if
end builderUnpackZip

////////////////////////////////////////////////////////////////////////////////

function builderMergExtUrl pMergExtVersion, pEdition
   return "https://downloads.livecode.com/mergext/mergExt_" & pEdition & "_" & pMergExtVersion & ".zip"
end builderMergExtUrl

private function builderMergExtDownloadedFilePath pMergExtVersion, pEdition
   return sWorkDir & "/ext/downloads/MergExt-" & pEdition & "-" & pMergExtVersion & ".zip"
end builderMergExtDownloadedFilePath

private command builderMergExtUnpack pMergExtVersion, pEdition
   local tFolder
   put builderUnpackFolder() & slash & "Ext" into tFolder
   
   local tZipFile
   put builderMergExtDownloadedFilePath(pMergExtVersion, pEdition) into tZipFile
   
   local tURL
   put builderMergExtUrl(pMergExtVersion, pEdition) into tURL
   
   builderEnsureZip tZipFile, tURL
   
   builderUnpackZip tZipFile, tFolder
end builderMergExtUnpack

////////////////////////////////////////////////////////////////////////////////

function builderTSNetUrl pTSNetVersion, pEdition
   return "https://downloads.livecode.com/tsNet/tsNet_" & pEdition & "_" & pTSNetVersion & ".zip"
end builderTSNetUrl

private function builderTSNetDownloadedFilePath pTSNetVersion, pEdition
   return sWorkDir & "/ext/downloads/tsNet_" & pEdition & "_" & pTSNetVersion & ".zip"
end builderTSNetDownloadedFilePath

private command builderTSNetUnpack pTSNetVersion, pEdition
   local tFolder
   put builderUnpackFolder() & slash & "Ext" into tFolder
   
   local tZipFile
   put builderTSNetDownloadedFilePath(pTSNetVersion, pEdition) into tZipFile
   
   local tURL
   put builderTSNetUrl(pTSNetVersion, pEdition) into tURL
   
   builderEnsureZip tZipFile, tURL
   
   builderUnpackZip tZipFile, tFolder
end builderTSNetUnpack

////////////////////////////////////////////////////////////////////////////////

command builderExtUnpack pEdition
   local tFolder
   put builderUnpackFolder() & slash & "Ext" into tFolder
   
   if there is a folder tFolder then
      builderLog "message", "Cleaning unpack folder" && tFolder
      get shell(merge("rm -r '[[tFolder]]'"))
   end if
   
   local tEdition
   put toUpper(char 1 of pEdition) & toLower(char 2 to -1 of pEdition) into tEdition
   -- Ensure that the MergExt bundle is downloaded and unpacked
   builderMergExtUnpack builderMergExtVersion(), tEdition
   
   if pEdition is not "community" then
      -- Ensure that the TSNet bundle is downloaded and unpacked
      builderTSNetUnpack builderTSNetVersion(), tEdition
   end if
end builderExtUnpack
