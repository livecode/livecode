script "InstallerUninstallerUIUninstallCardBehavior"
########################################################################

local sProgressPeak
local sProgressDirection

on preOpenCard
   setTitleField the long id of field "Page Title", "Uninstallation in Progress", "LiveCode Installer"
   
   put empty into field "Page Description"
   
   set the uProgress of control "Progress" to 0
   
   put "Starting uninstallation" into field "Phase"
   
   hide control "Next"
   hide control "Back"
   hide control "Cancel"
end preOpenCard

on openCard
   local tLocation
   set the itemDelimiter to slash
   put item 1 to -2 of the effective filename this stack into tLocation
   
   if there is no file (tLocation & slash & ".setup.txt") then
      set the uError of card "Finish" to "Not a valid install of LiveCode."
      go to card "Finish"
      exit openCard
   end if
   
   put 0 into sProgressPeak
   put "up" into sProgressDirection
   
   installerRunUninstall tLocation, tLocation & slash & ".setup.txt", not manifestNeedsElevation(url ("file:" & tLocation & slash & ".setup.txt"))
end openCard

########################################################################

-- This event is dispatched by the back-end stack when it receives a progress update from
-- the child.
on installerProgressChanged pProgress, pMessage
   if pProgress < sProgressPeak then
      put "down" into sProgressDirection
   else
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
   else
      set the uError of card "Finish" to empty
   end if
   go card "Finish"
end installerFinished

########################################################################

command updateProgress pNewPercentage, pMessage
   put "Uninstalling..." into field "Phase"
   set the uProgress of control "Progress" of me to pNewPercentage
end updateProgress

########################################################################
