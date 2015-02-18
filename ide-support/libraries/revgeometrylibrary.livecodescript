script "revgeometrylibrary"

on revLoadLibrary
   insert the script of me into back
end revLoadLibrary

on revUnloadLibrary
   remove the script of me from back
end revUnloadLibrary

# OK-2007-08-10 : Added variable declarations and quotes round literals to this script, also tidied up some code and fixed some bugs.

local lGeometryObject, lOrigWidth, lOrigHeight
local lTimerID

on resizeStack p1, p2, p3, p4
  global gREVMessageDispatch
  put p3 into lOrigWidth
  put p4 into lOrigHeight
  repeat for each line tStack in gREVMessageDispatch["revChangeWindowSize"]
    send "revResizeStack" to this card of stack tStack
  end repeat
  
  # OK-2007-08-10: Added declaration for global, previously this block would not have worked as intended.
  global gREVSuppressMessages
  if char 1 to 3 of REVTargetStack() is "rev" then
    if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then 
      pass resizeStack
    end if
  end if
  
  if "cREVGeometryCache" is among the lines of the customPropertySets of this card of stack REVTargetStack() then 
    revUpdateGeometry
  end if
  
  if the environment is "development" then
    if lTimerID is not empty then 
      cancel lTimerID
    end if
    
    put revTargetStack(the long id of the target) into tStack
    
    local tPalettes
    put revPropertyPalettes(the long id of stack tStack) into tPalettes
    repeat for each line tPalette in tPalettes
      if the short name of this card of stack tPalette is "revStackSize&Position" and not the cDontUpdate of stack tPalette then
        send "revUpdatePalette" to this cd of stack tPalette in 1 second
        put the result into lTimerID
        exit repeat
      end if
    end repeat
    
    if there is a stack "message box" then
      local tDefaultStack
      put the defaultStack into tDefaultStack
      send "revUpdateActiveStack tDefaultStack, tDefaultStack" to stack "message box"
    end if
    
  end if
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then 
    pass resizeStack
  end if
end resizeStack

