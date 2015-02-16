script "revoldidelibrary"

on revLoadLibrary
   insert the script of me into back
end revLoadLibrary

on revUnloadLibrary
   remove the script of me from back
end revUnloadLibrary

////////////////////////////////////////////////////////////////////////////////

-- Suspend Development Tools
--   Removes all IDE related backscripts and stacks so that an application can
--   run as if almost built as a standalone.
--   This affects:
--     - the frontscripts
--     - the backscripts
--     - the openstacks
--     - the current tool
--     - the windowBoundingRect
--     - the script debug mode
--
--  Note at present, holding down shift will cause *all* rev backscript and front
--  scripts to be removed including library scripts such as Table, PrintBack etc.
--
command revIDESuspendDevelopmentTools
   global gREVRestore
   
   -- Store the old setting of scriptdebugmode and disable it
   put the cREVScriptDebugMode of stack "revPreferences" into gREVRestore["scriptdebugmode"]
   
   -- As cREVScriptDebugMode is a virtual custom property handler present in the revBackScript
   -- we need to do this before removing the backscripts!
   revIDESetScriptDebugMode false
   
   # OK-2008-05-23 : Suspend any script editors.
   send "revSESuspendEditors" to revScriptEditorMain()
   
   -- Remove front and back scripts that pertain to the IDE
   -- If the shift-key is down, we remove *all* 'rev' front and back scripts, including
   -- the library scripts.
   local tScriptsList, tRemoveFrontList, tRemoveBackList
   select empty
   if the shiftKey is down then
      put the frontScripts into tScriptsList
      repeat for each line l in tScriptsList
         if char 1 to 3 of the short name of l is "rev" then
            put l & cr after tRemoveFrontList
            remove script of l from front
         end if
      end repeat
      put the backScripts into tScriptsList
      repeat for each line l in tScriptsList
         if char 1 to 3 of the short name of l is "rev" then
            put l & cr after tRemoveBackList
            remove script of l from back
         end if
      end repeat
   else
      put the frontScripts into tScriptsList
      repeat for each line l in tScriptsList
         if the short name of l is among the items of "revFrontScriptLibrary,revDebugger,revShortCutsLibrary" then
            put l & cr after tRemoveFrontList
            remove script of l from front
         end if
      end repeat
      put the backScripts into tScriptsList
      repeat for each line l in tScriptsList
         if the short name of l is "revBackScriptLibrary,revShortCutsLibrary" then
            put l & cr after tRemoveBackList
            remove script of l from back
         end if
      end repeat
   end if
   delete last char of tRemoveBackList
   delete last char of tRemoveFrontList
   
   -- Close all Revolution IDE stacks
   local tList, tMode, tClosedStacksList
   put the openStacks into tList
   lock messages
   repeat for each line tStack in tList
      if char 1 to 3 of the short name of stack tStack is "rev" or the short name of stack tStack is among the items of "Message Box,Home,answer dialog,ask dialog" then
         # OK-2008-05-23 : Compare the stack's mode to its style. If they are equal then we don't need to store the mode, as opening the 
         # stack will result in the default mode being that which matches its style.
         local tModeName
         put stackModeName(the mode of stack tStack) into tModeName
         if tModeName is the style of stack tStack then
            put empty into tMode
         else
            put tModeName into tMode
         end if
         close stack tStack
         put tStack,tMode & return after tClosedStacksList
      end if
   end repeat
   delete last char of tClosedStacksList
   
   -- Switch to browser tool
   put the tool into gREVRestore["tool"]
   choose browse tool
   
   put tRemoveBackList into gREVRestore["backscripts"]
   put tRemoveFrontList into gREVRestore["frontScripts"]
   put tClosedStacksList into gREVRestore["stacks"]
   
   -- Reset the windowBoundingRect
   put the windowBoundingRect into gREVRestore["windowBoundingRect"]
   set the windowBoundingRect to 0,0,item 3 to 4 of the windowBoundingRect
   
   -- Setup the default menubar to one specific to this mode
   set the defaultMenuBar to "Restore Development Tools"
   
   -- Show the restore dialog
   lock recent
   palette "revRestore"
   
   unlock recent
   unlock messages
end revIDESuspendDevelopmentTools

