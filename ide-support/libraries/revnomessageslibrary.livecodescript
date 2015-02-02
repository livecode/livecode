script "revnomessageslibrary"

on revLoadLibrary
end revLoadLibrary

on revUnloadLibrary
end revUnloadLibrary

--Bug 5765:  Added try/catches to handle errors.
# OK-2008-03-13 : Bug 6009 and 6007, Added pass to metacard to mouseDown and all key messages. Added explicit
# target check to mouseDown and menuPick to ensure menu shortcuts work correctly.

global gRevDevelopment

on mouseEnter
  if revOKTarget() then pass mouseEnter
  try
    if "mouseEnter" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "mouseEnter" to btn "revFrontScript" of stack "revLibrary"
    if "mouseEnter" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "mouseEnter" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseEnter

on mouseLeave
  if revOKTarget() then pass mouseLeave
  try
    if "mouseLeave" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "mouseLeave" to btn "revFrontScript" of stack "revLibrary"
    if "mouseLeave" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "mouseLeave" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseLeave

on mouseDown pButtonNumber
  # OK-2008-03-13 : Bug 6009, Mousedown is sent by the engine directly to menus if their associated keyboard shortcut is pressed.
  # In this situation, revOKTarget must be explictly passed the target of the message as it won't always be correct.
  local tTarget
  put the long id of the target into tTarget
  if revOKTarget(tTarget) then pass mouseDown
  try
    # OK-2008-03-27 : the mouseDown handler in revFrontscript needs to know the target of the message
    revIDEHandleMouseDown pButtonNumber, tTarget
    #if "mouseDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "mouseDown" to btn "revFrontScript" of stack "revLibrary"
    if "mouseDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "mouseDown" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  
  # OK-2008-03-06 : Bug 6007 and 6009, Exit here to allow engine to handle the mouseDown as normal
  pass mouseDown to metacard
end mouseDown

on mouseUp pButtonNo
  if revOKTarget() then pass mouseUp
  try
    # OK-2008-03-27 : Bug where right-click context menus would not work with messages suppressed. Fixed by
    # providing a way of explicitly passing the target to revFrontScript.
    local tTarget
    put the long id of the target into tTarget
    revIDEHandleMouseUp pButtonNo, tTarget
    #if "mouseUp" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "mouseUp pButtonNo" to btn "revFrontScript" of stack "revLibrary"
    if "mouseUp" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "mouseUp pButtonNo" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseUp

on mouseWithin
  if revOKTarget() then pass mouseWithin
  try
    if "mouseWithin" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "mouseWithin" to btn "revFrontScript" of stack "revLibrary"
    if "mouseWithin" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "mouseWithin" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseWithin

on mouseStillDown pButtonNo
  if revOKTarget() then pass mouseStillDown
  try
    if "mouseStillDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "mouseStillDown pButtonNo" to btn "revFrontScript" of stack "revLibrary"
    if "mouseStillDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "mouseStillDown pButtonNo" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseStillDown

on mouseMove x,y
  if revOKTarget() then pass mouseMove
  try
    if "mouseMove" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "mouseMove x,y" to btn "revFrontScript" of stack "revLibrary"
    if "mouseMove" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "mouseMove x,y" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseMove

on mouseRelease pButtonNo
  if revOKTarget() then pass mouseRelease
  try
    if "mouseRelease" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "mouseRelease pButtonNo" to btn "revFrontScript" of stack "revLibrary"
    if "mouseRelease" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "mouseRelease pButtonNo" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseRelease

on mouseDoubleDown pButtonNo
  if revOKTarget() then pass mouseDoubleDown
  try
    if "mouseDoubleDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "mouseDoubleDown pButtonNo" to btn "revFrontScript" of stack "revLibrary"
    if "mouseDoubleDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "mouseDoubleDown pButtonNo" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseDoubleDown

on mouseDoubleUp pButtonNo
  if revOKTarget() then pass mouseDoubleUp
  try
    local tTarget
    put the long id of the target into tTarget
    if "mouseDoubleUp" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "mouseDoubleUp pButtonNo, tTarget" to btn "revFrontScript" of stack "revLibrary"
    if "mouseDoubleUp" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "mouseDoubleUp pButtonNo, tTarget" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseDoubleUp

