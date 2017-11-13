script "CoreInterfaceGroup"
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

on TestGroupSelection
   create stack "Test"
   set the defaultStack to "Test"

   select empty
   get "ThisIsNotEmpty"
   group
   TestAssert "Group empty selection sets it to empty", it is empty

   create button "A"
   create button "B"
   select button "A" and button "B"
   set the name of the templateGroup to "NewGroup"
   get "ThisIsNotEmpty"
   group
   TestAssert "Group selection sets it to new group", the short name of it is "NewGroup"

   delete stack "Test"
end TestGroupSelection

on TestGroupByScript
   create stack "Test"
   set the defaultStack to "Test"

   create button "A"
   create button "B"
   set the name of the templateGroup to "NewGroup"
   get "ThisIsNotEmpty"
   group button "A" and button "B"
   TestAssert "Group objects sets it to new group", the short name of it is "NewGroup"

   delete stack "Test"
end TestGroupByScript

on TestGroupUndo_Bug20642
   create stack "Test"
   set the defaultStack to "Test"

   create button "A"
   select button "A"
   group
   select the last group
   delete
   undo

   TestAssert "Undo grouped button deletion does not crash", the short name of button "A" is "A"
end TestGroupUndo_Bug20642
