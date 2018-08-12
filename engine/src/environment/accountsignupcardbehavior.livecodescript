script "EnvironmentAccountSignupCardBehavior"
on preOpenCard
   put empty into field "name"
   put empty into field "email"
   put empty into field "password"
   set the cPassword of field "password" to empty
   put empty into field "retype password"
   set the cPassword of field "retype password" to empty
   put empty into field "signup_message"
   hide field "signup_message"
   hide image "loader"
   selectIndicator "account"
end preOpenCard

on  accountSignup
   // Verify information
   local tName, tEmail, tPassword,tReType
   
   put field "name" into tName
   put field "email" into tEmail
   put the cPassword of field "password" into tPassword
   put the cPassword of field "retype password" into tReType
   
   if tName is empty then
      accountSignUpMessage "Please enter your name."
      exit accountSignup
   end if
   
   if tEmail is empty then
      accountSignUpMessage "Please enter your email address."
      exit accountSignup
   end if
   
   if tPassword is empty then
      accountSignUpMessage "Please enter a password."
      exit accountSignup
   end if
   
   if tReType is empty then
      accountSignUpMessage "Please retype the password."
      exit accountSignup
   end if
   
   if tPassword is not tReType then
      accountSignUpMessage "Please ensure your password and retyped password match."
      exit accountSignup
   end if
   
   accountCreate tName, tEmail, tPassword
end accountSignup

on openField
   accountSignupMessageClose
end openField

on focusIn
   accountSignupMessageClose
end focusIn