on openField
  if revOKTarget() then pass openField
  try
    if "openField" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "openField" to btn "revFrontScript" of stack "revLibrary"
    if "openField" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "openField" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end openField

on closeField
  if revOKTarget() then pass closeField
  try
    if "closeField" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "closeField" to btn "revFrontScript" of stack "revLibrary"
    if "closeField" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "closeField" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end closeField

on exitField
  if revOKTarget() then pass exitField
  try
    if "exitField" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send exitField to btn "revFrontScript" of stack "revLibrary"
    if "exitField" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send exitField to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end exitField

on keyDown pKey
  if revOKTarget() then pass keyDown
  try
    if "keyDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "keyDown pKey" to btn "revFrontScript" of stack "revLibrary"
    if "keyDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "keyDown pKey" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  pass keyDown to metaCard
end keyDown

on keyUp pKey
  if revOKTarget() then pass keyUp
  try
    if "keyUp" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "keyUp pKey" to btn "revFrontScript" of stack "revLibrary"
    if "keyUp" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "keyUp pKey" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end keyUp

on rawKeyDown pKeyNo
  if revOKTarget() then pass rawKeyDown
  try
    if "rawKeyDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "rawKeyDown pKeyNo" to btn "revFrontScript" of stack "revLibrary"
    if "rawKeyDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "rawKeyDown pKeyNo" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  pass rawKeyDown to metaCard
end rawKeyDown

on rawKeyUp pKeyNo
  if revOKTarget() then pass rawKeyUp
  try
    if "rawKeyUp" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "rawKeyUp pKeyNo" to btn "revFrontScript" of stack "revLibrary"
    if "rawKeyUp" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "rawKeyUp pKeyNo" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end rawKeyUp

on commandKeyDown pWhich
  if revOKTarget() then pass commandKeyDown
  try
    if "commandKeyDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revShortCutsLibrary" then send "commandKeyDown pWhich" to stack "revShortCutsLibrary"
    if "commandKeyDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "commandKeyDown pWhich" to btn "revFrontScript" of stack "revLibrary"
    if "commandKeyDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "commandKeyDown pWhich" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  
  pass commandKeyDown to metacard
end commandKeyDown

on arrowKey pKey
   if revOKTarget() then pass arrowKey
   try
      if "arrowKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revShortCutsLibrary" then send "arrowKey pKey, the long id of the target" to stack "revShortCutsLibrary"
      if "arrowKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "arrowKey pKey" to btn "revFrontScript" of stack "revLibrary"
      if "arrowKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "arrowKey pKey" to btn "revBackScript" of stack "revLibrary"
   catch tError
      if gREVDevelopment then
         writeError (tError)
      end if
   end try
   pass arrowKey to metaCard
end arrowKey

on functionKey pKey
  if revOKTarget() then pass functionKey
  try
    if "functionKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "functionKey pKey" to btn "revFrontScript" of stack "revLibrary"
    if "functionKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "functionKey pKey" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
   pass functionKey to metaCard
end functionKey

on pasteKey
  if revOKTarget() then pass pasteKey
  try
    if "pasteKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send pasteKey to btn "revFrontScript" of stack "revLibrary"
    if "pasteKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send pasteKey to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
   pass pasteKey to metaCard
end pasteKey

on cutKey
  if revOKTarget() then pass cutKey
  try
    if "cutKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send cutKey to btn "revFrontScript" of stack "revLibrary"
    if "cutKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send cutKey to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  pass cutKey to metaCard
end cutKey

on copyKey
  if revOKTarget() then pass copyKey
  try
    if "copyKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send copyKey to btn "revFrontScript" of stack "revLibrary"
    if "copyKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send copyKey to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
   pass copyKey to metaCard
end copyKey

on backSpaceKey
  if revOKTarget() then pass backSpaceKey
  try
    if "backSpaceKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send backSpaceKey to btn "revFrontScript" of stack "revLibrary"
    if "backSpaceKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send backSpaceKey to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  pass backspaceKey to metaCard
end backSpaceKey

on deleteKey
  if revOKTarget() then pass deleteKey
  try
    if "deleteKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send deleteKey to btn "revFrontScript" of stack "revLibrary"
    if "deleteKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send deleteKey to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  pass deleteKey to metaCard
end deleteKey

on returnInField
  if revOKTarget() then pass returnInField
  try
    if "returnInField" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send returnInField to btn "revFrontScript" of stack "revLibrary"
    if "returnInField" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send returnInField to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  
  pass returnInField to metacard
