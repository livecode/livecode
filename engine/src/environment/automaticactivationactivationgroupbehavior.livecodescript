script "EnvironmentAutomaticActivationActivationGroupBehavior"
local sPasswordIsHash
local sPassword

on preOpenBackground
   set the textFont of field "Error Message" to messageFont()
   set the textSize of field "Error Message" to messageFontSize()
   
   set the textColor of field "Pass Label" to headingColor()
   set the textColor of field "Pass" to subtitleColor()
   set the textColor of field "Login" to subtitleColor()
   set the textColor of field "Login Label" to headingColor()
   set the textColor of field "Error Message" to primaryErrorColor()
      
   repeat for each item tControl in "Login Label,Login,Pass Label,Pass"
      set the textFont of control tControl to entryFont()
      set the textSize of control tControl to entryFontSize()
      set the textStyle of control tControl to "bold"
   end repeat
   
   set the uLabel of control "Quit" to "Quit"
   set the uDefault of control "Quit" to false
   set the uEnabled of control "Quit" to true
   
   set the uLabel of control "Activate" to "Activate"
   set the uDefault of control "Activate" to true
   
   pass preOpenBackground
end preOpenBackground

################################################################################

command setLogin pEmail
   put pEmail into field "Login"
   updateActivateEnablement
end setLogin

command attemptActivation
   if field "Login" is empty or field "Pass" is empty then
      exit attemptActivation
   end if
   
   if sPasswordIsHash then
      performAutomaticActivation word 1 to -1 of field "Login", empty, word 1 to -1 of sPassword
   else
      performAutomaticActivation word 1 to -1 of field "Login", word 1 to -1 of sPassword, the md5Digest of word 1 to -1 of sPassword
   end if
   if the result is "error" then
      show group "Input"
      updateErrorMessage
   end if
end attemptActivation

command updateActivateEnablement
   set the uEnabled of control "Activate" to (word 1 to -1 of field "Login" is not empty and word 1 to -1 of field "Pass" is not empty)
end updateActivateEnablement

################################################################################

command updateErrorMessage
   local tMessage
   put activationError() into tMessage
   if tMessage is empty then
      put activationReason() into tMessage
   end if
   if tMessage is empty then
      hide group "Error"
      exit updateErrorMessage
   end if
   
   lock screen
   set the width of field "Error Message" to 1024
   
   put tMessage into field "Error Message"
   set the width of field "Error Message" to min(the formattedWidth of field "Error Message", the width of group "Error" - 20)
   set the height of field "Error Message" to the formattedHeight of field "Error Message"
   set the left of field "Error Message" to  the left of group "Error" + (the width of group "Error" - the width of field "Error Message") div 2 + 20
   set the loc of button "Error Symbol" to the loc of group "Error"
   set the left of button "Error Symbol" to the left of field "Error Message" - 20
   
   show group "Error"
   unlock screen
end updateErrorMessage

################################################################################

command setPassword pHash, pValue
   put pHash into sPasswordIsHash
   put pValue into sPassword
   updatePassword
end setPassword

command updatePassword
   lock screen
   if sPasswordIsHash then
      put "********" into field "Pass"
   else
      put sPassword into field "Pass"
   end if
   repeat with x = 1 to the number of chars of field "Pass"
      set the imageSource of char x of field "Pass" to 10025
   end repeat
   unlock screen
end updatePassword

on keyDown
   if the short name of the target is "Pass" then
      selectionChanged
   end if
   pass keyDown
end keyDown

on rawKeyUp
   if the short name of the target is "Pass" then
      selectionChanged
   end if
   updateActivateEnablement
end rawKeyUp

on selectionChanged
   if the short name of the target is "Pass" then
      if sPasswordIsHash and the text of field "Pass" is "********" then
         select char 1 to -1 of field "Pass"
      else
         get the selectedChunk
         put false into sPasswordIsHash
         put the text of field "Pass" into sPassword
         updatePassword
         select it
      end if
   end if
   updateActivateEnablement
end selectionChanged

################################################################################
