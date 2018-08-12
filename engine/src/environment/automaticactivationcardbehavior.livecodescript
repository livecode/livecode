script "EnvironmentAutomaticActivationCardBehavior"
on preOpenCard
   put "Activation" into field "Title"
   put "Email" into field "Login Label"
   put "Password" into field "Pass Label"
   
   set the lockText of field "Login" to false   
   set the visible of group "Error" to false
   show group "Input"
   
   local tLogin
   put activationLogin() into tLogin
   if tLogin is not an array then
      setLogin empty
      setPassword false, empty
   else
      setLogin tLogin["email"]
      if tLogin["hash"] is not empty then
         setPassword true, tLogin["hash"]
      else
         setPassword false, tLogin["password"]
      end if
   end if
   
   updateActivateEnablement
   
   updateErrorMessage
end preOpenCard
