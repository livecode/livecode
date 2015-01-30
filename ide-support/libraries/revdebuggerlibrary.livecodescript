script "revdebuggerlibrary"
global gREVDevelopment
local lReturnHandler, lBreakPointsRestore

on traceError pHandlerName, pWhatError, pWhatError2, pWhatError3
  global gREVMovingPalette, gREVSuppressErrors, gREVRestorePoints
  if gREVDevelopment then write "traceError" && the long time && the params & cr & cr to stdOut
  lock messages
  lock screen
  if the cREVScriptDebugMode of stack "revPreferences" is false then pass traceError
  if item 1 of pWhatError3 is not empty and item 1 of pWhatError3 is not a number then -- user thrown error
    send "errorDialog" && pWhatError3 to the target
    set the traceAbort to true
    set the traceReturn to true
    exit to top
  end if
  if gREVSuppressErrors then
    put the breakPoints into gREVRestorePoints
    send "revRestoreBreakPoints" to me in 1 second
    set the breakPoints to empty
    set the traceAbort to true
    set the traceReturn to true
    exit to top
  end if
  if gREVDevelopment then write "traceError line 2" & cr to stdout
  if pWhatError3 is empty or the enabled of btn "ignore" of stack "revErrorDisplay" or the waitDepth > 2 then 
    -- ditch bogus errors or errors on top of existing debug state
    -- or recursion problems that may lock up the IDE
    if gREVDevelopment then write "traceError branch" & cr to stdOut
    if gREVDevelopment then write the executionContexts & cr to stdout
    if gREVRestorePoints is empty then
      put the breakPoints into gREVRestorePoints
      send "revRestoreBreakPoints" to me in 1 second
      set the breakPoints to empty
      disable btn "ignore" of stack "revErrorDisplay"
      disable btn "debug..." of stack "revErrorDisplay"
      set the traceAbort to true
      set the traceReturn to true
      exit to top
    end if
    exit to top
    put pWhatError2,pWhatError,0,the long id of the target into tError
    if gREVDevelopment then write "traceError executionError" && the executionError of stack "revErrorDisplay" & cr to stdOut
    put true into tWorkAroundError
  else
    put pWhatError3 into tError
  end if 
  global gREVSuppressErrors
  if gREVDevelopment then write "traceError line 4" & cr to stdout
  if gREVDevelopment then write "traceError executionError" && tError to stdOut
  put the long id of the target into tObject
  if gREVDevelopment then write "errorObject" && tObject & cr to stdOut
  if gREVDevelopment then write "traceError line 6" & cr to stdout
  -- handle error code here, too sensitive to send
  -- this is taken from the code in cd 1 of revErrorDisplay
  set the lockCursor to false
  if the platform is "MacOS" and there is a stack "revMenuBar" then
    put the defaultStack into tReturn
    set the defaultStack to "revMenuBar"
    set the defaultMenuBar to "revMenuBar"
    set the defaultStack to tReturn
  end if
  set the defaultStack to "revErrorDisplay"
  set the executionError of this stack to empty
  set the scriptError of this stack to empty
  set the errorObject of this stack to empty
  set the textHeight of fld "error" to 16
  put the htmlText of fld 1 into tFldContent
  enable btn "Ignore"
  set the default of btn "ignore" to true
  enable btn "Debug..."
  if gREVDevelopment then write "traceError line 7" & cr to stdout
  repeat with i = 1 to the number of controls
    if gREVDevelopment then write "traceError line 7a" && i & cr to stdout
    if the cREVGeneral["profile"] of control i is not "debugmode" then
      set the cREVGeneral["profile"] of control i to "debugmode"
      if gREVDevelopment then write "traceError line 7b" && i & cr to stdout
      set the customPropertySet of control i to "cREVDivergeddebugmode"
      if gREVDevelopment then write "traceError line 7c" && i & cr to stdout
      if the customKeys of control i is not empty then put the customProperties of control i into tTempArray1
      if gREVDevelopment then write "traceError line 7d" && i & cr to stdout
      set the properties of control i to tTempArray1
      if gREVDevelopment then write "traceError line 7e" && i & cr to stdout
    end if
  end repeat
  if gREVDevelopment then write "traceError line 8" & cr to stdout
  set the htmlText of fld 1 to tFldContent
  put "     " & tab & "executing at" && the long time & cr into tErrorString
  if item 2 of line 1 of tError is 0 then -- go down until error is found
    repeat with i = 1 to the number of lines in tError
      if item 2 of line i of tError is not 0 then exit repeat
    end repeat
    put i into tErrorLine
  else
    put 1 into tErrorLine
  end if
  if gREVDevelopment then write "traceError line 9" & cr to stdout
  get wordOffset("stack",tObject)
  put word (it) to (it+1) of tObject into tStackName
  put the short name of tStackName into tTargetStack
  if the mode of stack tTargetStack is 5 then 
    toplevel tTargetStack
  end if
  if the passKey of stack tTargetStack is not the passWord of stack tTargetStack then -- its password protected
    answer error "There was an error executing a script in stack" && tTargetStack & ".  No more information is available because the stack is password protected."
    close stack "revErrorDisplay"
    close stack "execution error"
    if gREVRestorePoints is empty then
      put the breakPoints into gREVRestorePoints
      send "revRestoreBreakPoints" to me in 1 second
      set the breakPoints to empty
      set the traceAbort to true
      set the traceReturn to true
      exit to top
    end if
    exit to top
  end if
  put "Type" & tab after tErrorString
  
  if gREVDevelopment then write "traceError line 9i" & cr & tError & cr to stdout
  
  set the itemDel to comma
  put item 1 of line 1 of tError into tNumber -- look up number based on line 1 always
  
  if gREVDevelopment then write "traceError line 9ii" & cr to stdout
  
  if tNumber is 634 then
    put item 4 to -1 of line 1 of tError & cr after tErrorString
  else
    put line tNumber of the cErrorsList of cd 1 of stack "REVErrorDisplay" & cr after tErrorString
  end if
  
  if gREVDevelopment then write "traceError line 9iii" & cr to stdout
  
  put "Object" & tab & the short name of tObject & cr after tErrorString
  
  if gREVDevelopment then write "traceError line 9iv" & cr to stdout
  
  if word 1 to -1 of line (item 2 of tError) of the script of tObject is empty or item 2 of tError is 0 then put tErrorLine into tLineNo else put 1 into tLineNo
  put "Line" & tab & word 1 to -1 of line (item 2 of line tLineNo of tError) of the script of tObject & cr after tErrorString
  if gREVDevelopment then write "traceError line 9v" & cr to stdout
  set the cLastError of this cd to line -2 of tErrorString & cr & tObject
  if gREVDevelopment then write "traceError line 9vi" & cr to stdout
  put false into tExit
  if gREVDevelopment then write "traceError line 9a" & cr to stdout
  repeat for each line l in tError
    if gREVDevelopment then write "traceError line 9a1" & cr to stdout
    if the number of items in l is 4 then
      put true into tExit
      exit repeat
    end if
  end repeat
  if gREVDevelopment then write "traceError line 9a2" & cr to stdout
  if not tExit then put line 1 of tError into l
  if gREVDevelopment then write "traceError line 9a3" & cr to stdout
  put "Hint" & tab & item 4 of l after tErrorString
  if gREVDevelopment then write "traceError line 9a5" & cr to stdout
  if char 1 to 3 of tTargetStack is "rev" then
    put "internal" into word 2 of tErrorString
    put true into tUIError
  end if
  if gREVDevelopment then write "traceError line 9b" & cr to stdout
  if tUIError and not gREVDevelopment then
    put cr & cr & cr & "*** --Internal error" & cr & tErrorString after URL "file:components/save/sessionlog.txt"
    put the breakPoints into gREVRestorePoints
    send "revRestoreBreakPoints" to me in 1 second
    set the breakPoints to empty
    set the traceAbort to true
    set the traceReturn to true
    exit to top
  else
    put tErrorString & cr & cr before fld "error"
  end if
  set the imageSource of char 1 of fld "error" to 202696
  set the textStyle of line 1 of fld "error" to "bold"
  set the textStyle of line 2 to 5 of fld "error" to "plain"
  set the textColor of line 1 to 5 of fld "error" to "black"
  set the hScroll of fld "error" to 0
  
  set the vScroll of fld "error" to 0
  set the hScroll of fld "error" to 0
  put base64encode(line 1 to 5 of fld "error") into tUniqueChunk
  set the tUniqueChunk of this stack to tError & cr & the long id of tObject
  set the cCurrentObject of this cd to tError & cr & the long id of tObject
  enable btn "Edit script..."
  send "revLineColors" to cd 1 of stack "revErrorDisplay" in 20 seconds
  if gREVDevelopment then write "traceError line 9d" && the lockMessages && the defaultStack & cr to stdout
  set the rect of fld "error" to -2,-2,the right of this cd, the bottom of this cd - 90
  set the top of btn "edit script..." to the bottom of fld "error" + 20
  set the right of btn "debug..." to the left of btn "edit script..." - 12
  set the right of btn "ignore" to the left of btn "edit script..." - 12
  set the top of btn "debug..." to the top of btn "edit script..."
  set the top of btn "ignore" to the bottom of btn "debug..." + 12
  unlock messages
  lock messages
  
  --  -- bug 3028, if an error occurs in a modal stack, 
  --  -- toplevel the stack so that the errors palette is accessible
  --  if the mode of tStackName is 5 then toplevel (the short name of tStackName)
  
  global gREVExit, gREVExit2 -- force no messages
  put true into gREVExit
  put true into gREVExit2
  modeless "revErrorDisplay"
  if the formattedWidth of fld "error" > the width of fld "error" then set the hScrollBar of fld "error" to true
  else set the hScrollBar of fld "error" to false
  put false into gREVExit
  put false into gREVExit2
  select line 1 to 5 of fld "error"
  set the hScroll of fld "error" to 0
  if gREVDevelopment then write "traceError line 9e" & cr to stdout
  show stack "revErrorDisplay"
  if tWorkAroundError then
    if gREVRestorePoints is empty then
      put the breakPoints into gREVRestorePoints
      send "revRestoreBreakPoints" to me in 1 second
      set the breakPoints to empty
      disable btn "ignore"
      disable btn "debug..."
      set the traceAbort to true
      set the traceReturn to true
      exit to top
    end if
  else
    enable btn "ignore"
    enable btn "debug..."
  end if
  unlock messages
  unlock screen
  exit traceError -- passing this message goes to traceDone, don't do this
