script "revshortcutslibrary"

on revLoadLibrary
   insert the script of me into front
end revLoadLibrary

on revUnloadLibrary
   remove the script of me from front
end revUnloadLibrary

on revNewTool
   if the tool is "browse tool" then
      if "button id 1109" is in the frontScripts then
         remove script of me from front
         insert script of me into back
         go (the topStack) -- keyboard focus for shortcuts to the running stack
      end if
    else the tool is "pointer tool" then
    	remove script of me from back
    	insert script of me into front
    end if
end revNewTool

# OK-2008-03-19 : gREVSuppressMessages was being used with no declaration in multiple places
global gREVSuppressMessages


on arrowKey pDirection, pTarget
   if pTarget is empty then
      put the target into pTarget
   end if
   
   if word 1 of the name of pTarget is "button" and the arm of pTarget then pass arrowKey
   if the commandKey is down and the optionKey is down and the number of lines in (the selObj) > 1 then
      --align
      if pDirection is "up" then put "top" into pDirection
      if pDirection is "down" then put "bottom" into pDirection
      local tPosition
      do "put the" && pDirection && "of the" && line 1 of (the selObj) && "into tPosition"
      repeat for each line l in (the selObj)
         do "set the" && pDirection && "of" && l && "to" && tPosition
      end repeat
      revSendDimensionsUpdate
      exit arrowKey
   end if
   
   if the navigationArrows and (not revOKTarget() and (the optionKey is down or (the selectedField is empty and (the selObj) is empty) or (the selectedField is not empty and revTargetStack(the long id of the selectedField) is not the short name of the topStack))) then
      set the defaultStack to the topStack
      if pDirection is "left" then pass arrowKey to top
      if pDirection is "right" then pass arrowKey to top
      if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass arrowKey
   end if
   
   if the selectedField is not empty and (not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget())) then pass arrowKey
   
   local tStack
   put revTargetStack(line 1 of (the selObj)) into tStack
   
   if (tStack is line 1 of the openStacks or tStack is the short name of the topStack) \
         and word 1 of (the selobj) is not "card" then
      lock screen
      
      # OK-2008-03-19 : Previously was using lDistance with no declaration
      # It apears that this is mean to be a script local so I changed it.
      local tDistance
      
      if the shiftkey is down then
         put 10 into tDistance 
      else
         put 1 into tDistance
      end if
      
      repeat for each line tObject in (the selObj)  
         switch pDirection
            case "left"
               set the left of tObject to the left of tObject - tDistance
               break
            case "right"
               set the left of tObject to the left of tObject + tDistance
               break
            case "up"
               set the top of tObject to the top of tObject - tDistance
               break
            case "down"
               set the top of tObject to the top of tObject + tDistance
               break
         end switch
      end repeat
      revSendDimensionsUpdate
      unlock screen
   end if
   exit to top
end arrowKey

