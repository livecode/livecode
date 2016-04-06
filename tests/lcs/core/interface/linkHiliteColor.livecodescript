script "CoreInterfaceLinkHiliteColor"
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

on TestLinkHiliteColors
   TestAssert "linkhilitecolor default", the linkhilitecolor is "255,0,0"

   set the linkhilitecolor to "blue"
   TestAssert "linkhilitecolor string", the linkhilitecolor is "blue"

   set the linkhilitecolor to "255,255,0"
   TestAssert "linkhilitecolor RGB", the linkhilitecolor is "255,255,0"

   set the linkhilitecolor to "#336633"
   TestAssert "linkhilitecolor hex", the linkhilitecolor is "#336633"

   set the linkhilitecolor to empty
   TestAssert "linkhilitecolor empty", the linkhilitecolor is empty
end TestLinkHiliteColors
