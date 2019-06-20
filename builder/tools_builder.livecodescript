script "ToolsBuilder"
////////////////////////////////////////////////////////////////////////////////

-- Building the LiveCode Tools consists of several stages.
--   1) Fetch native code components for each platform
--   2) Fetch the latest IDE
--   3) Fetch the latest documentation
--   4) Build the documentation's clumps
--   5) Compile the install package
--   6) Deploy an installer

////////////////////////////////////////////////////////////////////////////////

command toolsBuilderRun pPlatform, pEdition, pVersion
   -- If on windows or linux, we can't do anything macosxy due to lack of lipo/strip :o(
   local tEngineFolders

   get "win-x86 win-x86_64 linux-x86 linux-x86_64 linux-armv6hf macosx ios android-armeabi-v7a android-arm64-v8a android-x86 android-x86_64 emscripten"
      
   repeat for each word tPlatform in it
      builderFetchEngine pVersion, tPlatform
      put the result into tEngineFolders[tPlatform]
   end repeat
   
   -- Next make sure our IDE is up to date
   local tIdeFolder
   put builderIdeRepoFolder() into tIdeFolder
   
   -- Fetch the documentation
   local tDocsFolder
   put builderBuiltDocsFolder() into tDocsFolder
   -- Now build the package
   local tPackageFile
   toolsBuilderMakePackage pVersion, pEdition, pPlatform, tEngineFolders, tIdeFolder, tDocsFolder
   put the result into tPackageFile
   
   -- Compute the private folder
   local tPrivateFolder
   put builderPrivateRepoFolder() into tPrivateFolder
   
   -- Now build the installer
   toolsBuilderMakeInstaller pVersion, pEdition, pPlatform, tIdeFolder, tPrivateFolder, tPackageFile
end toolsBuilderRun

command toolsBuilderRunDisk pPlatform, pEdition, pVersion
   if pPlatform is not "macosx" then
      exit toolsBuilderRunDisk
   end if
   
   if the platform is not "macos" then
      builderLog "error", "Cannot sign and build disk image for Mac on this platform"
      exit toolsBuilderRunDisk
   end if
   
   local tInputFolder
   put builderInputFolder() into tInputFolder
   
   local tInputFile
   put tInputFolder & slash & "installer-mac.zip" into tInputFile
   
   toolsBuilderMakeDisk pVersion, pEdition, pPlatform, tInputFile
end toolsBuilderRunDisk

command toolsBuilderRunBundle pPlatform, pEdition, pVersion
   if pPlatform is not "macosx" then
      exit toolsBuilderRunBundle
   end if
   
   if the platform is not "macos" then
      builderLog "error", "Cannot generate MacOSX app bundle on this platform"
      exit toolsBuilderRunBundle
   end if
   
   toolsBuilderMakeAppBundle pVersion, pEdition, pPlatform
end toolsBuilderRunBundle

////////////////////////////////////////////////////////////////////////////////

private command toolsBuilderFilterExternals pFolder, pPlatform
   -- Enter the extension directory
   local tOldFolder
   put the defaultFolder into tOldFolder
   set the defaultFolder to pFolder
   if the result is not empty then
      builderLog "error", merge("Failed to enter '[[pFolder]]': [[the result]]")
      exit toolsBuilderFilterExternals
   end if

   local tFiles
   -- Remove any iOS code if we're not building the OSX installer
   if pPlatform is not "macosx" then
      put the files into tFiles
      filter lines of tFiles with "*.lcext"
      
      -- Don't remove lcext files ith Android externals in them
      local tLCExt
      repeat for each line tLCExt in tFiles
         revZipOpenArchive pFolder & slash & tLCExt, "read"
         if the result is not empty then
            builderLog "error", "Cannot open lcext file" && tLCExt
            exit toolsBuilderFilterExternals
         end if
         
         local tArchiveFiles
         put revZipEnumerateItems(pFolder & slash & tLCExt) into tArchiveFiles
         filter tArchiveFiles with "Android/External-*"
         revZipCloseArchive pFolder & slash & tLCExt
         
         if tArchiveFiles is empty then
            get shell(merge("rm -fv '[[ tLCExt ]]'"))
         end if
      end repeat
   end if

   -- If the directory contains no code any more, remove it
   put the files & return & the folders into tFiles
   filter lines of tFiles with regex pattern "^.*\.(so|dylib|bundle|dll|lcext)$"

   set the defaultFolder to tOldFolder
   if the result is not empty then
      builderLog "error", merge("Failed to enter '[[tOldFolder]]': [[the result]]")
   end if

   return tFiles is not empty
end toolsBuilderFilterExternals

private command toolsBuilderPrepareExt pEdition, pPlatform
   builderExtUnpack pEdition
   
   -- Remove un-needed files from the unpacked Ext collection
   local tOldFolder
   local tCollectionFolder
   put builderUnpackFolder(pEdition) & slash & "Ext" into tCollectionFolder
   put the defaultFolder into tOldFolder
   set the defaultFolder to tCollectionFolder
   repeat for each line tExtension in the folders
      -- Skip special names
      if tExtension is empty or tExtension is "." or tExtension is ".." then next repeat
      
      toolsBuilderFilterExternals tCollectionFolder & "/" & tExtension, pPlatform
      if the result is false then
         set the defaultFolder to tCollectionFolder
         get shell(merge("rm -rf '[[tExtension]]'"))
      end if
   end repeat
   set the defaultFolder to tOldFolder
end toolsBuilderPrepareExt

