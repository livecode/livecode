script "SymbolicatorScript"
constant kLiveCodeAppId = "com.runrev.livecode"
constant kStagingUrl = "https://downloads.livecode.com/livecode/staging"
constant kShortFlags = "v,d,h"
constant kLongFlags = "verbose,debug,help,fetch,hang"
constant kValueOptions = "repo,cache-dir,symbol-dir,version,staging-url,edition,output"

local sDebugging
local sVerbose
local sHangReport


---------------------------------------
---------- COMMAND-LINE TOOL ----------
---------------------------------------


on startup
   local tValidArgs
   put getValidArgs() into tValidArgs
   
   -- Convert the command-line arguments into an array for easier reference
   local tArgs
   try
      put arrayifyArguments(tValidArgs) into tArgs
   catch tError
      write "Error processing arguments:" && tError & return to stderr
      quit -1
   end try
   
   -- If help was requested, show it then exit
   if the number of elements of tArgs is zero or "help" is among the keys of tArgs then
      showHelp
      quit 0
   end if
   
   if "hang" is among the keys of tArgs then put true into sHangReport
   
   -- Show debugging information?
   if "d" or "debug" is among the keys of tArgs then put true into sDebugging
   
   -- Show verbose information?
   if "v" or "verbose" is among the keys of tArgs then put true into sVerbose
   
   -- Read the crash log file
   local tCrashLog
   put url ("file:" & tArgs["__inputs__"]) into tCrashLog
   if the result is not empty then
      write "Failed to read" && quote & tArgs["__inputs__"] & quote & return to stderr
      quit -1
   end if
   
   -- Set some default options if they weren't specified
   local tGuessSucceeded
   local tVersion
   local tEdition
   put guessVersionAndEdition(tCrashLog, tVersion, tEdition) into tGuessSucceeded
   if "staging-url" is not among the keys of tArgs then put kStagingUrl into tArgs["staging-url"]
   if "repo" is not among the keys of tArgs then 
      -- Assumes we're in the main repo folder
      if "fetch" is among the keys of tArgs then
         -- Use the private repo
         set the itemDelimiter to slash
         put item 1 to -2 of the current folder into tArgs["repo"]
      else
         put the current folder into tArgs["repo"]
      end if
   end if
   
   -- Dump the input arguments to stderr if debugging is enabled
   debugPrint dumpArray(tArgs)
   
   -- Check that the mandatory options are present
   if "version" is not among the keys of tArgs and not tGuessSucceeded then
      write "Error: version could not be auto-detected" & return to stderr
      quit -1
   end if
   if "edition" is not among the keys of tArgs and not tGuessSucceeded then
      write "Error: edition could not be auto-detected" & return to stderr
      quit -1
   end if
   if "fetch" is among the keys of tArgs and "cache-dir" is not among the keys of tArgs then
      write "Error: cache directory must be specified when fetching symbols" & return to stderr
      quit -1
   end if
   if "fetch" is not among the keys of tArgs and "symbol-dir" is not among the keys of tArgs then
      write "Error: directory containing debugging symbols must be specified when not fetching symbols" & return to stderr
      quit -1
   end if
   if "fetch" is among the keys of tArgs and "output" is not among the keys of tArgs then
      write "Error: output file must be specified when fetching symbols (interaction may be required)" & return to stderr
      quit -1
   end if
   
   -- Version and edition of LiveCode we're symbolicating
   if "version" is among the keys of tArgs then put tArgs["version"] into tVersion
   if "edition" is among the keys of tArgs then put tArgs["edition"] into tEdition
   
   -- Download the symbols for the requested version if necessary
   local tSymbolDir
   local tSymbolPath
   if "fetch" is among the keys of tArgs then
      local tCacheDir
      local tRepoDir
      put tArgs["cache-dir"] into tCacheDir
      put tArgs["repo"] into tRepoDir
      put pathToSymbolDirForVersion(tCacheDir, tVersion) into tSymbolDir
      try
         verbosePrint "Downloading debug symbols for" && tVersion && "to" && tSymbolDir
         fetchSymbolsForTag tRepoDir, tSymbolDir, tArgs["staging-url"], tVersion
      catch tError
         write "Error:" && tError & return to stderr
         quit -1
      end try
   else
      put pathToSymbolDirForVersion(tArgs["symbol-dir"], tVersion) into tSymbolDir
   end if
   put pathToSymbolFileForVersion(tSymbolDir, tEdition) into tSymbolPath
   
   -- Process the crashlog file
   local tSymbolicatedLog
   try
      verbosePrint "Symbolicating log"
      put symbolicateLog(tCrashLog, tSymbolPath) into tSymbolicatedLog
   catch tError
      write "Failed to symbolicate crash log:" && tError & return to stderr
      quit -1
   end try
   
   -- Write the output to the appropriate location
   if "output" is not among the keys of tArgs then
      write tSymbolicatedLog to stdout
   else
      local tOutput
      put tArgs["output"] into tOutput
      put tSymbolicatedLog into url ("file:" & tOutput)
      if the result is not empty then write "Failed to write output:" && the result to stderr
      quit -1
   end if
   
   -- Processing complete
   quit 0
