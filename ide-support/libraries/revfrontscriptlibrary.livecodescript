script "revfrontscriptlibrary"

on revLoadLibrary
	if the target is not me then
		pass "revLoadLibrary"
	end if	
   insert the script of me into front
end revLoadLibrary

on revUnloadLibrary
	if the target is not me then
		pass "revUnloadLibrary"
	end if	
   remove the script of me from front
end revUnloadLibrary

global gREVStackStatus
global gREVSuppressMessages
global gREVSuppressMessages
local lRefreshAOTimerID

-- MW-2010-12-16: [[ Bug 9217 ]] Refactored from the backscript so that
--   we are able to handle a 'quit' event when the debugger is active.
on appleEvent p1, p2, p3
   if p1 & p2 is "aevtquit" then
      if the mode of stack "answer dialog" is not 0 then exit to top -- dialog on screen
      
      -- Make sure the debugger is terminated
      revDebuggerStop
      
      send "revHandleAppleEventQuit" to me in 10 millisecs
      exit appleEvent
   end if
   
   pass appleEvent
end appleEvent

on saveStackRequest
   
   global gREVMessageDispatch
   repeat for each line l in gREVMessageDispatch["revSaveStackRequest"]
      send "revSaveStackRequest" to this cd of stack l
   end repeat
   
   local tStackName
   put the short name of the owner of the target into tStackName
   revCheckScriptSaved tStackName
   
   put empty into gREVStackStatus[revTargetStack(the target)]
   if "id 1230" is in the frontScripts and "revTools" is in the frontScripts then
      select empty
      send "revSetMarkers true" to btn "revReshape" of stack "revTools"
      remove script of btn "revReshape" of stack "revTools" from front
   end if
   
   # OK-2008-03-14 : Bug 5890. Added global variable to work around the problem that 
   # passing this message to metacard can prevent the revSave command finishing its
   # execution, leading to stacks not being saved and the save dialog lingering.
   global gREVAllowSaveStackRequest
   
   if (not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget())) or gREVAllowSaveStackRequest then 
      pass saveStackRequest
   else 
      pass saveStackRequest to metaCard
   end if
end saveStackRequest

on nameChanged pOldname, pNewName, pTarget
   global gREVSuppressMessages, gREVMessageDispatch, gREVStackName
   
   if revTargetStack(the target) is "answer dialog" then pass nameChanged
   
   local tTarget
   lock screen
   if pTarget is not empty then put pTarget into tTarget
   else put the long id of the target into tTarget
   if word 1 of tTarget is "stack" 
   then replace comma with "_" in pNewName--engine seems to do this
   
   local tPalettes
   put revPalettesList() into tPalettes
   set the wholeMatches to true
   
   local tLinkedObjects
   repeat for each line l in tPalettes
      if word 1 of tTarget is "stack" and the cREVLinked of stack l is not empty then
         put empty into tLinkedObjects
         local tObject, tSkip
         repeat for each line m in the cREVLinked of stack l
            put m into tObject
            put 0 into tSkip
            local tWordOffset, tStackOffset, tStackName
            repeat 2 times
               put wordOffset("stack", tObject, tSkip) into tWordOffset
               if tWordOffset = 0 then exit repeat
               add tSkip to tWordOffset
               add tWordOffset to tSkip
               put tWordOffset + 1 into tStackOffset
               put char 2 to -2 of word tStackOffset of tObject into tStackName
               if tStackName is pOldName then
                  put quote&pNewName&quote into word tStackOffset of tObject
                  exit repeat
               end if
            end repeat
            put tObject & cr after tLinkedObjects
         end repeat
         delete last char of tLinkedObjects
         set the cREVLinked of stack l to tLinkedObjects
      end if
      
      local tObjects
      if the cREVLinked of stack l is not empty
      then put the cREVLinked of stack l into tObjects
      else put (the selobj) into tObjects
      
      # LG-2008-03-17
      # Bug 6092
      # It is possible that the object tObjects refers to may have been
      # deleted if messages are locked.
      if tObjects is empty or there is a tObjects then                  # LG 2008-08-13, Bug 6694
         set the title of stack l to revPaletteTitle(tObjects) 
         send "revUpdatePalette" to this cd of stack l
      end if 
   end repeat
   
   # OK-2008-03-11 : Update the script editor so that the object's new name is displayed.
   put the long id of the target into tTarget
   send "revSEObjectNameChanged tTarget, pOldName, pNewName" to revScriptEditorMain()
   
   if pNewName is empty and word 1 of tTarget is "stack" then
      lock messages
      set the name of tTarget to pOldName
      put pOldName into pNewName
      unlock messages
   end if
   
   repeat for each line l in gREVMessageDispatch["revNameChanged"]
      send "revNameChanged pNewName, pOldName" to this cd of stack l
   end repeat
   
   --if name is empty need to match id
   if pOldName is empty
   then put word 1 to 3 of tTarget into pOldName
   
   --update Application Overview global variables/custom properties with new name
   if word 1 of tTarget is "stack" and pOldName is gREVStackName then
      --stack name has changed
      put pNewName into gREVStackName
   end if
   
   local tTargetStack
   put revTargetStack(tTarget) into tTargetStack
   
   local tHCAddressing
   put the hcAddressing of stack tTargetStack into tHCAddressing
   set the hcAddressing of stack tTargetStack to false
   send "revNameChanged the long id of tTarget, pOldName, pNewName" to stack "revApplicationOverview"
   set the hcAddressing of stack tTargetStack to tHCAddressing
   
   if word 1 of tTarget is "stack" then
      -- OK-2007-06-28: Bug 5040
      send "revUpdateActiveStack pOldName, pNewName" to stack "Message Box"
      
      --    --update active stack in Message Box
      --    if the cREVLockedToStack of stack "Message Box" then
      --      if the cREVLinkedStack of stack "Message Box" is pOldName then
      --        put the defaultStack into tDefaultStack
      --        set the defaultStack to "Message Box"
      --        set the cREVLinkedStack of stack "Message Box" to pNewName
      --        set the cREVActiveStack of stack "Message Box" to pNewName
      --        if the short name of this cd is among the items of "Single Line,Multiple Lines" then
      --          
      --          set the label of btn "Open Stacks" to pNewName
      --          send "revSetLabelSize" to stack "Message Box"
      --        end if
      --        set the defaultStack to tDefaultStack
      --      end if
      --    else
      --      revSetActiveStacks
      --    end if
   end if
   unlock screen
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass nameChanged
end nameChanged

