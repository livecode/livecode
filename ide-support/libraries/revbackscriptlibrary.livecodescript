script "revbackscriptlibrary"

on revLoadLibrary
   insert the script of me into back
end revLoadLibrary

on revUnloadLibrary
   remove the script of me from back
end revUnloadLibrary

global gREVSuppressMessages
global gREVStackStatus

local lUpdateDimensionsTimerID, lUpdateAOControlsTimerID, lLastTopStack

on revEditImage pWhichImage
   local tAppExists
   if the platform is "MacOS" and char 1 of the systemVersion is "1" then 
      if there is a folder (the cEditorPath of stack "revPreferences") \
      or there is a file (the cEditorPath of stack "revPreferences") then
         put true into tAppExists
      end if
   else if there is a file (the cEditorPath of stack "revPreferences")
   then put true into tAppExists
   
   if the cEditorPath of stack "revPreferences" is empty or not tAppExists then
      answer warning "You haven't selected an external image editor application, or I couldn't find the editor selected.  Would you like to select one?  (Note the editor you select must be able to handle PNG format images.)" with "Cancel" or "Select now"
      if it is "Cancel" then exit revEditImage
      if it is "select now" then
         if the platform is "MacOS"
         then answer file "Select an image editor:" with filter "APPL"
         else answer file "Select an image editor:"
         if it is empty then exit revEditImage
         set the cEditorPath of stack "revPreferences" to it
         set the itemDel to "/"
         put item -1 of it into fld "external editor" of stack "revPreferences"
         set the itemDel to comma
      end if
   end if
   
   local tObjects
   put pWhichImage into tObjects
   set the defaultStack to the topStack
   
   local tNumber
   repeat for each line l in tObjects
      put the number of l into tNumber
      if the fileName of image tNumber is not empty then
         local tPath
         put the cEditorPath of stack "revPreferences" into tPath
         
         local tImagePath
         put the fileName of image tNumber into tImagePath
         if tPath is among the lines of the openProcesses then kill process tPath
         launch tImagePath with tPath
         if the result is not empty then
            answer error "There was a problem launching the editor." & cr & the result with "Cancel"
            exit revEditImage
         end if
         
         local tEditFileName
         put true into tEditFileName
      else
         local tReturn
         put the fileType into tReturn
         set the fileType to "PNGf????"
         try
            export image tNumber to file "temp" & tNumber & ".png" as png
         catch tError
            answer error line (item 1 of line 1 of tError) of the cErrorsList \
            of cd 1 of stack "revErrorDisplay"
            exit revEditImage
         end try
         set the fileType to tReturn
         put the cEditorPath of stack "revPreferences" into tPath
         put the directory & "/" & ("temp" & tNumber & ".png") into tImagePath
         if tPath is among the lines of the openProcesses then kill process tPath
         launch tImagePath with tPath
         if the result is not empty then
            answer error "There was a problem launching the editor." & cr & the result with "Cancel"
            exit revEditImage
         end if
      end if
   end repeat
   if tEditFileName then answer "Now launching image editor.  Use the editor to make changes to the image, then save the image and close the editor."
   else answer "Now launching image editor.  Use the editor to make changes to the image, then save the image and close the editor.  Then click Update to update the image." with "Cancel" or "Update"
   if it is "OK" then
      repeat for each line l in tObjects
         put the number of l into tNumber
         
         local tFileName
         put the fileName of image tNumber into tFileName
         set the fileName of image tNumber to empty
         set the fileName of image tNumber to tFileName
      end repeat
      exit revEditImage
   end if
   if it is "cancel" then
      repeat for each line l in tObjects
         put the number of l into tNumber
         delete file ("temp" & tNumber & ".png")
      end repeat
   else
      repeat for each line l in tObjects
         put the number of l into tNumber
         put URL ("binfile:" & ("temp" & tNumber & ".png")) into image tNumber
         delete file ("temp" & tNumber & ".png")
      end repeat
   end if
end revEditImage

# OK-2007-11-27 : Removed...
--on run2
--   if 1 is 1 then
--    put 1 into fld 1200
--  end if
--end run2

function revImageType pImageNo
   --take the numbered image and return a three character file extension for it
   local tHeader
   put char 1 to 100 of image pImageNo into tHeader
   --PNG
   if charToNum(char 1 of tHeader) is 137 and char 2 to 4 of tHeader is "PNG" then return ".png"
   --JPEG
   if charToNum(char 1 of tHeader) is among the items of "255,216" and "JFIF" is in tHeader then return ".jpg"
   --GIF
   if "GIF" is in tHeader then return ".gif"
   --PICT
   --can't get the code to detect this right, use internal format
   --default
   --edited in mc or recompressed by MC (imported BMP, XWD etc.)
   return ".ppm"
end revImageType

function revIsPICT pWhatPath
   --takes a valid file path and returns if PICT or not
   if not (there is a file pWhatPath) then return "error"
   
   local tData
   put char 1 to 600 of url ("binfile:"&pWhatPath) into tData
   
   local tPICT
   repeat with i = 1 to 7
      if charToNum(char i of tData) is 0 then
         put true into tPICT
      else
         put false into tPict
         exit repeat
      end if
   end repeat
   if tPict then
      if charToNum(char 523 of tData) is 0 and charToNum(char 524 of tData) is 17 and charToNum(char 525 of tData) is 2 and charToNum(char 526 of tData) is 255 then
         put true into tPict
      else
         put false into tPict
      end if
      
      local tFinal
      if tPict then
         put true into tFinal
      else
         put false into tFinal
      end if
   else
      put false into tFinal
   end if
   return tFinal
end revIsPICT

on revCheckQuit
   global gREVShutDownList, gREVShutDown
   
   local tMainStacks
   put the mainStacks into tMainStacks
   
   local tListOfStacks
   put empty into tListOfStacks
   repeat for each line l in tMainStacks
      if the substacks of stack l is empty
      then put l & cr after tListOfStacks
      else put l & cr & the substacks of stack l & cr after tListOfStacks
   end repeat
   delete last char of tListOfStacks
   
   local tEditors, tFinalList
   repeat for each line l in tListOfStacks -- script editors first
      if word 1 of l is "revScriptEditor" then
         put l & cr after tEditors
      else
         put l & cr after tFinalList
      end if
   end repeat
   delete last char of tFinalList
   if the number of lines in tEditors is not 0 then
      delete last char of tEditors
      put tEditors & cr before tFinalList
   end if
   put tFinalList into tListOfStacks
   
   local tCheck
   repeat for each line l in tListOfStacks
      set the defaultStack to l
      put revSaveCheck(l,false) into tCheck
      if tCheck is false then return false -- exit appleEvent handler too
   end repeat
   set the cSelectGrouped of stack "revPreferences" to the selectGroupedControls
   
   local tMode
   repeat for each line l in gREVShutdownlist
      lock messages
      put the cREVLoadInfo["mode"] of stack l into tMode
      unlock messages
      if tMode is "invisible" then go inv stack l
      else do "go stack" && quote & l & quote && "as" && tMode
   end repeat
   
   global gREVMessageDispatch
   repeat for each line l in gREVMessageDispatch["revShutDown"]
      send "revShutDown" to this cd of stack l
   end repeat
   
   local tPalettes
   put the openStacks into tPalettes
   filter tPalettes with "revPropertyPalette*"
   
   local tOpenTabPositions
   put empty into tOpenTabPositions
   lock screen
   lock error dialogs --close the properties palettes with the errors locked in case they're giving the quit errors
   repeat for each line l in tPalettes
      if the cREVLinked of stack l is empty
      then put the cREVCurrentTabs of stack l & tab & the topLeft of stack l & cr after tOpenTabPositions
      close stack l
   end repeat
   unlock error dialogs
   
   -- closes any active database connections
   local tActiveDatabase, tWorkaround
   try
      put revdb_connections() into tActiveDatabase
      if tActiveDatabase is not empty then
         repeat for each item i in tActiveDatabase
            get revdb_disconnect(i)
         end repeat
      end if
   catch tJunk
      put empty into tWorkAround -- workaround engine bug
   end try
   
   delete last char of tOpenTabPositions
   set the cREVOpenTabPositions of stack "revPreferences" to tOpenTabPositions
   
   set the cREVScriptFont of stack "revPreferences" to the scriptTextFont
   set the cREVScriptSize of stack "revPreferences" to the scriptTextSize
   
   --  ## mf 17/08/2005 Save Docs Preferences (Bug 2834)
   -- no longer relevant in new docs 2.7
   --  put the openStacks into tOpenStacks
   --  filter tOpenStacks with "revDocs*"
   --  if there is a stack (line 1 of tOpenStacks)#save preferences in topmost revDocs
   --  then send "closeStack" to stack (line 1 of tOpenStacks)
   
   if not the secureMode then
      revInternal__savePreferences
   end if
   put true into gREVShutDown
   
   return "quit"
end revCheckQuit


on shutDownRequest
  revCheckQuit
  if the result is "quit" then pass shutDownRequest
end shutDownRequest

on revSendDimensionsUpdate
  if lUpdateDimensionsTimerID is not empty then cancel lUpdateDimensionsTimerID
  send "revUpdateSizeandPosition line 1 of the selobj" to me in 500 milliseconds
  put the result into lUpdateDimensionsTimerID
end revSendDimensionsUpdate

on revUpdateSizeandPosition pTarget
   if "development" is not in the environment
   then exit revUpdateSizeandPosition
   
   local tPalettes
   put revPropertyPalettes(pTarget) into tPalettes
   repeat for each line l in tPalettes
      if the short name of this cd of stack l is "revSize&Position" 
      then send "revUpdatePalette" to this cd of stack l
   end repeat
end revUpdateSizeandPosition

function revCheckEdit pTarget
   # MW-2010-10-13: [[ Bug 8940 ]] Check the 'cantSelect' of the target object before deciding whether it is editable
   if the tool is "browse tool" then
      return false
   end if
   
   if (pTarget is not empty and the effective cantSelect of pTarget) or \
          (pTarget is empty and the effective cantSelect of the target) then
      return false
   end if
   
   if the mode of stack revTargetstack(pTarget) is not 1 then
      return false
   end if
   
   return true
end revCheckEdit

on revUpdatePalette
end revUpdatePalette

function revPaletteObjects
   --if the cCurrentObject of the target is not empty then return the cCurrentObject of the target
   local tSelobj
   put the selobj into tSelobj
   switch
      case the cREVLinked of this stack is not empty
         return the cREVLinked of this stack
      break
      case tSelobj is empty
         if the mode of the topStack is 1
         then return the long name of the topStack
         else return empty
      break
      default
         return tSelobj
      break
   end switch
end revPaletteObjects

on revStandardUpdate
   global gREVLanguageNames
   
   --sheets
   local tSheets
   put the subStacks of stack "revTemplatePalette" & cr & "revIconChooser" into tSheets
   repeat for each line l in tSheets
      if the mode of stack l is not 0 then close stack l
   end repeat 
   
   local tObjects
   put revPaletteObjects() into tObjects
   
   --check the real/vs. language property names
   if the cLanguageNames of this cd is not gREVLanguageNames then
      if gREVLanguageNames then
         set the cLanguageNames of this cd to true
         
         -- custom prop arrays where we don't want to set the control name to the name of the array
         repeat for each line tControl in the cREVCustomPropertiesInUse of this cd
            if the cRevLabel of control tControl is not empty then set the label of control tControl to the cRevLabel of control tControl
            if the cRevToolTip of control tControl is not empty then set the toolTip of control tControl to the cRevToolTip of control tControl
            if the cRevText of control tControl is not empty then set the text of control tControl to the cRevText of control tControl
         end repeat
         
         
         repeat for each line l in the cREVPropertiesInUse of this cd
            if there is a control l then
               if word 1 of the name of control l is "button" and the style of control l is not "menu" then
                  set the cEnglishName of control l to the label of control l
                  set the label of control l to the short name of control l
                  set the toolTip of control l to item 1 of the cEnglishName of control l
               end if
               if (word 1 of the name of control l is "field" or (word 1 of the name of control l is "button" and the style of control l is "menu")) and there is a field ("revlabel" && l) then
                  set the cEnglishname of control l to the text of fld ("revLabel" && l)
                  set the text of fld ("revLabel" && l) to the short name of control l
                  set the toolTip of fld ("revLabel" && l) to the cEnglishName of control l
                  set the toolTip of control l to the cEnglishName of control l
               end if
            end if
         end repeat
      else
         set the cLanguageNames of this cd to false
         
         -- custom prop arrays where we don't want to set the control name to the name of the array
         repeat for each line tControl in the cREVCustomPropertiesInUse of this cd
            if the cEnglishLabel of control tControl is not empty then set the label of control tControl to the cEnglishLabel of control tControl
            if the cEnglishToolTip of control tControl is not empty then set the toolTip of control tControl to the cEnglishToolTip of control tControl
            if the cEnglishText of control tControl is not empty then set the text of control tControl to the cEnglishText of control tControl
         end repeat
         
         
         repeat for each line l in the cREVPropertiesInUse of this cd
            if there is a control l then 
               if word 1 of the name of control l is "button" and the style of control l is not "menu" then
                  set the toolTip of control l to item 1 of the label of control l
                  set the label of control l to the cEnglishName of control l
               end if
               if (word 1 of the name of control l is "field" or (word 1 of the name of control l is "button" and the style of control l is "menu")) and there is a field ("revlabel" && l) then
                  set the toolTip of fld ("revlabel" && l) to the text of fld ("revLabel" && l)
                  set the toolTip of control l to the text of fld ("revlabel" && l)
                  set the text of fld ("revLabel" && l) to the cEnglishName of control l
               end if
            end if
         end repeat
      end if
   end if
   
   --update the objects
   local tObject
   put line 1 of tObjects into tObject
   
   repeat for each line l in the cREVPropertiesInUse of this cd
      if l is empty then next repeat
      try
         local tProperty
         put the l of tObject into tProperty
         
         local tObjectCondition
         put empty into tObjectCondition
         set the caseSensitive to true
         repeat for each line tCurrent in tObjects
            if the l of tCurrent is not tProperty then
               put "multiple" into tObjectCondition
               exit repeat
            end if
         end repeat
         set the caseSensitive to false
         if tObjectCondition is not "multiple" then
            
            # LG-2008-03-07
            # Special case for "dashes" property. The control is a checkbox but the actual property
            # is a pair of integers.
            if l is "dashes" then
               if tProperty is empty then
                  put false into tObjectCondition
               else
                  put true into tObjectCondition
               end if
            else
               put tProperty into tObjectCondition
            end if
         end if
      catch someError
         if word 1 of the name of control l is "field" then put empty into control l
         else if word 1 of the name of control l is "button" then set the hilite of control l to false
         disable control l
         set the backPattern of control l to empty
         next repeat
      end try
      
      if l is "unicodeLabel" then
         enable control "label"
      else if l is "unicodeText" then
         enable control "text"
      else
         enable control l
      end if
      
      switch tObjectCondition
         case "multiple"
            if word 1 of the name of control l is "button" then
               set the hilite of btn l to false
               set the backPattern of btn l to 200921
            else if word 1 of the name of control l is "field" then
               put empty into fld l
               set the backPattern of fld l to 200921
            end if
            break
         case "true"
         case "false"
            if word 1 of the name of control l is "button" then
               set the hilite of btn l to tObjectCondition
               set the backPattern of btn l to empty
            else
               put tObjectCondition into control l
            end if   
            break
         default            
            if l is "unicodeLabel" then
               set the unicodeText of field "label" to tObjectCondition
            else if l is "unicodeText" then
               set the unicodeText of control "text" to tObjectCondition
            else if l is "scaleFactor" then
               if tObjectCondition is empty then set the label of control "scalefactor" to 1
               else set the label of control "scalefactor" to tObjectCondition
            else
               if word 1 of the name of control l is "button" and the style of control l is "menu" then
                  -- MM-2011-11-25: Allow the captilisation of property values (used by layerMode e.g. static appears as Static in menu).
                  if the cCapitalisePropertyValue of control l then
                     put the upper of char 1 of tObjectCondition into char 1 of tObjectCondition
                  end if
                  set the label of control l to tObjectCondition
               else 
                  put tObjectCondition into control l
               end if
            end if
            
            if l is "unicodeLabel" then
               set the backPattern of field "label" to empty
            else if l is "unicodeText" then
               set the backPattern of control "text" to empty
            else
               set the backPattern of control l to empty
            end if
      end switch
   end repeat
   
   --check to disabled objects if a multi selection
   if the number of lines in tObjects > 1 then
      repeat for each line l in the cREVMultipleDisabled of this cd
         disable control l
         set the backPattern of control l to empty
         if word 1 of the name of control l is "field" then put empty into control l
         else if word 1 of the name of control l is "button" then set the hilite of control l to false
      end repeat
   end if
   
   --   --unicode labels
   --   put line 1 of tObjects into tObject
   --   if "label" is among the lines of the cREVPropertiesInUse of this cd then
   --      if comma is in the effective textFont of tObject then
   --         set the textFont of char 1 to -1 of fld "label" to the textFont of tObject
   --      else
   --         set the textFont of char 1 to -1 of fld "label" to empty
   --      end if
   --   end if
   --   if "text" is among the lines of the cREVPropertiesInUse of this cd then
   --      if comma is in the effective textFont of tObject then
   --         set the textFont of char 1 to -1 of fld "text" to the textFont of tObject
   --      else
   --         set the textFont of char 1 to -1 of fld "text" to empty
   --      end if
   --   end if
end revStandardUpdate


setProp revDisabled pBoolean
  if word 1 of the target is "group" then
    repeat with i = 1 to the number of controls in the target
      set the disabled of control i of the target to pBoolean
    end repeat
  else
    set the disabled of the target to pBoolean
  end if
  # lock messages
  # if pBoolean is true then
  # if not (there is a control ("revDisabled" & the id of the target)) then
  # put the properties of the templateGraphic into tPrevTemplate
  # reset the templateGraphic
  # set the backColor of the templateGraphic to 124,124,124
  # set the filled of the templateGraphic to true
  # set the lineSize of the templateGraphic to 0
  # set the rect of the templateGraphic to the rect of the target
  # set the ink of the templateGraphic to "notSrcOr"
  # set the name of the templateGraphic to "revDisabled" & the id of the target
  # create graphic
  # reset the templateGraphic
  # set the properties of the templateGraphic to tPrevTemplate
  # end if
  # else
  # if there is a control ("revDisabled" & the id of the target) then delete control ("revDisabled" & the id of the target)
  # end if
  # unlock messages
  #old disablement routine: if inks aren't used
  # --a standard enable/disable routine - sets different properties dependant on object type
  # if pBoolean is true then
  # disable the target
  # if word 1 of the name of the target is "field" then
  # set the backColor of the target to 220,220,220
  # put empty into the target
  # else if word 1 of the name of the target is "button" then
  # set the hilite of the target to false
  # else if word 1 of the name of the target is "scrollBar" then
  # set the thumbPosition of the target to 0
  # end if
  # else
  # enable the target
  # if word 1 of the name of the target is "field" then set the backColor of the target to empty
  # end if
end revDisabled

on revBuildPalette
  --this handler builds list of which objects to enable and disable and set properties on in each palette
  --based on name and class of each object
  local tControlsList
  --start with the controls that disable when there are multiple objects selected
  set the defaultStack to the target
  if the revAllowMultipleList of this cd is empty then
    repeat with i = 1 to the number of controls
      if the revAllowMultiple of control i is false then put the name of control i & cr after tControlsList
    end repeat
    delete last char of tControlsList
    set the revAllowMultipleList of this cd to tControlsList
    answer "Palette" && the short name of the target && "rebuilt.  Save?" with "No" or "Yes"
    if it is "Yes" then save this stack
  end if
end revBuildPalette

on revMovePalettesForSheet
   -- removes palette windows that may get in the way of a sheet dialog
   local tStacksList
   put the openStacks into tStacksList
   
   local tStackRect
   put the rect of stack "answer dialog" into tStackRect
   
   local tMoveDistance
   repeat for each line l in tStacksList
      if the mode of stack l is 4 then
         if the topLeft of stack l is within tStackRect then
            put item 4 of tStackRect + 20 - item 2 of the topLeft of stack l into tMoveDistance
            move stack l relative 0,tMoveDistance
         end if
      end if
   end repeat
end revMovePalettesForSheet

