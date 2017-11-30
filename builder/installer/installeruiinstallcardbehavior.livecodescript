script "InstallerInstallerUIInstallCardBehavior"
########################################################################

local sProgressPeak
local sProgressDirection
local sDidCancel

on preOpenCard
   setTitleField the long id of field "Page Title", "Installation in Progress", "LiveCode Installer"
   
   put empty into field "Page Description"
   
   set the uProgress of control "Progress" to 0
   
   put "Starting installation" into field "Phase"
   
   hide control "Next"
   hide control "Back"
   set the uEnabled of control "Cancel" to true
   set the uLabel of control "Cancel" to "Cancel"
end preOpenCard

on openCard
   local tLocation
   put settingsLocation() into tLocation
   
   local tOptions
   put settingsDesktopShortcut() into tOptions["-desktopshortcut"]
   put settingsStartMenuShortcuts() into tOptions["-startmenu"]
   if settingsTarget() is "all users" then
      put true into tOptions["-allusers"]
   end if
   
   put "up" into sProgressDirection
   put 0 into sProgressPeak
   
   try
      put false into sDidCancel
      installerRun tLocation, tLocation & slash & ".setup.txt", tOptions, not settingsRequireAdminPrivileges()
   catch tError
      installerFinished tError
   end try
end openCard

on actionCancel
   put true into sDidCancel
   installerCancel
end actionCancel

########################################################################

-- This event is dispatched by the back-end stack when the phase of the install process
-- changes.
on installerPhaseChanged pNewPhase
   switch pNewPhase
      case "Install"
         set the uEnabled of control "Cancel" to true
         put "up" into sProgressDirection
         put "Installing..." into field "Phase"
         break
      case "Uninstall"
         set the uEnabled of control "Cancel" to false
         put "up" into sProgressDirection
         put "Uninstalling existing version..." into field "Phase"
         break
      case "Cleanup"
         set the uEnabled of control "Cancel" to false
         put "down" into sProgressDirection
         put "Removing installed files..." into field "Phase"
         break
   end switch
end installerPhaseChanged

-- This event is dispatched by the back-end stack when it receives a progress update from
-- the child.
on installerProgressChanged pProgress, pMessage
   if pProgress > sProgressPeak then
      put pProgress into sProgressPeak
   end if
   
   if sProgressDirection is "down"  then
      put (100 - pProgress) * sProgressPeak / 100.0 into pProgress
   end if
   
   updateProgress pProgress, pMessage
end installerProgressChanged

-- This event is dispatched by the back-end stack when it recieves an error from the child.
on installerFinished pError
   if pError is not empty then
      set the uError of card "Finish" to pError
   else if sDidCancel then
      set the uError of card "Finish" to "Cancelled"
   else
      set the uError of card "Finish" to empty
   end if
   go card "Finish"
end installerFinished

########################################################################

local sLastUpdate

command updateProgress pNewPercentage, pMessage
   if sLastUpdate is not empty and (the millisecs - sLastUpdate < 25) then
      exit updateProgress
   end if 
   put the millisecs into sLastUpdate
   set the uProgress of control "Progress" of me to pNewPercentage
end updateProgress

########################################################################
