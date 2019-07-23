script "revSaveAsIOSStandalone"

-- Each stackfile has:
--   moveSubstacks
--   renameGeneric
--   substackFolder
--
-- Each stack has:
--   destroyStack
--   password

command revSaveAsMobileStandalone pStack, pAppBundle, pTarget, pSettings
   local tTargetType
   if pTarget contains "device" then
      put "ios device" into tTargetType
   else
      put "ios simulator" into tTargetType
   end if
   
   try
      dispatch "savingMobileStandalone" to stack pStack with tTargetType, pAppBundle
   end try
   
   try
      revSaveAsMobileStandaloneMain pStack, pAppBundle, pTarget, pSettings
   catch tError
      if revTestEnvironment() then
         return tError
      end if
   end try
   
   try
      if tError is not empty then
         put empty into pAppBundle
      end if
      dispatch "mobileStandaloneSaved" to stack pStack with tTargetType, pAppBundle
   end try
   
   if tError is not empty then
      throw tError
   end if
   
   -- SN-2014-01-31: [[ Bug 11124 ]] No warning when an external file is missing
   if revStandaloneGetWarnings() is not empty and pTarget contains "Simulator" then
      set the cWarnings of stack "revBuildResults" to revStandaloneGetWarnings()
   end if
end revSaveAsMobileStandalone

private function getIosSdkInfo pSDKVersion, @rInfo
   local tSDKPath, tSettings
   put merge("/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS[[pSDKVersion]].sdk") into tSDKPath
   get revIDEDeployIOSGetDeviceSDK(pSDKVersion)
   if there is no folder (it & tSDKPath) then
      return false
   end if
   put it into tSettings["root path"]
   put (it & tSDKPath) into tSettings["sdk path"]
   replace "." with "_" in pSDKVersion
   put pSDKVersion into tSettings["suffix"]
   put revMobileRuntimeFolder("Device", tSettings["suffix"]) into tSettings["runtime path"]
   put tSettings into rInfo
   return true
end getIosSdkInfo

// Check whether the output from linking is valid.
// This has been added after Xcode 7.0 adds warnings when the minimum
// version does not match the compiled version.
private function linkingIsValid pLinkingOutput
   if pLinkingOutput is empty then return true
   
   repeat for each line tLine in pLinkingOutput
      // Discard Xcode 7 new warnings like:
      // ld: warning: object file (<path_to_exe) was built for newer iOS version (9.0) than being linked (7.0)
      // ld: warning: ignoring linker optimzation..
      if not (tLine begins with "ld: warning:") then
         return false
      end if
   end repeat
   
   return true
end linkingIsValid
   
