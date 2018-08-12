script "DocsParser"
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

on TestSetup
   TestSkipIfNot "docs"
   
   -- Only run these tests on desktop platforms
   if the platform is not among the items of "MacOS,Windows,Linux" then
      return "SKIP Tests are not runnable on" && the platform
   end if

   local tDocsParser
   put TestGetEngineRepositoryPath() & "/ide-support/revdocsparser.livecodescript" into tDocsParser
   start using stack tDocsParser
end TestSetup

on TestBug16323
   local tDoc
   put "/*" into tDoc
   put return & "Name: OnOpen" after tDoc
   put return & "Type: message" after tDoc
   put return & "Syntax: OnOpen" after tDoc
   put return & "Name: OnClose" after tDoc
   put return & "Type: message" after tDoc
   put return & "Syntax: OnClose" after tDoc
   put return & "*/" after tDoc
   put return & "module com.livecode.widget" after tDoc
   put return & "end module" after tDoc

   local tParsed
   put revDocsGenerateDocsFileFromText(tDoc, "") into tParsed

   local tArray
   put revDocsParseDocTextToLibraryArray("", tParsed, "", "") into tArray

   TestAssert "unwanted syntax element accumulation", tArray["doc"][2]["Syntax"][2] is empty
end TestBug16323

on TestBug17523
   local tModule
   put "widget com.livecode.testdocs" into tModule
   put return & "/**" after tModule
   put return & "Summary: Test" after tModule
   put return & "*/" after tModule
   put return & "property" && quote & "testProp" & quote && "get mTest" after tModule
   put return & "end widget" after tModule
   
   local tParsed
   put revDocsGenerateDocsFileFromText(tModule, "") into tParsed
   
   local tArray
   put revDocsParseDocTextToLibraryArray("", tParsed, "", "") into tArray
   
   TestAssert "property name has quotes stripped", tArray["doc"][1]["display name"] is "testProp"
end TestBug17523
