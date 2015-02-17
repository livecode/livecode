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
    if "mouseEnter" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "mouseEnter" to stack "revFrontScriptLibrary"
    if "mouseEnter" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "mouseEnter" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseEnter

on mouseLeave
  if revOKTarget() then pass mouseLeave
  try
    if "mouseLeave" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "mouseLeave" to stack "revFrontScriptLibrary"
    if "mouseLeave" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "mouseLeave" to stack "revBackScriptLibrary"
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
    #if "mouseDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "mouseDown" to stack "revFrontScriptLibrary"
    if "mouseDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "mouseDown" to stack "revBackScriptLibrary"
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
    #if "mouseUp" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "mouseUp pButtonNo" to stack "revFrontScriptLibrary"
    if "mouseUp" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "mouseUp pButtonNo" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseUp

on mouseWithin
  if revOKTarget() then pass mouseWithin
  try
    if "mouseWithin" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "mouseWithin" to stack "revFrontScriptLibrary"
    if "mouseWithin" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "mouseWithin" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseWithin

on mouseStillDown pButtonNo
  if revOKTarget() then pass mouseStillDown
  try
    if "mouseStillDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "mouseStillDown pButtonNo" to stack "revFrontScriptLibrary"
    if "mouseStillDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "mouseStillDown pButtonNo" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseStillDown

on mouseMove x,y
  if revOKTarget() then pass mouseMove
  try
    if "mouseMove" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "mouseMove x,y" to stack "revFrontScriptLibrary"
    if "mouseMove" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "mouseMove x,y" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseMove

on mouseRelease pButtonNo
  if revOKTarget() then pass mouseRelease
  try
    if "mouseRelease" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "mouseRelease pButtonNo" to stack "revFrontScriptLibrary"
    if "mouseRelease" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "mouseRelease pButtonNo" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseRelease

on mouseDoubleDown pButtonNo
  if revOKTarget() then pass mouseDoubleDown
  try
    if "mouseDoubleDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "mouseDoubleDown pButtonNo" to stack "revFrontScriptLibrary"
    if "mouseDoubleDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "mouseDoubleDown pButtonNo" to stack "revBackScriptLibrary"
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
    if "mouseDoubleUp" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "mouseDoubleUp pButtonNo, tTarget" to stack "revFrontScriptLibrary"
    if "mouseDoubleUp" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "mouseDoubleUp pButtonNo, tTarget" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end mouseDoubleUp

on openField
  if revOKTarget() then pass openField
  try
    if "openField" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "openField" to stack "revFrontScriptLibrary"
    if "openField" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "openField" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end openField

on closeField
  if revOKTarget() then pass closeField
  try
    if "closeField" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "closeField" to stack "revFrontScriptLibrary"
    if "closeField" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "closeField" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end closeField

on exitField
  if revOKTarget() then pass exitField
  try
    if "exitField" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send exitField to stack "revFrontScriptLibrary"
    if "exitField" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send exitField to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end exitField

on keyDown pKey
  if revOKTarget() then pass keyDown
  try
    if "keyDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "keyDown pKey" to stack "revFrontScriptLibrary"
    if "keyDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "keyDown pKey" to stack "revBackScriptLibrary"
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
    if "keyUp" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "keyUp pKey" to stack "revFrontScriptLibrary"
    if "keyUp" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "keyUp pKey" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end keyUp