end startup

private command showHelp
   local tHelpMessage
   put \
"LiveCode symbolicator script for OSX engines"                                  & return & \
"Adds symbols to crash logs generated by MacOSX"                                & return & \
                                                                                  return & \
"Usage:"                                                                        & return & \
"  symbolicate.sh --help"                                                       & return & \
"  symbolicate.sh [-v|--verbose] [-d|--debug] [--hang] [--repo=...] "           & return & \
"    [--symbol-dir=...] [--fetch --cache-dir=... [--staging-url=...]] "         & return & \
"    [--version=...] [--edition=...] [--output=...] input.log"                  & return & \
                                                                                  return & \
"Options:"                                                                      & return & \
"  --help               Displays this message"                                  & return & \
                                                                                  return & \
"  --verbose            Displays more progress information"                     & return & \
                                                                                  return & \
"  --debug              Displays information that may help debug problems with" & return & \
"                       this script"                                            & return & \
                                                                                  return & \
"  --hang               Treat the input file as a hang log rather than a crash" & return & \
"                       log"                                                    & return & \
                                                                                  return & \
"  --repo=...           Path to a current checkout of the LiveCode source code" & return & \
"                       repository. If not specified, the current directory"    & return & \
"                       will be used."                                          & return & \
                                                                                  return & \
"  --symbol-dir=...     Path to a directory where the debugging symbols for"    & return & \
"                       the LiveCode engines are stored. This option is"        & return & \
"                       required if --fetch is not specified."                  & return & \
                                                                                  return & \
"  --fetch              Automatically fetches the engine debugging symbols"     & return & \
"                       from a staging server. If this option is specified,"    & return & \
"                       the --output option is also required as interaction"    & return & \
"                       may be required to authenticate to the server. The"     & return & \
"                       directory into which the symbols will be downloaded"    & return & \
"                       must also be given, using the --cache-dir option."      & return & \
                                                                                  return & \
"  --cache-dir=...      Directory to which symbols will be downloaded when"     & return & \
"                       the --fetch option is specified. (Once fetched, these"  & return & \
"                       symbols can be re-used by specifying this directory as" & return & \
"                       the --symbol-dir in future invocations)."               & return & \
                                                                                  return & \
"  --staging-url=...    When fetching symbols, the URL to the staging server"   & return & \
"                       that contains the archived debugging symbols. If not"   & return & \
"                       specified, the default LiveCode Ltd. staging server"    & return & \
"                       will be used; this server is not currently publicly"    & return & \
"                       accessible."                                            & return & \
                                                                                  return & \
"  --version=...        Version of the engine the crash log is from. If not"    & return & \
"                       given, the script will attempt to auto-detect it."      & return & \
                                                                                  return & \
"  --edition=...        One of 'community', 'indy' or 'business'; identifies"   & return & \
"                       the edition of LiveCode the crash log is from. If not"  & return & \
"                       given, the script will attempt to auto-detect it."      & return & \
                                                                                  return & \
"  --output=...         Path to the file to which the symbolicated log will be" & return & \
"                       written. If --fetch has been specified, this option is" & return & \
"                       required as user interaction may be required. Any"      & return & \
"                       existing contents of this file will be overwritten. If" & return & \
"                       not given, the output will be written to stdout."       & return & \
                                                                                  return & \
"  input.log            Path to the log that should be symbolicated. Only one"  & return & \
"                       input file may be specified."                           & return & \
                                                                                  return   \
      into tHelpMessage
   write tHelpMessage to stdout
end showHelp

