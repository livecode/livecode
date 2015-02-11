script "revdebuggerlibrary"

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

# This constant specifies that the debugger should use the cREVGeneral
# custom property set to store its metadata.
constant kMetadataType = "general"

################################################################################
#
# Debugger Library Interface
#
################################################################################

# Description
#   Initializes the debugger, should be called on IDE startup or when script debug mode is turned on.
command revDebuggerInitialize
   # When the debugger is first initialized, the breakpoints property should be empty as other
   # IDE stacks should not be setting it. Unfortunately if there are no breakpoints set, traceError messages
   # are not sent by the engine, so we have to set a dummy breakpoint here to prevent this problem.
   if debuggerEnabled() then
      set the breakpoints to the long id of me,5000
   else
      set the breakpoints to empty
   end if
end revDebuggerInitialize

# Returns
#   Whether or not the debugger is enabled. This is currently defined as whether or not the cREVScriptDebugMode
#   of stack "revPreferences" is true.
function revDebuggerEnabled
   return debuggerEnabled()
end revDebuggerEnabled

# Description
#   Enables the debugger, should be called when the user switches script debug mode on.
command revDebuggerEnable
   # When the debugger is enabled we need to go through all the open stacks and 
   # activate their breakpoints if appropriate.
   local tOpenStacks
   put the openStacks into tOpenStacks
   repeat for each line tStack in tOpenStacks
      if not debuggerStackAllowed(the long id of stack tStack) then
         next repeat
      end if
      revDebuggerActivateBreakpoints the long id of stack tStack
      revDebuggerActivateWatches the long id of stack tStack
   end repeat
   
   revDebuggerActivateWatches empty
   
   set the cREVScriptDebugMode of stack "revPreferences" to true
   
   # OK-2008-08-07 : 6875 - The debugger must be re-initialized when script debug node is switched on.
   revDebuggerInitialize
end revDebuggerEnable

local sContext

# Description
#   Disables the debugger, should be called when the user switches script debug mode off.
command revDebuggerDisable
   set the breakpoints to empty
   set the watchedVariables to empty
   set the cREVScriptDebugMode of stack "revPreferences" to false
   put empty into sContext
end revDebuggerDisable

# Returns
#   The current debug context if there is one, empty otherwise. The context returned by this function
#    is persistent, in that if we are debugging, there is always a debug context. Unless the debug context
#    is set by using revDebuggerSetContext, it will be the context of the next line to execute in the debugger.
function revDebuggerGetContext
   # Check that we are definitely still debugging- the user may have manually turned off the debugger.
   if the traceStack is empty or not revDebuggerEnabled() then
      return empty
   end if
   
   # If we are debugging, but there is no context set, we use the next line to be executed,
   # which is the first line of revDebuggerContexts (hopefully).
   local tContext
   if sContext is empty then
      put line 1 of revDebuggerContexts() into tContext
   else
      put sContext into tContext
   end if
   
   return tContext
end revDebuggerGetContext
  
  
# Parameters
#   pContext : a debug context in the format returned by revDebuggerContexts. Note this includes a number as the first item.
# Description
#   Sets the debugger context to pContext. This value will only remain while the debugger remains on its current line
#   when the line changes (e.g. because the user has pressed "Step over") the context will be reset to the current one.
command revDebuggerSetContext pContext
   put pContext into sContext
end revDebuggerSetContext

# Returns
#   A list of execution contexts
# Description
#   This function returns a filtered list of execution contexts with IDE contexts removed.
#   The returned list is reversed so that it takes the order of a call stack. Whether the debugger is active
#   or not is defined as whether the revDebuggerContexts is empty.
function revDebuggerContexts
   local tContextsRaw, tContexts, tControl, tStack
   global gREVDevelopment
   
   put the executionContexts into tContextsRaw
   
   # tLineNumber contains a one-based index that links directly into MCexecutioncontexts in the engine
   # allowing us to deal with situations when multiple contexts have the same object and line number (recursion).
   local tLineNumber
   put 0 into tLineNumber
   repeat for each line tContext in tContextsRaw
      add 1 to tLineNumber
      
      # OK-2009-06-23 : Bug 7954 - Parse the execution contexts properly to avoid problems with commas
      local tArray
      put parseExecutionContext(tContext) into tArray
      
      if tArray["behavior"] is not empty then
         put tArray["behavior"] into tControl
      else
         put tArray["object"] into tControl
      end if
      
      --      if the number of items of tContext = 4 then
      --         put item 1 to -4 of tContext into tControl
      --      else
      --         put item 1 to -3 of tContext into tControl
      --      end if
      
      put revTargetStack(the long id of tControl) into tStack
      
      # If gREVDevelopment is false then only non IDE stacks can appear in the contexts
      if not gREVDEvelopment then
         if debuggerStackAllowed(the long id of stack tStack) then
            put return & tLineNumber & comma & tContext before tContexts
         else
            next repeat
         end if
      else
         # If in development mode, we allow debugging of any stack except the
         #  debugger itself and anything that was called by the debugger, i.e. appears after a debugger
         #  call in the call stack.
         
         if tStack is "revDebuggerLibrary" then
            # No debugger stack contexts are placed onto the call stack, however we also need to remove
            # *any* contexts that appear after a trace, traceError or traceBreak (or traceDone although less important).
            --if item -2 of tContext is among the words of "trace traceError traceBreak traceDone" then
            if tArray["handler"] is among the words of "trace traceError traceBreak traceDone" then
               exit repeat
            else
               next repeat
            end if
         else
            put return & tLineNumber & comma & tContext before tContexts
         end if
      end if
   end repeat
   delete the first char of tContexts
   return tContexts
end revDebuggerContexts
  
function revDebuggerParseExecutionContext pContext
   return  parseExecutionContext(pContext)
end revDebuggerParseExecutionContext

# OK-2009-06-23 : Bug 7954 - Parses an execution context taking into account that object long ids may contain commas.
private function parseExecutionContext pContext
   # If pContext was from the revDebuggerContexts it will have an extra "context number" at the start,
   # chop this off first.
   local tArray
   if item 1 of pContext is an integer then
      put item 1 of pContext into tArray["context number"]
      delete item 1 of pContext
   end if
   
   local tItemNumber
   put 0 into tItemNumber
   
   repeat for each item tItem in pContext
      add 1 to tItemNumber
      if tItem is an integer then
         put tItem into tArray["line number"]
         put item (tItemNumber - 1) of pContext into tArray["handler"]
         put item 1 to (tItemNumber - 2) of pContext into tArray["object"]
         put item (tItemNumber + 1 ) to -1 of pContext into tArray["behavior"]
         return tArray
      end if
   end repeat
end parseExecutionContext

function revDebuggerParseExecutionContext pContext
   return parseExecutionContext(pContext)
end revDebuggerParseExecutionContext
  

# Returns
#   A list of global variable names
# Description
#   Returns a filtered list of global names. Included for symmetry with other call to get variable names.
#   The list contains one global name per line.
function revDebuggerGlobalNames
   local tGlobalsRaw, tGlobals
   
   # OK-2009-01-29 : Bug 7178 - The 64 bit Vista environment variables ending with (x86) cause problems 
   # because Revolution throws an error when parsing do "global $VARIABLE(x86)" or similar. For now, we just 
   # remove these until we can test on a 64 bit Vista machine to get them working.
   put revDebuggerValidGlobalNames() into tGlobalsRaw
   replace comma with return in tGlobalsRaw
   
   global gREVShowStacks
   if gREVShowStacks is not true then
      filter tGlobalsRaw without "gREV*"
   end if
   
   sort lines of tGlobalsRaw ascending
   put tGlobalsRaw into tGlobals
   
   return tGlobals
end revDebuggerGlobalNames

# Returns
#   A list of the globals with valid names. I.e not the ones with (x86) in them on Vista 64bit.
function revDebuggerValidGlobalNames
   local tGlobalsRaw
   put the globals into tGlobalsRaw
   
   replace comma with return in tGlobalsRaw
   
   # For now we just filter out the Vista 64 bit ones. Really we should remove anything that is not a valid Rev identifier
   # according to lextable.cpp, but that can easily be added in later as this is the only place in the IDE this is done.
   filter tGlobalsRaw without "*(x86)"
   filter tGlobalsRaw without "*/*"
   filter tGlobalsRaw without "*-*"
   
   replace return with comma in tGlobalsRaw
   
   return tGlobalsRaw
end revDebuggerValidGlobalNames

# Parameters
#   pContext : a debug context in the format returned by revDebuggerContexts (<context number>,<object>,<handler>,<line>
# Returns
#   A list of local variable names in a given debug context. The list contains three lines, each line is a list of
#   variable names separated by commas. Line 1 contains the parameter passed to the current handler, line 2 contains
#   the local variables declared in the current handler up to the current execution point. Line 3 contains the currently
#   declared script local variables. If pContext is empty, attempts to use the current debugger context.
function revDebuggerLocalNames pContext
   local tLocalsRaw, tLocalsCommand, tLocals, tNames
   put "return the localNames" into tLocalsCommand
   
   local tContextNumber
   if pContext is empty then
      put item 1 of revDebuggerGetContext() into tContextNumber
   else
      put item 1 of pContext into tContextNumber
   end if
   
   set the debugContext to tContextNumber
   debugDo tLocalsCommand
   put the result into tLocalsRaw
   set the debugContext to empty
   
   -- Line 1 is the parameters passed to the current handler/function
   -- Line 2 is the locals declared in the current handler/function
   -- line 3 is the script locals
   -- For now we just combine and sort.
   repeat for each line tLine in tLocalsRaw
      if tLine is not empty then
         put tLine into tNames
         replace comma with return in tNames
         put tNames & return after tLocals
      end if
   end repeat
   delete the last char of tLocals
   
   sort lines of tLocals ascending
   return tLocals
end revDebuggerLocalNames

# Parameters
#   pContext : a debug context in the format returned by revDebuggerContexts (<context number>,<object>,<handler>,<line>
# Returns
#   A list of variable names in the specified context. The return value is in the same format as the variableNames property. If no context is specified
#   the current debugger context is used.
function revDebuggerVariableNames pContext
   local tCommand
   
   put "return the variableNames" into tCommand
   
   local tContextNumber
   if pContext is empty then
      put item 1 of revDebuggerGetContext() into tContextNumber
   else
      put item 1 of pContext into tContextNumber
   end if
   
   set the debugContext to tContextNumber
   
   local tVariablesRaw
   debugDo tCommand
   put the result into tVariablesRaw
   set the debugContext to empty
   
   # For now, return with no processing
   return tVariablesRaw
