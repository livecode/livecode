script "EnvironmentUnlicensedBkgndBehavior"
command setMessage pPrimary, pSecondary
   set the textFont of field "Message" to messageFont()
   set the textSize of field "Message" to messageFontSize()
   put pPrimary & return & pSecondary into field "Message"
   set the textColor of line 1 of field "Message" to primaryErrorColor()
   set the textColor of line 2 of field "Message" to secondaryErrorColor()
   
   set the width of field "Message" to the formattedWidth of field "Message"
   
   set the left of field "Message" to (the width of this card - the width of field "Message") div 2 + 8
   set the loc of button "Alert" to the left of field "Message" - 8, the item 2 of the loc of button "Alert"
end setMessage

command setActivate pQuestion, pAction
   if pQuestion is empty then
      hide field "Activate"
      hide button "Underlay"
      show button "Underlay Full"
   else
      show field "Activate"
      show button "Underlay"
      hide button "Underlay Full"
   end if
   put pQuestion && pAction into field "Activate"
   set the linkText of word the number of words of pQuestion + 1 to -1 of field "Activate" to "activate"
   set the textStyle of word the number of words of pQuestion + 1 to -1 of field "Activate" to "link"
end setActivate
