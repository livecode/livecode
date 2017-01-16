script "CoreGoStack"
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

on TestGoTwice
   create stack
   lock screen
   go it
   go it
   unlock screen
   TestAssert "go stack twice does not cause crash", true
end TestGoTwice


on TestGoToCard
	create stack
	set the defaultstack to the short name of it
	create card
	set the script of this card to \ 
		"on closeCard; set the cClosed of me to true; end closeCard"
	set the script of card 1 to \
		"on openCard; set the cOpened of me to true; end openCard"
	go card 1
	
	TestAssert "openCard sent to opened card", the cOpened of card 1
	TestAssert "closeCard sent to closed card", the cClosed of card 2
end TestGoToCard