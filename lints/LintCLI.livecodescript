script "LintCLI"
----------------------------------------------------------------
-- Main program
----------------------------------------------------------------

on startup
   send "LintCLIMain" to me in 0
end startup

on LintCLIMain
   LoadLibraries
   ResetStats
   
   LinterAddMessageCallback the long id of me, "LintCLIMessageCallback"

   local tArgs
   put GetProcessedArguments() into tArgs
   if "help" is among the keys of tArgs["options"] then
      LintCLIUsage
   end if

   if "verbose" is among the keys of tArgs["options"] then
      put 2 into sVerbosity
   else
      put 1 into sVerbosity
   end if

   local tPath
   if the number of elements in tArgs["arguments"] is 0 then
      LinterScanPath the defaultFolder
   else
      repeat for each element tPath in tArgs["arguments"]
         LinterScanPath tPath
      end repeat
   end if

   PrintStats

   if IsLintSuccessful() then
      quit 0
   else
      quit 33
   end if
end LintCLIMain

on LintCLIUsage
   write \
         "Usage: LintCLI [OPTION ...] [--] [FILE ...]" & return & \
         return & \
         "Check LiveCode source code & documentation for problems." & \
         return & \
         "Options:" & return & \
         "  -h, --help                 Print this message" & return & \
         "      --verbose              Output extra debugging information" & return & \
         "  --                         Treat all remaining arguments as filenames" & return & \
         return & \
         "Report bugs to <http://quality.livecode.com/>" & return \
         to stdout
   quit 0
end LintCLIUsage

local sFixedCount
local sUnfixedCount

private command ResetStats
   put 0 into sFixedCount
   put 0 into sUnfixedCount
end ResetStats

private command UpdateStats pIsFixed
   if pIsFixed then
      add 1 to sFixedCount
   else
      add 1 to sUnfixedCount
   end if
end UpdateStats

private command PrintStats
   local tTotal, tMessage
   put sFixedCount + sUnfixedCount into tTotal

   switch tTotal
      case 0
         put "No lint warnings" into tMessage
         break
      case 1
         put "1 lint warning" into tMessage
         break
      default
         put merge("[[tTotal]] lint warnings") into tMessage
         break
   end switch

   switch sFixedCount
      case 0
         break
      case 1
      default
         put merge(" ([[sFixedCount]] fixed)") after tMessage
         break
   end switch

   write tMessage & return to stdout
end PrintStats

private function IsLintSuccessful
   return (sUnfixedCount is 0)
end IsLintSuccessful

----------------------------------------------------------------
-- Argument handling
----------------------------------------------------------------

constant kOptions = "h,help verbose"

private function GetRelevantArguments
   local tArg, tFound, tResult
   repeat for each element tArg in the commandArguments
      if tArg ends with "LintCLI.livecodescript" then
         put true into tFound
         next repeat
      end if
      if tFound then
         put tArg into tResult[1 + the number of elements in tResult]
      end if
   end repeat
   return tResult
end GetRelevantArguments

private command ConvertArgumentToPath @xArg
   if the platform is "Win32" then
      replace "\" with "/" in xArg
   end if
end ConvertArgumentToPath

private function GetProcessedArguments
   local tProcessed
   put GetOpt(kOptions, GetRelevantArguments()) into tProcessed

   local tError, tErrors
   repeat for each element tError in tProcessed["errors"]
      put tError & return after tErrors
   end repeat
   if tErrors is not empty then
      LogError tErrors
   end if

   -- Convert non-option arguments into filenames
   local tKey
   repeat for each key tKey in tProcessed["arguments"]
      ConvertArgumentToPath tProcessed["arguments"][tKey]
   end repeat

   return tProcessed
end GetProcessedArguments

----------------------------------------------------------------
-- Library handling
----------------------------------------------------------------

private command LoadLibraries
   LoadLintStack "LintCLIBackscriptLibrary"
   LoadScriptExtension "getopt"
   LoadLibraryExtension "json"
   LoadLintStack "LintLibrary"
   LoadLintStack "LinterLibrary"
end LoadLibraries

private command LoadLintStack pLibrary
   local tFilePath, tStackName
   put GetSourceFolder() & "/lints/" & pLibrary & ".livecodescript" \
         into tFilePath
   LogDebug merge("Loading [[tFilePath]]")
   put the name of stack tFilePath into tStackName
   send "revLoadLibrary" to stack tStackName
end LoadLintStack

private command LoadScriptExtension pExtension
   local tFilePath
   put GetSourceFolder() & "/extensions/script-libraries/" & \
         pExtension & "/" & pExtension & ".livecodescript" \
         into tFilePath
   LogDebug merge("Loading [[tFilePath]]")
   start using stack tFilePath
end LoadScriptExtension

private command LoadLibraryExtension pExtension
   local tFilePath
   put GetSourceFolder() & "/extensions/libraries/" & \
         pExtension & "/module.lcm" \
         into tFilePath
   LogDebug merge("Loading [[tFilePath]]")
   load extension from file tFilePath
   if the result is not empty then
      LogError the result
   end if
end LoadLibraryExtension

private function GetSourceFolder
   set the itemdelimiter to slash
   return item 1 to -3 of the filename of this me
end GetSourceFolder

----------------------------------------------------------------
-- Logging & error handling
----------------------------------------------------------------

local sVerbosity

on LintCLIMessageCallback pType, pLocation, pMessage, pIsFixed
   local tLocationString, tElement, tLine, tSuffix, tLint
   
   if word 1 of pType is "lint" then
      repeat for each element tElement in pLocation
         put tElement & ":" after tLocationString
      end repeat

      put word 2 to -1 of pType into tLint
      if tLint is not empty then
         put "[" & tLint & "]" after tSuffix
      end if
      if pIsFixed then
         put "[fixed]" after tSuffix
      end if
      if tSuffix is not empty then
         put space before tSuffix
      end if

      repeat for each line tLine in pMessage
         write tLocationString & tLine & tSuffix & return to stdout
      end repeat
      
      UpdateStats pIsFixed
   else
      LogMessage pType, pMessage
   end if
end LintCLIMessageCallback

on errorDialog pExecutionError
   LogDebug pExecutionError
   LogError "Uncaught exception"
end errorDialog

private command LogError pMessage
   LogMessage "error", pMessage
   quit 1
end LogError

private command LogDebug pMessage
   LogMessage "debug", pMessage
end LogDebug

private command LogMessage pLevel, pMessage
   if pMessage is empty then
      exit LogMessage
   end if
   
   local tPrefix
   switch pLevel
      case "debug"
         if sVerbosity < 2 then
            exit LogMessage
         end if
      case "warning"
         put "warning: " into tPrefix
         break
      case "error"
         put "ERROR: " into tPrefix
         break
      default
         if sVerbosity < 1 then
            exit LogMessage
         end if
         put empty into tPrefix
         break
   end switch
   
   local tLine
   repeat for each line tLine in pMessage
      write tPrefix & tLine & return to stderr
   end repeat
end LogMessage