end revDebuggerVariableNames

local sDebuggerElementList

function revDebuggerGetElementList
   return sDebuggerElementList
end revDebuggerGetElementList


# Parameters
#   pVariable : the name of a variable
#   pContext : the debug context to get the variable's value at. Must be one of the current revDebuggerContexts.
#   pElementList : a numeric array of keys if an array element is required. If empty, it is assumed that the whole variable is required.
# Returns
#   The value of the variable. If pContext is not specified, uses sContext
function revDebuggerGetValue pVariable, pContext, pElementList, pIsGlobal
   local tValue, tCommand
   
   local tContextNumber
   if pContext is empty then
      put item 1 of revDebuggerGetContext() into tContextNumber
   else
      put item 1 of pContext into tContextNumber
   end if
   
   local tName
   set the itemDelimiter to "["
   put item 1 of pVariable into tName
   set the itemDelimiter to comma
   
   --   local tIsGlobal
   --   if pIsGlobal is empty then
   --      put (tName is among the items of the globalNames) into tIsGlobal
   --   else
   --      put pIsGlobal into tIsGlobal
   --   end if
   
   # OK-2009-06-22 : Bug 8109 - Don't try to detect if the variable is a global. This is flawed anyway
   # because it might have the same name as a global but not actually be declared as one. It is assumed that the 
   # variable is non-global unless pIsGlobal is specified as true.  If the debugger is not actually active, then there
   # will be no debugger contexts, in this case we assume its a global as no other variables could be displayed anyway.
   if pIsGlobal or the number of lines of revDebuggerContexts() = 0 then
      put "global " & tName & "; return " & pVariable into tCommand
   else
      put "return " & pVariable into tCommand
   end if
   
   if pElementList is an array then
      put pElementList into sDebuggerElementList
      put "[revDebuggerGetElementList()]"after tCommand
   end if
   
   set the debugContext to tContextNumber
   debugDo tCommand
   put the result into tValue
   set the debugContext to empty
   
   return tValue
end revDebuggerGetValue


# Parameters
#   pName : the name of an array variable
#   pContext : the debug context to get the variable's keys at. Must be one of the curent revDebuggerContexts.
# Returns
#   A sorted list of keys for the given variable. If the extents of the array are non-empty then the list is sorted
#   numerically, otherwise it is sorted alphebetically. If pContext is empty then uses sContext.
function revDebuggerGetKeys pName, pContext
   local tKeys, tCommand, tExtents, tExtentsCommand
   
   local tContextNumber
   if pContext is empty then
      put item 1 of revDebuggerGetContext() into tContextNumber
   else
      put item 1 of pContext into tContextNumber
   end if
   
   if pName is among the items of the globalNames then
      put "global " & pName & "; return the keys of " & pName into tCommand
   else
      put "return the keys of " & pName into tCommand
   end if
   
   put "return the extents of " & pName into tExtentsCommand
   
   set the debugContext to tContextNumber
   debugDo tCommand
   put the result into tKeys
   debugDo tExtentsCommand
   put the result into tExtents
   set the debugContext to empty
   
   if tExtents is empty then
      sort tKeys
   else
      sort tKeys numeric
   end if
   
   return tKeys
end revDebuggerGetKeys

# Parameters
#   pCommand : a chunk of Revolution code to execute
#   pContext : the debug context to execute in, must be in the form returned by revDebuggerContexts
# Description
#   Executes the specified code in the debug context pContext,
#   returns the result on line 1 and any error that may have ocurred on line 2.
#   If pContext is empty, uses sContext.
command revDebuggerDo pCommand, pContext
   local tContextNumber
   if pContext is empty then
      put item 1 of revDebuggerGetContext() into tContextNumber
   else
      put item 1 of pContext into tContextNumber
   end if
   
   local tResult
   set the debugContext to tContextNumber
   try
      debugDo pCommand
      put the result into tResult
   catch tError
      return tError
   end try
   set the debugContext to empty
   
   return tResult & return & tError
end revDebuggerDo

local sDebugElements, sDebugValue

# Parameters
#   pVariableName : the name of the variable to set
#   pElementList : a numeric array of keys, or empty if pVariableName is not an array
#   pNewValue : the new value to set
#   pContext : the debug context, if empty, uses sContext
# Description
#   Sets the specified value in the specified debug context
command revDebuggerSetValue pVariableName, pElementList, @pNewValue, pContext
   
   
   --   if pVariableName is among the items of the globalNames then
   --      put "global " & the globalNames & return into tStatement
   --   end if
   
   put pElementList into sDebugElements
   put pNewValue into sDebugValue
   
--   local tStatement 
--   put "put pNewValue into " & pVariableName after tStatement 
--   repeat with x = 1 to item 2 of line 1 of extents(pElementList) 
--      put "[pElementList[" & x & "]]" after tStatement 
   --   end repeat
   
   # In order not to interfere with the code being debugged, we change the value by passing a reference
   # to the variable to be changed back to the debugger. This prevents the creation of any variables in the
   # current debug context.
   local tStatement
   put "revDebuggerSetValueDo " & pVariableName into tStatement
   
   local tContextNumber
   if pContext is empty then
      put item 1 of revDebuggerGetContext() into tContextNumber
   else
      put item 1 of pContext into tContextNumber
   end if
   
   local tResult
   set the debugContext to tContextNumber
   try
      debugDo tStatement
      put the result into tResult
   catch tError
      return tError
   end try
   
   delete variable sDebugValue
   delete variable sDebugElements
   
   return tResult
end revDebuggerSetValue

command revDebuggerSetValueDo @pVariable
   local tStatement 
   put "put sDebugValue into pVariable" into tStatement 
   repeat with x = 1 to item 2 of line 1 of extents(sDebugElements) 
      put "[sDebugElements[" & x & "]]" after tStatement 
   end repeat
   
   do tStatement
end revDebuggerSetValueDo

# Description
#   Runs the currently debugging script until the next breakpoint
command revDebuggerRun
   set the traceStack to empty
   set the traceUntil to 65535
   set the traceReturn to true
   revDebuggerSetContext empty
end revDebuggerRun

# Description
#  Performs the debugger "stop" function, terminates the script currently being debugged
command revDebuggerStop
   # When executing certain structures, the debugger will continue to loop until the
   # breakpoints property becomes empty. In order to make sure the debugger always
   # exits when asked to, we set the breakpoints to empty here and restore them again
   # once the handler is finished.
   local tBreakpoints
   put the breakpoints into tBreakpoints
   set the breakpoints to empty
   
   set the traceStack to empty
   set the traceUntil to 65535
   set the traceAbort to true
   set the traceReturn to true
   
   // [[ Bug 9940 ]]  Close printing when execution is stopped manually in the debugger.
   close printing
   
   revDebuggerSetContext empty
   send "revDebuggerSetBreakpoints tBreakpoints" to me in 0 milliseconds
end revDebuggerStop

# Parameters
#   pBreakpoints : the value to set the breakpoints property to.
# Description
#   Sets the breakpoints property to the specified value. Please do not call this command
#   from outside the debugger. It has to be made public due to how the debugger works in the engine.
command revDebuggerSetBreakpoints pBreakpoints
   set the breakpoints to pBreakpoints
end revDebuggerSetBreakpoints

# Stores the last trace mode chosen by the user. This is either "Step Over", "Step Into" or "Step Out"
local sLastTraceMode

# Parameters
#   pObject : reference to the object to perform the step into on.
# Description
#   Performs the step into debugger action. If the next line to execute cannot be stepped into,
#   e.g. because its an engine command, does the same as step over.
command revDebuggerStepInto pObject
   put "Step Into" into sLastTraceMode
   set the traceStack to revTargetStack(pObject)
   set the traceUntil to 65535
   set the traceReturn to true
   revDebuggerSetContext empty
end revDebuggerStepInto

# Parameters
#   pObject : reference to the object to perform the step over on.
# Description
#   Performs the step over debugger action.
command revDebuggerStepOver pObject
   put "Step Over" into sLastTraceMode
   set the traceStack to revTargetStack(pObject)
   set the traceReturn to true
   set the traceUntil to 65535
   revDebuggerSetContext empty
end revDebuggerStepOver

# Parameters
#   pObject : reference to the object to perform the step out on.
# Description
#   Performs the step out debugger action
command revDebuggerStepOut pObject
   put "Step Out" into sLastTraceMode
   set the traceStack to revTargetStack(pObject)
   set the traceReturn to true
   
   local tCurrentContexts
   put revDebuggerContexts() into tCurrentContexts
   
   local tNextContext
   put item 1 of line 1 of tCurrentContexts - 2 into tNextContext
   
   set the traceUntil to tNextContext
   revDebuggerSetContext empty
end revDebuggerStepOut

# The maximum number of breakpoints that can be active at any time. The debugger library
# will not allow more to be activated than this.
constant kActiveBreakpointLimit = 100

# Max number of breakpoints that may be added to any object.
constant kObjectBreakpointLimit = 100

# Parameters
#   pObject : reference to the object whose script the breakpoint should be put in
#   pLine : the line number that the breakpoint should be placed on
# Description
#   Sets a breakpoint on the specified line and object. If the maximum number of
#   breakpoints has already been reached, does nothing and returns an error string.
#   Note that the breakpoint must be explicitly activation using the revDebuggerActivateBreakpoint
#   command before it will actually trigger the debugger to break.
# Returns
#   Empty under normal circumstances. If there are already the maximum number
#   of breakpoints set then returns an error string.
command revDebuggerAddBreakpoint pObject, pLine
   debuggerAddBreakpoint pObject, pLine, empty
end revDebuggerAddBreakpoint

