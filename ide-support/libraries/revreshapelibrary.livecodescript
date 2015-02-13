script "revreshapelibrary"

on revLoadLibrary
   insert the script of me into front
end revLoadLibrary

on revUnloadLibrary
   remove the script of me from front
end revUnloadLibrary

on revNewTool
   if the long id of me is among the lines of the frontScripts then
      select empty
      revSetMarkers true
      remove script of me from front
   end if
end revNewTool

on revReshapeGraphic
   if the long id of me is not among the lines of the frontScripts then
      choose pointer tool
      insert script of me into front
      put empty into gREVObjectsList
      revSetMarkers
   else
      revSetMarkers true
      remove script of me from front
   end if
end revReshapeGraphic

on mouseDown
  local tDistance
  if word 1 of the name of the target is not "graphic" then pass mouseDown
  if the tool is not "pointer tool" then
    revSetMarkers
    remove the script of me from front
    pass mouseDown
  end if
  if the style of the target is not "polygon" then
    if the style of the target is "curve" or the style of the target is "line" then
      set the style of the target to "polygon"
    else
      beep
      pass mouseDown
    end if
  end if
  if not the selected of the target then select the target
  put the mouseLoc into tClickedLoc
  put the points of the target into tOriginalPoints
  put item 1 of tClickedLoc into tClickedX
  put item 2 of tClickedLoc into tClickedY
  repeat with i = 1 to the number of lines in tOriginalPoints
    put item 1 of(line i of tOriginalPoints) into tCordX
    put item 2 of(line i of tOriginalPoints) into tCordY
    put round(sqrt(((tCordX-tClickedX)^2)+((tCordY-tClickedY)^2))) & "," into item i of tDistance
  end repeat
  put min(tDistance) into tMinDistance
  if tMinDistance > 5 then
    pass mouseDown
  end if
  repeat with i = 1 to the num of items in tDistance
    if item i of tDistance is tMinDistance then
      put i into tPoint
      exit repeat
    end if
  end repeat
  if the optionKey is "down" then
    --work out which closer to when mouse first pressed
    put item 1 of (line tPoint-1 of tOriginalPoints) into tCordX
    put item 2 of (line tPoint-1 of tOriginalPoints) into tCordY
    put round(sqrt(((tCordX-tClickedX)^2)+((tCordY-tClickedY)^2))) into tPrevDistance
    put item 1 of (line tPoint+1 of tOriginalPoints) into tCordX
    put item 2 of (line tPoint+1 of tOriginalPoints) into tCordY
    put round(sqrt(((tCordX-tClickedX)^2)+((tCordY-tClickedY)^2))) into tAfterDistance
    --work out which closer to now
    wait 100 milliseconds
    put the mouseH into tClickedX
    put the mouseV into tClickedY
    put item 1 of (line tPoint-1 of tOriginalPoints) into tCordX
    put item 2 of (line tPoint-1 of tOriginalPoints) into tCordY
    put round(sqrt(((tCordX-tClickedX)^2)+((tCordY-tClickedY)^2))) into tPrevDistance2
    put item 1 of (line tPoint+1 of tOriginalPoints) into tCordX
    put item 2 of (line tPoint+1 of tOriginalPoints) into tCordY
    put round(sqrt(((tCordX-tClickedX)^2)+((tCordY-tClickedY)^2))) into tAfterDistance2
    --take the two and work out which one has been *moved* closer to (not which one *is* closer)
    put tPrevDistance2 - tPrevDistance into tEarlierPointMoved
    put tAfterDistance2 - tAfterDistance into tAfterPointMoved
    --dependant on which point has been moved closer too, insert the point either before or after the
    --handle selected
    if tEarlierPointMoved < tAfterPointMoved then
      put cr after line tPoint-1 of tOriginalPoints
      put tClickedLoc into line tPoint of tOriginalPoints
    else
      put cr after line tPoint of tOriginalPoints
      put tClickedLoc into line tPoint+1 of tOriginalPoints
      add 1 to tPoint
    end if
    set the points of the target to tOriginalPoints
  end if
  if the commandKey is "down" then
    --delete the point
    if the number of lines in tOriginalPoints > 1 then
      if the shiftKey is "down" then put cr into line tPoint of tOriginalPoints
      else delete line tPoint of tOriginalPoints
      set the points of the target to tOriginalPoints
      --set the revPropertyName of stack "revProfileEditor" to "points"
      --set the revPropertyContents of stack "revProfileEditor" to tOriginalPoints
    else
      beep
    end if
    pass mouseDown
  end if
  repeat while the mouse is down
    put the mouseloc into line tPoint of tOriginalPoints
    set points of the target to tOriginalPoints
  end repeat
  --set the revPropertyName of stack "revProfileEditor" to "points"
  --set the revPropertyContents of stack "revProfileEditor" to tOriginalPoints
  --pass mouseDown
