script "InstallerUninstallerUIFinishCardBehavior"
on preOpenCard
   if the uError of me is empty then
      put "Uninstallation Complete" into field "Page Title"
      put "The uninstallation completed." into field "Page Description"
      hide control "Outcome"
      hide control "Cancel"
      hide control "Back"
      set the uEnabled of control "Next" to true
      set the uDefault of control "Next" to true
      set the uLabel of control "Next" to "Finish"
   else
      put "Uninstallation Failed" into field "Page Title"
      put "The uninstallation was not successful." into field "Page Description"
      put the uError of me into field "Outcome"
      show field "Outcome"
      
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
   if the uError of me is empty then
      if the environment is "installer" then
         lock messages
         quit
      end if
   else
      go card "Welcome"
   end if
end actionNext

on actionBack
   if the environment is "installer" then
      lock messages
      quit
   end if
end actionBack
