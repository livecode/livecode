script "PackageCompiler"
################################################################################

local sCurrentTempFolder

-- Create a new compiler object, represented as an array
command packageCompilerCreate @rCompiler
   put empty into rCompiler
end packageCompilerCreate

-- Destroy the given compiler object, ensuring any intermediate
-- resources are deleted.
command packageCompilerDestroy @self
   put empty into self
end packageCompilerDestroy

-- Set compiler options:
--    temporary: the folder to use for temporary files
--    sign.certificate: the code-signing certificate to use
--    sign.privatekey: the code-signing private key to use
--    sign.password: the passphrase to use to decode the private key
command packageCompilerConfigure @self, pOption, pValue
   put pValue into self["options"][pOption]
end packageCompilerConfigure

-- Add a source folder to the compiler. Files from these
-- sources are referenced using:
--    pTag ':' pFile
-- A source can also be 'based on' another source. If the 'based on' parameter is set to another
-- source, then any files present in the base source that are used from the new source will be
-- derived via bsdiff.
command packageCompilerConfigureSource @self, pTag, pFolder, pBasedOn
   abstractCanonicalize pFolder
   
   put pFolder into self["sources"][pTag]

   compilerProgress self, "Added source for '" & pTag & "': '" & pFolder & "'"
   
   -- If the folder doesn't exist, but a prefix of the folder path is a file
   -- then register that prefix as abstract.
   if there is no folder pFolder then
      set the itemDelimiter to slash
      repeat with i = the number of items in pFolder - 1 down to 2
         if there is a file (item 1 to i of pFolder) then
            abstractAddPath item 1 to i of pFolder
            exit repeat
         end if
      end repeat
      set the itemDelimiter to comma
   end if
   
   if pBasedOn is not empty then
      throw "not implemented"
   end if
   
   if pBasedOn is not pTag then
      put pBasedOn into self["base_sources"][pTag]
   end if
end packageCompilerConfigureSource

-- Configure a top-level variable in the compiler. These
-- variables are substituted into [[...]] clauses in the commands
command packageCompilerConfigureVariable @self, pName, pValue
   put pValue into self["default context"][pName]
end packageCompilerConfigureVariable

-- Configure a callback for status messages.
command packageCompilerSetReportCallback @self, pObject, pHandler
   put pObject into self["report.target"]
   put pHandler into self["report.handler"]
end packageCompilerSetReportCallback

-- Parse the given script
command packageCompilerParse @self, pScript
   try
      compilerParse self, pScript
   catch tException
   end try
   if tException is not empty and the number of lines of tException is not 1 then
      throw tException
   end if
   return tException
end packageCompilerParse

-- Build the given installer component
command packageCompilerBuild @self, pInstaller, pOutput
   -- The files array will be an array with keys of the form:
   --   <process>':'<abs path>
   -- Mapping to the item name in the zip archive. Here process is what (if anything) should
   -- happen to the source file before being placed into the zip:
   --   none: use it as is
   --   text: map any line endings to appropriate form for target platform
   --   slim-i386: extract the i386 part of a mac os x exe, and strip it
   --   slim-ppc: extract the ppc part of a mac os x exe, and strip it
   --
   -- The manifest will be a return-delimited list of entries detailing where files in the zip are
   -- to be placed:
   --   <type> [tab] <zip item> [tab] <target>
   -- Here <type> is either file, folder or exe. For folders, the <zip item> is ignored. For exes
   -- a 'chmod' is done to the resulting file if required for that platform.
   --
   -- The groups key is used to list items such as externals and dbdrivers that 
   -- need lists generated. It is modified by the declare/emit commands.
   --
   put empty into self["files"]
   put empty into self["manifest"]
   put empty into self["stack"]
   put empty into self["warnings"]
   put self["default context"] into self["context"]
   put pOutput into self["output"]
   try
      put self["options"]["temporary"] into sCurrentTempFolder
      compilerRun self, pInstaller
   catch tException
   end try
   
   -- If the exception is not one of ours, then throw it. Otherwise we return.
   if tException is not empty then
      if the number of lines of tException is not 1 then
         throw tException
      end if
      return tException
   end if
   
   local tArchive, tTmpFiles
   
   -- TODO: Normalize manifest by adding missing folders
   compilerNormalizeManifest self
   
   -- Open a zip type archive
   compilerProgress self, "Opening zip archive '" & pOutput & "'"
   archiveOpenZip tArchive, pOutput
   
   -- Compute the list of used files
   local tUsedFiles
   repeat for each element tFile in self["files"]
      put true into tUsedFiles[tFile]
   end repeat
   
   -- Process the manifest to eliminate any patches that rely on unused files
   local tManifest
   set the itemDelimiter to  tab
   repeat for each line tLine in self["manifest"]
      if item 1 of tLine is among the words of "file executable" and \
            item 4 of tLine is not empty then
         if not tUsedFiles[item 4 of tLine] then
            delete item 4 of tLine
         end if
      end if
      put tLine & return after tManifest
   end repeat
   set the itemDelimiter to comma
   
   -- Add  the manifest file, this simply goes at the root
   archiveAddItemWithData tArchive, "manifest.txt", compilerProcessText(self, tManifest)
   
   -- Now loop through all the files
   local tSourceFiles, tItemFiles
   repeat for each key tItem in self["files"]
      local tItemProcess, tItemFile
      set the itemDelimiter to ":"
      put item 1 of tItem into tItemProcess
      put item 2 to -1 of tItem into tItemFile
      set the itemDelimiter to comma
      
      -- Fetch the file to base this item on (if any). Notice that we ignore base references
      -- if the base file itself is not used directly.
      -- TODO: Make sure this resolves to the final 'base' file since we don't support
      --   cascaded patches yet.
      local tBaseItem, tBaseItemProcess, tBaseItemFile
      if self["base_files"][tItem] is not empty and self["files"][self["base_files"][tItem]] is not empty then
         put self["base_files"][tItem] into tBaseItem
         set the itemDelimiter to ":"
         put item 1 of tBaseItem into tBaseItemProcess
         put item 2 to -1 of tBaseItem into tBaseItemFile
         set the itemDelimiter to comma
      else
         put empty into tBaseItem
         put empty into tBaseItemFile
      end if
      
      -- First check to see if the item has been processed.
      local tDerivedFile
      if tItemFiles[tItem] is empty then
         put compilerProcessItem(self, tItemProcess, tItemFile) into tDerivedFile
         put tDerivedFile into tItemFiles[tItem]
         if tItemFile is not tDerivedFile then
            put tDerivedFile & return after tTmpFiles
         end if
      end if
      
      -- Now check to see if the base item, if not empty, has been processed
      if tBaseItem is not empty and tItemFiles[tBaseItem] is empty then
         put compilerProcessItem(self, tBaseItemProcess, tBaseItemFile) into tDerivedFile
         put tDerivedFile into tItemFiles[tBaseItem]
         if tBaseItemFile is not tDerivedFile then
            put tDerivedFile & return after tTmpFiles
         end if
      end if
      
      -- Finally either compute a diff, or just add the file directly
      if tBaseItem is empty then
         archiveAddItemWithFile tArchive, self["files"][tItem], tItemFiles[tItem]
      else if tItemFiles[tBaseItem] is not empty and tItemFiles[tItem] is not empty then
         put compilerMakeTemporaryFile(self, "diff") into tDerivedFile
         put tDerivedFile & return after tTmpFiles
         compilerProgress self, "Diffing '" & tItemFiles[tBaseItem] &"' and '" & tItemFiles[tItem] & "'"
         _internal bsdiff abstractPinFile(tItemFiles[tBaseItem]) to abstractPinFile(tItemFiles[tItem]) into tDerivedFile
         archiveAddItemWithFile tArchive, self["files"][tItem], tDerivedFile
      end if
   end repeat
   
   -- Now loop through the derived files
   repeat for each element tDerivedItem in self["derived"]
      archiveAddItemWithData tArchive, tDerivedItem["name"], compilerProcessText(self, tDerivedItem["data"])
   end repeat
   
   -- Close and create the final archive
   local tError
   compilerProgress self, "Closing zip archive"
   archiveClose tArchive
   if the result is not empty then
      put "error: archive: " & the result into tError
   end if
   
   -- Remove any temporary files generated in the above process
   if self["options"]["temporary"] is empty then
      repeat for each line tFile in tTmpFiles
         delete file tFile
      end repeat
   end if
   
   -- Return the error, if any
   return tError