////////////////////////////////////////////////////////////////////////////////

private command toolsBuilderMakePackage pVersion, pEdition, pPlatform, pEngineFolders, pIdeFolder, pDocsFolder
   local tPackageFile
   
   local tEditionType
   local tBaseEditionType
   put pEdition into tEditionType
   
   put editionBaseEditionType(pEdition) into tBaseEditionType

   -- Escape the version (by replacing all non-alphanumeric characters with "_")
   local tVersionEscaped
   put replaceText(pVersion,"\W","_") into tVersionEscaped
   
   -- Compute the package temp folder
   local tTempFolder
   put builderWorkFolder() & slash & "tools-" & pPlatform & "-" & pVersion & ".tmp" into tTempFolder
   builderEnsureFolder tTempFolder
   
   -- Compute the package output file
   put builderWorkFolder() & slash & "tools-" & pPlatform & "-" & pVersion & ".zip" into tPackageFile
   builderEnsureFolderForFile tPackageFile
   
   -- Ensure that Ext is available for inclusion
   toolsBuilderPrepareExt pEdition, pPlatform
   
   --return tPackageFile
   
   -- Configure the package compiler appropriately
   local tPackager
   packageCompilerCreate tPackager
   packageCompilerSetReportCallback tPackager, the long id of me, "toolsBuilderPackageReport"
   
   -- Setup the temporary dir
   packageCompilerConfigure tPackager, "temporary", tTempFolder
   
   -- Configure the sources: ide, docs, macosx, windows, linux
   packageCompilerConfigureSource tPackager, "ide", pIdeFolder
   packageCompilerConfigureSource tPackager, "ide-support", builderRepoFolder() & slash & "ide-support"
   packageCompilerConfigureSource tPackager, "docs", pDocsFolder
   packageCompilerConfigureSource tPackager, "engine", pEngineFolders[pPlatform]
   packageCompilerConfigureSource tPackager, "macosx", pEngineFolders["macosx"]
   packageCompilerConfigureSource tPackager, "linux-x86", pEngineFolders["linux-x86"]
   packageCompilerConfigureSource tPackager, "linux-x86_64", pEngineFolders["linux-x86_64"]
   packageCompilerConfigureSource tPackager, "linux-armv6-hf", pEngineFolders["linux-armv6hf"]
   packageCompilerConfigureSource tPackager, "win-x86", pEngineFolders["win-x86"]
   packageCompilerConfigureSource tPackager, "win-x86_64", pEngineFolders["win-x86_64"]
   packageCompilerConfigureSource tPackager, "ios", pEngineFolders["ios"]
   packageCompilerConfigureSource tPackager, "android-armeabi-v7a", pEngineFolders["android-armeabi-v7a"]
   packageCompilerConfigureSource tPackager, "android-arm64-v8a", pEngineFolders["android-arm64-v8a"]
   packageCompilerConfigureSource tPackager, "android-x86", pEngineFolders["android-x86"]
   packageCompilerConfigureSource tPackager, "android-x86_64", pEngineFolders["android-x86_64"]
   packageCompilerConfigureSOurce tPackager, "emscripten", pEngineFolders["emscripten"]
   packageCompilerConfigureSource tPackager, "prebuilt", builderRepoFolder() & slash & "prebuilt"
   packageCompilerConfigureSource tPackager, "repo", builderRepoFolder()
   packageCompilerConfigureSource tPackager, "ext", builderUnpackFolder(pEdition)

   if editionIsInPrivateRepo(tEditionType) then
      packageCompilerConfigureSource tPackager, "private", builderPrivateRepoFolder()
   end if
   
   -- Now set up variables used by the description: TargetFolder, SupportFolder, ToolsFolder, TargetPlatform, TargetEdition
   packageCompilerConfigureVariable tPackager, "TargetFolder", "[[installFolder]]"
   packageCompilerConfigureVariable tPackager, "TargetEdition", tEditionType
   packageCompilerConfigureVariable tPackager, "BaseEdition", tBaseEditionType
   if pPlatform is "macosx" then
      packageCompilerConfigureVariable tPackager, "SupportFolder", "[[installFolder]]/Contents/Tools"
      packageCompilerConfigureVariable tPackager, "ToolsFolder", "[[installFolder]]/Contents/Tools"
   else
      packageCompilerConfigureVariable tPackager, "SupportFolder", "[[installFolder]]"
      packageCompilerConfigureVariable tPackager, "ToolsFolder", "[[installFolder]]"
   end if
   if pPlatform begins with "linux" then
      packageCompilerConfigureVariable tPackager, "TargetPlatform", "linux"
   else if pPlatform begins with "win" then
      packageCompilerConfigureVariable tPackager, "TargetPlatform", "windows"
   else
      packageCompilerConfigureVariable tPackager, "TargetPlatform", pPlatform
   end if
   packageCompilerConfigureVariable tPackager, "TodaysDate", "[[todaysDate]]"
   packageCompilerConfigureVariable tPackager, "VersionTag", pVersion
   packageCompilerConfigureVariable tPackager, "EscapedVersionTag", tVersionEscaped
   packageCompilerConfigureVariable tPackager, "ProductBranch", char 1 to 3 of pVersion
   packageCompilerConfigureVariable tPackager, "TargetArchitectures", "i386"
   if pPlatform ends with "-x86" then
      packageCompilerConfigureVariable tPackager, "TargetArchitecture", "x86"
   else if pPlatform ends with "-x86_64" then
      packageCompilerConfigureVariable tPackager, "TargetArchitecture", "x86_64"
   else if pPlatform ends with "-armv6hf" then
      packageCompilerConfigureVariable tPackager, "TargetArchitecture", "armv6-hf"
   end if
   
   -- Now all the names
   local tProductName
   put "LiveCode" && editionDisplayName(pEdition) into tProductName

   -- product tag is the lowercase product name with spaces removed
   local tProductTag
   put toLower(tProductName) into tProductTag
   replace space with empty in tProductTag
   
   packageCompilerConfigureVariable tPackager, "ProductTitle", tProductName && getReadableVersion(pVersion)
   packageCompilerConfigureVariable tPackager, "ProductTag", tProductTag & "_" & getTaggedVersion(pVersion)

   packageCompilerConfigureVariable tPackager, "ProductName", tProductName
   
   -- The edition tags.
   packageCompilerConfigureVariable tPackager, "EditionTagLower", "-" & toLower(tEditionType)
   packageCompilerConfigureVariable tPackager, "EditionTagUpper", "-" & editionTitleCase(tEditionType)
   packageCompilerConfigureVariable tPackager, "BaseEditionTagLower", "-" & toLower(tBaseEditionType)
   packageCompilerConfigureVariable tPackager, "BaseEditionTagUpper", "-" & editionTitleCase(tBaseEditionType)
   
   local tSuccess
   put true into tSuccess
   
   local tDescFile
   put builderRepoFolder() & slash & "Installer/package.txt" into tDescFile
   builderLog "message", "Parsing description file '" & tDescFile & "'"
   packageCompilerParse tPackager, url ("file:" & tDescFile)
   if the result is empty then
      toolsBuilderPackageReport "report", "Parsed description file '" & tDescFile & "'"
   else
      toolsBuilderPackageReport "error", item 2 to -1 of the result, item 1 of the result
      put false into tSuccess
   end if
   
   if tSuccess then
      if there is a file tPackageFile then
         delete file tPackageFile
      end if
      
      builderLog "message", "Building package '" & tPackageFile & "'"
      packageCompilerBuild tPackager, "LiveCode", tPackageFile
      if the result is empty then
         builderLog "report", "Built package '" & tPackageFile & "'"
      else
         toolsBuilderPackageReport "error", item 2 to -1 of the result, item 1 of the result
         put false into tSuccess
      end if
   end if
   
   packageCompilerDestroy tPackager
   
   if not tSuccess then
      throw "failed"
   end if
   
   return tPackageFile