on revUpdateGeometry
   lock messages
   
   -- 2006-05-22:O.K:Fix for 3413
   local tDefaultStack
   put the defaultStack into tDefaultStack
   set the defaultStack to REVTargetStack()
   
   if (the cREVGeometryCache["order"] of this card is empty) or (the cREVGeometryCache["total"] of this card is not the number of controls) or (the number of words in the customKeys["cREVGeometryCacheIDs"] of this card is 0) then 
      revCacheGeometry
   end if
   
   local tCardWidth
   put the width of this stack + the cREVGeneral["virtualWidth"] of this cd into tCardWidth
   
   local tCardHeight
   put the height of this stack + the cREVGeneral["virtualHeight"] of this cd into tCardHeight
   add the vScroll of this stack to tCardHeight
   
   local tCardLoc
   put trunc(tCardWidth / 2), trunc(tCardHeight / 2) into tCardLoc
   
   local tIDArray
   put the customProperties["cREVGeometryCacheIDs"] of this card into tIDArray
   
   # OK-2007-08-10 : The below block of code was modifying the repeat loop control, which would have caused unexpected consequences.
   # Added the tControlId variable to fix this problem
   local tControlId
   repeat for each line tControl in the cREVGeometryCache["order"] of this card
      put tIDArray[tControl] into tControlId
      if not (there is a control id tControlId) then
         revCacheGeometry
         exit repeat
      end if     
   end repeat
   
   # OK-2007-08-10 : Not sure why these two lines are necessary...
   set the defaultStack to REVTargetStack()
   put the customProperties["cREVGeometryCacheIDs"] of this card into tIDArray
   
   # OK-2007-08-10 : Modified the below repeat loop to remove the unreadable use of "l", and also to
   # prevent the modifying of the repeat loop control variable, which could have had unexpected consequences.
   repeat for each line tControl in the cREVGeometryCache["order"] of this card
      put tIDArray[tControl] into tControlId
      if the number of words in tControlId is 0 then 
         next repeat
      end if
      
      local tProfileName
      put the cREVGeneral["profile"] of control id tControlId into tProfileName
      if tProfileName is empty or tProfileName is the cREVGeneral["masterName"] of control id tControlId then 
         put "Master" into tProfileName
      end if
      
      if the cREVGeometry[tProfileName] of control id tControlId is not true then 
         next repeat
      end if
      
      local tGroupLockLocation
      if word 1 of the name of control id tControlId is "group" then
         put the lockLocation of control id tControlId into tGroupLockLocation[tControlId]
         set the lockLocation of control id tControlId to true
      end if
      
      local tObjectRect
      put the rect of control id tControlId into tObjectRect
      if the cREVGeometry[tProfileName,"moveH"] of control id tControlId is true then
         
         --HORIZONTAL MOVEMENT
         local tObjectH
         put item 1 of the loc of control id tControlId into tObjectH
         if the cREVGeometry[tProfilename,"moveHAbsolute"] of control id tControlId is true then
            --absolute movement of H
            if the cREVGeometry[tProfileName,"moveHObjectRef"] of control id tControlId is "card" then
               --move H in absolute reference to the card
               local tDistance
               put the cREVGeometry[tProfileName,"moveHDistance"] of control id tControlId into tDistance
               if the cREVGeometry[tProfileName,"moveHObjectSide"] of control id tControlId is "left" then
                  local tHMoved
                  put the left of this card + tDistance into tHMoved
                  
                  local tDifference
                  put tHMoved - tObjectH into tDifference
                  add tDifference to item 1 of tObjectRect
                  add tDifference to item 3 of tObjectRect
               else if the cREVGeometry[tProfileName,"moveHObjectSide"] of control id tControlId is "right" then
                  put tCardWidth + tDistance into tHMoved
                  put tHMoved - tObjectH into tDifference
                  add tDifference to item 1 of tObjectRect
                  add tDifference to item 3 of tObjectRect
               else  if the cREVGeometry[tProfileName,"moveHObjectSide"] of control id tControlId is "center" then
                  put item 1 of tCardLoc + tDistance into tHMoved
                  put tHMoved - tObjectH into tDifference
                  add tDifference to item 1 of tObjectRect
                  add tDifference to item 3 of tObjectRect
               end if
            else
               --move H in absolute reference to an object
               put the cREVGeometry[tProfileName,"moveHDistance"] of control id tControlId into tDistance
               
               local tObjectRef
               put the cREVGeometry[tProfileName,"moveHObjectRef"] of control id tControlId into tObjectRef
               put tIDArray[tObjectRef] into tObjectRef
               if the cREVGeometry[tProfileName,"moveHObjectSide"] of control id tControlId is "left" then
                  put the left of control id tObjectRef + tDistance into tHMoved
                  put tHMoved - tObjectH into tDifference
                  add tDifference to item 1 of tObjectRect
                  add tDifference to item 3 of tObjectRect
               else if the cREVGeometry[tProfileName,"moveHObjectSide"] of control id tControlId is "right" then
                  put the right of control id tObjectRef + tDistance into tHMoved
                  put tHMoved - tObjectH into tDifference
                  add tDifference to item 1 of tObjectRect
                  add tDifference to item 3 of tObjectRect
               else  if the cREVGeometry[tProfileName,"moveHObjectSide"] of control id tControlId is "center" then
                  put item 1 of the loc of control id tObjectRef + tDistance into tHMoved
                  put tHMoved - tObjectH into tDifference
                  add tDifference to item 1 of tObjectRect
                  add tDifference to item 3 of tObjectRect
               end if
            end if
         else
            --relative movement of H
            if the cREVGeometry[tProfileName,"moveHObjectRef"] of control id tControlId is "card" then
               --move H in relative reference to the card
               put the cREVGeometry[tProfileName,"moveHDistance"] of control id tControlId into tDistance
               --we assume relative to bottom
               put round(tDistance * tCardWidth) into tDistance
               put tCardWidth + tDistance into tHMoved
               put tHMoved - tObjectH into tDifference
               add tDifference to item 1 of tObjectRect
               add tDifference to item 3 of tObjectRect
            else
               put the cREVGeometry[tProfileName,"moveHDistance"] of control id tControlId into tDistance
               put the cREVGeometry[tProfileName,"moveHObjectRef"] of control id tControlId into tObjectRef
               put tIDArray[tObjectRef] into tObjectRef
               put round(tDistance * tCardWidth) into tDistance
               
               # TH-2008-07-07 :: Bug 6391, The linked object should remain a fixed percentage of the width of the card away
               # from the right of the object it is linked to.  Previously this section used item 1 of the loc of the object being linked to
               # instead of the right, this caused an initial jump in location of the linked object.
               --put item 1 of the loc of control id tObjectRef + tDistance into tHMoved
               put item 3 of the rect of control id tObjectRef + tDistance into tHMoved
               
               put tHMoved - tObjectH into tDifference
               add tDifference to item 1 of tObjectRect
               add tDifference to item 3 of tObjectRect
            end if
         end if
      else
         ----CHECK FOR EITHER LEFT OR RIGHT SCALING
         ----LEFT SCALING
         if the cREVGeometry[tProfileName,"scaleLeft"] of control id tControlId is true then
            if the cREVGeometry[tProfilename,"scaleLeftAbsolute"] of control id tControlId is true then
               --absolute scaling of left
               if the cREVGeometry[tProfileName,"scaleLeftObjectRef"] of control id tControlId is "card" then
                  --scale left in absolute reference to the card
                  put the cREVGeometry[tProfileName,"scaleLeftDistance"] of control id tControlId into tDistance
                  if the cREVGeometry[tProfileName,"scaleLeftObjectSide"] of control id tControlId is "left" then
                     put the left of this cd + tDistance into item 1 of tObjectRect
                  else if the cREVGeometry[tProfileName,"scaleLeftObjectSide"] of control id tControlId is "right" then
                     put tCardWidth + tDistance into item 1 of tObjectRect
                  else  if the cREVGeometry[tProfileName,"scaleLeftObjectSide"] of control id tControlId is "center" then
                     put item 1 of tCardLoc + tDistance into item 1 of tObjectRect
                  end if
               else
                  --scale left in absolute reference to an object
                  put the cREVGeometry[tProfileName,"scaleLeftDistance"] of control id tControlId into tDistance
                  put the cREVGeometry[tProfileName,"scaleLeftObjectRef"] of control id tControlId into tObjectRef
                  put tIDArray[tObjectRef] into tObjectRef
                  if the cREVGeometry[tProfileName,"scaleLeftObjectSide"] of control id tControlId is "left" then
                     put the left of control id tObjectRef + tDistance into item 1 of tObjectRect
                  else if the cREVGeometry[tProfileName,"scaleLeftObjectSide"] of control id tControlId is "right" then
                     put the right of control id tObjectRef + tDistance into item 1 of tObjectRect
                  else  if the cREVGeometry[tProfileName,"scaleLeftObjectSide"] of control id tControlId is "center" then
                     put item 1 of the loc of control id tObjectRef + tDistance into item 1 of tObjectRect
                  end if
               end if
            else
               --relative scaling of left
               if the cREVGeometry[tProfileName,"scaleLeftObjectRef"] of control id tControlId is "card" then
                  --scale left in relative reference to the card
                  put the cREVGeometry[tProfileName,"scaleLeftDistance"] of control id tControlId into tDistance
                  --we assume relative to center
                  put round(tDistance * tCardWidth) into tDistance
                  put tCardWidth+tDistance into item 1 of tObjectRect
               else
                  put the cREVGeometry[tProfileName,"scaleLeftDistance"] of control id tControlId into tDistance
                  put the cREVGeometry[tProfileName,"scaleLeftObjectRef"] of control id tControlId into tObjectRef
                  put tIDArray[tObjectRef] into tObjectRef
                  put round(tDistance * tCardWidth) into tDistance
                  # TH-2008-07-07 :: Bug 6391, similar problem to elsewhere, shouldn't be 
                  # using loc, instead item of rect
                  --put item 1 of the loc of control id tObjectRef+tDistance into item 1 of tObjectRect
                  put item 3 of the rect of control id tObjectRef+tDistance into item 1 of tObjectRect 
               end if
            end if
         end if
         ----RIGHT SCALING
         if the cREVGeometry[tProfileName,"scaleRight"] of control id tControlId is true then
            if the cREVGeometry[tProfilename,"scaleRightAbsolute"] of control id tControlId is true then
               --absolute scaling of right
               if the cREVGeometry[tProfileName,"scaleRightObjectRef"] of control id tControlId is "card" then
                  --scale right in absolute reference to the card
                  put the cREVGeometry[tProfileName,"scaleRightDistance"] of control id tControlId into tDistance
                  if the cREVGeometry[tProfileName,"scaleRightObjectSide"] of control id tControlId is "right" then
                     put tCardWidth + tDistance into item 3 of tObjectRect
                  else if the cREVGeometry[tProfileName,"scaleRightObjectSide"] of control id tControlId is "left" then
                     put 0 + tDistance into item 3 of tObjectRect
                  else  if the cREVGeometry[tProfileName,"scaleRightObjectSide"] of control id tControlId is "center" then
                     put item 1 of tCardLoc + tDistance into item 3 of tObjectRect
                  end if
               else
                  --scale right in absolute reference to an object
                  put the cREVGeometry[tProfileName,"scaleRightDistance"] of control id tControlId into tDistance
                  put the cREVGeometry[tProfileName,"scaleRightObjectRef"] of control id tControlId into tObjectRef
                  put tIDArray[tObjectRef] into tObjectRef
                  if the cREVGeometry[tProfileName,"scaleRightObjectSide"] of control id tControlId is "right" then
                     put the right of control id tObjectRef + tDistance into item 3 of tObjectRect
                  else if the cREVGeometry[tProfileName,"scaleRightObjectSide"] of control id tControlId is "left" then
                     put the left of control id tObjectRef + tDistance into item 3 of tObjectRect
                  else  if the cREVGeometry[tProfileName,"scaleRightObjectSide"] of control id tControlId is "center" then
                     put item 1 of the loc of control id tObjectRef + tDistance into item 3 of tObjectRect
                  end if
               end if
            else
               --relative scaling of right
               if the cREVGeometry[tProfileName,"scaleRightObjectRef"] of control id tControlId is "card" then
                  --scale right in relative reference to the card
                  put the cREVGeometry[tProfileName,"scaleRightDistance"] of control id tControlId into tDistance
                  --we assume relative to center
                  put round(tDistance * tCardWidth) into tDistance
                  put tCardWidth+tDistance into item 3 of tObjectRect
               else
                  put the cREVGeometry[tProfileName,"scaleRightDistance"] of control id tControlId into tDistance
                  put the cREVGeometry[tProfileName,"scaleRightObjectRef"] of control id tControlId into tObjectRef
                  put tIDArray[tObjectRef] into tObjectRef
                  put round(tDistance * tCardWidth) into tDistance
                  
                  # TH-2008-07-07 :: Bug 6391, similar problem to elsewhere, should be making use of the
                  # right not item 1 of the loc
                  --put item 1 of the loc of control id tObjectRef+tDistance into item 3 of tObjectRect
                  put item 3 of the rect of control id tObjectRef+tDistance into item 3 of tObjectRect                  
               end if
            end if
         end if
         --END OF HORIZONTAL
      end if
      --VERTICAL
      if the cREVGeometry[tProfileName,"moveV"] of control id tControlId is true then
         --VERTICAL MOVEMENT
         
         local tObjectV
         put item 2 of the loc of control id tControlId into tObjectV
         if the cREVGeometry[tProfilename,"moveVAbsolute"] of control id tControlId is true then
            --absolute movement of V
            if the cREVGeometry[tProfileName,"moveVObjectRef"] of control id tControlId is "card" then
               --move V in absolute reference to the card
               put the cREVGeometry[tProfileName,"moveVDistance"] of control id tControlId into tDistance
               if the cREVGeometry[tProfileName,"moveVObjectSide"] of control id tControlId is "top" then
                  local tVMoved
                  put the top of this cd + tDistance into tVMoved
                  put tVMoved - tObjectV into tDifference
                  add tDifference to item 2 of tObjectRect
                  add tDifference to item 4 of tObjectRect
               else if the cREVGeometry[tProfileName,"moveVObjectSide"] of control id tControlId is "bottom" then
                  put tCardHeight + tDistance into tVMoved
                  put tVMoved - tObjectV into tDifference
                  add tDifference to item 2 of tObjectRect
                  add tDifference to item 4 of tObjectRect
               else  if the cREVGeometry[tProfileName,"moveVObjectSide"] of control id tControlId is "center" then
                  put item 2 of tCardLoc + tDistance into tVMoved
                  put tVMoved - tObjectV into tDifference
                  add tDifference to item 2 of tObjectRect
                  add tDifference to item 4 of tObjectRect
               end if
            else
               --move V in absolute reference to an object
               put the cREVGeometry[tProfileName,"moveVDistance"] of control id tControlId into tDistance
               put the cREVGeometry[tProfileName,"moveVObjectRef"] of control id tControlId into tObjectRef
               put tIDArray[tObjectRef] into tObjectRef
               if the cREVGeometry[tProfileName,"moveVObjectSide"] of control id tControlId is "top" then
                  put the top of control id tObjectRef + tDistance into tVMoved
                  put tVMoved - tObjectV into tDifference
                  add tDifference to item 2 of tObjectRect
                  add tDifference to item 4 of tObjectRect
               else if the cREVGeometry[tProfileName,"moveVObjectSide"] of control id tControlId is "bottom" then
                  put the bottom of control id tObjectRef + tDistance into tVMoved
                  put tVMoved - tObjectV into tDifference
                  add tDifference to item 2 of tObjectRect
                  add tDifference to item 4 of tObjectRect
               else  if the cREVGeometry[tProfileName,"moveVObjectSide"] of control id tControlId is "center" then
                  put item 2 of the loc of control id tObjectRef + tDistance into tVMoved
                  put tVMoved - tObjectV into tDifference
                  add tDifference to item 2 of tObjectRect
                  add tDifference to item 4 of tObjectRect
               end if
            end if
         else
            --relative movement of V
            if the cREVGeometry[tProfileName,"moveVObjectRef"] of control id tControlId is "card" then
               --move V in relative reference to the right
               put the cREVGeometry[tProfileName,"moveVDistance"] of control id tControlId into tDistance
               --we assume relative to bottom
               put round(tDistance * tCardHeight) into tDistance
               put tCardHeight + tDistance into tVMoved
               put tVMoved - tObjectV into tDifference
               add tDifference to item 2 of tObjectRect
               add tDifference to item 4 of tObjectRect
            else
               put the cREVGeometry[tProfileName,"moveVDistance"] of control id tControlId into tDistance
               put the cREVGeometry[tProfileName,"moveVObjectRef"] of control id tControlId into tObjectRef
               put tIDArray[tObjectRef] into tObjectRef
               put round(tDistance * tCardHeight) into tDistance
               
               # TH-2008-07-07 :: Bug 6391, The linked object should remain a fixed percentage of the height of the card away
               # from the top of the object it is linked to.  Previously this section used item 2 of the loc of the object being linked to
               # instead of the bottom, this caused an initial jump in location of the linked object.
               --put item 2 of the loc of control id tObjectRef + tDistance into tVMoved
               put item 4 of the rect of control id tObjectRef + tDistance into tVMoved
               
               put tVMoved - tObjectV into tDifference
               add tDifference to item 2 of tObjectRect
               add tDifference to item 4 of tObjectRect
            end if
         end if
      else
         ----CHECK FOR EITHER TOP OR BOTTOM SCALING
         ----TOP SCALING
         if the cREVGeometry[tProfileName,"scaleTop"] of control id tControlId is true then
            if the cREVGeometry[tProfilename,"scaleTopAbsolute"] of control id tControlId is true then
               --absolute scaling of Top
               if the cREVGeometry[tProfileName,"scaleTopObjectRef"] of control id tControlId is "card" then
                  --scale Top in absolute reference to the card
                  put the cREVGeometry[tProfileName,"scaleTopDistance"] of control id tControlId into tDistance
                  if the cREVGeometry[tProfileName,"scaleTopObjectSide"] of control id tControlId is "Top" then
                     put the top of this cd + tDistance into item 2 of tObjectRect
                  else if the cREVGeometry[tProfileName,"scaleTopObjectSide"] of control id tControlId is "bottom" then
                     put tCardHeight + tDistance into item 2 of tObjectRect
                  else  if the cREVGeometry[tProfileName,"scaleTopObjectSide"] of control id tControlId is "center" then
                     put item 2 of tCardLoc + tDistance into item 2 of tObjectRect
                  end if
               else
                  --scale Top in absolute reference to an object
                  put the cREVGeometry[tProfileName,"scaleTopDistance"] of control id tControlId into tDistance
                  put the cREVGeometry[tProfileName,"scaleTopObjectRef"] of control id tControlId into tObjectRef
                  put tIDArray[tObjectRef] into tObjectRef
                  if the cREVGeometry[tProfileName,"scaleTopObjectSide"] of control id tControlId is "top" then
                     put the top of control id tObjectRef + tDistance into item 2 of tObjectRect
                  else if the cREVGeometry[tProfileName,"scaleTopObjectSide"] of control id tControlId is "bottom" then
                     put the bottom of control id tObjectRef + tDistance into item 2 of tObjectRect
                  else  if the cREVGeometry[tProfileName,"scaleTopObjectSide"] of control id tControlId is "center" then
                     put item 2 of the loc of control id tObjectRef + tDistance into item 2 of tObjectRect
                  end if
               end if
            else
               --relative scaling of Top
               if the cREVGeometry[tProfileName,"scaleTopObjectRef"] of control id tControlId is "card" then
                  --scale Top in relative reference to the card
                  put the cREVGeometry[tProfileName,"scaleTopDistance"] of control id tControlId into tDistance
                  --we assume relative to center
                  put round(tDistance * tCardHeight) into tDistance
                  put tCardHeight+tDistance into item 2 of tObjectRect
               else
                  put the cREVGeometry[tProfileName,"scaleTopDistance"] of control id tControlId into tDistance
                  put the cREVGeometry[tProfileName,"scaleTopObjectRef"] of control id tControlId into tObjectRef
                  put tIDArray[tObjectRef] into tObjectRef
                  put round(tDistance * tCardHeight) into tDistance
                  # TH-2008-07-07 :: Bug 6391, similar problem to elsewhere, shouldn't be 
                  # using loc, instead item of rect
                  --put item 2 of the loc of control id tObjectRef+tDistance into item 2 of tObjectRect
                  put item 4 of the rect of control id tObjectRef+tDistance into item 2 of tObjectRect
               end if
            end if
         end if
         ----Bottom SCALING
         if the cREVGeometry[tProfileName,"scaleBottom"] of control id tControlId is true then
            if the cREVGeometry[tProfilename,"scaleBottomAbsolute"] of control id tControlId is true then
               --absolute scaling of Bottom
               if the cREVGeometry[tProfileName,"scaleBottomObjectRef"] of control id tControlId is "card" then
                  --scale Bottom in absolute reference to the card
                  put the cREVGeometry[tProfileName,"scaleBottomDistance"] of control id tControlId into tDistance
                  if the cREVGeometry[tProfileName,"scaleBottomObjectSide"] of control id tControlId is "top" then
                     put the top of this cd + tDistance into item 4 of tObjectRect
                  else if the cREVGeometry[tProfileName,"scaleBottomObjectSide"] of control id tControlId is "bottom" then
                     put tCardHeight + tDistance into item 4 of tObjectRect
                  else  if the cREVGeometry[tProfileName,"scaleBottomObjectSide"] of control id tControlId is "center" then
                     put item 2 of tCardLoc + tDistance into item 4 of tObjectRect
                  end if
               else
                  --scale Bottom in absolute reference to an object
                  put the cREVGeometry[tProfileName,"scaleBottomDistance"] of control id tControlId into tDistance
                  put the cREVGeometry[tProfileName,"scaleBottomObjectRef"] of control id tControlId into tObjectRef
                  put tIDArray[tObjectRef] into tObjectRef
                  if the cREVGeometry[tProfileName,"scaleBottomObjectSide"] of control id tControlId is "top" then
                     put the top of control id tObjectRef + tDistance into item 4 of tObjectRect
                  else if the cREVGeometry[tProfileName,"scaleBottomObjectSide"] of control id tControlId is "bottom" then
                     put the bottom of control id tObjectRef + tDistance into item 4 of tObjectRect
                  else  if the cREVGeometry[tProfileName,"scaleBottomObjectSide"] of control id tControlId is "center" then
                     put item 2 of the loc of control id tObjectRef + tDistance into item 4 of tObjectRect
                  end if
               end if
            else
               --relative scaling of Bottom
               if the cREVGeometry[tProfileName,"scaleBottomObjectRef"] of control id tControlId is "card" then
                  --scale Bottom in relative reference to the card
                  put the cREVGeometry[tProfileName,"scaleBottomDistance"] of control id tControlId into tDistance
                  --we assume relative to bottom
                  put round(tDistance * tCardHeight) into tDistance
                  put tCardHeight+tDistance into item 4 of tObjectRect
               else
                  put the cREVGeometry[tProfileName,"scaleBottomDistance"] of control id tControlId into tDistance
                  put the cREVGeometry[tProfileName,"scaleBottomObjectRef"] of control id tControlId into tObjectRef
                  put tIDArray[tObjectRef] into tObjectRef
                  put round(tDistance * tCardHeight) into tDistance
                  
                  # TH-2008-07-07 :: Bug 6391, similar problem to elsewhere, should be making use of the
                  # top not item item 2 of the loc
                  --put item 2 of the loc of control id tObjectRef+tDistance into item 4 of tObjectRect
                  put item 4 of the rect of control id tObjectRef+tDistance into item 4 of tObjectRect
               end if
            end if
         end if
         --END OF VERTICAL
      end if
      --CHECK WHETHER TO SHOW SCROLLBARS
      if word 1 of the name of control id tControlId is "field" then
         if the cREVGeometry[tProfileName,"showHScroll"] of control id tControlId is true then
            --show H scrollbar if text too small
            
            local tWidth
            put the formattedWidth of control id tControlId into tWidth
            if item 3 of tObjectRect - item 1 of tObjectRect < tWidth then
               set the hScrollbar of control id tControlId to true
            else
               set the hScrollbar of control id tControlId to false
            end if
         end if
         if the cREVGeometry[tProfileName,"showVScroll"] of control id tControlId is true then
            --show V scrollbar if text too small
            
            local tHeight
            put the formattedHeight of control id tControlId into tHeight
            if item 4 of tObjectRect - item 2 of tObjectRect < tHeight then
               set the vScrollbar of control id tControlId to true
            else
               set the vScrollbar of control id tControlId to false
            end if
         end if
      end if
      --end SCROLLBAR check
      ----CONSTRAINTS TAB - MINIMUM AND MAXIMUM SIZES, PLACEMENT, ETC
      if the cREVGeometry[tProfileName,"dontClipText"] of control id tControlId is true then
         --don't CLIP text - all object types
         local tPassH
         put false into tPassH
         
         local tPassV
         put false into tPassV
         if word 1 of the name of control id tControlId is "field" then
            if the hScrollbar of control id tControlId is true then
               put true into tPassH
            end if
            if the vScrollBar of control id tControlId is true then
               put true into tPassV
            end if
         end if
         put the formattedWidth of control id tControlId into tWidth
         put the formattedHeight of control id tControlId into tHeight
         if not tPassH and item 3 of tObjectRect - item 1 of tObjectRect < tWidth then
            --object is too narrow, make it wider
            put revSetWidth(tObjectRect,tWidth) into tObjectRect
         end if
         if not tPassV and item 4 of tObjectRect - item 2 of tObjectRect < tHeight then
            --object is too short, make it taller
            put revSetHeight(tObjectRect,tHeight) into tObjectRect
         end if
      end if
      --check min/max WIDTH and HEIGHT
      if the cREVGeometry[tProfileName,"minWidth"] of control id tControlId is not empty then
         --minimum width check
         put the cREVGeometry[tProfileName,"minWidth"] of control id tControlId into tWidth
         if item 3 of tObjectRect - item 1 of tObjectRect < tWidth then
            put revSetWidth(tObjectRect,tWidth) into tObjectRect
         end if
      end if
      if the cREVGeometry[tProfileName,"minHeight"] of control id tControlId is not empty then
         --minimum height check
         put the cREVGeometry[tProfileName,"minHeight"] of control id tControlId into tHeight
         if item 4 of tObjectRect - item 2 of tObjectRect < tHeight then
            put revSetHeight(tObjectRect,tHeight) into tObjectRect
         end if
      end if
      if the cREVGeometry[tProfileName,"maxwidth"] of control id tControlId is not empty then
         --maximum width check
         put the cREVGeometry[tProfileName,"maxWidth"] of control id tControlId into tWidth
         if item 3 of tObjectRect - item 1 of tObjectRect > tWidth then
            put revSetWidth(tObjectRect,tWidth) into tObjectRect
         end if
      end if
      if the cREVGeometry[tProfileName,"maxHeight"] of control id tControlId is not empty then
         --maximum height check
         put the cREVGeometry[tProfileName,"maxHeight"] of control id tControlId into tHeight
         if item 4 of tObjectRect - item 2 of tObjectRect > tHeight then
            put revSetHeight(tObjectRect,tHeight) into tObjectRect
         end if
      end if
      --CHECK edges left,right,top,bottom
      if the cREVGeometry[tProfileName,"minLeft"] of control id tControlId is not empty then
         --check left of object
         
         local tMinLeft
         put the cREVGeometry[tProfileName,"minLeft"] of control id tControlId into tMinLeft
         if item 1 of tObjectRect < tMinLeft then
            --set the rect to move the object right
            put tMinLeft - item 1 of tObjectRect into tDifference
            add tDifference to item 1 of tObjectRect
            add tDifference to item 3 of tObjectRect
         end if
      end if
      if the cREVGeometry[tProfileName,"maxRight"] of control id tControlId is not empty then
         --check right of object
         
         local tMaxRight
         put the cREVGeometry[tProfileName,"maxRight"] of control id tControlId into tMaxRight
         if item 3 of tObjectRect > tMaxRight then
            --set the rect to move the object left
            put item 3 of tObjectRect - tMaxRight into tDifference
            subtract tDifference from item 1 of tObjectRect
            subtract tDifference from item 3 of tObjectRect
         end if
      end if
      if the cREVGeometry[tProfileName,"minTop"] of control id tControlId is not empty then
         --check top of object
         
         local tMinTop
         put the cREVGeometry[tProfileName,"minTop"] of control id tControlId into tMinTop
         if item 2 of tObjectRect < tMinTop then
            --set the rect to move the object down
            put tMinTop - item 2 of tObjectRect into tDifference
            add tDifference to item 2 of tObjectRect
            add tDifference to item 4 of tObjectRect
         end if
      end if
      if the cREVGeometry[tProfileName,"maxBottom"] of control id tControlId is not empty then
         --check bottom of object
         
         local tMaxBottom
         put the cREVGeometry[tProfileName,"maxBottom"] of control id tControlId into tMaxBottom
         if item 4 of tObjectRect > tMaxBottom then
            --set the rect to move the object up
            put item 4 of tObjectRect - tMaxBottom into tDifference
            subtract tDifference from item 2 of tObjectRect
            subtract tDifference from item 4 of tObjectRect
         end if
      end if
      if word 1 of the name of control id tControlId is "field" then
         if the hilitedLines of control id tControlId is not 0 then
            local tRestore
            put the hilitedLines of control id tControlId into tRestore
         end if
      end if
      unlock messages -- km get resizeControl
      set the rect of control id tControlId to tObjectRect
      lock messages
      if tRestore is not empty then
         set the hilitedLines of control id tControlId to tRestore
         put empty into tRestore
      end if
   end repeat
   repeat for each line l in keys(tGroupLockLocation)
      if l is empty then next repeat
      set the lockLocation of control id l to tGroupLockLocation[l]
   end repeat
   if the cREVGeometry["expandStack"] of this card is true then
      local tStackRect
      put the rect of this stack into tStackRect
      put the formattedWidth of this cd + the formattedLeft of this cd into tWidth
      if tCardWidth < tWidth then put item 1 of tStackRect + (tWidth-the cREVGeneral["virtualWidth"] of this card) into item 3 of tStackRect
      put the formattedHeight of this cd + the formattedTop of this cd into tHeight
      if tCardHeight < tHeight then put item 2 of tStackRect + (tHeight-the cREVGeneral["virtualHeight"] of this card) into item 4 of tStackRect
      set the rect of this stack to tStackRect
   end if
   put empty into lOrigWidth
   put empty into lOrigHeight
   --unlock screen
   unlock messages
   set the defaultStack to tDefaultStack
