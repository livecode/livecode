script "SegmentedControlHighlight"
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

local sWidget

on TestSetup
   TestLoadExtension "com.livecode.library.iconsvg"
   TestLoadExtension "com.livecode.library.scriptitems"
   TestLoadExtension "com.livecode.library.widgetutils"
   TestLoadExtension "com.livecode.widget.segmented"

   create widget as "com.livecode.widget.segmented"
   put the long id of it into sWidget
end TestSetup

local sHiliteChanged

on hiliteChanged
   put true into sHiliteChanged
end hiliteChanged

on TestHilitedItems
   ---------- Single highlight
   put false into sHiliteChanged
   set the hilitedItems of sWidget to "2"
   wait 0 with messages
   TestAssert "can set hilitedItems", \
         the hilitedItems of sWidget is "2"
   TestAssert "setting hilitedItems sends hiliteChanged", sHiliteChanged

   put false into sHiliteChanged
   set the hilitedItems of sWidget to the hilitedItems of sWidget
   wait 0 with messages
   TestAssert "setting hilitedItems to same value doesn't send hiliteChanged", \
         not sHiliteChanged

   put false into sHiliteChanged
   set the hilitedItems of sWidget to empty
   wait 0 with messages
   TestAssert "can clear hilitedItems", \
         the hilitedItems of sWidget is empty
   TestAssert "clearing hilitedItems sends hiliteChanged", sHiliteChanged

   set the hilitedItems of sWidget to "1,2"
   TestAssert "only first item of hilitedItems used for single highlight", \
         the hilitedItems of sWidget is "1"

   ---------- Multiple highlight
   set the multipleHilites of sWidget to true
   set the hilitedItems of sWidget to "2,1"
   TestAssert "order in hilitedItems doesn't matter", \
         the hilitedItems of sWidget is "1,2"
   set the hilitedItems of sWidget to "1,2,4"
   TestAssert "unknown segments in hilitedItems are ignored", \
         the hilitedItems of sWidget is "1,2"
   set the hilitedItems of sWidget to "1,2,1,2"
   TestAssert "duplicate segments in hilitedItems are ignored", \
         the hilitedItems of sWidget is "1,2"
end TestHilitedItems

on TestHilitedItemNames
   ---------- Single highlight
   set the hilitedItemNames of sWidget to "segment2"
   TestAssert "can set hilitedItemNames", \
         the hilitedItems of sWidget is "2" and \
         the hilitedItemNames of sWidget is "segment2"

   -- Need to show that it's possible to cope with a segment with an
   -- empty name, so add one
   set the itemCount of sWidget to 4
   set the itemNames of sWidget to (item 1 to 3 of the itemNames of sWidget)

   set the hilitedItemNames of sWidget to ","
   TestAssert "can hilite segment with empty name", \
         the hilitedItems of sWidget is "4" and \
         the number of items in the hilitedItemNames of sWidget is 1 and \
         item 1 of the hilitedItemNames of sWidget is empty

   set the hilitedItemNames of sWidget to empty
   TestAssert "can clear hilitedItemNames", \
         the hilitedItems of sWidget is "" and \
         the number of items in the hilitedItemNames of sWidget is 0 and \
         the hilitedItemNames of sWidget is empty

   ---------- Multiple highlight
   set the multipleHilites of sWidget to true
   set the hilitedItemNames of sWidget to ",segment1"
   TestAssert "order in hilitedItemNames doesn't matter", \
         the hilitedItems of sWidget is "1,4" and \
         the number of items in the hilitedItemNames of sWidget is 2 and \
         item 1 to 2 of the hilitedItemNames of sWidget is item 1 to 2 of "segment1,,,"

   set the hilitedItemNames of sWidget to "segment1,segment2,segment4"
   TestAssert "unknown segments in hilitedItemNames are ignored", \
         the hilitedItems of sWidget is "1,2" and \
         the number of items in the hilitedItemNames of sWidget is 2 and \
         (item 1 to 2 of the hilitedItemNames of sWidget is \
               item 1 to 2 of "segment1,segment2")

   set the hilitedItemNames of sWidget to "segment1,segment2,segment1"
   TestAssert "duplicate segments in hilitedItemNames are ignored", \
         the hilitedItems of sWidget is "1,2" and \
         the number of items in the hilitedItemNames of sWidget is 2 and \
         (item 1 to 2 of the hilitedItemNames of sWidget is \
               item 1 to 2 of "segment1,segment2")

   ---------- Duplicate names
   -- These tests are intended to test the pathological case where
   -- there are duplicate itemNames
   set the itemNames of sWidget to "A,A,B,B"
   set the hilitedItems of sWidget to "2,4"
   TestAssert "hilitedItemNames reported correctly when names duplicated", \
         the hilitedItems of sWidget is "2,4" and \
         the hilitedItemNames of sWidget is "A,B"

   set the hilitedItemNames of sWidget to "B,A"
   TestAssert "setting hilitedItemNames takes first duplicated name", \
         the hilitedItems of sWidget is "1,3" and \
         the hilitedItemNames of sWidget is "A,B"
end TestHilitedItemNames

on TestMultipleHilites
   TestAssert "single highlight by default", \
         the multipleHilites of sWidget is false

   set the multipleHilites of sWidget to true
   set the hilitedItems of sWidget to "1,2,3"

   put false into sHiliteChanged
   set the multipleHilites of sWidget to false
   wait 0 with messages
   TestAssert "setting single highlight resets multiple highlight", \
         the hilitedItems of sWidget is empty
   TestAssert "resetting highlight sends hiliteChanged", \
         sHiliteChanged

   set the multipleHilites of sWidget to true
   set the hilitedItems of sWidget to "2"
   put false into sHiliteChanged
   set the multipleHilites of sWidget to false
   wait 0 with messages
   TestAssert "setting single highlight preserves single highlight", \
         the hilitedItems of sWidget is "2"
   TestAssert "preserving single highlight doesn't send hiliteChanged", \
         sHiliteChanged
end TestMultipleHilites
