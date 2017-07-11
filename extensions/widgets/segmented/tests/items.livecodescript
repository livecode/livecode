script "SegmentedControlItems"
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
   TestLoadExtension "com.livecode.library.widgetutils"
   TestLoadExtension "com.livecode.library.scriptitems"
   TestLoadExtension "com.livecode.widget.segmented"

   create stack
   create widget as "com.livecode.widget.segmented"
   put the long id of it into sWidget
end TestSetup

on TestItemCount
   -- Setting the itemCount should work
   set the itemCount of sWidget to 4
   TestAssert "increase itemCount", the itemCount of sWidget is 4
   set the itemCount of sWidget to 1
   TestAssert "reduce itemCount", the itemCount of sWidget is 1
end TestItemCount

on TestItemNameGeneration
   set the itemDelimiter to ","

   -- Adding segments should assign unique names
   set the itemCount of sWidget to 1
   set the itemCount of sWidget to 2
   TestDiagnostic "Found item names:" && the itemNames of sWidget
   TestAssert "names available", \
         the number of items in the itemNames of sWidget is \
         the itemCount of sWidget
   TestAssert "names generated", \
         (item 1 of the itemNames of sWidget is not \
         item 2 of the itemNames of sWidget)

   -- Adding segments should assign unique names even if the
   -- "default" name is already taken
   local tDefaultItemName
   set the itemCount of sWidget to 1
   set the itemCount of sWidget to 2
   put item 2 of the itemNames of sWidget into tDefaultItemName

   set the itemCount of sWidget to 1
   set the itemNames of sWidget to tDefaultItemName

   set the itemCount of sWidget to 2
   TestDiagnostic "Found item names:" && the itemNames of sWidget
   TestAssert "unique names available", \
         the number of items in the itemNames of sWidget is \
         the itemCount of sWidget
   TestAssert "unique names generated", \
         (item 1 of the itemNames of sWidget is not \
         item 2 of the itemNames of sWidget)
end TestItemNameGeneration

private command TestStringListProperty pPropertyName, pValues, pDefault
   local tPropValue

   set the itemDelimiter to ","
   set the itemCount of sWidget to 2

   -- Setting the property works
   put item 1 to 2 of pValues into tPropValue
   set the pPropertyName of sWidget to tPropValue
   TestAssert merge("correct number of [[pPropertyName]] values"), \
         the number of items in the pPropertyName of sWidget is \
         the itemCount of sWidget
   TestAssert merge("can set [[pPropertyName]]"), \
         the pPropertyName of sWidget is tPropValue

   -- Setting the property to a string with too few items sets the
   -- remaining names to empty
   put item 1 of pValues into tPropValue
   set the pPropertyName of sWidget to tPropValue
   TestDiagnostic pPropertyName & ":" && the pPropertyName of sWidget
   TestAssert merge("correct number of [[pPropertyName]] values even when empty"), \
         the number of items in the pPropertyName of sWidget is \
         the itemCount of sWidget
   TestAssert merge("missing [[pPropertyName]] values get correct default"), \
         item 1 to 2 of the pPropertyName of sWidget is \
         item 1 to 2 of (tPropValue & "," & pDefault & ",,,,")
end TestStringListProperty

on TestItemNames
   TestStringListProperty "itemNames", "fizz,buzz"
end TestItemNames

on TestItemLabels
   TestStringListProperty "itemLabels", "Fizz,Buzz"
end TestItemLabels

on TestItemIcons
   TestStringListProperty "itemIcons", "circle arrow left,circle arrow right"
end TestItemIcons

on TestHilitedItemIcons
   TestStringListProperty "hilitedItemIcons", "arrow left,arrow right"
end TestHilitedItemIcons

on TestItemMinWidths
   TestStringListProperty "itemMinExtents", "250,100", 0
end TestItemMinWidths
