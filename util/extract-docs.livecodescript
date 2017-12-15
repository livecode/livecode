#!
 
send "extractDocs" to me
 
on extractDocs
   
   local tDocsParser
   local tOutputDir
   
   local tArgs
   put the commandArguments into tArgs
   put tArgs[2] into tDocsParser
   put tArgs[3] into tOutputDir
   
   try
      start using stack tDocsParser
   catch tError
      write "Error: could not load docs parser:" && tDocsParser & return & tError & return to stderr
      quit 1
   end try
   
   if there is not a folder tOutputDir then
      create folder tOutputDir
   end if
   
   if there is not a folder tOutputDir then
      write "Error: could not create output directory: " & tOutputDir & return to stderr
      quit 2
   end if
   
   local tArg
   repeat with tArg = 4 to the number of elements of tArgs
      local tName, tSource, tContent
      local tDocs
      
      if tArgs[tArg] ends with "lcb" then
         write tArgs[tArg] to stdout
         put "api" into tName
         put revDocsGenerateDocsFileFromModularFile(tArgs[tArg]) into tDocs
      else
         if there is a stack tArgs[tArg] then
            put "api" into tName
            put the script of stack tArgs[tArg] into tContent
            put the long id of stack tArgs[tArg] into tSource
         else if tArgs[tArg] ends with ".lcidl" then
            if the platform is "Win32" then
               set the itemDelimiter to backslash
            else
               set the itemDelimiter to slash
            end if
            put "com.livecode.external." & tolower(char 1 to -7 of the last item of tArgs[tArg]) into tName
            put textDecode(url ("binfile:" & tArgs[tArg]), "UTF8") into tContent
            put tArgs[tArg] into tSource
         else
            write "Error: stack not found: " & tArgs[tArg] & return to stderr
            quit 3
         end if

         put revDocsGenerateDocsFileFromText(tContent, tSource) into tDocs
      end if
      
      put textEncode(tDocs, "UTF-8") into tDocs
      
      replace numToByte(13) with numToByte(10) in tDocs
      
      put tDocs into url ("binfile:" & tOutputDir & "/" & tName & ".lcdoc")
   end repeat
   
   quit 0
   	
end extractDocs

on errorDialog tError
   write "Error:" && tError to stderr
   quit 10
end errorDialog
