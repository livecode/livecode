script "InstallerInstallerUITypeCardBehavior"
on preOpenCard
   setTitleField the long id of field "Page Title", "Select Installation Type", "LiveCode Installer"
   
   put "Please choose the type of installation you would like to perform." into field "Page Description"
   
   set the uType of control "All Users" to "allusers"
   set the uType of control "This User" to "youonly"
   set the uType of control "Custom" to "custom"
   
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

on onCheckChanged
   setInstallTypeSetting the short name of the target
   synchronizeUI
end onCheckChanged

command synchronizeUI
   repeat for each item tType in "all users,this user,custom"
      set the uCheck of control tType of group "Install Type" to settingsInstallType() is tType
   end repeat
end synchronizeUI

on actionNext
   switch settingsInstallType()
      case "all users"
      case "this user"
         go to card "Confirm"
         break
      case "custom"
         go to card "Customize"
         break
   end switch
end actionNext
