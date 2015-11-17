script "CoreGraphicsDashes"

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
 local tDashes
 create graphic "myGraphic"
 set the dashes of graphic "myGraphic" to empty,10,20,30
 put the dashes of graphic "myGraphic" into tDashes
 TestAssert "empty dashes in the beginning", tDashes is "0,10,20,30"
 delete graphic "myGraphic"
end TestEmptyInTheBeginning


on TestEmptyInTheMiddle
 local tDashes
 create graphic "myGraphic"
 set the dashes of graphic "myGraphic" to 10,empty,20,30
 put the dashes of graphic "myGraphic" into tDashes
 TestAssert "empty dashes in the middle", tDashes is "10,0,20,30"
 delete graphic "myGraphic"
end TestEmptyInTheMiddle

on TestEmptyInTheEnd
 local tDashes
 create graphic "myGraphic"
 set the dashes of graphic "myGraphic" to 10,20,empty
 put the dashes of graphic "myGraphic" into tDashes
 TestAssert "empty dashes in the end", tDashes is "10,20"
 delete graphic "myGraphic"
end TestEmptyInTheEnd