end returnInField

on enterInField
  if revOKTarget() then pass enterInField
  try
    if "enterInField" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send enterInField to btn "revFrontScript" of stack "revLibrary"
    if "enterInField" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send enterInField to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  
  pass enterInField to metacard
end enterInField

on returnKey
  if revOKTarget() then pass returnKey
  try
    if "returnKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send returnKey to btn "revFrontScript" of stack "revLibrary"
    if "returnKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send returnKey to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  
  pass returnKey to metacard
end returnKey

on enterKey
  if revOKTarget() then pass enterKey
  try
    if "enterKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send enterKey to btn "revFrontScript" of stack "revLibrary"
    if "enterKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send enterKey to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  
  pass enterKey to metacard
end enterKey

on idle
  if revOKTarget() then pass idle
  try
    if "idle" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send idle to btn "revFrontScript" of stack "revLibrary"
    if "idle" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send idle to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end idle

on controlKeyDown pKey
  if revOKTarget() then pass controlKeyDown
  try
    if "controlKeyDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "controlKeyDown pKey" to btn "revFrontScript" of stack "revLibrary"
    if "controlKeyDown" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "controlKeyDown pKey" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  
  pass controlKeyDown to metacard
end controlKeyDown

on focusIn
  if revOKTarget() then pass focusIn
  try
    if "focusIn" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send focusIn to btn "revFrontScript" of stack "revLibrary"
    if "focusIn" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send focusIn to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end focusIn

on focusOut
  if revOKTarget() then pass focusOut
  try
    if "focusOut" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send focusOut to btn "revFrontScript" of stack "revLibrary"
    if "focusOut" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send focusOut to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end focusOut

on scrollBarDrag pPosition
  if revOKTarget() then pass scrollBarDrag
  try
    if "scrollBarDrag" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "scrollBarDrag pPosition" to btn "revFrontScript" of stack "revLibrary"
    if "scrollBarDrag" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "scrollBarDrag pPosition" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scrollBarDrag

on preOpenCard
  if revOKTarget() then pass preOpenCard
  try
    if "preOpenCard" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send preOpenCard to btn "revFrontScript" of stack "revLibrary"
    if "preOpenCard" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send preOpenCard to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end preOpenCard

on openCard
  if revOKTarget() then pass openCard
  try
    if "openCard" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send openCard to btn "revFrontScript" of stack "revLibrary"
    if "openCard" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send openCard to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end openCard

on preOpenStack
  if revOKTarget() then pass preOpenStack
  try
    # OK-2008-03-27 : Bug 6229. Application browser was failing to update with messages locked because it was using "the target" to determine which
    # stack had been opened. Changed so that the target can be forwarded to revFrontScript.
    #if "preOpenStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send preOpenStack to btn "revFrontScript" of stack "revLibrary"
    revIDEHandleNewStack the long id of the target
    if "preOpenStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send preOpenStack to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end preOpenStack

on openStack
  if revOKTarget() then pass openStack
  try
    if "openStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send openStack to btn "revFrontScript" of stack "revLibrary"
    if "openStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send openStack to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end openStack

on startUp
  if revOKTarget() then pass startUp
  try
    if "startUp" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send startUp to btn "revFrontScript" of stack "revLibrary"
    if "startUp" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send startUp to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end startUp

on closeStack
  if revOKTarget() then pass closeStack
  try
    if "closeStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send closeStack to btn "revFrontScript" of stack "revLibrary"
    if "closeStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send closeStack to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      --write tError & return to stderr
      writeError (tError)
    end if
  end try
end closeStack

# TH-2008-06-18 bug 6217, placed here from revfronscript.
# MW-2008-09-05: [[ Bug 6217 ]] Removed the close stack at the end of the handler
#   since this is done by the call to revIDEHandleCloseStackRequest
on closeStackRequest
  if revOKTarget() then
    pass closeStackRequest
  end if
   
  local tTarget
  put the long id of the target into tTarget
  try
    revIDEHandleCloseStackRequest tTarget
    --if "closeStackRequest" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send closeStackRequest to btn "revFrontScript" of stack "revLibrary"
    --if "closeStackRequest" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send closeStackRequest to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gRevDevelopment then
      writeError (tError)
    end if
 end try