end revUpdateGeometry

function revSetWidth pRect,pWidth
  local tRect
  put pRect into tRect
  put item 1 of tRect + pWidth into item 3 of tRect
  return tRect
end revSetWidth

function revSetHeight pRect,pHeight
  local tRect
  put pRect into tRect
  put item 2 of tRect + pHeight into item 4 of tRect
  return tRect
end revSetHeight

on resizeControl
   global gREVStacksList
   if "development" is in the environment 
   then revUpdateSizeandPosition the long id of the target
   
   global gREVMessageDispatch
   repeat for each line tStack in gREVMessageDispatch["revResizeControl"]
      send "revResizeControl" to this cd of stack tStack
   end repeat
   
   if "cREVGeometryCache" is among the lines of the customPropertySets of this cd then
      set the cREVGeometryCache["order"] of this card to empty
   end if
   
   local tTarget
   put the long id of this cd of stack revTargetStack() into tTarget
   -- KM added executionContexts check to ensure resized using pointer tool not by the new message in 3.5
   if char 1 to 3 of revTargetStack() is not "rev" and "cREVGeometryCache" is among the lines of the customPropertySets of tTarget and the number of lines in the executionContexts is 1 then 
      send "revCacheGeometry true" to tTarget
   end if
   
   # OK-2007-08-10 : Another use of an undeclared global variable was found here meaning that this message would have never been passed
   # Added the declaration in the line below.
   global gREVSuppressMessages
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then 
      pass resizeControl
   end if
