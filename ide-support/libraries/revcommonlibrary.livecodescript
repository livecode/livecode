script "revcommonlibrary"

on revLoadLibrary
   insert the script of me into back
end revLoadLibrary

on revUnloadLibrary
   remove the script of me from back
end revUnloadLibrary

function revTargetStack pWhich
   local tParse
   if pWhich is not empty and there is a pWhich and word 1 of pWhich is not "the"
   then put the long id of pWhich into tParse
   else put the long id of the target into tParse
   get wordOffset("stack",tParse)
   
   local tStackName
   put word (it) to (it+1) of tParse into tStackName
   put the short name of tStackName into tStackName
   return tStackName
end revTargetstack

# OK-2008-03-24 : Bug 6186
on revGoURL pUrl
   global gREVWebBrowser
   revSetWindowsShellCommand
   
   local tNT
   put revRunningWindowsNT() into tNT
   if "file://" is not in pUrl then
      replace "file:/" with "file:///" in pUrl
   end if
   
   if char 1 to 4 of pUrl is not "file" \
          and char 1 to 4 of pUrl is not "http" \
          and char 1 to 7 of pUrl is not "mailto:" \
          and char 1 to 3 of pUrl is not "ftp" \
          and char 1 to 4 of pUrl is not "feed" then 
      put "http://" before pUrl
   end if
   
   launch url pUrl
end revGoURL

on revGoPDF pFilename
  launch document pFilename
end revGoPDF

# TH-2008-06-27 :: Bug 6521/6432, have seperated revMail into two alternative handlers.  revMailUnicode
# should be ussed by people sending unicode characters, the original revMail can be used otherwise
command revMail pTo, pCC, pSubject, pBody
   revMailUnicode pTo, pCC, uniEncode(pSubject), uniEncode(pBody)
end revMail

