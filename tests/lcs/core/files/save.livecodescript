script "CoreFilesSave"
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

constant kTestDirectory = "__CoreFilesSave"
constant kTestFile = "__CoreFilesSave/Save.livecode"
constant kVersions = "8.1:7000,8.0:7000,7.0:7000,5.5:5500,2.7:2700"
constant kVersionsWidget = "8.1:8100,8.0:8000,7.0:7000,5.5:5500,2.7:2700"
constant kVersionsBlockOverflow = "8.1:8100,8.0:8000,7.0:7000,5.5:5500,2.7:2700"

constant kMinVersion = "7.0"
constant kWidgetMinVersion = "8.0"
constant kBlockOverflowMinVersion = "8.1"

local sTestStack, sTestStackWidget, sTestStackBlockOverflow

command TestSetup
   create folder kTestDirectory

   create invisible stack "non-widget"
   put it into sTestStack

   create invisible stack "widget"
   put it into sTestStackWidget
   set the defaultstack to sTestStackWidget
   TestLoadExtension "com.livecode.library.widgetutils" --required for clock
   TestLoadExtension "com.livecode.widget.clock"
   create widget as "com.livecode.widget.clock"
   
   create invisible stack "block overflow"
   put it into sTestStackBlockOverflow
   set the defaultstack to sTestStackBlockOverflow
   create field
   local tText
   repeat 40000 times
      put "a" after tText
   end repeat
   set the text of field 1 to tText
end TestSetup

command TestTearDown
   local tFile, tFolder
   put the defaultfolder into tFolder
   set the defaultfolder to kTestDirectory
   repeat for each line tFile in the files
      delete file tFile
   end repeat
   set the defaultfolder to tFolder
   delete folder kTestDirectory
end TestTearDown

private command __TestStackFileHasFormat pVersion
   local tActual, tExpected, tVersion

   set the itemdelimiter to ":"
   put item 1 of pVersion into tVersion
   put "REVO" & item 2 of pVersion into tExpected

   open file kTestFile for binary read
   read from file kTestFile for 8 bytes
   put it into tActual
   close file kTestFile

   TestAssert merge("stack file has version [[tVersion]]"), tActual is tExpected
   TestDiagnostic merge("version was '[[tActual]]'")
end __TestStackFileHasFormat

private command __TestSaveAsFormat pStack, pVersion
   local tVersion, tName
   set the itemdelimiter to ":"
   put item 1 of pVersion into tVersion
   put the name of pStack into tName

   save pStack as kTestFile with format tVersion
   TestAssert merge("save [[tName]] stack file with version [[tVersion]]"), the result is empty

   __TestStackFileHasFormat pVersion
end __TestSaveAsFormat

private command __TestSaveAsNewestFormat pStack, pVersion
   local tName
   put the name of pStack into tName
   save pStack as kTestFile with newest format
   TestAssert "save [[tName]] stack file with newest version", the result is empty

   __TestStackFileHasFormat pVersion
end __TestSaveAsNewestFormat

private command __TestSaveAsGlobalFormat pStack, pVersion
   local tVersion, tName
   set the itemdelimiter to ":"
   put item 1 of pVersion into tVersion
   put the name of pStack into tName

   set the stackfileversion to tVersion
   save pStack as kTestFile
   TestAssert merge("save [[tName]] stack file with version [[tVersion]]"), the result is empty

   __TestStackFileHasFormat pVersion
end __TestSaveAsGlobalFormat

private command __TestSaveAsDefaultFormat pStack, pVersion
   local tName
   put the name of pStack into tName
   save pStack as kTestFile
   TestAssert merge("save [[tName]] stack file with default version"), the result is empty

   __TestStackFileHasFormat pVersion
end __TestSaveAsDefaultFormat

private command __TestMinStackFileVersion pStack, pVersion
   local tVersion, tName
   set the itemdelimiter to ":"
   put item 1 of pVersion into tVersion
   put the short name of pStack into tName
   
   TestAssert merge("stack [[tName]] has minStackFileVersion of [[tVersion]]"), the minStackFileVersion of pStack is tVersion
end __TestMinStackFileVersion

on TestSaveAsFormat
   TestSkipIfNot "write"
   
   local tVersion

   set the itemdelimiter to ","
   repeat for each item tVersion in kVersions
      __TestSaveAsFormat sTestStack, tVersion
   end repeat
   repeat for each item tVersion in kVersionsWidget
      __TestSaveAsFormat sTestStackWidget, tVersion
   end repeat
   repeat for each item tVersion in kVersionsBlockOverflow
      __TestSaveAsFormat sTestStackBlockOverflow, tVersion
   end repeat
end TestSaveAsFormat

on TestSaveAsNewestFormat
   TestSkipIfNot "write"
   local tVersion

   set the itemdelimiter to ","
   __TestSaveAsNewestFormat sTestStack, item 1 of kVersions
   __TestSaveAsNewestFormat sTestStackWidget, item 1 of kVersionsWidget
   __TestSaveAsNewestFormat sTestStackBlockOverflow, item 1 of kVersionsBlockOverflow
end TestSaveAsNewestFormat

on TestSaveAsGlobalFormat
   TestSkipIfNot "write"
   local tVersion

   set the itemdelimiter to ","
   repeat for each item tVersion in kVersions
      __TestSaveAsGlobalFormat sTestStack, tVersion
   end repeat
   repeat for each item tVersion in kVersionsWidget
      __TestSaveAsGlobalFormat sTestStackWidget, tVersion
   end repeat
   repeat for each item tVersion in kVersionsBlockOverflow
      __TestSaveAsGlobalFormat sTestStackBlockOverflow, tVersion
   end repeat
end TestSaveAsGlobalFormat

on TestSaveAsDefaultFormat
   TestSkipIfNot "write"
   
   local tVersion
   set the itemdelimiter to ","
   __TestSaveAsDefaultFormat sTestStack, item 1 of kVersions
   __TestSaveAsDefaultFormat sTestStackWidget, item 1 of kVersionsWidget
   __TestSaveAsDefaultFormat sTestStackBlockOverflow, item 1 of kVersionsBlockOverflow
end TestSaveAsDefaultFormat

on TestMinStackFileVersion
   
   TestSkipIfNot "write"
   set the itemdelimiter to ","
   __TestMinStackFileVersion sTestStack, kMinVersion
   __TestMinStackFileVersion sTestStackWidget, kWidgetMinVersion
   __TestMinStackFileVersion sTestStackBlockOverflow, kBlockOverflowMinVersion
end TestMinStackFileVersion