on rawKeyDown pKeyNo
  if revOKTarget() then pass rawKeyDown
  try
    if "rawKeyDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "rawKeyDown pKeyNo" to stack "revFrontScriptLibrary"
    if "rawKeyDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "rawKeyDown pKeyNo" to stack "revBackScriptLibrary"
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
    if "rawKeyUp" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "rawKeyUp pKeyNo" to stack "revFrontScriptLibrary"
    if "rawKeyUp" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "rawKeyUp pKeyNo" to stack "revBackScriptLibrary"
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
    if "commandKeyDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "commandKeyDown pWhich" to stack "revFrontScriptLibrary"
    if "commandKeyDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "commandKeyDown pWhich" to stack "revBackScriptLibrary"
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
      if "arrowKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "arrowKey pKey" to stack "revFrontScriptLibrary"
      if "arrowKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "arrowKey pKey" to stack "revBackScriptLibrary"
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
    if "functionKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "functionKey pKey" to stack "revFrontScriptLibrary"
    if "functionKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "functionKey pKey" to stack "revBackScriptLibrary"
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
    if "pasteKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send pasteKey to stack "revFrontScriptLibrary"
    if "pasteKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send pasteKey to stack "revBackScriptLibrary"
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
    if "cutKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send cutKey to stack "revFrontScriptLibrary"
    if "cutKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send cutKey to stack "revBackScriptLibrary"
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
    if "copyKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send copyKey to stack "revFrontScriptLibrary"
    if "copyKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send copyKey to stack "revBackScriptLibrary"
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
    if "backSpaceKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send backSpaceKey to stack "revFrontScriptLibrary"
    if "backSpaceKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send backSpaceKey to stack "revBackScriptLibrary"
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
    if "deleteKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send deleteKey to stack "revFrontScriptLibrary"
    if "deleteKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send deleteKey to stack "revBackScriptLibrary"
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
    if "returnInField" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send returnInField to stack "revFrontScriptLibrary"
    if "returnInField" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send returnInField to stack "revBackScriptLibrary"
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
    if "enterInField" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send enterInField to stack "revFrontScriptLibrary"
    if "enterInField" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send enterInField to stack "revBackScriptLibrary"
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
    if "returnKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send returnKey to stack "revFrontScriptLibrary"
    if "returnKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send returnKey to stack "revBackScriptLibrary"
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
    if "enterKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send enterKey to stack "revFrontScriptLibrary"
    if "enterKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send enterKey to stack "revBackScriptLibrary"
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
    if "idle" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send idle to stack "revFrontScriptLibrary"
    if "idle" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send idle to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end idle

on controlKeyDown pKey
  if revOKTarget() then pass controlKeyDown
  try
    if "controlKeyDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "controlKeyDown pKey" to stack "revFrontScriptLibrary"
    if "controlKeyDown" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "controlKeyDown pKey" to stack "revBackScriptLibrary"
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
    if "focusIn" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send focusIn to stack "revFrontScriptLibrary"
    if "focusIn" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send focusIn to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end focusIn

on focusOut
  if revOKTarget() then pass focusOut
  try
    if "focusOut" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send focusOut to stack "revFrontScriptLibrary"
    if "focusOut" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send focusOut to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end focusOut

on scrollBarDrag pPosition
  if revOKTarget() then pass scrollBarDrag
  try
    if "scrollBarDrag" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "scrollBarDrag pPosition" to stack "revFrontScriptLibrary"
    if "scrollBarDrag" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "scrollBarDrag pPosition" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scrollBarDrag

on preOpenCard
  if revOKTarget() then pass preOpenCard
  try
    if "preOpenCard" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send preOpenCard to stack "revFrontScriptLibrary"
    if "preOpenCard" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send preOpenCard to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end preOpenCard

on openCard
  if revOKTarget() then pass openCard
  try
    if "openCard" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send openCard to stack "revFrontScriptLibrary"
    if "openCard" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send openCard to stack "revBackScriptLibrary"
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
    #if "preOpenStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send preOpenStack to stack "revFrontScriptLibrary"
    revIDEHandleNewStack the long id of the target
    if "preOpenStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send preOpenStack to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end preOpenStack

on openStack
  if revOKTarget() then pass openStack
  try
    if "openStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send openStack to stack "revFrontScriptLibrary"
    if "openStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send openStack to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end openStack

on startUp
  if revOKTarget() then pass startUp
  try
    if "startUp" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send startUp to stack "revFrontScriptLibrary"
    if "startUp" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send startUp to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end startUp

on closeStack
  if revOKTarget() then pass closeStack
  try
    if "closeStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send closeStack to stack "revFrontScriptLibrary"
    if "closeStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send closeStack to stack "revBackScriptLibrary"
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
    --if "closeStackRequest" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send closeStackRequest to stack "revFrontScriptLibrary"
    --if "closeStackRequest" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send closeStackRequest to stack "revBackScriptLibrary"
  catch tError
    if gRevDevelopment then
      writeError (tError)
    end if
 end try
end closeStackRequest

on quitMC
  if revOKTarget() then pass quitMC
  try
    if "quitMC" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send quitMC to stack "revFrontScriptLibrary"
    if "quitMC" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send quitMC to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end quitMC

on playStopped
  if revOKTarget() then pass playStopped
  try
    if "playStopped" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send playStopped to stack "revFrontScriptLibrary"
    if "playStopped" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send playStopped to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end playStopped

on moveStopped
  if revOKTarget() then pass moveStopped
  try
    if "moveStopped" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send moveStopped to stack "revFrontScriptLibrary"
    if "moveStopped" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send moveStopped to stack "revBackScriptLibrary"
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
    if "nameChanged" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "nameChanged pOldName, pNewName, tTarget" to stack "revFrontScriptLibrary"
    if "nameChanged" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "nameChanged pOldName, pNewName, tTarget" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end nameChanged