-- Restore Development Tools
--
-- Reverts the IDE to the state it was before 'Suspend Development Tools' was called.
--
command revIDERestoreDevelopmentTools
   global gREVRestore, gREVDontLoadMenus, gREVDontError
   lock messages
   repeat for each line l in gREVRestore["backscripts"]
      insert script of l into back
   end repeat
   repeat for each line l in gREVRestore["frontScripts"]
      insert script of l into front
   end repeat
   put true into gREVDontLoadMenus
   put true into gREVDontError
   
   local tMode, tStackName
   repeat for each line tStack in gREVRestore["stacks"]
      put item 2 of tStack into tMode
      put item 1 of tStack into tStackName
      
      --      put item tMode of "toplevel,toplevel,modeless,palette,modal" into tMode
      --    do "go stack" && quote & (item 1 of l) & quote && "as" && tMode
      
      # OK-2008-08-25 : Bug 7022 - If plugins throw errors when opened, this wil fail, so we catch
      # the errors with a try here, and disregard them.
      try
         
         # OK-2008-05-23 : If tMode is empty, it means that the stack's mode matched its style at the time of closing
         # and this means that no mode change is required.
         lock messages
         switch tMode
            case "toplevel"
               go stack tStackName as toplevel
               break
            case "modeless"
               go stack tStackName as modeless
               break
            case "palette"
               go stack tStackName as palette
               break
            case "modal"
               go stack tStackName as modal
               break
            default
               go stack tStackName
         end switch
         
      end try
   end repeat
   close stack "revRestore"
   set the defaultMenuBar to "revMenuBar"
   set the width of stack "revMenuBar" to the width of stack "revMenuBar" + 1 -- bug 1806
   set the width of stack "revMenuBar" to the width of stack "revMenuBar" - 1
   set the windowBoundingRect to gREVRestore["windowBoundingRect"]
   choose gREVRestore["tool"]
   
   -- Restore the script debug mode
   revIDESetScriptDebugMode gREVRestore["scriptdebugmode"]
   
   # OK-2008-05-23 : Restore any script editors.
   send "revSERestoreEditors" to revScriptEditorMain()
   
   unlock messages
end revIDERestoreDevelopmentTools

# Parameters
#   pModeNumber : the value of the "mode" property of an open stack
# Returns
#   What style that mode corresponds to.
private function stackModeName pModeNumber
   switch pModeNumber
      case 1
      case 2
         return "toplevel"
         break
      case 3
         return "modeless"
         break
      case 4
         return "palette"
         break
      case 5
         return "modal"
         break
   end switch
end stackModeName

////////////////////////////////////////////////////////////////////////////////

command revIDESetScriptDebugMode pState
   local tOldLockMessages
   put the lockMessages into tOldLockMessages
   
   set the lockMessages to false
   set the cREVScriptDebugMode of stack "revPreferences" to pState
   
   set the lockMessages to tOldLockMessages
end revIDESetScriptDebugMode

# LG-2008-04-22
command revIDEStartCenterCheckOnStackClose pStackName
   --  Want to check if there are any user stacks open.  If there are not we want to display the
   --  start center.
   
   if revOkTarget(the long id of stack pStackName) or the cStartPageShow of stack "revPreferences" is false or the cReloadStartCenter of stack "revPreferences" is false then
      exit revIDEStartCenterCheckOnStackClose
   end if
   
   # LG-2008-06-20 - Bug 6600
   # The stack being closed may still be in the openStacks as revIDEStartCenterCheckOnStackClose is no longer sent in time.
   # Therefore the stack must be filtered out of the openStacks list.
   repeat for each line openStack in the openStacks
      -- Check is openStack is a user stack
      --if char 1 to 3 of openStack is not "rev" or openStack is not among the items of "Message Box,Home,answer dialog,ask dialog,script debugger,file selector,color chooser" then
      if ((not revOkTarget(the long id of stack openStack)) and (the short name of stack openStack is not pStackName)) then
         exit revIDEStartCenterCheckOnStackClose
      end if
      
      # OK-2009-01-20 : Additionally there are certain IDE stacks which we don't want the start centre to jump up over. These are 
      # basically most windows other than the tools palette, menubar and possibly message box and property inspector.
      if (the short name of stack openStack is not pStackName) and (openStack is not among the lines of revIDESideWindows()) then
         exit revIDEStartCenterCheckOnStackClose
      end if
   end repeat
   
   # LG-2008-06-20  - Bug 6600
   # Now sent in time so the start center will appear after the stack disappears.
   send "revIDEShowStartCenter" to me in 0 milliseconds
end revIDEStartCenterCheckOnStackClose

# Returns
#   A list of the short names of IDE stacks which are considered not to be "main windows", i.e they could be open most of the time
#   Even when the user is not actually using them. This may require some tweaking to get right...
function revIDESideWindows
   local tWindows
   put "revMenubar" & return & \
         "revTools" & return & \
         "message box" & return & \
         "revApplicationOverview" & return into tWindows
   
   # The property inspector needs a little more work to deal with...
   repeat for each line tStack in the mainstacks
      if word 1 of the short name of stack tStack is "revPropertyPalette" and word 2 of the short name of stack tStack is a number then
         put the short name of stack tStack & return after tWindows
      end if
   end repeat
   delete the last char of tWindows
   
   return tWindows