on idChanged pOldId, pNewId
   global gREVSuppressMessages
   
   global gREVMessageDispatch
   repeat for each line l in gREVMessageDispatch["revIdChanged"]
      send "revIdChanged pOldId, pNewId" to this cd of stack l
   end repeat
   
   local tObject
   put the long id of the target into tObject
   send "revSEObjectIdChanged tObject, pOldId, pNewId" to revScriptEditorMain()
   
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then
      pass idChanged
   end if
end idChanged

# OK-2008-04-11 : New script editor changes
on mainstackChanged pOldStackName, pNewStackName
   local tStack
   put the long id of the target into tStack
   
   send "revSEMainstackChanged tStack, pOldStackName, pNewStackName" to revScriptEditorMain()
   pass mainstackChanged
end mainstackChanged

on editScript pObject
   local tResult
   _editScriptNew pObject
   put the result into tResult 
   if tResult is "pass" then
      pass editScript
   end if
end editScript


private command _editScriptNew pObject
   # First obtain a correct object reference. The reason for this is edit group mode
   local tObject
   put revRuggedId(pObject) into tObject
   
   # OK-2008-08-19 : Bug 6957
   # Dispatch the "revEditScript" message to any plugins that require it.
   global gREVMessageDispatch
   repeat for each line tPlugin in gREVMessageDispatch["revEditScript"]
      try
         send "revEditScript tObject" to this card of stack tPlugin
      end try
   end repeat
   
   # Search for existing editor first.
   local tExistingEditor
   put revScriptEditor(tObject) into tExistingEditor
   if tExistingEditor is not empty then
      revGoScriptEditor the long id of stack tExistingEditor
      send "revSESetCurrentObject tObject" to stack tExistingEditor
      exit _editScriptNew
   end if
   
   # MW-2012-02-21: If we are editing the "Template Stack" behavior then we create a new
   #   window in 'special' mode.
   if revRuggedId(the long id of button "Template Stack" of stack "revNewScriptEditor") is tObject then
      revEditScriptInNewWindow pObject, true
      exit _editScriptNew
   end if
   
   # If the preference is to use tabs, then we find the topmost script editor
   # and add the specified object as a tab to it.
   local tUseTabs
   
   local tUseTabsString
   put "useTabs" into tUseTabsString
   
   send "revSEGetPreference tUseTabsString" to revScriptEditorMain()
   put the result into tUseTabs
   
   if tUseTabs then
      local tEditor
      put revTopMostScriptEditor() into tEditor
      
      # Do not allow a script editor to edit its own objects, force a new window
      # this makes debugging the script editor easier.
      local tStack
      put revTargetStack(tObject) into tStack
      if tEditor is empty then
         revEditScriptInNewWindow pObject
      else if tStack is the short name of tEditor then
         revEditScriptInNewWindow pObject
      else
         # OK-2008-07-24 : Use call instead of send here to prevent problems with the defaultStack changing.
         # Specifically this fixes an issue when debugging, if the user steps into a handler located in a script that
         # is not being edited, the script gets added to the editor, which causes the the defaultStack to change
         # and potentially breaks the debug being run in the debugger.
         call "revSEAddTargetObject tObject" to tEditor
         call "revSESetCurrentObject tObject" to tEditor
         revGoScriptEditor tEditor
      end if
   else
      # If the preference is not to use tabs, then we create a new script editor for the object.
      revEditScriptInNewWindow tObject
   end if
