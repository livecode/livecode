script "__startup"
constant kBootHash = "@BOOT_HASH@"
constant kEngineVersion = "@ENGINE_VERSION@"

on startup
   -- Directory containing the initial stack files
   local tStartupFolder
   put "/boot/standalone" into tStartupFolder
   
   -- Startup stack file
   local tStartupStack
   put tStartupFolder & slash & "__boot.livecode" into tStartupStack
   
   -- Directory containing auxiliary stack files
   local tAuxFolder
   put "/boot/auxiliary_stackfiles" into tAuxFolder
   
   -- Directory containing extensions
   local tExtensionFolder, tExtensionListFile
   put "/boot/extensions" into tExtensionFolder
   put tExtensionFolder & slash & "extensions.txt" into tExtensionListFile
   
   local tError, tFolder, tFile
   local tStackName, tStackData
   local tModuleList, tModuleName
   try
      -------------------------------------------------------------
      -- Validate engine version
      if the version is not kEngineVersion then
         throw "Engine mismatch: found" && the version & ", expected" && kEngineVersion
      end if
      
      -------------------------------------------------------------
      -- Validate boot stack hash
      open file tStartupStack for binary read
      read from file tStartupStack until end
      close file tStartupStack
      if sha1Digest(it) is not kBootHash then
         throw "Boot stack hash mismatch - standalone is corrupted"
      end if
      
      -------------------------------------------------------------
      -- Load auxiliary stack files and run their load handlers
      if there is a directory tAuxFolder then
         set the defaultfolder to tAuxFolder
         
         repeat for each line tFile in the files
            
            -- This should autoload the file
            put the name of stack tFile into tStackName
            
            -- The revLoadLibrary handler should insert the stack into
            -- the backscripts or frontscripts as necessary.
            send "revLoadLibrary" to stack tStackName
            
         end repeat
      end if
      
      -------------------------------------------------------------
      -- Load extensions
      if there is a file tExtensionListFile then
         
         -- Read list of extensions
         open file tExtensionListFile for "UTF-8" text read
         read from file tExtensionListFile until end
         put it into tModuleList
         close file tExtensionListFile
         
         repeat for each line tModuleName in tModuleList
            put tExtensionFolder & slash & tModuleName into tFolder
            
            load extension from file (tFolder & slash & "module.lcm") \
                  with resource path (tFolder & slash & "resources")
            
            if the result is not empty then
               throw tModuleName & ":" && the result
            end if
         end repeat
      end if
      
   catch tError
   end try
   
   -- Set the initial working directory to the directory that contains
   -- the initial stack.
   set the defaultfolder to tStartupFolder
   
   -- Try to print something vaguely helpful to the the log
   if tError is not empty then
      write "startup failed:" && tError to stderr
   end if
   
   return tError
end startup
