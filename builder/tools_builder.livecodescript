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
   if the platform is not "macos" then
      get "windows linux linux-x64 linux-armv6hf"
   else
      get "windows linux linux-x64 linux-armv6hf macosx ios android"
   end if
      
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

////////////////////////////////////////////////////////////////////////////////

private command toolsBuilderMakePackage pVersion, pEdition, pPlatform, pEngineFolders, pIdeFolder, pDocsFolder
   local tPackageFile
   
   local tEditionType
   put pEdition into tEditionType
   
   -- Compute the package temp folder
   local tTempFolder
   put builderWorkFolder() & slash & "tools-" & pPlatform & "-" & pVersion & ".tmp" into tTempFolder
   builderEnsureFolder tTempFolder
   
   -- Compute the package output file
   put builderWorkFolder() & slash & "tools-" & pPlatform & "-" & pVersion & ".zip" into tPackageFile
   builderEnsureFolderForFile tPackageFile
   
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
   packageCompilerConfigureSource tPackager, "linux-i386", pEngineFolders["linux"]
   packageCompilerConfigureSource tPackager, "linux-x86_64", pEngineFolders["linux-x64"]
   packageCompilerConfigureSource tPackager, "linux-armv6-hf", pEngineFolders["linux-armv6hf"]
   packageCompilerConfigureSource tPackager, "windows", pEngineFolders["windows"]
   packageCompilerConfigureSource tPackager, "ios", pEngineFolders["ios"]
   packageCompilerConfigureSource tPackager, "android", pEngineFolders["android"]
   packageCompilerConfigureSource tPackager, "prebuilt", builderRepoFolder() & slash & "prebuilt"
   if tEditionType is "Commercial" then
      packageCompilerConfigureSource tPackager, "private", builderPrivateRepoFolder()
   end if
   
   -- Now set up variables used by the description: TargetFolder, SupportFolder, ToolsFolder, TargetPlatform, TargetEdition
   packageCompilerConfigureVariable tPackager, "TargetFolder", "[[installFolder]]"
   packageCompilerConfigureVariable tPackager, "TargetEdition", tEditionType
   if pPlatform is "macosx" then
      packageCompilerConfigureVariable tPackager, "SupportFolder", "[[installFolder]]/Contents/Tools"
      packageCompilerConfigureVariable tPackager, "ToolsFolder", "[[installFolder]]/Contents/Tools"
   else
      packageCompilerConfigureVariable tPackager, "SupportFolder", "[[installFolder]]"
      packageCompilerConfigureVariable tPackager, "ToolsFolder", "[[installFolder]]"
   end if
   if pPlatform is "linux-x64" or pPlatform is "linux-armv6hf" or pPlatform is "linux" then
      packageCompilerConfigureVariable tPackager, "TargetPlatform", "linux"
   else
      packageCompilerConfigureVariable tPackager, "TargetPlatform", pPlatform
   end if
   packageCompilerConfigureVariable tPackager, "TodaysDate", "[[todaysDate]]"
   packageCompilerConfigureVariable tPackager, "VersionTag", pVersion
   packageCompilerConfigureVariable tPackager, "TargetArchitectures", "i386"
   if pPlatform is "linux" then
      packageCompilerConfigureVariable tPackager, "TargetArchitecture", "i386"
   else if pPlatform is "linux-x64" then
      packageCompilerConfigureVariable tPackager, "TargetArchitecture", "x86_64"
   else if pPlatform is "linux-armv6hf" then
      packageCompilerConfigureVariable tPackager, "TargetArchitecture", "armv6-hf"
   end if
   
   -- Now all the names
   if pEdition is "Community" then
      packageCompilerConfigureVariable tPackager, "ProductTitle", "LiveCode Community" && getReadableVersion(pVersion)
      packageCompilerConfigureVariable tPackager, "ProductTag", "livecodecommunity_" & getTaggedVersion(pVersion)
      if pPlatform is "Linux" or pPlatform is "Linux-x64" or pPlatform is "Linux-armv6hf" then
         packageCompilerConfigureVariable tPackager, "ProductName", "livecodecommunity"
      else
         packageCompilerConfigureVariable tPackager, "ProductName", "LiveCode Community"
      end if
   else
      packageCompilerConfigureVariable tPackager, "ProductTitle", "LiveCode" && getReadableVersion(pVersion)
      packageCompilerConfigureVariable tPackager, "ProductTag", "livecode_" & getTaggedVersion(pVersion)
      if pPlatform is "Linux" or pPlatform is "Linux-x64" or pPlatform is "Linux-armv6hf" then
         packageCompilerConfigureVariable tPackager, "ProductName", "livecode"
      else
         packageCompilerConfigureVariable tPackager, "ProductName", "LiveCode"
      end if
   end if
   
   -- The edition tags.
   packageCompilerConfigureVariable tPackager, "EditionTagLower", "-" & toLower(char 1 of tEditionType) & char 2 to -1 of tEditionType
   packageCompilerConfigureVariable tPackager, "EditionTagUpper", "-" & toUpper(char 1 of tEditionType) & char 2 to -1 of tEditionType
   
   local tSuccess
   put true into tSuccess
   
   local tDescFile
   put pIdeFolder & slash & "Installer/package.txt" into tDescFile
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
   put the short name of stack (builderSystemFolder() & "/installer.rev") into tInstaller
   
   -- Choose the apporpriate skin to use for the installer
   local tResources
   if pEdition is "Community" then
      put the short name of stack (builderSystemFolder() & "/tools_installer_resources.rev") into tResources
   else
      put the short name of stack (builderCommercialResourceFolder() & "/tools_installer_resources.rev") into tResources
   end if
   set the mainStack of stack tResources to tInstaller
   
   -- Make sure the installer library knows what product it is (for logging file name)
   if pEdition is "Community" then
      set the uProduct of stack tInstaller to "LiveCode Community"
      set the uProductTitle of stack tInstaller to "LiveCode Community" && getReadableVersion(pVersion)
      set the uProductTag of stack tInstaller to "livecodecommunity-" & pVersion
      set the uProductDetails of stack tInstaller to loadTextFile(pIdeFolder & slash & "Installer/description.txt")
      set the uProductLicense of stack tInstaller to loadTextFile(pIdeFolder & slash & "License Agreement.txt")
   else
      set the uProduct of stack tInstaller to "LiveCode"
      set the uProductTitle of stack tInstaller to "LiveCode" && getReadableVersion(pVersion)
      set the uProductTag of stack tInstaller to "livecode-" & pVersion
      set the uProductDetails of stack tInstaller to loadTextFile(pIdeFolder & slash & "Installer/description.txt")
      set the uProductLicense of stack tInstaller to loadTextFile(pPrivateFolder & slash & "License Agreement.txt")
   end if
   
   
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
   put "RunRev Ltd" into tParams["version"]["CompanyName"]
   if pEdition is "Community" then
      put "RunRev LiveCode Community Installer" into tParams["version"]["FileDescription"]
      put "LiveCode Community Installer" into tParams["version"]["ProductName"]
   else
      put "RunRev LiveCode Installer" into tParams["version"]["FileDescription"]
      put "LiveCode Installer" into tParams["version"]["ProductName"]
   end if
   put "©" && word -1 of the long date & ". All rights reserved worldwide." into tParams["version"]["LegalCopyright"]
   put pVersion into tParams["version"]["ProductVersion"]
   put pVersion into tParams["version"]["FileVersion"]
   
   # AL-2015-03-14: Use deploy parameters to include libURL scriptified stack in installer standalone
   local tAuxStackfiles
   if there is a stack "revLibUrl" then
      put the effective filename of stack "revLibUrl" into tAuxStackfiles
   else
      put builderRepoFolder() & slash & "ide-support" & slash & "revliburl.livecodescript" into tAuxStackfiles
   end if	
   put return & builderSystemFolder() & slash & "installer_utilities.livecodescript" after tAuxStackfiles
   
   start using stack (builderRepoFolder() & slash & "ide-support" & slash & "revsblibrary.livecodescript")
   repeat for each line tStackFile in tAuxStackfiles
      local tTempStackFile
      put builderMakeTemporaryFile(tTempFolder, "stack") into tTempStackFile
      revSBResaveScriptOnlyStackAsProperStackFile the short name of stack tStackFile, tStackFile, tTempStackFile
      put tTempStackFile & return after tParams["auxiliary_stackfiles"]
   end repeat
   delete the last char of tParams["auxiliary_stackfiles"]
   
   put "insert script of stack" && quote & "revLibUrl" & quote && "into back" into tParams["startup_script"]
   put return & "insert script of stack" && quote & "InstallerUtilities" & quote && "into back" after tParams["startup_script"]
   
   switch pPlatform
      case "windows"
         -- Process the manifest appropriately
         local tManifestFile
         get windowsManifest()
         replace "[[Name]]" with tParams["version"]["ProductName"] in it
         replace "[[Description]]" with tParams["version"]["FileDescription"] in it
         put tTempFolder & "/manifest.xml" into tManifestFile
         put it into url ("binfile:" & tManifestFile)
         put tManifestFile into tParams["manifest"]
         
         -- First we deploy the installer unsigned
         put abstractPinFile(builderInstallerEngine("windows")) into tParams["engine"]
         put tInstallerStackfile into tParams["stackfile"] 
         put pPackageFile into tParams["payload"]
         put tOutputFileStub & ".unsigned.exe" into tParams["output"]
         
         if pEdition is "Commercial" then
            put builderCommercialResourceFolder() & "/installer.ico" into tParams["appicon"]
         end if
         
         builderLog "message", "Deploying unsigned windows installer to '" && tParams["output"] & "'"
         _internal deploy windows tParams
         if the result is not empty then
            builderLog "error", "Deploy to windows failed - " & the result
            throw "failure"
         end if
         
         -- Next we sign the installer using the certificate (if available)
         if there is a folder (builderSystemFolder() & slash & "certificates") then
            put tParams["output"] into tSignParams["input"]
            put tOutputFileStub & ".exe" into tSignParams["output"]
            put builderSystemFolder() & slash & "certificates/runrev_bin.spc" into tSignParams["certificate"]
            put builderSystemFolder() & slash & "certificates/runrev.pvk" into tSignParams["privatekey"]
            put word 1 of url ("file:" & builderSystemFolder() & slash & "certificates/runrev_password.txt") into tSignParams["passphrase"]
            put "http://timestamp.verisign.com/scripts/timstamp.dll" into tSignParams["timestamper"]
            put "http://www.runrev.com" into tSignParams["url"]
            if pEdition is "Community" then
               put "RunRev LiveCode Community Installer" into tSignParams["description"]
            else
               put "RunRev LiveCode Installer" into tSignParams["description"]
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
         
      case "linux"
      case "linux-x64"
      case "linux-armv6hf"
         put abstractPinFile(builderInstallerEngine(pPlatform)) into tParams["engine"]
         put tInstallerStackFile into tParams["stackfile"]
         put pPackageFile into tParams["payload"]
         if pPlatform is "linux" then
            put tOutputFileStub & ".x86" into tParams["output"]
         else if pPlatform is "linux-x64" then
            put tOutputFileStub & ".x64" into tParams["output"]
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
         replace "[[BundleName]]" with "runrevinstaller" in tPlistFile
         
         create folder tOutputFileStub & ".app"
         create folder tOutputFileStub & ".app" & slash & "Contents"
         create folder tOutputFileStub & ".app" & slash & "Contents/MacOS"
         create folder tOutputFileStub & ".app" & slash & "Contents/Resources"
         put tPlistFile into url ("file:" & tOutputFileStub & ".app" & slash & "Contents/Info.plist")
         if pEdition is "Community" then
            put url ("binfile:" & builderInstallerEngine("macosx") & slash & "Contents/Resources/Installer.icns") into url ("binfile:" & tOutputFileStub & ".app" & slash & "Contents/Resources/Installer.icns")
         else
            put url ("binfile:" & pPrivateFolder & slash & "rsrc/Installer.icns") into url ("binfile:" & tOutputFileStub & ".app" & slash & "Contents/Resources/Installer.icns")
         end if
         
         -- Get us an installer by dieting the source installer
         if false then
            put abstractPinFile(builderInstallerEngine("macosx") & slash & "Contents/MacOS/installer") into tDietParams["input"]
            put tTempFolder & slash & "macosx-installer-x86" into tDietParams["output"]
            put true into tDietParams["keep_x86"]
            put false into tDietParams["keep_ppc"]
            builderLog "message", "Dieting input installer down to x86"
            --_internal diet macosx tDietParams
            dietAndStrip "x86", tDietParams["input"], tDietParams["output"]
            if the result is not empty then
               builderLog "error", "Dieting input installer to x86 failed -" && the result
               throw "failure"
            end if
            
            put tTempFolder & slash & "macosx-installer-x86" into tParams["engine_x86"]
         end if
         
         put abstractPinFile(builderInstallerEngine("macosx") & slash & "Contents/MacOS/installer") into tParams["engine_x86"]
         put tInstallerStackFile into tParams["stackfile"]
         put tOutputFileStub & ".app" & slash & "Contents/MacOS/installer" into tParams["output"]
         
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