end traceError

on revRestoreBreakPoints
  global gREVRestorePoints
  if gREVDevelopment then write "revRestoreBreakPoints" && the long time && the params & cr & cr to stdOut
  set the breakPoints to gREVRestorePoints
  if gREVRestorePoints is not empty then
    set the cREVScriptDebugMode of stack "revPreferences" to true
  end if
  put empty into gREVRestorePoints
end revRestoreBreakPoints

on traceDone
  if gREVDevelopment then write "traceDone" && the long time && the params & cr & cr to stdOut
  
  disable btn "ignore" of stack "revErrorDisplay"
  disable btn "debug..." of stack "revErrorDisplay"
  
  if lBreakPointsRestore is not empty then -- come back from UI stack to normal debug
    -- can't think why this would ever come up here, but just in case:
    set the breakPoints to lBreakPointsRestore
    put empty into lBreakPointsRestore
  end if
  
  set the traceStack to empty
  set the tracemode of stack "revTemplateScriptEditor" to empty
  
  repeat for each line l in the openStacks
    if word 1 of l is "revScriptEditor" and the cREVDebugging of this cd of stack l is not empty then
      set the cCurrentSearch of this cd of stack l to the cRestoreSearch of this cd of stack l
      set the cREVDebugging of this cd of stack l to false
    end if
  end repeat
  
  send "revUpdatePalette" to card 1 of stack "revVariableWatcher"
  --set the title of stack "Message Box" to empty
   
  set the traceReturn to true