end resizeControl

on moveControl
  global gREVMessageDispatch
  
  if "development" is in the environment and the mode of stack "revRestore" is 0
  then revUpdateSizeandPosition the long id of the target
   
  repeat for each line tStack in gREVMessageDispatch["revMoveControl"]
    send "revMoveControl" to this cd of stack tStack
  end repeat
  
  if "cREVGeometryCache" is among the lines of the customPropertySets of this cd then
    set the cREVGeometryCache["order"] of this card to empty
  end if
  
  local tTarget
  put the long id of this cd of stack revTargetStack() into tTarget
  if char 1 to 3 of revTargetStack() is not "rev" and "cREVGeometryCache" is among the lines of the customPropertySets of tTarget \
      then send "revCacheGeometry true" to tTarget
  
  # OK-2007-08-10 : Another use of an undeclared global variable was found here meaning that this message would have never been passed
  # Added the declaration in the line below.
  global gREVSuppressMessages
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then
    pass moveControl
  end if
end moveControl


on revSetGeometryObject pWhich
  put line 1 of pWhich into lGeometryObject
end revSetGeometryObject

local lIDArray

on revCacheGeometry pUpdateSizes
  lock messages
  revCheckGeoDefaultStack
  
  local tNoControls
  put the number of controls into tNoControls
  delete local lIDArray
  
  local tNumber
  put the milliseconds into tNumber
  repeat with i = 1 to tNoControls
    local tAltID
    put the cREVGeneral["revUniqueID"] of control i into tAltID
    
    local tAltIDsList
    if tAltID is empty or tAltID is among the lines of tAltIDsList then
      set the cREVGeneral["revUniqueID"] of control i to tNumber
      add 1 to tNumber
      put tNumber into tAltID
    end if
    put tAltID & cr after tAltIDsList
    put the id of control i into lIDArray[tAltID]
  end repeat
  
  local tProfileName
  repeat with i = 1 to tNoControls
    put the cREVGeneral["profile"] of control i into tProfileName
    if tProfileName is empty or tProfileName is the cREVGeneral["masterName"] of control i then put "Master" into tProfileName
    if the cREVGeometry[tProfileName] of control i is not true then next repeat
    put the long id of control i into lGeometryObject
    if pUpdateSizes then revCalculateGeometryDistances
    
    local tCardRanking
    put the cREVGeometry[tProfilename & comma & "cardRanking"] of control i into tCardRanking
    if tCardRanking > 0 then
      local tStackDependant
      put the cREVGeneral["revUniqueID"] of control i, tCardRanking & return after tStackDependant
    else
      --the tricky part - go through all the 6 sides and follow them up to find out how far away from the card the object is
      local tDistanceCounter
      put 0 into tDistanceCounter
      repeat for each item l in "scaleLeftObjectRef,scaleRightObjectRef,scaleTopObjectRef,scaleBottomObjectRef,moveHObjectRef,moveVObjectRef"
        local tBreakOut
        put 0 into tBreakOut
        
        local tNextObject
        put the cREVGeometry[tProfileName,l] of control i into tNextObject
        if there is a control id lIDArray[tNextObject] then
          put lIDArray[tNextObject] into tNextObject
          repeat
            add 1 to tDistanceCounter
            add 1 to tBreakOut
            --cycle through objects, break out if empty, card, no object found or total number of loops exceeds object count
            local tNextProfileName
            put the cREVGeneral["profile"] of control id tNextObject into tNextProfileName
            if tNextProfileName is empty or tNextProfileName is the cREVGeneral["masterName"] of control i then
              put "Master" into tNextProfileName
            end if
            
            # OK-2007-08-10 : Previously the first clause of this if condition refered to the variable "tNextProfileNames", which was
            # undeclared and not refered to anywhere else. I changed this to tNextProfileName
            if tNextProfileName is empty or tNextProfileName is the cREVGeneral["masterName"] of control id tNextObject then
              put "Master" into tNextProfileName
            end if
            put the cREVGeometry[tNextProfileName,l] of control id tNextObject into tNextObject
            if tNextObject is "card" or tNextObject is empty then exit repeat
            if not (there is a control id tNextObject) then exit repeat
            if tBreakOut > tNoControls then exit repeat
          end repeat
        end if
      end repeat
      local tStackDistance
      put the cREVGeneral["revUniqueID"] of control i,tDistanceCounter & return after tStackDistance
    end if
  end repeat
  put empty into lGeometryObject
  delete the last char of tStackDependant
  delete the last char of tStackDistance
  sort lines of tStackDependant numeric descending by item 2 of each
  sort lines of tStackDistance numeric ascending by item 2 of each
  repeat with i = 1 to the number of lines in tStackDependant
    delete item 2 of line i of tStackDependant
  end repeat
  repeat with i = 1 to the number of lines in tStackDistance
    delete item 2 of line i of tStackDistance
  end repeat
  set the cREVGeometrycache["order"] of this card to tStackDependant & return & tStackDistance
  set the cREVGeometryCache["total"] of this card to the number of controls
  set the cREVGeometryCache["stackID"] of this stack to the ID of this stack
  set the customProperties["cREVGeometryCacheIDs"] of this cd to lIDArray
  put empty into lOrigWidth
  put empty into lOrigHeight
  unlock messages
