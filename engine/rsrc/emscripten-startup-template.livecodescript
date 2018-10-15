constant kEngineVersion = "@ENGINE_VERSION@"

-- Directories that engine expects to normally be present
constant kStandardFolders = "/tmp:/livecode:/boot:/boot/standalone:/boot/fonts"

-- Directory containing the initial stack files
constant kStartupFolder = "/boot/standalone"

local tError, tFolder
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


   @STARTUP_SCRIPT@

catch tError
end try

-- Try to print something vaguely helpful to the the log
if tError is not empty then
   write "startup failed:" && tError & return to stderr
   throw tError
end if

-- Set the initial working directory to the directory that contains
-- the initial stack.
set the defaultfolder to kStartupFolder
