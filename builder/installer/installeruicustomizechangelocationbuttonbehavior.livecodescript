script "InstallerInstallerUICustomizeChangeLocationButtonBehavior"
on mouseUp
   repeat forever
      answer folder "Please choose a folder to install into:"
      if it is not empty then
         put slash & thisProductFolder() after it
         if not locationIsSuitable(it)  then
            answer "The location you have chosen is not suitable for the installation. Please choose either an empty folder, or an existing revEnterprise installation folder." with "Okay" and "Cancel"
            if it is "cancel" then
               exit mouseUp
            end if
         else
            exit repeat
         end if
      else
         exit mouseUp
      end if
   end repeat
   if it is not empty then
      setCustomLocationSetting it
      synchronizeUI
   end if
end mouseUp

-- Make sure we block this to stop stack moving logic!
on mouseDown
end mouseDown
