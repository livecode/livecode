#!
 
send "descriptifyStack" to me
 
on descriptifyStack
   
   local tInputStack
   local tLibURLStack
   local tOutputStack
    
   put $0 into tOutputStack
   put $1 into tInputStack
   
   local tInputStackName
   try
      put the short name of stack tInputStack into tInputStackName
   catch tError
      write "Error: could not open source environment stack:" && tError & return to stderr
      quit 1
   end try

   local tArg, tStack, tName
   repeat with tArg = 2 to $#
      do "put $" & tArg && "into tStack" 
      
      if tStack is empty then
         next repeat
      end if
      
      if there is a stack tStack then
         put the short name of stack tStack into tName
         set the scriptOnly of stack tName to false
         set the mainstack of stack tName to tInputStackName
      else
         write "Error: could not open stack file" && tStack & return to stderr
         quit 2
      end if
   end repeat
   
   set the stackFiles of stack tInputStackName to empty
   
   try
      save stack tInputStack as tOutputStack
   catch tError
      write "Error: could not save updated environment stack:" && tError & return to stderr
      quit 4
   end try
   
   quit 0
   	
end descriptifyStack

on errorDialog tError
   write "Error:" && tError to stderr
   quit 10
end errorDialog