end mouseDown

on selectedObjectChanged
  --if the hiliteIcon of btn "reshape" of stack "revTools" is "200042" then send "revNewTool" to group 1 of stack "revTools"
  --send "revNewTool" to group 1 of stack "revTools"
  put the selobj into tSelobj
  put true into tReshapableGrc
  repeat for each line l in tSelobj
    if word 1 of l is "graphic" and the style of l is among the items of "curve,line,polygon"
    then next repeat
    else
      put false into tReshapableGrc
      exit repeat
    end if
  end repeat
  if tReshapableGrc then
    revSetMarkers
  else
    send "revSetMarkers true" to me
    remove the script of me from front
  end if
  pass selectedObjectChanged
end selectedObjectChanged

on revSetMarkers pRemove
  global gREVObjectsList
  put (the selObj) into tObjectsList
  repeat for each line l in gREVObjectsList
    if not exists(l) then next repeat
    if the revOriginalMarkerFilled of l is not empty then set the markerFilled of l to the revOriginalMarkerFilled of l
    if the revOriginalMarkerPoints of l is not empty then set the markerPoints of l to the revOriginalMarkerPoints of l
    if the revOriginalMarker of l is not empty then set the markerDrawn of l to the revOriginalMarker of l
    if the revOriginalMarkerColor of l is not empty then set the borderColor of l to the revOriginalMarkerColor of l
    if the revOriginalMarkerOutLine of l is not empty then set the markerColor of l to the revOriginalMarkerOutLine of l
    put the customKeys of l into tCustomKeysList
    set the wholeMatches to true
    get lineOffset("revOriginalMarkerFilled",tCustomKeysList)
    delete line it of tCustomKeysList
    get lineOffset("revOriginalMarkerOutLine",tCustomKeysList)
    delete line it of tCustomKeysList
    get lineOffset("revOriginalMarkerColor",tCustomKeysList)
    delete line it of tCustomKeysList
    get lineOffset("revOriginalMarkerPoints",tCustomKeysList)
    delete line it of tCustomKeysList
    get lineOffset("revOriginalMarker",tCustomKeysList)
    delete line it of tCustomKeysList
    set the customKeys of l to tCustomkeysList
  end repeat
  put empty into gREVObjectsList
  if pRemove is not true then
    repeat for each line l in tObjectsList
      if word 1 of l is not "graphic" then
        lock messages
        set the selected of l to false
        unlock messages
        next repeat
      end if
      if the style of l is not "polygon" then
        if the style of l is "curve" or the style of l is "line" then
          set the style of l to "polygon"
        else
          lock messages
          set the selected of l to false
          unlock messages
          next repeat
        end if
      end if
      if the revOriginalMarkerFilled of l is empty then set the revOriginalMarkerFilled of l to the markerFilled of l
      if the revOriginalMarkerColor of l is empty then set the revOriginalMarkerColor of l to the borderColor of l
      if the revOriginalMarkerOutLine of l is empty then set the revOriginalMarkerOutLine of l to the markerColor of l
      if the revOriginalMarker of l is empty then set the revOriginalMarker of l to the markerDrawn of l
      if the revOriginalMarkerPoints of l is empty then set the revOriginalMarkerPoints of l to the markerPoints of l
      set the markerFilled of l to true
      set the borderColor of l to the accentColor
      set the markerColor of l to black
      set markerDrawn of l to true
      set the markerPoints of l to the revMarkerPoints of stack "revReshapeLibrary"
      --set markerPoints of l to -3,-3&cr& 3,-3&cr& 3,3&cr& -3,3&cr& -3,-3
      put l & cr after gREVObjectsList
    end repeat
  end if
end revSetMarkers
