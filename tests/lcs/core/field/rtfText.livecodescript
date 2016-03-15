script "CoreFieldRtfText"
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

local sLinkStart, sLinkEnd
local sMetadataStart, sMetadataEnd
local sBGColourStart, sBGColourEnd

private command createField
   create field "input"
   
   // Sets the indexes
   put 0 into sLinkStart
   put 0 into sLinkEnd
   put 0 into sMetadataStart
   put 0 into sMetadataEnd
   put 0 into sBGColourStart
   put 0 into sBGColourEnd
   
   put "Some text in a field" into field "input"
end createField

private command setBackgroundColour pStart, pEnd
   put pStart into sBGColourStart
   put pEnd into sBGColourEnd
   set the backgroundcolor of char pStart to pEnd of field "input" to "255,0,0"
end setBackgroundColour

private function checkBGColour
   if sBGColourStart is 0 then
      return true
   else
      return (the backgroundcolor of char sBGColourStart to sBGColourEnd of field "input" is "255,0,0")
   end if
end checkBGColour

private command setMetadata pStart, pEnd
   put pStart into sMetadataStart
   put pEnd into sMetadataEnd
   set the metadata of char pStart to pEnd of field "input" to "foo"
end setMetadata

private function checkMetadata
   if sMetadataStart is 0 then
      return true
   else
      return (the metadata of char sMetadataStart to sMetadataEnd of field "input" is "foo")
   end if
end checkMetadata

private command setLink pStart, pEnd
   put pStart into sLinkStart
   put pEnd into sLinkEnd
   set the textStyle of char pStart to pEnd of field "input" to link
   set the linkText of char pStart to pEnd of field "input" to "www.livecode.com"
end setLink

private function checkLink
   if sLinkStart is 0 then
      return true
   else
      return (the textstyle of char sLinkStart to sLinkEnd of field "input" is link \
            and the linkText of char sLinkStart to sLinkEnd of field "input" is "www.livecode.com")
   end if
end checkLink

// Test the RTF export of the field.
// We set the rtftext of the field to its RTF text - all the styles should be kept
private command testRtfExport pMessage
   set the rtfText of field "input" to the rtfText of field "input"   
   testAssert "RTF export:" && pMessage, checkBGColour() and checkMetadata() and checkLink()
end testRtfExport


////////////////////////////////////////////////////////////////////////////////
// Tests
////////////////////////////////////////////////////////////////////////////////

on TestSetup
   if the platform is "HTML5" then
      return "SKIP bug 16545"
   end if
end TestSetup

// Test backgroundColour on its own
on TestRtfExportWithBackgroundColour
   createField
   setBackgroundColour 6,12
   
   testRtfExport "backgroundColour"
end TestRtfExportWithBackgroundColour

// test Link on its own
on TestRtfExportWithLink
   createField
   
   setLink 6,12
   
   testRtfExport "link"
end TestRtfExportWithLink

// Test Metadata on its own
on TestRtfExportWithMetadata
   createField
   setMetadata 6,12
   
   testRtfExport "metadata"
end TestRtfExportWithMetadata

// test background with link - overlapping
on TestRtfExportWithBackgroundColourAndLink
   createField
   
   setBackgroundColour 6,12
   setLink 11,14
   
   testRtfExport "BackgroundColour and link - overlapping"
end TestRtfExportWithBackgroundColourAndLink

// test backgroundColour with link - overlapping
on TestRtfExportWithLinkAndBackgroundColour
   createField
   
   setLink 6,12
   setBackgroundColour 11,14
   
   testRtfExport "Link and backgroundColour - overlapping"
end TestRtfExportWithLinkAndBackgroundColour

// test backgroundColour with link - same chars
on TestRtfExportWithLinkPlusBackgroundColour
   createField
   
   setLink 6,12
   setBackgroundColour 6,12
   
   testRtfExport "Link and backgroundColour - same range"
end TestRtfExportWithLinkPlusBackgroundColour

// test background with link - same chars
on TestRtfExportWithLinkInBackgroundColour
   createField
   
   setBackgroundColour 6,14
   setLink 11,12
   
   testRtfExport "Link in backgroundColour"