end packageCompilerBuild

function packageCompilerGetWarnings @self
   return self["warnings"]
end packageCompilerGetWarnings

################################################################################

-- Parsing builds a tree of components, and flattens compound commands
-- such as 'with' and 'into'.
private command compilerParse @self, pScript
   -- This stores the current list of contexts, one per line. The currently
   -- supported contexts are:
   --   installer:
   --      a top-level aggregation of components
   --   component:
   --      a collection of commands that can be 'included'
   --   with:
   --      changes the value of a variable for a given set of commands 
   --   into:
   --      sets a default install target for a given set of commands
   --
   local tContext
   put empty into tContext
   
   local tComponentName, tComponentType, tComponentCommands, tComponentCommandCount
   put empty into tComponentName
   put empty into tComponentType
   put empty into tComponentCommands
   put 0 into tComponentCommandCount
   
   local tLineNumber
   put 0 into tLineNumber
   
   repeat for each line tLine in pScript
      -- Increment the line number
      add 1 to tLineNumber
      
      -- Ignore comment lines
      if word 1 of tLine begins with "//" or \
            word 1 of tLine begins with "--" or \
            word 1 of tLine begins with "#" then
         next repeat
      end if
      
      -- Ignore empty lines
      if word 1 to -1 of tLine is empty then
         next repeat
      end if
      
      -- Determine the depth by counting tab characters
      local tDepth
      put 0 into tDepth
      repeat while the first char of tLine is tab
         add 1 to tDepth
         delete char 1 of tLine
      end repeat
      
      -- If the depth is greater than the current context depth, it is an error
      if tDepth > the number of lines of tContext then
         compilerSyntaxError self, "Invalid depth for command", tLineNumber
      end if
      
      -- Pop context until we reach the appropriate depth
      repeat while tDepth < the number of lines of tContext
         delete the last line of tContext
         put "pop" into tComponentCommands[tComponentCommandCount]["type"]
         put tLine into tComponentCommands[tComponentCommandCount]["line content"]
         put tLineNumber into tComponentCommands[tComponentCommandCount]["line"]
         add 1 to tComponentCommandCount
      end repeat
      
      -- If we are back at top-level context, we are done processing the current
      -- installer/component, so enter the command list into self
      if tContext is empty then
         put tComponentCommands into self[tComponentType][tComponentName]
         put empty into tComponentName
         put empty into tComponentType
         put empty into tComponentCommands
         put 0 into tComponentCommandCount
      end if
      
      -- Strip any whitespace from the line
      put word 1 to -1 of tLine into tLine
      
      -- Check to see if depth is appropriate for the command
      if word 1 of tLine is among the items of "installer,component" then
         if tContext is not empty then
            compilerSyntaxError self, "'Command only allowed at top-level", tLineNumber
         end if
      else
         if tContext is empty then
            compilerSyntaxError self, "Command not allowed at top-level", tLineNumber
         end if
      end if
      
      -- Now process the commands, depending on the first word
      local tCommand, tNeedPush
      put false into tNeedPush
      put empty into tCommand
      put tLine into tCommand["line content"]
      put tLineNumber into tCommand["line"]
      switch word 1 of tLine
         case "installer"
         case "component"
            -- Ensure there are only two words
            if the number of words of tLine is not 2 then
               compilerSyntaxError self, "Invalid command, syntax is '" && (word 1 of tLine) && "<name>'", tLineNumber
            end if
            
            put word 1 of tLine & return after tContext
            put word 1 of tLine & "s" into tComponentType
            put wordToName(word 2 of tLine) into tComponentName
            
            -- Check that the component doesn't already exist
            if tComponentName is among the keys of self[tComponentType][tComponentName] then
               compilerSyntaxError self, "Component already '" & tComponentName & "' already declared", tLineNumber
            end if
            
            -- Starting a new component means an implicit push
            put true into tNeedPush
            
            -- And resetting of the RootFolder
            put "set" into tCommand["type"]
            put "RootFolder" into tCommand["variable"]
            put empty into tCommand["value"]
            break
         case "additional"
         	-- Ensure that the following word is "manifest" and that there is
         	-- a final word containing the file to use
         	if the number of words of tLine is not 3 or word 2 of tLine is not "manifest" then
         	   compilerSyntaxError self, "Invalid command, syntax is 'additional manifest <file>'", tLineNumber
         	end if
         	
         	-- Get the manifest file name and parse it too
         	local tExtraManifest
         	local tExtraManifestName
         	put word 3 of tLine into tExtraManifestName
         	if char 1 of tExtraManifestName is quote and char -1 of tExtraManifestName is quote then
         		put char 2 to -2 of tExtraManifestName into tExtraManifestName
         	end if
         	put url ("file:" & builderRepoFolder() & slash & tExtraManifestName) into tExtraManifest
         	if the result is not empty then
         	   compilerSyntaxError self, "Could not open additional manifest" && builderRepoFolder() & slash & tExtraManifestName, tLineNumber
         	end if
         	try
         		compilerParse self, tExtraManifest
         	catch tException
         		-- Rethrow with some additional context information
         		throw "In additional manifest" && tExtraManifestName & ":" && tException
         	end try
         	
         	-- Note that no executable commands are added beyond those that
         	-- were added from the manifest file that was just included
         	put "nop" into tCommand["type"]
         	break
         case "include"
            -- Ensure that there are two words
            if the number of words of tLine is not 2 then
               compilerSyntaxError self, "Invalid command, syntax is 'include <name>'", tLineNumber
            end if
            
            -- Add the command
            put "include" into tCommand["type"]
            put wordToName(word 2 of tLine) into tCommand["name"]
            break
         case "set"
            -- Ensure that there are four words and the form is correct
            if the number of words of tLine is not 4 or \
                  word 3 of tLine is not "to" then
               compilerSyntaxError self, "Invalid command, syntax is 'set <var> is <value>'", tLineNumber
            end if
            
            -- Add the command
            put "set" into tCommand["type"]
            put wordToName(word 2 of tLine) into tCommand["variable"]
            put wordToName(word 4 of tLine) into tCommand["value"]
            break
         case "codesign"
            -- Ensure that there is only one word
            if the number of words of tLine is not 1 then
               compilerSyntaxError self, "Invalid command, syntax is 'codesign'", tLineNumber
            end if
            
            -- This command places a new context
            put "with" & return after tContext
            
            -- It also implies an implicit push
            put true into tNeedPush
            
            -- Otherwise it is just a special case of 'set'
            put "set" into tCommand["type"]
            put "SignExecutables" into tCommand["variable"]
            put "true" into tCommand["value"]
            break
         case "with"
            -- Ensure that there are 5 words, and the form is correct
            if the number of words of tLine is not 5 or \
                  word 3 of tLine is not "as" or \
                  word 5 of tLine is not "do" then
               compilerSyntaxError self, "Invalid command, syntax is 'with <var> as <value> do'", tLineNumber
            end if
            
            -- This command places a new context
            put "with" & return after tContext
            
            -- It also implies an implicit push
            put true into tNeedPush
            
            -- Otherwise, its just a set
            put "set" into tCommand["type"]
            put wordToName(word 2 of tLine) into tCommand["variable"]
            put wordToName(word 4 of tLine) into tCommand["value"]
            break
         case "into"
            -- Ensure that there are 3 words and the form is correct
            if the number of words of tLine is not 3 or \
                  word 3 of tLine is not "place" then
               compilerSyntaxError self, "Invalid command, syntax is 'into <folder> place'", tLineNumber
            end if
            
            -- This command places a new context
            put "into" & return after tContext
            
            -- It also implies an implicit push
            put true into tNeedPush
            
            -- Otherwise its just a set of the RootFolder variable
            put "set" into tCommand["type"]
            put "RootFolder" into tCommand["variable"]
            put wordToName(word 2 of tLine) into tCommand["value"]
            break
         case "executable"
         case "stack"
         case "textfile"
         case "file"
         case "folder"
         case "rfolder"
            -- Ensure that there are either 2 words or 4 words, and the form is correct
            if the number of words of tLine is not among the items of "2,4,6" or \
                  the number of words of tLine is 4 and word 3 of tLine is not "as" or \
                  the number of words of tLine is 6 and word 5 of tLine is not "base" then
               compilerSyntaxError self, "Invalid command, syntax is '" && word 1 of tLine && "<source> [ as <target> [ base <base-source> ] ]'", tLineNumber
            end if
            
            -- Put together the command
            put "copy" into tCommand["type"]
            put word 1 of tLine into tCommand["class"]
            put wordToName(word 2 of tLine) into tCommand["source"]
            if the number of words of tLine >= 4 then
               put wordToName(word 4 of tLine) into tCommand["target"]
            end if
            if the number of words of tLine >= 6 then
               put wordToName(word 6 of tLine) into tCommand["source-base"]
            end if
            break
         case "emit"
            local tClass
            put wordToName(word 2 of tLine) into tClass
            
            local tMaxWords
            switch tClass
            case "externals"
            case "dbdrivers"
               delete the last char of tClass
               put 4 into tMaxWords
               break
            case "string"
            case "variable"
               put 5 into tMaxWords
               put wordToName(word 3 of tLine) into tCommand["name"]
               break
            default
               put 0 into tMaxWords
            end switch
            
            if the number of words of tLine is not tMaxWords or \
                  word -2 of tLine is not "to" then
               compilerSyntaxError self, "Invalid command, syntax is 'emit <class> [{<variable> | <string>}] to <target>'", tLineNumber
            end if
            
            put "emit" into tCommand["type"]
            put tClass into tCommand["class"]
            put wordToName(word -1 of tLine) into tCommand["target"]
            break
         case "declare"
            if the number of words of tLine is not 5 or \
                  word 2 of tLine is not among the items of "external,dbdriver" or \
                  word 4 of tLine is not "using" then
               compilerSyntaxError self, "Invalid command, syntax is 'declare external <name> using <filelist>'", tLineNumber
            end if
            
            put "declare" into tCommand["type"]
            put wordToName(word 2 of tLine) into tCommand["class"]
            put wordToName(word 3 of tLine) into tCommand["name"]
            put wordToName(word 5 of tLine) into tCommand["files"]
            break
         case "if"
         case "ifnot"
            if not (the number of words of tLine is 5 and word 3 of tLine is "is" and word 5 of tLine is "then") and \
                  not (the number of words of tLine is 4 and word 3 of tLine is "exists" and word 4 of tLine is "then") then
               compilerSyntaxError self, "Invalid command, syntax is 'if[not] <name> ( is <name> | exists ) then'", tLineNumber
            end if
            
            -- This command places a new context
            put "if" & return after tContext
            
            -- It also implies an implicit push
            put true into tNeedPush
            
            -- Add the command
            put "if" into tCommand["type"]
            if word 1 of tLine is "ifnot" then
               put true into tCommand["negated"]
            end if
            put wordToName(word 2 of tLine) into tCommand["left"]
            if the number of words of tLine is 5 then
               put wordToName(word 4 of tLine) into tCommand["right"]
            end if
            break
         case "shortcut"
            if not (the number of words of tLine is 4 and word 3 of tLine is "to") and \
                  not the number of words of tLine is 2  then
               compilerSyntaxError self, "Invalid command, syntax is 'shortcut <name> [ to <target> ]'", tLineNumber
            end if
            
            put "shortcut" into tCommand["type"]
            put wordToName(word 2 of tLine) into tCommand["source"]
            if the number of words of tLine is 4 then
               put wordToName(word 4 of tLine) into tCommand["target"]
            end if
            break
         case "place"
            if the number of words of tLine is not 4 or \
                  word 2 of tLine is not among the items of "record,uninstaller" or \
                  word 3 of tLine is not "at" then
               compilerSyntaxError self, "Invalid command, syntax is 'place (record|uninstaller) at <target>'", tLineNumber
            end if
            
            put "place" into tCommand["type"]
            put word 2 of tLine into tCommand["class"]
            put wordToName(word 4 of tLine) into tCommand["target"]
            break
         case "register"
            if not (the number of words of tLine is 3 and word 2 of tLine is "key") and \
                  not (the number of words of tLine is 5 and word 2 of tLine is among the words of "key value" and word 4 of tLine is "as") and \
                  not (the number of words of tLine is 7 and word 2 of tLine is "value" and word 4 of tLine is "of" and word 6 of tLine is "as") and \
                  not (the number of words of tLine is 6 and word 2 of tLine is among the words of "key value" and word 4 of tLine is "as" and word 5 of tLine is among the words of "string path number") and \
                  not (the number of words of tLine is 8 and word 2 of tLine is "value" and word 4 of tLine is "of" and word 6 of tLine is "as" and word 7 of tLine is among the words of "string path number") then
               compilerSyntaxError self, "Invalid command, syntax is 'register (value | key) <key_or_value> [ of <key> ] [ as <value> ]'", tLineNumber
            end if
            
            put "register" into tCommand["type"]
            put word 2 of tLine into tCommand["class"]
            put wordToName(word 3 of tLine) into tCommand["target"]
            
            local tRegistryValue, tRegistryType
            if the number of words of tLine is 5 then
               put word 5 of tLine into tRegistryValue
               put empty into tRegistryType
            else if the number of words of tLine is 6 then
               put word 6 of tLine into tRegistryValue
               put word 5 of tLine into tRegistryType
            else if the number of words of tLine is 7 then
               put word 7 of tLine into tRegistryValue
               put empty into tRegistryType
               put wordToName(word 5 of tLine) into tCommand["parent"]
            else if the number of words of tLine is 8 then
               put word 8 of tLine into tRegistryValue
               put word 7 of tLine into tRegistryType
               put wordToName(word 5 of tLine) into tCommand["parent"]
            end if
            
            if tRegistryType is empty  then
               if char 1 of tRegistryValue is quote or tRegistryValue is not an integer then
                  put "string" into tCommand["value_type"]
                  put wordToName(tRegistryValue) into tCommand["value"]
               else 
                  put "number" into tCommand["value_type"]
                  put tRegistryValue into tCommand["value"]
               end if
            else
               put tRegistryType into tCommand["value_type"]
               put wordToName(tRegistryValue) into tCommand["value"]
            end if
            break
         case "rename"
            if the number of words of tLine is not 4 or word 3 of tLine is not "to" then
               compilerSyntaxError self, "Invalid command, syntax is 'rename <source> to <target>'", tLineNumber
            end if
            
            put "rename" into tCommand["type"]
            put wordToName(word 2 of tLine) into tCommand["source"]
            put wordToName(word 4 of tLine) into tCommand["target"]
            break
         case "desktop"
            if the number of words of tLine is not 5 then
               compilerSyntaxError self, "Invalid command, syntax is 'desktop (application | icon) <source> as <target>'", tLineNumber
            end if
            
            put "desktop" into tCommand["type"]
            put word 2 of tLine into tCommand["class"]
            put wordToName(word 3 of tLine) into tCommand["source"]
            put wordToName(word 5 of tLine) into tCommand["target"]
            break
         default
            compilerSyntaxError self, "Unknown command type -" && word 1 of tLine, tLineNumber
            break
      end switch
      
      if tNeedPush then
         put "push" into tComponentCommands[tComponentCommandCount]["type"]
         put tLine into tComponentCommands[tComponentCommandCount]["line content"]
         put tLineNumber into tComponentCommands[tComponentCommandCount]["line"]
         add 1 to tComponentCommandCount
      end if
      
      if tCommand is an array then
         put tLineNumber into tCommand["line"]
         put tCommand into tComponentCommands[tComponentCommandCount]
         add 1 to tComponentCommandCount
      end if
   end repeat
   
   if tComponentName is not empty then
      -- Make sure we unwind the context stack for the last component
      repeat while tContext is not empty
         delete the last line of tContext
         put "pop" into tComponentCommands[tComponentCommandCount]["type"]
         add 1 to tComponentCommandCount
      end repeat
      
      put tComponentCommands into self[tComponentType][tComponentName]
   end if