end _editScriptNew




on resumeStack
   global gREVScriptEdit
   put true into gREVScriptEdit
   
   revSetActiveStacks
   
   global gREVMessageDispatch
   repeat for each line l in gREVMessageDispatch["revResumeStack"]
      send "revResumeStack" to this cd of stack l
   end repeat
   
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass resumeStack
end resumeStack

on suspendStack
   global gREVScriptEdit, gREVBuildingPaletteExemption
   --suspendStack sent during the palette building process
   --it doesn't count then for selecting the background
   if not gREVBuildingPaletteExemption then put false into gREVScriptEdit
   --revIDESuspendStackFront
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then
      pass suspendStack
   end if
end suspendStack

on newStack
   send "revSetActiveStacks" to me in 10 milliseconds
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then 
      pass newStack
   end if
end newStack

on revSetActiveStacks
   global gREVShutdown
   --update the topmost stack the message box is operating on if applicable
   switch
      case the mode of stack "Message Box" is 0
      case the cREVLockedtoStack of stack "Message Box"
      case the cREVDebugMode of stack "Message Box"
         --exit criteria
         break
      default
         if the short name of this cd of stack "Message Box" is among the items of "Single Line,Multiple Lines" then
            set the cREVActiveStack of stack "Message Box" to the short name of the topStack
            send "menuPick the short name of the topStack" to btn "Open Stacks" of this cd of stack "Message Box"
         end if
         break
   end switch

   send "revUpdatePaletteStack" to stack "revBackScriptLibrary" in 100 milliseconds
   
   -- Update the simulate button as appropriate
   try
      revIDEDeployUpdate
   end try
end revSetActiveStacks


# OK-2008-03-27 : Bug 6229. Refactored to allow a target to be passed to the 
# handler that deals with the opening of new stacks.
on preOpenStack
   revIDEHandleNewStack the long id of the target
   
   local tScaleFactor
   put the cREVGeneral["scalefactor"] of this stack into tScaleFactor
   if tScaleFactor is a number and tScaleFactor is not 1 then
      set the scalefactor of this stack to tScaleFactor
   end if 
  
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass preOpenStack
end preOpenStack

# OK-2007-07-06 : Reworked recent paths system
command revUpdateRecentFiles pStack
   if there is no stack pStack then
      exit revUpdateRecentFiles
   end if
   
   if char 1 to 3 of pStack is "rev" or pStack is among the items of "Message Box,Home,answer dialog,ask dialog,script debugger,file selector,color chooser" then
      exit revUpdateRecentFiles
   end if
   
   if the effective filename of stack pStack is empty then
      exit revUpdateRecentFiles
   end if
   
   revIDEAddRecentPath the effective filename of stack pStack
end revUpdateRecentFiles

on rawKeyUp pWhich
   if pWhich is 65386 then send "menuPick" && quote & "Help Contents" & quote to btn "Help" of stack "revMenuBar"
   pass rawKeyUp
end rawKeyUp

# OK-2008-03-19 : No longer needed with new debugger
--local lMessageID

on openStack
   global gREVPalettes
   
   local tStackName
   if gREVPalettes is not empty then
      put revTargetStack() into tStackName
      
      if char 1 to 3 of tStackName is "rev" or char 1 to 3 of tStackName is "copy of rev" or tStackName is "Message box" and the mode of stack tStackName is 4 and the visible of stack tStackName is false then
         send "menuPick" && quote & "Palettes" & quote to btn "View" of stack "revMenuBar"
         if the bottom of stack tStackName > item 4 of the screenRect or the right of stack tStackName > item 3 of the screenRect then
            set the bottomRight of stack tStackName to item 3 to 4 of the windowBoundingRect
         end if
      end if
   end if
   
   # OK-2008-03-19 : Modifications for new debugger
   # Activate breakpoints for newly opened stack, but only if its a real "openStack" as opposed
   # to something doing "send openstack to button revfrontscript"
   
   #  if "revCheckBreakPoints" is in the pendingMessages then cancel lMessageID
   #  send "revCheckBreakPoints" to me in 500 milliseconds
   #  put the result into lMessageID
   
   local tTarget
   put the long id of the target into tTarget
   if tTarget is not the long id of me then
      local tStack
      put revTargetStack(tTarget) into tStack
      if revDebuggerEnabled() then
         revDebuggerActivateBreakpoints the long id of stack tStack
      end if
   end if
   
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass openStack
end openStack