end closeStackRequest

on quitMC
  if revOKTarget() then pass quitMC
  try
    if "quitMC" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send quitMC to btn "revFrontScript" of stack "revLibrary"
    if "quitMC" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send quitMC to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end quitMC

on playStopped
  if revOKTarget() then pass playStopped
  try
    if "playStopped" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send playStopped to btn "revFrontScript" of stack "revLibrary"
    if "playStopped" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send playStopped to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end playStopped

on moveStopped
  if revOKTarget() then pass moveStopped
  try
    if "moveStopped" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send moveStopped to btn "revFrontScript" of stack "revLibrary"
    if "moveStopped" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send moveStopped to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end moveStopped

on nameChanged pOldName, pNewName
  if revOKTarget() then pass nameChanged
  try
    put the long id of the target into tTarget
    if "nameChanged" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "nameChanged pOldName, pNewName, tTarget" to btn "revFrontScript" of stack "revLibrary"
    if "nameChanged" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "nameChanged pOldName, pNewName, tTarget" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end nameChanged

on idChanged pOldID, pNewID
  if revOKTarget() then pass idChanged
  try
    if "idChanged" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "idChanged pOldID, pNewID" to btn "revFrontScript" of stack "revLibrary"
    if "idChanged" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "idChanged pOldID, pNewID" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end idChanged

on saveRequest
  if revOKTarget() then pass saveRequest
  try
    if "saveRequest" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send saveRequest to btn "revFrontScript" of stack "revLibrary"
    if "saveRequest" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send saveRequest to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  pass saveRequest
end saveRequest

on moveControl
  if revOKTarget() then pass moveControl
  try
    if "moveControl" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send moveControl to btn "revFrontScript" of stack "revLibrary"
    if "moveControl" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send moveControl to btn "revBackScript" of stack "revLibrary"
    if "moveControl" is among the lines of the cREVGeneral["handlerlist"] of btn "revGeometryBack" of stack "revLibrary" then send moveControl to btn "revGeometryBack" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end moveControl

on resizeControl
  if revOKTarget() then pass resizeControl
  try
    if "resizeControl" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send resizeControl to btn "revFrontScript" of stack "revLibrary"
    if "resizeControl" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send resizeControl to btn "revBackScript" of stack "revLibrary"
    if "resizeControl" is among the lines of the cREVGeneral["handlerlist"] of btn "revGeometryBack" of stack "revLibrary" then send resizeControl to btn "revGeometryBack" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end resizeControl

on menuPick pWhich, pLast
  # OK-2008-03-13 : Bug 6009, Menupick is sent by the engine directly to menus if their associated keyboard shortcut is pressed.
  # In this situation, revOKTarget must be explictly passed the target of the message as it could be wrong.
  local tTarget
  put the long id of the target into tTarget
  if revOKTarget(tTarget) then pass menuPick
  try
    if "menuPick" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "menuPick pWhich, pLast" to btn "revFrontScript" of stack "revLibrary"
    if "menuPick" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "menuPick pWhich, pLast" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end menuPick

on selectionChanged
  if revOKTarget() then pass selectionChanged
  try
    if "selectionChanged" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send selectionChanged to btn "revFrontScript" of stack "revLibrary"
    if "selectionChanged" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send selectionChanged to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end selectionChanged

on selectedObjectChanged
  if revOKTarget() then pass selectedObjectChanged
  try
    if "selectedObjectChanged" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send selectedObjectChanged to btn "revFrontScript" of stack "revLibrary"
    if "selectedObjectChanged" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send selectedObjectChanged to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end selectedObjectChanged

on resizeStack pNewWidth, pNewHeight, pOldWidth, pOldHeight
  if revOKTarget() then pass resizeStack
  try
    if "resizeStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "resizeStack pNewWidth, pNewHeight, pOldWidth, pOldHeight" to btn "revFrontScript" of stack "revLibrary"
    if "resizeStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "resizeStack pNewWidth, pNewHeight, pOldWidth, pOldHeight" to btn "revBackScript" of stack "revLibrary"
    if "resizeStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revGeometryBack" of stack "revLibrary" then send "resizeStack pNewWidth, pNewHeight, pOldWidth, pOldHeight" to btn "revGeometryBack" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end resizeStack