# Parameters
#   pObject : reference to the object whose script the breakpoint should be put in
#   pLine : the line number that the breakpoint should be placed on
#   pCondition : a Revolution expression evaluating to true or false.
# Description
#   Sets a conditional breakpoint on the specified line and object. If the maximum number of
#   breakpoints has already been reached, does nothing and returns an error string.
#   Note that the breakpoint must be explicitly activation using the revDebuggerActivateBreakpoint
#   command before it will actually trigger the debugger to break. When creating a conditional
#   breakpoint, a breakpoint is set on the specified line as normal, but the condition is saved
#   as metadata along with the breakpoint. When the break is reached, the debugger library
#   evaluates the condition and only breaks if it evaluates to true. The condition is evaluated in
#   the context of the line that the breakpoint is on, e.g. all variables will have whatever values
#   they had at that point of execution. If pCondition is empty a normal breakpoint is set.
# Returns
#   Empty under normal circumstances. If there are already the maximum number
#   of breakpoints set then returns an error string.
command revDebuggerAddConditionalBreakpoint pObject, pLine, pCondition
   debuggerAddBreakpoint pObject, pLine, pCondition
end revDebuggerAddConditionalBreakpoint

# Parameters
#   pObject : reference to the object whose script the breakpoint was in
#   pLine : the line number of the breakpoint to remove
# Description
#   Removes the specified breakpoint if it exists. Note that this also deactivates it.
# Returns
#   Empty under normal circumstances. If the specified breakpoint cannot be found,
#   returns an error string but does not throw an error. The reason for this is that
#   breakpoints are stored as stack metadata, which in theory could be changed from
#   outside this script (although shouldn't).
command revDebuggerRemoveBreakpoint pObject, pLine
   # Deactivate the breakpoint then remove it from the object
   revDebuggerDeactivateBreakpoint pObject, pLine
   
   local tStack
   put debuggerTargetStack(pObject) into tStack
   
   local tBreakpoint
   put debuggerGetId(pObject) & comma & pLine into tBreakpoint
   
   local tObjectBreakpoints
   put revMetadataGet(tStack, kMetadataType, "breakpoints") into tObjectBreakpoints
   
   local tStates
   put revMetadataGet(tStack, kMetadataType, "breakpointstates") into tStates
   
   local tConditions
   put revMetadataGet(tStack, kMetadataType, "breakpointconditions") into tConditions
   
   local tLineNumber
   put debuggerBreakpointOffset(tBreakpoint, tObjectBreakpoints) into tLineNumber
   
   if tLineNumber = 0 then
      return "Breakpoint not found"
   end if
   delete line tLineNumber of tObjectBreakpoints
   delete line tLineNumber of tStates
   delete line tLineNumber of tConditions
   
   revMetadataSet tStack, kMetadataType, "breakpoints", tObjectBreakpoints
   revMetadataSet tStack, kMetadataType, "breakpointstates", tStates
   revMetadataSet tStack, kMetadataType, "breakpointconditions", tConditions
   
   debuggerCleanBreakpoints tStack
end revDebuggerRemoveBreakpoint

# Parameters
#   pOldObject : the currently assigned object of the breakpoint
#   pOldLine : the currently assigned line of the breakpoint
#   pNewObject : the new object to move the breakpoint to
#   pNewLine : the new line to move the breakpoint to
# Description
#   Moves the specified breakpoint to the specified location. This is provided as
#   a convenience and efficiency helper to the script editor.
command revDebuggerMoveBreakpoint pOldObject, pOldLine, pNewObject, pNewLine
   if pOldLine = pNewLine and the long id of pOldObject is the long id of pNewObject then
      exit revDebuggerMoveBreakpoint
   end if
   
   if pNewLine < 1 then
      return "invalid line specified: " pNewLine
   end if
   
   # OK-2008-05-01 : Don't enforce this because the script editor will want to move the
   # breakpoint before applying the script when the user is editing it.
   #   if pNewLine > the number of lines of the script of pNewObject then
   #      return "invalid line specified: " & pNewLine
   #   end if
   
   revDebuggerSuspendBreakpoints pOldObject
   
   local tStack
   put debuggerTargetStack(pOldObject) into tStack
   
   local tBreakpoints
   put revMetadataGet(tStack, kMetadataType, "breakpoints") into tBreakpoints
   
   local tBreakpoint
   put debuggerGetId(pOldObject) & comma & pOldLine into tBreakpoint
   
   local tLineNumber
   put debuggerBreakpointOffset(tBreakpoint, tBreakpoints) into tLineNumber
   if tLineNumber = 0 then
      exit revDebuggerMoveBreakpoint
   end if
   
   local tChanged
   put false into tChanged
   put debuggerGetId(pNewObject) & comma & pNewLine into line tLineNumber of tBreakpoints
   
   revMetadataSet tStack, kMetadataType, "breakpoints", tBreakpoints
   revDebuggerRestoreBreakpoints pOldObject
end revDebuggerMoveBreakpoint

# Description
#   Removes breakpoints from all open stacks. If gREVDevelopment is true this includes IDE stacks.
command revDebuggerClearAllBreakpoints
   global gREVDevelopment
   local tMainstacks
   if not gREVDevelopment then
      put revFilterStacksList(the mainstacks) into tMainstacks
   else
      put the mainstacks into tMainstacks
   end if
   
   local tStacks
   repeat for each line tMainstack in tMainstacks
      put the long id of stack tMainstack & return after tStacks
      repeat for each line tSubstack in the substacks of stack tMainstack
         put the long id of stack tSubstack of stack tMainstack & return after tStacks
      end repeat
   end repeat
   delete the last char of tStacks
   
      repeat for each line tStack in tStacks
         debuggerClearStackBreakpoints tStack
      end repeat
end revDebuggerClearAllBreakpoints

# Parameters
#   pObject : reference to the object to remove breakpoints from
# Description
#   Removes all breakpoints from the stack that owns pObject, this means all breakpoints set on the 
#   and any of its cards / objects are removed and deactivated.
private command debuggerClearStackBreakpoints pObject
   local tStack
   put debuggerTargetStack(pObject) into tStack
   
   local tStackBreakpoints
   put revMetadataGet(tStack, kMetadataType, "breakpoints") into tStackBreakpoints
   
   repeat for each line tBreakpoint in tStackBreakpoints
      # Because the old debugger also used the breakpoints metadata property, we can't assume when
      # clearing all breakpoints that that values in the metadata will be valid, hence the try.
      try
         local tObjectId
         put debuggerResolveId(item 1 of tBreakpoint, tStack) into tObjectId
         revDebuggerRemoveBreakpoint tObjectId, item 2 of tBreakpoint
      end try
   end repeat
end debuggerClearStackBreakpoints



# Parameters
#   pObject : reference to the object to deactivate breakpoints for
# Description
#   Suspends all breakpoints for pObject. This removes them from the breakpoints
#   property but does not actually mark them as deactivated. Breakpoints are only
#   suspending temporarily, e.g. next time rev restarts, they will be restored.
command revDebuggerSuspendBreakpoints pObject
   local tBreakpoints
   put the breakpoints into tBreakpoints
   
   local tNewBreakpoints
   repeat for each line tBreakpoint in tBreakpoints
      if item 1 to -2 of tBreakpoint is the long id of pObject then
         next repeat
      end if
      put tBreakpoint & return after tNewBreakpoints
   end repeat
   delete the last char of tNewBreakpoints
   set the breakpoints to tNewBreakpoints
end revDebuggerSuspendBreakpoints

# Parameters
#   pObject : reference to the object to deactivate breakpoints for
# Description
#   Restores the correct state of all breakpoints for the specified object.
#   i.e. activates them if their state is "active"
command revDebuggerRestoreBreakpoints pObject
   revDebuggerActivateBreakpoints debuggerTargetStack(pObject), pObject
end revDebuggerRestoreBreakpoints

constant kMaxActiveWatches = 100

# Parameters
#   pObject : reference to the object owning the watch
#   pHandler : the name of the handler owning the watch
#   pVariable : the name of the varaiable being watched
# Description
#   Activates the specified watch. I.e by putting it into the watchedVariables
command revDebuggerActivateWatch pObject, pHandler, pVariable
   local tStoredWatches
   put debuggerGetWatchInformation(pObject, "watches") into tStoredWatches
   
   local tWatch
   put debuggerBuildWatchLineFromDetails(pObject, pHandler, pVariable) into tWatch
   
   local tLineNumber
   put debuggerWatchOffset(tWatch, tStoredWatches) into tLineNumber
   if tLineNumber = 0 then
      return "watch_not_found"
   end if
   
   local tCondition
   put item 4 to -1 of line tLineNumber of tStoredWatches into tCondition
   
   # Activate the watch
   local tCurrentWatches
   put the watchedVariables into tCurrentWatches
   if the number of lines of tCurrentWatches >= kMaxActiveWatches then
      return "Active watch limit reached"
   end if
   
   local tWatchString
   if pObject is empty then
      put empty & comma & empty & comma & pVariable & comma & tCondition into tWatchString
   else
      put debuggerResolveId(item 1 of tWatch, debuggerTargetStack(pObject)) & comma & pHandler & comma & pVariable & comma & tCondition into tWatchString
   end if
   
   if tCurrentWatches is empty then
      set the watchedVariables to tWatchString
   else
      if tWatchString is not among the lines of tCurrentWatches then
         set the watchedVariables to tCurrentWatches & return & tWatchString
      end if
   end if
   
   # Store the breakpoint's status in the object so it is remembered between restarts etc
   debuggerUpdateWatchStatus pObject, tLineNumber, "active"
end revDebuggerActivateWatch

command revDebuggerActivateWatches pStack
   local tWatches
   put debuggerGetWatchInformation(pStack, "watches") into tWatches
   
   local tWatchStates
   put debuggerGetWatchInformation(pStack, "watchstates") into tWatchStates
   
   local tLineNumber
   put 1 into tLineNumber
   repeat for each line tWatchState in tWatchStates
      local tCurrentWatches
      put the watchedVariables into tCurrentWatches
      if the number of lines of tCurrentWatches >= kMaxActiveWatches then
         return "Active watch limit reached"
      end if
      
      if tWatchState is not "active" then
         add 1 to tLineNumber
         next repeat
      end if
      
      local tWatch
      put line tLineNumber of tWatches into tWatch
      
      local tWatchString
      if item 1 of tWatch is empty then
         put tWatch into tWatchString
      else
         put debuggerResolveId(item 1 of tWatch, pStack) & comma & item 2 to -1 of tWatch into tWatchString
      end if
      
      if tCurrentWatches is empty then
         set the watchedVariables to tWatchString
      else
         if tWatchString is not among the lines of tCurrentWatches then
            set the watchedVariables to tCurrentWatches & return & tWatchString
         end if
      end if
      
      add 1 to tLineNumber
   end repeat