command toolsBuilderMakeDisk pVersion, pEdition, pPlatform, tInputZip
   -- Compute the output file
   local tOutputFileStub, tOutputFileFolder
   put builderOutputFolder() into tOutputFileFolder
   put tOutputFileFolder & slash & getInstallerFilenameStub(pVersion, pPlatform, pEdition) into tOutputFileStub
   builderEnsureFolderForFile tOutputFileStub
   
   -- Unzip the input zip
   builderLog "message", "Unzipping macosx instaler to " && tOutputFileStub & ".app" & "'"
   get shell("unzip -qq -o -d" && quote & tOutputFileFolder & quote && quote & tInputZip & quote)
   if it is not empty then
      builderLog "error", "Could not extract macosx app from zip:" && it
      throw "failure"
   end if
   
   get shell("/usr/bin/security -q find-identity -v")
   filter it with "*Developer ID Application: Runtime Revolution Ltd*"
   if it is not empty then
      builderLog "message", "Signing macosx installer'" && tOutputFileStub & ".app" & "'"
      wait 1 second
      get shell("/usr/bin/codesign -s" && word 2 of it && "'" & tOutputFileStub & ".app'")
      if it is not empty then
         builderLog "error", "Could not macosx sign installer:" && it
         throw "failure"
      end if
      builderLog "report", "Successfully signed macosx installer'" && tOutputFileStub & ".app" & "'"
   end if
   
   local tDmgFile, tDmgName, tDmgAppFolder
   put tOutputFileStub & ".dmg" into tDmgFile
   if pEdition is "Community" then
      put "LiveCode Community" && getReadableVersion(pVersion) && "Installer" into tDmgName
      put (tOutputFileFolder & slash & "Install LiveCode Community" && getReadableVersion(pVersion) & ".app") into tDmgAppFolder
   else
      put "LiveCode" && getReadableVersion(pVersion) && "Installer" into tDmgName
      put (tOutputFileFolder & slash & "Install LiveCode" && getReadableVersion(pVersion) & ".app") into tDmgAppFolder
   end if
   builderLog "message", "Building Mac DMG to '" & tDmgFile & "'"
   if there is a file tDmgFile then
      delete file tDmgFile
      if there is a file tDmgFile then
         builderLog "error", "Could not remove existing dmg - make sure it is not mounted"
         throw "failure"
      end if
   end if
   
   --local tTrashFile
   --put builderWorkspaceFolder() & "/deploy/.Trash" into tTrashFile
   --put empty into URL ("file:" & tTrashFile)
   
   -- Do the DMG build
   rename (tOutputFileStub & ".app") to tDmgAppFolder
   -- SN-2015-03-09: [[ DMG creation failure ]] Ensure that hdiutil will allocate enough
   --  memory if the installer end up being over a gigabyte large.
   get shell("hdiutil create -fs HFS+ -size 1G -volname" && quote & tDmgName & quote && "-srcfolder" && quote & tDmgAppFolder & quote && quote & tDmgFile & quote)
   rename tDmgAppFolder to (tOutputFileStub & ".app")
   if "created:" is not in it then
      builderLog "error", "Dmg creation failed -" && the last line of it
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

