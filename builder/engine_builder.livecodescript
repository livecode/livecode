script "EngineBuilder"
---------------------------------------------------------------------

private function engineBuilderVMRunPath
   return "/Applications/VMware Fusion.app/Contents/Library/vmrun"
end engineBuilderVMRunPath

private function engineBuilderVMFolder
   return "/Users/michael/Documents/Virtual Machines"
end engineBuilderVMFolder

private function engineBuilderWindowsVMPath
   return engineBuilderVMFolder() & "/Windows - Build.vmwarevm"
end engineBuilderWindowsVMPath

private function engineBuilderLinuxVMPath
   return engineBuilderVMFolder() & "/Linux - Build.vmwarevm"
end engineBuilderLinuxVMPath

private function engineBuilderLinuxVMRepoPath
   return "/host/Workspace/livecode/builds/livecode"
end engineBuilderLinuxVMRepoPath

private function engineBuilderLinuxVMCommercialPath
   return "/host/Workspace/livecode/builds/livecode-commercial"
end engineBuilderLinuxVMCommercialPath

private function engineBuilderWindowsVMCommercialPath
   return "C:/Workspace/livecode"
end engineBuilderWindowsVMCommercialPath

private function engineBuilderWindowsVMRepoPath
   return "C:/Workspace/livecode-commercial"
end engineBuilderWindowsVMRepoPath

private function engineBuilderLogFolder
   return builderRepoFolder() & "/_build/logs"
end engineBuilderLogFolder

private function engineBuilderLogFolderLinuxVM
   return engineBuilderLinuxVMRepoPath() & "/_build/logs"
end engineBuilderLogFolderLinuxVM

private function engineBuilderLogFolderWindowsVM
   return engineBuilderLinuxWimdowsRepoPath() & "/_build/logs"
end engineBuilderLogFolderWindowsVM

---------------------------------------------------------------------

private function wrapQ pVal
   return quote & pVal & quote
end wrapQ

---------------------------------------------------------------------

private command engineBuilderBuildMac pEdition, pVersion
   builderLog "report", "Building Mac engines..."
   put empty into URL ("file:" & engineBuilderLogFolder() & "/error_mac.log")
   put empty into URL ("file:" & engineBuilderLogFolder() & "/build_mac.log")
   
   local tCommand
   put "sh" && wrapQ(builderCommercialFolder() & "/build-mac.osx") && ">" &&  wrapQ(engineBuilderLogFolder() & "/build_mac.log") && "2>"  && wrapQ(engineBuilderLogFolder() & "/error_mac.log") into tCommand
   get shell(tCommand)
   
   builderLog "message", "Mac build result..."
   builderLog "message", URL ("file:" & engineBuilderLogFolder() & "/error_mac.log")
   builderLog "report", "Building Mac complete."
end engineBuilderBuildMac

private command engineBuilderBuildIOS pEdition, pVersion
   builderLog "report", "Building iOS engines..."
   put empty into URL ("file:" & engineBuilderLogFolder() & "/error_ios.log")
   put empty into URL ("file:" & engineBuilderLogFolder() & "/build_ios.log")
   
   local tCommand
   put "sh" && wrapQ(builderCommercialFolder() & "/build-ios.osx") && ">" &&  wrapQ(engineBuilderLogFolder() & "/build_ios.log") && "2>"  && wrapQ(engineBuilderLogFolder() & "/error_ios.log") into tCommand
   get shell(tCommand)
   
   builderLog "message", "iOS build result..."
   builderLog "message", URL ("file:" & engineBuilderLogFolder() & "/error_ios.log")
   builderLog "report", "Building iOS complete."
end engineBuilderBuildIOS

private command engineBuilderBuildAndroid pEdition, pVersion
   builderLog "report", "Building Android engines..."
   put empty into URL ("file:" & engineBuilderLogFolder() & "/error_android.log")
   put empty into URL ("file:" & engineBuilderLogFolder() & "/build_android.log")
   
   local tCommand
   put "sh" && wrapQ(builderCommercialFolder() & "/build-android.osx") && ">" &&  wrapQ(engineBuilderLogFolder() & "/build_android.log") && "2>"  && wrapQ(engineBuilderLogFolder() & "/error_android.log") into tCommand
   get shell(tCommand)
   
   builderLog "message", "Android build result..."
   builderLog "message", URL ("file:" & engineBuilderLogFolder() & "/error_android.log")
   builderLog "report", "Building Android complete."
end engineBuilderBuildAndroid