function revSaveCheck pStack, pNoWarning
   if the secureMode then return true
   put the short name of stack pStack into pStack
   if gREVStackStatus[pStack] is "edited" and the short name of stack (the mainstack of stack pStack) is not the short name of revScriptEditorMain() then
      toplevel stack pStack
      
      local tStackPath
      put the effective fileName of stack pStack into tStackPath
      set the itemDel to "/"
      put item -1 of tStackPath into tStackPath
      set the itemDel to comma
      
      local tWarning
      if tStackPath is empty then
         put "<p><b>Do you want to save changes to stack" && quote & pStack & quote & "</b>" & "<b> before closing?</b><font size=" & quote & "11" & quote & "></p></font>" & cr & "<p><font size=" & quote & "11" & quote & "> </font></p>" & cr & "<p><font size=" & quote & "11" & quote & ">If you don't save, your changes will be lost.</p>" into tWarning
      else
         put "<p><b>Do you want to save changes to stack" && quote & pStack & quote && "and all other stacks in the file" && quote & tStackPath & quote && "before closing?</b><font size=" & quote & "11" & quote & "></p></font>" & cr & "<p> </p>" & cr & "<p><font size=" & quote & "11" & quote & ">If you don't save, your changes will be lost.</p>" into tWarning
      end if
      
      local tHideStack
      if not the visible of stack pStack then
         put true into tHideStack
         show stack pStack
      else
         put false into tHideStack
      end if
      if the mode of stack pStack is 0 or not the visible of stack pStack then -- no sheet
         if pNoWarning then
            answer warning tWarning with "Don't Save" or "Save"
         else 
            answer warning tWarning with "Don't Save" or "Cancel" or "Save"
         end if
      else
         send "revMovePalettesForSheet" to me in 50 milliseconds
         if pNoWarning then 
            answer warning tWarning with "Don't Save" or "Save" as sheet
         else 
            answer warning tWarning with "Don't Save" or "Cancel" or "Save" as sheet
         end if
      end if
      if tHideStack then hide stack pStack
      switch it
         case "Save"
            if the effective filename of stack pStack is not empty then 
               revSave pStack
               if the result is not empty then 
                  return false
               end if
            else 
               # OK-2007-09-26 : Bug 5434. It is possible that the user may change their mind after the save dialog appears. In this case
               # revSaveAs has been modified to return a value in the result, which we check for here.
               revSaveAs pStack
               if the result is not empty then
                  return false
               end if
            end if
         break
         case "Don't Save"
            put empty into gREVStackStatus[pStack]
         break
         case "Cancel"
            return false
         break
      end switch
   else if word 1 of pStack is "revScriptEditor" and the mode of stack pStack is not 0 then
      -- check script is saved
      send "revCheckScriptSaved" to cd 1 of stack pStack
      -- this will exit to top on cancel
      lock messages
      close stack pStack
      unlock messages
   end if
   return true
end revSaveCheck

on appleEvent p1, p2, p3
   if p1&p2 is "aevtodoc" then
      --appleEvent sent when stack is opened from the finder
      local tData
      request AppleEvent data
      put it into tData
      
      local tExists
      local tCanonical
      repeat for each line l in tData
         lock messages
         put utilityCanonicalizePath(l) into tCanonical
         put (there is a stack tCanonical) into tExists
         unlock messages
         // AL-2014-10-27: [[ Bug 12558 ]] Stack opened in apple event should report correct error
         if not tExists then answer error "Unable to open stack:" && the result
         else go stack tCanonical
      end repeat
      -------------------------------------------------------
      --LG 2008-02-28
      send "revUpdatePalette" to card 1 of stack "revMenuBar"
      -------------------------------------------------------
      exit appleEvent
   end if
   pass appleEvent
end appleEvent

-- MW-2010-12-16: [[ Bug 9217 ]] This is called by the appleEvent handler in
--   revFrontScript when a quit message is received. It ensures the debugger is
--   stopped before anything else happens.
on revHandleAppleEventQuit
   revCheckQuit
   if the result is "quit" then
      lock messages -- prevent duplicate shutDownRequest
      try
         remove script of stack "revDebugger" from front
         remove script of stack "revDebugger" from back
      end try
      quit
   end if
end revHandleAppleEventQuit

# OK-2007-11-27 : Tidied up
on revSave pWhichStack
   local tDefaultStack
   put the defaultStack into tDefaultStack
   
   put the short name of stack pWhichStack into pWhichStack
   
   unlock screen
   global gREVMessageDispatch
   repeat for each line tStack in gREVMessageDispatch["revSaveStackRequest"]
      send "revSaveStackRequest" to this card of stack tStack
   end repeat
   
   if "id 1230" is in the frontScripts and "revTools" is in the frontScripts then
      select empty
      send "revSetMarkers true" to button "revReshape" of stack "revTools"
      remove script of button "revReshape" of stack "revTools" from front
   end if
   
   
   
   local tStackName
   put pWhichStack into tStackName
   set the defaultStack to "revSaving"
   
   local tMainStack
   put the mainStack of stack tStackName into tMainStack
   
   local tSubStacks
   put the substacks of stack tMainStack into tSubStacks
   
   local tSubTabbed
   repeat for each line tStack in tSubStacks
      put tab & tStack & return after tSubTabbed
   end repeat
   delete last char of tSubTabbed
   put tSubTabbed into tSubStacks
   
   local tOutput
   put "Saving..." & return & "stack:" & tab & tStackName & return into tOutput
   put "mainStack:" & tab & tMainStack & return after tOutput
   put "subStacks:" & tSubStacks & return after tOutput
   put "file name:" & tab & the effective fileName of stack tStackName after tOutput
   put tOutput into field 1 of stack "revSaving"
   
   set the width of field 1 to the formattedWidth of field 1 + 8
   set the height of field 1 to the formattedHeight of field 1
   set the left of field 1 to 0
   set the top of field 1 to 0
   set the width of this stack to the right of field 1
   set the height of this stack to the height of field 1
   set the loc of this stack to the loc of stack tStackName
   set the loc of stack "revSaving" to the loc of stack pWhichStack
   palette "revSaving"
   
   local tPrompt
   put "Save complete - click to continue" into tPrompt
   
   local tStacksList
   local tFinal
   put pWhichStack into tFinal
   put the mainstack of stack pWhichStack into tStacksList
   
   local tResultLog
   
   set the cSavingStack of card 1 of stack "revSaving" to pWhichStack
   
   wait 50 milliseconds
   set the cursor to watch
   
   local tSelectedList
   put (the selObj) into tSelectedList
   select empty
   compact stack pWhichStack
   
   -- Dispatch the pre-save hook, just before we lock messages and do the save.
   dispatch "revHookPreSaveStack" to me with the long id of stack pWhichStack
   
   lock messages
   
   local tOldStackFileVersion
   put the stackFileVersion into tOldStackFileVersion
   
   if the buildnumber < 10000 then
      if the cPreserveStackVersion of stack "revPreferences" is true and the cREVGeneral["stackfileversion"] of stack pWhichStack is not empty then
         try
            set the stackFileVersion to the cREVGeneral["stackfileversion"] of stack pWhichStack
         catch tError
            --
         end try
      end if
   else
      // SN-2014-10-07: [[ Bug 13593 ]] Substacks are always saved in 2.4 format
      set the stackFileVersion to revStackFileVersion(the filename of stack (the mainstack of stack pWhichStack))
   end if
   
   
   if the scaleFactor of stack pWhichStack is a number then
      set the cREVGeneral["scalefactor"] of stack pWhichStack to the scalefactor of stack pWhichStack
   end if
   
   local tOldStackFileType
   put the stackFileType into tOldStackFileType
   if the cPreserveStackCreator of stack "revPreferences" is true then
      local tOldFolder
      put the folder into tOldFolder
      
      local tStackFilename
      put the filename of stack pWhichStack into tStackFilename
      if there is a file tStackFilename then
         set the itemDelimiter to "/"
         set the folder to item 1 to -2 of tStackFilename
         
         local tStackFileDetails
         put the detailed files into tStackFileDetails
         
         local tStackFileLeaf
         put the urlEncode of the last item of tStackFilename into tStackFileLeaf
         set the itemDelimiter to comma
         get item 11 of line lineOffset(tStackFileLeaf, tStackFileDetails) of tStackFileDetails
         if char 1 to 4 of it is not "????" then
            set the stackFileType to char 1 to 4 of it & "RSTK"
         end if
      end if
      
      set the folder to tOldFolder
   end if
   
   local tSaveResult
   
   # TH - 27/11/07 : Bug 5569, savestack request not sent to the users stack because messages are locked at this point
   try
      # OK-2008-03-14 : Bug 5890. When saving a stack with suppress messages turned on
      # execution of the revSave command terminates after the sending of saveStackRequest
      # to the target stack. This is because revFrontScript handles the message and passes
      # to metaCard, which prevents further execution. This bug is worked round with a nasty
      # hack involving a global...
      global gREVAllowSaveStackRequest
      put true into gREVAllowSaveStackRequest
      
      set the defaultStack to pWhichStack
      send "saveStackRequest" to this card of stack pWhichStack
      set the defaultStack to tDefaultStack
   end try
   
   save stack pWhichStack
   put the result into tSaveResult
   
   
   ## MJ - 17/07/2006 : Bug 3722, it appears these two lines here override the result...
   ## Hence the introduction of the above variable.
   set the stackFileType to tOldStackFileType
   set the stackFileVersion to tOldStackFileVersion
   
   unlock messages
   
   local tResult
   if tSaveResult is not empty then
      put tSaveResult & return after tResultLog
      if "backup" is in tSaveResult then put tSaveResult & return & "Check the file path, and make sure you have sufficient permissions." into tResult
      else put tSaveResult into tResult
      answer "Can't save stack" && pWhichStack && "due to an error:" & return & tResult
      close stack "revSaving"
   else
      -- Dispatch the post-save hook, straight after the save, but only if it succeeded.
      dispatch "revHookPostSaveStack" to me with the long id of stack pWhichStack
      
      put tPrompt into line -1 of fld 1 of stack "revSaving"
      put empty into gREVStackStatus[pWhichStack]
      
      local tMainStackList
      put the mainStack of stack pWhichStack into tMainStackList
      put return & the substacks of stack pWhichStack after tMainStackList
      repeat for each line tStack in tMainStackList
         if tStack is empty then next repeat
         put empty into gREVStackStatus[tStack]
      end repeat
      revUpdateRecentFiles pWhichStack
   end if
   
   repeat for each line tSelectedObject in tSelectedList
      set the selected of tSelectedObject to true
   end repeat
   
   delete last char of tResultLog
   put "Save complete - click to continue" into line -1 of field 1 of stack "revSaving"
   
   -- MW-2012-09-17: [[ Bug 10388 ]] Flush events to stop bunching.
   get flushEvents("all")
   
   -- improve speed KM oct 07 4932
   local tTime
   put the milliseconds into tTime
   repeat
      if the milliseconds - tTime > 400 then exit repeat
      if the mouseClick then exit repeat
   end repeat
   
   close stack "revSaving"
   set the defaultStack to pWhichStack
   return tResultLog
end revSave

# OK-2007-09-26 : Modified to return a value in the result if the stack is not saved
on revSaveAs pShortName, pSubstackOnly
   local tMakeMainStack, tMainStackName, tStackName
   put the short name of stack pShortName into pShortName
   
   global gREVMessageDispatch
   repeat for each line tStack in gREVMessageDispatch["revSaveStackRequest"]
      send "revSaveStackRequest" to this cd of stack tStack
   end repeat
   
   if "id 1230" is in the frontScripts and "revTools" is in the frontScripts then
      select empty
      send "revSetMarkers true" to btn "revReshape" of stack "revTools"
      remove script of btn "revReshape" of stack "revTools" from front
   end if
   
   local tDefaultStackBackup
   put the defaultStack into tDefaultStackBackup
   
   local tShortNameBackup
   put pShortName into tShortNameBackup
   
   set the defaultStack to tDefaultStackBackup
   
   put true into tMakeMainStack
   put the mainStack of stack pShortName into tMainStackName
   if tMainStackName is not pShortName and tMainStackName is not "Home" then
      if not pSubStackOnly then
         put false into tMakeMainStack
         put tMainStackName into pShortName
      end if
   end if
   
   put revFixPath(pShortName) into tStackName
   
   // MM-2012-03-09: Added new 5.5 stack file format to drop down.
   local tType
   if the buildnumber > 10000 then
      local tStackFileType
      if there is a file the filename of stack pShortName then put revStackFileVersion(the filename of stack pShortName) into tStackFileType
      
      switch tStackFileType
         case 5.5
            ask file "Save stack" && pShortName && "as:" with tStackName with type "Legacy LiveCode Stack (5.5)|livecode,rev|RSTK" or type "LiveCode Stack|livecode,rev|RSTK" or  type "Legacy LiveCode Stack (2.7)|livecode,rev|RSTK" or type "Legacy LiveCode Stack (2.4)|livecode,rev|RSTK"
            break
         case 2.7
            ask file "Save stack" && pShortName && "as:" with tStackName with type "Legacy LiveCode Stack (2.7)|livecode,rev|RSTK" or type "LiveCode Stack|livecode,rev|RSTK" or type "Legacy LiveCode Stack (5.5)|livecode,rev|RSTK" or type "Legacy LiveCode Stack (2.4)|livecode,rev|RSTK"
            break
         case 2.4
            ask file "Save stack" && pShortName && "as:" with tStackName with type "Legacy LiveCode Stack (2.4)|livecode,rev|RSTK" or type "LiveCode Stack|livecode,rev|RSTK" or type "Legacy LiveCode Stack (5.5)|livecode,rev|RSTK" or type "Legacy LiveCode Stack (2.7)|livecode,rev|RSTK"
            break
         default
            ask file "Save stack" && pShortName && "as:" with tStackName with type "LiveCode Stack|livecode,rev|RSTK" or type "Legacy LiveCode Stack (5.5)|livecode,rev|RSTK" or type "Legacy LiveCode Stack (2.7)|livecode,rev|RSTK" or type "Legacy LiveCode Stack (2.4)|livecode,rev|RSTK"
            break
      end switch
      
      put the result into tType
      
      switch tType
         case "Legacy LiveCode Stack (5.5)"
            put 5.5 into tType
            break
         case "Legacy LiveCode Stack (2.7)"
            put 2.7 into tType
            break
         case "Legacy LiveCode Stack (2.4)"
            put 2.4 into tType
            break
         default
            put 7.0 into tType
            break
      end switch
   else
      # BB:2014/02/20
      # Once LiveCode 7.0 is the main line, this build number check should be removed and the > 10000 variant used
      ask file "Save stack" && pShortName && "as:" with tStackName with type "LiveCode Stack|livecode,rev|RSTK" or type "Legacy LiveCode Stack (2.7)|livecode,rev|RSTK" or type "Legacy LiveCode Stack (2.4)|livecode,rev|RSTK"
      put the result into tType
      
      switch tType
         case "Legacy LiveCode Stack (2.7)"
            put 2.7 into tType
            break
         case "Legacy LiveCode Stack (2.4)"
            put 2.4 into tType
            break
         default
            put 5.5 into tType
            break
      end switch
   end if
   
   if it is empty then
      put "edited" into gREVStackStatus["pShortName"]
      return "Cancelled"
   end if
   
   local tFilePath
   if the platform is not "MacOS" and not (it ends with ".rev" or it ends with ".livecode") then
      put ".livecode" after it
   end if
   put it into tFilePath
   
   # Display warning if user has not selected the latest file format
   if (the buildnumber > 10000 and tType < 7) or (the buildnumber < 10000 and tType < 5.5) then
      answer "Saving in legacy format may result in loss of data if you are using recent LiveCode features. Are you sure you wish to continue?" with "No" and "Yes"
      if it is not "Yes" then
         put "edited" into gREVStackStatus[pShortName]
         return "Cancelled"
      end if
   end if
   
   if the platform is not in "Win32,MacOS" and there is a file tFilePath then
      answer warning "File exists.  Overwrite?" with "Cancel" or "OK"
      if it is "Cancel" then
         return "Cancelled"
      else 
         delete file tFilePath
      end if
   end if
   
   local tStackColors, tHomeColors
   if tMakeMainStack and pShortName is not tMainStackName then
      put the colors of stack pShortName into tStackColors
      put the colors of stack "Home" into tHomeColors
      repeat with i = 1 to 8
         if line i of tStackColors is empty
         then put line i of tHomeColors into line i of tStackColors
      end repeat
      set the colors of stack pShortName to tStackColors
      set mainStack stack pShortName to pShortName
      set the cursor to watch
      -- remove revOnline fingerprint
      revRemoveRevOnlineKey "fingerprint",tMainStackName
      save stack tMainStackName
   end if
   set the cursor to watch
   lock messages
   revRemoveRevOnlineKey "fingerprint",pShortname
   local tOldStackFileVersion
   put the stackFileVersion into tOldStackFileVersion
   set the stackFileVersion to tType
   
   # TH-2007-11-27: Bug 5569, savestack request not sent to the users stack because messages are locked at this point
   try
      # OK-2008-03-14 : Bug 5890. When saving a stack with suppress messages turned on
      # execution of the revSave command terminates after the sending of saveStackRequest
      # to the target stack. This is because revFrontScript handles the message and passes
      # to metaCard, which prevents further execution. This bug is worked round with a nasty
      # hack involving a global...
      global gREVAllowSaveStackRequest
      put true into gREVAllowSaveStackRequest
      
      set the defaultStack to pShortName
      send "saveStackRequest" to this card of stack pShortName
      set the defaultStack to tDefaultStackBackup
   end try
   
   save stack pShortName as tFilePath
   if the result is not empty then
      answer "Can't save stack" && pShortName && "with error:" & return & the result
      close stack "revSaving"
   else
      put empty into gREVStackStatus[pShortName]
      
      local tMainStackList
      put the mainStack of stack pShortName into tMainStackList
      put return & the substacks of stack pShortName after tMainStackList
      repeat for each line xy in tMainStackList
         if xy is empty then next repeat
         put empty into gREVStackStatus[xy]
      end repeat
      revUpdateRecentFiles pShortName
      revKillAllLinkedPalettes
      
      set the cREVGeneral["stackfileversion"] of stack pShortName to the stackFileVersion
      
   end if
   set the stackFileVersion to tOldStackFileVersion
   unlock messages
   
   return empty
end revSaveAs

function revStackFileVersion pStackFile
      open file pStackFile for binary read
      read from file pStackFile for 8
      close file pStackFile
      if it begins with "REVO" then
         return (byte 5 of it & "." & byte 6 of it)
      else
         return "2.4"
      end if
end revStackFileVersion

on revRemoveRevOnlineKey pKey, pStack
   local tCustomKeys
   put the customKeys["cREVOnline"] of stack pStack into tCustomKeys
   
   local tOffset
   put lineOffset(pKey,tCustomKeys) into tOffset
   if tOffset is not 0 then
      delete line tOffset of tCustomKeys
      set the customKeys["cREVOnline"] of stack pStack to tCustomKeys
   end if
end revRemoveRevOnlineKey

function revFixPath pPath
   local tReturnPath
   put pPath into tReturnPath
   
   local tRemoveChars
   if the platform is "MacOS" then
      put ":" & tab into tRemoveChars
   else if the platform is "Win32" then
      put "\/:*?<>|" & quote & tab into tRemoveChars
   else
      put " :?*<>/\()[]{}|'`" & quote & tab into tRemoveChars
      put tolower(pPath) into tReturnPath
   end if
   if revExtensionShow() then
      if not (tReturnPath ends with ".rev" or tReturnPath ends with ".livecode") then
         put ".livecode" after tReturnPath
      end if
   end if
   repeat for each char c in tRemoveChars
      replace c with "_" in tReturnPath
   end repeat
   return tReturnPath
end revFixPath

on newGroup
   selectedObjectChanged
   set the cREVGeneral["revUniqueID"] of the target to the milliseconds
   
   //Bug 10669
   repeat with x = 1 to the number of controls of the target
      set the cREVGeneral["revUniqueID"] of control x of the target to the milliseconds
   end repeat
   
   revUpdateAOControls
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass newGroup
end newGroup


on newButton
  send "revNewButton the long id of the target" to me in 50 milliseconds
  revUpdateAOControls
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass newButton
end newButton

