script "EnvironmentResourcesResourcesPushButtonBehaviorButtonBehavior"
local sEnabled
local sDefault
local sInside
local sPressed

on mouseDown
   put true into sPressed
   updateChild
end mouseDown

on mouseUp
   put false into sPressed
   updateChild
   
   if sEnabled then
      dispatch "onClick" to me
   end if
end mouseUp

on mouseRelease
   put false into sPressed
   updateChild
end mouseRelease

on mouseEnter
   if sEnabled then
      put true into sInside
      updateChild
   end if
end mouseEnter

on mouseMove
   if sPressed then
      put the mouseLoc is within the rect of me into sInside
      updateChild
   end if
end mouseMove

on mouseLeave
   put false into sInside
   updateChild
end mouseLeave

on preOpenControl
   set the width of button 1 of me to 84
   set the height of button 1 of me to 24
   set the width of field 1 of me to 84
   set the height of field 1 of me to 24
   set the style of button 1 of me to "transparent"
   set the textColor of field 1 of me to "white"
   set the textAlign of field 1 of me to "center"
   if sEnabled is empty then
      put true into sEnabled
   end if
   if sDefault is empty then
      put false into sDefault
   end if
   put false into sInside
   put false into sPressed
   updateChild
end preOpenControl

setProp uEnabled pValue
   put pValue into sEnabled
   updateChild
end uEnabled

setProp uDefault pValue
   put pValue into sDefault
   updateChild
end uDefault

setProp uLabel pValue
   set the text of field 1 of me to pValue
end uLabel

private command updateChild
   if not sEnabled then
      get "button-disabled"
   else if sPressed then
      if sInside then
         get "button-pressed"
      else
         get "button-standard"
      end if
   else if sInside then
      get "button-hover"
   else if sDefault then
      get "button-default"
   else
      get "button-standard"
   end if
   set the icon of button 1 of me to resolveImageId(it)
   if sEnabled then
      set the textColor of field 1 of me to "#ffffff"
   else
      set the textColor of field 1 of me to "#474747"
   end if
end updateChild
