script "InstallerInstallerUIWaitCardBehavior"
on preOpenCard
   setTitleField the long id of field "Page Title", "Conflicting Apps", "LiveCode Installer"
   set the uEnabled of control "Back" to true
   set the uLabel of control "Back" to "Back"
   set the uEnabled of control "Next" to false
   set the uLabel of control "Next" to "Continue"   
   set the uEnabled of control "Cancel" to true
   set the uLabel of control "Cancel" to "Cancel"   
   show control "Cancel"
   show control "Next"
   show control "Back"
   
   refreshAppsList
   pass preOpenCard
end preOpenCard

on closeCard
   if "refreshAppsList" is in the pendingMessages then
      cancel item 1 of line lineOffset("refreshAppsList", the pendingMessages) of the pendingMessages
   end if
end closeCard

-- We stick on the wait card until there are no more conflicts.
-- If there are no confilts, move on to installation.
--
on refreshAppsList
   local tConflicts
   put fetchConflictingApps() into tConflicts
   if tConflicts is empty then
      wait 100 millisecs
      go to card "Install"
      exit refreshAppsList
   end if   
   put "LiveCode cannot be installed due to conflicting apps.  This is most likely being caused by having a version of LiveCode already running in the install location.  To proceed, quit this version of LiveCode." & \
         return & return & "The installer will automatically continue when these conflicting applications are closed:" & \
         return & return & tab & tConflicts into field "Page Description"   
   send "refreshAppsList" to me in 2 seconds
end refreshAppsList

on actionBack
   if settingsInstallType() is "custom" then
      go to card "Customize"
   else
      go to card "Type"
   end if
end actionBack

-- Return a list of executables that conflict with the install.
--
private function fetchConflictingApps 
   local tInstallLocation
   put settingsLocation () into tInstallLocation
   switch the platform
      case "Win32"
         get fetchConflictingWindows(tInstallLocation)
         break
      case "MacOS"
         get fetchConflictingMac(tInstallLocation)
         break
      case "Linux"
         get fetchConflictingLinux(tInstallLocation)
         break
   end switch
   return it
end fetchConflictingApps

-- On OS X we use the nitfy lsof command which lists the executables running in a given folder
-- (and who is running them - though in this case not needed)
--
private function fetchConflictingMac pInstallPath
   local tProcessList
   put "/Contents/MacOS/" after pInstallPath
   put shell("lsof -Fpcn +D" && quote & pInstallPath & quote) into tProcessList
   
   local tProcesses, tFoundProcess
   if tProcessList is not empty then
      repeat for each line tLine in tProcessList
         if char 1 of tLine is "p" and tProcesses is not empty then
            put return after tProcesses
         end if
         put char 2 to -1 of tLine & tab after tProcesses
      end repeat
      delete char 1 of tProcesses
      delete char -1 of tProcesses
   end if
   
   set the itemDel to tab
   repeat for each line tProcess in tProcesses
      if item 2 of tProcess is "LiveCode" then
         return item 3 of tProcess
      end if
   end repeat
   return empty
end fetchConflictingMac

-- On Windows we use a custom installer command.
--
private function fetchConflictingWindows pInstallPath
   if the environment is "installer" then
      put "/LiveCode.exe" after pInstallPath
      replace slash with backslash in pInstallPath
      do "_internal listTasksWithModule pInstallPath"
      return item 1 of the result
   end if
end fetchConflictingWindows

-- On Linux, we parse ps.
-- This needs tweeking.  We return the last word which go a bit weird for path names with a space.
--
private function fetchConflictingLinux pInstallPath
   get shell ("ps aux | egrep '" & pInstallPath & "' | grep -v 'grep'")
   if it is not empty then
      return the last word of it
   end if
   return empty
end fetchConflictingLinux