end traceDone

on trace h, l, p
  if gREVDevelopment then write "trace" && the long time && the params & cr & cr to stdOut
  global gREVShowStacks
  put revTargetStack(the long id of the target) into tTargetStack
  if not gREVDevelopment and (char 1 to 3 of tTargetStack is "rev" or tTargetStack is among the items of "Message Box,Home,answer dialog,ask dialog,script debugger,file selector") then 
    if gREVDevelopment then write "jump through rev stack messages" & cr to stdOut
    put the breakPoints into tBreakPoints
    put tBreakPoints into lBreakPointsRestore
    put line 2 of lReturnHandler,line 1 of lReturnHandler+1 into line (the number of lines in tBreakPoints+1) of tBreakPoints
    set the breakPoints to tBreakPoints
    set the traceStack to empty
    set the traceReturn to true
  else
    if lBreakPointsRestore is not empty then -- come back from UI stack to normal debug
      set the breakPoints to lBreakPointsRestore
      put empty into lBreakPointsRestore
    end if
    put revScriptEditor(the long id of the target) into tScriptEditor
    if tScriptEditor is empty then
      put true into tReturn
    end if
    switch the tracemode of stack "revTemplateScriptEditor"
    case "Step Over"
      if tReturn then 
        set the traceReturn to true
        exit trace
      end if
      if the cCurrentSearch of cd 1 of stack tScriptEditor is "debug" and h is in the cDebuggingHandler of cd 1 of stack tScriptEditor then traceBreak h, l, p
      else set the traceReturn to true
      break
    case "Trace"
      traceBreak h, l, p
      send "revTrace" to me in the traceDelay milliseconds
      break
    default
      if the cREVScriptDebugMode of stack "revPreferences" is false then exit trace
      traceBreak h, l, p
    end switch
  end if