on idChanged pOldID, pNewID
  if revOKTarget() then pass idChanged
  try
    if "idChanged" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "idChanged pOldID, pNewID" to stack "revFrontScriptLibrary"
    if "idChanged" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "idChanged pOldID, pNewID" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end idChanged

on saveRequest
  if revOKTarget() then pass saveRequest
  try
    if "saveRequest" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send saveRequest to stack "revFrontScriptLibrary"
    if "saveRequest" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send saveRequest to stack "revBackScriptLibrary"
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
    if "moveControl" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send moveControl to stack "revFrontScriptLibrary"
    if "moveControl" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send moveControl to stack "revBackScriptLibrary"
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
    if "resizeControl" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send resizeControl to stack "revFrontScriptLibrary"
    if "resizeControl" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send resizeControl to stack "revBackScriptLibrary"
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
    if "menuPick" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "menuPick pWhich, pLast" to stack "revFrontScriptLibrary"
    if "menuPick" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "menuPick pWhich, pLast" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end menuPick

on selectionChanged
  if revOKTarget() then pass selectionChanged
  try
    if "selectionChanged" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send selectionChanged to stack "revFrontScriptLibrary"
    if "selectionChanged" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send selectionChanged to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end selectionChanged

on selectedObjectChanged
  if revOKTarget() then pass selectedObjectChanged
  try
    if "selectedObjectChanged" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send selectedObjectChanged to stack "revFrontScriptLibrary"
    if "selectedObjectChanged" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send selectedObjectChanged to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end selectedObjectChanged

on resizeStack pNewWidth, pNewHeight, pOldWidth, pOldHeight
  if revOKTarget() then pass resizeStack
  try
    if "resizeStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "resizeStack pNewWidth, pNewHeight, pOldWidth, pOldHeight" to stack "revFrontScriptLibrary"
    if "resizeStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "resizeStack pNewWidth, pNewHeight, pOldWidth, pOldHeight" to stack "revBackScriptLibrary"
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
    if "moveStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "moveStack x,y" to stack "revFrontScriptLibrary"
    if "moveStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "moveStack x,y" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end moveStack

on closeCard
  if revOKTarget() then pass closeCard
  try
    if "closeCard" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send closeCard to stack "revFrontScriptLibrary"
    if "closeCard" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send closeCard to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end closeCard

on resumeStack
  if revOKTarget() then pass resumeStack
  try
    if "resumeStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send resumeStack to stack "revFrontScriptLibrary"
    if "resumeStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send resumeStack to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end resumeStack

on suspendStack
  if revOKTarget() then pass suspendStack
  try
    if "suspendStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "revIDESuspendStackFront" to button "revFrontScript" of stack "revLibrary"
    if "suspendStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send suspendStack to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end suspendStack

on iconifyStack
  if revOKTarget() then pass iconifyStack
  try
    if "iconifyStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send iconifyStack to stack "revFrontScriptLibrary"
    if "iconifyStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send iconifyStack to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end iconifyStack

on appleEvent pClass, pID, pSender
  if revOKTarget() then pass appleEvent
  try
    if "appleEvent" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "appleEvent pClass, pID, pSender" to stack "revFrontScriptLibrary"
    if "appleEvent" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "appleEvent pClass, pID, pSender" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end appleEvent

on closeBackground pID
  if revOKTarget() then pass closeBackground
  try
    if "closeBackground" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "closeBackground pID" to stack "revFrontScriptLibrary"
    if "closeBackground" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "closeBackground pID" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end closeBackground

on currentTimeChanged pTime
  if revOKTarget() then pass currentTimeChanged
  try
    if "currentTimeChanged" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "currentTimeChanged pTime" to stack "revFrontScriptLibrary"
    if "currentTimeChanged" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "currentTimeChanged pTime" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end currentTimeChanged

on cutKey
  if revOKTarget() then pass cutKey
  try
    if "cutKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send cutKey to stack "revFrontScriptLibrary"
    if "cutKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send cutKey to stack "revBackScriptLibrary"
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
    if "deletebackground" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send deletebackground to stack "revFrontScriptLibrary"
    if "deletebackground" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send deletebackground to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deletebackground

on deleteButton
  if revOKTarget() then pass deleteButton
  try
    if "deleteButton" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send deleteButton to stack "revFrontScriptLibrary"
    if "deleteButton" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send deleteButton to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deleteButton