on moveStack x,y
  if revOKTarget() then pass moveStack
  try
    if "moveStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "moveStack x,y" to btn "revFrontScript" of stack "revLibrary"
    if "moveStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "moveStack x,y" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end moveStack

on closeCard
  if revOKTarget() then pass closeCard
  try
    if "closeCard" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send closeCard to btn "revFrontScript" of stack "revLibrary"
    if "closeCard" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send closeCard to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end closeCard

on resumeStack
  if revOKTarget() then pass resumeStack
  try
    if "resumeStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send resumeStack to btn "revFrontScript" of stack "revLibrary"
    if "resumeStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send resumeStack to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end resumeStack

on suspendStack
  if revOKTarget() then pass suspendStack
  try
    if "suspendStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "revIDESuspendStackFront" to button "revFrontScript" of stack "revLibrary"
    if "suspendStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send suspendStack to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end suspendStack

on iconifyStack
  if revOKTarget() then pass iconifyStack
  try
    if "iconifyStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send iconifyStack to btn "revFrontScript" of stack "revLibrary"
    if "iconifyStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send iconifyStack to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end iconifyStack

on appleEvent pClass, pID, pSender
  if revOKTarget() then pass appleEvent
  try
    if "appleEvent" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "appleEvent pClass, pID, pSender" to btn "revFrontScript" of stack "revLibrary"
    if "appleEvent" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "appleEvent pClass, pID, pSender" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end appleEvent

on closeBackground pID
  if revOKTarget() then pass closeBackground
  try
    if "closeBackground" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "closeBackground pID" to btn "revFrontScript" of stack "revLibrary"
    if "closeBackground" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "closeBackground pID" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end closeBackground

on currentTimeChanged pTime
  if revOKTarget() then pass currentTimeChanged
  try
    if "currentTimeChanged" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "currentTimeChanged pTime" to btn "revFrontScript" of stack "revLibrary"
    if "currentTimeChanged" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "currentTimeChanged pTime" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end currentTimeChanged

on cutKey
  if revOKTarget() then pass cutKey
  try
    if "cutKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send cutKey to btn "revFrontScript" of stack "revLibrary"
    if "cutKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send cutKey to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  
  pass cutKey to metacard
end cutKey

on deletebackground
  if revOKTarget() then pass deletebackground
  try
    if "deletebackground" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send deletebackground to btn "revFrontScript" of stack "revLibrary"
    if "deletebackground" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send deletebackground to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deletebackground

on deleteButton
  if revOKTarget() then pass deleteButton
  try
    if "deleteButton" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send deleteButton to btn "revFrontScript" of stack "revLibrary"
    if "deleteButton" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send deleteButton to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deleteButton

on deletecard
  if revOKTarget() then pass deletecard
  try
    if "deletecard" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send deletecard to btn "revFrontScript" of stack "revLibrary"
    if "deletecard" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send deletecard to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deletecard

on deleteEPS
  if revOKTarget() then pass deleteEPS
  try
    if "deleteEPS" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send deleteEPS to btn "revFrontScript" of stack "revLibrary"
    if "deleteEPS" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send deleteEPS to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deleteEPS

on deleteField
  if revOKTarget() then pass deleteField
  try
    if "deleteField" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send deleteField to btn "revFrontScript" of stack "revLibrary"
    if "deleteField" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send deleteField to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deleteField

on deleteGraphic
  if revOKTarget() then pass deleteGraphic
  try
    if "deleteGraphic" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send deleteGraphic to btn "revFrontScript" of stack "revLibrary"
    if "deleteGraphic" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send deleteGraphic to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deleteGraphic

on deleteImage
  if revOKTarget() then pass deleteImage
  try
    if "deleteImage" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send deleteImage to btn "revFrontScript" of stack "revLibrary"
    if "deleteImage" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send deleteImage to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deleteImage

on deletePlayer
  if revOKTarget() then pass deletePlayer
  try
    if "deletePlayer" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send deletePlayer to btn "revFrontScript" of stack "revLibrary"
    if "deletePlayer" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send deletePlayer to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deletePlayer

on dragMove
  if revOKTarget() then pass dragMove
  try
    if "dragMove" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send dragMove to btn "revFrontScript" of stack "revLibrary"
    if "dragMove" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send dragMove to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end dragMove

on dragEnter
  if revOKTarget() then pass dragEnter
  try
    if "dragEnter" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send dragEnter to btn "revFrontScript" of stack "revLibrary"
    if "dragEnter" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send dragEnter to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end dragEnter