end revIDESideWindows

command revIDEShowStartCenter
   show stack "revStartCenter"
   set the cMode of stack "revStartCenter" to "run"
   modeless stack "revStartCenter"
end revIDEShowStartCenter

command revIDEShowSearchEngine
   go stack "revSearchEngine"
end revIDEShowSearchEngine

# OK-2008-06-24: Bug 6575 - Message box command + m shortcut not working in script editor
# Description
#   Launches and shows the message box, the same as the Tools -> Message box menu item
command revIDEShowMessageBox
   set the cUserOpen of stack "Message Box" to true
   if the mode of stack "Message Box" is 0 then
      set the visible of stack "Message Box" to true
      go stack "Message Box"
   else if line 1 of the openStacks is not "Message Box" then
      if the selectedField is not "field 2" or the number of this card of stack "message box" is not 1 then
         lock screen
         go card 1 of stack "Message box"
         set the title of stack "Message box" to "Message Box (Single Line)"
         --set the hilitedButtonName of group "button tabs" of stack "message box" to "Single Line"
         # DW-2013-03-26
         # fixing message box not hiliting first line in 6.0rc3 due to objects being referenced by number rather than a unique name
         --focus on field 1 of stack "message box"
         --focus on field 2 of stack "message box"
         focus on field 3 of stack "message box"
         select text of field 3 of stack "message box"
      end if
   else
      set the cUserOpen of stack "Message Box" to false
      close stack "Message Box"
   end if
end revIDEShowMessageBox

# OK-2008-06-24 : Abstracted from "Window" button in menubar.
# Description
#   Sends the current window to the back.
command revIDESendCurrentWindowToBack
   lock messages
   
   local tWindowsList
   put the windows into tWindowsList
   
   local tWindows, tWindows2
   repeat for each line l in tWindowsList
      if the mode of stack l is among the items of "1,2" then
         put l & return after tWindows
      else if the mode of stack l is "3" then
         put l & return after tWindows2
      end if
   end repeat
   if char -1 of tWindows2 is return then delete char -1 of tWindows2
   if char -1 of tWindows is return then delete char -1 of tWindows
   
   local tCurrentStackName
   put char 3 to -1 of line (lineOffset("!c",the text of me)) of the text of me into tCurrentStackName
   delete line lineOffset(tCurrentStackName,tWindows) of tWindows
   
   if the num of lines in tWindows2 is not 0 
   then put tWindows2 & return & tWindows into tWindows
   
   repeat for each line l in tWindows
      -- delete char 1 to 2 of l
      if the mode of stack l is 1 or the mode of stack l is 2 then toplevel l
      else if the mode of stack l is 3 then modeless l
      show stack l
   end repeat
   unlock messages
end revIDESendCurrentWindowToBack

--local sAddedObjects

# Parameters
#   pRootObject : the object to list the tree for, this is a reference to any Revolution object
#   pIncludeSubstacks : if pRootObject is a mainstack, this determines whether or not its substacks are searched. Otherwise its ignored.
# Returns
#   Empty if all possible objects were returned, otherwise a warning string explaining why some / all were ommited.
# Description
#   rList will be set to the list of objects underneath the root object. This is inclusive, i.e. it includes pRootObject.
#   If pRootObject is a mainstack, the stack will be searched, followed by all mainstacks if pIncludeSubstacks is true.
#   If pRootObject is a stack, the object list is the stack, all its cards, and all objects on each card
#   If pRootObject is a card the list is the card and all objects on it
#   If pRootObject is any other control, the list is pRootObject.
function revIDEListSearchObjectTree pRootObject, pObeyDontSearch, pMarked, pIncludeSubstacks, pIgnorePasswordProtected, pScriptsOnly, @rList
   --delete variable sAddedObjects
   
   # Put the root object into the list first. If this is not possible because to dontSearch or card marking then
   # return an error and put empty into the list. What might also happen is that one of the child objects
   # as its dontSearch set to true. In this case, we'll return a partial list.
   if word 1 of the name of pRootObject is among the words of "field group card" and pObeyDontSearch then
      put empty into rList
      return "dontSearch of root object set"
   end if
   
   # Set the HCAddressing property of the owning stack of pRootObject to false, as this will mess
   # up everything otherwise.
   local tOldHCAddressing
   put the hcAddressing of stack revTargetStack(pRootObject) into tOldHCAddressing
   set the hcAddressing of stack revTargetStack(pRootObject) to false
   
   # If the root object is a mainstack, we add the stack itself, then all its substacks if applicable
   local tResult
   if word 1 of pRootObject is "stack" and the mainstack of pRootObject is the short name of pRootObject and pIncludeSubstacks then
      addStack pRootObject, pObeyDontSearch, pMarked, pIgnorePasswordProtected, rList, pScriptsOnly
      put the result into tResult
      
      repeat for each line tSubstack in the substacks of pRootObject
         local tSubstackResult
         addStack the name of stack tSubstack, pObeyDontSearch, pMarked, pIgnorePasswordProtected, rList, pScriptsOnly
         put the result into tSubstackResult
         if tResult is empty then
            put tSubstackResult into tResult
         end if
      end repeat
   else if word 1 of pRootObject is "stack" then
      # If its not a mainstack, or we're not adding substacks, just add the stack
      addStack pRootObject, pObeyDontSearch, pMarked, pIgnorePasswordProtected, rList, pScriptsOnly
      put the result into tResult
   else if word 1 of pRootObject is "card" then
      # If its a card then add the card and all its controls
      addCard pRootObject, pObeyDontSearch, pMarked, rList, pScriptsOnly
      put the result into tResult
   else if word 1 of pRootObject is "group" then
      # If its a group then add the group and all its control
      addGroup pRootObject, pObeyDontSearch, rList, pScriptsOnly
      put the result into tResult
   else
      # All other objects, i.e non-containers, just add the single object to the list.
      addObject pRootObject, pObeyDontSearch, rList, pScriptsOnly
      put the result into tResult
   end if
   
   set the hcAddressing of stack revTargetStack(pRootObject) to tOldHCAddressing
   
   return tResult