on revNewButton pTarget
   if exists(pTarget) is false then exit revNewButton
   if the width of pTarget < 9 and the height of pTarget < 9 then
      if the style of pTarget is "menu" then
         if the menuMode of pTarget is "tabbed" then
            set the width of pTarget to the cTabButtonWidth of stack "revPreferences"
            set the height of pTarget to the cTabButtonHeight of stack "revPreferences"
         else
            set the width of pTarget to the cMenuWidth of stack "revPreferences"
            set the height of pTarget to the cMenuHeight of stack "revPreferences"
         end if
      else
         set the width of pTarget to the cButtonWidth of stack "revPreferences"
         set the height of pTarget to the cButtonHeight of  stack "revPreferences"
      end if
   end if
   set the defaultStack to revTargetStack(pTarget)
   
   local tName
   if the cREVGeneral["viewerobject"] of pTarget is true then
      put the short name of pTarget into tName
      replace "Button" with "Report" in tName
      
      local tCheck
      repeat
         put "btn" && quote & tName & quote into tCheck
         if not exists(tCheck) then exit repeat
         add 1 to word -1 of tName
      end repeat
      set the name of pTarget to tName
      set the cREVReport["viewername"] of pTarget to tName
   end if
   set the cREVGeneral["revUniqueID"] of pTarget to the milliseconds
   if the left of pTarget < 0 then set the left of pTarget to 0
   if the top of pTarget < 0 then set the top of pTarget to 0
end revNewButton

on newField
  if the width of the target < 9 and the height of the target < 9 then
    if the text target is "label text" then
      set the width of the target to the cLabelFieldWidth of stack "revPreferences"
      set the height of the target to the cLabelFieldHeight of stack "revPreferences"
    else
      set the width of the target to the cFieldWidth of stack "revPreferences"
      set the height of the target to the cFieldHeight of stack "revPreferences"
    end if
  end if
  set the cREVGeneral["revUniqueID"] of the target to the milliseconds
  if the left of the target < 0 then set the left of the target to 0
  if the top of the target < 0 then set the top of the target to 0
  revUpdateAOControls
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass newField
end newField

on newScrollbar
  if the width of the target < 9 and the height of the target < 9 then
    if the platform is "MacOS" then
      if the style of the target is not "progress" then
        set the width of the target to the cMacOSScrollbarWidth of stack "revPreferences"
        set the height of the target to the cMacOSScrollbarHeight of stack "revPreferences"
      else
        set the width of the target to the cMacOSProgressScrollbarWidth of stack "revPreferences"
        set the height of the target to the cMacOSProgressScrollbarHeight of stack "revPreferences"
      end if
    else
      set the width of the target to the cScrollbarwidth of stack "revPreferences"
      set the height of the target to the cScrollbarHeight of stack "revPreferences"
    end if
  end if
  set the cREVGeneral["revUniqueID"] of the target to the milliseconds
  if the left of the target < 0 then set the left of the target to 0
  if the top of the target < 0 then set the top of the target to 0
  revUpdateAOControls
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass newScrollbar
end newScrollbar

on newImage
  if the width of the target < 9 and the height of the target < 9 then
    set the width of the target to the cREVImageWidth of stack "revPreferences"
    set the height of the target to the cREVImageHeight of stack "revPreferences"
  end if
  set the cREVGeneral["revUniqueID"] of the target to the milliseconds
  if the left of the target < 0 then set the left of the target to 0
  if the top of the target < 0 then set the top of the target to 0
  revUpdateAOControls
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass newImage
end newImage

on newEPS
   
   local tStack
   put revTargetStack(the long id of the target) into tStack
   lock screen
   set the width of the target to (the width of stack tStack / 2)
   set the height of the target to (the height of stack tStack / 2)
   unlock screen
   set the cREVGeneral["revUniqueID"] of the target to the milliseconds
   if the left of the target < 0 then set the left of the target to 0
   if the top of the target < 0 then set the top of the target to 0
   revUpdateAOControls
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass newEPS
end newEPS



on newGraphic
   if the width of the target < 9 and the height of the target < 9 then
      local tOrigLoc
      put the loc of the target into tOrigLoc
      if the style of the target is in "polygon,curve" then set the points of the target to 0,0,100,100,100,50,50,100
      if the style of the target is "line" then set the points of the target to 0,0,120,120
      set the width of the target to the cREVGraphicWidth of stack "revPreferences"
      set the height of the target to the cREVGraphicHeight of stack "revPreferences"
      set the loc of the target to tOrigLoc
   end if
   set the cREVGeneral["revUniqueID"] of the target to the milliseconds
   if the left of the target < 0 then set the left of the target to 0
   if the top of the target < 0 then set the top of the target to 0
   revUpdateAOControls
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass newGraphic
end newGraphic

on newPlayer
  if the width of the target < 9 and the height of the target < 9 then
    set the width of the target to the cREVPlayerWidth of stack "revPreferences"
    set the height of the target to the cREVPlayerHeight of stack "revPreferences"
  end if
  set the cREVGeneral["revUniqueID"] of the target to the milliseconds
  if the left of the target < 0 then set the left of the target to 0
  if the top of the target < 0 then set the top of the target to 0
  revUpdateAOControls
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass newPlayer
end newPlayer

on newGroup
  selectedObjectChanged
  set the cREVGeneral["revUniqueID"] of the target to the milliseconds
  revUpdateAOControls
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass newGroup
end newGroup

on newCard
  if the mode of stack "revApplicationOverview" is not 0
  then send "revUpdateCards revTargetStack(the long id of the target)" to stack "revApplicationOverview"
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass newCard
end newCard

on newStack
   lock screen
   lock messages
   
   local tName
   put the short name of the target into tName
   set the name of the target to tName && "1"
   if there is a stack tName then
      repeat
         if word -1 of tName is a number then add 1 to word -1 of tName
         else put " 1" after tName
         if not (there is a stack tName) then
            set the name of the target to tName
            show stack tName
            exit repeat
         end if
      end repeat
   else
      set the name of the target to tName
   end if
   send "revUpdatePaletteStack" to me in 100 milliseconds
   if the mode of stack "revApplicationOverview" is not 0
   then send "revNewStack the short name of the target" to stack "revApplicationOverview"
   unlock screen
   unlock messages
   pass newStack
end newStack

# Parameters
#   pObject : reference to the object about to be deleted
# Description
#   Updates the property inspector, application browser and script editor
#   in response to pObject being deleted.
command revIDEHandleObjectDeleted pObject
   -- Need to pass the object as the target may have changed.
   revDeleteLinkedObject pObject
   revUpdateAOControls
   revIDEUpdateScriptEditorOnObjectDeletion pObject
end revIDEHandleObjectDeleted

# Parameters
#   pObject : reference to the object about to be deleted
# Description
#   Updates the script editor in response to pObject being deleted.
private command revIDEUpdateScriptEditorOnObjectDeletion pObject
   send "revSEObjectDeleted pObject" to revScriptEditorMain()
end revIDEUpdateScriptEditorOnObjectDeletion

# OK-2008-05-05
# Parameters
#   pType : either "compilation", "execution" or "warning"
#   pCode : the id of the error as returned by the engine.
# Returns
#   The description of the specified error. Note that if pType is warning,
#   this function will currently return empty.
function revIDELookupError pType, pCode
   if pType is "compilation" then
      return line pCode of the cScriptErrors of card 1 of stack "revErrorDisplay"
   else if pType is "execution" then
      return line pCode of the cErrorsList of card 1 of stack "revErrorDisplay"
   else if pType is "warning" then
      return empty
   end if
end revIDELookupError

on deleteButton
   revIDEHandleObjectDeleted the long id of the target
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then
      pass deleteButton
   end if
end deleteButton

on deleteField
   revIDEHandleObjectDeleted the long id of the target
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then
      pass deleteField
   end if
end deleteField

on deleteGroup
   revIDEHandleObjectDeleted the long id of the target
   
   if the mode of stack "revMenuManager" is not 0 and there is a stack (the cREVDefaultStack of stack "revMenuManager") and (there is a background id (the cREVMenuID of stack "revMenuManager") of stack (the cREVDefaultStack of stack "revMenuManager")) and the long id of the target is the long id of background id (the cREVMenuID of stack "revMenuManager") of stack (the cREVDefaultStack of stack "revMenuManager") then 
      send "revDeleteMenuManagerGroup" to card 1 of stack "revMenuManager"
   end if
   
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then
      pass deleteGroup
   end if
end deleteGroup

on deleteGraphic
   revIDEHandleObjectDeleted the long id of the target
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then
      pass deleteGraphic
   end if
end deleteGraphic

on deletePlayer
   revIDEHandleObjectDeleted the long id of the target
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then
      pass deletePlayer
   end if
end deletePlayer

on deleteScrollbar
   revIDEHandleObjectDeleted the long id of the target
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then
      pass deleteScrollbar
   end if
end deleteScrollbar

on deleteImage
   revIDEHandleObjectDeleted the long id of the target
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then
      pass deleteImage
   end if
end deleteImage

on deleteCard
   revDeleteLinkedObject
   if the mode of stack "revApplicationOverview" is not 0 then
      send "revUpdateCards revTargetStack(the long id of the target)" to stack "revApplicationOverview" in 500 milliseconds
   end if
   
   revIDEUpdateScriptEditorOnObjectDeletion the long id of the target
   
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then
      pass deleteCard
   end if
end deleteCard

on deleteStack
   # OK-2008-07-29 : Bug 6796 - The deletestack message is sent to the card not the stack
   revIDEHandleObjectDeleted the long id of the long owner of the target
   
   # BB-2014-07-25 : Bug 9598 - Catch an error being thrown when LiveCode IDE is shutting down
   # Fix courtesy of Mark Wieder
   try
      revSetActiveStacks
   catch e
   end try
   
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then
      pass deleteStack
   end if
end deleteStack

on newTool
   global gREVMessageDispatch
   repeat for each line l in gREVMessageDispatch["revNewTool"]
      send "revNewTool" to this cd of stack l
   end repeat
   
	if "revShortCutsLibrary" is in revInternal__ListLoadedLibraries() then	
   		send "revNewTool" to stack "revShortCutsLibrary"
   	end if
   
   if the tool is "browse tool" then
      set the defaultStack to the topStack
      if the editBg then set the editBg to false
      if the mode of stack "revApplicationOverview" is not 0 \
            and the short name of this cd of stack "revApplicationOverview" is "Cards and Controls" then
         send "revRefreshControls true, true" to stack "revApplicationOverview"
      end if
	end if
   
   if "revReshapeLibrary" is in revInternal__ListLoadedLibraries() then
	   send "revNewTool" to stack "revReshapeLibrary"
	end if	
   
   if word 1 of the tool is not in "pointerbrowseselectpencilbucketrectanglelineroundedovalbrusheraserspraypolygoncurveregulardropper" then
      revNewObjectName
      do "if there is a template" & word 1 of the tool && "then select the template"&word 1 of the tool
      send "selectedObjectChanged" to me
   else if (the selObj) is not empty and the id of the selObj is 0 then
      do "set the selected of" && (line 1 of the selObj) && "to false"
   end if
   if the tool is "pointer tool" or the tool is "browse tool" then
      send "selectedObjectChanged" to me
      lock messages
      repeat with i = 1 to the number of btns in group "graphics" of stack "revTools"
         set the hilite of btn i of group "graphics" of stack "revTools" to false
      end repeat
      repeat with i = 1 to the number of btns in group "paint" of stack "revTools"
         set the hilite of btn i of group "paint" of stack "revTools" to false
      end repeat
      set the loc of btn "Graphic Hilite" of stack "revTools" to -100,-100
      
      local tPendingMessages
      put the pendingMessages into tPendingMessages
      
      local tMessageIdentifier
      put "mouseUp",the long id of btn "pointer" of cd 1 of stack "revTools" into tMessageIdentifier
      
      local tMessageLine
      put lineOffset(tMessageIdentifier,tPendingMessages) into tMessageLine
      if tMessageLine is not 0 then cancel (item 1 of line tMessageLine of tPendingMessages)
      if the tool is "pointer tool" and the mode of stack "revTools" is not 0 then
         unhilite btn "Mac switcher" of stack "revTools"
         set the icon of btn "Mac switcher" of stack "revTools" to 203072
         set the hilitedButton of group "win switcher" of stack "revTools" to 2
         set the hilitedButton of group "linux switcher" of stack "revTools" to 2
      else if the mode of stack "revTools" is not 0 and the tool is "browse tool" then
         hilite btn "Mac switcher" of stack "revTools"
         set the icon of btn "mac switcher" of stack "revTools" to 203072
         set the hilitedButton of group "win switcher" of stack "revTools" to 1
         set the hilitedButton of group "linux switcher" of stack "revTools" to 1
      end if
      unlock messages
   else if the mode of stack "revTools" is not 0 then
      -- some other tool
      set the hilitedButton of group "win switcher" of stack "revTools" to 0
      set the hilitedButton of group "linux switcher" of stack "revTools" to 0
      set the icon of btn "mac switcher" of stack "revTools" to 203178
      unhilite btn "mac switcher" of stack "revTools"
      unlock messages
   end if
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass newTool
end newTool

on revNewObjectName pWhichType
   local tObjectType
   if pWhichType is not empty then put pWhichtype into tObjectType else put word 1 of the tool into tObjectType
   
   local tNum
   if tObjectType is not in "pointer,browse" then
      set the defaultStack to the topStack
      put 1 into tNum
      
      local tName
      repeat
         put tObjectType && quote & tObjectType && tNum & quote into tName
         if not (there is a tName) then exit repeat
         add 1 to tNum
      end repeat
      put toUpper(char 1 of tObjectType) into char 1 of tObjectType
      if tObjectType is "Browser" then exit revNewObjectName -- engine glitch
      do "set the name of the template" & tObjectType && "to" && quote & tObjectType && tNum & quote
   end if
end revNewObjectName

function revCheckGroupDelete
   if the selectedField is not empty then return true
   repeat for each line l in the selectedObjects
      if word 1 of l is "group" and the number of lines in the cardNames of l > 1 then
         answer warning "This group is placed on multiple cards," & cr & "really delete it?" with "Yes" or "No"
         if it is "No" then return false
      end if
      if word 1 of l is "group" and the dgProps["control type"] of l is "Data Grid" then
         answer warning "Would you like to delete the record template associated with this data grid? This action cannot be undone." with "Yes" or "No"
         if it is "Yes" then 
            local tTemplate
            put the dgProps["row template"] of l into tTemplate
            
            local tCardWord
            put wordoffset("card",tTemplate) into tCardWord
            
            local tTemplateCard
            put word tCardWord to -1 of tTemplate into tTemplateCard
            
            local tStackWord
            put wordoffset("stack",tTemplate) into tStackWord
            
            local tTemplateStack
            put word tStackWord to -1 of tTemplate into tTemplateStack
            if the number of cards in stack tTemplateStack is 2 then delete stack tTemplateStack
            else 
               delete tTemplateCard
            end if
         end if
      end if
   end repeat
   return true
end revCheckGroupDelete

function revLookUpPackage pPath
   local tPath
   put pPath into tPath
   if char -4 to -1 of tPath is ".app" then
      put "/Contents/MacOS" after tPath
      
      local tReturn
      put the directory into tReturn
      set the directory to tPath
      
      local tFiles
      put the files into tFiles
      set the directory to tReturn
      --answer tPath & "/" & line 1 of tFiles
      return tPath & "/" & line 1 of tFiles
   end if
   return tPath
end revLookUpPackage

on reloadStack pStackName, pFileName
   global gREVSuppressMessages
   if the filename of stack pStackName is pFileName then
      toplevel pStackName
   else
      set the defaultStack to pStackName
      
      local tFilePath
      put the effective fileName of stack pStackName into tFilePath
      if the number of words in tFilePath > 0 then put comma && pStackName & "?" after tFilePath else put pStackName & "?" into tFilepath
      answer warning "A stack with the same name as the one you are trying to load is already open." & cr & "Before loading" && pFileName & ", what do you want to do with stack:" & cr & tFilePath with "Save" or "Purge" or "Cancel"
      if it is "Cancel" then exit reloadStack
      if gREVSuppressMessages then
         lock messages
         send "revInsertMessageBlock" to me in 100 milliseconds
         revInternal__UnloadLibrary "revNoMessagesLibrary"
      end if
      
      local tLockMessages
      put the lockMessages into tLockMessages
      if it is "Purge" then
         revAOUpdateForDeleteStack pStackName
         lock messages
         delete stack pStackName
         set the lockMessages to tLockMessages
         go stack pFileName
      else
         if the filename of stack pStackName is empty then
            revSaveAs pStackName
         else
            revSave pStackName
         end if
         revAOUpdateForDeleteStack pStackName
         lock messages
         delete stack pStackName
         set the lockMessages to tLockMessages
         go to stack pFileName
      end if
      
      revAOUpdateForDeleteStack pStackName
      lock messages
      delete stack pStackName
      set the lockMessages to tLockMessages
      go to stack pFileName
   end if
   
   send "revNewStack the short name of stack pFileName" to stack "revApplicationOverview"
end reloadStack

on revAOUpdateForDeleteStack pStackName
   if the mode of stack "revApplicationOverview" is not 0 then
      local tStacks
      put pStackName into tStacks
      if the substacks of stack pStackName is not empty
      then put cr & the substacks of stack pStackName after tStacks
      send "revAODeleteStack pStackName, tStacks" to stack "revApplicationOverview"
   end if
end revAOUpdateForDeleteStack

on revInsertMessageBlock
   revInternal__LoadLibrary "revNoMessagesLibrary"
end revInsertMessageBlock

function revExtensionShow
  --returns true if windows is hiding file extensions for files registered, otherwise returns false
  if the platform is "MacOS" then
    return the cREVExtension of stack "revPreferences"
  else if the platform is "Win32" then
    return queryRegistry("HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced\HideFileExt") is not the cREVGeneral["cREVHideFileTypes"] of stack "Home"
  end if
  return true
end revExtensionShow

---------------------------------------- Rulers ----------------------------------------

on revCalculateRulers
   lock messages
   hide stack "revRulersH"
   hide stack "revRulersV"
   switch the cREVRulerMeasurement of stack "revPreferences"
      case "centimeters"
         set the icon of btn 1 of stack "revRulersH" to 200708
         set the icon of btn 1 of stack "revRulersV" to 200711
         break
      case "inches"
         set the icon of btn 1 of stack "revRulersH" to 200709
         set the icon of btn 1 of stack "revRulersV" to 200712
         break
      case "pixels"
         set the icon of btn 1 of stack "revRulersH" to 200705
         set the icon of btn 1 of stack "revRulersV" to 200713
         break
   end switch
   if the platform is "MacOS" then
      set the width of stack "revRulersH" to the width of the topStack+26
      set the left of btn 1 of stack "revRulersV" to -1
      set the height of stack "revRulersV" to the height of the topStack+3
      set the top of btn 1 of stack "revRulersV" to -31
      set the left of btn 1 of stack "revRulersH" to -2
      set the top of btn 1 of stack "revRulersH" to -1
      set the topLeft of stack "revRulersV" to item 1 of the topLeft of the topStack - 20,item 2 of the topLeft of the topStack + 5
   else
      set the top of btn 1 of stack "revRulersH" to 0
      set the width of stack "revRulersH" to the width of the topStack+24
      set the left of btn 1 of stack "revRulersV" to 0
      set the height of stack "revRulersV" to the height of the topStack+3
      set the top of btn 1 of stack "revRulersV" to -30
      set the left of btn 1 of stack "revRulersH" to -1
      set the topLeft of stack "revRulersV" to item 1 of the topLeft of the topStack - 20,item 2 of the topLeft of the topStack + 1
   end if
   set the topLeft of stack "revRulersH" to item 1 of the bottomLeft of the topStack - 20, item 2 of the bottomLeft of the topStack+3
   set the cREVHSubtract of stack "revRulersH" to item 1 of the topLeft of the topStack - item 1 of the topLeft of stack "revRulersH"
   set the cREVVSubtract of stack "revRulersH" to item 2 of the topLeft of the topStack - item 2 of the topLeft of stack "revRulersH"
   set the cREVHSubtract of stack "revRulersV" to item 1 of the topLeft of the topStack - item 1 of the topLeft of stack "revRulersV"
   set the cREVVSubtract of stack "revRulersV" to item 2 of the topLeft of the topStack - item 2 of the topLeft of stack "revRulersV"
   set the cREVCurrentStack of stack "revRulersH" to the short name of the topStack
   set the right of grc "end marker" of stack "revRulersH" to the width of the topStack + 27
   revInternal__LoadLibrary "revRulersScriptLibrary"
   show stack "revRulersH"
   show stack "revRulersV"
   unlock messages
