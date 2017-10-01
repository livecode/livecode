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
      
      if there is not a stack tArgs[tArg] then
         write "Error: stack not found: " & tArgs[tArg] & return to stderr
         quit 3
      end if
            
      local tDocs
      put revDocsGenerateDocsFileFromText(the script of stack tArgs[tArg],the long id of stack tArgs[tArg]) into tDocs
      
      put textEncode(tDocs, "UTF-8") into tDocs
      
      replace numToByte(13) with numToByte(10) in tDocs
      
      put tDocs into url ("binfile:" & tOutputDir & "/" & "api.lcdoc")
   end repeat
   
   quit 0
   	
end extractDocs

on errorDialog tError
   write "Error:" && tError to stderr
   quit 10
end errorDialog
