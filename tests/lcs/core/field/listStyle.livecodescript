script "CoreFieldListStyle"
/*
Copyright (C) 2015 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

on TestStyledTextResetsListStyle

open this stack
create fld "myFld"
put "This is some text & return & this is some more text" into fld "myFld"
put "disc,circle,square,decimal,lower latin,upper latin,lower roman,upper roman,skip" into tListStyles
repeat for each item tListStyle in tListStyles
  set the listStyle of char 1 to -1 of fld "myFld" to tListStyle
  put the styledText of fld "myFld" into tStyledText
  set the styledText of fld "myFld" to tStyledText

  TestAssert "setting styledText respects listStyle for '" & tListStyle & "'", the listStyle of char 1 to -1 of fld "myFld" is tListStyle
end repeat

set the listStyle of char 1 to -1 of fld "myFld" to empty
put the styledText of fld "myFld" into tStyledText
set the styledText of fld "myFld" to tStyledText
TestAssert "setting styledText respects listStyle for empty", the listStyle of char 1 to -1 of fld "myFld" is empty

delete fld "myFld"
end TestStyledTextResetsListStyle