end revIDEListSearchObjectTree

private command addObject pObject, pObeyDontSearch, @rList, pScriptsOnly
   if word 1 of the name of pObject is among the words of "card group field" and pObeyDontSearch and the dontSearch of pObject then
      return "dontSearch set on object"
   end if
   
   local tId
   put the long id of pObject into tId
   if word 3 of tId is 0 then
      # Due to an engine bug, some objects can have IDs of zero. For now, we ignore this...
      --breakpoint
   end if
   
   
   if not pScriptsOnly or word 1 of the script of pObject is not empty then
      --if sAddedObjects[the long id of pObject] then
      --log "Duplicate object added: " & pObject
      --log "Context: " & the executionContexts
      --end if
      
      --put true into sAddedObjects[the long id of pObject]
      put the long id of pObject & return after rList
   end if
   return empty
end addObject

private command addCard pCard, pObeyDontSearch, pMarked, @rList, pScriptsOnly
   if pMarked is not empty and pMarked is not the mark of pCard then
      return "marked mismatch"
   end if
   
   # Add the card itself
   local tResult
   addObject pCard, pObeyDontSearch, rList, pScriptsOnly
   put the result into tResult
   
   # Add the non-grouped controls
   local tObjectResult
   repeat with x = 1 to the number of controls of pCard
      # Don't add groups
      if word 1 of the name of control x of pCard is among the words of "grp group background bkgnd bg" then
         next repeat
      end if
      
      # Don't add grouped controls
      if the owner of control x of pCard is not empty and word 1 of the name of the owner of control x of pCard is among the words of "grp group background bkgnd bg" then
         next repeat
      end if
      
      addObject the long id of control x of pCard, pObeyDontSearch, rList, pScriptsOnly
      put the result into tObjectResult
      
      if tResult is empty then
         put tObjectResult into tResult
      end if
   end repeat
   
   # Add any groups.
   local tGroup, tGroupResult
   repeat for each line tId in the groupIds of pCard
      put the long id of group id tId of pCard into tGroup
      if the backgroundBehavior of tGroup then
         next repeat
      end if
      
      # Note that if a group on the card does not have its backgroundBehavior set, but is placed on multiple cards,
      # it will be duplicated in the list. I'm not sure if this is correct or not...
      addGroup tGroup, pObeyDontSearch, rList, pScriptsOnly
      put the result into tGroupResult
      
      if tResult is empty then
         put tGroupResult into tResult
      end if
   end repeat
   
   return tResult
end addCard

private command addGroup pGroup, pObeyDontSearch, @rList, pScriptsOnly
   if pObeyDontSearch and the dontSearch of pGroup then
      return "dontSearch set on group"
   end if
   
   local tResult
   addObject pGroup, pObeyDontSearch, rList, pScriptsOnly
   put the result into tResult
   
   local tObjectResult
   repeat with x = 1 to the number of controls of pGroup
      addObject the long id of control x of pGroup, pObeyDontSearch, rList, pScriptsOnly
      put the result into tObjectResult
      
      if tResult is empty then
         put tObjectResult into tResult
      end if
   end repeat
   
   return tResult
