script "CoreFieldHilitedLines"

/*
Copyright (C) 2015 Runtime Revolution Ltd.

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

on TestEmptyInTheBeginning
 local tHilitedLines
 open this stack
 create field "myTestField"
 put 1 & return & 2 & return & 3 into field "myTestField"
 set the listbehavior of field "myTestField" to true
 set the hilitedlines of field "myTestField" to empty,1,2,3
 put the hilitedlines of field "myTestField" into tHilitedLines
 write "HILITEDLINES:" && tHilitedLines & return to stderr
 write "FIELD CONTENTS:" && field "myTestField" & return to stderr
 TestAssert "empty in the beginning", tHilitedLines is "1,2,3"
 delete field "myTestField"
end TestEmptyInTheBeginning


on TestEmptyInTheMiddle
 local tHilitedLines
 open this stack
 create field "myTestField"
 put 1 & return & 2 & return & 3 into field "myTestField"
 set the listbehavior of field "myTestField" to true
 set the hilitedlines of field "myTestField" to 1,empty,3
 put the hilitedlines of field "myTestField" into tHilitedLines
 write "HILITEDLINES:" && tHilitedLines & return to stderr
 TestAssert "empty in the middle", tHilitedLines is "1,3"
 delete field "myTestField"
end TestEmptyInTheMiddle

on TestEmptyInTheEnd
 local tHilitedLines
 open this stack
 create field "myTestField"
 put 1 & return & 2 & return & 3 into field "myTestField"
 set the listbehavior of field "myTestField" to true
 set the hilitedlines of field "myTestField" to 1,2,empty
 put the hilitedlines of field "myTestField" into tHilitedLines
 write "HILITEDLINES:" && tHilitedLines & return to stderr
 TestAssert "empty in the end", tHilitedLines is "1,2"
 delete field "myTestField"
end TestEmptyInTheEnd