end revDebuggerActivateWatches

# Parameters
#   pStack : reference to the stack to activate breakpoints for
#   pObject : reference to an object on the stack
# Description
#   Activates breakpoints stored on pStack. If pObject is empty then all breakpoints
#   are activated, otherwise only those breakpoints that are in the script of pObject 
#   are activated.
command revDebuggerActivateBreakpoints pStack, pObject
   local tStack
   if pObject is not empty then
      put debuggerTargetStack(pObject) into tStack
      if the long id of tStack is not the long id of pStack then
         return "invalid object"
      end if
   end if
   
   local tLineNumber
   put 1 into tLineNumber
   
   local tStackBreakpoints
   put revMetadataGet(pStack, kMetadataType, "breakpoints") into tStackBreakpoints
   
   local tStates
   put revMetadataGet(pStack, kMetadataType, "breakpointstates") into tStates
   
   local tBreakpointsToActivate
   repeat for each line tBreakpoint in tStackBreakpoints
      if pObject is empty or item 1 of tBreakpoint is debuggerGetId(pObject) then
         local tState
         put line tLineNumber of tStates into tState
         
         if tState is "active" then
            put debuggerResolveId(item 1 of tBreakpoint, pStack) & comma & item 2 of tBreakpoint & return after tBreakpointsToActivate
         end if
      end if
   end repeat
   delete the last char of tBreakpointsToActivate
   
   # If the breakpoints property is allowed to contain duplicates,  then the lines with duplicate breakpoints on
   # will break more than once, i.e. multiple traceBreak messages. We should ensure that this doesn't happen,
   # as there is no good reason for it and it will just cause annoyance.
   local tBreakpointsNotActivated
   repeat for each line tBreakpoint in tBreakpointsToActivate
      if tBreakpoint is among the lines of the breakpoints then
         next repeat
      end if
      put tBreakpoint & return after tBreakpointsNotActivated
   end repeat
   delete the last char of tBreakpointsNotActivated
   put tBreakpointsNotActivated into tBreakpointsToActivate
   
   if the breakpoints is empty then
      set the breakpoints to tBreakpointsToActivate
   else
      set the breakpoints to the breakpoints & return & tBreakpointsToActivate
   end if
end revDebuggerActivateBreakpoints

# Parameters
#   pObject : reference to the object owning the breakpoint
#   pLine : the line number that the breakpoint is on
# Description
#   Activates the specified breakpoint, if possible. This command might fail if too
#   many breakpoints have been activated, so check the result.
# Returns
#   Empty if successful, an error string otherwise.
command revDebuggerActivateBreakpoint pObject, pLine
   local tBreakpoint
   put debuggerGetId(pObject) & comma & pLine into tBreakpoint
   
   local tStack
   put debuggerTargetStack(pObject) into tStack
   
   local tObjectBreakpoints
   put revMetadataGet(tStack, kMetadataType, "breakpoints") into tObjectBreakpoints
   
   local tLineNumber
   put debuggerBreakpointOffset(tBreakpoint, tObjectBreakpoints) into tLineNumber
   
   if tLineNumber = 0 then
      # This is a bug in the code that called this command
      return "breakpoint_not_found"
   end if
   
   # Activate the breakpoint
   local tCurrentBreakpoints
   put the breakpoints into tCurrentBreakpoints
   if the number of lines of tCurrentBreakpoints >= kActiveBreakpointLimit then
      return "Active breakpoint limit reached"
   end if
   
   local tBreakpointString
   put debuggerResolveId(item 1 of tBreakpoint, tStack) & comma & pLine into tBreakpointString
   
   if tCurrentBreakpoints is empty then
      set the breakpoints to tBreakpointString
   else
      if tBreakpointString is not among the lines of tCurrentBreakpoints then
         set the breakpoints to tCurrentBreakpoints & return & tBreakpointString
      end if
   end if
   
   # Store the breakpoint's status in the object so it is remembered between restarts etc
   debuggerUpdateBreakpointStatus tStack, tLineNumber, "active"
end revDebuggerActivateBreakpoint

# Parameters
#   pObject : reference to the object owning the breakpoint
#   pLine : the line number that the breakpiont is on
#   pCondition : the condition to evaluate
# Description
#   Sets the specified condition on the breakpoint. Set the condition to empty to remove it.
command revDebuggerSetBreakpointCondition pObject, pLine, pCondition
   local tBreakpoint
   put debuggerGetId(pObject) & comma & pLine into tBreakpoint
   
   local tStack
   put debuggerTargetStack(pObject) into tStack
   
   local tObjectBreakpoints
   put revMetadataGet(tStack, kMetadataType, "breakpoints") into tObjectBreakpoints
   
   local tLineNumber
   put debuggerBreakpointOffset(tBreakpoint, tObjectBreakpoints) into tLineNumber
   
   debuggerUpdateBreakpointCondition tStack, tLineNumber, pCondition
end revDebuggerSetBreakpointCondition

# Parameters
#   pObject : reference to the object owning the breakpoint
#   pLine : the line number that the breakpoint is on
# Description
#   Deactivates the specified breakpoint.
command revDebuggerDeactivateBreakpoint pObject, pLine
   # Ensure that the breakpoint actually exists
   local tBreakpoint
   put debuggerGetId(pObject) & comma & pLine into tBreakpoint
   
   local tStack
   put debuggerTargetStack(pObject) into tStack
   
   local tObjectBreakpoints
   put revMetadataGet(tStack, kMetadataType, "breakpoints") into tObjectBreakpoints
   
   local tStackLineNumber
   put debuggerBreakpointOffset(tBreakpoint, tObjectBreakpoints) into tStackLineNumber
   
   if tStackLineNumber = 0 then
      # This is a bug in the code that called this command. Don't throw errors in the debugger library
      # if possible though as it could make the IDE unstable.
      return "breakpoint not found"
   end if
   
   # Deactivate the breakpoint
   local tCurrentBreakpoints
   put the breakpoints into tCurrentBreakpoints
   
   local tBreakpointString
   put debuggerResolveId(item 1 of tBreakpoint, tStack) & comma & pLine into tBreakpointString
   
   local tLineNumber
   put debuggerBreakpointOffset(tBreakpointString, tCurrentBreakpoints) into tLineNumber
   delete line tLineNumber of tCurrentBreakpoints
   set the breakpoints to tCurrentBreakpoints
   
   # Update the breakpoint's status
   # Store the breakpoint's status in the object so it is remember between restarts etc
   debuggerUpdateBreakpointStatus tStack, tStackLineNumber, "inactive"
end revDebuggerDeactivateBreakpoint

# Parameters
#   pObject : reference to the object to list breakpoints for.
# Description
#   Returns a list of the breakpoints set for pObject. This includes both active
#   inactive and conditional breakpoints. The breakpoints list is returned in the same
#   format as the engine "breakpoints" property.
function revDebuggerListBreakpoints pObject
   local tStack
   put debuggerTargetStack(pObject) into tStack
   
   debuggerCleanBreakpoints tStack
   
   local tStackBreakpoints
   put revMetadataGet(tStack, kMetadataType, "breakpoints") into tStackBreakpoints
   
   local tBreakpointsList
   repeat for each line tBreakpoint in tStackBreakpoints
      if debuggerGetId(pObject) = item 1 of tBreakpoint then
         local tBreakpointString
         put debuggerResolveId(item 1 of tBreakpoint, tStack) & comma & item 2 of tBreakpoint into tBreakpointString
         put tBreakpointString & return after tBreakpointsList
      end if
   end repeat
   delete the last char of tBreakpointsList
   
   return tBreakpointsList
end revDebuggerListBreakpoints

# Returns
#   A list of all active breakpoints
function revDebuggerListActiveBreakpoints
   global gREVDevelopment
   
   # With gREVDevelopment we return all breakpoints exactly as they are
   if gREVDevelopment then
      return the breakpoints
   end if
   
   # Otherwise we filter out IDE stacks from the breakpoints list.
   local tBreakpoints
   put the breakpoints into tBreakpoints
   
   local tFilteredBreakpoints
   repeat for each line tBreakpoint in tBreakpoints
      local tObject
      put item 1 to -2 of tBreakpoint into tObject
      
      local tStack
      put revTargetStack(revRuggedId(tObject)) into tStack
      
      if revIDEStack(the effective filename of stack tStack) then
         next repeat
      end if
      
      put tBreakpoint & return after tFilteredBreakpoints
   end repeat
   delete the last char of tFilteredBreakpoints
   
   return tFilteredBreakpoints
end revDebuggerListActiveBreakpoints

# Returns
#   A list of all active watches
function revDebuggerListActiveWatches
   return the watchedVariables
end revDebuggerListActiveWatches

function revDebuggerParseWatch pWatch
   local tItemNumber
   put 0 into tItemNumber
   
   # If item 1 is empty, it means no object is included (global watch)
   # In this case, the parsing is straightforward
   local tArray
   if item 1 of pWatch is empty then
      put empty into tArray["object"]
      put item 2 of pWatch into tArray["handler"]
      put item 3 of pWatch into tArray["variable"]
      put item 4 to -1 of pWatch into tArray["condition"]
      return tArray
   end if
   
   # Otherwise the object will be a long id. So we go through the items until we find one that
   # is either 0 or 1 words. This will be the handler name. Once we've found that the rest is easy.
   repeat for each item tItem in pWatch
      add 1 to tItemNumber
      if the number of words of tItem = 1 or the number of words of tItem = 0 then
         # We've found the handler name (allowing it to be empty for future support of script locals / globals)
         put tItem into tArray["handler"]
         put item 1 to (tItemNumber - 1) of pWatch into tArray["object"]
         put item (tItemNumber + 1) of pWatch into tArray["variable"]
         put item (tItemNumber + 2) to - 1 of pWatch into tArray["condition"]
         return tArray
      end if
   end repeat
