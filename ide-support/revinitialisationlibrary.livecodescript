script "revinitialisationlibrary"
// A array that stores whether a library is loaded
// sLoadedLibraries["library name"] = true
local sLoadedLibraries

command revInternal__LoadLibrary pLibraryName
   // Create the path to the library file. Assumption is made that the directory, file and stack name are all the same.
   set the itemdel to "/"
   local tLibraryPath
   put revEnvironmentToolsetPath() & "/libraries/" & pLibraryName & "/" & pLibraryName & ".livecodescript" into tLibraryPath
   
   // Check if the stack file exists
   if there is not a file tLibraryPath then return "no file for that library:" && tLibraryPath
   
   try
      // Open the stack
      go stack tLibraryPath
      
      // Get it's name
      local tStackName
      put the name of stack tLibraryPath into tStackName
      
      // Insert the script into the backscript
      send "revLoadLibrary" to stack tStackName
      
      put true into sLoadedLibraries[pLibraryName]
      
      return true
   catch tError
      return "Error while loading stack:" && tLibraryPath
   end try
end revInternal__LoadLibrary

command revInternal__UnloadLibrary pLibraryName
   // Check if the library is loaded
   if pLibraryName is not among the keys of sLoadedLibraries then return "Library not loaded"
   if there is not a stack tStackName then return "library not loaded"
   
   try
      // Request the library shuts down
      send "revUnloadLibrary" to stack tStackName
      
      delete variable sLoadedLibraries[pLibraryName]
      
      return true
   catch tError
      return "Error while unloading library. Library" && pLibraryName && "did not complete the reVUnloadLibrary request"
   end try
end revInternal__UnloadLibrary
