script "LintLibrary"
----------------------------------------------------------------
-- Library management
----------------------------------------------------------------

on revLoadLibrary
   if the target is me then
      insert the script of me into back
   end if
end revLoadLibrary

on revUnloadLibrary
   if the target is me then
      remove the script of me from back
   end if
end revUnloadLibrary

----------------------------------------------------------------
-- Utility handlers
----------------------------------------------------------------

private command ArrayPush @xArray, pValue
   put pValue into xArray[1 + the number of elements in xArray]
end ArrayPush

private command ArrayPop @xArray
   delete variable xArray[the number of elements in xArray]
end ArrayPop

private command ArrayPropPush pProperty, pValue
   local tArray
   put the pProperty of me into tArray
   set the pProperty of me to empty
   put pValue into tArray[1 + the number of elements in tArray]
   set the pProperty of me to tArray
end ArrayPropPush

private command ArrayPropPop @xArray
   local tArray
   put the pProperty of me into tArray
   set the pProperty of me to empty
   delete variable tArray[the number of elements in tArray]
   set the pProperty of me to tArray
end ArrayPropPop

private function LintGetContents pInterpretation
   if the cInterpretation of me begins with pInterpretation then
      return the cContents of me
   else
      throw "Mismatch interpretation" & return & \
            merge("Expected: [[pInterpretation]]") & return & \
            merge("Have: [[the cInterpretation of me]]")
   end if
end LintGetContents

private command LintProblemWithLocation pLocation, pMessage, pIsFixed
   local tMessageInfo, tMessages
   put the cCurrentLint of me into tMessageInfo["lint"]
   put pLocation into tMessageInfo["location"]
   put pMessage into tMessageInfo["message"]
   put ((pIsFixed is true) and LintIsFixEnabled()) into tMessageInfo["fixed"]
   
   ArrayPropPush "cMessages", tMessageInfo
   
   -- Invoke callbacks
   local tCallback, tType
   put "lint" && the cCurrentLint of me into tType
   repeat for each element tCallback in the cMessageCallbacks of me
      dispatch tCallback["handler"] \
            to tCallback["target"] \
            with tType, pLocation, pMessage, pIsFixed
   end repeat
end LintProblemWithLocation

----------------------------------------------------------------
-- Handlers for general use
----------------------------------------------------------------

function LintIsFixEnabled
   return the cIsFixEnabled of me
end LintIsFixEnabled

function LintGetTextContents
   return LintGetContents("text")
end LintGetTextContents

function LintSetTextContents pNewContents
   if LintIsFixEnabled() then
      set the cContents of me to pNewContents
      set the cIsModified of me to true
   end if
end LintSetTextContents

function LintGetBinaryContents
   return LintGetContents("binary")
end LintGetBinaryContents

function LintGetLocation
   return the cLocation of me
end LintGetLocation

on LintPushLocation pLocation
   ArrayPropPush "cLocation", pLocation
end LintPushLocation

on LintPopLocation pLocation
   ArrayPropPop "cLocation"
end LintPopLocation

on LintProblemWithPosition pRow, pColumn, pMessage, pIsFixed
   local tLocation
   
   put the cLocation of me into tLocation
   ArrayPush tLocation, pRow
   ArrayPush tLocation, pColumn
   
   LintProblemWithLocation tLocation, pMessage, pIsFixed
end LintProblemWithPosition

on LintProblem pMessage, pIsFixed
   LintProblemWithLocation the cLocation of me, pMessage, pIsFixed
end LintProblem