on revCheckBreakPoints
   # OK-2008-03-19 : Modifications for new debugger, this is no longer needed.
   #  if the traceStack is empty then
   #    if the cREVScriptDebugMode of stack "revPreferences" then 
   #      set the cREVScriptDebugMode of stack "revPreferences" to true
   #    end if
   #  end if
end revCheckBreakPoints

local lNoRecurse

on preOpenCard
   global gREVStackName
   
   if lNoRecurse then
      put false into lNoRecurse
      pass preOpencard
   end if
   
   local tTarget
   put revTargetStack() into tTarget
   if char 1 to 3 of tTarget is not "rev" and char 1 to 11 of tTarget is not "Copy of rev" and "cREVGeometryCache" is among the lines of the customPropertySets of the target \
          and (the cREVGeometryCache["order"] of this cd is empty \
          or the cREVGeometryCache["total"] of this cd is not the number of controls \
          or the number of words in the customKeys["cREVGeometryCacheIDs"] of this cd is 0) \
          then
      revCacheGeometry
   else if "cREVGeometryCache" is among the lines of the customPropertySets of stack revTargetStack() and the cREVGeometryCache["stackID"] of stack revTargetStack() is not the id of stack revTargetStack() then
      revCacheGeometry
      repeat with i = 1 to the number of cds
         set the cREVGeometryCache["order"] of cd i to empty
      end repeat
   end if
   if the cREVGeometry["preOpenCardUpdate"] of this cd of stack tTarget is true then revUpdateGeometry
   
   global gREVMessageDispatch
   repeat for each line l in gREVMessageDispatch["revPreOpenCard"]
      send "revPreOpenCard" to this cd of stack l
   end repeat
   
   
   
   
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass preOpenCard
end preOpenCard

local lPrefsSaveTime

on closeStack
   global gREVTargetStack, gREVShowStacks, gREVShutDown
   
   put "stack" && quote & revTargetStack() & quote into gREVTargetStack
   
   send "revUpdateScriptEdit" to me in 10 milliseconds
   send "revCloseUpdate" to me in 100 milliSeconds
   
   local tStack
   put revTargetStack(the long id of the target) into tStack
   put the mainStack of stack tStack into tStack
   
   local tStacks
   put tStack into tStacks
   if the substacks of stack tStack is not empty
   then put cr & the substacks of stack tStack after tStacks
   send "revCheckStackOpen tStack,tStacks" to me in 1 second
   
   --update recent files
   local tTargetStack
   put revTargetStack(the long id of the target) into tTargetStack
   revUpdateRecentFiles tTargetStack
   
   # OK-2008-09-03 : Bug 7073 - This can cause property inspector dialogs to lock up for some reason,
   # this problem is avoided by moving this line to resumeStack in the start centre.
   --   send "updateRecentStacks" to stack "revStartCenter"
   
   # OK-2008-01-25 : Bug 5337. The recent files must be kept in a cleaned state in order to preserve
   # compatibility with 2.8.1 and older versions of Revolution
   revIDECleanRecentPaths
   
   # OK-2008-08-11 : If the destroyStack property of the stack being closed is true, then the
   # stack will be removed from memory, and any script editors affected by this must be closed.
   if the destroyStack of stack tTargetStack then
      revIDEHandleObjectDeleted the long id of stack tTargetStack
   end if
   
   if gREVTargetSTack is empty or (there is not a stack gREVTargetStack) then exit closeStack
   
   if word 1 of gREVTargetStack is not "stack" then
      put "stack" && quote before gREVTargetStack
      put quote after gREVTargetStack
   end if
   
   if the seconds - lPrefsSaveTime > 20 then -- save more frequently to keep data on unexpected exit
      -- MM-2011-03-13: [[ Bug 10073]] Make sure the stack file version is maintained at 2.7 for backwards compatability reasons
      revInternal__SavePreferences
      
      put the seconds into lPrefsSaveTime
   end if
   
   if not gREVShutDown then
      if there is a gREVTargetStack and not (the destroyStack of gREVTargetStack) and the short name of gREVTargetStack is among the lines of revFilterstacksList(the short name of gREVtargetStack) then
         local tStackslist
         if the cASKDestroy of stack "revPreferences" is among the items of "ask,remove" then
            put the mainStack of gREVTargetStack & cr & the substacks of stack (the mainStack of gREVTargetStack) into tStackslist
            
            local tAllClosed
            put true into tAllClosed
            repeat for each line l in tStacksList
               if l is empty then next repeat
               if quote & l & quote is word 2 of gREVTargetStack then next repeat
               if the mode of stack l is 0 then
                  put true into tAllClosed
               else
                  put false into tAllClosed
                  exit repeat
               end if
            end repeat
            
            local tStackName
            put revTargetStack() into tStackName
            
            local tFileName
            # OK-2008-08-05 : Bug 6562 - Bug where the user was being prompted to delete the home stack on startup,
            # caused by revIDEStack returning true/false instead of empty/non-empty.
            --            if tAllClosed and revIDEStack(the effective filename of stack tStackName) is not empty then
            if tAllClosed and not revIDEStack(the effective filename of stack tStackName) then
               put the effective fileName of gREVTargetStack into tFileName
               
               # TH - 7689 improved wording of warning message
               if tFileName is empty and the cASKDestroy of stack "revPreferences" is "ask" then 
                  answer question "You have closed the last stack in this file.  Since it has not yet been saved if you proceed you will be unable to recover it.  Do you want to remove all stacks in this file from memory?  (You can turn this warning off in Preferences under the Edit menu.)" with "No" or "Yes"
               else if the cASKDestroy of stack "revPreferences" is "ask" then 
                  answer question "You have closed the last stack in file" && tFilename & ".  Do you want to remove all stacks in this file from memory?  (You can turn this warning off in Preferences under the Edit menu.)" with "No" or "Yes"
               end if
               --               if tFileName is empty then 
               --                  put "[stack not saved - no file found]" into tFileName
               --               end if
               --               if the cASKDestroy of stack "revPreferences" is "ask" then 
               --                  answer question "You have closed the last stack in file" && tFilename & ".  Do you want to remove all stacks in this file from memory?  (You can turn this warning off in Preferences under the Edit menu.)" with "No" or "Yes"
               --               end if
               
               
               
               
               if it is not "No" then
                  local tMainStack
                  # Bug 4192
                  put line 1 of tStacksList into tMainstack
                  send "revDeleteStack tMainStack" to me in 150 milliseconds
                  exit closeStack
               end if
            end if
         end if
      end if
   end if
   
   revSetActiveStacks
   
   --  if the mode of stack "revSearch" is not 0 then
   --    send "revCloseStack" to stack "revSearch"
   --  end if
   
   global gREVMessageDispatch
   repeat for each line l in gREVMessageDispatch["revCloseStack"]
      send "revCloseStack" to this cd of stack l
   end repeat
   
   --set the executionError to empty
   
   # LG-2008-04-22
   revIDEStartCenterCheckOnStackClose tStack
   --------------------------------------------------
   
   
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass closeStack
end closeStack