function getReadableVersion pVersion
   local tNumber
   set the itemDelimiter to "-"
   put item 1 of pVersion into tNumber
   if tNumber ends with ".0" then
      delete char -2 to -1 of tNumber
   end if
   
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
end getReadableVersion

function getInstallerFilenameStub pVersion, pPlatform, pEdition
   set the itemDelimiter to "-"
   if item 2 of pVersion is "gm" then
      delete item -2 to -1 of pVersion
   end if
   replace "-" with "_" in pVersion
   replace "." with "_" in pVersion
   if pPlatform is "macosx" then
      put "Mac" into pPlatform
   else if pPlatform is "windows" then
      put "Windows" into pPlatform
   else if pPlatform is "linux" or pPlatform is "linux-x64" or pPlatform is "linux-armv6hf" then
      put "Linux" into pPlatform
   end if
   if pEdition is "Community" then
      return "LiveCodeCommunityInstaller-" & pVersion & "-" & pPlatform
   else
      return "LiveCodeInstaller-" & pVersion & "-" & pPlatform
   end if
end getInstallerFilenameStub

function loadTextFile pFile
   get url ("binfile:" & pFile)
   replace numToChar(13) & numToChar(10) with return in it
   replace numToChar(13) with return in it
   return it
end loadTextFile