end addGroup

private command addStack pStack, pObeyDontSearch, pMarked, pIgnorePasswordProtected, @rList, pScriptsOnly
   local tResult
   
   if pIgnorePasswordProtected then
      try
         get the script of pStack
      catch tError
         return "Stack password protected"
      end try
   end if
   
   # The stack itself
   addObject pStack, pObeyDontSearch, rList, pScriptsOnly
   put the result into tResult
   
   # All background groups on the stack
   local tBackground, tBackgroundResult
   repeat for each line tId in the backgroundIds of pStack
      addGroup the long id of background id tId of pStack, pObeyDontSearch, rList, pScriptsOnly
      put the result into tBackgroundResult
      
      if tResult is empty then
         put tBackgroundResult into tResult
      end if
   end repeat
   
   # All the cards on the stack
   local tCardResult
   repeat with x = 1 to the number of cards of pStack
      addCard the long id of card x of pStack, pObeyDontSearch, pMarked, rList, pScriptsOnly
      put the result into tCardResult
      if tResult is empty then
         put tCardResult into tResult
      end if
   end repeat
   
   return tResult
end addStack

function revIDEImmediateOwner pObject, pType
   # Start with the object itself
   local tObject
   put revRuggedId(pObject) into tObject
   
   repeat
      # First check if the current object is of the type we want, if so, return it
      if pType is "card" then
         if word 1 of the name of tObject is "card" then
            return tObject
         else if word 1 of the name of tObject is "stack" then
            return tObject
         end if
      end if
      
      # If the object has no owner we can't continue, this can only happen if the caller requests the card that owns a stack,
      # so is a bug.
      if the owner of tObject is empty then
         exit repeat
      end if
      
      # This means the object was not of the type we wanted, but it does have an owner, so we check its owner.
      put the owner of tObject into tObject
   end repeat
   
   # Should never get here...
end revIDEImmediateOwner

# OK-2009-10-09 : The following stuff added for the auto update checker and related systems
command revIDELaunchUpsellLink pAction
   local tBaseUrl
   put "http://developer.runrev.com/redirect.irev?action=" into tBaseUrl
   
   local tUrl
   put tBaseUrl & pAction into tUrl
   
   if pAction is among the items of "buy_early_update_pack,buy_update_pack,buy_license" then
      put "&edition=" & revIDEGetProductEditionName() after tUrl
   end if
   
   launch url tUrl
end revIDELaunchUpsellLink

function revIDEGetProductEditionName
   switch line 4 of the revLicenseInfo
      case "Enterprise"
         return "revEnterprise"
         break
      case "Studio"
         return "revStudio"
         break
      case "Media"
         return "revMedia"
         break
      default
         return empty
   end switch
end revIDEGetProductEditionName

private function idePrefToPropertyName pPref
   return "c" & pPref
end idePrefToPropertyName

function revIDEPrefGet pPref
   local tPropname
   put idePrefToPropertyName(pPref) into tPropname
   return the tPropname of stack "revPreferences"
end revIDEPrefGet

command revIDEPrefSet pPref, pValue
   local tPropname
   put idePrefToPropertyName(pPref) into tPropname
   set the tPropname of stack "revPreferences" to pValue
end revIDEPrefSet

////////////////////////////////////////////////////////////////////////////////

# OK-2007-07-06 : Reworked recent paths API based on old cRecentPaths property

# OK-2007-11-29 : Bug 5548, this variable stores a mapping from name to absolute path for the recent files list,
# it is updated every time recentFileNamesListGet() is called. This is used by revIDEGetRecentPathsAsMenu() to
# update sRecentFileItemMap.
local sRecentFileMap

# This variable stores a mapping from menu item to absolute path for the recent files list. It is updated every
# time that revIDEGetRecentPathsAsMenu() is called.
local sRecentFileItemMap

# Parameters
#   pPath : the full path of the stack file to add to the recent files list, in Revolution format.
# Description
#   Adds a stack file to the recent paths list. New stack files are added to the top of the list. If pPath is already in the 
#   list then it is not added again, instead it is moved to the top of the list. If the list is full, then the last item is 
#   removed to make way for pPath. Note that this command *does not* check if the file exists before adding it. 
command revIDEAddRecentPath pPath
  local tList
  put recentPathListGet() into tList
  
  if tList is empty then
    put pPath into tList
  else
    put pPath & return before tList
  end if
  
  recentPathListSet tList
end revIDEAddRecentPath