end revCacheGeometry

on revCalculateGeometryDistances pWhichNumber
  if lOrigWidth is empty then put the width of this stack into lOrigWidth
  if lOrigHeight is empty then put the height of this stack into lOrigHeight
  repeat for each item tEdge in "left,right,top,bottom"
    if revGetGeometry("scale" & tEdge & "Absolute") is false then
      --we need to calculate the distance as it stands as a percentage of card width or height
      get revCalculateScaleDistance(tEdge)
      if tEdge is "left" or tEdge is "right" then
        --its left right percent
        local tValue
        put it / (lOrigWidth + (the cREVGeneral["virtualWidth"] of this card of this stack)) into tValue
        revCleanSetGeometry "scale" & tEdge & "Distance", tValue
      else
        put it / (lOrigHeight + (the cREVGeneral["virtualHeight"] of this card of this stack)) into tValue
        revCleanSetGeometry "scale" & tEdge & "Distance", tValue
      end if
    else
      --calculate distances absolutely rather than as percentage
      get revCalculateScaleDistance(tEdge)
      revCleanSetGeometry "scale" & tEdge & "Distance", it
    end if
  end repeat
  repeat for each item tDimension in "h,v"
    if revGetGeometry("move" & tDimension & "Absolute") is false then
      --we need to calculate the distance as it stands as a percentage of card width or height
      get revCalculateMoveDistance(tDimension)
      if it is false then next repeat
      if tDimension is "h" then
        --its left right percent
        put it / (lOrigWidth + (the cREVGeneral["virtualWidth"] of this card of this stack)) into tValue
        revCleanSetGeometry "move" & tDimension & "Distance", tValue
      else
        put it / (lOrigHeight + (the cREVGeneral["virtualHeight"] of this card of this stack)) into tValue
        revCleanSetGeometry "move" & tDimension & "Distance", tValue
      end if
    else
      --calculate distances absolutely rather than as percentage
      get revCalculateMoveDistance(tDimension)
      revCleanSetGeometry "move" & tDimension & "Distance", it
    end if
  end repeat
