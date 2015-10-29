script "CoreFieldTextAlign"
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

on TestStyledTextResetsTextAlign

open this stack
create fld "myFld"
put "This is some text & return & this is some more text" into fld "myFld"
set the textAlign of char 1 to -1 of fld "myFld" to "center"
put the styledText of fld "myFld" into tStyledText
set the styledText of fld "myFld" to tStyledText

TestAssert "setting styledText respects textAlign", the textAlign of char 1 to -1 of fld "myFld" is "center"

delete fld "myFld"
end TestStyledTextResetsTextAlign
