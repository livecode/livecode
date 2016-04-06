script "CoreFieldTabWidths"

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
 local tTabWidths
 create field "myTestField"
 set the tabWidths of field "myTestField" to empty,10,20,30
 put the tabWidths of field "myTestField" into tTabWidths
 TestAssert "empty tabWidths in the beginning", tTabWidths is "0,10,20,30"
 delete field "myTestField"
end TestEmptyInTheBeginning


on TestEmptyInTheMiddle
 local tTabWidths
 create field "myTestField"
 set the tabWidths of field "myTestField" to 10,empty,20,empty,30
 put the tabWidths of field "myTestField" into tTabStops
 TestAssert "empty tabWidths in the middle", tTabStops is "10,0,20,0,30"
 delete field "myTestField"
end TestEmptyInTheMiddle

on TestEmptyInTheEnd
 local tTabWidths
 create field "myTestField"
 set the tabWidths of field "myTestField" to 10,20,empty
 put the tabWidths of field "myTestField" into tTabWidths
 TestAssert "empty tabWidths in the end", tTabWidths is "10,20"
 delete field "myTestField"
end TestEmptyInTheEnd

