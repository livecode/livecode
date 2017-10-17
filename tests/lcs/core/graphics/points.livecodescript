script "CoreGraphicsPoints"

/*
Copyright (C) 2017 LiveCode Ltd.

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


on TestPolygonPoints
 local tPoints, tA, tB
 put "69,226" & CR & "40,257" & CR & "56,285" into tA
 put "40,272" & CR & "71,226" & CR & "93,250" into tB
 
 create graphic "myGraphic"
 set the style of graphic "myGraphic" to "polygon"
 -- CRs in a points list are used to separate different subpolygons
 set the points of graphic "myGraphic" to tA & CR & CR & CR & CR & tB
 put the points of graphic "myGraphic" into tPoints
 -- We should only get one empty line
 TestAssert "point list with multiple CRs", tPoints is tA & CR & CR & tB
 delete graphic "myGraphic"
end TestPolygonPoints