on errorDialog pError
  if revOKTarget() then pass errorDialog
  try
    if "errorDialog" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "errorDialog pError" to btn "revFrontScript" of stack "revLibrary"
    if "errorDialog" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "errorDialog pError" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end errorDialog

on escapeKey
  if revOKTarget() then pass escapeKey
  try
    if "escapeKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send escapeKey to btn "revFrontScript" of stack "revLibrary"
    if "escapeKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send escapeKey to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end escapeKey

on help
  if revOKTarget() then pass help
  try
    if "help" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send help to btn "revFrontScript" of stack "revLibrary"
    if "help" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send help to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end help

on libraryStack
  if revOKTarget() then pass libraryStack
  try
    if "libraryStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send libraryStack to btn "revFrontScript" of stack "revLibrary"
    if "libraryStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send libraryStack to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end libraryStack

on newBackground
  if revOKTarget() then pass newBackground
  try
    if "newBackground" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send newBackground to btn "revFrontScript" of stack "revLibrary"
    if "newBackground" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send newBackground to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newBackground

on NewButton
  if revOKTarget() then pass NewButton
  try
    if "NewButton" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send NewButton to btn "revFrontScript" of stack "revLibrary"
    if "NewButton" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send NewButton to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end NewButton

on newPlayer
  if revOKTarget() then pass newPlayer
  try
    if "newPlayer" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send newPlayer to btn "revFrontScript" of stack "revLibrary"
    if "newPlayer" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send newPlayer to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newPlayer

on newCard
  if revOKTarget() then pass newCard
  try
    if "newCard" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send newCard to btn "revFrontScript" of stack "revLibrary"
    if "newCard" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send newCard to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newCard

on newEPS
  if revOKTarget() then pass newEPS
  try
    if "newEPS" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send newEPS to btn "revFrontScript" of stack "revLibrary"
    if "newEPS" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send newEPS to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newEPS

on newField
  if revOKTarget() then pass newField
  try
    if "newField" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send newField to btn "revFrontScript" of stack "revLibrary"
    if "newField" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send newField to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newField

on newGraphic
  if revOKTarget() then pass newGraphic
  try
    if "newGraphic" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send newGraphic to btn "revFrontScript" of stack "revLibrary"
    if "newGraphic" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send newGraphic to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newGraphic

on newGroup
  if revOKTarget() then pass newGroup
  try
    if "newGroup" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send newGroup to btn "revFrontScript" of stack "revLibrary"
    if "newGroup" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send newGroup to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newGroup

on newImage
  if revOKTarget() then pass newImage
  try
    if "newImage" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send newImage to btn "revFrontScript" of stack "revLibrary"
    if "newImage" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send newImage to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newImage

on newScrollbar
  if revOKTarget() then pass newScrollbar
  try
    if "newScrollbar" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send newScrollbar to btn "revFrontScript" of stack "revLibrary"
    if "newScrollbar" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send newScrollbar to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newScrollbar

on newStack
  if revOKTarget() then pass newStack
  try
    if "newStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send newStack to btn "revFrontScript" of stack "revLibrary"
    if "newStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send newStack to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gRevDevelopment then
      writeError (tError)
    end if
  end try
end newStack

on newTool pTool
  if revOKTarget() then pass newTool
  try
    if "newTool" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "newTool pTool" to btn "revFrontScript" of stack "revLibrary"
    if "newTool" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "newTool pTool" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newTool

on openBackground pID
  if revOKTarget() then pass openBackground
  try
    if "openBackground" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "openBackground pID" to btn "revFrontScript" of stack "revLibrary"
    if "openBackground" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "openBackground pID" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end openBackground

on preOpenbackground pID
  if revOKTarget() then pass preOpenbackground
  try
    if "preOpenbackground" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "preOpenbackground pID" to btn "revFrontScript" of stack "revLibrary"
    if "preOpenbackground" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "preOpenbackground pID" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end preOpenbackground

on reloadStack pStack, pFile
  if revOKTarget() then pass reloadStack
  try
    if "reloadStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "reloadStack pStack, pFile" to btn "revFrontScript" of stack "revLibrary"
    if "reloadStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "reloadStack pStack, pFile" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end reloadStack