end revCalculateRulers

on revPartCalculateRulers
   lock messages
   --this can be done with flicker by altering the width first, then the topLeft
   
   local tRect
   put the rect of stack "revRulersH" into tRect
   put item 1 of the bottomLeft of the topStack-20 into item 1 of tRect
   put item 2 of the bottomLeft of the topStack+3 into item 2 of tRect
   if the platform is "MacOS" then add 4 to item 2 of tRect
   put item 2 of tRect + 16 into item 4 of tRect
   if the platform is "MacOS" then subtract 2 from item 4 of tRect
   put item 1 of tRect + the width of the topStack + 24 into item 3 of tRect -- set width
   if the platform is "MacOS" then add 2 to item 3 of tRect
   set the rect of stack "revRulersH" to tRect
   --now the V ruler
   put the rect of stack "revRulersV" into tRect
   put item 1 of the topLeft of the topStack - 20 into item 1 of tRect
   put item 1 of tRect + 16 into item 3 of tRect
   if the platform is "MacOS" then subtract 2 from item 3 of tRect
   put item 2 of the topLeft of the topStack into item 2 of tRect
   if the platform is "MacOS" then add 1 to item 2 of tRect
   put item 2 of tRect + the height of the topStack + 3 into item 4 of tRect -- set height
   if the platform is "MacOS" then add 2 to item 4 of tRect
   set the rect of stack "revRulersV" to tRect
   
   local tRight
   if the platform is not "MacOS" then put 3 into tRight else put 4 into tRight
   set the right of grc "end marker" of stack "revRulersH" to the right of cd 1 of stack "revRulersH" +tRight
   set the cREVHSubtract of stack "revRulersH" to item 1 of the topLeft of the topStack - item 1 of the topLeft of stack "revRulersH"
   set the cREVVSubtract of stack "revRulersH" to item 2 of the topLeft of the topStack - item 2 of the topLeft of stack "revRulersH"
   set the cREVHSubtract of stack "revRulersV" to item 1 of the topLeft of the topStack - item 1 of the topLeft of stack "revRulersV"
   set the cREVVSubtract of stack "revRulersV" to item 2 of the topLeft of the topStack - item 2 of the topLeft of stack "revRulersV"
   set the cREVCurrentStack of stack "revRulersH" to the short name of the topStack
   unlock messages
end revPartCalculateRulers

---------------------------------------- end of Rulers ----------------------------------------
local lTimerID

on selectedObjectChanged
  if "updateForSelectedObjectChanged" is in the pendingMessages then cancel lTimerID
  send "updateForSelectedObjectChanged" to me in 200 milliseconds
  put the result into lTimerID
end selectedObjectChanged

# OK-2008-03-20 : Moved variable declaration as it was being used before declaration
local lHiddenPalettes

on updateForSelectedObjectChanged
   global gREVStopUpdates, gREVStacksList, gREVSelectionDescription
   global gREVMessageDispatch
   
   if gREVStopUpdates then exit updateForSelectedObjectChanged
   
   --check palettes are on screen, workaround for stacks in float about all move that are toplevel
   
   if lHiddenPalettes is not empty then
      revSEShowPalettes
   end if   
   lock screen
   
   --first, determine the standard notation to describe the selection
   if the number of lines in (the selObj) is 1 then
      put the name of the selObj into gREVSelectionDescription
   else
      local tObjectTypes
      repeat for each line l in (the selObj)
         add 1 to tObjectTypes[revStyle(l)]
      end repeat
      get keys(tObjectTypes)
      if the number of lines in it is 1 then --its a single object type
         put "multiple" && it & "s" into gREVSelectionDescription
      else
         put "multiple object types" into gREVSelectionDescription
      end if
   end if
   --profiles in the selection description
   
   if (the selObj) is empty then
      put empty into gREVSelectionDescription
   else if the id of the selObj is 0 then
      put "the template" & revStyle(the selObj) & ", Master Profile" into gREVSelectionDescription
   else
      
      local tProfile
      put revReturnCurrentProfile() into tProfile
      if tProfile is space then put ", multiple profiles" after gREVSelectionDescription
      else put comma && tProfile && "Profile" after gREVSelectionDescription
   end if
   
   --despatch property palette messages
   
   repeat for each line l in gREVStacksList
      if word 1 of l is among the items of "revPropertyPalette,revAppearance" then
         send "revUpdateTabs" to stack l
         send "revUpdatePalette" to this cd of stack l
      end if
   end repeat
   
   --non property palette IDE stacks
   repeat for each line l in gREVStacksList
      if "revPropertyPalette" is not in l and not the cREVShaded of stack l
      then send "revUpdatePalette" to this cd of stack l
   end repeat
   
   --plugins
   repeat for each line l in gREVMessageDispatch["revSelectedObjectChanged"]
      send "revSelectedObjectChanged" to this cd of stack l
   end repeat
   unlock screen
   --don't pass this message or you get duplicates
end updateForSelectedObjectChanged

on preOpenStack
   lock screen
   
   local tTargetStack
   put the short name of the owner of the target into tTargetStack
   if char 1 to 3 of tTargetStack is "rev" or tTargetStack is among the items of "Message Box,Home,answer dialog,ask dialog,script debugger,file selector" then
      --sanity check inside screen
      if the bottom of stack tTargetStack > item 4 of the windowBoundingRect
      then set the bottom of stack tTargetStack to item 4 of the windowBoundingRect
      if the top of stack tTargetStack < item 2 of the windowBoundingRect
      then set the top of stack tTargetStack to item 2 of the windowBoundingRect
      if the right of stack tTargetStack > item 3 of the windowBoundingRect
      then set the right of stack tTargetStack to item 3 of the windowBoundingRect
      if the left of stack tTargetStack < item 1 of the windowBoundingRect
      then set the left of stack tTargetStack to item 1 of the windowBoundingRect
   end if
   unlock screen
   
   if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass preOpenStack
end preOpenStack

function revStyle pWhichString
  constant kObjectTypes="button,field,graphic,group,scrollbar,image,player,eps"
  switch
  case word 1 of pWhichString is "card" and word 2 of pWhichString is among the items of kObjectTypes
    --hcAddressing special case
    return word 2 of pWhichString
    break
  case word 1 of pWhichString is "the"
    --templateObject
    return char 9 to -1 of word 2 of pWhichString
    break
  default
    return word 1 of pWhichString
    break
  end switch
end revStyle

on undoChanged
  global gREVUndoList
  put empty into gREVUndoList
end undoChanged

local lSelectionTimerID, lSelectionObject


on selectionChanged
  --  
  --  put (the target) into lSelectionObject
  --  if "revUpdateSelectionChanged" is not in the pendingMessages then
  --    send "revUpdateSelectionChanged" to me in 400 milliseconds
  --    put the result into lSelectionTimerID
  --  else
  --    cancel lSelectionTimerID
  --    send "revUpdateSelectionChanged" to me in 400 milliseconds
  --    
  --    put the result into lSelectionTimerID
  --  end if
  --if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass selectionChanged
end selectionChanged



on openField
  put (the target) into lSelectionObject
  if "revUpdateSelectionChanged" is not in the pendingMessages then
    send "revUpdateSelectionChanged" to me in 400 milliseconds
    put the result into lSelectionTimerID
  else
    cancel lSelectionTimerID
    send "revUpdateSelectionChanged" to me in 400 milliseconds
    put the result into lSelectionTimerID
  end if
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass openField
end openField


on revUpdateSelectionChanged
  global gREVChunkStore
  
  --temporary disable
  --  if word 1 lSelectionObject is not "field" or the selectedField is empty then
  --    
  --    put empty into gREVChunkStore
  --    
  --    exit revUpdateSelectionChanged
  --  end if
  --  
  --  put word 1 to 4 of (the selectedChunk) && the long id of the selectedField into gREVChunkStore
  --  
  
  
  --  --  if the mode of stack "revMenubar" is not 0 then
  --  --    lock screen
  --  --    send "revUpdatePalette" to cd 1 of stack "revMenuBar"
  --  --    unlock screen
  --  --  end if
end revUpdateSelectionChanged


on exitField
  selectionChanged
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass exitField
end exitField

on closeField
  selectionChanged
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass closeField
end closeField

on libraryStack
  if the mode of stack "Message Box" is not 0 \
      and the short name of this cd of stack "Message Box" is "stacksInUse"
  then send "revUpdateLibraryStacks" to this cd of stack "Message Box"
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass libraryStack
end libraryStack

on releaseStack
  if the mode of stack "Message Box" is not 0 \
      and the short name of this cd of stack "Message Box" is "stacksInUse"
  then send "revUpdateLibraryStacks" to this cd of stack "Message Box"
  if not gREVSuppressMessages or (gREVSuppressMessages and revOKTarget()) then pass releaseStack
end releaseStack

on mainStackChanged
  if the mode of stack "revApplicationOverview" is not 0
  then send "revMainStackChanged the long id of the target" to stack "revApplicationOverview" 
end mainStackChanged

function revReturnSpaces pString
   local tSpaces
   put empty into tSpaces
   repeat for each char c in pString
      if c is not space then exit repeat
      put space after tSpaces
   end repeat
   return tSpaces
end revReturnSpaces

on revDeleteLinkedObject pTarget
   lock screen
   if pTarget is empty then put the long id of the target into pTarget
   local tLinkedPalettesList
   put revLinkedSubControlPalettes(pTarget) into tLinkedPalettesList
   repeat for each line l in tLinkedPalettesList
      close stack l
   end repeat
   unlock screen
end revDeleteLinkedObject

on revKillRedundantPalettes
   lock screen
   local tPalettes
   put revPalettesList() into tPalettes
   repeat for each line l in tPalettes
      repeat for each line m in the cREVLinked of stack l
         if (there is not a m) then 
            close stack l
            exit repeat
         end if
      end repeat
   end repeat
   unlock screen
end revKillRedundantPalettes

on revKillAllLinkedPalettes
   lock screen
   
   local tPalettes
   put revPalettesList() into tPalettes
   repeat for each line l in tPalettes
      if the cREVLinked of stack l is not empty
      then close stack l
   end repeat
   unlock screen
end revKillAllLinkedPalettes

on revKill
  lock messages;quit;
end revKill

function revScriptEditors pStackPath
    return _revScriptEditorsNew(pStackPath)
end revScriptEditors

private function _revScriptEditorsOld pStackPath
   local tStacks
   put the mainStack of stack pStackPath into tStacks
   put cr & the substacks of stack tStacks after tStacks
   
   local tPalettes
   put the openStacks into tPalettes
   filter tPalettes with "revPropertyPalette*"
   
   local tScriptEditorCards
   put empty into tScriptEditorCards
   repeat for each line l in tPalettes
      if the short name of this cd of stack l is "revScript" then
         if revTargetStack(the cTargetObject of this cd of stack l) is among the lines of tStacks then
            put the long id of this cd of stack l & cr after tScriptEditorCards
         end if
      end if
   end repeat
   delete last char of tScriptEditorCards
   return tScriptEditorCards
end _revScriptEditorsOld
 
# Change this for now to prevent the ide interfering with the new script editor
constant kScriptEditorPrefix = "revNewScriptEditor "
 
private function _revScriptEditorsNew pStack
  local tStacks
  put the mainStack of pStack into tStacks
  put return & the substacks of pStack after tStacks
   
  local tScriptEditors
  put the openStacks into tScriptEditors
  filter tScriptEditors with kScriptEditorPrefix & "*"
   
  local tFoundEditors
  repeat for each line tEditor in tScriptEditors
    local tTargetObjects
    send "revSEGetTargetObjects" to tEditor
    put the result into tTargetObjects
     
    repeat for each line tTargetObject in tTargetObjects
      if revTargetStack(tTargetObject) is among the lines of tStacks then
        put tEditor & return after tFoundEditors
      end if
    end repeat
  end repeat
  delete last char of tFoundEditors
  return tFoundEditors
end _revScriptEditorsNew
   
# Parameters
#   pObject : reference to the object to edit the script of
# Description
#   Creates a new script editor for the specified object and opens it.
command revEditScriptInNewWindow pObject, pSpecial
   # OK-2009-03-16 : Bug 6620 - The script editor can be broken if a user has gREVDevelopment true
   # and aborts a repeat loop that is opening script editors...
   local tOldAllowInterrupts
   put the allowInterrupts into tOldAllowInterrupts
   set the allowInterrupts to false
   
   lock cursor
   set the cursor to watch
   
   local tEditorCount
   put the number of lines of revListScriptEditors() into tEditorCount
   
   local tNewEditorName
   put kScriptEditorPrefix & tEditorCount + 1 into tNewEditorName
   
   # OK-2008-12-03 : Bug 6926 and 7032 - Always ensure that any old script editors have been fully purged,
   # as some users may lock messages and close the script editor from the message box, preventing it from running
   # code needed to finalize itself.
   if there is a stack tNewEditorName then
      delete stack tNewEditorName
   end if
   
   local tTemplateVisibility, tTemplateRect
   put the visible of revScriptEditorTemplate() into tTemplateVisibility
   set the visible of revScriptEditorTemplate() to false
   
   # OK-2009-02-05 : This appears to be redundant, but have left it in for now as this command is rather fragile
   put the rect of revScriptEditorTemplate() into tTemplateRect
   
   # OK-2009-02-05 : This appears to be redundant
   --   local tRect, tPrefname
   --   put "rect" into tPrefname
   --   send "revSEGetPreference tPrefname" to revScriptEditorMain()
   --   put the result into tRect
   --   put the tPropname of stack "revPreferences" into tRect
   
   local tEditor
   lock messages
   
   # MW-2012-02-21: If 'special' is set then we make sure the editor's stack script is set
   #    rather than using a behavior - this allows that editor to edit the template stack script.
   
   
   ## EJB 2013-12-05: Bug 11546
   ## Reset the template group before cloning the stack so any properties set elsewhere are not inherited by the editor group
   reset the templateGroup
   if pSpecial then
      clone invisible revScriptEditorTemplate() as "revTempScriptEditor"
      set the behavior of stack "revTempScriptEditor" to empty
      set the script of stack "revTempScriptEditor" to the script of button "Template Stack" of stack "revNewScriptEditor"
      clone invisible stack "revTempScriptEditor" as tNewEditorName
      delete stack "revTempScriptEditor"
   else
      clone invisible revScriptEditorTemplate() as tNewEditorName
   end if
   
   put the long id of it into tEditor
   if tTemplateVisibility then
      set the visible of revScriptEditorTemplate() to true
   end if
   
   # OK-2009-02-05 : This appears to be redundant, but have left it in for now as this command is rather fragile
   set the rect of revScriptEditorTemplate() to tTemplateRect
   unlock messages
   
   set the mainstack of tEditor to the short name of revScriptEditorMain()
   set the destroyStack of tEditor to true
   set the destroyWindow of tEditor to true
   set the label of tEditor to the short name of tEditor
   
   set the cMutable of tEditor to true
   set the _ideoverride of tEditor to true
   
   # OK-2008-07-24 : Note the use of call instead of send here. This is important because
   # it prevents the defaultStack changing when debugging, which can potentially break
   # user code in the debugger which would have run fine otherwise.
   local tTrueString
   put "true" into tTrueString
   
   call "revSEInitialize tTrueString" to tEditor
   call "revSEAddTargetObject pObject" to tEditor
   send "revGoScriptEditor tEditor" to me in 0 milliseconds
   
   # OK-2008-06-29 : Bug 6661 - This is sent last because setting the script editor's current object
   # also focuses and selects the correct point in the script field, this needs to be done after the script
   # editor window has been opened otherwise doing the select command will throw an error and result
   # in char 1 to 0 being selected instead of what the user wants.
   send "revSESetCurrentObject pObject" to tEditor in 0 milliseconds
   unlock cursor
   
   set the allowInterrupts to tOldAllowInterrupts
end revEditScriptInNewWindow

   
# Returns
#   A list of the currently open script editors, ordered with topmost first.
function revListScriptEditors
   local tStacks
   put the openStacks into tStacks
   filter tStacks with kScriptEditorPrefix & "*"
   
   local tFullStacks
   repeat for each line tStack in tStacks
      put the long id of stack tStack & return after tFullStacks
   end repeat
   delete the last char of tFullStacks
   
   return tFullStacks
end revListScriptEditors
   
   # Returns
   #   The long id of the topmost script editor, or empty if there is no script editor open.
function revTopMostScriptEditor
  local tEditors
  put revListScriptEditors() into tEditors
  if the number of lines of tEditors = 0 then
    return empty
  else
    return line 1 of tEditors
  end if
end revTopMostScriptEditor
   
function revScriptEditorTemplate
  # For now instead of adding the stack properly, we just ensure it is in memory when asked for
  get revScriptEditorMain()
   
  # The script editor template is a substack of the main script editor stack.
  return the long id of stack "revSETemplate"
end revScriptEditorTemplate
   
function revScriptEditorMain
   --return the long id of stack (revEnvironmentToolsetPath() & slash & "revscripteditor.rev")
   # OK-2008-11-04 : Cannot refer to the script editor by path because it has no path in Media.
   return the long id of stack "revNewScriptEditor"
end revScriptEditorMain
   
command revGoScriptEditor pEditor
   # When the user clicks on any stack, revFrontScript intercepts the resulting mouseDown
   # and among other things, attempts to detect whether or not the stack has been edited
   # as a result of the click. Unfortunately any stack clicked on in run mode is flagged as edited,
   # this causes a problem with script editors asking the user to "save" etc when they are closed.
   # For now we work around this here by manually ensuring the editor is not flagged as edited.
   put empty into gREVStackStatus[the short name of pEditor]
   
   set the visible of pEditor to true
   toplevel pEditor
   
   # OK-2008-08-18 : Bug 6935 - The script editor should be un-iconified here.
   if the iconic of pEditor then
      set the iconic of pEditor to false
   end if
   
   # OK-2008-06-24 : Bug 6603 - Palettes not hidden when script editor first opens
   revSEHidePalettes
end revGoScriptEditor

function revIsMenuBarGroup pGroupID
   local tOwnerStack
   put char 2 to -2 of word wordOffset("stack",pGroupID)+1 of pGroupID into tOwnerStack
   if the short name of pGroupID is the menubar of the stack tOwnerStack \
   or the long name of pGroupID is the defaultMenuBar
   then return true
   else return false
end revIsMenuBarGroup

function revMenuManagerGroup pGroups
   if the mode of stack "revMenuManager" is 0 then return false
   
   local tMenuManagerBG
   if there is not a bg id (the cREVMenuID of stack "revMenuManager") \
   of stack (the cREVDefaultStack of stack "revMenuManager") then return false
   put the long id of bg id (the cREVMenuID of stack "revMenuManager") \
   of stack (the cREVDefaultStack of stack "revMenuManager") into tMenuManagerBG
   repeat for each line l in pGroups
      if l is tMenuManagerBG then return true
   end repeat
   return false
end revMenuManagerGroup

on revCheckScriptSaved pStack
    _revCheckScriptSavedNew pStack
end revCheckScriptSaved
   
private command _revCheckScriptSavedOld pWhichStack
  local tEnabled
  repeat for each line l in revScriptEditors(pWhichStack)
    do "put the enabled of btn" && quote & "apply" & quote && "of" && l && "into tEnabled"
    if tEnabled then
      send "revCheckScriptSaved" to l
    end if
  end repeat
end _revCheckScriptSavedOld
   
private command _revCheckScriptSavedNew pStack
  # Not required
end _revCheckScriptSavedNew


on errorDialog pWhatError, pWhatError2
   ######################
   close printing
  global gREVSuppressErrors, gREVDevelopment
  if gREVDevelopment then write the params & cr & the long time & cr & "--------" & cr & cr to stdOut
  if gREVSuppressErrors then exit errorDialog
  filter pWhatError without "*,0,0"
  if (item 1 of pWhatError is not a number or item 2 of pWhatError is not a number) and the number of words in pWhatError is not 0 then
    answer "The following user thrown error was generated and not handled, so it was intercepted by Revolution:" & cr & cr & the params
    exit errorDialog
  end if
  if the number of lines in pWhatError is 0 then exit errorDialog -- bogus error
  set the executionError of stack "revErrorDisplay" to pWhatError
  set the scriptError of stack "revErrorDisplay" to pWhatError2
  repeat for each line l in pWhatError
    if item 1 of l is 353 then
      set the errorObject of stack "revErrorDisplay" to item -1 of l
      exit repeat
    end if
  end repeat
  if the mode of stack "revErrorDisplay" is not 0 then
    send "preOpenCard" to cd 1 of stack "revErrorDisplay"
    send "opencard" to cd 1 of stack "revErrorDisplay"
  else
    modeless "revErrorDisplay"
  end if
