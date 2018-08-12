script "EnvironmentAccountSignupPasswordFieldBehaviorButtonBehavior"
on keydown tKey
   local tSChar,tEChar, tPasswd, tBulletCode
   put word 2 of the selectedChunk of me into tSChar
   put word 4 of the selectedChunk of me into tEChar
   put the cPassword of me into tPasswd
   if tEChar > tSChar then -- some text is selected
      delete char tSChar to tEChar of me
      delete char tSChar to tEChar of tPasswd
   end if
   put word 2 of the selectedChunk of me into tSChar
   put word 4 of the selectedChunk of me into tEChar
   put tKey after char tEChar of tPasswd
   set the cPassword of me to tPasswd
   --put tPasswd  # debugging
   if the platform is "MacOS" then 
      put 165 into tBulletCode
   else
      put 149 into tBulletCode
   end if
   
   put numtochar(tBulletCode) after char tEChar of the target
end keyDown

on backspaceKey
   local tSChar,tEChar, tPasswd
   put word 2 of the selectedChunk of me into tSChar
   put word 4 of the selectedChunk of me into tEChar
   put the cPassword of me into tPasswd
   if tEChar > tSChar then -- text is selected
      delete char tSChar to tEChar of me
      delete char tSChar to tEChar of tPasswd
   else
      delete char tEChar of me
      delete char tEChar of tPasswd
   end if
   set the cPassword of me to tPasswd
   
   --put tPasswd # debugging
end backspaceKey

on deleteKey
   local tSChar,tEChar, tPasswd
   put word 2 of the selectedChunk of me into tSChar
   put word 4 of the selectedChunk of me into tEChar
   put the cPassword of me into tPasswd
   if tEChar > tSChar then -- text is selected
      delete char tSChar to tEChar of me
      delete char tSChar to tEChar of tPasswd
   else
      delete char tSChar of me
      delete char tSChar of tPasswd
   end if
   set the cPassword of me to tPasswd
   
   --put tPasswd # debugging
end deleteKey

on openField
   --put the cPassword of me # for debugging
   put bulletize(the cPassword of me) into me
   accountSignupMessageClose
end openField

function bulletize pText
   local tBulletCode, tBullets, tChar
   if the platform is "MacOS" then 
      put 165 into tBulletCode
   else
      put 149 into tBulletCode
   end if
   put empty into tBullets
   repeat for each char tChar in pText
      put numToChar(tBulletCode) after tBullets
   end repeat
   return tBullets
end bulletize
