script "CoreFieldTabStops"

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

on TestEmptyInTheBeginning
 local tTabStops
 create field "myTestField"
 set the tabStops of field "myTestField" to empty,10,20,30
 put the tabStops of field "myTestField" into tTabStops
 TestAssert "empty tabStops in the beginning", tTabStops is "0,10,20,30"
 delete field "myTestField"
end TestEmptyInTheBeginning


on TestEmptyInTheMiddle
 local tTabStops
 create field "myTestField"
 set the tabStops of field "myTestField" to 10,empty,20,empty,30
 put the tabStops of field "myTestField" into tTabStops
 TestAssert "empty tabStops in the middle", tTabStops is "10,10,20,20,30"
 delete field "myTestField"
end TestEmptyInTheMiddle

on TestEmptyInTheEnd
 local tTabStops
 create field "myTestField"
 set the tabStops of field "myTestField" to 10,20,empty
 put the tabStops of field "myTestField" into tTabStops
 TestAssert "empty tabStops in the end", tTabStops is "10,20"
 delete field "myTestField"
end TestEmptyInTheEnd