on deletecard
  if revOKTarget() then pass deletecard
  try
    if "deletecard" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send deletecard to stack "revFrontScriptLibrary"
    if "deletecard" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send deletecard to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deletecard

on deleteEPS
  if revOKTarget() then pass deleteEPS
  try
    if "deleteEPS" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send deleteEPS to stack "revFrontScriptLibrary"
    if "deleteEPS" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send deleteEPS to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deleteEPS

on deleteField
  if revOKTarget() then pass deleteField
  try
    if "deleteField" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send deleteField to stack "revFrontScriptLibrary"
    if "deleteField" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send deleteField to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deleteField

on deleteGraphic
  if revOKTarget() then pass deleteGraphic
  try
    if "deleteGraphic" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send deleteGraphic to stack "revFrontScriptLibrary"
    if "deleteGraphic" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send deleteGraphic to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deleteGraphic

on deleteImage
  if revOKTarget() then pass deleteImage
  try
    if "deleteImage" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send deleteImage to stack "revFrontScriptLibrary"
    if "deleteImage" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send deleteImage to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deleteImage

on deletePlayer
  if revOKTarget() then pass deletePlayer
  try
    if "deletePlayer" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send deletePlayer to stack "revFrontScriptLibrary"
    if "deletePlayer" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send deletePlayer to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end deletePlayer

on dragMove
  if revOKTarget() then pass dragMove
  try
    if "dragMove" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send dragMove to stack "revFrontScriptLibrary"
    if "dragMove" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send dragMove to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end dragMove

on dragEnter
  if revOKTarget() then pass dragEnter
  try
    if "dragEnter" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send dragEnter to stack "revFrontScriptLibrary"
    if "dragEnter" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send dragEnter to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end dragEnter

on errorDialog pError
  if revOKTarget() then pass errorDialog
  try
    if "errorDialog" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "errorDialog pError" to stack "revFrontScriptLibrary"
    if "errorDialog" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "errorDialog pError" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end errorDialog

on escapeKey
  if revOKTarget() then pass escapeKey
  try
    if "escapeKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send escapeKey to stack "revFrontScriptLibrary"
    if "escapeKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send escapeKey to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end escapeKey

on help
  if revOKTarget() then pass help
  try
    if "help" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send help to stack "revFrontScriptLibrary"
    if "help" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send help to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end help

on libraryStack
  if revOKTarget() then pass libraryStack
  try
    if "libraryStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send libraryStack to stack "revFrontScriptLibrary"
    if "libraryStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send libraryStack to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end libraryStack

on newBackground
  if revOKTarget() then pass newBackground
  try
    if "newBackground" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send newBackground to stack "revFrontScriptLibrary"
    if "newBackground" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send newBackground to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newBackground

on NewButton
  if revOKTarget() then pass NewButton
  try
    if "NewButton" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send NewButton to stack "revFrontScriptLibrary"
    if "NewButton" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send NewButton to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end NewButton

on newPlayer
  if revOKTarget() then pass newPlayer
  try
    if "newPlayer" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send newPlayer to stack "revFrontScriptLibrary"
    if "newPlayer" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send newPlayer to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newPlayer

on newCard
  if revOKTarget() then pass newCard
  try
    if "newCard" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send newCard to stack "revFrontScriptLibrary"
    if "newCard" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send newCard to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newCard

on newEPS
  if revOKTarget() then pass newEPS
  try
    if "newEPS" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send newEPS to stack "revFrontScriptLibrary"
    if "newEPS" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send newEPS to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newEPS

on newField
  if revOKTarget() then pass newField
  try
    if "newField" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send newField to stack "revFrontScriptLibrary"
    if "newField" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send newField to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newField

on newGraphic
  if revOKTarget() then pass newGraphic
  try
    if "newGraphic" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send newGraphic to stack "revFrontScriptLibrary"
    if "newGraphic" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send newGraphic to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newGraphic

on newGroup
  if revOKTarget() then pass newGroup
  try
    if "newGroup" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send newGroup to stack "revFrontScriptLibrary"
    if "newGroup" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send newGroup to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newGroup

on newImage
  if revOKTarget() then pass newImage
  try
    if "newImage" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send newImage to stack "revFrontScriptLibrary"
    if "newImage" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send newImage to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newImage

on newScrollbar
  if revOKTarget() then pass newScrollbar
  try
    if "newScrollbar" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send newScrollbar to stack "revFrontScriptLibrary"
    if "newScrollbar" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send newScrollbar to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newScrollbar

