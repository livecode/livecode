script "InstallerInstallerUIWelcomeCardBehavior"
on preOpenCard
   setTitleField the long id of field "Page Title", the uProductTitle of stack "Installer" && "Installer"
   
   put the uProductDetails of stack "Installer" into field "Page Description"
   
   set the uEnabled of control "Back" to false
   set the uLabel of control "Back" to "Back"
   set the uEnabled of control "Next" to true
   set the uDefault of control "Next" to true
   set the uLabel of control "Next" to "Continue"
   set the uEnabled of control "Cancel" to true
   set the uLabel of control "Cancel" to "Cancel"
   show control "Next"
   show control "Cancel"
   show control "Back"
end preOpenCard