end compilerParse

################################################################################

private command compilerRun @self, pInstaller
   if pInstaller is not among the keys of self["installers"] then
      compilerExecutionError self, "Unknown installer - '" & pInstaller & "'"
   end if
   compilerExecute self, self["installers"][pInstaller]
end compilerRun

private command compilerExecute @self, pCommands
   local tCommand 
   repeat with tCommandIndex = 0 to (the number of elements of pCommands-1)
      put pCommands[tCommandIndex] into tCommand
      -- If we are ignoring commands at the moment (as a result of an if)
      -- then skip. Note we always execute pushes and pops to ensure
      -- nesting is sound.
      if self["context"]["__IGNORE__"] and tCommand["type"] is not among the items of "push,pop" then
         next repeat
      end if
      
      if false then
         compilerProgress self, "TRACE: Op =" && tCommand["type"] && "(line" && tCommand["line"] && colon && tCommand["line content"] & ")"
      end if
      
      switch tCommand["type"]
         case "push"
            compilerExecutePush self, tCommand
            break
         case "pop"
            compilerExecutePop self, tCommand
            break
         case "include"
            compilerExecuteInclude self, tCommand
            break
         case "set"
            compilerExecuteSet self, tCommand
            break
         case "copy"
            compilerExecuteCopy self, tCommand
            break
         case "create"
            compilerExecuteCreate self, tCommand
            break
         case "emit"
            compilerExecuteEmit self, tCommand
            break
         case "declare"
            compilerExecuteDeclare self, tCommand
            break
         case "if"
            compilerExecuteIf self, tCommand
            break
         case "shortcut"
            compilerExecuteShortcut self, tCommand
            break
         case "place"
            compilerExecutePlace self, tCommand
            break
         case "register"
            compilerExecuteRegister self, tCommand
            break
         case "rename"
            compilerExecuteRename self, tCommand
            break
         case "desktop"
            compilerExecuteDesktop self, tCommand
            break
         case "nop"
         	break
         default
            throw "Unknown command -" && tCommand["type"]
      end switch
   end repeat
