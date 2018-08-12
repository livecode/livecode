script "InstallerInstallerUICustomizeCardBehavior"
on preOpenCard
   setTitleField the long id of field "Page Title", "Customize", "LiveCode Installer"
   
   put "Please customize your installation of LiveCode." into field "Page Description"
   
   set the uLabel of control "All Users" to "Install for all users"
   set the uLabel of control "This User" to "Install for you only"
   set the uLabel of control "Desktop Shortcut" to "Create desktop shortcut"
   set the uLabel of control "Start Menu Shortcut" to "Create start menu shortcut"
   
   set the uEnabled of control "Back" to true
   set the uLabel of control "Back" to "Back"
   set the uEnabled of control "Next" to true
   set the uDefault of control "Next" to true
   set the uLabel of control "Next" to "Continue"
   set the uEnabled of control "Cancel" to true
   set the uLabel of control "Cancel" to "Cancel"
   
   show control "Cancel"
   show control "Back"
   
   synchronizeUI
end preOpenCard

command synchronizeUI
   repeat for each item tState in "all users,this user"
      set the uCheck of control tState to tState is settingsTarget()
   end repeat
   
   put settingsLocation() into field "Location"
   set the toolTip of field "Location" to settingsLocation()
   
   set the itemDelimiter to slash
   set the textColor of the item 1 to -1 of field "Location" to "#555555"
   set the textColor of the last item of field "Location" to "#569e54"
   
   set the uCheck of control "Start Menu Shortcut" to settingsStartMenuShortcuts()
   set the uCheck of control "Desktop Shortcut" to settingsDesktopShortcut()
   
   set the visible of control "Start Menu Shortcut" to the platform is not "macos"
   set the visible of control "Desktop Shortcut" to the platform is "win32"
end synchronizeUI

on onCheckChanged
   if the short name of the target is among the items of "All Users,This User" then
      setCustomTargetSetting the short name of the target
      synchronizeUI
   else if the short name of the target is "Desktop Shortcut" then
      setCustomDesktopShortcutSetting the uCheck of control "Desktop Shortcut"
   else if the short name of the target is "Start Menu Shortcut" then
      setCustomStartMenuShortcutsSetting the uCheck of control "Start Menu Shortcut"
   end if
end onCheckChanged