end errorDialog


----------------------------------function to list all the groups (including nested ones) in a stack

function revBackgroundNames pStack
   local tBackgroundIDS
   put revBackgroundIDS(pStack) into tBackgroundIDS
   
   local tBackgroundNames
   put empty into tBackgroundNames
   repeat for each line l in tBackgroundIDS
      if quote is in the name of bg id l of stack pStack
      then put the short name of bg id l of stack pStack &cr after tBackgroundNames
      else put the name of bg id l of stack pStack & cr after tBackgroundNames
   end repeat
   return tBackgroundNames
end revBackgroundNames

function revBackgroundIDs pStack
   local tBackgrounds
   put empty into tBackgrounds
   
   local tNumBgs
   put the num of bgs in stack pStack into tNumBgs
   repeat with i = 1 to tNumBgs
      if the id of bg i of stack pStack is not among the lines of tBackgrounds
      then put the id of bg i of stack pStack & cr after tBackgrounds
      repeat with j = 1 to the num of grps in bg i of stack pStack
         if the id of grp j of bg i of stack pStack is not among the lines of tBackgrounds
         then put the id of grp j of bg i of stack pStack & cr after tBackgrounds
      end repeat
   end repeat
   delete last char of tBackgrounds
   return tBackgrounds
end revBackgroundIDs

----------------------------------

--------------------------------------------------------------------------
Property Palette Functions
--------------------------------------------------------------------------

function revLinkedPalettesList pLinkedObjects
   local tOpenStacks
   put the openStacks into tOpenStacks
   filter tOpenStacks with "revPropertyPalette*"
   
   local tLinkedPalettes
   put empty into tLinkedPalettes
   repeat for each line l in pLinkedObjects
      repeat for each line m in tOpenStacks
         if l is among the lines of the cREVLinked of stack m and m is not among the lines of tLinkedPalettes 
         then put m & cr after tLinkedPalettes
      end repeat
   end repeat
   delete last char of tLinkedPalettes
   return tLinkedPalettes
end revLinkedPalettesList

function revLinkedSubControlPalettes pObjects
   local tOpenStacks
   put the openStacks into tOpenStacks
   filter tOpenStacks with "revPropertyPalette*"
   
   local tLinkedPalettes
   put empty into tLinkedPalettes
   repeat for each line l in pObjects
      repeat for each line m in tOpenStacks
         if l is in the cREVLinked of stack m and m is not among the lines of tLinkedPalettes 
         then put m & cr after tLinkedPalettes
      end repeat
   end repeat
   delete last char of tLinkedPalettes
   return tLinkedPalettes
end revLinkedSubControlPalettes

function revLinkedAppearancePalettesList pLinkedObjects
   local tOpenStacks
   put the openStacks into tOpenStacks
   filter tOpenStacks with "revAppearance*"
   
   local tLinkedPalettes
   put empty into tLinkedPalettes
   repeat for each line l in pLinkedObjects
      repeat for each line m in tOpenStacks
         if l is among the lines of the cREVLinked of stack m and m is not among the lines of tLinkedPalettes 
         then put m & cr after tLinkedPalettes
      end repeat
   end repeat
   delete last char of tLinkedPalettes
   return tLinkedPalettes
end revLinkedAppearancePalettesList

on revBuildLinkedPalette pObject
  send "revBuildPropertyPalette pObject" to stack "revTemplatePalette"
end revBuildLinkedPalette

function revPalettesList
   local tPalettes
   put the openStacks into tPalettes
   filter tPalettes with "revPropertyPalette*"
   return tPalettes
end revPalettesList

function revPropertyPalettes pObjects
   --returns a list of all palettes affected by given objects
   local tOpenStacks
   put the openStacks into tOpenStacks
   filter tOpenStacks with "revPropertyPalette*"
   
   local tObjects
   put the selobj into tObjects
   if tObjects is empty then put the long id of the topStack into tObjects
   
   local tPalettes
   put empty into tPalettes
   repeat for each line l in pObjects
      repeat for each line m in tOpenStacks
         --check if stack is linked to object or is the active palette and the object is in the selection
         if (l is among the lines of the cREVLinked of stack m \
         or (the cREVLinked of stack m is empty and l is among the lines of tObjects)) \
         and m is not among the lines of tPalettes
         then put m & cr after tPalettes
      end repeat
   end repeat
   delete last char of tPalettes
   return tPalettes
end revPropertyPalettes

function revPaletteTitle pObjects
   if pObjects is empty then return "Inspector"
   if the num of lines in pObjects is 1 then
      if word 1 of pObjects is "the" then return pObjects
      return the name of pObjects, " ID" && the id of pObjects
   else
      local tAllSameInfo
      put revAllSameType2(pObjects) into tAllSameInfo
      if (line 1 of tAllSameInfo)
      then return "Multiple" && (line 2 of tAllSameInfo) & "s"
      else return "Multiple objects"
   end if
end revPaletteTitle

function revAllSameType2 pObjects
   --first line true or false
   --second line the object type
   
   local tAllSame
   put true into tAllSame
   
   local tFirstObject
   put line 1 of pObjects into tFirstObject
   
   local tFirstObjectType
   put revDiscernObjectType(tFirstObject) into tFirstObjectType
   
   repeat for each line l in pObjects
      if revDiscernObjectType(l) is not tFirstObjectType then
         put false into tAllSame
         exit repeat
      end if
   end repeat
   
   return tAllSame & cr & tFirstObjectType
end revAllSameType2

function revDiscernObjectType pObject
   if (the cREVGeneral["viewerobject"] of pObject) \
          or (the cREVGeneral["viewercreator"] of pObject) then return "report"
   if word 1 of pObject is not "group" and the cRevGeneral["databasequeryobject"] of pObject 
   then return "databasequery"
   
   local tObject
   put word 1 of pObject into tObject
   switch tObject
      case "button"
         local tTheObject
         if the style of pObject is "menu" then
            if the menuMode of pObject is "tabbed" then 
               put "tab" into tTheObject
            else
               put "menu" into tTheObject
            end if
         else
            put "button" into tTheObject
         end if
         break
      case "graphic"
         switch the style of pObject
            case "rectangle"
            case "text" -- OK-2010-03-08: Bug 8651 - Returns "text" with showName & lineSize of 0
               put "rectangle" into tTheObject
               break
            case "roundrect"
               put "roundedrectangle" into tTheObject
               break
            case "curve"
               put "freehandcurve" into tTheObject
               break
            case "regular"
               put "regularpolygon" into tTheObject
               break
            default
               put the style of pObject into tTheObject
               break
         end switch
         break
      case "scrollbar"
         switch the style of pObject
            case "scale"
               put "slider" into tTheObject
               break
            case "progress"
               put "progressbar" into tTheObject
               break
            default
               if the height of pObject < 29 and the width of pObject < 17 
               then put "littlearrows" into tTheObject
               else put "scrollbar" into tTheObject
               break
         end switch
         break
      case "the"
         --templateObject selected
         put char 9 to -1 of word 2 of pObject into tTheObject
         replace "bkgnd" with "group" in tTheObject
         break
      case "bkgnd"
         --change wording from "bkgnd" to "group" for compatibility with tab name
         put "group" into tTheObject
         break
      case "group"
         if the dgProps["control type"] of pObject is "Data Grid" then
            put "DataGrid" into tTheObject
         else
            put "group" into tTheObject
         end if
         break
      default
         put word 1 of pObject into tTheObject
         break
   end switch
   return tTheObject
end revDiscernObjectType


----------------------------------------------

on revReloadBreakpoints
   exit revReloadBreakpoints
   
   local tStacksList
   put the mainStacks into tStacksList
   repeat for each line l in tStacksList
      if not (there is a stack l) then next repeat -- stack saved on another machine with file path wrong
      local tList
      put the cREVGeneral["debugObjects"] of stack l into tList
      if the number of lines in tList > 250 then
         answer error "It appears your breakPoint information has been corrupted.  Fixing this will remove all breakPoints from this stack.  If you don't fix this, Revolution may not work correctly.  Please file a bug report as this problem should not occur." with "Don't fix" or "Fix"
         if it is "fix" then
            set the cREVGeneral["debugObjects"] of stack l to empty
            put empty into tList
         end if
      end if
      repeat for each line x in tList
         if not exists(x) then next repeat
         set the cREVGeneral["breakPoints"] of x to the cREVGeneral["breakPoints"] of x
      end repeat
   end repeat
end revReloadBreakpoints

on revClearAllBreakPoints
   # OK-2008-02-21 : Removed to prevent interference with new debugger
   exit revClearAllBreakpoints
   
   local tStacksList
   put the mainStacks into tStacksList
   put revFilterStacksList(tStacksList) into tStacksList
   
   local tList
   repeat for each line l in tStacksList
      put the cREVGeneral["debugObjects"] of stack l into tList
      repeat for each line x in tList
         set the cREVGeneral["breakPoints"] of x to empty
      end repeat
   end repeat
end revClearAllBreakPoints

setProp cREVScriptDebugMode pWhat
   _setScriptDebugModeNew pWhat
   pass cREVScriptDebugMode
end cREVScriptDebugMode

private command _setScriptDebugModeNew pMode
  lock messages
  set the cREVScriptDebugMode of stack "revPreferences" to pMode
  unlock messages
  
  if pMode is not "true" then
    revDebuggerRun
  end if
end _setScriptDebugModeNew


private command _setScriptDebugModeOld pWhat
   global gREVSavedBreakPoints
   if pWhat is true then
      if gREVSavedBreakPoints is not empty then 
         set the breakPoints to gREVSavedBreakPoints
      else
         set the breakPoints to the long id of group 1 of stack "revLibrary",500
      end if
      revReloadBreakpoints
   else
      if the breakPoints is not empty then 
         put the breakPoints into gREVSavedBreakPoints
      else
         put empty into gREVSavedBreakPoints
      end if
      set the breakPoints to empty
      
      local tReturn
      put true into tReturn
      if the mode of stack "revErrorDisplay" is not 0 then
         disable btn "ignore" of stack "revErrorDisplay"
         disable btn "debug..." of stack "revErrorDisplay"
      end if
   end if
   
   local tStacksList
   put the openStacks into tStacksList
   filter tStacksList with "revScriptEditor*"
   repeat for each line l in tStacksList
      if the cBreakPoints of cd 1 of stack l is not pWhat then set the cBreakPoints of cd 1 of stack l to pWhat
   end repeat
   if tReturn then
      -- MW-2008-02-28: [[ Bug 3816 ]] A previously missing unlock messages here caused any stacks that are opened
      --   by dragging onto the icon or double-clicking to *not* get their preOpen/open messages
      lock messages
      set the cREVScriptDebugMode of stack "revPreferences" to pWhat
      unlock messages
      send "traceDone" to btn "revFrontScript" of stack "revLibrary"
   end if
end _setScriptDebugModeOld




function revAllSameType pObjects
   --first line true or false
   --second line the object type
   local tAllSame
   put true into tAllSame
   
   local tFirstObjectType
   put revDiscernObjectType(line 1 of pObjects) into tFirstObjectType
   repeat for each line l in pObjects
      if revDiscernObjectType(l) is not tFirstObjectType then
         put false into tAllSame
         exit repeat
      end if
   end repeat
   return tAllSame & cr & tFirstObjectType
end revAllSameType

function revObjectTabs pObjects
   --returns object tabs for given object
   --some objects have multiple object specific tabs
   --will correspond to text of btn "Tabs"
   local tAllSameInfo
   put revAllSameType(pObjects) into tAllSameInfo
   
   if (line 1 of tAllSameInfo) then
      local tTabs
      switch (line 2 of tAllSameInfo)
         case "button"
            put "Button"&cr&"Menu"&cr&"Report" into tTabs
         break
         case "field"
            put "Field"&cr&"Behavior"&cr&"Database"&cr&"Table" into tTabs
         break
         case "player"
            put "Player"&cr&"Interaction" into tTabs
         break
         case "stack"
            put "Stack"&cr&"General" into tTabs
         break
         default
            put line 2 of tAllSameInfo into tTabs
            put toUpper(char 1 of tTabs) into char 1 of tTabs
         break
      end switch
      return "Basic"&cr&tTabs&cr&"Custom"
   else
      --multiple different types of object
      return "Basic"&cr&"Custom"
   end if
end revObjectTabs

function revReturnSelectionDescription pObjects
   --first line of result is the label of the selection description btn
   --second line of result is the cREVLabelID of btn "Selection Description"
   
   if line 1 of revAllSameType(pObjects) then
      switch
         case word 1 of pObjects is "the" --template object selected
            local tSelectionDescription
            put line 1 of pObjects into line 1 of tSelectionDescription
            put line 1 of tSelectionDescription into line 2 of tSelectionDescription
            set the cREVLabelID of btn "Selection Description" of bg "revBackdrop" to tSelectionDescription
         break
         case the num of lines in pObjects > 1 
            # OK-2008-03-20 : This appears to have been ommited
            local tFirstObjectType
            put revDiscernObjectType(line 1 of pObjects) into tFirstObjectType
            
            put "multiple" && tFirstObjectType & "s" into line 1 of tSelectionDescription
            put line 1 of tSelectionDescription into line 2 of tSelectionDescription
         break
         default
            put the name of pObjects into line 1 of tSelectionDescription
            put pObjects into line 2 of tSelectionDescription
         break
      end switch
   else
      --multiple object types
      put "multiple object types" into line 1 of tSelectionDescription
      put line 1 of tSelectionDescription into line 2 of tSelectionDescription
   end if
   return tSelectionDescription
end revReturnSelectionDescription

------------------------------------------
# Script Editor functions
------------------------------------------

# Parameters
#   pObject : either a long id or a valid reference to an object.
# Returns
#   The "rugged" id of the object. This can be used the same as a long id,
#   but is slightly more resistant to stuff like objects being grouped, renamed etc.
#   The script editor uses rugged ids to store object ids when there is a chance that the object
#   could be renamed or grouped etc. 
function revRuggedId pObject
   # First get the long id of our object. If pObject is a valid object reference we just get the 
   # object's long id. If its not a valid reference, this does not always mean the object doesn't
   # exist (because of edit group mode), in this case we assume that pObject is the long id of the object
   # we want, and parse it directly to obtain the rugged id, without needing to refer to the actual
   # object at all.
   local tObject
   if there is a pObject then
      put the long id of pObject into tObject
   else
      put pObject into tObject
   end if
   
   # Find out what the name of the stack owning the object is. We are only interested in the 
   # stack that immediately owns the object, which could be a substack. To find this we can
   # simply search from the front and find the first ocurrence of "of stack".
   local tSubStack, tMainstack
   local tStackFound
   local tWordNumber
   put 1 into tWordNumber
   repeat for each word tWord in tObject
      # The first match is always the substack, the second (and last) match is always the mainstack
      if tWord is "stack" then
         if tStackFound then
            put tMainStack into tSubStack
            put word (tWordNumber + 1) of tObject into tMainStack
            exit repeat
         else
            put word (tWordNumber + 1) of tObject into tMainStack
         end if
         put true into tStackFound
      end if
      add 1 to tWordNumber
   end repeat
   
   try
      local tStack
      if tSubStack is not empty then
         put "stack " & tSubStack & " of " & (the name of stack (char 2 to -2 of tMainStack)) into tStack
      else
         put (the name of stack (char 2 to -2 of tMainStack)) into tStack
      end if
   catch tError
      --      log "Error getting rugged id of object"
      --      log "pObject = " & pObject
      --      log "Context = " & the executionContexts
   end try
   
   # MW-2008-09-08: [[ Bug 7142 ]] If the hcAddressing of the stack is true, then the long id
   #    is in a different form than this expects. Thus we temporarily fetch it while hcAddressing
   #    is false.
   if the hcAddressing of tStack then
      set the hcAddressing of tStack to false
      put the long id of pObject into tObject
      set the hcAddressing of tStack to true
   end if
   
   local tType
   put word 1 of tObject into tType
   
   local tId
   if tType is "stack" then
      # With stacks, the name is the most robust way to refer to them. We assume that the stack exists...
      put tStack into tId
   else if tType is "audioclip" then
      put "audioclip id" && word 3 of tObject & " of " & tStack into tId
   else if tType is "videoclip" then
      put "videoclip id" && word 3 of tObject & " of " & tStack into tId
   else
      # With any other object, the safest thing is to return the object id and the name of its owning stack.
      # For cards we use "card id..." for other objects "control id..". This is significant because when in
      # edit group mode, the group being edited can be referred to as "control id.." but *not* "group id..."
      
      local tPrefix
      if tType is "card" then
         put "card" into tPrefix
      else
         put "control" into tPrefix
      end if
      put tPrefix & " id " & word 3 of tObject & " of " & tStack into tId
   end if
   
   return tId
end revRuggedId

function revScriptEditor pObject
    return _revScriptEditorNew(the long id of pObject)
end revScriptEditor

# Parameters
#   pObject : reference to the object to edit the script of
# Returns
#   The long id of the script editor that is currently editing pObject
#   or empty if none is.
private function _revScriptEditorNew pObject
  local tEditors
  put revListScriptEditors() into tEditors
  
  local tFoundEditor
  repeat for each line tEditor in tEditors
    local tTargetObjects
    send "revSEGetTargetObjects" to tEditor
    put the result into tTargetObjects
    
    if pObject is empty or there is no pObject then
      #log "Invalid object supplied from context:  " & return & the executionContexts
      log "Invalid object supplied: " & pObject
    end if
    
    if revRuggedId(pObject) is among the lines of tTargetObjects then
      put the short name of tEditor into tFoundEditor
      exit repeat
    end if
  end repeat
  
  return tFoundEditor
end _revScriptEditorNew

private function _revScriptEditorOld pObject
  #LG 2008-01-08 - Bug 5695
  # Added code which updates the ids of the targets of all the script editors.
  local tScriptEditor, tScriptEditors
  put the openStacks into tScriptEditors
  filter tScriptEditors with "revScriptEditor*"
  repeat for each line tScriptEditor in tScriptEditors
    lock messages
    #LG 2008-03-27
    #Bug 6087
    if there is a (the cTargetObject of card 1 of stack tScriptEditor) then
      set the cTargetObject of card 1 of stack tScriptEditor to the long id of the cTargetObject of card 1 of stack tScriptEditor
    end if
    unlock messages
  end repeat
  if word 1 of pObject is "group" and the backGroundBehavior of pObject then 
    local tCardName
    put line 1 of the cardNames of pObject into tCardName
    if word 1 to 2 of tCardName is "card id" then 
      put word 3 of tCardName into word 7 of pObject
    else
      # OK-2007-09-17 : Bug 5403 (pObject was previously mistyped as tObject)
      put the ID of card tCardName of stack revTargetStack(pObject) into word 7 of pObject
    end if
  end if
  
  repeat for each line tStack in tScriptEditors
    --check if stack is linked to object or is the active palette and the object is in the selection
    if pObject is among the lines of the cTargetObject of this card of stack tStack then
      return tStack
    end if
  end repeat
  return empty
end _revScriptEditorOld

on revShowScriptEditor pStack
    _revShowScriptEditorNew pStack
end revShowScriptEditor

private command _revShowScriptEditorOld pStack
    lock messages -- unlocked messages cause editor to appear behind menu bar
  --if menu bar has text and icons hidden, and editor loaded twice
  show stack pStack
  revSEHidePalettes
end _revShowScriptEditorOld

private command _revShowScriptEditorNew pStack
   modeless pStack
   revSEHidePalettes
end _revShowScriptEditorNew

