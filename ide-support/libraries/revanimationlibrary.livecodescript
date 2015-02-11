script "revanimationlibrary"

on revLoadLibrary
   insert the script of me into back
end revLoadLibrary

on revUnloadLibrary
   remove the script of me from back
end revUnloadLibrary

--------Animation Library--------
local lOverRide

on revGoToFramePaused pWhichAnimation, pWhich
  global gREVCurrentAnimation
  lock messages
  if pWhichAnimation is not empty then put pWhichAnimation into tName
  else put gREVCurrentAnimation into tName
  if tName is empty then revAnimationError "no name"
  if pWhich is not a number and pWhich is not empty then
    --look it up to see if you can find the frame
    put the cREVAnimation[tName,frameNames] of this cd & cr into tKeyFrames
    put lineOffset(comma&pWhich&cr,tKeyFrames) into tLineNo
    if tLineNo is 0 then revAnimationError "frame named" && pWhich && "not listed"
    put item 1 of line tLineNo of tKeyFrames into pWhich
  end if
  if pWhich > the cREVAnimation[tName,calculatedTotal] of this cd then revAnimationError "frame" && pWhich && "not found"
  if "cached" is in the cREVAnimation[tName,status] of this cd then 
    put pWhich into tFrameNo
    --insert cached frame lookup code here
    --*
     
    exit revGoToFramePaused
  end if
  if lOverRide is not true then
    put the cREVAnimation[tName,status] of this cd into tPrevStatus
    put "paused" into line 1 of tPrevStatus
    set the cREVAnimation[tName,status] of this cd to tPrevStatus
  end if
  put pWhich into tFrameNo
  put the cREVAnimation[tName,currentFrame] of this cd into tPrevframe
  set the cREVAnimation[tName,currentFrame] of this cd to tFrameNo
  put tFrameNo & comma into tFrameLookUp
  --move animation to tFrameNo
  repeat for each line l in the cREVAnimation[tName,objectsList] of this cd
    put empty into tFinalProps
    --check to send any messages
    put the cREVAnimation[tName,tFrameNo,messages] of l into tMessages
    put the cREVAnimation[tName,tPrevframe,messages] of l into tPrevMessages
    put the cREVAnimation[tName,tFrameNo,playState] of l into tPlayState
    if line 3 of tPrevMessages is not empty then send (line 3 of tPrevMessages) to l
    if line 1 of tMessages is not empty then send (line 1 of tMessages) to l
    put the cREVAnimation[tName,keyFrames] of l into tKeyFrames
    if lineOffset(tFrameLookUp,tKeyFrames) is not 0 then
      --its a key frame for this object, set the properties
      put the customProperties[(cREVAnimation&tName&tFrameNo)] of l into tTempProps
      set the properties of l to tTempProps
      if line 2 of tMessages is not empty then
        send (line 2 of tMessages) to l
      end if
      if line 4 of tPrevMessages is not empty then
        send (line 4 of tPrevMessages) to l
      end if
      if tPlayState is "play" then 
        start playing l
      else if tPlayState is "stop" then 
        stop playing l
      end if
    else
      --calculate tweening
      --which frame are we between
      repeat for each line k in tKeyFrames
        if item 1 of k < tFrameNo then put item 1 of k into tBefore
        if item 1 of k > tFrameNo then 
          put item 1 of k into tAfter
          exit repeat
        end if
      end repeat
      if tAfter is empty then
        --go as a keyframe to the last keyframe
        put the customProperties[(cREVAnimation&tName&tBefore)] of l into tTempProps
        set the properties of l to tTempProps
      else
        put the customProperties[(cREVAnimation&tName&tBefore)] of l into tPropsBefore
        put the customProperties[(cREVAnimation&tName&tAfter)] of l into tPropsAfter
        put keys(tPropsBefore) into tPropertiesList
        put the customProperties[cREVAnimation] of l into tTemplookup
        --calculate the tweening percentage, and acceleration if any
        -- there is a bug in the MC engine here, remember to report
        put tName,tBefore,acceleration into tLookUp
        put tTempLookUp[tLookUp] into tAccel
        if tAccel is empty then
          put tAfter - tBefore into tTotal
          put tFrameNo - tBefore into tCurrent
          put tCurrent/tTotal into tPercentage
        else
          --acceleration stuff
          if tAccel < 1 then
            --deccelerate
            put tAfter - tBefore into tTotal
            put tFrameNo - tBefore into tCurrent
            put abs(tAccel) into tAccel
            put (1-(1-(tCurrent/tTotal)^(tAccel/tTotal))^(tTotal/tAccel)) into tPercentage
            --put tAccel & cr & tPercentage*100 --debug
          else
            --accelerate
            put tAfter - tBefore into tTotal
            put tFrameNo - tBefore into tCurrent
            put (1-(1-(tCurrent/tTotal))^(tAccel/tTotal))^(tTotal/tAccel) into tPercentage
            --put tAccel & cr & tPercentage*100 --debug
          end if
        end if
        --now do the matches and build the final array for this object
        set the caseSensitive to true
        repeat for each line p in tPropertiesList
          if p is among the items of "rect,colors,points,hScroll,vScroll,patterns,icon,hiliteIcon,margins,tabStops,borderWidth,lineSize,textSize,textHeight,layer" then
            put true into tNumberTest
          else
            put tPropsBefore[p] into tNumberTest
            put tPropsAfter[p] after tNumberTest
            replace comma with 0 in tNumberTest
            replace cr with 0 in tNumberTest
            if tNumberTest is a number then put true into tNumberTest else put false into tNumberTest
          end if
          if tPropsBefore[p] is not tPropsAfter[p] and the number of items in tPropsBefore[p] is the number of items in tPropsAfter[p] and tNumberTest then
            --tween to a percentage
            put empty into tFinalOutput
            put 0 into tLineNo
            repeat for each line c in tPropsBefore[p]
              add 1 to tLineNo
              put line tLineNo of tPropsAfter[p] into c2
              put empty into tFinalValue
              repeat with i = 1 to the number of items in c
                put item i of c into tBeforeValue
                put item i of c2 into tAfterValue
                put tAfterValue - tBeforeValue into tDifference
                put round(tDifference * tPercentage) + tBeforeValue into item (the number of items in tFinalValue+1) of tFinalValue
              end repeat
              put tFinalValue & cr after tFinalOutput
            end repeat
            delete last char of tFinalOutput
            put tFinalOutput into tFinalProps[p]
          else
            --set as fixed
            put tPropsBefore[p] into tFinalProps[p]
          end if
        end repeat
        set the properties of l to tFinalProps
        if line 2 of tMessages is not empty then send (line 2 of tMessages) to l
        if line 4 of tPrevMessages is not empty then send (line 4 of tPrevMessages) to l
      end if
    end if
  end repeat
  if lOverride is not true then
    revUpdateAnimationPalette tFrameNo
  end if
  unlock messages