on newStack
  if revOKTarget() then pass newStack
  try
    if "newStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send newStack to stack "revFrontScriptLibrary"
    if "newStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send newStack to stack "revBackScriptLibrary"
  catch tError
    if gRevDevelopment then
      writeError (tError)
    end if
  end try
end newStack

on newTool pTool
  if revOKTarget() then pass newTool
  try
    if "newTool" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "newTool pTool" to stack "revFrontScriptLibrary"
    if "newTool" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "newTool pTool" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end newTool

on openBackground pID
  if revOKTarget() then pass openBackground
  try
    if "openBackground" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "openBackground pID" to stack "revFrontScriptLibrary"
    if "openBackground" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "openBackground pID" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end openBackground

on preOpenbackground pID
  if revOKTarget() then pass preOpenbackground
  try
    if "preOpenbackground" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "preOpenbackground pID" to stack "revFrontScriptLibrary"
    if "preOpenbackground" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "preOpenbackground pID" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end preOpenbackground

on reloadStack pStack, pFile
  if revOKTarget() then pass reloadStack
  try
    if "reloadStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "reloadStack pStack, pFile" to stack "revFrontScriptLibrary"
    if "reloadStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "reloadStack pStack, pFile" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end reloadStack

on scriptParsingError pError
  if revOKTarget() then pass scriptParsingError
  try
    if "scriptParsingError" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "scriptParsingError pError" to stack "revFrontScriptLibrary"
    if "scriptParsingError" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "scriptParsingError pError" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scriptParsingError

on scrollbarBeginning pStart
  if revOKTarget() then pass scrollbarBeginning
  try
    if "scrollbarBeginning" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "scrollbarBeginning pStart" to stack "revFrontScriptLibrary"
    if "scrollbarBeginning" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "scrollbarBeginning pStart" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scrollbarBeginning

on scrollbarEnd pEnd
  if revOKTarget() then pass scrollbarEnd
  try
    if "scrollbarEnd" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "scrollbarEnd pEnd" to stack "revFrontScriptLibrary"
    if "scrollbarEnd" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "scrollbarEnd pEnd" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scrollbarEnd

on scrollbarLineDec pPosition
  if revOKTarget() then pass scrollbarLineDec
  try
    if "scrollbarLineDec" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "scrollbarLineDec pPosition" to stack "revFrontScriptLibrary"
    if "scrollbarLineDec" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "scrollbarLineDec pPosition" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scrollbarLineDec

on scrollbarLineInc pPosition
  if revOKTarget() then pass scrollbarLineInc
  try
    if "scrollbarLineInc" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "scrollbarLineInc pPosition" to stack "revFrontScriptLibrary"
    if "scrollbarLineInc" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "scrollbarLineInc pPosition" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scrollbarLineInc

on scrollbarPageInc pPosition
  if revOKTarget() then pass scrollbarPageInc
  try
    if "scrollbarPageInc" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "scrollbarPageInc pPosition" to stack "revFrontScriptLibrary"
    if "scrollbarPageInc" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "scrollbarPageInc pPosition" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end scrollbarPageInc

on signal pSignalNo
  if revOKTarget() then pass signal
  try
    if "signal" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "signal pSignalNo" to stack "revFrontScriptLibrary"
    if "signal" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "signal pSignalNo" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end signal

on socketError pID, pError
  if revOKTarget() then pass socketError
  try
    if "socketError" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "socketError pID, pError" to stack "revFrontScriptLibrary"
    if "socketError" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "socketError pID, pError" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end socketError

on socketTimeout pID
  if revOKTarget() then pass socketTimeout
  try
    if "socketTimeout" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send "socketTimeout pID" to stack "revFrontScriptLibrary"
    if "socketTimeout" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send "socketTimeout pID" to stack "revBackScriptLibrary"
  catch tError
    if gREVDevelopment then
      writeError (tError)
    end if
  end try
end socketTimeout

on tabKey
  if revOKTarget() then pass tabKey
  try
    if "tabKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send tabKey to stack "revFrontScriptLibrary"
    if "tabKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send tabKey to stack "revBackScriptLibrary"
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
    if "undoKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send undoKey to stack "revFrontScriptLibrary"
    if "undoKey" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send undoKey to stack "revBackScriptLibrary"
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
    if "uniconifyStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revFrontScriptLibrary" then send uniconifyStack to stack "revFrontScriptLibrary"
    if "uniconifyStack" is among the lines of the cREVGeneral["handlerlist"] of stack "revBackScriptLibrary" then send uniconifyStack to stack "revBackScriptLibrary"
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