constant kIosExternals = "revzip revxml dbsqlite dbmysql dbodbc dbpostgresql revpdfprinter revsecurity"
private command revSaveAsMobileStandaloneMain pStack, pAppBundle, pTarget, pSettings
   if the platform is not "macos" then
      throw "not supported on this platform"
   end if
   
   revStandaloneResetWarnings
   
   -- If we're searching for inclusions, do it here 
   local tConfirm
   put true into tConfirm
   if pSettings["inclusions"] is "search" then 
      put false into tConfirm
      revSBSearchForInclusions pStack, pSettings
   end if
   
   -- Get the development root for device builds
   -- MM-2012-09-18: Rejiged the SDKs we use.  Things are now split up into sim, armv7, armv7, 
   -- since we now potentially need to use different SDKs for each.
   --
   -- For each, we have:
   --   suffix - the verison of the form 5_1, used to fetch teh correct user created externals (e.g. rrecanvas-device-5_1.lcext)
   --   runtime path - folder containing the LiveCode enngines and externals
   --   sdk path - location of the iOS SDK (within the XCode bundle)
   --   root path - root locaiton of the Developer tools (the XCode bundle)
   --
   
   local tSDKs, tSDKInfo
   if pTarget is "Device" then
      local tIosXcodePairs, tValid
      
      put revIDEDeployIOSGetIphoneOSes() into tIosXcodePairs
      put false into tValid
      
      repeat for each element tIosXcodePair in tIosXcodePairs
         if getIosSDKInfo(item 1 of tIosXcodePair, tSDKInfo) then
            put true into tValid
            exit repeat
         end if
      end repeat
      
      if not tValid then
         throw revIDEDeployIOSGetInvalidityMessage(tIosXcodePairs)
      end if
      put tSDKInfo into tSDKs["arm"]
      
      -- Compute the list of archs in the fat binary
      local tRawArchs
      put shell("otool -fv" && quote & mapFilePath(tSDKs["arm"]["runtime path"] & slash & "Standalone") & quote) into tRawArchs
      if tRawArchs is empty then
         put shell("otool -hv" && quote & mapFilePath(tSDKs["arm"]["runtime path"] & slash & "Standalone") & quote) into tRawArchs
      end if
      
      local tArchs
      -- SN-2015-03-30: [[ Bug 14794 ]] If the user specifies a 32-bit only build,
      --  then we do not do the 64-bit build.
      if tRawArchs contains "ARM64" and not pSettings["ios,32-bit only"] then
         put "arm64 " after tArchs
      end if
      if tRawArchs contains "V7" and pSettings["ios,minimum version"] < 11.0 then
         put "armv7 " after tArchs
      end if
      
      -- Make sure all dependencies are included
      revSBUpdateForDependencies "ios", replaceText(tArchs, space, comma), "iphoneos" & item 1 of tIosXcodePair, pSettings
   else
      
      -- MM-2012-09-18: We now only support the 4.3 simulators or later.
      -- For sim builds we just need the suffix (so we can extract the correct external) and where to look for the engines (runtime folder).
      -- SN-2015-02-05: [[ Bug 14422 ]] We only support 5.1 simulator or later.
      --
      local tSuffix
      // SN-2015-05-08: We only want the simulator version, not the whole string 'Simulator X.Y'
      put last word of pTarget into tSuffix
      
      -- Make sure all dependencies are included
      revSBUpdateForDependencies "ios", "x86_64", "iphonesimulator" & tSuffix, pSettings
      
      replace "." with "_" in tSuffix
      put tSuffix into tSDKS["sim"]["suffix"]
      put revMobileRuntimeFolder(pTarget, tSDKs["sim"]["suffix"]) into tSDKs["sim"]["runtime path"]
      
   end if
   
   revSBEnsurePerExtensionSettings "ios", pSettings
   if the result is not empty then
      return the result
   end if
   
   -- Manually remove built-in implementations from detected script library inclusions
   revSBRemoveInclusions pStack, "ios", tConfirm, pSettings
   
   -- Make sure old-style keys are retained, and updated with new information
   -- from inclusions pane of standalone settings GUI
   revSBConvertSettingsForPlatform pSettings, "ios"
   
   -- Compute the base folder
   local tBaseFolder
   set the itemDelimiter to slash
   put item 1 to -2 of the effective filename of stack pStack into tBaseFolder
   set the itemDelimiter to comma
   
   -- Fetch the various options we need to initially build the app-bundle
   local tName, tExternals, tDrivers
   put pSettings["name"] into tName
   
   if pSettings["ios,display name"] is empty then
      put tName into pSettings["ios,display name"]
   end if
   if pSettings["ios,bundle id"] is empty then
      put "com.yourcompany.yourapp" into pSettings["ios,bundle id"]
   end if
   
   -- MM-2014-01-29: [[ OpenSSL ]] Include the revsecurity library.
   -- Compute the externals list
   repeat for each word tExternal in kIosExternals
      if pSettings["ios,include" && tExternal] then
         if tExternal begins with "db" then 
            if "revdb" is not among the lines of tExternals then
               put "revdb" & return after tExternals
            end if
            put tExternal & return after tDrivers
         else if tExternal is among the words of "revpdfprinter revsecurity" then
            put tExternal & return after tDrivers
         else
            put tExternal & return after tExternals
         end if
      end if
   end repeat
   delete the last char of tExternals
   delete the last char of tDrivers
   
   -- Compute the stackfiles list
   lock messages
   local tStackFiles
   revSBRelativeStackFilesList pStack, tStackFiles
   unlock messages
   
   -- Make sure the app-bundle isn't already there
   if there is a folder pAppBundle then
      get shell("rm -r" && quote & pAppBundle & quote)
      if it is not empty or there is a folder pAppBundle then
         throw "unable to remove existing app-bundle"
      end if
   end if
   
   -- Make sure we can find the provisioning profile
   if pTarget is "Device" and pSettings["ios,profile"] is empty then
      throw "device build requires a provisioning profile to be chosen"
   end if
   
   if pTarget is "Device" and \
         there is no file ("~/Library/MobileDevice/Provisioning Profiles/" & pSettings["ios,profile"] & ".mobileprovision") then
      throw "could not find specified provisioning profile"
   end if
   
   -- Now create the folder, and copy in the initial files we need
   create folder pAppBundle
   
   -- MM-2012-09-18: Removed the copied files adn replaced with CopiedFonts and CopiedExternals (since that's all we used the CopeidFiels for).
   --
   -- The copied fonts are just a list of custom fonts used by the app.
   -- The copied externals is an array of the form:
   -- external name => (
   --   type - one of external, dylib, external-tmp
   --   location - the location of the external within the bundle, used by the deploy command to link to
   --   armv6 - location of the armv6 build
   --   armv7 - location of the armv7 external build
   --   sim - location of the simulator external build
   -- )
   --
   local tCopiedFonts, tCopiedExternals   
   
   -- Copy the main resources
   local tRedirects
   
   -- MM-2012-09-25: A 4 inch splash screen is required for iphone apps to take up the whole iphone 5 screen space.
   --
   if pSettings["ios,iphone 4inch splash"] is empty then
      put mapFilePath(revMobileRuntimeFolder(pTarget) & "/Default4InchSplash.png") into pSettings["ios,iphone 4inch splash"]
   end if
   
   -- MM-2011-09-28: If we have splash screens and icons  specifed in the old format, copy accross using the old format.
   -- Otherwise, copy accross using the new format (where individual icons and splash screens are specified rther than implied).
   --
   if pSettings["ios,icon"] is not empty then
      revCopyMobileIcons pSettings, tBaseFolder, pAppBundle
   else
      revCopyMobileNewIcons pSettings, tBaseFolder, pAppBundle
   end if
   if pSettings["ios,splash"] is not empty then
      revCopyMobileSplash pSettings, tBaseFolder, pAppBundle
   else
      revCopyMobileNewSplash pSettings, tBaseFolder, pAppBundle
   end if
   
   local tPlist, tCustomEntitlements
   revSBUpdateSettingsForExtensions "ios", pSettings
   revCopyMobileFiles pSettings, tBaseFolder, pAppBundle, pTarget, tSDKs, tRedirects, tCopiedExternals, tCopiedFonts, tPlist, tCustomEntitlements
   revCopyMobileStackFiles tStackFiles, tBaseFolder, pAppBundle
   
   -- Copy in the appropriate externals and drivers
   revCopyMobileExternals tExternals, pAppBundle, pTarget, tSDKs, tCopiedExternals
   revCopyMobileDrivers tDrivers, pAppBundle, pTarget, tSDKs, tCopiedExternals
   
   -- Generate the plist
   revCreateMobilePlist pSettings, pAppBundle, pTarget, tCopiedFonts, tPlist
   
   -- Generate the PkgInfo file
   put "APPL????" into url ("binfile:" & pAppBundle & slash & "PkgInfo")
   if the result is not empty then
      throw "unable to create PkgInfo file"
   end if
   
   -- Copy in the resource rules and provisioning profile
   if pTarget is "Device" then
      put url ("binfile:~/Library/MobileDevice/Provisioning Profiles/" & pSettings["ios,profile"] & ".mobileprovision") into \
            url ("binfile:" & pAppBundle & slash & "embedded.mobileprovision")
      if the result is not empty then
         throw "unable to copy provisioning profile"
      end if
   end if
   
   repeat for each line tLib in pSettings["extension_code_resources"]
      local tSourceFile, tSourcePath, tSourceName
      set the itemdelimiter to slash
      put item -1 of tLib into tSourceFile
      put item 1 to -2 of tLib into tSourcePath
      set the itemDelimiter to "."
      put item 1 to -2 of tSourceFile into tSourceName
      
      -- hack to uniquify modules
      -- tCopiedExternals should be changed to be keyed on file path
      -- however that has consequences for external mappings
      if tSourceFile ends with ".lcm" then
         put uuid() into tSourceName
      end if
      
      if there is a file tLib then
         put tSourceFile into tCopiedExternals[tSourceName]["location"]
      else
         create folder (pAppBundle & "/Frameworks")
         put "Frameworks/" & tSourceFile into tCopiedExternals[tSourceName]["location"]
      end if
      
      if pTarget is "Device" then
         put "extension-code-resource" into tCopiedExternals[tSourceName]["type"]
         put tLib into tCopiedExternals[tSourceName]["arm"]
      else
         --!TODO Add support for static libraries and frameworks by linking the engine for a simulator build
         if there is a file tLib then
            revSBCopyFileToFile tLib, pAppBundle & slash & tSourceFile, "revStandaloneProgressCallback", the long id of me
         else
            -- framework
            revSBCopyFolderToDestination tLib, pAppBundle & "/Frameworks", "revStandaloneProgressCallback", the long id of me
         end if
         put "extension-code-resource" into tCopiedExternals[tSourceName]["type"]
         put tLib into tCopiedExternals[tSourceName]["sim"]
      end if
   end repeat
   
   -- Externals to reference
   -- MM-2012-09-18: Use the new copied externals array instead of the copy files.
   --
   local tDeployExternals, tLibraryMappings
   if pSettings["ios,library"] is not empty then
      put pSettings["ios,library"] & return after tLibraryMappings 
   end if
   repeat for each key tExternalName in tCopiedExternals
      local tExternalRec
      put tCopiedExternals[tExternalName] into tExternalRec
      if tExternalRec["type"] is among the words of "external external-tmp" then
         put tExternalName & return after tDeployExternals
      end if
      if tLibraryMappings is not empty then
         put return after tLibraryMappings
      end if
      if not (tExternalRec["location"] ends with ".lcm" or \
            tExternalRec["location"] ends with ".a" or \
            (tExternalRec["location"] ends with ".lcext" \
            and pTarget is "Device")) then
         put tExternalName & ":" & "./" & tExternalRec["location"] after tLibraryMappings
      end if
   end repeat
   delete the last char of tDeployExternals
   put tDeployExternals into pSettings["externals"]
   put tLibraryMappings into pSettings["ios,library"]
   
   local tDeploy
   -- Update the deploy params with script library and extension inclusions
   revSBUpdateDeployParams pStack, "ios", pSettings, tDeploy
   
   -- Link the engine if a device build
   local tLinkEngineFile
   if pTarget is "Device" then
      revStandaloneProgress "Linking engine..."      
      
      repeat for each word tInstSet in "arm"
         if tSDKs[tInstSet]["suffix"] is empty then
            next repeat
         end if
         
         local tSDKRoot
         put tSDKs[tInstSet]["root path"] into tSDKRoot
         
         -- Change the path point to the SDK tools first
         local tOldPath
         put $PATH into tOldPath
         put tSdkRoot & slash & "Platforms/iPhoneOS.platform/Developer/usr/bin" & ":" before $PATH
         
         -- MM-2013-09-23: [[ iOS7 Support ]] g++ appears to have moved in XCode 5.
         --
         if tSDKs[tInstSet]["tools path"] is not empty then
            put tSDKs[tInstSet]["tools path"] & ":" before $PATH
         end if
         
         -- Compute the SDK path
         local tSdkPath
         put tSDKs[tInstSet]["sdk path"] into tSdkPath
         
         -- Construct the link command
         local tLinkCommand
         put empty into tLinkCommand
         
         -- Standard g++ options to create an executable output file
         put "-isysroot " & quote & tSdkPath & quote & return after tLinkCommand
         
         -- MW-2013-06-26: [[ CloneAndRun ]] Only strip global symbols if an installed env.
         if revEnvironmentIsInstalled() then
            -- FG-2015-01-21: [[ Widgets ]] We need to keep all exported symbols
            -- Make sure the only exported symbol is "main"
            --put "-Wl,-x -Wl,-exported_symbol -Wl,_main" & return after tLinkCommand
         end if
         
         -- MW-2013-06-25: [[ PIE ]] Make sure we make the minversion 4.3 so we can make
         --   ARMv7+ PIE builds.
         -- Make the minimum run version of the engine 4.3
         -- SN-2015-02-05: [[ Bug 14422 ]] The linker minimum iphoneOS version
         --   should be relevant with the desired minimum version.
         -- SN-2015-02-19: [[ Bug 14625 ]] Minimum version is architecture-specific
         --put revGetMinimumOSByArch(pSettings["ios,minimum version"]) into tVersion
         --put "-miphoneos-version-min=" & tVersion["armv7"] & return after tLinkCommand
         
         -- SN-2015-02-23: [[ Bug 14625 ]] Use a iOS 5.1/iOS 7.0 consistent stdlib
         put "-stdlib=libc++" & return after tLinkCommand
         
         -- Add reference to the partially linked standalone engine
         put quote & mapFilePath(tSDKs[tInstSet]["runtime path"] & slash & "Standalone") & quote & space after tLinkCommand
         
         -- Loop through the copied files list, extracting anything that needs to be
         -- linked with the engine
         local tLinkExts, tLinkDeps
         put empty into tLinkExts
         put empty into tLinkDeps
         
         -- Extract the dependency info from the extension
         -- MM-2012-09-18: Use the new copied externals array instead of the copy files.
         --
         set the itemDelimiter to "."
         repeat for each line tLinkExt in keys of tCopiedExternals
            switch the last item of tCopiedExternals[tLinkExt][tInstSet]
               case "txt"
                  -- deps file
                  get url ("file:" & tCopiedExternals[tLinkExt][tInstSet])
                  repeat for each line tLine in it
                     put true into tLinkDeps[tLine]
                  end repeat 
                  break
               case "a"
                  put quote & tCopiedExternals[tLinkExt][tInstSet] & quote & return after tLinkCommand
                  put "-force_load" && quote & tCopiedExternals[tLinkExt][tInstSet] & quote & return after tLinkCommand
                  break
               case "bundle"
               case "framework"
                  -- check if the library is static or dynamic
                  local tFileName
                  set the itemDelimiter to slash
                  put the last item of tCopiedExternals[tLinkExt][tInstSet] into tFileName
                  set the itemDelimiter to "."
                  delete the last item of tFileName
                  get shell("file" && quote & tCopiedExternals[tLinkExt][tInstSet] & "/" & tFileName & quote)
                  if it contains "ar archive" then
                     -- static frameworks are just a package for a static library
                     put quote & tCopiedExternals[tLinkExt][tInstSet] & "/" & tFileName & quote & return after tLinkCommand
                     put "-force_load" && quote & tCopiedExternals[tLinkExt][tInstSet] & "/" & tFileName & quote & return after tLinkCommand
                  else
                     -- dynamic framework iOS 8+
                     revSBCopyFolderToDestination tCopiedExternals[tLinkExt][tInstSet], pAppBundle & "/Frameworks", "revStandaloneProgressCallback", the long id of me
                     if tCopiedExternals[tLinkExt][tInstSet] ends with ".framework" then
                        -- remove simulator slices
                        -- if we are copying a bundle these can only be used for resources
                        local tCopiedFrameworkBinary
                        put quote & pAppBundle & "/Frameworks/" & tFileName & ".framework/" & tFileName  & quote into tCopiedFrameworkBinary
                        repeat for each word tArch in "i386 x86_64"
                           get shell("lipo -remove" && quote & tArch & quote && "-output" && tCopiedFrameworkBinary && tCopiedFrameworkBinary)
                        end repeat
                     end if
                  end if
                  break
               case "embeddedframework"
                  -- .embeddedframework is a hack that some SDKs use for single drag and drop of
                  -- multiple resource bundles and frameworks
                  throw "The contents of the embeddedframework folder should be added to the module code resource folder not the embeddedframework folder itself:" & return & tCopiedExternals[tLinkExt][tInstSet]
                  break
               case "lcm"
                  local tModuleFolder
                  set the itemDelimiter to slash
                  put tCopiedExternals[tLinkExt][tInstSet] into tModuleFolder
                  
                  local tModule
                  put item -4 of tModuleFolder into tModule
                  
                  local tModuleFile
                  filter tDeploy["modules"] without ("*/" & tModule  & "/*")
                  if tDeploy["modules"] is not empty then
                     put return after tDeploy["modules"]
                  end if
                  put tCopiedExternals[tLinkExt][tInstSet] after tDeploy["modules"]
                  
                  set the itemDelimiter to "."
                  break
               case "lcext"
               default
                  put tCopiedExternals[tLinkExt][tInstSet] into tLinkExts[tLinkExt] 
                  _internal extract "__MISC" "__deps" from tLinkExts[tLinkExt]
                  repeat for each line tLine in it
                     put true into tLinkDeps[tLine]
                  end repeat 
                  
                  -- Add references to each partially linked extension file
                  put quote & tLinkExts[tLinkExt] & quote & return after tLinkCommand
                  break
            end switch
         end repeat
         
         -- Fetch the deps for the standalone itself
         _internal extract "__MISC" "__deps" from mapFilePath(tSDKs[tInstSet]["runtime path"] & slash & "Standalone")
         repeat for each line tLine in it
            put true into tLinkDeps[tLine]
         end repeat
         
         -- Add references to each dependency
         repeat for each key tLinkDep in tLinkDeps
            if word 1 of tLinkDep is "library" then
               put "-l" & word 2 of tLinkDep & return after tLinkCommand
            else if word 1 of tLinkDep is "framework" then
               put "-framework" && word 2 of tLinkDep & return after tLinkCommand
            else if word 1 of tLinkDep is "weak-framework" then
               put "-weak_framework" && word 2 of tLinkDep & return after tLinkCommand
            end if
         end repeat
         
         -- SN-2015-02-19: [[ Bug 14625 ]] Build each binary separately according
         -- to their architecture.
         local tArchSpecificEngineList, tArchSpecificEngineFile
         put empty into tArchSpecificEngineList
         repeat for each word tArch in tArchs
            -- Make a temporary file for link options and execute it
            local tLinkOptionsFile
            put tempName() into tLinkOptionsFile
            
            put tempName() into tArchSpecificEngineFile
            put tLinkCommand & "-o" && quote & tArchSpecificEngineFile & quote & return into url ("binfile:" & tLinkOptionsFile)
            
            local tiPhoneMinVersion
            put revGetMinimumOSByArch(pSettings["ios,minimum version"]) into tiPhoneMinVersion
            
            -- MM-2013-09-23: [[ iOS7 Support ]] Use g++ instead of llvm-g++-4.2. XCode 5.0 uses llvm 5.0.
            -- g++ appears to be sym-linked to the appropriate compiler in all SDKS.
            -- SN-2015-02-19: [[ Bug 14625 ]] The minimum iOS version is bound to the architecture
            get shell("g++ -arch " & tArch && "-miphoneos-version-min=" & tiPhoneMinVersion[tArch] && " -w " & quote & "@" & tLinkOptionsFile & quote)
            
            put tArchSpecificEngineFile & space after tArchSpecificEngineList
            delete file tLinkOptionsFile
            if not linkingIsValid(it) or there is no file tArchSpecificEngineFile then
               throw "linking for" && tInstSet && " (" & tArch & ") failed with " & it
            end if
         end repeat
         
         -- MM-2011-09-28: Create the engine for the instruction set specified in the build type setting.
         --
         put tempName() into tSDKs[tInstSet]["engine file"]
         -- SN-2015-02-19: [[ Bug 14625 ]] Make the fat binary with the architecture-specific engines
         get shell("lipo -create " & tArchSpecificEngineList & " -output " & quote & tSDKs[tInstSet]["engine file"] & quote)
         
         if it is not empty or there is no file tSDKs[tInstSet]["engine file"] then
            throw "linking for" && tInstSet && "failed with " & it
         end if
         
         -- Put the path back the way it was
         put tOldPath into $PATH
         
      end repeat
      
      -- MM-2012-09-18: We now should have the required engines linked.  If we are building a universal binary,
      -- create from the two separate linkede engines.
      --=
      put tSDKs["arm"]["engine file"] into tLinkEngineFile
      
      if there is no file tLinkEngineFile then
         throw "creating engine failed"
      end if
      
      put tLinkEngineFile into tDeploy["engine"]      
   else
      put mapFilePath(tSDKs["sim"]["runtime path"] & slash & "Standalone") into tDeploy["engine"]
   end if
   
   -- Generate the executable
   revStandaloneProgress "Building executable..."
   
   -- Stackfile to use
   put the effective filename of stack pStack into tDeploy["stackfile"]
   
   -- Put the redirects if any
   -- MM-2014-10-06: [[ Bug 13583 ]] Due to issues with the redirects, we use symlinks with the iOS 8 sim.
   -- PM-2016-09-16: [[ Bug 18414 ]] iOS 10 - tSDKs["sim"]["suffix"] is x_y on iOS x.y
   local tSimVersion, tOldItemDel
   put tSDKs["sim"]["suffix"] into tSimVersion
   put the itemdel into tOldItemDel
   set the itemdel to"_"
   if item 1 of tSimVersion < 8 then
      put tRedirects into tDeploy["redirects"]
   else
      put empty into tDeploy["redirects"]
      repeat for each line tRedirect in tRedirects
         get offset("//", tRedirect)
         if it is not 0 then
            --symlinkRedirect char (it + 2) to -1 of tRedirect, pAppBundle & slash & char 1 to (it - 1) of tRedirect
            local tSource, tTarget
            put char (it + 2) to -1 of tRedirect into tSource
            put pAppBundle into tTarget
            
            put slash & char 1 to (it - 1) of tRedirect after tTarget
            
            symlinkRedirect tSource, tTarget
         end if
      end repeat
   end if
   set the itemdel to tOldItemDel
   
   -- Output file to create
   put pAppBundle & slash & tName into tDeploy["output"]
   
   -- Splash to use (if non-commercial)
   if line 3 of the revLicenseInfo is among the words of "Educational Personal" then
      -- MM-2011-09-28: Descend through possible spash settings until a splash is found.
      repeat for each item tSplash in "splash,iphone splash,retina splash,ipad portrait splash,ipad landscape splash,ipad retina portrait splash,ipad retina landscape splash"
         if pSettings["ios," & tSplash] is not empty then
            put pSettings["ios," & tSplash] into tDeploy["splash"]
            exit repeat
         end if
      end repeat
      -- MW-2011-03-17: Make sure absolute paths work correctly
      if not (tDeploy["splash"] begins with "/") then
         put tBaseFolder & slash before tDeploy["splash"]
      end if
   end if
   
   -- SN-2015-03-16: [[ iOS Font Mapping ]] Fontmapping deploy command added
   --  We have our fontmapping list (the default fonts), and the custom ones the user
   --  may have added
   local tFontmapFiles,tStandardFontmapRoot
   
   -- We want the repo version to be able to build iOS standalones with the fontmap
   if revEnvironmentIsInstalled() then
      put mapFilePath(revMobileRuntimeFolder(pTarget)) into tStandardFontmapRoot
   else
      local tOldDel
      put the itemdelimiter into tOldDel
      set the itemdelimiter to slash
      put item 1 to -4 of revEnvironmentBinariesPath() into tStandardFontmapRoot
      put "/engine/rsrc" after tStandardFontmapRoot
      set the itemdelimiter to tOldDel
   end if
   
   -- We add our fontmapping, and the user one if not empty
   put tStandardFontmapRoot & slash & "fontmap" into tFontMapFiles
   local tUserFontMappings
   put pSettings["ios,fontmappings"] into tUserFontMappings
   set the itemdelimiter to comma
   if tUserFontMappings is not empty then
      // Make sure to use the right absolute filepath
      if not (tUserFontMappings begins with slash) then
         put tBaseFolder & slash before tUserFontMappings
      end if
      put tUserFontMappings into item 2 of tFontMapFiles
   end if
   
   repeat for each item tFontmapFile in tFontMapFiles
      -- Alert the user if their font map has not been found on the disk
      if there is no file tFontmapFile then
         revStandaloneAddWarning "Could not find font mapping file" && tFontmapFile
      else
         local tFontMappings
         put URL("file:" & tFontmapFile) into tFontMappings
         -- Ensure that the list ends with a linefeed
         if the last char of tFontMappings is not LF then
            put LF after tFontMappings
         end if
         put tFontMappings after tDeploy["fontmappings"]
      end if
   end repeat
   
   -- delete the last LF
   delete last char of tDeploy["fontmappings"]
   
   -- Make sure the standalone is generated with a unique uuid
   put uuid() into tDeploy["uuid"]
   
   try
      _internal deploy ios tDeploy
      if the result is not empty then
         throw the result
      end if
   catch tError
   finally
      delete file tLinkEngineFile
   end try
   
   if tError is not empty then
      throw tError
   end if
   
   -- MW-2013-06-26: [[ CloneAndRun ]] If not installed and creating a device build
   --    then create the dSYM debug info within the app bundle.
   if pTarget is "Device" and not revEnvironmentIsInstalled() then
      revStandaloneProgress "Extracting debug symbols..."
      
      // SN-2015-09-10: [[ Xcode 7.0 ]] Xcode 7.0 sets the env var MallocNanoZone to 1
      //  but this makes dsymutil crash...
      //  We reinstate it after symbol extraction as a user might as well set MallocNanoZone
      local tOldMallocNanoZone
      if $MallocNanoZone is not empty then
         put $MallocNanoZone into tOldMallocNanoZone
         put 0 into $MallocNanoZone
      end if
      
      get shell("dsymutil" && quote & tDeploy["output"] & quote)
      if it is not empty then
         revStandaloneProgress "Extracting debug symbols failed"
      end if
      
      // Reinstate env var MallocNanoZone if it was set.
      if $MallocNanoZone is not empty then
         put tOldMallocNanoZone into $MallocNanoZone
      end if
   end if
   
   -- Only need to sign the bundle if its heading for a device
   if pTarget is "Device" then
      revStandaloneProgress "Signing app bundle..."
      
      local tCertificate, tEntitlements, tProfileInfo
      put revGetMobileProfileInfo(pSettings["ios,profile"]) into tProfileInfo
      
      -- Get the list of valid identities that are available
      local tValidIdentities
      get shell("/usr/bin/security -q find-identity -v -p codesigning")
      if the last line of it contains "valid identities found" then
         delete the last line of it
         -- "it" now is of the form:
         -- 1) Certificate1IdHEX "Certificate1NameString"
         -- 2) Certificate2IdHEX "Certificate2NameString"
         repeat for each line tIdentity in it
            put the second word of tIdentity into tValidIdentities[char 2 to -2 of the last word of tIdentity]
         end repeat
      else
         put empty into tValidIdentities
      end if
      
      -- Use the first one we can that is in the profile
      put empty into tCertificate
      repeat for each line tIdentity in tProfileInfo["identities"]
         if tValidIdentities[tIdentity] is not empty then
            put tValidIdentities[tIdentity] into tCertificate
            exit repeat
         end if
      end repeat
      
      -- If there isn't one available its an error
      if tCertificate is empty then
         throw "could not find a valid identity to use for the selected profile"
      end if
      
      put url ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "Entitlements.xcent")) into tEntitlements
      
      -- Construct the entitlements file
      replace "${GET_TASK_ALLOW}" with not tProfileInfo["store"] in tEntitlements
      replace "${APP_IDENTIFIER}" with tProfileInfo["appid"] in tEntitlements
      replace "${BUNDLE_IDENTIFIER}" with pSettings["ios,bundle id"] in tEntitlements
      replace "${CUSTOM_ENTITLEMENTS}" with tCustomEntitlements in tEntitlements
      
      -- MM-2012-02-12: Added support for push notificaitons entitlements
      if pSettings["ios,push notifications"] then
         if tProfileInfo["store"] then
            get url ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "RemoteNotificationStoreEntitlements.xcent"))
         else
            get url ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "RemoteNotificationEntitlements.xcent"))
         end if
         replace "${REMOTE_NOTIFICATIONS}" with it in tEntitlements
      else
         replace "${REMOTE_NOTIFICATIONS}" with empty in tEntitlements
      end if     
      
      // SN-2015-09-17: [[ BetaApp ]] Add entitlement for beta apps
      local tBetaEntitlement
      if pSettings["ios,beta version"] then
         put url("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "BetaReportEntitlement.xcent")) into tBetaEntitlement
      end if
      replace "${BETA_REPORT_ENTITLEMENT}" with tBetaEntitlement in tEntitlements
      
      put numToChar(0xfa) & numToChar(0xde) & numToChar(0x71) & numToChar(0x71) & binaryEncode("M", the length of tEntitlements + 8) before tEntitlements
      
      local tEntitlementsFile
      put tempName() into tEntitlementsFile
      put tEntitlements into url ("binfile:" & tEntitlementsFile)
      
      -- MW-2011-03-17: Make sure the certificate is encoded appropriately
      put uniDecode(uniEncode(tCertificate, "UTF8")) into tCertificate
      
      -- Do the codesigning
      // SN-2015-09-11: [[ Xcode 7.0 ]] codesign in Xcode 7.0 will not work if CODESIGN_ALLOCATE
      // environment variable is set, so we only set it up to Xcode 6.4
      // We use Clang version to get the Xcode version in use
      local tClangVersion, tClangMajorVersion
      put shell("clang++ --version") into tClangVersion
      if matchText(tClangVersion, "Apple LLVM version ([0-9]+)\.[0-9]+.[0-9]+", tClangMajorVersion) \
            and tClangMajorVersion < 7 then
         put tSdkRoot & slash & "Platforms/iPhoneOS.platform/Developer/usr/bin/codesign_allocate" into $CODESIGN_ALLOCATE
      end if
      
      -- make sure you have permission to clear extended attributes from bundle files
      get shell("chmod -R u+w" && quote & pAppBundle & quote)
      if the result is not 0 then
         throw "setting permissions failed with error:" && it
      end if
      
      -- clear extended attributes from bundle files
      get shell("xattr -cr" && quote & pAppBundle & quote)
      if the result is not 0 then
         throw "clearing extended attributes failed with error:" && it
      end if
      
      --try
      -- Perform the codesigning of the main bundle
      local tResult
      __CodesignFrameworks pAppBundle & "/Frameworks", tCertificate, tEntitlementsFile
      put shell("/usr/bin/codesign --deep --verbose -f -s" && quote & tCertificate & quote && \
            "--entitlements" && quote & tEntitlementsFile & quote && \
            quote & pAppBundle & quote) into tResult
      -- MM-2012-10-25: [[ Bug ]] try catch finally oddness meant that any errors here were being ignored.
      delete file tEntitlementsFile
      if not (tResult contains "signed bundle" or tResult contains "signed app bundle") then
         throw "codesigning failed with" && tResult
      end if
      --      finally
      --         -- Delete the temporary file
      --         delete file tEntitlementsFile
      --      end try
      
   end if
end revSaveAsMobileStandaloneMain

private command __CodesignFrameworks pPath, pCertificate, pEntitlements
   local tFrameworks
   put folders(pPath) into tFrameworks
   filter tFrameworks with "*.framework"
   repeat for each line tFramework in tFrameworks
      __CodesignFrameworks pPath & "/" & tFramework & "/Frameworks", pCertificate, pEntitlements
      
      local tResult
      put shell("/usr/bin/codesign --deep --verbose -f -s" && quote & pCertificate & quote && \
            "--entitlements" && quote & pEntitlements & quote && \
            quote & pPath & "/" & tFramework & quote) into tResult
      if not (tResult contains "signed bundle" or tResult contains "signed app bundle") then
         throw "codesigning failed with" && tResult
      end if
   end repeat
end __CodesignFrameworks

command revSaveAsMobileDeployer pDeployAppBundle, pAppBundle, pAppName, pAppId, pAppExecutables
   create folder pDeployAppBundle
   create folder pDeployAppBundle & slash & "Contents"
   create folder pDeployAppBundle & slash & "Contents" & "MacOS"
   
   local tProperties
   put pAppName into tProperties["name"]
   put pAppId into tProperties["id"]
   put pAppExecutables into tProperties["executables"]
   --put url ("binfile:" & revMobileRuntimeFolder(pTarget) & slash & "StoreEntitlements.xcent") into tProperties["store_entitlements"]
   --put url ("binfile:" & revMobileRuntimeFolder(pTarget) & slash & "Entitlements.xcent") into tProperties["entitlements"]
   --put the compress of the arrayEncode of tProperties into url ("binfile:" & pDeployAppBundle & slash & "Contents/MacOS/properties")
   
   local tDeploy
   -- TODO build executable
end revSaveAsMobileDeployer

################################################################################

local sCFBundleIcons
private command revCopyMobileNewIcons pSettings, pBaseFolder, pAppBundle   
   revStandaloneProgress "Copying icons..."    
   /*
   The entry for the AppStore icon in the JSON file should have the form:
   {
   "idiom" : "ios-marketing",
   "size" : "1024x1024",
   "scale" : "1x",
   "filename" : "iTunesArtwork@2x.png"
   }
   */
   local sIconsList 
   put  \
         "appstore icon,ios-marketing,1024x1024,1x,ItunesArtwork@2x.png" & return & \
         "iphone icon,iphone,57x57,1x,Icon-App-57x57@1x.png" & return & \
         "retina icon,iphone,57x57,2x,Icon-App-57x57@2x.png" & return & \
         "iOS 7 retina icon,iphone,60x60,2x,Icon-App-60x60@2x.png" & return & \
         "iphone 6 plus icon,iphone,60x60,3x,Icon-App-60x60@3x.png" & return & \
         "iphone X icon,iphone,60x60,3x,Icon-App-60x60@3x.png" & return & \
         "ipad icon,ipad,72x72,1x,Icon-App-72x72@1x.png" & return & \
         "ipad retina icon,ipad,72x72,2x,Icon-App-72x72@2x.png" & return & \
         "iOS 7 ipad icon,ipad,76x76,1x,Icon-App-76x76@1x.png" & return & \
         "iOS 7 ipad retina icon,ipad,76x76,2x,Icon-App-76x76@2x.png" & return & \
         "iPad Pro 12.9 icon,ipad,83.5x83.5,2x,Icon-App-83.5x83.5@2x.png" \
         into sIconsList
   
   local tAssetsFolder, tAppIconFolder
   put tempname() into tAssetsFolder
   create folder  tAssetsFolder
   put tAssetsFolder & slash & "AppIcon.appiconset" into tAppIconFolder
   create folder tAppIconFolder
   
   local tContentsJSON
   put "{" & CR & quote & "images" & quote & ":[" & CR after tContentsJSON
   
   repeat for each line tType in sIconsList
      get pSettings["ios," & item 1 of tType]
      if not (it begins with "/") then
         put pBaseFolder & slash before it
      end if
      if there is a file it then         
         put url ("binfile:" & it) into url ("binfile:" & tAppIconFolder & slash &  item 5 of tType)
         if the result is not empty then
            throw "could not copy icon '" & it & "'"
         end if
         -- add an entry in the Contents.json file
         local tEntry
         put "{" & CR into tEntry
         put quote & "idiom" & quote & ":" & quote & item 2 of tType & quote & comma & CR after tEntry
         put quote & "size" & quote & ":" & quote & item 3 of tType & quote & comma & CR after tEntry
         put quote & "scale" & quote & ":" & quote & item 4 of tType & quote & comma & CR after tEntry
         put quote & "filename" & quote & ":" & quote & item 5 of tType & quote  & CR after tEntry
         put "}" & comma & CR after tEntry
         put tEntry after tContentsJSON
      end if
   end repeat
   
   put "]" & CR & "}" after tContentsJSON
   put tContentsJSON into url ("file:" & tAppIconFolder & slash &  "Contents.json")
   if the result is not empty then
      throw "could not create json file '" & it & "'"
   end if
   
   local tCompiledAssetsFolder
   put tempname() into tCompiledAssetsFolder
   create folder  tCompiledAssetsFolder
   
   local tMinVersion
   put pSettings["ios,minimum version"] into tMinVersion
   if tMinVersion is empty then put "6.0" into tMinVersion
   
   -- create the Assets.car file
   get shell("xcrun actool " &  tAssetsFolder & \ 
         " --compile " & tCompiledAssetsFolder & \
         " --platform iphoneos  --minimum-deployment-target " & tMinVersion & \
         " --app-icon AppIcon --output-partial-info-plist " & \
         tCompiledAssetsFolder &"/partial.plist")
   
   local tCompiledAssetsFolderContents,tPartialPlist
   put url("file:"& tCompiledAssetsFolder & slash & "partial.plist") into tPartialPlist
   
   -- store the icon-related part of the partial plist in a script local, to use it later in revCreateMobilePlist
   put line 5 to -3 of tPartialPlist into sCFBundleIcons
   
   put files(tCompiledAssetsFolder) into tCompiledAssetsFolderContents
   filter tCompiledAssetsFolderContents without "partial.plist"
   
   -- copy the Assets.car file and the icon files in the app bundle
   repeat for each line tFile in tCompiledAssetsFolderContents
      put url ("binfile:" & tCompiledAssetsFolder & slash & tFile) into url ("binfile:" & pAppBundle & slash &  tFile)
   end repeat
   
   -- cleanup
   delete folder tCompiledAssetsFolder
   delete folder tAppIconFolder
   delete folder tAssetsFolder
   
end revCopyMobileNewIcons

private command revCopyMobileNewSplash pSettings, pBaseFolder, pAppBundle   
   -- Only copy in the splash screen verbatim if we are a commercial/review license type
   if line 3 of the revLicenseInfo is among the words of "Personal Educational Evaluation" then
      exit revCopyMobileNewSplash
   end if
   
   revStandaloneProgress "Copying launch images..."
   
   -- PM-2015-03-19: [[ Bug 13969 ]] Make sure we use the correct names for the splash images in iphone6/6+
   repeat for each line tType in ("iphone splash" & return & "retina splash,@2x" & return & "ipad portrait splash,-Portrait" & \
         return & "ipad landscape splash,-Landscape" & return & "ipad retina portrait splash,-Portrait@2x" & return & "ipad retina landscape splash,-Landscape@2x" & \
         return & "ipad pro 12.9 portrait splash,-iPadProPortrait@2x" & return & "ipad pro 12.9 landscape splash,-iPadProLandscape@2x" & \
         return & "iphone 4inch splash,-568h@2x" & return & "iphone 6 splash,-667h@2x" & \
         return & "iphone 6 plus portrait splash,-736h@3x" & return & "iphone 6 plus landscape splash,-414h@3x" & \
         return & "iphone X portrait splash,-812h@3x" & return & "iphone X landscape splash,-375h@3x" & \
         return & "iphone XR portrait splash,-896h@2x" & return & "iphone XR landscape splash,-414h@2x" & \
         return & "iphone XSMAX portrait splash,-896h@3x" & return & "iphone X landscape splash,-414h@3x")
      get pSettings["ios," & item 1 of tType] 
      if not (it begins with "/") then
         put pBaseFolder & slash before it
      end if
      if there is a file it then         
         put url ("binfile:" & it) into url ("binfile:" & pAppBundle & slash & "Default" & item 2 of tType & ".png")
         if the result is not empty then
            throw "could not copy splash '" & it & "'"
         end if
      end if
   end repeat
end revCopyMobileNewSplash

private command revCopyMobileIcons pSettings, pBaseFolder, pAppBundle
   local tIconFile
   put pSettings["ios,icon"] into tIconFile
   if tIconFile is empty then
      exit revCopyMobileIcons
   end if
   
   revStandaloneProgress "Copying icons..."
   
   if not (tIconFile begins with "/") then
      put pBaseFolder & slash before tIconFile
   end if
   
   repeat for each item tType in ",-72,-114"
      get revModifyMobileImageFile(tIconFile, tType)
      if there is a file it then         
         --MM-2011-09-28: Make sure the icons are of the appropriate size.
         --
         switch tType
            case "-72"
               if getImageDimensions(it) is not "72,72" then
                  revStandaloneAddWarning "iPad Icon wrong size"
               end if               
               break
            case "-114"
               if getImageDimensions(it) is not "114,114" then
                  revStandaloneAddWarning "iPhone Hi-Res Icon wrong size"
               end if               
               break
            default
               if getImageDimensions(it) is not "57,57" then
                  revStandaloneAddWarning "iPhone Icon wrong size"
               end if               
               break
         end switch
         
         put url ("binfile:" & it) into url ("binfile:" & pAppBundle & slash & "Icon" & tType & ".png")
         if the result is not empty then
            throw "could not copy icon '" & it & "'"
         end if
      end if
   end repeat
end revCopyMobileIcons

private command revCopyMobileSplash pSettings, pBaseFolder, pAppBundle
   local tSplashFile
   put pSettings["ios,splash"] into tSplashFile
   if tSplashFile is empty then
      exit revCopyMobileSplash
   end if
   
   -- Only copy in the splash screen verbatim if we are a commercial/review license type
   if line 3 of the revLicenseInfo is among the words of "Personal Educational Evaluation" then
      exit revCopyMobileSplash
   end if
   
   revStandaloneProgress "Copying launch images..."
   
   if not (tSplashFile begins with "/") then
      put pBaseFolder & slash before tSplashFile
   end if
   
   repeat for each item tType in ",@2x,-Landscape,-Portrait"
      get revModifyMobileImageFile(tSplashFile, tType)
      if there is a file it then         
         --MM-2011-09-28: Make sure the splash screens are of the appropriate size.
         --
         switch tType
            case "-@2"
               if getImageDimensions(it) is not "640,960" then
                  revStandaloneAddWarning "iPhone Hi-Res Splash wrong size"
               end if               
               break
            case "-Landscape"
               if getImageDimensions(it) is not "114,114" then
                  revStandaloneAddWarning "iPhone Hi-Res Icon wrong size"
               end if               
               break
            case "-Portrait"
               if getImageDimensions(it) is not "768,1024" then
                  revStandaloneAddWarning "iPad Portrait Splash wrong size"
               end if               
               break
            default
               if getImageDimensions(it) is not "1024,768" then
                  revStandaloneAddWarning "iPad Landscape Splash wrong size"
               end if               
               break
         end switch
         
         put url ("binfile:" & it) into url ("binfile:" & pAppBundle & slash & "Default" & tType & ".png")
         if the result is not empty then
            throw "could not launch image '" & it & "'"
         end if
      end if
   end repeat
end revCopyMobileSplash

private function revModifyMobileImageFile pFile, pType
   if pFile ends with ".png" then
      delete char -4 to -1 of pFile
   end if
   
   if pFile ends with "@2x" then
      delete char -3 to -1 of pFile
   end if
   
   repeat for each item tSuffix in "-Landscape,-Portrait,-72,-114"
      if pFile ends with tSuffix then
         delete char -(the number of chars of tSuffix) to -1 of pFile
         exit repeat
      end if
   end repeat
   
   return pFile & pType & ".png"
end revModifyMobileImageFile

################################################################################

private command addToManifest pFile, pName, pAppBundle, @xManifest, @xRedirects
   if pName contains "//" or pName contains tab then
      local tWarning
      put "File/folder to include in standalone contains invalid chars (tab or //):" into tWarning
      revStandaloneAddWarning tWarning && quote & pFile & quote
      exit addToManifest
   end if
   
   if there is a file pFile then
      put pAppBundle & slash & pName & tab & pFile & return after xManifest
      put pName & "//" & pFile & return after xRedirects
   else if there is a folder pFile then
      revCopyMobileFilesComputeFolderManifest xManifest, pFile, pAppBundle & slash & pName
      put pName & "//" & pFile & return after xRedirects
   else
      revStandaloneAddWarning "File/folder to include in standalone not found:" && quote & pFile & quote
   end if
end addToManifest

-- The files list from the standalone builder contains relative and absolute references.
--
-- Relative references get copied and keep their folder structure.
-- Absolute references get copied in without folder structure.
-- If a file ends in '/*' it means it is a folder and should be recursed into.
-- e.g.
--   foobar/baz -> <base>/foobar/baz
--   C:/foobar/baz -> <base>/baz
--
private command revCopyMobileFiles pSettings, pBaseFolder, pAppBundle, pAppTarget, pSDKs, @rRedirects, @rExternals, @rFonts, @rPlist, @rCustomEntitlements
   revStandaloneProgress "Copying files..."
   
   -- Only use redirects in simulator 4.x
   -- MM-2012-03-26: revMobileComputeTargetSuffix has changed for 5.1 support.  Added device check to make
   -- sure that we don't try and use redirects for device builds.
   -- MM-2012-09-18: Since we now no longer support sims before 4.3, we always use redirects for sim builds.
   --
   
   local tUseRedirects
   if pAppTarget is not "Device" then
      put true into tUseRedirects
   end if 
   
   -- Start with an empty redirects list
   put empty into rRedirects
   
   local tFileData
   revSBResolveCopyFilesList pBaseFolder, "ios", pSettings, tFileData
   
   -- Compute a list of the files we need to copy. This is in the form
   --   <target> [tab] <source>
   -- If <source> is empty then <target> is a folder that needs to be created.
   --
   local tManifest
   set the itemDelimiter to slash
   repeat for each element tFile in tFileData
      revSBEnsureFolder pAppBundle & slash & item 1 to -2 of tFile["name"]
      
      -- MW-2010-12-16: [[ Bug 9238 ]] Check for 'resources' folder
      if item 1 of tFile["name"] is "Resources" then
         throw "iOS apps that contain a top-level folder called 'Resources' will fail to run"
      end if
      
      addToManifest tFile["resolved"], tFile["name"], pAppBundle, tManifest, rRedirects
   end repeat
   
   -- Remove trailing delimiter
   if tUseRedirects then
      delete the last char of rRedirects
   else
      put empty into rRedirects
   end if
   
   -- Keep track of processed dylibs
   local tProcessedDylibs
   
   -- Now do the actual work of copying the files
   set the itemDelimiter to tab
   repeat for each line tItem in tManifest
      local tTarget, tSource
      put item 1 of tItem into tTarget
      put item 2 of tItem into tSource
      
      -- If the source is empty then just create the target folder
      if tSource is empty then
         create folder tTarget
         next repeat
      end if
      
      -- If the file is a font, then just copy and record as such
      if revSBFileIsFontFile(tSource) then
         revSBCopyFileToFile tSource, tTarget, "revStandaloneProgressCallback", the long id of me
         put "font", tTarget & return after rFonts
         next repeat
      end if
      
      -- If the file is an entitlement then handle appropriately
      if tSource ends with ".xcent" then
         put url ("binfile:" & tSource) after rCustomEntitlements
         next repeat
      end if
      
      -- If the file is an lcext and it is a zip file, handle appropriately.
      -- MM-2012-09-18: Updated to use the new copied externals array instead of the copy files,
      -- where we extract the appropraite external for the instruciton set(s) we are building for.
      --
      local tIsZip, tError
      put revSBLcextFileIsZipArchive(tSource, tError) into tIsZip
      if tError is not empty then
         throw tError
      end if
      
      if tSource ends with ".lcext" and tIsZip then
         if tProcessedDylibs[tSource] then
            next repeat
         end if         
         
         put true into tProcessedDylibs[tSource]
         set the itemDel to slash
         get char 1 to -7 of the last item of tSource
         set the itemDel to tab
         
         if pSDKs["sim"]["suffix"] is not empty then
            -- MERG-2013-09-05: [[ Bug 11152 ]] Only include the external if it is found for this platform
            if lcextExtractExternal(tSource, "Simulator-" & pSDKs["sim"]["suffix"], tTarget) then
               put tTarget into rExternals[it]["sim"]
               put "external" into rExternals[it]["type"]
               
               -- Since simulator externals are not statically linked to the engine, we need to make sure the location
               -- of the external is set correctlty relative to the app bundle, so the deploy command knows what to
               -- dynamically link to.
               --
               put char (the number of chars in pAppBundle + 2) to -1 of tTarget into rExternals[it]["location"]
            end if
         else
            put "external-tmp" into rExternals[it]["type"]        
            repeat for each word tInstSet in "armv6 armv7 arm64 arm"
               if pSDKs[tInstSet]["suffix"] is not empty then
                  put the tempName into tTarget
                  if lcextExtractExternal(tSource, "Device-" & pSDKs[tInstSet]["suffix"], tTarget) then
                     put tTarget into rExternals[it][tInstSet]
                     put it & ".lcext" into rExternals[it]["location"]
                  end if
               end if             
            end repeat
         end if
         next repeat
      end if
      
      -- If the file is a dylib, then see if there is a target specific version and record
      -- MM-2012-09-18: Ignore old style externals.
      --
      if tSource ends with ".lcext" or tSource ends with ".dylib" then         
         next repeat
      end if
      	  
      -- MERG-2015-04-24: [[ TemplateAppMetadata ]] If a template plist is found then we use it instead of the default template.
      if tSource ends with "Info.plist" then
         put url ("binfile:" & tSource) into rPlist
      end if
      
      -- Otherwise its just a file
      if not tUseRedirects then
         revSBCopyFileToFile tSource, tTarget, "revStandaloneProgressCallback", the long id of me
      end if
   end repeat
end revCopyMobileFiles

private command revCopyMobileFilesComputeFolderManifest @xManifest, pSource, pTarget
   -- Record the folder that is needed
   put pTarget & return after xManifest
   
   -- Now start recursive descent
   local tOldFolder
   put the folder into tOldFolder
   set the folder to pSource
   
   repeat for each line tFile in the files
      put pTarget & slash & tFile & tab & pSource & slash & tFile & return after xManifest
   end repeat
   
   repeat for each line tFolder in the folders
      if tFolder is ".." then
         next repeat
      end if
      
      revCopyMobileFilesComputeFolderManifest xManifest, pSource & slash & tFolder, pTarget & slash & tFolder
   end repeat
   
   set the folder to tOldFolder
end revCopyMobileFilesComputeFolderManifest

################################################################################

-- MERG-2013-09-05: [[ Bug 11152 ]] Change to function to return whether the external was found or not. 
private function lcextExtractExternal pExternal, pBuild, pTarget
   revZipOpenArchive pExternal, "read"
   if the result is empty then
      revZipExtractItemToFile pExternal, "iOS/External-" & pBuild, pTarget
      revZipCloseArchive pExternal
   end if
   if not (there is file pTarget) then
      revStandaloneAddWarning "Could not find appropriate build of external '" & pExternal & "' for " & pBuild
      return false
   end if
   return true
end lcextExtractExternal

################################################################################

-- MM-2012-09-18: Updated to use the new copied externals array instead of the copy files,
-- where we extract the appropraite external for the instruciton set(s) we are building for.
--
private command revCopyMobileExternals pExternals, pAppBundle, pTarget, pSDKs, @rExternals 
   repeat for each word tInstSet in "sim armv6 armv7 arm64 arm"
      if pSDKs[tInstSet]["suffix"] is not empty then
         repeat for each line tLine in pExternals      
            local tFile
            put mapFilePath(pSDKs[tInstSet]["runtime path"] & slash & tLine) into tFile
            if there is a file tFile then
               if pTarget is not "device" then
                  revSBCopyFileToFile tFile, pAppBundle & slash & tLine & ".dylib", "revStandaloneProgressCallback", the long id of me
               end if
               put tFile into rExternals[tLine][tInstSet]
               put "external" into rExternals[tLine]["type"]
               put tLine & ".dylib" into rExternals[tLine]["location"] 
            else
               throw "Could not find external" && tLine && "for" && pTarget
            end if
         end repeat
      end if             
   end repeat     
end revCopyMobileExternals

-- MM-2012-09-18: Updated to use the new copied externals array instead of the copy files,
-- where we extract the appropraite external for the instruciton set(s) we are building for.
--
private command revCopyMobileDrivers pDrivers, pAppBundle, pTarget, pSDKs, @rExternals
   repeat for each word tInstSet in "sim armv6 armv7 arm64 arm"
      if pSDKs[tInstSet]["suffix"] is not empty then
         repeat for each line tLine in pDrivers
            local tFile
            put mapFilePath(pSDKs[tInstSet]["runtime path"] & slash & tLine) into tFile
            if there is a file tFile then
               if pTarget is not "device" then
                  revSBCopyFileToFile tFile, pAppBundle & slash & tLine & ".dylib", "revStandaloneProgressCallback", the long id of me
               end if
               put tFile into rExternals[tLine][tInstSet]
               put "dylib" into rExternals[tLine]["type"]
               put tLine & ".dylib" into rExternals[tLine]["location"]
            else
               throw "Could not find external" && tLine && "for" && pTarget
            end if
         end repeat
      end if             
   end repeat
end revCopyMobileDrivers

-- Helper function to convert the Xcode version to the format the plist expects. Examples:
-- 7.2 --> 0720
-- 8.3.3 --> 0833
-- 10.2 --> 1020
-- 10.2.1 --> 1021
-- relies on the fact that + 0 to empty gives 0
private function convertXcodeVersion pInput
   set the itemdel to "."
   return format("%02d%d%d", item 1 of pInput + 0, item 2 of pInput + 0, item 3 of pInput + 0)
end convertXcodeVersion

################################################################################

private command revCopyMobileStackFiles pStackFiles, pBaseFolder, pAppBundle
end revCopyMobileStackFiles

################################################################################

private function revGetMinimumOSByArch pMinimumOS 
   local tMinimumOSByArch  
   -- SN-2015-02-02: [[ Bug 14422 ]] Minimum OS version is the
   --  same for all archs
   -- We only support version >= 6.0
   --
   if pMinimumOS is empty or pMinimumOS < "6.0" then
      put "6.0" into tMinimumOSByArch["i386"]
   else
      put pMinimumOS after tMinimumOSByArch["i386"]
   end if
   
   -- Assign the same value to the other architectures.
   -- Possible architectures:
   --   i386, x86-64, armv6, armv7, armv7s, arm64, ppc, ppc64
   put tMinimumOSByArch["i386"] into tMinimumOSByArch["armv6"]
   put tMinimumOSByArch["i386"] into tMinimumOSByArch["armv7"]
   put tMinimumOSByArch["i386"] into tMinimumOSByArch["armv7s"]
   put tMinimumOSByArch["i386"] into tMinimumOSByArch["arm64"]
   put tMinimumOSByArch["i386"] into tMinimumOSByArch["x86-64"]
   return tMinimumOSByArch
end revGetMinimumOSByArch

private command revCreateMobilePlist pSettings, pAppBundle, pTarget, pFonts, pPlist
   local tTemplateFile
   
   if pPlist is empty then
      put mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "Settings.plist") into tTemplateFile
      if there is no file tTemplateFile then
         throw "could not find plist template for" && tolower(pTarget) && "target"
      end if
   end if
   
   ----------
   
   local tExeName, tBundleId, tBundleVersion,tBundleBuild,tMinimumOSByArch
   put pSettings["name"] into tExeName
   put pSettings["ios,bundle id"] into tBundleId
   put pSettings["ios,bundle version"] into tBundleVersion
   put pSettings["ios,bundle build"] into tBundleBuild
   
   -- MW-2011-03-15: Make sure the bundle version is defaulted to 1.0.0 if empty
   if tBundleVersion is empty then
      put "1.0.0" into tBundleVersion
   end if
   
   if tBundleBuild is empty then
      put "1" into tBundleBuild
   end if
   ----------
   
   local tDisplayName, tMinimumOS, tDeviceFamily, tRequiredCapabilities, tPersistentWifi, tEnableBackgroundExecution
   local tDisableATS
   local tFileSharing, tPrerenderedIcon
   put pSettings["ios,display name"] into tDisplayName
   put pSettings["ios,minimum version"] into tMinimumOS
   put pSettings["ios,device family"] into tDeviceFamily
   put pSettings["ios,device capabilities"] into tRequiredCapabilities
   put pSettings["ios,persistent wifi"] into tPersistentWifi
   -- MW-2011-03-10: Force this to true for now, until we can fix the engine
   put pSettings["ios,enable background execution"] into tEnableBackgroundExecution
   
   -- AB-2017-05-31: Add more background modes
   local tBackgroundModes
   
   if tEnableBackgroundExecution is empty then
      put "false" into tEnableBackgroundExecution
   end if
   
   if tEnableBackgroundExecution then
      put pSettings["ios,background audio"] into tBackgroundModes["${PLAY_AUDIO_WHEN_IN_BACKGROUND}"]
      put pSettings["ios,background location update"] into tBackgroundModes["${BACKGROUND_LOCATION_UPDATE}"]
      put pSettings["ios,background voip"] into tBackgroundModes["${BACKGROUND_VOIP}"]
      put pSettings["ios,background newsstand downloads"] into tBackgroundModes["${BACKGROUND_NEWSSTAND_DOWNLOADS}"]
      put pSettings["ios,external acc comn"] into tBackgroundModes["${EXTERNAL_ACC_COMM}"]
      put pSettings["ios,use bt le"] into tBackgroundModes["${USE_BT_LE}"]
      put pSettings["ios,acts as bt le"] into tBackgroundModes["${ACTS_AS_BT_LE}"]
      put pSettings["ios,background fetch"] into tBackgroundModes["${BACKGROUND_FETCH}"]
      put pSettings["ios,remote notifications"] into tBackgroundModes["${REMOTE_NOTIFICATIONS}"] 
   else
      put false into tBackgroundModes["${PLAY_AUDIO_WHEN_IN_BACKGROUND}"]
      put false into tBackgroundModes["${BACKGROUND_LOCATION_UPDATE}"]
      put false into tBackgroundModes["${BACKGROUND_VOIP}"]
      put false into tBackgroundModes["${BACKGROUND_NEWSSTAND_DOWNLOADS}"]
      put false into tBackgroundModes["${EXTERNAL_ACC_COMM}"]
      put false into tBackgroundModes["${USE_BT_LE}"]
      put false into tBackgroundModes["${ACTS_AS_BT_LE}"]
      put false into tBackgroundModes["${BACKGROUND_FETCH}"]
      put false into tBackgroundModes["${REMOTE_NOTIFICATIONS}"] 
   end if
   put pSettings["ios,file sharing"] into tFileSharing
   put pSettings["ios,prerendered icon"] into tPrerenderedIcon 
   
   local tURLWhitelist
   put pSettings["ios,urlwhitelist"] into tURLWhitelist
   
   if tURLWhitelist is empty then
      put "" into tURLWhitelist
   end if
   
   if tDisplayName is empty then
      put tExeName into tDisplayName
   end if
   
   put revGetMinimumOSByArch(tMinimumOS) into tMinimumOSByArch
   if tDeviceFamily is empty then
      put "1" into tDeviceFamily
   end if
   if tPersistentWifi is empty then
      put "false" into tPersistentWifi
   end if
   
   if tFileSharing is empty then
      put "false" into tFileSharing
   end if
   if tPrerenderedIcon is empty then
      put "false" into tPrerenderedIcon
   end if
   
   -- tExitsOnSuspend defaulted to true. We want this to be false to allow background audio
   repeat for each key tKey in tBackgroundModes
      if tBackgroundModes[tKey] is empty then
         put false into tBackgroundModes[tKey]
      end if
   end repeat   
   
   // SN-2015-02-11: [[ Bug 16299 ]] Add ${DISABLE_ATS} in the Plist template
   if pSettings["ios,disable ATS"] then
      put URL("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "DisableATS.plist")) into tDisableATS
   end if
   
   
   ----------
   -- PM-2015-02-17: [[ Bug 14482 ]] Added a new "solid" statusbarstyle
   local tStatusBarHidden, tStatusBarStyle, tIPadStatusBarHidden, tStatusBarSolid
   put pSettings["ios,status bar hidden"] into tStatusBarHidden
   put pSettings["ios,status bar style"] into tStatusBarStyle
   put pSettings["ios,ipad status bar hidden"] into tIPadStatusBarHidden
   
   -- MM-2011-09-28: The orientations settings have now been adjusted.  The standalone builder should wipe out
   -- any old oreintation settings.  If the user has not visited the new standalone builder then the new settings will not
   -- be present and the old settings will still be there.  If this is the case, port old settings.
   --
   local tIPhoneOrientation, tIPadOrientations
   if pSettings["ios,initial orientation"] is not empty  then
      put pSettings["ios,initial orientation"] into tIPhoneOrientation
   else if pSettings["ios,iphone initial orientation"] is not empty then
      put pSettings["ios,iphone initial orientation"] into tIPhoneOrientation
   else
      put "Portrait" into tIPhoneOrientation
   end if
   if pSettings["ios,interface orientations"] is not empty then
      put pSettings["ios,interface orientations"] into tIPadOrientations
   else if  pSettings["ios,ipad intial orientations"] is not empty then
      put pSettings["ios,ipad intial orientations"] into tIPadOrientations
   else
      if 2 is among the items of pSettings["ios,device family"] then
         revStandaloneAddWarning "No iPad supported intial orientation.  Assuming Portrait."
         put "Portrait" into tIPadOrientations
      end if
   end if 
   
   -- MM-2011-09-28: Make sure portrait and landscape iPad splashes are present if required.
   --
   if 2 is among the items of pSettings["ios,device family"] then
      if tIPadOrientations contains "Portrait" and there is no file (pAppBundle & "/Default-Portrait.png") then
         revStandaloneAddWarning "No iPad Portrait Splash Screen"
      end if
      if tIPadOrientations contains "Landscape" and there is no file (pAppBundle & "/Default-Landscape.png") then
         revStandaloneAddWarning "No iPad Landscape Splash Screen"
      end if
   end if
   
   if tStatusBarHidden is empty then
      put "false" into tStatusBarHidden
   end if
   if tIPadStatusBarHidden is empty then
      put "false" into tIPadStatusBarHidden
   end if
   if tStatusBarStyle is empty then
      put "Default" into tStatusBarStyle
   end if
   -- PM-2015-02-17: [[ Bug 14482 ]] Added a new "solid" statusbarstyle, 
   -- which is opaque and automatically shifts down the app view by 20 pixels
   if tStatusBarStyle is "solid" then
      put "BlackOpaque" into tStatusBarStyle
      put true into tStatusBarSolid
   end if
   
   ----------
   
   local tCustomFonts
   repeat for each line tEntry in pFonts
      if item 1 of tEntry is not "font" then
         next repeat
      end if
      put "<string>" & char (the number of chars in pAppBundle + 2) to -1 of item 2 to -1 of tEntry & "</string>" & return after tCustomFonts
   end repeat
   
   ----------
   
   local tHealthKit
   if pSettings["externals"] contains "mergHK" then
      put "<key>NSHealthShareUsageDescription</key>" & CR & \
            "<string>This application requires access to read the user's health data</string>" & CR & \
            "<key>NSHealthUpdateUsageDescription</key>" & CR & \
            "<string>This application requires access to update the user's health data</string>" \
            into tHealthKit
   else
      put empty into tHealthKit
   end if
   
   ----------
   
   if pPlist is empty then
      put url ("binfile:" & tTemplateFile) into pPlist
   end if
   // SN-2014-10-27: [[ Bug 13827 ]] Strings in a Plist file should be UTF-8 encoded
   replace "${EXECUTABLE_NAME}" with textEncode(tExeName, "utf-8") in pPlist
   replace "${BUNDLE_IDENTIFIER}" with textEncode(tBundleId, "utf-8") in pPlist
   replace "${BUNDLE_VERSION}" with tBundleVersion in pPlist
   replace "${BUNDLE_BUILD}" with tBundleBuild in pPlist
   replace "${BUNDLE_DISPLAY_NAME_SUPPORT}" with "<string>" & \ 	
         textEncode(tDisplayName, "utf-8") & "</string>" in pPlist
   		
   -- SN-2015-02-02: [[ Bug 14422 ]] We now have minimum version depending on the arch.
   replace "${MINIMUM_OS_SUPPORT}" with "<string>" & tMinimumOSByArch["i386"] & "</string>" in pPlist
   replace "${CUSTOM_FONTS}" with tCustomFonts in pPlist
   replace "${DISABLE_ATS}" with tDisableATS in pPlist
   replace "${HEALTHKIT}" with tHealthKit in pPlist
   
   -- PM-2018-01-15: [[Bug 20852]] Get info about Xcode/SDK versions
   if pTarget is "Device" then
      local tDtPlatformBuild, tDtPlatformVersion, tDtSdkBuild, tDtSdkName, tDtXcode, tDtXcodeBuild
      
      -- Note: The values returned by the shell commands contain a trailing LF, so we have to remove it
      get shell("xcodebuild -version -sdk iphoneos ProductBuildVersion 2> /dev/null")
      -- Those 2 values are the same, e.g. "15C107"
      put it into tDtPlatformBuild
      delete last char of tDtPlatformBuild
      put it into tDtSdkBuild 
      delete last char of tDtSdkBuild
      
      get shell("xcodebuild -version -sdk iphoneos PlatformVersion 2> /dev/null") -- e.g. "11.2"
      put it into tDtPlatformVersion
      delete last char of tDtPlatformVersion
      
      put "iphoneos" & tDtPlatformVersion into tDtSdkName
      
      get shell("xcodebuild -version 2> /dev/null| awk '/Xcode/ {print $NF}' ") -- e.g. "9.2"
      put it into tDtXcode
      put convertXcodeVersion(tDtXcode) into tDtXcode 
      
      get shell("xcodebuild -version 2> /dev/null| awk 'END {print $NF}' ") -- e.g. "9C40b"
      put it into tDtXcodeBuild
      delete last char of tDtXcodeBuild
      
      replace "${DT_PLATFORM_BUILD}" with "<string>" & tDtPlatformBuild & "</string>" in pPlist
      replace "${DT_PLATFORM_VERSION}" with "<string>" & tDtPlatformVersion & "</string>" in pPlist
      replace "${DT_SDK_BUILD}" with "<string>" & tDtSdkBuild & "</string>" in pPlist
      replace "${DT_SDK_NAME}" with "<string>" & tDtSdkName & "</string>" in pPlist
      replace "${DT_XCODE}" with "<string>" & tDtXcode & "</string>" in pPlist
      replace "${DT_XCODE_BUILD}" with "<string>" & tDtXcodeBuild & "</string>" in pPlist
      
   end if
   
   get empty
   repeat for each item tItem in tDeviceFamily
      put "<integer>" & tItem & "</integer>" after it
   end repeat
   replace "${DEVICE_SUPPORT}" with it in pPlist
   
   get empty
   repeat for each key tKey in tRequiredCapabilities
      if tRequiredCapabilities[tKey] is "true" then
         put "<key>" & tKey & "</key><true/>" after it
      else if tRequiredCapabilities[tKey] is "false" then
         put "<key>" & tKey & "</key><false/>" after it
      end if
   end repeat
   
   -- MM-2011-09-28: If building for arm v7 only, then add arm v7 required plist value
   --
   if pSettings["ios,build type"] is "Arm v7" then
      put "<key>armv7</key><true/>" after it
   end if
   
   -- Building for iOS 11.0 or more only builds the 64bit slice, so update the plist
   if pSettings["ios,minimum version"] >= 11.0 then
      put "<key>arm64</key><true/>" after it
   end if
   
   replace "${DEVICE_CAPABILITY}" with it in pPlist
   
   replace "${PERSISTENT_WIFI}" with "<" & tPersistentWifi & "/>" in pPlist
   replace "${APPLICATION_EXITS_ON_SUSPEND}" with "<" & not tEnableBackgroundExecution & "/>" in pPlist
   replace "${FILE_SHARING}" with "<" & tFileSharing & "/>" in pPlist
   replace "${PRE_RENDERED_ICON}" with "<" & tPrerenderedIcon & "/>" in pPlist
   
   -- MW-2010-11-30: We need to make sure the first item in the list is the
   --     initial orientation as otherwise we get odd behavior (in the simulator
   --     at least!)
   -- MM-2011-09-28: Updated oreintation handling to generate new plist values.
   --   
   replace "${IPHONE_INITIAL_ORIENTATION}" with "<string>UIInterfaceOrientation" & tIPhoneOrientation & "</string>" in pPlist
   replace "${IPHONE_SUPPORTED_ORIENTATIONS}" with "<string>UIInterfaceOrientation" & tIPhoneOrientation & "</string>" in pPlist
   replace "${IPAD_INITIAL_ORIENTATION}" with "<string>UIInterfaceOrientation" & item 1 of  tIPadOrientations & "</string>" in pPlist
   get empty
   repeat for each item tItem in tIPadOrientations
      put "<string>UIInterfaceOrientation" & tItem & "</string>" after it
   end repeat 
   replace "${IPAD_SUPPORTED_ORIENTATIONS}" with it in pPlist
   
   replace "${STATUS_BAR_STYLE}" with "<string>UIStatusBarStyle" & tStatusBarStyle & "</string>" in pPlist
   replace "${STATUS_BAR_HIDDEN}" with "<" & tStatusBarHidden & "/>" in pPlist
   replace "${IPAD_STATUS_BAR_HIDDEN}" with "<" & tIPadStatusBarHidden & "/>" in pPlist
   
   -- PM-2015-02-17: [[ Bug 14482 ]] Added a new "solid" statusbarstyle
   if tStatusBarSolid is empty then
      put false into tStatusBarSolid
   end if
   replace "${STATUS_BAR_SOLID}" with "<" & tStatusBarSolid & "/>" in pPlist
   
   -- the icons-related part of the plist is stored in sCFBundleIcons
   replace "${BUNDLE_ICONS}" with sCFBundleIcons in pPlist
   
   -- MM-2014-09-30: [[ iOS8 Support ]] Added new iOS 7 style entries for the splash screens
   -- MM-2014-10-06: [[ Bug 13512 ]] Splash screen sizes are in points and should omit the @2x @3x suffix, iOS is smart enough to figure that out.
   local tSplashScreens   
   if 1 is among the items of tDeviceFamily then
      put "Default|Default|{320, 480}|Portrait" & return after tSplashScreens
      put "Default-568h@2x|Default-568h|{320, 568}|Portrait" & return after tSplashScreens      
      -- PM-2015-03-19: [[ Bug 13969 ]] Use the correct name for iPhone 6/6+ splash screens
      put "Default-667h@2x|Default-667h|{375, 667}|Portrait" & return after tSplashScreens
      if tIPhoneOrientation contains "Portrait" then
         put "Default-736h@3x|Default-736h|{414, 736}|Portrait" & return after tSplashScreens
         put "Default-812h@3x|Default-812h|{375, 812}|Portrait" & return after tSplashScreens
         // iPhone XR
         put "Default-896h@2x|Default-896h|{414, 896}|Portrait" & return after tSplashScreens
         // iPhone XSMAX
         put "Default-896h@3x|Default-896h|{414, 896}|Portrait" & return after tSplashScreens
      else
         put "Default-414h@3x|Default-414h|{414, 736}|Landscape" & return after tSplashScreens
         put "Default-375h@3x|Default-375h|{375, 812}|Landscape" & return after tSplashScreens
         // iPhone XR
         put "Default-414h@2x|Default-414h|{414, 896}|Landscape" & return after tSplashScreens
         // iPhone XSMAX
         put "Default-414h@3x|Default-414h|{414, 896}|Landscape" & return after tSplashScreens
      end if      
   end if   
   if 2 is among the items of tDeviceFamily then      
      if tIPadOrientations contains "Portrait" then
         put "Default-Portrait|Default-Portrait|{768, 1024}|Portrait" & return after tSplashScreens
         put "Default-iPadProPortrait|Default-iPadProPortrait|{1024, 1366}|Portrait" & return after tSplashScreens
      end if
      if tIPadOrientations contains "Landscape" then
         put "Default-Landscape|Default-Landscape|{768, 1024}|Landscape" & return after tSplashScreens
         put "Default-iPadProLandscape|Default-iPadProLandscape|{1024, 1366}|Landscape" & return after tSplashScreens
      end if
   end if
   
   local tSplashTemplate, tSplashData
   put URL ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "SplashscreenSettings.plist")) into tSplashTemplate 
   
   set the itemDel to "|"
   repeat for each line tSplashScreen in tSplashScreens
      if there is a file (pAppBundle & slash & item 1 of tSplashScreen & ".png") or \
            there is a file (pAppBundle & slash & item 1 of tSplashScreen & "@2x.png") or \
            there is a file (pAppBundle & slash & item 1 of tSplashScreen & "@3x.png")
      then
         get tSplashTemplate 
         replace "${NAME}" with item 2 of tSplashScreen in it
         replace "${SIZE}" with item 3 of tSplashScreen in it
         replace "${ORIENTATION}" with item 4 of tSplashScreen in it
         put it & return after tSplashData
      end if
   end repeat
   set the itemDel to comma
   
   replace "${SPLASHSCREENS}" with tSplashData in pPlist
   
   ----------
   -- MM-2012-02-12: Added support for push notificaitons and custom URL schemes
   
   if pSettings["ios,push notifications"] then
      replace "${REMOTE_NOTIFICATION_TYPES}" with URL ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "RemoteNotificationSettings.plist")) in pPlist      
   else
      replace "${REMOTE_NOTIFICATION_TYPES}" with empty in pPlist
   end if
   
   local tUsesLocalNotifications
   if pSettings["ios,local notifications"] then
      put "true" into tUsesLocalNotifications   
   else
      put "false" into tUsesLocalNotifications 
   end if
   
   replace "${USES_LOCAL_NOTIFICATIONS}" with "<" & tUsesLocalNotifications & "/>" in pPlist
   
   if pSettings["ios,url name"] is not empty then
      get URL ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "URLSchemeSettings.plist"))
      replace "${BUNDLE_IDENTIFIER}" with tBundleId in it
      replace "${URL_NAME}" with pSettings["ios,url name"] in it
      replace "${URL_TYPES}" with it in pPlist
   else
      replace "${URL_TYPES}" with empty in pPlist
   end if
   
   ----------
   -- PM-2014-10-20: [[ Bug 13590]] Set the location authorization type and description
   -- PM-2014-10-20: [[ Bugs 13764 and 14290]] gps and location-services should work even if n/a is selected in standalone settings
   if tRequiredCapabilities["gps"] is not false or tRequiredCapabilities["location-services"] is not false then
      local tLocationAuthType
      put pSettings["ios,location auth type"] into tLocationAuthType
      if tLocationAuthType is empty then
         put "always" into tLocationAuthType
      end if
      
      // On iOS 11+ we need a new key NSLocationAlwaysAndWhenInUseUsageDescription AND the old NSLocationWhenInUseUsageDescription key, in case 
      // we want the app to access Location Services when it is used. If we want to access Location Services always, we need the old NSLocationAlwaysUsageDescription 
      // key too (i.e. 3 keys in total). Moreover, NSLocationAlwaysUsageDescription is required for backwards compatibility
      local tSdkVersion
      put revIDEDeployIOSGetSDKVersions() into tSdkVersion
      set the itemDel to "."
      if item 1 of tSdkVersion >= 11 then
         get "<key>NSLocationAlwaysAndWhenInUseUsageDescription</key><string>This application requires access to Location Services</string>" & CR \
               & "<key>NSLocationWhenInUseUsageDescription</key><string>This application requires access to Location Services when in use</string>" 
         
         if tLocationAuthType is "always" then
            put CR & "<key>NSLocationAlwaysUsageDescription</key><string>This application requires access to Location Services always</string>" after it
         end if
      else
         if tLocationAuthType is "always" then
            get "<key>NSLocationAlwaysUsageDescription</key><string>This application requires access to Location Services always</string>"
         else
            get "<key>NSLocationWhenInUseUsageDescription</key><string>This application requires access to Location Services when in use</string>"
         end if
      end if
   else
      get empty
   end if
   replace "${LOCATION_AUTH_TYPE}" with it in pPlist
   
   -- AB : helper stmts to generate the actual strings that we are going to insert 
   -- the background modes into the plist file
   
   local tBackgroundValues
   put "<string>audio</string>" into tBackgroundValues["${PLAY_AUDIO_WHEN_IN_BACKGROUND}"]
   put "<string>location</string>" into tBackgroundValues["${BACKGROUND_LOCATION_UPDATE}"]
   put "<string>voip</string>" into tBackgroundValues["${BACKGROUND_VOIP}"]
   put "<string>newsstand-content</string>" into tBackgroundValues["${BACKGROUND_NEWSSTAND_DOWNLOADS}"]
   put "<string>external-accessory</string>" into tBackgroundValues["${EXTERNAL_ACC_COMM}"]
   put "<string>bluetooth-central</string>" into tBackgroundValues["${USE_BT_LE}"]
   put "<string>bluetooth-peripheral</string>" into tBackgroundValues["${ACTS_AS_BT_LE}"]
   put "<string>fetch</string>" into tBackgroundValues["${BACKGROUND_FETCH}"]
   put "<string>remote-notification</string>" into tBackgroundValues["${REMOTE_NOTIFICATIONS}"]
   
   -- PM-2015-10-08:  [[ Bug 16154 ]] Allow audio playing when the app is on background
   -- AB-2017-05-31: Add BackgroundModes to Plist
   repeat for each key tKey in tBackgroundModes
      if tBackgroundModes[tKey] is true then
         replace tKey with tBackgroundValues[tKey] in pPlist
      else
         replace tKey with empty in pPlist
      end if
   end repeat
   
   -- AB-2017-05-31: Add App URL Query Whitelist
   local tWhitelistStrings
   repeat for each item tItem in tURLWhitelist 
      put "<string>" & tItem & "</string>" & return after tWhitelistStrings
   end repeat
   replace "${APP_URL_WHITELIST}" with tWhitelistStrings in pPlist
   
   
   ----------   
   
   put pPlist into url ("binfile:" & pAppBundle & slash & "Info.plist")   
