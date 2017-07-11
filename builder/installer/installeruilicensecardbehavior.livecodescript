script "InstallerInstallerUILicenseCardBehavior"
on preOpenCard
   setTitleField the long id of field "Page Title", "License Agreement", "LiveCode Installer"
   put empty into field "Page Description"
   
   set the uEnabled of control "Back" to true
   set the uLabel of control "Back" to "Back"
   set the uEnabled of control "Next" to false
   set the uDefault of control "Next" to true
   set the uLabel of control "Next" to "Continue"
   set the uEnabled of control "Cancel" to true
   set the uLabel of control "Cancel" to "Cancel"
   
   show control "Cancel"
   show control "Back"
   
   set the uLabel of control "Accept" to "I accept the terms and conditions of this license"
   set the uCheck of control "Accept" to false
   
   --set the uText of control "License Agreement" to the uProductLicense of stack "Installer"
   set the text of field "License" to the uProductLicense of stack "Installer"   
end preOpenCard

on onCheckChanged
   set the uEnabled of control "Next" to the uCheck of control "Accept"
end onCheckChanged
