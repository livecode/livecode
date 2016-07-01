#!
 
send "updateLibURL" to me
 
on updateLibURL
   
   local tInputStack
   local tLibURLStack
   local tOutputStack
    
   put $0 into tInputStack
   put $1 into tLibURLStack
   put $2 into tOutputStack

   local tSourceDir
   set the itemdel to slash
   put item 1 to -2 of the filename of this stack into tSourceDir
   set the itemdel to comma
   
   try
      go stack tInputStack
   catch tError
      write "Error: could not open source environment stack:" && tError & return to stderr
      quit 1
   end try
   
   local tScript
   if there is a stack tLibURLStack then
      put the script of stack tLibURLStack into tScript
   else
      write "Error: could not open libURL stack" & return to stderr
      quit 2
   end if
   
   set the script of button "revLibURL" of card "Launch" of stack tInputStack to tScript
   if the result is not empty then
      write "Error: could not parse script of libURL:" && the result & return to stderr
      quit 3
   end if
   
   try
      save stack tInputStack as tOutputStack
   catch tError
      write "Error: could not save updated environment stack:" && tError & return to stderr
      quit 4
   end try
   
   quit 0
   	
end updateLibURL

on errorDialog tError
   write "Error:" && tError to stderr
   quit 10
end errorDialog