end revCreateMobilePlist

################################################################################

private function revGetMobileProfileInfo pProfileId
   local tContents
   put url ("file:~/Library/MobileDevice/Provisioning Profiles/" & pProfileId & ".mobileprovision") into tContents
   
   local tAppId, tName, tIsDist, tIdentities
   
   get lineOffset("<key>DeveloperCertificates</key>", tContents)
   if it is 0 then
      throw "invalid provisioning profile"
   end if
   
   local tCertIndex, tCerts
   get line it + 2 to it + (lineOffset("</array>", tContents, it) - 1) of tContents
   repeat for each line tLine in it
      if tLine contains "<data>" and tLine contains "</data>" then
         add 1 to tCertIndex
         put char (offset("<data>", tLine) + 6) to  offset("</data>", tLine) of tLine into tCerts[tCertIndex]
      else if tLine contains "<data>" then
         add 1 to tCertIndex
      else if not (tLine contains "</data>") then
         put tLine & return after tCerts[tCertIndex]
      end if
   end repeat
   
   repeat for each element tCert in tCerts
      put the base64Decode of tCert into tCert
      if tCert contains "Distribution" then
         put true into tIsDist
         get offset("iPhone Distribution:", tCert)
      else if tCert Contains "iOS Development:" then
         get offset("iOS Development:", tCert)
      else if tCert Contains "Apple Development:" then
         get offset("Apple Development:", tCert)
      else
         get offset("iPhone Developer:", tCert)
      end if
      put char it to it + charToNum(char it - 1 of tCert) - 1 of tCert & return after tIdentities
   end repeat
   delete the last char of tIdentities
   
   get lineOffset("<key>ApplicationIdentifierPrefix</key>", tContents)
   if it is 0 then
      throw "invalid provisioning profile"
   end if
   get word 1 to -1 of line it + 2 of tContents
   get char 9 to -10 of it
   put it into tAppId
   
   get lineOffset("<key>Name</key>", tContents)
   if it is 0 then
      throw "invalid provisioning profile"
   end if
   get word 1 to -1 of line it + 1 of tContents
   put char 9 to -10 of it into tName
   
   local tInfo
   put pProfileId into tInfo["id"]
   put tName into tInfo["name"]
   put tAppId into tInfo["appid"]
   put tIdentities into tInfo["identities"]
   put tIsDist into tInfo["store"]
   
   return tInfo