on revDeleteStack pWhichStacks
   repeat for each line l in pWhichStacks
      delete stack l
   end repeat
end revDeleteStack

on revCheckStackOpen pStack, pStacks
   if the mode of stack "revApplicationOverview" is not 0 and there is not a stack pStack 
   then send "revAODeleteStack pStack, pStacks" to stack "revApplicationOverview"
end revCheckStackOpen

# TH-2008-06-18 : Bug 6217, This is now handled in revbackscript which prevents the accidental 
# closing of unsaved new stacks when messages are suppressed
on closeStackRequest
   local tResult
   revIDEHandleCloseStackRequest the long id of the target
   put the result into tResult
   if tResult is "pass" then
      pass closeStackRequest
   end if
   
   if tResult is "exit to top" then
      exit to top
   end if
end closeStackRequest


on revCloseUpdate
   global gREVStacksList, gREVMessageDispatch
   if "revCloseUpdate" is in the pendingMessages then exit revCloseUpdate
   revCheckToolbar
   put empty into gREVStacksList
   put empty into gREVMessageDispatch
   
   local tOpenList
   put the openStacks into tOpenList
   repeat for each line l in tOpenList
      if char 1 to 3 of l is "rev" then
         if the cREVGetsUpdate of stack l is true then 
            put l & cr after gREVStacksList
         end if
      end if
      
      local tLoadArray
      if there is a stack l then
         put the customProperties["cREVLoadInfo"] of stack l into tLoadArray
      end if
      repeat for each line x in keys(tLoadArray)
         if tLoadArray[x] is true and l is not among the lines of gREVMessageDispatch[x] then put l into line (the number of lines in gREVMessageDispatch[x]+1) of gREVMessageDispatch[x]
      end repeat
   end repeat
   delete last char of gREVStacksList
end revCloseUpdate

-- **Bookmark Entry** Editing Handlers

on revCheckToolbar
if the topStack is empty then exit revCheckToolBar
  if the mode of the topStack is not 1 and the cREVNewObject of stack "revmenubar" is "Card"  then send "revUpdatePalette" to cd 1 of stack "revMenuBar"
  else if the mode of the topStack is 1 and the cREVNewObject of stack "revmenubar" is "Stack" then send "revUpdatePalette" to cd 1 of stack "revMenubar"
end revCheckToolbar