# Parameters
#   pPath : the full path of the stack file to remove from the list in Revolution format
# Description
#   Removes a stack file from the recent paths list. If the file is not in the list then the list will not change
#   and no error will be thrown.
command revIDERemoveRecentPath pPath
  local tList
  put recentPathListGet() into tList
  
  set the caseSensitive to true
  
  local tLineNumber
  put lineOffset(pPath, tList) into tLineNumber
  if tLineNumber = 0 then
    exit revIDERemoveRecentPath
  end if
  
  delete line tLineNumber of tList
  
  recentPathListSet tList
end revIDERemoveRecentPath

# Parameters
#   <none>
# Description
#   Returns the list of recent paths exactly as it is stored- one path per line, ordered by most recently added.
function revIDEGetRecentPaths
  return recentPathListGet()
end revIDEGetRecentPaths

# Parameters
#   pRecentPaths : a list of paths to set the recent paths list to. This should contain full paths, one per line.
# Description
#   Sets the recent path list to pRecentPaths. The order is preserved. If pRecentPaths contains duplicates these 
#   are removed. If pRecentPaths contains more paths than the maximum number allowed, then line 1 to the maximum
#   number allowed of pRecentPaths is used. Note that this command *does not* check if the paths are valid or
#   if the files exist.
command revIDESetRecentPaths pRecentPaths
  recentPathListSet pRecentPaths
end revIDESetRecentPaths

# Parameters
#   <none>
# Description
#   Removes all non-accessible items from the recent paths list. This means in particular files that don't exist.
command revIDECleanRecentPaths
  local tList
  put recentPathListGet() into tList
  
  local tCleanedList
  local tCanonicalPath
  repeat for each line tItem in tList
    put utilityCanonicalizePath(tItem) into tCanonicalPath
    if tCanonicalPath is empty then
      next repeat
    end if
    
    if revIDEStack(tCanonicalPath) then
      next repeat
    end if
    
    if tCanonicalPath is not among the lines of tCleanedList then
      put tCanonicalPath & return after tCleanedList
    end if
     
    if the number of lines of tCleanedList > recentPathListMaximumSizeGet() then
      exit repeat
    end if
  end repeat
  delete the last char of tCleanedList
   
  recentPathListSet tCleanedList
end revIDECleanRecentPaths

# Parameters
#   <none>
# Description
#   Returns a list of the recent paths in a suitable format for a menu. Note that this is in the form of a submenu
#   ie there is a tab in front of each line.
function revIDEGetRecentPathsAsMenu 
  local tNamesList
  put recentFileNamesListGet() into tNamesList
  
  if the cRecentSort of stack "revPreferences" is "alphabetically" then
    set the caseSensitive to false
    sort lines of tNamesList ascending text
    set the caseSensitive to true
  end if
  
  # OK-2007-11-29 : Save a copy of the sorted list for the mapping constructed later
  local tOriginalNames
  put tNamesList into tOriginalNames
  
  put recentPathEscapeNameList(tNamesList) into tNamesList
   
  local tLineNumber
  put 1 into tLineNumber
   
  local tMenuList, tOriginalName, tOriginalPath
  repeat for each line tLine in tNamesList
    put line tLineNumber of tOriginalNames into tOriginalName
    put sRecentFileMap[tOriginalName] into tOriginalPath
    put tOriginalPath into sRecentFileItemMap[tOriginalName]
    put tab & tLine & return after tMenuList
    add 1 to tLineNumber
  end repeat
  delete the last char of tMenuList
  
  if tMenuList is empty then 
    put tab & "(None Found" into tMenuList
  end if
   
  return tMenuList
end revIDEGetRecentPathsAsMenu

# Parameters
#   <none>
# Description
#   Returns a list of the recent paths in a suitable format for the start center.
function revIDEGetRecentPathsAsList
   local tNamesList
   put recentFileNamesListGet() into tNamesList
   
   if the cRecentSort of stack "revPreferences" is "alphabetically" then
      set the caseSensitive to false
      sort lines of tNamesList ascending text
      set the caseSensitive to true
   end if
   
   local tList
   repeat for each line tLine in tNamesList
      put tLine & tab & sRecentFileMap[tLine] & return after tList
   end repeat
   delete the last char of tList
   
   return tList
end revIDEGetRecentPathsAsList


# Parameters
#   pItem : one of the lines of the recent file menu as returned by revIDEGetRecentPathsAsMenu()
# Description
#   Returns the absolute file path of the stack corresponding to pItem
function revIDEResolveRecentPathMenuItem pItem
  return sRecentFileItemMap[pItem]
end revIDEResolveRecentPathMenuItem