end toolsBuilderMakePackage

on toolsBuilderPackageReport pType, pMessage, pLine
   if pLine is not empty then
      put "Line" && pLine && ":" && pMessage into pMessage
   end if
   builderLog pType, pMessage
end toolsBuilderPackageReport


////////////////////////////////////////////////////////////////////////////////

private command toolsBuilderMakeInstaller pVersion, pEdition, pPlatform, pIdeFolder, pPrivateFolder, pPackageFile
   -- Make sure no stack messages trigger
   lock messages
   
   -- Ensure the correct capitalisation of the edition name
   put editionTitleCase(pEdition) into pEdition
   
   -- Compute the deploy temp folder
   local tTempFolder
   put builderWorkFolder() & slash & "tools-" & pPlatform & "-" & pVersion & ".tmp" into tTempFolder
   builderEnsureFolder tTempFolder
   
   -- Compute the output file
   local tOutputFileStub, tOutputFileFolder
   put builderOutputFolder() into tOutputFileFolder
   put tOutputFileFolder & slash & getInstallerFilenameStub(pVersion, pPlatform, pEdition) into tOutputFileStub
   builderEnsureFolderForFile tOutputFileStub
   
   local tInstaller
   put the short name of stack (builderSystemFolder() & "/installer.livecode") into tInstaller
   
   -- Choose the apporpriate skin to use for the installer
   local tResources
   if pEdition is "Community" then
      put the short name of stack (builderSystemFolder() & "/tools_installer_resources.rev") into tResources
   else
      builderLog "report", "Commercial installer resources:" && builderCommercialResourceFolder() & slash & pEdition & "/tools_installer_resources.rev"
      put the short name of stack (builderCommercialResourceFolder() & slash & pEdition & "/tools_installer_resources.rev") into tResources
   end if
   set the mainStack of stack tResources to tInstaller
   
   -- Make sure the installer library knows what product it is (for logging file name)
   local tProductName
   put "LiveCode" && editionDisplayName(pEdition) into tProductName
   local tProductTag
   put toLower(tProductName) into tProductTag
   replace space with empty in tProductTag
   
   set the uProduct of stack tInstaller to tProductName
   set the uProductTitle of stack tInstaller to tProductName && getReadableVersion(pVersion)
   set the uProductTag of stack tInstaller to tProductTag & "-" & pVersion
   set the uProductDetails of stack tInstaller to loadTextFile(builderSystemFolder() & "/../Installer/description.txt")
   
   switch pEdition
      case "community"
         set the uProductLicense of stack tInstaller to loadTextFile(pIdeFolder & slash & "License Agreement.txt")
         break
      case "communityplus"
         set the uProductLicense of stack tInstaller to loadTextFile(pPrivateFolder & slash & "CommunityPlus License Agreement.txt")
         break
      default
         set the uProductLicense of stack tInstaller to loadTextFile(pPrivateFolder & slash & "License Agreement.txt")
         break
   end switch
   
   -- move behaviors to substacks
   local tInstallerBehaviors, tInstallerBehavior
   put files(builderSystemFolder() & "/installer") into tInstallerBehaviors
   filter tInstallerBehaviors without ".*"
   
   repeat for each line tInstallerBehavior in tInstallerBehaviors
      local tInstallerBehaviorPath
      put builderSystemFolder() & "/installer/" & tInstallerBehavior into tInstallerBehaviorPath
      if there is a stack tInstallerBehaviorPath then
         set the scriptOnly of stack tInstallerBehaviorPath to false
         set the mainstack of stack tInstallerBehaviorPath to tInstaller
      end if
   end repeat
   set the stackFiles of stack tInstaller to empty
   
   -- Save the restructured installer stack out to a temp file and delete from memory
   local tInstallerStackfile
   put tTempFolder & slash & "installer_stackfile.rev" into tInstallerStackfile
   builderLog "message", "Creating installer stackfile at '" & tInstallerStackfile & "'"
   save stack tInstaller as tInstallerStackfile
   if the result is not empty then
      builderLog "error", "Failed to create installer stackfile due to -" && the result
      delete stack tInstaller
      throw "failure"
   end if
   builderLog "report", "Created installer stackfile at '" & tInstallerStackfile & "'"
   delete stack tInstaller
   
   -- No more stack manipulations so unlock messages
   unlock messages
   
   -- Next build up the deploy info
   local tDietParams, tSignParams, tParams
   put "LiveCode Ltd" into tParams["version"]["CompanyName"]
   if pEdition is "Community" then
      put "LiveCode Community Installer" into tParams["version"]["FileDescription"]
      put "LiveCode Community Installer" into tParams["version"]["ProductName"]
   else
      put "LiveCode Installer" into tParams["version"]["FileDescription"]
      put "LiveCode Installer" into tParams["version"]["ProductName"]
   end if
   put "©" && word -1 of the long date & ". All rights reserved worldwide." into tParams["version"]["LegalCopyright"]
   put pVersion into tParams["version"]["ProductVersion"]
   put pVersion into tParams["version"]["FileVersion"]
   
   # TODO: Add the initialisation library and init libs in same way as normal standalone building
   # AL-2015-03-14: Use deploy parameters to include libURL scriptified stack in installer standalone
   local tAuxStackfiles
   if there is a stack "revLibUrl" then
      put the effective filename of stack "revLibUrl" into tAuxStackfiles
   else
      put builderRepoFolder() & slash & "ide-support" & slash & "revliburl.livecodescript" into tAuxStackfiles
   end if	
   put return & builderSystemFolder() & slash & "installer_utilities.livecodescript" after tAuxStackfiles
   put tAuxStackfiles into tParams["auxiliary_stackfiles"]
   
   // revLibURL needs to initialise its custom props, and extensionInitialize must therefore be called.
   put merge("send [[quote]]extensionInitialize[[quote]] to stack [[quote]]revLibUrl[[quote]]") into tParams["startup_script"]
   put return & "insert script of stack" && quote & "InstallerUtilities" & quote && "into back" after tParams["startup_script"]
   
   switch pPlatform
      case "win-x86"
      case "win-x86_64"
         -- Process the manifest appropriately
         local tManifestFile
         get windowsManifest()
         replace "[[Name]]" with tParams["version"]["ProductName"] in it
         replace "[[Description]]" with tParams["version"]["FileDescription"] in it
         put tTempFolder & "/manifest.xml" into tManifestFile
         put it into url ("binfile:" & tManifestFile)
         put tManifestFile into tParams["manifest"]
         
         -- First we deploy the installer unsigned
         put abstractPinFile(builderInstallerEngine(pPlatform)) into tParams["engine"]
         put tInstallerStackfile into tParams["stackfile"] 
         put pPackageFile into tParams["payload"]
         put tOutputFileStub & ".unsigned.exe" into tParams["output"]
         
         // libURL is needed in the timestamp processing
         start using stack (builderRepoFolder() & slash & "ide-support" & slash & "revliburl.livecodescript")

         if pEdition is not "Community" then
            put builderCommercialResourceFolder() & slash & pEdition & "/installer.ico" into tParams["appicon"]
         end if
         
         builderLog "message", "Deploying unsigned windows installer to '" && tParams["output"] & "'"
         _internal deploy windows tParams
         if the result is not empty then
            builderLog "error", "Deploy to windows failed - " & the result
            throw "failure"
         end if
         
         -- Next we sign the installer using the certificate (if available)
         local tCertificatesFolder
         put builderPrivateRepoFolder() & slash & "certificates" into tCertificatesFolder
         if there is a folder (tCertificatesFolder) then
            put tParams["output"] into tSignParams["input"]
            put tOutputFileStub & ".exe" into tSignParams["output"]
            put tCertificatesFolder & "/runrev_bin.spc" into tSignParams["certificate"]
            put tCertificatesFolder & "/runrev.pvk" into tSignParams["privatekey"]
            put word 1 of url ("file:" & tCertificatesFolder & "/runrev_password.txt") into tSignParams["passphrase"]
            put "http://timestamp.comodoca.com/authenticode" into tSignParams["timestamper"]
            put "http://www.livecode.com" into tSignParams["url"]
            if pEdition is "Community" then
               put "LiveCode Community Installer" into tSignParams["description"]
            else
               put "LiveCode Installer" into tSignParams["description"]
            end if
            
            builderLog "message", "Signing windows installer to '" && tSignParams["output"] & "'"
            _internal sign windows tSignParams
            if the result is not empty then
               builderLog "error", "Signing of windows executable failed - " & the result
               throw "failure"
            end if
         else
            put url ("binfile:" & tParams["output"]) into url ("binfile:" & tOutputFileStub & ".exe")
         end if
         
         builderLog "report", "Deployed windows installer to '" && tOutputFileStub & ".exe" & "'"
         break
         
      case "linux-x86"
      case "linux-x86_64"
      case "linux-armv6hf"
         put abstractPinFile(builderInstallerEngine(pPlatform)) into tParams["engine"]
         put tInstallerStackFile into tParams["stackfile"]
         put pPackageFile into tParams["payload"]
         if pPlatform is "linux-x86" then
            put tOutputFileStub & ".x86" into tParams["output"]
         else if pPlatform is "linux-x86_64" then
            put tOutputFileStub & ".x86_64" into tParams["output"]
         else -- if pPlatform is "linux-armv6hf"
            put tOutputFileStub & ".rpi" into tParams["output"]
         end if
         
         builderLog "message", "Deploying linux installer to '" && tParams["output"] & "'"
         _internal deploy linux tParams
         if the result is not empty then
            builderLog "error", "Deploy to linux failed - " & the result
            throw "failure"
         end if
         
         builderLog "report", "Deployed linux installer to '" && tParams["output"] & "'"
         break
         
      case "macosx"
         local tPlistFile
         put macPList() into tPlistFile
         replace "[[BundleId]]" with "com.runrev.installer" in tPlistFile
         replace "[[BundleName]]" with "livecodeinstaller" in tPlistFile
         
         create folder tOutputFileStub & ".app"
         create folder tOutputFileStub & ".app" & slash & "Contents"
         create folder tOutputFileStub & ".app" & slash & "Contents/MacOS"
         create folder tOutputFileStub & ".app" & slash & "Contents/Resources"
         put tPlistFile into url ("file:" & tOutputFileStub & ".app" & slash & "Contents/Info.plist")
         if pEdition is "Community" then
            put url ("binfile:" & builderCommunityResourceFolder() & "/Installer.icns")  into url ("binfile:" & tOutputFileStub & ".app" & slash & "Contents/Resources/Installer.icns")
         else
            put url ("binfile:" & builderCommercialResourceFolder() & slash & pEdition & "/Installer.icns") into url ("binfile:" & tOutputFileStub & ".app" & slash & "Contents/Resources/Installer.icns")
         end if
         
         -- Deploy the installer
         put abstractPinFile(builderInstallerEngine("macosx") & slash & "Contents/MacOS/Installer") into tParams["engine"]
         put tInstallerStackFile into tParams["stackfile"]
         put tOutputFileStub & ".app" & slash & "Contents/MacOS/Installer" into tParams["output"]
         
         -- We deploy as 64-bit only as 32-bit apps will be blocked on MacOS 10.15+
         put "x86-64" into tParams["architectures"]
         
         put url ("binfile:" & pPackageFile) into url ("binfile:" & tOutputFileStub & ".app" & slash & "Contents/Resources/payload")
         
         builderLog "message", "Deploying macosx installer to '" && tParams["output"] & "'"
         _internal deploy macosx tParams
         if the result is not empty then
            builderLog "error", "Deploy to macosx failed - " & the result
            throw "failure"
         end if
         
         builderLog "report", "Deployed macosx installer to '" && tOutputFileStub & ".app" & "'"
         break
   end switch