end revGoToFramePaused

on revUpdateAnimationPalette pWhichframe
  if there is a stack "revAnimation" and the mode of stack "revAnimation" is not 0 then
    put pWhichFrame into tFrameNo
    --updates the animation palette itself
    put tFrameNo into fld "current frame" of stack "revAnimation"
    lock screen
    put the hScroll of group "frame graphics" of stack "revAnimation" into tPrev
    set the hScroll of group "frame graphics" of stack "revAnimation" to 0
    set the left of grc "frame1" of stack "revAnimation" to (the left of fld "timeLine" of stack "revAnimation" + 1 + tFrameNo * 10)
    set the left of grc "frame2" of stack "revAnimation" to (the left of fld "timeLine" of stack "revAnimation" - 9 + tFrameNo * 10)
    set the hScroll of group "frame graphics" of stack "revAnimation" to tPrev
    if the left of grc "frame1" of stack "revAnimation" < the left of fld "timeLine" of stack "revAnimation" then
      put the left of fld "timeLine" of stack "revAnimation" - the left of grc "frame1" of stack "revAnimation" into tDistanceOff
      add 10 to tDistanceOff
      set the hScroll of fld "timeLine" of stack "revAnimation" to tPrev - tDistanceOff
      set the hScroll of group "frame graphics" of stack "revAnimation" to tPrev - tDistanceOff
      set the hscroll of fld "frame markers" of stack "revAnimation" to tPrev - tDistanceOff
      set the hscroll of fld "frame numbers" of stack "revAnimation" to tPrev - tDistanceOff
    else if the left of grc "frame1" of stack "revAnimation" > the right of fld "timeLine" of stack "revAnimation" then
      put the left of grc "frame1" of stack "revAnimation" - the right of fld "timeLine" of stack "revAnimation" into tDistanceOff
      add 20 to tDistanceOff
      set the hScroll of fld "timeLine" of stack "revAnimation" to tPrev + tDistanceOff
      set the hScroll of group "frame graphics" of stack "revAnimation" to tPrev + tDistanceOff
      set the hscroll of fld "frame markers" of stack "revAnimation" to tPrev + tDistanceOff
      set the hscroll of fld "frame numbers" of stack "revAnimation" to tPrev + tDistanceOff
    end if
    unlock screen
    if "revCheckPalette" is not in the pendingMessages then send "revCheckPalette" to cd 1 of stack "revAnimation" in 250 milliseconds
    send "revStartTracking" to fld "timeline" of stack "revAnimation" --don't add keyframe
  end if
end revUpdateAnimationPalette