end revCalculateGeometryDistances

on revCalcLIDArray
  delete local lIDArray
  repeat with i = 1 to the number of controls
    local tAltID
    put the cREVGeneral["revUniqueID"] of control i into tAltID
    put the id of control i into lIDArray[tAltID]
  end repeat
end revCalcLIDArray

function revCalculateMoveDistance pWhichSide
  local tObjectEdge
  if pWhichSide is not empty then
    put pWhichSide into tObjectEdge
  else 
    put "center" into tObjectEdge
  end if
  
  local tTargetEdge
  put revGetGeometry("move"&tObjectEdge&"ObjectSide") into tTargetEdge
  revCheckGeoDefaultStack
  if lOrigWidth is empty then put the width of this card into lOrigWidth
  if lOrigHeight is empty then put the height of this card into lOrigHeight
  
  local tCardWidth
  put lOrigWidth + the cREVGeneral["virtualWidth"] of this card into tCardWidth
  
  local tCardHeight
  put lOrigHeight + the cREVGeneral["virtualHeight"] of this card + the scroll of this stack into tCardHeight
  if revGetGeometry("move"&tObjectEdge& "ObjectRef") is "card" then
    local tObjectLoc
    put trunc(tCardWidth / 2), trunc(tCardHeight / 2) into tObjectLoc
    local tObjectRect
    put 0, 0, tCardWidth, tCardHeight into tObjectRect
  else
    local tObject
    put revGetGeometry("move"&tObjectEdge&"ObjectRef") into tObject
    revCalcLIDArray
    put lIDArray[tObject] into tObject
    if tObject is empty then return false
    put the loc of control id tObject into tObjectLoc
    put the rect of control id tObject into tObjectRect
  end if
  switch tObjectEdge
  case "H"
    if tTargetEdge is "Center" then return item 1 of the loc of lGeometryObject - item 1 of tObjectLoc
    if tTargetEdge is "Left" then return item 1 of the loc of lGeometryObject - item 1 of tObjectRect
    if tTargetEdge is "Right" then return item 1 of the loc of lGeometryObject - item 3 of tObjectRect
    break
  case "V"
    if tTargetEdge is "Center" then return item 2 of the loc of lGeometryObject - item 2 of tObjectLoc
    if tTargetEdge is "Top" then return item 2 of the loc of lGeometryObject - item 2 of tObjectRect
    if tTargetEdge is "Bottom" then return item 2 of the loc of lGeometryObject - item 4 of tObjectRect
    break
  end switch