command revMailUnicode pTo, pCC, pSubject, pBody
   put uniDecode(pSubject, "UTF8") into pSubject
   put uniDecode(pBody, "UTF8") into pBody
   put urlEncode(pSubject) into pSubject
   put urlEncode(pBody) into pBody
   replace "+" with "%20" in pSubject
   replace "+" with "%20" in pBody
   
   local tURL
   if the paramcount is 1 then
      put "mailto:" & pTo into tURL
   else
      local tCC, tSubject, tBody
      if pCC is not empty then put true into tCC
      if pSubject is not empty then put true into tSubject
      if pBody is not empty then put true into tBody
      
      put "mailto:" & pTo into tURL
      if tCC then
         if (tURL contains "?Subject=") or (tURL contains "?Body=") then
            put "&" & "CC=" & pCC after tURL
         else
            put "?" & "CC=" & pCC after tURL
         end if
      end if
      if tSubject then
         if (tURL contains "?CC=") or (tURL contains "?Body=") then
            put "&" & "Subject=" & pSubject after tURL
         else
            put "?" & "Subject=" & pSubject after tURL
         end if
      end if
      if tBody then
         if (tURL contains "?CC=") or (tURL contains "?Subject=") then
            put "&" & "Body=" & pBody after tURL
         else
            put "?" & "Body=" & pBody after tURL
         end if
      end if
      # TH-2008-08-05 :: Bug fix added if statement 6865
      if tCC or tBody or tSubject then
         put "&Content-Type:text/plain;"&"charset=utf-8" after tURL
      end if
   end if
   
   if the platform is not "win32" then
      launch url tURL
   else
      local tMailApp
      put queryRegistry("HKEY_CLASSES_ROOT\mailto\shell\open\command\") into tMailApp
      replace quote & "%1" & quote with tURL in tMailApp
      replace "%1" with tURL in tMailApp
      replace "%ProgramFiles%" with $ProgramFiles in tMailApp
      if the length of tMailApp > 32767 then
         return "message too long"
      end if
      open process tMailApp for neither
   end if
end revMailUnicode


function revRunningWindowsNT
  if the platform is not "Win32" then return false
  if word 1 of the systemVersion is "Windows"
  then return false
  else return true
end revRunningWindowsNT

on revSetWindowsShellCommand
  if the platform is not "Win32" then exit revSetWindowsShellCommand
  set the hideConsoleWindows to true
  if $COMSPEC is not empty then set the shellCommand to $COMSPEC
  else
    --just in case some windows versions don't use $COMSPEC
    if revRunningWindowsNT() then set the shellCommand to "cmd.exe"
    else set the shellCommand to "command.com"
  end if
end revSetWindowsShellCommand


getProp cREVGeneral[pWhichProp] pWhichProfile
   lock messages
   local tRevUniqueID
   put the cREVGeneral["revUniqueID"] of the target into tRevUniqueID
   unlock messages
   
   # OK-2008-04-14 : The long id will return an invalid reference when in edit group mode,
   # instead the "rugged id" should be used.
   --put the long id of the target into tTarget
   
   # OK-2008-06-02 : Because this script is used in standalones, using revRuggedId will break this
   # because its located in revBackscript (which is not included in standalones). As we don't require a
   # rugged id in standalones anyway, just revert to the old behavior in standalone environment.
   local tTarget
   if the environment is "development" then
      put revRuggedId(the long id of the target) into tTarget
   else
      put the long id of the target into tTarget
   end if
   
   local tMilliseconds
   if pWhichProp is "revUniqueID" and tRevUniqueID is empty then
      put the milliseconds into tMilliSeconds
      set the cREVGeneral["revUniqueID"] of tTarget to tMilliSeconds
      return tMilliSeconds
   else if pWhichProp is "revUniqueID" then
      return tRevUniqueID
   end if
   
   # OK-2009-02-11 : Global not declared...
   global gREVSuppressMessages
   if pWhichProp is not "profile" then if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass cREVGeneral
   lock messages
   
   local tProfile
   put the cREVGeneral["profile"] of tTarget into tProfile
   if tProfile is empty or tProfile is the cREVGeneral["masterName"] of tTarget then put "Master" into tProfile
   unlock messages
   return tProfile
end cREVGeneral
  

setProp revProfile pWhich
  set the cREVGeneral["profile"] of the target to pWhich
end revProfile

getProp revProfile
  return the cREVGeneral["profile"] of the target
end revProfile



setProp cREVGeneral[pWhichProp] pWhichProfile
   local tTarget
   put the long id of the target into tTarget
   
   local tTargetLongID
   put tTarget into tTargetLongID
   if pWhichProp is not "profile" and pWhichProp is not "inLineImages" and pWhichProp is not "virtualWidth" and pWhichProp is not "virtualHeight" and pWhichProp is not "breakPoints" then pass cREVGeneral
   if pWhichProp is "virtualWidth" then
      lock messages
      local tOriginalWidth
      put the cREVGeneral["virtualWidth"] of tTarget into tOriginalWidth
      set the cREVGeneral["virtualWidth"] of tTarget to pWhichProfile
      
      local tDifference
      put tOriginalWidth - pWhichProfile into tDifference
      
      local tRect
      put the rect of stack revTargetStack() into tRect
      put item 3 of tRect + tDifference into item 3 of tRect
      set the rect of stack revtargetStack() to tRect
      unlock messages
      exit cREVGeneral
   end if
   if pWhichProp is "virtualHeight" then
      lock messages
      local tOriginalHeight
      put the cREVGeneral["virtualHeight"] of tTarget into tOriginalHeight
      set the cREVGeneral["virtualHeight"] of tTarget to pWhichProfile
      put tOriginalHeight - pWhichProfile into tDifference
      put the rect of stack revTargetStack() into tRect
      put item 4 of tRect + tDifference into item 4 of tRect
      set the rect of stack revtargetStack() to tRect
      unlock messages
      exit cREVGeneral
   end if
   if pWhichProp is "breakPoints" then
      
      pass cREVGeneral
      
      local tBreakPointsList
      put the breakPoints into tBreakPointsList
      
      local tMainStack
      put the mainStack of stack revTargetStack(tTarget) into tMainStack
      
      local tObjects
      put the cREVGeneral["debugObjects"] of stack tMainStack into tObjects
      
      local tStack
      put word -2 to -1 of tTarget into tStack
      put the name of tStack into word -2 to -1 of tTarget
      if pWhichProfile is not empty then
         if tTarget is not among the lines of tObjects then
            --add breakpoint object
            put tTarget into line (the number of lines in tObjects)+1 of tObjects
         end if
      else
         if tTarget is among the lines of tOBjects then
            --remove breakpoint object
            delete line lineOffset(tTarget,tObjects) of tOBjects
         end if
      end if 
      set the cREVGeneral["debugObjects"] of stack tMainStack to tObjects
      
      local tOldPoints
      repeat for each line l in tBreakPointsList
         if tTargetLongID is not in l then put l & cr after tOldPoints
      end repeat
      delete last char of tOldPoints
      put tOldPoints into tBreakPointsList
      
      local tAddPoints
      repeat for each line l in pWhichProfile
         put tTarget,l & cr after tAddPoints
      end repeat
      delete last char of tAddPoints
      put tAddPoints into line (the number of lines in tBreakPointsList+1) of tBreakPointsList
      set the breakPoints to tBreakPointsList
      pass cREVGeneral
   end if
   --inline images next
   if pWhichProp is "inLineImages" then
      if pWhichProfile is true then
         lock messages
         lock screen
         set the lockLocation of the templateGroup to true
         set the rect of the templateGroup to item 1 to 2 of the rect of tTarget,item 3 of the rect of tTarget-the scrollbarWidth of tTarget,item 4 of the rect of tTarget
         set the name of the templateGroup to the short name of tTarget&"images"&the number of this cd
         create group
         reset the templateGroup
         
         local tGroupName
         put the short name of tTarget & "images"&the number of this cd into tGroupName
         --now pad the group so it can scroll
         reset the templateGraphic
         set the style of the templateGraphic to "line"
         set the lineSize of the templateGraphic to 0
         set the topLeft of the templateGraphic to the topLeft of tTarget
         create grc in group tGroupName
         reset the templateGraphic
         set the scroll of group tGroupName to 0
         set the scroll of tTarget to 0
         set the cREVGeneral["preInlineText"&the number of this cd] of tTarget to the text of tTarget
         set the itemDel to "/"
         
         local tSubtract
         put 0 into tSubtract
         
         local tFinalExit
         repeat
            if tFinalExit then exit repeat
            
            local tLineCount
            put 0 into tLineCount
            set the scroll of group tGroupName to 0
            set the scroll of tTarget to 0
            repeat for each line l in the text of tTarget
               put true into tFinalExit
               add 1 to tLineCount
               if item 2 of l is "*IMAGEFILE*" then
                  do "put empty into line tLineCount of" && tTarget
                  reset the templateImage
                  set the fileName of the templateImage to item 3 to -1 of l
                  
                  local tFieldName
                  put the short name of tTarget into tFieldName
                  
                  local tTop
                  put (the formattedHeight of line 1 to tLineCount of fld tFieldName + the top of tTarget) into tTop
                  set the width of the templateImage to the formattedWidth of the templateImage
                  
                  local tImageHeight
                  put the formattedHeight of the templateImage into tImageHeight
                  set the height of the templateImage to tImageHeight
                  set the lockLoc of the templateImage to true
                  set the topLeft of the templateImage to the left of tTarget+8,tTop-tSubtract
                  create image in group tGroupName
                  
                  local tNewLines
                  put 0 into tNewLines
                  repeat
                     add 1 to tNewLines
                     put cr after line tLineCount of the target
                     if the formattedHeight of line tLineCount to tLineCount+tNewLines of fld tFieldName >= tImageHeight then exit repeat
                  end repeat
                  put cr after line tLineCount of the target
                  put false into tFinalExit
                  exit repeat
               end if
               if item 2 of l is "*IMAGEID*" then
                  do "put empty into line tLineCount of" && tTarget
                  reset the templateButton
                  set the showName of the templateButton to false
                  set the showBorder of the templateButton to false
                  set the traversalOn of the templateButton to false
                  set the autoHilite of the templateButton to false
                  set icon of the templateButton to item 3 to -1 of l
                  put the short name of tTarget into tFieldName
                  put (the formattedHeight of line 1 to tLineCount of fld tFieldName + the top of tTarget) into tTop
                  set the width of the templateButton to the formattedWidth of the templateButton
                  put the formattedHeight of the templateButton into tImageHeight
                  set the height of the templateButton to tImageHeight
                  set the lockLoc of the templateButton to true
                  set the topLeft of the templateButton to the left of tTarget+8,tTop-tSubtract
                  create button in group tGroupName
                  put 0 into tNewLines
                  repeat
                     add 1 to tNewLines
                     put cr after line tLineCount of the target
                     if the formattedHeight of line tLineCount to tLineCount+tNewLines of fld tFieldName >= tImageHeight then exit repeat
                  end repeat
                  put cr after line tLineCount of the target
                  put false into tFinalExit
                  exit repeat
               end if
            end repeat
            put 30 into tSubtract
         end repeat
         reset the templateButton
         reset the templateImage
         --now pad the group so it can scroll
         
         local tFieldHeight
         put the formattedHeight of fld tFieldName into tFieldHeight
         reset the templateGraphic
         set the style of the templateGraphic to "line"
         set the lineSize of the templateGraphic to 0
         set the topLeft of the templateGraphic to the left of tTarget,tFieldHeight+the top of tTarget
         create grc in group tGroupName
         reset the templateGraphic
         set the scroll of group tGroupName to 0
         set the scroll of the target to 0
         unlock screen
         unlock messages
      else
         delete group (the short name of tTarget&"images"&the number of this cd)
         set the text of tTarget to the cREVGeneral["preInlineText"&the number of this cd] of tTarget
         set the cREVGeneral["preInlineText"&the number of this cd] of tTarget to empty
      end if
      set the cREVGEneral["inLineImages"&the number of this cd] of tTarget to pWhichProfile
      exit cREVGeneral
   end if
   global gREVProfileReadOnly, gREVAutoCreateProfiles
   lock screen
   lock messages
   
   local tReturnSet
   put the customPropertySet of tTarget into tReturnSet
   set the cREVGeometryCache["order"] of this card to empty
   if the customkeys["cRevGeometry"] of tTarget is not empty then
      if "revUpdateGeometry" is in the pendingMessages then 
         cancel item 1 of line (lineOffset("revUpdateGeometry",the pendingMessages)) of the pendingMessages
      end if
      send "revUpdateGeometry" to stack revTargetStack(tTarget) in 250 milliseconds -- update geometry
   end if
   if pWhichProfile is not empty and pWhichprofile is not "master" and pWhichProfile is not the cREVGeneral["masterName"] of tTarget and pWhichProfile is not among the lines of the cREVGeneral["profileList"] of tTarget then
      if gREVAutoCreateProfiles then revNewProfile pWhichProfile, tTarget
      unlock screen
      unlock messages
      exit cREVGeneral
   end if
   if the cREVGeneral["profile"] of tTarget is "Master" or the cREVGeneral["profile"] of tTarget is empty or the cREVGeneral["profile"] of tTarget is the cREVGeneral["masterName"] of tTarget then
      --its in master, going out
      if not gREVProfileReadOnly then
         set the customPropertySet of tTarget to "cREVTempMaster"
         
         local tTempArray
         put the properties of tTarget into tTempArray
         set the customProperties of tTarget to tTempArray
      end if
   else
      --its another profile, going into or through master
      set the customPropertySet of tTarget to "cREVMaster"
      
      local tCurrentMasterArray
      if the customKeys["cREVMaster"] of tTarget is not empty then put the customProperties of tTarget into tCurrentMasterArray
      if not gREVProfileReadOnly then
         set the customPropertySet of tTarget to "cREVTempMaster"
         
         local tTempMasterArray
         put the customProperties of tTarget into tTempMasterArray
         
         local tCurrentPropertiesArray
         put the properties of tTarget into tCurrentPropertiesArray
         --compare the temporary master with the current properties
         --any changes:
         --the original tempMaster value gets added to the current master array
         --and the new value gets created in the current profile array
         
         local tCurrentPropertyKeys
         put keys(tCurrentPropertiesArray) into tCurrentPropertyKeys
         set the caseSensitive to true
         repeat for each line l in tCurrentPropertyKeys
            if tTempMasterArray[l] is not tCurrentPropertiesArray[l] then
               put tTempMasterArray[l] into tCurrentMasterArray[l]
               
               local tProfileArray
               put tCurrentPropertiesArray[l] into tProfileArray[l]
            end if
         end repeat
         --transfer the current profile array to the object
         set the customPropertySet of tTarget to "cREVDiverged" & the cREVGeneral["profile"] of tTarget
         set the customProperties of tTarget to tProfileArray
         --now transfer the current master array to the master profile
         set the customPropertySet of tTarget to "cREVMaster"
         set the customProperties of tTarget to tCurrentMasterArray
      end if
      
      # MW-2010-10-14: [[ Bug 9060 ]] In 4.5 the 'id' property of objects became mutable - we never
      #    want to set this property as part of a profile, so make sure its never in the array.
      delete variable tCurrentMasterArray["id"]
      set the properties of tTarget to tCurrentMasterArray
      set the cREVGeneral["profile"] of tTarget to "Master"
   end if
   if pWhichProfile is not empty and pWhichProfile is not "master" and pWhichProfile is not the cREVGeneral["masterName"] of tTarget then
      set the cREVGeneral["profile"] of tTarget to pWhichProfile
      set the customPropertySet of tTarget to "cREVDiverged"&pWhichProfile
      
      local tTempArray1
      if the customKeys of tTarget is not empty then put the customProperties of tTarget into tTempArray1
      
      # MW-2010-10-14: [[ Bug 9060 ]] In 4.5 the 'id' property of objects became mutable - we never
      #    want to set this property as part of a profile, so make sure its never in the array.
      delete variable tTempArray1["id"]
      set the properties of tTarget to tTempArray1
   end if
   set the customPropertySet of tTarget to tReturnSet
   unlock screen
   unlock messages
end cREVGeneral

function revAppVersion
  return the version
end revAppVersion

function revLoadedStacks pWhich
   global gREVShowStacks
   
   local tMainstacks
   put the mainStacks into tMainStacks
   switch pWhich
      case "all"
         break
      case "preference"
         if not gREVShowStacks
         then put revFilterStacksList(tMainStacks) into tMainStacks --filters Revolution stacks
         break
      default
         put revFilterStacksList(tMainStacks) into tMainStacks
   end switch
   
   local tStacksInMemory
   put empty into tStacksInMemory
   repeat for each line l in tMainStacks
      if the substacks of stack l is empty
      then put l & cr after tStacksInMemory
      else put l & cr & the substacks of stack l & cr after tStacksInMemory
   end repeat
   delete last char of tStacksInMemory
   return tStacksInMemory
end revLoadedStacks

-- MW-2010-09-29: Slight tweak to this so that IDE stacks never appear in lists - regardless of mode.
function revFilterStacksList pStacks
   local tReturnValue
   repeat for each line l in pStacks
      if l is among the items of "Message Box,Home,Answer Dialog,Ask Dialog,Script Debugger,File Selector,Color Chooser,Magnify" then
         next repeat
      end if
      if char 1 to 3 of l is "rev" then
         next repeat
      end if
      
      -- This allows a hook function somewhere in the message path to do extra filtering on stacks.
      dispatch function "revHookIsUserStack" to me with the long id of stack l
      if it is not "handled" or the result then
         put l & cr after tReturnValue
      end if
   end repeat
   delete last char of tReturnValue
   return tReturnValue
end revFilterStacksList


# OK-2007-07-05: Rewrote revRotatePoly and revPoints to fix bug 4882

# Parameters
#   pGraphic : A reference to a graphic
#   pAngle : The angle to rotate the graphic by
# Description
#   Performs a rotation transformation whose origin is the loc of the graphic and
#   whose magnitude is pAngle degrees. Preserves the topLeft of the graphic, but may
#   cause distortion due to rounding errors.
command revRotatePoly pGraphic, pAngle
    local tPoints, tFinalPoints
    local tLoc
    local tSinAngle, tCosAngle
    local tCurrentH, tCurrentV, tTransformedH, tTransformedV
    local tOrigLeft, tOrigTop
    
    lock screen
    lock messages
    if the realPoints of pGraphic is empty then
        put the effective points of pGraphic into tPoints
    else
        put the realPoints of pGraphic into tPoints
    end if
    put the loc of pGraphic into tLoc
    put item 1 of tLoc into tOrigLeft
    put item 2 of tLoc into tOrigTop
    put sin(pi*pAngle/180) into tSinAngle
    put cos(pi*pAngle/180) into tCosAngle
    repeat for each line tPoint in tPoints
        if tPoint is not empty then
            put item 1 of tPoint into tCurrentH
            put item 2 of tPoint into tCurrentV
            put (tOrigLeft+(tCosAngle*tCurrentH)-(tSinAngle*tCurrentV), tOrigTop+(tCosAngle*tCurrentV)+(tSinAngle*tCurrentH) ) after tFinalPoints
        end if
        put cr after tFinalPoints
    end repeat
    delete char -1 of tFinalPoints -- is CR
    if the style of pGraphic is among the items of "oval,rectangle,roundrect,regular" then
        set the style of pGraphic to "polygon"
    end if
    set points of pGraphic to tFinalPoints
    set the realPoints of pGraphic to tFinalPoints
    set loc of pGraphic to tLoc
    unlock messages
    unlock screen
end revRotatePoly

command revRotatePolygonOld pGraphic, pAngle
  local tTopLeft
  put the topLeft of pGraphic into tTopLeft
  
  local tPoints
  put revPoints(pGraphic) into tPoints
  
  local tLoc
  put the loc of pGraphic into tLoc
  
  local tSinAngle
  put sin(pAngle * (pi / 180)) into tSinAngle
  
  local tCosAngle
  put cos(pAngle * (pi / 180)) into tCosAngle
  
  local tLineCount
  put (the number of lines of tPoints) into tLineCount
  
  local tCurrentH, tCurrentV, tTransformedH, tTransformedV, tFinalPoints
  repeat for each line tPoint in tPoints
    if tPoint is empty then
      put empty & return after tFinalPoints
      next repeat
    end if 
    
    put item 1 of tPoint into tCurrentH
    put item 2 of tPoint into tCurrentV
    
    put trunc((tCosAngle * tCurrentH) - (tSinAngle * tCurrentV)) into tTransformedH
    put trunc((tSinAngle * tCurrentH) + (tCosAngle * tCurrentV)) into tTransformedV
    put tTransformedH + (item 1 of line 1 of tPoints) into tCurrentH
    put tTransformedV + (item 2 of line 1 of tPoints) into tCurrentV
    
    put tCurrentH,tCurrentV & return after tFinalPoints
  end repeat
  delete the last char of tFinalPoints
  
  set the points of pGraphic to tFinalPoints
  set the topLeft of pGraphic to tTopLeft
end revRotatePolygonOld

# Parameters
#   pGraphic : A reference to a graphic
# Returns
#   A return delimited list of points in the same format as the points property.
# Description
#   Note that the return format is different to the old behavior (changed in 2.9-0-dp-1). The 
#   points returned by this function are relative to the centre of the graphic,
#   unlike the points returned by the engine which are absolute.
function revPoints pGraphic
  local tNumPoints
  put the number of lines in the points of pGraphic into tNumPoints
  
  local tCentreH
  put item 1 of loc of pGraphic into tCentreH
  
  local tCentreV
  put item 2 of loc of pGraphic into tCentreV
  
  local tPoints
  put the points of pGraphic into tPoints
   
  local tResult
  repeat for each line tPoint in tPoints
    if tPoint is empty then
      put empty & return after tResult
      next repeat
    end if
    
    local tAbsolutePointH
    put item 1 of tPoint into tAbsolutePointH
    
    local tAbsolutePointV
    put item 2 of tPoint into tAbsolutePointV
    
    put tAbsolutePointH - tCentreH & comma & tAbsolutePointV - tCentreV & return after tResult
  end repeat
  delete the last char of tResult
  
  return tResult
end revPoints


----polygon rotation functions (used by menu bar)
--on revRotatePoly pGraphic, pAngle
--  put the topLeft of pGraphic into tTopLeft
--  put revPoints(pGraphic) into tPoints
--  put the loc of pGraphic into tLoc
--  put sin(pAngle * (pi / 180)) into tSinAngle
--  put cos(pAngle * (pi / 180)) into tCosAngle
--  put (number of items of tPoints) div 2 into tNumItems
--  repeat with i = 1 to tNumItems
--    put (item (i + (i - 1)) of tPoints) into tCurrentH -- + item 1 of the center
--    put (item (i + i) of tPoints) into tCurrentV -- + item 2 of the center
--    put trunc((tCosAngle * tCurrentH) - (tSinAngle * tCurrentV)) into tTempH
--    put trunc((tSinAngle * tCurrentH) + (tCosAngle * tCurrentV)) into tTempV
--    put tTempH + (item 1 of tPoints) into tCurrentH
--    put tTempV + (item 2 of tPoints) into tCurrentV
--    put tCurrentH,tCurrentV & comma after tFinalPoints
--  end repeat
--  delete last char of tFinalPoints
--  set the points of pGraphic to tFinalPoints
--  set the topLeft of pGraphic to tTopLeft
--end revRotatePoly
--
--function revPoints pGraphic
--  put the number of lines in the points of pGraphic into tNumPoints
--  put empty into tResult
--  put item 1 of loc of pGraphic into tStartlH
--  put item 2 of loc of pGraphic into tStartV
--  put the points of pGraphic into tPoints
--  replace cr with comma in tPoints
--  if last char of tPoints is comma then delete last char of tPoints
--  repeat with i = 1 to tNumPoints
--    put (item (i + (i - 1)) of tPoints) - tStartlH into tCurrentH
--    put (item (i + i) of tPoints) - tStartV into tCurrentV
--    put tCurrentH,tCurrentV & comma after tResult
--  end repeat
--  delete last char of tResult
--  return tResult
--end revPoints



--Path Conversion Functions

function revUnixFromMacPath thePath,assumeOSX
  local numberOfLevels
  if assumeOSX is empty then -- set default based on current OS
    set the itemDelimiter to "."
    if the platform is "MacOS" and item 1 of the systemVersion is "10"
    then put true into assumeOSX
    else put false into assumeOSX
  end if
  
  if char 1 of thePath is colon then -- relative path
    repeat while char 1 of thePath is colon
      delete char 1 of thePath
      add 1 to numberOfLevels -- keep track
    end repeat
    if numberOfLevels is 1 then
      put ".:" before thePath
    else -- more than one level
      repeat for (numberOfLevels - 1) times
        put "..:" before thePath
      end repeat
    end if
  else -- colon is not char 1 - therefore it's an absolute path
    set the itemDelimiter to colon
    if assumeOSX then
      if line 1 of the drives is item 1 of thePath then delete item 1 of thePath -- startup drive
      else put "Volumes:" before thePath -- non-startup drives are rooted in the Volumes dir on OS X
    end if
    put colon before thePath -- will be transformed to slash
  end if
  
  replace colon with linefeed in thePath
  replace "/" with colon in thePath
  replace linefeed with "/" in thePath
  return thePath
end revUnixFromMacPath

function revMacFromUnixPath thePath,assumeOSX
  local numberOfLevels
  if assumeOSX is empty then -- set default based on current OS
    set the itemDelimiter to "."
    if the platform is "MacOS" and item 1 of the systemVersion is "10"
    then put true into assumeOSX
    else put false into assumeOSX
  end if
  
  -- MW-2008-02-28: [[ Bug 5678 ]] Make sure we resolve the UNIX ~ component as MacOS
  --   paths don't support this
  if char 1 of thePath is "~" then
    local tOldFolder
    put the folder into tOldFolder
    
    set the itemDelimiter to "/"
    set the folder to item 1 of thePath
    put the folder & slash & item 2 to -1 of thePath into thePath
    set the folder to tOldFolder
  end if
  
  replace colon with linefeed in thePath
  if char 1 of thePath is "/" then delete char 1 of thePath
  replace "/" with colon in thePath
  replace linefeed with "/" in thePath
  if char 1 to 2 of thePath is ".:" then delete char 1 of thePath
  else if char 1 to 3 of thePath is "..:" then -- relative path
    repeat while char 1 to 3 of thePath is "..:"
      delete char 1 to 3 of thePath
      add 1 to numberOfLevels
    end repeat
    repeat for numberOfLevels + 1 times
      put ":" before thePath
    end repeat
  end if
  
  if char 1 of thePath is not ":" and assumeOSX then
    -- it's an absolute path, so adjust for OS X if necessary
    set the itemDelimiter to ":"
    if item 1 of thePath is "Volumes" then delete item 1 of thePath -- non-startup disk
    else put line 1 of the drives & ":" before thePath -- need to prepend startup disk name
  end if
  return thePath
end revMacFromUnixPath




--File transfer functions
function revWindowsFromUnixPath pUnixPath
  replace "/" with "\" in pUnixPath
  return pUnixPath
end revWindowsFromUnixPath

on revCopyFile pSrcFile, pDestFile
   replace "//" with "/" in pSrcFile
   replace "//" with "/" in pDestFile
   
   # If the two paths are identical, this is a no-op as we are replacing the file with itself
   if pSrcFile is pDestFile then
      return empty
   end if
   
   # If the destination is a folder, and its the same folder as the file already resides in, this is also a no-op.
   set the itemDelimiter to slash
   if item 1 to -2 of pSrcFile is pDestFile and there is a folder pDestFile then
      return empty
   end if
   
   if the platform is "MacOS" then
      if "applescript" is in the alternateLanguages then
         if there is a file pDestFile then delete file pDestfile -- overwrite before copying
         do revAppleScriptFull("copyFile",pSrcFile,pDestFile) as "applescript"
         if word 1 to 2 of the result is "document file" then
            #Copy successful so don't return anything
            return empty
         else if the result is not "compile error" and the result is not "execution error" then
            return createCopyFileResult(the result, pDestFile)
         else
            return the result
         end if
      else return "Error: AppleScript not installed"
   else if the platform is "Win32" then
      revSetWindowsShellCommand
      
      # OK-2008-03-24 : Bug 6180, standardise the paths so that we can refer to items in them,
      # The paths will get converted back into Windows paths by revWindowsFromUnixPath.
      replace "\" with "/" in pDestFile
      replace "\" with "/" in pSrcFile
      
      if there is a directory pDestFile then
         set the itemDel to "/"
         if char -1 of pDestFile is "/" then
            delete char -1 of pDestFile
         end if
         
         local tDestFileName
         put item -1 of pSrcFile into tDestFileName
         set the itemDel to comma
         put "/" & tDestFileName after pDestFile
      end if
      get shell ("copy" && revWindowsFromUnixPath(quote&pSrcFile&quote) && revWindowsFromUnixPath(quote&pDestFile&quote))
   else
      get shell ("cp -f" && quote&pSrcFile&quote && quote&pDestFile&quote)
   end if
   return the result
end revCopyFile

private command log pMessage
  put pMessage & return after msg
end log

on mouseDoubleUp pButtonNo, pTarget
   --not been handled
   --pTarget set in suppress messages frontscript
   if pTarget is empty
   then put the long id of the target into pTarget
   
   local tStack
   put revTargetStack(pTarget) into tStack
   
   local tDefaultStack
   put the defaultStack into tDefaultStack
   set the defaultStack to tStack
   if the mode of stack tStack is not 0 then
      # OK-2008-11-28 : Bug 6455 - Must pass the correct mouse button here or button 1 will be assumed.
      click button pButtonNo at the clickLoc
   end if
   set the defaultStack to tDefaultStack
end mouseDoubleUp

on revCopyFolder pSrcFolder, pDestFolder
   if the last char of pSrcFolder is "/" then delete the last char of pSrcFolder
   if the platform is "MacOS" then
      if "applescript" is in the alternateLanguages then
         if there is not a folder pDestFolder then
            create folder pDestFolder
         end if
         do revAppleScriptFull("copyFolder",pSrcFolder,pDestFolder) as "applescript"
         if word 1 of the result is "folder" then
            #Copy successful so don't return anything
            return empty
         else
            return the result
         end if
      else return "Error: AppleScript not installed"
   else if the platform is "Win32" then
      revSetWindowsShellCommand
      if there is a directory pDestFolder then
         set the itemDel to "/"
         if char -1 of pDestFolder is "/" then delete char -1 of pDestFolder
         
         local tLastItemOfSrcFolder
         put item -1 of pSrcFolder into tLastItemofSrcFolder
         put "/" & tLastItemofSrcFolder after pDestFolder
         create directory pDestFolder
      end if
      
      # TH-2008-03-11 : Fix for 5465, should remedy problem with copy sometimes crashing on vista
      # now use robocopy instead of xcopy if operating under vista.
      if the systemVersion is "NT 6.0" then
         get shell ("robocopy" && revWindowsFromUnixPath(quote&pSrcFolder&quote) && revWindowsFromUnixPath(quote&pDestFolder&quote) && "/E /NFL /NDL /NS /NC /NJH /NJS")
      else
         get shell ("xcopy /I /E /Y /R" && revWindowsFromUnixPath(quote&pSrcFolder&quote) && revWindowsFromUnixPath(quote&pDestFolder&quote))
      end if
      
   else
      get shell ("cp -rf" && quote&pSrcFolder&quote && quote&pDestFolder&quote)
   end if
   return the result
end revCopyFolder

on revCopyFolder__Standalone pSrcFolder, pDestFolder
   if the last char of pSrcFolder is "/" then delete the last char of pSrcFolder
   if the platform is "MacOS" then
      if "applescript" is in the alternateLanguages then
         if there is not a folder pDestFolder then
            create folder pDestFolder
         end if
         do revAppleScriptFull("copyFolder",pSrcFolder,pDestFolder) as "applescript"
         if word 1 of the result is "folder" then
            #Copy successful so don't return anything
            return empty
         else
            return the result
         end if
      else return "Error: AppleScript not installed"
   else if the platform is "Win32" then
      revSetWindowsShellCommand
      if there is a directory pDestFolder then
         set the itemDel to "/"
         if char -1 of pDestFolder is "/" then delete char -1 of pDestFolder
         
         local tLastItemOfSrcFolder
         put item -1 of pSrcFolder into tLastItemofSrcFolder
         put "/" & tLastItemofSrcFolder after pDestFolder
         create directory pDestFolder
      end if
      
      # TH-2008-03-11 : Fix for 5465, should remedy problem with copy sometimes crashing on vista
      # now use robocopy instead of xcopy if operating under vista.
      if the systemVersion is "NT 6.0" then
         get shell ("robocopy" && revWindowsFromUnixPath(quote&pSrcFolder&quote) && revWindowsFromUnixPath(quote&pDestFolder&quote) && "/E /NFL /NDL /NS /NC /NJH /NJS")
      else
         get shell ("xcopy /I /E /Y /R" && revWindowsFromUnixPath(quote&pSrcFolder&quote) && revWindowsFromUnixPath(quote&pDestFolder&quote))
      end if
      
   else
      get shell ("cp -rf" && quote&pSrcFolder&quote && quote&pDestFolder&quote)
   end if
   return the result
end revCopyFolder__Standalone



on revCopyPackage pSrcFolder, pDestFolder
   if the platform is "MacOS" then
      if "applescript" is in the alternateLanguages then
         if there is not a folder pDestFolder then
            create folder pDestFolder
         end if
         do revAppleScriptFull("copyPackage",pSrcFolder,pDestFolder) as "applescript"
      else return "Error: AppleScript not installed"
   else if the platform is "Win32" then
      revSetWindowsShellCommand
      if there is a directory pDestFolder then
         set the itemDel to "/"
         if char -1 of pDestFolder is "/" then delete char -1 of pDestFolder
         
         local tLastItemOfSrcFolder
         put item -1 of pSrcFolder into tLastItemofSrcFolder
         put "/" & tLastItemofSrcFolder after pDestFolder
         create directory pDestFolder
      end if
      
      # TH-2008-03-11 : Fix for 5465, should remedy problem with copy sometimes crashing on vista
      # now use robocopy instead of xcopy if operating under vista.
      if the systemVersion is "NT 6.0" then
         get shell ("robocopy" && revWindowsFromUnixPath(quote&pSrcFolder&quote) && revWindowsFromUnixPath(quote&pDestFolder&quote) && "/E /NFL /NDL /NS /NC /NJH /NJS")
      else
         get shell ("xcopy /I /E /Y /R" && revWindowsFromUnixPath(quote&pSrcFolder&quote) && revWindowsFromUnixPath(quote&pDestFolder&quote))
      end if
      
      
   else
      get shell ("cp -rf" && quote&pSrcFolder&quote && quote&pDestFolder&quote)
   end if
   return the result
end revCopyPackage


on revDeleteFolder pSrcFolder
   if the platform is "MacOS" and char 1 of the systemVersion is not "1" then
      if "applescript" is in the alternateLanguages then
         # OK-2009-02-11 : Removed unquoted literal pDestFolder
         --do revAppleScriptFull("deleteFolder",pSrcFolder,pDestFolder) as applescript
         do revAppleScriptFull("deleteFolder", pSrcFolder) as "applescript"
      else return "Error: AppleScript not installed"
   else if the platform is "Win32" then
      revSetWindowsShellCommand
      -- check for Win 2000, XP, Vista
      if char 1 of word 2 of the systemVersion >= "5" then
         get shell ("rmdir /s /q" && revWindowsFromUnixPath(quote&pSrcFolder&quote))
      else
         get shell ("deltree /Y" && revWindowsFromUnixPath(quote&pSrcFolder&quote))
      end if
   else
      get shell ("rm -rf" && quote&pSrcFolder&quote)
   end if
   return the result
end revDeleteFolder

on revMoveFolder pSrcFolder, pDestFolder
  if the platform is "MacOS" and char 1 of the systemVersion is not "1" then
    if "applescript" is in the alternateLanguages then
      do revAppleScriptFull("moveFolder",pSrcFolder,pDestFolder) as "applescript"
    else return "Error: AppleScript not installed"
  else if the platform is "Win32" then
    revSetWindowsShellCommand
    get shell ("move /Y" && revWindowsFromUnixPath(quote&pSrcFolder&quote) && revWindowsFromUnixPath(quote&pDestFolder&quote))
  else
    get shell ("mv -f" && quote&pSrcFolder&quote && quote&pDestFolder&quote)
  end if
  return the result
end revMoveFolder

--function revPathtoApplescript pPath
--  put empty into tApplescript
--  set itemdelimiter to "/"
--  put the number of items in pPath into tNumItems
--  replace "\" with "\\" in pPath
--  replace quote with "\" & quote in pPath
--  
--  if "/Desktop Folder/" is in pPath
--  then
--    repeat with i = tNumItems down to 3
--      if i > 4 then
--        put tApplescript && quote & item i of pPath & quote && "of folder" into tApplescript
--      else
--        if i = 3 then
--          put tApplescript && quote & item i of pPath & quote into tApplescript
--        else
--          put tApplescript && quote & item i of pPath & quote && "of folder" into tApplescript
--        end if
--      end if
--    end repeat
--  else
--    repeat with i = tNumItems down to 2
--      if i > 3 then
--        put tApplescript && quote & item i of pPath & quote && "of folder" into tApplescript
--      else
--        if i = 2 then
--          put tApplescript && quote & item i of pPath & quote into tApplescript
--        else
--          put tApplescript && quote & item i of pPath & quote && "of disk" into tApplescript
--        end if
--      end if
--    end repeat
--  end if
--  return tApplescript
--end revPathtoApplescript

function revPathtoApplescript pPath
   local tApplescript
   put empty into tApplescript
   replace "\" with "\\" in pPath
   replace quote with "\" & quote in pPath
   put quote & revMacFromUnixPath(pPath) & quote into tApplescript
   return tApplescript
end revPathtoApplescript

# OK-2009-02-11 : Bug 7580 - Added to allow revCopyFile to return something useful
# Currently, it just returns empty. However the disabled branch of code will convert
# the parameters into an absolute file path in case we want to use this in the future.
private function createCopyFileResult pPath, pDestination
   return empty
   
   local tPath
   put pPath into tPath
   if char 1 of tPath is quote and char -1 of tPath is quote then
      delete char 1 of tPath
      delete char -1 of tPath
   end if
   
   local tIsFilename
   if tPath contains ":" then
      put false into tIsFilename
   else
      put true into tIsFilename
   end if
   
   if not tIsFilename then
      # Assume absolute path and convert
      local tUnixPath
      put revUnixFromMacPath(tPath) into tUnixPath
      
      replace ("\" & quote) with quote in tUnixPath
      replace "\\" with "\" in tUnixPath
      return tUnixPath
   else
      # Turn it into an absolute path for consistent return values
      local tFolder
      if there is a folder pDestination then
         put pDestination into tFolder
      else
         set the itemDelimiter to slash
         put item 1 to -2 of pDestination into tFolder
         set the itemDelimiter to comma
      end if
      
      return tFolder & slash & tPath
   end if
end createCopyFileResult

--function revApplescriptFull pfunction,pSrc,pDest
--   switch pfunction
--      case "deleteFolder"
--         put "tell application" && quote & "Finder" & quote & cr & tab & "delete folder" && revPathtoApplescript(pSrc) && cr & "end tell" into tApplescriptfull
--         exit switch
--      case "copyFile"
--         if there is a folder pDest then
--            put "tell application" && quote & "Finder" & quote & cr & tab & "copy file" && revPathtoApplescript(pSrc) && "to folder" && revPathtoApplescript(pDest) && cr & "end tell" into tApplescriptfull
--         else
--            set the itemDel to "/"
--            put item -1 of pDest into tFile
--            put item 1 to -2 of pDest into tDest
--            put item -1 of pSrc into tOrig
--            --put "tell application" && quote & "Finder" & quote & cr & tab & "copy file" && revPathtoApplescript(pSrc) && "to folder" && revPathtoApplescript(tDest) && cr & tab & "set name of file" && revPathtoApplescript(tDest & "/" & tOrig) && "to" && quote & tFile & quote & cr & "end tell" into tApplescriptfull
--            put "tell application " & quote & "Finder" & quote & return & " duplicate file " & revPathtoApplescript(pSrc) & " to folder " & revPathtoApplescript(tDest) & " without replacing" & return & "set name of file " & revPathtoApplescript(tDest & "/" & tOrig) & " to " & quote & tFile & quote & return & "end tell" into tApplescriptfull
--         end if
--         exit switch
--      case "copyPackage"
--         put "tell application" && quote & "Finder" & quote & cr & tab & "copy package" && revPathtoApplescript(pSrc) && "to folder" && revPathtoApplescript(pDest) && cr & "end tell" into tApplescriptfull
--         exit switch
--      case "copyFolder"
--         put "tell application" && quote & "Finder" & quote & cr & tab & "copy folder" && revPathtoApplescript(pSrc) && "to folder" && revPathtoApplescript(pDest) && cr & "end tell" into tApplescriptfull
--         exit switch
--      case "moveFolder"
--         put "tell application" && quote & "Finder" & quote & cr & tab & "move folder" && revPathtoApplescript(pSrc) && "to folder" && revPathtoApplescript(pDest) && cr & "end tell" into tApplescriptfull
--         exit switch
--   end switch
--   return tApplescriptfull
--end revApplescriptFull

function revApplescriptFull pFunction, pSrc, pDest
   local tScript
   switch pFunction
      case "deleteFolder"
         put "tell application" && quote & "Finder" & quote & cr & tab & "delete folder" && revPathtoApplescript(pSrc) && cr & "end tell" into tScript
         break
      case "copyFile"
         # OK-2009-02-11 : Bug 7580 - Rewritten to take some extra cases into account
         put buildCopyFileApplescript(pSrc, pDest, false) into tScript
         break
      case "copyPackage"
         put "tell application" && quote & "Finder" & quote & cr & tab & "copy package" && revPathtoApplescript(pSrc) && "to folder" && revPathtoApplescript(pDest) && cr & "end tell" into tScript
         break
      case "copyFolder"
         put "tell application" && quote & "Finder" & quote & cr & tab & "copy folder" && revPathtoApplescript(pSrc) && "to folder" && revPathtoApplescript(pDest) && cr & "end tell" into tScript
         break
      case "moveFolder"
         put "tell application" && quote & "Finder" & quote & cr & tab & "move folder" && revPathtoApplescript(pSrc) && "to folder" && revPathtoApplescript(pDest) && cr & "end tell" into tScript
         break
   end switch
   return tScript
end revApplescriptFull

# OK-2009-02-11 : Bug 7580
# Returns the applescript for the revCopyFile command.
private function buildCopyFileApplescript pSrc, pDest, pReplace
   # If these are both identical, its a no-op regardless of pReplace.
   if pSrc is pDest then
      return empty
   end if
   
   # Convert the source path into applescript
   local tSource
   put revPathToAppleScript(pSrc) into tSource
   
   # Get the name of the destination file, this will only be used if pDest was not a folder
   local tName
   set the itemDelimiter to slash
   put item -1 of pDest into tName
   
   # Get the source folder so we can compare it to the destination folder
   local tSourceFolder, tDestinationFolder, tDestWasFolder
   put item 1 to -2 of pSrc into tSourceFolder
   
   # Work out whether the destination is a folder or not
   if there is a folder pDest then
      put pDest into tDestinationFolder
      put true into tDestWasFolder
   else
      put item 1 to -2 of pDest into tDestinationFolder
      put false into tDestWasFolder
   end if
   
   local tScript
   
   local tDestination
   if tSourceFolder is tDestinationFolder and tDestWasFolder then
      # If we are copying to the same folder and not renaming the file then either its a no-op, or we let applescript rename the file for us.
      if pReplace then
         # Duplicate the file by passing no location to the duplicate command, return the resulting file which will have "copy" appended to it
         put merge("tell application [[quote]]Finder[[quote]][[return]][[tab]]duplicate file [[tSource]][[return]][[tab]]return the result as text[[return]]end tell") into tScript
      else
         # This is a no-op as we are replacing the file with itself
         return empty
      end if
   else if tSourceFolder is tDestinationFolder and not tDestWasFolder then
      # If we are copying to the same folder and also setting the name of the file
      # Duplicate the file by passing no location to the duplicate command, then rename it using set the name of file, return the resulting file path.
      put merge("tell application [[quote]]Finder[[quote]][[return]][[tab]]duplicate file [[tSource]][[return]][[tab]]set the name of (the result) to [[quote]][[tName]][[quote]][[return]][[tab]]return the result as text[[return]]end tell") into tScript
   else if tDestWasFolder then
      # We are copying to a different folder, and not renaming the file, this is easy.
      put revPathToApplescript(pDest) into tDestination
      put merge("tell application [[quote]]Finder[[quote]][[return]][[tab]]duplicate file [[tSource]] to folder [[tDestination]][[return]][[tab]]return the result as text[[return]]end tell") into tScript
   else
      # We are copying to a different folder and also renaming the file.
      put revPathToAppleScript(item 1 to -2 of pDest) into tDestination
      
      local tNewPath
      put revPathToAppleScript(item 1 to -2 of pDest & slash & item -1 of pSrc) into tNewPath
      put merge("tell application [[quote]]Finder[[quote]][[return]][[tab]]duplicate file [[tSource]] to folder [[tDestination]] with replacing[[return]][[tab]]set the name of file [[tNewPath]] to [[quote]][[tName]][[quote]][[return]][[tab]]return the result as text[[return]]end tell") into tScript
   end if
   
   return tScript
end buildCopyFileApplescript


-- MW-2011-01-05: [[ Bug 9267 ]] Although deprecated as of 4.5, for backwards compatibility
--   this function returns 'Enterprise'. This is the feature set which LiveCode has from 4.5 onwards.
--function revLicenseType
--   local tType
--   if the environment is not "development" then
--      put "Standalone" into tType
--   else
--      put "Enterprise" into tType
--   end if
--   return tType
--end revLicenseType

on revChangeWindowSize pWidth, pHeight, pSlide, pCard, pDelay
   global gREVResizeTime
   -- set total time
   
   local tTotalTime
   if pDelay is not empty then put pDelay into tTotalTime
   else if gREVResizeTime is not empty then put gREVResizeTime into tTotalTime
   else put 250 into tTotalTime
   
   local tWidthTotal, tHeightTotal
   put pWidth - the width of this stack into tWidthTotal
   put pHeight - the height of this stack into tHeightTotal
   
   local tWidthFraction
   if tWidthTotal is 0 then put 0 into tWidthFraction
   else put tTotalTime / tWidthTotal into tWidthFraction
   
   local tHeightFraction
   if tHeightTotal is 0 then put 0 into tHeightFraction
   else put tTotalTime / tHeightTotal into tHeightFraction
   if tWidthFraction is 0 and tHeightFraction is 0 then exit revChangeWindowSize
   
   local tStackBottom
   put the bottom of this stack into tStackBottom
   
   local tStackRight
   put the right of this stack into tStackRight
   
   local tStart
   put the milliseconds into tStart
   
   local tTopLeft
   put item 1 to 2 of the rect of this stack into tTopLeft
   lock messages
   
   local tReturnCard
   if pCard is not empty and (tWidthTotal is not 0 or tHeightTotal is not 0) then
      put the number of this card into tReturnCard
      go cd pCard
   end if
   if pSlide is not false and pSlide is not "snap" then
      repeat
         local tMillisecs
         put the milliseconds into tMillisecs
         
         local tWidthPosition, tHeightPosition
         if tWidthFraction is 0 then put 0 into tWidthPosition
         else put round((tMillisecs - tStart)/tWidthFraction) into tWidthPosition
         if tHeightFraction is 0 then put 0 into tHeightFraction
         else put round((tMillisecs - tStart)/tHeightFraction) into tHeightPosition
         set the rect of this stack to tTopLeft, tStackRight + tWidthPosition, tStackBottom + tHeightPosition
         if the milliseconds > tStart+tTotalTime then exit repeat
      end repeat
   end if
   --ensure move complete for last few pixels
   set the rect of this stack to tTopLeft,tStackRight+tWidthTotal,tStackBottom+tHeightTotal
   if tReturnCard is not empty then go cd tReturnCard
   unlock messages
end revChangeWindowSize

on revChangePage
  
end revChangePage

on revUpdateCellValue
  
end revUpdateCellValue

on revMoveCellDirection
  
end revMoveCellDirection

on revDatabaseError pType, pError, pDataLinkedObject
   local tResult
   put the cREVDatabaseQuery["sqlresult"] of pDataLinkedObject into tResult
   if tResult is a number then
      exit revDatabaseError
   end if
   if tResult is "empty" then 
      put empty into tResult
   end if
   
   local tErrorMessage
   switch pType
      case "Connect"
         put "There was an error in connecting to the database. Please check that the connection fields are correct." & cr & "Revdb error:" && tResult into tErrorMessage
         break
      case "ReadData"
         put "No such record found in the database:" && short name of pError & cr & "Revdb error:" && tResult into tErrorMessage
         break
      case "WriteData"
         put "Update failed on the SQL command:" && pError & cr & "Revdb error:" && tResult into tErrorMessage
         break
      case "NoKey"
         put "No primary key has been entered for the database:" && short name of pError & cr & "Revdb error:" && tResult into tErrorMessage
         break
      case "NoTable"
         put "No table has been selected for updating the database: " && short name of pError  & cr & "Revdb error:" && tResult into tErrorMessage
         break
      case "NoQuery"
         put "No SQL command set in the database:" && short name of pError into tErrorMessage
         break
      case "ExecuteQuery"
         put "An error resulted for the SQL command in the database:" && short name of pError & cr & "Revdb error:" && tResult into tErrorMessage
         break 
      case "EmptyView"
         put "No records returned in the database:" && short name of pError into tErrorMessage
         break
      default
         put "An error has occured with the database:" && short name of pError & cr & "Revdb error:" && tResult into tErrorMessage
   end switch
   
   answer error tErrorMessage
end revDatabaseError

function revAnswerFile pType, pPrompt
  -- KM 4th June 2005
  -- handler takes file type and brings up answer file with all types listed
  -- any corrections can be made in one place
  switch pType
  case "stack"
    answer file pPrompt with type "LiveCode Stacks|livecode,rev,mc|RSTK,MSTK" or type "All Files|"
    break
  case "image"
    answer file pPrompt with type "All Picture Files|png,gif,jpg,jpeg,bmp,pct,xbm,xpm,xwd,pbm,ppm,pgm" or type "Portable Network Graphics|png" or type "Joint Photographics Experts Group|jpg,jpeg" or type "Graphics Interchange Format|gif" or type "Windows Bitmap|bmp" or type "Unix Formats|xbm,xpm,xwd,pbm,ppm,pgm" or type "Macintosh PICT - displays on MacOS only|pct" or type "All Files|"
    break
  case "quicktime"
    answer file pPrompt with type "QuickTime Movies|mov" or type "All Files|"
    break
  case "audio" -- implement later
  case "video"
  case "text"
  case "eps"
  default
    answer file pPrompt
    break
  end switch
  return it
end revAnswerFile

# MJVH 13/09/2006 Adding in the ability to open multiple stacks at once. Thus we need to use fileS instead of file.
function revAnswerFiles pType, pPrompt
  switch pType
  case "stack"
    answer files pPrompt with type "LiveCode Stacks|livecode,rev,mc|RSTK,MSTK" or type "All Files|"
    break
  case "image"
    answer files pPrompt with type "All Picture Files|png,gif,jpg,jpeg,bmp,pct,xbm,xpm,xwd,pbm,ppm,pgm" or type "Portable Network Graphics|png" or type "Joint Photographics Experts Group|jpg,jpeg" or type "Graphics Interchange Format|gif" or type "Windows Bitmap|bmp" or type "Unix Formats|xbm,xpm,xwd,pbm,ppm,pgm" or type "Macintosh PICT - displays on MacOS only|pct" or type "All Files|"
    break
  case "quicktime"
    answer files pPrompt with type "QuickTime Movies|mov" or type "All Files|"
    break
  case "audio" -- implement later
  case "video"
  case "text"
  case "eps"
  default
    answer files pPrompt
    break
  end switch
  return it
end revAnswerFiles


command scrollUp
   local tScrollValue
   put round(the formattedheight of the target / 20) into tScrollValue
   set the vScroll of the target to (the vScroll of the target + tScrollValue)
end scrollUp

command scrollDown
   local tScrollValue
   put round(the formattedheight of the target / 20) into tScrollValue
   set the vScroll of the target to (the vScroll of target - tScrollValue)
end scrollDown

command scrollLeft
   local tScrollValue
   put round(the formattedWidth of the target / 20) into tScrollValue
   set the hScroll of the target to (the hScroll of the target + tScrollValue)
end scrollLeft

command scrollRight
   local tScrollValue
   put round(the formattedwidth of the target / 20) into tScrollValue
   set the hScroll of the target to (the hScroll of target - tScrollValue)
end scrollRight

# Parameters
#   pObject : reference to any Revolution object that can contain text (field, button, group, card, stack)
#   pSize : The font size to use. If empty, uses default system font size
#  pStyles : A list of styles to apply (eg bold,italic,underline). If empty, uses system default.
# Description
#   Applies the preferred IDE system font style (face, size and weight / decorations) to the specified object in the correct way for the
#   curent platform and system version.
command revFontApplySystemStyle pObject, pSize, pStyles
   local tFont
   put revFontGetSystemFont() into tFont
   
   local tSize
   if pSize is empty then
      put revFontGetSystemFontSize() into tSize
   else
      put pSize into tSize
   end if
   
   local tStyles
   if pStyles is empty then
      put revFontGetSystemFontStyles() into tStyles
   else
      put pStyles into tStyles
   end if
   
   _revFontApplyStylesToObject pObject, tFont, tSize, tStyles
end revFontApplySystemStyle

function revFontGetSystemFont
   switch _revFontGetPlatform()
      case "Windows"
         if word 1 of the systemVersion is "NT" and word 2 of the systemVersion >= 5 and "Segoe UI" is among the lines of the fontNames then
            return "Segoe UI"
         else if word 1 of the systemVersion is "NT" and word 2 of the systemVersion >= 5 and "Tahoma" is among the lines of the fontNames then
            return "Tahoma"
         else
            return "MS Sans Serif"
         end if
         break
         
      case "MacOS X"
         return "Lucida Grande"
         break
         
      case "Linux"
         return "Helvetica"
         break
         
   end switch
end revFontGetSystemFont

function revFontGetSystemFontSize
   switch _revFontGetPlatform()
      case "Windows"
         if word 1 of the systemVersion is "NT" and word 2 of the systemVersion >= 5 and "Segoe UI" is among the lines of the fontNames then
            return 12
         else if word 1 of the systemVersion is "NT" and word 2 of the systemVersion >= 5 and "Tahoma" is among the lines of the fontNames then
            return  11
         else
            return 10
         end if
         break
         
      case "MacOS X"
         return 11
         break
         
      case "Linux"
         return 12
         break
         
   end switch
end revFontGetSystemFontSize

function  revFontGetSystemFontStyles
   return empty
end revFontGetSystemFontStyles

# Applies the specified text styles to pObject, depending on the object's type.
private command _revFontApplyStylesToObject pObject, pFont, pSize, pStyle
   set the textFont of pObject to pFont
   set the textSize of pObject to pSize
   set the textStyle of pObject to pStyle
   
   if word 1 of the name of pObject is "field" then
      set the textFont of char 1 to -1 of pObject to empty
      set the textSize of char 1 to -1  of pObject to empty
      --set the textStyle of char 1 to -1 of pObject to empty
   end if
end _revFontApplyStylesToObject

# This is wrapped for now as its good for testing.
private function _revFontGetPlatform
   return   _revEnvironmentPlatform()
end _revFontGetPlatform

# Duplicated from home stack to make available in standalones for now.
private function _revEnvironmentPlatform
   local tPlatform
   
   switch the platform
      case "Linux"
         put "Linux" into tPlatform
         break
      case "MacOS"
         if char 1 of the systemVersion is 1 then
            put "MacOS X" into tPlatform
         else
            put "MacOS Classic" into tPlatform
         end if
         break
      case "Win32"
         put "Windows" into tPlatform
         break
      default
         put "Unsupported" into tPlatform
         break
   end switch
   
   return tPlatform
end _revEnvironmentPlatform

# Parameters
#   pPath : Any valid path that can be ensured
# Returns
#   empty if successful, an error string otherwise
# Description
#   Makes sure that a path exists by creating any folders that are required.
#   Will take a path including a filename, but will not create the file, only
#   the folders that contain it.
command revEnsurePath pPath
   if pPath is empty then
      exit revEnsurePath
   end if
   
   if there is a folder pPath or there is a file pPath then
      exit revEnsurePath
   end if
   
   local tPath, tPart
   set the itemDelimiter to slash
   repeat for each item tPart in pPath
      put tPart & "/" after tPath
      if there is no folder tPath then
         create folder tPath
      end if
   end repeat
end revEnsurePath

# EJB
# Function to return license type in 6.0
function revLicenseType
   if the environment is "development" then
      return tolower(value("line 3 of revLicenseInfo"))
   end if
   return the environment
end revLicenseType

// AL-2014-09-12: [[ Bug 13417 ]] Utility for determining chronological relation of mac versions
function macVersionLessThan pLeft, pRight
   local tDiff
   set the itemdelimiter to "."
   repeat with x = 1 to the number of items of pRight
      put item x of pLeft - item x of pRight into tDiff
      if tDiff < 0 then
         return true
      else if tDiff > 0 then
         exit repeat
      end if
   end repeat
   return false
end macVersionLessThan