end revDebuggerParseWatch

# Parameters
#   pObject : reference to the object whose script the watch should be in
#   pHandler : the name of the handler that the watch should be placed in
#   pVariable : the name of the variable to watch
#   pCondition : a Revolution expression to evaluate on hitting the watch. Pass empty for no condition.
# Description
#   Registers a watch with the specified parameters. Does not activate the watch.
command revDebuggerAddWatch pObject, pHandler, pVariable, pCondition
   local tWatches
   put debuggerGetWatchInformation(pObject, "watches") into tWatches
   if the number of lines of tWatches >= kMaxActiveWatches then
      # Return an error here as the other option (removal of other watches
      # to make room) could potentially confuse the user.
      return "Maximum number of watches reached"
   end if
   
   local tWatch
   put debuggerBuildWatchLineFromDetails(pObject, pHandler, pVariable, pCondition, true) into tWatch
   if tWatches is empty then
      put tWatch into tWatches
   else
      put tWatches & return & tWatch into tWatches
   end if
   debuggerSetWatchInformation pObject, "watches", tWatches
   
   # New watches are initially inactive.
   local tStates
   put debuggerGetWatchInformation(pObject, "watchstates") into tStates
   put "inactive" into line (the number of lines of tWatches) of tStates
   debuggerSetWatchInformation pObject, "watchstates", tStates
   
   debuggerCleanWatches pObject
end revDebuggerAddWatch

# Parameters
#   pObject : reference to the object owning the watch
#   pHandler : the name of the handler owning the watch
#   pVariable : the name of the variable being watched
# Description
#   Removes the specified watch, deactivating it first.
command revDebuggerRemoveWatch pObject, pHandler, pVariable
   # Deactivate the watch then remove it from the object
   revDebuggerDeactivateWatch pObject, pHandler, pVariable
   
   local tWatch
   put debuggerBuildWatchLineFromDetails(pObject, pHandler, pVariable) into tWatch
   
   local tStoredWatches
   put debuggerGetWatchInformation(pObject, "watches") into tStoredWatches
   
   local tStates
   put debuggerGetWatchInformation(pObject, "watchstates") into tStates
   
   local tLineNumber
   put debuggerWatchOffset(tWatch, tStoredWatches) into tLineNumber
   if tLineNumber = 0 then
      return "Watch not found"
   end if
   delete line tLineNumber of tStoredWatches
   delete line tLineNumber of tStates
   
   debuggerSetWatchInformation pObject, "watches", tStoredWatches
   debuggerSetWatchInformation pObject, "watchstates", tStates
   debuggerCleanWatches pObject
end revDebuggerRemoveWatch

private function debuggerBuildWatchLineFromDetails pObject, pHandler, pVariable, pCondition, pIncludeTrailingCommaIfConditionEmpty
   local tLine
   if pObject is empty then
      put empty & comma & empty & comma & pVariable into tLine
   else
      put debuggerGetId(pObject) & comma & pHandler & comma & pVariable into tLine
   end if
   
   if pIncludeTrailingCommaIfConditionEmpty or pCondition is not empty then
      put comma after tLine
   end if
   
   if pCondition is not empty then
      put pCondition after tLine
   end if
   
   return tLine
end debuggerBuildWatchLineFromDetails

private function debuggerGetWatchDataStoreForObject pObject
   if pObject is empty then
      return the long id of stack "revPreferences"
   else
      return debuggerTargetStack(pObject)
   end if
end debuggerGetWatchDataStoreForObject

private function debuggerGetWatchDataStoreTypeForObject pObject
   if pObject is empty then
      return "debugger"
   else
      return kMetadataType
   end if
end debuggerGetWatchDataStoreTypeForObject

private command debuggerSetWatchInformation pObject, pInformationType, pValue
   # Watch information (watchedVariables) can be found in two possible locations,
   # if pObject is given, then the stack owning pObject will contain the metadata in its
   # cREVGeneral custom property set. This is for watchedVariables at handler or script local scope.
   # For global variables, pObject is empty, and the information is stored in the preferences stack.
   local tMetadataType
   put debuggerGetWatchDataStoreTypeForObject(pObject) into tMetadataType
   
   local tMetadataStore
   put debuggerGetWatchDataStoreForObject(pObject) into tMetadataStore
   
   revMetadataSet tMetadataStore, tMetadataType, pInformationType, pValue
end debuggerSetWatchInformation


private function debuggerGetWatchInformation pObject, pInformationType
   # Watch information (watchedVariables) can be found in two possible locations,
   # if pObject is given, then the stack owning pObject will contain the metadata in its
   # cREVGeneral custom property set. This is for watchedVariables at handler or script local scope.
   # For global variables, pObject is empty, and the information is stored in the preferences stack.
   local tMetadataType
   put debuggerGetWatchDataStoreTypeForObject(pObject) into tMetadataType
   
   local tMetadataStore
   put debuggerGetWatchDataStoreForObject(pObject) into tMetadataStore
   
   return revMetadataGet(tMetadataStore, tMetadataType, pInformationType)
end debuggerGetWatchInformation


# Parameters
#   pObject : reference to the object owning the watch
#   pHandler : the name of the handler owning the watch
#   pVariable : the name of the varaiable being watched
# Description
#   Deactivates the specified watch, by removing it from the watchedVariables.
command revDebuggerDeactivateWatch pObject, pHandler, pVariable
   --   local tWatch, tDataStore, tMetadataType
   --   if pObject is empty then
   --      put empty & comma & empty & comma & pVariable into tWatch
   
   --   else
   --   put debuggerGetId(pObject) & comma & pHandler & comma & pVariable into tWatch
   --   put debuggerTargetStack(pObject) into tDataStore
   --   put kMetadataType into tMetadataType
   --   end if
   
   local tStackWatches
   put debuggerGetWatchInformation(pObject, "watches") into tStackWatches
   
   local tWatch
   put debuggerBuildWatchLineFromDetails(pObject, pHandler, pVariable) into tWatch
   
   local tStackLineNumber
   put debuggerWatchOffset(tWatch, tStackWatches) into tStackLineNumber
   
   if tStackLineNumber = 0 then
      # This is a bug in the code that called this command. Don't throw errors in the debugger library
      # if possible though as it could make the IDE unstable.
      return "Watch not found"
   end if
   
   # Deactivate the watch
   local tCurrentWatches
   put the watchedVariables into tCurrentWatches
   
   local tCondition
   put item 4 to -1 of line tStackLineNumber of tStackWatches into tCondition
   
   local tResolvedObject
   if pObject is empty then
      put empty into tResolvedObject
   else
      put debuggerResolveId(item 1 of tWatch, debuggerTargetStack(pObject)) into tResolvedObject
   end if
   
   local tWatchString
   if tCondition is empty then
      put tResolvedObject & comma & pHandler & comma & pVariable into tWatchString
   else
      put tResolvedObject & comma & pHandler & comma & pVariable & comma & tCondition into tWatchString
   end if
   
   local tLineNumber
   put debuggerWatchOffset(tWatchString, tCurrentWatches) into tLineNumber
   
   delete line tLineNumber of tCurrentWatches
   set the watchedVariables to tCurrentWatches
   
   # Store the watch's status in the object so it is remembered between restarts etc
   debuggerUpdateWatchStatus pObject, tStackLineNumber, "inactive"
end revDebuggerDeactivateWatch

# Parameters
#   pObject : reference to the object to list watches for
# Returns
#   A list of watches set on the specified object, in the format of the watchedVariables property.
function revDebuggerListWatches pObject
   local tWatches
   put debuggerGetWatchInformation(pObject, "watches") into tWatches
   
   local tMatchingWatches
   repeat for each line tWatch in tWatches
      if pObject is empty then
         put empty & comma & item 2 to -1 of tWatch & return after tMatchingWatches
      else
         if debuggerGetId(pObject) is item 1 of tWatch then
            put debuggerResolveId(item 1 of tWatch, debuggerTargetStack(pObject)) & comma & item 2 to -1 of tWatch & return after tMatchingWatches
         end if
      end if
   end repeat
   delete the last char of tMatchingWatches
   
   return tMatchingWatches
end revDebuggerListWatches

function revDebuggerListWatchStates pObject
   
   local tWatches
   put debuggerGetWatchInformation(pObject, "watches") into tWatches
   
   local tMatchingLineNumbers
   
   local tLineNumber
   put 1 into tLineNumber
   repeat for each line tWatch in tWatches
      if pObject is empty then
         put tLineNumber & return after tMatchingLineNumbers
      else
         if debuggerGetId(pObject) is item 1 of tWatch then
            put tLineNumber & return after tMatchingLineNumbers
         end if
      end if
      add 1 to tLineNumber
   end repeat
   delete the last char of tMatchingLineNumbers
   
   local tStates
   put debuggerGetWatchInformation(pObject, "watchstates") into tStates
   
   local tMatchingStates
   repeat for each line tMatchingLineNumber in tMatchingLineNumbers
      put line tMatchingLineNumber of tStates & return after tMatchingStates
   end repeat
   delete the last char of tMatchingStates
   
   return tMatchingStates
end revDebuggerListWatchStates

# Parameters
#   pObject : reference to the object to list breakpoints for.
# Description
#   Returns a list of the statuses of the breakpoints for pObject.
#   each status is either "active" or "inactive". These cannot be
#   set directly, but are set by using the revDebuggerActivateBreakpoint
#   and revDebuggerDeactiveBreakpoint commands. The statuses are returned
#   in the same order as the breakpoints list.
function revDebuggerListBreakpointStates pObject
   local tStack
   put debuggerTargetStack(pObject) into tStack
   
   local tBreakpoints
   put revMetadataGet(tStack, kMetadataType, "breakpoints") into tBreakpoints
   
   local tLineNumber
   put 1 into tLineNumber
   
   local tMatchingLines
   repeat for each line tBreakpoint in tBreakpoints
      if debuggerGetId(pObject) = item 1 of tBreakpoint then
         put tLineNumber & return after tMatchingLines
      end if
      add 1 to tLineNumber
   end repeat
   delete the last char of tMatchingLines
   
   local tStates
   put revMetadataGet(tStack, kMetadataType, "breakpointstates") into tStates
   
   local tMatchingStates
   repeat for each line tMatchingLine in tMatchingLines
      put line tMatchingLine of tStates & return after tMatchingStates
   end repeat
   delete the last char of tMatchingStates
   
   return tMatchingStates