end revCalculateMoveDistance

on revCheckGeoDefaultStack
  local tDefaultStack
  put revTargetStack() into tDefaultStack
  if word 1 of tDefaultStack is "revPropertyPalette" and char 1 to 11 of the short name of this cd is "revGeometry" then
    if revPaletteObjects() is not empty then set the defaultStack to revTargetStack(line 1 of (revPaletteObjects()))
    else set the defaultStack to the topStack
  end if
end revCheckGeoDefaultStack

function revCalculateScaleDistance pWhichside
  revCheckGeoDefaultStack
  if lOrigWidth is empty then put the width of this card into lOrigWidth
  if lOrigHeight is empty then put the height of this card into lOrigHeight
  local tCardWidth
  put lOrigWidth + the cREVGeneral["virtualWidth"] of this card into tCardWidth
  
  local tCardHeight
  put lOrigHeight + the cREVGeneral["virtualHeight"] of this card + the scroll of this stack into tCardHeight
  if pWhichSide is not empty then
    local tObjectEdge
    put pWhichSide into tObjectEdge
    local tTargetEdge
    put revGetGeometry("scale" & tObjectEdge & "ObjectSide") into tTargetEdge
    if tTargetEdge is empty then
      put "center" into tTargetEdge
    end if
  else
    put the cREVWhichSelected of this cd of stack "revGeometryManager" into tObjectEdge
    put revGetGeometry("scale"&tObjectEdge&"ObjectSide") into tTargetEdge
  end if
  if revGetGeometry("scale"&tObjectEdge& "ObjectRef") is "card" then
    local tObjectLoc
    put trunc(tCardWidth / 2), trunc(tCardHeight / 2) into tObjectLoc
    
    local tObjectRect
    put 0, 0, tCardWidth, tCardHeight into tObjectRect
  else
    local tObject
    put revGetGeometry("scale" & tObjectEdge & "ObjectRef") into tObject
    revCalcLIDArray
    put lIDArray[tObject] into tObject
    if tObject is empty then return empty
    put the loc of control id tObject into tObjectLoc
    put the rect of control id tObject into tObjectRect
  end if
  switch tObjectEdge
  case "left"
    if tTargetEdge is "Center" then return the left of lGeometryObject - item 1 of tObjectLoc
    if tTargetEdge is "Left" then return the left of lGeometryObject - item 1 of tObjectRect
    if tTargetEdge is "Right" then return the left of lGeometryObject - item 3 of tObjectRect
    break
  case "right"
    if tTargetEdge is "Center" then return the right of lGeometryObject - item 1 of tObjectLoc
    if tTargetEdge is "Left" then return the right of lGeometryObject - item 1 of tObjectRect
    if tTargetEdge is "Right" then return the right of lGeometryObject - item 3 of tObjectRect
    break
  case "top"
    if tTargetEdge is "Center" then return the top of lGeometryObject - item 2 of tObjectLoc
    if tTargetEdge is "Top" then return the top of lGeometryObject - item 2 of tObjectRect
    if tTargetEdge is "Bottom" then return the top of lGeometryObject - item 4 of tObjectRect
    break
  case "bottom"
    --answer lGeometryObject & cr & tTargetEdge & cr & tObjectRect & cr & the bottom of lGeometryObject & cr & tObjectLoc
    if tTargetEdge is "Center" then return the bottom of lGeometryObject - item 2 of tObjectLoc
    if tTargetEdge is "Top" then return the bottom of lGeometryObject - item 2 of tObjectRect
    if tTargetEdge is "Bottom" then return the bottom of lGeometryObject - item 4 of tObjectRect
    break
  end switch
