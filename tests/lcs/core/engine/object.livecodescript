script "CoreEngineObjects"
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


on TestCreateButton
   create button "test"
   TestAssert "button created", there is a button "test"
end TestCreateButton

on TestCreateCard
   create card "test"
   TestAssert "card created", there is a card "test"
end TestCreateCard

on TestCreateEPS
   create EPS "test"
   TestAssert "EPS created", there is a EPS "test"
end TestCreateEPS

on TestCreateField
   create field "test"
   TestAssert "field created", there is a field "test"
end TestCreateField

on TestCreateGraphic
   create graphic "test"
   TestAssert "graphic created", there is a graphic "test"
end TestCreateGraphic

on TestCreateGroup
   create group "test"
   TestAssert "group created", there is a group "test"
end TestCreateGroup

on TestCreateImage
   create image "test"
   TestAssert "image created", there is a image "test"
end TestCreateImage

on TestCreatePlayer
   create player "test"
   TestAssert "player created", there is a player "test"
end TestCreatePlayer

on TestCreateScrollbar
   create scrollbar "test"
   TestAssert "scrollbar created", there is a scrollbar "test"
end TestCreateScrollbar

on TestCreateStack
   create stack "test"
   TestAssert "stack created", there is a stack "test"
end TestCreateStack