end TestRtfExportWithLinkInBackgroundColour

// test background with link - same chars
on TestRtfExportWithBackgroundColourInLink
   createField
   
   setLink 6,14
   setBackgroundColour 11,12
   
   testRtfExport "BackgroundColour in link"
end TestRtfExportWithBackgroundColourInLink

////////////////////

// test metadata with link - overlapping
on TestRtfExportWithMetadataAndLink
   createField
   
   setMetadata 6,12
   setLink 11,14
   
   testRtfExport "Metadata and link - overlapping"
end TestRtfExportWithMetadataAndLink

// test Metadata with link - overlapping
on TestRtfExportWithLinkAndMetadata
   createField
   
   setLink 6,12
   setMetadata 11,14
   
   testRtfExport "Link and Metadata - overlapping"
end TestRtfExportWithLinkAndMetadata

// test Metadata with link - same chars
on TestRtfExportWithLinkPlusMetadata
   createField
   
   setLink 6,12
   setMetadata 6,12
   
   testRtfExport "Link and Metadata - same range"
end TestRtfExportWithLinkPlusMetadata

// test background with link - same chars
on TestRtfExportWithLinkInMetadata
   createField
   
   setMetadata 6,14
   setLink 11,12
   
   testRtfExport "Link in Metadata"
end TestRtfExportWithLinkInMetadata

// test background with link - same chars
on TestRtfExportWithMetadataInLink
   createField
   
   setLink 6,14
   setMetadata 11,12
   
   testRtfExport "Metadata in link"
end TestRtfExportWithMetadataInLink

///////////////////

// test background with Metadata - overlapping
on TestRtfExportWithBackgroundColourAndMetadata
   createField
   setBackgroundColour 6,12
   setMetadata 11,14
   
   testRtfExport "BackgroundColour and Metadata - overlapping"
end TestRtfExportWithBackgroundColourAndMetadata

// test backgroundColour with Metadata - overlapping
on TestRtfExportWithMetadataAndBackgroundColour
   createField
   
   setMetadata 6,12
   setBackgroundColour 11,14
   
   testRtfExport "Metadata and backgroundColour - overlapping"
end TestRtfExportWithMetadataAndBackgroundColour

// test backgroundColour with Metadata - same chars
on TestRtfExportWithMetadataPlusBackgroundColour
   createField
   
   setMetadata 6,12
   setBackgroundColour 6,12
   
   testRtfExport "Metadata and backgroundColour - same range"
end TestRtfExportWithMetadataPlusBackgroundColour

// test background with Metadata - same chars
on TestRtfExportWithMetadataInBackgroundColour
   createField
   
   setBackgroundColour 6,14
   setMetadata 11,12
   
   testRtfExport "Metadata in backgroundColour"
end TestRtfExportWithMetadataInBackgroundColour

// test background with Metadata - same chars
on TestRtfExportWithBackgroundColourInMetadata
   createField
   
   setMetadata 6,14
   setBackgroundColour 11,12
   
   testRtfExport "BackgroundColour in Metadata"
end TestRtfExportWithBackgroundColourInMetadata

///////////////////

on TestRtfExportWithBackgroundColourInLinkInMetadata
   createField
   
   setMetadata 1,-1
   setLink 6,14
   setBackgroundColour 11,12
   
   testRtfExport "BackgroundColour in Link in Metadata"
end TestRtfExportWithBackgroundColourInLinkInMetadata

on TestRtfExportWithBackgroundColourAndLinkAndMetadata
   createField
   
   setMetadata 1,12
   setLink 6,14
   setBackgroundColour 11,-1
   
   testRtfExport "Metadata and link and backgroundcolour - overlapping"
end TestRtfExportWithBackgroundColourAndLinkAndMetadata

on TestRtfExportWithBackgroundColourPlusLinkPlusMetadata
   createField
   
   setMetadata 6,-1
   setLink 6,-1
   setBackgroundColour 6,-1
   
   testRtfExport "BackgroundColour plus Link plus Metadata - same range"
end TestRtfExportWithBackgroundColourPlusLinkPlusMetadata