end revGetMobileProfileInfo

################################################################################
function revMobileRuntimeFolder pTarget, pVersion
   local tStub
   if pTarget is "Device" then
      if pVersion is empty then
         // SN-2015-04-30: [[ Bug 15175 ]] Take the latest valid SDK installed
         // (can be 8.2 or 8.3 onwards on Yosemite).
         local tVersion
         if revIDEDeployIOSGetLatestSDKInstalled(tVersion) then
            replace "." with "_" in tVersion
            put tVersion into pVersion
         end if
      end if
      put "Device-" & pVersion into tStub
   else
      // SN-2015-05-01: [[ Refactor iOS SDK version ]] Build the simulator location
      //  stub on the fly.
      replace "." with "_" in pTarget
      replace space with "-" in pTarget
      put pTarget into tStub
   end if
   
   local tUserPath, tOverridePath
   
   put revOverrideRuntimePath() & slash & "iOS/" & tStub into tOverridePath
   if there is a folder tOverridePath then
      return tOverridePath
   end if
   
   put revEnvironmentUserRuntimePath() & slash & "iOS/" & tStub into tUserPath
   if there is a folder tUserPath then
      return tUserPath
   end if
   
   return revEnvironmentRuntimePath() & slash & "iOS/" & tStub
end revMobileRuntimeFolder

