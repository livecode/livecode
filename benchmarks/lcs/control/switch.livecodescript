script "ControlSwitch"
/*
Copyright (C) 2017 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of  the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

on BenchmarkStaticSwitch
	BenchmarkStartTiming "StaticSwitch"
	repeat 1000000 times
		switch "foo"
		case "foo1"
			break
		case "foo2"
			break
		case "foo3"
			break
		case "foo4"
			break
		case "foo5"
			break
		case "foo6"
			break
		case "foo7"
			break
		case "foo8"
			break
		case "foo9"
			break
		case "foo10"
			break
		case "foo11"
			break
		case "foo12"
			break
		case "foo13"
			break
		case "foo14"
			break
		case "foo15"
			break
		case "foo16"
			break
		case "foo17"
			break
		case "foo18"
			break
		case "foo19"
			break
		case "foo20"
			break
		case "foo21"
			break
		case "foo22"
			break
		case "foo23"
			break
		case "foo24"
			break
		case "foo25"
			break
		case "foo26"
			break
		case "foo27"
			break
		case "foo28"
			break
		case "foo29"
			break
		case "foo30"
			break
		case "foo31"
			break
		case "foo32"
			break
		case "foo33"
			break
		case "foo34"
			break
		case "foo35"
			break
		case "foo36"
			break
		case "foo37"
			break
		case "foo38"
			break
		case "foo39"
			break
		case "foo40"
			break
		case "foo41"
			break
		case "foo42"
			break
		case "foo43"
			break
		case "foo44"
			break
		case "foo45"
			break
		case "foo46"
			break
		case "foo47"
			break
		case "foo48"
			break
		case "foo49"
			break
		case "foo50"
			break
		case "foo51"
			break
		case "foo52"
			break
		case "foo53"
			break
		case "foo54"
			break
		case "foo55"
			break
		case "foo56"
			break
		case "foo57"
			break
		case "foo58"
			break
		case "foo59"
			break
		case "foo60"
			break
		case "foo61"
			break
		case "foo62"
			break
		case "foo63"
			break
		case "foo64"
			break
		case "foo"
			break
		default
			break
		end switch
	end repeat
	BenchmarkStopTiming
end BenchmarkStaticSwitch