end toolsBuilderMakeInstaller

private function escapeArg pArg
   replace "'" with "\'" in pArg
   put "'" before pArg
   put "'" after pArg
   return pArg
end escapeArg

-- Needed on MacOS Sierra
private command clearExtendedAttributes pAppBundle
   -- make sure you have permission to clear extended attributes from bundle files
   get shell("chmod -R u+w" && pAppBundle)
   if the result is not zero then
      builderLog "error", "Setting permissions failed with error:" && it
      throw "failure"
   end if
      
   -- clear extended attributes from bundle files
   get shell("xattr -cr" && pAppBundle)
   if the result is not zero then
      builderLog "error", "Clearing extended attributes failed with error:" && it
      throw "failure"
   end if
end clearExtendedAttributes

private function findSigningIdentity
   get shell("/usr/bin/security -q find-identity -v")
   filter it with "*Developer ID Application:*"
   return it
end findSigningIdentity

command toolsBuilderMakeAppBundle pVersion, pEdition, pPlatform
   -- Compute the name of the installer bundle
   local tOutputFileStub, tOutputFileFolder
   put builderOutputFolder() into tOutputFileFolder
   put tOutputFileFolder & slash & getInstallerFilenameStub(pVersion, pPlatform, pEdition) into tOutputFileStub
   
   -- Remove any stale app bundles
   local tAppBundle
   put tOutputFileFolder & slash & getBundleFilenameStub(pVersion, pPlatform, pEdition) & ".app" into tAppBundle
   if there is a folder tAppBundle then
      builderLog "message", "Removing old macosx app bundle" && "'" & tAppBundle & "'"
      get shell ("chmod -Rvv u+w" && escapeArg(tAppBundle) && "&&" && "rm -r" && escapeArg(tAppBundle))
      if the result is not zero then
         builderLog "error", "Failed to remove old macosx app bundle:" && it
         throw "failure"
      end if
   end if
   
   -- Run the installer to generate the raw app bundle
   builderLog "message", "Installing macosx IDE to" && "'" & tAppBundle & "'"
   get shell(tOutputFileStub & ".app/Contents/MacOS/installer install -ui -log /dev/stderr -location" && escapeArg(tAppBundle))
   if the result is not zero then
      builderLog "error", "Failed to run macosx installer:" && it
      throw "failure"
   end if
   
   -- Remove the list of installed files from the app bundle (it isn't needed)
   delete file tAppBundle & "/.setup.txt"
   
   -- Copy just the executable out of the installer to use as the auto-updater
   -- (on OSX, the payload is separate and is un-neccessary)
   builderLog "message", "Adding auto-updater to app bundle"
   get shell ("cp" && escapeArg(tAppBundle & "/LiveCode Setup.app/Contents/MacOS/Installer") && escapeArg(tAppBundle & "/Contents/MacOS/autoupdater"))
   if the result is not zero then
      builderLog "error", "Failed to add auto-updater:" && it
      throw "failure"
   end if
   
   -- Remove the rest of the installer from the app bundle
   builderLog "message", "Removing installer from app bundle"
   get shell("rm -r" && escapeArg(tAppBundle & "/LiveCode Setup.app"))
   if the result is not zero then
      builderLog "error", "Failed to clean macosx app bundle:" & it
      throw "failure"
   end if
   
   -- Copy a shell script and AppleScript into the app bundle to catch attempts 
   -- from old auto-updaters to launch the installer
   local tEngineFolder
   builderFetchEngine empty, "macosx"
   put the result into tEngineFolder
   builderLog "message", "Adding installer shim to app bundle"
   get shell ("install -m755" && escapeArg(tEngineFolder & "/installer-stub") && escapeArg(tAppBundle & "/Contents/MacOS/installer"))
   if the result is not zero then
      builderLog "error", "Failed to copy installer stub shell script:" && it
      throw "failure"
   end if
   get shell ("mkdir -p" && escapeArg(tAppBundle & "/Contents/Resources/Installer"))
   if the result is not zero then
      builderLog "error", "Failed to create installer resources directory:" && it
      throw "failure"
   end if
   get shell ("osacompile" && "-o" && escapeArg(tAppBundle & "/Contents/Resources/Installer/ShowDmgWindow.scpt") && escapeArg(builderCommunityResourceFolder() & "/dmg/open-dmg.applescript"))
   if the result is not zero then
      builderLog "error", "Failed to compile installer stub AppleScript:" && it
      throw "failure"
   end if
   
   -- Remove a problematic (but un-neccessary) file from the CEF framework bundle
   -- Doing it here is ugly, but is temporary (only needed in 6.7 and 7.1).
   get shell ("rm -f" && escapeArg(tAppBundle & "/Contents/Frameworks/Chromium Embedded Framework.framework/Manifest"))

   -- Ensure the permissions on the app bundle are correct
   -- This needs to be done *before* signing as it affects the signature
   -- Can't remove write permission yet, though, as signing writes into the bundle
   get shell ("chmod -Rvv ugo+rX" && escapeArg(tAppBundle))
   if the result is not zero then
      builderLog "error", "Failed to set app bundle permissions:" && it
      throw "failure"
   end if

   -- Find the signing identity
   local tSigningId
   get findSigningIdentity()
   put it into tSigningId
   if it is not empty then
      -- Sign the iOS simulator dylibs
      local tIosRuntimes
      put folders(tAppBundle & "/Contents/Tools/Runtime/iOS") into tIosRuntimes
      repeat for each line tLine in tIosRuntimes
         -- Ignore device builds
         if not (tLine begins with "Simulator-") then next repeat

         -- Get the list of all iOS externals in this folder
         local tRuntimeFolder
         local tRuntimeContents
         put tAppBundle & "/Contents/Tools/Runtime/iOS/" & tLine into tRuntimeFolder
         put files(tRuntimeFolder) into tRuntimeContents
         repeat for each line tFile in tRuntimeContents
            -- Ignore files that aren't dylibs
            local tFullPath
            put tRuntimeFolder & "/" & tFile into tFullPath
            get shell("file" && escapeArg(tFullPath))
            if tFile is "Standalone" or "Mach-O" is not among the words of it then next repeat

            clearExtendedAttributes escapeArg(tFullPath)
			
            -- Sign the extension
            -- The "--force" parameter strips the existing signature (which was
            -- made using a not-valid-for-distribution profile)
            builderLog "message", "Signing iOS extension" && "'" & tFullPath & "'"
            get shell("/usr/bin/codesign --sign" && word 2 of tSigningId && "--force --verbose=2" && escapeArg(tFullPath))
            if the result is not zero then
                builderLog "error", "Could not sign iOS extension:" && it
                throw "failure"
            end if
         end repeat
      end repeat
      
      -- Sign any extension code resources
      local tExtensions
      put folders(tAppBundle & "/Contents/Tools/Extensions") into tExtensions
      repeat for each line tExtension in tExtensions
         local tCodeFolders
         put folders(tAppBundle & "/Contents/Tools/Extensions/" & tExtension & "/code") into tCodeFolders
         filter tCodeFolders with "*iphonesimulator*"
         if tCodeFolders is empty then next repeat
         
         
         repeat for each line tCodeFolder in tCodeFolders
            local tDylibs
            filter files(tAppBundle & "/Contents/Tools/Extensions/" & tExtension & "/code/" & tCodeFolder) \
               with "*.dylib" into tDylibs
            if tDylibs is empty then next repeat
            
            repeat for each line tFile in tDylibs
               -- Ignore files that aren't dylibs
               local tFullCodePath
               put tAppBundle & "/Contents/Tools/Extensions/" & tExtension & "/code/" & tCodeFolder & "/" & tFile into tFullCodePath
               get shell("file" && escapeArg(tFullCodePath))
               if "Mach-O" is not among the words of it then next repeat

               clearExtendedAttributes escapeArg(tFullCodePath)
   			
               -- Sign the extension
               -- The "--force" parameter strips the existing signature (which was
               -- made using a not-valid-for-distribution profile)
               builderLog "message", "Signing iOS extension" && "'" & tFullCodePath & "'"
               get shell("/usr/bin/codesign --sign" && word 2 of tSigningId && "--force --verbose=2" && escapeArg(tFullCodePath))
               if the result is not zero then
                   builderLog "error", "Could not sign iOS extension:" && it
                   throw "failure"
               end if
            end repeat
         end repeat
      end repeat

      clearExtendedAttributes escapeArg(tAppBundle)
    		
      builderLog "message", "Signing macosx app bundle" && "'" & tAppBundle & "'"
      wait 1 second
      get shell("/usr/bin/codesign --sign" && word 2 of tSigningId && "--deep --verbose=2" && escapeArg(tAppBundle))
      if the result is not zero then
         builderLog "error", "Could not sign macosx app bundle:" && it
         throw "failure"
      end if
      builderLog "report", "Successfully signed macosx app bundle" && "'" & tAppBundle & "'"
   else
      builderLog "error", "No valid identity found for signing OSX app bundle"
      throw "failure"
   end if

   -- One last permission change (but a non-signature-breaking one) to prevent accidental IDE modifications
   get shell ("chmod -Rvv -w" && escapeArg(tAppBundle & slash & "Contents"))
   if the result is not zero then
      builderLog "error", "Failed to make app bundle read-only:" && it
      throw "failure"
   end if
end toolsBuilderMakeAppBundle

command toolsBuilderMakeDisk pVersion, pEdition, pPlatform
   -- Get the name of the app bundle that is being built
   local tOutputFileFolder
   local tDmgFolder
   local tAppBundle
   put builderOutputFolder() into tOutputFileFolder
   put builderOutputFolder() & slash & "DMG" into tDmgFolder
   put tOutputFileFolder & slash & getBundleFilenameStub(pVersion, pPlatform, pEdition) & ".app" into tAppBundle
   
   -- DMG output filename
   local tDmgFile
   local tTempDmg
   put tOutputFileFolder & slash & getDmgFilenameStub(pVersion, pPlatform, pEdition) & ".dmg" into tDmgFile
   put tOutputFileFolder & slash & getDmgFilenameStub(pVersion, pPlatform, pEdition) & "-rw.dmg" into tTempDmg
   
   -- Remove any stale DMG folder
   if there is a folder tDmgFolder then
      builderLog "message", "Removing old DMG directory"
      get shell ("chmod -Rvv u+w" && escapeArg(tDmgFolder) && "&&" && "rm -r" && escapeArg(tDmgFolder))
      if the result is not zero then
         builderLog "error","Failed to remove old DMG directory:" && it
         throw "failure"
      end if
   end if

   -- Create a new DMG folder
   create folder tDmgFolder
   
   -- Copy the app bundle into the DMG contents folder
   -- Note the need to preserve permissions!
   builderLog "message", "Copying app bundle into DMG directory"
   get shell ("cp -Rp" && escapeArg(tAppBundle) && escapeArg(tDmgFolder & slash))
   if the result is not zero then
      builderLog "error", "Failed to copy app bundle into DMG directory:" && it
      throw "failure"
   end if
   
   builderLog "message", "Building Mac DMG to '" & tDmgFile & "'"
   if there is a file tDmgFile then
      delete file tDmgFile
   end if
   if there is a file tTempDmg then
      delete file tTempDmg
   end if
   if there is a file tDmgFile or there is a file tTempDmg then
      builderLog "error", "Could not remove existing dmg - make sure it is not mounted"
      throw "failure"
   end if
   
   -- The following is sometimes needed to work around bugs in hdiutil
   --local tTrashFile
   --put builderWorkspaceFolder() & "/deploy/.Trash" into tTrashFile
   --put empty into URL ("file:" & tTrashFile)
   
   -- Attempt to detach any DMGs with the name we're about to create
   -- We ignore failures here as it may not be mounted and an already-mounted
   -- disk will cause a very visible error during the next step.
   local tVolumeName
   put getBundleFilenameStub(pVersion, pPlatform, pEdition) into tVolumeName
   get shell ("hdiutil detach" && escapeArg(tVolumeName))
   
   -- Generate an initial read-write DMG and mount it
   -- This needs to be run with root privileges to get the ownership correct
   -- If the $GENERATE_DMG_SCRIPT env var is set, the script identified by
   -- that path will be used instead of the one stored in the repo
   builderLog "message", "Generating initial DMG"
   local tGenerateScript
   if $GENERATE_DMG_SCRIPT is not empty then
      -- Check that the generate script is identical to the in-repo copy
      -- so it is kept up-to-date
      local tLocalScript
      local tRepoScript
      get shell("which" && escapeArg($GENERATE_DMG_SCRIPT))
      put url("file:" & line 1 of it) into tLocalScript
      put url("file:" & builderSystemFolder() & "/generate-dmg.sh") into tRepoScript
      if tLocalScript is not tRepoScript then
         builderLog "error", "Local DMG generation script differs from the repo copy (please update)"
         throw "failure"
      end if
      
      put "sudo -n" && $GENERATE_DMG_SCRIPT into tGenerateScript
   else if $GENERATE_DMG_SUDO is not empty and $GENERATE_DMG_SUDO is not 0 then
      put "sudo -n" && escapeArg(builderSystemFolder() & "/generate-dmg.sh") into tGenerateScript
   else
      put escapeArg(builderSystemFolder() & "/generate-dmg.sh") into tGenerateScript
   end if
   get shell (tGenerateScript && escapeArg(tVolumeName) && escapeArg(tDmgFolder) && escapeArg(tTempDmg))
   if "created:" is not in it then
      builderLog "error", "Failed to create initial DMG:" && it
      throw "failure"
   end if
   
   -- Generate the multi-resolution TIFF for the DMG background image
   local tBackgroundImage
   put tOutputFileFolder & slash & "bg-" & pEdition & ".tiff" into tBackgroundImage
   builderLog "message", "Generating DMG background image"
   get shell ("tiffutil -cathidpicheck" && escapeArg(builderCommunityResourceFolder() & "/dmg/bg-" & pEdition & ".png") \
         && escapeArg(builderCommunityResourceFolder() & "/dmg/bg-" & pEdition & "@2x.png") \
         && "-out" && escapeArg(tBackgroundImage))
   if the result is not zero then
      builderLog "error","Failed to generate DMG background image:" && it
      throw "failure"
   end if
   
   -- Run the DMG prettifier script (this unmounts the DMG at the end)
   builderLog "message", "Setting DMG background and styling"
   get shell (builderRepoFolder() & "/tools/make-dmg-pretty.sh" && escapeArg(tVolumeName) && escapeArg(tBackgroundImage))
   if it is not empty or the result is not zero then
      builderLog "error", "Failed to apply DMG styling:" && it
      throw "failure"
   end if 
   
   -- Convert the DMG into a read-only, compressed DMG
   builderLog "message", "Compressing DMG"
   get shell ("hdiutil convert" && escapeArg(tTempDmg) && "-format UDBZ -o" && escapeArg(tDmgFile))
   if the result is not zero or not line -1 of it begins with "created:" then
      builderLog "error", "Failed to compress DMG:" && it
      throw "failure"
   end if
   
   builderLog "report", "Deployed macosx dmg to '" & tDmgFile & "'"
end toolsBuilderMakeDisk

////////////////////////////////////////////////////////////////////////////////

function getTaggedVersion pVersion
   set the itemDelimiter to "-"
   if item 2 of pVersion is "gm" then
      delete item -2 to -1 of pVersion
   end if
   replace "-" with "_" in pVersion
   replace "." with "_" in pVersion
   return pVersion
end getTaggedVersion

function getFullReadableVersion pVersion
   local tNumber
   set the itemDelimiter to "-"
   put item 1 of pVersion into tNumber
   
   if item 2 of pVersion is "gm" then
      delete item -2 to -1 of pVersion
   end if
   
   local tTag
   if item 2 of pVersion is not empty then
      put item 2 to -1 of pVersion into tTag
      replace "-" with space in tTag
      put " (" & tTag & ")" into tTag
   end if
   
   return tNumber & tTag
end getFullReadableVersion

function getReadableVersion pVersion
   put getFullReadableVersion(pVersion) into pVersion
   set the itemDelimiter to space

   local tNumber, tTag
   put item 1 of pVersion into tNumber

   if tNumber ends with ".0" then
      delete char -2 to -1 of tNumber
      put tNumber into item 1 of pVersion
   end if

   return pVersion
end getReadableVersion

function getBundleFilenameStub pVersion, pPlatform, pEdition
   return "LiveCode" && editionDisplayName(pEdition) && getFullReadableVersion(pVersion)
end getBundleFilenameStub

function getInstallerFilenameStub pVersion, pPlatform, pEdition
   set the itemDelimiter to "-"
   if item 2 of pVersion is "gm" then
      delete item -2 to -1 of pVersion
   end if
   replace "-" with "_" in pVersion
   replace "." with "_" in pVersion
   if pPlatform is "macosx" then
      put "Mac" into pPlatform
   else if pPlatform is "win-x86" then
      put "Windows-x86" into pPlatform
   else if pPlatform is "win-x86_64" then
      put "Windows-x86_64" into pPlatform
   else if pPlatform begins with "linux" then
      put "Linux" into pPlatform
   end if

   return "LiveCode" & editionTitleCase(pEdition) & "Installer-" & pVersion & "-" & pPlatform
end getInstallerFilenameStub

function getDmgFilenameStub pVersion, pPlatform, pEdition
      set the itemDelimiter to "-"
   if item 2 of pVersion is "gm" then
      delete item -2 to -1 of pVersion
   end if
   replace "-" with "_" in pVersion
   replace "." with "_" in pVersion
   if pPlatform is "macosx" then
      put "Mac" into pPlatform
   else if pPlatform is "win-x86" then
      put "Windows-x86" into pPlatform
   else if pPlatform is "win-x86_64" then
      put "Windows-x86_64" into pPlatform
   else if pPlatform is "linux" or pPlatform is "linux-x64" or pPlatform is "linux-armv6hf" then
      put "Linux" into pPlatform
   end if
   return "LiveCode" & editionTitleCase(pEdition) & "-" & pVersion & "-" & pPlatform
end getDmgFilenameStub

function loadTextFile pFile
   get url ("binfile:" & pFile)
   replace numToChar(13) & numToChar(10) with return in it
   replace numToChar(13) with return in it
   return it
end loadTextFile
