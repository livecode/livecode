script "InstallerInstallerUIFinishCardBehavior"
on preOpenCard
   if the uError of me is empty then
      setTitleField the long id of field "Page Title", "Installation Complete", "LiveCode Installer"
      put "Installation complete." into field "Page Description"
      
      -- This command returns the effective user ID (EUID)
      -- If running as root or via su/sudo, the EUID is 0
      local t_uid
      if the platform is "Linux" then
         put shell("id -u") into t_uid
      else
         put 1 into t_uid
      end if
      
      -- Running LiveCode as root can have undesired consequences
      -- e.g. wrong ownership of the ~/.runrev folder and contents
      if t_uid is 0 then
         hide control "Launch LiveCode"
         set the uCheck of control "Launch LiveCode" to false
      else
         show control "Launch LiveCode"
         set the uLabel of control "Launch LiveCode" to "Launch LiveCode"
         set the uCheck of control "Launch LiveCode" to true
      end if
      
      hide control "Display Release Notes"
      --set the uLabel of control "Display Release Notes" to "Display Release Notes"
      
      hide control "Outcome"
      hide control "Cancel"
      hide control "Back"
      set the uEnabled of control "Next" to true
      set the uDefault of control "Next" to true
      set the uLabel of control "Next" to "Finish"
   else
      if the uError of me is "Cancelled" then
         setTitleField the long id of field "Page Title", "Installation Cancelled", "LiveCode Installer"
         put "The installation was cancelled." into field "Page Description"
         hide control "Outcome"
      else
         setTitleField the long id of field "Page Title", "Installation Failed", "LiveCode Installer"
         put "The installation was not successful." into field "Page Description"
         put the uError of me into field "Outcome"
         show control "Outcome"
      end if
      
      hide control "Launch LiveCode"
      hide control "Display Release Notes"
      
      hide control "Cancel"
      set the uEnabled of control "Back" to true
      set the uLabel of control "Back" to "Quit"
      set the uDefault of control "Next" to true
      set the uEnabled of control "Next" to true
      set the uLabel of control "Next" to "Retry"
      show control "Back"
   end if
   show control "Next"
end preOpenCard

on actionNext
   local tPath
   if the uError of me is empty then
      if the uCheck of control "Launch LiveCode" then
         if the platform is "macos" then
            launch settingsLocation()
         else if the platform is "win32" then
            --launch settingsLocation() & slash & "LiveCode.exe"
            launch settingsLocation() & slash & the uProductTitle of stack "Installer" & ".exe"
         else -- if the platform is "linux"
            get the uProduct of stack "Installer"
            set the itemdelimiter to "."
            if item 1 of the version < 7 then
               replace space with empty in it
               get toLower(it)
               if the processor is "x86" then
                  put settingsLocation() & slash & it & ".i386" into tPath
               else
                  put settingsLocation() & slash & it & "." & the processor into tPath
               end if
            else
               // From 7.1, the executable  name is no longer lowercase, whitespace-free
               put settingsLocation() & slash & it & "." & the processor into tPath
            end if
            launch quote&tPath&quote
         end if
      end if
      quitInstaller
   else
      go card "Confirm"
   end if
end actionNext

on actionBack
   quitInstaller
end actionBack