on revSetScriptEditorRect pScriptEditor
   # OK-2008-03-11 : Not sure if this is needed anymore, removed for now.
   exit revSetScriptEditorRect
   
   --   local tOpenStacks
   --   put the openStacks into tOpenStacks
   --   filter tOpenStacks with "revScriptEditor*"
   --   delete line lineOffset(pScriptEditor,tOpenStacks) of tOpenStacks
   
   --   put empty into tScriptEditorTopLefts
   --   repeat for each line l in tOpenStacks
   --      put the topLeft of stack l & cr after tScriptEditorTopLefts
   --   end repeat
   --   delete last char of tScriptEditorTopLefts
   
   --   --tiling
   --   put the cScriptEditorRect of stack "revPreferences" into tRect
   --   put item 1 to 2 of (the cScriptEditorRect of stack "revPreferences") into tTopLeft
   --   repeat
   --      if tTopLeft is not among the lines of tScriptEditorTopLefts then exit repeat
   --      else 
   --         repeat with i = 1 to 4
   --            add 23 to item i of tRect
   --         end repeat
   --         put item 1 to 2 of tRect into tTopLeft
   --      end if
   --   end repeat
   
   --   --sanity check
   --   set the rect of stack pScriptEditor to tRect
   --   put the windowBoundingRect into tWindowBoundingRect
   --   put item 3 of tWindowBoundingRect - item 1 of tWindowBoundingRect into tMaxX
   --   put item 4 of tWindowBoundingRect - item 2 of tWindowBoundingRect into tMaxY
   
   --   lock messages
   --   if the height of stack pScriptEditor > tMaxY
   --   then set the height of stack pScriptEditor to tMaxY
   --   if the width of stack pScriptEditor > tMaxX
   --   then set the width of stack pScriptEditor to tMaxX
   
   --   add item 1 of tWindowBoundingRect to tMaxX
   --   add item 2 of tWindowBoundingRect to tMaxY
   
   --   put the right of stack pScriptEditor into tRight
   --   put the bottom of stack pScriptEditor into tBottom
   
   --   if the left of stack pScriptEditor < item 1 of tWindowBoundingRect then set the left of stack pScriptEditor to item 1 of tWindowBoundingRect
   --   if the right of stack pScriptEditor  > tMaxX then set the right of stack pScriptEditor to tMaxX
   --   if the top of stack pScriptEditor < item 2 of tWindowBoundingRect then set the top of stack pScriptEditor to item 2 of tWindowBoundingRect
   --   if the bottom of stack pScriptEditor > tMaxY then set the bottom of stack pScriptEditor to tMaxY
   
   --   if the right of stack pScriptEditor is not tRight or the bottom of stack pScriptEditor is not tBottom then
   --      send "resizeStack" to this cd of stack pScriptEditor
   --   end if
   
   --   unlock messages
end revSetScriptEditorRect

on revUpdatePaletteStack
   global gREVStacksList
   
   local tSelObj
   put the selobj into tSelobj
   if word 1 of tSelobj is "stack" then
      if the mode of tSelobj is 1 then
         select empty
      end if
   end if
   if (the selobj) is empty and the short name of the topStack is not lLastTopStack then
      lock screen
      repeat for each line l in gREVStacksList
         if word 1 of l is "revPropertyPalette" then
            send "revUpdateTabs" to stack l
            send "revUpdatePalette" to this cd of stack l
         end if
      end repeat
      unlock screen
   end if
   put the short name of the topStack into lLastTopStack
end revUpdatePaletteStack

function revBrowserTopStack
   local tOpenStacks
   put the openStacks into tOpenStacks
   
   local tNotOpenOffset
   put the num of lines in tOpenStacks + 1 into tNotOpenOffset
   
   local tLoadedStacks
   put revLoadedStacks("preference") into tLoadedStacks
   set the wholeMatches to true
   
   local tStacksList
   put empty into tStacksList
   
   local tMode
   repeat for each line l in tLoadedStacks
      put the mode of stack l into tMode
      if tMode is 0 then put 13 into tMode
      
      local tLineOffset
      put lineOffset(l,tOpenStacks) into tLineOffset
      if tLineOffset is 0 then put tNotOpenOffset into tLineOffset
      put tLineOffset,tMode,l & cr after tStacksList
   end repeat
   sort lines of tStacksList numeric ascending by item 1 of each
   sort lines of tStacksList numeric ascending by item 2 of each
   return item 3 of line 1 of tStacksList
end revBrowserTopStack

------------------------------------
Script Editor hide/show palettes
------------------------------------
local lShowPalettesMsgID

on revInitialiseSEHidePalettes
  put empty into lShowPalettesMsgID
  put empty into lHiddenPalettes
end revInitialiseSEHidePalettes

on revSEHidePalettes
   if lShowPalettesMsgID is not empty then cancel lShowPalettesMsgID
   put empty into lShowPalettesMsgID
   
   if lHiddenPalettes is not empty and (the mode of stack "message box" is not 0 and the visible of stack "Message Box" is false) then exit revSEHidePalettes
   
   lock screen
   lock messages
   
   put empty into lHiddenPalettes
   
   if the mode of stack "revErrorDisplay" is not 0 and the cSEHideErrors of stack "revPreferences" then
      hide stack "revErrorDisplay"
      put "revErrorDisplay" & cr after lHiddenPalettes
   end if
   
   if the cSEHidePalettes of stack "revPreferences" then
      local tOpenStacks
      put the openStacks into tOpenStacks
      filter tOpenStacks with "rev*"
      filter tOpenStacks without "revDragControl"
      repeat for each line l in tOpenStacks
         if (l is not among the items of "revMenuBar,revErrorDisplay,revVariableWatcher,revMessageWatcher,revDocsLanguageQuickref") and the mode of stack l is 4 then
            # OK-2008-06-24 : Bug 6621 : Prevent palettes being hidden if they are owned by the script editor
            if the mainstack of stack l is the short name of revScriptEditorMain() then
               next repeat
            end if
            
            # OK-2009-12-22: Bug 8497 : Also keep separate variable viewer panes open
            if the short name of stack l begins with "revVariableVisualizer" then
               next repeat
            end if
            
            hide stack l
            put l & cr after lHiddenPalettes
         end if
      end repeat
   end if
   -- must be after the others
   if the mode of stack "Message Box" is not 0 and the cSEHideMessageBox of stack "revPreferences" then
      hide stack "Message Box"
      put "Message Box" & cr after lHiddenPalettes
   end if
   
   delete last char of lHiddenPalettes
   
   unlock messages
   unlock screen
end revSEHidePalettes

private command log pMessage
   put pMessage & return after msg
end log

# OK-2008-05-12 : Added to fix problem with script editor dialogs not being recognized as part of the script editor
# and palettes consequently being reshown when they are invoked.
# Parameters
#   pStackname : the short name of a stack
# Returns
#   Whether or not the stack pStackname is part of the script editor.
private function revSEStackIsScriptEditor pStackname
   # The script editor mainstack is just a container for the script editor's components,
   # if this stack is open it means we're editing the script editor, so effectively the stack
   # is not part of the script editor.
   if pStackname is the short name of revScriptEditorMain() then
      return false
   end if
   
   # All substacks of the script editor are included at the moment. This includes dialogs and script editor instances
   if the owner of stack pStackname is not empty and the short name of the owner of stack pStackname is the short name of revScriptEditorMain() then
      return true
   end if
   
   return false
end revSEStackIsScriptEditor

on revSEShowPalettes
  send "revSEDelayShowPalettes" to me in 200 milliseconds
  put the result into lShowPalettesMsgID
end revSEShowPalettes

on revSEDelayShowPalettes
   if the iconic of stack "revMenuBar" then exit revSEDelayShowPalettes
   
   local tObject
   put the focusedObject into tObject
   if tObject is not empty then
      local tStack
      put revTargetStack(tObject) into tStack
      if tStack is "Answer dialog" or "revVariableWatcher" is in tStack or "revMessageWatcher" is in tStack or revSEStackIsScriptEditor(tStack) then exit revSEDelayShowPalettes
   end if
   lock screen
   
   local tRestore
   if tObject is not empty and tStack is "Message Box" and the number of this cd of stack "Message box" is among the items of "1,2" then
      put true into tRestore
   end if
   repeat for each line tHiddenStack in lHiddenPalettes
      # OK-2009-10-08 : Its possible that the stack could be been removed between showing and hiding the palettes,
      # this is due to extra mutable windows added with the new script editor. Check this first or it will keep throwing
      # errors.
      if there is no stack tHiddenStack then
         next repeat
      end if
      lock messages
      show stack tHiddenStack
      if the mode of stack tHiddenStack is 0 then--Unix Window Managers 
         if word 1 of tHiddenStack is "revPropertyPalette" then 
            send "revUpdateTabs" to stack tHiddenStack
         end if
         # TH-2008-01-22 : If mode of stack is 0 the stack has been closed and should
         # therefore not be opened again.  Bug 2641
         --palette l
      end if
   end repeat
   if tRestore then 
      select text of fld "Message Field" of stack "Message Box" -- works better than "focus"
   end if
   put empty into lHiddenPalettes
   unlock screen
   unlock messages
end revSEDelayShowPalettes


# OK-2007-11-29 : Bug 5562
# Parameters
#   pStack : refererence to stack to import image into.
# Description
#   Prompts the user to select images, if some images are selected then
#   imports these into pStack. If pStack is not given then imports into the defaultStack
on revImportImageFile pStack
  local tDefaultStack
  put the defaultStack into tDefaultStack
  
  local tStack
  if pStack is empty then
    put the long id of the defaultStack into tStack
  else
    put pStack into tStack
  end if
  
  try
    local tFilePaths
    put revAnswerFiles("image", "Select an image file to import:") into tFilePaths
    if tFilePaths is empty then
      exit revImportImageFile
    end if
    
    repeat for each line tFilePath in tFilePaths
      local tConvertPICT
      put false into tConvertPICT 
       
      if the platform is "MacOS" then
        if revIsPICT(tFilePath) then
          put true into tConvertPICT
          if not (the cNOPICTWarning of stack "revPreferences") then
            answer warning "This file is in Macintosh PICT format, which can only be displayed on Mac OS systems.  " & \
                "Revolution can convert the imported image to PNG format, which can be displayed cross platform. " & \
                "Would you like Revolution to convert the imported image to PNG format?" \
                with "Don't Show Again" or "Cancel" or "Don't Convert" or "Convert to PNG Format"
             
            if it is "Cancel" then
              exit revImportImageFile
            end if
            if it is "Don't Show Again" then
              set the cNOPICTWarning of stack "revPreferences" to true
            end if
            if it is "Don't Convert" then
              put false into tConvertPICT
            end if
          end if
        end if
        if tFilePath is not empty then
          lock screen
          
          # OK-2007-11-23 : Bug 5562
          set the defaultStack to the short name of tStack
          
          import paint from file tFilePath
          if tConvertPICT then
            import snapshot from last image
            set the name of last image to the short name of image (the number of images -1)
            delete image (the number of images -1)
          end if
          set the loc of last image to the loc of this card
          
          set the defaultStack to tDefaultStack
          
          unlock screen
        end if
      else
        # Windows and Unix
        if revIsPICT(tFilePath) then
          put true into tConvertPICT
          if not (the cNOPICTWarning of stack "revPreferences") then
            answer warning "This file is in Macintosh PICT format.  This format can only be displayed on Mac OS systems." with "Don't Show Again" or "Cancel" or "Continue"
            if it is "Cancel" then
              exit revImportImageFile
            end if
            if it is "Don't Show Again" then
              set the cNOPICTWarning of stack "revPreferences" to true
            end if
            if it is "No" then
              put false into tConvertPICT
            end if
          end if
        end if
        if tFilePath is not empty then
          # OK-2007-11-23 : Bug 5562
          set the defaultStack to the short name of tStack
          
          import paint from file tFilePath
          set the loc of last image to the loc of this card
          
          set the defaultStack to tDefaultStack
        end if
      end if
    end repeat
  catch someError
    answer "An error occured when trying to import the image file:" & cr & tFilePath & cr & someError
  end try
end revImportImageFile

on revUpdateAOControls pTarget
   global gREVStackName
   
   local tTargetStack
   put revTargetStack(pTarget) into tTargetStack
   
   local tHCAddressing
   put the hcAddressing of stack tTargetStack into tHCAddressing
   set the hcAddressing of stack tTargetStack to false
   if pTarget is empty then put the long id of the target into pTarget
   else put the long id of pTarget into pTarget -- id with HCAddressing false
   send "revClearControlCache pTarget" to stack "revApplicationOverview"
   if the mode of stack "revApplicationOverview" is not 0 \
   and revTargetStack(pTarget) is gREVStackName \
   and the short name of this cd of stack "revApplicationOverview" is "Cards and Controls" then
      
      if lUpdateAOControlsTimerID is not empty then cancel lUpdateAOControlsTimerID
      send "revRefreshControls true, false" to cd "Cards and Controls" of stack "revApplicationOverview" in 1 second
      put the result into lUpdateAOControlsTimerID
   end if
   set the hcAddressing of stack tTargetStack to tHCAddressing
end revUpdateAOControls

function revColorToNumber pWhich
   local tColorTranslation
   put the cREVColorTranslation of stack "revLibrary" into tColorTranslation
   --cREVColorTranslation includes synonyms
   if pWhich is item 1 of tColorTranslation
   then return item 2 to 4 of tColorTranslation
   
   local tItemNo
   put itemOffset(cr & pWhich,tColorTranslation) into tItemNo
   return line 1 of (item tItemNo+1 to tItemNo+3 of tColorTranslation)
end revColorToNumber

function revNumberToColor pColor
   local tColorTranslation
   put the cREVColorTranslation2 of stack "revLibrary" into tColorTranslation
   --cREVColorTranslation2 doesn't include synonyms
   --so the name returned is always in fld "Color Names"
   if pColor is in item -3 to -1 of tColorTranslation then return word 2 of item -4 of tColorTranslation
   
   local tLine
   put lineOffSet(pColor&cr,tColorTranslation) into tLine
   return item 1 of line tLine of tColorTranslation
end revNumberToColor

function revComponentVersion pStack
   local tResult
   put empty into tResult
   if pStack is "all" then
      local tList
      put the stackfiles of stack "home" into tList
      
      local tStack
      repeat for each line i in tList
         put item 1 of i into tStack
         if there is a stack tStack then
            put tStack & ":" && the cVersionNo of stack tStack & "-" & the cBuildNo of stack tStack & cr after tResult 
         end if
      end repeat
      put "home:" && the cVersionNo of stack "home" & "-" & the cBuildNo of stack "home" & cr after tResult 
   else
      if there is a stack pStack then
         put the cVersionNo of stack pStack & "-" & the cBuildNo of stack pStack into tResult 
      end if
   end if
   return tResult
end revComponentVersion

function revExternalVersion pExternal
   local tExternalVersion
   put empty into tExternalVersion
   
   local tExternalList
   put empty into tExternalList
   if (the platform is "MacOS") and (char 1 of the systemVersion is not "1") then
      --MacOS 9+
      local tOS9HomePath
      put the fileName of stack "home" into tOS9HomePath
      
      local tCheckExternals
      put getResources(tOS9HomePath,"CODE") into tCheckExternals
   else
      -- Win,OSX and Unix
      put the externals of stack "home" into tCheckExternals
   end if
   
   local tExternals
   repeat for each line i in tCheckExternals
      repeat for each item j in "revdb,revxml,revvideograbber,revspeech"
         if i contains j then
            put j & "," after tExternals
         end if
      end repeat
   end repeat
   if the last char of tExternals is comma then delete the last char of tExternals
   repeat for each item k in tExternals
      switch k
         case "revxml"
            local tXML
            put "2.1RC1-1" into tXML
            put k & ":" && tXML & cr after tExternalVersion
         break
         case "revdb"
            local tDB
            put "2.1RC1-1" into tDB
            put k & ":" && tDB & cr after tExternalVersion
         break
         case "revvideograbber"
            local tVideoGrabber
            put "2.1RC1-1" into tVideoGrabber
            put k & ":" && tVideoGrabber & cr after tExternalVersion
         break
         case "revspeech"
            local tSpeech
            put "2.1RC1-1" into tSpeech
            put k & ":" && tSpeech & cr after tExternalVersion
         break
      end switch
      put k & "," after tExternalList
   end repeat
   if the last char of tExternalList is comma then delete the last char of tExternalList
   if pExternal is not "all" then
      if pExternal is among the items of tExternalList then
         local tNumber
         put itemoffset(pExternal,tExternalList) into tNumber
         
         local tLineVersion
         put line tNumber of tExternalVersion into tLineVersion
         set the itemDelimiter to ":"
         
         local tVersion
         put char 2 to -1 of item 2 of tLineVersion into tVersion
         return tVersion
      end if
   else
      return tExternalVersion
   end if
end revExternalVersion

on revUpdateAB
  if the mode of stack "revApplicationOverview" is not 0 then
    lock screen
    send "revRetainStacks" to stack "revApplicationOverview"
    send "revInitAppBrowser" to stack "revApplicationOverview"
    unlock screen
  end if
end revUpdateAB

# OK-2007-08-02 : Added revGetDatabaseDriverPath to revDB external, and updated this function to use it.

--function revDatabaseDriverList
--  local tPath,tSave,tDrivers
--  put revEnvironmentExternalsPath() & "/Database Drivers/Database Drivers.txt" into tEnvironmentFile
--  put revEnvironmentUserExternalsPath() & "/Database Drivers/Database Drivers.txt" into tUserFile
--  put url ("file:"&tUserFile) into tUserDrivers
--  put url ("file:"&tEnvironmentFile) into tEnvironmentDrivers
--  repeat for each line l in tUserDrivers
--    put item 1 of l & cr after tDrivers
--  end repeat
--  repeat for each line l in tEnvironmentDrivers
--    if item 1 of l is not among the lines of tDrivers then put item 1 of l & cr after tDrivers
--  end repeat
--  delete last char of tDrivers
--  return tDrivers
--end revDatabaseDriverList

constant kDriverListFile = "Database Drivers.txt"

-- MW-2013-06-13: [[ CloneAndRun ]] Construct the list of dbdrivers manually
function revDatabaseDriverBuildList
   if revEnvironmentIsInstalled() then
      pass revDatabaseDriverBuildList
   end if
   
   local tList
   put empty into tList
   // AL-2015-02-17: [[ SB Inclusions ]] Ensure mysql module name is standard (was dbmysql)
   repeat for each item tExternal in "MySQL mysql,ODBC odbc,PostgreSQL postgresql,SqLite sqlite,Oracle oracle"
      get lineOffset(word 1 of tExternal, the externals of stack "Home")
      if it is not 0 then
         set the itemDelimiter to slash
         put word 2 to -1 of tExternal, the last item of line it of the externals of stack "Home" & return after tList
         set the itemDelimiter to comma
      end if
   end repeat
   delete the last char of tList
   return tList
end revDatabaseDriverBuildList

function revDatabaseDriverList
   local tEnvironmentList
   local tDrivers
   
   -- MW-2013-06-13: [[ CloneAndRun ]] Special-case when not an installation.
   if revEnvironmentIsInstalled() then
      # OK-2007-09-24 : Legacy driver support. Unfortunately the legacy driver support also includes a legacy revDB external,
      # which does not support the revGetDatabaseDriverPath function. So if legacy drivers are to be used, we need to revert to the old
      # way of listing the drivers. Because the setting does not take place until Revolution is restarted, we can't just check it 
      # directly, so instead we simply attempt to call the function revGetDatabaseDriverPath and assume if its not there that legacy drivers
      # are in use.
      local tDriverPaths
      try
         put revGetDatabaseDriverPath() into tDriverPaths
      catch tError
         put revEnvironmentExternalsPath() & "/Legacy/Database Drivers" into tDriverPaths
         put return & revEnvironmentUserExternalsPath() & "/Legacy/Database Drivers" after tDriverPaths
      end try
      
      local tEnvironmentFile
      put line 1 of tDriverPaths & slash & kDriverListFile into tEnvironmentFile
      
      local tUserFile
      put line 2 of tDriverPaths & slash & kDriverListFile into tUserFile
      
      # OK-2007-09-24 : Legacy driver support. If legacy drivers are in use, we ignore the environment list
      # as things might get confused if we use a mixture of new and legacy drivers.
      if the cREVUseLegacyDrivers of stack "revPreferences" then
         put empty into tEnvironmentList
      else
         put url ("file:" & tEnvironmentFile) into tEnvironmentList
      end if
      
      local tUserList
      put url ("file:" & tUserFile) into tUserList
      
      # OK-2007-09-14 : Bug 5394 : On linux the engine does not convert the line endings
      # correctly resulting in only the first line of the drivers list being returned. We 
      # work around this for now until the bug is fixed in the engine.
      if the platform is "Linux" then
         # Windows line endings
         replace (numToChar(10) & numToChar(13)) with numToChar(10) in tUserList
         replace (numToChar(10) & numToChar(13)) with numToChar(10) in tEnvironmentList
         
         # Mac line endings
         replace numToChar(13) with numToChar(10) in tUserList
         replace numToChar(13) with numToChar(10) in tEnvironmentList
      end if
      
      repeat for each line tDriver in tUserList
         put item 1 of tDriver & return after tDrivers
      end repeat
   else
      put revDatabaseDriverBuildList() into tEnvironmentList
   end if
   
   repeat for each line tDriver in tEnvironmentList
      if item 1 of tDriver is not among the lines of tDrivers then
         put item 1 of tDriver & return after tDrivers
      end if
   end repeat
   delete the last char of tDrivers
   
   return tDrivers