on mouseDown pWhich
  local tResult
  revIDEHandleMouseDown pWhich, the long id of the target
  put the result into tResult
  
  switch tResult
  case "exit"
    exit mouseDown
    break
  case "exit to top"
    exit to top
    break
  default
    if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then 
      pass mouseDown
    end if
  end switch
end mouseDown

on revClearBlock
  # OK-2008-03-31 : No longer needed, but kept here just in case other stuff refers to it.
  
  --put false into lBlockMouseUp -- don't continue to block mouseUps after editing a script
  -- might be needed if the user edits script with shortcut and moves mouse outside, mouseRelease instead of mouseUp
end revClearBLock

on revSetEdited pWhich
  if the mode of stack "revSaving" is 0 then put "edited" into gREVStackStatus[pWhich]
end revSetEdited

on mouseUp pWhich
  local tTarget
  put the long id of the target into tTarget
  
  local tResult
  revIDEHandleMouseUp pWhich, tTarget
  put the result into tResult
  
  switch tResult
  case "pass"
    pass mouseUp
    break
  case "exit"
    exit mouseUp
    break
  case "exit to top"
    exit to top
  default
    if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then
      pass mouseUp
    end if
  end switch
end mouseUp

on mouseDoubleUp pButtonNo, pTarget
  if revCheckEdit(pTarget) then
  	revIDEOpenPalette("inspector")
  else if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then 
  	pass mouseDoubleUp
  end if
end mouseDoubleUp

on openField
   local tTargetStack
  put revTargetStack() into tTargetSTack
  if the mode of stack tTargetStack is 1 and the mode of stack "revSaving" is 0 then put "edited" into gREVStackStatus[tTargetSTack]
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass openField
end openField

on closeField
  if the cREVGetsUpdate of stack revTargetStack() is not empty then set the backPattern of the target to empty
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass closeField
end closeField

on scrollBarDrag
   if the cREVGeneral["inLineImages"&the number of this cd] of the target is true then
      local tGroupName
      put the short name of the target & "images"&the number of this cd into tGroupName
      set the scroll of group tGroupName to the scroll of the target
   end if
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass scrollBarDrag
end scrollBarDrag

on menuPick
  if the cREVGetsUpdate of stack revTargetStack() is true then set the backPattern of the target to empty
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass menuPick
end menuPick

# OK-2007-07-03: Bug 4768. If Messages are supressed we need to block the dragMove message here to fix the bug.
on dragMove
  if not gREVSuppressMessages then
    pass dragMove
  end if
end dragMove



function revCheckDelete
   if the selectedField is not empty then return true
   
   local tSelObj
   put the selobj into tSelObj
   filter tSelobj with "group*"
   repeat for each line l in tSelObj
      if word 1 of l is "group" and the number of lines in the cardNames of l > 1 then
         answer warning "This group is placed on multiple cards," & cr & "really delete it?" with "Yes" or "No"
         if it is "No" then return false
      end if
      
      if word 1 of l is "group" and the uProps["control type"] of l is "Data Grid" then
         answer warning "Would you like to delete the record template associated with this data grid? This action cannot be undone." with "Yes" or "No"
         if it is "Yes" then 
            put the uProps["record template"] of l into tTemplate
            put wordoffset("card",tTemplate) into tCardWord
            put word tCardWord to -1 of tTemplate into tTemplateCard
            put wordoffset("stack",tTemplate) into tStackWord
            put word tStackWord to -1 of tTemplate into tTemplateStack
            if the number of cards in stack tTemplateStack is 2 then delete stack tTemplateStack
            else 
               delete tTemplateCard
            end if
         end if
      end if
      
   end repeat
   
   local tMenuManagerBG
   if the mode of stack "revMenuManager" is not 0 and there is a bg id (the cREVMenuID of stack "revMenuManager") \
          of stack (the cREVDefaultStack of stack "revMenuManager") then
      put the long id of bg id (the cREVMenuID of stack "revMenuManager") \
             of stack (the cREVDefaultStack of stack "revMenuManager") into tMenuManagerBG
      repeat for each line l in tSelObj
         if l is tMenuManagerBG then
            answer warning "This group is being edited with the Menu Manager.  Really delete it?" with "Yes" or "No"
            if it is "No" then return false
         end if
      end repeat
   end if
   repeat for each line l in tSelobj
      if revIsMenuBarGroup(l) then
         answer warning "This group is the menubar on MacOS.  Really delete it?" with "Yes" or "No"
         if it is "No" then return false
      end if
   end repeat
   return true
end revCheckDelete

on backspaceKey
  if revCheckDelete() then if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass backspaceKey
  else pass backSpaceKey to metaCard
end backspaceKey

on deleteKey
  if revCheckDelete() then if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass deleteKey
  else pass deleteKey to metaCard
