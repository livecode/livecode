script "CorePrinting"
/*
Copyright (C) 2018 LiveCode Ltd.

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

on TestPDFPrinter
   TestSkipIfNot "external", "revpdfprinter"
   
   local tFile, tStack
   put TestGetTestBuildFolder() & slash & "test.pdf" into tFile
   create stack
   put it into tStack
   open printing to pdf tFile
   print card 1 of tStack
   close printing
   TestAssert "print stack to pdf", there is a file tFile
end TestPDFPrinter

command _TestOpenPrintingBadDest
   local tFile
   put TestGetTestBuildFolder() & slash & "test" into tFile
   open printing to baddest tFile
end _TestOpenPrintingBadDest

on TestOpenPrintingBadDest
   TestAssertThrow "open printing to bad dest throws", \
      _TestOpenPrintingBadDest, the long id of me, \
      EE_PRINT_UNKNOWNDST
end TestOpenPrintingBadDest