end revDebuggerListBreakpointStates

# Parameters
#   pObject : reference to the object to list breakpoints for.
# Description
#   Returns a list of the conditions of the breakpoints for pObject.
function revDebuggerListBreakpointConditions pObject
   local tStack
   put debuggerTargetStack(pObject) into tStack
   
   local tBreakpoints
   put revMetadataGet(tStack, kMetadataType, "breakpoints") into tBreakpoints
   
   local tLineNumber
   put 1 into tLineNumber
   
   local tMatchingLines
   repeat for each line tBreakpoint in tBreakpoints
      if debuggerGetId(pObject) = item 1 of tBreakpoint then
         put tLineNumber & return after tMatchingLines
      end if
      add 1 to tLineNumber
   end repeat
   delete the last char of tMatchingLines
   
   local tConditions
   put revMetadataGet(tStack, kMetadataType, "breakpointconditions") into tConditions
   
   local tMatchingConditions
   repeat for each line tMatchingLine in tMatchingLines
      put line tMatchingLine of tConditions & return after tMatchingConditions
   end repeat
   delete the last char of tMatchingConditions
   
   return tMatchingConditions
end revDebuggerListBreakpointConditions

# Parameters
#   @pScript : The script of the object to find an available line to break on. This is not mutated, its a reference for efficiency
#   pLineNumber : the number of the line to start the search from. This is *not* inclusive, i.e you must provide the number
#                          of the line *before* the first line you wish to put a breakpoint on. The reason for this is line continuation
# Returns
#   The number of the next available line after pLineNumber where a breakpoint can be placed in pScript.
#   If there is no appropriate line before the end of the script, 0 is returned. Note that pLineNumber must be a valid
#   line to put a breakpoint on itself, otherwise this may not return a valid line number, e.g. could be outside a handler.
function revDebuggerNextAvailableBreakpoint @pScript, pLineNumber
   # For now we have to hack this without access to the parser
   local tMatchingLineNumber
   
   # If pLineNumber is 0, we check the first line of the script, otherwise the second line is the first
   # possible breakpoint position. Of course the first line cannot be a continuation, so we just check
   # it is not a comment. If the line doesn't match, resume the search as if the user had passed 1.
   local tLineNumber
   if pLineNumber = 0 then
      if token 1 of line 1 of pScript is not empty then
         return 1
      else
         put 1 into tLineNumber
      end if
   else
      put pLineNumber into tLineNumber
   end if
   
   local tCurrentLineNumber
   put tLineNumber - 1 into tCurrentLineNumber
   
   local tText
   put line tLineNumber to -1 of pScript into tText
   
   local tContinuation
   put debuggerLineHasContinuation(line 1 of tText) into tContinuation
   
   local tInHandler
   
   repeat for each line tCurrentLine in line 2 to -1 of tText
      add 1 to tCurrentLineNumber
      
      # Detect if the line is a comment or the line is whitespace
      if token 1 of tCurrentLine is empty then
         next repeat
      end if
      
      # Detect if the line is a continuation of line tLineNumber
      if tContinuation then
         put debuggerLineHasContinuation(tCurrentLine) into tContinuation
         next repeat
      end if
      
      # Detect if the line is a variable declaration
      # (Check removed, I think this may actually be ok to allow breakpoints on)
      #      if token 1 of tCurrentLine is among the words of "constant local global" then
      #         next repeat
      #      end if
      
      # With control structures, only the first line of the control structure, and the statements inside it
      # can be broken on. These intermediate parts of structures cannot be broken on. Note that "else if" is an
      # exception to this.
      if token 1 of tCurrentLine is among the words of "finally catch case else" then
         next repeat
      end if
      
      # The endings of control structures cannot be broken on (apart from handlers).
      if token 1 of tCurrentLine is "end" and token 2 of tCurrentLine is among the words of "repeat switch if try" then
         next repeat
      end if
      
      put tCurrentLineNumber into tMatchingLineNumber
      exit repeat
   end repeat
   
   if tMatchingLineNumber is empty then
      return 0
   else
      # Add 1 because we started at line 2...
      return tMatchingLineNumber + 1
   end if
end revDebuggerNextAvailableBreakpoint
     

################################################################################
#
# Debugger library internals
#
################################################################################

# Stores the last contexts that was displayed in the script editor. Note that we store the entire call stack
# in order to unambiguously identify each execution point. This is particularly important when dealing with recursion.
local sLastDisplayedContexts

# Parameters
#   pBreakpoint : the breakpoint to search for (object, line)
#   pBreakpoints : the list of breakpoints to search in
# Returns
#   The line number that the specified breakpoint appears on in
#   pBreakpoints, or 0 if it does not appear.
private function debuggerBreakpointOffset pBreakpoint, pBreakpoints
   set the wholeMatches to true
   return lineOffset(pBreakpoint, pBreakpoints)
   set the wholeMatches to false
end debuggerBreakpointOffset

# Parameters
#   pWatch : the watch to search for (object, handler, variable)
#   pWatches : the list of watches to search in
private function debuggerWatchoffset pWatch, pWatches
   local tMatchString
   if the last char of pWatch is not comma then
      put pWatch & comma into tMatchString
   else
      put pWatch into tMatchString
   end if
   return lineOffset(tMatchString, pWatches)
end debuggerWatchoffset