on commandKeyDown pWhich
   global gREVUIScriptShortcut, gREVScriptShortcut, gREVReverseGrouped, gREVScriptEdit, gREVShowStacks
   -- pass to object if rev stack focused
   -- pass to top -> menu bar if not
   -- don't send to menu bar to avoid duplicates if enabled
   --message box keyboard shortcuts
   if the short name of the focusedObject is "Message Field" \
         and revTargetStack(the focusedObject) is "Message Box" \
         and pWhich is among the items of "P,N,U" then
      pass commandKeyDown
   end if 
   
   --shortcuts in ask or answer
   if the mode of stack "ask dialog" is not 0 or the mode of stack "answer dialog" is not 0 then
      pass commandKeyDown
   end if
   
   --keyboard shortcut in Script Editor
   if the short name of the focusedObject is "Script" \
         and char 1 to -2 of revTargetStack(the focusedObject) is "revPropertyPalette" \
         and pWhich is "F" then
      pass commandKeyDown
   end if
   
   if the optionKey is down and pWhich is "S" and the shiftKey is not down then
      --command-option-S saves all stacks
      local tOpenStacks
      put revFilterStacksList(the openStacks) into tOpenStacks
      repeat for each line tStack in tOpenStacks
         if the effective fileName of stack tStack is not empty
         then revSave tStack
         else revSaveAs tStack
      end repeat
      exit commandKeyDown
   end if
   
   if the shiftKey is not down then
      switch pWhich
         case "="
            if pWhich is "=" and the number of lines in (the selObj) > 1 then
               local tWidth
               put the width of the selObj into tWidth
               repeat for each line tObject in (the selObj)
                  set the width of tObject to tWidth
               end repeat
            end if
            break
         case ";"
            if the selectedObjects is not empty then
               repeat for each line l in (the selObj)
                  set the textStyle of l to empty
                  set the textSize of l to empty
                  set the textColor of l to empty
                  set the textFont of l to empty
               end repeat
            else
               set the textStyle of the selectedChunk to empty
               set the textSize of the selectedChunk to empty
               set the textColor of the selectedChunk to empty
               set the textFont of the selectedChunk to empty
            end if
            break
         case "7"
            if the mode of stack "revMessageWatcher" is not 0 then
               send "mouseUp" to btn "clear" of stack "revMessageWatcher"
            end if
            break
         default
            pass commandKeyDown
      end switch
   else
      switch pWhich
         case "?"
         case "/"
            if the optionKey is not down then
               send "menuPick" && quote & "Help Contents" & quote to btn "Help" of stack "revMenuBar"
            end if
            --      if the optionKey is down then revDocsToggle
            --      else
            --        revDocsToggle "show"
            --        send "menuPick" && quote & "Help Contents" & quote to btn "Help" of stack "revMenuBar"
            --      end if
            break
         case "="
         case "+"
            if the number of lines in (the selObj) > 1 then 
               put the height of the selObj into tWidth
               repeat for each line l in (the selObj)
                  set the height of l to tWidth
               end repeat
            end if
            break
         case "S"         
         case "C"
            // AL-2014-09-16: [[ Bug 13362 ]] Don't open script editor for IDE stacks unless gREVShowStacks is true
            local tTopStack
            put the topStack into tTopStack
            if (char 1 to 3 of the short name of tTopStack is "rev") and (not gREVShowStacks) then
               exit commandKeyDown
            end if
            
            if pWhich is "S" then
               edit script of tTopStack
            else
               edit script of this card of tTopStack
            end if
            
            # OK-2008-07-07: Bug 6710 - The exit to top is preventing the script editor from finishing its initialization,
            # I'm not 100% certain how safe this fix is but it seems to be working ok.
            --exit to top
            exit commandKeyDown
            break
      end switch
   end if
   
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass commandKeyDown
end commandKeyDown

on returnKey
  # TH-2008-03-06 : Fix for bug 6014
  if not errorDisplayIsTopmost() then
    if the tool is "pointer tool" and (the selObj) is not empty \
        and (((the selectedLine) is empty and (the selectedText) is empty) \
        or the short name of the topStack is revTargetStack(line 1 of (the selObj)))
    then send "mouseDoubleUp" to the selObj
  end if
  
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then 
    pass returnKey
  end if
end returnKey

# TH-2008-03-06 : This function returns true when revErrorDisplay is the topmost stack, it
# excludes stacks above it with higher modes ie. palettes, messagebox etc. This will
# determine whether or not the reverrordisplay stack is active/focused.
private function errorDisplayIsTopmost
  repeat for each line tStack in the openstacks
    if the mode of stack tStack > the mode of stack "revErrorDisplay" then
      next repeat
    end if
    if tStack is "revErrorDisplay" then
      return true
    else
      return false
    end if
  end repeat
end errorDisplayIsTopmost

on tabKey
   if the commandKey is down and the optionKey is down then
      --toggle tool
      if the tool is "pointer tool" then 
         send "menuPick" && quote & "Browse Tool" & quote to btn "Tools" of stack "revMenuBar"
      else 
         send "menuPick" && quote & "Pointer tool" & quote to btn "Tools" of stack "revMenuBar"
      end if
      exit tabKey
   end if
   
   # OK-2008-05-05 : Ctrl+tab is the standard shortcut for switching between tabs
   # so if the script editor is focused then we hijack this shortcut for now.
   local tFocusObject
   if the focusedObject is not empty then
      put the long id of the focusedObject into tFocusObject
   end if
   
   local tStack
   
   if tFocusObject is not empty then
      put revTargetStack(the focusedObject) into tStack
      if the mainstack of stack tStack is the short name of revScriptEditorMain() then
         pass tabKey
      end if
   end if
   
   if the commandKey is down and the controlKey is down then
      --toggle palettes
      send "mouseUp" to btn "toggle palettes" of stack "revMenuBar"
   end if
   pass tabKey
end tabKey