end trace

on revTrace
  if gREVDevelopment then write "revTrace" && the long time && the params & cr & cr to stdOut
  if the traceStack is not empty then set the traceReturn to true
end revTrace

on updateVariable l, vname, vvalue
  if gREVDevelopment then write "updateVariable" && the long time && the params & cr & cr to stdOut
  if the mode of stack "revVariableWatcher" is 0 then palette "revVariableWatcher"
  revDebugScript l,the long id of the target, vName
end updateVariable

on revDebugScript pLine,pObject,pVarName
  if gREVDevelopment then write "revDebugScript" && the long time && the params & cr & cr to stdOut
  if the cREVScriptDebugMode of stack "revPreferences" is false then exit revDebugScript
  if pObject is not empty then put pObject into tObject
  else put the long id of the target into tObject
  edit script of tObject
  put (revScriptEditor(tObject)) into tEditorStack
  send "revUpdatePalette" to card 1 of stack "revVariableWatcher"
  --  put lineOffset(pVarName,fld "Names" of stack "revVariableWatcher") into tLineNo
  --  if pVarName is not empty and tLineNo is not 0 then
  --    set the hilitedLines of fld "names" of stack "revVariableWatcher" to tLineno
  --    set the hilitedLines of fld "Values" of stack "revVariableWatcher" to tLineNo
  --  end if
  if the cCurrentSearch of cd 1 of stack tEditorStack is not "debug" then
    set the cRestoreSearch of this cd of stack tEditorStack to the cCurrentSearch of this cd of stack tEditorStack
    set the cCurrentSearch of cd 1 of stack tEditorStack to "debug"
  end if
  set the cREVDebugging of cd 1 of stack tEditorStack to pLine
  set the cDebuggingHandler of cd 1 of stack tEditorStack to pHandler
end revDebugScript

on traceBreak pHandler, pLine
  if gREVDevelopment then write "traceBreak" && the long time && the params & cr & cr to stdOut
  if the cREVScriptDebugMode of stack "revPreferences" is false then pass traceBreak
  if "button id 1042" is not in the frontScripts then wait 20 milliseconds with messages -- message sent from context menu, frontScripts temporarily gone
  put the long id of the target into tObject
  edit script of tObject
  put (revScriptEditor(tObject)) into tEditorStack
  if the cCurrentSearch of cd 1 of stack tEditorStack is not "debug" then set the cRestoreSearch of this cd of stack tEditorStack to the cCurrentSearch of this cd of stack tEditorStack
  set the cCurrentSearch of cd 1 of stack tEditorStack to "debug"
  set the cREVDebugging of cd 1 of stack tEditorStack to pLine
  put pLine & cr & tObject into lReturnHandler -- use this in trace to set a breakpoint to pass UI stacks faster
  set the cDebuggingHandler of cd 1 of stack tEditorStack to pHandler
  send "revUpdatePalette" to card 1 of stack "revVariableWatcher"
end traceBreak

setProp cREVScriptDebugMode pWhat
  if gREVDevelopment then write "cREVScriptDebugMode" && the long time && the params & cr & cr to stdOut
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
    put true into tReturn
    if the mode of stack "revErrorDisplay" is not 0 then
      disable btn "ignore" of stack "revErrorDisplay"
      disable btn "debug..." of stack "revErrorDisplay"
    end if
  end if
  put the openStacks into tStacksList
  filter tStacksList with "revScriptEditor*"
  repeat for each line l in tStacksList
    if the cBreakPoints of cd 1 of stack l is not pWhat then set the cBreakPoints of cd 1 of stack l to pWhat
  end repeat
  if tReturn then 
    lock messages
    set the cREVScriptDebugMode of stack "revPreferences" to pWhat
    traceDone
  end if
  pass cREVScriptDebugMode
end cREVScriptDebugMode

on revDebuggerGrabValue pValue
  global gREVVariableWatcherValue
  put pValue into gREVVariableWatcherValue
end revDebuggerGrabValue
