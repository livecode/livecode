script "CoreControlSwitch"
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

on TestStaticSwitch
	switch "foo"
	case "foo"
		get "foo"
		break
	case "bar"
		get "bar"
		break
	default
		get empty
		break
	end switch
	TestAssert "matching first case", it is "foo"

	switch "bar"
	case "foo"
		get "foo"
		break
	case "bar"
		get "bar"
		break
	default
		get empty
		break
	end switch
	TestAssert "matching second case", it is "bar"

	switch "something else"
	case "foo"
		get "foo"
		break
	case "bar"
		get "bar"
		break
	default
		get empty
		break
	end switch
	TestAssert "matching default case", it is empty

	switch "foo"
	case "foo"
		get "foo"
	case "bar"
		get "bar"
		break
	default
		get empty
		break
	end switch
	TestAssert "matching first then fallthrough", it is "bar"

	get empty
	switch "something else"
	case "foo"
		get "foo"
		break
	case "bar"
		get "bar"
		break
	end switch
	TestAssert "matching none no default", it is empty

	switch "something else"
	case "foo"
		get "foo"
		break
	case "bar"
		get "bar"
		break
	default
		get empty
	end switch
	TestAssert "matching none default no break", it is empty

	switch "foo"
	case "foo"
	case "bar"
		get "bar"
		break
	default
		get empty
	end switch
	TestAssert "matching elided first", it is "bar"

	switch "bar"
	case "foo"
	case "bar"
		get "bar"
		break
	default
		get empty
	end switch
	TestAssert "matching elided second", it is "bar"
end TestStaticSwitch