on scriptParsingError pError
  if revOKTarget() then pass scriptParsingError
  try
    if "scriptParsingError" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "scriptParsingError pError" to btn "revFrontScript" of stack "revLibrary"
    if "scriptParsingError" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "scriptParsingError pError" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scriptParsingError

on scrollbarBeginning pStart
  if revOKTarget() then pass scrollbarBeginning
  try
    if "scrollbarBeginning" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "scrollbarBeginning pStart" to btn "revFrontScript" of stack "revLibrary"
    if "scrollbarBeginning" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "scrollbarBeginning pStart" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scrollbarBeginning

on scrollbarEnd pEnd
  if revOKTarget() then pass scrollbarEnd
  try
    if "scrollbarEnd" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "scrollbarEnd pEnd" to btn "revFrontScript" of stack "revLibrary"
    if "scrollbarEnd" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "scrollbarEnd pEnd" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scrollbarEnd

on scrollbarLineDec pPosition
  if revOKTarget() then pass scrollbarLineDec
  try
    if "scrollbarLineDec" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "scrollbarLineDec pPosition" to btn "revFrontScript" of stack "revLibrary"
    if "scrollbarLineDec" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "scrollbarLineDec pPosition" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scrollbarLineDec

on scrollbarLineInc pPosition
  if revOKTarget() then pass scrollbarLineInc
  try
    if "scrollbarLineInc" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "scrollbarLineInc pPosition" to btn "revFrontScript" of stack "revLibrary"
    if "scrollbarLineInc" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "scrollbarLineInc pPosition" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scrollbarLineInc

on scrollbarPageInc pPosition
  if revOKTarget() then pass scrollbarPageInc
  try
    if "scrollbarPageInc" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "scrollbarPageInc pPosition" to btn "revFrontScript" of stack "revLibrary"
    if "scrollbarPageInc" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "scrollbarPageInc pPosition" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scrollbarPageInc

on signal pSignalNo
  if revOKTarget() then pass signal
  try
    if "signal" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "signal pSignalNo" to btn "revFrontScript" of stack "revLibrary"
    if "signal" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "signal pSignalNo" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end signal

on socketError pID, pError
  if revOKTarget() then pass socketError
  try
    if "socketError" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "socketError pID, pError" to btn "revFrontScript" of stack "revLibrary"
    if "socketError" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "socketError pID, pError" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end socketError

on socketTimeout pID
  if revOKTarget() then pass socketTimeout
  try
    if "socketTimeout" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send "socketTimeout pID" to btn "revFrontScript" of stack "revLibrary"
    if "socketTimeout" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send "socketTimeout pID" to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end socketTimeout

on tabKey
  if revOKTarget() then pass tabKey
  try
    if "tabKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send tabKey to btn "revFrontScript" of stack "revLibrary"
    if "tabKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send tabKey to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  
  pass tabKey to metacard
end tabKey

on undoKey
  if revOKTarget() then pass undoKey
  try
    if "undoKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send undoKey to btn "revFrontScript" of stack "revLibrary"
    if "undoKey" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send undoKey to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
  
  pass undoKey to metacard
end undoKey

on uniconifyStack
  if revOKTarget() then pass uniconifyStack
  try
    if "uniconifyStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revFrontScript" of stack "revLibrary" then send uniconifyStack to btn "revFrontScript" of stack "revLibrary"
    if "uniconifyStack" is among the lines of the cREVGeneral["handlerlist"] of btn "revBackScript" of stack "revLibrary" then send uniconifyStack to btn "revBackScript" of stack "revLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end uniconifyStack

# OK-2008-03-13 : Bug 6009. The target will not be correct in the case of keyboard shortcuts. Function therefore
# extended to allow a target to be explicity passed.
function revOKTarget pTarget
  local tParse
  if pTarget is not empty then
    put the long id of pTarget into tParse
  else
    put the long id of the target into tParse
  end if
  
  get wordOffset("stack",tParse)
  put word (it) to (it+1) of tParse into tStackName
  put the short name of tStackName into tStackName
  if char 1 to 3 of tStackName is "rev" then return true
  if tStackName is among the items of "Script Error,Execution Error,Script Errors,Home,Message Box,Script Debugger,Answer Dialog,Ask Dialog,file selector" then return true
  return false
end revOKTarget

command writeError tError
  write tError & return to stderr
end writeError





