script "InstallerUninstallerUIStackBehavior"
######################################################################

on preOpenStack
   // PM-2016-02-27: [[ Bug 13723]] Use a card border 
   if the platform is "win32" or the platform contains "linux" then
      local tCards, tCurrentCard, tControlsA
      
      put the cardNames of stack "UninstallerUI" of stack "Installer" into tCards
      repeat with tCardIndex = 1 to the number of lines in tCards
         put line tCardIndex of tCards into tCurrentCard
         set the showBorder of card tCurrentCard of stack "UninstallerUI" of stack "Installer" to true
      end repeat
   end if
   
   -- Sort out fonts and sizes
   configureFonts the short name of me
   
   -- Make sure there's no error state
   set the uError of card "Finish" to empty
   
   set the loc of this stack to the screenLoc
   set the visible of this stack to true
   set the processType to "foreground"
   
   go card "Welcome"
end preOpenStack

-- Default handling for next/back/cancel buttons
on actionNext
   go next card
end actionNext

on actionBack
   go prev card
end actionBack

on actionCancel
   if the environment is "installer" then
      lock messages
      quit
   end if
end actionCancel

######################################################################

local sMovingOffset

on mouseDown
   put the clickLoc into sMovingOffset
end mouseDown

on mouseUp
   put empty into sMovingOffset
end mouseUp

on mouseRelease
   put empty into sMovingOffset
end mouseRelease

on mouseMove
   if sMovingOffset is not empty then
      get the globalLoc of the mouseLoc
      set the topLeft of me to item 1 of it - item 1 of sMovingOffset, item 2 of it - item 2 of sMovingOffset
   end if
end mouseMove

######################################################################

// SN-2015-0925: [[ Bug 11133 ]] Add runFaceless command
//  Copied and adapted from card "Uninstall", function openStack
command runFaceless
   local tLocation
   set the itemDelimiter to slash
   put item 1 to -2 of the effective filename of this stack into tLocation
   
   if there is no file (tLocation & slash & ".setup.txt") then
      write "Not a valid install of LiveCode." & LF & tLocation & slash & ".setup.txt cannot be found" & LF to stderr
      exit runFaceless
   end if
   
   // We are obviously not uninstalling a pre-installed version.
   actionUninstall tLocation, tLocation & slash & ".setup.txt", false
end runFaceless

function manifestNeedsElevation pManifest
   if the environment is not "installer" then
      return false
   end if
   
   set the itemDelimiter to tab
   repeat for each line tEntry in pManifest
      switch item 1 of tEntry
         case "folder"
            -- Need to work out what to do here
            break
         case "file"
            get item 2 of tEntry
            replace slash with backslash in it
            do "_internal canDeleteFile it; get the result"
            if not it then
               return true
            end if
            break
         case "registry key"
            get item 2 of tEntry
            replace "HKEY_CURRENT_USER" with "HKCU" in it
            do "_internal canDeleteKey it; get the result"
            if not it then
               return true
            end if
            break
      end switch
   end repeat
   
   return false
end manifestNeedsElevation