end deleteKey

local lSeenErrors, lEmptyFile

on revVerboseDebug pWhat
   global gREVVerboseDebug
   set the cVerboseDebug of stack "revPreferences" to pWhat
   
   -- setting this 'cause its going to crash, so save now
   -- MM-2011-03-13: [[ Bug 10073]] Make sure the stack file version is maintained at 2.7 for backwards compatability reasons
  revInternal__SavePreferences
   
   set the cVerboseDebug of stack "revPreferences" to pWhat
   set the messageMessages to pWhat
   put pWhat into gREVVerboseDebug
   if pWhat then
      delete file "verboseLog.txt"
      open file "verboseLog.txt" for text write
   end if
end revVerboseDebug

on messageHandled t, m
  global gREVVerboseDebug
  if lEmptyFile > 50000 then -- about 10MB
    close file "verboseLog.txt"
    delete file "verboseLog.txt"
    open file "verboseLog.txt" for text write
    put 1 into lEmptyFile
  end if 
  if gREVVerboseDebug then 
    write "messageHandled" && the long time && t, m, the long id of the target & cr to file "verboseLog.txt"
    add 1 to lEmptyFile
  end if
  send "recordMessage t, m, the long id of the target, true" to card 1 of stack "revMessageWatcher"
end messageHandled

on messageNotHandled t, m
  global gREVVerboseDebug
  if gREVVerboseDebug then 
    write "messageNotHandled" && the long time && t, m, the long id of the target & cr to file "verboseLog.txt"
    add 1 to lEmptyFile
  end if
  send "recordMessage t, m, the long id of the target, false" to card 1 of stack "revMessageWatcher"
end messageNotHandled

on scriptParsingError pWhich
   global gREVSuppressErrors
   if gREVSuppressErrors then exit scriptParsingError
   
   local tDigest
   put MD5Digest(pWhich) into tDigest
   if tDigest is among the lines of lSeenErrors then exit scriptParsingError
   else put tDigest into line (the number of lines in lSeenErrors)+1 of lSeenErrors
   set the executionError of stack "revErrorDisplay" to empty
   set the scriptError of stack "revErrorDisplay" to pWhich
   set the errorObject of stack "revErrorDisplay" to the long id of the target
   if the mode of stack "revErrorDisplay" is not 0 then
      send "preOpenCard" to cd 1 of stack "revErrorDisplay"
      send "opencard" to cd 1 of stack "revErrorDisplay"
   else
      modeless "revErrorDisplay"
   end if
end scriptParsingError

# OK-2008-03-27 : Bug where right-click context menus don't work with messages suppressed
# Added ability to specify the target explicitly.

# Parameters
#   pTarget : the target of the event that invoked the popup menu
# Description
#   Shows the IDE's right-click context menu depending on the target type.
on revShowpopupmenu pTarget
   local tTarget
   if pTarget is not empty then
      put the long id of pTarget into tTarget
   else
      put the long id of the target into tTarget
   end if
   
   global gREVMenuTarget
   
   local tTargetStack
   put revTargetStack(tTarget) into tTargetStack
   
   local tHCAddressing
   put the hcAddressing of stack tTargetStack into tHCAddressing
   set the hcAddressing of stack tTargetStack to false
   
   put the long id of tTarget into gREVMenuTarget
   if the number of lines in (the selObj) > 1 and the long id of tTarget is among the lines of (the selObj) then
      send "revRefresh" to btn "MultiSelection" of this cd of stack "revMenuBar"
      popup button "MultiSelection" of this cd of stack "revMenubar"
      set the hcAddressing of stack tTargetStack to tHCAddressing
      exit revShowpopupmenu
   end if  
   switch word 1 of the name of tTarget
      case "button"
         if the menuName of tTarget is not empty then
            send "revRefresh" to btn "menu" of this cd of stack "revMenuBar"
            popup button "menu" of this cd of stack "revMenubar"
         else
            send "revRefresh" to btn "standard" of this cd of stack "revMenuBar"
            popup button "standard" of this cd of stack "revMenubar"
         end if
      break
      case "field"
         send "revRefresh" to btn "field" of this cd of stack "revMenuBar"
         popup button "field" of this cd of stack "revMenubar"
      break
      case "group"
         send "revRefresh" to btn "group" of this cd of stack "revMenuBar"
         popup button "group" of this cd of stack "revMenubar"
      break
      case "image"
         send "revRefresh" to btn "image" of this cd of stack "revMenuBar"
         popup button "image" of this cd of stack "revMenubar"
      break
      case "card"
         send "revRefresh" to btn "card" of this cd of stack "revMenuBar"
         popup button "card" of this cd of stack "revMenubar"
      break
      case "player"
         send "revRefresh" to btn "player" of this cd of stack "revMenuBar"
         popup button "player" of this cd of stack "revMenubar"
      break
      case "graphic"
         if the style of tTarget is "polygon" then
            send "revRefresh" to btn "polygon" of this cd of stack "revMenuBar"
            popup button "polygon" of this cd of stack "revMenubar"
         else
            send "revRefresh" to btn "standard" of this cd of stack "revMenuBar"
            popup button "standard" of this cd of stack "revMenubar"
         end if
      break
      default
         send "revRefresh" to btn "standard" of this cd of stack "revMenuBar"
         popup button "standard" of this cd of stack "revMenubar"
      break
   end switch
   set the hcAddressing of stack tTargetStack to tHCAddressing
