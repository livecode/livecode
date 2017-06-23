script "InstallerInstallerUIConfirmCardBehavior"
on preOpenCard
   setTitleField the long id of field "Page Title", "Ready to Install", "LiveCode Installer"
   
   local tDetails
   put "THIS INSTALLATION WILL:" & return after tDetails
   put return after tDetails
   put "Install LiveCode to:" && settingsLocation() & return after tDetails
   put return after tDetails
   
   local tScope
   if settingsTarget() is "all users" then
      put "for all users" into tScope
   else
      put "for this user" into tScope
   end if
   
   if the platform is not "macos" then
      if the platform is not "linux" then
         if settingsDesktopShortcut() then
            put "Create a desktop shortcut" && tScope & "." & return after tDetails
         end if
      end if
      
      if settingsStartMenuShortcuts() then
         put "Create start menu shortcuts" && tScope & "." & return after tDetails
      end if
      
      put return after tDetails
   end if
   
   if locationIsExistingInstall(settingsLocation()) then
      put "The previous installation of LiveCode at this location will be removed before continuing." & return after tDetails
      put return after tDetails
   end if
   
   if settingsRequireAdminPrivileges() then
      put "Administrator privileges will be required to perform the installation." & return after tDetails
   end if
   
   put tDetails into field "Page Description"
   set the textColor of char 1 to -1 of field "Page Description" to "#555555"
   
   set the uEnabled of control "Back" to true
   set the uLabel of control "Back" to "Back"
   set the uEnabled of control "Next" to true
   set the uLabel of control "Next" to "Install"
   set the uDefault of control "Next" to true
   set the uEnabled of control "Cancel" to true
   set the uLabel of control "Cancel" to "Cancel"
   
   show control "Cancel"
   show control "Back"
end preOpenCard

on actionBack
   if settingsInstallType() is "custom" then
      go to card "Customize"
   else
      go to card "Type"
   end if
end actionBack