################################################################################

-- MW-2013-06-13: [[ CloneAndRun ]] Map a file path from where the sb expects them to be to local
--   files in the build folder.
function mapFilePath pPath
   if revEnvironmentIsInstalled() then
      return pPath
   end if
   
   local tIsDevice
   put pPath contains "/Device-" into tIsDevice
   
   local tRepo, tComponent, tBuild, tSuffix
   set the itemDelimiter to slash
   put revEnvironmentRepositoryPath() into tRepo
   put the last item of pPath into tComponent
   put item -2 of pPath into tBuild
   replace "Simulator-" with "iphonesimulator" in tBuild
   replace "Device-" with "iphoneos" in tBuild
   replace "_" with "." in tBuild
   if revEnvironmentBinariesPath() contains "/Debug" then
      put "/Debug" after tBuild
   else
      put "/Release" after tBuild
   end if
   
   if the editionType is "community" then
      put "community" into tSuffix
   else
      put "commercial" into tSuffix
   end if
   
   local tPath
   switch the last item of pPath
      case "Settings.plist"
         if tIsDevice then
            put tRepo & slash & "engine/rsrc/mobile-device-template.plist" into tPath
         else
            put tRepo & slash & "engine/rsrc/mobile-template.plist" into tPath
         end if
         break
      case "Entitlements.xcent"
         put tRepo & slash & "engine/rsrc/template-entitlements.xcent" into tPath
         break
      case "StoreEntitlements.xcent"
         put tRepo & slash & "engine/rsrc/template-store-entitlements.xcent" into tPath
         break
      case "RemoteNotificationEntitlements.xcent"
         put tRepo & slash & "engine/rsrc/template-remote-notification-entitlements.xcent" into tPath
         break
      case "RemoteNotificationStoreEntitlements.xcent"
         put tRepo & slash & "engine/rsrc/template-remote-notification-store-entitlements.xcent" into tPath
         break
      case "BetaReportEntitlement.xcent"
         put tRepo & slash & "engine/rsrc/template-beta-report-entitlement.xcent" into tPath
         break
      case "DisableATS.plist"
         put tRepo & slash & "engine/rsrc/mobile-disable-ats-template.plist" into tPath
         break
      case "RemoteNotificationSettings.plist"
         put tRepo & slash & "engine/rsrc/mobile-remote-notification-template.plist" into tPath
         break
      case "URLSchemeSettings.plist"
         put tRepo & slash & "engine/rsrc/mobile-url-scheme-template.plist" into tPath
         break
      case "SplashscreenSettings.plist"
         put tRepo & slash & "engine/rsrc/mobile-splashscreen-template.plist" into tPath
         break
      case "Standalone"
         if tIsDevice then
            put tRepo & merge("/_build/ios/[[tBuild]]/standalone-mobile-lib-[[tSuffix]].lcext") into tPath
         else
            put tRepo & merge("/_build/ios/[[tBuild]]/standalone-mobile-[[tSuffix]].ios-engine") into tPath
         end if
         break
      case "revzip"
      case "revxml"
      case "revdb"
      case "dbsqlite"
      case "dbmysql"
      case "revpdfprinter"
      case "revsecurity"
         if tIsDevice then
            put tRepo & merge("/_build/ios/[[tBuild]]/[[the last item of pPath]].lcext") into tPath
         else
            put tRepo & merge("/_build/ios/[[tBuild]]/[[the last item of pPath]].dylib") into tPath
         end if
         break
      case "Default4InchSplash.png"
         put tRepo & slash & "engine/rsrc/Default-568h@2x.png" into tPath
         break
      default
         put pPath into tPath
         break
   end switch
   
   return tPath