end revDatabaseDriverList

-- MW-2013-06-13: [[ CloneAndRun ]] If we aren't installed, then generate the externals
--   list by hand.
private function revExternalsBuildList 
   local tList
   put empty into tList
   repeat for each item tExternal in "revbrowser Browser,revdb Database,revspeech Speech,revvideograbber Video Grabber,revxml XML,revzip Revolution Zip,revfont Font Support"
      get lineOffset(word 1 of tExternal, the externals of stack "Home")
      if it is not 0 then
         set the itemDelimiter to slash
         put word 2 to -1 of tExternal, the last item of line it of the externals of stack "Home" & return after tList
         set the itemDelimiter to comma
      end if
   end repeat
   delete the last char of tList
   return tList
end revExternalsBuildList

function revExternalsList
   local tPath,tSave,tDrivers
   
   -- MW-2013-06-13: [[ CloneAndRun ]] Special-case when not an installation.
   local tEnvironmentFile
   if revEnvironmentIsInstalled() then
      
      put revEnvironmentExternalsPath() & "/Externals.txt" into tEnvironmentFile
      
      local tUserFile
      put revEnvironmentUserExternalsPath() & "/Externals.txt" into tUserFile
      
      local tUserDrivers
      put url ("file:"&tUserFile) into tUserDrivers
      
      local tEnvironmentDrivers
      put url ("file:"&tEnvironmentFile) into tEnvironmentDrivers
      
      repeat for each line l in tUserDrivers
         put item 1 of l & cr after tDrivers
      end repeat
   else
      put revExternalsBuildList() into tEnvironmentDrivers
   end if
   
   repeat for each line l in tEnvironmentDrivers
      if item 1 of l is not among the lines of tDrivers then put item 1 of l & cr after tDrivers
   end repeat
   delete last char of tDrivers
   return tDrivers
end revExternalsList

constant kDriverIndexName = "Database Drivers.txt"
constant kDriverIndexSubFolder = "Database Drivers"

function revDBDriverPath pDriver, pPlatform, pArchitecture
   -- MW-2013-06-13: [[ CloneAndRun ]] If we are not installed, special-case the request.
   if not revEnvironmentIsInstalled() then
      if pPlatform contains "MacOSX" and the platform is not "macos" or \
            pPlatform is "Linux" and the platform is not "linux" or \
            pPlatform is "Windows" and the platform is not "win32" then
         return empty
      end if
      
      repeat for each line tLine in revDatabaseDriverBuildList()
         if item 1 of tLine is pDriver then
            return revEnvironmentBinariesPath() & slash & item 2 of tLine
         end if
      end repeat
      
      return empty
   end if
   
   -- returns path to database driver from runtime or user runtime folder
   -- will be a folder on Mac OS X, a list of files on Windows
   
   local tEnvironmentFolder, tUserFolder
   switch pPlatform
      case "MacOSX"
         if pArchitecture is empty then
            put "Universal" into pArchitecture
         end if
         put revEnvironmentRuntimePath() & "/Mac OS X/" & pArchitecture & "/Externals" into tEnvironmentFolder
         put revEnvironmentUserRuntimePath() & "/Mac OS X/" & pArchitecture & "/Externals" into tUserFolder
         break
         
      case "Windows"
         put revEnvironmentRuntimePath() & "/Windows/x86-32/Externals" into tEnvironmentFolder
         put revEnvironmentUserRuntimePath() & "/Windows/x86-32/Externals" into tUserFolder
         break
         
      case "Linux"
         put revEnvironmentRuntimePath() & "/Linux/" & pArchitecture & "/Externals" into tEnvironmentFolder
         put revEnvironmentUserRuntimePath() & "/Linux/" & pArchitecture & "/Externals" into tUserFolder
         break
   end switch
   
   --   if the cREVUseLegacyDrivers of stack "revPreferences" and pPlatform is not "Linux" then 
   --      put "/Legacy" after tUserFolder
   --   end if
   
   local tEnvironmentFile, tUserFile, tLegacyFile
   put tEnvironmentFolder & slash & kDriverIndexSubFolder & slash & kDriverIndexName into tEnvironmentFile
   put tUserFolder & slash & kDriverIndexSubFolder & slash & kDriverIndexName into tUserFile
   put tUserFolder & "/Legacy/" & kDriverIndexSubFolder & slash & kDriverIndexName into tLegacyFile
   
   local tUserDrivers
   put url ("file:"&tUserFile) into tUserDrivers
   
   local tEnvironmentDrivers
   put url ("file:" & tEnvironmentFile) into tEnvironmentDrivers
   
   # OK-2009-02-03 : Bug 7433 - If using legacy drivers, this is where the index will be. Otherwise the variable
   # will simply be empty and we fall back to using the normal driver.
   local tLegacyDrivers
   if the cREVUseLegacyDrivers of stack "revPreferences" and pPlatform is not "Linux" then 
      put url ("file:" & tLegacyFile) into tLegacyDrivers
   end if
   
   # Attempt to locate a legacy driver if we are meant to use them, otherwise just search for the drivers as normal
   local tLineNo, tBasePath
   if the cREVUseLegacyDrivers of stack "revPreferences" and pPlatform is not "Linux" then
      put lineOffset(pDriver & comma, tLegacyDrivers) into tLineNo
      if tLineNo <> 0 then
         put line tLineNo of tLegacyDrivers into tLine
         put tLegacyFile into tBasePath
      end if
   end if
   
   if tLineNo is empty or tLineNo = 0 then
      put lineOffset(pDriver & comma,tUserDrivers) into tLineNo
      
      if tLineNo is not 0 then 
         put line tLineNo of tUserDrivers into tLine
         put tUserFile into tBasePath
      else
         put lineOffset(pDriver & comma,tEnvironmentDrivers) into tLineNo
         if tLineNo is not 0 then 
            put line tLineNo of tEnvironmentDrivers into tLine
            put tEnvironmentFile into tBasePath
         end if
      end if
   end if
   
   set the itemDel to slash
   delete item -1 of tBasePath
   set the itemDel to comma
   
   local tReturn
   repeat with i = 2 to the number of items in tLine
      put tBasePath & slash & item i of tLine & cr after tReturn
   end repeat
   delete last char of tReturn
   return tReturn
end revDBDriverPath

function revExternalPath pDriver, pPlatform, pArchitecture
   -- returns path to external from runtime or user runtime folder
   -- will be a folder on Mac OS X, a list of files on Windows
   
   -- MW-2013-06-13: [[ CloneAndRun ]] If we aren't an installed environment then compute
   --    relative to the build folder.
   if not revEnvironmentIsInstalled() then
      if pPlatform contains "MacOSX" and the platform is not "macos" or \
            pPlatform is "Linux" and the platform is not "linux" or \
            pPlatform is "Windows" and the platform is not "win32" then
         return empty
      end if
      
      repeat for each line tLine in revExternalsBuildList()
         if item 1 of tLine is pDriver then
            return revEnvironmentBinariesPath() & slash & item 2 of tLine
         end if
      end repeat
      
      return empty
   end if
   
   # OK-2009-01-31 : Bug 7433 - Added support for legacy externals.
   local tEnvironmentfile, tUserFile, tLegacyFile
   switch pPlatform
      case "MacOSX"
         if pArchitecture is empty then
            put "Universal" into pArchitecture
         end if
         put revEnvironmentRuntimePath() & "/Mac OS X/" & pArchitecture & "/Externals/Externals.txt" into tEnvironmentFile
         put revEnvironmentUserRuntimePath() & "/Mac OS X/" & pArchitecture & "/Externals/Externals.txt" into tUserFile
         put revEnvironmentUserRuntimePath() & "/Mac OS X/" & pArchitecture & "/Externals/Legacy/Externals.txt" into tLegacyFile
         break
      case "Windows"
         put revEnvironmentRuntimePath() & "/Windows/x86-32/Externals/Externals.txt" into tEnvironmentFile
         put revEnvironmentUserRuntimePath() & "/Windows/x86-32/Externals/Externals.txt" into tUserFile
         put revEnvironmentUserRuntimePath() & "/Windows/x86-32/Externals/Legacy/Externals.txt" into tLegacyFile
         break
         # OK-2007-08-14 : Linux support reinstated
      case "Linux"
         put revEnvironmentRuntimePath() & "/Linux/" & pArchitecture & "/Externals/Externals.txt" into tEnvironmentFile
         put revEnvironmentUserRuntimePath() & "/Linux/" & pArchitecture & "/Externals/Externals.txt" into tUserFile
         put revEnvironmentUserRuntimePath() & "/Linux/" & pArchitecture & "/Externals/Legacy/Externals.txt" into tLegacyFile
         break
   end switch
   
   local tUserDrivers
   put url ("file:" & tUserFile) into tUserDrivers
   
   local tEnvironmentDrivers
   put url ("file:" & tEnvironmentFile) into tEnvironmentDrivers
   
   # OK-2009-01-31 : Bug 7433
   local tLegacyDrivers
   if the cREVUseLegacyDrivers of stack "revPreferences" then
      put url ("file:" & tLegacyFile) into tLegacyDrivers
   end if
   
   # OK-2009-01-31 : Bug 7433 - If we're set to use legacy drivers, check them first,
   # if no legacy driver was found for the required libray, then we just default to the normal one.
   local tLineNo, tBasePath
   if the cREVUseLegacyDrivers of stack "revPreferences" then
      
      put lineOffset(pDriver & comma, tLegacyDrivers) into tLineNo
      if tLineNo <> 0 then
         put line tLineNo of tLegacyDrivers into tLine
         put tLegacyFile into tBasePath
      end if
   end if
   
   if tLineNo is empty or tLineNo is 0 then
      put lineOffset(pDriver & comma, tUserDrivers) into tLineNo
      
      if tLineNo is not 0 then 
         put line tLineNo of tUserDrivers into tLine
         put tUserFile into tBasePath
      else
         put lineOffset(pDriver & comma, tEnvironmentDrivers) into tLineNo
         if tLineNo is not 0 then 
            put line tLineNo of tEnvironmentDrivers into tLine
            put tEnvironmentFile into tBasePath
         end if
      end if
   end if
   
   set the itemDelimiter to slash
   delete item -1 of tBasePath
   set the itemDelimiter to comma
   
   local tReturn
   repeat with i = 2 to the number of items in tLine
      put tBasePath & slash & item i of tLine & return after tReturn
   end repeat
   delete last char of tReturn
   return tReturn
end revExternalPath




--

function revDirectoryWalking pFolder
   if there is not a folder pFolder then
      if there is a file pFolder then
         return pFolder
      else
         return empty
      end if
   else
      set the defaultFolder to pFolder
      local tList
      put the detailed files into tList
      
      local tFilesList
      repeat for each line i in tList
         put pFolder & "/" & i & cr after tFilesList
      end repeat
      
      local tDirList
      put the folders into tDirList
      repeat with i = 2 to the number of lines of tDirList
         put revDirectoryWalking (pFolder & "/" & (line i of tDirList)) after tFilesList
      end repeat
      return tFilesList
   end if
end revDirectoryWalking

function revDirWalker pFolder
   local tDefaultFolder
   put the defaultFolder into tDefaultFolder
   
   local tList
   put revDirectoryWalking(pFolder) into tList
   set the defaultFolder to tDefaultFolder
   return tList
end revDirWalker

on revCompress pFolder, pSavePath, pName
   local tFileType
   put the fileType into tFileType
   if the paramcount is 0 then
      return "No parameters entered"
      exit revCompress
   end if
   set the itemDelimiter to "/"
   
   local tFolderName
   put item -1 of pFolder into tFolderName
   
   local tDetailedFileList
   put revdirWalker(pFolder) into tDetailedFileList
   if tDetailedFileList is empty then exit revCompress
   set the itemDelimiter to comma
   repeat for each line i in tDetailedFileList
      local tFileList
      put URLDecode( item 1 of i) & cr after tFileList
   end repeat
   set the itemDelimiter to "/"
   
   local tOffset, tList
   repeat for each line i in tFileList
      put itemOffset(tFolderName,i) into tOffset
      put item tOffset to -1 of i & cr after tList
   end repeat
   
   local tStackName
   if (the paramcount is 1) or (the paramcount is 2) then
      put tFolderName & ".dir" into tStackName
   else
      put pName & ".dir" into tStackName
   end if
   
   create stack tStackName
   set the cSourcePlatform of stack tStackName to the platform
   set the cSourceFolderName of stack tStackName to tFolderName
   set the cSourceFilesList of stack tStackName to tFileList
   set the cFilesList of stack tStackName to tList
   repeat with i = 1 to the number of lines in tFileList
      set the cBinFile[line i of tList] of stack tStackName to URL ("binfile:" & line i of tFileList)
      set the cResFile[line i of tList] of stack tStackName to URL ("resfile:" & line i of tFileList)
      if the platform is "MacOS" then
         set the itemDelimiter to comma
         local tType
         put item 11 of line i of tDetailedFileList into tType
         if tType is empty then
            set the cFileType[line i of tList] of stack tStackName to "        "
         else
            set the cFileType[line i of tList] of stack tStackName to tType
         end if
         set the itemDelimiter to "/"
      end if
   end repeat
   
   local tSavePath
   if the paramcount is 1 then
      put the defaultFolder & "/" & tStackName into tSavePath
   else
      if the last char of pSavePath is "/" then delete the last char of pSavePath
      put pSavePath & "/" & tStackName into tSavePath
   end if
   save stack tStackName as tSavePath
   put compress(URL ("binfile:" & tSavePath)) into URL ("binfile:" & tSavePath & ".gz") 
   delete file tSavePath
   close stack tStackName
   delete stack tStackName
   set the fileType to tFileType
   return empty
end revCompress

on revDecompress pFile, pFolder
   local tFileType
   put the fileType into tFileType
   
   local tFile
   put pFile into tFile
   if char -7 to -1 of tFile is ".dir.gz" then
      delete char -3 to -1 of tFile
   else
      exit revDecompress
   end if
   try
      put decompress(URL ("binfile:" & pFile)) into URL ("binfile:" & tFile)
      if the result is not empty then throw the result
   catch tError
      return "Invalid file type"
      exit revDecompress
   end try
   if the last char of pFolder is "/" then delete the last char of pFolder
   
   open stack tFile
   
   local tName, tFilesList, tFolderName
   put the short name of this stack into tName
   put the cFilesList of stack tName into tFilesList
   put the cSourceFolderName of stack tName into tFolderName
   
   local tPlatform
   put the cSourcePlatform of stack tName into tPlatform
   if the paramcount is 1 then
      put the defaultFolder into pFolder
   end if
   
   local tDirPath, tFolderPath, tOffset, tItems, tCreateFolder
   repeat for each line i in tFilesList
      put i into tDirPath
      set the itemDelimiter to "/"
      delete the last item of tDirPath
      put pFolder & "/" & tDirPath into tFolderPath
      put itemOffset(tFolderName,tFolderPath) into tOffset
      put the number of items in tFolderPath into tItems
      repeat with j = tOffset to tItems
         put item 1 to j of tFolderPath into tCreateFolder
         if there is not a folder tCreateFolder then create folder tCreateFolder
      end repeat
      if tPlatform is "MacOS" then
         set the fileType to the cFileType[i] of stack tName
      end if
      put the cBinFile[i] of stack tName into URL ("binfile:" & pFolder & "/" & i)
      put the cResFile[i] of stack tName into URL ("resfile:" & pFolder & "/" & i)
   end repeat
   delete file tFile
   close stack tName
   delete stack tName
   set the fileType to tFileType
   return empty
end revDecompress

on revAlignObjects pDirection -- alignment of objects
   switch pDirection
      case "left"
         local tObjectNames
         put the selectedObjects into tObjectNames
         
         local tNumberOfObjects
         put the number of lines in tObjectNames into tNumberOfObjects
         if tNumberOfObjects < 2 then exit to top
         get line 1 of tObjectNames
         
         local tXCoord
         put the left of it into tXcoord
         repeat with i = 1 to tNumberOfObjects
            get line i of tObjectNames
            set the left of it to tXcoord
            send "revCacheGeometry true" to it -- geometry
         end repeat
      break
      case "right"
         put the selectedObjects into tObjectNames
         put the number of lines in tObjectNames into tNumberOfObjects
         if tNumberOfObjects < 2 then exit to top
         get line 1 of tObjectNames
         put the right of it into tXcoord
         repeat with i = 1 to tNumberOfObjects
            get line i of tObjectNames
            set the right of it to tXcoord
            send "revCacheGeometry true" to it -- geometry
         end repeat
      break
      case "top"
         put the selectedObjects into tObjectNames
         put the number of lines in tObjectNames into tNumberOfObjects
         if tNumberOfObjects < 2 then exit to top
         get line 1 of tObjectNames
         
         local tYcoord
         put the top of it into tYcoord
         repeat with i = 1 to tNumberOfObjects
            get line i of tObjectNames
            set the top of it to tYcoord
            send "revCacheGeometry true" to it -- geometry
         end repeat
      break
      case "bottom"
         put the selectedObjects into tObjectNames
         put the number of lines in tObjectNames into tNumberOfObjects
         if tNumberOfObjects < 2 then exit to top
         get line 1 of tObjectNames
         put the bottom of it into tYcoord
         repeat with i = 1 to tNumberOfObjects
            get line i of tObjectNames
            set the bottom of it to tYcoord    
            send "revCacheGeometry true" to it -- geometry
         end repeat
      break
      case "Make widths equal"
         put the selectedObjects into tObjectNames
         put the number of lines in tObjectNames into tNumberOfObjects
         if tNumberOfObjects < 2 then exit to top
         get line 1 of tObjectNames
         
         local tFirstWidth
         put the width of it into tFirstWidth
         repeat with i = 1 to tNumberOfObjects
            get line i of tObjectNames
            set the width of it to tFirstWidth
            send "revCacheGeometry true" to it -- geometry
         end repeat
      break
      case "Make heights equal"
         put the selectedObjects into tObjectNames
         put the number of lines in tObjectNames into tNumberOfObjects
         if tNumberOfObjects < 2 then exit to top
         get line 1 of tObjectNames
         
         local tFirstHeight
         put the height of it into tFirstHeight
         repeat with i = 1 to tNumberOfObjects
            get line i of tObjectNames
            set the height of it to tFirstHeight
            send "revCacheGeometry true" to it -- geometry
         end repeat
      break
   end switch
end revAlignObjects

on revSaveLayout -- save current object layer and rect for undo
   global gREVUndoList
   put empty into gREVUndoList
   
   local tSavedList
   put the selectedObjects into tSavedList
   repeat for each line l in tSavedList
      put l,the rect of l & cr after gREVUndoList
   end repeat
   put "rect" after gREVUndoList
end revSaveLayout


on revSendToBack pObjects
  local l, tObjectArray, tKeys
  
  #maintain the layer order of a multiple-object selection
  put revObjectOrder(pObjects) into tObjectArray
  put keys(tObjectArray) into tKeys
  sort lines of tKeys numeric descending
  
  repeat for each line l in tKeys
    #relayering grouped controls can cause an error
    try
      set the layer of tObjectArray[l] to bottom
    catch tJunk
    end try
  end repeat
  revUpdateAOControls line 1 of pObjects
end revSendToBack

function revObjectOrder pObjects
   local tObjectArray
  repeat for each line l in pObjects
    put l into tObjectArray[the layer of l]    
  end repeat
  return tObjectArray  
end revObjectOrder

