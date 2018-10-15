script "Scriptifier"
local sInit = "false"

---------------------------------------------
--- EVENTS

on openStack
   __InitUI
end openStack

on resumeStack
  __UpdateStackList
end resumeStack

on menuPick pWhich
   switch short name of the target
      case "mainstacks"
         break
   end switch
   pass menuPick
end menuPick

on mouseUp pButton
   if pButton is 1 then
      switch the short name of the target
         case "Scriptify"
            __Scriptify the label of button "mainstacks" of me
            if the result is not empty then
               answer the result
            end if
            break
      end switch
   end if
   pass mouseUp
end mouseUp

---------------------------------------------
--- UI GENERATION

private command __InitUI
   if sInit then
      return empty for value
   end if
   
   reset the templateButton
   set the style of the templateButton to "menu"
   set the menuMode of the templateButton to "option"
   create button "mainstacks"
   
   set the rect of button "mainstacks" to 5,5,155,26
   set the text of button "mainstacks" to the mainstacks
   
   set the style of the templateButton to "standard"
   create button "Scriptify"
   set the rect of button "Scriptify" to 160,5,235,26
   
   __UpdateStackList
   
   set the visible of this stack to true
end __InitUI

command ResetUI
   __ResetUI
end ResetUI

private command __ResetUI
   local tControlID
   repeat for each line tControlID in the childControlIds of this card
      delete control id tControlID of this card
   end repeat
   
   put false into sInit
   
   __InitUI
end __ResetUI

private command __UpdateStackList
   local tStacks, tStack
   repeat for each line tStack in the mainstacks
      if the scriptOnly of stack tStack then
         next repeat
      end if
      
      put tStack & return after tStacks
   end repeat
   
   set the text of button "mainstacks" to tStacks
end __UpdateStackList

---------------------------------------------
--- SCRIPTIFIER

local sUniqueNames

private command __Scriptify pStack
   __Clean
   
   __EnsureStack pStack
   if the result is not empty then
      return the result
   end if
   
   local tDirectory
   put the filename of stack pStack into tDirectory
   
   set the itemDelimiter to "."
   delete the last item of tDirectory
   __EnsureDirectory tDirectory
   if the result is not empty then
      return the result
   end if
   
   __RecurseObjects the long id of stack pStack, tDirectory, the long id of stack pStack
   
   save stack pStack
end __Scriptify

private command __Clean
   delete variable sUniqueNames
end __Clean

private command __EnsureStack pStack
   if there is no stack pStack then
      return "no stack" for error
   end if
   return empty for value
end __EnsureStack

private command __EnsureDirectory @xDirectory
   if xDirectory ends with slash then
      delete the last char of xDirectory
   end if
   
   local tPath
   set the itemDelimiter to slash
   repeat for each item tFolder in xDirectory
      put tFolder & slash after tPath
      if there is not a folder tPath then
         create folder tPath
         if the result is not empty then
            return the result for error
         end if
      end if
   end repeat
   
   return empty for value
end __EnsureDirectory

private command __RecurseObjects pObject, pDirectory, pTarget
   -- don't mess with objects that already have behaviors or have
   -- trivial scripts
   if the number of words of the script of pObject > 10 and \
         the behavior of pObject is empty then
      local tBehaviorName
      __UniqueBehaviorNameForObject pObject, tBehaviorName
      if the result is not empty then
         throw "Could not create a unique behavior name for:" & return & \
               the long name of pObject & return & "Please change the object name."
      end if
      
      __CreateBehavior pObject, tBehaviorName, pDirectory, pTarget
      if the result is not empty then
         return the result
      end if
   end if
   
   local tID
   switch word 1 of pObject
      case "stack"
         repeat for each line tID in the cardIDs of pObject
            __RecurseObjects the long id of card id tID of pObject, pDirectory, pTarget
         end repeat
         repeat for each line tID in the sharedGroupIDs of pObject
            __RecurseObjects the long id of control id tID of pObject,  pDirectory, pTarget
         end repeat
         repeat for each line tStack in the substacks of pObject
            __RecurseObjects the long id of stack tStack of pObject, pDirectory, pTarget
         end repeat
         break
      case "card"
         repeat for each line tID in the childControlIDs of pObject
            get the long id of control id tID of pObject
            if word 1 of it is in "group bkgnd" and the sharedBehavior of it then
               next repeat
            end if
            __RecurseObjects it,  pDirectory, pTarget
         end repeat
         break
      case "group"
      case "bkgnd"
         repeat for each line tID in the childControlIDs of pObject
            __RecurseObjects the long id of control id tID of pObject, pDirectory, pTarget
         end repeat
         break
   end switch
end __RecurseObjects

private command __UniqueBehaviorNameForObject pObject, @rBehaviorName
   local tBehaviorName
   -- behavior name: <stack name><Object name><Object type>Behavior
   local tStack
   put ideMainStackOfObject(pObject) into tStack
   
   local tStackName
   put the short name of tStack into tStackName
   
   put toUpper(char 1 of pObject) & char 2 to -1 of word 1 of pObject & "Behavior" after tBehaviorName
   
   repeat
      get the short name of pObject
      if it is not empty then
         put toUpper(char 1 of it) & char 2 to -1 of it before tBehaviorName
      end if
      
      if the long id of pObject is tStack then
         exit repeat
      end if
      
      put the long owner of pObject into pObject
   end repeat
   put replaceText(tBehaviorName,"[^a-zA-Z0-9]",empty) into tBehaviorName
   
   if sUniqueNames[tBehaviorName] then
      return "not unique" for error
   else
      put true into sUniqueNames[tBehaviorName]
      put tBehaviorName into rBehaviorName
      return empty for value
   end if
end __UniqueBehaviorNameForObject


private command __CreateBehavior pObject, pBehaviorName, pDirectory, pTarget
   if there is a stack pBehaviorName then
      delete stack pBehaviorName
   end if
   
   create script only stack pBehaviorName
   set the script of stack pBehaviorName to the script of pObject
   
   local tFileName
   put tolower(pBehaviorName) & ".livecodescript" into tFileName
   
   local tStackName
   put the short name of pTarget into tStackName
   put replaceText(tStackName,"[^a-zA-Z0-9]",empty) into tStackName
   delete char 1 to the length of tStackName of tFileName
   
   set the filename of stack pBehaviorName to (pDirectory & slash & tFileName)
   
   lock messages
   save stack pBehaviorName
   unlock messages
   
   set the behavior of pObject to the long id of stack pBehaviorName
   set the script of pObject to empty
   
   get the stackFiles of pTarget
   if it is not empty then
      put return after it
   end if
   
   set the itemDelimiter to slash
   
   put pBehaviorName, the last item of pDirectory & slash & tFileName after it
   set the stackFiles of pTarget to it
   
   return tFileName for value
end __CreateBehavior