end revCalculateScaleDistance

on revCleanSetGeometry pWhichProperty, pWhatValue
  --a cut down version to avoid recursion
  --determine the profile to edit
  local tProfileName
  put the cREVGeneral["profile"] of lGeometryObject into tProfileName
  if tProfileName is empty or tProfileName is the cREVGeneral["masterName"] of lGeometryObject then
    put "Master" into tProfileName
  end if
  if "objectRef" is in pWhichProperty and pWhatValue is not "card" and there is a control pWhatValue then
    put the cREVGeneral["revUniqueID"] of control pWhatValue into pWhatValue
  end if
  --set property
  set the cREVGeometry[tProfileName,pWhichProperty] of lGeometryObject to pWhatValue
end revCleanSetGeometry

on revSetGeometry pWhichProperty,pWhatValue
  revCheckGeoDefaultStack
  --empty card cache
  set the cREVGeometryCache["order"] of this card of this stack to empty
  set the cREVGeometryCache["total"] of this card of this stack to empty
  set the customKeys["cREVGeometryCacheIDs"] of this card of this stack to empty
  if "revCacheGeometry" is not in the pendingMessages then
    send "revCacheGeometry true" to this cd of this stack in 50 milliseconds
  end if
  --determine the profile to edit 
  local tProfileName
  put the cREVGeneral["profile"] of lGeometryObject into tProfileName
  if tProfileName is empty or tProfileName is the cREVGeneral["masterName"] of lGeometryObject then
    put "Master" into tProfileName
  end if
  --check that some geo has been set
  set the cREVGeometry[tProfileName] of lGeometryObject to true
  --update expected rect
 
  set the cREVGeometry[tProfileName & comma & "expectedRect"] of lGeometryObject to the rect of lGeometryObject
  --check if property contains "objectRef" and replace with milliseconds ID if so
  if "objectRef" is in pWhichProperty and pWhatValue is not "card" and there is a control pWhatValue then
    put the cREVGeneral["revUniqueID"] of control pWhatValue into pWhatValue
  end if
  --set property
  set the cREVGeometry[tProfileName,pWhichProperty] of lGeometryObject to pWhatValue
  --calculate relative distances if applicable
  # if "absolute" is in pWhichProperty and pWhatValue is false then revCalculateGeometryDistances
  revCalculateGeometryCardRank
end revSetGeometry

function revGetGeometry pWhichValue
  local tProfileName
  put the cREVGeneral["profile"] of lGeometryObject into tProfileName
  if tProfileName is empty or tProfileName is the cREVGeneral["masterName"] of lGeometryObject then put "Master" into tProfileName
  --check that some geo has been set
  return the cREVGeometry[tProfileName,pWhichValue] of lGeometryObject
end revGetGeometry

on revCalculateGeometryCardRank
  local tCount
  put 0 into tCount
  repeat for each item tEdge in "left,right,top,bottom"
    if revGetGeometry("scale" & tEdge & "ObjectRef") is "card" and revGetGeometry("scale" & tEdge) is true then
      add 1 to tCount
    end if
  end repeat
  if revGetGeometry("moveHObjectRef") is "card" and revGetGeometry("moveH") then add 1 to tCount
  if revGetGeometry("moveVObjectRef") is "card" and revGetGeometry("moveV") then add 1 to tCount
  revCleanSetGeometry "cardRanking",tCount
end revCalculateGeometryCardRank

# Parameters
#   pWhich : The unique id of an object as used by the geometry manager.
# Returns
#   The name of the object that matches the specified unique id. Returns pWhich if
#   pWhich contains only whitespace. Note: if pWhich is non-empty, but no object is found
#   that matches it, pWhich will be returned. This is due to the cREVGeneral getprop setting 
#   the result, and revUniqueIDToName failing to reset it.
function revUniqueIDToName pWhich
  local tReturn
  put the defaultStack into tReturn
  if the number of words in pWhich is 0 then
    return pWhich
  end if
  
  set the defaultStack to revTargetStack(line 1 of (the selObj))
  repeat with i = 1 to the number of controls
    if the cREVGeneral["revUniqueID"] of control i is pWhich then
      local tName
      put the name of control i into tName
      set the defaultStack to tReturn
      return tName
    end if
  end repeat
end revUniqueIDToName