on revMoveBackward pObjects
  local l, tObjectArray, tKeys
  
  #maintain the layer order of a multiple-object selection
  put revObjectOrder(pObjects) into tObjectArray
  put keys(tObjectArray) into tKeys
  sort lines of tKeys numeric ascending
  
  repeat for each line l in tKeys
    #relayering grouped controls can cause an error
    try
      set the layer of tObjectArray[l] to (l - 1)
    catch tJunk
    end try
  end repeat
  revUpdateAOControls line 1 of pObjects
end revMoveBackward

on revMoveForward pObjects
   local l, it
   
   #maintain the layer order of a multiple-object selection
   local tObjectArray
   put revObjectOrder(pObjects) into tObjectArray
   
   local tKeys
   put keys(tObjectArray) into tKeys
   sort lines of tKeys numeric descending
   
   repeat for each line l in tKeys
      #relayering grouped controls can cause an error
      try
         set the layer of tObjectArray[l] to (l + 1)
      catch tJunk
      end try
   end repeat
   revUpdateAOControls line 1 of pObjects
end revMoveForward

on revBringToFront pObjects
  local l, tObjectArray, tKeys
  
  #maintain the layer order of a multiple-object selection
  put revObjectOrder(pObjects) into tObjectArray
  put keys(tObjectArray) into tKeys
  sort lines of tKeys numeric ascending
  
  repeat for each line l in tKeys
    #relayering grouped controls can cause an error
    try
      set the layer of tObjectArray[l] to top
    catch tJunk
    end try
  end repeat
  revUpdateAOControls line 1 of pObjects
end revBringToFront






##### mf 03/08/2005 Functions to check whether a stack can be removed from memory 
##### without causing problems (Bug 2475).
function revBeforeRemovingStackFromMemory pStack
  local tStack, tStacksList, tInstructions, tCheckScriptEditors
  
  put the mainStack of stack pStack into tStack
  if the substacks of stack tStack is empty
  then put tStack into tStacksList
  else put tStack & cr & the substacks of stack tStack into tStacksList
  put revCheckLockedInspectors(tStacksList) into tInstructions
  put revCheckScriptEditors(tStacksList) into tCheckScriptEditors
  if tCheckScriptEditors is not empty then put cr & tCheckScriptEditors after tInstructions
  
  if the mode of stack "revMenuManager" is not 0 and \
      the cREVDefaultStack of stack "revMenuManager" is among the lines of tStacksList
  then put cr & "Close the Menu Builder." after tInstructions
   
  if the mode of stack "revSearch" is not 0
  then put cr & "Close the Find and Replace dialog." after tInstructions
   
  if the mode of stack "revDBQuerySetup" is not 0 and \
      the short name of the topStack is among the lines of tStacksList
  then put cr & "Close the Database Query Builder." after tInstructions
   
  if the mode of stack "revStandaloneSettings" is not 0 and \
      the cREVTargetStack of stack "revStandaloneSettings" is among the lines of tStacksList
  then put cr & "Close the Standalone Application Settings dialog." after tInstructions
   
  if (the cREVLockedtoStack of stack "Message Box") and \
      the cREVLinkedStack of stack "Message Box" is among the lines of tStacksList
  then put cr & "Unlock the Message Box." after tInstructions
   
  if char 1 of tInstructions is cr then delete char 1 of tInstructions
  return tInstructions
end revBeforeRemovingStackFromMemory

function revCheckLockedInspectors pStacksList
  local tOpenStacks, l, m
  put the openStacks into tOpenStacks
  repeat for each line l in tOpenStacks
    if word 1 of l is "revPropertyPalette" then
      repeat for each line m in the cREVLinked of stack l
        if revTargetStack(m) is among the lines of pStacksList
        then return "Close any Object Inspectors locked to objects in this stack or its substacks."
      end repeat
    end if
  end repeat
  return empty
end revCheckLockedInspectors

function revCheckScriptEditors pStacksList
  local tOpenStacks, l, tStack
  put the openStacks into tOpenStacks
  repeat for each line l in tOpenStacks
    if word 1 of l is "revScriptEditor" then
      put revTargetStack(the cTargetObject of this cd of stack l) into tStack
      if tStack is among the lines of pStacksList
      then return "Close any open Script Editors for any objects in this stack or its substacks."
    end if
  end repeat
  return empty      
end revCheckScriptEditors
#####mf 03/08/2005 End of functions to check whether a stack can be removed from memory 
##### without causing problems (Bug 2475)


-- prevent plug-in errors if messages not handled

on revCloseStack
  
end revCloseStack

on revEditScript
  
end revEditScript

on revIDChanged
  
end revIDChanged

on revMouseMove
  
end revMouseMove

on revMoveControl
  
end revMoveControl

on revNameChanged
  
end revNameChanged

on revNewTool
  
end revNewTool

on revPreOpenCard
  
end revPreOpenCard

on revPreOpenStack
  
end revPreOpenStack

on revResizeStack
  
end revResizeStack

on revResumeStack
  
end revResumeStack

on revSaveStackRequest
  
end revSaveStackRequest

on revSelectedObjectChanged
  
end revSelectedObjectChanged

on revSelectionChanged
  
end revSelectionChanged

on revShutDown
  
end revShutDown

function revCombineFilePaths pPaths, pPaths2
   -- combines file paths based on last item, where duplicates keeps list 1
   set the itemDel to slash
   
   local tPaths
   repeat for each line l in pPaths2
      if the number of words in l is 0 then next repeat
      put l into tPaths[item -1 of l]
   end repeat
   repeat for each line l in pPaths
      if the number of words in l is 0 then next repeat
      put l into tPaths[item -1 of l]
   end repeat
   
   local tReturn
   repeat for each element e in tPaths
      put e & cr after tReturn
   end repeat
   delete last char of tReturn
   return tReturn
end revCombineFilePaths

function revAbsoluteFolderListing pDirectory
   local tReturn
   if there is no directory pDirectory then return empty
   put the directory into tReturn
   set the directory to pDirectory
   
   local tFiles
   put the files into tFiles
   
   local tOutput
   repeat for each line l in tFiles
      put the directory & slash & l & cr after tOutput
   end repeat
   delete last char of tOutput
   set the directory to tReturn
   return tOutput
end revAbsoluteFolderListing

local lExecutingDesktopChanged

on deskTopChanged
  if lExecutingDesktopChanged then exit deskTopChanged
  put true into lExecutingDesktopChanged
  repeat for each line l in the openStacks
    send "deskTopChanged" to cd 1 of stack l
  end repeat
  put false into lExecutingDesktopChanged
end deskTopChanged



################################################################################
#######################  Documentation Stack Backscript ########################
################################################################################

-- Number of xml files per clump
constant kFilesPerClump = 50
-- Name of clump index
constant kIndexName = "clumps.index"
-- Number of clumps allowed to be open at once
constant kMaxOpenClumps = 10
-- Clump name prefix
constant kClumpNamePrefix = "revDocClump_"


function revDocumentationRetrieve pType, pTag, pFolder
   -- We know how many files per clump cuz of kFilesPerClump
   -- pType is either "dictionary/" or "glossary/"
   local tClumpNumber, tFileNumber, tPropNumber, tPropName
   local tClumpName, tDocData, tClumpPath, tType, tOpenClumps
   ---------
   
   put pType into tType
   if the last char of tType is not slash then put slash after tType
   
   if pFolder is empty then
      put revEnvironmentDocumentationPath() & slash & "packaged_xml/" & tType into tClumpPath
   else
      put pFolder & slash & "packaged_xml/" & tType into tClumpPath
   end if
   
   set the itemDelimiter to "."
   put item 1 of pTag into tFileNumber
   if tFileNumber is not a number then
      return empty
   end if
   put ((tFileNumber - 1) div kFilesPerClump) + 1 into tClumpNumber
   put ((tFileNumber - 1) mod kFilesPerClump) + 1 into tPropNumber
   
   put "c" & tPropNumber into tPropName
   put the mainStacks into tOpenClumps
   filter tOpenClumps with kClumpNamePrefix & "*"
   if the number of lines of tOpenClumps > (kMaxOpenClumps -1) then
      local tLines, tRandomLine
      -----
      set the randomSeed to the seconds
      put the number of lines of tOpenClumps into tLines
      put the random of tLines into tRandomLine
      if there is a stack (line tRandomLine of tOpenClumps) then
         delete stack (line tRandomLine of tOpenClumps)
      end if
      delete line tRandomLine of tOpenClumps
   end if
   
   put kClumpNamePrefix & tClumpNumber into tClumpName
   
   put the tPropName of stack (tClumpPath & tClumpName & ".rev") into tDocData
   if tDocData is empty then
      return empty
   end if
   
   -- We've got the doc's compressed data, uncompress and return
   return the decompress of tDocData
end revDocumentationRetrieve

function revCalculateRelativePath pRoot, pFile
  local tResult, tCount
  set the itemDel to "/"
  put 1 into tCount
  local tIsFolder
  put there is a folder pFile into tIsFolder
  repeat for each item tNode in pRoot
    if item 1 to tCount of pRoot is item 1 to tCount of pFile then
    else
      put "../" before tResult
      if item tCount of pFile is not empty then
        put item tCount of pFile & "/" after tResult
      end if
    end if
    add 1 to tCount
  end repeat
  put "./" before tResult
  put item tCount to -1 of pFile after tResult
  
  if char -1 of tResult is "/" then
    delete char -1 of tResult
  end if
  
  return tResult
end revCalculateRelativePath


# OK-2008-06-24 : Bugs 6592 and 6591
# Returns
#   A list of the open stack names which should be included in a window menu. 
function revIDEWindowList
   local tOpenstacks
   put the openStacks into tOpenstacks
   
   local tStacks
   repeat for each line tStackName in tOpenstacks
      if tStackName begins with kScriptEditorPrefix or tStackName is "revDictionary" or tStackName is "revResourceCenter" then
         put the name of stack tStackName & return after tStacks
      else if char 1 to 3 of tStackName is not "rev" and tStackName is not among the items of "Message Box,Home,answer dialog,ask dialog,script debugger,file selector" then
         put the name of stack tStackName & return after tStacks
      end if
   end repeat
   delete the last char of tStacks
   
   if tStacks is empty then
      return empty
   else
      local tTopMostStack
      put line 1 of tStacks into tTopMostStack
      
      local tOtherstacks
      put line 2 to -1 of tStacks into tOtherstacks
      sort tOtherstacks
      
      put tTopMostStack & return & tOtherstacks into tStacks
   end if
   
   return tStacks
end revIDEWindowList

# Returns
#   A list of open Windows in suitable format for putting into a menu button. This will be any
#   script editors that are open, the Dictionary and Resource Center if they are open, and user stacks.
#   If none of these things are open, the function will return empty. This will contain the same number of
#   lines as the revIDEWindowList function.
function revIDEWindowListAsMenu
   local tFinal
   put revIDEWindowList() into tFinal
   
   local tTopMostStack
   put line 1 of tFinal into tTopMostStack
   if tFinal is empty then      
      return empty
   else
      local tStacks
      put empty into tStacks
      repeat for each line tStack in tFinal
         if the short name of tStack begins with kScriptEditorPrefix or the short name of tStack is "revDictionary" or the short name of tStack is "revResourceCenter" then
            put "!n" & the title of tStack & return after tStacks
         else
            # OK-2010-02-19: Bug 8157 - Escape special characters in stack names before building menu
            put "!n" & revIDEEscapeStackNameForMenu(the short name of tStack) & return after tStacks
         end if
      end repeat
      delete last char of tStacks
      
      set the wholeMatches to true   
      put "!c" into char 1 to 2 of line (lineOffset(tTopMostStack,tFinal)) of tStacks
      if the platform is "MacOS" then 
         replace "/" with "\/" in tStacks
         replace "(" with "\(" in tStacks
      else
         replace "/" with "//" in tStacks
      end if
   end if
   
   return tStacks
end revIDEWindowListAsMenu

# OK-2010-02-19: Bug 8157 - Escape special characters from stack names in menus, more may 
# need to be added here.
private function revIDEEscapeStackNameForMenu pName
   replace "!" with "\!" in pName
   return pName
end revIDEEscapeStackNameForMenu

# OK-2008-03-31 : Bug 6280
local sBlockMouseUp
local sPopupMenuUp

command revClearMouseUpBlock
  put false into sBlockMouseUp
end revClearMouseUpBlock

# OK-2008-03-27 : Bug where right-click context menus won't work with messages suppressed
# Added ability to explicity specify a target.

# Parameters
#   pWhich : the button number pressed (1, 2 or 3)
#   pTarget : reference to the object that was clicked on
# Returns
#   Either empty if the caller should decide whether to pass the message, or one of the following:
#   "exit", "exit to top", "pass".
command revIDEHandleMouseUp pWhich, pTarget
  global gREVPassMouseUp, gREVPopups
  
  local tTarget
  put the long id of pTarget into tTarget
  
  if sBlockMouseUp then 
    put false into sBlockMouseUp
    return "exit to top"
  end if
  
  if gREVPassMouseUp then
    put false into gREVPassMouseUp
    return "pass"
  end if
  
  if sPopupMenuUp then
    put false into sPopupMenuUp
    return "exit"
  end if
  
  if the cREVGetsUpdate of stack revTargetStack(tTarget) is not empty and the backPattern of tTarget is 200921 then
    set the backPattern of tTarget to empty
  end if
   
  if the platform is not "MacOS" and the commandKey is down and the shiftKey is down and pWhich is 3 and (not revOKTarget(tTarget) or (revOKTarget(tTarget) and gREVPopups)) then
    revShowPopupmenu tTarget
    return "exit"
 end if
 // SN-2014-09-18: [[ Bug 13475 ]] Follow the same rule as revIDEHandleMouseDown, to avoid duplicates
  if pWhich is an integer and revCheckEdit(tTarget) then
    if the platform is not "MacOS" and pWhich is 3 and (not revOKTarget(tTarget) or (revOKTarget(tTarget) and gREVPopups)) then
      revShowPopupmenu tTarget
    end if
    return "exit"
  end if
end revIDEHandleMouseUp

# TH-2008-06-18 : Bug 6217, this code has been copied from revFrontscript which now simply calls this 
# handler.  This prevents the accidental closing of unsaved new stacks when messages are suppressed
# Parameters
#   pTarget : reference to the current card of the stack that is being closed
# Returns
#   Either empty if the caller should decide whether to pass the message, or one of the following:
#   "exit", "exit to top", "pass".
command revIDEHandleCloseStackRequest   pTarget
   local tFocusedObject
   put the focusedObject into tFocusedObject
   if word 1 of tFocusedObject is "field" and word 1 of revTargetStack(tFocusedObject) is "revPropertyPalette" then
      send "closeField" to tFocusedObject
   end if
   
   # OK-2008-02-27 : Bug 5915
   if revSaveCheck(revTargetStack(pTarget)) then
      if gREVSuppressMessages and not (gREVSuppressMessages and revOKTarget()) then
         lock messages
         close stack revTargetStack(pTarget)
         unlock messages
      else
         return "pass"
      end if
   end if
   
   return empty
end revIDEHandleCloseStackRequest


# OK-2008-03-27 : Bug where right-click context menus wouldn't work with messages suppressed.
# Refactored to allow a target to be passed.

# Parameters
#   pWhich : the mouse button number clicked (1, 2 or 3) (Or empty if the mouseDown is the result of a keyboard shortcut)
#   pTarget : reference to the target object of the mouse click.
# Returns
#   Empty if the caller should decide whether to pass the mouseDown message. "exit" if the caller
#   should exit the mouseDown. "Exit to top" if the caller should exit to top.
command revIDEHandleMouseDown pWhich, pTarget
   local tTarget
   put the long id of pTarget into tTarget
   
   global gREVPopups, gREVScriptShortcut, gREVUIScriptShortcut, gREVPaintImage
   if the platform is "MacOS" and the commandKey is down and the shiftKey is down and pWhich is 3 and (not revOKTarget(tTarget) or (revOKTarget(tTarget) and gREVPopups)) then
      put true into sPopupMenuUp
      revShowPopupmenu tTarget
      return "exit"
   end if
   
   # OK-2008-03-20: Bug 6153. If the mouseDown was sent because of menu accelerators instead
   # of an actual mouseDown, the button number will be empty, so check this before editing script.
   if the commandKey is down and the optionKey is down and gREVScriptShortcut is true and pWhich is not empty then
      if not revOKTarget(tTarget) or gREVUIScriptShortcut then
         put true into sBlockMouseUp
         send "revClearMouseUpBlock" to me in 1 second
         edit script of tTarget
      end if
      # OK-2008-07-30 - Don't exit to top here because doing so prevents the script editor initializing itself.
      --return "exit to top"
      return "exit"
   end if
   
   # MW-2008-03-20: [[ Bug 6160 ]] If the mouseDown is synthetic (from a non-engine source) then
   # we should pass the message, since there's no nice way to tell where a message was sent from
   # at the moment, we assume that non-engine sources don't pass an integer parameter
   if pWhich is a integer and revCheckEdit(tTarget) then
      if pWhich is 3 then
         if the shiftKey is down then
            set the selected of tTarget to true
         else
            select tTarget
         end if
         if the platform is "MacOS" and (not revOKTarget(tTarget) or (revOKTarget(tTarget) and gREVPopups)) then 
            revShowPopupmenu tTarget
         end if
         send "selectedObjectChanged" to me
      end if
      if the mode of stack "revSaving" is 0 then put "edited" into gREVStackStatus[revTargetStack(tTarget)]
      return "exit to top"
   end if
end revIDEHandleMouseDown

# OK-2008-03-27 : Bug 6229. When a stack is opened with messages suppressed, this message
# is sent by revNoMessages to this script to update the IDE. The reason for this is that we 
# need to know the name of the newly opened stack, which is originally in the target, but is
# lost when the message is sent.

# Parameters
#   pTarget : the target of the message, i.e. current card of the stack that was opened
# Description
#   Updates the IDE in response to a stack being opened.
command revIDEHandleNewStack pTarget
   global gREVStacksList, gREVTargetStack, gREVTempDont, gREVMessageDispatch, gREVScriptEdit, gREVShowStacks
   
   local tParse
   put the long id of pTarget into tParse
   
   local tStackName
   get wordOffset("stack", tParse)
   put word (it) to (it + 1) of tParse into tStackName
   
   revCheckStackCollision tStackName
   
   put true into gREVScriptEdit
   
   send "revCheckToolbar" to me in 100 milliseconds
   
   put the name of tStackName into gREVTargetStack
   --put word -2 to -1 of the long id of the target into gREVTargetStack
   put empty into gREVStacksList
   
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
   
   revUpdateRecentFiles the short name of gREVTargetStack
   
   revSetActiveStacks
   if the mode of stack "revAnimation" is not 0 then
      local tReturn
      put the defaultStack into tReturn
      lock screen
      set the defaultStack to "revAnimation"
      send "preOpenCard" to cd 1 of stack "revAnimation"
      unlock screen
      set the defaultStack to tReturn
   end if
   
   local tStack
   if the mode of stack "revApplicationOverview" is not 0 then
      put revTargetStack(the long id of stack tStackName) into tStack
      send "revNewStack tStack" to stack "revApplicationOverview"
   end if
   
   global gREVMessageDispatch
   repeat for each line l in gREVMessageDispatch["revPreOpenStack"]
      send "revPreOpenStack" to this cd of stack l
   end repeat
end revIDEHandleNewStack


command revOpenStacks
   local filterstring
   if the systemFileSelector then
      switch the platform
         case "Win32"
            put "Revolution Stacks" & cr & "*.rev" & cr & "MetaCard Stacks" & cr & "*.mc" & cr & "All Files" & cr & "*.*" into filterstring
         break
         case "MacOS"
            put "MSTKSTAKTEXTRSTK" into filterstring
         break
         default
            put "*.rev" into filterstring
      end switch
   else put "*.rev" into filterstring
   get revAnswerFiles("stack","Open stack:")
   if it is not empty then
      local tStacksToOpen
      put it into tStacksToOpen
      repeat for each line tStackToOpen in tStacksToOpen
         revOpenStack tStackToOpen
      end repeat
   end if
   return tStacksToOpen
end revOpenStacks

on revOpenStack pStackName
   local tError
   put empty into tError
   set the cursor to watch
   go stack pStackName
   if the result is not empty then 
      put the result into tError
   end if
   if tError is not empty then
      answer error "There was a problem opening that stack:" & cr & tError
      set the cursor to 1
   end if
end revOpenStack

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