# Parameters
#   pPath : The full path of a stack
# Description
#   Returns true if pPath is the path of an IDE stack, false otherwise.
function revIDEStack pPath
   # OK-2008-06-16 : There is of course one other IDE mainstack that is not a stackFile of stack "Home"....
   if pPath is the filename of stack "Home" then
      return true
   end if
   
   local tStackfilePath
   repeat for each line tStackfile in the stackfiles of stack "home"
      put item 2 to -1 of tStackfile into tStackfilePath
      if pPath is tStackfilePath then
         return true
      end if
   end repeat
   return false
end revIDEStack

# OK-2007-11-29 : Bug 5597
# Parameters
#   <none>
# Description
#   Sent when the user has requested to "Save" either by clicking in File -> Save As or by pressing ctrl-s.
#   This command attempts to save the topStack if appropriate. Note that if the shiftKey is down, the command 
#   exits, this was kept to preserve existing behavior. Sends "closeField" to the focused field of the property
#   inspector if it is open.
command revIDESaveRequest
  if the mode of the topStack is not 1 then
    exit revIDESaveRequest
  end if
  if the shiftKey is down then
    exit revIDESaveRequest
  end if
   
  if (the focusedObject) is not empty and word 1 of revTargetStack(the focusedObject) is "revPropertyPalette" and word 1 of (the focusedObject) is "field" then
    local tObject
    put the focusedObject into tObject
    send "closeField" to tObject
  end if
  
  if the effective filename of the topStack is not empty then
    revSave (the short name of the topStack)
  else
    revSaveAs (the short name of the topStack)
  end if
end revIDESaveRequest


# OK-2007-07-05: Utility functions added for reworking of recent paths API

# Parameters
#   pName : a list of recent file names
# Description
#   Escapes chars that have special meanings in Revolution menus and returns the escaped version of pName
private function recentPathEscapeNameList pName
  if utilityGetPlatform() is "MacOS" then 
    replace "/" with "\/" in pName
    replace "(" with "\(" in pName
    replace "!" with "\!" in pName
    replace "&" with "\&" in pName
  else
    replace "!" with "\!" in pName
    replace "&" with "\&" in pName
    replace "/" with "\/" in pName
  end if
  
  return pName
end recentPathEscapeNameList

private function recentPathListGet
  # OK-2007-09-11 : Bug 5337, when retrieving the list of stacks, we also check the old property name
  # cRecentPaths, as old versions of Revolution using the same preferences stack might have changed it.
  # If the old one doesn't match then its been updated, so we set the new one to the old one. 
  local tOldRecentPaths
  lock messages
  put the cRecentPaths of stack "revPreferences" into tOldRecentPaths
  unlock messages
  
  if tOldRecentPaths <> the cRecentStackPaths of stack "revPreferences" then
    set the cRecentStackPaths of stack "revPreferences" to tOldRecentPaths
  end if
  
  return the cRecentStackPaths of stack "revPreferences"
end recentPathListGet

private command recentPathListSet pList
  set the cRecentStackPaths of stack "revPreferences" to pList
  # OK-2007-09-11 : Bug 5337, keep the old property up to date with the new one
  lock messages
  set the cRecentPaths of stack "revPreferences" to pList
  unlock messages
end recentPathListSet

private function recentPathListMaximumSizeGet
  return the cNumRecent of stack "revPreferences"
end recentPathListMaximumSizeGet

# Returns whether the path pPath is case sensitive. pPath must point to a file.
private function utilityPathIsCaseSensitive pPath
  local tFolder
  
  local tOriginalFolder
  put the folder into tOriginalFolder
  
  set the itemDelimiter to slash
  put item 1 to -2 of pPath into tFolder
  
  local tIsCaseSensitive
  put false into tIsCaseSensitive
  
  set the folder to tolower(tFolder)
  if the result is not empty then
    put true into tIsCaseSensitive
  end if
  
  set the folder to toupper(tFolder)
  if the result is not empty then
    put true into tIsCaseSensitive
  end if
  
  set the folder to tOriginalFolder
  
  return tIsCaseSensitive
end utilityPathIsCaseSensitive

# Given two different paths where the filename is the same, returns a portion of pPath appropriate to differentiate
# it from pDuplicatePath. This is used for the recent file menu. This is slightly experimental in an attempt to 
# improve the existing way of doing this.
private function utilityDifferentialPath pPath, pDuplicatePath
  set the itemDelimiter to slash
  
  local tItemCount
  put the number of items of pPath into tItemCount
  
  if the number of items of pPath = 1 then
    return pPath
  end if
  
  # tDifferentialPath will be the path that we return. We always return at least the filename.
  local tDifferentialPath
  put item -1 of pPath into tDifferentialPath
  
  # Starting with the item before the filename, loop back through pPath, comparing each item with pDuplicatePath.
  # Once a difference is found, we make the different item the first item in our differential path. If the path is 
  # relative, add "../" at the start.
  repeat with tItem = -2 down to -(tItemCount)
    put item tItem of pPath & slash before tDifferentialPath
    if item tItem of pPath <> item tItem of pDuplicatePath then
      if tItem <> 1 then
        put "../" before tDifferentialPath
        exit repeat
      end if
    end if
  end repeat
  if the first char of tDifferentialPath is slash then
    delete the first char of tDifferentialPath
  end if
   
  return tDifferentialPath