private command debugPrint pString
   if sDebugging then write pString & return to stderr
end debugPrint

private command verbosePrint pString
   if sVerbose then write pString & return to stderr
end verbosePrint

private function getValidArgs
   local tValidArgs
   local tShortFlags
   local tLongFlags
   local tValueOptions
   put kShortFlags into tShortFlags
   put kLongFlags into tLongFlags
   put kValueOptions into tValueOptions
   split tShortFlags by comma as set 
   split tLongFlags by comma as set 
   split tValueOptions by comma as set 
   put tShortFlags into tValidArgs["short_flag_arg"]
   put tLongFlags into tValidArgs["long_flag_arg"]
   put tValueOptions into tValidArgs["value_arg"]
   return tValidArgs
end getValidArgs

-- Used by the arrayifyArguments function to add options into the argument array
private command addValueOptionToArgArray @xArgArray, pOptionName, pOptionValue
   -- Add this option to the array. This is done in a "smart" manner:
   -- if the array already contains an option of this name, they are accumulated into a sub-array
   -- otherwise, the array value is set directly to the option value
   if pOptionName is among the keys of xArgArray then
      -- If the option has already been made into an array, just append the option value
      if xArgArray[pOptionName] is an array then
         put pOptionValue into xArgArray[pOptionName][the number of lines in the keys of xArgArray[pOptionName] + 1]
      else
         -- Convert the option into a sub-array
         local tCurrentValue
         local tIndex
         put 1 into tIndex
         if pOptionName is among the keys of xArgArray then
            put xArgArray[pOptionName] into tCurrentValue
            put tCurrentValue into xArgArray[pOptionName][1]
            put 2 into tIndex
         end if
         
         put pOptionValue into xArgArray[pOptionName][tIndex]
      end if
   else
      -- Simply add the option into the array
      put pOptionValue into xArgArray[pOptionName]
   end if
end addValueOptionToArgArray

-- Validates a command-line argument
private command validateArgument pValidArgs, pOptionName, pType
   switch pType
      case "value"
         -- Is this a valid name for an argument with a value?
         if pOptionName is not among the keys of pValidArgs["value_arg"] then
            -- Better error messages!
            if pOptionName is among the keys of pValidArgs["long_flag_arg"] then
               throw "option" && quote & "--" & pOptionName & quote && "does not take a value"
            else
               throw "unknown option" && quote & pOptionName & quote
            end if
         end if
         break
      case "long"
         -- Is this a valid name for an argument without a value?
         if pOptionName is not among the keys of pValidArgs["long_flag_arg"] then
            -- Better error messages!
            if pOptionName is among the keys of pValidArgs["value_arg"] then
               throw "option" && quote & "--" & pOptionName & quote && "requires a value"
            else
               throw "unknown option" && quote & pOptionName & quote
            end if
         end if
         break
      case "short"
         -- Is this a valid short flag argument?
         if pOptionName is not among the keys of pValidArgs["short_flag_arg"] then
            throw "unknown flag" && quote & "-" & pOptionName & quote
         end if
         break
   end switch
end validateArgument