end revShowpopupmenu

function revOKTarget pWhich
   local tParse
   if pWhich is not empty then put pWhich into tParse
   else put the long id of the target into tParse
   get wordOffset("stack",tParse)
   
   local tStackName
   put word (it) to (it+1) of tParse into tStackName
   put the short name of tStackName into tStackName
   if char 1 to 3 of tStackName is "rev" then return true
   if tStackName is among the items of "Script Error,Execution Error,Script Errors,Home,Message Box,Script Debugger,Answer Dialog,Ask Dialog,file selector,Updater Wrapper" then return true
   
   return false
end revOKTarget

--on colorChanged
--  put revPalettesList() into tList
--  repeat for each line l in tList
--    if the name of this cd of stack l is "revColors" then
--
--    end if
--  end repeat
--end colorChanged

function revGetProperties pObject
   --return list of properties for an object, excluding all custom properties
   --also works for global and local properties
   --includes synonyms
   set the customPropertySet of stack "revLibrary" to "cREVPropertiesList"
   
   local tProperties
   put the pObject of stack "revLibrary" into tProperties
   set the wholeMatches to true
   repeat for each item i in "customKeys,customProperties,customPropertySet,customPropertySets"
      delete line lineOffset(i,tProperties) of tProperties
   end repeat
   return tProperties
end revGetProperties

on revUpdateScriptEdit
  global gREVScriptEdit
  put true into gREVScriptEdit
end revUpdateScriptEdit

on moveStack pLeft, pTop
  if the platform is "MacOS" then -- prevent stack getting stuck under menu bar
    if pTop < the bottom of stack "revMenuBar" and the right of stack revTargetStack() < the right of stack "revMenuBar" + 5 then
      set the top of stack revTargetStack() to the bottom of stack "revMenuBar" + 20
    end if
  end if
  
  -- LG-2008-02-18
  -- Added to update a stacks position in it's property inspector when its moved.
  local tPropertyInspectors
  -- Returns a list of open property inspectors.
  put revPropertyPalettes(the long id of the owner of the target) into tPropertyInspectors
  repeat for each line tPropertyInspector in tPropertyInspectors
     -- LG-2008-02-18
    -- Bug 5993
    if the short name of this card of stack tPropertyInspector is "revStackSize&Position" then
      send "revUpdatePalette" to this card of stack tPropertyInspector
    end if
  end repeat
  
  pass moveStack
end moveStack

# OK-2008-03-27 : Bug 6229. Added ability to specify a target
on revCheckStackCollision pStack
  local tTarget
  if pStack is empty then
    put the long id of the target into tTarget
  else
    put pStack into tTarget
  end if
  
  local tOffset
  put wordOffset("stack",tTarget) into tOffset
  
  local tLongStack
  put word tOffset to -1 of tTarget into tLongStack
  
  local tStacks
  put the short name of tLongStack into tStacks
  if the mainStack of tLongStack is not tStacks
  then exit revCheckStackCollision
  
  local tFilePath
  put the fileName of tLongStack into tFilePath
  if the substacks of tLongStack is not empty 
  then put cr & the substacks of tLongStack after tStacks
  
  local tMainStacks
  put the mainStacks into tMainStacks
  
  local tFileInfo
  put empty into tFileInfo
  
  local tFilePath2, tStacks2
  repeat for each line l in tMainStacks
    put the fileName of stack l into tFilePath2
    if tFilePath2 is tFilePath then next repeat
    put l into tStacks2
    if the substacks of stack l is not empty
    then put cr & the substacks of stack l after tStacks2
    repeat for each line m in tStacks2
      if m is among the lines of tStacks then
        if tFilePath2 is not empty then put "in file" && tFilePath2 & space into tFileInfo
        answer warning "A stack" && quote & m & quote && tFileInfo & \
            "is already in memory.  The Revolution UI does not distinguish stacks which have identical names, so editing these stack files while both are in memory could result in data loss."
            exit repeat
         end if
      end repeat
   end repeat
end revCheckStackCollision

private command log pMessage
  put pMessage & return after msg
end log

