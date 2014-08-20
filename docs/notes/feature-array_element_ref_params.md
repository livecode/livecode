# Array element pass by reference

It is now possible to pass parts of an array by reference. For example, the following 

`on mouseUp
   local tArray
   put "" into tArray[1][2]
   passByRef tArray[1]
   put tArray[1][2]
end mouseUp

on passByRef @rArray
   put "changed" into rArray[2]
end passByRef`

in the script of a button will result in "changed" appearing in the message box when the button is pressed.

This allows users to reduce the overhead associated with passing sub-arrays to handlers, as this would no longer require copying the sub-array internally.