-- Processes command-line argumenst into an array
-- NOTE: all arguments taking values begin "--" while those that are flags begin "-"
-- Flags using "-" may be specified together (e.g. "-a -b" and "-ab" are the same)
-- For arguments taking values, empty values are allowed (on the command line, these would be '--arg=""')
private function arrayifyArguments pValidArgs
   -- Loop over the command-line arguments 
   local tSkipNext
   local tNoMoreOptions
   repeat with i = 1 to ($# - 1)
      local tArg
      local tArray
      local tOptionName
      local tOptionValue
      
      -- Skip this argument if necessary
      if tSkipNext then
         put false into tSkipNext
         next repeat
      end if
      
      put the value of ("$" & i) into tArg
      
      -- If this argument begins with "--" then it is an option
      if not tNoMoreOptions and tArg begins with "--" then
         -- If the option in its entirety is "--", it incidates that all further arguments are inputs, not options
         if tArg is "--" then
            put true into tNoMoreOptions
         else
            -- Does this option specify its value directly? (i.e does it contain '='?)
            local tEqualsOffset
            put offset("=", tArg) into tEqualsOffset
            if tEqualsOffset is not zero then
               -- Split the option into the option name and value
               put char 3 to (tEqualsOffset - 1) of tArg into tOptionName
               put char (tEqualsOffset + 1) to -1 of tArg into tOptionValue
               
               -- Options must have a name (i.e "--=" is invalid)
               if tOptionName is empty then throw "Option with empty name"
               
               -- Validate the argument
               validateArgument pValidArgs, tOptionName, "value"
               
               -- Add the option to the arguments array
               addValueOptionToArgArray tArray, tOptionName, tOptionValue
            else
               -- This option began "--" but doesn't contain an '='; it may or may not take a value
               put char 3 to -1 of tArg into tOptionName
               if tOptionName is among the keys of pValidArgs["value_arg"] then
                  -- Argument validation is skipped here as we know it is a valid value arg
                  
                  -- Get the value from the next argument
                  put the value of ("$" & (i+1)) into tOptionValue
                  
                  -- Add the option to the arguments array
                  addValueOptionToArgArray tArray, tOptionName, tOptionValue
                  
                  -- Don't try to process the next argument as we've already consumed it
                  put true into tSkipNext
               else
                  -- Validate the argument
                  validateArgument pValidArgs, tOptionName, "long"
                  
                  -- This argument is a boolean; set it to true
                  put true into tArray[tOptionName]
               end if
            end if
         end if
      else if not tNoMoreOptions and tArg begins with "-" then
         -- If the option begins with "-" then it is a flag argument. Multiple flags may be joined.
         local tFlags
         put char 2 to -1 of tArg into tFlags
         repeat for each char tFlag in tFlags
            -- Validate this flag then add it to the argument array
            validateArgument pValidArgs, tFlag, "short"
            put true into tArray[tOptionName]
         end repeat
      else
         -- This is an input
         addValueOptionToArgArray tArray, "__inputs__", tArg
      end if
   end repeat
   
   return tArray
end arrayifyArguments

-- Formats an array nicely for debugging output purposes
private function dumpArray pArray
   return doDumpArray(pArray, 0)
end dumpArray

private function doDumpArray pArray, pDepth
   local tOutput
   local tPadding
   
   repeat with i = 1 to pDepth
      put space after tPadding
   end repeat
   
   put tPadding & "{" & return after tOutput
   repeat for each line tKey in the keys of pArray
      put tPadding && quote & tKey & quote && ":" & space after tOutput
      if pArray[tKey] is an array then
         put return & doDumpArray(pArray[tKey], pDepth+1)  after tOutput
      else if pArray[tKey] is a number then
         put pArray[tKey] & return after tOutput
      else
         put quote & pArray[tKey] & quote & return after tOutput
      end if
   end repeat
   put tPadding & "}" & return after tOutput
   return tOutput
end doDumpArray


----------------------------------------------------
---------- GIT-WRANGLING & STAGED SYMBOLS ----------
----------------------------------------------------


-- Returns the path to the symbol file for the given version tag and edition
private function pathToSymbolFileForVersion pSymbolDir, pEdition
   local tEditionCanonical
   put toUpper(char 1 of pEdition) & toLower(char 2 to -1 of pEdition) into tEditionCanonical
   return pSymbolDir & "/LiveCode-" & tEditionCanonical & ".app"
end pathToSymbolFileForVersion

-- Returns the path to the symbol directory for the given version tag
private function pathToSymbolDirForVersion pCacheDir, pVersion
   return pCacheDir & "/" & pVersion
end pathToSymbolDirForVersion

-- Downloads the symbol files for the given git tag to hte given directory
private command fetchSymbolsForTag pRepoPath, pOutputDir, pServerPath, pTag
   local tFullVersion
   local tCommitHash
   
   put commitHashForTag(pRepoPath, pTag) into tCommitHash
   verbosePrint "Commit hash for tag" && pTag && "is" && tCommitHash
   
   put fullVersionForTag(pRepoPath, pTag) into tFullVersion
   verbosePrint "Full version for tag" && pTag && "is" && quote & tFullVersion & quote
   
   fetchSymbolsForRevision pOutputDir, pServerPath, tFullVersion, tCommitHash
end fetchSymbolsForTag

-- Uses the git repo at the given path to find the commit hash for the given version tag
private function commitHashForTag pRepoPath, pTag
   -- Run git in the repo directory to get the tag
   get shell ("git --git-dir" && escapeArg(pRepoPath & "/.git") && "rev-list -n 1" && escapeArg(pTag))
   if the result is not zero then throw "unable to get commit hash for tag" && quote & pTag & quote & ":" && it
   return line 1 of it
end commitHashForTag

-- Uses the git repo at the given path to find the full version number for the given version tag
private function fullVersionForTag pRepoPath, pTag
   -- Is the repo the main one?
   local tMainRepoPath
   local tMainRepoHash
   if there is not a file (pRepoPath & "/version") then
      -- Get the commit hash for the main repo
      get shell ("git --git-dir" && escapeArg(pRepoPath & "/.git") && "rev-parse" && pTag & "^{commit}:livecode")
      if the result is not zero then throw "unable to get main repo submodule pointer:" && it
      put line 1 of it into tMainRepoHash
      put pRepoPath & "/livecode" into tMainRepoPath
   else
      put pRepoPath into tMainRepoPath
      put commitHashForTag(pRepoPath, pTag) into tMainRepoHash
   end if
   
   -- Get the contents of the version file
   local tVersionFile
   put versionFileForCommit(tMainRepoPath, tMainRepoHash) into tVersionFile
   
   -- Find the line containing the full version name
   local tVersionOffset
   put lineOffset("BUILD_LONG_VERSION", tVersionFile) into tVersionOffset
   if tVersionOffset is zero then throw "could not parse version file"
   
   -- The full version is everything after the '='
   return word 3 to -1 of line tVersionOffset of tVersionFile
end fullVersionForTag

-- Given a main repo path and main repo commit hash, returns the contents of the version file
private function versionFileForCommit pRepoPath, pCommitHash
   -- Tell git to write the contents of the version file out to stdout
   get shell ("git --git-dir" && escapeArg(pRepoPath & "/.git") && "show" && pCommitHash & ":version")
   if the result is not zero then throw "unable to get version file from git repo:" && it
   return it
end versionFileForCommit

private command getServerCredentials pURL, @rUsername, @rPassword
   -- TODO: disable echo for password
   write "Enter your username for" && pURL && ": " to stdout
   read from stdin for 1 line
   put line 1 of it into rUsername
   write "Enter your password for" && pURL && ": " to stdout
   read from stdin for 1 line
   put line 1 of it into rPassword
end getServerCredentials

-- Downloads the symbols from the (non-public!) staging area for the given version tag and commit hash
private command fetchSymbolsForRevision pOutputDir, pServerPath, pFullVersion, pCommitHash
   -- The full version number may contain spaces that need to be percent-encoded
   -- (urlEncode will replace them with '+')
   replace space with "%20" in pFullVersion
   
   -- Generate the URL for the file containing the symbols
   local tURL, tMacSymbolsZip, tCommitHashLength, tBuildNumber
   
   put char 7 to 11 of pFullVersion into tBuildNumber
   
   if pFullVersion begins with "8" then
      
      if tBuildNumber < 14023 then -- 8.0.0 until 8.1.3
         put 7 into tCommitHashLength
         put "mac-bin.tar.xz" into tMacSymbolsZip
      else if tBuildNumber >= 14023 and tBuildNumber < 14035 then -- 8.1.4 RC-1 until 8.1.6
         put 7 into tCommitHashLength
         put "universal-mac-macosx10.6-bin.tar.bz2" into tMacSymbolsZip
      else if tBuildNumber >= 14035 then -- 8.1.7 RC-1 +
         put 10 into tCommitHashLength
         put "universal-mac-macosx10.6-bin.tar.bz2" into tMacSymbolsZip
      end if
      
   else -- pFullVersion begins with "9"
      
      if tBuildNumber < 15004 then -- DP-1 until DP-4
         put 7 into tCommitHashLength
         put "mac-bin.tar.xz" into tMacSymbolsZip
      else if tBuildNumber is "15004" or tBuildNumber is "15005" then -- DP-5 and DP-6
         put 7 into tCommitHashLength
         put "mac-bin.tar.bz2" into tMacSymbolsZip
      else if tBuildNumber is "15009" or tBuildNumber is "15010" then -- DP-7 and DP-8
         put 7 into tCommitHashLength
         put "universal-mac-macosx10.9-bin.tar.bz2" into tMacSymbolsZip
      else if tBuildNumber >= 15012 then -- DP-9+
         put 10 into tCommitHashLength
         put "universal-mac-macosx10.9-bin.tar.bz2" into tMacSymbolsZip
      end if
      
   end if
   
   put pServerPath & "/" & pFullVersion & "/g" & char 1 to tCommitHashLength of pCommitHash & slash & tMacSymbolsZip into tURL
   
   -- Create the output folder, if required
   if there is not a folder pOutputDir then
      create folder pOutputDir
   end if
   
   -- Download the file
   local tOutputFile
   put pOutputDir & slash & tMacSymbolsZip into tOutputFile
   if there is not a file tOutputFile then
      verbosePrint "Downloading" && tURL && "to" && tOutputFile
      
      -- User interaction is needed here to get the user-name and password for the staging server
      local tUsername
      local tPassword
      getServerCredentials tURL, tUsername, tPassword
      
      local tCurlCmd
      put ("curl" && escapeArg(tURL) && "-o" && escapeArg(tOutputFile) && "--user" && escapeArg(tUsername) & ":" & escapeArg(tPassword)) into tCurlCmd
      debugPrint "Running curl:" && tCurlCmd
      get shell (tCurlCmd)
      if the result is not zero then
         throw "failed to download packaged symbols:" && it
      end if
   else
      verbosePrint "Symbols package already present at" && tOutputFile
   end if
   
   -- Unpack the symbols
   if there is not a folder (pOutputDir & "/LiveCode-Community.app") then
      -- Unpack the archived symbols
      local tDefaultFolder
      verbosePrint "Unpacking symbols"
      put the defaultFolder into tDefaultFolder
      set the defaultFolder to pOutputDir
      get shell ("tar --strip-components 1 -xf " && tMacSymbolsZip)
      if the result is not zero then 
         throw "unpacking staged symbols failed:" && it
      end if
      set the defaultFolder to tDefaultFolder
   else
      -- Symbols are already unpacked
      verbosePrint "Symbols are already unpacked"
   end if
end fetchSymbolsForRevision


-----------------------------------
---------- SYMBOLICATION ----------
-----------------------------------

private function findLineBeginningWith pWord, pLog
   local tFoundLineNumber, tCurrentLine, tNextLine
   put lineOffset(pWord, pLog) into tNextLine
   repeat while tNextLine is not 0
      add tNextLine to tCurrentLine
      if line tCurrentLine of pLog begins with pWord then 
      	put tCurrentLine into tFoundLineNumber
      	exit repeat
      end if
      put lineOffset(pWord, pLog, tCurrentLine) into tNextLine
   end repeat
   return tFoundLineNumber
end findLineBeginningWith

-- Guesses the LiveCode engine version and edition from the crash log
private function guessVersionAndEdition pLog, @rVersion, @rEdition
   -- Find the line that contains the embedded version information
   local tVersionLineNumber
   put findLineBeginningWith("Version:", pLog) into tVersionLineNumber
   if tVersionLineNumber is zero then return false
   
   -- Version line has the form "Version: 8.0.0.13012 [DP 12]"
   local tVersionLine
   local tVersionNumber
   local tVersionSuffix
   put line tVersionLineNumber of pLog into tVersionLine
   if (char 2 to -1 of word 3 of tVersionLine) is among the items of "RC,DP" then
      put "-" & toLower(char 2 to -1 of word 3 of tVersionLine) into tVersionSuffix
      put "-" & (char 1 to -2 of word 4 of tVersionLine) after tVersionSuffix
   end if
   set the itemDelimiter to "."
   put (item 1 to 3 of (word 2 of tVersionLine)) into tVersionNumber
   put (tVersionNumber & tVersionSuffix) into rVersion
   verbosePrint "Auto-detected version as" && rVersion
   
   -- Find the line that contains the process name
   local tProcessLineNumber
   put findLineBeginningWith("Process:", pLog) into tProcessLineNumber
   if tProcessLineNumber is zero then return false
   
   -- Look for keywords in the line
   local tProcessLine
   put line tProcessLineNumber of pLog into tProcessLine
   
   switch
      case "LiveCode-Community" is among the words of tProcessLine
         put "community" into rEdition
         break
         
      case "LiveCode-Indy" is among the words of tProcessLine
         put "indy" into rEdition
         break
         
      case "LiveCode-Business" is among the words of tProcessLine
         put "business" into rEdition
         break
         
      default
         return false
         break
   end switch
   
   -- The autodetection seems to have worked
   verbosePrint "Auto-detected edition as" && rEdition
   return true
end guessVersionAndEdition

-- Finds the line range in the given log that corresponds to the stack trace of the crashed thread
private command findCrashStackTraceLineRange pLog, @rStart, @rEnd
      -- The stack trace begins after a line with "Thread N Crashed:"
   local tStackTraceLineNumber
   put lineOffset("Crashed:", pLog) into tStackTraceLineNumber
   if tStackTraceLineNumber is zero then throw "Could not find stack trace"
   
   -- Count the number of lines in the stack trace
   -- They are numbered and we use this numbering to detect twhich lines belong to the trace
   local tStackTraceLineCount
   repeat while word 1 of line (tStackTraceLineNumber+tStackTraceLineCount+1) of pLog is tStackTraceLineCount
      put tStackTraceLineCount + 1 into tStackTraceLineCount
   end repeat
   
   put tStackTraceLineNumber+1 into rStart
   put tStackTraceLineNumber+tStackTraceLineCount into rEnd
end findCrashStackTraceLineRange

-- Takes an OSX crash log and finds the addresses needing symbolicated
function addressesFromCrashLog pLog
   -- Find the range that represents the stack trace needing symbolicated
   local tStackTraceStart
   local tStackTraceEnd
   findCrashStackTraceLineRange pLog, tStackTraceStart, tStackTraceEnd
   
   -- Extract just the stack trace then find the addresses in the trace
   local tStackTrace
   put line tStackTraceStart to tStackTraceEnd of pLog into tStackTrace
   
   local tOutput
   repeat for each line tLine in tStackTrace
      -- The address is word 3 of the log
      put word 3 of tLine & return after tOutput
   end repeat
   
   return tOutput
end addressesFromCrashLog

-- Finds the line range in the given log that corresponds to the stack trace
-- of the 
private command findHangStackTraceLineRange pLog, @rStart, @rEnd
      -- The stack trace begins after a line with "Thread N Crashed:"
   local tStackTraceLineNumber
   put lineOffset("Heaviest", pLog) into tStackTraceLineNumber
   if tStackTraceLineNumber is zero then throw "Could not find stack trace"
   
   -- Count the number of lines in the stack trace
   -- Just keep going until there is a blank line
   local tStackTraceLineCount
   
   repeat while word 1 of line (tStackTraceLineNumber+tStackTraceLineCount+1) of pLog is not empty
      put tStackTraceLineCount + 1 into tStackTraceLineCount
   end repeat
   
   put tStackTraceLineNumber+1 into rStart
   put tStackTraceLineNumber+tStackTraceLineCount into rEnd
end findHangStackTraceLineRange

-- Takes an OSX hang report and finds the addresses needing symbolicated
function addressesFromHangReport pLog
   -- Find the range that represents the stack trace needing symbolicated
   local tStackTraceStart
   local tStackTraceEnd
   findHangStackTraceLineRange pLog, tStackTraceStart, tStackTraceEnd
   
   -- Extract just the stack trace then find the addresses in the trace
   local tStackTrace
   put line tStackTraceStart to tStackTraceEnd of pLog into tStackTrace
   
   local tOutput
   repeat for each line tLine in tStackTrace
      -- The address is enclosed in square brackets as word -1 of each line
      put char 2 to -2 of word -1 of tLine & return after tOutput
   end repeat
   
   return tOutput
end addressesFromHangReport

-- Finds the addresses needing symbolicated from the given log
function addressesFromLog pLog
	if sHangReport then
		return addressesFromHangReport(pLog)
	else
		return addressesFromCrashLog(pLog)
	end if
end addressesFromLog

-- Finds the load address of the LiveCode executable in the given crash log
function loadAddressFromLog pLog
   -- Find the line containing the "Binary Images:" header
   local tHeaderOffset
   put lineOffset("Binary Images:", pLog) into tHeaderOffset
   if tHeaderOffset is zero then throw "Couldn't find load addresses list"
   
   -- Find the line containing the LiveCode IDE identifier ("com.runrev.livecode")
   local tExeLineOffset, tCheckAppID
   put kLiveCodeAppId into tCheckAppId
   if not sHangReport then
      put "+" before tCheckAppId
   end if
   
   repeat with i = tHeaderOffset+1 to the number of lines of pLog
      -- Format of lines is "0xXXXX - 0xYYYY +com.runrev.livecode"
      if word 4 of line i of pLog is tCheckAppId then exit repeat
   end repeat
   if i > the number of lines of pLog then throw "Could not find LiveCode executable's load offset"
   put i into tExeLineOffset
   
   return word 1 of line tExeLineOffset of pLog
end loadAddressFromLog

private function escapeArg pArg
   replace "'" with "\'" in pArg
   return "'" & pArg & "'"
end escapeArg

function symbolsForAddresses pAddresses, pAppBundle, pLoadAddress
   -- Full path to the executable inside the app bundle
   local tExecutable
   get shell("/usr/libexec/PlistBuddy -c" && quote & "Print CFBundleExecutable" & quote && escapeArg(pAppBundle & "/Contents/Info.plist"))
   if the result is not zero then throw "couldn't get executable from app bundle's Plist:" && it
   put pAppBundle & "/Contents/MacOS/" & line 1 of it into tExecutable
   
   -- Feed the "atos" program a list of addresses
   local tCommand
   local tSymbols
   replace return with space in pAddresses
   --put "xcrun atos -o" && escapeArg(tExecutable) && "-l" && format("%#x", pLoadAddress) && pAddresses into tCommand
   put "xcrun atos -o" && escapeArg(tExecutable) && "-arch x86_64 " && "-l" && pLoadAddress && pAddresses into tCommand
   
   get shell(tCommand)
   if the result is not zero then throw "failed to symbolicate address list using atos:" && it
   put it into tSymbols
   
   return tSymbols
end symbolsForAddresses

function mergeSymbolsIntoCrashLog pLog, pSymbols
   -- Find the line range of the stack trace that was symbolicated
   local tStackTraceStart
   local tStackTraceEnd
   findCrashStackTraceLineRange pLog, tStackTraceStart, tStackTraceEnd
   
   -- Sanity check
   if the number of lines of pSymbols is not (tStackTraceEnd - tStackTraceStart + 1) then throw "symbolicated trace is different length to original"
   
   -- Loop through the symbols list and replace where appropriate
   repeat with i = 0 to (the number of lines of pSymbols) - 1
      -- In the original log, is this a LiveCode symbol?
      if word 2 of line (tStackTraceStart+i) of pLog is kLiveCodeAppId then
         local tSymbol
         put line (i+1) of pSymbols into tSymbol
         put tSymbol into word 4 to -1 of line (tStackTraceStart+i) of pLog
      end if
   end repeat
   
   -- Return the modified log
   return pLog
end mergeSymbolsIntoCrashLog

function mergeSymbolsIntoHangReport pLog, pSymbols
   -- Find the line range of the stack trace that was symbolicated
   local tStackTraceStart
   local tStackTraceEnd
   findHangStackTraceLineRange pLog, tStackTraceStart, tStackTraceEnd
   
   -- Sanity check
   if the number of lines of pSymbols is not (tStackTraceEnd - tStackTraceStart + 1) then throw "symbolicated trace is different length to original"
   
   -- Loop through the symbols list and replace where appropriate
   repeat with i = 0 to (the number of lines of pSymbols) - 1
      -- In the original log, is this a LiveCode symbol?
      if line (tStackTraceStart+i) of pLog contains "LiveCode" then
         local tSymbol
         put line (i+1) of pSymbols into tSymbol
         put tSymbol into word 2 of line (tStackTraceStart+i) of pLog
      end if
   end repeat
   
   -- Return the modified log
   return pLog
end mergeSymbolsIntoHangReport

function mergeSymbolsIntoLog pLog, pSymbols
   if sHangReport then
      return mergeSymbolsIntoHangReport(pLog, pSymbols)
   else
      return mergeSymbolsIntoCrashLog(pLog, pSymbols)
   end if
end mergeSymbolsIntoLog

function symbolicateLog pLog, pAppBundle
   -- Get the load address for the LiveCode executable
   local tLoadAddress
   put loadAddressFromLog(pLog) into tLoadAddress
   
   -- Symbolicate the stack trace for the crashed thread
   local tAddresses
   local tSymbols
   local tSymbolicatedLog
   put addressesFromLog(pLog) into tAddresses
   put symbolsForAddresses(tAddresses, pAppBundle, tLoadAddress) into tSymbols
   put mergeSymbolsIntoLog(pLog, tSymbols) into tSymbolicatedLog
   
   return tSymbolicatedLog
end symbolicateLog