# Parameters
#   pLine : a line of Revolution script
# Returns
#   Whether or not pLine has a continuation
private function debuggerLineHasContinuation pLine
   # First find if the line contains a continuation char among its words, if not then we're done.
   local tContinuationCharOffset
   put wordOffset("\", pLine) into tContinuationCharOffset
   if tContinuationCharOffset = 0 then
      return false
   end if
   
   # Nothing apart from a comment can legally come after the continuation char so if we
   # have one then the result is probably true, however we have to ensure that the char
   # we found is not in a comment itself. This cannot easily be done without access to the
   # parser in the engine, however make a simple attempt here.
   
   repeat for each word tWord in pLine
      # Miss out literals as they can contain any char
      if char 1 of tWord is quote and char -1 of tWord is quote then
         next repeat
      end if
      
      # If we find a word containing the continuation char, find out if there is a comment
      # char before it. If so, return false. Otherwise return true
      local tOffset
      put offset("\", tWord) into tOffset
      if tOffset <> 0 then
         local tPrefix
         put char 1 to tOffset of tWord into tPrefix
         if tPrefix contains "#" or tPrefix contains "//" or tPrefix contains "--" then
            return false
         else
            return true
         end if
      end if
   end repeat
end debuggerLineHasContinuation


# Parameters
#   pObject : reference to the object owning the breakpoint
#   pLine : the line number that the breakpoint is on
#   pContext : the debug context to evaluate the condition in
# Description
#   Evaluates the specified breakpoint's condition and returns whether to break or not.
#   If the breakpoint has no condition attached to to, returns true, otherwise it depends on the condition.
private function debuggerEvaluateCondition pObject, pLine, pContext
   
   local tStack
   put debuggerTargetStack(pObject) into tStack
   
   local tBreakpoints
   put revMetadataGet(tStack, kMetadataType, "breakpoints") into tBreakpoints
   
   local tBreakpoint
   put debuggerGetId(pObject) & comma & pLine into tBreakpoint
   
   local tLineNumber
   put debuggerBreakpointOffset(tBreakpoint, tBreakpoints) into tLineNumber
   if tLineNumber = 0 then
      # The debugger library does not know about this breakpoint but this is not neccessarily an error,
      # it could be a script breakpoint or one that the user manually set. Also it could be an internal breakpoint
      #  used by the debugger library to step over code faster. So here we act as though no condition was set.
      return true
   end if
   
   local tConditions
   put revMetadataGet(tStack, kMetadataType, "breakpointconditions") into tConditions
   
   local tCondition
   put line tLineNumber of tConditions into tCondition
   
   #log "Condition found for specified breakpoint: " & tCondition
   if tCondition is empty then
      return true
   end if
   
   local tResult
   
   set the debugContext to pContext
   debugDo "return " & tCondition
   put the result into tResult
   set the debugContext to empty
   #log "Result of condition evaluation: " & tResult
   
   if tResult then
      return true
   else
      return false
   end if
end debuggerEvaluateCondition

private function debuggerTargetStack pObject
   local tStackName
   put revTargetStack(pObject) into tStackName
   return the long id of stack tStackName
end debuggerTargetStack

# Implements the revDebuggerAddBreakpoint and revDebuggerAddConditionalBreakpoint
# commands, please see the comments above them for semantics and parameters.
private command debuggerAddBreakpoint pObject, pLine, pCondition
   local tStack
   put debuggerTargetStack(pObject) into tStack
   
   local tStackBreakpoints
   put revMetadataGet(tStack, kMetadataType, "breakpoints") into tStackBreakpoints
   if the number of lines of tStackBreakpoints >= kObjectBreakpointLimit then
      # Return an error here as the other option, removal of other breakpoints
      # to make room could potentially confuse the user.
      return "Maximum number of breakpoints reached on object"
   end if
   
   local tNewBreakpoint
   put debuggerGetId(pObject) & comma & pLine into tNewBreakpoint
   
   if tStackBreakpoints is empty then
      put tNewBreakpoint into tStackBreakpoints
   else
      put tStackBreakpoints & return & tNewBreakpoint into tStackBreakpoints
   end if
   revMetadataSet tStack, kMetadataType, "breakpoints", tStackBreakpoints
   
   # Update the breakpoint state, this is inactive for newly added breakpoints
   local tStates
   put revMetadataGet(tStack, kMetadataType, "breakpointstates") into tStates
   put "inactive" into line (the number of lines of tStackBreakpoints) of tStates
   revMetadataSet tStack, kMetadataType, "breakpointstates", tStates
   
   if pCondition is empty then
      exit debuggerAddBreakpoint
   end if
   
   # Apply the condition if given.
   local tStackConditions
   put revMetadataGet(tStack, kMetadataType, "breakpointconditions") into tStackConditions
   put pCondition into line (the number of lines of tStackBreakpoints) of tStackConditions
   revMetadataSet tStack, kMetadataType, "breakpointconditions", tStackConditions
   
   debuggerCleanBreakpoints tStack
end debuggerAddBreakpoint

# Parameters
#   pObject : reference to the object to clean watches for
# Description
#   Cleans up the specified stack's watch related metadata by removing
#   watches that cannot be resolved (e.g. because the object was deleted)
#   and removing any duplicates.
private command debuggerCleanWatches pObject
   local tStack
   put debuggerGetWatchDataStoreForObject(pObject) into tStack
   
   local tMetadataType
   put debuggerGetWatchDataStoreTypeForObject(pObject) into tMetadataType
   
   local tWatches
   put revMetadataGet(tStack, tMetadataType, "watches") into tWatches
   
   local tStates
   put revMetadataGet(tStack, tMetadataType, "watchstates") into tStates
   
   local tLineNumber
   put 1 into tLineNumber
   
   local tCleanedWatches, tCleanedStates
   repeat for each line tWatch in tWatches
      # If an object is specified, we check that all the watches can be resolved for it,
      # otherwise we're cleaning the global watches, so we don't perform this check.
      if pObject is not empty then
         try
            get debuggerResolveId(item 1 of tWatch, tStack)
         catch tError
            # debuggerResolveId will fail if the watch cannot be resolved,
            # this means the object has somehow been deleted, moved etc without
            # the debugger library knowing about. This watch should be removed.
            next repeat
         end try
      end if
      
      # Remove duplicates
      if debuggerWatchOffset(tWatch, tCleanedWatches) <> 0 then
         next repeat
      end if
      
      put tWatch & return after tCleanedWatches
      put line tLineNumber of tStates & return after tCleanedStates
      add 1 to tLineNumber
   end repeat
   delete the last char of tCleanedWatches
   delete the last char of tCleanedStates
   
   revMetadataSet tStack, tMetadataType, "watches", tCleanedWatches
   revMetadataSet tStack, tMetadataType, "watchstates", tCleanedStates
end debuggerCleanWatches

# Parameters
#   pStack : reference to the stack to clean breakpoints for
# Description
#   Cleans up the specified stack's breakpoint related metadata by removing
#   breakpoints that cannot be resolved (e.g. because the object was deleted)
#   and removing any duplicates.
private command debuggerCleanBreakpoints pStack
   local tBreakpoints
   put revMetadataGet(pStack, kMetadataType, "breakpoints") into tBreakpoints
   
   local tStates
   put revMetadataGet(pStack, kMetadataType, "breakpointstates") into tStates
   
   local tConditions
   put revMetadataGet(pStack, kMetadataType, "breakpointconditions") into tConditions
   
   local tLineNumber
   put 1 into tLineNumber
   
   local tCleanedBreakpoints, tCleanedStates, tCleanedConditions
   repeat for each line tBreakpoint in tBreakpoints
      try
         get debuggerResolveId(item 1 of tBreakpoint, pStack)
      catch tError
         # debuggerResolveId will fail if the breakpoint cannot be resolved,
         # this means the object has somehow been deleted, moved etc without
         # the debugger library knowing about. This breakpoint should be removed.
         next repeat
      end try
      
      # Remove duplicates
      if tBreakpoint is among the lines of tCleanedBreakpoints then
         next repeat
      end if
      
      put tBreakpoint & return after tCleanedBreakpoints
      put line tLineNumber of tStates & return after tCleanedStates
      put line tLineNumber of tConditions & return after tCleanedConditions
      
      add 1 to tLineNumber
   end repeat
   delete the last char of tCleanedBreakpoints
   delete the last char of tCleanedStates
   delete the last char of tCleanedConditions
   
   revMetadataSet pStack, kMetadataType, "breakpoints", tCleanedBreakpoints
   revMetadataSet pStack, kMetadataType, "breakpointstates", tCleanedStates
   revMetadataSet pStack, kMetadataType, "breakpointconditions", tCleanedConditions
end debuggerCleanBreakpoints

# Parameters
#   pStack : reference to the stack to update
#   pBreakpointNumber : the lineoffset of the breakpoint to edit in the breakpoints list of pStack
#   pStatus : the breakpoint status to set. This can be "active" or "inactive".
# Description
#   Sets a breakpoint's status to pStatus.
private command debuggerUpdateBreakpointStatus pStack, pBreakpointNumber, pStatus
   local tObjectBreakpointStatus
   put revMetadataGet(pStack, kMetadataType, "breakpointstates") into tObjectBreakpointStatus
   put pStatus into line pBreakpointNumber of tObjectBreakpointStatus
   revMetadataSet pStack, kMetadataType, "breakpointstates", tObjectBreakpointStatus
end debuggerUpdateBreakpointStatus

# Parameters
#   pStack : reference to the stack to update
#   pBreakpointNumber : the lineoffset of the breakpoint to edit in the breakpoints list of pStack
#   pCondition : the breakpoint condition to set. This is a Revolution expression to be evaluated in the
#                       breakpoint's context and should evaluate to true or false.
# Description
#   Sets a breakpoint's condition to pCondition.
private command debuggerUpdateBreakpointCondition pStack, pBreakpointNumber, pCondition
   local tConditions
   put revMetadataGet(pStack, kMetadataType, "breakpointconditions") into tConditions
   put pCondition into line pBreakpointNumber of tConditions
   revMetadataSet pStack, kMetadataType, "breakpointconditions", tConditions
end debuggerUpdateBreakpointCondition

private command debuggerUpdateWatchStatus pObject, pWatchNumber, pStatus
   local tStack
   put debuggerGetWatchDataStoreForObject(pObject) into tStack
   
   local tMetadataType
   put debuggerGetWatchDataStoreTypeForObject(pObject) into tMetadataType
   
   local tStates
   put revMetadataGet(tStack, tMetadataType, "watchstates") into tStates
   
   put pStatus into line pWatchNumber of tStates
   
   revMetadataSet tStack, tMetadataType, "watchstates", tStates
end debuggerUpdateWatchStatus

# Paramaeters
#  pId : the debugger id of an object used to store a breakpoint.
#  pStack : reference to the stack that the id is owned by (or is)
# Returns
#   The long id of the object that pId refers to
# Description
#   pId is either the short id of a control in pStack, or 0 if the breakpoint
#   being resolved is in the stack script.
private function debuggerResolveId pId, pStack
   if pId is 0 then
      return the long id of pStack
   else
      # As the object could be a card, we attempt that first
      if there is a card id pId of pStack then
         return the long id of card id pId of pStack
      else
         return the long id control id pId of pStack
      end if
   end if
end debuggerResolveId

# Parameters
#   pObject : reference to the object to get the debugger id for
# Returns
#   The debugger id for the specified object. This is what is stored by the debugger
#   with breakpoints set on the object, and used to determine which object each
#   breakpoint is associated with. The debugger id is the short id of the object if
#   it is a control or a card. If the object is a stack, the debugger id is 0.
private function debuggerGetId pObject
   if word 1 of the name of pObject is "stack" then
      return 0
   end if
   
   return the id of pObject
end debuggerGetId

# Returns
#   Whether or not the debugger is enabled. This is currently determined by the "Script Debug Mode"
#   setting in the Revolution preferences stack.
private function debuggerEnabled
   
   # OK-2009-01-07 : It seems that under certain quite specific circumstances, this line can throw an error
   # somewhere in the MCU_dofrontscripts method. Unfortunately the only recipe I have is an OS X specific
   # bug involving revBrowser and a massive call stack, debugging this was proving too time-consuming, so
   # for now, the issue is fixed by locking messages to prevent the frontscripts running at all.
   lock messages
   
   # OK-2009-01-12 : Bizarrely it seems that the above lock messages can cause virtual setProps to not work
   # properly under some circumstances. In particular it broke the docs editor. So instead of just returning the 
   # required value, we now evaluate it, unlock messages, then return it...
   
   get the cREVScriptDebugMode of stack "revPreferences"
   unlock messages
   
   return (it is true)
end debuggerEnabled

# Parameters
#   pStack : reference to a stack
# Returns
#   Whether or not the debugger is allowed to debug the specified stack in
#   the current circumstances.
private function debuggerStackAllowed pStack
   global gREVDevelopment
   if gREVDEvelopment then
      # With gREVDevelopment allow all stacks to be debugged except the debugger, as doing this will probably not work anyway.
      if the short name of pStack is "revDebuggerLibrary" then
         return false
      else
         return true
      end if
   end if
   
   # This check is flawed and needs to be sorted out...
   if char 1 to 3 of the short name of pStack is "rev" or the short name of pStack is among the items of "Message Box,Home,Answer Dialog,Ask Dialog,File Selector" then
      return false
   else
      return true
   end if
end debuggerStackAllowed

# Parameters
#   pObject : reference to the object owning the executing code
#   pHandler : the name of the handler executing
#   pLine : the number of line about to be executed
#   pState : either "debug" if still debugging or "edit" if debugging has finished.
# Description
#   Called when the script editor may need to be updated because a new line
#   of code is about to be executed in the debugger. If there is no script
#   editor open for pObject then does nothing.
#   Otherwise tells the script editor to display the appropriate line etc.
private command debuggerUpdateScriptEditor pObject, pHandler, pLine, pState
   local tScriptEditor
   put revScriptEditor(the long id of pObject) into tScriptEditor
   if tScriptEditor is empty then
      debuggerShowScript pObject
   end if
   
   put revScriptEditor(the long id of pObject) into tScriptEditor
   put revDebuggerContexts() into sLastDisplayedContexts
   
   local tMode
   send "revSEGetMode" to stack tScriptEditor
   put the result into tMode
   if pState is not tMode then
      # This must be sent in time to allow the script editor a chance to set its current object,
      # otherwise an infinite loop could occur.
      send "revSESetMode pState" to stack tScriptEditor in 0 milliseconds
   end if
   
   local tFalseString
   put "false" into tFalseString
   
   # These must be sent in time to allow the script editor a chance to set its current object,
   # otherwise an infinite loop could occur.
   send "revSEGoExecutionPoint pObject, pLine, true" to stack tScriptEditor in 0 milliseconds
   send "revSEUpdate tFalseString" to stack tScriptEditor in 0 milliseconds
   
   # OK-2008-08-18 : Bug 6935 - Toplevel / uniconify the script editor here.
   revGoScriptEditor the name of stack tScriptEditor
end debuggerUpdateScriptEditor

private function debuggerErrorsSuppressed
   global gREVSuppressErrors
   return (gREVSuppressErrors is true)
end debuggerErrorsSuppressed

private function revScriptEditors
   return revListScriptEditors()
end revScriptEditors

private command debuggerShowScript pObject
   edit the script of pObject
end debuggerShowScript

private command log pMessage
   put pMessage & return after msg
end log

# Description
#   Shows revErrorDisplay in response to a trace error.
#   This has been hacked in for now to avoid modifying revErrorDisplay.
private command debuggerShowErrorDialog pObject, pHandler, pLine, pError
   answer error "An execution error ocurred in the script of the object : " & the short name of pObject & return & \
   "The error was on line " & pLine & " in the " & pHandler & " handler. The error was: " & return & \
   pError & " What would you like to do?" with "Ignore" or "Debug" or "Script"
   
   if it is "Ignore" then
      revDebuggerStop
      exit debuggerShowErrorDialog
   end if
   
   if it is "Script" then
      revDebuggerStop
      debuggerUpdateScriptEditor pObject, pHandler, pLine, "edit"
   end if
   
   if it is "Debug" then
      debuggerUpdateScriptEditor pObject, pHandler, pLine, "debug"
   end if
end debuggerShowErrorDialog

# Parameters
#   pTarget : the long id of the target of the message
#   pContextLine : the complete line of the executionContexts that represents the code in the user stack about to run.
# Returns
#   The target object that contains the line of script about to run, taking parentScripts into account.
private function debuggerResolveTarget pTarget, pContextLine
   # OK-2009-06-23: Bug 7954
   local tArray
   put parseExecutionContext(pContextLine) into tArray
   
   if tArray["behavior"] is not empty then
      return tArray["behavior"]
   else
      return pTarget
   end if
   
   --   if the number of items of pContextLine = 4 then
   --      return item 4 of pContextLine
   --   else
   --      return pTarget
   --   end if
end debuggerResolveTarget

# Parameters
#   pContextLine: the complete line of the executionContexts that represents the code in the user stack about to run.
private function debuggerResolveHandler pContextLine
    # OK-2009-06-23 : Bug 7954
   local tArray
   put parseExecutionContext(pContextLine) into tArray
   return tArray["handler"]
   
   --   if the number of items of pContextLine = 5 then
   --      return item 3 of pContextLine
   --   else
   --      return item -2 of pContextLine
   --   end if
end debuggerResolveHandler

private function debuggerTargetIsModal pTarget
   return (the mode of stack (revTargetStack(pTarget)) = 5)
end debuggerTargetIsModal

private command debuggerShowModalWarning
   
end debuggerShowModalWarning

################################################################################
#
# Event handlers
#
################################################################################

local sLastAllowedContext

# Description
#   Sent every time a line is about to be executed when tracing . If the stack containing the
#   code cannot be debugged, then continues executing the code without
#   taking any action. Otherwise searches for a script editor that might
#   be editing the target object, and sends it appropriate messages to tell
#   it which line is about to execute. (Also needs to do something with breakpoints)
#   If the last debug action was "Step Over" rather than "Step Into", checks
#   if the current execution context is the one after the line where "Step Over"
#   was called from, then updates the script editor. If the last debug action
#   was "Step Into", always updates the script editor.
on trace pHandler, pLine, pPosition
   local tTarget
   put debuggerResolveTarget(the long id of the target, line -2 of the executionContexts) into tTarget
   
   local tStack
   put debuggerTargetStack(tTarget) into tStack
   
   local tCurrentContexts
   put revDebuggerContexts() into tCurrentContexts
   
   if not debuggerStackAllowed(tStack) then
      # OK-2009-03-13 : Instead of stepping out, instead we just ignore this and wait for the next
      # trace message. This will result in the debugger being slower as some of the advantage of using
      # traceUntil will be lost. 
      local tNextContextToTry
      if sLastAllowedContext is not empty then
         put sLastAllowedContext - 1 into tNextContextToTry
      else
         put item 1 of line 1 of tCurrentContexts - 1 into tNextContextToTry
      end if
      
      # In some cases, this can result in a negative context, this happens in particular when an engine command
      # calls IDE code, e.g get url. For now we hack around this.
      if tNextContextToTry < 0 then
         put 0 into tNextContextToTry
      end if
      
      set the traceUntil to tNextContextToTry
      set the traceReturn to true
      exit trace
   else
      put item 1 of line 1 of tCurrentContexts into sLastAllowedContext
   end if
   
   local tNextContextCount
   
   if sLastTraceMode is "Step Into" then
      debuggerUpdateScriptEditor tTarget, pHandler, pLine, "debug"
      
   else if sLastTraceMode is "Step Over" then
      local tLastHandler
      put debuggerResolveHandler(line 1 of sLastDisplayedContexts) into tLastHandler
      if line 2 to -1 of sLastDisplayedContexts is line 2 to -1 of tCurrentContexts and pHandler is tLastHandler then
         debuggerUpdateScriptEditor tTarget, pHandler, pLine, "debug"
         set the traceStack to the name of tStack
      else
         # Calculate the number of execution contexts that there should be before we get the next trace message.
         # I'm not certain why this needs to be -2 (would expect it to be -1), perhaps because the check is carried out before the exiting handler
         # is removed from the call stack??
         put(item 1 of line 1 of tCurrentContexts) - 2 into tNextContextCount       
         set the traceUntil to tNextContextCount
         set the traceReturn to true
      end if
      
   else if sLastTraceMode is "Step Out" then
      # OK-2009-06-23 : Bug 7954
      local tArray
      put parseExecutionContext(line 2 of sLastDisplayedContexts) into tArray
      
      if line 3 to -1 of sLastDisplayedContexts is line 2 to -1 of revDebuggerContexts() and pHandler is tArray["handler"] then
         debuggerUpdateScriptEditor tTarget, pHandler, pLine, "debug"
      else
         put item 1 line 1 of tCurrentContexts - 2 into tNextContextCount
         set the traceUntil to tNextContextCount
         set the traceReturn to true
      end if
   end if
end trace
  

# Description
#   Sent when tracing has finished, i.e. there is nothing left to debug. Tells
#   all open script editors to switch to edit mode, then does the debugger run
#   action.
on traceDone
   # OK-2009-03-13 : Once we've got a traceDone, shut down the debugger first,
   # as otherwise we have to trace through all the script editor code which slows things down.
   revDebuggerRun
   
   repeat for each line tScriptEditor in revScriptEditors()
      send "revSESetMode edit" to tScriptEditor
   end repeat
end traceDone

constant kErrorRecursionLimit = 2

# Parameters
#   pHandler : the name of the handler that threw the error
#   pLineNumber : the number of line that the error was on
#   pPosition : the char number that the error was on
#   pError : the error details
# Description
#   Sent when an execution error occurs when debugging and not inside a try block.
#   If the debugger is not enabled, does nothing. If the error was thrown by a user
#   via the throw command, or messages are suppressed, exits to top.
#   Otherwise, launches the debugger, showing the current execution context.
on traceError pHandler, pLine, pPosition, pError
   if item 1 of pError is kErrorRecursionLimit then
      local tMessage
      put "The handler: " & pHandler & " has reached the recursion limit of: " & the recursionLimit & "." & return & \
             "Execution will be terminated to prevent hang" into tMessage
      
      answer error tMessage
      revDebuggerStop
      exit traceError
   end if
   
   local tTarget
   put debuggerResolveTarget(the long id of the target, line -2 of the executionContexts) into tTarget
   
   close printing
   
   if not debuggerEnabled() then
      pass traceError
   end if
   
   if debuggerTargetIsModal(tTarget) then
      debuggerShowModalWarning
      pass traceError
   end if
   
   local tStack
   put debuggerTargetStack(tTarget) into tStack
   
   # OK-2008-11-05 : If the target of the debugger message is modal, then act as though the debugger
   # is disabled. This is not ideal, but beats allowing the IDE to lock up.
   if the mode of tStack >= 5 then
      pass traceError
   end if
   
   if pError is empty then
      revDebuggerStop
      exit traceError
   end if
   
   if debuggerErrorsSuppressed() or the waitDepth > 2 then
      exit to top
   end if
   
   if not debuggerStackAllowed(tStack) then
      revDebuggerStop
      exit traceError
   end if
   
   debuggerUpdateScriptEditor tTarget, pHandler, pLine, "debug"
   
   set the traceStack to the short name of tStack
   
   local tEditor
   put revScriptEditor(tTarget) into tEditor
   
   # OK-2008-07-10 : This needs to be sent in time so that it arrives after the script
   # editor has finished initializing itself.
   send "revSEDisplayExecutionError pError, tTarget" to stack tEditor in 0 milliseconds
end traceError

# Parameters
#   pHandler : the handler that the breakpoint was triggered in
#   pLine : the line number that the breakpoint was on
# Description
#   Sent when a breakpoint triggers. If the debugger is not enabled, does
#   nothing. Otherwise, opens a script editor for the object (unless one is
#   already open) and updates it appropriately.
on traceBreak pHandler, pLine
   local tTarget
   put debuggerResolveTarget(the long id of the target, line -2 of the executionContexts) into tTarget
   
   if not debuggerEnabled() then
      pass traceBreak
   end if
   
   if debuggerTargetIsModal(tTarget) then
      debuggerShowModalWarning
      pass traceBreak
   end if
   
   local tStack
   put debuggerTargetStack(tTarget) into tStack
   if not debuggerStackAllowed(tStack) then
      revDebuggerRun
      exit traceBreak
   end if
   
   local tContext
   put tTarget & comma & pHandler & comma & pLine into tContext
   
   if debuggerEvaluateCondition(tTarget, pLine, tContext) then
      set the traceStack to the short name of debuggerTargetStack(the long id of the target)
      debuggerShowScript tTarget
      debuggerUpdateScriptEditor tTarget, pHandler, pLine, "debug"
   else
      set the traceReturn to true
   end if
end traceBreak


on updateVariable pLine, pHandler, pValue
   traceBreak pHandler, pLine
end updateVariable

  








