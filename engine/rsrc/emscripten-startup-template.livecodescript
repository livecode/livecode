script "__startup"
constant kBootHash = "@BOOT_HASH@"
constant kEngineVersion = "@ENGINE_VERSION@"

-- Directories that engine expects to normally be present
constant kStandardFolders = "/tmp:/livecode:/boot:/boot/standalone:/boot/fonts"

-- Directory containing the initial stack files
constant kStartupFolder = "/boot/standalone"
-- Startup stack file
constant kStartupStackFile = "/boot/standalone/__boot.livecode"
-- Directory containing auxiliary stack files
constant kAuxStackFolder = "/boot/auxiliary_stackfiles"
-- Auxiliary stack file list
constant kAuxStackListFile = "/boot/auxiliary_stackfiles/auxiliary_stackfiles.txt"
-- Directory containing extensions
constant kExtensionFolder = "/boot/extensions"
-- Extension list file
constant kExtensionListFile = "/boot/extensions/extensions.txt"

on startup
   local tError, tFolder, tFile, tByteString, tByte
   local tStackName, tStackData, tStackHashString
   local tModuleList, tModuleName, tAuxiliaryList
   try
      ----------------------------------------------------------------
      -- Create standard filesystem layout
      set the itemdelimiter to ":"
      repeat for each item tFolder in kStandardFolders
         if there is not a folder tFolder then
            create folder tFolder
            if the result is not empty then
               throw the result
            end if
         end if
      end repeat
      
      -------------------------------------------------------------
      -- Validate engine version
      if the version is not kEngineVersion then
         throw "Engine mismatch: found" && the version & ", expected" && kEngineVersion
      end if
      
      -------------------------------------------------------------
      -- Validate boot stack hash
      open file kStartupStackFile for binary read
      if the result is not empty then
         throw the result
      end if
      read from file kStartupStackFile until end
      put it into tStackData
      if the result is not empty and the result is not "eof" then
         throw the result
      end if
      close file kStartupStackFile
      
      put the sha1Digest of (byte 1 to 1024 of tStackData) into tStackData
      put empty into tStackHashString
      repeat for each byte tByte in tStackData
         put baseConvert(byteToNum(tByte), 10, 16) into tByteString
         switch the number of chars in tByteString
            case 1
               put "0" after tStackHashString
            case 2
               put tByteString after tStackHashString
         end switch
      end repeat
      
      if kBootHash is not tStackHashString then
         throw  "Boot stack hash mismatch - standalone is corrupted"
      end if
      
      -------------------------------------------------------------
      -- Load auxiliary stack files and run their load handlers
      
      if there is a file kAuxStackListFile then
         
         local tAuxStacks
         open file kAuxStackListFile for "UTF-8" text read
         read from file kAuxStackListFile until end
         put it into tAuxStacks
         close file kAuxStackListFile
         
         set the defaultfolder to kAuxStackFolder
         repeat for each line tFile in tAuxStacks
            -- This should autoload the file
            put the name of stack tFile into tStackName
         end repeat  
         
      end if
      
      -------------------------------------------------------------
      -- Load extensions
      if there is a file kExtensionListFile then
         
         -- Read list of extensions
         open file kExtensionListFile for "UTF-8" text read
         read from file kExtensionListFile until end
         put it into tModuleList
         close file kExtensionListFile
         
         repeat for each line tModuleName in tModuleList
            put kExtensionFolder & slash & tModuleName into tFolder
            
            load extension from file (tFolder & slash & "module.lcm") \
                  with resource path (tFolder & slash & "resources")
            
            if the result is not empty then
               throw tModuleName & ":" && the result
            end if
         end repeat
      end if
      
      @STARTUP_SCRIPT@
      
   catch tError
   end try
   
   -- Set the initial working directory to the directory that contains
   -- the initial stack.
   set the defaultfolder to kStartupFolder
   
   -- Try to print something vaguely helpful to the the log
   if tError is not empty then
      write "startup failed:" && tError & return to stderr
   end if
   
   return tError
end startup