on revPlayAnimation pWhichAnimation, pStart, pEnd
  lock messages
  if the lockScreen then unlock screen
  if pStart is not a number and pStart is not empty then
    --look it up to see if you can find the frame
    put the cREVAnimation[pWhichAnimation,frameNames] of this cd & cr into tKeyFrames
    put lineOffset(comma&pStart&cr,tKeyFrames) into tLineNo
    if tLineNo is 0 then revAnimationError "frame named" && pStart && "not listed"
    put item 1 of line tLineNo of tKeyFrames into pStart
  end if
  if pEnd is not a number and pEnd is not empty then
    --look it up to see if you can find the frame
    put the cREVAnimation[pWhichAnimation,frameNames] of this cd & cr into tKeyFrames
    put lineOffset(comma&pEnd&cr,tKeyFrames) into tLineNo
    if tLineNo is 0 then revAnimationError "frame named" && pEnd && "not listed"
    put item 1 of line tLineNo of tKeyFrames into pEnd
  end if
  if pStart is empty then put 1 into pStart
  if pEnd is empty then put the cREVAnimation[pWhichAnimation,"calculatedTotal"] of this cd into pEnd
  put true into lOverRide
  put pStart into tCurrent
  put pEnd into tTotal
  put the cREVAnimation[pWhichAnimation,"frameRate"] of this cd into tRate
  set the cREVAnimation[pWhichAnimation,status] of this cd to "playing"
  put 1000 / tRate into tWait
  put 1 into tIncrement
  put the milliseconds into tStart
  if pStart <= pEnd then
    repeat with i = tCurrent to tTotal
      if i is tTotal then put false into lOverRide
      revGoToFramePaused pWhichAnimation,i
      if the cREVAnimation[pWhichAnimation,status] of this cd is "paused" then 
        put false into lOverride
        revUpdateAnimationPalette i
        exit repeat
      end if
      add 1 to tIncrement
      repeat
        if the milliseconds >= tStart+(tIncrement*tWait) then exit repeat
      end repeat
    end repeat
  else
    repeat with i = tCurrent down to tTotal
      if i is tTotal then put false into lOverRide
      revGoToFramePaused pWhichAnimation,i
      if the cREVAnimation[pWhichAnimation,status] of this cd is "paused" then
        put false into lOverride        
        revUpdateAnimationPalette i
        exit repeat
      end if
      add 1 to tIncrement
      repeat
        if the milliseconds >= tStart+(tIncrement*tWait) then exit repeat
      end repeat
    end repeat
  end if
  unlock messages
end revPlayAnimation

on revStopAnimation pWhichAnimation
  set the cREVAnimation[pWhichAnimation,status] of this cd to "paused"
end revStopAnimation

--on revPreLoadAnimation pWhichAnimation
--  global gREVCurrentAnimation
--  if pWhichAnimation is not empty then put pWhichAnimation into tName
--  else put gREVCurrentAnimation into tName
--  if tName is empty then revAnimationError "no name"
--  if "cached" is in the cREVAnimation[tName,status] of this cd then
    --its already cached, exit
--    exit revPreLoadAnimation
--  end if
--  put the cREVAnimation[tName,status] of this cd into tPrevStatus
--  put "cached" into line 2 of tPrevStatus
--  set the cREVAnimation[tName,status] of this cd to tPrevStatus
  --now cache the animation
  --*
--  
--end revPreLoadAnimation

on revAnimationError pWhich
  if "frame" is in pWhich and "not found" is in pWhich and there is a stack "revAnimation" and the mode of stack "revAnimation" is not 0 then
    put the cREVAnimation[tName,calculatedTotal] of this cd into tFrameNo
    revGoToFramePaused the cCurrentAnimation of this cd of stack "revAnimation",tFrameNo
    put word 2 of pWhich into tFrameNo
    put tFrameNo into fld "current frame" of stack "revAnimation"
    lock screen
    put the hScroll of group "frame graphics" of stack "revAnimation" into tPrev
    set the hScroll of group "frame graphics" of stack "revAnimation" to 0
    set the left of grc "frame1" of stack "revAnimation" to (the left of fld "timeLine" of stack "revAnimation" + 1 + tFrameNo * 10)
    set the left of grc "frame2" of stack "revAnimation" to (the left of fld "timeLine" of stack "revAnimation" - 9 + tFrameNo * 10)
    set the hScroll of group "frame graphics" of stack "revAnimation" to tPrev
    unlock screen
    if "revCheckPalette" is not in the pendingMessages then send "revCheckPalette" to cd 1 of stack "revAnimation" in 250 milliseconds
    send "revStartTracking" to fld "timeline" of stack "revAnimation" --don't add keyframe
  else
   -- put "Animation error" && pWhich
    exit to top
  end if
end revAnimationError