end compilerExecute

private command compilerExecutePush @self, pCommand
   get the number of elements of self["stack"]
   put self["context"] into self["stack"][it]
end compilerExecutePush

private command compilerExecutePop @self, pCommand
   get the number of elements of self["stack"]
   if it is zero then
      compilerExecutionError self, "Attempt to pop context when stack is empty (" & quote & pCommand["line content"] & quote && "on line" && pCommand["line"] & ")"
   end if
   put self["stack"][it - 1] into self["context"]
   delete variable self["stack"][it - 1]
end compilerExecutePop

private command compilerExecuteInclude @self, pCommand
   local tComponent
   put compilerEvaluate(self, pCommand["name"], pCommand["line"]) into tComponent
   
   if tComponent is not among the keys of self["components"] then
      compilerExecutionError self, "Unknown component '" & tComponent & "'", pCommand["line"]
   end if
   
   compilerExecute self, self["components"][tComponent]
end compilerExecuteInclude

private command compilerExecuteSet @self, pCommand
   local tValue
   put compilerEvaluate(self, pCommand["value"], pCommand["line"]) into tValue
   put tValue into self["context"][pCommand["variable"]]
end compilerExecuteSet

private command compilerExecuteCopy @self, pCommand
   local tSource, tSourceType, tSourceFile
   put compilerEvaluate(self, pCommand["source"], pCommand["line"]) into tSource
   set the itemDelimiter to ":"
   put item 1 of tSource into tSourceType
   put item 2 to -1 of tSource into tSourceFile
   set the itemDelimiter to comma
   
   local tTarget
   if pCommand["target"] is not empty then
      put compilerEvaluate(self, pCommand["target"], pCommand["line"]) into tTarget
   else
      set the itemDelimiter to "/"
      put the last item of tSourceFile into tTarget
      set the itemDelimiter to comma
   end if
   if self["context"]["RootFolder"] is not empty then
      put self["context"]["RootFolder"] & slash before tTarget
   end if
   
   local tBaseSource, tBaseSourceFile, tBaseSourceType
   if pCommand["source-base"] is not empty then
      put compilerEvaluate(self, pCommand["source-base"], pCommand["line"]) into tBaseSource
      set the itemDelimiter to ":"
      put item 1 of tBaseSource into tBaseSourceType
      put item 2 to -1 of tBaseSource into tBaseSourceFile
      set the itemDelimiter to comma
   else
      put empty into tBaseSource
      put empty into tBaseSourceFile
      put empty into tBaseSourceType
   end if
   
   -- If the source is unknown, throw an error
   if tSourceType is not among the keys of self["sources"] then
      compilerExecutionError self, "Unknown source type - '" & tSourceType & "'", pCommand["line"]
   end if
   
   -- If the source file doesn't exist, append a warning
   local tSourcePath, tBaseSourcePath
   put self["sources"][tSourceType] & slash & tSourceFile into tSourcePath
   if self["base_sources"][tSourceType] is not empty then
      put self["sources"][self["base_sources"][tSourceType]] & slash & tSourceFile into tBaseSourcePath
   else if tBaseSource is not empty then
      put self["sources"][tBaseSourceType] & slash & tBaseSourceFile into tBaseSourcePath
   end if
   
   -- If the source is a folder then we must check that it is a bundle,
   -- otherwise it is a file. We construct the 'manifest' which, in the
   -- case of a file will be one item long, but in the case of the bundle
   -- will be determined by its manifest file.
   local tItemManifest
   if abstractThereIsAFolder(tSourcePath) then
      if pCommand["class"] is not among the items of "folder,executable,rfolder" then
         compilerBuildWarning self, "Item '" & tSourcePath & "' is a folder, but not of executable or folder type", pCommand["line"]
         exit compilerExecuteCopy
      end if
      
      -- Simply take the manifest file from within the bundle (one assumes
      -- this will be automagically processed so shouldn't be any problems).
      -- The expected syntax is:
      --    <class>,/<relativefile>
      -- (If rfolder then we generate one)
      if pCommand["class"] is among the items "executable,rfolder" then
         compilerProgress self, "Creating manifest for item '" & tSourcePath & "'"
         put createManifest(tSourcePath) into tItemManifest
      else if abstractThereIsAFile(tSourcePath & slash & "Manifest") then
         put url ("file:" & tSourcePath & slash & "Manifest") into tItemManifest
      else
         put pCommand["class"], empty into tItemManifest
      end if
   else
      if not abstractThereIsAFile(tSourcePath) then
         compilerExecutionError self, "Item '" & tSourcePath & "' could not be found", pCommand["line"]
      end if
      put pCommand["class"], empty into tItemManifest
   end if
   
   -- Work out the processing type required and any suffix required due
   -- to the processing. Note here that (for each item) the various paths
   -- are simply formed by concatenating items from the manifest with
   -- the roots.
   repeat for each line tItem in tItemManifest
      local tItemClass, tItemSourcePath, tItemSourceFile, tItemTarget, tBaseItemSourcePath
      
      if the number of items of tItem is 0 then
         next repeat
      end if
      
      put item 1 of tItem into tItemClass
      put tSourcePath & item 2 of tItem into tItemSourcePath
      put tSourceFile & item 2 of tItem into tItemSourceFile
      put tTarget & item 2 of tItem into tItemTarget
      
      if tBaseSourcePath is not empty then
         put tBaseSourcePath & item 2 of tItem into tBaseItemSourcePath
      end if
      
      if tItemClass is not "folder" and not abstractThereIsAFile(tItemSourcePath) then
         compilerExecutionError self, "Bundle item '" & tItemSourcePath & "' could not be found", pCommand["line"]
      end if
      
      local tProcess, tSuffix, tManifestType
      put empty into tSuffix
      switch tItemClass
         case "executable"
            local tExeType
            put sniffExecutable(abstractPinFile(tItemSourcePath)) into tExeType
            if tExeType is "macosx" and tSourceType is not "ios" then
               get self["context"]["TargetArchitectures"]
               sort items of it ascending
               put "slim-" & it  into tProcess
               if "i386" is among the items of it then
                  put "i" after tSuffix
               end if
               if "ppc" is among the items of it then
                  put "p" after tSuffix
               end if
               if "x86_64" is among the items of it then
                  put "a" after tSuffix
               end if
            else if tExeType is "windows" and compilerEvaluateOptVar(self, "SignExecutables") and self["options"]["sign.certificate"] is not empty then
               put "sign" into tProcess
               put "s" into tSuffix
            else
               put "none" into tProcess
            end if
            put "executable" into tManifestType
            break
         case "textfile"
            put "text" into tProcess
            put "txt" into tSuffix
            put "file" into tManifestType
            break
         case "file"
         case "stack"
            put "none" into tProcess
            put "file" into tManifestType
            break
         case "folder"
            put empty into tProcess
            put "folder" into tManifestType
            break
      end switch
      
      -- If the item is not just an (empty) folder creation, then we
      -- must reference it in the files list
      local tItemName, tBaseItemName
      if tManifestType is not "folder" then
         -- Compute the item name
         put tSourceType & slash & tItemSourceFile into tItemName
         if tSuffix is not empty then
            put "#" & tSuffix after tItemName
         end if
         
         -- Add an entry to the file list (if not a folder)
         put tItemName into self["files"][tProcess & ":" & tItemSourcePath]
         
         if tBaseSourceFile is not empty then
            -- If this was a 'based' clause then use the right files
            put tBaseSourceType & slash & tBaseSourceFile into tBaseItemName
            put tProcess & ":" & tBaseItemSourcePath into self["base_files"][tProcess & ":" & tItemSourcePath]
         else if there is a file tBaseItemSourcePath then
            -- If the file exists in the base source path, then we construct a base item name
            put self["base_sources"][tSourceType] & slash & tItemSourceFile into tBaseItemName
            if tSuffix is not empty then
               put "#" & tSuffix after tBaseItemName
            end if
            
            -- Link the new file to the base file
            put tProcess & ":" & tBaseItemSourcePath into self["base_files"][tProcess & ":" & tItemSourcePath]
         else if tProcess is "sign" then
            -- If we are signing, and we aren't basing on an older version then we base the signed item
            -- on the unsigned one.
            put "none:" & tItemSourcePath into self["base_files"][tProcess & ":" & tItemSourcePath]
            put tSourceType & slash & tItemSourceFile into tBaseItemName
         else if tProcess is "slim-i386" or tProcess is "slim-ppc" then
            -- If we are slimming to i386, then base it on i386+ppc
            put "slim-i386,ppc:" & tItemSourcePath into self["base_files"][tProcess & ":" & tItemSourcePath]
            put tSourceType & slash & tItemSourceFile & "#ip" into tBaseItemName
         else
            put empty into tBaseItemName
         end if
      else
         put empty into tItemName
         put empty into tBaseItemName
      end if
      
      -- Add an entry to the manifest
      put tManifestType & tab & tItemTarget & tab & tItemName & tab & tBaseItemName & return after self["manifest"]
   end repeat
end compilerExecuteCopy

private command compilerExecuteCreate @self, pCommand
   -- Work out the target path of the new folder
   local tTarget
   put compilerEvaluate(self, pCommand["target"], pCommand["line"]) into tTarget
   if self["context"]["RootFolder"] is not empty then
      put self["context"]["RootFolder"] & slash before tTarget
   end if
   
   -- Add an entry to the manifest
   put "folder" & tab & tTarget & return after self["manifest"]
end compilerExecuteCreate

private command compilerExecuteEmit @self, pCommand
   local tTarget
   put compilerEvaluate(self, pCommand["target"], pCommand["line"]) into tTarget
   if self["context"]["RootFolder"] is not empty then
      put self["context"]["RootFolder"] & slash before tTarget
   end if
   
   local tClass
   put pCommand["class"] into tClass
   
   local tIndex
   put the number of elements in self["derived"] + 1 into tIndex
   put "derived/" & tClass & "-" & tIndex & ".txt" into self["derived"][tIndex]["name"]
   put "text" into self["derived"][tIndex]["type"]
   
   -- allow emitting a a variable to a file
   switch tClass
   case "dbdriver"
   case "external"
      put self["groups"][tClass] into self["derived"][tIndex]["data"]
      put empty into self["groups"][tClass]
      break
   case "variable"
      put self["context"][pCommand["name"]] into self["derived"][tIndex]["data"]
      break
   case "string"
      put pCommand["name"] into self["derived"][tIndex]["data"]
      break
   end switch
   
   put "file" & tab & tTarget & tab & self["derived"][tIndex]["name"] & return after self["manifest"]
end compilerExecuteEmit

private command compilerExecuteDeclare @self, pCommand
   local tName, tFiles
   put compilerEvaluate(self, pCommand["name"], pCommand["line"]) into tName
   put compilerEvaluate(self, pCommand["files"], pCommand["line"]) into tFiles
   
   put tName, tFiles & return after self["groups"][pCommand["class"]]
end compilerExecuteDeclare

private command compilerExecuteIf @self, pCommand
   local tLeft, tRight
   
   local tCondition
   if "right" is among the keys of pCommand then
      put compilerEvaluateVar(self, pCommand["left"], pCommand["line"]) into tLeft
      put compilerEvaluate(self, pCommand["right"], pCommand["line"]) into tRight
      
      -- If the operands do not match, then we ignore the subsequent
      -- block(s)
      put tLeft is tRight into tCondition
   else
      put pCommand["left"] is among the keys of self["context"] into tCondition
   end if
   if pCommand["negated"] then
      put not tCondition into tCondition
   end if
   
   if not tCondition then
      put true into self["context"]["__IGNORE__"]
   end if
end compilerExecuteIf

private command compilerExecuteShortcut @self, pCommand
   local tSource, tTarget
   put compilerEvaluate(self, pCommand["source"], pCommand["line"]) into tSource
   if pCommand["target"] is not empty then
      put compilerEvaluate(self, pCommand["target"], pCommand["line"]) into tTarget
   end if
   
   -- Check that the given target is present in the manifest
   local tFound
   if tTarget is not empty then
      put false into tFound
      set the itemDelimiter to tab
      repeat for each line tLine in self["manifest"]
         -- See if the target manages exactly
         if item 2 of tLine is tTarget then
            put true into tFound
            exit repeat
         end if
         
         -- See if the target matches a folder prefix (e.g. .../LiveCode.app)
         if item 2 of tLine begins with (tTarget & slash) then
            put true into tFound
            exit repeat
         end if
      end repeat
      set the itemDelimiter to comma
   else
      put true into tFound
   end if
   
   if not tFound then
      compilerBuildWarning self, "Target for shortcut not declared", pCommand["line"]
   end if
   
   put "shortcut" & tab & tSource & tab & tTarget & return after self["manifest"]
end compilerExecuteShortcut

private command compilerExecutePlace @self, pCommand
   local tTarget
   put compilerEvaluate(self, pCommand["target"], pCommand["line"]) into tTarget
   put pCommand["class"] & tab & tTarget & return after self["manifest"]
end compilerExecutePlace

private command compilerExecuteRegister @self, pCommand
   local tTarget, tValue
   
   -- Target is always present, for class 'key' it is the full key path, for 'value' it is the value of 'parent'.
   put compilerEvaluate(self, pCommand["target"], pCommand["line"]) into tTarget
   
   -- Value may be present, its the value to which the entity is set. Note that
   -- registry values are escaped:
   --   \q -> "
   --   \\ -> \
         if pCommand["value"] is not empty then
   put compilerEvaluate(self, pCommand["value"], pCommand["line"]) into tValue
   replace "\q" with quote in tValue
   replace "\\" with "\" in tValue
end if

local tRoot
put compilerEvaluateOptVar(self, "RegistryRoot") into tRoot
if pCommand["class"] is "key" then
   -- If the 'RegistryRoot' var is not empty, then we prefix the target key with that
   if tRoot is not empty then
      if tTarget is not empty then
         put slash before tTarget
      end if
      put tRoot before tTarget
   end if
   put "registry key" & tab & tTarget & tab & tValue & tab & pCommand["value_type"] & return after self["manifest"]
else
   -- In this case we have a 'parent' key
   local tParent
   if pCommand["parent"] is not empty then
      put compilerEvaluate(self, pCommand["parent"], pCommand["line"]) into tParent
   end if
   if tRoot is not empty then
      if tParent is not empty then
         put slash before tParent
      end if
      put tRoot before tParent
   end if
   put "registry value" & tab & tParent & "//" & tTarget & tab & tValue & tab & pCommand["value_type"] & return after self["manifest"]
end if
end compilerExecuteRegister

private command compilerExecuteRename @self, pCommand
   local tSource, tTarget
   
   put compilerEvaluate(self, pCommand["source"], pCommand["line"]) into tSource
   put compilerEvaluate(self, pCommand["target"], pCommand["line"]) into tTarget
   
   put "rename" & tab & tTarget & tab & tSource & return after self["manifest"]
end compilerExecuteRename

private command compilerExecuteDesktop @self, pCommand
   local tSource, tTarget
   
   put compilerEvaluate(self, pCommand["source"], pCommand["line"]) into tSource
   put compilerEvaluate(self, pCommand["target"], pCommand["line"]) into tTarget
   
   local tSourceType, tSourceFile
   set the itemDelimiter to ":"
   put item 1 of tSource into tSourceType
   put item 2 to -1 of tSource into tSourceFile
   set the itemDelimiter to comma
   
   -- If the source is unknown, throw an error
   if tSourceType is not among the keys of self["sources"] then
      compilerExecutionError self, "Unknown source type - '" & tSourceType & "'", pCommand["line"]
   end if
   
   -- Compute the source path, and if it doesn't exist append a warning
   local tSourcePath
   put self["sources"][tSourceType] & slash & tSourceFile into tSourcePath
   if not abstractThereIsAFile(tSourcePath) then
      compilerExecutionError self, "Item '" & tSourcePath & "' could not be found", pCommand["line"]
   end if
   
   -- If its an application, process the data into a derived file
   if pCommand["class"] is "application" then
      local tIndex
      put the number of elements in self["derived"] + 1 into tIndex
      put "derived/" & "desktop" & "-" & tIndex & ".txt" into self["derived"][tIndex]["name"]
      put "text" into self["derived"][tIndex]["type"]
      
      -- And process the data
      local tData
      repeat for each line tLine in abstractContentsOfTextFile(tSourcePath)
         put compilerEvaluate(self, tLine, pCommand["line"]) & return after tData
      end repeat
      delete the last char of tData
      put tData into self["derived"][tIndex]["data"]
      
      put "desktop-application" & tab & tTarget & tab & self["derived"][tIndex]["name"] & return after self["manifest"]
   else
      put tSourceType & slash & tSourceFile into self["files"]["none:" & tSourcePath]
      put "desktop-icon" & tab & tTarget & tab & tSourceType & slash & tSourceFile & return after self["manifest"]
   end if
end compilerExecuteDesktop

private function compilerEvaluate @self, pValue, pLineNumber
   replace "[[" with return & "[[" in pValue
   replace "]]" with "]]" & return in pValue
   
   local tNewValue
   put empty into tNewValue
   repeat for each line tLine in pValue
      if tLine begins with "[[" and tLine ends with "]]" then
         put char 3 to -3 of tLine into tLine
         put compilerEvaluateVar(self, tLine, pLineNumber) after tNewValue
      else
         put tLine after tNewValue
      end if
   end repeat
   
   return tNewValue
end compilerEvaluate

private function compilerEvaluateVar @self, pVar, pLineNumber
   if pVar is not among the keys of self["context"] then
      compilerExecutionError self, "Unknown variable '" & pVar & "'", pLineNumber
   end if
   return self["context"][pVar]
end compilerEvaluateVar

private function compilerEvaluateOptVar @self, pVar, pLineNumber
   if pVar is not among the keys of self["context"] then
      return empty
   end if
   return self["context"][pVar]
end compilerEvaluateOptVar

################################################################################

-- Normalization analyses the list of actions ensuring that folder creation actions are added as
-- needed to make sure folders exist before files are place within them.
private command compilerNormalizeManifest @self
   local tManifest
   put self["manifest"] into tManifest
   
   -- The commands in the manifest act on two domains - the filesystem and the shortcut system.
   -- First we split the manifest into these two pieces and process them separately.
   local tFileManifest, tShortcutManifest, tPlaceManifest
   set the itemDelimiter to tab
   repeat for each line tLine in tManifest
      get item 1 of tLine
      set the itemDelimiter to comma
      if it is among the items of "rename,shortcut,registry key,registry value" then
         put tLine & return after tShortcutManifest
      else if it is among the items of "record,uninstaller" then
         put tLine & return after tPlaceManifest
      else
         put tLine & return after tFileManifest
      end if
      set the itemDelimiter to tab
   end repeat
   
   -- Next sort the files by the target path in ascending order which means shallower paths will
   -- preceded deeper paths.
   sort tFileManifest ascending by item 2 of each
   put tPlaceManifest after tFileManifest
   
   -- Now we loop through the manifest, ensuring that any use of target folder is preceeded by
   -- its creation. We keep track of which folders do exist to ensure folders are only ever created
   -- once.
   local tCreatedFolders, tNewFileManifest
   put empty into tNewFileManifest
   put empty into tCreatedFolders
   repeat for each line tLine in tFileManifest
      local tTargetPath
      put item 2 of tLine into tTargetPath
      
      local tTargetFolder
      set the itemDelimiter to slash
      put item 1 to -2 of tTargetPath into tTargetFolder
      repeat with i = 1 to the number of items of tTargetFolder
         if not tCreatedFolders[item 1 to i of tTargetFolder] then
            put "folder" & tab & item 1 to i of tTargetFolder & return after tNewFileManifest
            put true into tCreatedFolders[item 1 to i of tTargetFolder]
         end if
      end repeat
      set the itemDelimiter to tab
      put tLine & return after tNewFileManifest
   end repeat
   
   put tNewFileManifest & tShortcutManifest into tManifest
   
   put tManifest into self["manifest"]
end compilerNormalizeManifest

################################################################################

private function compilerProcessItem @self, pProcess, pFile
   local tDerivedFile
   
   if pProcess is "text" then
      put compilerMakeTemporaryFile(self, pProcess) into tDerivedFile
      put compilerProcessText(self, abstractContentsOfBinaryFile(pFile)) into url ("binfile:" & tDerivedFile)
   else if pProcess is "sign" then
      put compilerProcessSign(self, pFile) into tDerivedFile
   else if pProcess begins with "slim-" then
      put compilerProcessSlim(self, pFile, char 6 to -1 of pProcess) into tDerivedFile
   else
      put pFile into tDerivedFile
   end if
   
   return tDerivedFile
end compilerProcessItem

private function compilerProcessText @self, pText
   local tLineEnding
   switch self["context"]["TargetPlatform"]
      case "linux"
         put numToChar(10) into tLineEnding
         break
      case "windows"
         put numToChar(13) & numToChar(10) into tLineEnding
         break
      case "macosx"
         put numToChar(13) into tLineEnding
         break
   end switch
   
   replace numToChar(13) & numToChar(10) with tLineEnding in pText
   replace numToChar(13) with tLineEnding in pText
   replace numToChar(10) with tLineEnding in pText
   
   return pText
end compilerProcessText

private function compilerProcessSlim @self, pInputFile, pTargetArchs
   -- No need to slim or strip inputs any more.
   if true then
      return pInputFile
   end if
   
   local tDerivedFile
   put compilerMakeTemporaryFile(self, "slim")  into tDerivedFile
   
   -- Use the 'diet' internal command to perform the appropriate lipo/strip operation
   local tDietParams
   put abstractPinFile(pInputFile) into tDietParams["input"]
   put tDerivedFile into tDietParams["output"]
   
   -- Note that we use 'i386' in the arch's list here, but the diet command uses 'x86'
   repeat for each item tArch in pTargetArchs
      if tArch is "i386" then
         put "x86" into tArch
      end if
      put true into tDietParams["keep_" & tArch]
   end repeat
   
   -- Now run the diet command
   compilerProgress self, "Stripping/dieting '" & pInputFile & "' to" && pTargetArchs
   --_internal diet macosx tDietParams
   dietAndStrip pTargetArchs, tDietParams["input"], tDietParams["output"]
   
   if the result is "no architectures left" then
      compilerBuildWarning self, "Executable '" & pInputFile & "' does not contain requested architecture(s) " & pTargetArchs
      return empty
   end if
   
   if the result is not empty then
      compilerBuildWarning self, "Diet of executable '" & pInputFile & "' failed -" && the result
      return empty
   end if
   
   return tDerivedFile
end compilerProcessSlim

private function compilerProcessCodesign @self, pFile
   local tFile
   put abstractPinFile(pFile) into tFile
   -- Find the signing identity and sign the app bundle
   get builderFindSigningIdentity()
   if it is empty then
      compilerBuildWarning self, "Signing identity not found"
      return empty
   end if
   
   get shell("/usr/bin/codesign --sign" && word 2 of it && "--verbose=2" && quote & tFile & quote)
   if the result is not zero then
      compilerBuildWarning self, "Signing of executable '" & pFile & "' failed -" && the result
      return empty
   end if
   
   return tFile
end compilerProcessCodesign

private function compilerMakeTemporaryFile @self, pTag
   local tFolder
   put self["options"]["temporary"] into tFolder
   if tFolder  is empty then
      return tempName()
   end if
   
   local tIndex
   put self["last_temp_index"] into tIndex
   add 1 to tIndex
   repeat while there is a file (tFolder & slash & pTag & "-" & tIndex)
      add 1 to tIndex
   end repeat
   put tIndex into self["last_temp_index"]
   
   return tFolder & slash & pTag & "-" & tIndex
end compilerMakeTemporaryFile

################################################################################

private command archiveOpenZip @self, pOutputFile
   put "zip" into self["type"]
   put pOutputFile into self["output"]
   put empty into self["error"]
   
   revZipOpenArchive self["output"], "write"
   if the result is not empty then
      put "on open:" && item 2 to -1 of the result into self["error"]
      exit archiveOpenZip
   end if
   
   put true into self["opened"]
end archiveOpenZip

private command archiveClose @self
   if self["type"] is "zip" then
      if self["opened"] then
         revZipCloseArchive self["output"]
         if the result is not empty then
            put "on close:" && item 2 to -1 of the result into self["error"]
         end if
      end if
   end if
   
   return self["error"]
end archiveClose

private command archiveAddItemWithData @self, pItemName, pData
   if self["error"] is not empty then
      exit archiveAddItemWithData
   end if
   
   if self["type"] is "zip" then
      -- LiveCode 7 has a bug where externals can't access parameter variables
      -- so the data needs to be copied into a local so revZip can access it.
      local tData
      put pData into tData
      revZipAddItemWithData self["output"], pItemName, "tData"
      if the result is not empty then
         put "on add item '" & pItemName & "' as data:" && item 2 to -1 of the result into self["error"]
      end if
   end if
end archiveAddItemWithData

private command archiveAddItemWithFile @self, pItemName, pFile  
   if self["error"] is not empty then
      exit archiveAddItemWithFile
   end if
   
   if self["type"] is "zip" then
      if abstractFileIsPinned(pFile) then
         revZipAddItemWithFile self["output"], pItemName, abstractPinFile(pFile)
      else
         local tData
         put abstractContentsOfBinaryFile(pFile) into tData
         revZipAddItemWithData self["output"], pItemName, "tData"
      end if
      if the result is not empty then
         put "on add item '" & pItemName & "' with file '" & pFile & "':" && item 2 to -1 of the result into self["error"]
      end if
   end if
end archiveAddItemWithFile

################################################################################

local sAbstractFolders
local sAbstractPathCache

private command abstractAddPath pAbstractPath
   put "closed" into sAbstractFolders[pAbstractPath]["state"]
end abstractAddPath

private command abstractEnsureFolderIsReady pFolder
   if sAbstractFolders[pFolder]["state"] is "open" then
      exit abstractEnsureFolderIsReady
   end if
   
   revZipOpenArchive pFolder, "read"
   if the result is not empty then
      throw ",Cannot open source zip '" & pFolder & "'"
   end if
   
   put "open" into sAbstractFolders[pFolder]["state"]
   
   set the itemDelimiter to slash
   
   repeat for each line tItem in revZipEnumerateItems(pFolder)
      put "file" into sAbstractFolders[pFolder]["items"][tItem]["type"]
      repeat with i = 1 to the number of items in tItem - 1
         put "folder" into sAbstractFolders[pFolder]["items"][item 1 to i of tItem]["type"]
      end repeat
   end repeat
end abstractEnsureFolderIsReady

private function abstractPathIsRegular pPath
   -- If we've computed this before then use the cached result
   if pPath is among the keys of sAbstractPathCache then
      return sAbstractPathCache[pPath]["is_regular"]
   end if
   
   -- If the path is prefixed by <folder>/ for an abstract folder
   -- then it is not regular.
   -- If the path is one of the abstract folders then it is not
   -- regular.
   local tIsRegular, tAbstractFolder
   put true into tIsRegular
   repeat for each key tAbstractFolder in sAbstractFolders
      if pPath begins with (tAbstractFolder & slash) or \
            pPath is tAbstractFolder then
         put false into tIsRegular
         exit repeat
      end if
   end repeat
   
   -- Cache the result
   put tIsRegular into sAbstractPathCache[pPath]["is_regular"]
   if not tIsRegular then
      put tAbstractFolder into sAbstractPathCache[pPath]["branch"]
      put char (the length of tAbstractFolder + 2) to -1 of pPath into sAbstractPathCache[pPath]["leaf"]
      
      -- Ensure the abstract folder is open
      abstractEnsureFolderIsReady tAbstractFolder
   end if
   
   return tIsRegular
end abstractPathIsRegular

private command abstractPathDecompose pFile, @rBranch, @rLeaf
   put sAbstractPathCache[pFile]["branch"] into rBranch
   put sAbstractPathCache[pFile]["leaf"] into rLeaf
end abstractPathDecompose

private function abstractThereIsAFile pFile
   abstractCanonicalize pFile
   
   if abstractPathIsRegular(pFile) then
      return there is a file pFile
   end if
   
   local tBranch, tLeaf
   abstractPathDecompose pFile, tBranch, tLeaf
   
   return sAbstractFolders[tBranch]["items"][tLeaf]["type"] is "file"
end abstractThereIsAFile

private function abstractThereIsAFolder pFolder
   abstractCanonicalize pFolder
   
   if abstractPathIsRegular(pFolder) then
      return there is a folder pFolder
   end if
   
   local tBranch, tLeaf
   abstractPathDecompose pFolder, tBranch, tLeaf
   
   return sAbstractFolders[tBranch]["items"][tLeaf]["type"] is "folder"
end abstractThereIsAFolder

private function abstractContentsOfTextFile pFile
   abstractCanonicalize pFile
   
   if abstractPathIsRegular(pFile) then
      return url ("file:" & pFile)
   end if
   
   get abstractContentsOfBinaryFile(pFile)
   replace numToChar(13) & numToChar(10) with numToChar(10) in it
   replace numToChar(13) with numToChar(10) in it
   
   return it
end abstractContentsOfTextFile

private function abstractContentsOfBinaryFile pFile
   abstractCanonicalize pFile
   
   if abstractPathIsRegular(pFile) then
      return url ("binfile:" & pFile)
   end if
   
   local tBranch, tLeaf
   abstractPathDecompose pFile, tBranch, tLeaf
   
   local tData
   if not sAbstractFolders[tBranch]["items"][tLeaf]["has_data"] then
      if not sAbstractFolders[tBranch]["items"][tLeaf]["has_pin"] then
         revZipExtractItemToVariable tBranch, tLeaf, "tData"
         if the result is not empty then
            throw "Could not extract item '" & tLeaf & "' from archive '" & tBranch & "'"
         end if
      else
         put url ("binfile:" & sAbstractFolders[tBranch]["items"][tLeaf]["pin"]) into tData
         if the result is not empty then
            throw "Could read data for item '" & tLeaf & "' from archive '" & tBranch & "'"
         end if
      end if
      
      put true into sAbstractFolders[tBranch]["items"][tLeaf]["has_data"]
      put tData into sAbstractFolders[tBranch]["items"][tLeaf]["data"]
   else
      put sAbstractFolders[tBranch]["items"][tLeaf]["data"] into tData
   end if
   
   return tData
end abstractContentsOfBinaryFile

function abstractPinFile pFile
   abstractCanonicalize pFile
   
   if abstractPathIsRegular(pFile) then
      return pFile
   end if
   
   local tBranch, tLeaf
   abstractPathDecompose pFile, tBranch, tLeaf
   
   local tPinnedFile
   if not sAbstractFolders[tBranch]["items"][tLeaf]["has_pin"] then
      put builderMakeTemporaryFile(sCurrentTempFolder, "pin") into tPinnedFile
      if not sAbstractFolders[tBranch]["items"][tLeaf]["has_data"] then
         revZipExtractItemToFile tBranch, tLeaf, tPinnedFile
         if the result is not empty then
            throw "Could not extract item '" & tLeaf & "' from archive '" & tBranch & "'"
         end if
      else
         put sAbstractFolders[tBranch]["items"][tLeaf]["data"] into url ("binfile:" & tPinnedFile)
         if the result is not empty then
            throw "Could write data for item '" & tLeaf & "' from archive '" & tBranch & "'"
         end if
      end if
      
      put true into sAbstractFolders[tBranch]["items"][tLeaf]["has_pin"]
      put tPinnedFile into sAbstractFolders[tBranch]["items"][tLeaf]["pin"]
   else
      put sAbstractFolders[tBranch]["items"][tLeaf]["pin"] into tPinnedFile
   end if
   
   return tPinnedFile
end abstractPinFile

private function abstractFileIsPinned pFile
   abstractCanonicalize pFile
   
   if abstractPathIsRegular(pFile) then
      return true
   end if
   
   local tBranch, tLeaf
   abstractPathDecompose pFile, tBranch, tLeaf
   
   return sAbstractFolders[tBranch]["items"][tLeaf]["has_pin"]
end abstractFileIsPinned

private function abstractTheItemsOf pPath, pType
   abstractCanonicalize pPath
   
   if abstractPathIsRegular(pPath) then
      local tOldFolder
      put the folder into tOldFolder
      set the folder to pPath
      if pType is "folder" then
         get the folders
      else
         get the files
      end if
      set the folder to tOldFolder
      return it
   end if
   
   local tBranch, tLeaf
   abstractPathDecompose pPath, tBranch, tLeaf
   
   local tItem, tItems
   set the itemDelimiter to slash
   repeat for each key tItem in sAbstractFolders[tBranch]["items"]
      if item 1 to -2 of tItem is not tLeaf then
         next repeat
      end if
      if sAbstractFolders[tBranch]["items"][tItem]["type"] is not pType then
         next repeat
      end if
      put item -1 of tItem & return after tItems
   end repeat
   delete the last char of tItems
   
   return tItems
end abstractTheItemsOf

private function abstractTheFoldersOf pPath
   return abstractTheItemsOf(pPath, "folder")
end abstractTheFoldersOf

private function abstractTheFilesOf pPath
   return abstractTheItemsOf(pPath, "file")
end abstractTheFilesOf

command abstractCanonicalize @xPath
   if char 1 of xPath is not "/" and char 2 of xPath is not ":" then
      put the folder & slash before xPath
   end if
   if the last char of xPath is "/" and xPath is not "/" then
      delete the last char of xPath
   end if
end abstractCanonicalize

################################################################################

private command compilerDump @self
   local tListing
   repeat for each word tType in "installers components"
      put tType & return after tListing
      repeat for each key tName in self[tType]
         put tab & tName & return after tListing
         repeat for each element tCommand in self[tType][tName]
            put tab & tab & tCommand["type"] & return after tListing
            repeat for each key tKey in tCommand
               if tKey is "type" then
                  next repeat
               end if
               put tab & tab & tab & tKey & "=" & tCommand[tKey] & return after tListing
            end repeat
         end repeat
      end repeat
   end repeat
   put tListing
end compilerDump

private command compilerExecutionError @self, pError, pLine
   throw pLine, pError
end compilerExecutionError

private command compilerSyntaxError @self, pError, pLine
   throw pLine, pError
end compilerSyntaxError

private command compilerBuildWarning @self, pError, pLine
   replace "\" with "\\" in pError
   replace return with "\n" in pError
   if pLine is not empty then
      put "warning, line" && pLine & ":" && pError & return after self["warnings"]
   else
      put "warning:" && pError & return after self["warnings"]
   end if
   
   if self["report.target"] is not empty then
      dispatch self["report.handler"] to self["report.target"] with "warning", pError, pLine
   end if
end compilerBuildWarning

private command compilerProgress @self, pMessage
   if self["report.target"] is not empty then
      dispatch self["report.handler"] to self["report.target"] with "message", pMessage
   end if
end compilerProgress

################################################################################

private function sniffExecutable pPath
   if there is no file pPath then
      return "unknown"
   end if
   
   local tMagic
   put readBytesFromFile(pPath, 4) into tMagic
   
   if tMagic is empty then
      return "unknown"
   end if
   
   local tMagicInt, tMagicHex
   get binaryDecode("M", tMagic, tMagicInt)
   put baseConvert(tMagicInt, 10, 16) into tMagicHex
   if tMagicHex is among the items of "feedface,cefaedfe,cafebabe,bebafeca" then
      return "macosx"
   end if
   
   if char 1 to 2 of tMagic is "MZ" then
      return "windows"
   end if
   
   return "unknown"
end sniffExecutable

private function wordToName pWord
   if char 1 of pWord is quote then
      return char 2 to -2 of pWord
   end if
   return pWord
end wordToName

private function readBytesFromFile pPath, pCount
   open file pPath for binary read
   if the result is not empty then
      return empty
   end if
   read from file pPath for pCount bytes
   close file pPath
   return it
end readBytesFromFile

private function createManifest pBase, pPath, pPrefix
   local tManifest, tFolders, tFiles   
   put abstractTheFoldersOf(pBase & slash & pPath) into tFolders
   put abstractTheFilesOf(pBase & slash & pPath) into tFiles
   filter tFolders without ".*"
   filter tFiles without ".*"
   
   if tFiles is empty and tFolders is empty then
      put "folder," & pPath into tManifest
   else
      repeat for each line tFolder in tFolders
         if pPath is empty then
            get createManifest(pBase, tFolder)
         else
            get createManifest(pBase, pPath & slash & tFolder)
         end if
         if it is not empty then
            if tManifest is not empty then
               put return after tManifest
            end if
            put it after tManifest
         end if
      end repeat
      
      repeat for each line tFile in tFiles
         if tManifest is not empty then
            put return after tManifest
         end if
         if pPath ends with "Contents/MacOS" then
            get "executable"
         else
            get "file"
         end if
         set the itemDel to comma
         put it & comma & slash & pPath & slash & tFile after tManifest
      end repeat
   end if
   
   replace "//" with "/" in tManifest
   return tManifest
end createManifest
