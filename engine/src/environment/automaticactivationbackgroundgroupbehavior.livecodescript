script "EnvironmentAutomaticActivationBackgroundGroupBehavior"
on preOpenBackground
   set the linkColor of this stack to livecodeColor()
   set the linkHiliteColor of this stack to livecodeLightColor()
   set the linkVisitedColor of this stack to livecodeColor()
   
   set the textFont of field "Title" to titleFont()
   set the textSize of field "Title" to titleFontSize()
   
   set the textColor of field "Title" to titleColor()
   set the textColor of field "Version" to livecodeColor()
   
   put the version into field "Version"
   
   pass preOpenBackground
end preOpenBackground