end utilityDifferentialPath

# Wrapper round the platform to ease testing of platform specific code
private function utilityGetPlatform
  return the platform
end utilityGetPlatform

# Returns the *actual* path of the file specified by pPath. Returns empty if the file doesnt exist
private function utilityCanonicalizePath pPath
  local tDefaultFolder
  put the defaultFolder into tDefaultFolder
  
  set the itemDelimiter to slash
   
  local tFolder, tFilename
  put item 1 to -2 of pPath into tFolder
  put item -1 of pPath into tFilename
  
  set the defaultFolder to tFolder
   
  set the caseSensitive to utilityPathIsCaseSensitive(pPath)
   
  local tFiles
  put the files into tFiles
   
  local tLineNumber
  set the wholeMatches to true
  put lineOffset(tFilename, tFiles) into tLineNumber
  
  local tExists
  if tLineNumber = 0 then
    put false into tExists
  else
    put true into tExists
  end if
  
  local tCanonicalFolder
  put the defaultFolder into tCanonicalFolder
  set the defaultFolder to tDefaultFolder
  
  if tExists then
    return tCanonicalFolder & slash & line tLineNumber of tFiles
  else
    return empty
  end if
end utilityCanonicalizePath

# Returns a list of the recent stack files containing the names only. If two files have the same name then
# an appropriate portion of their path is shown in order to differentiate them. Populates a script local with a
# mapping from name back to original path. This is used to resolve the recent files when the user clicks on one in a gui menu.
private function recentFileNamesListGet
  local tList
  put recentPathListGet() into tList
  
  set the itemDelimiter to slash
  
  # Process the list of paths and construct a list of file names only. Where there are two items with the same
  # filename, insert some path information to allow the user to distinguish the files.
  local tFilenameList
  
  local tLineNumber
  put 0 into tLineNumber
  
  set the wholeMatches to false
  repeat for each line tPath in tList
    add 1 to tLineNumber
    local tFilename
    put item -1 of tPath into tFilename
    
    # Construct a list that contains all the elements of tList *except* the current line tLine. Get the lineOffset of the filename
    # in this list. If its not zero then it means there are other files in the list with the same filename as the current line.
    local tDuplicateLineNumber
    put lineOffset(tFilename, line 1 to (tLineNumber - 1) of tList & return & line (tLineNumber + 1) to -1 of tList) into tDuplicateLineNumber
    
    local tNewItem
    if tDuplicateLineNumber <> 0 then
      # Adjust tDuplicateLineNumber to it points to the correct line in tList
      if tDuplicateLineNumber >= tLineNumber then
        add 1 to tDuplicateLineNumber
      end if
      put utilityDifferentialPath(tPath, line tDuplicateLineNumber of tList) into tNewItem
    else
      put tFilename into tNewItem
    end if
    
    if utilityGetPlatform() is not "MacOS" and char 1 of tNewItem is "(" then
      put space before tNewItem
    end if
    
    put tNewItem & return after tFilenameList
    put tPath into sRecentFileMap[tNewItem]
  end repeat
  delete the last char of tFilenameList
  set the wholeMatches to true
  
  return tFilenameList  
end recentFileNamesListGet

////////////////////////////////////////////////////////////////////////////////

command revIDECheckForUpdates
   if line 3 of revLicenseInfo is not "evaluation" then
      local tCommand
      set the itemdelimiter to "/"
      switch the platform
         case "MacOS"
            put item 1 to -5 of the filename of stack "home" & "/LiveCode Setup.app/Contents/MacOS/installer" into tCommand
            put quote & tCommand & quote && "install -foregroundupdate" into tCommand
            break
         case "Win32"
            put item 1 to -3 of the filename of stack "home" & "/.setup.exe" into tCommand
            put quote & tCommand & quote && "install -foregroundupdate" into tCommand
            break
         case "Linux"
            put item 1 to -3 of the filename of stack "home" & "/.setup.x86" into tCommand
            put tCommand && "install -foregroundupdate" into tCommand
            break
      end switch
      set the itemDel to comma
      open process tCommand for neither
   end if   
end revIDECheckForUpdates

////////////////////////////////////////////////////////////////////////////////

private command log pMessage
   put pMessage after msg
end log