end mapFilePath

################################################################################

// SN-2015-11-10: [[ Bug 16389 ]] Escape all the single quotes in pString
// The only way to escape them is to stop the single-quote string,
// added a single quote in a double-quoted string, and resuming the
// single quote string.
private function escapeSingleQuote pString
   replace "'" with "'" & quote & "'" & quote & "'" in pString
   return pString
end escapeSingleQuote

private command symlinkRedirect pSource, pTarget
   if there is a file pTarget then
      // We don't want to erase files that have possibly be already copied ith the same name
      // such as externals, fonts or plist files.
      exit symlinkRedirect
   end if
   if there is a file pSource then
      // Hardlink, since iOS simulator 9.0 does not seem able to handle symlinks
      local tEscapedSource, tEscapedTarget
      put escapeSingleQuote(pSource) into tEscapedSource
      put escapeSingleQuote(pTarget) into tEscapedTarget
      
      get shell(merge("ln '[[tEscapedSource]]' '[[tEscapedTarget]]' || cp '[[tEscapedSource]]' '[[tEscapedTarget]]'"))
      
      if the result is not empty then
         // Both 'ln' and 'cp' failedn
         throw "Cannot copy" && pSource
      end if
   else if there is a folder pSource then
      local tOldFolder
      put the folder into tOldFolder
      set the folder to pSource      
      set the itemDel to slash
      local tFiles
      put the files & return & the folders into tFiles
      repeat for each line tFile in tFiles
         if tFile is not empty and not (tFile begins with ".") then
            symlinkRedirect pSource & slash & tFile, pTarget & slash & tFile
         end if
      end repeat  
      set the folder to tOldFolder
   end if   
end symlinkRedirect

################################################################################
