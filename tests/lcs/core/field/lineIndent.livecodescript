script "CoreFieldLineIndent"
/*
Copyright (C) 2016 LiveCode Ltd.

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

on TestLineIndent

open this stack
create fld "myFld"
put "This is some text & return & this is some more text" into fld "myFld"
repeat for each item tIndent in "firstIndent,leftIndent,rightIndent"
  TestAssert "getting the effective " & tIndent, the effective tIndent of line 2 of fld "myFld" is 0
  
  put random(5) into tIndentValue
  set the tIndent of line 1 of fld "myFld" to tIndentValue
  TestAssert "setting the " & tIndent & " of line 1 of field", the tIndent of line 1 of fld "myFld" is tIndentValue
end repeat


delete fld "myFld"
end TestLineIndent