private command engineBuilderBuildLinux pEdition, pVersion
   local tCommand   
   
   builderLog "report", "Building Linux engines..."
   put empty into URL ("file:" & engineBuilderLogFolder() & "/error_linux.log")
   put empty into URL ("file:" & engineBuilderLogFolder() & "/build_linux.log")
   
   builderLog "message", "Starting VM..."
   put wrapQ(engineBuilderVMRunPath()) && "-T fusion start" && wrapQ(engineBuilderLinuxVMPath()) && "nogui" into tCommand
   get shell(tCommand)
   if it contains "error" then
      builderLog "report", "Error starting VM:" && it
      throw "failure"
   end if
   
   builderLog "message", "Compiling engines..."
   put wrapQ(engineBuilderVMRunPath()) && "-T fusion -gu build -gp runlog runScriptInGuest" && wrapQ(engineBuilderLinuxVMPath()) && "/bin/sh" && wrapQ(engineBuilderLinuxVMCommercialPath() & "/build-linux.lnx >" && engineBuilderLogFolderLinuxVM() & "/build_linux.log" && "2>" && engineBuilderLogFolderLinuxVM() & "/error_linux.log") into tCommand
   get shell(tCommand)
   if it contains "error" then
      builderLog "report", "Error building Linux engines:" && it
      throw "failure"
   end if   
   builderLog "message", "Linux compile result..."
   builderLog "message", URL ("file:" & engineBuilderLogFolder() & "/error_linux.log")
   
   builderLog "message", "Suspending VM..."
   put wrapQ(engineBuilderVMRunPath()) && "-T fusion suspend" && wrapQ(engineBuilderLinuxVMPath()) into tCommand
   get shell(tCommand)
   if it contains "error" then
      builderLog "report", "Error suspending VM:" && it
      throw "failure"
   end if
   
   builderLog "report", "Building Linux complete."
end engineBuilderBuildLinux

private command engineBuilderBuildWindows pEdition, pVersion
   exit engineBuilderBuildWindows
   
   builderLog "report", "Building Windows engines..."
   put empty into URL ("file:" & engineBuilderLogFolder() & "/error_windows.log")
   
   local tCommand   
   put wrapQ(engineBuilderVMRunPath()) && "-T fusion -gu administrator -gp runlog runScriptInGuest" && wrapQ(engineBuilderWindowsVMPath()) && wrapQ() && "cmd.exe /k \" & wrapQ(engineBuilderWindowsVMCommercialPath() & "/build-windows.bat" && pVersion && builderBuildsFolder()) into tCommand
   get shell(tCommand)
   
   if it is not empty then
      builderLog "report", "Error building Windows engines:" && it
      throw "failure"
   else   
      builderLog "message", "Windows build result..."
      builderLog "message", URL ("file:" & engineBuilderLogFolder() & "/error_windows.log")
      builderLog "report", "Building Windows complete."
   end if
end engineBuilderBuildWindows

---------------------------------------------------------------------

private command engineBuilderBuildEnvironment pEdition, pVersion
   builderLog "report", "Configuring commercial environment stack..."
   
   builderLog "message", "Fetching public key..."
   local tPostDataArray
   put pVersion into tPostDataArray["version"]
   put "jobbies123!" into tPostDataArray["password"]
   
   local tPostData
   repeat for each line tKey in the keys of tPostDataArray
      put "&" & tKey & "=" & urlencode(tPostDataArray[tKey]) after tPostData
   end repeat
   delete the first char of tPostData
   
   local tResult
   post tPostData to URL "http://livecode.com/livecode/building/fetch_key"
   put the result into tResult
   if the result is not empty then
      builderLog "report", "Error fetching public key for version" && pVersion & ":" && tResult
      throw "failure"
   end if
   
   local tPubKey
   put line 2 to -1 of it into tPubKey
   if line 1 of it is not "success" then
      builderLog "report", "Error fetching public key for version" && pVersion & ":" && line 2 to -1 of it
      throw "failure"
   end if
   
   builderLog "message", "New public key for version" && pVersion & "..."
   builderLog "message", tPubKey
   
   builderLog "message", "Writing public key to stack..."
   local tKeyFile
   put the tempName into tKeyFile
   put tPubKey into URL ("binfile:" & tKeyFile)
   get shell(builderSystemFolder() & "/public_key_setter -ui" && builderCommercialFolder() & "/src/environment.livecode" && tKeyFile)
   builderLog "message", it
   delete file tKeyFile
   
   builderLog "report", "Commercial environment stack configured."
end engineBuilderBuildEnvironment

---------------------------------------------------------------------

command engineBuilderRun pPlatform, pEdition, pVersion
   builderEnsureFolder engineBuilderLogFolder()
   
   switch pPlatform
      case "environment"
         engineBuilderBuildEnvironment pEdition, pVersion
         break
      case "mac"
         engineBuilderBuildMac pEdition, pVersion
         break
      case "linux"
         engineBuilderBuildLinux pEdition, pVersion
         break
      case "windows"
         engineBuilderBuildWindows pEdition, pVersion
         break
      case "android"
         engineBuilderBuildAndroid pEdition, pVersion
         break
      case "ios"
         engineBuilderBuildiOS pEdition, pVersion
         break         
   end switch
end engineBuilderRun

---------------------------------------------------------------------
